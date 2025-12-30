/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2014  Erik Haenel et al.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/


#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <string>
#include <map>

#ifndef RELEASE
#include <cassert>
#endif

#include "ui/error.hpp"
#include "io/filesystem.hpp"

#define SETTING_B_DEVELOPERMODE       "internal.developermode"
#define SETTING_B_DEBUGGER            "internal.debugger"
#define SETTING_B_SYSTEMPRINTS        "internal.usesystemprints"
#define SETTING_B_DRAFTMODE           "plotting.draftmode"
#define SETTING_B_COMPACT             "table.compact"
#define SETTING_B_LOADEMPTYCOLS       "table.loademptycols"
#define SETTING_B_SHOWHINTS           "ui.showhints"
#define SETTING_B_USECUSTOMLANG       "ui.usecustomlang"
#define SETTING_B_EXTERNALDOCWINDOW   "ui.externaldocwindow"
#define SETTING_B_EXTENDEDFILEINFO    "io.extendedfileinfo"
#define SETTING_B_LOGFILE             "io.logfile"
#define SETTING_B_DEFCONTROL          "io.defcontrol"
#define SETTING_B_TABLEREFS           "flowctrl.alwaysreferencetables"
#define SETTING_B_USEESCINSCRIPTS     "flowctrl.useescinscripts"
#define SETTING_B_ENABLEEXECUTE       "flowctrl.enableexecute"
#define SETTING_B_MASKDEFAULT         "flowctrl.maskdefault"
#define SETTING_B_DECODEARGUMENTS     "debugger.decodearguments"
#define SETTING_B_GREETING            "terminal.greeting"
#define SETTING_V_PRECISION           "terminal.precision"
#define SETTING_V_WINDOW_X            "terminal.windowsize.x"
#define SETTING_V_WINDOW_Y            "terminal.windowsize.y"
#define SETTING_V_BUFFERSIZE          "terminal.buffersize"
#define SETTING_S_TERMINALFONT        "terminal.font"
#define SETTING_S_HISTORYFONT         "history.font"
#define SETTING_V_AUTOSAVE            "table.autosave"
#define SETTING_S_EXEPATH             "path.exepath"
#define SETTING_S_SAVEPATH            "path.savepath"
#define SETTING_S_LOADPATH            "path.loadpath"
#define SETTING_S_PLOTPATH            "path.plotpath"
#define SETTING_S_SCRIPTPATH          "path.scriptpath"
#define SETTING_S_PROCPATH            "path.procpath"
#define SETTING_S_WORKPATH            "path.workpath"
#define SETTING_S_LOADPATHMASK        "path.loadpath.filemask"
#define SETTING_S_SAVEPATHMASK        "path.savepath.filemask"
#define SETTING_S_PLOTPATHMASK        "path.plotpath.filemask"
#define SETTING_S_SCRIPTPATHMASK      "path.scriptpath.filemask"
#define SETTING_S_PROCPATHMASK        "path.procpath.filemask"
#define SETTING_S_PLOTFONT            "plotting.plotfont"

// Setting value definitions for the GUI
#define SETTING_B_SHOWGRIDLINES       "table.ui.showgrid"
#define SETTING_B_AUTOGROUPCOLS       "table.ui.autogroupcols"
#define SETTING_B_SHOWQMARKS          "table.ui.showquotationmarks"
#define SETTING_S_LATEXROOT           "path.latexpath"
#define SETTING_V_CARETBLINKTIME      "ui.caretblinktime"
#define SETTING_V_FOCUSEDLINE         "debugger.focusedline"
#define SETTING_B_LINESINSTACK        "debugger.linenumbersinstacktrace"
#define SETTING_B_MODULESINSTACK      "debugger.modulesinstacktrace"
#define SETTING_B_GLOBALVARS          "debugger.showglobalvars"
#define SETTING_B_PROCEDUREARGS       "debugger.showarguments"
#define SETTING_B_FLASHTASKBAR        "debugger.flashtaskbar"
#define SETTING_B_POINTTOERROR        "debugger.alwayspointtoerror"
#define SETTING_B_TOOLBARTEXT         "ui.showtoolbartext"
#define SETTING_B_TOOLBARSTRETCH      "ui.stretchingtoolbar"
#define SETTING_B_PATHSONTABS         "ui.showpathsontabs"
#define SETTING_S_TOOLBARICONSTYLE    "ui.toolbariconstyle"
#define SETTING_B_ICONSONTABS         "ui.showiconsontabs"
#define SETTING_B_FLOATONPARENT       "ui.floatonparent"
#define SETTING_V_POS_SASH_V          "ui.position.verticalsash"
#define SETTING_V_POS_SASH_H          "ui.position.horizontalsash"
#define SETTING_V_POS_SASH_T          "ui.position.terminalsash"
#define SETTING_S_WINDOWSIZE          "ui.windowsize"
#define SETTING_S_UITHEME             "ui.theme"
#define SETTING_B_WINDOWSHOWN         "ui.windowshown"
#define SETTING_B_APPAUTOCLOSE        "ui.appautoclose"
#define SETTING_B_PRINTINCOLOR        "print.usecolor"
#define SETTING_B_PRINTLINENUMBERS    "print.linenumbers"
#define SETTING_B_SAVESESSION         "save.session"
#define SETTING_B_SAVEBOOKMARKS       "save.bookmarks"
#define SETTING_B_FORMATBEFORESAVING  "save.format"
#define SETTING_B_USEREVISIONS        "save.revisions"
#define SETTING_B_AUTOSAVEEXECUTION   "save.beforeexecution"
#define SETTING_B_SAVESASHS           "save.sashs"
#define SETTING_B_SAVEWINDOWSIZE      "save.windowsize"
#define SETTING_B_FOLDLOADEDFILE      "editor.foldloadedfile"
#define SETTING_B_HIGHLIGHTLOCALS     "editor.highlightlocals"
#define SETTING_B_USETABS             "editor.usetabs"
#define SETTING_B_LINELENGTH          "editor.linelengthindicator"
#define SETTING_B_DEFPAGERELNOTES     "editor.defaultpage.withreleasenotes"
#define SETTING_B_ALWAYSDEFPAGE       "editor.defaultpage.showalways"
#define SETTING_B_HOMEENDCANCELS      "editor.autocomp.homeendcancels"
#define SETTING_B_BRACEAUTOCOMP       "editor.autocomp.braces"
#define SETTING_B_QUOTEAUTOCOMP       "editor.autocomp.quotes"
#define SETTING_B_BLOCKAUTOCOMP       "editor.autocomp.blocks"
#define SETTING_B_SMARTSENSE          "editor.autocomp.smartsense"
#define SETTING_B_CALLTIP_ARGS        "editor.calltip.detectarguments"
#define SETTING_S_EDITORFONT          "editor.font"
#define SETTING_B_AN_START            "editor.analyzer._"
#define SETTING_B_AN_USENOTES         "editor.analyzer.usenotes"
#define SETTING_B_AN_USEWARNINGS      "editor.analyzer.usewarnings"
#define SETTING_B_AN_USEERRORS        "editor.analyzer.useerrors"
#define SETTING_B_AN_MAGICNUMBERS     "editor.analyzer.style.magicnumbers"
#define SETTING_B_AN_UNDERSCOREARGS   "editor.analyzer.style.underscoredarguments"
#define SETTING_B_AN_THISFILE         "editor.analyzer.namespace.thisfile"
#define SETTING_B_AN_COMMENTDENS      "editor.analyzer.metrics.commentdensity"
#define SETTING_B_AN_LOC              "editor.analyzer.metrics.linesofcode"
#define SETTING_B_AN_COMPLEXITY       "editor.analyzer.metrics.complexity"
#define SETTING_B_AN_ALWAYSMETRICS    "editor.analyzer.metrics.showalways"
#define SETTING_B_AN_RESULTSUP        "editor.analyzer.result.suppression"
#define SETTING_B_AN_RESULTASS        "editor.analyzer.result.assignment"
#define SETTING_B_AN_TYPING           "editor.analyzer.variables.typing"
#define SETTING_B_AN_MISLEADINGTYPE   "editor.analyzer.variables.misleadingtype"
#define SETTING_B_AN_VARLENGTH        "editor.analyzer.variables.length"
#define SETTING_B_AN_UNUSEDVARS       "editor.analyzer.variables.unused"
#define SETTING_B_AN_GLOBALVARS       "editor.analyzer.variables.globals"
#define SETTING_B_AN_TYPE_MISUSE      "editor.analyzer.variables.typemisuse"
#define SETTING_B_AN_CONSTANTS        "editor.analyzer.runtime.constants"
#define SETTING_B_AN_INLINEIF         "editor.analyzer.runtime.inlineif"
#define SETTING_B_AN_PROCLENGTH       "editor.analyzer.runtime.procedurelength"
#define SETTING_B_AN_PROGRESS         "editor.analyzer.runtime.progress"
#define SETTING_B_AN_FALLTHROUGH      "editor.analyzer.switch.fallthrough"
#define SETTING_S_ST_START            "editor.style._"
#define SETTING_S_ST_STANDARD         "editor.style.standard.editor"
#define SETTING_S_ST_CONSOLESTD       "editor.style.standard.terminal"
#define SETTING_S_ST_COMMAND          "editor.style.command.default"
#define SETTING_S_ST_PROCCOMMAND      "editor.style.command.procedure"
#define SETTING_S_ST_COMMENT          "editor.style.comment"
#define SETTING_S_ST_DOCCOMMENT       "editor.style.documentation.comment"
#define SETTING_S_ST_DOCKEYWORD       "editor.style.documentation.keyword"
#define SETTING_S_ST_OPTION           "editor.style.option"
#define SETTING_S_ST_FUNCTION         "editor.style.function.builtin"
#define SETTING_S_ST_CUSTOMFUNC       "editor.style.function.custom"
#define SETTING_S_ST_CLUSTER          "editor.style.cluster"
#define SETTING_S_ST_CONSTANT         "editor.style.constant"
#define SETTING_S_ST_SPECIALVAL       "editor.style.specialval"
#define SETTING_S_ST_STRING           "editor.style.string.literals"
#define SETTING_S_ST_STRINGPARSER     "editor.style.string.parser"
#define SETTING_S_ST_INCLUDES         "editor.style.includes"
#define SETTING_S_ST_OPERATOR         "editor.style.operator"
#define SETTING_S_ST_PROCEDURE        "editor.style.procedure"
#define SETTING_S_ST_NUMBER           "editor.style.number"
#define SETTING_S_ST_METHODS          "editor.style.methods.builtin"
#define SETTING_S_ST_CUSTOMMETHOD     "editor.style.methods.custom"
#define SETTING_S_ST_INSTALL          "editor.style.install"
#define SETTING_S_ST_DEFVARS          "editor.style.defaultvariables"
#define SETTING_S_ST_ACTIVELINE       "editor.style.activeline"

// Default color values
#define DEFAULT_ST_STANDARD      "0:0:0-255:255:255-0100"
#define DEFAULT_ST_CONSOLESTD    "0:0:100-255:255:255-0000"
#define DEFAULT_ST_COMMAND       "0:128:255-255:255:255-1011"
#define DEFAULT_ST_PROCCOMMAND   "128:0:0-255:255:255-1011"
#define DEFAULT_ST_COMMENT       "0:128:0-255:255:183-0000"
#define DEFAULT_ST_DOCCOMMENT    "0:128:192-255:255:183-1000"
#define DEFAULT_ST_DOCKEYWORD    "128:0:0-255:255:183-1000"
#define DEFAULT_ST_OPTION        "0:128:100-255:255:255-0001"
#define DEFAULT_ST_FUNCTION      "0:0:255-255:255:255-1001"
#define DEFAULT_ST_CUSTOMFUNC    "0:0:160-255:255:255-0001"
#define DEFAULT_ST_CLUSTER       "96:96:96-255:255:255-0001"
#define DEFAULT_ST_CONSTANT      "255:0:128-255:255:255-1001"
#define DEFAULT_ST_SPECIALVAL    "0:0:0-255:255:255-1001"
#define DEFAULT_ST_STRING        "128:128:255-255:255:255-0001"
#define DEFAULT_ST_STRINGPARSER  "0:128:192-255:255:255-1001"
#define DEFAULT_ST_INCLUDES      "128:0:0-255:255:255-1001"
#define DEFAULT_ST_OPERATOR      "255:0:0-255:255:255-0001"
#define DEFAULT_ST_PROCEDURE     "128:0:0-255:255:255-1001"
#define DEFAULT_ST_NUMBER        "176:150:0-255:255:255-0001"
#define DEFAULT_ST_METHODS       "0:180:50-255:255:255-1001"
#define DEFAULT_ST_CUSTOMMETHOD  "0:128:35-255:255:255-0001"
#define DEFAULT_ST_INSTALL       "128:128:128-255:255:255-0001"
#define DEFAULT_ST_DEFVARS       "0:0:160-255:255:255-1101"
#define DEFAULT_ST_ACTIVELINE    "0:0:0-221:230:255-0000"

#define DEFAULT_ST_UITHEME       "153:209:255-255:255:255-0000"



/*
 * Headerdatei zur Settings-Klasse
 */




/////////////////////////////////////////////////
/// \brief This class represents a single
/// abstract settings value implemented as void*.
/// We're using asserts to avoid memory issues.
/// Those won't trigger in release mode, of
/// course. A setting value might either be a
/// boolean, a unsigned int or a std::string.
/////////////////////////////////////////////////
class SettingsValue
{
    public:
        /////////////////////////////////////////////////
        /// \brief The type of the setting value.
        /////////////////////////////////////////////////
        enum SettingsValueType
        {
            TYPELESS,
            BOOL,
            UINT,
            STRING
        };

        /////////////////////////////////////////////////
        /// \brief Additional setting value properties.
        /////////////////////////////////////////////////
        enum SettingsValueProperties
        {
            NONE = 0x0,
            SAVE = 0x1,
            HIDDEN = 0x2,
            PATH = 0x4,
            IMMUTABLE = 0x8,
            UIREFRESH = 0x10
        };

    private:
        void* m_value;
        SettingsValueType m_type;
        int m_valueProperties;
        size_t m_min;
        size_t m_max;

        /////////////////////////////////////////////////
        /// \brief Assign a new boolean value to the
        /// internal memory.
        ///
        /// \param value bool
        /// \return void
        ///
        /////////////////////////////////////////////////
        void assign(bool value)
        {
            m_type = BOOL;
            m_value = static_cast<void*>(new bool);
            *(static_cast<bool*>(m_value)) = value;
        }

        /////////////////////////////////////////////////
        /// \brief Assign a new unsigned int value to the
        /// internal memory.
        ///
        /// \param value size_t
        /// \return void
        ///
        /////////////////////////////////////////////////
        void assign(size_t value)
        {
            m_type = UINT;
            m_value = static_cast<void*>(new size_t);
            *(static_cast<size_t*>(m_value)) = value;
        }

        /////////////////////////////////////////////////
        /// \brief Assign a new std::string value to the
        /// internal memory.
        ///
        /// \param value const std::string&
        /// \return void
        ///
        /////////////////////////////////////////////////
        void assign(const std::string& value)
        {
            m_type = STRING;
            m_value = static_cast<void*>(new std::string);
            *(static_cast<std::string*>(m_value)) = value;
        }

        /////////////////////////////////////////////////
        /// \brief Clear the internal memory (called by
        /// the destructor, for example).
        ///
        /// \return void
        ///
        /////////////////////////////////////////////////
        void clear()
        {
            switch (m_type)
            {
                case BOOL:
                    delete (bool*)m_value;
                    break;
                case UINT:
                    delete (size_t*)m_value;
                    break;
                case STRING:
                    delete (std::string*)m_value;
                    break;
                case TYPELESS:
                    break;
            }

            m_type = TYPELESS;
            m_value = nullptr;
        }

    public:
        /////////////////////////////////////////////////
        /// \brief Default constructor. Creates an empty
        /// and typeless setting value.
        /////////////////////////////////////////////////
        SettingsValue() : m_value(nullptr), m_type(TYPELESS), m_valueProperties(SettingsValue::NONE), m_min(0), m_max(0) {}

        /////////////////////////////////////////////////
        /// \brief Create a setting value from a boolean.
        ///
        /// \param value bool
        /// \param properties int
        ///
        /////////////////////////////////////////////////
        explicit SettingsValue(bool value, int properties = SettingsValue::SAVE) : m_min(0), m_max(0)
        {
            m_valueProperties = properties;
            assign(value);
        }

        /////////////////////////////////////////////////
        /// \brief Create a setting value from an
        /// unsigned integer and define its minimal and
        /// maximal possible values
        ///
        /// \param value size_t
        /// \param _min size_t
        /// \param _max size_t
        /// \param properties int
        ///
        /////////////////////////////////////////////////
        SettingsValue(size_t value, size_t _min, size_t _max, int properties = SettingsValue::SAVE)
        {
            m_valueProperties = properties;
            m_min = _min;
            m_max = _max;
            assign(value);
        }

        /////////////////////////////////////////////////
        /// \brief Create a setting value from a const
        /// char*, represented as a std::string internally.
        ///
        /// \param value const char*
        /// \param properties int
        ///
        /////////////////////////////////////////////////
        explicit SettingsValue(const char* value, int properties = SettingsValue::SAVE) : m_min(0), m_max(0)
        {
            m_valueProperties = properties;
            assign(std::string(value));
        }

        /////////////////////////////////////////////////
        /// \brief Create a setting value from a
        /// std::string.
        ///
        /// \param value const std::string&
        /// \param properties int
        ///
        /////////////////////////////////////////////////
        SettingsValue(const std::string& value, int properties = SettingsValue::SAVE) : m_min(0), m_max(0)
        {
            m_valueProperties = properties;
            assign(value);
        }

        /////////////////////////////////////////////////
        /// \brief Destructor. Will free the allocated
        /// memory.
        /////////////////////////////////////////////////
        ~SettingsValue()
        {
            clear();
        }

        /////////////////////////////////////////////////
        /// \brief Copy constructor. Creates a new
        /// instance and copies all contents. (Does not
        /// copy the pointers themselves, of course.)
        ///
        /// \param value const SettingsValue&
        ///
        /////////////////////////////////////////////////
        SettingsValue(const SettingsValue& value)
        {
            m_valueProperties = value.m_valueProperties;
            m_min = value.m_min;
            m_max = value.m_max;

            switch (value.m_type)
            {
                case BOOL:
                    assign(*(bool*)value.m_value);
                    break;
                case UINT:
                    assign(*(size_t*)value.m_value);
                    break;
                case STRING:
                    assign(*(std::string*)value.m_value);
                    break;
                case TYPELESS:
                    break;
            }
        }

        /////////////////////////////////////////////////
        /// \brief Assignment operator overload. Clears
        /// the contents of the assignee and takes the
        /// value and the type of the assigned value.
        ///
        /// \param value const SettingsValue&
        /// \return SettingsValue&
        /// \remark Does change the internal value type,
        /// if necessary.
        ///
        /////////////////////////////////////////////////
        SettingsValue& operator=(const SettingsValue& value)
        {
            clear();
            m_valueProperties = value.m_valueProperties;
            m_min = value.m_min;
            m_max = value.m_max;

            switch (value.m_type)
            {
                case BOOL:
                    assign(*(bool*)value.m_value);
                    break;
                case UINT:
                    assign(*(size_t*)value.m_value);
                    break;
                case STRING:
                    assign(*(std::string*)value.m_value);
                    break;
                case TYPELESS:
                    break;
            }

            return *this;
        }

        /////////////////////////////////////////////////
        /// \brief Get the internal value type.
        ///
        /// \return SettingsValueType
        ///
        /////////////////////////////////////////////////
        SettingsValueType getType() const
        {
            return m_type;
        }

        /////////////////////////////////////////////////
        /// \brief Returns, whether this is a setting
        /// value, which shall be saved to the
        /// configuration file.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool shallSave() const
        {
            return m_valueProperties & SAVE;
        }

        /////////////////////////////////////////////////
        /// \brief Returns, whether this setting value is
        /// an internal-only setting and should not be
        /// presented to and modified by the user.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isHidden() const
        {
            return m_valueProperties & HIDDEN;
        }

        /////////////////////////////////////////////////
        /// \brief Returns, whether this setting value
        /// represents a file path and a corresponding
        /// validation is necessary.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isPath() const
        {
#ifndef RELEASE
            assert(m_type == STRING);
#endif
            return m_valueProperties & PATH;
        }

        /////////////////////////////////////////////////
        /// \brief Returns, whether this setting value is
        /// mutable by the user in the terminal.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isMutable() const
        {
            return !(m_valueProperties & (IMMUTABLE | HIDDEN));
        }

        /////////////////////////////////////////////////
        /// \brief Returns true, when a setting modifies
        /// the graphical user interface and therefore
        /// needs to refresh it.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isUiModifying() const
        {
            return m_valueProperties & UIREFRESH;
        }

        /////////////////////////////////////////////////
        /// \brief Returns the minimal value of an
        /// unsigned int value type.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t min() const
        {
#ifndef RELEASE
            assert(m_type == UINT);
#endif
            return m_min;
        }

        /////////////////////////////////////////////////
        /// \brief Returns the maximal value of an
        /// unsigned int value type.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t max() const
        {
#ifndef RELEASE
            assert(m_type == UINT);
#endif
            return m_max;
        }

        /////////////////////////////////////////////////
        /// \brief Returns a reference to a boolean value
        /// type setting.
        ///
        /// \return bool&
        ///
        /////////////////////////////////////////////////
        bool& active()
        {
#ifndef RELEASE
            assert(m_type != TYPELESS);
            assert(m_value != nullptr);
            assert(m_type == BOOL);
#endif
            return *static_cast<bool*>(m_value);
        }

        /////////////////////////////////////////////////
        /// \brief Returns a reference to an unsigned int
        /// value type setting.
        ///
        /// \return size_t&
        ///
        /////////////////////////////////////////////////
        size_t& value()
        {
#ifndef RELEASE
            assert(m_type != TYPELESS);
            assert(m_value != nullptr);
            assert(m_type == UINT);
#endif
            return *static_cast<size_t*>(m_value);
        }

        /////////////////////////////////////////////////
        /// \brief Returns a reference to a std::string
        /// value type setting.
        ///
        /// \return std::string&
        ///
        /////////////////////////////////////////////////
        std::string& stringval()
        {
#ifndef RELEASE
            assert(m_type != TYPELESS);
            assert(m_value != nullptr);
            assert(m_type == STRING);
#endif
            return *static_cast<std::string*>(m_value);
        }

        /////////////////////////////////////////////////
        /// \brief Returns the value of a boolean value
        /// type setting.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool active() const
        {
#ifndef RELEASE
            assert(m_type != TYPELESS);
            assert(m_value != nullptr);
            assert(m_type == BOOL);
#endif
            return *static_cast<bool*>(m_value);
        }

        /////////////////////////////////////////////////
        /// \brief Returns the value of an unsigned int
        /// value type setting.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t value() const
        {
#ifndef RELEASE
            assert(m_type != TYPELESS);
            assert(m_value != nullptr);
            assert(m_type == UINT);
#endif
            return *static_cast<size_t*>(m_value);
        }

        /////////////////////////////////////////////////
        /// \brief Returns the value of a std::string
        /// value type setting.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        std::string stringval() const
        {
#ifndef RELEASE
            assert(m_type != TYPELESS);
            assert(m_value != nullptr);
            assert(m_type == STRING);
#endif
            return *static_cast<std::string*>(m_value);
        }

};





/////////////////////////////////////////////////
/// \brief This class manages the setting values
/// of the internal (kernel) settings of this
/// application.
/////////////////////////////////////////////////
class Settings : public FileSystem
{
	private:
	    std::string sSettings_ini;

		// Imports of different configuration file versions
		void import_v1x(const std::string& sSettings);
		bool set(const std::string&);

		// Helper
		std::string replaceExePath(const std::string& _sPath);
		void prepareFilePaths(const std::string& _sExePath);

    protected:
        std::map<std::string, SettingsValue> m_settings;

	public:
		Settings();
		Settings(const Settings& _settings);

        void copySettings(const Settings& _settings);

        /////////////////////////////////////////////////
        /// \brief Assignment operator overload.
        ///
        /// \param _settings const Settings&
        /// \return Settings&
        ///
        /////////////////////////////////////////////////
        Settings& operator=(const Settings& _settings)
        {
            copySettings(_settings);
            return *this;
        }

        // loading and saving configuration files
		void save(const std::string& _sWhere, bool bMkBackUp = false);
		void load(const std::string& _sWhere);

        /////////////////////////////////////////////////
        /// \brief Returns a reference to the setting
        /// value, which corresponds to the passed
        /// string. Throws an exception, if the setting
        /// does not exist.
        ///
        /// \param value const std::string&
        /// \return SettingsValue&
        ///
        /////////////////////////////////////////////////
		SettingsValue& getSetting(const std::string& value)
		{
		    auto iter = m_settings.find(value);

		    if (iter != m_settings.end())
                return iter->second;

            throw SyntaxError(SyntaxError::INVALID_SETTING, "", value);
		}

        /////////////////////////////////////////////////
        /// \brief Returns a const reference to the
        /// setting value, which corresponds to the
        /// passed string. Throws an exception, if the
        /// setting does not exist.
        ///
        /// \param value const std::string&
        /// \return const SettingsValue&
        ///
        /////////////////////////////////////////////////
		const SettingsValue& getSetting(const std::string& value) const
		{
		    auto iter = m_settings.find(value);

		    if (iter != m_settings.end())
                return iter->second;

            throw SyntaxError(SyntaxError::INVALID_SETTING, "", value);
		}

        /////////////////////////////////////////////////
        /// \brief Returns a reference to the internal
        /// map of setting values.
        ///
        /// \return std::map<std::string, SettingsValue>&
        ///
        /////////////////////////////////////////////////
		std::map<std::string, SettingsValue>& getSettings()
		{
		    return m_settings;
		}

        /////////////////////////////////////////////////
        /// \brief Returns a const reference to the
        /// internal map of setting values.
        ///
        /// \return const std::map<std::string, SettingsValue>&
        ///
        /////////////////////////////////////////////////
		const std::map<std::string, SettingsValue>& getSettings() const
		{
		    return m_settings;
		}

        /////////////////////////////////////////////////
        /// \brief Enables a setting with boolean value
        /// type. If the setting does not have this type,
        /// nothing happens.
        ///
        /// \param option const std::string&
        /// \return void
        ///
        /////////////////////////////////////////////////
		void enable(const std::string& option)
		{
		    auto iter = m_settings.find(option);

		    if (iter != m_settings.end() && iter->second.getType() == SettingsValue::BOOL)
                iter->second.active() = true;
		}

        /////////////////////////////////////////////////
        /// \brief Disables a setting with boolean value
        /// type. If the setting does not have this type,
        /// nothing happens.
        ///
        /// \param option const std::string&
        /// \return void
        ///
        /////////////////////////////////////////////////
		void disable(const std::string& option)
		{
		    auto iter = m_settings.find(option);

		    if (iter != m_settings.end() && iter->second.getType() == SettingsValue::BOOL)
                iter->second.active() = false;
		}

        /////////////////////////////////////////////////
        /// \brief Returns true, if the setting with
        /// boolean value type is enabled, false
        /// otherwise. If the setting does not have a
        /// boolean value type, this method will return
        /// false as well.
        ///
        /// \param option const std::string&
        /// \return bool
        ///
        /////////////////////////////////////////////////
		bool isEnabled(const std::string& option) const
        {
            auto iter = m_settings.find(option);

		    if (iter != m_settings.end() && iter->second.getType() == SettingsValue::BOOL)
                return iter->second.active();

            return false;
        }

        /////////////////////////////////////////////////
        /// \brief Enables or disables the system
        /// printing functionality. This is a convenience
        /// wrapper for the direct map access.
        ///
        /// \param _bSystemPrints bool
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void enableSystemPrints(bool _bSystemPrints = true)
        {
            m_settings[SETTING_B_SYSTEMPRINTS].active() = _bSystemPrints;
        }

        /////////////////////////////////////////////////
        /// \brief Update the default plotting font. This
        /// member function evaluates first, whether the
        /// selected font actually exists.
        ///
        /// \param plotFont const std::string&
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void setDefaultPlotFont(const std::string& plotFont)
        {
            std::string _sPlotFont = plotFont;

            if (_sPlotFont == "palatino")
                _sPlotFont = "pagella";

            if (_sPlotFont == "times")
                _sPlotFont = "termes";

            if (_sPlotFont == "bookman")
                _sPlotFont = "bonum";

            if (_sPlotFont == "avantgarde")
                _sPlotFont = "adventor";

            if (_sPlotFont == "chancery")
                _sPlotFont = "chorus";

            if (_sPlotFont == "courier")
                _sPlotFont = "cursor";

            if (_sPlotFont == "helvetica")
                _sPlotFont = "heros";

            if (_sPlotFont == "pagella"
                    || _sPlotFont == "adventor"
                    || _sPlotFont == "bonum"
                    || _sPlotFont == "chorus"
                    || _sPlotFont == "cursor"
                    || _sPlotFont == "heros"
                    || _sPlotFont == "heroscn"
                    || _sPlotFont == "schola"
                    || _sPlotFont == "termes"
                )
            {
                m_settings[SETTING_S_PLOTFONT].stringval() = _sPlotFont;
            }
        }

        // Getter wrappers
        //
        /////////////////////////////////////////////////
        /// \brief Returns, whether the developer mode is
        /// currently enabled.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
		inline bool isDeveloperMode() const
            {return m_settings.at(SETTING_B_DEVELOPERMODE).active();}

        /////////////////////////////////////////////////
        /// \brief Returns, whether the plotting draft
        /// mode is enabled.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool isDraftMode() const
            {return m_settings.at(SETTING_B_DRAFTMODE).active();}

        /////////////////////////////////////////////////
        /// \brief Returns, whether tables shall be
        /// displayed in a more compact way (does nothing
        /// in the external table viewer).
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
		inline bool createCompactTables() const
            {return m_settings.at(SETTING_B_COMPACT).active();}

        /////////////////////////////////////////////////
        /// \brief Returns, whether NumeRe shall greet
        /// the user with a funny message at application
        /// start-up.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
		inline bool showGreeting() const
            {return m_settings.at(SETTING_B_GREETING).active();}

        /////////////////////////////////////////////////
        /// \brief Returns, whether the tip-of-the-day
        /// shall be displayed to the user at application
        /// start-up.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool showHints() const
            {return m_settings.at(SETTING_B_SHOWHINTS).active();}

        /////////////////////////////////////////////////
        /// \brief Returns, whether ESC key presses shall
        /// be handled, when scripts are being executed.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool useEscInScripts() const
            {return m_settings.at(SETTING_B_USEESCINSCRIPTS).active();}

        /////////////////////////////////////////////////
        /// \brief Returns, whether NumeRe shall load and
        /// save custom functions automatically.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool controlDefinitions() const
            {return m_settings.at(SETTING_B_DEFCONTROL).active();}

        /////////////////////////////////////////////////
        /// \brief Returns, whether the file tree or the
        /// terminal file explorer shall display extended
        /// file information for NumeRe data files.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool showExtendedFileInfo() const
            {return m_settings.at(SETTING_B_EXTENDEDFILEINFO).active();}

        /////////////////////////////////////////////////
        /// \brief Returns, whether the terminal inputs
        /// shall be protocoled in a dedicated logfile.
        /// This setting does not modify the history
        /// functionality.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool useLogFile() const
            {return m_settings.at(SETTING_B_LOGFILE).active();}

        /////////////////////////////////////////////////
        /// \brief Returns, whether NumeRe shall keep
        /// empty columns when loading a file to memory.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool loadEmptyCols() const
            {return m_settings.at(SETTING_B_LOADEMPTYCOLS).active();}

        /////////////////////////////////////////////////
        /// \brief Returns the precision for displaying
        /// floating numbers in the terminal. This value
        /// determines the number of valid numbers, which
        /// shall be displayed.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
		inline size_t getPrecision() const
            {return m_settings.at(SETTING_V_PRECISION).value();}

        /////////////////////////////////////////////////
        /// \brief Returns the current application root
        /// folder path.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string getExePath() const
            {return m_settings.at(SETTING_S_EXEPATH).stringval();}

        /////////////////////////////////////////////////
        /// \brief Returns the current working path
        /// (connected to the <wp> token).
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string getWorkPath() const
            {return m_settings.at(SETTING_S_WORKPATH).stringval();}

        /////////////////////////////////////////////////
        /// \brief Returns the current saving path.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
		inline std::string getSavePath() const
            {return m_settings.at(SETTING_S_SAVEPATH).stringval();}

        /////////////////////////////////////////////////
        /// \brief Returns the current loading path.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
		inline std::string getLoadPath() const
            {return m_settings.at(SETTING_S_LOADPATH).stringval();}

        /////////////////////////////////////////////////
        /// \brief Returns the current plotting path
        /// (plot storing location).
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
		inline std::string getPlotPath() const
            {return m_settings.at(SETTING_S_PLOTPATH).stringval();}

        /////////////////////////////////////////////////
        /// \brief Returns the current script import
        /// folder path.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
		inline std::string getScriptPath() const
            {return m_settings.at(SETTING_S_SCRIPTPATH).stringval();}

        /////////////////////////////////////////////////
        /// \brief Returns the current procedure root
        /// import path.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string getProcPath() const
            {return m_settings.at(SETTING_S_PROCPATH).stringval();}

        /////////////////////////////////////////////////
        /// \brief Returns the timespan for the autosave
        /// interval in seconds.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
		inline size_t getAutoSaveInterval() const
            {return m_settings.at(SETTING_V_AUTOSAVE).value();}

        /////////////////////////////////////////////////
        /// \brief Returns the current terminal buffer
        /// size.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        inline size_t getBuffer() const
            {return m_settings.at(SETTING_V_BUFFERSIZE).value();}

        /////////////////////////////////////////////////
        /// \brief Returns the current window size of the
        /// terminal.
        ///
        /// \param nWindow int
        /// \return size_t
        /// \note The y value might not be updated if the
        /// user changes the height of the terminal.
        ///
        /////////////////////////////////////////////////
        inline size_t getWindow(int nWindow = 0) const
            {return nWindow ? m_settings.at(SETTING_V_WINDOW_Y).value()-1 : m_settings.at(SETTING_V_WINDOW_X).value()-1;}

        /////////////////////////////////////////////////
        /// \brief Returns the current plotting font
        /// name.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string getDefaultPlotFont() const
            {return m_settings.at(SETTING_S_PLOTFONT).stringval();}

        /////////////////////////////////////////////////
        /// \brief Returns a semicolon-separated list of
        /// the current defined path placeholders and
        /// their values.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string getTokenPaths() const
        {
            return "<>="+m_settings.at(SETTING_S_EXEPATH).stringval()
                +";<wp>="+m_settings.at(SETTING_S_WORKPATH).stringval()
                +";<savepath>="+m_settings.at(SETTING_S_SAVEPATH).stringval()
                +";<loadpath>="+m_settings.at(SETTING_S_LOADPATH).stringval()
                +";<plotpath>="+m_settings.at(SETTING_S_PLOTPATH).stringval()
                +";<scriptpath>="+m_settings.at(SETTING_S_SCRIPTPATH).stringval()
                +";<procpath>="+m_settings.at(SETTING_S_PROCPATH).stringval()+";";
        }

        /////////////////////////////////////////////////
        /// \brief Returns, whether system messages shall
        /// be printed to the terminal.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool systemPrints() const
            {return m_settings.at(SETTING_B_SYSTEMPRINTS).active();}

        /////////////////////////////////////////////////
        /// \brief Returns, whether the debugger is
        /// currently active.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool useDebugger() const
            {return m_settings.at(SETTING_B_DEBUGGER).active();}

        /////////////////////////////////////////////////
        /// \brief Returns, whether user language files
        /// shall be used to override internal language
        /// strings.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool useCustomLangFiles() const
            {return m_settings.at(SETTING_B_USECUSTOMLANG).active();}

        /////////////////////////////////////////////////
        /// \brief Returns, whether documentations shall
        /// be displayed in external windows.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool useExternalDocWindow() const
            {return m_settings.at(SETTING_B_EXTERNALDOCWINDOW).active();}

        /////////////////////////////////////////////////
        /// \brief Returns, whether the user enabled the
        /// \c execute command.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool executeEnabled() const
            {return m_settings.at(SETTING_B_ENABLEEXECUTE).active();}

        /////////////////////////////////////////////////
        /// \brief Returns, whether loop flow control
        /// statements shall use the \c mask option
        /// automatically.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool useMaskDefault() const
            {return m_settings.at(SETTING_B_MASKDEFAULT).active();}

        /////////////////////////////////////////////////
        /// \brief Returns, whether the debugger shall
        /// try to decode procedure arguments.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        inline bool decodeArguments() const
            {return m_settings.at(SETTING_B_DECODEARGUMENTS).active();}

};


#endif


