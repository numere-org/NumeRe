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
#include "../../kernel.hpp"
#define STYLES_COUNT 20

extern mglGraph _fontData;

// Function prototype
bool isNotEmptyExpression(const string& sExpr);

static value_type* evaluateNumerical(int& nResults, string sExpression)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Parser& _parser = NumeReKernel::getInstance()->getParser();

    if (_data.containsTablesOrClusters(sExpression))
        getDataElements(sExpression, _parser, _data, NumeReKernel::getInstance()->getSettings());

    _parser.SetExpr(sExpression);

    return _parser.Eval(nResults);
}

static string evaluateString(string sExpression)
{
    string sDummy;
    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sExpression))
        NumeReKernel::getInstance()->getStringParser().evalAndFormat(sExpression, sDummy, true);

    return sExpression;
}

static bool checkColorChars(const std::string& sColorSet)
{
    std::string sColorChars = "#| wWhHkRrQqYyEeGgLlCcNnBbUuMmPp123456789{}";
    for (unsigned int i = 0; i < sColorSet.length(); i++)
    {
        if (sColorSet[i] == '{' && i+3 >= sColorSet.length())
            return false;
        else if (sColorSet[i] == '{'
            && (sColorSet[i+3] != '}'
                || sColorChars.substr(3,sColorChars.length()-14).find(sColorSet[i+1]) == std::string::npos
                || sColorSet[i+2] > '9'
                || sColorSet[i+2] < '1'))
            return false;
        if (sColorChars.find(sColorSet[i]) == std::string::npos)
            return false;
    }
    return true;
}

static bool checkLineChars(const std::string& sLineSet)
{
    std::string sLineChars = " -:;ij|=";
    for (unsigned int i = 0; i < sLineSet.length(); i++)
    {
        if (sLineChars.find(sLineSet[i]) == std::string::npos)
            return false;
    }
    return true;
}

static bool checkPointChars(const std::string& sPointSet)
{
    std::string sPointChars = " .*+x#sdo^v<>";
    for (unsigned int i = 0; i < sPointSet.length(); i++)
    {
        if (sPointChars.find(sPointSet[i]) == std::string::npos)
            return false;
    }
    return true;
}




// --> Konstruktor <--
PlotData::PlotData() : FileSystem()
{
    PlotData::reset();
}

// --> Destruktor <--
PlotData::~PlotData()
{
}


/* --> Parameter setzen: Verwendet die bool matchParams(const string&, const string&, char)-Funktion,
 *     um die einzelnen Befehle zu identifizieren. Unbekannte Befehle werden automatisch ignoriert. 7
 *     Dies ist dann automatisch Fehlertoleranter <--
 */
void PlotData::setParams(const string& __sCmd, int nType)
{
    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();

    string sCmd = toLowerCase(__sCmd);
    if (findParameter(sCmd, "reset") && (nType == ALL || nType & SUPERGLOBAL))
        reset();
    if (findParameter(sCmd, "grid") && (nType == ALL || nType & GLOBAL))
        intSettings[INT_GRID] = 1;
    if (findParameter(sCmd, "grid", '=') && (nType == ALL || nType & GLOBAL))
    {
        unsigned int nPos = findParameter(sCmd, "grid", '=')+4;
        if (getArgAtPos(sCmd, nPos) == "fine")
            intSettings[INT_GRID] = 2;
        else if (getArgAtPos(sCmd, nPos) == "coarse")
            intSettings[INT_GRID] = 1;
        else
            intSettings[INT_GRID] = 1;
    }
    if (findParameter(sCmd, "nogrid") && (nType == ALL || nType & GLOBAL))
        intSettings[INT_GRID] = 0;
    if ((findParameter(sCmd, "alpha") || findParameter(sCmd, "transparency")) && (nType == ALL || nType & LOCAL))
    {
        logicalSettings[LOG_ALPHA] = true;

        if (findParameter(sCmd, "alpha", '='))
        {
            _parser.SetExpr(getArgAtPos(sCmd, findParameter(sCmd, "alpha", '=')+5));
            floatSettings[FLOAT_ALPHAVAL] = 1 - _parser.Eval().real();

            if (floatSettings[FLOAT_ALPHAVAL] < 0 || floatSettings[FLOAT_ALPHAVAL] > 1)
                floatSettings[FLOAT_ALPHAVAL] = 0.5;
        }

        if (findParameter(sCmd, "transparency", '='))
        {
            _parser.SetExpr(getArgAtPos(sCmd, findParameter(sCmd, "transparency", '=')+12));
            floatSettings[FLOAT_ALPHAVAL] = 1 - _parser.Eval().real();

            if (floatSettings[FLOAT_ALPHAVAL] < 0 || floatSettings[FLOAT_ALPHAVAL] > 1)
                floatSettings[FLOAT_ALPHAVAL] = 0.5;
        }
    }
    if ((findParameter(sCmd, "noalpha") || findParameter(sCmd, "notransparency")) && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_ALPHA] = false;
    if (findParameter(sCmd, "light") && (nType == ALL || nType & LOCAL))
        intSettings[INT_LIGHTING] = 1;
    if (findParameter(sCmd, "light", '=') && (nType == ALL || nType & LOCAL))
    {
        if (getArgAtPos(sCmd, findParameter(sCmd, "light", '=')+5) == "smooth")
            intSettings[INT_LIGHTING] = 2;
        else if (getArgAtPos(sCmd, findParameter(sCmd, "light", '=')+5) == "soft")
            intSettings[INT_LIGHTING] = 2;
        else
            intSettings[INT_LIGHTING] = 0;
    }
    if (findParameter(sCmd, "nolight") && (nType == ALL || nType & LOCAL))
        intSettings[INT_LIGHTING] = 0;
    if (findParameter(sCmd, "axis") && (nType == ALL || nType & GLOBAL))
        logicalSettings[LOG_AXIS] = true;
    if (findParameter(sCmd, "noaxis") && (nType == ALL || nType & GLOBAL))
        logicalSettings[LOG_AXIS] = false;
    if (findParameter(sCmd, "box") && (nType == ALL || nType & GLOBAL))
        logicalSettings[LOG_BOX] = true;
    if (findParameter(sCmd, "nobox") && (nType == ALL || nType & GLOBAL))
        logicalSettings[LOG_BOX] = false;
    if (findParameter(sCmd, "lcont") && (nType == ALL || nType & LOCAL))
    {
        logicalSettings[LOG_CONTLABELS] = true;

        if (findParameter(sCmd, "lcont", '='))
        {
            _parser.SetExpr(getArgAtPos(sCmd, findParameter(sCmd, "lcont", '=')));
            intSettings[INT_CONTLINES] = intCast(_parser.Eval());
        }
    }
    if (findParameter(sCmd, "nolcont") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_CONTLABELS] = false;
    if (findParameter(sCmd, "pcont") && (nType == ALL || nType & LOCAL))
    {
        logicalSettings[LOG_CONTPROJ] = true;

        if (findParameter(sCmd, "pcont", '='))
        {
            _parser.SetExpr(getArgAtPos(sCmd, findParameter(sCmd, "pcont", '=')));
            intSettings[INT_CONTLINES] = intCast(_parser.Eval());
        }
    }
    if (findParameter(sCmd, "nopcont") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_CONTPROJ] = false;
    if (findParameter(sCmd, "fcont") && (nType == ALL || nType & LOCAL))
    {
        logicalSettings[LOG_CONTFILLED] = true;

        if (findParameter(sCmd, "fcont", '='))
        {
            _parser.SetExpr(getArgAtPos(sCmd, findParameter(sCmd, "fcont", '=')));
            intSettings[INT_CONTLINES] = intCast(_parser.Eval());
        }
    }
    if (findParameter(sCmd, "nofcont") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_CONTFILLED] = false;
    if (findParameter(sCmd, "xerrorbars") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_XERROR] = true;
    if (findParameter(sCmd, "noxerrorbars") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_XERROR] = false;
    if (findParameter(sCmd, "yerrorbars") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_YERROR] = true;
    if (findParameter(sCmd, "noyerrorbars") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_YERROR] = false;
    if (findParameter(sCmd, "errorbars") && (nType == ALL || nType & LOCAL))
    {
        logicalSettings[LOG_XERROR] = true;
        logicalSettings[LOG_YERROR] = true;
    }
    if (findParameter(sCmd, "noerrorbars") && (nType == ALL || nType & LOCAL))
    {
        logicalSettings[LOG_XERROR] = false;
        logicalSettings[LOG_YERROR] = false;
    }
    if (findParameter(sCmd, "logscale") && (nType == ALL || nType & GLOBAL))
    {
        for (int i = 0; i < 4; i++)
        {
            bLogscale[i] = true;
        }
    }
    if (findParameter(sCmd, "nologscale") && (nType == ALL || nType & GLOBAL))
    {
        for (int i = 0; i < 4; i++)
        {
            bLogscale[i] = false;
        }
    }
    if (findParameter(sCmd, "xlog") && (nType == ALL || nType & GLOBAL))
        bLogscale[0] = true;
    if (findParameter(sCmd, "ylog") && (nType == ALL || nType & GLOBAL))
        bLogscale[1] = true;
    if (findParameter(sCmd, "zlog") && (nType == ALL || nType & GLOBAL))
        bLogscale[2] = true;
    if (findParameter(sCmd, "clog") && (nType == ALL || nType & GLOBAL))
        bLogscale[3] = true;
    if (findParameter(sCmd, "noxlog") && (nType == ALL || nType & GLOBAL))
        bLogscale[0] = false;
    if (findParameter(sCmd, "noylog") && (nType == ALL || nType & GLOBAL))
        bLogscale[1] = false;
    if (findParameter(sCmd, "nozlog") && (nType == ALL || nType & GLOBAL))
        bLogscale[2] = false;
    if (findParameter(sCmd, "noclog") && (nType == ALL || nType & GLOBAL))
        bLogscale[3] = false;
    if (findParameter(sCmd, "samples", '=') && (nType == ALL || nType & LOCAL))
    {
        int nPos = findParameter(sCmd, "samples", '=') + 7;
        _parser.SetExpr(getArgAtPos(__sCmd, nPos));
        intSettings[INT_SAMPLES] = intCast(_parser.Eval());

        if (isnan(_parser.Eval().real()) || isinf(_parser.Eval().real()))
            intSettings[INT_SAMPLES] = 100;
    }
    if (findParameter(sCmd, "t", '=') && (nType == ALL || nType & LOCAL))
    {
        int nPos = findParameter(sCmd, "t", '=')+1;
        string sTemp_1 = getArgAtPos(__sCmd, nPos);
        ranges[4].reset(sTemp_1);
    }
    if (findParameter(sCmd, "colorrange", '=') && (nType == ALL || nType & GLOBAL))
    {
        unsigned int nPos = findParameter(sCmd, "colorrange", '=') + 10;
        string sTemp_1 = getArgAtPos(__sCmd, nPos);
        ranges[CRANGE].reset(sTemp_1);

        if (ranges[CRANGE].front().real() > ranges[CRANGE].back().real())
        {
            bMirror[CRANGE] = true;
            ranges[CRANGE].reset(ranges[CRANGE].back(), ranges[CRANGE].front());
        }
    }
    if (findParameter(sCmd, "rotate", '=') && (nType == ALL || nType & GLOBAL))
    {
        int nPos = findParameter(sCmd, "rotate", '=')+6;
        string sTemp = getArgAtPos(__sCmd, nPos);
        if (sTemp.find(",") != string::npos && sTemp.length() > 1)
        {
            if (sTemp.find(',') && sTemp.find(',') != sTemp.length()-1)
            {
                _parser.SetExpr(sTemp);
                _parser.Eval();
                int nResults = _parser.GetNumResults();
                mu::value_type* dTemp = _parser.Eval(nResults);
                dRotateAngles[0] = dTemp[0].real();
                dRotateAngles[1] = dTemp[1].real();
            }
            else if (!sTemp.find(','))
            {
                _parser.SetExpr(sTemp.substr(1));
                dRotateAngles[1] = _parser.Eval().real();
            }
            else if (sTemp.find(',') == sTemp.length()-1)
            {
                _parser.SetExpr(sTemp.substr(0,sTemp.length()-1));
                dRotateAngles[0] = _parser.Eval().real();
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
                dRotateAngles[0] += ceil(-dRotateAngles[0]/180.0)*180.0;

            if (dRotateAngles[0] > 180)
                dRotateAngles[0] -= floor(dRotateAngles[0]/180.0)*180.0;

            if (dRotateAngles[1] < 0)
                dRotateAngles[1] += ceil(-dRotateAngles[1]/360.0)*360.0;

            if (dRotateAngles[1] > 360)
                dRotateAngles[1] -= floor(dRotateAngles[1]/360.0)*360.0;

        }
    }
    if (findParameter(sCmd, "origin", '=') && (nType == ALL || nType & GLOBAL))
    {
        int nPos = findParameter(sCmd, "origin", '=')+6;
        string sTemp = getArgAtPos(__sCmd, nPos);
        if (sTemp.find(',') != string::npos && sTemp.length() > 1)
        {
            _parser.SetExpr(sTemp);
            int nResults = 0;
            mu::value_type* dTemp = _parser.Eval(nResults);
            if (nResults)
            {
                for (int i = 0; i < 3; i++)
                {
                    if (i < nResults && !isnan(dTemp[i].real()) && !isinf(dTemp[i].real()))
                        dOrigin[i] = dTemp[i].real();
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
    if (findParameter(sCmd, "slices", '=') && (nType == ALL || nType & LOCAL))
    {
        int nPos = findParameter(sCmd, "slices", '=')+6;
        string sTemp = getArgAtPos(__sCmd, nPos);
        if (sTemp.find(',') != string::npos && sTemp.length() > 1)
        {
            _parser.SetExpr(sTemp);
            int nResults = 0;
            mu::value_type* dTemp = _parser.Eval(nResults);
            if (nResults)
            {
                for (int i = 0; i < 3; i++)
                {
                    if (i < nResults && !isnan(dTemp[i].real()) && !isinf(dTemp[i].real()) && dTemp[i].real() <= 5 && dTemp[i].real() >= 0)
                        nSlices[i] = (unsigned short)dTemp[i].real();
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
    if (findParameter(sCmd, "streamto", '=') && (nType == ALL || nType & SUPERGLOBAL))
    {
        int nPos = findParameter(sCmd, "streamto", '=')+8;
        string sTemp = getArgAtPos(__sCmd, nPos);
        if (sTemp.find(',') != string::npos && sTemp.length() > 1)
        {
            _parser.SetExpr(sTemp);
            int nResults = 0;
            mu::value_type* dTemp = _parser.Eval(nResults);

            if (nResults >= 2)
            {
                nTargetGUI[0] = intCast(dTemp[0]);
                nTargetGUI[1] = intCast(dTemp[1]);
            }
        }
    }
    if (findParameter(sCmd, "connect") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_CONNECTPOINTS] = true;
    if (findParameter(sCmd, "noconnect") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_CONNECTPOINTS] = false;
    if (findParameter(sCmd, "points") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_DRAWPOINTS] = true;
    if (findParameter(sCmd, "nopoints") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_DRAWPOINTS] = false;
    if (findParameter(sCmd, "open") && (nType == ALL || nType & SUPERGLOBAL))
        logicalSettings[LOG_OPENIMAGE] = true;
    if (findParameter(sCmd, "noopen") && (nType == ALL || nType & SUPERGLOBAL))
        logicalSettings[LOG_OPENIMAGE] = false;
    if (findParameter(sCmd, "interpolate") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_INTERPOLATE] = true;
    if (findParameter(sCmd, "nointerpolate") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_INTERPOLATE] = false;
    if (findParameter(sCmd, "hires") && (nType == ALL || nType & SUPERGLOBAL))
        intSettings[INT_HIGHRESLEVEL] = 2;
    if (findParameter(sCmd, "hires", '=') && (nType == ALL || nType & SUPERGLOBAL))
    {
        int nPos = findParameter(sCmd, "hires", '=')+5;
        if (getArgAtPos(sCmd, nPos) == "all")
        {
            logicalSettings[LOG_ALLHIGHRES] = true;
            intSettings[INT_HIGHRESLEVEL] = 2;
        }
        else if (getArgAtPos(sCmd, nPos) == "allmedium")
        {
            logicalSettings[LOG_ALLHIGHRES] = true;
            intSettings[INT_HIGHRESLEVEL] = 1;
        }
        else if (getArgAtPos(sCmd, nPos) == "medium")
        {
            intSettings[INT_HIGHRESLEVEL] = 1;
        }
    }
    if (findParameter(sCmd, "legend", '=') && (nType == ALL || nType & GLOBAL))
    {
        int nPos = findParameter(sCmd, "legend", '=')+6;
        if (getArgAtPos(sCmd, nPos) == "topleft" || getArgAtPos(sCmd, nPos) == "left")
            intSettings[INT_LEGENDPOSITION] = 2;
        else if (getArgAtPos(sCmd, nPos) == "bottomleft")
            intSettings[INT_LEGENDPOSITION] = 0;
        else if (getArgAtPos(sCmd, nPos) == "bottomright")
            intSettings[INT_LEGENDPOSITION] = 1;
        else
            intSettings[INT_LEGENDPOSITION] = 3;
    }
    if (findParameter(sCmd, "nohires") && (nType == ALL || nType & SUPERGLOBAL))
    {
        intSettings[INT_HIGHRESLEVEL] = 0;
        logicalSettings[LOG_ALLHIGHRES] = false;
    }
    if (findParameter(sCmd, "animate") && (nType == ALL || nType & SUPERGLOBAL))
        logicalSettings[LOG_ANIMATE] = true;
    if (findParameter(sCmd, "animate", '=') && (nType == ALL || nType & SUPERGLOBAL))
    {
        unsigned int nPos = findParameter(sCmd, "animate", '=')+7;
        _parser.SetExpr(getArgAtPos(__sCmd, nPos));
        intSettings[INT_ANIMATESAMPLES] = intCast(_parser.Eval());
        if (intSettings[INT_ANIMATESAMPLES] && !isinf(_parser.Eval()) && !isnan(_parser.Eval()))
            logicalSettings[LOG_ANIMATE] = true;
        else
        {
            intSettings[INT_ANIMATESAMPLES] = 50;
            logicalSettings[LOG_ANIMATE] = false;
        }
        if (intSettings[INT_ANIMATESAMPLES] > 128)
            intSettings[INT_ANIMATESAMPLES] = 128;
        if (intSettings[INT_ANIMATESAMPLES] < 1)
            intSettings[INT_ANIMATESAMPLES] = 50;
    }
    if (findParameter(sCmd, "marks", '=') && (nType == ALL || nType & LOCAL))
    {
        unsigned int nPos = findParameter(sCmd, "marks", '=')+5;
        _parser.SetExpr(getArgAtPos(__sCmd, nPos));
        intSettings[INT_MARKS] = intCast(_parser.Eval());
        if (!intSettings[INT_MARKS] || isinf(_parser.Eval().real()) || isnan(_parser.Eval().real()))
            intSettings[INT_MARKS] = 0;
        if (intSettings[INT_MARKS] > 9)
            intSettings[INT_MARKS] = 9;
        if (intSettings[INT_MARKS] < 0)
            intSettings[INT_MARKS] = 0;
    }
    if (findParameter(sCmd, "nomarks") && (nType == ALL || nType & LOCAL))
        intSettings[INT_MARKS] = 0;
    if (findParameter(sCmd, "textsize", '=') && (nType == ALL || nType & SUPERGLOBAL))
    {
        unsigned int nPos = findParameter(sCmd, "textsize", '=')+8;
        _parser.SetExpr(getArgAtPos(__sCmd, nPos));
        floatSettings[FLOAT_TEXTSIZE] = _parser.Eval().real();

        if (isinf(floatSettings[FLOAT_TEXTSIZE]) || isnan(floatSettings[FLOAT_TEXTSIZE]))
            floatSettings[FLOAT_TEXTSIZE] = 5;

        if (floatSettings[FLOAT_TEXTSIZE] <= -1)
            floatSettings[FLOAT_TEXTSIZE] = 5;
    }
    if (findParameter(sCmd, "aspect", '=') && (nType == ALL || nType & SUPERGLOBAL))
    {
        unsigned int nPos = findParameter(sCmd, "aspect", '=') + 6;
        _parser.SetExpr(getArgAtPos(__sCmd, nPos));
        floatSettings[FLOAT_ASPECT] = _parser.Eval().real();
        if (floatSettings[FLOAT_ASPECT] <= 0 || isnan(floatSettings[FLOAT_ASPECT]) || isinf(floatSettings[FLOAT_ASPECT]))
            floatSettings[FLOAT_ASPECT] = 4/3;
    }
    if (findParameter(sCmd, "noanimate") && (nType == ALL || nType & SUPERGLOBAL))
        logicalSettings[LOG_ANIMATE] = false;
    if (findParameter(sCmd, "silent") && (nType == ALL || nType & SUPERGLOBAL))
        logicalSettings[LOG_SILENTMODE] = true;
    if (findParameter(sCmd, "nosilent") && (nType == ALL || nType & SUPERGLOBAL))
        logicalSettings[LOG_SILENTMODE] = false;
    if (findParameter(sCmd, "cut") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_CUTBOX] = true;
    if (findParameter(sCmd, "nocut") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_CUTBOX] = false;
    if (findParameter(sCmd, "flow") && (nType == ALL || nType & LOCAL))
    {
        logicalSettings[LOG_FLOW] = true;
        logicalSettings[LOG_PIPE] = false;
    }
    if (findParameter(sCmd, "noflow") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_FLOW] = false;
    if (findParameter(sCmd, "pipe") && (nType == ALL || nType & LOCAL))
    {
        logicalSettings[LOG_PIPE] = true;
        logicalSettings[LOG_FLOW] = false;
    }
    if (findParameter(sCmd, "nopipe") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_PIPE] = false;
    if (findParameter(sCmd, "flength") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_FIXEDLENGTH] = true;
    if (findParameter(sCmd, "noflength") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_FIXEDLENGTH] = false;
    if (findParameter(sCmd, "colorbar") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_COLORBAR] = true;
    if (findParameter(sCmd, "nocolorbar") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_COLORBAR] = false;
    if (findParameter(sCmd, "orthoproject") && (nType == ALL || nType & GLOBAL))
        logicalSettings[LOG_ORTHOPROJECT] = true;
    if (findParameter(sCmd, "noorthoproject") && (nType == ALL || nType & GLOBAL))
        logicalSettings[LOG_ORTHOPROJECT] = false;
    if (findParameter(sCmd, "area") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_AREA] = true;
    if (findParameter(sCmd, "noarea") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_AREA] = false;
    if (findParameter(sCmd, "bars") && (nType == ALL || nType & LOCAL))
    {
        floatSettings[FLOAT_BARS] = 0.9;
        floatSettings[FLOAT_HBARS] = 0.0;
    }
    if (findParameter(sCmd, "bars", '=') && (nType == ALL || nType & LOCAL))
    {
        _parser.SetExpr(getArgAtPos(__sCmd, findParameter(sCmd, "bars", '=')+4));
        floatSettings[FLOAT_BARS] = _parser.Eval().real();
        if (floatSettings[FLOAT_BARS]
            && !isinf(_parser.Eval().real())
            && !isnan(_parser.Eval().real())
            && (floatSettings[FLOAT_BARS] < 0.0 || floatSettings[FLOAT_BARS] > 1.0))
            floatSettings[FLOAT_BARS] = 0.9;
        floatSettings[FLOAT_HBARS] = 0.0;
    }
    if (findParameter(sCmd, "hbars") && (nType == ALL || nType & LOCAL))
    {
        floatSettings[FLOAT_BARS] = 0.0;
        floatSettings[FLOAT_HBARS] = 0.9;
    }
    if (findParameter(sCmd, "hbars", '=') && (nType == ALL || nType & LOCAL))
    {
        _parser.SetExpr(getArgAtPos(__sCmd, findParameter(sCmd, "hbars", '=')+5));
        floatSettings[FLOAT_HBARS] = _parser.Eval().real();
        if (floatSettings[FLOAT_HBARS]
            && !isinf(_parser.Eval().real())
            && !isnan(_parser.Eval().real())
            && (floatSettings[FLOAT_HBARS] < 0.0 || floatSettings[FLOAT_HBARS] > 1.0))
            floatSettings[FLOAT_HBARS] = 0.9;
        floatSettings[FLOAT_BARS] = 0.0;
    }
    if ((findParameter(sCmd, "nobars") || findParameter(sCmd, "nohbars")) && (nType == ALL || nType & LOCAL))
    {
        floatSettings[FLOAT_BARS] = 0.0;
        floatSettings[FLOAT_HBARS] = 0.0;
    }
    if (findParameter(sCmd, "steps") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_STEPPLOT] = true;
    if (findParameter(sCmd, "nosteps") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_STEPPLOT] = false;
    if (findParameter(sCmd, "boxplot") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_BOXPLOT] = true;
    if (findParameter(sCmd, "noboxplot") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_BOXPLOT] = false;
    if (findParameter(sCmd, "colormask") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_COLORMASK] = true;
    if (findParameter(sCmd, "nocolormask") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_COLORMASK] = false;
    if (findParameter(sCmd, "alphamask") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_ALPHAMASK] = true;
    if (findParameter(sCmd, "noalphamask") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_ALPHAMASK] = false;
    if (findParameter(sCmd, "schematic") && (nType == ALL || nType & GLOBAL))
        logicalSettings[LOG_SCHEMATIC] = true;
    if (findParameter(sCmd, "noschematic") && (nType == ALL || nType & GLOBAL))
        logicalSettings[LOG_SCHEMATIC] = false;
    if (findParameter(sCmd, "perspective", '=') && (nType == ALL || nType & GLOBAL))
    {
        _parser.SetExpr(getArgAtPos(__sCmd, findParameter(sCmd, "perspective", '=')+11));
        floatSettings[FLOAT_PERSPECTIVE] = fabs(_parser.Eval());
        if (floatSettings[FLOAT_PERSPECTIVE] >= 1.0)
            floatSettings[FLOAT_PERSPECTIVE] = 0.0;
    }
    if (findParameter(sCmd, "noperspective") && (nType == ALL || nType & GLOBAL))
        floatSettings[FLOAT_PERSPECTIVE] = 0.0;
    if (findParameter(sCmd, "cloudplot") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_CLOUDPLOT] = true;
    if (findParameter(sCmd, "nocloudplot") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_CLOUDPLOT] = false;
    if (findParameter(sCmd, "region") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_REGION] = true;
    if (findParameter(sCmd, "noregion") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_REGION] = false;
    if ((findParameter(sCmd, "crust") || findParameter(sCmd, "reconstruct")) && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_CRUST] = true;
    if ((findParameter(sCmd, "nocrust") || findParameter(sCmd, "noreconstruct")) && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_CRUST] = false;
    if (findParameter(sCmd, "maxline", '=') && (nType == ALL || nType & LOCAL))
    {
        string sTemp = getArgAtPos(__sCmd, findParameter(sCmd, "maxline", '=')+7);
        if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
            sTemp = sTemp.substr(1,sTemp.length()-2);
        _lHlines[0].sDesc = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));
        if (sTemp.length())
            _lHlines[0].sStyle = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));
        replaceControlChars(_lHlines[0].sDesc);
    }
    if (findParameter(sCmd, "minline", '=') && (nType == ALL || nType & LOCAL))
    {
        string sTemp = getArgAtPos(__sCmd, findParameter(sCmd, "minline", '=')+7);
        if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
            sTemp = sTemp.substr(1,sTemp.length()-2);
        _lHlines[1].sDesc = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));
        if (sTemp.length())
            _lHlines[1].sStyle = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));
        replaceControlChars(_lHlines[1].sDesc);
    }
    if ((findParameter(sCmd, "hline", '=') || findParameter(sCmd, "hlines", '=')) && (nType == ALL || nType & LOCAL))
    {
        string sTemp;
        if (findParameter(sCmd, "hline", '='))
            sTemp = getArgAtPos(__sCmd, findParameter(sCmd, "hline", '=')+5);
        else
            sTemp = getArgAtPos(__sCmd, findParameter(sCmd, "hlines", '=')+6);
        if (sTemp.find(',') != string::npos)
        {
            if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
                sTemp = sTemp.substr(1,sTemp.length()-2);

            value_type* v = nullptr;
            int nResults = 0;
            v = evaluateNumerical(nResults, getNextArgument(sTemp, true));

            for (int i = 0; i < nResults; i++)
            {
                if (i)
                    _lHlines.push_back(Line());

                _lHlines[i+2].dPos = v[i].real();
            }

            string sDescList = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));

            if (sDescList.front() == '{')
                sDescList.erase(0,1);
            if (sDescList.back() == '}')
                sDescList.erase(sDescList.length()-1);
            for (size_t i = 2; i < _lHlines.size(); i++)
            {
                if (!sDescList.length())
                    break;
                _lHlines[i].sDesc = removeSurroundingQuotationMarks(getNextArgument(sDescList, true));
                replaceControlChars(_lHlines[i].sDesc);
            }

            if (sTemp.length())
            {
                string sStyles = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));
                if (sStyles.front() == '{')
                    sStyles.erase(0,1);
                if (sStyles.back() == '}')
                    sStyles.erase(sStyles.length()-1);
                for (size_t i = 2; i < _lHlines.size(); i++)
                {
                    if (!sStyles.length())
                        break;
                    _lHlines[i].sStyle = removeSurroundingQuotationMarks(getNextArgument(sStyles, true));
                }
            }
        }
    }
    if ((findParameter(sCmd, "vline", '=') || findParameter(sCmd, "vlines", '=')) && (nType == ALL || nType & LOCAL))
    {
        string sTemp;
        if (findParameter(sCmd, "vline", '='))
            sTemp = getArgAtPos(__sCmd, findParameter(sCmd, "vline", '=')+5);
        else
            sTemp = getArgAtPos(__sCmd, findParameter(sCmd, "vlines", '=')+6);
        if (sTemp.find(',') != string::npos)
        {
            if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
                sTemp = sTemp.substr(1,sTemp.length()-2);

            value_type* v = nullptr;
            int nResults = 0;
            v = evaluateNumerical(nResults, getNextArgument(sTemp, true));

            for (int i = 0; i < nResults; i++)
            {
                if (i)
                    _lVLines.push_back(Line());

                _lVLines[i+2].dPos = v[i].real();
            }

            string sDescList = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));

            if (sDescList.front() == '{')
                sDescList.erase(0,1);
            if (sDescList.back() == '}')
                sDescList.erase(sDescList.length()-1);
            for (size_t i = 2; i < _lVLines.size(); i++)
            {
                if (!sDescList.length())
                    break;
                _lVLines[i].sDesc = removeSurroundingQuotationMarks(getNextArgument(sDescList, true));
                replaceControlChars(_lVLines[i].sDesc);
            }

            if (sTemp.length())
            {
                string sStyles = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));
                if (sStyles.front() == '{')
                    sStyles.erase(0,1);
                if (sStyles.back() == '}')
                    sStyles.erase(sStyles.length()-1);
                for (size_t i = 2; i < _lVLines.size(); i++)
                {
                    if (!sStyles.length())
                        break;
                    _lVLines[i].sStyle = removeSurroundingQuotationMarks(getNextArgument(sStyles, true));
                }
            }
        }
    }
    if (findParameter(sCmd, "timeaxes", '=') && (nType == ALL || nType & GLOBAL))
    {
        string sTemp;
        sTemp = getArgAtPos(__sCmd, findParameter(sCmd, "timeaxes", '=')+8);

        if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
            sTemp = sTemp.substr(1,sTemp.length()-2);

        string sAxesList = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));

        if (sAxesList.front() == '{')
            sAxesList.erase(0,1);

        if (sAxesList.back() == '}')
            sAxesList.erase(sAxesList.length()-1);

        string sFormat;

        if (sTemp.length())
        {
            sFormat = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));

            if (sFormat.front() == '{')
                sFormat.erase(0,1);

            if (sFormat.back() == '}')
                sFormat.erase(sFormat.length()-1);
        }

        while (sAxesList.length())
        {
            string sAxis = removeSurroundingQuotationMarks(getNextArgument(sAxesList, true));

            if (sAxis == "c")
                _timeAxes[3].activate(removeSurroundingQuotationMarks(getNextArgument(sFormat, true)));
            else if (sAxis.find_first_of("xyz") != string::npos)
                _timeAxes[sAxis[0]-'x'].activate(removeSurroundingQuotationMarks(getNextArgument(sFormat, true)));
        }
    }
    if (findParameter(sCmd, "lborder", '=') && (nType == ALL || nType & LOCAL))
    {
        string sTemp = getArgAtPos(__sCmd, findParameter(sCmd, "lborder", '=')+7);
        if (sTemp.find(',') != string::npos)
        {
            if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
                sTemp = sTemp.substr(1,sTemp.length()-2);
            _parser.SetExpr(getNextArgument(sTemp, true));
            _lVLines[0].dPos = _parser.Eval().real();
            _lVLines[0].sDesc = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));
            if (sTemp.length())
                _lVLines[0].sStyle = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));
        }
        replaceControlChars(_lVLines[0].sDesc);
    }
    if (findParameter(sCmd, "rborder", '=') && (nType == ALL || nType & LOCAL))
    {
        string sTemp = getArgAtPos(__sCmd, findParameter(sCmd, "rborder", '=')+7);
        if (sTemp.find(',') != string::npos)
        {
            if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
                sTemp = sTemp.substr(1,sTemp.length()-2);
            _parser.SetExpr(getNextArgument(sTemp, true));
            _lVLines[1].dPos = _parser.Eval().real();
            _lVLines[1].sDesc = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));
            if (sTemp.length())
                _lVLines[1].sStyle = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));
        }
        replaceControlChars(_lVLines[1].sDesc);
    }
    if (findParameter(sCmd, "addxaxis", '=') && (nType == ALL || nType & GLOBAL))
    {
        string sTemp = getArgAtPos(__sCmd, findParameter(sCmd, "addxaxis", '=')+8);

        if (sTemp.find(',') != string::npos || sTemp.find('"') != string::npos)
        {
            if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
                sTemp = sTemp.substr(1,sTemp.length()-2);

            if (getNextArgument(sTemp, false).front() != '"')
            {
                _parser.SetExpr(getNextArgument(sTemp, true));
                mu::value_type minval = _parser.Eval();
                _parser.SetExpr(getNextArgument(sTemp, true));
                _AddAxes[0].ivl.reset(minval, _parser.Eval());

                if (getNextArgument(sTemp, false).length())
                {
                    _AddAxes[0].sLabel = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));

                    if (getNextArgument(sTemp, false).length())
                    {
                        _AddAxes[0].sStyle = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));

                        if (!checkColorChars(_AddAxes[0].sStyle))
                            _AddAxes[0].sStyle = "k";
                    }
                }
                else
                    _AddAxes[0].sLabel = "\\i x";
            }
            else
            {
                _AddAxes[0].sLabel = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));

                if (getNextArgument(sTemp, false).length())
                {
                    _AddAxes[0].sStyle = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));

                    if (!checkColorChars(_AddAxes[0].sStyle))
                        _AddAxes[0].sStyle = "k";
                }
            }
        }
    }
    if (findParameter(sCmd, "addyaxis", '=') && (nType == ALL || nType & GLOBAL))
    {
        string sTemp = getArgAtPos(__sCmd, findParameter(sCmd, "addyaxis", '=')+8);

        if (sTemp.find(',') != string::npos || sTemp.find('"') != string::npos)
        {
            if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
                sTemp = sTemp.substr(1,sTemp.length()-2);

            if (getNextArgument(sTemp, false).front() != '"')
            {
                _parser.SetExpr(getNextArgument(sTemp, true));
                mu::value_type minval = _parser.Eval();
                _parser.SetExpr(getNextArgument(sTemp, true));
                _AddAxes[1].ivl.reset(minval, _parser.Eval());

                if (getNextArgument(sTemp, false).length())
                {
                    _AddAxes[1].sLabel = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));

                    if (getNextArgument(sTemp, false).length())
                    {
                        _AddAxes[1].sStyle = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));

                        if (!checkColorChars(_AddAxes[0].sStyle))
                            _AddAxes[1].sStyle = "k";
                    }
                }
                else
                    _AddAxes[1].sLabel = "\\i y";
            }
            else
            {
                _AddAxes[1].sLabel = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));

                if (getNextArgument(sTemp, false).length())
                {
                    _AddAxes[1].sStyle = evaluateString(getArgAtPos(getNextArgument(sTemp, true),0));

                    if (!checkColorChars(_AddAxes[1].sStyle))
                        _AddAxes[1].sStyle = "k";
                }
            }
        }
        //replaceControlChars(_[1].sDesc);
    }
    if (findParameter(sCmd, "colorscheme", '=') && (nType == ALL || nType & LOCAL))
    {
        unsigned int nPos = findParameter(sCmd, "colorscheme", '=') + 11;
        while (sCmd[nPos] == ' ')
            nPos++;
        if (sCmd[nPos] == '"')
        {
            string __sColorScheme = __sCmd.substr(nPos+1, __sCmd.find('"', nPos+1)-nPos-1);
            StripSpaces(__sColorScheme);
            if (!checkColorChars(__sColorScheme))
                stringSettings[STR_COLORSCHEME] = "kRryw";
            else
            {
                if (__sColorScheme == "#" && stringSettings[STR_COLORSCHEME].find('#') == string::npos)
                    stringSettings[STR_COLORSCHEME] += '#';
                else if (__sColorScheme == "|" && stringSettings[STR_COLORSCHEME].find('|') == string::npos)
                    stringSettings[STR_COLORSCHEME] += '|';
                else if ((__sColorScheme == "#|" || __sColorScheme == "|#") && (stringSettings[STR_COLORSCHEME].find('#') == string::npos || stringSettings[STR_COLORSCHEME].find('|') == string::npos))
                {
                    if (stringSettings[STR_COLORSCHEME].find('#') == string::npos && stringSettings[STR_COLORSCHEME].find('|') != string::npos)
                        stringSettings[STR_COLORSCHEME] += '#';
                    else if (stringSettings[STR_COLORSCHEME].find('|') == string::npos && stringSettings[STR_COLORSCHEME].find('#') != string::npos)
                        stringSettings[STR_COLORSCHEME] += '|';
                    else
                        stringSettings[STR_COLORSCHEME] += "#|";
                }
                else if (__sColorScheme != "#" && __sColorScheme != "|" && __sColorScheme != "#|" && __sColorScheme != "|#")
                    stringSettings[STR_COLORSCHEME] = __sColorScheme;
            }
        }
        else
        {
            string sTemp = sCmd.substr(nPos, sCmd.find(' ', nPos+1)-nPos);
            StripSpaces(sTemp);
            if (sTemp == "rainbow")
                stringSettings[STR_COLORSCHEME] = "BbcyrR";
            else if (sTemp == "grey")
                stringSettings[STR_COLORSCHEME] = "kw";
            else if (sTemp == "hot")
                stringSettings[STR_COLORSCHEME] = "kRryw";
            else if (sTemp == "cold")
                stringSettings[STR_COLORSCHEME] = "kBncw";
            else if (sTemp == "copper")
                stringSettings[STR_COLORSCHEME] = "kQqw";
            else if (sTemp == "map")
                stringSettings[STR_COLORSCHEME] = "UBbcgyqRH";
            else if (sTemp == "moy")
                stringSettings[STR_COLORSCHEME] = "kMqyw";
            else if (sTemp == "coast")
                stringSettings[STR_COLORSCHEME] = "BCyw";
            else if (sTemp == "viridis" || sTemp == "std")
                stringSettings[STR_COLORSCHEME] = "UNC{e4}y";
            else if (sTemp == "plasma")
                stringSettings[STR_COLORSCHEME] = "B{u4}p{q6}{y7}";
            else
                stringSettings[STR_COLORSCHEME] = "kRryw";
        }
        if (stringSettings[STR_COLORSCHEME].length() > 32)
        {
            stringSettings[STR_COLORSCHEME] = "BbcyrR";
            stringSettings[STR_COLORSCHEMEMEDIUM] = "{B4}{b4}{c4}{y4}{r4}{R4}";
            stringSettings[STR_COLORSCHEMELIGHT] = "{B8}{b8}{c8}{y8}{r8}{R8}";
        }
        else
        {
            while (stringSettings[STR_COLORSCHEME].find(' ') != string::npos)
            {
                stringSettings[STR_COLORSCHEME].erase(stringSettings[STR_COLORSCHEME].find(' '),1);
            }
            stringSettings[STR_COLORSCHEMELIGHT] = "";
            stringSettings[STR_COLORSCHEMEMEDIUM] = "";
            for (unsigned int i = 0; i < stringSettings[STR_COLORSCHEME].length(); i++)
            {
                if (stringSettings[STR_COLORSCHEME][i] == '#' || stringSettings[STR_COLORSCHEME][i] == '|')
                {
                    stringSettings[STR_COLORSCHEMELIGHT] += stringSettings[STR_COLORSCHEME][i];
                    stringSettings[STR_COLORSCHEMEMEDIUM] += stringSettings[STR_COLORSCHEME][i];
                    continue;
                }
                if (stringSettings[STR_COLORSCHEME][i] == '{'
                    && i+3 < stringSettings[STR_COLORSCHEME].length()
                    && stringSettings[STR_COLORSCHEME][i+3] == '}')
                {
                    stringSettings[STR_COLORSCHEMELIGHT] += "{";
                    stringSettings[STR_COLORSCHEMEMEDIUM] += "{";
                    stringSettings[STR_COLORSCHEMELIGHT] += stringSettings[STR_COLORSCHEME][i+1];
                    stringSettings[STR_COLORSCHEMEMEDIUM] += stringSettings[STR_COLORSCHEME][i+1];
                    if (stringSettings[STR_COLORSCHEME][i+2] >= '2' && stringSettings[STR_COLORSCHEME][i+2] <= '8')
                    {
                        stringSettings[STR_COLORSCHEMEMEDIUM] += stringSettings[STR_COLORSCHEME][i+2]-1;
                        if (stringSettings[STR_COLORSCHEME][i+2] < '6')
                            stringSettings[STR_COLORSCHEMELIGHT] += stringSettings[STR_COLORSCHEME][i+2]+3;
                        else
                            stringSettings[STR_COLORSCHEMELIGHT] += "9";
                        stringSettings[STR_COLORSCHEMELIGHT] += "}";
                        stringSettings[STR_COLORSCHEMEMEDIUM] += "}";
                    }
                    else
                    {
                        stringSettings[STR_COLORSCHEMELIGHT] += stringSettings[STR_COLORSCHEME].substr(i+2,2);
                        stringSettings[STR_COLORSCHEMEMEDIUM] += stringSettings[STR_COLORSCHEME].substr(i+2,2);
                    }
                    i += 3;
                    continue;
                }
                stringSettings[STR_COLORSCHEMELIGHT] += "{";
                stringSettings[STR_COLORSCHEMELIGHT] += stringSettings[STR_COLORSCHEME][i];
                stringSettings[STR_COLORSCHEMELIGHT] += "8}";
                stringSettings[STR_COLORSCHEMEMEDIUM] += "{";
                stringSettings[STR_COLORSCHEMEMEDIUM] += stringSettings[STR_COLORSCHEME][i];
                stringSettings[STR_COLORSCHEMEMEDIUM] += "4}";
            }
        }
    }
    if (findParameter(sCmd, "bgcolorscheme", '=') && (nType == ALL || nType & LOCAL))
    {
        unsigned int nPos = findParameter(sCmd, "bgcolorscheme", '=') + 13;
        while (sCmd[nPos] == ' ')
            nPos++;
        if (sCmd[nPos] == '"')
        {
            string __sBGColorScheme = __sCmd.substr(nPos+1, __sCmd.find('"', nPos+1)-nPos-1);
            StripSpaces(__sBGColorScheme);
            if (!checkColorChars(__sBGColorScheme))
                stringSettings[STR_BACKGROUNDCOLORSCHEME] = "kRryw";
            else
            {
                if (__sBGColorScheme == "#"
                    && stringSettings[STR_BACKGROUNDCOLORSCHEME].find('#') == string::npos)
                    stringSettings[STR_BACKGROUNDCOLORSCHEME] += '#';
                else if (__sBGColorScheme == "|"
                         && stringSettings[STR_BACKGROUNDCOLORSCHEME].find('|') == string::npos)
                    stringSettings[STR_BACKGROUNDCOLORSCHEME] += '|';
                else if ((__sBGColorScheme == "#|" || __sBGColorScheme == "|#")
                         && (stringSettings[STR_BACKGROUNDCOLORSCHEME].find('#') == string::npos
                             || stringSettings[STR_BACKGROUNDCOLORSCHEME].find('|') == string::npos))
                {
                    if (stringSettings[STR_BACKGROUNDCOLORSCHEME].find('#') == string::npos
                        && stringSettings[STR_BACKGROUNDCOLORSCHEME].find('|') != string::npos)
                        stringSettings[STR_BACKGROUNDCOLORSCHEME] += '#';
                    else if (stringSettings[STR_BACKGROUNDCOLORSCHEME].find('|') == string::npos
                             && stringSettings[STR_BACKGROUNDCOLORSCHEME].find('#') != string::npos)
                        stringSettings[STR_BACKGROUNDCOLORSCHEME] += '|';
                    else
                        stringSettings[STR_BACKGROUNDCOLORSCHEME] += "#|";
                }
                else if (__sBGColorScheme != "#" && __sBGColorScheme != "|" && __sBGColorScheme != "#|" && __sBGColorScheme != "|#")
                    stringSettings[STR_BACKGROUNDCOLORSCHEME] = __sBGColorScheme;
            }
        }
        else
        {
            string sTemp = sCmd.substr(nPos, sCmd.find(' ', nPos+1)-nPos);
            StripSpaces(sTemp);
            if (sTemp == "rainbow")
                stringSettings[STR_BACKGROUNDCOLORSCHEME] = "BbcyrR";
            else if (sTemp == "grey")
                stringSettings[STR_BACKGROUNDCOLORSCHEME] = "kw";
            else if (sTemp == "hot")
                stringSettings[STR_BACKGROUNDCOLORSCHEME] = "kRryw";
            else if (sTemp == "cold")
                stringSettings[STR_BACKGROUNDCOLORSCHEME] = "kBbcw";
            else if (sTemp == "copper")
                stringSettings[STR_BACKGROUNDCOLORSCHEME] = "kQqw";
            else if (sTemp == "map")
                stringSettings[STR_BACKGROUNDCOLORSCHEME] = "UBbcgyqRH";
            else if (sTemp == "moy")
                stringSettings[STR_BACKGROUNDCOLORSCHEME] = "kMqyw";
            else if (sTemp == "coast")
                stringSettings[STR_BACKGROUNDCOLORSCHEME] = "BCyw";
            else if (sTemp == "viridis" || sTemp == "std")
                stringSettings[STR_BACKGROUNDCOLORSCHEME] = "UNC{e4}y";
            else if (sTemp == "plasma")
                stringSettings[STR_BACKGROUNDCOLORSCHEME] = "B{u4}p{q6}{y7}";
            else if (sTemp == "real")
                stringSettings[STR_BACKGROUNDCOLORSCHEME] = "<<REALISTIC>>";
            else
                stringSettings[STR_BACKGROUNDCOLORSCHEME] = "BbcyrR";
        }
        if (stringSettings[STR_BACKGROUNDCOLORSCHEME].length() > 32)
        {
            stringSettings[STR_BACKGROUNDCOLORSCHEME] = "BbcyrR";
        }
        while (stringSettings[STR_BACKGROUNDCOLORSCHEME].find(' ') != string::npos)
        {
            stringSettings[STR_BACKGROUNDCOLORSCHEME] = stringSettings[STR_BACKGROUNDCOLORSCHEME].substr(0, stringSettings[STR_BACKGROUNDCOLORSCHEME].find(' ')) + stringSettings[STR_BACKGROUNDCOLORSCHEME].substr(stringSettings[STR_BACKGROUNDCOLORSCHEME].find(' ')+1);
        }
    }
    if (findParameter(sCmd, "plotcolors", '=') && (nType == ALL || nType & LOCAL))
    {
        unsigned int nPos = findParameter(sCmd, "plotcolors", '=')+10;
        string sTemp = getArgAtPos(__sCmd, nPos);
        if (checkColorChars(sTemp))
        {
            for (unsigned int i = 0; i < sTemp.length(); i++)
            {
                if (i >= STYLES_COUNT)
                    break;
                if (sTemp[i] == ' ')
                    continue;
                stringSettings[STR_COLORS][i] = sTemp[i];
            }
        }
    }
    if (findParameter(sCmd, "axisbind", '=') && (nType == ALL || nType & LOCAL))
    {
        unsigned int nPos = findParameter(sCmd, "axisbind", '=')+8;
        string sTemp = getArgAtPos(__sCmd, nPos);
        for (unsigned int i = 0; i < sTemp.length(); i++)
        {
            if (sTemp[i] == 'r' || sTemp[i] == 'l')
            {
                if (sTemp.length() > i+1 && (sTemp[i+1] == 't' || sTemp[i+1] == 'b'))
                {
                    stringSettings[STR_AXISBIND] += sTemp.substr(i,2);
                    i++;
                }
                else if (sTemp.length() > i+1 && (sTemp[i+1] == ' ' || sTemp[i+1] == 'r' || sTemp[i+1] == 'l'))
                {
                    stringSettings[STR_AXISBIND] += sTemp.substr(i,1) + "b";
                    if (sTemp[i+1] == ' ')
                        i++;
                }
                else if (sTemp.length() == i+1)
                    stringSettings[STR_AXISBIND] += sTemp.substr(i) + "b";
                else
                    stringSettings[STR_AXISBIND] += "lb";
            }
            else if (sTemp[i] == 't' || sTemp[i] == 'b')
            {
                if (sTemp.length() > i+1 && (sTemp[i+1] == 'l' || sTemp[i+1] == 'r'))
                {
                    stringSettings[STR_AXISBIND] += sTemp.substr(i+1,1) + sTemp.substr(i,1);
                    i++;
                }
                else if (sTemp.length() > i+1 && (sTemp[i+1] == ' ' || sTemp[i+1] == 't' || sTemp[i+1] == 'b'))
                {
                    stringSettings[STR_AXISBIND] += "l" + sTemp.substr(i,1);
                    if (sTemp[i+1] == ' ')
                        i++;
                }
                else if (sTemp.length() == i+1)
                    stringSettings[STR_AXISBIND] += "l" + sTemp.substr(i);
                else
                    stringSettings[STR_AXISBIND] += "lb";
            }
            else if (sTemp.substr(i,2) == "  ")
            {
                stringSettings[STR_AXISBIND] += "lb";
                i++;
            }
        }
        if (stringSettings[STR_AXISBIND].find('l') == string::npos && stringSettings[STR_AXISBIND].length())
        {
            for (unsigned int i = 0; i < stringSettings[STR_AXISBIND].length(); i++)
            {
                if (stringSettings[STR_AXISBIND][i] == 'r')
                    stringSettings[STR_AXISBIND][i] = 'l';
            }
        }
        if (stringSettings[STR_AXISBIND].find('b') == string::npos && stringSettings[STR_AXISBIND].length())
        {
            for (unsigned int i = 0; i < stringSettings[STR_AXISBIND].length(); i++)
            {
                if (stringSettings[STR_AXISBIND][i] == 't')
                    stringSettings[STR_AXISBIND][i] = 'b';
            }
        }
    }
    if (findParameter(sCmd, "linestyles", '=') && (nType == ALL || nType & LOCAL))
    {
        unsigned int nPos = findParameter(sCmd, "linestyles", '=')+10;
        string sTemp = getArgAtPos(__sCmd, nPos);
        if (checkLineChars(sTemp))
        {
            for (unsigned int i = 0; i < sTemp.length(); i++)
            {
                if (i >= STYLES_COUNT)
                    break;
                if (sTemp[i] == ' ')
                    continue;
                stringSettings[STR_LINESTYLES][i] = sTemp[i];
                stringSettings[STR_LINESTYLESGREY][i] = sTemp[i];
            }
        }
    }
    if (findParameter(sCmd, "linesizes", '=') && (nType == ALL || nType & LOCAL))
    {
        unsigned int nPos = findParameter(sCmd, "linesizes", '=')+9;
        string sTemp = getArgAtPos(__sCmd, nPos);

        for (unsigned int i = 0; i < sTemp.length(); i++)
        {
            if (i >= STYLES_COUNT)
                break;
            if (sTemp[i] == ' ')
                continue;
            if (sTemp[i] < '0' || sTemp[i] > '9')
                continue;
            stringSettings[STR_LINESIZES][i] = sTemp[i];
        }

    }
    if (findParameter(sCmd, "pointstyles", '=') && (nType == ALL || nType & LOCAL))
    {
        unsigned int nPos = findParameter(sCmd, "pointstyles", '=')+11;
        string sTemp = getArgAtPos(__sCmd, nPos);
        if (checkPointChars(sTemp))
        {
            int nChar = 0;
            string sChar = "";
            for (unsigned int i = 0; i < sTemp.length(); i++)
            {
                sChar = "";
                if (i >= 2*STYLES_COUNT || nChar >= STYLES_COUNT)
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
                    stringSettings[STR_POINTSTYLES].replace(2*nChar, 2, sChar);
                    nChar++;
                    continue;
                }
                if (sTemp[i] != '#')
                {
                    sChar = " ";
                    sChar += sTemp[i];
                    stringSettings[STR_POINTSTYLES].replace(2*nChar, 2, sChar);
                    nChar++;
                }
            }
        }
    }
    if (findParameter(sCmd, "styles", '=') && (nType == ALL || nType & LOCAL))
    {
        unsigned int nPos = findParameter(sCmd, "styles", '=')+6;
        string sTemp = getArgAtPos(__sCmd, nPos);
        unsigned int nJump = 0;
        unsigned int nStyle = 0;
        for (unsigned int i = 0; i < sTemp.length(); i += 4)
        {
            nJump = 0;
            if (nStyle >= STYLES_COUNT)
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
                    stringSettings[STR_LINESIZES][nStyle] = sTemp[i+j];
                    continue;
                }
                if (sTemp[i+j] == '#' && i+j+1 < sTemp.length() && checkPointChars(sTemp.substr(i+j,2)))
                {
                    stringSettings[STR_POINTSTYLES][2*nStyle] = '#';
                    if (sTemp[i+j+1] != ' ')
                        stringSettings[STR_POINTSTYLES][2*nStyle+1] = sTemp[i+j+1];
                    j++;
                    continue;
                }
                else if (sTemp[i+j] == '#')
                    continue;
                if (checkPointChars(sTemp.substr(i+j,1)))
                {
                    stringSettings[STR_POINTSTYLES][2*nStyle] = ' ';
                    stringSettings[STR_POINTSTYLES][2*nStyle+1] = sTemp[i+j];
                    continue;
                }
                if (checkColorChars(sTemp.substr(i+j,1)))
                {
                    stringSettings[STR_COLORS][nStyle] = sTemp[i+j];
                    continue;
                }
                if (checkLineChars(sTemp.substr(i+j,1)))
                {
                    stringSettings[STR_LINESTYLES][nStyle] = sTemp[i+j];
                    stringSettings[STR_LINESTYLESGREY][nStyle] = sTemp[i+j];
                    continue;
                }
            }
            nStyle++;
            i += nJump;
        }
    }
    if (findParameter(sCmd, "gridstyle", '=') && (nType == ALL || nType & GLOBAL))
    {
        unsigned int nPos = findParameter(sCmd, "gridstyle", '=')+9;
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
                    stringSettings[STR_GRIDSTYLE][2+i] = sTemp[i+j];
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
                    stringSettings[STR_GRIDSTYLE][i] = sTemp[i+j];
                    continue;
                }
                if (checkLineChars(sTemp.substr(i+j,1)))
                {
                    stringSettings[STR_GRIDSTYLE][i+1] = sTemp[i+j];
                    continue;
                }
            }
        }
    }
    if (findParameter(sCmd, "legendstyle", '=') && (nType == ALL || nType & LOCAL))
    {
        if (getArgAtPos(sCmd, findParameter(sCmd, "legendstyle", '=')+11) == "onlycolors")
            intSettings[INT_LEGENDSTYLE] = 1;
        else if (getArgAtPos(sCmd, findParameter(sCmd, "legendstyle", '=')+11) == "onlystyles")
            intSettings[INT_LEGENDSTYLE] = 2;
        else
            intSettings[INT_LEGENDSTYLE] = 0;
    }
    if (findParameter(sCmd, "coords", '=') && (nType == ALL || nType & GLOBAL))
    {
        int nPos = findParameter(sCmd, "coords", '=')+6;

        if (getArgAtPos(sCmd, nPos) == "cartesian" || getArgAtPos(sCmd, nPos) == "std")
            intSettings[INT_COORDS] = CARTESIAN;
        else if (getArgAtPos(sCmd, nPos) == "polar" || getArgAtPos(sCmd, nPos) == "polar_pz" || getArgAtPos(sCmd, nPos) == "cylindrical")
            intSettings[INT_COORDS] = POLAR_PZ;
        else if (getArgAtPos(sCmd, nPos) == "polar_rp")
            intSettings[INT_COORDS] = POLAR_RP;
        else if (getArgAtPos(sCmd, nPos) == "polar_rz")
            intSettings[INT_COORDS] = POLAR_RZ;
        else if (getArgAtPos(sCmd, nPos) == "spherical" || getArgAtPos(sCmd, nPos) == "spherical_pt")
            intSettings[INT_COORDS] = SPHERICAL_PT;
        else if (getArgAtPos(sCmd, nPos) == "spherical_rp")
            intSettings[INT_COORDS] = SPHERICAL_RP;
        else if (getArgAtPos(sCmd, nPos) == "spherical_rt")
            intSettings[INT_COORDS] = SPHERICAL_RT;
    }
    if (findParameter(sCmd, "font", '=') && (nType == ALL || nType & SUPERGLOBAL))
    {
        string sTemp = getArgAtPos(sCmd, findParameter(sCmd, "font", '=')+4);
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
        if (sTemp != stringSettings[STR_FONTSTYLE]
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
            stringSettings[STR_FONTSTYLE] = sTemp;
            _fontData.LoadFont(stringSettings[STR_FONTSTYLE].c_str(), (sTokens[0][1]+ "\\fonts").c_str());
        }
    }
    if ((findParameter(sCmd, "opng", '=')
        || findParameter(sCmd, "save", '=')
        || findParameter(sCmd, "export", '=')
        || findParameter(sCmd, "opnga", '=')
        || findParameter(sCmd, "oeps", '=')
        || findParameter(sCmd, "obps", '=')
        || findParameter(sCmd, "osvg", '=')
        || findParameter(sCmd, "otex", '=')
        || findParameter(sCmd, "otif", '=')
        || findParameter(sCmd, "ogif", '=')) && (nType == ALL || nType & SUPERGLOBAL))
    {
        unsigned int nPos = 0;
        if (findParameter(sCmd, "opng", '='))
            nPos = findParameter(sCmd, "opng", '=') + 4;
        else if (findParameter(sCmd, "opnga", '='))
            nPos = findParameter(sCmd, "opnga", '=') + 5;
        else if (findParameter(sCmd, "save", '='))
            nPos = findParameter(sCmd, "save", '=') + 4;
        else if (findParameter(sCmd, "export", '='))
            nPos = findParameter(sCmd, "export", '=') + 6;
        else if (findParameter(sCmd, "oeps", '='))
            nPos = findParameter(sCmd, "oeps", '=') + 4;
        else if (findParameter(sCmd, "obps", '='))
            nPos = findParameter(sCmd, "obps", '=') + 4;
        else if (findParameter(sCmd, "osvg", '='))
            nPos = findParameter(sCmd, "osvg", '=') + 4;
        else if (findParameter(sCmd, "otex", '='))
            nPos = findParameter(sCmd, "otex", '=') + 4;
        else if (findParameter(sCmd, "otif", '='))
            nPos = findParameter(sCmd, "otif", '=') + 4;
        else if (findParameter(sCmd, "ogif", '='))
            nPos = findParameter(sCmd, "ogif", '=') + 4;

        stringSettings[STR_FILENAME] = evaluateString(getArgAtPos(__sCmd, nPos));
        StripSpaces(stringSettings[STR_FILENAME]);
        if (stringSettings[STR_FILENAME].length())
        {
            string sExtension = "";
            if (stringSettings[STR_FILENAME].length() > 4)
                sExtension = stringSettings[STR_FILENAME].substr(stringSettings[STR_FILENAME].length()-4,4);
            if (sExtension != ".png"
                && (findParameter(sCmd, "opng", '=')
                    || findParameter(sCmd, "opnga", '=')))
                stringSettings[STR_FILENAME] += ".png";
            else if (sExtension != ".eps" && findParameter(sCmd, "oeps", '='))
                stringSettings[STR_FILENAME] += ".eps";
            else if (sExtension != ".bps" && findParameter(sCmd, "obps", '='))
                stringSettings[STR_FILENAME] += ".bps";
            else if (sExtension != ".svg" && findParameter(sCmd, "osvg", '='))
                stringSettings[STR_FILENAME] += ".svg";
            else if (sExtension != ".tex" && findParameter(sCmd, "otex", '='))
                stringSettings[STR_FILENAME] += ".tex";
            else if (sExtension != ".tif" && sExtension != ".tiff" && findParameter(sCmd, "otif", '='))
                stringSettings[STR_FILENAME] += ".tiff";
            else if (sExtension != ".gif" && findParameter(sCmd, "ogif", '='))
                stringSettings[STR_FILENAME] += ".gif";
            else if ((findParameter(sCmd, "export", '=') || findParameter(sCmd, "save", '=')) && stringSettings[STR_FILENAME].rfind('.') == string::npos)
                stringSettings[STR_FILENAME] += ".png";

            stringSettings[STR_FILENAME] = FileSystem::ValidizeAndPrepareName(stringSettings[STR_FILENAME], stringSettings[STR_FILENAME].substr(stringSettings[STR_FILENAME].rfind('.')));
        }
    }
    if ((findParameter(sCmd, "xlabel", '=')
        || findParameter(sCmd, "ylabel", '=')
        || findParameter(sCmd, "zlabel", '=')
        || findParameter(sCmd, "title", '=')
        || findParameter(sCmd, "background", '=')) && (nType == ALL || nType & GLOBAL))
    {
        int nPos = 0;
        if (findParameter(sCmd, "xlabel", '='))
        {
            nPos = findParameter(sCmd, "xlabel", '=') + 6;
            sAxisLabels[0] = getArgAtPos(__sCmd, nPos);
            bDefaultAxisLabels[0] = false;
        }
        if (findParameter(sCmd, "ylabel", '='))
        {
            nPos = findParameter(sCmd, "ylabel", '=') + 6;
            sAxisLabels[1] = getArgAtPos(__sCmd, nPos);
            bDefaultAxisLabels[1] = false;
        }
        if (findParameter(sCmd, "zlabel", '='))
        {
            nPos = findParameter(sCmd, "zlabel", '=') + 6;
            sAxisLabels[2] = getArgAtPos(__sCmd, nPos);
            bDefaultAxisLabels[2] = false;
        }
        if (findParameter(sCmd, "title", '='))
        {
            nPos = findParameter(sCmd, "title", '=') + 5;
            stringSettings[STR_PLOTTITLE] = getArgAtPos(__sCmd, nPos);
            StripSpaces(stringSettings[STR_PLOTTITLE]);
            if (stringSettings[STR_COMPOSEDTITLE].length())
                stringSettings[STR_COMPOSEDTITLE] += ", " + stringSettings[STR_PLOTTITLE];
            else
                stringSettings[STR_COMPOSEDTITLE] = stringSettings[STR_PLOTTITLE];
        }
        if (findParameter(sCmd, "background", '='))
        {
            nPos = findParameter(sCmd, "background", '=')+10;
            stringSettings[STR_BACKGROUND] = getArgAtPos(__sCmd, nPos);
            StripSpaces(stringSettings[STR_BACKGROUND]);
            if (stringSettings[STR_BACKGROUND].length())
            {
                if (stringSettings[STR_BACKGROUND].find('.') == string::npos)
                    stringSettings[STR_BACKGROUND] += ".png";
                else if (stringSettings[STR_BACKGROUND].substr(stringSettings[STR_BACKGROUND].rfind('.')) != ".png")
                    stringSettings[STR_BACKGROUND] = "";
                if (stringSettings[STR_BACKGROUND].length())
                {
                    stringSettings[STR_BACKGROUND] = FileSystem::ValidFileName(stringSettings[STR_BACKGROUND], ".png");
                }
            }
        }
        for (int i = 0; i < 3; i++)
        {
            StripSpaces(sAxisLabels[i]);
        }
    }
    if ((findParameter(sCmd, "xticks", '=')
        || findParameter(sCmd, "yticks", '=')
        || findParameter(sCmd, "zticks", '=')
        || findParameter(sCmd, "cticks", '=')) && (nType == ALL || nType & GLOBAL))
    {
        if (findParameter(sCmd, "xticks", '='))
        {
            sTickTemplate[0] = getArgAtPos(__sCmd, findParameter(sCmd, "xticks", '=')+6);
            if (sTickTemplate[0].find('%') == string::npos && sTickTemplate[0].length())
                sTickTemplate[0] += "%g";
        }
        if (findParameter(sCmd, "yticks", '='))
        {
            sTickTemplate[1] = getArgAtPos(__sCmd, findParameter(sCmd, "yticks", '=')+6);
            if (sTickTemplate[1].find('%') == string::npos && sTickTemplate[1].length())
                sTickTemplate[1] += "%g";
        }
        if (findParameter(sCmd, "zticks", '='))
        {
            sTickTemplate[2] = getArgAtPos(__sCmd, findParameter(sCmd, "zticks", '=')+6);
            if (sTickTemplate[2].find('%') == string::npos && sTickTemplate[2].length())
                sTickTemplate[2] += "%g";
        }
        if (findParameter(sCmd, "cticks", '='))
        {
            sTickTemplate[3] = getArgAtPos(__sCmd, findParameter(sCmd, "cticks", '=')+6);
            if (sTickTemplate[3].find('%') == string::npos && sTickTemplate[3].length())
                sTickTemplate[3] += "%g";
        }
    }
    if ((findParameter(sCmd, "xscale", '=')
        || findParameter(sCmd, "yscale", '=')
        || findParameter(sCmd, "zscale", '=')
        || findParameter(sCmd, "cscale", '=')) && (nType == ALL || nType & GLOBAL))
    {
        if (findParameter(sCmd, "xscale", '='))
        {
            _parser.SetExpr(getArgAtPos(__sCmd, findParameter(sCmd, "xscale", '=')+6));
            dAxisScale[0] = _parser.Eval().real();
        }
        if (findParameter(sCmd, "yscale", '='))
        {
            _parser.SetExpr(getArgAtPos(__sCmd, findParameter(sCmd, "yscale", '=')+6));
            dAxisScale[1] = _parser.Eval().real();
        }
        if (findParameter(sCmd, "zscale", '='))
        {
            _parser.SetExpr(getArgAtPos(__sCmd, findParameter(sCmd, "zscale", '=')+6));
            dAxisScale[2] = _parser.Eval().real();
        }
        if (findParameter(sCmd, "cscale", '='))
        {
            _parser.SetExpr(getArgAtPos(__sCmd, findParameter(sCmd, "cscale", '=')+6));
            dAxisScale[3] = _parser.Eval().real();
        }

        for (int i = 0; i < 4; i++)
        {
            if (dAxisScale[i] == 0)
                dAxisScale[i] = 1.0;
        }
    }
    if ((findParameter(sCmd, "xticklabels", '=')
        || findParameter(sCmd, "yticklabels", '=')
        || findParameter(sCmd, "zticklabels", '=')
        || findParameter(sCmd, "cticklabels", '=')) && (nType == ALL || nType & GLOBAL))
    {
        if (findParameter(sCmd, "xticklabels", '='))
        {
            sCustomTicks[0] = getArgAtPos(__sCmd, findParameter(sCmd, "xticklabels", '=')+11);
        }
        if (findParameter(sCmd, "yticklabels", '='))
        {
            sCustomTicks[1] = getArgAtPos(__sCmd, findParameter(sCmd, "yticklabels", '=')+11);
        }
        if (findParameter(sCmd, "zticklabels", '='))
        {
            sCustomTicks[2] = getArgAtPos(__sCmd, findParameter(sCmd, "zticklabels", '=')+11);
        }
        if (findParameter(sCmd, "cticklabels", '='))
        {
            sCustomTicks[3] = getArgAtPos(__sCmd, findParameter(sCmd, "cticklabels", '=')+11);
        }
    }
    if (sCmd.find('[') != string::npos && (nType == ALL || nType & GLOBAL))
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
            auto args = getAllArguments(__sCmd.substr(nPos, sCmd.find(']', nPos) - nPos));

            for (size_t i = 0; i < args.size(); i++)
            {
                if (i > 4)
                    break;

                if (args[i].find(':') == string::npos || args[i] == ":")
                    continue;

                ranges[i].reset(args[i]);

                if (i < 4)
                {
                    bRanges[i] = true;
                    nRanges = i+1;

                    if (ranges[i].front().real() > ranges[i].back().real())
                    {
                        bMirror[i] = true;
                        ranges[i].reset(ranges[i].back(), ranges[i].front());
                    }
                }
            }

            for (size_t i = 0; i < ranges.size(); i++)
            {
                if (isinf(ranges[i].front()) || isnan(ranges[i].front()))
                    ranges[i].reset(-10.0, ranges[i].back());

                if (isinf(ranges[i].back()) || isnan(ranges[i].back()))
                    ranges[i].reset(ranges[i].front(), 10.0);
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
    else if (nType == ALL || nType & GLOBAL)
        nRanges = 0;

    return;
}

// --> Alle Einstellungen zuruecksetzen <--
void PlotData::reset()
{
    _lVLines.clear();
    _lHlines.clear();

    ranges.intervals.clear();
    ranges.intervals.resize(3, Interval(-10.0, 10.0));
    ranges.intervals.push_back(Interval(NAN, NAN));
    ranges.intervals.push_back(Interval(0, 1.0));

    ranges.setNames({"x", "y", "z", "c", "t"});

    for (int i = 0; i < 3; i++)
    {
        bRanges[i] = false;
        bMirror[i] = false;
        dOrigin[i] = 0.0;
        sAxisLabels[i] = "";
        bDefaultAxisLabels[i] = true;
        _lHlines.push_back(Line());
        _lVLines.push_back(Line());
        nSlices[i] = 1;
    }

    for (int i = 0; i < 4; i++)
    {
        bLogscale[i] = false;
        sTickTemplate[i] = "";
        sCustomTicks[i] = "";
        dAxisScale[i] = 1.0;
        _timeAxes[i].deactivate();
    }

    for (int i = 0; i < 2; i++)
    {
        _AddAxes[i].ivl.reset(NAN, NAN);
        _AddAxes[i].sLabel = "";
        _AddAxes[i].sStyle = "k";
    }

    for (size_t i = 0; i < LOG_SETTING_SIZE; i++)
    {
        logicalSettings[i] = false;
    }

    logicalSettings[LOG_AXIS] = true;
    logicalSettings[LOG_OPENIMAGE] = true;
    logicalSettings[LOG_COLORBAR] = true;

    for (size_t i = 0; i < INT_SETTING_SIZE; i++)
    {
        intSettings[i] = 0;
    }

    intSettings[INT_SAMPLES] = 100;
    intSettings[INT_CONTLINES] = 35;
    intSettings[INT_ANIMATESAMPLES] = 50;
    intSettings[INT_LEGENDPOSITION] = 3;

    for (size_t i = 0; i < FLOAT_SETTING_SIZE; i++)
    {
        floatSettings[i] = 0.0;
    }

    floatSettings[FLOAT_ASPECT] = 4.0/3.0;
    floatSettings[FLOAT_ALPHAVAL] = 0.5;
    floatSettings[FLOAT_TEXTSIZE] = 5;

    for (size_t i = 0; i < STR_SETTING_SIZE; i++)
    {
        stringSettings[i].clear();
    }

    stringSettings[STR_COLORSCHEME] = "UNC{e4}y";
    stringSettings[STR_COLORSCHEMEMEDIUM] = "{U4}{N4}{C4}{e3}{y4}";
    stringSettings[STR_COLORSCHEMELIGHT] = "{U8}{N8}{C8}{e7}{y8}";
    stringSettings[STR_BACKGROUNDCOLORSCHEME] = "<<REALISTIC>>";
    stringSettings[STR_COLORS] =         "rbGqmPunclRBgQMpUNCL";
    stringSettings[STR_CONTCOLORS] =     "kUHYPCQNLMhuWypcqnlm";
    stringSettings[STR_CONTGREYS] =      "kwkwkwkwkwkwkwkwkwkw";
    stringSettings[STR_POINTSTYLES] =    " + x o s . d#+#x#.#* x o s . d#+#x#.#* +";
    stringSettings[STR_LINESTYLES] =     "----------;;;;;;;;;;";
    stringSettings[STR_LINESIZES] =      "00000000000000000000";
    stringSettings[STR_GREYS] =          "kHhWkHhWkHhWkHhWkHhW";
    stringSettings[STR_LINESTYLESGREY] = "-|=;i:j|=;i:j-|=:i;-";
    stringSettings[STR_GRIDSTYLE] = "=h0-h0";

    dRotateAngles[0] = 60;
    dRotateAngles[1] = 115;
    nTargetGUI[0] = -1;
    nTargetGUI[1] = -1;
    nRanges = 0;
    nRequestedLayers = 1;

    if (NumeReKernel::getInstance())
    {
        if (stringSettings[STR_FONTSTYLE] != NumeReKernel::getInstance()->getSettings().getDefaultPlotFont())
            stringSettings[STR_FONTSTYLE] = NumeReKernel::getInstance()->getSettings().getDefaultPlotFont();
    }
    else
        stringSettings[STR_FONTSTYLE] = "pagella";

    _fontData.LoadFont(stringSettings[STR_FONTSTYLE].c_str(), (sTokens[0][1] + "\\fonts").c_str());
}

// --> Daten im Speicher loeschen. Speicher selbst bleibt bestehen <--
void PlotData::deleteData(bool bGraphFinished /* = false*/)
{
    _lHlines.clear();
    _lVLines.clear();

    ranges.intervals.clear();
    ranges.intervals.resize(3, Interval(-10.0, 10.0));
    ranges.intervals.push_back(Interval(NAN, NAN));
    ranges.intervals.push_back(Interval(0.0, 1.0));

    ranges.setNames({"x", "y", "z", "c", "t"});

    for (int i = 0; i < 3; i++)
    {
        bRanges[i] = false;
        bMirror[i] = false;
        sAxisLabels[i] = "";
        bDefaultAxisLabels[i] = true;
        _lHlines.push_back(Line());
        _lVLines.push_back(Line());
    }

    for (int i = 0; i < 4; i++)
    {
        sCustomTicks[i] = "";
        dAxisScale[i] = 1.0;
        _timeAxes[i].deactivate();
    }

    nRanges = 0;

    if (bGraphFinished)
    {
        stringSettings[STR_COMPOSEDTITLE].clear();
        nTargetGUI[0] = -1;
        nTargetGUI[1] = -1;
    }

    if (!logicalSettings[LOG_ALLHIGHRES])
        intSettings[INT_HIGHRESLEVEL] = 0;

    nRequestedLayers = 1;
    intSettings[INT_LEGENDSTYLE] = 0;

    stringSettings[STR_COLORS] =         "rbGqmPunclRBgQMpUNCL";
    stringSettings[STR_CONTCOLORS] =     "kUHYPCQNLMhuWypcqnlm";
    stringSettings[STR_CONTGREYS] =      "kwkwkwkwkwkwkwkwkwkw";
    stringSettings[STR_POINTSTYLES] =    " + x o s . d#+#x#.#* x o s . d#+#x#.#* +";
    stringSettings[STR_LINESTYLES] =     "----------;;;;;;;;;;";
    stringSettings[STR_LINESIZES] =      "00000000000000000000";
    stringSettings[STR_GREYS] =          "kHhWkHhWkHhWkHhWkHhW";
    stringSettings[STR_LINESTYLESGREY] = "-|=;i:j|=;i:j-|=:i;-";
    stringSettings[STR_FILENAME].clear();
    stringSettings[STR_PLOTTITLE].clear();
    stringSettings[STR_AXISBIND].clear();
    stringSettings[STR_BACKGROUND].clear();

    for (int i = 0; i < 2; i++)
    {
        _AddAxes[i].ivl.reset(NAN, NAN);
        _AddAxes[i].sLabel = "";
        _AddAxes[i].sStyle = "k";
    }
}

/* --> Plotparameter als String lesen: Gibt nur Parameter zurueck, die von Plot zu Plot
 *     uebernommen werden. (sFileName und sAxisLabels[] z.B nicht) <--
 */
string PlotData::getParams(bool asstr) const
{
    const Settings& _option = NumeReKernel::getInstance()->getSettings();
    string sReturn = "";
    string sSepString = "; ";
    if (asstr)
    {
        sReturn = "\"";
        sSepString = "\", \"";
    }
    sReturn += "[";
    for (size_t i = 0; i < ranges.size(); i++)
    {
        sReturn += toString(ranges[i].front(), _option.getPrecision()) + ":" + toString(ranges[i].back(), _option.getPrecision());
        if (i < 2)
            sReturn += ", ";
    }
    sReturn += "]" + sSepString;
    if (logicalSettings[LOG_ALPHA])
        sReturn += "alpha" + sSepString;
    if (logicalSettings[LOG_ALPHAMASK])
        sReturn += "alphamask" +sSepString;
    if (logicalSettings[LOG_ANIMATE])
        sReturn += "animate [" + toString(intSettings[INT_ANIMATESAMPLES]) + " frames]" + sSepString;
    if (logicalSettings[LOG_AREA])
        sReturn += "area" + sSepString;
    sReturn += "aspect=" + toString(floatSettings[FLOAT_ASPECT], 4) + sSepString;
    if (logicalSettings[LOG_AXIS])
        sReturn += "axis" + sSepString;
    if (stringSettings[STR_AXISBIND].length())
    {
        sReturn += "axisbind=";
        if (asstr)
            sReturn += "\\\"" + stringSettings[STR_AXISBIND] + "\\\"";
        else
            sReturn += "\"" + stringSettings[STR_AXISBIND] + "\"";
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
    if (floatSettings[FLOAT_BARS])
        sReturn += "bars=" + toString(floatSettings[FLOAT_BARS], 4) + sSepString;
    if (stringSettings[STR_BACKGROUND].length())
    {
        if (asstr)
            sReturn += "background=\\\"" + stringSettings[STR_BACKGROUND] + "\\\"" +  sSepString;
        else
            sReturn += "background=\"" + stringSettings[STR_BACKGROUND] + "\"" + sSepString;
    }
    sReturn += "bgcolorscheme=";
    if (stringSettings[STR_BACKGROUNDCOLORSCHEME] == "BbcyrR")
        sReturn += "rainbow" + sSepString;
    else if (stringSettings[STR_BACKGROUNDCOLORSCHEME] == "kw")
        sReturn += "grey" + sSepString;
    else if (stringSettings[STR_BACKGROUNDCOLORSCHEME] == "kRryw")
        sReturn += "hot" + sSepString;
    else if (stringSettings[STR_BACKGROUNDCOLORSCHEME] == "kBncw")
        sReturn += "cold" + sSepString;
    else if (stringSettings[STR_BACKGROUNDCOLORSCHEME] == "kQqw")
        sReturn += "copper" + sSepString;
    else if (stringSettings[STR_BACKGROUNDCOLORSCHEME] == "UBbcgyqRH")
        sReturn += "map" + sSepString;
    else if (stringSettings[STR_BACKGROUNDCOLORSCHEME] == "kMqyw")
        sReturn += "moy" + sSepString;
    else if (stringSettings[STR_BACKGROUNDCOLORSCHEME] == "BCyw")
        sReturn += "coast" + sSepString;
    else if (stringSettings[STR_BACKGROUNDCOLORSCHEME] == "UNC{e4}y")
        sReturn += "viridis" + sSepString;
    else if (stringSettings[STR_BACKGROUNDCOLORSCHEME] == "B{u4}p{q6}{y7}")
        sReturn += "plasma" + sSepString;
    else if (stringSettings[STR_BACKGROUNDCOLORSCHEME] == "<<REALISTIC>>")
        sReturn += "real" + sSepString;
    else
    {
        if (asstr)
            sReturn += "\\\"" + stringSettings[STR_BACKGROUNDCOLORSCHEME] + "\\\"" + sSepString;
        else
            sReturn += "\"" + stringSettings[STR_BACKGROUNDCOLORSCHEME] + "\"" + sSepString;
    }
    if (logicalSettings[LOG_BOX])
        sReturn += "box" + sSepString;
    if (logicalSettings[LOG_BOXPLOT])
        sReturn += "boxplot" + sSepString;
    if (logicalSettings[LOG_CLOUDPLOT])
        sReturn += "cloudplot" + sSepString;
    if (logicalSettings[LOG_COLORBAR])
        sReturn += "colorbar" + sSepString;
    if (logicalSettings[LOG_COLORMASK])
        sReturn += "colormask" + sSepString;
    sReturn += "colorscheme=";
    if (stringSettings[STR_COLORSCHEME] == "BbcyrR")
        sReturn += "rainbow" + sSepString;
    else if (stringSettings[STR_COLORSCHEME] == "kw")
        sReturn += "grey" + sSepString;
    else if (stringSettings[STR_COLORSCHEME] == "kRryw")
        sReturn += "hot" + sSepString;
    else if (stringSettings[STR_COLORSCHEME] == "kBncw")
        sReturn += "cold" + sSepString;
    else if (stringSettings[STR_COLORSCHEME] == "kQqw")
        sReturn += "copper" + sSepString;
    else if (stringSettings[STR_COLORSCHEME] == "UBbcgyqRH")
        sReturn += "map" + sSepString;
    else if (stringSettings[STR_COLORSCHEME] == "kMqyw")
        sReturn += "moy" + sSepString;
    else if (stringSettings[STR_COLORSCHEME] == "BCyw")
        sReturn += "coast" + sSepString;
    else if (stringSettings[STR_COLORSCHEME] == "UNC{e4}y")
        sReturn += "viridis" + sSepString;
    else if (stringSettings[STR_COLORSCHEME] == "B{u4}p{q6}{y7}")
        sReturn += "plasma" + sSepString;
    else
    {
        if (asstr)
            sReturn += "\\\"" + stringSettings[STR_COLORSCHEME] + "\\\"" + sSepString;
        else
            sReturn += "\"" + stringSettings[STR_COLORSCHEME] + "\"" + sSepString;
    }
    if (logicalSettings[LOG_CONNECTPOINTS])
        sReturn += "connect" + sSepString;
    if (logicalSettings[LOG_CRUST])
        sReturn += "crust" + sSepString;
    if (intSettings[INT_COORDS] >= 100)
        sReturn += "spherical coords" + sSepString;
    else if (intSettings[INT_COORDS] >= 10)
        sReturn += "polar coords" + sSepString;
    if (logicalSettings[LOG_CUTBOX])
        sReturn += "cutbox" + sSepString;
    if (logicalSettings[LOG_XERROR] && logicalSettings[LOG_YERROR])
        sReturn += "errorbars" + sSepString;
    else if (logicalSettings[LOG_XERROR])
        sReturn += "xerrorbars" + sSepString;
    else if (logicalSettings[LOG_YERROR])
        sReturn += "yerrorbars" + sSepString;
    if (logicalSettings[LOG_FIXEDLENGTH])
        sReturn += "fixed length" + sSepString;
    if (logicalSettings[LOG_FLOW])
        sReturn += "flow" + sSepString;
    sReturn += "font="+stringSettings[STR_FONTSTYLE]+sSepString;
    if (intSettings[INT_GRID] == 1)
        sReturn += "grid=coarse" + sSepString;
    else if (intSettings[INT_GRID] == 2)
        sReturn += "grid=fine" + sSepString;
    sReturn += "gridstyle=";
    if (asstr)
        sReturn += "\\\"" + stringSettings[STR_GRIDSTYLE] + "\\\"";
    else
        sReturn += "\"" + stringSettings[STR_GRIDSTYLE] + "\"";
    sReturn += sSepString;
    if (floatSettings[FLOAT_HBARS])
        sReturn += "hbars=" + toString(floatSettings[FLOAT_HBARS], 4) + sSepString;
    if (logicalSettings[LOG_INTERPOLATE])
        sReturn += "interpolate" + sSepString;
    if (logicalSettings[LOG_CONTLABELS])
        sReturn += "lcont" + sSepString;
    sReturn += "legend=";
    if (intSettings[INT_LEGENDPOSITION] == 0)
        sReturn += "bottomleft";
    else if (intSettings[INT_LEGENDPOSITION] == 1)
        sReturn += "bottomright";
    else if (intSettings[INT_LEGENDPOSITION] == 2)
        sReturn += "topleft";
    else
        sReturn += "topright";
    sReturn += sSepString;
    if (intSettings[INT_LEGENDSTYLE] == 1)
        sReturn += "legendstyle=onlycolors" + sSepString;
    if (intSettings[INT_LEGENDSTYLE] == 2)
        sReturn += "legendstyle=onlystyles" + sSepString;
    if (intSettings[INT_LIGHTING] == 1)
        sReturn += "lighting" + sSepString;
    if (intSettings[INT_LIGHTING] == 2)
        sReturn += "lighting=smooth" + sSepString;
    if (asstr)
        sReturn += "linesizes=\\\"" + stringSettings[STR_LINESIZES] + "\\\"" + sSepString;
    else
        sReturn += "linesizes=\"" + stringSettings[STR_LINESIZES] + "\"" + sSepString;
    if (asstr)
        sReturn += "linestyles=\\\"" + stringSettings[STR_LINESTYLES] + "\\\"" + sSepString;
    else
        sReturn += "linestyles=\"" + stringSettings[STR_LINESTYLES] + "\"" + sSepString;
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
    if (intSettings[INT_MARKS])
        sReturn += "marks=" + toString(intSettings[INT_MARKS]) + sSepString;
    if (logicalSettings[LOG_OPENIMAGE])
        sReturn += "open" + sSepString;
    sReturn += "origin=";
    if (isnan(dOrigin[0]) && isnan(dOrigin[1]) && isnan(dOrigin[2]))
        sReturn += "sliding" + sSepString;
    else if (dOrigin[0] == 0.0 && dOrigin[1] == 0.0 && dOrigin[2] == 0.0)
        sReturn += "std" + sSepString;
    else
        sReturn += "[" + toString(dOrigin[0], _option) + ", " + toString(dOrigin[1], _option) + ", " + toString(dOrigin[2], _option) + "]" + sSepString;
    sReturn += "slices=[" +toString((int)nSlices[0]) + ", " + toString((int)nSlices[1]) + ", " + toString((int)nSlices[2]) + "]" + sSepString;
    if (logicalSettings[LOG_STEPPLOT])
        sReturn += "steps" + sSepString;
    if (logicalSettings[LOG_ORTHOPROJECT])
        sReturn += "orthogonal projection" + sSepString;
    if (logicalSettings[LOG_CONTPROJ])
        sReturn += "pcont" + sSepString;
    if (floatSettings[FLOAT_PERSPECTIVE])
        sReturn += "perspective=" + toString(floatSettings[FLOAT_PERSPECTIVE], _option) + sSepString;
    if (asstr)
        sReturn += "plotcolors=\\\"" + stringSettings[STR_COLORS] + "\\\"" + sSepString;
    else
        sReturn += "plotcolors=\"" + stringSettings[STR_COLORS] + "\"" + sSepString;
    if (asstr)
        sReturn += "pointstyles=\\\"" + stringSettings[STR_POINTSTYLES] + "\\\"" + sSepString;
    else
        sReturn += "pointstyles=\"" + stringSettings[STR_POINTSTYLES] + "\"" + sSepString;
    if (logicalSettings[LOG_PIPE])
        sReturn += "pipe" + sSepString;
    if (logicalSettings[LOG_DRAWPOINTS])
        sReturn += "points" + sSepString;
    if (logicalSettings[LOG_REGION])
        sReturn += "region" + sSepString;
    if (intSettings[INT_HIGHRESLEVEL])
    {
        if (intSettings[INT_HIGHRESLEVEL] == 1)
            sReturn += "medium";
        else
            sReturn += "high";
        sReturn += " resolution" + sSepString;
    }
    sReturn += "rotate=" + toString(dRotateAngles[0], _option) + "," + toString(dRotateAngles[1], _option) + sSepString;
    sReturn += "samples=" + toString(intSettings[INT_HIGHRESLEVEL]) + sSepString;
    if (logicalSettings[LOG_SCHEMATIC])
        sReturn += "schematic" + sSepString;
    if (logicalSettings[LOG_SILENTMODE])
        sReturn += "silent mode" + sSepString;
    sReturn += "textsize=" + toString(floatSettings[FLOAT_TEXTSIZE], _option) + sSepString;
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
        sReturn += "\"";
    return sReturn;
}


// --> Datenpunkte einstellen <--
void PlotData::setSamples(int _nSamples)
{
    intSettings[INT_SAMPLES] = _nSamples;
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
                stringSettings[STR_FILENAME] = sPath.substr(0,sPath.length()-1)+"/"+_sFileName+"\"";
            else
                stringSettings[STR_FILENAME] = sPath + "/" + _sFileName;
        }
        else
            stringSettings[STR_FILENAME] = _sFileName;

        stringSettings[STR_FILENAME] = FileSystem::ValidizeAndPrepareName(stringSettings[STR_FILENAME], sExt);
    }
    else
        stringSettings[STR_FILENAME] = "";
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

string PlotData::removeSurroundingQuotationMarks(const string& sString)
{
    if (sString.front() == '"' && sString.back() == '"')
        return sString.substr(1,sString.length()-2);
    return sString;
}


struct AxisLabels
{
    std::string x;
    std::string y;
    std::string z;
};

static std::map<CoordinateSystem, AxisLabels> getLabelDefinitions()
{
    std::map<CoordinateSystem, AxisLabels> mLabels;

    mLabels[CARTESIAN] =    {"\\i x",            "\\i y",              "\\i z"};
    mLabels[POLAR_PZ] =     {"\\varphi  [\\pi]", "\\i z",              "\\rho"};
    mLabels[POLAR_RP] =     {"\\rho",            "\\varphi  [\\pi]",   "\\i z"};
    mLabels[POLAR_RZ] =     {"\\rho",            "\\i z",              "\\varphi  [\\pi]"};
    mLabels[SPHERICAL_PT] = {"\\varphi  [\\pi]", "\\vartheta  [\\pi]", "\\i r"};
    mLabels[SPHERICAL_RP] = {"\\i r",            "\\varphi  [\\pi]",   "\\vartheta  [\\pi]"};
    mLabels[SPHERICAL_RT] = {"\\i r",            "\\vartheta  [\\pi]", "\\varphi  [\\pi]"};

    return mLabels;
}

// --> Lesen der einzelnen Achsenbeschriftungen <--
string PlotData::getAxisLabel(size_t axis) const
{
    if (!bDefaultAxisLabels[axis])
        return replaceToTeX(sAxisLabels[axis]);
    else
    {
        static std::map<CoordinateSystem,AxisLabels> mLabels = getLabelDefinitions();

        if (axis == 0)
            return mLabels[(CoordinateSystem)intSettings[INT_COORDS]].x;
        else if (axis == 1)
            return mLabels[(CoordinateSystem)intSettings[INT_COORDS]].y;
        else
            return mLabels[(CoordinateSystem)intSettings[INT_COORDS]].z;
    }

    return "";
}



















