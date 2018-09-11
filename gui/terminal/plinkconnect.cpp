// ////////////////////////////////////////
//
// Notes: There is always an available process -- the last in the list.  If I
//            start to use one, I open another one.
//        I designate methods as "ASYNCHRONOUS" if they don't have a wait-loop
//        If I assume that having had at least one connection determines a new
//            one will be ok, this might become asynchronous?
//
// Todo: Determine return status of run programs
//       Change struct ProcessInfo into a class (use good defaults & a destructor)
//       Instead of always killing and opening processes, I could keep some live
//
// //////////////////////////////////////
#include <wx/txtstrm.h>
#include <wx/process.h>
#include <wx/progdlg.h>

#include <wx/listimpl.cpp> // CAREFUL!
#include "plinkconnect.h"
#include "../../common/chameleonprocessevent.h"
#include "../../common/datastructures.h"
#include "../../common/debug.h"

#ifdef _DEBUG
	#define new DEBUG_NEW
#endif

#define PROCESSLIST


#ifdef _PC_INTERNAL_TIMER_ //#ifdef's not allowed inside Event Table
BEGIN_EVENT_TABLE(PlinkConnect, wxEvtHandler)
	EVT_TIMER(-1, PlinkConnect::OnTimerEvent)
    EVT_END_PROCESS(-1, PlinkConnect::onTerminate)
END_EVENT_TABLE()
#else
BEGIN_EVENT_TABLE(PlinkConnect, wxEvtHandler)
    EVT_END_PROCESS(-1, PlinkConnect::onTerminate)
END_EVENT_TABLE()
#endif

// List Type:
WX_DEFINE_LIST(ProcessInfoList);


// ASYNCHRONOUS
//////////////////////////////////////////////////////////////////////////////
///  public constructor PlinkConnect
///  Calls spawnConnection to jumpstart my connection/s
///
///  @param  plinkApp wxString  path+exec of plink.exe
///  @param  host     wxString  hostname
///  @param  user     wxString  username
///  @param  pass     wxString  password (no quotes allowed)
///
///  @return void
///
///  @author David Czechowski @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
PlinkConnect::PlinkConnect(wxString plinkApp, wxString host, wxString user, wxString pass)
{
	m_plinkApp = plinkApp; // essentially this is assumed accurate, and can't change
	m_isConnected = false;
	m_synchronous = false;

#ifdef _PC_INTERNAL_TIMER_
	m_timer.SetOwner(this);
	bool timerSuccess =	m_timer.Start(POLL_RATE);
	if(!timerSuccess) {
#ifdef DO_LOG
		wxLogDebug("PlinkConnect could not get a timer.\n");
#endif
	}
#endif

	m_host = host;
	m_user = user;
	m_pass = pass;

	if(pass != "")
	{
		spawnConnection(); // jumpstart the 1-always-open-connection
							//   this will also establish my status

	}

}


//////////////////////////////////////////////////////////////////////////////
///  public destructor ~PlinkConnect
///  Will call terminateAllConnections, then wait in a loop until everything
///     has terminated
///  SYNCHRONOUS
///
///  @return void
///
///  @author David Czechowski @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
PlinkConnect::~PlinkConnect()
{
	// Send NO more events:





#ifdef PROCESSLIST
	for(ProcessInfoList::Node* node = m_processes.GetFirst(); node; node = node->GetNext() ) {
		ProcessInfo* p = node->GetData();
		if(p->owner != NULL) {
#ifdef DO_LOG
			wxLogDebug("BAD BAD - PlinkConnect Deleted with a process still live!");
#endif
			p->owner = NULL;
		}
	}
#else
	for(int i = 0; i < m_procs.GetCount(); i++)
	{
		ProcessInfo* p = m_procs[i];

		if(p->owner != NULL) {
#ifdef DO_LOG
			wxLogDebug("BAD BAD - PlinkConnect Deleted with a process still live!");
#endif
			p->owner = NULL;
		}
	}
#endif

	terminateAllConnections();

#ifdef PROCESSLIST
	while(m_processes.GetCount() != 0) {
		// wait for the processes to remove themselves
		wxSafeYield();
	}
#else
	while(m_procs.GetCount() != 0) {
		// wait for the processes to remove themselves
		wxSafeYield();
	}
#endif



}


//////////////////////////////////////////////////////////////////////////////
///  public setLogin
///  Anything that is "" will be left unchanged.
///  This will reset and restart the connection/s whether things have changed
///     or not.  Which means this can be used to do a reset (passing all "").
///     If hostname, username, or pass change, it only makes sense to reset.
///  ASYNCHRONOUS
///
///  @param  host   wxString  new hostname
///  @param  user   wxString  new username
///  @param  pass   wxString  new passphrase
///
///  @return void
///
///  @author David Czechowski @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void PlinkConnect::setLogin(wxString host, wxString user, wxString pass)
{
	// Set appropriate values:
	if(host != "") {
		m_host = host;
	}
	if(user != "") {
		m_user = user;
	}
	if(pass != "") {
		#ifdef _DEBUG
			if(pass.Contains("\"")) {
				// this user's passphrase just isn't going to work
#ifdef DO_LOG
				wxLogDebug("Passwords with \" will not work! -- Unstable state entered.");
#endif
			}
		#endif
		m_pass = "\"" + pass + "\"";
	}

	// No if-changed test - otherwise, why was this method called

	// Reset:
	terminateAllConnections();

	// Start:
	spawnConnection(); // try to start the 1-always-open-connection
}


//////////////////////////////////////////////////////////////////////////////
///  private spawnConnection
///  This will create a process and append it to the end of the list of processes
///  This also affects my status, if stating plink.exe fails
///  ASYNCHRONOUS
///
///  @return void
///
///  @author David Czechowski @date 04-23-2004
//////// comment generated by Comment Maker from www.FeinSoftware.com /////////
//Private:
void PlinkConnect::spawnConnection()
{
	wxProcess* proc = new wxProcess(this);
	proc->Redirect();

	wxString p = m_pass;
	if(m_pass == "") {
		//p = "password";
		return;
	}

	wxString cmd = m_plinkApp
		+ " -pw " + p + " "
		+ m_user + "@" + m_host;

	long pid = wxExecute(cmd, wxEXEC_ASYNC, proc);

	if(pid == 0) {
		//Command could not be executed
#ifdef DO_LOG
		wxLogDebug("Could not start a Plink process -- Command could not be executed.");
#endif
		m_message = "Could not start a Plink process -- Command could not be executed.";
		m_isConnected = false;
		//delete proc not needed because ASYNC
	}
	else if (pid == -1) {
		// BAD ERROR!  User ought to upgrade their operating system
		// User has DDE running under windows (OLE deprecated this)
#ifdef DO_LOG
		wxLogDebug("Could not start a Plink process -- DDE in use.");
#endif
		m_message = "Could not start a Plink process -- DDE in use.";
		m_isConnected = false;
		//delete proc not needed because ASYNC
	}
	else { // Process is Live
		//wxLogDebug("Started another Plink connection - pid(%d)", pid);
		ProcessInfo* p = new ProcessInfo;
		p->pid = pid;
		p->proc = proc;
		p->state = PC_STARTING;
		p->stdinStream = new wxTextOutputStream(*proc->GetOutputStream(), wxEOL_UNIX);
		p->outputBuf = "";
		p->owner = NULL; // set when process gets used

#ifdef PROCESSLIST
		m_processes.Append(p);
#else
		m_procs.Add(p);
#endif

		// Pre-emptively pump in an echo command, which will signal
		//   a successful connection
		*(p->stdinStream) << "echo \"Successful Login\"" << endl;
	}

	return;
}


//////////////////////////////////////////////////////////////////////////////
///  public getIsConnected
///  If I am "Connected" it means, I can execute things remotely.
///  This is SYNCHRONOUS because if I'm not in a settled state yet, I go into
///     a wait loop
///
///  @return bool I'm either connected or not
///
///  @author David Czechowski @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
bool PlinkConnect::getIsConnected()
{
	int i = 0;

	wxProgressDialog* progress = NULL;

	wxProcessInfoListNode* first; //= m_processes.GetFirst();

#ifdef PROCESSLIST
	while( (first = m_processes.GetFirst()) != NULL && first->GetData()->state == PC_STARTING) {
#else
	while( m_procs.GetCount() > 0 && m_procs[0]->state == PC_STARTING)
	{
#endif
		// If the first connection is in the starting
		//   state, wait till "the dust settles"
		wxMilliSleep(250);

		if(progress != NULL)
		{
			progress->Pulse();
		}

		if(i % 4 == 0)
		{
#ifdef DO_LOG
			wxLogDebug("Synchronous network operation (getIsConnected): %d", i / 4);
#endif

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

	return m_isConnected;
}


//////////////////////////////////////////////////////////////////////////////
///  public getIsSettled
///  The use of this would be in similar situations as getIsConnected, except
///     this is ASYNCHRONOUS.  "Settled" simply means, that an isConnected
///     status check will not go into a wait-loop
///
///  @return bool If true, getIsConnected can be run without entering a wait-
///               loop.
///
///  @author David Czechowski @date 04-23-2004
//////// comment generated by Comment Maker from www.FeinSoftware.com /////////
bool PlinkConnect::getIsSettled()
{
	bool settled;

#ifdef PROCESSLIST
	settled = m_processes.GetFirst() != NULL && m_processes.GetFirst()->GetData()->state != PC_STARTING;
#else
	settled = (m_procs.GetCount() > 0 && m_procs[0]->state != PC_STARTING);
#endif

	return settled;
}


//////////////////////////////////////////////////////////////////////////////
///  public executeCommand
///  Used to start an Asynchronous process on the server.  All remaining inter
///      actions will be asynchronous through events (Termination & output)
///  This should only be called if isConnected is true - doesn't make sense
///     otherwise.
///  ASYNCHRONOUS (relatively)
///
///  @param  command              wxString       What command to run.
///  @param  listener             wxEvtHandler * Who will receive term&output
///                                              events
///
///  @return wxTextOutputStream * StdIn for the newly started process
///
///  @author David Czechowski @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
wxTextOutputStream* PlinkConnect::executeCommand(wxString command, wxEvtHandler* listener)
{
	return executeCmd(command, listener, false);
}


//////////////////////////////////////////////////////////////////////////////
///  private executeCmd
///  Starts the command in the remote shell.  Putting proper wrappings around
///      it.  This puts stores the necessary info into the ProcessInfo list.
///  Not much is done differently for Synchronous Commands except recording
///      that it is or isn't.
///  This is a generic function.  The synchronous wait-loop would need to be
///       done else where (executeSyncCommand).
///  ASYNCHRONOUS (relatively)
///
///  @param  command              wxString       command
///  @param  listener             wxEvtHandler * who'll get the events
///  @param  isSynch              bool           Whether this is being run
///                                                synchronously or not.
///
///  @return wxTextOutputStream * StdIn for the newly started process
///
///  @author David Czechowski @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
//Private:
wxTextOutputStream* PlinkConnect::executeCmd(wxString command, wxEvtHandler* listener, bool isSynch)
{

	if(!getIsConnected()) { // <-- synchronous
		// Prevent a stupid-user error
		return NULL;
	}

	// Set the owner who will listen for Events
#ifdef PROCESSLIST
	ProcessInfo* p = (ProcessInfo*)m_processes.Last()->GetData();
#else
	ProcessInfo* p = m_procs[m_procs.GetCount() - 1];
#endif
#ifdef DO_LOG
	wxLogDebug("Executing command.  Process: %x, command: %s", p, command);
#endif
	p->owner = listener;
	p->isRunSynch = isSynch;

	// Wrap the command:
	wxString cmd = "echo St_Ar_Tt_oK_eN ; " + command + " ; echo En_Dt_oK_eN\r";
	//wxString cmd = "echo St_Ar_Tt_oK_eN ; " + command + " && echo s_Uc_Es_S ; echo En_Dt_oK_eN\r";

	// Send it:
	//wxLogDebug("PC(%d) cmd is: %s", p->pid, cmd);
	*(p->stdinStream) << cmd;
	p->state = PC_EXECUTING;

	spawnConnection(); // pre-spawn the next connection

	return 	p->stdinStream;

}


//////////////////////////////////////////////////////////////////////////////
///  public executeSyncCommand
///  This will execute "command" on the remote server and wait until it
///     terminates.  Then the output will be returned.  WARNING: if 'command'
///     is going to be expecting user interaction, this will lock chameleon.
///  SYNCHRONOUS
///
///  @param  command  wxString  what to execute on the remote server.
///
///  @return wxString The resulting output
///
///  @author David Czechowski @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
wxString PlinkConnect::executeSyncCommand(wxString command)
{
	executeCmd(command, NULL, true);

	// executeCommand spawns a new process, so my process is the next-to-last one
#ifdef PROCESSLIST
	ProcessInfo* p = (ProcessInfo*)m_processes.GetLast()->GetPrevious()->GetData();
#else
	ProcessInfo* p = m_procs[m_procs.GetCount() - 2];//
#endif

	int i = 1;
	m_synchronous = true;

	wxProgressDialog* progress = NULL;

	while(p->state == PC_BUSY || p->state == PC_EXECUTING) {
		// Perhaps this should terminate after an amount of time
		wxMilliSleep(250);

		if(progress != NULL)
		{
			progress->Pulse();
		}

		if(i % 4 == 0)
		{
#ifdef DO_LOG
			wxLogDebug("Synchronous network operation (executeSyncCommand): %d (command: %s)", i / 4, command);
#endif

			if(i > 12 && progress == NULL)
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

	terminateConnection(p);

	m_synchronous = false;
	return p->outputBuf;
}


//////////////////////////////////////////////////////////////////////////////
///  private parseOutput
///  This is where OutputEvents are created, and where state-transitions occur
///  I make liberal use of tokens to determine the success/failure of what I'm
///      doing.  I have a shell for each connection I make.  I know when a
///      command has ended by appending an echo command to the end, and waiting
///      until it shows up.
///  I know I have made a successful connection by watching for the
///      success-token
///
///  @param  proc   ProcessInfo * The process which has output
///  @param  output wxString      What the output is.
///  @param  errlog wxString      [=""] This is useless except for during
///                                        PS_STARTING
///
///  @return void
///
///  @author David Czechowski @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
//Private:
void PlinkConnect::parseOutput(ProcessInfo* p, wxString output, wxString errLog)
{
	//wxLogDebug("Plink(%d) stdout: \"%s\"", p->pid, output);
	//wxLogDebug("Plink(%d) stderr: \"%s\"", p->pid, errLog);

	if(p->state != PC_ENDING) {
		p->outputBuf += output;
	}

	if(p->state == PC_STARTING) {
		if(p->outputBuf.Contains("Successful Login")) {
			// Yeah!  It succeeded
			p->outputBuf = "";

			m_isConnected = true;

			// State transition:
			p->state = PC_READY;
		}
		else if(errLog.Contains("Store key in cache? (y/n)")) {
			// "Store key in cache? (y/n)"
			m_message += errLog;
			terminateConnection(p);
		}
		else if(errLog.Contains("Unable to authenticate"))
		{
			// Obviously, the password was bad.  If we're here, we _should_
			// have come through onTerminate(), so there's no need to kill
			// the process.  Instead, we just need to let the world know that
			// authentication failed.
			p->state = PC_ENDING;
			m_message += errLog;


		}
		else {
			//*(p->stdinStream) << "echo \"Successful Login\"" << endl; <-- backup plan
			// Do nothing.  Just continue waiting.

			// (in the meantime:)
			m_message += errLog;
		}

	}

	if(p->state == PC_READY) {
		// nothing to do
		//wxLogDebug("PlinkConnect(%d): extranious output: \"%s\"", p->pid, output.Mid(output.Find("Successful Login"));
	}

	if(p->state == PC_EXECUTING) {
		if(p->outputBuf.Contains("St_Ar_Tt_oK_eN\r")) {
			// Scrub the output (remove start-token and prior)
			int end = p->outputBuf.Find("St_Ar_Tt_oK_eN\r")+16;
			p->outputBuf.Remove(0,end);
			p->state = PC_BUSY;
		}
	}

	if(p->state == PC_BUSY) {
		if(p->outputBuf.Contains("En_Dt_oK_eN")) {
			// Scrub the output (remove end-token and beyond)
			int start = p->outputBuf.Find("En_Dt_oK_eN");
			p->outputBuf.Remove(start);
			p->state = PC_ENDING;
		}

		//Throw Appropriate Events:
		if(p->outputBuf != "") {
			if(p->owner != NULL) { // if someone is listening
				ChameleonProcessEvent e(chEVT_PROCESS_STDOUT);
				e.SetString(p->outputBuf);
				p->owner->AddPendingEvent(e);
				// Clear the buffer:
				p->outputBuf = "";
			}
		}
		if(p->state == PC_ENDING) { // ie. just finished
			if(p->owner != NULL) {
				ChameleonProcessEvent e(chEVT_PROCESS_ENDED);
				e.SetInt(0); // <-- this is incomplete
				p->owner->AddPendingEvent(e);
			}

			// proc not being used any more, so terminate it
			if(!p->isRunSynch) {
				terminateConnection(p);
			}
			//else -- it's terminated from within execSynchCommand
			//           to prevent early deletion
		}
	}

	if(p->state == PC_ENDING) {
		// nothing to do
		#ifdef _DEBUG
			wxString temp = output;
			int start = temp.Find("En_Dt_oK_eN");
			if(start != -1) {
				temp.Remove(start);
			}

			if(temp != "") {
				//wxLogDebug("PlinkConnect(%d): extranious output: \"%s\"", p->pid, temp);
			}
		#endif
	}

	return;
}


//////////////////////////////////////////////////////////////////////////////
///  private terminateConnection
///  This will send "exit" if no program is currently being run, or it will
///     just kill the plink.exe process (locally).
///  SYNCHRONOUS (?)
///
///  @param  proc ProcessInfo * which process to terminate
///
///  @return void
///
///  @author David Czechowski @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
//Private:
void PlinkConnect::terminateConnection(ProcessInfo* p)
{
	if(p->state == PC_BUSY || p->state == PC_EXECUTING) {
		// Kill the plink process
		p->state = PC_ENDING;
		wxKill(p->pid, wxSIGKILL); // SIGTERM (doesn't always work!?)
	}
	else {
		//send exit:
		p->state = PC_ENDING;
		*(p->stdinStream) <<  "exit" << endl;
	}

	// All the termination clean-up is done in onTerminate()
}


//////////////////////////////////////////////////////////////////////////////
///  private terminateAllConnections
///  Loops through the Processes, and call termianteConnection on each.
///  ASYNCHRONOUS
///
///  @return void
///
///  @author David Czechowski @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
//Private:
void PlinkConnect::terminateAllConnections()
{
	m_isConnected = false;

	// Walk thru list m_procPID's
#ifdef PROCESSLIST
	for(ProcessInfoList::Node *node = m_processes.GetFirst(); node; node = node->GetNext() ) {
		ProcessInfo *p = node->GetData();
#else
	for(int i = 0; i < m_procs.GetCount(); i++)
	{
		ProcessInfo* p = m_procs[i];
#endif
		terminateConnection(p);
	}
}


//////////////////////////////////////////////////////////////////////////////
///  public PollTick
///  wxWindows' Process stuff is not event driven.  So, I need to poll the
///     processes.  This method gets called to do the polling.  Which means
///     this needs to get called periodically
///
///  @return void
///
///  @author David Czechowski @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
void PlinkConnect::PollTick() {
	// Check on all of the processes:

#ifdef PROCESSLIST
	for(ProcessInfoList::Node *node = m_processes.GetFirst(); node; node = node->GetNext() ) {
		ProcessInfo *p = node->GetData();
#else
	for(int i = 0; i < m_procs.GetCount(); i++)
	{
		ProcessInfo* p = m_procs[i];
#endif
		wxProcess* proc = p->proc;
		wxString errout = "";
		wxString output = "";

		wxInputStream* pStdOut = proc->GetInputStream();
		while(proc->IsInputAvailable()) {
			output += pStdOut->GetC();
		}

		if(p->state == PC_STARTING) {
			// Also check STDERR if I'm in the starting phase
			wxInputStream* pStdErr = proc->GetErrorStream();
			while(proc->IsErrorAvailable()) {
				errout += pStdErr->GetC();
			}
		}

		if(output != "" || errout != "") {
			parseOutput(p, output, errout);
		}
	}

}


//////////////////////////////////////////////////////////////////////////////
///  private onTerminate
///  When a process terminates this gets called.  It will determine which
///     which process it came from, then (if appropriate) send an event to
///     it's listener.
///
///  @param  event wxProcessEvent & the termination event
///
///  @return void
///
///  @author David Czechowski @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
//Private:
void PlinkConnect::onTerminate(wxProcessEvent& event) {

	//PollTick(); // Catch any last Outputs

	// Determine which process:
	long pid = event.GetPid();
	ProcessInfo* p;
	bool found = false;

#ifdef PROCESSLIST
	for(ProcessInfoList::Node* node = m_processes.GetFirst(); node; node = node->GetNext() ) {
		p = node->GetData();
#else
	for(int i = 0; i < m_procs.GetCount(); i++)
	{
		p = m_procs[i];
#endif
		if(p->pid == pid) {
			found = true;
			break;
		}
	}

	if(found) {
		//Remove and Delete the process:
		//wxLogDebug("A Plink Connection Terminated - pid(%d)", pid);
		PollTick();
		delete p->proc;

		delete p->stdinStream;

#ifdef PROCESSLIST
		m_processes.DeleteObject(p);
#else
		m_procs.Remove(p);
#endif
		delete p;

#ifdef PROCESSLIST
		if(m_processes.GetCount() == 0) {
#else
		if(m_procs.GetCount() == 0)
		{
#endif
			m_isConnected = false;
			//wxLogDebug("All Plink Processes Terminated");
		}
	}

}


//////////////////////////////////////////////////////////////////////////////
///  public ForceKillProcess
///  This will terminate the process whose StdIn stream w points to.
///
///  @param  w    wxTextOutputStream * The StdIn stream of the process to term
///
///  @return void
///
///  @author David Czechowski @date 04-23-2004
//////// comment generated by Comment Maker from www.FeinSoftware.com /////////
void PlinkConnect::ForceKillProcess(wxTextOutputStream* w)
{
	ProcessInfo* p = FindProcess(w);
	// "terminateConnection(p); will check for any new output before actually killing the proc
	//    and will cause the event.  Setting NULL will prevent this.
	if(p != NULL) {
		p->owner = NULL;
		terminateConnection(p);
	}
}

ProcessInfo* PlinkConnect::FindProcess(wxTextOutputStream* w)
{
// disable the warning about truncating pointers
#pragma warning (disable: 4311)
	// Determine which process:
	ProcessInfo* p;
	bool found = false;

#ifdef PROCESSLIST
	for(ProcessInfoList::Node* node = m_processes.GetFirst(); node; node = node->GetNext() ) {
		p = node->GetData();
#else
	for(int i = 0; i < m_procs.GetCount(); i++)
	{
		p = m_procs[i];
#endif
		if( (int)&(*(p->stdinStream)) == (int)(&(*w)) ) { // yeah, this is a hack :(
			found = true;
			break;
		}
	}
#pragma warning (default: 4311)

	if(!found)
	{
		return NULL;
	}

	return p;
}

bool PlinkConnect::DoingSynchronousOperation()
{
	return m_synchronous;
}



#ifdef _PC_INTERNAL_TIMER_
//////////////////////////////////////////////////////////////////////////////
///  private OnTimerEvent
///  If I have an internal timer, this is what gets the timer events.
///
///  @param  event wxTimerEvent & Timer Event.
///
///  @return void
///
///  @author David Czechowski @date 04-23-2004
//////////////////////////////////////////////////////////////////////////////
//Private:
void PlinkConnect::OnTimerEvent(wxTimerEvent &event) {
	PollTick();
}
#endif
