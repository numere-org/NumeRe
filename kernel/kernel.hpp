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
#include "core/ui/error.hpp"
#include "core/settings.hpp"
#include "core/io/output.hpp"
#include "core/datamanagement/datafile.hpp"
#include "core/datamanagement/container.hpp"
#include "core/plugins.hpp"
#include "core/version.h"
#include "core/maths/functionimplementation.hpp"
#include "core/utils/tools.hpp"
#include "core/built-in.hpp"
#include "core/maths/parser_functions.hpp"
#include "core/maths/define.hpp"
#include "core/plotting/plotdata.hpp"
#include "core/plotting/graph_helper.hpp"
#include "core/script.hpp"
#include "core/procedure/flowctrl.hpp"
#include "core/procedure/procedure.hpp"
#include "core/plugin.hpp"
#include "core/ui/language.hpp"
#include "debugmessenger.hpp"
#include "core/procedure/procedurelibrary.hpp"
// --> PARSER-HEADER <--
#include "core/ParserLib/muParser.h"

using namespace std;
using namespace mu;

// Forward declarations of the terminal class and
// the task container used for communicating between
// the kernel and the GUI
class wxTerm;
struct NumeReTask;


struct NumeReVariables
{
    vector<string> vVariables;
    size_t nNumerics;
    size_t nStrings;
    size_t nTables;
};


// This class provides the interface to the kernel of NumeRe
// It provides all functionalities, which was done by the main
// function before.
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
            NUMERE_PENDING,
            NUMERE_PENDING_SPECIAL,
            NUMERE_CALC_UPDATE,
            NUMERE_STATUSBAR_UPDATE,
            NUMERE_QUEUED_COMMAND,
            NUMERE_EDIT_FILE,
            NUMERE_OPEN_DOC,
            NUMERE_SHOW_TABLE,
            NUMERE_EDIT_TABLE,
            NUMERE_DEBUG_EVENT,
            NUMERE_ANSWER_READ,
            NUMERE_GRAPH_UPDATE
        };

    private:
        string sCommandLine;
        string sAnswer;
        string sPlotCompose;

        static NumeReKernel* kernelInstance;

        //datasets
        Settings _option;
        Output _out;
        Datafile _data;
        Parser _parser;
        Define _functions;
        PlotData _pData;
        Script _script;
        Procedure _procedure;

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

        // Functions for initializing the numerical parser
        void defineOperators();
        void defineConst();
        void defineFunctions();

    public:
        // Static public class members
        // Used for controlling the programm flow, to update the internal
        // state and to communicate with the graphical layer
        static int* baseStackPosition;
        static wxTerm* m_parent;
        static queue<NumeReTask> taskQueue;
        static int nLINE_LENGTH;
        static bool bWritingTable;
        static bool bCancelSignal;
        static int nOpenFileFlag;
        static int nLastStatusVal;
        static unsigned int nLastLineLength;
        static bool modifiedSettings;
        static NumeRe::Table table;
        static Debugmessenger _messenger;
        static bool bSupressAnswer;
        static bool bGettingLine;
        static bool bErrorNotification;
        static ofstream oLogFile;
        static size_t nScriptLine;
        static string sScriptFileName;
        static ProcedureLibrary ProcLibrary;

        // Constructor and Destructor
        NumeReKernel();
        ~NumeReKernel();

        // Static member functions
        // can be called from everywhere without an explicit instance of the kernel
        // These functions are the interface for communication with the GUI
        static NumeReKernel* getInstance()
        {
            return kernelInstance;
        }

        static void toggleTableStatus();
        static void flush();
        static void print(const string& sLine);
        static void printPreFmt(const string& sLine);
        static string formatResultOutput(int nNum, value_type* v, const Settings& _option);
        static void setFileName(const string& sFileName);
        static int numberOfNumbersPerLine(const Settings& _option);
        static void statusBar(int nStep, int nFirstStep, int nFinalStep, const string& sType);
        static void getline(string& sLine);
        static void gotoLine(const string& sFile, unsigned int nLine = 0);
        static void setDocumentation(const string& _sDocumentation);
        static bool GetAsyncCancelState();
        static void showTable(NumeRe::Table _table, string __name, bool openeditable = false);
        static void updateGraphWindow(GraphHelper* _helper);
        static NumeRe::Table getTable();
        NumeRe::Table getTable(const string& sTableName);
        static void showDebugEvent(const string& sTitle, const vector<string>& vModule, const vector<string>& vStacktrace, const vector<string>& vNumVars, const vector<string>& vStringVars, const vector<string>& vTables);
        static void waitForContinue();
        static void evalDebuggerBreakPoint(Settings& _option, const string& sCurrentCommand = "", Parser* _parser = nullptr);
        static void addToLog(const string& sLogMessage);

        // Public member functions
        // Main loop function
        KernelStatus MainLoop(const string& sCommand);

        Datafile& getData()
        {
            return _data;
        }

        Parser& getParser()
        {
            return _parser;
        }

        Define& getDefinitions()
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

        map<string,string> getPluginLanguageStrings();
        map<string,string> getFunctionLanguageStrings();
        vector<string> getPluginCommands();
        string ReadFileName();
        unsigned int ReadLineNumber();
        int ReadOpenFileFlag();
        string ReadAnswer();
        string ReadDoc();
        string getDocumentation(const string& sCommand);
        vector<string> getDocIndex();;
        NumeReVariables getVariableList();
        bool SettingsModified();
        int getAutosaveInterval() {return _option.getAutoSaveInterval();}
        long long int getLastSavedTime() {return _data.getLastSaved();}
        void Autosave();
        void StartUp(wxTerm* _parent, const string& sPath, const string& sPredefinedFuncs);
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
        void showDebugError(const string& sTitle);
        void updateLineLenght(int nLength);
        void sendErrorNotification();
};


// Definition of the task container
struct NumeReTask
{
    string sString;
    size_t nLine;
    vector<string> vDebugEvent;
    NumeRe::Table table;
    GraphHelper* graph;
    int taskType;
};

// This function fills the passed string up to the width nWidth with the characters cFill
// The characters are inserted on the right
inline string strfill(const string& sString, unsigned int nWidth, char cFill = ' ')
{
    if (!nWidth)
        return "";
    string sReturn = sString;
    if (sString.length() < nWidth)
        sReturn.insert(0, nWidth-sReturn.length(), cFill);
    return sReturn;
}

// This function fills the passed string up to the width nWidth with the characters cFill
// The characters are inserted on the left
inline string strlfill(const string& sString, unsigned int nWidth, char cFill = ' ')
{
    if (!nWidth)
        return "";
    string sReturn = sString;
    if (sString.length() < nWidth)
        sReturn.append(nWidth-sReturn.length(), cFill);
    return sReturn;
}

// This function provides a headline for the "windows" in the console
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

// This function points to the error indicated by nPos
inline string pointToError(unsigned int nPos)
{
    string sErrorPointer = "|   ";
    sErrorPointer += strfill("^^^", nPos+13) + "\n";
    return sErrorPointer;
}

