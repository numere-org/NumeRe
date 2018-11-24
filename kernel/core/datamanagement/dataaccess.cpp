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


static string handleCachedDataAccess(string& sLine, Parser& _parser, Datafile& _data, const Settings& _option);
static void replaceEntityStringOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityStringReplacement);
static void replaceEntityOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityName, const string& sEntityReplacement, const Indices& _idx, Datafile& _data, Parser& _parser, const Settings& _option);
static string createMafDataAccessString(const string& sAccessString, Parser& _parser);
static string createEveryDefinition(const string& sLine, Parser& _parser);
static string createMafVectorName(string sAccessString);
static vector<double> MafDataAccess(Datafile& _data, const string& sMafname, const string& sCache, const string& sMafAccess);
static string getMafFromAccessString(const string& sAccessString);
static string getMafAccessString(const string& sLine, const string& sEntity);
static void handleMafDataAccess(string& sLine, const string& sMafAccess, Parser& _parser, Datafile& _data);
static string getLastToken(const string& sLine);
static int evalColumnIndicesAndGetDimension(Datafile& _data, Parser& _parser, const string& sDatatable, const string& sDataExpression, string si_pos[2], string sj_pos[3], vector<long long int>& vLine, vector<long long int>& vCol, int i_pos[2], int j_pos[3], const Settings& _option);
static Table copyAndExtract(Datafile& _data, const string& sDatatable, const vector<long long int>& vLine, const vector<long long int>& vCol, int i_pos[2], int j_pos[3], int nDim);

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
	if (_data.containsCacheElements(sLine))
	{
		// --> Ist links vom ersten "cache(" ein "=" oder ueberhaupt ein "=" im gesamten Ausdruck? <--
		eq_pos = sLine.find("=");
		if (eq_pos == string::npos              // gar kein "="?
				|| !_data.containsCacheElements(sLine.substr(0, eq_pos))   // nur links von "cache("?
				|| (_data.containsCacheElements(sLine.substr(0, eq_pos))  // wenn rechts von "cache(", dann nur Logikausdruecke...
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
			if (_data.containsCacheElements(sCache.substr(sCache.find('(') + 1)))
			{
				_data.setCacheStatus(true);

				sLine_Temp = sCache.substr(sCache.find('(') + 1);
				for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
				{
					if (sLine_Temp.find(iter->first + "(") != string::npos)
						replaceDataEntities(sLine_Temp, iter->first + "(", _data, _parser, _option, bReplaceNANs);
				}
				sCache = sCache.substr(0, sCache.find('(') + 1) + sLine_Temp;
				_data.setCacheStatus(false);
			}
			sLine_Temp = sLine.substr(eq_pos + 1);

			// --> Gibt es rechts von "=" nochmals "cache("? <--
			if (_data.containsCacheElements(sLine_Temp))
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
	string sEntityName = sEntity.substr(0, sEntity.find('('));
	unsigned int nPos = 0;
	bool bWriteStrings = false;
	bool bWriteFileName = false;
	vector<double> vEntityContents;
	string sEntityReplacement = "";
	string sEntityStringReplacement = "";

	// handle MAF methods. sEntity already has "(" at its back
	while ((nPos = sLine.find(sEntity + ").", nPos)) != string::npos)
	{
		if (isInQuotes(sLine, nPos, true))
		{
			nPos++;
			continue;
		}
		handleMafDataAccess(sLine, getMafAccessString(sLine, sEntity), _parser, _data);
	}
	nPos = 0;
	while ((nPos = sLine.find(sEntity + ")", nPos)) != string::npos)
	{
		if (isInQuotes(sLine, nPos, true)
            || (nPos && isalpha(sLine[nPos-1])))
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
		if ((_idx.nI[0] == -1 || _idx.nJ[0] == -1) && !_idx.vI.size() && !_idx.vJ.size())
			throw SyntaxError(SyntaxError::INVALID_INDEX, sLine, SyntaxError::invalid_position);
		if (_idx.nI[1] == -2 && _idx.nJ[1] == -2)
			throw SyntaxError(SyntaxError::NO_MATRIX, sLine, SyntaxError::invalid_position);

		// evaluate the indices regarding the possible combinations:
		// -1: no index
		// -2: larges possible index
		// -3: string access in the current dimension
		if (_idx.nI[1] == -1)
			_idx.nI[1] = _idx.nI[0];
		if (_idx.nJ[1] == -1)
			_idx.nJ[1] = _idx.nJ[0];
		if (_idx.nJ[1] == -2)
			_idx.nJ[1] = _data.getCols(sEntityName, false) - 1;
		if (_idx.nI[1] == -2)
			_idx.nI[1] = _data.getLines(sEntityName, true) - _data.getAppendedZeroes(_idx.nJ[0], sEntityName) - 1;
		if (_idx.nI[0] == -3)
			bWriteStrings = true;
		if (_idx.nJ[0] == -3)
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
			if (_idx.vJ.size())
			{
				for (size_t j = 0; j < _idx.vJ.size(); j++)
				{
					sEntityStringReplacement += "\"" + _data.getHeadLineElement(_idx.vJ[j], sEntityName) + "\", ";
				}
			}
			else
			{
				for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
				{
					sEntityStringReplacement += "\"" + _data.getHeadLineElement(j, sEntityName) + "\", ";
				}
			}
			if (sEntityStringReplacement.length())
				sEntityStringReplacement.erase(sEntityStringReplacement.rfind(','));
		}
		else
		{
			// This is a usual data access
			// create a vector containing the data
			if (_idx.vI.size() && _idx.vJ.size())
			{
				vEntityContents = _data.getElement(_idx.vI, _idx.vJ, sEntityName);
			}
			else
			{
				parser_CheckIndices(_idx.nI[0], _idx.nI[1]);
				parser_CheckIndices(_idx.nJ[0], _idx.nJ[1]);

				// In theory, this will also handle matrix accesses by unrolling the matrix into a logical array
				for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
				{
					for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
					{
						vEntityContents.push_back(_data.getElement(i, j, sEntityName));
					}
				}
			}
		}

		// replace the occurences
		if (bWriteStrings)
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
			mu::CachedDataAccess _access = {sEntityName + "(" + _idx.sCompiledAccessEquation + ")", sEntityReplacement, sEntityName};
			_parser.CacheCurrentAccess(_access);

			// Replace the occurences
			replaceEntityOccurence(sLine, sEntityOccurence, sEntityName, sEntityReplacement, _idx, _data, _parser, _option);
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

	for (size_t i = 0; i < _parser.HasCachedAccess(); i++)
	{
		// Get the current cached data access
		_access = _parser.GetCachedAccess(i);

		if (_access.sAccessEquation.find("().") != string::npos)
		{
			// handle cached MAF methods
			_parser.SetVectorVar(_access.sVectorName, MafDataAccess(_data, getMafFromAccessString(_access.sAccessEquation), _access.sCacheName, createMafDataAccessString(_access.sAccessEquation, _parser)));
			continue;
		}

		// Read the indices
		_idx = parser_getIndices(_access.sAccessEquation, _parser, _data, _option);

		// check the indices
		if ((_idx.nI[0] == -1 || _idx.nJ[0] == -1) && !_idx.vI.size() && !_idx.vJ.size())
			throw SyntaxError(SyntaxError::INVALID_INDEX, sLine, SyntaxError::invalid_position);
		if (_idx.nI[1] == -2 && _idx.nJ[1] == -2)
			throw SyntaxError(SyntaxError::NO_MATRIX, sLine, SyntaxError::invalid_position);

		// Evaluate the indices
		if (_idx.nI[1] == -1)
			_idx.nI[1] = _idx.nI[0];
		if (_idx.nJ[1] == -1)
			_idx.nJ[1] = _idx.nJ[0];
		if (_idx.nI[1] == -2)
			_idx.nI[1] = _data.getLines(_access.sCacheName, false) - 1;
		if (_idx.nJ[1] == -2)
			_idx.nJ[1] = _data.getCols(_access.sCacheName, false) - 1;

		vector<long long int> vLine;
		vector<long long int> vCol;

		// Fill the vectors (either with the already available vectors or by constructing the index vectors)
		if (_idx.vI.size() && _idx.vJ.size())
		{
			vLine = _idx.vI;
			vCol = _idx.vJ;
		}
		else
		{
			for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
				vLine.push_back(i);
			for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
				vCol.push_back(j);
		}

		// Get new data (Parser::GetVectorVar returns a pointer to the vector var) and update the stored elements in the internal representation
		_data.copyElementsInto(_parser.GetVectorVar(_access.sVectorName), vLine, vCol, _access.sCacheName);
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
static void replaceEntityOccurence(string& sLine, const string& sEntityOccurence, const string& sEntityName, const string& sEntityReplacement, const Indices& _idx, Datafile& _data, Parser& _parser, const Settings& _option)
{
	sLine = " " + sLine + " ";

	vector<long long int> vLine;
	vector<long long int> vCol;
	size_t nPos = 0;

	// Fill the vectors with the needed index values
	if (_idx.vI.size() && _idx.vJ.size())
	{
		vLine = _idx.vI;
		vCol = _idx.vJ;
	}
	else
	{
		for (long long int i = _idx.nI[0]; i <= _idx.nI[1]; i++)
			vLine.push_back(i);
		for (long long int j = _idx.nJ[0]; j <= _idx.nJ[1]; j++)
			vCol.push_back(j);
	}

	// As long as the entity occurs
	while ((nPos = sLine.find(sEntityOccurence)) != string::npos)
	{
		// Get the last token
		string sLeft = getLastToken(sLine.substr(0, nPos));

		if (sLeft.length() < 3 || sLeft.back() != '(' || sLine[sLine.find_first_not_of(" ", nPos + sEntityOccurence.length())] != ')')
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
						+ toCmdString(_data.std(sEntityName, vLine, vCol))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "avg(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("avg(", sLine.find(sEntityOccurence)))
						+ toCmdString(_data.avg(sEntityName, vLine, vCol))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "max(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("max(", sLine.find(sEntityOccurence)))
						+ toCmdString(_data.max(sEntityName, vLine, vCol))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "min(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("min(", sLine.find(sEntityOccurence)))
						+ toCmdString(_data.min(sEntityName, vLine, vCol))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "prd(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("prd(", sLine.find(sEntityOccurence)))
						+ toCmdString(_data.prd(sEntityName, vLine, vCol))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "sum(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("sum(", sLine.find(sEntityOccurence)))
						+ toCmdString(_data.sum(sEntityName, vLine, vCol))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "num(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("num(", sLine.find(sEntityOccurence)))
						+ toCmdString(_data.num(sEntityName, vLine, vCol))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "and(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("and(", sLine.find(sEntityOccurence)))
						+ toCmdString(_data.and_func(sEntityName, vLine, vCol))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "xor(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("xor(", sLine.find(sEntityOccurence)))
						+ toCmdString(_data.xor_func(sEntityName, vLine, vCol))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "or(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("or(", sLine.find(sEntityOccurence)))
						+ toCmdString(_data.or_func(sEntityName, vLine, vCol))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "cnt(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("cnt(", sLine.find(sEntityOccurence)))
						+ toCmdString(_data.cnt(sEntityName, vLine, vCol))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "med(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("med(", sLine.find(sEntityOccurence)))
						+ toCmdString(_data.med(sEntityName, vLine, vCol))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "norm(")
			{
				_parser.DisableAccessCaching();
				sLine = sLine.substr(0, sLine.rfind("norm(", sLine.find(sEntityOccurence)))
						+ toCmdString(_data.norm(sEntityName, vLine, vCol))
						+ sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
			}
			else if (sLeft == "cmp(")
			{
				// cmp() is more difficult
				_parser.DisableAccessCaching();
				double dRef = 0.0;
				int nType = 0;
				string sArg = "";
				sLeft = sLine.substr(sLine.find(sLeft) + sLeft.length(), getMatchingParenthesis(sLine.substr(sLine.find(sLeft) + sLeft.length() - 1)) - 2);
				sArg = getNextArgument(sLeft, true);
				sArg = getNextArgument(sLeft, true);
				if (_data.containsCacheElements(sArg) || sArg.find("data(") != string::npos)
					getDataElements(sArg, _parser, _data, _option);
				_parser.SetExpr(sArg);
				dRef = (double)_parser.Eval();
				sArg = getNextArgument(sLeft, true);
				if (_data.containsCacheElements(sArg) || sArg.find("data(") != string::npos)
					getDataElements(sArg, _parser, _data, _option);
				_parser.SetExpr(sArg);
				nType = (int)_parser.Eval();
				sLine = sLine.replace(sLine.rfind("cmp(", sLine.find(sEntityOccurence)),
									  getMatchingParenthesis(sLine.substr(sLine.rfind("cmp(", sLine.find(sEntityOccurence)) + 3)) + 4,
									  toCmdString(_data.cmp(sEntityName, vLine, vCol, dRef, nType)));
			}
			else if (sLeft == "pct(")
			{
				// pct() is more difficult
				_parser.DisableAccessCaching();
				double dPct = 0.5;
				string sArg = "";
				sLeft = sLine.substr(sLine.find(sLeft) + sLeft.length(), getMatchingParenthesis(sLine.substr(sLine.find(sLeft) + sLeft.length() - 1)) - 2);
				sArg = getNextArgument(sLeft, true);
				sArg = getNextArgument(sLeft, true);
				if (_data.containsCacheElements(sArg) || sArg.find("data(") != string::npos)
					getDataElements(sArg, _parser, _data, _option);
				_parser.SetExpr(sArg);
				dPct = _parser.Eval();
				sLine = sLine.replace(sLine.rfind("pct(", sLine.find(sEntityOccurence)),
									  getMatchingParenthesis(sLine.substr(sLine.rfind("pct(", sLine.find(sEntityOccurence)) + 3)) + 4,
									  toCmdString(_data.pct(sEntityName, vLine, vCol, dPct)));
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


// this function is for extracting the data out of the data object and storing it to a continous block of memory
// used for example for the FFT, the FWT and the fitting algorithm
bool getData(const string& sTableName, Indices& _idx, const Datafile& _data, Datafile& _cache, int nDesiredCols, bool bSort)
{
	// write the data
	// the first case uses vector indices
	if (_idx.vI.size() || _idx.vJ.size())
	{
		for (long long int i = 0; i < _idx.vI.size(); i++)
		{
			for (long long int j = 0; j < _idx.vJ.size(); j++)
			{
				_cache.writeToCache(i, j, "cache", _data.getElement(_idx.vI[i], _idx.vJ[j], sTableName));
				if (!i)
					_cache.setHeadLineElement(j, "cache", _data.getHeadLineElement(_idx.vJ[j], sTableName));
			}
		}
	}
	else // this is for the case of a continous block of data
	{
		// Check the indices
		if (_idx.nI[0] == -1 || _idx.nJ[0] == -1)
			throw SyntaxError(SyntaxError::INVALID_INDEX, sTableName, sTableName);

		// Evaluate the indices
		if (_idx.nI[1] == -1)
			_idx.nI[1] = _idx.nI[0];
		else if (_idx.nI[1] == -2)
			_idx.nI[1] = _data.getLines(sTableName, false) - 1;
		if (_idx.nJ[1] == -1)
			_idx.nJ[1] = _idx.nJ[0];
		else if (_idx.nJ[1] == -2)
		{
			_idx.nJ[1] = _idx.nJ[0] + nDesiredCols - 1;
		}
		if (_data.getCols(sTableName, false) <= _idx.nJ[1])
			throw SyntaxError(SyntaxError::INVALID_INDEX, sTableName, sTableName);

		// Write the data
		for (long long int i = 0; i <= _idx.nI[1] - _idx.nI[0]; i++)
		{
			if (nDesiredCols == 2)
			{
				_cache.writeToCache(i, 0, "cache", _data.getElement(_idx.nI[0] + i, _idx.nJ[0], sTableName));
				_cache.writeToCache(i, 1, "cache", _data.getElement(_idx.nI[0] + i, _idx.nJ[1], sTableName));
				if (!i)
				{
					_cache.setHeadLineElement(0, "cache", _data.getHeadLineElement(_idx.nJ[0], sTableName));
					_cache.setHeadLineElement(1, "cache", _data.getHeadLineElement(_idx.nJ[1], sTableName));
				}
			}
			else
			{
				for (long long int j = 0; j <= _idx.nJ[1] - _idx.nJ[0]; j++)
				{
					_cache.writeToCache(i, j, "cache", _data.getElement(_idx.nI[0] + i, _idx.nJ[0] + j, sTableName));
					if (!i)
						_cache.setHeadLineElement(j, "cache", _data.getHeadLineElement(_idx.nJ[0] + j, sTableName));
				}
			}
		}
	}
	// sort, if sorting is activated
	if (bSort)
	{
		_cache.sortElements("cache -sort c=1[2:]");
	}
	return true;
}


// This function will extract the needed data into a table object
Table parser_extractData(const string& sDataExpression, Parser& _parser, Datafile& _data, const Settings& _option)
{
	string si_pos[2] = {"", ""};                    // String-Array fuer die Zeilen-Position: muss fuer alle Spalten identisch sein!
	string sj_pos[3] = {"", "", ""};                // String-Array fuer die Spalten: kann bis zu drei beliebige Werte haben
	string sDatatable = "data";
	int i_pos[2] = {0, 0};                          // Int-Array fuer den Wert der Zeilen-Positionen
	int j_pos[3] = {0, 0, 0};                       // Int-Array fuer den Wert der Spalten-Positionen
	int nMatch = 0;                                 // Int fuer die Position des aktuellen find-Treffers eines Daten-Objekts
	vector<long long int> vLine;
	vector<long long int> vCol;
	value_type* v = 0;
	int nResults = 0;

	// --> Ist da "cache" drin? Aktivieren wir den Cache-Status <--
	if (_data.containsCacheElements(sDataExpression) && sDataExpression.substr(0, 5) != "data(")
	{
		if (_data.isValidCache())
			_data.setCacheStatus(true);
		else
			throw SyntaxError(SyntaxError::NO_CACHED_DATA, sDataExpression, SyntaxError::invalid_position);

		// Find the current data table name
		for (auto iter = _data.mCachesMap.begin(); iter != _data.mCachesMap.end(); ++iter)
		{
			if (sDataExpression.find(iter->first + "(") != string::npos
                && (!sDataExpression.find(iter->first + "(")
                    || (sDataExpression.find(iter->first + "(") && checkDelimiter(sDataExpression.substr(sDataExpression.find(iter->first + "(") - 1, (iter->first).length() + 2)))))
			{
				sDatatable = iter->first;
				break;
			}
		}
	}
	else if (!_data.isValid())
		throw SyntaxError(SyntaxError::NO_DATA_AVAILABLE, sDataExpression, SyntaxError::invalid_position);

	// --> Klammer und schliessende Klammer finden und in einen anderen String schreiben <--
	nMatch = sDataExpression.find('(');
	si_pos[0] = sDataExpression.substr(nMatch, getMatchingParenthesis(sDataExpression.substr(nMatch)) + 1);

	// Handle the abbreviation "DATA() = DATA(:,:)"
	if (si_pos[0] == "()" || si_pos[0][si_pos[0].find_first_not_of(' ', 1)] == ')')
		si_pos[0] = "(:,:)";

    // If there's a data table in the arguments, get their contents here
	if (si_pos[0].find("data(") != string::npos || _data.containsCacheElements(si_pos[0]))
	{
		getDataElements(si_pos[0],  _parser, _data, _option);
	}

	// --> Rausgeschnittenen String am Komma ',' in zwei Teile teilen <--
    parser_SplitArgs(si_pos[0], sj_pos[0], ',', _option);

	// --> Gibt's einen Doppelpunkt? Dann teilen wir daran auch noch mal <--
	if (si_pos[0].find(':') != string::npos && si_pos[0].find('{') == string::npos)
	{
		si_pos[0] = "( " + si_pos[0] + " )";
        parser_SplitArgs(si_pos[0], si_pos[1], ':', _option);

        // Set the last index to "inf", if it is missing
		if (!isNotEmptyExpression(si_pos[1]))
			si_pos[1] = "inf";
	}
	else
		si_pos[1] = "";

	// --> Auswerten mit dem Parser <--
	// Get the first index
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

    // Get the last index
	if (si_pos[1] == "inf")
	{
		i_pos[1] = _data.getLines(sDatatable, false) - 1;
	}
	else if (isNotEmptyExpression(si_pos[1]))
	{
		_parser.SetExpr(si_pos[1]);
		i_pos[1] = (int)_parser.Eval() - 1;
	}
	else if (!vLine.size())
		i_pos[1] = i_pos[0] + 1;

	// --> Pruefen, ob die Reihenfolge der Indices sinnvoll ist <--
	parser_CheckIndices(i_pos[0], i_pos[1]);

    // Fallback solution for the column indices
	if (!isNotEmptyExpression(sj_pos[0]))
		sj_pos[0] = "0";

	/* --> Jetzt fuer die Spalten: Fummelig. Man soll bis zu 6 Spalten angeben koennen und
	 *     das Programm sollte trotzdem einen Sinn darin finden <--
	 */
    int nDim = evalColumnIndicesAndGetDimension(_data, _parser, sDatatable, sDataExpression, si_pos, sj_pos, vLine, vCol, i_pos, j_pos, _option);

	// Fill the column vector indices, if the line indices are already a vector
	if (vLine.size() && !vCol.size())
	{
		for (int n = 0; n < nDim; n++)
			vCol.push_back(j_pos[n]);
	}

	parser_CheckIndices(i_pos[0], i_pos[1]);

	// Validate, if the line indices have a reasonable large difference
	if (abs(i_pos[0] - i_pos[1]) <= 1 && vLine.size() <= 1)
		throw SyntaxError(SyntaxError::TOO_FEW_LINES, sDataExpression, SyntaxError::invalid_position);

    // copy the data and extract the table
    return copyAndExtract(_data, sDatatable, vLine, vCol, i_pos, j_pos, nDim);
}


// This function evaluates the column indices and returns the final dimension of the columns
static int evalColumnIndicesAndGetDimension(Datafile& _data, Parser& _parser, const string& sDatatable, const string& sDataExpression, string si_pos[2], string sj_pos[3], vector<long long int>& vLine, vector<long long int>& vCol, int i_pos[2], int j_pos[3], const Settings& _option)
{
	value_type* v = 0;
	int nResults = 0;
    int j = 0;
    int nDim = 0;

    // As long as the index list contains a colon
    while (sj_pos[j].find(':') != string::npos && sj_pos[j].find('{') && j < 2)
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

	// --> Alle nicht-beschriebenen Grenzen-Strings auf "" setzen <--
	for (int k = j + 1; k < 3; k++)
		sj_pos[k] = "";

	// --> Grenzen-Strings moeglichst sinnvoll auswerten <--
	for (int k = 0; k <= j; k++)
	{
		// --> "inf" bedeutet "infinity". Ergo: die letztmoegliche Spalte <--
		if (sj_pos[k] == "inf")
		{
			j_pos[k] = _data.getCols(sDatatable);
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

    // Ensure consistent indices
    if (i_pos[1] > _data.getLines(sDatatable, false))
        i_pos[1] = _data.getLines(sDatatable, false);
	if (j_pos[1] > _data.getCols(sDatatable) - 1)
		j_pos[1] = _data.getCols(sDatatable) - 1;

    // Validate the calculated indices
	if (!vLine.size()
        && !vCol.size()
        && (j_pos[0] < 0
            || j_pos[1] < 0
            || i_pos[0] > _data.getLines(sDatatable, false)
            || i_pos[1] > _data.getLines(sDatatable, false)
            || j_pos[0] > _data.getCols(sDatatable) - 1
            || j_pos[1] > _data.getCols(sDatatable) - 1))
	{
		throw SyntaxError(SyntaxError::INVALID_INDEX, sDataExpression, SyntaxError::invalid_position);
	}

	// --> Jetzt wissen wir die Spalten: Suchen wir im Falle von si_pos[1] == inf nach der laengsten <--
	if (si_pos[1] == "inf")
	{
		int nAppendedZeroes = _data.getAppendedZeroes(j_pos[0], sDatatable);
		for (int k = 1; k <= j; k++)
		{
			if (nAppendedZeroes > _data.getAppendedZeroes(j_pos[k], sDatatable))
				nAppendedZeroes = _data.getAppendedZeroes(j_pos[k], sDatatable);
		}
		if (nAppendedZeroes < i_pos[1])
			i_pos[1] = _data.getLines(sDatatable, true) - nAppendedZeroes - 1;
	}


	/* --> Bestimmen wir die "Dimension" des zu fittenden Datensatzes. Dabei ist es auch
	 *     von Bedeutung, ob Fehlerwerte verwendet werden sollen <--
	 */
	nDim = 0;
	if (j == 0 && vCol.size() < 2)
		throw SyntaxError(SyntaxError::TOO_FEW_COLS, sDataExpression, SyntaxError::invalid_position);
	else if (j == 0)
		nDim = vCol.size();
	else
	{
		nDim = j + 1;
	}

	return nDim;
}


// This function will copy the contents to the target table and extract the table
static Table copyAndExtract(Datafile& _data, const string& sDatatable, const vector<long long int>& vLine, const vector<long long int>& vCol, int i_pos[2], int j_pos[3], int nDim)
{
    Cache _cache;
    // Copy the contents of the data into the local cache object
	if (!vLine.size())
	{
	    // The indices are casual
		if (nDim == 2)
		{
			for (int i = 0; i <= abs(i_pos[0] - i_pos[1]); i++)
			{
				_cache.writeToCache(i, 0, "cache", _data.getElement(i + i_pos[0], j_pos[0], sDatatable));
				_cache.writeToCache(i, 1, "cache", _data.getElement(i + i_pos[0], j_pos[1], sDatatable));
			}
		}
		else if (nDim == 3)
		{
			for (int i = 0; i <= abs(i_pos[0] - i_pos[1]); i++)
			{
				_cache.writeToCache(i, 0, "cache", _data.getElement(i + i_pos[0], j_pos[0], sDatatable));
				_cache.writeToCache(i, 1, "cache", _data.getElement(i + i_pos[0], j_pos[1], sDatatable));
				_cache.writeToCache(i, 2, "cache", _data.getElement(i + i_pos[0], j_pos[2], sDatatable));
			}
		}
	}
	else
	{
	    // The indices are vectors
		if (nDim == 2)
		{
			for (size_t i = 0; i < vLine.size(); i++)
			{
				_cache.writeToCache(i, 0, "cache", _data.getElement(vLine[i], vCol[0], sDatatable));
				_cache.writeToCache(i, 1, "cache", _data.getElement(vLine[i], vCol[1], sDatatable));
			}
		}
		else if (nDim == 3)
		{
			for (size_t i = 0; i < vLine.size(); i++)
			{
				_cache.writeToCache(i, 0, "cache", _data.getElement(vLine[i], vCol[0], sDatatable));
				_cache.writeToCache(i, 1, "cache", _data.getElement(vLine[i], vCol[1], sDatatable));
				_cache.writeToCache(i, 2, "cache", _data.getElement(vLine[i], vCol[2], sDatatable));
			}
		}
	}

	// Sort the elements
	_cache.sortElements("cache -sort c=1[2:]");

	// Rename the table
	if (sDatatable != "cache")
        _cache.renameCache("cache", sDatatable, true);

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



