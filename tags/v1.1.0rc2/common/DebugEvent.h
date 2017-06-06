#ifndef __DEBUGEVENT__H__
#define __DEBUGEVENT__H__

#include <wx/event.h>
#include <wx/dynarray.h>
#include "datastructures.h"
#include "ProjectInfo.h"

DECLARE_EVENT_TYPE(wxEVT_DEBUG, wxID_ANY)


class wxDebugEvent : public wxEvent
{
	public:
		wxDebugEvent();
		virtual wxEvent *Clone() const { return new wxDebugEvent(*this); }

		void SetLineNumber(int line);
		void SetVariableNames(wxArrayString names);
		void SetVariableValues(wxArrayString values);
		void SetVariableTypes(wxArrayString types);
		//void SetSourceFilenames(wxArrayString filenames);
		void SetSourceFilename(wxString filename);
		void SetFunctionName(wxString funcname);
		void SetClassName(wxString classname);
		void SetErrorMessage(wxString message);
		//void SetRemoteMode(bool remote);
		void SetFileBreakpoints(FileBreakpointHash filebreakpoints);
		void SetProject(ProjectInfo* project);
		void SetTTYString(wxString tty);
		void SingleItemRequested(bool val) { m_singleItemRequested = val; }
		
		

		int GetLineNumber() { return m_lineNumber;}
		wxArrayString GetVariableNames() { return m_variableNames; }
		wxArrayString GetVariableValues() { return m_variableValues; }
		wxArrayString GetVariableTypes() { return m_variableTypes; }
		//wxArrayString GetSourceFilenames() { return m_sourceFilenames; }
		wxString GetSourceFilename() { return m_filename; }
		wxString GetFunctionName() { return m_functionName; }
		wxString GetClassName() { return m_className; }
		wxString GetErrorMessage() {return m_errorMessage; }
		bool IsRemote() { return m_project->IsRemote(); }
		FileBreakpointHash GetFileBreakpoints() { return m_filebreakpoints; }
		ProjectInfo* GetProject() { return m_project; }
		wxString GetTTYString() { return m_ttyString; }
		bool GetSingleItemRequested() const { return m_singleItemRequested; }

private:
		int m_lineNumber;
		wxArrayString m_variableNames;
		wxArrayString m_variableValues;
		wxArrayString m_variableTypes;
		//wxArrayString m_sourceFilenames;
		wxString m_filename;
		wxString m_errorMessage;
		wxString m_functionName;
		wxString m_className;
		wxString m_ttyString;
		//wxString m_executableFilename;
		bool m_remoteMode;
		FileBreakpointHash m_filebreakpoints;
		ProjectInfo* m_project;

		bool m_singleItemRequested;
		

};

typedef void (wxEvtHandler::*wxDebugEventFunction)(wxDebugEvent&);

#define EVT_DEBUG(fn) \
	DECLARE_EVENT_TABLE_ENTRY( \
			wxEVT_DEBUG, wxID_ANY, wxID_ANY, \
			(wxObjectEventFunction)(wxEventFunction)(wxDebugEventFunction)&fn, \
			(wxObject *) NULL),


#endif // __WXPROCESS2_EVENTS__H__

