#include "DebugManager.h"
//#include "cham_db.h"
#include "../editor/editor.h"
#include "../gui/wxssh.h"
#include "../gui/ChameleonProjectManager.h"
#include "../gui/NumeReWindow.h"
#include "../gui/dialogs/VariableWatchPanel.h"

BEGIN_EVENT_TABLE(DebugManager, wxEvtHandler)
	EVT_DEBUG						(DebugManager::OnDebugEvent)
END_EVENT_TABLE()

DebugManager::DebugManager( Debugger* debugger, wxSSH* debugTerminal, ChameleonProjectManager* projectManager,
						   NumeReWindow* mainFrame, VariableWatchPanel* watchPanel)
{
	m_debugger = debugger;
	m_debugTerminal = debugTerminal;
	m_projectManager = projectManager;
	m_mainFrame = mainFrame;
	m_watchPanel = watchPanel;
}

void DebugManager::OnDebugCommand( int eventID, ProjectInfo* project, NumeReEditor* currentEditor)
{
	wxDebugEvent debugEvent;

	debugEvent.SetId(eventID);

	if(eventID == ID_DEBUG_START)
	{
		ProjectInfo* debugProject = project;
		FileBreakpointHash bphash;

		if(project->IsSingleFile())
		{
			wxFileName sourceFile = currentEditor->GetFileName();
			wxString sourceFileName = sourceFile.GetFullPath(wxPATH_UNIX);
			wxArrayInt linenumbers = currentEditor->GetBreakpoints();
			bphash[sourceFileName] = linenumbers;

		}
		else
		{
			EditorPointerArray edList = project->GetEditors();
			int numOpenProjEds = edList.GetCount();
			for(int i = 0; i < numOpenProjEds; i++)
			{
				NumeReEditor* ed = edList[i];
				wxArrayInt breaks = ed->GetBreakpoints();
				if(breaks.GetCount() > 0)
				{
					wxFileName fn = ed->GetFileName();
					wxString filename = fn.GetFullPath(project->IsRemote() ? wxPATH_UNIX : wxPATH_DOS);

					bphash[filename] = breaks;
				}
			}
		}

		debugEvent.SetProject(debugProject);

		// set up the debug I/O panel connection
		if(debugProject->IsRemote())
		{
			wxString tty = m_debugTerminal->ConnectForDebug();

			if(!tty.StartsWith("/dev"))
			{
				wxFileName ttyfile("/dev/");
				ttyfile.AppendDir(tty);
				tty = ttyfile.GetFullPath(wxPATH_UNIX);
			}
			debugEvent.SetTTYString(tty);
		}

		debugEvent.SetFileBreakpoints(bphash);
		debugEvent.SetId(ID_DEBUG_START);
	}

	m_debugger->AddPendingEvent(debugEvent);
}

void DebugManager::OnDebugEvent( wxDebugEvent &event )
{
	int eventID = event.GetId();

	switch(eventID)
	{
		case ID_DEBUG_EXIT_ERROR:
		case ID_DEBUG_EXIT_NORMAL:
		{
			m_projectManager->ClearDebugFocus();

			wxLogDebug("Debugger exit");
			m_debugTerminal->Disconnect(false);

			m_watchPanel->DebuggerExited();
			break;
		}
		case ID_DEBUG_BREAKPOINT:
		{
			wxString file = event.GetSourceFilename();
			int linenumber = event.GetLineNumber();

			m_mainFrame->FocusOnLine(file, linenumber);

			if(event.GetVariableNames().GetCount() > 0)
			{
				m_watchPanel->UpdateVariableInfo(event);
			}
			break;
		}
		case ID_DEBUG_ADD_BREAKPOINT:
		case ID_DEBUG_REMOVE_BREAKPOINT:
			{
				if(m_debugger->isDebugging() && m_debugger->isPaused())
				{
					m_debugger->AddPendingEvent(event);
				}
				break;
			}
		case ID_DEBUG_ADD_WATCH:
		case ID_DEBUG_REMOVE_WATCH:
			{
				if(eventID == ID_DEBUG_ADD_WATCH)
				{
					m_watchPanel->AddWatchedVariables(event);
				}

				m_debugger->AddPendingEvent(event);
				break;
			}
		case ID_DEBUG_VARINFO:
			m_watchPanel->UpdateVariableInfo(event);
			break;
		case ID_DEBUG_RUNTOCURSOR:
			m_debugger->AddPendingEvent(event);
			break;
		default:
			wxLogDebug("Default DebugEvent.  Value: %d", eventID);
			break;
	}
}

