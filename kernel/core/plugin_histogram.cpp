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
/// \brief This structure gathers all necessary
/// parameters for the histograms.
/////////////////////////////////////////////////
struct HistogramParameters
{
    IntervalSet ranges;

    double binWidth[2];
    int nBin[2];
    HistBinMethod nMethod;
    std::string sTable;
    std::string sBinLabel;
    std::string sCountLabel;
    std::string sAxisLabels[3];
    std::string sSavePath;
    bool bSum;
    bool bAvg;
    bool bRelative;
    bool bGrid;
    bool bStoreGrid;
    bool bBars;
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
static void prepareIntervalsForHist(const std::string& sCmd, Interval& range, double dDataMin, double dDataMax)
{
    // Replace the missing interval boundaries
    // with the minimal and maximal data values
    if (mu::isnan(range.front()) && mu::isnan(range.back()))
        range.reset(dDataMin, dDataMax);
    else if (mu::isnan(range.front()))
        range.reset(dDataMin, range.back());
    else if (mu::isnan(range.back()))
        range.reset(range.front(), dDataMax);

    // Ensure that the selected interval is part of
    // the data interval
    if (!mu::isnan(range.front()) && !mu::isnan(range.back()))
    {
        if (range.min() > dDataMax || range.max() < dDataMin)
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
/// \param vLegends std::vector<std::string>&
/// \param isXLog bool
/// \return std::vector<std::vector<double>>
///
/////////////////////////////////////////////////
static std::vector<std::vector<double>> calculateHist1dData(MemoryManager& _data, const Indices& _idx, const HistogramParameters& _histParams, mglData& _histData, mglData& _mAxisVals, std::vector<std::string>& vLegends, bool isXLog)
{
    // Prepare the data table
    std::vector<std::vector<double>> vHistMatrix(_histParams.nBin[XCOORD], std::vector<double>(_histParams.bGrid ? 1 : _idx.col.size(), 0.0));

    int nCount = 0;

    if (_histParams.bGrid)
    {
        // Repeat for every bin
        for (int k = 0; k < _histParams.nBin[XCOORD]; k++)
        {
            nCount = 0;
            Interval ivl(isXLog ? pow(10.0, log10(_histParams.ranges[ZCOORD].min())+k*_histParams.binWidth[XCOORD]) : _histParams.ranges[ZCOORD].min()+k*_histParams.binWidth[XCOORD],
                         isXLog ? pow(10.0, log10(_histParams.ranges[ZCOORD].min())+(k+1)*_histParams.binWidth[XCOORD]) : _histParams.ranges[ZCOORD].min()+(k+1)*_histParams.binWidth[XCOORD]);

            // Detect the number of values, which
            // are part of the current bin interval
            for (size_t i = 2; i < _idx.col.size(); i++)
            {
                for (size_t l = 0; l < _idx.row.size(); l++)
                {
                    if (!_histParams.ranges[XCOORD].isInside(_data.getElement(_idx.row[l], _idx.col[0], _histParams.sTable).getNum().asCF64())
                        || !_histParams.ranges[YCOORD].isInside(_data.getElement(_idx.row[l], _idx.col[1], _histParams.sTable).getNum().asCF64())
                        || !_histParams.ranges[ZCOORD].isInside(_data.getElement(_idx.row[l], _idx.col[i], _histParams.sTable).getNum().asCF64()))
                        continue;

                    if (ivl.isInside(_data.getElement(_idx.row[l], _idx.col[i], _histParams.sTable).getNum().asCF64(), Interval::INCLUDE_LOWER))
                        nCount++;
                }

                if (i == 2 && !isXLog)
                    _mAxisVals.a[k] = _histParams.ranges[ZCOORD].min() + (k + 0.5) * _histParams.binWidth[XCOORD];
                else if (i == 2)
                    _mAxisVals.a[k] = pow(10.0, log10(_histParams.ranges[ZCOORD].min()) + (k + 0.5) * _histParams.binWidth[XCOORD]);
            }

            // Store the value in the corresponding column
            vHistMatrix[k][0] = nCount;
            _histData.a[k] = nCount;
        }

        vLegends.push_back(_histParams.sTable);
    }
    else
    {
        // Repeat for every data set
        for (size_t i = 0; i < _idx.col.size(); i++)
        {
            // Repeat for every bin
            for (int k = 0; k < _histParams.nBin[XCOORD]; k++)
            {
                nCount = 0;

                // Detect the number of values, which
                // are part of the current bin interval
                for (size_t l = 0; l < _idx.row.size(); l++)
                {
                    std::complex<double> val = _data.getElement(_idx.row[l], _idx.col[i], _histParams.sTable).getNum().asCF64();

                    if (isXLog)
                    {
                        if (std::floor((val.real() - std::pow(10.0, _histParams.ranges[XCOORD].min())) / _histParams.binWidth[XCOORD]) == k
                            || (val.real() == std::pow(10.0, _histParams.ranges[XCOORD].max()) && k+1 == _histParams.nBin[XCOORD]))
                            nCount++;
                    }
                    else
                    {
                        if (std::floor((val.real() - _histParams.ranges[XCOORD].min()) / _histParams.binWidth[XCOORD]) == k
                            || (val.real() == _histParams.ranges[XCOORD].max() && k+1 == _histParams.nBin[XCOORD]))
                            nCount++;
                    }
                }

                if (!i && !isXLog)
                    _mAxisVals.a[k] = _histParams.ranges[XCOORD].min() + (k + 0.5) * _histParams.binWidth[XCOORD];
                else if (!i)
                    _mAxisVals.a[k] = pow(10.0, log10(_histParams.ranges[XCOORD].min()) + (k + 0.5) * _histParams.binWidth[XCOORD]);

                // Store the value in the corresponding column
                vHistMatrix[k][i] = nCount;
                _histData.a[k + (_histParams.nBin[XCOORD] * i)] = nCount;
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
/// \param vCategories const ValueVector&
/// \param sCommonExponent std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string prepareTicksForHist1d(const HistogramParameters& _histParams, const mglData& _mAxisVals, const ValueVector& vCategories, std::string& sCommonExponent)
{
    const int NDIGITS = 4;
    std::string sTicks;
    double dCommonExponent = 1.0;

    // Try to find the common bin interval exponent to
    // factorize it out
    if (_histParams.bGrid)
    {
        if (toString(_histParams.ranges[ZCOORD].min() + _histParams.binWidth[XCOORD] / 2.0, NDIGITS).find('e') != std::string::npos
            || toString(_histParams.ranges[ZCOORD].min() + _histParams.binWidth[XCOORD] / 2.0, NDIGITS).find('E') != std::string::npos)
        {
            sCommonExponent = toString(_histParams.ranges[ZCOORD].min() + _histParams.binWidth[XCOORD] / 2.0, NDIGITS).substr(toString(_histParams.ranges[ZCOORD].min() + _histParams.binWidth[XCOORD] / 2.0, NDIGITS).find('e'));
            dCommonExponent = StrToDb("1.0" + sCommonExponent);

            for (int i = 0; i < _histParams.nBin[XCOORD]; i++)
            {
                if (toString((_histParams.ranges[ZCOORD].min() + i * _histParams.binWidth[XCOORD] + _histParams.binWidth[XCOORD] / 2.0) / dCommonExponent, NDIGITS).find('e') != std::string::npos)
                {
                    sCommonExponent = "";
                    dCommonExponent = 1.0;
                    break;
                }
            }

            sTicks = toString((_histParams.ranges[ZCOORD].min() + _histParams.binWidth[XCOORD] / 2.0) / dCommonExponent, NDIGITS) + "\\n";
        }
        else
            sTicks = toString(_histParams.ranges[ZCOORD].min() + _histParams.binWidth[XCOORD] / 2.0, NDIGITS) + "\\n";
    }
    else
    {
        if (vCategories.size())
        {
            for (size_t i = 0; i < vCategories.size(); i++)
            {
                if (i)
                    sTicks += "\\n";

                sTicks += vCategories[i].getStr();
            }

            return sTicks;
        }
        else if (toString(_histParams.ranges[XCOORD].min() + _histParams.binWidth[XCOORD] / 2.0, NDIGITS).find('e') != std::string::npos
                 || toString(_histParams.ranges[XCOORD].min() + _histParams.binWidth[XCOORD] / 2.0, NDIGITS).find('E') != std::string::npos)
        {
            sCommonExponent = toString(_histParams.ranges[XCOORD].min() + _histParams.binWidth[XCOORD] / 2.0, NDIGITS).substr(toString(_histParams.ranges[XCOORD].min() + _histParams.binWidth[XCOORD] / 2.0, NDIGITS).find('e'));
            dCommonExponent = StrToDb("1.0" + sCommonExponent);

            for (int i = 0; i < _histParams.nBin[XCOORD]; i++)
            {
                if (toString((_histParams.ranges[XCOORD].min() + i * _histParams.binWidth[XCOORD] + _histParams.binWidth[XCOORD] / 2.0) / dCommonExponent, NDIGITS).find('e') != std::string::npos)
                {
                    sCommonExponent = "";
                    dCommonExponent = 1.0;
                    break;
                }
            }

            sTicks = toString((_histParams.ranges[XCOORD].min() + _histParams.binWidth[XCOORD] / 2.0) / dCommonExponent, NDIGITS) + "\\n";
        }
        else
            sTicks = toString(_histParams.ranges[XCOORD].min() + _histParams.binWidth[XCOORD] / 2.0, NDIGITS) + "\\n";
    }

    // Create the ticks list by factorizing out
    // the common exponent
    for (int i = 1; i < _histParams.nBin[XCOORD] - 1; i++)
    {
        if (_histParams.nBin[XCOORD] > 16)
        {
            if (!((_histParams.nBin[XCOORD] - 1) % 2) && !(i % 2) && _histParams.nBin[XCOORD] - 1 < 33)
                sTicks += toString(_mAxisVals.a[i] / dCommonExponent, NDIGITS) + "\\n";
            else if (!((_histParams.nBin[XCOORD] - 1) % 2) && (i % 2) && _histParams.nBin[XCOORD] - 1 < 33)
                sTicks += "\\n";
            else if (!((_histParams.nBin[XCOORD] - 1) % 4) && !(i % 4))
                sTicks += toString(_mAxisVals.a[i] / dCommonExponent, NDIGITS) + "\\n";
            else if (!((_histParams.nBin[XCOORD] - 1) % 4) && (i % 4))
                sTicks += "\\n";
            else if (!((_histParams.nBin[XCOORD] - 1) % 3) && !(i % 3))
                sTicks += toString(_mAxisVals.a[i] / dCommonExponent, NDIGITS) + "\\n";
            else if (!((_histParams.nBin[XCOORD] - 1) % 3) && (i % 3))
                sTicks += "\\n";
            else if (!((_histParams.nBin[XCOORD] - 1) % 5) && !(i % 5))
                sTicks += toString(_mAxisVals.a[i] / dCommonExponent, NDIGITS) + "\\n";
            else if (!((_histParams.nBin[XCOORD] - 1) % 5) && (i % 5))
                sTicks += "\\n";
            else if (!((_histParams.nBin[XCOORD] - 1) % 7) && !(i % 7))
                sTicks += toString(_mAxisVals.a[i] / dCommonExponent, NDIGITS) + "\\n";
            else if (!((_histParams.nBin[XCOORD] - 1) % 7) && (i % 7))
                sTicks += "\\n";
            else if (!((_histParams.nBin[XCOORD] - 1) % 11) && !(i % 11))
                sTicks += toString(_mAxisVals.a[i] / dCommonExponent, NDIGITS) + "\\n";
            else if (!((_histParams.nBin[XCOORD] - 1) % 11) && (i % 11))
                sTicks += "\\n";
            else if (((_histParams.nBin[XCOORD] - 1) % 2 && (_histParams.nBin[XCOORD] - 1) % 3 && (_histParams.nBin[XCOORD] - 1) % 5 && (_histParams.nBin[XCOORD] - 1) % 7 && (_histParams.nBin[XCOORD] - 1) % 11) && !(i % 3))
                sTicks += toString(_mAxisVals.a[i] / dCommonExponent, NDIGITS) + "\\n";
            else
                sTicks += "\\n";
        }
        else
            sTicks += toString(_mAxisVals.a[i] / dCommonExponent, NDIGITS) + "\\n";
    }

    sTicks += toString(_mAxisVals.a[_histParams.nBin[XCOORD] - 1] / dCommonExponent, NDIGITS);

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
/// \param vCategories const ValueVector&
/// \param bFormat bool
/// \param bSilent bool
/// \return void
///
/////////////////////////////////////////////////
static void createOutputForHist1D(MemoryManager& _data, const Indices& _idx, const std::vector<std::vector<double>>& vHistMatrix, const HistogramParameters& _histParams, const mglData& _mAxisVals, const ValueVector& vCategories, bool bFormat, bool bSilent)
{
    Output& _out = NumeReKernel::getInstance()->getOutput();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    std::vector<std::vector<std::string>> sOut(vHistMatrix.size()+1, std::vector<std::string>(vHistMatrix[0].size()+1));

    // --> Schreibe Tabellenkoepfe <--
    sOut[0][0] = _histParams.sBinLabel;

    if (_histParams.bGrid)
        sOut[0][1] = _histParams.sCountLabel + ": " + _histParams.sTable;
    else
    {
        for (size_t i = 1; i < vHistMatrix[0].size() + 1; i++)
        {
            sOut[0][i] = _data.getTopHeadLineElement(_idx.col[i-1], _histParams.sTable);
        }
    }

    // --> Setze die ueblichen Ausgabe-Info-Parameter <--
    if (bFormat)
    {
        _out.setPluginName(_lang.get("HIST_OUT_PLGNINFO", PI_HIST, toString(_idx.col.front() + 1), toString(_idx.col.last()+1), _data.getDataFileName(_histParams.sTable)));

        if (_histParams.bGrid)
            _out.setCommentLine(_lang.get("HIST_OUT_COMMENTLINE", toString(_histParams.ranges[ZCOORD].min(), 5), toString(_histParams.ranges[ZCOORD].max(), 5), toString(_histParams.binWidth[XCOORD], 5)));
        else
            _out.setCommentLine(_lang.get("HIST_OUT_COMMENTLINE", toString(_histParams.ranges[XCOORD].min(), 5), toString(_histParams.ranges[XCOORD].max(), 5), toString(_histParams.binWidth[XCOORD], 5)));

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

    double diff = _mAxisVals.GetNN() > 1 ? 0.5*(_mAxisVals.a[1] - _mAxisVals.a[0]) : 0.0;

    // --> Fuelle die Ausgabe-Matrix <--
    for (size_t i = 1; i < vHistMatrix.size() + 1; i++)
    {
        if (vCategories.size() <= (i-1))
            sOut[i][0] = toString(_mAxisVals.a[i - 1]-diff, _option);
        else
            sOut[i][0] = vCategories[i-1].getStr();

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

            _out.format(sOut);

            if (!_out.isFile())
            {
                NumeReKernel::toggleTableStatus();
                make_hline();
            }
        }
    }
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
    mglGraph* _histGraph = new mglGraph(0);
    _histGraph->SetPenDelta(0.65);

    // Apply plot output size using the resolution
    // and the aspect settings
    if (_pData.getSettings(PlotData::INT_SIZE_X) > 0 && _pData.getSettings(PlotData::INT_SIZE_Y) > 0)
        _histGraph->SetSize(_pData.getSettings(PlotData::INT_SIZE_X), _pData.getSettings(PlotData::INT_SIZE_Y));
    else if (_pData.getSettings(PlotData::INT_HIGHRESLEVEL) == 2 && bSilent && _pData.getSettings(PlotData::LOG_SILENTMODE))
    {
        double dHeight = sqrt(1920.0 * 1440.0 / dAspect);
        _histGraph->SetSize((int)lrint(dAspect * dHeight), (int)lrint(dHeight));
    }
    else if (_pData.getSettings(PlotData::INT_HIGHRESLEVEL) == 1 && bSilent && _pData.getSettings(PlotData::LOG_SILENTMODE))
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
    _histGraph->SetFontSizeCM(0.24 * ((double)(1 + _pData.getSettings(PlotData::FLOAT_TEXTSIZE)) / 6.0), 72);
    _histGraph->SetBarWidth(_pData.getSettings(PlotData::FLOAT_BARS) ? _pData.getSettings(PlotData::FLOAT_BARS) : 0.9);

    _histGraph->SetRanges(1, 2, 1, 2, 1, 2);

    // Apply logarithmic functions to the axes,
    // if necessary
    _histGraph->SetFunc(_pData.getLogscale(XRANGE) ? "lg(x)" : "",
                        _pData.getLogscale(YRANGE) ? "lg(y)" : "",
                        _pData.getLogscale(ZRANGE) ? "lg(z)" : "");

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
/// \param vCategories const ValueVector&
/// \param bSilent bool
/// \return void
///
/////////////////////////////////////////////////
static void createPlotForHist1D(HistogramParameters& _histParams, mglData& _mAxisVals, mglData& _histData, const std::vector<std::string>& vLegends, const ValueVector& vCategories, bool bSilent)
{
    Output& _out = NumeReKernel::getInstance()->getOutput();
    PlotData& _pData = NumeReKernel::getInstance()->getPlottingData();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    // Define aspect and plotting colors
    double dAspect = _pData.getSettings(PlotData::FLOAT_ASPECT);
    int nStyle = 0;
    const int nStyleMax = 14;
    std::string sColorStyles[nStyleMax] = {"r", "g", "b", "q", "m", "P", "u", "R", "G", "B", "Q", "M", "p", "U"};
    double dMax = _histData.Maximal();

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
        _histGraph->AddLegend(vLegends[i].c_str(), (sColorStyles[nStyle] + (_histParams.bBars ? "" : "=.")).c_str());

        if (nStyle == nStyleMax - 1)
            nStyle = 0;
        else
            nStyle++;
    }

    // Get the common exponent and the precalculated ticks
    std::string sCommonExponent;
    std::string sTicks = prepareTicksForHist1d(_histParams, _mAxisVals, vCategories, sCommonExponent);

    // If we calculated a single histogram from a data grid,
    // we need to use the z ranges for the x ranges, because
    // those are the values we binned for
    if (_histParams.bGrid)
        _histParams.ranges[XCOORD] = _histParams.ranges[ZCOORD];

    // Update the x ranges for a possible logscale
    if (_pData.getLogscale(XRANGE) && _histParams.ranges[XCOORD].min() <= 0.0 && _histParams.ranges[XCOORD].max() > 0.0)
        _histParams.ranges[XCOORD].reset(_histParams.ranges[XCOORD].max() / 1e3, _histParams.ranges[XCOORD].max());
    else if (_pData.getLogscale(XRANGE) && _histParams.ranges[XCOORD].min() < 0.0 && _histParams.ranges[XCOORD].max() <= 0.0)
        _histParams.ranges[XCOORD].reset(1.0, 1.0);

    // Update the y ranges for a possible logscale
    if (_pData.getLogscale(YRANGE))
        _histGraph->SetRanges(_histParams.ranges[XCOORD].min(),
                              _histParams.ranges[XCOORD].max() + (vCategories.size() != 0),
                              0.1, 1.4 * dMax);
    else
        _histGraph->SetRanges(_histParams.ranges[XCOORD].min(),
                              _histParams.ranges[XCOORD].max() + (vCategories.size() != 0),
                              _histParams.bBars ? 0.0 : -0.05 * dMax, 1.05 * dMax);

    // Create the axes
    if (_pData.getSettings(PlotData::INT_AXIS) != AXIS_NONE)
    {
        if (!_pData.getLogscale(XRANGE) && _pData.getTimeAxis(XRANGE).use)
            _histGraph->SetTicksTime('x', 0, _pData.getTimeAxis(XRANGE).sTimeFormat.c_str());
        else if (!_pData.getLogscale(XRANGE) && _histParams.bBars)
        {
            if (vCategories.size())
                _histGraph->SetTicksVal('x', _mAxisVals, sTicks.c_str());
            else
            {
                double diff = _mAxisVals.GetNN() > 2 ? (_mAxisVals.a[1] - _mAxisVals.a[0]) : 0.0;

                if (_pData.getTickTemplate(XCOORD).length())
                    _histGraph->SetTickTempl('x', _pData.getTickTemplate(XCOORD).c_str());

                if (_pData.getTickTemplate(YCOORD).length())
                    _histGraph->SetTickTempl('y', _pData.getTickTemplate(YCOORD).c_str());

                int subticks = 0;
                int tickfactor = 1;

                // Determine the number of subticks and the tick factor for the
                // x axis through some basic heuristic. Could be improved in the
                // future to resolve the fixation to 5 subticks, but works for
                // now
                if (_mAxisVals.GetNN() > 25)
                {
                    tickfactor = _mAxisVals.GetNN() / 5;

                    if ((tickfactor % 5) < 3)
                        tickfactor -= tickfactor % 5;
                    else
                        tickfactor += 5-(tickfactor % 5);

                    if (!(tickfactor % 5))
                        subticks = 4;
                    else if (!(tickfactor % 4))
                        subticks = 3;
                    else if (!(tickfactor % 2))
                        subticks = 1;
                }
                else if (_mAxisVals.GetNN() > 15)
                {
                    subticks = 4;
                    tickfactor = 5;
                }
                else if (_mAxisVals.GetNN() > 10)
                {
                    subticks = 3;
                    tickfactor = 4;
                }
                else if (_mAxisVals.GetNN() > 5)
                {
                    subticks = 1;
                    tickfactor = 2;
                }

                _histGraph->SetTicks('x', tickfactor * diff, subticks, _mAxisVals.a[0]-0.5*diff);
            }
        }

        if (!_pData.getSettings(PlotData::LOG_BOX)
            && _histParams.ranges[XCOORD].min() <= 0.0
            && _histParams.ranges[XCOORD].max() >= 0.0
            && !_pData.getLogscale(YRANGE))
            _histGraph->Axis("AKDTVISO");
        else if (!_pData.getSettings(PlotData::LOG_BOX))
            _histGraph->Axis("AKDTVISO");
        else
            _histGraph->Axis();
    }

    // Create the surrounding box
    if (_pData.getSettings(PlotData::LOG_BOX))
        _histGraph->Box();

    // Write the axis labels
    if (_pData.getSettings(PlotData::INT_AXIS) != AXIS_NONE)
    {
        _histGraph->Label('x', _histParams.sBinLabel.c_str(), 0.0);

        if (sCommonExponent.length() && !_pData.getLogscale(XRANGE) && !_pData.getTimeAxis(XRANGE).use)
        {
            _histGraph->Puts(mglPoint(_histParams.ranges[XCOORD].max() + (_histParams.ranges[XCOORD].max() - _histParams.ranges[XCOORD].min()) / 10.0),
                             mglPoint(_histParams.ranges[XCOORD].max() + (_histParams.ranges[XCOORD].max() - _histParams.ranges[XCOORD].min()) / 10.0 + 1),
                             sCommonExponent.c_str(), ":TL", -1.3);
        }

        if (_pData.getSettings(PlotData::LOG_BOX))
            _histGraph->Label('y', _histParams.sCountLabel.c_str(), 0.0);
        else
            _histGraph->Label('y', _histParams.sCountLabel.c_str(), 1.1);
    }

    // Create the grid
    if (_pData.getSettings(PlotData::INT_GRID))
    {
        if (_pData.getSettings(PlotData::INT_GRID) == 2)
        {
            _histGraph->Grid("xy!", _pData.getGridStyle().c_str());
            _histGraph->Grid("xy", _pData.getFineGridStyle().c_str());
        }
        else
            _histGraph->Grid("xy", _pData.getGridStyle().c_str());
    }

    // Position the legend
    if (!_pData.getSettings(PlotData::LOG_BOX))
        _histGraph->Legend(1.25, 1.0);
    else
        _histGraph->Legend(_pData.getSettings(PlotData::INT_LEGENDPOSITION));

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

        if (!_histParams.bBars)
            sColor += "=.";

        if (nStyle == nStyleMax - 1)
            nStyle = 0;
        else
            nStyle++;
    }

    sColor += '\0';

    // Create the actual bars
    if (_histParams.bBars)
        _histGraph->Bars(_mAxisVals, _histData, sColor.c_str());
    else
        _histGraph->Plot(_mAxisVals, _histData, sColor.c_str());

    // Open the plot in the graph viewer
    // of write it directly to file
    if (_pData.getSettings(PlotData::LOG_OPENIMAGE) && !_pData.getSettings(PlotData::LOG_SILENTMODE) && !bSilent)
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
        _histGraph->WritePNG(sHistSavePath.c_str(), "", false);
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
/// \brief Returns the superset of both vectors
/// (if one is a subset of the other) or an empty
/// vector.
///
/// \param lhs const mu::Array&
/// \param rhs const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
static mu::Array isSubSet(const mu::Array& lhs, const mu::Array& rhs)
{
    // Are they equal?
    if (mu::all(lhs == rhs))
        return lhs;

    const mu::Array& longer = lhs.size() > rhs.size() ? lhs : rhs;
    const mu::Array& shorter = lhs.size() > rhs.size() ? rhs : lhs;

    // Ensure that one of the category definitions is a subset of the other one
    for (size_t i = 0; i < shorter.size(); i++)
    {
        auto pos = std::find(longer.begin(), longer.end(), shorter[i]);

        if (pos == longer.end() || *(pos+1) != shorter[i+1])
            return mu::Array();
    }

    // return the superset
    return longer;
}


/////////////////////////////////////////////////
/// \brief Get the bins from possible categorical
/// columns and return the valid categories for
/// the plot axes.
///
/// \param _data MemoryManager&
/// \param _histParams HistogramParameters&
/// \param _idx Indices&
/// \param vCategories mu::Array&
/// \return bool
///
/////////////////////////////////////////////////
static bool getBinsFromCategories(MemoryManager& _data, HistogramParameters& _histParams, Indices& _idx, mu::Array& vCategories)
{
    // For special column types, we use predefined bins instead of the calculated ones
    if (_data.getType(_idx.col, _histParams.sTable) == TableColumn::TYPE_CATEGORICAL)
    {
        // Try to use the category definitions as bins and labels
        _histParams.binWidth[XCOORD] = 1.0;
        vCategories = _data.getCategoryList(_idx.col.subidx(0, 1), _histParams.sTable);

        for (size_t i = 1; i < _idx.col.size(); i++)
        {
            // Ensure that all categories can be described by one common superset
            if (!(vCategories = isSubSet(vCategories, _data.getCategoryList(_idx.col.subidx(i, 1), _histParams.sTable))).size())
            {
                // If that's not the case, we simply use their IDs as
                // abstract categories to at least us reasoable bin values
                int nCategoryMin = intCast(std::max(_data.min(_histParams.sTable, _idx.row, _idx.col).real(),
                                                    _histParams.ranges[XCOORD].min()));
                int nCategoryMax = intCast(std::min(_data.max(_histParams.sTable, _idx.row, _idx.col).real(),
                                                    _histParams.ranges[XCOORD].max()));
                _histParams.nBin[XCOORD] = nCategoryMax - nCategoryMin + 1;

                for (int i = 0; i < _histParams.nBin[XCOORD]; i++)
                {
                    vCategories.push_back(mu::Category{mu::Numerical(i+nCategoryMin), "Cat:" + toString(i+nCategoryMin)});
                }

                return true;
            }
        }
    }
    else if (_data.getType(_idx.col, _histParams.sTable) == TableColumn::TYPE_LOGICAL)
    {
        // Use logical values as bins and labels
        _histParams.binWidth[XCOORD] = 1.0;
        _histParams.nBin[XCOORD] = 2;
        vCategories.push_back(mu::Category{mu::Numerical(0), "false"});
        vCategories.push_back(mu::Category{mu::Numerical(1), "true"});
    }

    // If we found a set of categories, ensure that their IDs are part of the
    // user specified x interval
    if (vCategories.size())
    {
        size_t i = 0;

        while (i < vCategories.size())
        {
            if (!_histParams.ranges[XCOORD].isInside(vCategories[i].getNum().asI64()))
                vCategories.erase(vCategories.begin() + i);
            else
                i++;
        }

        _histParams.nBin[XCOORD] = vCategories.size();

        return true;
    }

    return false;
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
/// \return void
///
/////////////////////////////////////////////////
static void createHist1D(const std::string& sCmd, const std::string& sTargettable, Indices& _idx, Indices& _tIdx, HistogramParameters& _histParams, bool bWriteToCache, bool bSilent)
{
    PlotData& _pData = NumeReKernel::getInstance()->getPlottingData();
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    int nMax = 0;

    mglData _histData;
    mglData _mAxisVals;

    if (_histParams.bGrid)
    {
        // x-Range
        prepareIntervalsForHist(sCmd, _histParams.ranges[XCOORD],
                                _data.min(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1)).real(),
                                _data.max(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1)).real());

        // y-Range
        prepareIntervalsForHist(sCmd, _histParams.ranges[YCOORD],
                                _data.min(_histParams.sTable, _idx.row, _idx.col.subidx(1, 1)).real(),
                                _data.max(_histParams.sTable, _idx.row, _idx.col.subidx(1, 1)).real());

        // z-Range
        prepareIntervalsForHist(sCmd, _histParams.ranges[ZCOORD],
                                _data.min(_histParams.sTable, _idx.row, _idx.col.subidx(2)).real(),
                                _data.max(_histParams.sTable, _idx.row, _idx.col.subidx(2)).real());
    }
    else
    {
        prepareIntervalsForHist(sCmd, _histParams.ranges[XCOORD],
                                _data.min(_histParams.sTable, _idx.row, _idx.col).real(),
                                _data.max(_histParams.sTable, _idx.row, _idx.col).real());
    }

    if (!_histParams.bGrid && _idx.col.size() > 32)
        throw SyntaxError(SyntaxError::WRONG_DATA_SIZE, sCmd, _histParams.sTable, _histParams.sTable);

    for (size_t i = 0; i < _idx.row.size(); i++)
    {
        for (size_t j = 2 * _histParams.bGrid; j < _idx.col.size(); j++)
        {
            if (_histParams.bGrid
                && _histParams.ranges[XCOORD].isInside(_data.getElement(_idx.row[i], _idx.col[0], _histParams.sTable).getNum().asCF64())
                && _histParams.ranges[YCOORD].isInside(_data.getElement(_idx.row[i], _idx.col[1], _histParams.sTable).getNum().asCF64())
                && _histParams.ranges[ZCOORD].isInside(_data.getElement(_idx.row[i], _idx.col[j], _histParams.sTable).getNum().asCF64()))
                nMax++;
            else if (!_histParams.bGrid
                     && _histParams.ranges[XCOORD].isInside(_data.getElement(_idx.row[i], _idx.col[j], _histParams.sTable).getNum().asCF64()))
                nMax++;
        }
    }

    mu::Array vCategories;

    if (!_histParams.nBin[XCOORD] && _histParams.binWidth[XCOORD] == 0.0)
    {
        if (_histParams.bGrid)
        {
            if (_histParams.nMethod == STURGES)
                _histParams.nBin[XCOORD] = (int)rint(1.0 + 3.3 * log10((double)nMax));
            else if (_histParams.nMethod == SCOTT)
                _histParams.binWidth[XCOORD] = 3.49 * _data.std(_histParams.sTable, _idx.row, _idx.col.subidx(2)).real() / pow((double)nMax, 1.0 / 3.0);
            else if (_histParams.nMethod == FREEDMAN_DIACONIS)
                _histParams.binWidth[XCOORD] = 2.0 * (_data.pct(_histParams.sTable, _idx.row, _idx.col.subidx(2), 0.75).real()
                                                 - _data.pct(_histParams.sTable, _idx.row, _idx.col.subidx(2), 0.25).real()) / pow((double)nMax, 1.0 / 3.0);
        }
        else
        {
            if (!getBinsFromCategories(_data, _histParams, _idx, vCategories))
            {
                if (_histParams.nMethod == STURGES)
                    _histParams.nBin[XCOORD] = (int)rint(1.0 + 3.3 * log10((double)nMax / (double)(_idx.col.size())));
                else if (_histParams.nMethod == SCOTT)
                    _histParams.binWidth[XCOORD] = 3.49 * _data.std(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1)).real() / pow((double)nMax / (double)(_idx.col.size()), 1.0 / 3.0);
                else if (_histParams.nMethod == FREEDMAN_DIACONIS)
                    _histParams.binWidth[XCOORD] = 2.0 * (_data.pct(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1), 0.75).real()
                                                     - _data.pct(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1), 0.25).real()) / pow((double)nMax / (double)(_idx.col.size()), 1.0 / 3.0);
            }
        }
    }

    // Initialize the mglData objects
    if (_histParams.nBin[XCOORD])
    {
        if (_histParams.bGrid)
            _histData.Create(_histParams.nBin[XCOORD]);
        else
            _histData.Create(_histParams.nBin[XCOORD], _idx.col.size());

        if (_histParams.binWidth[XCOORD] == 0.0)
        {
            // --> Berechne die Intervall-Laenge (Explizite Typumwandlung von int->double) <--
            if (_histParams.bGrid)
            {
                if (_pData.getLogscale(XRANGE))
                    _histParams.binWidth[XCOORD] = (log10(_histParams.ranges[ZCOORD].max()) - log10(_histParams.ranges[ZCOORD].min())) / (double)_histParams.nBin[XCOORD];
                else
                    _histParams.binWidth[XCOORD] = _histParams.ranges[ZCOORD].range() / (double)_histParams.nBin[XCOORD];
            }
            else
            {
                if (_pData.getLogscale(XRANGE))
                    _histParams.binWidth[XCOORD] = (log10(_histParams.ranges[XCOORD].max()) - log10(_histParams.ranges[XCOORD].min())) / (double)_histParams.nBin[XCOORD];
                else
                    _histParams.binWidth[XCOORD] = _histParams.ranges[XCOORD].range() / (double)_histParams.nBin[XCOORD];
            }
        }
    }
    else
    {
        // --> Gut. Dann berechnen wir daraus die Anzahl der Bins -> Es kann nun aber sein, dass der letzte Bin ueber
        //     das Intervall hinauslaeuft <--
        if (_histParams.binWidth[XCOORD] > _histParams.ranges[XCOORD].range())
            throw SyntaxError(SyntaxError::TOO_LARGE_BINWIDTH, sCmd, SyntaxError::invalid_position);

        for (int i = 0; (i*_histParams.binWidth[XCOORD])+_histParams.ranges[XCOORD].min() < _histParams.ranges[XCOORD].max()+_histParams.binWidth[XCOORD]; i++)
        {
            _histParams.nBin[XCOORD]++;
        }

        if (_histParams.nBin[XCOORD])
        {
            if (_histParams.bGrid)
                _histData.Create(_histParams.nBin[XCOORD]);
            else
                _histData.Create(_histParams.nBin[XCOORD], _idx.col.size());
        }
    }

    _mAxisVals.Create(_histParams.nBin[XCOORD]);

    // Calculate the data for the histogram
    std::vector<std::string> vLegends;
    std::vector<std::vector<double>> vHistMatrix = calculateHist1dData(_data, _idx, _histParams, _histData, _mAxisVals,
                                                                       vLegends, _pData.getLogscale(XRANGE));

    // Switch to relative frequencies
    if (_histParams.bRelative)
    {
        double dSum = 0;

        for (size_t i = 0; i < vHistMatrix.size(); i++)
        {
            dSum = std::accumulate(vHistMatrix[i].begin(), vHistMatrix[i].end(), dSum);
        }

        for (size_t i = 0; i < vHistMatrix.size(); i++)
        {
            for (size_t j = 0; j < vHistMatrix[i].size(); j++)
            {
                vHistMatrix[i][j] /= dSum;
            }
        }

        _histData /= dSum;
    }

    // Create the textual data for the terminal
    // and the file, if necessary
    createOutputForHist1D(_data, _idx, vHistMatrix, _histParams, _mAxisVals, vCategories,
                          !bWriteToCache || findParameter(sCmd, "export", '=') || findParameter(sCmd, "save", '='), bSilent);

    // Store the results into the output table,
    // if desired
    if (bWriteToCache)
    {
        _data.setHeadLineElement(_tIdx.col.front(), sTargettable, "Bins");

        if (vCategories.size())
            _data.convertColumns(sTargettable, _tIdx.col.subidx(0, 1), "string");

        for (size_t i = 0; i < vHistMatrix.size(); i++)
        {
            if (_tIdx.row.size() <= i)
                break;

            if (vCategories.size() <= i)
                _data.writeToTable(_tIdx.row[i], _tIdx.col.front(), sTargettable,
                                   _histParams.ranges[XCOORD].min() + i * _histParams.binWidth[XCOORD]);
            else
                _data.writeToTable(_tIdx.row[i], _tIdx.col.front(), sTargettable, vCategories[i].getStr());

            for (size_t j = 0; j < vHistMatrix[0].size(); j++)
            {
                if (_tIdx.col.size() <= j+1)
                    break;

                if (!i)
                    _data.setHeadLineElement(_tIdx.col[j+1], sTargettable, _data.getHeadLineElement(_idx.col[j], _histParams.sTable));

                _data.writeToTable(_tIdx.row[i], _tIdx.col[j+1], sTargettable, vHistMatrix[i][j]);
            }
        }
    }

    // Create the plot using the calculated data
    createPlotForHist1D(_histParams, _mAxisVals, _histData, vLegends, vCategories, bSilent);
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
            if (_histParams.ranges[XCOORD].isInside(_data.getElement(_idx.row[i], _idx.col[0], _histParams.sTable).getNum().asCF64())
                && _histParams.ranges[YCOORD].isInside(_data.getElement(_idx.row[i], _idx.col[1], _histParams.sTable).getNum().asCF64())
                && _data.isValidElement(_idx.row[i], _idx.col[2], _histParams.sTable))
            {
                _hist2DData[0].a[i] = _data.getElement(_idx.row[i], _idx.col[0], _histParams.sTable).getNum().asF64();
                _hist2DData[1].a[i] = _data.getElement(_idx.row[i], _idx.col[1], _histParams.sTable).getNum().asF64();
                _hist2DData[2].a[i] = _data.getElement(_idx.row[i], _idx.col[2], _histParams.sTable).getNum().asF64();
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
            _hist2DData[0].a[i] = _data.getElement(_idx.row[i], _idx.col[0], _histParams.sTable).getNum().asF64();
            _hist2DData[1].a[i] = _data.getElement(_idx.row[i], _idx.col[1], _histParams.sTable).getNum().asF64();

            for (size_t j = 0; j < _idx.col.size() - 2; j++)
            {
                if (_histParams.ranges[XCOORD].isInside(_data.getElement(_idx.row[i], _idx.col[0], _histParams.sTable).getNum().asCF64())
                    && _histParams.ranges[YCOORD].isInside(_data.getElement(_idx.row[i], _idx.col[1], _histParams.sTable).getNum().asCF64())
                    && _data.isValidElement(_idx.row[i], _idx.col[j + 2], _histParams.sTable))
                    _hist2DData[2].a[i + j * _idx.row.size()] = _data.getElement(_idx.row[i], _idx.col[j + 2], _histParams.sTable).getNum().asF64();
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
/// \param isLogScale bool
/// \param isHbar bool
/// \return mglData
///
/////////////////////////////////////////////////
static mglData calculateXYHist(MemoryManager& _data, const Indices& _idx, const HistogramParameters& _histParams, mglData* _mAxisVals, double dBinMin, double dMin, double dMax, double dIntLength, bool isLogScale, bool isHbar)
{
    mglData _histData(_histParams.nBin[isHbar]);
    Interval range(dMin, dMax);

    for (int k = 0; k < _histParams.nBin[isHbar]; k++)
    {
        Interval ivl(isLogScale ? pow(10.0, log10(dBinMin) + k*dIntLength) : (dBinMin + k*dIntLength),
                     isLogScale ? pow(10.0, log10(dBinMin) + (k+1)*dIntLength) : (dBinMin + (k+1)*dIntLength));
        double dSum = 0.0;
        size_t nCount = 0;

        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            if (ivl.isInside(_data.getElement(_idx.row[i], _idx.col[isHbar], _histParams.sTable).getNum().asCF64(), Interval::INCLUDE_LOWER))
            {
                if (_idx.col.size() == 3)
                {
                    if (_data.isValidElement(_idx.row[i], _idx.col[!isHbar], _histParams.sTable)
                        && _histParams.ranges[ZCOORD].isInside(_data.getElement(_idx.row[i], _idx.col[ZCOORD], _histParams.sTable).getNum().asCF64())
                        && range.isInside(_data.getElement(_idx.row[i], _idx.col[!isHbar], _histParams.sTable).getNum().asCF64()))
                    {
                        if (_histParams.bSum || _histParams.bAvg)
                            dSum += _data.getElement(_idx.row[i], _idx.col[ZCOORD], _histParams.sTable).getNum().asF64();
                        else
                            dSum++;

                        nCount++;
                    }
                }
                else
                {
                    for (size_t l = 0; l < _idx.row.size(); l++)
                    {
                        if (_data.isValidElement(_idx.row[l], _idx.col[!isHbar], _histParams.sTable)
                            && _data.isValidElement(_idx.row[(!isHbar*i+isHbar*l)], _idx.col[(!isHbar*l+isHbar*i)+2], _histParams.sTable)
                            && _histParams.ranges[ZCOORD].isInside(_data.getElement(_idx.row[(!isHbar*i+isHbar*l)],
                                                                                    _idx.col[(!isHbar*l+isHbar*i)+2], _histParams.sTable).getNum().asCF64())
                            && range.isInside(_data.getElement(_idx.row[l], _idx.col[!isHbar], _histParams.sTable).getNum().asCF64()))
                        {
                            dSum += _data.getElement(_idx.row[(!isHbar*i+isHbar*l)], _idx.col[(!isHbar*l+isHbar*i)+2], _histParams.sTable).getNum().asF64();
                            nCount++;
                        }
                    }
                }
            }
        }

        _histData.a[k] = dSum / (_histParams.bAvg ? nCount : 1);
        _mAxisVals->a[k] = ivl.middle();
    }

    return _histData;
}


/////////////////////////////////////////////////
/// \brief This static function converts the 3D
/// data into a datagrid containing histogram
/// points.
///
/// \param _data MemoryManager&
/// \param _idx const Indices&
/// \param _histParams const HistogramParameters&
/// \return std::vector<std::vector<double>>
///
/////////////////////////////////////////////////
static std::vector<std::vector<double>> calculateXYHistGrid(MemoryManager& _data, const Indices& _idx, const HistogramParameters& _histParams)
{
    std::vector<std::vector<double>> vRet(_histParams.nBin[XCOORD], std::vector<double>(_histParams.nBin[YCOORD]));

    double xMin = _histParams.ranges[XCOORD].min();
    double xInt = _histParams.binWidth[XCOORD];
    double yMin = _histParams.ranges[YCOORD].min();
    double yInt = _histParams.binWidth[YCOORD];
    bool xLog = NumeReKernel::getInstance()->getPlottingData().getLogscale(XCOORD);
    bool yLog = NumeReKernel::getInstance()->getPlottingData().getLogscale(YCOORD);

    for (int x = 0; x < _histParams.nBin[XCOORD]; x++)
    {
        Interval xInterval(xLog ? pow(10.0, log10(xMin)+x*xInt) : (xMin+x*xInt),
                           xLog ? pow(10.0, log10(xMin)+(x+1)*xInt) : (xMin+(x+1)*xInt));

        for (int y = 0; y < _histParams.nBin[YCOORD]; y++)
        {
            Interval yInterval(yLog ? pow(10.0, log10(yMin)+y*yInt) : (yMin+y*yInt),
                               yLog ? pow(10.0, log10(yMin)+(y+1)*yInt) : (yMin+(y+1)*yInt));

            double dSum = 0.0;
            size_t nCount = 0;

            for (size_t i = 0; i < _idx.row.size(); i++)
            {
                if (xInterval.isInside(_data.getElement(_idx.row[i], _idx.col[XCOORD], _histParams.sTable).getNum().asCF64(), Interval::INCLUDE_LOWER)
                    && yInterval.isInside(_data.getElement(_idx.row[i], _idx.col[YCOORD], _histParams.sTable).getNum().asCF64(), Interval::INCLUDE_LOWER))
                {
                    if (_idx.col.size() == 3)
                    {
                        if (_histParams.ranges[ZCOORD].isInside(_data.getElement(_idx.row[i], _idx.col[ZCOORD], _histParams.sTable).getNum().asCF64()))
                        {
                            if (_histParams.bSum || _histParams.bAvg)
                                dSum += _data.getElement(_idx.row[i], _idx.col[ZCOORD], _histParams.sTable).getNum().asF64();
                            else
                                dSum++;

                            nCount++;
                        }
                    }
                    else
                    {
                        for (size_t l = 0; l < _idx.row.size(); l++)
                        {
                            if (_histParams.ranges[ZCOORD].isInside(_data.getElement(_idx.row[x], _idx.col[y+2], _histParams.sTable).getNum().asCF64()))
                            {
                                dSum += _data.getElement(_idx.row[x], _idx.col[y+2], _histParams.sTable).getNum().asF64();
                                nCount++;
                            }
                        }
                    }
                }
            }

            vRet[x][y] = dSum / (_histParams.bAvg ? nCount : 1);
        }
    }

    return vRet;
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
/// \param bWriteToCache bool
/// \param shallFormat bool
/// \param isSilent bool
/// \return void
///
/////////////////////////////////////////////////
static void createOutputForHist2D(MemoryManager& _data, const Indices& _idx, const std::string& sTargettable, const Indices& _tIdx, const HistogramParameters& _histParams, mglData _mAxisVals[2], mglData& _barHistData, mglData& _hBarHistData, bool bWriteToCache, bool shallFormat, bool isSilent)
{
    Output& _out = NumeReKernel::getInstance()->getOutput();
    const Settings& _option = NumeReKernel::getInstance()->getSettings();

    int maxRow = std::max(_histParams.nBin[XCOORD], _histParams.nBin[YCOORD])+1;

    std::vector<std::vector<std::string>> sOut(maxRow, std::vector<std::string>(4));

    std::string sCountLabelStart = "Counts";

    if (_histParams.bAvg)
        sCountLabelStart = "Avg";
    else if (_histParams.bSum)
        sCountLabelStart = "Accum";

    double diff[2];
    diff[XCOORD] = _mAxisVals[XCOORD].GetNN() > 1 ? 0.5*(_mAxisVals[XCOORD].a[1] - _mAxisVals[XCOORD].a[0]) : 0.0;
    diff[YCOORD] = _mAxisVals[YCOORD].GetNN() > 1 ? 0.5*(_mAxisVals[YCOORD].a[1] - _mAxisVals[YCOORD].a[0]) : 0.0;

    // Fill output tables
    for (int n = 0; n < 2; n++)
    {
        for (int k = 0; k < _histParams.nBin[n]; k++)
        {
            sOut[k + 1][0+2*n] = toString(_mAxisVals[n].a[k] - diff[n], _option);
            sOut[k + 1][1+2*n] = toString((!n ? _barHistData.a[k] : _hBarHistData.a[k]), _option);

            if (!k)
            {
                sOut[k][0+2*n] = "Bins: " + _histParams.sAxisLabels[n];
                sOut[k][1+2*n] = sCountLabelStart + ": " + _histParams.sAxisLabels[ZCOORD];
            }

            if (bWriteToCache)
            {
                if (_tIdx.row.size() <= (size_t)k || _tIdx.col.size() < 4)
                    continue;

                if (!k)
                {
                    _data.setHeadLineElement(_tIdx.col[0+2*n], sTargettable, "Bins: " + _histParams.sAxisLabels[n]);
                    _data.setHeadLineElement(_tIdx.col[1+2*n], sTargettable, sCountLabelStart + ": " + _histParams.sAxisLabels[ZCOORD]);
                }

                _data.writeToTable(_tIdx.row[k], _tIdx.col[0+2*n], sTargettable, _mAxisVals[n].a[k] - diff[n]);
                _data.writeToTable(_tIdx.row[k], _tIdx.col[1+2*n], sTargettable, (!n ? _barHistData.a[k] : _hBarHistData.a[k]));

            }
        }
    }

    // Write the binned XY grid to the target table
    if (bWriteToCache && _histParams.bStoreGrid)
    {
        std::vector<std::vector<double>> vGrid = calculateXYHistGrid(_data, _idx, _histParams);

        for (size_t x = 0; x < vGrid.size(); x++)
        {
            if (_tIdx.row.size() <= x)
                break;

            for (size_t y = 0; y < vGrid[x].size(); y++)
            {
                if (_tIdx.col.size() <= y)
                    break;

                if (!x)
                    _data.setHeadLineElement(_tIdx.col[4+y], sTargettable,
                                             sCountLabelStart + "(x(:),y(" + toString(y+1) + "))");

                _data.writeToTable(_tIdx.row[x], _tIdx.col[4+y], sTargettable, vGrid[x][y]);
            }
        }
    }

    if (shallFormat)
    {
        _out.setPluginName("2D-" + _lang.get("HIS_OUT_PLGNINFO", PI_HIST,
                                             toString(_idx.col.front()+1), toString(_idx.col.last()+1),
                                             _data.getDataFileName(_histParams.sTable)));
        _out.setCommentLine(_lang.get("HIST_OUT_COMMENTLINE2D",
                                      toString(_histParams.ranges[XCOORD].min(), 5), toString(_histParams.ranges[XCOORD].max(), 5),
                                      toString(_histParams.binWidth[XCOORD], 5),
                                      toString(_histParams.ranges[YCOORD].min(), 5), toString(_histParams.ranges[YCOORD].max(), 5),
                                      toString(_histParams.binWidth[YCOORD], 5)));

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

            _out.format(sOut);

            if (!_out.isFile())
            {
                NumeReKernel::toggleTableStatus();
                make_hline();
            }
        }
    }
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
/// \param bSilent bool
/// \return void
///
/////////////////////////////////////////////////
static void createPlotsForHist2D(const std::string& sCmd, HistogramParameters& _histParams, mglData _mAxisVals[2], mglData& _barHistData, mglData& _hBarHistData, mglData _hist2DData[3], bool isScatterPlot, bool bSilent)
{
    PlotData& _pData = NumeReKernel::getInstance()->getPlottingData();
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    Output& _out = NumeReKernel::getInstance()->getOutput();

    double dAspect = _pData.getSettings(PlotData::FLOAT_ASPECT);

    mglGraph* _histGraph = prepareGraphForHist(dAspect, _pData, bSilent);

/////////////////////////////////// BAR PLOT
    _histGraph->MultiPlot(3, 3, 0, 2, 1, "<^>");
    _histGraph->SetBarWidth(0.9);
    _histGraph->SetTuneTicks(3, 1.05);
    _histParams.binWidth[XCOORD] = _barHistData.Maximal() - _barHistData.Minimal();
    _histGraph->SetRanges(1, 2, 1, 2, 1, 2);

    _histGraph->SetFunc(_pData.getLogscale(XRANGE) ? "lg(x)" : "",
                        _pData.getLogscale(ZRANGE) ? "lg(y)" : "");

    if (_barHistData.Minimal() >= 0)
    {
        if (_pData.getLogscale(ZRANGE) && _barHistData.Maximal() > 0.0)
        {
            if (_barHistData.Minimal() - _histParams.binWidth[XCOORD] / 20.0 > 0)
                _histGraph->SetRanges(_histParams.ranges[XCOORD].min(), _histParams.ranges[XCOORD].max(), _barHistData.Minimal() - _histParams.binWidth[XCOORD] / 20.0, _barHistData.Maximal() + _histParams.binWidth[XCOORD] / 20.0);
            else if (_barHistData.Minimal() > 0)
                _histGraph->SetRanges(_histParams.ranges[XCOORD].min(), _histParams.ranges[XCOORD].max(), _barHistData.Minimal() / 2.0, _barHistData.Maximal() + _histParams.binWidth[XCOORD] / 20.0);
            else
                _histGraph->SetRanges(_histParams.ranges[XCOORD].min(), _histParams.ranges[XCOORD].max(), (1e-2 * _barHistData.Maximal() < 1e-2 ? 1e-2 * _barHistData.Maximal() : 1e-2), _barHistData.Maximal() + _histParams.binWidth[XCOORD] / 20.0);
        }
        else if (_barHistData.Maximal() < 0.0 && _pData.getLogscale(ZRANGE))
        {
            delete _histGraph;
            throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCmd, SyntaxError::invalid_position);
        }
        else
            _histGraph->SetRanges(_histParams.ranges[XCOORD].min(), _histParams.ranges[XCOORD].max(), 0.0, _barHistData.Maximal() + _histParams.binWidth[XCOORD] / 20.0);
    }
    else
        _histGraph->SetRanges(_histParams.ranges[XCOORD].min(), _histParams.ranges[XCOORD].max(), _barHistData.Minimal() - _histParams.binWidth[XCOORD] / 20.0, _barHistData.Maximal() + _histParams.binWidth[XCOORD] / 20.0);

    if (!_pData.getLogscale(XRANGE) && _pData.getTimeAxis(XRANGE).use)
        _histGraph->SetTicksTime('x', 0, _pData.getTimeAxis(XRANGE).sTimeFormat.c_str());

    _histGraph->Box();
    _histGraph->Axis();

    if (_pData.getSettings(PlotData::INT_GRID) == 1)
        _histGraph->Grid("xy", _pData.getGridStyle().c_str());
    else if (_pData.getSettings(PlotData::INT_GRID) == 2)
    {
        _histGraph->Grid("xy!", _pData.getGridStyle().c_str());
        _histGraph->Grid("xy", _pData.getFineGridStyle().c_str());
    }

    _histGraph->Label('x', _histParams.sAxisLabels[XCOORD].c_str(), 0);

    if (_histParams.bSum || _histParams.bAvg)
        _histGraph->Label('y', _histParams.sAxisLabels[ZCOORD].c_str(), 0);
    else
        _histGraph->Label('y', _histParams.sCountLabel.c_str(), 0);

    _histGraph->Bars(_mAxisVals[0], _barHistData, _pData.getColors().c_str());

///////////////////////////////////// CENTER GRAPH
    _histGraph->MultiPlot(3, 3, 3, 2, 2, "<_>");
//    _histGraph->SetTicks('x');
//    _histGraph->SetTicks('y');
    _histGraph->SetTuneTicks(3, 1.05);
    _histGraph->SetFontSizeCM(0.24 * ((double)(1 + std::max(0.0, _pData.getSettings(PlotData::FLOAT_TEXTSIZE)-2)) / 6.0), 72);

    _histGraph->SetRanges(1, 2, 1, 2, 1, 2);
    _histGraph->SetFunc(_pData.getLogscale(XRANGE) ? "lg(x)" : "",
                        _pData.getLogscale(YRANGE) ? "lg(y)" : "",
                        _pData.getLogscale(ZRANGE) ? "lg(z)" : "");

    if (_hist2DData[2].Maximal() < 0 && _pData.getLogscale(ZRANGE))
    {
        delete _histGraph;
        throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCmd, SyntaxError::invalid_position);
    }

    if (_pData.getLogscale(ZRANGE))
    {
        if (_hist2DData[2].Minimal() > 0.0)
            _histGraph->SetRanges(_histParams.ranges[XCOORD].min(), _histParams.ranges[XCOORD].max(),
                                  _histParams.ranges[YCOORD].min(), _histParams.ranges[YCOORD].max(),
                                  _hist2DData[2].Minimal(), _hist2DData[2].Maximal());
        else
            _histGraph->SetRanges(_histParams.ranges[XCOORD].min(), _histParams.ranges[XCOORD].max(),
                                  _histParams.ranges[YCOORD].min(), _histParams.ranges[YCOORD].max(),
                                  (1e-2 * _hist2DData[2].Maximal() < 1e-2 ? 1e-2 * _hist2DData[2].Maximal() : 1e-2), _hist2DData[2].Maximal());
    }
    else
        _histGraph->SetRanges(_histParams.ranges[XCOORD].min(), _histParams.ranges[XCOORD].max(),
                              _histParams.ranges[YCOORD].min(), _histParams.ranges[YCOORD].max(),
                              _hist2DData[2].Minimal(), _hist2DData[2].Maximal());

    if (!_pData.getLogscale(XRANGE) && _pData.getTimeAxis(XRANGE).use)
        _histGraph->SetTicksTime('x', 0, _pData.getTimeAxis(XRANGE).sTimeFormat.c_str());

    if (!_pData.getLogscale(YRANGE) && _pData.getTimeAxis(YRANGE).use)
        _histGraph->SetTicksTime('y', 0, _pData.getTimeAxis(YRANGE).sTimeFormat.c_str());

    _histGraph->Box();
    _histGraph->Axis("xyz");
    _histGraph->Colorbar(_pData.getColorScheme("I>").c_str());

    if (_pData.getSettings(PlotData::INT_GRID) == 1)
        _histGraph->Grid("xy", _pData.getGridStyle().c_str());
    else if (_pData.getSettings(PlotData::INT_GRID) == 2)
    {
        _histGraph->Grid("xy!", _pData.getGridStyle().c_str());
        _histGraph->Grid("xy", _pData.getFineGridStyle().c_str());
    }

    _histGraph->Label('x', _histParams.sAxisLabels[XCOORD].c_str(), 0);
    _histGraph->Label('y', _histParams.sAxisLabels[YCOORD].c_str(), 0);

    if (isScatterPlot)
        _histGraph->Dots(_hist2DData[0], _hist2DData[1], _hist2DData[2], _pData.getColorScheme().c_str());
    else
        _histGraph->Dens(_hist2DData[0], _hist2DData[1], _hist2DData[2], _pData.getColorScheme().c_str());

/////////////////////////////// H-BAR PLOT
    _histGraph->MultiPlot(3, 3, 5, 1, 2, "_>");
    _histGraph->SetBarWidth(0.9);
    _histGraph->SetTuneTicks(3, 1.05);
    _histGraph->SetFontSizeCM(0.24 * ((double)(1 + _pData.getSettings(PlotData::FLOAT_TEXTSIZE)) / 6.0), 72);
    _histParams.binWidth[YCOORD] = _hBarHistData.Maximal() - _hBarHistData.Minimal();

    _histGraph->SetRanges(1, 2, 1, 2, 1, 2);
    _histGraph->SetTicks('x');
    _histGraph->SetTicks('y');
    _histGraph->SetFunc(_pData.getLogscale(ZRANGE) ? "lg(x)" : "",
                        _pData.getLogscale(YRANGE) ? "lg(y)" : "");

    if (_hBarHistData.Minimal() >= 0)
    {
        if (_pData.getLogscale(ZRANGE) && _hBarHistData.Maximal() > 0.0)
        {
            if (_hBarHistData.Minimal() - _histParams.binWidth[YCOORD] / 20.0 > 0)
                _histGraph->SetRanges(_hBarHistData.Minimal() - _histParams.binWidth[YCOORD] / 20.0, _hBarHistData.Maximal() + _histParams.binWidth[YCOORD] / 20.0,
                                      _histParams.ranges[YCOORD].min(), _histParams.ranges[YCOORD].max());
            else if (_hBarHistData.Minimal() > 0)
                _histGraph->SetRanges(_hBarHistData.Minimal() / 2.0, _hBarHistData.Maximal() + _histParams.binWidth[YCOORD] / 20.0,
                                      _histParams.ranges[YCOORD].min(), _histParams.ranges[YCOORD].max());
            else
                _histGraph->SetRanges((1e-2 * _hBarHistData.Maximal() < 1e-2 ? 1e-2 * _hBarHistData.Maximal() : 1e-2), _hBarHistData.Maximal() + _histParams.binWidth[YCOORD] / 10.0,
                                      _histParams.ranges[YCOORD].min(), _histParams.ranges[YCOORD].max());
        }
        else if (_hBarHistData.Maximal() < 0.0 && _pData.getLogscale(ZRANGE))
        {
            delete _histGraph;
            throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCmd, SyntaxError::invalid_position);
        }
        else
            _histGraph->SetRanges(0.0, _hBarHistData.Maximal() + _histParams.binWidth[YCOORD] / 20.0,
                                  _histParams.ranges[YCOORD].min(), _histParams.ranges[YCOORD].max());
    }
    else
        _histGraph->SetRanges(_hBarHistData.Minimal() - _histParams.binWidth[YCOORD] / 20.0, _hBarHistData.Maximal() + _histParams.binWidth[YCOORD] / 20.0,
                              _histParams.ranges[YCOORD].min(), _histParams.ranges[YCOORD].max());

    if (!_pData.getLogscale(YRANGE) && _pData.getTimeAxis(YRANGE).use)
        _histGraph->SetTicksTime('y', 0, _pData.getTimeAxis(YRANGE).sTimeFormat.c_str());

    _histGraph->Box();
    _histGraph->Axis("xy");

    if (_pData.getSettings(PlotData::INT_GRID) == 1)
        _histGraph->Grid("xy", _pData.getGridStyle().c_str());
    else if (_pData.getSettings(PlotData::INT_GRID) == 2)
    {
        _histGraph->Grid("xy!", _pData.getGridStyle().c_str());
        _histGraph->Grid("xy", _pData.getFineGridStyle().c_str());
    }

    if (!_histParams.bSum && !_histParams.bAvg)
        _histGraph->Label('x', _histParams.sCountLabel.c_str(), 0);
    else
        _histGraph->Label('x', _histParams.sAxisLabels[ZCOORD].c_str(), 0);

    _histGraph->Label('y', _histParams.sAxisLabels[YCOORD].c_str(), 0);
    _histGraph->Barh(_mAxisVals[1], _hBarHistData, _pData.getColors().c_str());

////////////////////////////////// OUTPUT
    std::string sHistSavePath = _out.getFileName();

    if (_option.systemPrints() && !bSilent)
        NumeReKernel::printPreFmt(toSystemCodePage("|-> " + _lang.get("HIST_GENERATING_PLOT") + " ... "));

    if (_out.isFile())
        sHistSavePath = sHistSavePath.substr(0, sHistSavePath.length() - 4) + ".png";
    else
        sHistSavePath = _option.ValidFileName("<plotpath>/histogramm2d", ".png");

    if (_pData.getSettings(PlotData::LOG_OPENIMAGE) && !_pData.getSettings(PlotData::LOG_SILENTMODE) && !bSilent)
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
        _histGraph->WritePNG(sHistSavePath.c_str(), "", false);
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
/// \param bWriteToCache bool
/// \param bSilent bool
/// \return void
///
/////////////////////////////////////////////////
static void createHist2D(const std::string& sCmd, const std::string& sTargettable, Indices& _idx, Indices& _tIdx, HistogramParameters& _histParams, bool bWriteToCache, bool bSilent)
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

    if (_idx.col.size() > 3 && !_histParams.bAvg)
        _histParams.bSum = true;

    // Update the interval ranges using the minimal and maximal
    // data values in the corresponding spatial directions
    prepareIntervalsForHist(sCmd, _histParams.ranges[XCOORD],
                            _data.min(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1)).real(),
                            _data.max(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1)).real());

    prepareIntervalsForHist(sCmd, _histParams.ranges[YCOORD],
                            _data.min(_histParams.sTable, _idx.row, _idx.col.subidx(1, 1)).real(),
                            _data.max(_histParams.sTable, _idx.row, _idx.col.subidx(1, 1)).real());

    prepareIntervalsForHist(sCmd, _histParams.ranges[ZCOORD],
                            _data.min(_histParams.sTable, _idx.row, _idx.col.subidx(2)).real(),
                            _data.max(_histParams.sTable, _idx.row, _idx.col.subidx(2)).real());

    // Adapt the ranges for logscale
    if (_pData.getLogscale(XRANGE) && _histParams.ranges[XCOORD].max() < 0.0)
        throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCmd, SyntaxError::invalid_position);
    else if (_pData.getLogscale(XRANGE))
    {
        if (_histParams.ranges[XCOORD].min() < 0.0)
            _histParams.ranges[XCOORD].reset(1e-2 * _histParams.ranges[XCOORD].max() < 1e-2 ? 1e-2 * _histParams.ranges[XCOORD].max() : 1e-2,
                                        _histParams.ranges[XCOORD].max());
    }

    if (_pData.getLogscale(YRANGE) && _histParams.ranges[YCOORD].max() < 0.0)
        throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCmd, SyntaxError::invalid_position);
    else if (_pData.getLogscale(YRANGE))
    {
        if (_histParams.ranges[YCOORD].min() < 0.0)
            _histParams.ranges[YCOORD].reset(1e-2 * _histParams.ranges[YCOORD].max() < 1e-2 ? 1e-2 * _histParams.ranges[YCOORD].max() : 1e-2,
                                        _histParams.ranges[YCOORD].max());
    }

    // Count the number of valid entries for determining
    // the number of bins based upon the methods automatically
    for (size_t i = 0; i < _idx.row.size(); i++)
    {
        for (size_t j = 2; j < _idx.col.size(); j++)
        {
            if (_data.isValidElement(_idx.row[i], _idx.col[j], _histParams.sTable)
                && _histParams.ranges[XCOORD].isInside(_data.getElement(_idx.row[i], _idx.col[0], _histParams.sTable).getNum().asCF64())
                && _histParams.ranges[YCOORD].isInside(_data.getElement(_idx.row[i], _idx.col[1], _histParams.sTable).getNum().asCF64()))
                nMax++;
        }
    }

    // Determine the number of bins based upon the
    // selected method
    if (!_histParams.nBin[XCOORD] && _histParams.binWidth[XCOORD] == 0.0)
    {
        double divisor = pow((double)nMax / (double)(_idx.col.size()), 1.0 / 3.0);

        if (_histParams.nMethod == STURGES)
        {
            _histParams.nBin[XCOORD] = (int)rint(1.0 + 3.3 * log10((double)nMax / (double)(_idx.col.size()-2)));
            _histParams.nBin[YCOORD] = _histParams.nBin[XCOORD];
        }
        else if (_histParams.nMethod == SCOTT)
        {
            _histParams.binWidth[XCOORD] = 3.49 * _data.std(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1)).real() / divisor;
            _histParams.binWidth[YCOORD] = 3.49 * _data.std(_histParams.sTable, _idx.row, _idx.col.subidx(1, 1)).real() / divisor;
        }
        else if (_histParams.nMethod == FREEDMAN_DIACONIS)
        {
            _histParams.binWidth[XCOORD] = 2.0 * (_data.pct(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1), 0.75).real()
                                             - _data.pct(_histParams.sTable, _idx.row, _idx.col.subidx(0, 1), 0.25).real()) / divisor;
            _histParams.binWidth[YCOORD] = 2.0 * (_data.pct(_histParams.sTable, _idx.row, _idx.col.subidx(1, 1), 0.75).real()
                                             - _data.pct(_histParams.sTable, _idx.row, _idx.col.subidx(1, 1), 0.25).real()) / divisor;
        }
    }

    // Determine the number of bins based upon the
    // selected bin interval width
    if (!_histParams.nBin[XCOORD])
    {
        if (_histParams.binWidth[XCOORD] > _histParams.ranges[XCOORD].range()
            || _histParams.binWidth[YCOORD] > _histParams.ranges[YCOORD].range())
            throw SyntaxError(SyntaxError::TOO_LARGE_BINWIDTH, sCmd, SyntaxError::invalid_position);

        for (int n = XCOORD; n <= YCOORD; n++)
        {
            for (int i = 0; (i*_histParams.binWidth[n])+_histParams.ranges[n].min() < _histParams.ranges[n].max()+_histParams.binWidth[n]; i++)
            {
                _histParams.nBin[n]++;
            }

            double dDiff = _histParams.nBin[n] * _histParams.binWidth[n] - _histParams.ranges[n].range();
            _histParams.ranges[n].reset(_histParams.ranges[n].min()-dDiff*0.5, _histParams.ranges[n].max()+dDiff+0.5);
        }
    }

    // Determine the bin interval width
    if (_histParams.binWidth[XCOORD] == 0.0)
    {
        // --> Berechne die Intervall-Laenge (Explizite Typumwandlung von int->double) <--
        if (_pData.getLogscale(XRANGE))
            _histParams.binWidth[XCOORD] = (log10(_histParams.ranges[XCOORD].max()) - log10(_histParams.ranges[XCOORD].min())) / (double)_histParams.nBin[XCOORD];
        else
            _histParams.binWidth[XCOORD] = _histParams.ranges[XCOORD].range() / (double)_histParams.nBin[XCOORD];

        // Determine the bin interval width in
        // y direction
        if (_pData.getLogscale(YRANGE))
            _histParams.binWidth[YCOORD] = (log10(_histParams.ranges[YCOORD].max()) - log10(_histParams.ranges[YCOORD].min())) / (double)_histParams.nBin[YCOORD];
        else
            _histParams.binWidth[YCOORD] =_histParams.ranges[YCOORD].range() / (double)_histParams.nBin[YCOORD];
    }

    if (_histParams.binWidth[XCOORD] > _histParams.ranges[XCOORD].range()
        || _histParams.binWidth[YCOORD] > _histParams.ranges[YCOORD].range())
        throw SyntaxError(SyntaxError::TOO_LARGE_BINWIDTH, sCmd, SyntaxError::invalid_position);

    // Calculate the necessary data for the center plot
    calculateDataForCenterPlot(_data, _idx, _histParams, _hist2DData);

    _mAxisVals[0].Create(_histParams.nBin[XCOORD]);
    _mAxisVals[1].Create(_histParams.nBin[YCOORD]);

    // Calculate the necessary data for the bar chart
    // along the y axis of the central plot
    mglData _barHistData = calculateXYHist(_data, _idx, _histParams, &_mAxisVals[0],
                                           _histParams.ranges[XCOORD].min(),
                                           _histParams.ranges[YCOORD].min(), _histParams.ranges[YCOORD].max(), _histParams.binWidth[XCOORD],
                                           _pData.getLogscale(XRANGE), false);

    // Calculate the necessary data for the horizontal
    // bar chart along the x axis of the central plot
    mglData _hBarHistData = calculateXYHist(_data, _idx, _histParams, &_mAxisVals[1],
                                            _histParams.ranges[YCOORD].min(),
                                            _histParams.ranges[XCOORD].min(), _histParams.ranges[XCOORD].max(), _histParams.binWidth[YCOORD],
                                            _pData.getLogscale(YRANGE), true);

    // Format the data for the textual output for the
    // terminal, the text file and store the result in
    // the target table, if desired
    createOutputForHist2D(_data, _idx, sTargettable, _tIdx, _histParams, _mAxisVals, _barHistData, _hBarHistData, bWriteToCache,
                          !bWriteToCache || findParameter(sCmd, "export", '=') || findParameter(sCmd, "save", '='),
                          !_option.systemPrints() || bSilent);

    // Create the three plots as subplots
    createPlotsForHist2D(sCmd, _histParams, _mAxisVals, _barHistData, _hBarHistData, _hist2DData, _idx.col.size() == 3, bSilent);

    _out.reset();
}


/////////////////////////////////////////////////
/// \brief This function is the interface to both
/// the 1D and the 2D histogram generation.
///
/// \param cmdParser& CommandLineParser
/// \return void
///
/////////////////////////////////////////////////
void plugin_histogram(CommandLineParser& cmdParser)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Output& _out = NumeReKernel::getInstance()->getOutput();

	if (!_data.isValid())			// Sind ueberhaupt Daten vorhanden?
        throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, cmdParser.getCommandLine(), cmdParser.getExpr(), cmdParser.getExpr());

    bool bWriteToCache = false;
    bool bMake2DHist = cmdParser.getCommand() == "hist2d";
    bool bSilent = cmdParser.hasParam("silent");
    std::string sCountMethod = cmdParser.getParameterValue("counts");

    if (cmdParser.hasParam("sum"))
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED", cmdParser.getCommandLine()));

    HistogramParameters _histParams;

    _histParams.sTable = "data";
    _histParams.sAxisLabels[XCOORD] = "\\i x";
    _histParams.sAxisLabels[YCOORD] = "\\i y";
    _histParams.sAxisLabels[ZCOORD] = "\\i z";
    _histParams.nMethod = STURGES;
    _histParams.binWidth[XCOORD] = 0;
    _histParams.binWidth[YCOORD] = 0;
    _histParams.nBin[XCOORD] = 0;
    _histParams.nBin[YCOORD] = 0;
    _histParams.ranges = cmdParser.parseIntervals(false);
    _histParams.bGrid = false;
    _histParams.bRelative = sCountMethod == "relative";
    _histParams.bAvg = sCountMethod == "asavg";
    _histParams.bSum = sCountMethod == "accum" || cmdParser.hasParam("sum");
    _histParams.bStoreGrid = cmdParser.hasParam("storegrid");
    _histParams.bBars = !cmdParser.hasParam("nobars");

    // Ensure we have three intervals
    _histParams.ranges.intervals.resize(3);

    Indices _idx;
    Indices _tIdx;

    DataAccessParser _accessParser = cmdParser.getExprAsDataObject();

    if (!_accessParser.getDataObject().length())
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, cmdParser.getCommandLine(), cmdParser.getExpr(), cmdParser.getExpr());

    if (_accessParser.isCluster())
        throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, cmdParser.getCommandLine(),
                          _accessParser.getDataObject() + "{", _accessParser.getDataObject());

    _accessParser.evalIndices();
    _histParams.sTable = _accessParser.getDataObject();
    _idx = _accessParser.getIndices();

    if (_data.isEmpty(_histParams.sTable))
        throw SyntaxError(SyntaxError::NO_CACHED_DATA, cmdParser.getCommandLine(), cmdParser.getExpr(), cmdParser.getExpr());

    if ((cmdParser.hasParam("cols") || cmdParser.hasParam("c")) && !isValidIndexSet(_idx))
    {
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED", cmdParser.getCommandLine()));
        long long int nDataRow = VectorIndex::INVALID;
        long long int nDataRowFinal = 0;

        std::string sTemp;

        if (cmdParser.hasParam("cols"))
            sTemp = cmdParser.getParameterValue("cols");
        else
            sTemp = cmdParser.getParameterValue("c");

        std::string sTemp_2 = "";
        StripSpaces(sTemp);

        if (sTemp.find(':') != std::string::npos)
        {
            int nSep = 0;

            for (size_t i = 0; i < sTemp.length(); i++)
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

    // We can only calculate histograms of columns having a value-like character
    if (!_data.isValueLike(_idx.col, _histParams.sTable))
        throw SyntaxError(SyntaxError::WRONG_COLUMN_TYPE, cmdParser.getCommandLine(), _histParams.sTable+"(", _histParams.sTable);

    mu::Array vResults;

    // Get the number of selected bins
    if (cmdParser.hasParam("bins"))
        vResults = cmdParser.getParsedParameterValue("bins");
    else if (cmdParser.hasParam("b"))
        vResults = cmdParser.getParsedParameterValue("b");

    if (vResults.size())
    {
        _histParams.nBin[XCOORD] = vResults.getAsScalarInt();

        if (vResults.size() > 1)
            _histParams.nBin[YCOORD] = vResults[1].getNum().asI64();
        else
            _histParams.nBin[YCOORD] = _histParams.nBin[XCOORD];

        vResults.clear();
    }

    // Get the selected bin widths
    if (cmdParser.hasParam("width"))
        vResults = cmdParser.getParsedParameterValue("width");
    else if (cmdParser.hasParam("w"))
        vResults = cmdParser.getParsedParameterValue("w");

    if (vResults.size())
    {
        _histParams.binWidth[XCOORD] = vResults.front().getNum().asF64();

        if (vResults.size() > 1)
            _histParams.binWidth[YCOORD] = vResults[1].getNum().asF64();
        else
            _histParams.binWidth[YCOORD] = _histParams.binWidth[XCOORD];

        vResults.clear();
    }

    std::string sTargettable;

    // Find out, whether the user wants a copy of the calculated data
    if (cmdParser.hasParam("target"))
    {
        sTargettable = cmdParser.getTargetTable(_tIdx, "table");
        bWriteToCache = true;
    }
    else
    {
        sTargettable = getParameterValue(cmdParser.getCommandLine(), "tocache", "totable", "");

        if (sTargettable.length())
        {
            NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED", cmdParser.getCommandLine()));
            bWriteToCache = true;

            if (sTargettable.find('(') == std::string::npos)
                sTargettable += "()";

            if (!_data.isTable(sTargettable))
                _data.addTable(sTargettable, NumeReKernel::getInstance()->getSettings());

            sTargettable.erase(sTargettable.find('('));
            _tIdx.row = VectorIndex(0, VectorIndex::OPEN_END);
            _tIdx.col = VectorIndex(_data.getCols(sTargettable), VectorIndex::OPEN_END);
        }
        else if (cmdParser.hasParam("tocache") || cmdParser.hasParam("totable"))
        {
            NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED", cmdParser.getCommandLine()));
            bWriteToCache = true;

            if (!sTargettable.length())
            {
                sTargettable = "table";
                _tIdx.row = VectorIndex(0, VectorIndex::OPEN_END);
                _tIdx.col = VectorIndex(_data.getCols("table"), VectorIndex::OPEN_END);
            }
        }
    }

    // Get the target file name, if any
    if (cmdParser.hasParam("file"))
        _histParams.sSavePath = cmdParser.getFileParameterValueForSaving(".dat", NumeReKernel::getInstance()->getSettings().getSavePath(), "");
    else if (cmdParser.hasParam("save"))
    {
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED", cmdParser.getCommandLine()));
        _histParams.sSavePath = cmdParser.getParsedParameterValueAsString("save", "", true, true);
    }
    else if (cmdParser.hasParam("export"))
    {
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED", cmdParser.getCommandLine()));
        _histParams.sSavePath = cmdParser.getParsedParameterValueAsString("export", "", true, true);
    }

    _out.setStatus(_histParams.sSavePath.length() != 0);

    // Get axis labels
    if (!bMake2DHist)
    {
        if (cmdParser.hasParam("xlabel"))
            _histParams.sBinLabel = cmdParser.getParsedParameterValueAsString("xlabel", "Bins", true, true);
        else
            _histParams.sBinLabel = cmdParser.getParsedParameterValueAsString("binlabel", "Bins", true, true);

        if (cmdParser.hasParam("ylabel"))
            _histParams.sCountLabel = cmdParser.getParsedParameterValueAsString("ylabel", "Counts", true, true);
        else
            _histParams.sCountLabel = cmdParser.getParsedParameterValueAsString("countlabel", "Counts", true, true);

        if (_histParams.bRelative)
            _histParams.sCountLabel += " (rel.)";
    }
    else
    {
        _histParams.sBinLabel = cmdParser.getParsedParameterValueAsString("binlabel", "Bins", true, true);
        _histParams.sCountLabel = cmdParser.getParsedParameterValueAsString("countlabel", "Counts", true, true);
        _histParams.sAxisLabels[XCOORD] = cmdParser.getParsedParameterValueAsString("xlabel", "\\i x", true, true);
        _histParams.sAxisLabels[YCOORD] = cmdParser.getParsedParameterValueAsString("ylabel", "\\i y", true, true);
        _histParams.sAxisLabels[ZCOORD] = cmdParser.getParsedParameterValueAsString("zlabel", "\\i z", true, true);
    }

    // Get the bin estimation method
    std::string sMethod = getParameterValue(cmdParser.getCommandLine(), "method", "m", "");

    if (sMethod == "scott")
        _histParams.nMethod = SCOTT;
    else if (sMethod == "freedman")
        _histParams.nMethod = FREEDMAN_DIACONIS;
    else
        _histParams.nMethod = STURGES;

    // Had to rename the "grid" parameter due to name conflict
    if (cmdParser.hasParam("asgrid") && _idx.col.size() > 3)
        _histParams.bGrid = true;

    // Parse all plotting-specific parameters
    NumeReKernel::getInstance()->getPlottingData().setParams(cmdParser.getParameterList());

    //////////////////////////////////////////////////////////////////////////////////////
    if (bMake2DHist)
        createHist2D(cmdParser.getCommandLine(), sTargettable, _idx, _tIdx, _histParams, bWriteToCache, bSilent);
    else
        createHist1D(cmdParser.getCommandLine(), sTargettable, _idx, _tIdx, _histParams, bWriteToCache, bSilent);

    NumeReKernel::getInstance()->getPlottingData().deleteData(true);
}


