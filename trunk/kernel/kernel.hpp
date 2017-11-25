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
#include "core/output.hpp"
#include "core/datamanagement/datafile.hpp"
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
#include "core/procedure/loop.hpp"
#include "core/procedure/procedure.hpp"
#include "core/plugin.hpp"
#include "core/ui/language.hpp"
#include "debugmessenger.hpp"
#include "core/procedure/procedurelibrary.hpp"
// --> PARSER-HEADER <--
#include "core/ParserLib/muParser.h"

using namespace std;
using namespace mu;

class wxTerm;

typedef std::vector<std::vector<std::string> > stringmatrix;

class NumeReKernel
{
    private:
        string sCommandLine;
        string sAnswer;
        string sPlotCompose;

        //datasets
        Settings _option;
        Output _out;
        Datafile _data;
        Parser _parser;
        Define _functions;
        PlotData _pData;
        Script _script;
        Procedure _procedure;
        //mglGraph _graph;

        void printResult(const string& sLine, const string& sCmdCache, bool bScriptRunning);
        string maskProcedureSigns(string sLine);

        void defineOperators();
        void defineConst();
        void defineFunctions();

    public:
        static wxTerm* m_parent;
        static queue<GraphHelper*> graphHelper;
        static int nLINE_LENGTH;
        static bool bWritingTable;
        static bool bCancelSignal;
        static string sFileToEdit;
        static string sDocumentation;
        static unsigned int nLineToGoTo;
        static int nOpenFileFlag;
        static int nLastStatusVal;
        static unsigned int nLastLineLength;
        static bool modifiedSettings;
        static queue<stringmatrix> sTable;
        static vector<string> vDebugInfos;
        static queue<string> sTableName;
        static Debugmessenger _messenger;
        static bool bSupressAnswer;
        static bool bGettingLine;
        static bool bErrorNotification;
        ofstream oLogFile;
        static size_t nScriptLine;
        static string sScriptFileName;
        static ProcedureLibrary ProcLibrary;
        // return values indicating status:
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
            NUMERE_EDIT_FILE,
            NUMERE_OPEN_DOC,
            NUMERE_SHOW_TABLE,
            NUMERE_EDIT_TABLE,
            NUMERE_DEBUG_EVENT,
            NUMERE_ANSWER_READ,
            NUMERE_GRAPH_UPDATE
        };

        NumeReKernel();
        ~NumeReKernel();

        static void toggleTableStatus();
        static void flush();

        map<string,string> getPluginLanguageStrings();
        map<string,string> getFunctionLanguageStrings();
        vector<string> getPluginCommands();
        string ReadFileName();
        unsigned int ReadLineNumber();
        int ReadOpenFileFlag();
        string ReadAnswer();
        string ReadDoc();
        string getDocumentation(const string& sCommand);
        bool SettingsModified();
        int getAutosaveInterval() {return _option.getAutoSaveInterval();}
        long long int getLastSavedTime() {return _data.getLastSaved();}
        void Autosave();

        void StartUp(wxTerm* _parent, const string& sPath, const string& sPredefinedFuncs);
        KernelStatus MainLoop(const string& sCommand);
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

        static void setFileName(const string& sFileName);
        static void print(const string& sLine);
        static void printPreFmt(const string& sLine);
        void sendErrorNotification();
        static void statusBar(int nStep, int nFirstStep, int nFinalStep, const string& sType);
        static void getline(string& sLine);
        static void gotoLine(const string& sFile, unsigned int nLine = 0);
        static void setDocumentation(const string& _sDocumentation);
        static bool GetAsyncCancelState();
        static void showTable(string** __stable, size_t cols, size_t lines, string __name, bool openeditable = false);
        static void updateGraphWindow(GraphHelper* _helper);
        static stringmatrix getTable();
        static void showDebugEvent(const string& sTitle, const vector<string>& vModule, const vector<string>& vStacktrace, const vector<string>& vNumVars, const vector<string>& vStringVars);
        static void waitForContinue();
        static void evalDebuggerBreakPoint(Settings& _option, const map<string,string>& sStringMap, const string& sCurrentCommand = "", Parser* _parser = nullptr);
};



inline string strfill(const string& sString, unsigned int nWidth, char cFill = ' ')
{
    if (!nWidth)
        return "";
    string sReturn = sString;
    if (sString.length() < nWidth)
        sReturn.insert(0, nWidth-sReturn.length(), cFill);
    return sReturn;
}

inline string strlfill(const string& sString, unsigned int nWidth, char cFill = ' ')
{
    if (!nWidth)
        return "";
    string sReturn = sString;
    if (sString.length() < nWidth)
        sReturn.append(nWidth-sReturn.length(), cFill);
    return sReturn;
}

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

inline string pointToError(unsigned int nPos)
{
    string sErrorPointer = "|   ";
    sErrorPointer += strfill("^^^", nPos+13) + "\n";
    return sErrorPointer;
}
