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

// DataAccessParser constructor. This function will parse the passed
// command string into the first found data access and determine, whether
// it is a cluster and calculate the corresponding index set
DataAccessParser::DataAccessParser(const string& sCommand)
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
                    sDataObject = sCommand.substr(pos, i - pos);

                    // Ensure that the table exists
                    if (!instance->getData().isTable(sDataObject) && sDataObject != "data" && sDataObject != "string")
                    {
                        sDataObject.clear();
                        pos = string::npos;
                        continue;
                    }

                    // Calculate the indices
                    idx = parser_getIndices(sCommand.substr(pos), instance->getParser(), instance->getData(), instance->getSettings());
                    break;
                }
                else if (sCommand[i] == '{')
                {
                    // This is a cluster reference
                    sDataObject = sCommand.substr(pos, i - pos);

                    // Ensure that the cluster exists
                    if (!instance->getData().isCluster(sDataObject))
                    {
                        sDataObject.clear();
                        pos = string::npos;
                        continue;
                    }

                    // Calculate the indices and switch the access
                    // to a cluster access
                    bIsCluster = true;
                    idx = parser_getIndices(sCommand.substr(pos), instance->getParser(), instance->getData(), instance->getSettings());
                    break;
                }
                else
                    pos = string::npos;
            }
        }
    }

}

// Returns a reference to the data object identifier
string& DataAccessParser::getDataObject()
{
    return sDataObject;
}

// Returns a reference to the stored indices
Indices& DataAccessParser::getIndices()
{
    return idx;
}

// Determines, whether the data access references a cluster
bool DataAccessParser::isCluster()
{
    return bIsCluster;
}




static string handleCachedDataAccess(string& sLine, Parser& _parser, Datafile& _data, const Settings& _option);
static void replaceEntityStringOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityStringReplacement);
static void replaceEntityOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityName, const string& sEntityReplacement, const Indices& _idx, Datafile& _data, Parser& _parser, const Settings& _option, bool isCluster);
static string createMafDataAccessString(const string& sAccessString, Parser& _parser);
static string createEveryDefinition(const string& sLine, Parser& _parser);
static string createMafVectorName(string sAccessString);
static vector<double> MafDataAccess(Datafile& _data, const string& sMafname, const string& sCache, const string& sMafAccess);
static string getMafFromAccessString(const string& sAccessString);
static string getMafAccessString(const string& sLine, const string& sEntity);
static void handleMafDataAccess(string& sLine, const string& sMafAccess, Parser& _parser, Datafile& _data);
static string getLastToken(const string& sLine);
static int evalColumnIndicesAndGetDimension(Datafile& _data, Parser& _parser, const string& sDatatable, const string& sDataExpression, Indices& _idx, int nColumns, bool isCluster, const Settings& _option);
static NumeRe::Table copyAndExtract(Datafile& _data, const string& sDatatable, const Indices& _idx, int nDim);

/* --> Diese Funktion durchsucht einen gegebenen String sLine nach den Elementen "data(" oder "cache(" und erstetzt diese
 *     entsprechend der Syntax durch Elemente (oder Vektoren) aus dem Datenfile oder dem Cache. Falls des Weiteren auch
 *     noch Werte in den Cache geschrieben werden sollen (das Datenfile ist READ-ONLY), wird dies von dieser Funktion
 *     ebenfalls determiniert. <--
 * --> Um die ggf. ersetzten Vektoren weiterverwenden zu koennen, muss die Funktion parser_VectorToExpr() auf den String
 *     sLine angewendet werden. <--
 */
string getDataElements(string& sLine, Parser& _parser, Datafile& _data, const Settings& _option, bool bReplaceNANs)
{
	string sCache = "";             // Rueckgabe-string: Ggf. der linke Teil der Gleichung, falls es sich um eine Zuweisung handelt
	string sLine_Temp = "";         // temporaerer string, da wir die string-Referenz nicht unnoetig veraendern wollen
	unsigned int eq_pos = string::npos;                // int zum Zwischenspeichern der Position des "="

	// Evaluate possible cached equations
	if (_parser.HasCachedAccess() && !_parser.IsCompiling())
	{
		return handleCachedDataAccess(sLine, _parser, _data, _option);
	}

	// Validate the number of parentheses
	if (!validateParenthesisNumber(sLine))
		throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, SyntaxError::invalid_position);


	/* --> Jetzt folgt der ganze Spass fuer "cache(". Hier ist relativ viel aehnlich, allerdings gibt es
	 *     noch den Fall, dass "cache(" links des "=" auftauchen darf, da es sich dabei um eine Zuweisung
	 *     eines (oder mehrerer) Wert(e) an den Cache handelt. <--
	 */
	if (_data.containsTablesOrClusters(sLine))
	{
	    isClusterCandidate(sLine, sCache, false);

		// --> Ist links vom ersten "cache(" ein "=" oder ueberhaupt ein "=" im gesamten Ausdruck? <--
		eq_pos = sLine.find("=");
		if (eq_pos == string::npos              // gar kein "="?
				|| !_data.containsTablesOrClusters(sLine.substr(0, eq_pos))   // nur links von "cache("?
				|| (_data.containsTablesOrClusters(sLine.substr(0, eq_pos))  // wenn rechts von "cache(", dann nur Logikausdruecke...
					&& (sLine[eq_pos + 1] == '='
						|| sLine[eq_pos - 1] == '<'
						|| sLine[eq_pos - 1] == '>'
						|| sLine[eq_pos - 1] == '!'
					   )
				   )
		   )
		{
			// --> Cache-Lese-Status aktivieren! <--
			_data.setCacheStatus(true);

			try
			{
				// Try to find every cache and handle its contents
				for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); iter++)
				{
					if (sLine.find((iter->first) + "(") != string::npos)
						replaceDataEntities(sLine, (iter->first) + "(", _data, _parser, _option, bReplaceNANs);
				}

				// Try to find every cluster and handle its contents
				for (auto iter = _data.getClusterMap().begin(); iter != _data.getClusterMap().end(); iter++)
				{
					if (sLine.find((iter->first) + "{") != string::npos)
						replaceDataEntities(sLine, (iter->first) + "{", _data, _parser, _option, bReplaceNANs);
				}
			}
			catch (...)
			{
				_data.setCacheStatus(false);
				throw;
			}
			// --> Cache-Lese-Status deaktivieren <--
			_data.setCacheStatus(false);
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
				_data.setCacheStatus(true);

				sLine_Temp = sCache.substr(sCache.find_first_of("({") + 1);

				for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
				{
					if (sLine_Temp.find(iter->first + "(") != string::npos)
						replaceDataEntities(sLine_Temp, iter->first + "(", _data, _parser, _option, bReplaceNANs);
				}

				for (auto iter = _data.getClusterMap().begin(); iter != _data.getClusterMap().end(); ++iter)
				{
					if (sLine_Temp.find(iter->first + "{") != string::npos)
						replaceDataEntities(sLine_Temp, iter->first + "{", _data, _parser, _option, bReplaceNANs);
				}

				sCache = sCache.substr(0, sCache.find_first_of("({") + 1) + sLine_Temp;
				_data.setCacheStatus(false);
			}
			sLine_Temp = sLine.substr(eq_pos + 1);

			// --> Gibt es rechts von "=" nochmals "cache("? <--
			if (_data.containsTablesOrClusters(sLine_Temp))
			{
				/* --> Ja? Geht eigentlich trotzdem wie oben, mit Ausnahme, dass ueberall wo "sLine" aufgetreten ist,
				 *     nun "sLine_Temp" auftritt <--
				 */
				_data.setCacheStatus(true);

				try
				{
					// Try to find each cache and handle its content
					for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
					{
						if (sLine_Temp.find(iter->first + "(") != string::npos)
							replaceDataEntities(sLine_Temp, iter->first + "(", _data, _parser, _option, bReplaceNANs);
					}
					// Try to find each cluster and handle its content
					for (auto iter = _data.getClusterMap().begin(); iter != _data.getClusterMap().end(); ++iter)
					{
						if (sLine_Temp.find(iter->first + "{") != string::npos)
							replaceDataEntities(sLine_Temp, iter->first + "{", _data, _parser, _option, bReplaceNANs);
					}
				}
				catch (...)
				{
					_data.setCacheStatus(false);
					throw;
				}
				_data.setCacheStatus(false);
			}
			// --> sLine_Temp an sLine zuweisen <--
			sLine = sLine_Temp;
		}
	}


	// --> Findest du "data("? <--
	if (sLine.find("data(") != string::npos)
	{
		// --> Sind ueberhaupt Daten vorhanden? <--
		if (!_data.isValid() && (!sLine.find("data(") || checkDelimiter(sLine.substr(sLine.find("data(") - 1, 6))))
		{
			/* --> Nein? Mitteilen, BOOLEAN setzen (der die gesamte, weitere Auswertung abbricht)
			 *     und zurueck zur aufrufenden Funktion <--
			 */
			throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sLine, SyntaxError::invalid_position);
		}
		// --> Ist rechts von "data(" noch ein "=" und gehoert das nicht zu einem Logik-Ausdruck? <--
		eq_pos = sLine.find("=", sLine.find("data(") + 5);
		if (eq_pos != string::npos
				&& sLine[eq_pos + 1] != '='
				&& sLine[eq_pos - 1] != '<'
				&& sLine[eq_pos - 1] != '>'
				&& sLine[eq_pos - 1] != '!'
				&& !parser_CheckMultArgFunc(sLine.substr(0, sLine.find("data(")), sLine.substr(sLine.find(")", sLine.find("data(") + 1)))
		   )
		{
			if (sLine.substr(sLine.find("data(") + 5, sLine.find(",", sLine.find("data(") + 5) - sLine.find("data(") - 5).find("#") != string::npos)
			{
				sCache = sLine.substr(0, eq_pos);
				sLine = sLine.substr(eq_pos + 1);
			}
			else
			{
				// --> Ja? Dann brechen wir ebenfalls ab, da wir nicht in data() schreiben wollen <--
				throw SyntaxError(SyntaxError::READ_ONLY_DATA, sLine, SyntaxError::invalid_position);
			}
		}
		/* --> Diese Schleife ersetzt nacheinander alle Stellen, in denen "data(" auftritt, durch "Vektoren", die
		 *     in einer anderen Funktion weiterverarbeitet werden koennen. Eine aehnliche Schleife findet sich
		 *     auch fuer "cache(" etwas weiter unten. <--
		 * --> Wenn diese Schleife abgearbeitet ist, wird natuerlich auch noch geprueft, ob auch "cache(" gefunden
		 *     wird und ggf. die Schleife fuer den Cache gestartet. <--
		 */
		if (sLine.find("data(") != string::npos)
			replaceDataEntities(sLine, "data(", _data, _parser, _option, bReplaceNANs);
	}

	return sCache;
}

/* --> Diese Funktion ersetzt in einem gegebenen String sLine alle Entities (sEntity) von "data(" oder "cache(" und bricht
 *     ab, sobald ein Fehler auftritt. Der Fehler wird in der Referenz von bSegmentationFault gespeichert und kann in
 *     in der aufrufenden Funktion weiterverarbeitet werden <--
 */
void replaceDataEntities(string& sLine, const string& sEntity, Datafile& _data, Parser& _parser, const Settings& _option, bool bReplaceNANs)
{
	Indices _idx;
	string sEntityOccurence = "";
	string sEntityName = sEntity.substr(0, sEntity.find_first_of("({"));
	unsigned int nPos = 0;
	bool bWriteStrings = false;
	bool bWriteFileName = false;
	bool isCluster = _data.isCluster(sEntityName);
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

		if (nPos && !checkDelimiter(sLine.substr(nPos - 1, sEntity.length() + 1)))
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
		_idx = parser_getIndices(sEntityOccurence, _parser, _data, _option);

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
		if (bWriteFileName)
		{
			// Get the file name (or the cache table name)
			sEntityStringReplacement = "\"" + _data.getDataFileName(sEntityName) + "\"";
		}
		else if (bWriteStrings)
		{
			// Get the headlines
            for (size_t j = 0; j < _idx.col.size(); j++)
            {
                sEntityStringReplacement += "\"" + _data.getHeadLineElement(_idx.col[j], sEntityName) + "\", ";
            }

			if (sEntityStringReplacement.length())
				sEntityStringReplacement.erase(sEntityStringReplacement.rfind(','));
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
                sEntityStringReplacement = "{";

                // Create the string vector representation
                // using the calculated indices
                for (size_t i = 0; i < _idx.row.size(); i++)
                {
                    if (cluster.getType(_idx.row[i]) == NumeRe::ClusterItem::ITEMTYPE_DOUBLE)
                        sEntityStringReplacement += toCmdString(cluster.getDouble(_idx.row[i])) + ",";
                    else
                        sEntityStringReplacement += cluster.getString(_idx.row[i]) + ",";
                }

                sEntityStringReplacement.back() = '}';
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


// This function handles cached data access strings, which may be available in the parser
static string handleCachedDataAccess(string& sLine, Parser& _parser, Datafile& _data, const Settings& _option)
{
	mu::CachedDataAccess _access;
	Indices _idx;
	bool isCluster = false;

	for (size_t i = 0; i < _parser.HasCachedAccess(); i++)
	{
		// Get the current cached data access
		_access = _parser.GetCachedAccess(i);

		isCluster = _data.isCluster(_access.sAccessEquation);

		if (_access.sAccessEquation.find("().") != string::npos)
		{
			// handle cached MAF methods
			_parser.SetVectorVar(_access.sVectorName, MafDataAccess(_data, getMafFromAccessString(_access.sAccessEquation), _access.sCacheName, createMafDataAccessString(_access.sAccessEquation, _parser)));
			continue;
		}

		// Read the indices
		_idx = parser_getIndices(_access.sAccessEquation, _parser, _data, _option);

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


// This function simply replaces every occurences of the entity with its replacement
static void replaceEntityStringOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityStringReplacement)
{
	size_t nPos = 0;
	while ((nPos = sLine.find(sEntityOccurence)) != string::npos)
	{
		sLine.replace(nPos, sEntityOccurence.length(), sEntityStringReplacement);
	}
}

// This function replaces every occurence of the entity with either the vector name for the parser or with its statistics value
static void replaceEntityOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityName, const string& sEntityReplacement, const Indices& _idx, Datafile& _data, Parser& _parser, const Settings& _option, bool isCluster)
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

				if (_data.containsTablesOrClusters(sArg) || sArg.find("data(") != string::npos)
					getDataElements(sArg, _parser, _data, _option);

				_parser.SetExpr(sArg);
				dRef = _parser.Eval();
				sArg = getNextArgument(sLeft, true);

				if (_data.containsTablesOrClusters(sArg) || sArg.find("data(") != string::npos)
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

				if (_data.containsTablesOrClusters(sArg) || sArg.find("data(") != string::npos)
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


// This function handles the MAF data access using methods
static void handleMafDataAccess(string& sLine, const string& sMafAccess, Parser& _parser, Datafile& _data)
{
	size_t nPos = 0;
	// Replace the access string with its corresponding vector name
	string sMafVectorName = createMafVectorName(sMafAccess);

	// Set the vector variable with its value for the parser
	_parser.SetVectorVar(sMafVectorName, MafDataAccess(_data, getMafFromAccessString(sMafAccess), sMafAccess.substr(0, sMafAccess.find('(')), createMafDataAccessString(sMafAccess, _parser)));

	// Create a cached access and store it
	mu::CachedDataAccess _access = {sMafAccess, sMafVectorName, sMafAccess.substr(0, sMafAccess.find('('))};
	_parser.CacheCurrentAccess(_access);

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


// This function returns the modifier list obtained from the method string
// Pass 'DATA().FNC().cols().lines().grid().every(a,b)'
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


// This function returns the name of the MAF
static string getMafFromAccessString(const string& sAccessString)
{
	// Store these values statically
	static const int sMafListLength = 13;
	static string sMafList[sMafListLength] = {"std", "avg", "prd", "sum", "min", "max", "norm", "num", "cnt", "med", "and", "or", "xor"};

	for (int i = 0; i < sMafListLength; i++)
	{
		if (sAccessString.find("." + sMafList[i]) != string::npos)
			return sMafList[i];
	}
	return "";
}


// This function returns the first MAF access in the passed sLine string
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


// This function simply performs the MAF access and returns the vector of the results
static vector<double> MafDataAccess(Datafile& _data, const string& sMafname, const string& sCache, const string& sMafAccess)
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

	// return a vector with one NAN
	return vector<double>(1, NAN);
}


// This function creates an every definition from the methods, which the Datafile class is able to parse
static string createEveryDefinition(const string& sLine, Parser& _parser)
{
	value_type* v = 0;
	int nResults = 0;
	string sExpr = sLine.substr(sLine.find(".every(") + 6);
	sExpr.erase(getMatchingParenthesis(sExpr));
	string sEvery;
	if (sExpr.front() == '(')
		sExpr.erase(0, 1);
	if (sExpr.back() == ')')
		sExpr.pop_back();

	// Disable the access caching in this case, because it might interfere with the bytecode caching
	_parser.DisableAccessCaching();
	_parser.SetExpr(sExpr);
	v = _parser.Eval(nResults);

	if (nResults > 1)
	{
		sEvery = "every=" + toString((int)v[0]) + "," + toString((int)v[1]) + " ";
	}
	else
		sEvery = "every=" + toString((int)v[0]) + " ";
	return sEvery;
}


// This function simply returns the vector name obtained from the MAF method string
static string createMafVectorName(string sAccessString)
{
	sAccessString.replace(sAccessString.find("()"), 2, "[");
	sAccessString = replaceToVectorname(sAccessString);
	return sAccessString + "]";
}


// This function returns the last token of the current string, e.g. a function name, etc.
static string getLastToken(const string& sLine)
{
	string sToken = sLine;
	size_t pos = string::npos;
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


// this function is for extracting the desired number of columns of the data out of the data object
// and storing it to a continous block of memory used for example for
// regularize, spline, pulse, stfa
bool getData(const string& sTableName, Indices& _idx, const Datafile& _data, Datafile& _cache, int nDesiredCols, bool bSort)
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
            _cache.writeToTable(i, 0, "cache", _data.getElement(_idx.row[i], _idx.col.front(), sTableName));
            _cache.writeToTable(i, 1, "cache", _data.getElement(_idx.row[i], _idx.col.last(), sTableName));

            if (!i)
            {
                _cache.setHeadLineElement(0, "cache", _data.getHeadLineElement(_idx.col.front(), sTableName));
                _cache.setHeadLineElement(1, "cache", _data.getHeadLineElement(_idx.col.last(), sTableName));
            }
        }
    }
    else
    {
        for (long long int i = 0; i < _idx.row.size(); i++)
        {
            for (long long int j = 0; j < _idx.col.size(); j++)
            {
                _cache.writeToTable(i, j, "cache", _data.getElement(_idx.row[i], _idx.col[j], sTableName));

                if (!i)
                    _cache.setHeadLineElement(j, "cache", _data.getHeadLineElement(_idx.col[j], sTableName));
            }
        }
    }

	// sort, if sorting is activated
	if (bSort)
		_cache.sortElements("cache -sort c=1[2:]");

	return true;
}


// This function will extract the needed data into a table object
NumeRe::Table parser_extractData(const string& sDataExpression, Parser& _parser, Datafile& _data, const Settings& _option)
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


// This function evaluates the column indices and returns the final dimension of the columns
static int evalColumnIndicesAndGetDimension(Datafile& _data, Parser& _parser, const string& sDatatable, const string& sDataExpression, Indices& _idx, int nColumns, bool isCluster, const Settings& _option)
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

// This function will calculate the indices from the passed
// data expression and return them optimized for the plotting
// and the fitting algorithm
Indices getIndicesForPlotAndFit(const string& sExpression, string& sDataTable, int& nColumns, bool& openEnd, bool& isCluster)
{
    Indices _idx;
    openEnd = false;
    isCluster = false;
    sDataTable = "data";

    Datafile& _data = NumeReKernel::getInstance()->getData();
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    Settings& _option = NumeReKernel::getInstance()->getSettings();

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

// This function will copy the contents to the target table and extract the table
static NumeRe::Table copyAndExtract(Datafile& _data, const string& sDatatable, const Indices& _idx, int nDim)
{
    MemoryManager _cache;
    // Copy the contents of the data into the local cache object
    // The indices are vectors
    if (nDim == 2)
    {
        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            _cache.writeToTable(i, 0, "cache", _data.getElement(_idx.row[i], _idx.col[0], sDatatable));
            _cache.writeToTable(i, 1, "cache", _data.getElement(_idx.row[i], _idx.col[1], sDatatable));
        }
    }
    else if (nDim == 3)
    {
        for (size_t i = 0; i < _idx.row.size(); i++)
        {
            _cache.writeToTable(i, 0, "cache", _data.getElement(_idx.row[i], _idx.col[0], sDatatable));
            _cache.writeToTable(i, 1, "cache", _data.getElement(_idx.row[i], _idx.col[1], sDatatable));
            _cache.writeToTable(i, 2, "cache", _data.getElement(_idx.row[i], _idx.col[2], sDatatable));
        }
    }

	// Sort the elements
	_cache.sortElements("cache -sort c=1[2:]");

	// Rename the table
	if (sDatatable != "cache")
        _cache.renameTable("cache", sDatatable, true);

	// Return the extracted table object
	return _cache.extractTable(sDatatable);
}


// --> Prueft, ob ein Ausdruck Nicht-Leer ist (also auch, dass er nicht nur aus Leerzeichen besteht) <--
bool isNotEmptyExpression(const string& sExpr)
{
	if (!sExpr.length())
		return false;
	return sExpr.find_first_not_of(' ') != string::npos;
}

// This function checks, whether the passed command line contains
// the syntax for a cluster candidate, splits up the command line
// (if selected using doCut) and declares the new cluster, if it
// is not known up to now.
bool isClusterCandidate(string& sLine, string& sCluster, bool doCut)
{
    // Do nothing, if the current line does not contain
    // an assignment operator
    if (sLine.find('=') == string::npos)
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
            // Extract the cluster including its braces
            sCluster = sLine.substr(0, getMatchingParenthesis(sLine.substr(i))+i+1);

            // If the command line shall be splitted, do that
            // here
            if (doCut)
            {
                sLine.erase(0, sCluster.length());

                size_t nextCharPos = sLine.find_first_not_of(' ');

                if (nextCharPos != string::npos && sLine[nextCharPos] == '=' && sLine[nextCharPos+1] != '=')
                    sLine.erase(0, nextCharPos + 1);
            }

            StripSpaces(sCluster);

            // Get a reference to the datafile object
            Datafile& _data = NumeReKernel::getInstance()->getData();

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

// This function returns the data from the selected object
// and switches automatically between tables and clusters
double getDataFromObject(const string& sObject, long long int i, long long int j, bool isCluster)
{
    // Get a reference to the datafile object
    Datafile& _data = NumeReKernel::getInstance()->getData();

    // Fallback to ensure that valid indices are read
    if (i < 0 || j < 0)
        return NAN;

    // return the data depending on the passed isCluster
    // boolean, the object name and its indices
    return isCluster ? _data.getCluster(sObject).getDouble(i) : _data.getElement(i, j, sObject);
}

/* --> Diese Funktion teilt den String sToSplit am char cSep auf, wobei oeffnende und schliessende
 *     Klammern beruecksichtigt werden <--
 */
int parser_SplitArgs(string& sToSplit, string& sSecArg, const char& cSep, const Settings& _option, bool bIgnoreSurroundingParenthesis)
{
	size_t nFinalParenthesis = 0;
	int nSep = -1;

	StripSpaces(sToSplit);

	if (!bIgnoreSurroundingParenthesis)
	{
		// Get the matchin parenthesis
		// In this case is the opening parenthesis the first character
		nFinalParenthesis = getMatchingParenthesis(sToSplit);
		if (nFinalParenthesis == string::npos)
            throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sToSplit, "(");

        // remove the parentheses
        sToSplit = sToSplit.substr(1, nFinalParenthesis - 1);
	}
	else
	{
	    // Assume that there would be surrounding parentheses
	    nFinalParenthesis = sToSplit.length() + 1;
	}

	// --> Suchen wir nach dem char cSep <--
	for (unsigned int i = 0; i < sToSplit.length(); i++)
	{
	    // search the matching parentheses
	    if (sToSplit[i] == '(' || sToSplit[i] == '[' || sToSplit[i] == '{')
        {
            size_t nMatch = getMatchingParenthesis(sToSplit.substr(i));
            if (nMatch == string::npos)
                throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sToSplit, i);
            i += nMatch;
        }
		// If the separator character was found, stop the current loop
		if (sToSplit[i] == cSep)
		{
			nSep = i;
			break;
		}
	}

	if (nSep == -1)
	{
		throw SyntaxError(SyntaxError::SEPARATOR_NOT_FOUND, sToSplit, SyntaxError::invalid_position);
	}

	// --> Teilen wir nun den string sToSplit in sSecArg und sToSplit auf <--
	sSecArg = sToSplit.substr(nSep + 1);
	sToSplit = sToSplit.substr(0, nSep);

	// return the position of the final parenthesis
	return nFinalParenthesis;
}


/* --> Diese Funktion prueft, ob das Argument, dass sich zwischen sLeft und sRight befindet, in einer
 *     Multi-Argument-Funktion steht <--
 */
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


// --> Prueft, ob der zweite Index groesser als der erste ist und vertauscht sie ggf. <--
void parser_CheckIndices(int& nIndex_1, int& nIndex_2)
{
	if (nIndex_1 < 0)
		nIndex_1 = 0;
	if (nIndex_2 < nIndex_1)
	{
		int nTemp = nIndex_1;
		nIndex_1 = nIndex_2,
		nIndex_2 = nTemp;
		if (nIndex_1 < 0)
			nIndex_1 = 0;
	}
	return;
}


// --> Prueft, ob der zweite Index groesser als der erste ist und vertauscht sie ggf. <--
void parser_CheckIndices(long long int& nIndex_1, long long int& nIndex_2)
{
	if (nIndex_1 < 0)
		nIndex_1 = 0;
	if (nIndex_2 < nIndex_1)
	{
		long long int nTemp = nIndex_1;
		nIndex_1 = nIndex_2,
		nIndex_2 = nTemp;
		if (nIndex_1 < 0)
			nIndex_1 = 0;
	}
	return;
}



