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
/// \brief DataAccessParser default constructor.
/////////////////////////////////////////////////
DataAccessParser::DataAccessParser()
{
    //
}


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
/// \brief DataAccessParser copy constructor.
///
/// \param _accessParser const DataAccessParser&
///
/////////////////////////////////////////////////
DataAccessParser::DataAccessParser(const DataAccessParser& _accessParser)
{
    sDataObject = _accessParser.sDataObject;
    idx = _accessParser.idx;
    bIsCluster = _accessParser.bIsCluster;
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
/// \brief This member function returns the index
/// definitions as a human-readable string.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string DataAccessParser::getIndexString()
{
    return idx.row.to_string() + ", " + idx.col.to_string();
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


/////////////////////////////////////////////////
/// \brief Covenience wrapper method for the
/// global function.
///
/// \return std::vector<size_t>
///
/////////////////////////////////////////////////
std::vector<size_t> DataAccessParser::getDataGridDimensions() const
{
    return ::getDataGridDimensions(idx, sDataObject);
}


static void resolveTablesAndClusters(string& sLine, Parser& _parser, MemoryManager& _data, const Settings& _option, int options);
static const string handleCachedDataAccess(string& sLine, Parser& _parser, MemoryManager& _data, const Settings& _option);
static void replaceEntityStringOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityStringReplacement);
static void replaceEntityOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityName, const string& sEntityReplacement, const Indices& _idx, MemoryManager& _data, Parser& _parser, const Settings& _option, bool isCluster);
static string createMafDataAccessString(const string& sAccessString, Parser& _parser);
static string createEveryDefinition(const string& sLine, Parser& _parser);
static string createMafVectorName(string sAccessString);
static vector<mu::value_type> MafDataAccess(MemoryManager& _data, const string& sMafname, const string& sCache, const string& sMafAccess);
static string getMafFromAccessString(const string& sAccessString);
static string getMafAccessString(const string& sLine, const string& sEntity);
static void handleMafDataAccess(string& sLine, const string& sMafAccess, Parser& _parser, MemoryManager& _data);
static string getLastToken(const string& sLine);


size_t findAssignmentOperator(StringView sCmd)
{
    size_t pos = sCmd.find('=');

    if (pos != std::string::npos
        && pos > 0
        && sCmd[pos - 1] != '!'
        && sCmd[pos - 1] != '<'
        && sCmd[pos - 1] != '>'
        && sCmd[pos + 1] != '='
        && sCmd[pos - 1] != '=')
        return pos;

    return std::string::npos;
}


/////////////////////////////////////////////////
/// \brief Searches the passed string for calls
/// to any table or cluster and replaces them with
/// internal vectors or their respective values.
///
/// \param sLine string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \param _option const Settings&
/// \param options int
/// \return string
///
/// This function actually delegates the hard work
/// to replaceDataEntities(), which is itself
/// public and replaces all calls to a single
/// data entity.
/////////////////////////////////////////////////
string getDataElements(string& sLine, Parser& _parser, MemoryManager& _data, const Settings& _option, int options)
{
	// Evaluate possible cached equations
	if ((_parser.HasCachedAccess() || _parser.GetCachedEquation().length()) && !_parser.IsCompiling())
		return handleCachedDataAccess(sLine, _parser, _data, _option);

	// Validate the number of parentheses
	if (!validateParenthesisNumber(sLine))
		throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, SyntaxError::invalid_position);

	string sCache;             // Rueckgabe-string: Ggf. der linke Teil der Gleichung, falls es sich um eine Zuweisung handelt
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
            eq_pos = findAssignmentOperator(sLine);

        // Try to handle the assignment operator shortcut
        if (eq_pos != std::string::npos)
        {
            sCache = sLine.substr(0, eq_pos);
            StripSpaces(sCache);

            // Direct assignment shortcut
            if (sCache.find('(') != std::string::npos
                && sCache.substr(sCache.find('(')) == "()"
                && sCache.find_first_of(" +-*/?:!%&|<>=") == std::string::npos)
            {
                std::string source = sLine.substr(eq_pos+1);
                StripSpaces(source);

                // Is the source also a complete table
                if (source.find('(') != std::string::npos
                    && source.substr(source.find("(")) == "()"
                    && source.find_first_of(" +-*/?:!%&|<>=") == std::string::npos
                    && _data.isTable(source))
                {
                    _data.copyTable(source, sCache);
                    sLine = _parser.CreateTempVectorVar(std::vector<mu::value_type>({_data.getLines(source.substr(0, source.find("("))),
                                                                                     _data.getCols(source.substr(0, source.find("(")))}));
                    return "";
                }
            }
        }

		if (eq_pos == string::npos              // gar kein "="?
            || !_data.containsTablesOrClusters(sCache))   // nur links von "cache("?
		{
            resolveTablesAndClusters(sLine, _parser, _data, _option, options);
            sCache.clear();
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
                resolveTablesAndClusters(sLine_Temp, _parser, _data, _option, options);
				sCache = sCache.substr(0, sCache.find_first_of("({") + 1) + sLine_Temp;
			}

			sLine_Temp = sLine.substr(eq_pos + 1);

			// --> Gibt es rechts von "=" nochmals "cache("? <--
			if (_data.containsTablesOrClusters(sLine_Temp))
			{
				/* --> Ja? Geht eigentlich trotzdem wie oben, mit Ausnahme, dass ueberall wo "sLine" aufgetreten ist,
				 *     nun "sLine_Temp" auftritt <--
				 */
                resolveTablesAndClusters(sLine_Temp, _parser, _data, _option, options);
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
/// \param options int
/// \return void
///
/////////////////////////////////////////////////
static void resolveTablesAndClusters(string& sLine, Parser& _parser, MemoryManager& _data, const Settings& _option, int options)
{
    // Try to find every cache and handle its contents
    if (_data.containsTables(sLine))
    {
        for (auto iter = _data.getTableMap().begin(); iter != _data.getTableMap().end(); iter++)
        {
            if (sLine.find((iter->first) + "(") != string::npos)
                replaceDataEntities(sLine, iter->first + "(", _data, _parser, _option, options);
        }
    }

    // Try to find every cluster and handle its contents
    if (_data.containsClusters(sLine))
    {
        for (auto iter = _data.getClusterMap().begin(); iter != _data.getClusterMap().end(); iter++)
        {
            if (sLine.find((iter->first) + "{") != string::npos)
                replaceDataEntities(sLine, iter->first + "{", _data, _parser, _option, options);
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
/// \param options int
/// \return void
///
/// Because this function calls the index parser,
/// it will automatically resolve all nested calls
/// to any data entity included in the current call
/// to the specified data entity.
/////////////////////////////////////////////////
void replaceDataEntities(string& sLine, const string& sEntity, MemoryManager& _data, Parser& _parser, const Settings& _option, int options)
{
	string sEntityOccurence = "";
	string sEntityName = sEntity.substr(0, sEntity.length()-1);
	unsigned int nPos = 0;
	bool bWriteStrings = false;
	bool bWriteFileName = false;
	bool isCluster = sEntity.back() == '{';
	vector<mu::value_type> vEntityContents;
	string sEntityReplacement = "";
	string sEntityStringReplacement = "";
	NumeRe::StringParser& _stringParser = NumeReKernel::getInstance()->getStringParser();


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
		Indices _idx;
		getIndices(sEntityOccurence, _idx, _parser, _data, _option);

		// check the indices, whether they are possible in the current context
		if (!isValidIndexSet(_idx))
			throw SyntaxError(SyntaxError::INVALID_INDEX, sLine, SyntaxError::invalid_position, _idx.row.to_string() + ", " + _idx.col.to_string());

		if (!isCluster && _idx.row.isOpenEnd() && _idx.col.isOpenEnd())
			throw SyntaxError(SyntaxError::NO_MATRIX, sLine, SyntaxError::invalid_position);

		// evaluate the indices regarding the possible combinations:
		// -1: no index
		// -2: larges possible index
		// -3: string access in the current dimension

		if (!isCluster && _idx.col.isOpenEnd())
			_idx.col.setRange(0, _data.getCols(sEntityName, false)-1);

		if (!isCluster && _idx.row.isOpenEnd())
			_idx.row.setRange(0, _data.getColElements(_idx.col.subidx(0, 1), sEntityName)-1);
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

            sEntityStringReplacement = _stringParser.createTempStringVectorVar(vStringContents);
		}
		else if (!isCluster)
		{
			// This is a usual data access
			// create a vector containing the data
#warning NOTE (numere#1#08/17/21): Might be the source of some bytecode issues
			if (options & INSERT_STRINGS)
                sEntityStringReplacement = _stringParser.createTempStringVectorVar(_data.getElementAsString(_idx.row, _idx.col, sEntityName));
			else if (_data.getType(_idx.col, sEntityName) > TableColumn::STRINGLIKE)
                sEntityStringReplacement = _stringParser.createTempStringVectorVar(_data.getElementMixed(_idx.row, _idx.col, sEntityName));
            else
                vEntityContents = _data.getElement(_idx.row, _idx.col, sEntityName);
		}
		else if (isCluster)
		{
		    // Get a reference to the current cluster
		    NumeRe::Cluster& cluster = _data.getCluster(sEntityName);

		    // If the cluster contains only doubles,
		    // create a vector, otherwise insert the
		    // mixed vector representation
		    if (!cluster.size())
                vEntityContents.push_back(NAN);
		    else if (cluster.isDouble())
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
                        sEntityStringReplacement = _stringParser.createTempStringVectorVar({cluster.getParserString(_idx.row.front())});
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
                            vStringContents.push_back(cluster.getParserString(_idx.row[i]));
                    }

                    sEntityStringReplacement = _stringParser.createTempStringVectorVar(vStringContents);
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
			mu::CachedDataAccess _access = {sEntityName + (isCluster ? "{" + _idx.sCompiledAccessEquation + "}" : "(" + _idx.sCompiledAccessEquation + ")") , sEntityReplacement, sEntityName, isCluster ? mu::CachedDataAccess::IS_CLUSTER : mu::CachedDataAccess::NO_FLAG};
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
static const string handleCachedDataAccess(string& sLine, Parser& _parser, MemoryManager& _data, const Settings& _option)
{
	for (size_t i = 0; i < _parser.HasCachedAccess(); i++)
	{
		// Get the current cached data access
		const mu::CachedDataAccess& _access = _parser.GetCachedAccess(i);

		if (_access.flags & mu::CachedDataAccess::IS_TABLE_METHOD)
		{
			// handle cached MAF methods
			_parser.SetVectorVar(_access.sVectorName, MafDataAccess(_data,
                                                                    getMafFromAccessString(_access.sAccessEquation),
                                                                    _access.sCacheName,
                                                                    createMafDataAccessString(_access.sAccessEquation, _parser)));
			continue;
		}

		// Create an index
		Indices _idx;

		bool isCluster = _access.flags & mu::CachedDataAccess::IS_CLUSTER;

		// Read the indices
		getIndices(_access.sAccessEquation, _idx, _parser, _data, _option);

		// check the indices
		if (!isValidIndexSet(_idx))
			throw SyntaxError(SyntaxError::INVALID_INDEX, sLine, SyntaxError::invalid_position, _idx.row.to_string() + ", " + _idx.col.to_string());

		if (!isCluster && _idx.row.isOpenEnd() && _idx.col.isOpenEnd())
			throw SyntaxError(SyntaxError::NO_MATRIX, sLine, SyntaxError::invalid_position);

		// Evaluate the indices
		if (_idx.row.isOpenEnd())
			_idx.row.setRange(0, isCluster ? _data.getCluster(_access.sCacheName).size()-1 : _data.getLines(_access.sCacheName, false)-1);

		if (_idx.col.isOpenEnd())
			_idx.col.setRange(0, isCluster ? 0 : _data.getCols(_access.sCacheName, false)-1);

		// Get new data (Parser::GetVectorVar returns a pointer to the vector var) and update the stored elements in the internal representation
		if (isCluster)
        {
            NumeRe::Cluster& clst = _data.getCluster(_access.sCacheName);

            // Ensure that this cluster has only numerically
            // interpretable values
            if (!clst.isDouble())
            {
                _parser.DisableAccessCaching();
                return getDataElements(sLine, _parser, _data, _option);
            }

            clst.insertDataInArray(_parser.GetVectorVar(_access.sVectorName), _idx.row);
        }
        else
        {
            // Ensure that the selected columns are actually numerically
            // interpretable values
            if (_data.getType(_idx.col, _access.sCacheName) >= TableColumn::STRINGLIKE)
            {
                _parser.DisableAccessCaching();
                return getDataElements(sLine, _parser, _data, _option);
            }

            _data.copyElementsInto(_parser.GetVectorVar(_access.sVectorName), _idx.row, _idx.col, _access.sCacheName);
        }

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
				mu::value_type dRef = 0.0;
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
				mu::value_type dPct = 0.5;
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
	if (!NumeReKernel::getInstance()->getStringParser().isStringExpression(sMafVectorName))
    {
        // Set the vector variable with its value for the parser
        _parser.SetVectorVar(sMafVectorName,
                             MafDataAccess(_data,
                                           getMafFromAccessString(sMafAccess),
                                           sMafAccess.substr(0, sMafAccess.find('(')),
                                           createMafDataAccessString(sMafAccess, _parser)));

        // Create a cached access and store it
        mu::CachedDataAccess _access = {sMafAccess,
                                        sMafVectorName,
                                        sMafAccess.substr(0, sMafAccess.find('(')),
                                        mu::CachedDataAccess::IS_TABLE_METHOD};
        _parser.CacheCurrentAccess(_access);
    }
    else
    {
        NumeRe::StringParser& _stringParser = NumeReKernel::getInstance()->getStringParser();

        if (sMafVectorName.front() == '{')
            sMafVectorName = _stringParser.createTempStringVectorVar(getAllArguments(sMafVectorName.substr(1, sMafVectorName.length()-2)));
        else
            sMafVectorName = _stringParser.createTempStringVectorVar({sMafVectorName});
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
/// Pass 'DATA().FNC().cols().rows().grid().every(a,b)'
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

	if (sAccessString.find(".rows") != string::npos)
		sDataMaf += "lines";

	if (sAccessString.find(".every(") != string::npos)
		sDataMaf += createEveryDefinition(sAccessString, _parser);

	return sDataMaf;
}


/////////////////////////////////////////////////
/// \brief This function returns the name of the
/// MAF.
///
/// \param sAccessString const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string getMafFromAccessString(const std::string& sAccessString)
{
	// Store these values statically
	static const int sMafListLength = 16;
	static std::string sMafList[sMafListLength] = {"std", "avg", "prd", "sum", "min", "max", "norm", "num", "cnt", "med", "and", "or", "xor", "size", "maxpos", "minpos"};
	size_t pos = 0;

	for (int i = 0; i < sMafListLength; i++)
	{
	    pos = sAccessString.find("." + sMafList[i]);

		if (pos != std::string::npos
            && (pos + sMafList[i].length() + 1 >= sAccessString.length() || sAccessString[pos+sMafList[i].length()+1] == '.' || isDelimiter(sAccessString[pos+sMafList[i].length()+1])))
			return sMafList[i];
	}

	// Special case: we only have TAB().rows or TAB().cols
	if ((pos = sAccessString.find(".rows")) != std::string::npos)
    {
		if (pos + 4 + 1 >= sAccessString.length()
            || sAccessString[pos+4+1] == '.'
            || isDelimiter(sAccessString[pos+4+1]))
			return "rows";

    }
	else if ((pos = sAccessString.find(".cols")) != std::string::npos)
    {
		if (pos + 4 + 1 >= sAccessString.length()
            || sAccessString[pos+4+1] == '.'
            || isDelimiter(sAccessString[pos+4+1]))
			return "cols";

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
/// \return vector<mu::value_type>
///
/////////////////////////////////////////////////
static vector<mu::value_type> MafDataAccess(MemoryManager& _data, const string& sMafname, const string& sCache, const string& sMafAccess)
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

	if (sMafname == "rows")
		return std::vector<mu::value_type>(1, _data.size(sCache, sMafAccess.find("grid") != std::string::npos ? "grid" : "").front());

	if (sMafname == "cols")
		return std::vector<mu::value_type>(1, _data.size(sCache, sMafAccess.find("grid") != std::string::npos ? "grid" : "").back());

	if (sMafname == "maxpos")
		return _data.maxpos(sCache, sMafAccess);

	if (sMafname == "minpos")
		return _data.minpos(sCache, sMafAccess);

	// return a vector with one NAN
	return vector<mu::value_type>(1, NAN);
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
/// \brief Realizes the "aliasof()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_aliasof(const std::string& sTableName, std::string sMethodArguments)
{
    // Might be necessary to resolve the contents of the reference
    getDataElements(sMethodArguments,
                    NumeReKernel::getInstance()->getParser(),
                    NumeReKernel::getInstance()->getMemoryManager(),
                    NumeReKernel::getInstance()->getSettings());

    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sMethodArguments))
    {
        std::string sDummy;
        NumeRe::StringParser::StringParserRetVal res = NumeReKernel::getInstance()->getStringParser().evalAndFormat(sMethodArguments, sDummy, true);

        if (res == NumeRe::StringParser::STRING_NUMERICAL)
            sMethodArguments = "\"\"";
    }

    NumeReKernel::getInstance()->getMemoryManager().addReference(sTableName, sMethodArguments.substr(1, sMethodArguments.length()-2));

    return sMethodArguments;
}


/////////////////////////////////////////////////
/// \brief Realizes the "convert()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_convert(const std::string& sTableName, std::string sMethodArguments)
{
    std::string sColumns = getNextArgument(sMethodArguments, true);

    // Might be necessary to resolve the contents of columns and conversions
    getDataElements(sColumns,
                    NumeReKernel::getInstance()->getParser(),
                    NumeReKernel::getInstance()->getMemoryManager(),
                    NumeReKernel::getInstance()->getSettings());

    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sMethodArguments))
    {
        std::string sDummy;
        sMethodArguments += " -nq";
        NumeRe::StringParser::StringParserRetVal res = NumeReKernel::getInstance()->getStringParser().evalAndFormat(sMethodArguments, sDummy, true);

        if (res == NumeRe::StringParser::STRING_NUMERICAL)
            return "\"\"";
    }

    int nResults = 0;
    NumeReKernel::getInstance()->getParser().SetExpr(sColumns);
    mu::value_type* v = NumeReKernel::getInstance()->getParser().Eval(nResults);

    if (NumeReKernel::getInstance()->getMemoryManager().convertColumns(sTableName, VectorIndex(v, nResults, 0), sMethodArguments))
        return "\"" + sMethodArguments + "\"";

    return "\"\"";
}


/////////////////////////////////////////////////
/// \brief Realizes the "typeof()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_typeof(const std::string& sTableName, std::string sMethodArguments)
{
    // Might be necessary to resolve the contents of columns and conversions
    getDataElements(sMethodArguments,
                    NumeReKernel::getInstance()->getParser(),
                    NumeReKernel::getInstance()->getMemoryManager(),
                    NumeReKernel::getInstance()->getSettings());

    int nResults = 0;
    NumeReKernel::getInstance()->getParser().SetExpr(sMethodArguments);
    mu::value_type* v = NumeReKernel::getInstance()->getParser().Eval(nResults);

    std::string sRet;

    for (int i = 0; i < nResults; i++)
    {
        if (sRet.length())
            sRet += ",";

        TableColumn::ColumnType type = NumeReKernel::getInstance()->getMemoryManager().getType(VectorIndex(intCast(v[i])-1), sTableName);
        sRet += "\"" + TableColumn::typeToString(type) + "\"";
    }

    if (sRet.length())
    {
        if (sRet.find(',') != std::string::npos)
            return "{" + sRet + "}";

        return sRet;
    }

    return "\"\"";
}


/////////////////////////////////////////////////
/// \brief Realizes the "categories()" table
/// method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_categories(const std::string& sTableName, std::string sMethodArguments)
{
    // Might be necessary to resolve the contents of columns and conversions
    getDataElements(sMethodArguments,
                    NumeReKernel::getInstance()->getParser(),
                    NumeReKernel::getInstance()->getMemoryManager(),
                    NumeReKernel::getInstance()->getSettings());

    int nResults = 0;
    NumeReKernel::getInstance()->getParser().SetExpr(sMethodArguments);
    mu::value_type* v = NumeReKernel::getInstance()->getParser().Eval(nResults);

    ValueVector vCategories = NumeReKernel::getInstance()->getMemoryManager().getCategoryList(VectorIndex(v, nResults, 0), sTableName);

    if (!vCategories.size())
        return "\"\"";

    std::string sRet;

    for (size_t i = 0; i < vCategories.size(); i+=2)
    {
        if (sRet.length())
            sRet += ",";

        sRet += "\"" + vCategories[i] + "\"," + vCategories[i+1];
    }

    return "{" + sRet + "}";
}


/////////////////////////////////////////////////
/// \brief Realizes the "categorize()" table
/// method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_categorize(const std::string& sTableName, std::string sMethodArguments)
{
    std::string sColumns = getNextArgument(sMethodArguments, true);
    std::vector<std::string> vCategories;

    // Might be necessary to resolve the contents of columns and conversions
    getDataElements(sColumns,
                    NumeReKernel::getInstance()->getParser(),
                    NumeReKernel::getInstance()->getMemoryManager(),
                    NumeReKernel::getInstance()->getSettings());

    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sMethodArguments))
    {
        std::string sDummy;
        sMethodArguments += " -nq";
        NumeRe::StringParser::StringParserRetVal res = NumeReKernel::getInstance()->getStringParser().evalAndFormat(sMethodArguments, sDummy, true);

        if (res == NumeRe::StringParser::STRING_NUMERICAL)
            return "\"\"";

        vCategories = NumeReKernel::getInstance()->getAns().getInternalStringArray();
    }

    int nResults = 0;
    NumeReKernel::getInstance()->getParser().SetExpr(sColumns);
    mu::value_type* v = NumeReKernel::getInstance()->getParser().Eval(nResults);

    if (NumeReKernel::getInstance()->getMemoryManager().setCategories(sTableName, VectorIndex(v, nResults, 0), vCategories))
    {
        vCategories = NumeReKernel::getInstance()->getMemoryManager().getCategoryList(VectorIndex(v, nResults, 0), sTableName);

        if (!vCategories.size())
            return "\"\"";

        std::string sRet;

        for (size_t i = 0; i < vCategories.size(); i+=2)
        {
            if (sRet.length())
                sRet += ",";

            sRet += "\"" + vCategories[i] + "\"," + vCategories[i+1];
        }

        return "{" + sRet + "}";
    }

    return "\"\"";
}


/////////////////////////////////////////////////
/// \brief Realizes the "describe()" method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_annotate(const std::string& sTableName, std::string sMethodArguments)
{
    // Might be necessary to resolve the contents of columns and conversions
    getDataElements(sMethodArguments,
                    NumeReKernel::getInstance()->getParser(),
                    NumeReKernel::getInstance()->getMemoryManager(),
                    NumeReKernel::getInstance()->getSettings());

    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sMethodArguments))
    {
        std::string sDummy;
        sMethodArguments += " -nq";
        NumeRe::StringParser::StringParserRetVal res = NumeReKernel::getInstance()->getStringParser().evalAndFormat(sMethodArguments, sDummy, true);

        if (res == NumeRe::StringParser::STRING_NUMERICAL)
            return "\"\"";
    }

    NumeReKernel::getInstance()->getMemoryManager().writeComment(sTableName, sMethodArguments);
    return "\"" + sMethodArguments + "\"";
}


/////////////////////////////////////////////////
/// \brief Typedef for a table method
/////////////////////////////////////////////////
typedef std::string (*TableMethod)(const std::string&, std::string);


/////////////////////////////////////////////////
/// \brief Returns the declared list of table
/// methods.
///
/// \return std::map<std::string, TableMethod>
///
/////////////////////////////////////////////////
static std::map<std::string, TableMethod> getInplaceTableMethods()
{
    std::map<std::string, TableMethod> mTableMethods;

    mTableMethods["aliasof"] = tableMethod_aliasof;
    mTableMethods["convert"] = tableMethod_convert;
    mTableMethods["typeof"] = tableMethod_typeof;
    mTableMethods["describe"] = tableMethod_annotate;
    mTableMethods["categorylist"] = tableMethod_categories;
    mTableMethods["categorize"] = tableMethod_categorize;

    return mTableMethods;
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
    static std::map<std::string, TableMethod> mMethods = getInplaceTableMethods();

    for (auto& method : mMethods)
    {
        if (sAccessString.find("." + method.first + "(") != std::string::npos)
        {
            std::string sTableName = sAccessString.substr(0, sAccessString.find("()."));
            std::string sMethodArguments = sAccessString.substr(sAccessString.find("." + method.first + "(") + 1 + method.first.length());
            sMethodArguments.erase(getMatchingParenthesis(sMethodArguments)+1);
            sMethodArguments = sMethodArguments.substr(1, sMethodArguments.length()-2);

            return method.second(sTableName, sMethodArguments);
        }
    }

    if (sAccessString.find(".name") != std::string::npos)
        return "\"" + sAccessString.substr(0, sAccessString.find("().")+2) + "\"";

    if (sAccessString.find(".description") != std::string::npos)
        return "\"" + NumeReKernel::getInstance()->getMemoryManager().getComment(sAccessString.substr(0, sAccessString.find("()."))) + "\"";

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
/// \brief This function extracts a portion of a
/// table and returns it to the calling function.
/// The returned pointer may as well be a
/// nullptr, if the selectd table does not exist.
///
/// \param sCmd const std::string&
/// \param _accessParser DataAccessParser&
/// \param nDesiredCols int
/// \param bSort bool
/// \return Memory*
///
/////////////////////////////////////////////////
Memory* extractRange(const std::string& sCmd, DataAccessParser& _accessParser, int nDesiredCols, bool bSort)
{
    // Validize the obtained index sets
    if (!isValidIndexSet(_accessParser.getIndices()))
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sCmd, _accessParser.getDataObject() + "(", _accessParser.getDataObject() + "()");

    // Select the number of relevant columns
    if (nDesiredCols > 0 && _accessParser.getIndices().col.isOpenEnd())
        _accessParser.getIndices().col.setRange(0, _accessParser.getIndices().col.front() + nDesiredCols-1);
    else if (nDesiredCols == 2 && _accessParser.getIndices().col.numberOfNodes() == 2u)
    {
        // Check number of nodes and convert the columns to an explicit two-element vector,
        // if the user did not pass an open-end index set
        Indices& _idx = _accessParser.getIndices();
        _idx.col = VectorIndex(std::vector<int>({_idx.col.front(), _idx.col.last()}));
    }

    // Evaluate the number of rows, if necessary
    _accessParser.evalIndices();

    Memory* _mem = NumeReKernel::getInstance()->getMemoryManager().getTable(_accessParser.getDataObject());

    if (!_mem)
        return nullptr;

    _mem = _mem->extractRange(_accessParser.getIndices().row, _accessParser.getIndices().col);

    if (bSort)
        _mem->sortElements(0, _mem->getLines()-1, 0, _mem->getCols()-1, "-c=1[2:]");

    return _mem;
}


/////////////////////////////////////////////////
/// \brief This function will return the access
/// parser instance for the current expression
/// validate, whether the expression is actual
/// resolveable.
///
/// \param sExpression StringView
/// \return DataAccessParser
///
/////////////////////////////////////////////////
DataAccessParser getAccessParserForPlotAndFit(StringView sExpression)
{
    // Search for tables and clusters
    DataAccessParser _accessParser(sExpression);

    if (!_accessParser.getDataObject().length())
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, sExpression.to_string(), SyntaxError::invalid_position);

    if (!isValidIndexSet(_accessParser.getIndices()))
        throw SyntaxError(SyntaxError::INVALID_INDEX, sExpression.to_string(), SyntaxError::invalid_position, _accessParser.getIndices().row.to_string() + ", " + _accessParser.getIndices().col.to_string());

    return _accessParser;
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
    DataAccessParser _accessParser = getAccessParserForPlotAndFit(sExpression);

    sDataTable = _accessParser.getDataObject();
    _idx = _accessParser.getIndices();
    isCluster = _accessParser.isCluster();

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
    if (findAssignmentOperator(sLine) == string::npos || sLine[sLine.find_first_not_of(' ')] == '-')
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
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type getDataFromObject(const string& sObject, long long int i, long long int j, bool isCluster)
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


/////////////////////////////////////////////////
/// \brief Returns the accessed data grid
/// dimensions.
///
/// \param _idx const Indices&
/// \param sTableName const std::string&
/// \return std::vector<size_t>
///
/////////////////////////////////////////////////
std::vector<size_t> getDataGridDimensions(const Indices& _idx, const std::string& sTableName)
{
    MemoryManager& _manager = NumeReKernel::getInstance()->getMemoryManager();
    std::vector<size_t> sizes(2, 0);

    if (_manager.isTable(sTableName))
    {
        sizes[0] = _manager.cnt(sTableName, _idx.row, _idx.col.subidx(0, 1)).real();
        sizes[1] = _manager.cnt(sTableName, _idx.row, _idx.col.subidx(1, 1)).real();
    }

    return sizes;
}

