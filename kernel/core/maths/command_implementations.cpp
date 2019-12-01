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

#include "command_implementations.hpp"
#include "parser_functions.hpp"
#include "matrixoperations.hpp"
#include "spline.h"
#include "wavelet.hpp"
#include "../../kernel.hpp"

#define TRAPEZOIDAL 1
#define SIMPSON 2

Integration_Vars parser_iVars;
static double localizeExtremum(string& sCmd, double* dVarAdress, Parser& _parser, const Settings& _option, double dLeft, double dRight, double dEps = 1e-10, int nRecursion = 0);
static double localizeZero(string& sCmd, double* dVarAdress, Parser& _parser, const Settings& _option, double dLeft, double dRight, double dEps = 1e-10, int nRecursion = 0);
static vector<size_t> getSamplesForDatagrid(const string& sCmd, const string& sZVals, size_t nSamples, Parser& _parser, Datafile& _data, const Settings& _option);
static vector<double> extractVectorForDatagrid(const string& sCmd, string& sVectorVals, const string& sZVals, size_t nSamples, Parser& _parser, Datafile& _data, const Settings& _option);
static void expandVectorToDatagrid(vector<double>& vXVals, vector<double>& vYVals, vector<vector<double>>& vZVals, size_t nSamples_x, size_t nSamples_y);


/////////////////////////////////////////////////
/// \brief This static function performs an
/// integration step using a trapezoidal
/// approximation algorithm.
///
/// \param x double&
/// \param dx double
/// \param upperBoundary double
/// \param vResult vector<double>&
/// \param vFunctionValues vector<double>&
/// \param bReturnFunctionPoints bool
/// \return void
///
/////////////////////////////////////////////////
static void integrationstep_trapezoidal(double& x, double dx, double upperBoundary, vector<double>& vResult, vector<double>& vFunctionValues, bool bReturnFunctionPoints)
{
    x += dx;
    int nResults;
    value_type* v = NumeReKernel::getInstance()->getParser().Eval(nResults);

    // Evaluate the current integration step for each of the
    // defined functions
    for (int i = 0; i < nResults; i++)
    {
        if (x > upperBoundary && isnan(v[i]))
            v[i] = 0.0;

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

            break;
        }
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
/// \param dx double
/// \param upperBoundary double
/// \param vResult vector<double>&
/// \param vFunctionValues vector<double>&
/// \param bReturnFunctionPoints bool
/// \return void
///
/////////////////////////////////////////////////
static void integrationstep_simpson(double& x, double dx, double upperBoundary, vector<double>& vResult, vector<double>& vFunctionValues, bool bReturnFunctionPoints)
{
    // Evaluate the intermediate function value
    x += dx / 2.0;
    int nResults;
    value_type* v = NumeReKernel::getInstance()->getParser().Eval(nResults);

    for (int i = 0; i < nResults; i++)
    {
        if (x > upperBoundary && isnan(v[i]))
            v[i] = 0.0;
    }

    vector<double> vInter(v, v+nResults);

    // Evaluate the end function value
    x += dx / 2.0;
    v = NumeReKernel::getInstance()->getParser().Eval(nResults);

    for (int i = 0; i < nResults; i++)
    {
        if (x > upperBoundary && isnan(v[i]))
            v[i] = 0.0;

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

            break;
        }
    }

    // Set the last sample as the first one
    vFunctionValues.assign(v, v+nResults);
}


/////////////////////////////////////////////////
/// \brief This static function integrates single
/// dimension data.
///
/// \param sIntegrationExpression string&
/// \param sParams string&
/// \return vector<double>
///
/////////////////////////////////////////////////
static vector<double> integrateSingleDimensionData(string& sIntegrationExpression, string& sParams)
{
    value_type* v = nullptr;
    int nResults = 0;
    string sLowerBoundary;
    double x0 = 0.0;
    double x1 = 0.0;
    bool bReturnFunctionPoints = false;
    bool bCalcXvals = false;

    vector<double> vResult;

    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    // Extract the integration interval
    if (sParams.length() && findParameter(sParams, "x", '='))
    {
        sLowerBoundary = getArgAtPos(sParams, findParameter(sParams, "x", '=') + 1);

        // Replace the colon with a comma
        if (sLowerBoundary.find(':') != string::npos)
            sLowerBoundary.replace(sLowerBoundary.find(':'), 1, ",");

        // Set the interval expression and evaluate it
        _parser.SetExpr(sLowerBoundary);
        v = _parser.Eval(nResults);

        if (nResults > 1)
            x1 = v[1];

        x0 = v[0];
    }

    // Are the samples of the integral desired?
    if (sParams.length() && findParameter(sParams, "points"))
        bReturnFunctionPoints = true;

    // Are the corresponding x values desired?
    if (sParams.length() && findParameter(sParams, "xvals"))
        bCalcXvals = true;

    // Get table name and the corresponding indices
    string sDatatable = sIntegrationExpression.substr(0, sIntegrationExpression.find('('));
    Indices _idx = getIndices(sIntegrationExpression, _parser, _data, _option);

    // Calculate the integral of the data set
    evaluateIndices(sDatatable, _idx, _data);

    // The indices are vectors
    //
    // If it is a single column or row, then we simply
    // summarize its contents, otherwise we calculate the
    // integral with the trapezoidal method
    if (_idx.row.size() == 1 || _idx.col.size() == 1)
        vResult.push_back(_data.sum(sDatatable, _idx.row, _idx.col));
    else
    {
        Datafile _cache;

        // Copy the data
        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            _cache.writeToTable(i, 0, "cache", _data.getElement(_idx.row[i], _idx.col[0], sDatatable));
            _cache.writeToTable(i, 1, "cache", _data.getElement(_idx.row[i], _idx.col[1], sDatatable));
        }

        // Sort the data
        _cache.sortElements("cache -sort c=1[2]");
        double dResult = 0.0;
        long long int j = 1;

        // Calculate the integral by jumping over NaNs
        for (long long int i = 0; i < _cache.getLines("cache", false) - 1; i++) //nan-suche
        {
            j = 1;

            if (!_cache.isValidEntry(i, 1, "cache"))
                continue;

            while (!_cache.isValidEntry(i + j, 1, "cache") && i + j < _cache.getLines("cache", false) - 1)
                j++;

            if (!_cache.isValidEntry(i + j, 0, "cache") || !_cache.isValidEntry(i + j, 1, "cache"))
                break;

            if (sLowerBoundary.length() && x0 > _cache.getElement(i, 0, "cache"))
                continue;

            if (sLowerBoundary.length() && x1 < _cache.getElement(i + j, 0, "cache"))
                break;

            // Calculate either the integral, its samples or the corresponding x values
            if (!bReturnFunctionPoints && !bCalcXvals)
                dResult += (_cache.getElement(i, 1, "cache") + _cache.getElement(i + j, 1, "cache")) / 2.0 * (_cache.getElement(i + j, 0, "cache") - _cache.getElement(i, 0, "cache"));
            else if (bReturnFunctionPoints && !bCalcXvals)
            {
                if (vResult.size())
                    vResult.push_back((_cache.getElement(i, 1, "cache") + _cache.getElement(i + j, 1, "cache")) / 2.0 * (_cache.getElement(i + j, 0, "cache") - _cache.getElement(i, 0, "cache")) + vResult.back());
                else
                    vResult.push_back((_cache.getElement(i, 1, "cache") + _cache.getElement(i + j, 1, "cache")) / 2.0 * (_cache.getElement(i + j, 0, "cache") - _cache.getElement(i, 0, "cache")));
            }
            else
                vResult.push_back(_cache.getElement(i + j, 0, "cache"));
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
/// \param sCmd const string&
/// \param _data Datafile&
/// \param _parser Parser&
/// \param _option const Settings&
/// \param _functions Define&
/// \return vector<double>
///
/////////////////////////////////////////////////
vector<double> integrate(const string& sCmd, Datafile& _data, Parser& _parser, const Settings& _option, Define& _functions)
{
	string sParams = "";        // Parameter-string
	string sIntegrationExpression;
	string sPrecision;
	string sLowerBoundary;
	string sUpperBoundary;
	value_type* v = 0;
	int nResults = 0;
	vector<double> vResult;   // Ausgabe-Wert
	vector<double> vFunctionValues; // Werte an der Stelle n und n+1
	bool bNoIntVar = false;     // Boolean: TRUE, wenn die Funktion eine Konstante der Integration ist
	bool bLargeInterval = false;    // Boolean: TRUE, wenn ueber ein grosses Intervall integriert werden soll
	bool bReturnFunctionPoints = false;
	bool bCalcXvals = false;
	int nSign = 1;              // Vorzeichen, falls die Integrationsgrenzen getauscht werden muessen
	unsigned int nMethod = TRAPEZOIDAL;    // 1 = trapezoidal, 2 = simpson

	sPrecision = "1e-3";
	parser_iVars.vValue[0][3] = 1e-3;
	double& x = parser_iVars.vValue[0][0];
	double& x0 = parser_iVars.vValue[0][1];
	double& x1 = parser_iVars.vValue[0][2];
	double& dx = parser_iVars.vValue[0][3];

	// It's not possible to calculate the integral of a string expression
	if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
		throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "integrate");

	// Separate function from the parameter string
	if (sCmd.find("-set") != string::npos)
	{
		sParams = sCmd.substr(sCmd.find("-set"));
		sIntegrationExpression = sCmd.substr(9, sCmd.find("-set") - 9);
	}
	else if (sCmd.find("--") != string::npos)
	{
		sParams = sCmd.substr(sCmd.find("--"));
		sIntegrationExpression = sCmd.substr(9, sCmd.find("--") - 9);
	}
	else if (sCmd.length() > 9)
		sIntegrationExpression = sCmd.substr(9);

	StripSpaces(sIntegrationExpression);

	// Ensure that the function is available
	if (!sIntegrationExpression.length())
		throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, sCmd, SyntaxError::invalid_position);

	// If needed, prompt for the integration function
	if (sIntegrationExpression.length() && sIntegrationExpression.find("??") != string::npos)
		sIntegrationExpression = promptForUserInput(sIntegrationExpression);

	StripSpaces(sIntegrationExpression);

	// If the integration function contains a data object,
	// the calculation is done way different from the usual
	// integration
	if ((sIntegrationExpression.substr(0, 5) == "data(" || _data.isTable(sIntegrationExpression))
			&& getMatchingParenthesis(sIntegrationExpression) != string::npos
			&& sIntegrationExpression.find_first_not_of(' ', getMatchingParenthesis(sIntegrationExpression) + 1) == string::npos) // xvals
        return integrateSingleDimensionData(sIntegrationExpression, sParams);

	// No data set for integration
	if (sIntegrationExpression.find("{") != string::npos)
		convertVectorToExpression(sIntegrationExpression, _option);

    // Call custom defined functions
	if (sIntegrationExpression.length() && !_functions.call(sIntegrationExpression))
	{
		sIntegrationExpression = "";
		throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, sCmd, SyntaxError::invalid_position);
	}

	// Evaluate the parameters
	if (sParams.length())
	{
		int nPos = 0;

		if (findParameter(sParams, "precision", '=') || findParameter(sParams, "p", '=') || findParameter(sParams, "eps", '='))
		{
		    if (findParameter(sParams, "precision", '='))
                nPos = findParameter(sParams, "precision", '=') + 9;
		    else if (findParameter(sParams, "p", '='))
                nPos = findParameter(sParams, "p", '=') + 1;
		    else
                nPos = findParameter(sParams, "eps", '=') + 3;

			sPrecision = getArgAtPos(sParams, nPos);
			StripSpaces(sPrecision);

			if (isNotEmptyExpression(sPrecision))
			{
				_parser.SetExpr(sPrecision);
				dx = _parser.Eval();

				if (isinf(dx) || isnan(dx))
				{
					vResult.push_back(NAN);
					return vResult;
				}

				if (!dx)
					sPrecision = "";
			}
		}

		if (findParameter(sParams, "x", '='))
		{
			nPos = findParameter(sParams, "x", '=') + 1;
			sLowerBoundary = getArgAtPos(sParams, nPos);
			StripSpaces(sLowerBoundary);

			if (sLowerBoundary.find(':') != string::npos)
			{
			    sUpperBoundary = sLowerBoundary;
			    sLowerBoundary = getNextIndex(sUpperBoundary, true);

				if (isNotEmptyExpression(sLowerBoundary))
				{
					_parser.SetExpr(sLowerBoundary);

					if (isVariableInAssignedExpression(_parser, parser_iVars.sName[0]))
						sLowerBoundary = "";
					else
					{
						x0 = _parser.Eval();

						if (isinf(x0) || isnan(x0))
						{
							vResult.push_back(NAN);
							return vResult;
						}
					}
				}

				if (isNotEmptyExpression(sUpperBoundary))
				{
					_parser.SetExpr(sUpperBoundary);

					if (isVariableInAssignedExpression(_parser, parser_iVars.sName[0]))
						sUpperBoundary = "";
					else
					{
						x1 = _parser.Eval();

						if (isinf(x1) || isnan(x1))
						{
							vResult.push_back(NAN);
							return vResult;
						}
					}
				}

				if (sLowerBoundary.length() && sUpperBoundary.length() && x0 == x1)
					throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);

				if (!sLowerBoundary.length() || !sUpperBoundary.length())
					throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
			}
			else
				throw SyntaxError(SyntaxError::NO_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
		}

		if (findParameter(sParams, "method", '='))
		{
			nPos = findParameter(sParams, "method", '=') + 6;

			if (getArgAtPos(sParams, nPos) == "trapezoidal")
				nMethod = TRAPEZOIDAL;

			if (getArgAtPos(sParams, nPos) == "simpson")
				nMethod = SIMPSON;
		}

		if (findParameter(sParams, "m", '='))
		{
			nPos = findParameter(sParams, "m", '=') + 1;

			if (getArgAtPos(sParams, nPos) == "trapezoidal")
				nMethod = TRAPEZOIDAL;

			if (getArgAtPos(sParams, nPos) == "simpson")
				nMethod = SIMPSON;
		}

		if (findParameter(sParams, "steps", '='))
		{
			sPrecision = getArgAtPos(sParams, findParameter(sParams, "steps", '=') + 5);
			_parser.SetExpr(sPrecision);
			dx = (x1 - x0) / _parser.Eval();
		}

		if (findParameter(sParams, "s", '='))
		{
			sPrecision = getArgAtPos(sParams, findParameter(sParams, "s", '=') + 1);
			_parser.SetExpr(sPrecision);
			dx = (x1 - x0) / _parser.Eval();
		}

		if (findParameter(sParams, "points"))
			bReturnFunctionPoints = true;

		if (findParameter(sParams, "xvals"))
			bCalcXvals = true;
	}

	// Ensure that a function is available
	if (!sIntegrationExpression.length())
        throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, sCmd, SyntaxError::invalid_position);

	// Check, whether the expression actual depends
	// upon the integration variable
	_parser.SetExpr(sIntegrationExpression);

	if (!isVariableInAssignedExpression(_parser, parser_iVars.sName[0]))
		bNoIntVar = true;       // Nein? Dann setzen wir den Bool auf TRUE und sparen uns viel Rechnung

	_parser.Eval(nResults);
	vResult.resize(nResults, 0.0);

	// Set the calculation variables to their starting values
	vFunctionValues.resize(nResults, 0.0);

	// Ensure that interation ranges are available
	if (!sLowerBoundary.length() || !sUpperBoundary.length())
	    throw SyntaxError(SyntaxError::NO_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);

	// Exchange borders, if necessary
	if (x1 < x0)
	{
		// --> Ja? Dann tauschen wir sie fuer die Berechnung einfach aus <--
		double dTemp = x0;
		x0 = x1;
		x1 = dTemp;
		nSign *= -1; // Beachten wir das Tauschen der Grenzen durch ein zusaetzliches Vorzeichen
	}

	// Calculate the numerical integration
	if (!bNoIntVar || bReturnFunctionPoints || bCalcXvals)
	{
	    // In this case, we have to calculate the integral
	    // numerically
		if (sPrecision.length() && dx > x1 - x0)
			sPrecision = "";

        // If the precision is invalid (e.g. due to a very
        // small interval, simply guess a reasonable interval
        // here
		if (!sPrecision.length())
            dx = (x1 - x0) / 100;

		// Ensure that the precision is not negative
		if (dx < 0)
			dx *= -1;

		// Calculate the x values, if desired
		if (bCalcXvals)
		{
			x = x0;
			vResult[0] = x;

			while (x + dx < x1)
			{
				x += dx;
				vResult.push_back(x);
			}

			return vResult;
		}

		// Set the expression in the parser
		_parser.SetExpr(sIntegrationExpression);

		// Is it a large interval (then it will need more time)
		if ((x1 - x0) / dx >= 9.9e6)
			bLargeInterval = true;

		// Do not allow a very high number of integration steps
		if ((x1 - x0) / dx > 1e10)
			throw SyntaxError(SyntaxError::INVALID_INTEGRATION_PRECISION, sCmd, SyntaxError::invalid_position);

		// Set the integration variable to the lower border
		x = x0;

		// Calculate the first sample(s)
		v = _parser.Eval(nResults);
		vFunctionValues.assign(v, v+nResults);

		// Perform the actual numerical integration
		while (x + dx < x1 + dx * 1e-1)
		{
		    // Calculate the samples first
			if (nMethod == TRAPEZOIDAL)
                integrationstep_trapezoidal(x, dx, x1, vResult, vFunctionValues, bReturnFunctionPoints);
			else if (nMethod == SIMPSON)
                integrationstep_simpson(x, dx, x1, vResult, vFunctionValues, bReturnFunctionPoints);

			// Print a status value, if needed
			if (_option.getSystemPrintStatus() && bLargeInterval)
			{
				if (!bLargeInterval)
				{
					if ((int)((x - x0) / (x1 - x0) * 20) > (int)((x - dx - x0) / (x1 - x0) * 20))
						NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + toString((int)((x - x0) / (x1 - x0) * 20) * 5) + " %");
				}
				else
				{
					if ((int)((x - x0) / (x1 - x0) * 100) > (int)((x - dx - x0) / (x1 - x0) * 100))
						NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + toString((int)((x - x0) / (x1 - x0) * 100)) + " %");
				}

				if (NumeReKernel::GetAsyncCancelState())//GetAsyncKeyState(VK_ESCAPE))
				{
					NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + _lang.get("COMMON_CANCEL") + ".\n");
					throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
				}
			}
		}
	}
	else
	{
		// In this case we don't have a dependency
		// upon the integration variable. The result
		// is simply constant
		string sTemp = sIntegrationExpression;
		sIntegrationExpression.erase();

		// Apply the analytical solution
		while (sTemp.length())
			sIntegrationExpression += getNextArgument(sTemp, true) + "*" + parser_iVars.sName[0] + ",";

		sIntegrationExpression.erase(sIntegrationExpression.length() - 1, 1);

		// Calculate the integral analytically
		_parser.SetExpr(sIntegrationExpression);
		x = x1;
		v = _parser.Eval(nResults);
		vResult.assign(v, v+nResults);

		x = x0;
		v = _parser.Eval(nResults);

		for (int i = 0; i < nResults; i++)
			vResult[i] -= v[i];
	}

	// Display a success message
	if (_option.getSystemPrintStatus() && bLargeInterval)
		NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... 100 %: " + _lang.get("COMMON_SUCCESS") + "!\n");

	return vResult;
}


/////////////////////////////////////////////////
/// \brief This static function re-evaluates the
/// boundary expression and updates the internal
/// variables correspondingly.
///
/// \param sRenewBoundariesExpression const string&
/// \param y0 double&
/// \param y1 double&
/// \param sIntegrationExpression const string&
/// \return void
///
/////////////////////////////////////////////////
static void refreshBoundaries(const string& sRenewBoundariesExpression, double& y0, double& y1, const string& sIntegrationExpression)
{
    // Refresh the y boundaries, if necessary
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    double* boundaries;
    int boundaryCount;
    _parser.SetExpr(sRenewBoundariesExpression);
    boundaries = _parser.Eval(boundaryCount);

    y0 = boundaries[0];
    y1 = boundaries[1];

    _parser.SetExpr(sIntegrationExpression);
}


/////////////////////////////////////////////////
/// \brief Calculate the integral of a function
/// in two dimensions.
///
/// \param sCmd const string&
/// \param _data Datafile&
/// \param _parser Parser&
/// \param _option const Settings&
/// \param _functions Define&
/// \return vector<double>
///
/////////////////////////////////////////////////
vector<double> integrate2d(const string& sCmd, Datafile& _data, Parser& _parser, const Settings& _option, Define& _functions)
{
	string __sCmd = findCommand(sCmd).sString;
	string sParams = "";            // Parameter-string
	string sPrecision;
	string sBoundariesX[2];
	string sBoundariesY[2];
	string sRenewBoundariesExpression;
	string sIntegrationExpression;                // string fuer die zu integrierende Funktion
	value_type* v = 0;
	int nResults = 0;
	vector<double> vResult[3];      // value_type-Array, wobei vResult[0] das eigentliche Ergebnis speichert
	// und vResult[1] fuer die Zwischenergebnisse des inneren Integrals ist
	vector<double> fx_n[2][3];          // value_type-Array fuer die jeweiligen Stuetzstellen im inneren und aeusseren Integral
	bool bIntVar[2] = {true, true}; // bool-Array, das speichert, ob und welche Integrationsvariablen in sInt_Fct enthalten sind
	bool bRenewBoundaries = false;      // bool, der speichert, ob die Integralgrenzen von x oder y abhaengen
	bool bLargeArray = false;       // bool, der TRUE fuer viele Datenpunkte ist;
	int nSign = 1;                  // Vorzeichen-Integer
	unsigned int nMethod = TRAPEZOIDAL;       // trapezoidal = 1, simpson = 2

	sPrecision = "1e-3";
	parser_iVars.vValue[0][3] = 1e-3;
	parser_iVars.vValue[1][3] = 1e-3;

	double& x = parser_iVars.vValue[0][0];
	double& x0 = parser_iVars.vValue[0][1];
	double& x1 = parser_iVars.vValue[0][2];
	double& dx = parser_iVars.vValue[0][3];
	double& y = parser_iVars.vValue[1][0];
	double& y0 = parser_iVars.vValue[1][1];
	double& y1 = parser_iVars.vValue[1][2];
	double& dy = parser_iVars.vValue[1][3];

	// Strings may not be integrated
	if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
		throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "integrate");

	// Extract integration function and parameter list
	if (sCmd.find("-set") != string::npos)
	{
		sParams = sCmd.substr(sCmd.find("-set"));
		sIntegrationExpression = sCmd.substr(__sCmd.length(), sCmd.find("-set") - __sCmd.length());
	}
	else if (sCmd.find("--") != string::npos)
	{
		sParams = sCmd.substr(sCmd.find("--"));
		sIntegrationExpression = sCmd.substr(__sCmd.length(), sCmd.find("--") - __sCmd.length());
	}
	else if (sCmd.length() > __sCmd.length())
		sIntegrationExpression = sCmd.substr(__sCmd.length());

	StripSpaces(sIntegrationExpression);

	// Ensure that the integration function is available
	if (!sIntegrationExpression.length())
		throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, sCmd, SyntaxError::invalid_position);

	// Prompt for an input, if necessary
	if (sIntegrationExpression.length() && sIntegrationExpression.find("??") != string::npos)
		sIntegrationExpression = promptForUserInput(sIntegrationExpression);

	// Expand the integration function, if necessary
	if (sIntegrationExpression.find("{") != string::npos)
		convertVectorToExpression(sIntegrationExpression, _option);

	// Try to call custom functions
	if (sIntegrationExpression.length() && !_functions.call(sIntegrationExpression))
		throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, sCmd, SyntaxError::invalid_position);

    // Evaluate the parameters
	if (sParams.length())
	{
		int nPos = 0;

		if (findParameter(sParams, "precision", '=') || findParameter(sParams, "p", '=') || findParameter(sParams, "eps", '='))
		{
		    if (findParameter(sParams, "precision", '='))
                nPos = findParameter(sParams, "precision", '=') + 9;
		    else if (findParameter(sParams, "p", '='))
                nPos = findParameter(sParams, "p", '=') + 1;
		    else
                nPos = findParameter(sParams, "eps", '=') + 3;

			sPrecision = getArgAtPos(sParams, nPos);
			StripSpaces(sPrecision);

			if (isNotEmptyExpression(sPrecision))
			{
				_parser.SetExpr(sPrecision);
				dx = _parser.Eval();

				if (isinf(dx) || isnan(dx))
				{
					vResult[0].push_back(NAN);
					return vResult[0];
				}

				if (!dx)
					sPrecision = "";
                else
                    dy = dx;
			}
		}

		if (findParameter(sParams, "x", '='))
		{
			nPos = findParameter(sParams, "x", '=') + 1;
			sBoundariesX[0] = getArgAtPos(sParams, nPos);
			StripSpaces(sBoundariesX[0]);

			if (sBoundariesX[0].find(':') != string::npos)
			{
			    sBoundariesX[1] = sBoundariesX[0];
			    sBoundariesX[0] = getNextIndex(sBoundariesX[1], true);

				if (isNotEmptyExpression(sBoundariesX[0]))
				{
					_parser.SetExpr(sBoundariesX[0]);

					if (isVariableInAssignedExpression(_parser, parser_iVars.sName[0]) || isVariableInAssignedExpression(_parser, parser_iVars.sName[1]))
						sBoundariesX[0] = "";
					else
					{
						x0 = _parser.Eval();

						if (isinf(x0) || isnan(x0))
						{
							vResult[0].push_back(NAN);
							return vResult[0];
						}
					}
				}

				if (isNotEmptyExpression(sBoundariesX[1]))
				{
					_parser.SetExpr(sBoundariesX[1]);

					if (isVariableInAssignedExpression(_parser, parser_iVars.sName[0]) || isVariableInAssignedExpression(_parser, parser_iVars.sName[1]))
						sBoundariesX[1] = "";
					else
					{
						x1 = _parser.Eval();

						if (isinf(x1) || isnan(x1))
						{
							vResult[0].push_back(NAN);
							return vResult[0];
						}
					}
				}

				if (sBoundariesX[0].length() && sBoundariesX[1].length() && x0 == x1)
					throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);

				if (!sBoundariesX[0].length() || !sBoundariesX[1].length())
					throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
			}
			else
				throw SyntaxError(SyntaxError::NO_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
		}

		if (findParameter(sParams, "y", '='))
		{
			nPos = findParameter(sParams, "y", '=') + 1;
			sBoundariesY[0] = getArgAtPos(sParams, nPos);
			StripSpaces(sBoundariesY[0]);

			if (sBoundariesY[0].find(':') != string::npos)
			{
			    sBoundariesY[1] = sBoundariesY[0];
			    sBoundariesY[0] = getNextIndex(sBoundariesY[1], true);

				if (isNotEmptyExpression(sBoundariesY[0]))
				{
					_parser.SetExpr(sBoundariesY[0]);

					if (isVariableInAssignedExpression(_parser, parser_iVars.sName[1]))
					{
						sBoundariesY[0] = "";
					}
					else
					{
						y0 = _parser.Eval();

						if (isinf(y0) || isnan(y0))
						{
							vResult[0].push_back(NAN);
							return vResult[0];
						}
					}
				}

				if (isNotEmptyExpression(sBoundariesY[1]))
				{
					_parser.SetExpr(sBoundariesY[1]);

					if (isVariableInAssignedExpression(_parser, parser_iVars.sName[1]))
						sBoundariesY[1] = "";
					else
					{
						y1 = _parser.Eval();

						if (isinf(y1) || isnan(y1))
						{
							vResult[0].push_back(NAN);
							return vResult[0];
						}
					}
				}

				if (sBoundariesY[0].length() && sBoundariesY[1].length() && y0 == y1)
					throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);

				if (!sBoundariesY[0].length() || !sBoundariesY[1].length())
					throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
			}
			else
				throw SyntaxError(SyntaxError::NO_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
		}

		if (findParameter(sParams, "method", '='))
		{
			nPos = findParameter(sParams, "method", '=') + 6;

			if (getArgAtPos(sParams, nPos) == "trapezoidal")
				nMethod = TRAPEZOIDAL;

			if (getArgAtPos(sParams, nPos) == "simpson")
				nMethod = SIMPSON;
		}

		if (findParameter(sParams, "m", '='))
		{
			nPos = findParameter(sParams, "m", '=') + 1;

			if (getArgAtPos(sParams, nPos) == "trapezoidal")
				nMethod = TRAPEZOIDAL;

			if (getArgAtPos(sParams, nPos) == "simpson")
				nMethod = SIMPSON;
		}

		if (findParameter(sParams, "steps", '='))
		{
			sPrecision = getArgAtPos(sParams, findParameter(sParams, "steps", '=') + 5);
			_parser.SetExpr(sPrecision);
			dx = (x1 - x0) / _parser.Eval();
			dy = dx;
		}

		if (findParameter(sParams, "s", '='))
		{
			sPrecision = getArgAtPos(sParams, findParameter(sParams, "s", '=') + 1);
			_parser.SetExpr(sPrecision);
			dx = (x1 - x0) / _parser.Eval();
			dy = dx;
		}
	}

    // Ensure that the integration function is available
	if (!sIntegrationExpression.length())
        throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, sCmd, SyntaxError::invalid_position);

	// Check, whether the expression depends upon one or both
	// integration variables
	_parser.SetExpr(sIntegrationExpression);

	if (!isVariableInAssignedExpression(_parser, parser_iVars.sName[0]))
		bIntVar[0] = false;

	if (!isVariableInAssignedExpression(_parser, parser_iVars.sName[1]))
		bIntVar[1] = false;

    // Prepare the memory for integration
	_parser.Eval(nResults);

	for (int i = 0; i < 3; i++)
	{
		vResult[i].resize(nResults, 0.0);
		fx_n[0][i].resize(nResults, 0.0);
		fx_n[1][i].resize(nResults, 0.0);
	}

	// Ensure that the integration ranges are available
	if (!sBoundariesX[0].length() || !sBoundariesX[1].length() || !sBoundariesY[0].length() || !sBoundariesY[1].length())
	    throw SyntaxError(SyntaxError::NO_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);

	// Sort the intervals and track it with an additional sign
	//
	// First: x range
	if (x0 > x1)
	{
		double dTemp = x0;
		x0 = x1;
		x1 = dTemp;
		nSign *= -1;
	}

	// now y range
	if (y0 > y1)
	{
		double dTemp = y0;
		string sTemp = sBoundariesY[0];
		y0 = y1;
		sBoundariesY[0] = sBoundariesY[1];
		y1 = dTemp;
		sBoundariesY[1] = sTemp;
		nSign *= -1;
	}

	if (findVariableInExpression(sBoundariesY[0] + " + " + sBoundariesY[1], parser_iVars.sName[0]) != string::npos)
    {
		bRenewBoundaries = true;    // Ja? Setzen wir den bool entsprechend
        sRenewBoundariesExpression = getNextArgument(sBoundariesY[0], false) + "," + getNextArgument(sBoundariesY[1], false);
    }

	// Does the expression depend upon at least one integration
	// variable?
	if (bIntVar[0] || bIntVar[1])
	{
		// Ensure that the precision is reasonble
		if (sPrecision.length() && (dx > x1 - x0 || dy > y1 - y0))
			sPrecision = "";

        // If the precision is invalid, we guess a reasonable value here
		if (!sPrecision.length())
		{
		    // We use the smallest intervall and split it into
		    // 100 parts
            dx = min(x1 - x0, y1 - y0) / 100;
		}

		// Ensure that the precision is positive
		if (dx < 0)
			dx *= -1;

		/* --> Legacy: womoeglich sollen einmal unterschiedliche Praezisionen fuer "x" und "y"
		 *     moeglich sein. Inzwischen weisen wir hier einfach mal die Praezision von "x" an
		 *     die fuer "y" zu. <--
		 */
		dy = dx;

		// Special case: the expression only depends upon "y" and not upon "x"
		// In this case, we switch everything, because the integration is much
		// faster in this case
		if ((bIntVar[1] && !bIntVar[0]) && !bRenewBoundaries)
		{
			NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("PARSERFUNCS_INTEGRATE2_SWAPVARS", parser_iVars.sName[0], parser_iVars.sName[1]) + " ... ");
			size_t pos;

            while ((pos = findVariableInExpression(sIntegrationExpression, parser_iVars.sName[1])) != string::npos)
                sIntegrationExpression.replace(pos, parser_iVars.sName[1].length(), parser_iVars.sName[0]);

			// --> Strings tauschen <--
			string sTemp = sBoundariesX[0];
			sBoundariesX[0] = sBoundariesY[0];
			sBoundariesY[0] = sTemp;
			sTemp = sBoundariesX[1];
			sBoundariesX[1] = sBoundariesY[1];
			sBoundariesY[1] = sTemp;

			// --> Werte tauschen <---
			value_type vTemp = x0;
			x0 = y0;
			y0 = vTemp;
			vTemp = x1;
			x1 = y1;
			y1 = vTemp;
			bIntVar[0] = true;
			bIntVar[1] = false;
			NumeReKernel::printPreFmt(_lang.get("COMMON_SUCCESS") + ".\n");
		}

		// Is it a very slow integration?
		if (((x0 - x) * (y0 - y) / dx >= 1e3 && bIntVar[0] && bIntVar[1])
				|| ((x0 - x) * (y0 - y) / dx >= 9.9e6 && (bIntVar[0] || bIntVar[1])))
			bLargeArray = true;

        // Avoid calculation with too many steps
		if (((x0 - x) * (y0 - y) / dx > 1e10 && bIntVar[0] && bIntVar[1])
				|| ((x0 - x) * (y0 - y) / dx > 1e10 && (bIntVar[0] || bIntVar[1])))
			throw SyntaxError(SyntaxError::INVALID_INTEGRATION_PRECISION, sCmd, SyntaxError::invalid_position);

		// --> Kleine Info an den Benutzer, dass der Code arbeitet <--
		if (_option.getSystemPrintStatus())
			NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... 0 %");

		// --> Setzen wir "x" und "y" auf ihre Startwerte <--
		x = x0; // x = x_0
		y = y0; // y = y_0

		// --> Fall: "x" und "y" enthalten. Sehr umstaendlich und aufwaendig zu rechnen <--
		if (bIntVar[0] && bIntVar[1])
		{
			// --> Werte mit den Startwerten die erste Stuetzstelle fuer die y-Integration aus <--
			v = _parser.Eval(nResults);
			fx_n[1][0].assign(v, v+nResults);

			/* --> Berechne das erste y-Integral fuer die erste Stuetzstelle fuer x
			 *     Die Schleife laeuft so lange wie y < y_1 <--
			 */
			while (y + dy < y1 + dy * 1e-1)
			{
				if (nMethod == TRAPEZOIDAL)
                    integrationstep_trapezoidal(y, dy, y1, vResult[1], fx_n[1][0], false);
				else if (nMethod == SIMPSON)
                    integrationstep_simpson(y, dy, y1, vResult[1], fx_n[1][0], false);
			}

			fx_n[0][0] = vResult[1];
		}
		else
		{
			// --> Hier ist nur "x" oder nur "y" enthalten. Wir koennen uns das erste Integral sparen <--
			v = _parser.Eval(nResults);
			fx_n[0][0].assign(v, v+nResults);
		}

		/* --> Das eigentliche, numerische Integral. Es handelt sich um nichts weiter als viele
		 *     while()-Schleifendurchlaeufe.
		 *     Die aeussere Schleife laeuft so lange x < x_1 ist. <--
		 */
		while (x + dx < x1 + dx * 1e-1)
		{
			if (nMethod == TRAPEZOIDAL)
			{
				x += dx; // x + dx

				// Refresh the y boundaries, if necessary
				if (bRenewBoundaries)
                    refreshBoundaries(sRenewBoundariesExpression, y0, y1, sIntegrationExpression);

				// --> Setzen wir "y" auf den Wert, der von der unteren y-Grenze vorgegeben wird <--
				y = y0;
				// --> Werten wir sofort die erste y-Stuetzstelle aus <--
				v = _parser.Eval(nResults);
				fx_n[1][0].assign(v, v+nResults);
				vResult[1].assign(nResults, 0.0);

				// --> Ist eigentlich sowohl "x" als auch "y" in f(x,y) (oder ggf. nur "y"?) vorhanden? <--
				if (bIntVar[1])
				{
					// --> Ja? Dann muessen wir wohl diese Integration muehsam ausrechnen <--
					while (y + dy < y1 + dy * 1e-1) // so lange y < y_1
					    integrationstep_trapezoidal(y, dy, y1, vResult[1], fx_n[1][0], false);
				}
				else if (bIntVar[0] && !bIntVar[1])
				{
					// We may calculate the whole integral using a single trapez
					y = y1;
					v = _parser.Eval(nResults);

					for (int i = 0; i < nResults; i++)
						vResult[1][i] = (y1 - y0) * (fx_n[1][0][i] + v[i]) * 0.5;
				}

				// --> Weise das Ergebnis der y-Integration an die zweite Stuetzstelle der x-Integration zu <--
				for (int i = 0; i < nResults; i++)
				{
					if (x > x1 && isnan(vResult[1][i]))
						vResult[1][i] = 0.0;

					vResult[0][i] += dx * (fx_n[0][0][i] + vResult[1][i]) * 0.5; // Berechne das Trapez zu x
					fx_n[0][0][i] = vResult[1][i]; // Weise den Wert der zweiten Stuetzstelle an die erste Stuetzstelle zu
				}
			}
			else if (nMethod == SIMPSON)
			{
			    for (size_t n = 1; n <= 2; n++)
                {
                    x += dx / 2.0; // x + dx

                    // Refresh the y boundaries, if necessary
                    if (bRenewBoundaries)
                        refreshBoundaries(sRenewBoundariesExpression, y0, y1, sIntegrationExpression);

                    // Set y to the first position
                    y = y0;

                    // Calculate the first position
                    if (n == 1)
                    {
                        v = _parser.Eval(nResults);
                        fx_n[1][0].assign(v, v+nResults);
                    }

                    vResult[n].assign(nResults, 0.0);

                    // --> Ist eigentlich sowohl "x" als auch "y" in f(x,y) (oder ggf. nur "y"?) vorhanden? <--
                    if (bIntVar[1])
                    {
                        // --> Ja? Dann muessen wir wohl diese Inegration muehsam ausrechnen <--
                        while (y + dy < y1 + dy * 1e-1) // so lange y < y_1
                            integrationstep_simpson(y, dy, y1, vResult[n], fx_n[1][0], false);
                    }
                    else if (bIntVar[0] && !bIntVar[1])
                    {
                        y = (y0 + y1) / 2.0;
                        v = _parser.Eval(nResults);
                        fx_n[1][1].assign(v, v+nResults);

                        y = y1;
                        v = _parser.Eval(nResults);

                        for (int i = 0; i < nResults; i++)
                            vResult[n][i] = (y1 - y0) / 6.0 * (fx_n[1][0][i] + 4.0 * fx_n[1][1][i] + v[i]);
                    }

                    // --> Weise das Ergebnis der y-Integration an die zweite Stuetzstelle der x-Integration zu <--
                    for (int i = 0; i < nResults; i++)
                    {
                        if (x > x1 && isnan(vResult[n][i]))
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
			if (_option.getSystemPrintStatus())
			{
				if (!bLargeArray)
				{
					if ((int)((x - x0) / (x1 - x0) * 20) > (int)((x - dx - x0) / (x1 - x0) * 20))
						NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + toString((int)((x - x0) / (x1 - x0) * 20) * 5) + " %");
				}
				else
				{
					if ((int)((x - x0) / (x1 - x0) * 100) > (int)((x - dx - x0) / (x1 - x0) * 100))
						NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + toString((int)((x - x0) / (x1 - x0) * 100)) + " %");
				}

				if (NumeReKernel::GetAsyncCancelState())//GetAsyncKeyState(VK_ESCAPE))
				{
					NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + _lang.get("COMMON_CANCEL") + "!\n");
					throw SyntaxError(SyntaxError::PROCESS_ABORTED_BY_USER, "", SyntaxError::invalid_position);
				}
			}
		}

        // Show a success message
		if (_option.getSystemPrintStatus())
			NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... 100 %");
	}
	else if (!bRenewBoundaries)
	{
		// In this case, the interval borders do not depend upon each other
		// and the expressin is also independent
		string sTemp = sIntegrationExpression;
		string sInt_Fct_2 = "";

		while (sTemp.length())
			sInt_Fct_2 += getNextArgument(sTemp, true) + "*" + parser_iVars.sName[0] + "*" + parser_iVars.sName[1] + ",";

		sInt_Fct_2.erase(sInt_Fct_2.length() - 1, 1);

		// --> Schnelle Loesung: Konstante x Flaeche, die vom Integral umschlossen wird <--
		x = x1 - x0;
		y = y1 - y0;
		_parser.SetExpr(sInt_Fct_2);
		v = _parser.Eval(nResults);
		vResult[0].assign(v, v+nResults);
	}
	else
	{
		/* --> Doofer Fall: zwar eine Funktion, die weder von "x" noch von "y" abhaengt,
		 *     dafuer aber erfordert, dass die Grenzen des Integrals jedes Mal aktualisiert
		 *     werden. <--
		 */
		if (_option.getSystemPrintStatus())
			NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("PARSERFUNCS_INTEGRATE_CONSTANT") + " ... ");

		// --> Waehle willkuerliche Praezision von 1e-4 <--
		dx = 1e-4;
		dy = 1e-4;
		// --> Setze "x" und "y" auf ihre unteren Grenzen <--
		x = x0;
		y = y0;
		// --> Werte erste x-Stuetzstelle aus <--
		v = _parser.Eval(nResults);
		fx_n[0][0].assign(v, v+nResults);

		/* --> Berechne das eigentliche Integral. Unterscheidet sich nur begrenzt von dem oberen,
		 *     ausfuehrlichen Fall, ausser dass die innere Schleife aufgrund des Fehlens der Inte-
		 *     grationsvariablen "y" vollstaendig wegfaellt <--
		 */
		while (x + 1e-4 < x1 + 1e-5)
		{
			if (nMethod == TRAPEZOIDAL)
			{
				x += dx; // x + dx

				// --> Erneuere die Werte der x- und y-Grenzen <--
				refreshBoundaries(sRenewBoundariesExpression, y0, y1, sIntegrationExpression);
				// --> Setze "y" wieder auf die untere Grenze <--
				y = y0;

				// --> Setze den Speicher fuer die "innere" Integration auf 0 <--
				vResult[1].assign(nResults, 0.0);

				// --> Werte erste y-Stuetzstelle aus <--
				v = _parser.Eval(nResults);
				fx_n[1][0].assign(v, v+nResults);

				// --> Setze "y" auf die obere Grenze <--
				y = y1;
				// --> Werte die zweite Stuetzstelle aus <--
				v = _parser.Eval(nResults);

				for (int i = 0; i < nResults; i++)
					vResult[0][i] += dx * (fx_n[0][0][i] + (y1 - y0) * (fx_n[1][0][i] + v[i]) * 0.5) * 0.5; // Berechne das Trapez zu x

                fx_n[0][0] = vResult[1]; // Weise den Wert der zweiten Stuetzstelle an die erste Stuetzstelle zu
			}
			else if (nMethod == SIMPSON)
			{
			    for (size_t n = 1; n <= 2; n++)
                {
                    x += dx / 2.0; // x + dx

                    // --> Erneuere die Werte der x- und y-Grenzen <--
                    refreshBoundaries(sRenewBoundariesExpression, y0, y1, sIntegrationExpression);
                    // --> Setze "y" wieder auf die untere Grenze <--
                    y = y0;

                    // --> Setze den Speicher fuer die "innere" Integration auf 0 <--
                    vResult[n].assign(nResults, 0.0);

                    // --> Werte erste y-Stuetzstelle aus <--
                    v = _parser.Eval(nResults);
                    fx_n[1][0].assign(v, v+nResults);

                    // --> Setze "y" auf die obere Grenze <--
                    y = (y0 + y1) / 2.0;
                    // --> Werte die zweite Stuetzstelle aus <--
                    v = _parser.Eval(nResults);
                    fx_n[1][1].assign(v, v+nResults);

                    // --> Setze "y" auf die obere Grenze <--
                    y = y1;
                    // --> Werte die zweite Stuetzstelle aus <--
                    v = _parser.Eval(nResults);

                    for (int i = 0; i < nResults; i++)
                        vResult[n][i] = (y1 - y0) / 6.0 * (fx_n[1][0][i] + 4.0 * fx_n[1][1][i] + v[i]);
                }

				for (int i = 0; i < nResults; i++)
					vResult[0][i] += dx / 6.0 * (fx_n[0][0][i] + 4.0 * vResult[1][i] + vResult[2][i]); // Berechne das Trapez zu x

                fx_n[0][0] = vResult[2]; // Weise den Wert der zweiten Stuetzstelle an die erste Stuetzstelle zu
			}
		}

	}

	// --> Falls die Grenzen irgendwo getauscht worden sind, wird dem hier Rechnung getragen <--
	for (int i = 0; i < nResults; i++)
		vResult[0][i] *= nSign;

	// --> FERTIG! Teilen wir dies dem Benutzer mit <--
	if (_option.getSystemPrintStatus())
		NumeReKernel::printPreFmt(": " + _lang.get("COMMON_SUCCESS") + "!\n");

	// --> Fertig! Zurueck zur aufrufenden Funkton! <--
	return vResult[0];
}


/////////////////////////////////////////////////
/// \brief Calculate the numerical differential
/// of the passed expression or data set.
///
/// \param sCmd const string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \param _functions Define&
/// \return vector<double>
///
/////////////////////////////////////////////////
vector<double> differentiate(const string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option, Define& _functions)
{
	string sExpr = sCmd.substr(findCommand(sCmd).sString.length() + findCommand(sCmd).nPos);
	string sEps = "";
	string sVar = "";
	string sPos = "";
	double dEps = 0.0;
	double dPos = 0.0;
	double* dVar = 0;
	value_type* v = 0;
	int nResults = 0;
	int nSamples = 100;
	vector<double> vInterval;
	vector<double> vResult;

	// Strings cannot be differentiated
	if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sExpr))
		throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "diff");

    // Remove trailing parameter lists
	if (sExpr.find("-set") != string::npos)
		sExpr.erase(sExpr.find("-set"));
	else if (sExpr.find("--") != string::npos)
		sExpr.erase(sExpr.find("--"));

    // Try to call custom functions
	if (!_functions.call(sExpr))
		throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, sExpr, sExpr);

	StripSpaces(sExpr);

	// Numerical expressions and data sets are handled differently
	if ((sExpr.find("data(") == string::npos && !_data.containsTablesOrClusters(sExpr))
			&& (sCmd.find("-set") != string::npos || sCmd.find("--") != string::npos))
	{
	    // This is a numerical expression
		if (sCmd.find("-set") != string::npos)
			sVar = sCmd.substr(sCmd.find("-set"));
		else
			sVar = sCmd.substr(sCmd.find("--"));

		// Try to call custom functions
		if (!_functions.call(sVar))
			throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, sVar, sVar);

		StripSpaces(sVar);

		// Is the "eps" parameter available?
		if (findParameter(sVar, "eps", '='))
		{

			sEps = getArgAtPos(sVar, findParameter(sVar, "eps", '=') + 3);
			sVar += " ";
			sVar = sVar.substr(0, findParameter(sVar, "eps", '=')) + sVar.substr(sVar.find(' ', findParameter(sVar, "eps", '=') + 3));

			if (isNotEmptyExpression(sEps))
			{
				_parser.SetExpr(sEps);
				dEps = _parser.Eval();
			}

			if (isinf(dEps) || isnan(dEps))
				dEps = 0.0;

			if (dEps < 0)
				dEps *= -1;
		}

		// Is the "samples" parameter available?
		if (findParameter(sVar, "samples", '='))
		{

			_parser.SetExpr(getArgAtPos(sVar, findParameter(sVar, "samples", '=') + 7));
			nSamples = (int)_parser.Eval();
			sVar += " ";
			sVar = sVar.substr(0, findParameter(sVar, "samples", '=')) + sVar.substr(sVar.find(' ', findParameter(sVar, "samples", '=') + 7));

			if (nSamples <= 0)
				nSamples = 100;
		}

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
				if (_data.containsTablesOrClusters(sPos) || sPos.find("data(") != string::npos)
					getDataElements(sPos, _parser, _data, _option);

				if (sPos.find(':') != string::npos)
					sPos.replace(sPos.find(':'), 1, ",");
				_parser.SetExpr(sPos);
				v = _parser.Eval(nResults);

				if (isinf(v[0]) || isnan(v[0]))
				{
					vResult.push_back(NAN);
					return vResult;
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
			throw SyntaxError(SyntaxError::NO_DIFF_VAR, sCmd, SyntaxError::invalid_position);

		// Define a reasonable precision if no precision was set
		if (!dEps)
			dEps = 1e-7;

        // Store the expression
		string sCompl_Expr = sExpr;

		// Expand the expression, if necessary
        if (sCompl_Expr.find("{") != string::npos)
            convertVectorToExpression(sCompl_Expr, _option);

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
	else if (sExpr.find("data(") != string::npos || _data.containsTablesOrClusters(sExpr))
	{
	    // This is a data set
	    //
	    // Get the indices first
		Indices _idx = getIndices(sExpr, _parser, _data, _option);

		// Extract the table name
		sExpr.erase(sExpr.find('('));

		// Validate the indices
		if (!isValidIndexSet(_idx))
			throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

        if (_idx.row.isOpenEnd())
            _idx.row.setRange(0, _data.getLines(sExpr, false)-1);

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
                if (_data.isValidEntry(_idx.row[i], _idx.col.front(), sExpr)
                        && _data.isValidEntry(_idx.row[i + 1], _idx.col.front(), sExpr))
                    vResult.push_back(_data.getElement(_idx.row[i + 1], _idx.col.front(), sExpr) - _data.getElement(_idx.row[i], _idx.col.front(), sExpr));
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
            Datafile _cache;

            for (size_t i = 0; i < _idx.row.size(); i++)
            {
                _cache.writeToTable(i, 0, "cache", _data.getElement(_idx.row[i], _idx.col[0], sExpr));
                _cache.writeToTable(i, 1, "cache", _data.getElement(_idx.row[i], _idx.col[1], sExpr));
            }

            _cache.sortElements("cache -sort c=1[2]");

            // Shall the x values be calculated?
            if (findParameter(sCmd, "xvals"))
            {
                // The x values are approximated to be in the
                // middle of the two samplex
                for (long long int i = 0; i < _cache.getLines("cache", false) - 1; i++)
                {
                    if (_cache.isValidEntry(i, 0, "cache")
                            && _cache.isValidEntry(i + 1, 0, "cache")
                            && _cache.isValidEntry(i, 1, "cache")
                            && _cache.isValidEntry(i + 1, 1, "cache"))
                        vResult.push_back((_cache.getElement(i + 1, 0, "cache") + _cache.getElement(i, 0, "cache")) / 2);
                    else
                        vResult.push_back(NAN);
                }
            }
            else
            {
                // We calculate the derivative of the data
                // by approximating it linearily
                for (long long int i = 0; i < _cache.getLines("cache", false) - 1; i++)
                {
                    if (_cache.isValidEntry(i, 0, "cache")
                            && _cache.isValidEntry(i + 1, 0, "cache")
                            && _cache.isValidEntry(i, 1, "cache")
                            && _cache.isValidEntry(i + 1, 1, "cache"))
                        vResult.push_back((_cache.getElement(i + 1, 1, "cache") - _cache.getElement(i, 1, "cache"))
                                          / (_cache.getElement(i + 1, 0, "cache") - _cache.getElement(i, 0, "cache")));
                    else
                        vResult.push_back(NAN);
                }
            }
        }
	}
	else
	{
	    // Ensure that a parameter list is available
		throw SyntaxError(SyntaxError::NO_DIFF_OPTIONS, sCmd, SyntaxError::invalid_position);
	}

	return vResult;
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
/// \param sCmd string&
/// \param sExpr string&
/// \param sInterval string&
/// \param nOrder int
/// \param nMode int
/// \return bool
///
/////////////////////////////////////////////////
static bool findExtremaInMultiResult(string& sCmd, string& sExpr, string& sInterval, int nOrder, int nMode)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    _parser.SetExpr(sExpr);
    int nResults;
    value_type* v = _parser.Eval(nResults);
    vector<double> vResults;
    int nResults_x = 0;
    Datafile _cache;

    // Store the results in the second column of a table
    for (int i = 0; i < nResults; i++)
        _cache.writeToTable(i, 1, "cache", v[i]);

    _parser.SetExpr(sInterval);
    v = _parser.Eval(nResults_x);

    // Write the results for the x interval in the first column
    if (nResults_x > 1)
    {
        for (int i = 0; i < nResults; i++)
        {
            if (i >= nResults_x)
                _cache.writeToTable(i, 0, "cache", 0.0);
            else
                _cache.writeToTable(i, 0, "cache", v[i]);
        }
    }
    else
        return false;

    sCmd = "cache -sort cols=1[2]";
    _cache.sortElements(sCmd);

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
    for (int i = 0; i + nanShift < _cache.getLines("cache", true); i++)
    {
        if (i == nOrder)
            break;

        while (isnan(_cache.getElement(i + nanShift, 1, "cache")) && i + nanShift < _cache.getLines("cache", true) - 1)
            nanShift++;

        data[i] = _cache.getElement(i + nanShift, 1, "cache");
    }

    // Sort the data and find the median
    gsl_sort(data, 1, nOrder);
    dExtremum = gsl_stats_median_from_sorted_data(data, 1, nOrder);

    // Go through the data points using sliding median to find the local
    // extrema in the data set
    for (int i = nOrder; i + nanShift < _cache.getLines("cache", false) - nOrder; i++)
    {
        int currNanShift = 0;
        dMedian = 0.0;

        for (int j = i; j < i + nOrder; j++)
        {
            while (isnan(_cache.getElement(j + nanShift + currNanShift, 1, "cache")) && j + nanShift + currNanShift < _cache.getLines("cache", true) - 1)
                currNanShift++;

            data[j - i] = _cache.getElement(j + nanShift + currNanShift, 1, "cache");
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
                    double dExtremum = _cache.getElement(i + nanShift, 1, "cache");

                    for (long long int k = i + nanShift; k >= 0; k--)
                    {
                        if (k == i - nOrder)
                            break;

                        if (nDir*_cache.getElement(k, 1, "cache") > nDir*dExtremum)
                        {
                            nExtremum = k;
                            dExtremum = _cache.getElement(k, 1, "cache");
                        }
                    }

                    vResults.push_back(_cache.getElement(nExtremum, 0, "cache"));
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
    sCmd = "_~extrema[~_~]";
    _parser.SetVectorVar("_~extrema[~_~]", vResults);
    return true;
}


/////////////////////////////////////////////////
/// \brief This static function finds extrema in
/// the selected data sets.
///
/// \param sCmd string&
/// \param sExpr string&
/// \param nOrder int
/// \param nMode int
/// \return bool
///
/////////////////////////////////////////////////
static bool findExtremaInData(string& sCmd, string& sExpr, int nOrder, int nMode)
{
    value_type* v;
    int nResults = 0;
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    _parser.SetExpr(sExpr);
    v = _parser.Eval(nResults);

    if (nResults > 1)
    {
        if (nOrder >= nResults / 3)
            nOrder = nResults / 3;

        double dMedian = 0.0, dExtremum = 0.0;
        double* data = 0;
        data = new double[nOrder];
        int nDir = 0;
        int nanShift = 0;
        vector<double> vResults;

        if (nOrder < 3)
        {
            vResults.push_back(NAN);
            return false;
        }

        for (int i = 0; i + nanShift < nResults; i++)
        {
            if (i == nOrder)
                break;

            while (isnan(v[i + nanShift]) && i + nanShift < nResults - 1)
                nanShift++;

            data[i] = v[i + nanShift];
        }

        gsl_sort(data, 1, nOrder);
        dExtremum = gsl_stats_median_from_sorted_data(data, 1, nOrder);

        for (int i = nOrder; i + nanShift < nResults - nOrder; i++)
        {
            int currNanShift = 0;
            dMedian = 0.0;

            for (int j = i; j < i + nOrder; j++)
            {
                while (isnan(v[j + nanShift + currNanShift]) && j + nanShift + currNanShift < nResults - 1)
                    currNanShift++;

                data[j - i] = v[j + nanShift + currNanShift];
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
                        double dExtremum = v[i + nanShift];

                        for (long long int k = i + nanShift; k >= 0; k--)
                        {
                            if (k == i - nOrder)
                                break;

                            if (nDir*v[k] > nDir*dExtremum)
                            {
                                nExtremum = k;
                                dExtremum = v[k];
                            }
                        }

                        vResults.push_back(nExtremum + 1);
                        i = nExtremum + nOrder;
                    }

                    nDir = 0;
                }

                dExtremum = dMedian;
            }

            nanShift += currNanShift;
        }

        if (data)
            delete[] data;

        if (!vResults.size())
            vResults.push_back(NAN);

        sCmd = "_~extrema[~_~]";
        _parser.SetVectorVar("_~extrema[~_~]", vResults);
        return true;
    }
    else
        throw SyntaxError(SyntaxError::NO_EXTREMA_VAR, sCmd, SyntaxError::invalid_position);
}


/////////////////////////////////////////////////
/// \brief This function is a wrapper to the
/// actual extrema localisation function
/// localizeExtremum() further below.
///
/// \param sCmd string&
/// \param _data Datafile&
/// \param _parser Parser&
/// \param _option const Settings&
/// \param _functions Define&
/// \return bool
///
/////////////////////////////////////////////////
bool findExtrema(string& sCmd, Datafile& _data, Parser& _parser, const Settings& _option, Define& _functions)
{
	unsigned int nSamples = 21;
	int nOrder = 5;
	double dVal[2];
	double dBoundaries[2] = {0.0, 0.0};
	int nMode = 0;
	double* dVar = 0;
	string sExpr = "";
	string sParams = "";
	string sInterval = "";
	string sVar = "";

	// We cannot search extrema in strings
	if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
		throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "extrema");

	// Separate expression and parameter string
	if (sCmd.find("-set") != string::npos)
	{
		sExpr = sCmd.substr(0, sCmd.find("-set"));
		sParams = sCmd.substr(sCmd.find("-set"));
	}
	else if (sCmd.find("--") != string::npos)
	{
		sExpr = sCmd.substr(0, sCmd.find("--"));
		sParams = sCmd.substr(sCmd.find("--"));
	}
	else if (sCmd.find("data(") == string::npos && !_data.containsTablesOrClusters(sCmd))
		throw SyntaxError(SyntaxError::NO_EXTREMA_OPTIONS, sCmd, SyntaxError::invalid_position);
	else
		sExpr = sCmd;

    // Isolate the expression
	StripSpaces(sExpr);
	sExpr = sExpr.substr(findCommand(sExpr).sString.length());

	// Ensure that the expression is not empty
	// and that the custom functions don't throw
	// any errors
	if (!isNotEmptyExpression(sExpr) || !_functions.call(sExpr))
		return false;

	if (!_functions.call(sParams))
		return false;

	StripSpaces(sParams);

	// If the expression or the parameter list contains
	// data elements, get their values here
	if (sExpr.find("data(") != string::npos || _data.containsTablesOrClusters(sExpr))
		getDataElements(sExpr, _parser, _data, _option, false);

	if (sParams.find("data(") != string::npos || _data.containsTablesOrClusters(sParams))
		getDataElements(sParams, _parser, _data, _option, false);

	// Evaluate the parameters
	if (findParameter(sParams, "min"))
		nMode = -1;

	if (findParameter(sParams, "max"))
		nMode = 1;

	if (findParameter(sParams, "samples", '='))
	{
		_parser.SetExpr(getArgAtPos(sParams, findParameter(sParams, "samples", '=') + 7));
		nSamples = (unsigned int)_parser.Eval();

		if (nSamples < 21)
			nSamples = 21;

		sParams.erase(findParameter(sParams, "samples", '=') - 1, 8);
	}

	if (findParameter(sParams, "points", '='))
	{
		_parser.SetExpr(getArgAtPos(sParams, findParameter(sParams, "points", '=') + 6));
		nOrder = (int)_parser.Eval();

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
            return findExtremaInMultiResult(sCmd, sExpr, sInterval, nOrder, nMode);
		else
		{
			if (!isVariableInAssignedExpression(_parser, sVar))
			{
				sCmd = "nan";
				return true;
			}

			dVar = getPointerToVariable(sVar, _parser);

			if (!dVar)
				throw SyntaxError(SyntaxError::EXTREMA_VAR_NOT_FOUND, sCmd, sVar, sVar);

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
                        sCmd = "nan";
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
	else if (sCmd.find("data(") != string::npos || _data.containsTablesOrClusters(sCmd))
		return findExtremaInData(sCmd, sExpr, nOrder, nMode);
	else
		throw SyntaxError(SyntaxError::NO_EXTREMA_VAR, sCmd, SyntaxError::invalid_position);

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
	sCmd = "";
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
				vResults.push_back(localizeExtremum(sExpr, dVar, _parser, _option, dBoundaries[0] + (i - 1) * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1), dBoundaries[0] + i * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1)));
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
				vResults.push_back(localizeExtremum(sExpr, dVar, _parser, _option, dBoundaries[0] + nTemp * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1), dBoundaries[0] + i * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1)));
			}
		}
		dVal[0] = dVal[1];
	}

	// If we didn't find any results
	// examine the boundaries for possible extremas
	if (!sCmd.length() && !vResults.size())
	{
		dVal[0] = _parser.Diff(dVar, dBoundaries[0]);
		dVal[1] = _parser.Diff(dVar, dBoundaries[1]);

        // Examine the left boundary
		if (dVal[0]
				&& (!nMode
					|| (dVal[0] < 0 && nMode == 1)
					|| (dVal[0] > 0 && nMode == -1)))
			sCmd = toString(dBoundaries[0], _option);

		// Examine the right boundary
		if (dVal[1]
				&& (!nMode
					|| (dVal[1] < 0 && nMode == -1)
					|| (dVal[1] > 0 && nMode == 1)))
		{
			if (sCmd.length())
				sCmd += ", ";

			sCmd += toString(dBoundaries[1], _option);
		}

		// Still nothing found?
		if (!dVal[0] && ! dVal[1])
			sCmd = "nan";
	}
	else
	{
		sCmd = "_~extrema[~_~]";
		_parser.SetVectorVar("_~extrema[~_~]", vResults);
	}

	return true;
}


/////////////////////////////////////////////////
/// \brief This static function finds zeroes in
/// a multi-result expression, i.e. an expression
/// containing a table or similar.
///
/// \param sCmd string&
/// \param sExpr string&
/// \param sInterval string&
/// \param nMode int
/// \return bool
///
/////////////////////////////////////////////////
static bool findZeroesInMultiResult(string& sCmd, string& sExpr, string& sInterval, int nMode)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    _parser.SetExpr(sExpr);
    int nResults;
    value_type* v = _parser.Eval(nResults);
    Datafile _cache;

    vector<double> vResults;
    int nResults_x = 0;

    for (int i = 0; i < nResults; i++)
        _cache.writeToTable(i, 1, "cache", v[i]);

    _parser.SetExpr(sInterval);
    v = _parser.Eval(nResults_x);

    if (nResults_x > 1)
    {
        for (int i = 0; i < nResults; i++)
        {
            if (i >= nResults_x)
                _cache.writeToTable(i, 0, "cache", 0.0);
            else
                _cache.writeToTable(i, 0, "cache", v[i]);
        }
    }
    else
        return false;

    sCmd = "cache -sort cols=1[2]";
    _cache.sortElements(sCmd);

    for (long long int i = 1; i < _cache.getLines("cache", false); i++)
    {
        if (isnan(_cache.getElement(i - 1, 1, "cache")))
            continue;

        if (!nMode && _cache.getElement(i, 1, "cache")*_cache.getElement(i - 1, 1, "cache") <= 0.0)
        {
            if (_cache.getElement(i, 1, "cache") == 0.0)
            {
                vResults.push_back(_cache.getElement(i, 0, "cache"));
                i++;
            }
            else if (_cache.getElement(i - 1, 1, "cache") == 0.0)
                vResults.push_back(_cache.getElement(i - 1, 0, "cache"));
            else if (_cache.getElement(i, 1, "cache")*_cache.getElement(i - 1, 1, "cache") < 0.0)
                vResults.push_back(Linearize(_cache.getElement(i - 1, 0, "cache"), _cache.getElement(i - 1, 1, "cache"), _cache.getElement(i, 0, "cache"), _cache.getElement(i, 1, "cache")));
        }
        else if (nMode && _cache.getElement(i, 1, "cache")*_cache.getElement(i - 1, 1, "cache") <= 0.0)
        {
            if (_cache.getElement(i, 1, "cache") == 0.0 && _cache.getElement(i - 1, 1, "cache") == 0.0)
            {
                for (long long int j = i + 1; j < _cache.getLines("cache", false); j++)
                {
                    if (nMode * _cache.getElement(j, 1, "cache") > 0.0)
                    {
                        for (long long int k = i - 1; k <= j; k++)
                            vResults.push_back(_cache.getElement(k, 0, "cache"));

                        break;
                    }
                    else if (nMode * _cache.getElement(j, 1, "cache") < 0.0)
                        break;

                    if (j + 1 == _cache.getLines("cache", false) && i > 1 && nMode * _cache.getElement(i - 2, 1, "cache") < 0.0)
                    {
                        for (long long int k = i - 1; k <= j; k++)
                            vResults.push_back(_cache.getElement(k, 0, "cache"));

                        break;
                    }
                }

                continue;
            }
            else if (_cache.getElement(i, 1, "cache") == 0.0 && nMode * _cache.getElement(i - 1, 1, "cache") < 0.0)
                vResults.push_back(_cache.getElement(i, 0, "cache"));
            else if (_cache.getElement(i - 1, 1, "cache") == 0.0 && nMode * _cache.getElement(i, 1, "cache") > 0.0)
                vResults.push_back(_cache.getElement(i - 1, 0, "cache"));
            else if (_cache.getElement(i, 1, "cache")*_cache.getElement(i - 1, 1, "cache") < 0.0 && nMode * _cache.getElement(i - 1, 1, "cache") < 0.0)
                vResults.push_back(Linearize(_cache.getElement(i - 1, 0, "cache"), _cache.getElement(i - 1, 1, "cache"), _cache.getElement(i, 0, "cache"), _cache.getElement(i, 1, "cache")));
        }
    }

    if (!vResults.size())
        vResults.push_back(NAN);

    sCmd = "_~zeroes[~_~]";
    _parser.SetVectorVar("_~zeroes[~_~]", vResults);
    return true;
}


/////////////////////////////////////////////////
/// \brief This static function finds zeroes in
/// the selected data set.
///
/// \param sCmd string&
/// \param sExpr string&
/// \param nMode int
/// \return bool
///
/////////////////////////////////////////////////
static bool findZeroesInData(string& sCmd, string& sExpr, int nMode)
{
    value_type* v;
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

        sCmd = "_~zeroes[~_~]";
        _parser.SetVectorVar("_~zeroes[~_~]", vResults);
        return true;
    }
    else
        throw SyntaxError(SyntaxError::NO_ZEROES_VAR, sCmd, SyntaxError::invalid_position);
}


/////////////////////////////////////////////////
/// \brief This function is a wrapper to the
/// actual zeros localisation function
/// localizeZero() further below.
///
/// \param sCmd string&
/// \param _data Datafile&
/// \param _parser Parser&
/// \param _option const Settings&
/// \param _functions Define&
/// \return bool
///
/////////////////////////////////////////////////
bool findZeroes(string& sCmd, Datafile& _data, Parser& _parser, const Settings& _option, Define& _functions)
{
	unsigned int nSamples = 21;
	double dVal[2];
	double dBoundaries[2] = {0.0, 0.0};
	int nMode = 0;
	double* dVar = 0;
	double dTemp = 0.0;
	string sExpr = "";
	string sParams = "";
	string sInterval = "";
	string sVar = "";

	// We cannot find zeroes in strings
	if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
		throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "zeroes");

	// Separate expression and the parameter list
	if (sCmd.find("-set") != string::npos)
	{
		sExpr = sCmd.substr(0, sCmd.find("-set"));
		sParams = sCmd.substr(sCmd.find("-set"));
	}
	else if (sCmd.find("--") != string::npos)
	{
		sExpr = sCmd.substr(0, sCmd.find("--"));
		sParams = sCmd.substr(sCmd.find("--"));
	}
	else if (sCmd.find("data(") == string::npos && !_data.containsTablesOrClusters(sCmd))
		throw SyntaxError(SyntaxError::NO_ZEROES_OPTIONS, sCmd, SyntaxError::invalid_position);
	else
		sExpr = sCmd;

    // Isolate the expression
	StripSpaces(sExpr);
	sExpr = sExpr.substr(findCommand(sExpr).sString.length());

	// Ensure that custom functions don't throw any
	// errors and that the expression is not empty
	if (!isNotEmptyExpression(sExpr) || !_functions.call(sExpr))
		return false;

	if (!_functions.call(sParams))
		return false;

	StripSpaces(sParams);

	// If the expression or the parameter list contains
	// data elements, get their values here
	if (sExpr.find("data(") != string::npos || _data.containsTablesOrClusters(sExpr))
		getDataElements(sExpr, _parser, _data, _option, false);

	if (sParams.find("data(") != string::npos || _data.containsTablesOrClusters(sParams))
		getDataElements(sParams, _parser, _data, _option, false);

	// Evaluate the parameter list
	if (findParameter(sParams, "min") || findParameter(sParams, "down"))
		nMode = -1;
	if (findParameter(sParams, "max") || findParameter(sParams, "up"))
		nMode = 1;

	if (findParameter(sParams, "samples", '='))
	{
		_parser.SetExpr(getArgAtPos(sParams, findParameter(sParams, "samples", '=') + 7));
		nSamples = (int)_parser.Eval();

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
            return findZeroesInMultiResult(sCmd, sExpr, sInterval, nMode);
		else
		{
			if (!isVariableInAssignedExpression(_parser, sVar))
			{
				sCmd = "nan";
				return true;
			}

			dVar = getPointerToVariable(sVar, _parser);

			if (!dVar)
				throw SyntaxError(SyntaxError::ZEROES_VAR_NOT_FOUND, sCmd, sVar, sVar);

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
                        sCmd = "nan";
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
	else if (sCmd.find("data(") != string::npos || _data.containsTablesOrClusters(sCmd))
        return findZeroesInData(sCmd, sExpr, nMode);
	else
		throw SyntaxError(SyntaxError::NO_ZEROES_VAR, sCmd, SyntaxError::invalid_position);

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
	sCmd = "";
	dTemp = *dVar;

	*dVar = dBoundaries[0];
	vector<double> vResults;
	dVal[0] = _parser.Eval();

	// Find near zeros to the left of the boundary
	// which are probably not located due toe rounding
	// errors
	if (dVal[0] != 0.0 && fabs(dVal[0]) < 1e-10)
	{
		*dVar = dBoundaries[0] - 1e-10;
		dVal[1] = _parser.Eval();

		if (dVal[0]*dVal[1] < 0 && (nMode * dVal[0] <= 0.0))
			vResults.push_back(localizeExtremum(sExpr, dVar, _parser, _option, dBoundaries[0] - 1e-10, dBoundaries[0]));
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
				vResults.push_back((localizeZero(sExpr, dVar, _parser, _option, dBoundaries[0] + (i - 1) * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1), dBoundaries[0] + i * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1))));
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
				vResults.push_back(localizeZero(sExpr, dVar, _parser, _option, dBoundaries[0] + nTemp * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1), dBoundaries[0] + i * (dBoundaries[1] - dBoundaries[0]) / (double)(nSamples - 1)));
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
			vResults.push_back(localizeZero(sExpr, dVar, _parser, _option, dBoundaries[1], dBoundaries[1] + 1e-10));
	}

	*dVar = dTemp;

	if (!sCmd.length() && !vResults.size())
	{
	    // Still nothing found?
		sCmd = "nan";
	}
	else
	{
		sCmd = "_~zeroes[~_~]";
		_parser.SetVectorVar("_~zeroes[~_~]", vResults);
	}

	return true;
}


/////////////////////////////////////////////////
/// \brief This function searches for the
/// positions of all extrema, which are located
/// in the selected interval.
///
/// \param sCmd string&
/// \param dVarAdress double*
/// \param _parser Parser&
/// \param _option const Settings&
/// \param dLeft double
/// \param dRight double
/// \param dEps double
/// \param nRecursion int
/// \return double
///
/// The expression has to be setted in advance.
/// The function performs recursions until the
/// defined precision is reached.
/////////////////////////////////////////////////
static double localizeExtremum(string& sCmd, double* dVarAdress, Parser& _parser, const Settings& _option, double dLeft, double dRight, double dEps, int nRecursion)
{
	const unsigned int nSamples = 101;
	double dVal[2];

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
/// \param dVarAdress double*
/// \param _parser Parser&
/// \param _option const Settings&
/// \param dLeft double
/// \param dRight double
/// \param dEps double
/// \param nRecursion int
/// \return double
///
/// The expression has to be setted in advance.
/// The function performs recursions until the
/// defined precision is reached.
/////////////////////////////////////////////////
static double localizeZero(string& sCmd, double* dVarAdress, Parser& _parser, const Settings& _option, double dLeft, double dRight, double dEps, int nRecursion)
{
	const unsigned int nSamples = 101;
	double dVal[2];

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
/// \param sCmd string&
/// \param _parser Parser&
/// \param _option const Settings&
/// \param _functions Define&
/// \return void
///
/// The aproximated function is defined as a new
/// custom function.
/////////////////////////////////////////////////
void taylor(string& sCmd, Parser& _parser, const Settings& _option, Define& _functions)
{
	string sParams = "";
	string sVarName = "";
	string sExpr = "";
	string sExpr_cpy = "";
	string sArg = "";
	string sTaylor = "Taylor";
	string sPolynom = "";
	bool bUseUniqueName = false;
	size_t nth_taylor = 6;
	size_t nSamples = 0;
	size_t nMiddle = 0;
	double* dVar = 0;
	double dVarValue = 0.0;
	long double** dDiffValues = 0;

	// We cannot approximate string expressions
	if (containsStrings(sCmd))
		throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "taylor");

	// Extract the parameter list
	if (sCmd.find("-set") != string::npos)
		sParams = sCmd.substr(sCmd.find("-set"));
	else if (sCmd.find("--") != string::npos)
		sParams = sCmd.substr(sCmd.find("--"));
	else
	{
		NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_TAYLOR_MISSINGPARAMS"), _option));
		return;
	}

	// Evaluate the parameters
	if (findParameter(sParams, "n", '='))
	{
		_parser.SetExpr(sParams.substr(findParameter(sParams, "n", '=') + 1, sParams.find(' ', findParameter(sParams, "n", '=') + 1) - findParameter(sParams, "n", '=') - 1));
		nth_taylor = (unsigned int)_parser.Eval();

		if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
			nth_taylor = 6;

		sParams = sParams.substr(0, findParameter(sParams, "n", '=') - 1) + sParams.substr(findParameter(sParams, "n", '=') - 1 + _parser.GetExpr().length());
	}

	if (findParameter(sParams, "unique") || findParameter(sParams, "u"))
		bUseUniqueName = true;

	// Extract the variable and the approximation location
	if (sParams.find('=') == string::npos)
		return;
	else
	{
		if (sParams.substr(0, 2) == "-s")
			sParams = sParams.substr(4);
		else
			sParams = sParams.substr(2);

        // Get the variable name
		sVarName = sParams.substr(0, sParams.find('='));
		StripSpaces(sVarName);

		// Get the current value of the variable
		_parser.SetExpr(sParams.substr(sParams.find('=') + 1, sParams.find(' ', sParams.find('=')) - sParams.find('=') - 1));
		dVarValue = _parser.Eval();

		// Ensure that the location was chosen reasonable
		if (isinf(dVarValue) || isnan(dVarValue))
		{
			sCmd = "nan";
			return;
		}

		// Create the string element, which is used
		// for the variable in the created funcction
		// string
		if (!dVarValue)
			sArg = "x";
		else if (dVarValue < 0)
			sArg = "x+" + toString(-dVarValue, _option.getPrecision());
		else
			sArg = "x-" + toString(dVarValue, _option.getPrecision());
	}

	// Extract the expression
	sExpr = sCmd.substr(sCmd.find(' ') + 1);

	if (sExpr.find("-set") != string::npos)
		sExpr = sExpr.substr(0, sExpr.find("-set"));
	else
		sExpr = sExpr.substr(0, sExpr.find("--"));

	StripSpaces(sExpr);

	sExpr_cpy = sExpr;

	// Create a unique function name, if it is desired
	if (bUseUniqueName)
		sTaylor += toString((int)nth_taylor) + "_" + sExpr;

    // Ensure that the call to the custom function throws errors
	if (!_functions.call(sExpr))
		return;

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

	if (_option.getSystemPrintStatus())
		NumeReKernel::print(LineBreak(sTaylor, _option, true, 0, 8));

	sTaylor += _lang.get("PARSERFUNCS_TAYLOR_DEFINESTRING", sExpr_cpy, sVarName, toString(dVarValue, 4), toString((int)nth_taylor));

	bool bDefinitionSuccess = false;

	if (_functions.isDefined(sTaylor.substr(0, sTaylor.find(":="))))
		bDefinitionSuccess = _functions.defineFunc(sTaylor, true);
	else
		bDefinitionSuccess = _functions.defineFunc(sTaylor);

    if (bDefinitionSuccess)
        NumeReKernel::print(_lang.get("DEFINE_SUCCESS"), _option.getSystemPrintStatus());
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
/// \brief This function calculates the fast
/// fourier transform of the passed data set.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \return bool
///
/// The user may switch between complex or phase-
/// amplitude layout and whether an inverse
/// transform shall be calculated.
/////////////////////////////////////////////////
bool fastFourierTransform(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option)
{
	mglDataC _fftData;
	Indices _idx;

	double dNyquistFrequency = 1.0;
	double dTimeInterval = 0.0;
	double dPhaseOffset = 0.0;
	bool bInverseTrafo = false;
	bool bComplex = false;
	string sTargetTable = "fftdata";

	if (findParameter(sCmd, "inverse"))
		bInverseTrafo = true;

	if (findParameter(sCmd, "complex"))
		bComplex = true;

	// search for explicit "target" options and select the target cache
	sTargetTable = evaluateTargetOptionInCommand(sCmd, sTargetTable, _idx, _parser, _data, _option);

	if (findParameter(sCmd, "inverse") || findParameter(sCmd, "complex"))
	{
		for (unsigned int i = 0; i < sCmd.length(); i++)
		{
			if (sCmd[i] == '(')
				i += getMatchingParenthesis(sCmd.substr(i));

			if (sCmd[i] == '-')
			{
				sCmd.erase(i);
				break;
			}
		}
	}

	sCmd = sCmd.substr(sCmd.find(' ', sCmd.find("fft")));
	StripSpaces(sCmd);

	// get the data from the data object
	NumeRe::Table _table = parser_extractData(sCmd, _parser, _data, _option);

	dNyquistFrequency = _table.getLines() / (_table.getValue(_table.getLines() - 1, 0) - _table.getValue(0, 0)) / 2.0;
	dTimeInterval = (_table.getLines() - 1) / (_table.getValue(_table.getLines() - 1, 0));

	if (_option.getSystemPrintStatus())
	{
		if (!bInverseTrafo)
			NumeReKernel::printPreFmt(LineBreak("|-> " + _lang.get("PARSERFUNCS_FFT_FOURIERTRANSFORMING", toString(_table.getCols()), toString(dNyquistFrequency, 6)) + " ", _option, 0));
		else
			NumeReKernel::printPreFmt(LineBreak("|-> " + _lang.get("PARSERFUNCS_FFT_INVERSE_FOURIERTRANSFORMING", toString(_table.getCols()), toString(dNyquistFrequency, 6)) + " ", _option, 0));
	}

	_fftData.Create(_table.getLines());

	for (size_t i = 0; i < _table.getLines(); i++)
	{
		if (_table.getCols() == 2)
			_fftData.a[i] = dual(_table.getValue(i, 1), 0.0);
		else if (_table.getCols() == 3 && bComplex)
			_fftData.a[i] = dual(_table.getValue(i, 1), _table.getValue(i, 2));
		else if (_table.getCols() == 3 && !bComplex)
			_fftData.a[i] = dual(_table.getValue(i, 1) * cos(_table.getValue(i, 2)), _table.getValue(i, 1) * sin(_table.getValue(i, 3)));
	}

    // Calculate the actual transformation and apply some
    // normalisation
	if (!bInverseTrafo)
	{
		_fftData.FFT("x");
		_fftData.a[0] /= dual((double)_table.getLines(), 0.0);
		_fftData.a[(int)round(_fftData.GetNx() / 2.0)] /= dual(2.0, 0.0);

		for (long long int i = 1; i < _fftData.GetNx(); i++)
			_fftData.a[i] /= dual((double)_table.getLines() / 2.0, 0.0);
	}
	else
	{
		_fftData.a[0] *= dual(2.0, 0.0);
		_fftData.a[_fftData.GetNx() - 1] *= dual(2.0, 0.0);

		for (long long int i = 0; i < _fftData.GetNx(); i++)
			_fftData.a[i] *= dual((double)(_fftData.GetNx() - 1), 0.0);

		_fftData.FFT("ix");
	}

	if (_idx.col.isOpenEnd())
		_idx.col.setRange(0, _idx.col.front() + 3);

    // Store the results of the transformation in the target
    // table
	if (!bInverseTrafo)
	{
		if (_idx.row.isOpenEnd())
			_idx.row.setRange(0, _idx.row.front() + (int)round(_fftData.GetNx() / 2.0) + 1);

		for (long long int i = 0; i < (int)round(_fftData.GetNx() / 2.0) + 1; i++)
		{
			if (i > _idx.row.size())
				break;

			_data.writeToTable(_idx.row[i], _idx.col.front(), sTargetTable, 2.0 * (double)(i)*dNyquistFrequency / (double)(_fftData.GetNx()));

			if (!bComplex)
			{
				_data.writeToTable(_idx.row[i], _idx.col[1], sTargetTable, std::abs(_fftData.a[i]));

				// Stitch phase overflows into a continous array
				if (i > 2 && detectPhaseOverflow(&_fftData.a[i-2]))
				{
					if (std::arg(_fftData.a[i - 1]) - std::arg(_fftData.a[i - 2]) < 0.0)
						dPhaseOffset -= 2 * M_PI;
					else if (std::arg(_fftData.a[i - 1]) - std::arg(_fftData.a[i - 2]) > 0.0)
						dPhaseOffset += 2 * M_PI;
				}

				_data.writeToTable(_idx.row[i], _idx.col[2], sTargetTable, std::arg(_fftData.a[i]) + dPhaseOffset);
			}
			else
			{
				_data.writeToTable(i, _idx.col[1], sTargetTable, _fftData.a[i].real());
				_data.writeToTable(i, _idx.col[2], sTargetTable, _fftData.a[i].imag());
			}
		}

		// Write headlines
		_data.setCacheStatus(true);
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

	if (_option.getSystemPrintStatus())
		NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_DONE")) + ".\n");

	return true;
}


/////////////////////////////////////////////////
/// \brief This function calculates the fast
/// wavelet transform of the passed data set.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \return bool
///
/// The user may select the wavelet type from a
/// predefined set of wavelets and determine,
/// whether an inverse transform shall be
/// calculated.
/////////////////////////////////////////////////
bool fastWaveletTransform(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option)
{
	vector<double> vWaveletData;
	vector<double> vAxisData;
	Indices _idx;

	bool bInverseTrafo = false;
	bool bTargetGrid = false;
	string sTargetTable = "fwtdata";
	string sType = "d"; // d = daubechies, cd = centered daubechies, h = haar, ch = centered haar, b = bspline, cb = centered bspline
	int k = 4;

	if (findParameter(sCmd, "inverse"))
		bInverseTrafo = true;

	if (findParameter(sCmd, "grid"))
		bTargetGrid = true;

	if (findParameter(sCmd, "type", '='))
		sType = getArgAtPos(sCmd, findParameter(sCmd, "type", '=') + 4);

	if (findParameter(sCmd, "k", '='))
	{
		_parser.SetExpr(getArgAtPos(sCmd, findParameter(sCmd, "k", '=') + 1));
		k = (int)_parser.Eval();
	}


	// search for explicit "target" options and select the target cache
	sTargetTable = evaluateTargetOptionInCommand(sCmd, sTargetTable, _idx, _parser, _data, _option);

	if (findParameter(sCmd, "inverse") || findParameter(sCmd, "type", '=') || findParameter(sCmd, "k", '='))
	{
		for (unsigned int i = 0; i < sCmd.length(); i++)
		{
			if (sCmd[i] == '(')
				i += getMatchingParenthesis(sCmd.substr(i));

			if (sCmd[i] == '-')
			{
				sCmd.erase(i);
				break;
			}
		}
	}

	sCmd = sCmd.substr(sCmd.find(' ', sCmd.find("fwt")));
	StripSpaces(sCmd);

	// get the data from the data object
	NumeRe::Table _table = parser_extractData(sCmd, _parser, _data, _option);

	if (_option.getSystemPrintStatus())
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

		if (_option.getSystemPrintStatus())
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

	_data.setCacheStatus(true);

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

	if (_option.getSystemPrintStatus())
		NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_DONE")) + ".\n");

	_data.setCacheStatus(false);
	return true;
}


/////////////////////////////////////////////////
/// \brief This function samples a defined
/// expression in an array of discrete values.
///
/// \param sCmd string&
/// \param _data Datafile&
/// \param _parser Parser&
/// \param _option const Settings&
/// \param _functions Define&
/// \return bool
///
/////////////////////////////////////////////////
bool evalPoints(string& sCmd, Datafile& _data, Parser& _parser, const Settings& _option, Define& _functions)
{
	unsigned int nSamples = 100;
	double dLeft = 0.0;
	double dRight = 0.0;
	double* dVar = 0;
	double dTemp = 0.0;
	string sExpr = "";
	string sParams = "";
	string sInterval = "";
	string sVar = "x";
	bool bLogarithmic = false;

	if (sCmd.find("-set") != string::npos)
	{
		sExpr = sCmd.substr(0, sCmd.find("-set"));
		sParams = sCmd.substr(sCmd.find("-set"));
	}
	else if (sCmd.find("--") != string::npos)
	{
		sExpr = sCmd.substr(0, sCmd.find("--"));
		sParams = sCmd.substr(sCmd.find("--"));
	}
	else
		sExpr = sCmd;

	StripSpaces(sExpr);
	sExpr = sExpr.substr(findCommand(sExpr).sString.length());

    if (!_functions.call(sExpr) || !_functions.call(sParams))
        return false;

	StripSpaces(sParams);

	if (sExpr.find("data(") != string::npos || _data.containsTablesOrClusters(sExpr))
	{
		getDataElements(sExpr, _parser, _data, _option);

		if (sExpr.find("{") != string::npos)
			convertVectorToExpression(sExpr, _option);
	}

	if (sParams.find("data(") != string::npos || _data.containsTablesOrClusters(sParams))
	{
		getDataElements(sParams, _parser, _data, _option);

		if (sParams.find("{") != string::npos && NumeReKernel::getInstance()->getStringParser().isStringExpression(sParams))
			convertVectorToExpression(sParams, _option);
	}

	if (findParameter(sParams, "samples", '='))
	{
		sParams += " ";

		if (isNotEmptyExpression(getArgAtPos(sParams, findParameter(sParams, "samples", '=') + 7)))
		{
			_parser.SetExpr(getArgAtPos(sParams, findParameter(sParams, "samples", '=') + 7));
			nSamples = (unsigned int)_parser.Eval();
		}

		sParams.erase(findParameter(sParams, "samples", '=') - 1, 8);
	}

	if (findParameter(sParams, "logscale"))
	{
		bLogarithmic = true;
		sParams.erase(findParameter(sParams, "logscale") - 1, 8);
	}

	if (sParams.find('=') != string::npos
			|| (sParams.find('[') != string::npos
				&& sParams.find(']', sParams.find('['))
				&& sParams.find(':', sParams.find('['))))
	{
		if (sParams.substr(0, 2) == "--")
			sParams = sParams.substr(2);
		else if (sParams.substr(0, 4) == "-set")
			sParams = sParams.substr(4);

        vector<double> vInterval = readAndParseIntervals(sParams, _parser, _data, _functions, _option, false);

        if (!vInterval.size() && sParams.find('=') != string::npos)
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

			auto indices = getAllIndices(sInterval);

			_parser.SetExpr(indices[0] + "," + indices[1]);
			int nIndices;
			double* res = _parser.Eval(nIndices);
			vInterval.assign(res, res+2);
		}

		if (vInterval.size() > 4)
            sVar = "z";
        else if (vInterval.size() > 2)
            sVar = "y";

		if (isNotEmptyExpression(sExpr))
			_parser.SetExpr(sExpr);
		else
			_parser.SetExpr(sVar);

		_parser.Eval();
		dVar = getPointerToVariable(sVar, _parser);

		if (!dVar)
			throw SyntaxError(SyntaxError::EVAL_VAR_NOT_FOUND, sCmd, sVar, sVar);

        dLeft = vInterval[0];
        dRight = vInterval[1];

        if (isnan(dLeft) && isnan(dRight))
        {
            dLeft = -10.0;
            dRight = 10.0;
        }
        else if (isnan(dLeft) || isnan(dRight) || isinf(dLeft) || isinf(dRight))
        {
            sCmd = "nan";
            return false;
        }

		if (bLogarithmic && (dLeft <= 0.0 || dRight <= 0.0))
			throw SyntaxError(SyntaxError::WRONG_PLOT_INTERVAL_FOR_LOGSCALE, sCmd, SyntaxError::invalid_position);
	}

	if (isNotEmptyExpression(sExpr))
		_parser.SetExpr(sExpr);
	else if (dVar)
		_parser.SetExpr(sVar);
	else
		_parser.SetExpr("0");

	_parser.Eval();
	sCmd = "";
	vector<double> vResults;

	if (dVar)
	{
		dTemp = *dVar;
		*dVar = dLeft;
		vResults.push_back(_parser.Eval());

		for (unsigned int i = 1; i < nSamples; i++)
		{
			if (bLogarithmic)
				*dVar = pow(10.0, log10(dLeft) + i * (log10(dRight) - log10(dLeft)) / (double)(nSamples - 1));
			else
				*dVar = dLeft + i * (dRight - dLeft) / (double)(nSamples - 1);

			vResults.push_back(_parser.Eval());
		}

		*dVar = dTemp;
	}
	else
	{
		for (unsigned int i = 0; i < nSamples; i++)
			vResults.push_back(_parser.Eval());
	}

	sCmd = "_~evalpnts[~_~]";
	_parser.SetVectorVar("_~evalpnts[~_~]", vResults);

	return true;
}


/////////////////////////////////////////////////
/// \brief This function calculates a datagrid
/// from passed functions or (x-y-z) data values.
///
/// \param sCmd string&
/// \param sTargetCache string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _functions Define&
/// \param _option const Settings&
/// \return bool
///
/////////////////////////////////////////////////
bool createDatagrid(string& sCmd, string& sTargetCache, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
	unsigned int nSamples = 100;
	string sXVals = "";
	string sYVals = "";
	string sZVals = "";

	Indices _iTargetIndex;

	bool bTranspose = false;

	vector<double> vXVals;
	vector<double> vYVals;
	vector<vector<double> > vZVals;

	if (sCmd.find("data(") != string::npos && !_data.isValid())
		throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);

	if (_data.containsTablesOrClusters(sCmd) && !_data.isValidCache())
		throw SyntaxError(SyntaxError::NO_CACHED_DATA, sCmd, SyntaxError::invalid_position);

	// Extract the z expression from the command line
	if (sCmd.find("-set") != string::npos || sCmd.find("--") != string::npos)
	{
		sZVals = sCmd.substr(findCommand(sCmd).sString.length() + findCommand(sCmd).nPos);

		if (sCmd.find("-set") != string::npos)
		{
			sCmd.erase(0, sCmd.find("-set"));
			sZVals.erase(sZVals.find("-set"));
		}
		else
		{
			sCmd.erase(0, sCmd.find("--"));
			sZVals.erase(sZVals.find("--"));
		}

		StripSpaces(sZVals);
	}

	// Get the intervals
	if (sCmd.find('[') != string::npos && sCmd.find(']', sCmd.find('[')) != string::npos)
	{
		sXVals = sCmd.substr(sCmd.find('[') + 1, sCmd.find(']', sCmd.find('[')) - sCmd.find('[') - 1);
		StripSpaces(sXVals);

		if (sXVals.find(',') != string::npos)
		{
		    auto args = getAllArguments(sXVals);
		    sXVals = args[0];
            sYVals = args[1];
		}

		if (sXVals == ":")
			sXVals = "-10:10";

		if (sYVals == ":")
			sYVals = "-10:10";
	}

	// Validate the intervals
	if ((!findParameter(sCmd, "x", '=') && !sXVals.length()) || (!findParameter(sCmd, "y", '=') && !sYVals.length()) || (!findParameter(sCmd, "z", '=') && !sZVals.length()))
		throw SyntaxError(SyntaxError::TOO_FEW_ARGS, sCmd, SyntaxError::invalid_position, "datagrid");

	// Get the number of samples from the option list
	if (findParameter(sCmd, "samples", '='))
	{
		_parser.SetExpr(getArgAtPos(sCmd, findParameter(sCmd, "samples", '=') + 7));
		nSamples = (unsigned int)_parser.Eval();

		if (nSamples < 2)
			throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, sCmd, SyntaxError::invalid_position);

		sCmd.erase(sCmd.find(getArgAtPos(sCmd, findParameter(sCmd, "samples", '=') + 7), findParameter(sCmd, "samples", '=') - 1), getArgAtPos(sCmd, findParameter(sCmd, "samples", '=') + 7).length());
		sCmd.erase(findParameter(sCmd, "samples", '=') - 1, 8);
	}

	// search for explicit "target" options and select the target cache
	sTargetCache = evaluateTargetOptionInCommand(sCmd, sTargetCache, _iTargetIndex, _parser, _data, _option);

	// read the transpose option
	if (findParameter(sCmd, "transpose"))
	{
		bTranspose = true;
		sCmd.erase(findParameter(sCmd, "transpose") - 1, 9);
	}

	// Read the interval definitions from the option list, if they are included
	// Remove them from the command expression
	if (!sXVals.length())
	{
		sXVals = getArgAtPos(sCmd, findParameter(sCmd, "x", '=') + 1);
		sCmd.erase(sCmd.find(getArgAtPos(sCmd, findParameter(sCmd, "x", '=') + 1), findParameter(sCmd, "x", '=') - 1), getArgAtPos(sCmd, findParameter(sCmd, "x", '=') + 1).length());
		sCmd.erase(findParameter(sCmd, "x", '=') - 1, 2);
	}

	if (!sYVals.length())
	{
		sYVals = getArgAtPos(sCmd, findParameter(sCmd, "y", '=') + 1);
		sCmd.erase(sCmd.find(getArgAtPos(sCmd, findParameter(sCmd, "y", '=') + 1), findParameter(sCmd, "y", '=') - 1), getArgAtPos(sCmd, findParameter(sCmd, "y", '=') + 1).length());
		sCmd.erase(findParameter(sCmd, "y", '=') - 1, 2);
	}

	if (!sZVals.length())
	{
		while (sCmd[sCmd.length() - 1] == ' ' || sCmd[sCmd.length() - 1] == '=' || sCmd[sCmd.length() - 1] == '-')
			sCmd.erase(sCmd.length() - 1);
		sZVals = getArgAtPos(sCmd, findParameter(sCmd, "z", '=') + 1);
	}

	// Try to call the functions
	if (!_functions.call(sZVals))
		throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, sZVals, sZVals);

	// Get the samples
	vector<size_t> vSamples = getSamplesForDatagrid(sCmd, sZVals, nSamples, _parser, _data, _option);

	//>> X-Vector (Switch the samples depending on the "transpose" command line option)
	vXVals = extractVectorForDatagrid(sCmd, sXVals, sZVals, vSamples[bTranspose], _parser, _data, _option);

	//>> Y-Vector (Switch the samples depending on the "transpose" command line option)
	vYVals = extractVectorForDatagrid(sCmd, sYVals, sZVals, vSamples[1 - bTranspose], _parser, _data, _option);

	//>> Z-Matrix
	if (sZVals.find("data(") != string::npos || _data.containsTablesOrClusters(sZVals))
	{
		// Get the datagrid from another table
		DataAccessParser _accessParser(sZVals);

		if (!_accessParser.getDataObject().length() || _accessParser.isCluster())
            throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sZVals, SyntaxError::invalid_position);

		Indices& _idx = _accessParser.getIndices();

		// identify the table
		string& szDatatable = _accessParser.getDataObject();

		// Check the indices
		if (!isValidIndexSet(_idx))
			throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

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
            throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, sCmd, SyntaxError::invalid_position);

        // Expand the z vector into a matrix for the datagrid if necessary
        expandVectorToDatagrid(vXVals, vYVals, vZVals, vSamples[bTranspose], vSamples[1 - bTranspose]);
	}
	else
	{
		// Calculate the grid from formula
		_parser.SetExpr(sZVals);

		vector<double> vVector;

		for (unsigned int x = 0; x < vXVals.size(); x++)
		{
			parser_iVars.vValue[0][0] = vXVals[x];

			for (unsigned int y = 0; y < vYVals.size(); y++)
			{
				parser_iVars.vValue[1][0] = vYVals[y];
				vVector.push_back(_parser.Eval());
			}

			vZVals.push_back(vVector);
			vVector.clear();
		}
	}

	// Store the results in the target cache
	if (_iTargetIndex.row.isOpenEnd())
		_iTargetIndex.row.setRange(0, _iTargetIndex.row.front() + vXVals.size() - 1);

	if (_iTargetIndex.col.isOpenEnd())
		_iTargetIndex.col.setRange(0, _iTargetIndex.col.front() + vYVals.size() + 1);

	_data.setCacheStatus(true);

	// Write the x axis
	for (size_t i = 0; i < vXVals.size(); i++)
		_data.writeToTable(i, _iTargetIndex.col[0], sTargetCache, vXVals[i]);

	_data.setHeadLineElement(_iTargetIndex.col[0], sTargetCache, "x");

	// Write the y axis
	for (size_t i = 0; i < vYVals.size(); i++)
		_data.writeToTable(i, _iTargetIndex.col[1], sTargetCache, vYVals[i]);

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
				_data.setHeadLineElement(_iTargetIndex.col[j+2], sTargetCache, "z[" + toString((int)j + 1) + "]");
		}
	}

	_data.setCacheStatus(false);

	return true;
}


/////////////////////////////////////////////////
/// \brief This function will obtain the samples
/// of the datagrid for each spatial direction.
///
/// \param sCmd const string&
/// \param sZVals const string&
/// \param nSamples size_t
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \return vector<size_t>
///
/////////////////////////////////////////////////
static vector<size_t> getSamplesForDatagrid(const string& sCmd, const string& sZVals, size_t nSamples, Parser& _parser, Datafile& _data, const Settings& _option)
{
	vector<size_t> vSamples;

	// If the z vals are inside of a table then obtain the correct number of samples here
	if (sZVals.find("data(") != string::npos || _data.containsTablesOrClusters(sZVals))
	{
		// Get the indices and identify the table name
		DataAccessParser _accessParser(sZVals);

		if (!_accessParser.getDataObject().length() || _accessParser.isCluster())
            throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sZVals, SyntaxError::invalid_position);

        Indices& _idx = _accessParser.getIndices();
        string& sZDatatable = _accessParser.getDataObject();

		// Check the indices
		if (!isValidIndexSet(_idx))
			throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

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
		throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, sCmd, SyntaxError::invalid_position);

	return vSamples;
}


/////////////////////////////////////////////////
/// \brief This function will extract the x or y
/// vectors which are needed as axes for the
/// datagrid.
///
/// \param sCmd const string&
/// \param sVectorVals string&
/// \param sZVals const string&
/// \param nSamples size_t
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \return vector<double>
///
/////////////////////////////////////////////////
static vector<double> extractVectorForDatagrid(const string& sCmd, string& sVectorVals, const string& sZVals, size_t nSamples, Parser& _parser, Datafile& _data, const Settings& _option)
{
	vector<double> vVectorVals;

	// Data direct from the table, not an index pair
	if ((sVectorVals.find("data(") != string::npos || _data.containsTablesOrClusters(sVectorVals)) && sVectorVals.find(':', getMatchingParenthesis(sVectorVals.substr(sVectorVals.find('('))) + sVectorVals.find('(')) == string::npos)
	{
		// Get the indices
		Indices _idx = getIndices(sVectorVals, _parser, _data, _option);

		// Identify the table
		string sDatatable = "data";

		if (_data.containsTablesOrClusters(sVectorVals))
		{
			for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
			{
				if (sVectorVals.find(iter->first + "(") != string::npos
						&& (!sVectorVals.find(iter->first + "(")
							|| (sVectorVals.find(iter->first + "(") && checkDelimiter(sVectorVals.substr(sVectorVals.find(iter->first + "(") - 1, (iter->first).length() + 2)))))
				{
					sDatatable = iter->first;
					break;
				}
			}
		}

		// Check the indices
		if (!isValidIndexSet(_idx))
			throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

        // The indices are vectors
        if (_idx.col.isOpenEnd())
            _idx.col.setRange(0, _data.getCols(sDatatable)-1);

        if (_idx.row.isOpenEnd() && _idx.col.size() > 1)
            throw SyntaxError(SyntaxError::NO_MATRIX, sCmd, SyntaxError::invalid_position);

        if (_idx.row.isOpenEnd())
            _idx.row.setRange(0, _data.getLines(sDatatable, true) - _data.getAppendedZeroes(_idx.col.front(), sDatatable)-1);

        if (sZVals.find("data(") != string::npos || _data.containsTablesOrClusters(sZVals))
        {
            // Only if the z values are also a table read the vector from the table
            vVectorVals = _data.getElement(_idx.row, _idx.col, sDatatable);
        }
        else
        {
            // Otherwise use minimal and maximal values
            double dMin = _data.min(sDatatable, _idx.row, _idx.col);
            double dMax = _data.max(sDatatable, _idx.row, _idx.col);

            for (unsigned int i = 0; i < nSamples; i++)
                vVectorVals.push_back((dMax - dMin) / double(nSamples - 1)*i + dMin);
        }
	}
	else if (sVectorVals.find(':') != string::npos)
	{
		// Index pair - If the index pair contains data elements, get their values now
		if (sVectorVals.find("data(") != string::npos || _data.containsTablesOrClusters(sVectorVals))
			getDataElements(sVectorVals, _parser, _data, _option);

		if (sVectorVals.find("{") != string::npos)
			throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

		// Replace the colon with a comma and parse the vector vals
		sVectorVals.replace(sVectorVals.find(':'), 1, ",");
		_parser.SetExpr(sVectorVals);

		// Get the results
		double* dResult = 0;
		int nNumResults = 0;
		dResult = _parser.Eval(nNumResults);

		if (nNumResults < 2)
			throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

		// Fill the vector vals with the needed number of samples
		for (unsigned int i = 0; i < nSamples; i++)
			vVectorVals.push_back(dResult[0] + (dResult[1] - dResult[0]) / double(nSamples - 1)*i);
	}
	else
		throw SyntaxError(SyntaxError::SEPARATOR_NOT_FOUND, sCmd, SyntaxError::invalid_position);

	return vVectorVals;
}


/////////////////////////////////////////////////
/// \brief This function will expand the z vector
/// into a z matrix using triangulation.
///
/// \param vXVals vector<double>&
/// \param vYVals vector<double>&
/// \param vZVals vector<vector<double>>&
/// \param nSamples_x size_t
/// \param nSamples_y size_t
/// \return void
///
/////////////////////////////////////////////////
static void expandVectorToDatagrid(vector<double>& vXVals, vector<double>& vYVals, vector<vector<double>>& vZVals, size_t nSamples_x, size_t nSamples_y)
{
	vector<double> vVector;

	// Only if a dimension is a singleton
	if (vZVals.size() == 1 || vZVals[0].size() == 1)
	{
		// construct the needed MGL objects
		mglData _mData[4];
		mglGraph _graph;

		// Prepare the memory
		_mData[0].Create(nSamples_x, nSamples_y);
		_mData[1].Create(vXVals.size());
		_mData[2].Create(vYVals.size());

		if (vZVals.size() != 1)
			_mData[3].Create(vZVals.size());
		else
			_mData[3].Create(vZVals[0].size());

		// copy the x and y vectors
		for (unsigned int i = 0; i < vXVals.size(); i++)
			_mData[1].a[i] = vXVals[i];
		for (unsigned int i = 0; i < vYVals.size(); i++)
			_mData[2].a[i] = vYVals[i];

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

		vXVals.clear();
		vYVals.clear();
		vZVals.clear();

		// Refill the x and y vectors
		for (unsigned int i = 0; i < nSamples_x; i++)
			vXVals.push_back(_mData[1].Minimal() + (_mData[1].Maximal() - _mData[1].Minimal()) / (double)(nSamples_x - 1)*i);

		for (unsigned int i = 0; i < nSamples_y; i++)
			vYVals.push_back(_mData[2].Minimal() + (_mData[2].Maximal() - _mData[2].Minimal()) / (double)(nSamples_y - 1)*i);

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
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _functions Define&
/// \param _option const Settings&
/// \return bool
///
/////////////////////////////////////////////////
bool writeAudioFile(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
	using namespace little_endian_io;

	ofstream fAudio;
	string sAudioFileName = "<savepath>/audiofile.wav";
	string sDataset = "";
	int nSamples = 44100;
	int nChannels = 1;
	int nBPS = 16;
	unsigned int nDataChunkPos = 0;
	unsigned int nFileSize = 0;
	const double dValMax = 32760.0;
	double dMax = 0.0;
	Indices _idx;
	Matrix _mDataSet;
	sCmd.erase(0, findCommand(sCmd).nPos + findCommand(sCmd).sString.length()); // Kommando entfernen

	// Strings parsen
	if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
	{
		string sDummy = "";
		NumeReKernel::getInstance()->getStringParser().evalAndFormat(sCmd, sDummy, true);
	}

	// Funktionen aufrufen
	if (!_functions.call(sCmd))
		throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, SyntaxError::invalid_position);

	// Samples lesen
	if (findParameter(sCmd, "samples", '='))
	{
		string sSamples = getArgAtPos(sCmd, findParameter(sCmd, "samples", '=') + 7);

		if (sSamples.find("data(") != string::npos || _data.containsTablesOrClusters(sSamples))
			getDataElements(sSamples, _parser, _data, _option);

		_parser.SetExpr(sSamples);

		if (!isnan(_parser.Eval()) && !isinf(_parser.Eval()) && _parser.Eval() >= 1)
            nSamples = (int)_parser.Eval();
	}

	// Dateiname lesen
	if (findParameter(sCmd, "file", '='))
		sAudioFileName = getArgAtPos(sCmd, findParameter(sCmd, "file", '=') + 4);

	if (sAudioFileName.find('/') == string::npos && sAudioFileName.find('\\') == string::npos)
		sAudioFileName.insert(0, "<savepath>/");

	// Dateiname pruefen
	sAudioFileName = _data.ValidFileName(sAudioFileName, ".wav");

	// Indices lesen
	_idx = getIndices(sCmd, _parser, _data, _option);
	sDataset = sCmd.substr(0, sCmd.find('('));
	StripSpaces(sDataset);

    if (_idx.row.isOpenEnd())
        _idx.row.setRange(0, _data.getLines(sDataset, false)-1);

    if (_idx.col.isOpenEnd())
        _idx.col.setRange(0, _idx.col.front() + 1);

    if (_idx.col.size() > 2)
        return false;

    if (fabs(_data.max(sDataset, _idx.row, _idx.col)) > fabs(_data.min(sDataset, _idx.row, _idx.col)))
        dMax = fabs(_data.max(sDataset, _idx.row, _idx.col));
    else
        dMax = fabs(_data.min(sDataset, _idx.row, _idx.col));

    _mDataSet.push_back(_data.getElement(_idx.row, VectorIndex(_idx.col[0]), sDataset));

    if (_idx.col.size() == 2)
        _mDataSet.push_back(_data.getElement(_idx.row, VectorIndex(_idx.col[1]), sDataset));

    _mDataSet = transposeMatrix(_mDataSet);

	for (unsigned int i = 0; i < _mDataSet.size(); i++)
	{
		for (unsigned int j = 0; j < _mDataSet[0].size(); j++)
			_mDataSet[i][j] = _mDataSet[i][j] / dMax * dValMax;
	}

	nChannels = _mDataSet[0].size();

	// Datenstream oeffnen
	fAudio.open(sAudioFileName.c_str(), ios::binary);

	if (fAudio.fail())
		return false;

	// Wave Header
	fAudio << "RIFF----WAVEfmt ";
	write_word(fAudio, 16, 4);
	write_word(fAudio, 1, 2);
	write_word(fAudio, nChannels, 2);
	write_word(fAudio, nSamples, 4);
	write_word(fAudio, (nSamples * nBPS * nChannels) / 8, 4);
	write_word(fAudio, 2 * nChannels, 2);
	write_word(fAudio, nBPS, 2);

	nDataChunkPos = fAudio.tellp();
	fAudio << "data----";

	// Audio-Daten schreiben
	for (unsigned int i = 0; i < _mDataSet.size(); i++)
	{
		for (unsigned int j = 0; j < _mDataSet[0].size(); j++)
			write_word(fAudio, intCast(_mDataSet[i][j]), 2);
	}

	// Chunk sizes nachtraeglich einfuegen
	nFileSize = fAudio.tellp();
	fAudio.seekp(nDataChunkPos + 4);
	write_word(fAudio, nFileSize - nDataChunkPos + 8, 4);
	fAudio.seekp(4);
	write_word(fAudio, nFileSize - 8, 4);
	fAudio.close();
	return true;
}


/////////////////////////////////////////////////
/// \brief This function regularizes the samples
/// of a defined x-y-data array such that DeltaX
/// is equal for every x.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _functions Define&
/// \param _option const Settings&
/// \return bool
///
/// \todo Examine the mglRefill algorithm, as it
/// most probably does something else than
/// expected.
/////////////////////////////////////////////////
bool regularizeDataSet(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
	int nSamples = 100;
	string sDataset = "";
	string sColHeaders[2] = {"", ""};
	Indices _idx;
	mglData _x, _v;
	double dXmin, dXmax;
	sCmd.erase(0, findCommand(sCmd).nPos + findCommand(sCmd).sString.length()); // Kommando entfernen

	// Strings parsen
	if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
	{
		string sDummy = "";
		NumeReKernel::getInstance()->getStringParser().evalAndFormat(sCmd, sDummy, true);
	}

	// Funktionen aufrufen
	if (!_functions.call(sCmd))
		throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, SyntaxError::invalid_position);

	// Samples lesen
	if (findParameter(sCmd, "samples", '='))
	{
		string sSamples = getArgAtPos(sCmd, findParameter(sCmd, "samples", '=') + 7);

		if (sSamples.find("data(") != string::npos || _data.containsTablesOrClusters(sSamples))
			getDataElements(sSamples, _parser, _data, _option);

		_parser.SetExpr(sSamples);

		if (!isnan(_parser.Eval()) && !isinf(_parser.Eval()) && _parser.Eval() >= 1)
            nSamples = (int)_parser.Eval();
	}

	// Indices lesen
	_idx = getIndices(sCmd, _parser, _data, _option);
	sDataset = sCmd.substr(0, sCmd.find('('));
	StripSpaces(sDataset);
	Datafile _cache;
	getData(sDataset, _idx, _data, _cache);

	sColHeaders[0] = _cache.getHeadLineElement(0, "cache") + "\\n(regularized)";
	sColHeaders[1] = _cache.getHeadLineElement(1, "cache") + "\\n(regularized)";

	long long int nLines = _cache.getLines("cache", false);

	dXmin = _cache.min("cache", 0, nLines - 1, 0);
	dXmax = _cache.max("cache", 0, nLines - 1, 0);

	_x.Create(nLines);
	_v.Create(nLines);

	for (long long int i = 0; i < nLines; i++)
	{
		_x.a[i] = _cache.getElement(i, 0, "cache");
		_v.a[i] = _cache.getElement(i, 1, "cache");
	}

	if (_x.nx != _v.GetNx())
		return false;

	if (!findParameter(sCmd, "samples", '='))
		nSamples = _x.GetNx();

	mglData _regularized(nSamples);
	_regularized.Refill(_x, _v, dXmin, dXmax); //wohin damit?

	long long int nLastCol = _data.getCols(sDataset, false);

	for (long long int i = 0; i < nSamples; i++)
	{
		_data.writeToTable(i, nLastCol, sDataset, dXmin + i * (dXmax - dXmin) / (nSamples - 1));
		_data.writeToTable(i, nLastCol + 1, sDataset, _regularized.a[i]);
	}

	_data.setHeadLineElement(nLastCol, sDataset, sColHeaders[0]);
	_data.setHeadLineElement(nLastCol + 1, sDataset, sColHeaders[1]);
	return true;
}


/////////////////////////////////////////////////
/// \brief This function performs a pulse
/// analysis on the selected data set.
///
/// \param _sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _functions Define&
/// \param _option const Settings&
/// \return bool
///
/// The function calculates the maximal amplitude,
/// its position, the FWHM, the width near the
/// maximal amplitude (which is different from
/// the FWHM) and the energy in the pulse.
/////////////////////////////////////////////////
bool analyzePulse(string& _sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
	string sDataset = "";
	Indices _idx;
	mglData _v;
	vector<double> vPulseProperties;
	double dXmin = NAN, dXmax = NAN;
	double dSampleSize = NAN;
	string sCmd = _sCmd.substr(findCommand(_sCmd, "pulse").nPos + 5);

	// Strings parsen
	if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
	{
		string sDummy = "";
		NumeReKernel::getInstance()->getStringParser().evalAndFormat(sCmd, sDummy, true);
	}

	// Funktionen aufrufen
	if (!_functions.call(sCmd))
		throw SyntaxError(SyntaxError::FUNCTION_ERROR, _sCmd, SyntaxError::invalid_position);

	// Indices lesen
	_idx = getIndices(sCmd, _parser, _data, _option);
	sDataset = sCmd.substr(0, sCmd.find('('));
	StripSpaces(sDataset);
	Datafile _cache;
	getData(sDataset, _idx, _data, _cache);

	long long int nLines = _cache.getLines("cache", false);

	dXmin = _cache.min("cache", 0, nLines - 1, 0);
	dXmax = _cache.max("cache", 0, nLines - 1, 0);

	_v.Create(nLines);

	for (long long int i = 0; i < nLines; i++)
		_v.a[i] = _cache.getElement(i, 1, "cache");

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
		vPulseProperties.push_back(NAN);
		_sCmd.replace(findCommand(_sCmd, "pulse").nPos, string::npos, "_~pulse[~_~]");
		_parser.SetVectorVar("_~pulse[~_~]", vPulseProperties);

		return true;
	}

	// Ausgabe
	if (_option.getSystemPrintStatus())
	{
		NumeReKernel::toggleTableStatus();
		make_hline();
		NumeReKernel::print("NUMERE: " + toUpperCase(_lang.get("PARSERFUNCS_PULSE_HEADLINE")));
		make_hline();

		for (unsigned int i = 0; i < vPulseProperties.size(); i++)
			NumeReKernel::printPreFmt("|   " + _lang.get("PARSERFUNCS_PULSE_TABLE_" + toString((int)i + 1) + "_*", toString(vPulseProperties[i], _option)) + "\n");

		NumeReKernel::toggleTableStatus();
		make_hline();
	}

	_sCmd.replace(findCommand(_sCmd, "pulse").nPos, string::npos, "_~pulse[~_~]");
	_parser.SetVectorVar("_~pulse[~_~]", vPulseProperties);

	return true;
}


/////////////////////////////////////////////////
/// \brief This function performs the short-time
/// fourier analysis on the passed data set.
///
/// \param sCmd string&
/// \param sTargetCache string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _functions Define&
/// \param _option const Settings&
/// \return bool
///
/////////////////////////////////////////////////
bool shortTimeFourierAnalysis(string& sCmd, string& sTargetCache, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
	string sDataset = "";
	Indices _idx, _target;
	mglData _real, _imag, _result;
	int nSamples = 0;

	double dXmin = NAN, dXmax = NAN;
	double dFmin = 0.0, dFmax = 1.0;
	double dSampleSize = NAN;
	sCmd.erase(0, findCommand(sCmd).nPos + 4);

	// Strings parsen
	if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
	{
		string sDummy = "";
		NumeReKernel::getInstance()->getStringParser().evalAndFormat(sCmd, sDummy, true);
	}

	// Funktionen aufrufen
	if (!_functions.call(sCmd))
		throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, SyntaxError::invalid_position);

	if (findParameter(sCmd, "samples", '='))
	{
		_parser.SetExpr(getArgAtPos(sCmd, findParameter(sCmd, "samples", '=') + 7));
		nSamples = _parser.Eval();

		if (nSamples < 0)
			nSamples = 0;
	}

	if (findParameter(sCmd, "target", '='))
	{
		sTargetCache = getArgAtPos(sCmd, findParameter(sCmd, "target", '=') + 6);

        if (!_data.isTable(sTargetCache))
            _data.addTable(sTargetCache, _option);

		_target = getIndices(sTargetCache, _parser, _data, _option);
		sTargetCache.erase(sTargetCache.find('('));

		if (sTargetCache == "data")
			throw SyntaxError(SyntaxError::READ_ONLY_DATA, sCmd, SyntaxError::invalid_position);

		if (!isValidIndexSet(_target))
			throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
	}
	else
	{
		_target.row = VectorIndex(0LL, VectorIndex::OPEN_END);
		_target.col.front() = 0;

		if (_data.isTable("stfdat()"))
			_target.col.front() += _data.getCols("stfdat", false);
        else
            _data.addTable("stfdat()", _option);

		sTargetCache = "stfdat";
		_target.col.back() = VectorIndex::OPEN_END;
	}


	// Indices lesen
	_idx = getIndices(sCmd, _parser, _data, _option);
	sDataset = sCmd.substr(0, sCmd.find('('));
	StripSpaces(sDataset);
	Datafile _cache;
	getData(sDataset, _idx, _data, _cache);

	sDataset = _cache.getHeadLineElement(1, "cache");

	long long int nLines = _cache.getLines("cache", false);

	dXmin = _cache.min("cache", 0, nLines - 1, 0);
	dXmax = _cache.max("cache", 0, nLines - 1, 0);

	_real.Create(nLines);
	_imag.Create(nLines);

	for (long long int i = 0; i < nLines; i++)
		_real.a[i] = _cache.getElement(i, 1, "cache");

	if (!nSamples || nSamples > _real.GetNx())
		nSamples = _real.GetNx() / 32;

	// Tatsaechliche STFA
	_result = mglSTFA(_real, _imag, nSamples);

	dSampleSize = (dXmax - dXmin) / ((double)_result.GetNx() - 1.0);

	// Nyquist: _real.GetNx()/(dXmax-dXmin)/2.0
	dFmax = _real.GetNx() / (dXmax - dXmin) / 2.0;

	// Zielcache befuellen entsprechend der Fourier-Algorithmik

	if (_target.row.isOpenEnd())
		_target.row.setRange(0, _target.row.front() + _result.GetNx() - 1);//?

	if (_target.col.isOpenEnd())
		_target.col.setRange(0, _target.col.front() + _result.GetNy() + 1); //?

	// UPDATE DATA ELEMENTS
	for (int i = 0; i < _result.GetNx(); i++)
		_data.writeToTable(i, _target.col.front(), sTargetCache, dXmin + i * dSampleSize);

	_data.setHeadLineElement(_target.col.front(), sTargetCache, sDataset);
	dSampleSize = 2 * (dFmax - dFmin) / ((double)_result.GetNy() - 1.0);

	for (int i = 0; i < _result.GetNy() / 2; i++)
		_data.writeToTable(i, _target.col[1], sTargetCache, dFmin + i * dSampleSize); // Fourier f Hier ist was falsch

	_data.setHeadLineElement(_target.col[1], sTargetCache, "f [Hz]");

	for (int i = 0; i < _result.GetNx(); i++)
	{
		if (_target.row[i] == VectorIndex::INVALID)
			break;

		for (int j = 0; j < _result.GetNy() / 2; j++)
		{
			if (_target.col[j+2] == VectorIndex::INVALID)
				break;

			_data.writeToTable(_target.row[i], _target.col[j+2], sTargetCache, _result[i + (j + _result.GetNy() / 2)*_result.GetNx()]);

			if (!i)
				_data.setHeadLineElement(_target.col[j+2], sTargetCache, "A[" + toString((int)j + 1) + "]");
		}
	}

	return true;
}


/////////////////////////////////////////////////
/// \brief This function approximates the passed
/// data set using cubic splines.
///
/// \param sCmd string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _functions Define&
/// \param _option const Settings&
/// \return bool
///
/// The calculated spline polynomials are defined
/// as new custom functions.
/////////////////////////////////////////////////
bool calculateSplines(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
	Indices _idx;
	Datafile _cache;
	tk::spline _spline;
	vector<double> xVect, yVect;
	string sTableName = sCmd.substr(sCmd.find(' '));
	StripSpaces(sTableName);

	_idx = getIndices(sTableName, _parser, _data, _option);
	sTableName.erase(sTableName.find('('));
	getData(sTableName, _idx, _data, _cache);

	long long int nLines = _cache.getLines("cache", true) - _cache.getAppendedZeroes(0, "cache");

	if (nLines < 2)
		throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, sCmd, sTableName);

	for (long long int i = 0; i < nLines; i++)
	{
		xVect.push_back(_cache.getElement(i, 0, "cache"));
		yVect.push_back(_cache.getElement(i, 1, "cache"));
	}

	_spline.set_points(xVect, yVect);

	string sDefinition = "Spline(x) := ";
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

	if (_option.getSystemPrintStatus() && !NumeReKernel::bSupressAnswer)
		NumeReKernel::print(sDefinition);

    bool bDefinitionSuccess = false;

	if (_functions.isDefined(sDefinition.substr(0, sDefinition.find(":="))))
		bDefinitionSuccess = _functions.defineFunc(sDefinition, true);
	else
		bDefinitionSuccess = _functions.defineFunc(sDefinition);

    if (bDefinitionSuccess)
        NumeReKernel::print(_lang.get("DEFINE_SUCCESS"), _option.getSystemPrintStatus() && !NumeReKernel::bSupressAnswer);
    else
        NumeReKernel::issueWarning(_lang.get("DEFINE_FAILURE"));

	return true;
}


