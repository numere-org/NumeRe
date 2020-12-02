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
#include <fstream>
#include <queue>

// --> LOKALE HEADER <--
#include "windowmanager.hpp"

#include "core/built-in.hpp"
#include "core/plugins.hpp"
#include "core/script.hpp"
#include "core/settings.hpp"
#include "core/version.h"

#include "core/datamanagement/container.hpp"
#include "core/datamanagement/memorymanager.hpp"

#include "core/debugger/debugger.hpp"

#include "core/io/output.hpp"

#include "core/maths/define.hpp"
#include "core/maths/functionimplementation.hpp"
#include "core/maths/parser_functions.hpp"

#include "core/ParserLib/muParser.h"

#include "core/plotting/graph_helper.hpp"
#include "core/plotting/plotdata.hpp"

#include "core/procedure/procedure.hpp"
#include "core/procedure/procedurelibrary.hpp"

#include "core/strings/stringparser.hpp"

#include "core/ui/error.hpp"
#include "core/ui/language.hpp"

#include "core/utils/tools.hpp"


#ifndef KERNEL_HPP
#define KERNEL_HPP


using namespace std;
using namespace mu;

// Forward declarations of the terminal class and
// the task container used for communicating between
// the kernel and the GUI
class NumeReTerminal;
struct NumeReTask;


/////////////////////////////////////////////////
/// \brief This structure combines a vector of
/// declared variables including their values and
/// respective sizes with a set of variable type
/// counts, which can be used to separate the
/// single vector in multiple vectors containing
/// only a single type of variables.
/////////////////////////////////////////////////
struct NumeReVariables
{
    vector<string> vVariables;
    size_t nNumerics;
    size_t nStrings;
    size_t nTables;
    size_t nClusters;
};


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
            NUMERE_CLOSE_WINDOWS
        };

        enum DebuggerCodes
        {
            DEBUGGER_CONTINUE = 1,
            DEBUGGER_STEP,
            DEBUGGER_STEPOVER,
            DEBUGGER_LEAVE
        };

    private:
        string sCommandLine;
        string sAnswer;
        string sPlotCompose;

        static NumeReKernel* kernelInstance;

        //datasets
        FileSystem _fSys;
        Settings _option;
        Output _out;
        MemoryManager _memoryManager;
        Parser _parser;
        NumeRe::StringParser _stringParser;
        FunctionDefinitionManager _functions;
        PlotData _pData;
        Script _script;
        Procedure _procedure;
        NumeReDebugger _debugger;
        NumeRe::WindowManager _manager;

        // private member functions for special tasks
        void printResult(const string& sLine, const string& sCmdCache, bool bScriptRunning);
        string maskProcedureSigns(string sLine);
        bool handleCommandLineSource(string& sLine, const string& sCmdCache, string& sKeep);
        bool getLineFromCommandCache(string& sLine, string& sCmdCache, const string& sCurrentCommand);
        bool handleComposeBlock(string& sLine, const string& sCmdCache, const string& sCurrentCommand, KernelStatus& nReturnVal);
        bool handleProcedureWrite(const string& sLine, const string& sCmdCache, const string& sCurrentCommand, KernelStatus& nReturnVal);
        bool uninstallPlugin(const string& sLine, const string& sCurrentCommand);
        void handleToCmd(string& sLine, string& sCache, string& sCurrentCommand);
        bool evaluateProcedureCalls(string& sLine);
        bool executePlugins(string& sLine);
        bool handleFlowControls(string& sLine, const string& sCmdCache, const string& sCurrentCommand, KernelStatus& nReturnVal);
        bool evaluateStrings(string& sLine, string& sCache, const string& sCmdCache, bool& bWriteToCache, KernelStatus& nReturnVal);
        void createCalculationAnswer(int nNum, value_type* v, const string& sCmdCache);
        void resetAfterError(string& sCmdCache);
        string getGreeting();

        // Functions for initializing the numerical parser
        void defineOperators();
        void defineConst();
        void defineFunctions();

    public:
        // Static public class members
        // Used for controlling the programm flow, to update the internal
        // state and to communicate with the graphical layer
        static int* baseStackPosition;
        static NumeReTerminal* m_parent;
        static queue<NumeReTask> taskQueue;
        static int nLINE_LENGTH;
        static bool bWritingTable;
        static bool bCancelSignal;
        static int nOpenFileFlag;
        static int nLastStatusVal;
        static unsigned int nLastLineLength;
        static bool modifiedSettings;
        static NumeRe::Table table;
        static bool bSupressAnswer;
        static bool bGettingLine;
        static bool bErrorNotification;
        static ofstream oLogFile;
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
        static void print(const string& __sLine, bool printingEnabled = true);
        static void printPreFmt(const string& __sLine, bool printingEnabled = true);
        static string formatResultOutput(int nNum, value_type* v);
        static string formatResultOutput(const vector<string>& vStringResults);
        static void issueWarning(string sWarningMessage);
        static int numberOfNumbersPerLine();
        static void progressBar(int nStep, int nFirstStep, int nFinalStep, const string& sType);
        static void getline(string& sLine);
        static void gotoLine(const string& sFile, unsigned int nLine = 0);
        static void setDocumentation(const string& _sDocumentation);
        static bool GetAsyncCancelState();
        static void showTable(NumeRe::Table _table, string __name, bool openeditable = false);
        static void showStringTable(NumeRe::Container<string> _stringtable, string __name, bool openeditable = false);
        void showWindow(const NumeRe::Window& window);
        static NumeRe::Table getTable();
        NumeRe::Table getTable(const string& sTableName);
        NumeRe::Container<string> getStringTable(const string& sStringTableName);
        static void showDebugEvent(const string& sTitle, const vector<string>& vStacktrace);
        static int waitForContinue();
        static int evalDebuggerBreakPoint(const string& sCurrentCommand = "");
        static void addToLog(const string& sLogMessage);
        void refreshFunctionTree();
        void closeWindows(int type);

        // Public member functions
        // Main loop function
        KernelStatus MainLoop(const string& sCommand);

        FileSystem& getFileSystem()
        {
            return _fSys;
        }

        MemoryManager& getMemoryManager()
        {
            return _memoryManager;
        }

        NumeRe::Cluster& getAns()
        {
            return _memoryManager.getCluster("ans");
        }

        Parser& getParser()
        {
            return _parser;
        }

        NumeRe::StringParser& getStringParser()
        {
            return _stringParser;
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

        void displaySplash();
        map<string,string> getPluginLanguageStrings();
        map<string,string> getFunctionLanguageStrings();
        vector<string> getPluginCommands();
        int ReadOpenFileFlag();
        string ReadAnswer();
        string getDocumentation(const string& sCommand);
        vector<string> getDocIndex();
        NumeReVariables getVariableList();
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
        void StartUp(NumeReTerminal* _parent, const string& __sPath, const string& sPredefinedFunctions);
        void CloseSession();
        void CancelCalculation()
        {
            bCancelSignal = true;
        }
        void saveData();
        Settings getKernelSettings();
        void setKernelSettings(const Settings& _settings);
        vector<string> getPathSettings() const;
        void printVersionInfo();
        void updateLineLenght(int nLength);
        void sendErrorNotification();
};


/////////////////////////////////////////////////
/// \brief This structure abstracts the necessary
/// information for a task to be done by the
/// graphical user interface.
/////////////////////////////////////////////////
struct NumeReTask
{
    string sString;
    size_t nLine;
    vector<string> vDebugEvent;
    NumeRe::Table table;
    NumeRe::Window window;
    NumeRe::Container<string> stringTable;
    int taskType;

    NumeReTask() : sString(), nLine(0), vDebugEvent(), table(), window(), stringTable(), taskType(0) {}
};


/////////////////////////////////////////////////
/// \brief This function fills the passed string
/// up to the width nWidth with the characters
/// cFill. The characters are inserted on the
/// right.
///
/// \param sString const string&
/// \param nWidth unsigned int
/// \param cFill char
/// \return string
///
/////////////////////////////////////////////////
inline string strfill(const string& sString, unsigned int nWidth, char cFill = ' ')
{
    if (!nWidth)
        return "";
    string sReturn = sString;
    if (sString.length() < nWidth)
        sReturn.insert(0, nWidth-sReturn.length(), cFill);
    return sReturn;
}


/////////////////////////////////////////////////
/// \brief This function fills the passed string
/// up to the width nWidth with the characters
/// cFill. The characters are inserted on the
/// left.
///
/// \param sString const string&
/// \param nWidth unsigned int
/// \param cFill char
/// \return string
///
/////////////////////////////////////////////////
inline string strlfill(const string& sString, unsigned int nWidth, char cFill = ' ')
{
    if (!nWidth)
        return "";
    string sReturn = sString;
    if (sString.length() < nWidth)
        sReturn.append(nWidth-sReturn.length(), cFill);
    return sReturn;
}


/////////////////////////////////////////////////
/// \brief This function provides a headline for
/// the "windows" in the console.
///
/// \param sString const string&
/// \param cHeadLineSep char
/// \return string
///
/////////////////////////////////////////////////
inline string sectionHeadline(const string& sString, char cHeadLineSep = '-')
{
    string sSectionHeadline = "|\n|   " + toUpperCase(sString);
    if (sSectionHeadline.back() != ':')
        sSectionHeadline += ": ";
    else
        sSectionHeadline += " ";
    sSectionHeadline.append(NumeReKernel::nLINE_LENGTH-sSectionHeadline.length()+1, cHeadLineSep);
    sSectionHeadline += "\n";
    return sSectionHeadline;
}


/////////////////////////////////////////////////
/// \brief This function points to the error
/// indicated by nPos. It draws three circumflexes
/// below the error location.
///
/// \param nPos unsigned int
/// \return string
///
/////////////////////////////////////////////////
inline string pointToError(unsigned int nPos)
{
    string sErrorPointer = "|   ";
    sErrorPointer += strfill("^^^", nPos+13) + "\n";
    return sErrorPointer;
}


#endif // KERNEL_HPP

