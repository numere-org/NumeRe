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
    //
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

            bool useExternalData;
            std::ios::openmode openMode;

            DATATYPE** fileData;
            std::string* fileTableHeads;

            void open(std::ios::openmode mode)
            {
                if (fFileStream.is_open())
                    fFileStream.close();

                fFileStream.open(sFileName.c_str(), mode);

                if (!fFileStream.good())
                    throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

                openMode = mode;
            }

            void stripTrailingSpaces(std::string& _sToStrip)
            {
                if (_sToStrip.find_first_not_of(" \t") != std::string::npos)
                    _sToStrip.erase(_sToStrip.find_last_not_of(" \t")+1);
            }

            void replaceDecimalSign(std::string& _sToReplace)
            {
                if (_sToReplace.find(',') != std::string::npos)
                {
                    for (size_t i = 0; i < _sToReplace.length(); i++)
                    {
                        if (_sToReplace[i] == ',')
                            _sToReplace[i] = '.';
                    }
                }
            }

            void replaceTabSign(std::string& _sToReplace, bool bAddPlaceholders = false)
            {
                if (_sToReplace.find('\t') != std::string::npos)
                {
                    for (size_t i = 0; i < _sToReplace.length(); i++)
                    {
                        if (_sToReplace[i] == '\t')
                        {
                            _sToReplace[i] = ' ';

                            if (bAddPlaceholders)
                            {
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
            }

            std::pair<size_t, size_t> calculateCellExtents(const std::string& sContents)
            {
                // x, y
                std::pair<size_t, size_t> pCellExtents(0u, 1u);
                size_t nLastLineBreak = 0;

                for (size_t i = 0; i < sContents.length(); i++)
                {
                    if (sContents[i] == '\n')
                    {
                        pCellExtents.second++;

                        if (i - nLastLineBreak > pCellExtents.first)
                            pCellExtents.first = i - nLastLineBreak;

                        nLastLineBreak = i;
                    }
                }

                return pCellExtents;
            }

            std::string getLineFromHead(long long int nCol, size_t nLineNumber)
            {
                size_t nLastLineBreak = 0u;

                for (size_t i = 0; i < fileTableHeads[nCol].length(); i++)
                {
                    if (fileTableHeads[nCol][i] == '\n')
                    {
                        if (!nLineNumber)
                            return fileTableHeads[nCol].substr(nLastLineBreak, i - nLastLineBreak);

                        nLineNumber--;
                        nLastLineBreak = i+1;
                    }
                }

                if (nLineNumber == 1)
                    return fileTableHeads[nCol].substr(nLastLineBreak);

                return " ";
            }

            template <typename T> T readNumField()
            {
                T num;

                fFileStream.read((char*)&num, sizeof(T));

                return num;
            }

            std::string readStringField()
            {
                size_t nLength = readNumField<size_t>();

                if (!nLength)
                    return "";

                char* buffer = new char[nLength];

                fFileStream.read(buffer, nLength);

                std::string sBuffer(buffer, nLength);
                delete[] buffer;

                return sBuffer;
            }

            std::string getZipFileItem(const std::string& filename)
            {
                Zipfile* _zip = new Zipfile();

                if (!_zip->open(sFileName))
                {
                    _zip->close();
                    delete _zip;
                    throw SyntaxError(SyntaxError::DATAFILE_NOT_EXIST, sFileName, SyntaxError::invalid_position, sFileName);
                }

                std::string sFileItem = _zip->getZipItem(filename);
                _zip->close();
                delete _zip;

                return sFileItem;
            }

            template <typename T> T* readNumBlock(long long int& size)
            {
                size = readNumField<long long int>();

                if (!size)
                    return nullptr;

                T* data = new T[size];

                fFileStream.read((char*)data, sizeof(T)*size);

                return data;
            }

            DATATYPE* readDataBlock(long long int& size)
            {
                size = readNumField<long long int>();

                if (!size)
                    return nullptr;

                DATATYPE* data = new DATATYPE[size];

                fFileStream.read((char*)data, sizeof(DATATYPE)*size);

                return data;
            }

            DATATYPE** readDataArray(long long int& rows, long long int& cols)
            {
                rows = readNumField<long long int>();
                cols = readNumField<long long int>();

                if (!rows || !cols)
                    return nullptr;

                DATATYPE** data = new DATATYPE*[rows];

                for (long long int i = 0; i < rows; i++)
                {
                    data[i] = new DATATYPE[cols];
                    fFileStream.read((char*)data[i], sizeof(DATATYPE)*cols);
                }

                return data;
            }

            std::string* readStringBlock(long long int& size)
            {
                size = readNumField<long long int>();

                if (!size)
                    return nullptr;

                std::string* data = new std::string[size];

                for (long long int i = 0; i < size; i++)
                {
                    data[i] = readStringField();
                }

                return data;
            }

            std::vector<std::string> readTextFile(bool stripEmptyLines)
            {
                std::vector<std::string> vTextFile;
                std::string currentLine;

                while (!fFileStream.eof())
                {
                    std::getline(fFileStream, currentLine);

                    if (stripEmptyLines)
                        stripTrailingSpaces(currentLine);

                    if (!stripEmptyLines || currentLine.length())
                        vTextFile.push_back(currentLine);
                }

                return vTextFile;
            }

            std::vector<std::string> tokenize(std::string sString, const std::string& sSeparators, bool skipEmptyTokens = false)
            {
                std::vector<std::string> vTokens;

                while (sString.length())
                {
                    vTokens.push_back(sString.substr(0, sString.find_first_of(sSeparators)));

                    if (skipEmptyTokens && !vTokens.back().length())
                        vTokens.pop_back();

                    if (sString.find_first_of(sSeparators) != std::string::npos)
                        sString.erase(0, sString.find_first_of(sSeparators)+1);
                    else
                        break;
                }

                return vTokens;
            }

            template <typename T> void writeNumField(T num)
            {
                fFileStream.write((char*)&num, sizeof(T));
            }

            void writeStringField(const std::string& sString)
            {
                writeNumField<size_t>(sString.length());
                fFileStream.write(sString.c_str(), sString.length());
            }

            template <typename T> void writeNumBlock(T* data, long long int size)
            {
                writeNumField<long long int>(size);
                fFileStream.write((char*)data, sizeof(T)*size);
            }

            void writeDataBlock(DATATYPE* data, long long int size)
            {
                writeNumField<long long int>(size);
                fFileStream.write((char*)data, sizeof(DATATYPE)*size);
            }

            void writeDataArray(DATATYPE** data, long long int rows, long long int cols)
            {
                writeNumField<long long int>(rows);
                writeNumField<long long int>(cols);

                for (long long int i = 0; i < rows; i++)
                    fFileStream.write((char*)data[i], sizeof(DATATYPE)*cols);
            }

            void writeStringBlock(std::string* data, long long int size)
            {
                writeNumField<long long int>(size);

                for (long long int i = 0; i < size; i++)
                {
                    writeStringField(data[i]);
                }
            }

            void createStorage()
            {
                if (nRows && nCols && !fileData && !fileTableHeads)
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
            }

            void clearStorage()
            {
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

            void copyStringArray(std::string* from, std::string* to, long long int nElements)
            {
                if (!from || !to || !nElements)
                    return;

                for (long long int i = 0; i < nElements; i++)
                    to[i] = from[i];
            }

            template <typename T> void copyArray(T* from, T* to, long long int nElements)
            {
                if (!from || !to || !nElements)
                    return;

                for (long long int i = 0; i < nElements; i++)
                    to[i] = from[i];
            }

            bool isNumeric(const std::string& sString)
            {
                if (!sString.length())
                    return false;

                for (unsigned int i = 0; i < sString.length(); i++)
                {
                    if ((sString[i] >= '0' && sString[i] <= '9')
                        || sString[i] == 'e'
                        || sString[i] == 'E'
                        || sString[i] == '-'
                        || sString[i] == '+'
                        || sString[i] == '.'
                        || sString[i] == ','
                        || sString[i] == '\t'
                        || sString[i] == '%'
                        || sString[i] == ' ')
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
            GenericFile(const std::string& fileName) : FileSystem(), nRows(0), nCols(0), nPrecFields(7), useExternalData(false), fileData(nullptr), fileTableHeads(nullptr)
            {
                sFileName = ValidFileName(fileName, "", false);
                sFileExtension = getFileParts(sFileName).back();
            }

            GenericFile(const GenericFile& file) : GenericFile(file.sFileName)
            {
                assign(file);
            }

            virtual ~GenericFile()
            {
                clearStorage();

                if (fFileStream.is_open())
                    fFileStream.close();
            }

            bool is_open()
            {
                return fFileStream.is_open();
            }

            void close()
            {
                fFileStream.close();
                clearStorage();
            }

            bool good()
            {
                return fFileStream.good();
            }

            size_t tellg()
            {
                return fFileStream.tellg();
            }

            size_t tellp()
            {
                return fFileStream.tellp();
            }

            void seekg(size_t pos)
            {
                fFileStream.seekg(pos, ios::beg);
            }

            void seekp(size_t pos)
            {
                fFileStream.seekp(pos, ios::beg);
            }

            std::string getExtension()
            {
                return sFileExtension;
            }

            std::string getFileName()
            {
                return sFileName;
            }

            std::string getTableName()
            {
                if (!sTableName.length())
                {
                    sTableName = getFileParts(sFileName)[2];

                    for (size_t i = 0; i < sTableName.length(); i++)
                    {
                        if (sTableName[i] != '_' && !isalnum(sTableName[i]))
                            sTableName[i] = '_';
                    }

                    if (isdigit(sTableName.front()))
                        sTableName.insert(0, 1, '_');
                }

                return sTableName;
            }

            long long int getRows()
            {
                return nRows;
            }

            long long int getCols()
            {
                return nCols;
            }

            virtual bool read() = 0;
            virtual bool write() = 0;

            GenericFile& operator=(const GenericFile& file)
            {
                assign(file);
                return *this;
            }

            void getData(DATATYPE** data)
            {
                copyDataArray(fileData, data, nRows, nCols);
            }

            DATATYPE** getData(long long int& rows, long long int& cols)
            {
                rows = nRows;
                cols = nCols;

                return fileData;
            }

            void getColumnHeadings(std::string* sHead)
            {
                copyStringArray(fileTableHeads, sHead, nCols);
            }

            std::string* getColumnHeadings(long long int& cols)
            {
                cols = nCols;
                return fileTableHeads;
            }

            void setDimensions(long long int rows, long long int cols)
            {
                clearStorage();

                nRows = rows;
                nCols = cols;
            }

            void setTableName(const std::string& name)
            {
                sTableName = name;
            }

            void setTextfilePrecision(unsigned short nPrecision)
            {
                nPrecFields = nPrecision;
            }

            // use external data == no
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

            void addColumnHeadings(std::string* sHead, long long int cols)
            {
                createStorage();
                copyStringArray(sHead, fileTableHeads, cols);
            }

            // use external data == yes
            void setData(DATATYPE** data, long long int rows, long long int cols)
            {
                useExternalData = true;

                fileData = data;
                nRows = rows;
                nCols = cols;
            }

            void setColumnHeadings(std::string* sHead, long long int cols)
            {
                useExternalData = true;

                fileTableHeads = sHead;
                nCols = cols;
            }
    };

    //
    GenericFile<double>* getFileByType(const std::string& filename);

    //
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

    //
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

    //
    class CacheFile : public NumeReDataFile
    {
        private:
            std::vector<size_t> vFileIndex;
            size_t nIndexPos;

            void reset();
            void readSome();
            void writeSome();
            void readCacheHeader();
            void writeCacheHeader();

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

    //
    class CassyLabx : public GenericFile<double>
    {
        private:
            void readFile();

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

    //
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

    //
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

    //
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

    //
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

    //
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

    //
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

    //
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


