//////////////////////////////////////
//
//    Based on Derry Bryson's taTelnet, Copyright 2000
//
/////////////////////////////////////
#include "wxssh.h"
#include "../../common/debug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


BEGIN_EVENT_TABLE(wxSSH, wxTerm)
	EVT_PROCESS_STDOUT(wxSSH::OnPlinkOut)
	EVT_PROCESS_STDERR(wxSSH::OnPlinkErr)
	EVT_PROCESS_ENDED(wxSSH::OnPlinkTerm)
END_EVENT_TABLE()

wxSSH::wxSSH(wxWindow* parent, wxWindowID id, Networking* network, Options* _option, const wxString& sPath, const wxPoint& pos, int width, int height, const wxString& name)
	: wxTerm(parent, id, _option, sPath, pos, width, height, name)
{
	m_connected = false;
	m_networking = network;
	m_plinkStdIn = NULL;
	m_inputBuffer = "";
	m_isInESCsequence = false;
	m_startingDebugConnection = false;
}


//////////////////////////////////////////////////////////////////////////////
///  public virtual destructor ~wxSSH
///  If still connected, does the equiv. of a disconnect() (geeh maybe it
///    should just call that).
///
///  @return void
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxSSH::~wxSSH()
{
	if(m_connected)
	{
		// "No time for pleasantries"
		m_networking->ForceKillProcess(m_plinkStdIn);
		wxMilliSleep(250);
		wxLogDebug("wxSSH force killed m_plink.");
	}
}


//////////////////////////////////////////////////////////////////////////////
///  public virtual SendBack
///  Pass the data onto the plink-connection.  Nothing special needs doing.
///    All screen output occurs when the characters comes back from the
///    server.
///
///  @param  len  int    number of characters
///  @param  data char * pointer to a character array
///
///  @return void
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxSSH::SendBack(int len, char *data)
{
	if(m_connected) {
		wxString s(data, len); // = "";
		m_plinkStdIn->WriteString(s);
	}
}


//////////////////////////////////////////////////////////////////////////////
///  public virtual Connect
///  Take advantage of Networking's StartRemoteCommand.  I start the
///     command 'bash' as the remote program to give us a shell.
///
///  @return void
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxSSH::Connect()
{
	m_startingDebugConnection = false;

	if(m_connected) {
		Disconnect(); // might this be a problem because it's Asynchronous?
	}

	Reset();
	set_mode_flag(CURSORINVISIBLE);
	Refresh();

	// Start the new Process
	m_plinkStdIn = m_networking->StartRemoteCommand("bash", this);
	if(m_plinkStdIn != NULL) {
		m_connected = true;
		m_inputBuffer = "";
		m_isInESCsequence = false;
	}
}


//////////////////////////////////////////////////////////////////////////////
///  public ConnectForDebug
///  Similar to connect.  The command sent will put the shell into a sleep
///    after requesting the 'who am i' data.  This will go into a wait loop
///    until either the data comes back, or a fail token.
///
///  @return wxString Parsed 'who am i' data
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
wxString wxSSH::ConnectForDebug()
{
	m_startingDebugConnection = true;

	if(m_connected) {
		wxLogDebug("Bad, Bad.  Tried starting as a debug terminal while already connected.");
		Disconnect();
	}

	Reset();
	set_mode_flag(CURSORINVISIBLE);
	Refresh();

	// Start the new Process
	m_plinkStdIn = m_networking->StartRemoteCommand("tty && sleep 24h || echo \"CHdEBUGGER-CONNECT\"", this);
	if(m_plinkStdIn != NULL) {
		m_connected = true;
		m_inputBuffer = "";
		m_isInESCsequence = false;
	}

	// Synchronous:
	//wxRegEx reParseTTY(".+?\\s+(.+?)\\s+\\w{3}\\s+\\d+\\s+\\d+:\\d+\\s", wxRE_ADVANCED);

	wxRegEx reParseTTY("/([[:alnum:]]|/|_)+", wxRE_ADVANCED);
	while(!reParseTTY.Matches(m_inputBuffer) &&
		!m_inputBuffer.Contains("CHdEBUGGER-CONNECT")) {
		wxSafeYield(); // yes, mark probably won't like this, but darn it's simple, and works
	}

	wxString tty = reParseTTY.GetMatch(m_inputBuffer, 0); // empty string is return if it fails

	m_inputBuffer = "";
	m_startingDebugConnection = false;

	return tty;
}


//////////////////////////////////////////////////////////////////////////////
///  public virtual Disconnect
///  This will terminate the plink connection.
///
///  @param  clearDisplay bool  [=true] whether to clear the window or not
///
///  @return void
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxSSH::Disconnect(bool clearDisplay)
{
	if(m_connected)
	{
		m_networking->ForceKillProcess(m_plinkStdIn);

		// Not sure if this is needed or not... if we've got problems on exit, we'll come back here
		//wxUsleep(250);
		m_connected = false;

		if(clearDisplay) {
			GenericTerminal::Reset();
			set_mode_flag(CURSORINVISIBLE);
			GenericTerminal::Update();
			Refresh();
		}
		else {
			set_mode_flag(CURSORINVISIBLE);
		}
	}
}


//////////////////////////////////////////////////////////////////////////////
///  public bool IsConnected
///  Is connected returns true when Networking is able to start remote commands
///
///  @return bool
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
bool wxSSH::IsConnected(void)
{
	return m_connected;
}


//////////////////////////////////////////////////////////////////////////////
///  public OnPlinkOut
///  wxSSH should catch XTerm specific escape sequences (coming from the server)
///  The following sequences are useful in this respect:
///     * ESC]0;stringBEL -- Set icon name and window title to string
///     * ESC]1;stringBEL -- Set icon name to string
///     * ESC]2;stringBEL -- Set window title to string
///  (where ESC is escape character (char)27, and BEL is bell character char(7).)
///
///  Currently I only catch the first one, and throw it to the bit-bucket.  This
///     should probably be expanded to catch all 3.  Perhaps it should be
///     integrated more: GTerm has a name variable (I think).
///  I assume these sequences begin and end on the same line.
///
///  @param  event ChameleonProcessEvent & Output Event from Networking
///
///  @return void
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxSSH::OnPlinkOut(ChameleonProcessEvent &e)
{
	wxString s = e.GetString();
	int len = s.Length();

	wxString startEscSeq0 = wxString((char)27) + "]0;";
	//wxString startEscSeq1 = wxEmptyString + (char)27 + "1;";
	//wxString startEscSeq2 = wxEmptyString + (char)27 + "2;";

	int start = s.Find(startEscSeq0);
	if(start != -1) {
		int end = s.Find((char)7);
		if(end > start) {
			s.Remove(start, end-start);
			len -= (end-start);
		}
	}

	if(!m_isInESCsequence && !m_startingDebugConnection) { // && !s.Contains((char)27)) {
		//ProcessInput(len, s.c_str());
		ProcessInput(len, s.ToStdString());
	}
	else {
		m_inputBuffer += s;
	}

	return;
}


//////////////////////////////////////////////////////////////////////////////
///  public OnPlinkErr
///  Currently Networking won't send any StdErr output because it all comes in
////   from stdout (stderr is piped to stdout).
///
///  @param  event ChameleonProcessEvent & ErrEvent.
///
///  @return void
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxSSH::OnPlinkErr(ChameleonProcessEvent &e)
{
	wxLogDebug("Terminal-StdError: "+e.GetString());
}


//////////////////////////////////////////////////////////////////////////////
///  public OnPlinkTerm
///  This would be a surprise.  If this happens something likely happened to
///    the network connection, OR the user typed "exit".  Either way, I'm no
///    longer connected.  An explicit call to Connect would be needed to
///    start another shell again.
///
///  @param  event ChameleonProcessEvent & chProcessTermEvent
///
///  @return void
///
///  @author David Czechowski @date 04-22-2004
//////////////////////////////////////////////////////////////////////////////
void wxSSH::OnPlinkTerm(ChameleonProcessEvent &e)
{
	wxLogDebug("Terminal's External Process Terminated!");

	m_connected = false;
}

typedef struct
{
	int width;
	int height;
} ResizeData;

void wxSSH::UpdateRemoteSize(int width, int height)
{
	ProcessInfo* p = m_networking->GetProcessInfo(m_plinkStdIn);

	if(p == NULL)
	{
		return;
	}
	wxString eventName = wxString::Format("PRE%d", p->pid);
	wxString sharedName = wxString::Format("PSM%d", p->pid);
	int size = sizeof(ResizeData);

	HANDLE hSharedData;

	const wchar_t* buffer = sharedName.fn_str();
	//sprintf(buffer, "%s", sharedName.c_str());

	hSharedData = CreateFileMapping (INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, size, buffer);//(LPCTSTR )eventName.c_str());

	if (hSharedData == NULL)
	{
		/*
		//int err = GetLastError();

		LPVOID lpMsgBuf;
		LPVOID lpDisplayBuf;
		LPCTSTR lpszFunction = "Null handle: ";

		DWORD dw = GetLastError();

		FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

		lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
		(lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR));
		wsprintf((LPTSTR)lpDisplayBuf,
		TEXT("%s failed with error %d: %s"),
		lpszFunction, dw, lpMsgBuf);
		MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

		LocalFree(lpMsgBuf);
		LocalFree(lpDisplayBuf);
		*/

		return;
	}

	ResizeData* pSharedData = (ResizeData *) MapViewOfFile( hSharedData,
		FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, 0);

	//CSharedStruct<ResizeData> rd((char *)eventName.c_str());
	pSharedData->width = GetTermWidth();
	pSharedData->height = GetTermHeight();

	HANDLE hPlinkEvent = CreateEvent(NULL, FALSE, FALSE, eventName);

	SetEvent(hPlinkEvent);

	CloseHandle(hSharedData);

}
