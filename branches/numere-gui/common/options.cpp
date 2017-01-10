#include "Options.h"

// Includes:
#include <wx/filename.h>
#include <wx/arrstr.h>
#include <wx/dir.h>
#include "../perms/p.h"
#include "debug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

Options::Options()
{
	// Set Some Default Values (perhaps these should not be set!)
	m_pscpProg = "pscp.exe";
	m_plinkProg = "plink.exe";
	//m_mingwPath = "C:\\Program Files\\MinGW\\bin\\";
	m_username = "username";
	m_hostname = "localhost";
	m_password = "";
	m_remoteCompileOut = "a.out";
	m_terminalSize = 300;

	// Default to printing with black text, white background
	m_printStyle = wxSTC_PRINT_BLACKONWHITE;

	m_perms = new Permission();

	m_showToolbarText = true;
	m_printLineNumbers = false;
	m_combineWatchWindow = false;

	m_mingwProgramNames.Add("g++.exe");
	m_mingwProgramNames.Add("cc1plus.exe");
	m_mingwProgramNames.Add("gdb.exe");
}

Options::~Options()
{
	delete m_perms;
}


bool Options::SetPscpApp(wxString path_and_prog) {
	if(wxFileName::FileExists(path_and_prog)) {
		m_pscpProg = path_and_prog;
		// Could even check to make sure it's the right version (I'm dreaming)
		return true;
	}
	//else
		wxLogDebug("\"" + path_and_prog + "\" is an invalid PSCP.");
		return false;
}


bool Options::SetPlinkApp(wxString path_and_prog) {
	if(wxFileName::FileExists(path_and_prog)) {
		m_plinkProg = path_and_prog;
		return true;
	}
	//else
		wxLogDebug("\"" + path_and_prog + "\" is an invalid Plink.");
		return false;
}



bool Options::SetMingwBasePath(wxString path)
{
	if(wxFileName::DirExists(path))
	{
		m_mingwBasePath = path;
		return true;
	}
	else
	{
		wxLogDebug("\"" + path + "\" is an invalid path.");
		return false;
	}
}


bool Options::SetUsername(wxString user) {
	m_username = user;
	return true;
}


bool Options::SetHostname(wxString host) {
	m_hostname = host;
	return true;
}


bool Options::SetPassphrase(wxString pass) {
	if(pass.Contains('\"')) {
		return false;
	}
	//else
		m_password = pass;
		return true;
}


bool Options::SetRemoteCompileOut(wxString path_and_file) {
	m_remoteCompileOut = path_and_file;
	return true;
}

bool Options::SetLocalCompileOut(wxString path_and_file) {
	m_localCompileOut = path_and_file;
	return true;
}

void Options::SetPrintStyle(int style)
{
	m_printStyle = style;
}

void Options::SetShowToolbarText(bool useText)
{
	m_showToolbarText = useText;
}

void Options::SetShowCompileCommands(bool showCommands)
{
	m_showCompileCommands = showCommands;
}

void Options::SetTerminalHistorySize(int size)
{
	m_terminalSize = size;
}

void Options::SetLineNumberPrinting(bool printLineNumbers)
{
	m_printLineNumbers = printLineNumbers;
}

void Options::SetMingwBinPaths(wxArrayString paths)
{
	m_mingwBinPaths = paths;
}

void Options::SetMingwExecutables(StringFilenameHash files)
{
	m_mingwExecutableNames = files;
}

wxString Options::VerifyMingwPath(wxString mingwPath)
{
	wxArrayString mingwFiles = GetMingwProgramNames();
	wxArrayString mingwBinPaths;
	StringFilenameHash mingwExecutables;

	wxString errorMessage = wxEmptyString;
	wxString messageBoxCaption = wxEmptyString;
	//int messageBoxOptions = wxOK;

	if(!wxFileName::DirExists(mingwPath))
	{
		errorMessage = "The directory does not exist.";
		goto Finished;
	}



	for(size_t i = 0; i < mingwFiles.GetCount(); i++)
	{
		wxString programName = mingwFiles[i];

		wxString filePath = wxDir::FindFirst(mingwPath, programName);

		wxString binPath = wxFileName(filePath).GetPath();

		if(filePath == wxEmptyString)
		{
			errorMessage.Printf("Unable to find file: %s", programName);
			goto Finished;
		}
		else
		{
			wxFileName executablePath(filePath);
			mingwExecutables[programName] = executablePath;

			if(mingwBinPaths.Index(binPath) == wxNOT_FOUND)
			{
				mingwBinPaths.Add(binPath);
			}
		}
	}


Finished:


	if(errorMessage == wxEmptyString)
	{
		SetMingwBinPaths(mingwBinPaths);
		SetMingwExecutables(mingwExecutables);

		SetMingwBasePath(mingwPath);
	}

	return errorMessage;
}
