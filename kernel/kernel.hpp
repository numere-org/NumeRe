/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

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



#include <string>
#include <boost/nowide/fstream.hpp>
#include <queue>

// --> LOKALE HEADER <--
#include "windowmanager.hpp"

#include "core/built-in.hpp"
#include "core/plugins.hpp"
#include "core/script.hpp"
#include "core/settings.hpp"
#include "core/documentation/doc_helper.hpp"

#include "core/datamanagement/container.hpp"
#include "core/datamanagement/memorymanager.hpp"

#include "core/debugger/debugger.hpp"

#include "core/io/output.hpp"

#include "core/maths/define.hpp"
#include "core/maths/parser_functions.hpp"

#include "core/ParserLib/muParser.h"

#include "core/plotting/graph_helper.hpp"
#include "core/plotting/plotdata.hpp"

#include "core/procedure/procedure.hpp"
#include "core/procedure/procedurelibrary.hpp"

#include "core/ui/error.hpp"
#include "core/ui/language.hpp"

#include "core/utils/tools.hpp"

#include "core/structures.hpp"


#ifndef KERNEL_HPP
#define KERNEL_HPP

// Forward declarations of the terminal class and
// the task container used for communicating between
// the kernel and the GUI
class NumeReTerminal;
struct NumeReTask;


/////////////////////////////////////////////////
/// \brief This class provides the interface to
/// the core of NumeRe. It provides all
/// functionalities, which was done by the main
/// function before and handles the communication
/// to the graphical user interface.
/////////////////////////////////////////////////
class NumeReKernel
{
    public:
        // return values indicating status
        // pre-declared for the private member functions
        enum KernelStatus
        {
            NUMERE_QUIT = -1,
            NUMERE_ERROR = 0,
            NUMERE_DONE = 1,
            NUMERE_DONE_KEYWORD,
            NUMERE_PRINTLINE,
            NUMERE_PRINTLINE_PREFMT,
            NUMERE_ISSUE_WARNING,
            NUMERE_PENDING,
            NUMERE_PENDING_SPECIAL,
            NUMERE_CALC_UPDATE,
            NUMERE_STATUSBAR_UPDATE,
            NUMERE_QUEUED_COMMAND,
            NUMERE_EDIT_FILE,
            NUMERE_OPEN_DOC,
            NUMERE_SHOW_TABLE,
            NUMERE_EDIT_TABLE,
            NUMERE_SHOW_STRING_TABLE,
            NUMERE_DEBUG_EVENT,
            NUMERE_ANSWER_READ,
            NUMERE_SHOW_WINDOW,
            NUMERE_REFRESH_FUNCTIONTREE,
            NUMERE_CLOSE_WINDOWS,
            NUMERE_INSTALLATION_DONE,
            NUMERE_CLC_TERMINAL
        };

        enum DebuggerCodes
        {
            DEBUGGER_CONTINUE = 1,
            DEBUGGER_STEP,
            DEBUGGER_STEPOVER,
            DEBUGGER_LEAVE
        };

    private:
        std::string sCommandLine;
        std::string sAnswer;
        std::string sPlotCompose;
        std::queue<std::string> commandQueue;
        bool refreshTree;

        static NumeReKernel* kernelInstance;

        //datasets
        FileSystem _fSys;
        Settings _option;
        Output _out;
        MemoryManager _memoryManager;
        mu::Parser _parser;
        //NumeRe::StringParser _stringParser;
        FunctionDefinitionManager _functions;
        PlotData _pData;
        Script _script;
        Procedure _procedure;
        NumeReDebugger _debugger;
        NumeRe::WindowManager _manager;
        Documentation _documentation;

        // private member functions for special tasks
        void printResult(const std::string& sLine, bool bScriptRunning);
        std::string maskProcedureSigns(std::string sLine);
        bool handleCommandLineSource(std::string& sLine, std::string& sKeep);
        bool getLineFromCommandCache(std::string& sLine, const std::string& sCurrentCommand);
        bool handleComposeBlock(std::string& sLine, const std::string& sCurrentCommand, KernelStatus& nReturnVal);
        bool handleProcedureWrite(const std::string& sLine, const std::string& sCurrentCommand, KernelStatus& nReturnVal);
        bool uninstallPlugin(const std::string& sLine, const std::string& sCurrentCommand);
        void handleToCmd(std::string& sLine, std::string& sCache, std::string& sCurrentCommand);
        bool evaluateProcedureCalls(std::string& sLine);
        bool executePlugins(std::string& sLine);
        bool handleFlowControls(std::string& sLine, const std::string& sCurrentCommand, KernelStatus& nReturnVal);
        void createCalculationAnswer(int nNum, const mu::StackItem* v);
        void printErrorMessage(const std::string& errMsg, const std::string& errDesc, std::string expr, size_t pos);
        void resetAfterError();
        std::string getGreeting();
        void checkInternalStates();

        // Functions for initializing the numerical parser
        void defineOperators();
        void defineConst();
        void defineNumFunctions();
        void defineMatFunctions();
        void defineStrFunctions();

    public:
        // Static public class members
        // Used for controlling the programm flow, to update the internal
        // state and to communicate with the graphical layer
        static int* baseStackPosition;
        static NumeReTerminal* m_parent;
        static std::queue<NumeReTask> taskQueue;
        static int nLINE_LENGTH;
        static bool bWritingTable;
        static bool bCancelSignal;
        static int nOpenFileFlag;
        static int nLastStatusVal;
        static size_t nLastLineLength;
        static bool modifiedSettings;
        static NumeRe::Table table;
        static bool bSupressAnswer;
        static bool bGettingLine;
        static bool bErrorNotification;
        static ProcedureLibrary ProcLibrary;

        // Constructor and Destructor
        NumeReKernel();
        ~NumeReKernel();

        // Static member functions
        // can be called from everywhere without an explicit instance of the kernel
        // These functions are the interface for communication with the GUI
        /////////////////////////////////////////////////
        /// \brief This static member function returns a
        /// a pointer to the singleton instance of the
        /// kernel.
        ///
        /// \return NumeReKernel*
        ///
        /// Obtaining the pointer to the kernel allows
        /// getting references to the main class objects.
        /////////////////////////////////////////////////
        static NumeReKernel* getInstance()
        {
            return kernelInstance;
        }

        static void toggleTableStatus();
        static void flush();
        static void print(const std::string& __sLine, bool printingEnabled = true);
        static void printPreFmt(const std::string& __sLine, bool printingEnabled = true);
        static std::string formatResultOutput(int nNum, const mu::StackItem* v);
        static void issueWarning(std::string sWarningMessage);
        static void failMessage(std::string sFailMessage);
        static int numberOfNumbersPerLine();
        static void progressBar(int nStep, int nFirstStep, int nFinalStep, const std::string& sType);
        static void getline(std::string& sLine);
        static void gotoLine(const std::string& sFile, size_t nLine = 0);
        static void setDocumentation(const std::string& _sDocumentation);
        static void installationDone();
        static bool GetAsyncCancelState();
        static void showTable(NumeRe::Table _table, std::string __name, bool openeditable = false);
        static void showStringTable(NumeRe::Container<std::string> _stringtable, std::string __name, bool openeditable = false);
        void showWindow(const NumeRe::Window& window);
        static NumeRe::Table getTable();
        NumeRe::Table getTable(const std::string& sTableName);
        NumeRe::Container<std::string> getStringTable(const std::string& sStringTableName);
        static void showDebugEvent(const std::string& sTitle, const std::vector<std::string>& vStacktrace);
        static int waitForContinue();
        static int evalDebuggerBreakPoint(const std::string& sCurrentCommand = "");
        std::pair<std::string, size_t> getErrorLocation();
        static void clcTerminal();
        void refreshFunctionTree();
        void closeWindows(int type);

        // Public member functions
        // Main loop function
        KernelStatus MainLoop(const std::string& sCommand);

        FileSystem& getFileSystem()
        {
            return _fSys;
        }

        MemoryManager& getMemoryManager()
        {
            return _memoryManager;
        }

        mu::Parser& getParser()
        {
            return _parser;
        }

        FunctionDefinitionManager& getDefinitions()
        {
            return _functions;
        }

        Settings& getSettings()
        {
            return _option;
        }

        PlotData& getPlottingData()
        {
            return _pData;
        }

        Output& getOutput()
        {
            return _out;
        }

        Script& getScript()
        {
            return _script;
        }

        Procedure& getProcedureInterpreter()
        {
            return _procedure;
        }

        ProcedureLibrary& getProcedureLibrary()
        {
            return ProcLibrary;
        }

        NumeReDebugger& getDebugger()
        {
            return _debugger;
        }

        NumeRe::WindowManager& getWindowManager()
        {
            return _manager;
        }

        Documentation& getDocumentation()
        {
            return _documentation;
        }

        void displaySplash();
        std::map<std::string,std::string> getPluginLanguageStrings();
        std::map<std::string,std::string> getFunctionLanguageStrings();
        std::vector<std::string> getPluginCommands();
        int ReadOpenFileFlag();
        std::string ReadAnswer();
        NumeReVariables getVariableList();
        NumeReVariables getVariableListForAutocompletion();
        bool SettingsModified();
        int getAutosaveInterval() const
		{
			return _option.getAutoSaveInterval();
		}
        long long int getLastSavedTime() const
		{
			return _memoryManager.getLastSaved();
		}
        void Autosave();
        void StartUp(NumeReTerminal* _parent, const std::string& __sPath, const std::string& sPredefinedFunctions);
        void CloseSession();
        void CancelCalculation()
        {
            bCancelSignal = true;
        }
        void saveData();
        Settings getKernelSettings();
        void setKernelSettings(const Settings& _settings);
        void synchronizePathSettings();
        std::vector<std::string> getPathSettings() const;
        const std::vector<Package>& getInstalledPackages() const;
        std::map<std::string, std::string> getMenuMap() const;
        void initializeStackTracker();
        void printVersionInfo(bool shortInfo = false);
        void updateLineLenght(int nLength);
        void sendErrorNotification();
        void ensureMainWindowVisible();
};


/////////////////////////////////////////////////
/// \brief This structure abstracts the necessary
/// information for a task to be done by the
/// graphical user interface.
/////////////////////////////////////////////////
struct NumeReTask
{
    std::string sString;
    size_t nLine;
    std::vector<std::string> vDebugEvent;
    NumeRe::Table table;
    NumeRe::Window window;
    NumeRe::Container<std::string> stringTable;
    int taskType;

    NumeReTask() : sString(), nLine(0), vDebugEvent(), table(), window(), stringTable(), taskType(0) {}
};


/////////////////////////////////////////////////
/// \brief This function provides a headline for
/// the "windows" in the console.
///
/// \param sString const std::string&
/// \param cHeadLineSep char
/// \return std::string
///
/////////////////////////////////////////////////
inline std::string sectionHeadline(const std::string& sString, char cHeadLineSep = '-')
{
    std::string sSectionHeadline = "|\n|   " + toUpperCase(sString);

    if (sSectionHeadline.back() != ':')
        sSectionHeadline += ": ";
    else
        sSectionHeadline += " ";

    sSectionHeadline.append(NumeReKernel::nLINE_LENGTH-sSectionHeadline.length()+1, cHeadLineSep);
    sSectionHeadline += "\n";
    return sSectionHeadline;
}



#endif // KERNEL_HPP

