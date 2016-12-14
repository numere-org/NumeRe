#ifndef __CHAMELEON__NETWORKING__H__
#define __CHAMELEON__NETWORKING__H__
///////////////////////////////////////
//
// Notes: I strongly recommend anything that uses an asynch process, that it call ForceKill
//           in it's destructor if it's process hasn't terminated.
//
// Caution: sendFileContents will overwrite anything in it's way.
//
///////////////////////////////////////
#include <wx/wx.h>
#include <wx/regex.h>
//#include <wx/string.h>
#include <wx/filename.h>
#include "../common/datastructures.h"
#include "../common/debug.h"
class wxProcess;
class wxProcessEvent;
class PlinkConnect;
class Options;
class wxTextOutputStream;
class PipedProcess;
class LocalProcessManager;

#define POLL_RATE 10 //milliseconds


class Networking : public wxEvtHandler {
	public:
		// ..strcutors:
		Networking(Options* options);
		~Networking();

		// Methods:
		NetworkStatus GetStatus();
		bool DoingSynchronousOperation();
		wxString GetStatusDetails();
		bool SSHCacheFingerprint();
		void PingOptions();



		// Remote Specific:
		bool GetHomeDirPath(wxString &homeDir);
		bool GetFileContents(wxFileName file, wxString &contents);
		bool SendFileContents(wxString strng, wxFileName file);
		bool GetDirListing(wxString dirPath, DirListing &list, bool forceRefresh = false,
							bool includeHidden = false);

		// Processes:
		wxTextOutputStream* StartRemoteCommand(wxString cmd, wxEvtHandler* owner);
		wxTextOutputStream* StartLocalCommand(wxString cmd, wxEvtHandler* owner);
		wxString ExecuteRemoteCommand(wxString cmd);
		wxString ExecuteLocalCommand(wxString cmd);
		wxString ExecuteCommand(bool isRemote, wxString cmd);
		wxTextOutputStream* StartCommand(bool isRemote, wxString cmd, wxEvtHandler* owner);
		void ForceKillProcess(wxTextOutputStream* w); // <-- ICKY ICKY ICKY

		ProcessInfo* GetProcessInfo(wxTextOutputStream* w);

	private:
		// Events:
		void onTerm(wxProcessEvent &e);
		void onTimerTick(wxTimerEvent &e);

		// Methods:
		bool SSHGetHomeDirPath(wxString &homeDir);
		bool SSHGetFileContents(wxString file, wxString &contents);
		bool SCPDoTransfer(wxString from_path_name, wxString to_path_name);
		bool SSHGetDirListing(wxString dirPath, DirListing &listing,
								bool includeHidden = false);

		// Helpers:
		bool MaintainSettings();
		wxArrayString ParseFindOutput(wxString strng, bool includeHidden);
		bool SSHExecSyncCommand(wxString command, wxString &output);

		// Data:
		PlinkConnect* m_plinks;
		LocalProcessManager* m_processManager;

		Options* m_options;

		wxString m_currHost, m_currUser, m_currPass;
		NetworkStatus m_status;
		wxString m_statusDetails;
		wxString m_userHomeDir; // mini-cache
		wxTimer m_timer;




		DECLARE_EVENT_TABLE()
};


#endif // __CHAMELEON__NETWORKING__H__
