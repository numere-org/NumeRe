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


#include "plugins.hpp"
#include "../kernel.hpp"

/*
 * Plugin zur Erzeugung von Histogramm-Rubriken
 */

const std::string PI_HIST = "1.1.2";
extern mglGraph _fontData;

/////////////////////////////////////////////////
/// \brief This enumeration defines the available
/// bin determination methods for 1D and 2D
/// histograms.
/////////////////////////////////////////////////
enum HistBinMethod
{
    STURGES,
    SCOTT,
    FREEDMAN_DIACONIS
};


/////////////////////////////////////////////////
/// \brief This structure defines the available
/// ranges for the histograms.
/////////////////////////////////////////////////
struct Ranges
{
    double x[2];
    double y[2];
    double z[2];
};


/////////////////////////////////////////////////
/// \brief This structure gathers all necessary
/// parameters for the histograms.
/////////////////////////////////////////////////
struct HistogramParameters
{
    Ranges ranges;

    double binWidth[2];
    int nBin;
    HistBinMethod nMethod;
    std::string sTable;
    std::string sBinLabel;
    std::string sCountLabel;
    std::string sAxisLabels[3];
    std::string sSavePath;
};


/////////////////////////////////////////////////
/// \brief This static function returns the value
/// of the selected command line option (passable
/// in two representations) as a std::string.
///
/// \param sCmd const std::string&
/// \param sVersion1 const std::string&
/// \param sVersion2 const std::string&
/// \param sDefaultVal const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string getParameterValue(const std::string& sCmd, const std::string& sVersion1, const std::string& sVersion2, const std::string& sDefaultVal)
{
    // Try to find one of the two possible
    // option variants
    if (findParameter(sCmd, sVersion1, '=') || findParameter(sCmd, sVersion2, '='))
    {
        int nPos = 0;

        // Use the detected variant
        if (findParameter(sCmd, sVersion1, '='))
            nPos = findParameter(sCmd, sVersion1, '=')+sVersion1.length();
        else
            nPos = findParameter(sCmd, sVersion2, '=')+sVersion2.length();

        // Get the value of the option and
        // strip all surrounding spaces
        std::string val = getArgAtPos(sCmd, nPos);
        StripSpaces(val);
        return val;
    }

    return sDefaultVal;
}


/////////////////////////////////////////////////
/// \brief This static function decodes a
/// selected range definition (e.g. x=0:1) into
/// doubles.
///
/// \param sCmd const std::string&
/// \param sIdentifier const std::string&
/// \param dMin double&
/// \param dMax double&
/// \return void
///
/////////////////////////////////////////////////
static void getIntervalDef(const std::string& sCmd, const std::string& sIdentifier, double& dMin, double& dMax)
{
    // Does this interval definition exist?
    if (findParameter(sCmd, sIdentifier, '='))
    {
        // Get the interval definition
        std::string sTemp =  getArgAtPos(sCmd, findParameter(sCmd, sIdentifier, '=') + sIdentifier.length());

        // If the interval definition actually contains
        // a colon, decode it and use it
        if (sTemp.find(':') != std::string::npos)
        {
            if (sTemp.substr(0, sTemp.find(':')).length())
                dMin = StrToDb(sTemp.substr(0, sTemp.find(':')));

            if (sTemp.substr(sTemp.find(':') + 1).length())
                dMax = StrToDb(sTemp.substr(sTemp.find(':') + 1));
        }
    }
}


/////////////////////////////////////////////////
/// \brief This static function replaces invalid
/// ranges boundaries with the passed minimal and
/// maximal data values.
///
/// \param sCmd const std::string&
/// \param dMin double&
/// \param dMax double&
/// \param dDataMin double
/// \param dDataMax double
/// \return void
///
/////////////////////////////////////////////////
static void prepareIntervalsForHist(const std::string& sCmd, double& dMin, double& dMax, double dDataMin, double dDataMax)
{
    // Replace the missing interval boundaries
    // with the minimal and maximal data values
    if (isnan(dMin) && isnan(dMax))
    {
        dMin = dDataMin;
        dMax = dDataMax;
        double dIntervall = dMax - dMin;
        dMax += dIntervall / 10.0;
        dMin -= dIntervall / 10.0;
    }
    else if (isnan(dMin))
        dMin = dDataMin;
    else if (isnan(dMax))
        dMax = dDataMax;

    // Ensure the correct order
    if (dMax < dMin)
    {
        double dTemp = dMax;
        dMax = dMin;
        dMin = dTemp;
    }

    // Ensure that the selected interval is part of
    // the data interval
    if (!isnan(dMin) && !isnan(dMax))
    {
        if (dMin > dDataMax || dMax < dDataMin)
            throw SyntaxError(SyntaxError::INVALID_INTERVAL, sCmd, SyntaxError::invalid_position);
    }
}


/////////////////////////////////////////////////
/// \brief This static function calculates the
/// data for a 1D histogram. The data is returned
/// as a vector<vector<double>> instance. The
/// data for the plotting is filled in this
/// function as well.
///
/// \param _data MemoryManager&
/// \param _idx const Indices&
/// \param _histParams const HistogramParameters&
/// \param _histData mglData&
/// \param _mAxisVals mglData&
/// \param nMax int&
/// \param vLegends std::vector<std::string>&
/// \param bGrid bool
/// \param isXLog bool
/// \return std::vector<std::vector<double>>
///
/////////////////////////////////////////////////
static std::vector<std::vector<double>> calculateHist1dData(MemoryManager& _data, const Indices& _idx, const HistogramParameters& _histParams, mglData& _histData, mglData& _mAxisVals, int& nMax, std::vector<std::string>& vLegends, bool bGrid, bool isXLog)
{
    // Prepare the data table
    std::vector<std::vector<double>> vHistMatrix(_histParams.nBin, std::vector<double>(bGrid ? 1 : _idx.col.size(), 0.0));

    int nCount = 0;

    if (bGrid)
    {
        nMax = 0;

        // Repeat for every bin
        for (int k = 0; k < _histParams.nBin; k++)
        {
            nCount = 0;

            // Detect the number of values, which
            // are part of the current bin interval
            for (size_t i = 2; i < _idx.col.size(); i++)
            {
                for (size_t l = 0; l < _idx.row.size(); l++)
                {
                    if (_data.getElement(_idx.row[l], _idx.col[0], _histParams.sTable).real() > _histParams.ranges.x[1]
                            || _data.getElement(_idx.row[l], _idx.col[0], _histParams.sTable).real() < _histParams.ranges.x[0]
                            || _data.getElement(_idx.row[l], _idx.col[1], _histParams.sTable).real() > _histParams.ranges.y[1]
                            || _data.getElement(_idx.row[l], _idx.col[1], _histParams.sTable).real() < _histParams.ranges.y[0]
                            || _data.getElement(_idx.row[l], _idx.col[i], _histParams.sTable).real() > _histParams.ranges.z[1]
                            || _data.getElement(_idx.row[l], _idx.col[i], _histParams.sTable).real() < _histParams.ranges.z[0])
                        continue;

                    if (isXLog)
                    {
                        if (_data.getElement(_idx.row[l], _idx.col[i], _histParams.sTable).real() >= pow(10.0, log10(_histParams.ranges.z[0]) + k * _histParams.binWidth[0])
                                && _data.getElement(_idx.row[l], _idx.col[i], _histParams.sTable).real() < pow(10.0, log10(_histParams.ranges.z[0]) + (k + 1) * _histParams.binWidth[0]))
                            nCount++;
                    }
                    else
                    {
                        if (_data.getElement(_idx.row[l], _idx.col[i], _histParams.sTable).real() >= _histParams.ranges.z[0] + k * _histParams.binWidth[0]
                                && _data.getElement(_idx.row[l], _idx.col[i], _histParams.sTable).real() < _histParams.ranges.z[0] + (k + 1) * _histParams.binWidth[0])
                            nCount++;
                    }
                }

                if (i == 2 && !isXLog)
                    _mAxisVals.a[k] = _histParams.ranges.z[0] + (k + 0.5) * _histParams.binWidth[0];
                else if (i == 2)
                    _mAxisVals.a[k] = pow(10.0, log10(_histParams.ranges.z[0]) + (k + 0.5) * _histParams.binWidth[0]);
            }

            // Store the value in the corresponding column
            vHistMatrix[k][0] = nCount;
            _histData.a[k] = nCount;

            if (nCount > nMax)
                nMax = nCount;
        }

        vLegends.push_back("grid");
    }
    else
    {
        nMax = 0;

        // Repeat for every data set
        for (size_t i = 0; i < _idx.col.size(); i++)
        {
            // Repeat for every bin
            for (int k = 0; k < _histParams.nBin; k++)
            {
                nCount = 0;

                // Detect the number of values, which
                // are part of the current bin interval
                for (size_t l = 0; l < _idx.row.size(); l++)
                {
                    if (isXLog)
                    {
                        if (_data.getElement(_idx.row[l], _idx.col[i], _histParams.sTable).real() >= pow(10.0, log10(_histParams.ranges.x[0]) + k * _histParams.binWidth[0])
                                && _data.getElement(_idx.row[l], _idx.col[i], _histParams.sTable).real() < pow(10.0, log10(_histParams.ranges.x[0]) + (k + 1) * _histParams.binWidth[0]))
                            nCount++;
                    }
                    else
                    {
                        if (_data.getElement(_idx.row[l], _idx.col[i], _histParams.sTable).real() >= _histParams.ranges.x[0] + k * _histParams.binWidth[0]
                                && _data.getElement(_idx.row[l], _idx.col[i], _histParams.sTable).real() < _histParams.ranges.x[0] + (k + 1) * _histParams.binWidth[0])
                            nCount++;
                    }
                }

                if (!i && !isXLog)
                    _mAxisVals.a[k] = _histParams.ranges.x[0] + (k + 0.5) * _histParams.binWidth[0];
                else if (!i)
                    _mAxisVals.a[k] = pow(10.0, log10(_histParams.ranges.x[0]) + (k + 0.5) * _histParams.binWidth[0]);

                // Store the value in the corresponding column
                vHistMatrix[k][i] = nCount;
                _histData.a[k + (_histParams.nBin * i)] = nCount;

                if (nCount > nMax)
                    nMax = nCount;
            }

            // Create the plot legend entry for the current data set
            vLegends.push_back(replaceToTeX(_data.getTopHeadLineElement(_idx.col[i], _histParams.sTable)));
        }
    }

    return vHistMatrix;
}


/////////////////////////////////////////////////
/// \brief This static function calculates the
/// custom ticks (one for every bin) and formats
/// them accordingly.
///
/// \param _histParams const HistogramParameters&
/// \param _mAxisVals const mglData&
/// \param sCommonExponent std::string&
/// \param bGrid bool
/// \return std::string
///
/////////////////////////////////////////////////
static std::string prepareTicksForHist1d(const HistogramParameters& _histParams, const mglData& _mAxisVals, std::string& sCommonExponent, bool bGrid)
{
    std::string sTicks;
    double dCommonExponent = 1.0;

    // Try to find the common bin interval exponent to
    // factorize it out
    if (bGrid)
    {
        if (toString(_histParams.ranges.z[0] + _histParams.binWidth[0] / 2.0, 3).find('e') != std::string::npos || toString(_histParams.ranges.z[0] + _histParams.binWidth[0] / 2.0, 3).find('E') != std::string::npos)
        {
            sCommonExponent = toString(_histParams.ranges.z[0] + _histParams.binWidth[0] / 2.0, 3).substr(toString(_histParams.ranges.z[0] + _histParams.binWidth[0] / 2.0, 3).find('e'));
            dCommonExponent = StrToDb("1.0" + sCommonExponent);

            for (int i = 0; i < _histParams.nBin; i++)
            {
                if (toString((_histParams.ranges.z[0] + i * _histParams.binWidth[0] + _histParams.binWidth[0] / 2.0) / dCommonExponent, 3).find('e') != std::string::npos)
                {
                    sCommonExponent = "";
                    dCommonExponent = 1.0;
                    break;
                }
            }

            sTicks = toString((_histParams.ranges.z[0] + _histParams.binWidth[0] / 2.0) / dCommonExponent, 3) + "\\n";
        }
        else
            sTicks = toString(_histParams.ranges.z[0] + _histParams.binWidth[0] / 2.0, 3) + "\\n";
    }
    else
    {
        if (toString(_histParams.ranges.x[0] + _histParams.binWidth[0] / 2.0, 3).find('e') != std::string::npos || toString(_histParams.ranges.x[0] + _histParams.binWidth[0] / 2.0, 3).find('E') != std::string::npos)
        {
            sCommonExponent = toString(_histParams.ranges.x[0] + _histParams.binWidth[0] / 2.0, 3).substr(toString(_histParams.ranges.x[0] + _histParams.binWidth[0] / 2.0, 3).find('e'));
            dCommonExponent = StrToDb("1.0" + sCommonExponent);

            for (int i = 0; i < _histParams.nBin; i++)
            {
                if (toString((_histParams.ranges.x[0] + i * _histParams.binWidth[0] + _histParams.binWidth[0] / 2.0) / dCommonExponent, 3).find('e') != std::string::npos)
                {
                    sCommonExponent = "";
                    dCommonExponent = 1.0;
                    break;
                }
            }

            sTicks = toString((_histParams.ranges.x[0] + _histParams.binWidth[0] / 2.0) / dCommonExponent, 3) + "\\n";
        }
        else
            sTicks = toString(_histParams.ranges.x[0] + _histParams.binWidth[0] / 2.0, 3) + "\\n";
    }

    // Create the ticks list by factorizing out
    // the common exponent
    for (int i = 1; i < _histParams.nBin - 1; i++)
    {
        if (_histParams.nBin > 16)
        {
            if (!((_histParams.nBin - 1) % 2) && !(i % 2) && _histParams.nBin - 1 < 33)
                sTicks += toString(_mAxisVals.a[i] / dCommonExponent, 3) + "\\n";
            else if (!((_histParams.nBin - 1) % 2) && (i % 2) && _histParams.nBin - 1 < 33)
                sTicks += "\\n";
            else if (!((_histParams.nBin - 1) % 4) && !(i % 4))
                sTicks += toString(_mAxisVals.a[i] / dCommonExponent, 3) + "\\n";
            else if (!((_histParams.nBin - 1) % 4) && (i % 4))
                sTicks += "\\n";
            else if (!((_histParams.nBin - 1) % 3) && !(i % 3))
                sTicks += toString(_mAxisVals.a[i] / dCommonExponent, 3) + "\\n";
            else if (!((_histParams.nBin - 1) % 3) && (i % 3))
                sTicks += "\\n";
            else if (!((_histParams.nBin - 1) % 5) && !(i % 5))
                sTicks += toString(_mAxisVals.a[i] / dCommonExponent, 3) + "\\n";
            else if (!((_histParams.nBin - 1) % 5) && (i % 5))
                sTicks += "\\n";
            else if (!((_histParams.nBin - 1) % 7) && !(i % 7))
                sTicks += toString(_mAxisVals.a[i] / dCommonExponent, 3) + "\\n";
            else if (!((_histParams.nBin - 1) % 7) && (i % 7))
                sTicks += "\\n";
            else if (!((_histParams.nBin - 1) % 11) && !(i % 11))
                sTicks += toString(_mAxisVals.a[i] / dCommonExponent, 3) + "\\n";
            else if (!((_histParams.nBin - 1) % 11) && (i % 11))
                sTicks += "\\n";
            else if (((_histParams.nBin - 1) % 2 && (_histParams.nBin - 1) % 3 && (_histParams.nBin - 1) % 5 && (_histParams.nBin - 1) % 7 && (_histParams.nBin - 1) % 11) && !(i % 3))
                sTicks += toString(_mAxisVals.a[i] / dCommonExponent, 3) + "\\n";
            else
                sTicks += "\\n";
        }
        else
            sTicks += toString(_mAxisVals.a[i] / dCommonExponent, 3) + "\\n";
    }

    sTicks += toString(_mAxisVals.a[_histParams.nBin - 1] / dCommonExponent, 3);

    // Convert the common exponent into a LaTeX string
    if (sCommonExponent.length())
    {
        while (sTicks.find(sCommonExponent) != std::string::npos)
            sTicks.erase(sTicks.find(sCommonExponent), sCommonExponent.length());

        if (sCommonExponent.find('-') != std::string::npos)
            sCommonExponent = "\\times 10^{-" + sCommonExponent.substr(sCommonExponent.find_first_not_of('0', 2)) + "}";
        else
            sCommonExponent = "\\times 10^{" + sCommonExponent.substr(sCommonExponent.find_first_not_of('0', 2)) + "}";
    }

    return sTicks;
}


/////////////////////////////////////////////////
/// \brief This static function creates the
/// terminal and file output for a 1D histogram.
///
/// \param _data MemoryManager&
/// \param _idx const Indices&
/// \param vHistMatrix const std::vector<std::vector<double>>&
/// \param _histParams const HistogramParameters&
/// \param _mAxisVals const mglData&
/// \param bGrid bool
/// \param bFormat bool
/// \param bSilent bool
/// \return void
///
/////////////////////////////////////////////////
static void createOutputForHist1D(MemoryManager& _data, const Indices& _idx, const std::vector<std::vector<double>>& vHistMatrix, const HistogramParameters& _histParams, const mglData& _mAxisVals, bool bGrid, bool bFormat, bool bSilent)
{
    Output& _out = NumeReKernel::getInstance()->getOutput();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    std::string** sOut = new std::string*[vHistMatrix.size() + 1];

    for (size_t i = 0; i < vHistMatrix.size() + 1; i++)
    {
        sOut[i] = new std::string[vHistMatrix[0].size() + 1];

        for (size_t j = 0; j < vHistMatrix[0].size() + 1; j++)
        {
            sOut[i][j] = "";
        }
    }

    // --> Schreibe Tabellenkoepfe <--
    sOut[0][0] = _histParams.sBinLabel;

    if (bGrid)
        sOut[0][1] = condenseText(_histParams.sCountLabel) + ": grid";
    else
    {
        for (size_t i = 1; i < vHistMatrix[0].size() + 1; i++)
        {
            sOut[0][i] = condenseText(_histParams.sCountLabel) + ": " + _data.getTopHeadLineElement(_idx.col[i-1], _histParams.sTable);

            //size_t nPos;

            //while ((nPos = sOut[0][i].find(' ')) != std::string::npos)
            //    sOut[0][i][nPos] = '_';
        }
    }

    // --> Setze die ueblichen Ausgabe-Info-Parameter <--
    if (bFormat)
    {
        _out.setPluginName(_lang.get("HIST_OUT_PLGNINFO", PI_HIST, toString(_idx.col.front() + 1), toString(_idx.col.last()+1), _data.getDataFileName(_histParams.sTable)));

        if (bGrid)
            _out.setCommentLine(_lang.get("HIST_OUT_COMMENTLINE", toString(_histParams.ranges.z[0], 5), toString(_histParams.ranges.z[1], 5), toString(_histParams.binWidth[0], 5)));
        else
            _out.setCommentLine(_lang.get("HIST_OUT_COMMENTLINE", toString(_histParams.ranges.x[0], 5), toString(_histParams.ranges.x[1], 5), toString(_histParams.binWidth[0], 5)));

        _out.setPrefix("hist");
    }

    if (_out.isFile())
    {
        _out.setStatus(true);
        _out.setCompact(false);

        if (_histParams.sSavePath.length())
            _out.setFileName(_histParams.sSavePath);
        else
            _out.generateFileName();
    }
    else
        _out.setCompact(_option.createCompactTables());

    // --> Fuelle die Ausgabe-Matrix <--
    for (size_t i = 1; i < vHistMatrix.size() + 1; i++)
    {
        // --> Die erste Spalte enthaelt immer die linke Grenze des Bin-Intervalls <--
        sOut[i][0] = toString(_mAxisVals.a[i - 1], _option);

        for (size_t j = 0; j < vHistMatrix[0].size(); j++)
        {
            sOut[i][j + 1] = toString(vHistMatrix[i - 1][j], _option);
        }
    }

    // --> Uebergabe an Output::format(string**,int,int,Settings&), das den Rest erledigt
    if (bFormat)
    {
        if (_out.isFile() || (!bSilent && _option.systemPrints()))
        {
            if (!_out.isFile())
            {
                NumeReKernel::toggleTableStatus();
                make_hline();
                NumeReKernel::print("NUMERE: " + toSystemCodePage(toUpperCase(_lang.get("HIST_HEADLINE"))));
                make_hline();
            }

            _out.format(sOut, vHistMatrix[0].size() + 1, vHistMatrix.size() + 1, _option, true);

            if (!_out.isFile())
            {
                NumeReKernel::toggleTableStatus();
                make_hline();
            }
        }
    }

    // --> WICHTIG: Speicher wieder freigeben! <--
    for (size_t i = 0; i < vHistMatrix.size() + 1; i++)
    {
        delete[] sOut[i];
    }

    delete[] sOut;
}


/////////////////////////////////////////////////
/// \brief This static function prepares a
/// mglGraph instance for histogram plotting.
/// It's usable in 1D and 2D case.
///
/// \param dAspect double
/// \param _pData PlotData&
/// \param bSilent bool
/// \return mglGraph*
///
/////////////////////////////////////////////////
static mglGraph* prepareGraphForHist(double dAspect, PlotData& _pData, bool bSilent)
{
    // Create a new mglGraph instance on the heap
    mglGraph* _histGraph = new mglGraph();

    // Apply plot output size using the resolution
    // and the aspect settings
    if (_pData.getHighRes() == 2 && bSilent && _pData.getSilentMode())
    {
        double dHeight = sqrt(1920.0 * 1440.0 / dAspect);
        _histGraph->SetSize((int)lrint(dAspect * dHeight), (int)lrint(dHeight));
    }
    else if (_pData.getHighRes() == 1 && bSilent && _pData.getSilentMode())
    {
        double dHeight = sqrt(1280.0 * 960.0 / dAspect);
        _histGraph->SetSize((int)lrint(dAspect * dHeight), (int)lrint(dHeight));
    }
    else
    {
        double dHeight = sqrt(800.0 * 600.0 / dAspect);
        _histGraph->SetSize((int)lrint(dAspect * dHeight), (int)lrint(dHeight));
    }

    // Get curret plot font, font size
    // and the width of the bars
    _histGraph->CopyFont(&_fontData);
    _histGraph->SetFontSizeCM(0.24 * ((double)(1 + _pData.getTextSize()) / 6.0), 72);
    _histGraph->SetBarWidth(_pData.getBars() ? _pData.getBars() : 0.9);

    _histGraph->SetRanges(1, 2, 1, 2, 1, 2);

    // Apply logarithmic functions to the axes,
    // if necessary
    if (_pData.getxLogscale() && !_pData.getyLogscale() && !_pData.getzLogscale())
        _histGraph->SetFunc("lg(x)", "");
    else if (_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale())
        _histGraph->SetFunc("lg(x)", "lg(y)");
    else if (!_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale())
        _histGraph->SetFunc("", "lg(y)");
    else if (_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale())
        _histGraph->SetFunc("lg(x)", "", "lg(z)");
    else if (!_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale())
        _histGraph->SetFunc("", "", "lg(z)");
    else if (!_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale())
        _histGraph->SetFunc("", "lg(y)", "lg(z)");
    else if (_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale())
        _histGraph->SetFunc("lg(x)", "lg(y)", "lg(z)");

    return _histGraph;
}


/////////////////////////////////////////////////
/// \brief This static function creates the plot
/// for a 1D histogram.
///
/// \param _histParams HistogramParameters&
/// \param _mAxisVals mglData&
/// \param _histData mglData&
/// \param vLegends const std::vector<std::string>&
/// \param nMax int
/// \param bSilent bool
/// \param bGrid bool
/// \return void
///
/////////////////////////////////////////////////
static void createPlotForHist1D(HistogramParameters& _histParams, mglData& _mAxisVals, mglData& _histData, const std::vector<std::string>& vLegends, int nMax, bool bSilent, bool bGrid)
{
    Output& _out = NumeReKernel::getInstance()->getOutput();
    PlotData& _pData = NumeReKernel::getInstance()->getPlottingData();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    // Define aspect and plotting colors
    double dAspect = 8.0 / 3.0;
    int nStyle = 0;
    const int nStyleMax = 14;
    std::string sColorStyles[nStyleMax] = {"r", "g", "b", "q", "m", "P", "u", "R", "G", "B", "Q", "M", "p", "U"};

    // Get current color definition
    for (int i = 0; i < nStyleMax; i++)
    {
        sColorStyles[i] = _pData.getColors()[i];
    }

    // Get a new mglGraph instance located on the heap
    mglGraph* _histGraph = prepareGraphForHist(dAspect, _pData, bSilent);

    // Add all legends to the graph
    for (size_t i = 0; i < vLegends.size(); i++)
    {
        _histGraph->AddLegend(vLegends[i].c_str(), sColorStyles[nStyle].c_str());

        if (nStyle == nStyleMax - 1)
            nStyle = 0;
        else
            nStyle++;
    }

    // Get the common exponent and the precalculated ticks
    std::string sCommonExponent;
    std::string sTicks = prepareTicksForHist1d(_histParams, _mAxisVals, sCommonExponent, bGrid);

    // If we calculated a single histogram from a data grid,
    // we need to use the z ranges for the x ranges, because
    // those are the values we binned for
    if (bGrid)
    {
        _histParams.ranges.x[0] = _histParams.ranges.z[0];
        _histParams.ranges.x[1] = _histParams.ranges.z[1];
    }

    // Update the x ranges for a possible logscale
    if (_pData.getxLogscale() && _histParams.ranges.x[0] <= 0.0 && _histParams.ranges.x[1] > 0.0)
        _histParams.ranges.x[0] = _histParams.ranges.x[1] / 1e3;
    else if (_pData.getxLogscale() && _histParams.ranges.x[0] < 0.0 && _histParams.ranges.x[1] <= 0.0)
    {
        _histParams.ranges.x[0] = 1.0;
        _histParams.ranges.x[1] = 1.0;
    }

    // Update the y ranges for a possible logscale
    if (_pData.getyLogscale())
        _histGraph->SetRanges(_histParams.ranges.x[0], _histParams.ranges.x[1], 0.1, 1.4 * (double)nMax);
    else
        _histGraph->SetRanges(_histParams.ranges.x[0], _histParams.ranges.x[1], 0.0, 1.05 * (double)nMax);

    // Create the axes
    if (_pData.getAxis())
    {
        if (!_pData.getxLogscale())
            _histGraph->SetTicksVal('x', _mAxisVals, sTicks.c_str());

        if (!_pData.getBox() && _histParams.ranges.x[0] <= 0.0 && _histParams.ranges.x[1] >= 0.0 && !_pData.getyLogscale())
        {
            //_histGraph->SetOrigin(0.0,0.0);
            if (_histParams.nBin > 40)
                _histGraph->Axis("UAKDTVISO");
            else
                _histGraph->Axis("AKDTVISO");
        }
        else if (!_pData.getBox())
        {
            if (_histParams.nBin > 40)
                _histGraph->Axis("UAKDTVISO");
            else
                _histGraph->Axis("AKDTVISO");
        }
        else
        {
            if (_histParams.nBin > 40)
                _histGraph->Axis("U");
            else
                _histGraph->Axis();
        }
    }

    // Create the surrounding box
    if (_pData.getBox())
        _histGraph->Box();

    // Write the axis labels
    if (_pData.getAxis())
    {
        _histGraph->Label('x', _histParams.sBinLabel.c_str(), 0.0);

        if (sCommonExponent.length() && !_pData.getxLogscale())
        {
            _histGraph->Puts(mglPoint(_histParams.ranges.x[1] + (_histParams.ranges.x[1] - _histParams.ranges.x[0]) / 10.0), mglPoint(_histParams.ranges.x[1] + (_histParams.ranges.x[1] - _histParams.ranges.x[0]) / 10.0 + 1), sCommonExponent.c_str(), ":TL", -1.3);
        }

        if (_pData.getBox())
            _histGraph->Label('y', _histParams.sCountLabel.c_str(), 0.0);
        else
            _histGraph->Label('y', _histParams.sCountLabel.c_str(), 1.1);
    }

    // Create the grid
    if (_pData.getGrid())
    {
        if (_pData.getGrid() == 2)
        {
            _histGraph->Grid("xy!", _pData.getGridStyle().c_str());
            _histGraph->Grid("xy", _pData.getFineGridStyle().c_str());
        }
        else
            _histGraph->Grid("xy", _pData.getGridStyle().c_str());
    }

    // Position the legend
    if (!_pData.getBox())
        _histGraph->Legend(1.25, 1.0);
    else
        _histGraph->Legend(_pData.getLegendPosition());

    std::string sHistSavePath = _out.getFileName();

    // --> Ausgabe-Info-Parameter loeschen und ggf. bFile = FALSE setzen <--

    if (_option.systemPrints() && !bSilent)
        NumeReKernel::printPreFmt(toSystemCodePage("|-> " + _lang.get("HIST_GENERATING_PLOT") + " ... "));

    if (_out.isFile())
        sHistSavePath = sHistSavePath.substr(0, sHistSavePath.length() - 4) + ".png";
    else
        sHistSavePath = _option.ValidFileName("<plotpath>/histogramm", ".png");

    std::string sColor = "";
    nStyle = 0;

    // Create the color definition for the bars
    for (int i = 0; i < _histData.GetNy(); i++)
    {
        sColor += sColorStyles[nStyle];

        if (nStyle == nStyleMax - 1)
            nStyle = 0;
        else
            nStyle++;
    }

    // Create the actual bars
    _histGraph->Bars(_mAxisVals, _histData, sColor.c_str());

    // Open the plot in the graph viewer
    // of write it directly to file
    if (_pData.getOpenImage() && !_pData.getSilentMode() && !bSilent)
    {
        GraphHelper* _graphHelper = new GraphHelper(_histGraph, _pData);
        _graphHelper->setAspect(dAspect);
        NumeReKernel::getInstance()->getWindowManager().createWindow(_graphHelper);
        _histGraph = nullptr;
        if (_option.systemPrints())
            NumeReKernel::printPreFmt(_lang.get("COMMON_SUCCESS") + ".\n");
    }
    else
    {
        _histGraph->WriteFrame(sHistSavePath.c_str());
        delete _histGraph;

        if (_option.systemPrints() && !bSilent)
        {
            NumeReKernel::printPreFmt(_lang.get("COMMON_SUCCESS") + ".\n");
            NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("HIST_SAVED_AT", sHistSavePath), _option) + "\n");
        }
    }

    _out.reset();
}


/////////////////////////////////////////////////
/// \brief This static function is the driver
/// code for creating a 1D histogram.
///
/// \param sCmd const std::string&
/// \param sTargettable const std::string&
/// \param _idx Indices&
/// \param _tIdx Indices&
/// \param _histParams HistogramParameters&
/// \param bWriteToCache bool
/// \param bSilent bool
/// \param bGrid bool
/// \return void
///
/////////////////////////////////////////////////
static void createHist1D(const std::string& sCmd, const std::string& sTargettable, Indices& _idx, Indices& _tIdx, HistogramParameters& _histParams, bool bWriteToCache, bool bSilent, bool bGrid)
{
    PlotData& _pData = NumeReKernel::getInstance()->getPlottingData();
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    int nMax = 0;

    mglData _histData;
    mglData _mAxisVals;

    if (bGrid)
    {
        // x-Range
        prepareIntervalsForHist(sCmd, _histParams.ranges.x[0], _histParams.ranges.x[1],
                                _data.min(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1)).real(),
                                _data.max(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1)).real());

        // y-Range
        prepareIntervalsForHist(sCmd, _histParams.ranges.y[0], _histParams.ranges.y[1],
                                _data.min(_histParams.sTable, _idx.row, _idx.col.subidx(1, 1)).real(),
                                _data.max(_histParams.sTable, _idx.row, _idx.col.subidx(1, 1)).real());

        // z-Range
        prepareIntervalsForHist(sCmd, _histParams.ranges.z[0], _histParams.ranges.z[1],
                                _data.min(_histParams.sTable, _idx.row, _idx.col.subidx(2)).real(),
                                _data.max(_histParams.sTable, _idx.row, _idx.col.subidx(2)).real());
    }
    else
    {
        prepareIntervalsForHist(sCmd, _histParams.ranges.x[0], _histParams.ranges.x[1],
                                _data.min(_histParams.sTable, _idx.row, _idx.col).real(),
                                _data.max(_histParams.sTable, _idx.row, _idx.col).real());
    }

    for (size_t i = 0; i < _idx.row.size(); i++)
    {
        for (size_t j = 2 * bGrid; j < _idx.col.size(); j++)
        {
            if (_data.isValidElement(_idx.row[i], _idx.col[j], _histParams.sTable) && _data.getElement(_idx.row[i], _idx.col[j], _histParams.sTable).real() <= _histParams.ranges.x[1] && _data.getElement(_idx.row[i], _idx.col[j], _histParams.sTable).real() >= _histParams.ranges.x[0])
                nMax++;
        }
    }

    if (!_histParams.nBin && _histParams.binWidth[0] == 0.0)
    {
        if (bGrid)
        {
            if (_histParams.nMethod == STURGES)
                _histParams.nBin = (int)rint(1.0 + 3.3 * log10((double)nMax));
            else if (_histParams.nMethod == SCOTT)
                _histParams.binWidth[0] = 3.49 * _data.std(_histParams.sTable, _idx.row, _idx.col.subidx(2)).real() / pow((double)nMax, 1.0 / 3.0);
            else if (_histParams.nMethod == FREEDMAN_DIACONIS)
                _histParams.binWidth[0] = 2.0 * (_data.pct(_histParams.sTable, _idx.row, _idx.col.subidx(2), 0.75).real() - _data.pct(_histParams.sTable, _idx.row, _idx.col.subidx(2), 0.25).real()) / pow((double)nMax, 1.0 / 3.0);
        }
        else
        {
            if (_histParams.nMethod == STURGES)
                _histParams.nBin = (int)rint(1.0 + 3.3 * log10((double)nMax / (double)(_idx.col.size())));
            else if (_histParams.nMethod == SCOTT)
                _histParams.binWidth[0] = 3.49 * _data.std(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1)).real() / pow((double)nMax / (double)(_idx.col.size()), 1.0 / 3.0);
            else if (_histParams.nMethod == FREEDMAN_DIACONIS)
                _histParams.binWidth[0] = 2.0 * (_data.pct(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1), 0.75).real() - _data.pct(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1), 0.25).real()) / pow((double)nMax / (double)(_idx.col.size()), 1.0 / 3.0);
        }
    }

    // Initialize the mglData objects
    if (_histParams.nBin)
    {
        if (bGrid)
            _histData.Create(_histParams.nBin);
        else
            _histData.Create(_histParams.nBin, _idx.col.size());

        if (_histParams.binWidth[0] == 0.0)
        {
            // --> Berechne die Intervall-Laenge (Explizite Typumwandlung von int->double) <--
            if (bGrid)
            {
                if (_pData.getxLogscale())
                    _histParams.binWidth[0] = (log10(_histParams.ranges.z[1]) - log10(_histParams.ranges.z[0])) / (double)_histParams.nBin;
                else
                    _histParams.binWidth[0] = abs(_histParams.ranges.z[1] - _histParams.ranges.z[0]) / (double)_histParams.nBin;
            }
            else
            {
                if (_pData.getxLogscale())
                    _histParams.binWidth[0] = (log10(_histParams.ranges.x[1]) - log10(_histParams.ranges.x[0])) / (double)_histParams.nBin;
                else
                    _histParams.binWidth[0] = abs(_histParams.ranges.x[1] - _histParams.ranges.x[0]) / (double)_histParams.nBin;
            }
        }
    }
    else
    {
        // --> Gut. Dann berechnen wir daraus die Anzahl der Bins -> Es kann nun aber sein, dass der letzte Bin ueber
        //     das Intervall hinauslaeuft <--
        if (_histParams.binWidth[0] > _histParams.ranges.x[1] - _histParams.ranges.x[0])
            throw SyntaxError(SyntaxError::TOO_LARGE_BINWIDTH, sCmd, SyntaxError::invalid_position);

        for (int i = 0; (i * _histParams.binWidth[0]) + _histParams.ranges.x[0] < _histParams.ranges.x[1] + _histParams.binWidth[0]; i++)
        {
            _histParams.nBin++;
        }

        double dDiff = _histParams.nBin * _histParams.binWidth[0] - (double)(_histParams.ranges.x[1] - _histParams.ranges.x[0]);
        _histParams.ranges.x[0] -= dDiff / 2.0;
        _histParams.ranges.x[1] += dDiff / 2.0;

        if (_histParams.nBin)
        {
            if (bGrid)
                _histData.Create(_histParams.nBin);
            else
                _histData.Create(_histParams.nBin, _idx.col.size());
        }
    }

    _mAxisVals.Create(_histParams.nBin);

    // Calculate the data for the histogram
    std::vector<std::string> vLegends;
    std::vector<std::vector<double>> vHistMatrix = calculateHist1dData(_data, _idx, _histParams, _histData, _mAxisVals, nMax, vLegends, bGrid, _pData.getxLogscale());

    // Create the textual data for the terminal
    // and the file, if necessary
    createOutputForHist1D(_data, _idx, vHistMatrix, _histParams, _mAxisVals, bGrid,
                          !bWriteToCache || findParameter(sCmd, "export", '=') || findParameter(sCmd, "save", '='), bSilent);

    // Store the results into the output table,
    // if desired
    if (bWriteToCache)
    {
        _data.setHeadLineElement(_tIdx.col.front(), sTargettable, "Bins");

        for (size_t i = 0; i < vHistMatrix.size(); i++)
        {
            if (_tIdx.row.size() <= i)
                break;

            _data.writeToTable(_tIdx.row[i], _tIdx.col.front(), sTargettable, _histParams.ranges.x[0] + i * _histParams.binWidth[0] + _histParams.binWidth[0] / 2.0);

            for (size_t j = 0; j < vHistMatrix[0].size(); j++)
            {
                if (_tIdx.col.size() <= j)
                    break;

                if (!i)
                    _data.setHeadLineElement(_tIdx.col[j+1], sTargettable, _data.getHeadLineElement(_idx.col[j], _histParams.sTable));

                _data.writeToTable(_tIdx.row[i], _tIdx.col[j+1], sTargettable, vHistMatrix[i][j]);
            }
        }
    }

    // Create the plot using the calculated data
    createPlotForHist1D(_histParams, _mAxisVals, _histData, vLegends, nMax, bSilent, bGrid);
}


/////////////////////////////////////////////////
/// \brief This static function calculates the
/// data for the center plot part of the 2D
/// histogram.
///
/// \param _data MemoryManager&
/// \param _idx const Indices&
/// \param _histParams const HistogramParameters&
/// \param _hist2DData[3] mglData
/// \return void
///
/////////////////////////////////////////////////
static void calculateDataForCenterPlot(MemoryManager& _data, const Indices& _idx, const HistogramParameters& _histParams, mglData _hist2DData[3])
{
    if (_idx.col.size() == 3)
    {
        for (unsigned int i = 0; i < 3; i++)
        {
            _hist2DData[i].Create(_idx.row.size());
        }

        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            if (_data.isValidElement(_idx.row[i], _idx.col[0], _histParams.sTable)
                    && _data.isValidElement(_idx.row[i], _idx.col[1], _histParams.sTable)
                    && _data.isValidElement(_idx.row[i], _idx.col[2], _histParams.sTable)
                    && _data.getElement(_idx.row[i], _idx.col[0], _histParams.sTable).real() <= _histParams.ranges.x[1]
                    && _data.getElement(_idx.row[i], _idx.col[0], _histParams.sTable).real() >= _histParams.ranges.x[0]
                    && _data.getElement(_idx.row[i], _idx.col[1], _histParams.sTable).real() <= _histParams.ranges.y[1]
                    && _data.getElement(_idx.row[i], _idx.col[1], _histParams.sTable).real() >= _histParams.ranges.y[0])
            {
                _hist2DData[0].a[i] = _data.getElement(_idx.row[i], _idx.col[0], _histParams.sTable).real();
                _hist2DData[1].a[i] = _data.getElement(_idx.row[i], _idx.col[1], _histParams.sTable).real();
                _hist2DData[2].a[i] = _data.getElement(_idx.row[i], _idx.col[2], _histParams.sTable).real();
            }
            else
            {
                for (unsigned int k = 0; k < 3; k++)
                    _hist2DData[k].a[i] = NAN;
            }
        }
    }
    else
    {
        _hist2DData[0].Create(_idx.row.size());
        _hist2DData[1].Create(_idx.row.size());
        _hist2DData[2].Create(_idx.row.size(), _idx.col.size() - 2);

        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            _hist2DData[0].a[i] = _data.getElement(_idx.row[i], _idx.col[0], _histParams.sTable).real();
            _hist2DData[1].a[i] = _data.getElement(_idx.row[i], _idx.col[1], _histParams.sTable).real();

            for (size_t j = 0; j < _idx.col.size() - 2; j++)
            {
                if (_data.isValidElement(_idx.row[i], _idx.col[j + 2], _histParams.sTable)
                        && _data.getElement(_idx.row[i], _idx.col[0], _histParams.sTable).real() <= _histParams.ranges.x[1]
                        && _data.getElement(_idx.row[i], _idx.col[0], _histParams.sTable).real() >= _histParams.ranges.x[0]
                        && _data.getElement(_idx.row[i], _idx.col[1], _histParams.sTable).real() <= _histParams.ranges.y[1]
                        && _data.getElement(_idx.row[i], _idx.col[1], _histParams.sTable).real() >= _histParams.ranges.y[0])
                    _hist2DData[2].a[i + j * _idx.row.size()] = _data.getElement(_idx.row[i], _idx.col[j + 2], _histParams.sTable).real();
                else
                    _hist2DData[2].a[i + j * _idx.row.size()] = NAN;
            }
        }

    }
}


/////////////////////////////////////////////////
/// \brief This static function calculates the
/// data for the both bar plot on top and at
/// right of the center plot. Has to be called
/// for each plot indepently.
///
/// \param _data MemoryManager&
/// \param _idx const Indices&
/// \param _histParams const HistogramParameters&
/// \param _mAxisVals mglData*
/// \param dBinMin double
/// \param dMin double
/// \param dMax double
/// \param dIntLength double
/// \param nMax int
/// \param isLogScale bool
/// \param isHbar bool
/// \param bSum bool
/// \return mglData
///
/////////////////////////////////////////////////
static mglData calculateXYHist(MemoryManager& _data, const Indices& _idx, const HistogramParameters& _histParams, mglData* _mAxisVals, double dBinMin, double dMin, double dMax, double dIntLength, int nMax, bool isLogScale, bool isHbar, bool bSum)
{
    mglData _histData(_histParams.nBin);

    for (int k = 0; k < _histParams.nBin; k++)
    {
        double dSum = 0.0;

        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            if (_data.isValidElement(_idx.row[i], _idx.col[isHbar], _histParams.sTable)
                    && ((!isLogScale
                         && _data.getElement(_idx.row[i], _idx.col[isHbar], _histParams.sTable).real() >= dBinMin + k * dIntLength
                         && _data.getElement(_idx.row[i], _idx.col[isHbar], _histParams.sTable).real() < dBinMin + (k + 1)*dIntLength)
                        || (isLogScale
                            && _data.getElement(_idx.row[i], _idx.col[isHbar], _histParams.sTable).real() >= pow(10.0, log10(dBinMin) + k * dIntLength)
                            && _data.getElement(_idx.row[i], _idx.col[isHbar], _histParams.sTable).real() < pow(10.0, log10(dBinMin) + (k + 1)*dIntLength))
                       )
               )
            {
                if (_idx.col.size() == 3)
                {
                    if (_data.isValidElement(_idx.row[i], _idx.col[!isHbar], _histParams.sTable)
                            && _data.isValidElement(_idx.row[i], _idx.col[2], _histParams.sTable)
                            && _data.getElement(_idx.row[i], _idx.col[2], _histParams.sTable).real() >= _histParams.ranges.z[0]
                            && _data.getElement(_idx.row[i], _idx.col[2], _histParams.sTable).real() <= _histParams.ranges.z[1]
                            && _data.getElement(_idx.row[i], _idx.col[!isHbar], _histParams.sTable).real() >= dMin
                            && _data.getElement(_idx.row[i], _idx.col[!isHbar], _histParams.sTable).real() <= dMax)
                    {
                        if (bSum)
                            dSum += _data.getElement(_idx.row[i], _idx.col[2], _histParams.sTable).real();
                        else
                            dSum++;
                    }
                }
                else
                {
                    for (size_t l = 0; l < _idx.row.size(); l++)
                    {
                        if (_data.isValidElement(_idx.row[l], _idx.col[!isHbar], _histParams.sTable)
                                && _data.isValidElement(_idx.row[(!isHbar*i+isHbar*l)], _idx.col[(!isHbar*l+isHbar*i)+2], _histParams.sTable)
                                && _data.getElement(_idx.row[(!isHbar*i+isHbar*l)], _idx.col[(!isHbar*l+isHbar*i)+2], _histParams.sTable).real() >= _histParams.ranges.z[0]
                                && _data.getElement(_idx.row[(!isHbar*i+isHbar*l)], _idx.col[(!isHbar*l+isHbar*i)+2], _histParams.sTable).real() <= _histParams.ranges.z[1]
                                && _data.getElement(_idx.row[l], _idx.col[!isHbar], _histParams.sTable).real() >= dMin
                                && _data.getElement(_idx.row[l], _idx.col[!isHbar], _histParams.sTable).real() <= dMax)
                            dSum += _data.getElement(_idx.row[(!isHbar*i+isHbar*l)], _idx.col[(!isHbar*l+isHbar*i)+2], _histParams.sTable).real();
                    }
                }
            }
        }

        _histData.a[k] = dSum;

        if (!isLogScale)
            _mAxisVals->a[k] = dBinMin + (k + 0.5) * dIntLength;
        else
            _mAxisVals->a[k] = pow(10.0, log10(dBinMin) + (k + 0.5) * dIntLength);
    }

    return _histData;
}


/////////////////////////////////////////////////
/// \brief This static function creates the
/// textual output for terminal or file and
/// writes the data also to a table, if desired.
///
/// \param _data MemoryManager&
/// \param _idx const Indices&
/// \param sTargettable const std::string&
/// \param _tIdx const Indices&
/// \param _histParams const HistogramParameters&
/// \param _mAxisVals[2] mglData
/// \param _barHistData mglData&
/// \param _hBarHistData mglData&
/// \param bSum bool
/// \param bWriteToCache bool
/// \param shallFormat bool
/// \param isSilent bool
/// \return void
///
/////////////////////////////////////////////////
static void createOutputForHist2D(MemoryManager& _data, const Indices& _idx, const std::string& sTargettable, const Indices& _tIdx, const HistogramParameters& _histParams, mglData _mAxisVals[2], mglData& _barHistData, mglData& _hBarHistData, bool bSum, bool bWriteToCache, bool shallFormat, bool isSilent)
{
    Output& _out = NumeReKernel::getInstance()->getOutput();
    const Settings& _option = NumeReKernel::getInstance()->getSettings();

    std::string** sOut = new std::string*[_histParams.nBin + 1];

    for (int k = 0; k < _histParams.nBin + 1; k++)
        sOut[k] = new std::string[4];

    // Fill output tables
    for (int k = 0; k < _histParams.nBin; k++)
    {
        sOut[k + 1][0] = toString(_mAxisVals[0].a[k], _option);
        sOut[k + 1][1] = toString(_barHistData.a[k], _option);
        sOut[k + 1][2] = toString(_mAxisVals[1].a[k], _option);
        sOut[k + 1][3] = toString(_hBarHistData.a[k], _option);

        if (!k)
        {
            sOut[k][0] = "Bins_[x]";
            sOut[k][1] = (bSum ? "Sum_[x]" : "Counts_[x]");
            sOut[k][2] = "Bins_[y]";
            sOut[k][3] = (bSum ? "Sum_[y]" : "Counts_[y]");
        }

        if (bWriteToCache)
        {
            if (_tIdx.row.size() <= (size_t)k || _tIdx.col.size() < 4)
                continue;

            if (!k)
            {
                _data.setHeadLineElement(_tIdx.col[0], sTargettable, "Bins [x]");
                _data.setHeadLineElement(_tIdx.col[1], sTargettable, (bSum ? "Sum [x]" : "Counts [x]"));
                _data.setHeadLineElement(_tIdx.col[2], sTargettable, "Bins [y]");
                _data.setHeadLineElement(_tIdx.col[3], sTargettable, (bSum ? "Sum [y]" : "Counts [y]"));
            }

            _data.writeToTable(_tIdx.row[k], _tIdx.col[0], sTargettable, _mAxisVals[0].a[k]);
            _data.writeToTable(_tIdx.row[k], _tIdx.col[1], sTargettable, _barHistData.a[k]);
            _data.writeToTable(_tIdx.row[k], _tIdx.col[2], sTargettable, _mAxisVals[1].a[k]);
            _data.writeToTable(_tIdx.row[k], _tIdx.col[3], sTargettable, _hBarHistData.a[k]);
        }
    }

    if (shallFormat)
    {
        _out.setPluginName("2D-" + _lang.get("HIS_OUT_PLGNINFO", PI_HIST, toString(_idx.col.front()+1), toString(_idx.col.last()+1), _data.getDataFileName(_histParams.sTable)));
        _out.setCommentLine(_lang.get("HIST_OUT_COMMENTLINE2D", toString(_histParams.ranges.x[0], 5), toString(_histParams.ranges.x[1], 5), toString(_histParams.binWidth[0], 5), toString(_histParams.ranges.y[0], 5), toString(_histParams.ranges.y[1], 5), toString(_histParams.binWidth[1], 5)));

        _out.setPrefix("hist2d");
    }

    if (_out.isFile())
    {
        _out.setStatus(true);
        _out.setCompact(false);

        if (_histParams.sSavePath.length())
            _out.setFileName(_histParams.sSavePath);
        else
            _out.generateFileName();
    }
    else
        _out.setCompact(_option.createCompactTables());

    if (shallFormat)
    {
        if (_out.isFile() || !isSilent)
        {
            if (!_out.isFile())
            {
                NumeReKernel::toggleTableStatus();
                make_hline();
                NumeReKernel::print("NUMERE: 2D-" + toSystemCodePage(toUpperCase(_lang.get("HIST_HEADLINE"))));
                make_hline();
            }

            _out.format(sOut, 4, _histParams.nBin + 1, _option, true);

            if (!_out.isFile())
            {
                NumeReKernel::toggleTableStatus();
                make_hline();
            }
        }
    }


    for (int k = 0; k < _histParams.nBin + 1; k++)
        delete[] sOut[k];
    delete[] sOut;
}


/////////////////////////////////////////////////
/// \brief This static function creates the three
/// plots for the 2D histogram.
///
/// \param sCmd const std::string&
/// \param _histParams HistogramParameters&
/// \param _mAxisVals[2] mglData
/// \param _barHistData mglData&
/// \param _hBarHistData mglData&
/// \param _hist2DData[3] mglData
/// \param isScatterPlot bool
/// \param bSum bool
/// \param bSilent bool
/// \return void
///
/////////////////////////////////////////////////
static void createPlotsForHist2D(const std::string& sCmd, HistogramParameters& _histParams, mglData _mAxisVals[2], mglData& _barHistData, mglData& _hBarHistData, mglData _hist2DData[3], bool isScatterPlot, bool bSum, bool bSilent)
{
    PlotData& _pData = NumeReKernel::getInstance()->getPlottingData();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Output& _out = NumeReKernel::getInstance()->getOutput();

    double dAspect = 4.0 / 3.0;

    mglGraph* _histGraph = prepareGraphForHist(dAspect, _pData, bSilent);

/////////////////////////////////// BAR PLOT
    _histGraph->MultiPlot(3, 3, 0, 2, 1, "<>");
    _histGraph->SetBarWidth(0.9);
    _histGraph->SetTuneTicks(3, 1.05);
    _histParams.binWidth[0] = _barHistData.Maximal() - _barHistData.Minimal();
    _histGraph->SetRanges(1, 2, 1, 2, 1, 2);

    if (_pData.getxLogscale() && !_pData.getyLogscale() && !_pData.getzLogscale())
        _histGraph->SetFunc("lg(x)", "");
    else if (_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale())
        _histGraph->SetFunc("lg(x)", "");
    else if (!_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale())
        _histGraph->SetFunc("", "");
    else if (_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale())
        _histGraph->SetFunc("lg(x)", "lg(y)");
    else if (!_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale())
        _histGraph->SetFunc("", "lg(y)");
    else if (!_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale())
        _histGraph->SetFunc("", "lg(y)");
    else if (_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale())
        _histGraph->SetFunc("lg(x)", "lg(y)");

    if (_barHistData.Minimal() >= 0)
    {
        if (_pData.getzLogscale() && _barHistData.Maximal() > 0.0)
        {
            if (_barHistData.Minimal() - _histParams.binWidth[0] / 20.0 > 0)
                _histGraph->SetRanges(_histParams.ranges.x[0], _histParams.ranges.x[1], _barHistData.Minimal() - _histParams.binWidth[0] / 20.0, _barHistData.Maximal() + _histParams.binWidth[0] / 20.0);
            else if (_barHistData.Minimal() > 0)
                _histGraph->SetRanges(_histParams.ranges.x[0], _histParams.ranges.x[1], _barHistData.Minimal() / 2.0, _barHistData.Maximal() + _histParams.binWidth[0] / 20.0);
            else
                _histGraph->SetRanges(_histParams.ranges.x[0], _histParams.ranges.x[1], (1e-2 * _barHistData.Maximal() < 1e-2 ? 1e-2 * _barHistData.Maximal() : 1e-2), _barHistData.Maximal() + _histParams.binWidth[0] / 20.0);
        }
        else if (_barHistData.Maximal() < 0.0 && _pData.getzLogscale())
        {
            delete _histGraph;
            throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCmd, SyntaxError::invalid_position);
        }
        else
            _histGraph->SetRanges(_histParams.ranges.x[0], _histParams.ranges.x[1], 0.0, _barHistData.Maximal() + _histParams.binWidth[0] / 20.0);
    }
    else
        _histGraph->SetRanges(_histParams.ranges.x[0], _histParams.ranges.x[1], _barHistData.Minimal() - _histParams.binWidth[0] / 20.0, _barHistData.Maximal() + _histParams.binWidth[0] / 20.0);

    _histGraph->Box();
    _histGraph->Axis();

    if (_pData.getGrid() == 1)
        _histGraph->Grid("xy", _pData.getGridStyle().c_str());
    else if (_pData.getGrid() == 2)
    {
        _histGraph->Grid("xy!", _pData.getGridStyle().c_str());
        _histGraph->Grid("xy", _pData.getFineGridStyle().c_str());
    }

    _histGraph->Label('x', _histParams.sAxisLabels[0].c_str(), 0);

    if (bSum)
        _histGraph->Label('y', _histParams.sAxisLabels[2].c_str(), 0);
    else
        _histGraph->Label('y', _histParams.sCountLabel.c_str(), 0);

    _histGraph->Bars(_mAxisVals[0], _barHistData, _pData.getColors().c_str());

///////////////////////////////////// CENTER GRAPH
    _histGraph->MultiPlot(3, 3, 3, 2, 2, "<_>");
    _histGraph->SetTuneTicks(3, 1.05);

    _histGraph->SetRanges(1, 2, 1, 2, 1, 2);

    if (_pData.getxLogscale() && !_pData.getyLogscale() && !_pData.getzLogscale())
        _histGraph->SetFunc("lg(x)", "");
    else if (_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale())
        _histGraph->SetFunc("lg(x)", "lg(y)");
    else if (!_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale())
        _histGraph->SetFunc("", "lg(y)");
    else if (_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale())
        _histGraph->SetFunc("lg(x)", "", "lg(z)");
    else if (!_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale())
        _histGraph->SetFunc("", "", "lg(z)");
    else if (!_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale())
        _histGraph->SetFunc("", "lg(y)", "lg(z)");
    else if (_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale())
        _histGraph->SetFunc("lg(x)", "lg(y)", "lg(z)");

    if (_hist2DData[2].Maximal() < 0 && _pData.getzLogscale())
    {
        delete _histGraph;
        throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCmd, SyntaxError::invalid_position);
    }

    if (_pData.getzLogscale())
    {
        if (_hist2DData[2].Minimal() > 0.0)
            _histGraph->SetRanges(_histParams.ranges.x[0], _histParams.ranges.x[1], _histParams.ranges.y[0], _histParams.ranges.y[1], _hist2DData[2].Minimal(), _hist2DData[2].Maximal());
        else
            _histGraph->SetRanges(_histParams.ranges.x[0], _histParams.ranges.x[1], _histParams.ranges.y[0], _histParams.ranges.y[1], (1e-2 * _hist2DData[2].Maximal() < 1e-2 ? 1e-2 * _hist2DData[2].Maximal() : 1e-2), _hist2DData[2].Maximal());
    }
    else
        _histGraph->SetRanges(_histParams.ranges.x[0], _histParams.ranges.x[1], _histParams.ranges.y[0], _histParams.ranges.y[1], _hist2DData[2].Minimal(), _hist2DData[2].Maximal());

    _histGraph->Box();
    _histGraph->Axis("xy");
    _histGraph->Colorbar(_pData.getColorScheme("I>").c_str());

    if (_pData.getGrid() == 1)
        _histGraph->Grid("xy", _pData.getGridStyle().c_str());
    else if (_pData.getGrid() == 2)
    {
        _histGraph->Grid("xy!", _pData.getGridStyle().c_str());
        _histGraph->Grid("xy", _pData.getFineGridStyle().c_str());
    }

    _histGraph->Label('x', _histParams.sAxisLabels[0].c_str(), 0);
    _histGraph->Label('y', _histParams.sAxisLabels[1].c_str(), 0);

    if (isScatterPlot)
        _histGraph->Dots(_hist2DData[0], _hist2DData[1], _hist2DData[2], _pData.getColorScheme().c_str());
    else
        _histGraph->Dens(_hist2DData[0], _hist2DData[1], _hist2DData[2], _pData.getColorScheme().c_str());

/////////////////////////////// H-BAR PLOT
    _histGraph->MultiPlot(3, 3, 5, 1, 2, "_");
    _histGraph->SetBarWidth(0.9);
    _histGraph->SetTuneTicks(3, 1.05);
    _histParams.binWidth[0] = _hBarHistData.Maximal() - _hBarHistData.Minimal();

    _histGraph->SetRanges(1, 2, 1, 2, 1, 2);

    if (_pData.getxLogscale() && !_pData.getyLogscale() && !_pData.getzLogscale())
        _histGraph->SetFunc("", "");
    else if (_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale())
        _histGraph->SetFunc("", "lg(y)");
    else if (!_pData.getxLogscale() && _pData.getyLogscale() && !_pData.getzLogscale())
        _histGraph->SetFunc("", "lg(y)");
    else if (_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale())
        _histGraph->SetFunc("lg(x)", "");
    else if (!_pData.getxLogscale() && !_pData.getyLogscale() && _pData.getzLogscale())
        _histGraph->SetFunc("lg(x)", "");
    else if (!_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale())
        _histGraph->SetFunc("lg(x)", "lg(y)");
    else if (_pData.getxLogscale() && _pData.getyLogscale() && _pData.getzLogscale())
        _histGraph->SetFunc("lg(x)", "lg(y)");

    if (_hBarHistData.Minimal() >= 0)
    {
        if (_pData.getzLogscale() && _hBarHistData.Maximal() > 0.0)
        {
            if (_hBarHistData.Minimal() - _histParams.binWidth[0] / 20.0 > 0)
                _histGraph->SetRanges(_hBarHistData.Minimal() - _histParams.binWidth[0] / 20.0, _hBarHistData.Maximal() + _histParams.binWidth[0] / 20.0, _histParams.ranges.y[0], _histParams.ranges.y[1]);
            else if (_hBarHistData.Minimal() > 0)
                _histGraph->SetRanges(_hBarHistData.Minimal() / 2.0, _hBarHistData.Maximal() + _histParams.binWidth[0] / 20.0, _histParams.ranges.y[0], _histParams.ranges.y[1]);
            else
                _histGraph->SetRanges((1e-2 * _hBarHistData.Maximal() < 1e-2 ? 1e-2 * _hBarHistData.Maximal() : 1e-2), _hBarHistData.Maximal() + _histParams.binWidth[0] / 10.0, _histParams.ranges.y[0], _histParams.ranges.y[1]);
        }
        else if (_hBarHistData.Maximal() < 0.0 && _pData.getzLogscale())
        {
            delete _histGraph;
            throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCmd, SyntaxError::invalid_position);
        }
        else
            _histGraph->SetRanges(0.0, _hBarHistData.Maximal() + _histParams.binWidth[0] / 20.0, _histParams.ranges.y[0], _histParams.ranges.y[1]);
    }
    else
        _histGraph->SetRanges(_hBarHistData.Minimal() - _histParams.binWidth[0] / 20.0, _hBarHistData.Maximal() + _histParams.binWidth[0] / 20.0, _histParams.ranges.y[0], _histParams.ranges.y[1]);

    _histGraph->Box();
    _histGraph->Axis("xy");

    if (_pData.getGrid() == 1)
        _histGraph->Grid("xy", _pData.getGridStyle().c_str());
    else if (_pData.getGrid() == 2)
    {
        _histGraph->Grid("xy!", _pData.getGridStyle().c_str());
        _histGraph->Grid("xy", _pData.getFineGridStyle().c_str());
    }

    if (!bSum)
        _histGraph->Label('x', _histParams.sCountLabel.c_str(), 0);
    else
        _histGraph->Label('x', _histParams.sAxisLabels[2].c_str(), 0);

    _histGraph->Label('y', _histParams.sAxisLabels[1].c_str(), 0);
    _histGraph->Barh(_mAxisVals[1], _hBarHistData, _pData.getColors().c_str());

////////////////////////////////// OUTPUT
    std::string sHistSavePath = _out.getFileName();

    if (_option.systemPrints() && !bSilent)
        NumeReKernel::printPreFmt(toSystemCodePage("|-> " + _lang.get("HIST_GENERATING_PLOT") + " ... "));

    if (_out.isFile())
        sHistSavePath = sHistSavePath.substr(0, sHistSavePath.length() - 4) + ".png";
    else
        sHistSavePath = _option.ValidFileName("<plotpath>/histogramm2d", ".png");

    if (_pData.getOpenImage() && !_pData.getSilentMode() && !bSilent)
    {
        GraphHelper* _graphHelper = new GraphHelper(_histGraph, _pData);
        _graphHelper->setAspect(dAspect);
        NumeReKernel::getInstance()->getWindowManager().createWindow(_graphHelper);
        _histGraph = nullptr;

        if (_option.systemPrints())
            NumeReKernel::printPreFmt(_lang.get("COMMON_SUCCESS") + ".\n");
    }
    else
    {
        _histGraph->WriteFrame(sHistSavePath.c_str());
        delete _histGraph;

        if (_option.systemPrints() && !bSilent)
            NumeReKernel::printPreFmt(_lang.get("COMMON_SUCCESS") + ".\n");
        if (!_out.isFile() && _option.systemPrints() && !bSilent)
            NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("HIST_SAVED_AT", sHistSavePath), _option) + "\n");
    }
}


/////////////////////////////////////////////////
/// \brief This static function is the driver
/// function for creating a 2D histogram.
///
/// \param sCmd const std::string&
/// \param sTargettable const std::string&
/// \param _idx Indices&
/// \param _tIdx Indices&
/// \param _histParams HistogramParameters&
/// \param bSum bool
/// \param bWriteToCache bool
/// \param bSilent bool
/// \return void
///
/////////////////////////////////////////////////
static void createHist2D(const std::string& sCmd, const std::string& sTargettable, Indices& _idx, Indices& _tIdx, HistogramParameters& _histParams, bool bSum, bool bWriteToCache, bool bSilent)
{
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    PlotData& _pData = NumeReKernel::getInstance()->getPlottingData();
    Output& _out = NumeReKernel::getInstance()->getOutput();

    int nMax = 0;

    mglData _hist2DData[3];
    mglData _mAxisVals[2];

    if (_idx.col.size() < 3)
        throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, SyntaxError::invalid_position);

    if (_idx.col.size() > 3)
        bSum = true;

    // Update the interval ranges using the minimal and maximal
    // data values in the corresponding spatial directions
    prepareIntervalsForHist(sCmd, _histParams.ranges.x[0], _histParams.ranges.x[1],
                            _data.min(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1)).real(),
                            _data.max(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1)).real());

    prepareIntervalsForHist(sCmd, _histParams.ranges.y[0], _histParams.ranges.y[1],
                            _data.min(_histParams.sTable, _idx.row, _idx.col.subidx(1, 1)).real(),
                            _data.max(_histParams.sTable, _idx.row, _idx.col.subidx(1, 1)).real());

    prepareIntervalsForHist(sCmd, _histParams.ranges.z[0], _histParams.ranges.z[1],
                            _data.min(_histParams.sTable, _idx.row, _idx.col.subidx(2)).real(),
                            _data.max(_histParams.sTable, _idx.row, _idx.col.subidx(2)).real());

    // Adapt the ranges for logscale
    if (_pData.getxLogscale() && _histParams.ranges.x[1] < 0.0)
        throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCmd, SyntaxError::invalid_position);
    else if (_pData.getxLogscale())
    {
        if (_histParams.ranges.x[0] < 0.0)
            _histParams.ranges.x[0] = (1e-2 * _histParams.ranges.x[1] < 1e-2 ? 1e-2 * _histParams.ranges.x[1] : 1e-2);
    }

    if (_pData.getyLogscale() && _histParams.ranges.y[1] < 0.0)
        throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCmd, SyntaxError::invalid_position);
    else if (_pData.getyLogscale())
    {
        if (_histParams.ranges.y[0] < 0.0)
            _histParams.ranges.y[0] = (1e-2 * _histParams.ranges.y[1] < 1e-2 ? 1e-2 * _histParams.ranges.y[1] : 1e-2);
    }

    // Count the number of valid entries for determining
    // the number of bins based upon the methods automatically
    for (size_t i = 0; i < _idx.row.size(); i++)
    {
        for (size_t j = 2; j < _idx.col.size(); j++)
        {
            if (_data.isValidElement(_idx.row[i], _idx.col[j], _histParams.sTable)
                    && _data.getElement(_idx.row[i], _idx.col[0], _histParams.sTable).real() <= _histParams.ranges.x[1]
                    && _data.getElement(_idx.row[i], _idx.col[0], _histParams.sTable).real() >= _histParams.ranges.x[0]
                    && _data.getElement(_idx.row[i], _idx.col[1], _histParams.sTable).real() <= _histParams.ranges.y[1]
                    && _data.getElement(_idx.row[i], _idx.col[1], _histParams.sTable).real() >= _histParams.ranges.y[0])
                nMax++;
        }
    }

    // Determine the number of bins based upon the
    // selected method
    if (!_histParams.nBin && _histParams.binWidth[0] == 0.0)
    {
        if (_histParams.nMethod == STURGES)
            _histParams.nBin = (int)rint(1.0 + 3.3 * log10((double)nMax / (double)(_idx.col.size()-2)));
        else if (_histParams.nMethod == SCOTT)
            _histParams.binWidth[0] = 3.49 * _data.std(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1)).real() / pow((double)nMax / (double)(_idx.col.size()), 1.0 / 3.0);
        else if (_histParams.nMethod == FREEDMAN_DIACONIS)
           _histParams.binWidth[0] = 2.0 * (_data.pct(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1), 0.75).real() - _data.pct(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1), 0.25).real()) / pow((double)nMax / (double)(_idx.col.size()), 1.0 / 3.0);
    }

    // Determine the number of bins based upon the
    // selected bin interval width
    if (!_histParams.nBin)
    {
        if (_histParams.binWidth[0] > _histParams.ranges.x[1] - _histParams.ranges.x[0])
            throw SyntaxError(SyntaxError::TOO_LARGE_BINWIDTH, sCmd, SyntaxError::invalid_position);

        for (int i = 0; (i * _histParams.binWidth[0]) + _histParams.ranges.x[0] < _histParams.ranges.x[1] + _histParams.binWidth[0]; i++)
        {
            _histParams.nBin++;
        }

        double dDiff = _histParams.nBin * _histParams.binWidth[0] - (double)(_histParams.ranges.x[1] - _histParams.ranges.x[0]);
        _histParams.ranges.x[0] -= dDiff / 2.0;
        _histParams.ranges.x[1] += dDiff / 2.0;
    }

    _mAxisVals[0].Create(_histParams.nBin);
    _mAxisVals[1].Create(_histParams.nBin);

    // Determine the bin interval width
    if (_histParams.binWidth[0] == 0.0)
    {
        // --> Berechne die Intervall-Laenge (Explizite Typumwandlung von int->double) <--
        if (_pData.getxLogscale())
            _histParams.binWidth[0] = (log10(_histParams.ranges.x[1]) - log10(_histParams.ranges.x[0])) / (double)_histParams.nBin;
        else
            _histParams.binWidth[0] = abs(_histParams.ranges.x[1] - _histParams.ranges.x[0]) / (double)_histParams.nBin;
    }

    // Determine the bin interval width in
    // y direction
    if (_pData.getyLogscale())
        _histParams.binWidth[1] = (log10(_histParams.ranges.y[1]) - log10(_histParams.ranges.y[0])) / (double)_histParams.nBin;
    else
        _histParams.binWidth[1] = abs(_histParams.ranges.y[1] - _histParams.ranges.y[0]) / (double)_histParams.nBin;

    // Calculate the necessary data for the center plot
    calculateDataForCenterPlot(_data, _idx, _histParams, _hist2DData);

    // Calculate the necessary data for the bar chart
    // along the y axis of the central plot
    mglData _barHistData = calculateXYHist(_data, _idx, _histParams, &_mAxisVals[0],
                                           _histParams.ranges.x[0], _histParams.ranges.y[0], _histParams.ranges.y[1], _histParams.binWidth[0],
                                           nMax, _pData.getxLogscale(), false, bSum);

    // Calculate the necessary data for the horizontal
    // bar chart along the x axis of the central plot
    mglData _hBarHistData = calculateXYHist(_data, _idx, _histParams, &_mAxisVals[1],
                                            _histParams.ranges.y[0], _histParams.ranges.x[0], _histParams.ranges.x[1], _histParams.binWidth[1],
                                            nMax, _pData.getyLogscale(), true, bSum);

    // Format the data for the textual output for the
    // terminal, the text file and store the result in
    // the target table, if desired
    createOutputForHist2D(_data, _idx, sTargettable, _tIdx, _histParams, _mAxisVals, _barHistData, _hBarHistData, bSum, bWriteToCache,
                          !bWriteToCache || findParameter(sCmd, "export", '=') || findParameter(sCmd, "save", '='),
                          !_option.systemPrints() || bSilent);

    // Create the three plots as subplots
    createPlotsForHist2D(sCmd, _histParams, _mAxisVals, _barHistData, _hBarHistData, _hist2DData, _idx.col.size() == 3, bSum, bSilent);

    _out.reset();
}


/////////////////////////////////////////////////
/// \brief This function is the interface to both
/// the 1D and the 2D histogram generation.
///
/// \param sCmd std::string&
/// \return void
///
/////////////////////////////////////////////////
void plugin_histogram(std::string& sCmd)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Output& _out = NumeReKernel::getInstance()->getOutput();

	if (!_data.isValid())			// Sind ueberhaupt Daten vorhanden?
        throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);

    bool bWriteToCache = false;
    bool bMake2DHist = false;
    bool bSum = false;
    bool bSilent = false;
    bool bGrid = false;


    HistogramParameters _histParams;

    _histParams.sTable = "data";
    _histParams.sAxisLabels[0] = "\\i x";
    _histParams.sAxisLabels[1] = "\\i y";
    _histParams.sAxisLabels[2] = "\\i z";
    _histParams.nMethod = STURGES;
    _histParams.ranges.x[0] = NAN;
    _histParams.ranges.x[1] = NAN;
    _histParams.ranges.y[0] = NAN;
    _histParams.ranges.y[1] = NAN;
    _histParams.ranges.z[0] = NAN;
    _histParams.ranges.z[1] = NAN;

    Indices _idx;
    Indices _tIdx;

    if (_data.matchTableAsParameter(sCmd).length())
        _histParams.sTable = _data.matchTableAsParameter(sCmd);
    else
    {
        DataAccessParser _accessParser(sCmd);

        if (!_accessParser.getDataObject().length())
            throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, SyntaxError::invalid_position);

        _accessParser.evalIndices();
        _histParams.sTable = _accessParser.getDataObject();
        _idx = _accessParser.getIndices();
    }

    if (_data.isEmpty(_histParams.sTable))
        throw SyntaxError(SyntaxError::NO_CACHED_DATA, sCmd, SyntaxError::invalid_position);

    if (findCommand(sCmd).sString == "hist2d")
        bMake2DHist = true;

    if ((findParameter(sCmd, "cols", '=') || findParameter(sCmd, "c", '=')) && !isValidIndexSet(_idx))
    {
        long long int nDataRow = VectorIndex::INVALID;
        long long int nDataRowFinal = 0;

        int nPos = 0;

        if (findParameter(sCmd, "cols", '='))
            nPos = findParameter(sCmd, "cols", '=') + 4;
        else
            nPos = findParameter(sCmd, "c", '=') + 1;

        std::string sTemp = getArgAtPos(sCmd, nPos);
        std::string sTemp_2 = "";
        StripSpaces(sTemp);

        if (sTemp.find(':') != std::string::npos)
        {
            int nSep = 0;

            for (unsigned int i = 0; i < sTemp.length(); i++)
            {
                if (sTemp[i] == ':')
                {
                    nSep = i;
                    break;
                }
            }

            sTemp_2 = sTemp.substr(0, nSep);
            sTemp = sTemp.substr(nSep + 1);
            StripSpaces(sTemp);
            StripSpaces(sTemp_2);

            if (sTemp_2.length())
                nDataRow = intCast(StrToDb(sTemp_2));
            else
                nDataRow = 1;

            if (sTemp.length())
            {
                nDataRowFinal = intCast(StrToDb(sTemp));

                if (nDataRowFinal > _data.getCols(_histParams.sTable) || !nDataRowFinal)
                    nDataRowFinal = _data.getCols(_histParams.sTable);
            }
            else
                nDataRowFinal = _data.getCols(_histParams.sTable);
        }
        else if (sTemp.length())
            nDataRow = intCast(StrToDb(sTemp));

        _idx.row = VectorIndex(0, _data.getLines(_histParams.sTable)-1);
        _idx.col = VectorIndex(nDataRow, nDataRowFinal-1);
    }

    _histParams.nBin = intCast(StrToDb(getParameterValue(sCmd, "bins", "b", "0")));
    _histParams.binWidth[0] = StrToDb(getParameterValue(sCmd, "width", "w", "0"));
    std::string sTargettable = getParameterValue(sCmd, "tocache", "totable", "");

    if (sTargettable.length())
    {
        bWriteToCache = true;

        if (sTargettable.find('(') == std::string::npos)
            sTargettable += "()";

        if (!_data.isTable(sTargettable))
            _data.addTable(sTargettable, NumeReKernel::getInstance()->getSettings());

        sTargettable.erase(sTargettable.find('('));
        _tIdx.row = VectorIndex(0, VectorIndex::OPEN_END);
        _tIdx.col = VectorIndex(_data.getCols(sTargettable), VectorIndex::OPEN_END);
    }
    else
    {
        sTargettable = evaluateTargetOptionInCommand(sCmd, sTargettable, _tIdx, NumeReKernel::getInstance()->getParser(), _data, NumeReKernel::getInstance()->getSettings());

        if (sTargettable.length())
            bWriteToCache = true;
    }

    if (findParameter(sCmd, "tocache") || findParameter(sCmd, "totable"))
    {
        bWriteToCache = true;

        if (!sTargettable.length())
        {
            sTargettable = "table";
            _tIdx.row = VectorIndex(0, VectorIndex::OPEN_END);
            _tIdx.col = VectorIndex(_data.getCols("table"), VectorIndex::OPEN_END);
        }
    }

    _histParams.sSavePath = getParameterValue(sCmd, "save", "export", "");

    if (findParameter(sCmd, "save") || findParameter(sCmd, "export"))
        _out.setStatus(true);

    if (_histParams.sSavePath.length())
        _out.setStatus(true);

    if (!bMake2DHist)
    {
        _histParams.sBinLabel = getParameterValue(sCmd, "xlabel", "binlabel", "Bins");
        _histParams.sCountLabel = getParameterValue(sCmd, "ylabel", "countlabel", "Counts");
    }
    else
    {
        _histParams.sBinLabel = getParameterValue(sCmd, "binlabel", "binlabel", "Bins");
        _histParams.sCountLabel = getParameterValue(sCmd, "countlabel", "countlabel", "Counts");
        _histParams.sAxisLabels[0] = getParameterValue(sCmd, "xlabel", "xlabel", "\\i x");
        _histParams.sAxisLabels[1] = getParameterValue(sCmd, "ylabel", "ylabel", "\\i y");
        _histParams.sAxisLabels[2] = getParameterValue(sCmd, "zlabel", "zlabel", "\\i z");
    }

    std::string sMethod = getParameterValue(sCmd, "method", "m", "");

    if (sMethod == "scott")
        _histParams.nMethod = SCOTT;
    else if (sMethod == "freedman")
        _histParams.nMethod = FREEDMAN_DIACONIS;
    else
        _histParams.nMethod = STURGES;

    if (findParameter(sCmd, "sum"))
        bSum = true;

    getIntervalDef(sCmd, "x", _histParams.ranges.x[0], _histParams.ranges.x[1]);
    getIntervalDef(sCmd, "y", _histParams.ranges.y[0], _histParams.ranges.y[1]);
    getIntervalDef(sCmd, "z", _histParams.ranges.z[0], _histParams.ranges.z[1]);

    if (findParameter(sCmd, "silent"))
        bSilent = true;

    if (findParameter(sCmd, "grid") && _idx.col.size() > 3)
        bGrid = true;

    //////////////////////////////////////////////////////////////////////////////////////
    if (bMake2DHist)
        createHist2D(sCmd, sTargettable, _idx, _tIdx, _histParams, bSum, bWriteToCache, bSilent);
    else
        createHist1D(sCmd, sTargettable, _idx, _tIdx, _histParams, bWriteToCache, bSilent, bGrid);
}


