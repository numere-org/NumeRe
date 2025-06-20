#ifndef OPTIONS_H
#define OPTIONS_H

#include <wx/wx.h>
#include <wx/fileconf.h>
#include <wx/tokenzr.h>

#include <vector>

#include "datastructures.h"
#include "../kernel/core/settings.hpp"
#include "../kernel/core/utils/stringtools.hpp"

// copied from stc.h
// PrintColourMode - force black text on white background for printing.
#define wxSTC_PRINT_BLACKONWHITE 2

// PrintColourMode - text stays coloured, but all background is forced to be white for printing.
#define wxSTC_PRINT_COLOURONWHITE 3

/////////////////////////////////////////////////
/// \brief This structure contains the necessary
/// data to completely define a style for a
/// distinctive syntax element.
/////////////////////////////////////////////////
struct SyntaxStyles
{
    wxColour foreground;
    wxColour background;
    bool bold;
    bool italics;
    bool underline;
    bool defaultbackground;

    /////////////////////////////////////////////////
    /// \brief Convert a colorstring (r:g:b) into an
    /// actual color instance.
    ///
    /// \param colorstring const wxString&
    /// \return wxColour
    ///
    /////////////////////////////////////////////////
    wxColour StrToColorNew(const wxString& colorstring) const
    {
        wxArrayString channels = wxStringTokenize(colorstring, ":");
        return wxColour(StrToInt(channels[0].ToStdString()), StrToInt(channels[1].ToStdString()), StrToInt(channels[2].ToStdString()));
    }

    /////////////////////////////////////////////////
    /// \brief Convert a colorstring of the old
    /// representation (rrrgggbbb) into an actual
    /// color instance.
    ///
    /// \param colorstring const wxString&
    /// \return wxColour
    ///
    /////////////////////////////////////////////////
    wxColour StrToColorOld(const wxString& colorstring) const
    {
        unsigned char channel_r = StrToInt(colorstring.substr(0,3).ToStdString());
        unsigned char channel_g = StrToInt(colorstring.substr(3,3).ToStdString());
        unsigned char channel_b = StrToInt(colorstring.substr(6,3).ToStdString());
        return wxColour(channel_r, channel_g, channel_b);
    }

    /////////////////////////////////////////////////
    /// \brief Imports a new syntax style string
    /// (r:g:b-r:g:b-BIUD) and converts it into
    /// actual usable variables.
    ///
    /// \param styleDef const wxString&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void importNew(const wxString& styleDef)
    {
        wxArrayString styles = wxStringTokenize(styleDef, "-");

        foreground = StrToColorNew(styles[0]);
        background = StrToColorNew(styles[1]);

        bold = styles[2][0] == '1';
        italics = styles[2][1] == '1';
        underline = styles[2][2] == '1';
        defaultbackground = styles[2][3] == '1';
    }

    /////////////////////////////////////////////////
    /// \brief Imports an old syntax style string
    /// (rrrgggbbbrrrgggbbbBIUD) and converts it into
    /// actual usable variables.
    ///
    /// \param styleDef const wxString&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void importOld(const wxString& styleDef)
    {
        if (styleDef.length() < 21)
            return;

        foreground = StrToColorOld(styleDef.substr(0,9));
        background = StrToColorOld(styleDef.substr(9,9));

        if (styleDef[18] == '1')
            bold = true;

        if (styleDef[19] == '1')
            italics = true;

        if (styleDef[20] == '1')
            underline = true;

        if (styleDef.length() > 21)
        {
            if (styleDef[21] == '0')
                defaultbackground = false;
        }
        else
            defaultbackground = false;
    }

    /////////////////////////////////////////////////
    /// \brief Transforms the internal data into a
    /// string style representation according the
    /// following scheme: r:g:b-r:g:b-BIUD
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string to_string() const
    {
        std::string style;

        style = toString((int)foreground.Red()) + ":" + toString((int)foreground.Green()) + ":" + toString((int)foreground.Blue()) + "-";
        style += toString((int)background.Red()) + ":" + toString((int)background.Green()) + ":" + toString((int)background.Blue()) + "-";
        style += bold ? "1" : "0";
        style += italics ? "1" : "0";
        style += underline ? "1" : "0";
        style += defaultbackground ? "1" : "0";

        return style;
    }

    /////////////////////////////////////////////////
    /// \brief Default constructor. Creates a "style-
    /// less" style.
    /////////////////////////////////////////////////
    SyntaxStyles() : foreground(*wxBLACK), background(*wxWHITE), bold(false), italics(false), underline(false), defaultbackground(true) {}

    /////////////////////////////////////////////////
    /// \brief Generic constructor. Creates an
    /// instance of this class by importing a style
    /// string in new or old fashion.
    ///
    /// \param styleDef const wxString&
    ///
    /////////////////////////////////////////////////
    SyntaxStyles(const wxString& styleDef) : SyntaxStyles()
    {
        if (styleDef.find(':') != std::string::npos)
            importNew(styleDef);
        else
            importOld(styleDef);
    }
};





/////////////////////////////////////////////////
/// \brief This class implements an interface of
/// the internal Settings object adapted to be
/// usable from the GUI.
/////////////////////////////////////////////////
class Options : public Settings
{
	friend class OptionsDialog;

	public:
		Options();
		~Options();

		// Modifiers:
		void SetEditorFont(wxFont font)
            {
                font.SetEncoding(wxFONTENCODING_CP1252);
                m_settings[SETTING_S_EDITORFONT].stringval() = font.GetNativeFontInfoUserDesc().ToStdString();
            }

		void SetPrintStyle(int style)
            {m_settings[SETTING_B_PRINTINCOLOR].active() = (style == wxSTC_PRINT_COLOURONWHITE);}
		void SetTerminalHistorySize(int size)
            {m_settings[SETTING_V_BUFFERSIZE].value() = size;}
		void SetShowToolbarText(bool useText)
            {m_settings[SETTING_B_TOOLBARTEXT].active() = useText;}
		void SetShowPathOnTabs(bool useText)
            {m_settings[SETTING_B_PATHSONTABS].active() = useText;}
		void SetLineNumberPrinting(bool printLineNumbers)
            {m_settings[SETTING_B_PRINTLINENUMBERS].active() = printLineNumbers;}
		void SetSaveSession(bool saveSession)
            {m_settings[SETTING_B_SAVESESSION].active() = saveSession;}
		void SetSaveBookmarksInSession(bool save)
            {m_settings[SETTING_B_SAVEBOOKMARKS].active() = save;}
		void SetFormatBeforeSaving(bool formatBeforeSave)
            {m_settings[SETTING_B_FORMATBEFORESAVING].active() = formatBeforeSave;}
		void SetLaTeXRoot(const wxString& root)
            {m_settings[SETTING_S_LATEXROOT].stringval() = root.ToStdString();}
		void SetKeepBackupFile(bool keepFile)
            {m_settings[SETTING_B_USEREVISIONS].active() = keepFile;}
		void SetCaretBlinkTime(int nTime)
            {m_settings[SETTING_V_CARETBLINKTIME].value() = nTime;}
		void SetDebuggerFocusLine(int nLine)
            {m_settings[SETTING_V_FOCUSEDLINE].value() = nLine;}
		void SetShowLinesInStackTrace(bool show)
            {m_settings[SETTING_B_LINESINSTACK].active() = show;}
		void SetShowModulesInStackTrace(bool show)
            {m_settings[SETTING_B_MODULESINSTACK].active() = show;}
		void SetShowProcedureArguments(bool show)
            {m_settings[SETTING_B_PROCEDUREARGS].active() = show;}
		void SetShowGlobalVariables(bool show)
            {m_settings[SETTING_B_GLOBALVARS].active() = show;}
		void SetFoldDuringLoading(bool fold)
            {m_settings[SETTING_B_FOLDLOADEDFILE].active() = fold;}
		void SetHighlightLocalVariables(bool highlight)
            {m_settings[SETTING_B_HIGHLIGHTLOCALS].active() = highlight;}


		// Accessors:  (inlined)
		wxFont GetEditorFont() const
            {
                return toFont(m_settings.at(SETTING_S_EDITORFONT).stringval());
            }
		int GetPrintStyle() const
            {return m_settings.at(SETTING_B_PRINTINCOLOR).active() ? wxSTC_PRINT_COLOURONWHITE : wxSTC_PRINT_BLACKONWHITE;}
		wxString GetLaTeXRoot() const
            {return m_settings.at(SETTING_S_LATEXROOT).stringval();}
		bool GetShowToolbarText() const
            {return m_settings.at(SETTING_B_TOOLBARTEXT).active();}
		bool GetShowPathOnTabs() const
            {return m_settings.at(SETTING_B_PATHSONTABS).active();}
		bool GetLineNumberPrinting() const
            {return m_settings.at(SETTING_B_PRINTLINENUMBERS).active();}
		bool GetSaveSession() const
            {return m_settings.at(SETTING_B_SAVESESSION).active();}
		bool GetSaveBookmarksInSession() const
            {return m_settings.at(SETTING_B_SAVEBOOKMARKS).active();}
		bool GetFormatBeforeSaving() const
            {return m_settings.at(SETTING_B_FORMATBEFORESAVING).active();}
		bool GetKeepBackupFile() const
            {return m_settings.at(SETTING_B_USEREVISIONS).active();}
		int GetTerminalHistorySize() const
            {return m_settings.at(SETTING_V_BUFFERSIZE).value();}
		int GetCaretBlinkTime() const
            {return m_settings.at(SETTING_V_CARETBLINKTIME).value();}
		int GetDebuggerFocusLine() const
            {return m_settings.at(SETTING_V_FOCUSEDLINE).value();}
		bool GetShowLinesInStackTrace() const
            {return m_settings.at(SETTING_B_LINESINSTACK).active();}
		bool GetShowModulesInStackTrace() const
            {return m_settings.at(SETTING_B_MODULESINSTACK).active();}
		bool GetShowProcedureArguments() const
            {return m_settings.at(SETTING_B_PROCEDUREARGS).active();}
		bool GetShowGlobalVariables() const
            {return m_settings.at(SETTING_B_GLOBALVARS).active();}
		bool GetFoldDuringLoading() const
            {return m_settings.at(SETTING_B_FOLDLOADEDFILE).active();}
		bool GetHighlightLocalVariables() const
            {return m_settings.at(SETTING_B_HIGHLIGHTLOCALS).active();}

        /////////////////////////////////////////////////
        /// \brief An enumeration of all available syntax
        /// styles.
        /////////////////////////////////////////////////
		enum Styles
		{
            STANDARD,
            CONSOLE_STD,
            COMMAND,
            COMMENT,
            DOCCOMMENT,
            DOCKEYWORD,
            OPTION,
            FUNCTION,
            CUSTOM_FUNCTION,
            CLUSTER,
            CONSTANT,
            SPECIALVAL,
            STRING,
            STRINGPARSER,
            INCLUDES,
            OPERATOR,
            PROCEDURE,
            NUMBER,
            PROCEDURE_COMMAND,
            METHODS,
            CUSTOM_METHOD,
            INSTALL,
            DEFAULT_VARS,
            ACTIVE_LINE,
            UI_THEME,
            STYLE_END
		};

		enum UiThemeShades
		{
		    TOOLBAR = 100,
		    PANEL = 140,
		    TREE = 100,
		    EDITORMARGIN = 160,
		    STATUSBAR = 160,
		    GRIDLABELS = 140
		};

        /////////////////////////////////////////////////
        /// \brief An enumeration of all available static
        /// analyzer options.
        /////////////////////////////////////////////////
		enum AnalyzerOptions
		{
		    USE_NOTES,
		    USE_WARNINGS,
		    USE_ERRORS,
		    COMMENT_DENSITY,
		    LINES_OF_CODE,
		    COMPLEXITY,
            MAGIC_NUMBERS,
            ALWAYS_SHOW_METRICS,
            INLINE_IF,
            CONSTANT_EXPRESSION,
            RESULT_SUPPRESSION,
            RESULT_ASSIGNMENT,
            TYPE_ORIENTATION,
            MISLEADING_TYPE,
            ARGUMENT_UNDERSCORE,
            VARIABLE_LENGTH,
            UNUSED_VARIABLES,
            PROCEDURE_LENGTH,
            THISFILE_NAMESPACE,
            PROGRESS_RUNTIME,
            SWITCH_FALLTHROUGH,
            GLOBAL_VARIABLES,
            TYPE_MISUSE,
		    ANALYZER_OPTIONS_END
		};

        SyntaxStyles GetDefaultSyntaxStyle(size_t i) const;
		SyntaxStyles GetSyntaxStyle(size_t i) const;
		void SetSyntaxStyle(size_t i, const SyntaxStyles& styles);

        void SetAnalyzerOption(AnalyzerOptions opt, int nVal);
        int GetAnalyzerOption(AnalyzerOptions opt) const;

        void readColoursFromConfig(wxFileConfig* _config);
        void readAnalyzerOptionsFromConfig(wxFileConfig* _config);

        wxArrayString GetStyleIdentifier() const;
        size_t GetIdByIdentifier(const wxString& identifier) const;

        static wxFont toFont(const std::string& sFontDescr)
        {
            wxFont font;
            font.SetNativeFontInfoUserDesc(sFontDescr);
            return font;
        }

        static std::string toString(wxFont font)
        {
            font.SetEncoding(wxFONTENCODING_CP1252);
            return font.GetNativeFontInfoUserDesc().ToStdString();
        }

	private:
		std::string analyzerOptsToString(AnalyzerOptions opt) const;
		std::string syntaxStylesToString(Styles style) const;
};



#endif
