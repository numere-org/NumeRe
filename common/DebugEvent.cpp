#include "DebugEvent.h"

#include "debug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

DEFINE_EVENT_TYPE(wxEVT_DEBUG)
wxDebugEvent::wxDebugEvent()
{
	wxEvent::m_eventType = wxEVT_DEBUG;
}

void wxDebugEvent::SetLineNumber(int line)
{
	m_lineNumber = line;
}

void wxDebugEvent::SetVariableNames(wxArrayString names)
{
	m_variableNames = names;
}

void wxDebugEvent::SetVariableValues(wxArrayString values)
{
	m_variableValues = values;
}

void wxDebugEvent::SetVariableTypes(wxArrayString types)
{
	m_variableTypes = types;
}

void wxDebugEvent::SetErrorMessage(wxString message)
{
	m_errorMessage = message;
}

/*
void wxDebugEvent::SetExecutableFilename(wxString filename)
{
	m_executableFilename = filename;
}


void wxDebugEvent::SetRemoteMode(bool remote)
{
	m_remoteMode = remote;
}


void wxDebugEvent::SetSourceFilenames(wxArrayString filenames)
{
	m_sourceFilenames = filenames;
}
*/

void wxDebugEvent::SetFileBreakpoints(FileBreakpointHash filebreakpoints)
{
	m_filebreakpoints = filebreakpoints;
}

void wxDebugEvent::SetSourceFilename(wxString filename)
{
	m_filename = filename;
}

void wxDebugEvent::SetProject(ProjectInfo* project)
{
	m_project = project;
}

void wxDebugEvent::SetFunctionName(wxString funcname)
{
	m_functionName = funcname;
}

void wxDebugEvent::SetClassName(wxString classname)
{
	m_className = classname;
}

void wxDebugEvent::SetTTYString(wxString tty)
{
	m_ttyString = tty;
}