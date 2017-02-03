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

// --> LOKALE HEADER <--
#include "core/error.hpp"
#include "core/settings.hpp"
#include "core/output.hpp"
#include "core/datafile.hpp"
#include "core/plugins.hpp"
#include "core/version.h"
#include "core/parser.hpp"
#include "core/tools.hpp"
#include "core/built-in.hpp"
#include "core/parser_functions.hpp"
#include "core/define.hpp"
#include "core/plotdata.hpp"
//#include "menues.hpp"
#include "core/script.hpp"
#include "core/loop.hpp"
#include "core/procedure.hpp"
#include "core/plugin.hpp"
#include "core/language.hpp"
#include "debugmessenger.hpp"
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
    public:
        static wxTerm* m_parent;
        static int nLINE_LENGTH;
        static bool bWritingTable;
        static bool bCancelSignal;
        static string sFileToEdit;
        static string sDocumentation;
        static unsigned int nLineToGoTo;
        static int nLastStatusVal;
        static unsigned int nLastLineLength;
        static bool modifiedSettings;
        static stringmatrix sTable;
        static string sTableName;
        static Debugmessenger _messenger;
        bool bSupressAnswer;
        ofstream oLogFile;
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
            NUMERE_ANSWER_READ
        };

        NumeReKernel();
        ~NumeReKernel();

        static void toggleTableStatus();
        static void flush();

        string ReadFileName();
        unsigned int ReadLineNumber();
        string ReadAnswer();
        string ReadDoc();
        bool SettingsModified();

        void StartUp(wxTerm* _parent);
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

        void updateLineLenght(int nLength);

        static void setFileName(const string& sFileName);
        static void print(const string& sLine);
        static void printPreFmt(const string& sLine);
        static void statusBar(int nStep, int nFirstStep, int nFinalStep, const string& sType);
        static void getline(string& sLine);
        static void gotoLine(const string& sFile, unsigned int nLine = 0);
        static void setDocumentation(const string& _sDocumentation);
        static bool GetAsyncCancelState();
        static void showTable(string** __stable, size_t cols, size_t lines, string __name);
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

// cerr << "|" << endl << "|   " << toUpperCase(_lang.get("DBG_MODULE")) << ": " << std::setfill((char)196) << std::setw(_option.getWindow()-6-_lang.get("DBG_MODULE").length()) << (char)196 << endl;
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

//cerr << "|  " << (char)192 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)196 << (char)193 << (char)196 << (char)196 << std::setfill((char)196) << std::setw(nErrorPos+1) << (char)217 << endl;
inline string pointToError(unsigned int nPos)
{
    string sErrorPointer = "|   ";
    //sErrorPointer += (char)192 + strfill(string(1,(char)193), 10, 196) + strfill(string(1,(char)217), nPos+2, 196) + "\n";
    sErrorPointer += strfill("^^^", nPos+13) + "\n";
    return sErrorPointer;
}
