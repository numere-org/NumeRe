
#include "graph_helper.hpp"

Graph_helper::Graph_helper(string& sCommand, Datafile* __data, Parser* __parser, Settings* __option, Define* __functions, PlotData* __pData)
{
    __sCmd = sCommand;
    _data = __data;
    _parser = __parser;
    _option = __option;
    _functions = __functions;
    _pData = __pData;

    _mDataPlots = 0;
    nDataDim = 0;
    nDataPlots = 0;
}

Graph_helper::~Graph_helper()
{
    if (_mDataPlots)
    {
        for (int i = 0; i < nDataPlots; i++)
            delete[] _mDataPlots[i];
        delete[] _mDataPlots;
        _mDataPlots = 0;
        nDataPlots = 0;
    }
    if (nDataDim)
    {
        delete[] nDataDim;
        nDataDim = 0;
    }
}

int Graph_helper::Draw(mglGraph* _graph)
{
    /*! --> Wenn in der ersten Zeile und der ersten Spalte x- und y-Werte stehen, koennten auch mesh-/surf-Plots
     *      aus Daten generiert werden <--
     */

    string sFunc = "";                      // string mit allen Funktionen
    string sLabels = "";                    // string mit den Namen aller Funktionen (Fuer die Legende)
    string sDataLabels = "";                // string mit den Datenpunkt-Namen (Fuer die Legende)
    string sDataPlots = "";                 // string mit allen Datenpunkt-Plot-Ausdruecken
    string sCommand = "";    // string mit dem Plot-Kommando (mesh, grad, plot, surf, cont, etc.)
    vector<string> vPlotCompose;

    mglData _mBackground;                   // mglData-Objekt fuer ein evtl. Hintergrundbild
    int* nDataDim = 0;                      // Pointer auf die jeweilige Dimension der einzelnen Datensaetze

    double dRanges[3][2];                   // Fuer die berechneten Plot-Intervalle
    double dDataRanges[3][2];               // Fuer die berechneten Daten-Intervalle (hoehere Prioritaet)
    double dColorRanges[2];
    int nSamples;                           // Zahl der Datenpunkte
    bool b2D = false;                       // 2D-Plot ja/nein
    bool b3D = false;                       // 3D-Plot ja/nein
    bool b2DVect = false;
    bool b3DVect = false;
    bool bDraw = false;
    bool bDraw3D = false;
    vector<string> vDrawVector;
    int nStyle = 0;                         // Nummer des aktuellen Plotstyles (automatische Variation des Styles)
    const int nStyleMax = 14;               // Gesamtzahl der Styles
    int nFunctions = 0;                     // Zahl der zu plottenden Funktionen
    string sConvLegends = "";               // Variable fuer Legenden, die dem String-Parser zugewiesen werden
    string sDummy = "";
    string sPlotParams = "";
    unsigned int nLegends = 0;
    unsigned int nMaxPlotDim = 1;
    string sCmd = __sCmd;

    if (findCommand(sCmd).sString == "plotcompose")
    {
        sCmd.erase(findCommand(sCmd).nPos, 11);
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
                nMaxPlotDim = 3;
            }
            else if (__sCMD.substr(0,4) == "mesh" || __sCMD.substr(0,4) == "surf" || __sCMD.substr(0,4) == "cont")
            {
                nMaxPlotDim = 3;
            }
            else if (__sCMD.substr(0,4) == "vect" || __sCMD.substr(0,4) == "dens" || __sCMD.substr(0,4) == "grad")
            {
                if (nMaxPlotDim < 3)
                    nMaxPlotDim = 2;
            }
            vPlotCompose.push_back(sCmd.substr(0, sCmd.find("<<COMPOSE>>")));
            sCmd.erase(0, sCmd.find("<<COMPOSE>>")+11);
            StripSpaces(sCmd);
        }
        for (unsigned int i = 0; i < vPlotCompose.size(); i++)
        {
            if (vPlotCompose[i].find("--") != string::npos)
                sPlotParams += vPlotCompose[i].substr(vPlotCompose[i].find("--"));
            else if (vPlotCompose[i].find("-set") != string::npos)
                sPlotParams += vPlotCompose[i].substr(vPlotCompose[i].find("-set"));
            sPlotParams += " ";
        }
        if (sPlotParams.length())
        {
            if (sPlotParams.find("??") != string::npos)
            {
                sPlotParams = parser_Prompt(sPlotParams);
            }
            if (!_functions->call(sPlotParams, *_option))
                throw FUNCTION_ERROR;
            if ((containsStrings(sPlotParams) || _data->containsStringVars(sPlotParams))
                && sPlotParams.find('=') != string::npos)
            {
                if (_data->containsStringVars(sPlotParams))
                    _data->getStringValues(sPlotParams);
                unsigned int nPos = 0;
                while (sPlotParams.find('=', nPos) != string::npos)
                {
                    nPos = sPlotParams.find('=', nPos)+1;
                    if (nPos >= sPlotParams.length())
                        break;
                    while (sPlotParams[nPos] == ' ')
                        nPos++;
                    if ((sPlotParams[nPos] != '"'
                            && sPlotParams[nPos] != '#'
                            && sPlotParams[nPos] != '('
                            && sPlotParams.substr(nPos, 10) != "to_string("
                            && sPlotParams.substr(nPos, 12) != "string_cast("
                            && sPlotParams.substr(nPos, 8) != "to_char("
                            && sPlotParams.substr(nPos, 8) != "replace("
                            && sPlotParams.substr(nPos, 11) != "replaceall("
                            && sPlotParams.substr(nPos, 5) != "char("
                            && sPlotParams.substr(nPos, 7) != "string("
                            && sPlotParams.substr(nPos, 7) != "substr("
                            && sPlotParams.substr(nPos, 6) != "split("
                            && sPlotParams.substr(nPos, 6) != "sum("
                            && sPlotParams.substr(nPos, 6) != "min("
                            && sPlotParams.substr(nPos, 6) != "max("
                            && sPlotParams.substr(nPos, 5) != "data("
                            && sPlotParams.substr(nPos, 6) != "cache(")
                        || isInQuotes(sPlotParams, nPos-1))
                        continue;
                    if (sPlotParams.substr(nPos,4) == "min(" || sPlotParams.substr(nPos,4) == "max(" || sPlotParams.substr(nPos,4) == "sum(")
                    {
                        int nPos_temp = getMatchingParenthesis(sPlotParams.substr(nPos+3))+nPos+3;
                        if (!containsStrings(sPlotParams.substr(nPos+3, nPos_temp-nPos-3)) && !_data->containsStringVars(sPlotParams.substr(nPos+3, nPos_temp-nPos-3)))
                            continue;
                    }
                    if (sPlotParams[nPos] == '(')
                    {
                        int nPos_temp = getMatchingParenthesis(sPlotParams.substr(nPos))+nPos;
                        if (!containsStrings(sPlotParams.substr(nPos, nPos_temp-nPos)) && !_data->containsStringVars(sPlotParams.substr(nPos, nPos_temp-nPos)))
                            continue;
                        else
                        {
                            nPos_temp = nPos+1;
                            while (!containsStrings(sPlotParams.substr(nPos, nPos_temp-nPos)) && !_data->containsStringVars(sPlotParams.substr(nPos, nPos_temp-nPos)))
                                nPos_temp++;
                            nPos = nPos_temp-1;
                        }
                    }
                    for (unsigned int i = nPos; i < sPlotParams.length(); i++)
                    {
                        if (sPlotParams[i] == '(')
                        {
                            i += getMatchingParenthesis(sPlotParams.substr(i));
                            continue;
                        }
                        if (((sPlotParams[i] == ' ' || sPlotParams[i] == ')') && !isInQuotes(sPlotParams, i)) || i+1 == sPlotParams.length())
                        {
                            string sParsedString;
                            if (i+1 == sPlotParams.length())
                                sParsedString = sPlotParams.substr(nPos);
                            else
                                sParsedString = sPlotParams.substr(nPos, i-nPos);
                            if (!parser_StringParser(sParsedString, sDummy, *_data, *_parser, *_option, true))
                            {
                                throw STRING_ERROR;
                            }
                            if (i+1 == sPlotParams.length())
                                sPlotParams.replace(nPos, string::npos, sParsedString);// = sPlotParams.substr(0, nPos) + sParsedString;
                            else
                                sPlotParams.replace(nPos, i-nPos, sParsedString);// = sPlotParams.substr(0, nPos) + sParsedString + sPlotParams.substr(i);
                            break;
                        }
                    }
                }
            }
            //sPlotParams = "-set minline=\"\" maxline=\"\" hline=\"\" vline=\"\" lborder=\"\" rborder=\"\" " + sPlotParams;
            //cerr << sPlotParams << endl;
            _pData->setGlobalComposeParams(sPlotParams, *_parser, *_option);
        }
        sPlotParams = "";
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
            nMaxPlotDim = 3;
        }
        else if (__sCMD.substr(0,4) == "mesh" || __sCMD.substr(0,4) == "surf" || __sCMD.substr(0,4) == "cont")
        {
            nMaxPlotDim = 3;
        }
        else if (__sCMD.substr(0,4) == "vect" || __sCMD.substr(0,4) == "dens" || __sCMD.substr(0,4) == "grad")
        {
            if (nMaxPlotDim < 3)
                nMaxPlotDim = 2;
        }

        vPlotCompose.push_back(sCmd);
    }


    // String-Arrays fuer die endgueltigen Styles:
    string sLineStyles[nStyleMax];
    string sContStyles[nStyleMax];
    string sPointStyles[nStyleMax];
    string sConPointStyles[nStyleMax];

    string sOutputName = ""; // string fuer den Export-Dateinamen
    value_type* vResults;                   // Pointer auf ein Ergebnis-Array (fuer den muParser)

    // --> Diese Zeile klaert eine gefuellte Zeile, die nicht mit "endl" abgeschlossen wurde <--
    if (!_pData->getSilentMode() && _option->getSystemPrintStatus())
        cerr << "                                              \r";

    for (unsigned int nPlotCompose = 0; nPlotCompose < vPlotCompose.size(); nPlotCompose++)
    {
        sCmd = vPlotCompose[nPlotCompose];
        sPlotParams = "";
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
            _pData->deleteData();
            b2D = false;
            b3D = false;
            b2DVect = false;
            b3DVect = false;
            bDraw3D = false;
            bDraw = false;
            vDrawVector.clear();
            sLabels = "";
        }
        sCommand = findCommand(sCmd).sString;
        if (sCmd.find("-set") != string::npos)
            sFunc = sCmd.substr(sCommand.length(), sCmd.find("-set")-sCommand.length());
        else if (sCmd.find("--") != string::npos)
            sFunc = sCmd.substr(sCommand.length(), sCmd.find("--")-sCommand.length());
        else
            sFunc = sCmd.substr(sCommand.length());

        // --> Unnoetige Leerstellen entfernen <--
        StripSpaces(sFunc);

        // --> Ruf' ggf. den Prompt auf <--
        if (sFunc.find("??") != string::npos)
            sFunc = parser_Prompt(sFunc);

        // --> Ggf. die Plotparameter setzen <--
        if (sCmd.find("--") != string::npos)
            sPlotParams = sCmd.substr(sCmd.find("--"));
        else if (sCmd.find("-set") != string::npos)
            sPlotParams = sCmd.substr(sCmd.find("-set"));
        if (sPlotParams.length())
        {
            if (sPlotParams.find("??") != string::npos)
            {
                sPlotParams = parser_Prompt(sPlotParams);
            }
            if (!_functions->call(sPlotParams, *_option))
                throw FUNCTION_ERROR;
            if ((containsStrings(sPlotParams) || _data->containsStringVars(sPlotParams))
                && sPlotParams.find('=') != string::npos)
            {
                unsigned int nPos = 0;
                if (_data->containsStringVars(sPlotParams))
                    _data->getStringValues(sPlotParams);
                while (sPlotParams.find('=', nPos) != string::npos)
                {
                    nPos = sPlotParams.find('=', nPos)+1;
                    if (nPos >= sPlotParams.length())
                        break;
                    while (sPlotParams[nPos] == ' ')
                        nPos++;
                    if ((sPlotParams[nPos] != '"'
                            && sPlotParams[nPos] != '#'
                            && sPlotParams[nPos] != '('
                            && sPlotParams.substr(nPos, 10) != "to_string("
                            && sPlotParams.substr(nPos, 12) != "string_cast("
                            && sPlotParams.substr(nPos, 8) != "to_char("
                            && sPlotParams.substr(nPos, 8) != "replace("
                            && sPlotParams.substr(nPos, 11) != "replaceall("
                            && sPlotParams.substr(nPos, 5) != "char("
                            && sPlotParams.substr(nPos, 7) != "string("
                            && sPlotParams.substr(nPos, 7) != "substr("
                            && sPlotParams.substr(nPos, 6) != "split("
                            && sPlotParams.substr(nPos, 6) != "sum("
                            && sPlotParams.substr(nPos, 6) != "min("
                            && sPlotParams.substr(nPos, 6) != "max("
                            && sPlotParams.substr(nPos, 5) != "data("
                            && sPlotParams.substr(nPos, 6) != "cache(")
                        || isInQuotes(sPlotParams, nPos-1))
                        continue;
                    if (sPlotParams.substr(nPos,4) == "min(" || sPlotParams.substr(nPos,4) == "max(" || sPlotParams.substr(nPos,4) == "sum(")
                    {
                        int nPos_temp = getMatchingParenthesis(sPlotParams.substr(nPos+3))+nPos+3;
                        if (!containsStrings(sPlotParams.substr(nPos+3, nPos_temp-nPos-3)) && !_data->containsStringVars(sPlotParams.substr(nPos+3, nPos_temp-nPos-3)))
                            continue;
                    }
                    if (sPlotParams[nPos] == '(')
                    {
                        int nPos_temp = getMatchingParenthesis(sPlotParams.substr(nPos))+nPos;
                        if (!containsStrings(sPlotParams.substr(nPos, nPos_temp-nPos)) && !_data->containsStringVars(sPlotParams.substr(nPos, nPos_temp-nPos)))
                            continue;
                        else
                        {
                            nPos_temp = nPos+1;
                            while (!containsStrings(sPlotParams.substr(nPos, nPos_temp-nPos)) && !_data->containsStringVars(sPlotParams.substr(nPos, nPos_temp-nPos)))
                                nPos_temp++;
                            nPos = nPos_temp-1;
                        }
                    }
                    for (unsigned int i = nPos; i < sPlotParams.length(); i++)
                    {
                        if (sPlotParams[i] == '(')
                        {
                            i += getMatchingParenthesis(sPlotParams.substr(i));
                            continue;
                        }
                        if (((sPlotParams[i] == ' ' || sPlotParams[i] == ')') && !isInQuotes(sPlotParams, i)) || i+1 == sPlotParams.length())
                        {
                            string sParsedString;
                            if (i+1 == sPlotParams.length())
                                sParsedString = sPlotParams.substr(nPos);
                            else
                                sParsedString = sPlotParams.substr(nPos, i-nPos);
                            if (!parser_StringParser(sParsedString, sDummy, *_data, *_parser, *_option, true))
                            {
                                throw STRING_ERROR;
                            }
                            if (i+1 == sPlotParams.length())
                                sPlotParams.replace(nPos, string::npos, sParsedString);// = sPlotParams.substr(0, nPos) + sParsedString;
                            else
                                sPlotParams.replace(nPos, i-nPos, sParsedString);// = sPlotParams.substr(0, nPos) + sParsedString + sPlotParams.substr(i);
                            break;
                        }
                    }
                }
            }
            if (vPlotCompose.size() > 1)
                _pData->setLocalComposeParams(sPlotParams, *_parser, *_option);
            else
                _pData->setParams(sPlotParams, *_parser, *_option);
        }

        if (!sFunc.length())
            continue;

        if (!nPlotCompose)
        {
            if (_pData->getHighRes() == 2)           // Aufloesung und Qualitaet einstellen
            {
                double dHeight = sqrt(1920.0 * 1440.0); // _pData->getAspect());
                _graph->SetSize((int)lrint(dHeight), (int)lrint(dHeight));          // FullHD!
            }
            else if (_pData->getHighRes() == 1 || !_option->getbUseDraftMode())
            {
                double dHeight = sqrt(1280.0 * 960.0); // _pData->getAspect());
                _graph->SetSize((int)lrint(dHeight), (int)lrint(dHeight));           // ehem. die bessere Standard-Aufloesung
                //_graph->SetQuality(5);
            }
            else
            {
                double dHeight = sqrt(800.0 * 600.0); // _pData->getAspect());
                _graph->SetSize((int)lrint(dHeight), (int)lrint(dHeight));
                // --> Im Falle, dass wir meinen mesh/surf/anders gearteten 3D-Plot machen, drehen wir die Qualitaet runter <--
                if (sCommand.substr(0,4) != "plot")
                    _graph->SetQuality(MGL_DRAW_FAST);
            }
            // --> Noetige Einstellungen und Deklarationen fuer den passenden Plot-Stil <--
            _graph->CopyFont(&_fontData);
            _graph->SetFontSizeCM(0.24*((double)(1+_pData->getTextSize())/6.0), 72);
        }

        /*if (_pData->getColorScheme() == "kw" || _pData->getColorScheme() == "wk")
        {
            bGreyScale = true;
        }*/
        for (int i = 0; i < nStyleMax; i++)
        {
            sLineStyles[i] = " ";
            sLineStyles[i][0] = _pData->getColors()[i];
            sPointStyles[i] = " ";
            sPointStyles[i][0] = _pData->getColors()[i];
            sContStyles[i] = " ";
            sContStyles[i][0] = _pData->getContColors()[i];
            sConPointStyles[i] = " ";
            sConPointStyles[i][0] = _pData->getColors()[i];
        }
        for (int i = 0; i < nStyleMax; i++)
        {
            sLineStyles[i] += _pData->getLineStyles()[i];
            if (_pData->getDrawPoints())
            {
                if (_pData->getPointStyles()[2*i] != ' ')
                    sLineStyles[i] += _pData->getPointStyles()[2*i];
                sLineStyles[i] += _pData->getPointStyles()[2*i+1];
            }
            if (_pData->getyError() || _pData->getxError())
            {
                if (_pData->getPointStyles()[2*i] != ' ')
                    sPointStyles[i] += _pData->getPointStyles()[2*i];
                sPointStyles[i] += _pData->getPointStyles()[2*i+1];
            }
            else
            {
                if (_pData->getPointStyles()[2*i] != ' ')
                    sPointStyles[i] += " " + _pData->getPointStyles().substr(2*i,1) + _pData->getPointStyles().substr(2*i+1,1);
                else
                    sPointStyles[i] += " " + _pData->getPointStyles().substr(2*i+1,1);
            }
            if (_pData->getPointStyles()[2*i] != ' ')
                sConPointStyles[i] += _pData->getLineStyles().substr(i,1) + _pData->getPointStyles().substr(2*i,1) + _pData->getPointStyles().substr(2*i+1,1);
            else
                sConPointStyles[i] += _pData->getLineStyles().substr(i,1) + _pData->getPointStyles().substr(2*i+1,1);
            sContStyles[i] += _pData->getLineStyles()[i];
            //cerr << sPointStyles[i] << endl;
            //cerr << sLineStyles[i] << endl;
        }
        //cerr << _pData->getPointStyles() << endl;
        //cerr << _pData->getLineStyles() << endl;

        if (_pData->getBars())
            _graph->SetBarWidth(0.9);

        // --> Allg. Einstellungen <--
        //_graph->SetFontSizeCM(0.28, 72);
        if (!_pData->getAnimateSamples()
            || (_pData->getAnimateSamples()
                && sCommand.substr(0,4) != "mesh"
                && sCommand.substr(0,4) != "surf"
                && sCommand.substr(0,4) != "grad"
                && sCommand.substr(0,4) != "cont"))
            nSamples = _pData->getSamples();
        else
            nSamples = 100;

        // --> Ggf. waehlen eines Default-Dateinamens <--
/*        if (!_pData->getFileName().length() && !nPlotCompose)
        {
            string sExt = ".png";
            if (_pData->getAnimateSamples())
                sExt = ".gif";
            if (vPlotCompose.size() > 1)
                _pData->setFileName("composition"+sExt);
            else if (sCommand == "plot3d")
                _pData->setFileName("plot3d"+sExt);
            else if (sCommand == "plot")
                _pData->setFileName("plot"+sExt);
            else if (sCommand == "meshgrid3d" || sCommand.substr(0,6) == "mesh3d")
                _pData->setFileName("meshgrid3d"+sExt);
            else if (sCommand.substr(0,4) == "mesh")
                _pData->setFileName("meshgrid"+sExt);
            else if (sCommand.substr(0,6) == "surf3d" || sCommand == "surface3d")
                _pData->setFileName("surface3d"+sExt);
            else if (sCommand.substr(0,4) == "surf")
                _pData->setFileName("surface"+sExt);
            else if (sCommand.substr(0,6) == "cont3d" || sCommand == "contour3d")
                _pData->setFileName("contour3d"+sExt);
            else if (sCommand.substr(0.4) == "cont")
                _pData->setFileName("contour"+sExt);
            else if (sCommand.substr(0,6) == "grad3d" || sCommand == "gradient3d")
                _pData->setFileName("gradient3d"+sExt);
            else if (sCommand.substr(0,4) == "grad")
                _pData->setFileName("gradient"+sExt);
            else if (sCommand == "density3d" || sCommand.substr(0,6) == "dens3d")
                _pData->setFileName("density3d"+sExt);
            else if (sCommand.substr(0,4) == "dens")
                _pData->setFileName("density"+sExt);
            else if (sCommand.substr(0,6) == "vect3d" || sCommand == "vector3d")
                _pData->setFileName("vectorfield3d"+sExt);
            else if (sCommand.substr(0,4) == "vect")
                _pData->setFileName("vectorfield"+sExt);
            else if (sCommand.substr(0,6) == "draw3d")
                _pData->setFileName("drawing3d"+sExt);
            else if (sCommand.substr(0,4) == "draw")
                _pData->setFileName("drawing"+sExt);
            else
                _pData->setFileName("unknown_style"+sExt);
        }
        if ((containsStrings(_pData->getFileName()) || _data->containsStringVars(_pData->getFileName())) && !nPlotCompose)
        {
            string sTemp = _pData->getFileName();
            string sTemp_2 = "";
            string sExtension = sTemp.substr(sTemp.find('.'));
            sTemp = sTemp.substr(0,sTemp.find('.'));
            if (sExtension[sExtension.length()-1] == '"')
            {
                sTemp += "\"";
                sExtension = sExtension.substr(0,sExtension.length()-1);
            }
            parser_StringParser(sTemp, sTemp_2, *_data, *_parser, *_option, true);
            _pData->setFileName(sTemp.substr(1,sTemp.length()-2)+sExtension);
            if (_option->getbDebug())
                cerr << "|-> DEBUG: _pData->getFileName() = " << _pData->getFileName() << endl;
        }
        if (_pData->getAnimateSamples() && _pData->getFileName().substr(_pData->getFileName().rfind('.')) != ".gif" && !nPlotCompose)
            _pData->setFileName(_pData->getFileName().substr(0, _pData->getFileName().length()-4) + ".gif");
*/
        if (!nPlotCompose)
        {
            sOutputName = _pData->getFileName();
            StripSpaces(sOutputName);
            if (sOutputName[0] == '"' && sOutputName[sOutputName.length()-1] == '"')
                sOutputName = sOutputName.substr(1,sOutputName.length()-2);
        }
        //if (!_pData->getSilentMode() && _option->getSystemPrintStatus())
            //cerr << toSystemCodePage("|-> Berechne Daten für ");

        if (sCommand == "surface3d"
            || sCommand.substr(0,6) == "surf3d"
            || sCommand == "meshgrid3d"
            || sCommand.substr(0,6) == "mesh3d"
            || sCommand == "contour3d"
            || sCommand.substr(0,6) == "cont3d"
            || sCommand == "density3d"
            || sCommand.substr(0,6) == "dens3d"
            || sCommand == "gradient3d"
            || sCommand.substr(0,6) == "grad3d")
        {
            //_parser->DefineVar(parser_iVars.sName[1], &parser_iVars.vValue[1][0]);   // Plotvariable: y
            //_parser->DefineVar(parser_iVars.sName[2], &parser_iVars.vValue[2][0]);   // Plotvariable: z
            b3D = true;
            if (nSamples > 50)
            {
                if (_pData->getHighRes() == 2 && nSamples > 100)
                    nSamples = 100;
                else if ((_pData->getHighRes() == 1 || !_option->getbUseDraftMode()) && nSamples > 74)
                    nSamples = 100;
                else
                    nSamples = 50;
            }
            /*if (!_pData->getSilentMode() && _option->getSystemPrintStatus() && sCommand.substr(0,4) == "surf")
                //cerr << toSystemCodePage("3D-Oberflächen-");
            else if (!_pData->getSilentMode() && _option->getSystemPrintStatus() && sCommand.substr(0,4) == "mesh")
                //cerr << "3D-Meshgrid-";
            else if (!_pData->getSilentMode() && _option->getSystemPrintStatus() && sCommand.substr(0,4) == "cont")
                //cerr << "3D-Kontur-";
            else if (!_pData->getSilentMode() && _option->getSystemPrintStatus() && sCommand.substr(0,4) == "dens")
                //cerr << "3D-Dichte-";
            else if (!_pData->getSilentMode() && _option->getSystemPrintStatus() && sCommand.substr(0,4) == "grad")
                */
                //cerr << "3D-Gradienten-";
        }
        else if (sCommand.substr(0,6) == "vect3d" ||sCommand == "vector3d")
        {
            //_parser->DefineVar(parser_iVars.sName[1], &parser_iVars.vValue[1][0]);   // Plotvariable: y
            //_parser->DefineVar(parser_iVars.sName[2], &parser_iVars.vValue[2][0]);   // Plotvariable: z
            b3DVect = true;
            if (nSamples > 11)
                nSamples = 11;
            if (!_pData->getSilentMode() && _option->getSystemPrintStatus())
                //cerr << "3D-Vektorfeld-";
            if (_pData->getPipe() || _pData->getFlow())
            {
                if (nSamples % 2)
                    nSamples -= 1;
                if (_pData->getHighRes() <= 1 && _option->getbUseDraftMode())
                    _graph->SetQuality(5);
                else
                    _graph->SetQuality(6);
            }
        }
        else if (sCommand.substr(0,4) == "vect")
        {
            //_parser->DefineVar(parser_iVars.sName[1], & parser_iVars.vValue[1][0]);  // Plotvariable: y
            b2DVect = true;
            if (nSamples > 21)
                nSamples = 21;
            if (!_pData->getSilentMode() && _option->getSystemPrintStatus())
                //cerr << "Vektorfeld-";
            if (_pData->getPipe() || _pData->getFlow())
            {
                if (_pData->getHighRes() <= 1 && _option->getbUseDraftMode())
                    _graph->SetQuality(5);
                else
                    _graph->SetQuality(6);
            }
        }
        else if (sCommand.substr(0,4) == "mesh"
            || sCommand.substr(0,4) == "surf"
            || sCommand.substr(0,4) == "cont"
            || sCommand.substr(0,4) == "grad"
            || sCommand.substr(0,4) == "dens")
        {
            //_parser->DefineVar(parser_iVars.sName[1], &parser_iVars.vValue[1][0]);   // Plotvariable: y
            b2D = true;
            /*if (!_pData->getSilentMode() && _option->getSystemPrintStatus() && sCommand.substr(0,4) == "surf")
                //cerr << toSystemCodePage("2D-Oberflächen-");
            else if (!_pData->getSilentMode() && _option->getSystemPrintStatus() && sCommand.substr(0,4) == "mesh")
                //cerr << "2D-Meshgrid-";
            else if (!_pData->getSilentMode() && _option->getSystemPrintStatus() && sCommand.substr(0,4) == "cont")
                //cerr << "2D-Kontur-";
            else if (!_pData->getSilentMode() && _option->getSystemPrintStatus() && sCommand.substr(0,4) == "dens")
                //cerr << "2D-Dichte-";
            else if (!_pData->getSilentMode() && _option->getSystemPrintStatus() && sCommand.substr(0,4) == "grad")
                //cerr << "2D-Gradienten-";
                */
        }
        else if (sCommand == "plot3d")
        {
            //_parser->DefineVar(parser_iVars.sName[1], &parser_iVars.vValue[1][0]);   // Plotvariable: y
            //_parser->DefineVar(parser_iVars.sName[2], &parser_iVars.vValue[2][0]);   // Plotvariable: z
            //_parser->DefineVar(parser_iVars.sName[3], &parser_iVars.vValue[3][0]);   // Plotparameter: t
            if (!_pData->getSilentMode() && _option->getSystemPrintStatus())
                cerr << "3D-";
        }
        else if (sCommand == "draw")
        {
            bDraw = true;
            //if (!_pData->getSilentMode() && _option->getSystemPrintStatus())
                //cerr << "Zeichnung ... " << endl;
        }
        else if (sCommand == "draw3d")
        {
            bDraw3D = true;
            //if (!_pData->getSilentMode() && _option->getSystemPrintStatus())
                //cerr << "3D-Zeichnung ... " << endl;
        }
        /*if (!_pData->getSilentMode() && _option->getSystemPrintStatus() && !_pData->getAnimateSamples() && !(bDraw3D || bDraw))
            //cerr << "Plot ... " << endl;
        else if (!_pData->getSilentMode() && _option->getSystemPrintStatus() && !(bDraw3D || bDraw))
        {
            cerr << "Animation: Bitte etwas Geduld ... " << endl << "|-> ... ";
            //_parser->DefineVar(parser_iVars.sName[3], &parser_iVars.vValue[3][0]);   // Animations-Variable: t
        }*/
        // --> Logarithmische Skalierung; ein bisschen Fummelei <--
        if (!_pData->getCoords())
        {
            if (_pData->getxLogscale() || _pData->getyLogscale() || _pData->getzLogscale())
                _graph->SetRanges(0.1,10.0, 0.1,10.0, 0.1,10.0);
            if (_pData->getxLogscale() && !_pData->getyLogscale() && !_pData->getzLogscale())
                _graph->SetFunc("lg(x)", "");
            else if (_pData->getxLogscale() && _pData->getyLogscale() && !_pData->getzLogscale())
                _graph->SetFunc("lg(x)", "lg(y)");
            else if (_pData->getxLogscale() && _pData->getyLogscale() && _pData->getzLogscale() && (b2D || sCommand == "plot3d"))
                _graph->SetFunc("lg(x)", "lg(y)", "lg(z)");
            else if (_pData->getxLogscale() && _pData->getyLogscale() && _pData->getzLogscale() && !(b2D || sCommand == "plot3d"))
                _graph->SetFunc("lg(x)", "lg(y)");
            else if (!_pData->getxLogscale() && _pData->getyLogscale() && !_pData->getzLogscale())
                _graph->SetFunc("", "lg(y)");
            else if (!_pData->getxLogscale() && _pData->getyLogscale() && _pData->getzLogscale() && (b2D || sCommand == "plot3d"))
                _graph->SetFunc("", "lg(y)", "lg(z)");
            else if (!_pData->getxLogscale() && !_pData->getyLogscale() && _pData->getzLogscale() && (b2D || sCommand == "plot3d"))
                _graph->SetFunc("", "", "lg(z)");
            else if (_pData->getxLogscale() && !_pData->getyLogscale() && _pData->getzLogscale() && (b2D || sCommand == "plot3d"))
                _graph->SetFunc("lg(x)", "", "lg(z)");
            else if (!_pData->getxLogscale() && _pData->getyLogscale() && _pData->getzLogscale() && !(b2D || sCommand == "plot3d"))
                _graph->SetFunc("", "lg(y)");
            else if (_pData->getxLogscale() && !_pData->getyLogscale() && _pData->getzLogscale() && !(b2D || sCommand == "plot3d"))
                _graph->SetFunc("lg(x)", "");
        }

        if (!bDraw3D && !bDraw)
        {
            // --> Bevor die Call-Methode auf Define angewendet wird, sollten wir die Legenden-"strings" in sFunc kopieren <--
            if (!addLegends(sFunc))
                throw STRING_ERROR; // --> Bei Fehlern: Zurueck zur aufrufenden Funktion <--
        }

        if (_option->getbDebug())
        {
            cerr << "|-> DEBUG: Returned; sFunc = " << sFunc << endl;
        }
        // --> Ersetze Definitionen durch die tatsaechlichen Ausdruecke <--
        if (!_functions->call(sFunc, *_option))
            throw FUNCTION_ERROR;

        // --> Ruf' ggf. nochmals den Prompt auf (nicht dass es wirklich zu erwarten waere) <--
        if (sFunc.find("??") != string::npos)
            sFunc = parser_Prompt(sFunc);

        /*--> Hier werden die Datenpunkt-Plots an die mglData-Instanz _mDataPlots zugewiesen.
         *    Dabei erlaubt die Syntax sowohl eine gezielte Wahl von Datenreihen (Spalten),
         *    als auch eine automatische. <--
         */
        if (sFunc.find("data(") != string::npos || _data->containsCacheElements(sFunc))
        {
            string sFuncTemp = sFunc;
            string sToken = "";

            while (sFuncTemp.length())
            {
                sToken = getNextArgument(sFuncTemp, true);
                StripSpaces(sToken);
                if (_option->getbDebug())
                    cerr << "|-> DEBUG: sToken = " << sToken << endl;
                if (sToken.find("data(") != string::npos || sToken.find("cache(") != string::npos)
                {
                    if (sToken.find("data(") != string::npos && sToken.find("data("))
                        throw DATAPOINTS_CANNOT_BE_MODIFIED_WHILE_PLOTTING;
                    if (sToken.find("cache(") != string::npos && sToken.find("cache("))
                        throw DATAPOINTS_CANNOT_BE_MODIFIED_WHILE_PLOTTING;
                    string sSubstr = sToken.substr(getMatchingParenthesis(sToken.substr(sToken.find('(')))+sToken.find('(')+1);
                    if (_option->getbDebug())
                        cerr << "|-> DEBUG: sSubstr = " << sSubstr << endl;
                    if (sSubstr[sSubstr.find_first_not_of(' ')] != '"')
                        throw DATAPOINTS_CANNOT_BE_MODIFIED_WHILE_PLOTTING;
                }
            }

            // --> Positions-Integer, um sFunc in Funktionen und Datenpunkt-Angaben zu trennen <--
            unsigned int nPos = 0;
            unsigned int nPos_1 = 0;

            // --> Trennen der Funktionen und Datenpunkt-Angaben <--
            while (sFunc.find(',', nPos) != string::npos)
            {
                nPos = sFunc.find(',', nPos);
                if ((sFunc.substr(0,nPos).find("data(") == string::npos)
                    && (sFunc.substr(0,nPos).find("cache(") == string::npos))
                {
                    // --> Hier gibt's weder "data(" noch "cache(" ==> Naechster Schleifendurchlauf! <--
                    if (_option->getbDebug())
                        cerr << "|-> DEBUG: Continue!" << endl;
                    nPos++;
                    continue;
                }
                else if (sFunc.substr(0,nPos).find("data(") != string::npos)
                {
                    // --> Position der Klammer von "data(" speichern <--
                    nPos_1 = sFunc.substr(0,nPos).find("data(") + 4;
                    // --> Passende, schliessende Klammer finden <--
                    nPos_1 = getMatchingParenthesis(sFunc.substr(nPos_1)) + nPos_1 + 1;
                    // --> Legenden-Strings beachten <--
                    nPos_1 = sFunc.find('"', nPos_1+1);
                    // --> Das letzte '"' vor dem "," finden <--
                    while (sFunc.find('"', nPos_1+1) != string::npos && sFunc.find(',', nPos_1+1) != string::npos)
                    {
                        if (isInQuotes(sFunc, sFunc.find(',', nPos_1+1)))
                            nPos_1 = sFunc.find('"', nPos_1+1);
                        else
                        {
                            nPos_1 = sFunc.find(',', nPos_1+1)-1;
                            break;
                        }
                    }
                    // --> Dummer -1-Wert abfangen <--
                    if (nPos_1 == string::npos || sFunc.find(',', nPos_1+1) == string::npos)
                        nPos_1 = sFunc.length();
                    else
                        nPos_1++;

                    // --> String sFunc am vorherigen Komma und am Komma nach dem letzten zugehoerigen "," trennen <--
                    if (sFunc.rfind(',', sFunc.substr(0,nPos).find("data(")) > sFunc.rfind('"', sFunc.substr(0,nPos).find("data(")))
                        sDataPlots += ";" + sFunc.substr(sFunc.rfind(',', (sFunc.substr(0,nPos).find("data(")))+1, nPos_1-sFunc.rfind(',', (sFunc.substr(0,nPos).find("data(")))-1);
                    else
                        sDataPlots += ";" + sFunc.substr(sFunc.substr(0,nPos).find("data("), nPos_1-sFunc.substr(0,nPos).find("data("));
                }
                else if (sFunc.substr(0,nPos).find("cache(") != string::npos)
                {
                    // --> Wie im vorherigen Fall, allerdings fuer "cache(" <--
                    nPos_1 = sFunc.substr(0,nPos).find("cache(") + 5;
                    nPos_1 = getMatchingParenthesis(sFunc.substr(nPos_1)) + nPos_1 + 1;
                    while (sFunc.find('"', nPos_1+1) != string::npos && sFunc.find(',', nPos_1+1) != string::npos)
                    {
                        if (isInQuotes(sFunc, sFunc.find(',', nPos_1+1)))
                            nPos_1 = sFunc.find('"', nPos_1+1);
                        else
                        {
                            nPos_1 = sFunc.find(',', nPos_1+1)-1;
                            break;
                        }
                    }
                    if (nPos_1 == string::npos || sFunc.find(',', nPos_1+1) == string::npos)
                        nPos_1 = sFunc.length();
                    else
                        nPos_1++;
                    if (sFunc.rfind(',', sFunc.substr(0,nPos).find("cache(")) > sFunc.rfind('"', sFunc.substr(0,nPos).find("cache(")))
                        sDataPlots += ";" + sFunc.substr(sFunc.rfind(',', (sFunc.substr(0,nPos).find("cache(")))+1, nPos_1-sFunc.rfind(',', (sFunc.substr(0,nPos).find("cache(")))-1);
                    else
                        sDataPlots += ";" + sFunc.substr(sFunc.substr(0,nPos).find("cache("), nPos_1-sFunc.substr(0,nPos).find("cache("));
                }
                else
                    continue;

                // --> Restliche Funktionen zusammenschneiden: Teil vor dem gefundenen String + Teil nach dem gefundenen String <--
                sFunc = sFunc.substr(0, sFunc.find(sDataPlots.substr(sDataPlots.rfind(';')+1))) + sFunc.substr(sFunc.find(sDataPlots.substr(sDataPlots.rfind(';')+1)) + sDataPlots.substr(sDataPlots.rfind(';')+1).length());
                // --> Ueberzaehlige Kommata entfernen (etwa so wie ',,') <--
                removeArgSep(sFunc);
                // --> Nach dem Rausschneiden der Substrings, muessen wir die Index-Variable um die Laenge des Substrings zuruecksetzen <--
                nPos = nPos_1 - sDataPlots.substr(sDataPlots.rfind(';')+1).length();

            }

            // --> Ggf. wiederholen des Schleifenausdrucks, da der letzte Ausdruck moeglicherweise nicht identifiziert wird <--
            if ((sFunc.find("data(", nPos) != string::npos)
                || (sFunc.find("cache(", nPos) != string::npos))
            {
                if (sFunc.find("data(", nPos) != string::npos)
                {
                    nPos_1 = sFunc.find("data(", nPos) + 4;
                    nPos_1 = getMatchingParenthesis(sFunc.substr(nPos_1)) + nPos_1;
                    nPos_1 = sFunc.find('"', nPos_1+1);
                    while (sFunc.find('"', nPos_1+1) != string::npos && sFunc.find(',', nPos_1+1) != string::npos)
                    {
                        if (isInQuotes(sFunc, sFunc.find(',', nPos_1+1)))
                            nPos_1 = sFunc.find('"', nPos_1+1);
                        else
                        {
                            nPos_1 = sFunc.find(',', nPos_1+1)-1;
                            break;
                        }
                    }
                    if (nPos_1 == string::npos || sFunc.find(',', nPos_1+1) == string::npos)
                        nPos_1 = sFunc.length();
                    else
                        nPos_1++;

                    if (sFunc.rfind(',', sFunc.find("data(")) > sFunc.rfind('"', sFunc.find("data(")))
                        sDataPlots += ";" + sFunc.substr(sFunc.rfind(',', (sFunc.find("data(")))+1, nPos_1-sFunc.rfind(',', (sFunc.find("data(")))-1);
                    else
                        sDataPlots += ";" + sFunc.substr(sFunc.find("data("), nPos_1-sFunc.find("data("));
                }
                else
                {
                    nPos_1 = sFunc.find("cache(", nPos) + 5;
                    nPos_1 = getMatchingParenthesis(sFunc.substr(nPos_1)) + nPos_1;
                    while (sFunc.find('"', nPos_1+1) != string::npos && sFunc.find(',', nPos_1+1) != string::npos)
                    {
                        if (isInQuotes(sFunc, sFunc.find(',', nPos_1+1)))
                            nPos_1 = sFunc.find('"', nPos_1+1);
                        else
                        {
                            nPos_1 = sFunc.find(',', nPos_1+1)-1;
                            break;
                        }
                    }
                    if (nPos_1 == string::npos || sFunc.find(',', nPos_1+1) == string::npos)
                        nPos_1 = sFunc.length();
                    else
                        nPos_1++;
                    if (sFunc.rfind(',', sFunc.find("cache(")) > sFunc.rfind('"', sFunc.find("cache(")))
                        sDataPlots += ";" + sFunc.substr(sFunc.rfind(',', (sFunc.find("cache(")))+1, nPos_1-sFunc.rfind(',', (sFunc.find("cache(")))-1);
                    else
                        sDataPlots += ";" + sFunc.substr(sFunc.find("cache("), nPos_1-sFunc.find("cache("));
                }
                sFunc = sFunc.substr(0, sFunc.find(sDataPlots.substr(sDataPlots.rfind(';')+1))) + sFunc.substr(sFunc.find(sDataPlots.substr(sDataPlots.rfind(';')+1)) + sDataPlots.substr(sDataPlots.rfind(';')+1).length());
            }

            /* --> In dem string sFunc sind nun nur noch "gewoehnliche" Funktionen, die der Parser auswerten kann,
             *     in dem string sDataPlots sollten alle Teile sein, die "data(" oder "cache(" enthalten <--
             */
            if (_option->getbDebug())
            {
                cerr << "|-> DEBUG: sFunc = " << sFunc << endl;
                cerr << "|-> DEBUG: sDataPlots = " << sDataPlots << endl;
            }

            // --> Zaehlen wir die Datenplots. Durch den char ';' ist das recht simpel <--
            for (unsigned int i = 0; i < sDataPlots.length(); i++)
            {
                if (sDataPlots[i] == ';')
                    nDataPlots++;
            }

            if (_option->getbDebug())
                cerr << "|-> DEBUG: nDataPlots = " << nDataPlots << endl;

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
                    || sDataLabels.substr(n_dpos, sDataLabels.find(';', n_dpos)-n_dpos).find("cache(") != string::npos)
                        && sDataLabels.substr(n_dpos, sDataLabels.find(';', n_dpos)-n_dpos).find(',') != string::npos
                        && sDataLabels.substr(n_dpos, sDataLabels.find(';', n_dpos)-n_dpos).find(')') != string::npos)
                {
                    if ((sDataLabels.substr(n_dpos, sDataLabels.find(';', n_dpos)-n_dpos).find("data(") != string::npos && !_data->isValid())
                        || (sDataLabels.substr(n_dpos, sDataLabels.find(';', n_dpos)-n_dpos).find("cache(") != string::npos && !_data->isValidCache()))
                    {
                        //cerr << toSystemCodePage("|-> FEHLER: Keine Daten verfügbar!") << endl;
                        throw NO_DATA_AVAILABLE;
                    }
                    string sTemp = sDataLabels.substr(n_dpos, sDataLabels.find(';', n_dpos)-n_dpos);
                    StripSpaces(sTemp);
                    string sArgs = sTemp.substr(sTemp.find('('), sTemp.find(')')+1-sTemp.find('('));
                    string sArg_1 = "<<empty>>";
                    string sArg_2 = "<<empty>>";
                    string sArg_3 = "<<empty>>";
                    parser_SplitArgs(sArgs, sArg_1, ',', *_option, false);
                    if (sArg_1.find(':') != string::npos)
                    {
                        sArg_1 = "(" + sArg_1 + ")";
                        parser_SplitArgs(sArg_1, sArg_2, ':', *_option, false);
                        StripSpaces(sArg_2);
                        if (sArg_2.find(':') != string::npos)
                        {
                            sArg_2 = "(" + sArg_2 + ")";
                            parser_SplitArgs(sArg_2, sArg_3, ':', *_option, false);
                            StripSpaces(sArg_2);
                            if (sArg_3.find(':') != string::npos)
                            {
                                sArg_3 = "(" + sArg_3 + ")";
                                parser_SplitArgs(sArg_3, sArgs, ':', *_option, false);
                                StripSpaces(sArg_3);
                            }
                        }
                    }
                    if (_option->getbDebug())
                        cerr << "|-> DEBUG: sArg_1 = " << sArg_1 << ", sArg_2 = " << sArg_2 << ", sArg_3 = " << sArg_3 << ", sArgs = " << sArgs << endl;
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
                    if (sTemp.find("cache(") != string::npos)
                    {
                        _data->setCacheStatus(true);
                    }
                    if (sArg_2 == "<<empty>>" && sArg_3 == "<<empty>>" && sCommand != "plot3d")
                    {
                        _parser->SetExpr(sArg_1);
                        sTemp = "\"" + _data->getHeadLineElement((int)_parser->Eval()-1) + "\"";
                    }
                    else if (sArg_2.length())
                    {
                        if (sCommand != "plot3d")
                        {
                            if (!(_pData->getyError() || _pData->getxError()) && sArg_3 == "<<empty>>")
                            {
                                _parser->SetExpr(sArg_2);
                                sTemp = "\"" + _data->getHeadLineElement((int)_parser->Eval()-1) + " vs. ";
                                _parser->SetExpr(sArg_1);
                                sTemp += _data->getHeadLineElement((int)_parser->Eval()-1) + "\"";
                            }
                            else if (sArg_3 != "<<empty>>")
                            {
                                _parser->SetExpr(sArg_2);
                                sTemp = "\"" + _data->getHeadLineElement((int)_parser->Eval()-1) + " vs. ";
                                _parser->SetExpr(sArg_1);
                                sTemp += _data->getHeadLineElement((int)_parser->Eval()-1) + "\"";
                            }
                            else
                            {
                                double dArg_1, dArg_2;
                                _parser->SetExpr(sArg_1);
                                dArg_1 = _parser->Eval();
                                _parser->SetExpr(sArg_2);
                                dArg_2 = _parser->Eval();
                                if (dArg_1 < dArg_2)
                                    sTemp = "\"" + _data->getHeadLineElement((int)dArg_1) + " vs. " + _data->getHeadLineElement((int)dArg_1-1) + "\"";
                                else
                                    sTemp = "\"" + _data->getHeadLineElement((int)dArg_2-1) + " vs. " + _data->getHeadLineElement((int)dArg_2) + "\"";
                            }
                        }
                        else if (sArg_3 == "<<empty>>" || !sArg_3.length())
                        {
                            double dArg_1, dArg_2;
                            _parser->SetExpr(sArg_1);
                            dArg_1 = _parser->Eval();
                            _parser->SetExpr(sArg_2);
                            dArg_2 = _parser->Eval();
                            if (dArg_1 < dArg_2)
                                sTemp = "\"" + _data->getHeadLineElement((int)dArg_1-1) + ", " + _data->getHeadLineElement((int)dArg_2-2) + ", " + _data->getHeadLineElement((int)dArg_2-1) + "\"";
                            else
                                sTemp = "\"" + _data->getHeadLineElement((int)dArg_2-1) + ", " + _data->getHeadLineElement((int)dArg_1-2) + ", " + _data->getHeadLineElement((int)dArg_1-1) + "\"";
                        }
                        else if (sArg_3.length())
                        {
                            _parser->SetExpr(sArg_1);
                            sTemp = "\"" + _data->getHeadLineElement((int)_parser->Eval()-1) + ", ";
                            _parser->SetExpr(sArg_2);
                            sTemp += _data->getHeadLineElement((int)_parser->Eval()-1) + ", ";
                            _parser->SetExpr(sArg_3);
                            sTemp += _data->getHeadLineElement((int)_parser->Eval()-1) + "\"";
                        }
                    }
                    else if (!sArg_2.length())
                    {
                        if (sCommand != "plot3d")
                        {
                            _parser->SetExpr(sArg_1);
                            sTemp = "\"" + _data->getHeadLineElement((int)_parser->Eval()) + " vs. " + _data->getHeadLineElement((int)_parser->Eval()-1) + "\"";
                        }
                        else if (sArg_3 == "<<empty>>" || !sArg_3.length())
                        {
                            _parser->SetExpr(sArg_1);
                            sTemp = "\"" + _data->getHeadLineElement((int)_parser->Eval()-1) + ", "
                                + _data->getHeadLineElement((int)_parser->Eval()) + ", "
                                + _data->getHeadLineElement((int)_parser->Eval()+1) + "\"";
                        }
                        else if (sArg_3.length())
                        {
                            _parser->SetExpr(sArg_1);
                            sTemp = "\"" + _data->getHeadLineElement((int)_parser->Eval()-1) + ", " + _data->getHeadLineElement((int)_parser->Eval()) + ", ";
                            _parser->SetExpr(sArg_3);
                            sTemp += _data->getHeadLineElement((int)_parser->Eval()-1) + "\"";
                        }
                    }
                    else
                    {
                        n_dpos = sDataLabels.find(';', n_dpos)+1;
                        continue;
                    }
                    _data->setCacheStatus(false);
                    for (unsigned int n = 0; n < sTemp.length(); n++)
                    {
                        if (sTemp[n] == '_')
                            sTemp[n] = ' ';
                    }
                    sDataLabels = sDataLabels.substr(0,n_dpos) + sTemp + sDataLabels.substr(sDataLabels.find(';', n_dpos));

                }
                n_dpos = sDataLabels.find(';', n_dpos)+1;
            }

            if (_option->getbDebug())
            {
                cerr << "|-> DEBUG: sDataPlots = " << sDataPlots << endl;
                cerr << "|-> DEBUG: sDataLabels = " << sDataLabels << endl;
            }

            string si_pos[2] = {"", ""};                    // String-Array fuer die Zeilen-Position: muss fuer alle Spalten identisch sein!
            string sj_pos[6] = {"", "", "", "", "", ""};    // String-Array fuer die Spalten: kann bis zu sechs beliebige Werte haben
            int i_pos[2] = {0, 0};                          // Int-Array fuer den Wert der Zeilen-Positionen
            int j_pos[6] = {0, 0, 0, 0, 0, 0};              // Int-Array fuer den Wert der Spalten-Positionen
            int nMatch = 0;                                 // Int fuer die Position des aktuellen find-Treffers eines Daten-Objekts

            // --> Bestimmen wir die Zeilen- und Spalten-Koordinaten des aktuellen Daten-Objekts <--
            for (int i = 0; i < nDataPlots; i++)
            {
                // --> Suchen wir den aktuellen und den darauf folgenden ';' <--
                nPos = sDataPlots.find(';', nPos)+1;
                nPos_1 = sDataPlots.find(';', nPos);

                // --> Ist da "cache" drin? Aktivieren wir den Cache-Status <--
                if (sDataPlots.substr(nPos, nPos_1-nPos).find("cache(") != string::npos)
                    _data->setCacheStatus(true);

                // --> Klammer und schliessende Klammer finden und in einen anderen String schreiben <--
                nMatch = sDataPlots.find('(', nPos);
                si_pos[0] = sDataPlots.substr(nMatch, getMatchingParenthesis(sDataPlots.substr(nMatch))+1);

                if (_option->getbDebug())
                    cerr << "|-> DEBUG: si_pos[0] = " << si_pos[0] << endl;

                // --> Rausgeschnittenen String am Komma ',' in zwei Teile teilen <--
                try
                {
                    parser_SplitArgs(si_pos[0], sj_pos[0], ',', *_option, false);
                }
                catch (...)
                {
                    delete[] _mDataPlots;
                    delete[] nDataDim;
                    throw;
                }
                if (_option->getbDebug())
                    cerr << "|-> DEBUG: si_pos[0] = " << si_pos[0] << ", sj_pos[0] = " << sj_pos[0] << endl;

                // --> Gibt's einen Doppelpunkt? Dann teilen wir daran auch noch mal <--
                if (si_pos[0].find(':') != string::npos)
                {
                    si_pos[0] = "( " + si_pos[0] + " )";
                    try
                    {
                        parser_SplitArgs(si_pos[0], si_pos[1], ':', *_option, false);
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

                if (_option->getbDebug())
                {
                    cerr << "|-> DEBUG: si_pos[0] = " << si_pos[0] << ", si_pos[1] = " << si_pos[1] << endl;
                }

                // --> Auswerten mit dem Parser <--
                if (parser_ExprNotEmpty(si_pos[0]))
                {
                    _parser->SetExpr(si_pos[0]);
                    i_pos[0] = (int)_parser->Eval() - 1;
                }
                else
                    i_pos[0] = 0;
                if (si_pos[1] == "inf")
                {
                    i_pos[1] = _data->getLines(true);
                }
                else if (parser_ExprNotEmpty(si_pos[1]))
                {
                    _parser->SetExpr(si_pos[1]);
                    i_pos[1] = (int)_parser->Eval();
                }
                else
                    i_pos[1] = i_pos[0]+1;
                // --> Pruefen, ob die Reihenfolge der Indices sinnvoll ist <--
                parser_CheckIndices(i_pos[0], i_pos[1]);

                if (_option->getbDebug())
                    cerr << "|-> DEBUG: i_pos[0] = " << i_pos[0] << ", i_pos[1] = " << i_pos[1] << endl;

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
                        parser_SplitArgs(sj_pos[j], sj_pos[j+1], ':', *_option, false);
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
                        j_pos[k] = _data->getCols();
                        break;
                    }
                    else if (parser_ExprNotEmpty(sj_pos[k]))
                    {
                        // --> Hat einen Wert: Kann man auch auswerten <--
                        _parser->SetExpr(sj_pos[k]);
                        j_pos[k] = (int)_parser->Eval() - 1;
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
                if (_option->getbDebug())
                    cerr << "|-> DEBUG: j_pos[0] = " << j_pos[0] << ", j_pos[1] = " << j_pos[1] << endl;

                // --> Jetzt wissen wir die Spalten: Suchen wir im Falle von si_pos[1] == inf nach der laengsten <--
                if (si_pos[1] == "inf")
                {
                    int nAppendedZeroes = _data->getAppendedZeroes(j_pos[0]);
                    for (int k = 1; k <= j; k++)
                    {
                        if (nAppendedZeroes > _data->getAppendedZeroes(j_pos[k]))
                            nAppendedZeroes = _data->getAppendedZeroes(j_pos[k]);
                    }
                    if (nAppendedZeroes < i_pos[1])
                        i_pos[1] -= nAppendedZeroes;
                    if (_option->getbDebug())
                        cerr << "|-> DEBUG: i_pos[1] = " << i_pos[1] << endl;
                }


                /* --> Bestimmen wir die "Dimension" des zu plottenden Datensatzes. Dabei ist es auch
                 *     von Bedeutung, ob Fehlerbalken anzuzeigen sind <--
                 */
                nDataDim[i] = 0;
                if (j == 0 && !_pData->getxError() && !_pData->getyError())
                    nDataDim[i] = 2;
                else if (j == 0 && _pData->getxError() && _pData->getyError())
                    nDataDim[i] = 4;
                else if (j == 0 && (_pData->getxError() || _pData->getyError()))
                    nDataDim[i] = 3;
                else if (j == 1)
                {
                    if (sCommand == "plot" && !_pData->getxError() && !_pData->getyError())
                        nDataDim[i] = 2;
                    else if (sCommand == "plot" && _pData->getxError() && _pData->getyError())
                        nDataDim[i] = 4;
                    else if (sCommand == "plot" && (_pData->getxError() || _pData->getyError()))
                        nDataDim[i] = 3;
                    else if (sCommand == "plot3d" && !_pData->getxError() && !_pData->getyError())
                        nDataDim[i] = 3;
                    else if (sCommand == "plot3d" && _pData->getxError() && _pData->getyError())
                        nDataDim[i] = 6;
                    else if (b2D)
                        nDataDim[i] = 3;
                }
                else
                {
                    nDataDim[i] = j+1;
                }

                /* --> Mit der bestimmten Dimension koennen wir einen passenden Satz neuer mglData-Objekte
                 *     im _mDataPlots-Array initialisieren <--
                 */
                _mDataPlots[i] = new mglData[nDataDim[i]];
                if (_option->getbDebug())
                    cerr << "|-> DEBUG: mglData-Element created! nDataDim[i] = " << nDataDim[i] << endl;

                // --> Datenspeicher der mglData-Objekte vorbereiten <--
                for (int q = 0; q < nDataDim[i]; q++)
                {
                    if (b2D && q == 2)///TEMPORARY
                        _mDataPlots[i][q].Create(i_pos[1]-i_pos[0],i_pos[1]-i_pos[0]);
                    else
                        _mDataPlots[i][q].Create(i_pos[1]-i_pos[0]);
                }

                // --> Datenspeicher der mglData-Objekte mit den noetigen Daten fuellen <--
                for (int l = 0; l < i_pos[1]-i_pos[0]; l++)
                {
                    if (j == 0 && _data->getCols() > j_pos[0])
                    {
                        // --> Nur y-Datenwerte! <--
                        _mDataPlots[i][0].a[l] = l+1+i_pos[0];
                        if (_data->isValidEntry(l+i_pos[0], j_pos[0]))
                        {
                            if (!l && (isinf(_data->getElement(l+i_pos[0], j_pos[0])) || isnan(_data->getElement(l+i_pos[0], j_pos[0]))))
                                _mDataPlots[i][1].a[l] = NAN;
                            else if (isinf(_data->getElement(l+i_pos[0], j_pos[0])))
                                _mDataPlots[i][1].a[l] = NAN;
                            else
                                _mDataPlots[i][1].a[l] = _data->getElement(l+i_pos[0], j_pos[0]);
                        }
                        else if (l)
                            _mDataPlots[i][1].a[l] = NAN;
                        else
                            _mDataPlots[i][1].a[0] = NAN;
                        // --> Pseudo-0-Fehler fuer den Fall, dass Errorplotting aktiviert, aber keine Error-Daten vorhanden sind <--
                        if (_pData->getxError())
                            _mDataPlots[i][2].a[l] = 0.0;
                        if (_pData->getxError() && _pData->getyError())
                            _mDataPlots[i][3].a[l] = 0.0;
                        else if (_pData->getyError())
                            _mDataPlots[i][2].a[l] = 0.0;
                    }
                    else if (_data->getCols() <= j_pos[0])
                    {
                        // --> Dumme Spalten-Fehler abfangen! <--
                        for (int i = 0; i < nDataPlots; i++)
                            delete[] _mDataPlots[i];
                        delete[] _mDataPlots;
                        _mDataPlots = 0;
                        delete[] nDataDim;
                        nDataDim = 0;
                        _data->setCacheStatus(false);
                        throw TOO_FEW_COLS;
                    }
                    else if (nDataDim[i] == 2 && j == 1)
                    {
                        if ((_data->getCols() > j_pos[0]+1 && (j_pos[0] < j_pos[1] || sj_pos[1] == "inf"))
                            || (_data->getCols() > j_pos[1]+1 && j_pos[0] > j_pos[1]))
                        {
                            // --> xy-Datenwerte! <--
                            if (_data->isValidEntry(l+i_pos[0], j_pos[0]))
                            {
                                if (!l && (isnan(_data->getElement(l+i_pos[0], j_pos[0])) || isinf(_data->getElement(l+i_pos[0], j_pos[0]))))
                                    _mDataPlots[i][0].a[l] = NAN;
                                else if (isinf(_data->getElement(l+i_pos[0], j_pos[0])))
                                    _mDataPlots[i][0].a[l] = NAN;
                                else
                                    _mDataPlots[i][0].a[l] = _data->getElement(l+i_pos[0], j_pos[0]);
                            }
                            else if (l)
                                _mDataPlots[i][0].a[l] = NAN;
                            else
                                _mDataPlots[i][0].a[0] = NAN;
                            if (_data->isValidEntry(l+i_pos[0], j_pos[1]) && sj_pos[1] != "inf")
                            {
                                if (!l && (isinf(_data->getElement(l+i_pos[0], j_pos[1])) || isnan(_data->getElement(l+i_pos[0], j_pos[1]))))
                                    _mDataPlots[i][1].a[l] = NAN;
                                else if (isinf(_data->getElement(l+i_pos[0], j_pos[1])))
                                    _mDataPlots[i][1].a[l] = NAN;
                                else
                                    _mDataPlots[i][1].a[l] = _data->getElement(l+i_pos[0], j_pos[1]);
                            }
                            else if (_data->isValidEntry(l+i_pos[0], j_pos[0]+1) && sj_pos[1] == "inf")
                            {
                                if (!l && (isinf(_data->getElement(l+i_pos[0], j_pos[0]+1)) || isnan(_data->getElement(l+i_pos[0], j_pos[0]+1))))
                                    _mDataPlots[i][1].a[l] = NAN;
                                else if (isinf(_data->getElement(l+i_pos[0], j_pos[0]+1)))
                                    _mDataPlots[i][1].a[l] = NAN;
                                else
                                    _mDataPlots[i][1].a[l] = _data->getElement(l+i_pos[0], j_pos[0]+1);
                            }
                            else if (l)
                                _mDataPlots[i][1].a[l] = NAN;
                            else
                                _mDataPlots[i][1].a[0] = NAN;
                        }
                        else if ((_data->getCols() > j_pos[0] && (j_pos[0] < j_pos[1] || sj_pos[1] == "inf"))
                            || (_data->getCols() > j_pos[1] && j_pos[0] > j_pos[1]))
                        {
                            // --> Reinterpretation der angegeben Spalten, falls eine Spalte zu wenig angegeben wurde <--
                            _mDataPlots[i][0].a[l] = l+1+i_pos[0];
                            if (_data->isValidEntry(l+i_pos[0], j_pos[0]) && (j_pos[0] < j_pos[1] || sj_pos[1] == "inf"))
                            {
                                if (!l && (isnan(_data->getElement(l+i_pos[0], j_pos[0])) || isinf(_data->getElement(l+i_pos[0], j_pos[0]))))
                                    _mDataPlots[i][1].a[l] = NAN;
                                else if (isinf(_data->getElement(l+i_pos[0], j_pos[0])))
                                    _mDataPlots[i][1].a[l] = NAN;
                                else
                                    _mDataPlots[i][1].a[l] = _data->getElement(l+i_pos[0], j_pos[0]);
                            }
                            else if (_data->isValidEntry(l+i_pos[0], j_pos[1]) && (j_pos[0] > j_pos[1]))
                            {
                                if (!l && (isnan(_data->getElement(l+i_pos[0], j_pos[1])) || isinf(_data->getElement(l+i_pos[0], j_pos[1]))))
                                    _mDataPlots[i][1].a[l] = NAN;
                                else if (isinf(_data->getElement(l+i_pos[0], j_pos[1])))
                                    _mDataPlots[i][1].a[l] = NAN;
                                else
                                    _mDataPlots[i][1].a[l] = _data->getElement(l+i_pos[0], j_pos[1]);
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
                            _data->setCacheStatus(false);
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
                                if (b2D && q == 2)
                                {
                                    for (int k = 0; k < i_pos[1]-i_pos[0]; k++)
                                    {
                                        if (_data->num(i_pos[0], i_pos[1], j_pos[0]+1) <= k)
                                        {
                                            _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = NAN;
                                            continue;
                                        }
                                        if (_data->getCols() > q + k + j_pos[0] && _data->isValidEntry(l+i_pos[0], q+k+j_pos[0]) && (j_pos[0] <= j_pos[1] || sj_pos[1] == "inf"))
                                        {
                                            if (!l && (isnan(_data->getElement(l+i_pos[0], q + k + j_pos[0])) || isinf(_data->getElement(l+i_pos[0], q + k + j_pos[0]))))
                                                _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = NAN;
                                            else if (isinf(_data->getElement(l+i_pos[0], q+k + j_pos[0])))
                                                _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = NAN;
                                            else
                                                _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = _data->getElement(l+i_pos[0], q+k + j_pos[0]);
                                        }
                                        else
                                            _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = NAN;
                                    }
                                }
                                else
                                {
                                    // --> Vorwaerts zaehlen <--
                                    if (_data->getCols() > q + j_pos[0] && _data->isValidEntry(l+i_pos[0], q+j_pos[0]) && (j_pos[0] <= j_pos[1] || sj_pos[1] == "inf"))
                                    {
                                        if (!l && (isnan(_data->getElement(l+i_pos[0], q + j_pos[0])) || isinf(_data->getElement(l+i_pos[0], q + j_pos[0]))))
                                            _mDataPlots[i][q].a[l] = NAN;
                                        else if (isinf(_data->getElement(l+i_pos[0], q + j_pos[0])))
                                            _mDataPlots[i][q].a[l] = NAN;
                                        else
                                            _mDataPlots[i][q].a[l] = _data->getElement(l+i_pos[0], q + j_pos[0]);
                                    }
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
                                if (b2D && q == 2)
                                {
                                    for (int k = 0; k < i_pos[1]-i_pos[0]; k++)
                                    {
                                        if (_data->num(i_pos[0], i_pos[1], j_pos[0]-1) <= k)
                                        {
                                            _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = NAN;
                                            continue;
                                        }
                                        if (_data->getCols() > j_pos[0] && _data->isValidEntry(l+i_pos[0], j_pos[0]-q-k) && (j_pos[0]-q-k >= j_pos[1] && j_pos[0]-q-k >= 0))
                                        {
                                            if (!l && (isnan(_data->getElement(l+i_pos[0], j_pos[0]-q-k)) || isinf(_data->getElement(l+i_pos[0], j_pos[0]-q-k))))
                                                _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = NAN;
                                            else if (isinf(_data->getElement(l+i_pos[0], q+k + j_pos[0])))
                                                _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = NAN;
                                            else
                                                _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = _data->getElement(l+i_pos[0], j_pos[0]-q-k);
                                        }
                                        else
                                            _mDataPlots[i][q].a[l+(i_pos[1]-i_pos[0])*k] = NAN;
                                    }
                                }
                                else
                                {
                                    // --> Rueckwaerts zaehlen <--
                                    if (_data->getCols() > j_pos[0] && _data->isValidEntry(l+i_pos[0], j_pos[0]-q) && j_pos[0]-q >= 0 && j_pos[0]-q >= j_pos[1])
                                    {
                                        if (!l && (isnan(_data->getElement(l+i_pos[0], j_pos[0]-q)) || isinf(_data->getElement(l+i_pos[0], j_pos[0]-q))))
                                            _mDataPlots[i][q].a[l] = NAN;
                                        else if (isinf(_data->getElement(l+i_pos[0], j_pos[0]-q)))
                                            _mDataPlots[i][q].a[l] = NAN;
                                        else
                                            _mDataPlots[i][q].a[l] = _data->getElement(l+i_pos[0], j_pos[0]-q);
                                    }
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
                        for (int k = 0; k < nDataDim[i]; k++)
                        {
                            if (b2D && k == 2)
                            {
                                for (int m = 0; m < i_pos[1]-i_pos[0]; m++)
                                {
                                    if (_data->num(i_pos[0], i_pos[1], j_pos[k-1]) <= m)
                                    {
                                        _mDataPlots[i][k].a[l+(i_pos[1]-i_pos[0])*m] = NAN;
                                        continue;
                                    }
                                    if (_data->getCols() > j_pos[k]+m && _data->isValidEntry(l+i_pos[0], j_pos[k]+m))
                                    {
                                        if (!l && (isnan(_data->getElement(l+i_pos[0], j_pos[k]+m)) || isinf(_data->getElement(l+i_pos[0], j_pos[k]+m))))
                                            _mDataPlots[i][k].a[l+(i_pos[1]-i_pos[0])*m] = NAN;
                                        else if (isinf(_data->getElement(l+i_pos[0], j_pos[k]+m)))
                                            _mDataPlots[i][k].a[l+(i_pos[1]-i_pos[0])*m] = NAN;
                                        else
                                            _mDataPlots[i][k].a[l+(i_pos[1]-i_pos[0])*m] = _data->getElement(l+i_pos[0], j_pos[k]+m);
                                    }
                                }
                            }
                            else
                            {
                                if (_data->getCols() > j_pos[k] && _data->isValidEntry(l+i_pos[0], j_pos[k]))
                                {
                                    if (!l && (isnan(_data->getElement(l+i_pos[0], j_pos[k])) || isinf(_data->getElement(l+i_pos[0], j_pos[k]))))
                                        _mDataPlots[i][k].a[l] = NAN;
                                    else if (isinf(_data->getElement(l+i_pos[0], j_pos[k])))
                                        _mDataPlots[i][k].a[l] = NAN;
                                    else
                                        _mDataPlots[i][k].a[l] = _data->getElement(l+i_pos[0], j_pos[k]);
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
                            dDataRanges[q][0] = _mDataPlots[i][q].a[l];
                            dDataRanges[q][1] = _mDataPlots[i][q].a[l];
                        }
                        else
                        {
                            if (b2D && q == 2)
                            {
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
                                if (dDataRanges[q][0] > _mDataPlots[i][q].a[l] || isnan(dDataRanges[q][0]))
                                    dDataRanges[q][0] = _mDataPlots[i][q].a[l];
                                if (dDataRanges[q][1] < _mDataPlots[i][q].a[l] || isnan(dDataRanges[q][1]))
                                    dDataRanges[q][1] = _mDataPlots[i][q].a[l];
                            }
                        }
                    }
                }
                // --> Sicherheitshalber den Cache wieder deaktivieren <--
                _data->setCacheStatus(false);
            }
        }

        // --> Zuweisen der uebergebliebenen Funktionen an den "Legenden-String". Ebenfalls extrem fummelig <--
        if (sFunc.length() && !bDraw3D && !bDraw)
        {
            if (_option->getbDebug())
                cerr << "|-> DEBUG: Applying Legends ..." << endl;
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
                if (_option->getbDebug())
                    cerr << "|-> DEBUG: n_pos = " << n_pos << ", n_pos_2 = " << n_pos_2 << endl;
                while (sFunc.find(',', n_pos_2+1) != string::npos && isInQuotes(sFunc, n_pos_2))
                {
                    n_pos_2 = sFunc.find(',', n_pos_2+1);
                }
                if (n_pos_2 == string::npos || isInQuotes(sFunc, n_pos_2))
                    n_pos_2 = sFunc.length();
                sLabels += sFunc.substr(n_pos,n_pos_2-n_pos)+ ";";
                if (_option->getbDebug())
                    cerr << "|-> DEBUG: sLabels = " << sLabels << endl;
                sFunc = sFunc.substr(0,n_pos) + (n_pos_2 < sFunc.length() ? sFunc.substr(n_pos_2) : "");
                if (_option->getbDebug())
                    cerr << "|-> DEBUG: sFunc = " << sFunc << endl;
            }
            while (sFunc.find('"') != string::npos || sFunc.find('#') != string::npos);
            StripSpaces(sLabels);
        }
        if (_option->getbDebug())
        {
            cerr << "|-> DEBUG: sFunc = " << sFunc << endl;
            cerr << "|-> DEBUG: sLabels = " << sLabels << endl;
        }

        // --> Vektor-Konvertierung <--
        if (sFunc.find("{{") != string::npos)
            parser_VectorToExpr(sFunc, *_option);

        // --> "String"-Objekt? So was kann man einfach nicht plotten: Aufraeumen und zurueck! <--
        if ((containsStrings(sFunc) || _data->containsStringVars(sFunc)) && !(bDraw3D || bDraw))
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
            throw CANNOT_PLOT_STRINGS;
        }

        /* --> Wenn der Funktionen-String nicht leer ist, weise ihn dem Parser zu; anderenfalls verwende das
         *     eindeutige Token "<<empty>>", dass viel einfacher zu identfizieren ist <--
         */
        if (parser_ExprNotEmpty(sFunc) && !(bDraw3D || bDraw))
        {
            _parser->SetExpr(sFunc);
            _parser->Eval();
            nFunctions = _parser->GetNumResults();
            if ((_pData->getColorMask() || _pData->getAlphaMask()) && b2D && nFunctions % 2)
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
                throw NUMBER_OF_FUNCTIONS_NOT_MATCHING;
            }
        }
        else if (parser_ExprNotEmpty(sFunc) && (bDraw3D || bDraw))
        {
            string sArgument;
            sFunc += " ";
            while (sFunc.length())
            {
                sArgument = getNextArgument(sFunc, true);
                //cerr << sArgument << endl;
                StripSpaces(sArgument);
                if (!sArgument.length())
                    continue;
                vDrawVector.push_back(sArgument);
            }
            sFunc = "<<empty>>";
        }
        else
            sFunc = "<<empty>>";

        /* --> Die DatenPlots ueberschreiben ggf. die Ranges, allerdings haben vom Benutzer gegebene
         *     Ranges stets die hoechste Prioritaet (was eigentlich auch logisch ist) <--
         * --> Log-Plots duerfen in der gewaehlten Achse nicht <= 0 sein! <--
         */
        if (_option->getbDebug())
            cerr << endl;

        if (!nPlotCompose)
        {
            // --> Standard-Ranges zuweisen: wenn weniger als i+1 Ranges gegeben sind und Datenranges vorhanden sind, verwende die Datenranges <--
            for (int i = 0; i < 3; i++)
            {
                if (bDraw3D || bDraw)///Temporary
                {
                    dRanges[i][0] = _pData->getRanges(i);
                    dRanges[i][1] = _pData->getRanges(i,1);
                    continue;
                }
                if (_mDataPlots && (_pData->getGivenRanges() < i+1 || !_pData->getRangeSetting(i)))
                {
                    if ((isinf(dDataRanges[i][0]) || isnan(dDataRanges[i][0])) && (unsigned)i < nMaxPlotDim)
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
                        throw PLOTDATA_IS_NAN;
                    }
                    else if (!(isinf(dDataRanges[i][0]) || isnan(dDataRanges[i][0])))
                        dRanges[i][0] = dDataRanges[i][0];
                    else
                        dRanges[i][0] = -10.0;
                }
                else
                    dRanges[i][0] = _pData->getRanges(i);

                if (_mDataPlots && (_pData->getGivenRanges() < i+1 || !_pData->getRangeSetting(i)))
                {
                    if ((isinf(dDataRanges[i][1]) || isnan(dDataRanges[i][1])) && (unsigned)i < nMaxPlotDim)
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
                        throw PLOTDATA_IS_NAN;
                    }
                    else if (!(isinf(dDataRanges[i][1]) || isnan(dDataRanges[i][1])))
                        dRanges[i][1] = dDataRanges[i][1];
                    else
                        dRanges[i][1] = 10.0;
                }
                else
                    dRanges[i][1] = _pData->getRanges(i,1);

                if (_option->getbDebug())
                {
                    cerr << "|-> DEBUG: " << dRanges[i][0] << ":" << dRanges[i][1] << endl;
                }
            }

            // --> Spezialfall: Wenn nur eine Range gegeben ist, verwende im 3D-Fall diese fuer alle drei benoetigten Ranges <--
            if (_pData->getGivenRanges() == 1 && (b3D || b3DVect))
            {
                for (int i = 1; i < 3; i++)
                {
                    dRanges[i][0] = dRanges[0][0];
                    dRanges[i][1] = dRanges[0][1];
                }
            }
        }
        // --> Sonderkoordinatensaetze und dazu anzugleichende Ranges. Noch nicht korrekt implementiert <--
        if (!_pData->getCoords())
        {
            /* --> Im Falle logarithmischer Plots muessen die Darstellungsintervalle angepasst werden. Falls
             *     die Intervalle zu Teilen im Negativen liegen, versuchen wir trotzdem etwas sinnvolles
             *     daraus zu machen. <--
             */
            if (_pData->getxLogscale())
            {
                if (dRanges[0][0] <= 0 && dRanges[0][1] > 0)
                {
                    dRanges[0][0] = dRanges[0][1] / 1e3;
                }
                else if (dRanges[0][0] < 0 && dRanges[0][1] <= 0)
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
            }

            if (_pData->getyLogscale())
            {
                if (dRanges[1][0] <= 0 && dRanges[1][1] > 0)
                {
                    dRanges[1][0] = dRanges[1][1] / 1e3;
                }
                else if (dRanges[1][0] < 0 && dRanges[1][1] <= 0)
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
            }

            if (_pData->getzLogscale() && (b2D || sCommand == "plot3d" || b3D || b3DVect))
            {
                if (dRanges[2][0] <= 0 && dRanges[2][1] > 0)
                {
                    dRanges[2][0] = dRanges[2][1] / 1e3;
                }
                else if (dRanges[2][0] < 0 && dRanges[2][1] <= 0)
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
            }
        }
        else if (_pData->getCoords())
        {
            // --> Im Falle polarer oder sphaerischer Koordinaten muessen die Darstellungsintervalle angepasst werden <--
            if (_pData->getCoords() == 1)
            {
                if (sCommand.find("3d") == string::npos)
                {
                    dRanges[0][0] = 0.0;
                    dRanges[0][1] = 1.99999999;
                }
                else
                {
                    dRanges[0][0] = 0.0;
                    dRanges[1][0] = 0.0;
                    dRanges[1][1] = 1.99999999;
                }
                if (!(b2D || sCommand == "plot3d" || b3D || b3DVect || b2DVect))
                    dRanges[1][0] = 0.0;
            }
            else if (_pData->getCoords() == 2)
            {
                if (sCommand.find("3d") == string::npos)
                {
                    dRanges[0][0] = 0.0;
                    dRanges[0][1] = 1.99999999;
                }
                else
                {
                    dRanges[1][0] = 0.0;
                    dRanges[1][1] = 1.99999999;
                }
                if (!(b2D || sCommand == "plot3d" || b3D || b3DVect || b2DVect))
                    dRanges[1][0] = 0.0;
                else if (sCommand.find("3d") != string::npos)
                {
                    dRanges[0][0] = 0.0;
                    dRanges[2][0] = 0.0;
                    dRanges[2][1] = 0.99999999;
                }
                else
                {
                    dRanges[1][0] = 0.0;
                    dRanges[1][1] = 0.99999999;
                    dRanges[2][0] = 0.0;
                }
            }
        }

        // --> Plotvariablen auf den Anfangswert setzen <--
        if (_option->getbDebug())
            cerr << "|" << endl;
        parser_iVars.vValue[0][0] = dRanges[0][0];  // Plotvariable: x
        if (b2D || b2DVect)
        {
            parser_iVars.vValue[1][0] = dRanges[1][0];  // Plotvariable: y
        }
        if (b3D || b3DVect)
        {
            parser_iVars.vValue[1][0] = dRanges[1][0];  // Plotvariable: y
            parser_iVars.vValue[2][0] = dRanges[2][0];  // Plotvariable: z
        }
        if (sCommand == "plot3d" || _pData->getAnimateSamples())
        {
            parser_iVars.vValue[3][0] = _pData->gettBoundary();  // Plotparameter: t
        }

        // --> Plot-Speicher vorbereiten <--
        if (!b2D && sFunc != "<<empty>>" && sCommand != "plot3d" && !b3D && !b3DVect && !b2DVect)
            _pData->setDim(nSamples, nFunctions);    // nSamples * nFunctions (Standardplot)
        else if (sFunc != "<<empty>>" && sCommand == "plot3d")
            nFunctions % 3 ? _pData->setDim(nSamples, 3, nFunctions/3+1) : _pData->setDim(nSamples, 3, nFunctions/3);     // nSamples * 3 * nSamples/3 (3D-Trajektorie)
        else if (sFunc != "<<empty>>" && b3D)
            _pData->setDim(nSamples, nSamples, nSamples);    // nSamples * nSamples * nSamples (3D-Plot)
        else if (sFunc != "<<empty>>" && b3DVect)
            _pData->setDim(nSamples, nSamples, 3*nSamples);  // nSamples * nSamples * 3*nSamples (3D-Vektorplot)
        else if (sFunc != "<<empty>>" && b2DVect)
            _pData->setDim(nSamples, nSamples, 2*nSamples);  // nSamples * nSamples * 2*nSamples (2D-Vektorplot)
        else if (sFunc != "<<empty>>")
            _pData->setDim(nSamples, nSamples, nFunctions);  // nSamples * nSamples * nFunctions (2D-Plot)

        if (_option->getbDebug())
            cerr << "|-> Plot-Cache finished!" << endl;

        if (_pData->getAnimateSamples())
            _graph->StartGIF(sOutputName.c_str(), 40); // 40msec = 2sec bei 50 Frames, d.h. 25 Bilder je Sekunde

        string sLabelsCache[2];
        sLabelsCache[0] = sLabels;
        sLabelsCache[1] = sDataLabels;

        if (_pData->getBackground().length() && _pData->getBGColorScheme() != "<<REALISTIC>>")
        {
            //cerr << "|-> Lade Hintergrund ... ";
            _mBackground.Import(_pData->getBackground().c_str(), "kw");
            //cerr << "Abgeschlossen." << endl;
        }

        /*********************************
         * Eigentlicher Plot-Algorithmus *
         *********************************/

        for (int t_animate = 0; t_animate <= _pData->getAnimateSamples(); t_animate++)
        {
            if (_pData->getAnimateSamples() && !_pData->getSilentMode() && _option->getSystemPrintStatus())
            {
                // --> Falls es eine Animation ist, muessen die Plotvariablen zu Beginn zurueckgesetzt werden <--
                cerr << "\r|-> Rendere Frame " << t_animate+1 << " von " << _pData->getAnimateSamples()+1 << " ... ";
                nStyle = 0;

                // --> Neuen GIF-Frame oeffnen <--
                _graph->NewFrame();
                if (t_animate)
                {
                    parser_iVars.vValue[0][0] = dRanges[0][0];
                    parser_iVars.vValue[1][0] = dRanges[1][0];
                    parser_iVars.vValue[2][0] = dRanges[2][0];
                    parser_iVars.vValue[3][0] += (_pData->gettBoundary(1) - _pData->gettBoundary())/(double)_pData->getAnimateSamples();
                    sLabels = sLabelsCache[0];
                    sDataLabels = sLabelsCache[1];
                }
            }
            double dt_max = parser_iVars.vValue[3][0];

            // --> Ist ein Titel gegeben? Dann weisse ihn zu <--
            if (_pData->getTitle().length())
                _graph->Title(fromSystemCodePage(_pData->getTitle()).c_str(), "", -1.5);

            // --> Orthogonal-Projektionen aktivieren <--
            if (_pData->getOrthoProject()
                && (b3D
                    || (b2D && sCommand.substr(0,4) != "grad" && sCommand.substr(0,4) != "dens")
                    || b3DVect
                    || sCommand == "plot3d")
                )
            {
                /*if (_pData->getColorbar() && !b3DVect && sCommand != "plot3d")
                    _graph->Colorbar(_pData->getColorScheme().c_str());*/
                _graph->Ternary(4);
                _graph->SetRotatedText(false);
            }

            // --> Rotationen und allg. Grapheneinstellungen <--
            if (nMaxPlotDim > 2 && !nPlotCompose)
            {
                //_graph->Rotate(_pData->getRotateAngle(), _pData->getRotateAngle(1));
            }
            if (sCommand.substr(0,4) == "mesh" || sCommand.substr(0,4) == "surf" || sCommand.substr(0,4) == "cont" || b3D || b3DVect || _pData->getPipe() || bDraw3D || bDraw)
            {
                /*if (!b2DVect)
                    _graph->Rotate(_pData->getRotateAngle(),_pData->getRotateAngle(1));    // Ausrichtung des Plots einstellen*/

                // --> Licht- und Transparenz-Effekte <--
                if (sCommand.substr(0,4) == "surf"
                    || (sCommand.substr(0,4) == "dens" && sCommand.find("3d") != string::npos && !_pData->getContProj())
                    || (sCommand.substr(0,4) == "cont" && sCommand.find("3d") && _pData->getContFilled() && !_pData->getContProj())
                    || bDraw3D
                    || bDraw)
                _graph->Alpha(_pData->getTransparency());
                if (_pData->getAlphaMask() && sCommand.substr(0,4) == "surf" && !_pData->getTransparency())
                    _graph->Alpha(true);

                _graph->Light(_pData->getLighting());
                if (_pData->getLighting())
                {
                    if (!_pData->getPipe() && !bDraw)
                    {
                        _graph->AddLight(1, mglPoint(5,30,5), 'w', 0.2);
                        _graph->AddLight(2, mglPoint(5,-30,5), 'w', 0.08);
                    }
                    else
                    {
                        _graph->AddLight(1, mglPoint(-5,5,1), 'w', 0.4);
                        _graph->AddLight(2, mglPoint(-3,3,9), 'w', 0.1);
                        _graph->AddLight(3, mglPoint(5,-5,1), 'w', 0.1);
                        _graph->Light(0, false);
                    }
                }
            }
            /*else if (sCommand == "plot3d")
            {
                _graph->Rotate(_pData->getRotateAngle(),_pData->getRotateAngle(1));
            }*/


            // --> Plot-Speicher mit den berechneten Daten fuellen <--
            if (!b2D && sFunc != "<<empty>>" && sCommand != "plot3d" && !b3D && !b3DVect && !b2DVect)
            {
                for (int x = 0; x < nSamples; x++)
                {
                    if (x != 0)
                        parser_iVars.vValue[0][0] += (dRanges[0][1] - dRanges[0][0]) / (double)(nSamples-1);
                    vResults = _parser->Eval(nFunctions);
                    for (int i = 0; i < nFunctions; i++)
                    {
                        if (isinf(vResults[i]) || isnan(vResults[i]))
                        {
                            if (!x)
                                _pData->setData(x, i, NAN);
                            else
                                _pData->setData(x, i, NAN);
                        }
                        else
                            _pData->setData(x, i, (double)vResults[i]);
                    }
                }
            }
            else if (sFunc != "<<empty>>" && b3D)
            {
                mglPoint _mLowerEdge = parser_CalcCutBox(_pData->getRotateAngle(1), dRanges, 0, _pData->getCoords(), true);
                mglPoint _mHigherEdge = parser_CalcCutBox(_pData->getRotateAngle(1), dRanges, 1, _pData->getCoords(), true);
                // --> Wie oben, aber 3D-Fall <--
                for (int x = 0; x < nSamples; x++)
                {
                    if (x != 0)
                    {
                        parser_iVars.vValue[0][0] += (dRanges[0][1] - dRanges[0][0]) / (double)(nSamples-1);
                    }
                    parser_iVars.vValue[1][0] = dRanges[1][0];
                    for (int y = 0; y < nSamples; y++)
                    {
                        if (y != 0)
                        {
                            parser_iVars.vValue[1][0] += (dRanges[1][1] - dRanges[1][0]) / (double)(nSamples-1);
                        }
                        parser_iVars.vValue[2][0] = dRanges[2][0];
                        for (int z = 0; z < nSamples; z++)
                        {
                            if (z != 0)
                            {
                                parser_iVars.vValue[2][0] += (dRanges[2][1] - dRanges[2][0]) / (double)(nSamples-1);
                            }
                            double dResult = _parser->Eval();
                            //vResults = &_parser->Eval();

                            if (isnan(dResult)
                                || isinf(dResult)
                                || (_pData->getCutBox()
                                    && sCommand.substr(0,4) != "cont"
                                    && sCommand.substr(0,4) != "grad"
                                    && (sCommand.substr(0,4) != "dens" || (sCommand.substr(0,4) == "dens" && _pData->getCloudPlot()))
                                    && ((parser_iVars.vValue[0][0] >= _mLowerEdge.val(0) && parser_iVars.vValue[0][0] <= _mHigherEdge.val(0))
                                        && (parser_iVars.vValue[1][0] >= _mLowerEdge.val(1) && parser_iVars.vValue[1][0] <= _mHigherEdge.val(1))
                                        && (parser_iVars.vValue[2][0] >= _mLowerEdge.val(2) && parser_iVars.vValue[2][0] <= _mHigherEdge.val(2)))))
                            {
                                _pData->setData(x, y, NAN, z);
                            }
                            else
                                _pData->setData(x, y, dResult, z);

                            /*if (_option->getbDebug())
                                cerr << "|-> DEBUG: (x = " << x << ", y = " << y << ", z = " << z << ") " << _parser->Eval() << " / " << _pData->getData(x,y,z) << endl;*/
                        }
                    }
                    /*if (_option->getbDebug())
                        cerr << "|" << endl;*/
                }

            }
            else if (sFunc != "<<empty>>" && sCommand == "plot3d")
            {
                // --> Parametrische Kurven berechnen <--
                for (int k = 0; k < _pData->getLayers(); k++)
                {
                    parser_iVars.vValue[0][0] = 0.0;
                    parser_iVars.vValue[1][0] = 0.0;
                    parser_iVars.vValue[2][0] = 0.0;
                    parser_iVars.vValue[3][0] = _pData->gettBoundary();
                    vResults = _parser->Eval(nFunctions);
                    parser_iVars.vValue[0][0] = vResults[3*k];
                    if (3*k+1 < nFunctions)
                        parser_iVars.vValue[1][0] = vResults[3*k+1];
                    if (3*k+2 < nFunctions)
                        parser_iVars.vValue[2][0] = vResults[3+k+2];
                    int nRenderSamples = nSamples;
                    for (int t = 0; t < nRenderSamples; t++)
                    {
                        if (t != 0)
                        {
                            if (_pData->getAnimateSamples())
                            {
                                double dSamples = 1.0;
                                if ((t_animate*100.0)/_pData->getAnimateSamples() <= 25.0)
                                {
                                    dSamples = (double)(nSamples-1) * 0.25;
                                }
                                else if ((t_animate*100.0)/_pData->getAnimateSamples() <= 50.0)
                                {
                                    dSamples = (double)(nSamples-1) * 0.5;
                                }
                                else if ((t_animate*100.0)/_pData->getAnimateSamples() <= 75.0)
                                {
                                    dSamples = (double)(nSamples-1) * 0.75;
                                }
                                else
                                {
                                    dSamples = (double)(nSamples-1);
                                }
                                nRenderSamples = (int)dSamples + 1;
                                parser_iVars.vValue[3][0] += (dt_max - _pData->gettBoundary()) / dSamples;
                            }
                            else
                                parser_iVars.vValue[3][0] += (_pData->gettBoundary(1) - _pData->gettBoundary()) / (double)(nSamples-1);
                            parser_iVars.vValue[0][0] = _pData->getData(t-1,0,k);
                            parser_iVars.vValue[1][0] = _pData->getData(t-1,1,k);
                            parser_iVars.vValue[2][0] = _pData->getData(t-1,2,k);
                        }
                        // --> Wir werten alle Koordinatenfunktionen zugleich aus und verteilen sie auf die einzelnen Parameterkurven <--
                        vResults = _parser->Eval(nFunctions);
                        for (int i = 0; i < 3; i++)
                        {
                            if (i+3*k >= nFunctions)
                                break;
                            if (isinf(vResults[i+3*k]) || isnan(vResults[i+3+k]))
                            {
                                if (!t)
                                    _pData->setData(t, i, NAN, k);
                                else
                                    _pData->setData(t, i, NAN, k);
                            }
                            else
                                _pData->setData(t, i, (double)vResults[i+3*k], k);
                        }
                    }
                    for (int t = nRenderSamples; t < nSamples; t++)
                    {
                        for (int i = 0; i < 3; i++)
                            _pData->setData(t,i, NAN,k);
                    }
                }
                parser_iVars.vValue[3][0] = dt_max;
            }
            else if (sFunc != "<<empty>>" && b3DVect)
            {
                // --> Vektorfeld (3D= <--
                for (int x = 0; x < nSamples; x++)
                {
                    if (x != 0)
                    {
                        parser_iVars.vValue[0][0] += (dRanges[0][1] - dRanges[0][0]) / (double)(nSamples-1);
                    }
                    parser_iVars.vValue[1][0] = dRanges[1][0];
                    for (int y = 0; y < nSamples; y++)
                    {
                        if (y != 0)
                        {
                            parser_iVars.vValue[1][0] += (dRanges[1][1] - dRanges[1][0]) / (double)(nSamples-1);
                        }
                        parser_iVars.vValue[2][0] = dRanges[2][0];
                        for (int z = 0; z < nSamples; z++)
                        {
                            if (z != 0)
                            {
                                parser_iVars.vValue[2][0] += (dRanges[2][1] - dRanges[2][0]) / (double)(nSamples-1);
                            }
                            vResults = _parser->Eval(nFunctions);

                            for (int i = 0; i < nFunctions; i++)
                            {
                                if (i > 2)
                                    break;
                                if (isnan(vResults[i]) || isinf(vResults[i]))
                                {
                                    if (!x || !y)
                                        _pData->setData(x, y, NAN, 3*z+i);
                                    else
                                        _pData->setData(x, y, NAN, 3*z+i);
                                }
                                else
                                    _pData->setData(x, y, (double)vResults[i], 3*z+i);
                            }
                            /*if (_option->getbDebug())
                                cerr << "|-> DEBUG: (x = " << x << ", y = " << y << ", z = " << z << ") " << _parser->Eval() << " / " << _pData->getData(x,y,z) << endl;*/
                        }
                    }
                    /*if (_option->getbDebug())
                        cerr << "|" << endl;*/
                }
            }
            else if (sFunc != "<<empty>>" && b2DVect)
            {
                // --> Wie oben, aber 2D-Fall <--
                for (int x = 0; x < nSamples; x++)
                {
                    if (x != 0)
                    {
                        parser_iVars.vValue[0][0] += (dRanges[0][1] - dRanges[0][0]) / (double)(nSamples-1);
                    }
                    parser_iVars.vValue[1][0] = dRanges[1][0];
                    for (int y = 0; y < nSamples; y++)
                    {
                        if (y != 0)
                        {
                            parser_iVars.vValue[1][0] += (dRanges[1][1] - dRanges[1][0]) / (double)(nSamples-1);
                        }
                        vResults = _parser->Eval(nFunctions);

                        for (int i = 0; i < nFunctions; i++)
                        {
                            if (i > 1)
                                break;
                            if (isnan(vResults[i]) || isinf(vResults[i]))
                            {
                                if (!x || !y)
                                    _pData->setData(x, y, NAN, i);
                                else
                                    _pData->setData(x, y, NAN, i);
                            }
                            else
                                _pData->setData(x, y, (double)vResults[i], i);
                        }
                    /*if (_option->getbDebug())
                        cerr << "|-> DEBUG: (x = " << x << ", y = " << y << ", z = " << z << ") " << _parser->Eval() << " / " << _pData->getData(x,y,z) << endl;*/
                    }
                }
                /*if (_option->getbDebug())
                    cerr << "|" << endl;*/
            }
            else if (sFunc != "<<empty>>")
            {
                // --> Wie oben, aber 2D-Fall <--
                for (int x = 0; x < nSamples; x++)
                {
                    if (x != 0)
                    {
                        parser_iVars.vValue[0][0] += (dRanges[0][1] - dRanges[0][0]) / (double)(nSamples-1);
                    }
                    parser_iVars.vValue[1][0] = dRanges[1][0];
                    for (int y = 0; y < nSamples; y++)
                    {
                        if (y != 0)
                        {
                            parser_iVars.vValue[1][0] += (dRanges[1][1] - dRanges[1][0]) / (double)(nSamples-1);
                        }
                        vResults = _parser->Eval(nFunctions);
                        for (int i = 0; i < nFunctions; i++)
                        {
                            if (isnan(vResults[i]) || isinf(vResults[i]))
                            {
                                if (!x || !y)
                                    _pData->setData(x, y, NAN, i);
                                else
                                    _pData->setData(x, y, NAN, i);
                            }
                            else
                                _pData->setData(x, y, (double)vResults[i], i);
                        }
                        /*if (_option->getbDebug())
                            cerr << "|-> DEBUG: (x = " << x << ", y = " << y << ") " << _parser->Eval() << " / " << _pData->getData(x,y) << endl;*/
                    }
                    /*if (_option->getbDebug())
                        cerr << "|" << endl;*/
                }
            }

            if (b2DVect)
                _pData->normalize(2, t_animate);
            else if (b3DVect)
                _pData->normalize(3, t_animate);
            if (_option->getbDebug())
                cerr << "|-> DEBUG: Adding params ..." << endl;
            //cerr << _pData->getGivenRanges() << endl;

            /* --> Darstellungsintervalle anpassen: Wenn nicht alle vorgegeben sind, sollten die fehlenden
             *     passend berechnet werden. Damit aber kein Punkt auf dem oberen oder dem unteren Rahmen
             *     liegt, vergroessern wir das Intervall um 5% nach oben und 5% nach unten <--
             * --> Fuer Vektor- und 3D-Plots ist das allerdings recht sinnlos <--
             */
            if (sFunc != "<<empty>>")
            {
                if (isnan(_pData->getMin()) || isnan(_pData->getMax()) || isinf(_pData->getMin()) || isinf(_pData->getMax()))
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
                    throw PLOTDATA_IS_NAN;
                }
            }
            if (!(b3D || b3DVect || b2DVect) && !nPlotCompose && !(bDraw3D || bDraw))
            {
                if (!b2D && _pData->getGivenRanges() < 2 && sCommand != "plot3d")
                {
                    if (sFunc != "<<empty>>")
                    {
                        if (_mDataPlots && _pData->getMin() < dRanges[1][0])
                            dRanges[1][0] = _pData->getMin();
                        if (_mDataPlots && _pData->getMax() > dRanges[1][1])
                            dRanges[1][1] = _pData->getMax();
                        if (!_mDataPlots)
                        {
                            dRanges[1][0] = _pData->getMin();
                            dRanges[1][1] = _pData->getMax();
                        }
                    }
                    double dInt = fabs(dRanges[1][1] - dRanges[1][0]);
                    if (dInt == 0.0 || (dInt < 1e-4 * dRanges[1][0]))
                        dInt = fabs(dRanges[1][1]);
                    dRanges[1][0] -= dInt / 20.0;
                    if (dRanges[1][0] <= 0 && _pData->getyLogscale())
                        dRanges[1][0] += dInt / 20.0;
                    if (dRanges[1][0] < 0.0 && _pData->getCoords())
                        dRanges[1][0] = 0.0;
                    dRanges[1][1] += dInt / 20.0;
                }
                else if (sCommand == "plot3d" && _pData->getGivenRanges() < 3)
                {
                    if (sFunc != "<<empty>>")
                    {
                        for (int i = 0; i < 3; i++)
                        {
                            if (_pData->getGivenRanges() >= i+1 && _pData->getRangeSetting(i))
                                continue;
                            if (_mDataPlots && _pData->getMin(i) < dRanges[i][0])
                                dRanges[i][0] = _pData->getMin(i);
                            if (_mDataPlots && _pData->getMax(i) > dRanges[i][1])
                                dRanges[i][1] = _pData->getMax(i);
                            if (!_mDataPlots)
                            {
                                dRanges[i][0] = _pData->getMin(i);
                                dRanges[i][1] = _pData->getMax(i);
                            }
                            double dInt = fabs(dRanges[i][1] - dRanges[i][0]);
                            if (dInt == 0.0 || dInt < 1e-4 * dRanges[i][0])
                                dInt = fabs(dRanges[i][1]);
                            dRanges[i][0] -= dInt / 20.0;
                            dRanges[i][1] += dInt / 20.0;
                            if ((dRanges[i][0] < 0.0 && (_pData->getCoords() == 2 || (_pData->getCoords() == 1 && i < 2))) || (dRanges[i][0] && _pData->getCoords() && !i))
                                dRanges[i][0] = 0.0;
                        }
                        if (_pData->getCoords() && dRanges[1][0] != 0.0)
                            dRanges[1][0] = 0.0;
                        if (_pData->getCoords() && dRanges[1][1] != 2.0)
                            dRanges[1][1] = 1.99999999;
                        if (_pData->getCoords() == 2 && dRanges[2][1] != 1.0)
                            dRanges[2][1] = 0.99999999;
                        if (_pData->getCoords() == 2 && dRanges[2][0] != 0.0)
                            dRanges[2][0] = 0.0;
                    }
                }
                else if (_pData->getGivenRanges() < 3)
                {
                    if (sFunc != "<<empty>>")
                    {
                        if (_mDataPlots && _pData->getMin() < dRanges[2][0])
                            dRanges[2][0] = _pData->getMin();
                        if (_mDataPlots && _pData->getMax() > dRanges[2][1])
                            dRanges[2][1] = _pData->getMax();
                        if (!_mDataPlots)
                        {
                            dRanges[2][0] = _pData->getMin();
                            dRanges[2][1] = _pData->getMax();
                        }
                    }
                    double dInt = fabs(dRanges[2][1] - dRanges[2][0]);
                    if (dInt == 0.0 || dInt < 1e-4 * dRanges[2][0])
                        dInt = fabs(dRanges[2][1]);
                    dRanges[2][0] -= dInt / 20.0;
                    if (dRanges[2][0] <= 0 && _pData->getzLogscale())
                        dRanges[2][0] += dInt / 20.0;
                    if (dRanges[2][0] < 0.0 && _pData->getCoords())
                        dRanges[2][0] = 0.0;
                    dRanges[2][1] += dInt / 20.0;
                }
            }
            else if (b2DVect && !nPlotCompose && !(bDraw3D || bDraw))
            {
                if (sFunc != "<<empty>>")
                {
                    if (_pData->getGivenRanges() < 3)
                    {
                        dRanges[2][0] = _pData->getMin();
                        dRanges[2][1] = _pData->getMax();
                    }
                    if (_pData->getCoords())
                    {
                        dRanges[1][0] = 0.0;
                        dRanges[1][1] = 1.99999999;
                        dRanges[0][0] = 0.0;
                    }
                }
            }
            if (_pData->getxLogscale() || _pData->getyLogscale() || _pData->getzLogscale())
            {
                if (_pData->getxLogscale())
                {
                    if (dRanges[0][0] <= 0 && dRanges[0][1] <= 0)
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
                    else if (dRanges[0][0] <= 0)
                        dRanges[0][0] = dRanges[0][1]*1e-3;
                }
                if (_pData->getyLogscale())
                {
                    if (dRanges[1][0] <= 0 && dRanges[1][1] <= 0)
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
                    else if (dRanges[1][0] <= 0)
                        dRanges[1][0] = dRanges[1][1]*1e-3;
                }
                if (_pData->getzLogscale() && (b2D || sCommand == "plot3d" || b3D || b3DVect))
                {
                    if (dRanges[2][0] <= 0 && dRanges[2][1] <= 0)
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
                    else if (dRanges[2][0] <= 0)
                        dRanges[2][0] = dRanges[2][1]*1e-3;
                }
            }

            // --> (Endgueltige) Darstellungsintervalle an das Plot-Objekt uebergeben <--
            if (!nPlotCompose)
            {
                /*if (_pData->getCoords())
                {
                    _graph->SetRanges(0.0, 1.9999999,dRanges[1][0],dRanges[1][1],dRanges[2][0],dRanges[2][1]);
                }
                else*/
                    _graph->SetRanges(dRanges[0][0], dRanges[0][1], dRanges[1][0], dRanges[1][1], dRanges[2][0], dRanges[2][1]);
                if (_pData->getBars() && _mDataPlots)
                {
                    int nMinbars = -1;
                    for (int k = 0; k < nDataPlots; k++)
                    {
                        if (nMinbars == -1 || nMinbars > _mDataPlots[k][0].GetNx())
                            nMinbars = _mDataPlots[k][0].GetNx();
                    }
                    if (nMinbars < 2)
                        nMinbars = 2;
                    _graph->SetRange('x',dRanges[0][0]-fabs(dRanges[0][0]-dRanges[0][1])/(double)(2*(nMinbars-1)), dRanges[0][1]+fabs(dRanges[0][0]-dRanges[0][1])/(double)(2*(nMinbars-1)));
                    if (sCommand == "plot3d")
                        _graph->SetRange('y',dRanges[1][0]-fabs(dRanges[1][0]-dRanges[1][1])/(double)(2*(nMinbars-1)), dRanges[1][1]+fabs(dRanges[1][0]-dRanges[1][1])/(double)(2*(nMinbars-1)));
                }

                if (!isnan(_pData->getColorRange()))
                {
                    /*if (_pData->getzLogscale() || _pData->getyLogscale() || _pData->getxLogscale())
                        _graph->SetRange('c',  _pData->getColorRange(1)*1e-3, _pData->getColorRange(1));
                    else*/
                        _graph->SetRange('c', _pData->getColorRange(), _pData->getColorRange(1));
                        dColorRanges[0] = _pData->getColorRange();
                        dColorRanges[1] = _pData->getColorRange(1);

                }
                else if (_mDataPlots)
                {
                    double dColorMin = dDataRanges[2][0];
                    double dColorMax = dDataRanges[2][1];
                    if (_pData->getMax() > dColorMax && sFunc != "<<empty>>" && sCommand == "plot3d")
                        dColorMax = _pData->getMax(2);
                    if (_pData->getMin() < dColorMin && sFunc != "<<empty>>" && sCommand == "plot3d")
                        dColorMin = _pData->getMin(2);

                    _graph->SetRange('c', dColorMin-0.05*(dColorMax-dColorMin), dColorMax+0.05*(dColorMax-dColorMin));
                    dColorRanges[0] = dColorMin-0.05*(dColorMax-dColorMin);
                    dColorRanges[1] = dColorMax+0.05*(dColorMax-dColorMin);
                }
                else
                {
                    if ((b2DVect || b3DVect) && (_pData->getFlow() || _pData->getPipe()))
                    {
                        _graph->SetRange('c', -1.05, 1.05);
                        dColorRanges[0] = -1.05;
                        dColorRanges[1] = 1.05;
                    }
                    else if (b2DVect || b3DVect)
                    {
                        _graph->SetRange('c', -0.05, 1.05);
                        dColorRanges[0] = -1.05;
                        dColorRanges[1] = 1.05;
                    }
                    else
                    {
                        _graph->SetRange('c', _pData->getMin()-0.05*(_pData->getMax()-_pData->getMin()), _pData->getMax()+0.05*(_pData->getMax()-_pData->getMin()));
                        dColorRanges[0] = _pData->getMin()-0.05*(_pData->getMax()-_pData->getMin());
                        dColorRanges[1] = _pData->getMax()+0.05*(_pData->getMax()-_pData->getMin());
                    }
                }
                // --> Andere Parameter setzen (die i. A. von den bestimmten Ranges abghaengen) <--
                if (_pData->getAxis())
                {
                    if (_option->getbDebug())
                    {
                        for (int i = 0; i < 3; i++)
                        {
                            cerr << "|-> DEBUG: " << dRanges[i][0] << ":" << dRanges[i][1] << endl;
                        }
                    }
                    for (int i = 0; i < 4; i++)
                    {
                        if (_pData->getTickTemplate(i).length())
                        {
                            if (i < 3)
                                _graph->SetTickTempl('x'+i, _pData->getTickTemplate(i).c_str());
                            else
                                _graph->SetTickTempl('c', _pData->getTickTemplate(i).c_str());
                        }
                        if (_pData->getCustomTick(i).length())
                        {
                            int nCount = 1;
                            mglData _mAxisRange;
                            if (i < 3)
                            {
                                for (unsigned int n = 0; n < _pData->getCustomTick(i).length(); n++)
                                {
                                    if (_pData->getCustomTick(i)[n] == '\n')
                                        nCount++;
                                }
                                //cerr << nCount << endl;
                                _mAxisRange.Create(nCount);
                                if (nCount == 1)
                                {
                                    _mAxisRange.a[0] = dRanges[i][0] + (dRanges[i][1]-dRanges[i][0])/2.0;
                                }
                                else
                                {
                                    for (int n = 0; n < nCount; n++)
                                    {
                                        _mAxisRange.a[n] = dRanges[i][0] + (double)n*(dRanges[i][1]-dRanges[i][0])/(double)(nCount-1);
                                        //cerr << dRanges[i][0] + (double)n*(dRanges[i][1]-dRanges[i][0])/(double)(nCount-1) << endl;
                                    }
                                }
                                _graph->SetTicksVal('x'+i, _mAxisRange, _pData->getCustomTick(i).c_str());
                            }
                            else
                            {
                                for (unsigned int n = 0; n < _pData->getCustomTick(i).length(); n++)
                                {
                                    if (_pData->getCustomTick(i)[n] == '\n')
                                        nCount++;
                                }
                                _mAxisRange.Create(nCount);
                                if (nCount == 1)
                                    _mAxisRange.a[0] = dColorRanges[0] + (dColorRanges[1]-dColorRanges[0])/2.0;
                                else
                                {
                                    for (int n = 0; n < nCount; n++)
                                    {
                                        _mAxisRange.a[n] = dColorRanges[0] + (double)n*(dColorRanges[1]-dColorRanges[0])/(double)(nCount-1);
                                    }
                                }
                                _graph->SetTicksVal('c', _mAxisRange, _pData->getCustomTick(i).c_str());
                            }
                        }
                    }
                    if (_pData->getBox() || _pData->getCoords())
                    {
                        if (!(b2D || b3D || sCommand == "plot3d" || b3DVect || b2DVect))
                        {
                            if (_pData->getCoords())
                            {
                                //_graph->SetOrigin(0.5, dRanges[1][1]);
                                _graph->SetOrigin(0.0, dRanges[1][1]);
                                _graph->SetFunc("y*cos(pi*x)", "y*sin(pi*x)");
                                //_graph->SetTickShift(mglPoint(0.25, 0.0));
                                if (!_pData->getSchematic())
                                    _graph->Axis("xy"); //U
                                else
                                {
                                    _graph->SetTickLen(1e-20);
                                    _graph->Axis("xy_");
                                }
                            }
                            else
                            {
                                if (!_pData->getSchematic())
                                    _graph->Axis("xy");
                            }
                        }
                        else if (sCommand.find("3d") != string::npos)
                        {
                            if (_pData->getCoords() == 1)
                            {
                                _graph->SetOrigin(dRanges[0][1], 0.0, dRanges[2][0]);
                                _graph->SetFunc("x*cos(pi*y)", "x*sin(pi*y)", "z");
                                if (!_pData->getSchematic())
                                    _graph->Axis();
                                else
                                {
                                    _graph->SetTickLen(1e-20);
                                    _graph->Axis("_");
                                }
                            }
                            else if (_pData->getCoords() == 2)
                            {
                                _graph->SetOrigin(dRanges[0][1], 0.0, 0.5);
                                _graph->SetFunc("x*cos(pi*y)*sin(pi*z)", "x*sin(pi*y)*sin(pi*z)", "x*cos(pi*z)");
                                if (!_pData->getSchematic())
                                    _graph->Axis();
                                else
                                {
                                    _graph->SetTickLen(1e-20);
                                    _graph->Axis("_");
                                }
                            }
                            else
                            {
                                if (!_pData->getSchematic())
                                    _graph->Axis("xyz");
                            }
                        }
                        else if (b2DVect)
                        {
                            if (_pData->getCoords())
                            {
                                _graph->SetOrigin(dRanges[0][1], 0.0);
                                _graph->SetFunc("x*cos(pi*y)", "x*sin(pi*y)");
                                if (!_pData->getSchematic())
                                    _graph->Axis("xy");
                                else
                                {
                                    _graph->SetTickLen(1e-20);
                                    _graph->Axis("xy_");
                                }
                            }
                            else
                            {
                                if (!_pData->getSchematic())
                                    _graph->Axis("xy");
                            }
                        }
                        else
                        {
                            if (_pData->getCoords() == 1)
                            {
                                _graph->SetOrigin(0.0, dRanges[1][0], dRanges[2][1]);
                                _graph->SetFunc("z*cos(pi*x)", "z*sin(pi*x)", "y");
                                //_graph->SetTickShift(mglPoint(0.15, 0.0, 0.0));
                                if (!_pData->getSchematic())
                                    _graph->Axis(); //U
                                else
                                {
                                    _graph->SetTickLen(1e-20);
                                    _graph->Axis("_");
                                }
                            }
                            else if(_pData->getCoords() == 2)
                            {
                                _graph->SetOrigin(0.0, 0.5, dRanges[2][1]);
                                _graph->SetFunc("z*cos(pi*x)*sin(pi*y)", "z*sin(pi*x)*sin(pi*y)", "z*cos(pi*y)");
                                //_graph->SetTickShift(mglPoint(0.15,0.15, 0.0));
                                if (!_pData->getSchematic())
                                    _graph->Axis(); //U
                                else
                                {
                                    _graph->SetTickLen(1e-20);
                                    _graph->Axis("_");
                                }
                            }
                            else
                            {
                                if (!_pData->getSchematic())
                                    _graph->Axis();
                            }
                        }
                    }
                    else if (isnan(_pData->getOrigin()) && isnan(_pData->getOrigin(1)) && isnan(_pData->getOrigin(2)))
                    {
                        if (dRanges[0][0] <= 0.0
                            && dRanges[0][1] >= 0.0
                            && dRanges[1][0] <= 0.0
                            && dRanges[1][1] >= 0.0
                            && dRanges[2][0] <= 0.0
                            && dRanges[2][1] >= 0.0
                            && nMaxPlotDim > 2 //(sCommand.find("3d") != string::npos || (b2D && sCommand != "dens"))
                            )
                        {
                            _graph->SetOrigin(0.0, 0.0, 0.0);
                            if (!_pData->getSchematic())
                                _graph->Axis("AKDTVISO");
                            else
                            {
                                _graph->SetTickLen(1e-20);
                                _graph->SetTicks('x', -5, 1);
                                _graph->SetTicks('y', -5, 1);
                                _graph->SetTicks('z', -5, 1);
                                _graph->Axis("AKDTVISO_");
                            }
                        }
                        else if (dRanges[0][0] <= 0.0
                            && dRanges[0][1] >= 0.0
                            && dRanges[1][0] <= 0.0
                            && dRanges[1][1] >= 0.0
                            && nMaxPlotDim <= 2 //(sCommand.find("3d") == string::npos && !(b2D && sCommand != "dens"))
                            )
                        {
                            _graph->SetOrigin(0.0, 0.0);
                            if (!_pData->getSchematic())
                                _graph->Axis("AKDTVISO");
                            else
                            {
                                _graph->SetTickLen(1e-20);
                                _graph->SetTicks('x', -5, 1);
                                _graph->SetTicks('y', -5, 1);
                                _graph->Axis("AKDTVISO_");
                            }
                        }
                        else if (nMaxPlotDim == 3)
                        {
                            if (dRanges[0][0] <= 0.0 && dRanges[0][1] >= 0.0)
                            {
                                if (dRanges[1][0] <= 0.0 && dRanges[1][1] >= 0.0)
                                    _graph->SetOrigin(0.0, 0.0, dRanges[2][0]);
                                else if (dRanges[2][0] <= 0.0 && dRanges[2][1] >= 0.0)
                                    _graph->SetOrigin(0.0, dRanges[1][0], 0.0);
                                else
                                    _graph->SetOrigin(0.0, dRanges[1][0], dRanges[2][0]);
                            }
                            else if (dRanges[1][0] <= 0.0 && dRanges[1][1] >= 0.0)
                            {
                                if (dRanges[2][0] <= 0.0 && dRanges[2][1] >= 0.0)
                                    _graph->SetOrigin(dRanges[0][0], 0.0, 0.0);
                                else
                                    _graph->SetOrigin(dRanges[0][0], 0.0, dRanges[2][0]);
                            }
                            else if (dRanges[2][0] <= 0.0 && dRanges[2][1] >= 0.0)
                                _graph->SetOrigin(dRanges[0][0], dRanges[1][0], 0.0);
                            else
                                _graph->SetOrigin(dRanges[0][0], dRanges[1][0], dRanges[2][0]);

                            if (!_pData->getSchematic())
                                _graph->Axis("AKDTVISO");
                            else
                            {
                                _graph->SetTickLen(1e-20);
                                _graph->SetTicks('x', -5, 1);
                                _graph->SetTicks('y', -5, 1);
                                _graph->SetTicks('z', -5, 1);
                                _graph->Axis("AKDTVISO_");
                            }

                        }
                        else if (nMaxPlotDim <= 2)
                        {
                            if (dRanges[0][0] <= 0.0 && dRanges[0][1] >= 0.0)
                            {
                                _graph->SetOrigin(0.0, dRanges[1][0]);
                            }
                            else if (dRanges[1][0] <= 0.0 && dRanges[1][1] >= 0.0)
                            {
                                _graph->SetOrigin(dRanges[0][0], 0.0);
                            }
                            else
                                _graph->SetOrigin(dRanges[0][0], dRanges[1][0]);

                            if (!_pData->getSchematic())
                                _graph->Axis("AKDTVISO");
                            else
                            {
                                _graph->SetTickLen(1e-20);
                                _graph->SetTicks('x', -5, 1);
                                _graph->SetTicks('y', -5, 1);
                                _graph->Axis("AKDTVISO_");
                            }
                        }
                    }
                    else if (_pData->getOrigin() != 0.0 || _pData->getOrigin(1) != 0.0 || _pData->getOrigin(2) != 0.0)
                    {
                        if (dRanges[0][0] <= _pData->getOrigin()
                            && dRanges[0][1] >= _pData->getOrigin()
                            && dRanges[1][0] <= _pData->getOrigin(1)
                            && dRanges[1][1] >= _pData->getOrigin(1)
                            && dRanges[2][0] <= _pData->getOrigin(2)
                            && dRanges[2][1] >= _pData->getOrigin(2)
                            && nMaxPlotDim > 2 //(sCommand.find("3d") != string::npos || (b2D && sCommand != "dens"))
                            )
                        {
                            _graph->SetOrigin(_pData->getOrigin(), _pData->getOrigin(1), _pData->getOrigin(2));
                            if (!_pData->getSchematic())
                                _graph->Axis("AKDTVISO");
                            else
                            {
                                _graph->SetTickLen(1e-20);
                                _graph->SetTicks('x', -5, 1);
                                _graph->SetTicks('y', -5, 1);
                                _graph->SetTicks('z', -5, 1);
                                _graph->Axis("AKDTVISO_");
                            }
                        }
                        else if (dRanges[0][0] <= _pData->getOrigin()
                            && dRanges[0][1] >= _pData->getOrigin()
                            && dRanges[1][0] <= _pData->getOrigin(1)
                            && dRanges[1][1] >= _pData->getOrigin(1)
                            && nMaxPlotDim <= 2 //(sCommand.find("3d") == string::npos && !(b2D && sCommand != "dens"))
                            )
                        {
                            _graph->SetOrigin(_pData->getOrigin(), _pData->getOrigin(1));
                            if (!_pData->getSchematic())
                                _graph->Axis("AKDTVISO");
                            else
                            {
                                _graph->SetTickLen(1e-20);
                                _graph->SetTicks('x', -5, 1);
                                _graph->SetTicks('y', -5, 1);
                                _graph->Axis("AKDTVISO_");
                            }
                        }
                        else if (nMaxPlotDim == 3)
                        {
                            if (dRanges[0][0] <= _pData->getOrigin() && dRanges[0][1] >= _pData->getOrigin())
                            {
                                if (dRanges[1][0] <= _pData->getOrigin(1) && dRanges[1][1] >= _pData->getOrigin(1))
                                    _graph->SetOrigin(_pData->getOrigin(), _pData->getOrigin(1), dRanges[2][0]);
                                else if (dRanges[2][0] <= _pData->getOrigin(2) && dRanges[2][1] >= _pData->getOrigin(2))
                                    _graph->SetOrigin(_pData->getOrigin(), dRanges[1][0], _pData->getOrigin(2));
                                else
                                    _graph->SetOrigin(_pData->getOrigin(), dRanges[1][0], dRanges[2][0]);
                            }
                            else if (dRanges[1][0] <= _pData->getOrigin(1) && dRanges[1][1] >= _pData->getOrigin(1))
                            {
                                if (dRanges[2][0] <= _pData->getOrigin(2) && dRanges[2][1] >= _pData->getOrigin(2))
                                    _graph->SetOrigin(dRanges[0][0], _pData->getOrigin(1), _pData->getOrigin(2));
                                else
                                    _graph->SetOrigin(dRanges[0][0], _pData->getOrigin(1), dRanges[2][0]);
                            }
                            else if (dRanges[2][0] <= _pData->getOrigin(2) && dRanges[2][1] >= _pData->getOrigin(2))
                                _graph->SetOrigin(dRanges[0][0], dRanges[1][0], _pData->getOrigin(2));
                            else
                                _graph->SetOrigin(dRanges[0][0], dRanges[1][0], dRanges[2][0]);

                            if (!_pData->getSchematic())
                                _graph->Axis("AKDTVISO");
                            else
                            {
                                _graph->SetTickLen(1e-20);
                                _graph->SetTicks('x', -5, 1);
                                _graph->SetTicks('y', -5, 1);
                                _graph->SetTicks('z', -5, 1);
                                _graph->Axis("AKDTVISO_");
                            }

                        }
                        else if (nMaxPlotDim <= 2)
                        {
                            if (dRanges[0][0] <= _pData->getOrigin() && dRanges[0][1] >= _pData->getOrigin())
                            {
                                _graph->SetOrigin(_pData->getOrigin(), dRanges[1][0]);
                            }
                            else if (dRanges[1][0] <= _pData->getOrigin(1) && dRanges[1][1] >= _pData->getOrigin(1))
                            {
                                _graph->SetOrigin(dRanges[0][0], _pData->getOrigin(1));
                            }
                            else
                                _graph->SetOrigin(dRanges[0][0], dRanges[1][0]);

                            if (!_pData->getSchematic())
                                _graph->Axis("AKDTVISO");
                            else
                            {
                                _graph->SetTickLen(1e-20);
                                _graph->SetTicks('x', -5, 1);
                                _graph->SetTicks('y', -5, 1);
                                _graph->Axis("AKDTVISO_");
                            }
                        }
                    }
                    else if (dRanges[0][0] <= 0.0
                        && dRanges[0][1] >= 0.0
                        && dRanges[1][0] <= 0.0
                        && dRanges[1][1] >= 0.0
                        && dRanges[2][0] <= 0.0
                        && dRanges[2][1] >= 0.0
                        && nMaxPlotDim > 2 //(sCommand.find("3d") != string::npos || (b2D && sCommand != "dens"))
                        )
                    {
                        _graph->SetOrigin(0.0, 0.0, 0.0);
                        if (!_pData->getSchematic())
                            _graph->Axis("AKDTVISO");
                        else
                        {
                            _graph->SetTickLen(1e-20);
                            _graph->SetTicks('x', -5, 1);
                            _graph->SetTicks('y', -5, 1);
                            _graph->SetTicks('z', -5, 1);
                            _graph->Axis("AKDTVISO_");
                        }
                    }
                    else if (dRanges[0][0] <= 0.0
                        && dRanges[0][1] >= 0.0
                        && dRanges[1][0] <= 0.0
                        && dRanges[1][1] >= 0.0
                        && nMaxPlotDim <= 2 //(sCommand.find("3d") == string::npos && !(b2D && sCommand != "dens"))
                        )
                    {
                        _graph->SetOrigin(0.0, 0.0);
                        if (!_pData->getSchematic())
                            _graph->Axis("AKDTVISO");
                        else
                        {
                            _graph->SetTickLen(1e-20);
                            _graph->SetTicks('x', -5, 1);
                            _graph->SetTicks('y', -5, 1);
                            _graph->Axis("AKDTVISO_");
                        }
                    }
                    else if (nMaxPlotDim > 2) //sCommand.find("3d") != string::npos || (b2D && sCommand != "dens"))
                    {
                        _graph->SetOrigin(dRanges[0][0], dRanges[1][0], dRanges[2][0]);
                        if (!_pData->getSchematic())
                            _graph->Axis("AKDTVISO");
                        else
                        {
                            _graph->SetTickLen(1e-20);
                            _graph->SetTicks('x', -5, 1);
                            _graph->SetTicks('y', -5, 1);
                            _graph->SetTicks('z', -5, 1);
                            _graph->Axis("AKDTVISO_");
                        }
                    }
                    else
                    {
                        if (!_pData->getSchematic())
                            _graph->Axis("AKDTVISO");
                        else
                        {
                            _graph->SetTickLen(1e-20);
                            _graph->SetTicks('x', -5, 1);
                            _graph->SetTicks('y', -5, 1);
                            _graph->SetTicks('z', -5, 1);
                            _graph->Axis("AKDTVISO_");
                        }
                    }
                }
                if (_pData->getGrid() && !b2DVect && !b3DVect) // Standard-Grid
                {
                    if (_pData->getGrid() == 1)
                        _graph->Grid("xyzt", "=h");
                    else if (_pData->getGrid() == 2)
                    {
                        _graph->Grid("xyzt!", "=h");
                        _graph->Grid("xyzt", "-h");
                    }
                }
                else if (_pData->getGrid()) // Vektor-Grid
                    _graph->Grid("xyzt", "h");
                if (_pData->getBox() || _pData->getCoords())
                {
                    if (!_pData->getSchematic())
                        _graph->Box();
                    else
                        _graph->Box("k", false);
                }
                /*if (_pData->getCoords())
                {
                    //if (!((int)rint(dRanges[0][1]) % 2))
                    //    _graph->SetRanges(0.0, rint(dRanges[0][1])-0.00000001, dRanges[1][0], dRanges[1][1], dRanges[2][0], dRanges[2][1]);
                    //else
                    //    _graph->SetRanges(0.0, rint(dRanges[0][1])+0.99999999, dRanges[1][0], dRanges[1][1], dRanges[2][0], dRanges[2][1]);
                        _graph->SetRanges(0.0, dRanges[0][1], dRanges[1][0], dRanges[1][1], dRanges[2][0], dRanges[2][1]);
                }*/
            }
            if (_pData->getColorbar() && (sCommand.substr(0,4) == "grad" || sCommand.substr(0,4) == "dens") && !b3D && !_pData->getSchematic())
            {
                // --> In diesem Fall haetten wir gerne eine Colorbar fuer den Farbwert <--
                if (_pData->getBox())
                {
                    if (!(_pData->getContProj() && _pData->getContFilled()) && sCommand.substr(0,4) != "dens")
                    {
                        _graph->Colorbar(_pData->getColorSchemeLight("I>").c_str());
                        /*if (!_pData->getGreyscale())
                            _graph->Colorbar("{B8}{b8}{c8}{y8}{r8}{R8}I>");
                        else
                            _graph->Colorbar("hWI>");*/
                    }
                    else
                    {
                        _graph->Colorbar(_pData->getColorScheme("I>").c_str());
                        /*if (!_pData->getGreyscale())
                            _graph->Colorbar("I>");
                        else
                            _graph->Colorbar("kwI>");*/
                    }
                }
                else
                {
                    if (!(_pData->getContProj() && _pData->getContFilled()) && sCommand.substr(0,4) != "dens")
                    {
                        _graph->Colorbar(_pData->getColorSchemeLight(">").c_str());
                        /*if (!_pData->getGreyscale())
                            _graph->Colorbar("{B8}{b8}{c8}{y8}{r8}{R8}>");
                        else
                            _graph->Colorbar("hW>");*/
                    }
                    else
                    {
                        _graph->Colorbar(_pData->getColorScheme().c_str());
                        /*if (!_pData->getGreyscale())
                            _graph->Colorbar();
                        else
                            _graph->Colorbar("kw");*/
                    }
                }
            }
            else if (_pData->getColorbar() && !_pData->getSchematic() /*&& !_pData->getOrthoProject()*/
                && (sCommand.substr(0,4) == "mesh"
                    || sCommand.substr(0,4) == "surf"
                    || sCommand.substr(0,4) == "cont"
                    || sCommand.substr(0,4) == "dens"
                    || sCommand.substr(0,4) == "grad"
                    || (sCommand.substr(0,6) == "plot3d" && _pData->getMarks())
                    )
                )
            {
                /*if (b3D)
                    _graph->SetRange('c', _pData->getMin(), _pData->getMax());*/
                _graph->Colorbar(_pData->getColorScheme().c_str());
            }

            if (!nPlotCompose && nMaxPlotDim > 2)
                _graph->Perspective(_pData->getPerspective());
            //cerr << "|-> Berechne Ausgabe ..." << endl;

            // --> Rendern des Hintergrundes <--
            if (_pData->getBackground().length())
            {
                //cerr << "|-> Rendere Hintergrund ... ";
                if (_pData->getBGColorScheme() != "<<REALISTIC>>")
                {
                    _graph->SetRanges(dRanges[0][0], dRanges[0][1], dRanges[1][0], dRanges[1][1], _mBackground.Minimal(), _mBackground.Maximal());
                    _graph->Dens(_mBackground, _pData->getBGColorScheme().c_str());
                }
                else
                    _graph->Logo(_pData->getBackground().c_str());

                //_graph->Dens(_mBackground, "kRQYEGLCNBUMPw");
                _graph->Rasterize();
                _graph->SetRanges(dRanges[0][0], dRanges[0][1], dRanges[1][0], dRanges[1][1], dRanges[2][0], dRanges[2][1]);
                //cerr << "Abgeschlossen." << endl;
            }

            // --> Nun kopieren wir die aufbereiteten Datenpunkte in ein mglData-Objekt und plotten die Daten aus diesem Objekt <--
            if (b2D)        // 2D-Plot
            {
                if (sFunc != "<<empty>>")
                {
                    if (_option->getbDebug())
                        cerr << "|-> DEBUG: 2D-Data-Copy" << endl;
                    mglData _mData(nSamples, nSamples);
                    mglData _mMaskData;
                    if (_pData->getColorMask() || _pData->getAlphaMask())
                        _mMaskData.Create(nSamples, nSamples);

                    mglData _mContVec(35);
                    for (int nCont = 0; nCont < 35; nCont++)
                    {
                        _mContVec.a[nCont] = nCont*(dRanges[2][1]-dRanges[2][0])/34.0+dRanges[2][0];
                    }
                    _mContVec.a[17] = (dRanges[2][1]-dRanges[2][0])/2.0 + dRanges[2][0];

                    int nPos = 0;
                    for (int k = 0; k < nFunctions; k++)
                    {
                        StripSpaces(sLabels);
                        for (long int i = 0; i < nSamples; i++)
                        {
                            for (long int j = 0; j < nSamples; j++)
                            {
                                // --> Das Array im mglData-Objekt ist nur 1D vorhanden <--
                                _mData.a[i+nSamples*j] = _pData->getData(i,j,k);
                                if (_pData->getColorMask() || _pData->getAlphaMask())
                                    _mMaskData.a[i+nSamples*j] = _pData->getData(i,j,k+1);
                            }
                        }
                        if (_option->getbDebug())
                            cerr << "|-> DEBUG: generating 2D-Plot..." << endl;

                        if (_pData->getCutBox()
                            && sCommand.substr(0,4) != "cont"
                            && sCommand.substr(0,4) != "grad"
                            && sCommand.substr(0,4) != "dens")
                            _graph->SetCutBox(parser_CalcCutBox(_pData->getRotateAngle(1), dRanges, 0, _pData->getCoords(), false), parser_CalcCutBox(_pData->getRotateAngle(1), dRanges, 1, _pData->getCoords(), false));

                        // --> Entsprechend dem gewuenschten Plotting-Style plotten <--
                        if (sCommand.substr(0,4) == "mesh")
                        {
                            if (_pData->getBars())
                                _graph->Boxs(_mData, _pData->getColorScheme("#").c_str());
                            else
                                _graph->Mesh(_mData, _pData->getColorScheme().c_str());
                        }
                        else if (sCommand.substr(0,4) == "surf")
                        {
                            if (_pData->getColorMask() || _pData->getAlphaMask())
                                k++;
                            if (_pData->getBars())
                                _graph->Boxs(_mData, _pData->getColorScheme().c_str());
                            else if (_pData->getColorMask())
                                _graph->SurfC(_mData, _mMaskData, _pData->getColorScheme().c_str());
                            else if (_pData->getAlphaMask())
                                _graph->SurfA(_mData, _mMaskData, _pData->getColorScheme().c_str());
                            else
                                _graph->Surf(_mData, _pData->getColorScheme().c_str());
                            //cerr << "\"" << _pData->getColorScheme().c_str() << "\" " << strlen(_pData->getColorScheme().c_str()) << endl;
                        }
                        else if (sCommand.substr(0,4) == "cont")
                        {
                            if (_pData->getContLabels())
                            {
                                if (_pData->getContFilled())
                                    _graph->ContF(_mContVec, _mData, _pData->getColorScheme().c_str());
                                _graph->Cont(_mContVec, _mData, _pData->getColorScheme("t").c_str());
                            }
                            else if (_pData->getContProj())
                            {
                                if (_pData->getContFilled())
                                {
                                    _graph->ContF(_mContVec, _mData, _pData->getColorScheme("_").c_str());
                                    _graph->Cont(_mContVec, _mData, "_k");
                                }
                                else
                                    _graph->Cont(_mContVec, _mData, _pData->getColorScheme("_").c_str());
                            }
                            else if (_pData->getContFilled())
                            {
                                _graph->ContF(_mContVec, _mData, _pData->getColorScheme().c_str());
                                _graph->Cont(_mContVec, _mData, "k");
                            }
                            else
                                _graph->Cont(_mContVec, _mData, _pData->getColorScheme().c_str());
                        }
                        else if (sCommand.substr(0,4) == "grad")
                        {
                            if (_pData->getHighRes() || !_option->getbUseDraftMode())
                            {
                                if (_pData->getContFilled() && _pData->getContProj())
                                    _graph->Grad(_mData, _pData->getColorSchemeMedium().c_str(), "value 10");
                                else
                                    _graph->Grad(_mData, _pData->getColorScheme().c_str(), "value 10");
                            }
                            else
                            {
                                if (_pData->getContFilled() && _pData->getContProj())
                                    _graph->Grad(_mData, _pData->getColorSchemeMedium().c_str());
                                else
                                    _graph->Grad(_mData, _pData->getColorScheme().c_str());
                            }
                            if (!(_pData->getContFilled() && _pData->getContProj()))
                                _graph->Dens(_mData, _pData->getColorSchemeLight().c_str());
                        }
                        else if (sCommand.substr(0,4) == "dens")
                        {
                            _graph->Dens(_mData, _pData->getColorScheme().c_str());
                        }
                        else
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
                            throw PLOT_ERROR;
                        }

                        if (_pData->getCutBox()
                            && sCommand.substr(0,4) != "cont"
                            && sCommand.substr(0,4) != "grad"
                            && sCommand.substr(0,4) != "dens")
                            _graph->SetCutBox(mglPoint(0), mglPoint(0));
                        // --> Ggf. Konturlinien ergaenzen <--
                        if (_pData->getContProj() && sCommand.substr(0,4) != "cont")
                        {
                            if (_pData->getContFilled() && sCommand.substr(0,4) != "dens")
                            {
                                _graph->ContF(_mContVec, _mData, _pData->getColorScheme("_").c_str());
                                _graph->Cont(_mContVec, _mData, "_k");
                            }
                            else if (sCommand.substr(0,4) == "dens" && _pData->getContFilled())
                                _graph->Cont(_mContVec, _mData, "_k");
                            else
                                _graph->Cont(_mContVec, _mData, _pData->getColorScheme("_").c_str());
                        }
                        if (_pData->getContLabels() && sCommand.substr(0,4) != "cont" && nFunctions == 1)
                        {
                            _graph->Cont(_mContVec, _mData, ("t"+sContStyles[nStyle]).c_str());
                            if (_pData->getContFilled())
                                _graph->ContF(_mContVec, _mData, _pData->getColorScheme().c_str());
                        }

                        /* --> 2D-Fall und mehr als eine Funktion impliziert, dass wir ein Unterscheidungsmerkmal
                         *     benoetigen: wir ergaenzen Konturlinien und eine Legende <--
                         */
                        if ((nFunctions > 1 && !(_pData->getColorMask() || _pData->getAlphaMask()))
                            || (nFunctions > 2 && (_pData->getColorMask() || _pData->getAlphaMask()))
                            || (nFunctions && nDataPlots))
                        {
                            if (_pData->getContLabels() && sCommand.substr(0,4) != "cont")
                            {
                                _graph->Cont(_mData, ("t"+sContStyles[nStyle]).c_str());
                            }
                            else if (!_pData->getContLabels() && sCommand.substr(0,4) != "cont")
                                _graph->Cont(_mData, sContStyles[nStyle].c_str());
                            nPos = sLabels.find(';');
                            sConvLegends = sLabels.substr(0,nPos);
                            // --> Der String-Parser wertet Ausdruecke wie "Var " + #var aus <--
                            parser_StringParser(sConvLegends, sDummy, *_data, *_parser, *_option, true);
                            sLabels = sLabels.substr(nPos+1);
                            if (sConvLegends != "\"\"")
                            {
                                _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sContStyles[nStyle].c_str());
                                nLegends++;
                            }
                            if (nStyle == nStyleMax-1)
                                nStyle = 0;
                            else
                                nStyle++;
                        }
                    }
                }
                if (_mDataPlots)
                {
                    mglData _mContVec(35);
                    for (int nCont = 0; nCont < 35; nCont++)
                    {
                        _mContVec.a[nCont] = nCont*(dRanges[2][1]-dRanges[2][0])/34.0+dRanges[2][0];
                    }
                    _mContVec.a[17] = (dRanges[2][1]-dRanges[2][0])/2.0 + dRanges[2][0];

                    int nPos = 0;
                    for (int j = 0; j < nDataPlots; j++)
                    {
                        StripSpaces(sDataLabels);

                        if (_pData->getCutBox()
                            && sCommand.substr(0,4) != "cont"
                            && sCommand.substr(0,4) != "grad"
                            && sCommand.substr(0,4) != "dens")
                            _graph->SetCutBox(parser_CalcCutBox(_pData->getRotateAngle(1), dRanges, 0, _pData->getCoords(), false), parser_CalcCutBox(_pData->getRotateAngle(1), dRanges, 1, _pData->getCoords(), false));

                        // --> Entsprechend dem gewuenschten Plotting-Style plotten <--
                        if (sCommand.substr(0,4) == "mesh")
                        {
                            if (_pData->getBars())
                                _graph->Boxs(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorScheme("#").c_str());
                            else
                                _graph->Mesh(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorScheme().c_str());
                        }
                        else if (sCommand.substr(0,4) == "surf")
                        {
                            if (_pData->getBars())
                                _graph->Boxs(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorScheme().c_str());
                            else if (_pData->getColorMask() && j < nDataPlots-1)
                                _graph->SurfC(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _mDataPlots[j+1][2], _pData->getColorScheme().c_str());
                            else if (_pData->getAlphaMask() && j < nDataPlots-1)
                                _graph->SurfA(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _mDataPlots[j+1][2], _pData->getColorScheme().c_str());
                            else
                                _graph->Surf(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorScheme().c_str());
                            if (_pData->getColorMask() || _pData->getAlphaMask())
                                j++;

                            //cerr << "\"" << _pData->getColorScheme().c_str() << "\" " << strlen(_pData->getColorScheme().c_str()) << endl;
                        }
                        else if (sCommand.substr(0,4) == "cont")
                        {
                            if (_pData->getContLabels())
                            {
                                if (_pData->getContFilled())
                                    _graph->ContF(_mContVec, _mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorScheme().c_str());
                                _graph->Cont(_mContVec, _mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorScheme("t").c_str());
                            }
                            else if (_pData->getContProj())
                            {
                                if (_pData->getContFilled())
                                {
                                    _graph->ContF(_mContVec, _mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorScheme("_").c_str());
                                    _graph->Cont(_mContVec, _mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], "_k");
                                }
                                else
                                    _graph->Cont(_mContVec, _mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorScheme("_").c_str());
                            }
                            else if (_pData->getContFilled())
                            {
                                _graph->ContF(_mContVec, _mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorScheme().c_str());
                                _graph->Cont(_mContVec, _mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], "k");
                            }
                            else
                                _graph->Cont(_mContVec, _mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorScheme().c_str());
                        }
                        else if (sCommand.substr(0,4) == "grad")
                        {
                            if (_pData->getHighRes() || !_option->getbUseDraftMode())
                            {
                                if (_pData->getContFilled() && _pData->getContProj())
                                    _graph->Grad(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorSchemeMedium().c_str(), "value 10");
                                else
                                    _graph->Grad(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorScheme().c_str(), "value 10");
                            }
                            else
                            {
                                if (_pData->getContFilled() && _pData->getContProj())
                                    _graph->Grad(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorSchemeMedium().c_str());
                                else
                                    _graph->Grad(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorScheme().c_str());
                            }
                            if (!(_pData->getContFilled() && _pData->getContProj()))
                                _graph->Dens(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorSchemeLight().c_str());
                        }
                        else if (sCommand.substr(0,4) == "dens")
                        {
                            _graph->Dens(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorScheme().c_str());
                        }
                        else
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
                            throw PLOT_ERROR;
                        }

                        if (_pData->getCutBox()
                            && sCommand.substr(0,4) != "cont"
                            && sCommand.substr(0,4) != "grad"
                            && sCommand.substr(0,4) != "dens")
                            _graph->SetCutBox(mglPoint(0), mglPoint(0));

                        // --> Ggf. Konturlinien ergaenzen <--
                        if (_pData->getContProj() && sCommand.substr(0,4) != "cont")
                        {
                            if (_pData->getContFilled() && sCommand.substr(0,4) != "dens")
                            {
                                _graph->ContF(_mContVec, _mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorScheme("_").c_str());
                                _graph->Cont(_mContVec, _mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], "_k");
                            }
                            else if (sCommand.substr(0,4) == "dens" && _pData->getContFilled())
                                _graph->Cont(_mContVec, _mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], "_k");
                            else
                                _graph->Cont(_mContVec, _mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorScheme("_").c_str());
                        }
                        if (_pData->getContLabels() && sCommand.substr(0,4) != "cont" && nDataPlots == 1)
                        {
                            _graph->Cont(_mContVec, _mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], ("t"+sContStyles[nStyle]).c_str());
                            if (_pData->getContFilled())
                                _graph->ContF(_mContVec, _mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorScheme().c_str());
                        }

                        if ((nDataPlots > 1 && !(_pData->getColorMask() || _pData->getAlphaMask()))
                            || (nDataPlots > 2 && (_pData->getColorMask() || _pData->getAlphaMask()))
                            || (nFunctions && nDataPlots))
                        {
                            if (_pData->getContLabels() && sCommand.substr(0,4) != "cont")
                            {
                                _graph->Cont(_mDataPlots[j][2], ("t"+sContStyles[nStyle]).c_str());
                            }
                            else if (!_pData->getContLabels() && sCommand.substr(0,4) != "cont")
                                _graph->Cont(_mDataPlots[j][2], sContStyles[nStyle].c_str());
                            nPos = sDataLabels.find(';');
                            sConvLegends = sDataLabels.substr(0,nPos);
                            // --> Der String-Parser wertet Ausdruecke wie "Var " + #var aus <--
                            parser_StringParser(sConvLegends, sDummy, *_data, *_parser, *_option, true);
                            sDataLabels = sDataLabels.substr(nPos+1);
                            if (sConvLegends != "\"\"")
                            {
                                _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sContStyles[nStyle].c_str());
                                nLegends++;
                            }
                            if (nStyle == nStyleMax-1)
                                nStyle = 0;
                            else
                                nStyle++;
                        }

                    }

                }
                // --> Position der Legende etwas aendern <--
                if (nFunctions > 1 && nLegends && !_pData->getSchematic() && nPlotCompose+1 == vPlotCompose.size())
                {
                    _graph->Legend(1.35,1.2);
                }
            }
            else if (sCommand != "plot3d" && !b3D && !b3DVect && !b2DVect && !bDraw3D && !bDraw)      // Standardplot
            {
                if (sFunc != "<<empty>>")
                {
                    mglData _mData(nSamples);
                    int nPos = 0;
                    for (int j = 0; j < nFunctions; j++)
                    {
                        StripSpaces(sLabels);
                        for (long int i = 0; i < nSamples; i++)
                        {
                            _mData.a[i] = _pData->getData(i,j);
                        }
                        if (!_pData->getArea() && !_pData->getRegion())
                            _graph->Plot(_mData, sLineStyles[nStyle].c_str());
                        else if (_pData->getRegion() && j+1 < nFunctions)
                        {
                            mglData _mData2(nSamples);
                            for (long int i = 0; i < nSamples; i++)
                                _mData2.a[i] = _pData->getData(i,j+1);

                            if (nStyle == nStyleMax-1)
                                _graph->Region(_mData, _mData2, ("{"+_pData->getColors().substr(nStyle,1) + "7}{" + _pData->getColors().substr(0,1)+"7}").c_str());
                            else
                                _graph->Region(_mData, _mData2, ("{"+_pData->getColors().substr(nStyle,1) + "7}{" + _pData->getColors().substr(nStyle+1,1)+"7}").c_str());
                            _graph->Plot(_mData, sLineStyles[nStyle].c_str());
                            if (nStyle == nStyleMax-1)
                                _graph->Plot(_mData2, sLineStyles[0].c_str());
                            else
                                _graph->Plot(_mData2, sLineStyles[nStyle+1].c_str());
                            for (int k = 0; k < 2; k++)
                            {
                                nPos = sLabels.find(';');
                                sConvLegends = sLabels.substr(0,nPos) + " -nq";
                                parser_StringParser(sConvLegends, sDummy, *_data, *_parser, *_option, true);
                                sConvLegends = "\"" + sConvLegends + "\"";
                                sLabels = sLabels.substr(nPos+1);
                                if (sConvLegends != "\"\"")
                                {
                                    _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sLineStyles[nStyle].c_str());
                                    nLegends++;
                                }
                                /*if (_pData->getDrawPoints())
                                    _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sConnectedDataPlotStyles[nStyle].c_str());
                                else
                                    _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sColorStyles[nStyle].c_str());*/

                                if (nStyle == nStyleMax-1)
                                    nStyle = 0;
                                else
                                    nStyle++;
                            }
                            j++;
                            continue;
                        }
                        else if (_pData->getArea() || _pData->getRegion())
                            _graph->Area(_mData, (sLineStyles[nStyle] + "{" + _pData->getColors()[nStyle] + "9}").c_str());
                        /*if (_pData->getDrawPoints())
                            _graph->Plot(_mData, sConnectedDataPlotStyles[nStyle].c_str());
                        else
                            _graph->Plot(_mData, sColorStyles[nStyle].c_str());*/
                        nPos = sLabels.find(';');
                        sConvLegends = sLabels.substr(0,nPos) + " -nq";
                        parser_StringParser(sConvLegends, sDummy, *_data, *_parser, *_option, true);
                        sConvLegends = "\"" + sConvLegends + "\"";
                        sLabels = sLabels.substr(nPos+1);
                        if (sConvLegends != "\"\"")
                        {
                            _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sLineStyles[nStyle].c_str());
                            nLegends++;
                        }
                        /*if (_pData->getDrawPoints())
                            _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sConnectedDataPlotStyles[nStyle].c_str());
                        else
                            _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sColorStyles[nStyle].c_str());*/
                        if (nStyle == nStyleMax-1)
                            nStyle = 0;
                        else
                            nStyle++;
                    }
                }
                if (_mDataPlots)
                {
                    int nPos = 0;
                    for (int j = 0; j < nDataPlots; j++)
                    {
                        StripSpaces(sDataLabels);
                        if (!_pData->getxError() && !_pData->getyError())
                        {
                            /* --> Falls die Option "interpolate" aktiv ist und der Datensatz aus ausreichend Datenpunkten besteht,
                             *     stellen wir die Datenpunkte als Kurve dar. Dabei werden diese jedoch nicht geordnet! <--
                             */
                            /*if (_pData->getInterpolate() && _mDataPlots[j][0].GetNx() < nSamples)
                                _mDataPlots[j][0] = _mDataPlots[j][0].Resize(nSamples);*/
                            if (_pData->getInterpolate() && _mDataPlots[j][0].GetNx() >= nSamples)
                            {
                                if (!_pData->getArea() && !_pData->getBars() && !_pData->getRegion())
                                    _graph->Plot(_mDataPlots[j][0], _mDataPlots[j][1], sLineStyles[nStyle].c_str());
                                else if (_pData->getBars() && !_pData->getArea() && !_pData->getRegion())
                                    _graph->Bars(_mDataPlots[j][0], _mDataPlots[j][1], (sLineStyles[nStyle]+"^").c_str());
                                else if (_pData->getRegion() && j+1 < nDataPlots)
                                {
                                    if (nStyle == nStyleMax-1)
                                        _graph->Region(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j+1][0], _mDataPlots[j+1][1], ("{"+_pData->getColors().substr(nStyle,1) +"7}{"+ _pData->getColors().substr(0,1)+"7}").c_str());
                                    else
                                        _graph->Region(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j+1][0], _mDataPlots[j+1][1], ("{"+_pData->getColors().substr(nStyle,1) +"7}{"+ _pData->getColors().substr(nStyle+1,1)+"7}").c_str());
                                    _graph->Plot(_mDataPlots[j][0], _mDataPlots[j][1], sLineStyles[nStyle].c_str());
                                    j++;
                                    if (nStyle == nStyleMax-1)
                                        _graph->Plot(_mDataPlots[j][0], _mDataPlots[j][1], sLineStyles[0].c_str());
                                    else
                                        _graph->Plot(_mDataPlots[j][0], _mDataPlots[j][1], sLineStyles[nStyle+1].c_str());
                                    for (int k = 0; k < 2; k++)
                                    {
                                        nPos = sDataLabels.find(';');
                                        sConvLegends = sDataLabels.substr(0,nPos);
                                        parser_StringParser(sConvLegends, sDummy, *_data, *_parser, *_option, true);
                                        sDataLabels = sDataLabels.substr(nPos+1);
                                        if (sConvLegends != "\"\"")
                                        {
                                            nLegends++;
                                            _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sLineStyles[nStyle].c_str());
                                        }
                                    if (nStyle == nStyleMax-1)
                                        nStyle = 0;
                                    else
                                        nStyle++;

                                    }
                                    continue;
                                }
                                else if (_pData->getArea() || _pData->getRegion())
                                    _graph->Area(_mDataPlots[j][0], _mDataPlots[j][1], (sLineStyles[nStyle] + "{" + _pData->getColors()[nStyle] + "9}").c_str());
                            }
                            else if (_pData->getConnectPoints() || (_pData->getInterpolate() && _mDataPlots[j][0].GetNx() >= 0.9 * nSamples))
                            {
                                if (!_pData->getArea() && !_pData->getBars())
                                    _graph->Plot(_mDataPlots[j][0], _mDataPlots[j][1], sConPointStyles[nStyle].c_str());
                                else if (_pData->getBars() && !_pData->getArea())
                                    _graph->Bars(_mDataPlots[j][0], _mDataPlots[j][1], (sLineStyles[nStyle]+"^").c_str());
                                else
                                    _graph->Area(_mDataPlots[j][0], _mDataPlots[j][1], (sLineStyles[nStyle] + "{" + _pData->getColors()[nStyle] + "9}").c_str());
                            }
                            else
                            {
                                if (!_pData->getArea() && !_pData->getBars())
                                    _graph->Plot(_mDataPlots[j][0], _mDataPlots[j][1], sPointStyles[nStyle].c_str());
                                else if (_pData->getBars() && !_pData->getArea())
                                    _graph->Bars(_mDataPlots[j][0], _mDataPlots[j][1], (sLineStyles[nStyle]+"^").c_str());
                                else
                                    _graph->Stem(_mDataPlots[j][0], _mDataPlots[j][1], sConPointStyles[nStyle].c_str());
                            }
                        }
                        else if (_pData->getxError() && _pData->getyError() && nDataDim[j] >= 4)
                            _graph->Error(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _mDataPlots[j][3], sPointStyles[nStyle].c_str());
                        else if ((_pData->getyError() || _pData->getxError()) && nDataDim[j] >= 3)
                        {
                            if (_pData->getyError() && !_pData->getxError())
                            {
                                _graph->Error(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], sPointStyles[nStyle].c_str());
                            }
                            else if (_pData->getyError() && _pData->getxError())
                            {
                                _graph->Error(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _mDataPlots[j][2], sPointStyles[nStyle].c_str());
                            }
                            else
                            {
                                mglData _mDataTemp;
                                _mDataTemp.Create(_mDataPlots[j][2].GetNx());
                                _mDataTemp.Fill(0.0, NAN, 'x');
                                _graph->Error(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _mDataTemp, sPointStyles[nStyle].c_str());
                            }
                        }
                        nPos = sDataLabels.find(';');
                        sConvLegends = sDataLabels.substr(0,nPos);
                        parser_StringParser(sConvLegends, sDummy, *_data, *_parser, *_option, true);
                        sDataLabels = sDataLabels.substr(nPos+1);
                        if (sConvLegends != "\"\"")
                        {
                            nLegends++;
                            if (!_pData->getxError() && !_pData->getyError())
                            {
                                if ((_pData->getInterpolate() && _mDataPlots[j][0].GetNx() >= nSamples) || _pData->getBars())
                                    _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sLineStyles[nStyle].c_str());
                                else if (_pData->getConnectPoints() || (_pData->getInterpolate() && _mDataPlots[j][0].GetNx() >= 0.9 * nSamples))
                                    _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sConPointStyles[nStyle].c_str());
                                else
                                    _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sPointStyles[nStyle].c_str());
                            }
                            else
                                _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sPointStyles[nStyle].c_str());
                        }
                        if (nStyle == nStyleMax-1)
                            nStyle = 0;
                        else
                            nStyle++;
                    }
                }
                for (unsigned int i = 0; i < 3; i++)
                {
                    if (_pData->getHLines(i).sDesc.length())
                    {
                        _graph->Line(mglPoint(dRanges[0][0], _pData->getHLines(i).dPos), mglPoint(dRanges[0][1], _pData->getHLines(i).dPos), "k=", 100);
                        if (!i || i > 1)
                        {
                            _graph->Puts(dRanges[0][0]+0.03*fabs(dRanges[0][0]-dRanges[0][1]), _pData->getHLines(i).dPos+0.01*fabs(dRanges[1][1]-dRanges[1][0]), fromSystemCodePage(_pData->getHLines(i).sDesc).c_str(), ":kL");
                        }
                        else
                        {
                            _graph->Puts(dRanges[0][0]+0.03*fabs(dRanges[0][0]-dRanges[0][1]), _pData->getHLines(i).dPos-0.04*fabs(dRanges[1][1]-dRanges[1][0]), fromSystemCodePage(_pData->getHLines(i).sDesc).c_str(), ":kL");
                        }
                    }
                    if (_pData->getVLines(i).sDesc.length())
                    {
                        _graph->Line(mglPoint(_pData->getVLines(i).dPos, dRanges[1][0]), mglPoint(_pData->getVLines(i).dPos, dRanges[1][1]), "k=");
                        _graph->Puts(mglPoint(_pData->getVLines(i).dPos-0.01*fabs(dRanges[0][0]-dRanges[0][1]), dRanges[1][0]+0.05*fabs(dRanges[1][0]-dRanges[1][1])), mglPoint(_pData->getVLines(i).dPos-0.01*fabs(dRanges[0][0]-dRanges[0][1]), dRanges[1][1]), fromSystemCodePage(_pData->getVLines(i).sDesc).c_str(), ":kL");
                    }
                }
                if (nLegends && !_pData->getSchematic() && nPlotCompose+1 == vPlotCompose.size())
                    _graph->Legend();
            }
            else if (b3D)   // 3D-Plot
            {
                if (sFunc != "<<empty>>")
                {
                    if (_option->getbDebug())
                        cerr << "|-> DEBUG: 3D-Data-Copy" << endl;
                    mglData _mData(nSamples, nSamples, nSamples);
                    mglData _mContVec(15);
                    double dMin = _pData->getMin();
                    double dMax = _pData->getMax();
                    if (!isnan(_pData->getColorRange()))
                    {
                        dMin = _pData->getColorRange();
                        dMax = _pData->getColorRange(1);
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

                    for (long int i = 0; i < nSamples; i++)
                    {
                        for (long int j = 0; j < nSamples; j++)
                        {
                            for (long int k = 0; k < nSamples; k++)
                            {
                                // --> Das Array im mglData-Objekt ist nur 1D vorhanden <--
                                _mData.a[i + nSamples*j + nSamples*nSamples*k] = _pData->getData(i,j,k);
                            }
                        }
                    }
                    if (_option->getbDebug())
                        cerr << "|-> DEBUG: generating 3D-Plot..." << endl;

                    if (_pData->getCutBox()
                        && sCommand.substr(0,4) != "cont"
                        && sCommand.substr(0,4) != "grad"
                        && (sCommand.substr(0,4) != "dens" || (sCommand.substr(0,4) == "dens" && _pData->getCloudPlot())))
                    {
                        //_graph->SetCutBox(mglPoint(0,0,-1), mglPoint(1,1,1.1));
                        _graph->SetCutBox(parser_CalcCutBox(_pData->getRotateAngle(1), dRanges, 0, _pData->getCoords(), true), parser_CalcCutBox(_pData->getRotateAngle(1), dRanges, 1, _pData->getCoords(), true));
                    }
                    // --> Entsprechend dem gewuenschten Plotting-Style plotten <--
                    if (sCommand.substr(0,4) == "mesh")
                        _graph->Surf3(_mData, _pData->getColorScheme("#").c_str(), "value 11");
                    else if (sCommand.substr(0,4) == "surf" && !_pData->getTransparency())
                        _graph->Surf3(_mData, _pData->getColorScheme().c_str(), "value 11");
                    else if (sCommand.substr(0,4) == "surf" && _pData->getTransparency())
                        _graph->Surf3A(_mData, _mData, _pData->getColorScheme().c_str(), "value 11");
                    else if (sCommand.substr(0,4) == "cont")
                    {
                        /*if (_pData->getContLabels())
                        {
                            if (_pData->getContFilled())
                                _graph->ContF(_mContVec, _mData);
                            _graph->Cont(_mContVec, _mData, "t");
                        }
                        else*/
                        if (_pData->getContProj())
                        {
                            if (_pData->getContFilled())
                            {
                                _graph->ContFX(_mContVec, _mData.Sum("x"), _pData->getColorScheme().c_str(), parser_getProjBackground(_pData->getRotateAngle(1), dRanges, 0));
                                _graph->ContFY(_mContVec, _mData.Sum("y"), _pData->getColorScheme().c_str(), parser_getProjBackground(_pData->getRotateAngle(1), dRanges, 1));
                                _graph->ContFZ(_mContVec, _mData.Sum("z"), _pData->getColorScheme().c_str(), dRanges[2][0]);
                                _graph->ContX(_mContVec, _mData.Sum("x"), "k", parser_getProjBackground(_pData->getRotateAngle(1), dRanges, 0));
                                _graph->ContY(_mContVec, _mData.Sum("y"), "k", parser_getProjBackground(_pData->getRotateAngle(1), dRanges, 1));
                                _graph->ContZ(_mContVec, _mData.Sum("z"), "k", dRanges[2][0]);
                            }
                            else
                            {
                                _graph->ContX(_mContVec, _mData.Sum("x"), _pData->getColorScheme().c_str(), parser_getProjBackground(_pData->getRotateAngle(1), dRanges, 0));
                                _graph->ContY(_mContVec, _mData.Sum("y"), _pData->getColorScheme().c_str(), parser_getProjBackground(_pData->getRotateAngle(1), dRanges, 1));
                                _graph->ContZ(_mContVec, _mData.Sum("z"), _pData->getColorScheme().c_str(), dRanges[2][0]);
                            }
                        }
                        else if (_pData->getContFilled())
                        {
                            _graph->ContF3(_mContVec, _mData, _pData->getColorScheme("x").c_str());
                            _graph->ContF3(_mContVec, _mData, _pData->getColorScheme().c_str());
                            _graph->ContF3(_mContVec, _mData, _pData->getColorScheme("z").c_str());
                            _graph->Cont3(_mContVec, _mData, "kx");
                            _graph->Cont3(_mContVec, _mData, "k");
                            _graph->Cont3(_mContVec, _mData, "kz");
                        }
                        else
                        {
                            _graph->Cont3(_mContVec, _mData, _pData->getColorScheme("x").c_str());
                            _graph->Cont3(_mContVec, _mData, _pData->getColorScheme().c_str());
                            _graph->Cont3(_mContVec, _mData, _pData->getColorScheme("z").c_str());
                        }
                    }
                    else if (sCommand.substr(0,4) == "grad")
                    {
                        mglData x(nSamples), y(nSamples), z(nSamples);
                        for (int n = 0; n < nSamples; n++)
                        {
                            x.a[n] = n*(dRanges[0][1]-dRanges[0][0])/(double)(nSamples-1)+dRanges[0][0];
                            y.a[n] = n*(dRanges[1][1]-dRanges[1][0])/(double)(nSamples-1)+dRanges[1][0];
                            z.a[n] = n*(dRanges[2][1]-dRanges[2][0])/(double)(nSamples-1)+dRanges[2][0];
                        }
                        if (_pData->getHighRes() || !_option->getbUseDraftMode())
                        {
                            if (_pData->getContFilled() && _pData->getContProj())
                                _graph->Grad(x,y,z,_mData, _pData->getColorSchemeMedium().c_str(), "value 10");
                            else
                                _graph->Grad(x,y,z,_mData, _pData->getColorScheme().c_str(), "value 10");
                        }
                        else
                        {
                            if (_pData->getContFilled() && _pData->getContProj())
                                _graph->Grad(x,y,z,_mData, _pData->getColorSchemeMedium().c_str());
                            else
                                _graph->Grad(x,y,z,_mData, _pData->getColorScheme().c_str());
                        }
                        if (!(_pData->getContProj()))
                        {
                            _graph->Dens3(_mData, _pData->getColorSchemeLight("x").c_str());
                            _graph->Dens3(_mData, _pData->getColorSchemeLight().c_str());
                            _graph->Dens3(_mData, _pData->getColorSchemeLight("z").c_str());
                        }
                    }
                    else if (sCommand.substr(0,4) == "dens")
                    {
                        if (!(_pData->getContFilled() && _pData->getContProj()) && !_pData->getCloudPlot())
                        {
                            _graph->Dens3(_mData, _pData->getColorScheme("x").c_str());
                            _graph->Dens3(_mData, _pData->getColorScheme().c_str());
                            _graph->Dens3(_mData, _pData->getColorScheme("z").c_str());
                        }
                        else if (_pData->getCloudPlot() && !(_pData->getContFilled() && _pData->getContProj()))
                            _graph->Cloud(_mData, _pData->getColorScheme().c_str());
                    }
                    else
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
                        throw PLOT_ERROR;
                    }

                    if (_pData->getCutBox()
                        && sCommand.substr(0,4) != "cont"
                        && sCommand.substr(0,4) != "grad"
                        && (sCommand.substr(0,4) != "dens" || (sCommand.substr(0,4) == "dens" && _pData->getCloudPlot())))
                    {
                        _graph->SetCutBox(mglPoint(0), mglPoint(0));
                    }

                    // --> Ggf. Konturlinien ergaenzen <--
                    if (_pData->getContProj() && sCommand.substr(0,4) != "cont")
                    {
                        if (_pData->getContFilled() && (sCommand.substr(0,4) != "dens" && sCommand.substr(0,4) != "grad"))
                        {
                            _graph->ContFX(_mContVec, _mData.Sum("x"), _pData->getColorScheme().c_str(), parser_getProjBackground(_pData->getRotateAngle(1), dRanges, 0));
                            _graph->ContFY(_mContVec, _mData.Sum("y"), _pData->getColorScheme().c_str(), parser_getProjBackground(_pData->getRotateAngle(1), dRanges, 1));
                            _graph->ContFZ(_mContVec, _mData.Sum("z"), _pData->getColorScheme().c_str(), dRanges[2][0]);
                            _graph->ContX(_mContVec, _mData.Sum("x"), "k", parser_getProjBackground(_pData->getRotateAngle(1), dRanges, 0));
                            _graph->ContY(_mContVec, _mData.Sum("y"), "k", parser_getProjBackground(_pData->getRotateAngle(1), dRanges, 1));
                            _graph->ContZ(_mContVec, _mData.Sum("z"), "k", dRanges[2][0]);
                        }
                        else if ((sCommand.substr(0,4) == "dens" || sCommand.substr(0,4) == "grad") && _pData->getContFilled())
                        {
                            if (sCommand == "dens")
                            {
                                _graph->DensX(_mData.Sum("x"), _pData->getColorScheme().c_str(), parser_getProjBackground(_pData->getRotateAngle(1), dRanges, 0));
                                _graph->DensY(_mData.Sum("y"), _pData->getColorScheme().c_str(), parser_getProjBackground(_pData->getRotateAngle(1), dRanges, 1));
                                _graph->DensZ(_mData.Sum("z"), _pData->getColorScheme().c_str(), dRanges[2][0]);
                            }
                            else
                            {
                                _graph->DensX(_mData.Sum("x"), _pData->getColorSchemeLight().c_str(), parser_getProjBackground(_pData->getRotateAngle(1), dRanges, 0));
                                _graph->DensY(_mData.Sum("y"), _pData->getColorSchemeLight().c_str(), parser_getProjBackground(_pData->getRotateAngle(1), dRanges, 1));
                                _graph->DensZ(_mData.Sum("z"), _pData->getColorSchemeLight().c_str(), dRanges[2][0]);
                            }
                            _graph->ContX(_mContVec, _mData.Sum("x"), "k", parser_getProjBackground(_pData->getRotateAngle(1), dRanges, 0));
                            _graph->ContY(_mContVec, _mData.Sum("y"), "k", parser_getProjBackground(_pData->getRotateAngle(1), dRanges, 1));
                            _graph->ContZ(_mContVec, _mData.Sum("z"), "k", dRanges[2][0]);
                        }
                        else
                        {
                            _graph->ContX(_mContVec, _mData.Sum("x"), _pData->getColorScheme().c_str(), parser_getProjBackground(_pData->getRotateAngle(1), dRanges, 0));
                            _graph->ContY(_mContVec, _mData.Sum("y"), _pData->getColorScheme().c_str(), parser_getProjBackground(_pData->getRotateAngle(1), dRanges, 1));
                            _graph->ContZ(_mContVec, _mData.Sum("z"), _pData->getColorScheme().c_str(), dRanges[2][0]);
                        }
                    }
                }
            }
            else if (b3DVect)   // 3D-Vektorplot
            {
                if (sFunc != "<<empty>>")
                {
                    if (_option->getbDebug())
                        cerr << "|-> DEBUG: 3D-Data-Copy" << endl;
                    mglData _mData_x(nSamples, nSamples, nSamples);
                    mglData _mData_y(nSamples, nSamples, nSamples);
                    mglData _mData_z(nSamples, nSamples, nSamples);

                    StripSpaces(sLabels);

                    for (long int i = 0; i < nSamples; i++)
                    {
                        for (long int j = 0; j < nSamples; j++)
                        {
                            for (long int k = 0; k < nSamples; k++)
                            {
                                // --> Das Array im mglData-Objekt ist nur 1D vorhanden <--
                                _mData_x.a[i + nSamples*j + nSamples*nSamples*k] = _pData->getData(i,j,3*k);
                                _mData_y.a[i + nSamples*j + nSamples*nSamples*k] = _pData->getData(i,j,3*k+1);
                                _mData_z.a[i + nSamples*j + nSamples*nSamples*k] = _pData->getData(i,j,3*k+2);
                            }
                        }
                    }
                    if (_option->getbDebug())
                        cerr << "|-> DEBUG: generating 3D-Plot..." << endl;

                    if (_pData->getCutBox())
                    {
                        //_graph->SetCutBox(mglPoint(0,0,-1), mglPoint(1,1,1.1));
                        _graph->SetCutBox(parser_CalcCutBox(_pData->getRotateAngle(1), dRanges, 0, _pData->getCoords(), true), parser_CalcCutBox(_pData->getRotateAngle(1), dRanges, 1, _pData->getCoords(), true));
                    }
                    // --> Entsprechend dem gewuenschten Plotting-Style plotten <--
                    if (_pData->getFlow())
                        _graph->Flow(_mData_x, _mData_y, _mData_z, _pData->getColorScheme("v").c_str());
                    else if (_pData->getPipe())
                        _graph->Pipe(_mData_x, _mData_y, _mData_z, _pData->getColorScheme().c_str());
                    else if (_pData->getFixedLength())
                        _graph->Vect(_mData_x, _mData_y, _mData_z, _pData->getColorScheme("f").c_str());
                    else if (!_pData->getFixedLength())
                        _graph->Vect(_mData_x, _mData_y, _mData_z, _pData->getColorScheme().c_str());
                    else
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
                        throw PLOT_ERROR;
                    }

                    if (_pData->getCutBox())
                    {
                        _graph->SetCutBox(mglPoint(0), mglPoint(0));
                    }

                }
            }
            else if (b2DVect)   // 2D-Vektorplot
            {
                if (sFunc != "<<empty>>")
                {
                    if (_option->getbDebug())
                        cerr << "|-> DEBUG: 3D-Data-Copy" << endl;
                    mglData _mData_x(nSamples, nSamples, nSamples);
                    mglData _mData_y(nSamples, nSamples, nSamples);

                    StripSpaces(sLabels);

                    for (long int i = 0; i < nSamples; i++)
                    {
                        for (long int j = 0; j < nSamples; j++)
                        {
                            // --> Das Array im mglData-Objekt ist nur 1D vorhanden <--
                            _mData_x.a[i + nSamples*j] = _pData->getData(i,j,0);
                            _mData_y.a[i + nSamples*j] = _pData->getData(i,j,1);
                        }
                    }
                    if (_option->getbDebug())
                        cerr << "|-> DEBUG: generating 3D-Plot..." << endl;

                    // --> Entsprechend dem gewuenschten Plotting-Style plotten <--
                    if (_pData->getFlow())
                        _graph->Flow(_mData_x, _mData_y, _pData->getColorScheme("v").c_str());
                    else if (_pData->getPipe())
                        _graph->Pipe(_mData_x, _mData_y, _pData->getColorScheme().c_str());
                    else if (_pData->getFixedLength())
                        _graph->Vect(_mData_x, _mData_y, _pData->getColorScheme("f").c_str());
                    else if (!_pData->getFixedLength())
                        _graph->Vect(_mData_x, _mData_y, _pData->getColorScheme().c_str());
                    else
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
                        throw PLOT_ERROR;
                    }

                }
            }
            else if (bDraw)
            {
                string sStyle;
                string sTextString;
                for (unsigned int v = 0; v < vDrawVector.size(); v++)
                {
                    sStyle = "k";
                    sTextString = "";
                    //cerr << vDrawVector[v] << endl;
                    if (containsStrings(vDrawVector[v]) || _data->containsStringVars(vDrawVector[v]))
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
                        //sStyle = vDrawVector[v].substr(vDrawVector[v].rfind(',')+1);
                        sStyle = sStyle.substr(0,sStyle.rfind(')')) + " -nq";
                        parser_StringParser(sStyle, sDummy, *_data, *_parser, *_option, true);
                        //vDrawVector[v].erase(vDrawVector[v].rfind(','));
                    }
                    if (containsStrings(vDrawVector[v]) || _data->containsStringVars(vDrawVector[v]))
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
                        //sTextString = vDrawVector[v].substr(vDrawVector[v].rfind(',')+1);
                        sTextString += " -nq";
                        parser_StringParser(sTextString, sDummy, *_data, *_parser, *_option, true);
                        //vDrawVector[v].erase(vDrawVector[v].rfind(','));
                    }
                    //cerr << vDrawVector[v] << endl;
                    if (vDrawVector[v][vDrawVector[v].length()-1] == ')')
                        _parser->SetExpr(vDrawVector[v].substr(vDrawVector[v].find('(')+1,vDrawVector[v].rfind(')')-vDrawVector[v].find('(')-1));
                    else
                        _parser->SetExpr(vDrawVector[v].substr(vDrawVector[v].find('(')+1));
                    vResults = _parser->Eval(nFunctions);
                    //cerr << nFunctions << endl;
                    //cerr << vDrawVector[v].substr(0,5) << endl;

                    //trace(x,y,x,y,STYLE), face(x,y,x,y,x,y,STYLE), sphere(x,y,r,STYLE), circle(x,y,r,STYLE), cuboid(x,y,x,y,STYLE)
                    if (vDrawVector[v].substr(0,6) == "trace(")
                    {
                        if (nFunctions < 4)
                            continue;
                        _graph->Line(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), sStyle.c_str());
                    }
                    else if (vDrawVector[v].substr(0,5) == "face(" || vDrawVector[v].substr(0,7) == "cuboid(")
                    {
                        if (nFunctions < 4)
                            continue;
                        if (nFunctions < 6)
                            _graph->Face(mglPoint(vResults[2]-vResults[3]+vResults[1], vResults[3]+vResults[2]-vResults[0]),
                                        mglPoint(vResults[2],vResults[3]),
                                        mglPoint(vResults[0]-vResults[3]+vResults[1], vResults[1]+vResults[2]-vResults[0]),
                                        mglPoint(vResults[0],vResults[1]),
                                        sStyle.c_str());
                        else if (nFunctions < 8)
                            _graph->Face(mglPoint(vResults[4],vResults[5]),
                                        mglPoint(vResults[2],vResults[3]),
                                        mglPoint(vResults[0]+vResults[4]-vResults[2], vResults[1]+vResults[5]-vResults[3]),
                                        mglPoint(vResults[0],vResults[1]),
                                        sStyle.c_str());
                        else
                            _graph->Face(mglPoint(vResults[4],vResults[5]),
                                        mglPoint(vResults[2],vResults[3]),
                                        mglPoint(vResults[6],vResults[7]),
                                        mglPoint(vResults[0],vResults[1]),
                                        sStyle.c_str());
                    }
                    else if (vDrawVector[v].substr(0,9) == "triangle(")
                    {
                        if (nFunctions < 4)
                            continue;
                        double c = hypot(vResults[2]-vResults[0],vResults[3]-vResults[1])/2.0*sqrt(3)/hypot(vResults[2],vResults[3]);
                        if (nFunctions < 6)
                            _graph->Face(mglPoint((-vResults[0]+vResults[2])/2.0-c*vResults[3], (-vResults[1]+vResults[3])/2.0+c*vResults[2]),
                                        mglPoint(vResults[2],vResults[3]),
                                        mglPoint((-vResults[0]+vResults[2])/2.0-c*vResults[3], (-vResults[1]+vResults[3])/2.0+c*vResults[2]),
                                        mglPoint(vResults[0],vResults[1]),
                                        sStyle.c_str());
                        else
                            _graph->Face(mglPoint(vResults[4],vResults[5]),
                                        mglPoint(vResults[2],vResults[3]),
                                        mglPoint(vResults[4],vResults[5]),
                                        mglPoint(vResults[0],vResults[1]),
                                        sStyle.c_str());
                    }
                    else if (vDrawVector[v].substr(0,7) == "sphere(")
                    {
                        if (nFunctions < 3)
                            continue;
                        _graph->Sphere(mglPoint(vResults[0], vResults[1]), vResults[2], sStyle.c_str());
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
                        _graph->Drop(mglPoint(vResults[0], vResults[1]),
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
                        _graph->Circle(mglPoint(vResults[0], vResults[1]), vResults[2], sStyle.c_str());
                    }
                    else if (vDrawVector[v].substr(0,4) == "arc(")
                    {
                        if (nFunctions < 5)
                            continue;
                        _graph->Arc(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), vResults[4], sStyle.c_str());
                    }
                    else if (vDrawVector[v].substr(0,6) == "point(")
                    {
                        if (nFunctions < 2)
                            continue;
                        _graph->Mark(mglPoint(vResults[0], vResults[1]), sStyle.c_str());
                    }
                    else if (vDrawVector[v].substr(0,6) == "curve(")
                    {
                        if (nFunctions < 8)
                            continue;
                        _graph->Curve(mglPoint(vResults[0], vResults[1]),
                                    mglPoint(vResults[2], vResults[3]),
                                    mglPoint(vResults[4], vResults[5]),
                                    mglPoint(vResults[6], vResults[7]), sStyle.c_str());
                    }
                    else if (vDrawVector[v].substr(0,8) == "ellipse(")
                    {
                        if (nFunctions < 5)
                            continue;
                        _graph->Ellipse(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), vResults[4], sStyle.c_str());
                    }
                    else if (vDrawVector[v].substr(0,5) == "text(")
                    {
                        if (!sTextString.length())
                        {
                            sTextString = sStyle;
                            sStyle = "k";
                        }
                        if (nFunctions >= 4)
                            _graph->Puts(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), sTextString.c_str(), sStyle.c_str());
                        else if (nFunctions >= 2)
                            _graph->Puts(mglPoint(vResults[0], vResults[1]), sTextString.c_str(), sStyle.c_str());
                        else
                            continue;
                    }
                    else if (vDrawVector[v].substr(0,8) == "polygon(")
                    {
                        if (nFunctions < 5 || vResults[4] < 3)
                            continue;
                        _graph->Polygon(mglPoint(vResults[0], vResults[1]), mglPoint(vResults[2], vResults[3]), (int)vResults[4], sStyle.c_str());
                    }
                    else
                        continue;
                }
            }
            else if (bDraw3D)
            {
                string sStyle;
                string sTextString;
                for (unsigned int v = 0; v < vDrawVector.size(); v++)
                {
                    sStyle = "k";
                    sTextString = "";
                    //cerr << vDrawVector[v] << endl;
                    if (containsStrings(vDrawVector[v]) || _data->containsStringVars(vDrawVector[v]))
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
                        //sStyle = vDrawVector[v].substr(vDrawVector[v].rfind(',')+1);
                        sStyle = sStyle.substr(0,sStyle.rfind(')')) + " -nq";
                        parser_StringParser(sStyle, sDummy, *_data, *_parser, *_option, true);
                        //vDrawVector[v].erase(vDrawVector[v].rfind(','));
                    }
                    if (containsStrings(vDrawVector[v]) || _data->containsStringVars(vDrawVector[v]))
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
                        //sTextString = vDrawVector[v].substr(vDrawVector[v].rfind(',')+1);
                        sTextString += " -nq";
                        parser_StringParser(sTextString, sDummy, *_data, *_parser, *_option, true);
                        //vDrawVector[v].erase(vDrawVector[v].rfind(','));
                    }
                    //cerr << vDrawVector[v] << endl;
                    if (vDrawVector[v][vDrawVector[v].length()-1] == ')')
                        _parser->SetExpr(vDrawVector[v].substr(vDrawVector[v].find('(')+1,vDrawVector[v].rfind(')')-vDrawVector[v].find('(')-1));
                    else
                        _parser->SetExpr(vDrawVector[v].substr(vDrawVector[v].find('(')+1));
                    vResults = _parser->Eval(nFunctions);
                    //cerr << nFunctions << endl;
                    //cerr << vDrawVector[v].substr(0,5) << endl;

                    //trace(x,y,x,y,STYLE), face(x,y,x,y,x,y,STYLE), sphere(x,y,r,STYLE), circle(x,y,r,STYLE), cuboid(x,y,x,y,STYLE)
                    if (vDrawVector[v].substr(0,6) == "trace(")
                    {
                        if (nFunctions < 6)
                            continue;
                        _graph->Line(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), sStyle.c_str());
                    }
                    else if (vDrawVector[v].substr(0,5) == "face(")
                    {
                        if (nFunctions < 6)
                            continue;
                        if (nFunctions < 9)
                            _graph->Face(mglPoint(vResults[3]-vResults[4]+vResults[1], vResults[4]+vResults[3]-vResults[0], vResults[5]),
                                        mglPoint(vResults[3],vResults[4],vResults[5]),
                                        mglPoint(vResults[0]-vResults[4]+vResults[1], vResults[1]+vResults[3]-vResults[0], vResults[2]),
                                        mglPoint(vResults[0],vResults[1],vResults[2]),
                                        sStyle.c_str());
                        else if (nFunctions < 12)
                            _graph->Face(mglPoint(vResults[6],vResults[7],vResults[8]),
                                        mglPoint(vResults[3],vResults[4],vResults[5]),
                                        mglPoint(vResults[0]+vResults[6]-vResults[3],vResults[1]+vResults[7]-vResults[4],vResults[2]+vResults[8]-vResults[5]),
                                        mglPoint(vResults[0],vResults[1],vResults[2]),
                                        sStyle.c_str());
                        else
                            _graph->Face(mglPoint(vResults[6],vResults[7],vResults[8]),
                                        mglPoint(vResults[3],vResults[4],vResults[5]),
                                        mglPoint(vResults[9],vResults[10],vResults[11]),
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
                            _graph->Face(mglPoint((-vResults[0]+vResults[3])/2.0-c*vResults[4], (-vResults[1]+vResults[4])/2.0+c*vResults[3], (vResults[5]+vResults[2])/2.0),
                                        mglPoint(vResults[3],vResults[4],vResults[5]),
                                        mglPoint((-vResults[0]+vResults[3])/2.0-c*vResults[4], (-vResults[1]+vResults[4])/2.0+c*vResults[3], (vResults[5]+vResults[2])/2.0),
                                        mglPoint(vResults[0],vResults[1],vResults[2]),
                                        sStyle.c_str());
                        else
                            _graph->Face(mglPoint(vResults[6],vResults[7],vResults[8]),
                                        mglPoint(vResults[3],vResults[4],vResults[5]),
                                        mglPoint(vResults[6],vResults[7],vResults[8]),
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

                        _graph->Face(mglPoint(vResults[0],vResults[1],vResults[2]),
                                    mglPoint(vResults[0],vResults[1],vResults[2])+_mDx,
                                    mglPoint(vResults[0],vResults[1],vResults[2])+_mDy,
                                    mglPoint(vResults[0],vResults[1],vResults[2])+_mDx+_mDy,
                                    sStyle.c_str());
                        _graph->Face(mglPoint(vResults[0],vResults[1],vResults[2]),
                                    mglPoint(vResults[0],vResults[1],vResults[2])+_mDx,
                                    mglPoint(vResults[0],vResults[1],vResults[2])+_mDz,
                                    mglPoint(vResults[0],vResults[1],vResults[2])+_mDz+_mDx,
                                    sStyle.c_str());
                        _graph->Face(mglPoint(vResults[0],vResults[1],vResults[2]),
                                    mglPoint(vResults[0],vResults[1],vResults[2])+_mDy,
                                    mglPoint(vResults[0],vResults[1],vResults[2])+_mDz,
                                    mglPoint(vResults[0],vResults[1],vResults[2])+_mDy+_mDz,
                                    sStyle.c_str());
                        _graph->Face(mglPoint(vResults[0],vResults[1],vResults[2])+_mDz,
                                    mglPoint(vResults[0],vResults[1],vResults[2])+_mDx+_mDz,
                                    mglPoint(vResults[0],vResults[1],vResults[2])+_mDy+_mDz,
                                    mglPoint(vResults[0],vResults[1],vResults[2])+_mDx+_mDy+_mDz,
                                    sStyle.c_str());
                        _graph->Face(mglPoint(vResults[0],vResults[1],vResults[2])+_mDy,
                                    mglPoint(vResults[0],vResults[1],vResults[2])+_mDx+_mDy,
                                    mglPoint(vResults[0],vResults[1],vResults[2])+_mDz+_mDy,
                                    mglPoint(vResults[0],vResults[1],vResults[2])+_mDz+_mDx+_mDy,
                                    sStyle.c_str());
                        _graph->Face(mglPoint(vResults[0],vResults[1],vResults[2])+_mDx,
                                    mglPoint(vResults[0],vResults[1],vResults[2])+_mDy+_mDx,
                                    mglPoint(vResults[0],vResults[1],vResults[2])+_mDz+_mDx,
                                    mglPoint(vResults[0],vResults[1],vResults[2])+_mDy+_mDz+_mDx,
                                    sStyle.c_str());
                    }
                    else if (vDrawVector[v].substr(0,7) == "sphere(")
                    {
                        if (nFunctions < 4)
                            continue;
                        _graph->Sphere(mglPoint(vResults[0], vResults[1], vResults[2]), vResults[3], sStyle.c_str());
                    }
                    else if (vDrawVector[v].substr(0,5) == "cone(")
                    {
                        if (nFunctions < 7)
                            continue;
                        if (nFunctions >= 8)
                            _graph->Cone(mglPoint(vResults[0],vResults[1],vResults[2]), mglPoint(vResults[3],vResults[4],vResults[5]), vResults[6], vResults[7], sStyle.c_str());
                        else
                            _graph->Cone(mglPoint(vResults[0],vResults[1],vResults[2]), mglPoint(vResults[3],vResults[4],vResults[5]), vResults[6], 0.0, sStyle.c_str());
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
                        _graph->Drop(mglPoint(vResults[0], vResults[1], vResults[2]),
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
                        _graph->Circle(mglPoint(vResults[0], vResults[1], vResults[2]), vResults[3], sStyle.c_str());
                    }
                    else if (vDrawVector[v].substr(0,4) == "arc(")
                    {
                        if (nFunctions < 7)
                            continue;
                        if (nFunctions < 9)
                            _graph->Arc(mglPoint(vResults[0], vResults[1], vResults[2]),
                                        mglPoint(vResults[3], vResults[4], vResults[5]), vResults[6], sStyle.c_str());
                        else
                            _graph->Arc(mglPoint(vResults[0], vResults[1], vResults[2]),
                                        mglPoint(vResults[3], vResults[4], vResults[5]),
                                        mglPoint(vResults[6], vResults[7], vResults[8]),
                                        vResults[9], sStyle.c_str());

                    }
                    else if (vDrawVector[v].substr(0,6) == "point(")
                    {
                        if (nFunctions < 3)
                            continue;
                        _graph->Mark(mglPoint(vResults[0], vResults[1], vResults[2]), sStyle.c_str());
                    }
                    else if (vDrawVector[v].substr(0,6) == "curve(")
                    {
                        if (nFunctions < 12)
                            continue;
                        _graph->Curve(mglPoint(vResults[0], vResults[1], vResults[2]),
                                    mglPoint(vResults[3], vResults[4], vResults[5]),
                                    mglPoint(vResults[6], vResults[7], vResults[8]),
                                    mglPoint(vResults[9], vResults[10], vResults[11]), sStyle.c_str());
                    }
                    else if (vDrawVector[v].substr(0,8) == "ellipse(")
                    {
                        if (nFunctions < 7)
                            continue;
                        _graph->Ellipse(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), vResults[6], sStyle.c_str());
                    }
                    else if (vDrawVector[v].substr(0,5) == "text(")
                    {
                        if (!sTextString.length())
                        {
                            sTextString = sStyle;
                            sStyle = "k";
                        }
                        if (nFunctions >= 6)
                            _graph->Puts(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), sTextString.c_str(), sStyle.c_str());
                        else if (nFunctions >= 3)
                            _graph->Puts(mglPoint(vResults[0], vResults[1], vResults[2]), sTextString.c_str(), sStyle.c_str());
                        else
                            continue;
                    }
                    else if (vDrawVector[v].substr(0,8) == "polygon(")
                    {
                        if (nFunctions < 7 || vResults[6] < 3)
                            continue;
                        _graph->Polygon(mglPoint(vResults[0], vResults[1], vResults[2]), mglPoint(vResults[3], vResults[4], vResults[5]), (int)vResults[6], sStyle.c_str());
                    }
                    else
                        continue;
                }
            }
            else            // 3D-Trajektorie
            {
                if (_pData->getCutBox())
                    _graph->SetCutBox(parser_CalcCutBox(_pData->getRotateAngle(1), dRanges, 0, _pData->getCoords(), true), parser_CalcCutBox(_pData->getRotateAngle(1), dRanges, 1, _pData->getCoords(), true));
                if (sFunc != "<<empty>>")
                {
                    mglData _mData[3] = {mglData(nSamples), mglData(nSamples), mglData(nSamples)};
                    int nLabels = 0;
                    unsigned int nPos = 0;
                    while (sLabels.find(';', nPos) != string::npos)
                    {
                        nPos = sLabels.find(';', nPos) + 1;
                        nLabels++;
                        if (nPos >= sLabels.length())
                            break;
                    }

                    nPos = 0;
                    if (nLabels > _pData->getLayers())
                    {
                        for (int i = 0; i < _pData->getLayers(); i++)
                        {
                            for (int j = 0; j < 2; j++)
                            {
                                if (nLabels == 1)
                                    break;
                                nPos = sLabels.find(';', nPos);
                                sLabels = sLabels.substr(0,nPos-1) + ", " + sLabels.substr(nPos+2);
                                nLabels--;
                            }
                            nPos = sLabels.find(';', nPos) + 1;
                            nLabels--;
                        }
                        nPos = 0;
                    }
                    if (_option->getbDebug())
                        cerr << LineBreak("|-> DEBUG: sLabels = " + sLabels, *_option) << endl;

                    for (int k = 0; k < _pData->getLayers(); k++)
                    {
                        StripSpaces(sLabels);
                        for (long int i = 0; i < nSamples; i++)
                        {
                            for (int j = 0; j < 3; j++)
                            {
                                _mData[j].a[i] = _pData->getData(i,j,k);
                            }
                        }
                        if (!_pData->getArea() && !_pData->getRegion())
                            _graph->Plot(_mData[0], _mData[1], _mData[2], sLineStyles[nStyle].c_str());
                        else if (_pData->getRegion() && k+1 < _pData->getLayers())
                        {
                            mglData _mData2[3] = {mglData(nSamples), mglData(nSamples), mglData(nSamples)};
                            for (long int i = 0; i < nSamples; i++)
                            {
                                for (int j = 0; j < 3; j++)
                                {
                                    _mData2[j].a[i] = _pData->getData(i,j,k+1);
                                }
                            }
                            if (nStyle == nStyleMax-1)
                                _graph->Region(_mData[0], _mData[1], _mData[2], _mData2[0], _mData2[1], _mData2[2], ("{"+_pData->getColors().substr(nStyle,1) + "7}{" + _pData->getColors().substr(0,1)+"7}").c_str());
                            else
                                _graph->Region(_mData[0], _mData[1], _mData[2], _mData2[0], _mData2[1], _mData2[2], ("{"+_pData->getColors().substr(nStyle,1) + "7}{" + _pData->getColors().substr(nStyle+1,1)+"7}").c_str());
                            _graph->Plot(_mData[0], _mData[1], _mData[2], sLineStyles[nStyle].c_str());
                            if (nStyle == nStyleMax-1)
                                _graph->Plot(_mData2[0], _mData2[1], _mData2[2], sLineStyles[0].c_str());
                            else
                                _graph->Plot(_mData2[0], _mData2[1], _mData2[2], sLineStyles[nStyle+1].c_str());
                            for (int n = 0; n < 2; n++)
                            {
                                nPos = sLabels.find(';');
                                sConvLegends = sLabels.substr(0,nPos) + " -nq";
                                parser_StringParser(sConvLegends, sDummy, *_data, *_parser, *_option, true);
                                sConvLegends = "\"" + sConvLegends + "\"";
                                sLabels = sLabels.substr(nPos+1);
                                if (sConvLegends != "\"\"")
                                {
                                    _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sLineStyles[nStyle].c_str());
                                    nLegends++;
                                }
                                if (nStyle == nStyleMax-1)
                                    nStyle = 0;
                                else
                                    nStyle++;
                            }
                            k++;
                            continue;
                        }
                        else
                            _graph->Area(_mData[0], _mData[1], _mData[2], (sLineStyles[nStyle] + "{" + _pData->getColors()[nStyle] + "9}").c_str());
                        nPos = sLabels.find(';');
                        sConvLegends = sLabels.substr(0,nPos) + " -nq";
                        //sConvLegends = "\"Trajektorie " + toString(k+1) + "\"";
                        parser_StringParser(sConvLegends, sDummy, *_data, *_parser, *_option, true);
                        sConvLegends = "\""+sConvLegends + "\"";
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
                        if (sLabels.length() > nPos+1)
                            sLabels = sLabels.substr(nPos+1);
                        if (sConvLegends != "\"\"")
                        {
                            _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sLineStyles[nStyle].c_str());
                            nLegends++;
                        }
                        if (nStyle == nStyleMax-1)
                            nStyle = 0;
                        else
                            nStyle++;
                    }
                }
                if (_mDataPlots)
                {
                    int nPos = 0;
                    for (int j = 0; j < nDataPlots; j++)
                    {
                        StripSpaces(sDataLabels);
                        if (!_pData->getxError() && !_pData->getyError())
                        {
                            // --> Interpolate-Schalter. Siehe weiter oben fuer Details <--
                            if (_pData->getInterpolate() && _mDataPlots[j][0].GetNx() >= nSamples)
                            {
                                if (!_pData->getArea() && !_pData->getBars() && !_pData->getRegion())
                                    _graph->Plot(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], sLineStyles[nStyle].c_str());
                                else if (_pData->getBars() && !_pData->getArea() && !_pData->getRegion())
                                    _graph->Bars(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], (sLineStyles[nStyle] + "^").c_str());
                                else if (_pData->getRegion() && j+1 < nDataPlots)
                                {
                                    if (nStyle == nStyleMax-1)
                                        _graph->Region(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _mDataPlots[j+1][0], _mDataPlots[j+1][1], _mDataPlots[j+1][2], ("{"+_pData->getColors().substr(nStyle,1) +"7}{"+ _pData->getColors().substr(0,1)+"7}").c_str());
                                    else
                                        _graph->Region(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _mDataPlots[j+1][0], _mDataPlots[j+1][1], _mDataPlots[j+1][2], ("{"+_pData->getColors().substr(nStyle,1) +"7}{"+ _pData->getColors().substr(nStyle+1,1)+"7}").c_str());
                                    _graph->Plot(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], sLineStyles[nStyle].c_str());
                                    j++;
                                    if (nStyle == nStyleMax-1)
                                        _graph->Plot(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], sLineStyles[0].c_str());
                                    else
                                        _graph->Plot(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], sLineStyles[nStyle+1].c_str());
                                    for (int k = 0; k < 2; k++)
                                    {
                                        nPos = sDataLabels.find(';');
                                        sConvLegends = sDataLabels.substr(0,nPos);
                                        parser_StringParser(sConvLegends, sDummy, *_data, *_parser, *_option, true);
                                        sDataLabels = sDataLabels.substr(nPos+1);
                                        if (sConvLegends != "\"\"")
                                        {
                                            nLegends++;
                                            _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sLineStyles[nStyle].c_str());
                                        }
                                    if (nStyle == nStyleMax-1)
                                        nStyle = 0;
                                    else
                                        nStyle++;

                                    }
                                    continue;
                                }
                                else
                                    _graph->Area(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], (sLineStyles[nStyle] + "{" + _pData->getColors()[nStyle] + "9}").c_str());
                            }
                            else if (_pData->getConnectPoints() || (_pData->getInterpolate() && _mDataPlots[j][0].GetNx() >= 0.9*nSamples))
                            {
                                if (!_pData->getArea() && !_pData->getBars())
                                    _graph->Plot(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], sConPointStyles[nStyle].c_str());
                                else if (_pData->getBars() && !_pData->getArea())
                                    _graph->Bars(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], (sLineStyles[nStyle]+"^").c_str());
                                else
                                    _graph->Area(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], (sLineStyles[nStyle] + "{" + _pData->getColors()[nStyle] + "9}").c_str());
                            }
                            else
                            {
                                if (!_pData->getArea() && !_pData->getMarks() && !_pData->getBars())
                                    _graph->Plot(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], sPointStyles[nStyle].c_str());
                                else if (_pData->getMarks() && !_pData->getBars() && !_pData->getArea())
                                    _graph->Dots(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], _pData->getColorScheme(toString(_pData->getMarks())).c_str());
                                else if (_pData->getBars() && !_pData->getArea())
                                    _graph->Bars(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], (sLineStyles[nStyle]+"^").c_str());
                                else
                                    _graph->Stem(_mDataPlots[j][0], _mDataPlots[j][1], _mDataPlots[j][2], sConPointStyles[nStyle].c_str());
                            }
                        }
                        else if (_pData->getxError() && _pData->getyError())
                        {
                            for (int m = 0; m < _mDataPlots[j][0].nx; m++)
                            {
                                _graph->Error(mglPoint(_mDataPlots[j][0].a[m], _mDataPlots[j][1].a[m], _mDataPlots[j][2].a[m]), mglPoint(_mDataPlots[j][3].a[m], _mDataPlots[j][4].a[m], _mDataPlots[j][5].a[m]), sPointStyles[nStyle].c_str());
                            }
                        }
                        nPos = sDataLabels.find(';');
                        sConvLegends = sDataLabels.substr(0,nPos);
                        parser_StringParser(sConvLegends, sDummy, *_data, *_parser, *_option, true);
                        sDataLabels = sDataLabels.substr(nPos+1);
                        if (sConvLegends != "\"\"")
                        {
                            nLegends++;
                            if (!_pData->getxError() && !_pData->getyError())
                            {
                                if ((_pData->getInterpolate() && _mDataPlots[j][0].GetNx() >= nSamples) || _pData->getBars())
                                    _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(),sLineStyles[nStyle].c_str());
                                else if (_pData->getConnectPoints() || (_pData->getInterpolate() && _mDataPlots[j][0].GetNx() >= 0.9*nSamples))
                                    _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sConPointStyles[nStyle].c_str());
                                else if (!_pData->getMarks())
                                    _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sPointStyles[nStyle].c_str());
                            }
                            else
                                _graph->AddLegend(fromSystemCodePage(replaceToTeX(sConvLegends.substr(1,sConvLegends.length()-2))).c_str(), sPointStyles[nStyle].c_str());
                        }
                        if (nStyle == nStyleMax-1)
                            nStyle = 0;
                        else
                            nStyle++;
                    }
                }
                if (_pData->getCutBox())
                    _graph->SetCutBox(mglPoint(0), mglPoint(0));
                if (!(_pData->getMarks() && sCommand.substr(0,6) == "plot3d") && nLegends && !_pData->getSchematic() && nPlotCompose+1 == vPlotCompose.size())
                {
                    if (_pData->getRotateAngle() ||_pData->getRotateAngle(1))
                        _graph->Legend(1.35,1.2);
                    else
                        _graph->Legend();
                }
            }

            if (!nPlotCompose)
            {
                // --> Achsen beschriften <--
                if (_pData->getAxis()
                    && _pData->getBox()
                    && !_pData->getCoords()
                    && (!_pData->getSchematic()
                        || matchParams(sPlotParams, "xlabel", '=')
                        || matchParams(sPlotParams, "ylabel", '=')
                        || matchParams(sPlotParams, "zlabel", '=')))
                {
                    _graph->Label('x', fromSystemCodePage(_pData->getxLabel()).c_str(), 0.0);
                    _graph->Label('y', fromSystemCodePage(_pData->getyLabel()).c_str(), 0.0);
                    _graph->Label('z', fromSystemCodePage(_pData->getzLabel()).c_str(), 0.0);
                }
                else if (_pData->getAxis()
                    && _pData->getCoords()
                    && (!_pData->getSchematic()
                        || matchParams(sPlotParams, "xlabel", '=')
                        || matchParams(sPlotParams, "ylabel", '=')
                        || matchParams(sPlotParams, "zlabel", '=')))
                {
                    // --> Im Falle von krummliniger Koordinaten sind die Achsen anders beschriftet <--
                    if (_pData->getCoords() && !(b2D || sCommand.find("3d") != string::npos || b2DVect))
                        _graph->Label('x', fromSystemCodePage(_pData->getxLabel()).c_str(), 0.25);
                    else if (_pData->getCoords() && b2DVect)
                        _graph->Label('x', fromSystemCodePage(_pData->getzLabel()).c_str(), 0.0);
                    else if (sCommand.find("3d") != string::npos && _pData->getCoords() == 1)
                        _graph->Label('x', fromSystemCodePage(_pData->getzLabel()).c_str(), -0.5);
                    else if (sCommand.find("3d") != string::npos && _pData->getCoords() == 2)
                        _graph->Label('x', fromSystemCodePage(_pData->getzLabel()).c_str(), -0.4);
                    else if (_pData->getCoords() == 1)
                        _graph->Label('x', fromSystemCodePage(_pData->getxLabel()).c_str(), -0.65);
                    else
                        _graph->Label('x', fromSystemCodePage(_pData->getxLabel()).c_str(), -0.7);

                    if (_pData->getCoords() && !(b2D || sCommand.find("3d") != string::npos || b2DVect))
                        _graph->Label('y', fromSystemCodePage(_pData->getzLabel()).c_str(), 0.0);
                    else if (_pData->getCoords() && b2DVect)
                        _graph->Label('y', fromSystemCodePage(_pData->getxLabel()).c_str(), 0.25);
                    else if (_pData->getCoords() == 1 && sCommand.find("3d") != string::npos)
                        _graph->Label('y', fromSystemCodePage(_pData->getxLabel()).c_str(), -0.65);
                    else if (_pData->getCoords() == 2 && sCommand.find("3d") != string::npos)
                        _graph->Label('y', fromSystemCodePage(_pData->getxLabel()).c_str(), -0.7);
                    else if (_pData->getCoords() == 1)
                        _graph->Label('y', fromSystemCodePage(_pData->getyLabel()).c_str(), 0.0);
                    else
                        _graph->Label('y', fromSystemCodePage(_pData->getyLabel()).c_str(), -0.4);

                    if (b2D && _pData->getCoords() == 2)
                        _graph->Label('z', fromSystemCodePage(_pData->getzLabel()).c_str(), -0.5);
                    else if (sCommand.find("3d") != string::npos && _pData->getCoords() == 2)
                        _graph->Label('z', fromSystemCodePage(_pData->getyLabel()).c_str(), -0.4);
                    else if (sCommand.find("3d") != string::npos && _pData->getCoords() == 1)
                        _graph->Label('z', fromSystemCodePage(_pData->getyLabel()).c_str(), 0.0);
                    else if (_pData->getCoords() == 1)
                        _graph->Label('z', fromSystemCodePage(_pData->getzLabel()).c_str(), -0.5);
                }
                else if (_pData->getAxis()
                    && !_pData->getBox()
                    && (!_pData->getSchematic()
                        || matchParams(sPlotParams, "xlabel", '=')
                        || matchParams(sPlotParams, "ylabel", '=')
                        || matchParams(sPlotParams, "zlabel", '=')))
                {
                    _graph->Label('x', fromSystemCodePage(_pData->getxLabel()).c_str(), 1.1);
                    _graph->Label('y', fromSystemCodePage(_pData->getyLabel()).c_str(), 1.1);
                    _graph->Label('z', fromSystemCodePage(_pData->getzLabel()).c_str(), 1.1);
                }
            }
            // --> GIF-Frame beenden <--
            if (_pData->getAnimateSamples())
                _graph->EndFrame();
        }

        // --> GIF-Datei schliessen <--
        if (_pData->getAnimateSamples())
            _graph->CloseGIF();

    }
    // --> Zurueck zur aufrufenden Funktion! <--
    return 0;
}

void Graph_helper::Reload()
{
    return;
}

