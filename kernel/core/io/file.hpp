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

#ifndef NUMERE_FILE_HPP
#define NUMERE_FILE_HPP

#include <string>
#include <boost/nowide/convert.hpp>
#include <fstream>
#include <cmath>
#include <vector>
#include <utility>

#include "zip++.hpp"
#include "../utils/stringtools.hpp"
#include "../ui/error.hpp"
#include "../datamanagement/tablecolumn.hpp"
#include "filesystem.hpp"

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief This structure wraps all necessary
    /// meta information of a single file.
    /////////////////////////////////////////////////
    struct FileHeaderInfo
    {
        std::string sFileExtension;
        std::string sFileName;
        std::string sTableName;
        std::string sComment;
        int64_t nRows;
        int64_t nCols;
        int32_t versionMajor;
        int32_t versionMinor;
        int32_t versionBuild;
        float fileVersion;
        __time64_t timeStamp;
        bool needsConversion;

        FileHeaderInfo() : sFileExtension(), sFileName(), sTableName(), sComment(), nRows(0), nCols(0), versionMajor(-1), versionMinor(-1), versionBuild(-1), timeStamp(0), needsConversion(true) {}
    };


    /////////////////////////////////////////////////
    /// \brief Template class representing a generic
    /// file. This class may be specified for the
    /// main data type contained in the read or
    /// written table. All other file classes are
    /// derived from this class. This class cannot be
    /// instantiated directly, because the read and
    /// write methods are declared as pure virtual.
    /////////////////////////////////////////////////
    class GenericFile : public FileSystem
    {
        protected:
            std::fstream fFileStream;
            std::string sFileExtension;
            std::string sFileName;
            std::string sTableName;
            std::string sComment;
            int64_t nRows;
            int64_t nCols;
            unsigned short nPrecFields;
            bool needsConversion;

            // Flag for using external data, i.e. data, which won't be deleted
            // by this class upon calling "clearStorage()" or the destructor
            bool useExternalData;
            std::ios::openmode openMode;

            // The main data table
            TableColumnArray* fileData;

            /////////////////////////////////////////////////
            /// \brief This method has to be used to open the
            /// target file in stream mode. If the file cannot
            /// be opened, this method throws an error.
            ///
            /// \param mode std::ios::openmode
            /// \return void
            ///
            /////////////////////////////////////////////////
            void open(std::ios::openmode mode)
            {
                if (fFileStream.is_open())
                    fFileStream.close();

                fFileStream.open(boost::nowide::widen(sFileName).c_str(), mode);

                if (!fFileStream.good())
                    throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

                openMode = mode;
            }

            /////////////////////////////////////////////////
            /// \brief This method strips trailing spaces
            /// from the passed string.
            ///
            /// \param _sToStrip std::string&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void stripTrailingSpaces(std::string& _sToStrip)
            {
                if (_sToStrip.find_first_not_of(" \t") != std::string::npos)
                    _sToStrip.erase(_sToStrip.find_last_not_of(" \t")+1);
            }

            /////////////////////////////////////////////////
            /// \brief This method simply replaces commas
            /// with dots in the passed string to enable
            /// correct parsing into a double.
            ///
            /// \param _sToReplace std::string&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void replaceDecimalSign(std::string& _sToReplace)
            {
                for (size_t i = 0; i < _sToReplace.length(); i++)
                {
                    if (_sToReplace[i] == ',')
                        _sToReplace[i] = '.';
                }
            }

            /////////////////////////////////////////////////
            /// \brief This method replaces tabulator
            /// characters with whitespaces to simplify the
            /// column determination (the used tokenizer will
            /// only have to consider whitespaces as separator
            /// characters). Sometimes, replacing tabulators
            /// into whitespaces will destroy  column
            /// information. To avoid this, placeholders
            /// (underscores) may be inserted as "empty"
            /// column cells.
            ///
            /// \param _sToReplace std::string&
            /// \param bAddPlaceholders bool
            /// \return void
            ///
            /////////////////////////////////////////////////
            void replaceTabSign(std::string& _sToReplace, bool bAddPlaceholders = false)
            {
                bool bKeepColumns = false;

                // Determine, if columns shall be kept (automatic heuristics)
                if (!bAddPlaceholders && _sToReplace.find(' ') == std::string::npos)
                {
                    bKeepColumns = true;

                    if (_sToReplace[0] == '\t')
                        _sToReplace.insert(0, "---");
                }

                for (size_t i = 0; i < _sToReplace.length(); i++)
                {
                    if (_sToReplace[i] == '\t')
                    {
                        _sToReplace.replace(i, 1, "  ");

                        // Shall "empty cells" be added?
                        if (bAddPlaceholders)
                        {
                            // Insert underscores as empty cells
                            if (!i)
                                _sToReplace.insert(0, 1, '_');
                            else if (_sToReplace[i-1] == ' ')
                                _sToReplace.insert(i, 1, '_');

                            if (i+2 == _sToReplace.length())
                                _sToReplace += "_";
                        }

                        // Shall empty cells be kept (determined by heuristic)
                        if (bKeepColumns && (i+2 == _sToReplace.length() || _sToReplace[i+2] == '\t'))
                            _sToReplace.insert(i + 2, "---");
                    }
                }

                // Transform single whitespaces into special characters,
                // which will hide them from the tokenizer. Do this only,
                // if there are also locations with multiple whitespaces
                if (bAddPlaceholders && _sToReplace.find("  ") != std::string::npos)
                {
                    for (size_t i = 1+(_sToReplace.front() == '#'); i < _sToReplace.length()-1; i++)
                    {
                        if (_sToReplace[i] == ' ' && _sToReplace[i-1] != ' ' && _sToReplace[i+1] != ' ')
                            _sToReplace[i] = '\1';
                    }
                }
            }

            /////////////////////////////////////////////////
            /// \brief This method calculates the extents of
            /// the passed string, if it is used as a table
            /// column headlines. This method will return a
            /// std::pair<> with the maximal number of
            /// characters in a line in the first and the
            /// number of lines in the second component.
            ///
            /// \param sContents const std::string&
            /// \return std::pair<size_t, size_t>
            ///
            /////////////////////////////////////////////////
            std::pair<size_t, size_t> calculateCellExtents(const std::string& sContents, const std::string& sUnit)
            {
                // Prepare the std::pair<> to contain the extents of the cell.
                // (One line is the minimal value)
                std::pair<size_t, size_t> pCellExtents(0u, 1u);
                size_t nLastLineBreak = 0;

                // Search for linebreak characters
                for (size_t i = 0; i < sContents.length(); i++)
                {
                    // Linebreak character found?
                    if (sContents[i] == '\n')
                    {
                        // Increment the number of lines
                        pCellExtents.second++;

                        // Use the maximal number of characters per line
                        // as the first column
                        if (i - nLastLineBreak > pCellExtents.first)
                            pCellExtents.first = i - nLastLineBreak;

                        nLastLineBreak = i;
                    }
                    /*else if (sContents.substr(i, 2) == "\\n")
                    {
                        // Increment the number of lines
                        pCellExtents.second++;

                        // Use the maximal number of characters per line
                        // as the first column
                        if (i - nLastLineBreak > pCellExtents.first)
                            pCellExtents.first = i - nLastLineBreak;

                        nLastLineBreak = i+1;
                    }*/
                }

                if (sUnit.length())
                {
                    pCellExtents.second++;

                    if (pCellExtents.first < sUnit.length()+2)
                        pCellExtents.first = sUnit.length()+2;
                }

                // Examine the last part of the string, which won't be
                // detected by the for loop
                if (sContents.length() - nLastLineBreak > pCellExtents.first)
                    pCellExtents.first = sContents.length() - nLastLineBreak;

                return pCellExtents;
            }

            /////////////////////////////////////////////////
            /// \brief This method gets the selected line
            /// number from the table column headline in the
            /// selected column. If the selected text does not
            /// contain enough lines, a simple whitespaces is
            /// returned.
            ///
            /// \param nCol long longint
            /// \param nLineNumber size_t
            /// \return std::string
            ///
            /////////////////////////////////////////////////
            std::string getLineFromHead(long long int nCol, size_t nLineNumber)
            {
                size_t nLastLineBreak = 0u;

                if (!fileData->at(nCol))
                {
                    if (!nLineNumber)
                        return TableColumn::getDefaultColumnHead(nCol);

                    return " ";
                }

                std::string sHeadLine = fileData->at(nCol)->m_sHeadLine;

                // Search the selected part
                for (size_t i = 0; i < sHeadLine.length(); i++)
                {
                    // Linebreak character found?
                    if (sHeadLine[i] == '\n')
                    {
                        // If this is the correct line number, return
                        // the corresponding substring
                        if (!nLineNumber)
                            return sHeadLine.substr(nLastLineBreak, i - nLastLineBreak);

                        // Decrement the line number and store the
                        // position of the current line break
                        nLineNumber--;
                        nLastLineBreak = i+1;
                    }
                    /*else if (sHeadLine.substr(i, 2) == "\\n")
                    {
                        // If this is the correct line number, return
                        // the corresponding substring
                        if (!nLineNumber)
                            return sHeadLine.substr(nLastLineBreak, i - nLastLineBreak);

                        // Decrement the line number and store the
                        // position of the current line break
                        nLineNumber--;
                        nLastLineBreak = i+2;
                    }*/
                }

                // Catch the last part of the string, which is not found by
                // the for loop
                if (!nLineNumber)
                    return sHeadLine.substr(nLastLineBreak);

                // If we have only one line remaining, we return the unit instead
                // (if any)
                if (nLineNumber == 1 && fileData->at(nCol)->m_sUnit.length())
                    return "[" + fileData->at(nCol)->m_sUnit + "]";

                // Not enough lines in this string, return a whitespace character
                return " ";
            }

            /////////////////////////////////////////////////
            /// \brief This method is a template fo reading
            /// a numeric field of the selected template type
            /// in binary mode.
            ///
            /// \return T
            ///
            /////////////////////////////////////////////////
            template <typename T> T readNumField()
            {
                T num;
                fFileStream.read((char*)&num, sizeof(T));
                return num;
            }

            /////////////////////////////////////////////////
            /// \brief This mehtod can be used to read a
            /// string field from the file in binary mode.
            ///
            /// \return std::string
            ///
            /////////////////////////////////////////////////
            std::string readStringField()
            {
                // Get the length of the field
                uint32_t nLength = readNumField<uint32_t>();

                // If the length is zero, return an empty string
                if (!nLength)
                    return "";

                // Create a character buffer with the corresponding
                // length
                char* buffer = new char[nLength];

                // Read the contents into the buffer
                fFileStream.read(buffer, nLength);

                // Copy the buffer into a string and delete the buffer
                std::string sBuffer(buffer, nLength);
                delete[] buffer;

                return sBuffer;
            }

            /////////////////////////////////////////////////
            /// \brief This method may be used to get the
            /// contents of an embedded file in a zipfile and
            /// return the contents as string.
            ///
            /// \param filename const std::string&
            /// \return std::string
            ///
            /////////////////////////////////////////////////
            std::string getZipFileItem(const std::string& filename)
            {
                // Create a Zipfile class object
                Zipfile* _zip = new Zipfile();

                // Open the file in ZIP mode
                if (!_zip->open(sFileName))
                {
                    _zip->close();
                    delete _zip;
                    throw SyntaxError(SyntaxError::DATAFILE_NOT_EXIST, sFileName, SyntaxError::invalid_position, sFileName);
                }

                // Obtain the contents of the embedded file
                std::string sFileItem = _zip->getZipItem(filename);

                // Close the Zip file and delete the class instance
                _zip->close();
                delete _zip;

                return sFileItem;
            }

            /////////////////////////////////////////////////
            /// \brief This method template is for reading a
            /// block of numeric data into memory in binary
            /// mode.
            ///
            /// \param size int64_t&
            /// \return template <typename T>T*
            ///
            /////////////////////////////////////////////////
            template <typename T> T* readNumBlock(int64_t& size)
            {
                // Get the number of values in the data block
                size = readNumField<int64_t>();

                // If the size is zero, return a null pointer
                if (!size)
                    return nullptr;

                // Create new storage for the data
                T* data = new T[size];

                // Read the contents of the file into the created storager
                fFileStream.read((char*)data, sizeof(T)*size);

                return data;
            }

            /////////////////////////////////////////////////
            /// \brief This method template is for reading a
            /// whole two-dimensional array of data into
            /// memory in binary mode.
            ///
            /// \param rows int64_t&
            /// \param cols lint64_t&
            /// \return template <typenameT>T*
            ///
            /////////////////////////////////////////////////
            template <typename T> T** readDataArray(int64_t& rows, int64_t& cols)
            {
                // Get the dimensions of the data block in memory
                rows = readNumField<int64_t>();
                cols = readNumField<int64_t>();

                // If one of the dimensions is zero, return a null pointer
                if (!rows || !cols)
                    return nullptr;

                // Prepare a new storage object for the contained data
                T** data = new T*[rows];

                // Create the storage for the columns during reading the
                // file and read the contents directly to memory
                for (int64_t i = 0; i < rows; i++)
                {
                    data[i] = new T[cols];
                    fFileStream.read((char*)data[i], sizeof(T)*cols);
                }

                return data;
            }

            /////////////////////////////////////////////////
            /// \brief This method can be used for reading a
            /// block of string data to memory in binary mode.
            ///
            /// \param size int64_t&
            /// \return std::string*
            ///
            /////////////////////////////////////////////////
            std::string* readStringBlock(int64_t& size)
            {
                // Get the number of strings in the current block
                size = readNumField<int64_t>();

                // If no strings are in the file, return a null pointer
                if (!size)
                    return nullptr;

                // Create an array of strings for the data in the file
                std::string* data = new std::string[size];

                // Read each string separately (because their length is
                // most probably independent on each other)
                for (long long int i = 0; i < size; i++)
                {
                    data[i] = readStringField();
                }

                return data;
            }

            /////////////////////////////////////////////////
            /// \brief This method may be used to read the
            /// file in text mode and to obtain the data as
            /// a vector.
            ///
            /// \param stripEmptyLines bool
            /// \return std::vector<std::string>
            ///
            /////////////////////////////////////////////////
            std::vector<std::string> readTextFile(bool stripEmptyLines)
            {
                std::vector<std::string> vTextFile;
                std::string currentLine;

                // Read the whole file linewise
                while (!fFileStream.eof())
                {
                    std::getline(fFileStream, currentLine);

                    // If empty lines shall be stripped, strip
                    // trailing whitespaces first
                    if (stripEmptyLines)
                        stripTrailingSpaces(currentLine);

                    // If empty lines shall be stripped, store
                    // only non-empty lines
                    if (!stripEmptyLines || currentLine.length())
                        vTextFile.push_back(currentLine);
                }

                return vTextFile;
            }

            enum TokenizerOptions
            {
                DEFAULT = 0x0,
                SKIP_EMPTY = 0x1,
                CONSIDER_QMARKS = 0x2
            };

            /////////////////////////////////////////////////
            /// \brief This method may be used to separater a
            /// line into multiple tokens using a set of
            /// separator characters. If empty token shall
            /// be skipped, then only tokens with a non-zero
            /// length are stored.
            ///
            /// \param sString const std::string&
            /// \param sSeparators const std::string&
            /// \param options int
            /// \return std::vector<std::string>
            ///
            /////////////////////////////////////////////////
            std::vector<std::string> tokenize(const std::string& sString, const std::string& sSeparators, int options = DEFAULT)
            {
                std::vector<std::string> vTokens;
                // Initialize index variables
                size_t iStart = 0;

                // Initialize variable for checking if inside quotes
                bool inQuotation = false;
                bool considerQMarks = options & CONSIDER_QMARKS;

                // Iterate over string line
                for (size_t i = 0; i < sString.length(); ++i)
                {
                    char c = sString[i];

                    if (c == '"' && considerQMarks)
                    {
                        // Encountered quote, toggle inQuotation
                        inQuotation = !inQuotation;
                    }
                    else if (sSeparators.find(c) != std::string::npos && (!inQuotation || !considerQMarks))
                    {
                        // Found a separator outside string, add to Tokens vector
                        // Exclude entry and ending string quotes
                        if (considerQMarks && sString[iStart] == '"' && sString[i-1] == '"')
                        {
                            vTokens.push_back(sString.substr(iStart+1, i - iStart - 2));
                            replaceAll(vTokens.back(), "\"\"", "\"");
                        }
                        else
                            vTokens.push_back(sString.substr(iStart, i - iStart));

                        iStart = i + 1; // Update Start index for next token
                    }
                }

                // Add the last token or one token if no separator found
                if (considerQMarks && sString[iStart] == '"' && sString.back() == '"')
                {
                    vTokens.push_back(sString.substr(iStart+1, sString.length() - iStart - 2));
                    replaceAll(vTokens.back(), "\"\"", "\"");
                }
                else
                    vTokens.push_back(sString.substr(iStart));

                // If empty Tokens are not being stored, remove all empty tokens
                // from the vector
                if (options & SKIP_EMPTY)
                {
                    vTokens.erase(std::remove_if(vTokens.begin(), vTokens.end(),
                                                 [](const std::string &token)
                                                 { return token.empty(); }),
                                  vTokens.end());
                }
                return vTokens;
            }
            /////////////////////////////////////////////////
            /// \brief This method template can be used to
            /// write a numeric value to file in binary mode.
            ///
            /// \param num T
            /// \return void
            ///
            /////////////////////////////////////////////////
            template <typename T> void writeNumField(T num)
            {
                fFileStream.write((char*)&num, sizeof(T));
            }

            /////////////////////////////////////////////////
            /// \brief This method may be used to write a
            /// string to file in binary mode.
            ///
            /// \param sString const std::string&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void writeStringField(const std::string& sString)
            {
                // Store the length of string as numeric value first
                writeNumField<uint32_t>((uint32_t)sString.length());
                fFileStream.write(sString.c_str(), sString.length());
            }

            /////////////////////////////////////////////////
            /// \brief This method template may be used to
            /// write a block of data of the selected type to
            /// the file in binary mode.
            ///
            /// \param data T*
            /// \param size int64_t
            /// \return void
            ///
            /////////////////////////////////////////////////
            template <typename T> void writeNumBlock(T* data, int64_t size)
            {
                // Store the length of the data block first
                writeNumField<int64_t>(size);
                fFileStream.write((char*)data, sizeof(T)*size);
            }

            /////////////////////////////////////////////////
            /// \brief This method may be used to write a
            /// two-dimensional array of data to the file in
            /// binary mode.
            ///
            /// \param data T**
            /// \param rows int64_t
            /// \param cols int64_t
            /// \return void
            ///
            /////////////////////////////////////////////////
            template <typename T> void writeDataArray(T** data, int64_t rows, int64_t cols)
            {
                // Store the dimensions of the array first
                writeNumField<int64_t>(rows);
                writeNumField<int64_t>(cols);

                // Write the contents to the file in linewise fashion
                for (int64_t i = 0; i < rows; i++)
                    fFileStream.write((char*)data[i], sizeof(T)*cols);
            }

            /////////////////////////////////////////////////
            /// \brief This method may be used to write a
            /// block of strings into the file in binary mode.
            ///
            /// \param data std::string*
            /// \param size int64_t
            /// \return void
            ///
            /////////////////////////////////////////////////
            void writeStringBlock(std::string* data, int64_t size)
            {
                // Store the number of fields first
                writeNumField<int64_t>(size);

                // Write each field separately, because their length is
                // independent on each other
                for (int64_t i = 0; i < size; i++)
                {
                    writeStringField(data[i]);
                }
            }

            /////////////////////////////////////////////////
            /// \brief This method prepares the internal
            /// storage, so that it may contain the read
            /// data. This method is only used for textual
            /// files.
            ///
            /// \return void
            ///
            /////////////////////////////////////////////////
            void createStorage()
            {
                if (nCols > 0 && !fileData)
                {
                    fileData = new TableColumnArray;
                    fileData->resize(nCols);
                }
                else if (nRows < 0 || nCols < 0)
                    throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);
            }

            /////////////////////////////////////////////////
            /// \brief This method cleares the internal
            /// storage. This method is called by the
            /// destructor automatically.
            ///
            /// \return void
            ///
            /////////////////////////////////////////////////
            void clearStorage()
            {
                // Only delete the storage, if it is internal data. Do
                // not delete the storage, if it is linked from an external
                // source
                if (!useExternalData && fileData)
                {
                    fileData->clear();
                    delete fileData;
                    fileData = nullptr;
                }
                else
                    fileData = nullptr;

                nRows = 0;
                nCols = 0;
            }

            /////////////////////////////////////////////////
            /// \brief This method may be used to copy two-
            /// dimensional arrays of data. Both source and
            /// target arrays have to exist in advance.
            ///
            /// \param from T**
            /// \param to T**
            /// \param rows long long int
            /// \param cols long long int
            /// \return void
            ///
            /////////////////////////////////////////////////
            template <typename T> void copyDataArray(T** from, T** to, long long int rows, long long int cols)
            {
                if (!from || !to || !rows || !cols)
                    return;

                for (long long int i = 0; i < rows; i++)
                {
                    for (long long int j = 0; j < cols; j++)
                        to[i][j] = from[i][j];
                }
            }

            /////////////////////////////////////////////////
            /// \brief This method may be used to copy string
            /// arrays. Both source and target arrays have to
            /// exist in advance.
            ///
            /// \param from std::string*
            /// \param to std::string*
            /// \param nElements long long int
            /// \return void
            ///
            /////////////////////////////////////////////////
            void copyStringArray(std::string* from, std::string* to, long long int nElements)
            {
                if (!from || !to || !nElements)
                    return;

                for (long long int i = 0; i < nElements; i++)
                    to[i] = from[i];
            }

            /////////////////////////////////////////////////
            /// \brief This method template may be used to
            /// copy arrays of data of the selected type.
            /// Both source and target arrays have to exist
            /// in advance.
            ///
            /// \param from T*
            /// \param to T*
            /// \param nElements long long int
            /// \return void
            ///
            /////////////////////////////////////////////////
            template <typename T> void copyArray(T* from, T* to, long long int nElements)
            {
                if (!from || !to || !nElements)
                    return;

                for (long long int i = 0; i < nElements; i++)
                    to[i] = from[i];
            }

            /////////////////////////////////////////////////
            /// \brief This method may be used to determine,
            /// whether a string contains \em only numeric
            /// data.
            ///
            /// \param sString const std::string&
            /// \return bool
            ///
            /////////////////////////////////////////////////
            bool isNumeric(const std::string& sString)
            {
                if (!sString.length())
                    return false;

                // Apply the simplest heuristic: mostly every numerical valid character
                // and ignore characters, which cannot represent a value by themselves
                if (sString.find_first_not_of(" 0123456789.,eianfEIANF+-*/\t") != std::string::npos
                    || (sString.find_first_not_of("+-* .,\teE") == std::string::npos && sString.find("---") == std::string::npos))
                    return false;

                // Eliminate invalid character positions
                if (tolower(sString.front()) == 'e'
                    || tolower(sString.front()) == 'a'
                    || tolower(sString.front()) == 'f'
                    || tolower(sString.back()) == 'e')
                    return false;

                // Regression fix introduced because NA is accepted as NaN
                for (size_t i = 1; i < sString.length()-1; i++)
                {
                    if (sString[i] == '-' || sString[i] == '+')
                    {
                        if (tolower(sString[i-1]) != 'e'
                            && (isdigit(sString[i-1]) && sString.find_first_of("iI", i+1) == std::string::npos)
                            && sString[i-1] != ' ')
                            return false;
                    }
                }

                return true;
            }

            /////////////////////////////////////////////////
            /// \brief This method is used by the assignment
            /// operator and the copy constructor to copy the
            /// contents of the passed GenericFile instance.
            ///
            /// \param file const GenericFile&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void assign(const GenericFile& file)
            {
                clearStorage();

                nRows = file.nRows;
                nCols = file.nCols;
                nPrecFields = file.nPrecFields;
                useExternalData = file.useExternalData;
                sFileName = file.sFileName;
                sFileExtension = file.sFileExtension;
                sTableName = file.sTableName;
                sComment = file.sComment;
                needsConversion = file.needsConversion;

                // Creates only the vector but leaves the
                // actual columns untouched
                createStorage();

                if (file.fileData && fileData)
                {
                    for (long long int col = 0; col < nCols; col++)
                        fileData->at(col).reset(file.fileData->at(col)->copy());
                }
            }

        public:
            // Constructor from filename
            /////////////////////////////////////////////////
            /// \brief Constructor from filename.
            ///
            /// \param fileName const std::string&
            ///
            /////////////////////////////////////////////////
            GenericFile(const std::string& fileName) : FileSystem(), nRows(0), nCols(0), nPrecFields(7), needsConversion(true), useExternalData(false), fileData(nullptr)
            {
                // Initializes the file system from the kernel
                initializeFromKernel();
                sFileName = ValidFileName(fileName, "", false);
                sFileExtension = getFileParts(sFileName).back();
            }

            /////////////////////////////////////////////////
            /// \brief Copy constructor.
            ///
            /// \param file const GenericFile&
            ///
            /////////////////////////////////////////////////
            GenericFile(const GenericFile& file) : GenericFile(file.sFileName)
            {
                assign(file);
            }

            /////////////////////////////////////////////////
            /// \brief Virtual destructor: we'll work with
            /// instances on the heap, therefore we'll need
            /// virtual declared destructors. This destructor
            /// will clear the internal memory and closes the
            /// file stream, if it is still open.
            /////////////////////////////////////////////////
            virtual ~GenericFile()
            {
                clearStorage();

                if (fFileStream.is_open())
                    fFileStream.close();
            }

            /////////////////////////////////////////////////
            /// \brief Wrapper for fstream::is_open()
            ///
            /// \return bool
            ///
            /////////////////////////////////////////////////
            bool is_open()
            {
                return fFileStream.is_open();
            }

            /////////////////////////////////////////////////
            /// \brief Wrapper for fstream::close(). Will
            /// also clear the internal memory.
            ///
            /// \return void
            ///
            /////////////////////////////////////////////////
            void close()
            {
                fFileStream.close();
                clearStorage();
            }

            /////////////////////////////////////////////////
            /// \brief Wrapper for fstream::good()
            ///
            /// \return bool
            ///
            /////////////////////////////////////////////////
            bool good()
            {
                return fFileStream.good();
            }

            /////////////////////////////////////////////////
            /// \brief Wrapper for fstream::tellg()
            ///
            /// \return size_t
            ///
            /////////////////////////////////////////////////
            size_t tellg()
            {
                return fFileStream.tellg();
            }

            /////////////////////////////////////////////////
            /// \brief Wrapper for fstream::tellp()
            ///
            /// \return size_t
            ///
            /////////////////////////////////////////////////
            size_t tellp()
            {
                return fFileStream.tellp();
            }

            /////////////////////////////////////////////////
            /// \brief Wrapper for fstream::seekg() with
            /// start from the beginning of the stream.
            ///
            /// \param pos size_t
            /// \return void
            ///
            /////////////////////////////////////////////////
            void seekg(size_t pos)
            {
                fFileStream.seekg(pos, std::ios::beg);
            }

            /////////////////////////////////////////////////
            /// \brief Wrapper for fstream::seekp() with
            /// start from the beginning of the stream.
            ///
            /// \param pos size_t
            /// \return void
            ///
            /////////////////////////////////////////////////
            void seekp(size_t pos)
            {
                fFileStream.seekp(pos, std::ios::beg);
            }

            /////////////////////////////////////////////////
            /// \brief Returns the file extension.
            ///
            /// \return std::string
            ///
            /////////////////////////////////////////////////
            std::string getExtension()
            {
                return sFileExtension;
            }

            /////////////////////////////////////////////////
            /// \brief Returns the file name.
            ///
            /// \return std::string
            ///
            /////////////////////////////////////////////////
            std::string getFileName()
            {
                return sFileName;
            }

            /////////////////////////////////////////////////
            /// \brief Returns the table name referenced in
            /// the file. Will default to the file name with
            /// non-alnum characters replaced with
            /// underscores, if the file does not reference a
            /// table name by itself.
            ///
            /// \return std::string
            ///
            /////////////////////////////////////////////////
            std::string getTableName()
            {
                // Has the table name not been defined yet
                if (!sTableName.length())
                {
                    // Get the file name (not the whole path)
                    sTableName = getFileParts(sFileName)[2];

                    // Replace all non-alnum characters with underscores
                    for (size_t i = 0; i < sTableName.length(); i++)
                    {
                        if (sTableName[i] != '_' && !isalnum(sTableName[i]))
                            sTableName[i] = '_';
                    }

                    // If the first character is a digit, prepend an
                    // underscore
                    if (isdigit(sTableName.front()))
                        sTableName.insert(0, 1, '_');
                }

                return sTableName;
            }

            /////////////////////////////////////////////////
            /// \brief Returns the comment stored with the
            /// referenced file.
            ///
            /// \return std::string
            ///
            /////////////////////////////////////////////////
            std::string getComment()
            {
                return sComment;
            }

            /////////////////////////////////////////////////
            /// \brief Sets the comment to be written to the
            /// referencedfile.
            ///
            /// \param comment const std::string&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void setComment(const std::string& comment)
            {
                sComment = comment;
            }

            /////////////////////////////////////////////////
            /// \brief Returns the number of rows.
            ///
            /// \return int64_t
            ///
            /////////////////////////////////////////////////
            int64_t getRows()
            {
                return nRows;
            }

            /////////////////////////////////////////////////
            /// \brief Returns the number of columns.
            ///
            /// \return int64_t
            ///
            /////////////////////////////////////////////////
            int64_t getCols()
            {
                return nCols;
            }

            /////////////////////////////////////////////////
            /// \brief Returns the file header information
            /// structure.
            ///
            /// \return FileHeaderInfo
            ///
            /////////////////////////////////////////////////
            virtual FileHeaderInfo getFileHeaderInformation()
            {
                FileHeaderInfo info;

                info.nCols = nCols;
                info.nRows = nRows;
                info.sFileExtension = sFileExtension;
                info.sFileName = sFileName;
                info.sTableName = getTableName();
                info.sComment = sComment;
                info.needsConversion = needsConversion;

                return info;
            }

            /////////////////////////////////////////////////
            /// \brief Pure virtual declaration of the read
            /// access method. Has to be implemented in all
            /// derived classes and can be used to read the
            /// contents of the file to memory.
            ///
            /// \return bool
            ///
            /////////////////////////////////////////////////
            virtual bool read() = 0;

            /////////////////////////////////////////////////
            /// \brief Pure virtual declaration of the write
            /// access method. Has to be implemented in all
            /// derived classes and can be used to write the
            /// contents in memory to the target file.
            ///
            /// \return bool
            ///
            /////////////////////////////////////////////////
            virtual bool write() = 0;

            /////////////////////////////////////////////////
            /// \brief Assignment operator definition.
            ///
            /// \param file const GenericFile&
            /// \return GenericFile&
            ///
            /////////////////////////////////////////////////
            GenericFile& operator=(const GenericFile& file)
            {
                assign(file);
                return *this;
            }

            /////////////////////////////////////////////////
            /// \brief This method copies the internal data
            /// to the passed memory address. The target
            /// memory must already exist.
            ///
            /// \param data TableColumnArray*
            /// \return void
            ///
            /////////////////////////////////////////////////
            void getData(TableColumnArray* data)
            {
                if (data && fileData)
                {
                    for (int64_t col = 0; col < nCols; col++)
                    {
                        if (fileData->at(col))
                            data->at(col).reset(fileData->at(col)->copy());
                    }
                }
            }

            /////////////////////////////////////////////////
            /// \brief This method returns a pointer to the
            /// internal memory with read and write access.
            /// This pointer shall not stored for future use,
            /// because the referenced memory will be deleted
            /// upon destruction of this class instance.
            ///
            /// \param rows int64_t&
            /// \param cols int64_t&
            /// \return TableColumnArray*
            ///
            /////////////////////////////////////////////////
            TableColumnArray* getData(int64_t& rows, int64_t& cols)
            {
                rows = nRows;
                cols = nCols;

                return fileData;
            }

            /////////////////////////////////////////////////
            /// \brief Sets the dimensions of the data table,
            /// which will be used in the future. Clears the
            /// internal memory in advance.
            ///
            /// \param rows int64_t
            /// \param cols int64_t
            /// \return void
            ///
            /////////////////////////////////////////////////
            void setDimensions(int64_t rows, int64_t cols)
            {
                clearStorage();

                nRows = rows;
                nCols = cols;
            }

            /////////////////////////////////////////////////
            /// \brief Set the table's name.
            ///
            /// \param name const std::string&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void setTableName(const std::string& name)
            {
                sTableName = name;
            }

            /////////////////////////////////////////////////
            /// \brief Set the precision, which shall be used
            /// to convert the floating point numbers into
            /// strings.
            ///
            /// \param nPrecision unsigned short
            /// \return void
            ///
            /////////////////////////////////////////////////
            void setTextfilePrecision(unsigned short nPrecision)
            {
                nPrecFields = nPrecision;
            }

            /////////////////////////////////////////////////
            /// \brief This method created the internal
            /// storage and copies the passed data to this
            /// storage.
            ///
            /// \param data TableColumnArray*
            /// \param rows int64_t
            /// \param cols int64_t
            /// \return void
            ///
            /////////////////////////////////////////////////
            void addData(TableColumnArray* data, int64_t rows, int64_t cols)
            {
                if (!nRows && !nCols)
                {
                    nRows = rows;
                    nCols = cols;
                }

                createStorage();

                if (fileData && data)
                {
                    for (int64_t col = 0; col < nCols; col++)
                       fileData->at(col).reset(data->at(col)->copy());
                }
            }

            /////////////////////////////////////////////////
            /// \brief This method refernces the passed
            /// external data internally. The data is not
            /// copied and must exist as long as thos class
            /// exists.
            ///
            /// \param data TableColumnArray*
            /// \param rows int64_t
            /// \param cols int64_t
            /// \return void
            ///
            /////////////////////////////////////////////////
            void setData(TableColumnArray* data, int64_t rows, int64_t cols)
            {
                useExternalData = true;

                fileData = data;
                nRows = rows;
                nCols = cols;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This function determines the correct
    /// class to be used for the filename passed to
    /// this function. If there's no fitting file
    /// type, a null pointer is returned. The calling
    /// function is responsible for clearing the
    /// created instance. The returned pointer is of
    /// the type GenericFile, but actually
    /// references an instance of a derived class.
    ///
    /// \param filename const std::string&
    /// \param sExt std::string
    /// \return GenericFile*
    ///
    /////////////////////////////////////////////////
    GenericFile* getFileByType(const std::string& filename, std::string sExt = "");

    bool canLoadFile(const std::string& filename);


    /////////////////////////////////////////////////
    /// \brief This class is a facet for an arbitrary
    /// GenericFile instance. It can be used to read
    /// the contents of the contained file more
    /// easily.
    /////////////////////////////////////////////////
    class GenericFileView
    {
        private:
            GenericFile* m_file;

        public:
            /////////////////////////////////////////////////
            /// \brief Default constructor.
            /////////////////////////////////////////////////
            GenericFileView() : m_file(nullptr) {}

            /////////////////////////////////////////////////
            /// \brief Constructor from an available
            /// GenericFile instance.
            ///
            /// \param _file GenericFile*
            ///
            /////////////////////////////////////////////////
            GenericFileView(GenericFile* _file) : m_file(_file) {}

            /////////////////////////////////////////////////
            /// \brief Attaches a new GenericFile instance to
            /// this facet class.
            ///
            /// \param _file GenericFile*
            /// \return void
            ///
            /////////////////////////////////////////////////
            void attach(GenericFile* _file)
            {
                m_file = _file;
            }

            /////////////////////////////////////////////////
            /// \brief Returns the internally stored
            /// GenericFile instance pointer.
            ///
            /// \return GenericFile*
            ///
            /////////////////////////////////////////////////
            GenericFile* getPtr()
            {
                return m_file;
            }

            /////////////////////////////////////////////////
            /// \brief Returns the number of columns in the
            /// internally stored GenericFile instance.
            ///
            /// \return int64_t
            ///
            /////////////////////////////////////////////////
            int64_t getCols() const
            {
                if (m_file)
                    return m_file->getCols();

                return 0;
            }

            /////////////////////////////////////////////////
            /// \brief Returns the number of rows in the
            /// internally stored GenericFile instance.
            ///
            /// \return int64_t
            ///
            /////////////////////////////////////////////////
            int64_t getRows() const
            {
                if (m_file)
                    return m_file->getRows();

                return 0;
            }

            /////////////////////////////////////////////////
            /// \brief Returns the value stored at the passed
            /// positions. A default constructed std::complex<double>
            /// object instance is returned, if the element
            /// does not exist.
            ///
            /// \param row int64_t
            /// \param col int64_t
            /// \return std::complex<double>
            ///
            /////////////////////////////////////////////////
            std::complex<double> getElement(int64_t row, int64_t col) const
            {
                if (m_file)
                {
                    if (row >= m_file->getRows() || col >= m_file->getCols())
                        return 0;

                    // dummy variable
                    int64_t r,c;
                    TableColumnArray* arr = m_file->getData(r,c);

                    if (arr->at(col))
                        return arr->at(col)->getValue(row);
                }

                return std::complex<double>();
            }

            /////////////////////////////////////////////////
            /// \brief Returns the value stored at the passed
            /// positions. An empty string is returned, if
            /// the element does not exist.
            ///
            /// \param row int64_t
            /// \param col int64_t
            /// \return std::string
            ///
            /////////////////////////////////////////////////
            std::string getStringElement(int64_t row, int64_t col) const
            {
                if (m_file)
                {
                    if (row >= m_file->getRows() || col >= m_file->getCols())
                        return 0;

                    // dummy variable
                    int64_t r,c;
                    TableColumnArray* arr = m_file->getData(r,c);

                    if (arr->at(col))
                        return arr->at(col)->getValueAsInternalString(row);
                }

                return "";
            }

            /////////////////////////////////////////////////
            /// \brief Returns the column heading stored for
            /// the passed column. Returns an empty string,
            /// if the column does not exist.
            ///
            /// \param col int64_t
            /// \return string
            ///
            /////////////////////////////////////////////////
            std::string getColumnHead(int64_t col) const
            {
                if (m_file)
                {
                    if (col >= m_file->getCols())
                        return "";

                    // dummy variable
                    int64_t r,c;
                    TableColumnArray* arr = m_file->getData(r,c);

                    if (arr->at(col))
                        return arr->at(col)->m_sHeadLine;
                }

                return "";
            }
    };


    typedef GenericFileView FileView;


    /////////////////////////////////////////////////
    /// \brief This class resembles an arbitrary text
    /// data file, which is formatted in a table-like
    /// manner. The columns may be separated using
    /// tabulators and/or whitespace characters.
    /////////////////////////////////////////////////
    class TextDataFile : public GenericFile
    {
        private:
            void readFile();
            void writeFile();
            void writeHeader();
            void writeTableHeads(const std::vector<size_t>& vColumnWidth, size_t nNumberOfLines);
            void writeTableContents(const std::vector<size_t>& vColumnWidth);
            void addSeparator(const std::vector<size_t>& vColumnWidth);

            void decodeTableHeads(std::vector<std::string>& vFileContents, long long int nComment);

        protected:
            std::vector<size_t> calculateColumnWidths(size_t& nNumberOfLines);

        public:
            TextDataFile(const std::string& filename);
            virtual ~TextDataFile();

            virtual bool read() override
            {
                readFile();
                return true;
            }

            virtual bool write() override
            {
                writeFile();
                return true;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class represents a Markdown file,
    /// which is especially supported as exporting
    /// target.
    /////////////////////////////////////////////////
    class MarkDownFile : public TextDataFile
    {
        private:
            void writeFile();
            void writeTableHeads(const std::vector<size_t>& vColumnWidth, size_t nNumberOfLines);
            void writeTableContents(const std::vector<size_t>& vColumnWidth);
            void addSeparator(const std::vector<size_t>& vColumnWidth);

        public:
            MarkDownFile(const std::string& filename);
            virtual ~MarkDownFile();

            virtual bool read() override
            {
                // not possible right now
                return false;
            }

            virtual bool write() override
            {
                writeFile();
                return true;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class represents a HTML file,
    /// which is especially supported as exporting
    /// target.
    /////////////////////////////////////////////////
    class HtmlFile : public GenericFile
    {
        private:
            void writeFile();

        public:
            HtmlFile(const std::string& filename);
            virtual ~HtmlFile();

            virtual bool read() override
            {
                // not possible right now
                return false;
            }

            virtual bool write() override
            {
                writeFile();
                return true;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class resembles the binary NumeRe
    /// data file format. The data is red and written
    /// in binary mode using the methods from
    /// GenericFile.
    /////////////////////////////////////////////////
    class NumeReDataFile : public GenericFile
    {
        protected:
            bool isLegacy;
            __time64_t timeStamp;
            int32_t versionMajor;
            int32_t versionMinor;
            int32_t versionBuild;
            const short fileSpecVersionMajor = 4;
            const short fileSpecVersionMinor = 10;
            float fileVersionRead;
            size_t checkPos;
            size_t checkStart;

            void writeHeader();
            void writeDummyHeader();
            void writeFile();
            void writeColumn(const TblColPtr& col);
            void readHeader(bool performShaCheck = true);
            void skipDummyHeader();
            void readFile();
            void readColumn(TblColPtr& col);
            void readColumnV4(TblColPtr& col);
            void readLegacyFormat();
            void* readGenericField(std::string& type, int64_t& size);
            void deleteGenericData(void* data, const std::string& type);

        public:
            NumeReDataFile(const std::string& filename);
            NumeReDataFile(const NumeReDataFile& file);
            virtual ~NumeReDataFile();

            virtual bool read() override
            {
                readFile();
                return true;
            }

            virtual bool write() override
            {
                writeFile();
                return true;
            }

            NumeReDataFile& operator=(NumeReDataFile& file);

            /////////////////////////////////////////////////
            /// \brief Reads only the header of the
            /// referenced file.
            ///
            /// \return void
            ///
            /////////////////////////////////////////////////
            void readFileInformation()
            {
                open(std::ios::in | std::ios::binary);
                readHeader(false);
            }

            /////////////////////////////////////////////////
            /// \brief Returns the file timestamp.
            ///
            /// \return __time64_t
            ///
            /////////////////////////////////////////////////
            __time64_t getTimeStamp()
            {
                return timeStamp;
            }

            virtual FileHeaderInfo getFileHeaderInformation() override
            {
                FileHeaderInfo info;

                info.nCols = nCols;
                info.nRows = nRows;
                info.sFileExtension = sFileExtension;
                info.sFileName = sFileName;
                info.sTableName = getTableName();
                info.sComment = sComment;
                info.versionMajor = versionMajor;
                info.versionMinor = versionMinor;
                info.versionBuild = versionBuild;
                info.fileVersion = fileVersionRead;
                info.timeStamp = timeStamp;
                info.needsConversion = needsConversion;

                return info;
            }

            std::string getVersionString();
    };


    /////////////////////////////////////////////////
    /// \brief This class resembles the cache file
    /// used to autosave and recover the tables in
    /// memory. It is derived from the NumeRe data
    /// file format and uses its functionalities to
    /// layout the data in the file: the cache file
    /// starts with a header containing the number of
    /// tables in the file and the character
    /// positions in the file, where each table
    /// starts. The tables themselves are written in
    /// the NumeRe data file format.
    /////////////////////////////////////////////////
    class CacheFile : public NumeReDataFile
    {
        private:
            std::vector<uint32_t> vFileIndex;
            size_t nIndexPos;

            void reset();
            void readSome();
            void writeSome();


        public:
            CacheFile(const std::string& filename);
            virtual ~CacheFile();

            virtual bool read() override
            {
                readSome();
                return true;
            }

            virtual bool write() override
            {
                writeSome();
                return true;
            }

            void readCacheHeader();
            void writeCacheHeader();

            /////////////////////////////////////////////////
            /// \brief Returns the number of tables stored in
            /// the referenced cache file.
            ///
            /// \return size_t
            ///
            /////////////////////////////////////////////////
            size_t getNumberOfTables()
            {
                return vFileIndex.size();
            }

            /////////////////////////////////////////////////
            /// \brief Sets the number of tables to be stored
            /// in the referenced cache file.
            ///
            /// \param nTables size_t
            /// \return void
            ///
            /////////////////////////////////////////////////
            void setNumberOfTables(size_t nTables)
            {
                vFileIndex = std::vector<uint32_t>(nTables, 0u);
            }

            /////////////////////////////////////////////////
            /// \brief Returns the character position of the
            /// passed table index.
            ///
            /// \param nthTable size_t
            /// \return uint32_t
            ///
            /////////////////////////////////////////////////
            uint32_t getPosition(size_t nthTable)
            {
                if (nthTable < vFileIndex.size())
                    return vFileIndex[nthTable];

                return -1;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class resembles the CASSYLab
    /// *.labx file format, which is based upon XML.
    /// Only reading from this file format is
    /// supported.
    /////////////////////////////////////////////////
    class CassyLabx : public GenericFile
    {
        private:
            void readFile();
            double extractValueFromTag(const std::string& sTag);

        public:
            CassyLabx(const std::string& filename);
            virtual ~CassyLabx();

            virtual bool read() override
            {
                readFile();
                return true;
            }

            virtual bool write() override
            {
                // do nothing here
                return false;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class resembles a comma separated
    /// value file format (*.csv). The algorithm may
    /// detect the separator character automatically.
    /// Reading and writing is supported for this
    /// file format.
    /////////////////////////////////////////////////
    class CommaSeparatedValues : public GenericFile
    {
        private:
            void readFile();
            void writeFile();
            void writeHeader();
            char findSeparator(const std::vector<std::string>& vTextData);
            void countColumns(const std::vector<std::string>& vTextData, char& cSep);

        public:
            CommaSeparatedValues(const std::string& filename);
            virtual ~CommaSeparatedValues();

            virtual bool read() override
            {
                readFile();
                return true;
            }

            virtual bool write() override
            {
                writeFile();
                return true;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class resembles a LaTeX table.
    /// This class formats the data into this format
    /// using some heuristics on whether a long table
    /// or a standard table is needed to contain the
    /// tabular data. Only writing is supported by
    /// this file format.
    /////////////////////////////////////////////////
    class LaTeXTable : public GenericFile
    {
        private:
            void writeFile();
            void writeHeader();
            void writeTableHeads();
            size_t countHeadLines();
            std::string replaceNonASCII(const std::string& sText);
            std::string formatNumber(const std::complex<double>& number);

        public:
            LaTeXTable(const std::string& filename);
            virtual ~LaTeXTable();

            virtual bool read() override
            {
                // do nothing here
                return false;
            }

            virtual bool write() override
            {
                writeFile();
                return true;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class resembles a JCAMP-DX file
    /// format (*.jcm, *.jdx, *.dx). The data in this
    /// format may be hashed somehow to save storage.
    /// Only reading is suported by this class.
    /////////////////////////////////////////////////
    class JcampDX : public GenericFile
    {
        private:
            struct MetaData;

            void readFile();
            size_t readTable(std::vector<std::string>& vFileContents, size_t nTableStart, MetaData);
            void parseLabel(std::string& sLine);
            std::vector<double> parseLine(const std::string& sLine);

        public:
            JcampDX(const std::string& filename);
            virtual ~JcampDX();

            virtual bool read() override
            {
                readFile();
                return true;
            }

            virtual bool write() override
            {
                // do nothing
                return false;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class resembles an OpenDocument
    /// spreadsheet (*.ods), which is based upon a
    /// zipped XML file. The data is read using the
    /// Zipfile extractor from GenericFile. Only
    /// reading is supported by this class.
    /////////////////////////////////////////////////
    class OpenDocumentSpreadSheet : public GenericFile
    {
        private:
            void readFile();

        public:
            OpenDocumentSpreadSheet(const std::string& filename);
            virtual ~OpenDocumentSpreadSheet();

            virtual bool read() override
            {
                readFile();
                return true;
            }

            virtual bool write() override
            {
                // do nothing
                return false;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class resembles an Excel (97)
    /// workbook (*.xls), which is composed out of a
    /// compound file. Reading and writing is done
    /// using the BasicExcel library.
    /////////////////////////////////////////////////
    class XLSSpreadSheet : public GenericFile
    {
        private:
            const size_t MAXSHEETLENGTH = 31;
            void readFile();
            void writeFile();

        public:
            XLSSpreadSheet(const std::string& filename);
            virtual ~XLSSpreadSheet();

            virtual bool read() override
            {
                readFile();
                return true;
            }

            virtual bool write() override
            {
                writeFile();
                return true;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class resembles an Excel (2003)
    /// spreadsheet (*.xlsx), which is based upon a
    /// zipped XML file. The data is read using the
    /// Zipfile extractor from GenericFile. Only
    /// reading is supported by this class.
    /////////////////////////////////////////////////
    class XLSXSpreadSheet : public GenericFile
    {
        private:
            void readFile();
            void evalIndices(const std::string& sIndices, int& nLine, int& nCol);

        public:
            XLSXSpreadSheet(const std::string& filename);
            virtual ~XLSXSpreadSheet();

            virtual bool read() override
            {
                readFile();
                return true;
            }

            virtual bool write() override
            {
                // do nothing
                return false;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class resembles an Igor binary
    /// wave file format (*.ibw). The data is read by
    /// the WaveMetrics implementation of the file
    /// format. Only reading is supported by this
    /// class.
    /////////////////////////////////////////////////
    class IgorBinaryWave : public GenericFile
    {
        private:
            bool bXZSlice;

            void readFile();

        public:
            IgorBinaryWave(const std::string& filename);
            IgorBinaryWave(const IgorBinaryWave& file);
            virtual ~IgorBinaryWave();

            virtual bool read() override
            {
                readFile();
                return true;
            }

            virtual bool write() override
            {
                // do nothing
                return false;
            }

            IgorBinaryWave& operator=(const IgorBinaryWave& file);

            /////////////////////////////////////////////////
            /// \brief Activates the XZ-slicing of the Igor
            /// binary wave, which is used to roll out 3D
            /// data in 2D slices.
            ///
            /// \return void
            ///
            /////////////////////////////////////////////////
            void useXZSlicing()
            {
                bXZSlice = true;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class implements a Zygo MetroPro
    /// binary dat file. The data is read by
    /// accessing the ZygoLib.
    /////////////////////////////////////////////////
    class ZygoDat : public GenericFile
    {
        private:
            void readFile();

        public:
            ZygoDat(const std::string& filename);
            ZygoDat(const ZygoDat& file);
            virtual ~ZygoDat();

            virtual bool read() override
            {
                readFile();
                return true;
            }

            virtual bool write() override
            {
                return false;
            }

            ZygoDat& operator=(const ZygoDat& file);
        };
}



#endif // NUMERE_FILE_HPP


