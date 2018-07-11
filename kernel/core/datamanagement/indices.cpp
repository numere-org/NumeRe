/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2018  Erik Haenel et al.

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

#include "dataaccess.hpp"
#include "../utils/tools.hpp"
#include <vector>

using namespace std;

static void handleArgumentForIndices(Indices& _idx, Parser& _parser, Datafile& _data, string& sArgument, const string& sCmd);
static void extractIndexList(string& sArgument, string sI[2], string sJ[2]);
static void handleIndexVectors(Parser& _parser, Indices& _idx, string sI[2], string sJ[2]);
static void handleCasualIndices(Parser& _parser, Indices& _idx, string sI[2], string sJ[2], const string& sCmd);
static void expandIndexVectors(Indices& _idx, Datafile& _data, const string& sCmd);
static bool isCandidateForVectors(string sI[2], string sJ[2]);
static bool isCandidateForCasuals(string sI[2], string sJ[2]);

/*
 * --> Gibt DATENELEMENT-Indices als Ints in einem Indices-Struct zurueck <--
 * --> Index = -1, falls der Index nicht gefunden wurde/kein DATENELEMENT uebergeben wurde <--
 * --> Index = -2, falls der Index den gesamten Bereich erlaubt <--
 * --> Index = -3, falls der Index eine Stringreferenz ist <--
 * --> Gibt alle angegeben Indices-1 zurueck <--
 */
Indices parser_getIndices(const string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option)
{
	Indices _idx;

	string sArgument = "";
	unsigned int nPos = 0;

	// Check, whether indices are available
	if (sCmd.find('(') == string::npos)
		return _idx;

	// Find parenthesis position and get the matching closing parenthesis
	nPos = sCmd.find('(');
	size_t nClosingParens = getMatchingParenthesis(sCmd);
	if (nClosingParens == string::npos)
		return _idx;
	sArgument = sCmd.substr(nPos + 1, nClosingParens - nPos - 1);

	// If the argument contains tables, get their values. This leads to a recursion!
	if (sArgument.find("data(") != string::npos || _data.containsCacheElements(sArgument))
		getDataElements(sArgument, _parser, _data, _option);

	// Remove not necessary white spaces
	StripSpaces(sArgument);

	// This is the handler for the abbreviation TABLE() == TABLE(:,:)
	if (!sArgument.length())
	{
		_idx.nI[0] = 0;
		_idx.nJ[0] = 0;
		_idx.nI[1] = -2;
		_idx.nJ[1] = -2;
		return _idx;
	}

	// Store the obtained argument (this will contain the results from the recursion)
	_idx.sCompiledAccessEquation = sArgument;

	// If the argument contains a comma, handle it as a usual index list
	if (sArgument.find(',') != string::npos)
	{
		handleArgumentForIndices(_idx, _parser, _data, sArgument, sCmd);
	}
	return _idx;
}


/*
 *   LOCAL FUNCTIONS FOR parser_getIndices()
 */

static void handleArgumentForIndices(Indices& _idx, Parser& _parser, Datafile& _data, string& sArgument, const string& sCmd)
{
	string sI[2] = {"<<NONE>>", "<<NONE>>"};
	string sJ[2] = {"<<NONE>>", "<<NONE>>"};

	// extract the (textual) indices from the argument list and store it in sI and sJ
	extractIndexList(sArgument, sI, sJ);

	// Ensure that the access equation contains at least one vector
	if (isCandidateForVectors(sI, sJ))
	{
        // Try to match the textual indices to vectors
        handleIndexVectors(_parser, _idx, sI, sJ);
	}

	// Ensure that the indices are casuals and no indices
	if (isCandidateForCasuals(sI, sJ))
    {
        // Handle the casual indices
        handleCasualIndices(_parser, _idx, sI, sJ, sCmd);
    }

	if (_idx.vI.size() || _idx.vJ.size())
	{
		// Expand the casual indices to vectors if needed
		expandIndexVectors(_idx, _data, sCmd);
	}
}

// separates the argument into its up to four parts and returns the position after the last operator
static void extractIndexList(string& sArgument, string sI[2], string sJ[2])
{
	int nParenthesis = 0;
	size_t nPos = 0;

	// Go through the arguments and separate it at the relevant characters
	for (unsigned int n = 0; n < sArgument.length(); n++)
	{
		// handle parentheses
		if (sArgument[n] == '(' || sArgument[n] == '{')
			nParenthesis++;
		if (sArgument[n] == ')' || sArgument[n] == '}')
			nParenthesis--;

		// If all parentheses are closed and there's a colon
		if (sArgument[n] == ':' && !nParenthesis)
		{
			if (!nPos)
			{
				if (!n)
					sI[0] = "<<EMPTY>>";
				else
					sI[0] = sArgument.substr(0, n);
			}
			else if (n == nPos)
			{
				sJ[0] = "<<EMPTY>>";
			}
			else
			{
				sJ[0] = sArgument.substr(nPos, n - nPos);
			}
			nPos = n + 1;
		}

		// If all parentheses are closed and there's a comma
		if (sArgument[n] == ',' && !nParenthesis)
		{
			if (!nPos)
			{
				if (!n)
					sI[0] = "<<EMPTY>>";
				else
					sI[0] = sArgument.substr(0, n);
			}
			else
			{
				if (n == nPos)
					sI[1] = "<<EMPTY>>";
				else
					sI[1] = sArgument.substr(nPos, n - nPos);
			}
			nPos = n + 1;
		}
	}

	// Handle the last possible argument:
	// - Either it's a single column or
	// - the last index of the column
	if (sJ[0] == "<<NONE>>")
	{
		if (nPos < sArgument.length())
			sJ[0] = sArgument.substr(nPos);
		else
			sJ[0] = "<<EMPTY>>";
	}
	else if (nPos < sArgument.length())
		sJ[1] = sArgument.substr(nPos);
	else
		sJ[1] = "<<EMPTY>>";
}

// This function will evaluate the indices and it tries to match it to a vector
static void handleIndexVectors(Parser& _parser, Indices& _idx, string sI[2], string sJ[2])
{
	value_type* v;
	int nResults;

	// Evaluate the line indices
	if (sI[0] != "<<NONE>>" && sI[1] == "<<NONE>>")
	{
		if (sI[0] == "#")
			_idx.nI[0] = -3;
		else
		{
			_parser.SetExpr(sI[0]);
			v = _parser.Eval(nResults);
			if (nResults > 1)
			{
				// vector
				for (int n = 0; n < nResults; n++)
				{
					if (!isnan(v[n]) && !isinf(v[n]))
						_idx.vI.push_back(intCast(v[n]) - 1);
				}
			}
			else // single index
				_idx.nI[0] = intCast(v[0]) - 1;
		}
	}

	// Evalute the column indices
	if (sJ[0] != "<<NONE>>" && sJ[1] == "<<NONE>>")
	{
		if (sJ[0] == "#")
			_idx.nJ[0] = -3;
		else
		{
			_parser.SetExpr(sJ[0]);
			v = _parser.Eval(nResults);
			if (nResults > 1)
			{
				// vector
				for (int n = 0; n < nResults; n++)
				{
					if (!isnan(v[n]) && !isinf(v[n]))
						_idx.vJ.push_back(intCast(v[n]) - 1);
				}
			}
			else // single index
				_idx.nJ[0] = intCast(v[0]) - 1;
		}
	}
}

// This function will evaluate all indices at once and store them into the Indices object
static void handleCasualIndices(Parser& _parser, Indices& _idx, string sI[2], string sJ[2], const string& sCmd)
{
	value_type* v = 0;
	int nResults = 0;
	string sIndexExpressions;
	vector<int> vIndexNumbers;

	// Go through all indices and connect them to one single equation
	// store the assignment of the indices
	for (int n = 0; n < 2; n++)
	{
		if (sI[n] == "<<EMPTY>>")
		{
			if (n)
				_idx.nI[n] = -2; //special one: last possible index
			else
				_idx.nI[0] = 0;
		}
		else if (sI[n] != "<<NONE>>")
		{
			if (!_idx.vI.size() && _idx.nI[0] != -3)
			{
				// connect the indices
				if (sIndexExpressions.length())
					sIndexExpressions += ",";
				sIndexExpressions += sI[n];
				// Store the assignment (lines are positive)
				vIndexNumbers.push_back((n + 1));
			}
		}
		if (sJ[n] == "<<EMPTY>>")
		{
			if (n)
				_idx.nJ[n] = -2; // special one: last possible index
			else
				_idx.nJ[0] = 0;
		}
		else if (sJ[n] != "<<NONE>>")
		{
			if (!_idx.vJ.size() && _idx.nJ[0] != -3)
			{
				// connect the indices
				if (sIndexExpressions.length())
					sIndexExpressions += ",";
				sIndexExpressions += sJ[n];
				// store the assignments (columns are negative)
				vIndexNumbers.push_back(-(n + 1));
			}
		}
	}

	// If the index expression list has a length, evaluate it
	if (sIndexExpressions.length())
	{
		_parser.SetExpr(sIndexExpressions);
		v = _parser.Eval(nResults);

		// check whether the number of the results is matching
		if ((size_t)nResults != vIndexNumbers.size())
			throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);

		// map the results to their assignments
		for (int i = 0; i < nResults; i++)
		{
			if (isnan(v[i]) || intCast(v[i]) <= 0)
				throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
			if (isinf(v[i])) // infinity => last possible index
				v[i] = -1; // only -1 because it will be decremented in the following lines
			if (vIndexNumbers[i] > 0)
				_idx.nI[vIndexNumbers[i] - 1] = intCast(v[i]) - 1;
			else
				_idx.nJ[abs(vIndexNumbers[i]) - 1] = intCast(v[i]) - 1;
		}
	}
}

// This function will expand casual indices into vectors
static void expandIndexVectors(Indices& _idx, Datafile& _data, const string& sCmd)
{
	// Get the cache name from the command string
	// should now only contain the name of the table
	string sCache = sCmd.substr(0, sCmd.find('('));

	// check, whether it exists
	if (!sCache.length())
		throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd, SyntaxError::invalid_position);

	// If the cache is not really a cache
	if (sCache.find("data") == string::npos && !_data.isCacheElement(sCache))
		throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, sCmd, SyntaxError::invalid_position);

	// remove leading whitespaces
	if (sCache.find(' ') != string::npos)
		sCache.erase(0, sCache.rfind(' ') + 1);

	// Expand the line indices
	if (!_idx.vI.size())
	{
		if (_idx.nI[0] == -1)
			throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
		// Handle special cases
		if (_idx.nI[1] == -2 && _idx.nI[0] != -3)
		{
			for (long long int i = _idx.nI[0]; i < _data.getLines(sCache, false); i++)
				_idx.vI.push_back(i);
		}
		else if (_idx.nI[1] == -1)
			_idx.vI.push_back(_idx.nI[0]);
		else if (_idx.nI[0] != -3)
		{
			// Just fill the vector with every value from the first to the last index
			for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
				_idx.vI.push_back(i);
		}
	}

	// Expand the column indices
	if (!_idx.vJ.size())
	{
		if (_idx.nJ[0] == -1)
			throw SyntaxError(SyntaxError::INVALID_INDEX, sCmd, SyntaxError::invalid_position);
		// Handle special cases
		if (_idx.nJ[1] == -2 && _idx.nJ[0] != -3)
		{
			for (long long int j = _idx.nJ[0]; j < _data.getCols(sCache); j++)
				_idx.vJ.push_back(j);
		}
		else if (_idx.nJ[1] == -1)
			_idx.vJ.push_back(_idx.nJ[0]);
		else if (_idx.nJ[0] != -3)
		{
			// Just fill the vector with every value from the first to the last index
			for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
				_idx.vJ.push_back(j);
		}
	}

}

// This function will ensure that at least one of the indices contains a vector
static bool isCandidateForVectors(string sI[2], string sJ[2])
{
    return (sI[1] == "<<NONE>>" || sJ[1] == "<<NONE>>");
}

// This function will ensure that at least one index is a casual one
static bool isCandidateForCasuals(string sI[2], string sJ[2])
{
    return (sI[1] != "<<NONE>>" || sJ[1] != "<<NONE>>");
}

