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
#include "../../kernel.hpp"
#include <vector>

using namespace std;
using namespace mu;


/////////////////////////////////////////////////
/// \brief DataAccessParser constructor. This
/// function will parse the passed command string
/// into the first found data access and
/// determine, whether it is a cluster and
/// calculate the corresponding index set.
///
/// \param sCommand StringView
///
/////////////////////////////////////////////////
DataAccessParser::DataAccessParser(StringView sCommand)
{
    size_t pos = string::npos;
    bIsCluster = false;

    // Get a pointer to the current kernel instance
    NumeReKernel* instance = NumeReKernel::getInstance();

    // Do only something, if the instance is valid
    if (instance)
    {
        // Examine the whole command line
        for (size_t i = 0; i < sCommand.length(); i++)
        {
            // Is this a possible character start
            // for a data object identifier?
            if (pos == string::npos && (sCommand[i] == '_' || isalpha(sCommand[i])))
                pos = i;

            // Is a possible start character available
            // and the current character is not part of a
            // valid object identifier character set?
            if (pos != string::npos && sCommand[i] != '_' && sCommand[i] != '~' && !isalnum(sCommand[i]))
            {
                // If the current character is an opening parenthesis
                // or an opening brace, parse the identified data
                // access, otherwise simply reset the possible starting
                // character
                if (sCommand[i] == '(')
                {
                    // This is a usual table or a reference to "string()"
                    sDataObject = sCommand.subview(pos, i - pos).to_string();

                    // Ensure that the table exists
                    if (!instance->getMemoryManager().isTable(sDataObject) && sDataObject != "string")
                    {
                        sDataObject.clear();
                        pos = string::npos;
                        continue;
                    }

                    // Calculate the indices
                    ::getIndices(sCommand.subview(pos), idx, instance->getParser(), instance->getMemoryManager(), instance->getSettings());
                    break;
                }
                else if (sCommand[i] == '{')
                {
                    // This is a cluster reference
                    sDataObject = sCommand.subview(pos, i - pos).to_string();

                    // Ensure that the cluster exists
                    if (!instance->getMemoryManager().isCluster(sDataObject))
                    {
                        sDataObject.clear();
                        pos = string::npos;
                        continue;
                    }

                    // Calculate the indices and switch the access
                    // to a cluster access
                    bIsCluster = true;
                    ::getIndices(sCommand.subview(pos), idx, instance->getParser(), instance->getMemoryManager(), instance->getSettings());
                    break;
                }
                else
                    pos = string::npos;
            }
        }
    }

}


/////////////////////////////////////////////////
/// \brief Evaluates open end indices using the
/// identified data object size.
///
/// \return void
///
/////////////////////////////////////////////////
void DataAccessParser::evalIndices()
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    if (idx.row.isOpenEnd())
    {
        if (bIsCluster)
            idx.row.setRange(0, _data.getCluster(sDataObject).size()-1);
        else
            idx.row.setRange(0, _data.getLines(sDataObject)-1);
    }

    if (idx.col.isOpenEnd())
    {
        if (bIsCluster)
            idx.col = VectorIndex(idx.col.front());
        else
            idx.col.setRange(0, _data.getCols(sDataObject)-1);
    }
}


/////////////////////////////////////////////////
/// \brief Returns a reference to the data object
/// identifier.
///
/// \return string&
///
/////////////////////////////////////////////////
string& DataAccessParser::getDataObject()
{
    return sDataObject;
}


/////////////////////////////////////////////////
/// \brief Returns a reference to the stored
/// indices.
///
/// \return Indices&
///
/////////////////////////////////////////////////
Indices& DataAccessParser::getIndices()
{
    return idx;
}


/////////////////////////////////////////////////
/// \brief Determines, whether the data access
/// references a cluster.
///
/// \return bool
///
/////////////////////////////////////////////////
bool DataAccessParser::isCluster() const
{
    return bIsCluster;
}



static void resolveTablesAndClusters(string& sLine, Parser& _parser, MemoryManager& _data, const Settings& _option, bool bReplaceNANs);
static const string& handleCachedDataAccess(string& sLine, Parser& _parser, MemoryManager& _data, const Settings& _option);
static void replaceEntityStringOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityStringReplacement);
static void replaceEntityOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityName, const string& sEntityReplacement, const Indices& _idx, MemoryManager& _data, Parser& _parser, const Settings& _option, bool isCluster);
static string createMafDataAccessString(const string& sAccessString, Parser& _parser);
static string createEveryDefinition(const string& sLine, Parser& _parser);
static string createMafVectorName(string sAccessString);
static vector<double> MafDataAccess(MemoryManager& _data, const string& sMafname, const string& sCache, const string& sMafAccess);
static string getMafFromAccessString(const string& sAccessString);
static string getMafAccessString(const string& sLine, const string& sEntity);
static void handleMafDataAccess(string& sLine, const string& sMafAccess, Parser& _parser, MemoryManager& _data);
static string getLastToken(const string& sLine);
static int evalColumnIndicesAndGetDimension(MemoryManager& _data, Parser& _parser, const string& sDatatable, const string& sDataExpression, Indices& _idx, int nColumns, bool isCluster, const Settings& _option);
static NumeRe::Table copyAndExtract(MemoryManager& _data, const string& sDatatable, const Indices& _idx, int nDim);


/////////////////////////////////////////////////
/// \brief Searches the passed string for calls
/// to any table or cluster and replaces them with
/// internal vectors or their respective values.
///
/// \param sLine string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \param bReplaceNANs bool
/// \return string
///
/// This function actually delegates the hard work
/// to replaceDataEntities(), which is itself
/// public and replaces all calls to a single
/// data entity.
/////////////////////////////////////////////////
string getDataElements(string& sLine, Parser& _parser, MemoryManager& _data, const Settings& _option, bool bReplaceNANs)
{
	// Evaluate possible cached equations
	if ((_parser.HasCachedAccess() || _parser.GetCachedEquation().length()) && !_parser.IsCompiling())
		return handleCachedDataAccess(sLine, _parser, _data, _option);

	// Validate the number of parentheses
	if (!validateParenthesisNumber(sLine))
		throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, SyntaxError::invalid_position);

	string sCache = "";             // Rueckgabe-string: Ggf. der linke Teil der Gleichung, falls es sich um eine Zuweisung handelt
	string sLine_Temp;         // temporaerer string, da wir die string-Referenz nicht unnoetig veraendern wollen
	size_t eq_pos = string::npos;                // int zum Zwischenspeichern der Position des "="


	/* --> Jetzt folgt der ganze Spass fuer "cache(". Hier ist relativ viel aehnlich, allerdings gibt es
	 *     noch den Fall, dass "cache(" links des "=" auftauchen darf, da es sich dabei um eine Zuweisung
	 *     eines (oder mehrerer) Wert(e) an den Cache handelt. <--
	 */
	if (_data.containsTablesOrClusters(sLine))
	{
	    isClusterCandidate(sLine, sCache, false);

		// Only try to find the position of the equality
		// sign, if the line does not start with a minus. In
		// these cases, it is most assured a parameter string
		// full of equality signs.
        if (sLine[sLine.find_first_not_of(' ')] != '-')
            eq_pos = sLine.find('=');

		if (eq_pos == string::npos              // gar kein "="?
            || !_data.containsTablesOrClusters(sLine.substr(0, eq_pos))   // nur links von "cache("?
            || (sLine[eq_pos + 1] == '=' || sLine[eq_pos - 1] == '<' || sLine[eq_pos - 1] == '>' || sLine[eq_pos - 1] == '!')) // wenn rechts von "cache(", dann nur Logikausdruecke...
		{
            resolveTablesAndClusters(sLine, _parser, _data, _option, bReplaceNANs);
		}
		else
		{
			/* --> Nein? Dann ist das eine Zuweisung. Wird komplizierter zu loesen. Auﬂerdem kann dann rechts von
			 *     "=" immer noch "cache(" auftreten. <--
			 * --> Suchen wir zuerst mal nach der Position des "=" und speichern diese in eq_pos <--
			 */
			// --> Teilen wir nun sLine an "=": Der Teillinks in sCache, der Teil rechts in sLine_Temp <--
			sCache = sLine.substr(0, eq_pos);
			StripSpaces(sCache);

			while (sCache[0] == '(')
				sCache.erase(0, 1);

			// --> Gibt's innerhalb von "cache()" nochmal einen Ausdruck "cache("? <--
			if (_data.containsTablesOrClusters(sCache.substr(sCache.find_first_of("({") + 1)))
			{
				sLine_Temp = sCache.substr(sCache.find_first_of("({") + 1);
                resolveTablesAndClusters(sLine_Temp, _parser, _data, _option, bReplaceNANs);
				sCache = sCache.substr(0, sCache.find_first_of("({") + 1) + sLine_Temp;
			}

			sLine_Temp = sLine.substr(eq_pos + 1);

			// --> Gibt es rechts von "=" nochmals "cache("? <--
			if (_data.containsTablesOrClusters(sLine_Temp))
			{
				/* --> Ja? Geht eigentlich trotzdem wie oben, mit Ausnahme, dass ueberall wo "sLine" aufgetreten ist,
				 *     nun "sLine_Temp" auftritt <--
				 */
                resolveTablesAndClusters(sLine_Temp, _parser, _data, _option, bReplaceNANs);
			}

			// --> sLine_Temp an sLine zuweisen <--
			sLine = sLine_Temp;
		}
	}

	return sCache;
}


/////////////////////////////////////////////////
/// \brief Resolves every call to a cluster or a
/// table.
///
/// \param sLine string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \param bReplaceNANs bool
/// \return void
///
/////////////////////////////////////////////////
static void resolveTablesAndClusters(string& sLine, Parser& _parser, MemoryManager& _data, const Settings& _option, bool bReplaceNANs)
{
    // Try to find every cache and handle its contents
    if (_data.containsTables(sLine))
    {
        for (auto iter = _data.getTableMap().begin(); iter != _data.getTableMap().end(); iter++)
        {
            if (sLine.find((iter->first) + "(") != string::npos)
                replaceDataEntities(sLine, iter->first + "(", _data, _parser, _option, bReplaceNANs);
        }
    }

    // Try to find every cluster and handle its contents
    if (_data.containsClusters(sLine))
    {
        for (auto iter = _data.getClusterMap().begin(); iter != _data.getClusterMap().end(); iter++)
        {
            if (sLine.find((iter->first) + "{") != string::npos)
                replaceDataEntities(sLine, iter->first + "{", _data, _parser, _option, bReplaceNANs);
        }
    }
}


/////////////////////////////////////////////////
/// \brief This function replaces all calls to a
/// single data entity with an internal vector or
/// its value, respectively.
///
/// \param sLine string&
/// \param sEntity const string&
/// \param _data Datafile&
/// \param _parser Parser&
/// \param _option const Settings&
/// \param bReplaceNANs bool
/// \return void
///
/// Because this function calls the index parser,
/// it will automatically resolve all nested calls
/// to any data entity included in the current call
/// to the specified data entity.
/////////////////////////////////////////////////
void replaceDataEntities(string& sLine, const string& sEntity, MemoryManager& _data, Parser& _parser, const Settings& _option, bool bReplaceNANs)
{
	Indices _idx;
	string sEntityOccurence = "";
	string sEntityName = sEntity.substr(0, sEntity.length()-1);
	unsigned int nPos = 0;
	bool bWriteStrings = false;
	bool bWriteFileName = false;
	bool isCluster = sEntity.back() == '{';
	vector<double> vEntityContents;
	string sEntityReplacement = "";
	string sEntityStringReplacement = "";


	// handle MAF methods. sEntity already has "(" at its back
	while (!isCluster && (nPos = sLine.find(sEntity + ").", nPos)) != string::npos)
	{
		if (isInQuotes(sLine, nPos, true) || (nPos && !isDelimiter(sLine[nPos-1])))
		{
			nPos++;
			continue;
		}

		handleMafDataAccess(sLine, getMafAccessString(sLine, sEntity), _parser, _data);
	}

	nPos = 0;

	// handle logical table accesses
	while (!isCluster && (nPos = sLine.find(sEntity + ")", nPos)) != string::npos)
	{
		if (isInQuotes(sLine, nPos, true) || (nPos && !isDelimiter(sLine[nPos-1])))
		{
			nPos++;
			continue;
		}

		sLine.replace(nPos, sEntity.length()+1, (_data.getCols(sEntity.substr(0, sEntity.length()-1)) ? "true" : "false"));
	}

	if (sLine.find(sEntity) == string::npos)
		return;

	nPos = 0;
	/* --> Diese Schleife ersetzt nacheinander alle Stellen, in denen sEntity auftritt, durch "Vektoren", die
	 *     in einer anderen Funktion weiterverarbeitet werden koennen. <--
	 */
	do
	{
		/* --> Zunaechst muessen wir herausfinden welche(s) Datenelement(e) wir aus der Datafile-Tabelle
		 *     extrahieren sollen. Dies wird durch die Syntax data(i,j) bestimmt, wobei i der Zeilen- und
		 *     j der Spaltenindex ist. i und j koennen mittels der Syntax "i_0:i_1" bzw. "j_0:j_1" einen
		 *     Bereich bestimmen, allerdings (noch) keine Matrix. (Also entweder nur i oder j) <--
		 * --> Speichere zunaechst den Teil des Strings nach "data(" in si_pos[0] <--
		 */
		sEntityOccurence = sLine.substr(sLine.find(sEntity, nPos));
		nPos = sLine.find(sEntity, nPos);

		if (nPos && (!isDelimiter(sLine[nPos-1]) || isInQuotes(sLine, nPos)))
		{
			nPos++;
			continue;
		}

		// Get the current entity
		sEntityOccurence = sEntityOccurence.substr(0, getMatchingParenthesis(sEntityOccurence) + 1);
		vEntityContents.clear();
		sEntityReplacement.clear();
		sEntityStringReplacement.clear();

		// Reading the indices happens in this function
		getIndices(sEntityOccurence, _idx, _parser, _data, _option);

		// check the indices, whether they are possible in the current context
		if (!isValidIndexSet(_idx))
			throw SyntaxError(SyntaxError::INVALID_INDEX, sLine, SyntaxError::invalid_position);

		if (!isCluster && _idx.row.isOpenEnd() && _idx.col.isOpenEnd())
			throw SyntaxError(SyntaxError::NO_MATRIX, sLine, SyntaxError::invalid_position);

		// evaluate the indices regarding the possible combinations:
		// -1: no index
		// -2: larges possible index
		// -3: string access in the current dimension

		if (!isCluster && _idx.col.isOpenEnd())
			_idx.col.setRange(0, _data.getCols(sEntityName, false)-1);

		if (!isCluster && _idx.row.isOpenEnd())
			_idx.row.setRange(0, _data.getLines(sEntityName, true) - _data.getAppendedZeroes(_idx.col.front(), sEntityName)-1);
		else if (isCluster && _idx.row.isOpenEnd())
            _idx.row.setRange(0, _data.getCluster(sEntityName).size()-1);

		if (_idx.row.isString())
			bWriteStrings = true;

		if (_idx.col.isString())
			bWriteFileName = true;

		// Handle the filename and headline access different from the usual data access
		if (!isCluster && bWriteFileName)
		{
			// Get the file name (or the cache table name)
			sEntityStringReplacement = "\"" + _data.getDataFileName(sEntityName) + "\"";
		}
		else if (!isCluster && bWriteStrings)
		{
		    vector<string> vStringContents;

			// Get the headlines
            for (size_t j = 0; j < _idx.col.size(); j++)
                vStringContents.push_back("\"" + _data.getHeadLineElement(_idx.col[j], sEntityName) + "\"");

            sEntityStringReplacement = NumeReKernel::getInstance()->getStringParser().createTempStringVectorVar(vStringContents);
		}
		else if (!isCluster)
		{
			// This is a usual data access
			// create a vector containing the data
            vEntityContents = _data.getElement(_idx.row, _idx.col, sEntityName);
		}
		else if (isCluster)
		{
		    // Get a reference to the current cluster
		    NumeRe::Cluster& cluster = _data.getCluster(sEntityName);

		    // If the cluster contains only doubles,
		    // create a vector, otherwise insert the
		    // mixed vector representation
		    if (cluster.isDouble())
            {
                // Create the vector using the indices
                for (size_t i = 0; i < _idx.row.size(); i++)
                    vEntityContents.push_back(cluster.getDouble(_idx.row[i]));
            }
            else
            {
                if (_idx.row.size() == 1)
                {
                    // Only a single element - needs no vector braces
                    if (cluster.getType(_idx.row.front()) == NumeRe::ClusterItem::ITEMTYPE_DOUBLE)
                        vEntityContents.push_back(cluster.getDouble(_idx.row.front()));
                    else
                        sEntityStringReplacement = cluster.getString(_idx.row.front());
                }
                else
                {
                    // Prepare a string vector
                    vector<string> vStringContents;

                    // Create the string vector representation
                    // using the calculated indices
                    for (size_t i = 0; i < _idx.row.size(); i++)
                    {
                        if (cluster.getType(_idx.row[i]) == NumeRe::ClusterItem::ITEMTYPE_DOUBLE)
                            vStringContents.push_back(toCmdString(cluster.getDouble(_idx.row[i])));
                        else
                            vStringContents.push_back(cluster.getString(_idx.row[i]));
                    }

                    sEntityStringReplacement = NumeReKernel::getInstance()->getStringParser().createTempStringVectorVar(vStringContents);
                }
            }
		}

		// replace the occurences
		if (sEntityStringReplacement.length())
		{
			// Replace the strings (we don't need caching here)
			_parser.DisableAccessCaching();
			replaceEntityStringOccurence(sLine, sEntityOccurence, sEntityStringReplacement);
		}
		else
		{
			// Replace the numerical occurences
			// Define the vector name
			sEntityReplacement = replaceToVectorname(sEntityOccurence);

			// Set the vector variable and its value for the parser
			_parser.SetVectorVar(sEntityReplacement, vEntityContents);

			// Cache the current access if needed
			mu::CachedDataAccess _access = {sEntityName + (isCluster ? "{" + _idx.sCompiledAccessEquation + "}" : "(" + _idx.sCompiledAccessEquation + ")") , sEntityReplacement, sEntityName};
			_parser.CacheCurrentAccess(_access);

			// Replace the occurences
			replaceEntityOccurence(sLine, sEntityOccurence, sEntityName, sEntityReplacement, _idx, _data, _parser, _option, isCluster);
		}
	}
	while (sLine.find(sEntity, nPos) != string::npos);

	return;
}


/////////////////////////////////////////////////
/// \brief This function handles cached data
/// access strings, which may be available in the
/// parser.
///
/// \param sLine string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \return string
///
/////////////////////////////////////////////////
static const string& handleCachedDataAccess(string& sLine, Parser& _parser, MemoryManager& _data, const Settings& _option)
{
	for (size_t i = 0; i < _parser.HasCachedAccess(); i++)
	{
		// Get the current cached data access
		const mu::CachedDataAccess& _access = _parser.GetCachedAccess(i);

		bool isCluster = _data.isCluster(_access.sAccessEquation);

		if (_access.sAccessEquation.find("().") != string::npos)
		{
			// handle cached MAF methods
			_parser.SetVectorVar(_access.sVectorName, MafDataAccess(_data, getMafFromAccessString(_access.sAccessEquation), _access.sCacheName, createMafDataAccessString(_access.sAccessEquation, _parser)));
			continue;
		}

		// Create an index
		Indices _idx;

		// Read the indices
		getIndices(_access.sAccessEquation, _idx, _parser, _data, _option);

		// check the indices
		if (!isValidIndexSet(_idx))
			throw SyntaxError(SyntaxError::INVALID_INDEX, sLine, SyntaxError::invalid_position);

		if (!isCluster && _idx.row.isOpenEnd() && _idx.col.isOpenEnd())
			throw SyntaxError(SyntaxError::NO_MATRIX, sLine, SyntaxError::invalid_position);

		// Evaluate the indices
		if (_idx.row.isOpenEnd())
			_idx.row.setRange(0, isCluster ? _data.getCluster(_access.sCacheName).size()-1 : _data.getLines(_access.sCacheName, false)-1);

		if (_idx.col.isOpenEnd())
			_idx.col.setRange(0, isCluster ? 0 : _data.getCols(_access.sCacheName, false)-1);

		// Get new data (Parser::GetVectorVar returns a pointer to the vector var) and update the stored elements in the internal representation
		if (isCluster)
            _data.getCluster(_access.sCacheName).insertDataInArray(_parser.GetVectorVar(_access.sVectorName), _idx.row);
        else
            _data.copyElementsInto(_parser.GetVectorVar(_access.sVectorName), _idx.row, _idx.col, _access.sCacheName);

		_parser.UpdateVectorVar(_access.sVectorName);
	}

	// Update the equation (probably there are cached elements, which could not be cached)
	sLine = _parser.GetCachedEquation();
	return _parser.GetCachedTarget();
}


/////////////////////////////////////////////////
/// \brief This function simply replaces every
/// occurences of the entity with its replacement.
///
/// \param sLine string&
/// \param sEntityOccurence const string&
/// \param sEntityStringReplacement const string&
/// \return void
///
/////////////////////////////////////////////////
static void replaceEntityStringOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityStringReplacement)
{
	size_t nPos = 0;
	while ((nPos = sLine.find(sEntityOccurence)) != string::npos)
	{
		sLine.replace(nPos, sEntityOccurence.length(), sEntityStringReplacement);
	}
}


/////////////////////////////////////////////////
/// \brief This function replaces every occurence
/// of the entity with either the vector name for
/// the parser or with its statistics value.
///
/// \param sLine string&
/// \param sEntityOccurence const string&
/// \param sEntityName const string&
/// \param sEntityReplacement const string&
/// \param _idx const Indices&
/// \param _data Datafile&
/// \param _parser Parser&
/// \param _option const Settings&
/// \param isCluster bool
/// \return void
///
/////////////////////////////////////////////////
static void replaceEntityOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityName, const string& sEntityReplacement, const Indices& _idx, MemoryManager& _data, Parser& _parser, const Settings& _option, bool isCluster)
{
	sLine = " " + sLine + " ";

	size_t nPos = 0;

	// As long as the entity occurs
	while ((nPos = sLine.find(sEntityOccurence, nPos)) != string::npos)
	{
	    // if the last character before the current occurence is
	    // alphanumeric, then ignore this occurence
	    if (nPos && (isalnum(sLine[nPos-1]) || sLine[nPos-1] == '_'))
        {
            nPos++;
            continue;
        }

		// Get the last token
		string sLeft = getLastToken(sLine.substr(0, nPos));

		// Find the next character, which is no whitespace
        size_t nNextNonWhiteSpace = sLine.find_first_not_of(" ", nPos + sEntityOccurence.length());

		if (sLeft.length() < 3 || sLeft.back() != '(' || (sLine[nNextNonWhiteSpace] != ')' && sLine[nNextNonWhiteSpace] != ','))
		{
			// Simply replace it with the vector name
			sLine.replace(nPos, sEntityOccurence.length(), sEntityReplacement);
			continue;
		}
		else
		{
			// Calculate the statistical value and replace it with the result
			// Although it seems to be duplicated code, these are only one-liners for each case
			if (sLeft == "std(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("std(", sLine.find(sEntityOccurence)))
						+ toCmdString(isCluster ? _data.getCluster(sEntityName).std(_idx.row) : _data.std(sEntityName, _idx.row, _idx.col))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "avg(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("avg(", sLine.find(sEntityOccurence)))
						+ toCmdString(isCluster ? _data.getCluster(sEntityName).avg(_idx.row) : _data.avg(sEntityName, _idx.row, _idx.col))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "max(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("max(", sLine.find(sEntityOccurence)))
						+ toCmdString(isCluster ? _data.getCluster(sEntityName).max(_idx.row) : _data.max(sEntityName, _idx.row, _idx.col))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "min(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("min(", sLine.find(sEntityOccurence)))
						+ toCmdString(isCluster ? _data.getCluster(sEntityName).min(_idx.row) : _data.min(sEntityName, _idx.row, _idx.col))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "prd(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("prd(", sLine.find(sEntityOccurence)))
						+ toCmdString(isCluster ? _data.getCluster(sEntityName).prd(_idx.row) : _data.prd(sEntityName, _idx.row, _idx.col))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "sum(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("sum(", sLine.find(sEntityOccurence)))
						+ toCmdString(isCluster ? _data.getCluster(sEntityName).sum(_idx.row) : _data.sum(sEntityName, _idx.row, _idx.col))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "num(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("num(", sLine.find(sEntityOccurence)))
						+ toCmdString(isCluster ? _data.getCluster(sEntityName).num(_idx.row) : _data.num(sEntityName, _idx.row, _idx.col))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "and(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("and(", sLine.find(sEntityOccurence)))
						+ toCmdString(isCluster ? _data.getCluster(sEntityName).and_func(_idx.row) : _data.and_func(sEntityName, _idx.row, _idx.col))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "xor(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("xor(", sLine.find(sEntityOccurence)))
						+ toCmdString(isCluster ? _data.getCluster(sEntityName).xor_func(_idx.row) : _data.xor_func(sEntityName, _idx.row, _idx.col))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "or(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("or(", sLine.find(sEntityOccurence)))
						+ toCmdString(isCluster ? _data.getCluster(sEntityName).or_func(_idx.row) : _data.or_func(sEntityName, _idx.row, _idx.col))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "cnt(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("cnt(", sLine.find(sEntityOccurence)))
						+ toCmdString(isCluster ? _data.getCluster(sEntityName).cnt(_idx.row) : _data.cnt(sEntityName, _idx.row, _idx.col))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "med(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("med(", sLine.find(sEntityOccurence)))
						+ toCmdString(isCluster ? _data.getCluster(sEntityName).med(_idx.row) : _data.med(sEntityName, _idx.row, _idx.col))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "norm(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("norm(", sLine.find(sEntityOccurence)))
						+ toCmdString(isCluster ? _data.getCluster(sEntityName).norm(_idx.row) : _data.norm(sEntityName, _idx.row, _idx.col))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "cmp(")
			{
				// cmp() is more difficult
				_parser.DisableAccessCaching();
				double dRef = 0.0;
				int nType = 0;
				string sArg = "";
				sLeft = sLine.substr(sLine.find(sLeft) + sLeft.length(), getMatchingParenthesis(sLine.substr(sLine.find(sLeft) + sLeft.length() - 1)) - 1);
				sArg = getNextArgument(sLeft, true);
				sArg = getNextArgument(sLeft, true);

				if (_data.containsTablesOrClusters(sArg))
					getDataElements(sArg, _parser, _data, _option);

				_parser.SetExpr(sArg);
				dRef = _parser.Eval();
				sArg = getNextArgument(sLeft, true);

				if (_data.containsTablesOrClusters(sArg))
					getDataElements(sArg, _parser, _data, _option);

				_parser.SetExpr(sArg);
				nType = intCast(_parser.Eval());
				sLine = sLine.replace(sLine.rfind("cmp(", sLine.find(sEntityOccurence)),
									  getMatchingParenthesis(sLine.substr(sLine.rfind("cmp(", sLine.find(sEntityOccurence)) + 3)) + 4,
									  toCmdString(isCluster ? _data.getCluster(sEntityName).cmp(_idx.row, dRef, nType) : _data.cmp(sEntityName, _idx.row, _idx.col, dRef, nType)));
			}
			else if (sLeft == "pct(")
			{
				// pct() is more difficult
				_parser.DisableAccessCaching();
				double dPct = 0.5;
				string sArg = "";
				sLeft = sLine.substr(sLine.find(sLeft) + sLeft.length(), getMatchingParenthesis(sLine.substr(sLine.find(sLeft) + sLeft.length() - 1)) - 1);
				sArg = getNextArgument(sLeft, true);
				sArg = getNextArgument(sLeft, true);

				if (_data.containsTablesOrClusters(sArg))
					getDataElements(sArg, _parser, _data, _option);

				_parser.SetExpr(sArg);
				dPct = _parser.Eval();
				sLine = sLine.replace(sLine.rfind("pct(", sLine.find(sEntityOccurence)),
									  getMatchingParenthesis(sLine.substr(sLine.rfind("pct(", sLine.find(sEntityOccurence)) + 3)) + 4,
									  toCmdString(isCluster ? _data.getCluster(sEntityName).pct(_idx.row, dPct) : _data.pct(sEntityName, _idx.row, _idx.col, dPct)));
			}
			else //Fallback
				sLine.replace(nPos, sEntityOccurence.length(), sEntityReplacement);
		}
	}
}


/////////////////////////////////////////////////
/// \brief This function handles the MAF data
/// access using methods.
///
/// \param sLine string&
/// \param sMafAccess const string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \return void
///
/////////////////////////////////////////////////
static void handleMafDataAccess(string& sLine, const string& sMafAccess, Parser& _parser, MemoryManager& _data)
{
	size_t nPos = 0;
	// Replace the access string with its corresponding vector name
	string sMafVectorName = createMafVectorName(sMafAccess);

	// Only store the value, if it is not a string literal (i.e. the name)
	if (sMafVectorName.front() != '"')
    {
        // Set the vector variable with its value for the parser
        _parser.SetVectorVar(sMafVectorName, MafDataAccess(_data, getMafFromAccessString(sMafAccess), sMafAccess.substr(0, sMafAccess.find('(')), createMafDataAccessString(sMafAccess, _parser)));

        // Create a cached access and store it
        mu::CachedDataAccess _access = {sMafAccess, sMafVectorName, sMafAccess.substr(0, sMafAccess.find('('))};
        _parser.CacheCurrentAccess(_access);

    }

	// Replace every occurence
	while ((nPos = sLine.find(sMafAccess, nPos)) != string::npos)
	{
		if (isInQuotes(sLine, nPos, true))
		{
			nPos++;
			continue;
		}
		sLine.replace(nPos, sMafAccess.length(), sMafVectorName);
	}
}


/////////////////////////////////////////////////
/// \brief This function returns the modifier
/// list obtained from the method string.
///
/// \param sAccessString const string&
/// \param _parser Parser&
/// \return string
///
/// Pass 'DATA().FNC().cols().lines().grid().every(a,b)'
/////////////////////////////////////////////////
static string createMafDataAccessString(const string& sAccessString, Parser& _parser)
{
	string sDataMaf;

	if (sAccessString.find(".grid") != string::npos)
		sDataMaf += "grid";
	if (sAccessString.find(".cols") != string::npos)
		sDataMaf += "cols";
	if (sAccessString.find(".lines") != string::npos)
		sDataMaf += "lines";
	if (sAccessString.find(".every(") != string::npos)
		sDataMaf += createEveryDefinition(sAccessString, _parser);

	return sDataMaf;
}


/////////////////////////////////////////////////
/// \brief This function returns the name of the
/// MAF.
///
/// \param sAccessString const string&
/// \return string
///
/////////////////////////////////////////////////
static string getMafFromAccessString(const string& sAccessString)
{
	// Store these values statically
	static const int sMafListLength = 16;
	static string sMafList[sMafListLength] = {"std", "avg", "prd", "sum", "min", "max", "norm", "num", "cnt", "med", "and", "or", "xor", "size", "maxpos", "minpos"};
	size_t pos = 0;

	for (int i = 0; i < sMafListLength; i++)
	{
	    pos = sAccessString.find("." + sMafList[i]);

		if (pos != string::npos
            && (pos + sMafList[i].length() + 1 >= sAccessString.length() || sAccessString[pos+sMafList[i].length()+1] == '.' || isDelimiter(sAccessString[pos+sMafList[i].length()+1])))
			return sMafList[i];
	}

	return "";
}


/////////////////////////////////////////////////
/// \brief This function returns the first MAF
/// access in the passed sLine string.
///
/// \param sLine const string&
/// \param sEntity const string&
/// \return string
///
/////////////////////////////////////////////////
static string getMafAccessString(const string& sLine, const string& sEntity)
{
	size_t nPos = 0;
	static string sDelim = "+-*/^%&| ,=<>!()[]{}"; // delimiters (no "."!)

	if ((nPos = sLine.find(sEntity + ").")) != string::npos)
	{
		// go through the string and try to find a MAF acces
		for (size_t i = nPos; i < sLine.length(); i++)
		{
			if (sLine[i] == '(' || sLine[i] == '[' || sLine[i] == '{')
				i += getMatchingParenthesis(sLine.substr(i)) + 1;
			if (i >= sLine.length())
				return sLine.substr(nPos);
			if (sDelim.find(sLine[i]) != string::npos)
				return sLine.substr(nPos, i - nPos);
			if (i + 1 == sLine.length())
				return sLine.substr(nPos, i - nPos + 1);
		}
	}

	// return nothing
	return "";
}


/////////////////////////////////////////////////
/// \brief This function simply performs the MAF
/// access and returns the vector of the results.
///
/// \param _data Datafile&
/// \param sMafname const string&
/// \param sCache const string&
/// \param sMafAccess const string&
/// \return vector<double>
///
/////////////////////////////////////////////////
static vector<double> MafDataAccess(MemoryManager& _data, const string& sMafname, const string& sCache, const string& sMafAccess)
{
	if (sMafname == "std")
		return _data.std(sCache, sMafAccess);
	if (sMafname == "avg")
		return _data.avg(sCache, sMafAccess);
	if (sMafname == "prd")
		return _data.prd(sCache, sMafAccess);
	if (sMafname == "sum")
		return _data.sum(sCache, sMafAccess);
	if (sMafname == "min")
		return _data.min(sCache, sMafAccess);
	if (sMafname == "max")
		return _data.max(sCache, sMafAccess);
	if (sMafname == "norm")
		return _data.norm(sCache, sMafAccess);
	if (sMafname == "num")
		return _data.num(sCache, sMafAccess);
	if (sMafname == "cnt")
		return _data.cnt(sCache, sMafAccess);
	if (sMafname == "med")
		return _data.med(sCache, sMafAccess);
	if (sMafname == "and")
		return _data.and_func(sCache, sMafAccess);
	if (sMafname == "or")
		return _data.or_func(sCache, sMafAccess);
	if (sMafname == "xor")
		return _data.xor_func(sCache, sMafAccess);
	if (sMafname == "size")
		return _data.size(sCache, sMafAccess);
	if (sMafname == "maxpos")
		return _data.maxpos(sCache, sMafAccess);
	if (sMafname == "minpos")
		return _data.minpos(sCache, sMafAccess);

	// return a vector with one NAN
	return vector<double>(1, NAN);
}


/////////////////////////////////////////////////
/// \brief This function creates an every
/// definition from the methods, which the
/// Datafile class is able to parse.
///
/// \param sLine const string&
/// \param _parser Parser&
/// \return string
///
/////////////////////////////////////////////////
static string createEveryDefinition(const string& sLine, Parser& _parser)
{
	string sExpr = sLine.substr(sLine.find(".every(") + 6);
	sExpr.erase(getMatchingParenthesis(sExpr)+1);

	// Resolve possible remaining calls to data tables or clusters
	getDataElements(sExpr, _parser, NumeReKernel::getInstance()->getMemoryManager(), NumeReKernel::getInstance()->getSettings());

	return "every=" + sExpr + " ";
}


/////////////////////////////////////////////////
/// \brief This function simply returns the
/// vector name obtained from the MAF method
/// string.
///
/// \param sAccessString string
/// \return string
///
/////////////////////////////////////////////////
static string createMafVectorName(string sAccessString)
{
    if (sAccessString.find(".name") != string::npos)
        return "\"" + sAccessString.substr(0, sAccessString.find("().")+2) + "\"";

    sAccessString.replace(sAccessString.find("()"), 2, "[");
    sAccessString = replaceToVectorname(sAccessString);
    return sAccessString + "]";
}


/////////////////////////////////////////////////
/// \brief This function returns the last token
/// of the current string, e.g. a function name,
/// etc.
///
/// \param sLine const string&
/// \return string
///
/////////////////////////////////////////////////
static string getLastToken(const string& sLine)
{
	string sToken = sLine;
	size_t pos;
	// Strip the spaces, because the string may end with "... FUNCTION(   "
	StripSpaces(sToken);

	if (sToken.back() == '(')
	{
		// Probably a function name here
		pos = sToken.find_last_of(" !%&|<>=?:.,/*-+^(){}#", sToken.length() - 2);
		if (pos == string::npos)
			return sToken;
		sToken.erase(0, pos + 1);
		return sToken;
	}

	// Can be every thing else
	pos = sToken.find_last_of(" !%&|<>=?:.,/*-+^(){}#");
	if (pos == string::npos)
		return sToken;
	sToken.erase(0, pos + 1);
	return sToken;
}


/////////////////////////////////////////////////
/// \brief This function is for extracting the
/// desired number of columns out of the data
/// object and storing it to a continous block of
/// memory used for example for regularize,
/// spline, pulse, stfa.
///
/// \param sTableName const string&
/// \param _idx Indices&
/// \param _data const Datafile&
/// \param _cache Datafile&
/// \param nDesiredCols int
/// \param bSort bool
/// \return bool
///
/////////////////////////////////////////////////
bool getData(const string& sTableName, Indices& _idx, const MemoryManager& _data, MemoryManager& _cache, int nDesiredCols, bool bSort)
{
	// write the data
	// the first case uses vector indices
    if (_idx.row.isOpenEnd())
        _idx.row.setRange(0, _data.getLines(sTableName, false)-1);

    if (_idx.col.isOpenEnd())
        _idx.col.setRange(0, _idx.col.front() + nDesiredCols-1);

    // If the command requires two columns and the column indices contain two
    // nodes, handle them here. Otherwise use a vectorial access in the lower
    // section of this conditional statement
    if (nDesiredCols == 2 && _idx.col.numberOfNodes() == 2 && _idx.col.isExpanded()) // Do not expand in this case!
    {
        for (long long int i = 0; i < _idx.row.size(); i++)
        {
            _cache.writeToTable(i, 0, "table", _data.getElement(_idx.row[i], _idx.col.front(), sTableName));
            _cache.writeToTable(i, 1, "table", _data.getElement(_idx.row[i], _idx.col.last(), sTableName));

            if (!i)
            {
                _cache.setHeadLineElement(0, "table", _data.getHeadLineElement(_idx.col.front(), sTableName));
                _cache.setHeadLineElement(1, "table", _data.getHeadLineElement(_idx.col.last(), sTableName));
            }
        }
    }
    else
    {
        for (long long int i = 0; i < _idx.row.size(); i++)
        {
            for (long long int j = 0; j < _idx.col.size(); j++)
            {
                _cache.writeToTable(i, j, "table", _data.getElement(_idx.row[i], _idx.col[j], sTableName));

                if (!i)
                    _cache.setHeadLineElement(j, "table", _data.getHeadLineElement(_idx.col[j], sTableName));
            }
        }
    }

	// sort, if sorting is activated
	if (bSort)
		_cache.sortElements("sort -table c=1[2:]");

	return true;
}


/////////////////////////////////////////////////
/// \brief This function will extract the needed
/// data into a table object.
///
/// \param sDataExpression const string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \return NumeRe::Table
///
/////////////////////////////////////////////////
NumeRe::Table parser_extractData(const string& sDataExpression, Parser& _parser, MemoryManager& _data, const Settings& _option)
{
	string sDatatable = "data";                             // Int fuer die Position des aktuellen find-Treffers eines Daten-Objekts
	int nColumns = 0;
	bool openEnd = false;
	bool isCluster = false;

	// Get the correct index set
	Indices _idx = getIndicesForPlotAndFit(sDataExpression, sDatatable, nColumns, openEnd, isCluster);

	// Evaluate the indices and determine the dimension
    int nDim = evalColumnIndicesAndGetDimension(_data, _parser, sDatatable, sDataExpression, _idx, nColumns, isCluster, _option);

	// Validate, if the line indices have a reasonable large difference
	if (_idx.row.size() <= 1)
		throw SyntaxError(SyntaxError::TOO_FEW_LINES, sDataExpression, SyntaxError::invalid_position);

    // copy the data and extract the table
    return copyAndExtract(_data, sDatatable, _idx, nDim);
}


/////////////////////////////////////////////////
/// \brief This function evaluates the column
/// indices and returns the final dimension of
/// the columns.
///
/// \param _data Datafile&
/// \param _parser Parser&
/// \param sDatatable const string&
/// \param sDataExpression const string&
/// \param _idx Indices&
/// \param nColumns int
/// \param isCluster bool
/// \param _option const Settings&
/// \return int
///
/////////////////////////////////////////////////
static int evalColumnIndicesAndGetDimension(MemoryManager& _data, Parser& _parser, const string& sDatatable, const string& sDataExpression, Indices& _idx, int nColumns, bool isCluster, const Settings& _option)
{
    int nDim = 0;

    // Ensure consistent indices
    _idx.row.setRange(0, _data.getLines(sDatatable, false)-1);
    _idx.col.setRange(0, _data.getCols(sDatatable)-1);

    // Validate the calculated indices
	if (_idx.row.front() > _data.getLines(sDatatable, false)
            || _idx.col.front() > _data.getCols(sDatatable) - 1)
	{
		throw SyntaxError(SyntaxError::INVALID_INDEX, sDataExpression, SyntaxError::invalid_position);
	}

	/* --> Bestimmen wir die "Dimension" des zu fittenden Datensatzes. Dabei ist es auch
	 *     von Bedeutung, ob Fehlerwerte verwendet werden sollen <--
	 */
	nDim = 0;

	if ((nColumns == 1 && _idx.col.size() < 2) || isCluster)
		throw SyntaxError(SyntaxError::TOO_FEW_COLS, sDataExpression, SyntaxError::invalid_position);
	else if (nColumns == 1)
		nDim = _idx.col.size();
	else
	{
		nDim = nColumns;
	}

	return nDim;
}


/////////////////////////////////////////////////
/// \brief This function will calculate the
/// indices from the passed data expression and
/// return them optimized for the plotting and
/// the fitting algorithm.
///
/// \param sExpression const string&
/// \param sDataTable string&
/// \param nColumns int&
/// \param openEnd bool&
/// \param isCluster bool&
/// \return Indices
///
/////////////////////////////////////////////////
Indices getIndicesForPlotAndFit(const string& sExpression, string& sDataTable, int& nColumns, bool& openEnd, bool& isCluster)
{
    Indices _idx;
    openEnd = false;
    isCluster = false;
    sDataTable = "data";

    // Search for tables and clusters
    DataAccessParser _accessParser(sExpression);

    if (_accessParser.getDataObject().length())
    {
        sDataTable = _accessParser.getDataObject();
        _idx = _accessParser.getIndices();
        isCluster = _accessParser.isCluster();
    }
    else
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sExpression, SyntaxError::invalid_position);

    if (!isValidIndexSet(_idx))
        throw SyntaxError(SyntaxError::INVALID_INDEX, sExpression, SyntaxError::invalid_position);

    // Determine the number of passed columns and
    // whether the user left an open end in the column
    // index list
    if (_idx.col.numberOfNodes() > 2)
        nColumns = _idx.col.numberOfNodes();
    else
    {
        nColumns = _idx.col.numberOfNodes();
        openEnd = _idx.col.isOpenEnd();

        if (isCluster)
            nColumns = 1;
    }

    return _idx;
}


/////////////////////////////////////////////////
/// \brief This function will copy the contents
/// to the target table and extract the table.
///
/// \param _data Datafile&
/// \param sDatatable const string&
/// \param _idx const Indices&
/// \param nDim int
/// \return NumeRe::Table
///
/////////////////////////////////////////////////
static NumeRe::Table copyAndExtract(MemoryManager& _data, const string& sDatatable, const Indices& _idx, int nDim)
{
    MemoryManager _cache;
    // Copy the contents of the data into the local cache object
    // The indices are vectors
    if (nDim == 2)
    {
        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            _cache.writeToTable(i, 0, "table", _data.getElement(_idx.row[i], _idx.col[0], sDatatable));
            _cache.writeToTable(i, 1, "table", _data.getElement(_idx.row[i], _idx.col[1], sDatatable));
        }
    }
    else if (nDim == 3)
    {
        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            _cache.writeToTable(i, 0, "table", _data.getElement(_idx.row[i], _idx.col[0], sDatatable));
            _cache.writeToTable(i, 1, "table", _data.getElement(_idx.row[i], _idx.col[1], sDatatable));
            _cache.writeToTable(i, 2, "table", _data.getElement(_idx.row[i], _idx.col[2], sDatatable));
        }
    }

	// Sort the elements
	_cache.sortElements("sort -table c=1[2:]");

	// Rename the table
	if (sDatatable != "table")
        _cache.renameTable("table", sDatatable, true);

	// Return the extracted table object
	return _cache.extractTable(sDatatable);
}


/////////////////////////////////////////////////
/// \brief This function checks, whether the
/// passed expression is non-empty (i.e. it
/// contains more than white spaces).
///
/// \param sExpr const string&
/// \return bool
///
/////////////////////////////////////////////////
bool isNotEmptyExpression(const string& sExpr)
{
	if (!sExpr.length())
		return false;
	return sExpr.find_first_not_of(' ') != string::npos;
}


/////////////////////////////////////////////////
/// \brief This function checks, whether the
/// passed command line contains the syntax for a
/// cluster candidate, splits up the command line
/// (if selected using doCut) and declares the
/// new cluster, if it is not known up to now.
///
/// \param sLine string&
/// \param sCluster string&
/// \param doCut bool
/// \return bool
///
/////////////////////////////////////////////////
bool isClusterCandidate(string& sLine, string& sCluster, bool doCut)
{
    // Do nothing, if the current line does not contain
    // an assignment operator
    if (sLine.find('=') == string::npos || sLine[sLine.find_first_not_of(' ')] == '-')
        return false;

    size_t nQuotes = 0;

    // Go through the command line and try to detect
    // a opening brace following an alphanumeric character
    // or an underscore
    for (size_t i = 1; i < sLine.length(); i++)
    {
        // consider quotation marks
        if (sLine[i] == '"' && (!i || sLine[i-1] != '\\'))
            nQuotes++;

        // Is this an opening brace following the alphanumeric
        // character? Then try to extract the corresponding cluster
        // name
        if (!(nQuotes % 2) && sLine[i] == '{' && (sLine[i-1] == '_' || isalnum(sLine[i-1])))
        {
            size_t start = 0;

            // Find the starting position
            for (int j = i-1; j >= 0; j--)
            {
                if (!isalnum(sLine[j]) && sLine[j] != '_' && sLine[j] != '~')
                    start = j+1;
            }

            // Extract the cluster including its braces
            sCluster = sLine.substr(start, getMatchingParenthesis(sLine.substr(i))+(i-start)+1);

            // If the command line shall be splitted, do that
            // here
            if (doCut)
            {
                sLine.erase(start, sCluster.length());

                size_t nextCharPos = sLine.find_first_not_of(' ');

                if (nextCharPos != string::npos && sLine[nextCharPos] == '=' && sLine[nextCharPos+1] != '=')
                    sLine.erase(start, nextCharPos + 1);
            }

            StripSpaces(sCluster);

            // Get a reference to the datafile object
            MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

            // Declare the extracted cluster, if it is not
            // known to the clustermanager
            if (!_data.isCluster(sCluster.substr(0, sCluster.find('{'))))
                _data.newCluster(sCluster);

            return true;
        }

        // These are characters, which do not belong to a cluster
        // identifier. Return false in this case.
        if (!(nQuotes % 2) && sLine[i] != '~' && sLine[i] != '_' && sLine[i] != '[' && sLine[i] != ']' && (sLine[i] == '(' || sLine[i] == '=' || ispunct(sLine[i])))
            return false;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This function returns the data from
/// the selected object and switches
/// automatically between tables and clusters.
///
/// \param sObject const string&
/// \param i long longint
/// \param j long longint
/// \param isCluster bool
/// \return double
///
/////////////////////////////////////////////////
double getDataFromObject(const string& sObject, long long int i, long long int j, bool isCluster)
{
    // Get a reference to the datafile object
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    // Fallback to ensure that valid indices are read
    if (i < 0 || j < 0)
        return NAN;

    // return the data depending on the passed isCluster
    // boolean, the object name and its indices
    return isCluster ? _data.getCluster(sObject).getDouble(i) : _data.getElement(i, j, sObject);
}


/////////////////////////////////////////////////
/// \brief This function checks, whether the
/// argument located between sLeft and sRight is
/// part of a multi-argument function.
///
/// \param sLeft const string&
/// \param sRight const string&
/// \return bool
///
/////////////////////////////////////////////////
bool parser_CheckMultArgFunc(const string& sLeft, const string& sRight)
{
    // Get the last token form the left side (can be the name of a function, for example)
	string sFunc = getLastToken(sLeft);

	// If it doesn't end with a parenthesis, it cannot be a function
	if (sFunc.back() != '(')
		return false;

	bool bCMP = false;

	// examine the first non-whitespace character of the right side:
	// - if it is a parenthesis, then simply break
	// - if it is a comma, then try to find the next parenthesis. If it exists, it can be a compare-like function signature
	for (unsigned int i = 0; i < sRight.length(); i++)
	{
		if (sRight[i] != ' ')
		{
			if (sRight[i] == ')')
				break;
			else if (sRight[i] == ',')
			{
				if (sRight.find(')', i + 1) != string::npos)
					bCMP = true;
				else
					return false;
				break;
			}
		}
	}

	// Now compare sFunc with the possible multi-argument functions
	if (sFunc == "max(" && !bCMP)
		return true;
	else if (sFunc == "min(" && !bCMP)
		return true;
	else if (sFunc == "sum(" && !bCMP)
		return true;
	else if (sFunc == "avg(" && !bCMP)
		return true;
	else if (sFunc == "num(" && !bCMP)
		return true;
	else if (sFunc == "cnt(" && !bCMP)
		return true;
	else if (sFunc == "med(" && !bCMP)
		return true;
	else if (sFunc == "pct(" && bCMP)
		return true;
	else if (sFunc == "std(" && !bCMP)
		return true;
	else if (sFunc == "prd(" && !bCMP)
		return true;
	else if (sFunc == "and(" && !bCMP)
		return true;
	else if (sFunc == "or(" && !bCMP)
		return true;
	else if (sFunc == "xor(" && !bCMP)
		return true;
	else if (sFunc == "cmp(" && bCMP)
		return true;
	else if (sFunc == "norm(" && !bCMP)
		return true;
	else
		return false;
}


