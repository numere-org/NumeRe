#include "LocalProcessManager.h"
#include "PipedProcess.h"
#include "../common/chameleonprocessevent.h"
#include <wx/filefn.h>


BEGIN_EVENT_TABLE(LocalProcessManager, wxEvtHandler)
	EVT_PROCESS_STDOUT(LocalProcessManager::OnProcessOutput)
	EVT_PROCESS_STDERR(LocalProcessManager::OnProcessOutput)
	EVT_PROCESS_ENDED(LocalProcessManager::OnProcessTerminated)
END_EVENT_TABLE()

LocalProcessManager::LocalProcessManager()
{
	m_nextProcessID = 0;
}

wxTextOutputStream* LocalProcessManager::StartLocalCommand( wxString command, wxEvtHandler* parent )
{
	PipedProcess* pProcess;

	pProcess = new PipedProcess((void**)&pProcess, this, m_nextProcessID, true, wxGetCwd());
	m_nextProcessID++;

	wxString finalCommand;


	//long pid = wxExecute(command, wxEXEC_ASYNC, pProcess);
	long pid = pProcess->Launch(command);

	ProcessInfo* p = new ProcessInfo;
	p->pid = pid;
	p->proc = pProcess;
	p->state = PC_STARTING;
	p->stdinStream = new wxTextOutputStream(*pProcess->GetOutputStream(), wxEOL_NATIVE);
	p->outputBuf = "";
	p->owner = parent; // set when process gets used
	
	m_processes.Add(p);

	return p->stdinStream;
}

void LocalProcessManager::OnProcessTerminated( ChameleonProcessEvent& event)//wxProcessEvent& event )
{
	ProcessInfo* p = FindProcess(event.GetPid());

	if(p != NULL)
	{
		m_processes.Remove(p);

		ChameleonProcessEvent event(chEVT_PROCESS_ENDED);
		event.SetInt(0);

		p->owner->AddPendingEvent(event);

		delete p->stdinStream;
		delete p;
	}
	
}

void LocalProcessManager::OnProcessOutput( ChameleonProcessEvent& event )
{
	int pid = event.GetInt();

	ProcessInfo* p = FindProcess(pid);

	if(p != NULL)
	{
		p->owner->AddPendingEvent(event);
	}
}

ProcessInfo* LocalProcessManager::FindProcess(int pid)
{
	int index = -1;

	for(int i = 0; i < m_processes.GetCount(); i++)
	{
		if(m_processes[i]->pid == pid)
		{
			index = i;
			break;
		}
	}

	if(index >= 0)
	{
		return m_processes[index];
	}
	else
	{
		return NULL;
	}
}
