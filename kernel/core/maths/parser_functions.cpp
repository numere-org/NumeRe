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


value_type vAns;
extern mglGraph _fontData;


/////////////////////////////////////////////////
/// \brief This function searches for the
/// selected variable in the passed string and
/// returns the position of the first occurence
/// or string::npos, if nothing was found.
///
/// \param sExpr const std::string&
/// \param sVarName const std::string&
/// \return size_t
///
/////////////////////////////////////////////////
size_t findVariableInExpression(const std::string& sExpr, const std::string& sVarName)
{
    size_t nMatch = 0;
    const static std::string sOperators = "+-*/,^!%&|?:#<>='; ";
    const static std::string sDelimiterLeft = sOperators + "([{";
#warning TODO (numere#1#12/05/21): Cannot detect variables with methods (if that is needed)
    const static std::string sDelimiterRight = sOperators + ")]}";

    // search the first match of the token, which is surrounded by the
    // defined separator characters
    while ((nMatch = sExpr.find(sVarName, nMatch)) != string::npos)
    {
        if ((!nMatch || sDelimiterLeft.find(sExpr[nMatch-1]) != string::npos)
            && (nMatch + sVarName.length() >= sExpr.length() || sDelimiterRight.find(sExpr[nMatch+sVarName.length()]) != string::npos)
            && !isInQuotes(sExpr, nMatch))
        {
            return nMatch;
        }

        nMatch++;
    }

    return std::string::npos;
}


/////////////////////////////////////////////////
/// \brief This function replaces vector
/// expressions with their corresponding multi-
/// expression equation.
///
/// \param sLine string&
/// \param _option const Settings&
/// \return void
///
/// It is used quite extensively, however, it
/// might not be necessary everywhere, because
/// the parser can cope with the vector syntax
/// now.
/////////////////////////////////////////////////
void convertVectorToExpression(string& sLine, const Settings& _option)
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
            convertVectorToExpression(sTemp, _option);

            sBuffer += sTemp + ",";
        }

        sLine = sBuffer.substr(0, sBuffer.length()-1);
        sTemp = sLine;
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

        if (nPos && (isalnum(sTemp[nPos-1]) || sTemp[nPos-1] == '_'))
        {
            // Ensure that there's a matching parenthesis
            if (getMatchingParenthesis(sTemp.substr(nPos)) == string::npos)
                throw SyntaxError(SyntaxError::INCOMPLETE_VECTOR_SYNTAX, sLine, SyntaxError::invalid_position);

            nPos += getMatchingParenthesis(sTemp.substr(nPos));
            continue;
        }

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
					sTemp = addMissingVectorComponent(vVectors[j], vScalars[j], vScalars[j + 1], bIsStringExpression);
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


/////////////////////////////////////////////////
/// \brief This function determines the value of
/// missing vector components (i.e. a vector
/// contains not enough elements compared to the
/// others used in the current expression) by
/// applying a simple heuristic to the expression.
/// It will either insert "1", "0" or an empty
/// string in string expressions.
///
/// \param sVectorComponent const string&
/// \param sLeft const string&
/// \param sRight const string&
/// \param bAddStrings bool
/// \return string
///
/////////////////////////////////////////////////
string addMissingVectorComponent(const string& sVectorComponent, const string& sLeft, const string& sRight, bool bAddStrings)
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


/////////////////////////////////////////////////
/// \brief This function returns the position of
/// the first delimiter in the passed string, but
/// it jumps over parentheses and braces.
///
/// \param sLine const string&
/// \return unsigned int
///
/////////////////////////////////////////////////
unsigned int getPositionOfFirstDelimiter(const string& sLine)
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


/////////////////////////////////////////////////
/// \brief This function is invoked, if a prompt
/// operator ("??") was found in a string.
///
/// \param __sCommand const string&
/// \return string
///
/// It will ask the user to provide the needed
/// value during the execution.
/////////////////////////////////////////////////
string promptForUserInput(const string& __sCommand)
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


/////////////////////////////////////////////////
/// \brief This function returns the pointer to
/// the passed variable.
///
/// \param sVarName const string&
/// \param _parser Parser&
/// \return double*
///
/////////////////////////////////////////////////
mu::value_type* getPointerToVariable(const string& sVarName, Parser& _parser)
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


/////////////////////////////////////////////////
/// \brief This function returns the factorial of
/// the passed integral value.
///
/// \param nNumber int
/// \return int
///
/////////////////////////////////////////////////
int integralFactorial(int nNumber)
{
	if (nNumber < 0)
		nNumber *= -1;

	if (nNumber == 0)
		return 1;

	for (int i = nNumber - 1; i > 0; i--)
		nNumber *= i;

	return nNumber;
}


/////////////////////////////////////////////////
/// \brief This function evaluates the
/// "target=TABLE()" expression and creates the
/// target table, if needed.
///
/// \param sCmd string&
/// \param sDefaultTarget const string&
/// \param _idx Indices&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \return string
///
/// If this option is not found, the function
/// will create a default target cache.
/////////////////////////////////////////////////
string evaluateTargetOptionInCommand(string& sCmd, const string& sDefaultTarget, Indices& _idx, Parser& _parser, MemoryManager& _data, const Settings& _option)
{
	string sTargetTable;

	// search for the target option in the command string
	if (findParameter(sCmd, "target", '='))
	{
		// Extract the table name
		sTargetTable = getArgAtPos(sCmd, findParameter(sCmd, "target", '=') + 6);

		// Create the target table, if it doesn't exist
		if (!_data.isTable(sTargetTable.substr(0, sTargetTable.find('(')) + "()"))
			_data.addTable(sTargetTable.substr(0, sTargetTable.find('(')), _option);

		// Read the target indices
		getIndices(sTargetTable, _idx, _parser, _data, _option);
		sTargetTable.erase(sTargetTable.find('('));

		// check the indices
		if (!isValidIndexSet(_idx))
			throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position, _idx.row.to_string() + ", " + _idx.col.to_string());

		// remove the target option and its value from the command line
		sCmd.erase(sCmd.find(getArgAtPos(sCmd, findParameter(sCmd, "target", '=') + 6), findParameter(sCmd, "target", '=') - 1), getArgAtPos(sCmd, findParameter(sCmd, "target", '=') + 6).length());
		sCmd.erase(findParameter(sCmd, "target", '=') - 1, 7);
	}
	else if (sDefaultTarget.length())
	{
		// If not found, create a default index set
		_idx.row = VectorIndex(0LL, VectorIndex::OPEN_END);
		_idx.col.front() = 0;

		// Create cache, if needed. Otherwise get first empty column
		if (_data.isTable(sDefaultTarget + "()"))
			_idx.col.front() += _data.getCols(sDefaultTarget, false);
		else
			_data.addTable(sDefaultTarget, _option);

		_idx.col.back() = VectorIndex::OPEN_END;
		sTargetTable = sDefaultTarget;
	}

	// return the target table name
	return sTargetTable;
}


/////////////////////////////////////////////////
/// \brief This function will evaluate the passed
/// indices, so that they match the dimensions of
/// the passed cache, if they are defined as open
/// ended.
///
/// \param sCache const string&
/// \param _idx Indices&
/// \param _data Datafile&
/// \return bool
///
/////////////////////////////////////////////////
bool evaluateIndices(const string& sCache, Indices& _idx, MemoryManager& _data)
{
	// Check the initial indices
	if (!isValidIndexSet(_idx))
		return false;

	// Evaluate the case for an open end index
	if (_idx.row.isOpenEnd())
		_idx.row.setRange(0, _data.getLines(sCache.substr(0, sCache.find('(')), false)-1);

	if (_idx.col.isOpenEnd())
		_idx.col.setRange(0, _data.getCols(sCache.substr(0, sCache.find('(')), false)-1);

	// Signal success
	return true;
}


/////////////////////////////////////////////////
/// \brief This static function handles a single
/// legacy interval. It's a helper for
/// readAndParseIntervals().
///
/// \param sExpr string&
/// \param sLegacyIntervalIdentifier const string&
/// \param _parser Parser&6
/// \param vInterval vector<double>&
/// \param nMinSize size_t
/// \param bEraseInterval bool
/// \return void
///
/////////////////////////////////////////////////
static void readAndParseLegacyIntervals(string& sExpr, const string& sLegacyIntervalIdentifier, Parser& _parser, vector<double>& vInterval, size_t nMinSize, bool bEraseInterval)
{
    string sInterval = getArgAtPos(sExpr, findParameter(sExpr, sLegacyIntervalIdentifier, '=') + 1);
    EndlessVector<string> indices;

    // Erase the interval definition, if needed
    if (bEraseInterval)
    {
        sExpr.erase(sExpr.find(sInterval), sInterval.length());
        size_t pos = sExpr.rfind(sLegacyIntervalIdentifier, findParameter(sExpr, sLegacyIntervalIdentifier, '='));
        sExpr.erase(pos, findParameter(sExpr, sLegacyIntervalIdentifier, '=') + 1 - pos);
    }

    // If the intervall contains a colon, split it there
    if (sInterval.find(':') != string::npos)
        indices = getAllIndices(sInterval);
    else
        indices.push_back(sInterval);

    while (vInterval.size() < nMinSize)
        vInterval.push_back(NAN);

    for (size_t i = 0; i < 2; i++)
    {
        if (isNotEmptyExpression(indices[i]))
        {
            _parser.SetExpr(indices[i]);
            vInterval.push_back(_parser.Eval().real());
        }
        else
            vInterval.push_back(NAN);
    }
}


/////////////////////////////////////////////////
/// \brief This function will read the interval
/// syntax in commands and return their values in
/// a single vector.
///
/// \param sExpr string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _functions Define&
/// \param _option const Settings&
/// \param bEraseInterval bool
/// \return vector<double>
///
/////////////////////////////////////////////////
vector<double> readAndParseIntervals(string& sExpr, Parser& _parser, MemoryManager& _data, FunctionDefinitionManager& _functions, const Settings& _option, bool bEraseInterval)
{
	vector<double> vInterval;

	// Get user defined functions
	if (!_functions.call(sExpr))
		throw SyntaxError(SyntaxError::FUNCTION_ERROR, sExpr, SyntaxError::invalid_position);

	// If the expression contains data elements, get their contents here
	if (_data.containsTablesOrClusters(sExpr))
		getDataElements(sExpr, _parser, _data, _option);

	// Get the interval for x
	if (findParameter(sExpr, "x", '='))
        readAndParseLegacyIntervals(sExpr, "x", _parser, vInterval, 0, bEraseInterval);

	// Get the interval for y
	if (findParameter(sExpr, "y", '='))
        readAndParseLegacyIntervals(sExpr, "y", _parser, vInterval, 2, bEraseInterval);

	// Get the interval for z
	if (findParameter(sExpr, "z", '='))
        readAndParseLegacyIntervals(sExpr, "z", _parser, vInterval, 4, bEraseInterval);

	// Read the interval syntax
	if (sExpr.find('[') != string::npos
			&& sExpr.find(']', sExpr.find('[')) != string::npos
			&& sExpr.find(':', sExpr.find('[')) != string::npos)
	{
		size_t nPos = 0;
		size_t nMatchingParens = 0;

		// Find the correct interval bracket
		do
		{
			nPos = sExpr.find('[', nPos);

			if (nPos == string::npos || (nMatchingParens = getMatchingParenthesis(sExpr.substr(nPos))) == string::npos)
				break;

            nMatchingParens += nPos;
			nPos++;
		}
		while (isInQuotes(sExpr, nPos) || sExpr.substr(nPos, nMatchingParens - nPos).find(':') == string::npos);

		// If an interval bracket was found
		if (nPos != string::npos && nMatchingParens != string::npos)
		{
			string sRanges = sExpr.substr(nPos, nMatchingParens - nPos);

			// Erase the interval part from the expression, if needed
			if (bEraseInterval)
				sExpr.erase(nPos - 1, nMatchingParens - nPos + 2);

            // Split the whole argument list
            auto args = getAllArguments(sRanges);

			// Try to split the indices in every argument
			for (size_t i = 0; i < args.size(); i++)
			{
				if (args[i].find(':') == string::npos)
					continue;

				auto indices = getAllIndices(args[i]);

				// Set the intervals and parse them
				for (size_t j = 0; j < 2; j++)
                {
                    if (isNotEmptyExpression(indices[j]))
                    {
                        _parser.SetExpr(indices[j]);
                        vInterval.push_back(_parser.Eval().real());
                    }
                    else
                        vInterval.push_back(NAN);
                }
			}
		}
	}

	// Return the calculated interval part
	return vInterval;
}

