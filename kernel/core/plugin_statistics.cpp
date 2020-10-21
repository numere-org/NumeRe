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

#include <vector>
#include <boost/math/distributions/students_t.hpp>

#include "plugins.hpp"
#include "maths/parser_functions.hpp"
#include "../kernel.hpp"

/*
 * Funktionen zur Berechnung von Mittelwert und Standardabweichung
 */

const std::string PI_MED = "1.1.2";


/////////////////////////////////////////////////
/// \brief This enumeration defines the available
/// statistical values in the vector returned
/// from calcStats().
/////////////////////////////////////////////////
enum StatsFields
{
    STATS_AVG,
    STATS_STD,
    STATS_CONFINT,
    STATS_STDERR,
    STATS_MED,
    STATS_Q1,
    STATS_Q3,
    STATS_RMS,
    STATS_SKEW,
    STATS_EXC,
    STATS_MIN,
    STATS_MAX,
    STATS_NUM,
    STATS_CNT,
    STATS_S_T,
    STATS_FIELD_COUNT
};


/////////////////////////////////////////////////
/// \brief This static function calculates the
/// statistical values for all columns in the
/// passed table and returns them as a vector
/// array.
///
/// \param _data MemoryManager&
/// \param sTable const std::string&
/// \return std::vector<std::vector<double>>
///
/////////////////////////////////////////////////
static std::vector<std::vector<double>> calcStats(MemoryManager& _data, const std::string& sTable)
{
    long long int nLines = _data.getLines(sTable);
    long long int nCols = _data.getCols(sTable);

    std::vector<std::vector<double>> vStats (STATS_FIELD_COUNT, std::vector<double>());

    // Calculate built-in statistical values (short-cuts)
    vStats[STATS_AVG] = _data.avg(sTable, "cols");
    vStats[STATS_STD] = _data.std(sTable, "cols");
    vStats[STATS_MED] = _data.med(sTable, "cols");
    vStats[STATS_Q1]  = _data.pct(sTable, "cols", 0.25);
    vStats[STATS_Q3]  = _data.pct(sTable, "cols", 0.75);
    vStats[STATS_MIN] = _data.min(sTable, "cols");
    vStats[STATS_MAX] = _data.max(sTable, "cols");
    vStats[STATS_NUM] = _data.num(sTable, "cols");
    vStats[STATS_CNT] = _data.cnt(sTable, "cols");
    vStats[STATS_RMS] = _data.norm(sTable, "cols");

    for (long long int j = 0; j < nCols; j++)
    {
        vStats[STATS_CONFINT].push_back(0.0);
        vStats[STATS_SKEW].push_back(0.0);
        vStats[STATS_EXC].push_back(0.0);

        // Calculate Confidence interval count,
        // Skewness and Excess
        for (long long int i = 0; i < nLines; i++)
        {
            if (!_data.isValidElement(i, j, sTable))
                continue;

            if (fabs(_data.getElement(i, j, sTable)) <= vStats[STATS_STD][j])
                vStats[STATS_CONFINT][j]++;

            vStats[STATS_SKEW][j] += intPower(_data.getElement(i, j, sTable) - vStats[STATS_AVG][j], 3);
            vStats[STATS_EXC][j] += intPower(_data.getElement(i, j, sTable) - vStats[STATS_AVG][j], 4);
        }

        // Finalize the confidence interval count
        vStats[STATS_CONFINT][j] /= vStats[STATS_NUM][j];
        vStats[STATS_CONFINT][j] = round(10000.0*vStats[STATS_CONFINT][j]) / 100.0;

        // Finalize Skewness and Excess
        vStats[STATS_SKEW][j] /= vStats[STATS_NUM][j] * intPower(vStats[STATS_STD][j], 3);
        vStats[STATS_EXC][j] /= vStats[STATS_NUM][j] * intPower(vStats[STATS_STD][j], 4);
        vStats[STATS_EXC][j] -= 3.0; // Convert Kurtosis to Excess

        // Calculate 2nd order stats values available
        // from simple arithmetic operations
        vStats[STATS_STDERR].push_back(vStats[STATS_STD][j] / sqrt(vStats[STATS_NUM][j]));
        vStats[STATS_RMS][j] /= sqrt(vStats[STATS_NUM][j]);

        // Use BOOST to calculate the Student-t value for
        // the current number of freedoms
        boost::math::students_t dist(vStats[STATS_NUM][j]);
        vStats[STATS_S_T].push_back(boost::math::quantile(boost::math::complement(dist, 0.025)));
    }
}


static void createStatsFile(Output& _out, const std::vector<std::vector<double>>& vStats, const std::string& sSavePath, MemoryManager& _data, const std::string& sTable, const Settings& _option)
{
    int nLine = _data.getLines(sTable);
    int nCol = _data.getCols(sTable);
    int nHeadlines = _data.getHeadlineCount(sTable);
    const int nPrecision = 4;

    // --> Allozieren einer Ausgabe-String-Matrix <--
    std::string** sOut = new std::string*[nLine + STATS_FIELD_COUNT+1 + nHeadlines];

    for (int i = 0; i < nLine + STATS_FIELD_COUNT+1 + nHeadlines; i++)
    {
        sOut[i] = new std::string[nCol];
    }

    // --> Berechnung fuer jede Spalte der Matrix! <--
    for (int j = 0; j < nCol; j++)
    {
        if (!vStats[STATS_NUM][j])
        {
            sOut[nHeadlines + nLine + 0][j] = "<<SUMBAR>>";
            sOut[nHeadlines + nLine + 1 + STATS_AVG][j] = _lang.get("STATS_TYPE_AVG") + ": ---";
            sOut[nHeadlines + nLine + 1 + STATS_STD][j] = _lang.get("STATS_TYPE_STD") + ": ---";
            sOut[nHeadlines + nLine + 1 + STATS_CONFINT][j] = _lang.get("STATS_TYPE_CONFINT") + ": ---";
            sOut[nHeadlines + nLine + 1 + STATS_STDERR][j] = _lang.get("STATS_TYPE_STDERR") + ": ---";
            sOut[nHeadlines + nLine + 1 + STATS_MED][j] = _lang.get("STATS_TYPE_MED") + ": ---";
            sOut[nHeadlines + nLine + 1 + STATS_Q1][j] = "Q1: ---";
            sOut[nHeadlines + nLine + 1 + STATS_Q3][j] = "Q3: ---";
            sOut[nHeadlines + nLine + 1 + STATS_RMS][j] = _lang.get("STATS_TYPE_RMS") + ": ---";
            sOut[nHeadlines + nLine + 1 + STATS_SKEW][j] = _lang.get("STATS_TYPE_SKEW") + ": ---";
            sOut[nHeadlines + nLine + 1 + STATS_EXC][j] = _lang.get("STATS_TYPE_EXCESS") + ": ---";
            sOut[nHeadlines + nLine + 1 + STATS_MIN][j] = "min: ---";
            sOut[nHeadlines + nLine + 1 + STATS_MAX][j] = "max: ---";
            sOut[nHeadlines + nLine + 1 + STATS_NUM][j] = "num: ---";
            sOut[nHeadlines + nLine + 1 + STATS_CNT][j] = "cnt: ---";
            sOut[nHeadlines + nLine + 1 + STATS_S_T][j] = "s_t: ---";
            continue;
        }

        std::string sHeadline = _data.getHeadLineElement(j, sTable);

        for (int i = 0; i < nHeadlines; i++)
        {
            if (sHeadline.length())
            {
                sOut[i][j] = sHeadline.substr(0, sHeadline.find("\\n"));

                if (sHeadline.find("\\n") != std::string::npos)
                    sHeadline.erase(0, sHeadline.find("\\n") + 2);
                else
                    break;
            }
        }


        for (int i = 0; i < nLine; i++)
        {
            if (!_data.isValidElement(i,j, sTable))
            {
                sOut[i + nHeadlines][j] = "---";
                continue;
            }

            sOut[i + nHeadlines][j] = toString(_data.getElement(i,j, sTable), _option); // Kopieren der Matrix in die Ausgabe
        }

        sOut[nHeadlines + nLine + 0][j] = "<<SUMBAR>>"; // Schreiben der berechneten Werte in die letzten drei Zeilen der Ausgabe
        sOut[nHeadlines + nLine + 1 + STATS_AVG][j] = _lang.get("STATS_TYPE_AVG") + ": " + toString(vStats[STATS_AVG][j], nPrecision);
        sOut[nHeadlines + nLine + 1 + STATS_STD][j] = _lang.get("STATS_TYPE_STD") + ": " + toString(vStats[STATS_STD][j], nPrecision);
        sOut[nHeadlines + nLine + 1 + STATS_CONFINT][j] = _lang.get("STATS_TYPE_CONFINT") + ": " + toString(vStats[STATS_CONFINT][j], nPrecision) + " %";
        sOut[nHeadlines + nLine + 1 + STATS_STDERR][j] = _lang.get("STATS_TYPE_STDERR") + ": " + toString(vStats[STATS_STDERR][j], nPrecision);
        sOut[nHeadlines + nLine + 1 + STATS_MED][j] = _lang.get("STATS_TYPE_MED") + ": " + toString(vStats[STATS_MED][j], nPrecision);
        sOut[nHeadlines + nLine + 1 + STATS_Q1][j] = "Q1: " + toString(vStats[STATS_Q1][j], nPrecision);
        sOut[nHeadlines + nLine + 1 + STATS_Q3][j] = "Q3: " + toString(vStats[STATS_Q3][j], nPrecision);
        sOut[nHeadlines + nLine + 1 + STATS_RMS][j] = _lang.get("STATS_TYPE_RMS") + ": " + toString(vStats[STATS_RMS][j], nPrecision);
        sOut[nHeadlines + nLine + 1 + STATS_SKEW][j] = _lang.get("STATS_TYPE_SKEW") + ": " + toString(vStats[STATS_SKEW][j], nPrecision);
        sOut[nHeadlines + nLine + 1 + STATS_EXC][j] = _lang.get("STATS_TYPE_EXCESS") + ": " + toString(vStats[STATS_EXC][j], nPrecision);
        sOut[nHeadlines + nLine + 1 + STATS_MIN][j] = "min: " + toString(vStats[STATS_MIN][j], nPrecision);
        sOut[nHeadlines + nLine + 1 + STATS_MAX][j] = "max: " + toString(vStats[STATS_MAX][j], nPrecision);
        sOut[nHeadlines + nLine + 1 + STATS_NUM][j] = "num: " + toString(vStats[STATS_NUM][j], nPrecision);
        sOut[nHeadlines + nLine + 1 + STATS_CNT][j] = "cnt: " + toString(vStats[STATS_CNT][j], nPrecision);
        sOut[nHeadlines + nLine + 1 + STATS_S_T][j] = "s_t: " + toString(vStats[STATS_S_T][j], nPrecision);
    }

    // --> Allgemeine Ausgabe-Info-Parameter setzen <--
    _out.setPluginName(_lang.get("STATS_OUT_PLGNINFO", PI_MED, _data.getDataFileName(sTable)));
    _out.setPrefix("stats");

    _out.setCompact(false);
    _out.setCommentLine(_lang.get("STATS_OUT_COMMENTLINE"));

    _out.format(sOut, nCol, nLine + STATS_FIELD_COUNT+1 + nHeadlines, _option, true, nHeadlines);

    for (int i = 0; i < nLine + STATS_FIELD_COUNT+1 + nHeadlines; i++)
    {
        delete[] sOut[i];
    }

    delete[] sOut;

    _out.reset();
}


static void createStatsOutput(Output& _out, const std::vector<std::vector<double>>& vStats, const std::string& sSavePath, MemoryManager& _data, const std::string& sTable, const Settings& _option)
{
    int nCol = _data.getCols(sTable);
    int nHeadlines = _data.getHeadlineCount(sTable);
    const int nPrecision = 4;

    if (_out.isFile())
        createStatsFile(_out, vStats, sSavePath, _data, sTable, _option);

    std::string** sOverview = new std::string*[STATS_FIELD_COUNT + nHeadlines];

    for (int i = 0; i < STATS_FIELD_COUNT+nHeadlines; i++)
        sOverview[i] = new std::string[nCol+1];

    sOverview[0][0] = " ";

    for (int j = 0; j < nCol; j++)
    {
        std::string sHeadline = _data.getHeadLineElement(j, sTable);

        for (int i = 0; i < nHeadlines; i++)
        {
            if (sHeadline.length())
            {
                sOverview[i][j+1] = sHeadline.substr(0, sHeadline.find("\\n"));

                if (sHeadline.find("\\n") != std::string::npos)
                    sHeadline.erase(0, sHeadline.find("\\n") + 2);
                else
                    break;
            }
        }

        if (!j)
        {
            sOverview[nHeadlines + STATS_AVG][j] = _lang.get("STATS_TYPE_AVG") + ":";
            sOverview[nHeadlines + STATS_STD][j] = _lang.get("STATS_TYPE_STD") + ":";
            sOverview[nHeadlines + STATS_CONFINT][j] = _lang.get("STATS_TYPE_CONFINT") + ":";
            sOverview[nHeadlines + STATS_STDERR][j] = _lang.get("STATS_TYPE_STDERR") + ":";
            sOverview[nHeadlines + STATS_MED][j] = _lang.get("STATS_TYPE_MED") + ":";
            sOverview[nHeadlines + STATS_Q1][j] = "Q1:";
            sOverview[nHeadlines + STATS_Q3][j] = "Q3:";
            sOverview[nHeadlines + STATS_RMS][j] = _lang.get("STATS_TYPE_RMS") + ":";
            sOverview[nHeadlines + STATS_SKEW][j] = _lang.get("STATS_TYPE_SKEW") + ":";
            sOverview[nHeadlines + STATS_EXC][j] = _lang.get("STATS_TYPE_EXCESS") + ":";
            sOverview[nHeadlines + STATS_MIN][j] = "min:";
            sOverview[nHeadlines + STATS_MAX][j] = "max:";
            sOverview[nHeadlines + STATS_NUM][j] = "num:";
            sOverview[nHeadlines + STATS_CNT][j] = "cnt:";
            sOverview[nHeadlines + STATS_S_T][j] = "s_t:";
        }

        for (int n = STATS_AVG; n < STATS_FIELD_COUNT; n++)
        {
            if (n == STATS_CONFINT)
                sOverview[nHeadlines + n][j+1] = toString(vStats[n][j], nPrecision) + " %";
            else
                sOverview[nHeadlines + n][j+1] = toString(vStats[n][j], nPrecision);
        }
    }

    _out.setCompact(false);
    _out.setCommentLine(_lang.get("STATS_OUT_COMMENTLINE"));

    NumeReKernel::toggleTableStatus();
    make_hline();
    NumeReKernel::print("NUMERE: " + toSystemCodePage(toUpperCase(_lang.get("STATS_HEADLINE"))));
    make_hline();
    _out.format(sOverview, nCol+1, STATS_FIELD_COUNT+nHeadlines, _option, true, nHeadlines);
    _out.reset();
    NumeReKernel::toggleTableStatus();
    make_hline();

    // --> Speicher wieder freigeben! <--
    for (int i = 0; i < STATS_FIELD_COUNT+nHeadlines; i++)
        delete[] sOverview[i];

    delete[] sOverview;

    // --> Output-Instanz wieder zuruecksetzen <--
    _out.reset();

}


void plugin_statistics(std::string& sCmd, MemoryManager& _data)
{
    MemoryManager& _rootData = NumeReKernel::getInstance()->getMemoryManager();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    Indices _idx;

    std::string sSavePath = "";  // Variable fuer den Speicherpfad
    std::string sTarget = evaluateTargetOptionInCommand(sCmd, "table", _idx, NumeReKernel::getInstance()->getParser(), _rootData, _option);

    if (!_data.isValid())	// Sind eigentlich Daten verfuegbar?
        throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);

    if (findParameter(sCmd, "save", '=') || findParameter(sCmd, "export", '='))
    {
        int nPos = 0;

        if (findParameter(sCmd, "save", '='))
            nPos = findParameter(sCmd, "save", '=')+4;
        else
            nPos = findParameter(sCmd, "export", '=')+6;

        _out.setStatus(true);
        sSavePath = getArgAtPos(sCmd, nPos);
    }

    if (findParameter(sCmd, "save") || findParameter(sCmd, "export"))
        _out.setStatus(true);

    std::string sDatatable = "data";

    if (_data.matchTableAsParameter(sCmd).length())
        sDatatable = _data.matchTableAsParameter(sCmd);

    if (!_data.getLines(sDatatable) || !_data.getCols(sDatatable))
        throw SyntaxError(SyntaxError::NO_CACHED_DATA, sCmd, SyntaxError::invalid_position);

    std::vector<std::vector<double>> vStats = calcStats(_data, sDatatable);

    for (size_t i = 0; i < vStats.size(); i++)
    {
        for (size_t j = 0; j < vStats[i].size(); j++)
        {
            if (!i && j < _idx.col.size())
                _rootData.setHeadLineElement(_idx.col[j], sTarget, _data.getHeadLineElement(j, sDatatable));

            if (i < _idx.row.size() && j < _idx.col.size())
                _rootData.writeToTable(_idx.row[i], _idx.col[j], sTarget, vStats[i][j]);
        }
    }

    createStatsOutput(_out, vStats, sSavePath, _data, sDatatable, _option);
}


