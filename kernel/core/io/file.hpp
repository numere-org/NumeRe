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
#include <fstream>
#include <cmath>
#include <vector>
#include <utility>

#include "../utils/zip++.hpp"
#include "../ui/error.hpp"
#include "../filesystem.hpp"

namespace NumeRe
{
    struct FileHeaderInfo
    {
        std::string sFileExtension;
        std::string sFileName;
        std::string sTableName;
        std::string sComment;
        long long int nRows;
        long long int nCols;
        long int versionMajor;
        long int versionMinor;
        long int versionBuild;
        time_t timeStamp;

        FileHeaderInfo() : sFileExtension(), sFileName(), sTableName(), sComment(), nRows(0), nCols(0), versionMajor(-1), versionMinor(-1), versionBuild(-1), timeStamp(0) {}
    };

    // Template class representing a generic file. This class may be specified
    // for the main data type contained in the read or written table. All other
    // file classes are derived from this class. This class cannot be instantiated
    // directly, because the read and write methods are declared as pure virtual
    template <class DATATYPE>
    class GenericFile : public FileSystem
    {
        protected:
            std::fstream fFileStream;
            std::string sFileExtension;
            std::string sFileName;
            std::string sTableName;
            long long int nRows;
            long long int nCols;
            unsigned short nPrecFields;

            // Flag for using external data, i.e. data, which won't be deleted
            // by this class upon calling "clearStorage()" or the destructor
            bool useExternalData;
            std::ios::openmode openMode;

            // The main data table
            DATATYPE** fileData;

            // The table column headlines
            std::string* fileTableHeads;

            // This method has to be used, to open the target file in stream
            // mode. If the file cannot be opened, this method throws an
            // error
            void open(std::ios::openmode mode)
            {
                if (fFileStream.is_open())
                    fFileStream.close();

                fFileStream.open(sFileName.c_str(), mode);

                if (!fFileStream.good())
                    throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

                openMode = mode;
            }

            // This method strips trailing spaces from the passed string
            void stripTrailingSpaces(std::string& _sToStrip)
            {
                if (_sToStrip.find_first_not_of(" \t") != std::string::npos)
                    _sToStrip.erase(_sToStrip.find_last_not_of(" \t")+1);
            }

            // This method simply replaces commas with dots in the passed string
            // to enable correct parsing into a double
            void replaceDecimalSign(std::string& _sToReplace)
            {
                for (size_t i = 0; i < _sToReplace.length(); i++)
                {
                    if (_sToReplace[i] == ',')
                        _sToReplace[i] = '.';
                }
            }

            // This method replaces tabulator characters into whitespaces
            // to simplify the column determination (the used tokenizer will
            // only have to consider whitespaces as separator characters).
            // Sometimes, replacing tabulators into whitespaces will destroy
            // column information. To avoid this, placeholders (underscores) may
            // be inserted as "empty" column cells.
            void replaceTabSign(std::string& _sToReplace, bool bAddPlaceholders = false)
            {
                for (size_t i = 0; i < _sToReplace.length(); i++)
                {
                    if (_sToReplace[i] == '\t')
                    {
                        _sToReplace[i] = ' ';

                        // Shall "empty cells" be added?
                        if (bAddPlaceholders)
                        {
                            // Insert underscores as empty cells
                            if (!i)
                                _sToReplace.insert(0, 1, '_');
                            else if (_sToReplace[i-1] == ' ')
                                _sToReplace.insert(i, 1, '_');

                            if (i+1 == _sToReplace.length())
                                _sToReplace += "_";
                        }
                    }
                }
            }

            // This method calculates the extents of the passed string, if it
            // is used as a table column headline. This method will return a
            // std::pair<> with the maximal numbers of characters in a line
            // in the first and the number of lines in the second component
            std::pair<size_t, size_t> calculateCellExtents(const std::string& sContents)
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
                }

                // Examine the last part of the string, which won't be
                // detected by the for loop
                if (sContents.length() - nLastLineBreak > pCellExtents.first)
                    pCellExtents.first = sContents.length() - nLastLineBreak;

                return pCellExtents;
            }

            // This method gets the selected line number from the table column
            // headline in the selected column. If the selected text does not
            // contain enough lines, a simple whitespace is returned
            std::string getLineFromHead(long long int nCol, size_t nLineNumber)
            {
                size_t nLastLineBreak = 0u;

                // Search the selected part
                for (size_t i = 0; i < fileTableHeads[nCol].length(); i++)
                {
                    // Linebreak character found?
                    if (fileTableHeads[nCol][i] == '\n')
                    {
                        // If this is the correct line number, return
                        // the corresponding substring
                        if (!nLineNumber)
                            return fileTableHeads[nCol].substr(nLastLineBreak, i - nLastLineBreak);

                        // Decrement the line number and store the
                        // position of the current line break
                        nLineNumber--;
                        nLastLineBreak = i+1;
                    }
                }

                // Catch the last part of the string, which is not found by
                // the for loop
                if (!nLineNumber)
                    return fileTableHeads[nCol].substr(nLastLineBreak);

                // Not enough lines in this string, return a whitespace character
                return " ";
            }

            // This method is a template for reading an numeric field of the
            // selected template type in binary mode
            template <typename T> T readNumField()
            {
                T num;
                fFileStream.read((char*)&num, sizeof(T));
                return num;
            }

            // This method can be used to read a string field from the file
            // in binary mode
            std::string readStringField()
            {
                // Get the length of the field
                size_t nLength = readNumField<size_t>();

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

            // This method may be used to get the contents of an embedded file
            // in a zipfile and return the contents as string
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

            // This method template is for reading a block of numeric data
            // into memory in binary mode
            template <typename T> T* readNumBlock(long long int& size)
            {
                // Get the number of values in the data block
                size = readNumField<long long int>();

                // If the size is zero, return a null pointer
                if (!size)
                    return nullptr;

                // Create new storage for the data
                T* data = new T[size];

                // Read the contents of the file into the created storager
                fFileStream.read((char*)data, sizeof(T)*size);

                return data;
            }

            // This method template is for reading a whole two-dimensional
            // array of data into memory in binary mode
            DATATYPE** readDataArray(long long int& rows, long long int& cols)
            {
                // Get the dimensions of the data block in memory
                rows = readNumField<long long int>();
                cols = readNumField<long long int>();

                // If one of the dimensions is zero, return a null pointer
                if (!rows || !cols)
                    return nullptr;

                // Prepare a new storage object for the contained data
                DATATYPE** data = new DATATYPE*[rows];

                // Create the storage for the columns during reading the
                // file and read the contents directly to memory
                for (long long int i = 0; i < rows; i++)
                {
                    data[i] = new DATATYPE[cols];
                    fFileStream.read((char*)data[i], sizeof(DATATYPE)*cols);
                }

                return data;
            }

            // This method can be used for reading a block of string data
            // to memory in binary mode
            std::string* readStringBlock(long long int& size)
            {
                // Get the number of strings in the current block
                size = readNumField<long long int>();

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

            // This method may be used to read the file in text mode and
            // to obtain the data as a vector
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

            // This method may be used to separate a line into multiple
            // tokens using a set of separator characters. If empty tokens
            // shall be skipped, then only tokens with a non-zero length
            // are stored
            std::vector<std::string> tokenize(std::string sString, const std::string& sSeparators, bool skipEmptyTokens = false)
            {
                std::vector<std::string> vTokens;

                // As long as the string has a length
                while (sString.length())
                {
                    // Store the string until the first separator character
                    vTokens.push_back(sString.substr(0, sString.find_first_of(sSeparators)));

                    // If empty tokens shall not be stored, remove the last
                    // token again, if it is empty
                    if (skipEmptyTokens && !vTokens.back().length())
                        vTokens.pop_back();

                    // Erase the contents of the line including the first
                    // separator character
                    if (sString.find_first_of(sSeparators) != std::string::npos)
                        sString.erase(0, sString.find_first_of(sSeparators)+1);
                    else
                        break;
                }

                return vTokens;
            }

            // This method template can be used to write a numeric value
            // to file in binary mode
            template <typename T> void writeNumField(T num)
            {
                fFileStream.write((char*)&num, sizeof(T));
            }

            // This method may be used to write a string to file in
            // binary mode
            void writeStringField(const std::string& sString)
            {
                // Store the length of string as numeric value first
                writeNumField<size_t>(sString.length());
                fFileStream.write(sString.c_str(), sString.length());
            }

            // This method template may be used to write a block of
            // data of the selected type to the file in binary mode
            template <typename T> void writeNumBlock(T* data, long long int size)
            {
                // Store the length of the data block first
                writeNumField<long long int>(size);
                fFileStream.write((char*)data, sizeof(T)*size);
            }

            // This method may be used to write a two-dimensional array of
            // data to the file in binary mode
            void writeDataArray(DATATYPE** data, long long int rows, long long int cols)
            {
                // Store the dimensions of the array first
                writeNumField<long long int>(rows);
                writeNumField<long long int>(cols);

                // Write the contents to the file in linewise fashion
                for (long long int i = 0; i < rows; i++)
                    fFileStream.write((char*)data[i], sizeof(DATATYPE)*cols);
            }

            // This method may used to write a block of strings into the
            // file in binary mode
            void writeStringBlock(std::string* data, long long int size)
            {
                // Store the number of fields first
                writeNumField<long long int>(size);

                // Write each field separately, because their length is
                // independent on each other
                for (long long int i = 0; i < size; i++)
                {
                    writeStringField(data[i]);
                }
            }

            // This method prepares the internal storage, so that it
            // may contain the read data. This method is only used
            // for textual files
            void createStorage()
            {
                if (nRows > 0 && nCols > 0 && !fileData && !fileTableHeads)
                {
                    fileTableHeads = new std::string[nCols];

                    fileData = new DATATYPE*[nRows];

                    for (long long int i = 0; i < nRows; i++)
                    {
                        fileData[i] = new DATATYPE[nCols];

                        for (long long int j = 0; j < nCols; j++)
                            fileData[i][j] = NAN;
                    }
                }
                else if (nRows < 0 || nCols < 0)
                    throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);
            }

            // This method cleares the internal storage. This method
            // is called by the destructor automatically
            void clearStorage()
            {
                // Only delete the storage, if it is internal data. Do
                // not delete the storage, if it is linked from an external
                // source
                if (!useExternalData)
                {
                    if (fileTableHeads)
                    {
                        delete[] fileTableHeads;
                        fileTableHeads = nullptr;
                    }

                    if (fileData)
                    {
                        for (long long int i = 0; i < nRows; i++)
                            delete[] fileData[i];

                        delete[] fileData;
                        fileData = nullptr;
                    }

                }
                else
                {
                    fileTableHeads = nullptr;
                    fileData = nullptr;
                }

                nRows = 0;
                nCols = 0;
            }

            // This method may be used to copy two-dimensional arrays of
            // data. Both source and target arrays have to exist in advance
            void copyDataArray(DATATYPE** from, DATATYPE** to, long long int rows, long long int cols)
            {
                if (!from || !to || !rows || !cols)
                    return;

                for (long long int i = 0; i < rows; i++)
                {
                    for (long long int j = 0; j < cols; j++)
                        to[i][j] = from[i][j];
                }
            }

            // This method may be used to copy string arrays. Both
            // source and target arrays have to exist in advance
            void copyStringArray(std::string* from, std::string* to, long long int nElements)
            {
                if (!from || !to || !nElements)
                    return;

                for (long long int i = 0; i < nElements; i++)
                    to[i] = from[i];
            }

            // This method template may be used to copy arrays of data
            // of the selected type. Both source and target arrays have
            // to exist in advance
            template <typename T> void copyArray(T* from, T* to, long long int nElements)
            {
                if (!from || !to || !nElements)
                    return;

                for (long long int i = 0; i < nElements; i++)
                    to[i] = from[i];
            }

            // This method may be used to determine, whether a string
            // contains only numeric data
            bool isNumeric(const std::string& sString)
            {
                if (!sString.length())
                    return false;

                static const std::string sVALIDNUMERICCHARACTERS = "0123456789eE+-.,\t% ";

                for (unsigned int i = 0; i < sString.length(); i++)
                {
                    if (sVALIDNUMERICCHARACTERS.find(sString[i]) != std::string::npos)
                        continue;
                    else if (sString.substr(i, 3) == "nan"
                        || sString.substr(i, 3) == "NaN"
                        || sString.substr(i, 3) == "NAN"
                        || sString.substr(i, 3) == "inf"
                        || sString.substr(i, 3) == "INF"
                        )
                    {
                        i += 2;
                        continue;
                    }
                    else
                    {
                        return false;
                    }
                }

                return true;
            }

            // This method is used by the assignment operator and
            // the copy constructor to copy the contents of the
            // passed GenericFile instance
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

                createStorage();
                copyDataArray(file.fileData, fileData, nRows, nCols);
                copyStringArray(file.fileTableHeads, fileTableHeads, nCols);
            }

        public:
            // Constructor from filename
            GenericFile(const std::string& fileName) : FileSystem(), nRows(0), nCols(0), nPrecFields(7), useExternalData(false), fileData(nullptr), fileTableHeads(nullptr)
            {
                // Initializes the file system from the kernel
                initializeFromKernel();
                sFileName = ValidFileName(fileName, "", false);
                sFileExtension = getFileParts(sFileName).back();
            }

            // Copy constructor
            GenericFile(const GenericFile& file) : GenericFile(file.sFileName)
            {
                assign(file);
            }

            // Virtual destructor: we'll work with instances on the heap
            // therefore we'll need virtual declared destructors. This
            // destructor will clear the internal memory and closes the
            // file stream, if it is still open
            virtual ~GenericFile()
            {
                clearStorage();

                if (fFileStream.is_open())
                    fFileStream.close();
            }

            // Wrapper for fstream::is_open()
            bool is_open()
            {
                return fFileStream.is_open();
            }

            // Wrapper for fstream::close(). Will also clear the internal
            // memory
            void close()
            {
                fFileStream.close();
                clearStorage();
            }

            // Wrapper for fstream::good()
            bool good()
            {
                return fFileStream.good();
            }

            // Wrapper for fstream::tellg()
            size_t tellg()
            {
                return fFileStream.tellg();
            }

            // Wrapper for fstream::tellp()
            size_t tellp()
            {
                return fFileStream.tellp();
            }

            // Wrapper for fstream::seekg() with start from the
            // beginning of the stream
            void seekg(size_t pos)
            {
                fFileStream.seekg(pos, ios::beg);
            }

            // Wrapper for fstream::seekp() with start from the
            // beginning of the stream
            void seekp(size_t pos)
            {
                fFileStream.seekp(pos, ios::beg);
            }

            // Returns the file extension
            std::string getExtension()
            {
                return sFileExtension;
            }

            // Returns the file name
            std::string getFileName()
            {
                return sFileName;
            }

            // Returns the table name referenced in the file. Will
            // default to the file name with non-alnum characters
            // replaced with underscores, if the file does not reference
            // a table name by itself
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

            // Returns the number of rows
            long long int getRows()
            {
                return nRows;
            }

            // Returns the number of columns
            long long int getCols()
            {
                return nCols;
            }

            // Returns the file header information
            // structure
            virtual FileHeaderInfo getFileHeaderInformation()
            {
                FileHeaderInfo info;

                info.nCols = nCols;
                info.nRows = nRows;
                info.sFileExtension = sFileExtension;
                info.sFileName = sFileName;
                info.sTableName = getTableName();

                return info;
            }

            // Pure virtual declaration of read and write
            // access methods. These have to be implemented in all
            // derived classes
            virtual bool read() = 0;
            virtual bool write() = 0;

            // Assignent operator definition
            GenericFile& operator=(const GenericFile& file)
            {
                assign(file);
                return *this;
            }

            // This method copies the internal data to the
            // passed memory address. The target memory must
            // exist
            void getData(DATATYPE** data)
            {
                copyDataArray(fileData, data, nRows, nCols);
            }

            // This method returns a pointer to the internal
            // memory with read and write access. This pointer
            // shall not be stored for future use, because
            // the referenced memory will be deleted upon
            // destruction of this class instance
            DATATYPE** getData(long long int& rows, long long int& cols)
            {
                rows = nRows;
                cols = nCols;

                return fileData;
            }

            // This method copies the column headings of the
            // internal data to the passed memory address.
            // The target memory must exist
            void getColumnHeadings(std::string* sHead)
            {
                copyStringArray(fileTableHeads, sHead, nCols);
            }

            // This method returns a pointer to the column
            // headings of the internal memory with read and
            // write access. This pointer shall not be stored
            // for future use, because the referenced memory
            // will be deleted upon destruction of this
            // class instance
            std::string* getColumnHeadings(long long int& cols)
            {
                cols = nCols;
                return fileTableHeads;
            }

            // Sets the dimensions of the data table, which
            // will be used in the future. Clears the internal
            // memory in advance
            void setDimensions(long long int rows, long long int cols)
            {
                clearStorage();

                nRows = rows;
                nCols = cols;
            }

            // Set the table's name
            void setTableName(const std::string& name)
            {
                sTableName = name;
            }

            // Set the precision, which shall be used to convert
            // the floating point numbers into strings
            void setTextfilePrecision(unsigned short nPrecision)
            {
                nPrecFields = nPrecision;
            }

            // This method creates the internal storage and copies
            // the passed data to this storage
            void addData(DATATYPE** data, long long int rows, long long int cols)
            {
                if (!nRows && !nCols)
                {
                    nRows = rows;
                    nCols = cols;
                }

                createStorage();
                copyDataArray(data, fileData, rows, cols);
            }

            // This method creates the internal storage (if not already
            // done) and copies the passed column headings to this storage
            void addColumnHeadings(std::string* sHead, long long int cols)
            {
                createStorage();
                copyStringArray(sHead, fileTableHeads, cols);
            }

            // This method references the passed external data internally.
            // The data is not copied and must exist as long as this class
            // exists
            void setData(DATATYPE** data, long long int rows, long long int cols)
            {
                useExternalData = true;

                fileData = data;
                nRows = rows;
                nCols = cols;
            }

            // This method references the passed external column headings internally.
            // The headings are not copied and must exist as long as this class
            // exists
            void setColumnHeadings(std::string* sHead, long long int cols)
            {
                useExternalData = true;

                fileTableHeads = sHead;
                nCols = cols;
            }
    };



    // This function determines the correct class to be used for the filename
    // passed to this function. If there's no fitting file type, a null pointer
    // is returned. The calling function is responsible for clearing the
    // created instance. The returned pointer is of the type of GenericFile<double>
    // but references an instance of a derived class
    GenericFile<double>* getFileByType(const std::string& filename);

    template <class DATATYPE>
    class GenericFileView
    {
        private:
            GenericFile<DATATYPE>* m_file;

        public:
            GenericFileView() : m_file(nullptr) {}
            GenericFileView(GenericFile<DATATYPE>* _file) : m_file(_file) {}

            void attach(GenericFile<DATATYPE>* _file)
            {
                m_file = _file;
            }

            GenericFile<DATATYPE>* getPtr()
            {
                return m_file;
            }

            long long int getCols() const
            {
                if (m_file)
                    return m_file->getCols();

                return 0;
            }

            long long int getRows() const
            {
                if (m_file)
                    return m_file->getRows();

                return 0;
            }

            DATATYPE getElement(long long int row, long long int col) const
            {
                if (m_file)
                {
                    if (row >= m_file->getRows() || col >= m_file->getCols())
                        return 0;

                    // dummy variable
                    long long int r,c;
                    return m_file->getData(r,c)[row][col];
                }

                return 0;
            }

            string getColumnHead(long long int col) const
            {
                if (m_file)
                {
                    if (col >= m_file->getCols())
                        return "";

                    // dummy variable
                    long long int c;
                    return m_file->getColumnHeadings(c)[col];
                }

                return "";
            }
    };

    typedef GenericFileView<double> FileView;


    // This class resembles an arbitrary text data file, which is formatted in a
    // table-like manner. The columns may be separated using tabulators and/or
    // whitespace characters.
    class TextDataFile : public GenericFile<double>
    {
        private:
            void readFile();
            void writeFile();
            void writeHeader();
            void writeTableHeads(const std::vector<size_t>& vColumnWidth, size_t nNumberOfLines);
            void writeTableContents(const std::vector<size_t>& vColumnWidth);
            void addSeparator(const std::vector<size_t>& vColumnWidth);

            void decodeTableHeads(std::vector<std::string>& vFileContents, long long int nComment);
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



    // This class resembles the binary NumeRe data file format. The data is
    // read and written in binary mode using the methods from GenericFile
    class NumeReDataFile : public GenericFile<double>
    {
        protected:
            bool isLegacy;
            time_t timeStamp;
            std::string sComment;
            long int versionMajor;
            long int versionMinor;
            long int versionBuild;
            const short fileVersionMajor = 2;
            const short fileVersionMinor = 0;

            void writeHeader();
            void writeDummyHeader();
            void writeFile();
            void readHeader();
            void skipDummyHeader();
            void readFile();
            void readLegacyFormat();
            void* readGenericField(std::string& type, long long int& size);
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

            void readFileInformation()
            {
                open(std::ios::in | std::ios::binary);
                readHeader();
            }

            time_t getTimeStamp()
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
                info.timeStamp = timeStamp;

                return info;
            }


            std::string getVersionString();
            std::string getComment()
            {
                return sComment;
            }

            void setComment(const std::string& comment)
            {
                sComment = comment;
            }
    };



    // This class resembles the cache file used to auto save and recover the tables
    // in memory. It's derived from the NumeRe data file format and uses its
    // functionalities to layout the data in the file: the cache file starts with
    // a header containing the number of tables in the file and the character
    // positions in the file, where each table starts. The tables itself are written
    // in the NumeRe data file format
    class CacheFile : public NumeReDataFile
    {
        private:
            std::vector<size_t> vFileIndex;
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

            size_t getNumberOfTables()
            {
                return vFileIndex.size();
            }

            void setNumberOfTables(size_t nTables)
            {
                vFileIndex = std::vector<size_t>(nTables, 0u);
            }

            size_t getPosition(size_t nthTable)
            {
                if (nthTable < vFileIndex.size())
                    return vFileIndex[nthTable];

                return -1;
            }
    };



    // This class resembles the CASSYLab *.labx file format, which is based
    // upon XML. Only reading from this file format is supported.
    class CassyLabx : public GenericFile<double>
    {
        private:
            void readFile();
            double extractValueFromTag(const string& sTag);

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



    // This class resembles a comma separated value file format (*.csv). The
    // algorithm may detect the separator character automatically. Reading
    // and writing is supported for this file format
    class CommaSeparatedValues : public GenericFile<double>
    {
        private:
            void readFile();
            void writeFile();
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



    // This class resembles a LaTeX table. This class formats the data
    // into this format using some heuristics on whether a long table
    // or a standard table is needed to contain the tabular data. Only
    // writing is supported by this file format.
    class LaTeXTable : public GenericFile<double>
    {
        private:
            void writeFile();
            void writeHeader();
            void writeTableHeads();
            size_t countHeadLines();
            std::string replaceNonASCII(const std::string& sText);
            std::string formatNumber(double number);

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



    // This class resembles a JCAMP-DX file format (*.jcm, *.jdx, *.dx). The
    // data in this format may be hashed somehow to save storage. Only reading
    // is supported by this class.
    class JcampDX : public GenericFile<double>
    {
        private:
            void readFile();
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



    // This class resembles a OpenDocument spreadsheet (*.ods), which is based
    // upon a zipped XML file. The data is read using the Zipfile extractor
    // from GenericFile. Only reading is supported by this class.
    class OpenDocumentSpreadSheet : public GenericFile<double>
    {
        private:
            void readFile();
            std::string expandLine(const std::string& sLine);

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



    // This class resembles a Excel (97) workbook (*.xls), which is composed
    // out of a compound file. Reading and writing is done using the BasicExcel
    // library.
    class XLSSpreadSheet : public GenericFile<double>
    {
        private:
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



    // This class resembles a Excel (2003) spreadsheet (*.xlsx), which is based
    // upon a zipped XML file. The data is read using the Zipfile extractor
    // from GenericFile. Only reading is supported by this class.
    class XLSXSpreadSheet : public GenericFile<double>
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



    // This class resembles a Igor binary wave file format (*.ibw). The data is
    // read by the WaveMetrics implementation of the file format. Only reading is
    // supported by this class.
    class IgorBinaryWave : public GenericFile<double>
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

            void useXZSlicing()
            {
                bXZSlice = true;
            }
    };
}



#endif // NUMERE_FILE_HPP


