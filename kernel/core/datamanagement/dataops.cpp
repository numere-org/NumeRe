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

#include <wx/image.h>
#include "dataops.hpp"
#include "container.hpp"
#include "../../kernel.hpp"
#include "../ui/language.hpp"
#include "../utils/tools.hpp"
#include "../ui/error.hpp"
#include "../structures.hpp"
#include "../built-in.hpp"
#include "../maths/parser_functions.hpp"
std::string removeQuotationMarks(const std::string&);

using namespace std;

static void evaluateTransposeForDataOperation(const string& sTarget, Indices& _iSourceIndex, Indices& _iTargetIndex, const MemoryManager& _data, bool bTranspose);
static void performDataOperation(const string& sSource, const string& sTarget, const Indices& _iSourceIndex, const Indices& _iTargetIndex, MemoryManager& _data, bool bMove, bool bTranspose);

extern Language _lang;


/////////////////////////////////////////////////
/// \brief This function is a wrapper for the
/// Datafile object. It will simply do the whole
/// UI stuff and let the Datafile object do the
/// hard work.
///
/// \param _data Datafile&
/// \param _option Settings&
/// \param _parser Parser&
/// \param sFileName string
/// \return void
///
/////////////////////////////////////////////////
void load_data(MemoryManager& _data, Settings& _option, Parser& _parser, string sFileName, string sFileFormat)
{
    // check, if the filename is available
    if (!sFileName.length())
    {
        // If not, prompt to the user
        NumeReKernel::print(LineBreak(_lang.get("BUILTIN_LOADDATA_ENTER_NAME", _data.getPath()), _option));
        do
        {
            NumeReKernel::printPreFmt("|\n|<- ");
            NumeReKernel::getline(sFileName);		// gesamte Zeile einlesen: Koennte ja auch eine Leerstelle enthalten sein
            StripSpaces(sFileName);
        }
        while (!sFileName.length());

        // If the user entered a 0 then he wants to abort
        if (sFileName == "0")
        {
            NumeReKernel::print(_lang.get("COMMON_CANCEL"));
            return;
        }

    }

    // No data available in memory?
    if (_data.isEmpty("data"))	// Es sind noch keine Daten vorhanden?
        _data.openFile(sFileName, false, false, 0, "", sFileFormat);
    else	// Sind sie doch? Dann muessen wir uns was ueberlegen...
    {
        // append the data?
        string c = "";
        NumeReKernel::print(LineBreak(_lang.get("BUILTIN_LOADDATA_ASK_APPEND", _data.getDataFileName("data")), _option));
        NumeReKernel::printPreFmt("|\n|<- ");
        NumeReKernel::getline(c);

        // Abort, if user entered a zero
        if (c == "0")
        {
            NumeReKernel::print(_lang.get("COMMON_CANCEL"));
            return;
        }
        else if (c == _lang.YES())		// Anhaengen?
        {
            // append the data -> hand the control over to the corresponding function
            CommandLineParser appCmdParser("append \"" + sFileName + "\"", "append", CommandLineParser::CMD_DAT_PAR);
            append_data(appCmdParser);
        }
        else				// Nein? Dann vielleicht ueberschreiben?
        {
            // overwrite?
            c = "";
            NumeReKernel::print(LineBreak(_lang.get("BUILTIN_LOADDATA_ASK_OVERRIDE"), _option));
            NumeReKernel::printPreFmt("|\n|<- ");
            NumeReKernel::getline(c);

            if (c == _lang.YES())					// Also ueberschreiben
            {
                // Clear memory
                _data.removeData();			// Speicher freigeben...

                // Open the file and copy its contents
                _data.openFile(sFileName, false, false, 0, "", sFileFormat);
            }
            else							// Kannst du dich vielleicht mal entscheiden?
            {
                // User aborts
                NumeReKernel::print(_lang.get("COMMON_CANCEL"));
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief This function handles appending data
/// sets to already existing data.
///
/// \param cmdParser CommandLineParser&
/// \return void
///
/////////////////////////////////////////////////
void append_data(CommandLineParser& cmdParser)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    const Settings& _option = NumeReKernel::getInstance()->getSettings();

    // Copy the default path and the path tokens
    int nArgument = 0;
    std::string sFileList = cmdParser.getExprForFileOperation();
    std::string sFileFormat;

    if (cmdParser.hasParam("fileformat"))
        sFileFormat = cmdParser.getParsedParameterValueAsString("fileformat", "", true, true);

    // If the command expression contains the parameter "all" and the
    // argument (i.e. the filename) contains wildcards
    if (cmdParser.hasParam("all") && (sFileList.find('*') != string::npos || sFileList.find('?') != string::npos))
    {
        // Insert the default loadpath, if no path is passed
        if (sFileList.find('/') == string::npos)
            sFileList = "<loadpath>/" + sFileList;

        // Get the file list, which fulfills the file path scheme
        std::vector<std::string> vFilelist = NumeReKernel::getInstance()->getFileSystem().getFileList(sFileList, FileSystem::FULLPATH);

        // Ensure that at least one file exists
        if (!vFilelist.size())
            throw SyntaxError(SyntaxError::FILE_NOT_EXIST, cmdParser.getCommandLine(), SyntaxError::invalid_position, sFileList);

        // Go through all elements in the vFilelist list
        for (size_t i = 0; i < vFilelist.size(); i++)
        {
            // Load the data. The melting of multiple files
            // is processed automatically
            _data.setbLoadEmptyColsInNextFile(cmdParser.hasParam("keepdim") || cmdParser.hasParam("complete"));
            _data.openFile(vFilelist[i], false, false, 0, "", sFileFormat);
        }

        // Inform the user and return
        if (!_data.isEmpty("data") && _option.systemPrints())
            NumeReKernel::print(LineBreak(_lang.get("BUILTIN_APPENDDATA_ALL_SUCCESS", toString((int)vFilelist.size()), sFileList, toString(_data.getLines("data", false)), toString(_data.getCols("data", false))), _option));

        return;
    }

    NumeRe::FileHeaderInfo info;

    _data.setbLoadEmptyColsInNextFile(cmdParser.hasParam("keepdim") || cmdParser.hasParam("complete"));
    // Simply load the data directly -> Melting is done automatically
    if (cmdParser.hasParam("head") || cmdParser.hasParam("h"))
    {
        if (cmdParser.hasParam("head"))
        {
            auto vPar = cmdParser.getParsedParameterValue("head");

            if (vPar.size())
                nArgument = vPar.getAsScalarInt();
        }
        else
        {
            auto vPar = cmdParser.getParsedParameterValue("h");

            if (vPar.size())
                nArgument = vPar.getAsScalarInt();
        }

        info = _data.openFile(sFileList, false, false, nArgument, "", sFileFormat);
    }
    else
        info = _data.openFile(sFileList, false, false, 0, "", sFileFormat);

    // Inform the user
    if (!_data.isEmpty("data") && _option.systemPrints())
        NumeReKernel::print(LineBreak(_lang.get("BUILTIN_LOADDATA_SUCCESS", info.sFileName, toString(info.nRows), toString(info.nCols)), _option));
}


/////////////////////////////////////////////////
/// \brief This function removes all allocated
/// tables and frees the assigned memory.
///
/// \param _data Datafile&
/// \param _option Settings&
/// \param bIgnore bool
/// \return void
///
/////////////////////////////////////////////////
void clear_cache(MemoryManager& _data, Settings& _option, bool bIgnore)
{
    // Only if there is valid data in the cache
    if (_data.isValid())
    {
        // If the flag "ignore" is not set, ask the user for confirmation
        if (!bIgnore)
        {
            string c = "";

            if (!_data.getSaveStatus())
                NumeReKernel::print(LineBreak(_lang.get("BUILTIN_CLEARCACHE_CONFIRM_NOTSAFED"), _option));
            else
                NumeReKernel::print(LineBreak(_lang.get("BUILTIN_CLEARCACHE_CONFIRM"), _option));

            NumeReKernel::printPreFmt("|\n|<- ");
            NumeReKernel::getline(c);

            if (c != _lang.YES())
            {
                NumeReKernel::print(_lang.get("COMMON_CANCEL"));
                return;
            }
        }

        string sAutoSave = _option.getSavePath() + "/cache.tmp";
        string sCache_file = _option.getExePath() + "/numere.cache";

        // Clear the complete cache and remove the cache files
        _data.removeTablesFromMemory();
        remove(sAutoSave.c_str());
        remove(sCache_file.c_str());

        // Inform the user, if printing is allowed
        if (_option.systemPrints())
            NumeReKernel::print(LineBreak(_lang.get("BUILTIN_CLEARCACHE_SUCCESS"), _option));
    }
    else if (_option.systemPrints())
        NumeReKernel::print(LineBreak(_lang.get("BUILTIN_CLEARCACHE_EMPTY"), _option));
}


/////////////////////////////////////////////////
/// \brief This static function searches for the
/// named table in the cache map, evaluates the
/// specified indices and deletes the
/// corresponding contents from the table.
///
/// \param sCache const string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \return bool
///
/////////////////////////////////////////////////
static bool searchAndDeleteTable(const string& sCache, Parser& _parser, MemoryManager& _data)
{
    for (auto iter = _data.getTableMap().begin(); iter != _data.getTableMap().end(); ++iter)
    {
        if (sCache.substr(0, sCache.find('(')) == iter->first)
        {
            // Cache was found
            // Get the indices from the cache expression
            Indices _iDeleteIndex = getIndices(sCache, _parser, _data, false);

            // Check the indices
            if (!isValidIndexSet(_iDeleteIndex))
                return false;

            // Evaluate the indices
            if (_iDeleteIndex.row.isOpenEnd())
                _iDeleteIndex.row.setRange(0, _data.getLines(iter->first, false) - 1);

            if (_iDeleteIndex.col.isOpenEnd())
                _iDeleteIndex.col.setRange(0, _data.getCols(iter->first) - 1);

            // Delete the section identified by the cache expression
            // The indices are vectors
            _data.deleteBulk(iter->first, _iDeleteIndex.row, _iDeleteIndex.col);

            // If everything is deleted, reset the meta data as well
            if (_data.isEmpty(iter->first))
            {
                NumeRe::TableMetaData meta = _data.getMetaData(iter->first);
                meta.comment.clear();
                meta.source.clear();
                meta.modify();
                _data.setMetaData(iter->first, meta);
            }

            // Return true
            return true;
        }
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This static function searches for the
/// named cluster in the cluster map, evaluates
/// the specified indices and deletes the
/// corresponding contents from the cluster.
///
/// \param sCluster const string&
/// \param _parser Parser&
/// \param _data Datafile&
/// \return bool
///
/////////////////////////////////////////////////
static bool searchAndDeleteCluster(const string& sCluster, Parser& _parser, MemoryManager& _data)
{
    for (auto iter = _data.getClusterMap().begin(); iter != _data.getClusterMap().end(); ++iter)
    {
        if (sCluster.substr(0, sCluster.find('{')) == iter->first)
        {
            // Cache was found
            // Get the indices from the cache expression
            Indices _iDeleteIndex = getIndices(sCluster, _parser, _data, false);

            // Check the indices
            if (!isValidIndexSet(_iDeleteIndex))
                return false;

            // Evaluate the indices
            if (_iDeleteIndex.row.isOpenEnd())
                _iDeleteIndex.row.setRange(0, _data.getCluster(iter->first).size() - 1);

            // Delete the section identified by the cache expression
            // The indices are vectors
            _data.getCluster(iter->first).deleteItems(_iDeleteIndex.row);

            // Return true
            return true;
        }
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This function removes one or multiple
/// entries in the selected table or cluster.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool deleteCacheEntry(CommandLineParser& cmdParser)
{
    Parser& _parser = NumeReKernel::getInstance()->getParser();
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
    bool bSuccess = false;

    // Remove the command from the command line
    std::string sExpr = cmdParser.getExpr();

    // As long as a next argument may be extracted
    while (sExpr.length())
    {
        // Get the next argument
        string sCache = getNextArgument(sExpr, true);

        // Try to find the current cache in the list of available caches
        StripSpaces(sCache);

        // Is it a normal table?
        if (!_data.isCluster(sCache) && searchAndDeleteTable(sCache, _parser, _data))
            bSuccess = true;

        // Is it a cluster?
        if (_data.isCluster(sCache) && searchAndDeleteCluster(sCache, _parser, _data))
            bSuccess = true;
    }

    // return the value of the boolean flag
    return bSuccess;
}


/////////////////////////////////////////////////
/// \brief This function copies whole chunks of
/// data between tables.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool CopyData(CommandLineParser& cmdParser)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    bool bTranspose = cmdParser.hasParam("transpose");
    Indices _iTargetIndex;

    // Get the target from the option or use the default one
    std::string sTarget = cmdParser.getTargetTable(_iTargetIndex, "table");

    // Get the actual source data name and the corresponding indices
    DataAccessParser accessParser = cmdParser.getExprAsDataObject();
    accessParser.evalIndices();

    if (!accessParser.getDataObject().length())
        return false;

    // Apply the transpose flag on the indices, if necessary
    evaluateTransposeForDataOperation(sTarget, accessParser.getIndices(), _iTargetIndex, _data, bTranspose);

    // Perform the actual data operation. Move is set to false
    performDataOperation(accessParser.getDataObject(), sTarget, accessParser.getIndices(), _iTargetIndex, _data, false, bTranspose);

    return true;
}


/////////////////////////////////////////////////
/// \brief This function will move the selected
/// part of a data table to a new location.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool moveData(CommandLineParser& cmdParser)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    bool bTranspose = cmdParser.hasParam("transpose");
    Indices _iTargetIndex;

    // Get the target expression from the option. The default one is empty and will raise an error
    std::string sTarget = cmdParser.getTargetTable(_iTargetIndex, "");

    // If the target cache name is empty, raise an error
    if (!sTarget.length())
        return false;

    // Get the actual source data name and the corresponding indices
    DataAccessParser accessParser = cmdParser.getExprAsDataObject();
    accessParser.evalIndices();

    if (!accessParser.getDataObject().length())
        return false;

    // Apply the transpose flag on the indices, if necessary
    evaluateTransposeForDataOperation(sTarget, accessParser.getIndices(), _iTargetIndex, _data, bTranspose);

    // Perform the actual data operation. Move is set to true
    performDataOperation(accessParser.getDataObject(), sTarget, accessParser.getIndices(), _iTargetIndex, _data, true, bTranspose);

    return true;
}


/////////////////////////////////////////////////
/// \brief This function evaluates the transpose
/// flag and switches the indices correspondingly.
///
/// \param sTarget const string&
/// \param _iSourceIndex Indices&
/// \param _iTargetIndex Indices&
/// \param _data const Datafile&
/// \param bTranspose bool
/// \return void
///
/////////////////////////////////////////////////
static void evaluateTransposeForDataOperation(const string& sTarget, Indices& _iSourceIndex, Indices& _iTargetIndex, const MemoryManager& _data, bool bTranspose)
{
    if (!isValidIndexSet(_iTargetIndex))
    {
        // This section is for cases, in which the target was not defined
        // Get the dimensions of the target to calculate the indices correspondingly
        // Depending on the transpose flag, the rows and columns are exchanged
        if (!bTranspose)
        {
            _iTargetIndex.row = VectorIndex(0LL, _iSourceIndex.row.size() - 1);
            _iTargetIndex.col = VectorIndex(_data.getCols(sTarget, false), _data.getCols(sTarget, false) + _iSourceIndex.col.size() - 1);
        }
        else
        {
            _iTargetIndex.row = VectorIndex(0LL, _iSourceIndex.col.size() - 1);
            _iTargetIndex.col = VectorIndex(_data.getCols(sTarget, false), _data.getCols(sTarget, false) + _iSourceIndex.row.size() - 1);
        }
    }
    else if (_iTargetIndex.row.size())
    {
        // This section is for cases, in which the target was defined via vectors
        // Get the dimensions of the target to calculate the indices correspondingly
        // Depending on the transpose flag, the rows and columns are exchanged
        //
        // The vectors are cleared because they will probably contain not reasonable data
        if (!bTranspose)
        {
            if (_iTargetIndex.row.isOpenEnd())
                _iTargetIndex.row = VectorIndex(_iTargetIndex.row.front(), _iTargetIndex.row.front() + _iSourceIndex.row.size() - 1);

            if (_iTargetIndex.col.isOpenEnd())
                _iTargetIndex.col = VectorIndex(_iTargetIndex.col.front(), _iTargetIndex.col.front() + _iSourceIndex.col.size() - 1);
        }
        else
        {
            if (_iTargetIndex.row.isOpenEnd())
                _iTargetIndex.row = VectorIndex(_iTargetIndex.row.front(), _iTargetIndex.row.front() + _iSourceIndex.col.size() - 1);

            if (_iTargetIndex.col.isOpenEnd())
                _iTargetIndex.col = VectorIndex(_iTargetIndex.col.front(), _iTargetIndex.col.front() + _iSourceIndex.row.size() - 1);
        }
    }
}


/////////////////////////////////////////////////
/// \brief This function will perform the actual
/// data operation.
///
/// \param sSource const string&
/// \param sTarget const string&
/// \param _iSourceIndex const Indices&
/// \param _iTargetIndex const Indices&
/// \param _data Datafile&
/// \param bMove bool
/// \param bTranspose bool
/// \return void
///
/////////////////////////////////////////////////
static void performDataOperation(const string& sSource, const string& sTarget, const Indices& _iSourceIndex, const Indices& _iTargetIndex, MemoryManager& _data, bool bMove, bool bTranspose)
{
    // Get the extract as a table
    NumeRe::Table extract = _data.extractTable(sSource, _iSourceIndex.row, _iSourceIndex.col);

    // If we move then we need to delete the old data
    if (bMove)
        _data.deleteBulk(sSource, _iSourceIndex.row, _iSourceIndex.col);

    // Insert the copied table at the new location
    _data.insertCopiedTable(extract, sTarget, _iTargetIndex.row, _iTargetIndex.col, bTranspose);
}


/////////////////////////////////////////////////
/// \brief This static function sorts clusters
/// and is called by sortData, if the selected
/// data object equals a cluster identifier.
///
/// \param cmdParser CommandLineParser&
/// \param sCluster const string&
/// \param _idx Indices&
/// \return bool
///
/////////////////////////////////////////////////
static bool sortClusters(CommandLineParser& cmdParser, const string& sCluster, Indices& _idx)
{
    vector<int> vSortIndex;
    NumeRe::Cluster& cluster = NumeReKernel::getInstance()->getMemoryManager().getCluster(sCluster);

    // Evalulate special index values
    if (_idx.row.isOpenEnd())
        _idx.row.setRange(0, cluster.size() - 1);

    // Perform the actual sorting operation
    // The member function will be able to handle the remaining command line parameters by itself
    vSortIndex = cluster.sortElements(_idx.row, cmdParser.getParameterList());

    // If the sorting index contains elements, the user had requested them
    if (vSortIndex.size())
    {
        // Transform the integer indices into doubles
        mu::Array vDoubleSortIndex;

        for (size_t i = 0; i < vSortIndex.size(); i++)
            vDoubleSortIndex.push_back(vSortIndex[i]);

        // Set the vector name and set the vector for the parser
        cmdParser.setReturnValue(vDoubleSortIndex);
    }
    else
        cmdParser.clearReturnValue(); // simply clear, if the user didn't request a sorting index

    // Return true
    return true;
}


/////////////////////////////////////////////////
/// \brief This function is a wrapper for the
/// corresponding member function of the Datafile
/// object.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool sortData(CommandLineParser& cmdParser)
{
    vector<int> vSortIndex;
    DataAccessParser _accessParser = cmdParser.getExprAsDataObject();

    if (!_accessParser.getDataObject().length())
        throw SyntaxError(SyntaxError::TABLE_DOESNT_EXIST, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Get the indices
    Indices& _idx = _accessParser.getIndices();

    // Ensure that the indices are reasonable
    if (!isValidIndexSet(_idx))
        throw SyntaxError(SyntaxError::INVALID_INDEX, cmdParser.getCommandLine(), "", _idx.row.to_string() + ", " + _idx.col.to_string());

    // If the current cache equals a cluster, leave the function at
    // this point and redirect the control to the cluster sorting
    // function
    if (_accessParser.isCluster())
        return sortClusters(cmdParser, _accessParser.getDataObject(), _idx);

    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    // Evalulate special index values
    _accessParser.evalIndices();

    // Perform the actual sorting operation
    // The member function will be able to handle the remaining command line parameters by itself
    vSortIndex = _data.sortElements(_accessParser.getDataObject(),
                                    _idx.row, _idx.col,
                                    cmdParser.getParameterList());

    // If the sorting index contains elements, the user had requested them
    if (vSortIndex.size())
    {
        // Transform the integer indices into doubles
        mu::Array vDoubleSortIndex;

        for (size_t i = 0; i < vSortIndex.size(); i++)
            vDoubleSortIndex.push_back(vSortIndex[i]);

        // Set the vector name and set the vector for the parser
        cmdParser.setReturnValue(vDoubleSortIndex);
    }
    else
        cmdParser.clearReturnValue(); // simply clear, if the user didn't request a sorting index

    // Return true
    return true;
}


/////////////////////////////////////////////////
/// \brief This function writes the string
/// contents in the command to a file.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool writeToFile(CommandLineParser& cmdParser)
{
    fstream fFile;
    string sFileName;

    bool bAppend = false;
    bool bTrunc = true;
    bool bKeepEmptyLines = cmdParser.hasParam("keepdim") || cmdParser.hasParam("k");
    FileSystem _fSys;
    _fSys.initializeFromKernel();

    // Try to find the parameter string
    if (cmdParser.getParameterList().length())
    {
        // Get the file name
        if (cmdParser.hasParam("file"))
        {
            sFileName = cmdParser.getFileParameterValueForSaving(".txt", "<savepath>", "");

            // Scripts, procedures and data files may not be written directly
            // this avoids reloads during the execution and other unexpected
            // behavior
            if (sFileName.ends_with(".nprc")
                    || sFileName.ends_with(".nscr")
                    || sFileName.ends_with(".ndat"))
            {
                string sErrorToken;

                if (sFileName.ends_with(".nprc"))
                    sErrorToken = "NumeRe-Prozedur";
                else if (sFileName.ends_with(".nscr"))
                    sErrorToken = "NumeRe-Script";
                else if (sFileName.ends_with(".ndat"))
                    sErrorToken = "NumeRe-Datenfile";

                throw SyntaxError(SyntaxError::FILETYPE_MAY_NOT_BE_WRITTEN, cmdParser.getCommandLine(), SyntaxError::invalid_position, sErrorToken);
            }
        }

        // Get the file open mode
        std::string sMode = cmdParser.getParameterValue("mode");

        if (sMode.length())
        {
            if (sMode == "append" || sMode == "app")
                bAppend = true;
            else if (sMode == "trunc")
                bTrunc = true;
            else if (sMode == "override" || sMode == "overwrite")
            {
                bAppend = false;
                bTrunc = false;
            }
            else
                return false;
        }
    }

    // Ensure that a filename is available
    if (!sFileName.length())
        throw SyntaxError(SyntaxError::NO_FILENAME, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Parse the expression
    std::vector<mu::Array> contents = cmdParser.parseExpr();

    if (!contents.size() || !contents.front().size())
        throw SyntaxError(SyntaxError::NO_STRING_FOR_WRITING, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Open the file in the selected mode
    if (bAppend)
        fFile.open(sFileName.c_str(), ios_base::app | ios_base::out | ios_base::ate);
    else if (bTrunc)
        fFile.open(sFileName.c_str(), ios_base::trunc | ios_base::out);
    else
    {
        if (!fileExists(sFileName))
            ofstream fTemp(sFileName.c_str());

        fFile.open(sFileName.c_str());
    }

    // Ensure that the file is read- and writable
    if (fFile.fail())
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, cmdParser.getCommandLine(), SyntaxError::invalid_position, sFileName);

    // Write the expression linewise
    // Add linebreaks after each subexpression
    for (size_t i = 0; i < contents.size(); i++)
    {
        for (size_t j = 0; j < contents[i].size(); j++)
        {
            std::string sLine = contents[i][j].printVal();

            if (!sLine.length() && !bKeepEmptyLines)
                continue;

            fFile << sLine + "\n";
        }
    }

    fFile << std::flush;

    // close the file stream if it is open and return
    if (fFile.is_open())
        fFile.close();

    return true;
}


/////////////////////////////////////////////////
/// \brief This function reads the content of a
/// file as strings and copies them to a
/// temporary string vector variable.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool readFromFile(CommandLineParser& cmdParser)
{
    std::string sInput = "";
    std::string sCommentEscapeSequence = cmdParser.getParsedParameterValueAsString("comments", "");
    // Kind of a hack
    mu::Array comments;

    if (sCommentEscapeSequence.length())
        comments = NumeReKernel::getInstance()->getParser().Eval();

    std::string sStringSequence = cmdParser.getParsedParameterValueAsString("qmarks", "");

    bool bKeepEmptyLines = cmdParser.hasParam("keepdim") || cmdParser.hasParam("k");

    if (sCommentEscapeSequence != " ")
        StripSpaces(sCommentEscapeSequence);

    StripSpaces(sStringSequence);

    // Get the source file name from the command string or the parameter list
    std::string sFileName = cmdParser.getExprAsFileName(".txt");

    // Ensure that a filename is present
    if (!sFileName.length())
        throw SyntaxError(SyntaxError::NO_FILENAME, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Open the file and ensure that it is readable
    StyledTextFile fFile(sFileName);

    if (!fFile.getLinesCount())
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, cmdParser.getCommandLine(), SyntaxError::invalid_position, sFileName);

    if (sCommentEscapeSequence.length())
    {
        replaceAll(sCommentEscapeSequence, "\\t", "\t");
        replaceAll(sStringSequence, "\\\"", "\"");

        //EndlessVector<std::string> args = getAllArguments(sCommentEscapeSequence);
        EndlessVector<std::string> args;
        std::vector<std::string> strArr = comments.as_str_vector();
        args.assign(strArr.begin(), strArr.end());
        fFile.reStyle(args[0],
                      args[0],
                      args[1],
                      args[1],
                      args[2],
                      sStringSequence,
                      sStringSequence.length() != 0);
    }

    // create a new vector for the file's contents
    std::vector<std::string> vFileContents;

    // Read the complete file, where each line is a separate string expression
    for (int i = 0; i < fFile.getLinesCount(); i++)
    {
        std::string sLine = sCommentEscapeSequence.length() ? fFile.getStrippedLine(i) : fFile.getLine(i);

        // Omit empty lines
        if (!sLine.length() || sLine == "\"\"" || sLine == "\"")
        {
            if (bKeepEmptyLines && i + 1 < fFile.getLinesCount())
                vFileContents.push_back("");

            continue;
        }

        // Add the missing quotation marks
        //if (sLine.front() != '"')
        //	sLine = '"' + sLine;
        //
        //if (sLine.back() != '"')
        //	sLine += '"';

        // Append the parsed string to the vector
        vFileContents.push_back(sLine);
    }

    // Create a new temporary variable, if we actually
    // read something from the file
    if (vFileContents.size())
        cmdParser.setReturnValue(vFileContents);
    else
        cmdParser.setReturnValue("\"\"");

    // return true
    return true;
}


/////////////////////////////////////////////////
/// \brief This function reads image data from an
/// image file and stores it as a cache table.
///
/// \param CommandLineParser& cmdParser
/// \return bool
///
/////////////////////////////////////////////////
bool readImage(CommandLineParser& cmdParser)
{
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    std::string sChannels = "grey";
    Indices _idx;
    mu::Array vIndices;

    // Get the target cache from the command line or use the default one
    std::string sTargetCache = cmdParser.getTargetTable(_idx, "image");

    // Get the file name from the command line or the parameter list
    std::string sFileName = cmdParser.getExprAsFileName(".bmp");

    if (cmdParser.hasParam("channels"))
        sChannels = cmdParser.getParsedParameterValueAsString("channels", "grey");

    // Ensure that a filename is present
    if (!sFileName.length())
        throw SyntaxError(SyntaxError::NO_FILENAME, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Initialize all wxWidgets image handlers (should already be available, though)
    wxInitAllImageHandlers();

    // Create the image object
    wxImage image;

    g_logger.info("Loading image file '" + sFileName + "'.");

    // Load the file to the image object (wxWidgets will try to autodetect the image file format)
    // and throw an error, if it doesn't succeed
    if (!image.LoadFile(sFileName, wxBITMAP_TYPE_ANY))
    {
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sFileName);
    }

    // Get the dimensions of the data and the pointer to the data itself
    int nWidth = image.GetWidth();
    int nHeight = image.GetHeight();
    unsigned char* imageData = image.GetData();

    // Evaluate the indices correspondingly
    if (_idx.row.isOpenEnd())
        _idx.row.setRange(0, _idx.row.front() + std::max(nWidth, nHeight) - 1);

    if (_idx.col.isOpenEnd() && sChannels == "grey")
        _idx.col.setRange(0, _idx.col.front() + 2 + nHeight - 1);
    else if (_idx.col.isOpenEnd())
        _idx.col.setRange(0, _idx.col.front() + 2 + sChannels.length()*nHeight - 1);

    vIndices.push_back(_idx.row.min() + 1);
    vIndices.push_back(_idx.row.max() + 1);
    vIndices.push_back(_idx.col.min() + 1);
    vIndices.push_back(_idx.col.max() + 1);

    int rowmax = _idx.row.subidx(0, nWidth).max();
    Memory* _table = _data.getTable(sTargetCache);

    // Write the axes to the target cache
    for (uint32_t i = 0; i < nWidth; i++)
    {
        if (_idx.row[i] == VectorIndex::INVALID)
            break;

        _table->writeData(_idx.row[i], _idx.col.front(), i + 1);
    }

    for (uint32_t i = 0; i < nHeight; i++)
    {
        if (_idx.row[i] == VectorIndex::INVALID)
            break;

        _table->writeData(_idx.row[i], _idx.col[1], i + 1);

        if (sChannels == "grey")
            _table->writeData(rowmax, _idx.col[2 + i], (uint8_t)0);
        else
        {
            for (size_t n = 0; n < sChannels.length(); n++)
                _table->writeData(rowmax, _idx.col[2 + i + n * nHeight], (uint8_t)0);
        }
    }

    _table->convertColumns(_idx.col.subidx(0, 2), "value.ui32");
    _table->convertColumns(_idx.col.subidx(2, sChannels == "grey" ? nHeight : sChannels.length()*nHeight), "value.ui8");

    // Write headlines
    _data.setHeadLineElement(_idx.col[0], sTargetCache, "x");
    _data.setHeadLineElement(_idx.col[1], sTargetCache, "y");

    g_logger.debug("Writing image data to table.");

    // Copy the average of the RGB channels (grey scale) to the data object
    #pragma omp parallel for
    for (int j = 0; j < nHeight; j++)
    {
        int iData = 0;

        if (_idx.col[j + 2] == VectorIndex::INVALID)
            continue;

        if (sChannels == "grey")
            _table->setHeadLineElement(_idx.col[2 + j], "z(x(:),y(" + toString(j + 1) + "))");
        else
        {
            for (size_t n = 0; n < sChannels.length(); n++)
                _table->setHeadLineElement(_idx.col[2 + j + n * nHeight], "z(x(:),y(" + toString(j) + "))_" + sChannels[n]);
        }

        for (int i = 0; i < nWidth; i++)
        {
            if (_idx.row[i] == VectorIndex::INVALID)
                break;

            // The actual copy process
            if (sChannels == "grey")
            {
                // Calculate the luminosity of the three channels and write it to the table
                _table->writeDataDirectUnsafe(_idx.row[i],
                                              _idx.col[2 + (nHeight - j - 1)],
                                              imageData[j * 3 * nWidth + iData] * 0.299
                                              + imageData[j * 3 * nWidth + iData + 1] * 0.587
                                              + imageData[j * 3 * nWidth + iData + 2] * 0.114);
            }
            else
            {
                for (size_t n = 0; n < sChannels.length(); n++)
                {
                    // Store the selected channel
                    switch (sChannels[n])
                    {
                        case 'r':
                            _table->writeDataDirectUnsafe(_idx.row[i],
                                                          _idx.col[2 + (nHeight - j - 1) + n * nHeight],
                                                          imageData[j * 3 * nWidth + iData]);
                            break;
                        case 'g':
                            _table->writeDataDirectUnsafe(_idx.row[i],
                                                          _idx.col[2 + (nHeight - j - 1) + n * nHeight],
                                                          imageData[j * 3 * nWidth + iData + 1]);
                            break;
                        case 'b':
                            _table->writeDataDirectUnsafe(_idx.row[i],
                                                          _idx.col[2 + (nHeight - j - 1) + n * nHeight],
                                                          imageData[j * 3 * nWidth + iData + 2]);
                            break;
                    }
                }
            }

            // Advance the iterator three channels
            iData += 3;
        }
    }

    _table->markModified();
    g_logger.debug("Image file loaded.");

    cmdParser.setReturnValue(vIndices);

    // return true
    return true;
}

