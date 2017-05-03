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

#include <mgl2/mgl.h>
#include "plotdata.hpp"

extern mglGraph _fontData;

// --> Konstruktor <--
PlotData::PlotData() : FileSystem()
{
    dPlotData = 0;
    sFontStyle = "pagella";
//    _graph = 0;
    PlotData::reset();
}

// --> Allgemeiner Konstruktor <--
PlotData::PlotData(int _nLines, int _nRows, int _nLayers) : FileSystem()
{
    PlotData();
    nRows = _nRows;
    nLines = _nLines;
    nLayers = _nLayers;
    dPlotData = new double**[nLines];
    for (int i = 0; i < nLines; i++)
    {
        dPlotData[i] = new double*[nRows];
        for (int j = 0; j < nRows; j++)
        {
            dPlotData[i][j] = new double[nLayers];
            for (int k = 0; k < nLayers; k++)
            {
                dPlotData[i][j][k] = 0.0;
            }
        }
    }
    for (int i = 0; i < 3; i++)
    {
        dRanges[i][0] = -10.0;
        dRanges[i][1] = 10.0;
    }

}

// --> Destruktor <--
PlotData::~PlotData()
{
    if (dPlotData)
    {
        for (int i = 0; i < nLines; i++)
        {
            for (int j = 0; j < nRows; j++)
            {
                delete[] dPlotData[i][j];
            }
            delete[] dPlotData[i];
        }
        delete[] dPlotData;
        dPlotData = 0;
    }
//    if (_graph)
//        delete _graph;
}

// --> Daten in Speicher schreiben: beachtet auch die aktuelle Groesse des Speichers <--
void PlotData::setData(int _i, int _j, double dData, int _k)
{
    if (!dPlotData || _i >= nLines || _j >= nRows || _k >= nLayers)
    {
        setDim(_i+1, _j+1, _k+1);
    }
    dPlotData[_i][_j][_k] = dData;
    if (_k > nRequestedLayers-1)
        nRequestedLayers = _k+1;
    if (isnan(dData))
        return;
    if (isnan(dMin) || dMin > dData)
    {
        dMin = dData;
    }
    if (isnan(dMax) || dMax < dData)
    {
        dMax = dData;
    }
    return;
}

// --> Daten aus Speicher lesen <--
double PlotData::getData(int _i, int _j, int _k) const
{
    if (!dPlotData || _i >= nLines || _j >= nRows || _k >= nLayers)
        return NAN;
    else
        return dPlotData[_i][_j][_k];
}

// --> Gespeicherten Dateinamen lesen <--
string PlotData::getFileName() const
{
    return sFileName;
}

/* --> Parameter setzen: Verwendet die bool matchParams(const string&, const string&, char)-Funktion,
 *     um die einzelnen Befehle zu identifizieren. Unbekannte Befehle werden automatisch ignoriert. 7
 *     Dies ist dann automatisch Fehlertoleranter <--
 */
void PlotData::setParams(const string& __sCmd, Parser& _parser, const Settings& _option, int nType)
{
    string sCmd = toLowerCase(__sCmd);
    if (matchParams(sCmd, "reset") && (!nType || nType == 1))
        reset();
    if (matchParams(sCmd, "grid") && (!nType || nType == 1))
        nGrid = 1;
    if (matchParams(sCmd, "grid", '=') && (!nType || nType == 1))
    {
        unsigned int nPos = matchParams(sCmd, "grid", '=')+4;
        if (getArgAtPos(sCmd, nPos) == "fine")
            nGrid = 2;
        else if (getArgAtPos(sCmd, nPos) == "coarse")
            nGrid = 1;
        else
            nGrid = 1;
    }
    if (matchParams(sCmd, "nogrid") && (!nType || nType == 1))
        nGrid = 0;
    if ((matchParams(sCmd, "alpha") || matchParams(sCmd, "transparency")) && (!nType || nType == 2))
        bAlpha = true;
    if ((matchParams(sCmd, "noalpha") || matchParams(sCmd, "notransparency")) && (!nType || nType == 2))
        bAlpha = false;
    if (matchParams(sCmd, "light") && (!nType || nType == 2))
        nLighting = 1;
    if (matchParams(sCmd, "light", '=') && (!nType || nType == 2))
    {
        if (getArgAtPos(sCmd, matchParams(sCmd, "light", '=')+5) == "smooth")
            nLighting = 2;
        else if (getArgAtPos(sCmd, matchParams(sCmd, "light", '=')+5) == "soft")
            nLighting = 2;
        else
            nLighting = 0;
    }
    if (matchParams(sCmd, "nolight") && (!nType || nType == 2))
        nLighting = 0;
    if (matchParams(sCmd, "axis") && (!nType || nType == 1))
        bAxis = true;
    if (matchParams(sCmd, "noaxis") && (!nType || nType == 1))
        bAxis = false;
    if (matchParams(sCmd, "box") && (!nType || nType == 1))
        bBox = true;
    if (matchParams(sCmd, "nobox") && (!nType || nType == 1))
        bBox = false;
    if (matchParams(sCmd, "lcont") && (!nType || nType == 2))
        bContLabels = true;
    if (matchParams(sCmd, "nolcont") && (!nType || nType == 2))
        bContLabels = false;
    if (matchParams(sCmd, "pcont") && (!nType || nType == 2))
        bContProj = true;
    if (matchParams(sCmd, "nopcont") && (!nType || nType == 2))
        bContProj = false;
    if (matchParams(sCmd, "fcont") && (!nType || nType == 2))
        bContFilled = true;
    if (matchParams(sCmd, "nofcont") && (!nType || nType == 2))
        bContFilled = false;
    if (matchParams(sCmd, "xerrorbars") && (!nType || nType == 2))
        bxError = true;
    if (matchParams(sCmd, "noxerrorbars") && (!nType || nType == 2))
        bxError = false;
    if (matchParams(sCmd, "yerrorbars") && (!nType || nType == 2))
        byError = true;
    if (matchParams(sCmd, "noyerrorbars") && (!nType || nType == 2))
        byError = false;
    if (matchParams(sCmd, "errorbars") && (!nType || nType == 2))
    {
        bxError = true;
        byError = true;
    }
    if (matchParams(sCmd, "noerrorbars") && (!nType || nType == 2))
    {
        bxError = false;
        byError = false;
    }
    if (matchParams(sCmd, "logscale") && (!nType || nType == 1))
    {
        for (int i = 0; i < 4; i++)
        {
            bLogscale[i] = true;
        }
    }
    if (matchParams(sCmd, "nologscale") && (!nType || nType == 1))
    {
        for (int i = 0; i < 4; i++)
        {
            bLogscale[i] = false;
        }
    }
    if (matchParams(sCmd, "xlog") && (!nType || nType == 1))
        bLogscale[0] = true;
    if (matchParams(sCmd, "ylog") && (!nType || nType == 1))
        bLogscale[1] = true;
    if (matchParams(sCmd, "zlog") && (!nType || nType == 1))
        bLogscale[2] = true;
    if (matchParams(sCmd, "clog") && (!nType || nType == 1))
        bLogscale[3] = true;
    if (matchParams(sCmd, "noxlog") && (!nType || nType == 1))
        bLogscale[0] = false;
    if (matchParams(sCmd, "noylog") && (!nType || nType == 1))
        bLogscale[1] = false;
    if (matchParams(sCmd, "nozlog") && (!nType || nType == 1))
        bLogscale[2] = false;
    if (matchParams(sCmd, "noclog") && (!nType || nType == 1))
        bLogscale[3] = false;
    if (matchParams(sCmd, "samples", '=') && (!nType || nType == 2))
    {
        int nPos = matchParams(sCmd, "samples", '=') + 7;
        _parser.SetExpr(getArgAtPos(__sCmd, nPos));
        nSamples = (int)_parser.Eval();
        if (isnan(_parser.Eval()) || isinf(_parser.Eval()))
            nSamples = 100;
        if (_option.getbDebug())
            cerr << "|-> DEBUG: nSamples = " << nSamples << endl;
    }
    if (matchParams(sCmd, "t", '=') && (!nType || nType == 2))
    {
        int nPos = matchParams(sCmd, "t", '=')+1;
        string sTemp_1 = "(" + getArgAtPos(__sCmd, nPos) + ")";
        string sTemp_2 = "";
        if (sTemp_1.find(':') != string::npos)
        {
            parser_SplitArgs(sTemp_1, sTemp_2, ':', _option, false);
            _parser.SetExpr(sTemp_1);
            dtParam[0] = _parser.Eval();
            if (isnan(dtParam[0]) || isinf(dtParam[0]))
                dtParam[0] = 0;
            _parser.SetExpr(sTemp_2);
            dtParam[1] = _parser.Eval();
            if (isnan(dtParam[1]) || isinf(dtParam[1]))
                dtParam[1] = 1;
        }
    }
    if (matchParams(sCmd, "colorrange", '=') && (!nType || nType == 1))
    {
        unsigned int nPos = matchParams(sCmd, "colorrange", '=') + 10;
        string sTemp_1 = "(" + getArgAtPos(__sCmd, nPos) + ")";
        string sTemp_2 = "";
        if (sTemp_1.find(':') != string::npos)
        {
            parser_SplitArgs(sTemp_1, sTemp_2, ':', _option, false);
            _parser.SetExpr(sTemp_1);
            dColorRange[0] = _parser.Eval();
            _parser.SetExpr(sTemp_2);
            dColorRange[1] = _parser.Eval();
            if (isnan(dColorRange[0]) || isnan(dColorRange[1]) || isinf(dColorRange[0]) || isinf(dColorRange[1]))
            {
                dColorRange[0] = NAN;
                dColorRange[1] = NAN;
            }
        }
    }
    if (matchParams(sCmd, "rotate", '=') && (!nType || nType == 1))
    {
        int nPos = matchParams(sCmd, "rotate", '=')+6;
        string sTemp = getArgAtPos(__sCmd, nPos);
        if (sTemp.find(",") != string::npos && sTemp.length() > 1)
        {
            if (sTemp.find(',') && sTemp.find(',') != sTemp.length()-1)
            {
                _parser.SetExpr(sTemp);
                _parser.Eval();
                int nResults = _parser.GetNumResults();
                double* dTemp = _parser.Eval(nResults);
                dRotateAngles[0] = dTemp[0];
                dRotateAngles[1] = dTemp[1];
            }
            else if (!sTemp.find(','))
            {
                _parser.SetExpr(sTemp.substr(1));
                dRotateAngles[1] = _parser.Eval();
            }
            else if (sTemp.find(',') == sTemp.length()-1)
            {
                _parser.SetExpr(sTemp.substr(0,sTemp.length()-1));
                dRotateAngles[0] = _parser.Eval();
            }
            for (unsigned int i = 0; i < 2; i++)
            {
                if (isinf(dRotateAngles[i]) || isnan(dRotateAngles[i]))
                {
                    if (!i)
                        dRotateAngles[i] = 60;
                    else
                        dRotateAngles[i] = 115;
                }
            }
            if (dRotateAngles[0] < 0)
            {
                dRotateAngles[0] += ceil(-dRotateAngles[0]/180.0)*180.0;
            }
            if (dRotateAngles[0] > 180)
            {
                dRotateAngles[0] -= floor(dRotateAngles[0]/180.0)*180.0;
            }
            if (dRotateAngles[1] < 0)
            {
                dRotateAngles[1] += ceil(-dRotateAngles[1]/360.0)*360.0;
            }
            if (dRotateAngles[1] > 360)
            {
                dRotateAngles[1] -= floor(dRotateAngles[1]/360.0)*360.0;
            }
        }
    }
    if (matchParams(sCmd, "origin", '=') && (!nType || nType == 1))
    {
        int nPos = matchParams(sCmd, "origin", '=')+6;
        string sTemp = getArgAtPos(__sCmd, nPos);
        if (sTemp.find(',') != string::npos && sTemp.length() > 1)
        {
            _parser.SetExpr(sTemp);
            int nResults = 0;
            double* dTemp = _parser.Eval(nResults);
            if (nResults)
            {
                for (int i = 0; i < 3; i++)
                {
                    if (i < nResults && !isnan(dTemp[i]) && !isinf(dTemp[i]))
                        dOrigin[i] = dTemp[i];
                    else
                        dOrigin[i] = 0.0;
                }
            }
        }
        else if (sTemp == "sliding")
        {
            for (int i = 0; i < 3; i++)
                dOrigin[i] = NAN;
        }
        else
        {
            for (int i = 0; i < 3; i++)
                dOrigin[i] = 0.0;
        }
    }
    if (matchParams(sCmd, "slices", '=') && (!nType || nType == 2))
    {
        int nPos = matchParams(sCmd, "slices", '=')+6;
        string sTemp = getArgAtPos(__sCmd, nPos);
        if (sTemp.find(',') != string::npos && sTemp.length() > 1)
        {
            _parser.SetExpr(sTemp);
            int nResults = 0;
            double* dTemp = _parser.Eval(nResults);
            if (nResults)
            {
                for (int i = 0; i < 3; i++)
                {
                    if (i < nResults && !isnan(dTemp[i]) && !isinf(dTemp[i]) && dTemp[i] <= 5 && dTemp >= 0)
                        nSlices[i] = (unsigned short)dTemp[i];
                    else
                        nSlices[i] = 1;
                }
            }
        }
        else
        {
            for (int i = 0; i < 3; i++)
                nSlices[i] = 1;
        }
    }
    if (matchParams(sCmd, "connect") && (!nType || nType == 2))
        bConnectPoints = true;
    if (matchParams(sCmd, "noconnect") && (!nType || nType == 2))
        bConnectPoints = false;
    if (matchParams(sCmd, "points") && (!nType || nType == 2))
        bDrawPoints = true;
    if (matchParams(sCmd, "nopoints") && (!nType || nType == 2))
        bDrawPoints = false;
    if (matchParams(sCmd, "open") && (!nType || nType == 1))
        bOpenImage = true;
    if (matchParams(sCmd, "noopen") && (!nType || nType == 1))
        bOpenImage = false;
    if (matchParams(sCmd, "interpolate") && (!nType || nType == 2))
        bInterpolate = true;
    if (matchParams(sCmd, "nointerpolate") && (!nType || nType == 2))
        bInterpolate = false;
    if (matchParams(sCmd, "hires") && (!nType || nType == 1))
        nHighResLevel = 2;
    if (matchParams(sCmd, "hires", '=') && (!nType || nType == 1))
    {
        int nPos = matchParams(sCmd, "hires", '=')+5;
        if (getArgAtPos(sCmd, nPos) == "all")
        {
            bAllHighRes = true;
            nHighResLevel = 2;
        }
        else if (getArgAtPos(sCmd, nPos) == "allmedium")
        {
            bAllHighRes = true;
            nHighResLevel = 1;
        }
        else if (getArgAtPos(sCmd, nPos) == "medium")
        {
            nHighResLevel = 1;
        }
    }
    if (matchParams(sCmd, "legend", '=') && (!nType || nType == 1))
    {
        int nPos = matchParams(sCmd, "legend", '=')+6;
        if (getArgAtPos(sCmd, nPos) == "topleft" || getArgAtPos(sCmd, nPos) == "left")
            nLegendPosition = 2;
        else if (getArgAtPos(sCmd, nPos) == "bottomleft")
            nLegendPosition = 0;
        else if (getArgAtPos(sCmd, nPos) == "bottomright")
            nLegendPosition = 1;
        else
            nLegendPosition = 3;
    }
    if (matchParams(sCmd, "nohires") && (!nType || nType == 1))
    {
        nHighResLevel = 0;
        bAllHighRes = false;
    }
    if (matchParams(sCmd, "animate") && (!nType || nType == 1))
        bAnimate = true;
    if (matchParams(sCmd, "animate", '=') && (!nType || nType == 1))
    {
        unsigned int nPos = matchParams(sCmd, "animate", '=')+7;
        _parser.SetExpr(getArgAtPos(__sCmd, nPos));
        nAnimateSamples = (int)_parser.Eval();
        if (nAnimateSamples && !isinf(_parser.Eval()) && !isnan(_parser.Eval()))
            bAnimate = true;
        else
        {
            nAnimateSamples = 50;
            bAnimate = false;
        }
        if (nAnimateSamples > 128)
            nAnimateSamples = 128;
        if (nAnimateSamples < 1)
            nAnimateSamples = 50;
    }
    if (matchParams(sCmd, "marks", '=') && (!nType || nType == 2))
    {
        unsigned int nPos = matchParams(sCmd, "marks", '=')+5;
        _parser.SetExpr(getArgAtPos(__sCmd, nPos));
        nMarks = (int)_parser.Eval();
        if (!nMarks || isinf(_parser.Eval()) || isnan(_parser.Eval()))
            nMarks = 0;
        if (nMarks > 9)
            nMarks = 9;
        if (nMarks < 0)
            nMarks = 0;
    }
    if (matchParams(sCmd, "nomarks") && (!nType || nType == 2))
        nMarks = 0;
    if (matchParams(sCmd, "textsize", '=') && (!nType || nType == 1))
    {
        unsigned int nPos = matchParams(sCmd, "textsize", '=')+8;
        _parser.SetExpr(getArgAtPos(__sCmd, nPos));
        nTextsize = (int)_parser.Eval();
        if (!nTextsize || isinf(_parser.Eval()) || isnan(_parser.Eval()))
            nTextsize = 5;
        if (nTextsize > 19)
            nTextsize = 19;
        if (nTextsize < 1)
            nTextsize = 1;
    }
    if (matchParams(sCmd, "aspect", '=') && (!nType || nType == 1))
    {
        unsigned int nPos = matchParams(sCmd, "aspect", '=') + 6;
        _parser.SetExpr(getArgAtPos(__sCmd, nPos));
        dAspect = _parser.Eval();
        if (dAspect <= 0 || isnan(dAspect) || isinf(dAspect))
            dAspect = 4/3;
    }
    if (matchParams(sCmd, "noanimate") && (!nType || nType == 1))
        bAnimate = false;
    if (matchParams(sCmd, "silent") && (!nType || nType == 1))
        bSilentMode = true;
    if (matchParams(sCmd, "nosilent") && (!nType || nType == 1))
        bSilentMode = false;
    if (matchParams(sCmd, "cut") && (!nType || nType == 2))
        bCutBox = true;
    if (matchParams(sCmd, "nocut") && (!nType || nType == 2))
        bCutBox = false;
    if (matchParams(sCmd, "flow") && (!nType || nType == 2))
    {
        bFlow = true;
        if (bPipe)
            bPipe = false;
    }
    if (matchParams(sCmd, "noflow") && (!nType || nType == 2))
        bFlow = false;
    if (matchParams(sCmd, "pipe") && (!nType || nType == 2))
    {
        bPipe = true;
        if (bFlow)
            bFlow = false;
    }
    if (matchParams(sCmd, "nopipe") && (!nType || nType == 2))
        bPipe = false;
    if (matchParams(sCmd, "flength") && (!nType || nType == 2))
        bFixedLength = true;
    if (matchParams(sCmd, "noflength") && (!nType || nType == 2))
        bFixedLength = false;
    if (matchParams(sCmd, "colorbar") && (!nType || nType == 2))
        bColorbar = true;
    if (matchParams(sCmd, "nocolorbar") && (!nType || nType == 2))
        bColorbar = false;
    if (matchParams(sCmd, "orthoproject") && (!nType || nType == 1))
        bOrthoProject = true;
    if (matchParams(sCmd, "noorthoproject") && (!nType || nType == 1))
        bOrthoProject = false;
    if (matchParams(sCmd, "area") && (!nType || nType == 2))
        bArea = true;
    if (matchParams(sCmd, "noarea") && (!nType || nType == 2))
        bArea = false;
    if (matchParams(sCmd, "bars") && (!nType || nType == 2))
    {
        dBars = 0.9;
        dHBars = 0.0;
    }
    if (matchParams(sCmd, "bars", '=') && (!nType || nType == 2))
    {
        _parser.SetExpr(getArgAtPos(__sCmd, matchParams(sCmd, "bars", '=')+4));
        dBars = _parser.Eval();
        if (dBars && !isinf(_parser.Eval()) && !isnan(_parser.Eval()) && (dBars < 0.0 || dBars > 1.0))
            dBars = 0.9;
        dHBars = 0.0;
    }
    if (matchParams(sCmd, "hbars") && (!nType || nType == 2))
    {
        dBars = 0.0;
        dHBars = 0.9;
    }
    if (matchParams(sCmd, "hbars", '=') && (!nType || nType == 2))
    {
        _parser.SetExpr(getArgAtPos(__sCmd, matchParams(sCmd, "hbars", '=')+5));
        dHBars = _parser.Eval();
        if (dHBars && !isinf(_parser.Eval()) && !isnan(_parser.Eval()) && (dHBars < 0.0 || dHBars > 1.0))
            dHBars = 0.9;
        dBars = 0.0;
    }
    if ((matchParams(sCmd, "nobars") || matchParams(sCmd, "nohbars")) && (!nType || nType == 2))
    {
        dBars = 0.0;
        dHBars = 0.0;
    }
    if (matchParams(sCmd, "steps") && (!nType || nType == 2))
        bStepPlot = true;
    if (matchParams(sCmd, "nosteps") && (!nType || nType == 2))
        bStepPlot = false;
    if (matchParams(sCmd, "boxplot") && (!nType || nType == 2))
        bBoxPlot = true;
    if (matchParams(sCmd, "noboxplot") && (!nType || nType == 2))
        bBoxPlot = false;
    if (matchParams(sCmd, "colormask") && (!nType || nType == 2))
        bColorMask = true;
    if (matchParams(sCmd, "nocolormask") && (!nType || nType == 2))
        bColorMask = false;
    if (matchParams(sCmd, "alphamask") && (!nType || nType == 2))
        bAlphaMask = true;
    if (matchParams(sCmd, "noalphamask") && (!nType || nType == 2))
        bAlphaMask = false;
    if (matchParams(sCmd, "schematic") && (!nType || nType == 1))
        bSchematic = true;
    if (matchParams(sCmd, "noschematic") && (!nType || nType == 1))
        bSchematic = false;
    if (matchParams(sCmd, "perspective", '=') && (!nType || nType == 1))
    {
        _parser.SetExpr(getArgAtPos(__sCmd, matchParams(sCmd, "perspective", '=')+11));
        dPerspective = fabs(_parser.Eval());
        if (dPerspective >= 1.0)
            dPerspective = 0.0;
    }
    if (matchParams(sCmd, "noperspective") && (!nType || nType == 1))
        dPerspective = 0.0;
    if (matchParams(sCmd, "cloudplot") && (!nType || nType == 2))
        bCloudPlot = true;
    if (matchParams(sCmd, "nocloudplot") && (!nType || nType == 2))
        bCloudPlot = false;
    if (matchParams(sCmd, "region") && (!nType || nType == 2))
        bRegion = true;
    if (matchParams(sCmd, "noregion") && (!nType || nType == 2))
        bRegion = false;
    if ((matchParams(sCmd, "crust") || matchParams(sCmd, "reconstruct")) && (!nType || nType == 2))
        bCrust = true;
    if ((matchParams(sCmd, "nocrust") || matchParams(sCmd, "noreconstruct")) && (!nType || nType == 2))
        bCrust = false;
    if (matchParams(sCmd, "maxline", '=') && (!nType || nType == 2))
    {
        string sTemp = getArgAtPos(__sCmd, matchParams(sCmd, "maxline", '=')+7);
        if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
            sTemp = sTemp.substr(1,sTemp.length()-2);
        _lHlines[0].sDesc = getArgAtPos(getNextArgument(sTemp, true),0);
        if (sTemp.length())
            _lHlines[0].sStyle = getArgAtPos(getNextArgument(sTemp, true),0);
        replaceControlChars(_lHlines[0].sDesc);
    }
    if (matchParams(sCmd, "minline", '=') && (!nType || nType == 2))
    {
        string sTemp = getArgAtPos(__sCmd, matchParams(sCmd, "minline", '=')+7);
        if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
            sTemp = sTemp.substr(1,sTemp.length()-2);
        _lHlines[1].sDesc = getArgAtPos(getNextArgument(sTemp, true),0);
        if (sTemp.length())
            _lHlines[1].sStyle = getArgAtPos(getNextArgument(sTemp, true),0);
        replaceControlChars(_lHlines[1].sDesc);
    }
    if (matchParams(sCmd, "hline", '=') && (!nType || nType == 2))
    {
        string sTemp = getArgAtPos(__sCmd, matchParams(sCmd, "hline", '=')+5);
        if (sTemp.find(',') != string::npos)
        {
            if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
                sTemp = sTemp.substr(1,sTemp.length()-2);
            _parser.SetExpr(getNextArgument(sTemp, true));
            _lHlines[2].dPos = _parser.Eval();
            _lHlines[2].sDesc = getArgAtPos(getNextArgument(sTemp, true),0);
            if (sTemp.length())
                _lHlines[2].sStyle = getArgAtPos(getNextArgument(sTemp, true),0);
        }
        replaceControlChars(_lHlines[2].sDesc);
    }
    if (matchParams(sCmd, "vline", '=') && (!nType || nType == 2))
    {
        string sTemp = getArgAtPos(__sCmd, matchParams(sCmd, "vline", '=')+5);
        if (sTemp.find(',') != string::npos)
        {
            if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
                sTemp = sTemp.substr(1,sTemp.length()-2);
            _parser.SetExpr(getNextArgument(sTemp, true));
            _lVLines[2].dPos = _parser.Eval();
            _lVLines[2].sDesc = getArgAtPos(getNextArgument(sTemp, true),0);
            if (sTemp.length())
                _lVLines[2].sStyle = getArgAtPos(getNextArgument(sTemp, true),0);
        }
        replaceControlChars(_lVLines[2].sDesc);
    }
    if (matchParams(sCmd, "lborder", '=') && (!nType || nType == 2))
    {
        string sTemp = getArgAtPos(__sCmd, matchParams(sCmd, "lborder", '=')+7);
        if (sTemp.find(',') != string::npos)
        {
            if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
                sTemp = sTemp.substr(1,sTemp.length()-2);
            _parser.SetExpr(getNextArgument(sTemp, true));
            _lVLines[0].dPos = _parser.Eval();
            _lVLines[0].sDesc = getArgAtPos(getNextArgument(sTemp, true),0);
            if (sTemp.length())
                _lVLines[0].sStyle = getArgAtPos(getNextArgument(sTemp, true),0);
        }
        replaceControlChars(_lVLines[0].sDesc);
    }
    if (matchParams(sCmd, "rborder", '=') && (!nType || nType == 2))
    {
        string sTemp = getArgAtPos(__sCmd, matchParams(sCmd, "rborder", '=')+7);
        if (sTemp.find(',') != string::npos)
        {
            if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
                sTemp = sTemp.substr(1,sTemp.length()-2);
            _parser.SetExpr(getNextArgument(sTemp, true));
            _lVLines[1].dPos = _parser.Eval();
            _lVLines[1].sDesc = getArgAtPos(getNextArgument(sTemp, true),0);
            if (sTemp.length())
                _lVLines[1].sStyle = getArgAtPos(getNextArgument(sTemp, true),0);
        }
        replaceControlChars(_lVLines[1].sDesc);
    }
    if (matchParams(sCmd, "addxaxis", '=') && (!nType || nType == 1))
    {
        string sTemp = getArgAtPos(__sCmd, matchParams(sCmd, "addxaxis", '=')+8);
        if (sTemp.find(',') != string::npos || sTemp.find('"') != string::npos)
        {
            if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
                sTemp = sTemp.substr(1,sTemp.length()-2);
            if (getNextArgument(sTemp, false).front() != '"')
            {
                _parser.SetExpr(getNextArgument(sTemp, true));
                _AddAxes[0].dMin = _parser.Eval();
                _parser.SetExpr(getNextArgument(sTemp, true));
                _AddAxes[0].dMax = _parser.Eval();
                if (getNextArgument(sTemp, false).length())
                {
                    _AddAxes[0].sLabel = "@{"+getArgAtPos(getNextArgument(sTemp, true),0)+"}";
                    if (getNextArgument(sTemp, false).length())
                    {
                        _AddAxes[0].sStyle = getArgAtPos(getNextArgument(sTemp, true),0);
                        if (!checkColorChars(_AddAxes[0].sStyle))
                            _AddAxes[0].sStyle = "k";
                    }
                }
                else
                {
                    _AddAxes[0].sLabel = "@{\\i x}";
                }
            }
            else
            {
                _AddAxes[0].sLabel = "@{"+getArgAtPos(getNextArgument(sTemp, true),0)+"}";
                if (getNextArgument(sTemp, false).length())
                {
                    _AddAxes[0].sStyle = getArgAtPos(getNextArgument(sTemp, true),0);
                    if (!checkColorChars(_AddAxes[0].sStyle))
                        _AddAxes[0].sStyle = "k";
                }
            }
        }
        //replaceControlChars(_[1].sDesc);
    }
    if (matchParams(sCmd, "addyaxis", '=') && (!nType || nType == 1))
    {
        string sTemp = getArgAtPos(__sCmd, matchParams(sCmd, "addyaxis", '=')+8);
        if (sTemp.find(',') != string::npos || sTemp.find('"') != string::npos)
        {
            if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
                sTemp = sTemp.substr(1,sTemp.length()-2);
            if (getNextArgument(sTemp, false).front() != '"')
            {
                _parser.SetExpr(getNextArgument(sTemp, true));
                _AddAxes[1].dMin = _parser.Eval();
                _parser.SetExpr(getNextArgument(sTemp, true));
                _AddAxes[1].dMax = _parser.Eval();
                if (getNextArgument(sTemp, false).length())
                {
                    _AddAxes[1].sLabel = "@{"+getArgAtPos(getNextArgument(sTemp, true),0) + "}";
                    if (getNextArgument(sTemp, false).length())
                    {
                        _AddAxes[1].sStyle = getArgAtPos(getNextArgument(sTemp, true),0);
                        if (!checkColorChars(_AddAxes[0].sStyle))
                            _AddAxes[1].sStyle = "k";
                    }
                }
                else
                {
                    _AddAxes[1].sLabel = "@{\\i y}";
                }
            }
            else
            {
                _AddAxes[1].sLabel = "@{"+getArgAtPos(getNextArgument(sTemp, true),0)+"}";
                if (getNextArgument(sTemp, false).length())
                {
                    _AddAxes[1].sStyle = getArgAtPos(getNextArgument(sTemp, true),0);
                    if (!checkColorChars(_AddAxes[1].sStyle))
                        _AddAxes[1].sStyle = "k";
                }
            }
        }
        //replaceControlChars(_[1].sDesc);
    }
    if (matchParams(sCmd, "colorscheme", '=') && (!nType || nType == 2))
    {
        unsigned int nPos = matchParams(sCmd, "colorscheme", '=') + 11;
        while (sCmd[nPos] == ' ')
            nPos++;
        if (sCmd[nPos] == '"')
        {
            string __sColorScheme = __sCmd.substr(nPos+1, __sCmd.find('"', nPos+1)-nPos-1);
            StripSpaces(__sColorScheme);
            if (!checkColorChars(__sColorScheme))
                sColorScheme = "kRryw";
            else
            {
                if (__sColorScheme == "#" && sColorScheme.find('#') == string::npos)
                    sColorScheme += '#';
                else if (__sColorScheme == "|" && sColorScheme.find('|') == string::npos)
                    sColorScheme += '|';
                else if ((__sColorScheme == "#|" || __sColorScheme == "|#") && (sColorScheme.find('#') == string::npos || sColorScheme.find('|') == string::npos))
                {
                    if (sColorScheme.find('#') == string::npos && sColorScheme.find('|') != string::npos)
                        sColorScheme += '#';
                    else if (sColorScheme.find('|') == string::npos && sColorScheme.find('#') != string::npos)
                        sColorScheme += '|';
                    else
                        sColorScheme += "#|";
                }
                else if (__sColorScheme != "#" && __sColorScheme != "|" && __sColorScheme != "#|" && __sColorScheme != "|#")
                    sColorScheme = __sColorScheme;
            }
            //sColorScheme = __sCmd.substr(nPos+1, __sCmd.find('"', nPos+1)-nPos-1);
            //StripSpaces(sColorScheme);
            //if (!checkColorChars(sColorScheme))
            //    sColorScheme = "BbcyrR";
        }
        else
        {
            string sTemp = sCmd.substr(nPos, sCmd.find(' ', nPos+1)-nPos);
            StripSpaces(sTemp);
            if (sTemp == "rainbow")
                sColorScheme = "BbcyrR";
            else if (sTemp == "grey")
                sColorScheme = "kw";
            else if (sTemp == "hot")
                sColorScheme = "kRryw";
            else if (sTemp == "cold")
                sColorScheme = "kBncw";
            else if (sTemp == "copper")
                sColorScheme = "kQqw";
            else if (sTemp == "map")
                sColorScheme = "UBbcgyqRH";
            else if (sTemp == "moy")
                sColorScheme = "kMqyw";
            else if (sTemp == "coast")
                sColorScheme = "BCyw";
            else if (sTemp == "viridis" || sTemp == "std")
                sColorScheme = "UNC{e4}y";
            else if (sTemp == "plasma")
                sColorScheme = "B{u4}p{q6}{y7}";
            else
                sColorScheme = "kRryw";
        }
        if (sColorScheme.length() > 32)
        {
            sColorScheme = "BbcyrR";
            sColorSchemeMedium = "{B4}{b4}{c4}{y4}{r4}{R4}";
            sColorSchemeLight = "{B8}{b8}{c8}{y8}{r8}{R8}";
        }
        else
        {
            while (sColorScheme.find(' ') != string::npos)
            {
                sColorScheme.erase(sColorScheme.find(' '),1);
            }
            sColorSchemeLight = "";
            sColorSchemeMedium = "";
            for (unsigned int i = 0; i < sColorScheme.length(); i++)
            {
                if (sColorScheme[i] == '#' || sColorScheme[i] == '|')
                {
                    sColorSchemeLight += sColorScheme[i];
                    sColorSchemeMedium += sColorScheme[i];
                    continue;
                }
                if (sColorScheme[i] == '{' && i+3 < sColorScheme.length() && sColorScheme[i+3] == '}')
                {
                    sColorSchemeLight += "{";
                    sColorSchemeMedium += "{";
                    sColorSchemeLight += sColorScheme[i+1];
                    sColorSchemeMedium += sColorScheme[i+1];
                    if (sColorScheme[i+2] >= '2' && sColorScheme[i+2] <= '8')
                    {
                        sColorSchemeMedium += sColorScheme[i+2]-1;
                        if (sColorScheme[i+2] < '6')
                            sColorSchemeLight += sColorScheme[i+2]+3;
                        else
                            sColorSchemeLight += "9";
                        sColorSchemeLight += "}";
                        sColorSchemeMedium += "}";
                    }
                    else
                    {
                        sColorSchemeLight += sColorScheme.substr(i+2,2);
                        sColorSchemeMedium += sColorScheme.substr(i+2,2);
                    }
                    i += 3;
                    continue;
                }
                sColorSchemeLight += "{";
                sColorSchemeLight += sColorScheme[i];
                sColorSchemeLight += "8}";
                sColorSchemeMedium += "{";
                sColorSchemeMedium += sColorScheme[i];
                sColorSchemeMedium += "4}";
            }
        }
    }
    if (matchParams(sCmd, "bgcolorscheme", '=') && (!nType || nType == 2))
    {
        unsigned int nPos = matchParams(sCmd, "bgcolorscheme", '=') + 13;
        while (sCmd[nPos] == ' ')
            nPos++;
        if (sCmd[nPos] == '"')
        {
            string __sBGColorScheme = __sCmd.substr(nPos+1, __sCmd.find('"', nPos+1)-nPos-1);
            StripSpaces(__sBGColorScheme);
            if (!checkColorChars(__sBGColorScheme))
                sBackgroundColorScheme = "kRryw";
            else
            {
                if (__sBGColorScheme == "#" && sBackgroundColorScheme.find('#') == string::npos)
                    sBackgroundColorScheme += '#';
                else if (__sBGColorScheme == "|" && sBackgroundColorScheme.find('|') == string::npos)
                    sBackgroundColorScheme += '|';
                else if ((__sBGColorScheme == "#|" || __sBGColorScheme == "|#") && (sBackgroundColorScheme.find('#') == string::npos || sBackgroundColorScheme.find('|') == string::npos))
                {
                    if (sBackgroundColorScheme.find('#') == string::npos && sBackgroundColorScheme.find('|') != string::npos)
                        sBackgroundColorScheme += '#';
                    else if (sBackgroundColorScheme.find('|') == string::npos && sBackgroundColorScheme.find('#') != string::npos)
                        sBackgroundColorScheme += '|';
                    else
                        sBackgroundColorScheme += "#|";
                }
                else if (__sBGColorScheme != "#" && __sBGColorScheme != "|" && __sBGColorScheme != "#|" && __sBGColorScheme != "|#")
                    sBackgroundColorScheme = __sBGColorScheme;
            }
            /*sBackgroundColorScheme = __sCmd.substr(nPos+1, __sCmd.find('"', nPos+1)-nPos-1);
            StripSpaces(sBackgroundColorScheme);
            if (!checkColorChars(sBackgroundColorScheme))
                sBackgroundColorScheme = "BbcyrR";*/
        }
        else
        {
            string sTemp = sCmd.substr(nPos, sCmd.find(' ', nPos+1)-nPos);
            StripSpaces(sTemp);
            if (sTemp == "rainbow")
                sBackgroundColorScheme = "BbcyrR";
            else if (sTemp == "grey")
                sBackgroundColorScheme = "kw";
            else if (sTemp == "hot")
                sBackgroundColorScheme = "kRryw";
            else if (sTemp == "cold")
                sBackgroundColorScheme = "kBbcw";
            else if (sTemp == "copper")
                sBackgroundColorScheme = "kQqw";
            else if (sTemp == "map")
                sBackgroundColorScheme = "UBbcgyqRH";
            else if (sTemp == "moy")
                sBackgroundColorScheme = "kMqyw";
            else if (sTemp == "coast")
                sBackgroundColorScheme = "BCyw";
            else if (sTemp == "viridis" || sTemp == "std")
                sBackgroundColorScheme = "UNC{e4}y";
            else if (sTemp == "plasma")
                sBackgroundColorScheme = "B{u4}p{q6}{y7}";
            else if (sTemp == "real")
                sBackgroundColorScheme = "<<REALISTIC>>";
            else
                sBackgroundColorScheme = "BbcyrR";
        }
        if (sBackgroundColorScheme.length() > 32)
        {
            sBackgroundColorScheme = "BbcyrR";
        }
        while (sBackgroundColorScheme.find(' ') != string::npos)
        {
            sBackgroundColorScheme = sBackgroundColorScheme.substr(0, sBackgroundColorScheme.find(' ')) + sBackgroundColorScheme.substr(sBackgroundColorScheme.find(' ')+1);
        }
    }
    if (matchParams(sCmd, "plotcolors", '=') && (!nType || nType == 2))
    {
        unsigned int nPos = matchParams(sCmd, "plotcolors", '=')+10;
        string sTemp = getArgAtPos(__sCmd, nPos);
        if (checkColorChars(sTemp))
        {
            for (unsigned int i = 0; i < sTemp.length(); i++)
            {
                if (i >= 14)
                    break;
                if (sTemp[i] == ' ')
                    continue;
                sColors[i] = sTemp[i];
            }
        }
    }
    if (matchParams(sCmd, "axisbind", '=') && (!nType || nType == 2))
    {
        unsigned int nPos = matchParams(sCmd, "axisbind", '=')+8;
        string sTemp = getArgAtPos(__sCmd, nPos);
        for (unsigned int i = 0; i < sTemp.length(); i++)
        {
            if (sTemp[i] == 'r' || sTemp[i] == 'l')
            {
                if (sTemp.length() > i+1 && (sTemp[i+1] == 't' || sTemp[i+1] == 'b'))
                {
                    sAxisBind += sTemp.substr(i,2);
                    i++;
                }
                else if (sTemp.length() > i+1 && (sTemp[i+1] == ' ' || sTemp[i+1] == 'r' || sTemp[i+1] == 'l'))
                {
                    sAxisBind += sTemp.substr(i,1) + "b";
                    if (sTemp[i+1] == ' ')
                        i++;
                }
                else if (sTemp.length() == i+1)
                    sAxisBind += sTemp.substr(i) + "b";
                else
                    sAxisBind += "lb";
            }
            else if (sTemp[i] == 't' || sTemp[i] == 'b')
            {
                if (sTemp.length() > i+1 && (sTemp[i+1] == 'l' || sTemp[i+1] == 'r'))
                {
                    sAxisBind += sTemp.substr(i+1,1) + sTemp.substr(i,1);
                    i++;
                }
                else if (sTemp.length() > i+1 && (sTemp[i+1] == ' ' || sTemp[i+1] == 't' || sTemp[i+1] == 'b'))
                {
                    sAxisBind += "l" + sTemp.substr(i,1);
                    if (sTemp[i+1] == ' ')
                        i++;
                }
                else if (sTemp.length() == i+1)
                    sAxisBind += "l" + sTemp.substr(i);
                else
                    sAxisBind += "lb";
            }
            else if (sTemp.substr(i,2) == "  ")
            {
                sAxisBind += "lb";
                i++;
            }
        }
        if (sAxisBind.find('l') == string::npos && sAxisBind.length())
        {
            for (unsigned int i = 0; i < sAxisBind.length(); i++)
            {
                if (sAxisBind[i] == 'r')
                    sAxisBind[i] = 'l';
            }
        }
        if (sAxisBind.find('b') == string::npos && sAxisBind.length())
        {
            for (unsigned int i = 0; i < sAxisBind.length(); i++)
            {
                if (sAxisBind[i] == 't')
                    sAxisBind[i] = 'b';
            }
        }
    }
    if (matchParams(sCmd, "linestyles", '=') && (!nType || nType == 2))
    {
        unsigned int nPos = matchParams(sCmd, "linestyles", '=')+10;
        string sTemp = getArgAtPos(__sCmd, nPos);
        if (checkLineChars(sTemp))
        {
            for (unsigned int i = 0; i < sTemp.length(); i++)
            {
                if (i >= 14)
                    break;
                if (sTemp[i] == ' ')
                    continue;
                sLineStyles[i] = sTemp[i];
                sLineStylesGrey[i] = sTemp[i];
            }
        }
    }
    if (matchParams(sCmd, "linesizes", '=') && (!nType || nType == 2))
    {
        unsigned int nPos = matchParams(sCmd, "linesizes", '=')+9;
        string sTemp = getArgAtPos(__sCmd, nPos);

        for (unsigned int i = 0; i < sTemp.length(); i++)
        {
            if (i >= 14)
                break;
            if (sTemp[i] == ' ')
                continue;
            if (sTemp[i] < '0' || sTemp[i] > '9')
                continue;
            sLineSizes[i] = sTemp[i];
        }

    }
    if (matchParams(sCmd, "pointstyles", '=') && (!nType || nType == 2))
    {
        unsigned int nPos = matchParams(sCmd, "pointstyles", '=')+11;
        string sTemp = getArgAtPos(__sCmd, nPos);
        if (checkPointChars(sTemp))
        {
            int nChar = 0;
            string sChar = "";
            for (unsigned int i = 0; i < sTemp.length(); i++)
            {
                sChar = "";
                if (i >= 28 || nChar >= 14)
                    break;
                if (sTemp[i] == ' ')
                {
                    nChar++;
                    continue;
                }
                if (sTemp[i] == '#' && i+1 < sTemp.length() && sTemp[i+1] != ' ')
                {
                    sChar = "#";
                    i++;
                    sChar += sTemp[i];
                    sPointStyles.replace(2*nChar, 2, sChar);
                    nChar++;
                    continue;
                }
                if (sTemp[i] != '#')
                {
                    sChar = " ";
                    sChar += sTemp[i];
                    sPointStyles.replace(2*nChar, 2, sChar);
                    nChar++;
                }
            }
        }
    }
    if (matchParams(sCmd, "styles", '=') && (!nType || nType == 2))
    {
        unsigned int nPos = matchParams(sCmd, "styles", '=')+6;
        string sTemp = getArgAtPos(__sCmd, nPos);
        unsigned int nJump = 0;
        unsigned int nStyle = 0;
        for (unsigned int i = 0; i < sTemp.length(); i += 4)
        {
            nJump = 0;
            if (nStyle >= 14)
                break;
            if (sTemp.substr(i,4).find('#') != string::npos)
                nJump = 1;
            for (unsigned int j = 0; j < 4+nJump; j++)
            {
                if (i+j >= sTemp.length())
                    break;
                if (sTemp[i+j] == ' ')
                    continue;
                if (sTemp[i+j] >= '0' && sTemp[i+j] <= '9')
                {
                    sLineSizes[nStyle] = sTemp[i+j];
                    continue;
                }
                if (sTemp[i+j] == '#' && i+j+1 < sTemp.length() && checkPointChars(sTemp.substr(i+j,2)))
                {
                    sPointStyles[2*nStyle] = '#';
                    if (sTemp[i+j+1] != ' ')
                        sPointStyles[2*nStyle+1] = sTemp[i+j+1];
                    j++;
                    continue;
                }
                else if (sTemp[i+j] == '#')
                    continue;
                if (checkPointChars(sTemp.substr(i+j,1)))
                {
                    sPointStyles[2*nStyle] = ' ';
                    sPointStyles[2*nStyle+1] = sTemp[i+j];
                    continue;
                }
                if (checkColorChars(sTemp.substr(i+j,1)))
                {
                    sColors[nStyle] = sTemp[i+j];
                    continue;
                }
                if (checkLineChars(sTemp.substr(i+j,1)))
                {
                    sLineStyles[nStyle] = sTemp[i+j];
                    sLineStylesGrey[nStyle] = sTemp[i+j];
                    continue;
                }
            }
            nStyle++;
            i += nJump;
        }
    }
    if (matchParams(sCmd, "gridstyle", '=') && (!nType || nType == 1))
    {
        unsigned int nPos = matchParams(sCmd, "gridstyle", '=')+9;
        string sTemp = getArgAtPos(__sCmd, nPos);
        for (unsigned int i = 0; i < sTemp.length(); i += 3)
        {
            for (unsigned int j = 0; j < 3; j++)
            {
                if (i+j >= sTemp.length())
                    break;
                if (sTemp[i+j] == ' ')
                    continue;
                if (sTemp[i+j] >= '0' && sTemp[i+j] <= '9')
                {
                    sGridStyle[2+i] = sTemp[i+j];
                    continue;
                }
                if (sTemp[i+j] == '#')
                    continue;
                if (checkPointChars(sTemp.substr(i+j,1)))
                {
                    continue;
                }
                if (checkColorChars(sTemp.substr(i+j,1)))
                {
                    sGridStyle[i] = sTemp[i+j];
                    continue;
                }
                if (checkLineChars(sTemp.substr(i+j,1)))
                {
                    sGridStyle[i+1] = sTemp[i+j];
                    continue;
                }
            }
        }
    }
    if (matchParams(sCmd, "legendstyle", '=') && (!nType || nType == 2))
    {
        if (getArgAtPos(sCmd, matchParams(sCmd, "legendstyle", '=')+11) == "onlycolors")
            nLegendstyle = 1;
        else if (getArgAtPos(sCmd, matchParams(sCmd, "legendstyle", '=')+11) == "onlystyles")
            nLegendstyle = 2;
        else
            nLegendstyle = 0;
    }
    if (matchParams(sCmd, "coords", '=') && (!nType || nType == 1))
    {
        int nPos = matchParams(sCmd, "coords", '=')+6;
        if (getArgAtPos(sCmd, nPos) == "cartesian" || getArgAtPos(sCmd, nPos) == "std")
        {
            nCoords = CARTESIAN;
        }
        else if (getArgAtPos(sCmd, nPos) == "polar" || getArgAtPos(sCmd, nPos) == "polar_pz" || getArgAtPos(sCmd, nPos) == "cylindrical")
        {
            nCoords = POLAR_PZ;
        }
        else if (getArgAtPos(sCmd, nPos) == "polar_rp")
        {
            nCoords = POLAR_RP;
        }
        else if (getArgAtPos(sCmd, nPos) == "polar_rz")
        {
            nCoords = POLAR_RZ;
        }
        else if (getArgAtPos(sCmd, nPos) == "spherical" || getArgAtPos(sCmd, nPos) == "spherical_pt")
        {
            nCoords = SPHERICAL_PT;
        }
        else if (getArgAtPos(sCmd, nPos) == "spherical_rp")
        {
            nCoords = SPHERICAL_RP;
        }
        else if (getArgAtPos(sCmd, nPos) == "spherical_rt")
        {
            nCoords = SPHERICAL_RT;
        }
    }
    if (matchParams(sCmd, "font", '=') && (!nType || nType == 1))
    {
        string sTemp = getArgAtPos(sCmd, matchParams(sCmd, "font", '=')+4);
        StripSpaces(sTemp);
        if (sTemp == "palatino")
            sTemp = "pagella";
        if (sTemp == "times")
            sTemp = "termes";
        if (sTemp == "bookman")
            sTemp = "bonum";
        if (sTemp == "avantgarde")
            sTemp = "adventor";
        if (sTemp == "chancery")
            sTemp = "chorus";
        if (sTemp == "courier")
            sTemp = "cursor";
        if (sTemp == "helvetica")
            sTemp = "heros";
        if (sTemp != sFontStyle
            && (sTemp == "pagella"
                || sTemp == "adventor"
                || sTemp == "bonum"
                || sTemp == "chorus"
                || sTemp == "cursor"
                || sTemp == "heros"
                || sTemp == "heroscn"
                || sTemp == "schola"
                || sTemp == "termes")
            )
        {
            sFontStyle = sTemp;
            _fontData.LoadFont(sFontStyle.c_str(), (sTokens[0][1]+ "\\fonts").c_str());
        }
    }
    if ((matchParams(sCmd, "opng", '=')
        || matchParams(sCmd, "save", '=')
        || matchParams(sCmd, "export", '=')
        || matchParams(sCmd, "opnga", '=')
        || matchParams(sCmd, "oeps", '=')
        || matchParams(sCmd, "osvg", '=')
        || matchParams(sCmd, "otex", '=')
        || matchParams(sCmd, "ogif", '=')) && (!nType || nType == 1))
    {
        unsigned int nPos = 0;
        if (matchParams(sCmd, "opng", '='))
            nPos = matchParams(sCmd, "opng", '=') + 4;
        else if (matchParams(sCmd, "opnga", '='))
            nPos = matchParams(sCmd, "opnga", '=') + 5;
        else if (matchParams(sCmd, "save", '='))
            nPos = matchParams(sCmd, "save", '=') + 4;
        else if (matchParams(sCmd, "export", '='))
            nPos = matchParams(sCmd, "export", '=') + 6;
        else if (matchParams(sCmd, "oeps", '='))
            nPos = matchParams(sCmd, "oeps", '=') + 4;
        else if (matchParams(sCmd, "osvg", '='))
            nPos = matchParams(sCmd, "osvg", '=') + 4;
        else if (matchParams(sCmd, "otex", '='))
            nPos = matchParams(sCmd, "otex", '=') + 4;
        else if (matchParams(sCmd, "ogif", '='))
            nPos = matchParams(sCmd, "ogif", '=') + 4;

        sFileName = getArgAtPos(__sCmd, nPos);
        StripSpaces(sFileName);
        if (sFileName.length())
        {
            string sExtension = "";
            if (sFileName.length() > 4)
                sExtension = sFileName.substr(sFileName.length()-4,4);
            if (sExtension != ".png"
                && (matchParams(sCmd, "opng", '=')
                    || matchParams(sCmd, "opnga", '=')))
                sFileName += ".png";
            else if (sExtension != ".eps" && matchParams(sCmd, "oeps", '='))
                sFileName += ".eps";
            else if (sExtension != ".svg" && matchParams(sCmd, "osvg", '='))
                sFileName += ".svg";
            else if (sExtension != ".tex" && matchParams(sCmd, "otex", '='))
                sFileName += ".tex";
            else if (sExtension != ".gif" && matchParams(sCmd, "ogif", '='))
                sFileName += ".gif";
            else if ((matchParams(sCmd, "export", '=') || matchParams(sCmd, "save", '=')) && sFileName.rfind('.') == string::npos)
                sFileName += ".png";

            sFileName = FileSystem::ValidFileName(sFileName, sFileName.substr(sFileName.rfind('.')));
        }
    }
    if ((matchParams(sCmd, "xlabel", '=')
        || matchParams(sCmd, "ylabel", '=')
        || matchParams(sCmd, "zlabel", '=')
        || matchParams(sCmd, "title", '=')
        || matchParams(sCmd, "background", '=')) && (!nType || nType == 1))
    {
        int nPos = 0;
        if (matchParams(sCmd, "xlabel", '='))
        {
            nPos = matchParams(sCmd, "xlabel", '=') + 6;
            sAxisLabels[0] = getArgAtPos(__sCmd, nPos);
        }
        if (matchParams(sCmd, "ylabel", '='))
        {
            nPos = matchParams(sCmd, "ylabel", '=') + 6;
            sAxisLabels[1] = getArgAtPos(__sCmd, nPos);
        }
        if (matchParams(sCmd, "zlabel", '='))
        {
            nPos = matchParams(sCmd, "zlabel", '=') + 6;
            sAxisLabels[2] = getArgAtPos(__sCmd, nPos);
        }
        if (matchParams(sCmd, "title", '='))
        {
            nPos = matchParams(sCmd, "title", '=') + 5;
            sPlotTitle = getArgAtPos(__sCmd, nPos);
            StripSpaces(sPlotTitle);
        }
        if (matchParams(sCmd, "background", '='))
        {
            nPos = matchParams(sCmd, "background", '=')+10;
            sBackground = getArgAtPos(__sCmd, nPos);
            StripSpaces(sBackground);
            if (sBackground.length())
            {
                if (sBackground.find('.') == string::npos)
                    sBackground += ".png";
                else if (sBackground.substr(sBackground.rfind('.')) != ".png")
                    sBackground = "";
                if (sBackground.length())
                {
                    sBackground = FileSystem::ValidFileName(sBackground, ".png");
                }
            }
        }
        for (int i = 0; i < 3; i++)
        {
            StripSpaces(sAxisLabels[i]);
            if (sAxisLabels[i].length())
                sAxisLabels[i] = "@{" + sAxisLabels[i] + "}";
        }
    }
    if ((matchParams(sCmd, "xticks", '=')
        || matchParams(sCmd, "yticks", '=')
        || matchParams(sCmd, "zticks", '=')
        || matchParams(sCmd, "cticks", '=')) && (!nType || nType == 1))
    {
        if (matchParams(sCmd, "xticks", '='))
        {
            sTickTemplate[0] = getArgAtPos(__sCmd, matchParams(sCmd, "xticks", '=')+6);
            if (sTickTemplate[0].find('%') == string::npos && sTickTemplate[0].length())
                sTickTemplate[0] += "%g";
        }
        if (matchParams(sCmd, "yticks", '='))
        {
            sTickTemplate[1] = getArgAtPos(__sCmd, matchParams(sCmd, "yticks", '=')+6);
            if (sTickTemplate[1].find('%') == string::npos && sTickTemplate[1].length())
                sTickTemplate[1] += "%g";
        }
        if (matchParams(sCmd, "zticks", '='))
        {
            sTickTemplate[2] = getArgAtPos(__sCmd, matchParams(sCmd, "zticks", '=')+6);
            if (sTickTemplate[2].find('%') == string::npos && sTickTemplate[2].length())
                sTickTemplate[2] += "%g";
        }
        if (matchParams(sCmd, "cticks", '='))
        {
            sTickTemplate[3] = getArgAtPos(__sCmd, matchParams(sCmd, "cticks", '=')+6);
            if (sTickTemplate[3].find('%') == string::npos && sTickTemplate[3].length())
                sTickTemplate[3] += "%g";
        }
    }
    if ((matchParams(sCmd, "xscale", '=')
        || matchParams(sCmd, "yscale", '=')
        || matchParams(sCmd, "zscale", '=')
        || matchParams(sCmd, "cscale", '=')) && (!nType || nType == 1))
    {
        if (matchParams(sCmd, "xscale", '='))
        {
            _parser.SetExpr(getArgAtPos(__sCmd, matchParams(sCmd, "xscale", '=')+6));
            dAxisScale[0] = _parser.Eval();
        }
        if (matchParams(sCmd, "yscale", '='))
        {
            _parser.SetExpr(getArgAtPos(__sCmd, matchParams(sCmd, "yscale", '=')+6));
            dAxisScale[1] = _parser.Eval();
        }
        if (matchParams(sCmd, "zscale", '='))
        {
            _parser.SetExpr(getArgAtPos(__sCmd, matchParams(sCmd, "zscale", '=')+6));
            dAxisScale[2] = _parser.Eval();
        }
        if (matchParams(sCmd, "cscale", '='))
        {
            _parser.SetExpr(getArgAtPos(__sCmd, matchParams(sCmd, "cscale", '=')+6));
            dAxisScale[3] = _parser.Eval();
        }

        for (int i = 0; i < 4; i++)
        {
            if (dAxisScale[i] == 0)
                dAxisScale[i] = 1.0;
        }
    }
    if ((matchParams(sCmd, "xticklabels", '=')
        || matchParams(sCmd, "yticklabels", '=')
        || matchParams(sCmd, "zticklabels", '=')
        || matchParams(sCmd, "cticklabels", '=')) && (!nType || nType == 1))
    {
        if (matchParams(sCmd, "xticklabels", '='))
        {
            sCustomTicks[0] = getArgAtPos(__sCmd, matchParams(sCmd, "xticklabels", '=')+11);
        }
        if (matchParams(sCmd, "yticklabels", '='))
        {
            sCustomTicks[1] = getArgAtPos(__sCmd, matchParams(sCmd, "yticklabels", '=')+11);
        }
        if (matchParams(sCmd, "zticklabels", '='))
        {
            sCustomTicks[2] = getArgAtPos(__sCmd, matchParams(sCmd, "zticklabels", '=')+11);
        }
        if (matchParams(sCmd, "cticklabels", '='))
        {
            sCustomTicks[3] = getArgAtPos(__sCmd, matchParams(sCmd, "cticklabels", '=')+11);
        }
    }
    if (sCmd.find('[') != string::npos && (!nType || nType == 1))
    {
        unsigned int nPos = 0;

        do
        {
            nPos = sCmd.find('[', nPos);
            if (nPos == string::npos)
                break;
            nPos++;
        }
        while (isInQuotes(sCmd, nPos));

        if (nPos != string::npos && sCmd.find(']', nPos) != string::npos)
        {
            string sRanges[3];
            sRanges[0] = __sCmd.substr(nPos, sCmd.find(']', nPos) - nPos);
            int i = 0;
            while (sRanges[0].find(',') != string::npos)
            {
                if (i == 3)
                {
                    sRanges[0] = "(" + sRanges[0] + ")";
                    parser_SplitArgs(sRanges[0], sRanges[2], ',', _option, false);
                    if (sRanges[0].find(':') == string::npos)
                    {
                        sRanges[0] = sRanges[2];
                        i++;
                        continue;
                    }
                    sRanges[0] = "(" + sRanges[0] + ")";
                    parser_SplitArgs(sRanges[0], sRanges[1], ':', _option, false);
                    if (parser_ExprNotEmpty(sRanges[0]))
                    {
                        _parser.SetExpr(sRanges[0]);
                        dColorRange[0] = (double)_parser.Eval();
                    }
                    if (parser_ExprNotEmpty(sRanges[1]))
                    {
                        _parser.SetExpr(sRanges[1]);
                        dColorRange[1] = (double)_parser.Eval();
                    }
                    if (isnan(dColorRange[0]) || isnan(dColorRange[1]) || isinf(dColorRange[0]) || isinf(dColorRange[1]))
                    {
                        dColorRange[0] = NAN;
                        dColorRange[1] = NAN;
                    }
                }
                else if (i == 4)
                    return;
                else
                {
                    sRanges[0] = "(" + sRanges[0] + ")";
                    parser_SplitArgs(sRanges[0], sRanges[2], ',', _option, false);
                    if (sRanges[0].find(':') == string::npos)
                    {
                        sRanges[0] = sRanges[2];
                        i++;
                        continue;
                    }
                    sRanges[0] = "(" + sRanges[0] + ")";
                    parser_SplitArgs(sRanges[0], sRanges[1], ':', _option, false);
                    if (parser_ExprNotEmpty(sRanges[0]))
                    {
                        _parser.SetExpr(sRanges[0]);
                        dRanges[i][0] = (double)_parser.Eval();
                    }
                    if (parser_ExprNotEmpty(sRanges[1]))
                    {
                        _parser.SetExpr(sRanges[1]);
                        dRanges[i][1] = (double)_parser.Eval();
                    }
                    if (parser_ExprNotEmpty(sRanges[0]) || parser_ExprNotEmpty(sRanges[1]))
                        bRanges[i] = true;
                    if (dRanges[i][0] > dRanges[i][1])
                    {
                        bMirror[i] = true;
                        double dTemp = dRanges[i][1];
                        dRanges[i][1] = dRanges[i][0];
                        dRanges[i][0] = dTemp;
                    }
                    i++;
                    sRanges[0] = sRanges[2];
                    nRanges = i;
                }
            }
            if (sRanges[0].find(':') != string::npos)
            {
                if (i == 3)
                {
                    sRanges[0] = "(" + sRanges[0] + ")";
                    parser_SplitArgs(sRanges[0], sRanges[1], ':', _option, false);
                    if (parser_ExprNotEmpty(sRanges[0]))
                    {
                        _parser.SetExpr(sRanges[0]);
                        dColorRange[0] = (double)_parser.Eval();
                    }
                    if (parser_ExprNotEmpty(sRanges[1]))
                    {
                        _parser.SetExpr(sRanges[1]);
                        dColorRange[1] = (double)_parser.Eval();
                    }
                    if (isnan(dColorRange[0]) || isnan(dColorRange[1]) || isinf(dColorRange[0]) || isinf(dColorRange[1]))
                    {
                        dColorRange[0] = NAN;
                        dColorRange[1] = NAN;
                    }
                }
                else if (i == 4)
                    return;
                else
                {
                    sRanges[0] = "(" + sRanges[0] + ")";
                    parser_SplitArgs(sRanges[0], sRanges[1], ':', _option, false);
                    if (parser_ExprNotEmpty(sRanges[0]))
                    {
                        _parser.SetExpr(sRanges[0]);
                        dRanges[i][0] = (double)_parser.Eval();
                    }
                    if (parser_ExprNotEmpty(sRanges[1]))
                    {
                        _parser.SetExpr(sRanges[1]);
                        dRanges[i][1] = (double)_parser.Eval();
                    }
                    if (parser_ExprNotEmpty(sRanges[0]) || parser_ExprNotEmpty(sRanges[1]))
                        bRanges[i] = true;
                    if (dRanges[i][0] > dRanges[i][1])
                    {
                        bMirror[i] = true;
                        double dTemp = dRanges[i][1];
                        dRanges[i][1] = dRanges[i][0];
                        dRanges[i][0] = dTemp;
                    }
                    nRanges = i+1;
                }
            }
            for (unsigned int i = 0; i < 3; i++)
            {
                if (isinf(dRanges[i][0]) || isnan(dRanges[i][0]))
                    dRanges[i][0] = -10.0;
                if (isinf(dRanges[i][1]) || isnan(dRanges[i][1]))
                    dRanges[i][1] = 10.0;
            }
            for (int n = nRanges-1; n >= 0; n--)
            {
                if (bRanges[n])
                    break;
                if (!bRanges[n])
                    nRanges--;
            }
        }
    }
    else if (!nType || nType == 1)
        nRanges = 0;
    return;
}

// --> Alle Einstellungen zuruecksetzen <--
void PlotData::reset()
{
    if (dPlotData)
    {
        for (int i = 0; i < nLines; i++)
        {
            for (int j = 0; j < nRows; j++)
            {
                delete[] dPlotData[i][j];
            }
            delete[] dPlotData[i];
        }
        delete[] dPlotData;
    }
    dPlotData = 0;

    for (int i = 0; i < 3; i++)
    {
        bRanges[i] = false;
        bMirror[i] = false;
        dRanges[i][0] = -10.0;
        dRanges[i][1] = 10.0;
        dOrigin[i] = 0.0;
        sAxisLabels[i] = "";
        sAxisBind.clear();
        _lHlines[i].sDesc = "";
        _lHlines[i].sStyle = "k;2";
        _lHlines[i].dPos = 0.0;
        _lVLines[i].sDesc = "";
        _lVLines[i].sStyle = "k;2";
        _lVLines[i].dPos = 0.0;
        nSlices[i] = 1;
    }

    for (int i = 0; i < 4; i++)
    {
        bLogscale[i] = false;
        sTickTemplate[i] = "";
        sCustomTicks[i] = "";
        dAxisScale[i] = 1.0;
    }
    for (int i = 0; i < 2; i++)
    {
        _AddAxes[i].dMin = NAN;
        _AddAxes[i].dMax = NAN;
        _AddAxes[i].sLabel = "";
        _AddAxes[i].sStyle = "k";
    }
    dAspect = 4.0/3.0;
    dtParam[0] = 0.0;
    dtParam[1] = 1.0;
    dRotateAngles[0] = 60;
    dRotateAngles[1] = 115;
    dColorRange[0] = NAN;
    dColorRange[1] = NAN;
    nLines = 100;
    nRows = 1;
    nLayers = 1;
    nRequestedLayers = 1;
    nSamples = 100;
    dMin = NAN;
    dMax = NAN;
    dMaximum = 1.0;
    nGrid = 0;
    nLighting = 0;
    bAxis = true;
    bAlpha = false;
    bBox = false;
    bContLabels = false;
    bContProj = false;
    bContFilled = false;
    bxError = false;
    byError = false;
    bConnectPoints = false;
    bDrawPoints = false;
    bOpenImage = true;
    bInterpolate = false;
    nHighResLevel = 0;
    bAllHighRes = false;
    bSilentMode = false;
    bAnimate = false;
    bCutBox = false;
    bFlow = false;
    bPipe = false;
    bFixedLength = false;
    bColorbar = true;
    bOrthoProject = false;
    bArea = false;
    dBars = 0.0;
    dHBars = 0.0;
    bStepPlot = false;
    bBoxPlot = false;
    bColorMask = false;
    bAlphaMask = false;
    bSchematic = false;
    bCloudPlot = false;
    bRegion = false;
    bCrust = false;
    dPerspective = 0.0;
    sColorScheme = "UNC{e4}y";
    sColorSchemeMedium = "{U4}{N4}{C4}{e3}{y4}";
    sColorSchemeLight = "{U8}{N8}{C8}{e7}{y8}";
    sBackgroundColorScheme = "<<REALISTIC>>";
    sBackground = "";
    sColors = "rbGqmPuRBgQMpU";
    sGreys = "kHhWkHhWkHhWkH";
    sContColors = "kUHYPCQhuWypcq";
    sContGreys = "kwkwkwkwkwkwkw";
    sPointStyles = " + x o s d#+#x x o s d#+#x +";
    sLineStyles = "-------;;;;;;;";
    sLineSizes = "00000000000000";
    sLineStylesGrey = "-|=;i:j|=;i:j-";
    sGridStyle = "=h0-h0";
    nAnimateSamples = 50;
    nRanges = 0;
    nMarks = 0;
    nTextsize = 5;
    sFileName = "";
    sPlotTitle = "";
    nCoords = CARTESIAN;
    nLegendPosition = 3;
    nLegendstyle = 0;
    if (sFontStyle != "pagella")
    {
        sFontStyle = "pagella";
        _fontData.LoadFont(sFontStyle.c_str(), (sTokens[0][1]+ "\\fonts").c_str());
    }
    return;
}

// --> Daten im Speicher loeschen. Speicher selbst bleibt bestehen <--
void PlotData::deleteData()
{
    if (dPlotData)
    {
        for (int i = 0; i < nLines; i++)
        {
            for (int j = 0; j < nRows; j++)
            {
                for (int k = 0; k < nLayers; k++)
                {
                    dPlotData[i][j][k] = NAN;
                }
            }
        }
    }
    for (int i = 0; i < 3; i++)
    {
        dRanges[i][0] = -10.0;
        dRanges[i][1] = 10.0;
        bRanges[i] = false;
        bMirror[i] = false;
    }
    for (int i = 0; i < 4; i++)
    {
        sCustomTicks[i] = "";
        dAxisScale[i] = 1.0;
    }
    nRanges = 0;
    sFileName = "";
    sPlotTitle = "";
    if (!bAllHighRes)
        nHighResLevel = 0;
    dMin = NAN;
    dMax = NAN;
    dMaximum = 1.0;
    nRequestedLayers = 1;
    sColors = "rbGqmPuRBgQMpU";
    sGreys = "kHhWkHhWkHhWkH";
    sContColors = "kUHYPCQhuWypcq";
    sContGreys = "kwkwkwkwkwkwkw";
    sPointStyles = " + x o s d#+#x x o s d#+#x +";
    sLineStyles = "-------;;;;;;;";
    sLineSizes = "00000000000000";
    sLineStylesGrey = "-|=;i:j|=;i:j-";
    nLegendstyle = 0;
    sAxisBind.clear();
    sFunctionAxisBind.clear();
    sBackground = "";
    dColorRange[0] = NAN;
    dColorRange[1] = NAN;
    for (int i = 0; i < 3; i++)
    {
        sAxisLabels[i] = "";
        _lHlines[i].sDesc = "";
        _lHlines[i].sStyle = "k;2";
        _lVLines[i].sDesc = "";
        _lVLines[i].sStyle = "k;2";
        _lHlines[i].dPos = 0.0;
        _lVLines[i].dPos = 0.0;
    }
    for (int i = 0; i < 2; i++)
    {
        _AddAxes[i].dMin = NAN;
        _AddAxes[i].dMax = NAN;
        _AddAxes[i].sLabel = "";
        _AddAxes[i].sStyle = "k";
    }
    return;
}

/* --> Plotparameter als String lesen: Gibt nur Parameter zurueck, die von Plot zu Plot
 *     uebernommen werden. (sFileName und sAxisLabels[] z.B nicht) <--
 */
string PlotData::getParams(const Settings& _option, bool asstr) const
{
    string sReturn = "";
    string sSepString = "; ";
    if (asstr)
    {
        sReturn = "\"";
        sSepString = "\", \"";
    }
    sReturn += "[";
    for (int i = 0; i < 3; i++)
    {
        sReturn += toString(dRanges[i][0], _option) + ":" + toString(dRanges[i][1], _option);
        if (i < 2)
            sReturn += ", ";
    }
    sReturn += "]" + sSepString;
    if (bAlpha)
        sReturn += "alpha" + sSepString;
    if (bAlphaMask)
        sReturn += "alphamask" +sSepString;
    if (bAnimate)
        sReturn += "animate [" + toString(nAnimateSamples) + " frames]" + sSepString;
    if (bArea)
        sReturn += "area" + sSepString;
    sReturn += "aspect=" + toString(dAspect, 4) + sSepString;
    if (bAxis)
        sReturn += "axis" + sSepString;
    if (sAxisBind.length())
    {
        sReturn += "axisbind=";
        if (asstr)
            sReturn += "\\\"" + sAxisBind + "\\\"";
        else
            sReturn += "\"" + sAxisBind + "\"";
        sReturn += sSepString;
    }
    sReturn += "axisscale=[";
    for (int i = 0; i < 4; i++)
    {
        sReturn += toString(dAxisScale[i], _option);
        if (i < 3)
            sReturn += ", ";
    }
    sReturn += "]" + sSepString;
    if (dBars)
        sReturn += "bars=" + toString(dBars, 4) + sSepString;
    if (sBackground.length())
    {
        if (asstr)
            sReturn += "background=\\\"" + sBackground + "\\\"" +  sSepString;
        else
            sReturn += "background=\"" + sBackground + "\"" + sSepString;
    }
    sReturn += "bgcolorscheme=";
    if (sBackgroundColorScheme == "BbcyrR")
        sReturn += "rainbow" + sSepString;
    else if (sBackgroundColorScheme == "kw")
        sReturn += "grey" + sSepString;
    else if (sBackgroundColorScheme == "kRryw")
        sReturn += "hot" + sSepString;
    else if (sBackgroundColorScheme == "kBncw")
        sReturn += "cold" + sSepString;
    else if (sBackgroundColorScheme == "kQqw")
        sReturn += "copper" + sSepString;
    else if (sBackgroundColorScheme == "UBbcgyqRH")
        sReturn += "map" + sSepString;
    else if (sBackgroundColorScheme == "kMqyw")
        sReturn += "moy" + sSepString;
    else if (sBackgroundColorScheme == "BCyw")
        sReturn += "coast" + sSepString;
    else if (sBackgroundColorScheme == "UNC{e4}y")
        sReturn += "viridis" + sSepString;
    else if (sBackgroundColorScheme == "B{u4}p{q6}{y7}")
        sReturn += "plasma" + sSepString;
    else if (sBackgroundColorScheme == "<<REALISTIC>>")
        sReturn += "real" + sSepString;
    else
    {
        if (asstr)
            sReturn += "\\\"" + sBackgroundColorScheme + "\\\"" + sSepString;
        else
            sReturn += "\"" + sBackgroundColorScheme + "\"" + sSepString;
    }
    if (bBox)
        sReturn += "box" + sSepString;
    if (bBoxPlot)
        sReturn += "boxplot" + sSepString;
    if (bCloudPlot)
        sReturn += "cloudplot" + sSepString;
    if (bColorbar)
        sReturn += "colorbar" + sSepString;
    if (bColorMask)
        sReturn += "colormask" + sSepString;
    if (!isnan(dColorRange[0]) && !isnan(dColorRange[1]))
        sReturn += "colorrange="+toString(dColorRange[0], _option) + ":" + toString(dColorRange[1],_option) + sSepString;
    sReturn += "colorscheme=";
    if (sColorScheme == "BbcyrR")
        sReturn += "rainbow" + sSepString;
    else if (sColorScheme == "kw")
        sReturn += "grey" + sSepString;
    else if (sColorScheme == "kRryw")
        sReturn += "hot" + sSepString;
    else if (sColorScheme == "kBncw")
        sReturn += "cold" + sSepString;
    else if (sColorScheme == "kQqw")
        sReturn += "copper" + sSepString;
    else if (sColorScheme == "UBbcgyqRH")
        sReturn += "map" + sSepString;
    else if (sColorScheme == "kMqyw")
        sReturn += "moy" + sSepString;
    else if (sColorScheme == "BCyw")
        sReturn += "coast" + sSepString;
    else if (sColorScheme == "UNC{e4}y")
        sReturn += "viridis" + sSepString;
    else if (sColorScheme == "B{u4}p{q6}{y7}")
        sReturn += "plasma" + sSepString;
    else
    {
        if (asstr)
            sReturn += "\\\"" + sColorScheme + "\\\"" + sSepString;
        else
            sReturn += "\"" + sColorScheme + "\"" + sSepString;
    }
    if (bConnectPoints)
        sReturn += "connect" + sSepString;
    if (bCrust)
        sReturn += "crust" + sSepString;
    if (nCoords == 1)
        sReturn += "polar coords" + sSepString;
    if (nCoords == 2)
        sReturn += "spherical coords" + sSepString;
    if (bCutBox)
        sReturn += "cutbox" + sSepString;
    if (bxError && byError)
        sReturn += "errorbars" + sSepString;
    else if (bxError)
        sReturn += "xerrorbars" + sSepString;
    else if (byError)
        sReturn += "yerrorbars" + sSepString;
    if (bFixedLength)
        sReturn += "fixed length" + sSepString;
    if (bFlow)
        sReturn += "flow" + sSepString;
    sReturn += "font="+sFontStyle+sSepString;
    if (nGrid == 1)
        sReturn += "grid=coarse" + sSepString;
    else if (nGrid == 2)
        sReturn += "grid=fine" + sSepString;
    sReturn += "gridstyle=";
    if (asstr)
        sReturn += "\\\"" + sGridStyle + "\\\"";
    else
        sReturn += "\"" + sGridStyle + "\"";
    sReturn += sSepString;
    if (dHBars)
        sReturn += "hbars=" + toString(dHBars, 4) + sSepString;
    if (bInterpolate)
        sReturn += "interpolate" + sSepString;
    if (bContLabels)
        sReturn += "lcont" + sSepString;
    sReturn += "legend=";
    if (nLegendPosition == 0)
        sReturn += "bottomleft";
    else if (nLegendPosition == 1)
        sReturn += "bottomright";
    else if (nLegendPosition == 2)
        sReturn += "topleft";
    else
        sReturn += "topright";
    sReturn += sSepString;
    if (nLegendstyle == 1)
        sReturn += "legendstyle=onlycolors" + sSepString;
    if (nLegendstyle == 2)
        sReturn += "legendstyle=onlystyles" + sSepString;
    if (nLighting == 1)
        sReturn += "lighting" + sSepString;
    if (nLighting == 2)
        sReturn += "lighting=smooth" + sSepString;
    if (asstr)
        sReturn += "linesizes=\\\"" + sLineSizes + "\\\"" + sSepString;
    else
        sReturn += "linesizes=\"" + sLineSizes + "\"" + sSepString;
    if (asstr)
        sReturn += "linestyles=\\\"" + sLineStyles + "\\\"" + sSepString;
    else
        sReturn += "linestyles=\"" + sLineStyles + "\"" + sSepString;
    if (bLogscale[0] && bLogscale[1] && bLogscale[2] && bLogscale[3])
        sReturn += "logscale" + sSepString;
    else
    {
        if (bLogscale[0])
            sReturn += "xlog" + sSepString;
        if (bLogscale[1])
            sReturn += "ylog" + sSepString;
        if (bLogscale[2])
            sReturn += "zlog" + sSepString;
        if (bLogscale[3])
            sReturn += "clog" + sSepString;
    }
    if (nMarks)
        sReturn += "marks=" + toString(nMarks) + sSepString;
    if (bOpenImage)
        sReturn += "open" + sSepString;
    sReturn += "origin=";
    if (isnan(dOrigin[0]) && isnan(dOrigin[1]) && isnan(dOrigin[2]))
        sReturn += "sliding" + sSepString;
    else if (dOrigin[0] == 0.0 && dOrigin[1] == 0.0 && dOrigin[2] == 0.0)
        sReturn += "std" + sSepString;
    else
        sReturn += "[" + toString(dOrigin[0], _option) + ", " + toString(dOrigin[1], _option) + ", " + toString(dOrigin[2], _option) + "]" + sSepString;
    sReturn += "slices=[" +toString((int)nSlices[0]) + ", " + toString((int)nSlices[1]) + ", " + toString((int)nSlices[2]) + "]" + sSepString;
    if (bStepPlot)
        sReturn += "steps" + sSepString;
    if (bOrthoProject)
        sReturn += "orthogonal projection" + sSepString;
    if (bContProj)
        sReturn += "pcont" + sSepString;
    if (dPerspective)
        sReturn += "perspective=" + toString(dPerspective, _option) + sSepString;
    if (asstr)
        sReturn += "plotcolors=\\\"" + sColors + "\\\"" + sSepString;
    else
        sReturn += "plotcolors=\"" + sColors + "\"" + sSepString;
    if (asstr)
        sReturn += "pointstyles=\\\"" + sPointStyles + "\\\"" + sSepString;
    else
        sReturn += "pointstyles=\"" + sPointStyles + "\"" + sSepString;
    if (bPipe)
        sReturn += "pipe" + sSepString;
    if (bDrawPoints)
        sReturn += "points" + sSepString;
    if (bRegion)
        sReturn += "region" + sSepString;
    if (nHighResLevel)
    {
        if (nHighResLevel == 1)
            sReturn += "medium";
        else
            sReturn += "high";
        sReturn += " resolution" + sSepString;
    }
    sReturn += "rotate=" + toString(dRotateAngles[0], _option) + "," + toString(dRotateAngles[1], _option) + sSepString;
    sReturn += "samples=" + toString(nSamples) + sSepString;
    if (bSchematic)
        sReturn += "schematic" + sSepString;
    if (bSilentMode)
        sReturn += "silent mode" + sSepString;
    sReturn += "t=" + toString(dtParam[0], _option) + ":" + toString(dtParam[1], _option) + sSepString;
    sReturn += "textsize=" + toString(nTextsize) + sSepString;
    sReturn += "tickstemplate=[";
    for (int i = 0; i < 4; i++)
    {
        if (i == 3)
        {
            if (asstr)
                sReturn += "c=\\\"" + sTickTemplate[i] + "\\\"";
            else
                sReturn += "c=\"" + sTickTemplate[i] + "\"";
        }
        else
        {
            sReturn += char('x'+i);
            if (asstr)
                sReturn += "=\\\"" + sTickTemplate[i] + "\\\", ";
            else
                sReturn += "=\"" + sTickTemplate[i] + "\", ";
        }
    }
    sReturn += "]" + sSepString;
    sReturn += "tickslabels=[";
    for (int i = 0; i < 4; i++)
    {
        if (i == 3)
        {
            if (asstr)
                sReturn += "c=\\\"" + sCustomTicks[i] + "\\\"";
            else
                sReturn += "c=\"" + sCustomTicks[i] + "\"";
        }
        else
        {
            sReturn += char('x'+i);
            if (asstr)
                sReturn += "=\\\"" + sCustomTicks[i] + "\\\", ";
            else
                sReturn += "=\"" + sCustomTicks[i] + "\", ";
        }
    }
    sReturn += "]" + sSepString;
    if (asstr)
        sReturn + "\"";
    return sReturn;
}

/* --> Groesse des Speichers einstellen: falls die neuen Groessen (ganz oder teilweise) kleiner als
 *     die bisherigen sind, werden automatisch die groesseren gewaehlt <--
 */
void PlotData::setDim(int _i, int _j, int _k)
{
    nRequestedLayers = _k;
    if (_i == nLines && _j == nRows && _k == nLayers && dPlotData)
    {
        return;
    }
    int nNLines = _i;
    int nNRows = _j;
    int nNLayers = _k;
    double*** dTemp;
    if (nNLines < nLines)
        nNLines = nLines;
    if (nNRows < nRows)
        nNRows = nRows;
    if (nNLayers < nLayers)
        nNLayers = nLayers;
    if (!dPlotData)
    {
        dPlotData = new double**[nNLines];
        for (int i = 0; i < nNLines; i++)
        {
            dPlotData[i] = new double*[nNRows];
            for (int j = 0; j < nNRows; j++)
            {
                dPlotData[i][j] = new double[nNLayers];
                for (int k = 0; k < nNLayers; k++)
                {
                    dPlotData[i][j][k] = NAN;
                }
            }
        }
        nLines = nNLines;
        nRows = nNRows;
        nLayers = nNLayers;
    }
    else
    {
        dTemp = new double**[nLines];
        for (int i = 0; i < nLines; i++)
        {
            dTemp[i] = new double*[nRows];
            for (int j = 0; j < nRows; j++)
            {
                dTemp[i][j] = new double[nLayers];
                for (int k = 0; k < nLayers; k++)
                {
                    dTemp[i][j][k] = dPlotData[i][j][k];
                }
            }
        }

        for (int i = 0; i < nLines; i++)
        {
            for (int j = 0; j < nRows; j++)
            {
                delete[] dPlotData[i][j];
            }
            delete[] dPlotData[i];
        }
        delete[] dPlotData;
        dPlotData = 0;

        dPlotData = new double**[nNLines];
        for (int i = 0; i < nNLines; i++)
        {
            dPlotData[i] = new double*[nNRows];
            for (int j = 0; j < nNRows; j++)
            {
                dPlotData[i][j] = new double[nNLayers];
                for (int k = 0; k < nNLayers; k++)
                {
                    if (i < nLines && j < nRows && k < nLayers)
                        dPlotData[i][j][k] = dTemp[i][j][k];
                    else
                        dPlotData[i][j][k] = NAN;
                }
            }
        }
        for (int i = 0; i < nLines; i++)
        {
            for (int j = 0; j < nRows; j++)
            {
                delete[] dTemp[i][j];
            }
            delete[] dTemp[i];
        }
        delete[] dTemp;
        dTemp = 0;

        nLines = nNLines;
        nRows = nNRows;
        nLayers = nNLayers;
    }

    return;
}

// --> Spalten lesen <--
int PlotData::getRows() const
{
    return nRows;
}

// --> Zeilen lesen <--
int PlotData::getLines() const
{
    return nLines;
}

// --> Ebenen lesen <--
int PlotData::getLayers(bool bFull) const
{
    return bFull ? nLayers : nRequestedLayers;
}

// --> Minimum aller Daten im Speicher lesen <--
double PlotData::getMin(int nCol)
{
    double _dMin = NAN;
    if (dPlotData)
    {
        if (nCol == ALLRANGES)
        {
            return dMin;
        }
        else if (nCol == ONLYLEFT) // l
        {
            for (int j = 0; j < nRows; j++)
            {
                if (getFunctionAxisbind(j)[0] != 'l')
                    continue;
                for (int i = 0; i < nLines; i++)
                {
                    if (dPlotData[i][j][0] < _dMin || isnan(_dMin))
                        _dMin = dPlotData[i][j][0];
                }
            }
        }
        else if (nCol == ONLYRIGHT) // r
        {
            for (int j = 0; j < nRows; j++)
            {
                if (getFunctionAxisbind(j)[0] != 'r')
                    continue;
                for (int i = 0; i < nLines; i++)
                {
                    if (dPlotData[i][j][0] < _dMin || isnan(_dMin))
                        _dMin = dPlotData[i][j][0];
                }
            }
        }
        else if (nCol >= nRows)
            return NAN;
        else
        {
            for (int i = 0; i < nLines; i++)
            {
                for (int k = 0; k < nLayers; k++)
                {
                    if (dPlotData[i][nCol][k] < _dMin || isnan(_dMin))
                        _dMin = dPlotData[i][nCol][k];
                }
            }
        }
    }
    return _dMin;
}

// --> Maximum aller Daten im Speicher lesen <--
double PlotData::getMax(int nCol)
{
    double _dMax = NAN;
    if (dPlotData)
    {
        if (nCol == ALLRANGES)
        {
            return dMax;
        }
        else if (nCol == ONLYLEFT) // l
        {
            for (int j = 0; j < nRows; j++)
            {
                if (getFunctionAxisbind(j)[0] != 'l')
                    continue;
                for (int i = 0; i < nLines; i++)
                {
                    if (dPlotData[i][j][0] > _dMax || isnan(_dMax))
                        _dMax = dPlotData[i][j][0];
                }
            }
        }
        else if (nCol == ONLYRIGHT) // r
        {
            for (int j = 0; j < nRows; j++)
            {
                if (getFunctionAxisbind(j)[0] != 'r')
                    continue;
                for (int i = 0; i < nLines; i++)
                {
                    if (dPlotData[i][j][0] > _dMax || isnan(_dMax))
                        _dMax = dPlotData[i][j][0];
                }
            }
        }
        else if (nCol >= nRows)
            return NAN;
        else
        {
            for (int i = 0; i < nLines; i++)
            {
                for (int k = 0; k < nLayers; k++)
                {
                    if (dPlotData[i][nCol][k] > _dMax || isnan(_dMax))
                        _dMax = dPlotData[i][nCol][k];
                }
            }
        }
    }
    return _dMax;
}

vector<double> PlotData::getWeightedRanges(int nCol, double dLowerPercentage, double dUpperPercentage)
{
    vector<double> vRanges(2, NAN);
    size_t nLength = 0;

    if (dPlotData)
    {
        if (nCol == ALLRANGES)
        {
            double* dData = new double[nRows*nLayers*nLines];

            for (int i = 0; i < nLines; i++)
            {
                for (int j = 0; j < nRows; j++)
                {
                    for (int k = 0; k < nLayers; k++)
                        dData[i+j*nLines+k*nLines*nRows] = dPlotData[i][j][k];
                }
            }

            nLength = qSortDouble(dData, nRows*nLayers*nLines);

            rangeByPercentage(dData, nLength, dLowerPercentage, dUpperPercentage, vRanges);

            delete[] dData;
        }
        else if (nCol == ONLYLEFT) // l
        {
            double* dData = new double[nRows*nLines];
            int row = 0;

            for (int j = 0; j < nRows; j++)
            {
                if (getFunctionAxisbind(j)[0] != 'l')
                    continue;
                for (int i = 0; i < nLines; i++)
                {
                    dData[i + row*nLines] = dPlotData[i][j][0];
                }
                row++;
            }

            nLength = qSortDouble(dData, nLines*row);

            rangeByPercentage(dData, nLength, dLowerPercentage, dUpperPercentage, vRanges);

            delete[] dData;
        }
        else if (nCol == ONLYRIGHT) // r
        {
            double* dData = new double[nRows*nLines];
            int row = 0;

            for (int j = 0; j < nRows; j++)
            {
                if (getFunctionAxisbind(j)[0] != 'r')
                    continue;
                for (int i = 0; i < nLines; i++)
                {
                    dData[i+row*nLines] = dPlotData[i][j][0];
                }
                row++;
            }

            nLength = qSortDouble(dData, nLines*row);

            rangeByPercentage(dData, nLength, dLowerPercentage, dUpperPercentage, vRanges);

            delete[] dData;
        }
        else if (nCol >= nRows)
            return vRanges;
        else
        {
            double* dData = new double[nLines*nLayers];

            for (int i = 0; i < nLines; i++)
            {
                for (int k = 0; k < nLayers; k++)
                {
                    dData[i+k*nLines] = dPlotData[i][nCol][k];
                }
            }

            nLength = qSortDouble(dData, nLines*nLayers);

            rangeByPercentage(dData, nLength, dLowerPercentage, dUpperPercentage, vRanges);

            delete[] dData;
        }
    }
    return vRanges;
}

// --> Ein spezifisches Plot-Intervall einstellen <--
void PlotData::setRanges(int _j, double x_0, double x_1)
{
    dRanges[_j][0] = x_0;
    dRanges[_j][1] = x_1;
    return;
}

// --> Grenzen eines Intervalls lesen <--
double PlotData::getRanges(int _j, int _i)
{
    return dRanges[_j][_i];
}

// --> Datenpunkte einstellen <--
void PlotData::setSamples(int _nSamples)
{
    nSamples = _nSamples;
}

// --> Ausgabe-Dateinamen setzen <--
void PlotData::setFileName(string _sFileName)
{
    if (_sFileName.length())
    {
        string sExt = _sFileName.substr(_sFileName.rfind('.'));
        if (sExt[sExt.length()-1] == '"')
            sExt = sExt.substr(0,sExt.length()-1);
        if (_sFileName.find('\\') == string::npos && _sFileName.find('/') == string::npos)
        {
            if (sPath[0] == '"' && sPath[sPath.length()-1] == '"')
                sFileName = sPath.substr(0,sPath.length()-1)+"/"+_sFileName+"\"";
            else
                sFileName = sPath + "/" + _sFileName;
        }
        else
            sFileName = _sFileName;

        sFileName = FileSystem::ValidFileName(sFileName, sExt);
    }
    else
        sFileName = "";
    return;
}

// --> Vektorlaengen auf das Intervall [0,1] normieren <--
void PlotData::normalize(int nDim, int t_animate)
{
    double dCurrentMax = 0.0;
    if (dPlotData)
    {
        if (!t_animate)
        {
            dMaximum = 0.0;
            double dNorm = 0.0;
            dMax = NAN;
            dMin = NAN;
            for (int i = 0; i < nLines; i++)
            {
                for (int j = 0; j < nRows; j++)
                {
                    if (nDim == 2)
                    {
                        dNorm = 0.0;
                        for (int k = 0; k < nDim; k++)
                        {
                            if (isnan(dPlotData[i][j][k]))
                            {
                                dNorm = 0.0;
                                break;
                            }
                            dNorm += dPlotData[i][j][k]*dPlotData[i][j][k];
                        }
                        if (dMaximum < dNorm)
                            dMaximum = dNorm;
                    }
                    else
                    {
                        dNorm = 0.0;
                        for (int k = 0; k < nLayers; k++)
                        {
                            if (isnan((dPlotData[i][j][k])))
                            {
                                dNorm = 0.0;
                                if (!k)
                                    k = 2;
                                else
                                    k += 3-(k % 3)-1;
                                continue;
                            }
                            dNorm += dPlotData[i][j][k]*dPlotData[i][j][k];
                            if (k && !((k+1) % 3))
                            {
                                //cerr << dNorm << endl;
                                if (dMaximum < dNorm)
                                    dMaximum = dNorm;
                                dNorm = 0.0;
                            }
                        }
                    }
                }
            }
            dMaximum = sqrt(dMaximum);
            if (!dMaximum)
                dMaximum = 1.0;
        }
        else
        {
            double dNorm = 0.0;
            dMax = NAN;
            dMin = NAN;
            for (int i = 0; i < nLines; i++)
            {
                for (int j = 0; j < nRows; j++)
                {
                    if (nDim == 2)
                    {
                        dNorm = 0.0;
                        for (int k = 0; k < nDim; k++)
                        {
                            if (isnan(dPlotData[i][j][k]))
                            {
                                dNorm = 0.0;
                                break;
                            }
                            dNorm += dPlotData[i][j][k]*dPlotData[i][j][k];
                        }
                        if (dCurrentMax < dNorm)
                            dCurrentMax = dNorm;
                    }
                    else
                    {
                        dNorm = 0.0;
                        for (int k = 0; k < nLayers; k++)
                        {
                            if (isnan((dPlotData[i][j][k])))
                            {
                                dNorm = 0.0;
                                if (!k)
                                    k = 2;
                                else
                                    k += 3-(k % 3)-1;
                                continue;
                            }
                            dNorm += dPlotData[i][j][k]*dPlotData[i][j][k];
                            if (k && !((k+1) % 3))
                            {
                                //cerr << dNorm << endl;
                                if (dCurrentMax < dNorm)
                                    dCurrentMax = dNorm;
                                dNorm = 0.0;
                            }
                        }
                    }
                }
            }
            dCurrentMax = sqrt(dCurrentMax);
            if (!dCurrentMax)
                dCurrentMax = 1.0;
        }
        //cerr << dMaximum << endl;
        //cerr << dCurrentMax << endl;
        //cerr << 1/(dCurrentMax*dCurrentMax/dMaximum) << endl;
        for (int i = 0; i < nLines; i++)
        {
            for (int j = 0; j < nRows; j++)
            {
                for (int k = 0; k < nLayers; k++)
                {
                    if (!t_animate)
                        dPlotData[i][j][k] /= dMaximum;
                    else if (dCurrentMax >= 1.0)
                        dPlotData[i][j][k] /= (dCurrentMax*dCurrentMax)/dMaximum;
                    else
                        dPlotData[i][j][k] /= (dCurrentMax*dCurrentMax)/dMaximum;
                    if (isnan(dMax) || dMax < dPlotData[i][j][k])
                        dMax = dPlotData[i][j][k];
                    if (isnan(dMin) || dMin > dPlotData[i][j][k])
                        dMin = dPlotData[i][j][k];
                }
            }
        }
    }
    return;
}

// --> \n und \t passend ersetzen <--
void PlotData::replaceControlChars(string& sString)
{
    if (sString.find('\t') == string::npos && sString.find('\n') == string::npos)
        return;

    for (unsigned int i = 0; i < sString.length(); i++)
    {
        if (sString[i] == '\t' && sString.substr(i+1,2) == "au")
            sString.replace(i,1,"\\t");
        if (sString[i] == '\n' && sString[i+1] == 'u')
            sString.replace(i,1,"\\n");
    }
    return;
}

void PlotData::rangeByPercentage(double* dData, size_t nLength, double dLowerPercentage, double dUpperPercentage, vector<double>& vRanges)
{
    if (dLowerPercentage == 1.0)
    {
        vRanges[0] = dData[0];
        size_t pos = dUpperPercentage*nLength;
        vRanges[1] = dData[pos];
    }
    else if (dUpperPercentage == 1.0)
    {
        size_t pos = (1.0-dLowerPercentage)*nLength;
        vRanges[1] = dData[nLength-1];
        vRanges[0] = dData[pos];
    }
    else
    {
        size_t lowerpos = (1.0-dLowerPercentage)/2.0*(nLength);
        size_t upperpos = (dUpperPercentage/2.0+0.5)*(nLength);
        vRanges[0] = dData[lowerpos];
        vRanges[1] = dData[upperpos];
    }
}

// --> Lesen der einzelnen Achsenbeschriftungen <--
string PlotData::getxLabel() const
{
    if (sAxisLabels[0].length())
        return replaceToTeX(sAxisLabels[0]);
    else
    {
        switch (nCoords)
        {
            case CARTESIAN:
                return "@{\\i x}";
            case POLAR_PZ:
            case SPHERICAL_PT:
                return "@{\\varphi  [\\pi]}";
            case POLAR_RP:
            case POLAR_RZ:
                return "@{\\rho}";
            case SPHERICAL_RP:
            case SPHERICAL_RT:
                return "@{\\i r}";
        }
    }
    return "";
}

string PlotData::getyLabel() const
{
    if (sAxisLabels[1].length())
        return replaceToTeX(sAxisLabels[1]);
    else
    {
        switch (nCoords)
        {
            case CARTESIAN:
                return "@{\\i y}";
            case POLAR_PZ:
            case POLAR_RZ:
                return "@{\\i z}";
            case POLAR_RP:
            case SPHERICAL_RP:
                return "@{\\varphi  [\\pi]}";
            case SPHERICAL_PT:
            case SPHERICAL_RT:
                return "@{\\vartheta  [\\pi]}";
        }
    }
    return "";
}

string PlotData::getzLabel() const
{
    if (sAxisLabels[2].length())
        return replaceToTeX(sAxisLabels[2]);
    else
    {
        switch (nCoords)
        {
            case CARTESIAN:
            case POLAR_RP:
                return "@{\\i z}";
            case POLAR_PZ:
                return "@{\\rho}";
            case SPHERICAL_PT:
                return "@{\\i r}";
            case SPHERICAL_RP:
                return "@{\\vartheta  [\\pi]}";
            case POLAR_RZ:
            case SPHERICAL_RT:
                return "@{\\varphi  [\\pi]}";
        }
    }
    return "";
}



















