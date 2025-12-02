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
#include "fitcontroller.hpp"
#include "../interval.hpp"


/////////////////////////////////////////////////
/// \brief This structure combines all main
/// fitting parameter and storage objects to
/// simplify the interaction between the main
/// routine and the (file static) helper routines.
/////////////////////////////////////////////////
struct FittingData
{
    std::vector<double> vx;
    std::vector<double> vy;
    std::vector<double> vy_w;
    std::vector<std::vector<double> > vz;
    std::vector<std::vector<double> > vz_w;

    IntervalSet ivl;
    bool restricted[3];

    size_t nDim;
    size_t nFitVars;
    bool bUseErrors;
    bool bSaveErrors;
    bool bNoParams;
    bool b1DChiMap;
    double eps_rel;
    double eps_abs;
    int nMaxIterations;

    std::string sFitFunction;
    std::string sRestrictions;
    std::string sParams;
    std::string sChiMap;
    std::string sChiMap_Vars[2];
};

// These are the prototypes of the file static helper routines
static std::vector<double> evaluateFittingParams(FittingData& fitData, CommandLineParser& cmdParser, Indices& _idx, std::string& sTeXExportFile, bool& bTeXExport, bool& bMaskDialog);
static mu::varmap_type getFittingParameters(FittingData& fitData, const mu::varmap_type& varMap, const std::string& sCmd);
static int getDataForFit(CommandLineParser& cmdParser, std::string& sDimsForFitLog, FittingData& fitData);
static void removeObsoleteParentheses(std::string& sFunction);
static bool calculateChiMap(std::string sFunctionDefString, const std::string& sFuncDisplay, Indices& _idx, mu::varmap_type& varMap, mu::varmap_type& paramsMap, FittingData& fitData, std::vector<double> vInitialVals);
static std::string applyFitAlgorithm(Fitcontroller& _fControl, FittingData& fitData, mu::varmap_type& paramsMap, const std::string& sFuncDisplay);
static void calculateCovarianceData(FittingData& fitData, double dChisq, size_t paramsMapSize);
static std::string getFitOptionsTable(Fitcontroller& _fControl, FittingData& fitData, const std::string& sFuncDisplay, const std::string& sFittedFunction, const std::string& sDimsForFitLog, double dChisq, const mu::varmap_type& paramsMap, size_t nSize, bool forFitLog);
static std::string getParameterTable(FittingData& fitData, mu::varmap_type& paramsMap, const std::vector<double>& vInitialVals, size_t windowSize, const std::string& sPMSign, bool forFitLog);
static std::string constructCovarianceMatrix(FittingData& fitData, size_t paramsMapSize, bool forFitLog);
static double calculatePercentageAvgAndCreateParserVariables(FittingData& fitData, mu::varmap_type& paramsMap, double dChisq);
static std::string getFitAnalysis(Fitcontroller& _fControl, FittingData& fitData, double dNormChisq, double dAverageErrorPercentage, bool noOverfitting);
static void createTeXExport(Fitcontroller& _fControl, const std::string& sTeXExportFile, const std::string& sCmd, mu::varmap_type& paramsMap, FittingData& fitData, const std::vector<double>& vInitialVals, size_t nSize, const std::string& sFitAnalysis, const std::string& sFuncDisplay, const std::string& sFittedFunction, double dChisq);


/////////////////////////////////////////////////
/// \brief Fit a function with some params and
/// algorithmic parameters to a passed data set.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool fitDataSet(CommandLineParser& cmdParser)
{
    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    const Settings& _option = NumeReKernel::getInstance()->getSettings();

    // Declare the FittingData object first
    FittingData fitData;

    std::vector<double> vInitialVals;
    std::vector<double> vInterVal;
    double dChisq = 0.0;
    Indices _idx;

    std::ofstream oFitLog;
    std::string sFitLog = "<savepath>/numerefit.log";
    sFitLog = _data.ValidFileName(sFitLog, ".log");

    bool bMaskDialog = false;
    bool bTeXExport = false;
    std::string sTeXExportFile = "<savepath>/fit.tex";
    std::string sDimsForFitLog;
    std::string sFunctionDefString = "";
    std::string sFuncDisplay = "";

    // Prepare the default values in the FittingData
    // object for the further calculation
    fitData.ivl.intervals.resize(3);
    fitData.restricted[0] = false;
    fitData.restricted[1] = false;
    fitData.restricted[2] = false;

    fitData.nDim = 1;
    fitData.nFitVars = 0;
    fitData.bUseErrors = false;
    fitData.bSaveErrors = false;
    fitData.bNoParams = false;
    fitData.b1DChiMap = false;
    fitData.eps_abs = 1e-4;
    fitData.eps_rel = 1e-4;
    fitData.nMaxIterations = 500;
    fitData.sChiMap_Vars[0].clear();
    fitData.sChiMap_Vars[1].clear();
    fitData.sFitFunction = cmdParser.getParameterList();
    fitData.sRestrictions = "";

    fitData.bUseErrors = cmdParser.getCommand() == "fitw";

    // Ensure that data is available
    if (!isNotEmptyExpression(cmdParser.getExpr()))
        throw SyntaxError(SyntaxError::NO_DATA_FOR_FIT, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Evaluate all passed parameters in this file static
    // function and return the initial fitting parameter
    // values
    vInterVal = evaluateFittingParams(fitData, cmdParser, _idx, sTeXExportFile, bTeXExport, bMaskDialog);

    fitData.sFitFunction = " " + fitData.sFitFunction + " ";
    _parser.SetInitValue(mu::Value(0.0));
    mu::varmap_type varMap;
    mu::varmap_type paramsMap;

    try
    {
        _parser.SetExpr(fitData.sFitFunction);
        _parser.Eval();
        varMap = _parser.GetUsedVar();

        // Ensure that no variable is initialized to VOID
        for (auto& iter : varMap)
        {
            if (iter.second->getCommonType() == mu::TYPE_VOID)
                *iter.second = mu::Value(0.0);
        }

        // Get the map containing all the fitting parameters,
        // which will be used by the fit
        paramsMap = getFittingParameters(fitData, varMap, cmdParser.getCommandLine());
    }
    catch (...)
    {
        // Duplication needed for fallbacks
        _parser.SetInitValue(mu::Value());
        throw;
    }

    _parser.SetInitValue(mu::Value());

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
            throw SyntaxError(SyntaxError::CANNOT_BE_A_FITTING_PARAM, cmdParser.getCommandLine(), SyntaxError::invalid_position, "x");

        if (fitData.sChiMap_Vars[0] == "y" || fitData.sChiMap_Vars[1] == "y")
            throw SyntaxError(SyntaxError::CANNOT_BE_A_FITTING_PARAM, cmdParser.getCommandLine(), SyntaxError::invalid_position, "y");

        if (varMap.find(fitData.sChiMap_Vars[0]) == varMap.end())
            throw SyntaxError(SyntaxError::FITFUNC_NOT_CONTAINS, cmdParser.getCommandLine(), SyntaxError::invalid_position, fitData.sChiMap_Vars[0]);

        if (varMap.find(fitData.sChiMap_Vars[1]) == varMap.end())
            fitData.b1DChiMap = true;
    }

    // Ensure that "x" is present in the function, if the fit
    // is along a single dimension
    if (!fitData.nFitVars || !(fitData.nFitVars & 1))
        throw SyntaxError(SyntaxError::FITFUNC_NOT_CONTAINS, cmdParser.getCommandLine(), SyntaxError::invalid_position, "x");

    // Validate the parameters in the parameter map. If the
    // parameter map contains one of the variables x,y,z, then
    // throw an error. Also ensure that all passed fitting
    // parameters are also part of the fit function
    for (auto pItem = paramsMap.begin(); pItem != paramsMap.end(); ++pItem)
    {
        if (pItem->first == "x" || pItem->first == "y" || pItem->first == "z")
            throw SyntaxError(SyntaxError::CANNOT_BE_A_FITTING_PARAM, cmdParser.getCommandLine(), SyntaxError::invalid_position, pItem->first);

        if (varMap.find(pItem->first) == varMap.end())
            throw SyntaxError(SyntaxError::FITFUNC_NOT_CONTAINS, cmdParser.getCommandLine(), SyntaxError::invalid_position, pItem->first);
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
        throw SyntaxError(SyntaxError::NO_PARAMS_FOR_FIT, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    sFuncDisplay = fitData.sFitFunction;

    // Get the necessary data for the fitting routine. This is done
    // in the following file static routine. It will also return the
    // actual fitting dimension
    fitData.nDim = getDataForFit(cmdParser, sDimsForFitLog, fitData);

    // Ensure that we're not trying to use more paramters than values
    if (paramsMap.size() > fitData.vy.size())
        throw SyntaxError(SyntaxError::OVERFITTING_ERROR, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    if (paramsMap.size() > fitData.vx.size())
        throw SyntaxError(SyntaxError::OVERFITTING_ERROR, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    if ((fitData.nFitVars & 2)
        && (paramsMap.size() > fitData.vz.size() || paramsMap.size() > fitData.vz[0].size()))
        throw SyntaxError(SyntaxError::OVERFITTING_ERROR, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Remove obsolete parentheses in the function displaying
    // string and the actual fitting function
    removeObsoleteParentheses(sFuncDisplay);
    removeObsoleteParentheses(fitData.sFitFunction);

    if (_option.systemPrints())
        NumeReKernel::printPreFmt("|-> " + _lang.get("PARSERFUNCS_FIT_FITTING", sFuncDisplay) + " ");

    for (auto iter = paramsMap.begin(); iter != paramsMap.end(); ++iter)
    {
        if (!iter->second->size())
            iter->second->push_back(mu::Value(0.0));

        vInitialVals.push_back(iter->second->front().getNum().asF64());
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
    sFunctionDefString = applyFitAlgorithm(_fControl, fitData, paramsMap, sFuncDisplay);

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
    if (!bMaskDialog && _option.systemPrints())
        reduceLogFilesize(sFitLog);

    std::string sFittedFunction = _fControl.getFitFunction();

    // Open the fitting log file stream
    oFitLog.open(sFitLog.c_str(), std::ios_base::ate | std::ios_base::app);

    // If the we could not open the file stream
    // abort and inform the user
    if (oFitLog.fail())
    {
        oFitLog.close();
        NumeReKernel::printPreFmt("\n");
        throw SyntaxError(SyntaxError::CANNOT_OPEN_FITLOG, cmdParser.getCommandLine(), SyntaxError::invalid_position);
    }

    // Write the headline to the fitting logfile
    oFitLog << std::setw(156) << std::setfill('=') << '=' << std::endl;
    oFitLog << toUpperCase(_lang.get("PARSERFUNCS_FIT_HEADLINE")) << ": " << getTimeStamp(false) << std::endl;
    oFitLog << std::setw(156) << std::setfill('=') << '=' << std::endl;

    // Write the fitting options table to the log file
    oFitLog << getFitOptionsTable(_fControl, fitData, sFuncDisplay, sFittedFunction, sDimsForFitLog, dChisq, paramsMap, nSize, true) << std::endl;

    std::string sPMSign = " ";
    sPMSign[0] = (char)177;

    // Prepare the headline for the fitting parameter table
    if (fitData.bUseErrors)
        oFitLog << _lang.get("PARSERFUNCS_FIT_LOG_TABLEHEAD1") << std::endl;
    else
        oFitLog << _lang.get("PARSERFUNCS_FIT_LOG_TABLEHEAD2") << std::endl;

    // Write the fitting parameter table to the logfile
    oFitLog << std::setw(156) << std::setfill('-') << '-' << std::endl;
    oFitLog << getParameterTable(fitData, paramsMap, vInitialVals, 160, sPMSign, true);
    oFitLog << std::setw(156) << std::setfill('-') << '-' << std::endl;


    if (_option.systemPrints())
        NumeReKernel::printPreFmt(_lang.get("COMMON_SUCCESS") + ".\n");

    if (_option.systemPrints() && !bMaskDialog)
    {
        NumeReKernel::toggleTableStatus();
        make_hline();
        NumeReKernel::print(toSystemCodePage("NUMERE: " + toUpperCase(_lang.get("PARSERFUNCS_FIT_HEADLINE"))));
        make_hline();

        // Write the fitting options table to the terminal screen
        NumeReKernel::printPreFmt("|-> " + getFitOptionsTable(_fControl, fitData, sFuncDisplay, sFittedFunction, sDimsForFitLog, dChisq, paramsMap, nSize, false));
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
        oFitLog << std::endl;
        oFitLog << _lang.get("PARSERFUNCS_FIT_CORRELMAT_HEAD") << ":" << std::endl;
        oFitLog << std::endl;
        oFitLog << constructCovarianceMatrix(fitData, paramsMap.size(), true) << std::endl;

        if (_option.systemPrints() && !bMaskDialog)
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
    std::string sFitAnalysis = getFitAnalysis(_fControl, fitData, dChisq / (double)(nSize - paramsMap.size()), dAverageErrorPercentage, nSize != paramsMap.size());

    // Write the fitting analysis to the logfile and to the terminal
    oFitLog << _lang.get("PARSERFUNCS_FIT_ANALYSIS") << ":" << std::endl;
    oFitLog << sFitAnalysis << std::endl;
    oFitLog.close();

    if (_option.systemPrints() && !bMaskDialog)
    {
        NumeReKernel::printPreFmt("|\n|-> " + _lang.get("PARSERFUNCS_FIT_ANALYSIS") + ":\n");
        NumeReKernel::print(sFitAnalysis);
        NumeReKernel::toggleTableStatus();
        make_hline();
    }

    // If the user requested an export of the fitting results to
    // a TeX file, this is done here
    if (bTeXExport)
        createTeXExport(_fControl, sTeXExportFile, cmdParser.getCommandLine(), paramsMap, fitData, vInitialVals, nSize, sFitAnalysis, sFuncDisplay, sFittedFunction, dChisq);

    // Update the function definition of the automatically created
    // Fit(x) or Fitw(x) function
    bool bDefinitionSuccess = false;
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    if (!_functions.isDefined(sFunctionDefString))
        bDefinitionSuccess = _functions.defineFunc(sFunctionDefString);
    else if (_functions.getDefinitionString(_functions.getFunctionIndex(sFunctionDefString)) != sFunctionDefString)
        bDefinitionSuccess = _functions.defineFunc(sFunctionDefString, true);
    else if (_functions.getDefinitionString(_functions.getFunctionIndex(sFunctionDefString)) == sFunctionDefString)
        return true;

    if (bDefinitionSuccess)
        NumeReKernel::print(_lang.get("DEFINE_SUCCESS"), _option.systemPrints());
    else
        NumeReKernel::issueWarning(_lang.get("DEFINE_FAILURE"));

    return true;
}


/////////////////////////////////////////////////
/// \brief This static function evaluates the
/// passed parameters of the fitting routine.
///
/// \param fitData FittingData&
/// \param cmdParser CommandLineParser&
/// \param _idx Indices&
/// \param sTeXExportFile std::string&
/// \param bTeXExport bool&
/// \param bMaskDialog bool&
/// \return std::vector<double>
///
/////////////////////////////////////////////////
static std::vector<double> evaluateFittingParams(FittingData& fitData, CommandLineParser& cmdParser, Indices& _idx, std::string& sTeXExportFile, bool& bTeXExport, bool& bMaskDialog)
{
    std::string sParamList = cmdParser.getParameterList();
    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();

    std::vector<double> vInterVal;
    static const std::string sBADFUNCTIONS = "ascii(),char(),findfile(),findparam(),gauss(),getopt(),is_string(),rand(),replace(),replaceall(),split(),strfnd(),string_cast(),strrfnd(),strlen(),time(),to_char(),to_cmd(),to_string(),to_value()";

    // Evaluate the chi^2 map option
    if (findParameter(sParamList, "chimap", '='))
    {
        fitData.sChiMap = getArgAtPos(sParamList, findParameter(sParamList, "chimap", '=') + 6);
        eraseToken(sParamList, "chimap", true);

        if (fitData.sChiMap.length())
        {
            getIndices(fitData.sChiMap, _idx, _parser, _data, true);

            if (!isValidIndexSet(_idx))
                throw SyntaxError(SyntaxError::INVALID_INDEX, cmdParser.getCommandLine(), SyntaxError::invalid_position, _idx.row.to_string() + ", " + _idx.col.to_string());

            if (_idx.col.size() < 2)
                throw SyntaxError(SyntaxError::INVALID_INDEX, cmdParser.getCommandLine(), SyntaxError::invalid_position, _idx.row.to_string() + ", " + _idx.col.to_string());

            evaluateIndices(fitData.sChiMap, _idx, _data);
            fitData.sChiMap.erase(fitData.sChiMap.find('('));
            fitData.sChiMap_Vars[0] = _data.getHeadLineElement(_idx.col[0], fitData.sChiMap);
            fitData.sChiMap_Vars[1] = _data.getHeadLineElement(_idx.col[1], fitData.sChiMap);
        }
    }

    // Does the user want to export the results into a TeX file?
    if (findParameter(sParamList, "export", '='))
    {
        bTeXExport = true;
        sTeXExportFile = getArgAtPos(sParamList, findParameter(sParamList, "export", '=') + 6);
        eraseToken(sParamList, "export", true);
    }
    else if (findParameter(sParamList, "export"))
    {
        bTeXExport = true;
        eraseToken(sParamList, "export", false);
    }

    // Ensure that the file name for the TeX file is valid
    if (bTeXExport)
    {
        sTeXExportFile = _data.ValidFileName(sTeXExportFile, ".tex");

        if (!sTeXExportFile.ends_with(".tex"))
            sTeXExportFile.replace(sTeXExportFile.rfind('.'), std::string::npos, ".tex");
    }

    // Separate expression from the command line option list
    for (size_t i = 0; i < sParamList.length(); i++)
    {
        if (sParamList[i] == '(')
            i += getMatchingParenthesis(StringView(sParamList, i));

        if (sParamList[i] == '-')
        {
            sParamList.erase(0, i);
            break;
        }
    }

    // Decode possible interval definitions in the command
    // line option list
    vInterVal = readAndParseIntervals(sParamList, _parser, _data, _functions, true);

    // Evaluate the contents of the parsed interval definitions
    for (size_t i = 0; i < vInterVal.size(); i += 2)
    {
        fitData.ivl[i / 2].reset(vInterVal[i], vInterVal[i+1]);
        fitData.restricted[i / 2] = true;
    }

    // Insert the command line option list after the intervals
    // were parsed into the original command line
    for (size_t i = 0; i < fitData.sFitFunction.length(); i++)
    {
        if (fitData.sFitFunction[i] == '(')
            i += getMatchingParenthesis(StringView(fitData.sFitFunction, i));

        if (fitData.sFitFunction[i] == '-')
        {
            fitData.sFitFunction.replace(i, std::string::npos, sParamList.substr(sParamList.find('-')));
            break;
        }
    }

    sParamList = fitData.sFitFunction;

    // Because it's quite likely that one misspells the option value
    // "saverr", whe accept also the spelling "saveer" as an alternative
    if (findParameter(fitData.sFitFunction, "saverr"))
    {
        fitData.bSaveErrors = true;
        fitData.sFitFunction.erase(findParameter(fitData.sFitFunction, "saverr") - 1, 6);
        sParamList.erase(findParameter(sParamList, "saverr") - 1, 6);
    }

    if (findParameter(fitData.sFitFunction, "saveer"))
    {
        fitData.bSaveErrors = true;
        fitData.sFitFunction.erase(findParameter(fitData.sFitFunction, "saveer") - 1, 6);
        sParamList.erase(findParameter(sParamList, "saveer") - 1, 6);
    }

    // The masking paramter
    if (findParameter(fitData.sFitFunction, "mask"))
    {
        bMaskDialog = true;
        fitData.sFitFunction.erase(findParameter(fitData.sFitFunction, "mask") - 1, 6);
        sParamList.erase(findParameter(sParamList, "mask") - 1, 6);
    }

    // Ensure that a fitting function was defined
    if (!findParameter(fitData.sFitFunction, "with", '='))
        throw SyntaxError(SyntaxError::NO_FUNCTION_FOR_FIT, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Changes to the tolerance
    if (findParameter(fitData.sFitFunction, "tol", '='))
    {
        _parser.SetExpr(getArgAtPos(fitData.sFitFunction, findParameter(fitData.sFitFunction, "tol", '=') + 3));
        eraseToken(sParamList, "tol", true);
        eraseToken(fitData.sFitFunction, "tol", true);

        mu::Array v = _parser.Eval();

        if (v.size() > 1)
        {
            fitData.eps_rel = std::abs(v[0].getNum().asF64());
            fitData.eps_abs = std::abs(v[1].getNum().asF64());
        }
        else
        {
            fitData.eps_rel = std::abs(v[0].getNum().asF64());
            fitData.eps_abs = std::abs(v[0].getNum().asF64());
        }

        if (std::isnan(fitData.eps_rel) || std::isinf(fitData.eps_rel))
            fitData.eps_rel = 1e-4;

        if (std::isnan(fitData.eps_abs) || std::isinf(fitData.eps_abs))
            fitData.eps_abs = 1e-4;
    }

    // Changes to the maximal number of iterations
    if (findParameter(fitData.sFitFunction, "iter", '='))
    {
        _parser.SetExpr(getArgAtPos(fitData.sFitFunction, findParameter(fitData.sFitFunction, "iter", '=') + 4));
        eraseToken(sParamList, "iter", true);
        eraseToken(fitData.sFitFunction, "iter", true);
        fitData.nMaxIterations = std::abs(_parser.Eval().getAsScalarInt());

        if (!fitData.nMaxIterations)
            fitData.nMaxIterations = 500;
    }

    // Fitting parameter restrictions
    if (findParameter(fitData.sFitFunction, "restrict", '='))
    {
        fitData.sRestrictions = getArgAtPos(fitData.sFitFunction, findParameter(fitData.sFitFunction, "restrict", '=') + 8);
        eraseToken(sParamList, "restrict", true);
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
    if (!findParameter(fitData.sFitFunction, "params", '='))
    {
        fitData.bNoParams = true;
        fitData.sFitFunction = fitData.sFitFunction.substr(findParameter(fitData.sFitFunction, "with", '=') + 4);
        sParamList.erase(findParameter(sParamList, "with", '=') - 1);
    }
    else if (findParameter(fitData.sFitFunction, "with", '=') < findParameter(fitData.sFitFunction, "params", '='))
    {
        fitData.sParams = fitData.sFitFunction.substr(findParameter(fitData.sFitFunction, "params", '=') + 6);
        fitData.sFitFunction = fitData.sFitFunction.substr(findParameter(fitData.sFitFunction, "with", '=') + 4, findParameter(fitData.sFitFunction, "params", '=') - 1 - findParameter(fitData.sFitFunction, "with", '=') - 4);
        sParamList = sParamList.substr(0, findParameter(sParamList, "with", '=') - 1);
    }
    else
    {
        fitData.sParams = fitData.sFitFunction.substr(findParameter(fitData.sFitFunction, "params", '=') + 6, findParameter(fitData.sFitFunction, "with", '=') - 1 - findParameter(fitData.sFitFunction, "params", '=') - 6);
        fitData.sFitFunction = fitData.sFitFunction.substr(findParameter(fitData.sFitFunction, "with", '=') + 4);
        sParamList = sParamList.substr(0, findParameter(sParamList, "params", '=') - 1);
    }

    // Remove surrounding brackets from the parameter list
    if (fitData.sParams.find('[') != std::string::npos)
        fitData.sParams = fitData.sParams.substr(fitData.sParams.find('[') + 1);

    if (fitData.sParams.find(']') != std::string::npos)
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
            throw SyntaxError(SyntaxError::FUNCTION_ERROR, cmdParser.getCommandLine(), fitData.sParams, fitData.sParams);

        if (_data.containsTables(fitData.sParams))
            getDataElements(fitData.sParams, _parser, _data);

        //if (fitData.sParams.find("{") != std::string::npos && NumeReKernel::getInstance()->getStringParser().isStringExpression(fitData.sParams))
        //    convertVectorToExpression(fitData.sParams);
    }

    StripSpaces(sParamList);

    // Evaluate function definition calls
    if (!_functions.call(fitData.sFitFunction))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, cmdParser.getCommandLine(), fitData.sFitFunction, fitData.sFitFunction);

    // Get values from a references data object
    if (_data.containsTables(fitData.sFitFunction))
        getDataElements(fitData.sFitFunction, _parser, _data);

    // Expand remaining vectors
    if (fitData.sFitFunction.find("{") != std::string::npos)
        convertVectorToExpression(fitData.sFitFunction);

    size_t nPos = 0;

    // Ensure that the fitting function does not contain any of the
    // functions from the bad fitting functions list (a list of functions,
    // which cannot be used to solve the optimisation problem)
    while (sBADFUNCTIONS.find(',', nPos) != std::string::npos)
    {
        if (fitData.sFitFunction.find(sBADFUNCTIONS.substr(nPos, sBADFUNCTIONS.find(',', nPos) - nPos - 1)) != std::string::npos)
        {
            throw SyntaxError(SyntaxError::FUNCTION_CANNOT_BE_FITTED, cmdParser.getCommandLine(), SyntaxError::invalid_position, sBADFUNCTIONS.substr(nPos, sBADFUNCTIONS.find(',', nPos) - nPos));
        }
        else
            nPos = sBADFUNCTIONS.find(',', nPos) + 1;

        if (nPos >= sBADFUNCTIONS.length())
            break;
    }

    return vInterVal;
}


/////////////////////////////////////////////////
/// \brief This static function returns a map of
/// all fitting paramters used in the fitting
/// algorithm.
///
/// \param fitData FittingData&
/// \param varMap const mu::varmap_type&
/// \param sCmd const std::string&
/// \return mu::varmap_type
///
/////////////////////////////////////////////////
static mu::varmap_type getFittingParameters(FittingData& fitData, const mu::varmap_type& varMap, const std::string& sCmd)
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
        if (fitData.sParams.find('=') != std::string::npos)
        {
            for (size_t i = 0; i < fitData.sParams.length(); i++)
            {
                if (fitData.sParams[i] == '=')
                {
                    for (size_t j = i; j < fitData.sParams.length(); j++)
                    {
                        if (fitData.sParams[j] == '(')
                            j += getMatchingParenthesis(StringView(fitData.sParams, j));

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


/////////////////////////////////////////////////
/// \brief This static function obtains the data
/// needed for the fitting algorithm and stores
/// it in the vector members in the FittingData
/// object.
///
/// \param cmdParser CommandLineParser&
/// \param sDimsForFitLog std::string&
/// \param fitData FittingData&
/// \return int
///
/////////////////////////////////////////////////
static int getDataForFit(CommandLineParser& cmdParser, std::string& sDimsForFitLog, FittingData& fitData)
{
    int nColumns = 0;
    int nDim = 0;

    mu::Array xArray;
    mu::Array yArray;
    mu::Array zArray;

    DataView _dataView = cmdParser.getExprAsDataView();

    _dataView.evalIndices();

    if (!_dataView.isValueLike())
        throw SyntaxError(SyntaxError::WRONG_COLUMN_TYPE, cmdParser.getCommandLine(), _dataView.getDataName(), cmdParser.getExpr());

    _dataView.reserveAxes(1 + (fitData.nFitVars & 2), false);
    nColumns = _dataView.cols();

    // Extract the x axis
    xArray = _dataView.getAxis(0);

    // Get the y and the z axes, if possible
    if (fitData.nFitVars & 2)
    {
        yArray = _dataView.getAxis(1);
        zArray = _dataView.get(VectorIndex(0, _dataView.rows()-1), VectorIndex(0));
    }
    else
        yArray = _dataView.get(VectorIndex(0, _dataView.rows()-1), VectorIndex(0));

    if (nColumns < 2 && fitData.bUseErrors)
        throw SyntaxError(SyntaxError::TOO_FEW_COLS, cmdParser.getCommandLine(), _dataView.getDataName(), cmdParser.getExpr());

    // Update the intervals, if not already defined by the suer
    if (mu::isnan(fitData.ivl[0].front()))
        fitData.ivl[0].reset(xArray.call("min").front().as_cmplx(),
                             xArray.call("max").front().as_cmplx());

    if (mu::isnan(fitData.ivl[1].front()))
        fitData.ivl[1].reset(yArray.call("min").front().as_cmplx(),
                             yArray.call("max").front().as_cmplx());

    if ((fitData.nFitVars & 2) && mu::isnan(fitData.ivl[2].front()))
    {
        // Tuple vs Grid
        if (nColumns <= 2)
            fitData.ivl[2].reset(zArray.call("min").front().as_cmplx(),
                                 zArray.call("max").front().as_cmplx());
        else
            fitData.ivl[2].reset(_dataView.min().as_cmplx(),
                                 _dataView.max().as_cmplx());
    }

    // Copy the data
    if (!(fitData.nFitVars & 2))
    {
        // Only x
        for (size_t i = 0; i < _dataView.rows(); i++)
        {
            if (!fitData.ivl[0].isInside(xArray.get(i).as_cmplx()) || !fitData.ivl[1].isInside(yArray.get(i).as_cmplx()))
                continue;

            fitData.vx.push_back(xArray.get(i).getNum().asF64());
            fitData.vy.push_back(yArray.get(i).getNum().asF64());

            if (fitData.bUseErrors)
            {
                if (nColumns == 2)
                    fitData.vy_w.push_back(_dataView.get(i, 1).isValid() ? std::abs(_dataView.get(i, 1).getNum().asF64()) : 0.0);
                else
                {
                    mu::Value valx_w = _dataView.get(i, 1);
                    mu::Value valy_w = _dataView.get(i, 2);

                    if (valx_w.isValid() && valy_w.isValid())
                        fitData.vy_w.push_back(std::sqrt(std::abs(valx_w.getNum().asF64() * valy_w.getNum().asF64())));
                    else if (valx_w.isValid())
                        fitData.vy_w.push_back(std::abs(valx_w.getNum().asF64()));
                    else if (valy_w.isValid())
                        fitData.vy_w.push_back(std::abs(valy_w.getNum().asF64()));
                    else
                        fitData.vy_w.push_back(0.0);
                }
            }
        }
    }
    else
    {
        if (nColumns <= 2)
        {
            // x and y for tuples
            for (size_t i = 0; i < _dataView.rows(); i++)
            {
                if (!fitData.ivl[0].isInside(xArray.get(i).as_cmplx())
                    || !fitData.ivl[1].isInside(yArray.get(i).as_cmplx())
                    || !fitData.ivl[2].isInside(zArray.get(i).as_cmplx()))
                    continue;

                fitData.vx.push_back(xArray.get(i).getNum().asF64());
                fitData.vy.push_back(yArray.get(i).getNum().asF64());

                fitData.vz.push_back({zArray.get(i).getNum().asF64()});

                if (fitData.bUseErrors)
                {
                    if (_dataView.get(i, 2).isValid())
                        fitData.vy_w.push_back({std::abs(_dataView.get(i, 2).getNum().asF64())});
                    else
                        fitData.vy_w.push_back({0.0});
                }
            }
        }
        else
        {
            // x and y for grid
            for (size_t i = 0; i < _dataView.rows(); i++)
            {
                if (!fitData.ivl[0].isInside(xArray.get(i).as_cmplx())
                    || !fitData.ivl[1].isInside(yArray.get(i).as_cmplx()))
                    continue;

                fitData.vx.push_back(xArray.get(i).getNum().asF64());
                fitData.vy.push_back(yArray.get(i).getNum().asF64());

                std::vector<double> vTempZ;

                for (size_t j = 0; j < std::min(yArray.size(), _dataView.cols()); j++)
                {
                    if (!fitData.ivl[2].isInside(_dataView.get(i, j).as_cmplx()))
                    {
                        vTempZ.push_back(NAN);

                        if (fitData.bUseErrors)
                            fitData.vy_w.push_back(0.0);
                    }
                    else
                    {
                        vTempZ.push_back(_dataView.get(i, j).getNum().asF64());

                        if (fitData.bUseErrors)
                        {
                            if (_dataView.get(i, j + yArray.size()).isValid())
                                fitData.vy_w.push_back(std::abs(_dataView.get(i, j+yArray.size()).getNum().asF64()));
                            else
                                fitData.vy_w.push_back(0.0);
                        }
                    }
                }

                fitData.vz.push_back(vTempZ);

                if (fitData.vy_w.size() && fitData.bUseErrors)
                {
                    fitData.vz_w.push_back(fitData.vy_w);
                    fitData.vy_w.clear();
                }
            }
        }
    }

    // Define the dimension of the fit data set, which loosely corresponds to the number of columns
    nDim = std::min(nColumns, 3) + 1 + (fitData.nFitVars & 2);

    // Prepare the message for the fit log
    if (_dataView.isTable())
    {
        MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
        sDimsForFitLog = _dataView.getTableIndices().col.to_string()
            + " " + _lang.get("PARSERFUNCS_FIT_FROM") + " " + _data.getDataFileName(_dataView.getDataName());
    }
    else
        sDimsForFitLog = "1-" + toString(_dataView.cols())
            + " " + _lang.get("PARSERFUNCS_FIT_FROM") + " " + _dataView.getDataName();

    return nDim;
}


/////////////////////////////////////////////////
/// \brief This static function removes obsolete
/// surrounding parentheses in function strings.
///
/// \param sFunction std::string&
/// \return void
///
/////////////////////////////////////////////////
static void removeObsoleteParentheses(std::string& sFunction)
{
    StripSpaces(sFunction);

    // As long as the first character is an opening
    // parenthesis, search the closing one and remove both,
    // if it is the last character of the function
    while (sFunction.front() == '(')
    {
        if (getMatchingParenthesis(sFunction) == sFunction.length() - 1 && getMatchingParenthesis(sFunction) != std::string::npos)
        {
            sFunction.erase(0, 1);
            sFunction.pop_back();
            StripSpaces(sFunction);
        }
        else
            break;
    }
}


/////////////////////////////////////////////////
/// \brief This static function calculates the
/// chi^2 map instead of applying the single fit.
///
/// \param sFunctionDefString std::string
/// \param sFuncDisplay const std::string&
/// \param _idx Indices&
/// \param varMap mu::varmap_type&
/// \param paramsMap mu::varmap_type&
/// \param fitData FittingData&
/// \param vInitialVals std::vector<double>
/// \return bool
///
/////////////////////////////////////////////////
static bool calculateChiMap(std::string sFunctionDefString, const std::string& sFuncDisplay, Indices& _idx, mu::varmap_type& varMap, mu::varmap_type& paramsMap, FittingData& fitData, std::vector<double> vInitialVals)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    FunctionDefinitionManager& _functions = NumeReKernel::getInstance()->getDefinitions();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    Fitcontroller _fControl(&_parser);

    for (size_t i = 0; i < _idx.row.size(); i++)
    {
        for (size_t j = 0; j <= (_idx.row.size() - 1) * (!fitData.b1DChiMap); j++)
        {
            auto iter = paramsMap.begin();

            for (size_t n = 0; n < vInitialVals.size(); n++)
            {
                *(iter->second) = mu::Value(vInitialVals[n]);
                ++iter;
            }

            if (!_data.isValidElement(_idx.row[i], _idx.col[0], fitData.sChiMap))
                continue;

            *(varMap.at(fitData.sChiMap_Vars[0])) = _data.getElement(_idx.row[i], _idx.col[0], fitData.sChiMap);

            if (!fitData.b1DChiMap)
            {
                if (!_data.isValidElement(_idx.row[j], _idx.col[1], fitData.sChiMap))
                    continue;

                *(varMap.at(fitData.sChiMap_Vars[1])) = _data.getElement(_idx.row[j], _idx.col[1], fitData.sChiMap);
            }

            if (fitData.nDim >= 2 && fitData.nFitVars == 1)
            {
                if (!fitData.bUseErrors)
                {
                    if (!_fControl.fit(fitData.vx, fitData.vy, fitData.sFitFunction, fitData.sRestrictions, paramsMap,
                                       fitData.eps_rel, fitData.eps_abs, fitData.nMaxIterations))
                    {
                        if (_option.systemPrints())
                            NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");

                        return false;
                    }

                    sFunctionDefString = "Fit(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
                }
                else
                {
                    if (!_fControl.fit(fitData.vx, fitData.vy, fitData.vy_w, fitData.sFitFunction, fitData.sRestrictions, paramsMap,
                                       fitData.eps_rel, fitData.eps_abs, fitData.nMaxIterations))
                    {
                        if (_option.systemPrints())
                            NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");

                        return false;
                    }

                    sFunctionDefString = "Fitw(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
                }
            }
            else if (fitData.nDim == 3)
            {
                if (!_fControl.fit(fitData.vx, fitData.vy, fitData.vz, fitData.sFitFunction, fitData.sRestrictions, paramsMap,
                                   fitData.eps_rel, fitData.eps_abs, fitData.nMaxIterations))
                {
                    if (_option.systemPrints())
                        NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");

                    return false;
                }

                sFunctionDefString = "Fit(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
            }
            else if (fitData.nDim == 5)
            {
                if (!_fControl.fit(fitData.vx, fitData.vy, fitData.vz, fitData.vz_w, fitData.sFitFunction, fitData.sRestrictions, paramsMap,
                                   fitData.eps_rel, fitData.eps_abs, fitData.nMaxIterations))
                {
                    if (_option.systemPrints())
                        NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");

                    return false;
                }

                sFunctionDefString = "Fitw(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
            }

            _data.writeToTable(_idx.row[i], _idx.col[1 + (!fitData.b1DChiMap) * (j + 1)], fitData.sChiMap, _fControl.getFitChi());

            if (!i && !fitData.b1DChiMap)
                _data.setHeadLineElement(_idx.col[1 + (!fitData.b1DChiMap) * (j + 1)], fitData.sChiMap, "chi^2(" + toString(j + 1) + ")");
            else if (!i)
                _data.setHeadLineElement(_idx.col[1 + (!fitData.b1DChiMap) * (j + 1)], fitData.sChiMap, "chi^2");

        }
    }

    auto iter = paramsMap.begin();

    for (size_t n = 0; n < vInitialVals.size(); n++)
    {
        *(iter->second) = mu::Value(vInitialVals[n]);
        ++iter;
    }

    if (_option.systemPrints())
    {
        NumeReKernel::printPreFmt(_lang.get("COMMON_SUCCESS") + ".\n");
        NumeReKernel::print(_lang.get("PARSERFUNCS_FIT_CHIMAPLOCATION", fitData.sChiMap));
    }

    bool bDefinitionSuccess = false;

    if (!_functions.isDefined(sFunctionDefString))
        bDefinitionSuccess = _functions.defineFunc(sFunctionDefString);
    else if (_functions.getDefinitionString(_functions.getFunctionIndex(sFunctionDefString)) != sFunctionDefString)
        bDefinitionSuccess = _functions.defineFunc(sFunctionDefString, true);
    else if (_functions.getDefinitionString(_functions.getFunctionIndex(sFunctionDefString)) == sFunctionDefString)
        return true;

    if (bDefinitionSuccess)
        NumeReKernel::print(_lang.get("DEFINE_SUCCESS"), _option.systemPrints());
    else
        NumeReKernel::issueWarning(_lang.get("DEFINE_FAILURE"));

    return true;

}


/////////////////////////////////////////////////
/// \brief This static function applies the
/// fitting algorithm to the obtained data and
/// the prepared fitting function. It returns the
/// function definition string for the
/// autmatically created fitting function.
///
/// \param _fControl Fitcontroller&
/// \param fitData FittingData&
/// \param paramsMap mu::varmap_type&
/// \param sFuncDisplay const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string applyFitAlgorithm(Fitcontroller& _fControl, FittingData& fitData, mu::varmap_type& paramsMap, const std::string& sFuncDisplay)
{
    std::string sFunctionDefString;
    Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (fitData.nDim >= 2 && fitData.nFitVars == 1)
    {
        if (!fitData.bUseErrors)
        {
            if (!_fControl.fit(fitData.vx, fitData.vy, fitData.sFitFunction, fitData.sRestrictions, paramsMap,
                               fitData.eps_rel, fitData.eps_abs, fitData.nMaxIterations))
            {
                if (_option.systemPrints())
                    NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");

                throw SyntaxError(SyntaxError::FUNCTION_CANNOT_BE_FITTED, sFuncDisplay, sFuncDisplay, sFuncDisplay);
            }

            sFunctionDefString = "Fit(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
        }
        else
        {
            if (!_fControl.fit(fitData.vx, fitData.vy, fitData.vy_w, fitData.sFitFunction, fitData.sRestrictions, paramsMap,
                               fitData.eps_rel, fitData.eps_abs, fitData.nMaxIterations))
            {
                if (_option.systemPrints())
                    NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");

                throw SyntaxError(SyntaxError::FUNCTION_CANNOT_BE_FITTED, sFuncDisplay, sFuncDisplay, sFuncDisplay);
            }

            sFunctionDefString = "Fitw(x) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
        }
    }
    else if (fitData.nDim == 3)
    {
        if (!_fControl.fit(fitData.vx, fitData.vy, fitData.vz, fitData.sFitFunction, fitData.sRestrictions, paramsMap,
                           fitData.eps_rel, fitData.eps_abs, fitData.nMaxIterations))
        {
            if (_option.systemPrints())
                NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");

            throw SyntaxError(SyntaxError::FUNCTION_CANNOT_BE_FITTED, sFuncDisplay, sFuncDisplay, sFuncDisplay);
        }

        sFunctionDefString = "Fit(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
    }
    else if (fitData.nDim == 5)
    {
        if (!_fControl.fit(fitData.vx, fitData.vy, fitData.vz, fitData.vz_w, fitData.sFitFunction, fitData.sRestrictions, paramsMap,
                           fitData.eps_rel, fitData.eps_abs, fitData.nMaxIterations))
        {
            if (_option.systemPrints())
                NumeReKernel::printPreFmt(_lang.get("COMMON_FAILURE") + "!\n");

            throw SyntaxError(SyntaxError::FUNCTION_CANNOT_BE_FITTED, sFuncDisplay, sFuncDisplay, sFuncDisplay);
        }

        sFunctionDefString = "Fitw(x,y) := " + sFuncDisplay + " " + _lang.get("PARSERFUNCS_FIT_DEFINECOMMENT");
    }

    return sFunctionDefString;
}


/////////////////////////////////////////////////
/// \brief This static function multiplies the
/// elements in the covariance matrix with the
/// "correct" reduced chi^2 value (this depends
/// upon whether fitting weights shall be used
/// for fitting).
///
/// \param fitData FittingData&
/// \param dChisq double
/// \param paramsMapSize size_t
/// \return void
///
/////////////////////////////////////////////////
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
    for (size_t i = 0; i < fitData.vz_w.size(); i++)
    {
        for (size_t j = 0; j < fitData.vz_w[0].size(); j++)
        {
            fitData.vz_w[i][j] *= dFactor;
        }
    }

}


/////////////////////////////////////////////////
/// \brief This static function returns a string
/// containing the whole fitting options
/// (algorithm parameters, etc.).
///
/// \param _fControl Fitcontroller&
/// \param fitData FittingData&
/// \param sFuncDisplay const std::string&
/// \param sFittedFunction const std::string&
/// \param sDimsForFitLog const std::string&
/// \param dChisq double
/// \param paramsMap const mu::varmap_type&
/// \param nSize size_t
/// \param forFitLog bool
/// \return std::string
///
/////////////////////////////////////////////////
static std::string getFitOptionsTable(Fitcontroller& _fControl, FittingData& fitData, const std::string& sFuncDisplay, const std::string& sFittedFunction, const std::string& sDimsForFitLog, double dChisq, const mu::varmap_type& paramsMap, size_t nSize, bool forFitLog)
{
    std::string sFitParameterTable;
    std::string sPrefix;

    if (forFitLog)
    {
        sFitParameterTable += _lang.get("PARSERFUNCS_FIT_FUNCTION", sFuncDisplay) + "\n";
        sFitParameterTable += _lang.get("PARSERFUNCS_FIT_FITTED_FUNC", sFittedFunction) + "\n";
        sFitParameterTable += _lang.get("PARSERFUNCS_FIT_DATASET") + " " + sDimsForFitLog + "\n";
    }
    else
    {
        sFitParameterTable += _lang.get("PARSERFUNCS_FIT_FUNCTION", sFittedFunction) + "\n";
        sPrefix = "|-> ";
    }

    if (fitData.bUseErrors)
        sFitParameterTable += sPrefix + _lang.get("PARSERFUNCS_FIT_POINTS_W_ERR", toString(nSize)) + "\n";
    else
        sFitParameterTable += sPrefix + _lang.get("PARSERFUNCS_FIT_POINTS_WO_ERR", toString(nSize)) + "\n";

    if (fitData.restricted[0])
        sFitParameterTable += sPrefix + _lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "x", toString(fitData.ivl[0].min(), 5), toString(fitData.ivl[0].max(), 5)) + "\n";

    if (fitData.restricted[1])
        sFitParameterTable += sPrefix + _lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "y", toString(fitData.ivl[1].min(), 5), toString(fitData.ivl[1].max(), 5)) + "\n";

    if (fitData.restricted[2])
        sFitParameterTable += sPrefix + _lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "z", toString(fitData.ivl[2].min(), 5), toString(fitData.ivl[2].max(), 5)) + "\n";


    if (fitData.sRestrictions.length())
        sFitParameterTable += sPrefix + _lang.get("PARSERFUNCS_FIT_PARAM_RESTRICTS", fitData.sRestrictions) + "\n";

    sFitParameterTable += sPrefix + _lang.get("PARSERFUNCS_FIT_FREEDOMS", toString(nSize - paramsMap.size())) + "\n";
    sFitParameterTable += sPrefix + _lang.get("PARSERFUNCS_FIT_ALGORITHM_SETTINGS", toString(fitData.eps_rel, 5), toString(fitData.eps_abs, 5), toString(fitData.nMaxIterations)) + "\n";
    sFitParameterTable += sPrefix + _lang.get("PARSERFUNCS_FIT_ITERATIONS", toString(_fControl.getIterations())) + "\n";

    if (nSize != paramsMap.size() && !(fitData.nFitVars & 2))
    {
        sFitParameterTable += sPrefix + _lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7)) + "\n";
        sFitParameterTable += sPrefix + _lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7)) + "\n";
        sFitParameterTable += sPrefix + _lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7)) + "\n";
    }
    else if (fitData.nFitVars & 2 && nSize != paramsMap.size() )
    {
        sFitParameterTable += sPrefix + _lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7)) + "\n";
        sFitParameterTable += sPrefix + _lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7)) + "\n";
        sFitParameterTable += sPrefix + _lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7)) + "\n";
    }

    return sFitParameterTable;
}


/////////////////////////////////////////////////
/// \brief This static function returns the
/// layouted covariance matrix as a string for
/// printing in file and to terminal.
///
/// \param fitData FittingData&
/// \param paramsMapSize size_t
/// \param forFitLog bool
/// \return std::string
///
/////////////////////////////////////////////////
static std::string constructCovarianceMatrix(FittingData& fitData, size_t paramsMapSize, bool forFitLog)
{
    std::string sCovMatrix;

    // Construct the whole matrix as a string
    for (size_t n = 0; n < paramsMapSize; n++)
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
        for (size_t k = 0; k < paramsMapSize; k++)
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


/////////////////////////////////////////////////
/// \brief This static function returns the
/// fitting paramters including their initial and
/// final values as a string.
///
/// \param fitData FittingData&
/// \param paramsMap mu::varmap_type&
/// \param vInitialVals const std::vector<double>&
/// \param windowSize size_t
/// \param sPMSign const std::string&
/// \param forFitLog bool
/// \return std::string
///
/////////////////////////////////////////////////
static std::string getParameterTable(FittingData& fitData, mu::varmap_type& paramsMap, const std::vector<double>& vInitialVals, size_t windowSize, const std::string& sPMSign, bool forFitLog)
{
    Settings& _option = NumeReKernel::getInstance()->getSettings();
    auto pItem = paramsMap.begin();
    std::string sParameterTable;

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
            + strfill(toString(vInitialVals[n], _option), (pItem->first.length() > (windowSize - 32) / 2 + windowSize % 2 ? 0u : (windowSize - 32) / 2 + windowSize % 2 - pItem->first.length()))
            + strfill(pItem->second->print(_option.getPrecision()), (windowSize - 50) / 2)
            + strfill(sPMSign + " " + toString(sqrt(abs(fitData.vz_w[n][n])), 5), 16);

        // Append the percentage of error compared to the final
        // value if the final value does exist
        if (fitData.vz_w[n][n])
            // Changed the position of the sqrt as it contained the parameter itself
            sParameterTable += " "
                + strfill("(" + toString(std::abs(std::sqrt(std::abs(fitData.vz_w[n][n]))
                                                  / pItem->second->front().getNum().asCF64() * 100.0), 4) + "%)", 16) + "\n";
        else
            sParameterTable += "\n";

        ++pItem;
    }

    return sParameterTable;
}


/////////////////////////////////////////////////
/// \brief This static function calculates the
/// average of the percentual fitting errors and
/// creates the variables for storing the fitting
/// errors for each parameter for further
/// calculations.
///
/// \param fitData FittingData&
/// \param paramsMap mu::varmap_type&
/// \param dChisq double
/// \return double
///
/////////////////////////////////////////////////
static double calculatePercentageAvgAndCreateParserVariables(FittingData& fitData, mu::varmap_type& paramsMap, double dChisq)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    std::string sErrors = "";
    auto pItem = paramsMap.begin();
    double dAverageErrorPercentage = 0.0;

    // Go through all fitting parameters and summarize the
    // percentual errors and construct error variables, if the
    // user required them
    for (size_t n = 0; n < paramsMap.size(); n++)
    {
        if (pItem == paramsMap.end())
            break;

        // Add the percentage value
        if (fitData.vz_w[n][n])
        {
            dAverageErrorPercentage += abs(sqrt(abs(fitData.vz_w[n][n] / (*(pItem->second)).front().as_cmplx())) * 100.0);
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


/////////////////////////////////////////////////
/// \brief This static function returns the
/// fitting analysis depending upon the chi^2
/// value and the sum of the percentual errors.
///
/// \param _fControl Fitcontroller&
/// \param fitData FittingData&
/// \param dNormChisq double
/// \param dAverageErrorPercentage double
/// \param noOverfitting bool
/// \return std::string
///
/////////////////////////////////////////////////
static std::string getFitAnalysis(Fitcontroller& _fControl, FittingData& fitData, double dNormChisq, double dAverageErrorPercentage, bool noOverfitting)
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


/////////////////////////////////////////////////
/// \brief This static function writes the
/// contents of the logfile to a TeX file, if the
/// user requested this option.
///
/// \param _fControl Fitcontroller&
/// \param sTeXExportFile const std::string&
/// \param sCmd const std::string&
/// \param paramsMap mu::varmap_type&
/// \param fitData FittingData&
/// \param vInitialVals const std::vector<double>&
/// \param nSize size_t
/// \param sFitAnalysis const std::string&
/// \param sFuncDisplay const std::string&
/// \param sFittedFunction const std::string&
/// \param dChisq double
/// \return void
///
/////////////////////////////////////////////////
static void createTeXExport(Fitcontroller& _fControl, const std::string& sTeXExportFile, const std::string& sCmd, mu::varmap_type& paramsMap, FittingData& fitData, const std::vector<double>& vInitialVals, size_t nSize, const std::string& sFitAnalysis, const std::string& sFuncDisplay, const std::string& sFittedFunction, double dChisq)
{
    std::ofstream oTeXExport;

    oTeXExport.open(sTeXExportFile.c_str(), std::ios_base::trunc);

    // Ensure that the file stream can be opened
    if (oTeXExport.fail())
    {
        oTeXExport.close();
        NumeReKernel::printPreFmt("\n");
        throw SyntaxError(SyntaxError::CANNOT_OPEN_TARGET, sCmd, SyntaxError::invalid_position, sTeXExportFile);
    }

    // Write the headline to the TeX file
    oTeXExport << "%\n% " << _lang.get("OUTPUT_PRINTLEGAL_TEX") << "\n%" << std::endl;
    oTeXExport << "\\section{" << _lang.get("PARSERFUNCS_FIT_HEADLINE") << ": " << getTimeStamp(false)  << "}" << std::endl;
    oTeXExport << "\\begin{itemize}" << std::endl;
    oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_FUNCTION", "$" + replaceToTeX(sFuncDisplay, true) + "$") << std::endl;
    oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_FITTED_FUNC", "$" + replaceToTeX(sFittedFunction, true) + "$") << std::endl;

    if (fitData.bUseErrors)
        oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_POINTS_W_ERR", toString(nSize)) << std::endl;
    else
        oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_POINTS_WO_ERR", toString(nSize)) << std::endl;

    if (fitData.restricted[0])
        oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "x", toString(fitData.ivl[0].min(), 5), toString(fitData.ivl[0].max(), 5)) << std::endl;

    if (fitData.restricted[1])
        oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "y", toString(fitData.ivl[1].min(), 5), toString(fitData.ivl[1].max(), 5)) << std::endl;

    if (fitData.restricted[2])
        oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_COORD_RESTRICTS", "z", toString(fitData.ivl[2].min(), 5), toString(fitData.ivl[2].max(), 5)) << std::endl;

    if (fitData.sRestrictions.length())
        oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_PARAM_RESTRICTS", "$" + replaceToTeX(fitData.sRestrictions, true) + "$") << std::endl;

    oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_FREEDOMS", toString(nSize - paramsMap.size())) << std::endl;
    oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_ALGORITHM_SETTINGS", toString(fitData.eps_rel, 5), toString(fitData.eps_abs, 5), toString(fitData.nMaxIterations)) << std::endl;
    oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_ITERATIONS", toString(_fControl.getIterations())) << std::endl;

    // Write the calculated fitting result values to the TeX file
    if (nSize != paramsMap.size() && !(fitData.nFitVars & 2))
    {
        std::string sChiReplace = _lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7));
        sChiReplace.replace(sChiReplace.find("chi^2"), 5, "$\\chi^2$");
        oTeXExport << "\t\\item " << sChiReplace << std::endl;
        sChiReplace = _lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7));
        sChiReplace.replace(sChiReplace.find("chi^2"), 5, "$\\chi^2$");
        oTeXExport << "\t\\item " << sChiReplace << std::endl;
        oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7)) << std::endl;
    }
    else if (fitData.nFitVars & 2 && nSize != paramsMap.size())
    {
        std::string sChiReplace = _lang.get("PARSERFUNCS_FIT_CHI2", toString(dChisq, 7));
        sChiReplace.replace(sChiReplace.find("chi^2"), 5, "$\\chi^2$");
        oTeXExport << "\t\\item " << sChiReplace << std::endl;
        sChiReplace = _lang.get("PARSERFUNCS_FIT_RED_CHI2", toString(dChisq / (double) (nSize - paramsMap.size()), 7));
        sChiReplace.replace(sChiReplace.find("chi^2"), 5, "$\\chi^2$");
        oTeXExport << "\t\\item " << sChiReplace << std::endl;
        oTeXExport << "\t\\item " << _lang.get("PARSERFUNCS_FIT_STD_DEV", toString(sqrt(_fControl.getFitChi() / (double)(nSize - paramsMap.size())), 7)) << std::endl;
    }

    // Start the table for the fitting parameters
    oTeXExport << "\\end{itemize}\n\\begin{table}[htb]\n\t\\centering\n\t\\begin{tabular}{cccc}\n\t\t\\toprule" << std::endl;

    // Write the headline for the fitting parameters
    if (fitData.bUseErrors)
        oTeXExport << "\t\t" << _lang.get("PARSERFUNCS_FIT_PARAM") << " & "
                   << _lang.get("PARSERFUNCS_FIT_INITIAL") << " & "
                   << _lang.get("PARSERFUNCS_FIT_FITTED") << " & "
                   << _lang.get("PARSERFUNCS_FIT_PARAM_DEV") << "\\\\" << std::endl;
    else
        oTeXExport << "\t\t" << _lang.get("PARSERFUNCS_FIT_PARAM") << " & "
                   << _lang.get("PARSERFUNCS_FIT_INITIAL") << " & "
                   << _lang.get("PARSERFUNCS_FIT_FITTED") << " & "
                   << _lang.get("PARSERFUNCS_FIT_ASYMPTOTIC_ERROR") << "\\\\" << std::endl;

    oTeXExport << "\t\t\\midrule" << std::endl;

    auto pItem = paramsMap.begin();
    std::string sErrors = "";
    std::string sPMSign = " ";

    // Write the fitting parameters linewise to the table
    for (size_t n = 0; n < paramsMap.size(); n++)
    {
        if (pItem == paramsMap.end())
            break;

        oTeXExport << "\t\t$" <<  replaceToTeX(pItem->first, true) << "$ & $"
                   << vInitialVals[n] << "$ & $"
                   << pItem->second->front().printVal() << "$ & $\\pm"
                   << sqrt(abs(fitData.vz_w[n][n]));

        // Append the percentual error value, if the current parameter
        // is non-zero.
        if (fitData.vz_w[n][n])
        {
            oTeXExport << " \\quad (" + toString(abs(sqrt(abs(fitData.vz_w[n][n] / (pItem->second->front().as_cmplx()))) * 100.0), 4) + "\\%)$\\\\" << std::endl;
        }
        else
            oTeXExport << "$\\\\" << std::endl;

        ++pItem;
    }

    // Close the fitting parameter table
    oTeXExport << "\t\t\\bottomrule\n\t\\end{tabular}\n\\end{table}" << std::endl;

    // Write the correlation matrix
    if (paramsMap.size() > 1 && paramsMap.size() != nSize)
    {
        oTeXExport << "\n\\subsection{" << _lang.get("PARSERFUNCS_FIT_CORRELMAT_HEAD") << "}" << std::endl;
        oTeXExport << "\\[\n\t\\begin{pmatrix}" << std::endl;

        for (size_t n = 0; n < paramsMap.size(); n++)
        {
            oTeXExport << "\t\t";

            for (size_t k = 0; k < paramsMap.size(); k++)
            {
                oTeXExport << fitData.vz_w[n][k] / sqrt(fabs(fitData.vz_w[n][n]*fitData.vz_w[k][k]));

                if (k + 1 < paramsMap.size())
                    oTeXExport << " & ";
            }

            oTeXExport << "\\\\" << std::endl;
        }

        oTeXExport << "\t\\end{pmatrix}\n\\]" << std::endl;

    }

    oTeXExport << "\n\\subsection{" << _lang.get("PARSERFUNCS_FIT_ANALYSIS") << "}" << std::endl;
    oTeXExport << sFitAnalysis << std::endl;
    oTeXExport.close();
}


