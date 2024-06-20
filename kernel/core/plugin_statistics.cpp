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

#include "plugins.hpp"
#include "maths/parser_functions.hpp"
#include "maths/student_t.hpp"
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
/// \param _idx const Indices&
/// \return std::vector<std::vector<double>>
///
/////////////////////////////////////////////////
static std::vector<std::vector<double>> calcStats(MemoryManager& _data, const std::string& sTable, const Indices& _idx)
{
    std::vector<std::vector<double>> vStats (STATS_FIELD_COUNT, std::vector<double>());

    for (size_t j = 0; j < _idx.col.size(); j++)
    {
        // Calculate built-in statistical values (short-cuts)
        vStats[STATS_AVG].push_back(_data.avg(sTable, _idx.row, _idx.col.subidx(j, 1)).real());
        vStats[STATS_STD].push_back(_data.std(sTable, _idx.row, _idx.col.subidx(j, 1)).real());
        vStats[STATS_MED].push_back(_data.med(sTable, _idx.row, _idx.col.subidx(j, 1)).real());
        vStats[STATS_Q1].push_back(_data.pct(sTable, _idx.row, _idx.col.subidx(j, 1), 0.25).real());
        vStats[STATS_Q3].push_back(_data.pct(sTable, _idx.row, _idx.col.subidx(j, 1), 0.75).real());
        vStats[STATS_MIN].push_back(_data.min(sTable, _idx.row, _idx.col.subidx(j, 1)).real());
        vStats[STATS_MAX].push_back(_data.max(sTable, _idx.row, _idx.col.subidx(j, 1)).real());
        vStats[STATS_NUM].push_back(_data.num(sTable, _idx.row, _idx.col.subidx(j, 1)).real());
        vStats[STATS_CNT].push_back(_data.cnt(sTable, _idx.row, _idx.col.subidx(j, 1)).real());
        vStats[STATS_RMS].push_back(_data.norm(sTable, _idx.row, _idx.col.subidx(j, 1)).real());

        // Many values make no sense if no data
        // is available
        if (!vStats[STATS_NUM].back())
        {
            vStats[STATS_CONFINT].push_back(NAN);
            vStats[STATS_SKEW].push_back(NAN);
            vStats[STATS_EXC].push_back(NAN);
            vStats[STATS_STDERR].push_back(NAN);
            vStats[STATS_S_T].push_back(NAN);
            vStats[STATS_STD].back() = NAN;
            vStats[STATS_RMS].back() = NAN;
            continue;
        }

        vStats[STATS_CONFINT].push_back(0.0);
        vStats[STATS_SKEW].push_back(0.0);
        vStats[STATS_EXC].push_back(0.0);

        // Calculate Confidence interval count,
        // Skewness and Excess
        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            if (!_data.isValidElement(_idx.row[i], _idx.col[j], sTable))
                continue;

            double val = _data.getElement(_idx.row[i], _idx.col[j], sTable).real();

            if (fabs(val - vStats[STATS_AVG].back()) <= vStats[STATS_STD].back())
                vStats[STATS_CONFINT].back()++;

            vStats[STATS_SKEW].back() += intPower(val - vStats[STATS_AVG].back(), 3);
            vStats[STATS_EXC].back() += intPower(val - vStats[STATS_AVG].back(), 4);
        }

        // Finalize the confidence interval count
        vStats[STATS_CONFINT].back() /= vStats[STATS_NUM].back();
        vStats[STATS_CONFINT].back() = round(10000.0*vStats[STATS_CONFINT].back()) / 100.0;

        // Finalize Skewness and Excess
        vStats[STATS_SKEW].back() /= vStats[STATS_NUM].back() * intPower(vStats[STATS_STD].back(), 3);
        vStats[STATS_EXC].back() /= vStats[STATS_NUM].back() * intPower(vStats[STATS_STD].back(), 4);
        vStats[STATS_EXC].back() -= 3.0; // Convert Kurtosis to Excess

        // Calculate 2nd order stats values available
        // from simple arithmetic operations
        vStats[STATS_STDERR].push_back(vStats[STATS_STD].back() / sqrt(vStats[STATS_NUM].back()));
        vStats[STATS_RMS].back() /= sqrt(vStats[STATS_NUM].back());

        // Use BOOST to calculate the Student-t value for
        // the current number of freedoms
        vStats[STATS_S_T].push_back(student_t(vStats[STATS_NUM].back(), 0.95));
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
/// \param _idx const Indices&
/// \param _option const Settings&
/// \return void
///
/////////////////////////////////////////////////
static void createStatsFile(Output& _out, const std::vector<std::vector<double>>& vStats, const std::string& sSavePath, MemoryManager& _data, const std::string& sTable, const Indices& _idx, const Settings& _option)
{
    int nLine = vStats.size();
    int nCol = vStats[0].size();
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
        std::string sHeadline = _data.getHeadLineElement(_idx.col[j], sTable);

        for (int i = 0; i < nHeadlines; i++)
        {
            if (sHeadline.length())
            {
                sOut[i][j] = sHeadline.substr(0, sHeadline.find('\n'));

                if (sHeadline.find('\n') != std::string::npos)
                    sHeadline.erase(0, sHeadline.find('\n') + 1);
                else
                    break;
            }
        }

        // Write the table values to the single columns
        for (int i = 0; i < nLine; i++)
        {
            if (!_data.isValidElement(_idx.row[i], _idx.col[j], sTable))
            {
                sOut[i + nHeadlines][j] = "---";
                continue;
            }

            sOut[i + nHeadlines][j] = toString(_data.getElement(_idx.row[i], _idx.col[j], sTable), _option.getPrecision()); // Kopieren der Matrix in die Ausgabe
        }

        // Write the calculated stats to the columns
        sOut[nHeadlines + nLine + 0][j] = "<<SUMBAR>>"; // Schreiben der berechneten Werte in die letzten drei Zeilen der Ausgabe

        for (int n = STATS_AVG; n < STATS_FIELD_COUNT; n++)
            sOut[nHeadlines + nLine + 1 + n][j] = getStatFieldName(n) + ": " + toString(vStats[n][j], nPrecision);
    }

    // --> Allgemeine Ausgabe-Info-Parameter setzen <--
    _out.setFileName(sSavePath);
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
/// \param _idx const Indices&
/// \param _option const Settings&
/// \return void
///
/////////////////////////////////////////////////
static void createStatsOutput(Output& _out, const std::vector<std::vector<double>>& vStats, const std::string& sSavePath, MemoryManager& _data, const std::string& sTable, const Indices& _idx, const Settings& _option)
{
    int nCol = vStats[0].size();
    int nHeadlines = _data.getHeadlineCount(sTable);
    const int nPrecision = 4;

    // Redirect the control, if necessary
    if (_out.isFile())
        createStatsFile(_out, vStats, sSavePath, _data, sTable, _idx, _option);

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
        std::string sHeadline = _data.getHeadLineElement(_idx.col[j], sTable);
        std::string sUnit = _data.getUnit(_idx.col[j], sTable);

        if (sUnit.length())
            sUnit.insert(0, 1, ' ');

        for (int i = 0; i < nHeadlines; i++)
        {
            if (sHeadline.length())
            {
                sOverview[i][j+1] = sHeadline.substr(0, sHeadline.find('\n'));

                if (sHeadline.find('\n') != std::string::npos)
                    sHeadline.erase(0, sHeadline.find('\n') + 1);
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
                sOverview[nHeadlines + n][j+1] = "---";
            }

            continue;
        }

        // Write the actual values to the string table
        for (int n = STATS_AVG; n < STATS_FIELD_COUNT; n++)
        {
            if (n == STATS_CONFINT)
                sOverview[nHeadlines + n][j+1] = toString(vStats[n][j], nPrecision) + " %";
            else if (n == STATS_CNT || n == STATS_NUM || n == STATS_S_T || n == STATS_EXC || n == STATS_SKEW)
                sOverview[nHeadlines + n][j+1] = toString(vStats[n][j], nPrecision);
            else
                sOverview[nHeadlines + n][j+1] = toString(vStats[n][j], nPrecision) + sUnit;
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
/// \param cmdParser CommandLineParser&
/// \return void
///
/////////////////////////////////////////////////
void plugin_statistics(CommandLineParser& cmdParser)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Output& _out = NumeReKernel::getInstance()->getOutput();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    Indices _idx;

    std::string sSavePath;
    DataAccessParser accessParser = cmdParser.getExprAsDataObject();

    if (!accessParser.getDataObject().length())
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, cmdParser.getCommandLine(), cmdParser.getExpr(), cmdParser.getExpr());

    std::string sDatatable = accessParser.getDataObject();

    if (accessParser.isCluster())
        throw SyntaxError(SyntaxError::CACHE_DOESNT_EXIST, cmdParser.getCommandLine(), sDatatable + "{", sDatatable);

    accessParser.evalIndices();

    // Get the target table, if the user specified one,
    // otherwise just leave it empty
    std::string sTarget = cmdParser.getTargetTable(_idx, "");

    // Ensure that at least some data is available
    if (!_data.isValid())
        throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, cmdParser.getCommandLine(), sDatatable + "(", sDatatable);

    // Get the target file name, if any
    if (cmdParser.hasParam("file"))
        sSavePath = cmdParser.getFileParameterValueForSaving(".dat", NumeReKernel::getInstance()->getSettings().getSavePath(), "");
    else if (cmdParser.hasParam("save"))
    {
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED", cmdParser.getCommandLine()));
        sSavePath = cmdParser.getParameterValueAsString("save", "", true, true);
    }
    else if (cmdParser.hasParam("export"))
    {
        NumeReKernel::issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED", cmdParser.getCommandLine()));
        sSavePath = cmdParser.getParameterValueAsString("export", "", true, true);
    }

    _out.setStatus(sSavePath.length() != 0);

    // Ensure that the table is not empty
    if (_data.isEmpty(sDatatable))
        throw SyntaxError(SyntaxError::NO_CACHED_DATA, cmdParser.getCommandLine(), sDatatable + "(", sDatatable);

    // Calculate the statistics
    std::vector<std::vector<double>> vStats = calcStats(_data, sDatatable, accessParser.getIndices());

    // Write the statistics to the target table, if a
    // target table was specified
    if (sTarget.length())
    {
        for (size_t i = 0; i < vStats.size(); i++)
        {
            for (size_t j = 0; j < vStats[i].size(); j++)
            {
                if (!i && j < _idx.col.size())
                    _data.setHeadLineElement(_idx.col[j+1], sTarget, _data.getHeadLineElement(accessParser.getIndices().col[j], sDatatable));

                if (!vStats[STATS_NUM][j])
                    continue;

                if (i < _idx.row.size() && j < _idx.col.size())
                    _data.writeToTable(_idx.row[i], _idx.col[j+1], sTarget, vStats[i][j]);
            }
        }

        std::vector<std::string> sRet;
        _data.convertColumns(sTarget, _idx.col.subidx(0, 1), "string");
        _data.setHeadLineElement(_idx.col[0], sTarget, "Stats");

        for (int n = STATS_AVG; n < STATS_FIELD_COUNT; n++)
        {
            sRet.push_back("\"" + getStatFieldName(n) + "\"");
            _data.writeToTable(_idx.row[n], _idx.col[0], sTarget, getStatFieldName(n));
        }

        cmdParser.setReturnValue(sRet);
    }
    else
        cmdParser.clearReturnValue();

    // Create the output for the terminal and the file,
    // if necessary
    createStatsOutput(_out, vStats, sSavePath, _data, sDatatable, accessParser.getIndices(), _option);
}


