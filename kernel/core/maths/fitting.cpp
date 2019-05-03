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

#include "parser_functions.hpp"
#include "../../kernel.hpp"

// This structure combines all main fitting
// parameter and storage objects to simplify
// the interaction between the main routine and
// the (file static) helper routines
struct FittingData
{
    vector<double> vx;
    vector<double> vy;
    vector<double> vy_w;
    vector<vector<double> > vz;
    vector<vector<double> > vz_w;

    double dMin;
    double dMax;
    double dMinY;
    double dMaxY;

    size_t nDim;
	size_t nFitVars;
	bool bUseErrors;
	bool bSaveErrors;
	bool bRestrictXVals;
	bool bRestrictYVals;
	bool bNoParams;
	bool b1DChiMap;
	double dPrecision;
	int nMaxIterations;

	string sFitFunction;
	string sRestrictions;
	string sParams;
	string sChiMap;
	string sChiMap_Vars[2];
};

// These are the prototypes of the file static helper routines
static vector<double> evaluateFittingParams(FittingData& fitData, string& sCmd, Indices& _idx, string& sTeXExportFile, bool& bTeXExport, bool& bMaskDialog);
static mu::varmap_type getFittingParameters(FittingData& fitData, const mu::varmap_type& varMap, const string& sCmd);
static int getDataForFit(const string& sCmd, string& sDimsForFitLog, FittingData& fitData);
static void removeObsoleteParentheses(string& sFunction);
static bool calculateChiMap(string sFunctionDefString, const string& sFuncDisplay, Indices& _idx, mu::varmap_type& varMap, mu::varmap_type& paramsMap, FittingData& fitData, vector<double> vInitialVals);
static string applyFitAlgorithm(Fitcontroller& _fControl, FittingData& fitData, mu::varmap_type& paramsMap, const string& sFuncDisplay);
static void calculateCovarianceData(FittingData& fitData, double dChisq, size_t paramsMapSize);
static string getFitOptionsTable(Fitcontroller& _fControl, FittingData& fitData, const string& sFuncDisplay, const string& sFittedFunction, const string& sDimsForFitLog, double dChisq, const mu::varmap_type& paramsMap, size_t nSize, bool forFitLog);
static string getParameterTable(FittingData& fitData, mu::varmap_type& paramsMap, const vector<double>& vInitialVals, size_t windowSize, const string& sPMSign, bool forFitLog);
static string constructCovarianceMatrix(FittingData& fitData, size_t paramsMapSize, bool forFitLog);
static double calculatePercentageAvgAndCreateParserVariables(FittingData& fitData, mu::varmap_type& paramsMap, double dChisq);
static string getFitAnalysis(Fitcontroller& _fControl, FittingData& fitData, double dNormChisq, double dAverageErrorPercentage, bool noOverfitting);
static void createTeXExport(Fitcontroller& _fControl, const string& sTeXExportFile, const string& sCmd, mu::varmap_type& paramsMap, FittingData& fitData, const vector<double>& vInitialVals, size_t nSize, const string& sFitAnalysis, const string& sFuncDisplay, const string& sFittedFunction, double dChisq);

// This is the fitting main routine
bool parser_fit(string& sCmd, Parser& _parser, Datafile& _data, Define& _functions, const Settings& _option)
{
    // Declare the FittingData object first
    FittingData fitData;

	vector<double> vInitialVals;
	vector<double> vInterVal;
	double dChisq = 0.0;
	Indices _idx;

	ofstream oFitLog;
	string sFitLog = "<savepath>/numerefit.log";
	sFitLog = _data.ValidFileName(sFitLog, ".log");

	bool bMaskDialog = false;
	bool bTeXExport = false;
	string sTeXExportFile = "<savepath>/fit.tex";
	string sDimsForFitLog;
	string sFunctionDefString = "";
	string sFuncDisplay = "";

	// Prepare the default values in the FittingData
	// object for the further calculation
	fitData.dMin = NAN;
	fitData.dMax = NAN;
	fitData.dMinY = NAN;
	fitData.dMaxY = NAN;
	fitData.nDim = 1;
	fitData.nFitVars = 0;
	fitData.bUseErrors = false;
	fitData.bSaveErrors = false;
	fitData.bRestrictXVals = false;
	fitData.bRestrictYVals = false;
	fitData.bNoParams = false;
	fitData.b1DChiMap = false;
	fitData.dPrecision = 1e-4;
	fitData.nMaxIterations = 500;
	fitData.sChiMap_Vars[0].clear();
	fitData.sChiMap_Vars[1].clear();
	fitData.sFitFunction = sCmd;
	fitData.sRestrictions = "";

	if (findCommand(sCmd, "fit").sString == "fitw")
		fitData.bUseErrors = true;

    // Ensure that data is available
	if (sCmd.find("data(") == string::npos && !_data.containsTablesOrClusters(sCmd))
		throw SyntaxError(SyntaxError::NO_DATA_FOR_FIT, sCmd, SyntaxError::invalid_position);

	// Evaluate all passed parameters in this file static
	// function and return the initial fitting parameter
	// values
	vInterVal = evaluateFittingParams(fitData, sCmd, _idx, sTeXExportFile, bTeXExport, bMaskDialog);

	fitData.sFitFunction = " " + fitData.sFitFunction + " ";
	_parser.SetExpr(fitData.sFitFunction);
	_parser.Eval();
	mu::varmap_type varMap = _parser.GetUsedVar();

	// Get the map containing all the fitting parameters,
	// which will be used by the fit
	mu::varmap_type paramsMap = getFittingParameters(fitData, varMap, sCmd);

	fitData.sParams.clear();

	// Determine the number of fitting variables
	if (varMap.find("x") != varMap.end())
		fitData.nFitVars += 1;

	if (varMap.find("y") != varMap.end())
		fitData.nFitVars += 2;

	if (varMap.find("z") != varMap.end())
		fitData.nFitVars += 4;

    // If a chi^2 map shall be calculated, ensure that its
    // fitting parameter names are set correctly
	if (fitData.sChiMap.length())
	{
		if (fitData.sChiMap_Vars[0] == "x" || fitData.sChiMap_Vars[1] == "x")
			throw SyntaxError(SyntaxError::CANNOT_BE_A_FITTING_PARAM, sCmd, SyntaxError::invalid_position, "x");

		if (fitData.sChiMap_Vars[0] == "y" || fitData.sChiMap_Vars[1] == "y")
			throw SyntaxError(SyntaxError::CANNOT_BE_A_FITTING_PARAM, sCmd, SyntaxError::invalid_position, "y");

		if (varMap.find(fitData.sChiMap_Vars[0]) == varMap.end())
			throw SyntaxError(SyntaxError::FITFUNC_NOT_CONTAINS, sCmd, SyntaxError::invalid_position, fitData.sChiMap_Vars[0]);

		if (varMap.find(fitData.sChiMap_Vars[1]) == varMap.end())
			fitData.b1DChiMap = true;
	}

	// Ensure that "x" is present in the function, if the fit
	// is along a single dimension
	if (!fitData.nFitVars || !(fitData.nFitVars & 1))
		throw SyntaxError(SyntaxError::FITFUNC_NOT_CONTAINS, sCmd, SyntaxError::invalid_position, "x");

    // Validate the parameters in the parameter map. If the
    // parameter map contains one of the variables x,y,z, then
    // throw an error. Also ensure that all passed fitting
    // parameters are also part of the fit function
	for (auto pItem = paramsMap.begin(); pItem != paramsMap.end(); ++pItem)
	{
		if (pItem->first == "x" || pItem->first == "y" || pItem->first == "z")
			throw SyntaxError(SyntaxError::CANNOT_BE_A_FITTING_PARAM, sCmd, SyntaxError::invalid_position, pItem->first);

		if (varMap.find(pItem->first) == varMap.end())
			throw SyntaxError(SyntaxError::FITFUNC_NOT_CONTAINS, sCmd, SyntaxError::invalid_position, pItem->first);
	}

	// Remove the parameters of the chi^2 map from the usual
	// fitting parameters, because we'll set their values
	// elsewhere
	if (fitData.sChiMap.length())
	{
		paramsMap.erase(fitData.sChiMap_Vars[0]);

		if (!fitData.b1DChiMap)
			paramsMap.erase(fitData.sChiMap_Vars[1]);
	}

	// Ensure that at least a single parameter is available
	if (!paramsMap.size())
		throw SyntaxError(SyntaxError::NO_PARAMS_FOR_FIT, sCmd, SyntaxError::invalid_position);

	sFuncDisplay = fitData.sFitFunction;

	sCmd.erase(0, findCommand(sCmd).nPos + findCommand(sCmd).sString.length());
	StripSpaces(sCmd);

	// Get the necessary data for the fitting routine. This is done
	// in the following file static routine. It will also return the
	// actual fitting dimension
    fitData.nDim = getDataForFit(sCmd, sDimsForFitLog, fitData);

    // Ensure that we're not trying to use more paramters than values
    if (paramsMap.size() > fitData.vy.size())
        throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);

	if (paramsMap.size() > fitData.vx.size())
		throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);

    if ((fitData.nFitVars & 2)
        && (paramsMap.size() > fitData.vz.size() || paramsMap.size() > fitData.vz[0].size()))
        throw SyntaxError(SyntaxError::OVERFITTING_ERROR, sCmd, SyntaxError::invalid_position);

	// Remove obsolete parentheses in the function displaying
	// string and the actual fitting function
	removeObsoleteParentheses(sFuncDisplay);
	removeObsoleteParentheses(fitData.sFitFunction);

	if (_option.getSystemPrintStatus())
		NumeReKernel::printPreFmt(LineBreak("|-> " + _lang.get("PARSERFUNCS_FIT_FITTING", sFuncDisplay) + " ", _option, 0));

	for (auto iter = paramsMap.begin(); iter != paramsMap.end(); ++iter)
	{
		vInitialVals.push_back(*(iter->second));
	}

	// If the user desires a chi^2 map, then it is calculated in the
	// following static function and the code returns after the map
	// has been calculated
	if (fitData.sChiMap.length())
		return calculateChiMap(sFunctionDefString, sFuncDisplay, _idx, varMap, paramsMap, fitData, vInitialVals);

    // Create a fitcontroller object
	Fitcontroller _fControl(&_parser);

	// Apply the fitting algorithm to data and fitting function.
	// This will update the parameter values and return all necessary
	// values for evaluating the fitting result.
	sFunctionDefString =  applyFitAlgorithm(_fControl, fitData, paramsMap, sFuncDisplay);

	// DONE. We're finished with fitting. The following lines have the
	// single purpose to display the results in the logfiles and in the
	// terminal, if necessary.
    //
	// Overwrite the z-weighting field with the parameter
	// covariance matrix as we don't need the weighting field any more
	fitData.vz_w = _fControl.getCovarianceMatrix();
	dChisq = _fControl.getFitChi();

	// Calculate the data for the covariance matrix. This includes multiplying
	// the matrix elements with the chi^2 value depending on the type of the
	// fit
	calculateCovarianceData(fitData, dChisq, paramsMap.size());
	size_t nSize = ((fitData.vz.size()) ? (fitData.vz.size() * fitData.vz[0].size()) : fitData.vx.size());

	// Reduce the file size of the fit log, if necessary
	if (!bMaskDialog && _option.getSystemPrintStatus())
		reduceLogFilesize(sFitLog);

	string sFittedFunction = _fControl.getFitFunction();

	// Open the fitting log file stream
	oFitLog.open(sFitLog.c_str(), ios_base::ate | ios_base::app);

	// If the we could not open the file stream
	// abort and inform the user
	if (oFitLog.fail())
	{
		oFitLog.close();
		_data.setCacheStatus(false);
		NumeReKernel::printPreFmt("\n");
		throw SyntaxError(SyntaxError::CANNOT_OPEN_FITLOG, sCmd, SyntaxError::invalid_position);
	}

	// Write the headline to the fitting logfile
	oFitLog << std::setw(76) << std::setfill('=') << '=' << endl;
	oFitLog << toUpperCase(_lang.get("PARSERFUNCS_FIT_HEADLINE")) << ": " << getTimeStamp(false) << endl;
	oFitLog << std::setw(76) << std::setfill('=') << '=' << endl;

	// Write the fitting options table to the log file
	oFitLog << getFitOptionsTable(_fControl, fitData, sFuncDisplay, sFittedFunction, sDimsForFitLog, dChisq, paramsMap, nSize, true) << endl;

	string sPMSign = " ";
	sPMSign[0] = (char)177;

	// Prepare the headline for the fitting parameter table
	if (fitData.bUseErrors)
		oFitLog << _lang.get("PARSERFUNCS_FIT_LOG_TABLEHEAD1") << endl;
	else
		oFitLog << _lang.get("PARSERFUNCS_FIT_LOG_TABLEHEAD2") << endl;

    // Write the fitting parameter table to the logfile
	oFitLog << std::setw(76) << std::setfill('-') << '-' << endl;
	oFitLog << getParameterTable(fitData, paramsMap, vInitialVals, 80, sPMSign, true);
	oFitLog << std::setw(76) << std::setfill('-') << '-' << endl;


	if (_option.getSystemPrintStatus())
		NumeReKernel::printPreFmt(_lang.get("COMMON_SUCCESS") + ".\n");

	if (_option.getSystemPrintStatus() && !bMaskDialog)
	{
		NumeReKernel::toggleTableStatus();
		make_hline();
		NumeReKernel::print(toSystemCodePage("NUMERE: " + toUpperCase(_lang.get("PARSERFUNCS_FIT_HEADLINE"))));
		make_hline();

		// Write the fitting options table to the terminal screen
		NumeReKernel::print(getFitOptionsTable(_fControl, fitData, sFuncDisplay, sFittedFunction, sDimsForFitLog, dChisq, paramsMap, nSize, false));
		NumeReKernel::printPreFmt("|\n");

		// Prepare the headline for the fitting parameter table
		// displayed in the terminal
		if (fitData.bUseErrors)
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

		// Write the fitting parameter table to the terminal
		NumeReKernel::printPreFmt("|   " + strfill("-", _option.getWindow() - 4, '-') + "\n");
        NumeReKernel::printPreFmt(getParameterTable(fitData, paramsMap, vInitialVals, _option.getWindow(), sPMSign, false));
		NumeReKernel::printPreFmt("|   " + strfill("-", _option.getWindow() - 4, '-') + "\n");
	}

	// Calculate the average of the percentual errors, which will be used to
	// derive a fitting analysis and create the variable to store the individual
	// fitting error values needed for further calulations (error propagation, etc.)
	double dAverageErrorPercentage = calculatePercentageAvgAndCreateParserVariables(fitData, paramsMap, dChisq);

	// Write the correlation matrix to the log file and the terminal
	if (paramsMap.size() > 1 && paramsMap.size() != nSize)
	{
		oFitLog << endl;
		oFitLog << _lang.get("PARSERFUNCS_FIT_CORRELMAT_HEAD") << ":" << endl;
		oFitLog << endl;
        oFitLog << constructCovarianceMatrix(fitData, paramsMap.size(), true) << endl;

		if (_option.getSystemPrintStatus() && !bMaskDialog)
		{
			NumeReKernel::printPreFmt("|\n|-> " + toSystemCodePage(_lang.get("PARSERFUNCS_FIT_CORRELMAT_HEAD")) + ":\n|\n");
			NumeReKernel::printPreFmt(constructCovarianceMatrix(fitData, paramsMap.size(), false));
		}
	}

	if (fitData.nFitVars & 2)
		nSize *= nSize;

    // Get the fitting analysis as a string. The fitting analysis is
    // calculated from the average of the percentual errors and the
    // value of the reduced chi^2
    string sFitAnalysis = getFitAnalysis(_fControl, fitData, dChisq / (double)(nSize - paramsMap.size()), dAverageErrorPercentage, nSize != paramsMap.size());

	// Write the fitting analysis to the logfile and to the terminal
	oFitLog << _lang.get("PARSERFUNCS_FIT_ANALYSIS") << ":" << endl;
	oFitLog << sFitAnalysis << endl;
	oFitLog.close();

	if (_option.getSystemPrintStatus() && !bMaskDialog)
	{
		NumeReKernel::printPreFmt("|\n|-> " + _lang.get("PARSERFUNCS_FIT_ANALYSIS") + ":\n");
		NumeReKernel::print(sFitAnalysis);
		NumeReKernel::toggleTableStatus();
		make_hline();
	}

	// If the user requested an export of the fitting results to
	// a TeX file, this is done here
	if (bTeXExport)
        createTeXExport(_fControl, sTeXExportFile, sCmd, paramsMap, fitData, vInitialVals, nSize, sFitAnalysis, sFuncDisplay, sFittedFunction, dChisq);

    // Update the function definition of the automatically created
    // Fit(x) or Fitw(x) function
	bool bDefinitionSuccess = false;

	if (!_functions.isDefined(sFunctionDefString))
		bDefinitionSuccess = _functions.defineFunc(sFunctionDefString);
	else if (_functions.getDefine(_functions.getFunctionIndex(sFunctionDefString)) != sFunctionDefString)
		bDefinitionSuccess = _functions.defineFunc(sFunctionDefString, true);
    else if (_functions.getDefine(_functions.getFunctionIndex(sFunctionDefString)) == sFunctionDefString)
        return true;

    if (bDefinitionSuccess)
        NumeReKernel::print(_lang.get("DEFINE_SUCCESS"), _option.getSystemPrintStatus());
    else
        NumeReKernel::issueWarning(_lang.get("DEFINE_FAILURE"));

	return true;
}


// This static function evaluates the passed
// parameters of the fitting routine
static vector<double> evaluateFittingParams(FittingData& fitData, string& sCmd, Indices& _idx, string& sTeXExportFile, bool& bTeXExport, bool& bMaskDialog)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    vector<double> vInterVal;
    static const string sBADFUNCTIONS = "ascii(),char(),findfile(),findparam(),gauss(),getopt(),is_string(),rand(),replace(),replaceall(),split(),strfnd(),string_cast(),strrfnd(),strlen(),time(),to_char(),to_cmd(),to_string(),to_value()";

    // Evaluate the chi^2 map option
    if (matchParams(sCmd, "chimap", '='))
	{
		fitData.sChiMap = getArgAtPos(sCmd, matchParams(sCmd, "chimap", '=') + 6);
		eraseToken(sCmd, "chimap", true);

		if (fitData.sChiMap.length())
		{
			if (fitData.sChiMap.substr(0, fitData.sChiMap.find('(')) == "data")
				throw SyntaxError(SyntaxError::READ_ONLY_DATA, sCmd, SyntaxError::invalid_position);

			_idx = parser_getIndices(fitData.sChiMap, _parser, _data, _option);

			if (!isValidIndexSet(_idx))
				throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

			if (_idx.col.size() < 2)
				throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

			parser_evalIndices(fitData.sChiMap, _idx, _data);
			fitData.sChiMap.erase(fitData.sChiMap.find('('));
            fitData.sChiMap_Vars[0] = _data.getHeadLineElement(_idx.col[0], fitData.sChiMap);
            fitData.sChiMap_Vars[1] = _data.getHeadLineElement(_idx.col[1], fitData.sChiMap);
		}
	}

	// Does the user want to export the results into a TeX file?
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

	// Ensure that the file name for the TeX file is valid
	if (bTeXExport)
	{
		sTeXExportFile = _data.ValidFileName(sTeXExportFile, ".tex");

		if (sTeXExportFile.substr(sTeXExportFile.rfind('.')) != ".tex")
			sTeXExportFile.replace(sTeXExportFile.rfind('.'), string::npos, ".tex");
	}

	// Separate expression from the command line option list
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

	// Decode possible interval definitions in the command
	// line option list
	vInterVal = parser_IntervalReader(sCmd, _parser, _data, _functions, _option, true);

	// Evaluate the contents of the parsed interval definitions
	if (vInterVal.size())
	{
		if (vInterVal.size() >= 4)
		{
			fitData.dMin = vInterVal[0];
			fitData.dMax = vInterVal[1];
			fitData.dMinY = vInterVal[2];
			fitData.dMaxY = vInterVal[3];

			if (!isnan(fitData.dMin) || !isnan(fitData.dMax))
				fitData.bRestrictXVals = true;

			if (!isnan(fitData.dMinY) || !isnan(fitData.dMaxY))
				fitData.bRestrictYVals = true;
		}
		else if (vInterVal.size() == 2)
		{
			fitData.dMin = vInterVal[0];
			fitData.dMax = vInterVal[1];

			if (!isnan(fitData.dMin) || !isnan(fitData.dMax))
				fitData.bRestrictXVals = true;
		}
	}

	// Insert the command line option list after the intervals
	// were parsed into the original command line
	for (unsigned int i = 0; i < fitData.sFitFunction.length(); i++)
	{
		if (fitData.sFitFunction[i] == '(')
			i += getMatchingParenthesis(fitData.sFitFunction.substr(i));

		if (fitData.sFitFunction[i] == '-')
		{
			fitData.sFitFunction.replace(i, string::npos, sCmd.substr(sCmd.find('-')));
			break;
		}
	}

	sCmd = fitData.sFitFunction;

	// Because it's quite likely that one misspells the option value
	// "saverr", whe accept also the spelling "saveer" as an alternative
	if (matchParams(fitData.sFitFunction, "saverr"))
	{
		fitData.bSaveErrors = true;
		fitData.sFitFunction.erase(matchParams(fitData.sFitFunction, "saverr") - 1, 6);
		sCmd.erase(matchParams(sCmd, "saverr") - 1, 6);
	}

	if (matchParams(fitData.sFitFunction, "saveer"))
	{
		fitData.bSaveErrors = true;
		fitData.sFitFunction.erase(matchParams(fitData.sFitFunction, "saveer") - 1, 6);
		sCmd.erase(matchParams(sCmd, "saveer") - 1, 6);
	}

	// The masking paramter
	if (matchParams(fitData.sFitFunction, "mask"))
	{
		bMaskDialog = true;
		fitData.sFitFunction.erase(matchParams(fitData.sFitFunction, "mask") - 1, 6);
		sCmd.erase(matchParams(sCmd, "mask") - 1, 6);
	}

	// Ensure that a fitting function was defined
	if (!matchParams(fitData.sFitFunction, "with", '='))
		throw SyntaxError(SyntaxError::NO_FUNCTION_FOR_FIT, sCmd, SyntaxError::invalid_position);

    // Changes to the tolerance
	if (matchParams(fitData.sFitFunction, "tol", '='))
	{
		_parser.SetExpr(getArgAtPos(fitData.sFitFunction, matchParams(fitData.sFitFunction, "tol", '=') + 3));
		eraseToken(sCmd, "tol", true);
		eraseToken(fitData.sFitFunction, "tol", true);
		fitData.dPrecision = fabs(_parser.Eval());

		if (isnan(fitData.dPrecision) || isinf(fitData.dPrecision) || fitData.dPrecision == 0)
			fitData.dPrecision = 1e-4;
	}

	// Changes to the maximal number of iterations
	if (matchParams(fitData.sFitFunction, "iter", '='))
	{
		_parser.SetExpr(getArgAtPos(fitData.sFitFunction, matchParams(fitData.sFitFunction, "iter", '=') + 4));
		eraseToken(sCmd, "iter", true);
		eraseToken(fitData.sFitFunction, "iter", true);
		fitData.nMaxIterations = abs(rint(_parser.Eval()));

		if (!fitData.nMaxIterations)
			fitData.nMaxIterations = 500;
	}

	// Fitting parameter restrictions
	if (matchParams(fitData.sFitFunction, "restrict", '='))
	{
		fitData.sRestrictions = getArgAtPos(fitData.sFitFunction, matchParams(fitData.sFitFunction, "restrict", '=') + 8);
		eraseToken(sCmd, "restrict", true);
		eraseToken(fitData.sFitFunction, "restrict", true);

		if (fitData.sRestrictions.length() && fitData.sRestrictions.front() == '[' && fitData.sRestrictions.back() == ']')
		{
			fitData.sRestrictions.erase(0, 1);
			fitData.sRestrictions.pop_back();
		}

		StripSpaces(fitData.sRestrictions);

		if (fitData.sRestrictions.length())
		{
			if (fitData.sRestrictions.front() == ',')
				fitData.sRestrictions.erase(0, 1);

			if (fitData.sRestrictions.back() == ',')
				fitData.sRestrictions.pop_back();

			_parser.SetExpr(fitData.sRestrictions);
			_parser.Eval();
		}
	}

	// The fitting parameter list including their possible
	// starting values
	if (!matchParams(fitData.sFitFunction, "params", '='))
	{
		fitData.bNoParams = true;
		fitData.sFitFunction = fitData.sFitFunction.substr(matchParams(fitData.sFitFunction, "with", '=') + 4);
		sCmd.erase(matchParams(sCmd, "with", '=') - 1);
	}
	else if (matchParams(fitData.sFitFunction, "with", '=') < matchParams(fitData.sFitFunction, "params", '='))
	{
		fitData.sParams = fitData.sFitFunction.substr(matchParams(fitData.sFitFunction, "params", '=') + 6);
		fitData.sFitFunction = fitData.sFitFunction.substr(matchParams(fitData.sFitFunction, "with", '=') + 4, matchParams(fitData.sFitFunction, "params", '=') - 1 - matchParams(fitData.sFitFunction, "with", '=') - 4);
		sCmd = sCmd.substr(0, matchParams(sCmd, "with", '=') - 1);
	}
	else
	{
		fitData.sParams = fitData.sFitFunction.substr(matchParams(fitData.sFitFunction, "params", '=') + 6, matchParams(fitData.sFitFunction, "with", '=') - 1 - matchParams(fitData.sFitFunction, "params", '=') - 6);
		fitData.sFitFunction = fitData.sFitFunction.substr(matchParams(fitData.sFitFunction, "with", '=') + 4);
		sCmd = sCmd.substr(0, matchParams(sCmd, "params", '=') - 1);
	}

	// Remove surrounding brackets from the parameter list
	if (fitData.sParams.find('[') != string::npos)
		fitData.sParams = fitData.sParams.substr(fitData.sParams.find('[') + 1);

	if (fitData.sParams.find(']') != string::npos)
		fitData.sParams = fitData.sParams.substr(0, fitData.sParams.find(']'));

	StripSpaces(fitData.sFitFunction);

	// Remove the possible trailing minus character
	if (fitData.sFitFunction.back() == '-')
	{
		fitData.sFitFunction.back() = ' ';
		StripSpaces(fitData.sFitFunction);
	}

	// Evaluate the defined paramters and their initial values
	if (!fitData.bNoParams)
	{
		StripSpaces(fitData.sParams);

		if (fitData.sParams.back() == '-')
		{
			fitData.sParams.back() = ' ';
			StripSpaces(fitData.sParams);
		}

		if (!_functions.call(fitData.sParams))
			throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd,fitData. sParams, fitData.sParams);

		if (fitData.sParams.find("data(") != string::npos || _data.containsTablesOrClusters(fitData.sParams))
		{
			getDataElements(fitData.sParams, _parser, _data, _option);
		}

		if (fitData.sParams.find("{") != string::npos && (containsStrings(fitData.sParams) || _data.containsStringVars(fitData.sParams)))
			parser_VectorToExpr(fitData.sParams, _option);
	}

	StripSpaces(sCmd);

	// Evaluate function definition calls
	if (!_functions.call(fitData.sFitFunction))
		throw SyntaxError(SyntaxError::FUNCTION_ERROR, sCmd, fitData.sFitFunction, fitData.sFitFunction);

    // Get values from a references data object
	if (fitData.sFitFunction.find("data(") != string::npos || _data.containsTablesOrClusters(fitData.sFitFunction))
	{
		getDataElements(fitData.sFitFunction, _parser, _data, _option);
	}

	// Expand remaining vectors
	if (fitData.sFitFunction.find("{") != string::npos)
		parser_VectorToExpr(fitData.sFitFunction, _option);

    size_t nPos = 0;

    // Ensure that the fitting function does not contain any of the
    // functions from the bad fitting functions list (a list of functions,
    // which cannot be used to solve the optimisation problem)
	while (sBADFUNCTIONS.find(',', nPos) != string::npos)
	{
		if (fitData.sFitFunction.find(sBADFUNCTIONS.substr(nPos, sBADFUNCTIONS.find(',', nPos) - nPos - 1)) != string::npos)
		{
			throw SyntaxError(SyntaxError::FUNCTION_CANNOT_BE_FITTED, sCmd, SyntaxError::invalid_position, sBADFUNCTIONS.substr(nPos, sBADFUNCTIONS.find(',', nPos) - nPos));
		}
		else
			nPos = sBADFUNCTIONS.find(',', nPos) + 1;

		if (nPos >= sBADFUNCTIONS.length())
			break;
	}

	return vInterVal;
}


// This static function returns a map of all
// fitting paramters used in the fitting algorithm
static mu::varmap_type getFittingParameters(FittingData& fitData, const mu::varmap_type& varMap, const string& sCmd)
{
    mu::varmap_type paramsMap;
    Parser& _parser = NumeReKernel::getInstance()->getParser();

    // If the user did not provide his own fitting parameters,
    // we'll create our own list depending on the found variables
    // in the fitting expression
    if (fitData.bNoParams)
	{
	    // Get the variable map
		paramsMap = varMap;

		// Remove x and y variables
		if (paramsMap.find("x") != paramsMap.end())
			paramsMap.erase(paramsMap.find("x"));

		if (paramsMap.find("y") != paramsMap.end())
			paramsMap.erase(paramsMap.find("y"));

		if (!paramsMap.size())
			throw SyntaxError(SyntaxError::NO_PARAMS_FOR_FIT, sCmd, SyntaxError::invalid_position);
	}
	else
	{
	    // Evaluate the fitting parameters provided
	    // by the user
		_parser.SetExpr(fitData.sParams);
		_parser.Eval();

		// Remove the values from the expression, to avoid
		// that they would be mis-identified as fitting
		// parameters
		if (fitData.sParams.find('=') != string::npos)
		{
			for (size_t i = 0; i < fitData.sParams.length(); i++)
			{
				if (fitData.sParams[i] == '=')
				{
					for (size_t j = i; j < fitData.sParams.length(); j++)
					{
						if (fitData.sParams[j] == '(')
							j += getMatchingParenthesis(fitData.sParams.substr(j));

						if (fitData.sParams[j] == ',')
						{
							fitData.sParams.erase(i, j - i);
							break;
						}

						if (j == fitData.sParams.length() - 1)
							fitData.sParams.erase(i);
					}
				}
			}

			_parser.SetExpr(fitData.sParams);
			_parser.Eval();
		}

		paramsMap = _parser.GetUsedVar();
	}

	return paramsMap;
}


// This static function obtains the data needed for
// the fitting algorithm and stores it in the vector
// members in the FittingData object
static int getDataForFit(const string& sCmd, string& sDimsForFitLog, FittingData& fitData)
{
    Datafile& _data = NumeReKernel::getInstance()->getData();
	vector<double> vTempZ;
    string sDataTable = "";
    Indices _idx;
    int nColumns = 0;
    bool openEnd = false;
    bool isCluster = false;
    sDataTable = "data";
    int nDim = 0;

    _idx = getIndicesForPlotAndFit(sCmd, sDataTable, nColumns, openEnd, isCluster);


    if (_idx.row.isOpenEnd())
    {
        if (!isCluster)
            _idx.row.setRange(0, _data.getLines(sDataTable, false)-1);
        else
            _idx.row.setRange(0, _data.getCluster(sDataTable).size()-1);
    }

    if (!isCluster && _idx.col.isOpenEnd())
    {
        _idx.col.setRange(0, _data.getCols(sDataTable, false)-1);
    }

    if (!isCluster)
    {
        if (_idx.row.back() > _data.getLines(sDataTable, false))
            _idx.row.setRange(0, _data.getLines(sDataTable, false)-1);

        if (_idx.col.back() > _data.getCols(sDataTable))
            _idx.col.setRange(0, _data.getCols(sDataTable)-1);
    }
    else
    {
        if (_idx.row.back() > _data.getCluster(sDataTable).size())
            _idx.row.setRange(0, _data.getCluster(sDataTable).size()-1);
    }

	/* --> Bestimmen wir die "Dimension" des zu fittenden Datensatzes. Dabei ist es auch
	 *     von Bedeutung, ob Fehlerwerte verwendet werden sollen <--
	 */
	nDim = 0;

	if (nColumns == 1 && fitData.bUseErrors && _idx.col.size() < 3)
		throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, SyntaxError::invalid_position);

	if (nColumns == 1 && !_idx.col.size())
		nDim = 2;
	else if (nColumns == 1)
		nDim = _idx.col.size();
	else if (nColumns == 2)
	{
		if (!fitData.bUseErrors)
		{
			if (!(fitData.nFitVars & 2))
			{
				nDim = 2;

				if (_idx.col.size() < 2)
					throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, SyntaxError::invalid_position);
			}
			else
			{
				nDim = 3;

				if (_idx.col.size() < _idx.row.size() + 2)
					throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, SyntaxError::invalid_position);
			}
		}
		else
		{
			if (!(fitData.nFitVars & 2))
			{
				nDim = 4;

				if (_idx.col.size() < 3)
					throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, SyntaxError::invalid_position);
			}
			else
			{
				nDim = 5;

				if (_idx.col.size() < 3 * _idx.row.size() + 2)
					throw SyntaxError(SyntaxError::TOO_FEW_COLS, sCmd, SyntaxError::invalid_position);
			}
		}
	}
	else
	{
		nDim = _idx.col.size();
	}

	if (isnan(fitData.dMin))
	{
		fitData.dMin = _data.min(sDataTable, _idx.row, VectorIndex(_idx.col.front()));
	}

	if (isnan(fitData.dMax))
	{
		fitData.dMax = _data.max(sDataTable, _idx.row, VectorIndex(_idx.col.front()));
	}

	if (fitData.dMax < fitData.dMin)
	{
		double dTemp = fitData.dMax;
		fitData.dMax = fitData.dMin;
		fitData.dMin = dTemp;
	}

	if (fitData.nFitVars & 2 && !isCluster)
	{
		if (isnan(fitData.dMinY))
		{
            fitData.dMinY = _data.min(sDataTable, _idx.row, VectorIndex(_idx.col[1]));
		}

		if (isnan(fitData.dMaxY))
		{
			fitData.dMaxY = _data.max(sDataTable, _idx.row, VectorIndex(_idx.col[1]));
		}

		if (fitData.dMaxY < fitData.dMinY)
		{
			double dTemp = fitData.dMaxY;
			fitData.dMaxY = fitData.dMinY;
			fitData.dMinY = dTemp;
		}
	}

	if (nDim == 2 || isCluster)
	{
        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            if (nColumns == 1)
            {
                if (isValidValue(getDataFromObject(sDataTable, _idx.row[i], _idx.col[0], isCluster)))
                {
                    fitData.vx.push_back(_idx.row[i] + 1);
                    fitData.vy.push_back(getDataFromObject(sDataTable, _idx.row[i], _idx.col[0], isCluster));
                }
            }
            else
            {
                if (_data.isValidEntry(_idx.row[i], _idx.col[0], sDataTable) && _data.isValidEntry(_idx.row[i], _idx.col[1], sDataTable))
                {
                    if (!isnan(fitData.dMin) && !isnan(fitData.dMax) && (_data.getElement(_idx.row[i], _idx.col[0], sDataTable) < fitData.dMin || _data.getElement(_idx.row[i], _idx.col[0], sDataTable) > fitData.dMax))
                        continue;

                    fitData.vx.push_back(_data.getElement(_idx.row[i], _idx.col[0], sDataTable));
                    fitData.vy.push_back(_data.getElement(_idx.row[i], _idx.col[1], sDataTable));
                }
            }
        }
	}
	else if (!isCluster && nDim == 4)
	{
        int nErrorCols = 2;

        if (nColumns == 2)
        {
            if (abs(_idx.col[1] - _idx.col[0]) == 2)
                nErrorCols = 1;
        }
        else if (nColumns == 4)
            nErrorCols = 2;

        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            if (nColumns == 2)
            {
                if (_data.isValidEntry(_idx.row[i], _idx.col[0], sDataTable) && _data.isValidEntry(_idx.row[i], _idx.col[1], sDataTable))
                {
                    if (!isnan(fitData.dMin) && !isnan(fitData.dMax) && (_data.getElement(_idx.row[i], _idx.col[0], sDataTable) < fitData.dMin || _data.getElement(_idx.row[i], _idx.col[0], sDataTable) > fitData.dMax))
                        continue;

                    fitData.vx.push_back(_data.getElement(_idx.row[i], _idx.col[0], sDataTable));
                    fitData.vy.push_back(_data.getElement(_idx.row[i], _idx.col[1], sDataTable));

                    if (nErrorCols == 1)
                    {
                        if (_data.isValidEntry(_idx.row[i], _idx.col[2], sDataTable))
                            fitData.vy_w.push_back(fabs(_data.getElement(_idx.row[i], _idx.col[2], sDataTable)));
                        else
                            fitData.vy_w.push_back(0.0);
                    }
                    else
                    {
                        if (_data.isValidEntry(_idx.row[i], _idx.col[2], sDataTable) && _data.isValidEntry(_idx.row[i], _idx.col[3], sDataTable) && (_data.getElement(_idx.row[i], _idx.col[2], sDataTable) && _data.getElement(_idx.row[i], _idx.col[3], sDataTable)))
                            fitData.vy_w.push_back(sqrt(fabs(_data.getElement(_idx.row[i], _idx.col[2], sDataTable)) * fabs(_data.getElement(_idx.row[i], _idx.col[3], sDataTable))));
                        else if (_data.isValidEntry(_idx.row[i], _idx.col[2], sDataTable) && _data.getElement(_idx.row[i], _idx.col[2], sDataTable))
                            fitData.vy_w.push_back(fabs(_data.getElement(_idx.row[i], _idx.col[2], sDataTable)));
                        else if (_data.isValidEntry(_idx.row[i], _idx.col[3], sDataTable) && _data.getElement(_idx.row[i], _idx.col[3], sDataTable))
                            fitData.vy_w.push_back(fabs(_data.getElement(_idx.row[i], _idx.col[3], sDataTable)));
                        else
                            fitData.vy_w.push_back(0.0);
                    }
                }
            }
            else
            {
                if (_data.isValidEntry(_idx.row[i], _idx.col[0], sDataTable) && _data.isValidEntry(_idx.row[i], _idx.col[1], sDataTable))
                {
                    if (!isnan(fitData.dMin) && !isnan(fitData.dMax) && (_data.getElement(_idx.row[i], _idx.col[0], sDataTable) < fitData.dMin || _data.getElement(_idx.row[i], _idx.col[0], sDataTable) > fitData.dMax))
                        continue;

                    fitData.vx.push_back(_data.getElement(_idx.row[i], _idx.col[0], sDataTable));
                    fitData.vy.push_back(_data.getElement(_idx.row[i], _idx.col[1], sDataTable));

                    if (_data.isValidEntry(_idx.row[i], _idx.col[2], sDataTable) && _data.isValidEntry(_idx.row[i], _idx.col[3], sDataTable) && (_data.getElement(_idx.row[i], _idx.col[2], sDataTable) && _data.getElement(_idx.row[i], _idx.col[3], sDataTable)))
                        fitData.vy_w.push_back(sqrt(fabs(_data.getElement(_idx.row[i], _idx.col[2], sDataTable)) * fabs(_data.getElement(i, _idx.col[3], sDataTable))));
                    else if (_data.isValidEntry(_idx.row[i], _idx.col[2], sDataTable) && _data.getElement(_idx.row[i], _idx.col[2], sDataTable))
                        fitData.vy_w.push_back(fabs(_data.getElement(_idx.row[i], _idx.col[2], sDataTable)));
                    else if (_data.isValidEntry(_idx.row[i], _idx.col[3], sDataTable) && _data.getElement(_idx.row[i], _idx.col[3], sDataTable))
                        fitData.vy_w.push_back(fabs(_data.getElement(_idx.row[i], _idx.col[3], sDataTable)));
                    else
                        fitData.vy_w.push_back(0.0);
                }
            }
        }
	}
	else if (!isCluster && (fitData.nFitVars & 2))
	{
        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            if (!_data.isValidEntry(_idx.row[i], _idx.col[1], sDataTable) || _data.getElement(_idx.row[i], _idx.col[1], sDataTable) < fitData.dMinY || _data.getElement(_idx.row[i], _idx.col[1], sDataTable) > fitData.dMaxY)
                continue;
            else
                fitData.vy.push_back(_data.getElement(_idx.row[i], _idx.col[1], sDataTable));

            if (!_data.isValidEntry(_idx.row[i], _idx.col[0], sDataTable) || _data.getElement(_idx.row[i], _idx.col[0], sDataTable) < fitData.dMin || _data.getElement(_idx.row[i], _idx.col[0], sDataTable) > fitData.dMax)
                continue;
            else
                fitData.vx.push_back(_data.getElement(_idx.row[i], _idx.col[0], sDataTable));

            for (size_t k = 2; k < _idx.col.size(); k++)
            {
                if (nColumns > 3 && k == _idx.row.size() + 2)
                    break;

                if (!_data.isValidEntry(_idx.row[k], _idx.col[1], sDataTable)
                        || _data.getElement(_idx.row[k], _idx.col[1], sDataTable) < fitData.dMinY
                        || _data.getElement(_idx.row[k], _idx.col[1], sDataTable) > fitData.dMaxY)
                    continue;
                else
                    vTempZ.push_back(_data.getElement(_idx.row[i], _idx.col[k], sDataTable));
            }

            fitData.vz.push_back(vTempZ);
            vTempZ.clear();

            if (fitData.vy_w.size() && fitData.bUseErrors)
            {
                fitData.vz_w.push_back(fitData.vy_w);
                fitData.vy_w.clear();
            }
        }
	}
	else
	{
        for (unsigned int i = 0; i < _idx.row.size(); i++)
        {
            if (_data.isValidEntry(_idx.row[i], _idx.col[0], sDataTable) && _data.isValidEntry(_idx.row[i], _idx.col[1], sDataTable))
            {
                if (!isnan(fitData.dMin) && !isnan(fitData.dMax) && (_data.getElement(_idx.row[i], _idx.col[0], sDataTable) < fitData.dMin || _data.getElement(_idx.row[i], _idx.col[0], sDataTable) > fitData.dMax))
                    continue;

                fitData.vx.push_back(_data.getElement(_idx.row[i], _idx.col[0], sDataTable));
                fitData.vy.push_back(_data.getElement(_idx.row[i], _idx.col[1], sDataTable));

                if (_data.isValidEntry(_idx.row[i], _idx.col[2], sDataTable))
                    fitData.vy_w.push_back(fabs(_data.getElement(_idx.row[i], _idx.col[2], sDataTable)));
                else
                    fitData.vy_w.push_back(0.0);
            }
        }
	}

	sDimsForFitLog.clear();

	if (nDim == 2)
	{
		sDimsForFitLog += toString(_idx.col.front()+1);

		if (nColumns == 2)
		{
			sDimsForFitLog += ", " + toString(_idx.col.last() + 1);
		}
	}
	else if (nDim == 4)
	{
		int nErrorCols = 2;

		if (nColumns == 2)
		{
			if (_idx.col.size() == 3)
				nErrorCols = 1;
		}
		else if (nColumns == 4)
			nErrorCols = 2;

		if (nColumns == 2)
		{
            sDimsForFitLog += toString(_idx.col[0] + 1) + ", " + toString(_idx.col[1] + 1) + ", " + toString(_idx.col[2] + 1);

            if (nErrorCols == 2)
                sDimsForFitLog + ", " + toString(_idx.col[3] + 1);
		}
		else
		{
			sDimsForFitLog += toString(_idx.col[0] + 1) + ", " + toString(_idx.col[1] + 1) + ", " + toString(_idx.col[2] + 1) + ", " + toString(_idx.col[3] + 1);
		}
	}
	else if ((fitData.nFitVars & 2))
	{
		if (nColumns == 2)
		{
			sDimsForFitLog += toString(_idx.col[0] + 1) + ", " + toString(_idx.col[1] + 1) + ", " + toString(_idx.col[2] + 1) + "-" + toString(_idx.col.last()+1);

			if (fitData.bUseErrors)
				sDimsForFitLog += ", " + toString(_idx.col[2] + 2 + _idx.row.size()) + "-" + toString(_idx.col[2] + 2 + 2 * _idx.row.size());
		}
		else
		{
			sDimsForFitLog += toString(_idx.col[0] + 1) + ", " + toString(_idx.col[1] + 1) + ", " + toString(_idx.col[2] + 1) + "-" + toString(_idx.col[2] + _idx.row.size());

			if (fitData.bUseErrors)
			{
				if (nColumns > 3)
					sDimsForFitLog += ", " + toString(_idx.col[3] + 1) + "-" + toString(_idx.col[3] + _idx.row.size());
				else
					sDimsForFitLog += ", " + toString(_idx.col[2] + _idx.row.size() + 1) + "-" + toString(_idx.col[0] + 2 * (_idx.row.size()));
			}
		}
	}
	else
	{
		for (int k = 0; k < (int)nDim; k++)
		{
			sDimsForFitLog += toString(_idx.col[k] + 1);

			if (k + 1 < (int)nDim)
				sDimsForFitLog += ", ";
		}
	}

	sDimsForFitLog += " " + _lang.get("PARSERFUNCS_FIT_FROM") + " " + _data.getDataFileName(sDataTable);

    return nDim;
}


// This static function removes obsolete surrounding
// parentheses in function strings
static void removeObsoleteParentheses(string& sFunction)
{
    StripSpaces(sFunction);

    // As long as the first character is an opening
    // parenthesis, search the closing one and remove both,
    // if it is the last character of the function
	while (sFunction.front() == '(')
	{
		if (getMatchingParenthesis(sFunction) == sFunction.length() - 1 && getMatchingParenthesis(sFunction) != string::npos)
		{
			sFunction.erase(0, 1);
			sFunction.pop_back();
			StripSpaces(sFunction);
		}
		else
			break;
	}
}


// This static function calculates the chi^2 map instead
// of applying the single fit
static bool calculateChiMap(string sFunctionDefString, const string& sFuncDisplay, Indices& _idx, mu::varmap_type& varMap, mu::varmap_type& paramsMap, FittingData& fitData, vector<double> vInitialVals)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Datafile& _data = NumeReKernel::getInstance()->getData();
    Define& _functions = NumeReKernel::getInstance()->getDefinitions();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    Fitcontroller _fControl(&_parser);

    for (size_t i = 0; i < _idx.row.size(); i++)
    {
        for (size_t j = 0; j <= (_idx.row.size() - 1) * (!fitData.b1DChiMap); j++)
        {
            auto iter = paramsMap.begin();

            for (unsigned int n = 0; n < vInitialVals.size(); n++)
            {
                *(iter->second) = vInitialVals[n];
                ++iter;
            }

            if (!_data.isValidEntry(_idx.row[i], _idx.col[0], fitData.sChiMap))
                continue;

            *(varMap.at(fitData.sChiMap_Vars[0])) = _data.getElement(_idx.row[i], _idx.col[0], fitData.sChiMap);

            if (!fitData.b1DChiMap)
            {
                if (!_data.isValidEntry(_idx.row[j], _idx.col[1], fitData.sChiMap))
                    continue;

                *(varMap.at(fitData.sChiMap_Vars[1])) = _data.getElement(_idx.row[j], _idx.col[1], fitData.sChiMap);
            }

            if (fitData.nDim >= 2 && fitData.nFitVars == 1)
            {
                if (!fitData.bUseErrors)
                {
                    if (!_fControl.fit(fitData.vx, fitData.vy, fitData.sFitFunction, fitData.sRestrictions, paramsMap, fitData.dPrecision, fitData.nMaxIterations))
                    {
                        if (_option.getSystemPrintStatus())
                            NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");

                        return false;
                    }

                    sFunctionDefString = "Fit(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
                }
                else
                {
                    if (!_fControl.fit(fitData.vx, fitData.vy, fitData.vy_w, fitData.sFitFunction, fitData.sRestrictions, paramsMap, fitData.dPrecision, fitData.nMaxIterations))
                    {
                        if (_option.getSystemPrintStatus())
                            NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");

                        return false;
                    }

                    sFunctionDefString = "Fitw(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
                }
            }
            else if (fitData.nDim == 3)
            {
                if (!_fControl.fit(fitData.vx, fitData.vy, fitData.vz, fitData.sFitFunction, fitData.sRestrictions, paramsMap, fitData.dPrecision, fitData.nMaxIterations))
                {
                    if (_option.getSystemPrintStatus())
                        NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");

                    return false;
                }

                sFunctionDefString = "Fit(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
            }
            else if (fitData.nDim == 5)
            {
                if (!_fControl.fit(fitData.vx, fitData.vy, fitData.vz, fitData.vz_w, fitData.sFitFunction, fitData.sRestrictions, paramsMap, fitData.dPrecision, fitData.nMaxIterations))
                {
                    if (_option.getSystemPrintStatus())
                        NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");

                    return false;
                }

                sFunctionDefString = "Fitw(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
            }

            _data.writeToCache(_idx.row[i], _idx.col[1 + (!fitData.b1DChiMap) * (j + 1)], fitData.sChiMap, _fControl.getFitChi());

            if (!i && !fitData.b1DChiMap)
                _data.setHeadLineElement(_idx.col[1 + (!fitData.b1DChiMap) * (j + 1)], fitData.sChiMap, "chi^2[" + toString(j + 1) + "]");
            else if (!i)
                _data.setHeadLineElement(_idx.col[1 + (!fitData.b1DChiMap) * (j + 1)], fitData.sChiMap, "chi^2");

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
        NumeReKernel::print(LineBreak(_lang.get("PARSERFUNCS_FIT_CHIMAPLOCATION", fitData.sChiMap), _option));
    }

    bool bDefinitionSuccess = false;

    if (!_functions.isDefined(sFunctionDefString))
        bDefinitionSuccess = _functions.defineFunc(sFunctionDefString);
    else if (_functions.getDefine(_functions.getFunctionIndex(sFunctionDefString)) != sFunctionDefString)
        bDefinitionSuccess = _functions.defineFunc(sFunctionDefString, true);
    else if (_functions.getDefine(_functions.getFunctionIndex(sFunctionDefString)) == sFunctionDefString)
        return true;

    if (bDefinitionSuccess)
        NumeReKernel::print(_lang.get("DEFINE_SUCCESS"), _option.getSystemPrintStatus());
    else
        NumeReKernel::issueWarning(_lang.get("DEFINE_FAILURE"));

    return true;

}


// This static function applies the fitting algorithm to
// the obtained data and the prepared fitting function. It
// returns the function definition string for the autmatically
// created fitting function
static string applyFitAlgorithm(Fitcontroller& _fControl, FittingData& fitData, mu::varmap_type& paramsMap, const string& sFuncDisplay)
{
    string sFunctionDefString;
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (fitData.nDim >= 2 && fitData.nFitVars == 1)
	{
		if (!fitData.bUseErrors)
		{
			if (!_fControl.fit(fitData.vx, fitData.vy, fitData.sFitFunction, fitData.sRestrictions, paramsMap, fitData.dPrecision, fitData.nMaxIterations))
			{
				if (_option.getSystemPrintStatus())
					NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");

				return false;
			}

			sFunctionDefString = "Fit(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
		}
		else
		{
			if (!_fControl.fit(fitData.vx, fitData.vy, fitData.vy_w, fitData.sFitFunction, fitData.sRestrictions, paramsMap, fitData.dPrecision, fitData.nMaxIterations))
			{
				if (_option.getSystemPrintStatus())
					NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");

				return false;
			}

			sFunctionDefString = "Fitw(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
		}
	}
	else if (fitData.nDim == 3)
	{
		if (!_fControl.fit(fitData.vx, fitData.vy, fitData.vz, fitData.sFitFunction, fitData.sRestrictions, paramsMap, fitData.dPrecision, fitData.nMaxIterations))
		{
			if (_option.getSystemPrintStatus())
				NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");

			return false;
		}

		sFunctionDefString = "Fit(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
	}
	else if (fitData.nDim == 5)
	{
		if (!_fControl.fit(fitData.vx, fitData.vy, fitData.vz, fitData.vz_w, fitData.sFitFunction, fitData.sRestrictions, paramsMap, fitData.dPrecision, fitData.nMaxIterations))
		{
			if (_option.getSystemPrintStatus())
				NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");

			return false;
		}

		sFunctionDefString = "Fitw(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
	}

	return sFunctionDefString;
}


// This static function multiplies the elements in the covariance
// matrix with the "correct" reduced chi^2 value (this depends
// upon whether fitting weights shall be used for fitting)
static void calculateCovarianceData(FittingData& fitData, double dChisq, size_t paramsMapSize)
{
    // Do nothing, if fitting weights have been used
    if (fitData.bUseErrors)
        return;

    double dSize = (fitData.vz.size()) ? (fitData.vz.size() * fitData.vz[0].size()) : fitData.vx.size();
    double dFactor = dChisq;

    // Calculate the factor depending on the number of fitting dimensions
    if (!(fitData.nFitVars & 2))
        dFactor /= dSize - paramsMapSize;
    else
        dFactor /= dSize*dSize - paramsMapSize;

    // SCale all elements in the covariance matrix with the
    // calculated factor
    for (unsigned int i = 0; i < fitData.vz_w.size(); i++)
    {
        for (unsigned int j = 0; j < fitData.vz_w[0].size(); j++)
        {
            fitData.vz_w[i][j] *= dFactor;
        }
    }

}


// This static function returns a string containing the whole fitting
// options (algorithm parameters, etc.)
static string getFitOptionsTable(Fitcontroller& _fControl, FittingData& fitData, const string& sFuncDisplay, const string& sFittedFunction, const string& sDimsForFitLog, double dChisq, const mu::varmap_type& paramsMap, size_t nSize, bool forFitLog)
{
    string sFitParameterTable;

    if (forFitLog)
    {
        sFitParameterTable += _lang.get("PARSERFUNCS_FIT_FUNCTION", sFuncDisplay) + "\n";
        sFitParameterTable += _lang.get("PARSERFUNCS_FIT_FITTED_FUNC", sFittedFunction) + "\n";
        sFitParameterTable += _lang.get("PARSERFUNCS_FIT_DATASET") + " " + sDimsForFitLog + "\n";
    }
    else
    {
        sFitParameterTable += _lang.get("PARSERFUNCS_FIT_FUNCTION", sFittedFunction) + "\n";
    }

	if (fitData.bUseErrors)
		sFitParameterTable += _lang.get("PARSERFUNCS_FIT_POINTS_W_ERR", toString((int)nSize)) + "\n";
	else
		sFitParameterTable += _lang.get("PARSERFUNCS_FIT_POINTS_WO_ERR", toString((int)nSize)) + "\n";

	if (fitData.bRestrictXVals)
		sFitParameterTable += _lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "x", toString(fitData.dMin, 5), toString(fitData.dMax, 5)) + "\n";

	if (fitData.bRestrictYVals)
		sFitParameterTable += _lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "y", toString(fitData.dMinY, 5), toString(fitData.dMaxY, 5)) + "\n";

	if (fitData.sRestrictions.length())
		sFitParameterTable += _lang.get("PARSERFUNCS_FIT_PARAM_RESTRICTS", fitData.sRestrictions) + "\n";

	sFitParameterTable += _lang.get("PARSERFUNCS_FIT_FREEDOMS", toString((int)nSize - paramsMap.size())) + "\n";
	sFitParameterTable += _lang.get("PARSERFUNCS_FIT_ALGORITHM_SETTINGS", toString(fitData.dPrecision, 5), toString(fitData.nMaxIterations)) + "\n";
	sFitParameterTable += _lang.get("PARSERFUNCS_FIT_ITERATIONS", toString(_fControl.getIterations())) + "\n";

	if (nSize != paramsMap.size() && !(fitData.nFitVars & 2))
	{
		sFitParameterTable += _lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7)) + "\n";
		sFitParameterTable += _lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7)) + "\n";
		sFitParameterTable += _lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7)) + "\n";
	}
	else if (fitData.nFitVars & 2 && nSize != paramsMap.size() )
	{
		sFitParameterTable += _lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7)) + "\n";
		sFitParameterTable += _lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7)) + "\n";
		sFitParameterTable += _lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7)) + "\n";
	}

	return sFitParameterTable;
}


// This static function returns the layouted covariance matrix
// as a string for printing in file and to terminal
static string constructCovarianceMatrix(FittingData& fitData, size_t paramsMapSize, bool forFitLog)
{
    string sCovMatrix;

    // Construct the whole matrix as a string
    for (unsigned int n = 0; n < paramsMapSize; n++)
    {
        // The terminal requires some indentation
        if (!forFitLog)
            sCovMatrix += "|   ";

        // Append the part of the opening parenthesis
        if (!n)
            sCovMatrix += "/";
        else if (n + 1 == paramsMapSize)
            sCovMatrix += "\\";
        else
            sCovMatrix += "|";

        // Write the current matrix line
        for (unsigned int k = 0; k < paramsMapSize; k++)
        {
            sCovMatrix += " " + strfill(toString(fitData.vz_w[n][k] / sqrt(fabs(fitData.vz_w[n][n] * fitData.vz_w[k][k])), 3), 10);
        }

        // Append the part of the closing parenthesis
        if (!n)
            sCovMatrix += " \\\n";
        else if (n + 1 == paramsMapSize)
            sCovMatrix += " /\n";
        else
            sCovMatrix += " |\n";
    }

    return sCovMatrix;
}


// This static function returns the fitting paramters including
// their initial and final values as a string
static string getParameterTable(FittingData& fitData, mu::varmap_type& paramsMap, const vector<double>& vInitialVals, size_t windowSize, const string& sPMSign, bool forFitLog)
{
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    auto pItem = paramsMap.begin();
    string sParameterTable;

    // Construct the fitting parameter table as a single string
    for (size_t n = 0; n < paramsMap.size(); n++)
	{
		if (pItem == paramsMap.end())
			break;

        // The terminal requires some indentation
        if (!forFitLog)
            sParameterTable += "|   ";

        // Write the single fitting parameter line including
        // parameter name, initial and final value and errors
        sParameterTable += pItem->first + "    "
            + strfill(toString(vInitialVals[n], _option), (windowSize - 32) / 2 + windowSize % 2 - pItem->first.length())
            + strfill(toString(*(pItem->second), _option), (windowSize - 50) / 2)
            + strfill(sPMSign + " " + toString(sqrt(abs(fitData.vz_w[n][n])), 5), 16);

        // Append the percentage of error compared to the final
        // value if the final value does exist
        if (fitData.vz_w[n][n])
            sParameterTable += " " + strfill("(" + toString(abs(sqrt(abs(fitData.vz_w[n][n] / (*(pItem->second)))) * 100.0), 4) + "%)", 16) + "\n";
        else
            sParameterTable += "\n";

		++pItem;
	}

	return sParameterTable;
}


// This static function calculates the average of the percentual
// fitting errors and creates the variables for storing the
// fitting errors for each parameter for further calculations
static double calculatePercentageAvgAndCreateParserVariables(FittingData& fitData, mu::varmap_type& paramsMap, double dChisq)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    string sErrors = "";
	auto pItem = paramsMap.begin();
	double dAverageErrorPercentage = 0.0;

	// Go through all fitting parameters and summarize the
	// percentual errors and construct error variables, if the
	// user required them
	for (unsigned int n = 0; n < paramsMap.size(); n++)
	{
		if (pItem == paramsMap.end())
			break;

        // Add the percentage value
		if (fitData.vz_w[n][n])
		{
			dAverageErrorPercentage += abs(sqrt(abs(fitData.vz_w[n][n] / (*(pItem->second)))) * 100.0);
		}

		// Add a constructed variable containing the error value
		if (fitData.bSaveErrors)
		{
			sErrors += pItem->first + "_error = " + toCmdString(sqrt(abs(fitData.vz_w[n][n]))) + ",";
		}

		++pItem;
	}

	// Devide the sum to obtain the average value
	dAverageErrorPercentage /= (double)paramsMap.size();

	// Create the error variables and the chi variable
	if (fitData.bSaveErrors)
	{
		sErrors.pop_back();
		_parser.SetExpr(sErrors);
		_parser.Eval();
	}

	_parser.SetExpr("chi = " + toCmdString(sqrt(dChisq)));
	_parser.Eval();

	return dAverageErrorPercentage;
}


// This static function returns the fitting analysis depending
// upon the chi^2 value and the sum of the percentual errors
static string getFitAnalysis(Fitcontroller& _fControl, FittingData& fitData, double dNormChisq, double dAverageErrorPercentage, bool noOverfitting)
{
    if (fitData.nFitVars & 2)
		dNormChisq = sqrt(dNormChisq);

    if (_fControl.getIterations() == fitData.nMaxIterations)
	{
		return _lang.get("PARSERFUNCS_FIT_MAXITER_REACHED");
	}
	else
	{
		if (noOverfitting)
		{
			if (fitData.bUseErrors)
			{
				if (log10(dNormChisq) > -1.0 && log10(dNormChisq) < 0.5 && dAverageErrorPercentage < 50.0)
					return _lang.get("PARSERFUNCS_FIT_GOOD_W_ERROR");
				else if (log10(dNormChisq) <= -1.0 && dAverageErrorPercentage < 20.0)
					return _lang.get("PARSERFUNCS_FIT_BETTER_W_ERROR");
				else if (log10(dNormChisq) >= 0.5 && log10(dNormChisq) < 1.5 && dAverageErrorPercentage < 100.0)
					return _lang.get("PARSERFUNCS_FIT_NOT_GOOD_W_ERROR");
				else
					return _lang.get("PARSERFUNCS_FIT_BAD_W_ERROR");
			}
			else
			{
				if (log10(dNormChisq) < -3.0 && dAverageErrorPercentage < 20.0)
					return _lang.get("PARSERFUNCS_FIT_GOOD_WO_ERROR");
				else if (log10(dNormChisq) < 0.0 && dAverageErrorPercentage < 50.0)
					return _lang.get("PARSERFUNCS_FIT_IMPROVABLE_WO_ERROR");
				else if (log10(dNormChisq) >= 0.0 && log10(dNormChisq) < 0.5 && dAverageErrorPercentage < 100.0)
					return _lang.get("PARSERFUNCS_FIT_NOT_GOOD_WO_ERROR");
				else
					return _lang.get("PARSERFUNCS_FIT_BAD_WO_ERROR");
			}
		}
		else
		{
			return _lang.get("PARSERFUNCS_FIT_OVERFITTING");
		}
	}

	return "";
}


// This static function writes the contents of the logfile to
// a TeX file, if the used requested this option
static void createTeXExport(Fitcontroller& _fControl, const string& sTeXExportFile, const string& sCmd, mu::varmap_type& paramsMap, FittingData& fitData, const vector<double>& vInitialVals, size_t nSize, const string& sFitAnalysis, const string& sFuncDisplay, const string& sFittedFunction, double dChisq)
{
    ofstream oTeXExport;

    oTeXExport.open(sTeXExportFile.c_str(), ios_base::trunc);

    // Ensure that the file stream can be opened
    if (oTeXExport.fail())
    {
        oTeXExport.close();
        NumeReKernel::printPreFmt("\n");
        throw SyntaxError(SyntaxError::CANNOT_OPEN_TARGET, sCmd, SyntaxError::invalid_position, sTeXExportFile);
    }

    // Write the headline to the TeX file
    oTeXExport << "%\n% " << _lang.get("OUTPUT_PRINTLEGAL_TEX") << "\n%" << endl;
    oTeXExport << "\\section{" << _lang.get("PARSERFUNCS_FIT_HEADLINE") << ": " << getTimeStamp(false)  << "}" << endl;
    oTeXExport << "\\begin{itemize}" << endl;
    oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_FUNCTION", "$" + replaceToTeX(sFuncDisplay, true) + "$") << endl;
    oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_FITTED_FUNC", "$" + replaceToTeX(sFittedFunction, true) + "$") << endl;

    if (fitData.bUseErrors)
        oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_POINTS_W_ERR", toString((int)nSize)) << endl;
    else
        oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_POINTS_WO_ERR", toString((int)nSize)) << endl;

    if (fitData.bRestrictXVals)
        oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "x", toString(fitData.dMin, 5), toString(fitData.dMax, 5)) << endl;

    if (fitData.bRestrictYVals)
        oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "y", toString(fitData.dMinY, 5), toString(fitData.dMaxY, 5)) << endl;

    if (fitData.sRestrictions.length())
        oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_PARAM_RESTRICTS", "$" + replaceToTeX(fitData.sRestrictions, true) + "$") << endl;

    oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_FREEDOMS", toString((int)nSize - paramsMap.size())) << endl;
    oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_ALGORITHM_SETTINGS", toString(fitData.dPrecision, 5), toString(fitData.nMaxIterations)) << endl;
    oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_ITERATIONS", toString(_fControl.getIterations())) << endl;

    // Write the calculated fitting result values to the TeX file
    if (nSize != paramsMap.size() && !(fitData.nFitVars & 2))
    {
        string sChiReplace = _lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7));
        sChiReplace.replace(sChiReplace.find("chi^2"), 5, "$\\chi^2$");
        oTeXExport << "\t\\item " << sChiReplace << endl;
        sChiReplace = _lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7));
        sChiReplace.replace(sChiReplace.find("chi^2"), 5, "$\\chi^2$");
        oTeXExport << "\t\\item " << sChiReplace << endl;
        oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7)) << endl;
    }
    else if (fitData.nFitVars & 2 && nSize != paramsMap.size())
    {
        string sChiReplace = _lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7));
        sChiReplace.replace(sChiReplace.find("chi^2"), 5, "$\\chi^2$");
        oTeXExport << "\t\\item " << sChiReplace << endl;
        sChiReplace = _lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7));
        sChiReplace.replace(sChiReplace.find("chi^2"), 5, "$\\chi^2$");
        oTeXExport << "\t\\item " << sChiReplace << endl;
        oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7)) << endl;
    }

    // Start the table for the fitting parameters
    oTeXExport << "\\end{itemize}" << endl << "\\begin{table}[htb]" << endl << "\t\\centering\n\t\\begin{tabular}{cccc}" << endl << "\t\t\\toprule" << endl;

    // Write the headline for the fitting parameters
    if (fitData.bUseErrors)
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

	auto pItem = paramsMap.begin();
	string sErrors = "";
	string sPMSign = " ";

	// Write the fitting parameters linewise to the table
	for (unsigned int n = 0; n < paramsMap.size(); n++)
	{
		if (pItem == paramsMap.end())
			break;

        oTeXExport << "\t\t$" <<  replaceToTeX(pItem->first, true) << "$ & $"
                   << vInitialVals[n] << "$ & $"
                   << *(pItem->second) << "$ & $\\pm"
                   << sqrt(abs(fitData.vz_w[n][n]));

        // Append the percentual error value, if the current parameter
        // is non-zero.
        if (fitData.vz_w[n][n])
        {
            oTeXExport << " \\quad (" + toString(abs(sqrt(abs(fitData.vz_w[n][n] / (*(pItem->second)))) * 100.0), 4) + "\\%)$\\\\" << endl;
        }
        else
            oTeXExport << "$\\\\" << endl;

		++pItem;
	}

	// Close the fitting parameter table
    oTeXExport << "\t\t\\bottomrule" << endl << "\t\\end{tabular}" << endl << "\\end{table}" << endl;

    // Write the correlation matrix
	if (paramsMap.size() > 1 && paramsMap.size() != nSize)
	{
        oTeXExport << endl << "\\subsection{" << _lang.get("PARSERFUNCS_FIT_CORRELMAT_HEAD") << "}" << endl;
        oTeXExport << "\\[" << endl << "\t\\begin{pmatrix}" << endl;

        for (unsigned int n = 0; n < paramsMap.size(); n++)
        {
            oTeXExport << "\t\t";

            for (unsigned int k = 0; k < paramsMap.size(); k++)
            {
                oTeXExport << fitData.vz_w[n][k] / sqrt(fabs(fitData.vz_w[n][n]*fitData.vz_w[k][k]));

                if (k + 1 < paramsMap.size())
                    oTeXExport << " & ";
            }

            oTeXExport << "\\\\" << endl;
        }

        oTeXExport << "\t\\end{pmatrix}" << endl << "\\]" << endl;

	}

    oTeXExport << endl;
    oTeXExport << "\\subsection{" << _lang.get("PARSERFUNCS_FIT_ANALYSIS") << "}" << endl;
    oTeXExport << sFitAnalysis << endl;
    oTeXExport.close();
}


