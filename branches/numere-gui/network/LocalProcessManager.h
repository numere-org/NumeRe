#ifndef LOCAL_PROCESS_MANAGER_H
#define LOCAL_PROCESS_MANAGER_H


#include <wx/wx.h>
#include <wx/dynarray.h>
#include "../common/datastructures.h"
#include "../common/chameleonprocessevent.h"


class wxProcessEvent;
class wxTextOutputStream;
class PipedProcess;

//WX_DEFINE_ARRAY_PTR(PipedProcess*, PipedProcessArray);
//WX_DEFINE_ARRAY_PTR(ProcessInfo*, ProcessInfoArray);

class LocalProcessManager : public wxEvtHandler
{
public:
	LocalProcessManager();

	wxTextOutputStream* StartLocalCommand(wxString command, wxEvtHandler* parent);

private:
	void OnProcessOutput(ChameleonProcessEvent& event);
	//void OnProcessStandardOut(ChameleonProcessEvent& event);
	//void OnProcessStandardErr(ChameleonProcessEvent& event);
	void OnProcessTerminated(ChameleonProcessEvent& event);//wxProcessEvent& event);

	ProcessInfo* FindProcess(int pid);

	ProcessInfoArray m_processes;

	int m_nextProcessID;


	DECLARE_EVENT_TABLE()
};


#endif