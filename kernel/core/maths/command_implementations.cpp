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
#include <regex>

#include "command_implementations.hpp"
#include "parser_functions.hpp"
#include "matrixoperations.hpp"
#include "spline.h"
#include "wavelet.hpp"
#include "filtering.hpp"
#include "../AudioLib/audiofile.hpp"
#include "../../kernel.hpp"
#include "../../../network/http.h"

#define TRAPEZOIDAL 1
#define SIMPSON 2

using namespace std;

DefaultVariables _defVars;


/////////////////////////////////////////////////
/// \brief This static function performs an
/// integration step using a trapezoidal
/// approximation algorithm.
///
/// \param x mu::Variable&
/// \param x0 const mu::Value&
/// \param dx const mu::Value&
/// \param vResult mu::Array&
/// \param vFunctionValues mu::Array&
/// \param bReturnFunctionPoints bool
/// \return void
///
/////////////////////////////////////////////////
static void integrationstep_trapezoidal(mu::Variable& x, const mu::Value& x0, const mu::Value& dx, mu::Array& vResult, mu::Array& vFunctionValues, bool bReturnFunctionPoints)
{
    x = x0;
    mu::Array v = NumeReKernel::getInstance()->getParser().Eval();

    // Evaluate the current integration step for each of the
    // defined functions
    for (size_t i = 0; i < v.size(); i++)
    {
        if (mu::isnan(v[i]))
            v[i] = mu::Value(0.0);
    }

    // Now calculate the area below the curve
    if (!bReturnFunctionPoints)
        vResult += dx * (vFunctionValues + v) * 0.5;
    else
    {
        // Calculate the integral
        if (vResult.size())
            vResult.push_back(dx * (vFunctionValues[0] + v[0]) * mu::Value(0.5) + vResult.back());
        else
            vResult.push_back(dx * (vFunctionValues[0] + v[0]) * mu::Value(0.5));
    }

    // Set the last sample as the first one
    vFunctionValues = v;
}


/////////////////////////////////////////////////
/// \brief This static function performs an
/// integration step using the Simpson
/// approximation algorithm.
///
/// \param x mu::Variable&
/// \param x0 const mu::Value&
/// \param x1 const mu::Value&
/// \param dx const mu::Value&
/// \param vResult mu::Array&
/// \param vFunctionValues mu::Array&
/// \param bReturnFunctionPoints bool
/// \return void
///
/////////////////////////////////////////////////
static void integrationstep_simpson(mu::Variable& x, const mu::Value& x0, const mu::Value& x1, const mu::Value& dx, mu::Array& vResult, mu::Array& vFunctionValues, bool bReturnFunctionPoints)
{
    // Evaluate the intermediate function value
    x = x0;

    mu::Array v = NumeReKernel::getInstance()->getParser().Eval();

    for (size_t i = 0; i < v.size(); i++)
    {
        if (mu::isnan(v[i]))
            v[i] = mu::Value(0.0);
    }

    mu::Array vInter = v;

    // Evaluate the end function value
    x = x1;
    v = NumeReKernel::getInstance()->getParser().Eval();

    for (size_t i = 0; i < v.size(); i++)
    {
        if (mu::isnan(v[i]))
            v[i] = mu::Value(0.0);
    }

    // Now calculate the area below the curve
    if (!bReturnFunctionPoints)
        vResult += dx / mu::Value(6.0) * (vFunctionValues + mu::Value(4.0) * vInter + v); // b-a/6*(f(a)+4f(a+b/2)+f(b))
    else
    {
        // Calculate the integral at the current x position
        if (vResult.size())
            vResult.push_back(dx / mu::Value(6.0) * (vFunctionValues[0] + mu::Value(4.0) * vInter[0] + v[0]) + vResult.back());
        else
            vResult.push_back(dx / mu::Value(6.0) * (vFunctionValues[0] + mu::Value(4.0) * vInter[0] + v[0]));
    }

    // Set the last sample as the first one
    vFunctionValues = v;
}


/////////////////////////////////////////////////
/// \brief This static function integrates single
/// dimension data.
///
/// \param cmdParser& ComandLineParser
/// \return mu::Array
///
/////////////////////////////////////////////////
static mu::Array integrateSingleDimensionData(CommandLineParser& cmdParser)
{
    bool bReturnFunctionPoints = cmdParser.hasParam("points");
    bool bCalcXvals = cmdParser.hasParam("xvals");

    mu::Array vResult;

    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    // Extract the integration interval
    IntervalSet ivl = cmdParser.parseIntervals();

    // Get table name and the corresponding indices
    DataAccessParser accessParser = cmdParser.getExprAsDataObject();
    accessParser.evalIndices();
    Indices& _idx = accessParser.getIndices();
    std::string sDatatable = accessParser.getDataObject();

    if (!_data.isValueLike(accessParser.getIndices().col, accessParser.getDataObject()))
        throw SyntaxError(SyntaxError::WRONG_COLUMN_TYPE, cmdParser.getCommandLine(), accessParser.getDataObject()+"(", accessParser.getDataObject());

    // The indices are vectors
    //
    // If it is a single column or row, then we simply
    // summarize its contents, otherwise we calculate the
    // integral with the trapezoidal method
    if ((_idx.row.size() == 1 || _idx.col.size() == 1) && !bReturnFunctionPoints)
        vResult.push_back(mu::Value(_data.sum(sDatatable, _idx.row, _idx.col)));
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
        mu::Value dResult;
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

            if (ivl.intervals.size() >= 1 && ivl.intervals[0].front().real() > _cache.getElement(i, 0, "table").getNum().asF64())
                continue;

            if (ivl.intervals.size() >= 1 && ivl.intervals[0].back().real() < _cache.getElement(i + j, 0, "table").getNum().asF64())
                break;

            // Calculate either the integral, its samples or the corresponding x values
            if (!bReturnFunctionPoints && !bCalcXvals)
                dResult += (_cache.getElement(i, 1, "table") + _cache.getElement(i + j, 1, "table")) / mu::Value(2.0) * (_cache.getElement(i + j, 0, "table") - _cache.getElement(i, 0, "table"));
            else if (bReturnFunctionPoints && !bCalcXvals)
            {
                if (vResult.size())
                    vResult.push_back((_cache.getElement(i, 1, "table") + _cache.getElement(i + j, 1, "table")) / mu::Value(2.0) * (_cache.getElement(i + j, 0, "table") - _cache.getElement(i, 0, "table")) + vResult.back());
                else
                    vResult.push_back((_cache.getElement(i, 1, "table") + _cache.getElement(i + j, 1, "table")) / mu::Value(2.0) * (_cache.getElement(i + j, 0, "table") - _cache.getElement(i, 0, "table")));
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
    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    const Settings& _option = NumeReKernel::getInstance()->getSettings();
    string sIntegrationExpression = cmdParser.getExprAsMathExpression();
    int nResults = 0;
    mu::Array vResult(mu::Value(0.0));   // Ausgabe-Wert
    mu::Array vFunctionValues(mu::Value(0.0)); // Werte an der Stelle n und n+1
    bool bLargeInterval = false;    // Boolean: TRUE, wenn ueber ein grosses Intervall integriert werden soll
    bool bReturnFunctionPoints = cmdParser.hasParam("points");
    bool bCalcXvals = cmdParser.hasParam("xvals");
    unsigned int nMethod = TRAPEZOIDAL;    // 1 = trapezoidal, 2 = simpson
    size_t nSamples = 1e3;

    mu::Variable& x = _defVars.vValue[0][0];
    double range;

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

        range = ivl[0].range();
    }
    else
        throw SyntaxError(SyntaxError::NO_INTEGRATION_RANGES, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    mu::Array vParVal = cmdParser.getParsedParameterValue("precision");

    if (vParVal.size())
        nSamples = std::rint(range / vParVal.front().getNum().asF64());
    else
    {
        vParVal = cmdParser.getParsedParameterValue("p");

        if (vParVal.size())
            nSamples = std::rint(range / vParVal.front().getNum().asF64());
        else
        {
            vParVal = cmdParser.getParsedParameterValue("eps");

            if (vParVal.size())
                nSamples = std::rint(range / vParVal.front().getNum().asF64());
        }
    }

    vParVal = cmdParser.getParsedParameterValue("steps");

    if (vParVal.size())
        nSamples = std::abs(vParVal.getAsScalarInt());
    else
    {
        vParVal = cmdParser.getParsedParameterValue("s");

        if (vParVal.size())
            nSamples = std::abs(vParVal.getAsScalarInt());
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

    // Ensure that we have only a single expression
    if (nResults > 1)
    {
        sIntegrationExpression = "{" + sIntegrationExpression + "}";
        _parser.SetExpr(sIntegrationExpression);
    }

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
            vResult.push_back(mu::Value(ivl[0](i, nSamples)));
        }

        cmdParser.setReturnValue(vResult);
        return true;
    }

    // Is it a large interval (then it will need more time)
    if ((nMethod == TRAPEZOIDAL && nSamples >= 9.9e6) || (nMethod == SIMPSON && nSamples >= 1e4))
        bLargeInterval = true;

    // Do not allow a very high number of integration steps
    if (nSamples > 1e10)
        throw SyntaxError(SyntaxError::INVALID_INTEGRATION_PRECISION, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Set the integration variable to the lower border
    x.overwrite(mu::Value(ivl[0](0)));

    // Calculate the first sample(s)
    vFunctionValues = _parser.Eval();

    mu::Value dx(range / (nSamples-1));

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
    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();
    const Settings& _option = NumeReKernel::getInstance()->getSettings();
    string sIntegrationExpression = cmdParser.getExprAsMathExpression();                // string fuer die zu integrierende Funktion
    int nResults = 0;
    mu::Array vResult[3];      // value_type-Array, wobei vResult[0] das eigentliche Ergebnis speichert
    // und vResult[1] fuer die Zwischenergebnisse des inneren Integrals ist
    mu::Array fx_n[2][3];          // value_type-Array fuer die jeweiligen Stuetzstellen im inneren und aeusseren Integral
    bool bRenewBoundaries = false;      // bool, der speichert, ob die Integralgrenzen von x oder y abhaengen
    bool bLargeArray = false;       // bool, der TRUE fuer viele Datenpunkte ist;
    unsigned int nMethod = TRAPEZOIDAL;       // trapezoidal = 1, simpson = 2
    size_t nSamples = 1e3;

    mu::Variable& x = _defVars.vValue[0][0];
    mu::Variable& y = _defVars.vValue[1][0];

    double range;

    // Ensure that the integration function is available
    if (!sIntegrationExpression.length())
        throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Evaluate the parameters
    IntervalSet ivl = cmdParser.parseIntervals();

    if (ivl.intervals.size() >= 2)
    {
        if (ivl[0].front() == ivl[0].back())
            throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, cmdParser.getCommandLine(), SyntaxError::invalid_position);

        if (isinf(ivl[0].min()) || isnan(ivl[0].min())
            || isinf(ivl[0].max()) || isnan(ivl[0].max())
            || isinf(ivl[1].min()) || isnan(ivl[1].min())
            || isinf(ivl[1].max()) || isnan(ivl[1].max()))
        {
            cmdParser.setReturnValue("nan");
            return false;
        }

        range = std::min(ivl[0].range(), ivl[1].range());
    }
    else
        throw SyntaxError(SyntaxError::NO_INTEGRATION_RANGES, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    mu::Array vParVal = cmdParser.getParsedParameterValue("precision");

    if (vParVal.size())
        nSamples = std::rint(range / vParVal.front().getNum().asF64());
    else
    {
        vParVal = cmdParser.getParsedParameterValue("p");

        if (vParVal.size())
            nSamples = std::rint(range / vParVal.front().getNum().asF64());
        else
        {
            vParVal = cmdParser.getParsedParameterValue("eps");

            if (vParVal.size())
                nSamples = std::rint(range / vParVal.front().getNum().asF64());
        }
    }

    vParVal = cmdParser.getParsedParameterValue("steps");

    if (vParVal.size())
        nSamples = std::abs(vParVal.getAsScalarInt());
    else
    {
        vParVal = cmdParser.getParsedParameterValue("s");

        if (vParVal.size())
            nSamples = std::abs(vParVal.getAsScalarInt());
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

    // Ensure that we have only a single expression
    if (nResults > 1)
    {
        sIntegrationExpression = "{" + sIntegrationExpression + "}";
        _parser.SetExpr(sIntegrationExpression);
    }

    for (int i = 0; i < 3; i++)
    {
        vResult[i] = mu::Value(0.0);
        fx_n[0][i] = mu::Value(0.0);
        fx_n[1][i] = mu::Value(0.0);
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
    x.overwrite(mu::Value(ivl[0](0))); // x = x_0

    // y might depend on the starting value
    if (bRenewBoundaries)
        refreshBoundaries(ivl, sIntegrationExpression);

    y.overwrite(mu::Value(ivl[1](0))); // y = y_0

    mu::Value dx(ivl[0].range() / (nSamples-1));
    mu::Value dy(ivl[1].range() / (nSamples-1));

    // --> Werte mit den Startwerten die erste Stuetzstelle fuer die y-Integration aus <--
    fx_n[1][0] = _parser.Eval();

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
                dy = ivl[1].range() / (nSamples-1);
            }

            // --> Setzen wir "y" auf den Wert, der von der unteren y-Grenze vorgegeben wird <--
            y = ivl[1](0);
            // --> Werten wir sofort die erste y-Stuetzstelle aus <--
            fx_n[1][0] = _parser.Eval();
            vResult[1] = mu::Value(0.0);

            for (size_t j = 1; j < nSamples; j++)
                integrationstep_trapezoidal(y, ivl[1](j, nSamples), dy, vResult[1], fx_n[1][0], false);

            // --> Weise das Ergebnis der y-Integration an die zweite Stuetzstelle der x-Integration zu <--
            for (size_t i = 0; i < vResult[1].size(); i++)
            {
                if (mu::isnan(vResult[1][i]))
                    vResult[1][i] = mu::Value(0.0);
            }

            vResult[0] += dx * (fx_n[0][0] + vResult[1]) * mu::Value(0.5); // Berechne das Trapez zu x
            fx_n[0][0] = vResult[1]; // Weise den Wert der zweiten Stuetzstelle an die erste Stuetzstelle zu
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
                    dy = ivl[1].range() / (nSamples-1);
                }

                // Set y to the first position
                y = ivl[1](0);

                // Calculate the first position
                if (n == 1)
                    fx_n[1][0] = _parser.Eval();

                vResult[n] = mu::Value(0.0);

                for (size_t j = 1; j < nSamples; j++)
                    integrationstep_simpson(y, ivl[1](2*j-1, 2*nSamples-1), ivl[1](2*j, 2*nSamples-1), dy, vResult[n], fx_n[1][0], false);

                // --> Weise das Ergebnis der y-Integration an die zweite Stuetzstelle der x-Integration zu <--
                for (size_t i = 0; i < vResult[n].size(); i++)
                {
                    if (mu::isnan(vResult[n][i]))
                        vResult[n][i] = mu::Value(0.0);
                }
            }

            vResult[0] += dx / mu::Value(6.0) * (fx_n[0][0] + mu::Value(4.0) * vResult[1] + vResult[2]); // Berechne das Trapez zu x
            fx_n[0][0] = vResult[2]; // Weise den Wert der zweiten Stuetzstelle an die erste Stuetzstelle zu
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
    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();
    std::string sExpr = cmdParser.getExprAsMathExpression();
    std::string sVar = "";
    std::string sPos = "";
    double dEps = 0.0;
    mu::Variable* dVar = nullptr;
    int nResults = 0;
    int nSamples = 100;
    size_t order = 1;
    mu::Array vInterval;
    mu::Array vResult;
    mu::Array paramVal;

    // Get the order of the differntiation
    paramVal = cmdParser.getParsedParameterValue("order");

    if (paramVal.size())
    {
        order = paramVal.getAsScalarInt();
        order = std::min(order, (size_t)3u);
    }

    // Numerical expressions and data sets are handled differently
    if (!_data.containsTablesOrClusters(sExpr) && cmdParser.getParameterList().length())
    {
        // Is the "eps" parameter available?
        paramVal = cmdParser.getParsedParameterValue("eps");

        if (paramVal.size())
            dEps = fabs(paramVal.front().getNum().asCF64());

        paramVal = cmdParser.getParsedParameterValue("samples");

        if (paramVal.size())
            nSamples = std::abs(paramVal.getAsScalarInt());

        sVar = cmdParser.getParameterList();

        if (!_functions.call(sVar))
            throw SyntaxError(SyntaxError::FUNCTION_ERROR, cmdParser.getCommandLine(), sVar, sVar);

        // Is a variable interval defined?
        if (sVar.find('=') != std::string::npos
            || (sVar.find('[') != std::string::npos
                && sVar.find(']', sVar.find('[')) != std::string::npos
                && sVar.find(':', sVar.find('[')) != std::string::npos))
        {
            // Remove possible parameter list initializers
            if (sVar.substr(0, 2) == "--")
                sVar = sVar.substr(2);
            else if (sVar.substr(0, 4) == "-set")
                sVar = sVar.substr(4);

            // Extract variable intervals or locations
            if (sVar.find('[') != std::string::npos
                    && sVar.find(']', sVar.find('[')) != std::string::npos
                    && sVar.find(':', sVar.find('[')) != std::string::npos)
            {
                sPos = sVar.substr(sVar.find('[') + 1, getMatchingParenthesis(StringView(sVar, sVar.find('['))) - 1);
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
                    getDataElements(sPos, _parser, _data);

                if (sPos.front() != '{'
                    && sPos.back() != '}'
                    && sPos.find(':') != std::string::npos)
                    sPos.replace(sPos.find(':'), 1, ",");

                _parser.SetExpr("{" + sPos + "}");
                vInterval = _parser.Eval();

                if (mu::isinf(vInterval.front().getNum().asCF64()) || mu::isnan(vInterval.front()))
                {
                    cmdParser.setReturnValue("nan");
                    return true;
                }
            }

            // Set the expression for differentiation
            // and evaluate it
            _parser.SetExpr(sExpr);
            _parser.Eval(nResults);

            if (nResults > 1)
                _parser.SetExpr("{" + sExpr + "}");

            // Get the address of the variable
            dVar = getPointerToVariable(sVar, _parser);
        }

        // Ensure that the address could be found
        if (!dVar)
            throw SyntaxError(SyntaxError::NO_DIFF_VAR, cmdParser.getCommandLine(), SyntaxError::invalid_position);

        // Evaluate the differential at the desired
        // locations
        if (vInterval.size() == 1 || vInterval.size() > 2)
        {
            // single point or a vector
            vResult = _parser.Diff(dVar, vInterval, dEps, order);
        }
        else
        {
            // a range -> use the samples
            for (int i = 0; i < nSamples; i++)
            {
                mu::Value dPos = vInterval[0] + (vInterval[1] - vInterval[0]) / mu::Value(nSamples - 1) * mu::Value(i);
                vResult.push_back(_parser.Diff(dVar, dPos, dEps, order).front());
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
        size_t nFilterSize = 5;
        paramVal = cmdParser.getParsedParameterValue("points");

        if (paramVal.size())
        {
            nFilterSize = paramVal.getAsScalarInt();

            if (!(nFilterSize % 2))
                nFilterSize++;
        }

        NumeRe::SavitzkyGolayDiffFilter diff(nFilterSize, order);

        // Validate the indices
        if (!isValidIndexSet(_idx))
            throw SyntaxError(SyntaxError::INVALID_INDEX, cmdParser.getCommandLine(), SyntaxError::invalid_position,
                              _idx.row.to_string() + ", " + _idx.col.to_string());

        if (_idx.row.isOpenEnd())
            _idx.row.setRange(0, _data.getLines(sTableName, false)-1);

        if (_idx.col.isOpenEnd())
            _idx.col.setRange(0, _idx.col.front()+1);

        if (!_data.isValueLike(accessParser.getIndices().col, accessParser.getDataObject()))
            throw SyntaxError(SyntaxError::WRONG_COLUMN_TYPE, cmdParser.getCommandLine(),
                              accessParser.getDataObject()+"(", accessParser.getDataObject());

        // If shorter than filter's size return an invalid
        // value
        if (_idx.row.size() < nFilterSize)
        {
            vResult.push_back(mu::Value(NAN));
            cmdParser.setReturnValue(vResult);
            return true;
        }

        // Copy the data contents, sort the values
        // and calculate the derivative

        // Vectors as indices
        //
        // Depending on the number of selected columns, we either
        // have to sort the data or we assume that the difference
        // between two values is 1
        if (_idx.col.size() == 1)
        {
            vResult.resize(_idx.row.size());

            // No sorting, difference is 1
            //
            // Jump over NaNs and get the difference of the neighbouring
            // values, which is identical to the derivative in this case
            for (size_t i = nFilterSize/2; i < _idx.row.size() - nFilterSize/2; i++)
            {
                for (int j = 0; j < (int)nFilterSize; j++)
                {
                    if (_data.isValidElement(_idx.row[i + j - nFilterSize/2], _idx.col.front(), sTableName))
                        vResult[i] += mu::Value(diff.apply(j, 0, _data.getElement(_idx.row[i + j - nFilterSize/2],
                                                                                  _idx.col.front(),
                                                                                  sTableName).getNum().asCF64()));
                }
            }

            // Repeat the first and last values
            for (size_t i = 0; i < nFilterSize/2; i++)
            {
                vResult[i] = vResult[nFilterSize/2];
                vResult[vResult.size()-1-i] = vResult[vResult.size()-1-nFilterSize/2];
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
                NumeReKernel::getInstance()->issueWarning(_lang.get("COMMON_SYNTAX_DEPRECATED", cmdParser.getCommandLine()));

                // The x values are approximated to be in the
                // middle of the two samplex
                for (int i = 0; i < _cache.getLines("table", false) - 1; i++)
                {
                    if (_cache.isValidElement(i, 0, "table")
                            && _cache.isValidElement(i + 1, 0, "table")
                            && _cache.isValidElement(i, 1, "table")
                            && _cache.isValidElement(i + 1, 1, "table"))
                        vResult.push_back((_cache.getElement(i + 1, 0, "table") + _cache.getElement(i, 0, "table")) / mu::Value(2.0));
                    else
                        vResult.push_back(mu::Value(NAN));
                }
            }
            else
            {
                vResult.resize(_cache.getLines("table", false));

                // We calculate the derivative of the data
                // by approximating it linearily
                for (int i = nFilterSize/2; i < _cache.getLines("table", false) - (int)nFilterSize/2; i++)
                {
                    std::pair<mu::Value, size_t> avgDiff(0.0, 0);

                    for (int j = 0; j < (int)nFilterSize; j++)
                    {
                        if (_cache.isValidElement(i + j - nFilterSize/2, 0, "table")
                            && _cache.isValidElement(i + j - nFilterSize/2, 1, "table"))
                        {
                            vResult[i] += mu::Value(diff.apply(j, 0, _cache.getElement(i + j - nFilterSize/2, 1, "table").getNum().asCF64()));

                            // Calculate the average difference
                            if (_cache.isValidElement(i + j - nFilterSize/2 - 1, 0, "table"))
                            {
                                avgDiff.first += _cache.getElement(i + j - nFilterSize/2, 0, "table")
                                                - _cache.getElement(i + j - nFilterSize/2 - 1, 0, "table");
                                avgDiff.second++;
                            }
                        }
                    }

                    if (!avgDiff.second)
                        vResult[i] = mu::Value(NAN);
                    else
                        vResult[i] /= (avgDiff.first/mu::Value(avgDiff.second)).pow(order);
                }

                // Repeat the first and last values
                for (size_t i = 0; i < nFilterSize/2; i++)
                {
                    vResult[i] = vResult[nFilterSize/2];
                    vResult[vResult.size()-1-i] = vResult[vResult.size()-1-nFilterSize/2];
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
        sInterval = sParams.substr(sParams.find('[') + 1, getMatchingParenthesis(StringView(sParams, sParams.find('['))) - 1);
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
/// \param nOrder size_t
/// \param nMode int
/// \return bool
///
/////////////////////////////////////////////////
static bool findExtremaInMultiResult(CommandLineParser& cmdParser, string& sExpr, string& sInterval, size_t nOrder, int nMode)
{
    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();
    _parser.SetExpr(sExpr);
    mu::Array v = _parser.Eval();
    mu::Array vResults;
    MemoryManager _cache;
    size_t res = v.size();

    // Store the results in the second column of a table
    for (size_t i = 0; i < v.size(); i++)
        _cache.writeToTable(i, 1, "table", v[i]);

    _parser.SetExpr(sInterval);
    v = _parser.Eval();

    // Write the results for the x interval in the first column
    if (v.size() > 1)
    {
        for (size_t i = 0; i < res; i++)
        {
            if (i >= v.size())
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
    std::vector<double> data(nOrder);
    double nDir = 0;
    size_t nanShift = 0;

    if (nOrder >= res / 3)
        nOrder = res / 3;

    // Ensure that the number of used points is reasonable
    if (nOrder < 3)
    {
        vResults = mu::Value(NAN);
        return false;
    }

    // Find the first median and use it as starting point
    // for identifying the next extremum
    for (size_t i = 0; i + nanShift < (size_t)_cache.getLines("table", true); i++)
    {
        if (i == nOrder)
            break;

        while (mu::isnan(_cache.getElement(i + nanShift, 1, "table"))
               && i + nanShift < (size_t)_cache.getLines("table", true) - 1)
            nanShift++;

        data[i] = _cache.getElement(i + nanShift, 1, "table").getNum().asF64();
    }

    // Sort the data and find the median
    gsl_sort(&data[0], 1, nOrder);
    dExtremum = gsl_stats_median_from_sorted_data(&data[0], 1, nOrder);

    // Go through the data points using sliding median to find the local
    // extrema in the data set
    for (size_t i = nOrder; i + nanShift < _cache.getLines("table", false) - nOrder; i++)
    {
        size_t currNanShift = 0;
        dMedian = 0.0;

        for (size_t j = i; j < i + nOrder; j++)
        {
            while (mu::isnan(_cache.getElement(j + nanShift + currNanShift, 1, "table"))
                   && j + nanShift + currNanShift < (size_t)_cache.getLines("table", true) - 1)
                currNanShift++;

            data[j - i] = _cache.getElement(j + nanShift + currNanShift, 1, "table").getNum().asF64();
        }

        gsl_sort(&data[0], 1, nOrder);
        dMedian = gsl_stats_median_from_sorted_data(&data[0], 1, nOrder);

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
                    size_t nExtremum = i + nanShift;
                    double dExtremum = _cache.getElement(i + nanShift, 1, "table").getNum().asF64();

                    for (size_t k = i + nanShift; k >= 0; k--)
                    {
                        if (k == i - nOrder)
                            break;

                        if (nDir*_cache.getElement(k, 1, "table").getNum().asF64() > nDir*dExtremum)
                        {
                            nExtremum = k;
                            dExtremum = _cache.getElement(k, 1, "table").getNum().asF64();
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
        vResults = mu::Value(NAN);

    cmdParser.setReturnValue(vResults);
    return true;
}


/////////////////////////////////////////////////
/// \brief Static helper function for
/// findExtremaInData
///
/// \param v const mu::Array&
/// \param start size_t
/// \param nOrder size_t
/// \param nanShiftStart size_t
/// \param nNewNanShift size_t&
/// \return double
///
/////////////////////////////////////////////////
static double calculateMedian(const mu::Array& v, size_t start, size_t nOrder, size_t nanShiftStart, size_t& nNewNanShift)
{
    std::vector<double> data;

    for (size_t i = start; i < start + nOrder; i++)
    {
        while (mu::isnan(v[i + nanShiftStart + nNewNanShift])
               && i + nanShiftStart + nNewNanShift < v.size() - 1)
            nNewNanShift++;

        if (i + nanShiftStart + nNewNanShift >= v.size())
            break;

        data.push_back(v[i + nanShiftStart + nNewNanShift].getNum().asF64());
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
/// \param nOrder size_t
/// \param nMode int
/// \return bool
///
/////////////////////////////////////////////////
static bool findExtremaInData(CommandLineParser& cmdParser, string& sExpr, size_t nOrder, int nMode)
{
    mu::Array v;
    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();
    _parser.SetExpr(sExpr);
    v = _parser.Eval();

    if (v.size() > 1)
    {
        if (nOrder >= v.size() / 3)
            nOrder = v.size() / 3;

        double dMedian = 0.0, dExtremum = 0.0;
        double nDir = 0;
        size_t nanShift = 0;
        mu::Array vResults;

        if (nOrder < 3)
        {
            vResults = mu::Value(NAN);
            return false;
        }

        dExtremum = calculateMedian(v, 0, nOrder, 0, nanShift);

        for (size_t i = nOrder; i + nanShift < v.size() - nOrder; i++)
        {
            size_t currNanShift = 0;
            dMedian = calculateMedian(v, i, nOrder, nanShift, currNanShift);

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
                        size_t nExtremum = i + nanShift;
                        double dLocalExtremum = v[i + nanShift].getNum().asF64();

                        for (size_t k = i + nanShift; k >= 0; k--)
                        {
                            if (k == i + nanShift - nOrder)
                                break;

                            if (nDir*v[k].getNum().asF64() > nDir*dLocalExtremum)
                            {
                                nExtremum = k;
                                dLocalExtremum = v[k].getNum().asF64();
                            }
                        }

                        vResults.push_back(mu::Value(nExtremum + 1));
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
            vResults = mu::Value(NAN);

        cmdParser.setReturnValue(vResults);
        return true;
    }
    else
        throw SyntaxError(SyntaxError::NO_EXTREMA_VAR, cmdParser.getCommandLine(), SyntaxError::invalid_position);
}

/////////////////////////////////////////////////
/// \brief This function searches for the
/// positions of all extrema, which are located
/// in the selected interval.
///
/// \param sCmd string&
/// \param dVarAdress mu::Variable*
/// \param _parser mu::Parser&
/// \param _option const Settings&
/// \param dLeft mu::Value
/// \param dRight mu::Value
/// \param dEps double
/// \param nRecursion int
/// \return mu::value_type
///
/// The expression has to be setted in advance.
/// The function performs recursions until the
/// defined precision is reached.
/////////////////////////////////////////////////
static mu::Value localizeExtremum(string& sCmd, mu::Variable* dVarAdress, mu::Parser& _parser, const Settings& _option, mu::Value dLeft, mu::Value dRight, double dEps = 1e-10, int nRecursion = 0)
{
    const size_t nSamples = 101;
    mu::Array dVal(2);

    if (_parser.GetExpr() != sCmd)
    {
        _parser.SetExpr(sCmd);
        _parser.Eval();
    }

    // Calculate the leftmost value
    dVal[0] = _parser.Diff(dVarAdress, dLeft, 1e-7).front();

    // Separate the current interval in
    // nSamples steps and examine each step
    for (size_t i = 1; i < nSamples; i++)
    {
        // Calculate the next value
        dVal[1] = _parser.Diff(dVarAdress, dLeft + mu::Value(i) * (dRight - dLeft) / mu::Value(nSamples - 1), 1e-7).front();

        // Multiply the values to find a sign change
        if (dVal[0]*dVal[1] < mu::Value(0))
        {
            // Sign change
            // return, if precision is reached. Otherwise perform
            // a new recursion between the two values
            if (std::abs(dRight.getNum().asCF64() - dLeft.getNum().asCF64()) / (double)(nSamples - 1) <= dEps
                || fabs(log(dEps)) + 1 < nRecursion * 2)
                return dLeft + mu::Value(i - 1) * (dRight - dLeft) / mu::Value(nSamples - 1)
                    + mu::Value(Linearize(0.0, dVal[0].getNum().asF64(),
                                ((dRight - dLeft) / mu::Value(nSamples - 1)).getNum().asF64(),
                                dVal[1].getNum().asF64()));
            else
                return localizeExtremum(sCmd, dVarAdress, _parser, _option,
                                        dLeft + mu::Value(i - 1) * (dRight - dLeft) / mu::Value(nSamples - 1),
                                        dLeft + mu::Value(i) * (dRight - dLeft) / mu::Value(nSamples - 1), dEps, nRecursion + 1);
        }
        else if (dVal[0]*dVal[1] == mu::Value(0.0))
        {
            // One of the two vwlues is zero.
            // Jump over all following zeros due
            // to constness
            int nTemp = i - 1;

            if (dVal[0] != mu::Value(0.0))
            {
                while (dVal[0]*dVal[1] == mu::Value(0.0) && mu::Value(i + 1 < nSamples))
                {
                    i++;
                    dVal[1] = _parser.Diff(dVarAdress, dLeft + mu::Value(i) * (dRight - dLeft) / mu::Value(nSamples - 1), 1e-7).front();
                }
            }
            else
            {
                while (dVal[1] == mu::Value(0.0) && mu::Value(i + 1 < nSamples))
                {
                    i++;
                    dVal[1] = _parser.Diff(dVarAdress, dLeft + mu::Value(i) * (dRight - dLeft) / mu::Value(nSamples - 1), 1e-7).front();
                }
            }

            // return, if precision is reached. Otherwise perform
            // a new recursion between the two values
            if ((i - nTemp) * std::abs(dRight.getNum().asCF64() - dLeft.getNum().asCF64()) / (double)(nSamples - 1) <= dEps
                || (!nTemp && i + 1 == nSamples) || fabs(log(dEps)) + 1 < nRecursion * 2)
                return dLeft + mu::Value(nTemp) * (dRight - dLeft) / mu::Value(nSamples - 1)
                    + mu::Value(Linearize(0.0, dVal[0].getNum().asF64(),
                                (i - nTemp) * (dRight - dLeft).getNum().asF64() / (double)(nSamples - 1),
                                dVal[1].getNum().asF64()));
            else
                return localizeExtremum(sCmd, dVarAdress, _parser, _option,
                                        dLeft + mu::Value(nTemp) * (dRight - dLeft) / mu::Value(nSamples - 1),
                                        dLeft + mu::Value(i) * (dRight - dLeft) / mu::Value(nSamples - 1), dEps, nRecursion + 1);
        }

        dVal[0] = dVal[1];
    }

    // If no explict sign change was found,
    // interpolate the position by linearisation
    *dVarAdress = dLeft;
    dVal[0] = _parser.Eval().front();
    *dVarAdress = dRight;
    dVal[1] = _parser.Eval().front();
    return Linearize(dLeft.getNum().asF64(), dVal[0].getNum().asF64(),
                     dRight.getNum().asF64(), dVal[1].getNum().asF64());
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
    mu::Parser& _parser = instance->getParser();

    size_t nSamples = 21;
    size_t nOrder = 5;
    mu::Array dVal(2, mu::Value(0.0));
    mu::Array dBoundaries(2, mu::Value(0.0));
    int nMode = 0;
    mu::Variable* dVar = nullptr;
    std::string sExpr = "";
    std::string sParams = "";
    std::string sInterval = "";
    std::string sVar = "";

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
        getDataElements(sExpr, _parser, _data, false);

    if (_data.containsTablesOrClusters(sParams))
        getDataElements(sParams, _parser, _data, false);

    // Evaluate the parameters
    if (findParameter(sParams, "min"))
        nMode = -1;

    if (findParameter(sParams, "max"))
        nMode = 1;

    if (findParameter(sParams, "samples", '='))
    {
        nSamples = cmdParser.getParsedParameterValue("samples").getAsScalarInt();

        if (nSamples < 21)
            nSamples = 21;

        sParams.erase(findParameter(sParams, "samples", '=') - 1, 8);
    }

    if (findParameter(sParams, "points", '='))
    {
        nOrder = cmdParser.getParsedParameterValue("points").getAsScalarInt();

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
            _parser.SetExpr("{" + sExpr + "}");

        mu::Array res = _parser.Eval();

        if (res.size() > 1)
            return findExtremaInMultiResult(cmdParser, sExpr, sInterval, nOrder, nMode);
        else
        {
            if (findVariableInExpression(sExpr, sVar) == std::string::npos)
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
                    dBoundaries[i] = _parser.Eval().front();

                    if (mu::isinf(dBoundaries[i].getNum().asCF64()) || mu::isnan(dBoundaries[i]))
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
                mu::Value Temp = dBoundaries[1];
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
    if (intCast(std::abs(dBoundaries[1].getNum().asCF64() - dBoundaries[0].getNum().asCF64())))
        nSamples = (nSamples - 1) * intCast(std::abs(dBoundaries[1].getNum().asCF64() - dBoundaries[0].getNum().asCF64())) + 1;

    // Ensure that we calculate a reasonable number of samples
    if (nSamples > 10001)
        nSamples = 10001;

    // Set the expression and evaluate it once
    _parser.SetExpr(sExpr);
    _parser.Eval();
    mu::Array vResults;
    dVal[0] = _parser.Diff(dVar, dBoundaries[0], 1e-7).front();

    // Evaluate the extrema for all samples. We search for
    // a sign change in the derivative and examine these intervals
    // in more detail
    for (size_t i = 1; i < nSamples; i++)
    {
        // Evaluate the derivative at the current sample position
        dVal[1] = _parser.Diff(dVar,
                               dBoundaries[0] + mu::Value(i) * (dBoundaries[1]-dBoundaries[0]) / mu::Value(nSamples-1),
                               1e-7).front();

        // Is it a sign change or a actual zero?
        if (dVal[0]*dVal[1] < mu::Value(0))
        {
            if (!nMode
                || (nMode == 1 && (dVal[0] > mu::Value(0) && dVal[1] < mu::Value(0)))
                || (nMode == -1 && (dVal[0] < mu::Value(0) && dVal[1] > mu::Value(0))))
            {
                // Examine the current interval in more detail
                vResults.push_back(localizeExtremum(sExpr, dVar, _parser, instance->getSettings(),
                                                    dBoundaries[0] + mu::Value(i-1)*(dBoundaries[1]-dBoundaries[0]) / mu::Value(nSamples-1),
                                                    dBoundaries[0] + mu::Value(i)*(dBoundaries[1]-dBoundaries[0]) / mu::Value(nSamples-1)));
            }
        }
        else if (dVal[0]*dVal[1] == mu::Value(0.0))
        {
            if (!nMode
                || (nMode == 1 && (dVal[0] > mu::Value(0.0) || dVal[1] < mu::Value(0.0)))
                || (nMode == -1 && (dVal[0] < mu::Value(0.0) || dVal[1] > mu::Value(0.0))))
            {
                int nTemp = i - 1;

                // Jump over multiple zeros due to constness
                if (dVal[0] != mu::Value(0.0))
                {
                    while (dVal[0]*dVal[1] == mu::Value(0.0) && mu::Value(i + 1 < nSamples))
                    {
                        i++;
                        dVal[1] = _parser.Diff(dVar,
                                               dBoundaries[0] + mu::Value(i)*(dBoundaries[1]-dBoundaries[0]) / mu::Value(nSamples-1),
                                               1e-7).front();
                    }
                }
                else
                {
                    while (dVal[1] == mu::Value(0.0) && mu::Value(i + 1 < nSamples))
                    {
                        i++;
                        dVal[1] = _parser.Diff(dVar,
                                               dBoundaries[0] + mu::Value(i)*(dBoundaries[1]-dBoundaries[0]) / mu::Value(nSamples-1),
                                               1e-7).front();
                    }
                }

                // Store the current location
                vResults.push_back(localizeExtremum(sExpr, dVar, _parser, instance->getSettings(),
                                                    dBoundaries[0] + mu::Value(nTemp)*(dBoundaries[1]-dBoundaries[0]) / mu::Value(nSamples-1),
                                                    dBoundaries[0] + mu::Value(i)*(dBoundaries[1]-dBoundaries[0]) / mu::Value(nSamples-1)));
            }
        }

        dVal[0] = dVal[1];
    }

    // If we didn't find any results
    // examine the boundaries for possible extremas
    if (!vResults.size())
    {
        dVal[0] = _parser.Diff(dVar, dBoundaries[0]).front();
        dVal[1] = _parser.Diff(dVar, dBoundaries[1]).front();
        std::string sRetVal;

        // Examine the left boundary
        if (std::abs(dVal[0].getNum().asCF64())
            && (!nMode
                || (bool(dVal[0] < mu::Value(0.0)) && nMode == 1)
                || (bool(dVal[0] > mu::Value(0.0)) && nMode == -1)))
            sRetVal = dBoundaries[0].print(instance->getSettings().getPrecision());

        // Examine the right boundary
        if (std::abs(dVal[1].getNum().asCF64())
            && (!nMode
                || (bool(dVal[1] < mu::Value(0.0)) && nMode == -1)
                || (bool(dVal[1] > mu::Value(0.0)) && nMode == 1)))
        {
            if (sRetVal.length())
                sRetVal += ", ";

            sRetVal += dBoundaries[1].print(instance->getSettings().getPrecision());
        }

        // Still nothing found?
        if (!std::abs(dVal[0].getNum().asCF64()) && !std::abs(dVal[1].getNum().asCF64()))
            sRetVal = "nan";

        cmdParser.setReturnValue(sRetVal);
    }
    else
        cmdParser.setReturnValue(vResults);

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
    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();
    _parser.SetExpr(sExpr);
    mu::Array v = _parser.Eval();
    MemoryManager _cache;
    size_t res = v.size();

    mu::Array vResults;

    for (size_t i = 0; i < res; i++)
        _cache.writeToTable(i, 1, "table", v[i]);

    _parser.SetExpr(sInterval);
    v = _parser.Eval();

    if (v.size() > 1)
    {
        for (size_t i = 0; i < res; i++)
        {
            if (i >= v.size())
                _cache.writeToTable(i, 0, "table", 0.0);
            else
                _cache.writeToTable(i, 0, "table", v[i]);
        }
    }
    else
        return false;

    std::string sSortingExpr = "sort -table cols=1[2]";
    _cache.sortElements(sSortingExpr);

    for (size_t i = 1; i < (size_t)_cache.getLines("table", false); i++)
    {
        if (mu::isnan(_cache.getElement(i - 1, 1, "table")))
            continue;

        if (!nMode && bool(_cache.getElement(i, 1, "table")*_cache.getElement(i - 1, 1, "table") <= mu::Value(0.0)))
        {
            if (_cache.getElement(i, 1, "table") == mu::Value(0.0))
            {
                vResults.push_back(_cache.getElement(i, 0, "table"));
                i++;
            }
            else if (_cache.getElement(i - 1, 1, "table") == mu::Value(0.0))
                vResults.push_back(_cache.getElement(i - 1, 0, "table"));
            else if (_cache.getElement(i, 1, "table")*_cache.getElement(i - 1, 1, "table") < mu::Value(0.0))
                vResults.push_back(Linearize(_cache.getElement(i - 1, 0, "table").getNum().asF64(),
                                             _cache.getElement(i - 1, 1, "table").getNum().asF64(),
                                             _cache.getElement(i, 0, "table").getNum().asF64(),
                                             _cache.getElement(i, 1, "table").getNum().asF64()));
        }
        else if (nMode && bool(_cache.getElement(i, 1, "table")*_cache.getElement(i - 1, 1, "table") <= mu::Value(0.0)))
        {
            if (_cache.getElement(i, 1, "table") == mu::Value(0.0) && _cache.getElement(i - 1, 1, "table") == mu::Value(0.0))
            {
                for (size_t j = i + 1; j < (size_t)_cache.getLines("table", false); j++)
                {
                    if (mu::Value(nMode) * _cache.getElement(j, 1, "table") > mu::Value(0.0))
                    {
                        for (size_t k = i - 1; k <= j; k++)
                            vResults.push_back(_cache.getElement(k, 0, "table"));

                        break;
                    }
                    else if (mu::Value(nMode) * _cache.getElement(j, 1, "table") < mu::Value(0.0))
                        break;

                    if (j + 1 == (size_t)_cache.getLines("table", false)
                        && i > 1
                        && bool(mu::Value(nMode) * _cache.getElement(i - 2, 1, "table") < mu::Value(0.0)))
                    {
                        for (size_t k = i - 1; k <= j; k++)
                            vResults.push_back(_cache.getElement(k, 0, "table"));

                        break;
                    }
                }

                continue;
            }
            else if (_cache.getElement(i, 1, "table") == mu::Value(0.0)
                     && mu::Value(nMode) * _cache.getElement(i - 1, 1, "table") < mu::Value(0.0))
                vResults.push_back(_cache.getElement(i, 0, "table"));
            else if (_cache.getElement(i - 1, 1, "table") == mu::Value(0.0)
                     && mu::Value(nMode) * _cache.getElement(i, 1, "table") > mu::Value(0.0))
                vResults.push_back(_cache.getElement(i - 1, 0, "table"));
            else if (_cache.getElement(i, 1, "table")*_cache.getElement(i - 1, 1, "table") < mu::Value(0.0)
                     && mu::Value(nMode) * _cache.getElement(i - 1, 1, "table") < mu::Value(0.0))
                vResults.push_back(Linearize(_cache.getElement(i - 1, 0, "table").getNum().asF64(),
                                             _cache.getElement(i - 1, 1, "table").getNum().asF64(),
                                             _cache.getElement(i, 0, "table").getNum().asF64(),
                                             _cache.getElement(i, 1, "table").getNum().asF64()));
        }
    }

    if (!vResults.size())
        vResults = mu::Value(NAN);

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
    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();
    _parser.SetExpr(sExpr);
    mu::Array v = _parser.Eval();

    if (v.size() > 1)
    {
        mu::Array vResults;

        for (size_t i = 1; i < v.size(); i++)
        {
            if (mu::isnan(v[i - 1]))
                continue;

            if (!nMode && bool(v[i]*v[i - 1] <= mu::Value(0.0)))
            {
                if (v[i] == mu::Value(0.0))
                {
                    vResults.push_back(mu::Value(i + 1));
                    i++;
                }
                else if (v[i - 1] == mu::Value(0.0))
                    vResults.push_back(mu::Value(i));
                else if (fabs(v[i].getNum().asCF64()) <= fabs(v[i - 1].getNum().asCF64()))
                    vResults.push_back(mu::Value(i + 1));
                else
                    vResults.push_back(mu::Value(i));
            }
            else if (nMode && bool(v[i]*v[i - 1] <= mu::Value(0.0)))
            {
                if (v[i] == mu::Value(0.0) && v[i - 1] == mu::Value(0.0))
                {
                    for (size_t j = i + 1; j < v.size(); j++)
                    {
                        if (mu::Value(nMode) * v[j] > mu::Value(0.0))
                        {
                            for (size_t k = i - 1; k <= j; k++)
                                vResults.push_back(mu::Value(k));

                            break;
                        }
                        else if (mu::Value(nMode) * v[j] < mu::Value(0.0))
                            break;

                        if (j + 1 == v.size()
                            && i > 2
                            && bool(mu::Value(nMode) * v[i - 2] < mu::Value(0.0)))
                        {
                            for (size_t k = i - 1; k <= j; k++)
                                vResults.push_back(mu::Value(k));

                            break;
                        }
                    }

                    continue;
                }
                else if (v[i] == mu::Value(0.0) && mu::Value(nMode) * v[i - 1] < mu::Value(0.0))
                    vResults.push_back(mu::Value(i + 1));
                else if (v[i - 1] == mu::Value(0.0) && mu::Value(nMode) * v[i] > mu::Value(0.0))
                    vResults.push_back((double)i);
                else if (fabs(v[i].getNum().asCF64()) <= fabs(v[i - 1].getNum().asCF64())
                         && bool(mu::Value(nMode) * v[i - 1] < mu::Value(0.0)))
                    vResults.push_back(mu::Value(i + 1));
                else if (mu::Value(nMode) * v[i - 1] < mu::Value(0.0))
                    vResults.push_back(mu::Value(i));
            }
        }

        if (!vResults.size())
            vResults = mu::Value(NAN);

        cmdParser.setReturnValue(vResults);
        return true;
    }
    else
        throw SyntaxError(SyntaxError::NO_ZEROES_VAR, cmdParser.getCommandLine(), SyntaxError::invalid_position);
}


/////////////////////////////////////////////////
/// \brief This function searches for the
/// positions of all zeroes (roots), which are
/// located in the selected interval.
///
/// \param sCmd string&
/// \param dVarAdress mu::Variable*
/// \param _parser mu::Parser&
/// \param _option const Settings&
/// \param dLeft mu::Value
/// \param dRight mu::Value
/// \param dEps double
/// \param nRecursion int
/// \return mu::Value
///
/// The expression has to be setted in advance.
/// The function performs recursions until the
/// defined precision is reached.
/////////////////////////////////////////////////
static mu::Value localizeZero(string& sCmd, mu::Variable* dVarAdress, mu::Parser& _parser, const Settings& _option, mu::Value dLeft, mu::Value dRight, double dEps = 1e-10, int nRecursion = 0)
{
    const size_t nSamples = 101;
    mu::Array dVal(2);

    if (_parser.GetExpr() != sCmd)
    {
        _parser.SetExpr(sCmd);
        _parser.Eval();
    }

    // Calculate the leftmost value
    *dVarAdress = dLeft;
    dVal[0] = _parser.Eval().front();

    // Separate the current interval in
    // nSamples steps and examine each step
    for (size_t i = 1; i < nSamples; i++)
    {
        // Calculate the next value
        *dVarAdress = dLeft + mu::Value(i) * (dRight - dLeft) / mu::Value(nSamples - 1);
        dVal[1] = _parser.Eval().front();

        // Multiply the values to find a sign change
        if (dVal[0]*dVal[1] < mu::Value(0))
        {
            // Sign change
            // return, if precision is reached. Otherwise perform
            // a new recursion between the two values
            if (std::abs(dRight.getNum().asCF64() - dLeft.getNum().asCF64()) / (double)(nSamples - 1) <= dEps
                || fabs(log(dEps)) + 1 < nRecursion * 2)
                return dLeft + mu::Value(i - 1) * (dRight - dLeft) / mu::Value(nSamples - 1)
                    + mu::Value(Linearize(0.0, dVal[0].getNum().asF64(),
                                          (dRight - dLeft).getNum().asF64() / (double)(nSamples - 1),
                                          dVal[1].getNum().asF64()));
            else
                return localizeZero(sCmd, dVarAdress, _parser, _option,
                                    dLeft + mu::Value(i - 1) * (dRight - dLeft) / mu::Value(nSamples - 1),
                                    dLeft + mu::Value(i) * (dRight - dLeft) / mu::Value(nSamples - 1), dEps, nRecursion + 1);
        }
        else if (dVal[0]*dVal[1] == mu::Value(0.0))
        {
            // One of the two vwlues is zero.
            // Jump over all following zeros due
            // to constness
            int nTemp = i - 1;

            if (dVal[0] != mu::Value(0.0))
            {
                while (dVal[0]*dVal[1] == mu::Value(0.0) && mu::Value(i + 1 < nSamples))
                {
                    i++;
                    *dVarAdress = dLeft + mu::Value(i) * (dRight - dLeft) / mu::Value(nSamples - 1);
                    dVal[1] = _parser.Eval().front();
                }
            }
            else
            {
                while (dVal[1] == mu::Value(0.0) && mu::Value(i + 1 < nSamples))
                {
                    i++;
                    *dVarAdress = dLeft + mu::Value(i) * (dRight - dLeft) / mu::Value(nSamples - 1);
                    dVal[1] = _parser.Eval().front();
                }
            }

            // return, if precision is reached. Otherwise perform
            // a new recursion between the two values
            if ((i - nTemp) * std::abs(dRight.getNum().asCF64() - dLeft.getNum().asCF64()) / (double)(nSamples - 1) <= dEps
                || (!nTemp && i + 1 == nSamples) || fabs(log(dEps)) + 1 < nRecursion * 2)
                return dLeft + mu::Value(nTemp) * (dRight - dLeft) / mu::Value(nSamples - 1)
                    + mu::Value(Linearize(0.0, dVal[0].getNum().asF64(),
                                          (i - nTemp) * (dRight - dLeft).getNum().asF64() / (double)(nSamples - 1),
                                          dVal[1].getNum().asF64()));
            else
                return localizeZero(sCmd, dVarAdress, _parser, _option,
                                    dLeft + mu::Value(nTemp) * (dRight - dLeft) / mu::Value(nSamples - 1),
                                    dLeft + mu::Value(i) * (dRight - dLeft) / mu::Value(nSamples - 1), dEps, nRecursion + 1);
        }

        dVal[0] = dVal[1];
    }

    // If no explict sign change was found,
    // interpolate the position by linearisation
    *dVarAdress = dLeft;
    dVal[0] = _parser.Eval().front();
    *dVarAdress = dRight;
    dVal[1] = _parser.Eval().front();
    return Linearize(dLeft.getNum().asF64(), dVal[0].getNum().asF64(),
                     dRight.getNum().asF64(), dVal[1].getNum().asF64());
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

    size_t nSamples = 21;
    mu::Array dVal(2, mu::Value(0.0));
    mu::Array dBoundaries(2, mu::Value(0.0));
    int nMode = 0;
    mu::Variable* dVar = nullptr;
    mu::Array dTemp;
    std::string sExpr = "";
    std::string sParams = "";
    std::string sInterval = "";
    std::string sVar = "";

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
        getDataElements(sExpr, _parser, _data, false);

    if (_data.containsTablesOrClusters(sParams))
        getDataElements(sParams, _parser, _data, false);

    // Evaluate the parameter list
    if (findParameter(sParams, "min") || findParameter(sParams, "down"))
        nMode = -1;
    if (findParameter(sParams, "max") || findParameter(sParams, "up"))
        nMode = 1;

    if (findParameter(sParams, "samples", '='))
    {
        nSamples = cmdParser.getParsedParameterValue("samples").getAsScalarInt();

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
            if (findVariableInExpression(sExpr, sVar) == std::string::npos)
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
                    dBoundaries[i] = _parser.Eval().front();

                    if (mu::isinf(dBoundaries[i].getNum().asCF64()) || mu::isnan(dBoundaries[i]))
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
                mu::Value Temp = dBoundaries[1];
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
    if (intCast(std::abs(dBoundaries[1].getNum().asCF64() - dBoundaries[0].getNum().asCF64())))
        nSamples = (nSamples - 1) * intCast(std::abs(dBoundaries[1].getNum().asCF64() - dBoundaries[0].getNum().asCF64())) + 1;

    // Ensure that we calculate a reasonable
    // amount of samples
    if (nSamples > 10001)
        nSamples = 10001;

    // Set the expression and evaluate it once
    _parser.SetExpr(sExpr);
    _parser.Eval();

    dTemp = *dVar;

    *dVar = dBoundaries[0];
    mu::Array vResults;
    dVal[0] = _parser.Eval().front();

    // Find near zeros to the left of the boundary
    // which are probably not located due toe rounding
    // errors
    if (bool(dVal[0] != mu::Value(0.0)) && fabs(dVal[0].getNum().asCF64()) < 1e-10)
    {
        *dVar = dBoundaries[0] - mu::Value(1e-10);
        dVal[1] = _parser.Eval().front();

        if (dVal[0]*dVal[1] < mu::Value(0) && mu::Value(nMode) * dVal[0] <= mu::Value(0.0))
            vResults.push_back(localizeExtremum(sExpr, dVar, _parser, instance->getSettings(),
                                                dBoundaries[0] - mu::Value(1e-10),
                                                dBoundaries[0]));
    }

    // Evaluate all samples. We try to find
    // sign changes and evaluate the intervals, which
    // contain the sign changes, further
    for (size_t i = 1; i < nSamples; i++)
    {
        // Evalute the current sample
        *dVar = dBoundaries[0] + mu::Value(i) * (dBoundaries[1] - dBoundaries[0]) / mu::Value(nSamples - 1);
        dVal[1] = _parser.Eval().front();

        if (dVal[0]*dVal[1] < mu::Value(0))
        {
            if (!nMode
                || (nMode == -1 && bool(dVal[0] > mu::Value(0.0) && dVal[1] < mu::Value(0.0)))
                || (nMode == 1 && bool(dVal[0] < mu::Value(0.0) && dVal[1] > mu::Value(0.0))))
            {
                // Examine the current interval
                vResults.push_back((localizeZero(sExpr, dVar, _parser, instance->getSettings(),
                                                 dBoundaries[0] + mu::Value(i-1) * (dBoundaries[1]-dBoundaries[0]) / mu::Value(nSamples-1),
                                                 dBoundaries[0] + mu::Value(i) * (dBoundaries[1]-dBoundaries[0]) / mu::Value(nSamples-1))));
            }
        }
        else if (dVal[0]*dVal[1] == mu::Value(0.0))
        {
            if (!nMode
                || (nMode == -1 && bool(dVal[0] > mu::Value(0.0) || dVal[1] < mu::Value(0.0)))
                || (nMode == 1 && bool(dVal[0] < mu::Value(0.0) || dVal[1] > mu::Value(0.0))))
            {
                int nTemp = i - 1;

                // Ignore consecutive zeros due to
                // constness
                if (dVal[0] != mu::Value(0.0))
                {
                    while (dVal[0]*dVal[1] == mu::Value(0.0) && mu::Value(i + 1 < nSamples))
                    {
                        i++;
                        *dVar = dBoundaries[0] + mu::Value(i) * (dBoundaries[1] - dBoundaries[0]) / mu::Value(nSamples - 1);
                        dVal[1] = _parser.Eval().front();
                    }
                }
                else
                {
                    while (dVal[1] == mu::Value(0.0) && mu::Value(i + 1 < nSamples))
                    {
                        i++;
                        *dVar = dBoundaries[0] + mu::Value(i) * (dBoundaries[1] - dBoundaries[0]) / mu::Value(nSamples - 1);
                        dVal[1] = _parser.Eval().front();
                    }
                }

                // Store the result
                vResults.push_back(localizeZero(sExpr, dVar, _parser, instance->getSettings(),
                                                dBoundaries[0] + mu::Value(nTemp) * (dBoundaries[1] - dBoundaries[0])/mu::Value(nSamples-1),
                                                dBoundaries[0] + mu::Value(i) * (dBoundaries[1] - dBoundaries[0])/mu::Value(nSamples-1)));
            }
        }

        dVal[0] = dVal[1];
    }

    // Examine the right boundary, because there might be
    // a zero slightly right from the interval
    if (bool(dVal[0] != mu::Value(0.0)) && fabs(dVal[0].getNum().asCF64()) < 1e-10)
    {
        *dVar = dBoundaries[1] + mu::Value(1e-10);
        dVal[1] = _parser.Eval().front();

        if (dVal[0]*dVal[1] < mu::Value(0) && mu::Value(nMode) * dVal[0] <= mu::Value(0.0))
            vResults.push_back(localizeZero(sExpr, dVar, _parser, instance->getSettings(),
                                            dBoundaries[1],
                                            dBoundaries[1] + mu::Value(1e-10)));
    }

    *dVar = dTemp;

    if (!vResults.size())
        cmdParser.setReturnValue("nan");
    else
        cmdParser.setReturnValue(vResults);

    return true;
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
    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();
    const Settings& _option = NumeReKernel::getInstance()->getSettings();

    const double dPRECISION = 1e-1;
    std::string sVarName = "";
    std::string sExpr = cmdParser.getExprAsMathExpression();
    std::string sExpr_cpy = "";
    std::string sArg = "";
    std::string sTaylor = "Taylor";
    std::string sPolynom = "";
    bool bUseUniqueName = cmdParser.hasParam("unique") || cmdParser.hasParam("u");
    size_t nth_taylor = 6;
    size_t nSamples = 0;
    mu::Variable* dVar = nullptr;
    mu::Array dVarValue;
    mu::Array vCoeffs;

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
    auto vParVal = cmdParser.getParsedParameterValue("n");

    if (vParVal.size())
        nth_taylor = abs(vParVal.getAsScalarInt());

    std::vector<std::string> vParams = cmdParser.getAllParametersWithValues();

    for (const std::string& sPar : vParams)
    {
        if (sPar != "n")
        {
            sVarName = sPar;
            dVarValue = cmdParser.getParsedParameterValue(sVarName).front();

            // Ensure that the location was chosen reasonable
            if (mu::isinf(dVarValue.front().getNum().asCF64()) || mu::isnan(dVarValue.front()))
                return;

            // Create the string element, which is used
            // for the variable in the created funcction
            // string
            if (mu::all(dVarValue == mu::Array(mu::Value(0.0))))
                sArg = "x";
            else if (mu::all(dVarValue < mu::Array(mu::Value(0))))
                sArg = "x+" + (-dVarValue).print(_option.getPrecision());
            else
                sArg = "x-" + dVarValue.print(_option.getPrecision());

            break;
        }
    }

    // Extract the expression
    sExpr_cpy = sExpr;

    // Create a unique function name, if it is desired
    if (bUseUniqueName)
        sTaylor += toString(nth_taylor) + "_" + cmdParser.getExpr();

    StripSpaces(sExpr);
    _parser.SetExpr(sExpr);

    // Ensure that the expression uses the selected variable
    if (findVariableInExpression(sExpr, sVarName) == std::string::npos)
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

        for (size_t i = 0; i < sTaylor.length(); i++)
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
        vCoeffs = _parser.Eval();
        sTaylor += vCoeffs.back().print(_option.getPrecision());
    }
    else
    {
        // nth order polynomial
        *dVar = dVarValue;

        // the constant term
        vCoeffs.push_back(_parser.Eval().front());
        sPolynom = vCoeffs.back().print(_option.getPrecision()) + ",";

        nSamples = 6*nth_taylor + 1;
        const size_t nFILTERSIZE = 7;
        NumeRe::SavitzkyGolayDiffFilter filter(nFILTERSIZE, 1);
        mu::Array vValues(nSamples, mu::Value(0.0));
        mu::Array vDiffValues(nSamples, mu::Value(0.0));;
        double dPrec = dPRECISION / nth_taylor;

        // Prepare smoothing array
        for (size_t i = 0; i < vValues.size(); i++)
        {
            *dVar = dVarValue + mu::Value(((int)i - (int)nSamples/2)*dPrec);
            vValues[i] = _parser.Eval().front();
        }

        // Perform the derivation
        for (size_t n = 0; n < nth_taylor; n++)
        {
            for (size_t i = nFILTERSIZE/2; i < vValues.size()-nFILTERSIZE/2; i++)
            {
                vDiffValues[i] = mu::Value(0.0);

                for (size_t j = 0; j < nFILTERSIZE; j++)
                    vDiffValues[i] += mu::Value(filter.apply(j, 0, vValues[i + j - nFILTERSIZE/2].getNum().asCF64()));

                vDiffValues[i] /= mu::Value(dPrec);
            }

            vCoeffs.push_back(vDiffValues[vDiffValues.size()/2] / mu::Value(integralFactorial(n+1)));
            sPolynom += vCoeffs.back().print(_option.getPrecision()) + ",";
            vValues = vDiffValues;
        }

        sTaylor += "polynomial(" + sArg + "," + sPolynom.substr(0, sPolynom.length()-1) + ")";
    }

    //if (_option.systemPrints())
    //    NumeReKernel::print(LineBreak(sTaylor, _option, true, 0, 8));

    sTaylor += " " + _lang.get("PARSERFUNCS_TAYLOR_DEFINESTRING", sExpr_cpy, sVarName, dVarValue.print(4), toString(nth_taylor));

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

    cmdParser.setReturnValue(vCoeffs);
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
/// \brief This structure gathers all information
/// needed for calculating a FFT in one or two
/// dimensions.
/////////////////////////////////////////////////
struct FFTData
{
    int lines;
    int cols;
    bool bInverseTrafo;
    bool bShiftAxis;
    bool bComplex;
    double dFrequencyOffset;
    double dNyquistFrequency[2];
    double dTimeInterval[2];
};


/////////////////////////////////////////////////
/// \brief This static function calculates a 1D
/// FFT and stores the result in the target table.
///
/// \param _data MemoryManager&
/// \param _idx Indices&
/// \param sTargetTable const std::string&
/// \param _fftData mglDataC&
/// \param vAxis std::vector<size_t>&
/// \param _fft FFTData&
/// \return void
///
/////////////////////////////////////////////////
static void calculate1dFFT(MemoryManager& _data, Indices& _idx, const std::string& sTargetTable, mglDataC& _fftData, std::vector<size_t>& vAxis, FFTData& _fft)
{
    //_fftData.Save("D:/Software/NumeRe/save/fftdata.txt");

    if (!_fft.bInverseTrafo)
    {
        _fftData.FFT("x");

        double samples = _fft.lines/2.0;

        _fftData.a[0] /= dual(2*samples, 0.0);

        for (int i = 1; i < _fftData.GetNx(); i++)
            _fftData.a[i] /= dual(samples, 0.0);
    }
    else
    {
        double samples = _fftData.GetNx()/2.0;

        _fftData.a[0] *= dual(2*samples, 0.0);

        for (int i = 1; i < _fftData.GetNx(); i++)
            _fftData.a[i] *= dual(samples, 0.0);

        _fftData.FFT("ix");
    }

    if (_idx.col.isOpenEnd())
        _idx.col.setRange(0, _idx.col.front() + 3);

    if (_fft.bShiftAxis && !_fft.bInverseTrafo)
        vAxis = getShiftedAxis(_fft.lines, _fft.bInverseTrafo);

    // Store the results of the transformation in the target
    // table
    if (!_fft.bInverseTrafo)
    {
        size_t nElements = _fftData.GetNx();
        double dPhaseOffset = 0.0;

        if (_idx.row.isOpenEnd())
            _idx.row.setRange(0, _idx.row.front() + nElements);

        for (size_t i = 0; i < nElements; i++)
        {
            if (i > _idx.row.size())
                break;

            if (_fft.bShiftAxis)
                _data.writeToTable(_idx.row[i], _idx.col.front(), sTargetTable,
                                   _fft.dFrequencyOffset + 2.0 * (double)(i)*_fft.dNyquistFrequency[0] / (double)(_fftData.GetNx()));
            else if (i <= nElements/2)
                _data.writeToTable(_idx.row[i], _idx.col.front(), sTargetTable,
                                   _fft.dFrequencyOffset + 2.0 * (double)(i)*_fft.dNyquistFrequency[0] / (double)(_fftData.GetNx()));
            else
                _data.writeToTable(_idx.row[i], _idx.col.front(), sTargetTable,
                                   _fft.dFrequencyOffset + 2.0 * (double)(-(int)nElements+(int)i)*_fft.dNyquistFrequency[0] / (double)(_fftData.GetNx()));

            if (!_fft.bComplex)
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
                // Only complex values
                _data.writeToTable(_idx.row[vAxis[i]], _idx.col[1], sTargetTable, mu::Value(_fftData.a[i], false));
            }
        }

        // Write headlines
        _data.setHeadLineElement(_idx.col.front(), sTargetTable, _lang.get("COMMON_FREQUENCY") + " [Hz]");

        if (!_fft.bComplex)
        {
            _data.setHeadLineElement(_idx.col[1], sTargetTable, _lang.get("COMMON_AMPLITUDE"));
            _data.setHeadLineElement(_idx.col[2], sTargetTable, _lang.get("COMMON_PHASE") + " [rad]");
        }
        else
            _data.setHeadLineElement(_idx.col[1], sTargetTable, _lang.get("COMMON_AMPLITUDE"));
    }
    else
    {
        if (_idx.row.isOpenEnd())
            _idx.row.setRange(0, _idx.row.front() + _fftData.GetNx());

        for (int i = 0; i < _fftData.GetNx(); i++)
        {
            if (i > (int)_idx.row.size())
                break;

            _data.writeToTable(_idx.row[i], _idx.col[0], sTargetTable, (double)(i)*_fft.dTimeInterval[0] / (double)(_fftData.GetNx() - 1));
            _data.writeToTable(_idx.row[i], _idx.col[1], sTargetTable, mu::Value(_fftData.a[i], false));
        }

        // Write headlines
        _data.setHeadLineElement(_idx.col[0], sTargetTable, _lang.get("COMMON_TIME") + " [s]");
        _data.setHeadLineElement(_idx.col[1], sTargetTable, _lang.get("COMMON_SIGNAL"));
    }
}


/////////////////////////////////////////////////
/// \brief This static function calculates a 2D
/// FFT and stores the result in the target table.
///
/// \param _data MemoryManager&
/// \param _idx Indices&
/// \param sTargetTable const std::string&
/// \param _fftData mglDataC&
/// \param vAxis std::vector<size_t>&
/// \param _fft FFTData&
/// \return void
///
/////////////////////////////////////////////////
static void calculate2dFFT(MemoryManager& _data, Indices& _idx, const std::string& sTargetTable, mglDataC& _fftData, std::vector<size_t>& vAxis, FFTData& _fft)
{
    if (!_fft.bInverseTrafo)
    {
        _fftData.FFT("xy");

        double samples = _fft.lines*(_fft.cols-2)/2.0;

        _fftData.a[0] /= dual(2*samples, 0.0);

        for (long long int i = 1; i < _fftData.GetNN(); i++)
            _fftData.a[i] /= dual(samples, 0.0);
    }
    else
    {
        double samples = _fft.lines*(_fft.cols-2)/2.0;

        _fftData.a[0] *= dual(2*samples, 0.0);

        for (long long int i = 1; i < _fftData.GetNN(); i++)
            _fftData.a[i] *= dual(samples, 0.0);

        _fftData.FFT("iy");
        _fftData.FFT("ix");
    }

    if (_idx.col.isOpenEnd())
        _idx.col.setRange(0, _idx.col.front() + _fft.cols);

    // Store the results of the transformation in the target
    // table
    if (!_fft.bInverseTrafo)
    {
        int nElemsLines = _fftData.GetNx();
        int nElemsCols = _fftData.GetNy();

        if (_idx.row.isOpenEnd())
            _idx.row.setRange(0, _idx.row.front() + std::max(nElemsCols, nElemsLines));

        for (int i = 0; i < nElemsLines; i++)
        {
            if ((size_t)i > _idx.row.size())
                break;

            // Write x axis
            if (_fft.bShiftAxis)
                _data.writeToTable(_idx.row[i], _idx.col[0], sTargetTable, _fft.dNyquistFrequency[0]*(-1.0 + 2.0 * (double)(i) / nElemsLines));
            else
                _data.writeToTable(_idx.row[i], _idx.col[0], sTargetTable, 2.0 * (double)(i)*_fft.dNyquistFrequency[0] / (double)(nElemsLines));

            for (int j = 0; j < nElemsCols; j++)
            {
                // Write the values
                if (_fft.bShiftAxis)
                    _data.writeToTable(_idx.row[i + (i >= nElemsLines/2 ? -nElemsLines/2 : nElemsLines/2+nElemsLines % 2)],
                                       _idx.col[j+2 + (j >= nElemsCols/2 ? -nElemsCols/2 : nElemsCols/2+nElemsCols % 2)],
                                       sTargetTable,
                                       _fft.bComplex ? mu::Value(_fftData.a[i+j*nElemsLines], false) : mu::Value(std::abs(_fftData.a[i+j*nElemsLines])));
                else
                    _data.writeToTable(_idx.row[i],
                                       _idx.col[j+2],
                                       sTargetTable,
                                       _fft.bComplex ? mu::Value(_fftData.a[i+j*nElemsLines], false) : mu::Value(std::abs(_fftData.a[i+j*nElemsLines])));
            }
        }

        // Write y axis
        for (int i = 0; i < nElemsCols; i++)
        {
            if (i > (int)_idx.row.size())
                break;

            if (_fft.bShiftAxis)
                _data.writeToTable(_idx.row[i], _idx.col[1], sTargetTable, _fft.dNyquistFrequency[1]*(-1.0 + 2.0 * (double)(i) / nElemsCols));
            else
                _data.writeToTable(_idx.row[i], _idx.col[1], sTargetTable, 2.0 * (double)(i)*_fft.dNyquistFrequency[1] / (double)(nElemsCols));
        }

        // Write headlines
        _data.setHeadLineElement(_idx.col[0], sTargetTable, _lang.get("COMMON_FREQUENCY") + " [Hz]");
        _data.setHeadLineElement(_idx.col[1], sTargetTable, _lang.get("COMMON_FREQUENCY") + " [Hz]");

        for (int j = 0; j < nElemsCols; j++)
        {
            _data.setHeadLineElement(_idx.col[j+2], sTargetTable, _lang.get("COMMON_AMPLITUDE") + "(:," + toString(j+1) + ")");
        }
    }
    else
    {
        int nElemsLines = _fftData.GetNx();
        int nElemsCols = _fftData.GetNy();

        if (_idx.row.isOpenEnd())
            _idx.row.setRange(0, _idx.row.front() + std::max(nElemsCols, nElemsLines));

        for (int i = 0; i < nElemsLines; i++)
        {
            if ((size_t)i > _idx.row.size())
                break;

            // Write x axis
            _data.writeToTable(_idx.row[i], _idx.col[0], sTargetTable, (double)(i)*_fft.dTimeInterval[0] / (double)(nElemsLines - 1));

            // Write the values
            for (int j = 0; j < nElemsCols; j++)
            {
                _data.writeToTable(_idx.row[i], _idx.col[j+2], sTargetTable, mu::Value(_fftData.a[i+j*nElemsLines], false));
            }
        }

        // Write y axis
        for (int i = 0; i < nElemsCols; i++)
        {
            if ((size_t)i > _idx.row.size())
                break;

            _data.writeToTable(_idx.row[i], _idx.col[1], sTargetTable, (double)(i)*_fft.dTimeInterval[1] / (double)(nElemsCols - 1));
        }

        // Write headlines
        _data.setHeadLineElement(_idx.col[0], sTargetTable, _lang.get("COMMON_TIME") + " [s]");
        _data.setHeadLineElement(_idx.col[1], sTargetTable, _lang.get("COMMON_TIME") + " [s]");

        for (int j = 0; j < nElemsCols; j++)
        {
            _data.setHeadLineElement(_idx.col[j+2], sTargetTable, _lang.get("COMMON_SIGNAL") + "(:," + toString(j+1) + ")");
        }
    }
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
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    const Settings& _option = NumeReKernel::getInstance()->getSettings();

    mglDataC _fftData;
    Indices _idx;
    FFTData _fft;

    _fft.dNyquistFrequency[0] = 1;
    _fft.dNyquistFrequency[1] = 1;
    _fft.dTimeInterval[0] = 0;
    _fft.dTimeInterval[1] = 0;
    _fft.dFrequencyOffset = 0.0;
    _fft.bInverseTrafo = cmdParser.hasParam("inverse");
    _fft.bComplex = cmdParser.hasParam("complex");
    _fft.bShiftAxis = cmdParser.hasParam("axisshift");
    bool bIs2DFFT = cmdParser.getCommand() == "fft2d";
    string sTargetTable = "fftdata";

    // search for explicit "target" options and select the target cache
    sTargetTable = cmdParser.getTargetTable(_idx, sTargetTable);

    DataAccessParser accessParser = cmdParser.getExprAsDataObject();

    // get the data from the data object and sort only for the forward transformation
    std::unique_ptr<Memory> _mem(extractRange(cmdParser.getCommandLine(), accessParser, bIs2DFFT ? -1 : 2, !_fft.bInverseTrafo));

    if (!_mem)
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, cmdParser.getCommandLine(), accessParser.getDataObject() + "(", accessParser.getDataObject() + "()");

    if (!_mem->isValueLike(VectorIndex(0, _mem->getCols()-1)))
        throw SyntaxError(SyntaxError::WRONG_COLUMN_TYPE, cmdParser.getCommandLine(), accessParser.getDataObject()+"(", accessParser.getDataObject());

    _mem->shrink();

    _fft.lines = _mem->getElemsInColumn(0);
    _fft.cols = _mem->getCols();

    if (_fft.lines % 2 && _fft.lines > 1e3)
        _fft.lines--;

    _fft.dNyquistFrequency[0] = _fft.lines/(_mem->readMem(_fft.lines - 1, 0).getNum().asF64()-_mem->readMem(0, 0).getNum().asF64())/2.0;
    _fft.dTimeInterval[0] = (_fft.lines - 1) / (_mem->max(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(0)).real() - _mem->min(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(0)).real());

    if (bIs2DFFT)
    {
        if (_fft.cols % 2 && _fft.cols > 1e3)
            _fft.cols--;

        int collines = _mem->getElemsInColumn(1);

        _fft.dNyquistFrequency[1] = collines/(_mem->readMem(collines-1, 1).getNum().asF64()-_mem->readMem(0, 1).getNum().asF64())/2.0;
        _fft.dTimeInterval[1] = (collines - 1) / (_mem->readMem(collines - 1, 1).getNum().asF64());
    }

    // Check the dimensions of the input data
    if (_fft.lines < 10 || _fft.cols < 2 || (bIs2DFFT && _fft.cols < _mem->getElemsInColumn(1)+2))
        throw SyntaxError(SyntaxError::WRONG_DATA_SIZE, cmdParser.getCommandLine(), cmdParser.getExpr());

    // Adapt the values for the shifted axis
    if (_fft.bShiftAxis)
    {
        _fft.dFrequencyOffset = -_fft.dNyquistFrequency[0] * (1 + (_fft.lines % 2) * 1.0 / _fft.lines);
        _fft.dTimeInterval[0] = fabs((_fft.lines + (_fft.lines % 2)) / (_mem->readMem(0, 0).getNum().asF64())) * 0.5;

        if (bIs2DFFT)
            _fft.dTimeInterval[1] = fabs((_fft.cols-2 + (_fft.cols % 2)) / (_mem->readMem(0, 1).getNum().asF64())) * 0.5;
    }

    if (_option.systemPrints())
    {
        if (!_fft.bInverseTrafo)
            NumeReKernel::printPreFmt(LineBreak("|-> " + _lang.get("PARSERFUNCS_FFT_FOURIERTRANSFORMING", toString(_fft.cols)) + " ", _option, 0));
        else
            NumeReKernel::printPreFmt(LineBreak("|-> " + _lang.get("PARSERFUNCS_FFT_INVERSE_FOURIERTRANSFORMING", toString(_fft.cols)) + " ", _option, 0));
    }

    if (bIs2DFFT)
        _fftData.Create(_fft.lines, _fft.cols-2);
    else
        _fftData.Create(_fft.lines);

    std::vector<size_t> vAxis;

    // Prepare the axis (shifted if necessary)
    if (_fft.bShiftAxis && _fft.bInverseTrafo)
        vAxis = getShiftedAxis(_fft.lines, _fft.bInverseTrafo);
    else
    {
        for (size_t i = 0; i < (size_t)_fft.lines; i++)
            vAxis.push_back(i);
    }

    // Lambda expression for catching and converting NaNs into zeros
    auto nanguard = [](const mu::Value& val) {return mu::isnan(val) ? std::complex<double>(0.0) : val.getNum().asCF64();};

    // Copy the data
    for (int i = 0; i < _fft.lines; i++)
    {
        if (_fft.cols == 2)
            _fftData.a[i] = nanguard(_mem->readMem(vAxis[i], 1)); // Can be complex or not: does not matter
        else if (_fft.cols == 3 && _fft.bComplex)
            _fftData.a[i] = nanguard(dual(_mem->readMem(vAxis[i], 1).getNum().asF64(), _mem->readMem(vAxis[i], 2).getNum().asF64()));
        else if (_fft.cols == 3 && !_fft.bComplex)
            _fftData.a[i] = nanguard(dual(_mem->readMem(vAxis[i],1).getNum().asF64()*cos(_mem->readMem(vAxis[i], 2).getNum().asF64()),
                                          _mem->readMem(vAxis[i],1).getNum().asF64()*sin(_mem->readMem(vAxis[i], 2).getNum().asF64())));
        else if (bIs2DFFT)
        {
            int nLines = _fft.lines;
            int nCols = _fft.cols-2;

            for (int j = 0; j < nCols; j++)
            {
                if (_fft.bShiftAxis && _fft.bInverseTrafo)
                    _fftData.a[i+j*nLines] = nanguard(_mem->readMem(i + (i >= nLines/2 ? -nLines/2 : nLines/2 + nLines % 2),
                                                                    j+2 + (j >= nCols/2 ? -nCols/2 : nCols/2 + nCols % 2)));
                else
                    _fftData.a[i+j*nLines] = nanguard(_mem->readMem(i, j+2));
            }
        }

    }

    // Calculate the actual transformation and apply some
    // normalisation
    if (bIs2DFFT)
        calculate2dFFT(_data, _idx, sTargetTable, _fftData, vAxis, _fft);
    else
        calculate1dFFT(_data, _idx, sTargetTable, _fftData, vAxis, _fft);


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
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    const Settings& _option = NumeReKernel::getInstance()->getSettings();

    std::vector<double> vWaveletData;
    std::vector<double> vAxisData;
    Indices _idx;

    bool bInverseTrafo = cmdParser.hasParam("inverse");
    bool bTargetGrid = cmdParser.hasParam("grid");
    std::string sTargetTable = "fwtdata";
    std::string sType = "d"; // d = daubechies, cd = centered daubechies, h = haar, ch = centered haar, b = bspline, cb = centered bspline
    int k = 4;

    std::string sParVal = cmdParser.getParameterValue("type");

    if (sParVal.length())
        sType = sParVal;

    sParVal = cmdParser.getParameterValue("method");

    if (!sType.length() && sParVal.length())
        sType = sParVal;

    mu::Array vParVal = cmdParser.getParsedParameterValue("k");

    if (vParVal.size())
        k = vParVal.getAsScalarInt();

    // search for explicit "target" options and select the target cache
    sTargetTable = cmdParser.getTargetTable(_idx, sTargetTable);

    DataAccessParser accessParser = cmdParser.getExprAsDataObject();

    // get the data from the data object
    std::unique_ptr<Memory> _mem(extractRange(cmdParser.getCommandLine(), accessParser, 2, true));

    if (!_mem)
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, cmdParser.getCommandLine(), accessParser.getDataObject() + "(", accessParser.getDataObject() + "()");

    if (!_mem->isValueLike(VectorIndex(0, _mem->getCols()-1)))
        throw SyntaxError(SyntaxError::WRONG_COLUMN_TYPE, cmdParser.getCommandLine(), accessParser.getDataObject()+"(", accessParser.getDataObject());

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

    for (size_t i = 0; i < (size_t)_mem->getLines(); i++)
    {
        vWaveletData.push_back(_mem->readMem(i, 1).getNum().asF64());

        if (bTargetGrid)
            vAxisData.push_back(_mem->readMem(i, 0).getNum().asF64());
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

                _data.writeToTable(_idx.row[i], _idx.col[j], sTargetTable, tWaveletData.getValue(i, j).real());
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

    for (size_t i = 0; i < vWaveletData.size(); i++)
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
    mu:: Parser& _parser = NumeReKernel::getInstance()->getParser();
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    size_t nSamples = 100;
    mu::Variable* dVar = 0;
    mu::Array dTemp = 0.0;
    std::string sExpr = cmdParser.getExprAsMathExpression();
    std::string sVar = "x";
    static string zero = "0.0";
    bool bLogarithmic = cmdParser.hasParam("logscale");

    // Evaluate calls in the expression
    // to any table or cluster
    if (_data.containsTablesOrClusters(sExpr))
    {
        getDataElements(sExpr, _parser, _data);

        if (sExpr.find("{") != string::npos)
            convertVectorToExpression(sExpr);
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
                mu::Array* res = _parser.Eval(nIndices);
                ivl.intervals.push_back(Interval(res[0].front().getNum().asCF64(), res[1].front().getNum().asCF64()));

                break;
            }
        }
    }

    if (!ivl.size())
        ivl.intervals.push_back(Interval(-10.0, 10.0));

    mu::Array vSamples = cmdParser.getParsedParameterValue("samples");

    if (vSamples.size())
        nSamples = vSamples.getAsScalarInt();
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

    if (mu::isnan(ivl[0].front()) && mu::isnan(ivl[0].back()))
        ivl[0] = Interval(-10.0, 10.0);
    else if (mu::isnan(ivl[0].front()) || mu::isnan(ivl[0].back()) || mu::isinf(ivl[0].front()) || mu::isinf(ivl[0].back()))
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
    mu::Array vResults;

    // Evaluate the selected expression at
    // the selected samples
    if (dVar)
    {
        dTemp = *dVar;
        *dVar = ivl[0](0);
        vResults.push_back(_parser.Eval().front());

        for (size_t i = 1; i < nSamples; i++)
        {
            // Is a logarithmic distribution needed?
            if (bLogarithmic)
                *dVar = ivl[0].log(i, nSamples);
            else
                *dVar = ivl[0](i, nSamples);

            vResults.push_back(_parser.Eval().front());
        }

        *dVar = dTemp;
    }
    else
    {
        for (size_t i = 0; i < nSamples; i++)
            vResults.push_back(_parser.Eval().front());
    }

    cmdParser.setReturnValue(vResults);
    return true;
}


/////////////////////////////////////////////////
/// \brief This function will obtain the samples
/// of the datagrid for each spatial direction.
///
/// \param cmdParser CommandLineParser&
/// \param nSamples const std::vector<size_t>&
/// \return std::vector<size_t>
///
/////////////////////////////////////////////////
static std::vector<size_t> getSamplesForDatagrid(CommandLineParser& cmdParser, const std::vector<size_t>& nSamples)
{
    vector<size_t> vSamples = nSamples;

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
            _idx.row.setRange(0, _data.getColElements(_idx.col, sZDatatable)-1);

        vSamples.front() = _idx.row.size();
        vSamples.back() = _idx.col.size();

        // Check for singletons
        if (vSamples[0] < 2 && vSamples[1] >= 2)
            vSamples[0] = vSamples[1];
        else if (vSamples[1] < 2 && vSamples[0] >= 2)
            vSamples[1] = vSamples[0];
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
/// \param vZVals std::vector<mu::Array>&
/// \param nSamples_x size_t
/// \param nSamples_y size_t
/// \return void
///
/////////////////////////////////////////////////
static void expandVectorToDatagrid(IntervalSet& ivl, std::vector<mu::Array>& vZVals, size_t nSamples_x, size_t nSamples_y)
{
    // Only if a dimension is a singleton
    if (vZVals.size() == 1 || vZVals[0].size() == 1)
    {
        mu::Array vVector;

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
        for (size_t i = 0; i < nSamples_x; i++)
            _mData[1].a[i] = ivl[0](i, nSamples_x).real();

        for (size_t i = 0; i < nSamples_y; i++)
            _mData[2].a[i] = ivl[1](i, nSamples_y).real();

        // copy the z vector
        if (vZVals.size() != 1)
        {
            for (size_t i = 0; i < vZVals.size(); i++)
                _mData[3].a[i] = vZVals[i][0].getNum().asF64();
        }
        else
        {
            for (size_t i = 0; i < vZVals[0].size(); i++)
                _mData[3].a[i] = vZVals[0][i].getNum().asF64();
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
        for (size_t i = 0; i < nSamples_x; i++)
        {
            for (size_t j = 0; j < nSamples_y; j++)
                vVector.push_back(_mData[0].a[i + nSamples_x * j]);

            vZVals.push_back(vVector);
            vVector.clear();
        }
    }
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
    std::vector<size_t> vSamples = {100, 100};
    bool bTranspose = cmdParser.hasParam("transpose");

    Indices _iTargetIndex;
    std::vector<mu::Array> vZVals;

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
    auto vParVal = cmdParser.getParsedParameterValue("samples");

    if (vParVal.size())
    {
        vSamples.front() = abs(vParVal.getAsScalarInt());

        if (vParVal.size() >= 2)
            vSamples[1] = abs(vParVal[1].getNum().asI64());
        else
            vSamples[1] = vSamples.front();

    }

    if (vSamples.front() < 2 || vSamples.back() < 2)
        throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Get the samples
    vSamples = getSamplesForDatagrid(cmdParser, vSamples);

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

        if (!_data.isValueLike(_accessParser.getIndices().col, _accessParser.getDataObject()))
            throw SyntaxError(SyntaxError::WRONG_COLUMN_TYPE, cmdParser.getCommandLine(), _accessParser.getDataObject()+"(", _accessParser.getDataObject());

        Indices& _idx = _accessParser.getIndices();

        // identify the table
        std::string& szDatatable = _accessParser.getDataObject();

        // Check the indices
        if (!isValidIndexSet(_idx))
            throw SyntaxError(SyntaxError::INVALID_INDEX, cmdParser.getCommandLine(), SyntaxError::invalid_position, _idx.row.to_string() + ", " + _idx.col.to_string());

        // the indices are vectors
        mu::Array vVector;

        if (_idx.col.isOpenEnd())
            _idx.col.setRange(0, _data.getCols(szDatatable)-1);

        if (_idx.row.isOpenEnd())
            _idx.row.setRange(0, _data.getColElements(_idx.col.subidx(0), szDatatable)-1);

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

        mu::Array vVector;

        for (size_t x = 0; x < vSamples[bTranspose]; x++)
        {
            _defVars.vValue[0][0] = ivl[0](x, vSamples[bTranspose]);

            for (size_t y = 0; y < vSamples[1-bTranspose]; y++)
            {
                _defVars.vValue[1][0] = ivl[1](y, vSamples[1-bTranspose]);
                vVector.push_back(_parser.Eval().front());
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
    _iTargetIndex.row.setOpenEndIndex(_iTargetIndex.row.front()+vZVals.size()-1);
    int rowmax = _iTargetIndex.row.max();

    // Pre-allocate
    for (size_t j = 0; j < vZVals.front().size(); j++)
    {
        if (_iTargetIndex.col[j+2] == VectorIndex::INVALID)
            break;

        _data.writeToTable(rowmax, _iTargetIndex.col[j+2], sTargetCache, mu::Value(0.0, false));
    }

    // Ensure, we can write complex values
    _data.convertColumns(sTargetCache, _iTargetIndex.col.subidx(2), "value.cf64");
    Memory* tab = _data.getTable(sTargetCache);

    // Write the z matrix
    for (size_t i = 0; i < vZVals.size(); i++)
    {
        if (_iTargetIndex.row[i] == VectorIndex::INVALID)
            break;

        for (size_t j = 0; j < vZVals[i].size(); j++)
        {
            if (_iTargetIndex.col[j+2] == VectorIndex::INVALID)
                break;

            tab->writeDataDirectUnsafe(_iTargetIndex.row[i], _iTargetIndex.col[j+2], vZVals[i][j].getNum().asCF64());

            if (!i)
                tab->setHeadLineElement(_iTargetIndex.col[j+2], "z(x(:),y(" + toString(j + 1) + "))");
        }
    }

    return true;
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
    mu::Array vVals = cmdParser.getParsedParameterValue("samples");

    if (vVals.size())
        nSamples = vVals.getAsScalarInt();

    // Dateiname lesen
    sAudioFileName = cmdParser.getFileParameterValueForSaving(".wav", "<savepath>", sAudioFileName);

    // Indices lesen
    DataAccessParser _accessParser = cmdParser.getExprAsDataObject();
    Indices& _idx = _accessParser.getIndices();

    if (_idx.col.isOpenEnd())
        _idx.col.setRange(0, _idx.col.front() + 1);

    if (!_data.isValueLike(_idx.col, _accessParser.getDataObject()))
        throw SyntaxError(SyntaxError::WRONG_COLUMN_TYPE, cmdParser.getCommandLine(), _accessParser.getDataObject()+"(", _accessParser.getDataObject());

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

    const std::string& sTab = _accessParser.getDataObject();

    for (size_t i = 0; i < _idx.row.size(); i++)
    {
        audiofile.get()->write(Audio::Sample(_data.getElement(_idx.row[i],_idx.col[0],sTab).getNum().asF64()/dMax,
                                             nChannels > 1 ? _data.getElement(_idx.row[i],_idx.col[1],sTab).getNum().asF64()/dMax : NAN));
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
    std::string sAudioFile = cmdParser.getExprAsFileName(".wav");

    g_logger.info("Load audiofile '" + sAudioFile + "'.");

    // Read the whole table or only the metadata
    if (sTarget.length())
    {
        std::unique_ptr<Audio::File> audiofile(Audio::getAudioFileByType(sAudioFile));

        if (!audiofile || !audiofile->isValid())
            return false;

        size_t nLen = audiofile->getLength();
        size_t nChannels = audiofile->getChannels();

        // Try to read the entire file
        _data.resizeTable(nChannels > 1 && _targetIdx.col.size() > 1
                          ? _targetIdx.col.subidx(0, 2).max()+1
                          : _targetIdx.col.front()+1,
                          sTarget);

        Memory* _table = _data.getTable(sTarget);
        int rowmin = _targetIdx.row.subidx(0, nLen).min();
        int rowmax = _targetIdx.row.subidx(0, nLen).max();

        // Write the first row for conversion and afterwards
        // last row for preallocation
        _table->writeData(rowmin, _targetIdx.col.front(), mu::Numerical(0.0F));
        _table->convertColumns(_targetIdx.col.subidx(0, 1), "value.f32");
        _table->writeData(rowmax, _targetIdx.col.front(), mu::Numerical(0.0F));

        // Same for second channel, if any
        if (nChannels > 1 && _targetIdx.col.size() > 1)
        {
            _table->writeData(rowmin, _targetIdx.col[1], mu::Numerical(0.0F));
            _table->convertColumns(_targetIdx.col.subidx(1, 1), "value.f32");
            _table->writeData(rowmax, _targetIdx.col[1], mu::Numerical(0.0F));
        }

        for (size_t i = 0; i < nLen; i++)
        {
            Audio::Sample sample = audiofile->read();

            if (_targetIdx.row.size() <= i)
                break;

            _table->writeDataDirectUnsafe(_targetIdx.row[i], _targetIdx.col[0], sample.leftOrMono);

            if (nChannels > 1 && _targetIdx.col.size() > 1)
                _table->writeDataDirectUnsafe(_targetIdx.row[i], _targetIdx.col[1], sample.right);
        }

        if (nChannels > 1 && _targetIdx.col.size() > 1)
        {
            _table->setHeadLineElement(_targetIdx.col[0], "A_L");
            _table->setHeadLineElement(_targetIdx.col[1], "A_R");
        }
        else
            _table->setHeadLineElement(_targetIdx.col[0], "A");

        _table->markModified();

        // Create the storage indices
        std::vector<std::complex<double>> vIndices = {_targetIdx.row.min()+1,
            _targetIdx.row.size() < nLen ? _targetIdx.row.max()+1 : _targetIdx.row.min()+nLen,
            (nChannels > 1 && _targetIdx.col.size() > 1 ? _targetIdx.col.subidx(0, 2).min()+1 : _targetIdx.col.front()+1),
            (nChannels > 1 && _targetIdx.col.size() > 1 ? _targetIdx.col.subidx(0, 2).max()+1 : _targetIdx.col.front()+1)};

        cmdParser.setReturnValue(vIndices);
        g_logger.info("Audiofile read.");
    }
    else
    {
        std::unique_ptr<Audio::File> audiofile(Audio::getAudioFileByType(sAudioFile));

        if (!audiofile || !audiofile->isValid())
            return false;

        // Only read the metadata
        size_t nLen = audiofile->getLength();
        size_t nChannels = audiofile->getChannels();
        size_t nSampleRate = audiofile->getSampleRate();

        // Create the metadata
        std::vector<std::complex<double>> vMetaData = {nLen, nChannels, nSampleRate, nLen / (double)nSampleRate};

        cmdParser.setReturnValue(vMetaData);
        g_logger.info("Audiofile metadata read.");
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
    std::vector<mu::Array> vSeekIndices = cmdParser.parseExpr();

    if (vSeekIndices.size() < 2)
        return false;

    std::string sAudioFile = cmdParser.getFileParameterValue(".wav");

    g_logger.info("Load audiofile '" + sAudioFile + "'.");
    std::unique_ptr<Audio::File> audiofile(Audio::getAudioFileByType(sAudioFile));

    if (!audiofile || !audiofile->isValid())
        return false;

    size_t nLen = audiofile->getLength();
    size_t nChannels = audiofile->getChannels();

    if (!audiofile->isSeekable() || std::max(vSeekIndices[0].front().getNum().asF64()-1, 0.0) >= nLen)
        return false;

    std::unique_ptr<Audio::SeekableFile> seekable(static_cast<Audio::SeekableFile*>(audiofile.release()));

    seekable->setPosition(std::max(vSeekIndices[0].front().getNum().asF64()-1, 0.0));
    nLen = std::min(nLen - seekable->getPosition(), (size_t)(std::max(vSeekIndices[1].front().getNum().asF64(), 0.0)));

    // Try to read the desired length from the file
    _data.resizeTable(nChannels > 1 && _targetIdx.col.size() > 1 ? _targetIdx.col.subidx(0, 2).max()+1 : _targetIdx.col.front()+1,
                      sTarget);

    Memory* _table = _data.getTable(sTarget);
    int rowmin = _targetIdx.row.subidx(0, nLen).min();
    int rowmax = _targetIdx.row.subidx(0, nLen).max();

    // Write the first row for conversion and afterwards
    // last row for preallocation
    _table->writeData(rowmin, _targetIdx.col.front(), mu::Numerical(0.0F));
    _table->convertColumns(_targetIdx.col.subidx(0, 1), "value.f32");
    _table->writeData(rowmax, _targetIdx.col.front(), mu::Numerical(0.0F));

    // Same for second channel, if any
    if (nChannels > 1 && _targetIdx.col.size() > 1)
    {
        _table->writeData(rowmin, _targetIdx.col[1], mu::Numerical(0.0F));
        _table->convertColumns(_targetIdx.col.subidx(1, 1), "value.f32");
        _table->writeData(rowmax, _targetIdx.col[1], mu::Numerical(0.0F));
    }

    for (size_t i = 0; i < nLen; i++)
    {
        Audio::Sample sample = seekable->read();

        if (_targetIdx.row.size() <= i)
            break;

        _table->writeDataDirectUnsafe(_targetIdx.row[i], _targetIdx.col[0], sample.leftOrMono);

        if (nChannels > 1 && _targetIdx.col.size() > 1)
            _table->writeDataDirectUnsafe(_targetIdx.row[i], _targetIdx.col[1], sample.right);
    }

    _table->markModified();

    if (nChannels > 1 && _targetIdx.col.size() > 1)
    {
        _table->setHeadLineElement(_targetIdx.col[0], "A_L");
        _table->setHeadLineElement(_targetIdx.col[1], "A_R");
    }
    else
        _table->setHeadLineElement(_targetIdx.col[0], "A");

    cmdParser.setReturnValue(toString(nLen));
    g_logger.info("Seeked portion read.");

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
    auto vParVal = cmdParser.getParsedParameterValue("samples");

    if (vParVal.size())
        nSamples = vParVal.getAsScalarInt();

    // Indices lesen
    DataAccessParser accessParser = cmdParser.getExprAsDataObject();

    std::unique_ptr<Memory> _mem(extractRange(cmdParser.getCommandLine(), accessParser, 2, true));

    if (!_mem)
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, cmdParser.getCommandLine(), accessParser.getDataObject() + "(", accessParser.getDataObject() + "()");

    if (!_mem->isValueLike(VectorIndex(0, _mem->getCols()-1)))
        throw SyntaxError(SyntaxError::WRONG_COLUMN_TYPE, cmdParser.getCommandLine(), accessParser.getDataObject()+"(", accessParser.getDataObject());

    sColHeaders[0] = _mem->getHeadLineElement(0) + "\n(regularized)";
    sColHeaders[1] = _mem->getHeadLineElement(1) + "\n(regularized)";

    long long int nLines = _mem->getLines();

    dXmin = _mem->min(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(0)).real();
    dXmax = _mem->max(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(0)).real();

    // Create splines
    tk::spline _spline;
    _spline.set_points(mu::real(_mem->readMem(VectorIndex(0, nLines-1), VectorIndex(0)).as_cmplx_vector()),
                       mu::real(_mem->readMem(VectorIndex(0, nLines-1), VectorIndex(1)).as_cmplx_vector()), false);

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
    mu::Array vPulseProperties;
    double dXmin = NAN, dXmax = NAN;
    double dSampleSize = NAN;

    // Indices lesen
    DataAccessParser accessParser = cmdParser.getExprAsDataObject();

    std::unique_ptr<Memory> _mem(extractRange(cmdParser.getCommandLine(), accessParser, 2, true));

    if (!_mem)
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, cmdParser.getCommandLine(), accessParser.getDataObject() + "(", accessParser.getDataObject() + "()");

    if (!_mem->isValueLike(VectorIndex(0, _mem->getCols()-1)))
        throw SyntaxError(SyntaxError::WRONG_COLUMN_TYPE, cmdParser.getCommandLine(), accessParser.getDataObject()+"(", accessParser.getDataObject());

    long long int nLines = _mem->getLines();

    dXmin = _mem->min(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(0)).real();
    dXmax = _mem->max(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(0)).real();

    _v.Create(nLines);

    for (long long int i = 0; i < nLines; i++)
        _v.a[i] = _mem->readMem(i, 1).getNum().asF64();

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

        for (size_t i = 0; i < vPulseProperties.size(); i++)
            NumeReKernel::printPreFmt("|   " + _lang.get("PARSERFUNCS_PULSE_TABLE_" + toString(i + 1) + "_*",
                                                         vPulseProperties[i].print(NumeReKernel::getInstance()->getSettings().getPrecision())) + "\n");

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

    auto vParVal = cmdParser.getParsedParameterValue("samples");

    if (vParVal.size())
        nSamples = std::max(vParVal.getAsScalarInt(), 0LL);

    std::string sTargetCache = cmdParser.getTargetTable(_target, "stfdat");

    // Indices lesen
    DataAccessParser _accessParser = cmdParser.getExprAsDataObject();
    _accessParser.evalIndices();
    Indices& _idx = _accessParser.getIndices();

    if (!_data.isValueLike(_accessParser.getIndices().col, _accessParser.getDataObject()))
        throw SyntaxError(SyntaxError::WRONG_COLUMN_TYPE, cmdParser.getCommandLine(), _accessParser.getDataObject()+"(",
                          _accessParser.getDataObject());

    dXmin = _data.min(_accessParser.getDataObject(), _idx.row, _idx.col.subidx(0, 1)).real();
    dXmax = _data.max(_accessParser.getDataObject(), _idx.row, _idx.col.subidx(0, 1)).real();

    _real.Create(_idx.row.size());
    _imag.Create(_idx.row.size());

    #pragma omp parallel for
    for (size_t i = 0; i < _idx.row.size(); i++)
    {
        mu::Value val = _data.getElement(_idx.row[i], _idx.col[1], _accessParser.getDataObject());
        _real.a[i] = val.as_cmplx().real();
        _imag.a[i] = val.as_cmplx().imag();
    }

    if (!nSamples || nSamples > _real.GetNx())
        nSamples = _real.GetNx() / 32;

    // Tatsaechliche STFA
    _result = mglSTFA(_real, _imag, nSamples);

    dSampleSize = (dXmax - dXmin) / ((double)_result.GetNx() - 1.0);

    // Nyquist: _real.GetNx()/(dXmax-dXmin)/2.0
    dFmax = _real.GetNx() / (dXmax - dXmin) / 2.0;

    // Zielcache befuellen entsprechend der Fourier-Algorithmik

    //if (_target.row.isOpenEnd())
    //    _target.row.setRange(0, _target.row.front() + _result.GetNx() - 1);

    if (_target.col.isOpenEnd())
        _target.col.setRange(0, _target.col.front() + _result.GetNy()/2 + 1);

    _data.resizeTable(_target.col.max(), sTargetCache);

    // Write the time axis
    for (int i = 0; i < _result.GetNx(); i++)
        _data.writeToTable(_target.row[i], _target.col.front(), sTargetCache, dXmin + i * dSampleSize);

    // Define headline
    _data.setHeadLineElement(_target.col.front(), sTargetCache, _data.getHeadLineElement(_idx.col.front(), _accessParser.getDataObject()));
    dSampleSize = 2 * (dFmax - dFmin) / ((double)_result.GetNy() - 1.0);

    // Write the frequency axis
    for (int i = 0; i < _result.GetNy() / 2; i++)
        _data.writeToTable(_target.row[i], _target.col[1], sTargetCache, dFmin + i * dSampleSize); // Fourier f

    _data.convertColumns(sTargetCache, _target.col.subidx(0, 2), "value.f64");

    // Define headline
    _data.setHeadLineElement(_target.col[1], sTargetCache, "f [Hz]");

    Memory* _mem = _data.getTable(sTargetCache);

    // Write first row in slow mode for pre-allocation
    for (int j = 0; j < _result.GetNy() / 2; j++)
    {
        if (_target.col[j+2] == VectorIndex::INVALID)
            continue;

        // Pre-allocate, first row index has to be valid
        _mem->writeData(_target.row.front(), _target.col[j+2], _result[(j + _result.GetNy() / 2)*_result.GetNx()]);

        // Update the headline
        _mem->setHeadLineElement(_target.col[j+2], "A(f(" + toString(j + 1) + "))");
    }

    _data.convertColumns(sTargetCache, _target.col.subidx(2, _result.GetNy()), "value.f64");

    // Write the STFA map in fast mode
    #pragma omp parallel for
    for (int j = 0; j < _result.GetNy() / 2; j++)
    {
        if (_target.col[j+2] == VectorIndex::INVALID)
            continue;

        for (int i = 1; i < _result.GetNx(); i++)
        {
            if (_target.row[i] == VectorIndex::INVALID)
                break;

            _mem->writeDataDirectUnsafe(_target.row[i], _target.col[j+2], _result[i + (j + _result.GetNy() / 2)*_result.GetNx()]);
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
    mu::Array vParVal = cmdParser.getParsedParameterValue("minval");

    if (vParVal.size())
        dLevel = vParVal.front().getNum().asF64();

    vParVal = cmdParser.getParsedParameterValue("attract");

    if (vParVal.size() > 1)
    {
        dAttrX = fabs(vParVal[0].getNum().asCF64());
        dAttrY = fabs(vParVal[1].getNum().asCF64());
    }
    else if (vParVal.size())
        dAttrY = fabs(vParVal[0].getNum().asCF64());

    vParVal = cmdParser.getParsedParameterValue("minlen");

    if (vParVal.size())
        dMinLen = fabs(vParVal.front().getNum().asCF64());

    Indices _target;
    std::string sTargetCache = cmdParser.getTargetTable(_target, "detectdat");

    // Indices lesen
    DataAccessParser accessParser = cmdParser.getExprAsDataObject();
    Indices& _idx = accessParser.getIndices();

    if (_idx.row.isOpenEnd())
        _idx.row.setRange(0, _data.getLines(accessParser.getDataObject())-1);

    if (_idx.col.isOpenEnd())
        _idx.col.setRange(0, _idx.col.front() + _data.getLines(accessParser.getDataObject(), true)
                                              - _data.getAppendedZeroes(_idx.col[1], accessParser.getDataObject()) + 1);

    if (!_data.isValueLike(_idx.col, accessParser.getDataObject()))
        throw SyntaxError(SyntaxError::WRONG_COLUMN_TYPE, cmdParser.getCommandLine(),
                          accessParser.getDataObject()+"(", accessParser.getDataObject());

    // Get x and y axis for the final scaling
    mu::Array vX = _data.getElement(_idx.row, _idx.col.subidx(0, 1), accessParser.getDataObject());
    mu::Array vY = _data.getElement(_idx.row, _idx.col.subidx(1, 1), accessParser.getDataObject());

    _idx.col = _idx.col.subidx(2);

    // Get the data
    std::unique_ptr<Memory> _mem(extractRange(cmdParser.getCommandLine(), accessParser, 100));

    if (!_mem)
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, cmdParser.getCommandLine(), accessParser.getDataObject() + "(", accessParser.getDataObject() + "()");

    long long int nLines = _mem->getLines();
    long long int nCols = _mem->getCols();

    // Restrict the attraction to not exceed the axis range
    dAttrX = min(dAttrX, (double)nLines);
    dAttrY = min(dAttrY, (double)nCols);

    // Use minimal data value if level is NaN
    if (isnan(dLevel))
        dLevel = _mem->min(VectorIndex(0, VectorIndex::OPEN_END), VectorIndex(0, VectorIndex::OPEN_END)).real();

    _mData.Create(nLines, nCols);

    // Copy the data to the mglData object
    for (long long int i = 0; i < nLines; i++)
    {
        for (long long int j = 0; j < nCols; j++)
        {
            _mData.a[i+j*nLines] = _mem->readMem(i, j).getNum().asF64();
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
        if (_target.row.size() <= (size_t)i)
            break;

        for (int j = 0; j < _res.GetNx(); j++)
        {
            if (_target.col.size() <= (size_t)j)
                break;

            if (!j)
                _data.writeToTable(_target.row[i], _target.col[j], sTargetCache,
                                   interpolateToGrid(mu::real(vX.as_cmplx_vector()), _res.a[j+i*_res.GetNx()]));
            else
                _data.writeToTable(_target.row[i], _target.col[j], sTargetCache,
                                   interpolateToGrid(mu::real(vY.as_cmplx_vector()), _res.a[j+i*_res.GetNx()]));
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

    tk::spline _spline;
    vector<double> xVect, yVect;

    DataAccessParser accessParser = cmdParser.getExprAsDataObject();

    std::unique_ptr<Memory> _mem(extractRange(cmdParser.getCommandLine(), accessParser, 2, true));

    if (!_mem)
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, cmdParser.getCommandLine(), accessParser.getDataObject() + "(", accessParser.getDataObject() + "()");

    if (!_mem->isValueLike(VectorIndex(0, _mem->getCols()-1)))
        throw SyntaxError(SyntaxError::WRONG_COLUMN_TYPE, cmdParser.getCommandLine(), accessParser.getDataObject()+"(", accessParser.getDataObject());

    int nLines = _mem->getLines();

    if (nLines < 2)
        throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, cmdParser.getCommandLine(), accessParser.getDataObject());

    if (_mem->getCols() < 2)
        throw SyntaxError(SyntaxError::TOO_FEW_COLS, cmdParser.getCommandLine(), accessParser.getDataObject());

    for (int i = 0; i < nLines; i++)
    {
        xVect.push_back(_mem->readMem(i, 0).getNum().asF64());
        yVect.push_back(_mem->readMem(i, 1).getNum().asF64());
    }

    // Set the points for the spline to calculate
    if (!_spline.set_points(xVect, yVect))
        throw SyntaxError(SyntaxError::CANNOT_SORT_CACHE, cmdParser.getCommandLine(), accessParser.getDataObject());

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

    mu::Array vParVal = cmdParser.getParsedParameterValue("alpha");

    if (vParVal.size())
        dAlpha = -vParVal.front().getNum().asF64() / 180.0 * M_PI; // deg2rad and change orientation for mathematical positive rotation

    _accessParser.getIndices().row.setOpenEndIndex(_data.getLines(_accessParser.getDataObject())-1);
    _accessParser.getIndices().col.setOpenEndIndex(_data.getCols(_accessParser.getDataObject())-1);

    if (!_data.isValueLike(_accessParser.getIndices().col, _accessParser.getDataObject()))
        throw SyntaxError(SyntaxError::WRONG_COLUMN_TYPE, cmdParser.getCommandLine(),
                          _accessParser.getDataObject()+"(", _accessParser.getDataObject());

    // Handle the image and datagrid cases
    if (cmdParser.getCommand() == "imrot" || cmdParser.getCommand() == "gridrot")
    {
        if (cmdParser.getCommand() == "gridrot")
        {
            source_x = mu::real(_data.getElement(_accessParser.getIndices().row, _accessParser.getIndices().col.subidx(0, 1),
                                                 _accessParser.getDataObject()).as_cmplx_vector());
            source_y = mu::real(_data.getElement(_accessParser.getIndices().row, _accessParser.getIndices().col.subidx(1, 1),
                                                 _accessParser.getDataObject()).as_cmplx_vector());

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

    // Remove obsolete entries (needed since new memory model)
    _source->shrink();

    // Get the edges
    Point topLeft(0, 0);
    Point topRightS(_source->getCols(false), 0);
    Point topRightP(_source->getCols(false)-1, 0);
    Point bottomLeftS(0, _source->getLines(false));
    Point bottomLeftP(0, _source->getLines(false)-1);
    Point bottomRightS(_source->getCols(false), _source->getLines(false));
    Point bottomRightP(_source->getCols(false)-1, _source->getLines(false)-1);

    // get the rotation origin
    Point origin = (bottomRightS + topLeft) / 2.0;

    // Calculate their final positions
    topLeft.rotate(dAlpha, origin);
    topRightS.rotate(dAlpha, origin);
    topRightP.rotate(dAlpha, origin);
    bottomLeftS.rotate(dAlpha, origin);
    bottomLeftP.rotate(dAlpha, origin);
    bottomRightS.rotate(dAlpha, origin);
    bottomRightP.rotate(dAlpha, origin);

    // Calculate the final image extent
    double topS   = std::min(topLeft.y, std::min(topRightS.y, std::min(bottomLeftS.y, bottomRightS.y)));
    double topP   = std::min(topLeft.y, std::min(topRightP.y, std::min(bottomLeftP.y, bottomRightP.y)));
    double bot    = std::max(topLeft.y, std::max(topRightS.y, std::max(bottomLeftS.y, bottomRightS.y)));
    double leftS  = std::min(topLeft.x, std::min(topRightS.x, std::min(bottomLeftS.x, bottomRightS.x)));
    double leftP  = std::min(topLeft.x, std::min(topRightP.x, std::min(bottomLeftP.x, bottomRightP.x)));
    double right  = std::max(topLeft.x, std::max(topRightS.x, std::max(bottomLeftS.x, bottomRightS.x)));

    int rows = ceil(bot - topS);
    int cols = ceil(right - leftS);

    // Insert the axes, if necessary
    if (cmdParser.getCommand() == "imrot")
    {
        // Write the x axis
        for (int i = 0; i < rows; i++)
        {
            if (_idx.row.size() <= (size_t)i)
                break;

            _data.writeToTable(_idx.row[i], _idx.col[0], sTargetTable, i+1);
        }

        // Write the y axis
        for (int i = 0; i < cols; i++)
        {
            if (_idx.row.size() <= (size_t)i)
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
            if (_idx.row.size() <= (size_t)i)
                break;

            Point p(i+topP, origin.y);
            p.rotate(-dAlpha, origin);

            p.x = interpolateToGrid(source_x, p.x, true);
            p.y = interpolateToGrid(source_y, p.y, true);

            p.rotate(dAlpha, origin_axis);

            _data.writeToTable(_idx.row[i], _idx.col[0], sTargetTable, p.x);
        }

        for (int j = 0; j < cols; j++)
        {
            if (_idx.row.size() <= (size_t)j)
                break;

            Point p(origin.x, j+leftP);
            p.rotate(-dAlpha, origin);

            p.x = interpolateToGrid(source_x, p.x, true);
            p.y = interpolateToGrid(source_y, p.y, true);

            p.rotate(dAlpha, origin_axis);

            _data.writeToTable(_idx.row[j], _idx.col[1], sTargetTable, p.y);
        }

        _idx.col = _idx.col.subidx(2);
    }

    Memory* _table = _data.getTable(sTargetTable);

    // Prepare the needed number of columns
    _table->resizeMemory(-1, _idx.col.subidx(0, cols).max()+1);

    // Calculate the rotated grid
    #pragma omp parallel for
    for (int j = 0; j < cols; j++)
    {
        if (_idx.col.size() <= (size_t)j)
            continue;

        for (int i = 0; i < rows; i++)
        {
            if (_idx.row.size() <= (size_t)i)
                break;

            // Create a point in rotated source coordinates
            // and rotate it backwards
            Point p(j + leftP, i + topP);
            p.rotate(-dAlpha, origin);

            // Store the interpolated value in target coordinates
            _table->writeDataDirect(_idx.row[i], _idx.col[j], _source->readMemInterpolated(p.y, p.x));
        }
    }

    // clear the memory instance
    delete _source;
    _table->markModified();
    _data.shrink(sTargetTable);
    g_logger.debug("Dataset rotated.");

    if (NumeReKernel::getInstance()->getSettings().systemPrints())
        NumeReKernel::print(_lang.get("BUILTIN_CHECKKEYWORD_ROTATETABLE_SUCCESS", sTargetTable));
}


// Forward declaration to make this function usable by the
// particle swarm optimizer (needs randomness)
mu::Array rndfnc_Random(const mu::Array& vRandMin, const mu::Array& vRandMax, const mu::Array& n);


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
    mu::Array vParVal = cmdParser.getParsedParameterValue("particles");

    if (vParVal.size())
        nNumParticles = vParVal.getAsScalarInt();

    vParVal = cmdParser.getParsedParameterValue("iter");

    if (vParVal.size())
        nMaxIterations = vParVal.getAsScalarInt();

    // Set the expression
    _parser.SetExpr(cmdParser.getExprAsMathExpression(true));

    // Determine intervals and dimensionality
    if (!ivl.size())
        ivl.intervals.push_back(Interval(-10.0, 10.0));

    nDims = ivl.size();

    // Restrict to 4 dimensions, because there are
    // only 4 default variables
    nDims = std::min((size_t)4u, nDims);

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

    std::vector<std::vector<double>> vPos;
    std::vector<std::vector<double>> vBest;
    std::vector<std::vector<double>> vVel;
    std::vector<double> vFunctionValues;
    std::vector<double> vBestValues;

    vPos.resize(nDims);
    vBest.resize(nDims);
    vVel.resize(nDims);

    // Prepare initial vectors
    for (size_t i = 0; i < nNumParticles; i++)
    {
        for (size_t j = 0; j < nDims; j++)
        {
            vPos[j].push_back(rndfnc_Random(mu::Value(ivl[j].min()), mu::Value(ivl[j].max()), mu::Array()).front().getNum().asF64());
            vVel[j].push_back(rndfnc_Random(mu::Value(ivl[j].min()), mu::Value(ivl[j].max()), mu::Array()).front().getNum().asF64()/5.0);

            _defVars.vValue[j][0] = mu::Value(vPos[j].back());
        }

        vFunctionValues.push_back(_parser.Eval().front().getNum().asF64());
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
                vVel[n][j] += rndfnc_Random(mu::Value(0), mu::Value(fRandRange), mu::Array()).front().getNum().asF64() * (vBest[n][j] - vPos[n][j])
                    + rndfnc_Random(mu::Value(0), mu::Value(fRandRange), mu::Array()).front().getNum().asF64() * (vBest[n][nGlobalBest] - vPos[n][j]);

                // Update positions
                vPos[n][j] += fAdaptiveVelFactor * vVel[n][j];

                // Restrict to interval boundaries
                vPos[n][j] = std::max(ivl[n].min(), std::min(vPos[n][j], ivl[n].max()));

                // Update the corresponding default variable
                _defVars.vValue[n][0] = mu::Value(vPos[n][j]);
            }

            // Recalculate the function value
            vFunctionValues[j] = _parser.Eval().front().getNum().asF64();

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
    mu::Array vRes;

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
        std::string sUserName = cmdParser.getParsedParameterValueAsString("usr", "", true);
        std::string sPassword = cmdParser.getParsedParameterValueAsString("pwd", "", true);
        std::string sPayLoad = cmdParser.getParsedParameterValueAsString("payload", "", true);
        std::vector<std::string> httpHeader;

        if (cmdParser.hasParam("header"))
            httpHeader = cmdParser.getParsedParameterValue("header").as_str_vector();

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
                size_t bytes = url::put(sUrl, sFileName, sUserName, sPassword, httpHeader);
                cmdParser.setReturnValue(mu::Value(mu::Numerical(bytes)));
            }
            else
            {
                // Get the response from the server
                std::string sUrlResponse = url::get(sUrl, sUserName, sPassword, httpHeader);

                // Get the file parameter value
                sFileName = cmdParser.getFileParameterValueForSaving(sFileName.substr(sFileName.rfind('.')), "<savepath>", sFileName);

                // Open the file binary and clean it
                std::ofstream file(sFileName, std::ios_base::trunc | std::ios_base::binary);

                // Stream the response to the file
                if (file.good())
                {
                    file << sUrlResponse;
                    cmdParser.setReturnValue(mu::Value(mu::Numerical(sUrlResponse.length())));
                }
                else
                    throw SyntaxError(SyntaxError::CANNOT_OPEN_TARGET, cmdParser.getCommandLine(), sFileName, sFileName);
            }
        }
        else if (sPayLoad.length())
        {
            // Get the response from the server
            std::string sUrlResponse = url::post(sUrl, sUserName, sPassword, httpHeader, sPayLoad);

            // Replace all masked characters in the return value
            replaceAll(sUrlResponse, "\r\n", "\n");
            cmdParser.setReturnValue(mu::Value(sUrlResponse));
        }
        else
        {
            // Get the response from the server
            std::string sUrlResponse = url::get(sUrl, sUserName, sPassword, httpHeader);

            // Replace all masked characters in the return value
            replaceAll(sUrlResponse, "\r\n", "\n");
            cmdParser.setReturnValue(mu::Value(sUrlResponse));
        }
    }
    catch (url::Error& e)
    {
        throw SyntaxError(SyntaxError::URL_ERROR, cmdParser.getCommandLine(), cmdParser.parseExprAsString(), e.what());
    }
}


/////////////////////////////////////////////////
/// \brief Handle mail actions based upon the
/// command line arguments for the command "mail".
///
/// \param cmdParser CommandLineParser&
/// \return void
///
/////////////////////////////////////////////////
void mailClient(CommandLineParser& cmdParser)
{
    // Handle mail actions
    try
    {
        // Get body, smtp server, username and password
        std::string sBody = cmdParser.parseExprAsString();
        std::string sServer = cmdParser.getParsedParameterValueAsString("server", "", true);
        std::string sUserName = cmdParser.getParsedParameterValueAsString("usr", "", true);
        std::string sPassword = cmdParser.getParsedParameterValueAsString("pwd", "", true);
        std::string sFromMail = cmdParser.getParsedParameterValueAsString("from", "", true);
        std::string sSubject = cmdParser.getParsedParameterValueAsString("subject", "", true);

        mu::Array rcpt;
        mu::Array cc;
        mu::Array bcc;

        if (cmdParser.hasParam("rcpt"))
            rcpt = cmdParser.getParsedParameterValue("rcpt");

        if (cmdParser.hasParam("cc"))
            cc = cmdParser.getParsedParameterValue("cc");

        if (cmdParser.hasParam("bcc"))
            bcc = cmdParser.getParsedParameterValue("bcc");

        // Remove all CRLF combinations first
        replaceAll(sBody, "\r\n", "\n");

        // Ensure that the body does not exceed the RFC 5322
        // line length limit
        std::vector<std::string> vLines = splitIntoLines(sBody, 78, true, 0, 0);
        sBody.clear();

        for (const auto& line : vLines)
        {
            if (sBody.length())
                sBody += "\n";

            sBody += line;
        }

        // Prepare the actual mail body from the string
        std::string sHeader;
        sHeader = "Date: " + formatRfc5322(sys_time_now()) + "\n";

        if (sFromMail.length())
            sHeader += "From: NumeRe <" + sFromMail + ">\n";

        if (rcpt.size())
            sHeader += "To: <" + rcpt.printJoined(">,<") + ">\n";

        if (cc.size())
            sHeader += "Cc: <" + cc.printJoined(">,<") + ">\n";

        if (bcc.size())
            sHeader += "Bcc: <" + bcc.printJoined(">,<") + ">\n";

        if (sFromMail.length())
            sHeader += "Message-ID: <" + getUuidV4() + sFromMail.substr(sFromMail.find('@')) + ">\n";

        sHeader += "Subject: " + sSubject + "\n";

        // Insert an empty line to divide headers from body, see RFC 5322
        sBody = sHeader + "\n" + sBody;

        // Convert all line breaks into CRLF combinations
        replaceAll(sBody, "\n", "\r\n");

        // Combine all recipient types into a common recipients list
        if (cc.size())
            rcpt.insert(rcpt.end(), cc.begin(), cc.end());

        if (bcc.size())
            rcpt.insert(rcpt.end(), bcc.begin(), bcc.end());

        // Validate all available recipient addresses
        static const std::regex mailValidator("^[^\\s@]+@([^\\s@.,]+\\.)+[^\\s@.,]{2,}$");

        if (sFromMail.length() && !std::regex_match(sFromMail, mailValidator))
            throw url::Error(sFromMail + " is not a valid mail address.");

        for (const mu::Value& mail : rcpt)
        {
            if (!std::regex_match(mail.printVal(), mailValidator))
                throw url::Error(mail.printVal() + " is not a valid mail address.");
        }

        // Send the mail to the recipients
        size_t bytes = url::sendMail(sServer, sBody, sUserName, sPassword, sFromMail, rcpt.as_str_vector());
        cmdParser.setReturnValue(mu::Value(mu::Numerical(bytes)));
    }
    catch (url::Error& e)
    {
        throw SyntaxError(SyntaxError::MAIL_ERROR, cmdParser.getCommandLine(), cmdParser.parseExprAsString(), e.what());
    }
}



