
#include "parser_functions.hpp"
#include "../kernel.hpp"

extern value_type vAns;
extern Integration_Vars parser_iVars;
//extern bool bSupressAnswer;
extern mglGraph _fontData;
extern Plugin _plugin;

void parser_plot_create2dPlot(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, Datafile& _data, Parser& _parser, mglData** _mDataPlots, mglData _mAxisVals[2], const Settings& _option, string& sLabels, string& sDataLabels, string* sContStyles, vector<short>& vType, int& nStyle, size_t& nLegends, int* nDataDim, int nDataPlots, int nFunctions, size_t nPlotCompose, size_t nPlotComposeSize);
bool parser_plot_2d(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, mglData& _mData, mglData& _mMaskData, mglData _mAxisVals[2], mglData& _mContVec, const Settings& _option);
void parser_plot_createStdPlot(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, Datafile& _data, Parser& _parser, mglData** _mDataPlots, mglData _mAxisVals[2], const Settings& _option, string& sLabels, string& sDataLabels, string* sLineStyles, string* sPointStyles, string* sContStyles, string* sConPointStyles, vector<short>& vType, int& nStyle, size_t& nLegends, int* nDataDim, int nDataPlots, int nFunctions, size_t nPlotCompose, size_t nPlotComposeSize);
bool parser_plot_std(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, mglData& _mData, mglData& _mAxisVals, mglData _mData2[2], const short nType);
void parser_plot_create3dPlot(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, mglData** _mDataPlots, mglData _mAxisVals[2], const Settings& _option, string& sFunc, string& sLabels, int* nDataDim, int nDataPlots);
void parser_plot_create3dVect(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, mglData** _mDataPlots, string& sFunc, string& sLabels, int* nDataDim, int nDataPlots);
void parser_plot_create2dVect(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, mglData** _mDataPlots, string& sFunc, string& sLabels, int* nDataDim, int nDataPlots);
void parser_plot_create2dDrawing(mglGraph& _graph, Parser& _parser, Datafile& _data, const Settings& _option, vector<string>& vDrawVector, value_type* vResults, int& nFunctions);
void parser_plot_create3dDrawing(mglGraph& _graph, Parser& _parser, Datafile& _data, const Settings& _option, vector<string>& vDrawVector, value_type* vResults, int& nFunctions);
void parser_plot_createStd3dPlot(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, Datafile& _data, Parser& _parser, mglData** _mDataPlots, mglData _mAxisVals[2], const Settings& _option, string& sLabels, string& sDataLabels, string* sLineStyles, string* sPointStyles, string* sContStyles, string* sConPointStyles, vector<short>& vType, int& nStyle, size_t& nLegends, int* nDataDim, int nDataPlots, int nFunctions, size_t nPlotCompose, size_t nPlotComposeSize);
bool parser_plot_std3d(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, mglData _mData[3], mglData _mData2[3], const short nType);
bool checkMultiPlotArray(unsigned int nMultiPlot[2], unsigned int& nSubPlotMap, unsigned int nPlotPos, unsigned int nCols, unsigned int nLines);
long getNN(const mglData& _mData);
void parser_plot_evaluatePlotParamString(PlotInfo& _pInfo, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option);
void parser_plot_filename(PlotData& _pData, PlotInfo& _pInfo, Datafile& _data, Parser& _parser, Settings& _option, size_t nPlotComposeSize, size_t nPlotCompose);
void parser_plot_setStyles(PlotData& _pData, PlotInfo& _pInfo, string* sLineStyles, string* sPointStyles, string* sContStyles, string* sConPointStyles);
void parser_plot_evaluateSubplot(mglGraph& _graph, PlotData& _pData, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option, size_t& nLegends, string& sCmd, size_t nMultiplots[2], size_t& nSubPlots, size_t& nSubPlotMap);
void parser_plot_displayMessage(PlotData& _pData, PlotInfo& _pInfo, mglGraph& _graph, const Settings& _option);
void parser_plot_evaluateDataPlots(PlotData& _pData, PlotInfo& _pInfo, Parser& _parser, Datafile& _data, Define& _functions, mglData**& _mDataPlots, const Settings& _option, vector<short>& vType, string& sFunc, string& sDataPlots, string& sDataLabels, string& sAxisBinds, string& sDataAxisBinds, int*& nDataDim, int& nDataPlots, double dDataRanges[3][2], double dSecDataRanges[2][2]);
void parser_plot_separateLegends(PlotInfo& _pInfo, string& sFunc, string& sLabels);
void parser_plot_prepareMemory(PlotData& _pData, PlotInfo& _pInfo, mglData* _mAxisVals, const string& sFunc, int nFunctions);
void parser_plot_defaultRanges(PlotData& _pData, PlotInfo& _pInfo, mglData** _mDataPlots, double dDataRanges[3][2], double dSecDataRanges[2][2], int* nDataDim, int nDataPlots, size_t nPlotCompose, bool bNewSubPlot);
int parser_plot_fillData(PlotData& _pData, PlotInfo& _pInfo, Parser& _parser, mglData* _mAxisVals, const string& sFunc, value_type* vResults, double dt_max, int t_animate, int nFunctions);
void parser_plot_fitPlotRanges(PlotData& _pData, PlotInfo& _pInfo, mglData** _mDataPlots, const string& sFunc, int* nDataDim, int nDataPlots, double dDataRanges[3][2], size_t nPlotCompose, bool bNewSubPlot);
void parser_plot_clearData(mglData** _mDataPlots, int* nDataDim, int nDataPlots);
void parser_plot_passRangesToGraph(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, mglData** _mDataPlots, mglData* _mAxisVals, const string& sFunc, int* nDataDim, int nDataPlots, double dDataRanges[3][2]);
void parser_plot_applyColorbar(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo);
void parser_plot_applyLighting(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo);

// --> Fuellt das Plotdata-Objekt mit Werten entsprechend sCmd und startet den Plot-Algorithmus <--
void parser_Plot(string& sCmd, Datafile& _data, Parser& _parser, Settings& _option, Define& _functions, PlotData& _pData)
{
    /*! --> Wenn in der ersten Zeile und der ersten Spalte x- und y-Werte stehen, koennten auch mesh-/surf-Plots
     *      aus Daten generiert werden <--
     */

    /*if (true && !(matchParams(sCmd, "opng", '=') || matchParams(sCmd, "oeps", '=') || matchParams(sCmd, "osvg", '=') || matchParams(sCmd, "otex", '=')))
    {
        Graph_helper _grapher(sCmd, &_data, &_parser, &_option, &_functions, &_pData);
        mglQT _graph(&_grapher, "NumeRe: Graph");
        _graph.Rotate(_pData.getRotateAngle(), _pData.getRotateAngle(1));
        _graph.ToggleRotate();
        _graph.Run();
        _pData.deleteData();
        return;
    }*/

    string sFunc = "";                      // string mit allen Funktionen
    string sLabels = "";                    // string mit den Namen aller Funktionen (Fuer die Legende)
    string sDataLabels = "";                // string mit den Datenpunkt-Namen (Fuer die Legende)
    string sDataPlots = "";                 // string mit allen Datenpunkt-Plot-Ausdruecken
    //string sCommand = "";    // string mit dem Plot-Kommando (mesh, grad, plot, surf, cont, etc.)
    vector<string> vPlotCompose;

    mglGraph _graph;                        // Eine Instanz des mglGraph's starten
    //_pData.prepareGraph();
    mglData** _mDataPlots = nullptr;              // 2D-Pointer auf ein Array aus mglData-Objekten
    mglData _mBackground;                   // mglData-Objekt fuer ein evtl. Hintergrundbild
    mglData _mAxisVals[3];
    PlotInfo _pInfo;
    int* nDataDim = nullptr;                      // Pointer auf die jeweilige Dimension der einzelnen Datensaetze

    _pInfo.sCommand = "";
    _pInfo.sPlotParams = "";
    _pInfo.dColorRanges[0] = 0.0;
    _pInfo.dColorRanges[1] = 1.0;
    _pInfo.b2D = false;
    _pInfo.b3D = false;
    _pInfo.b2DVect = false;
    _pInfo.b3DVect = false;
    _pInfo.bDraw = false;
    _pInfo.bDraw3D = false;
    _pInfo.nMaxPlotDim = 1;
    const int STYLEMAX = 14;
    _pInfo.nStyleMax = STYLEMAX;                  // Gesamtzahl der Styles
    int nStyle = 0;                         // Nummer des aktuellen Plotstyles (automatische Variation des Styles)
    _pInfo.nStyle = &nStyle;
    double dDataRanges[3][2];               // Fuer die berechneten Daten-Intervalle (hoehere Prioritaet)
    double dSecDataRanges[2][2] = {{NAN,NAN},{NAN,NAN}};
    string sAxisBinds = "";
    string sDataAxisBinds = "";

    bool bAnimateVar = false;
    vector<string> vDrawVector;
    vector<short> vType;
    //const short TYPE_DATA = -1;
    const short TYPE_FUNC = 1;
    int nFunctions = 0;                     // Zahl der zu plottenden Funktionen
    _pInfo.nFunctions = &nFunctions;
    int nDataPlots = 0;                     // Zahl der zu plottenden Datensaetze
    string sConvLegends = "";               // Variable fuer Legenden, die dem String-Parser zugewiesen werden
    string sDummy = "";
    unsigned int nLegends = 0;
    unsigned int nMultiplots[2] = {0,0};
    bool bNewSubPlot = false;
    unsigned int nSubPlots = 0;
    unsigned int nSubPlotMap = 0;


    if (findCommand(sCmd).sString == "plotcompose")
    {
        sCmd.erase(findCommand(sCmd).nPos, 11);
        if (matchParams(sCmd, "multiplot", '='))
        {
            _parser.SetExpr(getArgAtPos(sCmd, matchParams(sCmd, "multiplot", '=')+9));
            int nRes = 0;
            value_type* v = _parser.Eval(nRes);
            if (nRes == 2)
            {
                nMultiplots[1] = (unsigned int)v[0];
                nMultiplots[0] = (unsigned int)v[1];
            }
            sCmd.erase(0, sCmd.find("<<COMPOSE>>")+11);
            if (findCommand(sCmd).sString != "subplot" && sCmd.find("subplot") != string::npos)
                sCmd.insert(0,"subplot <<COMPOSE>> ");
            //cerr << sCmd << endl;
        }
        StripSpaces(sCmd);
        while (sCmd.length())
        {
            string __sCMD = findCommand(sCmd).sString;
            if ((__sCMD.substr(0,4) == "mesh"
                    || __sCMD.substr(0,4) == "surf"
                    || __sCMD.substr(0,4) == "cont"
                    || __sCMD.substr(0,4) == "vect"
                    || __sCMD.substr(0,4) == "dens"
                    || __sCMD.substr(0,4) == "draw"
                    || __sCMD.substr(0,4) == "grad"
                    || __sCMD.substr(0,4) == "plot")
                && __sCMD.find("3d") != string::npos)
            {
                _pInfo.nMaxPlotDim = 3;
            }
            else if (__sCMD.substr(0,4) == "mesh" || __sCMD.substr(0,4) == "surf" || __sCMD.substr(0,4) == "cont")
            {
                _pInfo.nMaxPlotDim = 3;
            }
            else if (__sCMD.substr(0,4) == "vect" || __sCMD.substr(0,4) == "dens" || __sCMD.substr(0,4) == "grad")
            {
                if (_pInfo.nMaxPlotDim < 3)
                    _pInfo.nMaxPlotDim = 2;
            }
            vPlotCompose.push_back(sCmd.substr(0, sCmd.find("<<COMPOSE>>")));
            sCmd.erase(0, sCmd.find("<<COMPOSE>>")+11);
            StripSpaces(sCmd);
        }
        for (unsigned int i = 0; i < vPlotCompose.size(); i++)
        {
            if (vPlotCompose[i].find("-set") != string::npos
                && !isInQuotes(vPlotCompose[i], vPlotCompose[i].find("-set"))
                && findCommand(vPlotCompose[i]).sString != "subplot")
                _pInfo.sPlotParams += vPlotCompose[i].substr(vPlotCompose[i].find("-set"));
            else if (vPlotCompose[i].find("--") != string::npos
                && !isInQuotes(vPlotCompose[i], vPlotCompose[i].find("--"))
                && findCommand(vPlotCompose[i]).sString != "subplot")
                _pInfo.sPlotParams += vPlotCompose[i].substr(vPlotCompose[i].find("--"));
            _pInfo.sPlotParams += " ";
        }
        if (_pInfo.sPlotParams.length())
        {
            parser_plot_evaluatePlotParamString(_pInfo, _parser, _data, _functions, _option);
            _pData.setGlobalComposeParams(_pInfo.sPlotParams, _parser, _option);
        }
        _pInfo.sPlotParams = "";
    }
    else
    {
        string __sCMD = findCommand(sCmd).sString;
        if ((__sCMD.substr(0,4) == "mesh"
                || __sCMD.substr(0,4) == "surf"
                || __sCMD.substr(0,4) == "cont"
                || __sCMD.substr(0,4) == "vect"
                || __sCMD.substr(0,4) == "dens"
                || __sCMD.substr(0,4) == "draw"
                || __sCMD.substr(0,4) == "grad"
                || __sCMD.substr(0,4) == "plot")
            && __sCMD.find("3d") != string::npos)
        {
            _pInfo.nMaxPlotDim = 3;
        }
        else if (__sCMD.substr(0,4) == "mesh" || __sCMD.substr(0,4) == "surf" || __sCMD.substr(0,4) == "cont")
        {
            _pInfo.nMaxPlotDim = 3;
        }
        else if (__sCMD.substr(0,4) == "vect" || __sCMD.substr(0,4) == "dens" || __sCMD.substr(0,4) == "grad")
        {
            if (_pInfo.nMaxPlotDim < 3)
                _pInfo.nMaxPlotDim = 2;
        }

        vPlotCompose.push_back(sCmd);
    }

    // String-Arrays fuer die endgueltigen Styles:
    string sLineStyles[STYLEMAX];
    string sContStyles[STYLEMAX];
    string sPointStyles[STYLEMAX];
    string sConPointStyles[STYLEMAX];

    _pInfo.sLineStyles = sLineStyles;
    _pInfo.sContStyles = sContStyles;
    _pInfo.sPointStyles = sPointStyles;
    _pInfo.sConPointStyles = sConPointStyles;


    string sOutputName = ""; // string fuer den Export-Dateinamen
    value_type* vResults = nullptr;                   // Pointer auf ein Ergebnis-Array (fuer den muParser)

    // --> Diese Zeile klaert eine gefuellte Zeile, die nicht mit "endl" abgeschlossen wurde <--
    if (!_pData.getSilentMode() && _option.getSystemPrintStatus())
        NumeReKernel::printPreFmt("                                              \r");

    for (unsigned int nPlotCompose = 0; nPlotCompose < vPlotCompose.size(); nPlotCompose++)
    {
        vType.clear();
        sCmd = vPlotCompose[nPlotCompose];
        _pInfo.sPlotParams = "";
        if (nPlotCompose)
        {
            // --> Speicher wieder freigeben <--
            if (_mDataPlots)
            {
                for (int i = 0; i < nDataPlots; i++)
                    delete[] _mDataPlots[i];
                delete[] _mDataPlots;
                _mDataPlots = 0;
                nDataPlots = 0;
                sDataPlots = "";
                sDataLabels = "";
            }
            if (nDataDim)
            {
                delete[] nDataDim;
                nDataDim = 0;
            }
            _pData.deleteData();
            _pInfo.b2D = false;
            _pInfo.b3D = false;
            _pInfo.b2DVect = false;
            _pInfo.b3DVect = false;
            _pInfo.bDraw3D = false;
            _pInfo.bDraw = false;
            vDrawVector.clear();
            sLabels = "";
        }
        _pInfo.sCommand = findCommand(sCmd).sString;
        if (sCmd.find("-set") != string::npos && !isInQuotes(sCmd, sCmd.find("-set")))
            sFunc = sCmd.substr(_pInfo.sCommand.length(), sCmd.find("-set")-_pInfo.sCommand.length());
        else if (sCmd.find("--") != string::npos && !isInQuotes(sCmd, sCmd.find("--")))
            sFunc = sCmd.substr(_pInfo.sCommand.length(), sCmd.find("--")-_pInfo.sCommand.length());
        else
            sFunc = sCmd.substr(_pInfo.sCommand.length());

        // --> Unnoetige Leerstellen entfernen <--
        StripSpaces(sFunc);

        // --> Ruf' ggf. den Prompt auf <--
        if (sFunc.find("??") != string::npos)
            sFunc = parser_Prompt(sFunc);

        // --> Ggf. die Plotparameter setzen <--
        if (sCmd.find("-set") != string::npos && !isInQuotes(sCmd, sCmd.find("-set")) && _pInfo.sCommand != "subplot")
            _pInfo.sPlotParams = sCmd.substr(sCmd.find("-set"));
        else if (sCmd.find("--") != string::npos && !isInQuotes(sCmd, sCmd.find("--")) && _pInfo.sCommand != "subplot")
            _pInfo.sPlotParams = sCmd.substr(sCmd.find("--"));
        if (_pInfo.sPlotParams.length())
        {
            parser_plot_evaluatePlotParamString(_pInfo, _parser, _data, _functions, _option);
            if (vPlotCompose.size() > 1 && !bNewSubPlot)
                _pData.setLocalComposeParams(_pInfo.sPlotParams, _parser, _option);
            else
            {
                _pData.setParams(_pInfo.sPlotParams, _parser, _option);
            }
        }

        if (!sFunc.length() && vPlotCompose.size() > 1 && _pInfo.sCommand != "subplot")
            continue;
        else if (!sFunc.length() && _pInfo.sCommand != "subplot")
            throw PLOT_ERROR;

        if (!nPlotCompose)
        {
            if (_pData.getHighRes() == 2)           // Aufloesung und Qualitaet einstellen
            {
                double dHeight = sqrt(1920.0 * 1440.0 / _pData.getAspect());
                _graph.SetSize((int)lrint(_pData.getAspect()*dHeight), (int)lrint(dHeight));          // FullHD!
            }
            else if (_pData.getHighRes() == 1 || !_option.getbUseDraftMode())
            {
                double dHeight = sqrt(1280.0 * 960.0 / _pData.getAspect());
                _graph.SetSize((int)lrint(_pData.getAspect()*dHeight), (int)lrint(dHeight));           // ehem. die bessere Standard-Aufloesung
                //_graph.SetQuality(5);
            }
            else
            {
                double dHeight = sqrt(800.0 * 600.0 / _pData.getAspect());
                _graph.SetSize((int)lrint(_pData.getAspect()*dHeight), (int)lrint(dHeight));
                // --> Im Falle, dass wir meinen mesh/surf/anders gearteten 3D-Plot machen, drehen wir die Qualitaet runter <--
                if (_pInfo.sCommand.substr(0,4) != "plot")
                    _graph.SetQuality(MGL_DRAW_FAST);
            }
            // --> Noetige Einstellungen und Deklarationen fuer den passenden Plot-Stil <--
            _graph.CopyFont(&_fontData);
            _graph.SetFontSizeCM(0.24*((double)(1+_pData.getTextSize())/6.0), 72);
        }

        parser_plot_setStyles(_pData, _pInfo, sLineStyles, sPointStyles, sContStyles, sConPointStyles);

        if (_pData.getBars() || _pData.getHBars())
            _graph.SetBarWidth(_pData.getBars() ? _pData.getBars() : _pData.getHBars());

        // --> Allg. Einstellungen <--
        //_graph.SetFontSizeCM(0.28, 72);
        if (!_pData.getAnimateSamples()
            || (_pData.getAnimateSamples()
                && _pInfo.sCommand.substr(0,4) != "mesh"
                && _pInfo.sCommand.substr(0,4) != "surf"
                && _pInfo.sCommand.substr(0,4) != "grad"
                && _pInfo.sCommand.substr(0,4) != "cont"))
            _pInfo.nSamples = _pData.getSamples();
        else if (_pData.getSamples() > 1000
            && (_pInfo.sCommand.substr(0,4) == "mesh"
                || _pInfo.sCommand.substr(0,4) == "surf"
                || _pInfo.sCommand.substr(0,4) == "grad"
                || _pInfo.sCommand.substr(0,4) == "cont"))
            _pInfo.nSamples = 1000;
        else
            _pInfo.nSamples = 100;

        if (_pInfo.nSamples > 100 && _pInfo.b3D)
            _pInfo.nSamples = 100;


        // --> Ggf. waehlen eines Default-Dateinamens <--
        parser_plot_filename(_pData, _pInfo, _data, _parser, _option, vPlotCompose.size(), nPlotCompose);

        if (!nPlotCompose)
        {
            sOutputName = _pData.getFileName();
            StripSpaces(sOutputName);
            if (sOutputName[0] == '"' && sOutputName[sOutputName.length()-1] == '"')
                sOutputName = sOutputName.substr(1,sOutputName.length()-2);
        }

        //!!!! SUBPLOT-Logik
        if (findCommand(sCmd).sString == "subplot" && nMultiplots[0] && nMultiplots[1])
        {
            bNewSubPlot = true;
            nStyle = 0;

            parser_plot_evaluateSubplot(_graph, _pData, _parser, _data, _functions, _option, nLegends, sCmd, nMultiplots, nSubPlots, nSubPlotMap);

            nSubPlots++;
            continue;
        }
        else if (findCommand(sCmd).sString == "subplot")
            continue;

        parser_plot_displayMessage(_pData, _pInfo, _graph, _option);

        // --> Logarithmische Skalierung; ein bisschen Fummelei <--
        if (!_pData.getCoords()
            && (!nPlotCompose || bNewSubPlot)
            && (_pData.getxLogscale() || _pData.getyLogscale() || _pData.getzLogscale() || _pData.getcLogscale()))
        {
            parser_setLogScale(_graph, _pData, (_pInfo.b2D || _pInfo.sCommand == "plot3d"));
        }

        if (!_pInfo.bDraw3D && !_pInfo.bDraw)
        {
            // --> Bevor die Call-Methode auf Define angewendet wird, sollten wir die Legenden-"strings" in sFunc kopieren <--
            if (!addLegends(sFunc))
                return; // --> Bei Fehlern: Zurueck zur aufrufenden Funktion <--
        }

        // --> Ersetze Definitionen durch die tatsaechlichen Ausdruecke <--
        if (!_functions.call(sFunc, _option))
            throw FUNCTION_ERROR;

        // --> Ruf' ggf. nochmals den Prompt auf (nicht dass es wirklich zu erwarten waere) <--
        if (sFunc.find("??") != string::npos)
            sFunc = parser_Prompt(sFunc);

        /*--> Hier werden die Datenpunkt-Plots an die mglData-Instanz _mDataPlots zugewiesen.
         *    Dabei erlaubt die Syntax sowohl eine gezielte Wahl von Datenreihen (Spalten),
         *    als auch eine automatische. <--
         */
        parser_plot_evaluateDataPlots(_pData, _pInfo, _parser, _data, _functions, _mDataPlots, _option,
                                        vType, sFunc, sDataPlots, sDataLabels, sAxisBinds, sDataAxisBinds,
                                        nDataDim, nDataPlots, dDataRanges, dSecDataRanges);

        StripSpaces(sFunc);
        if (!sFunc.length() && !_mDataPlots)
            throw PLOT_ERROR;


        // --> Zuweisen der uebergebliebenen Funktionen an den "Legenden-String". Ebenfalls extrem fummelig <--
        parser_plot_separateLegends(_pInfo, sFunc, sLabels);

        // --> Vektor-Konvertierung <--
        if (sFunc.find("{") != string::npos && !_pInfo.bDraw3D && !_pInfo.bDraw)
            parser_VectorToExpr(sFunc, _option);

        // --> "String"-Objekt? So was kann man einfach nicht plotten: Aufraeumen und zurueck! <--
        if ((containsStrings(sFunc) || _data.containsStringVars(sFunc)) && !(_pInfo.bDraw3D || _pInfo.bDraw))
        {
            parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
            throw CANNOT_PLOT_STRINGS;
        }

        /* --> Wenn der Funktionen-String nicht leer ist, weise ihn dem Parser zu; anderenfalls verwende das
         *     eindeutige Token "<<empty>>", dass viel einfacher zu identfizieren ist <--
         */
        if (parser_ExprNotEmpty(sFunc) && !(_pInfo.bDraw3D || _pInfo.bDraw))
        {
            try
            {
                _parser.SetExpr(sFunc);
                _parser.Eval();

                if (parser_CheckVarOccurence(_parser, "t"))
                    bAnimateVar = true;
                nFunctions = _parser.GetNumResults();
                if ((_pData.getColorMask() || _pData.getAlphaMask()) && _pInfo.b2D && (nFunctions + nDataPlots) % 2)
                {
                    throw NUMBER_OF_FUNCTIONS_NOT_MATCHING;
                }
            }
            catch (...)
            {
                parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
                throw;
            }
        }
        else if (parser_ExprNotEmpty(sFunc) && (_pInfo.bDraw3D || _pInfo.bDraw))
        {
            string sArgument;
            sFunc += " ";
            while (sFunc.length())
            {
                sArgument = getNextArgument(sFunc, true);
                StripSpaces(sArgument);
                if (!sArgument.length())
                    continue;
                vDrawVector.push_back(sArgument);
            }
            sFunc = "<<empty>>";
        }
        else
            sFunc = "<<empty>>";

        if (nFunctions && !vType.size())
        {
            vType.assign((unsigned int)nFunctions, TYPE_FUNC);
            for (int i = 0; i < nFunctions; i++)
            {
                sAxisBinds += _pData.getAxisbind(i);
            }
        }
        _pData.setFunctionAxisbind(sAxisBinds);

        /* --> Die DatenPlots ueberschreiben ggf. die Ranges, allerdings haben vom Benutzer gegebene
         *     Ranges stets die hoechste Prioritaet (was eigentlich auch logisch ist) <--
         * --> Log-Plots duerfen in der gewaehlten Achse nicht <= 0 sein! <--
         */

        parser_plot_defaultRanges(_pData, _pInfo, _mDataPlots, dDataRanges, dSecDataRanges, nDataDim, nDataPlots, nPlotCompose, bNewSubPlot);

        // --> Plotvariablen auf den Anfangswert setzen <--
        parser_iVars.vValue[0][0] = _pInfo.dRanges[0][0];  // Plotvariable: x
        if (_pInfo.b2D || _pInfo.b2DVect)
        {
            parser_iVars.vValue[1][0] = _pInfo.dRanges[1][0];  // Plotvariable: y
        }
        if (_pInfo.b3D || _pInfo.b3DVect)
        {
            parser_iVars.vValue[1][0] = _pInfo.dRanges[1][0];  // Plotvariable: y
            parser_iVars.vValue[2][0] = _pInfo.dRanges[2][0];  // Plotvariable: z
        }
        if (_pInfo.sCommand == "plot3d" || _pData.getAnimateSamples())
        {
            parser_iVars.vValue[3][0] = _pData.gettBoundary();  // Plotparameter: t
        }

        // --> Plot-Speicher vorbereiten <--
        parser_plot_prepareMemory(_pData, _pInfo, _mAxisVals, sFunc, nFunctions);

        if (_pData.getAnimateSamples())
            _graph.StartGIF(sOutputName.c_str(), 40); // 40msec = 2sec bei 50 Frames, d.h. 25 Bilder je Sekunde

        string sLabelsCache[2];
        sLabelsCache[0] = sLabels;
        sLabelsCache[1] = sDataLabels;

        if (_pData.getBackground().length() && _pData.getBGColorScheme() != "<<REALISTIC>>")
        {
            NumeReKernel::printPreFmt("|-> " + toSystemCodePage(_lang.get("PLOT_LOADING_BACKGROUND")) + " ... ");
            _mBackground.Import(_pData.getBackground().c_str(), "kw");
            NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_DONE")) + ".\n");
        }

        /*********************************
         * Eigentlicher Plot-Algorithmus *
         *********************************/

        for (int t_animate = 0; t_animate <= _pData.getAnimateSamples(); t_animate++)
        {
            if (_pData.getAnimateSamples() && !_pData.getSilentMode() && _option.getSystemPrintStatus() && bAnimateVar)
            {
                // --> Falls es eine Animation ist, muessen die Plotvariablen zu Beginn zurueckgesetzt werden <--
                NumeReKernel::printPreFmt("\r|-> " + toSystemCodePage(_lang.get("PLOT_RENDERING_FRAME", toString(t_animate+1), toString(_pData.getAnimateSamples()+1))) + " ... ");
                nStyle = 0;

                // --> Neuen GIF-Frame oeffnen <--
                _graph.NewFrame();
                if (t_animate)
                {
                    parser_iVars.vValue[0][0] = _pInfo.dRanges[0][0];
                    parser_iVars.vValue[1][0] = _pInfo.dRanges[1][0];
                    parser_iVars.vValue[2][0] = _pInfo.dRanges[2][0];
                    parser_iVars.vValue[3][0] += (_pData.gettBoundary(1) - _pData.gettBoundary())/(double)_pData.getAnimateSamples();
                    sLabels = sLabelsCache[0];
                    sDataLabels = sLabelsCache[1];
                }
            }
            double dt_max = parser_iVars.vValue[3][0];

            // --> Ist ein Titel gegeben? Dann weisse ihn zu <--
            if (_pData.getTitle().length())
                _graph.Title(fromSystemCodePage(_pData.getTitle()).c_str(), "", -1.5);

            // --> Orthogonal-Projektionen aktivieren <--
            if (_pData.getOrthoProject()
                && (_pInfo.b3D
                    || (_pInfo.b2D && _pInfo.sCommand.substr(0,4) != "grad" && _pInfo.sCommand.substr(0,4) != "dens")
                    || _pInfo.b3DVect
                    || _pInfo.sCommand == "plot3d")
                )
            {
                _graph.Ternary(4);
                _graph.SetRotatedText(false);
            }

            // --> Rotationen und allg. Grapheneinstellungen <--
            if (_pInfo.nMaxPlotDim > 2 && (!nPlotCompose || bNewSubPlot))
            {
                _graph.Rotate(_pData.getRotateAngle(), _pData.getRotateAngle(1));
            }

            nFunctions = parser_plot_fillData(_pData, _pInfo, _parser, _mAxisVals, sFunc, vResults, dt_max, t_animate, nFunctions);

            if (_pInfo.b2DVect)
                _pData.normalize(2, t_animate);
            else if (_pInfo.b3DVect)
                _pData.normalize(3, t_animate);

            parser_plot_fitPlotRanges(_pData, _pInfo, _mDataPlots, sFunc, nDataDim, nDataPlots, dDataRanges, nPlotCompose, bNewSubPlot);

            // --> (Endgueltige) Darstellungsintervalle an das Plot-Objekt uebergeben <--
            if (!nPlotCompose || bNewSubPlot)
            {
                parser_plot_passRangesToGraph(_graph, _pData, _pInfo, _mDataPlots, _mAxisVals, sFunc, nDataDim, nDataPlots, dDataRanges);
            }

            parser_plot_applyColorbar(_graph, _pData, _pInfo);

            parser_plot_applyLighting(_graph, _pData, _pInfo);

            if ((!nPlotCompose || bNewSubPlot) && _pInfo.nMaxPlotDim > 2)
                _graph.Perspective(_pData.getPerspective());

            // --> Rendern des Hintergrundes <--
            if (_pData.getBackground().length())
            {
                NumeReKernel::printPreFmt("|-> " + toSystemCodePage(_lang.get("PLOT_RENDERING_BACKGROUND")) + " ... ");
                if (_pData.getBGColorScheme() != "<<REALISTIC>>")
                {
                    _graph.SetRanges(_pInfo.dRanges[0][0], _pInfo.dRanges[0][1], _pInfo.dRanges[1][0], _pInfo.dRanges[1][1], _mBackground.Minimal(), _mBackground.Maximal());
                    _graph.Dens(_mBackground, _pData.getBGColorScheme().c_str());
                }
                else
                    _graph.Logo(_pData.getBackground().c_str());

                //_graph.Dens(_mBackground, "kRQYEGLCNBUMPw");
                _graph.Rasterize();
                _graph.SetRanges(_pInfo.dRanges[0][0], _pInfo.dRanges[0][1], _pInfo.dRanges[1][0], _pInfo.dRanges[1][1], _pInfo.dRanges[2][0], _pInfo.dRanges[2][1]);
                NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_DONE")) + ".\n");
            }

            // --> Nun kopieren wir die aufbereiteten Datenpunkte in ein mglData-Objekt und plotten die Daten aus diesem Objekt <--
            if (_pInfo.b2D)        // 2D-Plot
            {
                parser_plot_create2dPlot(_graph, _pData, _pInfo, _data, _parser, _mDataPlots, _mAxisVals, _option,
                                        sLabels, sDataLabels, sContStyles, vType,
                                        nStyle, nLegends, nDataDim, nDataPlots, nFunctions, nPlotCompose, vPlotCompose.size());
            }
            else if (_pInfo.sCommand != "plot3d" && !_pInfo.b3D && !_pInfo.b3DVect && !_pInfo.b2DVect && !_pInfo.bDraw3D && !_pInfo.bDraw)      // Standardplot
            {
                parser_plot_createStdPlot(_graph, _pData, _pInfo, _data, _parser, _mDataPlots, _mAxisVals, _option,
                                        sLabels, sDataLabels, sLineStyles, sPointStyles, sContStyles, sConPointStyles, vType,
                                        nStyle, nLegends, nDataDim, nDataPlots, nFunctions, nPlotCompose, vPlotCompose.size());
            }
            else if (_pInfo.b3D)   // 3D-Plot
            {
                parser_plot_create3dPlot(_graph, _pData, _pInfo, _mDataPlots, _mAxisVals, _option, sFunc, sLabels, nDataDim, nDataPlots);
            }
            else if (_pInfo.b3DVect)   // 3D-Vektorplot
            {
                parser_plot_create3dVect(_graph, _pData, _pInfo, _mDataPlots, sFunc, sLabels, nDataDim, nDataPlots);
            }
            else if (_pInfo.b2DVect)   // 2D-Vektorplot
            {
                parser_plot_create2dVect(_graph, _pData, _pInfo, _mDataPlots, sFunc, sLabels, nDataDim, nDataPlots);
            }
            else if (_pInfo.bDraw)
            {
                parser_plot_create2dDrawing(_graph, _parser, _data, _option, vDrawVector, vResults, nFunctions);
            }
            else if (_pInfo.bDraw3D)
            {
                parser_plot_create3dDrawing(_graph, _parser, _data, _option, vDrawVector, vResults, nFunctions);
            }
            else            // 3D-Trajektorie
            {
                parser_plot_createStd3dPlot(_graph, _pData, _pInfo, _data, _parser, _mDataPlots, _mAxisVals, _option,
                                        sLabels, sDataLabels, sLineStyles, sPointStyles, sContStyles, sConPointStyles, vType,
                                        nStyle, nLegends, nDataDim, nDataPlots, nFunctions, nPlotCompose, vPlotCompose.size());
            }

            // --> GIF-Frame beenden <--
            if (_pData.getAnimateSamples() && bAnimateVar)
                _graph.EndFrame();
            if (!bAnimateVar)
                break;
        }

        // --> GIF-Datei schliessen <--
        if (_pData.getAnimateSamples() && bAnimateVar)
            _graph.CloseGIF();
        bNewSubPlot = false;
    }
    // --> Speichern und Erfolgsmeldung <--
    if (!_pData.getAnimateSamples() || !bAnimateVar)
    {
        if (!_pData.getSilentMode() && _option.getSystemPrintStatus())
            NumeReKernel::printPreFmt("|-> " + toSystemCodePage(_lang.get("PLOT_SAVING")) + " ... ");
        _graph.WriteFrame(sOutputName.c_str());

        // --> TeX-Ausgabe gewaehlt? Dann werden mehrere Dateien erzeugt, die an den Zielort verschoben werden muessen <--
        if (sOutputName.substr(sOutputName.length()-4, 4) == ".tex")
        {
            writeTeXMain(sOutputName);
        }
    }
    if (!_pData.getSilentMode() && _option.getSystemPrintStatus())
    {
        NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_SUCCESS")) + ".\n");
        NumeReKernel::printPreFmt(LineBreak("|   " +_lang.get("PLOT_SAVE_LOCATION", sOutputName), _option, 0) + "\n");
    }
    // --> Ist ein ImageViewer angegeben und der "openImage" TRUE? Dann oeffnen wir das erzeugte Bild mit diesem <--
    if (_pData.getOpenImage() && !_pData.getSilentMode() && _option.getSystemPrintStatus())
    {
        if (sOutputName.substr(sOutputName.rfind('.')) == ".tex"
            || sOutputName.substr(sOutputName.rfind('.')) == ".png"
            || sOutputName.substr(sOutputName.rfind('.')) == ".gif"
            || sOutputName.substr(sOutputName.rfind('.')) == ".jpg"
            || sOutputName.substr(sOutputName.rfind('.')) == ".jpeg"
            || sOutputName.substr(sOutputName.rfind('.')) == ".bmp")
            NumeReKernel::gotoLine(sOutputName);
        else
            openExternally(sOutputName, _option.getViewerPath(), _pData.getPath());
    }

    // --> Speicher wieder freigeben <--
    parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
    _pData.deleteData();

    // --> Zurueck zur aufrufenden Funktion! <--
    return;
}


void parser_plot_create2dPlot(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, Datafile& _data, Parser& _parser, mglData** _mDataPlots, mglData _mAxisVals[2], const Settings& _option,
                            string& sLabels, string& sDataLabels, string* sContStyles, vector<short>& vType,
                            int& nStyle, size_t& nLegends, int* nDataDim, int nDataPlots, int nFunctions, size_t nPlotCompose, size_t nPlotComposeSize)
{
    string sDummy = "";
    string sConvLegends = "";
    const short TYPE_FUNC = 1;
    const short TYPE_DATA = -1;
    mglData _mData(_pInfo.nSamples, _pInfo.nSamples);
    mglData _mMaskData;
    mglData _mPlotAxes[2];
    if (_pData.getColorMask() || _pData.getAlphaMask())
        _mMaskData.Create(_pInfo.nSamples, _pInfo.nSamples);

    mglData _mContVec(35);
    for (int nCont = 0; nCont < 35; nCont++)
    {
        _mContVec.a[nCont] = nCont*(_pInfo.dRanges[2][1]-_pInfo.dRanges[2][0])/34.0+_pInfo.dRanges[2][0];
    }
    _mContVec.a[17] = (_pInfo.dRanges[2][1]-_pInfo.dRanges[2][0])/2.0 + _pInfo.dRanges[2][0];

    int nPos[2] = {0,0};
    int nTypeCounter[2] = {0,0};

    for (unsigned int nType = 0; nType < vType.size(); nType++)
    {
        if (vType[nType] == TYPE_FUNC)
        {
            StripSpaces(sLabels);
            _mData.Create(_pInfo.nSamples, _pInfo.nSamples);
            for (long int i = 0; i < _pInfo.nSamples; i++)
            {
                for (long int j = 0; j < _pInfo.nSamples; j++)
                {
                    // --> Das Array im mglData-Objekt ist nur 1D vorhanden <--
                    _mData.a[i+_pInfo.nSamples*j] = _pData.getData(i,j,nTypeCounter[0]);
                    if ((_pData.getColorMask() || _pData.getAlphaMask()) && (vType.size() <= nType+1 || (vType.size() > nType+1 && vType[nType+1] == TYPE_FUNC)))
                        _mMaskData.a[i+_pInfo.nSamples*j] = _pData.getData(i,j,nTypeCounter[0]+1);
                }
            }
            _mPlotAxes[0] = _mAxisVals[0];
            _mPlotAxes[1] = _mAxisVals[1];
            if ((_pData.getColorMask() || _pData.getAlphaMask()) && vType.size() > nType+1 && vType[nType+1] == TYPE_DATA)
            {
                _mMaskData = _mDataPlots[nTypeCounter[1]][2];
                nTypeCounter[1]++;
            }
            else if ((_pData.getColorMask() || _pData.getAlphaMask()) && (vType.size() > nType+1 && vType[nType+1] == TYPE_FUNC))
                nTypeCounter[0]++;
        }
        else
        {
            StripSpaces(sDataLabels);
            if (_pData.getCoords())
                _mDataPlots[nTypeCounter[1]][0] = parser_fmod(_mDataPlots[nTypeCounter[1]][0], 2.0*M_PI);
            if (_pData.getCoords() == 2)
                _mDataPlots[nTypeCounter[1]][1] = parser_fmod(_mDataPlots[nTypeCounter[1]][1], 1.0*M_PI);

            _mData = _mDataPlots[nTypeCounter[1]][2];
            _mPlotAxes[0] = _mDataPlots[nTypeCounter[1]][0];
            _mPlotAxes[1] = _mDataPlots[nTypeCounter[1]][1];
            if ((_pData.getColorMask() || _pData.getAlphaMask()) && vType.size() > nType+1 && vType[nType+1] == TYPE_DATA)
            {
                _mMaskData = _mDataPlots[nTypeCounter[1]+1][2];
                nTypeCounter[1]++;
            }
            else if ((_pData.getColorMask() || _pData.getAlphaMask()) && (vType.size() <= nType+1 || (vType.size() > nType+1 && vType[nType+1] == TYPE_FUNC)))
            {
                for (long int i = 0; i < _pInfo.nSamples; i++)
                {
                    for (long int j = 0; j < _pInfo.nSamples; j++)
                    {
                        // --> Das Array im mglData-Objekt ist nur 1D vorhanden <--
                        _mData.a[i+_pInfo.nSamples*j] = _pData.getData(i,j,nTypeCounter[0]);
                        if ((_pData.getColorMask() || _pData.getAlphaMask()) && (vType.size() <= nType+1 || (vType.size() > nType+1 && vType[nType+1] == TYPE_FUNC)))
                            _mMaskData.a[i+_pInfo.nSamples*j] = _pData.getData(i,j,nTypeCounter[0]+1);
                    }
                }
                nTypeCounter[0]++;
            }
        }
        if (!parser_plot_2d(_graph, _pData, _pInfo, _mData, _mMaskData, _mPlotAxes, _mContVec, _option))
        {
            // --> Den gibt's nicht: Speicher freigeben und zurueck! <--
            parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
            return;
        }
        if (vType[nType] == TYPE_FUNC)
        {
            if ((nFunctions > 1 && !(_pData.getColorMask() || _pData.getAlphaMask()))
                || (nFunctions > 2 && (_pData.getColorMask() || _pData.getAlphaMask()))
                || (nFunctions && nDataPlots))
            {
                if (_pData.getContLabels() && _pInfo.sCommand.substr(0,4) != "cont")
                {
                    _graph.Cont(_mAxisVals[0], _mAxisVals[1], _mData, ("t"+sContStyles[nStyle]).c_str());
                }
                else if (!_pData.getContLabels() && _pInfo.sCommand.substr(0,4) != "cont")
                    _graph.Cont(_mAxisVals[0], _mAxisVals[1], _mData, sContStyles[nStyle].c_str());
                nPos[0] = sLabels.find(';');
                sConvLegends = sLabels.substr(0,nPos[0]);
                // --> Der String-Parser wertet Ausdruecke wie "Var " + #var aus <--
                parser_StringParser(sConvLegends, sDummy, _data, _parser, _option, true);
                sLabels = sLabels.substr(nPos[0]+1);
                if (sConvLegends != "\"\"")
                {
                    _graph.AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sContStyles[nStyle].c_str());
                    nLegends++;
                }
                if (nStyle == _pInfo.nStyleMax-1)
                    nStyle = 0;
                else
                    nStyle++;
            }
            nTypeCounter[0]++;
        }
        else
        {
            if ((nDataPlots > 1 && !(_pData.getColorMask() || _pData.getAlphaMask()))
                || (nDataPlots > 2 && (_pData.getColorMask() || _pData.getAlphaMask()))
                || (nFunctions && nDataPlots))
            {
                if (_pData.getContLabels() && _pInfo.sCommand.substr(0,4) != "cont")
                {
                    _graph.Cont(_mDataPlots[nTypeCounter[1]][2], ("t"+sContStyles[nStyle]).c_str());
                }
                else if (!_pData.getContLabels() && _pInfo.sCommand.substr(0,4) != "cont")
                    _graph.Cont(_mDataPlots[nTypeCounter[1]][2], sContStyles[nStyle].c_str());
                nPos[1] = sDataLabels.find(';');
                sConvLegends = sDataLabels.substr(0,nPos[1]);
                // --> Der String-Parser wertet Ausdruecke wie "Var " + #var aus <--
                parser_StringParser(sConvLegends, sDummy, _data, _parser, _option, true);
                sDataLabels = sDataLabels.substr(nPos[1]+1);
                if (sConvLegends != "\"\"")
                {
                    _graph.AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sContStyles[nStyle].c_str());
                    nLegends++;
                }
                if (nStyle == _pInfo.nStyleMax-1)
                    nStyle = 0;
                else
                    nStyle++;
            }
            nTypeCounter[1]++;
        }
        if ((_pData.getColorMask() || _pData.getAlphaMask()) && vType.size() > nType+1)
            nType++;
    }
    // --> Position der Legende etwas aendern <--
    if (nFunctions > 1 && nLegends && !_pData.getSchematic() && nPlotCompose+1 == nPlotComposeSize)
    {
        _graph.Legend(1.35,1.2);
    }
}

bool parser_plot_2d(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, mglData& _mData, mglData& _mMaskData, mglData _mAxisVals[2], mglData& _mContVec, const Settings& _option)
{
    /*if (_option.getbDebug())
        cerr << "|-> DEBUG: generating 2D-Plot..." << endl;*/

    if (_pData.getCutBox()
        && _pInfo.sCommand.substr(0,4) != "cont"
        && _pInfo.sCommand.substr(0,4) != "grad"
        && _pInfo.sCommand.substr(0,4) != "dens")
        _graph.SetCutBox(parser_CalcCutBox(_pData.getRotateAngle(1), _pInfo.dRanges, 0, _pData.getCoords()), parser_CalcCutBox(_pData.getRotateAngle(1), _pInfo.dRanges, 1, _pData.getCoords()));

    // --> Entsprechend dem gewuenschten Plotting-Style plotten <--
    if (_pInfo.sCommand.substr(0,4) == "mesh")
    {
        if (_pData.getBars())
            _graph.Boxs(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme("#").c_str());
        else
            _graph.Mesh(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
    }
    else if (_pInfo.sCommand.substr(0,4) == "surf")
    {
        if (_pData.getBars())
            _graph.Boxs(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
        else if (_pData.getColorMask())
            _graph.SurfC(_mAxisVals[0], _mAxisVals[1], _mData, _mMaskData, _pData.getColorScheme().c_str());
        else if (_pData.getAlphaMask())
            _graph.SurfA(_mAxisVals[0], _mAxisVals[1], _mData, _mMaskData, _pData.getColorScheme().c_str());
        else
            _graph.Surf(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
        //cerr << "\"" << _pData.getColorScheme().c_str() << "\" " << strlen(_pData.getColorScheme().c_str()) << endl;
    }
    else if (_pInfo.sCommand.substr(0,4) == "cont")
    {
        if (_pData.getContLabels())
        {
            if (_pData.getContFilled())
                _graph.ContF(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
            _graph.Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme("t").c_str());
        }
        else if (_pData.getContProj())
        {
            if (_pData.getContFilled())
            {
                _graph.ContF(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme("_").c_str());
                _graph.Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, "_k");
            }
            else
                _graph.Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme("_").c_str());
        }
        else if (_pData.getContFilled())
        {
            _graph.ContF(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
            _graph.Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, "k");
        }
        else
            _graph.Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
    }
    else if (_pInfo.sCommand.substr(0,4) == "grad")
    {
        if (_pData.getHighRes() || !_option.getbUseDraftMode())
        {
            if (_pData.getContFilled() && _pData.getContProj())
                _graph.Grad(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorSchemeMedium().c_str(), "value 10");
            else
                _graph.Grad(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str(), "value 10");
        }
        else
        {
            if (_pData.getContFilled() && _pData.getContProj())
                _graph.Grad(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorSchemeMedium().c_str());
            else
                _graph.Grad(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
        }
        if (!(_pData.getContFilled() && _pData.getContProj()))
            _graph.Dens(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorSchemeLight().c_str());
    }
    else if (_pInfo.sCommand.substr(0,4) == "dens")
    {
        if (_pData.getBars())
            _graph.Tile(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
        else
            _graph.Dens(_mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
    }
    else
        return false;

    if (_pData.getCutBox()
        && _pInfo.sCommand.substr(0,4) != "cont"
        && _pInfo.sCommand.substr(0,4) != "grad"
        && _pInfo.sCommand.substr(0,4) != "dens")
        _graph.SetCutBox(mglPoint(0), mglPoint(0));
    // --> Ggf. Konturlinien ergaenzen <--
    if (_pData.getContProj() && _pInfo.sCommand.substr(0,4) != "cont")
    {
        if (_pData.getContFilled() && _pInfo.sCommand.substr(0,4) != "dens")
        {
            _graph.ContF(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme("_").c_str());
            _graph.Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, "_k");
        }
        else if (_pInfo.sCommand.substr(0,4) == "dens" && _pData.getContFilled())
            _graph.Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, "_k");
        else
            _graph.Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme("_").c_str());
    }
    if (_pData.getContLabels() && _pInfo.sCommand.substr(0,4) != "cont" && *_pInfo.nFunctions == 1)
    {
        _graph.Cont(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, ("t"+_pInfo.sContStyles[*_pInfo.nStyle]).c_str());
        if (_pData.getContFilled())
            _graph.ContF(_mContVec, _mAxisVals[0], _mAxisVals[1], _mData, _pData.getColorScheme().c_str());
    }
    return true;
}

void parser_plot_createStdPlot(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, Datafile& _data, Parser& _parser, mglData** _mDataPlots, mglData _mAxisVals[2], const Settings& _option,
                            string& sLabels, string& sDataLabels, string* sLineStyles, string* sPointStyles, string* sContStyles, string* sConPointStyles, vector<short>& vType,
                            int& nStyle, size_t& nLegends, int* nDataDim, int nDataPlots, int nFunctions, size_t nPlotCompose, size_t nPlotComposeSize)
{
    string sDummy = "";
    string sConvLegends = "";
    const short TYPE_FUNC = 1;
    const short TYPE_DATA = -1;

    mglData _mData(_pInfo.nSamples);
    mglData _mPlotAxes;
    mglData _mData2[2];


    int nPos[2] = {0,0};
    int nTypeCounter[2] = {0,0};

    for (unsigned int nType = 0; nType < vType.size(); nType++)
    {
        for (int i = 0; i < 2; i++)
            _mData2[i].Create(_pInfo.nSamples);
        _mData.Create(_pInfo.nSamples);
        if (vType[nType] == TYPE_FUNC)
        {
            StripSpaces(sLabels);
            for (long int i = 0; i < _pInfo.nSamples; i++)
            {
                _mData.a[i] = _pData.getData(i,nTypeCounter[0]);
            }
            _mPlotAxes = _mAxisVals[0];
            if (_pData.getRegion() && vType.size() > nType+1 && vType[nType+1] == TYPE_DATA)
            {
                _mData2[0] = _mDataPlots[nTypeCounter[1]][1];
                nTypeCounter[1]++;
            }
            else if (_pData.getRegion() && vType.size() > nType+1 && vType[nType+1] == TYPE_FUNC)
            {
                for (long int i = 0; i < _pInfo.nSamples; i++)
                    _mData2[0].a[i] = _pData.getData(i,nTypeCounter[0]+1);
                nTypeCounter[0]++;
            }
            else
            {
                for (long int i = 0; i < _pInfo.nSamples; i++)
                    _mData2[0].a[i] = 0.0;
            }
            if (_pData.getAxisbind(nType)[0] == 'r')
            {
                _mData = (_mData-_pInfo.dSecAxisRanges[1][0]) * (_pInfo.dRanges[1][1]-_pInfo.dRanges[1][0])/(_pInfo.dSecAxisRanges[1][1]-_pInfo.dSecAxisRanges[1][0]) + _pInfo.dRanges[1][0];
            }
            if (_pData.getRegion() && _pData.getAxisbind(nType+1)[0] == 'r')
            {
                _mData2[0] = (_mData2[0]-_pInfo.dSecAxisRanges[1][0]) * (_pInfo.dRanges[1][1]-_pInfo.dRanges[1][0])/(_pInfo.dSecAxisRanges[1][1]-_pInfo.dSecAxisRanges[1][0]) + _pInfo.dRanges[1][0];
            }
        }
        else
        {
            StripSpaces(sDataLabels);
            if (_pData.getCoords())
                _mDataPlots[nTypeCounter[1]][0] = parser_fmod(_mDataPlots[nTypeCounter[1]][0], 2.0*M_PI);

            _mData = _mDataPlots[nTypeCounter[1]][1];
            _mPlotAxes = _mDataPlots[nTypeCounter[1]][0];
            if (_pData.getBoxplot()) ///
            {
                _mData2[0].Create(2);
                _mData2[0].a[0] = nTypeCounter[1]+0.5;
                _mData2[0].a[1] = nTypeCounter[1]+1.5;
                _mData = _mDataPlots[nTypeCounter[1]][1];
                _mData.Transpose();
            }
            else
            {
                if (_pData.getRegion() && vType.size() > nType+1 && vType[nType+1] == TYPE_DATA)
                {
                    _mData2[0] = _mDataPlots[nTypeCounter[1]+1][1];
                    nTypeCounter[1]++;
                }
                else if (_pData.getRegion() && vType.size() > nType+1 && vType[nType+1] == TYPE_FUNC)
                {
                    for (long int i = 0; i < _pInfo.nSamples; i++)
                        _mData2[0].a[i] = _pData.getData(i,nTypeCounter[0]);
                    nTypeCounter[0]++;
                }
                else
                {
                    _mData2[0].Create(getNN(_mData));
                    for (long int i = 0; i < getNN(_mData); i++)
                        _mData2[0].a[i] = 0.0;
                }
                if (_pData.getxError() && _pData.getyError() && nDataDim[nTypeCounter[1]] >= 4)
                {
                    _mData2[0] = _mDataPlots[nTypeCounter[1]][2];
                    _mData2[1] = _mDataPlots[nTypeCounter[1]][3];
                }
                else if ((_pData.getyError() || _pData.getxError()) && nDataDim[nTypeCounter[1]] >= 3)
                {
                    _mData2[0].Create(getNN(_mData));
                    _mData2[1].Create(getNN(_mData));
                    if (_pData.getyError() && !_pData.getxError())
                    {
                        _mData2[1] = _mDataPlots[nTypeCounter[1]][2];
                        for (long int i = 0; i < getNN(_mData); i++)
                            _mData2[0].a[i] = 0.0;
                    }
                    else if (_pData.getyError() && _pData.getxError())
                    {
                        _mData2[0] = _mDataPlots[nTypeCounter[1]][2];
                        _mData2[1] = _mDataPlots[nTypeCounter[1]][2];
                    }
                    else
                    {
                        _mData2[0] = _mDataPlots[nTypeCounter[1]][2];
                        for (long int i = 0; i < getNN(_mData); i++)
                            _mData2[1].a[i] = 0.0;
                    }
                }
            }
            if (_pData.getAxisbind(nType)[1] == 't')
            {
                for (int i = 0; i < getNN(_mPlotAxes); i++)
                {
                    if (_mPlotAxes.a[i] < _pInfo.dSecAxisRanges[0][0] || _mPlotAxes.a[i] > _pInfo.dSecAxisRanges[0][1])
                        _mPlotAxes.a[i] = NAN;
                }
                _mPlotAxes = (_mPlotAxes-_pInfo.dSecAxisRanges[0][0]) * (_pInfo.dRanges[0][1]-_pInfo.dRanges[0][0])/(_pInfo.dSecAxisRanges[0][1]-_pInfo.dSecAxisRanges[0][0]) + _pInfo.dRanges[0][0];
            }
            else
            {
                for (int i = 0; i < getNN(_mPlotAxes); i++)
                {
                    if (_mPlotAxes.a[i] < _pInfo.dRanges[0][0] || _mPlotAxes.a[i] > _pInfo.dRanges[0][1])
                        _mPlotAxes.a[i] = NAN;
                }
            }
            if (_pData.getAxisbind(nType)[0] == 'r')
            {
                _mData = (_mData-_pInfo.dSecAxisRanges[1][0]) * (_pInfo.dRanges[1][1]-_pInfo.dRanges[1][0])/(_pInfo.dSecAxisRanges[1][1]-_pInfo.dSecAxisRanges[1][0]) + _pInfo.dRanges[1][0];
            }
            if (_pData.getRegion() && _pData.getAxisbind(nType+1)[0] == 'r')
            {
                _mData2[0] = (_mData2[0]-_pInfo.dSecAxisRanges[1][0]) * (_pInfo.dRanges[1][1]-_pInfo.dRanges[1][0])/(_pInfo.dSecAxisRanges[1][1]-_pInfo.dSecAxisRanges[1][0]) + _pInfo.dRanges[1][0];
            }
            if (_pData.getyError() && _pData.getAxisbind(nType)[0] == 'r')
            {
                _mData2[1] = (_mData2[1]-_pInfo.dSecAxisRanges[1][0]) * (_pInfo.dRanges[1][1]-_pInfo.dRanges[1][0])/(_pInfo.dSecAxisRanges[1][1]-_pInfo.dSecAxisRanges[1][0]) + _pInfo.dRanges[1][0];
            }
            if (_pData.getxError() && _pData.getAxisbind(nType)[1] == 't')
            {
                _mData2[0] = (_mData2[0]-_pInfo.dSecAxisRanges[1][0]) *(_pInfo.dRanges[0][1]-_pInfo.dRanges[0][0])/(_pInfo.dSecAxisRanges[0][1]-_pInfo.dSecAxisRanges[0][0]) + _pInfo.dRanges[0][0];
            }

        }
        if (!parser_plot_std(_graph, _pData, _pInfo, _mData, _mPlotAxes, _mData2, vType[nType]))
        {
            // --> Den gibt's nicht: Speicher freigeben und zurueck! <--
            parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
            return;
        }
        if (vType[nType] == TYPE_FUNC)
        {
            if (_pData.getRegion() && vType.size() > nType+1)
            {
                for (int k = 0; k < 2; k++)
                {
                    nPos[0] = sLabels.find(';');
                    sConvLegends = sLabels.substr(0,nPos[0]) + " -nq";
                    parser_StringParser(sConvLegends, sDummy, _data, _parser, _option, true);
                    sConvLegends = "\"" + sConvLegends + "\"";
                    sLabels = sLabels.substr(nPos[0]+1);
                    if (sConvLegends != "\"\"")
                    {
                        _graph.AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sLineStyles[nStyle].c_str());
                        nLegends++;
                    }

                    if (nStyle == _pInfo.nStyleMax-1)
                        nStyle = 0;
                    else
                        nStyle++;
                }
            }
            else
            {
                nPos[0] = sLabels.find(';');
                sConvLegends = sLabels.substr(0,nPos[0]) + " -nq";
                parser_StringParser(sConvLegends, sDummy, _data, _parser, _option, true);
                sConvLegends = "\"" + sConvLegends + "\"";
                sLabels = sLabels.substr(nPos[0]+1);
                if (sConvLegends != "\"\"")
                {
                    _graph.AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), parser_getLegendStyle(sLineStyles[nStyle], _pData).c_str());
                    nLegends++;
                }
                if (nStyle == _pInfo.nStyleMax-1)
                    nStyle = 0;
                else
                    nStyle++;
            }
            nTypeCounter[0]++;
        }
        else
        {
            nPos[1] = sDataLabels.find(';');
            sConvLegends = sDataLabels.substr(0,nPos[1]);
            parser_StringParser(sConvLegends, sDummy, _data, _parser, _option, true);
            sDataLabels = sDataLabels.substr(nPos[1]+1);
            if (sConvLegends != "\"\"")
            {
                nLegends++;
                if (_pData.getBoxplot())
                {
                    _graph.AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), parser_getLegendStyle(sLineStyles[nStyle], _pData).c_str());
                }
                else if (!_pData.getxError() && !_pData.getyError())
                {
                    if ((_pData.getInterpolate() && _mDataPlots[nTypeCounter[1]][0].nx >= _pInfo.nSamples) || _pData.getBars())
                        _graph.AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), parser_getLegendStyle(sLineStyles[nStyle], _pData).c_str());
                    else if (_pData.getConnectPoints() || (_pData.getInterpolate() && _mDataPlots[nTypeCounter[1]][0].nx >= 0.9 * _pInfo.nSamples))
                        _graph.AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), parser_getLegendStyle(sConPointStyles[nStyle], _pData).c_str());
                    else
                        _graph.AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), parser_getLegendStyle(sPointStyles[nStyle], _pData).c_str());
                }
                else
                    _graph.AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), parser_getLegendStyle(sPointStyles[nStyle], _pData).c_str());
            }
            if (nStyle == _pInfo.nStyleMax-1)
                nStyle = 0;
            else
                nStyle++;

            nTypeCounter[1]++;
        }
        if (_pData.getRegion() && vType.size() > nType+1)
            nType++;
    }


    for (unsigned int i = 0; i < 3; i++)
    {
        if (_pData.getHLines(i).sDesc.length())
        {
            _graph.Line(mglPoint(_pInfo.dRanges[0][0], _pData.getHLines(i).dPos), mglPoint(_pInfo.dRanges[0][1], _pData.getHLines(i).dPos), _pData.getHLines(i).sStyle.c_str(), 100);
            if (!i || i > 1)
            {
                _graph.Puts(_pInfo.dRanges[0][0]+0.03*fabs(_pInfo.dRanges[0][0]-_pInfo.dRanges[0][1]), _pData.getHLines(i).dPos+0.01*fabs(_pInfo.dRanges[1][1]-_pInfo.dRanges[1][0]), fromSystemCodePage(_pData.getHLines(i).sDesc).c_str(), ":kL");
            }
            else
            {
                _graph.Puts(_pInfo.dRanges[0][0]+0.03*fabs(_pInfo.dRanges[0][0]-_pInfo.dRanges[0][1]), _pData.getHLines(i).dPos-0.04*fabs(_pInfo.dRanges[1][1]-_pInfo.dRanges[1][0]), fromSystemCodePage(_pData.getHLines(i).sDesc).c_str(), ":kL");
            }
        }
        if (_pData.getVLines(i).sDesc.length())
        {
            _graph.Line(mglPoint(_pData.getVLines(i).dPos, _pInfo.dRanges[1][0]), mglPoint(_pData.getVLines(i).dPos, _pInfo.dRanges[1][1]), _pData.getVLines(i).sStyle.c_str());
            _graph.Puts(mglPoint(_pData.getVLines(i).dPos-0.01*fabs(_pInfo.dRanges[0][0]-_pInfo.dRanges[0][1]), _pInfo.dRanges[1][0]+0.05*fabs(_pInfo.dRanges[1][0]-_pInfo.dRanges[1][1])), mglPoint(_pData.getVLines(i).dPos-0.01*fabs(_pInfo.dRanges[0][0]-_pInfo.dRanges[0][1]), _pInfo.dRanges[1][1]), fromSystemCodePage(_pData.getVLines(i).sDesc).c_str(), ":kL");
        }
    }
    if (nLegends && !_pData.getSchematic() && nPlotCompose+1 == nPlotComposeSize)
        _graph.Legend(_pData.getLegendPosition());
}

bool parser_plot_std(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, mglData& _mData, mglData& _mAxisVals, mglData _mData2[2], const short nType)
{
    if (nType == 1)
    {
        if (!_pData.getArea() && !_pData.getRegion())
            _graph.Plot(_mAxisVals, _mData, _pInfo.sLineStyles[*_pInfo.nStyle].c_str());
        else if (_pData.getRegion() && getNN(_mData2[0]))
        {
            if (*_pInfo.nStyle == _pInfo.nStyleMax-1)
                _graph.Region(_mAxisVals, _mData, _mData2[0], ("{"+_pData.getColors().substr(*_pInfo.nStyle,1) + "7}{" + _pData.getColors().substr(0,1)+"7}").c_str());
            else
                _graph.Region(_mAxisVals, _mData, _mData2[0], ("{"+_pData.getColors().substr(*_pInfo.nStyle,1) + "7}{" + _pData.getColors().substr(*_pInfo.nStyle+1,1)+"7}").c_str());
            _graph.Plot(_mAxisVals, _mData, _pInfo.sLineStyles[*_pInfo.nStyle].c_str());
            if (*_pInfo.nStyle == _pInfo.nStyleMax-1)
                _graph.Plot(_mAxisVals, _mData2[0], _pInfo.sLineStyles[0].c_str());
            else
                _graph.Plot(_mAxisVals, _mData2[0], _pInfo.sLineStyles[*_pInfo.nStyle+1].c_str());
        }
        else if (_pData.getArea() || _pData.getRegion())
            _graph.Area(_mAxisVals, _mData, (_pInfo.sLineStyles[*_pInfo.nStyle] + "{" + _pData.getColors()[*_pInfo.nStyle] + "9}").c_str());
    }
    else
    {

        if (_pData.getBoxplot())
        {
            _graph.BoxPlot(_mData2[0], _mData, _pInfo.sLineStyles[*_pInfo.nStyle].c_str());
        }
        else if (!_pData.getxError() && !_pData.getyError())
        {
            if (_pData.getInterpolate() && _mAxisVals.nx >= _pInfo.nSamples)
            {
                if (!_pData.getArea() && !_pData.getBars() && !_pData.getRegion())
                    _graph.Plot(_mAxisVals, _mData, _pInfo.sLineStyles[*_pInfo.nStyle].c_str());
                else if (_pData.getBars() && !_pData.getArea() && !_pData.getRegion())
                    _graph.Bars(_mAxisVals, _mData, (_pInfo.sLineStyles[*_pInfo.nStyle]+"^").c_str());
                else if (_pData.getArea() || _pData.getRegion())
                    _graph.Area(_mAxisVals, _mData, (_pInfo.sLineStyles[*_pInfo.nStyle] + "{" + _pData.getColors()[*_pInfo.nStyle] + "9}").c_str());
            }
            else if (_pData.getConnectPoints() || (_pData.getInterpolate() && _mAxisVals.nx >= 0.9 * _pInfo.nSamples))
            {
                if (!_pData.getArea() && !_pData.getBars())
                    _graph.Plot(_mAxisVals, _mData, _pInfo.sConPointStyles[*_pInfo.nStyle].c_str());
                else if (_pData.getBars() && !_pData.getArea())
                    _graph.Bars(_mAxisVals, _mData, (_pInfo.sLineStyles[*_pInfo.nStyle]+"^").c_str());
                else
                    _graph.Area(_mAxisVals, _mData, (_pInfo.sLineStyles[*_pInfo.nStyle] + "{" + _pData.getColors()[*_pInfo.nStyle] + "9}").c_str());
            }
            else
            {
                if (!_pData.getArea() && !_pData.getBars() && !_pData.getHBars() && !_pData.getStepplot())
                    _graph.Plot(_mAxisVals, _mData, _pInfo.sPointStyles[*_pInfo.nStyle].c_str());
                else if (_pData.getBars() && !_pData.getArea() && !_pData.getHBars() && !_pData.getStepplot())
                    _graph.Bars(_mAxisVals, _mData, (_pInfo.sLineStyles[*_pInfo.nStyle]+"^").c_str());
                else if (!_pData.getBars() && !_pData.getArea() && _pData.getHBars() && !_pData.getStepplot())
                    _graph.Barh(_mAxisVals, _mData, (_pInfo.sLineStyles[*_pInfo.nStyle]+"^").c_str());
                else if (!_pData.getBars() && !_pData.getArea() && !_pData.getHBars() && _pData.getStepplot())
                    _graph.Step(_mAxisVals, _mData, (_pInfo.sLineStyles[*_pInfo.nStyle]).c_str());
                else
                    _graph.Stem(_mAxisVals, _mData, _pInfo.sConPointStyles[*_pInfo.nStyle].c_str());
            }
        }
        else if (_pData.getxError() || _pData.getyError())
        {
            _graph.Error(_mAxisVals, _mData, _mData2[0], _mData2[1], _pInfo.sPointStyles[*_pInfo.nStyle].c_str());
        }
    }
    return true;
}

void parser_plot_create3dPlot(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, mglData** _mDataPlots, mglData _mAxisVals[2], const Settings& _option, string& sFunc, string& sLabels, int* nDataDim, int nDataPlots)
{
    if (sFunc != "<<empty>>")
    {
        mglData _mData(_pInfo.nSamples, _pInfo.nSamples, _pInfo.nSamples);
        mglData _mContVec(15);
        double dMin = _pData.getMin();
        double dMax = _pData.getMax();
        if (!isnan(_pData.getColorRange()))
        {
            dMin = _pData.getColorRange();
            dMax = _pData.getColorRange(1);
        }
        for (int nCont = 0; nCont < 15; nCont++)
        {
            _mContVec.a[nCont] = nCont*(dMax - dMin)/14.0+dMin;
            //cerr << _mContVec.a[nCont];
        }
        //cerr << endl;
        _mContVec.a[7] = (dMax-dMin)/2.0 + dMin;

        //int nPos = 0;
        StripSpaces(sLabels);

        for (long int i = 0; i < _pInfo.nSamples; i++)
        {
            for (long int j = 0; j < _pInfo.nSamples; j++)
            {
                for (long int k = 0; k < _pInfo.nSamples; k++)
                {
                    // --> Das Array im mglData-Objekt ist nur 1D vorhanden <--
                    _mData.a[i + _pInfo.nSamples*j + _pInfo.nSamples*_pInfo.nSamples*k] = _pData.getData(i,j,k);
                }
            }
        }

        if (_pData.getCutBox()
            && _pInfo.sCommand.substr(0,4) != "cont"
            && _pInfo.sCommand.substr(0,4) != "grad"
            && (_pInfo.sCommand.substr(0,4) != "dens" || (_pInfo.sCommand.substr(0,4) == "dens" && _pData.getCloudPlot())))
        {
            _graph.SetCutBox(parser_CalcCutBox(_pData.getRotateAngle(1), _pInfo.dRanges, 0, _pData.getCoords(), true), parser_CalcCutBox(_pData.getRotateAngle(1), _pInfo.dRanges, 1, _pData.getCoords(), true));
        }
        // --> Entsprechend dem gewuenschten Plotting-Style plotten <--
        if (_pInfo.sCommand.substr(0,4) == "mesh")
            _graph.Surf3(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme("#").c_str(), "value 11");
        else if (_pInfo.sCommand.substr(0,4) == "surf" && !_pData.getTransparency())
            _graph.Surf3(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme().c_str(), "value 11");
        else if (_pInfo.sCommand.substr(0,4) == "surf" && _pData.getTransparency())
            _graph.Surf3A(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _mData, _pData.getColorScheme().c_str(), "value 11");
        else if (_pInfo.sCommand.substr(0,4) == "cont")
        {
            if (_pData.getContProj())
            {
                if (_pData.getContFilled())
                {
                    _graph.ContFX(_mContVec, _mData.Sum("x"), _pData.getColorScheme().c_str(), parser_getProjBackground(_pData.getRotateAngle(1), _pInfo.dRanges));
                    _graph.ContFY(_mContVec, _mData.Sum("y"), _pData.getColorScheme().c_str(), parser_getProjBackground(_pData.getRotateAngle(1), _pInfo.dRanges, 1));
                    _graph.ContFZ(_mContVec, _mData.Sum("z"), _pData.getColorScheme().c_str(), _pInfo.dRanges[2][0]);
                    _graph.ContX(_mContVec, _mData.Sum("x"), "k", parser_getProjBackground(_pData.getRotateAngle(1), _pInfo.dRanges));
                    _graph.ContY(_mContVec, _mData.Sum("y"), "k", parser_getProjBackground(_pData.getRotateAngle(1), _pInfo.dRanges, 1));
                    _graph.ContZ(_mContVec, _mData.Sum("z"), "k", _pInfo.dRanges[2][0]);
                }
                else
                {
                    _graph.ContX(_mContVec, _mData.Sum("x"), _pData.getColorScheme().c_str(), parser_getProjBackground(_pData.getRotateAngle(1), _pInfo.dRanges));
                    _graph.ContY(_mContVec, _mData.Sum("y"), _pData.getColorScheme().c_str(), parser_getProjBackground(_pData.getRotateAngle(1), _pInfo.dRanges, 1));
                    _graph.ContZ(_mContVec, _mData.Sum("z"), _pData.getColorScheme().c_str(), _pInfo.dRanges[2][0]);
                }
            }
            else if (_pData.getContFilled())
            {
                for (unsigned short n = 0; n < _pData.getSlices(); n++)
                {
                    _graph.ContF3(_mContVec, _mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme("x").c_str(), (n+1)*_pInfo.nSamples/(_pData.getSlices()+1));
                    _graph.Cont3(_mContVec, _mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, "kx", (n+1)*_pInfo.nSamples/(_pData.getSlices()+1));
                }
                for (unsigned short n = 0; n < _pData.getSlices(1); n++)
                {
                    _graph.ContF3(_mContVec, _mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme().c_str(), (n+1)*_pInfo.nSamples/(_pData.getSlices(1)+1));
                    _graph.Cont3(_mContVec, _mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, "k", (n+1)*_pInfo.nSamples/(_pData.getSlices(1)+1));
                }
                for (unsigned short n = 0; n < _pData.getSlices(2); n++)
                {
                    _graph.ContF3(_mContVec, _mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme("z").c_str(), (n+1)*_pInfo.nSamples/(_pData.getSlices(2)+1));
                    _graph.Cont3(_mContVec, _mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, "kz", (n+1)*_pInfo.nSamples/(_pData.getSlices(2)+1));
                }
            }
            else
            {
                for (unsigned short n = 0; n < _pData.getSlices(); n++)
                    _graph.Cont3(_mContVec, _mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme("x").c_str(), (n+1)*_pInfo.nSamples/(_pData.getSlices()+1));
                for (unsigned short n = 0; n < _pData.getSlices(1); n++)
                    _graph.Cont3(_mContVec, _mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme().c_str(), (n+1)*_pInfo.nSamples/(_pData.getSlices(1)+1));
                for (unsigned short n = 0; n < _pData.getSlices(2); n++)
                    _graph.Cont3(_mContVec, _mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme("z").c_str(), (n+1)*_pInfo.nSamples/(_pData.getSlices(2)+1));
            }
        }
        else if (_pInfo.sCommand.substr(0,4) == "grad")
        {
            if (_pData.getHighRes() || !_option.getbUseDraftMode())
            {
                if (_pData.getContFilled() && _pData.getContProj())
                    _graph.Grad(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorSchemeMedium().c_str(), "value 10");
                else
                    _graph.Grad(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme().c_str(), "value 10");
            }
            else
            {
                if (_pData.getContFilled() && _pData.getContProj())
                    _graph.Grad(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorSchemeMedium().c_str());
                else
                    _graph.Grad(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme().c_str());
            }
            if (!(_pData.getContProj()))
            {
                _graph.Dens3(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorSchemeLight("x").c_str(), _pInfo.nSamples/2);
                _graph.Dens3(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorSchemeLight().c_str(), _pInfo.nSamples/2);
                _graph.Dens3(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorSchemeLight("z").c_str(), _pInfo.nSamples/2);
            }
        }
        else if (_pInfo.sCommand.substr(0,4) == "dens")
        {
            if (!(_pData.getContFilled() && _pData.getContProj()) && !_pData.getCloudPlot())
            {
                for (unsigned short n = 0; n < _pData.getSlices(); n++)
                    _graph.Dens3(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme("x").c_str(), (n+1)*_pInfo.nSamples/(_pData.getSlices()+1));
                for (unsigned short n = 0; n < _pData.getSlices(1); n++)
                    _graph.Dens3(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme().c_str(), (n+1)*_pInfo.nSamples/(_pData.getSlices(1)+1));
                for (unsigned short n = 0; n < _pData.getSlices(2); n++)
                    _graph.Dens3(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme("z").c_str(), (n+1)*_pInfo.nSamples/(_pData.getSlices(2)+1));
            }
            else if (_pData.getCloudPlot() && !(_pData.getContFilled() && _pData.getContProj()))
                _graph.Cloud(_mAxisVals[0], _mAxisVals[1], _mAxisVals[2], _mData, _pData.getColorScheme().c_str());
        }
        else
        {
            // --> Den gibt's nicht: Speicher freigeben und zurueck! <--
            parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
            return;
        }

        if (_pData.getCutBox()
            && _pInfo.sCommand.substr(0,4) != "cont"
            && _pInfo.sCommand.substr(0,4) != "grad"
            && (_pInfo.sCommand.substr(0,4) != "dens" || (_pInfo.sCommand.substr(0,4) == "dens" && _pData.getCloudPlot())))
        {
            _graph.SetCutBox(mglPoint(0), mglPoint(0));
        }

        // --> Ggf. Konturlinien ergaenzen <--
        if (_pData.getContProj() && _pInfo.sCommand.substr(0,4) != "cont")
        {
            if (_pData.getContFilled() && (_pInfo.sCommand.substr(0,4) != "dens" && _pInfo.sCommand.substr(0,4) != "grad"))
            {
                _graph.ContFX(_mContVec, _mData.Sum("x"), _pData.getColorScheme().c_str(), parser_getProjBackground(_pData.getRotateAngle(1), _pInfo.dRanges));
                _graph.ContFY(_mContVec, _mData.Sum("y"), _pData.getColorScheme().c_str(), parser_getProjBackground(_pData.getRotateAngle(1), _pInfo.dRanges, 1));
                _graph.ContFZ(_mContVec, _mData.Sum("z"), _pData.getColorScheme().c_str(), _pInfo.dRanges[2][0]);
                _graph.ContX(_mContVec, _mData.Sum("x"), "k", parser_getProjBackground(_pData.getRotateAngle(1), _pInfo.dRanges));
                _graph.ContY(_mContVec, _mData.Sum("y"), "k", parser_getProjBackground(_pData.getRotateAngle(1), _pInfo.dRanges, 1));
                _graph.ContZ(_mContVec, _mData.Sum("z"), "k", _pInfo.dRanges[2][0]);
            }
            else if ((_pInfo.sCommand.substr(0,4) == "dens" || _pInfo.sCommand.substr(0,4) == "grad") && _pData.getContFilled())
            {
                if (_pInfo.sCommand == "dens")
                {
                    _graph.DensX(_mData.Sum("x"), _pData.getColorScheme().c_str(), parser_getProjBackground(_pData.getRotateAngle(1), _pInfo.dRanges));
                    _graph.DensY(_mData.Sum("y"), _pData.getColorScheme().c_str(), parser_getProjBackground(_pData.getRotateAngle(1), _pInfo.dRanges, 1));
                    _graph.DensZ(_mData.Sum("z"), _pData.getColorScheme().c_str(), _pInfo.dRanges[2][0]);
                }
                else
                {
                    _graph.DensX(_mData.Sum("x"), _pData.getColorSchemeLight().c_str(), parser_getProjBackground(_pData.getRotateAngle(1), _pInfo.dRanges));
                    _graph.DensY(_mData.Sum("y"), _pData.getColorSchemeLight().c_str(), parser_getProjBackground(_pData.getRotateAngle(1), _pInfo.dRanges, 1));
                    _graph.DensZ(_mData.Sum("z"), _pData.getColorSchemeLight().c_str(), _pInfo.dRanges[2][0]);
                }
                _graph.ContX(_mContVec, _mData.Sum("x"), "k", parser_getProjBackground(_pData.getRotateAngle(1), _pInfo.dRanges));
                _graph.ContY(_mContVec, _mData.Sum("y"), "k", parser_getProjBackground(_pData.getRotateAngle(1), _pInfo.dRanges, 1));
                _graph.ContZ(_mContVec, _mData.Sum("z"), "k", _pInfo.dRanges[2][0]);
            }
            else
            {
                _graph.ContX(_mContVec, _mData.Sum("x"), _pData.getColorScheme().c_str(), parser_getProjBackground(_pData.getRotateAngle(1), _pInfo.dRanges));
                _graph.ContY(_mContVec, _mData.Sum("y"), _pData.getColorScheme().c_str(), parser_getProjBackground(_pData.getRotateAngle(1), _pInfo.dRanges, 1));
                _graph.ContZ(_mContVec, _mData.Sum("z"), _pData.getColorScheme().c_str(), _pInfo.dRanges[2][0]);
            }
        }
    }
}

void parser_plot_create3dVect(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, mglData** _mDataPlots, string& sFunc, string& sLabels, int* nDataDim, int nDataPlots)
{
    if (sFunc != "<<empty>>")
    {
        mglData _mData_x(_pInfo.nSamples, _pInfo.nSamples, _pInfo.nSamples);
        mglData _mData_y(_pInfo.nSamples, _pInfo.nSamples, _pInfo.nSamples);
        mglData _mData_z(_pInfo.nSamples, _pInfo.nSamples, _pInfo.nSamples);

        StripSpaces(sLabels);

        for (long int i = 0; i < _pInfo.nSamples; i++)
        {
            for (long int j = 0; j < _pInfo.nSamples; j++)
            {
                for (long int k = 0; k < _pInfo.nSamples; k++)
                {
                    // --> Das Array im mglData-Objekt ist nur 1D vorhanden <--
                    _mData_x.a[i + _pInfo.nSamples*j + _pInfo.nSamples*_pInfo.nSamples*k] = _pData.getData(i,j,3*k);
                    _mData_y.a[i + _pInfo.nSamples*j + _pInfo.nSamples*_pInfo.nSamples*k] = _pData.getData(i,j,3*k+1);
                    _mData_z.a[i + _pInfo.nSamples*j + _pInfo.nSamples*_pInfo.nSamples*k] = _pData.getData(i,j,3*k+2);
                }
            }
        }

        if (_pData.getCutBox())
        {
            _graph.SetCutBox(parser_CalcCutBox(_pData.getRotateAngle(1), _pInfo.dRanges, 0, _pData.getCoords(), true), parser_CalcCutBox(_pData.getRotateAngle(1), _pInfo.dRanges, 1, _pData.getCoords(), true));
        }
        // --> Entsprechend dem gewuenschten Plotting-Style plotten <--
        if (_pData.getFlow())
            _graph.Flow(_mData_x, _mData_y, _mData_z, _pData.getColorScheme("v").c_str());
        else if (_pData.getPipe())
            _graph.Pipe(_mData_x, _mData_y, _mData_z, _pData.getColorScheme().c_str());
        else if (_pData.getFixedLength())
            _graph.Vect(_mData_x, _mData_y, _mData_z, _pData.getColorScheme("f").c_str());
        else if (!_pData.getFixedLength())
            _graph.Vect(_mData_x, _mData_y, _mData_z, _pData.getColorScheme().c_str());
        else
        {
            // --> Den gibt's nicht: Speicher freigeben und zurueck! <--
            parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
            return;
        }

        if (_pData.getCutBox())
        {
            _graph.SetCutBox(mglPoint(0), mglPoint(0));
        }
    }
}

void parser_plot_create2dVect(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, mglData** _mDataPlots, string& sFunc, string& sLabels, int* nDataDim, int nDataPlots)
{
    if (sFunc != "<<empty>>")
    {
        mglData _mData_x(_pInfo.nSamples, _pInfo.nSamples);
        mglData _mData_y(_pInfo.nSamples, _pInfo.nSamples);

        StripSpaces(sLabels);

        for (long int i = 0; i < _pInfo.nSamples; i++)
        {
            for (long int j = 0; j < _pInfo.nSamples; j++)
            {
                // --> Das Array im mglData-Objekt ist nur 1D vorhanden <--
                _mData_x.a[i + _pInfo.nSamples*j] = _pData.getData(i,j,0);
                _mData_y.a[i + _pInfo.nSamples*j] = _pData.getData(i,j,1);
            }
        }

        // --> Entsprechend dem gewuenschten Plotting-Style plotten <--
        if (_pData.getFlow())
            _graph.Flow(_mData_x, _mData_y, _pData.getColorScheme("v").c_str());
        else if (_pData.getPipe())
            _graph.Pipe(_mData_x, _mData_y, _pData.getColorScheme().c_str());
        else if (_pData.getFixedLength())
            _graph.Vect(_mData_x, _mData_y, _pData.getColorScheme("f").c_str());
        else if (!_pData.getFixedLength())
            _graph.Vect(_mData_x, _mData_y, _pData.getColorScheme().c_str());
        else
        {
            // --> Den gibt's nicht: Speicher freigeben und zurueck! <--
            parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
            return;
        }
    }
}

void parser_plot_create2dDrawing(mglGraph& _graph, Parser& _parser, Datafile& _data, const Settings& _option, vector<string>& vDrawVector, value_type* vResults, int& nFunctions)
{
    string sStyle;
    string sTextString;
    string sDrawExpr;
    string sDummy;
    for (unsigned int v = 0; v < vDrawVector.size(); v++)
    {
        sStyle = "k";
        sTextString = "";
        sDrawExpr = "";
        if (containsStrings(vDrawVector[v]) || _data.containsStringVars(vDrawVector[v]))
        {
            for (int n = (int)vDrawVector[v].length()-1; n >= 0; n--)
            {
                if (vDrawVector[v][n] == ',' && !isInQuotes(vDrawVector[v], (unsigned)n, true))
                {
                    sStyle = vDrawVector[v].substr(n+1);
                    vDrawVector[v].erase(n);

                    break;
                }
            }
            sStyle = sStyle.substr(0,sStyle.rfind(')')) + " -nq";
            parser_StringParser(sStyle, sDummy, _data, _parser, _option, true);
        }
        if (containsStrings(vDrawVector[v]) || _data.containsStringVars(vDrawVector[v]))
        {
            for (int n = (int)vDrawVector[v].length()-1; n >= 0; n--)
            {
                if (vDrawVector[v][n] == ',' && !isInQuotes(vDrawVector[v], (unsigned)n, true))
                {
                    sTextString = vDrawVector[v].substr(n+1);
                    vDrawVector[v].erase(n);

                    break;
                }
            }
            sTextString += " -nq";
            parser_StringParser(sTextString, sDummy, _data, _parser, _option, true);
        }
        if (vDrawVector[v][vDrawVector[v].length()-1] == ')')
            sDrawExpr = vDrawVector[v].substr(vDrawVector[v].find('(')+1,vDrawVector[v].rfind(')')-vDrawVector[v].find('(')-1);
        else
            sDrawExpr = vDrawVector[v].substr(vDrawVector[v].find('(')+1);
        if (sDrawExpr.find('{') != string::npos)
            parser_VectorToExpr(sDrawExpr, _option);
        _parser.SetExpr(sDrawExpr);
        vResults = _parser.Eval(nFunctions);

        if (vDrawVector[v].substr(0,6) == "trace(")
        {
            if (nFunctions < 2)
                continue;
            if (nFunctions < 4)
                _graph.Line(mglPoint(),mglPoint(vResults[0], vResults[1]), sStyle.c_str());
            else
                _graph.Line(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,7) == "tracev(")
        {
            if (nFunctions < 2)
                continue;
            if (nFunctions < 4)
                _graph.Line(mglPoint(), mglPoint(vResults[0], vResults[1]), sStyle.c_str());
            else
                _graph.Line(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[0]+vResults[2], vResults[1]+vResults[3]), sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,5) == "face(" || vDrawVector[v].substr(0,7) == "cuboid(")
        {
            if (nFunctions < 4)
                continue;
            if (nFunctions < 6)
                _graph.Face(mglPoint(vResults[2]-vResults[3]+vResults[1], vResults[3]+vResults[2]-vResults[0]),
                            mglPoint(vResults[2],vResults[3]),
                            mglPoint(vResults[0]-vResults[3]+vResults[1], vResults[1]+vResults[2]-vResults[0]),
                            mglPoint(vResults[0],vResults[1]),
                            sStyle.c_str());
            else if (nFunctions < 8)
                _graph.Face(mglPoint(vResults[4],vResults[5]),
                            mglPoint(vResults[2],vResults[3]),
                            mglPoint(vResults[0]+vResults[4]-vResults[2], vResults[1]+vResults[5]-vResults[3]),
                            mglPoint(vResults[0],vResults[1]),
                            sStyle.c_str());
            else
                _graph.Face(mglPoint(vResults[4],vResults[5]),
                            mglPoint(vResults[2],vResults[3]),
                            mglPoint(vResults[6],vResults[7]),
                            mglPoint(vResults[0],vResults[1]),
                            sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,6) == "facev(")
        {
            if (nFunctions < 4)
                continue;
            if (nFunctions < 6)
                _graph.Face(mglPoint(vResults[0]+vResults[2]-vResults[3],vResults[1]+vResults[3]+vResults[2]),
                            mglPoint(vResults[0]+vResults[2],vResults[1]+vResults[3]),
                            mglPoint(vResults[0]-vResults[3],vResults[1]+vResults[2]),
                            mglPoint(vResults[0],vResults[1]),
                            sStyle.c_str());
            else if (nFunctions < 8)
                _graph.Face(mglPoint(vResults[0]+vResults[4]+vResults[2],vResults[1]+vResults[3]+vResults[5]),
                            mglPoint(vResults[0]+vResults[2],vResults[1]+vResults[3]),
                            mglPoint(vResults[0]+vResults[4],vResults[1]+vResults[5]),
                            mglPoint(vResults[0],vResults[1]),
                            sStyle.c_str());
            else
                _graph.Face(mglPoint(vResults[0]+vResults[4],vResults[1]+vResults[5]),
                            mglPoint(vResults[0]+vResults[2],vResults[1]+vResults[3]),
                            mglPoint(vResults[0]+vResults[6],vResults[1]+vResults[7]),
                            mglPoint(vResults[0],vResults[1]),
                            sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,9) == "triangle(")
        {
            if (nFunctions < 4)
                continue;
            double c = hypot(vResults[2]-vResults[0],vResults[3]-vResults[1])/2.0*sqrt(3)/hypot(vResults[2],vResults[3]);
            if (nFunctions < 6)
                _graph.Face(mglPoint((-vResults[0]+vResults[2])/2.0-c*vResults[3], (-vResults[1]+vResults[3])/2.0+c*vResults[2]),
                            mglPoint(vResults[2],vResults[3]),
                            mglPoint((-vResults[0]+vResults[2])/2.0-c*vResults[3], (-vResults[1]+vResults[3])/2.0+c*vResults[2]),
                            mglPoint(vResults[0],vResults[1]),
                            sStyle.c_str());
            else
                _graph.Face(mglPoint(vResults[4],vResults[5]),
                            mglPoint(vResults[2],vResults[3]),
                            mglPoint(vResults[4],vResults[5]),
                            mglPoint(vResults[0],vResults[1]),
                            sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,10) == "trianglev(")
        {
            if (nFunctions < 4)
                continue;
            double c = sqrt(3.0)/2.0;
            if (nFunctions < 6)
                _graph.Face(mglPoint((vResults[0]+0.5*vResults[2])-c*vResults[3], (vResults[1]+0.5*vResults[3])+c*vResults[2]),
                            mglPoint(vResults[0]+vResults[2],vResults[1]+vResults[3]),
                            mglPoint((vResults[0]+0.5*vResults[2])-c*vResults[3], (vResults[1]+0.5*vResults[3])+c*vResults[2]),
                            mglPoint(vResults[0],vResults[1]),
                            sStyle.c_str());
            else
                _graph.Face(mglPoint(vResults[0]+vResults[4],vResults[1]+vResults[5]),
                            mglPoint(vResults[0]+vResults[2],vResults[1]+vResults[3]),
                            mglPoint(vResults[0]+vResults[4],vResults[1]+vResults[5]),
                            mglPoint(vResults[0],vResults[1]),
                            sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,7) == "sphere(")
        {
            if (nFunctions < 3)
                continue;
            _graph.Sphere(mglPoint(vResults[0], vResults[1]), vResults[2], sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,5) == "drop(")
        {
            if (nFunctions < 5)
                continue;
            double dShift = 1;
            double dAspherity = 1;
            if (nFunctions >= 6)
                dShift = vResults[5];
            if (nFunctions >= 7)
                dAspherity = vResults[6];
            _graph.Drop(mglPoint(vResults[0], vResults[1]),
                        mglPoint(vResults[2], vResults[3]),
                        vResults[4],
                        sStyle.c_str(),
                        dShift,
                        dAspherity);
        }
        else if (vDrawVector[v].substr(0,7) == "circle(")
        {
            if (nFunctions < 3)
                continue;
            _graph.Circle(mglPoint(vResults[0], vResults[1]), vResults[2], sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,4) == "arc(")
        {
            if (nFunctions < 5)
                continue;
            _graph.Arc(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), vResults[4], sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,5) == "arcv(")
        {
            if (nFunctions < 5)
                continue;
            _graph.Arc(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2]+vResults[0], vResults[3]+vResults[1]), vResults[4], sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,6) == "point(")
        {
            if (nFunctions < 2)
                continue;
            _graph.Mark(mglPoint(vResults[0], vResults[1]), sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,6) == "curve(")
        {
            if (nFunctions < 8)
                continue;
            _graph.Curve(mglPoint(vResults[0], vResults[1]),
                        mglPoint(vResults[2], vResults[3]),
                        mglPoint(vResults[4], vResults[5]),
                        mglPoint(vResults[6], vResults[7]), sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,8) == "ellipse(")
        {
            if (nFunctions < 5)
                continue;
            _graph.Ellipse(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), vResults[4], sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,9) == "ellipsev(")
        {
            if (nFunctions < 5)
                continue;
            _graph.Ellipse(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2]+vResults[0], vResults[3]+vResults[1]), vResults[4], sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,5) == "text(")
        {
            if (!sTextString.length())
            {
                sTextString = sStyle;
                sStyle = "k";
            }
            if (nFunctions >= 4)
                _graph.Puts(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), sTextString.c_str(), sStyle.c_str());
            else if (nFunctions >= 2)
                _graph.Puts(mglPoint(vResults[0], vResults[1]), sTextString.c_str(), sStyle.c_str());
            else
                continue;
        }
        else if (vDrawVector[v].substr(0,8) == "polygon(")
        {
            if (nFunctions < 5 || vResults[4] < 3)
                continue;
            _graph.Polygon(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), (int)vResults[4], sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,9) == "polygonv(")
        {
            if (nFunctions < 5 || vResults[4] < 3)
                continue;
            _graph.Polygon(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2]+vResults[0], vResults[3]+vResults[1]), (int)vResults[4], sStyle.c_str());
        }
        else
            continue;
    }
}

void parser_plot_create3dDrawing(mglGraph& _graph, Parser& _parser, Datafile& _data, const Settings& _option, vector<string>& vDrawVector, value_type* vResults, int& nFunctions)
{
    string sStyle;
    string sTextString;
    string sDrawExpr;
    string sDummy;
    for (unsigned int v = 0; v < vDrawVector.size(); v++)
    {
        sStyle = "k";
        sTextString = "";
        sDrawExpr = "";
        if (containsStrings(vDrawVector[v]) || _data.containsStringVars(vDrawVector[v]))
        {
            for (int n = (int)vDrawVector[v].length()-1; n >= 0; n--)
            {
                if (vDrawVector[v][n] == ',' && !isInQuotes(vDrawVector[v], (unsigned)n, true))
                {
                    sStyle = vDrawVector[v].substr(n+1);
                    vDrawVector[v].erase(n);

                    break;
                }
            }
            sStyle = sStyle.substr(0,sStyle.rfind(')')) + " -nq";
            parser_StringParser(sStyle, sDummy, _data, _parser, _option, true);
        }
        if (containsStrings(vDrawVector[v]) || _data.containsStringVars(vDrawVector[v]))
        {
            for (int n = (int)vDrawVector[v].length()-1; n >= 0; n--)
            {
                if (vDrawVector[v][n] == ',' && !isInQuotes(vDrawVector[v], (unsigned)n, true))
                {
                    sTextString = vDrawVector[v].substr(n+1);
                    vDrawVector[v].erase(n);

                    break;
                }
            }
            sTextString += " -nq";
            parser_StringParser(sTextString, sDummy, _data, _parser, _option, true);
        }
        if (vDrawVector[v][vDrawVector[v].length()-1] == ')')
            sDrawExpr = vDrawVector[v].substr(vDrawVector[v].find('(')+1,vDrawVector[v].rfind(')')-vDrawVector[v].find('(')-1);
        else
            sDrawExpr = vDrawVector[v].substr(vDrawVector[v].find('(')+1);
        if (sDrawExpr.find('{') != string::npos)
            parser_VectorToExpr(sDrawExpr, _option);
        _parser.SetExpr(sDrawExpr);
        vResults = _parser.Eval(nFunctions);
        if (vDrawVector[v].substr(0,6) == "trace(")
        {
            if (nFunctions < 3)
                continue;
            if (nFunctions < 6)
                _graph.Line(mglPoint(), mglPoint(vResults[0], vResults[1], vResults[2]), sStyle.c_str());
            else
                _graph.Line(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,7) == "tracev(")
        {
            if (nFunctions < 3)
                continue;
            if (nFunctions < 6)
                _graph.Line(mglPoint(), mglPoint(vResults[0], vResults[1], vResults[2]), sStyle.c_str());
            else
                _graph.Line(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3]+vResults[0], vResults[4]+vResults[1], vResults[5]+vResults[2]), sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,5) == "face(")
        {
            if (nFunctions < 6)
                continue;
            if (nFunctions < 9)
                _graph.Face(mglPoint(vResults[3]-vResults[4]+vResults[1], vResults[4]+vResults[3]-vResults[0], vResults[5]),
                            mglPoint(vResults[0]+vResults[3],vResults[1]+vResults[4],vResults[2]+vResults[5]),
                            mglPoint(vResults[0]-vResults[4]+vResults[1], vResults[1]+vResults[3]-vResults[0], vResults[2]),
                            mglPoint(vResults[0],vResults[1],vResults[2]),
                            sStyle.c_str());
            else if (nFunctions < 12)
                _graph.Face(mglPoint(vResults[6],vResults[7],vResults[8]),
                            mglPoint(vResults[3],vResults[4],vResults[5]),
                            mglPoint(vResults[0]+vResults[6]-vResults[3],vResults[1]+vResults[7]-vResults[4],vResults[2]+vResults[8]-vResults[5]),
                            mglPoint(vResults[0],vResults[1],vResults[2]),
                            sStyle.c_str());
            else
                _graph.Face(mglPoint(vResults[6],vResults[7],vResults[8]),
                            mglPoint(vResults[3],vResults[4],vResults[5]),
                            mglPoint(vResults[9],vResults[10],vResults[11]),
                            mglPoint(vResults[0],vResults[1],vResults[2]),
                            sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,6) == "facev(")
        {
            if (nFunctions < 6)
                continue;
            if (nFunctions < 9)
                _graph.Face(mglPoint(vResults[0]+vResults[3]-vResults[4], vResults[1]+vResults[4]+vResults[3], vResults[5]+vResults[2]),
                            mglPoint(vResults[0]+vResults[3],vResults[1]+vResults[4], vResults[5]+vResults[2]),
                            mglPoint(vResults[0]-vResults[4],vResults[1]+vResults[3], vResults[2]),
                            mglPoint(vResults[0],vResults[1],vResults[2]),
                            sStyle.c_str());
            else if (nFunctions < 12)
                _graph.Face(mglPoint(vResults[0]+vResults[6]+vResults[3],vResults[1]+vResults[7]+vResults[4],vResults[2]+vResults[8]+vResults[5]),
                            mglPoint(vResults[0]+vResults[6],vResults[1]+vResults[4],vResults[2]+vResults[5]),
                            mglPoint(vResults[0]+vResults[3],vResults[1]+vResults[7],vResults[2]+vResults[8]),
                            mglPoint(vResults[0],vResults[1],vResults[2]),
                            sStyle.c_str());
            else
                _graph.Face(mglPoint(vResults[0]+vResults[6],vResults[1]+vResults[7],vResults[2]+vResults[8]),
                            mglPoint(vResults[0]+vResults[3],vResults[1]+vResults[4],vResults[2]+vResults[5]),
                            mglPoint(vResults[0]+vResults[9],vResults[1]+vResults[10],vResults[2]+vResults[11]),
                            mglPoint(vResults[0],vResults[1],vResults[2]),
                            sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,9) == "triangle(")
        {
            if (nFunctions < 6)
                continue;
            double c = sqrt((vResults[3]-vResults[0])*(vResults[3]-vResults[0])
                        +(vResults[4]-vResults[1])*(vResults[4]-vResults[1])
                        +(vResults[5]-vResults[2])*(vResults[5]-vResults[2]))/2.0*sqrt(3)/hypot(vResults[3],vResults[4]);
            if (nFunctions < 9)
                _graph.Face(mglPoint((-vResults[0]+vResults[3])/2.0-c*vResults[4], (-vResults[1]+vResults[4])/2.0+c*vResults[3], (vResults[5]+vResults[2])/2.0),
                            mglPoint(vResults[3],vResults[4],vResults[5]),
                            mglPoint((-vResults[0]+vResults[3])/2.0-c*vResults[4], (-vResults[1]+vResults[4])/2.0+c*vResults[3], (vResults[5]+vResults[2])/2.0),
                            mglPoint(vResults[0],vResults[1],vResults[2]),
                            sStyle.c_str());
            else
                _graph.Face(mglPoint(vResults[6],vResults[7],vResults[8]),
                            mglPoint(vResults[3],vResults[4],vResults[5]),
                            mglPoint(vResults[6],vResults[7],vResults[8]),
                            mglPoint(vResults[0],vResults[1],vResults[2]),
                            sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,10) == "trianglev(")
        {
            if (nFunctions < 6)
                continue;
            double c = sqrt((vResults[3])*(vResults[3])
                        +(vResults[4])*(vResults[4])
                        +(vResults[5])*(vResults[5]))/2.0*sqrt(3)/hypot(vResults[3],vResults[4]);
            if (nFunctions < 9)
                _graph.Face(mglPoint((vResults[0]+0.5*vResults[3])-c*vResults[4], (vResults[1]+0.5*vResults[4])+c*vResults[3], (vResults[5]+0.5*vResults[2])),
                            mglPoint(vResults[0]+vResults[3],vResults[1]+vResults[4],vResults[2]+vResults[5]),
                            mglPoint((vResults[0]+0.5*vResults[3])-c*vResults[4], (vResults[1]+0.5*vResults[4])+c*vResults[3], (vResults[5]+0.5*vResults[2])),
                            mglPoint(vResults[0],vResults[1],vResults[2]),
                            sStyle.c_str());
            else
                _graph.Face(mglPoint(vResults[0]+vResults[6],vResults[1]+vResults[7],vResults[2]+vResults[8]),
                            mglPoint(vResults[0]+vResults[3],vResults[1]+vResults[4],vResults[2]+vResults[5]),
                            mglPoint(vResults[0]+vResults[6],vResults[1]+vResults[7],vResults[2]+vResults[8]),
                            mglPoint(vResults[0],vResults[1],vResults[2]),
                            sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,7) == "cuboid(")
        {
            if (nFunctions < 6)
                continue;
            mglPoint _mDx;
            mglPoint _mDy;
            mglPoint _mDz;

            if (nFunctions < 9)
            {
                _mDx = mglPoint(vResults[3], vResults[4], vResults[5]);
                _mDy = mglPoint(-vResults[4], vResults[3], vResults[5]);
                _mDz = mglPoint(vResults[4]*vResults[5]-vResults[3]*vResults[5],
                                -vResults[4]*vResults[5]-vResults[3]*vResults[5],
                                vResults[3]*vResults[3]+vResults[4]*vResults[4])
                                    / sqrt(vResults[3]*vResults[3]+vResults[4]*vResults[4]+vResults[5]*vResults[5]);
            }
            else if (nFunctions < 12)
            {
                _mDx = mglPoint(vResults[3], vResults[4], vResults[5]);
                _mDy = mglPoint(vResults[6], vResults[7], vResults[8]);
                _mDz = mglPoint(vResults[4]*vResults[8]-vResults[7]*vResults[5],
                                vResults[6]*vResults[5]-vResults[3]*vResults[8],
                                vResults[3]*vResults[7]-vResults[6]*vResults[4]) * 2.0
                                    / (sqrt(vResults[3]*vResults[3]+vResults[4]*vResults[4]+vResults[5]*vResults[5])
                                        +sqrt(vResults[6]*vResults[6]+vResults[7]*vResults[7]+vResults[8]*vResults[8]));
            }
            else
            {
                _mDx = mglPoint(vResults[3], vResults[4], vResults[5]);
                _mDy = mglPoint(vResults[6], vResults[7], vResults[8]);
                _mDz = mglPoint(vResults[9], vResults[10], vResults[11]);
            }

            _graph.Face(mglPoint(vResults[0],vResults[1],vResults[2]),
                        mglPoint(vResults[0],vResults[1],vResults[2])+_mDx,
                        mglPoint(vResults[0],vResults[1],vResults[2])+_mDy,
                        mglPoint(vResults[0],vResults[1],vResults[2])+_mDx+_mDy,
                        sStyle.c_str());
            _graph.Face(mglPoint(vResults[0],vResults[1],vResults[2]),
                        mglPoint(vResults[0],vResults[1],vResults[2])+_mDx,
                        mglPoint(vResults[0],vResults[1],vResults[2])+_mDz,
                        mglPoint(vResults[0],vResults[1],vResults[2])+_mDz+_mDx,
                        sStyle.c_str());
            _graph.Face(mglPoint(vResults[0],vResults[1],vResults[2]),
                        mglPoint(vResults[0],vResults[1],vResults[2])+_mDy,
                        mglPoint(vResults[0],vResults[1],vResults[2])+_mDz,
                        mglPoint(vResults[0],vResults[1],vResults[2])+_mDy+_mDz,
                        sStyle.c_str());
            _graph.Face(mglPoint(vResults[0],vResults[1],vResults[2])+_mDz,
                        mglPoint(vResults[0],vResults[1],vResults[2])+_mDx+_mDz,
                        mglPoint(vResults[0],vResults[1],vResults[2])+_mDy+_mDz,
                        mglPoint(vResults[0],vResults[1],vResults[2])+_mDx+_mDy+_mDz,
                        sStyle.c_str());
            _graph.Face(mglPoint(vResults[0],vResults[1],vResults[2])+_mDy,
                        mglPoint(vResults[0],vResults[1],vResults[2])+_mDx+_mDy,
                        mglPoint(vResults[0],vResults[1],vResults[2])+_mDz+_mDy,
                        mglPoint(vResults[0],vResults[1],vResults[2])+_mDz+_mDx+_mDy,
                        sStyle.c_str());
            _graph.Face(mglPoint(vResults[0],vResults[1],vResults[2])+_mDx,
                        mglPoint(vResults[0],vResults[1],vResults[2])+_mDy+_mDx,
                        mglPoint(vResults[0],vResults[1],vResults[2])+_mDz+_mDx,
                        mglPoint(vResults[0],vResults[1],vResults[2])+_mDy+_mDz+_mDx,
                        sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,7) == "sphere(")
        {
            if (nFunctions < 4)
                continue;
            _graph.Sphere(mglPoint(vResults[0], vResults[1], vResults[2]), vResults[3], sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,5) == "cone(")
        {
            if (nFunctions < 7)
                continue;
            if (nFunctions >= 8)
                _graph.Cone(mglPoint(vResults[0],vResults[1],vResults[2]), mglPoint(vResults[3],vResults[4],vResults[5]), vResults[6], vResults[7], sStyle.c_str());
            else
                _graph.Cone(mglPoint(vResults[0],vResults[1],vResults[2]), mglPoint(vResults[3],vResults[4],vResults[5]), vResults[6], 0.0, sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,6) == "conev(")
        {
            if (nFunctions < 7)
                continue;
            if (nFunctions >= 8)
                _graph.Cone(mglPoint(vResults[0],vResults[1],vResults[2]), mglPoint(vResults[3],vResults[4],vResults[5])+mglPoint(vResults[0],vResults[1],vResults[2]), vResults[6], vResults[7], sStyle.c_str());
            else
                _graph.Cone(mglPoint(vResults[0],vResults[1],vResults[2]), mglPoint(vResults[3],vResults[4],vResults[5])+mglPoint(vResults[0],vResults[1],vResults[2]), vResults[6], 0.0, sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,5) == "drop(")
        {
            if (nFunctions < 7)
                continue;
            double dShift = 1;
            double dAspherity = 1;
            if (nFunctions >= 8)
                dShift = vResults[7];
            if (nFunctions >= 9)
                dAspherity = vResults[8];
            _graph.Drop(mglPoint(vResults[0], vResults[1], vResults[2]),
                        mglPoint(vResults[3], vResults[4], vResults[5]),
                        vResults[6],
                        sStyle.c_str(),
                        dShift,
                        dAspherity);
        }
        else if (vDrawVector[v].substr(0,7) == "circle(")
        {
            if (nFunctions < 4)
                continue;
            _graph.Circle(mglPoint(vResults[0], vResults[1], vResults[2]), vResults[3], sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,4) == "arc(")
        {
            if (nFunctions < 7)
                continue;
            if (nFunctions < 9)
                _graph.Arc(mglPoint(vResults[0], vResults[1], vResults[2]),
                            mglPoint(vResults[3], vResults[4], vResults[5]), vResults[6], sStyle.c_str());
            else
                _graph.Arc(mglPoint(vResults[0], vResults[1], vResults[2]),
                            mglPoint(vResults[3], vResults[4], vResults[5]),
                            mglPoint(vResults[6], vResults[7], vResults[8]),
                            vResults[9], sStyle.c_str());

        }
        else if (vDrawVector[v].substr(0,5) == "arcv(")
        {
            if (nFunctions < 7)
                continue;
            if (nFunctions < 9)
                _graph.Arc(mglPoint(vResults[0], vResults[1], vResults[2]),
                            mglPoint(vResults[3], vResults[4], vResults[5])+mglPoint(vResults[0],vResults[1],vResults[2]), vResults[6], sStyle.c_str());
            else
                _graph.Arc(mglPoint(vResults[0], vResults[1], vResults[2]),
                            mglPoint(vResults[3], vResults[4], vResults[5])+mglPoint(vResults[0],vResults[1],vResults[2]),
                            mglPoint(vResults[6], vResults[7], vResults[8])+mglPoint(vResults[0],vResults[1],vResults[2]),
                            vResults[9], sStyle.c_str());

        }
        else if (vDrawVector[v].substr(0,6) == "point(")
        {
            if (nFunctions < 3)
                continue;
            _graph.Mark(mglPoint(vResults[0], vResults[1], vResults[2]), sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,6) == "curve(")
        {
            if (nFunctions < 12)
                continue;
            _graph.Curve(mglPoint(vResults[0], vResults[1], vResults[2]),
                        mglPoint(vResults[3], vResults[4], vResults[5]),
                        mglPoint(vResults[6], vResults[7], vResults[8]),
                        mglPoint(vResults[9], vResults[10], vResults[11]), sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,8) == "ellipse(")
        {
            if (nFunctions < 7)
                continue;
            _graph.Ellipse(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), vResults[6], sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,9) == "ellipsev(")
        {
            if (nFunctions < 7)
                continue;
            _graph.Ellipse(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5])+mglPoint(vResults[0],vResults[1],vResults[2]), vResults[6], sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,5) == "text(")
        {
            if (!sTextString.length())
            {
                sTextString = sStyle;
                sStyle = "k";
            }
            if (nFunctions >= 6)
                _graph.Puts(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), sTextString.c_str(), sStyle.c_str());
            else if (nFunctions >= 3)
                _graph.Puts(mglPoint(vResults[0], vResults[1], vResults[2]), sTextString.c_str(), sStyle.c_str());
            else
                continue;
        }
        else if (vDrawVector[v].substr(0,8) == "polygon(")
        {
            if (nFunctions < 7 || vResults[6] < 3)
                continue;
            _graph.Polygon(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), (int)vResults[6], sStyle.c_str());
        }
        else if (vDrawVector[v].substr(0,9) == "polygonv(")
        {
            if (nFunctions < 7 || vResults[6] < 3)
                continue;
            _graph.Polygon(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5])+mglPoint(vResults[0],vResults[1],vResults[2]), (int)vResults[6], sStyle.c_str());
        }
        else
            continue;
    }
}

void parser_plot_createStd3dPlot(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, Datafile& _data, Parser& _parser, mglData** _mDataPlots, mglData _mAxisVals[2], const Settings& _option,
                            string& sLabels, string& sDataLabels, string* sLineStyles, string* sPointStyles, string* sContStyles, string* sConPointStyles, vector<short>& vType,
                            int& nStyle, size_t& nLegends, int* nDataDim, int nDataPlots, int nFunctions, size_t nPlotCompose, size_t nPlotComposeSize)
{
    string sDummy = "";
    string sConvLegends = "";
    const short TYPE_FUNC = 1;
    const short TYPE_DATA = -1;

    mglData _mData[3] = {mglData(_pInfo.nSamples), mglData(_pInfo.nSamples), mglData(_pInfo.nSamples)};
    mglData _mData2[3] = {mglData(_pInfo.nSamples), mglData(_pInfo.nSamples), mglData(_pInfo.nSamples)};
    int nLabels = 0;
    int nPos[2] = {0,0};
    int nTypeCounter[2] = {0,0};
    unsigned int nCurrentType;
    while (sLabels.find(';', nPos[0]) != string::npos)
    {
        nPos[0] = sLabels.find(';', nPos[0]) + 1;
        nLabels++;
        if (nPos[0] >= (int)sLabels.length())
            break;
    }

    nPos[0] = 0;
    if (nLabels > _pData.getLayers())
    {
        for (int i = 0; i < _pData.getLayers(); i++)
        {
            for (int j = 0; j < 2; j++)
            {
                if (nLabels == 1)
                    break;
                nPos[0] = sLabels.find(';', nPos[0]);
                sLabels = sLabels.substr(0,nPos[0]-1) + ", " + sLabels.substr(nPos[0]+2);
                nLabels--;
            }
            nPos[0] = sLabels.find(';', nPos[0]) + 1;
            nLabels--;
        }
        nPos[0] = 0;
    }
    if (_option.getbDebug())
        cerr << LineBreak("|-> DEBUG: sLabels = " + sLabels, _option) << endl;


    if (_pData.getCutBox())
        _graph.SetCutBox(parser_CalcCutBox(_pData.getRotateAngle(1), _pInfo.dRanges, 0, _pData.getCoords(), true), parser_CalcCutBox(_pData.getRotateAngle(1), _pInfo.dRanges, 1, _pData.getCoords(), true));


    for (unsigned int nType = 0; nType < vType.size(); nType++)
    {
        nCurrentType = nType;
        if (vType[nType] == TYPE_FUNC)
        {
            StripSpaces(sLabels);
            for (int i = 0; i < 3; i++)
            {
                _mData[i].Create(_pInfo.nSamples);
                _mData2[i].Create(_pInfo.nSamples);
            }

            for (long int i = 0; i < _pInfo.nSamples; i++)
            {
                for (int j = 0; j < 3; j++)
                {
                    _mData[j].a[i] = _pData.getData(i,j,nTypeCounter[0]);
                }
            }
            if (_pData.getCoords())
                _mData[1] = parser_fmod(_mData[1],2.0*M_PI);
            if (_pData.getCoords() == 2)
                _mData[2] = parser_fmod(_mData[2],1.0*M_PI);

            if (_pData.getRegion() && vType.size() > nType+3 && vType[nType+3] == TYPE_DATA)
            {
                _mData2[0] = _mDataPlots[nTypeCounter[1]][0];
                _mData2[1] = _mDataPlots[nTypeCounter[1]][1];
                _mData2[2] = _mDataPlots[nTypeCounter[1]][2];
                if (_pData.getCoords())
                    _mData2[1] = parser_fmod(_mData2[1],2.0*M_PI);
                if (_pData.getCoords() == 2)
                    _mData2[2] = parser_fmod(_mData2[2],1.0*M_PI);
                nTypeCounter[1]++;
                nType += 3;
            }
            else if (_pData.getRegion() && vType.size() > nType+3 && vType[nType+3] == TYPE_FUNC)
            {
                for (long int i = 0; i < _pInfo.nSamples; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        _mData2[j].a[i] = _pData.getData(i,j,nTypeCounter[0]+1);
                    }
                }
                if (_pData.getCoords())
                    _mData2[1] = parser_fmod(_mData2[1],2.0*M_PI);
                if (_pData.getCoords() == 2)
                    _mData2[2] = parser_fmod(_mData2[2],1.0*M_PI);
                nTypeCounter[0]++;
                nType++;
            }
            else
            {
                for (long int i = 0; i < _pInfo.nSamples; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        _mData2[j].a[i] = 0.0;
                    }
                }
            }
        }
        else
        {
            if (_pData.getCoords())
                _mDataPlots[nTypeCounter[1]][1] = parser_fmod(_mDataPlots[nTypeCounter[1]][1], 2.0*M_PI);
            if (_pData.getCoords() == 2)
                _mDataPlots[nTypeCounter[1]][2] = parser_fmod(_mDataPlots[nTypeCounter[1]][2], 1.0*M_PI);

            StripSpaces(sDataLabels);
            if (_pData.getxError() || _pData.getyError())
            {
                for (long int i = 0; i < _mDataPlots[nTypeCounter[1]][0].nx; i++)
                {
                    if (_mDataPlots[nTypeCounter[1]][0].a[i] < _pInfo.dRanges[0][0] || _mDataPlots[nTypeCounter[1]][0].a[i] > _pInfo.dRanges[0][1]
                        || _mDataPlots[nTypeCounter[1]][1].a[i] < _pInfo.dRanges[1][0] || _mDataPlots[nTypeCounter[1]][1].a[i] > _pInfo.dRanges[1][1]
                        || _mDataPlots[nTypeCounter[1]][1].a[i] < _pInfo.dRanges[2][0] || _mDataPlots[nTypeCounter[1]][1].a[i] > _pInfo.dRanges[2][1])
                    {
                        _mDataPlots[nTypeCounter[1]][0].a[i] = NAN;
                        _mDataPlots[nTypeCounter[1]][1].a[i] = NAN;
                        _mDataPlots[nTypeCounter[1]][2].a[i] = NAN;
                    }
                }
            }

            _mData[0] = _mDataPlots[nTypeCounter[1]][0];
            _mData[1] = _mDataPlots[nTypeCounter[1]][1];
            _mData[2] = _mDataPlots[nTypeCounter[1]][2];

            for (int j = 0; j < 3; j++)
            {
                for (int i = 0; i < getNN(_mData[j]); i++)
                {
                    if (_mData[j].a[i] < _pInfo.dRanges[j][0] || _mData[j].a[i] > _pInfo.dRanges[j][1])
                        _mData[j].a[i] = NAN;
                }
            }

            if (_pData.getRegion() && vType.size() > nType+1 && vType[nType+1] == TYPE_DATA)
            {
                _mData2[0] = _mDataPlots[nTypeCounter[1]+1][0];
                _mData2[1] = _mDataPlots[nTypeCounter[1]+1][1];
                _mData2[2] = _mDataPlots[nTypeCounter[1]+1][2];
                if (_pData.getCoords())
                    _mData2[1] = parser_fmod(_mData2[1],2.0*M_PI);
                if (_pData.getCoords() == 2)
                    _mData2[2] = parser_fmod(_mData2[2],1.0*M_PI);
                nTypeCounter[1]++;
                nType++;
            }
            else if (_pData.getRegion() && vType.size() > nType+1 && vType[nType+1] == TYPE_FUNC)
            {
                for (int j = 0; j < 3; j++)
                    _mData2[j].Create(_pInfo.nSamples);
                for (long int i = 0; i < _pInfo.nSamples; i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        _mData2[j].a[i] = _pData.getData(i,j,nTypeCounter[0]);
                    }
                }
                if (_pData.getCoords())
                    _mData2[1] = parser_fmod(_mData2[1],2.0*M_PI);
                if (_pData.getCoords() == 2)
                    _mData2[2] = parser_fmod(_mData2[2],1.0*M_PI);
                nTypeCounter[0]++;
                nType+=3;
            }
            else if (_pData.getRegion())
            {
                for (int j = 0; j < 3; j++)
                    _mData2[j].Create(getNN(_mData[0]));
                for (long int i = 0; i < getNN(_mData[0]); i++)
                {
                    for (int j = 0; j < 3; j++)
                    {
                        _mData2[j].a[i] = 0.0;
                    }
                }
            }

            if (_pData.getxError() && _pData.getyError())
            {
                _mData2[0] = _mDataPlots[nTypeCounter[1]][3];
                _mData2[1] = _mDataPlots[nTypeCounter[1]][4];
                _mData2[2] = _mDataPlots[nTypeCounter[1]][5];
            }
        }
        if (!parser_plot_std3d(_graph, _pData, _pInfo, _mData, _mData2, vType[nCurrentType]))
        {
            // --> Den gibt's nicht: Speicher freigeben und zurueck! <--
            if (_mDataPlots)
            {
                for (int i = 0; i < nDataPlots; i++)
                    delete[] _mDataPlots[i];
                delete[] _mDataPlots;
                _mDataPlots = 0;
                delete[] nDataDim;
                nDataDim = 0;
            }
            return;
        }
        if (vType[nCurrentType] == TYPE_FUNC)
        {
            if (_pData.getRegion() && vType.size() > nCurrentType+1)
            {
                for (int k = 0; k < 2; k++)
                {
                    nPos[0] = sLabels.find(';');
                    sConvLegends = sLabels.substr(0,nPos[0]) + " -nq";
                    parser_StringParser(sConvLegends, sDummy, _data, _parser, _option, true);
                    sConvLegends = "\"" + sConvLegends + "\"";
                    for (unsigned int l = 0; l < sConvLegends.length(); l++)
                    {
                        if (sConvLegends[l] == '(')
                            l += getMatchingParenthesis(sConvLegends.substr(l));
                        if (sConvLegends[l] == ',')
                        {
                            sConvLegends = "\"[" + sConvLegends.substr(1,sConvLegends.length()-2) + "]\"";
                            break;
                        }
                    }
                    sLabels = sLabels.substr(nPos[0]+1);
                    if (sConvLegends != "\"\"")
                    {
                        _graph.AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sLineStyles[nStyle].c_str());
                        nLegends++;
                    }

                    if (nStyle == _pInfo.nStyleMax-1)
                        nStyle = 0;
                    else
                        nStyle++;
                }
            }
            else
            {
                nPos[0] = sLabels.find(';');
                sConvLegends = sLabels.substr(0,nPos[0]) + " -nq";
                parser_StringParser(sConvLegends, sDummy, _data, _parser, _option, true);
                sConvLegends = "\"" + sConvLegends + "\"";
                sLabels = sLabels.substr(nPos[0]+1);
                if (sConvLegends != "\"\"")
                {
                    _graph.AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), parser_getLegendStyle(sLineStyles[nStyle], _pData).c_str());
                    nLegends++;
                }
                if (nStyle == _pInfo.nStyleMax-1)
                    nStyle = 0;
                else
                    nStyle++;
            }
            nTypeCounter[0]++;
            nType += 2;
        }
        else
        {
            nPos[1] = sDataLabels.find(';');
            sConvLegends = sDataLabels.substr(0,nPos[1]);
            parser_StringParser(sConvLegends, sDummy, _data, _parser, _option, true);
            sDataLabels = sDataLabels.substr(nPos[1]+1);
            if (sConvLegends != "\"\"")
            {
                nLegends++;
                if (!_pData.getxError() && !_pData.getyError())
                {
                    if ((_pData.getInterpolate() && _mDataPlots[nTypeCounter[1]][0].nx >= _pInfo.nSamples) || _pData.getBars())
                        _graph.AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), parser_getLegendStyle(sLineStyles[nStyle], _pData).c_str());
                    else if (_pData.getConnectPoints() || (_pData.getInterpolate() && _mDataPlots[nTypeCounter[1]][0].nx >= 0.9 * _pInfo.nSamples))
                        _graph.AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), parser_getLegendStyle(sConPointStyles[nStyle], _pData).c_str());
                    else
                        _graph.AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), parser_getLegendStyle(sPointStyles[nStyle], _pData).c_str());
                }
                else
                    _graph.AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), parser_getLegendStyle(sPointStyles[nStyle], _pData).c_str());
            }
            if (nStyle == _pInfo.nStyleMax-1)
                nStyle = 0;
            else
                nStyle++;

            nTypeCounter[1]++;
        }
    }


    if (_pData.getCutBox())
        _graph.SetCutBox(mglPoint(0), mglPoint(0));
    if (!((_pData.getMarks() || _pData.getCrust()) && _pInfo.sCommand.substr(0,6) == "plot3d") && nLegends && !_pData.getSchematic() && nPlotCompose+1 == nPlotComposeSize)
    {
        if (_pData.getRotateAngle() || _pData.getRotateAngle(1))
            _graph.Legend(1.35,1.2);
        else
            _graph.Legend(_pData.getLegendPosition());
    }
}

bool parser_plot_std3d(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, mglData _mData[3], mglData _mData2[3], const short nType)
{
    if (nType == 1)
    {
        if (!_pData.getArea() && !_pData.getRegion())
            _graph.Plot(_mData[0], _mData[1], _mData[2], _pInfo.sLineStyles[*_pInfo.nStyle].c_str());
        else if (_pData.getRegion() && getNN(_mData2[0]))
        {
            if (*_pInfo.nStyle == _pInfo.nStyleMax-1)
                _graph.Region(_mData[0], _mData[1], _mData[2], _mData2[0], _mData2[1], _mData2[2], ("{"+_pData.getColors().substr(*_pInfo.nStyle,1) + "7}{" + _pData.getColors().substr(0,1)+"7}").c_str());
            else
                _graph.Region(_mData[0], _mData[1], _mData[2], _mData2[0], _mData2[1], _mData2[2], ("{"+_pData.getColors().substr(*_pInfo.nStyle,1) + "7}{" + _pData.getColors().substr(*_pInfo.nStyle+1,1)+"7}").c_str());
            _graph.Plot(_mData[0], _mData[1], _mData[2], _pInfo.sLineStyles[*_pInfo.nStyle].c_str());
            if (*_pInfo.nStyle == _pInfo.nStyleMax-1)
                _graph.Plot(_mData2[0], _mData2[1], _mData2[2], _pInfo.sLineStyles[0].c_str());
            else
                _graph.Plot(_mData2[0], _mData2[1], _mData2[2], _pInfo.sLineStyles[*_pInfo.nStyle+1].c_str());
        }
        else
            _graph.Area(_mData[0], _mData[1], _mData[2], (_pInfo.sLineStyles[*_pInfo.nStyle] + "{" + _pData.getColors()[*_pInfo.nStyle] + "9}").c_str());
    }
    else
    {
        if (!_pData.getxError() && !_pData.getyError())
        {
            // --> Interpolate-Schalter. Siehe weiter oben fuer Details <--
            if (_pData.getInterpolate() && _mData[0].nx >= _pInfo.nSamples)
            {
                if (!_pData.getArea() && !_pData.getBars() && !_pData.getRegion())
                    _graph.Plot(_mData[0], _mData[1], _mData[2], _pInfo.sLineStyles[*_pInfo.nStyle].c_str());
                else if (_pData.getBars() && !_pData.getArea() && !_pData.getRegion())
                    _graph.Bars(_mData[0], _mData[1], _mData[2], (_pInfo.sLineStyles[*_pInfo.nStyle] + "^").c_str());
                /*else if (_pData.getRegion() && j+1 < nDataPlots)
                {
                    if (nStyle == nStyleMax-1)
                        _graph.Region(_mData[0], _mData[1], _mData[2], _mData2[0], _mData2[1], _mData2[2], ("{"+_pData.getColors().substr(nStyle,1) +"7}{"+ _pData.getColors().substr(0,1)+"7}").c_str());
                    else
                        _graph.Region(_mData[0], _mData[1], _mData[2], _mData2[0], _mData2[1], _mData2[2], ("{"+_pData.getColors().substr(nStyle,1) +"7}{"+ _pData.getColors().substr(nStyle+1,1)+"7}").c_str());
                    _graph.Plot(_mData[0], _mData[1], _mData[2], sLineStyles[nStyle].c_str());
                    j++;
                    if (nStyle == nStyleMax-1)_mData
                        _graph.Plot(_mData[0], _mData[1], _mData[2], sLineStyles[0].c_str());
                    else
                        _graph.Plot(_mData[0], _mData[1], _mData[2], sLineStyles[nStyle+1].c_str());
                }*/
                else
                    _graph.Area(_mData[0], _mData[1], _mData[2], (_pInfo.sLineStyles[*_pInfo.nStyle] + "{" + _pData.getColors()[*_pInfo.nStyle] + "9}").c_str());
            }
            else if (_pData.getConnectPoints() || (_pData.getInterpolate() && _mData[0].nx >= 0.9*_pInfo.nSamples))
            {
                if (!_pData.getArea() && !_pData.getBars())
                    _graph.Plot(_mData[0], _mData[1], _mData[2], _pInfo.sConPointStyles[*_pInfo.nStyle].c_str());
                else if (_pData.getBars() && !_pData.getArea())
                    _graph.Bars(_mData[0], _mData[1], _mData[2], (_pInfo.sLineStyles[*_pInfo.nStyle]+"^").c_str());
                else
                    _graph.Area(_mData[0], _mData[1], _mData[2], (_pInfo.sLineStyles[*_pInfo.nStyle] + "{" + _pData.getColors()[*_pInfo.nStyle] + "9}").c_str());
            }
            else
            {
                if (!_pData.getArea() && !_pData.getMarks() && !_pData.getBars() && !_pData.getStepplot() && !_pData.getCrust())
                    _graph.Plot(_mData[0], _mData[1], _mData[2], _pInfo.sPointStyles[*_pInfo.nStyle].c_str());
                else if (_pData.getMarks() && !_pData.getCrust() && !_pData.getBars() && !_pData.getArea() && !_pData.getStepplot())
                    _graph.Dots(_mData[0], _mData[1], _mData[2], _pData.getColorScheme(toString(_pData.getMarks())).c_str());
                else if (_pData.getCrust() && !_pData.getMarks() && !_pData.getBars() && !_pData.getArea() && !_pData.getStepplot())
                    _graph.Crust(_mData[0], _mData[1], _mData[2], _pData.getColorScheme().c_str());
                else if (_pData.getBars() && !_pData.getArea() && !_pData.getMarks() && !_pData.getStepplot() && !_pData.getCrust())
                    _graph.Bars(_mData[0], _mData[1], _mData[2], (_pInfo.sLineStyles[*_pInfo.nStyle]+"^").c_str());
                else if (!_pData.getBars() && !_pData.getArea() && !_pData.getMarks() && _pData.getStepplot() && !_pData.getCrust())
                    _graph.Step(_mData[0], _mData[1], _mData[2], (_pInfo.sLineStyles[*_pInfo.nStyle]).c_str());
                else
                    _graph.Stem(_mData[0], _mData[1], _mData[2], _pInfo.sConPointStyles[*_pInfo.nStyle].c_str());
            }
        }
        else if (_pData.getxError() || _pData.getyError())
        {
            for (int m = 0; m < _mData[0].nx; m++)
            {
                _graph.Error(mglPoint(_mData[0].a[m], _mData[1].a[m], _mData[2].a[m]), mglPoint(_mData2[0].a[m], _mData2[1].a[m], _mData2[2].a[m]), _pInfo.sPointStyles[*_pInfo.nStyle].c_str());
            }
        }
    }
    return true;
}

long getNN(const mglData& _mData)
{
    return _mData.nx * _mData.ny * _mData.nz;
}

void parser_plot_evaluatePlotParamString(PlotInfo& _pInfo, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
    string sDummy;
    if (_pInfo.sPlotParams.find("??") != string::npos)
    {
        _pInfo.sPlotParams = parser_Prompt(_pInfo.sPlotParams);
    }
    if (!_functions.call(_pInfo.sPlotParams, _option))
        throw FUNCTION_ERROR;
    if ((containsStrings(_pInfo.sPlotParams) || _data.containsStringVars(_pInfo.sPlotParams))
        && _pInfo.sPlotParams.find('=') != string::npos)
    {
        unsigned int nPos = 0;
        if (_data.containsStringVars(_pInfo.sPlotParams))
            _data.getStringValues(_pInfo.sPlotParams);
        while (_pInfo.sPlotParams.find('=', nPos) != string::npos)
        {
            nPos = _pInfo.sPlotParams.find('=', nPos)+1;
            if (nPos >= _pInfo.sPlotParams.length())
                break;
            while (_pInfo.sPlotParams[nPos] == ' ')
                nPos++;
            if ((_pInfo.sPlotParams[nPos] != '"'
                    && _pInfo.sPlotParams[nPos] != '#'
                    && _pInfo.sPlotParams[nPos] != '('
                    && _pInfo.sPlotParams.substr(nPos, 10) != "to_string("
                    && _pInfo.sPlotParams.substr(nPos, 12) != "string_cast("
                    && _pInfo.sPlotParams.substr(nPos, 8) != "to_char("
                    && _pInfo.sPlotParams.substr(nPos, 8) != "replace("
                    && _pInfo.sPlotParams.substr(nPos, 11) != "replaceall("
                    && _pInfo.sPlotParams.substr(nPos, 5) != "char("
                    && _pInfo.sPlotParams.substr(nPos, 7) != "string("
                    && _pInfo.sPlotParams.substr(nPos, 7) != "substr("
                    && _pInfo.sPlotParams.substr(nPos, 6) != "split("
                    && _pInfo.sPlotParams.substr(nPos, 6) != "sum("
                    && _pInfo.sPlotParams.substr(nPos, 6) != "min("
                    && _pInfo.sPlotParams.substr(nPos, 6) != "max("
                    && _pInfo.sPlotParams.substr(nPos, 5) != "data("
                    && !_data.containsCacheElements(_pInfo.sPlotParams.substr(nPos)))
                || isInQuotes(_pInfo.sPlotParams, nPos-1))
                continue;
            if (_data.containsCacheElements(_pInfo.sPlotParams.substr(nPos)))
            {
                bool bMatch = false;
                for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
                {
                    if (_pInfo.sPlotParams.substr(nPos, (iter->first).length()+1) == iter->first+"(")
                    {
                        bMatch = true;
                        break;
                    }
                }
                if (!bMatch)
                    continue;
            }
            if (_pInfo.sPlotParams.substr(nPos,4) == "min(" || _pInfo.sPlotParams.substr(nPos,4) == "max(" || _pInfo.sPlotParams.substr(nPos,4) == "sum(")
            {
                int nPos_temp = getMatchingParenthesis(_pInfo.sPlotParams.substr(nPos+3))+nPos+3;
                if (!containsStrings(_pInfo.sPlotParams.substr(nPos+3, nPos_temp-nPos-3)) && !_data.containsStringVars(_pInfo.sPlotParams.substr(nPos+3, nPos_temp-nPos-3)))
                    continue;
            }
            for (unsigned int i = nPos; i < _pInfo.sPlotParams.length(); i++)
            {
                if (_pInfo.sPlotParams[i] == '(')
                {
                    i += getMatchingParenthesis(_pInfo.sPlotParams.substr(i));
                    continue;
                }
                if (((_pInfo.sPlotParams[i] == ' ' || _pInfo.sPlotParams[i] == ')') && !isInQuotes(_pInfo.sPlotParams, i)) || i+1 == _pInfo.sPlotParams.length())
                {
                    string sParsedString;
                    if (i+1 == _pInfo.sPlotParams.length())
                        sParsedString = _pInfo.sPlotParams.substr(nPos);
                    else
                        sParsedString = _pInfo.sPlotParams.substr(nPos, i-nPos);
                    if (!parser_StringParser(sParsedString, sDummy, _data, _parser, _option, true))
                    {
                        throw STRING_ERROR;
                    }
                    if (_pInfo.sPlotParams[nPos] == '(' && sParsedString.front() != '(')
                    {
                        sParsedString = "(" + sParsedString + ")";
                    }
                    if (i+1 == _pInfo.sPlotParams.length())
                        _pInfo.sPlotParams.replace(nPos, string::npos, sParsedString);
                    else
                        _pInfo.sPlotParams.replace(nPos, i-nPos, sParsedString);
                    break;
                }
            }
        }
    }
}

void parser_plot_filename(PlotData& _pData, PlotInfo& _pInfo, Datafile& _data, Parser& _parser, Settings& _option, size_t nPlotComposeSize, size_t nPlotCompose)
{
    // --> Ggf. waehlen eines Default-Dateinamens <--
    if (!_pData.getFileName().length() && !nPlotCompose)
    {
        string sExt = ".png";
        if (_pData.getAnimateSamples())
            sExt = ".gif";
        if (nPlotComposeSize > 1)
            _pData.setFileName("composition"+sExt);
        else if (_pInfo.sCommand == "plot3d")
            _pData.setFileName("plot3d"+sExt);
        else if (_pInfo.sCommand == "plot")
            _pData.setFileName("plot"+sExt);
        else if (_pInfo.sCommand == "meshgrid3d" || _pInfo.sCommand.substr(0,6) == "mesh3d")
            _pData.setFileName("meshgrid3d"+sExt);
        else if (_pInfo.sCommand.substr(0,4) == "mesh")
            _pData.setFileName("meshgrid"+sExt);
        else if (_pInfo.sCommand.substr(0,6) == "surf3d" || _pInfo.sCommand == "surface3d")
            _pData.setFileName("surface3d"+sExt);
        else if (_pInfo.sCommand.substr(0,4) == "surf")
            _pData.setFileName("surface"+sExt);
        else if (_pInfo.sCommand.substr(0,6) == "cont3d" || _pInfo.sCommand == "contour3d")
            _pData.setFileName("contour3d"+sExt);
        else if (_pInfo.sCommand.substr(0.4) == "cont")
            _pData.setFileName("contour"+sExt);
        else if (_pInfo.sCommand.substr(0,6) == "grad3d" || _pInfo.sCommand == "gradient3d")
            _pData.setFileName("gradient3d"+sExt);
        else if (_pInfo.sCommand.substr(0,4) == "grad")
            _pData.setFileName("gradient"+sExt);
        else if (_pInfo.sCommand == "density3d" || _pInfo.sCommand.substr(0,6) == "dens3d")
            _pData.setFileName("density3d"+sExt);
        else if (_pInfo.sCommand.substr(0,4) == "dens")
            _pData.setFileName("density"+sExt);
        else if (_pInfo.sCommand.substr(0,6) == "vect3d" || _pInfo.sCommand == "vector3d")
            _pData.setFileName("vectorfield3d"+sExt);
        else if (_pInfo.sCommand.substr(0,4) == "vect")
            _pData.setFileName("vectorfield"+sExt);
        else if (_pInfo.sCommand.substr(0,6) == "draw3d")
            _pData.setFileName("drawing3d"+sExt);
        else if (_pInfo.sCommand.substr(0,4) == "draw")
            _pData.setFileName("drawing"+sExt);
        else
            _pData.setFileName("unknown_style"+sExt);
    }
    if ((containsStrings(_pData.getFileName()) || _data.containsStringVars(_pData.getFileName())) && !nPlotCompose)
    {
        string sTemp = _pData.getFileName();
        string sTemp_2 = "";
        string sExtension = sTemp.substr(sTemp.find('.'));
        sTemp = sTemp.substr(0,sTemp.find('.'));
        if (sExtension[sExtension.length()-1] == '"')
        {
            sTemp += "\"";
            sExtension = sExtension.substr(0,sExtension.length()-1);
        }
        parser_StringParser(sTemp, sTemp_2, _data, _parser, _option, true);
        _pData.setFileName(sTemp.substr(1,sTemp.length()-2)+sExtension);
    }
    if (_pData.getAnimateSamples() && _pData.getFileName().substr(_pData.getFileName().rfind('.')) != ".gif" && !nPlotCompose)
        _pData.setFileName(_pData.getFileName().substr(0, _pData.getFileName().length()-4) + ".gif");
}

void parser_plot_setStyles(PlotData& _pData, PlotInfo& _pInfo, string* sLineStyles, string* sPointStyles, string* sContStyles, string* sConPointStyles)
{
    for (int i = 0; i < _pInfo.nStyleMax; i++)
    {
        sLineStyles[i] = " ";
        sLineStyles[i][0] = _pData.getColors()[i];
        sPointStyles[i] = " ";
        sPointStyles[i][0] = _pData.getColors()[i];
        sContStyles[i] = " ";
        sContStyles[i][0] = _pData.getContColors()[i];
        sConPointStyles[i] = " ";
        sConPointStyles[i][0] = _pData.getColors()[i];
    }
    for (int i = 0; i < _pInfo.nStyleMax; i++)
    {
        sLineStyles[i] += _pData.getLineStyles()[i];
        if (_pData.getDrawPoints())
        {
            if (_pData.getPointStyles()[2*i] != ' ')
                sLineStyles[i] += _pData.getPointStyles()[2*i];
            sLineStyles[i] += _pData.getPointStyles()[2*i+1];
        }
        if (_pData.getyError() || _pData.getxError())
        {
            if (_pData.getPointStyles()[2*i] != ' ')
                sPointStyles[i] += _pData.getPointStyles()[2*i];
            sPointStyles[i] += _pData.getPointStyles()[2*i+1];
        }
        else
        {
            if (_pData.getPointStyles()[2*i] != ' ')
                sPointStyles[i] += " " + _pData.getPointStyles().substr(2*i,1) + _pData.getPointStyles().substr(2*i+1,1);
            else
                sPointStyles[i] += " " + _pData.getPointStyles().substr(2*i+1,1);
        }
        if (_pData.getPointStyles()[2*i] != ' ')
            sConPointStyles[i] += _pData.getLineStyles().substr(i,1) + _pData.getPointStyles().substr(2*i,1) + _pData.getPointStyles().substr(2*i+1,1);
        else
            sConPointStyles[i] += _pData.getLineStyles().substr(i,1) + _pData.getPointStyles().substr(2*i+1,1);
        sContStyles[i] += _pData.getLineStyles()[i];
        sLineStyles[i] += _pData.getLineSizes()[i];
    }
}

void parser_plot_evaluateSubplot(mglGraph& _graph, PlotData& _pData, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option,
                                size_t& nLegends, string& sCmd, size_t nMultiplots[2], size_t& nSubPlots, size_t& nSubPlotMap)
{
    if (nLegends && !_pData.getSchematic())
    {
        _graph.Legend(_pData.getLegendPosition());
        _graph.ClearLegend();
    }

    string sSubPlotIDX = sCmd.substr(findCommand(sCmd).nPos+7);
    if (sSubPlotIDX.find("-set") != string::npos || sSubPlotIDX.find("--") != string::npos)
    {
        if (sSubPlotIDX.find("-set") != string::npos)
            sSubPlotIDX.erase(sSubPlotIDX.find("-set"));
        else
            sSubPlotIDX.erase(sSubPlotIDX.find("--"));
    }
    StripSpaces(sSubPlotIDX);
    if (matchParams(sCmd, "cols", '=') || matchParams(sCmd, "lines", '='))
    {
        unsigned int nMultiLines = 1, nMultiCols = 1;

        if (matchParams(sCmd, "cols", '='))
        {
            _parser.SetExpr(getArgAtPos(sCmd, matchParams(sCmd, "cols", '=')+4));
            nMultiCols = (unsigned int)_parser.Eval();
        }
        if (matchParams(sCmd, "lines", '='))
        {
            _parser.SetExpr(getArgAtPos(sCmd, matchParams(sCmd, "lines", '=')+5));
            nMultiLines = (unsigned int)_parser.Eval();
        }
        if (sSubPlotIDX.length())
        {
            if (!_functions.call(sSubPlotIDX, _option))
                throw FUNCTION_ERROR;
            if (_data.containsCacheElements(sSubPlotIDX) || sSubPlotIDX.find("data(") != string::npos)
            {
                parser_GetDataElement(sSubPlotIDX, _parser, _data, _option);
            }
            _parser.SetExpr(sSubPlotIDX);
            int nRes = 0;
            value_type* v = _parser.Eval(nRes);
            if (nRes == 1)
            {
                if (v[0] < 1)
                    v[0] = 1;
                if ((unsigned int)v[0]-1 >= nMultiplots[0]*nMultiplots[1])
                    throw INVALID_SUBPLOT_INDEX;
                if (!checkMultiPlotArray(nMultiplots, nSubPlotMap, (unsigned int)(v[0]-1), nMultiCols, nMultiLines))
                    throw INVALID_SUBPLOT_INDEX;
                _graph.MultiPlot(nMultiplots[0], nMultiplots[1], (unsigned int)v[0]-1, nMultiCols, nMultiLines);
            }   // cols, lines
            else
            {
                if ((unsigned int)(v[1]-1+(v[0]-1)*nMultiplots[1]) >= nMultiplots[0]*nMultiplots[1])
                    throw INVALID_SUBPLOT_INDEX;
                if (!checkMultiPlotArray(nMultiplots, nSubPlotMap, (unsigned int)((v[1]-1)+(v[0]-1)*nMultiplots[0]), nMultiCols, nMultiLines))
                    throw INVALID_SUBPLOT_INDEX;
                _graph.MultiPlot(nMultiplots[0], nMultiplots[1], (unsigned int)((v[1]-1)+(v[0]-1)*nMultiplots[0]), nMultiCols, nMultiLines);
            }
        }
        else
        {
            if (nSubPlots >= nMultiplots[0]*nMultiplots[1])
                throw INVALID_SUBPLOT_INDEX;
            int nPlotPos = 1;
            for (unsigned int nSub = 0; nSub < nMultiplots[0]*nMultiplots[1]; nSub++)
            {
                if (nPlotPos & nSubPlotMap)
                    nPlotPos <<= 1;
                else
                {
                    if (!checkMultiPlotArray(nMultiplots, nSubPlotMap, nSub, nMultiCols, nMultiLines))
                        throw INVALID_SUBPLOT_INDEX;
                    _graph.MultiPlot(nMultiplots[0], nMultiplots[1], nSub, nMultiCols, nMultiLines);
                    break;
                }
                if (nSub == nMultiplots[0]*nMultiplots[1]-1)
                {
                    throw INVALID_SUBPLOT_INDEX;
                }
            }
        }
    }
    else
    {
        if (sSubPlotIDX.length())
        {
            if (!_functions.call(sSubPlotIDX, _option))
                throw FUNCTION_ERROR;
            if (_data.containsCacheElements(sSubPlotIDX) || sSubPlotIDX.find("data(") != string::npos)
            {
                parser_GetDataElement(sSubPlotIDX, _parser, _data, _option);
            }
            _parser.SetExpr(sSubPlotIDX);
            int nRes = 0;
            value_type* v = _parser.Eval(nRes);
            if (nRes == 1)
            {
                if (v[0] < 1)
                    v[0] = 1;
                if ((unsigned int)v[0]-1 >= nMultiplots[0]*nMultiplots[1])
                    throw INVALID_SUBPLOT_INDEX;
                if ((unsigned int)v[0] != 1)
                    nRes <<= (unsigned int)(v[0]-1);
                if (nRes & nSubPlotMap)
                    throw INVALID_SUBPLOT_INDEX;
                nSubPlotMap |= nRes;
                _graph.SubPlot(nMultiplots[0], nMultiplots[1], (unsigned int)v[0]-1);
            }
            else
            {
                if ((unsigned int)(v[1]-1+(v[0]-1)*nMultiplots[0]) >= nMultiplots[0]*nMultiplots[1])
                    throw INVALID_SUBPLOT_INDEX;
                nRes = 1;
                if ((unsigned int)((v[1])+(v[0]-1)*nMultiplots[0]) != 1)
                    nRes <<= (unsigned int)((v[1]-1)+(v[0]-1)*nMultiplots[0]);
                if (nRes & nSubPlotMap)
                    throw INVALID_SUBPLOT_INDEX;
                nSubPlotMap |= nRes;
                _graph.SubPlot(nMultiplots[0], nMultiplots[1], (unsigned int)((v[1]-1)+(v[0]-1)*nMultiplots[0]));
            }
        }
        else
        {
            if (nSubPlots >= nMultiplots[0]*nMultiplots[1])
                throw INVALID_SUBPLOT_INDEX;
            int nPlotPos = 1;
            for (unsigned int nSub = 0; nSub < nMultiplots[0]*nMultiplots[1]; nSub++)
            {
                if (nPlotPos & nSubPlotMap)
                    nPlotPos <<= 1;
                else
                {
                    nSubPlotMap |= nPlotPos;
                    _graph.SubPlot(nMultiplots[0], nMultiplots[1], nSub);
                    break;
                }
                if (nSub == nMultiplots[0]*nMultiplots[1]-1)
                {
                    throw INVALID_SUBPLOT_INDEX;
                }
            }
        }
    }
}

void parser_plot_displayMessage(PlotData& _pData, PlotInfo& _pInfo, mglGraph& _graph, const Settings& _option)
{
    if (!_pData.getSilentMode() && _option.getSystemPrintStatus())
        NumeReKernel::printPreFmt(toSystemCodePage("|-> " + _lang.get("PLOT_CALCULATING_DATA_FOR") + " "));

    if (_pInfo.sCommand == "surface3d"
        || _pInfo.sCommand.substr(0,6) == "surf3d"
        || _pInfo.sCommand == "meshgrid3d"
        || _pInfo.sCommand.substr(0,6) == "mesh3d"
        || _pInfo.sCommand == "contour3d"
        || _pInfo.sCommand.substr(0,6) == "cont3d"
        || _pInfo.sCommand == "density3d"
        || _pInfo.sCommand.substr(0,6) == "dens3d"
        || _pInfo.sCommand == "gradient3d"
        || _pInfo.sCommand.substr(0,6) == "grad3d")
    {
        _pInfo.b3D = true;
        if (_pInfo.nSamples > 51)
        {
            if (_pData.getHighRes() == 2 && _pInfo.nSamples > 101)
                _pInfo.nSamples = 101;
            else if ((_pData.getHighRes() == 1 || !_option.getbUseDraftMode()) && _pInfo.nSamples > 74)
                _pInfo.nSamples = 101;
            else
                _pInfo.nSamples = 51;
        }
        if (!_pData.getSilentMode() && _option.getSystemPrintStatus() && _pInfo.sCommand.substr(0,4) == "surf")
            NumeReKernel::printPreFmt("3D-" + toSystemCodePage(_lang.get("PLOT_SURFACE")) + "-");
        else if (!_pData.getSilentMode() && _option.getSystemPrintStatus() && _pInfo.sCommand.substr(0,4) == "mesh")
            NumeReKernel::printPreFmt("3D-" + toSystemCodePage(_lang.get("PLOT_MESHGRID")) + "-");
        else if (!_pData.getSilentMode() && _option.getSystemPrintStatus() && _pInfo.sCommand.substr(0,4) == "cont")
            NumeReKernel::printPreFmt("3D-" + toSystemCodePage(_lang.get("PLOT_CONTOUR")) + "-");
        else if (!_pData.getSilentMode() && _option.getSystemPrintStatus() && _pInfo.sCommand.substr(0,4) == "dens")
            NumeReKernel::printPreFmt("3D-" + toSystemCodePage(_lang.get("PLOT_DENSITY")) + "-");
        else if (!_pData.getSilentMode() && _option.getSystemPrintStatus() && _pInfo.sCommand.substr(0,4) == "grad")
            NumeReKernel::printPreFmt("3D-" + toSystemCodePage(_lang.get("PLOT_GRADIENT")) + "-");
    }
    else if (_pInfo.sCommand.substr(0,6) == "vect3d" ||_pInfo.sCommand == "vector3d")
    {
        _pInfo.b3DVect = true;
        if (_pInfo.nSamples > 11)
            _pInfo.nSamples = 11;
        if (!_pData.getSilentMode() && _option.getSystemPrintStatus())
            NumeReKernel::printPreFmt("3D-" + toSystemCodePage(_lang.get("PLOT_VECTOR")) + "-");
        if (_pData.getPipe() || _pData.getFlow())
        {
            if (_pInfo.nSamples % 2)
                _pInfo.nSamples -= 1;
            if (_pData.getHighRes() <= 1 && _option.getbUseDraftMode())
                _graph.SetQuality(5);
            else
                _graph.SetQuality(6);
        }
    }
    else if (_pInfo.sCommand.substr(0,4) == "vect")
    {
        _pInfo.b2DVect = true;
        if (_pInfo.nSamples > 21)
            _pInfo.nSamples = 21;
        if (!_pData.getSilentMode() && _option.getSystemPrintStatus())
            NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("PLOT_VECTOR")) + "-");
        if (_pData.getPipe() || _pData.getFlow())
        {
            if (_pData.getHighRes() <= 1 && _option.getbUseDraftMode())
                _graph.SetQuality(5);
            else
                _graph.SetQuality(6);
        }
    }
    else if (_pInfo.sCommand.substr(0,4) == "mesh"
        || _pInfo.sCommand.substr(0,4) == "surf"
        || _pInfo.sCommand.substr(0,4) == "cont"
        || _pInfo.sCommand.substr(0,4) == "grad"
        || _pInfo.sCommand.substr(0,4) == "dens")
    {
        _pInfo.b2D = true;
        if (!_pData.getSilentMode() && _option.getSystemPrintStatus() && _pInfo.sCommand.substr(0,4) == "surf")
            NumeReKernel::printPreFmt("2D-" + toSystemCodePage(_lang.get("PLOT_SURFACE")) + "-");
        else if (!_pData.getSilentMode() && _option.getSystemPrintStatus() && _pInfo.sCommand.substr(0,4) == "mesh")
            NumeReKernel::printPreFmt("2D-" + toSystemCodePage(_lang.get("PLOT_MESHGRID")) + "-");
        else if (!_pData.getSilentMode() && _option.getSystemPrintStatus() && _pInfo.sCommand.substr(0,4) == "cont")
            NumeReKernel::printPreFmt("2D-" + toSystemCodePage(_lang.get("PLOT_CONTOUR")) + "-");
        else if (!_pData.getSilentMode() && _option.getSystemPrintStatus() && _pInfo.sCommand.substr(0,4) == "dens")
            NumeReKernel::printPreFmt("2D-" + toSystemCodePage(_lang.get("PLOT_DENSITY")) + "-");
        else if (!_pData.getSilentMode() && _option.getSystemPrintStatus() && _pInfo.sCommand.substr(0,4) == "grad")
            NumeReKernel::printPreFmt("2D-" + toSystemCodePage(_lang.get("PLOT_GRADIENT")) + "-");
    }
    else if (_pInfo.sCommand == "plot3d")
    {
        if (!_pData.getSilentMode() && _option.getSystemPrintStatus())
            NumeReKernel::printPreFmt("3D-");
    }
    else if (_pInfo.sCommand == "draw")
    {
        _pInfo.bDraw = true;
        if (!_pData.getSilentMode() && _option.getSystemPrintStatus())
            NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("PLOT_DRAWING")) + " ... \n");
    }
    else if (_pInfo.sCommand == "draw3d")
    {
        _pInfo.bDraw3D = true;
        if (!_pData.getSilentMode() && _option.getSystemPrintStatus())
            NumeReKernel::printPreFmt(toSystemCodePage("3D-"+_lang.get("PLOT_DRAWING")) + " ... \n");
    }
    if (!_pData.getSilentMode() && _option.getSystemPrintStatus() && !_pData.getAnimateSamples() && !(_pInfo.bDraw3D || _pInfo.bDraw))
        NumeReKernel::printPreFmt("Plot ... \n");
    else if (!_pData.getSilentMode() && _option.getSystemPrintStatus() && !(_pInfo.bDraw3D || _pInfo.bDraw))
    {
        NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("PLOT_ANIMATION")) + " ... \n");
    }

}

void parser_plot_evaluateDataPlots(PlotData& _pData, PlotInfo& _pInfo, Parser& _parser, Datafile& _data, Define& _functions, mglData**& _mDataPlots, const Settings& _option,
                                    vector<short>& vType, string& sFunc, string& sDataPlots, string& sDataLabels, string& sAxisBinds, string& sDataAxisBinds,
                                    int*& nDataDim, int& nDataPlots, double dDataRanges[3][2], double dSecDataRanges[2][2])
{
    const short TYPE_DATA = -1;
    const short TYPE_FUNC = 1;

    if (containsDataObject(sFunc) || _data.containsCacheElements(sFunc))
    {
        string sFuncTemp = sFunc;
        string sToken = "";

        while (sFuncTemp.length())
        {
            sToken = getNextArgument(sFuncTemp, true);
            StripSpaces(sToken);
            if (containsDataObject(sToken) || _data.containsCacheElements(sToken))
            {
                if (sToken.find("data(") != string::npos && sToken.find("data(") && checkDelimiter(sToken.substr(sToken.find("data(")-1,6)))
                    throw DATAPOINTS_CANNOT_BE_MODIFIED_WHILE_PLOTTING;
                if (_data.containsCacheElements(sToken.substr(0, sToken.find("(")+1)) && !_data.isCacheElement(sToken.substr(0,sToken.find("("))))
                    throw DATAPOINTS_CANNOT_BE_MODIFIED_WHILE_PLOTTING;
                string sSubstr = sToken.substr(getMatchingParenthesis(sToken.substr(sToken.find('(')))+sToken.find('(')+1);
                if (sSubstr[sSubstr.find_first_not_of(' ')] != '"')
                    throw DATAPOINTS_CANNOT_BE_MODIFIED_WHILE_PLOTTING;
            }
        }

        // --> Zerlegen von sFunc in Funktionenteile und Datenplots <--
        sFuncTemp = sFunc;
        sFunc.clear();
        unsigned int nPos = 0;
        unsigned int nPos_1 = 0;
        while(sFuncTemp.length())
        {
            sToken = getNextArgument(sFuncTemp, true);
            if (containsDataObject(sToken) || _data.containsCacheElements(sToken))
            {
                sDataPlots += ";" + sToken;
                vType.push_back(TYPE_DATA);
                sDataAxisBinds += _pData.getAxisbind((sDataAxisBinds.length() + sAxisBinds.length())/2);
            }
            else
            {
                sFunc += ","+sToken;
                _functions.call(sToken, _option);
                StripSpaces(sToken);
                if (sToken.front() == '{')
                    sToken.front() = ' ';
                if (sToken.back() == '}')
                    sToken.back() = ' ';
                while (getNextArgument(sToken, true).length())
                    vType.push_back(TYPE_FUNC);
                sAxisBinds += _pData.getAxisbind((sDataAxisBinds.length() + sAxisBinds.length())/2);
            }
        }
        sFunc.erase(0,1);
        /* --> In dem string sFunc sind nun nur noch "gewoehnliche" Funktionen, die der Parser auswerten kann,
         *     in dem string sDataPlots sollten alle Teile sein, die "data(" oder "cache(" enthalten <--
         */

        if (!_pInfo.b2DVect && !_pInfo.b3DVect && !_pInfo.b3D && !_pInfo.bDraw && !_pInfo.bDraw3D)
        {
            // --> Zaehlen wir die Datenplots. Durch den char ';' ist das recht simpel <--
            for (unsigned int i = 0; i < sDataPlots.length(); i++)
            {
                if (sDataPlots[i] == ';')
                    nDataPlots++;
            }

            // --> Allozieren wir Speicher fuer die gesamten Datenarrays <--
            _mDataPlots = new mglData*[nDataPlots];
            nDataDim = new int[nDataPlots];
            nPos = 0;

            // --> Nutzen wir den Vorteil von ";" als Trennzeichen <--
            int n_dpos = 0;

            // --> Trennen wir die Legenden der Datensaetze von den eigentlichen Datensaetzen ab <--
            do
            {
                if (sDataPlots.find('#') != string::npos && sDataPlots.find('#') < sDataPlots.find('"'))
                    n_dpos = sDataPlots.find('#');
                else
                    n_dpos = sDataPlots.find('"');
                if (sDataPlots.find(';', n_dpos+1) == string::npos)
                {
                    sDataLabels += sDataPlots.substr(n_dpos)+";";
                    sDataPlots = sDataPlots.substr(0,n_dpos);
                }
                else
                {
                    sDataLabels += sDataPlots.substr(n_dpos, sDataPlots.find(';', n_dpos+1)-n_dpos)+ ";";
                    sDataPlots = sDataPlots.substr(0,n_dpos) + sDataPlots.substr(sDataPlots.find(';', n_dpos+1));
                }
            }
            while (sDataPlots.find('"') != string::npos || sDataPlots.find('#') != string::npos);

            // --> Ersetzen von "data()" bzw. "cache()" durch die Spaltentitel <--
            n_dpos = 0;
            while (sDataLabels.find(';', n_dpos) != string::npos)
            {
                if ((sDataLabels.substr(n_dpos, sDataLabels.find(';', n_dpos)-n_dpos).find("data(") != string::npos
                    || _data.containsCacheElements(sDataLabels.substr(n_dpos, sDataLabels.find(';', n_dpos)-n_dpos)))
                        && (sDataLabels.substr(n_dpos, sDataLabels.find(';', n_dpos)-n_dpos).find(',') != string::npos
                            || sDataLabels.substr(n_dpos, sDataLabels.find(';', n_dpos)-n_dpos).substr(sDataLabels.substr(n_dpos, sDataLabels.find(';', n_dpos)-n_dpos).find('('),2) == "()")
                        && sDataLabels.substr(n_dpos, sDataLabels.find(';', n_dpos)-n_dpos).find(')') != string::npos)
                {
                    if ((sDataLabels.substr(n_dpos, sDataLabels.find(';', n_dpos)-n_dpos).find("data(") != string::npos && checkDelimiter(sDataLabels.substr(sDataLabels.substr(n_dpos, sDataLabels.find(';', n_dpos)-n_dpos).find("data(")-1, 6)) && !_data.isValid())
                        || (_data.containsCacheElements(sDataLabels.substr(n_dpos, sDataLabels.find(';', n_dpos)-n_dpos)) && !_data.isValidCache()))
                    {
                        throw NO_DATA_AVAILABLE;
                    }
                    string sTemp = sDataLabels.substr(n_dpos, sDataLabels.find(';', n_dpos)-n_dpos);
                    StripSpaces(sTemp);
                    string sTableName = sTemp.substr(0,sTemp.find('('));
                    if (sTableName[0] == ';' || sTableName[0] == '"')
                        sTableName.erase(0,1);
                    if (getMatchingParenthesis(sTemp.substr(sTemp.find('('))) == string::npos)
                        throw UNMATCHED_PARENTHESIS;
                    string sArgs = sTemp.substr(sTemp.find('('), getMatchingParenthesis(sTemp.substr(sTemp.find('(')))+1);
                    if (sArgs == "()")
                        sArgs = "(:,:)";
                    string sArg_1 = "<<empty>>";
                    string sArg_2 = "<<empty>>";
                    string sArg_3 = "<<empty>>";
                    parser_SplitArgs(sArgs, sArg_1, ',', _option);
                    if (sArg_1.find(':') != string::npos)
                    {
                        sArg_1 = "(" + sArg_1 + ")";
                        try
                        {
                            parser_SplitArgs(sArg_1, sArg_2, ':', _option);
                        }
                        catch (errorcode& e)
                        {
                            if (e == SEPARATOR_NOT_FOUND)
                            {
                                n_dpos = sDataLabels.find(';', n_dpos)+1;
                                continue;
                            }
                            else
                                throw;
                        }
                        StripSpaces(sArg_2);
                        if (sArg_2.find(':') != string::npos)
                        {
                            sArg_2 = "(" + sArg_2 + ")";
                            parser_SplitArgs(sArg_2, sArg_3, ':', _option);
                            StripSpaces(sArg_2);
                            if (sArg_3.find(':') != string::npos)
                            {
                                sArg_3 = "(" + sArg_3 + ")";
                                parser_SplitArgs(sArg_3, sArgs, ':', _option);
                                StripSpaces(sArg_3);
                            }
                        }
                    }
                    StripSpaces(sArg_1);
                    if (!sArg_1.length() && !sArg_2.length() && sArg_3 == "<<empty>>")
                    {
                        sArg_1 = "1";
                    }
                    else if (!sArg_1.length())
                    {
                        n_dpos = sDataLabels.find(';', n_dpos)+1;
                        continue;
                    }
                    if (_data.containsCacheElements(sTemp))
                    {
                        _data.setCacheStatus(true);
                    }
                    if (sArg_2 == "<<empty>>" && sArg_3 == "<<empty>>" && _pInfo.sCommand != "plot3d")
                    {
                        _parser.SetExpr(sArg_1);
                        sTemp = "\"" + _data.getTopHeadLineElement((int)_parser.Eval()-1, sTableName) + "\"";
                    }
                    else if (sArg_2.length())
                    {
                        if (_pInfo.sCommand != "plot3d")
                        {
                            if (!(_pData.getyError() || _pData.getxError()) && sArg_3 == "<<empty>>")
                            {
                                _parser.SetExpr(sArg_2);
                                sTemp = "\"" + _data.getTopHeadLineElement((int)_parser.Eval()-1,sTableName) + " vs. ";
                                _parser.SetExpr(sArg_1);
                                sTemp += _data.getTopHeadLineElement((int)_parser.Eval()-1,sTableName) + "\"";
                            }
                            else if (sArg_3 != "<<empty>>")
                            {
                                _parser.SetExpr(sArg_2);
                                sTemp = "\"" + _data.getTopHeadLineElement((int)_parser.Eval()-1,sTableName) + " vs. ";
                                _parser.SetExpr(sArg_1);
                                sTemp += _data.getTopHeadLineElement((int)_parser.Eval()-1,sTableName) + "\"";
                            }
                            else
                            {
                                double dArg_1, dArg_2;
                                _parser.SetExpr(sArg_1);
                                dArg_1 = _parser.Eval();
                                _parser.SetExpr(sArg_2);
                                dArg_2 = _parser.Eval();
                                if (dArg_1 < dArg_2)
                                    sTemp = "\"" + _data.getTopHeadLineElement((int)dArg_1,sTableName) + " vs. " + _data.getTopHeadLineElement((int)dArg_1-1,sTableName) + "\"";
                                else
                                    sTemp = "\"" + _data.getTopHeadLineElement((int)dArg_2-1,sTableName) + " vs. " + _data.getTopHeadLineElement((int)dArg_2,sTableName) + "\"";
                            }
                        }
                        else if (sArg_3 == "<<empty>>" || !sArg_3.length())
                        {
                            double dArg_1, dArg_2;
                            _parser.SetExpr(sArg_1);
                            dArg_1 = _parser.Eval();
                            _parser.SetExpr(sArg_2);
                            dArg_2 = _parser.Eval();
                            if (dArg_1 < dArg_2)
                                sTemp = "\"" + _data.getTopHeadLineElement((int)dArg_1-1,sTableName) + ", " + _data.getTopHeadLineElement((int)dArg_2-2,sTableName) + ", " + _data.getTopHeadLineElement((int)dArg_2-1,sTableName) + "\"";
                            else
                                sTemp = "\"" + _data.getTopHeadLineElement((int)dArg_2-1,sTableName) + ", " + _data.getTopHeadLineElement((int)dArg_1-2,sTableName) + ", " + _data.getTopHeadLineElement((int)dArg_1-1,sTableName) + "\"";
                        }
                        else if (sArg_3.length())
                        {
                            _parser.SetExpr(sArg_1);
                            sTemp = "\"" + _data.getTopHeadLineElement((int)_parser.Eval()-1,sTableName) + ", ";
                            _parser.SetExpr(sArg_2);
                            sTemp += _data.getTopHeadLineElement((int)_parser.Eval()-1,sTableName) + ", ";
                            _parser.SetExpr(sArg_3);
                            sTemp += _data.getTopHeadLineElement((int)_parser.Eval()-1,sTableName) + "\"";
                        }
                    }
                    else if (!sArg_2.length())
                    {
                        if (_pInfo.sCommand != "plot3d")
                        {
                            _parser.SetExpr(sArg_1);
                            sTemp = "\"" + _data.getTopHeadLineElement((int)_parser.Eval(),sTableName) + " vs. " + _data.getTopHeadLineElement((int)_parser.Eval()-1,sTableName) + "\"";
                        }
                        else if (sArg_3 == "<<empty>>" || !sArg_3.length())
                        {
                            _parser.SetExpr(sArg_1);
                            sTemp = "\"" + _data.getTopHeadLineElement((int)_parser.Eval()-1,sTableName) + ", "
                                + _data.getTopHeadLineElement((int)_parser.Eval(),sTableName) + ", "
                                + _data.getTopHeadLineElement((int)_parser.Eval()+1,sTableName) + "\"";
                        }
                        else if (sArg_3.length())
                        {
                            _parser.SetExpr(sArg_1);
                            sTemp = "\"" + _data.getTopHeadLineElement((int)_parser.Eval()-1,sTableName) + ", " + _data.getTopHeadLineElement((int)_parser.Eval(),sTableName) + ", ";
                            _parser.SetExpr(sArg_3);
                            sTemp += _data.getTopHeadLineElement((int)_parser.Eval()-1,sTableName) + "\"";
                        }
                    }
                    else
                    {
                        n_dpos = sDataLabels.find(';', n_dpos)+1;
                        continue;
                    }
                    _data.setCacheStatus(false);
                    for (unsigned int n = 0; n < sTemp.length(); n++)
                    {
                        if (sTemp[n] == '_')
                            sTemp[n] = ' ';
                    }
                    sDataLabels = sDataLabels.substr(0,n_dpos) + sTemp + sDataLabels.substr(sDataLabels.find(';', n_dpos));

                }
                n_dpos = sDataLabels.find(';', n_dpos)+1;
            }

            if (_option.getbDebug())
            {
                cerr << "|-> DEBUG: sDataPlots = " << sDataPlots << endl;
                cerr << "|-> DEBUG: sDataLabels = " << sDataLabels << endl;
            }

            string si_pos[2] = {"", ""};                    // String-Array fuer die Zeilen-Position: muss fuer alle Spalten identisch sein!
            string sj_pos[6] = {"", "", "", "", "", ""};    // String-Array fuer die Spalten: kann bis zu sechs beliebige Werte haben
            string sDataTable = "";
            int i_pos[2] = {0, 0};                          // Int-Array fuer den Wert der Zeilen-Positionen
            int j_pos[6] = {0, 0, 0, 0, 0, 0};              // Int-Array fuer den Wert der Spalten-Positionen
            int nMatch = 0;                                 // Int fuer die Position des aktuellen find-Treffers eines Daten-Objekts
            vector<long long int> vLine;
            vector<long long int> vCol;
            value_type* v = 0;
            int nResults = 0;

            // --> Bestimmen wir die Zeilen- und Spalten-Koordinaten des aktuellen Daten-Objekts <--
            for (int i = 0; i < nDataPlots; i++)
            {
                sDataTable = "data";
                // --> Suchen wir den aktuellen und den darauf folgenden ';' <--
                nPos = sDataPlots.find(';', nPos)+1;
                nPos_1 = sDataPlots.find(';', nPos);

                // --> Ist da "cache" drin? Aktivieren wir den Cache-Status <--
                if (_data.containsCacheElements(sDataPlots.substr(nPos, nPos_1-nPos)) && sDataPlots.substr(nPos,5) != "data(")
                {
                    _data.setCacheStatus(true);
                    for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
                    {
                        if (sDataPlots.substr(nPos, nPos_1-nPos).find(iter->first+"(") != string::npos
                            && (!sDataPlots.substr(nPos, nPos_1-nPos).find(iter->first+"(")
                                || checkDelimiter(sDataPlots.substr(nPos, nPos_1-nPos).substr(sDataPlots.substr(nPos, nPos_1-nPos).find(iter->first+"(")-1, (iter->first).length()+2))))
                        {
                            sDataTable = iter->first;
                            break;
                        }
                    }
                }
                // --> Klammer und schliessende Klammer finden und in einen anderen String schreiben <--
                nMatch = sDataPlots.find('(', nPos);
                si_pos[0] = sDataPlots.substr(nMatch, getMatchingParenthesis(sDataPlots.substr(nMatch))+1);
                if (si_pos[0] == "()" || si_pos[0][si_pos[0].find_first_not_of(' ',1)] == ')')
                    si_pos[0] = "(:,:)";
                if (containsDataObject(si_pos[0]) || _data.containsCacheElements(si_pos[0]))
                {
                    parser_GetDataElement(si_pos[0], _parser, _data, _option);
                }

                if (_option.getbDebug())
                    cerr << "|-> DEBUG: si_pos[0] = " << si_pos[0] << endl;

                // --> Rausgeschnittenen String am Komma ',' in zwei Teile teilen <--
                try
                {
                    parser_SplitArgs(si_pos[0], sj_pos[0], ',', _option);
                }
                catch (...)
                {
                    delete[] _mDataPlots;
                    delete[] nDataDim;
                    throw;
                }
                if (_option.getbDebug())
                    cerr << "|-> DEBUG: si_pos[0] = " << si_pos[0] << ", sj_pos[0] = " << sj_pos[0] << endl;

                // --> Gibt's einen Doppelpunkt? Dann teilen wir daran auch noch mal <--
                if (si_pos[0].find(':') != string::npos)
                {
                    si_pos[0] = "( " + si_pos[0] + " )";
                    try
                    {
                        parser_SplitArgs(si_pos[0], si_pos[1], ':', _option);
                    }
                    catch (...)
                    {
                        delete[] _mDataPlots;
                        delete[] nDataDim;
                        throw;
                    }
                    if (!parser_ExprNotEmpty(si_pos[1]))
                        si_pos[1] = "inf";
                }
                else
                    si_pos[1] = "";

                if (_option.getbDebug())
                {
                    cerr << "|-> DEBUG: si_pos[0] = " << si_pos[0] << ", si_pos[1] = " << si_pos[1] << endl;
                }

                // --> Auswerten mit dem Parser <--
                if (parser_ExprNotEmpty(si_pos[0]))
                {
                    _parser.SetExpr(si_pos[0]);
                    v = _parser.Eval(nResults);
                    if (nResults > 1)
                    {
                        for (int n = 0; n < nResults; n++)
                            vLine.push_back((int)v[n]-1);
                    }
                    else
                        i_pos[0] = (int)v[0] - 1;
                }
                else
                    i_pos[0] = 0;
                if (si_pos[1] == "inf")
                {
                    i_pos[1] = _data.getLines(sDataTable, false);
                }
                else if (parser_ExprNotEmpty(si_pos[1]))
                {
                    _parser.SetExpr(si_pos[1]);
                    i_pos[1] = (int)_parser.Eval();
                }
                else if (!vLine.size())
                    i_pos[1] = i_pos[0]+1;
                // --> Pruefen, ob die Reihenfolge der Indices sinnvoll ist <--
                parser_CheckIndices(i_pos[0], i_pos[1]);

                if (_option.getbDebug())
                    cerr << "|-> DEBUG: i_pos[0] = " << i_pos[0] << ", i_pos[1] = " << i_pos[1] << ", vLine.size() = " << vLine.size() << endl;

                if (!parser_ExprNotEmpty(sj_pos[0]))
                    sj_pos[0] = "0";

                /* --> Jetzt fuer die Spalten: Fummelig. Man soll bis zu 6 Spalten angeben koennen und
                 *     das Programm sollte trotzdem einen Sinn darin finden <--
                 */
                int j = 0;
                try
                {
                    while (sj_pos[j].find(':') != string::npos && j < 5)
                    {
                        sj_pos[j] = "( " + sj_pos[j] + " )";
                        // --> String am naechsten ':' teilen <--
                        parser_SplitArgs(sj_pos[j], sj_pos[j+1], ':', _option);
                        // --> Spezialfaelle beachten: ':' ohne linke bzw. rechte Grenze <--
                        if (!parser_ExprNotEmpty(sj_pos[j]))
                            sj_pos[j] = "1";
                        j++;
                        if (!parser_ExprNotEmpty(sj_pos[j]))
                            sj_pos[j] = "inf";
                    }
                }
                catch (...)
                {
                    delete[] _mDataPlots;
                    delete[] nDataDim;
                    throw;
                }
                // --> Alle nicht-beschriebenen Grenzen-Strings auf "" setzen <--
                for (int k = j+1; k < 6; k++)
                    sj_pos[k] = "";

                // --> Grenzen-Strings moeglichst sinnvoll auswerten <--
                for (int k = 0; k <= j; k++)
                {

                    // --> "inf" bedeutet "infinity". Ergo: die letztmoegliche Spalte <--
                    if (sj_pos[k] == "inf")
                    {
                        j_pos[k] = _data.getCols(sDataTable)-1;
                        break;
                    }
                    else if (parser_ExprNotEmpty(sj_pos[k]))
                    {
                        if (j == 0)
                        {
                            _parser.SetExpr(sj_pos[0]);
                            v = _parser.Eval(nResults);
                            if (nResults > 1)
                            {
                                for (int n = 0; n < nResults; n++)
                                {
                                    if (n >= 6)
                                        break;
                                    vCol.push_back((int)v[n]-1);
                                    j_pos[n] = (int)v[n]-1;
                                    j = n;
                                }
                                break;
                            }
                            else
                                j_pos[0] = (int)v[0] - 1;
                        }
                        else
                        {
                            // --> Hat einen Wert: Kann man auch auswerten <--
                            _parser.SetExpr(sj_pos[k]);
                            j_pos[k] = (int)_parser.Eval() - 1;
                        }
                        //cerr << j_pos[k] << endl;
                    }
                    else if (!k)
                    {
                        // --> erstes Element pro Forma auf 0 setzen <--
                        j_pos[k] = 0;
                    }
                    else // "data(2:4::7) = Spalten 2-4,5-7"
                    {
                        // --> Spezialfall. Verwendet vermutlich niemand <--
                        j_pos[k] = j_pos[k]+1;
                    }
                }

                if (_option.getbDebug())
                    cerr << "|-> DEBUG: j_pos[0] = " << j_pos[0] << ", j_pos[1] = " << j_pos[1] << ", vCol.size() = " << vCol.size() << endl;

                if (i_pos[1] > _data.getLines(sDataTable, false))
                    i_pos[1] = _data.getLines(sDataTable, false);
                if (j_pos[1] > _data.getCols(sDataTable)-1 && _pInfo.sCommand != "plot3d")
                    j_pos[1] = _data.getCols(sDataTable)-1;
                if (!vLine.size() && !vCol.size() && (j_pos[0] < 0
                    || j_pos[1] < 0
                    || i_pos[0] > _data.getLines(sDataTable, false)
                    || i_pos[1] > _data.getLines(sDataTable, false)
                    || (j_pos[0] > _data.getCols(sDataTable)-1 && _pInfo.sCommand != "plot3d")
                    || (j_pos[1] > _data.getCols(sDataTable)-1 && _pInfo.sCommand != "plot3d")))
                {
                    delete[] _mDataPlots;
                    delete[] nDataDim;
                    throw INVALID_INDEX;
                }



                /* --> Bestimmen wir die "Dimension" des zu plottenden Datensatzes. Dabei ist es auch
                 *     von Bedeutung, ob Fehlerbalken anzuzeigen sind <--
                 */
                nDataDim[i] = 0;
                /*if (vCol.size())
                    nDataDim[i] = vCol.size();
                else*/ if (j == 0 && !_pData.getxError() && !_pData.getyError())
                    nDataDim[i] = 2;
                else if (j == 0 && _pData.getxError() && _pData.getyError())
                    nDataDim[i] = 4;
                else if (j == 0 && (_pData.getxError() || _pData.getyError()))
                    nDataDim[i] = 3;
                else if (j == 1)
                {
                    if (_pInfo.sCommand == "plot" && !_pData.getxError() && !_pData.getyError())
                        nDataDim[i] = 2;
                    else if (_pInfo.sCommand == "plot" && _pData.getxError() && _pData.getyError())
                        nDataDim[i] = 4;
                    else if (_pInfo.sCommand == "plot" && (_pData.getxError() || _pData.getyError()))
                        nDataDim[i] = 3;
                    else if (_pInfo.sCommand == "plot3d" && !_pData.getxError() && !_pData.getyError())
                        nDataDim[i] = 3;
                    else if (_pInfo.sCommand == "plot3d" && _pData.getxError() && _pData.getyError())
                        nDataDim[i] = 6;
                    else if (_pInfo.b2D)
                        nDataDim[i] = 3;
                }
                else
                {
                    nDataDim[i] = j+1;
                }

                if (!nDataDim[i])
                {
                    delete[] nDataDim;
                    delete[] _mDataPlots;
                    throw PLOT_ERROR;
                }

                // --> Jetzt wissen wir die Spalten: Suchen wir im Falle von si_pos[1] == inf nach der laengsten <--
                if (si_pos[1] == "inf")
                {
                    int nAppendedZeroes = _data.getAppendedZeroes(j_pos[0],sDataTable);
                    if (sj_pos[1] == "inf"
                        && ((nDataDim[i] < 3 && _pInfo.sCommand == "plot")
                            || (nDataDim[i] == 3 && _pInfo.sCommand == "plot3d")))
                    {
                        for (int k = 1; k <= nDataDim[i]; k++)
                        {
                            //cerr << nAppendedZeroes << endl;
                            if (nAppendedZeroes > _data.getAppendedZeroes(j_pos[0]+k, sDataTable))
                                nAppendedZeroes = _data.getAppendedZeroes(j_pos[0]+k, sDataTable);
                        }
                    }
                    else if (_pInfo.b2D)
                    {
                        for (int j = j_pos[0]; j < j_pos[1]; j++)
                        {
                            if (nAppendedZeroes > _data.getAppendedZeroes(j, sDataTable))
                                nAppendedZeroes = _data.getAppendedZeroes(j, sDataTable);
                        }
                    }
                    else
                    {
                        for (int k = 1; k <= j; k++)
                        {
                            //cerr << nAppendedZeroes << endl;
                            if (nAppendedZeroes > _data.getAppendedZeroes(j_pos[k], sDataTable))
                                nAppendedZeroes = _data.getAppendedZeroes(j_pos[k], sDataTable);
                        }
                    }
                    //if (nAppendedZeroes < i_pos[1])
                        i_pos[1] = _data.getLines(sDataTable, true) - nAppendedZeroes;
                    if (_option.getbDebug())
                        cerr << "|-> DEBUG: i_pos[1] = " << i_pos[1] << endl;
                }


                /* --> Mit der bestimmten Dimension koennen wir einen passenden Satz neuer mglData-Objekte
                 *     im _mDataPlots-Array initialisieren <--
                 */
                _mDataPlots[i] = new mglData[nDataDim[i]];
                if (_option.getbDebug())
                    cerr << "|-> DEBUG: mglData-Element created! nDataDim[i] = " << nDataDim[i] << endl;

                // --> Datenspeicher der mglData-Objekte vorbereiten <--
                if (vLine.size())
                {
                    if (!vCol.size())
                    {
                        if (j == 0 && nDataDim[i] == 2)
                            vCol.push_back(-1);
                        vCol.push_back(j_pos[0]);
                        if (j == 1 && nDataDim[i] == 2)
                        {
                            if (sj_pos[1] != "inf")
                                vCol.push_back(j_pos[1]);
                            else if (sj_pos[1] == "inf")
                                vCol.push_back(j_pos[0]+1);
                        }
                        else if (j == 1 && nDataDim[i] >= 3)
                        {
                            if (j_pos[0] < j_pos[1] || sj_pos[1] == "inf")
                            {
                                for (int q = 1; q < nDataDim[i]; q++)
                                    vCol.push_back(j_pos[0]+q);
                            }
                            else
                            {
                                for (int q = 1; q < nDataDim[i]; q++)
                                    vCol.push_back(j_pos[0]-q);
                            }
                        }
                        else
                        {
                            for (int n = 1; n <= j; n++)
                                vCol.push_back(j_pos[n]);
                        }
                    }
                    vector<long long int> _vTempCol;
                    _vTempCol.push_back(0);
                    for (int q = 0; q < nDataDim[i]; q++)
                    {
                        _mDataPlots[i][q].Create(vLine.size());
                        if (vCol[q] == -1)
                        {
                            for (unsigned int n = 0; n < vLine.size(); n++)
                                _mDataPlots[i][q].a[n] = vLine[n]+1;
                        }
                        else
                        {
                            for (unsigned int n = 0; n < vLine.size(); n++)
                            {
                                _mDataPlots[i][q].a[n] = _data.getElement(vLine[n], vCol[q], sDataTable);
                            }
                            /*_vTempCol[0] = vCol[q];
                            //cerr << vCol[q] << endl;
                            _mDataPlots[i][q].Set(_data.getElement(vLine, _vTempCol, sDataTable));*/
                        }
                    }
                    // --> Berechnen der DataRanges <--
                    for (int l = 0; l < (int)vLine.size(); l++)
                    {
                        for (int q = 0; q < 3; q++)
                        {
                            if (q == 2 && nDataDim[i] < 3)
                            {
                                dDataRanges[2][0] = 0;
                                dDataRanges[2][1] = 0;
                                break;
                            }
                            if (l == 0 && i == 0)
                            {
                                if (q && _pData.getRangeSetting(q-1))
                                {
                                    if (_pData.getRanges(q-1,0) > _mDataPlots[i][q-1].a[l] || _pData.getRanges(q-1,1) < _mDataPlots[i][q-1].a[l])
                                    {
                                        if (_pInfo.sCommand != "plot")
                                        {
                                            dDataRanges[q][0] = NAN;
                                            dDataRanges[q][1] = NAN;
                                        }
                                        else
                                        {
                                            if (q < 2)
                                            {
                                                dSecDataRanges[q][0] = NAN;
                                                dSecDataRanges[q][1] = NAN;
                                            }
                                            dDataRanges[q][0] = NAN;
                                            dDataRanges[q][1] = NAN;
                                        }
                                        continue;
                                    }
                                }
                                if (_pInfo.sCommand != "plot" || q >= 2)
                                {
                                    dDataRanges[q][0] = _mDataPlots[i][q].a[l];
                                    dDataRanges[q][1] = _mDataPlots[i][q].a[l];
                                }
                                else
                                {
                                    if (sDataAxisBinds[2*i+!q] == 'l' || sDataAxisBinds[2*i+!q] == 'b')
                                    {
                                        dDataRanges[q][0] = _mDataPlots[i][q].a[l];
                                        dDataRanges[q][1] = _mDataPlots[i][q].a[l];
                                        dSecDataRanges[q][0] = NAN;
                                        dSecDataRanges[q][1] = NAN;
                                    }
                                    else
                                    {
                                        dSecDataRanges[q][0] = _mDataPlots[i][q].a[l];
                                        dSecDataRanges[q][1] = _mDataPlots[i][q].a[l];
                                        dDataRanges[q][0] = NAN;
                                        dDataRanges[q][1] = NAN;
                                    }
                                }
                            }
                            else
                            {
                                if (_pInfo.b2D && q == 2)
                                {
                                    if (_pData.getRangeSetting())
                                    {
                                        if (_pData.getRanges(0,0) > _mDataPlots[i][0].a[l] || _pData.getRanges(0,1) < _mDataPlots[i][0].a[l])
                                            continue;
                                    }
                                    if (_pData.getRangeSetting(1))
                                    {
                                        if (_pData.getRanges(1,0) > _mDataPlots[i][1].a[l] || _pData.getRanges(1,1) < _mDataPlots[i][1].a[l])
                                            continue;
                                    }
                                    for (int k = 0; k < i_pos[1]-i_pos[0]; k++)
                                    {
                                        if (dDataRanges[q][0] > _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] || isnan(dDataRanges[q][0]))
                                            dDataRanges[q][0] = _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k];
                                        if (dDataRanges[q][1] < _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] || isnan(dDataRanges[q][1]))
                                            dDataRanges[q][1] = _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k];
                                    }
                                }
                                else
                                {
                                    if (q && _pData.getRangeSetting())
                                    {
                                        if (_pData.getRanges(0,0) > _mDataPlots[i][0].a[l] || _pData.getRanges(0,1) < _mDataPlots[i][0].a[l])
                                            continue;
                                    }
                                    if (_pInfo.sCommand != "plot" || q >= 2)
                                    {
                                        if (dDataRanges[q][0] > _mDataPlots[i][q].a[l] || isnan(dDataRanges[q][0]))
                                            dDataRanges[q][0] = _mDataPlots[i][q].a[l];
                                        if (dDataRanges[q][1] < _mDataPlots[i][q].a[l] || isnan(dDataRanges[q][1]))
                                            dDataRanges[q][1] = _mDataPlots[i][q].a[l];
                                    }
                                    else
                                    {
                                        if (sDataAxisBinds[2*i+!q] == 'l' || sDataAxisBinds[2*i+!q] == 'b')
                                        {
                                            if (dDataRanges[q][0] > _mDataPlots[i][q].a[l] || isnan(dDataRanges[q][0]))
                                                dDataRanges[q][0] = _mDataPlots[i][q].a[l];
                                            if (dDataRanges[q][1] < _mDataPlots[i][q].a[l] || isnan(dDataRanges[q][1]))
                                                dDataRanges[q][1] = _mDataPlots[i][q].a[l];
                                        }
                                        else
                                        {
                                            if (dSecDataRanges[q][0] > _mDataPlots[i][q].a[l] || isnan(dSecDataRanges[q][0]))
                                                dSecDataRanges[q][0] = _mDataPlots[i][q].a[l];
                                            if (dSecDataRanges[q][1] < _mDataPlots[i][q].a[l] || isnan(dSecDataRanges[q][1]))
                                                dSecDataRanges[q][1] = _mDataPlots[i][q].a[l];
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                else
                {
                    for (int q = 0; q < nDataDim[i]; q++)
                    {
                        if (_pInfo.b2D && q == 2)///TEMPORARY
                            _mDataPlots[i][q].Create(i_pos[1]-i_pos[0],i_pos[1]-i_pos[0]);
                        else
                            _mDataPlots[i][q].Create(i_pos[1]-i_pos[0]);
                    }

                    // --> Datenspeicher der mglData-Objekte mit den noetigen Daten fuellen <--
                    for (int l = 0; l < i_pos[1]-i_pos[0]; l++)
                    {
                        if (j == 0 && _data.getCols(sDataTable) > j_pos[0])
                        {
                            // --> Nur y-Datenwerte! <--
                            _mDataPlots[i][0].a[l] = l+1+i_pos[0];
                            if (_data.isValidEntry(l+i_pos[0], j_pos[0], sDataTable))
                            {
                                if (!l && (isinf(_data.getElement(l+i_pos[0], j_pos[0], sDataTable)) || isnan(_data.getElement(l+i_pos[0], j_pos[0], sDataTable))))
                                    _mDataPlots[i][1].a[l] = NAN;
                                else if (isinf(_data.getElement(l+i_pos[0], j_pos[0], sDataTable)))
                                    _mDataPlots[i][1].a[l] = NAN;
                                else
                                    _mDataPlots[i][1].a[l] = _data.getElement(l+i_pos[0], j_pos[0], sDataTable);
                            }
                            else if (l)
                                _mDataPlots[i][1].a[l] = NAN;
                            else
                                _mDataPlots[i][1].a[0] = NAN;
                            // --> Pseudo-0-Fehler fuer den Fall, dass Errorplotting aktiviert, aber keine Error-Daten vorhanden sind <--
                            if (_pData.getxError())
                                _mDataPlots[i][2].a[l] = 0.0;
                            if (_pData.getxError() && _pData.getyError())
                                _mDataPlots[i][3].a[l] = 0.0;
                            else if (_pData.getyError())
                                _mDataPlots[i][2].a[l] = 0.0;
                        }
                        else if (_data.getCols(sDataTable) <= j_pos[0] && _pInfo.sCommand != "plot3d")
                        {
                            // --> Dumme Spalten-Fehler abfangen! <--
                            for (int i = 0; i < nDataPlots; i++)
                                delete[] _mDataPlots[i];
                            delete[] _mDataPlots;
                            _mDataPlots = 0;
                            delete[] nDataDim;
                            nDataDim = 0;
                            _data.setCacheStatus(false);
                            throw TOO_FEW_COLS;
                        }
                        else if (nDataDim[i] == 2 && j == 1)
                        {
                            if ((_data.getCols(sDataTable) > j_pos[0]+1 && (j_pos[0] < j_pos[1] || sj_pos[1] == "inf"))
                                || (_data.getCols(sDataTable) > j_pos[1]+1 && j_pos[0] > j_pos[1]))
                            {
                                // --> xy-Datenwerte! <--
                                if (_data.isValidEntry(l+i_pos[0], j_pos[0], sDataTable))
                                {
                                    if (!l && (isnan(_data.getElement(l+i_pos[0], j_pos[0], sDataTable)) || isinf(_data.getElement(l+i_pos[0], j_pos[0], sDataTable))))
                                        _mDataPlots[i][0].a[l] = NAN;
                                    else if (isinf(_data.getElement(l+i_pos[0], j_pos[0], sDataTable)))
                                        _mDataPlots[i][0].a[l] = NAN;
                                    else
                                        _mDataPlots[i][0].a[l] = _data.getElement(l+i_pos[0], j_pos[0], sDataTable);
                                }
                                else if (l)
                                    _mDataPlots[i][0].a[l] = NAN;
                                else
                                    _mDataPlots[i][0].a[0] = NAN;
                                if (_data.isValidEntry(l+i_pos[0], j_pos[1], sDataTable) && sj_pos[1] != "inf")
                                {
                                    if (!l && (isinf(_data.getElement(l+i_pos[0], j_pos[1], sDataTable)) || isnan(_data.getElement(l+i_pos[0], j_pos[1], sDataTable))))
                                        _mDataPlots[i][1].a[l] = NAN;
                                    else if (isinf(_data.getElement(l+i_pos[0], j_pos[1], sDataTable)))
                                        _mDataPlots[i][1].a[l] = NAN;
                                    else
                                        _mDataPlots[i][1].a[l] = _data.getElement(l+i_pos[0], j_pos[1], sDataTable);
                                }
                                else if (_data.isValidEntry(l+i_pos[0], j_pos[0]+1, sDataTable) && sj_pos[1] == "inf")
                                {
                                    if (!l && (isinf(_data.getElement(l+i_pos[0], j_pos[0]+1, sDataTable)) || isnan(_data.getElement(l+i_pos[0], j_pos[0]+1, sDataTable))))
                                        _mDataPlots[i][1].a[l] = NAN;
                                    else if (isinf(_data.getElement(l+i_pos[0], j_pos[0]+1, sDataTable)))
                                        _mDataPlots[i][1].a[l] = NAN;
                                    else
                                        _mDataPlots[i][1].a[l] = _data.getElement(l+i_pos[0], j_pos[0]+1, sDataTable);
                                }
                                else if (l)
                                    _mDataPlots[i][1].a[l] = NAN;
                                else
                                    _mDataPlots[i][1].a[0] = NAN;
                            }
                            else if ((_data.getCols(sDataTable) > j_pos[0] && (j_pos[0] < j_pos[1] || sj_pos[1] == "inf"))
                                || (_data.getCols(sDataTable) > j_pos[1] && j_pos[0] > j_pos[1]))
                            {
                                // --> Reinterpretation der angegeben Spalten, falls eine Spalte zu wenig angegeben wurde <--
                                _mDataPlots[i][0].a[l] = l+1+i_pos[0];
                                if (_data.isValidEntry(l+i_pos[0], j_pos[0], sDataTable) && (j_pos[0] < j_pos[1] || sj_pos[1] == "inf"))
                                {
                                    if (!l && (isnan(_data.getElement(l+i_pos[0], j_pos[0], sDataTable)) || isinf(_data.getElement(l+i_pos[0], j_pos[0], sDataTable))))
                                        _mDataPlots[i][1].a[l] = NAN;
                                    else if (isinf(_data.getElement(l+i_pos[0], j_pos[0], sDataTable)))
                                        _mDataPlots[i][1].a[l] = NAN;
                                    else
                                        _mDataPlots[i][1].a[l] = _data.getElement(l+i_pos[0], j_pos[0], sDataTable);
                                }
                                else if (_data.isValidEntry(l+i_pos[0], j_pos[1], sDataTable) && (j_pos[0] > j_pos[1]))
                                {
                                    if (!l && (isnan(_data.getElement(l+i_pos[0], j_pos[1], sDataTable)) || isinf(_data.getElement(l+i_pos[0], j_pos[1], sDataTable))))
                                        _mDataPlots[i][1].a[l] = NAN;
                                    else if (isinf(_data.getElement(l+i_pos[0], j_pos[1], sDataTable)))
                                        _mDataPlots[i][1].a[l] = NAN;
                                    else
                                        _mDataPlots[i][1].a[l] = _data.getElement(l+i_pos[0], j_pos[1], sDataTable);
                                }
                                else if (l)
                                    _mDataPlots[i][1].a[l] = NAN;
                                else
                                    _mDataPlots[i][1].a[0] = NAN;
                            }
                            else
                            {
                                // --> Dumme Spalten-Fehler abfangen <--
                                for (int i = 0; i < nDataPlots; i++)
                                    delete[] _mDataPlots[i];
                                delete[] _mDataPlots;
                                _mDataPlots = 0;
                                delete[] nDataDim;
                                nDataDim = 0;
                                _data.setCacheStatus(false);
                                throw TOO_FEW_COLS;
                            }
                        }
                        else if (nDataDim[i] >= 3 && j == 1)
                        {
                            // --> xyz-Datenwerte! <--
                            if (j_pos[0] < j_pos[1] || sj_pos[1] == "inf")
                            {
                                for (int q = 0; q < nDataDim[i]; q++)
                                {
                                    if (_pInfo.b2D && q == 2)
                                    {
                                        for (int k = 0; k < i_pos[1]-i_pos[0]; k++)
                                        {
                                            if (_data.num(sDataTable, i_pos[0], i_pos[1], j_pos[0]+1) <= k)
                                            {
                                                _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = NAN;
                                                continue;
                                            }
                                            if (_data.getCols(sDataTable) > q + k + j_pos[0] && _data.isValidEntry(l+i_pos[0], q+k+j_pos[0], sDataTable) && (j_pos[0] <= j_pos[1] || sj_pos[1] == "inf"))
                                            {
                                                if (!l && (isnan(_data.getElement(l+i_pos[0], q + k + j_pos[0], sDataTable)) || isinf(_data.getElement(l+i_pos[0], q + k + j_pos[0], sDataTable))))
                                                    _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = NAN;
                                                else if (isinf(_data.getElement(l+i_pos[0], q+k + j_pos[0], sDataTable)))
                                                    _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = NAN;
                                                else
                                                    _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = _data.getElement(l+i_pos[0], q+k + j_pos[0], sDataTable);
                                            }
                                            else
                                                _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = NAN;
                                        }
                                    }
                                    else
                                    {
                                        // --> Vorwaerts zaehlen <--
                                        if (_data.getCols(sDataTable) > q + j_pos[0] && _data.isValidEntry(l+i_pos[0], q+j_pos[0], sDataTable) && (j_pos[0] <= j_pos[1] || sj_pos[1] == "inf"))
                                        {
                                            if (!l && (isnan(_data.getElement(l+i_pos[0], q + j_pos[0], sDataTable)) || isinf(_data.getElement(l+i_pos[0], q + j_pos[0], sDataTable))))
                                                _mDataPlots[i][q].a[l] = NAN;
                                            else if (isinf(_data.getElement(l+i_pos[0], q + j_pos[0], sDataTable)))
                                                _mDataPlots[i][q].a[l] = NAN;
                                            else
                                                _mDataPlots[i][q].a[l] = _data.getElement(l+i_pos[0], q + j_pos[0], sDataTable);
                                        }
                                        else if (q == 2 && _pInfo.sCommand == "plot3d"
                                            && !(_data.getLines(sDataTable, true)-_data.getAppendedZeroes(q+j_pos[0],sDataTable)-i_pos[0]))
                                            _mDataPlots[i][q].a[l] = 0.0;
                                        else if (l)
                                            _mDataPlots[i][q].a[l] = NAN;
                                        else
                                            _mDataPlots[i][q].a[0] = NAN;
                                    }
                                }
                            }
                            else
                            {
                                for (int q = 0; q < nDataDim[i]; q++)
                                {
                                    if (_pInfo.b2D && q == 2)
                                    {
                                        for (int k = 0; k < i_pos[1]-i_pos[0]; k++)
                                        {
                                            if (_data.num(sDataTable, i_pos[0], i_pos[1], j_pos[0]-1) <= k)
                                            {
                                                _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = NAN;
                                                continue;
                                            }
                                            if (_data.getCols(sDataTable) > j_pos[0] && _data.isValidEntry(l+i_pos[0], j_pos[0]-q-k, sDataTable) && (j_pos[0]-q-k >= j_pos[1] && j_pos[0]-q-k >= 0))
                                            {
                                                if (!l && (isnan(_data.getElement(l+i_pos[0], j_pos[0]-q-k, sDataTable)) || isinf(_data.getElement(l+i_pos[0], j_pos[0]-q-k, sDataTable))))
                                                    _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = NAN;
                                                else if (isinf(_data.getElement(l+i_pos[0], q+k + j_pos[0], sDataTable)))
                                                    _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = NAN;
                                                else
                                                    _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = _data.getElement(l+i_pos[0], j_pos[0]-q-k, sDataTable);
                                            }
                                            else
                                                _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = NAN;
                                        }
                                    }
                                    else
                                    {
                                        // --> Rueckwaerts zaehlen <--
                                        if (_data.getCols(sDataTable) > j_pos[0] && _data.isValidEntry(l+i_pos[0], j_pos[0]-q,sDataTable) && j_pos[0]-q >= 0 && j_pos[0]-q >= j_pos[1])
                                        {
                                            if (!l && (isnan(_data.getElement(l+i_pos[0], j_pos[0]-q, sDataTable)) || isinf(_data.getElement(l+i_pos[0], j_pos[0]-q, sDataTable))))
                                                _mDataPlots[i][q].a[l] = NAN;
                                            else if (isinf(_data.getElement(l+i_pos[0], j_pos[0]-q, sDataTable)))
                                                _mDataPlots[i][q].a[l] = NAN;
                                            else
                                                _mDataPlots[i][q].a[l] = _data.getElement(l+i_pos[0], j_pos[0]-q, sDataTable);
                                        }
                                        else if (q == 2 && _pInfo.sCommand == "plot3d"
                                            && !(_data.getLines(sDataTable, true)-_data.getAppendedZeroes(j_pos[0]-q, sDataTable)-i_pos[0]))
                                            _mDataPlots[i][q].a[l] = 0.0;
                                        else if (l)
                                            _mDataPlots[i][q].a[l] = NAN;
                                        else
                                            _mDataPlots[i][q].a[0] = NAN;
                                    }
                                }
                            }
                        }
                        else
                        {
                            // --> Beliebige Spalten <--
                            //cerr << "arbitrary" << endl;
                            for (int k = 0; k < nDataDim[i]; k++)
                            {
                                //cerr << j_pos[k] << " " << k << endl;
                                if (_pInfo.b2D && k == 2)
                                {
                                    for (int m = 0; m < i_pos[1]-i_pos[0]; m++)
                                    {
                                        if (_data.num(sDataTable, i_pos[0], i_pos[1], j_pos[k-1]) <= m)
                                        {
                                            _mDataPlots[i][k].a[l+(i_pos[1]-i_pos[0])*m] = NAN;
                                            continue;
                                        }
                                        if (_data.getCols(sDataTable) > j_pos[k]+m && _data.isValidEntry(l+i_pos[0], j_pos[k]+m, sDataTable))
                                        {
                                            if (!l && (isnan(_data.getElement(l+i_pos[0], j_pos[k]+m, sDataTable)) || isinf(_data.getElement(l+i_pos[0], j_pos[k]+m, sDataTable))))
                                                _mDataPlots[i][k].a[l+(i_pos[1]-i_pos[0])*m] = NAN;
                                            else if (isinf(_data.getElement(l+i_pos[0], j_pos[k]+m, sDataTable)))
                                                _mDataPlots[i][k].a[l+(i_pos[1]-i_pos[0])*m] = NAN;
                                            else
                                                _mDataPlots[i][k].a[l+(i_pos[1]-i_pos[0])*m] = _data.getElement(l+i_pos[0], j_pos[k]+m, sDataTable);
                                        }
                                    }
                                }
                                else
                                {
                                    if (_data.getCols(sDataTable) > j_pos[k] && _data.isValidEntry(l+i_pos[0], j_pos[k], sDataTable))
                                    {
                                        if (!l && (isnan(_data.getElement(l+i_pos[0], j_pos[k], sDataTable)) || isinf(_data.getElement(l+i_pos[0], j_pos[k], sDataTable))))
                                            _mDataPlots[i][k].a[l] = NAN;
                                        else if (isinf(_data.getElement(l+i_pos[0], j_pos[k], sDataTable)))
                                            _mDataPlots[i][k].a[l] = NAN;
                                        else
                                            _mDataPlots[i][k].a[l] = _data.getElement(l+i_pos[0], j_pos[k], sDataTable);
                                    }
                                    else if (_pInfo.sCommand == "plot3d" && k < 3
                                        && !(_data.getLines(sDataTable, true)-_data.getAppendedZeroes(j_pos[k], sDataTable)-i_pos[0]))
                                    {
                                        _mDataPlots[i][k].a[l] = 0.0;
                                        //cerr << 0.0 << endl;
                                    }
                                    else if (l)
                                        _mDataPlots[i][k].a[l] = NAN;
                                    else
                                        _mDataPlots[i][k].a[0] = NAN;
                                }
                            }
                        }

                        // --> Berechnen der DataRanges <--
                        for (int q = 0; q < 3; q++)
                        {
                            if (q == 2 && nDataDim[i] < 3)
                            {
                                dDataRanges[2][0] = 0;
                                dDataRanges[2][1] = 0;
                                break;
                            }
                            if (l == 0 && i == 0)
                            {
                                if (q && _pData.getRangeSetting(q-1))
                                {
                                    if (_pData.getRanges(q-1,0) > _mDataPlots[i][q-1].a[l] || _pData.getRanges(q-1,1) < _mDataPlots[i][q-1].a[l])
                                    {
                                        if (_pInfo.sCommand != "plot")
                                        {
                                            dDataRanges[q][0] = NAN;
                                            dDataRanges[q][1] = NAN;
                                        }
                                        else
                                        {
                                            if (q < 2)
                                            {
                                                dSecDataRanges[q][0] = NAN;
                                                dSecDataRanges[q][1] = NAN;
                                            }
                                            dDataRanges[q][0] = NAN;
                                            dDataRanges[q][1] = NAN;
                                        }
                                        continue;
                                    }
                                }
                                if (_pInfo.sCommand != "plot" || q >= 2)
                                {
                                    dDataRanges[q][0] = _mDataPlots[i][q].a[l];
                                    dDataRanges[q][1] = _mDataPlots[i][q].a[l];
                                }
                                else
                                {
                                    if (sDataAxisBinds[2*i+!q] == 'l' || sDataAxisBinds[2*i+!q] == 'b')
                                    {
                                        dDataRanges[q][0] = _mDataPlots[i][q].a[l];
                                        dDataRanges[q][1] = _mDataPlots[i][q].a[l];
                                        dSecDataRanges[q][0] = NAN;
                                        dSecDataRanges[q][1] = NAN;
                                    }
                                    else
                                    {
                                        dSecDataRanges[q][0] = _mDataPlots[i][q].a[l];
                                        dSecDataRanges[q][1] = _mDataPlots[i][q].a[l];
                                        dDataRanges[q][0] = NAN;
                                        dDataRanges[q][1] = NAN;
                                    }
                                }
                            }
                            else
                            {
                                if (_pInfo.b2D && q == 2)
                                {
                                    if (_pData.getRangeSetting())
                                    {
                                        if (_pData.getRanges(0,0) > _mDataPlots[i][0].a[l] || _pData.getRanges(0,1) < _mDataPlots[i][0].a[l])
                                            continue;
                                    }
                                    if (_pData.getRangeSetting(1))
                                    {
                                        if (_pData.getRanges(1,0) > _mDataPlots[i][1].a[l] || _pData.getRanges(1,1) < _mDataPlots[i][1].a[l])
                                            continue;
                                    }
                                    for (int k = 0; k < i_pos[1]-i_pos[0]; k++)
                                    {
                                        if (dDataRanges[q][0] > _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] || isnan(dDataRanges[q][0]))
                                            dDataRanges[q][0] = _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k];
                                        if (dDataRanges[q][1] < _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] || isnan(dDataRanges[q][1]))
                                            dDataRanges[q][1] = _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k];
                                    }
                                }
                                else
                                {
                                    if (q && _pData.getRangeSetting())
                                    {
                                        if (_pData.getRanges(0,0) > _mDataPlots[i][0].a[l] || _pData.getRanges(0,1) < _mDataPlots[i][0].a[l])
                                            continue;
                                    }
                                    if (_pInfo.sCommand != "plot" || q >= 2)
                                    {
                                        if (dDataRanges[q][0] > _mDataPlots[i][q].a[l] || isnan(dDataRanges[q][0]))
                                            dDataRanges[q][0] = _mDataPlots[i][q].a[l];
                                        if (dDataRanges[q][1] < _mDataPlots[i][q].a[l] || isnan(dDataRanges[q][1]))
                                            dDataRanges[q][1] = _mDataPlots[i][q].a[l];
                                    }
                                    else
                                    {
                                        if (sDataAxisBinds[2*i+!q] == 'l' || sDataAxisBinds[2*i+!q] == 'b')
                                        {
                                            if (dDataRanges[q][0] > _mDataPlots[i][q].a[l] || isnan(dDataRanges[q][0]))
                                                dDataRanges[q][0] = _mDataPlots[i][q].a[l];
                                            if (dDataRanges[q][1] < _mDataPlots[i][q].a[l] || isnan(dDataRanges[q][1]))
                                                dDataRanges[q][1] = _mDataPlots[i][q].a[l];
                                        }
                                        else
                                        {
                                            if (dSecDataRanges[q][0] > _mDataPlots[i][q].a[l] || isnan(dSecDataRanges[q][0]))
                                                dSecDataRanges[q][0] = _mDataPlots[i][q].a[l];
                                            if (dSecDataRanges[q][1] < _mDataPlots[i][q].a[l] || isnan(dSecDataRanges[q][1]))
                                                dSecDataRanges[q][1] = _mDataPlots[i][q].a[l];
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                //cerr << dSecDataRanges[0][0] << "  " << dSecDataRanges[0][1] << endl;
                //cerr << dSecDataRanges[1][0] << "  " << dSecDataRanges[1][1] << endl;

                // --> Sicherheitshalber den Cache wieder deaktivieren <--
                _data.setCacheStatus(false);
                vLine.clear();
                vCol.clear();
            }
        }
    }
}

void parser_plot_prepareMemory(PlotData& _pData, PlotInfo& _pInfo, mglData* _mAxisVals, const string& sFunc, int nFunctions)
{
    if (!_pInfo.b2D && sFunc != "<<empty>>" && _pInfo.sCommand != "plot3d" && !_pInfo.b3D && !_pInfo.b3DVect && !_pInfo.b2DVect)
        _pData.setDim(_pInfo.nSamples, nFunctions);    // _pInfo.nSamples * nFunctions (Standardplot)
    else if (sFunc != "<<empty>>" && _pInfo.sCommand == "plot3d")
        nFunctions % 3 ? _pData.setDim(_pInfo.nSamples, 3, nFunctions/3+1) : _pData.setDim(_pInfo.nSamples, 3, nFunctions/3);     // _pInfo.nSamples * 3 * _pInfo.nSamples/3 (3D-Trajektorie)
    else if (sFunc != "<<empty>>" && _pInfo.b3D)
        _pData.setDim(_pInfo.nSamples, _pInfo.nSamples, _pInfo.nSamples);    // _pInfo.nSamples * _pInfo.nSamples * _pInfo.nSamples (3D-Plot)
    else if (sFunc != "<<empty>>" && _pInfo.b3DVect)
        _pData.setDim(_pInfo.nSamples, _pInfo.nSamples, 3*_pInfo.nSamples);  // _pInfo.nSamples * _pInfo.nSamples * 3*_pInfo.nSamples (3D-Vektorplot)
    else if (sFunc != "<<empty>>" && _pInfo.b2DVect)
        _pData.setDim(_pInfo.nSamples, _pInfo.nSamples, 2*_pInfo.nSamples);  // _pInfo.nSamples * _pInfo.nSamples * 2*_pInfo.nSamples (2D-Vektorplot)
    else if (sFunc != "<<empty>>")
        _pData.setDim(_pInfo.nSamples, _pInfo.nSamples, nFunctions);  // _pInfo.nSamples * _pInfo.nSamples * nFunctions (2D-Plot)

    _mAxisVals[0].Create(_pInfo.nSamples);
    if (_pInfo.b2D || _pInfo.b3D)
    {
        _mAxisVals[1].Create(_pInfo.nSamples);
        if (_pInfo.b3D)
            _mAxisVals[2].Create(_pInfo.nSamples);
    }
}

void parser_plot_separateLegends(PlotInfo& _pInfo, string& sFunc, string& sLabels)
{
    if (sFunc.length() && !_pInfo.bDraw3D && !_pInfo.bDraw)
    {
        unsigned int n_pos = 0;
        unsigned int n_pos_2 = 0;
        do
        {
            if (sFunc.find('#') != string::npos && sFunc.find('#') < sFunc.find('"'))
                n_pos = sFunc.find('#');
            else
                n_pos = sFunc.find('"');
            n_pos_2 = sFunc.find(',', n_pos);
            if (n_pos_2 == string::npos)
                n_pos_2 = sFunc.length();
            while (sFunc.find(',', n_pos_2+1) != string::npos && isInQuotes(sFunc, n_pos_2))
            {
                n_pos_2 = sFunc.find(',', n_pos_2+1);
            }
            if (n_pos_2 == string::npos || isInQuotes(sFunc, n_pos_2))
                n_pos_2 = sFunc.length();
            sLabels += sFunc.substr(n_pos,n_pos_2-n_pos)+ ";";
            sFunc = sFunc.substr(0,n_pos) + (n_pos_2 < sFunc.length() ? sFunc.substr(n_pos_2) : "");
        }
        while (sFunc.find('"') != string::npos || sFunc.find('#') != string::npos);
        StripSpaces(sLabels);
    }
}

void parser_plot_defaultRanges(PlotData& _pData, PlotInfo& _pInfo, mglData** _mDataPlots, double dDataRanges[3][2], double dSecDataRanges[2][2], int* nDataDim, int nDataPlots, size_t nPlotCompose, bool bNewSubPlot)
{
    if (!nPlotCompose || bNewSubPlot)
    {
        // --> Standard-Ranges zuweisen: wenn weniger als i+1 Ranges gegeben sind und Datenranges vorhanden sind, verwende die Datenranges <--
        for (int i = 0; i < 3; i++)
        {
            if (_pInfo.bDraw3D || _pInfo.bDraw)///Temporary
            {
                _pInfo.dRanges[i][0] = _pData.getRanges(i);
                _pInfo.dRanges[i][1] = _pData.getRanges(i,1);
                continue;
            }
            if (_mDataPlots && (_pData.getGivenRanges() < i+1 || !_pData.getRangeSetting(i)))
            {
                if ((isinf(dDataRanges[i][0]) || isnan(dDataRanges[i][0])) && (unsigned)i < _pInfo.nMaxPlotDim)
                {
                    parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
                    throw PLOTDATA_IS_NAN;
                }
                else if (!(isinf(dDataRanges[i][0]) || isnan(dDataRanges[i][0])))
                    _pInfo.dRanges[i][0] = dDataRanges[i][0];
                else
                    _pInfo.dRanges[i][0] = -10.0;
                if (i < 2 && !isinf(dSecDataRanges[i][0]) && !isnan(dSecDataRanges[i][0]))
                    _pInfo.dSecAxisRanges[i][0] = dSecDataRanges[i][0];
                else if (i < 2)
                    _pInfo.dSecAxisRanges[i][0] = NAN;
            }
            else
            {
                _pInfo.dRanges[i][0] = _pData.getRanges(i);
                if (i < 2)
                    _pInfo.dSecAxisRanges[i][0] = NAN;
            }
            if (_mDataPlots && (_pData.getGivenRanges() < i+1 || !_pData.getRangeSetting(i)))
            {
                if ((isinf(dDataRanges[i][1]) || isnan(dDataRanges[i][1])) && (unsigned)i < _pInfo.nMaxPlotDim)
                {
                    parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
                    throw PLOTDATA_IS_NAN;
                }
                else if (!(isinf(dDataRanges[i][1]) || isnan(dDataRanges[i][1])))
                    _pInfo.dRanges[i][1] = dDataRanges[i][1];
                else
                    _pInfo.dRanges[i][1] = 10.0;
                if (i < 2 && !isinf(dSecDataRanges[i][1]) && !isnan(dSecDataRanges[i][1]))
                    _pInfo.dSecAxisRanges[i][1] = dSecDataRanges[i][1];
                else if (i < 2)
                    _pInfo.dSecAxisRanges[i][1] = NAN;
            }
            else
            {
                _pInfo.dRanges[i][1] = _pData.getRanges(i,1);
                if (i < 2)
                    _pInfo.dSecAxisRanges[i][1] = NAN;
            }
            if (!isnan(_pData.getAddAxis(i).dMin))
            {
                _pInfo.dSecAxisRanges[i][0] = _pData.getAddAxis(i).dMin;
                _pInfo.dSecAxisRanges[i][1] = _pData.getAddAxis(i).dMax;
            }
        }

        // --> Spezialfall: Wenn nur eine Range gegeben ist, verwende im 3D-Fall diese fuer alle drei benoetigten Ranges <--
        if (_pData.getGivenRanges() == 1 && (_pInfo.b3D || _pInfo.b3DVect))
        {
            for (int i = 1; i < 3; i++)
            {
                _pInfo.dRanges[i][0] = _pInfo.dRanges[0][0];
                _pInfo.dRanges[i][1] = _pInfo.dRanges[0][1];
            }
        }
    }
    // --> Sonderkoordinatensaetze und dazu anzugleichende Ranges. Noch nicht korrekt implementiert <--
    if (!_pData.getCoords())
    {
        /* --> Im Falle logarithmischer Plots muessen die Darstellungsintervalle angepasst werden. Falls
         *     die Intervalle zu Teilen im Negativen liegen, versuchen wir trotzdem etwas sinnvolles
         *     daraus zu machen. <--
         */
        if (_pData.getxLogscale())
        {
            if (_pInfo.dRanges[0][0] <= 0 && _pInfo.dRanges[0][1] > 0)
            {
                _pInfo.dRanges[0][0] = _pInfo.dRanges[0][1] / 1e3;
            }
            else if (_pInfo.dRanges[0][0] < 0 && _pInfo.dRanges[0][1] <= 0)
            {
                parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
                throw WRONG_PLOT_INTERVAL_FOR_LOGSCALE;
            }
        }

        if (_pData.getyLogscale())
        {
            if (_pInfo.dRanges[1][0] <= 0 && _pInfo.dRanges[1][1] > 0)
            {
                _pInfo.dRanges[1][0] = _pInfo.dRanges[1][1] / 1e3;
            }
            else if (_pInfo.dRanges[1][0] < 0 && _pInfo.dRanges[1][1] <= 0)
            {
                parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
                throw WRONG_PLOT_INTERVAL_FOR_LOGSCALE;
            }
        }

        if (_pData.getzLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
        {
            if (_pInfo.dRanges[2][0] <= 0 && _pInfo.dRanges[2][1] > 0)
            {
                _pInfo.dRanges[2][0] = _pInfo.dRanges[2][1] / 1e3;
            }
            else if (_pInfo.dRanges[2][0] < 0 && _pInfo.dRanges[2][1] <= 0)
            {
                parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
                throw WRONG_PLOT_INTERVAL_FOR_LOGSCALE;
            }
        }
    }
    else if (_pData.getCoords())
    {
        // --> Im Falle polarer oder sphaerischer Koordinaten muessen die Darstellungsintervalle angepasst werden <--
        if (_pData.getCoords() == 1)
        {
            if (_pInfo.sCommand.find("3d") == string::npos && !_pInfo.b2DVect)
            {
                if (!_pData.getRangeSetting())
                {
                    _pInfo.dRanges[0][0] = 0.0;
                    _pInfo.dRanges[0][1] = 2*M_PI;
                }
            }
            else if (_pInfo.sCommand.find("3d") == string::npos)
            {
                _pInfo.dRanges[0][0] = 0.0;
                if (!_pData.getRangeSetting(1))
                {
                    _pInfo.dRanges[1][0] = 0.0;
                    _pInfo.dRanges[1][1] = 2*M_PI;
                }
            }
            else
            {
                _pInfo.dRanges[0][0] = 0.0;
                if (!_pData.getRangeSetting(1))
                {
                    _pInfo.dRanges[1][0] = 0.0;
                    _pInfo.dRanges[1][1] = 2*M_PI;
                }
            }
            if (!(_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect || _pInfo.b2DVect) && !_pData.getRangeSetting(1))
                _pInfo.dRanges[1][0] = 0.0;
        }
        else if (_pData.getCoords() == 2)
        {
            if (_pInfo.sCommand.find("3d") == string::npos)
            {
                if (!_pData.getRangeSetting())
                {
                    _pInfo.dRanges[0][0] = 0.0;
                    _pInfo.dRanges[0][1] = 2*M_PI;
                }
            }
            else
            {
                if (!_pData.getRangeSetting(1))
                {
                    _pInfo.dRanges[1][0] = 0.0;
                    _pInfo.dRanges[1][1] = 2*M_PI;
                }
            }
            if (!(_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect || _pInfo.b2DVect) && !_pData.getRangeSetting(1))
                _pInfo.dRanges[1][0] = 0.0;
            else if (_pInfo.sCommand.find("3d") != string::npos)
            {
                _pInfo.dRanges[0][0] = 0.0;
                if (!_pData.getRangeSetting(2))
                {
                    _pInfo.dRanges[2][0] = 0.0;
                    _pInfo.dRanges[2][1] = M_PI;
                }
            }
            else
            {
                _pInfo.dRanges[2][0] = 0.0;
                if (!_pData.getRangeSetting(1))
                {
                    _pInfo.dRanges[1][0] = 0.0;
                    _pInfo.dRanges[1][1] = M_PI;
                }
            }
        }
    }
}

int parser_plot_fillData(PlotData& _pData, PlotInfo& _pInfo, Parser& _parser, mglData* _mAxisVals, const string& sFunc, value_type* vResults, double dt_max, int t_animate, int nFunctions)
{
    // --> Plot-Speicher mit den berechneten Daten fuellen <--
    if (!_pInfo.b2D && sFunc != "<<empty>>" && _pInfo.sCommand != "plot3d" && !_pInfo.b3D && !_pInfo.b3DVect && !_pInfo.b2DVect)
    {
        for (int x = 0; x < _pInfo.nSamples; x++)
        {
            if (x != 0)
            {
                if (_pData.getxLogscale())
                    parser_iVars.vValue[0][0] = pow(10.0,log10(_pInfo.dRanges[0][0]) + (log10(_pInfo.dRanges[0][1]) - log10(_pInfo.dRanges[0][0])) / (double)(_pInfo.nSamples-1)*(double)x);
                else
                    parser_iVars.vValue[0][0] += (_pInfo.dRanges[0][1] - _pInfo.dRanges[0][0]) / (double)(_pInfo.nSamples-1);
            }
            _mAxisVals[0].a[x] = parser_iVars.vValue[0][0];
            vResults = _parser.Eval(nFunctions);
            for (int i = 0; i < nFunctions; i++)
            {
                if (isinf(vResults[i]) || isnan(vResults[i]))
                {
                    if (!x)
                        _pData.setData(x, i, NAN);
                    else
                        _pData.setData(x, i, NAN);
                }
                else
                    _pData.setData(x, i, (double)vResults[i]);
            }
        }
    }
    else if (sFunc != "<<empty>>" && _pInfo.b3D)
    {
        //mglPoint _mLowerEdge = parser_CalcCutBox(_pData.getRotateAngle(1), _pInfo.dRanges, 0, _pData.getCoords(), true);
        //mglPoint _mHigherEdge = parser_CalcCutBox(_pData.getRotateAngle(1), _pInfo.dRanges, 1, _pData.getCoords(), true);
        // --> Wie oben, aber 3D-Fall <--
        for (int x = 0; x < _pInfo.nSamples; x++)
        {
            if (x != 0)
            {
                if (_pData.getxLogscale())
                    parser_iVars.vValue[0][0] = pow(10.0,log10(_pInfo.dRanges[0][0]) + (log10(_pInfo.dRanges[0][1]) - log10(_pInfo.dRanges[0][0])) / (double)(_pInfo.nSamples-1)*(double)x);
                else
                    parser_iVars.vValue[0][0] += (_pInfo.dRanges[0][1] - _pInfo.dRanges[0][0]) / (double)(_pInfo.nSamples-1);
            }
            parser_iVars.vValue[1][0] = _pInfo.dRanges[1][0];
            _mAxisVals[0].a[x] = parser_iVars.vValue[0][0];
            for (int y = 0; y < _pInfo.nSamples; y++)
            {
                if (y != 0)
                {
                    if (_pData.getyLogscale())
                        parser_iVars.vValue[1][0] = pow(10.0,log10(_pInfo.dRanges[1][0]) + (log10(_pInfo.dRanges[1][1]) - log10(_pInfo.dRanges[1][0])) / (double)(_pInfo.nSamples-1)*(double)y);
                    else
                        parser_iVars.vValue[1][0] += (_pInfo.dRanges[1][1] - _pInfo.dRanges[1][0]) / (double)(_pInfo.nSamples-1);
                }
                parser_iVars.vValue[2][0] = _pInfo.dRanges[2][0];
                if (!x)
                    _mAxisVals[1].a[y] = parser_iVars.vValue[1][0];
                for (int z = 0; z < _pInfo.nSamples; z++)
                {
                    if (z != 0)
                    {
                        if (_pData.getyLogscale())
                            parser_iVars.vValue[2][0] = pow(10.0,log10(_pInfo.dRanges[2][0]) + (log10(_pInfo.dRanges[2][1]) - log10(_pInfo.dRanges[2][0])) / (double)(_pInfo.nSamples-1)*(double)y);
                        else
                            parser_iVars.vValue[2][0] += (_pInfo.dRanges[2][1] - _pInfo.dRanges[2][0]) / (double)(_pInfo.nSamples-1);
                    }
                    if (!x && !y)
                        _mAxisVals[2].a[z] = parser_iVars.vValue[2][0];
                    double dResult = _parser.Eval();
                    //vResults = &_parser.Eval();

                    if (isnan(dResult)
                        || isinf(dResult))
                    {
                        _pData.setData(x, y, NAN, z);
                    }
                    else
                        _pData.setData(x, y, dResult, z);
                }
            }
        }

    }
    else if (sFunc != "<<empty>>" && _pInfo.sCommand == "plot3d")
    {
        // --> Parametrische Kurven berechnen <--
        for (int k = 0; k < _pData.getLayers(); k++)
        {
            parser_iVars.vValue[0][0] = 0.0;
            parser_iVars.vValue[1][0] = 0.0;
            parser_iVars.vValue[2][0] = 0.0;
            parser_iVars.vValue[3][0] = _pData.gettBoundary();
            vResults = _parser.Eval(nFunctions);
            parser_iVars.vValue[0][0] = vResults[3*k];
            if (3*k+1 < nFunctions)
                parser_iVars.vValue[1][0] = vResults[3*k+1];
            if (3*k+2 < nFunctions)
                parser_iVars.vValue[2][0] = vResults[3+k+2];
            int nRenderSamples = _pInfo.nSamples;
            for (int t = 0; t < nRenderSamples; t++)
            {
                if (t != 0)
                {
                    if (_pData.getAnimateSamples())
                    {
                        double dSamples = 1.0;
                        if ((t_animate*100.0)/_pData.getAnimateSamples() <= 25.0)
                        {
                            dSamples = (double)(_pInfo.nSamples-1) * 0.25;
                        }
                        else if ((t_animate*100.0)/_pData.getAnimateSamples() <= 50.0)
                        {
                            dSamples = (double)(_pInfo.nSamples-1) * 0.5;
                        }
                        else if ((t_animate*100.0)/_pData.getAnimateSamples() <= 75.0)
                        {
                            dSamples = (double)(_pInfo.nSamples-1) * 0.75;
                        }
                        else
                        {
                            dSamples = (double)(_pInfo.nSamples-1);
                        }
                        nRenderSamples = (int)dSamples + 1;
                        parser_iVars.vValue[3][0] += (dt_max - _pData.gettBoundary()) / dSamples;
                    }
                    else
                        parser_iVars.vValue[3][0] += (_pData.gettBoundary(1) - _pData.gettBoundary()) / (double)(_pInfo.nSamples-1);
                    parser_iVars.vValue[0][0] = _pData.getData(t-1,0,k);
                    parser_iVars.vValue[1][0] = _pData.getData(t-1,1,k);
                    parser_iVars.vValue[2][0] = _pData.getData(t-1,2,k);
                }
                // --> Wir werten alle Koordinatenfunktionen zugleich aus und verteilen sie auf die einzelnen Parameterkurven <--
                vResults = _parser.Eval(nFunctions);
                for (int i = 0; i < 3; i++)
                {
                    if (i+3*k >= nFunctions)
                        break;
                    if (isinf(vResults[i+3*k]) || isnan(vResults[i+3+k]))
                    {
                        if (!t)
                            _pData.setData(t, i, NAN, k);
                        else
                            _pData.setData(t, i, NAN, k);
                    }
                    else
                        _pData.setData(t, i, (double)vResults[i+3*k], k);
                }
            }
            for (int t = nRenderSamples; t < _pInfo.nSamples; t++)
            {
                for (int i = 0; i < 3; i++)
                    _pData.setData(t,i, NAN,k);
            }
        }
        parser_iVars.vValue[3][0] = dt_max;
    }
    else if (sFunc != "<<empty>>" && _pInfo.b3DVect)
    {
        // --> Vektorfeld (3D= <--
        for (int x = 0; x < _pInfo.nSamples; x++)
        {
            if (x != 0)
            {
                parser_iVars.vValue[0][0] += (_pInfo.dRanges[0][1] - _pInfo.dRanges[0][0]) / (double)(_pInfo.nSamples-1);
            }
            parser_iVars.vValue[1][0] = _pInfo.dRanges[1][0];
            for (int y = 0; y < _pInfo.nSamples; y++)
            {
                if (y != 0)
                {
                    parser_iVars.vValue[1][0] += (_pInfo.dRanges[1][1] - _pInfo.dRanges[1][0]) / (double)(_pInfo.nSamples-1);
                }
                parser_iVars.vValue[2][0] = _pInfo.dRanges[2][0];
                for (int z = 0; z < _pInfo.nSamples; z++)
                {
                    if (z != 0)
                    {
                        parser_iVars.vValue[2][0] += (_pInfo.dRanges[2][1] - _pInfo.dRanges[2][0]) / (double)(_pInfo.nSamples-1);
                    }
                    vResults = _parser.Eval(nFunctions);

                    for (int i = 0; i < nFunctions; i++)
                    {
                        if (i > 2)
                            break;
                        if (isnan(vResults[i]) || isinf(vResults[i]))
                        {
                            if (!x || !y)
                                _pData.setData(x, y, NAN, 3*z+i);
                            else
                                _pData.setData(x, y, NAN, 3*z+i);
                        }
                        else
                            _pData.setData(x, y, (double)vResults[i], 3*z+i);
                    }
                    /*if (_option.getbDebug())
                        cerr << "|-> DEBUG: (x = " << x << ", y = " << y << ", z = " << z << ") " << _parser.Eval() << " / " << _pData.getData(x,y,z) << endl;*/
                }
            }
        }
    }
    else if (sFunc != "<<empty>>" && _pInfo.b2DVect)
    {
        // --> Wie oben, aber 2D-Fall <--
        for (int x = 0; x < _pInfo.nSamples; x++)
        {
            if (x != 0)
            {
                parser_iVars.vValue[0][0] += (_pInfo.dRanges[0][1] - _pInfo.dRanges[0][0]) / (double)(_pInfo.nSamples-1);
            }
            parser_iVars.vValue[1][0] = _pInfo.dRanges[1][0];
            for (int y = 0; y < _pInfo.nSamples; y++)
            {
                if (y != 0)
                {
                    parser_iVars.vValue[1][0] += (_pInfo.dRanges[1][1] - _pInfo.dRanges[1][0]) / (double)(_pInfo.nSamples-1);
                }
                vResults = _parser.Eval(nFunctions);

                for (int i = 0; i < nFunctions; i++)
                {
                    if (i > 1)
                        break;
                    if (isnan(vResults[i]) || isinf(vResults[i]))
                    {
                        if (!x || !y)
                            _pData.setData(x, y, NAN, i);
                        else
                            _pData.setData(x, y, NAN, i);
                    }
                    else
                        _pData.setData(x, y, (double)vResults[i], i);
                }
            /*if (_option.getbDebug())
                cerr << "|-> DEBUG: (x = " << x << ", y = " << y << ", z = " << z << ") " << _parser.Eval() << " / " << _pData.getData(x,y,z) << endl;*/
            }
        }
    }
    else if (sFunc != "<<empty>>")
    {
        // --> Wie oben, aber 2D-Fall <--
        for (int x = 0; x < _pInfo.nSamples; x++)
        {
            if (x != 0)
            {
                if (_pData.getxLogscale())
                    parser_iVars.vValue[0][0] = pow(10.0,log10(_pInfo.dRanges[0][0]) + (log10(_pInfo.dRanges[0][1]) - log10(_pInfo.dRanges[0][0])) / (double)(_pInfo.nSamples-1)*(double)x);
                else
                    parser_iVars.vValue[0][0] += (_pInfo.dRanges[0][1] - _pInfo.dRanges[0][0]) / (double)(_pInfo.nSamples-1);
            }
            parser_iVars.vValue[1][0] = _pInfo.dRanges[1][0];
            _mAxisVals[0].a[x] = parser_iVars.vValue[0][0];
            for (int y = 0; y < _pInfo.nSamples; y++)
            {
                if (y != 0)
                {
                    if (_pData.getyLogscale())
                        parser_iVars.vValue[1][0] = pow(10.0,log10(_pInfo.dRanges[1][0]) + (log10(_pInfo.dRanges[1][1]) - log10(_pInfo.dRanges[1][0])) / (double)(_pInfo.nSamples-1)*(double)y);
                    else
                        parser_iVars.vValue[1][0] += (_pInfo.dRanges[1][1] - _pInfo.dRanges[1][0]) / (double)(_pInfo.nSamples-1);
                }
                if (!x)
                    _mAxisVals[1].a[y] = parser_iVars.vValue[1][0];
                vResults = _parser.Eval(nFunctions);
                for (int i = 0; i < nFunctions; i++)
                {
                    if (isnan(vResults[i]) || isinf(vResults[i]))
                    {
                        if (!x || !y)
                            _pData.setData(x, y, NAN, i);
                        else
                            _pData.setData(x, y, NAN, i);
                    }
                    else
                        _pData.setData(x, y, (double)vResults[i], i);
                }
            }
        }
    }

    return nFunctions;
}

void parser_plot_fitPlotRanges(PlotData& _pData, PlotInfo& _pInfo, mglData** _mDataPlots, const string& sFunc, int* nDataDim, int nDataPlots, double dDataRanges[3][2], size_t nPlotCompose, bool bNewSubPlot)
{
    /* --> Darstellungsintervalle anpassen: Wenn nicht alle vorgegeben sind, sollten die fehlenden
     *     passend berechnet werden. Damit aber kein Punkt auf dem oberen oder dem unteren Rahmen
     *     liegt, vergroessern wir das Intervall um 5% nach oben und 5% nach unten <--
     * --> Fuer Vektor- und 3D-Plots ist das allerdings recht sinnlos <--
     */
    if (sFunc != "<<empty>>")
    {
        if (isnan(_pData.getMin()) || isnan(_pData.getMax()) || isinf(_pData.getMin()) || isinf(_pData.getMax()))
        {
            parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
            throw PLOTDATA_IS_NAN;
        }
    }
    if (!(_pInfo.b3D || _pInfo.b3DVect || _pInfo.b2DVect) && (!nPlotCompose || bNewSubPlot) && !(_pInfo.bDraw3D || _pInfo.bDraw))
    {
        if (!_pInfo.b2D && _pData.getGivenRanges() < 2 && _pInfo.sCommand != "plot3d")
        {
            if (sFunc != "<<empty>>")
            {
                if (_mDataPlots && _pData.getMin(-2) < _pInfo.dRanges[1][0])
                    _pInfo.dRanges[1][0] = _pData.getMin(-2);
                if (_mDataPlots && _pData.getMax(-2) > _pInfo.dRanges[1][1])
                    _pInfo.dRanges[1][1] = _pData.getMax(-2);
                if (!_mDataPlots)
                {
                    _pInfo.dRanges[1][0] = _pData.getMin(-2);
                    _pInfo.dRanges[1][1] = _pData.getMax(-2);
                }
                if (_mDataPlots && (_pData.getMin(-3) < _pInfo.dSecAxisRanges[1][0] || isnan(_pInfo.dSecAxisRanges[1][0])))
                    _pInfo.dSecAxisRanges[1][0] = _pData.getMin(-3);
                if (_mDataPlots && (_pData.getMax(-3) > _pInfo.dSecAxisRanges[1][1] || isnan(_pInfo.dSecAxisRanges[1][1])))
                    _pInfo.dSecAxisRanges[1][1] = _pData.getMax(-3);
                if (!_mDataPlots)
                {
                    _pInfo.dSecAxisRanges[1][0] = _pData.getMin(-3);
                    _pInfo.dSecAxisRanges[1][1] = _pData.getMax(-3);
                    if (!isnan(_pData.getAddAxis(1).dMin))
                    {
                        _pInfo.dSecAxisRanges[1][0] = _pData.getAddAxis(1).dMin;
                        _pInfo.dSecAxisRanges[1][1] = _pData.getAddAxis(1).dMax;
                    }
                }
                if ((isnan(_pData.getMin(-2)) || isnan(_pInfo.dRanges[1][0])) && isnan(dDataRanges[1][0]))
                {
                    _pInfo.dRanges[1][0] = _pInfo.dSecAxisRanges[1][0];
                    _pInfo.dRanges[1][0] = _pInfo.dSecAxisRanges[1][1];
                }
            }
            double dInt = fabs(_pInfo.dRanges[1][1] - _pInfo.dRanges[1][0]);
            if (dInt == 0.0 || (dInt < 1e-4 * _pInfo.dRanges[1][0]))
                dInt = fabs(_pInfo.dRanges[1][1]);
            _pInfo.dRanges[1][0] -= dInt / 20.0;
            if (_pInfo.dRanges[1][0] <= 0 && _pData.getyLogscale())
                _pInfo.dRanges[1][0] += dInt / 20.0;
            if (_pInfo.dRanges[1][0] < 0.0 && _pData.getCoords())
                _pInfo.dRanges[1][0] = 0.0;
            _pInfo.dRanges[1][1] += dInt / 20.0;
            dInt = fabs(_pInfo.dSecAxisRanges[1][1] - _pInfo.dSecAxisRanges[1][0]);
            if (!isnan(dInt) && isnan(_pData.getAddAxis(1).dMin))
            {
                if (dInt == 0.0 || (dInt < 1e-4 * _pInfo.dSecAxisRanges[1][0]))
                    dInt = fabs(_pInfo.dSecAxisRanges[1][1]);
                _pInfo.dSecAxisRanges[1][0] -= dInt / 20.0;
                if (_pInfo.dSecAxisRanges[1][0] <= 0 && _pData.getyLogscale())
                    _pInfo.dSecAxisRanges[1][0] += dInt / 20.0;
                if (_pInfo.dSecAxisRanges[1][0] < 0.0 && _pData.getCoords())
                    _pInfo.dSecAxisRanges[1][0] = 0.0;
                _pInfo.dSecAxisRanges[1][1] += dInt / 20.0;
            }
            for (int i = 0; i < 2; i++)
                _pData.setAddAxis(i, _pInfo.dSecAxisRanges[i][0], _pInfo.dSecAxisRanges[i][1]);
        }
        else if (_pInfo.sCommand == "plot3d" && _pData.getGivenRanges() < 3)
        {
            if (sFunc != "<<empty>>")
            {
                for (int i = 0; i < 3; i++)
                {
                    if (_pData.getGivenRanges() >= i+1 && _pData.getRangeSetting(i))
                        continue;
                    if (_mDataPlots && _pData.getMin(i) < _pInfo.dRanges[i][0])
                        _pInfo.dRanges[i][0] = _pData.getMin(i);
                    if (_mDataPlots && _pData.getMax(i) > _pInfo.dRanges[i][1])
                        _pInfo.dRanges[i][1] = _pData.getMax(i);
                    if (!_mDataPlots)
                    {
                        _pInfo.dRanges[i][0] = _pData.getMin(i);
                        _pInfo.dRanges[i][1] = _pData.getMax(i);
                    }
                    double dInt = fabs(_pInfo.dRanges[i][1] - _pInfo.dRanges[i][0]);
                    if (dInt == 0.0 || dInt < 1e-4 * _pInfo.dRanges[i][0])
                        dInt = fabs(_pInfo.dRanges[i][1]);
                    _pInfo.dRanges[i][0] -= dInt / 20.0;
                    _pInfo.dRanges[i][1] += dInt / 20.0;
                    if ((_pInfo.dRanges[i][0] < 0.0 && (_pData.getCoords() == 2 || (_pData.getCoords() == 1 && i < 2))) || (_pInfo.dRanges[i][0] && _pData.getCoords() && !i))
                        _pInfo.dRanges[i][0] = 0.0;
                }
                if (_pData.getCoords() && _pInfo.dRanges[1][0] != 0.0)
                    _pInfo.dRanges[1][0] = 0.0;
                if (_pData.getCoords() == 2 && _pInfo.dRanges[2][0] != 0.0)
                    _pInfo.dRanges[2][0] = 0.0;
            }
        }
        else if (_pData.getGivenRanges() < 3)
        {
            if (sFunc != "<<empty>>")
            {
                if (_mDataPlots && _pData.getMin() < _pInfo.dRanges[2][0])
                    _pInfo.dRanges[2][0] = _pData.getMin();
                if (_mDataPlots && _pData.getMax() > _pInfo.dRanges[2][1])
                    _pInfo.dRanges[2][1] = _pData.getMax();
                if (!_mDataPlots)
                {
                    _pInfo.dRanges[2][0] = _pData.getMin();
                    _pInfo.dRanges[2][1] = _pData.getMax();
                }
            }
            double dInt = fabs(_pInfo.dRanges[2][1] - _pInfo.dRanges[2][0]);
            if (dInt == 0.0 || dInt < 1e-4 * _pInfo.dRanges[2][0])
                dInt = fabs(_pInfo.dRanges[2][1]);
            _pInfo.dRanges[2][0] -= dInt / 20.0;
            if (_pInfo.dRanges[2][0] <= 0 && _pData.getzLogscale())
                _pInfo.dRanges[2][0] += dInt / 20.0;
            if (_pInfo.dRanges[2][0] < 0.0 && _pData.getCoords())
                _pInfo.dRanges[2][0] = 0.0;
            _pInfo.dRanges[2][1] += dInt / 20.0;
        }
    }
    else if (_pInfo.b2DVect && (!nPlotCompose || bNewSubPlot) && !(_pInfo.bDraw3D || _pInfo.bDraw))
    {
        if (sFunc != "<<empty>>")
        {
            if (_pData.getGivenRanges() < 3)
            {
                _pInfo.dRanges[2][0] = _pData.getMin();
                _pInfo.dRanges[2][1] = _pData.getMax();
            }
            if (_pData.getCoords())
            {
                _pInfo.dRanges[1][0] = 0.0;
                _pInfo.dRanges[0][0] = 0.0;
            }
        }
    }
    if (_pData.getxLogscale() || _pData.getyLogscale() || _pData.getzLogscale())
    {
        if (_pData.getxLogscale())
        {
            if ((_pInfo.dRanges[0][0] <= 0 && _pInfo.dRanges[0][1] <= 0) || _pData.getAxisScale() <= 0.0)
            {
                parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
                throw WRONG_PLOT_INTERVAL_FOR_LOGSCALE;
            }
            else if (_pInfo.dRanges[0][0] <= 0)
                _pInfo.dRanges[0][0] = _pInfo.dRanges[0][1]*1e-3;
        }
        if (_pData.getyLogscale())
        {
            if ((_pInfo.dRanges[1][0] <= 0 && _pInfo.dRanges[1][1] <= 0) || _pData.getAxisScale(1) <= 0.0)
            {
                parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
                throw WRONG_PLOT_INTERVAL_FOR_LOGSCALE;
            }
            else if (_pInfo.dRanges[1][0] <= 0)
                _pInfo.dRanges[1][0] = _pInfo.dRanges[1][1]*1e-3;
        }
        if (_pData.getzLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
        {
            if ((_pInfo.dRanges[2][0] <= 0 && _pInfo.dRanges[2][1] <= 0) || _pData.getAxisScale(2) <= 0.0)
            {
                parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
                throw WRONG_PLOT_INTERVAL_FOR_LOGSCALE;
            }
            else if (_pInfo.dRanges[2][0] <= 0)
                _pInfo.dRanges[2][0] = _pInfo.dRanges[2][1]*1e-3;
        }
    }
}

void parser_plot_clearData(mglData** _mDataPlots, int* nDataDim, int nDataPlots)
{
    if (_mDataPlots)
    {
        for (int i = 0; i < nDataPlots; i++)
            delete[] _mDataPlots[i];
        delete[] _mDataPlots;
        _mDataPlots = 0;
        delete[] nDataDim;
        nDataDim = 0;
    }
}

void parser_plot_passRangesToGraph(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo, mglData** _mDataPlots, mglData* _mAxisVals, const string& sFunc, int* nDataDim, int nDataPlots, double dDataRanges[3][2])
{
    if (_pData.getBoxplot() && !_pData.getRangeSetting() && nDataPlots)
    {
        _pInfo.dRanges[0][0] = 0;
        _pInfo.dRanges[0][1] = nDataPlots+1;
    }
    if (_pData.getInvertion())
        _graph.SetRange('x', _pInfo.dRanges[0][1]/_pData.getAxisScale(), _pInfo.dRanges[0][0]/_pData.getAxisScale());
    else
        _graph.SetRange('x', _pInfo.dRanges[0][0]/_pData.getAxisScale(), _pInfo.dRanges[0][1]/_pData.getAxisScale());
    if (_pData.getInvertion(1))
        _graph.SetRange('y', _pInfo.dRanges[1][1]/_pData.getAxisScale(1), _pInfo.dRanges[1][0]/_pData.getAxisScale(1));
    else
        _graph.SetRange('y', _pInfo.dRanges[1][0]/_pData.getAxisScale(1), _pInfo.dRanges[1][1]/_pData.getAxisScale(1));
    if (_pData.getInvertion(2))
        _graph.SetRange('z', _pInfo.dRanges[2][1]/_pData.getAxisScale(2), _pInfo.dRanges[2][0]/_pData.getAxisScale(2));
    else
        _graph.SetRange('z', _pInfo.dRanges[2][0]/_pData.getAxisScale(2), _pInfo.dRanges[2][1]/_pData.getAxisScale(2));

    if (_pData.getBars() && _mDataPlots && !_pInfo.b2D)
    {
        int nMinbars = -1;
        for (int k = 0; k < nDataPlots; k++)
        {
            if (nMinbars == -1 || nMinbars > _mDataPlots[k][0].nx)
                nMinbars = _mDataPlots[k][0].nx;
        }
        if (nMinbars < 2)
            nMinbars = 2;
        if (_pData.getInvertion())
            _graph.SetRange('x',(_pInfo.dRanges[0][1]-fabs(_pInfo.dRanges[0][0]-_pInfo.dRanges[0][1])/(double)(2*(nMinbars-1)))/_pData.getAxisScale(), (_pInfo.dRanges[0][0]+fabs(_pInfo.dRanges[0][0]-_pInfo.dRanges[0][1])/(double)(2*(nMinbars-1)))/_pData.getAxisScale());
        else
            _graph.SetRange('x',(_pInfo.dRanges[0][0]-fabs(_pInfo.dRanges[0][0]-_pInfo.dRanges[0][1])/(double)(2*(nMinbars-1)))/_pData.getAxisScale(), (_pInfo.dRanges[0][1]+fabs(_pInfo.dRanges[0][0]-_pInfo.dRanges[0][1])/(double)(2*(nMinbars-1)))/_pData.getAxisScale());
        if (_pInfo.sCommand == "plot3d")
        {
            if (_pData.getInvertion(1))
                _graph.SetRange('y',(_pInfo.dRanges[1][1]-fabs(_pInfo.dRanges[1][0]-_pInfo.dRanges[1][1])/(double)(2*(nMinbars-1)))/_pData.getAxisScale(1), (_pInfo.dRanges[1][0]+fabs(_pInfo.dRanges[1][0]-_pInfo.dRanges[1][1])/(double)(2*(nMinbars-1)))/_pData.getAxisScale(1));
            else
                _graph.SetRange('y',(_pInfo.dRanges[1][0]-fabs(_pInfo.dRanges[1][0]-_pInfo.dRanges[1][1])/(double)(2*(nMinbars-1)))/_pData.getAxisScale(1), (_pInfo.dRanges[1][1]+fabs(_pInfo.dRanges[1][0]-_pInfo.dRanges[1][1])/(double)(2*(nMinbars-1)))/_pData.getAxisScale(1));
        }
    }

    if (!isnan(_pData.getColorRange()) && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
    {
        if (_pData.getcLogscale() && ((_pData.getColorRange() <= 0.0 && _pData.getColorRange(1) <= 0.0) || _pData.getAxisScale(3) <= 0.0))
        {
            parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
            throw WRONG_PLOT_INTERVAL_FOR_LOGSCALE;
        }
        else if (_pData.getcLogscale() && _pData.getColorRange() <= 0.0)
        {
            _graph.SetRange('c', _pData.getColorRange(1)*1e-3/_pData.getAxisScale(3), _pData.getColorRange(1)/_pData.getAxisScale(3));
            _pInfo.dColorRanges[0] = _pData.getColorRange(1)*1e-3/_pData.getAxisScale(3);
            _pInfo.dColorRanges[1] = _pData.getColorRange(1)/_pData.getAxisScale(3);
        }
        else
        {
            _graph.SetRange('c', _pData.getColorRange()/_pData.getAxisScale(3), _pData.getColorRange(1)/_pData.getAxisScale(3));
            _pInfo.dColorRanges[0] = _pData.getColorRange()/_pData.getAxisScale(3);
            _pInfo.dColorRanges[1] = _pData.getColorRange(1)/_pData.getAxisScale(3);
        }
    }
    else if (_mDataPlots)
    {
        double dColorMin = dDataRanges[2][0]/_pData.getAxisScale(3);
        double dColorMax = dDataRanges[2][1]/_pData.getAxisScale(3);
        if (_pData.getMax() > dColorMax && sFunc != "<<empty>>" && _pInfo.sCommand == "plot3d")
            dColorMax = _pData.getMax(2)/_pData.getAxisScale(3);
        if (_pData.getMin() < dColorMin && sFunc != "<<empty>>" && _pInfo.sCommand == "plot3d")
            dColorMin = _pData.getMin(2)/_pData.getAxisScale(3);
        if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && dColorMin <= 0.0 && dColorMax <= 0.0)
        {
            parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
            throw WRONG_PLOT_INTERVAL_FOR_LOGSCALE;
        }
        else if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && dColorMin <= 0.0)
        {
            dColorMin = dColorMax * 1e-3;
            _graph.SetRange('c', dColorMin, dColorMax+0.05*(dColorMax-dColorMin));
            _pInfo.dColorRanges[0] = dColorMin;
            _pInfo.dColorRanges[1] = dColorMax+0.05*(dColorMax-dColorMin);
        }
        else if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
        {
            _graph.SetRange('c', dColorMin*0.95, dColorMax*1.05);
            _pInfo.dColorRanges[0] = dColorMin*0.95;
            _pInfo.dColorRanges[1] = dColorMax*1.05;
        }
        else
        {
            _graph.SetRange('c', dColorMin-0.05*(dColorMax-dColorMin), dColorMax+0.05*(dColorMax-dColorMin));
            _pInfo.dColorRanges[0] = dColorMin-0.05*(dColorMax-dColorMin);
            _pInfo.dColorRanges[1] = dColorMax+0.05*(dColorMax-dColorMin);
        }
    }
    else
    {
        if ((_pInfo.b2DVect || _pInfo.b3DVect) && (_pData.getFlow() || _pData.getPipe()))
        {
            if (_pData.getcLogscale())
            {
                _graph.SetRange('c', 1e-3, 1.05);
                _pInfo.dColorRanges[0] = 1e-3;
                _pInfo.dColorRanges[1] = 1.05;
            }
            else
            {
                _graph.SetRange('c', -1.05, 1.05);
                _pInfo.dColorRanges[0] = -1.05;
                _pInfo.dColorRanges[1] = 1.05;
            }
        }
        else if (_pInfo.b2DVect || _pInfo.b3DVect)
        {
            if (_pData.getcLogscale())
            {
                _graph.SetRange('c', 1e-3, 1.05);
                _pInfo.dColorRanges[0] = 1e-3;
                _pInfo.dColorRanges[1] = 1.05;
            }
            else
            {
                _graph.SetRange('c', -0.05, 1.05);
                _pInfo.dColorRanges[0] = -0.05;
                _pInfo.dColorRanges[1] = 1.05;
            }
        }
        else
        {
            if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && ((_pData.getMin() <= 0.0 && _pData.getMax()) || _pData.getAxisScale(3) <= 0.0))
            {
                if (_mDataPlots)
                {
                    for (int i = 0; i < nDataPlots; i++)
                        delete[] _mDataPlots[i];
                    delete[] _mDataPlots;
                    _mDataPlots = 0;
                    delete[] nDataDim;
                    nDataDim = 0;
                }
                throw WRONG_PLOT_INTERVAL_FOR_LOGSCALE;
            }
            else if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && _pData.getMin() <= 0.0)
            {
                _graph.SetRange('c', _pData.getMax()*1e-3/_pData.getAxisScale(3), (_pData.getMax()+0.05*(_pData.getMax()-_pData.getMin()))/_pData.getAxisScale(3));
                _pInfo.dColorRanges[0] = _pData.getMax()*1e-3/_pData.getAxisScale(3);
                _pInfo.dColorRanges[1] = (_pData.getMax()+0.05*(_pData.getMax()-_pData.getMin()))/_pData.getAxisScale(3);
            }
            else if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
            {
                _graph.SetRange('c', _pData.getMin()*0.95/_pData.getAxisScale(3), _pData.getMax()*1.05/_pData.getAxisScale(3));
                _pInfo.dColorRanges[0] = _pData.getMin()*0.95/_pData.getAxisScale(3);
                _pInfo.dColorRanges[1] = _pData.getMax()*1.05/_pData.getAxisScale(3);
            }
            else
            {
                _graph.SetRange('c', (_pData.getMin()-0.05*(_pData.getMax()-_pData.getMin()))/_pData.getAxisScale(3), (_pData.getMax()+0.05*(_pData.getMax()-_pData.getMin()))/_pData.getAxisScale(3));
                _pInfo.dColorRanges[0] = (_pData.getMin()-0.05*(_pData.getMax()-_pData.getMin()))/_pData.getAxisScale(3);
                _pInfo.dColorRanges[1] = (_pData.getMax()+0.05*(_pData.getMax()-_pData.getMin()))/_pData.getAxisScale(3);
            }
        }
    }
    // --> Andere Parameter setzen (die i. A. von den bestimmten Ranges abghaengen) <--
    // --> Gitter-, Koordinaten- und Achsenbeschriftungen <--
    parser_CoordSettings(_graph, _mAxisVals, _pData, _pInfo);

    if (_pData.getAxisScale() != 1.0 || _pData.getAxisScale(1) != 1.0 || _pData.getAxisScale(2) != 1.0 || _pData.getAxisScale(3) != 1.0)
    {
        if (_pData.getInvertion())
            _graph.SetRange('x', _pInfo.dRanges[0][1], _pInfo.dRanges[0][0]);
        else
            _graph.SetRange('x', _pInfo.dRanges[0][0], _pInfo.dRanges[0][1]);
        if (_pData.getInvertion(1))
            _graph.SetRange('y', _pInfo.dRanges[1][1], _pInfo.dRanges[1][0]);
        else
            _graph.SetRange('y', _pInfo.dRanges[1][0], _pInfo.dRanges[1][1]);
        if (_pData.getInvertion(2))
            _graph.SetRange('z', _pInfo.dRanges[2][1], _pInfo.dRanges[2][0]);
        else
            _graph.SetRange('z', _pInfo.dRanges[2][0], _pInfo.dRanges[2][1]);
        if (_pData.getBars() && _mDataPlots && !_pInfo.b2D)
        {
            int nMinbars = -1;
            for (int k = 0; k < nDataPlots; k++)
            {
                if (nMinbars == -1 || nMinbars > _mDataPlots[k][0].nx)
                    nMinbars = _mDataPlots[k][0].nx;
            }
            if (nMinbars < 2)
                nMinbars = 2;
            if (_pData.getInvertion())
                _graph.SetRange('x',_pInfo.dRanges[0][1]-fabs(_pInfo.dRanges[0][0]-_pInfo.dRanges[0][1])/(double)(2*(nMinbars-1)), _pInfo.dRanges[0][0]+fabs(_pInfo.dRanges[0][0]-_pInfo.dRanges[0][1])/(double)(2*(nMinbars-1)));
            else
                _graph.SetRange('x',_pInfo.dRanges[0][0]-fabs(_pInfo.dRanges[0][0]-_pInfo.dRanges[0][1])/(double)(2*(nMinbars-1)), _pInfo.dRanges[0][1]+fabs(_pInfo.dRanges[0][0]-_pInfo.dRanges[0][1])/(double)(2*(nMinbars-1)));
            if (_pInfo.sCommand == "plot3d")
            {
                if (_pData.getInvertion(1))
                    _graph.SetRange('y',_pInfo.dRanges[1][1]-fabs(_pInfo.dRanges[1][0]-_pInfo.dRanges[1][1])/(double)(2*(nMinbars-1)), _pInfo.dRanges[1][0]+fabs(_pInfo.dRanges[1][0]-_pInfo.dRanges[1][1])/(double)(2*(nMinbars-1)));
                else
                    _graph.SetRange('y',_pInfo.dRanges[1][0]-fabs(_pInfo.dRanges[1][0]-_pInfo.dRanges[1][1])/(double)(2*(nMinbars-1)), _pInfo.dRanges[1][1]+fabs(_pInfo.dRanges[1][0]-_pInfo.dRanges[1][1])/(double)(2*(nMinbars-1)));
            }
        }

        if (!isnan(_pData.getColorRange()) && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
        {
            if (_pData.getcLogscale() && _pData.getColorRange() <= 0.0)
            {
                _graph.SetRange('c', _pData.getColorRange(1)*1e-3, _pData.getColorRange(1));
                _pInfo.dColorRanges[0] = _pData.getColorRange(1)*1e-3;
                _pInfo.dColorRanges[1] = _pData.getColorRange(1);
            }
            else
            {
                _graph.SetRange('c', _pData.getColorRange(), _pData.getColorRange(1));
                _pInfo.dColorRanges[0] = _pData.getColorRange();
                _pInfo.dColorRanges[1] = _pData.getColorRange(1);
            }
        }
        else if (_mDataPlots)
        {
            double dColorMin = dDataRanges[2][0];
            double dColorMax = dDataRanges[2][1];
            if (_pData.getMax() > dColorMax && sFunc != "<<empty>>" && _pInfo.sCommand == "plot3d")
                dColorMax = _pData.getMax(2);
            if (_pData.getMin() < dColorMin && sFunc != "<<empty>>" && _pInfo.sCommand == "plot3d")
                dColorMin = _pData.getMin(2);

            if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && dColorMin <= 0.0 && dColorMax <= 0.0)
            {
                parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
                throw WRONG_PLOT_INTERVAL_FOR_LOGSCALE;
            }
            else if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && dColorMin <= 0.0)
            {
                dColorMin = dColorMax * 1e-3;
                _graph.SetRange('c', dColorMin, dColorMax+0.05*(dColorMax-dColorMin));
                _pInfo.dColorRanges[0] = dColorMin;
                _pInfo.dColorRanges[1] = dColorMax+0.05*(dColorMax-dColorMin);
            }
            else if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
            {
                _graph.SetRange('c', dColorMin*0.95, dColorMax*1.05);
                _pInfo.dColorRanges[0] = dColorMin*0.95;
                _pInfo.dColorRanges[1] = dColorMax*1.05;
            }
            else
            {
                _graph.SetRange('c', dColorMin-0.05*(dColorMax-dColorMin), dColorMax+0.05*(dColorMax-dColorMin));
                _pInfo.dColorRanges[0] = dColorMin-0.05*(dColorMax-dColorMin);
                _pInfo.dColorRanges[1] = dColorMax+0.05*(dColorMax-dColorMin);
            }
        }
        else
        {
            if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && _pData.getMin() <= 0.0 && _pData.getMax())
            {
                parser_plot_clearData(_mDataPlots, nDataDim, nDataPlots);
                throw WRONG_PLOT_INTERVAL_FOR_LOGSCALE;
            }
            else if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect) && _pData.getMin() <= 0.0)
            {
                _graph.SetRange('c', _pData.getMax()*1e-3, (_pData.getMax()+0.05*(_pData.getMax()-_pData.getMin())));
                _pInfo.dColorRanges[0] = _pData.getMax()*1e-3;
                _pInfo.dColorRanges[1] = (_pData.getMax()+0.05*(_pData.getMax()-_pData.getMin()));
            }
            else if (_pData.getcLogscale() && (_pInfo.b2D || _pInfo.sCommand == "plot3d" || _pInfo.b3D || _pInfo.b3DVect))
            {
                _graph.SetRange('c', _pData.getMin()*0.95, _pData.getMax()*1.05);
                _pInfo.dColorRanges[0] = _pData.getMin()*0.95;
                _pInfo.dColorRanges[1] = _pData.getMax()*1.05;
            }
            else
            {
                _graph.SetRange('c', (_pData.getMin()-0.05*(_pData.getMax()-_pData.getMin())), (_pData.getMax()+0.05*(_pData.getMax()-_pData.getMin())));
                _pInfo.dColorRanges[0] = (_pData.getMin()-0.05*(_pData.getMax()-_pData.getMin()));
                _pInfo.dColorRanges[1] = (_pData.getMax()+0.05*(_pData.getMax()-_pData.getMin()));
            }
        }
    }
}

void parser_plot_applyColorbar(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo)
{
    if (_pData.getColorbar() && (_pInfo.sCommand.substr(0,4) == "grad" || _pInfo.sCommand.substr(0,4) == "dens") && !_pInfo.b3D && !_pData.getSchematic())
    {
        if (_pData.getAxisScale(3) != 1.0)
            _graph.SetRange('c', _pInfo.dColorRanges[0]/_pData.getAxisScale(3), _pInfo.dColorRanges[1]/_pData.getAxisScale(3));
        // --> In diesem Fall haetten wir gerne eine Colorbar fuer den Farbwert <--
        if (_pData.getBox())
        {
            if (!(_pData.getContProj() && _pData.getContFilled()) && _pInfo.sCommand.substr(0,4) != "dens")
            {
                _graph.Colorbar(_pData.getColorSchemeLight("I>").c_str());
            }
            else
            {
                _graph.Colorbar(_pData.getColorScheme("I>").c_str());
            }
        }
        else
        {
            if (!(_pData.getContProj() && _pData.getContFilled()) && _pInfo.sCommand.substr(0,4) != "dens")
            {
                _graph.Colorbar(_pData.getColorSchemeLight(">").c_str());
            }
            else
            {
                _graph.Colorbar(_pData.getColorScheme().c_str());
            }
        }
        if (_pData.getAxisScale(3) != 1.0)
            _graph.SetRange('c', _pInfo.dColorRanges[0], _pInfo.dColorRanges[1]);
    }
    else if (_pData.getColorbar() && !_pData.getSchematic()
        && (_pInfo.sCommand.substr(0,4) == "mesh"
            || _pInfo.sCommand.substr(0,4) == "surf"
            || _pInfo.sCommand.substr(0,4) == "cont"
            || _pInfo.sCommand.substr(0,4) == "dens"
            || _pInfo.sCommand.substr(0,4) == "grad"
            || (_pInfo.sCommand.substr(0,6) == "plot3d" && (_pData.getMarks() || _pData.getCrust()))
            )
        )
    {
        if (_pData.getAxisScale(3) != 1.0)
            _graph.SetRange('c', _pInfo.dColorRanges[0]/_pData.getAxisScale(3), _pInfo.dColorRanges[1]/_pData.getAxisScale(3));
        _graph.Colorbar(_pData.getColorScheme().c_str());
        if (_pData.getAxisScale(3) != 1.0)
            _graph.SetRange('c', _pInfo.dColorRanges[0], _pInfo.dColorRanges[1]);
    }
}

void parser_plot_applyLighting(mglGraph& _graph, PlotData& _pData, PlotInfo& _pInfo)
{
    if (_pInfo.sCommand.substr(0,4) == "mesh"
        || _pInfo.sCommand.substr(0,4) == "surf"
        || _pInfo.sCommand.substr(0,4) == "cont"
        || _pInfo.b3D
        || _pInfo.b3DVect
        || _pData.getPipe()
        || _pInfo.bDraw3D
        || _pInfo.bDraw
        || (_pInfo.sCommand == "plot3d" && _pData.getCrust()))
    {
        // --> Licht- und Transparenz-Effekte <--
        if (_pInfo.sCommand.substr(0,4) == "surf"
            || (_pInfo.sCommand.substr(0,4) == "dens" && _pInfo.sCommand.find("3d") != string::npos && !_pData.getContProj())
            || (_pInfo.sCommand.substr(0,4) == "cont" && _pInfo.sCommand.find("3d") && _pData.getContFilled() && !_pData.getContProj())
            || _pInfo.bDraw3D
            || _pInfo.bDraw)
        _graph.Alpha(_pData.getTransparency());
        if (_pData.getAlphaMask() && _pInfo.sCommand.substr(0,4) == "surf" && !_pData.getTransparency())
            _graph.Alpha(true);

        if (_pData.getLighting())
        {
            _graph.Light(true);
            if (!_pData.getPipe() && !_pInfo.bDraw)
            {
                if (_pData.getLighting() == 1)
                {
                    _graph.AddLight(0, mglPoint(0,0,1), 'w', 0.35);
                    _graph.AddLight(1, mglPoint(5,30,5), 'w', 0.15);
                    _graph.AddLight(2, mglPoint(5,-30,5), 'w', 0.06);
                }
                else
                {
                    _graph.AddLight(0, mglPoint(0,0,1), 'w', 0.25);
                    _graph.AddLight(1, mglPoint(5,30,5), 'w', 0.1);
                    _graph.AddLight(2, mglPoint(5,-30,5), 'w', 0.04);
                }
            }
            else
            {
                if (_pData.getLighting() == 1)
                {
                    _graph.AddLight(1, mglPoint(-5,5,1), 'w', 0.4);
                    _graph.AddLight(2, mglPoint(-3,3,9), 'w', 0.1);
                    _graph.AddLight(3, mglPoint(5,-5,1), 'w', 0.1);
                }
                else
                {
                    _graph.AddLight(1, mglPoint(-5,5,1), 'w', 0.2);
                    _graph.AddLight(2, mglPoint(-3,3,9), 'w', 0.05);
                    _graph.AddLight(3, mglPoint(5,-5,1), 'w', 0.05);
                }
                _graph.Light(0, false);
            }
        }
    }
}


// --> Erledigt die nervenaufreibende Logscale-Logik <--
void parser_setLogScale(mglGraph& _graph, PlotData& _pData, bool bzLogscale)
{
    // --> Logarithmische Skalierung; ein bisschen Fummelei <--
    if (!_pData.getCoords())
    {
        if ((_pData.getxLogscale() || _pData.getyLogscale() || _pData.getzLogscale()) || _pData.getcLogscale())
        {
            _graph.SetRanges(0.1,10.0, 0.1,10.0, 0.1,10.0);
        }
        if (_pData.getxLogscale() && !_pData.getyLogscale() && !_pData.getzLogscale() && !_pData.getcLogscale())
            _graph.SetFunc("lg(x)", "");
        else if (_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale() && !_pData.getcLogscale())
            _graph.SetFunc("lg(x)", "lg(y)");
        else if (_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale() && bzLogscale && !_pData.getcLogscale())
            _graph.SetFunc("lg(x)", "lg(y)", "lg(z)");
        else if (_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale() && !bzLogscale && !_pData.getcLogscale())
            _graph.SetFunc("lg(x)", "lg(y)");
        else if (!_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale() && !_pData.getcLogscale())
            _graph.SetFunc("", "lg(y)");
        else if (!_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale() && bzLogscale && !_pData.getcLogscale())
            _graph.SetFunc("", "lg(y)", "lg(z)");
        else if (!_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale() && bzLogscale && !_pData.getcLogscale())
            _graph.SetFunc("", "", "lg(z)");
        else if (_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale() && bzLogscale && !_pData.getcLogscale())
            _graph.SetFunc("lg(x)", "", "lg(z)");
        else if (!_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale() && !bzLogscale && !_pData.getcLogscale())
            _graph.SetFunc("", "lg(y)");
        else if (_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale() && !bzLogscale && !_pData.getcLogscale())
            _graph.SetFunc("lg(x)", "");///----------------------------------------------------------------------
        else if (_pData.getxLogscale() && !_pData.getyLogscale() && !_pData.getzLogscale() && _pData.getcLogscale())
            _graph.SetFunc("lg(x)", "", "", "lg(c)");
        else if (_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale() && _pData.getcLogscale())
            _graph.SetFunc("lg(x)", "lg(y)", "", "lg(c)");
        else if (_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale() && bzLogscale && _pData.getcLogscale())
            _graph.SetFunc("lg(x)", "lg(y)", "lg(z)", "lg(c)");
        else if (_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale() && !bzLogscale && _pData.getcLogscale())
            _graph.SetFunc("lg(x)", "lg(y)", "", "lg(c)");
        else if (!_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale() && _pData.getcLogscale())
            _graph.SetFunc("", "lg(y)", "", "lg(c)");
        else if (!_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale() && bzLogscale && _pData.getcLogscale())
            _graph.SetFunc("", "lg(y)", "lg(z)", "lg(c)");
        else if (!_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale() && bzLogscale && _pData.getcLogscale())
            _graph.SetFunc("", "", "lg(z)", "lg(c)");
        else if (_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale() && bzLogscale && _pData.getcLogscale())
            _graph.SetFunc("lg(x)", "", "lg(z)", "lg(c)");
        else if (!_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale() && !bzLogscale && _pData.getcLogscale())
            _graph.SetFunc("", "lg(y)", "", "lg(c)");
        else if (_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale() && !bzLogscale && _pData.getcLogscale())
            _graph.SetFunc("lg(x)", "", "", "lg(c)");
        else if (!_pData.getxLogscale() && !_pData.getyLogscale() && !_pData.getzLogscale() && _pData.getcLogscale())
            _graph.SetFunc("", "", "", "lg(c)");
    }
    return;
}

void parser_directionalLight(mglGraph& _graph, double dRanges[3][2], double dPhi, double dTheta, int nId, char cColor, double dBrightness)
{
    mglPoint _mPoint(0.0,0.0,0.0);
    mglPoint _mDirection((dRanges[0][1]+dRanges[0][0])/2.0,(dRanges[1][1]+dRanges[1][0])/2.0,(dRanges[2][1]+dRanges[2][0])/2.0);
    double dNorm = hypot((dRanges[0][1]-dRanges[0][0])/2.0, (dRanges[1][1]-dRanges[1][0])/2.0);
    //cerr << "dNorm=" << dNorm << endl;
    _mPoint.x = dNorm*cos(dPhi/180.0*M_PI+M_PI_4)*sin(dTheta/180.0*M_PI);
    _mPoint.y = dNorm*sin(dPhi/180.0*M_PI+M_PI_4)*sin(dTheta/180.0*M_PI);
    _mPoint.z = dRanges[2][1];//0.5*dNorm*cos(dTheta/180.0*M_PI);
    /*cerr << "x=" << _mPoint.x
         << "  y=" << _mPoint.y
         << "  z=" << _mPoint.z
         << "  dx=" << _mDirection.x
         << "  dy=" << _mDirection.y
         << "  dz=" << _mDirection.z
         << endl;*/

    _mPoint += _mDirection;

    if (_mPoint.x > dRanges[0][1])
        _mPoint.x = dRanges[0][1];
    if (_mPoint.x < dRanges[0][0])
        _mPoint.x = dRanges[0][0];
    if (_mPoint.y > dRanges[1][1])
        _mPoint.y = dRanges[1][1];
    if (_mPoint.y < dRanges[1][0])
        _mPoint.y = dRanges[1][0];

    _mDirection = _mDirection - _mPoint;

    /*cerr << "x1=" << dRanges[0][0]
         << "  x2=" << dRanges[0][1]
         << "  y1=" << dRanges[1][0]
         << "  y2=" << dRanges[1][1]
         << "  z1=" << dRanges[2][0]
         << "  z2=" << dRanges[2][1]
         << endl;*/

    /*cerr << "x=" << _mPoint.x
         << "  y=" << _mPoint.y
         << "  z=" << _mPoint.z
         << "  dx=" << _mDirection.x
         << "  dy=" << _mDirection.y
         << "  dz=" << _mDirection.z
         << endl;*/
    //_mPoint.z *= 2.0;
    _graph.AddLight(nId, _mPoint, _mDirection, cColor, dBrightness);
    return;
}

mglPoint parser_CalcCutBox(double dPhi, double dRanges[3][2], int nEdge, int nCoords, bool b3D)
{
    if (dPhi >= 0.0 && dPhi < 90.0)
    {
        if (!nEdge)
        {
            if (!nCoords)
                return mglPoint(0.5*(dRanges[0][1]-dRanges[0][0])+dRanges[0][0], dRanges[1][0]-0.1*fabs(dRanges[1][0]), dRanges[2][0]-0.1*fabs(dRanges[2][0]));
            else if (nCoords == 1)
            {
                if (b3D)
                    return mglPoint(0.0, 1.5*M_PI, dRanges[2][0]-0.1*fabs(dRanges[2][0]));
                else
                    return mglPoint(1.5*M_PI, dRanges[1][0]-0.1*fabs(dRanges[1][0]), 0.0);
            }
            else if (nCoords == 2)
            {
                if (b3D)
                    return mglPoint(0.0, 1.5*M_PI, 0.0);
                else
                    return mglPoint(1.5*M_PI, 0.0, 0.0);
            }
        }
        else
        {
            if (!nCoords)
                return mglPoint(dRanges[0][1]+0.1*fabs(dRanges[0][1]), (0.5*(dRanges[1][1]-dRanges[1][0])+dRanges[1][0]), dRanges[2][1]+0.1*fabs(dRanges[2][1]));
            else if (nCoords == 1)
            {
                if (b3D)
                    return mglPoint(1.1*dRanges[0][1], 2.0*M_PI, dRanges[2][1]+0.1*fabs(dRanges[2][1]));
                else
                    return mglPoint(2.0*M_PI, dRanges[1][1]+0.1*fabs(dRanges[1][1]), 1.1*dRanges[2][1]);
            }
            else if (nCoords == 2)
            {
                if (b3D)
                    return mglPoint(1.1*dRanges[0][1], 2.0*M_PI, M_PI);
                else
                    return mglPoint(2.0*M_PI, M_PI, 1.1*dRanges[2][1]);
            }
        }
    }
    else if (dPhi >= 90.0 && dPhi < 180.0)
    {
        if (!nEdge)
        {
            if (!nCoords)
                return mglPoint(0.5*(dRanges[0][1]-dRanges[0][0])+dRanges[0][0], 0.5*(dRanges[1][1]-dRanges[1][0])+dRanges[1][0], dRanges[2][0]-0.1*fabs(dRanges[2][0]));
            else if (nCoords == 1)
            {
                if (b3D)
                    return mglPoint(0.0, 0.0, dRanges[2][0]-0.1*fabs(dRanges[2][0]));
                else
                    return mglPoint(0.0, dRanges[1][0]-0.1*fabs(dRanges[1][0]), 0.0);
            }
            else if (nCoords == 2)
            {
                if (b3D)
                    return mglPoint(0.0, 0.0, 0.0);
                else
                    return mglPoint(0.0, 0.0, 0.0);
            }
        }
        else
        {
            if (!nCoords)
                return mglPoint(dRanges[0][1]+0.1*fabs(dRanges[0][1]), dRanges[1][1]+0.1*fabs(dRanges[1][1]), dRanges[2][1]+0.1*fabs(dRanges[2][1]));
            else if (nCoords == 1)
            {
                if (b3D)
                    return mglPoint(dRanges[0][1]+0.1*fabs(dRanges[0][1]), 0.5*M_PI, dRanges[2][1]+0.1*fabs(dRanges[2][1]));
                else
                    return mglPoint(0.5*M_PI, dRanges[1][1]+0.1*fabs(dRanges[1][1]), dRanges[2][1]*1.1);
            }
            else if (nCoords == 2)
            {
                if (b3D)
                    return mglPoint(1.1*dRanges[0][1], 0.5*M_PI, M_PI);
                else
                    return mglPoint(0.5*M_PI, M_PI, 1.1*dRanges[2][1]);
            }
        }
    }
    else if (dPhi >= 180.0 && dPhi < 270.0)
    {
        if (!nEdge)
        {
            if (!nCoords)
                return mglPoint(dRanges[0][0]-0.1*fabs(dRanges[0][0]), 0.5*(dRanges[1][1]-dRanges[1][0])+dRanges[1][0], dRanges[2][0]-0.1*fabs(dRanges[2][0]));
            else if (nCoords == 1)
            {
                if (b3D)
                    return mglPoint(0.0, 0.5*M_PI, dRanges[2][0]-0.1*fabs(dRanges[2][0]));
                else
                    return mglPoint(0.5*M_PI, dRanges[1][0]-0.1*fabs(dRanges[1][0]), 0.0);
            }
            else if (nCoords == 2)
            {
                if (b3D)
                    return mglPoint(0.0, 0.5*M_PI, 0.0);
                else
                    return mglPoint(0.5*M_PI, 0.0, 0.0);
            }
        }
        else
        {
            if (!nCoords)
                return mglPoint((0.5*(dRanges[0][1]-dRanges[0][0])+dRanges[0][0]), dRanges[1][1]+0.1*fabs(dRanges[1][1]), dRanges[2][1]+0.1*fabs(dRanges[2][1]));
            else if (nCoords == 1)
            {
                if (b3D)
                    return mglPoint(1.1*dRanges[0][1], M_PI, dRanges[2][1]+0.1*fabs(dRanges[2][1]));
                else
                    return mglPoint(M_PI, dRanges[1][1]+0.1*fabs(dRanges[1][1]), 1.1*dRanges[2][1]);
            }
            else if (nCoords == 2)
            {
                if (b3D)
                    return mglPoint(1.1*dRanges[0][1], M_PI, M_PI);
                else
                    return mglPoint(M_PI, M_PI, 1.1*dRanges[2][1]);
            }
        }
    }
    else
    {
        if (!nEdge)
        {
            if (!nCoords)
                return mglPoint(dRanges[0][0]-0.1*fabs(dRanges[0][0]), dRanges[1][0]-0.1*fabs(dRanges[1][0]), dRanges[2][0]-0.1*fabs(dRanges[2][0]));
            else if (nCoords == 1)
            {
                if (b3D)
                    return mglPoint(0.0, M_PI, dRanges[2][0]-0.1*fabs(dRanges[2][0]));
                else
                    return mglPoint(M_PI, dRanges[1][0]-0.1*fabs(dRanges[1][0]), 0.0);
            }
            else if (nCoords == 2)
            {
                if (b3D)
                    return mglPoint(0.0, M_PI, 0.0);
                else
                    return mglPoint(M_PI, 0.0, 0.0);
            }
        }
        else
        {
            if (!nCoords)
                return mglPoint(0.5*(dRanges[0][1]-dRanges[0][0])+dRanges[0][0], 0.5*(dRanges[1][1]-dRanges[1][0])+dRanges[1][0], dRanges[2][1]+0.1*fabs(dRanges[2][1]));
            else if (nCoords == 1)
            {
                if (b3D)
                    return mglPoint(1.1*dRanges[0][1], 1.5*M_PI, dRanges[2][1]+0.1*fabs(dRanges[2][1]));
                else
                    return mglPoint(1.5*M_PI, dRanges[1][1]+0.1*fabs(dRanges[1][1]), 1.1*dRanges[2][1]);
            }
            else if (nCoords == 2)
            {
                if (b3D)
                    return mglPoint(1.1*dRanges[0][1], 1.5*M_PI, M_PI);
                else
                    return mglPoint(1.5*M_PI, M_PI, 1.1*dRanges[2][1]);
            }
        }
    }
    return mglPoint(0);
}

double parser_getProjBackground(double dPhi, double dRanges[3][2], int nEdge)
{
    if (dPhi >= 0.0 && dPhi < 90.0)
    {
        if (!nEdge)
        {
            return dRanges[0][0];
        }
        else
        {
            return dRanges[1][1];
        }
    }
    else if (dPhi >= 90.0 && dPhi < 180.0)
    {
        if (!nEdge)
        {
            return dRanges[0][0];
        }
        else
        {
            return dRanges[1][0];
        }
    }
    else if (dPhi >= 180.0 && dPhi < 270.0)
    {
        if (!nEdge)
        {
            return dRanges[0][1];
        }
        else
        {
            return dRanges[1][0];
        }
    }
    else
    {
        if (!nEdge)
        {
            return dRanges[0][1];
        }
        else
        {
            return dRanges[1][1];
        }
    }
    return 0.0;
}

string parser_getLegendStyle(const string& sLegend, const PlotData& _pData)
{
    if (_pData.getLegendStyle())
    {
        if (_pData.getLegendStyle() == 1)
        {
            return sLegend.substr(0,1) + "-";
        }
        else if (_pData.getLegendStyle() == 2)
        {
            return "k" + sLegend.substr(1);
        }
        else
            return sLegend;
    }
    else
        return sLegend;
}

mglData parser_fmod(const mglData& _mData, double dDenominator)
{
    if (!getNN(_mData))
        return _mData;
    mglData _mReturn(_mData.nx, _mData.ny, _mData.nz);

    for (long int i = 0; i < getNN(_mData); i++)
    {
        _mReturn.a[i] = fmod(_mData.a[i], dDenominator);
        // Spezialfall fuer krummlinige Koordinaten: wandle negative Winkel in positive um
        if (_mReturn.a[i] < 0)
            _mReturn.a[i] += dDenominator;
    }
    return _mReturn;
}

void parser_CoordSettings(mglGraph& _graph, mglData _mAxisVals[3], const PlotData& _pData, PlotInfo& _pInfo)
{
    if (_pData.getAxis())
    {
        for (int i = 0; i < 4; i++)
        {
            if (_pData.getTickTemplate(i).length())
            {
                if (i < 3)
                    _graph.SetTickTempl('x'+i, _pData.getTickTemplate(i).c_str());
                else
                    _graph.SetTickTempl('c', _pData.getTickTemplate(i).c_str());
            }
            else if (max(_pInfo.dRanges[i][0], _pInfo.dRanges[i][1])/_pData.getAxisScale(i) < 1e-2
                && max(_pInfo.dRanges[i][0], _pInfo.dRanges[i][1])/_pData.getAxisScale(i) >= 1e-3)
            {
                if (i < 3)
                    _graph.SetTickTempl('x'+i, "%g");
                else
                    _graph.SetTickTempl('c', "%g");
            }
            if (_pData.getCustomTick(i).length())
            {
                int nCount = 1;
                mglData _mAxisRange;
                if (i < 3)
                {
                    for (unsigned int n = 0; n < _pData.getCustomTick(i).length(); n++)
                    {
                        if (_pData.getCustomTick(i)[n] == '\n')
                            nCount++;
                    }
                    //cerr << nCount << endl;
                    _mAxisRange.Create(nCount);

                    // Ranges fuer customn ticks anpassen
                    if (_pData.getCoords())
                    {
                        if (!(_pInfo.b2D || _pInfo.b3D || _pInfo.sCommand == "plot3d" || _pInfo.b3DVect || _pInfo.b2DVect))
                        {
                            _pInfo.dRanges[0][0] = 0.0;
                            _pInfo.dRanges[1][0] = 0.0;
                            _pInfo.dRanges[0][1] = 2.0;
                        }
                        else if (_pInfo.sCommand.find("3d") != string::npos)
                        {
                            _pInfo.dRanges[0][0] = 0.0;
                            _pInfo.dRanges[1][0] = 0.0;
                            _pInfo.dRanges[1][1] = 2.0;
                            if (_pData.getCoords() == 2)
                            {
                                _pInfo.dRanges[2][0] = 0.0;
                                _pInfo.dRanges[2][1] = 1.0;
                            }
                        }
                        else if (_pInfo.b2DVect)
                        {
                            _pInfo.dRanges[0][0] = 0.0;
                            _pInfo.dRanges[1][0] = 0.0;
                            _pInfo.dRanges[1][1] = 2.0;
                        }
                        else
                        {
                            _pInfo.dRanges[0][0] = 0.0;
                            _pInfo.dRanges[0][1] = 2.0;
                            _pInfo.dRanges[2][0] = 0.0;
                            if (_pData.getCoords() == 2)
                            {
                                _pInfo.dRanges[1][0] = 0.0;
                                _pInfo.dRanges[1][1] = 1.0;
                            }
                        }
                    }

                    if (nCount == 1)
                    {
                        _mAxisRange.a[0] = _pInfo.dRanges[i][0] + (_pInfo.dRanges[i][1]-_pInfo.dRanges[i][0])/2.0;
                    }
                    else
                    {
                        for (int n = 0; n < nCount; n++)
                        {
                            _mAxisRange.a[n] = _pInfo.dRanges[i][0] + (double)n*(_pInfo.dRanges[i][1]-_pInfo.dRanges[i][0])/(double)(nCount-1);
                            //cerr << _pInfo.dRanges[i][0] + (double)n*(_pInfo.dRanges[i][1]-_pInfo.dRanges[i][0])/(double)(nCount-1) << endl;
                        }
                    }
                    _graph.SetTicksVal('x'+i, _mAxisRange, fromSystemCodePage(_pData.getCustomTick(i)).c_str());
                }
                else
                {
                    for (unsigned int n = 0; n < _pData.getCustomTick(i).length(); n++)
                    {
                        if (_pData.getCustomTick(i)[n] == '\n')
                            nCount++;
                    }
                    _mAxisRange.Create(nCount);
                    if (nCount == 1)
                        _mAxisRange.a[0] = _pInfo.dColorRanges[0] + (_pInfo.dColorRanges[1]-_pInfo.dColorRanges[0])/2.0;
                    else
                    {
                        for (int n = 0; n < nCount; n++)
                        {
                            _mAxisRange.a[n] = _pInfo.dColorRanges[0] + (double)n*(_pInfo.dColorRanges[1]-_pInfo.dColorRanges[0])/(double)(nCount-1);
                        }
                    }
                    _graph.SetTicksVal('c', _mAxisRange, fromSystemCodePage(_pData.getCustomTick(i)).c_str());
                }
            }
        }
        if (_pData.getBox() || _pData.getCoords())
        {
            if (!(_pInfo.b2D || _pInfo.b3D || _pInfo.sCommand == "plot3d" || _pInfo.b3DVect || _pInfo.b2DVect))
            {
                if (_pData.getCoords())
                {
                    _graph.SetOrigin(0.0, _pInfo.dRanges[1][1]/_pData.getAxisScale());
                    _graph.SetFunc(parser_CoordFunc("y*cos(pi*x*$PS$)", _pData.getAxisScale()).c_str(), parser_CoordFunc("y*sin(pi*x*$PS$)", _pData.getAxisScale()).c_str());
                    _graph.SetRange('x', 0.0, 1.9999999/_pData.getAxisScale());
                    _graph.SetRange('y', 0.0, _pInfo.dRanges[1][1]/_pData.getAxisScale(1));
                    if (!_pData.getSchematic())
                        _graph.Axis("xy"); //U
                    else
                    {
                        _graph.SetTickLen(1e-20);
                        _graph.Axis("xy_");
                    }
                    _graph.Box();
                    if (_pData.getGrid() == 1)
                        _graph.Grid("xyzt", _pData.getGridStyle().c_str());
                    else if (_pData.getGrid() == 2)
                    {
                        _graph.Grid("xyzt!", _pData.getGridStyle().c_str());
                        _graph.Grid("xyzt", _pData.getFineGridStyle().c_str());
                    }
                    _graph.SetFunc("y*cos(x)","y*sin(x)");
                    _mAxisVals[0] = parser_fmod(_mAxisVals[0], 2.0*M_PI);
                    _graph.SetRange('x', 0.0, 2.0*M_PI);
                    _graph.SetRange('y', 0.0, _pInfo.dRanges[1][1]);
                    if (!_pData.getSchematic()
                        || matchParams(_pInfo.sPlotParams, "xlabel", '=')
                        || matchParams(_pInfo.sPlotParams, "ylabel", '=')
                        || matchParams(_pInfo.sPlotParams, "zlabel", '='))
                    {
                        _graph.Label('x', fromSystemCodePage(_pData.getxLabel()).c_str(), 0.25);
                        _graph.Label('y', fromSystemCodePage(_pData.getzLabel()).c_str(), 0.0);
                    }
                    if (_pData.getBars() || _pData.getArea())
                        _graph.SetOrigin(0.0,0.0);
                    _pInfo.dRanges[0][0] = 0.0;
                    _pInfo.dRanges[0][1] = 2*M_PI;
                    _pInfo.dRanges[1][0] = 0.0;
                    //dRanges[1][1] =
                    //dRanges[2][0] =
                    //dRanges[2][1] =
                }
                else
                {
                    if (!_pData.getSchematic())
                    {
                        if (!isnan(_pData.getAddAxis(0).dMin) || !isnan(_pData.getAddAxis(1).dMin))
                        {
                            Axis _axis;
                            _graph.SetOrigin(_pInfo.dRanges[0][1], _pInfo.dRanges[1][1]);
                            for (int i = 0; i < 2; i++)
                            {
                                _axis = _pData.getAddAxis(i);
                                if (_axis.sLabel.length())
                                {
                                    _graph.SetRange('x'+i, _axis.dMin, _axis.dMax);
                                    if (!i)
                                        _graph.Axis("x", _axis.sStyle.c_str());
                                    else
                                        _graph.Axis("y", _axis.sStyle.c_str());
                                    _graph.Label('x'+i, fromSystemCodePage("#"+_axis.sStyle+"{"+_axis.sLabel+"}").c_str(), 0);
                                }
                            }
                            _graph.SetRanges(_pInfo.dRanges[0][0], _pInfo.dRanges[0][1], _pInfo.dRanges[1][0], _pInfo.dRanges[1][1], _pInfo.dRanges[2][0], _pInfo.dRanges[2][1]);
                            _graph.SetOrigin(_pInfo.dRanges[0][0], _pInfo.dRanges[1][0]);
                        }
                        _graph.Axis("xy");
                    }
                }
            }
            else if (_pInfo.sCommand.find("3d") != string::npos)
            {
                if (_pData.getCoords() == 1)
                {
                    _graph.SetOrigin(_pInfo.dRanges[0][1]/_pData.getAxisScale(), 0.0, _pInfo.dRanges[2][0]/_pData.getAxisScale(2));
                    _graph.SetFunc(parser_CoordFunc("x*cos(pi*y*$PS$)", _pData.getAxisScale(1)).c_str(), parser_CoordFunc("x*sin(pi*y*$PS$)", _pData.getAxisScale(1)).c_str(), "z");
                    _graph.SetRange('x', 0.0, _pInfo.dRanges[0][1]/_pData.getAxisScale());
                    _graph.SetRange('y', 0.0, 1.9999999/_pData.getAxisScale(1));
                    _graph.SetRange('z', _pInfo.dRanges[2][0]/_pData.getAxisScale(2), _pInfo.dRanges[2][1]/_pData.getAxisScale(2));
                    if (!_pData.getSchematic())
                        _graph.Axis();
                    else
                    {
                        _graph.SetTickLen(1e-20);
                        _graph.Axis("_");
                    }
                    _graph.Box();
                    if (_pInfo.b3DVect && _pData.getGrid())
                        _graph.Grid("xyzt", _pData.getFineGridStyle().c_str());
                    else if (_pData.getGrid() == 1)
                        _graph.Grid("xyzt", _pData.getGridStyle().c_str());
                    else if (_pData.getGrid() == 2)
                    {
                        _graph.Grid("xyzt!", _pData.getGridStyle().c_str());
                        _graph.Grid("xyzt", _pData.getFineGridStyle().c_str());
                    }
                    if (_mAxisVals[1].nx)
                    {
                        for (int y = 0; y < _pInfo.nSamples; y++)
                        {
                            _mAxisVals[1].a[y] = fmod(_mAxisVals[1].a[y],2.0*M_PI);
                        }
                    }
                    _graph.SetFunc("x*cos(y)","x*sin(y)","z");
                    _graph.SetRange('x', 0.0, _pInfo.dRanges[0][1]);
                    _graph.SetRange('y', 0.0, 2.0*M_PI);
                    _graph.SetRange('z', _pInfo.dRanges[2][0], _pInfo.dRanges[2][1]);
                    if (!_pData.getSchematic()
                        || matchParams(_pInfo.sPlotParams, "xlabel", '=')
                        || matchParams(_pInfo.sPlotParams, "ylabel", '=')
                        || matchParams(_pInfo.sPlotParams, "zlabel", '='))
                    {
                        _graph.Label('x', fromSystemCodePage(_pData.getzLabel()).c_str(), -0.5);
                        _graph.Label('y', fromSystemCodePage(_pData.getxLabel()).c_str(), (_pData.getRotateAngle(1)-225.0)/180.0); //-0.65
                        _graph.Label('z', fromSystemCodePage(_pData.getyLabel()).c_str(), 0.0);
                    }
                    if (_pData.getBars() || _pData.getArea())
                        _graph.SetOrigin(0.0,0.0,_pInfo.dRanges[2][0]);
                    _pInfo.dRanges[0][0] = 0.0;
                    //dRanges[0][1] = 2.0;
                    _pInfo.dRanges[1][0] = 0.0;
                    _pInfo.dRanges[1][1] = 2.0*M_PI;
                    //dRanges[2][0] =
                    //dRanges[2][1] =

                }
                else if (_pData.getCoords() == 2)
                {
                    _graph.SetOrigin(_pInfo.dRanges[0][1]/_pData.getAxisScale(), 0.0, 0.5/_pData.getAxisScale(2));
                    _graph.SetFunc(parser_CoordFunc("x*cos(pi*y*$PS$)*sin(pi*z*$TS$)", _pData.getAxisScale(1), _pData.getAxisScale(2)).c_str(), parser_CoordFunc("x*sin(pi*y*$PS$)*sin(pi*z*$TS$)", _pData.getAxisScale(1), _pData.getAxisScale(2)).c_str(), parser_CoordFunc("x*cos(pi*z*$PS$)", 1.0, _pData.getAxisScale(2)).c_str());
                    _graph.SetRange('x', 0.0, _pInfo.dRanges[0][1]/_pData.getAxisScale());
                    _graph.SetRange('y', 0.0, 1.9999999/_pData.getAxisScale(1));
                    _graph.SetRange('z', 0.0, 0.9999999/_pData.getAxisScale(2));
                    if (!_pData.getSchematic())
                        _graph.Axis();
                    else
                    {
                        _graph.SetTickLen(1e-20);
                        _graph.Axis("_");
                    }
                    _graph.Box();
                    if (_pInfo.b3DVect && _pData.getGrid())
                        _graph.Grid("xyzt", _pData.getFineGridStyle().c_str());
                    else if (_pData.getGrid() == 1)
                        _graph.Grid("xyzt", _pData.getGridStyle().c_str());
                    else if (_pData.getGrid() == 2)
                    {
                        _graph.Grid("xyzt!", _pData.getGridStyle().c_str());
                        _graph.Grid("xyzt", _pData.getFineGridStyle().c_str());
                    }
                    _graph.SetFunc("x*cos(y)*sin(z)","x*sin(y)*sin(z)","x*cos(z)");
                    _graph.SetOrigin(_pInfo.dRanges[0][1], 0.0, 0.5*M_PI);
                    if (_mAxisVals[1].nx && _mAxisVals[2].nx)
                    {
                        _mAxisVals[1] = parser_fmod(_mAxisVals[1], 2.0*M_PI);
                        _mAxisVals[2] = parser_fmod(_mAxisVals[2], 1.0*M_PI);
                    }
                    _graph.SetRange('x', 0.0, _pInfo.dRanges[0][1]);
                    _graph.SetRange('y', 0.0, 2.0*M_PI);
                    _graph.SetRange('z', 0.0, 1.0*M_PI);
                    if (!_pData.getSchematic()
                        || matchParams(_pInfo.sPlotParams, "xlabel", '=')
                        || matchParams(_pInfo.sPlotParams, "ylabel", '=')
                        || matchParams(_pInfo.sPlotParams, "zlabel", '='))
                    {
                        _graph.Label('x', fromSystemCodePage(_pData.getzLabel()).c_str(), -0.4);
                        _graph.Label('y', fromSystemCodePage(_pData.getxLabel()).c_str(), (_pData.getRotateAngle(1)-225.0)/180.0); //-0.7
                        _graph.Label('z', fromSystemCodePage(_pData.getyLabel()).c_str(), -0.9); // -0.4
                    }
                    if (_pData.getBars() || _pData.getArea())
                        _graph.SetOrigin(0.0,0.0,0.5*M_PI);
                    _pInfo.dRanges[0][0] = 0.0;
                    //dRanges[0][1] = 2.0;
                    _pInfo.dRanges[1][0] = 0.0;
                    _pInfo.dRanges[1][1] = 2.0*M_PI;
                    _pInfo.dRanges[2][0] = 0.0;
                    _pInfo.dRanges[2][1] = 1.0*M_PI;
                }
                else
                {
                    if (!_pData.getSchematic())
                        _graph.Axis("xyz");
                }
            }
            else if (_pInfo.b2DVect)
            {
                if (_pData.getCoords())
                {
                    _graph.SetOrigin(_pInfo.dRanges[0][1]/_pData.getAxisScale(), 0.0);
                    _graph.SetFunc(parser_CoordFunc("x*cos(pi*y*$PS$)", _pData.getAxisScale(1)).c_str(), parser_CoordFunc("x*sin(pi*y*$PS$)", _pData.getAxisScale(1)).c_str());
                    _graph.SetRange('x', 0.0, _pInfo.dRanges[0][1]/_pData.getAxisScale());
                    _graph.SetRange('y', 0.0, 1.9999999/_pData.getAxisScale(1));
                    if (!_pData.getSchematic())
                        _graph.Axis("xy");
                    else
                    {
                        _graph.SetTickLen(1e-20);
                        _graph.Axis("xy_");
                    }
                    _graph.Box();
                    if (_pData.getGrid() == 1)
                    {
                        _graph.Grid("xyzt", _pData.getFineGridStyle().c_str());
                    }
                    _graph.SetFunc("x*cos(y)","x*sin(y)");
                    _mAxisVals[1] = parser_fmod(_mAxisVals[1], 2.0*M_PI);
                    _graph.SetRange('x', 0.0, _pInfo.dRanges[0][1]);
                    _graph.SetRange('y', 0.0, 2.0*M_PI);
                    if (!_pData.getSchematic()
                        || matchParams(_pInfo.sPlotParams, "xlabel", '=')
                        || matchParams(_pInfo.sPlotParams, "ylabel", '=')
                        || matchParams(_pInfo.sPlotParams, "zlabel", '='))
                    {
                        _graph.Label('x', fromSystemCodePage(_pData.getzLabel()).c_str(), 0.0);
                        _graph.Label('y', fromSystemCodePage(_pData.getxLabel()).c_str(), 0.25);
                    }
                    _pInfo.dRanges[0][0] = 0.0;
                    //dRanges[0][1] = 2.0;
                    _pInfo.dRanges[1][0] = 0.0;
                    _pInfo.dRanges[1][1] = 2.0*M_PI;
                    //dRanges[2][0] =
                    //dRanges[2][1] =
                }
                else
                {
                    if (!_pData.getSchematic())
                        _graph.Axis("xy");
                }
            }
            else
            {
                if (_pData.getCoords() == 1)
                {
                    _graph.SetOrigin(0.0, _pInfo.dRanges[1][0]/_pData.getAxisScale(1), _pInfo.dRanges[2][1]/_pData.getAxisScale(2));
                    _graph.SetFunc(parser_CoordFunc("z*cos(pi*x*$PS$)", _pData.getAxisScale()).c_str(), parser_CoordFunc("z*sin(pi*x*$PS$)", _pData.getAxisScale()).c_str(), "y");
                    _graph.SetRange('x', 0.0, 1.9999999/_pData.getAxisScale());
                    _graph.SetRange('y', _pInfo.dRanges[1][0]/_pData.getAxisScale(1), _pInfo.dRanges[1][1]/_pData.getAxisScale(1));
                    _graph.SetRange('z', 0.0, _pInfo.dRanges[2][1]/_pData.getAxisScale(2));
                    if (!_pData.getSchematic())
                        _graph.Axis(); //U
                    else
                    {
                        _graph.SetTickLen(1e-20);
                        _graph.Axis("_");
                    }
                    _graph.Box();
                    if (_pData.getGrid() == 1)
                        _graph.Grid("xyzt", _pData.getGridStyle().c_str());
                    else if (_pData.getGrid() == 2)
                    {
                        _graph.Grid("xyzt!", _pData.getGridStyle().c_str());
                        _graph.Grid("xyzt", _pData.getFineGridStyle().c_str());
                    }
                    _graph.SetFunc("z*cos(x)","z*sin(x)","y");
                    _mAxisVals[0] = parser_fmod(_mAxisVals[0], 2.0*M_PI);
                    _graph.SetRange('x', 0.0, 2.0*M_PI);
                    _graph.SetRange('y', _pInfo.dRanges[1][0], _pInfo.dRanges[1][1]);
                    _graph.SetRange('z', 0.0, _pInfo.dRanges[2][1]);
                    if (!_pData.getSchematic()
                        || matchParams(_pInfo.sPlotParams, "xlabel", '=')
                        || matchParams(_pInfo.sPlotParams, "ylabel", '=')
                        || matchParams(_pInfo.sPlotParams, "zlabel", '='))
                    {
                        _graph.Label('x', fromSystemCodePage(_pData.getxLabel()).c_str(), (_pData.getRotateAngle(1)-225.0)/180.0);//-0.65
                        _graph.Label('y', fromSystemCodePage(_pData.getyLabel()).c_str(), 0.0);
                        _graph.Label('z', fromSystemCodePage(_pData.getzLabel()).c_str(), -0.5);
                    }
                    _pInfo.dRanges[0][0] = 0.0;
                    _pInfo.dRanges[0][1] = 2.0*M_PI;
                    //dRanges[1][0] = 0.0;
                    //dRanges[1][1] =
                    _pInfo.dRanges[2][0] = 0.0;
                    //dRanges[2][1] =
                }
                else if(_pData.getCoords() == 2)
                {
                    _graph.SetOrigin(0.0, 0.5/_pData.getAxisScale(1), _pInfo.dRanges[2][1]/_pData.getAxisScale(2));
                    _graph.SetFunc(parser_CoordFunc("z*cos(pi*x*$PS$)*sin(pi*y*$TS$)", _pData.getAxisScale(), _pData.getAxisScale(1)).c_str(), parser_CoordFunc("z*sin(pi*x*$PS$)*sin(pi*y*$TS$)", _pData.getAxisScale(), _pData.getAxisScale(1)).c_str(), parser_CoordFunc("z*cos(pi*y*$TS$)", 1.0, _pData.getAxisScale(1)).c_str());
                    _graph.SetRange('x', 0.0, 1.9999999/_pData.getAxisScale());
                    _graph.SetRange('y', 0.0, 0.9999999/_pData.getAxisScale(1));
                    _graph.SetRange('z', 0.0, _pInfo.dRanges[2][1]/_pData.getAxisScale(2));
                    if (!_pData.getSchematic())
                        _graph.Axis(); //U
                    else
                    {
                        _graph.SetTickLen(1e-20);
                        _graph.Axis("_");
                    }
                    _graph.Box();
                    if (_pData.getGrid() == 1)
                        _graph.Grid("xyzt", _pData.getGridStyle().c_str());
                    else if (_pData.getGrid() == 2)
                    {
                        _graph.Grid("xyzt!", _pData.getGridStyle().c_str());
                        _graph.Grid("xyzt", _pData.getFineGridStyle().c_str());
                    }
                    _graph.SetFunc("z*cos(x)*sin(y)", "z*sin(x)*sin(y)", "z*cos(y)");
                    _graph.SetOrigin(0.0, 0.5*M_PI, _pInfo.dRanges[2][1]);
                    _mAxisVals[0] = parser_fmod(_mAxisVals[0], 2.0*M_PI);
                    _mAxisVals[1] = parser_fmod(_mAxisVals[1], 1.0*M_PI);
                    _graph.SetRange('x', 0.0, 2.0*M_PI);
                    _graph.SetRange('y', 0.0, 1.0*M_PI);
                    _graph.SetRange('z', 0.0, _pInfo.dRanges[2][1]);
                    if (!_pData.getSchematic()
                        || matchParams(_pInfo.sPlotParams, "xlabel", '=')
                        || matchParams(_pInfo.sPlotParams, "ylabel", '=')
                        || matchParams(_pInfo.sPlotParams, "zlabel", '='))
                    {
                        _graph.Label('x', fromSystemCodePage(_pData.getxLabel()).c_str(), (_pData.getRotateAngle(1)-225.0)/180.0);//-0.6
                        _graph.Label('y', fromSystemCodePage(_pData.getyLabel()).c_str(), -0.9);//-0.4
                        _graph.Label('z', fromSystemCodePage(_pData.getzLabel()).c_str(), -0.4);//-0.5
                    }
                    _pInfo.dRanges[0][0] = 0.0;
                    _pInfo.dRanges[0][1] = 2.0*M_PI;
                    _pInfo.dRanges[1][0] = 0.0;
                    _pInfo.dRanges[1][1] = 1.0*M_PI;
                    _pInfo.dRanges[2][0] = 0.0;
                    //dRanges[2][1] =
                }
                else
                {
                    if (!_pData.getSchematic())
                        _graph.Axis();
                }
            }
        }
        else if (isnan(_pData.getOrigin()) && isnan(_pData.getOrigin(1)) && isnan(_pData.getOrigin(2)))
        {
            if (_pInfo.dRanges[0][0] <= 0.0
                && _pInfo.dRanges[0][1] >= 0.0
                && _pInfo.dRanges[1][0] <= 0.0
                && _pInfo.dRanges[1][1] >= 0.0
                && _pInfo.dRanges[2][0] <= 0.0
                && _pInfo.dRanges[2][1] >= 0.0
                && _pInfo.nMaxPlotDim > 2 //(sCommand.find("3d") != string::npos || (_pInfo.b2D && sCommand != "dens"))
                )
            {
                _graph.SetOrigin(0.0, 0.0, 0.0);
                if (!_pData.getSchematic())
                    _graph.Axis("AKDTVISO");
                else
                {
                    _graph.SetTickLen(1e-20);
                    _graph.SetTicks('x', -5, 1);
                    _graph.SetTicks('y', -5, 1);
                    _graph.SetTicks('z', -5, 1);
                    _graph.Axis("AKDTVISO_");
                }
            }
            else if (_pInfo.dRanges[0][0] <= 0.0
                && _pInfo.dRanges[0][1] >= 0.0
                && _pInfo.dRanges[1][0] <= 0.0
                && _pInfo.dRanges[1][1] >= 0.0
                && _pInfo.nMaxPlotDim <= 2 //(sCommand.find("3d") == string::npos && !(_pInfo.b2D && sCommand != "dens"))
                )
            {
                _graph.SetOrigin(0.0, 0.0);
                if (!_pData.getSchematic())
                    _graph.Axis("AKDTVISO");
                else
                {
                    _graph.SetTickLen(1e-20);
                    _graph.SetTicks('x', -5, 1);
                    _graph.SetTicks('y', -5, 1);
                    _graph.Axis("AKDTVISO_");
                }
            }
            else if (_pInfo.nMaxPlotDim == 3)
            {
                if (_pInfo.dRanges[0][0] <= 0.0 && _pInfo.dRanges[0][1] >= 0.0)
                {
                    if (_pInfo.dRanges[1][0] <= 0.0 && _pInfo.dRanges[1][1] >= 0.0)
                        _graph.SetOrigin(0.0, 0.0, _pInfo.dRanges[2][0]);
                    else if (_pInfo.dRanges[2][0] <= 0.0 && _pInfo.dRanges[2][1] >= 0.0)
                        _graph.SetOrigin(0.0, _pInfo.dRanges[1][0], 0.0);
                    else
                        _graph.SetOrigin(0.0, _pInfo.dRanges[1][0], _pInfo.dRanges[2][0]);
                }
                else if (_pInfo.dRanges[1][0] <= 0.0 && _pInfo.dRanges[1][1] >= 0.0)
                {
                    if (_pInfo.dRanges[2][0] <= 0.0 && _pInfo.dRanges[2][1] >= 0.0)
                        _graph.SetOrigin(_pInfo.dRanges[0][0], 0.0, 0.0);
                    else
                        _graph.SetOrigin(_pInfo.dRanges[0][0], 0.0, _pInfo.dRanges[2][0]);
                }
                else if (_pInfo.dRanges[2][0] <= 0.0 && _pInfo.dRanges[2][1] >= 0.0)
                    _graph.SetOrigin(_pInfo.dRanges[0][0], _pInfo.dRanges[1][0], 0.0);
                else
                    _graph.SetOrigin(_pInfo.dRanges[0][0], _pInfo.dRanges[1][0], _pInfo.dRanges[2][0]);

                if (!_pData.getSchematic())
                    _graph.Axis("AKDTVISO");
                else
                {
                    _graph.SetTickLen(1e-20);
                    _graph.SetTicks('x', -5, 1);
                    _graph.SetTicks('y', -5, 1);
                    _graph.SetTicks('z', -5, 1);
                    _graph.Axis("AKDTVISO_");
                }

            }
            else if (_pInfo.nMaxPlotDim <= 2)
            {
                if (_pInfo.dRanges[0][0] <= 0.0 && _pInfo.dRanges[0][1] >= 0.0)
                {
                    _graph.SetOrigin(0.0, _pInfo.dRanges[1][0]);
                }
                else if (_pInfo.dRanges[1][0] <= 0.0 && _pInfo.dRanges[1][1] >= 0.0)
                {
                    _graph.SetOrigin(_pInfo.dRanges[0][0], 0.0);
                }
                else
                    _graph.SetOrigin(_pInfo.dRanges[0][0], _pInfo.dRanges[1][0]);

                if (!_pData.getSchematic())
                    _graph.Axis("AKDTVISO");
                else
                {
                    _graph.SetTickLen(1e-20);
                    _graph.SetTicks('x', -5, 1);
                    _graph.SetTicks('y', -5, 1);
                    _graph.Axis("AKDTVISO_");
                }
            }
        }
        else if (_pData.getOrigin() != 0.0 || _pData.getOrigin(1) != 0.0 || _pData.getOrigin(2) != 0.0)
        {
            if (_pInfo.dRanges[0][0] <= _pData.getOrigin()
                && _pInfo.dRanges[0][1] >= _pData.getOrigin()
                && _pInfo.dRanges[1][0] <= _pData.getOrigin(1)
                && _pInfo.dRanges[1][1] >= _pData.getOrigin(1)
                && _pInfo.dRanges[2][0] <= _pData.getOrigin(2)
                && _pInfo.dRanges[2][1] >= _pData.getOrigin(2)
                && _pInfo.nMaxPlotDim > 2 //(sCommand.find("3d") != string::npos || (_pInfo.b2D && sCommand != "dens"))
                )
            {
                _graph.SetOrigin(_pData.getOrigin(), _pData.getOrigin(1), _pData.getOrigin(2));
                if (!_pData.getSchematic())
                    _graph.Axis("AKDTVISO");
                else
                {
                    _graph.SetTickLen(1e-20);
                    _graph.SetTicks('x', -5, 1);
                    _graph.SetTicks('y', -5, 1);
                    _graph.SetTicks('z', -5, 1);
                    _graph.Axis("AKDTVISO_");
                }
            }
            else if (_pInfo.dRanges[0][0] <= _pData.getOrigin()
                && _pInfo.dRanges[0][1] >= _pData.getOrigin()
                && _pInfo.dRanges[1][0] <= _pData.getOrigin(1)
                && _pInfo.dRanges[1][1] >= _pData.getOrigin(1)
                && _pInfo.nMaxPlotDim <= 2 //(sCommand.find("3d") == string::npos && !(_pInfo.b2D && sCommand != "dens"))
                )
            {
                _graph.SetOrigin(_pData.getOrigin(), _pData.getOrigin(1));
                if (!_pData.getSchematic())
                    _graph.Axis("AKDTVISO");
                else
                {
                    _graph.SetTickLen(1e-20);
                    _graph.SetTicks('x', -5, 1);
                    _graph.SetTicks('y', -5, 1);
                    _graph.Axis("AKDTVISO_");
                }
            }
            else if (_pInfo.nMaxPlotDim == 3)
            {
                if (_pInfo.dRanges[0][0] <= _pData.getOrigin() && _pInfo.dRanges[0][1] >= _pData.getOrigin())
                {
                    if (_pInfo.dRanges[1][0] <= _pData.getOrigin(1) && _pInfo.dRanges[1][1] >= _pData.getOrigin(1))
                        _graph.SetOrigin(_pData.getOrigin(), _pData.getOrigin(1), _pInfo.dRanges[2][0]);
                    else if (_pInfo.dRanges[2][0] <= _pData.getOrigin(2) && _pInfo.dRanges[2][1] >= _pData.getOrigin(2))
                        _graph.SetOrigin(_pData.getOrigin(), _pInfo.dRanges[1][0], _pData.getOrigin(2));
                    else
                        _graph.SetOrigin(_pData.getOrigin(), _pInfo.dRanges[1][0], _pInfo.dRanges[2][0]);
                }
                else if (_pInfo.dRanges[1][0] <= _pData.getOrigin(1) && _pInfo.dRanges[1][1] >= _pData.getOrigin(1))
                {
                    if (_pInfo.dRanges[2][0] <= _pData.getOrigin(2) && _pInfo.dRanges[2][1] >= _pData.getOrigin(2))
                        _graph.SetOrigin(_pInfo.dRanges[0][0], _pData.getOrigin(1), _pData.getOrigin(2));
                    else
                        _graph.SetOrigin(_pInfo.dRanges[0][0], _pData.getOrigin(1), _pInfo.dRanges[2][0]);
                }
                else if (_pInfo.dRanges[2][0] <= _pData.getOrigin(2) && _pInfo.dRanges[2][1] >= _pData.getOrigin(2))
                    _graph.SetOrigin(_pInfo.dRanges[0][0], _pInfo.dRanges[1][0], _pData.getOrigin(2));
                else
                    _graph.SetOrigin(_pInfo.dRanges[0][0], _pInfo.dRanges[1][0], _pInfo.dRanges[2][0]);

                if (!_pData.getSchematic())
                    _graph.Axis("AKDTVISO");
                else
                {
                    _graph.SetTickLen(1e-20);
                    _graph.SetTicks('x', -5, 1);
                    _graph.SetTicks('y', -5, 1);
                    _graph.SetTicks('z', -5, 1);
                    _graph.Axis("AKDTVISO_");
                }
            }
            else if (_pInfo.nMaxPlotDim <= 2)
            {
                if (_pInfo.dRanges[0][0] <= _pData.getOrigin() && _pInfo.dRanges[0][1] >= _pData.getOrigin())
                {
                    _graph.SetOrigin(_pData.getOrigin(), _pInfo.dRanges[1][0]);
                }
                else if (_pInfo.dRanges[1][0] <= _pData.getOrigin(1) && _pInfo.dRanges[1][1] >= _pData.getOrigin(1))
                {
                    _graph.SetOrigin(_pInfo.dRanges[0][0], _pData.getOrigin(1));
                }
                else
                    _graph.SetOrigin(_pInfo.dRanges[0][0], _pInfo.dRanges[1][0]);

                if (!_pData.getSchematic())
                    _graph.Axis("AKDTVISO");
                else
                {
                    _graph.SetTickLen(1e-20);
                    _graph.SetTicks('x', -5, 1);
                    _graph.SetTicks('y', -5, 1);
                    _graph.Axis("AKDTVISO_");
                }
            }
        }
        else if (_pInfo.dRanges[0][0] <= 0.0
            && _pInfo.dRanges[0][1] >= 0.0
            && _pInfo.dRanges[1][0] <= 0.0
            && _pInfo.dRanges[1][1] >= 0.0
            && _pInfo.dRanges[2][0] <= 0.0
            && _pInfo.dRanges[2][1] >= 0.0
            && _pInfo.nMaxPlotDim > 2 //(sCommand.find("3d") != string::npos || (_pInfo.b2D && sCommand != "dens"))
            )
        {
            _graph.SetOrigin(0.0, 0.0, 0.0);
            if (!_pData.getSchematic())
                _graph.Axis("AKDTVISO");
            else
            {
                _graph.SetTickLen(1e-20);
                _graph.SetTicks('x', -5, 1);
                _graph.SetTicks('y', -5, 1);
                _graph.SetTicks('z', -5, 1);
                _graph.Axis("AKDTVISO_");
            }
        }
        else if (_pInfo.dRanges[0][0] <= 0.0
            && _pInfo.dRanges[0][1] >= 0.0
            && _pInfo.dRanges[1][0] <= 0.0
            && _pInfo.dRanges[1][1] >= 0.0
            && _pInfo.nMaxPlotDim <= 2 //(sCommand.find("3d") == string::npos && !(_pInfo.b2D && sCommand != "dens"))
            )
        {
            _graph.SetOrigin(0.0, 0.0);
            if (!_pData.getSchematic())
                _graph.Axis("AKDTVISO");
            else
            {
                _graph.SetTickLen(1e-20);
                _graph.SetTicks('x', -5, 1);
                _graph.SetTicks('y', -5, 1);
                _graph.Axis("AKDTVISO_");
            }
        }
        else if (_pInfo.nMaxPlotDim > 2) //sCommand.find("3d") != string::npos || (_pInfo.b2D && sCommand != "dens"))
        {
            _graph.SetOrigin(_pInfo.dRanges[0][0], _pInfo.dRanges[1][0], _pInfo.dRanges[2][0]);
            if (!_pData.getSchematic())
                _graph.Axis("AKDTVISO");
            else
            {
                _graph.SetTickLen(1e-20);
                _graph.SetTicks('x', -5, 1);
                _graph.SetTicks('y', -5, 1);
                _graph.SetTicks('z', -5, 1);
                _graph.Axis("AKDTVISO_");
            }
        }
        else
        {
            if (!_pData.getSchematic())
                _graph.Axis("AKDTVISO");
            else
            {
                _graph.SetTickLen(1e-20);
                _graph.SetTicks('x', -5, 1);
                _graph.SetTicks('y', -5, 1);
                _graph.SetTicks('z', -5, 1);
                _graph.Axis("AKDTVISO_");
            }
        }
    }
    if (!_pData.getCoords())
    {
        if (_pData.getGrid() && !_pInfo.b2DVect && !_pInfo.b3DVect) // Standard-Grid
        {
            if (_pData.getGrid() == 1)
                _graph.Grid("xyzt", _pData.getGridStyle().c_str());
            else if (_pData.getGrid() == 2)
            {
                _graph.Grid("xyzt!", _pData.getGridStyle().c_str());
                _graph.Grid("xyzt", _pData.getFineGridStyle().c_str());
            }
        }
        else if (_pData.getGrid()) // Vektor-Grid
            _graph.Grid("xyzt", _pData.getFineGridStyle().c_str());
        if (_pData.getBox())
        {
            if (!_pData.getSchematic())
                _graph.Box();
            else
                _graph.Box("k", false);
        }

        // --> Achsen beschriften <--
        if (_pData.getAxis()
            && _pData.getBox()
            && (!_pData.getSchematic()
                || matchParams(_pInfo.sPlotParams, "xlabel", '=')
                || matchParams(_pInfo.sPlotParams, "ylabel", '=')
                || matchParams(_pInfo.sPlotParams, "zlabel", '=')))
        {
            _graph.Label('x', fromSystemCodePage(_pData.getxLabel()).c_str(), 0.0);
            _graph.Label('y', fromSystemCodePage(_pData.getyLabel()).c_str(), 0.0);
            _graph.Label('z', fromSystemCodePage(_pData.getzLabel()).c_str(), 0.0);
            //_graph.Label('t', fromSystemCodePage(_pData.getzLabel()).c_str(), 0.0);
        }
        else if (_pData.getAxis()
            && !_pData.getBox()
            && (!_pData.getSchematic()
                || matchParams(_pInfo.sPlotParams, "xlabel", '=')
                || matchParams(_pInfo.sPlotParams, "ylabel", '=')
                || matchParams(_pInfo.sPlotParams, "zlabel", '=')))
        {
            _graph.Label('x', fromSystemCodePage(_pData.getxLabel()).c_str(), 1.1);
            _graph.Label('y', fromSystemCodePage(_pData.getyLabel()).c_str(), 1.1);
            _graph.Label('z', fromSystemCodePage(_pData.getzLabel()).c_str(), 1.1);
        }
    }
    return;
}

string parser_CoordFunc(const string& sFunc, double dPhiScale, double dThetaScale)
{
    string sParsedFunction = sFunc;

    if (dPhiScale == 1.0)
    {
        while (sParsedFunction.find("$PS$") != string::npos)
            sParsedFunction.replace(sParsedFunction.find("$PS$"), 4, "1");
    }
    else
    {
        while (sParsedFunction.find("$PS$") != string::npos)
            sParsedFunction.replace(sParsedFunction.find("$PS$"), 4, "(" + toString(dPhiScale, 5) + ")");
    }
    if (dThetaScale == 1.0)
    {
        while (sParsedFunction.find("$TS$") != string::npos)
            sParsedFunction.replace(sParsedFunction.find("$TS$"), 4, "1");
    }
    else
    {
        while (sParsedFunction.find("$TS$") != string::npos)
            sParsedFunction.replace(sParsedFunction.find("$TS$"), 4, "(" + toString(dThetaScale, 5) + ")");
    }

    return sParsedFunction;
}

bool checkMultiPlotArray(unsigned int nMultiPlot[2], unsigned int& nSubPlotMap, unsigned int nPlotPos, unsigned int nCols, unsigned int nLines)
{                                     // cols, lines
    if (nPlotPos + nCols-1 >= (nPlotPos / nMultiPlot[0]+1)*nMultiPlot[0])
        return false;
    if (nPlotPos/nMultiPlot[0]+nLines-1 >= nMultiPlot[1])
        return false;
    unsigned int nCol0, nLine0, pos;
    nCol0 = nPlotPos % nMultiPlot[0];
    nLine0 = nPlotPos / nMultiPlot[0];

    //cerr << nCols << " " << nLines << endl;
    //cerr << nCol0 << " " << nLine0 << endl;

    for (unsigned int i = 0; i < nLines; i++)
    {
        for (unsigned int j = 0; j < nCols; j++)
        {
            //cerr << ((nCol0+j)+nMultiPlot[0]*(nLine0+i)) << endl;
            pos = 1;
            pos <<= ((nCol0+j)+nMultiPlot[0]*(nLine0+i));
            if (pos & nSubPlotMap)
                return false;
        }
    }
    for (unsigned int i = 0; i < nLines; i++)
    {
        for (unsigned int j = 0; j < nCols; j++)
        {
            pos = 1;
            pos <<= ((nCol0+j)+nMultiPlot[0]*(nLine0+i));
            nSubPlotMap |= pos;
        }
    }
    return true;
}
