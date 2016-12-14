#ifndef __COMPILER__EVENT__
#define __COMPILER__EVENT__

#include <wx/event.h>
#include <wx/filename.h>
#include "../common/datastructures.h"
#include "../common/debug.h"

DECLARE_EVENT_TYPE(chEVT_COMPILER_START, wxID_ANY)
DECLARE_EVENT_TYPE(chEVT_COMPILER_PROBLEM, wxID_ANY)
DECLARE_EVENT_TYPE(chEVT_COMPILER_END, wxID_ANY)

class CompilerEvent : public wxEvent
{
	public:
		CompilerEvent(wxEventType eventtype);

		wxFileName GetFile() { return m_file; }
		int GetInt() { return m_num; }
		CompileResult GetResult() { return m_tri; }
		wxString GetMessage() { return m_message; }
		wxString GetGCCOutput() { return m_output; }
		wxString GetCommandLine() { return m_commandLine; }
		bool IsRemoteFile() { return m_isRemoteFile; }

		void SetFile(wxFileName file) { m_file = file; }
		void SetInt(int i) { m_num = i; }
		void SetResult(CompileResult t) { m_tri = t; }
		void SetMessage(wxString s) { m_message = s; }
		void SetGCCOutput(wxString s) { m_output = s; }
		void SetCommandLine(wxString s) { m_commandLine = s; }
		void SetRemoteFile(bool remoteFile) { m_isRemoteFile = remoteFile; }

		virtual wxEvent *Clone() const { return new CompilerEvent(*this); }


	public:
		wxFileName m_file;
		CompileResult m_tri;
		int m_num;
		wxString m_message;
		wxString m_output;
		wxString m_commandLine;
		bool m_isRemoteFile;

};

typedef void (wxEvtHandler::*chCompilerEventFunction)(CompilerEvent&);

#define EVT_COMPILER_START(fn) \
	DECLARE_EVENT_TABLE_ENTRY( \
			chEVT_COMPILER_START, wxID_ANY, wxID_ANY, \
			(wxObjectEventFunction)(wxEventFunction)(chCompilerEventFunction)&fn, \
			(wxObject *) NULL),


#define EVT_COMPILER_PROBLEM(fn) \
	DECLARE_EVENT_TABLE_ENTRY( \
			chEVT_COMPILER_PROBLEM, wxID_ANY, wxID_ANY, \
			(wxObjectEventFunction)(wxEventFunction)(chCompilerEventFunction)&fn, \
			(wxObject *) NULL),


#define EVT_COMPILER_END(fn) \
	DECLARE_EVENT_TABLE_ENTRY( \
			chEVT_COMPILER_END, wxID_ANY, wxID_ANY, \
			(wxObjectEventFunction)(wxEventFunction)(chCompilerEventFunction)&fn, \
			(wxObject *) NULL),


#endif  // __COMPILER__EVENT__

