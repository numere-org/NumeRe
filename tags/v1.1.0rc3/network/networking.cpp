// //////////////////////////////////////////////////
//
//   Notes:  All external methods call SSH___ methods after dealing with
//               any caching or changes in host/user/pass.  It also deals
//               with setting the statusDetails.
//
//   ToDo: pull process stuff out of here
//
// //////////////////////////////////////////////////
#include <wx/regex.h> // for finding the host fingerprint
#include <wx/file.h>
#include <wx/process.h>
#include <wx/txtstrm.h>
#include <wx/progdlg.h>
#include "networking.h"
#include "plinkconnect.h"
#include "../common/datastructures.h"
#include "../common/Options.h"
#include "../common/debug.h"
#include "LocalProcessManager.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif

BEGIN_EVENT_TABLE(Networking, wxEvtHandler)
	EVT_TIMER(-1, Networking::onTimerTick)
    EVT_END_PROCESS(-1, Networking::onTerm)
END_EVENT_TABLE()


//////////////////////////////////////////////////////////////////////////////
///  public constructor Networking
///
///  @param  options Options * Needed for external pathes, and Host/User/Pass
///
///  @return void
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
Networking::Networking(Options* options)
{
	m_options = options;
	m_currHost = m_options->GetHostname();
	m_currUser = m_options->GetUsername();
	m_currPass = m_options->GetPassphrase();

	//Timer:
	m_timer.SetOwner(this);
	bool timerSuccess =	m_timer.Start(POLL_RATE);
	if(!timerSuccess) {
		wxLogDebug("PlinkConnect could not get a timer.\n");
	}

	//Plink:
	m_plinks = new PlinkConnect(m_options->GetPlinkApp(), m_currHost,
								m_currUser, m_currPass);

	m_processManager = new LocalProcessManager();

	m_status = NET_STARTING;
	//GetStatus(); <-- no need to be pre-emptive (here it's best not to be)
}


//////////////////////////////////////////////////////////////////////////////
///  public destructor ~Networking
///
///  @return void
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
Networking::~Networking()
{
	delete m_plinks;
	delete m_processManager;
}


//////////////////////////////////////////////////////////////////////////////
///  public GetHomeDirPath
///  Get the expansion of what the user's home directory is (~)
///
///  @param  homeDir wxString & return value
///
///  @return bool    Whether the call succeeded or not.
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool Networking::GetHomeDirPath(wxString &path)
{
	bool success = false;

	if(GetStatus() == NET_GOOD) {
		// mini-caching:
		if(m_userHomeDir == wxEmptyString)
		{
			if( SSHGetHomeDirPath(path) ) {
				m_userHomeDir = path;
				success = true;
				m_statusDetails = "";
			}
		}
		else {
			// pull for 'cache'
			path = m_userHomeDir;
			success = true;
		}
	}

	return success;
}


//////////////////////////////////////////////////////////////////////////////
///  public GetFileContents
///  Get the contents of a file from the server.
///
///  @param  file     wxFileName  What file to get.
///  @param  contents wxString &  The return value.
///
///  @return bool     Whether the call succeeded or not.
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool Networking::GetFileContents(wxFileName file, wxString &contents)
{
	bool success = false;

	if(GetStatus() == NET_GOOD) {
		if( SSHGetFileContents(file.GetFullPath(wxPATH_UNIX), contents) ) {
			m_statusDetails = "";
			success = true;
		}
	}

	return success;
}


//////////////////////////////////////////////////////////////////////////////
///  public SendFileContents
///  Caution: this will overwrite without confirmation.
///  This is accomplished through the use of pscp.  This deals with storing
///     the contents out to a local temp file, then calling the PSCP-use method
///
///  @param  strng wxString    The contents to send.
///  @param  file  wxFileName  The file to which to send it.
///
///  @return bool  Whether the call succeeded or not.
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool Networking::SendFileContents(wxString strng, wxFileName file)
{
	bool success = false;

	if(GetStatus() == NET_GOOD) {
		// Save the file someplace:
		wxString fname = wxFileName::CreateTempFileName("chm"); // chm?????.tmp
		if(fname != "") {
			wxFile f(fname, wxFile::write);
			if(f.IsOpened()) {
				bool written = f.Write(strng);
				f.Close();
				if(written) {
					// YEAH!, now I can send the file
					if( SCPDoTransfer(fname, file.GetFullPath(wxPATH_UNIX)) ) {
						success = true;
						m_statusDetails = "";
					}

					// Remove the temp file:
					wxRemoveFile(fname);
				}
				else {
					//success = false;
					m_statusDetails = "Could not write to the temp file " + fname;
				}
			}
			else {
				//success = false;
				m_statusDetails = "Could not open the temp file " + fname;
			}
		}
		else {
			//success = false;
			m_statusDetails = "Could not create a temp file";
		}
	}

	return success;
}


//////////////////////////////////////////////////////////////////////////////
///  public GetDirListing
///  Will return the contents of the directory specified.
///  Currently, no caching scheme is being used.  This is where it would go.
///
///  @param  dirPath       wxString     The path of the remote directory.
///  @param  list          DirListing & Return value.
///  @param  forceRefresh  bool         [=false] Whether to force refresh of
///                                                 any caching.
///  @param  includeHidden bool         [=false] Whether to include hidden
///                                                 files in the listing.
///
///  @return bool          Whether the call succeeded or not.
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool Networking::GetDirListing(wxString dirPath, DirListing &listing, bool forceRefresh,
							   bool includeHidden)
{
	bool success = false;

	if(GetStatus() == NET_GOOD) {
		// internally manage dirListing caches to increase speed
		//if(in cache) {
		//	blah
		//}
		//else
		if( SSHGetDirListing(dirPath, listing, includeHidden) ) {
			m_statusDetails = "";
			success = true;
		}
	}

	return success;
}


//////////////////////////////////////////////////////////////////////////////
///  private SSHGetHomeDirPath
///  "cd ~" into the user's home directory, then, if the dir change work (&&),
///      get the present working directory, "pwd".
///
///  @param  homeDir wxString & return value.
///
///  @return bool    Whether the call worked.
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
//Private:
bool Networking::SSHGetHomeDirPath(wxString &path) {
	wxString cmd = "cd ~ && pwd";

	wxString output;

	if( SSHExecSyncCommand(cmd, output) ) {
		output.Remove(output.Length()-2); // remove pwd's EOL("\r\n")
		path = output;
		m_statusDetails = "";
		return true;
	}
	//else
		m_statusDetails = output;
		path = "~";
		return false;
}


//////////////////////////////////////////////////////////////////////////////
///  private SSHGetFileContents
///  Use "cat" to spit out the contents of the file on the shell.
///
///  @param  file     wxString   What file to get.
///  @param  contents wxString & return value
///
///  @return bool     Whether the call succeeded or not.
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
//Private:
bool Networking::SSHGetFileContents(wxString file, wxString &contents)
{
	file.Replace(" ", "\\ ");
	wxString cmd = "cat " + file;


	wxString output;

	if( SSHExecSyncCommand(cmd, output) ) {

		/////////////
		output.Replace("\r\n", "\n", true);
		/////////////

		contents = output;
		m_statusDetails = "";
		return true;
	}
	//else
		m_statusDetails = output;
		contents = "";
		return false;

}


//////////////////////////////////////////////////////////////////////////////
///  private SCPDoTransfer
///  To simplify the transfer of files, I'm just using pscp to transfer the file
///  It would be nice, for consistency and speed, if I could just pipe into a
///     file.
///  It might even behoove us to re-write this to use:
///            "output = ExecuteLocalCommand(cmd);"
///  This ONLY DOES local -> remote transfers.  For speed reasons (needing to
///     authenticate everytime) this is not the optimal solution.
///
///  @param  from_path_name wxString  Local Path and Filename
///  @param  to_path_name   wxString  Remote Path and Filename
///
///  @return bool           Whether the call succeeded or not.
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
//Private:
bool Networking::SCPDoTransfer(wxString from_path_name, wxString to_path_name)
{
	// right now this only does local -> remote transfers
	wxString cmd = m_options->GetPscpApp() // + " -l " + ssh_user
					+ " -pw " + m_currPass + " -batch "
					+ from_path_name + " "
					+ m_currUser + "@" + m_currHost + ":" + to_path_name;
					//+ " && echo Chameleon-Transfer-Success";
	wxProcess* proc = new wxProcess();
	proc->Redirect();
	int exitcode = wxExecute(cmd, wxEXEC_SYNC, proc);

	// Determine success:
	bool success = false;

	if(exitcode == -1) { // Bad Exit -- Could not start process
		m_statusDetails = "Could not start the file transfer process.";
		//success = false;
	}
	else if(exitcode == 0) {
		// Assumed good
		success = true;
		m_statusDetails = "";
	}
	else { // Not bad Exit(hopefully good):
		// Grab the outputs:
		wxString output = "";
		wxString errlog = "";
		wxInputStream* rin = proc->GetInputStream();
		wxInputStream* rerr = proc->GetErrorStream();
		while(!rin->Eof()) {
			output += rin->GetC();
		}
		while(!rerr->Eof()) {
			errlog += rerr->GetC();
		}

		// Determine the success:
		if(output.Contains("Chameleon-Transfer-Success")) {
			success = true;
			m_statusDetails = "";
		}
		else {
			m_statusDetails = output + errlog;
		}
	}
	delete proc;

	return success;
}


//////////////////////////////////////////////////////////////////////////////
///  private SSHGetDirListing
///  Get the listing of the directory specified by doing a "find" on the
///     remote server, and parsing the output (via parsefindoutput).
///  It would be nice to use only "ls" instead of "find".  But, instead of
///      dealing with parsing(or regex'ing) an "ls -l" for what's a dir, and
///      what isn't, I use "find".
///  Try using "ls --color=never -Q --format=vertical -1 -A"
///
///  @param  dirPath       wxString     What directory.
///  @param  listing       DirListing & return value
///  @param  includeHidden bool         [=false] Whether to include hidden files
///
///  @return bool          Whether the call succeeded or not.
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
//Private:
bool Networking::SSHGetDirListing(wxString dirPath, DirListing &listing,
									bool includeHidden)
{
	wxString cmd = "cd " + dirPath
					+ " && find -maxdepth 1 -type f "
					+ " && echo N_E_TwOrKiNg-DiRs"
					+ " && find -maxdepth 1 -type d";
	wxString output;

	if( SSHExecSyncCommand(cmd, output) ) {
		// if returned true, the token will be there
		int tokenPos = output.Find("N_E_TwOrKiNg-DiRs\r\n");
		wxString files = output.Left(tokenPos); // +1
		wxString dirs = output.Mid(tokenPos+19);

		listing.fileNames = ParseFindOutput(files, includeHidden);
		listing.dirNames = ParseFindOutput(dirs, includeHidden);

		return true;
	}
	//else
		m_statusDetails = output;
		return false;
}


//////////////////////////////////////////////////////////////////////////////
///  private ParseFindOutput
///  This parsing can easily be broken!  But, I am pretty confident
///    in the integrity of the string I'm parsing, because I know
///    what's coming in.
///  This does both files and directories because they are structured/listed
///    the same.
///
///  @param  strng         wxString  The whole "find" output.
///  @param  includeHidden bool      Whether to include hidden things (./.foo)
///                                     in the dirlisting
///
///  @return wxArrayString return value.
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
//Private:
wxArrayString Networking::ParseFindOutput(wxString strng, bool includeHidden)
{
	wxArrayString r;
	r.Empty();

	while(strng.Length() > 2) {

		// Get the "./"
		char c1 = strng.GetChar(0); // peek
		char c2 = strng.GetChar(1); // peek
		strng.Remove(0,2); // pop

		if(c1 == '.' && c2 == '\r') {
			// Special case for . direcrtory
			strng = "\r" + strng; // push it back on
			c2 = '/';
		}

		//if(c1 != '.' && c2 != '/') {
		//	return r; // unexpected
		//}

		// Queue-up/Initialize for the loop
		// There should be at least 2 left: \r\n
		c1 = strng.GetChar(0);
		c2 = strng.GetChar(1);
		strng.Remove(0,2);

		wxString newitem = "";

		while(c1 != '\r' && c2 != '\n') {
			newitem += c1;
			c1 = c2;
			c2 = strng.GetChar(0); // peek
			strng.Remove(0,1); // pop
		}

		if(newitem != "") {
			if(newitem.Left(1) != "." || includeHidden) {
				r.Add(newitem);
			}
		}
	}

	return r;
}


//////////////////////////////////////////////////////////////////////////////
///  private SSHExecSyncCommand
///  Passes the command off to PlinkConnect after appending a boolean test to
///     know if the command completely successfully.
///
///  @param  command wxString   command to execute
///  @param  output  wxString & output from the command.
///
///  @return bool    Whether the call succeeded or not.
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
//Private:
bool Networking::SSHExecSyncCommand(wxString command, wxString &output) {
	command += " && echo Su_CC_ess-CMD";

	output = m_plinks->executeSyncCommand(command);

	int tokenPos = output.Find("Su_CC_ess-CMD");
	if(tokenPos != -1) {
		output.Remove(tokenPos);
		return true;
	}
	// else
		return false;
}


//////////////////////////////////////////////////////////////////////////////
///  private MaintainSettings
///  Not meant to update my status -- just my settings (if necessary)
///  This returns true if any settings have changed.  I need to do this
///     polling because Options is passive (it doesn't tell me when it's
///     changed).
///
///  @return bool Returns true if anything has changed in Options.
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
//Private:
bool Networking::MaintainSettings() {
	wxString newH = m_options->GetHostname();
	wxString newU = m_options->GetUsername();
	wxString newP = m_options->GetPassphrase();

	if(newH != m_currHost || newU != m_currUser || newP != m_currPass ) {
		// I don't check for plinkApp changes because I don't keep track of those
		// Something has changed
		m_currHost = newH;
		m_currUser = newU;
		m_currPass = newP;

		//Update PlinkConnect
		m_plinks->setLogin(newH, newU, newP); // this restarts the network
		m_status = NET_STARTING;

		m_userHomeDir = wxEmptyString;

		return true;
	}
	//else
	  return false;

}


//////////////////////////////////////////////////////////////////////////////
///  public GetStatus
///    NET_STARTING = NEVER used externally
///    NET_GOOD = good to go!
///    NET_UNKNOWN_HOST = host finger print not in cache
///    NET_CONN_REFUSED = host wouldn't allow a connection
///    NET_AUTH_FAILED = user+pass did not work on host
///    NET_ERROR_MESSAGE = default/catch-all
///  For more information, also check GetStatusDetails
///
///  @return NetworkStatus status
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
NetworkStatus Networking::GetStatus()
{
	if(MaintainSettings() || m_status == NET_STARTING) {
		// Update the status
		if(m_plinks->getIsConnected()) {
			m_status = NET_GOOD;
		}
		else {
			wxString message = m_plinks->getMessage();

			m_status = NET_ERROR_MESSAGE; //default
			m_statusDetails = message;

			if(message.Contains("key is not cached")) {
				// host finger print not in cache
				m_status = NET_UNKNOWN_HOST;

				// Grab the Fingerprint:
				wxRegEx reFingerprint("[[:digit:]]+[[:blank:]]+([[:xdigit:]]{2}:)+[[:xdigit:]]{2}");

				if(reFingerprint.Matches(message))
				{
					//wxString match = reFingerprint.GetMatch(message);
					//wxLogDebug("Matched fingerprint: \"%s\"", match);
					m_statusDetails = reFingerprint.GetMatch(message);
				}
				else
				{
					//wxLogDebug("Failed to match the fingerprint in: %s", errlog);
					m_statusDetails = "*unknown* - could not parse fingerprint";
				}
			}
			else if(message.Contains("Unable to authenticate")) {
				// user+pass did not work on host
				m_status = NET_AUTH_FAILED;
			}
			else if(message.Contains("Connection refused")) {
				// host wouldn't allow a connection
				m_status = NET_CONN_REFUSED;
			}
		}
	}
	return m_status;
}


//////////////////////////////////////////////////////////////////////////////
///  public GetStatusDetails
///    NET_STARTING = nothing
///    NET_GOOD = nothing
///    NET_UNKNOWN_HOST = host finger print
///    NET_CONN_REFUSED = stderr message
///    NET_AUTH_FAILED = stderr message
///    NET_ERROR_MESSAGE = message
///
///  @return wxString details
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxString Networking::GetStatusDetails()
{
	return m_statusDetails;
}


//////////////////////////////////////////////////////////////////////////////
///  public SSHCacheFingerprint
///  Basically, this means that the PlinkConnect will not work until the
///      fingerprint gets cached.  To do so, I run my own, send a "y" to accept
///      the cache then immediately kill the process.
///  I am assuming!!! that the connection should work, and I'm just doing this
///     to accept the cache.
///
///  @return void
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool Networking::SSHCacheFingerprint()
{
	//Start the program asynchonously, answer "y" to caching, then Kill it:

	wxString cmd = m_options->GetPlinkApp() + " " + m_currHost;
	wxProcess* proc = new wxProcess(0);
	proc->Redirect();
	long pid = wxExecute(cmd, wxEXEC_ASYNC, proc);
	wxTextOutputStream pout(*proc->GetOutputStream(), wxEOL_UNIX);
	pout << "y\recho \"Successful Login\"\r";

	int i = 0;

	wxProgressDialog* progress = nullptr;

	wxString output;

	while(i < 240)
	{
		wxMilliSleep(250);

		if(proc->IsInputAvailable())
		{
			wxInputStream* pStdOut = proc->GetInputStream();
			while(proc->IsInputAvailable()) {
				output += pStdOut->GetC();
			}
			break;
		}

		if(progress != NULL)
		{
			progress->Pulse();
		}

		if(i % 4 == 0)
		{
			wxLogDebug("Synchronous network operation (getting fingerprint): %d", i / 4);

			if(i > 12  && progress == NULL)
			{
				progress = new wxProgressDialog("Network Operation", "This network connection may take a while...");
			}
		}

		i++;
		wxSafeYield();
	}

	if(progress != NULL)
	{
		delete progress;
	}

	wxKill(pid, wxSIGKILL);

	// must have waited for a minute with no result
	if(output.Length() == 0)
	{
		wxMessageBox("Caching the fingerprint timed out.  Try logging in with Putty, then run Chameleon again",
					"Network Error", wxOK | wxCENTRE | wxICON_ERROR);
		return false;
	}
	else
	{
		wxLogDebug("Login presumed successful.  Output: %s", output);
	}

	m_plinks->setLogin("","",""); // terminate all connections, and do a spawn
	m_status = NET_STARTING;

	return true;
}


//////////////////////////////////////////////////////////////////////////////
///  public PingOptions
///  This is a way that Options* can tell Networking it has changed.
///  This will not (must not) go into any wait loops.
///
///  @return void
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void Networking::PingOptions() {
	// This function must NOT be SYNCHRONOUS
	//  (especially not locking -- wxSafeYield)

	// If nothing has changed, nothing will happen.
	if( MaintainSettings() || m_status == NET_STARTING) {
		// If settings have changed, the changes will be propagated to PlinkConnect,
		//     then (asynchronously) PlinkConnect will spawn a connection (if possible)
		if(m_plinks->getIsSettled()) {
			// only do this if I know I will can an immediate response
			GetStatus();
		}
	}
}




//============================================================================
//                        PROCESS MANAGEMENT stuff
//============================================================================




//////////////////////////////////////////////////////////////////////////////
///  public StartCommand
///  This is for asynchronous interaction.
///  Extremely simple: if isRemote is true, it calls StartRemoteCommand,
///     otherwise, it calls StartLocalCommand.
///
///  @param  isRemote             bool           Whether command should be run remotely or locally
///  @param  cmd                  wxString       The commandline to run
///  @param  owner                wxEvtHandler * The object to notify when stuff happens
///
///  @return wxTextOutputStream * The new process's stream, so the owner can send stuff to the process
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxTextOutputStream* Networking::StartCommand(bool isRemote, wxString cmd,
											 wxEvtHandler* owner)
{
	if(isRemote) {
		return StartRemoteCommand(cmd, owner);
	}
	else {
		return StartLocalCommand(cmd, owner);
	}
}


//////////////////////////////////////////////////////////////////////////////
///  public StartRemoteCommand
///  This is for asynchronous interaction.
///  Passes this off to PlinkConnect. [see PlinkConnect::executeCommand()]
///
///  @param  cmd                  wxString       command
///  @param  owner                wxEvtHandler * event receiver
///
///  @return wxTextOutputStream * stdin
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxTextOutputStream* Networking::StartRemoteCommand(wxString cmd, wxEvtHandler* owner)
{
	return m_plinks->executeCommand(cmd, owner);
}


//////////////////////////////////////////////////////////////////////////////
///  public StartLocalCommand
///  Will run a local command, once implemented
///
///  @param  cmd                  wxString       The command to execute
///  @param  owner                wxEvtHandler * The object to notify when stuff happens
///
///  @return wxTextOutputStream * The new process's stream, so the owner can send stuff to it
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxTextOutputStream* Networking::StartLocalCommand(wxString cmd, wxEvtHandler* owner)
{
	//wxLogDebug("Local Process Execution still missing.");
	//return NULL;
	return m_processManager->StartLocalCommand(cmd, owner);
}


//////////////////////////////////////////////////////////////////////////////
///  public ExecuteCommand
///  This starts a command, and waits until it has come to completion.
///  Extremely simple: if isRemote is true, it calls ExecuteRemoteCommand,
///     otherwise, it calls ExecuteLocalCommand.
///
///  @param  isRemote bool      Whether this is to be executed remotely.
///  @param  cmd      wxString  What command to execute.
///
///  @return wxString The resulting output
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxString Networking::ExecuteCommand(bool isRemote, wxString cmd)
{
	if(isRemote) {
		return ExecuteRemoteCommand(cmd);
	}
	else {
		return ExecuteLocalCommand(cmd);
	}
}


//////////////////////////////////////////////////////////////////////////////
///  public ExecuteRemoteCommand
///  This starts a command, and waits until it has come to completion.
///
///  @param  cmd      wxString  The command to execute
///
///  @return wxString The resulting output
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxString Networking::ExecuteRemoteCommand(wxString cmd)
{
	return m_plinks->executeSyncCommand(cmd);
}


//////////////////////////////////////////////////////////////////////////////
///  public ExecuteLocalCommand
///  This will (when implemented) execute a command locally, and wait until it comes to completion
///
///  @param  cmd      wxString  The command to execute
///
///  @return wxString The resulting output
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxString Networking::ExecuteLocalCommand(wxString cmd)
{
	wxLogDebug("Local Synchronous Process Execution still missing.");
	return wxEmptyString;
}


//////////////////////////////////////////////////////////////////////////////
///  public ForceKillProcess
///  This is certainly not a common-sense thing to pass when desiring to
///     terminate a process
///
///  @param  w    wxTextOutputStream * The output stream from the process to kill
///
///  @return void
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void Networking::ForceKillProcess(wxTextOutputStream* w)
{
	//Walk m_processes:
	bool isLocal = false;
	//for(ProcessInfoList::Node* node = m_processes.GetFirst(); node; node = node->GetNext() ) {
	//	p = node->GetData();
	//	if(p->pid == pid) {
	//		isLocal = true;
	//		break;
	//	}
	//}

	if(isLocal) {
		//
	}
	else {
		m_plinks->ForceKillProcess(w);
	}
}

ProcessInfo* Networking::GetProcessInfo(wxTextOutputStream* w)
{
	return m_plinks->FindProcess(w);
}


//Private:
void Networking::onTerm(wxProcessEvent &e) {
	//
}


//Private:
void Networking::onTimerTick(wxTimerEvent &e) {
	//

	m_plinks->PollTick();
}



bool Networking::DoingSynchronousOperation()
{
	return m_plinks->DoingSynchronousOperation();
}
