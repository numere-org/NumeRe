#include "Options.h"

// Includes:
#include <wx/filename.h>
#include <wx/arrstr.h>
#include <wx/dir.h>
#include "debug.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

int StrToInt(const string&);
string toString(int);

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
	m_LaTeXRoot = "C:/Program Files";

	// Default to printing with black text, white background
	m_printStyle = wxSTC_PRINT_BLACKONWHITE;


	m_showToolbarText = true;
	m_printLineNumbers = false;
	m_combineWatchWindow = false;
	m_saveSession = false;
	m_formatBeforeSaving = false;

	m_mingwProgramNames.Add("g++.exe");
	m_mingwProgramNames.Add("cc1plus.exe");
	m_mingwProgramNames.Add("gdb.exe");

	setDefaultSyntaxStyles();
}

Options::~Options()
{
}

void Options::setDefaultSyntaxStyles()
{
    for (int i = Styles::STANDARD; i < Styles::STYLE_END; i++)
    {
        SyntaxStyles _style = GetDefaultSyntaxStyle(i);

        vSyntaxStyles.push_back(_style);
    }
}

wxString Options::convertToString(const SyntaxStyles& _style)
{
    wxString sReturn;
    sReturn = this->toString(_style.foreground);
    sReturn += this->toString(_style.background);
    sReturn += _style.bold ? "1" : "0";
    sReturn += _style.italics ? "1" : "0";
    sReturn += _style.underline ? "1" : "0";
    sReturn += _style.defaultbackground ? "1" : "0";
    return sReturn;
}

SyntaxStyles Options::convertFromString(const wxString& styleString)
{
    SyntaxStyles _style;
    if (styleString.length() < 21)
        return _style;
    _style.foreground = StrToColor(styleString.substr(0,9));
    _style.background = StrToColor(styleString.substr(9,9));
    if (styleString[18] == '1')
        _style.bold = true;
    if (styleString[19] == '1')
        _style.italics = true;
    if (styleString[20] == '1')
        _style.underline = true;
    if (styleString.length() > 21)
    {
        if (styleString[21] == '0')
            _style.defaultbackground = false;
    }
    else
        _style.defaultbackground = false;
    return _style;
}

wxString Options::toString(const wxColour& color)
{
    wxString colstring = "";
    colstring += ::toString((int)color.Red());
    if (colstring.length() < 3)
        colstring.insert(0,3-colstring.length(), '0');
    colstring += ::toString((int)color.Green());
    if (colstring.length() < 6)
        colstring.insert(3,6-colstring.length(), '0');
    colstring += ::toString((int)color.Blue());
    if (colstring.length() < 9)
        colstring.insert(6,9-colstring.length(), '0');
    return colstring;
}

wxColour Options::StrToColor(wxString colorstring)
{
    unsigned char channel_r = StrToInt(colorstring.substr(0,3).ToStdString());
    unsigned char channel_g = StrToInt(colorstring.substr(3,3).ToStdString());
    unsigned char channel_b = StrToInt(colorstring.substr(6,3).ToStdString());
    return wxColour(channel_r, channel_g, channel_b);
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

SyntaxStyles Options::GetDefaultSyntaxStyle(size_t i)
{
    SyntaxStyles _style;
    switch (i)
    {
        case Styles::STANDARD:
            _style.italics = true;
            _style.defaultbackground = false;
            break;
        case Styles::CONSOLE_STD:
            _style.foreground = wxColour(0,0,100);
            _style.defaultbackground = false;
            break;
        case Styles::COMMAND:
            _style.foreground = wxColour(0,128,255);
            _style.bold = true;
            _style.underline = true;
            break;
        case Styles::COMMENT:
            _style.foreground = wxColour(0,128,0);
            _style.background = wxColour(255,255,183);
            _style.defaultbackground = false;
            _style.bold = true;
            break;
        case Styles::OPTION:
            _style.foreground = wxColour(0,128,100);
            break;
        case Styles::FUNCTION:
            _style.foreground = wxColour(0,0,255);
            _style.bold = true;
            break;
        case Styles::CUSTOM_FUNCTION:
            _style.foreground = wxColour(0,0,160);
            break;
        case Styles::CONSTANT:
            _style.foreground = wxColour(255,0,128);
            _style.bold = true;
            break;
        case Styles::SPECIALVAL: // ans cache ...
            _style.bold = true;
            break;
        case Styles::STRING:
            _style.foreground = wxColour(128,128,255);
            break;
        case Styles::STRINGPARSER:
            _style.foreground = wxColour(0,128,192);
            _style.bold = true;
            break;
        case Styles::OPERATOR:
            _style.foreground = wxColour(255,0,0);
            break;
        case Styles::INCLUDES:
        case Styles::PROCEDURE:
        case Styles::PROCEDURE_COMMAND:
            _style.foreground = wxColour(128,0,0);
            _style.bold = true;
            break;
        case Styles::NUMBER:
            _style.foreground = wxColour(255,128,64);
            break;
        case Styles::METHODS:
            _style.foreground = wxColour(0,180,50);
            _style.bold = true;
            break;
        case Styles::INSTALL:
            _style.foreground = wxColour(128,128,128);
            break;
        case Styles::DEFAULT_VARS: // x y z t
            _style.foreground = wxColour(0,0,160);
            _style.bold = true;
            _style.italics = true;
            break;
        case Styles::ACTIVE_LINE:
            _style.background = wxColour(221,230,255);
            _style.defaultbackground = false;
            break;
        case Styles::STYLE_END:
            break;
        // missing default intended => will result in warning, if a enum case is not handled in switch
    }
    return _style;
}

void Options::readColoursFromConfig(wxFileConfig* _config)
{
    if (!_config->HasGroup("Styles"))
        return;
    wxString KeyValues[] = {
        "STANDARD",
        "CONSOLE_STD",
        "COMMAND",
        "COMMENT",
        "OPTION",
        "FUNCTION",
        "CUSTOM_FUNCTION",
        "CONSTANT",
        "SPECIALVAL", // ans cache ...
        "STRING",
        "STRINGPARSER",
        "INCLUDES",
        "OPERATOR",
        "PROCEDURE",
        "NUMBER",
        "PROCEDURE_COMMAND",
        "METHODS",
        "INSTALL",
        "DEFAULT_VARS", // x y z t
        "ACTIVE_LINE",
        "STYLE_END"
    };
    wxString val;
    for (size_t i = 0; i < Styles::STYLE_END; i++)
    {
        if (_config->Read("Styles/"+KeyValues[i], &val))
        {
            vSyntaxStyles[i] = convertFromString(val);
        }
    }
}

void Options::writeColoursToConfig(wxFileConfig* _config)
{
    wxString KeyValues[] = {
        "STANDARD",
        "CONSOLE_STD",
        "COMMAND",
        "COMMENT",
        "OPTION",
        "FUNCTION",
        "CUSTOM_FUNCTION",
        "CONSTANT",
        "SPECIALVAL", // ans cache ...
        "STRING",
        "STRINGPARSER",
        "INCLUDES",
        "OPERATOR",
        "PROCEDURE",
        "NUMBER",
        "PROCEDURE_COMMAND",
        "METHODS",
        "INSTALL",
        "DEFAULT_VARS", // x y z t
        "ACTIVE_LINE",
        "STYLE_END"
    };
    for (size_t i = 0; i < Styles::STYLE_END; i++)
    {
        _config->Write("Styles/"+KeyValues[i], convertToString(vSyntaxStyles[i]));
    }
}

void Options::SetStyleForeground(size_t i, const wxColour& color)
{
    if (i < Styles::STYLE_END)
    {
        vSyntaxStyles[i].foreground = color;
    }
}

void Options::SetStyleBackground(size_t i, const wxColour& color)
{
    if (i < Styles::STYLE_END)
    {
        vSyntaxStyles[i].background = color;
    }
}


void Options::SetStyleDefaultBackground(size_t i, bool defaultbackground)
{
    if (i < Styles::STYLE_END)
    {
        vSyntaxStyles[i].defaultbackground = defaultbackground;
    }
}

void Options::SetStyleBold(size_t i, bool _bold)
{
    if (i < Styles::STYLE_END)
    {
        vSyntaxStyles[i].bold = _bold;
    }
}

void Options::SetStyleItalics(size_t i, bool _italics)
{
    if (i < Styles::STYLE_END)
    {
        vSyntaxStyles[i].italics = _italics;
    }
}

void Options::SetStyleUnderline(size_t i, bool _underline)
{
    if (i < Styles::STYLE_END)
    {
        vSyntaxStyles[i].underline = _underline;
    }
}



wxArrayString Options::GetStyleIdentifier()
{
    wxArrayString sReturn;
    sReturn.Add("STANDARD");
    sReturn.Add("CONSOLE_STD");
    sReturn.Add("COMMAND");
    sReturn.Add("COMMENT");
    sReturn.Add("OPTION");
    sReturn.Add("FUNCTION");
    sReturn.Add("CUSTOM_FUNCTION");
    sReturn.Add("CONSTANT");
    sReturn.Add("SPECIALVAL");
    sReturn.Add("STRING");
    sReturn.Add("STRINGPARSER");
    sReturn.Add("INCLUDES");
    sReturn.Add("OPERATOR");
    sReturn.Add("PROCEDURE");
    sReturn.Add("NUMBER");
    sReturn.Add("PROCEDURE_COMMAND");
    sReturn.Add("METHODS");
    sReturn.Add("INSTALL");
    sReturn.Add("DEFAULT_VARS");
    sReturn.Add("ACTIVE_LINE");

    return sReturn;
}


size_t Options::GetIdByIdentifier(const wxString& identifier)
{
    wxArrayString identifiers = GetStyleIdentifier();

    return identifiers.Index(identifier);
}








