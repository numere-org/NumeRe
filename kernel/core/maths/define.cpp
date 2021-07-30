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

#include "define.hpp"
#include "../../kernel.hpp"

using namespace mu;

//////////////////////////////////
// CLASS FUNCTIONDEFINITION
//////////////////////////////////

/////////////////////////////////////////////////
/// \brief Constructor of the FunctionDefinition
/// class. Creates a definition from the passed
/// definition string.
///
/// \param _sDefinitionString const string&
///
/////////////////////////////////////////////////
FunctionDefinition::FunctionDefinition(const string& _sDefinitionString)
{
    sDefinitionString = _sDefinitionString;

    if (sDefinitionString.length())
        decodeDefinition();
}


/////////////////////////////////////////////////
/// \brief Assignment operator overload for the
/// Function definition class.
///
/// \param _def const FunctionDefinition&
/// \return FunctionDefinition&
///
/////////////////////////////////////////////////
FunctionDefinition& FunctionDefinition::operator=(const FunctionDefinition& _def)
{
    sDefinitionString = _def.sDefinitionString;
    sSignature = _def.sSignature;
    sName = _def.sName;
    vArguments = _def.vArguments;
    sComment = _def.sComment;
    sParsedDefinitionString = _def.sParsedDefinitionString;

    return *this;
}


/////////////////////////////////////////////////
/// \brief This member function parses the call
/// to the contained function definition and
/// returns a function definition string
/// containing the passed values.
///
/// \param _sArgList const string&
/// \return string
///
/////////////////////////////////////////////////
string FunctionDefinition::parse(const string& _sArgList)
{
    string sParsedDefinition = sParsedDefinitionString;
    static const string sOperators = "+-*/^&|!?:{";
    string sArgs = _sArgList;

    vector<string> vArg;

    // Split the arguments
    while (sArgs.length())
    {
        vArg.push_back(getNextArgument(sArgs, true));
    }

    // Clear the vector, if the first and single
    // argument is empty
    if (vArg.size() == 1 && !vArg[0].length())
    {
        vArg.clear();
    }

    // check, whether the arguments contain operators,
    // which means, they are expressions and not only
    // values -> They have to be surrounded by parentheses
    for (unsigned int j = 0; j < vArg.size(); j++)
    {
        StripSpaces(vArg[j]);

        if (!vArg[j].length())
            vArg[j] = "0";

        // Do nothing, if the argument is already surrounded
        // with parentheses
        if (vArg[j].front() == '(' && vArg[j].back() == ')')
            continue;

        // Search for operators
        if (vArg[j].find_first_of(sOperators) != string::npos)
        {
            vArg[j] = "(" + vArg[j] + ")";
        }
    }

    // Replace the variables in the definition with
    // the passed arguments
    for (size_t n = 0; n < vArguments.size(); n++)
    {
        // If fewer values than required are passed,
        // append additional zeros
        if (n >= vArg.size())
            vArg.push_back("0");

        if (!vArg[n].length())
            vArg[n] = "0";

        // If the current argument is the dotdotdot
        // place holder, simply concatenate all following
        // input arguments
        if (vArguments[n] == ">>...<<")
        {
            if (n+1 < vArg.size())
            {
                for (unsigned int k = n+1; k < vArg.size(); k++)
                    vArg[n] += "," + vArg[k];

                vArg.erase(vArg.begin()+n+1, vArg.end());
            }
        }

        // Replace each occurence
        while (sParsedDefinition.find(vArguments[n]) != string::npos)
        {
            sParsedDefinition.replace(sParsedDefinition.find(vArguments[n]), vArguments[n].length(), vArg[n]);
        }
    }

    // Ensure that the number of passed values is not
    // larger than the intended number of arguments
    if (vArg.size() > vArguments.size())
        throw SyntaxError(SyntaxError::TOO_MANY_ARGS_FOR_DEFINE, sDefinitionString, SyntaxError::invalid_position);

    // Remove obsolete surrounding whitespaces
    StripSpaces(sParsedDefinition);

    return sParsedDefinition;
}


/////////////////////////////////////////////////
/// \brief This member function creates the save
/// string used for writing to the definition
/// file.
///
/// \return string
///
/////////////////////////////////////////////////
string FunctionDefinition::exportFunction() const
{
    string sExport = sName + "; " + sParsedDefinitionString + "; " + sDefinitionString + "; ";

    // Append the single arguments
    for (size_t i = 0; i < vArguments.size(); i++)
        sExport += vArguments[i] + "; ";

    return sExport;
}


/////////////////////////////////////////////////
/// \brief This member function imports a
/// previously exported definition string and
/// distributes its contents along the member
/// variables.
///
/// \param _sExportedString const string&
/// \return bool
///
/////////////////////////////////////////////////
bool FunctionDefinition::importFunction(const string& _sExportedString)
{
    string sImport = _sExportedString;
    vArguments.clear();

    // Decode function identifier
    sName = sImport.substr(0, sImport.find(';'));
    StripSpaces(sName);
    sImport.erase(0, sImport.find(';')+1);

    // Decode parsed expression
    sParsedDefinitionString = sImport.substr(0, sImport.find(';'));
    StripSpaces(sParsedDefinitionString);
    sImport.erase(0, sImport.find(';')+1);

    // Decode the original definition string
    sDefinitionString = sImport.substr(0, sImport.find(';'));
    StripSpaces(sDefinitionString);
    sImport.erase(0, sImport.find(';')+1);

    // Recreate the signature
    sSignature = sDefinitionString.substr(0, getMatchingParenthesis(sDefinitionString)+1);
    StripSpaces(sSignature);


    // Separate the comment
    if (findParameter(sDefinitionString, "comment", '='))
    {
        sComment = getArgAtPos(sDefinitionString, findParameter(sDefinitionString, "comment", '=')+7);
    }

    StripSpaces(sImport);

    // Decode the variable list
    while (sImport.length())
    {
        vArguments.push_back(sImport.substr(0, sImport.find(';')));
        StripSpaces(vArguments.back());
        sImport.erase(0, sImport.find(';')+1);
        StripSpaces(sImport);
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function appends a
/// comment, which might be set after the
/// definition.
///
/// \param _sComment const string&
/// \return bool
///
/////////////////////////////////////////////////
bool FunctionDefinition::appendComment(const string& _sComment)
{
    sComment += _sComment;

    // Replace a previous comment, if there's one available
    if (findParameter(sDefinitionString, "comment", '='))
    {
        size_t pos = findParameter(sDefinitionString, "comment", '=')+7;
        string sTemp = getArgAtPos(sDefinitionString, pos);
        pos = sDefinitionString.find(sTemp, pos);

        sDefinitionString.replace(pos, sTemp.length(), sComment);
    }
    else
        sDefinitionString += " -set comment=\"" + sComment + "\"";

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// definition of the function without the
/// appended parameters.
///
/// \return string
///
/////////////////////////////////////////////////
string FunctionDefinition::getDefinition() const
{
    string sDefinition = sDefinitionString.substr(sDefinitionString.find(":=")+2);

    // Remove trailing parameters
    if (sDefinition.find("-set") != string::npos)
        sDefinition.erase(sDefinition.find("-set"));
    else if (sDefinition.find("--") != string::npos)
        sDefinition.erase(sDefinition.find("--"));

    StripSpaces(sDefinition);

    return sDefinition;
}


/////////////////////////////////////////////////
/// \brief This private member function decodes
/// the definition in the private member variable
/// "sDefinitionString" into single fields.
///
/// \return bool
///
/////////////////////////////////////////////////
bool FunctionDefinition::decodeDefinition()
{
    // Store the function identifier
    sName = sDefinitionString.substr(0, sDefinitionString.find('('));
    StripSpaces(sName);

    // Store the signature
    sSignature = sDefinitionString.substr(0, getMatchingParenthesis(sDefinitionString)+1);
    StripSpaces(sSignature);

    // catch errors: numbers at the beginning or usual operators
    // in the name
    if (sName[0] >= '0' && sName[0] <= '9')
    {
        throw SyntaxError(SyntaxError::NO_NUMBER_AT_POS_1, sDefinitionString, SyntaxError::invalid_position, sName);
    }

    string sDelim = "+-*/^!=&| ><()?[]{}$%§~#:.,;";

    if (sName.find_first_of(sDelim) != string::npos)
    {
        SyntaxError(SyntaxError::FUNCTION_NAMES_MUST_NOT_CONTAIN_SIGN, sDefinitionString, SyntaxError::invalid_position, sName.substr(sName.find_first_of(sDelim), 1));
    }

    // Separate the comment
    if (findParameter(sDefinitionString, "comment", '='))
    {
        sComment = getArgAtPos(sDefinitionString, findParameter(sDefinitionString, "comment", '=')+7);
    }

    // Create the initial version of the parsed definition
    // by extracting the expression from the definition
    // string
    sParsedDefinitionString = sDefinitionString.substr(sDefinitionString.find(":=")+2);

    if (sParsedDefinitionString.find("-set") != string::npos)
        sParsedDefinitionString.erase(sParsedDefinitionString.find("-set"));
    else if (sParsedDefinitionString.find("--") != string::npos)
        sParsedDefinitionString.erase(sParsedDefinitionString.find("--"));

    // Surround the definition string with whitespaces for
    // simplifying the use of "checkDelimiter"
    sParsedDefinitionString = " " + sParsedDefinitionString + " ";

    // Split the argument list and validate its contents
    splitAndValidateArguments();

    // This section handles variables, which shall be
    // stored as values instead variables
    convertToValues();

    // Replace all occurences in the expression with
    // new placeholders
    replaceArgumentOccurences();

    // If the parsed definition string is multivalue
    // surround it with braces
    if (isMultiValue(sParsedDefinitionString))
        sParsedDefinitionString = "{" + sParsedDefinitionString + "}";

    return true;
}


/////////////////////////////////////////////////
/// \brief This private member function validates
/// the arguments of the function definition.
///
/// \return bool
///
/////////////////////////////////////////////////
bool FunctionDefinition::splitAndValidateArguments()
{
    string sDelim = "+-*/^!=&| ><()?[]{}$%§~#:.,;";

    // Get the argument list of the definition
    string sArguments = sDefinitionString.substr(sDefinitionString.find('('));
    sArguments.erase(getMatchingParenthesis(sArguments));
    sArguments.erase(0, 1);

    StripSpaces(sArguments);

    // Split the arguments using "getNextArgument"
    while (sArguments.length())
    {
        vArguments.push_back(getNextArgument(sArguments, true));
    }

    // Validate the arguments
    for (size_t i = 0; i < vArguments.size(); i++)
    {
        // Ellipsis shall always be the last argument
        if (vArguments[i] == "..." && i + 1 < vArguments.size())
        {
           throw SyntaxError(SyntaxError::ELLIPSIS_MUST_BE_LAST_ARG, sDefinitionString, SyntaxError::invalid_position);
        }

        // All other arguments shall not contain
        // delimiters
        if (vArguments[i] != "...")
        {
            if (vArguments[i].find_first_of(sDelim) != string::npos)
            {
                throw SyntaxError(SyntaxError::FUNCTION_ARGS_MUST_NOT_CONTAIN_SIGN, sDefinitionString, SyntaxError::invalid_position, vArguments[i].substr(vArguments[i].find_first_of(sDelim), 1));
            }
        }
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This private member function converts
/// the selected of the passed variables into
/// their values.
///
/// \return bool
///
/////////////////////////////////////////////////
bool FunctionDefinition::convertToValues()
{
    if (findParameter(sDefinitionString, "asval", '='))
    {
        string sAsVal = getArgAtPos(sDefinitionString, findParameter(sDefinitionString, "asval", '=')+5);

        if (sAsVal.front() == '{')
            sAsVal.erase(0,1);

        if (sAsVal.back() == '}')
            sAsVal.pop_back();

        // Parse the list numerically
        NumeReKernel::getInstance()->getParser().SetExpr(sAsVal);
        NumeReKernel::getInstance()->getParser().Eval();
        mu::varmap_type mAsVal = NumeReKernel::getInstance()->getParser().GetUsedVar();

        // Replace each occurence of the parsed variables
        // with their value
        for (auto iter = mAsVal.begin(); iter != mAsVal.end(); ++iter)
        {
            for (unsigned int i = 0; i < sParsedDefinitionString.length(); i++)
            {
                if (sParsedDefinitionString.substr(i, (iter->first).length()) == iter->first && checkDelimiter(sParsedDefinitionString.substr(i-1, (iter->first).length()+2)))
                {
                    sParsedDefinitionString.replace(i, (iter->first).length(), toString(*iter->second, NumeReKernel::getInstance()->getSettings().getPrecision()));
                }
            }
        }
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This private member function replaces
/// all occurences of the passed arguments with
/// the corresponding new placeholders.
///
/// \return bool
///
/////////////////////////////////////////////////
bool FunctionDefinition::replaceArgumentOccurences()
{
    // Replace all occurences in the expression with
    // new placeholders
    for (size_t i = 0; i < vArguments.size(); i++)
    {
        // create new position index
        size_t nPos = 0;

        // Search for the next occurence of the variable
        while ((nPos = sParsedDefinitionString.find(vArguments[i], nPos)) != string::npos)
        {
            // check, whether the found match is an actual variable
            if (checkDelimiter(sParsedDefinitionString.substr(nPos-1, vArguments[i].length() + 2)))
            {
                // replace VAR with >>VAR<< and increment the
                // position index by the variable length + 4
                sParsedDefinitionString.replace(nPos, vArguments[i].length(), ">>" + vArguments[i] + "<<");
                nPos += vArguments[i].length() + 4;
            }
            else
                nPos += vArguments[i].length();
        }

        // Replace the variable itself with >>VAR<<
        vArguments[i] = ">>" + vArguments[i] + "<<";
    }

    return true;
}



//////////////////////////////////
// CLASS FUNCTIONDEFINITIONMANAGER
//////////////////////////////////

/////////////////////////////////////////////////
/// \brief Default constructor of the
/// FunctionDefinitionManager class. Prepares the
/// list of protected command strings.
/////////////////////////////////////////////////
FunctionDefinitionManager::FunctionDefinitionManager(bool _isLocal) : FileSystem()
{
    sBuilt_In.clear();
    sCommands = ",for,if,while,endfor,endwhile,endif,else,elseif,continue,break,explicit,procedure,endprocedure,throw,return,switch,case,endswitch,default,";
    sFileName = "<>/functions.def";
    sTables = "";
    isLocal = _isLocal;
}


/////////////////////////////////////////////////
/// \brief FunctionDefinitionManager copy
/// constructor. Delegates first to the default
/// constructor.
///
/// \param _defined const FunctionDefinitionManager&
///
/////////////////////////////////////////////////
FunctionDefinitionManager::FunctionDefinitionManager(const FunctionDefinitionManager& _defined) : FunctionDefinitionManager(true)
{
    sFileName = _defined.sFileName;
    sTables = _defined.sTables;

    mFunctionsMap = _defined.mFunctionsMap;
    isLocal = _defined.isLocal;
}


/////////////////////////////////////////////////
/// \brief This private member function resolves
/// recursive definitions, which are handled by
/// replacing the occuring function calls with
/// their previous definition.
///
/// \param sDefinition string
/// \return string
///
/////////////////////////////////////////////////
string FunctionDefinitionManager::resolveRecursiveDefinitions(string sDefinition)
{
    // Get the different parts of the definition
    string sFunctionName = sDefinition.substr(0, sDefinition.find('(')+1);
    string sFunction = sDefinition.substr(sDefinition.find(":=")+2);
    string sFuncOccurence;

    // Remove obsolete surrounding whitespaces
    StripSpaces(sFunction);
    StripSpaces(sFunctionName);

    // Replace each occurence of this function by its previous
    // definition
    for (unsigned int i = 0; i < sFunction.length(); i++)
    {
        if (sFunction.substr(i, sFunctionName.length()) == sFunctionName
            && (!i || !isalnum(sFunction[i-1])))
        {
            sFuncOccurence = sFunction.substr(i, sFunction.find(')', i) + 1 - i);

            if (!call(sFuncOccurence))
            {
                return "";
            }

            StripSpaces(sFuncOccurence);
            sFunction.replace(i, sFunction.find(')', i) + 1 - i, sFuncOccurence);
        }
    }

    // Reconstruct the function definition using the
    // replaced definition string
    return sDefinition.substr(0, sDefinition.find(":=")) + " := " + sFunction;
}


/////////////////////////////////////////////////
/// \brief This private member function returns
/// the iterator to the function pointed by the
/// passed ID.
///
/// \param id size_t
/// \return map<string, FunctionDefinition>::const_iterator
///
/////////////////////////////////////////////////
map<string, FunctionDefinition>::const_iterator FunctionDefinitionManager::findItemById(size_t id) const
{
    if (id >= mFunctionsMap.size())
        return mFunctionsMap.end();

    map<string, FunctionDefinition>::const_iterator iter = mFunctionsMap.begin();

    // As long as the ID is not zero,
    // we increment the iterator and
    // decrement the ID
    while (iter != mFunctionsMap.end())
    {
        // Return the current iterator,
        // if the ID is zero
        if (!id)
            return iter;

        ++iter;
        id--;
    }

    // Return the end, if the ID does not
    // correspond to any item
    return mFunctionsMap.end();
}


/////////////////////////////////////////////////
/// \brief This method checks, whether the passed
/// function is already defined.
///
/// \param sFunc const string&
/// \return bool
///
/////////////////////////////////////////////////
bool FunctionDefinitionManager::isDefined(const string& sFunc)
{
    // Only use the function name
    string sToLocate = sFunc.substr(0,sFunc.find('('));
    StripSpaces(sToLocate);

    if (mFunctionsMap.find(sToLocate) != mFunctionsMap.end())
        return true;

    // If it is not already defined by the user, check additionally,
    // whether the passed function identifier corresponds either to
    // a built-in function or a protected command
    if (sBuilt_In.find("," + sToLocate + "(") != string::npos || sCommands.find("," + sToLocate + ",") != string::npos)
        return true;
    else
        return false;
}


/////////////////////////////////////////////////
/// \brief This function defines a custom
/// function, by passing it to a new
/// FunctionDefinition class instance.
///
/// \param sExpr const string&
/// \param bRedefine bool
/// \param bFallback bool
/// \return bool
///
/////////////////////////////////////////////////
bool FunctionDefinitionManager::defineFunc(const string& sExpr, bool bRedefine, bool bFallback)
{
    if (!NumeReKernel::getInstance())
        return false;

    // Handle the case that the user simply wanted to append
    // a comment to the existing definition
    if (bRedefine && sExpr.find("()") != string::npos && findParameter(sExpr, "comment", '=') && mFunctionsMap.size())
    {
        string sComment = getArgAtPos(sExpr, findParameter(sExpr, "comment", '=')+7);

        if (mFunctionsMap.find(sExpr.substr(0, sExpr.find("()"))) != mFunctionsMap.end())
        {
            mFunctionsMap[sExpr.substr(0, sExpr.find("()"))].appendComment(sComment);

            return true;
        }

        return false;
    }

    // Catch all possible syntax errors at this location
    if (sExpr.find('(') == string::npos
        || sExpr.find('(') > sExpr.find(":=")
        || sBuilt_In.find(","+sExpr.substr(0,sExpr.find('(')+1)) != string::npos
        || (sTables.length() && sTables.find(";"+sExpr.substr(0,sExpr.find('('))+";") != string::npos)
        || sCommands.find(","+sExpr.substr(0,sExpr.find('('))+",") != string::npos
        || sExpr.find(":=") == string::npos)
    {
        // Throw the corresponding error messages
        if (sExpr.find(":=") == string::npos)
            throw SyntaxError(SyntaxError::CANNOT_FIND_DEFINE_OPRT, sExpr, SyntaxError::invalid_position);
        else if (sExpr.find('(') == string::npos || sExpr.find('(') > sExpr.find(":="))
            throw SyntaxError(SyntaxError::CANNOT_FIND_FUNCTION_ARGS, sExpr, SyntaxError::invalid_position);
        else if (sBuilt_In.find(","+sExpr.substr(0,sExpr.find('(')+1)) != string::npos)
            throw SyntaxError(SyntaxError::FUNCTION_IS_PREDEFINED, sExpr, SyntaxError::invalid_position, sExpr.substr(0,sExpr.find('(')));
        else if (sTables.length() && sTables.find(";"+sExpr.substr(0,sExpr.find('('))+";") != string::npos)
            throw SyntaxError(SyntaxError::CACHE_ALREADY_EXISTS, sExpr, SyntaxError::invalid_position, sExpr.substr(0,sExpr.find('(')));
        else if (sCommands.find(","+sExpr.substr(0,sExpr.find('('))+",") != string::npos)
            throw SyntaxError(SyntaxError::FUNCTION_STRING_IS_COMMAND, sExpr, SyntaxError::invalid_position, sExpr.substr(0,sExpr.find('(')));
        else
            throw SyntaxError(SyntaxError::CANNOT_FIND_FUNCTION_ARGS, sExpr, SyntaxError::invalid_position);
    }

    // Check, whether the passed function is not already defined
    if (!bRedefine && isDefined(sExpr))
        throw SyntaxError(SyntaxError::FUNCTION_ALREADY_EXISTS, sExpr, SyntaxError::invalid_position, sExpr.substr(0, sExpr.find('(')));

    string sDefinition = sExpr;

    // Handle recursive redefinitions
    if (bRedefine && findParameter(sExpr, "recursive"))
        sDefinition = resolveRecursiveDefinitions(sDefinition);

    // Prepare the function definition and the possible
    // needed fallback
    FunctionDefinition definition(sDefinition);
    FunctionDefinition fallback;

    // Get the function string
    string sFunctionString = definition.getDefinition().substr(definition.getDefinition().find(":=")+2);

    // check, whether the new function would result in an
    // endless recursion
    if (!call(sFunctionString))
        return false;

    // Store the fallback definition
    if (bRedefine && isDefined(sDefinition))
        fallback = mFunctionsMap[definition.sName];

    // "define" the function
    mFunctionsMap[definition.sName] = definition;

    // Recreate the function string
    sFunctionString = definition.getDefinition().substr(definition.getDefinition().find(":=")+2);

    // Recheck, if the defined function will result in an
    // endless loop and revert the definition, if this is
    // the case
    try
    {
        if (!call(sFunctionString))
        {
            if (!bRedefine)
                mFunctionsMap.erase(definition.sName);
            else
                mFunctionsMap[definition.sName] = fallback;

            return false;
        }
    }
    catch (...)
    {
        if (!bRedefine)
            mFunctionsMap.erase(definition.sName);
        else
            mFunctionsMap[definition.sName] = fallback;

        throw;
    }

    if (!isLocal)
        NumeReKernel::getInstance()->refreshFunctionTree();

    return true;
}


/////////////////////////////////////////////////
/// \brief This function removes a previously
/// defined function from the internal memory.
///
/// \param sFunc const string&
/// \return bool
///
/////////////////////////////////////////////////
bool FunctionDefinitionManager::undefineFunc(const string& sFunc)
{
    if (mFunctionsMap.find(sFunc.substr(0, sFunc.find('('))) != mFunctionsMap.end())
        mFunctionsMap.erase(sFunc.substr(0, sFunc.find('(')));

    if (isLocal)
        return true;

    // Update the definition file, if the corresponding setting
    // is active
    //if (NumeReKernel::getInstance() && NumeReKernel::getInstance()->getSettings().controlDefinitions())
    //    save(NumeReKernel::getInstance()->getSettings());

    NumeReKernel::getInstance()->refreshFunctionTree();
    return true;
}


/////////////////////////////////////////////////
/// \brief This function searches for known
/// custom definitions in the passed expression
/// and replaces them with their parsed
/// definition strings.
///
/// \param sExpr string&
/// \param nRecursion int
/// \return bool
///
/////////////////////////////////////////////////
bool FunctionDefinitionManager::call(string& sExpr, int nRecursion)
{
    if (!NumeReKernel::getInstance())
        return false;

    string sTemp = "";
    string sImpFunc = "";
    bool bDoRecursion = false;

    if (!sExpr.length())
        return true;

    // Check, whether we've a possible candidate for an endless
    // loop: if the number of recursions is twice the number of
    // known function definitions, we've probably replaced every
    // function twice, so we do not expect to terminate it.
    if ((unsigned)nRecursion == mFunctionsMap.size()*2 + 1)
    {
        throw SyntaxError(SyntaxError::TOO_MANY_FUNCTION_CALLS, sExpr, SyntaxError::invalid_position);
    }

    /* --> Ergaenze ggf. Leerzeichen vor und nach dem Ausdruck, damit die Untersuchung der Zeichen vor und
     *     nach einem Treffer keinen SEG-FAULT wirft <--
     */
    if (sExpr[0] != ' ')
        sExpr = " " + sExpr;

    if (sExpr[sExpr.length()-1] != ' ')
        sExpr += " ";

    // search through the whole set of definitions for
    // possible matches
    for (auto iter = mFunctionsMap.begin(); iter != mFunctionsMap.end(); ++iter)
    {
        size_t nPos = 0;

        // Is there a possible match?
        if (sExpr.find(iter->second.sName + "(") != string::npos)
        {
            // Check for each occurence, whether the candidate
            // is an actual match and replace it with the parsed
            // definition string
            do
            {
                // Is it an actual match?
                if (!checkDelimiter(sExpr.substr(sExpr.find(iter->second.sName + "(", nPos)-1, iter->second.sName.length()+2))
                    || isInQuotes(sExpr, sExpr.find(iter->second.sName + "(", nPos), true))
                {
                    nPos = sExpr.find(iter->second.sName + "(", nPos) + iter->second.sName.length() + 1;
                    continue;
                }

                // Copy the part in front of the match into a
                // temporary buffer
                sTemp = sExpr.substr(0, sExpr.find(iter->second.sName + "(", nPos));

                // save the position of the argument's opening
                // parenthesis
                nPos = sExpr.find(iter->second.sName + "(",nPos) + iter->second.sName.length();

                // Copy the calling arguments
                string sArgs = sExpr.substr(nPos);
                size_t nPos_2 = getMatchingParenthesis(sArgs);

                // Check, whether the sArgs are terminated
                // by a parenthesis
                if (nPos_2 == string::npos)
                {
                    throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sExpr, nPos);
                }

                // Remove the surrounding parentheses
                sArgs.erase(nPos_2);
                sArgs.erase(0, 1);
                nPos += nPos_2 + 1;

                // Parse the function definition using the
                // passed arguments
                sImpFunc = iter->second.parse(sArgs);
                StripSpaces(sImpFunc);

                // Remove obsolete duplicated parenthesis pairs
                while (sExpr[nPos] == ')' && sExpr[nPos+1] == ')' && sTemp.back() == '(' && sTemp[sTemp.length()-2] == '(')
                {
                    nPos++;
                    sTemp.pop_back();
                }

                // Recreate the complete expression
                if (sImpFunc.front() == '{' && sImpFunc.back() == '}')
                {
                    if (sExpr[nPos] == ')' && sTemp.back() == '(')
                    {
                        if (sTemp[sTemp.length()-2] != ' ')
                        {
                            static string sDelim = "+-*/^!?:,!&|#";

                            if (sDelim.find(sTemp[sTemp.length()-2]) != string::npos)
                            {
                                sTemp.pop_back();
                                nPos++;
                            }
                        }
                        else
                        {
                            sTemp.pop_back();
                            nPos++;
                        }
                    }
                    sExpr = sTemp + sImpFunc + sExpr.substr(nPos);
                }
                else if (sTemp.back() == '(' && sExpr[nPos] == ')')
                {
                    sExpr = sTemp + sImpFunc + sExpr.substr(nPos);
                }
                else
                {
                    sExpr = sTemp + "(" + sImpFunc + ")" + sExpr.substr(nPos);
                }

                // If at least one replacement was preformed,
                // is is possible that we introduced another
                // function. Therefore we set the recursion
                // boolean to true
                if (!bDoRecursion)
                    bDoRecursion = true;
            }
            while (sExpr.find(iter->second.sName + "(", nPos) != string::npos);
        }
    }

    // If necessary, then perform another recursion
    if (bDoRecursion)
    {
        if (!call(sExpr, nRecursion+1))
            return false;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Returns the number of defined
/// functions.
///
/// \return unsigned int
///
/////////////////////////////////////////////////
unsigned int FunctionDefinitionManager::getDefinedFunctions() const
{
    return mFunctionsMap.size();
}


/////////////////////////////////////////////////
/// \brief Returns the definition string of the
/// ith defined custom function.
///
/// \param _i size_t
/// \return string
///
/////////////////////////////////////////////////
string FunctionDefinitionManager::getDefinitionString(size_t _i) const
{
    auto iter = findItemById(_i);

    if (iter != mFunctionsMap.end())
        return iter->second.sDefinitionString;

    return "";
}


/////////////////////////////////////////////////
/// \brief Returns the function signature of the
/// ith defined custom function.
///
/// \param _i size_t
/// \return string
///
/////////////////////////////////////////////////
string FunctionDefinitionManager::getFunctionSignature(size_t _i) const
{
    auto iter = findItemById(_i);

    if (iter != mFunctionsMap.end())
        return iter->second.sSignature;

    return "";
}


/////////////////////////////////////////////////
/// \brief Returns the implementation of the ith
/// defined custom function.
///
/// \param _i size_t
/// \return string
///
/////////////////////////////////////////////////
string FunctionDefinitionManager::getImplementation(size_t _i) const
{
    auto iter = findItemById(_i);

    if (iter != mFunctionsMap.end())
        return iter->second.getDefinition();

    return "";
}


/////////////////////////////////////////////////
/// \brief Returns the comment of the ith defined
/// function.
///
/// \param _i size_t
/// \return string
///
/////////////////////////////////////////////////
string FunctionDefinitionManager::getComment(size_t _i) const
{
    auto iter = findItemById(_i);

    if (iter != mFunctionsMap.end())
        return iter->second.sComment;

    return "";
}


/////////////////////////////////////////////////
/// \brief This member function resets the
/// FunctionDefinitionManager object to a state
/// before any function was defined.
///
/// \return bool
///
/////////////////////////////////////////////////
bool FunctionDefinitionManager::reset()
{
    mFunctionsMap.clear();

    return true;
}


/////////////////////////////////////////////////
/// \brief This function saves the function
/// definitions to the definition file.
///
/// \param _option const Settings&
/// \return bool
///
/////////////////////////////////////////////////
bool FunctionDefinitionManager::save(const Settings& _option)
{
    sFileName = FileSystem::ValidFileName(sFileName, ".def");
    ofstream ofDefineFile;

    // Do not save anything, if the map is empty
    if (mFunctionsMap.size())
    {
        if (_option.systemPrints())
            NumeReKernel::printPreFmt("|-> " + toSystemCodePage(_lang.get("DEFINE_SAVING_FUNCTIONS")) + " ... ");

        // Open the definition file
        ofDefineFile.open(sFileName.c_str(), ios_base::trunc);

        // Ensure that the file stream is in a
        // valid state
        if (ofDefineFile.good())
        {
            ofDefineFile << "# This file saves the function definitions. Do not edit unless you know, what you're doing!" << endl;

            // Save each definition
            for (auto iter = mFunctionsMap.begin(); iter != mFunctionsMap.end(); ++iter)
            {
                ofDefineFile << iter->second.exportFunction() << endl;
            }

            if (_option.systemPrints())
                NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_SUCCESS")) + ".\n");

            return true;
        }

        return false;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This function loads previously saved
/// function definitions to memory.
///
/// \param _option const Settings&
/// \param bAutoLoad bool
/// \return bool
///
/////////////////////////////////////////////////
bool FunctionDefinitionManager::load(const Settings& _option, bool bAutoLoad)
{
    sFileName = FileSystem::ValidFileName(sFileName, ".def");
    ifstream ifDefinedFile;
    string sInputLine;

    // Open the definition file
    ifDefinedFile.open(sFileName.c_str());

    // Check, whether the file stream is in
    // a valid state, aka the file exists
    if (ifDefinedFile.good())
    {
        if (_option.systemPrints() && !bAutoLoad)
            NumeReKernel::printPreFmt("|-> " + toSystemCodePage(_lang.get("DEFINE_LOADING_FUNCTIONS")) + " ... ");

        // Read every line of the definition file
        while (!ifDefinedFile.eof())
        {
            getline(ifDefinedFile, sInputLine);

            // Ignore emoty lines and comments
            if (!sInputLine.length() || sInputLine.front() == '#')
                continue;

            // Create an empty definition object and import
            // the read definition
            FunctionDefinition definition;
            definition.importFunction(sInputLine);
            mFunctionsMap[definition.sName] = definition;
        }

        if (!bAutoLoad && _option.systemPrints())
        {
            NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_SUCCESS")) + ".\n");
            NumeReKernel::print(LineBreak(_lang.get("DEFINE_DONE_LOADING", toString(mFunctionsMap.size())), _option));
        }

    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function updates the
/// internal list of predefined functions. If the
/// list is whitespace-separated, it will be
/// converted into a comma-separated list.
///
/// \param sPredefined const string&
/// \return void
///
/////////////////////////////////////////////////
void FunctionDefinitionManager::setPredefinedFuncs(const string& sPredefined)
{
    sBuilt_In = sPredefined;

    if ((sBuilt_In.find(',') == string::npos || sBuilt_In.find("()") == string::npos) && sBuilt_In.find(' ') != string::npos)
    {
        sBuilt_In.insert(0, ",");
        sBuilt_In += "()";

        for (size_t i = 0; i < sBuilt_In.length(); i++)
        {
            if (sBuilt_In[i] == ' ')
            {
                sBuilt_In.replace(i, 1, "(),");
            }
        }
    }
}






