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

#include "../utils/zip++.hpp"
#include "../ui/error.hpp"

namespace NumeRe
{
    template <class DATATYPE>
    class GenericFile
    {
        protected:
            std::fstream fFileStream;
            std::string sFileExtension;
            std::string sFileName;
            long long int nRows;
            long long int nCols;

            bool useExternalData;
            std::ios::openmode openMode;

            DATATYPE** fileData;
            std::string* fileTableHeads;

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

            template <typename T> T readNumField()
            {
                T num;

                fFileStream.read((char*)&num, sizeof(T));

                return num;
            }

            std::string readStringField()
            {
                long long int size = readNumField<long long int>();
                char* buffer = new char[size];

                fFileStream.read(buffer, size);

                std::string sBuffer(buffer, size);
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

            std::vector<std::string> tokenize(std::string sString, const std::string& sSeparators)
            {
                std::vector<std::string> vTokens;

                while (sString.length())
                {
                    vTokens.push_back(sString.substr(0, sString.find_first_of(sSeparators)));

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
                writeNumField<long long int>(sString.length());
                fFileStream.write(sString.c_str(), sString.length());
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

        public:
            GenericFile(const std::string& fileName, std::ios::openmode mode): nRows(0), nCols(0), useExternalData(false), fileData(nullptr), fileTableHeads(nullptr)
            {
                sFileName = fileName;

                if (fileName.rfind('.') != std::string::npos && fileName.rfind('/') != std::string::npos && fileName.rfind('.') > fileName.rfind('/'))
                    sFileExtension = fileName.substr(fileName.rfind('.'));

                fFileStream.open(sFileName.c_str(), mode);
                openMode = mode;
            }
            GenericFile(const GenericFile& file) : GenericFile(file.sFileName, file.openMode)
            {
                nRows = file.nRows;
                nCols = file.nCols;

                createStorage();
                copyDataArray(file.fileData, fileData, nRows, nCols);
                copyStringArray(file.fileTableHeads, fileTableHeads, nCols);
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
            std::string getExtension()
            {
                return sFileExtension;
            }
            std::string getFileName()
            {
                return sFileName;
            }
            long long int getRows()
            {
                return nRows;
            }
            long long int getCols()
            {
                return nCols;
            }

            virtual void read() = 0;
            virtual void write() = 0;

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
                copyStringArray(sHead, fileTableHeads, nCols);
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



    class NumeReDataFile : public GenericFile<double>
    {
        private:
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

        public:
            NumeReDataFile(const std::string& filename);
            NumeReDataFile(const NumeReDataFile& file);
            virtual ~NumeReDataFile();

            virtual void read() override
            {
                readFile();
            }
            virtual void write() override
            {
                writeFile();
            }

            void readFileInformation()
            {
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



    class CassyLabx : public GenericFile<double>
    {
        private:
            void readFile();

        public:
            CassyLabx(const std::string& filename);
            virtual ~CassyLabx();

            virtual void read() override
            {
                readFile();
            }

            virtual void write() override
            {
                // do nothing here
            }
    };



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

            virtual void read() override
            {
                readFile();
            }

            virtual void write() override
            {
                writeFile();
            }
    };



    class JcampDX : public GenericFile<double>
    {
        private:
            void readFile();
            void parseLabel(std::string& sLine);
            std::vector<double> parseLine(const std::string& sLine);

        public:
            JcampDX(const std::string& filename);
            virtual ~JcampDX();

            virtual void read() override
            {
                readFile();
            }

            virtual void write() override
            {
                // do nothing
            }
    };


    class OpenDocumentSpreadSheet : public GenericFile<double>
    {
        private:
            void readFile();
            std::string expandLine(const std::string& sLine);

        public:
            OpenDocumentSpreadSheet(const std::string& filename);
            ~OpenDocumentSpreadSheet();

            virtual void read() override
            {
                readFile();
            }

            virtual void write() override
            {
                // do nothing
            }
    };
}



#endif // NUMERE_FILE_HPP


