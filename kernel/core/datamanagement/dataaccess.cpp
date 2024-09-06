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
/// \param isAssignment bool
/// \param isLocal bool
///
/////////////////////////////////////////////////
DataAccessParser::DataAccessParser(StringView sCommand, bool isAssignment, bool isLocal)
{
    size_t pos = std::string::npos;
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
            if (pos == std::string::npos && (sCommand[i] == '_' || isalpha(sCommand[i])))
                pos = i;

            // Is a possible start character available
            // and the current character is not part of a
            // valid object identifier character set?
            if (pos != std::string::npos && sCommand[i] != '_' && sCommand[i] != '~' && !isalnum(sCommand[i]))
            {
                // If the current character is an opening parenthesis
                // or an opening brace, parse the identified data
                // access, otherwise simply reset the possible starting
                // character
                if (sCommand[i] == '(')
                {
                    // This is a usual table
                    sDataObject = sCommand.subview(pos, i - pos).to_string();

                    // Ensure that the table exists
                    if (!instance->getMemoryManager().isTable(sDataObject))
                    {
                        sDataObject.clear();
                        pos = std::string::npos;
                        continue;
                    }

                    if (isLocal)
                    {
                        // Calculate the indices. Has to use a copy of the parser
                        mu::Parser p = instance->getParser();
                        ::getIndices(sCommand.subview(pos), idx, p, instance->getMemoryManager(),
                                     isAssignment);
                    }
                    else
                        ::getIndices(sCommand.subview(pos), idx, instance->getParser(), instance->getMemoryManager(),
                                     isAssignment);
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
                        pos = std::string::npos;
                        continue;
                    }

                    // Calculate the indices and switch the access
                    // to a cluster access. Has to use a copy of the parser
                    bIsCluster = true;
                    mu::Parser p = instance->getParser();
                    ::getIndices(sCommand.subview(pos), idx, p, instance->getMemoryManager(),
                                 isAssignment);
                    break;
                }
                else
                    pos = std::string::npos;
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
/// \param asMatrix bool Set to false to use the
/// first column for determining the number of
/// rows.
/// \return void
///
/////////////////////////////////////////////////
void DataAccessParser::evalIndices(bool asMatrix)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    if (idx.row.isOpenEnd())
    {
        if (bIsCluster)
            idx.row.setRange(0, _data.getCluster(sDataObject).size() - 1);
        else if (asMatrix)
            idx.row.setRange(0, _data.getLines(sDataObject) - 1);
        else
            idx.row.setRange(0, _data.getColElements(idx.col.subidx(0, 1), sDataObject) - 1);
    }

    if (idx.col.isOpenEnd())
    {
        if (bIsCluster)
            idx.col = VectorIndex(idx.col.front());
        else
            idx.col.setRange(0, _data.getCols(sDataObject) - 1);
    }
}


/////////////////////////////////////////////////
/// \brief Returns a reference to the data object
/// identifier.
///
/// \return std::string&
///
/////////////////////////////////////////////////
std::string& DataAccessParser::getDataObject()
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
/// \brief Determines, whether the data access
/// contains indices corresponding to a matrix
/// access.
///
/// \return bool
///
/////////////////////////////////////////////////
bool DataAccessParser::isMatrix() const
{
    return idx.col.size() > 1 && idx.row.size() > 1;
}


/////////////////////////////////////////////////
/// \brief Convenience wrapper method for the
/// global function.
///
/// \return std::vector<size_t>
///
/////////////////////////////////////////////////
std::vector<size_t> DataAccessParser::getDataGridDimensions() const
{
    return ::getDataGridDimensions(idx, sDataObject);
}


static void resolveTablesAndClusters(std::string& sLine, mu::Parser& _parser, MemoryManager& _data, int options);
static std::string handleCachedDataAccess(std::string& sLine, mu::Parser& _parser, MemoryManager& _data);
static void replaceSingleAccess(std::string& sLine, const std::string& sEntityOccurence, DataAccessParser&& _access, MemoryManager& _data, mu::Parser& _parser, int options);
static void replaceEntityOccurence(std::string& sLine, const std::string& sEntityOccurence, const std::string& sEntityName, const std::string& sEntityReplacement, const Indices& _idx, MemoryManager& _data, mu::Parser& _parser, bool isCluster);
static std::string createMafDataAccessString(const std::string& sAccessString, mu::Parser& _parser);
static std::string createEveryCellDefinition(const std::string& sLine, const std::string& sType, mu::Parser& _parser);
static std::string createMafVectorName(std::string sAccessString);
static std::vector<std::complex<double>> MafDataAccess(MemoryManager& _data, const std::string& sMafname, const std::string& sCache, const std::string& sMafAccess);
static std::string getMafFromAccessString(const std::string& sAccessString);
static std::string getMafAccessString(const std::string& sLine, const std::string& sEntity);
static void handleMafDataAccess(std::string& sLine, const std::string& sMafAccess, mu::Parser& _parser, MemoryManager& _data);
static StringView getLastToken(StringView sLine);


/////////////////////////////////////////////////
/// \brief Static helper function to detect a
/// possible assignment operator.
///
/// \param sCmd StringView
/// \return size_t
///
/////////////////////////////////////////////////
size_t findAssignmentOperator(StringView sCmd)
{
    size_t nQuotes = 0;

    for (size_t i = 0; i < sCmd.length(); i++)
    {
        if (sCmd[i] == '"' && (!i || sCmd[i - 1] != '\\'))
            nQuotes++;

        if (!nQuotes)
        {
            if (sCmd[i] == '(' || sCmd[i] == '{')
            {
                size_t match = getMatchingParenthesis(sCmd.subview(i));

                if (match != std::string::npos)
                    i += match;
            }

            if (sCmd[i] == '='
                    && i > 0
                    && sCmd[i - 1] != '!'
                    && sCmd[i - 1] != '<'
                    && sCmd[i - 1] != '>'
                    && sCmd[i + 1] != '='
                    && sCmd[i - 1] != '=')
                return i;
        }
    }

    return std::string::npos;
}


/////////////////////////////////////////////////
/// \brief Searches the passed string for calls
/// to any table or cluster and replaces them with
/// internal vectors or their respective values.
///
/// \param sLine std::string&
/// \param _parser mu::Parser&
/// \param _data Datafile&
/// \param options int
/// \return std::string
///
/// This function actually delegates the hard work
/// to replaceDataEntities(), which is itself
/// public and replaces all calls to a single
/// data entity.
/////////////////////////////////////////////////
std::string getDataElements(std::string& sLine, mu::Parser& _parser, MemoryManager& _data, int options)
{
    // Evaluate possible cached equations
    if ((_parser.HasCachedAccess() || _parser.GetCachedEquation().length()) && !_parser.IsCompiling())
        return handleCachedDataAccess(sLine, _parser, _data);

    // Validate the number of parentheses
    if (!validateParenthesisNumber(sLine))
        throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sLine, sLine.find_first_of("({[]})"));

    std::string sCache;             // Rueckgabe-string: Ggf. der linke Teil der Gleichung, falls es sich um eine Zuweisung handelt
    size_t eq_pos = std::string::npos;                // int zum Zwischenspeichern der Position des "="


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
                    && getMatchingParenthesis(sCache) == sCache.length() - 1)
            {
                StringView source(sLine, eq_pos + 1);
                source.strip();

                // Is the source also a complete table
                if (source.find('(') != std::string::npos
                        && getMatchingParenthesis(source) == source.length() - 1
                        && _data.isTable(source))
                {
                    // We need to parse the source first, because it might be a
                    // standard vector access, which does not work with this copy
                    // logic
                    DataAccessParser src(source, false);

                    // If it is not a matrix, fall back to standard parsing
                    if (!src.isMatrix())
                    {
                        // Convert the StringView first, because its internal data
                        // will be mutated in the next step
                        std::string s = source.to_string();
                        sLine.erase(0, eq_pos + 1);
                        replaceSingleAccess(sLine, s, std::move(src), _data, _parser, options);
                        return sCache;
                    }

                    if (!_data.isTable(sCache))
                        _data.addTable(sCache, NumeReKernel::getInstance()->getSettings());

                    DataAccessParser tgt(sCache, true);

                    src.evalIndices();

                    _data.copyTable(src.getDataObject(), src.getIndices(), tgt.getDataObject(), tgt.getIndices());
                    sLine = _parser.CreateTempVar(mu::Array(std::vector<std::complex<double>>({src.getIndices().row.size(), src.getIndices().col.size()})));
                    return "";
                }
            }
        }

        if (eq_pos == std::string::npos              // gar kein "="?
                || !_data.containsTablesOrClusters(sCache))   // nur links von "cache("?
        {
            resolveTablesAndClusters(sLine, _parser, _data, options);
            sCache.clear();
        }
        else
        {
            /* --> Nein? Dann ist das eine Zuweisung. Wird komplizierter zu loesen. Außerdem kann dann rechts von
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
                std::string sLine_Temp = sCache.substr(sCache.find_first_of("({") + 1);
                resolveTablesAndClusters(sLine_Temp, _parser, _data, options);
                sCache = sCache.substr(0, sCache.find_first_of("({") + 1) + sLine_Temp;
            }

            sLine.erase(0, eq_pos + 1);

            // --> Gibt es rechts von "=" nochmals "cache("? <--
            if (_data.containsTablesOrClusters(sLine))
                resolveTablesAndClusters(sLine, _parser, _data, options);
        }
    }

    return sCache;
}


/////////////////////////////////////////////////
/// \brief Resolves every call to a cluster or a
/// table.
///
/// \param sLine std::string&
/// \param _parser mu::Parser&
/// \param _data Datafile&
/// \param options int
/// \return void
///
/////////////////////////////////////////////////
static void resolveTablesAndClusters(std::string& sLine, mu::Parser& _parser, MemoryManager& _data, int options)
{
    // Try to find every cache and handle its contents
    if (_data.containsTables(sLine))
    {
        for (auto iter = _data.getTableMap().begin(); iter != _data.getTableMap().end(); iter++)
        {
            if (sLine.find((iter->first) + "(") != std::string::npos)
                replaceDataEntities(sLine, iter->first + "(", _data, _parser, options);
        }
    }

    // Try to find every cluster and handle its contents
    if (_data.containsClusters(sLine))
    {
        for (auto iter = _data.getClusterMap().begin(); iter != _data.getClusterMap().end(); iter++)
        {
            if (sLine.find((iter->first) + "{") != std::string::npos)
                replaceDataEntities(sLine, iter->first + "{", _data, _parser, options);
        }
    }
}


/////////////////////////////////////////////////
/// \brief This function replaces all calls to a
/// single data entity with an internal vector or
/// its value, respectively.
///
/// \param sLine std::string&
/// \param sEntity const std::string&
/// \param _data Datafile&
/// \param _parser mu::Parser&
/// \param options int
/// \return void
///
/// Because this function calls the index parser,
/// it will automatically resolve all nested calls
/// to any data entity included in the current call
/// to the specified data entity.
/////////////////////////////////////////////////
void replaceDataEntities(std::string& sLine, const std::string& sEntity, MemoryManager& _data, mu::Parser& _parser, int options)
{
    size_t nPos = 0;
    bool isCluster = sEntity.back() == '{';

    // handle MAF methods. sEntity already has "(" at its back
    while (!isCluster && (nPos = sLine.find(sEntity + ").", nPos)) != std::string::npos)
    {
        if (isInQuotes(sLine, nPos, true) || (nPos && !isDelimiter(sLine[nPos - 1]) && sLine[nPos - 1] != '~'))
        {
            nPos++;
            continue;
        }

        handleMafDataAccess(sLine, getMafAccessString(sLine, sEntity), _parser, _data);
    }

    nPos = 0;

    // handle logical table accesses
    while (!isCluster && (nPos = sLine.find(sEntity + ")", nPos)) != std::string::npos)
    {
        if (isInQuotes(sLine, nPos, true) || (nPos && !isDelimiter(sLine[nPos - 1]) && sLine[nPos - 1] != '~'))
        {
            nPos++;
            continue;
        }

        sLine.replace(nPos, sEntity.length() + 1, (_data.getCols(StringView(sEntity, 0, sEntity.length() - 1)) ? "true" : "false"));
    }

    if (sLine.find(sEntity) == std::string::npos)
        return;

    nPos = 0;

    // Replace all remaining occurences with actual vectors
    do
    {
        // Locate the next entity occurence
        nPos = sLine.find(sEntity, nPos);

        // Ensure that this is delimited occurence
        if (nPos && ((!isDelimiter(sLine[nPos - 1]) && sLine[nPos - 1] != '~') || isInQuotes(sLine, nPos, true)))
        {
            nPos++;
            continue;
        }

        // Get the current entity
        StringView sEntityOccurence(sLine, nPos);
        sEntityOccurence = sEntityOccurence.subview(0, getMatchingParenthesis(sEntityOccurence) + 1);

        // Decode and replace the current detected data entity access
        replaceSingleAccess(sLine, sEntityOccurence.to_string(), DataAccessParser(sEntityOccurence, false), _data, _parser, options);
    }
    while (sLine.find(sEntity, nPos) != std::string::npos);
}


/////////////////////////////////////////////////
/// \brief Decode and replace a single detected
/// data entity access.
///
/// \param sLine std::string&
/// \param sEntityOccurence const std::string&
/// \param _access DataAccessParser&&
/// \param _data MemoryManager&
/// \param _parser mu::Parser&
/// \param options int
/// \return void
///
/////////////////////////////////////////////////
static void replaceSingleAccess(std::string& sLine, const std::string& sEntityOccurence, DataAccessParser&& _access, MemoryManager& _data, mu::Parser& _parser, int options)
{
    mu::Array vEntityContents;
    std::string sEntityStringReplacement;
    const std::string& sEntityName = _access.getDataObject();
    bool bWriteStrings = false;
    bool bWriteFileName = false;
    bool isCluster = _access.isCluster();
    Indices& _idx = _access.getIndices();

    // check the indices, whether they are possible in the current context
    if (!isValidIndexSet(_idx))
        throw SyntaxError(SyntaxError::INVALID_INDEX, sLine, SyntaxError::invalid_position, _access.getIndexString());

    if (!isCluster && _access.isMatrix())
        throw SyntaxError(SyntaxError::NO_MATRIX, sLine, SyntaxError::invalid_position);

    // evaluate the indices
    _access.evalIndices(false);

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
        mu::Array vStringContents;

        // Get the headlines
        for (size_t j = 0; j < _idx.col.size(); j++)
            vStringContents.push_back(_data.getHeadLineElement(_idx.col[j], sEntityName));

        sEntityStringReplacement = _parser.CreateTempVar(vStringContents);
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
        if (!cluster.size())
            vEntityContents.push_back(mu::Value());
        else
        {
            // Create the vector using the indices
            for (size_t i = 0; i < _idx.row.size(); i++)
                vEntityContents.push_back(cluster.getValue(_idx.row[i]));
        }
    }

    // replace the occurences
    if (sEntityStringReplacement.length())
    {
        // Replace the strings (we don't need caching here)
        _parser.DisableAccessCaching();
        replaceAll(sLine, sEntityOccurence, sEntityStringReplacement);
    }
    else
    {
        // Replace the numerical occurences
        // Define the vector name
        std::string sEntityReplacement = replaceToVectorname(sEntityOccurence);

        // Set the vector variable and its value for the parser
        _parser.SetInternalVar(sEntityReplacement, vEntityContents);

        // Cache the current access if needed
        if (_parser.IsCompiling() && _parser.CanCacheAccess())
        {
            mu::CachedDataAccess _access = {sEntityName + (isCluster
                                            ? "{" + _idx.sCompiledAccessEquation + "}"
                                            : "(" + _idx.sCompiledAccessEquation + ")"),
                                            sEntityReplacement, sEntityName,
                                            isCluster ? mu::CachedDataAccess::IS_CLUSTER : mu::CachedDataAccess::NO_FLAG
                                           };
            _parser.CacheCurrentAccess(_access);
        }

        // Replace the occurences
        replaceEntityOccurence(sLine, sEntityOccurence, sEntityName, sEntityReplacement, _idx, _data, _parser, isCluster);
    }
}


/////////////////////////////////////////////////
/// \brief This function handles cached data
/// access strings, which may be available in the
/// parser.
///
/// \param sLine std::string&
/// \param _parser mu::Parser&
/// \param _data Datafile&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string handleCachedDataAccess(std::string& sLine, mu::Parser& _parser, MemoryManager& _data)
{
    for (size_t i = 0; i < _parser.HasCachedAccess(); i++)
    {
        // Get the current cached data access
        const mu::CachedDataAccess& _access = _parser.GetCachedAccess(i);

        if (_access.flags & mu::CachedDataAccess::IS_TABLE_METHOD)
        {
            // handle cached MAF methods
            _parser.SetInternalVar(_access.sVectorName, MafDataAccess(_data,
                                   getMafFromAccessString(_access.sAccessEquation),
                                   _access.sCacheName,
                                   createMafDataAccessString(_access.sAccessEquation, _parser)));
            continue;
        }

        // Create an index
        Indices _idx;

        bool isCluster = _access.flags & mu::CachedDataAccess::IS_CLUSTER;

        // Read the indices
        getIndices(_access.sAccessEquation, _idx, _parser, _data, false);

        // check the indices
        if (!isValidIndexSet(_idx))
            throw SyntaxError(SyntaxError::INVALID_INDEX, sLine, SyntaxError::invalid_position, _idx.row.to_string() + ", " + _idx.col.to_string());

        if (!isCluster && _idx.row.isOpenEnd() && _idx.col.isOpenEnd())
            throw SyntaxError(SyntaxError::NO_MATRIX, sLine, SyntaxError::invalid_position);

        // Evaluate the indices
        if (_idx.row.isOpenEnd())
            _idx.row.setRange(0, isCluster ? _data.getCluster(_access.sCacheName).size() - 1 : _data.getLines(_access.sCacheName, false) - 1);

        if (_idx.col.isOpenEnd())
            _idx.col.setRange(0, isCluster ? 0 : _data.getCols(_access.sCacheName, false) - 1);

        // Get new data (Parser::GetVectorVar returns a pointer to the vector var) and update the stored elements in the internal representation
        if (isCluster)
        {
            NumeRe::Cluster& clst = _data.getCluster(_access.sCacheName);
            clst.insertDataInArray(_parser.GetInternalVar(_access.sVectorName), _idx.row);
        }
        else
            _data.copyElementsInto(_parser.GetInternalVar(_access.sVectorName), _idx.row, _idx.col, _access.sCacheName);
    }

    // Update the equation (probably there are cached elements, which could not be cached)
    sLine = _parser.GetCachedEquation();
    return _parser.GetCachedTarget();
}



/////////////////////////////////////////////////
/// \brief Small helper function for
/// replaceEntityOccurence to create a temporary
/// name based upon a defined variable name.
///
/// \param sVectName const std::string&
/// \param val const mu::Value&
/// \param _parser mu::Parser&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string createTempVar(const std::string& sVectName, const mu::Value& val, mu::Parser& _parser)
{
    _parser.SetInternalVar(sVectName, {val});
    return sVectName;
}


/////////////////////////////////////////////////
/// \brief This function replaces every occurence
/// of the entity with either the vector name for
/// the parser or with its statistics value.
///
/// \param sLine std::string&
/// \param sEntityOccurence const std::string&
/// \param sEntityName const std::string&
/// \param sEntityReplacement const std::string&
/// \param _idx const Indices&
/// \param _data Datafile&
/// \param _parser mu::Parser&
/// \param isCluster bool
/// \return void
///
/////////////////////////////////////////////////
static void replaceEntityOccurence(std::string& sLine, const std::string& sEntityOccurence, const std::string& sEntityName, const std::string& sEntityReplacement, const Indices& _idx, MemoryManager& _data, mu::Parser& _parser, bool isCluster)
{
    sLine = " " + sLine + " ";

    size_t nPos = 0;

    // As long as the entity occurs
    while ((nPos = sLine.find(sEntityOccurence, nPos)) != std::string::npos)
    {
        // if the last character before the current occurence is
        // alphanumeric, then ignore this occurence
        if (nPos && (isalnum(sLine[nPos - 1]) || sLine[nPos - 1] == '_'))
        {
            nPos++;
            continue;
        }

        // Get the last token
        StringView sLeft = getLastToken(StringView(sLine, 0, nPos));

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
            // NOTE: Those lines cannot be removed at the moment due to bytecode issues. Will be fixed
            // with the major parser rework
            /*if (sLeft == "std(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("std(", sLine.find(sEntityOccurence)))
                        + createTempVar(sEntityReplacement + "~std", isCluster ? _data.getCluster(sEntityName).std(_idx.row) : _data.std(sEntityName, _idx.row, _idx.col), _parser)
                        + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
            }
            else if (sLeft == "avg(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("avg(", sLine.find(sEntityOccurence)))
                        + createTempVar(sEntityReplacement + "~avg", isCluster ? _data.getCluster(sEntityName).avg(_idx.row) : _data.avg(sEntityName, _idx.row, _idx.col), _parser)
                        + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
            }
            else if (sLeft == "max(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("max(", sLine.find(sEntityOccurence)))
                        + createTempVar(sEntityReplacement + "~max", isCluster ? _data.getCluster(sEntityName).max(_idx.row) : _data.max(sEntityName, _idx.row, _idx.col), _parser)
                        + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
            }
            else if (sLeft == "min(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("min(", sLine.find(sEntityOccurence)))
                        + createTempVar(sEntityReplacement + "~min", isCluster ? _data.getCluster(sEntityName).min(_idx.row) : _data.min(sEntityName, _idx.row, _idx.col), _parser)
                        + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
            }
            else if (sLeft == "prd(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("prd(", sLine.find(sEntityOccurence)))
                        + createTempVar(sEntityReplacement + "~prd", isCluster ? _data.getCluster(sEntityName).prd(_idx.row) : _data.prd(sEntityName, _idx.row, _idx.col), _parser)
                        + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
            }
            else if (sLeft == "sum(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("sum(", sLine.find(sEntityOccurence)))
                        + createTempVar(sEntityReplacement + "~sum", isCluster ? _data.getCluster(sEntityName).sum(_idx.row) : _data.sum(sEntityName, _idx.row, _idx.col), _parser)
                        + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
            }
            else if (sLeft == "num(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("num(", sLine.find(sEntityOccurence)))
                        + createTempVar(sEntityReplacement + "~num", isCluster ? _data.getCluster(sEntityName).num(_idx.row) : _data.num(sEntityName, _idx.row, _idx.col), _parser)
                        + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
            }
            else if (sLeft == "and(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("and(", sLine.find(sEntityOccurence)))
                        + createTempVar(sEntityReplacement + "~and", isCluster ? _data.getCluster(sEntityName).and_func(_idx.row) : _data.and_func(sEntityName, _idx.row, _idx.col), _parser)
                        + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
            }
            else if (sLeft == "xor(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("xor(", sLine.find(sEntityOccurence)))
                        + createTempVar(sEntityReplacement + "~xor", isCluster ? _data.getCluster(sEntityName).xor_func(_idx.row) : _data.xor_func(sEntityName, _idx.row, _idx.col), _parser)
                        + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
            }
            else if (sLeft == "or(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("or(", sLine.find(sEntityOccurence)))
                        + createTempVar(sEntityReplacement + "~or", isCluster ? _data.getCluster(sEntityName).or_func(_idx.row) : _data.or_func(sEntityName, _idx.row, _idx.col), _parser)
                        + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
            }
            else if (sLeft == "cnt(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("cnt(", sLine.find(sEntityOccurence)))
                        + createTempVar(sEntityReplacement + "~cnt", isCluster ? _data.getCluster(sEntityName).cnt(_idx.row) : _data.cnt(sEntityName, _idx.row, _idx.col), _parser)
                        + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
            }
            else if (sLeft == "med(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("med(", sLine.find(sEntityOccurence)))
                        + createTempVar(sEntityReplacement + "~med", isCluster ? _data.getCluster(sEntityName).med(_idx.row) : _data.med(sEntityName, _idx.row, _idx.col), _parser)
                        + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
            }
            else if (sLeft == "norm(")
            {
                _parser.DisableAccessCaching();
                sLine = sLine.substr(0, sLine.rfind("norm(", sLine.find(sEntityOccurence)))
                        + createTempVar(sEntityReplacement + "~norm", isCluster ? _data.getCluster(sEntityName).norm(_idx.row) : _data.norm(sEntityName, _idx.row, _idx.col), _parser)
                        + sLine.substr(sLine.find(')', sLine.find(sEntityOccurence) + sEntityOccurence.length()) + 1);
            }
            else if (sLeft == "cmp(")
            {
                // cmp() is more difficult
                _parser.DisableAccessCaching();
                mu::Array dRef;
                int nType = 0;
                std::string sArg = "";
                size_t start = sLine.find(sLeft.to_string());
                sLeft = StringView(sLine, start + sLeft.length(),
                                   getMatchingParenthesis(StringView(sLine, start + sLeft.length() - 1)) - 1);
                getNextViewedArgument(sLeft);
                sArg = getNextViewedArgument(sLeft).to_string();

                if (_data.containsTablesOrClusters(sArg))
                    getDataElements(sArg, _parser, _data);

                _parser.SetExpr(sArg);
                dRef = _parser.Eval();
                sArg = getNextViewedArgument(sLeft).to_string();

                if (_data.containsTablesOrClusters(sArg))
                    getDataElements(sArg, _parser, _data);

                _parser.SetExpr(sArg);
                nType = _parser.Eval().getAsScalarInt();
                sLine = sLine.replace(sLine.rfind("cmp(", sLine.find(sEntityOccurence)),
                                      getMatchingParenthesis(StringView(sLine, sLine.rfind("cmp(", sLine.find(sEntityOccurence)) + 3)) + 4,
                                      createTempVar(sEntityReplacement + "~cmp", isCluster ? _data.getCluster(sEntityName).cmp(_idx.row, dRef.front().getNum().asCF64(), nType) : _data.cmp(sEntityName, _idx.row, _idx.col, dRef.front().getNum().asCF64(), nType), _parser));
            }
            else if (sLeft == "pct(")
            {
                // pct() is more difficult
                _parser.DisableAccessCaching();
                mu::Array dPct;
                std::string sArg = "";
                size_t start = sLine.find(sLeft.to_string());
                sLeft = StringView(sLine, start + sLeft.length(),
                                   getMatchingParenthesis(StringView(sLine, start + sLeft.length() - 1)) - 1);
                getNextViewedArgument(sLeft);
                sArg = getNextViewedArgument(sLeft).to_string();

                if (_data.containsTablesOrClusters(sArg))
                    getDataElements(sArg, _parser, _data);

                _parser.SetExpr(sArg);
                dPct = _parser.Eval();
                sLine = sLine.replace(sLine.rfind("pct(", sLine.find(sEntityOccurence)),
                                      getMatchingParenthesis(StringView(sLine, sLine.rfind("pct(", sLine.find(sEntityOccurence)) + 3)) + 4,
                                      createTempVar(sEntityReplacement + "~pct", isCluster ? _data.getCluster(sEntityName).pct(_idx.row, dPct.front().getNum().asCF64()) : _data.pct(sEntityName, _idx.row, _idx.col, dPct.front().getNum().asCF64()), _parser));
            }
            else */ //Fallback
                sLine.replace(nPos, sEntityOccurence.length(), sEntityReplacement);
        }
    }
}


/////////////////////////////////////////////////
/// \brief This function handles the MAF data
/// access using methods.
///
/// \param sLine std::string&
/// \param sMafAccess const std::string&
/// \param _parser mu::Parser&
/// \param _data Datafile&
/// \return void
///
/////////////////////////////////////////////////
static void handleMafDataAccess(std::string& sLine, const std::string& sMafAccess, mu::Parser& _parser, MemoryManager& _data)
{
    size_t nPos = 0;

    // Replace the access string with its corresponding vector name
    std::string sMafVectorName = createMafVectorName(sMafAccess);

    // If the return value is not already a numerical vector
    if (!_parser.ContainsInternalVars(sMafVectorName, false) && sMafVectorName != "true" && sMafVectorName != "false")
    {
        // Set the vector variable with its value for the parser
        _parser.SetInternalVar(sMafVectorName,
                               MafDataAccess(_data,
                                             getMafFromAccessString(sMafAccess),
                                             sMafAccess.substr(0, sMafAccess.find('(')),
                                             createMafDataAccessString(sMafAccess, _parser)));

        // Create a cached access and store it
        mu::CachedDataAccess _access = {sMafAccess,
                                        sMafVectorName,
                                        sMafAccess.substr(0, sMafAccess.find('(')),
                                        mu::CachedDataAccess::IS_TABLE_METHOD
                                       };
        _parser.CacheCurrentAccess(_access);
    }
    else
        _parser.DisableAccessCaching();

    // Replace every occurence
    while ((nPos = sLine.find(sMafAccess, nPos)) != std::string::npos)
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
/// \param sAccessString const std::string&
/// \param _parser mu::Parser&
/// \return std::string
///
/// Pass 'DATA().FNC().cols().rows().grid().every(a,b)'
/////////////////////////////////////////////////
static std::string createMafDataAccessString(const std::string& sAccessString, mu::Parser& _parser)
{
    std::string sDataMaf;

    if (sAccessString.find(".grid") != std::string::npos)
        sDataMaf += "grid";

    if (sAccessString.find(".cols") != std::string::npos)
        sDataMaf += "cols";

    if (sAccessString.find(".lines") != std::string::npos)
        sDataMaf += "lines";

    if (sAccessString.find(".rows") != std::string::npos)
        sDataMaf += "lines";

    if (sAccessString.find(".every(") != std::string::npos)
        sDataMaf += createEveryCellDefinition(sAccessString, "every", _parser);

    if (sAccessString.find(".cells(") != std::string::npos)
        sDataMaf += createEveryCellDefinition(sAccessString, "cells", _parser);

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
                && (pos + sMafList[i].length() + 1 >= sAccessString.length() || sAccessString[pos + sMafList[i].length() + 1] == '.' || isDelimiter(sAccessString[pos + sMafList[i].length() + 1])))
            return sMafList[i];
    }

    // Special case: we only have TAB().rows or TAB().cols
    if ((pos = sAccessString.find(".rows")) != std::string::npos)
    {
        if (pos + 4 + 1 >= sAccessString.length()
                || sAccessString[pos + 4 + 1] == '.'
                || isDelimiter(sAccessString[pos + 4 + 1]))
            return "rows";

    }
    else if ((pos = sAccessString.find(".cols")) != std::string::npos)
    {
        if (pos + 4 + 1 >= sAccessString.length()
                || sAccessString[pos + 4 + 1] == '.'
                || isDelimiter(sAccessString[pos + 4 + 1]))
            return "cols";

    }

    return "";
}


/////////////////////////////////////////////////
/// \brief This function returns the first MAF
/// access in the passed sLine string.
///
/// \param sLine const std::string&
/// \param sEntity const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string getMafAccessString(const std::string& sLine, const std::string& sEntity)
{
    size_t nPos = 0;
    static std::string sDelim = "+-*/^%&| ,=<>!()[]{}"; // delimiters (no "."!)

    if ((nPos = sLine.find(sEntity + ").")) != std::string::npos)
    {
        // go through the string and try to find a MAF acces
        for (size_t i = nPos; i < sLine.length(); i++)
        {
            if (sLine[i] == '(' || sLine[i] == '[' || sLine[i] == '{')
                i += getMatchingParenthesis(StringView(sLine, i)) + 1;

            if (i >= sLine.length())
                return sLine.substr(nPos);

            if (sDelim.find(sLine[i]) != std::string::npos)
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
/// \param sMafname const std::string&
/// \param sCache const std::string&
/// \param sMafAccess const std::string&
/// \return std::vector<std::complex<double>>
///
/////////////////////////////////////////////////
static std::vector<std::complex<double>> MafDataAccess(MemoryManager& _data, const std::string& sMafname, const std::string& sCache, const std::string& sMafAccess)
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
        return std::vector<std::complex<double>>(1, _data.size(sCache, sMafAccess.find("grid") != std::string::npos ? "grid" : "").front());

    if (sMafname == "cols")
        return std::vector<std::complex<double>>(1, _data.size(sCache, sMafAccess.find("grid") != std::string::npos ? "grid" : "").back());

    if (sMafname == "maxpos")
        return _data.maxpos(sCache, sMafAccess);

    if (sMafname == "minpos")
        return _data.minpos(sCache, sMafAccess);

    // return a vector with one NAN
    return std::vector<std::complex<double>>(1, NAN);
}


/////////////////////////////////////////////////
/// \brief This function creates an every
/// definition from the methods, which the
/// Datafile class is able to parse.
///
/// \param sLine const std::string&
/// \param sType const std::string&
/// \param _parser mu::Parser&
/// \return string
///
/////////////////////////////////////////////////
static std::string createEveryCellDefinition(const std::string& sLine, const std::string& sType, mu::Parser& _parser)
{
    std::string sExpr = sLine.substr(sLine.find("." + sType + "(") + 1 + sType.length());
    sExpr.erase(getMatchingParenthesis(sExpr) + 1);

    // Resolve possible remaining calls to data tables or clusters
    getDataElements(sExpr, _parser, NumeReKernel::getInstance()->getMemoryManager());

    return sType + "=" + sExpr + " ";
}


/////////////////////////////////////////////////
/// \brief Realizes the "aliasof()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_aliasof(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array a = _kernel->getParser().Eval();

    _kernel->getMemoryManager().addReference(sTableName, a.front().getStr());

    return _kernel->getParser().CreateTempVar(a);
}


/////////////////////////////////////////////////
/// \brief Realizes the "convert()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_convert(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    if (nResults < 2)
        sMethodArguments = "auto";
    else
        sMethodArguments = v[1].front().getStr();

    if (_kernel->getMemoryManager().convertColumns(sTableName, VectorIndex(v[0]), sMethodArguments))
        return _kernel->getParser().CreateTempVar(mu::Value(sMethodArguments));

    return _kernel->getParser().CreateTempVar(mu::Value(""));
}


/////////////////////////////////////////////////
/// \brief Realizes the "typeof()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_typeof(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();
    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    std::vector<std::string> vRet;

    for (size_t i = 0; i < v[0].size(); i++)
    {
        TableColumn::ColumnType type = _kernel->getMemoryManager().getType(VectorIndex(v[0][i].getNum().asI64() - 1), sTableName);
        vRet.push_back(TableColumn::typeToString(type));
    }

    if (vRet.size())
        return _kernel->getParser().CreateTempVar(vRet);

    return _kernel->getParser().CreateTempVar(mu::Value(""));
}


/////////////////////////////////////////////////
/// \brief Realizes the "categoriesof()" table
/// method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_categories(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();
    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    mu::Array vCategories = _kernel->getMemoryManager().getCategoryList(VectorIndex(v[0]), sTableName);

    if (!vCategories.size())
        return _kernel->getParser().CreateTempVar(mu::Value(""));

    return _kernel->getParser().CreateTempVar(vCategories);
}


/////////////////////////////////////////////////
/// \brief Realizes the "getunit()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_getunit(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();
    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    mu::Array vUnits;

    for (int i : VectorIndex(v[0]))
    {
        vUnits.push_back(_kernel->getMemoryManager().getUnit(i, sTableName));
    }

    return _kernel->getParser().CreateTempVar(vUnits);
}


/////////////////////////////////////////////////
/// \brief Realizes the "setunit()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_setunit(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    if (nResults < 2)
        return _kernel->getParser().CreateTempVar(mu::Value(""));

    for (size_t i = 0; i < std::min(v[0].size(), v[1].size()); i++)
    {
        _kernel->getMemoryManager().setUnit(v[0].get(i).getNum().asI64() - 1, sTableName, v[1].get(i).getStr());
    }

    return _kernel->getParser().CreateTempVar(v[1]);
}


/////////////////////////////////////////////////
/// \brief Realizes the "toSIunits()" table
/// method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_toSiUnits(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    UnitConversionMode mode = MODE_DIRECT;

    int nResults = 0;
    mu::Array* v = nullptr;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    v = _kernel->getParser().Eval(nResults);
    VectorIndex vCols(v[0]);

    if (nResults > 2)
    {
        if (v[1].front() == mu::Value("base"))
            mode = MODE_BASESI;
        else if (v[1].front() == mu::Value("simplify"))
            mode = MODE_SIMPLIFY;

        if (nResults > 3)
        {
            if (mu::all(v[2]))
            {
                mu::Array vUnits;

                for (int i : vCols)
                {
                    vUnits.push_back(_kernel->getMemoryManager().getTable(sTableName)->showUnitConversion(i, mode));
                }

                return _kernel->getParser().CreateTempVar(vUnits);
            }
        }
    }

    return _kernel->getParser().CreateTempVar(_kernel->getMemoryManager().getTable(sTableName)->toSiUnits(vCols, mode));
}


/////////////////////////////////////////////////
/// \brief Realizes the "inSIunits()" table
/// method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_asSiUnits(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    size_t col = v[0].getAsScalarInt() - 1;

    _kernel->getParser().SetInternalVar(sResultVectorName, _kernel->getMemoryManager().getTable(sTableName)->asSiUnits(col));

    return sResultVectorName;
}


/////////////////////////////////////////////////
/// \brief Realizes the "categorize()" table
/// method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_categorize(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();
    std::vector<std::string> vCategories;
    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    if (nResults < 2)
        return _kernel->getParser().CreateTempVar(mu::Value(""));

    vCategories = v[1].as_str_vector();

    if (_kernel->getMemoryManager().setCategories(sTableName, VectorIndex(v[0]), vCategories))
    {
        mu::Array cats = _kernel->getMemoryManager().getCategoryList(VectorIndex(v[0]), sTableName);

        if (!cats.size())
            return _kernel->getParser().CreateTempVar(mu::Value(""));

        return _kernel->getParser().CreateTempVar(cats);
    }

    return _kernel->getParser().CreateTempVar(mu::Value(""));
}


/////////////////////////////////////////////////
/// \brief Realizes the "fndcols()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_findCols(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();
    bool enableRegEx = false;

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    if (nResults > 1)
        enableRegEx = mu::all(v[1]);

    std::vector<std::complex<double>> vCols = _kernel->getMemoryManager().findCols(sTableName, v[0].as_str_vector(), enableRegEx);

    _kernel->getParser().SetInternalVar(sResultVectorName, vCols);
    return sResultVectorName;
}


/////////////////////////////////////////////////
/// \brief Realizes the "countif()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_counteq(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);
    VectorIndex vCols(v[0]);

    if (nResults < 2)
        return "nan";

#warning TODO (numere#2#09/06/24): Enable method chaining
#warning TODO (numere#2#09/04/24): Improve types and interface
    _kernel->getParser().SetInternalVar(sResultVectorName, _kernel->getMemoryManager().countIfEqual(sTableName, vCols, v[1]));
    return sResultVectorName;
}


/////////////////////////////////////////////////
/// \brief Realizes the "indexof()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_index(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);
    size_t col = v[0].getAsScalarInt() - 1;

    if (nResults < 2)
        return "nan";

    _kernel->getParser().SetInternalVar(sResultVectorName, _kernel->getMemoryManager().getIndex(sTableName, col, v[1]));
    return sResultVectorName;
}


/////////////////////////////////////////////////
/// \brief Realizes the "covarof()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_cov(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    if (nResults < 2)
        throw SyntaxError(SyntaxError::TOO_FEW_COLS, sTableName + "().covarof()", ".covarof(", ".covarof(");

    size_t col1 = v[0].getAsScalarInt() - 1;
    size_t col2 = v[1].getAsScalarInt() - 1;

    VectorIndex vIndex1(0, VectorIndex::OPEN_END);
    VectorIndex vIndex2(0, VectorIndex::OPEN_END);

    if (nResults > 2)
    {
        vIndex1 = VectorIndex(v[2]);

        if (nResults > 3)
            vIndex2 = VectorIndex(v[3]);
    }

    _kernel->getParser().SetInternalVar(sResultVectorName, mu::Value(_kernel->getMemoryManager().getCovariance(sTableName, col1, vIndex1, col2, vIndex2)));
    return sResultVectorName;
}


/////////////////////////////////////////////////
/// \brief Realizes the "pcorrof()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_pcorr(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    if (nResults < 2)
        throw SyntaxError(SyntaxError::TOO_FEW_COLS, sTableName + "().pcorrof()", ".pcorrof(", ".pcorrof(");

    size_t col1 = v[0].getAsScalarInt() - 1;
    size_t col2 = v[1].getAsScalarInt() - 1;

    VectorIndex vIndex1(0, VectorIndex::OPEN_END);
    VectorIndex vIndex2(0, VectorIndex::OPEN_END);

    if (nResults > 2)
    {
        vIndex1 = VectorIndex(v[2]);

        if (nResults > 3)
            vIndex2 = VectorIndex(v[3]);
    }

    _kernel->getParser().SetInternalVar(sResultVectorName, mu::Value(_kernel->getMemoryManager().getPearsonCorr(sTableName, col1, vIndex1, col2, vIndex2)));

    return sResultVectorName;
}


/////////////////////////////////////////////////
/// \brief Realizes the "scorrof()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_scorr(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    if (nResults < 2)
        throw SyntaxError(SyntaxError::TOO_FEW_COLS, sTableName + "().scorrof()", ".scorrof(", ".scorrof(");

    size_t col1 = v[0].getAsScalarInt() - 1;
    size_t col2 = v[1].getAsScalarInt() - 1;

    VectorIndex vIndex1(0, VectorIndex::OPEN_END);
    VectorIndex vIndex2(0, VectorIndex::OPEN_END);

    if (nResults > 2)
    {
        vIndex1 = VectorIndex(v[2]);

        if (nResults > 3)
            vIndex2 = VectorIndex(v[3]);
    }

    _kernel->getParser().SetInternalVar(sResultVectorName, mu::Value(_kernel->getMemoryManager().getSpearmanCorr(sTableName, col1, vIndex1, col2, vIndex2)));

    return sResultVectorName;
}


/////////////////////////////////////////////////
/// \brief Realizes the "zscoreof()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_zscore(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    size_t col = v[0].getAsScalarInt() - 1;
    VectorIndex vIndex(0, VectorIndex::OPEN_END);

    if (nResults > 1)
        vIndex = VectorIndex(v[1]);

    _kernel->getParser().SetInternalVar(sResultVectorName, _kernel->getMemoryManager().getZScore(sTableName, col, vIndex));

    return sResultVectorName;
}


/////////////////////////////////////////////////
/// \brief Realizes the "anovaof()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_anova(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    if (nResults < 2)
        throw SyntaxError(SyntaxError::TOO_FEW_COLS, sTableName + "().anovaof()", ".anovaof(", ".anovaof(");

    VectorIndex col1(v[0]);
    size_t col2 = v[1].getAsScalarInt() - 1;

    VectorIndex vIndex(0, VectorIndex::OPEN_END);
    double significance = 0.05;

    if (nResults > 2)
    {
        significance = v[2].front().getNum().asF64();

        if (nResults > 3)
            vIndex = VectorIndex(v[3]);
    }

    std::vector<AnovaResult> res = _kernel->getMemoryManager().getAnova(sTableName, col1, col2, vIndex, significance);

    mu::Array vRet;

    for (size_t i = 0; i < res.size(); i++)
    {
        std::string prefix = res.size() > 1 ? (res[i].prefix + "-") : "";

        vRet.push_back(prefix + "FisherRatio");
        vRet.push_back(res[i].m_FRatio);
        vRet.push_back(prefix + "FisherSignificanceVal");
        vRet.push_back(res[i].m_significanceVal);
        vRet.push_back(prefix + "SignificanceLevel");
        vRet.push_back(res[i].m_significance);
        vRet.push_back(prefix + "SignificantVariation");
        vRet.push_back(res[i].m_isSignificant);
        vRet.push_back(prefix + "Categories");
        vRet.push_back(res[i].m_numCategories);
    }

    _kernel->getParser().SetInternalVar(sResultVectorName, vRet);
    return sResultVectorName;
}


/////////////////////////////////////////////////
/// \brief Realizes the "kmeansof()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_kmeans(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    if (nResults < 2)
        throw SyntaxError(SyntaxError::TOO_FEW_COLS, sTableName + "().kmeansof()", ".kmeansof(", ".kmeansof(");

    VectorIndex cols(v[0]);
    size_t nClusters = v[1].getAsScalarInt();

    VectorIndex vIndex(0, VectorIndex::OPEN_END);
    size_t maxIterations = 100;
    Memory::KmeansInit init_method = Memory::KmeansInit::INIT_RANDOM;
    size_t n_init = 10;

    if (nResults > 2)
    {
        maxIterations = v[2].getAsScalarInt();

        if (nResults > 3)
        {
            init_method = Memory::stringToKmeansInit(v[3].front().getStr());

            if (nResults > 4)
                n_init = v[4].getAsScalarInt();
            else
            {
                // scikit: When n_init='auto', the number of runs depends on the value
                // of init: 10 if using init='random' or init is a callable; 1 if using init='k-means++'
                if (init_method != Memory::KmeansInit::INIT_RANDOM)
                    n_init = 1;
            }
        }
    }

    KMeansResult bestRes = _kernel->getMemoryManager().getKMeans(sTableName, cols, nClusters, maxIterations, init_method);

    for (size_t re_inits = 1; re_inits < n_init; re_inits++)
    {
        KMeansResult res = _kernel->getMemoryManager().getKMeans(sTableName, cols, nClusters, maxIterations, init_method);

        if (res.inertia < bestRes.inertia)
            bestRes = res;
    }

    _kernel->getParser().SetInternalVar(sResultVectorName, bestRes.cluster_labels);
    return sResultVectorName;
}


/////////////////////////////////////////////////
/// \brief Realizes the "binsof()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_binsof(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    size_t col = v[0].getAsScalarInt() - 1;
    size_t nBins = 0;

    if (nResults > 1)
        nBins = v[1].getAsScalarInt();

    _kernel->getParser().SetInternalVar(sResultVectorName, _kernel->getMemoryManager().getBins(sTableName, col, nBins));

    return sResultVectorName;
}


/////////////////////////////////////////////////
/// \brief Realizes the "rankof()" table method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_rank(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    size_t col = v[0].getAsScalarInt() - 1;
    VectorIndex vIndex(0, VectorIndex::OPEN_END);
    Memory::RankingStrategy _strat = Memory::RANK_COMPETETIVE;

    if (nResults > 1)
    {
        if (v[1].front() == mu::Value("f"))
            _strat = Memory::RANK_FRACTIONAL;
        else if (v[1].front() == mu::Value("d"))
            _strat = Memory::RANK_DENSE;

        if (nResults > 2)
            vIndex = VectorIndex(v[2]);
    }

    _kernel->getParser().SetInternalVar(sResultVectorName, _kernel->getMemoryManager().getRank(sTableName, col, vIndex, _strat));

    return sResultVectorName;
}


/////////////////////////////////////////////////
/// \brief Realizes the "describe()" method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_annotate(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    _kernel->getMemoryManager().writeComment(sTableName, v[0].front().getStr());
    return _kernel->getParser().CreateTempVar(mu::Value(v[0].front().getStr()));
}


/////////////////////////////////////////////////
/// \brief Realizes the "insertblock()" method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_insertBlock(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    size_t atRow = v[0].getAsScalarInt() - 1;

    if (nResults < 2)
        return "false";

    size_t atCol = v[1].getAsScalarInt() - 1;
    size_t rows = 1;
    size_t cols = 1;

    if (nResults > 2)
        rows = v[2].getAsScalarInt();

    if (nResults > 3)
        cols = v[3].getAsScalarInt();

    return toString(_kernel->getMemoryManager().insertBlock(sTableName, atRow, atCol, rows, cols));
}


/////////////////////////////////////////////////
/// \brief Realizes the "insertcols()" method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_insertCols(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    size_t col = v[0].getAsScalarInt() - 1;
    size_t num = 1;

    if (nResults > 1)
        num = v[1].getAsScalarInt();

    return toString(_kernel->getMemoryManager().insertCols(sTableName, col, num));
}


/////////////////////////////////////////////////
/// \brief Realizes the "insertrows()" method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_insertRows(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    size_t row = v[0].getAsScalarInt() - 1;
    size_t num = 1;

    if (nResults > 1)
        num = v[1].getAsScalarInt();

    return toString(_kernel->getMemoryManager().insertRows(sTableName, row, num));
}


/////////////////////////////////////////////////
/// \brief Realizes the "removeblock()" method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_removeBlock(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    size_t atRow = v[0].getAsScalarInt() - 1;

    if (nResults < 2)
        return "false";

    size_t atCol = v[1].getAsScalarInt() - 1;
    size_t rows = 1;
    size_t cols = 1;

    if (nResults > 2)
        rows = v[2].getAsScalarInt();

    if (nResults > 3)
        cols = v[3].getAsScalarInt();

    return toString(_kernel->getMemoryManager().removeBlock(sTableName, atRow, atCol, rows, cols));
}


/////////////////////////////////////////////////
/// \brief Realizes the "removecols()" method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_removeCols(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    return toString(_kernel->getMemoryManager().removeCols(sTableName, VectorIndex(v[0])));
}


/////////////////////////////////////////////////
/// \brief Realizes the "removerows()" method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_removeRows(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    int nResults = 0;
    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);

    return toString(_kernel->getMemoryManager().removeRows(sTableName, VectorIndex(v[0])));
}


/////////////////////////////////////////////////
/// \brief Realizes the "reordercols()" method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_reorderCols(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    VectorIndex vIndex;
    VectorIndex vNewOrder;
    int nResults = 0;

    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);
    vIndex = VectorIndex(v[0]);

    if (nResults < 2)
        throw SyntaxError(SyntaxError::TOO_FEW_ARGS, sTableName + "().reordercols()", ".reordercols(", ".reordercols(");

    vNewOrder = VectorIndex(v[1]);

    return toString(_kernel->getMemoryManager().reorderCols(sTableName, vIndex, vNewOrder));
}


/////////////////////////////////////////////////
/// \brief Realizes the "reorderrows()" method.
///
/// \param sTableName const std::string&
/// \param sMethodArguments std::string
/// \param sResultVectorName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string tableMethod_reorderRows(const std::string& sTableName, std::string sMethodArguments, const std::string& sResultVectorName)
{
    NumeReKernel* _kernel = NumeReKernel::getInstance();

    VectorIndex vIndex;
    VectorIndex vNewOrder;
    int nResults = 0;

    _kernel->getMemoryManager().updateDimensionVariables(sTableName);
    _kernel->getParser().SetExpr(sMethodArguments);
    mu::Array* v = _kernel->getParser().Eval(nResults);
    vIndex = VectorIndex(v[0]);

    if (nResults < 2)
        throw SyntaxError(SyntaxError::TOO_FEW_ARGS, sTableName + "().reorderrows()", ".reorderrows(", ".reorderrows(");

    vNewOrder = VectorIndex(v[1]);

    return toString(_kernel->getMemoryManager().reorderRows(sTableName, vIndex, vNewOrder));
}


/////////////////////////////////////////////////
/// \brief Typedef for a table method
/////////////////////////////////////////////////
typedef std::string (*TableMethod)(const std::string&, std::string, const std::string&);


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
    mTableMethods["categoriesof"] = tableMethod_categories;
    mTableMethods["categorize"] = tableMethod_categorize;
    mTableMethods["fndcols"] = tableMethod_findCols;
    mTableMethods["countif"] = tableMethod_counteq;
    mTableMethods["indexof"] = tableMethod_index;
    mTableMethods["covarof"] = tableMethod_cov;
    mTableMethods["pcorrof"] = tableMethod_pcorr;
    mTableMethods["scorrof"] = tableMethod_scorr;
    mTableMethods["rankof"] = tableMethod_rank;
    mTableMethods["zscoreof"] = tableMethod_zscore;
    mTableMethods["anovaof"] = tableMethod_anova;
    mTableMethods["kmeansof"] = tableMethod_kmeans;
    mTableMethods["binsof"] = tableMethod_binsof;
    mTableMethods["insertcells"] = tableMethod_insertBlock;
    mTableMethods["insertcols"] = tableMethod_insertCols;
    mTableMethods["insertrows"] = tableMethod_insertRows;
    mTableMethods["removecells"] = tableMethod_removeBlock;
    mTableMethods["removecols"] = tableMethod_removeCols;
    mTableMethods["removerows"] = tableMethod_removeRows;
    mTableMethods["reordercols"] = tableMethod_reorderCols;
    mTableMethods["reorderrows"] = tableMethod_reorderRows;
    mTableMethods["getunit"] = tableMethod_getunit;
    mTableMethods["setunit"] = tableMethod_setunit;
    mTableMethods["toSIunits"] = tableMethod_toSiUnits;
    mTableMethods["inSIunits"] = tableMethod_asSiUnits;

    return mTableMethods;
}


/////////////////////////////////////////////////
/// \brief This function simply returns the
/// vector name obtained from the MAF method
/// string.
///
/// \param sAccessString std::string
/// \return std::string
///
/////////////////////////////////////////////////
static std::string createMafVectorName(std::string sAccessString)
{
    static std::map<std::string, TableMethod> mMethods = getInplaceTableMethods();
    std::string sTableName = sAccessString.substr(0, sAccessString.find("()."));
    std::string sResultVectorName = sAccessString;
    sResultVectorName.replace(sResultVectorName.find("()"), 2, "[");
    sResultVectorName = replaceToVectorname(sResultVectorName);
    sResultVectorName += "]";

    for (auto& method : mMethods)
    {
        if (sAccessString.compare(0, sTableName.length() + method.first.length() + 4, sTableName + "()." + method.first + "(") == 0)
        {
            //std::string sMethodArguments = sAccessString.substr(sAccessString.find("." + method.first + "(") + 1 + method.first.length());
            std::string sMethodArguments = sAccessString.substr(sTableName.length() + method.first.length() + 3);
            sMethodArguments.erase(getMatchingParenthesis(sMethodArguments) + 1);
            sMethodArguments = sMethodArguments.substr(1, sMethodArguments.length() - 2);

            // Might be necessary to resolve the contents of columns and conversions
            getDataElements(sMethodArguments,
                            NumeReKernel::getInstance()->getParser(),
                            NumeReKernel::getInstance()->getMemoryManager());

            return method.second(sTableName, sMethodArguments, sResultVectorName);
        }
    }

    if (sAccessString.find(".name") != std::string::npos)
        return NumeReKernel::getInstance()->getParser().CreateTempVar(mu::Value(sAccessString.substr(0, sAccessString.find("().") + 2)));

    if (sAccessString.find(".description") != std::string::npos)
        return NumeReKernel::getInstance()->getParser().CreateTempVar(mu::Value(NumeReKernel::getInstance()->getMemoryManager().getComment(sAccessString.substr(0, sAccessString.find("().")))));

    if (sAccessString.find(".shrink") != std::string::npos)
    {
        NumeReKernel::getInstance()->getMemoryManager().shrink(sTableName);
        return "true";
    }

    return sResultVectorName;
}


/////////////////////////////////////////////////
/// \brief This function returns the last token
/// of the current string, e.g. a function name,
/// etc.
///
/// \param sLine StringView
/// \return StringView
///
/////////////////////////////////////////////////
static StringView getLastToken(StringView sLine)
{
    size_t pos;

    // Strip the spaces, because the string may end with "... FUNCTION(   "
    sLine.strip();

    if (sLine.length() && sLine.back() == '(')
    {
        // Probably a function name here
        pos = sLine.find_last_of(" !%&|<>=?:.,/*-+^(){}#", sLine.length() - 2);

        if (pos == std::string::npos)
            return sLine;

        // Remove leading characters including the found
        // operator
        sLine.trim_front(pos + 1);
        return sLine;
    }

    // Can be every thing else
    pos = sLine.find_last_of(" !%&|<>=?:.,/*-+^(){}#");

    if (pos == std::string::npos)
        return sLine;

    // Remove leading characters including the found
    // operator
    sLine.trim_front(pos + 1);
    return sLine;
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
        _accessParser.getIndices().col.setRange(0, _accessParser.getIndices().col.front() + nDesiredCols - 1);
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
        _mem->sortElements(0, _mem->getLines() - 1, 0, _mem->getCols() - 1, "-c=1[2:]");

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
    DataAccessParser _accessParser(sExpression, false);

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
/// \param sExpression const std::string&
/// \param sDataTable std::string&
/// \param nColumns int&
/// \param openEnd bool&
/// \param isCluster bool&
/// \return Indices
///
/////////////////////////////////////////////////
Indices getIndicesForPlotAndFit(const std::string& sExpression, std::string& sDataTable, int& nColumns, bool& openEnd, bool& isCluster)
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
/// \param sExpr StringView
/// \return bool
///
/////////////////////////////////////////////////
bool isNotEmptyExpression(StringView sExpr)
{
    if (!sExpr.length())
        return false;

    return sExpr.find_first_not_of(' ') != std::string::npos;
}


/////////////////////////////////////////////////
/// \brief This function checks, whether the
/// passed command line contains the syntax for a
/// cluster candidate, splits up the command line
/// (if selected using doCut) and declares the
/// new cluster, if it is not known up to now.
///
/// \param sLine std::string&
/// \param sCluster std::string&
/// \param doCut bool
/// \return bool
///
/////////////////////////////////////////////////
bool isClusterCandidate(std::string& sLine, std::string& sCluster, bool doCut)
{
    // Do nothing, if the current line does not contain
    // an assignment operator
    if (findAssignmentOperator(sLine) == std::string::npos || sLine[sLine.find_first_not_of(' ')] == '-')
        return false;

    size_t nQuotes = 0;

    // Go through the command line and try to detect
    // a opening brace following an alphanumeric character
    // or an underscore
    for (size_t i = 1; i < sLine.length(); i++)
    {
        // consider quotation marks
        if (sLine[i] == '"' && (!i || sLine[i - 1] != '\\'))
            nQuotes++;

        // Is this an opening brace following the alphanumeric
        // character? Then try to extract the corresponding cluster
        // name
        if (!(nQuotes % 2) && sLine[i] == '{' && (sLine[i - 1] == '_' || isalnum(sLine[i - 1])))
        {
            size_t start = 0;

            // Find the starting position
            for (int j = i - 1; j >= 0; j--)
            {
                if (!isalnum(sLine[j]) && sLine[j] != '_' && sLine[j] != '~')
                    start = j + 1;
            }

            // Extract the cluster including its braces
            sCluster = sLine.substr(start, getMatchingParenthesis(StringView(sLine, i)) + (i - start) + 1);

            // If the command line shall be splitted, do that
            // here
            if (doCut)
            {
                sLine.erase(start, sCluster.length());

                size_t nextCharPos = sLine.find_first_not_of(' ');

                if (nextCharPos != std::string::npos && sLine[nextCharPos] == '=' && sLine[nextCharPos + 1] != '=')
                    sLine.erase(start, nextCharPos + 1);
            }

            StripSpaces(sCluster);

            // Get a reference to the datafile object
            MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

            // Declare the extracted cluster, if it is not
            // known to the clustermanager
            if (!_data.isCluster(StringView(sCluster, 0, sCluster.find('{'))))
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
/// \param sObject const std::string&
/// \param i long long int
/// \param j long long int
/// \param isCluster bool
/// \return mu::Value
///
/////////////////////////////////////////////////
mu::Value getDataFromObject(const std::string& sObject, long long int i, long long int j, bool isCluster)
{
    // Get a reference to the datafile object
    static MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    // Fallback to ensure that valid indices are read
    if (i < 0 || j < 0)
        return NAN;

    // return the data depending on the passed isCluster
    // boolean, the object name and its indices
    return isCluster ? _data.getCluster(sObject).getValue(i) : _data.getElement(i, j, sObject);
}


/////////////////////////////////////////////////
/// \brief This function returns the data from
/// the selected object and switches
/// automatically between tables and clusters.
///
/// \param sObject const std::string&
/// \param vRows const VectorIndex&
/// \param j long long int
/// \param isCluster bool
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array getDataFromObject(const std::string& sObject, const VectorIndex& vRows, long long int j, bool isCluster)
{
    // Get a reference to the datafile object
    static MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    // Fallback to ensure that valid indices are read
    if (vRows.front() == VectorIndex::INVALID || j < 0)
        return mu::Value(NAN);

    // return the data depending on the passed isCluster
    // boolean, the object name and its indices
    if (isCluster)
    {
        NumeRe::Cluster& clst = _data.getCluster(sObject);
        mu::Array vRes;

        for (size_t i = 0; i < vRows.size(); i++)
        {
            vRes.push_back(clst.getValue(vRows[i]));
        }

        return vRes;
    }

    return _data.getElement(vRows, VectorIndex(j), sObject);
}


/////////////////////////////////////////////////
/// \brief This function checks, whether the
/// argument located between sLeft and sRight is
/// part of a multi-argument function.
///
/// \param sLeft StringView
/// \param sRight StringView
/// \return bool
///
/////////////////////////////////////////////////
bool parser_CheckMultArgFunc(StringView sLeft, StringView sRight)
{
    // Get the last token form the left side (can be the name of a function, for example)
    StringView sFunc = getLastToken(sLeft);

    // If it doesn't end with a parenthesis, it cannot be a function
    if (!sFunc.length() || sFunc.back() != '(')
        return false;

    bool bCMP = false;

    // examine the first non-whitespace character of the right side:
    // - if it is a parenthesis, then simply break
    // - if it is a comma, then try to find the next parenthesis. If it exists, it can be a compare-like function signature
    for (size_t i = 0; i < sRight.length(); i++)
    {
        if (sRight[i] != ' ')
        {
            if (sRight[i] == ')')
                break;
            else if (sRight[i] == ',')
            {
                if (sRight.find(')', i + 1) != std::string::npos)
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

