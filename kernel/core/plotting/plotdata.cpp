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
#include <map>
#include <utility>
#include "plotdata.hpp"
#include "../../kernel.hpp"
#include "../utils/filecheck.hpp"
#define STYLES_COUNT 20

extern mglGraph _fontData;
const char* SECAXIS_DEFAULT_COLOR = "k";

// Function prototype
bool isNotEmptyExpression(StringView sExpr);


/////////////////////////////////////////////////
/// \brief Static helper function to evaluate
/// parameters.
///
/// \param nResults int&
/// \param sExpression std::string
/// \return const mu::StackItem*
///
/////////////////////////////////////////////////
static const mu::StackItem* evaluate(int& nResults, std::string sExpression)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();

    if (_data.containsTables(sExpression))
        getDataElements(sExpression, _parser, _data);

    _parser.SetExpr(sExpression);

    return _parser.Eval(nResults);
}


/////////////////////////////////////////////////
/// \brief Parse an argument value
///
/// \param sCmd const std::string&
/// \param pos size_t
/// \return mu::Array
///
/////////////////////////////////////////////////
static mu::Array parseOpt(const std::string& sCmd, size_t pos)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();

    std::string sVal = getArgAtPos(sCmd, pos, ARGEXTRACT_NONE);

    if (_data.containsTables(sVal))
        getDataElements(sVal, _parser, _data);

    _parser.SetExpr(sVal);

    return _parser.Eval();
}


static mu::Array parseOptFor(const std::string& sCmd, const std::string& sParam)
{
    return parseOpt(sCmd, findParameter(sCmd, sParam, '=') + sParam.length());
}


/////////////////////////////////////////////////
/// \brief Static helper function to evaluate the
/// passed color characters for their validness.
///
/// \param sColorSet const std::string&
/// \return bool
///
/////////////////////////////////////////////////
static bool checkColorChars(const std::string& sColorSet)
{
    static std::string sColorChars = "#| wWhHkRrQqYyEeGgLlCcNnBbUuMmPp123456789{}";

    for (size_t i = 0; i < sColorSet.length(); i++)
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


/////////////////////////////////////////////////
/// \brief Static helper function to evaluate the
/// passed line type characters for their
/// validness.
///
/// \param sLineSet const std::string&
/// \return bool
///
/////////////////////////////////////////////////
static bool checkLineChars(const std::string& sLineSet)
{
    static std::string sLineChars = " -:;ij|=";

    for (size_t i = 0; i < sLineSet.length(); i++)
    {
        if (sLineChars.find(sLineSet[i]) == std::string::npos)
            return false;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Static heloer function to evaluate the
/// passeed point type characters for their
/// validness.
///
/// \param sPointSet const std::string&
/// \return bool
///
/////////////////////////////////////////////////
static bool checkPointChars(const std::string& sPointSet)
{
    static std::string sPointChars = " .*+x#sdo^v<>";

    for (size_t i = 0; i < sPointSet.length(); i++)
    {
        if (sPointChars.find(sPointSet[i]) == std::string::npos)
            return false;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Static helper function to create a map
/// containing the simple logical plot settings.
///
/// \return std::map<std::string,std::pair<PlotData::LogicalPlotSetting,PlotData::ParamType>>
///
/////////////////////////////////////////////////
static std::map<std::string,std::pair<PlotData::LogicalPlotSetting,PlotData::ParamType>> getGenericSwitches()
{
    std::map<std::string,std::pair<PlotData::LogicalPlotSetting,PlotData::ParamType>> mGenericSwitches;

    mGenericSwitches.emplace("box", std::make_pair(PlotData::LOG_BOX, PlotData::GLOBAL));
    mGenericSwitches.emplace("xerrorbars", std::make_pair(PlotData::LOG_XERROR, PlotData::LOCAL));
    mGenericSwitches.emplace("yerrorbars", std::make_pair(PlotData::LOG_YERROR, PlotData::LOCAL));
    mGenericSwitches.emplace("connect", std::make_pair(PlotData::LOG_CONNECTPOINTS, PlotData::LOCAL));
    mGenericSwitches.emplace("points", std::make_pair(PlotData::LOG_DRAWPOINTS, PlotData::LOCAL));
    mGenericSwitches.emplace("interpolate", std::make_pair(PlotData::LOG_INTERPOLATE, PlotData::LOCAL));
    mGenericSwitches.emplace("open", std::make_pair(PlotData::LOG_OPENIMAGE, PlotData::SUPERGLOBAL));
    mGenericSwitches.emplace("silent", std::make_pair(PlotData::LOG_SILENTMODE, PlotData::SUPERGLOBAL));
    mGenericSwitches.emplace("cut", std::make_pair(PlotData::LOG_CUTBOX, PlotData::LOCAL));
    mGenericSwitches.emplace("flength", std::make_pair(PlotData::LOG_FIXEDLENGTH, PlotData::LOCAL));
    mGenericSwitches.emplace("colorbar", std::make_pair(PlotData::LOG_COLORBAR, PlotData::LOCAL));
    mGenericSwitches.emplace("orthoproject", std::make_pair(PlotData::LOG_ORTHOPROJECT, PlotData::GLOBAL));
    mGenericSwitches.emplace("area", std::make_pair(PlotData::LOG_AREA, PlotData::LOCAL));
    mGenericSwitches.emplace("stacked", std::make_pair(PlotData::LOG_STACKEDBARS, PlotData::LOCAL));
    mGenericSwitches.emplace("steps", std::make_pair(PlotData::LOG_STEPPLOT, PlotData::LOCAL));
    mGenericSwitches.emplace("boxplot", std::make_pair(PlotData::LOG_BOXPLOT, PlotData::LOCAL));
    mGenericSwitches.emplace("ohlc", std::make_pair(PlotData::LOG_OHLC, PlotData::LOCAL));
    mGenericSwitches.emplace("candlestick", std::make_pair(PlotData::LOG_CANDLESTICK, PlotData::LOCAL));
    mGenericSwitches.emplace("colormask", std::make_pair(PlotData::LOG_COLORMASK, PlotData::LOCAL));
    mGenericSwitches.emplace("alphamask", std::make_pair(PlotData::LOG_ALPHAMASK, PlotData::LOCAL));
    mGenericSwitches.emplace("schematic", std::make_pair(PlotData::LOG_SCHEMATIC, PlotData::GLOBAL));
    mGenericSwitches.emplace("cloudplot", std::make_pair(PlotData::LOG_CLOUDPLOT, PlotData::LOCAL));
    mGenericSwitches.emplace("region", std::make_pair(PlotData::LOG_REGION, PlotData::LOCAL));
    mGenericSwitches.emplace("crust", std::make_pair(PlotData::LOG_CRUST, PlotData::LOCAL));
    mGenericSwitches.emplace("reconstruct", std::make_pair(PlotData::LOG_CRUST, PlotData::LOCAL));
    mGenericSwitches.emplace("valtab", std::make_pair(PlotData::LOG_TABLE, PlotData::GLOBAL));

    return mGenericSwitches;
}


/////////////////////////////////////////////////
/// \brief Static helper function to create a map
/// containing all color schemes and their
/// corresponding color characters.
///
/// \return std::map<std::string, std::string>
///
/////////////////////////////////////////////////
static std::map<std::string, std::string> getColorSchemes()
{
    std::map<std::string, std::string> mColorSchemes;

    mColorSchemes.emplace("rainbow", "BbcyrR");
    mColorSchemes.emplace("turbo", "{M3}n{c4}{e6}yq{R3}");
    mColorSchemes.emplace("grey", "kw");
    mColorSchemes.emplace("hot", "{R1}{r4}qy");
    mColorSchemes.emplace("cold", "{B2}n{c8}");
    mColorSchemes.emplace("copper", "{Q2}{q3}{q9}");
    mColorSchemes.emplace("map", "UBbcgyqRH");
    mColorSchemes.emplace("moy", "kMqyw");
    mColorSchemes.emplace("coast", "{B6}{c3}{y8}");
    mColorSchemes.emplace("viridis", "{U3}{N4}C{e4}y"); //UNC{e4}y
    mColorSchemes.emplace("std", "{U3}{N4}C{e4}y");
    mColorSchemes.emplace("plasma", "B{u4}p{q6}{y7}");
    mColorSchemes.emplace("hue", "rygcbmr");
    mColorSchemes.emplace("polarity", "w{n5}{u2}{r7}w");
    mColorSchemes.emplace("complex", "w{n5}{u2}{r7}w");
    mColorSchemes.emplace("spectral", "{R6}{q6}{y8}{l6}{N6}");
    mColorSchemes.emplace("coolwarm", "{n6}{r7}");
    mColorSchemes.emplace("ryg", "{R6}{y7}{G6}");

    return mColorSchemes;
}







/////////////////////////////////////////////////
/// \brief PlotData constructor. Calls
/// PlotData::reset() for initialisation.
/////////////////////////////////////////////////
PlotData::PlotData() : FileSystem()
{
    PlotData::reset();
}


/////////////////////////////////////////////////
/// \brief Identifies parameters and values in
/// the passed parameter string and updates the
/// selected type of the parameters
/// correspondingly.
///
/// \param sCmd const string&
/// \param nType int
/// \return void
///
/////////////////////////////////////////////////
void PlotData::setParams(const std::string& sCmd, int nType)
{
    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();
    static std::map<std::string,std::pair<PlotData::LogicalPlotSetting,PlotData::ParamType>> mGenericSwitches = getGenericSwitches();
    static std::map<std::string,std::string> mColorSchemes = getColorSchemes();

    if (findParameter(sCmd, "reset") && (nType == ALL || nType & SUPERGLOBAL))
        reset();

    // Handle generic switches first
    for (const auto& iter : mGenericSwitches)
    {
        if (findParameter(sCmd, iter.first) && (nType == ALL || nType & iter.second.second))
            logicalSettings[iter.second.first] = true;
        else if (findParameter(sCmd, "no" + iter.first) && (nType == ALL || nType & iter.second.second))
            logicalSettings[iter.second.first] = false;
    }

    if (findParameter(sCmd, "grid") && (nType == ALL || nType & GLOBAL))
        intSettings[INT_GRID] = 1;

    if (findParameter(sCmd, "grid", '=') && (nType == ALL || nType & GLOBAL))
    {
        size_t nPos = findParameter(sCmd, "grid", '=')+4;

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
            floatSettings[FLOAT_ALPHAVAL] = 1 - parseOptFor(sCmd, "alpha").front().getNum().asF64();

            if (floatSettings[FLOAT_ALPHAVAL] < 0 || floatSettings[FLOAT_ALPHAVAL] > 1)
                floatSettings[FLOAT_ALPHAVAL] = 0.5;
        }

        if (findParameter(sCmd, "transparency", '='))
        {
            floatSettings[FLOAT_ALPHAVAL] = 1 -  parseOptFor(sCmd, "transparency").front().getNum().asF64();

            if (floatSettings[FLOAT_ALPHAVAL] < 0 || floatSettings[FLOAT_ALPHAVAL] > 1)
                floatSettings[FLOAT_ALPHAVAL] = 0.5;
        }
    }

    if ((findParameter(sCmd, "noalpha") || findParameter(sCmd, "notransparency")) && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_ALPHA] = false;

    if (findParameter(sCmd, "axis") && (nType == ALL || nType & GLOBAL))
    {
        intSettings[INT_AXIS] = AXIS_STD;

        if (findParameter(sCmd, "axis", '='))
        {
            if (getArgAtPos(sCmd, findParameter(sCmd, "axis", '=')+4) == "nice")
                intSettings[INT_AXIS] = AXIS_NICE;
            else if (getArgAtPos(sCmd, findParameter(sCmd, "axis", '=')+4) == "equal")
                intSettings[INT_AXIS] = AXIS_EQUAL;
        }
    }

    if (findParameter(sCmd, "noaxis") && (nType == ALL || nType & GLOBAL))
        intSettings[INT_AXIS] = AXIS_NONE;

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

    if (findParameter(sCmd, "lcont") && (nType == ALL || nType & LOCAL))
    {
        logicalSettings[LOG_CONTLABELS] = true;

        if (findParameter(sCmd, "lcont", '='))
            intSettings[INT_CONTLINES] = parseOptFor(sCmd, "lcont").getAsScalarInt();
    }

    if (findParameter(sCmd, "nolcont") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_CONTLABELS] = false;

    if (findParameter(sCmd, "pcont") && (nType == ALL || nType & LOCAL))
    {
        logicalSettings[LOG_CONTPROJ] = true;

        if (findParameter(sCmd, "pcont", '='))
            intSettings[INT_CONTLINES] = parseOptFor(sCmd, "pcont").getAsScalarInt();
    }

    if (findParameter(sCmd, "nopcont") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_CONTPROJ] = false;

    if (findParameter(sCmd, "fcont") && (nType == ALL || nType & LOCAL))
    {
        logicalSettings[LOG_CONTFILLED] = true;

        if (findParameter(sCmd, "fcont", '='))
            intSettings[INT_CONTLINES] = parseOptFor(sCmd, "fcont").getAsScalarInt();
    }

    if (findParameter(sCmd, "nofcont") && (nType == ALL || nType & LOCAL))
        logicalSettings[LOG_CONTFILLED] = false;

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
        for (int i = XRANGE; i <= CRANGE; i++)
        {
            bLogscale[i] = true;
        }
    }

    if (findParameter(sCmd, "nologscale") && (nType == ALL || nType & GLOBAL))
    {
        for (int i = XRANGE; i <= CRANGE; i++)
        {
            bLogscale[i] = false;
        }
    }

    if (findParameter(sCmd, "xlog") && (nType == ALL || nType & GLOBAL))
        bLogscale[XRANGE] = true;

    if (findParameter(sCmd, "ylog") && (nType == ALL || nType & GLOBAL))
        bLogscale[YRANGE] = true;

    if (findParameter(sCmd, "zlog") && (nType == ALL || nType & GLOBAL))
        bLogscale[ZRANGE] = true;

    if (findParameter(sCmd, "clog") && (nType == ALL || nType & GLOBAL))
        bLogscale[CRANGE] = true;

    if (findParameter(sCmd, "noxlog") && (nType == ALL || nType & GLOBAL))
        bLogscale[XRANGE] = false;

    if (findParameter(sCmd, "noylog") && (nType == ALL || nType & GLOBAL))
        bLogscale[YRANGE] = false;

    if (findParameter(sCmd, "nozlog") && (nType == ALL || nType & GLOBAL))
        bLogscale[ZRANGE] = false;

    if (findParameter(sCmd, "noclog") && (nType == ALL || nType & GLOBAL))
        bLogscale[CRANGE] = false;

    if (findParameter(sCmd, "samples", '=') && (nType == ALL || nType & LOCAL))
    {
        mu::Array res = parseOptFor(sCmd, "samples");

        if (isnan(res.front().getNum().asF64()) || isinf(res.front().getNum().asF64()))
            intSettings[INT_SAMPLES] = 100;
        else
            intSettings[INT_SAMPLES] = res.getAsScalarInt();
    }

    if (findParameter(sCmd, "t", '=') && (nType == ALL || nType & LOCAL))
    {
        int nPos = findParameter(sCmd, "t", '=')+1;
        std::string sTemp_1 = getArgAtPos(sCmd, nPos);
        ranges[TRANGE].reset(sTemp_1);
    }

    if (findParameter(sCmd, "colorrange", '=') && (nType == ALL || nType & GLOBAL))
    {
        size_t nPos = findParameter(sCmd, "colorrange", '=') + 10;
        std::string sTemp_1 = getArgAtPos(sCmd, nPos);
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
        std::string sTemp = getArgAtPos(sCmd, nPos);
        if (sTemp.find(",") != std::string::npos && sTemp.length() > 1)
        {
            if (sTemp.find(',') && sTemp.find(',') != sTemp.length()-1)
            {
                int nResults;
                const mu::StackItem* dTemp = evaluate(nResults, sTemp);
                dRotateAngles[0] = dTemp[0].get().front().getNum().asF64();
                dRotateAngles[1] = dTemp[1].get().front().getNum().asF64();
            }
            else if (!sTemp.find(','))
            {
                _parser.SetExpr(sTemp.substr(1));
                dRotateAngles[1] = _parser.Eval().front().getNum().asF64();
            }
            else if (sTemp.find(',') == sTemp.length()-1)
            {
                _parser.SetExpr(sTemp.substr(0,sTemp.length()-1));
                dRotateAngles[0] = _parser.Eval().front().getNum().asF64();
            }

            for (size_t i = 0; i < 2; i++)
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
        std::string sTemp = getArgAtPos(sCmd, nPos);
        if (sTemp.find(',') != std::string::npos && sTemp.length() > 1)
        {
            int nResults = 0;
            const mu::StackItem* dTemp = evaluate(nResults, sTemp);
            if (nResults)
            {
                for (size_t i = 0; i < 3; i++)
                {
                    if (i < dTemp[0].get().size() && !isnan(dTemp[0].get()[i].getNum().asF64()) && !isinf(dTemp[0].get()[i].getNum().asF64()))
                        dOrigin[i] = dTemp[0].get()[i].getNum().asF64();
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
        std::string sTemp = getArgAtPos(sCmd, nPos);
        if (sTemp.find(',') != std::string::npos && sTemp.length() > 1)
        {
            int nResults;
            const mu::StackItem* dTemp = evaluate(nResults, sTemp);

            if (nResults)
            {
                const mu::Array& val = dTemp[0].get();
                for (size_t i = 0; i < 3; i++)
                {
                    if (i < val.size()
                        && !isnan(val[i].getNum().asF64())
                        && !isinf(val[i].getNum().asF64())
                        && val[i].getNum().asF64() <= 5
                        && val[i].getNum().asF64() >= 0)
                        nSlices[i] = val[i].getNum().asI64();
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
        mu::Array to = parseOptFor(sCmd, "streamto");

        if (to.size() > 1)
        {
            nTargetGUI[0] = to[0].getNum().asI64();
            nTargetGUI[1] = to[1].getNum().asI64();
        }
    }

    if (findParameter(sCmd, "size", '=') && (nType == ALL || nType & SUPERGLOBAL))
    {
        mu::Array s = parseOptFor(sCmd, "size");

        if (s.size() > 1)
        {
            intSettings[INT_SIZE_X] = s[0].getNum().asI64();
            intSettings[INT_SIZE_Y] = s[1].getNum().asI64();

            if (intSettings[INT_SIZE_X] > 0 && intSettings[INT_SIZE_Y] > 0)
                floatSettings[FLOAT_ASPECT] = intSettings[INT_SIZE_X] / (double)intSettings[INT_SIZE_Y];
            else
            {
                intSettings[INT_SIZE_X] = 0;
                intSettings[INT_SIZE_Y] = 0;
            }
        }
    }

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

    if (findParameter(sCmd, "complexmode", '=') && (nType == ALL || nType & SUPERGLOBAL))
    {
        int nPos = findParameter(sCmd, "complexmode", '=')+11;

        if (getArgAtPos(sCmd, nPos) == "reim")
            intSettings[INT_COMPLEXMODE] = CPLX_REIM;
        else if (getArgAtPos(sCmd, nPos) == "plane")
            intSettings[INT_COMPLEXMODE] = CPLX_PLANE;
        else
            intSettings[INT_COMPLEXMODE] = CPLX_NONE;
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
        mu::Array samples = parseOptFor(sCmd, "animate");

        intSettings[INT_ANIMATESAMPLES] = samples.getAsScalarInt();

        if (intSettings[INT_ANIMATESAMPLES]
            && !mu::isinf(samples.front().getNum().asCF64())
            && !mu::isnan(samples.front().getNum()))
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
        mu::Array s = parseOptFor(sCmd, "marks");
        intSettings[INT_MARKS] = s.getAsScalarInt();

        if (!intSettings[INT_MARKS]
            || isinf(s.front().getNum().asF64())
            || isnan(s.front().getNum().asF64()))
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
        floatSettings[FLOAT_TEXTSIZE] = parseOptFor(sCmd, "textsize").front().getNum().asF64();

        if (isinf(floatSettings[FLOAT_TEXTSIZE]) || isnan(floatSettings[FLOAT_TEXTSIZE]))
            floatSettings[FLOAT_TEXTSIZE] = 5;

        if (floatSettings[FLOAT_TEXTSIZE] <= -1)
            floatSettings[FLOAT_TEXTSIZE] = 5;
    }

    if (findParameter(sCmd, "aspect", '=') && (nType == ALL || nType & SUPERGLOBAL))
    {
        floatSettings[FLOAT_ASPECT] = parseOptFor(sCmd, "aspect").front().getNum().asF64();

        if (floatSettings[FLOAT_ASPECT] <= 0
            || isnan(floatSettings[FLOAT_ASPECT])
            || isinf(floatSettings[FLOAT_ASPECT]))
            floatSettings[FLOAT_ASPECT] = 4/3;
    }

    if (findParameter(sCmd, "noanimate") && (nType == ALL || nType & SUPERGLOBAL))
        logicalSettings[LOG_ANIMATE] = false;

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

    if (findParameter(sCmd, "bars") && (nType == ALL || nType & LOCAL))
    {
        floatSettings[FLOAT_BARS] = 0.9;
        floatSettings[FLOAT_HBARS] = 0.0;
    }

    if (findParameter(sCmd, "bars", '=') && (nType == ALL || nType & LOCAL))
    {
        mu::Array bars = parseOptFor(sCmd, "bars");

        floatSettings[FLOAT_BARS] = bars.front().getNum().asF64();

        if (floatSettings[FLOAT_BARS]
            && !isinf(bars.front().getNum().asF64())
            && !isnan(bars.front().getNum().asF64())
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
        mu::Array hbars = parseOptFor(sCmd, "hbars");
        floatSettings[FLOAT_HBARS] = hbars.front().getNum().asF64();

        if (floatSettings[FLOAT_HBARS]
            && !isinf(hbars.front().getNum().asF64())
            && !isnan(hbars.front().getNum().asF64())
            && (floatSettings[FLOAT_HBARS] < 0.0 || floatSettings[FLOAT_HBARS] > 1.0))
            floatSettings[FLOAT_HBARS] = 0.9;

        floatSettings[FLOAT_BARS] = 0.0;
    }

    if ((findParameter(sCmd, "nobars") || findParameter(sCmd, "nohbars")) && (nType == ALL || nType & LOCAL))
    {
        floatSettings[FLOAT_BARS] = 0.0;
        floatSettings[FLOAT_HBARS] = 0.0;
    }

    if (findParameter(sCmd, "perspective", '=') && (nType == ALL || nType & GLOBAL))
    {
        floatSettings[FLOAT_PERSPECTIVE] = fabs(parseOptFor(sCmd, "perspective").front().getNum().asF64());

        if (floatSettings[FLOAT_PERSPECTIVE] >= 1.0)
            floatSettings[FLOAT_PERSPECTIVE] = 0.0;
    }

    if (findParameter(sCmd, "noperspective") && (nType == ALL || nType & GLOBAL))
        floatSettings[FLOAT_PERSPECTIVE] = 0.0;

    if (findParameter(sCmd, "maxline", '=') && (nType == ALL || nType & LOCAL))
    {
        std::string sTemp = getArgAtPos(sCmd, findParameter(sCmd, "maxline", '=')+7);

        if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
            sTemp = sTemp.substr(1,sTemp.length()-2);

        _lHlines[0].sDesc = parseOpt(getNextArgument(sTemp, true),0).printVals();

        if (sTemp.length())
            _lHlines[0].sStyle = parseOpt(getNextArgument(sTemp, true),0).printVals();

        replaceControlChars(_lHlines[0].sDesc);
    }

    if (findParameter(sCmd, "minline", '=') && (nType == ALL || nType & LOCAL))
    {
        std::string sTemp = getArgAtPos(sCmd, findParameter(sCmd, "minline", '=')+7);

        if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
            sTemp = sTemp.substr(1,sTemp.length()-2);

        _lHlines[1].sDesc = parseOpt(getNextArgument(sTemp, true),0).printVals();

        if (sTemp.length())
            _lHlines[1].sStyle = parseOpt(getNextArgument(sTemp, true),0).printVals();

        replaceControlChars(_lHlines[1].sDesc);
    }

    if ((findParameter(sCmd, "hline", '=') || findParameter(sCmd, "hlines", '=')) && (nType == ALL || nType & LOCAL))
    {
        std::string sTemp;

        if (findParameter(sCmd, "hline", '='))
            sTemp = getArgAtPos(sCmd, findParameter(sCmd, "hline", '=')+5);
        else
            sTemp = getArgAtPos(sCmd, findParameter(sCmd, "hlines", '=')+6);

        if (sTemp.find(',') != std::string::npos)
        {
            if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
                sTemp = sTemp.substr(1,sTemp.length()-2);

            const mu::StackItem* v = nullptr;
            int nResults = 0;
            v = evaluate(nResults, sTemp);

            for (size_t i = 0; i < v[0].get().size(); i++)
            {
                if (i)
                    _lHlines.push_back(Line());

                _lHlines[i+2].dPos = v[0].get()[i].getNum().asF64();
            }

            if (nResults > 1)
            {
                for (size_t i = 2; i < _lHlines.size(); i++)
                {
                    if (i-2 >= v[1].get().size())
                        break;

                    _lHlines[i].sDesc = v[1].get()[i-2].getStr();
                    replaceControlChars(_lHlines[i].sDesc);
                }

                if (nResults > 2)
                {
                    for (size_t i = 2; i < _lHlines.size(); i++)
                    {
                        if (i-2 >= v[2].get().size())
                            break;

                        _lHlines[i].sStyle = v[2].get()[i-2].getStr();
                    }
                }
            }
        }
    }

    if ((findParameter(sCmd, "vline", '=') || findParameter(sCmd, "vlines", '=')) && (nType == ALL || nType & LOCAL))
    {
        std::string sTemp;

        if (findParameter(sCmd, "vline", '='))
            sTemp = getArgAtPos(sCmd, findParameter(sCmd, "vline", '=')+5);
        else
            sTemp = getArgAtPos(sCmd, findParameter(sCmd, "vlines", '=')+6);

        if (sTemp.find(',') != std::string::npos)
        {
            if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
                sTemp = sTemp.substr(1,sTemp.length()-2);

            const mu::StackItem* v = nullptr;
            int nResults = 0;
            v = evaluate(nResults, sTemp);

            for (size_t i = 0; i < v[0].get().size(); i++)
            {
                if (i)
                    _lVLines.push_back(Line());

                _lVLines[i+2].dPos = v[0].get()[i].getNum().asF64();
            }

            if (nResults > 1)
            {
                for (size_t i = 2; i < _lVLines.size(); i++)
                {
                    if (i-2 >= v[1].get().size())
                        break;

                    _lVLines[i].sDesc = v[1].get()[i-2].getStr();
                    replaceControlChars(_lVLines[i].sDesc);
                }

                if (nResults > 2)
                {
                    for (size_t i = 2; i < _lVLines.size(); i++)
                    {
                        if (i-2 >= v[2].get().size())
                            break;

                        _lVLines[i].sStyle = v[2].get()[i-2].getStr();
                    }
                }
            }
        }
    }

    if (findParameter(sCmd, "timeaxes", '=') && (nType == ALL || nType & GLOBAL))
    {
        std::string sTemp;
        sTemp = getArgAtPos(sCmd, findParameter(sCmd, "timeaxes", '=')+8);

        if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
            sTemp = sTemp.substr(1,sTemp.length()-2);

        const mu::StackItem* v = nullptr;
        int nResults = 0;
        v = evaluate(nResults, sTemp);

        for (size_t i = 0; i < v[0].get().size(); i++)
        {
            std::string sAxis = v[0].get()[i].getStr();

            if (sAxis == "c")
                _timeAxes[3].activate(nResults > 1 ? v[1].get().get(i).getStr() : "");
            else if (sAxis.find_first_of("xyz") != std::string::npos)
                _timeAxes[sAxis[0]-'x'].activate(nResults > 1 ? v[1].get().get(i).getStr() : "");
        }
    }

    if (findParameter(sCmd, "lborder", '=') && (nType == ALL || nType & LOCAL))
    {
        std::string sTemp = getArgAtPos(sCmd, findParameter(sCmd, "lborder", '=')+7);

        if (sTemp.find(',') != std::string::npos)
        {
            if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
                sTemp = sTemp.substr(1,sTemp.length()-2);

            const mu::StackItem* v = nullptr;
            int nResults = 0;
            v = evaluate(nResults, sTemp);
            _lVLines[0].dPos = v[0].get().front().getNum().asF64();

            if (nResults > 1)
            {
                _lVLines[0].sDesc = v[1].get().printVals();

                if (nResults > 2)
                    _lVLines[0].sStyle = v[2].get().printVals();
            }
        }

        replaceControlChars(_lVLines[0].sDesc);
    }

    if (findParameter(sCmd, "rborder", '=') && (nType == ALL || nType & LOCAL))
    {
        std::string sTemp = getArgAtPos(sCmd, findParameter(sCmd, "rborder", '=')+7);

        if (sTemp.find(',') != std::string::npos)
        {
            if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
                sTemp = sTemp.substr(1,sTemp.length()-2);

            const mu::StackItem* v = nullptr;
            int nResults = 0;
            v = evaluate(nResults, sTemp);
            _lVLines[0].dPos = v[0].get().front().getNum().asF64();

            if (nResults > 1)
            {
                _lVLines[0].sDesc = v[1].get().printVals();

                if (nResults > 2)
                    _lVLines[0].sStyle = v[2].get().printVals();
            }
        }

        replaceControlChars(_lVLines[1].sDesc);
    }

    if (findParameter(sCmd, "addxaxis", '=') && (nType == ALL || nType & GLOBAL))
    {
        std::string sTemp = getArgAtPos(sCmd, findParameter(sCmd, "addxaxis", '=')+8);

        if (sTemp.find(',') != std::string::npos || sTemp.find('"') != std::string::npos)
        {
            if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
                sTemp = sTemp.substr(1,sTemp.length()-2);

            const mu::StackItem* v = nullptr;
            int nResults = 0;
            v = evaluate(nResults, sTemp);

            if (nResults > 1 && v[0].get().getCommonType() == mu::TYPE_NUMERICAL && v[1].get().getCommonType() == mu::TYPE_NUMERICAL)
            {
                _AddAxes[XCOORD].ivl.reset(v[0].get().get(0).getNum().asCF64(),
                                           v[1].get().get(0).getNum().asCF64());

                if (nResults > 2)
                {
                    _AddAxes[XCOORD].sLabel = v[2].get().printVals();

                    if (nResults > 3)
                    {
                        _AddAxes[XCOORD].sStyle = v[3].get().printVals();

                        if (!checkColorChars(_AddAxes[XCOORD].sStyle))
                            _AddAxes[XCOORD].sStyle = SECAXIS_DEFAULT_COLOR;
                    }
                }
                else
                    _AddAxes[XCOORD].sLabel = "\\i x";
            }
            else
            {
                _AddAxes[XCOORD].sLabel = v[0].get().printVals();

                if (nResults > 1)
                {
                    _AddAxes[XCOORD].sStyle = v[1].get().printVals();

                    if (!checkColorChars(_AddAxes[XCOORD].sStyle))
                        _AddAxes[XCOORD].sStyle = SECAXIS_DEFAULT_COLOR;
                }
            }
        }
    }

    if (findParameter(sCmd, "addyaxis", '=') && (nType == ALL || nType & GLOBAL))
    {
        std::string sTemp = getArgAtPos(sCmd, findParameter(sCmd, "addyaxis", '=')+8);

        if (sTemp.find(',') != std::string::npos || sTemp.find('"') != std::string::npos)
        {
            if (sTemp[0] == '(' && sTemp[sTemp.length()-1] == ')')
                sTemp = sTemp.substr(1,sTemp.length()-2);

            const mu::StackItem* v = nullptr;
            int nResults = 0;
            v = evaluate(nResults, sTemp);

            if (nResults > 1 && v[0].get().getCommonType() == mu::TYPE_NUMERICAL && v[1].get().getCommonType() == mu::TYPE_NUMERICAL)
            {
                _AddAxes[YCOORD].ivl.reset(v[0].get().get(0).getNum().asCF64(),
                                           v[1].get().get(0).getNum().asCF64());

                if (nResults > 2)
                {
                    _AddAxes[YCOORD].sLabel = v[2].get().printVals();

                    if (nResults > 3)
                    {
                        _AddAxes[YCOORD].sStyle = v[3].get().printVals();

                        if (!checkColorChars(_AddAxes[YCOORD].sStyle))
                            _AddAxes[YCOORD].sStyle = SECAXIS_DEFAULT_COLOR;
                    }
                }
                else
                    _AddAxes[YCOORD].sLabel = "\\i y";
            }
            else
            {
                _AddAxes[YCOORD].sLabel = v[0].get().printVals();

                if (nResults > 1)
                {
                    _AddAxes[YCOORD].sStyle = v[1].get().printVals();

                    if (!checkColorChars(_AddAxes[YCOORD].sStyle))
                        _AddAxes[YCOORD].sStyle = SECAXIS_DEFAULT_COLOR;
                }
            }
        }
    }

    if (findParameter(sCmd, "colorscheme", '=') && (nType == ALL || nType & LOCAL))
    {
        size_t nPos = findParameter(sCmd, "colorscheme", '=') + 11;
        std::string sTemp = getArgAtPos(sCmd, nPos, ARGEXTRACT_NONE);

        if (sTemp.front() == '"')
        {
            std::string __sColorScheme = parseOpt(sCmd, nPos).printVals();

            if (!checkColorChars(__sColorScheme))
                stringSettings[STR_COLORSCHEME] = mColorSchemes["std"];
            else
            {
                if (__sColorScheme == "#" && stringSettings[STR_COLORSCHEME].find('#') == std::string::npos)
                    stringSettings[STR_COLORSCHEME] += '#';
                else if (__sColorScheme == "|" && stringSettings[STR_COLORSCHEME].find('|') == std::string::npos)
                    stringSettings[STR_COLORSCHEME] += '|';
                else if ((__sColorScheme == "#|" || __sColorScheme == "|#") && (stringSettings[STR_COLORSCHEME].find('#') == std::string::npos || stringSettings[STR_COLORSCHEME].find('|') == std::string::npos))
                {
                    if (stringSettings[STR_COLORSCHEME].find('#') == std::string::npos && stringSettings[STR_COLORSCHEME].find('|') != std::string::npos)
                        stringSettings[STR_COLORSCHEME] += '#';
                    else if (stringSettings[STR_COLORSCHEME].find('|') == std::string::npos && stringSettings[STR_COLORSCHEME].find('#') != std::string::npos)
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
            auto iter = mColorSchemes.find(sTemp);

            if (iter != mColorSchemes.end())
                stringSettings[STR_COLORSCHEME] = iter->second;
            else
                stringSettings[STR_COLORSCHEME] = mColorSchemes["std"];
        }

        if (stringSettings[STR_COLORSCHEME].length() > 32)
            stringSettings[STR_COLORSCHEME] = mColorSchemes["std"];

        while (stringSettings[STR_COLORSCHEME].find(' ') != std::string::npos)
        {
            stringSettings[STR_COLORSCHEME].erase(stringSettings[STR_COLORSCHEME].find(' '),1);
        }

        stringSettings[STR_COLORSCHEMELIGHT] = "";
        stringSettings[STR_COLORSCHEMEMEDIUM] = "";

        for (size_t i = 0; i < stringSettings[STR_COLORSCHEME].length(); i++)
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

    if (findParameter(sCmd, "bgcolorscheme", '=') && (nType == ALL || nType & LOCAL))
    {
        size_t nPos = findParameter(sCmd, "bgcolorscheme", '=') + 13;
        std::string sTemp = getArgAtPos(sCmd, nPos, ARGEXTRACT_NONE);

        if (sTemp.front() == '"')
        {
            std::string __sBGColorScheme = parseOpt(sCmd, nPos).printVals();
            StripSpaces(__sBGColorScheme);

            if (!checkColorChars(__sBGColorScheme))
                stringSettings[STR_BACKGROUNDCOLORSCHEME] = mColorSchemes["std"];
            else
            {
                if (__sBGColorScheme == "#"
                    && stringSettings[STR_BACKGROUNDCOLORSCHEME].find('#') == std::string::npos)
                    stringSettings[STR_BACKGROUNDCOLORSCHEME] += '#';
                else if (__sBGColorScheme == "|"
                         && stringSettings[STR_BACKGROUNDCOLORSCHEME].find('|') == std::string::npos)
                    stringSettings[STR_BACKGROUNDCOLORSCHEME] += '|';
                else if ((__sBGColorScheme == "#|" || __sBGColorScheme == "|#")
                         && (stringSettings[STR_BACKGROUNDCOLORSCHEME].find('#') == std::string::npos
                             || stringSettings[STR_BACKGROUNDCOLORSCHEME].find('|') == std::string::npos))
                {
                    if (stringSettings[STR_BACKGROUNDCOLORSCHEME].find('#') == std::string::npos
                        && stringSettings[STR_BACKGROUNDCOLORSCHEME].find('|') != std::string::npos)
                        stringSettings[STR_BACKGROUNDCOLORSCHEME] += '#';
                    else if (stringSettings[STR_BACKGROUNDCOLORSCHEME].find('|') == std::string::npos
                             && stringSettings[STR_BACKGROUNDCOLORSCHEME].find('#') != std::string::npos)
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
            StripSpaces(sTemp);

            auto iter = mColorSchemes.find(sTemp);

            if (iter != mColorSchemes.end())
                stringSettings[STR_BACKGROUNDCOLORSCHEME] = iter->second;
            else if (sTemp == "real")
                stringSettings[STR_BACKGROUNDCOLORSCHEME] = "<<REALISTIC>>";
            else
                stringSettings[STR_BACKGROUNDCOLORSCHEME] = mColorSchemes["std"];
        }

        if (stringSettings[STR_BACKGROUNDCOLORSCHEME].length() > 32)
            stringSettings[STR_BACKGROUNDCOLORSCHEME] = mColorSchemes["std"];

        while (stringSettings[STR_BACKGROUNDCOLORSCHEME].find(' ') != std::string::npos)
        {
            stringSettings[STR_BACKGROUNDCOLORSCHEME] = stringSettings[STR_BACKGROUNDCOLORSCHEME].substr(0, stringSettings[STR_BACKGROUNDCOLORSCHEME].find(' ')) + stringSettings[STR_BACKGROUNDCOLORSCHEME].substr(stringSettings[STR_BACKGROUNDCOLORSCHEME].find(' ')+1);
        }
    }

    if (findParameter(sCmd, "plotcolors", '=') && (nType == ALL || nType & LOCAL))
    {
        size_t nPos = findParameter(sCmd, "plotcolors", '=')+10;
        std::string sTemp = parseOpt(sCmd, nPos).printVals();
        if (checkColorChars(sTemp))
        {
            for (size_t i = 0; i < sTemp.length(); i++)
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
        size_t nPos = findParameter(sCmd, "axisbind", '=')+8;
        std::string sTemp = parseOpt(sCmd, nPos).printVals();
        for (size_t i = 0; i < sTemp.length(); i++)
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
        if (stringSettings[STR_AXISBIND].find('l') == std::string::npos && stringSettings[STR_AXISBIND].length())
        {
            for (size_t i = 0; i < stringSettings[STR_AXISBIND].length(); i++)
            {
                if (stringSettings[STR_AXISBIND][i] == 'r')
                    stringSettings[STR_AXISBIND][i] = 'l';
            }
        }
        if (stringSettings[STR_AXISBIND].find('b') == std::string::npos && stringSettings[STR_AXISBIND].length())
        {
            for (size_t i = 0; i < stringSettings[STR_AXISBIND].length(); i++)
            {
                if (stringSettings[STR_AXISBIND][i] == 't')
                    stringSettings[STR_AXISBIND][i] = 'b';
            }
        }
    }

    if (findParameter(sCmd, "linestyles", '=') && (nType == ALL || nType & LOCAL))
    {
        std::string sTemp = parseOptFor(sCmd, "linestyles").printVals();

        if (checkLineChars(sTemp))
        {
            for (size_t i = 0; i < sTemp.length(); i++)
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
        std::string sTemp = parseOptFor(sCmd, "linesizes").printVals();

        for (size_t i = 0; i < sTemp.length(); i++)
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
        std::string sTemp = parseOptFor(sCmd, "pointstyles").printVals();

        if (checkPointChars(sTemp))
        {
            int nChar = 0;
            std::string sChar = "";
            for (size_t i = 0; i < sTemp.length(); i++)
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
        std::string sTemp = parseOptFor(sCmd, "styles").printVals();

        size_t nJump = 0;
        size_t nStyle = 0;

        for (size_t i = 0; i < sTemp.length(); i += 4)
        {
            nJump = 0;
            if (nStyle >= STYLES_COUNT)
                break;
            if (sTemp.substr(i,4).find('#') != std::string::npos)
                nJump = 1;
            for (size_t j = 0; j < 4+nJump; j++)
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
        std::string sTemp = parseOptFor(sCmd, "gridstyle").printVals();

        for (size_t i = 0; i < sTemp.length(); i += 3)
        {
            for (size_t j = 0; j < 3; j++)
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
        std::string sCoords = getArgAtPos(sCmd, nPos);

        if (sCoords.find('-') != std::string::npos)
            sCoords.erase(sCoords.find('-'));

        if (sCoords == "cartesian" || sCoords == "std")
            intSettings[INT_COORDS] = CARTESIAN;
        else if (sCoords == "polar" || sCoords == "polar_pz" || sCoords == "cylindrical")
            intSettings[INT_COORDS] = POLAR_PZ;
        else if (sCoords == "polar_rp")
            intSettings[INT_COORDS] = POLAR_RP;
        else if (sCoords == "polar_rz")
            intSettings[INT_COORDS] = POLAR_RZ;
        else if (sCoords == "spherical" || sCoords == "spherical_pt")
            intSettings[INT_COORDS] = SPHERICAL_PT;
        else if (sCoords == "spherical_rp")
            intSettings[INT_COORDS] = SPHERICAL_RP;
        else if (sCoords == "spherical_rt")
            intSettings[INT_COORDS] = SPHERICAL_RT;
    }

    if (findParameter(sCmd, "coords", '=') && (nType == ALL || nType & LOCAL))
    {
        int nPos = findParameter(sCmd, "coords", '=')+6;
        std::string sCoords = getArgAtPos(sCmd, nPos);

        if (sCoords.find('-') != std::string::npos)
            sCoords.erase(0, sCoords.find('-')+1);

        if (sCoords == "parametric")
            logicalSettings[LOG_PARAMETRIC] = true;
    }

    if (findParameter(sCmd, "font", '=') && (nType == ALL || nType & SUPERGLOBAL))
    {
        std::string sTemp = getArgAtPos(sCmd, findParameter(sCmd, "font", '=')+4);
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
        size_t nPos = 0;

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

        stringSettings[STR_FILENAME] = getArgAtPos(sCmd, nPos, ARGEXTRACT_NONE);
        StripSpaces(stringSettings[STR_FILENAME]);

        // It is mostly possible to supply a file path without being enclosed
        // in quotation marks. is_dir checks for that
        if (!is_dir(stringSettings[STR_FILENAME]))
        {
            // String evaluation
            mu::Parser& _parser = NumeReKernel::getInstance()->getParser();
            _parser.SetExpr(stringSettings[STR_FILENAME]);
            mu::Array v = _parser.Eval();
            stringSettings[STR_FILENAME] = v.front().getStr();
        }

        if (stringSettings[STR_FILENAME].length())
        {
            std::string sExtension = "";

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
            else if ((findParameter(sCmd, "export", '=') || findParameter(sCmd, "save", '='))
                     && stringSettings[STR_FILENAME].rfind('.') == std::string::npos)
                stringSettings[STR_FILENAME] += ".png";

            stringSettings[STR_FILENAME] = FileSystem::ValidizeAndPrepareName(stringSettings[STR_FILENAME],
                                                                              stringSettings[STR_FILENAME].substr(stringSettings[STR_FILENAME].rfind('.')));
        }
    }

    if (findParameter(sCmd, "xlabel", '=') && (nType == ALL || nType & GLOBAL))
    {
        sAxisLabels[XCOORD] = parseOptFor(sCmd, "xlabel").printVals();
        bDefaultAxisLabels[XCOORD] = false;
    }

    if (findParameter(sCmd, "ylabel", '=') && (nType == ALL || nType & GLOBAL))
    {
        sAxisLabels[YCOORD] = parseOptFor(sCmd, "ylabel").printVals();
        bDefaultAxisLabels[YCOORD] = false;
    }

    if (findParameter(sCmd, "zlabel", '=') && (nType == ALL || nType & GLOBAL))
    {
        sAxisLabels[ZCOORD] = parseOptFor(sCmd, "zlabel").printVals();
        bDefaultAxisLabels[ZCOORD] = false;
    }

    if (findParameter(sCmd, "margin", '=') && (nType == ALL || nType & GLOBAL))
    {
        stringSettings[STR_PLOTBOUNDARIES] = parseOptFor(sCmd, "margin").printVals();
        StripSpaces(stringSettings[STR_PLOTBOUNDARIES]);
    }

    if ((findParameter(sCmd, "title", '=')
        || findParameter(sCmd, "background", '=')) && (nType == ALL || nType & GLOBAL))
    {
        if (findParameter(sCmd, "title", '='))
        {
            stringSettings[STR_PLOTTITLE] = parseOptFor(sCmd, "title").printVals();
            StripSpaces(stringSettings[STR_PLOTTITLE]);

            if (stringSettings[STR_COMPOSEDTITLE].length())
                stringSettings[STR_COMPOSEDTITLE] += ", " + stringSettings[STR_PLOTTITLE];
            else
                stringSettings[STR_COMPOSEDTITLE] = stringSettings[STR_PLOTTITLE];
        }

        if (findParameter(sCmd, "background", '='))
        {
            stringSettings[STR_BACKGROUND] = parseOptFor(sCmd, "background").printVals();
            StripSpaces(stringSettings[STR_BACKGROUND]);

            if (stringSettings[STR_BACKGROUND].length())
            {
                if (stringSettings[STR_BACKGROUND].find('.') == std::string::npos)
                    stringSettings[STR_BACKGROUND] += ".png";
                else if (stringSettings[STR_BACKGROUND].substr(stringSettings[STR_BACKGROUND].rfind('.')) != ".png")
                    stringSettings[STR_BACKGROUND] = "";

                if (stringSettings[STR_BACKGROUND].length())
                    stringSettings[STR_BACKGROUND] = FileSystem::ValidFileName(stringSettings[STR_BACKGROUND], ".png");
            }
        }
    }

    if ((findParameter(sCmd, "xticks", '=')
        || findParameter(sCmd, "yticks", '=')
        || findParameter(sCmd, "zticks", '=')
        || findParameter(sCmd, "cticks", '=')) && (nType == ALL || nType & GLOBAL))
    {
        int nPos = 0;

        if ((nPos = findParameter(sCmd, "xticks", '=')))
        {
            sTickTemplate[0] = parseOpt(sCmd, nPos+6).printVals();

            if (sTickTemplate[0].find('%') == std::string::npos && sTickTemplate[0].length())
                sTickTemplate[0] += "%g";
        }

        if ((nPos = findParameter(sCmd, "yticks", '=')))
        {
            sTickTemplate[1] = parseOpt(sCmd, nPos+6).printVals();

            if (sTickTemplate[1].find('%') == std::string::npos && sTickTemplate[1].length())
                sTickTemplate[1] += "%g";
        }

        if ((nPos = findParameter(sCmd, "zticks", '=')))
        {
            sTickTemplate[2] = parseOpt(sCmd, nPos+6).printVals();

            if (sTickTemplate[2].find('%') == std::string::npos && sTickTemplate[2].length())
                sTickTemplate[2] += "%g";
        }

        if ((nPos = findParameter(sCmd, "cticks", '=')))
        {
            sTickTemplate[3] = parseOpt(sCmd, nPos+6).printVals();

            if (sTickTemplate[3].find('%') == std::string::npos && sTickTemplate[3].length())
                sTickTemplate[3] += "%g";
        }
    }

    if ((findParameter(sCmd, "xscale", '=')
        || findParameter(sCmd, "yscale", '=')
        || findParameter(sCmd, "zscale", '=')
        || findParameter(sCmd, "cscale", '=')) && (nType == ALL || nType & GLOBAL))
    {
        int nPos = 0;

        if ((nPos = findParameter(sCmd, "xscale", '=')))
            dAxisScale[0] = parseOpt(sCmd, nPos+6).front().getNum().asF64();

        if ((nPos = findParameter(sCmd, "yscale", '=')))
            dAxisScale[1] = parseOpt(sCmd, nPos+6).front().getNum().asF64();

        if ((nPos = findParameter(sCmd, "zscale", '=')))
            dAxisScale[2] = parseOpt(sCmd, nPos+6).front().getNum().asF64();

        if ((nPos = findParameter(sCmd, "cscale", '=')))
            dAxisScale[3] = parseOpt(sCmd, nPos+6).front().getNum().asF64();

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
        int nPos = 0;

        if ((nPos = findParameter(sCmd, "xticklabels", '=')))
            sCustomTicks[0] = parseOpt(sCmd, nPos+11).printJoined("\n", true);

        if ((nPos = findParameter(sCmd, "yticklabels", '=')))
            sCustomTicks[1] = parseOpt(sCmd, nPos+11).printJoined("\n", true);

        if ((nPos = findParameter(sCmd, "zticklabels", '=')))
            sCustomTicks[2] = parseOpt(sCmd, nPos+11).printJoined("\n", true);

        if ((nPos = findParameter(sCmd, "cticklabels", '=')))
            sCustomTicks[3] = parseOpt(sCmd, nPos+11).printJoined("\n", true);
    }

    if (sCmd.find('[') != std::string::npos && (nType == ALL || nType & GLOBAL))
    {
        size_t nPos = 0;

        do
        {
            nPos = sCmd.find('[', nPos);
            if (nPos == std::string::npos)
                break;
            nPos++;
        }
        while (isInQuotes(sCmd, nPos));

        if (nPos != std::string::npos && sCmd.find(']', nPos) != std::string::npos)
        {
            auto args = getAllArguments(sCmd.substr(nPos, sCmd.find(']', nPos) - nPos));

            for (size_t i = 0; i < args.size(); i++)
            {
                if (i > 4)
                    break;

                if (args[i].find(':') == std::string::npos || args[i] == ":")
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

            // Do not perform this logic for CRANGE and TRANGE
            for (size_t i = 0; i <= ZRANGE; i++)
            {
                if (!bRanges[i])
                    continue;

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


/////////////////////////////////////////////////
/// \brief Resets all settings to the
/// initialisation stage.
///
/// \return void
///
/////////////////////////////////////////////////
void PlotData::reset()
{
    _lVLines.clear();
    _lHlines.clear();

    ranges.intervals.clear();
    ranges.intervals.resize(3, Interval(-10.0, 10.0));
    ranges.intervals.push_back(Interval(NAN, NAN));
    ranges.intervals.push_back(Interval(0, 1.0));

    ranges.setNames({"x", "y", "z", "c", "t"});

    for (int i = XRANGE; i <= ZRANGE; i++)
    {
        dOrigin[i] = 0.0;
        sAxisLabels[i] = "";
        bDefaultAxisLabels[i] = true;
        _lHlines.push_back(Line());
        _lVLines.push_back(Line());
        nSlices[i] = 1;
    }

    for (int i = XRANGE; i <= CRANGE; i++)
    {
        bRanges[i] = false;
        bMirror[i] = false;
        bLogscale[i] = false;
        sTickTemplate[i] = "";
        sCustomTicks[i] = "";
        dAxisScale[i] = 1.0;
        _timeAxes[i].deactivate();
    }

    for (int i = XRANGE; i <= YRANGE; i++)
    {
        _AddAxes[i].ivl.reset(NAN, NAN);
        _AddAxes[i].sLabel = "";
        _AddAxes[i].sStyle = SECAXIS_DEFAULT_COLOR;
    }

    for (size_t i = 0; i < LOG_SETTING_SIZE; i++)
    {
        logicalSettings[i] = false;
    }

    logicalSettings[LOG_OPENIMAGE] = true;
    logicalSettings[LOG_COLORBAR] = true;

    for (size_t i = 0; i < INT_SETTING_SIZE; i++)
    {
        intSettings[i] = 0;
    }

    intSettings[INT_AXIS] = AXIS_STD;
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
    stringSettings[STR_LINESIZES] =      "11111111111111111111";
    stringSettings[STR_GREYS] =          "kHhWkHhWkHhWkHhWkHhW";
    stringSettings[STR_LINESTYLESGREY] = "-|=;i:j|=;i:j-|=:i;-";
    stringSettings[STR_GRIDSTYLE] = "=h0-h0";
    stringSettings[STR_PLOTBOUNDARIES] = "<>_^";

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


/////////////////////////////////////////////////
/// \brief Delete the internal per-plot data
/// (i.e. weak reset).
///
/// \param bGraphFinished bool
/// \return void
///
/////////////////////////////////////////////////
void PlotData::deleteData(bool bGraphFinished /* = false*/)
{
    _lHlines.clear();
    _lVLines.clear();

    for (int i = XRANGE; i <= ZRANGE; i++)
    {
        ranges[i].reset(-10.0, 10.0);
        sAxisLabels[i] = "";
        bDefaultAxisLabels[i] = true;
        _lHlines.push_back(Line());
        _lVLines.push_back(Line());
    }

    ranges[CRANGE].reset(NAN, NAN);

    for (int i = XRANGE; i <= CRANGE; i++)
    {
        bRanges[i] = false;
        bMirror[i] = false;
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

        if (!logicalSettings[LOG_ALLHIGHRES])
            intSettings[INT_HIGHRESLEVEL] = 0;
    }

    logicalSettings[LOG_PARAMETRIC] = false;
    logicalSettings[LOG_STACKEDBARS] = false;
    nRequestedLayers = 1;
    intSettings[INT_LEGENDSTYLE] = 0;

    stringSettings[STR_COLORS] =         "rbGqmPunclRBgQMpUNCL";
    stringSettings[STR_CONTCOLORS] =     "kUHYPCQNLMhuWypcqnlm";
    stringSettings[STR_CONTGREYS] =      "kwkwkwkwkwkwkwkwkwkw";
    stringSettings[STR_POINTSTYLES] =    " + x o s . d#+#x#.#* x o s . d#+#x#.#* +";
    stringSettings[STR_LINESTYLES] =     "----------;;;;;;;;;;";
    stringSettings[STR_LINESIZES] =      "11111111111111111111";
    stringSettings[STR_GREYS] =          "kHhWkHhWkHhWkHhWkHhW";
    stringSettings[STR_LINESTYLESGREY] = "-|=;i:j|=;i:j-|=:i;-";
    stringSettings[STR_FILENAME].clear();
    stringSettings[STR_PLOTTITLE].clear();
    stringSettings[STR_AXISBIND].clear();
    stringSettings[STR_BACKGROUND].clear();
    stringSettings[STR_PLOTBOUNDARIES] = "<>_^";

    for (int i = XRANGE; i <= YRANGE; i++)
    {
        _AddAxes[i].ivl.reset(NAN, NAN);
        _AddAxes[i].sLabel = "";
        _AddAxes[i].sStyle = SECAXIS_DEFAULT_COLOR;
    }
}


/////////////////////////////////////////////////
/// \brief Return the internal plotting
/// parameters as a human-readable string. Can be
/// converted to an internal string using the
/// additional parameter.
///
/// \param asstr bool
/// \return std::string
///
/////////////////////////////////////////////////
std::string PlotData::getParams(bool asstr) const
{
    const Settings& _option = NumeReKernel::getInstance()->getSettings();
    std::string sReturn = "";
    std::string sSepString = "; ";
    static std::map<std::string,std::pair<PlotData::LogicalPlotSetting,PlotData::ParamType>> mGenericSwitches = getGenericSwitches();
    static std::map<std::string,std::string> mColorSchemes = getColorSchemes();

    if (asstr)
    {
        sReturn = "\"";
        sSepString = "\", \"";
    }

    sReturn += "[";

    for (size_t i = XRANGE; i <= TRANGE; i++)
    {
        sReturn += toString(ranges[i].front(), _option.getPrecision()) + ":" + toString(ranges[i].back(), _option.getPrecision());

        if (i < TRANGE)
            sReturn += ", ";
    }

    sReturn += "]" + sSepString;

    // Handle generic switches first
    for (const auto& iter : mGenericSwitches)
    {
        if (logicalSettings[iter.second.first] && iter.first != "reconstruct")
            sReturn += iter.first + sSepString;
    }

    if (logicalSettings[LOG_ALPHA])
        sReturn += "alpha" + sSepString;

    if (logicalSettings[LOG_ANIMATE])
        sReturn += "animate [" + toString(intSettings[INT_ANIMATESAMPLES]) + " frames]" + sSepString;

    sReturn += "aspect=" + toString(floatSettings[FLOAT_ASPECT], 4) + sSepString;

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

    if (stringSettings[STR_BACKGROUNDCOLORSCHEME] == "<<REALISTIC>>")
        sReturn += "real" + sSepString;
    else
    {
        bool found = false;

        for (const auto& iter : mColorSchemes)
        {
            if (iter.second == stringSettings[STR_BACKGROUNDCOLORSCHEME])
            {
                sReturn += iter.first + sSepString;
                found = true;
                break;
            }
        }

        if (!found)
        {
            if (asstr)
                sReturn += "\\\"" + stringSettings[STR_BACKGROUNDCOLORSCHEME] + "\\\"" + sSepString;
            else
                sReturn += "\"" + stringSettings[STR_BACKGROUNDCOLORSCHEME] + "\"" + sSepString;
        }
    }

    sReturn += "colorscheme=";

    {
        bool found = false;

        for (const auto& iter : mColorSchemes)
        {
            if (iter.second == stringSettings[STR_COLORSCHEME])
            {
                sReturn += iter.first + sSepString;
                found = true;
                break;
            }
        }

        if (!found)
        {
            if (asstr)
                sReturn += "\\\"" + stringSettings[STR_COLORSCHEME] + "\\\"" + sSepString;
            else
                sReturn += "\"" + stringSettings[STR_COLORSCHEME] + "\"" + sSepString;
        }
    }


    if (intSettings[INT_COMPLEXMODE] == CPLX_REIM)
        sReturn += "complexmode=reim" + sSepString;
    else if (intSettings[INT_COMPLEXMODE] == CPLX_PLANE)
        sReturn += "complexmode=plane" + sSepString;

    if (intSettings[INT_COORDS] >= 100)
        sReturn += "spherical coords" + sSepString;
    else if (intSettings[INT_COORDS] >= 10)
        sReturn += "polar coords" + sSepString;

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

    sReturn += "origin=";

    if (isnan(dOrigin[0]) && isnan(dOrigin[1]) && isnan(dOrigin[2]))
        sReturn += "sliding" + sSepString;
    else if (dOrigin[0] == 0.0 && dOrigin[1] == 0.0 && dOrigin[2] == 0.0)
        sReturn += "std" + sSepString;
    else
        sReturn += "[" + toString(dOrigin[0], _option) + ", " + toString(dOrigin[1], _option) + ", " + toString(dOrigin[2], _option) + "]" + sSepString;

    sReturn += "slices=[" +toString((int)nSlices[0]) + ", " + toString((int)nSlices[1]) + ", " + toString((int)nSlices[2]) + "]" + sSepString;

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


/////////////////////////////////////////////////
/// \brief Change the number of samples.
///
/// \param _nSamples int
/// \return void
///
/////////////////////////////////////////////////
void PlotData::setSamples(int _nSamples)
{
    intSettings[INT_SAMPLES] = _nSamples;
}


/////////////////////////////////////////////////
/// \brief Change the output file name.
///
/// \param _sFileName std::string
/// \return void
///
/////////////////////////////////////////////////
void PlotData::setFileName(std::string _sFileName)
{
    if (_sFileName.length())
    {
        std::string sExt = _sFileName.substr(_sFileName.rfind('.'));

        if (sExt[sExt.length()-1] == '"')
            sExt = sExt.substr(0,sExt.length()-1);

        if (_sFileName.find('\\') == std::string::npos && _sFileName.find('/') == std::string::npos)
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
}


/////////////////////////////////////////////////
/// \brief Change the plot title on-the-fly.
///
/// \param sTitle const std::string&
/// \return void
///
/////////////////////////////////////////////////
void PlotData::setTitle(const std::string& sTitle)
{
    stringSettings[STR_PLOTTITLE] = sTitle;
}


/////////////////////////////////////////////////
/// \brief Replaces tab and newlines
/// correspondingly.
///
/// \param sString std::string&
/// \return void
///
/////////////////////////////////////////////////
void PlotData::replaceControlChars(std::string& sString)
{
    if (sString.find('\t') == std::string::npos && sString.find('\n') == std::string::npos)
        return;

    for (size_t i = 0; i < sString.length(); i++)
    {
        if (sString[i] == '\t' && sString.substr(i+1,2) == "au")
            sString.replace(i, 1, "\\t");

        if (sString[i] == '\n' && sString[i+1] == 'u')
            sString.replace(i, 1, "\\n");
    }
}


/////////////////////////////////////////////////
/// \brief Removes surrounding quotation marks.
///
/// \param sString const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string PlotData::removeSurroundingQuotationMarks(const std::string& sString)
{
    if (sString.front() == '"' && sString.back() == '"')
        return sString.substr(1,sString.length()-2);

    return sString;
}


/////////////////////////////////////////////////
/// \brief Structure for simplification of the
/// standard axis labels
/////////////////////////////////////////////////
struct AxisLabels
{
    std::string x;
    std::string y;
    std::string z;
};


/////////////////////////////////////////////////
/// \brief Static helper function to create a map
/// containing the standard axis labels for each
/// coordinate system.
///
/// \return std::map<CoordinateSystem, AxisLabels>
///
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief Return the axis label associated to
/// the selected axis.
///
/// \param axis size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string PlotData::getAxisLabel(size_t axis) const
{
    if (!bDefaultAxisLabels[axis])
        return replaceToTeX(sAxisLabels[axis]);
    else if (intSettings[INT_COMPLEXMODE] == CPLX_PLANE)
    {
        if (axis == XCOORD)
            return "Re \\i z";
        else if (axis == YCOORD)
            return "Im \\i z";
        else if (axis == ZCOORD)
            return "|\\i z|";
    }
    else
    {
        static std::map<CoordinateSystem,AxisLabels> mLabels = getLabelDefinitions();

        if (axis == XCOORD)
            return mLabels[(CoordinateSystem)intSettings[INT_COORDS]].x;
        else if (axis == YCOORD)
            return mLabels[(CoordinateSystem)intSettings[INT_COORDS]].y;
        else if (axis == ZCOORD)
            return mLabels[(CoordinateSystem)intSettings[INT_COORDS]].z;
    }

    return "";
}




