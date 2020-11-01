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
        // Many values make no sense if no data
        // is available
        if (!vStats[STATS_NUM][j])
        {
            vStats[STATS_CONFINT].push_back(NAN);
            vStats[STATS_SKEW].push_back(NAN);
            vStats[STATS_EXC].push_back(NAN);
            vStats[STATS_STDERR].push_back(NAN);
            vStats[STATS_S_T].push_back(NAN);
            vStats[STATS_STD][j] = NAN;
            vStats[STATS_RMS][j] = NAN;
            continue;
        }

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

    return vStats;
}


/////////////////////////////////////////////////
/// \brief This static function maps the
/// statistical value enumerators to strings used
/// for the tables and the return value.
///
/// \param nStatField int
/// \return std::string
///
/////////////////////////////////////////////////
static std::string getStatFieldName(int nStatField)
{
    switch (nStatField)
    {
        case STATS_AVG:
            return _lang.get("STATS_TYPE_AVG");
        case STATS_STD:
            return _lang.get("STATS_TYPE_STD");
        case STATS_CONFINT:
            return _lang.get("STATS_TYPE_CONFINT");
        case STATS_STDERR:
            return _lang.get("STATS_TYPE_STDERR");
        case STATS_MED:
            return _lang.get("STATS_TYPE_MED");
        case STATS_Q1:
            return "Q1";
        case STATS_Q3:
            return "Q3";
        case STATS_RMS:
            return _lang.get("STATS_TYPE_RMS");
        case STATS_SKEW:
            return _lang.get("STATS_TYPE_SKEW");
        case STATS_EXC:
            return _lang.get("STATS_TYPE_EXCESS");
        case STATS_MIN:
            return "min";
        case STATS_MAX:
            return "max";
        case STATS_NUM:
            return "num";
        case STATS_CNT:
            return "cnt";
        case STATS_S_T:
            return "s_t";
        case STATS_FIELD_COUNT:
            return "";
    }

    return "";
}


/////////////////////////////////////////////////
/// \brief This static function will create the
/// output file using the functionalities of the
/// Output class.
///
/// \param _out Output&
/// \param vStats const std::vector<std::vector<double>>&
/// \param sSavePath const std::string&
/// \param _data MemoryManager&
/// \param sTable const std::string&
/// \param _option const Settings&
/// \return void
///
/////////////////////////////////////////////////
static void createStatsFile(Output& _out, const std::vector<std::vector<double>>& vStats, const std::string& sSavePath, MemoryManager& _data, const std::string& sTable, const Settings& _option)
{
    int nLine = _data.getLines(sTable);
    int nCol = _data.getCols(sTable);
    int nHeadlines = _data.getHeadlineCount(sTable);
    const int nPrecision = 4;

    // Create an output string matrix on the heap
    std::string** sOut = new std::string*[nLine + STATS_FIELD_COUNT+1 + nHeadlines];

    for (int i = 0; i < nLine + STATS_FIELD_COUNT+1 + nHeadlines; i++)
    {
        sOut[i] = new std::string[nCol];
    }

    // Fill the output matrix with the
    // previously calculated values
    for (int j = 0; j < nCol; j++)
    {
        // Write an empty column, if no values are available
        if (!vStats[STATS_NUM][j])
        {
            sOut[nHeadlines + nLine + 0][j] = "<<SUMBAR>>";

            for (int n = STATS_AVG; n < STATS_FIELD_COUNT; n++)
                sOut[nHeadlines + nLine + 1 + n][j] = getStatFieldName(n) + ": ---";

            continue;
        }

        // Add the headlines to the columns
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

        // Write the table values to the single columns
        for (int i = 0; i < nLine; i++)
        {
            if (!_data.isValidElement(i,j, sTable))
            {
                sOut[i + nHeadlines][j] = "---";
                continue;
            }

            sOut[i + nHeadlines][j] = toString(_data.getElement(i,j, sTable), _option); // Kopieren der Matrix in die Ausgabe
        }

        // Write the calculated stats to the columns
        sOut[nHeadlines + nLine + 0][j] = "<<SUMBAR>>"; // Schreiben der berechneten Werte in die letzten drei Zeilen der Ausgabe

        for (int n = STATS_AVG; n < STATS_FIELD_COUNT; n++)
            sOut[nHeadlines + nLine + 1 + n][j] = getStatFieldName(n) + ": " + toString(vStats[n][j], nPrecision);
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


/////////////////////////////////////////////////
/// \brief This static function creates the
/// output table for the terminal. The table will
/// get formatted using the Output class.
///
/// This function will also redirect the control
/// to createStatsFile if a file shall be created.
///
/// \param _out Output&
/// \param vStats const std::vector<std::vector<double>>&
/// \param sSavePath const std::string&
/// \param _data MemoryManager&
/// \param sTable const std::string&
/// \param _option const Settings&
/// \return void
///
/////////////////////////////////////////////////
static void createStatsOutput(Output& _out, const std::vector<std::vector<double>>& vStats, const std::string& sSavePath, MemoryManager& _data, const std::string& sTable, const Settings& _option)
{
    int nCol = _data.getCols(sTable);
    int nHeadlines = _data.getHeadlineCount(sTable);
    const int nPrecision = 4;

    // Redirect the control, if necessary
    if (_out.isFile())
        createStatsFile(_out, vStats, sSavePath, _data, sTable, _option);

    // Create the overview string table
    // on the heap
    std::string** sOverview = new std::string*[STATS_FIELD_COUNT + nHeadlines];

    for (int i = 0; i < STATS_FIELD_COUNT+nHeadlines; i++)
        sOverview[i] = new std::string[nCol+1];

    sOverview[0][0] = " ";

    // Write the calculated statistics to the
    // string table
    for (int j = 0; j < nCol; j++)
    {
        // Write the table column headlines
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

        // Write the first column with table row names
        if (!j)
        {
            for (int n = STATS_AVG; n < STATS_FIELD_COUNT; n++)
                sOverview[nHeadlines + n][j] = getStatFieldName(n) + ":";
        }

        // Write an empty column, if no values are available
        if (!vStats[STATS_NUM][j])
        {
            for (int n = STATS_AVG; n < STATS_FIELD_COUNT; n++)
            {
                if (n == STATS_CONFINT)
                    sOverview[nHeadlines + n][j+1] = "--- %";
                else
                    sOverview[nHeadlines + n][j+1] = "---";
            }

            continue;
        }

        // Write the actual values to the string table
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


/////////////////////////////////////////////////
/// \brief This is the implementation of the
/// stats command.
///
/// \param sCmd std::string&
/// \param _data MemoryManager& Might be different from the usual MemoryManager
/// \return std::string
///
/////////////////////////////////////////////////
std::string plugin_statistics(std::string& sCmd, MemoryManager& _data)
{
    MemoryManager& _rootData = NumeReKernel::getInstance()->getMemoryManager();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    Indices _idx;

    std::string sSavePath;
    std::string sRet;

    // Get the target table, if the user specified one,
    // otherwise just leave it empty
    std::string sTarget = evaluateTargetOptionInCommand(sCmd, "", _idx, NumeReKernel::getInstance()->getParser(), _rootData, _option);

    // Ensure that at least some data is available
    if (!_data.isValid())
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

    // Ensure that the table is not empty
    if (_data.isEmpty(sDatatable))
        throw SyntaxError(SyntaxError::NO_CACHED_DATA, sCmd, SyntaxError::invalid_position);

    // Calculate the statistics
    std::vector<std::vector<double>> vStats = calcStats(_data, sDatatable);

    // Write the statistics to the target table, if a
    // target table was specified
    if (sTarget.length())
    {
        for (size_t i = 0; i < vStats.size(); i++)
        {
            for (size_t j = 0; j < vStats[i].size(); j++)
            {
                if (!i && j < _idx.col.size())
                    _rootData.setHeadLineElement(_idx.col[j], sTarget, _data.getHeadLineElement(j, sDatatable));

                if (!vStats[STATS_NUM][j])
                    continue;

                if (i < _idx.row.size() && j < _idx.col.size())
                    _rootData.writeToTable(_idx.row[i], _idx.col[j], sTarget, vStats[i][j]);
            }
        }

        sRet = "{";

        for (int n = STATS_AVG; n < STATS_FIELD_COUNT; n++)
        {
            sRet += "\"" + getStatFieldName(n) + "\",";
        }

        sRet.back() = '}';
    }

    // Create the output for the terminal and the file,
    // if necessary
    createStatsOutput(_out, vStats, sSavePath, _data, sDatatable, _option);

    return sRet;
}


