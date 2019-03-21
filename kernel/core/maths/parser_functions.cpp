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


#include "parser_functions.hpp"
#include "../../kernel.hpp"
#include "spline.h"
#include "wavelet.hpp"

value_type vAns;
Integration_Vars parser_iVars;
//extern bool bSupressAnswer;
extern mglGraph _fontData;
Plugin _plugin;

const string sParserVersion = "1.0.2";
string parser_evalTargetExpression(string& sCmd, const string& sDefaultTarget, Indices& _idx, Parser& _parser, Datafile& _data, const Settings& _option);
static double parser_LocalizeExtremum(string& sCmd, double* dVarAdress, Parser& _parser, const Settings& _option, double dLeft, double dRight, double dEps = 1e-10, int nRecursion = 0);
static double parser_LocalizeZero(string& sCmd, double* dVarAdress, Parser& _parser, const Settings& _option, double dLeft, double dRight, double dEps = 1e-10, int nRecursion = 0);
static vector<size_t> parser_getSamplesForDatagrid(const string& sCmd, const string& sZVals, size_t nSamples, Parser& _parser, Datafile& _data, const Settings& _option);
static vector<double> parser_extractVectorForDatagrid(const string& sCmd, string& sVectorVals, const string& sZVals, size_t nSamples, Parser& _parser, Datafile& _data, const Settings& _option);
static void parser_expandVectorToDatagrid(vector<double>& vXVals, vector<double>& vYVals, vector<vector<double>>& vZVals, size_t nSamples_x, size_t nSamples_y);

// This static function prints the selected unit, its description
// its dimension and its value conversion to the terminal
void printUnits(const string& sUnit, const string& sDesc, const string& sDim, const string& sValues, unsigned int nWindowsize)
{
	NumeReKernel::printPreFmt("|     " + strlfill(sUnit, 11) + strlfill(sDesc, (nWindowsize - 17) / 3 + (nWindowsize + 1) % 3) + strlfill(sDim, (nWindowsize - 35) / 3) + "=" + strfill(sValues, (nWindowsize - 2) / 3) + "\n");
	return;
}


// This function checks, whether the selected variable occurs
// in the current expression
bool parser_CheckVarOccurence(Parser& _parser, const string_type& sVar)
{
	// Ensure that the current expression has been parsed
	_parser.Eval();

	// Get a map containing the variables used in the
	// parsed expression
	varmap_type variables = _parser.GetUsedVar();

	// Return false, if the map doesn't contain any elements
	if (!variables.size())
		return false;

    // Try to find the selected variable
    return variables.find(sVar) != variables.end();
}

// This function searches for the selected variable in the passed
// string and returns the position of the first occurence or
// string::npos, if nothing was found
size_t parser_findVariable(const string& sExpr, const string& sVarName)
{
    size_t nMatch = 0;
    const static string sDelimiter = "+-*/,^!%&|()?:{}[]#<>='";

    // search the first match of the token, which is surrounded by the
    // defined separator characters
    while ((nMatch = sExpr.find(sVarName, nMatch)) != string::npos)
    {
        if ((!nMatch || sDelimiter.find(sExpr[nMatch-1]) != string::npos)
            && (nMatch + sVarName.length() >= sExpr.length() || sDelimiter.find(sExpr[nMatch+sVarName.length()]) != string::npos))
        {
            return nMatch;
        }
        nMatch++;
    }

    return string::npos;
}

// Calculate the integral of a function or a data set
// in one dimension
vector<double> parser_Integrate(const string& sCmd, Datafile& _data, Parser& _parser, const Settings& _option, Define& _functions)
{
	string sParams = "";        // Parameter-string
	string sInt_Line[4];        // Array, in das alle Eingaben gespeichert werden
	string sLabel = "";
	value_type* v = 0;
	int nResults = 0;
	vector<double> vResult;   // Ausgabe-Wert
	vector<double> fx_n[3]; // Werte an der Stelle n und n+1
	bool bNoIntVar = false;     // Boolean: TRUE, wenn die Funktion eine Konstante der Integration ist
	bool bLargeInterval = false;    // Boolean: TRUE, wenn ueber ein grosses Intervall integriert werden soll
	bool bReturnFunctionPoints = false;
	bool bCalcXvals = false;
	int nSign = 1;              // Vorzeichen, falls die Integrationsgrenzen getauscht werden muessen
	unsigned int nMethod = 1;    // 1 = trapezoidal, 2 = simpson

	sInt_Line[2] = "1e-3";
	parser_iVars.vValue[0][3] = 1e-3;

	// It's not possible to calculate the integral of a string expression
	if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
	{
		throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "integrate");
	}

	if (_option.getSystemPrintStatus())
		NumeReKernel::printPreFmt("                                              \r");

	// Separate function from the parameter string
	if (sCmd.find("-set") != string::npos)
	{
		sParams = sCmd.substr(sCmd.find("-set"));
		sInt_Line[3] = sCmd.substr(9, sCmd.find("-set") - 9);
	}
	else if (sCmd.find("--") != string::npos)
	{
		sParams = sCmd.substr(sCmd.find("--"));
		sInt_Line[3] = sCmd.substr(9, sCmd.find("--") - 9);
	}
	else if (sCmd.length() > 9)
		sInt_Line[3] = sCmd.substr(9);

	StripSpaces(sInt_Line[3]);

	// Ensure that the function is available
	if (!sInt_Line[3].length())
		throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, sCmd, SyntaxError::invalid_position);

	// If needed, prompt for the integration function
	if (sInt_Line[3].length() && sInt_Line[3].find("??") != string::npos)
		sInt_Line[3] = parser_Prompt(sInt_Line[3]);

	StripSpaces(sInt_Line[3]);

	// If the integration function contains a data object,
	// the calculation is done way different from the usual
	// integration
	if ((sInt_Line[3].substr(0, 5) == "data(" || _data.isCacheElement(sInt_Line[3]))
			&& getMatchingParenthesis(sInt_Line[3]) != string::npos
			&& sInt_Line[3].find_first_not_of(' ', getMatchingParenthesis(sInt_Line[3]) + 1) == string::npos) // xvals
	{
	    // Extract the integration interval
		if (sParams.length() && matchParams(sParams, "x", '='))
		{
			sInt_Line[0] = getArgAtPos(sParams, matchParams(sParams, "x", '=') + 1);

			// Replace the colon with a comma
			if (sInt_Line[0].find(':') != string::npos)
				sInt_Line[0].replace(sInt_Line[0].find(':'), 1, ",");

            // Set the interval expression and evaluate it
			_parser.SetExpr(sInt_Line[0]);
			v = _parser.Eval(nResults);

			if (nResults > 1)
				parser_iVars.vValue[0][2] = v[1];

			parser_iVars.vValue[0][1] = v[0];
		}

		// Are the samples of the integral desired?
		if (sParams.length() && matchParams(sParams, "points"))
			bReturnFunctionPoints = true;

		// Are the corresponding x values desired?
		if (sParams.length() && matchParams(sParams, "xvals"))
			bCalcXvals = true;

        // Get table name and the corresponding indices
		string sDatatable = sInt_Line[3].substr(0, sInt_Line[3].find('('));
		Indices _idx = parser_getIndices(sInt_Line[3], _parser, _data, _option);

		// Calculate the integral of the data set
		if (_idx.vI.size())
		{
		    // The indices are vectors
		    //
            // If it is a single column or row, then we simply
            // summarize its contents, otherwise we calculate the
            // integral with the trapezoidal method
			if (_idx.vI.size() == 1 || _idx.vJ.size() == 1)
				vResult.push_back(_data.sum(sDatatable, _idx.vI, _idx.vJ));
			else
			{
				Datafile _cache;

				// Copy the data
				for (unsigned int i = 0; i < _idx.vI.size(); i++)
				{
					_cache.writeToCache(i, 0, "cache", _data.getElement(_idx.vI[i], _idx.vJ[0], sDatatable));
					_cache.writeToCache(i, 1, "cache", _data.getElement(_idx.vI[i], _idx.vJ[1], sDatatable));
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
					if (sInt_Line[0].length() && parser_iVars.vValue[0][1] > _cache.getElement(i, 0, "cache"))
						continue;
					if (sInt_Line[0].length() && parser_iVars.vValue[0][2] < _cache.getElement(i + j, 0, "cache"))
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
					{
						vResult.push_back(_cache.getElement(i + j, 0, "cache"));
					}
				}

				// If the integral was calculated, then there is a
				// single result, which hasn't been stored yet
				if (!bReturnFunctionPoints && !bCalcXvals)
					vResult.push_back(dResult);
			}
		}
		else
		{
		    // The indices are regular
		    //
            // If it is a single column or row, then we simply
            // summarize its contents, otherwise we calculate the
            // integral with the trapezoidal method
			if (_idx.nI[1] == -1 || _idx.nJ[1] == -1)
			{
				parser_evalIndices(sDatatable, _idx, _data);
				vResult.push_back(_data.sum(sDatatable, _idx.nI[0], _idx.nI[1], _idx.nJ[0], _idx.nJ[1]));
			}
			else
			{
				parser_evalIndices(sDatatable, _idx, _data);
				Datafile _cache;

				// Copy the data
				for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
				{
					_cache.writeToCache(i - _idx.nI[0], 0, "cache", _data.getElement(i, _idx.nJ[0], sDatatable));
					_cache.writeToCache(i - _idx.nI[0], 1, "cache", _data.getElement(i, _idx.nJ[1], sDatatable));
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
					if (sInt_Line[0].length() && parser_iVars.vValue[0][1] > _cache.getElement(i, 0, "cache"))
						continue;
					if (sInt_Line[0].length() && parser_iVars.vValue[0][2] < _cache.getElement(i + j, 0, "cache"))
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
					{
						vResult.push_back(_cache.getElement(i + j, 0, "cache"));
					}
				}

				// If the integral was calculated, then there is a
				// single result, which hasn't been stored yet
				if (!bReturnFunctionPoints)
					vResult.push_back(dResult);
			}
		}

		// Return the result of the integral
		return vResult;
	}

	// No data set for integration
	if (sInt_Line[3].find("{") != string::npos)
		parser_VectorToExpr(sInt_Line[3], _option);
	sLabel = sInt_Line[3];

    // Call custom defined functions
	if (sInt_Line[3].length() && !_functions.call(sInt_Line[3]))
	{
		sInt_Line[3] = "";
		sLabel = "";
		throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, sCmd, SyntaxError::invalid_position);
	}

	// Evaluate the parameters
	if (sParams.length())
	{
		int nPos = 0;
		if (matchParams(sParams, "precision", '='))
		{
			nPos = matchParams(sParams, "precision", '=') + 9;
			sInt_Line[2] = getArgAtPos(sParams, nPos);
			StripSpaces(sInt_Line[2]);
			if (isNotEmptyExpression(sInt_Line[2]))
			{
				_parser.SetExpr(sInt_Line[2]);
				parser_iVars.vValue[0][3] = _parser.Eval();
				if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
				{
					vResult.push_back(NAN);
					return vResult;
				}
				if (!parser_iVars.vValue[0][3])
					sInt_Line[2] = "";
			}
		}
		if (matchParams(sParams, "p", '='))
		{
			nPos = matchParams(sParams, "p", '=') + 1;
			sInt_Line[2] = getArgAtPos(sParams, nPos);
			StripSpaces(sInt_Line[2]);
			if (isNotEmptyExpression(sInt_Line[2]))
			{
				_parser.SetExpr(sInt_Line[2]);
				parser_iVars.vValue[0][3] = _parser.Eval();
				if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
				{
					vResult.push_back(NAN);
					return vResult;
				}
				if (!parser_iVars.vValue[0][3])
					sInt_Line[2] = "";
			}
		}
		if (matchParams(sParams, "eps", '='))
		{
			nPos = matchParams(sParams, "eps", '=') + 3;
			sInt_Line[2] = getArgAtPos(sParams, nPos);
			StripSpaces(sInt_Line[2]);
			if (isNotEmptyExpression(sInt_Line[2]))
			{
				_parser.SetExpr(sInt_Line[2]);
				parser_iVars.vValue[0][3] = _parser.Eval();
				if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
				{
					vResult.push_back(NAN);
					return vResult;
				}
				if (!parser_iVars.vValue[0][3])
					sInt_Line[2] = "";
			}
		}
		if (matchParams(sParams, "x", '='))
		{
			nPos = matchParams(sParams, "x", '=') + 1;
			sInt_Line[0] = getArgAtPos(sParams, nPos);
			StripSpaces(sInt_Line[0]);
			if (sInt_Line[0].find(':') != string::npos)
			{
				sInt_Line[0] = "(" + sInt_Line[0] + ")";
				parser_SplitArgs(sInt_Line[0], sInt_Line[1], ':', _option);
				StripSpaces(sInt_Line[0]);
				StripSpaces(sInt_Line[1]);
				if (isNotEmptyExpression(sInt_Line[0]))
				{
					_parser.SetExpr(sInt_Line[0]);
					if (parser_CheckVarOccurence(_parser, parser_iVars.sName[0]))
					{
						sInt_Line[0] = "";
					}
					else
					{
						parser_iVars.vValue[0][1] = _parser.Eval();
						if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
						{
							vResult.push_back(NAN);
							return vResult;
						}
					}
				}
				if (isNotEmptyExpression(sInt_Line[1]))
				{
					_parser.SetExpr(sInt_Line[1]);
					if (parser_CheckVarOccurence(_parser, parser_iVars.sName[0]))
						sInt_Line[1] = "";
					else
					{
						parser_iVars.vValue[0][2] = _parser.Eval();
						if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
						{
							vResult.push_back(NAN);
							return vResult;
						}
					}
				}
				if (sInt_Line[0].length() && sInt_Line[1].length() && parser_iVars.vValue[0][1] == parser_iVars.vValue[0][2])
					throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
				if (!sInt_Line[0].length() || !sInt_Line[1].length())
					throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
			}
			else
				throw SyntaxError(SyntaxError::NO_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
		}
		if (matchParams(sParams, "method", '='))
		{
			nPos = matchParams(sParams, "method", '=') + 6;
			if (getArgAtPos(sParams, nPos) == "trapezoidal")
				nMethod = 1;
			if (getArgAtPos(sParams, nPos) == "simpson")
				nMethod = 2;
		}
		if (matchParams(sParams, "m", '='))
		{
			nPos = matchParams(sParams, "m", '=') + 1;
			if (getArgAtPos(sParams, nPos) == "trapezoidal")
				nMethod = 1;
			if (getArgAtPos(sParams, nPos) == "simpson")
				nMethod = 2;
		}
		if (matchParams(sParams, "steps", '='))
		{
			sInt_Line[2] = getArgAtPos(sParams, matchParams(sParams, "steps", '=') + 5);
			_parser.SetExpr(sInt_Line[2]);
			parser_iVars.vValue[0][3] = (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) / _parser.Eval();
		}
		if (matchParams(sParams, "s", '='))
		{
			sInt_Line[2] = getArgAtPos(sParams, matchParams(sParams, "s", '=') + 1);
			_parser.SetExpr(sInt_Line[2]);
			parser_iVars.vValue[0][3] = (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) / _parser.Eval();
		}
		if (matchParams(sParams, "points"))
			bReturnFunctionPoints = true;
		if (matchParams(sParams, "xvals"))
			bCalcXvals = true;
	}

	// Ensure that a function is available
	if (!sInt_Line[3].length())
	{
        throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, sCmd, SyntaxError::invalid_position);
	}

	// Check, whether the expression actual depends
	// upon the integration variable
	_parser.SetExpr(sInt_Line[3]);
	if (!parser_CheckVarOccurence(_parser, parser_iVars.sName[0]))
		bNoIntVar = true;       // Nein? Dann setzen wir den Bool auf TRUE und sparen uns viel Rechnung
	_parser.Eval(nResults);
	vResult.resize(nResults);

	// Set the calculation variables to their starting values
	for (int i = 0; i < 3; i++)
		fx_n[i].resize(nResults);
	for (int i = 0; i < nResults; i++)
	{
		vResult[i] = 0.0;
		for (int j = 0; j < 3; j++)
			fx_n[j][i] = 0.0;
	}

	// Ensure that interation ranges are available
	if (!sInt_Line[0].length() || !sInt_Line[1].length())
	{
	    throw SyntaxError(SyntaxError::NO_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
	}

	// Exchange borders, if necessary
	if (parser_iVars.vValue[0][2] < parser_iVars.vValue[0][1])
	{
		// --> Ja? Dann tauschen wir sie fuer die Berechnung einfach aus <--
		value_type vTemp = parser_iVars.vValue[0][1];
		parser_iVars.vValue[0][1] = parser_iVars.vValue[0][2];
		parser_iVars.vValue[0][2] = vTemp;
		nSign *= -1; // Beachten wir das Tauschen der Grenzen durch ein zusaetzliches Vorzeichen
	}

	// Calculate the numerical integration
	if (!bNoIntVar || bReturnFunctionPoints || bCalcXvals)
	{
	    // In this case, we have to calculate the integral
	    // numerically
		if (sInt_Line[2].length() && parser_iVars.vValue[0][3] > parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1])
			sInt_Line[2] = "";

        // If the precision is invalid (e.g. due to a very
        // small interval, simply guess a reasonable interval
        // here
		if (!sInt_Line[2].length())
		{
            parser_iVars.vValue[0][3] = (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) / 100;
		}

		// Ensure that the precision is not negative
		if (parser_iVars.vValue[0][3] < 0)
		{
			parser_iVars.vValue[0][3] *= -1;
		}

		// Calculate the x values, if desired
		if (bCalcXvals)
		{
			parser_iVars.vValue[0][0] = parser_iVars.vValue[0][1];
			vResult[0] = parser_iVars.vValue[0][0];
			while (parser_iVars.vValue[0][0] + parser_iVars.vValue[0][3] < parser_iVars.vValue[0][2])
			{
				parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3];
				vResult.push_back(parser_iVars.vValue[0][0]);
			}
			return vResult;
		}

		// Set the expression in the parser
		_parser.SetExpr(sInt_Line[3]);

		// Is it a large interval (then it will need more time)
		if ((parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) / parser_iVars.vValue[0][3] >= 9.9e6)
			bLargeInterval = true;

		// Do not allow a very high number of integration steps
		if ((parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) / parser_iVars.vValue[0][3] > 1e10)
			throw SyntaxError(SyntaxError::INVALID_INTEGRATION_PRECISION, sCmd, SyntaxError::invalid_position);

		// Set the integration variable to the lower border
		parser_iVars.vValue[0][0] = parser_iVars.vValue[0][1];

		// Calculate the first sample(s)
		v = _parser.Eval(nResults);
		for (int i = 0; i < nResults; i++)
			fx_n[0][i] = v[i];

		// Perform the actual numerical integration
		while (parser_iVars.vValue[0][0] + parser_iVars.vValue[0][3] < parser_iVars.vValue[0][2] + parser_iVars.vValue[0][3] * 1e-1)
		{
		    // Calculate the samples first
			if (nMethod == 1)
			{
				parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3]; // x + dx
				v = _parser.Eval(nResults);    // n+1-te Stuetzstelle auswerten
				for (int i = 0; i < nResults; i++)
				{
					fx_n[1][i] = v[i];    // n+1-te Stuetzstelle auswerten
					if (parser_iVars.vValue[0][0] > parser_iVars.vValue[0][2] && isnan(fx_n[1][i]))
						fx_n[1][i] = 0.0;
				}
			}
			else if (nMethod == 2)
			{
				parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3] / 2.0;
				v = _parser.Eval(nResults);
				for (int i = 0; i < nResults; i++)
				{
					fx_n[1][i] = v[i];
					if (parser_iVars.vValue[0][0] > parser_iVars.vValue[0][2] && isnan(fx_n[1][i]))
						fx_n[1][i] = 0.0;
				}
				parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3] / 2.0;
				v = _parser.Eval(nResults);
				for (int i = 0; i < nResults; i++)
				{
					fx_n[2][i] = v[i];
					if (parser_iVars.vValue[0][0] > parser_iVars.vValue[0][2] && isnan(fx_n[2][i]))
						fx_n[2][i] = 0.0;
				}
			}

			// Now calculate the area below the curve
			if (nMethod == 1)
			{
				if (!bReturnFunctionPoints)
				{
					for (int i = 0; i < nResults; i++)
					{
						vResult[i] += parser_iVars.vValue[0][3] * (fx_n[0][i] + fx_n[1][i]) * 0.5; // Durch ein Trapez annaehern!
						//cerr << vResult[i] << endl;
					}
				}
				else
				{
					if (vResult.size())
						vResult.push_back(parser_iVars.vValue[0][3] * (fx_n[0][0] + fx_n[1][0]) * 0.5 + vResult.back());
					else
						vResult.push_back(parser_iVars.vValue[0][3] * (fx_n[0][0] + fx_n[1][0]) * 0.5);
				}
			}
			else if (nMethod == 2)
			{
				if (!bReturnFunctionPoints)
				{
					for (int i = 0; i < nResults; i++)
						vResult[i] += parser_iVars.vValue[0][3] / 6.0 * (fx_n[0][i] + 4.0 * fx_n[1][i] + fx_n[2][i]); // b-a/6*(f(a)+4f(a+b/2)+f(b))
				}
				else
				{
					if (vResult.size())
						vResult.push_back(parser_iVars.vValue[0][3] / 6.0 * (fx_n[0][0] + 4.0 * fx_n[1][0] + fx_n[2][0]) + vResult.back());
					else
						vResult.push_back(parser_iVars.vValue[0][3] / 6.0 * (fx_n[0][0] + 4.0 * fx_n[1][0] + fx_n[2][0]));
				}
			}

			// Set the last sample as the first one
			if (nMethod == 1)
			{
				for (int i = 0; i < nResults; i++)
					fx_n[0][i] = fx_n[1][i];              // Wert der n+1-ten Stuetzstelle an die n-te Stuetzstelle zuweisen
			}
			else if (nMethod == 2)
			{
				for (int i = 0; i < nResults; i++)
					fx_n[0][i] = fx_n[2][i];
			}

			// Print a status value, if needed
			if (_option.getSystemPrintStatus() && bLargeInterval)
			{
				if (!bLargeInterval)
				{
					if ((int)((parser_iVars.vValue[0][0] - parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) * 20) > (int)((parser_iVars.vValue[0][0] - parser_iVars.vValue[0][3] - parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) * 20))
					{
						NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + toString((int)((parser_iVars.vValue[0][0] - parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) * 20) * 5) + " %");
					}
				}
				else
				{
					if ((int)((parser_iVars.vValue[0][0] - parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) * 100) > (int)((parser_iVars.vValue[0][0] - parser_iVars.vValue[0][3] - parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) * 100))
					{
						NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + toString((int)((parser_iVars.vValue[0][0] - parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) * 100)) + " %");
					}
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
		string sTemp = sInt_Line[3];
		sInt_Line[3].erase();

		// Apply the analytical solution
		while (sTemp.length())
			sInt_Line[3] += getNextArgument(sTemp, true) + "*" + parser_iVars.sName[0] + ",";
		sInt_Line[3].erase(sInt_Line[3].length() - 1, 1);

		// Calculate the integral analytically
		_parser.SetExpr(sInt_Line[3]);
		parser_iVars.vValue[0][0] = parser_iVars.vValue[0][2];
		v = _parser.Eval(nResults);
		for (int i = 0; i < nResults; i++)
			vResult[i] = v[i];
		parser_iVars.vValue[0][0] = parser_iVars.vValue[0][1];

		v = _parser.Eval(nResults);
		for (int i = 0; i < nResults; i++)
			vResult[i] -= v[i];
	}

	// Display a success message
	if (_option.getSystemPrintStatus() && bLargeInterval)
	{
		NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... 100 %: " + _lang.get("COMMON_SUCCESS") + "!\n");
	}

	return vResult;
}

// Calculate the integral of a function in two dimensions
vector<double> parser_Integrate_2(const string& sCmd, Datafile& _data, Parser& _parser, const Settings& _option, Define& _functions)
{
	string __sCmd = findCommand(sCmd).sString;
	string sLabel = "";
	string sParams = "";            // Parameter-string
	string sInt_Line[2][3];         // string-Array fuer die Integralgrenzen
	string sInt_Fct;                // string fuer die zu integrierende Funktion
	value_type* v = 0;
	int nResults = 0;
	vector<double> vResult[3];      // value_type-Array, wobei vResult[0] das eigentliche Ergebnis speichert
	// und vResult[1] fuer die Zwischenergebnisse des inneren Integrals ist
	vector<double> fx_n[2][3];          // value_type-Array fuer die jeweiligen Stuetzstellen im inneren und aeusseren Integral
	bool bIntVar[2] = {true, true}; // bool-Array, das speichert, ob und welche Integrationsvariablen in sInt_Fct enthalten sind
	bool bRenewBorder = false;      // bool, der speichert, ob die Integralgrenzen von x oder y abhaengen
	bool bLargeArray = false;       // bool, der TRUE fuer viele Datenpunkte ist;
	int nSign = 1;                  // Vorzeichen-Integer
	unsigned int nMethod = 1;       // trapezoidal = 1, simpson = 2

	sInt_Line[0][2] = "1e-3";
	parser_iVars.vValue[0][3] = 1e-3;
	parser_iVars.vValue[1][3] = 1e-3;

	// Strings may not be integrated
	if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
	{
		throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "integrate");
	}
	if (_option.getSystemPrintStatus())
		NumeReKernel::printPreFmt("                                              \r");

	// Extract integration function and parameter list
	if (sCmd.find("-set") != string::npos)
	{
		sParams = sCmd.substr(sCmd.find("-set"));
		sInt_Fct = sCmd.substr(__sCmd.length(), sCmd.find("-set") - __sCmd.length());
	}
	else if (sCmd.find("--") != string::npos)
	{
		sParams = sCmd.substr(sCmd.find("--"));
		sInt_Fct = sCmd.substr(__sCmd.length(), sCmd.find("--") - __sCmd.length());
	}
	else if (sCmd.length() > __sCmd.length())
		sInt_Fct = sCmd.substr(__sCmd.length());

	StripSpaces(sInt_Fct);

	// Ensure that the integration function is available
	if (!sInt_Fct.length())
		throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, sCmd, SyntaxError::invalid_position);

	// Prompt for an input, if necessary
	if (sInt_Fct.length() && sInt_Fct.find("??") != string::npos)
		sInt_Fct = parser_Prompt(sInt_Fct);

	// Expand the integration function, if necessary
	if (sInt_Fct.find("{") != string::npos)
		parser_VectorToExpr(sInt_Fct, _option);

	sLabel = sInt_Fct;

	// Try to call custom functions
	if (sInt_Fct.length() && !_functions.call(sInt_Fct))
	{
		throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, sCmd, SyntaxError::invalid_position);
	}

    // Evaluate the parameters
	if (sParams.length())
	{
		int nPos = 0;
		if (matchParams(sParams, "precision", '='))
		{
			nPos = matchParams(sParams, "precision", '=') + 9;
			sInt_Line[0][2] = getArgAtPos(sParams, nPos);
			StripSpaces(sInt_Line[0][2]);
			if (isNotEmptyExpression(sInt_Line[0][2]))
			{
				_parser.SetExpr(sInt_Line[0][2]);
				parser_iVars.vValue[0][3] = _parser.Eval();
				if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
				{
					vResult[0].push_back(NAN);
					return vResult[0];
				}
				if (!parser_iVars.vValue[0][3])
					sInt_Line[0][2] = "";
				else
					parser_iVars.vValue[1][3] = parser_iVars.vValue[0][3];
			}
		}
		if (matchParams(sParams, "p", '='))
		{
			nPos = matchParams(sParams, "p", '=') + 1;
			sInt_Line[0][2] = getArgAtPos(sParams, nPos);
			StripSpaces(sInt_Line[0][2]);
			if (isNotEmptyExpression(sInt_Line[0][2]))
			{
				_parser.SetExpr(sInt_Line[0][2]);
				parser_iVars.vValue[0][3] = _parser.Eval();
				if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
				{
					vResult[0].push_back(NAN);
					return vResult[0];
				}
				if (!parser_iVars.vValue[0][3])
					sInt_Line[0][2] = "";
				else
					parser_iVars.vValue[1][3] = parser_iVars.vValue[0][3];
			}
		}
		if (matchParams(sParams, "eps", '='))
		{
			nPos = matchParams(sParams, "eps", '=') + 3;
			sInt_Line[0][2] = getArgAtPos(sParams, nPos);
			StripSpaces(sInt_Line[0][2]);
			if (isNotEmptyExpression(sInt_Line[0][2]))
			{
				_parser.SetExpr(sInt_Line[0][2]);
				parser_iVars.vValue[0][3] = _parser.Eval();
				if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
				{
					vResult[0].push_back(NAN);
					return vResult[0];
				}
				if (!parser_iVars.vValue[0][3])
					sInt_Line[0][2] = "";
				else
					parser_iVars.vValue[1][3] = parser_iVars.vValue[0][3];
			}
		}
		if (matchParams(sParams, "x", '='))
		{
			nPos = matchParams(sParams, "x", '=') + 1;
			sInt_Line[0][0] = getArgAtPos(sParams, nPos);
			StripSpaces(sInt_Line[0][0]);
			if (sInt_Line[0][0].find(':') != string::npos)
			{
				sInt_Line[0][0] = "(" + sInt_Line[0][0] + ")";
				parser_SplitArgs(sInt_Line[0][0], sInt_Line[0][1], ':', _option);
				StripSpaces(sInt_Line[0][0]);
				StripSpaces(sInt_Line[0][1]);
				if (isNotEmptyExpression(sInt_Line[0][0]))
				{
					_parser.SetExpr(sInt_Line[0][0]);
					if (parser_CheckVarOccurence(_parser, parser_iVars.sName[0]) || parser_CheckVarOccurence(_parser, parser_iVars.sName[1]))
					{
						sInt_Line[0][0] = "";
					}
					else
					{
						parser_iVars.vValue[0][1] = _parser.Eval();
						if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
						{
							vResult[0].push_back(NAN);
							return vResult[0];
						}
					}
				}
				if (isNotEmptyExpression(sInt_Line[0][1]))
				{
					_parser.SetExpr(sInt_Line[0][1]);
					if (parser_CheckVarOccurence(_parser, parser_iVars.sName[0]) || parser_CheckVarOccurence(_parser, parser_iVars.sName[1]))
						sInt_Line[0][1] = "";
					else
					{
						parser_iVars.vValue[0][2] = _parser.Eval();
						if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
						{
							vResult[0].push_back(NAN);
							return vResult[0];
						}
					}
				}
				if (sInt_Line[0][0].length() && sInt_Line[0][1].length() && parser_iVars.vValue[0][1] == parser_iVars.vValue[0][2])
					throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
				if (!sInt_Line[0][0].length() || !sInt_Line[0][1].length())
					throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
			}
			else
				throw SyntaxError(SyntaxError::NO_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
		}
		if (matchParams(sParams, "y", '='))
		{
			nPos = matchParams(sParams, "y", '=') + 1;
			sInt_Line[1][0] = getArgAtPos(sParams, nPos);
			StripSpaces(sInt_Line[1][0]);
			if (sInt_Line[1][0].find(':') != string::npos)
			{
				sInt_Line[1][0] = "(" + sInt_Line[1][0] + ")";
				parser_SplitArgs(sInt_Line[1][0], sInt_Line[1][1], ':', _option);
				StripSpaces(sInt_Line[1][0]);
				StripSpaces(sInt_Line[1][1]);
				if (isNotEmptyExpression(sInt_Line[1][0]))
				{
					_parser.SetExpr(sInt_Line[1][0]);
					if (parser_CheckVarOccurence(_parser, parser_iVars.sName[1]))
					{
						sInt_Line[1][0] = "";
					}
					else
					{
						parser_iVars.vValue[1][1] = _parser.Eval();
						if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
						{
							vResult[0].push_back(NAN);
							return vResult[0];
						}
					}
				}
				if (isNotEmptyExpression(sInt_Line[1][1]))
				{
					_parser.SetExpr(sInt_Line[1][1]);
					if (parser_CheckVarOccurence(_parser, parser_iVars.sName[1]))
						sInt_Line[1][1] = "";
					else
					{
						parser_iVars.vValue[1][2] = _parser.Eval();
						if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
						{
							vResult[0].push_back(NAN);
							return vResult[0];
						}
					}
				}
				if (sInt_Line[1][0].length() && sInt_Line[1][1].length() && parser_iVars.vValue[1][1] == parser_iVars.vValue[1][2])
					throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
				if (!sInt_Line[1][0].length() || !sInt_Line[1][1].length())
					throw SyntaxError(SyntaxError::INVALID_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
			}
			else
				throw SyntaxError(SyntaxError::NO_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
		}
		if (matchParams(sParams, "method", '='))
		{
			nPos = matchParams(sParams, "method", '=') + 6;
			if (getArgAtPos(sParams, nPos) == "trapezoidal")
				nMethod = 1;
			if (getArgAtPos(sParams, nPos) == "simpson")
				nMethod = 2;
		}
		if (matchParams(sParams, "m", '='))
		{
			nPos = matchParams(sParams, "m", '=') + 1;
			if (getArgAtPos(sParams, nPos) == "trapezoidal")
				nMethod = 1;
			if (getArgAtPos(sParams, nPos) == "simpson")
				nMethod = 2;
		}
		if (matchParams(sParams, "steps", '='))
		{
			sInt_Line[0][2] = getArgAtPos(sParams, matchParams(sParams, "steps", '=') + 5);
			_parser.SetExpr(sInt_Line[0][2]);
			parser_iVars.vValue[0][3] = (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) / _parser.Eval();
			parser_iVars.vValue[1][3] = parser_iVars.vValue[0][3];
		}
		if (matchParams(sParams, "s", '='))
		{
			sInt_Line[0][2] = getArgAtPos(sParams, matchParams(sParams, "s", '=') + 1);
			_parser.SetExpr(sInt_Line[0][2]);
			parser_iVars.vValue[0][3] = (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) / _parser.Eval();
			parser_iVars.vValue[1][3] = parser_iVars.vValue[0][3];
		}
		/*if (matchParams(sParams, "noround") || matchParams(sParams, "nr"))
		    bDoRoundResults = false;*/
	}

    // Ensure that the integration function is available
	if (!sInt_Fct.length())
	{
        throw SyntaxError(SyntaxError::NO_INTEGRATION_FUNCTION, sCmd, SyntaxError::invalid_position);
	}

	// Check, whether the expression depends upon one or both
	// integration variables
	_parser.SetExpr(sInt_Fct);
	if (!parser_CheckVarOccurence(_parser, parser_iVars.sName[0]))
		bIntVar[0] = false;
	if (!parser_CheckVarOccurence(_parser, parser_iVars.sName[1]))
		bIntVar[1] = false;

    // Prepare the memory for integration
	_parser.Eval(nResults);
	for (int i = 0; i < 3; i++)
	{
		vResult[i].resize(nResults);
		fx_n[0][i].resize(nResults);
		fx_n[1][i].resize(nResults);
	}

	for (int i = 0; i < nResults; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			vResult[j][i] = 0.0;
			fx_n[0][j][i] = 0.0;
			fx_n[1][j][i] = 0.0;
		}
	}

	// Ensure that the integration ranges are available
	if (!sInt_Line[0][0].length() || !sInt_Line[0][1].length() || !sInt_Line[1][0].length() || !sInt_Line[1][1].length())
	{
	    throw SyntaxError(SyntaxError::NO_INTEGRATION_RANGES, sCmd, SyntaxError::invalid_position);
	}

	// Sort the intervals and track it with an additional sign
	//
	// First: x range
	if (parser_iVars.vValue[0][1] > parser_iVars.vValue[0][2])
	{
		value_type vTemp = parser_iVars.vValue[0][1];
		parser_iVars.vValue[0][1] = parser_iVars.vValue[0][2];
		parser_iVars.vValue[0][2] = vTemp;
		nSign *= -1;
	}

	// now y range
	if (parser_iVars.vValue[1][1] > parser_iVars.vValue[1][2])
	{
		value_type vTemp = parser_iVars.vValue[1][1];
		string_type sTemp = sInt_Line[1][0];
		parser_iVars.vValue[1][1] = parser_iVars.vValue[1][2];
		sInt_Line[1][0] = sInt_Line[1][1];
		parser_iVars.vValue[1][2] = vTemp;
		sInt_Line[1][1] = sTemp;
		nSign *= -1;
	}

	// Do the interval borders depend upon the first interval?
	_parser.SetExpr(sInt_Line[1][0] + " + " + sInt_Line[1][1]);
	if (parser_CheckVarOccurence(_parser, parser_iVars.sName[0]))
		bRenewBorder = true;    // Ja? Setzen wir den bool entsprechend

	// Does the expression depend upon at least one integration
	// variable?
	if (bIntVar[0] || bIntVar[1])
	{
		// Ensure that the precision is reasonble
		if (sInt_Line[0][2].length() && (parser_iVars.vValue[0][3] > parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]
										 || parser_iVars.vValue[0][3] > parser_iVars.vValue[1][2] - parser_iVars.vValue[1][1]))
			sInt_Line[0][2] = "";

        // If the precision is invalid, we guess a reasonable value here
		if (!sInt_Line[0][2].length())
		{
		    // We use the smallest intervall and split it into
		    // 100 parts
            parser_iVars.vValue[0][3] = min(parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1], parser_iVars.vValue[1][2] - parser_iVars.vValue[1][1]) / 100;
		}

		// Ensure that the precision is positive
		if (parser_iVars.vValue[0][3] < 0)
			parser_iVars.vValue[0][3] *= -1;

		/* --> Legacy: womoeglich sollen einmal unterschiedliche Praezisionen fuer "x" und "y"
		 *     moeglich sein. Inzwischen weisen wir hier einfach mal die Praezision von "x" an
		 *     die fuer "y" zu. <--
		 */
		parser_iVars.vValue[1][3] = parser_iVars.vValue[0][3];

		// Special case: the expression only depends upon "y" and not upon "x"
		// In this case, we switch everything, because the integration is much
		// faster in this case
		if ((bIntVar[1] && !bIntVar[0]) && !bRenewBorder)
		{
			NumeReKernel::printPreFmt(LineBreak("|INTEGRATE> " + _lang.get("PARSERFUNCS_INTEGRATE2_SWAPVARS", parser_iVars.sName[0], parser_iVars.sName[1]) + " ...", _option, false, 0, 12) + "\n");
			// --> Leerzeichen als "virtuelle Delimiter" hinzufuegen <--
			string_type sTempFct = " " + sInt_Fct + " ";
			sInt_Fct = "";
			do
			{
				/* --> Pruefen wir, ob die Umgebung der gefundenen Variable "y" zu den "Delimitern" gehoert. Anderenfalls
				 *     koennte es sich ja auch um einen Variablennamen handeln. <--
				 */
				if (checkDelimiter(sTempFct.substr(sTempFct.find(parser_iVars.sName[1]) - 1, parser_iVars.sName[1].length() + 2)))
				{
					int nToReplace = sTempFct.find(parser_iVars.sName[1]);
					sTempFct.replace(nToReplace, parser_iVars.sName[0].length(), parser_iVars.sName[0]);
					sInt_Fct += sTempFct.substr(0, nToReplace + 2);
					sTempFct = sTempFct.substr(nToReplace + 2);
				}
				else
				{
					/* --> Selbst wenn die gefunde Stelle sich nicht als Variable "y" erwiesen hat, muessen wir den Substring
					 *     an die Variable sInt_Fct zuweisen, da wir anderenfalls in einen Loop laufen <--
					 */
					sInt_Fct += sTempFct.substr(0, sTempFct.find(parser_iVars.sName[1]) + 2);
					sTempFct = sTempFct.substr(sTempFct.find(parser_iVars.sName[1]) + 2);
				}
				// --> Weisen wir den restlichen String an den Parser zu <--
				if (sTempFct.length())
					_parser.SetExpr(sTempFct);
				else // Anderenfalls koennen wir auch abbrechen; der gesamte String wurde kontrolliert
					break;
			}
			while (parser_CheckVarOccurence(_parser, parser_iVars.sName[1])); // so lange im restlichen String noch Variablen gefunden werden

			// --> Das Ende des Strings ggf. noch anfuegen <--
			if (sTempFct.length())
				sInt_Fct += sTempFct;
			// --> Ueberzaehlige Leerzeichen entfernen <--
			StripSpaces(sInt_Fct);

			// --> Strings tauschen <--
			string_type sTemp = sInt_Line[0][0];
			sInt_Line[0][0] = sInt_Line[1][0];
			sInt_Line[1][0] = sTemp;
			sTemp = sInt_Line[0][1];
			sInt_Line[0][1] = sInt_Line[1][1];
			sInt_Line[1][1] = sTemp;

			// --> Werte tauschen <---
			value_type vTemp = parser_iVars.vValue[0][1];
			parser_iVars.vValue[0][1] = parser_iVars.vValue[1][1];
			parser_iVars.vValue[1][1] = vTemp;
			vTemp = parser_iVars.vValue[0][2];
			parser_iVars.vValue[0][2] = parser_iVars.vValue[1][2];
			parser_iVars.vValue[1][2] = vTemp;
			bIntVar[0] = true;
			bIntVar[1] = false;
			NumeReKernel::printPreFmt("|INTEGRATE> " + _lang.get("COMMON_SUCCESS") + "!\n");
		}

		// Set the expression
		_parser.SetExpr(sInt_Fct);

		// Is it a very slow integration?
		if (((parser_iVars.vValue[0][1] - parser_iVars.vValue[0][0]) * (parser_iVars.vValue[1][1] - parser_iVars.vValue[1][0]) / parser_iVars.vValue[0][3] >= 1e3 && bIntVar[0] && bIntVar[1])
				|| ((parser_iVars.vValue[0][1] - parser_iVars.vValue[0][0]) * (parser_iVars.vValue[1][1] - parser_iVars.vValue[1][0]) / parser_iVars.vValue[0][3] >= 9.9e6 && (bIntVar[0] || bIntVar[1])))
			bLargeArray = true;

        // Avoid calculation with too many steps
		if (((parser_iVars.vValue[0][1] - parser_iVars.vValue[0][0]) * (parser_iVars.vValue[1][1] - parser_iVars.vValue[1][0]) / parser_iVars.vValue[0][3] > 1e10 && bIntVar[0] && bIntVar[1])
				|| ((parser_iVars.vValue[0][1] - parser_iVars.vValue[0][0]) * (parser_iVars.vValue[1][1] - parser_iVars.vValue[1][0]) / parser_iVars.vValue[0][3] > 1e10 && (bIntVar[0] || bIntVar[1])))
			throw SyntaxError(SyntaxError::INVALID_INTEGRATION_PRECISION, sCmd, SyntaxError::invalid_position);

		// --> Kleine Info an den Benutzer, dass der Code arbeitet <--
		if (_option.getSystemPrintStatus())
			NumeReKernel::printPreFmt("|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... 0 %");

		// --> Setzen wir "x" und "y" auf ihre Startwerte <--
		parser_iVars.vValue[0][0] = parser_iVars.vValue[0][1]; // x = x_0
		parser_iVars.vValue[1][0] = parser_iVars.vValue[1][1]; // y = y_0

		// --> Fall: "x" und "y" enthalten. Sehr umstaendlich und aufwaendig zu rechnen <--
		if (bIntVar[0] && bIntVar[1])
		{
			// --> Werte mit den Startwerten die erste Stuetzstelle fuer die y-Integration aus <--
			v = _parser.Eval(nResults);
			for (int i = 0; i < nResults; i++)
				fx_n[1][0][i] = v[i];

			/* --> Berechne das erste y-Integral fuer die erste Stuetzstelle fuer x
			 *     Die Schleife laeuft so lange wie y < y_1 <--
			 */
			while (parser_iVars.vValue[1][0] + parser_iVars.vValue[1][3] < parser_iVars.vValue[1][2] + parser_iVars.vValue[1][3] * 1e-1)
			{
				if (nMethod == 1)
				{
					parser_iVars.vValue[1][0] += parser_iVars.vValue[1][3]; // y + dy
					v = _parser.Eval(nResults); // Werte stelle n+1 aus
					for (int i = 0; i < nResults; i++)
					{
						fx_n[1][1][i] = v[i]; // Werte stelle n+1 aus
						if (parser_iVars.vValue[1][0] > parser_iVars.vValue[1][2] && isnan(fx_n[1][1][i]))
							fx_n[1][1][i] = 0.0;
						vResult[1][i] += parser_iVars.vValue[1][3] * (fx_n[1][0][i] + fx_n[1][1][i]) * 0.5; // Berechne das Trapez zu y
						fx_n[1][0][i] = fx_n[1][1][i];  // Weise Wert an Stelle n+1 an Stelle n zu
					}
				}
				else if (nMethod == 2)
				{
					parser_iVars.vValue[1][0] += parser_iVars.vValue[1][3] / 2.0;
					v = _parser.Eval(nResults);
					for (int i = 0; i < nResults; i++)
						fx_n[1][1][i] = v[i];
					parser_iVars.vValue[1][0] += parser_iVars.vValue[1][3] / 2.0;
					v = _parser.Eval(nResults);
					for (int i = 0; i < nResults; i++)
					{
						fx_n[1][2][i] = v[i];
						if (parser_iVars.vValue[1][0] > parser_iVars.vValue[1][2] && isnan(fx_n[1][1][i]))
							fx_n[1][1][i] = 0.0;
						if (parser_iVars.vValue[1][0] > parser_iVars.vValue[1][2] && isnan(fx_n[1][2][i]))
							fx_n[1][2][i] = 0.0;
						vResult[1][i] = parser_iVars.vValue[1][2] / 6 * (fx_n[1][0][i] + 4.0 * fx_n[1][1][i] + fx_n[1][2][i]);
						fx_n[1][0][i] = fx_n[1][2][i];
					}
				}
			}
			for (int i = 0; i < nResults; i++)
				fx_n[0][0][i] = vResult[1][i]; // Weise ersten Stelle fuer x zu
		}
		else
		{
			// --> Hier ist nur "x" oder nur "y" enthalten. Wir koennen uns das erste Integral sparen <--
			v = _parser.Eval(nResults);
			for (int i = 0; i < nResults; i++)
				fx_n[0][0][i] = v[i];
		}

		/* --> Das eigentliche, numerische Integral. Es handelt sich um nichts weiter als viele
		 *     while()-Schleifendurchlaeufe.
		 *     Die aeussere Schleife laeuft so lange x < x_1 ist. <--
		 */
		while (parser_iVars.vValue[0][0] + parser_iVars.vValue[0][3] < parser_iVars.vValue[0][2] + parser_iVars.vValue[0][3] * 1e-1)
		{
			if (nMethod == 1)
			{
				parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3]; // x + dx
				// --> Preufen wir, ob die Grenzen ggf. von "x" abhaengen <--
				if (bRenewBorder)
				{
					/* --> Ja? Dann muessen wir jedes Mal diese Grenzen neu auswerten (Sollte man in Zukunft
					 *     noch intelligenter loesen) <--
					 */
					_parser.SetExpr(sInt_Line[1][0]);
					parser_iVars.vValue[1][1] = _parser.Eval();
					_parser.SetExpr(sInt_Line[1][1]);
					parser_iVars.vValue[1][2] = _parser.Eval();
					_parser.SetExpr(sInt_Fct);
				}

				// --> Setzen wir "y" auf den Wert, der von der unteren y-Grenze vorgegeben wird <--
				parser_iVars.vValue[1][0] = parser_iVars.vValue[1][1];
				// --> Werten wir sofort die erste y-Stuetzstelle aus <--
				v = _parser.Eval(nResults);
				for (int i = 0; i < nResults; i++)
					fx_n[1][0][i] = v[i];

				// --> Setzen wir die vResult-Variable fuer die innere Schleife auf 0 <--
				for (int i = 0; i < nResults; i++)
					vResult[1][i] = 0.0;

				// --> Ist eigentlich sowohl "x" als auch "y" in f(x,y) (oder ggf. nur "y"?) vorhanden? <--
				if (bIntVar[1] && (!bIntVar[0] || bIntVar[0]))
				{
					// --> Ja? Dann muessen wir wohl diese Integration muehsam ausrechnen <--
					while (parser_iVars.vValue[1][0] + parser_iVars.vValue[1][3] < parser_iVars.vValue[1][2] + parser_iVars.vValue[1][3] * 1e-1) // so lange y < y_1
					{
						parser_iVars.vValue[1][0] += parser_iVars.vValue[1][3]; // y + dy
						v = _parser.Eval(nResults); // Werte stelle n+1 aus
						for (int i = 0; i < nResults; i++)
						{
							fx_n[1][1][i] = v[i]; // Werte stelle n+1 aus
							if (parser_iVars.vValue[1][0] > parser_iVars.vValue[1][2] && isnan(fx_n[1][1][i]))
								fx_n[1][0][i] = 0.0;
							vResult[1][i] += parser_iVars.vValue[1][3] * (fx_n[1][0][i] + fx_n[1][1][i]) * 0.5; // Berechne das Trapez zu y
							fx_n[1][0][i] = fx_n[1][1][i];  // Weise Wert an Stelle n+1 an Stelle n zu
						}
					}
				}
				else if (bIntVar[0] && !bIntVar[1])
				{
					/* --> Nein? Dann koennen wir das gesamte y-Integral durch ein Trapez berechnen. Dazu
					 *     setzen wir die Variable "y" auf den Wert der oberen Grenze und werten das Ergebnis
					 *     fuer die obere Stuetzstelle aus. Anschliessend berechnen wir mit diesen beiden Stuetz-
					 *     stellen und der Breite des (aktuellen) Integrationsintervalls die Flaeche des um-
					 *     schlossenen Trapezes <--
					 */
					parser_iVars.vValue[1][0] = parser_iVars.vValue[1][2];
					v = _parser.Eval(nResults);
					for (int i = 0; i < nResults; i++)
					{
						fx_n[1][1][i] = v[i];
						vResult[1][i] = (parser_iVars.vValue[1][2] - parser_iVars.vValue[1][1]) * (fx_n[1][0][i] + fx_n[1][1][i]) * 0.5;
						fx_n[1][0][i] = fx_n[1][1][i];
					}
				}
				// --> Weise das Ergebnis der y-Integration an die zweite Stuetzstelle der x-Integration zu <--
				for (int i = 0; i < nResults; i++)
				{
					fx_n[0][1][i] = vResult[1][i];
					if (parser_iVars.vValue[0][0] > parser_iVars.vValue[0][2] && isnan(fx_n[0][1][i]))
						fx_n[0][1][i] = 0.0;
					vResult[0][i] += parser_iVars.vValue[0][3] * (fx_n[0][0][i] + fx_n[0][1][i]) * 0.5; // Berechne das Trapez zu x
					fx_n[0][0][i] = fx_n[0][1][i]; // Weise den Wert der zweiten Stuetzstelle an die erste Stuetzstelle zu
				}
			}
			else if (nMethod == 2)
			{
				parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3] / 2.0; // x + dx
				// --> Preufen wir, ob die Grenzen ggf. von "x" abhaengen <--
				if (bRenewBorder)
				{
					/* --> Ja? Dann muessen wir jedes Mal diese Grenzen neu auswerten (Sollte man in Zukunft
					 *     noch intelligenter loesen) <--
					 */
					_parser.SetExpr(sInt_Line[1][0]);
					parser_iVars.vValue[1][1] = _parser.Eval();
					_parser.SetExpr(sInt_Line[1][1]);
					parser_iVars.vValue[1][2] = _parser.Eval();
					_parser.SetExpr(sInt_Fct);
				}

				// --> Setzen wir "y" auf den Wert, der von der unteren y-Grenze vorgegeben wird <--
				parser_iVars.vValue[1][0] = parser_iVars.vValue[1][1];
				// --> Werten wir sofort die erste y-Stuetzstelle aus <--
				v = _parser.Eval(nResults);
				for (int i = 0; i < nResults; i++)
					fx_n[1][0][i] = v[i];

				// --> Setzen wir die vResult-Variable fuer die innere Schleife auf 0 <--
				for (int i = 0; i < nResults; i++)
					vResult[1][i] = 0.0;

				// --> Ist eigentlich sowohl "x" als auch "y" in f(x,y) (oder ggf. nur "y"?) vorhanden? <--
				if (bIntVar[1] && (!bIntVar[0] || bIntVar[0]))
				{
					// --> Ja? Dann muessen wir wohl diese Inegration muehsam ausrechnen <--
					while (parser_iVars.vValue[1][0] + parser_iVars.vValue[1][3] < parser_iVars.vValue[1][2] + parser_iVars.vValue[1][3] * 1e-1) // so lange y < y_1
					{
						parser_iVars.vValue[1][0] += parser_iVars.vValue[1][3] / 2.0; // y + dy
						v = _parser.Eval(nResults); // Werte stelle n+1 aus
						for (int i = 0; i < nResults; i++)
						{
							fx_n[1][1][i] = v[i]; // Werte stelle n+1 aus
							if (parser_iVars.vValue[1][0] > parser_iVars.vValue[1][2] && isnan(fx_n[1][1][i]))
								fx_n[1][1][i] = 0.0;
						}
						parser_iVars.vValue[1][0] += parser_iVars.vValue[1][3] / 2.0; // y + dy
						v = _parser.Eval(nResults); // Werte stelle n+1 aus
						for (int i = 0; i < nResults; i++)
						{
							fx_n[1][2][i] = v[i]; // Werte stelle n+1 aus
							if (parser_iVars.vValue[1][0] > parser_iVars.vValue[1][2] && isnan(fx_n[1][2][i]))
								fx_n[1][2][i] = 0.0;
							vResult[1][i] += parser_iVars.vValue[1][3] / 6.0 * (fx_n[1][0][i] + 4.0 * fx_n[1][1][i] + fx_n[1][2][i]); // Berechne das Trapez zu y
							fx_n[1][0][i] = fx_n[1][2][i];  // Weise Wert an Stelle n+1 an Stelle n zu
						}
					}
				}
				else if (bIntVar[0] && !bIntVar[1])
				{
					/* --> Nein? Dann koennen wir das gesamte y-Integral durch ein Trapez berechnen. Dazu
					 *     setzen wir die Variable "y" auf den Wert der oberen Grenze und werten das Ergebnis
					 *     fuer die obere Stuetzstelle aus. Anschliessend berechnen wir mit diesen beiden Stuetz-
					 *     stellen und der Breite des (aktuellen) Integrationsintervalls die Flaeche des um-
					 *     schlossenen Trapezes <--
					 */
					parser_iVars.vValue[1][0] = (parser_iVars.vValue[1][1] + parser_iVars.vValue[1][2]) / 2.0;
					v = _parser.Eval(nResults);
					for (int i = 0; i < nResults; i++)
						fx_n[1][1][i] = v[i];
					parser_iVars.vValue[1][0] = parser_iVars.vValue[1][2];
					v = _parser.Eval(nResults);
					for (int i = 0; i < nResults; i++)
					{
						fx_n[1][2][i] = v[i];
						vResult[1][i] = (parser_iVars.vValue[1][2] - parser_iVars.vValue[1][1]) / 6.0 * (fx_n[1][0][i] + 4.0 * fx_n[1][1][i] + fx_n[1][2][i]);
					}
				}
				// --> Weise das Ergebnis der y-Integration an die zweite Stuetzstelle der x-Integration zu <--
				for (int i = 0; i < nResults; i++)
				{
					fx_n[0][1][i] = vResult[1][i];
					if (parser_iVars.vValue[0][0] > parser_iVars.vValue[0][2] && isnan(fx_n[0][1][i]))
						fx_n[0][1][i] = 0.0;
				}

				parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3] / 2.0; // x + dx
				// --> Preufen wir, ob die Grenzen ggf. von "x" abhaengen <--
				if (bRenewBorder)
				{
					/* --> Ja? Dann muessen wir jedes Mal diese Grenzen neu auswerten (Sollte man in Zukunft
					 *     noch intelligenter loesen) <--
					 */
					_parser.SetExpr(sInt_Line[1][0]);
					parser_iVars.vValue[1][1] = _parser.Eval();
					_parser.SetExpr(sInt_Line[1][1]);
					parser_iVars.vValue[1][2] = _parser.Eval();
					_parser.SetExpr(sInt_Fct);
				}

				// --> Setzen wir "y" auf den Wert, der von der unteren y-Grenze vorgegeben wird <--
				parser_iVars.vValue[1][0] = parser_iVars.vValue[1][1];
				// --> Setzen wir die vResult-Variable fuer die innere Schleife auf 0 <--
				for (int i = 0; i < nResults; i++)
					vResult[2][i] = 0.0;

				// --> Ist eigentlich sowohl "x" als auch "y" in f(x,y) (oder ggf. nur "y"?) vorhanden? <--
				if (bIntVar[1] && (!bIntVar[0] || bIntVar[0]))
				{
					// --> Ja? Dann muessen wir wohl diese Inegration muehsam ausrechnen <--
					while (parser_iVars.vValue[1][0] + parser_iVars.vValue[1][3] < parser_iVars.vValue[1][2] + parser_iVars.vValue[1][3] * 1e-1) // so lange y < y_1
					{
						parser_iVars.vValue[1][0] += parser_iVars.vValue[1][3] / 2.0; // y + dy
						v = _parser.Eval(nResults); // Werte stelle n+1 aus
						for (int i = 0; i < nResults; i++)
						{
							fx_n[1][1][i] = v[i]; // Werte stelle n+1 aus
							if (parser_iVars.vValue[1][0] > parser_iVars.vValue[1][2] && isnan(fx_n[1][1][i]))
								fx_n[1][1][i] = 0.0;
						}
						parser_iVars.vValue[1][0] += parser_iVars.vValue[1][3] / 2.0; // y + dy
						v = _parser.Eval(nResults); // Werte stelle n+1 aus
						for (int i = 0; i < nResults; i++)
						{
							fx_n[1][2][i] = v[i]; // Werte stelle n+1 aus
							if (parser_iVars.vValue[1][0] > parser_iVars.vValue[1][2] && isnan(fx_n[1][2][i]))
								fx_n[1][2][i] = 0.0;
							vResult[2][i] += parser_iVars.vValue[1][3] / 6.0 * (fx_n[1][0][i] + 4.0 * fx_n[1][1][i] + fx_n[1][2][i]); // Berechne das Trapez zu y
							fx_n[1][0][i] = fx_n[1][2][i];  // Weise Wert an Stelle n+1 an Stelle n zu
						}
					}
				}
				else if (bIntVar[0] && !bIntVar[1])
				{
					/* --> Nein? Dann koennen wir das gesamte y-Integral durch ein Trapez berechnen. Dazu
					 *     setzen wir die Variable "y" auf den Wert der oberen Grenze und werten das Ergebnis
					 *     fuer die obere Stuetzstelle aus. Anschliessend berechnen wir mit diesen beiden Stuetz-
					 *     stellen und der Breite des (aktuellen) Integrationsintervalls die Flaeche des um-
					 *     schlossenen Trapezes <--
					 */
					parser_iVars.vValue[1][0] = (parser_iVars.vValue[1][1] + parser_iVars.vValue[1][2]) / 2.0;
					v = _parser.Eval(nResults);
					for (int i = 0; i < nResults; i++)
						fx_n[1][1][i] = v[i];
					parser_iVars.vValue[1][0] = parser_iVars.vValue[1][2];
					v = _parser.Eval(nResults);
					for (int i = 0; i < nResults; i++)
					{
						fx_n[1][2][i] = v[i];
						vResult[2][i] = (parser_iVars.vValue[1][2] - parser_iVars.vValue[1][1]) / 6.0 * (fx_n[1][0][i] + 4.0 * fx_n[1][1][i] + fx_n[1][2][i]);
					}
				}
				// --> Weise das Ergebnis der y-Integration an die zweite Stuetzstelle der x-Integration zu <--
				for (int i = 0; i < nResults; i++)
				{
					fx_n[0][2][i] = vResult[2][i];
					if (parser_iVars.vValue[0][0] > parser_iVars.vValue[0][2] && isnan(fx_n[0][2][i]))
						fx_n[0][2][i] = 0.0;
					vResult[0][i] += parser_iVars.vValue[0][3] / 6.0 * (fx_n[0][0][i] + 4.0 * fx_n[0][1][i] + fx_n[0][2][i]); // Berechne das Trapez zu x
					fx_n[0][0][i] = fx_n[0][2][i]; // Weise den Wert der zweiten Stuetzstelle an die erste Stuetzstelle zu
				}
			}

			// Show some progress
			if (_option.getSystemPrintStatus())
			{
				if (!bLargeArray)
				{
					if ((int)((parser_iVars.vValue[0][0] - parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) * 20) > (int)((parser_iVars.vValue[0][0] - parser_iVars.vValue[0][3] - parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) * 20))
					{
						NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + toString((int)((parser_iVars.vValue[0][0] - parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) * 20) * 5) + " %");
					}
				}
				else
				{
					if ((int)((parser_iVars.vValue[0][0] - parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) * 100) > (int)((parser_iVars.vValue[0][0] - parser_iVars.vValue[0][3] - parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) * 100))
					{
						NumeReKernel::printPreFmt("\r|INTEGRATE> " + _lang.get("COMMON_EVALUATING") + " ... " + toString((int)((parser_iVars.vValue[0][0] - parser_iVars.vValue[0][1]) / (parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1]) * 100)) + " %");
					}
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
	else if (!bRenewBorder)
	{
		// In this case, the interval borders do not depend upon each other
		// and the expressin is also independent
		string sTemp = sInt_Fct;

		string sInt_Fct_2 = "";
		while (sTemp.length())
			sInt_Fct_2 += getNextArgument(sTemp, true) + "*" + parser_iVars.sName[0] + "*" + parser_iVars.sName[1] + ",";

		sInt_Fct_2.erase(sInt_Fct_2.length() - 1, 1);

		// --> Schnelle Loesung: Konstante x Flaeche, die vom Integral umschlossen wird <--
		parser_iVars.vValue[0][0] = parser_iVars.vValue[0][2] - parser_iVars.vValue[0][1];
		parser_iVars.vValue[1][0] = parser_iVars.vValue[1][2] - parser_iVars.vValue[1][1];
		_parser.SetExpr(sInt_Fct_2);
		v = _parser.Eval(nResults);
		for (int i = 0; i < nResults; i++)
			vResult[0][i] = v[i];
	}
	else
	{
		/* --> Doofer Fall: zwar eine Funktion, die weder von "x" noch von "y" abhaengt,
		 *     dafuer aber erfordert, dass die Grenzen des Integrals jedes Mal aktualisiert
		 *     werden. <--
		 */
		if (_option.getSystemPrintStatus())
			NumeReKernel::printPreFmt("|INTEGRATE> " + _lang.get("PARSERFUNCS_INTEGRATE_CONSTANT") + " ... ");
		// --> Waehle willkuerliche Praezision von 1e-4 <--
		parser_iVars.vValue[0][3] = 1e-4;
		parser_iVars.vValue[1][3] = 1e-4;
		// --> Setze "x" und "y" auf ihre unteren Grenzen <--
		parser_iVars.vValue[0][0] = parser_iVars.vValue[0][1];
		parser_iVars.vValue[1][0] = parser_iVars.vValue[1][1];
		// --> Werte erste x-Stuetzstelle aus <--
		v = _parser.Eval(nResults);
		for (int i = 0; i < nResults; i++)
			fx_n[0][0][i] = v[i];

		/* --> Berechne das eigentliche Integral. Unterscheidet sich nur begrenzt von dem oberen,
		 *     ausfuehrlichen Fall, ausser dass die innere Schleife aufgrund des Fehlens der Inte-
		 *     grationsvariablen "y" vollstaendig wegfaellt <--
		 */
		while (parser_iVars.vValue[0][0] + 1e-4 < parser_iVars.vValue[0][2] + 1e-5)
		{
			if (nMethod == 1)
			{
				parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3]; // x + dx

				// --> Erneuere die Werte der x- und y-Grenzen <--
				_parser.SetExpr(sInt_Line[1][0]);
				parser_iVars.vValue[1][1] = _parser.Eval();
				_parser.SetExpr(sInt_Line[1][1]);
				parser_iVars.vValue[1][2] = _parser.Eval();
				// --> Weise dem Parser wieder die Funktion f(x,y) zu <--
				_parser.SetExpr(sInt_Fct);
				// --> Setze "y" wieder auf die untere Grenze <--
				parser_iVars.vValue[1][0] = parser_iVars.vValue[1][1];
				// --> Setze den Speicher fuer die "innere" Integration auf 0 <--
				for (int i = 0; i < nResults; i++)
					vResult[1][i] = 0.0;

				// --> Werte erste y-Stuetzstelle aus <--
				v = _parser.Eval(nResults);
				for (int i = 0; i < nResults; i++)
					fx_n[1][0][i] = v[i];
				// --> Setze "y" auf die obere Grenze <--
				parser_iVars.vValue[1][0] = parser_iVars.vValue[1][2];
				// --> Werte die zweite Stuetzstelle aus <--
				v = _parser.Eval(nResults);
				for (int i = 0; i < nResults; i++)
				{
					fx_n[1][1][i] = v[i];
					// --> Berechne das y-Trapez <--
					vResult[1][i] = (parser_iVars.vValue[1][2] - parser_iVars.vValue[1][1]) * (fx_n[1][0][i] + fx_n[1][1][i]) * 0.5;

					// --> Weise das y-Ergebnis der zweiten x-Stuetzstelle zu <--
					fx_n[0][1][i] = vResult[1][i];
					vResult[0][i] += parser_iVars.vValue[0][3] * (fx_n[0][0][i] + fx_n[0][1][i]) * 0.5; // Berechne das Trapez zu x
					fx_n[0][0][i] = fx_n[0][1][i]; // Weise den Wert der zweiten Stuetzstelle an die erste Stuetzstelle zu
				}
			}
			else if (nMethod == 2)
			{
				parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3] / 2.0; // x + dx

				// --> Erneuere die Werte der x- und y-Grenzen <--
				_parser.SetExpr(sInt_Line[1][0]);
				parser_iVars.vValue[1][1] = _parser.Eval();
				_parser.SetExpr(sInt_Line[1][1]);
				parser_iVars.vValue[1][2] = _parser.Eval();
				// --> Weise dem Parser wieder die Funktion f(x,y) zu <--
				_parser.SetExpr(sInt_Fct);
				// --> Setze "y" wieder auf die untere Grenze <--
				parser_iVars.vValue[1][0] = parser_iVars.vValue[1][1];
				// --> Setze den Speicher fuer die "innere" Integration auf 0 <--
				for (int i = 0; i < nResults; i++)
					vResult[1][i] = 0.0;

				// --> Werte erste y-Stuetzstelle aus <--
				v = _parser.Eval(nResults);
				for (int i = 0; i < nResults; i++)
					fx_n[1][0][i] = v[i];
				// --> Setze "y" auf die obere Grenze <--
				parser_iVars.vValue[1][0] = (parser_iVars.vValue[1][1] + parser_iVars.vValue[1][2]) / 2.0;
				// --> Werte die zweite Stuetzstelle aus <--
				v = _parser.Eval(nResults);
				for (int i = 0; i < nResults; i++)
					fx_n[1][1][i] = v[i];
				// --> Setze "y" auf die obere Grenze <--
				parser_iVars.vValue[1][0] = parser_iVars.vValue[1][2];
				// --> Werte die zweite Stuetzstelle aus <--
				v = _parser.Eval(nResults);
				for (int i = 0; i < nResults; i++)
				{
					fx_n[1][2][i] = v[i];
					// --> Berechne das y-Trapez <--
					vResult[1][i] = (parser_iVars.vValue[1][2] - parser_iVars.vValue[1][1]) / 6.0 * (fx_n[1][0][i] + 4.0 * fx_n[1][1][i] + fx_n[1][2][i]);

					// --> Weise das y-Ergebnis der zweiten x-Stuetzstelle zu <--
					fx_n[0][1][i] = vResult[1][i];
				}

				parser_iVars.vValue[0][0] += parser_iVars.vValue[0][3] / 2.0; // x + dx

				// --> Erneuere die Werte der x- und y-Grenzen <--
				_parser.SetExpr(sInt_Line[1][0]);
				parser_iVars.vValue[1][1] = _parser.Eval();
				_parser.SetExpr(sInt_Line[1][1]);
				parser_iVars.vValue[1][2] = _parser.Eval();
				// --> Weise dem Parser wieder die Funktion f(x,y) zu <--
				_parser.SetExpr(sInt_Fct);
				// --> Setze "y" wieder auf die untere Grenze <--
				parser_iVars.vValue[1][0] = parser_iVars.vValue[1][1];
				// --> Setze den Speicher fuer die "innere" Integration auf 0 <--
				for (int i = 0; i < nResults; i++)
					vResult[2][i] = 0.0;

				// --> Werte erste y-Stuetzstelle aus <--
				v = _parser.Eval(nResults);
				for (int i = 0; i < nResults; i++)
					fx_n[1][0][i] = v[i];
				// --> Setze "y" auf die obere Grenze <--
				parser_iVars.vValue[1][0] = (parser_iVars.vValue[1][1] + parser_iVars.vValue[1][2]) / 2.0;
				// --> Werte die zweite Stuetzstelle aus <--
				v = _parser.Eval(nResults);
				for (int i = 0; i < nResults; i++)
					fx_n[1][1][i] = v[i];
				// --> Setze "y" auf die obere Grenze <--
				parser_iVars.vValue[1][0] = parser_iVars.vValue[1][2];
				// --> Werte die zweite Stuetzstelle aus <--
				v = _parser.Eval(nResults);
				for (int i = 0; i < nResults; i++)
				{
					fx_n[1][2][i] = v[i];
					// --> Berechne das y-Trapez <--
					vResult[2][i] = (parser_iVars.vValue[1][2] - parser_iVars.vValue[1][1]) / 6.0 * (fx_n[1][0][i] + 4.0 * fx_n[1][1][i] + fx_n[1][2][i]);

					// --> Weise das y-Ergebnis der zweiten x-Stuetzstelle zu <--
					fx_n[0][2][i] = vResult[2][i];
					vResult[0][i] += parser_iVars.vValue[0][3] / 6.0 * (fx_n[0][0][i] + 4.0 * fx_n[0][1][i] + fx_n[0][2][i]); // Berechne das Trapez zu x
					fx_n[0][0][i] = fx_n[0][2][i]; // Weise den Wert der zweiten Stuetzstelle an die erste Stuetzstelle zu
				}
			}
		}

	}

	// --> Falls die Grenzen irgendwo getauscht worden sind, wird dem hier Rechnung getragen <--
	for (int i = 0; i < nResults; i++)
		vResult[0][i] *= nSign;

	// --> FERTIG! Teilen wir dies dem Benutzer mit <--
	if (_option.getSystemPrintStatus())
	{
		NumeReKernel::printPreFmt(": " + _lang.get("COMMON_SUCCESS") + "!\n");
	}

	// --> Fertig! Zurueck zur aufrufenden Funkton! <--
	return vResult[0];
}

// Calculate the numerical differential of the passed expression
vector<double> parser_Diff(const string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option, Define& _functions)
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
	if (containsStrings(sExpr) || _data.containsStringVars(sExpr))
	{
		throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "diff");
	}

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
	if ((sExpr.find("data(") == string::npos && !_data.containsCacheElements(sExpr))
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
		if (matchParams(sVar, "eps", '='))
		{

			sEps = getArgAtPos(sVar, matchParams(sVar, "eps", '=') + 3);
			sVar += " ";
			sVar = sVar.substr(0, matchParams(sVar, "eps", '=')) + sVar.substr(sVar.find(' ', matchParams(sVar, "eps", '=') + 3));

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
		if (matchParams(sVar, "samples", '='))
		{

			_parser.SetExpr(getArgAtPos(sVar, matchParams(sVar, "samples", '=') + 7));
			nSamples = (int)_parser.Eval();
			sVar += " ";
			sVar = sVar.substr(0, matchParams(sVar, "samples", '=')) + sVar.substr(sVar.find(' ', matchParams(sVar, "samples", '=') + 7));
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
				if (_data.containsCacheElements(sPos) || sPos.find("data(") != string::npos)
				{
					getDataElements(sPos, _parser, _data, _option);
				}
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
				{
					vInterval.push_back(v[i]);
				}
			}

			// Set the expression for differentiation
			// and evaluate it
			_parser.SetExpr(sExpr);
			_parser.Eval(nResults);

			// Get the address of the variable
			dVar = parser_GetVarAdress(sVar, _parser);
		}

		// Ensure that the address could be found
		if (!dVar)
		{
			throw SyntaxError(SyntaxError::NO_DIFF_VAR, sCmd, SyntaxError::invalid_position);
		}

		// Define a reasonable precision if no precision was set
		if (!dEps)
			dEps = 1e-7;

        // Store the expression
		string sCompl_Expr = sExpr;

		// Expand the expression, if necessary
        if (sCompl_Expr.find("{") != string::npos)
            parser_VectorToExpr(sCompl_Expr, _option);

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
	else if (sExpr.find("data(") != string::npos || _data.containsCacheElements(sExpr))
	{
	    // This is a data set
	    //
	    // Get the indices first
		Indices _idx = parser_getIndices(sExpr, _parser, _data, _option);

		// Extract the table name
		sExpr.erase(sExpr.find('('));

		// Validate the indices
		if (((_idx.nI[0] == -1 || _idx.nI[1] == -1) && !_idx.vI.size()) || (_idx.nJ[0] == -1 && !_idx.vJ.size()))
			throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

        // Copy the data contents, sort the values
        // and calculate the derivative
		if (!_idx.vI.size())
		{
		    // Regular indices
			if (_idx.nI[1] == -2)
				_idx.nI[1] = _data.getLines(sExpr, false) - 1;
			if (_idx.nJ[1] == -2)
				_idx.nJ[1] = _idx.nJ[0] + 1;

            // Depending on the number of selected columns, we either
            // have to sort the data or we assume that the difference
            // between two values is 1
			if (_idx.nJ[1] == -1)
			{
			    // No sorting, difference is 1
			    //
			    // Jump over NaNs and get the difference of the neighbouring
			    // values, which is identical to the derivative in this case
				for (long long int i = _idx.nI[0]; i <= _idx.nI[1] - 1; i++)
				{
					if (_data.isValidEntry(i, _idx.nJ[0], sExpr)
							&& _data.isValidEntry(i + 1, _idx.nJ[0], sExpr))
						vResult.push_back(_data.getElement(i + 1, _idx.nJ[0], sExpr) - _data.getElement(i, _idx.nJ[0], sExpr));
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
				for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
				{
					_cache.writeToCache(i - _idx.nI[0], 0, "cache", _data.getElement(i, _idx.nJ[0], sExpr));
					_cache.writeToCache(i - _idx.nI[0], 1, "cache", _data.getElement(i, _idx.nJ[1], sExpr));
				}
				_cache.sortElements("cache -sort c=1[2]");

				// Shall the x values be calculated?
				if (matchParams(sCmd, "xvals"))
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
			// Vectors as indices
			//
			// Depending on the number of selected columns, we either
            // have to sort the data or we assume that the difference
            // between two values is 1
			if (_idx.vJ.size() == 1)
			{
				// No sorting, difference is 1
			    //
			    // Jump over NaNs and get the difference of the neighbouring
			    // values, which is identical to the derivative in this case
				for (long long int i = 0; i < _idx.vI.size() - 1; i++)
				{
					if (_data.isValidEntry(_idx.vI[i], _idx.vJ[0], sExpr)
							&& _data.isValidEntry(_idx.vI[i + 1], _idx.vJ[0], sExpr))
						vResult.push_back(_data.getElement(_idx.vI[i + 1], _idx.vJ[0], sExpr) - _data.getElement(_idx.vI[i], _idx.vJ[0], sExpr));
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
				for (long long int i = 0; i < _idx.vI.size(); i++)
				{
					_cache.writeToCache(i, 0, "cache", _data.getElement(_idx.vI[i], _idx.vJ[0], sExpr));
					_cache.writeToCache(i, 1, "cache", _data.getElement(_idx.vI[i], _idx.vJ[1], sExpr));
				}
				_cache.sortElements("cache -sort c=1[2]");

				// Shall the x values be calculated?
				if (matchParams(sCmd, "xvals"))
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

	}
	else
	{
	    // Ensure that a parameter list is available
		throw SyntaxError(SyntaxError::NO_DIFF_OPTIONS, sCmd, SyntaxError::invalid_position);
	}
	return vResult;
}

// This function lists all known functions in the terminal
// It is more or less a legacy function, because the functions are
// now listed in the sidebar
void parser_ListFunc(const Settings& _option, const string& sType) //PRSRFUNC_LISTFUNC_[TYPES]_*
{
	NumeReKernel::toggleTableStatus();
	make_hline();
	NumeReKernel::printPreFmt("|-> NUMERE: " + toUpperCase(_lang.get("PARSERFUNCS_LISTFUNC_HEADLINE")));
	if (sType != "all")
	{
		NumeReKernel::printPreFmt("  [" + toUpperCase(_lang.get("PARSERFUNCS_LISTFUNC_TYPE_" + toUpperCase(sType))) + "]");
	}
	NumeReKernel::printPreFmt("\n");
	make_hline();
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("PARSERFUNCS_LISTFUNC_TABLEHEAD"), _option, false, 0, 28) + "\n|\n");
	vector<string> vFuncs;

	// Get the list of functions from the language file
	// depending on the selected type
	if (sType == "all")
		vFuncs = _lang.getList("PARSERFUNCS_LISTFUNC_FUNC_*");
	else
		vFuncs = _lang.getList("PARSERFUNCS_LISTFUNC_FUNC_*_[" + toUpperCase(sType) + "]");

    // Print the obtained function list on the terminal
	for (unsigned int i = 0; i < vFuncs.size(); i++)
	{
		NumeReKernel::printPreFmt(LineBreak("|   " + vFuncs[i], _option, false, 0, 60) + "\n");
	}
	NumeReKernel::printPreFmt("|\n");
	NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTFUNC_FOOTNOTE1"), _option));
	NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTFUNC_FOOTNOTE2"), _option));
	NumeReKernel::toggleTableStatus();
	make_hline();
	return;
}

// This function lists all custom defined functions
// It is more or less also a legacy function, because the
// custom defined functions are also listed in the sidebar
void parser_ListDefine(const Define& _functions, const Settings& _option)
{
	NumeReKernel::toggleTableStatus();
	make_hline();
	NumeReKernel::print("NUMERE: " + toUpperCase(_lang.get("PARSERFUNCS_LISTDEFINE_HEADLINE")));
	make_hline();
	if (!_functions.getDefinedFunctions())
	{
		NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_LISTDEFINE_EMPTY")));
	}
	else
	{
	    // Print all custom defined functions on the terminal
		for (unsigned int i = 0; i < _functions.getDefinedFunctions(); i++)
		{
		    // Print first the name of the function
			NumeReKernel::printPreFmt(sectionHeadline(_functions.getFunction(i).substr(0, _functions.getFunction(i).rfind('('))));

			// Print the comment, if it is available
			if (_functions.getComment(i).length())
			{
				NumeReKernel::printPreFmt(LineBreak("|       " + _lang.get("PARSERFUNCS_LISTDEFINE_DESCRIPTION", _functions.getComment(i)), _option, true, 0, 25) + "\n"); //10
			}

			// Print the actual implementation of the function
			NumeReKernel::printPreFmt(LineBreak("|       " + _lang.get("PARSERFUNCS_LISTDEFINE_DEFINITION", _functions.getFunction(i), _functions.getImplemention(i)), _option, false, 0, 29) + "\n"); //14
        }
		NumeReKernel::printPreFmt("|   -- " + toString((int)_functions.getDefinedFunctions()) + " " + toSystemCodePage(_lang.get("PARSERFUNCS_LISTDEFINE_FUNCTIONS"))  + " --\n");
	}
	NumeReKernel::toggleTableStatus();
	make_hline();
	return;
}

// This function lists all logical expressions
void parser_ListLogical(const Settings& _option)
{
	NumeReKernel::toggleTableStatus();
	make_hline();
	NumeReKernel::print(toSystemCodePage("NUMERE: " + toUpperCase(_lang.get("PARSERFUNCS_LISTLOGICAL_HEADLINE"))));
	make_hline();
	NumeReKernel::printPreFmt(toSystemCodePage("|   " + _lang.get("PARSERFUNCS_LISTLOGICAL_TABLEHEAD")) + "\n|\n");

	// Get the list of all logical expressions
	vector<string> vLogicals = _lang.getList("PARSERFUNCS_LISTLOGICAL_ITEM*");

	// Print the list on the terminal
	for (unsigned int i = 0; i < vLogicals.size(); i++)
		NumeReKernel::printPreFmt(toSystemCodePage("|   " + vLogicals[i]) + "\n");

	NumeReKernel::printPreFmt(toSystemCodePage("|\n"));
	NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTLOGICAL_FOOTNOTE1"), _option));
	NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTLOGICAL_FOOTNOTE2"), _option));
	NumeReKernel::toggleTableStatus();
	make_hline();
	return;
}

// This function lists all declared variables, which
// are known by the numerical and the string parser as
// well as the current declared data tables
//
// This function will be legacy in the near future
// if the planned variables widget is implemented
void parser_ListVar(mu::ParserBase& _parser, const Settings& _option, const Datafile& _data)
{
	int nDataSetNum = 1;
	map<string, int> VarMap;
	int nBytesSum = 0;

	// Query the used variables
	//
	// Get the numerical variables
	mu::varmap_type variables = _parser.GetVar();

	// Get the string variables
	map<string, string> StringMap = _data.getStringVars();

	// Get the current defined data tables
	map<string, long long int> CacheMap = _data.getCacheList();

	// Combine string and numerical variables to have
	// them sorted after their name
	for (auto iter = variables.begin(); iter != variables.end(); ++iter)
	{
		VarMap[iter->first] = 0;
	}
	for (auto iter = StringMap.begin(); iter != StringMap.end(); ++iter)
	{
		VarMap[iter->first] = 1;
	}

	// Get data table and string table sizes
	string sDataSize = toString(_data.getLines("data", false)) + " x " + toString(_data.getCols("data"));
	string sStringSize = toString((int)_data.getStringElements()) + " x " + toString((int)_data.getStringCols());

	NumeReKernel::toggleTableStatus();
	make_hline();
	NumeReKernel::print("NUMERE: " + toUpperCase(toSystemCodePage(_lang.get("PARSERFUNCS_LISTVAR_HEADLINE"))));
	make_hline();

	// Print all defined caches first
	for (auto iter = CacheMap.begin(); iter != CacheMap.end(); ++iter)
	{
		string sCacheSize = toString(_data.getCacheLines(iter->first, false)) + " x " + toString(_data.getCacheCols(iter->first, false));
		NumeReKernel::printPreFmt("|   " + iter->first + "()" + strfill("Dim:", (_option.getWindow(0) - 32) / 2 - (iter->first).length() + _option.getWindow(0) % 2) + strfill(sCacheSize, (_option.getWindow(0) - 50) / 2) + strfill("[double x double]", 19));
		if (_data.getSize(iter->second) >= 1024 * 1024)
			NumeReKernel::printPreFmt(strfill(toString(_data.getSize(iter->second) / (1024.0 * 1024.0), 4), 9) + " MBytes\n");
		else if (_data.getSize(iter->second) >= 1024)
			NumeReKernel::printPreFmt(strfill(toString(_data.getSize(iter->second) / (1024.0), 4), 9) + " KBytes\n");
		else
			NumeReKernel::printPreFmt(strfill(toString(_data.getSize(iter->second)), 9) + "  Bytes\n");
		nBytesSum += _data.getSize(iter->second);
	}
	NumeReKernel::printPreFmt("|   " + strfill("-", _option.getWindow(0) - 4, '-') + "\n");

	// Print now the dimension of the data table
	if (_data.isValid())
	{
		NumeReKernel::printPreFmt("|   data()" + strfill("Dim:", (_option.getWindow(0) - 32) / 2 - 4 + _option.getWindow(0) % 2) + strfill(sDataSize, (_option.getWindow(0) - 50) / 2) + strfill("[double x double]", 19));
		if (_data.getDataSize() >= 1024 * 1024)
			NumeReKernel::printPreFmt(strfill(toString(_data.getDataSize() / (1024.0 * 1024.0), 4), 9) + " MBytes\n");
		else if (_data.getDataSize() >= 1024)
			NumeReKernel::printPreFmt(strfill(toString(_data.getDataSize() / (1024.0), 4), 9) + " KBytes\n");
		else
			NumeReKernel::printPreFmt(strfill(toString(_data.getDataSize()), 9) + "  Bytes\n");
		nBytesSum += _data.getDataSize();

		NumeReKernel::printPreFmt("|   " + strfill("-", _option.getWindow(0) - 4, '-') + "\n");
    }

	// Print now the dimension of the string table
	if (_data.getStringElements())
	{
		NumeReKernel::printPreFmt("|   string()" + strfill("Dim:", (_option.getWindow(0) - 32) / 2 - 6 + _option.getWindow(0) % 2) + strfill(sStringSize, (_option.getWindow(0) - 50) / 2) + strfill("[string x string]", 19));
		if (_data.getStringSize() >= 1024 * 1024)
			NumeReKernel::printPreFmt(strfill(toString(_data.getStringSize() / (1024.0 * 1024.0), 4), 9) + " MBytes\n");
		else if (_data.getStringSize() >= 1024)
			NumeReKernel::printPreFmt(strfill(toString(_data.getStringSize() / (1024.0), 4), 9) + " KBytes\n");
		else
			NumeReKernel::printPreFmt(strfill(toString(_data.getStringSize()), 9) + "  Bytes\n");
		nBytesSum += _data.getStringSize();

		NumeReKernel::printPreFmt("|   " + strfill("-", _option.getWindow(0) - 4, '-') + "\n");
    }

    // Print now the set of variables
	for (auto item = VarMap.begin(); item != VarMap.end(); ++item)
	{
	    // The second member indicates, whether a
	    // variable is a string or a numerical variable
		if (item->second)
		{
		    // This is a string
			NumeReKernel::printPreFmt("|   " + item->first + strfill(" = ", (_option.getWindow(0) - 20) / 2 + 1 - _option.getPrecision() - (item->first).length() + _option.getWindow(0) % 2));
			if (StringMap[item->first].length() > (unsigned int)_option.getPrecision() + (_option.getWindow(0) - 60) / 2 - 4)
				NumeReKernel::printPreFmt(strfill("\"" + StringMap[item->first].substr(0, _option.getPrecision() + (_option.getWindow(0) - 60) / 2 - 7) + "...\"", (_option.getWindow(0) - 60) / 2 + _option.getPrecision()));
			else
				NumeReKernel::printPreFmt(strfill("\"" + StringMap[item->first] + "\"", (_option.getWindow(0) - 60) / 2 + _option.getPrecision()));
			NumeReKernel::printPreFmt(strfill("[string]", 19) + strfill(toString((int)StringMap[item->first].size()), 9) + "  Bytes\n");
			nBytesSum += StringMap[item->first].size();
		}
		else
		{
		    // This is a numerical variable
			NumeReKernel::printPreFmt("|   " + item->first + strfill(" = ", (_option.getWindow(0) - 20) / 2 + 1 - _option.getPrecision() - (item->first).length() + _option.getWindow(0) % 2) + strfill(toString(*variables[item->first], _option), (_option.getWindow(0) - 60) / 2 + _option.getPrecision()) + strfill("[double]", 19) + strfill("8", 9) + "  Bytes\n");
			nBytesSum += sizeof(double);
		}
	}

	// Create now the footer of the list:
	// Combine the number of variables and data
	// tables first
	NumeReKernel::printPreFmt("|   -- " + toString((int)VarMap.size()) + " " + toSystemCodePage(_lang.get("PARSERFUNCS_LISTVAR_VARS_AND")) + " ");
	if (_data.isValid() || _data.isValidCache() || _data.getStringElements())
	{
		if (_data.isValid() && _data.isValidCache() && _data.getStringElements())
		{
			NumeReKernel::printPreFmt(toString(2 + CacheMap.size()));
			nDataSetNum = CacheMap.size() + 2;
		}
		else if ((_data.isValid() && _data.isValidCache())
				 || (_data.isValidCache() && _data.getStringElements()))
		{
			NumeReKernel::printPreFmt(toString(1 + CacheMap.size()));
			nDataSetNum = CacheMap.size() + 1;
		}
		else if (_data.isValid() && _data.getStringElements())
		{
			NumeReKernel::printPreFmt("2");
			nDataSetNum = 2;
		}
		else if (_data.isValidCache())
		{
			NumeReKernel::printPreFmt(toString((int)CacheMap.size()));
			nDataSetNum = CacheMap.size();
		}
		else
			NumeReKernel::printPreFmt("1");
	}
	else
		NumeReKernel::printPreFmt("0");
	NumeReKernel::printPreFmt(" " + toSystemCodePage(_lang.get("PARSERFUNCS_LISTVAR_DATATABLES")) + " --");

	// Calculate now the needed memory for the stored values and print it at the
	// end of the footer line
	if (VarMap.size() > 9 && nDataSetNum > 9)
		NumeReKernel::printPreFmt(strfill("Total: ", (_option.getWindow(0) - 32 - _lang.get("PARSERFUNCS_LISTVAR_VARS_AND").length() - _lang.get("PARSERFUNCS_LISTVAR_DATATABLES").length())));
	else if (VarMap.size() > 9 || nDataSetNum > 9)
		NumeReKernel::printPreFmt(strfill("Total: ", (_option.getWindow(0) - 31 - _lang.get("PARSERFUNCS_LISTVAR_VARS_AND").length() - _lang.get("PARSERFUNCS_LISTVAR_DATATABLES").length())));
	else
		NumeReKernel::printPreFmt(strfill("Total: ", (_option.getWindow(0) - 30 - _lang.get("PARSERFUNCS_LISTVAR_VARS_AND").length() - _lang.get("PARSERFUNCS_LISTVAR_DATATABLES").length())));
	if (nBytesSum >= 1024 * 1024)
		NumeReKernel::printPreFmt(strfill(toString(nBytesSum / (1024.0 * 1024.0), 4), 8) + " MBytes\n");
	else if (nBytesSum >= 1024)
		NumeReKernel::printPreFmt(strfill(toString(nBytesSum / (1024.0), 4), 8) + " KBytes\n");
	else
		NumeReKernel::printPreFmt(strfill(toString(nBytesSum), 8) + "  Bytes\n");
	NumeReKernel::toggleTableStatus();
	make_hline();
	return;
}

// This function lists all known constants
// It is more or less a legacy function, because the
// constants are now listed in the sidebar
void parser_ListConst(const mu::ParserBase& _parser, const Settings& _option)
{
	const int nUnits = 20;
	// Define a set of units including a simple
	// heuristic, which defines, which constant
	// needs which unit
	static string sUnits[nUnits] =
	{
		"_G[m^3/(kg s^2)]",
		"_R[J/(mol K)]",
		"_coul_norm[V m/(A s)]",
		"_c[m/s]",
		"_elek[A s/(V m)]",
		"_elem[A s]",
		"_gamma[1/(T s)]",
		"_g[m/s^2]",
		"_hartree[J]",
		"_h[J s]",
		"_k[J/K]",
		"_m_[kg]",
		"_magn[V s/(A m)]",
		"_mu_[J/T]",
		"_n[1/mol]",
		"_rydberg[1/m]",
		"_r[m]",
		"_stefan[J/(m^2 s K^4)]",
		"_wien[m K]",
		"_[---]"
	};
	NumeReKernel::toggleTableStatus();
	make_hline();
	NumeReKernel::print("NUMERE: " + toSystemCodePage(toUpperCase(_lang.get("PARSERFUNCS_LISTCONST_HEADLINE"))));
	make_hline();

	// Get the map of all defined constants from the parser
	mu::valmap_type cmap = _parser.GetConst();
    valmap_type::const_iterator item = cmap.begin();

    // Print all constants, their values and their unit on
    // the terminal
    for (; item != cmap.end(); ++item)
    {
        if (item->first[0] != '_')
            continue;
        NumeReKernel::printPreFmt("|   " + item->first + strfill(" = ", (_option.getWindow() - 10) / 2 + 2 - _option.getPrecision() - (item->first).length() + _option.getWindow() % 2) + strfill(toString(item->second, _option), _option.getPrecision() + (_option.getWindow() - 50) / 2));
        for (int i = 0; i < nUnits; i++)
        {
            if (sUnits[i].substr(0, sUnits[i].find('[')) == (item->first).substr(0, sUnits[i].find('[')))
            {
                NumeReKernel::printPreFmt(strfill(sUnits[i].substr(sUnits[i].find('[')), 24) + "\n");
                break;
            }
        }
    }
    NumeReKernel::printPreFmt("|\n");
    NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTCONST_FOOTNOTE1"), _option));
    NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTCONST_FOOTNOTE2"), _option));
	NumeReKernel::toggleTableStatus();
	make_hline();
	return;
}

// This function lists all variables, which were
// used in the last evaluated expression
// This function is legacy and will be removed in
// the future. It cannot be called from the outside
void parser_ListExprVar(mu::ParserBase& _parser, const Settings& _option, const Datafile& _data)
{
	string_type sExpr = _parser.GetExpr();
	if (sExpr.length() == 0)
	{
		cerr << toSystemCodePage("|-> " + _lang.get("PARSERFUNCS_LISTEXPRVAR_EMPTY")) << endl;
		return;
	}

	// Query the used variables (must be done after calc)
	make_hline();
	cerr << "|-> NUMERE: " << toSystemCodePage(toUpperCase(_lang.get("PARSERFUNCS_LISTEXPRVAR_HEADLINE"))) << endl;
	make_hline();
	cerr << LineBreak("|   " + _lang.get("PARSERFUNCS_LISTEXPRVAR_EXPR", _parser.GetExpr()), _option, true, 0, 14) << endl;

	varmap_type variables = _parser.GetUsedVar();
	if (!variables.size())
	{
		cerr << "|" << endl
			 << toSystemCodePage("|-> " + _lang.get("PARSERFUNCS_LISTEXPRVAR_NOVARS")) << endl;
	}
	else
	{
		mu::varmap_type::const_iterator item = variables.begin();


		for (; item != variables.end(); ++item)
		{
			_parser.SetExpr(item->first);
			cerr << std::setprecision(_option.getPrecision());
			cerr << "|   " << item->first;
			cerr << std::setfill(' ') << std::setw((_option.getWindow() - 20) / 2 + 1 - _option.getPrecision() - (item->first).length() + _option.getWindow() % 2) << " = ";
			cerr << std::setw(_option.getPrecision() + (_option.getWindow() - 60) / 2) << _parser.Eval();
			cerr << std::setw(19) << "[double]";
			cerr << std::setw(9) << sizeof(double) << "  Bytes" << endl;
		}
		cerr << "|   -- " << _lang.get("PARSERFUNCS_LISTEXPRVAR_FOOTNOTE", toString((int)variables.size())) << " --" << endl;

	}
	_parser.SetExpr(sExpr);
	make_hline();
	return;
}

// This function lists all defined commands
// It is more or less a legacy function, because
// the commands are now listed in the sidebar
void parser_ListCmd(const Settings& _option)
{
	NumeReKernel::toggleTableStatus();
	make_hline();
	NumeReKernel::print("NUMERE: " + toSystemCodePage(toUpperCase(_lang.get("PARSERFUNCS_LISTCMD_HEADLINE")))); //PRSRFUNC_LISTCMD_*
	make_hline();
	NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("PARSERFUNCS_LISTCMD_TABLEHEAD"), _option, 0) + "\n|\n");

    // Get the list of all defined commands
    // from the language files
	vector<string> vCMDList = _lang.getList("PARSERFUNCS_LISTCMD_CMD_*");

	// Print the complete list on the terminal
	for (unsigned int i = 0; i < vCMDList.size(); i++)
	{
		NumeReKernel::printPreFmt(LineBreak("|   " + vCMDList[i], _option, false, 0, 42) + "\n");
	}

	NumeReKernel::printPreFmt("|\n");
	NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTCMD_FOOTNOTE1"), _option));
	NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTCMD_FOOTNOTE2"), _option));
	NumeReKernel::toggleTableStatus();
	make_hline();
}

// This function lists all unit conversions and
// their result, if applied on a 1. The units are
// partly physcially units, partly magnitudes.
void parser_ListUnits(const Settings& _option) //PRSRFUNC_LISTUNITS_*
{
	NumeReKernel::toggleTableStatus();
	make_hline();
	NumeReKernel::print("NUMERE: " + toSystemCodePage(toUpperCase(_lang.get("PARSERFUNCS_LISTUNITS_HEADLINE")))); //(_option.getWindow()-x)/3
	make_hline(); // 11       21  x=17             15   x=35      1               x=2      26
	printUnits(_lang.get("PARSERFUNCS_LISTUNITS_SYMBOL"), _lang.get("PARSERFUNCS_LISTUNITS_DESCRIPTION"), _lang.get("PARSERFUNCS_LISTUNITS_DIMENSION"), _lang.get("PARSERFUNCS_LISTUNITS_UNIT"), _option.getWindow());
	NumeReKernel::printPreFmt("|\n");
	printUnits("1'A",   _lang.get("PARSERFUNCS_LISTUNITS_UNIT_ANGSTROEM"),        "L",           "1e-10      [m]", _option.getWindow());
	printUnits("1'AU",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_ASTRO_UNIT"),       "L",           "1.4959787e11      [m]", _option.getWindow());
	printUnits("1'b",   _lang.get("PARSERFUNCS_LISTUNITS_UNIT_BARN"),             "L^2",         "1e-28    [m^2]", _option.getWindow());
	printUnits("1'cal", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_CALORY"),           "M L^2 / T^2", "4.1868      [J]", _option.getWindow());
	printUnits("1'Ci",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_CURIE"),            "1 / T",       "3.7e10     [Bq]", _option.getWindow());
	printUnits("1'eV",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_ELECTRONVOLT"),     "M L^2 / T^2", "1.60217657e-19      [J]", _option.getWindow());
	printUnits("1'fm",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_FERMI"),            "L",           "1e-15      [m]", _option.getWindow());
	printUnits("1'ft",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_FOOT"),             "L",           "0.3048      [m]", _option.getWindow());
	printUnits("1'Gs",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_GAUSS"),            "M / (T^2 I)", "1e-4      [T]", _option.getWindow());
	printUnits("1'in",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_INCH"),             "L",           "0.0254      [m]", _option.getWindow());
	printUnits("1'kmh", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_VELOCITY"),         "L / T",       "0.2777777...    [m/s]", _option.getWindow());
	printUnits("1'kn",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_KNOTS"),            "L / T",       "0.5144444...    [m/s]", _option.getWindow());
	printUnits("1'l",   _lang.get("PARSERFUNCS_LISTUNITS_UNIT_LITERS"),           "L^3",         "1e-3    [m^3]", _option.getWindow());
	printUnits("1'ly",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_LIGHTYEAR"),        "L",           "9.4607305e15      [m]", _option.getWindow());
	printUnits("1'mile", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_MILE"),             "L",           "1609.344      [m]", _option.getWindow());
	printUnits("1'mol", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_MOL"),              "N",           "6.022140857e23      ---", _option.getWindow());
	printUnits("1'mph", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_VELOCITY"),         "L / T",       "0.44703722    [m/s]", _option.getWindow());
	printUnits("1'Ps",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_POISE"),            "M / (L T)",   "0.1   [Pa s]", _option.getWindow());
	printUnits("1'pc",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_PARSEC"),           "L",           "3.0856776e16      [m]", _option.getWindow());
	printUnits("1'psi", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_PSI"),              "M / (L T^2)", "6894.7573     [Pa]", _option.getWindow());
	printUnits("1'TC",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_CELSIUS"),          "Theta",       "274.15      [K]", _option.getWindow());
	printUnits("1'TF",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_FAHRENHEIT"),       "Theta",       "255.92778      [K]", _option.getWindow());
	printUnits("1'Torr", _lang.get("PARSERFUNCS_LISTUNITS_UNIT_TORR"),             "M / (L T^2)", "133.322     [Pa]", _option.getWindow());
	printUnits("1'yd",  _lang.get("PARSERFUNCS_LISTUNITS_UNIT_YARD"),             "L",           "0.9144      [m]", _option.getWindow());
	NumeReKernel::printPreFmt("|\n");
	printUnits("1'G",   "(giga)",             "---",           "1e9      ---", _option.getWindow());
	printUnits("1'M",   "(mega)",             "---",           "1e6      ---", _option.getWindow());
	printUnits("1'k",   "(kilo)",             "---",           "1e3      ---", _option.getWindow());
	printUnits("1'm",   "(milli)",            "---",           "1e-3      ---", _option.getWindow());
	printUnits("1'mu",  "(micro)",            "---",           "1e-6      ---", _option.getWindow());
	printUnits("1'n",   "(nano)",             "---",           "1e-9      ---", _option.getWindow());

	NumeReKernel::printPreFmt("|\n");
	NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_LISTUNITS_FOOTNOTE"), _option));
	NumeReKernel::toggleTableStatus();
	make_hline();

	return;
}

// This function lists all declared plugins including
// their name, their command and their description.
// It is more or less a legacy function, because the
// plugins are also listed in the sidebar
void parser_ListPlugins(Parser& _parser, Datafile& _data, const Settings& _option)
{
	string sDummy = "";
	NumeReKernel::toggleTableStatus();
	make_hline();
	NumeReKernel::print(toSystemCodePage("NUMERE: " + toUpperCase(_lang.get("PARSERFUNCS_LISTPLUGINS_HEADLINE"))));
	make_hline();

	// Probably there's no plugin defined
	if (!_plugin.getPluginCount())
		NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_LISTPLUGINS_EMPTY")));
	else
	{
		NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("PARSERFUNCS_LISTPLUGINS_TABLEHEAD"), _option, 0) + "\n");
		NumeReKernel::printPreFmt("|\n");

		// Print all plugins (name, command and description)
		// on the terminal
		for (unsigned int i = 0; i < _plugin.getPluginCount(); i++)
		{
			string sLine = "|   ";
			if (_plugin.getPluginCommand(i).length() > 18)
				sLine += _plugin.getPluginCommand(i).substr(0, 15) + "...";
			else
				sLine += _plugin.getPluginCommand(i);
			sLine.append(23 - sLine.length(), ' ');

			// Print basic information about the plugin
			sLine += _lang.get("PARSERFUNCS_LISTPLUGINS_PLUGININFO", _plugin.getPluginName(i), _plugin.getPluginVersion(i), _plugin.getPluginAuthor(i));

			// Print the description
			if (_plugin.getPluginDesc(i).length())
			{
				sLine += "$" + _plugin.getPluginDesc(i);
			}
			sLine = '"' + sLine + "\" -nq";
			if (!parser_StringParser(sLine, sDummy, _data, _parser, _option, true))
			{
				NumeReKernel::toggleTableStatus();
				throw SyntaxError(SyntaxError::STRING_ERROR, "", SyntaxError::invalid_position);
			}
			NumeReKernel::printPreFmt(LineBreak(sLine, _option, true, 0, 25) + "\n");
		}
	}
	NumeReKernel::toggleTableStatus();
	make_hline();
	return;
}

// This function replaces vector expressions with their corresponding multi-expression equation
// It is used quite extensively, however, it might not be necessary everywhere, because the
// parser can cope with the vector syntax now.
void parser_VectorToExpr(string& sLine, const Settings& _option)
{
	vector<string> vVectors(1, "");
	vector<string> vScalars(1, "");

	string sTemp = sLine;
	string sDelim = "+-*/^&|!%";
	int nDim = 0;
	int nDim_vec = 0;
	unsigned int nPos = 0;
	size_t nQuotes = 0;
	bool bIsStringExpression = containsStrings(sLine);

	// Handle multi-expression expressions first
	if (isMultiValue(sLine))
    {
        string sBuffer;

        // Use the getNextArgument function to
        // obtain the next single expression part
        // of the current string
        while (sLine.length())
        {
            sTemp = getNextArgument(sLine, true);

            // Evaluate the single expression part using a recursion
            parser_VectorToExpr(sTemp, _option);

            sBuffer += sTemp + ",";
        }
        sLine = sBuffer.substr(0, sBuffer.length()-1);
    }

	// Reset the positions
	nPos = 0;
	nQuotes = 0;

	// Separate the expression in scalars and vectors
	for (nPos = 0; nPos < sTemp.length(); nPos++)
	{
		// Count the quotation marks to ensure that
		// we're only focussing on actual operators
		if (sTemp[nPos] == '"')
		{
			if (!nPos || (nPos && sTemp[nPos - 1] != '\\'))
				nQuotes++;
		}

		// If we're in quotation marks, then continue
		if (sTemp[nPos] != '{' || (nQuotes % 2))
			continue;

		if (isToStringArg(sTemp, nPos))
			continue;

		nDim_vec = 0;

		// Ensure that there's a matching parenthesis
		if (getMatchingParenthesis(sTemp.substr(nPos)) == string::npos)
			throw SyntaxError(SyntaxError::INCOMPLETE_VECTOR_SYNTAX, sLine, SyntaxError::invalid_position);

		// Extract the current vector
		vVectors.back() = sTemp.substr(nPos + 1, getMatchingParenthesis(sTemp.substr(nPos)) - 1);

		// If there's a part of the expression before the vector
		// copy this as the first scalar value
		if (sTemp.find('{', nPos) != 0)
			vScalars.back() += sTemp.substr(0, sTemp.find('{', nPos));

		// Ensure that the legacy syntax "{{VECTOR}}" is handled correctly
		if (vVectors.back()[0] == '{')
		{
			vVectors.back().erase(0, 1);

			if (vVectors.back().back() == '}')
				vVectors.back().pop_back();
		}

		// Ensure that we didn't copy the argument of a multi-argument function
		if (parser_CheckMultArgFunc(vScalars.back(), sTemp.substr(sTemp.find('}', nPos) + 1)))
		{
			vScalars.back() += vVectors.back();
			sTemp.erase(0, getMatchingParenthesis(sTemp.substr(nPos)) + nPos + 1);
			continue;
		}

		// Remove the part of the already copied part of the expressions
		sTemp.erase(0, getMatchingParenthesis(sTemp.substr(nPos)) + nPos + 1);
		nPos = 0;

		// Get the dimensions of the current vector
		if (vVectors.back().length())
		{
			string sTempCopy = vVectors.back();

			while (sTempCopy.length())
			{
			    // Get-cut the next argument
				if (getNextArgument(sTempCopy, true).length())
					nDim_vec++;
			}
		}

		// Save the largest dimension
		if (nDim_vec > nDim)
			nDim = nDim_vec;

		// Add new empty vector and scalar storages
		vVectors.push_back("");
		vScalars.push_back("");

		// Break, if the expression was handled completely
		if (!sTemp.length())
			break;
	}

	// If the command line is not empty, add this line to the
	// last scalar in the expression
	if (sTemp.length())
	{
		vScalars.back() += sTemp;
		vScalars.push_back("");
	}

	// Clear the lines and the temporary copy
	sTemp.clear();
	sLine.clear();

	// Expand the vectors and copy them back to the
	// command line
	if (!nDim)
	{
		// This was only a scalar value
		for (size_t i = 0; i < vScalars.size(); i++)
			sLine += vScalars[i];
	}
	else
	{
		// For the largest dimension of all vectors
		for (int i = 0; i < nDim; i++)
		{
			// For the number of vectors
			for (size_t j = 0; j < vVectors.size()-1; j++)
			{
				// Copy first the scalar part
				sLine += vScalars[j];
				sTemp.clear();

				// Get the next vector component or replace them by an empty one
				if (vVectors[j].length())
					sTemp = getNextArgument(vVectors[j], true);
				else
				{
					sTemp = parser_AddVectorComponent(vVectors[j], vScalars[j], vScalars[j + 1], bIsStringExpression);
				}

				// If we're currently handling a string expression
				if (!bIsStringExpression)
				{
					// Search for string delimiters in the current vector component
					// (a.k.a concatentation operators)
					for (unsigned int n = 0; n < sDelim.length(); n++)
					{
						// If there's a delimiter, enclose the current
						// vector component in parentheses
						if (sTemp.find(sDelim[n]) != string::npos)
						{
							sTemp = "(" + sTemp + ")";
							break;
						}
					}
				}

				// Append the vector component to the command line
				sLine += sTemp;
			}

			// Append the last scalar and a comma, if it is needed
			if (vScalars.size() > vVectors.size())
				sLine += vScalars[vScalars.size()-2];

			if (i < nDim - 1)
				sLine += ",";
		}
	}

	return;
}

// This function determines the value of missing vector components
// (i.e. a vector contains not enough elements compared to the others
// used in the current expression) by applying a simple heuristic to
// the expression. It will either return "1" or "0".
string parser_AddVectorComponent(const string& sVectorComponent, const string& sLeft, const string& sRight, bool bAddStrings)
{
	bool bOneLeft = false;
	bool bOneRight = false;

	// Examine some basic border cases
	if (sVectorComponent.length())
	{
	    // Do nothing because the vector component
	    // id already defined
		return sVectorComponent;
	}
	else if (bAddStrings)
    {
        // No vector component defined, but strings
        // are required, therefore simply return an
        // empty string
		return "\"\"";
    }
	else if (!sLeft.length() && !sRight.length())
	{
	    // No surrounding elements are available
	    // return a zero
		return "0";
	}

	// If the user surrounds the current vector
	// with extra parentheses, then the heuristic
	// will require a non-zero element.
	//
	// There's also a special case for the left
	// side: if a division operator was found
	// then we will return a one direclty.
    for (int i = sLeft.length() - 1; i >= 0; i--)
    {
        if (sLeft[i] != ' ')
        {
            if (sLeft[i] == '(')
            {
                for (int j = i - 1; j >= 0; j--)
                {
                    if (sLeft[j] == '(')
                    {
                        bOneLeft = true;
                        break;
                    }
                    if (sLeft[j] == '/')
                        return "1";
                }
            }
            else if (sLeft[i] == '/')
                return "1";
            break;
        }
    }

    // Now examine the right side. Only parentheses
    // are important in this case.
    for (unsigned int i = 0; i < sRight.length(); i++)
    {
        if (sRight[i] != ' ')
        {
            if (sRight[i] == ')')
            {
                for (unsigned int j = i + 1; j < sRight.length(); j++)
                {
                    if (sRight[j] == ')')
                    {
                        bOneRight = true;
                        break;
                    }
                }
            }
            break;
        }
    }

    // If both heuristics are requiring non-zero
    // elements, return a one
    if ((bOneLeft && bOneRight))
        return "1";

    // Fallback: return zero
	return "0";
}

// This function returns the position of the first delimiter
// in the passed string, but it jumps over parentheses and
// braces
unsigned int parser_getDelimiterPos(const string& sLine)
{
	static string sDelimiter = "+-*/ =^&|!<>,\n";

	// Go through the current line
	for (unsigned int i = 0; i < sLine.length(); i++)
	{
        // Jump over parentheses and braces
		if (sLine[i] == '(' || sLine[i] == '{')
			i += getMatchingParenthesis(sLine.substr(i));

        // Try to find the current character in
        // the defined list of delimiters
        if (sDelimiter.find(sLine[i]) != string::npos)
            return i;
	}

	// Nothing was found: return the largest possible
	// number
	return string::npos;
}

// This function is invoked, if a prompt operator
// ("??") was found in a string. It will ask the user
// to provide the needed value during the execution
string parser_Prompt(const string& __sCommand)
{
	string sReturn = "";                // Variable fuer Rueckgabe-String
	string sInput = "";                 // Variable fuer die erwartete Eingabe
	bool bHasDefaultValue = false;      // Boolean; TRUE, wenn der String einen Default-Value hat
	unsigned int nPos = 0;                       // Index-Variable

	if (__sCommand.find("??") == string::npos)    // Wenn's "??" gar nicht gibt, koennen wir sofort zurueck
		return __sCommand;
	sReturn = __sCommand;               // Kopieren wir den Uebergebenen String in sReturn

	// --> do...while-Schleife, so lange "??" im String gefunden wird <--
	do
	{
		/* --> Fuer jeden "??" muessen wir eine Eingabe abfragen, daher muessen
		 *     wir zuerst alle Variablen zuruecksetzen <--
		 */
		sInput = "";
		bHasDefaultValue = false;

		// --> Speichern der naechsten Position von "??" in nPos <--
		nPos = sReturn.find("??");

		// --> Pruefen wir, ob es die Default-Value-Klammer ("??[DEFAULT]") gibt <--
		if (sReturn.find("[", nPos) != string::npos)
		{
			// --> Es gibt drei moegliche Faelle, wie eine eckige Klammer auftreten kann <--
			if (sReturn.find("??", nPos + 2) != string::npos && sReturn.find("[", nPos) < sReturn.find("??", nPos + 2))
				bHasDefaultValue = true;
			else if (sReturn.find("??", nPos + 2) == string::npos)
				bHasDefaultValue = true;
			else
				bHasDefaultValue = false;
		}

		/* --> Eingabe in einer do...while abfragen. Wenn ein Defaultwert vorhanden ist,
		 *     braucht diese Schleife nicht loopen, auch wenn nichts eingegeben wird <--
		 */
		do
		{
			string sComp = sReturn.substr(0, nPos);
			// --> Zur Orientierung geben wir den Teil des Strings vor "??" aus <--
			NumeReKernel::printPreFmt("|-\?\?> " + sComp);
			NumeReKernel::getline(sInput);
			StripSpaces(sComp);
			if (sComp.length() && sInput.find(sComp) != string::npos)
				sInput.erase(0, sInput.find(sComp) + sComp.length());
			StripSpaces(sInput);
		}
		while (!bHasDefaultValue && !sInput.length());

		// --> Eingabe in den String einsetzen <--
		if (bHasDefaultValue && !sInput.length())
		{
			sReturn = sReturn.substr(0, nPos) + sReturn.substr(sReturn.find("[", nPos) + 1, sReturn.find("]", nPos) - sReturn.find("[", nPos) - 1) + sReturn.substr(sReturn.find("]", nPos) + 1);
		}
		else if (bHasDefaultValue && sInput.length())
		{
			sReturn = sReturn.substr(0, nPos) + sInput + sReturn.substr(sReturn.find("]", nPos) + 1);
		}
		else
		{
			sReturn = sReturn.substr(0, nPos) + sInput + sReturn.substr(nPos + 2);
		}
	}
	while (sReturn.find("??") != string::npos);

	GetAsyncKeyState(VK_ESCAPE);
	// --> Jetzt enthaelt der String sReturn "??" an keiner Stelle mehr und kann zurueckgegeben werden <--
	return sReturn;
}

// This function returns the pointer to the
// passed variable
double* parser_GetVarAdress(const string& sVarName, Parser& _parser)
{
    // Get the map of declared variables
	mu::varmap_type Vars = _parser.GetVar();

	// Try to find the selected variable in the map
	auto iter = Vars.find(sVarName);
	if (iter != Vars.end())
        return iter->second;

    // return a null pointer, if nothing
    // was found
    return nullptr;
}

// This function is a wrapper to the actual extrema
// localisation function "parser_LocalizeExtremum" further
// below.
bool parser_findExtrema(string& sCmd, Datafile& _data, Parser& _parser, const Settings& _option, Define& _functions)
{
	unsigned int nSamples = 21;
	int nOrder = 5;
	double dVal[2];
	double dLeft = 0.0;
	double dRight = 0.0;
	int nMode = 0;
	double* dVar = 0;
	string sExpr = "";
	string sParams = "";
	string sInterval = "";
	string sVar = "";

	// We cannot search extrema in strings
	if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
	{
		throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "extrema");
	}

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
	else if (sCmd.find("data(") == string::npos && !_data.containsCacheElements(sCmd))
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
	if (sExpr.find("data(") != string::npos || _data.containsCacheElements(sExpr))
	{
		getDataElements(sExpr, _parser, _data, _option, false);
	}
	if (sParams.find("data(") != string::npos || _data.containsCacheElements(sParams))
	{
		getDataElements(sParams, _parser, _data, _option, false);
	}

	// Evaluate the parameters
	if (matchParams(sParams, "min"))
		nMode = -1;
	if (matchParams(sParams, "max"))
		nMode = 1;
	if (matchParams(sParams, "samples", '='))
	{
		_parser.SetExpr(getArgAtPos(sParams, matchParams(sParams, "samples", '=') + 7));
		nSamples = (unsigned int)_parser.Eval();
		if (nSamples < 21)
			nSamples = 21;
		sParams.erase(matchParams(sParams, "samples", '=') - 1, 8);
	}
	if (matchParams(sParams, "points", '='))
	{
		_parser.SetExpr(getArgAtPos(sParams, matchParams(sParams, "points", '=') + 6));
		nOrder = (int)_parser.Eval();
		if (nOrder <= 3)
			nOrder = 3;
		sParams.erase(matchParams(sParams, "points", '=') - 1, 7);
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

		value_type* v = 0;
		Datafile _cache;
		_cache.setCacheStatus(true);
		int nResults = 0;
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
		_parser.SetExpr(sExpr);
		v = _parser.Eval(nResults);
		if (nResults > 1)
		{
			vector<double> vResults;
			int nResults_x = 0;
			for (int i = 0; i < nResults; i++)
			{
				_cache.writeToCache(i, 1, "cache", v[i]);
			}
			_parser.SetExpr(sInterval);
			v = _parser.Eval(nResults_x);
			if (nResults_x > 1)
			{
				for (int i = 0; i < nResults; i++)
				{
					if (i >= nResults_x)
					{
						_cache.writeToCache(i, 0, "cache", 0.0);
					}
					else
					{
						_cache.writeToCache(i, 0, "cache", v[i]);
					}
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
			if (nOrder < 3)
			{
				vResults.push_back(NAN);
				return false;
			}
			for (int i = 0; i + nanShift < _cache.getLines("cache", true); i++)
			{
				if (i == nOrder)
					break;
				while (isnan(_cache.getElement(i + nanShift, 1, "cache")) && i + nanShift < _cache.getLines("cache", true) - 1)
					nanShift++;
				data[i] = _cache.getElement(i + nanShift, 1, "cache");
			}
			gsl_sort(data, 1, nOrder);
			dExtremum = gsl_stats_median_from_sorted_data(data, 1, nOrder);
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
					{
						nDir = 1;
					}
					else if (dMedian < dExtremum)
					{
						nDir = -1;
					}
					dExtremum = dMedian;
				}
				else
				{
					if (nDir == 1)
					{
						if (dMedian < dExtremum)
						{
							if (!nMode || nMode == 1)
							{
								int nExtremum = i;
								double dExtremum = _cache.getElement(i + nanShift, 1, "cache");
								for (long long int k = i + nanShift; k >= 0; k--)
								{
									if (k == i - nOrder)
										break;
									if (_cache.getElement(k, 1, "cache") > dExtremum)
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
					else
					{
						if (dMedian > dExtremum)
						{
							if (!nMode || nMode == -1)
							{
								int nExtremum = i + nanShift;
								double dExtremum = _cache.getElement(i, 1, "cache");
								for (long long int k = i + nanShift; k >= 0; k--)
								{
									if (k == i - nOrder)
										break;
									if (_cache.getElement(k, 1, "cache") < dExtremum)
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
		else
		{
			if (!parser_CheckVarOccurence(_parser, sVar))
			{
				sCmd = toSystemCodePage("\"Bezüglich der Variablen " + sVar + " ist der Ausdruck konstant und besitzt keine Extrema!\"");
				return true;
			}
			dVar = parser_GetVarAdress(sVar, _parser);
			if (!dVar)
			{
				throw SyntaxError(SyntaxError::EXTREMA_VAR_NOT_FOUND, sCmd, sVar, sVar);
			}
			if (sInterval.find(':') == string::npos || sInterval.length() < 3)
				return false;
			if (isNotEmptyExpression(sInterval.substr(0, sInterval.find(':'))))
			{
				_parser.SetExpr(sInterval.substr(0, sInterval.find(':')));
				dLeft = _parser.Eval();
				if (isinf(dLeft) || isnan(dLeft))
				{
					sCmd = "nan";
					return false;
				}
			}
			else
				return false;
			if (isNotEmptyExpression(sInterval.substr(sInterval.find(':') + 1)))
			{
				_parser.SetExpr(sInterval.substr(sInterval.find(':') + 1));
				dRight = _parser.Eval();
				if (isinf(dRight) || isnan(dRight))
				{
					sCmd = "nan";
					return false;
				}
			}
			else
				return false;
			if (dRight < dLeft)
			{
				double Temp = dRight;
				dRight = dLeft;
				dLeft = Temp;
			}
		}
	}
	else if (sCmd.find("data(") != string::npos || _data.containsCacheElements(sCmd))
	{
		value_type* v;
		int nResults = 0;
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
					{
						nDir = 1;
					}
					else if (dMedian < dExtremum)
					{
						nDir = -1;
					}
					dExtremum = dMedian;
				}
				else
				{
					if (nDir == 1)
					{
						if (dMedian < dExtremum)
						{
							if (!nMode || nMode == 1)
							{
								int nExtremum = i + nanShift;
								double dExtremum = v[i + nanShift];
								for (long long int k = i + nanShift; k >= 0; k--)
								{
									if (k == i - nOrder)
										break;
									if (v[k] > dExtremum)
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
					else
					{
						if (dMedian > dExtremum)
						{
							if (!nMode || nMode == -1)
							{
								int nExtremum = i + nanShift;
								double dExtremum = v[i + nanShift];
								for (long long int k = i + nanShift; k >= 0; k--)
								{
									if (k == i - nOrder)
										break;
									if (v[k] < dExtremum)
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
	else
		throw SyntaxError(SyntaxError::NO_EXTREMA_VAR, sCmd, SyntaxError::invalid_position);

    // Calculate the number of samples depending on
    // the interval width
	if ((int)(dRight - dLeft))
	{
		nSamples = (nSamples - 1) * (int)(dRight - dLeft) + 1;
	}

	// Ensure that we calculate a reasonable number of samples
	if (nSamples > 10001)
		nSamples = 10001;

    // Set the expression and evaluate it once
	_parser.SetExpr(sExpr);
	_parser.Eval();
	sCmd = "";
	vector<double> vResults;
	dVal[0] = _parser.Diff(dVar, dLeft, 1e-7);

	// Evaluate the extrema for all samples. We search for
	// a sign change in the derivative and examine these intervals
	// in more detail
	for (unsigned int i = 1; i < nSamples; i++)
	{
	    // Evaluate the derivative at the current sample position
		dVal[1] = _parser.Diff(dVar, dLeft + i * (dRight - dLeft) / (double)(nSamples - 1), 1e-7);

		// Is it a sign change or a actual zero?
		if (dVal[0]*dVal[1] < 0)
		{
			if (!nMode
					|| (nMode == 1 && (dVal[0] > 0 && dVal[1] < 0))
					|| (nMode == -1 && (dVal[0] < 0 && dVal[1] > 0)))
			{
			    // Examine the current interval in more detail
				vResults.push_back(parser_LocalizeExtremum(sExpr, dVar, _parser, _option, dLeft + (i - 1) * (dRight - dLeft) / (double)(nSamples - 1), dLeft + i * (dRight - dLeft) / (double)(nSamples - 1)));
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
						dVal[1] = _parser.Diff(dVar, dLeft + i * (dRight - dLeft) / (double)(nSamples - 1), 1e-7);
					}
				}
				else
				{
					while (dVal[1] == 0.0 && i + 1 < nSamples)
					{
						i++;
						dVal[1] = _parser.Diff(dVar, dLeft + i * (dRight - dLeft) / (double)(nSamples - 1), 1e-7);
					}
				}

				// Store the current location
				vResults.push_back(parser_LocalizeExtremum(sExpr, dVar, _parser, _option, dLeft + nTemp * (dRight - dLeft) / (double)(nSamples - 1), dLeft + i * (dRight - dLeft) / (double)(nSamples - 1)));
			}
		}
		dVal[0] = dVal[1];
	}

	// If we didn't find any results
	// examine the boundaries for possible extremas
	if (!sCmd.length() && !vResults.size())
	{
		dVal[0] = _parser.Diff(dVar, dLeft);
		dVal[1] = _parser.Diff(dVar, dRight);

        // Examine the left boundary
		if (dVal[0]
				&& (!nMode
					|| (dVal[0] < 0 && nMode == 1)
					|| (dVal[0] > 0 && nMode == -1)))
			sCmd = toString(dLeft, _option);

		// Examine the right boundary
		if (dVal[1]
				&& (!nMode
					|| (dVal[1] < 0 && nMode == -1)
					|| (dVal[1] > 0 && nMode == 1)))
		{
			if (sCmd.length())
				sCmd += ", ";
			sCmd += toString(dRight, _option);
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

// This function is a wrapper to the actual zeros
// localisation function "parser_LocalizeZero" further
// below.
bool parser_findZeroes(string& sCmd, Datafile& _data, Parser& _parser, const Settings& _option, Define& _functions)
{
	unsigned int nSamples = 21;
	double dVal[2];
	double dLeft = 0.0;
	double dRight = 0.0;
	int nMode = 0;
	double* dVar = 0;
	double dTemp = 0.0;
	string sExpr = "";
	string sParams = "";
	string sInterval = "";
	string sVar = "";

	// We cannot find zeroes in strings
	if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
	{
		throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "zeroes");
	}

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
	else if (sCmd.find("data(") == string::npos && !_data.containsCacheElements(sCmd))
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
	if (sExpr.find("data(") != string::npos || _data.containsCacheElements(sExpr))
	{
		getDataElements(sExpr, _parser, _data, _option, false);
	}
	if (sParams.find("data(") != string::npos || _data.containsCacheElements(sParams))
	{
		getDataElements(sParams, _parser, _data, _option, false);
	}

	// Evaluate the parameter list
	if (matchParams(sParams, "min") || matchParams(sParams, "down"))
		nMode = -1;
	if (matchParams(sParams, "max") || matchParams(sParams, "up"))
		nMode = 1;
	if (matchParams(sParams, "samples", '='))
	{
		_parser.SetExpr(getArgAtPos(sParams, matchParams(sParams, "samples", '=') + 7));
		nSamples = (int)_parser.Eval();
		if (nSamples < 21)
			nSamples = 21;
		sParams.erase(matchParams(sParams, "samples", '=') - 1, 8);
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

		value_type* v = 0;
		Datafile _cache;
		_cache.setCacheStatus(true);
		int nResults = 0;
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
		_parser.SetExpr(sExpr);
		v = _parser.Eval(nResults);
		if (nResults > 1)
		{
			vector<double> vResults;
			int nResults_x = 0;
			for (int i = 0; i < nResults; i++)
			{
				_cache.writeToCache(i, 1, "cache", v[i]);
			}
			_parser.SetExpr(sInterval);
			v = _parser.Eval(nResults_x);
			if (nResults_x > 1)
			{
				for (int i = 0; i < nResults; i++)
				{
					if (i >= nResults_x)
					{
						_cache.writeToCache(i, 0, "cache", 0.0);
					}
					else
					{
						_cache.writeToCache(i, 0, "cache", v[i]);
					}
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
							if (j + 1 == _cache.getLines("cache", false) && i > 1 && nMode * _data.getElement(i - 2, 1, "cache") < 0.0)
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
		else
		{
			if (!parser_CheckVarOccurence(_parser, sVar))
			{
				if (!_parser.Eval())
					sCmd = "\"Der Ausdruck ist auf dem gesamten Intervall identisch Null!\"";
				else
					sCmd = toSystemCodePage("\"Bezüglich der Variablen " + sVar + " ist der Ausdruck konstant und besitzt keine Nullstellen!\"");
				return true;
			}
			dVar = parser_GetVarAdress(sVar, _parser);
			if (!dVar)
			{
				throw SyntaxError(SyntaxError::ZEROES_VAR_NOT_FOUND, sCmd, sVar, sVar);
			}
			if (sInterval.find(':') == string::npos || sInterval.length() < 3)
				return false;
			if (isNotEmptyExpression(sInterval.substr(0, sInterval.find(':'))))
			{
				_parser.SetExpr(sInterval.substr(0, sInterval.find(':')));
				dLeft = _parser.Eval();
				if (isinf(dLeft) || isnan(dLeft))
				{
					sCmd = "nan";
					return false;
				}
			}
			else
				return false;
			if (isNotEmptyExpression(sInterval.substr(sInterval.find(':') + 1)))
			{
				_parser.SetExpr(sInterval.substr(sInterval.find(':') + 1));
				dRight = _parser.Eval();
				if (isinf(dRight) || isnan(dRight))
				{
					sCmd = "nan";
					return false;
				}
			}
			else
				return false;
			if (dRight < dLeft)
			{
				double Temp = dRight;
				dRight = dLeft;
				dLeft = Temp;
			}
		}
	}
	else if (sCmd.find("data(") != string::npos || _data.containsCacheElements(sCmd))
	{
		value_type* v;
		int nResults = 0;
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
	else
		throw SyntaxError(SyntaxError::NO_ZEROES_VAR, sCmd, SyntaxError::invalid_position);

    // Calculate the interval
	if ((int)(dRight - dLeft))
	{
		nSamples = (nSamples - 1) * (int)(dRight - dLeft) + 1;
	}

	// Ensure that we calculate a reasonable
	// amount of samples
	if (nSamples > 10001)
		nSamples = 10001;

    // Set the expression and evaluate it once
	_parser.SetExpr(sExpr);
	_parser.Eval();
	sCmd = "";
	dTemp = *dVar;

	*dVar = dLeft;
	vector<double> vResults;
	dVal[0] = _parser.Eval();

	// Find near zeros to the left of the boundary
	// which are probably not located due toe rounding
	// errors
	if (dVal[0] != 0.0 && fabs(dVal[0]) < 1e-10)
	{
		*dVar = dLeft - 1e-10;
		dVal[1] = _parser.Eval();
		if (dVal[0]*dVal[1] < 0 && (nMode * dVal[0] <= 0.0))
		{
			vResults.push_back(parser_LocalizeExtremum(sExpr, dVar, _parser, _option, dLeft - 1e-10, dLeft));
		}
	}

	// Evaluate all samples. We try to find
	// sign changes and evaluate the intervals, which
	// contain the sign changes, further
	for (unsigned int i = 1; i < nSamples; i++)
	{
	    // Evalute the current sample
		*dVar = dLeft + i * (dRight - dLeft) / (double)(nSamples - 1);
		dVal[1] = _parser.Eval();
		if (dVal[0]*dVal[1] < 0)
		{
			if (!nMode
					|| (nMode == -1 && (dVal[0] > 0 && dVal[1] < 0))
					|| (nMode == 1 && (dVal[0] < 0 && dVal[1] > 0)))
			{
			    // Examine the current interval
				vResults.push_back((parser_LocalizeZero(sExpr, dVar, _parser, _option, dLeft + (i - 1) * (dRight - dLeft) / (double)(nSamples - 1), dLeft + i * (dRight - dLeft) / (double)(nSamples - 1))));
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
						*dVar = dLeft + i * (dRight - dLeft) / (double)(nSamples - 1);
						dVal[1] = _parser.Eval();
					}
				}
				else
				{
					while (dVal[1] == 0.0 && i + 1 < nSamples)
					{
						i++;
						*dVar = dLeft + i * (dRight - dLeft) / (double)(nSamples - 1);
						dVal[1] = _parser.Eval();
					}
				}

				// Store the result
				vResults.push_back(parser_LocalizeZero(sExpr, dVar, _parser, _option, dLeft + nTemp * (dRight - dLeft) / (double)(nSamples - 1), dLeft + i * (dRight - dLeft) / (double)(nSamples - 1)));
			}
		}
		dVal[0] = dVal[1];
	}

	// Examine the right boundary, because there might be
	// a zero slightly right from the interval
	if (dVal[0] != 0.0 && fabs(dVal[0]) < 1e-10)
	{
		*dVar = dRight + 1e-10;
		dVal[1] = _parser.Eval();
		if (dVal[0]*dVal[1] < 0 && nMode * dVal[0] <= 0.0)
		{
			vResults.push_back(parser_LocalizeZero(sExpr, dVar, _parser, _option, dRight, dRight + 1e-10));
		}
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

// This function searches for the position of all extrema,
// which are located in the selected interval. The expression
// has to be setted in advance. The function performs recursions
// until the defined precision is reached
static double parser_LocalizeExtremum(string& sCmd, double* dVarAdress, Parser& _parser, const Settings& _option, double dLeft, double dRight, double dEps, int nRecursion)
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
			{
				return dLeft + (i - 1) * (dRight - dLeft) / (double)(nSamples - 1) + Linearize(0.0, dVal[0], (dRight - dLeft) / (double)(nSamples - 1), dVal[1]);
			}
			else
				return parser_LocalizeExtremum(sCmd, dVarAdress, _parser, _option, dLeft + (i - 1) * (dRight - dLeft) / (double)(nSamples - 1), dLeft + i * (dRight - dLeft) / (double)(nSamples - 1), dEps, nRecursion + 1);
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
			{
				return dLeft + nTemp * (dRight - dLeft) / (double)(nSamples - 1) + Linearize(0.0, dVal[0], (i - nTemp) * (dRight - dLeft) / (double)(nSamples - 1), dVal[1]);
			}
			else
				return parser_LocalizeExtremum(sCmd, dVarAdress, _parser, _option, dLeft + nTemp * (dRight - dLeft) / (double)(nSamples - 1), dLeft + i * (dRight - dLeft) / (double)(nSamples - 1), dEps, nRecursion + 1);
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

// This function searches for the position of all zeroes (roots),
// which are located in the selected interval. The expression
// has to be setted in advance. The function performs recursions
// until the defined precision is reached
static double parser_LocalizeZero(string& sCmd, double* dVarAdress, Parser& _parser, const Settings& _option, double dLeft, double dRight, double dEps, int nRecursion)
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
			{
				return dLeft + (i - 1) * (dRight - dLeft) / (double)(nSamples - 1) + Linearize(0.0, dVal[0], (dRight - dLeft) / (double)(nSamples - 1), dVal[1]);
			}
			else
				return parser_LocalizeZero(sCmd, dVarAdress, _parser, _option, dLeft + (i - 1) * (dRight - dLeft) / (double)(nSamples - 1), dLeft + i * (dRight - dLeft) / (double)(nSamples - 1), dEps, nRecursion + 1);
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
			{
				return dLeft + nTemp * (dRight - dLeft) / (double)(nSamples - 1) + Linearize(0.0, dVal[0], (i - nTemp) * (dRight - dLeft) / (double)(nSamples - 1), dVal[1]);
			}
			else
				return parser_LocalizeZero(sCmd, dVarAdress, _parser, _option, dLeft + nTemp * (dRight - dLeft) / (double)(nSamples - 1), dLeft + i * (dRight - dLeft) / (double)(nSamples - 1), dEps, nRecursion + 1);
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

// This function approximates the passed expression using
// Taylor's method. The aproximated function is defined
// as a new custom function
void parser_Taylor(string& sCmd, Parser& _parser, const Settings& _option, Define& _functions)
{
	string sParams = "";
	string sVarName = "";
	string sExpr = "";
	string sExpr_cpy = "";
	string sArg = "";
	string sTaylor = "Taylor";
	string sPolynom = "";
	bool bUseUniqueName = false;
	unsigned int nth_taylor = 6;
	unsigned int nSamples = 0;
	unsigned int nMiddle = 0;
	double* dVar = 0;
	double dVarValue = 0.0;
	long double** dDiffValues = 0;

	// We cannot approximate string expressions
	if (containsStrings(sCmd))
	{
		throw SyntaxError(SyntaxError::STRINGS_MAY_NOT_BE_EVALUATED_WITH_CMD, sCmd, SyntaxError::invalid_position, "taylor");
	}

	// Extract the parameter list
	if (sCmd.find("-set") != string::npos)
	{
		sParams = sCmd.substr(sCmd.find("-set"));
	}
	else if (sCmd.find("--") != string::npos)
	{
		sParams = sCmd.substr(sCmd.find("--"));
	}
	else
	{
		NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_TAYLOR_MISSINGPARAMS"), _option));
		return;
	}

	// Evaluate the parameters
	if (matchParams(sParams, "n", '='))
	{
		_parser.SetExpr(sParams.substr(matchParams(sParams, "n", '=') + 1, sParams.find(' ', matchParams(sParams, "n", '=') + 1) - matchParams(sParams, "n", '=') - 1));
		nth_taylor = (unsigned int)_parser.Eval();
		if (isinf(_parser.Eval()) || isnan(_parser.Eval()))
			nth_taylor = 6;
		sParams = sParams.substr(0, matchParams(sParams, "n", '=') - 1) + sParams.substr(matchParams(sParams, "n", '=') - 1 + _parser.GetExpr().length());
	}
	if (matchParams(sParams, "unique") || matchParams(sParams, "u"))
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
			sArg = "*x^";
		else if (dVarValue < 0)
			sArg = "*(x+" + toString(-dVarValue, _option.getPrecision()) + ")^";
		else
			sArg = "*(x-" + toString(dVarValue, _option.getPrecision()) + ")^";
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
	if (!parser_CheckVarOccurence(_parser, sVarName))
	{
		NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_TAYLOR_CONSTEXPR", sVarName), _option));
		return;
	}

	// Get the address of the selected variable
	if (sVarName.length())
		dVar = parser_GetVarAdress(sVarName, _parser);
	if (!dVar)
		return;

    // If unique function names are desired,
    // generate them here by removing all operators
    // from the string
	if (bUseUniqueName)
	{
		for (unsigned int i = 0; i < sTaylor.length(); i++)
		{
			if (sTaylor[i] == ' '
                || sTaylor[i] == ','
                || sTaylor[i] == ';'
                || sTaylor[i] == '-'
                || sTaylor[i] == '*'
                || sTaylor[i] == '/'
                || sTaylor[i] == '%'
                || sTaylor[i] == '^'
                || sTaylor[i] == '!'
                || sTaylor[i] == '<'
                || sTaylor[i] == '>'
                || sTaylor[i] == '&'
                || sTaylor[i] == '|'
                || sTaylor[i] == '?'
                || sTaylor[i] == ':'
                || sTaylor[i] == '='
                || sTaylor[i] == '+'
                || sTaylor[i] == '['
                || sTaylor[i] == ']'
                || sTaylor[i] == '{'
                || sTaylor[i] == '}'
                || sTaylor[i] == '('
                || sTaylor[i] == ')')
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

		// Ignore zeros in the constant term
		if (toString(_parser.Eval(), _option) != "0")
			sPolynom = toString(_parser.Eval(), _option);

		// Handle the linear term
		if (toString(_parser.Diff(dVar, dVarValue, 1e-7), _option) == "0")
		{
		    // If it doesn't exist, then the only significant term is the constant term
			if (!sPolynom.length())
				sPolynom = "0";
		}
		else if (_parser.Diff(dVar, dVarValue) < 0)
			sPolynom += " - " + toString(-_parser.Diff(dVar, dVarValue, 1e-7), _option);
		else if (sPolynom.length())
			sPolynom += " + " + toString(_parser.Diff(dVar, dVarValue, 1e-7), _option);
		else
			sPolynom = toString(_parser.Diff(dVar, dVarValue, 1e-7), _option);

		// Append the linear term, and the polynomial factor
		// to the overall polynomial
		if (toString(_parser.Diff(dVar, dVarValue, 1e-7), _option) != "0")
			sPolynom += sArg.substr(0, sArg.length() - 1);
		sTaylor += sPolynom;
	}
	else
	{
	    // nth order polynomial
		*dVar = dVarValue;

		// Ignore zeros in the constant term
		if (toString(_parser.Eval(), _option) != "0")
			sPolynom = toString(_parser.Eval(), _option);

        // Handle the linear term
		if (toString(_parser.Diff(dVar, dVarValue, 1e-7), _option) != "0")
		{
			if (_parser.Diff(dVar, dVarValue, 1e-7) < 0)
				sPolynom += " - " + toString(-_parser.Diff(dVar, dVarValue, 1e-7), _option);
			else if (sPolynom.length())
				sPolynom += " + " + toString(_parser.Diff(dVar, dVarValue, 1e-7), _option);
			else
				sPolynom = toString(_parser.Diff(dVar, dVarValue, 1e-7), _option);
			sPolynom += sArg.substr(0, sArg.length() - 1);
		}

		nSamples = 4 * nth_taylor + 1;
		nMiddle = 2 * nth_taylor;

		// Create the memory for the derivatives
		dDiffValues = new long double*[nSamples];
		for (unsigned int i = 0; i < nSamples; i++)
		{
			dDiffValues[i] = new long double[2];
		}

		// Fill the first column with the x-axis values
		for (unsigned int i = 0; i < nSamples; i++)
		{
			dDiffValues[i][0] = dVarValue + ((double)i - (double)nMiddle) * 1e-1;
		}

		// Fill the second column with the first
		// order derivatives
		for (unsigned int i = 0; i < nSamples; i++)
		{
			dDiffValues[i][1] = _parser.Diff(dVar, dDiffValues[i][0], 1e-7);
		}

		// Evaluate the nth taylor polynomial and the nth
		// order derivative
		for (unsigned int j = 1; j < nth_taylor; j++)
		{
			for (unsigned int i = nMiddle; i < nSamples - j; i++)
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

			if (toString((double)dDiffValues[nMiddle][1], _option) == "0")
				continue;
			else if (dDiffValues[nMiddle][1] < 0)
				sPolynom += " - " + toString(-(double)dDiffValues[nMiddle][1] / int_faculty((int)j + 1), _option);
			else if (sPolynom.length())
				sPolynom += " + " + toString((double)dDiffValues[nMiddle][1] / int_faculty((int)j + 1), _option);
			else
				sPolynom = toString((double)dDiffValues[nMiddle][1] / int_faculty((int)j + 1), _option);

			sPolynom += sArg + toString((int)j + 1);
		}

		if (!sPolynom.length())
			sTaylor += "0";
		else
			sTaylor += sPolynom;
		for (unsigned int i = 0; i < nSamples; i++)
		{
			delete[] dDiffValues[i];
		}
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
        NumeReKernel::print(_lang.get("DEFINE_SUCCESS"));
    else
        NumeReKernel::issueWarning(_lang.get("DEFINE_FAILURE"));

	return;
}

int int_faculty(int nNumber)
{
	if (nNumber < 0)
		nNumber *= -1;
	if (nNumber == 0)
		return 1;
	for (int i = nNumber - 1; i > 0; i--)
	{
		nNumber *= i;
	}
	return nNumber;
}

bool parser_parseCmdArg(const string& sCmd, const string& sParam, Parser& _parser, int& nArgument)
{
	if (!sCmd.length() || !sParam.length())
		return false;

	unsigned int nPos = 0;
	if (matchParams(sCmd, sParam) || matchParams(sCmd, sParam, '='))
	{
		if (matchParams(sCmd, sParam))
		{
			nPos = matchParams(sCmd, sParam) + sParam.length();
		}
		else
		{
			nPos = matchParams(sCmd, sParam, '=') + sParam.length();
		}
		while (sCmd[nPos] == ' ' && nPos < sCmd.length() - 1)
			nPos++;
		if (sCmd[nPos] == ' ' || nPos >= sCmd.length() - 1)
			return false;

		string sArg = sCmd.substr(nPos);
		if (sArg[0] == '(')
			sArg = sArg.substr(1, getMatchingParenthesis(sArg) - 1);
		else
			sArg = sArg.substr(0, sArg.find(' '));
		_parser.SetExpr(sArg);
		if (isnan(_parser.Eval()) || isinf(_parser.Eval()))
			return false;
		nArgument = (int)_parser.Eval();
		return true;
	}
	return false;
}

bool parser_fit(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
	vector<double> vx;
	vector<double> vy;
	vector<double> vy_w;
	vector<double> vTempZ;
	vector<vector<double> > vz;
	vector<vector<double> > vz_w;

	vector<double> vInitialVals;

	ofstream oFitLog;
	ofstream oTeXExport;
	string sFitLog = "<savepath>/numerefit.log";
	sFitLog = _data.ValidFileName(sFitLog, ".log");
	unsigned int nDim = 1;
	unsigned int nFitVars = 0;
	bool bUseErrors = false;
	bool bSaveErrors = false;
	double dChisq = 0.0;
	double dNormChisq = 0.0;
	bool bRestrictXVals = false;
	bool bRestrictYVals = false;
	bool bMaskDialog = false;
	bool bNoParams = false;
	bool b1DChiMap = false;
	bool bTeXExport = false;
	string sTeXExportFile = "<savepath>/fit.tex";
	double dMin = NAN;
	double dMax = NAN;
	double dMinY = NAN;
	double dMaxY = NAN;
	double dPrecision = 1e-4;
	int nMaxIterations = 500;

	double dErrorPercentageSum = 0.0;
	vector<double> vInterVal;

	Indices _idx;

	if (findCommand(sCmd, "fit").sString == "fitw")
		bUseErrors = true;

	if (sCmd.find("data(") == string::npos && !_data.containsCacheElements(sCmd))
		throw SyntaxError(SyntaxError::NO_DATA_FOR_FIT, sCmd, SyntaxError::invalid_position);
	string sBadFunctions = "ascii(),char(),findfile(),findparam(),gauss(),getopt(),is_string(),rand(),replace(),replaceall(),split(),strfnd(),string_cast(),strrfnd(),strlen(),time(),to_char(),to_cmd(),to_string(),to_value()";
	string sFitFunction = sCmd;
	string sParams = "";
	string sFuncDisplay = "";
	string sFunctionDefString = "";
	string sFittedFunction = "";
	string sRestrictions = "";
	string sChiMap = "";
	string sChiMap_Vars[2] = {"", ""};

	mu::varmap_type varMap;
	mu::varmap_type paramsMap;

	if (matchParams(sCmd, "chimap", '='))
	{
		sChiMap = getArgAtPos(sCmd, matchParams(sCmd, "chimap", '=') + 6);
		eraseToken(sCmd, "chimap", true);

		if (sChiMap.length())
		{
			if (sChiMap.substr(0, sChiMap.find('(')) == "data")
				throw SyntaxError(SyntaxError::READ_ONLY_DATA, sCmd, SyntaxError::invalid_position);
			_idx = parser_getIndices(sChiMap, _parser, _data, _option);
			if ((_idx.nI[0] == -1 || _idx.nJ[0] == -1) && (!_idx.vI.size() && !_idx.vJ.size()))
				throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
			if (_idx.vJ.size() && _idx.vJ.size() < 2)
				throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
			parser_evalIndices(sChiMap, _idx, _data);
			sChiMap.erase(sChiMap.find('('));
			if (!_idx.vJ.size())
			{
				if (_idx.nJ[1] < _idx.nJ[0])
				{
					sChiMap_Vars[0] = _data.getHeadLineElement(_idx.nJ[0], sChiMap);
					sChiMap_Vars[1] = _data.getHeadLineElement(_idx.nJ[0] - 1, sChiMap);
				}
				else
				{
					sChiMap_Vars[0] = _data.getHeadLineElement(_idx.nJ[0], sChiMap);
					sChiMap_Vars[1] = _data.getHeadLineElement(_idx.nJ[0] + 1, sChiMap);
				}
			}
			else
			{
				sChiMap_Vars[0] = _data.getHeadLineElement(_idx.vJ[0], sChiMap);
				sChiMap_Vars[1] = _data.getHeadLineElement(_idx.vJ[1], sChiMap);
			}
		}
	}
	if (matchParams(sCmd, "export", '='))
	{
		bTeXExport = true;
		sTeXExportFile = getArgAtPos(sCmd, matchParams(sCmd, "export", '=') + 6);
		eraseToken(sCmd, "export", true);
	}
	else if (matchParams(sCmd, "export"))
	{
		bTeXExport = true;
		eraseToken(sCmd, "export", false);
	}

	if (bTeXExport)
	{
		sTeXExportFile = _data.ValidFileName(sTeXExportFile, ".tex");
		if (sTeXExportFile.substr(sTeXExportFile.rfind('.')) != ".tex")
			sTeXExportFile.replace(sTeXExportFile.rfind('.'), string::npos, ".tex");
	}

	for (unsigned int i = 0; i < sCmd.length(); i++)
	{
		if (sCmd[i] == '(')
			i += getMatchingParenthesis(sCmd.substr(i));
		if (sCmd[i] == '-')
		{
			sCmd.erase(0, i);
			break;
		}
	}
	vInterVal = parser_IntervalReader(sCmd, _parser, _data, _functions, _option, true);
	if (vInterVal.size())
	{
		if (vInterVal.size() >= 4)
		{
			dMin = vInterVal[0];
			dMax = vInterVal[1];
			dMinY = vInterVal[2];
			dMaxY = vInterVal[3];
			if (!isnan(dMin) || !isnan(dMax))
				bRestrictXVals = true;
			if (!isnan(dMinY) || !isnan(dMaxY))
				bRestrictYVals = true;
		}
		else if (vInterVal.size() == 2)
		{
			dMin = vInterVal[0];
			dMax = vInterVal[1];
			if (!isnan(dMin) || !isnan(dMax))
				bRestrictXVals = true;
		}
	}
	for (unsigned int i = 0; i < sFitFunction.length(); i++)
	{
		if (sFitFunction[i] == '(')
			i += getMatchingParenthesis(sFitFunction.substr(i));
		if (sFitFunction[i] == '-')
		{
			sFitFunction.replace(i, string::npos, sCmd.substr(sCmd.find('-')));
			break;
		}
	}
	sCmd = sFitFunction;

	if (matchParams(sFitFunction, "saverr"))
	{
		bSaveErrors = true;
		sFitFunction.erase(matchParams(sFitFunction, "saverr") - 1, 6);
		sCmd.erase(matchParams(sCmd, "saverr") - 1, 6);
	}
	if (matchParams(sFitFunction, "saveer"))
	{
		bSaveErrors = true;
		sFitFunction.erase(matchParams(sFitFunction, "saveer") - 1, 6);
		sCmd.erase(matchParams(sCmd, "saveer") - 1, 6);
	}
	if (matchParams(sFitFunction, "mask"))
	{
		bMaskDialog = true;
		sFitFunction.erase(matchParams(sFitFunction, "mask") - 1, 6);
		sCmd.erase(matchParams(sCmd, "mask") - 1, 6);
	}
	if (!matchParams(sFitFunction, "with", '='))
		throw SyntaxError(SyntaxError::NO_FUNCTION_FOR_FIT, sCmd, SyntaxError::invalid_position);
	if (matchParams(sFitFunction, "tol", '='))
	{
		_parser.SetExpr(getArgAtPos(sFitFunction, matchParams(sFitFunction, "tol", '=') + 3));
		eraseToken(sCmd, "tol", true);
		eraseToken(sFitFunction, "tol", true);
		dPrecision = fabs(_parser.Eval());
		if (isnan(dPrecision) || isinf(dPrecision) || dPrecision == 0)
			dPrecision = 1e-4;
	}
	if (matchParams(sFitFunction, "iter", '='))
	{
		_parser.SetExpr(getArgAtPos(sFitFunction, matchParams(sFitFunction, "iter", '=') + 4));
		eraseToken(sCmd, "iter", true);
		eraseToken(sFitFunction, "iter", true);
		nMaxIterations = abs(rint(_parser.Eval()));
		if (!nMaxIterations)
			nMaxIterations = 500;
	}
	if (matchParams(sFitFunction, "restrict", '='))
	{
		sRestrictions = getArgAtPos(sFitFunction, matchParams(sFitFunction, "restrict", '=') + 8);
		eraseToken(sCmd, "restrict", true);
		eraseToken(sFitFunction, "restrict", true);
		if (sRestrictions.length() && sRestrictions.front() == '[' && sRestrictions.back() == ']')
		{
			sRestrictions.erase(0, 1);
			sRestrictions.pop_back();
		}
		StripSpaces(sRestrictions);
		if (sRestrictions.length())
		{
			if (sRestrictions.front() == ',')
				sRestrictions.erase(0, 1);
			if (sRestrictions.back() == ',')
				sRestrictions.pop_back();
			_parser.SetExpr(sRestrictions);
			_parser.Eval();
		}
	}
	if (!matchParams(sFitFunction, "params", '='))
	{
		bNoParams = true;
		sFitFunction = sFitFunction.substr(matchParams(sFitFunction, "with", '=') + 4);
		sCmd.erase(matchParams(sCmd, "with", '=') - 1);
	}
	else if (matchParams(sFitFunction, "with", '=') < matchParams(sFitFunction, "params", '='))
	{
		sParams = sFitFunction.substr(matchParams(sFitFunction, "params", '=') + 6);
		sFitFunction = sFitFunction.substr(matchParams(sFitFunction, "with", '=') + 4, matchParams(sFitFunction, "params", '=') - 1 - matchParams(sFitFunction, "with", '=') - 4);
		sCmd = sCmd.substr(0, matchParams(sCmd, "with", '=') - 1);
	}
	else
	{
		sParams = sFitFunction.substr(matchParams(sFitFunction, "params", '=') + 6, matchParams(sFitFunction, "with", '=') - 1 - matchParams(sFitFunction, "params", '=') - 6);
		sFitFunction = sFitFunction.substr(matchParams(sFitFunction, "with", '=') + 4);
		sCmd = sCmd.substr(0, matchParams(sCmd, "params", '=') - 1);
	}
	if (sParams.find('[') != string::npos)
		sParams = sParams.substr(sParams.find('[') + 1);
	if (sParams.find(']') != string::npos)
		sParams = sParams.substr(0, sParams.find(']'));
	StripSpaces(sFitFunction);
	if (sFitFunction[sFitFunction.length() - 1] == '-')
	{
		sFitFunction[sFitFunction.length() - 1] = ' ';
		StripSpaces(sFitFunction);
	}
	if (!bNoParams)
	{
		StripSpaces(sParams);
		if (sParams[sParams.length() - 1] == '-')
		{
			sParams[sParams.length() - 1] = ' ';
			StripSpaces(sParams);
		}
		if (!_functions.call(sParams))
			throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, sParams, sParams);
		if (sParams.find("data(") != string::npos || _data.containsCacheElements(sParams))
		{
			getDataElements(sParams, _parser, _data, _option);
		}
		if (sParams.find("{") != string::npos && (containsStrings(sParams) || _data.containsStringVars(sParams)))
			parser_VectorToExpr(sParams, _option);
	}
	StripSpaces(sCmd);

	if (!_functions.call(sFitFunction))
		throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, sFitFunction, sFitFunction);

	if (sFitFunction.find("data(") != string::npos || _data.containsCacheElements(sFitFunction))
	{
		getDataElements(sFitFunction, _parser, _data, _option);
	}
	if (sFitFunction.find("{") != string::npos)
		parser_VectorToExpr(sFitFunction, _option);

	unsigned int nPos = 0;
	while (sBadFunctions.find(',', nPos) != string::npos)
	{
		if (sFitFunction.find(sBadFunctions.substr(nPos, sBadFunctions.find(',', nPos) - nPos - 1)) != string::npos)
		{
			throw SyntaxError(SyntaxError::FUNCTION_CANNOT_BE_FITTED, sCmd, SyntaxError::invalid_position, sBadFunctions.substr(nPos, sBadFunctions.find(',', nPos) - nPos));
		}
		else
			nPos = sBadFunctions.find(',', nPos) + 1;
		if (nPos >= sBadFunctions.length())
			break;
	}
	nPos = 0;

	sFitFunction = " " + sFitFunction + " ";
	_parser.SetExpr(sFitFunction);
	_parser.Eval();
	varMap = _parser.GetUsedVar();
	if (bNoParams)
	{
		paramsMap = varMap;
		if (paramsMap.find("x") != paramsMap.end())
			paramsMap.erase(paramsMap.find("x"));
		if (paramsMap.find("y") != paramsMap.end())
			paramsMap.erase(paramsMap.find("y"));
		if (!paramsMap.size())
			throw SyntaxError(SyntaxError::NO_PARAMS_FOR_FIT, sCmd, SyntaxError::invalid_position);
	}
	else
	{
		_parser.SetExpr(sParams);
		_parser.Eval();
		if (sParams.find('=') != string::npos)
		{
			for (unsigned int i = 0; i < sParams.length(); i++)
			{
				if (sParams[i] == '=')
				{
					for (unsigned int j = i; j < sParams.length(); j++)
					{
						if (sParams[j] == '(')
							j += getMatchingParenthesis(sParams.substr(j));
						if (sParams[j] == ',')
						{
							sParams.erase(i, j - i);
							break;
						}
						if (j == sParams.length() - 1)
							sParams.erase(i);
					}
				}
			}
			_parser.SetExpr(sParams);
			_parser.Eval();
		}
		paramsMap = _parser.GetUsedVar();
	}


	mu::varmap_type::const_iterator pItem = paramsMap.begin();
	mu::varmap_type::const_iterator vItem = varMap.begin();
	sParams = "";
	if (varMap.find("x") != varMap.end())
		nFitVars += 1;
	if (varMap.find("y") != varMap.end())
		nFitVars += 2;
	if (varMap.find("z") != varMap.end())
		nFitVars += 4;

	if (sChiMap.length())
	{
		if (sChiMap_Vars[0] == "x" || sChiMap_Vars[1] == "x")
		{
			throw SyntaxError(SyntaxError::CANNOT_BE_A_FITTING_PARAM, sCmd, SyntaxError::invalid_position, "x");
		}
		if (sChiMap_Vars[0] == "y" || sChiMap_Vars[1] == "y")
		{
			throw SyntaxError(SyntaxError::CANNOT_BE_A_FITTING_PARAM, sCmd, SyntaxError::invalid_position, "y");
		}
		if (varMap.find(sChiMap_Vars[0]) == varMap.end())
		{
			throw SyntaxError(SyntaxError::FITFUNC_NOT_CONTAINS, sCmd, SyntaxError::invalid_position, sChiMap_Vars[0]);
		}
		if (varMap.find(sChiMap_Vars[1]) == varMap.end())
		{
			b1DChiMap = true;
		}
	}

	if (!nFitVars || !(nFitVars & 1))
	{
		throw SyntaxError(SyntaxError::FITFUNC_NOT_CONTAINS, sCmd, SyntaxError::invalid_position, "x");
	}

	pItem = paramsMap.begin();
	for (; pItem != paramsMap.end(); ++pItem)
	{
		if (pItem->first == "x" || pItem->first == "y" || pItem->first == "z")
		{
			throw SyntaxError(SyntaxError::CANNOT_BE_A_FITTING_PARAM, sCmd, SyntaxError::invalid_position, pItem->first);
		}

		bool bParamFound = false;
		vItem = varMap.begin();
		for (; vItem != varMap.end(); ++vItem)
		{
			if (vItem->first == pItem->first)
			{
				bParamFound = true;
				break;
			}
		}
		if (!bParamFound)
		{
			throw SyntaxError(SyntaxError::FITFUNC_NOT_CONTAINS, sCmd, SyntaxError::invalid_position, pItem->first);
		}
	}
	if (sChiMap.length())
	{
		paramsMap.erase(sChiMap_Vars[0]);
		if (!b1DChiMap)
			paramsMap.erase(sChiMap_Vars[1]);
	}
	if (!paramsMap.size())
		throw SyntaxError(SyntaxError::NO_PARAMS_FOR_FIT, sCmd, SyntaxError::invalid_position);

	sFuncDisplay = sFitFunction;
	StripSpaces(sFuncDisplay);
	pItem = paramsMap.begin();

	if (_option.getbDebug())
		cerr << "|-> DEBUG: sFitFunction = " << sFitFunction << endl;
	sCmd.erase(0, findCommand(sCmd).nPos + findCommand(sCmd).sString.length());
	StripSpaces(sCmd);

	string si_pos[2] = {"", ""};                    // String-Array fuer die Zeilen-Position: muss fuer alle Spalten identisch sein!
	string sj_pos[6] = {"", "", "", "", "", ""};    // String-Array fuer die Spalten: kann bis zu sechs beliebige Werte haben
	string sDataTable = "data";
	int i_pos[2] = {0, 0};                          // Int-Array fuer den Wert der Zeilen-Positionen
	int j_pos[6] = {0, 0, 0, 0, 0, 0};              // Int-Array fuer den Wert der Spalten-Positionen
	int nMatch = 0;                                 // Int fuer die Position des aktuellen find-Treffers eines Daten-Objekts
	vector<long long int> vLine;
	vector<long long int> vCol;
	value_type* v = 0;
	int nResults = 0;

	// --> Ist da "cache" drin? Aktivieren wir den Cache-Status <--
	if (_data.containsCacheElements(sCmd) && sCmd.substr(0, 5) != "data(")
	{
		if (_data.isValidCache())
			_data.setCacheStatus(true);
		else
			throw SyntaxError(SyntaxError::NO_CACHED_DATA, sCmd, SyntaxError::invalid_position);

		for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
		{
			if (sCmd.find(iter->first + "(") != string::npos
					&& (!sCmd.find(iter->first + "(")
						|| (sCmd.find(iter->first + "(") && checkDelimiter(sCmd.substr(sCmd.find(iter->first + "(") - 1, (iter->first).length() + 2)))))
			{
				sDataTable = iter->first;
				break;
			}
		}
	}
	else if (!_data.isValid())
		throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);

	// --> Klammer und schliessende Klammer finden und in einen anderen String schreiben <--
	nMatch = sCmd.find('(');
	si_pos[0] = sCmd.substr(nMatch, getMatchingParenthesis(sCmd.substr(nMatch)) + 1);
	if (si_pos[0] == "()" || si_pos[0][si_pos[0].find_first_not_of(' ', 1)] == ')')
		si_pos[0] = "(:,:)";
	if (si_pos[0].find("data(") != string::npos || _data.containsCacheElements(si_pos[0]))
	{
		getDataElements(si_pos[0], _parser, _data, _option);
	}
	if (_option.getbDebug())
		cerr << "|-> DEBUG: si_pos[0] = " << si_pos[0] << endl;

    // Update the dimension variables used for accessing the elements
    _data.updateDimensionVariables(sDataTable);


	// --> Rausgeschnittenen String am Komma ',' in zwei Teile teilen <--
	try
	{
		parser_SplitArgs(si_pos[0], sj_pos[0], ',', _option);
	}
	catch (...)
	{
		throw;
	}
	if (_option.getbDebug())
		cerr << "|-> DEBUG: si_pos[0] = " << si_pos[0] << ", sj_pos[0] = " << sj_pos[0] << endl;

	// --> Gibt's einen Doppelpunkt? Dann teilen wir daran auch noch mal <--
	if (si_pos[0].find(':') != string::npos && si_pos[0].find('{') == string::npos)
	{
		si_pos[0] = "( " + si_pos[0] + " )";
		try
		{
			parser_SplitArgs(si_pos[0], si_pos[1], ':', _option);
		}
		catch (...)
		{
			throw;
		}
		if (!isNotEmptyExpression(si_pos[1]))
			si_pos[1] = "inf";
	}
	else
		si_pos[1] = "";

	// --> Auswerten mit dem Parser <--
	if (isNotEmptyExpression(si_pos[0]))
	{
		_parser.SetExpr(si_pos[0]);
		v = _parser.Eval(nResults);
		if (nResults > 1)
		{
			for (int n = 0; n < nResults; n++)
				vLine.push_back((int)v[n] - 1);
		}
		else
			i_pos[0] = (int)v[0] - 1;
	}
	else
		i_pos[0] = 0;
	if (si_pos[1] == "inf")
	{
		i_pos[1] = _data.getLines(sDataTable, false);
	}
	else if (isNotEmptyExpression(si_pos[1]))
	{
		_parser.SetExpr(si_pos[1]);
		i_pos[1] = (int)_parser.Eval();
	}
	else if (!vLine.size())
		i_pos[1] = i_pos[0] + 1;
	// --> Pruefen, ob die Reihenfolge der Indices sinnvoll ist <--
	parser_CheckIndices(i_pos[0], i_pos[1]);

	if (_option.getbDebug())
		cerr << "|-> DEBUG: i_pos[0] = " << i_pos[0] << ", i_pos[1] = " << i_pos[1] << ", vLine.size() = " << vLine.size() << endl;

	if (!isNotEmptyExpression(sj_pos[0]))
		sj_pos[0] = "0";

	/* --> Jetzt fuer die Spalten: Fummelig. Man soll bis zu 6 Spalten angeben koennen und
	 *     das Programm sollte trotzdem einen Sinn darin finden <--
	 */
	int j = 0;
	try
	{
		while (sj_pos[j].find(':') != string::npos && sj_pos[0].find('{') == string::npos && j < 5)
		{
			sj_pos[j] = "( " + sj_pos[j] + " )";
			// --> String am naechsten ':' teilen <--
			parser_SplitArgs(sj_pos[j], sj_pos[j + 1], ':', _option);
			// --> Spezialfaelle beachten: ':' ohne linke bzw. rechte Grenze <--
			if (!isNotEmptyExpression(sj_pos[j]))
				sj_pos[j] = "1";
			j++;
			if (!isNotEmptyExpression(sj_pos[j]))
				sj_pos[j] = "inf";
		}
	}
	catch (...)
	{
		throw;
	}

	// --> Alle nicht-beschriebenen Grenzen-Strings auf "" setzen <--
	for (int k = j + 1; k < 6; k++)
		sj_pos[k] = "";

	// --> Grenzen-Strings moeglichst sinnvoll auswerten <--
	for (int k = 0; k <= j; k++)
	{
		// --> "inf" bedeutet "infinity". Ergo: die letztmoegliche Spalte <--
		if (sj_pos[k] == "inf")
		{
			j_pos[k] = _data.getCols(sDataTable) - 1;
			break;
		}
		else if (isNotEmptyExpression(sj_pos[k]))
		{
			if (j == 0)
			{
				_parser.SetExpr(sj_pos[0]);
				v = _parser.Eval(nResults);
				if (nResults > 1)
				{
					for (int n = 0; n < nResults; n++)
					{
						if (n >= 6)
							break;
						vCol.push_back((int)v[n] - 1);
						j_pos[n] = (int)v[n] - 1;
						j = n;
					}
					break;
				}
				else
					j_pos[0] = (int)v[0] - 1;
			}
			else
			{
				// --> Hat einen Wert: Kann man auch auswerten <--
				_parser.SetExpr(sj_pos[k]);
				j_pos[k] = (int)_parser.Eval() - 1;
			}
		}
		else if (!k)
		{
			// --> erstes Element pro Forma auf 0 setzen <--
			j_pos[k] = 0;
		}
		else // "data(2:4::7) = Spalten 2-4,5-7"
		{
			// --> Spezialfall. Verwendet vermutlich niemand <--
			j_pos[k] = j_pos[k] + 1;
		}
	}
	if (i_pos[1] > _data.getLines(sDataTable, false))
		i_pos[1] = _data.getLines(sDataTable, false);
	if (j_pos[1] > _data.getCols(sDataTable) - 1)
		j_pos[1] = _data.getCols(sDataTable) - 1;
	if (!vLine.size() && !vCol.size() && (j_pos[0] < 0
										  || j_pos[1] < 0
										  || i_pos[0] > _data.getLines(sDataTable, false)
										  || i_pos[1] > _data.getLines(sDataTable, false)
										  || j_pos[0] > _data.getCols(sDataTable) - 1
										  || j_pos[1] > _data.getCols(sDataTable) - 1))
	{
		throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
	}

	// --> Jetzt wissen wir die Spalten: Suchen wir im Falle von si_pos[1] == inf nach der laengsten <--
	if (si_pos[1] == "inf")
	{
		int nAppendedZeroes = _data.getAppendedZeroes(j_pos[0], sDataTable);
		for (int k = 1; k <= j; k++)
		{
			if (nAppendedZeroes > _data.getAppendedZeroes(j_pos[k], sDataTable))
				nAppendedZeroes = _data.getAppendedZeroes(j_pos[k], sDataTable);
		}
		if (nAppendedZeroes < i_pos[1])
			i_pos[1] = _data.getLines(sDataTable, true) - nAppendedZeroes;
	}


	/* --> Bestimmen wir die "Dimension" des zu fittenden Datensatzes. Dabei ist es auch
	 *     von Bedeutung, ob Fehlerwerte verwendet werden sollen <--
	 */
	nDim = 0;
	if (j == 0 && bUseErrors && vCol.size() < 3)
		throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, SyntaxError::invalid_position);
	if (j == 0 && !vCol.size())
		nDim = 2;
	else if (j == 0)
		nDim = vCol.size();
	else if (j == 1)
	{
		if (!bUseErrors)
		{
			if (!(nFitVars & 2))
			{
				nDim = 2;
				if (abs(j_pos[1] - j_pos[0]) < 1)
					throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, SyntaxError::invalid_position);
			}
			else
			{
				nDim = 3;
				if (abs(j_pos[1] - j_pos[0]) < abs(i_pos[1] - i_pos[0]) + 1)
					throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, SyntaxError::invalid_position);
			}
		}
		else
		{
			if (!(nFitVars & 2))
			{
				nDim = 4;
				if (abs(j_pos[1] - j_pos[0]) < 2)
					throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, SyntaxError::invalid_position);
			}
			else
			{
				nDim = 5;
				if (abs(j_pos[1] - j_pos[0]) < 2 * abs(i_pos[1] - i_pos[0]) + 1)
					throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, SyntaxError::invalid_position);
			}
		}
	}
	else
	{
		nDim = j + 1;
	}

	parser_CheckIndices(i_pos[0], i_pos[1]);
	// Groesse der Datensaetze bestimmen:
	if (vLine.size() && !vCol.size())
	{
		vCol.push_back(j_pos[0]);
		if (j == 1)
		{
			if (nDim == 2)
			{
				if (sj_pos[1] == "inf")
					vCol.push_back(j_pos[0] + 1);
				else
					vCol.push_back(j_pos[1]);
			}
			else
			{
				if (j_pos[0] < j_pos[1] || sj_pos[1] == "inf")
				{
					for (unsigned int n = 1; n < nDim; n++)
						vCol.push_back(j_pos[0] + n);
				}
				else if (j_pos[0] < j_pos[1])
				{
					for (unsigned int n = 1; n < nDim; n++)
						vCol.push_back(j_pos[0] - n);
				}
			}
		}
		else
		{
			for (int n = 1; n <= j; n++)
				vCol.push_back(j_pos[n]);
		}
	}


	if (isnan(dMin))
	{
		if (!vLine.size())
			dMin = _data.min(sDataTable, i_pos[0], i_pos[1] - 1, j_pos[0]);
		else
			dMin = _data.min(sDataTable, vLine, vector<long long int>(1, vCol[0]));
	}
	if (isnan(dMax))
	{
		if (!vLine.size())
			dMax = _data.max(sDataTable, i_pos[0], i_pos[1] - 1, j_pos[0]);
		else
			dMax = _data.max(sDataTable, vLine, vector<long long int>(1, vCol[0]));
	}
	if (dMax < dMin)
	{
		double dTemp = dMax;
		dMax = dMin;
		dMin = dTemp;
	}

	if (nFitVars & 2)
	{
		if (isnan(dMinY))
		{
			if (!vLine.size())
			{
				if (j == 1 && j_pos[1] > j_pos[0])
					dMinY = _data.min(sDataTable, i_pos[0], i_pos[1] - 1, j_pos[0] + 1);
				else if (j == 1)
					dMinY = _data.min(sDataTable, i_pos[0], i_pos[1] - 1, j_pos[0] - 1);
				else
					dMinY = _data.min(sDataTable, i_pos[0], i_pos[1] - 1, j_pos[1]);
			}
			else
			{
				dMinY = _data.min(sDataTable, vLine, vector<long long int>(1, vCol[1]));
			}
		}
		if (isnan(dMaxY))
		{
			if (!vLine.size())
			{
				if (j == 1 && j_pos[1] > j_pos[0])
					dMaxY = _data.max(sDataTable, i_pos[0], i_pos[1] - 1, j_pos[0] + 1);
				else if (j == 1)
					dMaxY = _data.max(sDataTable, i_pos[0], i_pos[1] - 1, j_pos[1] - 1);
				else
					dMaxY = _data.max(sDataTable, i_pos[0], i_pos[1] - 1, j_pos[1]);
			}
			else
				dMaxY = _data.max(sDataTable, vLine, vector<long long int>(1, vCol[1]));
		}
		if (dMaxY < dMinY)
		{
			double dTemp = dMaxY;
			dMaxY = dMinY;
			dMinY = dTemp;
		}
	}

	if (nDim == 2)
	{

		if (!vLine.size())
		{
			for (int i = i_pos[0]; i < i_pos[1]; i++)
			{

				if (!j)
				{
					if (_data.isValidEntry(i, j_pos[0], sDataTable))
					{
						vx.push_back(i + 1);
						vy.push_back(_data.getElement(i, j_pos[0], sDataTable));
					}
				}
				else
				{
					if (_data.isValidEntry(i, j_pos[0], sDataTable) && _data.isValidEntry(i, j_pos[1], sDataTable) && sj_pos[1] != "inf")
					{
						if (!isnan(dMin) && !isnan(dMax) && (_data.getElement(i, j_pos[0], sDataTable) < dMin || _data.getElement(i, j_pos[0], sDataTable) > dMax))
						{
							continue;
						}
						vx.push_back(_data.getElement(i, j_pos[0], sDataTable));
						vy.push_back(_data.getElement(i, j_pos[1], sDataTable));
					}
					else if (_data.isValidEntry(i, j_pos[0], sDataTable) && _data.isValidEntry(i, j_pos[0] + 1, sDataTable) && sj_pos[1] == "inf")
					{
						if (!isnan(dMin) && !isnan(dMax) && (_data.getElement(i, j_pos[0], sDataTable) < dMin || _data.getElement(i, j_pos[0], sDataTable) > dMax))
						{
							continue;
						}
						vx.push_back(_data.getElement(i, j_pos[0], sDataTable));
						vy.push_back(_data.getElement(i, j_pos[0] + 1, sDataTable));
					}
				}
			}
		}
		else
		{
			for (unsigned int i = 0; i < vLine.size(); i++)
			{
				if (!j)
				{
					if (_data.isValidEntry(vLine[i], vCol[0], sDataTable))
					{
						vx.push_back(vLine[i] + 1);
						vy.push_back(_data.getElement(vLine[i], vCol[0], sDataTable));
					}
				}
				else
				{
					if (_data.isValidEntry(vLine[i], vCol[0], sDataTable) && _data.isValidEntry(vLine[i], vCol[1], sDataTable))
					{
						if (!isnan(dMin) && !isnan(dMax) && (_data.getElement(vLine[i], vCol[0], sDataTable) < dMin || _data.getElement(vLine[i], vCol[0], sDataTable) > dMax))
						{
							continue;
						}
						vx.push_back(_data.getElement(vLine[i], vCol[0], sDataTable));
						vy.push_back(_data.getElement(vLine[i], vCol[1], sDataTable));
					}
				}
			}
		}
		if (paramsMap.size() > vx.size())
			throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);
	}
	else if (nDim == 4)
	{
		if (!vLine.size())
		{
			int nErrorCols = 2;
			if (j == 1)
			{
				if (abs(j_pos[1] - j_pos[0]) == 2)
					nErrorCols = 1;
			}
			else if (j == 3)
				nErrorCols = 2;
			for (int i = i_pos[0]; i < i_pos[1]; i++)
			{
				if (j == 1)
				{
					if ((_data.isValidEntry(i, j_pos[0], sDataTable) && _data.isValidEntry(i, j_pos[0] + 1, sDataTable) && j_pos[0] < j_pos[1])
							|| (_data.isValidEntry(i, j_pos[1], sDataTable) && _data.isValidEntry(i, j_pos[1] - 1, sDataTable) && j_pos[1] < j_pos[0]))
					{
						if (!isnan(dMin) && !isnan(dMax) && (_data.getElement(i, j_pos[0], sDataTable) < dMin || _data.getElement(i, j_pos[0], sDataTable) > dMax))
						{
							continue;
						}
						if (j_pos[0] < j_pos[1])
						{
							vx.push_back(_data.getElement(i, j_pos[0], sDataTable));
							vy.push_back(_data.getElement(i, j_pos[0] + 1, sDataTable));
							if (nErrorCols == 1)
							{
								if (_data.isValidEntry(i, j_pos[0] + 2, sDataTable))
									vy_w.push_back(fabs(_data.getElement(i, j_pos[0] + 2, sDataTable)));
								else
									vy_w.push_back(0.0);
							}
							else
							{
								if (_data.isValidEntry(i, j_pos[0] + 2, sDataTable) && _data.isValidEntry(i, j_pos[0] + 3, sDataTable) && (_data.getElement(i, j_pos[0] + 2, sDataTable) && _data.getElement(i, j_pos[0] + 3, sDataTable)))
									vy_w.push_back(sqrt(fabs(_data.getElement(i, j_pos[0] + 2, sDataTable)) * fabs(_data.getElement(i, j_pos[0] + 3, sDataTable))));
								else if (_data.isValidEntry(i, j_pos[0] + 2, sDataTable) && _data.getElement(i, j_pos[0] + 2, sDataTable))
									vy_w.push_back(fabs(_data.getElement(i, j_pos[0] + 2, sDataTable)));
								else if (_data.isValidEntry(i, j_pos[0] + 3, sDataTable) && _data.getElement(i, j_pos[0] + 3, sDataTable))
									vy_w.push_back(fabs(_data.getElement(i, j_pos[0] + 3, sDataTable)));
								else
									vy_w.push_back(0.0);
							}
						}
						else
						{
							vx.push_back(_data.getElement(i, j_pos[0], sDataTable));
							vy.push_back(_data.getElement(i, j_pos[0] - 1, sDataTable));
							if (nErrorCols == 1)
							{
								if (_data.isValidEntry(i, j_pos[0] - 2, sDataTable))
									vy_w.push_back(fabs(_data.getElement(i, j_pos[0] - 2, sDataTable)));
								else
									vy_w.push_back(0.0);
							}
							else
							{
								if (_data.isValidEntry(i, j_pos[0] - 2, sDataTable) && _data.isValidEntry(i, j_pos[0] - 3, sDataTable) && (_data.getElement(i, j_pos[0] - 2, sDataTable) && _data.getElement(i, j_pos[0] - 3, sDataTable)))
									vy_w.push_back(sqrt(fabs(_data.getElement(i, j_pos[0] - 2, sDataTable)) * fabs(_data.getElement(i, j_pos[0] - 3, sDataTable))));
								else if (_data.isValidEntry(i, j_pos[0] - 2, sDataTable) && _data.getElement(i, j_pos[0] - 2, sDataTable))
									vy_w.push_back(fabs(_data.getElement(i, j_pos[0] - 2, sDataTable)));
								else if (_data.isValidEntry(i, j_pos[0] - 3, sDataTable) && _data.getElement(i, j_pos[0] - 3, sDataTable))
									vy_w.push_back(fabs(_data.getElement(i, j_pos[0] - 3, sDataTable)));
								else
									vy_w.push_back(0.0);
							}
						}
					}
				}
				else
				{
					if (_data.isValidEntry(i, j_pos[0], sDataTable) && _data.isValidEntry(i, j_pos[1], sDataTable))
					{
						if (!isnan(dMin) && !isnan(dMax) && (_data.getElement(i, j_pos[0], sDataTable) < dMin || _data.getElement(i, j_pos[0], sDataTable) > dMax))
						{
							continue;
						}
						vx.push_back(_data.getElement(i, j_pos[0], sDataTable));
						vy.push_back(_data.getElement(i, j_pos[1], sDataTable));
						if (_data.isValidEntry(i, j_pos[2], sDataTable) && _data.isValidEntry(i, j_pos[3], sDataTable) && (_data.getElement(i, j_pos[2], sDataTable) || _data.getElement(i, j_pos[3], sDataTable)))
							vy_w.push_back(sqrt(fabs(_data.getElement(i, j_pos[2], sDataTable)) * fabs(_data.getElement(i, j_pos[3], sDataTable))));
						if (_data.isValidEntry(i, j_pos[2], sDataTable) && _data.getElement(i, j_pos[2], sDataTable))
							vy_w.push_back(fabs(_data.getElement(i, j_pos[2], sDataTable)));
						if (_data.isValidEntry(i, j_pos[3], sDataTable) && _data.getElement(i, j_pos[3], sDataTable))
							vy_w.push_back(fabs(_data.getElement(i, j_pos[3], sDataTable)));
						else
							vy_w.push_back(0.0);
					}
				}
			}
			if (paramsMap.size() > vx.size())
				throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);
		}
		else
		{
			int nErrorCols = 2;
			if (j == 1)
			{
				if (abs(vCol[1] - vCol[0]) == 2)
					nErrorCols = 1;
			}
			else if (j == 3)
				nErrorCols = 2;
			for (unsigned int i = 0; i < vLine.size(); i++)
			{
				if (j == 1)
				{
					if (_data.isValidEntry(vLine[i], vCol[0], sDataTable) && _data.isValidEntry(vLine[i], vCol[1], sDataTable))
					{
						if (!isnan(dMin) && !isnan(dMax) && (_data.getElement(vLine[i], vCol[0], sDataTable) < dMin || _data.getElement(vLine[i], vCol[0], sDataTable) > dMax))
						{
							continue;
						}

						vx.push_back(_data.getElement(vLine[i], vCol[0], sDataTable));
						vy.push_back(_data.getElement(vLine[i], vCol[1], sDataTable));
						if (nErrorCols == 1)
						{
							if (_data.isValidEntry(vLine[i], vCol[2], sDataTable))
								vy_w.push_back(fabs(_data.getElement(vLine[i], vCol[2], sDataTable)));
							else
								vy_w.push_back(0.0);
						}
						else
						{
							if (_data.isValidEntry(vLine[i], vCol[2], sDataTable) && _data.isValidEntry(vLine[i], vCol[3], sDataTable) && (_data.getElement(vLine[i], vCol[2], sDataTable) && _data.getElement(vLine[i], vCol[3], sDataTable)))
								vy_w.push_back(sqrt(fabs(_data.getElement(vLine[i], vCol[2], sDataTable)) * fabs(_data.getElement(vLine[i], vCol[3], sDataTable))));
							else if (_data.isValidEntry(vLine[i], vCol[2], sDataTable) && _data.getElement(vLine[i], vCol[2], sDataTable))
								vy_w.push_back(fabs(_data.getElement(vLine[i], vCol[2], sDataTable)));
							else if (_data.isValidEntry(vLine[i], vCol[3], sDataTable) && _data.getElement(vLine[i], vCol[3], sDataTable))
								vy_w.push_back(fabs(_data.getElement(vLine[i], vCol[3], sDataTable)));
							else
								vy_w.push_back(0.0);
						}
					}
				}
				else
				{
					if (_data.isValidEntry(vLine[i], vCol[0], sDataTable) && _data.isValidEntry(vLine[i], vCol[1], sDataTable))
					{
						if (!isnan(dMin) && !isnan(dMax) && (_data.getElement(vLine[i], vCol[0], sDataTable) < dMin || _data.getElement(vLine[i], vCol[0], sDataTable) > dMax))
						{
							continue;
						}
						vx.push_back(_data.getElement(vLine[i], vCol[0], sDataTable));
						vy.push_back(_data.getElement(vLine[i], vCol[1], sDataTable));
						if (_data.isValidEntry(vLine[i], vCol[2], sDataTable) && _data.isValidEntry(vLine[i], vCol[3], sDataTable) && (_data.getElement(vLine[i], vCol[2], sDataTable) && _data.getElement(vLine[i], vCol[3], sDataTable)))
							vy_w.push_back(sqrt(fabs(_data.getElement(vLine[i], vCol[2], sDataTable)) * fabs(_data.getElement(i, vCol[3], sDataTable))));
						else if (_data.isValidEntry(vLine[i], vCol[2], sDataTable) && _data.getElement(vLine[i], vCol[2], sDataTable))
							vy_w.push_back(fabs(_data.getElement(vLine[i], vCol[2], sDataTable)));
						else if (_data.isValidEntry(vLine[i], vCol[3], sDataTable) && _data.getElement(vLine[i], vCol[3], sDataTable))
							vy_w.push_back(fabs(_data.getElement(vLine[i], vCol[3], sDataTable)));
						else
							vy_w.push_back(0.0);
					}
				}
			}
			if (paramsMap.size() > vx.size())
				throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);
		}

	}
	else if ((nFitVars & 2))
	{
		if (!vLine.size())
		{
			for (long long int i = i_pos[0]; i < i_pos[1]; i++)
			{
				if (j == 1 && j_pos[1] > j_pos[0])
				{
					if (!_data.isValidEntry(i, j_pos[0] + 1, sDataTable) || _data.getElement(i, j_pos[0] + 1, sDataTable) < dMinY || _data.getElement(i, j_pos[0] + 1, sDataTable) > dMaxY)
					{
					}
					else
						vy.push_back(_data.getElement(i, j_pos[0] + 1, sDataTable));
				}
				else if (j == 1)
				{
					if (!_data.isValidEntry(i, j_pos[0] - 1, sDataTable) || _data.getElement(i, j_pos[0] - 1, sDataTable) < dMinY || _data.getElement(i, j_pos[0] - 1, sDataTable) > dMaxY)
					{
					}
					else
						vy.push_back(_data.getElement(i, j_pos[0] - 1, sDataTable));
				}
				else
				{
					if (!_data.isValidEntry(i, j_pos[1], sDataTable) || _data.getElement(i, j_pos[1], sDataTable) < dMinY || _data.getElement(i, j_pos[1], sDataTable) > dMaxY)
					{
					}
					else
						vy.push_back(_data.getElement(i, j_pos[1], sDataTable));
				}
				if (!_data.isValidEntry(i, j_pos[0], sDataTable) || _data.getElement(i, j_pos[0], sDataTable) < dMin || _data.getElement(i, j_pos[0], sDataTable) > dMax)
				{
					continue;
				}
				else
					vx.push_back(_data.getElement(i, j_pos[0], sDataTable));

				if (j == 1 && j_pos[1] > j_pos[0])
				{
					for (long long int k = j_pos[0] + 2; k < j_pos[0] + i_pos[1] - i_pos[0] + 2; k++)
					{
						if (!_data.isValidEntry(k - j_pos[0] - 2 + i_pos[0], j_pos[0] + 1, sDataTable) || _data.getElement(k - j_pos[0] - 2 + i_pos[0], j_pos[0] + 1, sDataTable) < dMinY || _data.getElement(k - j_pos[0] - 2 + i_pos[0], j_pos[0] + 1, sDataTable) > dMaxY)
						{
							continue;
						}
						else
						{
							vTempZ.push_back(_data.getElement(i, k, sDataTable));
							if (bUseErrors && _data.isValidEntry(i, k + i_pos[1] - i_pos[0], sDataTable))
								vy_w.push_back(_data.getElement(i, k + i_pos[1] - i_pos[0], sDataTable));
							else if (bUseErrors)
								vy_w.push_back(0.0);
						}
					}
				}
				else if (j == 1)
				{
					for (long long int k = j_pos[0] - 2; k > j_pos[0] - i_pos[1] + i_pos[0] - 2; k--)
					{
						if (k < 0)
							break;
						if (!_data.isValidEntry(i_pos[0] - (k - j_pos[0] + 2), j_pos[0] - 1, sDataTable) || _data.getElement(i_pos[0] - (k - j_pos[0] + 2), j_pos[0] - 1, sDataTable) < dMinY || _data.getElement(i_pos[0] - (k - j_pos[0] + 2), j_pos[0] - 1, sDataTable) > dMaxY)
						{
							continue;
						}
						else
						{
							vTempZ.push_back(_data.getElement(i, k, sDataTable));
							if (bUseErrors && k - i_pos[1] + i_pos[0] >= 0 && _data.isValidEntry(i, k - i_pos[1] + i_pos[0], sDataTable))
								vy_w.push_back(_data.getElement(i, k - i_pos[1] + i_pos[0], sDataTable));
							else if (bUseErrors)
								vy_w.push_back(0.0);
						}
					}
				}
				else
				{
					for (long long int k = j_pos[2]; k < j_pos[2] + i_pos[1] - i_pos[0]; k++)
					{
						if (j > 2 && k == j_pos[3])
							break;
						if (!_data.isValidEntry(k - j_pos[2] + i_pos[0], j_pos[1], sDataTable) || _data.getElement(k - j_pos[2] + i_pos[0], j_pos[1], sDataTable) < dMinY || _data.getElement(k - j_pos[2] + i_pos[0], j_pos[1], sDataTable) > dMaxY)
						{
							continue;
						}
						else
						{
							vTempZ.push_back(_data.getElement(i, k, sDataTable));
							if (bUseErrors && j > 2 && _data.isValidEntry(i, k + j_pos[3], sDataTable))
								vy_w.push_back(_data.getElement(i, k + j_pos[3], sDataTable));
							else if (bUseErrors && _data.isValidEntry(i, k + i_pos[1] - i_pos[0], sDataTable))
								vy_w.push_back(_data.getElement(i, k + i_pos[1] - i_pos[0], sDataTable));
							else if (bUseErrors)
								vy_w.push_back(0.0);
						}
					}
				}
				vz.push_back(vTempZ);
				vTempZ.clear();
				if (vy_w.size() && bUseErrors)
				{
					vz_w.push_back(vy_w);
					vy_w.clear();
				}

			}
			if (paramsMap.size() > vz.size()
					|| paramsMap.size() > vz[0].size())
				throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);
		}
		else
		{
			for (long long int i = 0; i < vLine.size(); i++)
			{
				if (!_data.isValidEntry(vLine[i], vCol[1], sDataTable) || _data.getElement(vLine[i], vCol[1], sDataTable) < dMinY || _data.getElement(vLine[i], vCol[1], sDataTable) > dMaxY)
				{
				}
				else
					vy.push_back(_data.getElement(vLine[i], vCol[1], sDataTable));

				if (!_data.isValidEntry(vLine[i], vCol[0], sDataTable) || _data.getElement(vLine[i], vCol[0], sDataTable) < dMin || _data.getElement(vLine[i], vCol[0], sDataTable) > dMax)
				{
					continue;
				}
				else
					vx.push_back(_data.getElement(vLine[i], vCol[0], sDataTable));

				if (j == 1 && j_pos[1] > j_pos[0])
				{
					for (long long int k = j_pos[0] + 2; k < j_pos[0] + i_pos[1] - i_pos[0] + 2; k++)
					{
						if (!_data.isValidEntry(k - j_pos[0] - 2 + i_pos[0], j_pos[0] + 1, sDataTable) || _data.getElement(k - j_pos[0] - 2 + i_pos[0], j_pos[0] + 1, sDataTable) < dMinY || _data.getElement(k - j_pos[0] - 2 + i_pos[0], j_pos[0] + 1, sDataTable) > dMaxY)
						{
							continue;
						}
						else
						{
							vTempZ.push_back(_data.getElement(i, k, sDataTable));
							if (bUseErrors && _data.isValidEntry(i, k + i_pos[1] - i_pos[0], sDataTable))
								vy_w.push_back(_data.getElement(i, k + i_pos[1] - i_pos[0], sDataTable));
							else if (bUseErrors)
								vy_w.push_back(0.0);
						}
					}
				}
				else if (j == 1)
				{
					for (long long int k = j_pos[0] - 2; k > j_pos[0] - i_pos[1] + i_pos[0] - 2; k--)
					{
						if (k < 0)
							break;
						if (!_data.isValidEntry(i_pos[0] - (k - j_pos[0] + 2), j_pos[0] - 1, sDataTable) || _data.getElement(i_pos[0] - (k - j_pos[0] + 2), j_pos[0] - 1, sDataTable) < dMinY || _data.getElement(i_pos[0] - (k - j_pos[0] + 2), j_pos[0] - 1, sDataTable) > dMaxY)
						{
							continue;
						}
						else
						{
							vTempZ.push_back(_data.getElement(i, k, sDataTable));
							if (bUseErrors && k - i_pos[1] + i_pos[0] >= 0 && _data.isValidEntry(i, k - i_pos[1] + i_pos[0], sDataTable))
								vy_w.push_back(_data.getElement(i, k - i_pos[1] + i_pos[0], sDataTable));
							else if (bUseErrors)
								vy_w.push_back(0.0);
						}
					}
				}
				else
				{
					for (long long int k = vCol[2]; k < vCol.size(); k++)
					{
						if (j > 2 && k == vLine.size() + 2)
							break;
						if (!_data.isValidEntry(vLine[k], vCol[1], sDataTable)
								|| _data.getElement(vLine[k], vCol[1], sDataTable) < dMinY
								|| _data.getElement(vLine[k], vCol[1], sDataTable) > dMaxY)
						{
							continue;
						}
						else
						{
							vTempZ.push_back(_data.getElement(vLine[i], vCol[k], sDataTable));
						}
					}
				}
				vz.push_back(vTempZ);
				vTempZ.clear();
				if (vy_w.size() && bUseErrors)
				{
					vz_w.push_back(vy_w);
					vy_w.clear();
				}
			}
			if (paramsMap.size() > vz.size()
					|| paramsMap.size() > vz[0].size())
				throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);
		}
	}
	else
	{
		if (!vLine.size())
		{
			for (int i = i_pos[0]; i < i_pos[1]; i++)
			{
				if (_data.isValidEntry(i, j_pos[0], sDataTable) && _data.isValidEntry(i, j_pos[1], sDataTable))
				{
					if (!isnan(dMin) && !isnan(dMax) && (_data.getElement(i, j_pos[0], sDataTable) < dMin || _data.getElement(i, j_pos[0], sDataTable) > dMax))
					{
						continue;
					}
					vx.push_back(_data.getElement(i, j_pos[0], sDataTable));
					vy.push_back(_data.getElement(i, j_pos[1], sDataTable));

					if (_data.isValidEntry(i, j_pos[2], sDataTable))
						vy_w.push_back(fabs(_data.getElement(i, j_pos[2], sDataTable)));
					else
						vy_w.push_back(0.0);
				}
			}
			if (paramsMap.size() > vy.size())
				throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);
		}
		else
		{
			for (unsigned int i = 0; i < vLine.size(); i++)
			{
				if (_data.isValidEntry(vLine[i], vCol[0], sDataTable) && _data.isValidEntry(vLine[i], vCol[1], sDataTable))
				{
					if (!isnan(dMin) && !isnan(dMax) && (_data.getElement(vLine[i], vCol[0], sDataTable) < dMin || _data.getElement(vLine[i], vCol[0], sDataTable) > dMax))
					{
						continue;
					}
					vx.push_back(_data.getElement(vLine[i], vCol[0], sDataTable));
					vy.push_back(_data.getElement(vLine[i], vCol[1], sDataTable));
					if (_data.isValidEntry(vLine[i], vCol[2], sDataTable))
						vy_w.push_back(fabs(_data.getElement(vLine[i], vCol[2], sDataTable)));
					else
						vy_w.push_back(0.0);
				}
			}
			if (paramsMap.size() > vy.size())
				throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);
		}
	}

	if (paramsMap.size() > vx.size())
		throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);

	// Überzählige Klammern (durch Fit(x)) entfernen
	while (sFuncDisplay.front() == '(')
	{
		if (getMatchingParenthesis(sFuncDisplay) == sFuncDisplay.length() - 1 && getMatchingParenthesis(sFuncDisplay) != string::npos)
		{
			sFuncDisplay.erase(0, 1);
			sFuncDisplay.pop_back();
			StripSpaces(sFuncDisplay);
		}
		else
			break;
	}
	StripSpaces(sFitFunction);
	while (sFitFunction.front() == '(')
	{
		if (getMatchingParenthesis(sFitFunction) == sFitFunction.length() - 1 && getMatchingParenthesis(sFitFunction) != string::npos)
		{
			sFitFunction.erase(0, 1);
			sFitFunction.pop_back();
			StripSpaces(sFitFunction);
		}
		else
			break;
	}

	if (_option.getSystemPrintStatus())
		NumeReKernel::printPreFmt(LineBreak("|-> " + _lang.get("PARSERFUNCS_FIT_FITTING", sFuncDisplay) + " ", _option, 0));

	for (auto iter = paramsMap.begin(); iter != paramsMap.end(); ++iter)
	{
		vInitialVals.push_back(*(iter->second));
	}
	if (sChiMap.length())
	{
		Fitcontroller _fControl(&_parser);

		if (!_idx.vI.size())
		{
			for (long long int i = _idx.nI[0]; i < _idx.nI[1]; i++)
			{
				for (long long int j = _idx.nI[0]; j <= (_idx.nI[1] - 1) * (!b1DChiMap) + _idx.nI[0] * (b1DChiMap); j++)
				{
					auto iter = paramsMap.begin();
					for (unsigned int n = 0; n < vInitialVals.size(); n++)
					{
						*(iter->second) = vInitialVals[n];
						++iter;
					}
					if (!_data.isValidEntry(i, _idx.nJ[0], sChiMap))
						continue;
					*(varMap.at(sChiMap_Vars[0])) = _data.getElement(i, _idx.nJ[0], sChiMap);
					if (!b1DChiMap && _idx.nJ[0] < _idx.nJ[1])
					{
						if (!_data.isValidEntry(i, _idx.nJ[0] + 1, sChiMap))
							continue;
						*(varMap.at(sChiMap_Vars[1])) = _data.getElement(j, _idx.nJ[0] + 1, sChiMap);
					}
					else if (!b1DChiMap)
					{
						if (!_data.isValidEntry(i, _idx.nJ[0] - 1, sChiMap))
							continue;
						*(varMap.at(sChiMap_Vars[1])) = _data.getElement(j, _idx.nJ[0] - 1, sChiMap);
					}
					if (nDim >= 2 && nFitVars == 1)
					{
						if (!bUseErrors)
						{
							if (!_fControl.fit(vx, vy, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
							{
								if (_option.getSystemPrintStatus())
									NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
								return false;
							}
							sFunctionDefString = "Fit(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
						}
						else
						{
							if (!_fControl.fit(vx, vy, vy_w, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
							{
								if (_option.getSystemPrintStatus())
									NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
								return false;
							}
							sFunctionDefString = "Fitw(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
						}
					}
					else if (nDim == 3)
					{
						if (!_fControl.fit(vx, vy, vz, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
						{
							if (_option.getSystemPrintStatus())
								NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
							return false;
						}
						sFunctionDefString = "Fit(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
					}
					else if (nDim == 5)
					{
						if (!_fControl.fit(vx, vy, vz, vz_w, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
						{
							if (_option.getSystemPrintStatus())
								NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
							return false;
						}
						sFunctionDefString = "Fitw(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
					}
					if (_idx.nJ[0] < _idx.nJ[1])
					{
						_data.writeToCache(i, _idx.nJ[0] + 1 + (!b1DChiMap) * (j - _idx.nI[0] + 1), sChiMap, _fControl.getFitChi());
						if (i == _idx.nI[0] && !b1DChiMap)
							_data.setHeadLineElement(_idx.nJ[0] + 1 + (!b1DChiMap) * (j - _idx.nI[0] + 1), sChiMap, "chi^2[" + toString(j - _idx.nI[0] + 1) + "]");
						else if (i == _idx.nI[0])
							_data.setHeadLineElement(_idx.nJ[0] + 1 + (!b1DChiMap) * (j - _idx.nI[0] + 1), sChiMap, "chi^2");
					}
					else
					{
						_data.writeToCache(i, _idx.nJ[0] - 1 - (!b1DChiMap) * (j - _idx.nI[0] + 1), sChiMap, _fControl.getFitChi());
						if (i == _idx.nI[0] && !b1DChiMap)
							_data.setHeadLineElement(_idx.nJ[0] - 1 - (!b1DChiMap) * (j - _idx.nI[0] + 1), sChiMap, "chi^2[" + toString(j - _idx.nI[0] + 1) + "]");
						else if (i == _idx.nI[0])
							_data.setHeadLineElement(_idx.nJ[0] - 1 - (!b1DChiMap) * (j - _idx.nI[0] + 1), sChiMap, "chi^2");
					}
				}
			}
		}
		else
		{
			for (long long int i = 0; i < _idx.vI.size(); i++)
			{
				for (long long int j = 0; j <= (_idx.vI.size() - 1) * (!b1DChiMap); j++)
				{
					auto iter = paramsMap.begin();
					for (unsigned int n = 0; n < vInitialVals.size(); n++)
					{
						*(iter->second) = vInitialVals[n];
						++iter;
					}
					if (!_data.isValidEntry(_idx.vI[i], _idx.vJ[0], sChiMap))
						continue;
					*(varMap.at(sChiMap_Vars[0])) = _data.getElement(_idx.vI[i], _idx.vJ[0], sChiMap);
					if (!b1DChiMap)
					{
						if (!_data.isValidEntry(_idx.vI[j], _idx.vJ[1], sChiMap))
							continue;
						*(varMap.at(sChiMap_Vars[1])) = _data.getElement(_idx.vI[j], _idx.vJ[1], sChiMap);
					}
					if (nDim >= 2 && nFitVars == 1)
					{
						if (!bUseErrors)
						{
							if (!_fControl.fit(vx, vy, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
							{
								if (_option.getSystemPrintStatus())
									NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
								return false;
							}
							sFunctionDefString = "Fit(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
						}
						else
						{
							if (!_fControl.fit(vx, vy, vy_w, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
							{
								if (_option.getSystemPrintStatus())
									NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
								return false;
							}
							sFunctionDefString = "Fitw(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
						}
					}
					else if (nDim == 3)
					{
						if (!_fControl.fit(vx, vy, vz, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
						{
							if (_option.getSystemPrintStatus())
								NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
							return false;
						}
						sFunctionDefString = "Fit(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
					}
					else if (nDim == 5)
					{
						if (!_fControl.fit(vx, vy, vz, vz_w, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
						{
							if (_option.getSystemPrintStatus())
								NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
							return false;
						}
						sFunctionDefString = "Fitw(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
					}
					_data.writeToCache(_idx.vI[i], _idx.vJ[1 + (!b1DChiMap) * (j + 1)], sChiMap, _fControl.getFitChi());
					if (!i && !b1DChiMap)
						_data.setHeadLineElement(_idx.vJ[1 + (!b1DChiMap) * (j + 1)], sChiMap, "chi^2[" + toString(j + 1) + "]");
					else if (!i)
						_data.setHeadLineElement(_idx.vJ[1 + (!b1DChiMap) * (j + 1)], sChiMap, "chi^2");

				}
			}
		}
		auto iter = paramsMap.begin();
		for (unsigned int n = 0; n < vInitialVals.size(); n++)
		{
			*(iter->second) = vInitialVals[n];
			++iter;
		}
		if (_option.getSystemPrintStatus())
		{
			NumeReKernel::printPreFmt(_lang.get("COMMON_SUCCESS") + ".\n");
			NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_CHIMAPLOCATION", sChiMap), _option));
		}

		bool bDefinitionSuccess = false;

		if (!_functions.isDefined(sFunctionDefString))
			bDefinitionSuccess = _functions.defineFunc(sFunctionDefString);
		else if (_functions.getDefine(_functions.getFunctionIndex(sFunctionDefString)) != sFunctionDefString)
			bDefinitionSuccess = _functions.defineFunc(sFunctionDefString, true);
        else if (_functions.getDefine(_functions.getFunctionIndex(sFunctionDefString)) == sFunctionDefString)
            return true;

        if (bDefinitionSuccess)
            NumeReKernel::print(_lang.get("DEFINE_SUCCESS"));
        else
            NumeReKernel::issueWarning(_lang.get("DEFINE_FAILURE"));

		return true;
	}

	Fitcontroller _fControl(&_parser);

	if (nDim >= 2 && nFitVars == 1)
	{
		if (!bUseErrors)
		{
			if (!_fControl.fit(vx, vy, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
			{
				if (_option.getSystemPrintStatus())
					NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
				return false;
			}
			sFunctionDefString = "Fit(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
		}
		else
		{
			if (!_fControl.fit(vx, vy, vy_w, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
			{
				if (_option.getSystemPrintStatus())
					NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
				return false;
			}
			sFunctionDefString = "Fitw(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
		}
	}
	else if (nDim == 3)
	{
		if (!_fControl.fit(vx, vy, vz, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
		{
			if (_option.getSystemPrintStatus())
				NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
			return false;
		}
		sFunctionDefString = "Fit(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
	}
	else if (nDim == 5)
	{
		if (!_fControl.fit(vx, vy, vz, vz_w, sFitFunction, sRestrictions, paramsMap, dPrecision, nMaxIterations))
		{
			if (_option.getSystemPrintStatus())
				NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");
			return false;
		}
		sFunctionDefString = "Fitw(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
	}

	vz_w = _fControl.getCovarianceMatrix();
	dChisq = _fControl.getFitChi();

	dNormChisq = dChisq;

	unsigned int nSize = ((vz.size()) ? (vz.size() * vz[0].size()) : vx.size());
	if (!bUseErrors && !(nFitVars & 2))
	{
		for (unsigned int i = 0; i < vz_w.size(); i++)
		{
			for (unsigned int j = 0; j < vz_w[0].size(); j++)
			{
				vz_w[i][j] *= dChisq / (nSize - paramsMap.size());
			}
		}
	}
	else if (!bUseErrors)
	{
		for (unsigned int i = 0; i < vz_w.size(); i++)
		{
			for (unsigned int j = 0; j < vz_w[0].size(); j++)
			{
				vz_w[i][j] *= dChisq / (nSize * nSize - paramsMap.size());
			}
		}
	}

	if (!bMaskDialog && _option.getSystemPrintStatus())
		reduceLogFilesize(sFitLog);
	sFittedFunction = _fControl.getFitFunction();
	oFitLog.open(sFitLog.c_str(), ios_base::ate | ios_base::app);
	if (oFitLog.fail())
	{
		oFitLog.close();
		_data.setCacheStatus(false);
		NumeReKernel::printPreFmt("\n");
		throw SyntaxError(SyntaxError::CANNOT_OPEN_FITLOG, sCmd, SyntaxError::invalid_position);
	}
	if (bTeXExport)
	{
		oTeXExport.open(sTeXExportFile.c_str(), ios_base::trunc);
		if (oTeXExport.fail())
		{
			oTeXExport.close();
			_data.setCacheStatus(false);
			NumeReKernel::printPreFmt("\n");
			throw SyntaxError(SyntaxError::CANNOT_OPEN_TARGET, sCmd, SyntaxError::invalid_position, sTeXExportFile);
		}
	}
	// FITLOG
	oFitLog << std::setw(76) << std::setfill('=') << '=' << endl;
	oFitLog << toUpperCase(_lang.get("PARSERFUNCS_FIT_HEADLINE")) << ": " << getTimeStamp(false) << endl;
	oFitLog << std::setw(76) << std::setfill('=') << '=' << endl;
	oFitLog << (_lang.get("PARSERFUNCS_FIT_FUNCTION", sFuncDisplay)) << endl;
	oFitLog << (_lang.get("PARSERFUNCS_FIT_FITTED_FUNC", sFittedFunction)) << endl;
	oFitLog << (_lang.get("PARSERFUNCS_FIT_DATASET")) << " ";
	if (nDim == 2)
	{
		oFitLog << j_pos[0] + 1;
		if (j)
		{
			oFitLog << ", " << j_pos[1] + 1;
		}
	}
	else if (nDim == 4)
	{
		int nErrorCols = 2;
		if (j == 1)
		{
			if (abs(j_pos[1] - j_pos[0]) == 3)
				nErrorCols = 1;
		}
		else if (j == 3)
			nErrorCols = 2;

		if (j == 1)
		{
			if (j_pos[0] < j_pos[1])
			{
				oFitLog << j_pos[0] + 1 << ", " << j_pos[0] + 2 << ", " << j_pos[0] + 3;
				if (nErrorCols == 2)
					oFitLog << ", " << j_pos[0] + 4;
			}
			else
			{
				oFitLog << j_pos[0] + 1 << ", " << j_pos[0] << ", " << j_pos[0] - 1;
				if (nErrorCols == 2)
					oFitLog << ", " << j_pos[0] - 2;
			}
		}
		else
		{
			oFitLog << j_pos[0] + 1 << ", " << j_pos[1] + 1 << ", " << j_pos[2] + 1 << ", " << j_pos[3] + 1;
		}
	}
	else if ((nFitVars & 2))
	{
		if (j == 1 && j_pos[1] > j_pos[0])
		{
			oFitLog << j_pos[0] + 1 << ", " << j_pos[0] + 2 << ", " << j_pos[0] + 3 << "-" << j_pos[0] + 2 + i_pos[1] - i_pos[0];
			if (bUseErrors)
				oFitLog << ", " << j_pos[2] + 3 + i_pos[1] - i_pos[0] << "-" << j_pos[0] + 2 + 2 * (i_pos[1] - i_pos[0]);
		}
		else if (j == 1)
		{
			oFitLog << j_pos[0] + 1 << ", " << j_pos[0] << ", " << j_pos[0] - 1 << "-" << j_pos[0] - 2 - i_pos[1] + i_pos[0];
			if (bUseErrors)
				oFitLog << ", " << j_pos[2] - 3 - i_pos[1] + i_pos[0] << "-" << j_pos[0] - 2 - 2 * (i_pos[1] - i_pos[0]);
		}
		else
		{
			oFitLog << j_pos[0] + 1 << ", " << j_pos[1] + 1 << ", " << j_pos[2] + 1 << "-" << j_pos[2] + i_pos[1] - i_pos[0];
			if (bUseErrors)
			{
				if (j > 2)
					oFitLog << ", " << j_pos[3] + 1 << "-" << j_pos[3] + (i_pos[1] - i_pos[0]);
				else
					oFitLog << ", " << j_pos[2] + i_pos[1] - i_pos[0] + 1 << "-" << j_pos[0] + 2 * (i_pos[1] - i_pos[0]);
			}
		}
	}
	else
	{
		for (int k = 0; k < (int)nDim; k++)
		{
			oFitLog << j_pos[k] + 1;
			if (k + 1 < (int)nDim)
				oFitLog << ", ";
		}
	}
	oFitLog << " " << _lang.get("PARSERFUNCS_FIT_FROM") << " " << _data.getDataFileName(sDataTable) << endl;
	if (bUseErrors)
		oFitLog << (_lang.get("PARSERFUNCS_FIT_POINTS_W_ERR", toString((int)nSize))) << endl;
	else
		oFitLog << (_lang.get("PARSERFUNCS_FIT_POINTS_WO_ERR", toString((int)nSize))) << endl;
	if (bRestrictXVals)
		oFitLog << (_lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "x", toString(dMin, 5), toString(dMax, 5))) << endl;
	if (bRestrictYVals)
		oFitLog << (_lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "y", toString(dMinY, 5), toString(dMaxY, 5))) << endl;
	if (sRestrictions.length())
		oFitLog << _lang.get("PARSERFUNCS_FIT_PARAM_RESTRICTS", sRestrictions) << endl;
	oFitLog << _lang.get("PARSERFUNCS_FIT_FREEDOMS", toString((int)nSize - paramsMap.size())) << endl;
	oFitLog << _lang.get("PARSERFUNCS_FIT_ALGORITHM_SETTINGS", toString(dPrecision, 5), toString(nMaxIterations)) << endl;
	oFitLog << _lang.get("PARSERFUNCS_FIT_ITERATIONS", toString(_fControl.getIterations())) << endl;
	if (nSize != paramsMap.size() && !(nFitVars & 2))
	{
		oFitLog << _lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7)) << endl;
		oFitLog << _lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7)) << endl;
		oFitLog << _lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7)) << endl;
	}
	else if (nFitVars & 2 && nSize != paramsMap.size() )
	{
		oFitLog << _lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7)) << endl;
		oFitLog << _lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7)) << endl;
		oFitLog << _lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7)) << endl;
	}
	oFitLog << endl;
	if (bUseErrors)
		oFitLog << _lang.get("PARSERFUNCS_FIT_LOG_TABLEHEAD1") << endl;
	else
		oFitLog << _lang.get("PARSERFUNCS_FIT_LOG_TABLEHEAD2") << endl;
	oFitLog << std::setw(76) << std::setfill('-') << '-' << endl;

	// TEXEXPORT
	if (bTeXExport)
	{
		oTeXExport << "%\n% " << _lang.get("OUTPUT_PRINTLEGAL_TEX") << "\n%" << endl;
		oTeXExport << "\\section{" << _lang.get("PARSERFUNCS_FIT_HEADLINE") << ": " << getTimeStamp(false)  << "}" << endl;
		oTeXExport << "\\begin{itemize}" << endl;
		oTeXExport << "\t\\item " << (_lang.get("PARSERFUNCS_FIT_FUNCTION", "$" + replaceToTeX(sFuncDisplay, true) + "$")) << endl;
		oTeXExport << "\t\\item " << (_lang.get("PARSERFUNCS_FIT_FITTED_FUNC", "$" + replaceToTeX(sFittedFunction, true) + "$")) << endl;
        if (bUseErrors)
			oTeXExport << "\t\\item " << (_lang.get("PARSERFUNCS_FIT_POINTS_W_ERR", toString((int)nSize))) << endl;
		else
			oTeXExport << "\t\\item " << (_lang.get("PARSERFUNCS_FIT_POINTS_WO_ERR", toString((int)nSize))) << endl;
		if (bRestrictXVals)
			oTeXExport << "\t\\item " << (_lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "x", toString(dMin, 5), toString(dMax, 5))) << endl;
		if (bRestrictYVals)
			oTeXExport << "\t\\item " << (_lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "y", toString(dMinY, 5), toString(dMaxY, 5))) << endl;
		if (sRestrictions.length())
			oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_PARAM_RESTRICTS", "$" + replaceToTeX(sRestrictions, true) + "$") << endl;
		oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_FREEDOMS", toString((int)nSize - paramsMap.size())) << endl;
		oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_ALGORITHM_SETTINGS", toString(dPrecision, 5), toString(nMaxIterations)) << endl;
		oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_ITERATIONS", toString(_fControl.getIterations())) << endl;
		if (nSize != paramsMap.size() && !(nFitVars & 2))
		{
			string sChiReplace = _lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7));
			sChiReplace.replace(sChiReplace.find("chi^2"), 5, "$\\chi^2$");
			oTeXExport << "\t\\item " << sChiReplace << endl;
			sChiReplace = _lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7));
			sChiReplace.replace(sChiReplace.find("chi^2"), 5, "$\\chi^2$");
			oTeXExport << "\t\\item " << sChiReplace << endl;
			oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7)) << endl;
		}
		else if (nFitVars & 2 && nSize != paramsMap.size())
		{
			string sChiReplace = _lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7));
			sChiReplace.replace(sChiReplace.find("chi^2"), 5, "$\\chi^2$");
			oTeXExport << "\t\\item " << sChiReplace << endl;
			sChiReplace = _lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7));
			sChiReplace.replace(sChiReplace.find("chi^2"), 5, "$\\chi^2$");
			oTeXExport << "\t\\item " << sChiReplace << endl;
			oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7)) << endl;
		}
		oTeXExport << "\\end{itemize}" << endl << "\\begin{table}[htb]" << endl << "\t\\centering\n\t\\begin{tabular}{cccc}" << endl << "\t\t\\toprule" << endl;
		if (bUseErrors)
			oTeXExport << "\t\t" << _lang.get("PARSERFUNCS_FIT_PARAM") << " & "
					   << _lang.get("PARSERFUNCS_FIT_INITIAL") << " & "
					   << _lang.get("PARSERFUNCS_FIT_FITTED") << " & "
					   << _lang.get("PARSERFUNCS_FIT_PARAM_DEV") << "\\\\" << endl;
		else
			oTeXExport << "\t\t" << _lang.get("PARSERFUNCS_FIT_PARAM") << " & "
					   << _lang.get("PARSERFUNCS_FIT_INITIAL") << " & "
					   << _lang.get("PARSERFUNCS_FIT_FITTED") << " & "
					   << _lang.get("PARSERFUNCS_FIT_ASYMPTOTIC_ERROR") << "\\\\" << endl;
		oTeXExport << "\t\t\\midrule" << endl;
	}
	_data.setCacheStatus(false);


	if (_option.getSystemPrintStatus())
		NumeReKernel::printPreFmt(_lang.get("COMMON_SUCCESS") + ".\n");

	if (_option.getSystemPrintStatus() && !bMaskDialog)
	{
		NumeReKernel::toggleTableStatus();
		make_hline();
		NumeReKernel::print(toSystemCodePage("NUMERE: " + toUpperCase(_lang.get("PARSERFUNCS_FIT_HEADLINE"))));
		make_hline();
		NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_FUNCTION", sFittedFunction), _option, true));
		if (bUseErrors)
			NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_POINTS_W_ERR", toString((int)nSize))));
		else
			NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_POINTS_WO_ERR", toString((int)nSize))));
		if (bRestrictXVals)
			NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "x", toString(dMin, 5), toString(dMax, 5))));
		if (bRestrictYVals)
			NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "y", toString(dMinY, 5), toString(dMaxY, 5))));
		if (sRestrictions.length())
			NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_PARAM_RESTRICTS", sRestrictions)));
		NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_FREEDOMS", toString((int)nSize - paramsMap.size()))));
		NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_ALGORITHM_SETTINGS", toString(dPrecision, 5), toString(nMaxIterations))));
		NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_ITERATIONS", toString(_fControl.getIterations()))));
		if (nSize != paramsMap.size() && !(nFitVars & 2))
		{
			NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7))));
			NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7))));
			NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7))));
		}
		else if (nSize != paramsMap.size() && (nFitVars & 2))
		{
			NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7))));
			NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7))));
			NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7))));
		}
		NumeReKernel::printPreFmt("|\n");

		if (bUseErrors)
		{
			NumeReKernel::print(_lang.get("PARSERFUNCS_FIT_PARAM")
								+ strfill(_lang.get("PARSERFUNCS_FIT_INITIAL"), (_option.getWindow() - 32) / 2 + _option.getWindow() % 2 - 5 + 9 - _lang.get("PARSERFUNCS_FIT_PARAM").length())
								+ strfill(_lang.get("PARSERFUNCS_FIT_FITTED"), (_option.getWindow() - 50) / 2)
								+ strfill(_lang.get("PARSERFUNCS_FIT_PARAM_DEV"), 33));
		}
		else
		{
			NumeReKernel::print(_lang.get("PARSERFUNCS_FIT_PARAM")
								+ strfill(_lang.get("PARSERFUNCS_FIT_INITIAL"), (_option.getWindow() - 32) / 2 + _option.getWindow() % 2 - 5 + 9 - _lang.get("PARSERFUNCS_FIT_PARAM").length())
								+ strfill(_lang.get("PARSERFUNCS_FIT_FITTED"), (_option.getWindow() - 50) / 2)
								+ strfill(_lang.get("PARSERFUNCS_FIT_ASYMPTOTIC_ERROR"), 33));
		}
		NumeReKernel::printPreFmt("|   " + strfill("-", _option.getWindow() - 4, '-') + "\n");
	}
	pItem = paramsMap.begin();
	string sErrors = "";
	string sPMSign = " ";
	sPMSign[0] = (char)177;

	for (unsigned int n = 0; n < paramsMap.size(); n++)
	{
		if (pItem == paramsMap.end())
			break;
		oFitLog << pItem->first << "    ";
		oFitLog << std::setprecision(_option.getPrecision()) << std::setw(24 - pItem->first.length()) << std::setfill(' ') << vInitialVals[n];
		oFitLog << std::setprecision(_option.getPrecision()) << std::setw(15) << std::setfill(' ') << *(pItem->second);
		oFitLog << std::setprecision(_option.getPrecision()) << std::setw(16) << std::setfill(' ') << "± " + toString(sqrt(abs(vz_w[n][n])), 5);
		if (vz_w[n][n])
		{
			oFitLog << " " << std::setw(16) << std::setfill(' ') << "(" + toString(abs(sqrt(abs(vz_w[n][n] / (*(pItem->second)))) * 100.0), 4) + "%)" << endl;
			dErrorPercentageSum += abs(sqrt(abs(vz_w[n][n] / (*(pItem->second)))) * 100.0);
		}
		else
			oFitLog << endl;

		if (bTeXExport)
		{
			oTeXExport << "\t\t$" <<  replaceToTeX(pItem->first, true) << "$ & $"
					   << vInitialVals[n] << "$ & $"
					   << *(pItem->second) << "$ & $\\pm"
					   << sqrt(abs(vz_w[n][n]));
			if (vz_w[n][n])
			{
				oTeXExport << " \\quad (" + toString(abs(sqrt(abs(vz_w[n][n] / (*(pItem->second)))) * 100.0), 4) + "\\%)$\\\\" << endl;
			}
			else
				oTeXExport << "$\\\\" << endl;
		}
		if (_option.getSystemPrintStatus() && !bMaskDialog)
		{
			NumeReKernel::printPreFmt("|   " + pItem->first + "    "
									  + strfill(toString(vInitialVals[n], _option), (_option.getWindow() - 32) / 2 + _option.getWindow() % 2 - pItem->first.length())
									  + strfill(toString(*(pItem->second), _option), (_option.getWindow() - 50) / 2)
									  + strfill(sPMSign + " " + toString(sqrt(abs(vz_w[n][n])), 5), 16));
			if (vz_w[n][n])
				NumeReKernel::printPreFmt(" " + strfill("(" + toString(abs(sqrt(abs(vz_w[n][n] / (*(pItem->second)))) * 100.0), 4) + "%)", 16) + "\n");
			else
				NumeReKernel::printPreFmt("\n");
		}
		if (bSaveErrors)
		{
			sErrors += pItem->first + "_error = " + toCmdString(sqrt(abs(vz_w[n][n]))) + ",";
		}
		++pItem;
	}
	dErrorPercentageSum /= (double)paramsMap.size();
	if (bSaveErrors)
	{
		sErrors[sErrors.length() - 1] = ' ';
		_parser.SetExpr(sErrors);
		_parser.Eval();
	}
	_parser.SetExpr("chi = " + toCmdString(sqrt(dChisq)));
	_parser.Eval();
	oFitLog << std::setw(76) << std::setfill('-') << '-' << endl;
	if (bTeXExport)
	{
		oTeXExport << "\t\t\\bottomrule" << endl << "\t\\end{tabular}" << endl << "\\end{table}" << endl;
	}
	if (_option.getSystemPrintStatus() && !bMaskDialog)
		NumeReKernel::printPreFmt("|   " + strfill("-", _option.getWindow() - 4, '-') + "\n");
	if (paramsMap.size() > 1 && paramsMap.size() != nSize)
	{
		oFitLog << endl;
		oFitLog << _lang.get("PARSERFUNCS_FIT_CORRELMAT_HEAD") << ":" << endl;
		oFitLog << endl;
		for (unsigned int n = 0; n < paramsMap.size(); n++)
		{
			if (!n)
				oFitLog << '/';
			else if (n + 1 == paramsMap.size())
				oFitLog << '\\';
			else
				oFitLog << '|';
			for (unsigned int k = 0; k < paramsMap.size(); k++)
			{
				oFitLog << " " << std::setprecision(3) << std::setw(10) << std::setfill(' ') << vz_w[n][k] / sqrt(fabs(vz_w[n][n]*vz_w[k][k]));
			}
			if (!n)
				oFitLog << " \\";
			else if (n + 1 == paramsMap.size())
				oFitLog << " /";
			else
				oFitLog << " |";
			oFitLog << endl;
		}

		if (bTeXExport)
		{
			oTeXExport << endl << "\\subsection{" << _lang.get("PARSERFUNCS_FIT_CORRELMAT_HEAD") << "}" << endl;
			oTeXExport << "\\[" << endl << "\t\\begin{pmatrix}" << endl;
			for (unsigned int n = 0; n < paramsMap.size(); n++)
			{
				oTeXExport << "\t\t";
				for (unsigned int k = 0; k < paramsMap.size(); k++)
				{
					oTeXExport << vz_w[n][k] / sqrt(fabs(vz_w[n][n]*vz_w[k][k]));
					if (k + 1 < paramsMap.size())
						oTeXExport << " & ";
				}
				oTeXExport << "\\\\" << endl;
			}
			oTeXExport << "\t\\end{pmatrix}" << endl << "\\]" << endl;
		}

		if (_option.getSystemPrintStatus() && !bMaskDialog)
		{
			NumeReKernel::printPreFmt("|\n|-> " + toSystemCodePage(_lang.get("PARSERFUNCS_FIT_CORRELMAT_HEAD")) + ":\n|\n");
			for (unsigned int n = 0; n < paramsMap.size(); n++)
			{
				NumeReKernel::printPreFmt("|   ");
				if (!n)
					NumeReKernel::printPreFmt("/");
				else if (n + 1 == paramsMap.size())
					NumeReKernel::printPreFmt("\\");
				else
					NumeReKernel::printPreFmt("|");
				for (unsigned int k = 0; k < paramsMap.size(); k++)
				{
					NumeReKernel::printPreFmt(" " + strfill(toString(vz_w[n][k] / sqrt(fabs(vz_w[n][n] * vz_w[k][k])), 3), 10));
				}
				if (!n)
					NumeReKernel::printPreFmt(" \\\n");
				else if (n + 1 == paramsMap.size())
					NumeReKernel::printPreFmt(" /\n");
				else
					NumeReKernel::printPreFmt(" |\n");
			}
		}
	}
	if (nFitVars & 2)
		nSize *= nSize;
	dNormChisq /= (double)(nSize - paramsMap.size());
	if (nFitVars & 2)
		dNormChisq = sqrt(dNormChisq);
	// FITLOG
	oFitLog << endl;
	oFitLog << _lang.get("PARSERFUNCS_FIT_ANALYSIS") << ":" << endl;
	if (_fControl.getIterations() == nMaxIterations)
	{
		oFitLog << LineBreak(_lang.get("PARSERFUNCS_FIT_MAXITER_REACHED"), _option) << endl;
	}
	else
	{
		if (nSize != paramsMap.size())
		{
			if (bUseErrors)
			{
				if (log10(dNormChisq) > -1.0 && log10(dNormChisq) < 0.5 && dErrorPercentageSum < 50.0)
					oFitLog << _lang.get("PARSERFUNCS_FIT_GOOD_W_ERROR") << endl;
				else if (log10(dNormChisq) <= -1.0 && dErrorPercentageSum < 20.0)
					oFitLog << _lang.get("PARSERFUNCS_FIT_BETTER_W_ERROR") << endl;
				else if (log10(dNormChisq) >= 0.5 && log10(dNormChisq) < 1.5 && dErrorPercentageSum < 100.0)
					oFitLog << _lang.get("PARSERFUNCS_FIT_NOT_GOOD_W_ERROR") << endl;
				else
					oFitLog << _lang.get("PARSERFUNCS_FIT_BAD_W_ERROR") << endl;
			}
			else
			{
				if (log10(dNormChisq) < -3.0 && dErrorPercentageSum < 20.0)
					oFitLog << _lang.get("PARSERFUNCS_FIT_GOOD_WO_ERROR") << endl;
				else if (log10(dNormChisq) < 0.0 && dErrorPercentageSum < 50.0)
					oFitLog << _lang.get("PARSERFUNCS_FIT_IMPROVABLE_WO_ERROR") << endl;
				else if (log10(dNormChisq) >= 0.0 && log10(dNormChisq) < 0.5 && dErrorPercentageSum < 100.0)
					oFitLog << _lang.get("PARSERFUNCS_FIT_NOT_GOOD_WO_ERROR") << endl;
				else
					oFitLog << _lang.get("PARSERFUNCS_FIT_BAD_WO_ERROR") << endl;
			}
		}
		else
		{
			oFitLog << _lang.get("PARSERFUNCS_FIT_OVERFITTING") << endl;
		}
	}
	// TEXEXPORT
	oTeXExport << endl;
	oTeXExport << "\\subsection{" << _lang.get("PARSERFUNCS_FIT_ANALYSIS") << "}" << endl;
	if (_fControl.getIterations() == nMaxIterations)
	{
		oTeXExport << LineBreak(_lang.get("PARSERFUNCS_FIT_MAXITER_REACHED"), _option) << endl;
	}
	else
	{
		if (nSize != paramsMap.size())
		{
			if (bUseErrors)
			{
				if (log10(dNormChisq) > -1.0 && log10(dNormChisq) < 0.5 && dErrorPercentageSum < 50.0)
					oTeXExport << _lang.get("PARSERFUNCS_FIT_GOOD_W_ERROR") << endl;
				else if (log10(dNormChisq) <= -1.0 && dErrorPercentageSum < 20.0)
					oTeXExport << _lang.get("PARSERFUNCS_FIT_BETTER_W_ERROR") << endl;
				else if (log10(dNormChisq) >= 0.5 && log10(dNormChisq) < 1.5 && dErrorPercentageSum < 100.0)
					oTeXExport << _lang.get("PARSERFUNCS_FIT_NOT_GOOD_W_ERROR") << endl;
				else
					oTeXExport << _lang.get("PARSERFUNCS_FIT_BAD_W_ERROR") << endl;
			}
			else
			{
				if (log10(dNormChisq) < -3.0 && dErrorPercentageSum < 20.0)
					oTeXExport << _lang.get("PARSERFUNCS_FIT_GOOD_WO_ERROR") << endl;
				else if (log10(dNormChisq) < 0.0 && dErrorPercentageSum < 50.0)
					oTeXExport << _lang.get("PARSERFUNCS_FIT_IMPROVABLE_WO_ERROR") << endl;
				else if (log10(dNormChisq) >= 0.0 && log10(dNormChisq) < 0.5 && dErrorPercentageSum < 100.0)
					oTeXExport << _lang.get("PARSERFUNCS_FIT_NOT_GOOD_WO_ERROR") << endl;
				else
					oTeXExport << _lang.get("PARSERFUNCS_FIT_BAD_WO_ERROR") << endl;
			}
		}
		else
		{
			oTeXExport << _lang.get("PARSERFUNCS_FIT_OVERFITTING") << endl;
		}
	}

	if (_option.getSystemPrintStatus() && !bMaskDialog)
	{
		NumeReKernel::printPreFmt("|\n|-> " + _lang.get("PARSERFUNCS_FIT_ANALYSIS") + ":\n");
		if (_fControl.getIterations() == nMaxIterations)
		{
			NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_MAXITER_REACHED"), _option));
		}
		else
		{
			if (nSize != paramsMap.size())
			{
				if (bUseErrors)
				{
					if (log10(dNormChisq) > -1.0 && log10(dNormChisq) < 0.5 && dErrorPercentageSum < 50.0)
						NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_GOOD_W_ERROR"), _option));
					else if (log10(dNormChisq) <= -1.0 && dErrorPercentageSum < 20.0)
						NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_BETTER_W_ERROR"), _option));
					else if (log10(dNormChisq) >= 0.5 && log10(dNormChisq) < 1.5 && dErrorPercentageSum < 100.0)
						NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_NOT_GOOD_W_ERROR"), _option));
					else
						NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_BAD_W_ERROR"), _option));
				}
				else
				{
					if (log10(dNormChisq) < -3.0 && dErrorPercentageSum < 20.0)
						NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_GOOD_WO_ERROR"), _option));
					else if (log10(dNormChisq) < 0.0 && dErrorPercentageSum < 50.0)
						NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_IMPROVABLE_WO_ERROR"), _option));
					else if (log10(dNormChisq) >= 0.0 && log10(dNormChisq) < 0.5 && dErrorPercentageSum < 100.0)
						NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_NOT_GOOD_WO_ERROR"), _option));
					else
						NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_BAD_WO_ERROR"), _option));
				}
			}
			else
			{
				NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_OVERFITTING"), _option));
			}
		}
		NumeReKernel::toggleTableStatus();
		make_hline();
	}

	oFitLog.close();

	bool bDefinitionSuccess = false;

	if (!_functions.isDefined(sFunctionDefString))
		bDefinitionSuccess = _functions.defineFunc(sFunctionDefString);
	else if (_functions.getDefine(_functions.getFunctionIndex(sFunctionDefString)) != sFunctionDefString)
		bDefinitionSuccess = _functions.defineFunc(sFunctionDefString, true);
    else if (_functions.getDefine(_functions.getFunctionIndex(sFunctionDefString)) == sFunctionDefString)
        return true;

    if (bDefinitionSuccess)
        NumeReKernel::print(_lang.get("DEFINE_SUCCESS"));
    else
        NumeReKernel::issueWarning(_lang.get("DEFINE_FAILURE"));

	return true;
}

// fft data(:,:) -set inverse complex
bool parser_fft(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option)
{
	mglDataC _fftData;
	Indices _idx;

	double dNyquistFrequency = 1.0;
	double dTimeInterval = 0.0;
	double dPhaseOffset = 0.0;
	bool bInverseTrafo = false;
	bool bComplex = false;
	string sTargetTable = "fftdata";

	if (matchParams(sCmd, "inverse"))
		bInverseTrafo = true;
	if (matchParams(sCmd, "complex"))
		bComplex = true;

	// search for explicit "target" options and select the target cache
	sTargetTable = parser_evalTargetExpression(sCmd, sTargetTable, _idx, _parser, _data, _option);

	if (matchParams(sCmd, "inverse") || matchParams(sCmd, "complex"))
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
		{
			_fftData.a[i] = dual(_table.getValue(i, 1), 0.0);
		}
		else if (_table.getCols() == 3 && bComplex)
		{
			_fftData.a[i] = dual(_table.getValue(i, 1), _table.getValue(i, 2));
		}
		else if (_table.getCols() == 3 && !bComplex)
		{
			_fftData.a[i] = dual(_table.getValue(i, 1) * cos(_table.getValue(i, 2)), _table.getValue(i, 1) * sin(_table.getValue(i, 3)));
		}
	}


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


	if (_idx.nJ[1] == -2)
		_idx.nJ[1] = _idx.nJ[0] + 3;

	if (!bInverseTrafo)
	{
		if (_idx.nI[1] == -2)
			_idx.nI[1] = _idx.nI[0] + (int)round(_fftData.GetNx() / 2.0) + 1;

		for (long long int i = 0; i < (int)round(_fftData.GetNx() / 2.0) + 1; i++)
		{
			if (i > _idx.nI[1] - _idx.nI[0])
				break;

			_data.writeToCache(i + _idx.nI[0], _idx.nJ[0], sTargetTable, 2.0 * (double)(i)*dNyquistFrequency / (double)(_fftData.GetNx()));

			if (!bComplex)
			{
				_data.writeToCache(i + _idx.nI[0], _idx.nJ[0] + 1, sTargetTable, hypot(_fftData.a[i].real(), _fftData.a[i].imag()));

				if (i > 2 && (fabs(atan2(_fftData.a[i].imag(), _fftData.a[i].real()) - atan2(_fftData.a[i - 1].imag(), _fftData.a[i - 1].real())) >= M_PI)
						&& ((atan2(_fftData.a[i].imag(), _fftData.a[i].real()) - atan2(_fftData.a[i - 1].imag(), _fftData.a[i - 1].real())) * (atan2(_fftData.a[i - 1].imag(), _fftData.a[i - 1].real()) - atan2(_fftData.a[i - 2].imag(), _fftData.a[i - 2].real())) < 0))
				{
					if (atan2(_fftData.a[i - 1].imag(), _fftData.a[i - 1].real()) - atan2(_fftData.a[i - 2].imag(), _fftData.a[i - 2].real()) < 0.0)
						dPhaseOffset -= 2 * M_PI;
					else if (atan2(_fftData.a[i - 1].imag(), _fftData.a[i - 1].real()) - atan2(_fftData.a[i - 2].imag(), _fftData.a[i - 2].real()) > 0.0)
						dPhaseOffset += 2 * M_PI;
				}

				_data.writeToCache(i + _idx.nI[0], _idx.nJ[0] + 2, sTargetTable, atan2(_fftData.a[i].imag(), _fftData.a[i].real()) + dPhaseOffset);
			}
			else
			{
				_data.writeToCache(i, _idx.nJ[0] + 1, sTargetTable, _fftData.a[i].real());
				_data.writeToCache(i, _idx.nJ[0] + 2, sTargetTable, _fftData.a[i].imag());
			}
		}

		_data.setCacheStatus(true);
		_data.setHeadLineElement(_idx.nJ[0], sTargetTable, _lang.get("COMMON_FREQUENCY") + "_[Hz]");

		if (!bComplex)
		{
			_data.setHeadLineElement(_idx.nJ[0] + 1, sTargetTable, _lang.get("COMMON_AMPLITUDE"));
			_data.setHeadLineElement(_idx.nJ[0] + 2, sTargetTable, _lang.get("COMMON_PHASE") + "_[rad]");
		}
		else
		{
			_data.setHeadLineElement(_idx.nJ[0] + 1, sTargetTable, "Re(" + _lang.get("COMMON_AMPLITUDE") + ")");
			_data.setHeadLineElement(_idx.nJ[0] + 2, sTargetTable, "Im(" + _lang.get("COMMON_AMPLITUDE") + ")");
		}
	}
	else
	{
		if (_idx.nI[1] == -2)
			_idx.nI[1] = _idx.nI[0] + _fftData.GetNx();
		for (long long int i = 0; i < _fftData.GetNx(); i++)
		{
			if (i > _idx.nI[1] - _idx.nI[0])
				break;
			_data.writeToCache(i + _idx.nI[0], _idx.nJ[0], sTargetTable, (double)(i)*dTimeInterval / (double)(_fftData.GetNx() - 1));
			_data.writeToCache(i + _idx.nI[0], _idx.nJ[0] + 1, sTargetTable, _fftData.a[i].real());
			_data.writeToCache(i + _idx.nI[0], _idx.nJ[0] + 2, sTargetTable, _fftData.a[i].imag());
		}

		_data.setCacheStatus(true);
		_data.setHeadLineElement(_idx.nJ[0], sTargetTable, _lang.get("COMMON_TIME") + "_[s]");
		_data.setHeadLineElement(_idx.nJ[0] + 1, sTargetTable, "Re(" + _lang.get("COMMON_SIGNAL") + ")");
		_data.setHeadLineElement(_idx.nJ[0] + 2, sTargetTable, "Im(" + _lang.get("COMMON_SIGNAL") + ")");
	}
	if (_option.getSystemPrintStatus())
		NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_DONE")) + ".\n");

	_data.setCacheStatus(false);
	return true;
}

// fwt data(:,:) -set inverse type=cd k=1
bool parser_wavelet(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option)
{
	vector<double> vWaveletData;
	vector<double> vAxisData;
	Indices _idx;

	bool bInverseTrafo = false;
	bool bTargetGrid = false;
	string sTargetTable = "fwtdata";
	string sType = "d"; // d = daubechies, cd = centered daubechies, h = haar, ch = centered haar, b = bspline, cb = centered bspline
	int k = 4;

	if (matchParams(sCmd, "inverse"))
		bInverseTrafo = true;
	if (matchParams(sCmd, "grid"))
		bTargetGrid = true;
	if (matchParams(sCmd, "type", '='))
		sType = getArgAtPos(sCmd, matchParams(sCmd, "type", '=') + 4);
	if (matchParams(sCmd, "k", '='))
	{
		_parser.SetExpr(getArgAtPos(sCmd, matchParams(sCmd, "k", '=') + 1));
		k = (int)_parser.Eval();
	}


	// search for explicit "target" options and select the target cache
	sTargetTable = parser_evalTargetExpression(sCmd, sTargetTable, _idx, _parser, _data, _option);

	if (matchParams(sCmd, "inverse") || matchParams(sCmd, "type", '=') || matchParams(sCmd, "k", '='))
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

		if (_idx.nJ[1] == -2)
			_idx.nJ[1] = _idx.nJ[0] + tWaveletData.getCols() - 1;

		if (_idx.nI[1] == -2)
			_idx.nI[1] = _idx.nI[0] + tWaveletData.getLines() - 1;

		for (size_t i = 0; i < tWaveletData.getLines(); i++)
		{
			if (i + _idx.nI[0] > _idx.nI[1])
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
					_data.setHeadLineElement(j + _idx.nJ[0], sTargetTable, sHeadline);
				}
				if (j + _idx.nJ[0] > _idx.nJ[1])
					break;
				_data.writeToCache(i + _idx.nI[0], j + _idx.nJ[0], sTargetTable, tWaveletData.getValue(i, j));
			}
		}
		if (_option.getSystemPrintStatus())
			NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_DONE")) + ".\n");

		return true;
	}

	// write the output as usual data rows
	if (_idx.nJ[1] == -2)
		_idx.nJ[1] = _idx.nJ[0] + 2;

	if (_idx.nI[1] == -2)
		_idx.nI[1] = _idx.nI[0] + vWaveletData.size();
	for (long long int i = 0; i < vWaveletData.size(); i++)
	{
		if (i > _idx.nI[1] - _idx.nI[0])
			break;
		_data.writeToCache(i + _idx.nI[0], _idx.nJ[0], sTargetTable, (double)(i));
		_data.writeToCache(i, _idx.nJ[0] + 1, sTargetTable, vWaveletData[i]);
	}

	_data.setCacheStatus(true);
	if (!bInverseTrafo)
	{
		_data.setHeadLineElement(_idx.nJ[0], sTargetTable, _lang.get("COMMON_COEFFICIENT"));
		_data.setHeadLineElement(_idx.nJ[0] + 1, sTargetTable, _lang.get("COMMON_AMPLITUDE"));
	}
	else
	{
		_data.setHeadLineElement(_idx.nJ[0], sTargetTable, _lang.get("COMMON_TIME"));
		_data.setHeadLineElement(_idx.nJ[0] + 1, sTargetTable, _lang.get("COMMON_SIGNAL"));
	}
	if (_option.getSystemPrintStatus())
		NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_DONE")) + ".\n");

	_data.setCacheStatus(false);
	return true;
}

bool parser_evalPoints(string& sCmd, Datafile& _data, Parser& _parser, const Settings& _option, Define& _functions)
{
	unsigned int nSamples = 100;
	double dLeft = 0.0;
	double dRight = 0.0;
	double* dVar = 0;
	double dTemp = 0.0;
	string sExpr = "";
	string sParams = "";
	string sInterval = "";
	string sVar = "";
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

	if (isNotEmptyExpression(sExpr))
	{
		if (!_functions.call(sExpr))
			return false;
	}
	if (isNotEmptyExpression(sParams))
	{
		if (!_functions.call(sParams))
			return false;
	}
	StripSpaces(sParams);

	if (sExpr.find("data(") != string::npos || _data.containsCacheElements(sExpr))
	{

		getDataElements(sExpr, _parser, _data, _option);

		if (sExpr.find("{") != string::npos)
			parser_VectorToExpr(sExpr, _option);
	}

	if (sParams.find("data(") != string::npos || _data.containsCacheElements(sParams))
	{
		getDataElements(sParams, _parser, _data, _option);

		if (sParams.find("{") != string::npos && (containsStrings(sParams) || _data.containsStringVars(sParams)))
			parser_VectorToExpr(sParams, _option);
	}

	if (matchParams(sParams, "samples", '='))
	{
		sParams += " ";
		if (isNotEmptyExpression(getArgAtPos(sParams, matchParams(sParams, "samples", '=') + 7)))
		{
			_parser.SetExpr(getArgAtPos(sParams, matchParams(sParams, "samples", '=') + 7));
			nSamples = (unsigned int)_parser.Eval();
		}
		sParams.erase(matchParams(sParams, "samples", '=') - 1, 8);
	}
	if (matchParams(sParams, "logscale"))
	{
		bLogarithmic = true;
		sParams.erase(matchParams(sParams, "logscale") - 1, 8);
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

		if (isNotEmptyExpression(sExpr))
		{
			_parser.SetExpr(sExpr);
		}
		else
			_parser.SetExpr(sVar);
		_parser.Eval();
		dVar = parser_GetVarAdress(sVar, _parser);
		if (!dVar)
		{
			throw SyntaxError(SyntaxError::EVAL_VAR_NOT_FOUND, sCmd, sVar, sVar);
		}
		if (sInterval.find(':') == string::npos || sInterval.length() < 3)
			return false;
		if (isNotEmptyExpression(sInterval.substr(0, sInterval.find(':'))))
		{
			_parser.SetExpr(sInterval.substr(0, sInterval.find(':')));
			dLeft = _parser.Eval();
			if (isinf(dLeft) || isnan(dLeft))
			{
				sCmd = "nan";
				return false;
			}
		}
		else
			return false;
		if (isNotEmptyExpression(sInterval.substr(sInterval.find(':') + 1)))
		{
			_parser.SetExpr(sInterval.substr(sInterval.find(':') + 1));
			dRight = _parser.Eval();
			if (isinf(dRight) || isnan(dRight))
			{
				sCmd = "nan";
				return false;
			}
		}
		else
			return false;
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
		{
			vResults.push_back(_parser.Eval());
		}
	}
	sCmd = "_~evalpnts[~_~]";
	_parser.SetVectorVar("_~evalpnts[~_~]", vResults);

	return true;
}

// datagrid -x=x0:x1 y=y0:y1 z=func(x,y) samples=100
// datagrid -x=data(:,1) y=data(:,2) z=data(:,3)
// datagrid -x=data(2:,1) y=data(1,2:) z=data(2:,2:)
// datagrid EXPR -set [x0:x1, y0:y1] PARAMS
bool parser_datagrid(string& sCmd, string& sTargetCache, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
	unsigned int nSamples = 100;
	string sXVals = "";
	string sYVals = "";
	string sZVals = "";

	Indices _idx;

	bool bTranspose = false;

	vector<double> vXVals;
	vector<double> vYVals;
	vector<vector<double> > vZVals;


	if (sCmd.find("data(") != string::npos && !_data.isValid())
		throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sCmd, SyntaxError::invalid_position);
	if (_data.containsCacheElements(sCmd) && !_data.isValidCache())
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
			sXVals = "(" + sXVals + ")";
			try
			{
				parser_SplitArgs(sXVals, sYVals, ',', _option);
			}
			catch (...)
			{
				sXVals.pop_back();
				sXVals.erase(0, 1);
			}
			StripSpaces(sXVals);
			StripSpaces(sYVals);
		}
		if (sXVals == ":")
			sXVals = "-10:10";
		if (sYVals == ":")
			sYVals = "-10:10";
	}

	// Validate the intervals
	if ((!matchParams(sCmd, "x", '=') && !sXVals.length())
			|| (!matchParams(sCmd, "y", '=') && !sYVals.length())
			|| (!matchParams(sCmd, "z", '=') && !sZVals.length()))
	{
		throw SyntaxError(SyntaxError::TOO_FEW_ARGS, sCmd, SyntaxError::invalid_position, "datagrid");
	}

	// Get the number of samples from the option list
	if (matchParams(sCmd, "samples", '='))
	{
		_parser.SetExpr(getArgAtPos(sCmd, matchParams(sCmd, "samples", '=') + 7));
		nSamples = (unsigned int)_parser.Eval();
		if (nSamples < 2)
			throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, sCmd, SyntaxError::invalid_position);
		sCmd.erase(sCmd.find(getArgAtPos(sCmd, matchParams(sCmd, "samples", '=') + 7), matchParams(sCmd, "samples", '=') - 1), getArgAtPos(sCmd, matchParams(sCmd, "samples", '=') + 7).length());
		sCmd.erase(matchParams(sCmd, "samples", '=') - 1, 8);
	}

	// search for explicit "target" options and select the target cache
	sTargetCache = parser_evalTargetExpression(sCmd, sTargetCache, _idx, _parser, _data, _option);

	// read the transpose option
	if (matchParams(sCmd, "transpose"))
	{
		bTranspose = true;
		sCmd.erase(matchParams(sCmd, "transpose") - 1, 9);
	}

	// Read the interval definitions from the option list, if they are included
	// Remove them from the command expression
	if (!sXVals.length())
	{
		sXVals = getArgAtPos(sCmd, matchParams(sCmd, "x", '=') + 1);
		sCmd.erase(sCmd.find(getArgAtPos(sCmd, matchParams(sCmd, "x", '=') + 1), matchParams(sCmd, "x", '=') - 1), getArgAtPos(sCmd, matchParams(sCmd, "x", '=') + 1).length());
		sCmd.erase(matchParams(sCmd, "x", '=') - 1, 2);
	}
	if (!sYVals.length())
	{
		sYVals = getArgAtPos(sCmd, matchParams(sCmd, "y", '=') + 1);
		sCmd.erase(sCmd.find(getArgAtPos(sCmd, matchParams(sCmd, "y", '=') + 1), matchParams(sCmd, "y", '=') - 1), getArgAtPos(sCmd, matchParams(sCmd, "y", '=') + 1).length());
		sCmd.erase(matchParams(sCmd, "y", '=') - 1, 2);
	}
	if (!sZVals.length())
	{
		while (sCmd[sCmd.length() - 1] == ' ' || sCmd[sCmd.length() - 1] == '=' || sCmd[sCmd.length() - 1] == '-')
			sCmd.erase(sCmd.length() - 1);
		sZVals = getArgAtPos(sCmd, matchParams(sCmd, "z", '=') + 1);
	}

	// Try to call the functions
	if (!_functions.call(sZVals))
		throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, sZVals, sZVals);

	// Get the samples
	vector<size_t> vSamples = parser_getSamplesForDatagrid(sCmd, sZVals, nSamples, _parser, _data, _option);

	//>> X-Vector (Switch the samples depending on the "transpose" command line option)
	vXVals = parser_extractVectorForDatagrid(sCmd, sXVals, sZVals, vSamples[bTranspose], _parser, _data, _option);

	//>> Y-Vector (Switch the samples depending on the "transpose" command line option)
	vYVals = parser_extractVectorForDatagrid(sCmd, sYVals, sZVals, vSamples[1 - bTranspose], _parser, _data, _option);

	//>> Z-Matrix
	if (sZVals.find("data(") != string::npos || _data.containsCacheElements(sZVals))
	{
		// Get the datagrid from another table
		Indices _idx = parser_getIndices(sZVals, _parser, _data, _option);

		// identify the table
		string szDatatable = "data";
		if (_data.containsCacheElements(sZVals))
		{
			_data.setCacheStatus(true);
			for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
			{
				if (sZVals.find(iter->first + "(") != string::npos
						&& (!sZVals.find(iter->first + "(")
							|| (sZVals.find(iter->first + "(") && checkDelimiter(sZVals.substr(sZVals.find(iter->first + "(") - 1, (iter->first).length() + 2)))))
				{
					szDatatable = iter->first;
					break;
				}
			}
		}

		// Check the indices
		if ((_idx.nI[0] == -1 && !_idx.vI.size()) || (_idx.nJ[0] == -1 && !_idx.vJ.size()))
			throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

		// The indices are numbers
		if (!_idx.vI.size())
		{
			if (_idx.nI[1] == -1)
				_idx.nI[1] = _idx.nI[0];
			if (_idx.nJ[1] == -1)
				_idx.nJ[1] = _idx.nJ[0];
			if (_idx.nJ[1] == -2)
				_idx.nJ[1] = _data.getCols(szDatatable) - 1;

			parser_CheckIndices(_idx.nJ[0], _idx.nJ[1]);

			if (_idx.nI[1] == -2)
			{
				_idx.nI[1] = _data.getLines(szDatatable, true) - _data.getAppendedZeroes(_idx.nJ[0], szDatatable) - 1;
				for (long long int j = _idx.nJ[0] + 1; j <= _idx.nJ[1]; j++)
				{
					if (_data.getLines(szDatatable, true) - _data.getAppendedZeroes(j, szDatatable) - 1 > _idx.nI[1])
						_idx.nI[1] = _data.getLines(szDatatable, true) - _data.getAppendedZeroes(j, szDatatable) - 1;
				}
			}

			parser_CheckIndices(_idx.nI[0], _idx.nI[1]);

			// Get the data from the table. Choose the order of reading depending on the "transpose" command line option
			vector<double> vVector;
			if (!bTranspose)
			{
				for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
				{
					for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
					{
						vVector.push_back(_data.getElement(i, j, szDatatable));
					}
					vZVals.push_back(vVector);
					vVector.clear();
				}
			}
			else
			{
				for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
				{
					for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
					{
						vVector.push_back(_data.getElement(i, j, szDatatable));
					}
					vZVals.push_back(vVector);
					vVector.clear();
				}
			}

			// Check the content of the z matrix
			if (!vZVals.size() || (vZVals.size() == 1 && vZVals[0].size() == 1))
				throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, sCmd, SyntaxError::invalid_position);

			// Expand the z vector into a matrix for the datagrid if necessary
			parser_expandVectorToDatagrid(vXVals, vYVals, vZVals, vSamples[bTranspose], vSamples[1 - bTranspose]);
		}
		else
		{
			// the indices are vectors
			vector<double> vVector;

			// Get the data. Choose the order of reading depending on the "transpose" command line option
			if (!bTranspose)
			{
				for (size_t i = 0; i < _idx.vI.size(); i++)
				{
					vVector = _data.getElement(vector<long long int>(1, _idx.vI[i]), _idx.vJ, szDatatable);
					vZVals.push_back(vVector);
					vVector.clear();
				}
			}
			else
			{
				for (size_t j = 0; j < _idx.vJ.size(); j++)
				{
					vVector = _data.getElement(_idx.vI, vector<long long int>(1, _idx.vJ[j]), szDatatable);
					vZVals.push_back(vVector);
					vVector.clear();
				}
			}

			// Check the content of the z matrix
			if (!vZVals.size() || (vZVals.size() == 1 && vZVals[0].size() == 1))
				throw SyntaxError(SyntaxError::TOO_FEW_DATAPOINTS, sCmd, SyntaxError::invalid_position);

			// Expand the z vector into a matrix for the datagrid if necessary
			parser_expandVectorToDatagrid(vXVals, vYVals, vZVals, vSamples[bTranspose], vSamples[1 - bTranspose]);
		}
		_data.setCacheStatus(false);
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
	if (_idx.nI[1] == -2 || _idx.nI[1] == -1)
		_idx.nI[1] = _idx.nI[0] + vXVals.size();
	if (_idx.nJ[1] == -2 || _idx.nJ[1] == -1)
		_idx.nJ[1] = _idx.nJ[0] + vYVals.size() + 2;

	_data.setCacheStatus(true);

	// Write the x axis
	for (unsigned int i = 0; i < vXVals.size(); i++)
		_data.writeToCache(i, _idx.nJ[0], sTargetCache, vXVals[i]);
	_data.setHeadLineElement(_idx.nJ[0], sTargetCache, "x");

	// Write the y axis
	for (unsigned int i = 0; i < vYVals.size(); i++)
		_data.writeToCache(i, _idx.nJ[0] + 1, sTargetCache, vYVals[i]);
	_data.setHeadLineElement(_idx.nJ[0] + 1, sTargetCache, "y");

	// Write the z matrix
	for (unsigned int i = 0; i < vZVals.size(); i++)
	{
		if (i + _idx.nI[0] >= _idx.nI[1])
			break;
		for (unsigned int j = 0; j < vZVals[i].size(); j++)
		{
			if (j + 2 + _idx.nJ[0] >= _idx.nJ[1])
				break;
			_data.writeToCache(_idx.nI[0] + i, _idx.nJ[0] + 2 + j, sTargetCache, vZVals[i][j]);
			if (!i)
				_data.setHeadLineElement(_idx.nJ[0] + 2 + j, sTargetCache, "z[" + toString((int)j + 1) + "]");
		}
	}
	_data.setCacheStatus(false);

	return true;
}

// This function will obtain the samples of the datagrid for each spatial direction.
static vector<size_t> parser_getSamplesForDatagrid(const string& sCmd, const string& sZVals, size_t nSamples, Parser& _parser, Datafile& _data, const Settings& _option)
{
	vector<size_t> vSamples;
	// If the z vals are inside of a table then obtain the correct number of samples here
	if (sZVals.find("data(") != string::npos || _data.containsCacheElements(sZVals))
	{
		// Get the indices and identify the table name
		Indices _idx = parser_getIndices(sZVals, _parser, _data, _option);
		string sZDatatable = "data";
		if (_data.containsCacheElements(sZVals))
		{
			for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
			{
				if (sZVals.find(iter->first + "(") != string::npos
						&& (!sZVals.find(iter->first + "(")
							|| (sZVals.find(iter->first + "(") && checkDelimiter(sZVals.substr(sZVals.find(iter->first + "(") - 1, (iter->first).length() + 2)))))
				{
					sZDatatable = iter->first;
					break;
				}
			}
		}
		// Check the indices
		if ((_idx.nI[0] == -1 && !_idx.vI.size()) || (_idx.nJ[0] == -1 && !_idx.vJ.size()))
			throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
		if (!_idx.vI.size())
		{
			// The indices are numbers
			if (_idx.nI[1] == -1)
				_idx.nI[1] = _idx.nI[0];
			if (_idx.nJ[1] == -1)
				_idx.nJ[1] = _idx.nJ[0];
			if (_idx.nJ[1] == -2)
				_idx.nJ[1] = _data.getCols(sZDatatable) - 1;

			parser_CheckIndices(_idx.nJ[0], _idx.nJ[1]);

			if (_idx.nI[1] == -2)
			{
				_idx.nI[1] = _data.getLines(sZDatatable, true) - _data.getAppendedZeroes(_idx.nJ[0], sZDatatable) - 1;
				for (long long int j = _idx.nJ[0] + 1; j <= _idx.nJ[1]; j++)
				{
					if (_data.getLines(sZDatatable, true) - _data.getAppendedZeroes(j, sZDatatable) - 1 > _idx.nI[1])
						_idx.nI[1] = _data.getLines(sZDatatable, true) - _data.getAppendedZeroes(j, sZDatatable) - 1;
				}
			}

			parser_CheckIndices(_idx.nI[0], _idx.nI[1]);

			vSamples.push_back(_idx.nI[1] - _idx.nI[0] + 1);
			vSamples.push_back(_idx.nJ[1] - _idx.nJ[0] + 1);
		}
		else
		{
			// The indices are vectors
			vSamples.push_back(_idx.vI.size());
			vSamples.push_back(_idx.vJ.size());
		}

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

// This function will extract the x or y vectors which are needed as axes for the datagrid
static vector<double> parser_extractVectorForDatagrid(const string& sCmd, string& sVectorVals, const string& sZVals, size_t nSamples, Parser& _parser, Datafile& _data, const Settings& _option)
{
	vector<double> vVectorVals;

	// Data direct from the table, not an index pair
	if ((sVectorVals.find("data(") != string::npos || _data.containsCacheElements(sVectorVals)) && sVectorVals.find(':', getMatchingParenthesis(sVectorVals.substr(sVectorVals.find('('))) + sVectorVals.find('(')) == string::npos)
	{
		// Get the indices
		Indices _idx = parser_getIndices(sVectorVals, _parser, _data, _option);

		// Identify the table
		string sDatatable = "data";
		if (_data.containsCacheElements(sVectorVals))
		{
			_data.setCacheStatus(true);
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
		if ((_idx.nI[0] == -1 && !_idx.vI.size()) || (_idx.nJ[0] == -1 && !_idx.vJ.size()))
			throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
		if (!_idx.vI.size())
		{
			// The indices are numbers
			if (_idx.nI[1] == -1)
				_idx.nI[1] = _idx.nI[0];
			if (_idx.nJ[1] == -1)
				_idx.nJ[1] = _idx.nJ[0];
			if (_idx.nJ[1] == -2)
				_idx.nJ[1] = _data.getCols(sDatatable) - 1;
			if (_idx.nI[1] == -2 && _idx.nJ[1] != _idx.nJ[0])
				throw SyntaxError(SyntaxError::NO_MATRIX, sCmd, SyntaxError::invalid_position);
			if (_idx.nI[1] == -2)
				_idx.nI[1] = _data.getLines(sDatatable, true) - _data.getAppendedZeroes(_idx.nJ[0], sDatatable) - 1;

			parser_CheckIndices(_idx.nI[0], _idx.nI[1]);
			parser_CheckIndices(_idx.nJ[0], _idx.nJ[1]);

			// Only if the z values are also a table read the vector from the table
			if (sZVals.find("data(") != string::npos || _data.containsCacheElements(sZVals))
			{
				for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
				{
					for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
					{
						vVectorVals.push_back(_data.getElement(i, j, sDatatable));
					}
				}
			}
			else
			{
				// Otherwise use minimal and maximal values
				double dMin = _data.min(sDatatable, _idx.nI[0], _idx.nI[1], _idx.nJ[0], _idx.nJ[1]);
				double dMax = _data.max(sDatatable, _idx.nI[0], _idx.nI[1], _idx.nJ[0], _idx.nJ[1]);

				for (unsigned int i = 0; i < nSamples; i++)
					vVectorVals.push_back((dMax - dMin) / double(nSamples - 1)*i + dMin);
			}
		}
		else
		{
			// The indices are vectors
			if (sZVals.find("data(") != string::npos || _data.containsCacheElements(sZVals))
			{
				// Only if the z values are also a table read the vector from the table
				vVectorVals = _data.getElement(_idx.vI, _idx.vJ, sDatatable);
			}
			else
			{
				// Otherwise use minimal and maximal values
				double dMin = _data.min(sDatatable, _idx.vI, _idx.vJ);
				double dMax = _data.max(sDatatable, _idx.vI, _idx.vJ);

				for (unsigned int i = 0; i < nSamples; i++)
					vVectorVals.push_back((dMax - dMin) / double(nSamples - 1)*i + dMin);
			}
		}
		_data.setCacheStatus(false);
	}
	else if (sVectorVals.find(':') != string::npos)
	{
		// Index pair - If the index pair contains data elements, get their values now
		if (sVectorVals.find("data(") != string::npos || _data.containsCacheElements(sVectorVals))
		{
			getDataElements(sVectorVals, _parser, _data, _option);
		}
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
		{
			vVectorVals.push_back(dResult[0] + (dResult[1] - dResult[0]) / double(nSamples - 1)*i);
		}
	}
	else
		throw SyntaxError(SyntaxError::SEPARATOR_NOT_FOUND, sCmd, SyntaxError::invalid_position);

	return vVectorVals;
}

// This function will expand the z vector into a z matrix by triangulation
static void parser_expandVectorToDatagrid(vector<double>& vXVals, vector<double>& vYVals, vector<vector<double>>& vZVals, size_t nSamples_x, size_t nSamples_y)
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
		{
			vXVals.push_back(_mData[1].Minimal() + (_mData[1].Maximal() - _mData[1].Minimal()) / (double)(nSamples_x - 1)*i);
		}
		for (unsigned int i = 0; i < nSamples_y; i++)
		{
			vYVals.push_back(_mData[2].Minimal() + (_mData[2].Maximal() - _mData[2].Minimal()) / (double)(nSamples_y - 1)*i);
		}

		// Copy the z matrix
		for (unsigned int i = 0; i < nSamples_x; i++)
		{
			for (unsigned int j = 0; j < nSamples_y; j++)
			{
				vVector.push_back(_mData[0].a[i + nSamples_x * j]);
			}
			vZVals.push_back(vVector);
			vVector.clear();
		}
	}
}

// This function evaluates the "target=TABLE()" expression and creates the target table, if needed. If this option is not found, the function
// will create a default target cache.
string parser_evalTargetExpression(string& sCmd, const string& sDefaultTarget, Indices& _idx, Parser& _parser, Datafile& _data, const Settings& _option)
{
	string sTargetTable;

	// search for the target option in the command string
	if (matchParams(sCmd, "target", '='))
	{
		// Extract the table name
		sTargetTable = getArgAtPos(sCmd, matchParams(sCmd, "target", '=') + 6);

		// data is read-only. Therefore it cannot be used as target
		if (sTargetTable.substr(0, sTargetTable.find('(')) == "data")
			throw SyntaxError(SyntaxError::READ_ONLY_DATA, sCmd, sTargetTable);

		// Create the target table, if it doesn't exist
		if (!_data.isCacheElement(sTargetTable.substr(0, sTargetTable.find('(')) + "()"))
			_data.addCache(sTargetTable.substr(0, sTargetTable.find('(')), _option);

		// Read the target indices
		_idx = parser_getIndices(sTargetTable, _parser, _data, _option);
		sTargetTable.erase(sTargetTable.find('('));

		// check the indices
		if (_idx.nI[0] == -1 || _idx.nJ[0] == -1)
			throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

		// remove the target option and its value from the command line
		sCmd.erase(sCmd.find(getArgAtPos(sCmd, matchParams(sCmd, "target", '=') + 6), matchParams(sCmd, "target", '=') - 1), getArgAtPos(sCmd, matchParams(sCmd, "target", '=') + 6).length());
		sCmd.erase(matchParams(sCmd, "target", '=') - 1, 7);
	}
	else if (sDefaultTarget.length())
	{
		// If not found, create a default index set
		_idx.nI[0] = 0;
		_idx.nI[1] = -2;
		_idx.nJ[0] = 0;

		// Create cache, if needed. Otherwise get first empty column
		if (_data.isCacheElement(sDefaultTarget + "()"))
			_idx.nJ[0] += _data.getCols(sDefaultTarget, false);
		else
			_data.addCache(sDefaultTarget, _option);

		_idx.nJ[1] = -2;
		sTargetTable = sDefaultTarget;
	}

	// return the target table name
	return sTargetTable;
}

// This function will evaluate the passed indices, so that they match the dimensions of the passed cache.
bool parser_evalIndices(const string& sCache, Indices& _idx, Datafile& _data)
{
	// Check the initial indices
	if (_idx.nI[0] == -1 || _idx.nJ[0] == -1)
		return false;

	// Evaluate the case for an open end index
	if (_idx.nI[1] == -2)
		_idx.nI[1] = _data.getLines(sCache.substr(0, sCache.find('(')), false) - 1;

	if (_idx.nJ[1] == -2)
		_idx.nJ[1] = _data.getCols(sCache.substr(0, sCache.find('('))) - 1;

	// Evaluate the case for a missing index
	if (_idx.nI[1] == -1)
		_idx.nI[1] = _idx.nI[0];
	if (_idx.nJ[1] == -1)
		_idx.nJ[1] = _idx.nJ[0];

	// Signal success
	return true;
}

// This function will read the interval syntax and return it as a vector
vector<double> parser_IntervalReader(string& sExpr, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option, bool bEraseInterval)
{
	vector<double> vInterval;
	string sInterval[2] = {"", ""};

	// Get user defined functions
	if (!_functions.call(sExpr))
		throw SyntaxError(SyntaxError::FUNCTION_ERROR, sExpr, SyntaxError::invalid_position);

	// If the expression contains data elements, get their contents here
	if (sExpr.find("data(") != string::npos || _data.containsCacheElements(sExpr))
		getDataElements(sExpr, _parser, _data, _option);

	// Get the interval for x
	if (matchParams(sExpr, "x", '='))
	{
		sInterval[0] = getArgAtPos(sExpr, matchParams(sExpr, "x", '=') + 1);

		// Erase the interval definition, if needed
		if (bEraseInterval)
		{
			sExpr.erase(sExpr.find(sInterval[0]), sInterval[0].length());
			sExpr.erase(sExpr.rfind('x', matchParams(sExpr, "x", '=')), matchParams(sExpr, "x", '=') + 1 - sExpr.rfind('x', matchParams(sExpr, "x", '=')));
		}

		// If the intervall contains a colon, split it there
		if (sInterval[0].find(':') != string::npos)
			parser_SplitArgs(sInterval[0], sInterval[1], ':', _option, true);
		if (isNotEmptyExpression(sInterval[0]))
		{
			_parser.SetExpr(sInterval[0]);
			vInterval.push_back(_parser.Eval());
		}
		else
			vInterval.push_back(NAN);
		if (isNotEmptyExpression(sInterval[1]))
		{
			_parser.SetExpr(sInterval[1]);
			vInterval.push_back(_parser.Eval());
		}
		else
			vInterval.push_back(NAN);
	}

	// Get the interval for y
	if (matchParams(sExpr, "y", '='))
	{
		sInterval[0] = getArgAtPos(sExpr, matchParams(sExpr, "y", '=') + 1);

		// Erase the interval definition, if needed
		if (bEraseInterval)
		{
			sExpr.erase(sExpr.find(sInterval[0]), sInterval[0].length());
			sExpr.erase(sExpr.rfind('y', matchParams(sExpr, "y", '=')), matchParams(sExpr, "y", '=') + 1 - sExpr.rfind('y', matchParams(sExpr, "y", '=')));
		}

		// If the intervall contains a colon, split it there
		if (sInterval[0].find(':') != string::npos)
			parser_SplitArgs(sInterval[0], sInterval[1], ':', _option, true);
		while (vInterval.size() < 2)
		{
			vInterval.push_back(NAN);
		}
		if (isNotEmptyExpression(sInterval[0]))
		{
			_parser.SetExpr(sInterval[0]);
			vInterval.push_back(_parser.Eval());
		}
		else
			vInterval.push_back(NAN);
		if (isNotEmptyExpression(sInterval[1]))
		{
			_parser.SetExpr(sInterval[1]);
			vInterval.push_back(_parser.Eval());
		}
		else
			vInterval.push_back(NAN);
	}

	// Get the interval for z
	if (matchParams(sExpr, "z", '='))
	{
		sInterval[0] = getArgAtPos(sExpr, matchParams(sExpr, "z", '=') + 1);

		// Erase the interval definition, if needed
		if (bEraseInterval)
		{
			sExpr.erase(sExpr.find(sInterval[0]), sInterval[0].length());
			sExpr.erase(sExpr.rfind('z', matchParams(sExpr, "z", '=')), matchParams(sExpr, "z", '=') + 1 - sExpr.rfind('z', matchParams(sExpr, "z", '=')));
		}

		// If the intervall contains a colon, split it there
		if (sInterval[0].find(':') != string::npos)
			parser_SplitArgs(sInterval[0], sInterval[1], ':', _option, true);
		while (vInterval.size() < 4)
			vInterval.push_back(NAN);
		if (isNotEmptyExpression(sInterval[0]))
		{
			_parser.SetExpr(sInterval[0]);
			vInterval.push_back(_parser.Eval());
		}
		else
			vInterval.push_back(NAN);
		if (isNotEmptyExpression(sInterval[1]))
		{
			_parser.SetExpr(sInterval[1]);
			vInterval.push_back(_parser.Eval());
		}
		else
			vInterval.push_back(NAN);
	}

	// Read the interval syntax
	if (sExpr.find('[') != string::npos
			&& sExpr.find(']', sExpr.find('[')) != string::npos
			&& sExpr.find(':', sExpr.find('[')) != string::npos)
	{
		unsigned int nPos = 0;

		// Find the correct interval bracket
		do
		{
			nPos = sExpr.find('[', nPos);
			if (nPos == string::npos || sExpr.find(']', nPos) == string::npos)
				break;
			nPos++;
		}
		while (isInQuotes(sExpr, nPos) || sExpr.substr(nPos, sExpr.find(']') - nPos).find(':') == string::npos);

		// If an interval bracket was found
		if (nPos != string::npos && sExpr.find(']', nPos) != string::npos)
		{
			string sRanges[3];
			sRanges[0] = sExpr.substr(nPos, sExpr.find(']', nPos) - nPos);

			// Erase the interval part from the expression, if needed
			if (bEraseInterval)
				sExpr.erase(nPos - 1, sExpr.find(']', nPos) - nPos + 2);

			// As long as a comma is found in the interval
			while (sRanges[0].find(',') != string::npos)
			{
				sRanges[0] = "(" + sRanges[0] + ")";

				// Split at the comma
				parser_SplitArgs(sRanges[0], sRanges[2], ',', _option, false);
				if (sRanges[0].find(':') == string::npos)
				{
					sRanges[0] = sRanges[2];
					continue;
				}
				sRanges[0] = "(" + sRanges[0] + ")";

				// Split at the colon
				parser_SplitArgs(sRanges[0], sRanges[1], ':', _option, false);

				// Set the intervals and parse them
				if (isNotEmptyExpression(sRanges[0]))
				{
					_parser.SetExpr(sRanges[0]);
					vInterval.push_back(_parser.Eval());
				}
				else
					vInterval.push_back(NAN);
				if (isNotEmptyExpression(sRanges[1]))
				{
					_parser.SetExpr(sRanges[1]);
					vInterval.push_back(_parser.Eval());
				}
				else
					vInterval.push_back(NAN);
				sRanges[0] = sRanges[2];
			}

			// If a colon is found in the first element
			if (sRanges[0].find(':') != string::npos)
			{
				sRanges[0] = "(" + sRanges[0] + ")";

				// Split at the colon
				parser_SplitArgs(sRanges[0], sRanges[1], ':', _option, false);

				// Set the intervals and parse them
				if (isNotEmptyExpression(sRanges[0]))
				{
					_parser.SetExpr(sRanges[0]);
					vInterval.push_back(_parser.Eval());
				}
				else
					vInterval.push_back(NAN);
				if (isNotEmptyExpression(sRanges[1]))
				{
					_parser.SetExpr(sRanges[1]);
					vInterval.push_back(_parser.Eval());
				}
				else
					vInterval.push_back(NAN);
			}
		}
	}

	// Return the calculated interval part
	return vInterval;
}

// audio data() -samples=SAMPLES file=FILENAME
bool parser_writeAudio(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
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
	if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
	{
		string sDummy = "";
		if (!parser_StringParser(sCmd, sDummy, _data, _parser, _option, true))
			throw SyntaxError(SyntaxError::STRING_ERROR, sCmd, SyntaxError::invalid_position);
	}
	// Funktionen aufrufen
	if (!_functions.call(sCmd))
		throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, SyntaxError::invalid_position);

	// Samples lesen
	if (matchParams(sCmd, "samples", '='))
	{
		string sSamples = getArgAtPos(sCmd, matchParams(sCmd, "samples", '=') + 7);
		if (sSamples.find("data(") != string::npos || _data.containsCacheElements(sSamples))
		{
			getDataElements(sSamples, _parser, _data, _option);
		}
		_parser.SetExpr(sSamples);
		if (!isnan(_parser.Eval()) && !isinf(_parser.Eval()) && _parser.Eval() >= 1);
		nSamples = (int)_parser.Eval();
	}

	// Dateiname lesen
	if (matchParams(sCmd, "file", '='))
		sAudioFileName = getArgAtPos(sCmd, matchParams(sCmd, "file", '=') + 4);
	if (sAudioFileName.find('/') == string::npos && sAudioFileName.find('\\') == string::npos)
		sAudioFileName.insert(0, "<savepath>/");

	// Dateiname pruefen
	sAudioFileName = _data.ValidFileName(sAudioFileName, ".wav");

	// Indices lesen
	_idx = parser_getIndices(sCmd, _parser, _data, _option);
	sDataset = sCmd.substr(0, sCmd.find('('));
	StripSpaces(sDataset);
	if (_idx.vI.size() || _idx.vJ.size())
	{
		if (_idx.vJ.size() > 2)
			return false;
		if (fabs(_data.max(sDataset, _idx.vI, _idx.vJ)) > fabs(_data.min(sDataset, _idx.vI, _idx.vJ)))
			dMax = fabs(_data.max(sDataset, _idx.vI, _idx.vJ));
		else
			dMax = fabs(_data.min(sDataset, _idx.vI, _idx.vJ));
		_mDataSet.push_back(_data.getElement(_idx.vI, vector<long long int>(_idx.vJ[0]), sDataset));
		if (_idx.vJ.size() == 2)
			_mDataSet.push_back(_data.getElement(_idx.vI, vector<long long int>(_idx.vJ[1]), sDataset));
		_mDataSet = parser_transposeMatrix(_mDataSet);
	}
	else
	{
		if (_idx.nI[0] == -1 || _idx.nJ[0] == -1)
			return false;
		if (_idx.nI[1] == -1)
			_idx.nI[1] = _idx.nI[0];
		else if (_idx.nI[1] == -2)
			_idx.nI[1] = _data.getLines(sDataset, false) - 1;
		if (_idx.nJ[1] == -1)
			_idx.nJ[1] = _idx.nJ[0];
		else if (_idx.nJ[1] == -2)
		{
			_idx.nJ[1] = _idx.nJ[0] + 1;
		}
		if (_data.getCols(sDataset, false) <= _idx.nJ[1])
			_idx.nJ[1] = _idx.nJ[0];
		_mDataSet = parser_ZeroesMatrix(_idx.nI[1] - _idx.nI[0] + 1, (_idx.nJ[1] != _idx.nJ[0] ? 2 : 1));
		double dMaxCol[2] = {0.0, 0.0};
		if (_idx.nJ[1] != _idx.nJ[0])
		{
			if (fabs(_data.max(sDataset, _idx.nI[0], _idx.nI[1], _idx.nJ[1], -1)) > fabs(_data.min(sDataset, _idx.nI[0], _idx.nI[1], _idx.nJ[1], -1)))
				dMaxCol[1] = fabs(_data.max(sDataset, _idx.nI[0], _idx.nI[1], _idx.nJ[1], -1));
			else
				dMaxCol[1] = fabs(_data.min(sDataset, _idx.nI[0], _idx.nI[1], _idx.nJ[1], -1));
			for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
				_mDataSet[i - _idx.nI[0]][1] = _data.getElement(i, _idx.nJ[1], sDataset);
		}
		if (fabs(_data.max(sDataset, _idx.nI[0], _idx.nI[1], _idx.nJ[0], -1)) > fabs(_data.min(sDataset, _idx.nI[0], _idx.nI[1], _idx.nJ[0], -1)))
			dMaxCol[1] = fabs(_data.max(sDataset, _idx.nI[0], _idx.nI[1], _idx.nJ[0], -1));
		else
			dMaxCol[1] = fabs(_data.min(sDataset, _idx.nI[0], _idx.nI[1], _idx.nJ[0], -1));
		for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
			_mDataSet[i - _idx.nI[0]][0] = _data.getElement(i, _idx.nJ[0], sDataset);

		if (dMaxCol[0] > dMaxCol[1])
			dMax = dMaxCol[0];
		else
			dMax = dMaxCol[1];

	}

	for (unsigned int i = 0; i < _mDataSet.size(); i++)
	{
		for (unsigned int j = 0; j < _mDataSet[0].size(); j++)
		{
			_mDataSet[i][j] = _mDataSet[i][j] / dMax * dValMax;
		}
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
		{
			write_word(fAudio, (int)_mDataSet[i][j], 2);
		}
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

bool parser_regularize(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
	int nSamples = 100;
	string sDataset = "";
	string sColHeaders[2] = {"", ""};
	Indices _idx;
	mglData _x, _v;
	double dXmin, dXmax;
	sCmd.erase(0, findCommand(sCmd).nPos + findCommand(sCmd).sString.length()); // Kommando entfernen

	// Strings parsen
	if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
	{
		string sDummy = "";
		if (!parser_StringParser(sCmd, sDummy, _data, _parser, _option, true))
			throw SyntaxError(SyntaxError::STRING_ERROR, sCmd, SyntaxError::invalid_position);
	}
	// Funktionen aufrufen
	if (!_functions.call(sCmd))
		throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, SyntaxError::invalid_position);

	// Samples lesen
	if (matchParams(sCmd, "samples", '='))
	{
		string sSamples = getArgAtPos(sCmd, matchParams(sCmd, "samples", '=') + 7);
		if (sSamples.find("data(") != string::npos || _data.containsCacheElements(sSamples))
		{
			getDataElements(sSamples, _parser, _data, _option);
		}
		_parser.SetExpr(sSamples);
		if (!isnan(_parser.Eval()) && !isinf(_parser.Eval()) && _parser.Eval() >= 1);
		nSamples = (int)_parser.Eval();
	}

	// Indices lesen
	_idx = parser_getIndices(sCmd, _parser, _data, _option);
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
	if (!matchParams(sCmd, "samples", '='))
		nSamples = _x.GetNx();
	mglData _regularized(nSamples);
	_regularized.Refill(_x, _v, dXmin, dXmax); //wohin damit?

	long long int nLastCol = _data.getCols(sDataset, false);
	for (long long int i = 0; i < nSamples; i++)
	{
		_data.writeToCache(i, nLastCol, sDataset, dXmin + i * (dXmax - dXmin) / (nSamples - 1));
		_data.writeToCache(i, nLastCol + 1, sDataset, _regularized.a[i]);
	}
	_data.setHeadLineElement(nLastCol, sDataset, sColHeaders[0]);
	_data.setHeadLineElement(nLastCol + 1, sDataset, sColHeaders[1]);
	return true;
}

bool parser_pulseAnalysis(string& _sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
	string sDataset = "";
	Indices _idx;
	mglData _v;
	vector<double> vPulseProperties;
	double dXmin = NAN, dXmax = NAN;
	double dSampleSize = NAN;
	string sCmd = _sCmd.substr(findCommand(_sCmd, "pulse").nPos + 5);

	// Strings parsen
	if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
	{
		string sDummy = "";
		if (!parser_StringParser(sCmd, sDummy, _data, _parser, _option, true))
			throw SyntaxError(SyntaxError::STRING_ERROR, _sCmd, SyntaxError::invalid_position);
	}
	// Funktionen aufrufen
	if (!_functions.call(sCmd))
		throw SyntaxError(SyntaxError::FUNCTION_ERROR, _sCmd, SyntaxError::invalid_position);


	// Indices lesen
	_idx = parser_getIndices(sCmd, _parser, _data, _option);
	sDataset = sCmd.substr(0, sCmd.find('('));
	StripSpaces(sDataset);
	Datafile _cache;
	getData(sDataset, _idx, _data, _cache);

	long long int nLines = _cache.getLines("cache", false);

	dXmin = _cache.min("cache", 0, nLines - 1, 0);
	dXmax = _cache.max("cache", 0, nLines - 1, 0);

	_v.Create(nLines);

	for (long long int i = 0; i < nLines; i++)
	{
		_v.a[i] = _cache.getElement(i, 1, "cache");
	}

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
		NumeReKernel::print(LineBreak("NUMERE: " + toUpperCase(_lang.get("PARSERFUNCS_PULSE_HEADLINE")), _option));
		make_hline();
		for (unsigned int i = 0; i < vPulseProperties.size(); i++)
		{
			NumeReKernel::printPreFmt(LineBreak("|   " + _lang.get("PARSERFUNCS_PULSE_TABLE_" + toString((int)i + 1) + "_*", toString(vPulseProperties[i], _option)), _option, 0) + "\n");
		}
		NumeReKernel::toggleTableStatus();
		make_hline();
	}

	_sCmd.replace(findCommand(_sCmd, "pulse").nPos, string::npos, "_~pulse[~_~]");
	_parser.SetVectorVar("_~pulse[~_~]", vPulseProperties);

	return true;
}

bool parser_stfa(string& sCmd, string& sTargetCache, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
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
	if (containsStrings(sCmd) || _data.containsStringVars(sCmd))
	{
		string sDummy = "";
		if (!parser_StringParser(sCmd, sDummy, _data, _parser, _option, true))
			throw SyntaxError(SyntaxError::STRING_ERROR, sCmd, SyntaxError::invalid_position);
	}
	// Funktionen aufrufen
	if (!_functions.call(sCmd))
		throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, SyntaxError::invalid_position);

	if (matchParams(sCmd, "samples", '='))
	{
		_parser.SetExpr(getArgAtPos(sCmd, matchParams(sCmd, "samples", '=') + 7));
		nSamples = _parser.Eval();
		if (nSamples < 0)
			nSamples = 0;
	}
	if (matchParams(sCmd, "target", '='))
	{
		sTargetCache = getArgAtPos(sCmd, matchParams(sCmd, "target", '=') + 6);
		_target = parser_getIndices(sTargetCache, _parser, _data, _option);
		sTargetCache.erase(sTargetCache.find('('));
		if (sTargetCache == "data")
			throw SyntaxError(SyntaxError::READ_ONLY_DATA, sCmd, SyntaxError::invalid_position);

		if (_target.nI[0] == -1 || _target.nJ[0] == -1)
			throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
	}
	else
	{
		_target.nI[0] = 0;
		_target.nI[1] = -2;
		_target.nJ[0] = 0;
		if (_data.isCacheElement("stfdat()"))
			_target.nJ[0] += _data.getCols("stfdat", false);
		sTargetCache = "stfdat";
		_target.nJ[1] = -2;
	}


	// Indices lesen
	_idx = parser_getIndices(sCmd, _parser, _data, _option);
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
	{
		_real.a[i] = _cache.getElement(i, 1, "cache");
	}

	if (!nSamples || nSamples > _real.GetNx())
	{
		nSamples = _real.GetNx() / 32;
	}

	// Tatsaechliche STFA
	_result = mglSTFA(_real, _imag, nSamples);

	dSampleSize = (dXmax - dXmin) / ((double)_result.GetNx() - 1.0);

	// Nyquist: _real.GetNx()/(dXmax-dXmin)/2.0
	dFmax = _real.GetNx() / (dXmax - dXmin) / 2.0;

	// Zielcache befuellen entsprechend der Fourier-Algorithmik

	if (_target.nI[1] == -2 || _target.nI[1] == -1)
		_target.nI[1] = _target.nI[0] + _result.GetNx();//?
	if (_target.nJ[1] == -2 || _target.nJ[1] == -1)
		_target.nJ[1] = _target.nJ[0] + _result.GetNy() + 2; //?

	if (!_data.isCacheElement(sTargetCache))
		_data.addCache(sTargetCache, _option);
	_data.setCacheStatus(true);

	// UPDATE DATA ELEMENTS
	for (int i = 0; i < _result.GetNx(); i++)
		_data.writeToCache(i, _target.nJ[0], sTargetCache, dXmin + i * dSampleSize);
	_data.setHeadLineElement(_target.nJ[0], sTargetCache, sDataset);
	dSampleSize = 2 * (dFmax - dFmin) / ((double)_result.GetNy() - 1.0);
	for (int i = 0; i < _result.GetNy() / 2; i++)
		_data.writeToCache(i, _target.nJ[0] + 1, sTargetCache, dFmin + i * dSampleSize); // Fourier f Hier ist was falsch
	_data.setHeadLineElement(_target.nJ[0] + 1, sTargetCache, "f [Hz]");

	for (int i = 0; i < _result.GetNx(); i++)
	{
		if (i + _target.nI[0] >= _target.nI[1])
			break;
		for (int j = 0; j < _result.GetNy() / 2; j++)
		{
			if (j + 2 + _target.nJ[0] >= _target.nJ[1])
				break;
			_data.writeToCache(_target.nI[0] + i, _target.nJ[0] + 2 + j, sTargetCache, _result[i + (j + _result.GetNy() / 2)*_result.GetNx()]);
			if (!i)
				_data.setHeadLineElement(_target.nJ[0] + 2 + j, sTargetCache, "A[" + toString((int)j + 1) + "]");
		}
	}
	_data.setCacheStatus(false);

	return true;
}

string parser_createMonome(double dCoefficient, const string& sArgument)
{
	if (!dCoefficient)
		return "";
	if (dCoefficient < 0)
		return "-" + toString(fabs(dCoefficient), 4) + "*" + sArgument;
	else
		return "+" + toString(dCoefficient, 4) + "*" + sArgument;
}

bool parser_spline(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
	Indices _idx;
	Datafile _cache;
	tk::spline _spline;
	vector<double> xVect, yVect;
	string sTableName = sCmd.substr(sCmd.find(' '));
	StripSpaces(sTableName);

	_idx = parser_getIndices(sTableName, _parser, _data, _option);
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
		string sRange = "(";
		string sArgument;

		if (xVect[i] == 0)
			sArgument = "x";
		else if (xVect[i] < 0)
			sArgument = "(x+" + toString(fabs(xVect[i]), 4) + ")";
		else
			sArgument = "(x-" + toString(xVect[i], 4) + ")";

		vector<double> vCoeffs = _spline[i];

		sRange += toString(vCoeffs[0], 4) + parser_createMonome(vCoeffs[1], sArgument) + parser_createMonome(vCoeffs[2], sArgument + "^2") + parser_createMonome(vCoeffs[3], sArgument + "^3") + ")";

		if (i == xVect.size() - 2)
			sRange += "*ivl(x," + toString(xVect[i], 4) + "," + toString(xVect[i + 1], 4) + ",1,1)";
		else
			sRange += "*ivl(x," + toString(xVect[i], 4) + "," + toString(xVect[i + 1], 4) + ",1,2)";
		sDefinition += sRange;

		if (i < xVect.size() - 2)
			sDefinition += " + ";
	}

	if (_option.getSystemPrintStatus())
		NumeReKernel::print(LineBreak(sDefinition, _option, true, 0, 8));

    bool bDefinitionSuccess = false;

	if (_functions.isDefined(sDefinition.substr(0, sDefinition.find(":="))))
		bDefinitionSuccess = _functions.defineFunc(sDefinition, true);
	else
		bDefinitionSuccess = _functions.defineFunc(sDefinition);

    if (bDefinitionSuccess)
        NumeReKernel::print(_lang.get("DEFINE_SUCCESS"));
    else
        NumeReKernel::issueWarning(_lang.get("DEFINE_FAILURE"));

	return true;
}
