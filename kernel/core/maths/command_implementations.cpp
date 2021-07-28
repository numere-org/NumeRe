/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

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

#include <gsl/gsl_statistics.h>
#include <gsl/gsl_sort.h>
#include <algorithm>
#include <memory>

#include "command_implementations.hpp"
#include "parser_functions.hpp"
#include "matrixoperations.hpp"
#include "spline.h"
#include "wavelet.hpp"
#include "../AudioLib/audiofile.hpp"
#include "../../kernel.hpp"
#include "../../../common/http.h"

#define TRAPEZOIDAL 1
#define SIMPSON 2

DefaultVariables _defVars;
static double localizeExtremum(string& sCmd, double* dVarAdress, Parser& _parser, const Settings& _option, double dLeft, double dRight, double dEps = 1e-10, int nRecursion = 0);
static double localizeZero(string& sCmd, double* dVarAdress, Parser& _parser, const Settings& _option, double dLeft, double dRight, double dEps = 1e-10, int nRecursion = 0);
static std::vector<size_t> getSamplesForDatagrid(CommandLineParser& cmdParser, size_t nSamples);
static void expandVectorToDatagrid(IntervalSet& ivl, std::vector<std::vector<double>>& vZVals, size_t nSamples_x, size_t nSamples_y);


/////////////////////////////////////////////////
/// \brief This static function performs an
/// integration step using a trapezoidal
/// approximation algorithm.
///
/// \param x double&
/// \param x0 double
/// \param dx double
/// \param vResult vector<double>&
/// \param vFunctionValues vector<double>&
/// \param bReturnFunctionPoints bool
/// \return void
///
/////////////////////////////////////////////////
static void integrationstep_trapezoidal(double& x, double x0, double dx, vector<double>& vResult, vector<double>& vFunctionValues, bool bReturnFunctionPoints)
{
    x = x0;
    int nResults;
    mu::value_type* v = NumeReKernel::getInstance()->getParser().Eval(nResults);

    // Evaluate the current integration step for each of the
    // defined functions
    for (int i = 0; i < nResults; i++)
    {
        if (isnan(v[i]))
            v[i] = 0.0;
    }

    // Now calculate the area below the curve
    if (!bReturnFunctionPoints)
    {
        for (int i = 0; i < nResults; i++)
            vResult[i] += dx * (vFunctionValues[i] + v[i]) * 0.5;
    }
    else
    {
        // Calculate the integral
        if (vResult.size())
            vResult.push_back(dx * (vFunctionValues[0] + v[0]) * 0.5 + vResult.back());
        else
            vResult.push_back(dx * (vFunctionValues[0] + v[0]) * 0.5);
    }

    // Set the last sample as the first one
    vFunctionValues.assign(v, v+nResults);
}


/////////////////////////////////////////////////
/// \brief This static function performs an
/// integration step using the Simpson
/// approximation algorithm.
///
/// \param x double&
/// \param x0 double
/// \param x1 double
/// \param dx double
/// \param vResult vector<double>&
/// \param vFunctionValues vector<double>&
/// \param bReturnFunctionPoints bool
/// \return void
///
/////////////////////////////////////////////////
static void integrationstep_simpson(double& x, double x0, double x1, double dx, vector<double>& vResult, vector<double>& vFunctionValues, bool bReturnFunctionPoints)
{
    // Evaluate the intermediate function value
    x = x0;
    int nResults;
    mu::value_type* v = NumeReKernel::getInstance()->getParser().Eval(nResults);

    for (int i = 0; i < nResults; i++)
    {
        if (isnan(v[i]))
            v[i] = 0.0;
    }

    vector<double> vInter(v, v+nResults);

    // Evaluate the end function value
    x = x1;
    v = NumeReKernel::getInstance()->getParser().Eval(nResults);

    for (int i = 0; i < nResults; i++)
    {
        if (isnan(v[i]))
            v[i] = 0.0;
    }

    // Now calculate the area below the curve
    if (!bReturnFunctionPoints)
    {
        for (int i = 0; i < nResults; i++)
            vResult[i] += dx / 6.0 * (vFunctionValues[i] + 4.0 * vInter[i] + v[i]); // b-a/6*(f(a)+4f(a+b/2)+f(b))
    }
    else
    {
        // Calculate the integral at the current x position
        if (vResult.size())
            vResult.push_back(dx / 6.0 * (vFunctionValues[0] + 4.0 * vInter[0] + v[0]) + vResult.back());
        else
            vResult.push_back(dx / 6.0 * (vFunctionValues[0] + 4.0 * vInter[0] + v[0]));
    }

    // Set the last sample as the first one
    vFunctionValues.assign(v, v+nResults);
}


/////////////////////////////////////////////////
/// \brief This static function integrates single
/// dimension data.
///
/// \param cmdParser& ComandLineParser
/// \return vector<double>
///
/////////////////////////////////////////////////
static vector<mu::value_type> integrateSingleDimensionData(CommandLineParser& cmdParser)
{
    bool bReturnFunctionPoints = cmdParser.hasParam("points");
    bool bCalcXvals = cmdParser.hasParam("xvals");

    vector<mu::value_type> vResult;

    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    // Extract the integration interval
    IntervalSet ivl = cmdParser.parseIntervals();

    // Get table name and the corresponding indices
    DataAccessParser accessParser = cmdParser.getExprAsDataObject();
    accessParser.evalIndices();
    Indices& _idx = accessParser.getIndices();
    std::string sDatatable = accessParser.getDataObject();

    // The indices are vectors
    //
    // If it is a single column or row, then we simply
    // summarize its contents, otherwise we calculate the
    // integral with the trapezoidal method
    if ((_idx.row.size() == 1 || _idx.col.size() == 1) && !bReturnFunctionPoints)
        vResult.push_back(_data.sum(sDatatable, _idx.row, _idx.col));
    else if (_idx.row.size() == 1 || _idx.col.size() == 1)
    {
        // cumulative sum
        vResult.push_back(_data.getElement(_idx.row[0], _idx.col[0], sDatatable));

        if (_idx.row.size() == 1)
        {
            for (size_t i = 1; i < _idx.col.size(); i++)
                vResult.push_back(vResult.back() + _data.getElement(_idx.row[0], _idx.col[i], sDatatable));
        }
        else
        {
            for (size_t i = 1; i < _idx.row.size(); i++)
                vResult.push_back(vResult.back() + _data.getElement(_idx.row[i], _idx.col[0], sDatatable));
        }
    }
    else
    {
        MemoryManager _cache;

        // Copy the data
        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            _cache.writeToTable(i, 0, "table", _data.getElement(_idx.row[i], _idx.col[0], sDatatable));
            _cache.writeToTable(i, 1, "table", _data.getElement(_idx.row[i], _idx.col[1], sDatatable));
        }

        // Sort the data
        _cache.sortElements("sort -table c=1[2]");
        double dResult = 0.0;
        long long int j = 1;

        // Calculate the integral by jumping over NaNs
        for (long long int i = 0; i < _cache.getLines("table", false) - 1; i++) //nan-suche
        {
            j = 1;

            if (!_cache.isValidElement(i, 1, "table"))
                continue;

            while (!_cache.isValidElement(i + j, 1, "table") && i + j < _cache.getLines("table", false) - 1)
                j++;

            if (!_cache.isValidElement(i + j, 0, "table") || !_cache.isValidElement(i + j, 1, "table"))
                break;

            if (ivl.intervals.size() >= 1 && ivl.intervals[0].front() > _cache.getElement(i, 0, "table"))
                continue;

            if (ivl.intervals.size() >= 1 && ivl.intervals[0].back() < _cache.getElement(i + j, 0, "table"))
                break;

            // Calculate either the integral, its samples or the corresponding x values
            if (!bReturnFunctionPoints && !bCalcXvals)
                dResult += (_cache.getElement(i, 1, "table") + _cache.getElement(i + j, 1, "table")) / 2.0 * (_cache.getElement(i + j, 0, "table") - _cache.getElement(i, 0, "table"));
            else if (bReturnFunctionPoints && !bCalcXvals)
            {
                if (vResult.size())
                    vResult.push_back((_cache.getElement(i, 1, "table") + _cache.getElement(i + j, 1, "table")) / 2.0 * (_cache.getElement(i + j, 0, "table") - _cache.getElement(i, 0, "table")) + vResult.back());
                else
                    vResult.push_back((_cache.getElement(i, 1, "table") + _cache.getElement(i + j, 1, "table")) / 2.0 * (_cache.getElement(i + j, 0, "table") - _cache.getElement(i, 0, "table")));
            }
            else
                vResult.push_back(_cache.getElement(i + j, 0, "table"));
        }

        // If the integral was calculated, then there is a
        // single result, which hasn't been stored yet
        if (!bReturnFunctionPoints && !bCalcXvals)
            vResult.push_back(dResult);
    }

    // Return the result of the integral
    return vResult;
}


/////////////////////////////////////////////////
/// \brief Calculate the integral of a function
/// or a data set in a single dimension.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool integrate(CommandLineParser& cmdParser)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    const Settings& _option = NumeReKernel::getInstance()->getSettings();
    string sIntegrationExpression = cmdParser.getExprAsMathExpression();
    mu::value_type* v = 0;
    int nResults = 0;
    vector<mu::value_type> vResult;   // Ausgabe-Wert
    vector<mu::value_type> vFunctionValues; // Werte an der Stelle n und n+1
    bool bLargeInterval = false;    // Boolean: TRUE, wenn ueber ein grosses Intervall integriert werden soll
    bool bReturnFunctionPoints = cmdParser.hasParam("points");
    bool bCalcXvals = cmdParser.hasParam("xvals");
    unsigned int nMethod = TRAPEZOIDAL;    // 1 = trapezoidal, 2 = simpson
    size_t nSamples = 1e3;

    mu::value_type& x = _defVars.vValue[0][0];
    mu::value_type range;

    // It's not possible to calculate the integral of a string expression
    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sIntegrationExpression))
        throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, cmdParser.getCommandLine(), SyntaxError::invalid_position, "integrate");

    // Ensure that the function is available
    if (!sIntegrationExpression.length())
        throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // If the integration function contains a data object,
    // the calculation is done way different from the usual
    // integration
    if (_data.isTable(sIntegrationExpression)
            && getMatchingParenthesis(sIntegrationExpression) != string::npos
            && sIntegrationExpression.find_first_not_of(' ', getMatchingParenthesis(sIntegrationExpression) + 1) == string::npos) // xvals
    {
        cmdParser.setReturnValue(integrateSingleDimensionData(cmdParser));
        return true;
    }

    // Evaluate the parameters
    IntervalSet ivl = cmdParser.parseIntervals();

    if (ivl.size())
    {
        if (ivl[0].min() == ivl[0].max())
            throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, cmdParser.getCommandLine(), SyntaxError::invalid_position);

        if (isinf(ivl[0].min()) || isnan(ivl[0].min())
            || isinf(ivl[0].max()) || isnan(ivl[0].max()))
        {
            cmdParser.setReturnValue("nan");
            return false;
        }

        range = ivl[0].max() - ivl[0].min();
    }
    else
        throw SyntaxError(SyntaxError::NO_INTEGRATION_RANGES, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    std::vector<double> vParVal = cmdParser.getParameterValueAsNumericalValue("precision");

    if (vParVal.size())
        nSamples = std::rint(range / vParVal.front());
    else
    {
        vParVal = cmdParser.getParameterValueAsNumericalValue("p");

        if (vParVal.size())
            nSamples = std::rint(range / vParVal.front());
        else
        {
            vParVal = cmdParser.getParameterValueAsNumericalValue("eps");

            if (vParVal.size())
                nSamples = std::rint(range / vParVal.front());
        }
    }

    vParVal = cmdParser.getParameterValueAsNumericalValue("steps");

    if (vParVal.size())
        nSamples = std::abs(intCast(vParVal.front()));
    else
    {
        vParVal = cmdParser.getParameterValueAsNumericalValue("s");

        if (vParVal.size())
            nSamples = std::abs(intCast(vParVal.front()));
    }

    std::string sParVal = cmdParser.getParameterValue("method");

    if (!sParVal.length())
        sParVal = cmdParser.getParameterValue("m");

    if (sParVal == "trapezoidal")
        nMethod = TRAPEZOIDAL;
    else if (sParVal == "simpson")
        nMethod = SIMPSON;

    // Check, whether the expression actual depends
    // upon the integration variable
    _parser.SetExpr(sIntegrationExpression);

    _parser.Eval(nResults);
    vResult.resize(nResults, 0.0);

    // Set the calculation variables to their starting values
    vFunctionValues.resize(nResults, 0.0);

    // Calculate the numerical integration
    // If the precision is invalid (e.g. due to a very
    // small interval, simply guess a reasonable interval
    // here
    if (!nSamples)
        nSamples = 100;

    // Calculate the x values, if desired
    if (bCalcXvals)
    {
        for (size_t i = 0; i < nSamples; i++)
        {
            vResult.push_back(ivl[0](i, nSamples));
        }

        cmdParser.setReturnValue(vResult);
        return true;
    }

    // Set the expression in the parser
    _parser.SetExpr(sIntegrationExpression);

    // Is it a large interval (then it will need more time)
    if ((nMethod == TRAPEZOIDAL && nSamples >= 9.9e6) || (nMethod == SIMPSON && nSamples >= 1e4))
        bLargeInterval = true;

    // Do not allow a very high number of integration steps
    if (nSamples > 1e10)
        throw SyntaxError(SyntaxError::INVALID_INTEGRATION_PRECISION, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Set the integration variable to the lower border
    x = ivl[0](0);

    // Calculate the first sample(s)
    v = _parser.Eval(nResults);
    vFunctionValues.assign(v, v+nResults);

    double dx = range / (nSamples-1);

    // Perform the actual numerical integration
    for (size_t i = 1; i < nSamples; i++)
    {
        // Calculate the samples first
        if (nMethod == TRAPEZOIDAL)
            integrationstep_trapezoidal(x, ivl[0](i, nSamples), dx, vResult, vFunctionValues, bReturnFunctionPoints);
        else if (nMethod == SIMPSON)
            integrationstep_simpson(x, ivl[0](2*i-1, 2*nSamples-1), ivl[0](2*i, 2*nSamples-1), dx, vResult, vFunctionValues, bReturnFunctionPoints);

        // Print a status value, if needed
        if (_option.systemPrints() && bLargeInterval)
        {
            if ((int)(i / (double)nSamples * 100) > (int)((i-1) / (double)nSamples * 100))
                NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + toString((int)(i / (double)nSamples * 100)) + " %");

            if (NumeReKernel::GetAsyncCancelState())
            {
                NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + _lang.get("COMMON_CANCEL") + ".\n");
                throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
            }
        }
    }

    // Display a success message
    if (_option.systemPrints() && bLargeInterval)
        NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... 100 %: " + _lang.get("COMMON_SUCCESS") + "!\n");

    cmdParser.setReturnValue(vResult);
    return true;
}


/////////////////////////////////////////////////
/// \brief This static function re-evaluates the
/// boundary expression and updates the internal
/// variables correspondingly.
///
/// \param ivl IntervalSet&
/// \param y0 double&
/// \param y1 double&
/// \param sIntegrationExpression const string&
/// \return void
///
/////////////////////////////////////////////////
static void refreshBoundaries(IntervalSet& ivl, const string& sIntegrationExpression)
{
    // Refresh the y boundaries, if necessary
    ivl[1].refresh();

    NumeReKernel::getInstance()->getParser().SetExpr(sIntegrationExpression);
}


/////////////////////////////////////////////////
/// \brief Calculate the integral of a function
/// in two dimensions.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool integrate2d(CommandLineParser& cmdParser)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    const Settings& _option = NumeReKernel::getInstance()->getSettings();
    string sIntegrationExpression = cmdParser.getExprAsMathExpression();                // string fuer die zu integrierende Funktion
    mu::value_type* v = 0;
    int nResults = 0;
    vector<mu::value_type> vResult[3];      // value_type-Array, wobei vResult[0] das eigentliche Ergebnis speichert
    // und vResult[1] fuer die Zwischenergebnisse des inneren Integrals ist
    vector<mu::value_type> fx_n[2][3];          // value_type-Array fuer die jeweiligen Stuetzstellen im inneren und aeusseren Integral
    bool bRenewBoundaries = false;      // bool, der speichert, ob die Integralgrenzen von x oder y abhaengen
    bool bLargeArray = false;       // bool, der TRUE fuer viele Datenpunkte ist;
    unsigned int nMethod = TRAPEZOIDAL;       // trapezoidal = 1, simpson = 2
    size_t nSamples = 1e3;

    mu::value_type& x = _defVars.vValue[0][0];
    mu::value_type& y = _defVars.vValue[1][0];

    mu::value_type range;

    // Strings may not be integrated
    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sIntegrationExpression))
        throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, cmdParser.getCommandLine(), SyntaxError::invalid_position, "integrate");

    // Ensure that the integration function is available
    if (!sIntegrationExpression.length())
        throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Evaluate the parameters
    IntervalSet ivl = cmdParser.parseIntervals();

    if (ivl.intervals.size() >= 2)
    {
        if (ivl[0].front() == ivl[0].back())
            throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, cmdParser.getCommandLine(), SyntaxError::invalid_position);

        if (isinf(ivl[0].front()) || isnan(ivl[0].front())
            || isinf(ivl[0].back()) || isnan(ivl[0].back())
            || isinf(ivl[1].front()) || isnan(ivl[1].front())
            || isinf(ivl[1].back()) || isnan(ivl[1].back()))
        {
            cmdParser.setReturnValue("nan");
            return false;
        }

        range = std::min(ivl[0].max() - ivl[0].min(), ivl[1].max() - ivl[1].min());
    }
    else
        throw SyntaxError(SyntaxError::NO_INTEGRATION_RANGES, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    std::vector<double> vParVal = cmdParser.getParameterValueAsNumericalValue("precision");

    if (vParVal.size())
        nSamples = std::rint(range / vParVal.front());
    else
    {
        vParVal = cmdParser.getParameterValueAsNumericalValue("p");

        if (vParVal.size())
            nSamples = std::rint(range / vParVal.front());
        else
        {
            vParVal = cmdParser.getParameterValueAsNumericalValue("eps");

            if (vParVal.size())
                nSamples = std::rint(range / vParVal.front());
        }
    }

    vParVal = cmdParser.getParameterValueAsNumericalValue("steps");

    if (vParVal.size())
        nSamples = std::abs(intCast(vParVal.front()));
    else
    {
        vParVal = cmdParser.getParameterValueAsNumericalValue("s");

        if (vParVal.size())
            nSamples = std::abs(intCast(vParVal.front()));
    }

    std::string sParVal = cmdParser.getParameterValue("method");

    if (!sParVal.length())
        sParVal = cmdParser.getParameterValue("m");

    if (sParVal == "trapezoidal")
        nMethod = TRAPEZOIDAL;
    else if (sParVal == "simpson")
        nMethod = SIMPSON;

    // Check, whether the expression depends upon one or both
    // integration variables
    _parser.SetExpr(sIntegrationExpression);

    // Prepare the memory for integration
    _parser.Eval(nResults);

    for (int i = 0; i < 3; i++)
    {
        vResult[i].resize(nResults, 0.0);
        fx_n[0][i].resize(nResults, 0.0);
        fx_n[1][i].resize(nResults, 0.0);
    }

    if (ivl[1].contains(_defVars.sName[0]))
        bRenewBoundaries = true;    // Ja? Setzen wir den bool entsprechend

    // Ensure that the precision is reasonble
    // If the precision is invalid, we guess a reasonable value here
    if (!nSamples)
    {
        // We use the smallest intervall and split it into
        // 100 parts
        nSamples = 100;
    }

    // Is it a very slow integration?
    if ((nMethod == TRAPEZOIDAL && nSamples*nSamples >= 1e8) || (nMethod == SIMPSON && nSamples*nSamples >= 1e6))
        bLargeArray = true;

    // Avoid calculation with too many steps
    if (nSamples*nSamples > 1e10)
        throw SyntaxError(SyntaxError::INVALID_INTEGRATION_PRECISION, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // --> Kleine Info an den Benutzer, dass der Code arbeitet <--
    if (_option.systemPrints() && bLargeArray)
        NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... 0 %");

    // --> Setzen wir "x" und "y" auf ihre Startwerte <--
    x = ivl[0](0); // x = x_0

    // y might depend on the starting value
    if (bRenewBoundaries)
        refreshBoundaries(ivl, sIntegrationExpression);

    y = ivl[1](0); // y = y_0

    double dx = (ivl[0].max() - ivl[0].min()) / (nSamples-1);
    double dy = (ivl[1].max() - ivl[1].min()) / (nSamples-1);

    // --> Werte mit den Startwerten die erste Stuetzstelle fuer die y-Integration aus <--
    v = _parser.Eval(nResults);
    fx_n[1][0].assign(v, v+nResults);

    /* --> Berechne das erste y-Integral fuer die erste Stuetzstelle fuer x
     *     Die Schleife laeuft so lange wie y < y_1 <--
     */
    for (size_t j = 1; j < nSamples; j++)
    {
        if (nMethod == TRAPEZOIDAL)
            integrationstep_trapezoidal(y, ivl[1](j, nSamples), dy, vResult[1], fx_n[1][0], false);
        else if (nMethod == SIMPSON)
            integrationstep_simpson(y, ivl[1](2*j-1, 2*nSamples-1), ivl[1](2*j, 2*nSamples-1), dy, vResult[1], fx_n[1][0], false);
    }

    fx_n[0][0] = vResult[1];

    /* --> Das eigentliche, numerische Integral. Es handelt sich um nichts weiter als viele
     *     while()-Schleifendurchlaeufe.
     *     Die aeussere Schleife laeuft so lange x < x_1 ist. <--
     */
    for (size_t i = 1; i < nSamples; i++)
    {
        if (nMethod == TRAPEZOIDAL)
        {
            x = ivl[0](i, nSamples);

            // Refresh the y boundaries, if necessary
            if (bRenewBoundaries)
            {
                refreshBoundaries(ivl, sIntegrationExpression);
                dy = (ivl[1].max() - ivl[1].min()) / (nSamples-1);
            }

            // --> Setzen wir "y" auf den Wert, der von der unteren y-Grenze vorgegeben wird <--
            y = ivl[1](0);
            // --> Werten wir sofort die erste y-Stuetzstelle aus <--
            v = _parser.Eval(nResults);
            fx_n[1][0].assign(v, v+nResults);
            vResult[1].assign(nResults, 0.0);

            for (size_t j = 1; j < nSamples; j++)
                integrationstep_trapezoidal(y, ivl[1](j, nSamples), dy, vResult[1], fx_n[1][0], false);

            // --> Weise das Ergebnis der y-Integration an die zweite Stuetzstelle der x-Integration zu <--
            for (int i = 0; i < nResults; i++)
            {
                if (isnan(vResult[1][i]))
                    vResult[1][i] = 0.0;

                vResult[0][i] += dx * (fx_n[0][0][i] + vResult[1][i]) * 0.5; // Berechne das Trapez zu x
                fx_n[0][0][i] = vResult[1][i]; // Weise den Wert der zweiten Stuetzstelle an die erste Stuetzstelle zu
            }
        }
        else if (nMethod == SIMPSON)
        {
            for (size_t n = 1; n <= 2; n++)
            {
                x = ivl[0](2*i+n-2, 2*nSamples-1);

                // Refresh the y boundaries, if necessary
                if (bRenewBoundaries)
                {
                    refreshBoundaries(ivl, sIntegrationExpression);
                    dy = (ivl[1].max() - ivl[1].min()) / (nSamples-1);
                }

                // Set y to the first position
                y = ivl[1](0);

                // Calculate the first position
                if (n == 1)
                {
                    v = _parser.Eval(nResults);
                    fx_n[1][0].assign(v, v+nResults);
                }

                vResult[n].assign(nResults, 0.0);

                for (size_t j = 1; j < nSamples; j++)
                    integrationstep_simpson(y, ivl[1](2*j-1, 2*nSamples-1), ivl[1](2*j, 2*nSamples-1), dy, vResult[n], fx_n[1][0], false);

                // --> Weise das Ergebnis der y-Integration an die zweite Stuetzstelle der x-Integration zu <--
                for (int i = 0; i < nResults; i++)
                {
                    if (isnan(vResult[n][i]))
                        vResult[n][i] = 0.0;
                }
            }

            for (int i = 0; i < nResults; i++)
            {
                vResult[0][i] += dx / 6.0 * (fx_n[0][0][i] + 4.0 * vResult[1][i] + vResult[2][i]); // Berechne das Trapez zu x
                fx_n[0][0][i] = vResult[2][i]; // Weise den Wert der zweiten Stuetzstelle an die erste Stuetzstelle zu
            }
        }

        // Show some progress
        if (_option.systemPrints())
        {
            if (bLargeArray)
            {
                if ((int)(i / (double)nSamples * 100) > (int)((i-1) / (double)nSamples * 100))
                    NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + toString((int)(i / (double)nSamples * 100)) + " %");
            }

            if (NumeReKernel::GetAsyncCancelState())//GetAsyncKeyState(VK_ESCAPE))
            {
                NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + _lang.get("COMMON_CANCEL") + "!\n");
                throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
            }
        }
    }

    // Show a success message
    if (_option.systemPrints() && bLargeArray)
        NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... 100 %: " + _lang.get("COMMON_SUCCESS") + "!\n");

    // --> Fertig! Zurueck zur aufrufenden Funkton! <--
    cmdParser.setReturnValue(vResult[0]);
    return true;
}


/////////////////////////////////////////////////
/// \brief Calculate the numerical differential
/// of the passed expression or data set.
///
/// \param cmdParser CommandLineParser
/// \return bool
///
/////////////////////////////////////////////////
bool differentiate(CommandLineParser& cmdParser)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    const Settings& _option = NumeReKernel::getInstance()->getSettings();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();
    string sExpr = cmdParser.getExprAsMathExpression();
    string sVar = "";
    string sPos = "";
    double dEps = 0.0;
    double dPos = 0.0;
    double* dVar = 0;
    mu::value_type* v = 0;
    int nResults = 0;
    int nSamples = 100;
    vector<double> vInterval;
    vector<double> vResult;

    // Strings cannot be differentiated
    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sExpr))
        throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, cmdParser.getCommandLine(), SyntaxError::invalid_position, "diff");

    // Numerical expressions and data sets are handled differently
    if (!_data.containsTablesOrClusters(sExpr) && cmdParser.getParameterList().length())
    {
        // Is the "eps" parameter available?
        std::vector<double> paramVal = cmdParser.getParameterValueAsNumericalValue("eps");

        if (paramVal.size())
            dEps = fabs(paramVal.front());

        paramVal = cmdParser.getParameterValueAsNumericalValue("samples");

        if (paramVal.size())
            nSamples = intCast(fabs(paramVal.front()));

        sVar = cmdParser.getParameterList();

        if (!_functions.call(sVar))
            throw SyntaxError(SyntaxError::FUNCTION_ERROR, cmdParser.getCommandLine(), sVar, sVar);

        // Is a variable interval defined?
        if (sVar.find('=') != string::npos ||
                (sVar.find('[') != string::npos
                 && sVar.find(']', sVar.find('[')) != string::npos
                 && sVar.find(':', sVar.find('[')) != string::npos))
        {
            // Remove possible parameter list initializers
            if (sVar.substr(0, 2) == "--")
                sVar = sVar.substr(2);
            else if (sVar.substr(0, 4) == "-set")
                sVar = sVar.substr(4);

            // Extract variable intervals or locations
            if (sVar.find('[') != string::npos
                    && sVar.find(']', sVar.find('[')) != string::npos
                    && sVar.find(':', sVar.find('[')) != string::npos)
            {
                sPos = sVar.substr(sVar.find('[') + 1, getMatchingParenthesis(sVar.substr(sVar.find('['))) - 1);
                sVar = "x";
                StripSpaces(sPos);

                if (sPos == ":")
                    sPos = "-10:10";
            }
            else
            {
                int nPos = sVar.find('=');
                sPos = sVar.substr(nPos + 1, sVar.find(' ', nPos) - nPos - 1);
                sVar = " " + sVar.substr(0, nPos);
                sVar = sVar.substr(sVar.rfind(' '));
                StripSpaces(sVar);
            }

            // Evaluate the position/range expression
            if (isNotEmptyExpression(sPos))
            {
                if (_data.containsTablesOrClusters(sPos))
                    getDataElements(sPos, _parser, _data, _option);

                if (sPos.find(':') != string::npos)
                    sPos.replace(sPos.find(':'), 1, ",");
                _parser.SetExpr(sPos);
                v = _parser.Eval(nResults);

                if (isinf(v[0]) || isnan(v[0]))
                {
                    cmdParser.setReturnValue("nan");
                    return true;
                }

                for (int i = 0; i < nResults; i++)
                    vInterval.push_back(v[i]);
            }

            // Set the expression for differentiation
            // and evaluate it
            _parser.SetExpr(sExpr);
            _parser.Eval(nResults);

            // Get the address of the variable
            dVar = getPointerToVariable(sVar, _parser);
        }

        // Ensure that the address could be found
        if (!dVar)
            throw SyntaxError(SyntaxError::NO_DIFF_VAR, cmdParser.getCommandLine(), SyntaxError::invalid_position);

        // Define a reasonable precision if no precision was set
        if (!dEps)
            dEps = 1e-7;

        // Store the expression
        string sCompl_Expr = sExpr;

        // As long as the expression has a length
        while (sCompl_Expr.length())
        {
            // Get the next subexpression and
            // set it in the parser
            sExpr = getNextArgument(sCompl_Expr, true);
            _parser.SetExpr(sExpr);

            // Evaluate the differential at the desired
            // locations
            if (vInterval.size() == 1 || vInterval.size() > 2)
            {
                // single point or a vector
                for (unsigned int i = 0; i < vInterval.size(); i++)
                {
                    dPos = vInterval[i];
                    vResult.push_back(_parser.Diff(dVar, dPos, dEps));
                }
            }
            else
            {
                // a range -> use the samples
                for (int i = 0; i < nSamples; i++)
                {
                    dPos = vInterval[0] + (vInterval[1] - vInterval[0]) / (double)(nSamples - 1) * (double)i;
                    vResult.push_back(_parser.Diff(dVar, dPos, dEps));
                }
            }
        }
    }
    else if (_data.containsTablesOrClusters(sExpr))
    {
        // This is a data set
        //
        // Get the indices first
        DataAccessParser accessParser = cmdParser.getExprAsDataObject();
        Indices& _idx = accessParser.getIndices();
        std::string sTableName = accessParser.getDataObject();

        // Validate the indices
        if (!isValidIndexSet(_idx))
            throw SyntaxError(SyntaxError::INVALID_INDEX, cmdParser.getCommandLine(), SyntaxError::invalid_position, _idx.row.to_string() + ", " + _idx.col.to_string());

        if (_idx.row.isOpenEnd())
            _idx.row.setRange(0, _data.getLines(sTableName, false)-1);

        if (_idx.col.isOpenEnd())
            _idx.col.setRange(0, _idx.col.front()+1);

        // Copy the data contents, sort the values
        // and calculate the derivative

        // Vectors as indices
        //
        // Depending on the number of selected columns, we either
        // have to sort the data or we assume that the difference
        // between two values is 1
        if (_idx.col.size() == 1)
        {
            // No sorting, difference is 1
            //
            // Jump over NaNs and get the difference of the neighbouring
            // values, which is identical to the derivative in this case
            for (long long int i = 0; i < _idx.row.size() - 1; i++)
            {
                if (_data.isValidElement(_idx.row[i], _idx.col.front(), sTableName)
                        && _data.isValidElement(_idx.row[i + 1], _idx.col.front(), sTableName))
                    vResult.push_back(_data.getElement(_idx.row[i + 1], _idx.col.front(), sTableName) - _data.getElement(_idx.row[i], _idx.col.front(), sTableName));
                else
                    vResult.push_back(NAN);
            }
        }
        else
        {
            // We have to sort, because the difference is passed
            // explicitly
            //
            // Copy the data first and sort afterwards
            MemoryManager _cache;

            for (size_t i = 0; i < _idx.row.size(); i++)
            {
                _cache.writeToTable(i, 0, "table", _data.getElement(_idx.row[i], _idx.col[0], sTableName));
                _cache.writeToTable(i, 1, "table", _data.getElement(_idx.row[i], _idx.col[1], sTableName));
            }

            _cache.sortElements("sort -table c=1[2]");

            // Shall the x values be calculated?
            if (cmdParser.hasParam("xvals"))
            {
                // The x values are approximated to be in the
                // middle of the two samplex
                for (long long int i = 0; i < _cache.getLines("table", false) - 1; i++)
                {
                    if (_cache.isValidElement(i, 0, "table")
                            && _cache.isValidElement(i + 1, 0, "table")
                            && _cache.isValidElement(i, 1, "table")
                            && _cache.isValidElement(i + 1, 1, "table"))
                        vResult.push_back((_cache.getElement(i + 1, 0, "table") + _cache.getElement(i, 0, "table")) / 2);
                    else
                        vResult.push_back(NAN);
                }
            }
            else
            {
                // We calculate the derivative of the data
                // by approximating it linearily
                for (long long int i = 0; i < _cache.getLines("table", false) - 1; i++)
                {
                    if (_cache.isValidElement(i, 0, "table")
                            && _cache.isValidElement(i + 1, 0, "table")
                            && _cache.isValidElement(i, 1, "table")
                            && _cache.isValidElement(i + 1, 1, "table"))
                        vResult.push_back((_cache.getElement(i + 1, 1, "table") - _cache.getElement(i, 1, "table"))
                                          / (_cache.getElement(i + 1, 0, "table") - _cache.getElement(i, 0, "table")));
                    else
                        vResult.push_back(NAN);
                }
            }
        }
    }
    else
    {
        // Ensure that a parameter list is available
        throw SyntaxError(SyntaxError::NO_DIFF_OPTIONS, cmdParser.getCommandLine(), SyntaxError::invalid_position);
    }

    cmdParser.setReturnValue(vResult);
    return true;
}


/////////////////////////////////////////////////
/// \brief This static function extracts the
/// interval definition for the extrema and root
/// search functions.
///
/// \param sParams const string&
/// \param sVar string&
/// \return string
///
/////////////////////////////////////////////////
static string getIntervalForSearchFunctions(const string& sParams, string& sVar)
{
    string sInterval = "";

    // Extract the interval definition from the
    // parameter string
    if (sParams.find('=') != string::npos)
    {
        int nPos = sParams.find('=');
        sInterval = getArgAtPos(sParams, nPos + 1);

        if (sInterval.front() == '[' && sInterval.back() == ']')
        {
            sInterval.pop_back();
            sInterval.erase(0, 1);
        }

        sVar = " " + sParams.substr(0, nPos);
        sVar = sVar.substr(sVar.rfind(' '));
        StripSpaces(sVar);
    }
    else
    {
        sVar = "x";
        sInterval = sParams.substr(sParams.find('[') + 1, getMatchingParenthesis(sParams.substr(sParams.find('['))) - 1);
        StripSpaces(sInterval);

        if (sInterval == ":")
            sInterval = "-10:10";
    }

    return sInterval;
}


/////////////////////////////////////////////////
/// \brief This static function finds extrema in
/// a multi-result expression, i.e. an expression
/// containing a table or similar.
///
/// \param cmdParser CommandLineParser&
/// \param sExpr string&
/// \param sInterval string&
/// \param nOrder int
/// \param nMode int
/// \return bool
///
/////////////////////////////////////////////////
static bool findExtremaInMultiResult(CommandLineParser& cmdParser, string& sExpr, string& sInterval, int nOrder, int nMode)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    _parser.SetExpr(sExpr);
    int nResults;
    mu::value_type* v = _parser.Eval(nResults);
    vector<double> vResults;
    int nResults_x = 0;
    MemoryManager _cache;

    // Store the results in the second column of a table
    for (int i = 0; i < nResults; i++)
        _cache.writeToTable(i, 1, "table", v[i]);

    _parser.SetExpr(sInterval);
    v = _parser.Eval(nResults_x);

    // Write the results for the x interval in the first column
    if (nResults_x > 1)
    {
        for (int i = 0; i < nResults; i++)
        {
            if (i >= nResults_x)
                _cache.writeToTable(i, 0, "table", 0.0);
            else
                _cache.writeToTable(i, 0, "table", v[i]);
        }
    }
    else
        return false;

    std::string sSortingExpr = "sort -table cols=1[2]";
    _cache.sortElements(sSortingExpr);

    double dMedian = 0.0, dExtremum = 0.0;
    double* data = new double[nOrder];
    int nDir = 0;
    int nanShift = 0;

    if (nOrder >= nResults / 3)
        nOrder = nResults / 3;

    // Ensure that the number of used points is reasonable
    if (nOrder < 3)
    {
        vResults.push_back(NAN);
        return false;
    }

    // Find the first median and use it as starting point
    // for identifying the next extremum
    for (int i = 0; i + nanShift < _cache.getLines("table", true); i++)
    {
        if (i == nOrder)
            break;

        while (isnan(_cache.getElement(i + nanShift, 1, "table")) && i + nanShift < _cache.getLines("table", true) - 1)
            nanShift++;

        data[i] = _cache.getElement(i + nanShift, 1, "table");
    }

    // Sort the data and find the median
    gsl_sort(data, 1, nOrder);
    dExtremum = gsl_stats_median_from_sorted_data(data, 1, nOrder);

    // Go through the data points using sliding median to find the local
    // extrema in the data set
    for (int i = nOrder; i + nanShift < _cache.getLines("table", false) - nOrder; i++)
    {
        int currNanShift = 0;
        dMedian = 0.0;

        for (int j = i; j < i + nOrder; j++)
        {
            while (isnan(_cache.getElement(j + nanShift + currNanShift, 1, "table")) && j + nanShift + currNanShift < _cache.getLines("table", true) - 1)
                currNanShift++;

            data[j - i] = _cache.getElement(j + nanShift + currNanShift, 1, "table");
        }

        gsl_sort(data, 1, nOrder);
        dMedian = gsl_stats_median_from_sorted_data(data, 1, nOrder);

        if (!nDir)
        {
            if (dMedian > dExtremum)
                nDir = 1;
            else if (dMedian < dExtremum)
                nDir = -1;

            dExtremum = dMedian;
        }
        else
        {
            if (nDir*dMedian < nDir*dExtremum)
            {
                if (!nMode || nMode == nDir)
                {
                    int nExtremum = i + nanShift;
                    double dExtremum = _cache.getElement(i + nanShift, 1, "table");

                    for (long long int k = i + nanShift; k >= 0; k--)
                    {
                        if (k == i - nOrder)
                            break;

                        if (nDir*_cache.getElement(k, 1, "table") > nDir*dExtremum)
                        {
                            nExtremum = k;
                            dExtremum = _cache.getElement(k, 1, "table");
                        }
                    }

                    vResults.push_back(_cache.getElement(nExtremum, 0, "table"));
                    i = nExtremum + nOrder;
                }

                nDir = 0;
            }

            dExtremum = dMedian;
        }

        nanShift += currNanShift;
    }

    if (!vResults.size())
        vResults.push_back(NAN);

    delete[] data;
    cmdParser.setReturnValue(vResults);
    return true;
}


/////////////////////////////////////////////////
/// \brief Static helper function for
/// findExtremaInData
///
/// \param v value_type*
/// \param nResults int
/// \param start int
/// \param nOrder int
/// \param nanShiftStart int
/// \param nNewNanShift int&
/// \return double
///
/////////////////////////////////////////////////
static double calculateMedian(value_type* v, int nResults, int start, int nOrder, int nanShiftStart, int& nNewNanShift)
{
    vector<double> data;

    for (int i = start; i < start + nOrder; i++)
    {
        while (isnan(v[i + nanShiftStart + nNewNanShift]) && i + nanShiftStart + nNewNanShift < nResults - 1)
            nNewNanShift++;

        if (i + nanShiftStart + nNewNanShift >= nResults)
            break;

        data.push_back(v[i + nanShiftStart + nNewNanShift]);
    }

    gsl_sort(&data[0], 1, data.size());
    return gsl_stats_median_from_sorted_data(&data[0], 1, data.size());
}


/////////////////////////////////////////////////
/// \brief This static function finds extrema in
/// the selected data sets.
///
/// \param cmdParser CommandLineParser&
/// \param sExpr string&
/// \param nOrder int
/// \param nMode int
/// \return bool
///
/////////////////////////////////////////////////
static bool findExtremaInData(CommandLineParser& cmdParser, string& sExpr, int nOrder, int nMode)
{
    mu::value_type* v;
    int nResults = 0;
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    _parser.SetExpr(sExpr);
    v = _parser.Eval(nResults);

    if (nResults > 1)
    {
        if (nOrder >= nResults / 3)
            nOrder = nResults / 3;

        double dMedian = 0.0, dExtremum = 0.0;
        int nDir = 0;
        int nanShift = 0;
        vector<double> vResults;

        if (nOrder < 3)
        {
            vResults.push_back(NAN);
            return false;
        }

        dExtremum = calculateMedian(v, nResults, 0, nOrder, 0, nanShift);

        for (int i = nOrder; i + nanShift < nResults - nOrder; i++)
        {
            int currNanShift = 0;
            dMedian = calculateMedian(v, nResults, i, nOrder, nanShift, currNanShift);

            if (!nDir)
            {
                if (dMedian > dExtremum)
                    nDir = 1;
                else if (dMedian < dExtremum)
                    nDir = -1;

                dExtremum = dMedian;
            }
            else
            {
                if (nDir*dMedian < nDir*dExtremum)
                {
                    if (!nMode || nMode == nDir)
                    {
                        int nExtremum = i + nanShift;
                        double dLocalExtremum = v[i + nanShift];

                        for (long long int k = i + nanShift; k >= 0; k--)
                        {
                            if (k == i + nanShift - nOrder)
                                break;

                            if (nDir*v[k] > nDir*dLocalExtremum)
                            {
                                nExtremum = k;
                                dLocalExtremum = v[k];
                            }
                        }

                        vResults.push_back(nExtremum + 1);
                        i = nExtremum + nOrder;
                        nanShift = 0;
                    }

                    nDir = 0;
                }

                dExtremum = dMedian;
            }

            nanShift += currNanShift;
        }

        if (!vResults.size())
            vResults.push_back(NAN);

        cmdParser.setReturnValue(vResults);
        return true;
    }
    else
        throw SyntaxError(SyntaxError::NO_EXTREMA_VAR, cmdParser.getCommandLine(), SyntaxError::invalid_position);
}


/////////////////////////////////////////////////
/// \brief This function is a wrapper to the
/// actual extrema localisation function
/// localizeExtremum() further below.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool findExtrema(CommandLineParser& cmdParser)
{
    NumeReKernel* instance = NumeReKernel::getInstance();
    MemoryManager& _data = instance->getMemoryManager();
    Parser& _parser = instance->getParser();

    unsigned int nSamples = 21;
    int nOrder = 5;
    mu::value_type dVal[2];
    mu::value_type dBoundaries[2] = {0.0, 0.0};
    int nMode = 0;
    mu::value_type* dVar = 0;
    string sExpr = "";
    string sParams = "";
    string sInterval = "";
    string sVar = "";

    // We cannot search extrema in strings
    if (instance->getStringParser().isStringExpression(cmdParser.getExpr()))
        throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, cmdParser.getCommandLine(), SyntaxError::invalid_position, "extrema");

    if (!_data.containsTablesOrClusters(cmdParser.getExpr()) && !cmdParser.getParameterList().length())
        throw SyntaxError(SyntaxError::NO_EXTREMA_OPTIONS, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Isolate the expression
    StripSpaces(sExpr);
    sExpr = sExpr.substr(findCommand(sExpr).sString.length());

    // Ensure that the expression is not empty
    // and that the custom functions don't throw
    // any errors
    sExpr = cmdParser.getExpr();
    sParams = cmdParser.getParameterList();

    if (!isNotEmptyExpression(sExpr) || !instance->getDefinitions().call(sExpr))
        return false;

    if (!instance->getDefinitions().call(sParams))
        return false;

    StripSpaces(sParams);

    // If the expression or the parameter list contains
    // data elements, get their values here
    if (_data.containsTablesOrClusters(sExpr))
        getDataElements(sExpr, _parser, _data, instance->getSettings(), false);

    if (_data.containsTablesOrClusters(sParams))
        getDataElements(sParams, _parser, _data, instance->getSettings(), false);

    // Evaluate the parameters
    if (findParameter(sParams, "min"))
        nMode = -1;

    if (findParameter(sParams, "max"))
        nMode = 1;

    if (findParameter(sParams, "samples", '='))
    {
        _parser.SetExpr(getArgAtPos(sParams, findParameter(sParams, "samples", '=') + 7));
        nSamples = intCast(_parser.Eval());

        if (nSamples < 21)
            nSamples = 21;

        sParams.erase(findParameter(sParams, "samples", '=') - 1, 8);
    }

    if (findParameter(sParams, "points", '='))
    {
        _parser.SetExpr(getArgAtPos(sParams, findParameter(sParams, "points", '=') + 6));
        nOrder = intCast(_parser.Eval());

        if (nOrder <= 3)
            nOrder = 3;

        sParams.erase(findParameter(sParams, "points", '=') - 1, 7);
    }

    // Extract the interval
    if (sParams.find('=') != string::npos
            || (sParams.find('[') != string::npos
                && sParams.find(']', sParams.find('['))
                && sParams.find(':', sParams.find('['))))
    {
        if (sParams.substr(0, 2) == "--")
            sParams = sParams.substr(2);
        else if (sParams.substr(0, 4) == "-set")
            sParams = sParams.substr(4);

        int nResults = 0;

        sInterval = getIntervalForSearchFunctions(sParams, sVar);

        _parser.SetExpr(sExpr);
        _parser.Eval(nResults);

        if (nResults > 1)
            return findExtremaInMultiResult(cmdParser, sExpr, sInterval, nOrder, nMode);
        else
        {
            if (!isVariableInAssignedExpression(_parser, sVar))
            {
                cmdParser.setReturnValue("nan");
                return true;
            }

            dVar = getPointerToVariable(sVar, _parser);

            if (!dVar)
                throw SyntaxError(SyntaxError::EXTREMA_VAR_NOT_FOUND, cmdParser.getCommandLine(), sVar, sVar);

            if (sInterval.find(':') == string::npos || sInterval.length() < 3)
                return false;

            auto indices = getAllIndices(sInterval);

            for (size_t i = 0; i < 2; i++)
            {
                if (isNotEmptyExpression(indices[i]))
                {
                    _parser.SetExpr(indices[i]);
                    dBoundaries[i] = _parser.Eval();

                    if (isinf(dBoundaries[i]) || isnan(dBoundaries[i]))
                    {
                        cmdParser.setReturnValue("nan");
                        return false;
                    }
                }
                else
                    return false;
            }

            if (dBoundaries[1] < dBoundaries[0])
            {
                double Temp = dBoundaries[1];
                dBoundaries[1] = dBoundaries[0];
                dBoundaries[0] = Temp;
            }
        }
    }
    else if (cmdParser.exprContainsDataObjects())
        return findExtremaInData(cmdParser, sExpr, nOrder, nMode);
    else
        throw SyntaxError(SyntaxError::NO_EXTREMA_VAR, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Calculate the number of samples depending on
    // the interval width
    if ((int)(dBoundaries[1] - dBoundaries[0]))
        nSamples = (nSamples - 1) * (int)(dBoundaries[1] - dBoundaries[0]) + 1;

    // Ensure that we calculate a reasonable number of samples
    if (nSamples > 10001)
        nSamples = 10001;

    // Set the expression and evaluate it once
    _parser.SetExpr(sExpr);
    _parser.Eval();
    vector<double> vResults;
    dVal[0] = _parser.Diff(dVar, dBoundaries[0], 1e-7);

    // Evaluate the extrema for all samples. We search for
    // a sign change in the derivative and examine these intervals
    // in more detail
    for (unsigned int i = 1; i < nSamples; i++)
    {
        // Evaluate the derivative at the current sample position
        dVal[1] = _parser.Diff(dVar, dBoundaries[0] + i * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1), 1e-7);

        // Is it a sign change or a actual zero?
        if (dVal[0]*dVal[1] < 0)
        {
            if (!nMode
                    || (nMode == 1 && (dVal[0] > 0 && dVal[1] < 0))
                    || (nMode == -1 && (dVal[0] < 0 && dVal[1] > 0)))
            {
                // Examine the current interval in more detail
                vResults.push_back(localizeExtremum(sExpr, dVar, _parser, instance->getSettings(), dBoundaries[0] + (i - 1) * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1), dBoundaries[0] + i * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1)));
            }
        }
        else if (dVal[0]*dVal[1] == 0.0)
        {
            if (!nMode
                    || (nMode == 1 && (dVal[0] > 0 || dVal[1] < 0))
                    || (nMode == -1 && (dVal[0] < 0 || dVal[1] > 0)))
            {
                int nTemp = i - 1;

                // Jump over multiple zeros due to constness
                if (dVal[0] != 0.0)
                {
                    while (dVal[0]*dVal[1] == 0.0 && i + 1 < nSamples)
                    {
                        i++;
                        dVal[1] = _parser.Diff(dVar, dBoundaries[0] + i * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1), 1e-7);
                    }
                }
                else
                {
                    while (dVal[1] == 0.0 && i + 1 < nSamples)
                    {
                        i++;
                        dVal[1] = _parser.Diff(dVar, dBoundaries[0] + i * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1), 1e-7);
                    }
                }

                // Store the current location
                vResults.push_back(localizeExtremum(sExpr, dVar, _parser, instance->getSettings(), dBoundaries[0] + nTemp * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1), dBoundaries[0] + i * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1)));
            }
        }
        dVal[0] = dVal[1];
    }

    // If we didn't find any results
    // examine the boundaries for possible extremas
    if (!vResults.size())
    {
        dVal[0] = _parser.Diff(dVar, dBoundaries[0]);
        dVal[1] = _parser.Diff(dVar, dBoundaries[1]);
        std::string sRetVal;

        // Examine the left boundary
        if (dVal[0]
                && (!nMode
                    || (dVal[0] < 0 && nMode == 1)
                    || (dVal[0] > 0 && nMode == -1)))
            sRetVal = toString(dBoundaries[0], instance->getSettings());

        // Examine the right boundary
        if (dVal[1]
                && (!nMode
                    || (dVal[1] < 0 && nMode == -1)
                    || (dVal[1] > 0 && nMode == 1)))
        {
            if (sRetVal.length())
                sRetVal += ", ";

            sRetVal += toString(dBoundaries[1], instance->getSettings());
        }

        // Still nothing found?
        if (!dVal[0] && ! dVal[1])
            sRetVal = "nan";

        cmdParser.setReturnValue(sRetVal);
    }
    else
    {
        cmdParser.setReturnValue(vResults);
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This static function finds zeroes in
/// a multi-result expression, i.e. an expression
/// containing a table or similar.
///
/// \param cmdParser CommandLineParser&
/// \param sExpr string&
/// \param sInterval string&
/// \param nMode int
/// \return bool
///
/////////////////////////////////////////////////
static bool findZeroesInMultiResult(CommandLineParser& cmdParser, string& sExpr, string& sInterval, int nMode)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    _parser.SetExpr(sExpr);
    int nResults;
    mu::value_type* v = _parser.Eval(nResults);
    MemoryManager _cache;

    vector<double> vResults;
    int nResults_x = 0;

    for (int i = 0; i < nResults; i++)
        _cache.writeToTable(i, 1, "table", v[i]);

    _parser.SetExpr(sInterval);
    v = _parser.Eval(nResults_x);

    if (nResults_x > 1)
    {
        for (int i = 0; i < nResults; i++)
        {
            if (i >= nResults_x)
                _cache.writeToTable(i, 0, "table", 0.0);
            else
                _cache.writeToTable(i, 0, "table", v[i]);
        }
    }
    else
        return false;

    std::string sSortingExpr = "sort -table cols=1[2]";
    _cache.sortElements(sSortingExpr);

    for (long long int i = 1; i < _cache.getLines("table", false); i++)
    {
        if (isnan(_cache.getElement(i - 1, 1, "table")))
            continue;

        if (!nMode && _cache.getElement(i, 1, "table")*_cache.getElement(i - 1, 1, "table") <= 0.0)
        {
            if (_cache.getElement(i, 1, "table") == 0.0)
            {
                vResults.push_back(_cache.getElement(i, 0, "table"));
                i++;
            }
            else if (_cache.getElement(i - 1, 1, "table") == 0.0)
                vResults.push_back(_cache.getElement(i - 1, 0, "table"));
            else if (_cache.getElement(i, 1, "table")*_cache.getElement(i - 1, 1, "table") < 0.0)
                vResults.push_back(Linearize(_cache.getElement(i - 1, 0, "table"), _cache.getElement(i - 1, 1, "table"), _cache.getElement(i, 0, "table"), _cache.getElement(i, 1, "table")));
        }
        else if (nMode && _cache.getElement(i, 1, "table")*_cache.getElement(i - 1, 1, "table") <= 0.0)
        {
            if (_cache.getElement(i, 1, "table") == 0.0 && _cache.getElement(i - 1, 1, "table") == 0.0)
            {
                for (long long int j = i + 1; j < _cache.getLines("table", false); j++)
                {
                    if (nMode * _cache.getElement(j, 1, "table") > 0.0)
                    {
                        for (long long int k = i - 1; k <= j; k++)
                            vResults.push_back(_cache.getElement(k, 0, "table"));

                        break;
                    }
                    else if (nMode * _cache.getElement(j, 1, "table") < 0.0)
                        break;

                    if (j + 1 == _cache.getLines("table", false) && i > 1 && nMode * _cache.getElement(i - 2, 1, "table") < 0.0)
                    {
                        for (long long int k = i - 1; k <= j; k++)
                            vResults.push_back(_cache.getElement(k, 0, "table"));

                        break;
                    }
                }

                continue;
            }
            else if (_cache.getElement(i, 1, "table") == 0.0 && nMode * _cache.getElement(i - 1, 1, "table") < 0.0)
                vResults.push_back(_cache.getElement(i, 0, "table"));
            else if (_cache.getElement(i - 1, 1, "table") == 0.0 && nMode * _cache.getElement(i, 1, "table") > 0.0)
                vResults.push_back(_cache.getElement(i - 1, 0, "table"));
            else if (_cache.getElement(i, 1, "table")*_cache.getElement(i - 1, 1, "table") < 0.0 && nMode * _cache.getElement(i - 1, 1, "table") < 0.0)
                vResults.push_back(Linearize(_cache.getElement(i - 1, 0, "table"), _cache.getElement(i - 1, 1, "table"), _cache.getElement(i, 0, "table"), _cache.getElement(i, 1, "table")));
        }
    }

    if (!vResults.size())
        vResults.push_back(NAN);

    cmdParser.setReturnValue(vResults);
    return true;
}


/////////////////////////////////////////////////
/// \brief This static function finds zeroes in
/// the selected data set.
///
/// \param cmdParser CommandLineParser&
/// \param sExpr string&
/// \param nMode int
/// \return bool
///
/////////////////////////////////////////////////
static bool findZeroesInData(CommandLineParser& cmdParser, string& sExpr, int nMode)
{
    mu::value_type* v;
    int nResults = 0;
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    _parser.SetExpr(sExpr);
    v = _parser.Eval(nResults);

    if (nResults > 1)
    {
        vector<double> vResults;

        for (int i = 1; i < nResults; i++)
        {
            if (isnan(v[i - 1]))
                continue;

            if (!nMode && v[i]*v[i - 1] <= 0.0)
            {
                if (v[i] == 0.0)
                {
                    vResults.push_back((double)i + 1);
                    i++;
                }
                else if (v[i - 1] == 0.0)
                    vResults.push_back((double)i);
                else if (fabs(v[i]) <= fabs(v[i - 1]))
                    vResults.push_back((double)i + 1);
                else
                    vResults.push_back((double)i);
            }
            else if (nMode && v[i]*v[i - 1] <= 0.0)
            {
                if (v[i] == 0.0 && v[i - 1] == 0.0)
                {
                    for (int j = i + 1; j < nResults; j++)
                    {
                        if (nMode * v[j] > 0.0)
                        {
                            for (int k = i - 1; k <= j; k++)
                                vResults.push_back(k);

                            break;
                        }
                        else if (nMode * v[j] < 0.0)
                            break;

                        if (j + 1 == nResults && i > 2 && nMode * v[i - 2] < 0.0)
                        {
                            for (int k = i - 1; k <= j; k++)
                                vResults.push_back(k);

                            break;
                        }
                    }

                    continue;
                }
                else if (v[i] == 0.0 && nMode * v[i - 1] < 0.0)
                    vResults.push_back((double)i + 1);
                else if (v[i - 1] == 0.0 && nMode * v[i] > 0.0)
                    vResults.push_back((double)i);
                else if (fabs(v[i]) <= fabs(v[i - 1]) && nMode * v[i - 1] < 0.0)
                    vResults.push_back((double)i + 1);
                else if (nMode * v[i - 1] < 0.0)
                    vResults.push_back((double)i);
            }
        }

        if (!vResults.size())
            vResults.push_back(NAN);

        cmdParser.setReturnValue(vResults);
        return true;
    }
    else
        throw SyntaxError(SyntaxError::NO_ZEROES_VAR, cmdParser.getCommandLine(), SyntaxError::invalid_position);
}


/////////////////////////////////////////////////
/// \brief This function is a wrapper to the
/// actual zeros localisation function
/// localizeZero() further below.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool findZeroes(CommandLineParser& cmdParser)
{
    NumeReKernel* instance = NumeReKernel::getInstance();
    MemoryManager& _data = instance->getMemoryManager();
    Parser& _parser = instance->getParser();

    unsigned int nSamples = 21;
    mu::value_type dVal[2];
    mu::value_type dBoundaries[2] = {0.0, 0.0};
    int nMode = 0;
    mu::value_type* dVar = 0;
    mu::value_type dTemp = 0.0;
    string sExpr = "";
    string sParams = "";
    string sInterval = "";
    string sVar = "";

    // We cannot search zeroes in strings
    if (instance->getStringParser().isStringExpression(cmdParser.getExpr()))
        throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, cmdParser.getCommandLine(), SyntaxError::invalid_position, "zeroes");

    if (!_data.containsTablesOrClusters(cmdParser.getExpr()) && !cmdParser.getParameterList().length())
        throw SyntaxError(SyntaxError::NO_ZEROES_OPTIONS, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Ensure that the expression is not empty
    // and that the custom functions don't throw
    // any errors
    sExpr = cmdParser.getExpr();
    sParams = cmdParser.getParameterList();

    if (!isNotEmptyExpression(sExpr) || !instance->getDefinitions().call(sExpr))
        return false;

    if (!instance->getDefinitions().call(sParams))
        return false;

    StripSpaces(sParams);

    // If the expression or the parameter list contains
    // data elements, get their values here
    if (_data.containsTablesOrClusters(sExpr))
        getDataElements(sExpr, _parser, _data, NumeReKernel::getInstance()->getSettings(), false);

    if (_data.containsTablesOrClusters(sParams))
        getDataElements(sParams, _parser, _data, NumeReKernel::getInstance()->getSettings(), false);

    // Evaluate the parameter list
    if (findParameter(sParams, "min") || findParameter(sParams, "down"))
        nMode = -1;
    if (findParameter(sParams, "max") || findParameter(sParams, "up"))
        nMode = 1;

    if (findParameter(sParams, "samples", '='))
    {
        _parser.SetExpr(getArgAtPos(sParams, findParameter(sParams, "samples", '=') + 7));
        nSamples = intCast(_parser.Eval());

        if (nSamples < 21)
            nSamples = 21;

        sParams.erase(findParameter(sParams, "samples", '=') - 1, 8);
    }

    // Evaluate the interval
    if (sParams.find('=') != string::npos
            || (sParams.find('[') != string::npos
                && sParams.find(']', sParams.find('['))
                && sParams.find(':', sParams.find('['))))
    {
        if (sParams.substr(0, 2) == "--")
            sParams = sParams.substr(2);
        else if (sParams.substr(0, 4) == "-set")
            sParams = sParams.substr(4);

        int nResults = 0;

        sInterval = getIntervalForSearchFunctions(sParams, sVar);

        _parser.SetExpr(sExpr);
        _parser.Eval(nResults);

        if (nResults > 1)
            return findZeroesInMultiResult(cmdParser, sExpr, sInterval, nMode);
        else
        {
            if (!isVariableInAssignedExpression(_parser, sVar))
            {
                cmdParser.setReturnValue("nan");
                return true;
            }

            dVar = getPointerToVariable(sVar, _parser);

            if (!dVar)
                throw SyntaxError(SyntaxError::ZEROES_VAR_NOT_FOUND, cmdParser.getCommandLine(), sVar, sVar);

            if (sInterval.find(':') == string::npos || sInterval.length() < 3)
                return false;

            auto indices = getAllIndices(sInterval);

            for (size_t i = 0; i < 2; i++)
            {
                if (isNotEmptyExpression(indices[i]))
                {
                    _parser.SetExpr(indices[i]);
                    dBoundaries[i] = _parser.Eval();

                    if (isinf(dBoundaries[i]) || isnan(dBoundaries[i]))
                    {
                        cmdParser.setReturnValue("nan");
                        return false;
                    }
                }
                else
                    return false;
            }

            if (dBoundaries[1] < dBoundaries[0])
            {
                double Temp = dBoundaries[1];
                dBoundaries[1] = dBoundaries[0];
                dBoundaries[0] = Temp;
            }
        }
    }
    else if (cmdParser.exprContainsDataObjects())
        return findZeroesInData(cmdParser, sExpr, nMode);
    else
        throw SyntaxError(SyntaxError::NO_ZEROES_VAR, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Calculate the interval
    if ((int)(dBoundaries[1] - dBoundaries[0]))
        nSamples = (nSamples - 1) * (int)(dBoundaries[1] - dBoundaries[0]) + 1;

    // Ensure that we calculate a reasonable
    // amount of samples
    if (nSamples > 10001)
        nSamples = 10001;

    // Set the expression and evaluate it once
    _parser.SetExpr(sExpr);
    _parser.Eval();

    dTemp = *dVar;

    *dVar = dBoundaries[0];
    vector<mu::value_type> vResults;
    dVal[0] = _parser.Eval();

    // Find near zeros to the left of the boundary
    // which are probably not located due toe rounding
    // errors
    if (dVal[0] != 0.0 && fabs(dVal[0]) < 1e-10)
    {
        *dVar = dBoundaries[0] - 1e-10;
        dVal[1] = _parser.Eval();

        if (dVal[0]*dVal[1] < 0 && (nMode * dVal[0] <= 0.0))
            vResults.push_back(localizeExtremum(sExpr, dVar, _parser, instance->getSettings(), dBoundaries[0] - 1e-10, dBoundaries[0]));
    }

    // Evaluate all samples. We try to find
    // sign changes and evaluate the intervals, which
    // contain the sign changes, further
    for (unsigned int i = 1; i < nSamples; i++)
    {
        // Evalute the current sample
        *dVar = dBoundaries[0] + i * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1);
        dVal[1] = _parser.Eval();

        if (dVal[0]*dVal[1] < 0)
        {
            if (!nMode
                    || (nMode == -1 && (dVal[0] > 0 && dVal[1] < 0))
                    || (nMode == 1 && (dVal[0] < 0 && dVal[1] > 0)))
            {
                // Examine the current interval
                vResults.push_back((localizeZero(sExpr, dVar, _parser, instance->getSettings(), dBoundaries[0] + (i - 1) * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1), dBoundaries[0] + i * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1))));
            }
        }
        else if (dVal[0]*dVal[1] == 0.0)
        {
            if (!nMode
                    || (nMode == -1 && (dVal[0] > 0 || dVal[1] < 0))
                    || (nMode == 1 && (dVal[0] < 0 || dVal[1] > 0)))
            {
                int nTemp = i - 1;

                // Ignore consecutive zeros due to
                // constness
                if (dVal[0] != 0.0)
                {
                    while (dVal[0]*dVal[1] == 0.0 && i + 1 < nSamples)
                    {
                        i++;
                        *dVar = dBoundaries[0] + i * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1);
                        dVal[1] = _parser.Eval();
                    }
                }
                else
                {
                    while (dVal[1] == 0.0 && i + 1 < nSamples)
                    {
                        i++;
                        *dVar = dBoundaries[0] + i * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1);
                        dVal[1] = _parser.Eval();
                    }
                }

                // Store the result
                vResults.push_back(localizeZero(sExpr, dVar, _parser, instance->getSettings(), dBoundaries[0] + nTemp * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1), dBoundaries[0] + i * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1)));
            }
        }

        dVal[0] = dVal[1];
    }

    // Examine the right boundary, because there might be
    // a zero slightly right from the interval
    if (dVal[0] != 0.0 && fabs(dVal[0]) < 1e-10)
    {
        *dVar = dBoundaries[1] + 1e-10;
        dVal[1] = _parser.Eval();

        if (dVal[0]*dVal[1] < 0 && nMode * dVal[0] <= 0.0)
            vResults.push_back(localizeZero(sExpr, dVar, _parser, instance->getSettings(), dBoundaries[1], dBoundaries[1] + 1e-10));
    }

    *dVar = dTemp;

    if (!vResults.size())
        cmdParser.setReturnValue("nan");
    else
        cmdParser.setReturnValue(vResults);

    return true;
}


/////////////////////////////////////////////////
/// \brief This function searches for the
/// positions of all extrema, which are located
/// in the selected interval.
///
/// \param sCmd string&
/// \param dVarAdress mu::value_type*
/// \param _parser Parser&
/// \param _option const Settings&
/// \param dLeft double
/// \param dRight double
/// \param dEps double
/// \param nRecursion int
/// \return mu::value_type
///
/// The expression has to be setted in advance.
/// The function performs recursions until the
/// defined precision is reached.
/////////////////////////////////////////////////
static mu::value_type localizeExtremum(string& sCmd, mu::value_type* dVarAdress, Parser& _parser, const Settings& _option, double dLeft, double dRight, double dEps, int nRecursion)
{
    const unsigned int nSamples = 101;
    mu::value_type dVal[2];

    if (_parser.GetExpr() != sCmd)
    {
        _parser.SetExpr(sCmd);
        _parser.Eval();
    }

    // Calculate the leftmost value
    dVal[0] = _parser.Diff(dVarAdress, dLeft, 1e-7);

    // Separate the current interval in
    // nSamples steps and examine each step
    for (unsigned int i = 1; i < nSamples; i++)
    {
        // Calculate the next value
        dVal[1] = _parser.Diff(dVarAdress, dLeft + i * (dRight - dLeft) / (double)(nSamples - 1), 1e-7);

        // Multiply the values to find a sign change
        if (dVal[0]*dVal[1] < 0)
        {
            // Sign change
            // return, if precision is reached. Otherwise perform
            // a new recursion between the two values
            if ((dRight - dLeft) / (double)(nSamples - 1) <= dEps || fabs(log(dEps)) + 1 < nRecursion * 2)
                return dLeft + (i - 1) * (dRight - dLeft) / (double)(nSamples - 1) + Linearize(0.0, dVal[0], (dRight - dLeft) / (double)(nSamples - 1), dVal[1]);
            else
                return localizeExtremum(sCmd, dVarAdress, _parser, _option, dLeft + (i - 1) * (dRight - dLeft) / (double)(nSamples - 1), dLeft + i * (dRight - dLeft) / (double)(nSamples - 1), dEps, nRecursion + 1);
        }
        else if (dVal[0]*dVal[1] == 0.0)
        {
            // One of the two vwlues is zero.
            // Jump over all following zeros due
            // to constness
            int nTemp = i - 1;

            if (dVal[0] != 0.0)
            {
                while (dVal[0]*dVal[1] == 0.0 && i + 1 < nSamples)
                {
                    i++;
                    dVal[1] = _parser.Diff(dVarAdress, dLeft + i * (dRight - dLeft) / (double)(nSamples - 1), 1e-7);
                }
            }
            else
            {
                while (dVal[1] == 0.0 && i + 1 < nSamples)
                {
                    i++;
                    dVal[1] = _parser.Diff(dVarAdress, dLeft + i * (dRight - dLeft) / (double)(nSamples - 1), 1e-7);
                }
            }

            // return, if precision is reached. Otherwise perform
            // a new recursion between the two values
            if ((i - nTemp) * (dRight - dLeft) / (double)(nSamples - 1) <= dEps || (!nTemp && i + 1 == nSamples) || fabs(log(dEps)) + 1 < nRecursion * 2)
                return dLeft + nTemp * (dRight - dLeft) / (double)(nSamples - 1) + Linearize(0.0, dVal[0], (i - nTemp) * (dRight - dLeft) / (double)(nSamples - 1), dVal[1]);
            else
                return localizeExtremum(sCmd, dVarAdress, _parser, _option, dLeft + nTemp * (dRight - dLeft) / (double)(nSamples - 1), dLeft + i * (dRight - dLeft) / (double)(nSamples - 1), dEps, nRecursion + 1);
        }

        dVal[0] = dVal[1];
    }

    // If no explict sign change was found,
    // interpolate the position by linearisation
    *dVarAdress = dLeft;
    dVal[0] = _parser.Eval();
    *dVarAdress = dRight;
    dVal[1] = _parser.Eval();
    return Linearize(dLeft, dVal[0], dRight, dVal[1]);
}


/////////////////////////////////////////////////
/// \brief This function searches for the
/// positions of all zeroes (roots), which are
/// located in the selected interval.
///
/// \param sCmd string&
/// \param dVarAdress mu::value_type*
/// \param _parser Parser&
/// \param _option const Settings&
/// \param dLeft double
/// \param dRight double
/// \param dEps double
/// \param nRecursion int
/// \return mu::value_type
///
/// The expression has to be setted in advance.
/// The function performs recursions until the
/// defined precision is reached.
/////////////////////////////////////////////////
static mu::value_type localizeZero(string& sCmd, mu::value_type* dVarAdress, Parser& _parser, const Settings& _option, double dLeft, double dRight, double dEps, int nRecursion)
{
    const unsigned int nSamples = 101;
    mu::value_type dVal[2];

    if (_parser.GetExpr() != sCmd)
    {
        _parser.SetExpr(sCmd);
        _parser.Eval();
    }

    // Calculate the leftmost value
    *dVarAdress = dLeft;
    dVal[0] = _parser.Eval();

    // Separate the current interval in
    // nSamples steps and examine each step
    for (unsigned int i = 1; i < nSamples; i++)
    {
        // Calculate the next value
        *dVarAdress = dLeft + i * (dRight - dLeft) / (double)(nSamples - 1);
        dVal[1] = _parser.Eval();

        // Multiply the values to find a sign change
        if (dVal[0]*dVal[1] < 0)
        {
            // Sign change
            // return, if precision is reached. Otherwise perform
            // a new recursion between the two values
            if ((dRight - dLeft) / (double)(nSamples - 1) <= dEps || fabs(log(dEps)) + 1 < nRecursion * 2)
                return dLeft + (i - 1) * (dRight - dLeft) / (double)(nSamples - 1) + Linearize(0.0, dVal[0], (dRight - dLeft) / (double)(nSamples - 1), dVal[1]);
            else
                return localizeZero(sCmd, dVarAdress, _parser, _option, dLeft + (i - 1) * (dRight - dLeft) / (double)(nSamples - 1), dLeft + i * (dRight - dLeft) / (double)(nSamples - 1), dEps, nRecursion + 1);
        }
        else if (dVal[0]*dVal[1] == 0.0)
        {
            // One of the two vwlues is zero.
            // Jump over all following zeros due
            // to constness
            int nTemp = i - 1;

            if (dVal[0] != 0.0)
            {
                while (dVal[0]*dVal[1] == 0.0 && i + 1 < nSamples)
                {
                    i++;
                    *dVarAdress = dLeft + i * (dRight - dLeft) / (double)(nSamples - 1);
                    dVal[1] = _parser.Eval();
                }
            }
            else
            {
                while (dVal[1] == 0.0 && i + 1 < nSamples)
                {
                    i++;
                    *dVarAdress = dLeft + i * (dRight - dLeft) / (double)(nSamples - 1);
                    dVal[1] = _parser.Eval();
                }
            }

            // return, if precision is reached. Otherwise perform
            // a new recursion between the two values
            if ((i - nTemp) * (dRight - dLeft) / (double)(nSamples - 1) <= dEps || (!nTemp && i + 1 == nSamples) || fabs(log(dEps)) + 1 < nRecursion * 2)
                return dLeft + nTemp * (dRight - dLeft) / (double)(nSamples - 1) + Linearize(0.0, dVal[0], (i - nTemp) * (dRight - dLeft) / (double)(nSamples - 1), dVal[1]);
            else
                return localizeZero(sCmd, dVarAdress, _parser, _option, dLeft + nTemp * (dRight - dLeft) / (double)(nSamples - 1), dLeft + i * (dRight - dLeft) / (double)(nSamples - 1), dEps, nRecursion + 1);
        }

        dVal[0] = dVal[1];
    }

    // If no explict sign change was found,
    // interpolate the position by linearisation
    *dVarAdress = dLeft;
    dVal[0] = _parser.Eval();
    *dVarAdress = dRight;
    dVal[1] = _parser.Eval();
    return Linearize(dLeft, dVal[0], dRight, dVal[1]);
}


/////////////////////////////////////////////////
/// \brief This function approximates the passed
/// expression using Taylor's method.
///
/// \param cmdParser CommandLineParser&
/// \return void
///
/// The aproximated function is defined as a new
/// custom function.
/////////////////////////////////////////////////
void taylor(CommandLineParser& cmdParser)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    const Settings& _option = NumeReKernel::getInstance()->getSettings();

    string sVarName = "";
    string sExpr = cmdParser.getExprAsMathExpression();
    string sExpr_cpy = "";
    string sArg = "";
    string sTaylor = "Taylor";
    string sPolynom = "";
    bool bUseUniqueName = cmdParser.hasParam("unique") || cmdParser.hasParam("u");
    size_t nth_taylor = 6;
    size_t nSamples = 0;
    size_t nMiddle = 0;
    mu::value_type* dVar = 0;
    mu::value_type dVarValue = 0.0;
    long mu::value_type** dDiffValues = 0;

    // We cannot approximate string expressions
    if (containsStrings(sExpr))
        throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, cmdParser.getCommandLine(), SyntaxError::invalid_position, "taylor");

    // Extract the parameter list
    if (!cmdParser.getParameterList().length())
    {
        NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_TAYLOR_MISSINGPARAMS"), _option));
        return;
    }

    // Evaluate the parameters
    auto vParVal = cmdParser.getParameterValueAsNumericalValue("n");

    if (vParVal.size())
        nth_taylor = abs(intCast(vParVal.front()));

    std::vector<std::string> vParams = cmdParser.getAllParametersWithValues();

    for (const std::string& sPar : vParams)
    {
        if (sPar != "n")
        {
            sVarName = sPar;
            dVarValue = cmdParser.getParameterValueAsNumericalValue(sVarName).front();

            // Ensure that the location was chosen reasonable
            if (isinf(dVarValue) || isnan(dVarValue))
                return;

            // Create the string element, which is used
            // for the variable in the created funcction
            // string
            if (!dVarValue)
                sArg = "x";
            else if (dVarValue < 0)
                sArg = "x+" + toString(-dVarValue, _option.getPrecision());
            else
                sArg = "x-" + toString(dVarValue, _option.getPrecision());

            break;
        }
    }

    // Extract the expression
    sExpr_cpy = sExpr;

    // Create a unique function name, if it is desired
    if (bUseUniqueName)
        sTaylor += toString((int)nth_taylor) + "_" + cmdParser.getExpr();

    StripSpaces(sExpr);
    _parser.SetExpr(sExpr);

    // Ensure that the expression uses the selected variable
    if (!isVariableInAssignedExpression(_parser, sVarName))
    {
        NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_TAYLOR_CONSTEXPR", sVarName), _option));
        return;
    }

    // Get the address of the selected variable
    if (sVarName.length())
        dVar = getPointerToVariable(sVarName, _parser);

    if (!dVar)
        return;

    // If unique function names are desired,
    // generate them here by removing all operators
    // from the string
    if (bUseUniqueName)
    {
        string sOperators = " ,;-*/%^!<>&|?:=+[]{}()";

        for (unsigned int i = 0; i < sTaylor.length(); i++)
        {
            if (sOperators.find(sTaylor[i]) != string::npos)
            {
                sTaylor.erase(i, 1);
                i--;
            }
        }
    }

    sTaylor += "(x) := ";

    // Generate the taylor polynomial
    if (!nth_taylor)
    {
        // zero order polynomial
        *dVar = dVarValue;
        sTaylor += toString(_parser.Eval(), _option);
    }
    else if (nth_taylor == 1)
    {
        // First order polynomial
        *dVar = dVarValue;

        // the constant term
        sPolynom = toString(_parser.Eval(), _option) + ",";

        // Handle the linear term
        sPolynom += toString(_parser.Diff(dVar, dVarValue, 1e-7), _option);

        sTaylor += "polynomial(" + sArg + "," + sPolynom + ")";
    }
    else
    {
        // nth order polynomial
        *dVar = dVarValue;

        // the constant term
        sPolynom = toString(_parser.Eval(), _option) + ",";

        // Handle the linear term
        sPolynom += toString(_parser.Diff(dVar, dVarValue, 1e-7), _option) + ",";

        nSamples = 4 * nth_taylor + 1;
        nMiddle = 2 * nth_taylor;

        // Create the memory for the derivatives
        dDiffValues = new long double*[nSamples];
        for (size_t i = 0; i < nSamples; i++)
            dDiffValues[i] = new long double[2];

        // Fill the first column with the x-axis values
        for (size_t i = 0; i < nSamples; i++)
            dDiffValues[i][0] = dVarValue + ((double)i - (double)nMiddle) * 1e-1;

        // Fill the second column with the first
        // order derivatives
        for (size_t i = 0; i < nSamples; i++)
            dDiffValues[i][1] = _parser.Diff(dVar, dDiffValues[i][0], 1e-7);

        // Evaluate the nth taylor polynomial and the nth
        // order derivative
        for (size_t j = 1; j < nth_taylor; j++)
        {
            for (size_t i = nMiddle; i < nSamples - j; i++)
            {
                if (i == nMiddle)
                {
                    double dRight = (dDiffValues[nMiddle + 1][1] - dDiffValues[nMiddle][1]) / ((1.0 + (j - 1) * 0.5) * 1e-1);
                    double dLeft = (dDiffValues[nMiddle][1] - dDiffValues[nMiddle - 1][1]) / ((1.0 + (j - 1) * 0.5) * 1e-1);
                    dDiffValues[nMiddle][1] = (dLeft + dRight) / 2.0;
                }
                else
                {
                    dDiffValues[i][1] = (dDiffValues[i + 1][1] - dDiffValues[i][1]) / (1e-1);
                    dDiffValues[(int)nSamples - (int)i - 1][1] = (dDiffValues[(int)nSamples - (int)i - 1][1] - dDiffValues[(int)nSamples - (int)i - 2][1]) / (1e-1);
                }
            }

            sPolynom += toString((double)dDiffValues[nMiddle][1] / integralFactorial((int)j + 1), _option) + ",";
        }

        sTaylor += "polynomial(" + sArg + "," + sPolynom.substr(0, sPolynom.length()-1) + ")";

        for (size_t i = 0; i < nSamples; i++)
            delete[] dDiffValues[i];

        delete[] dDiffValues;
        dDiffValues = 0;
    }

    if (_option.systemPrints())
        NumeReKernel::print(LineBreak(sTaylor, _option, true, 0, 8));

    sTaylor += " " + _lang.get("PARSERFUNCS_TAYLOR_DEFINESTRING", sExpr_cpy, sVarName, toString(dVarValue, 4), toString((int)nth_taylor));

    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    bool bDefinitionSuccess = false;

    if (_functions.isDefined(sTaylor.substr(0, sTaylor.find(":="))))
        bDefinitionSuccess = _functions.defineFunc(sTaylor, true);
    else
        bDefinitionSuccess = _functions.defineFunc(sTaylor);

    if (bDefinitionSuccess)
        NumeReKernel::print(_lang.get("DEFINE_SUCCESS"), _option.systemPrints());
    else
        NumeReKernel::issueWarning(_lang.get("DEFINE_FAILURE"));

    return;
}


/////////////////////////////////////////////////
/// \brief This static function is a helper
/// function for fastFourierTransform() and
/// detects phase overflows, which can be
/// reconstructed during calculations.
///
/// \param cmplx std::complex<double>[3]
/// \return bool
///
/////////////////////////////////////////////////
static bool detectPhaseOverflow(std::complex<double> cmplx[3])
{
    return (fabs(std::arg(cmplx[2]) - std::arg(cmplx[1])) >= M_PI) && ((std::arg(cmplx[2]) - std::arg(cmplx[1])) * (std::arg(cmplx[1]) - std::arg(cmplx[0])) < 0);
}


/////////////////////////////////////////////////
/// \brief Calculates an axis index, which
/// performs the necessary data flips used for
/// the shifted fft axis.
///
/// \param nElements size_t
/// \param inverseTrafo bool
/// \return std::vector<size_t>
///
/////////////////////////////////////////////////
static std::vector<size_t> getShiftedAxis(size_t nElements, bool inverseTrafo)
{
    bool isOdd = nElements % 2;
    std::vector<size_t> vValues(nElements, 0u);
    size_t nOddVal;

    // Prepare the axis values
    for (size_t i = 0; i < nElements; i++)
    {
        vValues[i] = i;
    }

    // Extract the special odd value, if the axis
    // length is odd
    if (isOdd)
    {
        if (inverseTrafo)
        {
            nOddVal = vValues.back();
            vValues.pop_back();
        }
        else
        {
            nOddVal = vValues.front();
            vValues.erase(vValues.begin());
        }
    }

    // Create the actual axis: first part
    std::vector<size_t> vAxis(vValues.begin() + nElements / 2+(isOdd && inverseTrafo), vValues.end());

    // Insert the odd value, if necessay
    if (isOdd)
        vAxis.push_back(nOddVal);

    // second part
    vAxis.insert(vAxis.end(), vValues.begin(), vValues.begin() + nElements / 2+(isOdd && inverseTrafo));

    return vAxis;
}


/////////////////////////////////////////////////
/// \brief This function calculates the fast
/// fourier transform of the passed data set.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/// The user may switch between complex or phase-
/// amplitude layout and whether an inverse
/// transform shall be calculated.
/////////////////////////////////////////////////
bool fastFourierTransform(CommandLineParser& cmdParser)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    const Settings& _option = NumeReKernel::getInstance()->getSettings();

    mglDataC _fftData;
    Indices _idx;

    double dNyquistFrequency = 1.0;
    double dTimeInterval = 0.0;
    double dPhaseOffset = 0.0;
    double dFrequencyOffset = 0.0;
    bool bInverseTrafo = cmdParser.hasParam("inverse");
    bool bComplex = cmdParser.hasParam("complex");
    bool bShiftAxis = cmdParser.hasParam("axisshift");
    string sTargetTable = "fftdata";

    // search for explicit "target" options and select the target cache
    sTargetTable = cmdParser.getTargetTable(_idx, sTargetTable);

    // get the data from the data object
    NumeRe::Table _table = parser_extractData(cmdParser.getExpr(), _parser, _data, _option);

    dNyquistFrequency = _table.getLines() / (_table.getValue(_table.getLines() - 1, 0) - _table.getValue(0, 0)) / 2.0;
    dTimeInterval = (_table.getLines() - 1) / (_table.getValue(_table.getLines() - 1, 0));

    // Adapt the values for the shifted axis
    if (bShiftAxis)
    {
        dFrequencyOffset = -dNyquistFrequency * (1 + (_table.getLines() % 2) * 1.0 / _table.getLines());
        dTimeInterval = fabs((_table.getLines() + (_table.getLines() % 2)) / (_table.getValue(0, 0))) * 0.5;
    }

    if (_option.systemPrints())
    {
        if (!bInverseTrafo)
            NumeReKernel::printPreFmt(LineBreak("|-> " + _lang.get("PARSERFUNCS_FFT_FOURIERTRANSFORMING", toString(_table.getCols()), toString(dNyquistFrequency, 6)) + " ", _option, 0));
        else
            NumeReKernel::printPreFmt(LineBreak("|-> " + _lang.get("PARSERFUNCS_FFT_INVERSE_FOURIERTRANSFORMING", toString(_table.getCols()), toString(dTimeInterval, 6)) + " ", _option, 0));
    }

    _fftData.Create(_table.getLines());

    std::vector<size_t> vAxis;

    // Prepare the axis (shifted if necessary)
    if (bShiftAxis && bInverseTrafo)
        vAxis = getShiftedAxis(_table.getLines(), bInverseTrafo);
    else
    {
        for (size_t i = 0; i < _table.getLines(); i++)
            vAxis.push_back(i);
    }

    // Copy the data
    for (size_t i = 0; i < _table.getLines(); i++)
    {
        if (_table.getCols() == 2)
            _fftData.a[i] = dual(_table.getValue(vAxis[i], 1), 0.0);
        else if (_table.getCols() == 3 && bComplex)
            _fftData.a[i] = dual(_table.getValue(vAxis[i], 1), _table.getValue(vAxis[i], 2));
        else if (_table.getCols() == 3 && !bComplex)
            _fftData.a[i] = dual(_table.getValue(vAxis[i], 1) * cos(_table.getValue(vAxis[i], 2)), _table.getValue(vAxis[i], 1) * sin(_table.getValue(vAxis[i], 2)));
    }

    // Calculate the actual transformation and apply some
    // normalisation
    if (!bInverseTrafo)
    {
        _fftData.FFT("x");

        double samples = _table.getLines()/2.0;

        _fftData.a[0] /= dual(2*samples, 0.0);

        for (long long int i = 1; i < _fftData.GetNx(); i++)
            _fftData.a[i] /= dual(samples, 0.0);
    }
    else
    {
        double samples = _fftData.GetNx()/2;

        _fftData.a[0] *= dual(2*samples, 0.0);

        for (long long int i = 1; i < _fftData.GetNx(); i++)
            _fftData.a[i] *= dual(samples, 0.0);

        _fftData.FFT("ix");
    }

    if (_idx.col.isOpenEnd())
        _idx.col.setRange(0, _idx.col.front() + 3);

    if (bShiftAxis && !bInverseTrafo)
        vAxis = getShiftedAxis(_table.getLines(), bInverseTrafo);

    // Store the results of the transformation in the target
    // table
    if (!bInverseTrafo)
    {
        long long int nElements = _fftData.GetNx();

        if (_idx.row.isOpenEnd())
            _idx.row.setRange(0, _idx.row.front() + nElements);

        for (long long int i = 0; i < nElements; i++)
        {
            if (i > _idx.row.size())
                break;

            _data.writeToTable(_idx.row[i], _idx.col.front(), sTargetTable, dFrequencyOffset + 2.0 * (double)(i)*dNyquistFrequency / (double)(_fftData.GetNx()));

            if (!bComplex)
            {
                _data.writeToTable(_idx.row[vAxis[i]], _idx.col[1], sTargetTable, std::abs(_fftData.a[i]));

                // Stitch phase overflows into a continous array
                if (i > 2 && detectPhaseOverflow(&_fftData.a[i-2]))
                {
                    if (std::arg(_fftData.a[i - 1]) - std::arg(_fftData.a[i - 2]) < 0.0)
                        dPhaseOffset -= 2 * M_PI;
                    else if (std::arg(_fftData.a[i - 1]) - std::arg(_fftData.a[i - 2]) > 0.0)
                        dPhaseOffset += 2 * M_PI;
                }

                _data.writeToTable(_idx.row[vAxis[i]], _idx.col[2], sTargetTable, std::arg(_fftData.a[i]) + dPhaseOffset);
            }
            else
            {
                _data.writeToTable(_idx.row[vAxis[i]], _idx.col[1], sTargetTable, _fftData.a[i].real());
                _data.writeToTable(_idx.row[vAxis[i]], _idx.col[2], sTargetTable, _fftData.a[i].imag());
            }
        }

        // Write headlines
        _data.setHeadLineElement(_idx.col.front(), sTargetTable, _lang.get("COMMON_FREQUENCY") + "_[Hz]");

        if (!bComplex)
        {
            _data.setHeadLineElement(_idx.col[1], sTargetTable, _lang.get("COMMON_AMPLITUDE"));
            _data.setHeadLineElement(_idx.col[2], sTargetTable, _lang.get("COMMON_PHASE") + "_[rad]");
        }
        else
        {
            _data.setHeadLineElement(_idx.col[1], sTargetTable, "Re(" + _lang.get("COMMON_AMPLITUDE") + ")");
            _data.setHeadLineElement(_idx.col[2], sTargetTable, "Im(" + _lang.get("COMMON_AMPLITUDE") + ")");
        }
    }
    else
    {
        if (_idx.row.isOpenEnd())
            _idx.row.setRange(0, _idx.row.front() + _fftData.GetNx());

        for (long long int i = 0; i < _fftData.GetNx(); i++)
        {
            if (i > _idx.row.size())
                break;

            _data.writeToTable(_idx.row[i], _idx.col[0], sTargetTable, (double)(i)*dTimeInterval / (double)(_fftData.GetNx() - 1));
            _data.writeToTable(_idx.row[i], _idx.col[1], sTargetTable, _fftData.a[i].real());
            _data.writeToTable(_idx.row[i], _idx.col[2], sTargetTable, _fftData.a[i].imag());
        }

        // Write headlines
        _data.setHeadLineElement(_idx.col[0], sTargetTable, _lang.get("COMMON_TIME") + "_[s]");
        _data.setHeadLineElement(_idx.col[1], sTargetTable, "Re(" + _lang.get("COMMON_SIGNAL") + ")");
        _data.setHeadLineElement(_idx.col[2], sTargetTable, "Im(" + _lang.get("COMMON_SIGNAL") + ")");
    }

    if (_option.systemPrints())
        NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_DONE")) + ".\n");

    return true;
}


/////////////////////////////////////////////////
/// \brief This function calculates the fast
/// wavelet transform of the passed data set.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/// The user may select the wavelet type from a
/// predefined set of wavelets and determine,
/// whether an inverse transform shall be
/// calculated.
/////////////////////////////////////////////////
bool fastWaveletTransform(CommandLineParser& cmdParser)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    const Settings& _option = NumeReKernel::getInstance()->getSettings();

    vector<double> vWaveletData;
    vector<double> vAxisData;
    Indices _idx;

    bool bInverseTrafo = cmdParser.hasParam("inverse");
    bool bTargetGrid = cmdParser.hasParam("grid");
    string sTargetTable = "fwtdata";
    string sType = "d"; // d = daubechies, cd = centered daubechies, h = haar, ch = centered haar, b = bspline, cb = centered bspline
    int k = 4;

    std::string sParVal = cmdParser.getParameterValue("type");

    if (sParVal.length())
        sType = sParVal;

    std::vector<double> vParVal = cmdParser.getParameterValueAsNumericalValue("k");

    if (vParVal.size())
        k = intCast(vParVal.front());


    // search for explicit "target" options and select the target cache
    sTargetTable = cmdParser.getTargetTable(_idx, sTargetTable);

    // get the data from the data object
    NumeRe::Table _table = parser_extractData(cmdParser.getExpr(), _parser, _data, _option);

    if (_option.systemPrints())
    {
        string sExplType = "";

        if (sType.front() == 'c')
            sExplType = "Centered ";

        if (sType.back() == 'd' || sType.find("daubechies") != string::npos)
            sExplType += "Daubechies";
        else if (sType.back() == 'h' || sType.find("haar") != string::npos)
            sExplType += "Haar";
        else if (sType.back() == 'b' || sType.find("bspline") != string::npos)
            sExplType += "BSpline";

        if (!bInverseTrafo)
            NumeReKernel::printPreFmt(LineBreak("|-> " + _lang.get("PARSERFUNCS_WAVELET_TRANSFORMING", sExplType) + " ", _option, 0));
        else
            NumeReKernel::printPreFmt(LineBreak("|-> " + _lang.get("PARSERFUNCS_WAVELET_INVERSE_TRANSFORMING", sExplType) + " ", _option, 0));
    }

    for (size_t i = 0; i < _table.getLines(); i++)
    {
        vWaveletData.push_back(_table.getValue(i, 1));

        if (bTargetGrid)
            vAxisData.push_back(_table.getValue(i, 0));
    }

    // calculate the wavelet:
    if (sType == "d" || sType == "daubechies")
        calculateWavelet(vWaveletData, Daubechies, k, !bInverseTrafo);
    else if (sType == "cd" || sType == "cdaubechies")
        calculateWavelet(vWaveletData, CenteredDaubechies, k, !bInverseTrafo);
    else if (sType == "h" || sType == "haar")
        calculateWavelet(vWaveletData, Haar, k, !bInverseTrafo);
    else if (sType == "ch" || sType == "chaar")
        calculateWavelet(vWaveletData, CenteredHaar, k, !bInverseTrafo);
    else if (sType == "b" || sType == "bspline")
        calculateWavelet(vWaveletData, BSpline, k, !bInverseTrafo);
    else if (sType == "cb" || sType == "cbspline")
        calculateWavelet(vWaveletData, CenteredBSpline, k, !bInverseTrafo);

    // write the output as datagrid for plotting (only if not an inverse trafo)
    if (bTargetGrid && !bInverseTrafo)
    {
        NumeRe::Table tWaveletData = decodeWaveletData(vWaveletData, vAxisData);

        if (_idx.col.isOpenEnd())
            _idx.col.setRange(0, _idx.col.front() + tWaveletData.getCols()-1);

        if (_idx.row.isOpenEnd())
            _idx.row.setRange(0, _idx.row.front() + tWaveletData.getLines()-1);

        for (size_t i = 0; i < tWaveletData.getLines(); i++)
        {
            if (_idx.row[i] == VectorIndex::INVALID)
                break;

            for (size_t j = 0; j < tWaveletData.getCols(); j++)
            {
                // write the headlines
                if (!i)
                {
                    string sHeadline = "";

                    if (!j)
                        sHeadline = _lang.get("COMMON_TIME");
                    else if (j == 1)
                        sHeadline = _lang.get("COMMON_LEVEL");
                    else
                        sHeadline = _lang.get("COMMON_COEFFICIENT");

                    _data.setHeadLineElement(_idx.col[j], sTargetTable, sHeadline);
                }

                if (_idx.col[j] == VectorIndex::INVALID)
                    break;

                _data.writeToTable(_idx.row[i], _idx.col[j], sTargetTable, tWaveletData.getValue(i, j));
            }
        }

        if (_option.systemPrints())
            NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_DONE")) + ".\n");

        return true;
    }

    // write the output as usual data rows
    if (_idx.col.isOpenEnd())
        _idx.col.setRange(0, _idx.col.front() + 2);

    if (_idx.row.isOpenEnd())
        _idx.row.setRange(0,  _idx.row.front() + vWaveletData.size()-1);

    for (long long int i = 0; i < vWaveletData.size(); i++)
    {
        if (_idx.row[i] == VectorIndex::INVALID)
            break;

        _data.writeToTable(_idx.row[i], _idx.col[0], sTargetTable, (double)(i));
        _data.writeToTable(_idx.row[i], _idx.col[1], sTargetTable, vWaveletData[i]);
    }

    if (!bInverseTrafo)
    {
        _data.setHeadLineElement(_idx.col[0], sTargetTable, _lang.get("COMMON_COEFFICIENT"));
        _data.setHeadLineElement(_idx.col[1], sTargetTable, _lang.get("COMMON_AMPLITUDE"));
    }
    else
    {
        _data.setHeadLineElement(_idx.col[0], sTargetTable, _lang.get("COMMON_TIME"));
        _data.setHeadLineElement(_idx.col[1], sTargetTable, _lang.get("COMMON_SIGNAL"));
    }

    if (_option.systemPrints())
        NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_DONE")) + ".\n");

    return true;
}


/////////////////////////////////////////////////
/// \brief This function samples a defined
/// expression in an array of discrete values.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool evalPoints(CommandLineParser& cmdParser)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    unsigned int nSamples = 100;
    mu::value_type* dVar = 0;
    mu::value_type dTemp = 0.0;
    string sExpr = cmdParser.getExprAsMathExpression();
    string sVar = "x";
    static string zero = "0.0";
    bool bLogarithmic = cmdParser.hasParam("logscale");

    // Evaluate calls in the expression
    // to any table or cluster
    if (_data.containsTablesOrClusters(sExpr))
    {
        getDataElements(sExpr, _parser, _data, NumeReKernel::getInstance()->getSettings());

        if (sExpr.find("{") != string::npos)
            convertVectorToExpression(sExpr, NumeReKernel::getInstance()->getSettings());
    }

    IntervalSet ivl = cmdParser.parseIntervals();

    // Extract the interval definition
    if (!ivl.size() && cmdParser.getParameterList().find('=') != string::npos)
    {
        std::vector<std::string> vParams = cmdParser.getAllParametersWithValues();

        for (const std::string& sPar : vParams)
        {
            if (sPar != "samples")
            {
                sVar = sPar;
                std::string sInterval = cmdParser.getParameterValue(sPar);

                if (sInterval.front() == '[' && sInterval.back() == ']')
                {
                    sInterval.pop_back();
                    sInterval.erase(0, 1);
                }

                auto indices = getAllIndices(sInterval);

                _parser.SetExpr(indices[0] + "," + indices[1]);
                int nIndices;
                mu::value_type* res = _parser.Eval(nIndices);
                ivl.intervals.push_back(Interval(res[0], res[1]));

                break;
            }
        }
    }

    if (!ivl.size())
        ivl.intervals.push_back(Interval(-10.0, 10.0));

    std::vector<double> vSamples = cmdParser.getParameterValueAsNumericalValue("samples");

    if (vSamples.size())
        nSamples = intCast(vSamples.front());
    else if (ivl[0].getSamples())
        nSamples = ivl[0].getSamples();

    if (isNotEmptyExpression(sExpr))
        _parser.SetExpr(sExpr);
    else
        _parser.SetExpr(sVar);

    _parser.Eval();
    dVar = getPointerToVariable(sVar, _parser);

    if (!dVar)
        throw SyntaxError(SyntaxError::EVAL_VAR_NOT_FOUND, cmdParser.getCommandLine(), sVar, sVar);

    if (isnan(ivl[0].front()) && isnan(ivl[0].back()))
        ivl[0] = Interval(-10.0, 10.0);
    else if (isnan(ivl[0].front()) || isnan(ivl[0].back()) || isinf(ivl[0].front()) || isinf(ivl[0].back()))
    {
        cmdParser.setReturnValue("nan");
        return false;
    }

    if (bLogarithmic && (ivl[0].min() <= 0.0))
        throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Set the corresponding expression
    if (isNotEmptyExpression(sExpr))
        _parser.SetExpr(sExpr);
    else if (dVar)
        _parser.SetExpr(sVar);
    else
        _parser.SetExpr(zero);

    _parser.Eval();
    vector<mu::value_type> vResults;

    // Evaluate the selected expression at
    // the selected samples
    if (dVar)
    {
        dTemp = *dVar;
        *dVar = ivl[0](0);
        vResults.push_back(_parser.Eval());

        for (unsigned int i = 1; i < nSamples; i++)
        {
            // Is a logarithmic distribution needed?
            if (bLogarithmic)
                *dVar = ivl[0].log(i, nSamples);
            else
                *dVar = ivl[0](i, nSamples);

            vResults.push_back(_parser.Eval());
        }

        *dVar = dTemp;
    }
    else
    {
        for (unsigned int i = 0; i < nSamples; i++)
            vResults.push_back(_parser.Eval());
    }

    cmdParser.setReturnValue(vResults);
    return true;
}


/////////////////////////////////////////////////
/// \brief This function calculates a datagrid
/// from passed functions or (x-y-z) data values.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool createDatagrid(CommandLineParser& cmdParser)
{
    unsigned int nSamples = 100;
    bool bTranspose = cmdParser.hasParam("transpose");

    Indices _iTargetIndex;
    vector<vector<double> > vZVals;

    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    // search for explicit "target" options and select the target cache
    std::string sTargetCache = cmdParser.getTargetTable(_iTargetIndex, "grid");

    cmdParser.clearReturnValue();
    cmdParser.setReturnValue(sTargetCache);

    // Get the intervals
    IntervalSet ivl = cmdParser.parseIntervals();

    // Add missing intervals
    while (ivl.size() < 2)
    {
        ivl.intervals.push_back(Interval(-10.0, 10.0));
    }

    // Get the number of samples from the option list
    auto vParVal = cmdParser.getParameterValueAsNumericalValue("samples");

    if (vParVal.size())
        nSamples = abs(intCast(vParVal.front()));

    if (nSamples < 2)
        throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Get the samples
    std::vector<size_t> vSamples = getSamplesForDatagrid(cmdParser, nSamples);

    // extract samples from the interval set
    if (ivl[0].getSamples())
        vSamples[bTranspose] = ivl[0].getSamples();

    if (ivl[1].getSamples())
        vSamples[1-bTranspose] = ivl[1].getSamples();

    //>> Z-Matrix
    if (cmdParser.exprContainsDataObjects())
    {
        // Get the datagrid from another table
        DataAccessParser _accessParser = cmdParser.getExprAsDataObject();

        if (!_accessParser.getDataObject().length() || _accessParser.isCluster())
            throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, cmdParser.getCommandLine(), SyntaxError::invalid_position);

        Indices& _idx = _accessParser.getIndices();

        // identify the table
        std::string& szDatatable = _accessParser.getDataObject();

        // Check the indices
        if (!isValidIndexSet(_idx))
            throw SyntaxError(SyntaxError::INVALID_INDEX, cmdParser.getCommandLine(), SyntaxError::invalid_position, _idx.row.to_string() + ", " + _idx.col.to_string());

        // the indices are vectors
        vector<double> vVector;

        if (_idx.col.isOpenEnd())
            _idx.col.setRange(0, _data.getCols(szDatatable)-1);

        if (_idx.row.isOpenEnd())
        {
            _idx.row.setRange(0, _data.getLines(szDatatable, true) - _data.getAppendedZeroes(_idx.col.front(), szDatatable) - 1);

            for (size_t j = 1; j < _idx.col.size(); j++)
            {
                if (_data.getLines(szDatatable, true) - _data.getAppendedZeroes(_idx.col[j], szDatatable) > _idx.row.back())
                    _idx.row.setRange(0, _data.getLines(szDatatable, true) - _data.getAppendedZeroes(_idx.col[j], szDatatable) - 1);
            }
        }

        // Get the data. Choose the order of reading depending on the "transpose" command line option
        if (!bTranspose)
        {
            for (size_t i = 0; i < _idx.row.size(); i++)
            {
                vVector = _data.getElement(VectorIndex(_idx.row[i]), _idx.col, szDatatable);
                vZVals.push_back(vVector);
                vVector.clear();
            }
        }
        else
        {
            for (size_t j = 0; j < _idx.col.size(); j++)
            {
                vVector = _data.getElement(_idx.row, VectorIndex(_idx.col[j]), szDatatable);
                vZVals.push_back(vVector);
                vVector.clear();
            }
        }

        // Check the content of the z matrix
        if (!vZVals.size() || (vZVals.size() == 1 && vZVals[0].size() == 1))
            throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, cmdParser.getCommandLine(), SyntaxError::invalid_position);

        // Expand the z vector into a matrix for the datagrid if necessary
        expandVectorToDatagrid(ivl, vZVals, vSamples[bTranspose], vSamples[1 - bTranspose]);
    }
    else
    {
        Parser& _parser = NumeReKernel::getInstance()->getParser();

        // Calculate the grid from formula
        _parser.SetExpr(cmdParser.getExprAsMathExpression());

        vector<mu::value_type> vVector;

        for (unsigned int x = 0; x < vSamples[bTranspose]; x++)
        {
            _defVars.vValue[0][0] = ivl[0](x, vSamples[bTranspose]);

            for (unsigned int y = 0; y < vSamples[1-bTranspose]; y++)
            {
                _defVars.vValue[1][0] = ivl[1](y, vSamples[1-bTranspose]);
                vVector.push_back(_parser.Eval());
            }

            vZVals.push_back(vVector);
            vVector.clear();
        }
    }

    // Store the results in the target cache
    if (_iTargetIndex.row.isOpenEnd())
        _iTargetIndex.row.setRange(0, _iTargetIndex.row.front() + vSamples[bTranspose] - 1);

    if (_iTargetIndex.col.isOpenEnd())
        _iTargetIndex.col.setRange(0, _iTargetIndex.col.front() + vSamples[1-bTranspose] + 1);

    // Write the x axis
    for (size_t i = 0; i < vSamples[bTranspose]; i++)
        _data.writeToTable(i, _iTargetIndex.col[0], sTargetCache, ivl[0](i, vSamples[bTranspose]));

    _data.setHeadLineElement(_iTargetIndex.col[0], sTargetCache, "x");

    // Write the y axis
    for (size_t i = 0; i < vSamples[1-bTranspose]; i++)
        _data.writeToTable(i, _iTargetIndex.col[1], sTargetCache, ivl[1](i, vSamples[1-bTranspose]));

    _data.setHeadLineElement(_iTargetIndex.col[1], sTargetCache, "y");

    // Write the z matrix
    for (size_t i = 0; i < vZVals.size(); i++)
    {
        if (_iTargetIndex.row[i] == VectorIndex::INVALID)
            break;

        for (size_t j = 0; j < vZVals[i].size(); j++)
        {
            if (_iTargetIndex.col[j+2] == VectorIndex::INVALID)
                break;

            _data.writeToTable(_iTargetIndex.row[i], _iTargetIndex.col[j+2], sTargetCache, vZVals[i][j]);

            if (!i)
                _data.setHeadLineElement(_iTargetIndex.col[j+2], sTargetCache, "z(x(:),y(" + toString((int)j + 1) + "))");
        }
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This function will obtain the samples
/// of the datagrid for each spatial direction.
///
/// \param cmdParser CommandLineParser&
/// \param nSamples size_t
/// \return vector<size_t>
///
/////////////////////////////////////////////////
static vector<size_t> getSamplesForDatagrid(CommandLineParser& cmdParser, size_t nSamples)
{
    vector<size_t> vSamples;

    // If the z vals are inside of a table then obtain the correct number of samples here
    if (cmdParser.exprContainsDataObjects())
    {
        MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

        // Get the indices and identify the table name
        DataAccessParser _accessParser = cmdParser.getExprAsDataObject();

        if (!_accessParser.getDataObject().length() || _accessParser.isCluster())
            throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, cmdParser.getCommandLine(), SyntaxError::invalid_position);

        Indices& _idx = _accessParser.getIndices();
        std::string& sZDatatable = _accessParser.getDataObject();

        // Check the indices
        if (!isValidIndexSet(_idx))
            throw SyntaxError(SyntaxError::INVALID_INDEX, cmdParser.getCommandLine(), SyntaxError::invalid_position, _idx.row.to_string() + ", " + _idx.col.to_string());

        // The indices are vectors
        if (_idx.col.isOpenEnd())
            _idx.col.setRange(0, _data.getCols(sZDatatable)-1);

        if (_idx.row.isOpenEnd())
        {
            _idx.row.setRange(0, _data.getLines(sZDatatable, true) - _data.getAppendedZeroes(_idx.col.front(), sZDatatable) - 1);

            for (size_t j = 1; j < _idx.col.size(); j++)
            {
                if (_data.getLines(sZDatatable, true) - _data.getAppendedZeroes(_idx.col[j], sZDatatable) > _idx.row.back())
                    _idx.row.setRange(0, _data.getLines(sZDatatable, true) - _data.getAppendedZeroes(_idx.col[j], sZDatatable) - 1);
            }
        }

        vSamples.push_back(_idx.row.size());
        vSamples.push_back(_idx.col.size());

        // Check for singletons
        if (vSamples[0] < 2 && vSamples[1] >= 2)
            vSamples[0] = vSamples[1];
        else if (vSamples[1] < 2 && vSamples[0] >= 2)
            vSamples[1] = vSamples[0];
    }
    else
    {
        vSamples.push_back(nSamples);
        vSamples.push_back(nSamples);
    }

    if (vSamples.size() < 2 || vSamples[0] < 2 || vSamples[1] < 2)
        throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    return vSamples;
}


/////////////////////////////////////////////////
/// \brief This function will expand the z vector
/// into a z matrix using triangulation.
///
/// \param ivl IntervalSet&
/// \param vZVals vector<vector<double>>&
/// \param nSamples_x size_t
/// \param nSamples_y size_t
/// \return void
///
/////////////////////////////////////////////////
static void expandVectorToDatagrid(IntervalSet& ivl, vector<vector<double>>& vZVals, size_t nSamples_x, size_t nSamples_y)
{
    // Only if a dimension is a singleton
    if (vZVals.size() == 1 || vZVals[0].size() == 1)
    {
        vector<double> vVector;

        // construct the needed MGL objects
        mglData _mData[4];
        mglGraph _graph;

        // Prepare the memory
        _mData[0].Create(nSamples_x, nSamples_y);
        _mData[1].Create(nSamples_x);
        _mData[2].Create(nSamples_y);

        if (vZVals.size() != 1)
            _mData[3].Create(vZVals.size());
        else
            _mData[3].Create(vZVals[0].size());

        // copy the x and y vectors
        for (unsigned int i = 0; i < nSamples_x; i++)
            _mData[1].a[i] = ivl[0](i, nSamples_x);

        for (unsigned int i = 0; i < nSamples_y; i++)
            _mData[2].a[i] = ivl[1](i, nSamples_y);

        // copy the z vector
        if (vZVals.size() != 1)
        {
            for (unsigned int i = 0; i < vZVals.size(); i++)
                _mData[3].a[i] = vZVals[i][0];
        }
        else
        {
            for (unsigned int i = 0; i < vZVals[0].size(); i++)
                _mData[3].a[i] = vZVals[0][i];
        }

        // Set the ranges needed for the DataGrid function
        _graph.SetRanges(_mData[1], _mData[2], _mData[3]);

        // Calculate the data grid using a triangulation
        _graph.DataGrid(_mData[0], _mData[1], _mData[2], _mData[3]);

        vZVals.clear();

        // Refill the x and y vectors
        ivl[0] = Interval(_mData[1].Minimal(), _mData[1].Maximal());
        ivl[1] = Interval(_mData[2].Minimal(), _mData[2].Maximal());

        // Copy the z matrix
        for (unsigned int i = 0; i < nSamples_x; i++)
        {
            for (unsigned int j = 0; j < nSamples_y; j++)
                vVector.push_back(_mData[0].a[i + nSamples_x * j]);

            vZVals.push_back(vVector);
            vVector.clear();
        }
    }
}


/////////////////////////////////////////////////
/// \brief This function creates a WAVE file from
/// the selected data set.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool writeAudioFile(CommandLineParser& cmdParser)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    string sAudioFileName = "<savepath>/audiofile.wav";
    int nSamples = 44100;
    int nChannels = 1;
    double dMax = 0.0;
    double dMin = 0.0;

    // Samples lesen
    std::vector<double> vVals = cmdParser.getParameterValueAsNumericalValue("samples");

    if (vVals.size())
        nSamples = intCast(vVals.front());

    // Dateiname lesen
    sAudioFileName = cmdParser.getFileParameterValueForSaving(".wav", "<savepath>", sAudioFileName);

    // Indices lesen
    DataAccessParser _accessParser = cmdParser.getExprAsDataObject();
    Indices& _idx = _accessParser.getIndices();

    if (_idx.col.isOpenEnd())
        _idx.col.setRange(0, _idx.col.front() + 1);

    _accessParser.evalIndices();

    if (_idx.col.size() > 2)
        return false;

    // Find the absolute maximal value
    dMin = fabs(_data.min(_accessParser.getDataObject(), _idx.row, _idx.col));
    dMax = fabs(_data.max(_accessParser.getDataObject(), _idx.row, _idx.col));

    dMax = std::max(dMin, dMax);

    nChannels = _idx.col.size() > 1 ? 2 : 1;

    std::unique_ptr<Audio::File> audiofile(Audio::getAudioFileByType(sAudioFileName));

    if (!audiofile.get())
        return false;

    audiofile.get()->setChannels(nChannels);
    audiofile.get()->setSampleRate(nSamples);
    audiofile.get()->newFile();

    if (!audiofile.get()->isValid())
        return false;

    for (size_t i = 0; i < _idx.row.size(); i++)
    {
        audiofile.get()->write(Audio::Sample(_data.getElement(_idx.row[i], _idx.col[0], _accessParser.getDataObject()) / dMax, nChannels > 1 ? _data.getElement(_idx.row[i], _idx.col[1], _accessParser.getDataObject()) / dMax : NAN));
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Reads either the audio file meta
/// information or the whole audio file to memory.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool readAudioFile(CommandLineParser& cmdParser)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    Indices _targetIdx;
    std::string sTarget = cmdParser.getTargetTable(_targetIdx, "");

    // Read the whole table or only the metadata
    if (sTarget.length())
    {
        std::unique_ptr<Audio::File> audiofile(Audio::getAudioFileByType(cmdParser.getExprAsFileName(".wav")));

        if (!audiofile.get() || !audiofile.get()->isValid())
            return false;

        size_t nLen = audiofile.get()->getLength();
        size_t nChannels = audiofile.get()->getChannels();

        // Try to read the entire file
        _data.resizeTable(_targetIdx.row.size() < nLen ? _targetIdx.row.max()+1 : _targetIdx.row.min()+nLen,
                          nChannels > 1 && _targetIdx.col.size() > 1 ? _targetIdx.col.subidx(0, 2).max()+1 : _targetIdx.col.front()+1,
                          sTarget);

        for (size_t i = 0; i < nLen; i++)
        {
            Audio::Sample sample = audiofile.get()->read();

            if (_targetIdx.row.size() <= i)
                break;

            _data.writeToTable(_targetIdx.row[i], _targetIdx.col[0], sTarget, sample.leftOrMono);

            if (nChannels > 1 && _targetIdx.col.size() > 1)
                _data.writeToTable(_targetIdx.row[i], _targetIdx.col[1], sTarget, sample.right);
        }

        // Create the storage indices
        std::vector<double> vIndices = {_targetIdx.row.min()+1,
            _targetIdx.row.size() < nLen ? _targetIdx.row.max()+1 : _targetIdx.row.min()+nLen,
            (nChannels > 1 && _targetIdx.col.size() > 1 ? _targetIdx.col.subidx(0, 2).min()+1 : _targetIdx.col.front()+1),
            (nChannels > 1 && _targetIdx.col.size() > 1 ? _targetIdx.col.subidx(0, 2).max()+1 : _targetIdx.col.front()+1)};

        cmdParser.setReturnValue(vIndices);
    }
    else
    {
        std::unique_ptr<Audio::File> audiofile(Audio::getAudioFileByType(cmdParser.getExprAsFileName(".wav")));

        if (!audiofile.get() || !audiofile.get()->isValid())
            return false;

        // Only read the metadata
        size_t nLen = audiofile.get()->getLength();
        size_t nChannels = audiofile.get()->getChannels();
        size_t nSampleRate = audiofile.get()->getSampleRate();

        // Create the metadata
        std::vector<double> vMetaData = {nLen, nChannels, nSampleRate, nLen / (double)nSampleRate};

        cmdParser.setReturnValue(vMetaData);
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Seek a position in an audiofile and
/// extract a length of samples from it.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool seekInAudioFile(CommandLineParser& cmdParser)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    Indices _targetIdx;
    std::string sTarget = cmdParser.getTargetTable(_targetIdx, "audiodata");
    std::vector<double> vSeekIndices = cmdParser.parseExprAsNumericalValues();

    if (vSeekIndices.size() < 2)
        return false;

    std::unique_ptr<Audio::File> audiofile(Audio::getAudioFileByType(cmdParser.getFileParameterValue(".wav")));

    if (!audiofile.get() || !audiofile.get()->isValid())
        return false;

    size_t nLen = audiofile.get()->getLength();
    size_t nChannels = audiofile.get()->getChannels();

    if (!audiofile.get()->isSeekable() || std::max(vSeekIndices[0]-1, 0.0) >= nLen)
        return false;

    std::unique_ptr<Audio::SeekableFile> seekable(static_cast<Audio::SeekableFile*>(audiofile.release()));

    seekable.get()->setPosition(std::max(vSeekIndices[0]-1, 0.0));
    nLen = std::min(nLen - seekable.get()->getPosition(), (size_t)(std::max(vSeekIndices[1], 0.0)));

    // Try to read the desired length from the file
    _data.resizeTable(_targetIdx.row.size() < nLen ? _targetIdx.row.max()+1 : _targetIdx.row.min()+nLen,
                      nChannels > 1 && _targetIdx.col.size() > 1 ? _targetIdx.col.subidx(0, 2).max()+1 : _targetIdx.col.front()+1,
                      sTarget);

    for (size_t i = 0; i < nLen; i++)
    {
        Audio::Sample sample = seekable.get()->read();

        if (_targetIdx.row.size() <= i)
            break;

        _data.writeToTable(_targetIdx.row[i], _targetIdx.col[0], sTarget, sample.leftOrMono);

        if (nChannels > 1 && _targetIdx.col.size() > 1)
            _data.writeToTable(_targetIdx.row[i], _targetIdx.col[1], sTarget, sample.right);
    }

    cmdParser.setReturnValue(toString(nLen));

    return true;
}


/////////////////////////////////////////////////
/// \brief This function regularizes the samples
/// of a defined x-y-data array such that DeltaX
/// is equal for every x.
///
/// \param CommandLineParser& cmdParser
/// \return bool
///
/////////////////////////////////////////////////
bool regularizeDataSet(CommandLineParser& cmdParser)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    int nSamples = 0;
    string sColHeaders[2] = {"", ""};
    double dXmin, dXmax;

    // Samples lesen
    auto vParVal = cmdParser.getParameterValueAsNumericalValue("samples");

    if (vParVal.size())
        nSamples = intCast(vParVal.front());

    // Indices lesen
    DataAccessParser accessParser = cmdParser.getExprAsDataObject();
    Indices& _idx = accessParser.getIndices();

    MemoryManager _cache;
    getData(accessParser.getDataObject(), _idx, _data, _cache);

    _cache.sortElements("sort -table cols=1[2]");

    sColHeaders[0] = _cache.getHeadLineElement(0, "table") + "\\n(regularized)";
    sColHeaders[1] = _cache.getHeadLineElement(1, "table") + "\\n(regularized)";

    long long int nLines = _cache.getLines("table", false);

    dXmin = _cache.min("table", 0, nLines - 1, 0);
    dXmax = _cache.max("table", 0, nLines - 1, 0);

    // Create splines
    tk::spline _spline;
    _spline.set_points(_cache.getElement(VectorIndex(0, nLines-1), VectorIndex(0), "table"), _cache.getElement(VectorIndex(0, nLines-1), VectorIndex(1), "table"), false);

    if (!nSamples)
        nSamples = nLines;

    long long int nLastCol = _data.getCols(accessParser.getDataObject(), false);

    // Interpolate the data points
    for (long long int i = 0; i < nSamples; i++)
    {
        _data.writeToTable(i, nLastCol, accessParser.getDataObject(), dXmin + i * (dXmax-dXmin) / (nSamples-1));
        _data.writeToTable(i, nLastCol + 1, accessParser.getDataObject(), _spline(dXmin + i*(dXmax-dXmin) / (nSamples-1)));
    }

    _data.setHeadLineElement(nLastCol, accessParser.getDataObject(), sColHeaders[0]);
    _data.setHeadLineElement(nLastCol + 1, accessParser.getDataObject(), sColHeaders[1]);
    return true;
}


/////////////////////////////////////////////////
/// \brief This function performs a pulse
/// analysis on the selected data set.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/// The function calculates the maximal amplitude,
/// its position, the FWHM, the width near the
/// maximal amplitude (which is different from
/// the FWHM) and the energy in the pulse.
/////////////////////////////////////////////////
bool analyzePulse(CommandLineParser& cmdParser)
{
    mglData _v;
    vector<double> vPulseProperties;
    double dXmin = NAN, dXmax = NAN;
    double dSampleSize = NAN;

    // Indices lesen
    DataAccessParser accessParser = cmdParser.getExprAsDataObject();
    Indices& _idx = accessParser.getIndices();
    std::string sDataset = accessParser.getDataObject();

    MemoryManager _cache;
    getData(sDataset, _idx, NumeReKernel::getInstance()->getMemoryManager(), _cache);

    long long int nLines = _cache.getLines("table", false);

    dXmin = _cache.min("table", 0, nLines - 1, 0);
    dXmax = _cache.max("table", 0, nLines - 1, 0);

    _v.Create(nLines);

    for (long long int i = 0; i < nLines; i++)
        _v.a[i] = _cache.getElement(i, 1, "table");

    dSampleSize = (dXmax - dXmin) / ((double)_v.GetNx() - 1.0);
    mglData _pulse(_v.Pulse('x'));

    if (_pulse.nx >= 5)
    {
        vPulseProperties.push_back(_pulse[0]); // max Amp
        vPulseProperties.push_back(_pulse[1]*dSampleSize + dXmin); // pos max Amp
        vPulseProperties.push_back(2.0 * _pulse[2]*dSampleSize); // FWHM
        vPulseProperties.push_back(2.0 * _pulse[3]*dSampleSize); // Width near max
        vPulseProperties.push_back(_pulse[4]*dSampleSize); // Energy (Integral pulse^2)
    }
    else
    {
        cmdParser.setReturnValue("nan");
        return true;
    }

    // Ausgabe
    if (NumeReKernel::getInstance()->getSettings().systemPrints())
    {
        NumeReKernel::toggleTableStatus();
        make_hline();
        NumeReKernel::print("NUMERE: " + toUpperCase(_lang.get("PARSERFUNCS_PULSE_HEADLINE")));
        make_hline();

        for (unsigned int i = 0; i < vPulseProperties.size(); i++)
            NumeReKernel::printPreFmt("|   " + _lang.get("PARSERFUNCS_PULSE_TABLE_" + toString((int)i + 1) + "_*", toString(vPulseProperties[i], NumeReKernel::getInstance()->getSettings())) + "\n");

        NumeReKernel::toggleTableStatus();
        make_hline();
    }

    cmdParser.setReturnValue(vPulseProperties);
    return true;
}


/////////////////////////////////////////////////
/// \brief This function performs the short-time
/// fourier analysis on the passed data set.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool shortTimeFourierAnalysis(CommandLineParser& cmdParser)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    Indices _target;
    mglData _real, _imag, _result;
    int nSamples = 0;

    double dXmin = NAN, dXmax = NAN;
    double dFmin = 0.0, dFmax = 1.0;
    double dSampleSize = NAN;

    auto vParVal = cmdParser.getParameterValueAsNumericalValue("samples");

    if (vParVal.size())
        nSamples = std::max(intCast(vParVal.front()), 0LL);

    std::string sTargetCache = cmdParser.getTargetTable(_target, "stfdat");

    // Indices lesen
    DataAccessParser _accessParser = cmdParser.getExprAsDataObject();
    _accessParser.evalIndices();
    Indices& _idx = _accessParser.getIndices();

    dXmin = _data.min(_accessParser.getDataObject(), _idx.row, _idx.col.subidx(0, 1));
    dXmax = _data.max(_accessParser.getDataObject(), _idx.row, _idx.col.subidx(0, 1));

    _real.Create(_idx.row.size());
    _imag.Create(_idx.row.size());

    for (size_t i = 0; i < _idx.row.size(); i++)
    {
        _real.a[i] = _data.getElement(_idx.row[i], _idx.col[1], _accessParser.getDataObject());
    }

    if (!nSamples || nSamples > _real.GetNx())
        nSamples = _real.GetNx() / 32;

    // Tatsaechliche STFA
    _result = mglSTFA(_real, _imag, nSamples);

    dSampleSize = (dXmax - dXmin) / ((double)_result.GetNx() - 1.0);

    // Nyquist: _real.GetNx()/(dXmax-dXmin)/2.0
    dFmax = _real.GetNx() / (dXmax - dXmin) / 2.0;

    // Zielcache befuellen entsprechend der Fourier-Algorithmik

    if (_target.row.isOpenEnd())
        _target.row.setRange(0, _target.row.front() + _result.GetNx() - 1);

    if (_target.col.isOpenEnd())
        _target.col.setRange(0, _target.col.front() + _result.GetNy()/2 + 1);

    _data.resizeTable(std::max(_target.row.max(), _target.col.max()), _target.col.max(), sTargetCache);

    // Write the time axis
    for (int i = 0; i < _result.GetNx(); i++)
        _data.writeToTable(_target.row[i], _target.col.front(), sTargetCache, dXmin + i * dSampleSize);

    // Define headline
    _data.setHeadLineElement(_target.col.front(), sTargetCache, _data.getHeadLineElement(_idx.col.front(), _accessParser.getDataObject()));
    dSampleSize = 2 * (dFmax - dFmin) / ((double)_result.GetNy() - 1.0);

    // Write the frequency axis
    for (int i = 0; i < _result.GetNy() / 2; i++)
        _data.writeToTable(_target.row[i], _target.col[1], sTargetCache, dFmin + i * dSampleSize); // Fourier f

    // Define headline
    _data.setHeadLineElement(_target.col[1], sTargetCache, "f [Hz]");

    // Write the STFA map
    for (int i = 0; i < _result.GetNx(); i++)
    {
        if (_target.row[i] == VectorIndex::INVALID)
            break;

        for (int j = 0; j < _result.GetNy() / 2; j++)
        {
            if (_target.col[j+2] == VectorIndex::INVALID)
                break;

            _data.writeToTable(_target.row[i], _target.col[j+2], sTargetCache, _result[i + (j + _result.GetNy() / 2)*_result.GetNx()]);

            // Update the headline
            if (!i)
                _data.setHeadLineElement(_target.col[j+2], sTargetCache, "A[" + toString((int)j + 1) + "]");
        }
    }

    cmdParser.clearReturnValue();
    cmdParser.setReturnValue(sTargetCache);
    return true;
}


/////////////////////////////////////////////////
/// \brief This static function is a helper for
/// boneDetection to map/interpolate the
/// calculated values to the final grid.
///
/// \param vAxis const std::vector<double>&
/// \param dInterpolVal double
/// \param bExtent bool
/// \return double
///
/////////////////////////////////////////////////
static double interpolateToGrid(const std::vector<double>& vAxis, double dInterpolVal, bool bExtent = false)
{
    if (isnan(dInterpolVal))
        return dInterpolVal;

    // Find the base index
    int nBaseVal = intCast(dInterpolVal) + (dInterpolVal < 0 ? -1 : 0);

    // Get the decimal part
    double x = dInterpolVal - nBaseVal;

    if (nBaseVal >= 0 && nBaseVal+1 < (int)vAxis.size())
        return vAxis[nBaseVal] + (vAxis[nBaseVal+1] - vAxis[nBaseVal]) * x;
    else if (nBaseVal > 0 && nBaseVal < (int)vAxis.size()) // Repeat last distance
        return vAxis[nBaseVal] + (vAxis[nBaseVal] - vAxis[nBaseVal-1]) * x;
    else if (nBaseVal == -1 && nBaseVal+1 < (int)vAxis.size()) // Repeat first distance
        return vAxis.front() + (-1 + x) * (vAxis[1] - vAxis.front());

    if (bExtent)
    {
        if (nBaseVal >= (int)vAxis.size())
            return vAxis.back() + (nBaseVal - vAxis.size() + 1 + x) * (vAxis.back() - vAxis[vAxis.size()-2]);
        else if (nBaseVal < -1)
            return vAxis.front() + (nBaseVal + x) * (vAxis[1] - vAxis.front());
    }

    // nBaseVal below zero or not in the grid
    return NAN;
}


/////////////////////////////////////////////////
/// \brief This function is the implementation of
/// the detect command.
///
/// \param cmdParser CommandLineParser&
/// \return void
///
/////////////////////////////////////////////////
void boneDetection(CommandLineParser& cmdParser)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    mglData _mData;
    double dLevel = NAN, dAttrX = 0.0, dAttrY = 1.0, dMinLen = 0.0;

    // detect TABLE() -set minval=LVL attract={x,y} minlen=MIN target=TARGET()
    auto vParVal = cmdParser.getParameterValueAsNumericalValue("minval");

    if (vParVal.size())
        dLevel = vParVal.front();

    vParVal = cmdParser.getParameterValueAsNumericalValue("attract");

    if (vParVal.size() > 1)
    {
        dAttrX = fabs(vParVal[0]);
        dAttrY = fabs(vParVal[1]);
    }
    else if (vParVal.size())
        dAttrY = fabs(vParVal[0]);

    vParVal = cmdParser.getParameterValueAsNumericalValue("minlen");

    if (vParVal.size())
        dMinLen = fabs(vParVal.front());

    Indices _target;
    std::string sTargetCache = cmdParser.getTargetTable(_target, "detectdat");

    // Indices lesen
    DataAccessParser accessParser = cmdParser.getExprAsDataObject();
    Indices& _idx = accessParser.getIndices();

    if (_idx.row.isOpenEnd())
        _idx.row.setRange(0, _data.getLines(accessParser.getDataObject())-1);

    if (_idx.col.isOpenEnd())
        _idx.col.setRange(0, _idx.col.front() + _data.getLines(accessParser.getDataObject(), true) - _data.getAppendedZeroes(_idx.col[1], accessParser.getDataObject()) + 1);

    // Get x and y axis for the final scaling
    std::vector<double> vX = _data.getElement(_idx.row, _idx.col.subidx(0, 1), accessParser.getDataObject());
    std::vector<double> vY = _data.getElement(_idx.row, _idx.col.subidx(1, 1), accessParser.getDataObject());

    _idx.col = _idx.col.subidx(2);

    // Get the data
    MemoryManager _cache;
    getData(accessParser.getDataObject(), _idx, _data, _cache, 100, false);

    long long int nLines = _cache.getLines("table", false);
    long long int nCols = _cache.getCols("table", false);

    // Restrict the attraction to not exceed the axis range
    dAttrX = min(dAttrX, (double)nLines);
    dAttrY = min(dAttrY, (double)nCols);

    // Use minimal data value if level is NaN
    if (isnan(dLevel))
        dLevel = _cache.min("table", "all").front();

    _mData.Create(nLines, nCols);

    // Copy the data to the mglData object
    for (long long int i = 0; i < nLines; i++)
    {
        for (long long int j = 0; j < nCols; j++)
        {
            _mData.a[i+j*nLines] = _cache.getElement(i, j, "table");
        }
    }

    // Perform the actual bone detection
    mglData _res = _mData.Detect(dLevel, dAttrY, dAttrX, dMinLen);

    if (_target.row.isOpenEnd())
        _target.row.setOpenEndIndex(_res.GetNy() + _target.row.front());

    if (_target.col.isOpenEnd())
        _target.col.setOpenEndIndex(_res.GetNx() + _target.col.front());

    // Copy the results to the target table
    for (int i = 0; i < _res.GetNy(); i++)
    {
        if (_target.row.size() <= i)
            break;

        for (int j = 0; j < _res.GetNx(); j++)
        {
            if (_target.col.size() <= j)
                break;

            if (!j)
                _data.writeToTable(_target.row[i], _target.col[j], sTargetCache, interpolateToGrid(vX, _res.a[j+i*_res.GetNx()]));
            else
                _data.writeToTable(_target.row[i], _target.col[j], sTargetCache, interpolateToGrid(vY, _res.a[j+i*_res.GetNx()]));
        }
    }

    // Write axis labels
    _data.setHeadLineElement(_target.col[0], sTargetCache, "Structure_x");
    _data.setHeadLineElement(_target.col[1], sTargetCache, "Structure_y");

    if (NumeReKernel::getInstance()->getSettings().systemPrints())
        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_DETECT_SUCCESS", sTargetCache));
}


/////////////////////////////////////////////////
/// \brief This function approximates the passed
/// data set using cubic splines.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/// The calculated spline polynomials are defined
/// as new custom functions.
/////////////////////////////////////////////////
bool calculateSplines(CommandLineParser& cmdParser)
{
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    MemoryManager _cache;
    tk::spline _spline;
    vector<double> xVect, yVect;

    DataAccessParser accessParser = cmdParser.getExprAsDataObject();

    getData(accessParser.getDataObject(), accessParser.getIndices(), NumeReKernel::getInstance()->getMemoryManager(), _cache);

    long long int nLines = _cache.getLines("table", true) - _cache.getAppendedZeroes(0, "table");

    if (nLines < 2)
        throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, cmdParser.getCommandLine(), accessParser.getDataObject());

    for (long long int i = 0; i < nLines; i++)
    {
        xVect.push_back(_cache.getElement(i, 0, "table"));
        yVect.push_back(_cache.getElement(i, 1, "table"));
    }

    // Set the points for the spline to calculate
    _spline.set_points(xVect, yVect);

    string sDefinition = "Spline(x) := ";

    // Create the polynomial, which will be defined for future use
    for (size_t i = 0; i < xVect.size() - 1; i++)
    {
        string sRange = "polynomial(";

        if (xVect[i] == 0)
            sRange += "x,";
        else if (xVect[i] < 0)
            sRange += "x+" + toString(fabs(xVect[i]), 4) + ",";
        else
            sRange += "x-" + toString(xVect[i], 4) + ",";

        vector<double> vCoeffs = _spline[i];
        sRange += toString(vCoeffs[0], 4) + "," + toString(vCoeffs[1], 4) + "," + toString(vCoeffs[2], 4) + "," + toString(vCoeffs[3], 4) + ")";

        if (i == xVect.size() - 2)
            sRange += "*ivl(x," + toString(xVect[i], 4) + "," + toString(xVect[i + 1], 4) + ",1,1)";
        else
            sRange += "*ivl(x," + toString(xVect[i], 4) + "," + toString(xVect[i + 1], 4) + ",1,2)";

        sDefinition += sRange;

        if (i < xVect.size() - 2)
            sDefinition += " + ";
    }

    if (NumeReKernel::getInstance()->getSettings().systemPrints() && !NumeReKernel::bSupressAnswer)
        NumeReKernel::print(sDefinition);

    bool bDefinitionSuccess = false;

    if (_functions.isDefined(sDefinition.substr(0, sDefinition.find(":="))))
        bDefinitionSuccess = _functions.defineFunc(sDefinition, true);
    else
        bDefinitionSuccess = _functions.defineFunc(sDefinition);

    if (bDefinitionSuccess)
        NumeReKernel::print(_lang.get("DEFINE_SUCCESS"), NumeReKernel::getInstance()->getSettings().systemPrints() && !NumeReKernel::bSupressAnswer);
    else
        NumeReKernel::issueWarning(_lang.get("DEFINE_FAILURE"));

    return true;
}


/////////////////////////////////////////////////
/// \brief This is a static helper function to
/// round a value to a single decimal.
///
/// \param d double
/// \return double
///
/////////////////////////////////////////////////
static double rotRound(double d)
{
    return rint(d*10.0) / 10.0;
}


/////////////////////////////////////////////////
/// \brief This function rotates a table, an
/// image or a datagrid around a specified angle.
///
/// \param cmdParser CommandLineParser&
/// \return void
///
/////////////////////////////////////////////////
void rotateTable(CommandLineParser& cmdParser)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    DataAccessParser _accessParser = cmdParser.getExprAsDataObject();

    Indices _idx;
    double dAlpha = 0.0; // radians

    std::vector<double> source_x;
    std::vector<double> source_y;

    // Find the target of this operation
    std::string sTargetTable = cmdParser.getTargetTable(_idx, "rotdata");

    auto vParVal = cmdParser.getParameterValueAsNumericalValue("alpha");

    if (vParVal.size())
        dAlpha = -vParVal.front() / 180.0 * M_PI; // deg2rad and change orientation for mathematical positive rotation

    _accessParser.getIndices().row.setOpenEndIndex(_data.getLines(_accessParser.getDataObject())-1);
    _accessParser.getIndices().col.setOpenEndIndex(_data.getCols(_accessParser.getDataObject())-1);

    // Handle the image and datagrid cases
    if (cmdParser.getCommand() == "imrot" || cmdParser.getCommand() == "gridrot")
    {
        if (cmdParser.getCommand() == "gridrot")
        {
            source_x = _data.getElement(_accessParser.getIndices().row, _accessParser.getIndices().col.subidx(0, 1), _accessParser.getDataObject());
            source_y = _data.getElement(_accessParser.getIndices().row, _accessParser.getIndices().col.subidx(1, 1), _accessParser.getDataObject());

            // Remove trailing NANs
            while (isnan(source_x.back()))
                source_x.pop_back();

            while (isnan(source_y.back()))
                source_y.pop_back();

            // Determine the interval range of both axes
            // and calculate the relations between size and
            // interval ranges
            auto source_x_minmax = std::minmax_element(source_x.begin(), source_x.end());
            auto source_y_minmax = std::minmax_element(source_y.begin(), source_y.end());
            double dSizeRelation = source_x.size() / (double)source_y.size();
            double dRangeRelation = fabs(*source_x_minmax.second - *source_x_minmax.first) / fabs(*source_y_minmax.second - *source_y_minmax.first);

            // Warn, if the relations are too different
            if (fabs(dSizeRelation - dRangeRelation) > 1e-3)
                NumeReKernel::issueWarning(_lang.get("BUILTIN_CHECKKEYWORD_ROTATETABLE_WARN_AXES_NOT_PRESERVED"));
        }

        _accessParser.getIndices().col = _accessParser.getIndices().col.subidx(2);
    }

    // Extract the range (will pass the ownership)
    Memory* _source = _data.getTable(_accessParser.getDataObject())->extractRange(_accessParser.getIndices().row, _accessParser.getIndices().col);

    // Get the edges
    Point topLeft(0, 0);
    Point topRight(_source->getCols(false), 0);
    Point bottomLeft(0, _source->getLines(false));
    Point bottomRight(_source->getCols(false), _source->getLines(false));

    // get the rotation origin
    Point origin = (bottomRight + topLeft) / 2.0;

    // Calculate their final positions
    topLeft.rotate(dAlpha, origin);
    topRight.rotate(dAlpha, origin);
    bottomLeft.rotate(dAlpha, origin);
    bottomRight.rotate(dAlpha, origin);

    // Calculate the final image extent
    double top   = std::min(topLeft.y, std::min(topRight.y, std::min(bottomLeft.y, bottomRight.y)));
    double bot   = std::max(topLeft.y, std::max(topRight.y, std::max(bottomLeft.y, bottomRight.y)));
    double left  = std::min(topLeft.x, std::min(topRight.x, std::min(bottomLeft.x, bottomRight.x)));
    double right = std::max(topLeft.x, std::max(topRight.x, std::max(bottomLeft.x, bottomRight.x)));

    int rows = ceil(rotRound(bot) - rotRound(top)) + 1;
    int cols = ceil(rotRound(right) - rotRound(left)) + 1;

    // Compensate for ceil()
    top -= (ceil(rotRound(bot) - rotRound(top)) - (bot - top)) / 2.0;
    left -= (ceil(rotRound(right) - rotRound(left))- (right - left)) / 2.0;

    // Insert the axes, if necessary
    if (cmdParser.getCommand() == "imrot")
    {
        // Write the x axis
        for (int i = 0; i < rows; i++)
        {
            if (_idx.row.size() <= i)
                break;

            _data.writeToTable(_idx.row[i], _idx.col[0], sTargetTable, i+1);
        }

        // Write the y axis
        for (int i = 0; i < cols; i++)
        {
            if (_idx.row.size() <= i)
                break;

            _data.writeToTable(_idx.row[i], _idx.col[1], sTargetTable, i+1);
        }

        _idx.col = _idx.col.subidx(2);
    }
    else if (cmdParser.getCommand() == "gridrot")
    {
        Point origin_axis(interpolateToGrid(source_x, origin.x, true), interpolateToGrid(source_y, origin.y, true));

        // Rotate "cell" coordinates to old coord sys,
        // interpolate values and rotate back to obtain
        // the values of the new coordinates
        for (int i = 0; i < rows; i++)
        {
            if (_idx.row.size() <= i)
                break;

            Point p(i+top, origin.y);
            p.rotate(-dAlpha, origin);

            p.x = interpolateToGrid(source_x, p.x, true);
            p.y = interpolateToGrid(source_y, p.y, true);

            p.rotate(dAlpha, origin_axis);

            _data.writeToTable(_idx.row[i], _idx.col[0], sTargetTable, p.x);
        }

        for (int j = 0; j < cols; j++)
        {
            if (_idx.row.size() <= j)
                break;

            Point p(origin.x, j+left);
            p.rotate(-dAlpha, origin);

            p.x = interpolateToGrid(source_x, p.x, true);
            p.y = interpolateToGrid(source_y, p.y, true);

            p.rotate(dAlpha, origin_axis);

            _data.writeToTable(_idx.row[j], _idx.col[1], sTargetTable, p.y);
        }

        _idx.col = _idx.col.subidx(2);
    }

    // Calculate the rotated grid
    for (int i = 0; i < rows; i++)
    {
        if (_idx.row.size() <= i)
            break;

        for (int j = 0; j < cols; j++)
        {
            if (_idx.col.size() <= j)
                break;

            // Create a point in rotated source coordinates
            // and rotate it backwards
            Point p(j + left, i + top);
            p.rotate(-dAlpha, origin);

            // Store the interpolated value in target coordinates
            _data.writeToTable(_idx.row[i], _idx.col[j], sTargetTable, _source->readMemInterpolated(p.y, p.x));
        }
    }

    // clear the memory instance
    delete _source;

    if (NumeReKernel::getInstance()->getSettings().systemPrints())
        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_ROTATETABLE_SUCCESS", sTargetTable));
}


// Forward declaration to make this function usable by the
// particle swarm optimizer (needs randomness)
value_type parser_Random(value_type vRandMin, value_type vRandMax);


/////////////////////////////////////////////////
/// \brief This function implements a particle
/// swarm optimizer in up to four dimensions
/// (depending on the number of intervals
/// defined). The optimizer has an adaptive
/// velocity part, reducing the overall position
/// variation of the particles over time.
///
/// \param cmdParser CommandLineParser&
/// \return void
///
/////////////////////////////////////////////////
void particleSwarmOptimizer(CommandLineParser& cmdParser)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();

    size_t nGlobalBest = 0;
    size_t nNumParticles = 100;
    size_t nMaxIterations = 100;
    size_t nDims = 1;

    // Extract the interval information
    IntervalSet ivl = cmdParser.parseIntervals();

    // Handle parameters
    std::vector<double> vParVal = cmdParser.getParameterValueAsNumericalValue("particles");

    if (vParVal.size())
        nNumParticles = intCast(vParVal.front());

    vParVal = cmdParser.getParameterValueAsNumericalValue("iter");

    if (vParVal.size())
        nMaxIterations = intCast(vParVal.front());

    // Set the expression
    _parser.SetExpr(cmdParser.getExprAsMathExpression(true));

    // Determine intervals and dimensionality
    if (!ivl.size())
        ivl.intervals.push_back(Interval(-10.0, 10.0));

    nDims = ivl.size();

    // Restrict to 4 dimensions, because there are
    // only 4 default variables
    nDims = std::min(4u, nDims);

    // Determine the random range for the velocity vector
    double minRange = fabs(ivl[0].max() - ivl[0].min());

    for (size_t i = 1; i < nDims; i++)
    {
        if (fabs(ivl[i].max() - ivl[i].min()) < minRange)
            minRange = fabs(ivl[i].max() - ivl[i].min());
    }

    // The random range is a 10th of the smallest interval
    // range to avoid too large jumps in the particle velocity
    // compared to the interval range.
    double fRandRange = minRange / 10.0;

    std::vector<std::vector<mu::value_type>> vPos;
    std::vector<std::vector<mu::value_type>> vBest;
    std::vector<std::vector<mu::value_type>> vVel;
    std::vector<mu::value_type> vFunctionValues;
    std::vector<mu::value_type> vBestValues;

    vPos.resize(nDims);
    vBest.resize(nDims);
    vVel.resize(nDims);

    // Prepare initial vectors
    for (size_t i = 0; i < nNumParticles; i++)
    {
        for (size_t j = 0; j < nDims; j++)
        {
            vPos[j].push_back(parser_Random(ivl[j].min(), ivl[j].max()));
            vVel[j].push_back(parser_Random(ivl[j].min(), ivl[j].max())/5.0);

            _defVars.vValue[j][0] = vPos[j].back();
        }

        vFunctionValues.push_back(_parser.Eval());
    }

    for (size_t j = 0; j < nDims; j++)
    {
        vBest[j] = vPos[j];
    }

    vBestValues = vFunctionValues;

    // Find global best
    nGlobalBest = std::min_element(vBestValues.begin(), vBestValues.end()) - vBestValues.begin();

    // Iterate
    for (size_t i = 0; i < nMaxIterations; i++)
    {
        // Create an adaptive factor to reduce
        // particle position variation over time
        double fAdaptiveVelFactor = (nMaxIterations - i) / (double)nMaxIterations;

        for (size_t j = 0; j < nNumParticles; j++)
        {
            for (size_t n = 0; n < nDims; n++)
            {
                // Update velocities
                vVel[n][j] += parser_Random(0, fRandRange) * (vBest[n][j] - vPos[n][j]) + parser_Random(0, fRandRange) * (vBest[n][nGlobalBest] - vPos[n][j]);

                // Update positions
                vPos[n][j] += fAdaptiveVelFactor * vVel[n][j];

                // Restrict to interval boundaries
                vPos[n][j] = std::max(ivl[n].min(), std::min(vPos[n][j], ivl[n].max()));

                // Update the corresponding default variable
                _defVars.vValue[n][0] = vPos[n][j];
            }

            // Recalculate the function value
            vFunctionValues[j] = _parser.Eval();

            // Update the best positions
            if (vFunctionValues[j] < vBestValues[j])
            {
                vBestValues[j] = vFunctionValues[j];

                for (size_t n = 0; n < nDims; n++)
                {
                    vBest[n][j] = vPos[n][j];
                }
            }
        }

        // Update best global position
        nGlobalBest = std::min_element(vBestValues.begin(), vBestValues.end()) - vBestValues.begin();
    }

    // Create return value
    std::vector<double> vRes;

    for (size_t j = 0; j < nDims; j++)
    {
        vRes.push_back(vBest[j][nGlobalBest]);
    }

    // Create a temporary vector variable
    cmdParser.setReturnValue(vRes);
}


// Forward declaration for urlExecute
std::string removeQuotationMarks(const std::string&);


/////////////////////////////////////////////////
/// \brief This function implements the url
/// command providing an interface to http(s) and
/// (s)ftp URLs.
///
/// \param cmdParser CommandLineParser&
/// \return void
///
/////////////////////////////////////////////////
void urlExecute(CommandLineParser& cmdParser)
{
    // Try to get the contents of the desired URL
    try
    {
        // Get URL, username and password
        std::string sUrl = cmdParser.parseExprAsString();
        std::string sUserName = cmdParser.getParameterValueAsString("usr", "", true);
        std::string sPassword = cmdParser.getParameterValueAsString("pwd", "", true);

        // Push the response into a file, if necessary
        if (cmdParser.hasParam("file"))
        {
            std::string sFileName = sUrl.substr(sUrl.rfind("/"));

            if (sFileName.find('.') == std::string::npos)
                sFileName = "index.html";

            if (cmdParser.hasParam("up"))
            {
                // Get the file parameter value
                sFileName = cmdParser.getFileParameterValue(sFileName.substr(sFileName.rfind('.')), "<savepath>", sFileName);

                // Upload the file
                size_t bytes = url::put(sUrl, sFileName, sUserName, sPassword);
                cmdParser.setReturnValue(toString(bytes));
            }
            else
            {
                // Get the response from the server
                std::string sUrlResponse = url::get(sUrl, sUserName, sPassword);

                // Get the file parameter value
                sFileName = cmdParser.getFileParameterValueForSaving(sFileName.substr(sFileName.rfind('.')), "<savepath>", sFileName);

                // Open the file binary and clean it
                std::ofstream file(sFileName, std::ios_base::trunc | std::ios_base::binary);

                // Stream the response to the file
                if (file.good())
                {
                    file << sUrlResponse;
                    cmdParser.setReturnValue(toString(sUrlResponse.length()));
                }
                else
                    throw SyntaxError(SyntaxError::CANNOT_OPEN_TARGET, cmdParser.getCommandLine(), sFileName, sFileName);
            }
        }
        else
        {
            // Get the response from the server
            std::string sUrlResponse = url::get(sUrl, sUserName, sPassword);

            // Replace all masked characters in the return value
            replaceAll(sUrlResponse, "\r\n", "\n");
            replaceAll(sUrlResponse, "\\", "\\ ");
            replaceAll(sUrlResponse, "\"", "\\\"");
            cmdParser.setReturnValue("\"" + sUrlResponse + "\"");
        }
    }
    catch (url::Error& e)
    {
        throw SyntaxError(SyntaxError::URL_ERROR, cmdParser.getCommandLine(), cmdParser.parseExprAsString(), e.what());
    }
}



