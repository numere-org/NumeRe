#include "Options.h"

// Includes:
#include <wx/filename.h>
#include <wx/arrstr.h>
#include <wx/dir.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

Options::Options() : Settings()
{
}

Options::~Options()
{
}

/////////////////////////////////////////////////
/// \brief Change a static code analyzer setting.
///
/// \param opt AnalyzerOptions
/// \param nVal int
/// \return void
///
/////////////////////////////////////////////////
void Options::SetAnalyzerOption(AnalyzerOptions opt, int nVal)
{
    std::string setting = analyzerOptsToString(opt);

    if (setting.length())
        m_settings[setting].active() = nVal;
}


/////////////////////////////////////////////////
/// \brief Return the default syntax style as
/// defined by the programmers.
///
/// \param i size_t
/// \return SyntaxStyles
///
/////////////////////////////////////////////////
SyntaxStyles Options::GetDefaultSyntaxStyle(size_t i) const
{
    switch (i)
    {
        case Styles::STANDARD:
            return SyntaxStyles(DEFAULT_ST_STANDARD);
        case Styles::CONSOLE_STD:
            return SyntaxStyles(DEFAULT_ST_CONSOLESTD);
        case Styles::COMMAND:
            return SyntaxStyles(DEFAULT_ST_COMMAND);
        case Styles::COMMENT:
            return SyntaxStyles(DEFAULT_ST_COMMENT);
        case Styles::DOCCOMMENT:
            return SyntaxStyles(DEFAULT_ST_DOCCOMMENT);
        case Styles::DOCKEYWORD:
            return SyntaxStyles(DEFAULT_ST_DOCKEYWORD);
        case Styles::OPTION:
            return SyntaxStyles(DEFAULT_ST_OPTION);
        case Styles::FUNCTION:
            return SyntaxStyles(DEFAULT_ST_FUNCTION);
        case Styles::CUSTOM_FUNCTION:
            return SyntaxStyles(DEFAULT_ST_CUSTOMFUNC);
        case Styles::CLUSTER:
            return SyntaxStyles(DEFAULT_ST_CLUSTER);
        case Styles::CONSTANT:
            return SyntaxStyles(DEFAULT_ST_CONSTANT);
        case Styles::SPECIALVAL: // ans cache ...
            return SyntaxStyles(DEFAULT_ST_SPECIALVAL);
        case Styles::STRING:
            return SyntaxStyles(DEFAULT_ST_STRING);
        case Styles::STRINGPARSER:
            return SyntaxStyles(DEFAULT_ST_STRINGPARSER);
        case Styles::OPERATOR:
            return SyntaxStyles(DEFAULT_ST_OPERATOR);
        case Styles::INCLUDES:
            return SyntaxStyles(DEFAULT_ST_INCLUDES);
        case Styles::PROCEDURE:
            return SyntaxStyles(DEFAULT_ST_PROCEDURE);
        case Styles::PROCEDURE_COMMAND:
            return SyntaxStyles(DEFAULT_ST_PROCCOMMAND);
        case Styles::NUMBER:
            return SyntaxStyles(DEFAULT_ST_NUMBER);
        case Styles::METHODS:
            return SyntaxStyles(DEFAULT_ST_METHODS);
        case Styles::INSTALL:
            return SyntaxStyles(DEFAULT_ST_INSTALL);
        case Styles::DEFAULT_VARS: // x y z t
            return SyntaxStyles(DEFAULT_ST_DEFVARS);
        case Styles::ACTIVE_LINE:
            return SyntaxStyles(DEFAULT_ST_ACTIVELINE);
        case Styles::STYLE_END:
            break;
        // missing default intended => will result in warning, if a enum case is not handled in switch
    }

    return SyntaxStyles();
}


/////////////////////////////////////////////////
/// \brief Return the selected syntax style by
/// constructing it from the style string.
///
/// \param i size_t
/// \return SyntaxStyles
///
/////////////////////////////////////////////////
SyntaxStyles Options::GetSyntaxStyle(size_t i) const
{
    std::string style = syntaxStylesToString((Styles)i);

    if (style.length())
        return SyntaxStyles(m_settings.at(style).stringval());

    return SyntaxStyles();
}


/////////////////////////////////////////////////
/// \brief Update a selected syntax style by
/// converting the passed SyntaxStyles object
/// into a style string and storing it internally.
///
/// \param i size_t
/// \param styles const SyntaxStyles&
/// \return void
///
/////////////////////////////////////////////////
void Options::SetSyntaxStyle(size_t i, const SyntaxStyles& styles)
{
    std::string style = syntaxStylesToString((Styles)i);

    if (style.length())
        m_settings[style].stringval() = styles.to_string();
}


/////////////////////////////////////////////////
/// \brief Legacy syntax style import function
/// from file configuration.
///
/// \param _config wxFileConfig*
/// \return void
///
/////////////////////////////////////////////////
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
            std::string style = syntaxStylesToString((Styles)i);

            if (style.length())
                m_settings[style].stringval() = SyntaxStyles(val).to_string();
        }
    }
}


/////////////////////////////////////////////////
/// \brief Legacy analyzer import function from
/// file configuration.
///
/// \param _config wxFileConfig*
/// \return void
///
/////////////////////////////////////////////////
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
        "MISLEADING_TYPE",
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
            SetAnalyzerOption((AnalyzerOptions)i, StrToInt(val.ToStdString()));
        }
    }
}


/////////////////////////////////////////////////
/// \brief Return the value of the selected
/// static code analyzer option.
///
/// \param opt AnalyzerOptions
/// \return int
///
/////////////////////////////////////////////////
int Options::GetAnalyzerOption(AnalyzerOptions opt) const
{
    std::string setting = analyzerOptsToString(opt);

    if (setting.length())
        return m_settings.at(setting).active();

    return 0;
}


/////////////////////////////////////////////////
/// \brief This member function returns an array
/// of the style enumeration identifiers
/// converted to a string.
///
/// \return wxArrayString
///
/////////////////////////////////////////////////
wxArrayString Options::GetStyleIdentifier() const
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


/////////////////////////////////////////////////
/// \brief This member function returns the ID of
/// a syntax style by passing the style
/// identifier as a string.
///
/// \param identifier const wxString&
/// \return size_t
///
/////////////////////////////////////////////////
size_t Options::GetIdByIdentifier(const wxString& identifier) const
{
    wxArrayString identifiers = GetStyleIdentifier();

    return identifiers.Index(identifier);
}


/////////////////////////////////////////////////
/// \brief Convert the static code analyzer
/// enumeration values into the new settings
/// strings.
///
/// \param opt AnalyzerOptions
/// \return std::string
///
/////////////////////////////////////////////////
std::string Options::analyzerOptsToString(AnalyzerOptions opt) const
{
    switch (opt)
    {
        case AnalyzerOptions::USE_NOTES:
            return SETTING_B_AN_USENOTES;
        case AnalyzerOptions::USE_WARNINGS:
            return SETTING_B_AN_USEWARNINGS;
        case AnalyzerOptions::USE_ERRORS:
            return SETTING_B_AN_USEERRORS;
        case AnalyzerOptions::COMMENT_DENSITY:
            return SETTING_B_AN_COMMENTDENS;
        case AnalyzerOptions::LINES_OF_CODE:
            return SETTING_B_AN_LOC;
        case AnalyzerOptions::COMPLEXITY:
            return SETTING_B_AN_COMPLEXITY;
        case AnalyzerOptions::MAGIC_NUMBERS:
            return SETTING_B_AN_MAGICNUMBERS;
        case AnalyzerOptions::ALWAYS_SHOW_METRICS:
            return SETTING_B_AN_ALWAYSMETRICS;
        case AnalyzerOptions::INLINE_IF:
            return SETTING_B_AN_INLINEIF;
        case AnalyzerOptions::CONSTANT_EXPRESSION:
            return SETTING_B_AN_CONSTANTS;
        case AnalyzerOptions::RESULT_SUPPRESSION:
            return SETTING_B_AN_RESULTSUP;
        case AnalyzerOptions::RESULT_ASSIGNMENT:
            return SETTING_B_AN_RESULTASS;
        case AnalyzerOptions::TYPE_ORIENTATION:
            return SETTING_B_AN_TYPING;
        case AnalyzerOptions::MISLEADING_TYPE:
            return SETTING_B_AN_MISLEADINGTYPE;
        case AnalyzerOptions::ARGUMENT_UNDERSCORE:
            return SETTING_B_AN_UNDERSCOREARGS;
        case AnalyzerOptions::VARIABLE_LENGTH:
            return SETTING_B_AN_VARLENGTH;
        case AnalyzerOptions::UNUSED_VARIABLES:
            return SETTING_B_AN_UNUSEDVARS;
        case AnalyzerOptions::PROCEDURE_LENGTH:
            return SETTING_B_AN_PROCLENGTH;
        case AnalyzerOptions::THISFILE_NAMESPACE:
            return SETTING_B_AN_THISFILE;
        case AnalyzerOptions::PROGRESS_RUNTIME:
            return SETTING_B_AN_PROGRESS;
        case AnalyzerOptions::SWITCH_FALLTHROUGH:
            return SETTING_B_AN_FALLTHROUGH;
        case AnalyzerOptions::GLOBAL_VARIABLES:
            return SETTING_B_AN_GLOBALVARS;
        case AnalyzerOptions::ANALYZER_OPTIONS_END:
            break;
    }

    return "";
}


/////////////////////////////////////////////////
/// \brief Convert the syntax style enumeration
/// values into the new settings strings.
///
/// \param style Styles
/// \return std::string
///
/////////////////////////////////////////////////
std::string Options::syntaxStylesToString(Styles style) const
{
    switch (style)
    {
        case Styles::STANDARD:
            return SETTING_S_ST_STANDARD;
        case Styles::CONSOLE_STD:
            return SETTING_S_ST_CONSOLESTD;
        case Styles::COMMAND:
            return SETTING_S_ST_COMMAND;
        case Styles::COMMENT:
            return SETTING_S_ST_COMMENT;
        case Styles::DOCCOMMENT:
            return SETTING_S_ST_DOCCOMMENT;
        case Styles::DOCKEYWORD:
            return SETTING_S_ST_DOCKEYWORD;
        case Styles::OPTION:
            return SETTING_S_ST_OPTION;
        case Styles::FUNCTION:
            return SETTING_S_ST_FUNCTION;
        case Styles::CUSTOM_FUNCTION:
            return SETTING_S_ST_CUSTOMFUNC;
        case Styles::CLUSTER:
            return SETTING_S_ST_CLUSTER;
        case Styles::CONSTANT:
            return SETTING_S_ST_CONSTANT;
        case Styles::SPECIALVAL:
            return SETTING_S_ST_SPECIALVAL;
        case Styles::STRING:
            return SETTING_S_ST_STRING;
        case Styles::STRINGPARSER:
            return SETTING_S_ST_STRINGPARSER;
        case Styles::INCLUDES:
            return SETTING_S_ST_INCLUDES;
        case Styles::OPERATOR:
            return SETTING_S_ST_OPERATOR;
        case Styles::PROCEDURE:
            return SETTING_S_ST_PROCEDURE;
        case Styles::NUMBER:
            return SETTING_S_ST_NUMBER;
        case Styles::PROCEDURE_COMMAND:
            return SETTING_S_ST_PROCCOMMAND;
        case Styles::METHODS:
            return SETTING_S_ST_METHODS;
        case Styles::INSTALL:
            return SETTING_S_ST_INSTALL;
        case Styles::DEFAULT_VARS:
            return SETTING_S_ST_DEFVARS;
        case Styles::ACTIVE_LINE:
            return SETTING_S_ST_ACTIVELINE;
        case Styles::STYLE_END:
            break;
    }

    return "";
}




