#include "Options.h"

// Includes:
#include <wx/filename.h>
#include <wx/arrstr.h>
#include <wx/dir.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

int StrToInt(const string&);
string toString(int);

Options::Options()
{
	// Set Some Default Values (perhaps these should not be set!)
	m_terminalSize = 300;
	m_caretBlinkTime = 500;

	m_debuggerFocusLine = 10;
    m_showLineNumbersInStackTrace = true;
    m_showModulesInStackTrace = true;
    m_showProcedureArguments = true;
    m_showGlobalVariables = false;

	m_LaTeXRoot = "C:/Program Files";

	// Default to printing with black text, white background
	m_printStyle = wxSTC_PRINT_BLACKONWHITE;


	m_showToolbarText = false;
	m_showPathsOnTabs = false;
	m_printLineNumbers = false;
	m_saveSession = false;
	m_saveBookmarksInSession = false;
	m_formatBeforeSaving = false;
	m_keepBackupFile = true;
	m_foldDuringLoading = false;
	m_highlightLocalVariables = false;

	m_editorFont.SetNativeFontInfoUserDesc("Consolas 10");
	m_editorFont.SetEncoding(wxFONTENCODING_CP1252);

	setDefaultSyntaxStyles();

	for (size_t i = 0; i < AnalyzerOptions::ANALYZER_OPTIONS_END; i++)
        vAnalyzerOptions.push_back(1);

    vAnalyzerOptions[AnalyzerOptions::ALWAYS_SHOW_METRICS] = 0;
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

void Options::SetPrintStyle(int style)
{
	m_printStyle = style;
}

void Options::SetShowToolbarText(bool useText)
{
	m_showToolbarText = useText;
}

void Options::SetTerminalHistorySize(int size)
{
	m_terminalSize = size;
}

void Options::SetLineNumberPrinting(bool printLineNumbers)
{
	m_printLineNumbers = printLineNumbers;
}

void Options::SetAnalyzerOption(AnalyzerOptions opt, int nVal)
{
    if (opt < ANALYZER_OPTIONS_END && vAnalyzerOptions.size())
        vAnalyzerOptions[opt] = nVal;
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
            //_style.bold = true;
            break;
        case Styles::DOCCOMMENT:
            _style.foreground = wxColour(0,128,192);
            _style.background = wxColour(255,255,183);
            _style.defaultbackground = false;
            _style.bold = true;
            break;
        case Styles::DOCKEYWORD:
            _style.foreground = wxColour(128,0,0);
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
        case Styles::CLUSTER:
            _style.foreground = wxColour(96, 96, 96);
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

    static wxString KeyValues[] = {
        "STANDARD",
        "CONSOLE_STD",
        "COMMAND",
        "COMMENT",
        "DOCCOMMENT",
        "DOCKEYWORD",
        "OPTION",
        "FUNCTION",
        "CUSTOM_FUNCTION",
        "CLUSTER",
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
    static wxString KeyValues[] = {
        "STANDARD",
        "CONSOLE_STD",
        "COMMAND",
        "COMMENT",
        "DOCCOMMENT",
        "DOCKEYWORD",
        "OPTION",
        "FUNCTION",
        "CUSTOM_FUNCTION",
        "CLUSTER",
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

void Options::readAnalyzerOptionsFromConfig(wxFileConfig* _config)
{
    if (!_config->HasGroup("AnalyzerOptions"))
        return;

    static wxString KeyValues[] = {
        "USE_NOTES",
        "USE_WARNINGS",
        "USE_ERRORS",
        "COMMENT_DENSITY",
        "LINES_OF_CODE",
        "COMPLEXITY",
        "MAGIC_NUMBERS",
        "ALWAYS_SHOW_METRICS",
        "INLINE_IF",
        "CONSTANT_EXPRESSION",
        "RESULT_SUPPRESSION",
        "RESULT_ASSIGNMENT",
        "TYPE_ORIENTATION",
        "ARGUMENT_UNDERSCORE",
        "VARIABLE_LENGTH",
        "UNUSED_VARIABLES",
        "PROCEDURE_LENGTH",
        "THISFILE_NAMESPACE",
        "PROGRESS_RUNTIME",
        "SWITCH_FALLTHROUGH",
        "GLOBAL_VARIABLES",
        "ANALYZER_OPTIONS_END"
    };

    wxString val;

    for (size_t i = 0; i < AnalyzerOptions::ANALYZER_OPTIONS_END; i++)
    {
        if (_config->Read("AnalyzerOptions/"+KeyValues[i], &val))
        {
            vAnalyzerOptions[i] = StrToInt(val.ToStdString());
        }
    }
}

void Options::writeAnalyzerOptionsToConfig(wxFileConfig* _config)
{
    static wxString KeyValues[] = {
        "USE_NOTES",
        "USE_WARNINGS",
        "USE_ERRORS",
        "COMMENT_DENSITY",
        "LINES_OF_CODE",
        "COMPLEXITY",
        "MAGIC_NUMBERS",
        "ALWAYS_SHOW_METRICS",
        "INLINE_IF",
        "CONSTANT_EXPRESSION",
        "RESULT_SUPPRESSION",
        "RESULT_ASSIGNMENT",
        "TYPE_ORIENTATION",
        "ARGUMENT_UNDERSCORE",
        "VARIABLE_LENGTH",
        "UNUSED_VARIABLES",
        "PROCEDURE_LENGTH",
        "THISFILE_NAMESPACE",
        "PROGRESS_RUNTIME",
        "SWITCH_FALLTHROUGH",
        "GLOBAL_VARIABLES",
        "ANALYZER_OPTIONS_END"
    };

    for (size_t i = 0; i < AnalyzerOptions::ANALYZER_OPTIONS_END; i++)
    {
        _config->Write("AnalyzerOptions/"+KeyValues[i], ::toString(vAnalyzerOptions[i]).c_str());
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

int Options::GetAnalyzerOption(AnalyzerOptions opt)
{
    if (opt < ANALYZER_OPTIONS_END && vAnalyzerOptions.size())
        return vAnalyzerOptions[opt];

    return 0;
}


wxArrayString Options::GetStyleIdentifier()
{
    wxArrayString sReturn;
    sReturn.Add("STANDARD");
    sReturn.Add("CONSOLE_STD");
    sReturn.Add("COMMAND");
    sReturn.Add("COMMENT");
    sReturn.Add("DOCUMENTATIONCOMMENT");
    sReturn.Add("DOCUMENTATIONKEYWORD");
    sReturn.Add("OPTION");
    sReturn.Add("FUNCTION");
    sReturn.Add("CUSTOM_FUNCTION");
    sReturn.Add("CLUSTER");
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








