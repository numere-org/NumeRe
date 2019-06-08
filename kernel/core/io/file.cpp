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

#include "file.hpp"
#include "../IgorLib/ReadWave.h"
#include "../utils/tools.hpp"
#include "../utils/BasicExcel.hpp"
#include "../utils/tinyxml2.h"
#include "../ui/language.hpp"
#include "../version.h"

extern Language _lang;

namespace NumeRe
{
    using namespace std;

    //////////////////////////////////////////////
    // class TextDataFile
    //////////////////////////////////////////////
    //
    TextDataFile::TextDataFile(const string& filename) : GenericFile(filename)
    {
        //
    }

    TextDataFile::~TextDataFile()
    {
        //
    }

    void TextDataFile::readFile()
    {
        open(ios::in);

		// --> Benoetigte temporaere Variablen initialisieren <--
		string s = "";
		long long int nLine = 0;
		long long int nCol = 0;
		long long int nComment = 0;

		vector<string> vFileContents = readTextFile(true);

		if (!vFileContents.size())
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

		for (size_t i = 0; i < vFileContents.size(); i++)
        {
            if (!isNumeric(vFileContents[i]))
                replaceTabSign(vFileContents[i], true);
            else
            {
                replaceTabSign(vFileContents[i]);
                stripTrailingSpaces(vFileContents[i]);
            }
        }

		// --> Kommentare muessen auch noch escaped werden... <--
		for (size_t i = 0; i < vFileContents.size(); i++)
		{
			if (vFileContents[i][0] == '#' || !isNumeric(vFileContents[i]))	// ist das erste Zeichen ein #?
			{
				nComment++;			// Kommentarzeilen zaehlen
				continue; 			// Dann ist das wohl ein Kommentar -> Ueberspringen
			}
			else if (!nCol)			// Suche so lange, bis du eine Zeile findest, die nicht kommentiert wurde
			{						// Sobald die Spaltenzahl einmal bestimmt wurde, braucht das hier nicht mehr durchgefuehrt werden
				// --> Verwende einen Tokenizer, um den String in Teile zu zerlegen und diese zu zaehlen <--
				nCol = tokenize(vFileContents[i], " ", true).size();
			}
		}

		nRows = vFileContents.size() - nComment;	// Die maximale Zahl der Zeilen - die Zahl der Kommentare ergibt die noetige Zahl der Zeilen
		nCols = nCol;

		createStorage();

		nLine = 0;

		for (size_t i = 0; i < vFileContents.size(); i++)
        {
            if (vFileContents[i][0] == '#' || !isNumeric(vFileContents[i]))
            {
                // ignore table heads
                continue;
            }

            vector<string> vLine = tokenize(vFileContents[i], " ", true);

            if (vLine.size() != nCols)
                throw SyntaxError(SyntaxError::COL_COUNTS_DOESNT_MATCH, sFileName, SyntaxError::invalid_position, sFileName);

            for (size_t j = 0; j < vLine.size(); j++)
            {
                replaceDecimalSign(vLine[j]);

                if (vLine[j] == "---" || toLowerCase(vLine[j]) == "nan")
                    fileData[nLine][j] = NAN;
                else if (toLowerCase(vLine[j]) == "inf")
                    fileData[nLine][j] = INFINITY;
                else if (toLowerCase(vLine[j]) == "-inf")
                    fileData[nLine][j] = -INFINITY;
                else
                    fileData[nLine][j] = StrToDb(vLine[j]);
            }

            nLine++;
        }

        // --> Wurde die Datei schon von NumeRe erzeugt? Dann sind die Kopfzeilen einfach zu finden <--
        if (nComment)
        {
            decodeTableHeads(vFileContents, nComment);
        }
    }

    void TextDataFile::writeFile()
    {
        open(ios::out | ios::trunc);

        writeHeader();

        size_t nNumberOfHeadlines = 1u;
        vector<size_t> vColumns =  calculateColumnWidths(nNumberOfHeadlines);
        writeTableHeads(vColumns, nNumberOfHeadlines);
        addSeparator(vColumns);
        writeTableContents(vColumns);
    }

    void TextDataFile::writeHeader()
    {
        string sBuild = AutoVersion::YEAR;
        sBuild += "-";
        sBuild += AutoVersion::MONTH;
        sBuild += "-";
        sBuild += AutoVersion::DATE;

        fFileStream << "#\n";
        fFileStream << "# " + _lang.get("OUTPUT_PRINTLEGAL_LINE1") << "\n";
        fFileStream << "# NumeRe: Framework für Numerische Rechnungen" << "\n";
        fFileStream << "#=============================================" << "\n";
        fFileStream << "# " + _lang.get("OUTPUT_PRINTLEGAL_LINE2", sVersion, sBuild) << "\n";
        fFileStream << "# " + _lang.get("OUTPUT_PRINTLEGAL_LINE3", sBuild.substr(0, 4)) << "\n";
        fFileStream << "#\n";
        fFileStream << "# " + _lang.get("OUTPUT_PRINTLEGAL_LINE4", getTimeStamp(false)) << "\n";
        fFileStream << "#\n";
        fFileStream << "# " + _lang.get("OUTPUT_PRINTLEGAL_STD") << "\n#" << endl;
    }

    void TextDataFile::writeTableHeads(const vector<size_t>& vColumnWidth, size_t nNumberOfLines)
    {
        fFileStream << "# ";

        for (size_t i = 0; i < nNumberOfLines; i++)
        {
            for (long long int j = 0; j < nCols; j++)
            {
                fFileStream.width(vColumnWidth[j]);
                fFileStream.fill(' ');
                fFileStream << getLineFromHead(j, i);

                if (j+1 < nCols)
                {
                    fFileStream.width(0);
                    fFileStream << "  ";
                }
            }

            fFileStream.width(0);
            fFileStream << "\n";
        }
    }

    void TextDataFile::writeTableContents(const vector<size_t>& vColumnWidth)
    {
        for (long long int i = 0; i < nRows; i++)
        {
            for (long long int j = 0; j < nCols; j++)
            {
                fFileStream.width(vColumnWidth[j]+2);
                fFileStream.fill(' ');
                fFileStream.precision(nPrecFields);

                if (isnan(fileData[i][j]))
                    fFileStream << "---";
                else
                    fFileStream << fileData[i][j];
            }

            fFileStream.width(0);
            fFileStream << "\n";
        }
    }

    void TextDataFile::addSeparator(const vector<size_t>& vColumnWidth)
    {
        fFileStream << "#=";

        for (size_t j = 0; j < vColumnWidth.size(); j++)
        {
            fFileStream.width(vColumnWidth[j]);
            fFileStream.fill('=');
            fFileStream << "=";

            if (j+1 < vColumnWidth.size())
                fFileStream << "==";
        }

        fFileStream << "\n";
    }

    void TextDataFile::decodeTableHeads(vector<string>& vFileContents, long long int nComment)
    {
		long long int _nHeadline = 0;
        string sCommentSign = "#";

        if (vFileContents.size() > 14)
            sCommentSign.append(vFileContents[13].length()-1, '=');

        if (nComment >= 15 && vFileContents[2].substr(0, 21) == "# NumeRe: Framework f")
        {
            for (size_t k = 13; k < vFileContents.size(); k++)
            {
                if (vFileContents[k] == sCommentSign)
                {
                    _nHeadline = 14;
                    break;
                }
            }
        }
        else if (nComment == 1)
        {
            for (size_t i = 0; i < vFileContents.size(); i++)
            {
                if (vFileContents[i][0] == '#')
                {
                    if (vFileContents[i].find(' ') != string::npos)
                    {
                        if (vFileContents[i][1] != ' ')
                        {
                            for (unsigned int n = 0; n < vFileContents[i].length(); n++)
                            {
                                if (vFileContents[i][n] != '#')
                                {
                                    if (vFileContents[i][n] != ' ')
                                        vFileContents[i] = vFileContents[i].substr(0, n) + " " + vFileContents[i].substr(n);

                                    break;
                                }
                            }
                        }

                        if (nCols == tokenize(vFileContents[i], " ", true).size())
                            _nHeadline = 1;
                    }

                    break;
                }
                else if (!isNumeric(vFileContents[i]))
                {
                    if (nCols == tokenize(vFileContents[i], " ", true).size())
                        _nHeadline = 1;
                }
            }
        }
        else if (vFileContents[0][0] == '#' || !isNumeric(vFileContents[0]))
        {
            for (long long int i = 0; i < vFileContents.size(); i++)
            {
                if (vFileContents[i][0] != '#' && isNumeric(vFileContents[i]))
                {
                    if (vFileContents[i-1][0] == '#')
                    {
                        if ((nCols > 1 && vFileContents[i-1].find(' ') != string::npos) || (nCols == 1 && vFileContents[i-1].length() > 1))
                        {
                            if (vFileContents[i-1][1] != ' ')
                            {
                                for (unsigned int n = 0; n < vFileContents[i-1].length(); n++)
                                {
                                    if (vFileContents[i-1][n] != '#')
                                    {
                                        if (vFileContents[i-1][n] != ' ')
                                            vFileContents[i-1] = vFileContents[i-1].substr(0, n) + " " + vFileContents[i-1].substr(n);

                                        break;
                                    }
                                }
                            }

                            if (tokenize(vFileContents[i-1], " ", true).size() == nCols+1) // wegen "#" an erster stelle
                            {
                                _nHeadline = i;
                                break;
                            }
                        }

                        if (i > 1 && vFileContents[i-2][0] == '#' && ((vFileContents[i-2].find(' ') != string::npos && nCols > 1) || (nCols == 1 && vFileContents[i-2].length() > 1)))
                        {
                            if (vFileContents[i-2][1] != ' ')
                            {
                                for (size_t n = 0; n < vFileContents[i-2].length(); n++)
                                {
                                    if (vFileContents[i-2][n] != '#')
                                    {
                                        if (vFileContents[i-2][n] != ' ')
                                            vFileContents[i-2] = vFileContents[i-2].substr(0, n) + " " + vFileContents[i-2].substr(n);

                                        break;
                                    }
                                }
                            }

                            if (tokenize(vFileContents[i-2], " ", true).size() == nCols+1)
                                _nHeadline = i-1;
                        }
                    }
                    else if (!isNumeric(vFileContents[i-1]))
                    {
                        if ((vFileContents[i-1].find(' ') != string::npos && nCols > 1) || (nCols == 1 && vFileContents[i-1].length() > 1))
                        {
                            if (vFileContents[i-1][1] != ' ')
                            {
                                for (size_t n = 0; n < vFileContents[i-1].length(); n++)
                                {
                                    if (vFileContents[i-1][n] != '#')
                                    {
                                        if (vFileContents[i-1][n] != ' ')
                                            vFileContents[i-1] = vFileContents[i-1].substr(0, n) + " " + vFileContents[i-1].substr(n);

                                        break;
                                    }
                                }
                            }

                            if (tokenize(vFileContents[i-1], " ", true).size() == nCols)
                            {
                                _nHeadline = i;
                                break;
                            }
                        }

                        if (i > 1 && vFileContents[i-2][0] == '#' && ((vFileContents[i-2].find(' ') != string::npos && nCols > 1) || (nCols == 1 && vFileContents[i-2].length() > 1)))
                        {
                            if (vFileContents[i-2][1] != ' ')
                            {
                                for (unsigned int n = 0; n < vFileContents[i-2].length(); n++)
                                {
                                    if (vFileContents[i-2][n] != '#')
                                    {
                                        if (vFileContents[i-2][n] != ' ')
                                            vFileContents[i-2] = vFileContents[i-2].substr(0,n) + " " + vFileContents[i-2].substr(n);

                                        break;
                                    }
                                }
                            }

                            if (tokenize(vFileContents[i-2], " ", true).size() == nCols)
                                _nHeadline = i-1;
                        }
                    }

                    break;
                }
            }
        }

        if (_nHeadline)
		{
            long long int n = 0; 		// zweite Zaehlvariable

            for (size_t i = 0; i < vFileContents.size(); i++)
            {
                if (vFileContents[i][0] == '#' || !isNumeric(vFileContents[i]))
                {
                    n++;	// Erst n erhoehen, da man zuvor auch mit 1 angefangen hat, zu zaehlen

                    if (n == _nHeadline) // Da ist sie ja, die Zeile...
                    {
                        for (size_t k = i+1; k < vFileContents.size(); k++)
                        {
                            // TAB-Replace ignorieren
                            if (vFileContents[k].find(" _ ") != string::npos
                                || (vFileContents[k].find_first_not_of(" #") != string::npos && vFileContents[k][vFileContents[k].find_first_not_of(" #")] == '_')
                                || vFileContents[k].back() == '_')
                            {
                                break;
                            }

                            if (vFileContents[k][0] != '#' && isNumeric(vFileContents[k]))
                                break;

                            if (vFileContents[k].substr(0, 4) == "#===" || vFileContents[k].substr(0, 5) == "# ===")
                                break;

                            if (vFileContents[k].length() == vFileContents[i].length())
                            {
                                for (unsigned int l = 0; l < vFileContents[k].length(); l++)
                                {
                                    if (vFileContents[i][l] != ' ' && vFileContents[k][l] == ' ')
                                        vFileContents[k][l] = '_';
                                }
                            }
                            else if (vFileContents[k].length() < vFileContents[i].length() && vFileContents[i].back() != ' ' && vFileContents[k].back() != ' ')
                            {
                                vFileContents[k].append(vFileContents[i].length() - vFileContents[k].length(), ' ');

                                for (unsigned int l = 0; l < vFileContents[k].length(); l++)
                                {
                                    if (vFileContents[i][l] != ' ' && vFileContents[k][l] == ' ')
                                        vFileContents[k][l] = '_';
                                }
                            }
                            else if (vFileContents[k].length() > vFileContents[i].length() && vFileContents[k].back() != ' ' && vFileContents[i].back() != ' ')
                            {
                                for (unsigned int l = 0; l < vFileContents[i].length(); l++)
                                {
                                    if (vFileContents[i][l] != ' ' && vFileContents[k][l] == ' ')
                                        vFileContents[k][l] = '_';
                                }
                            }
                            else
                                break;
                        }

                        bool bBreakSignal = false;
                        vector<string> vHeadline;
                        vHeadline.resize((unsigned int)(2*nCols), "");

                        for (size_t k = i; k < vFileContents.size(); k++)
                        {
                            if (vFileContents[k][0] != '#' && isNumeric(vFileContents[k]))
                                break;

                            if (vFileContents[k].substr(0, 4) == "#===" || vFileContents[k].substr(0, 5) == "# ===")
                                break;

                            vector<string> vLine = tokenize(vFileContents[k], " ", true);

                            if (vLine.front() == "#")
                                vLine.erase(vLine.begin());

                            for (size_t j = 0; j < vLine.size(); j++)
                            {
                                if (k == i)
                                    vHeadline.push_back("");

                                if (k != i && j >= vHeadline.size())
                                {
                                    bBreakSignal = true;
                                    break;
                                }

                                if (vLine[j].find_first_not_of('_') == string::npos)
                                    continue;

                                while (vLine[j].front() == '_')
                                    vLine[j].erase(0,1);

                                while (vLine[j].back() == '_')
                                    vLine[j].pop_back();

                                if (!vHeadline[j].length())
                                    vHeadline[j] = utf8parser(vLine[j]);
                                else
                                {
                                    vHeadline[j] += "\\n";
                                    vHeadline[j] += utf8parser(vLine[j]);
                                }
                            }

                            if (bBreakSignal)
                                break;
                        }

                        for (auto iter = vHeadline.begin(); iter != vHeadline.end(); ++iter)
                        {
                            if (!(iter->length()))
                            {
                                iter = vHeadline.erase(iter);
                                iter--;
                            }
                        }

                        if (vHeadline.size() <= nCols)
                        {
                            copyStringArray(&vHeadline[0], fileTableHeads, vHeadline.size());
                        }

                        return;
                    }
                }
            }
		}
    }

    pair<size_t, size_t> TextDataFile::calculateCellExtents(const std::string& sContents)
    {
        // x, y
        pair<size_t, size_t> pCellExtents(0u, 1u);
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

    vector<size_t> TextDataFile::calculateColumnWidths(size_t& nNumberOfLines)
    {
        vector<size_t> vColumnWidths;
        const size_t NUMBERFIELDLENGTH = nPrecFields + 7;

        for (long long int j = 0; j < nCols; j++)
        {
            pair<size_t, size_t> pCellExtents = calculateCellExtents(fileTableHeads[j]);
            vColumnWidths.push_back(max(NUMBERFIELDLENGTH, pCellExtents.first));

            if (nNumberOfLines < pCellExtents.second)
                nNumberOfLines = pCellExtents.second;
        }

        return vColumnWidths;
    }

    string TextDataFile::getLineFromHead(long long int nCol, size_t nLineNumber)
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


    //////////////////////////////////////////////
    // class NumeReDataFile
    //////////////////////////////////////////////
    //
    NumeReDataFile::NumeReDataFile(const string& filename)
        : GenericFile(filename),
        isLegacy(false), timeStamp(0), versionMajor(0), versionMinor(0),
        versionBuild(0)
    {
        //
    }

    NumeReDataFile::NumeReDataFile(const NumeReDataFile& file) : GenericFile(file)
    {
        isLegacy = file.isLegacy;
        timeStamp = file.timeStamp;
        sComment = file.sComment;
        versionMajor = file.versionMajor;
        versionMinor = file.versionMinor;
        versionBuild = file.versionBuild;
    }

    NumeReDataFile::~NumeReDataFile()
    {
        //
    }

    void NumeReDataFile::writeHeader()
    {
        writeNumField(AutoVersion::MAJOR);
        writeNumField(AutoVersion::MINOR);
        writeNumField(AutoVersion::BUILD);
        writeNumField(time(0));

        writeDummyHeader();

        writeNumField(fileVersionMajor);
        writeNumField(fileVersionMinor);
        writeStringField(getTableName());
        writeStringField(sComment);
        writeStringField("FTYPE=DOUBLE");
        writeDataBlock(nullptr, 0);
        writeStringField("FTYPE=DOUBLE");
        writeDataBlock(nullptr, 0);
        writeStringField("FTYPE=DOUBLE");
        writeDataBlock(nullptr, 0);
        writeStringField("DTYPE=DOUBLE");
        writeNumField(nRows);
        writeNumField(nCols);
    }

    void NumeReDataFile::writeDummyHeader()
    {
        writeNumField(1LL);
        writeNumField(1LL);
        writeStringField("THIS_FILE_NEEDS_A_NEWER_VERSION_OF_NUMERE");
        long long int appZeros = 0;
        double data = NAN;
        bool valid = false;
        fFileStream.write((char*)&appZeros, sizeof(long long int));
        fFileStream.write((char*)&data, sizeof(double));
        fFileStream.write((char*)&valid, sizeof(bool));
    }

    void NumeReDataFile::writeFile()
    {
        if (!is_open())
            open(ios::binary | ios::out | ios::trunc);

        writeHeader();
        writeStringBlock(fileTableHeads, nCols);
        writeDataArray(fileData, nRows, nCols);
    }

    void NumeReDataFile::readHeader()
    {
        versionMajor = readNumField<long int>();
        versionMinor = readNumField<long int>();
        versionBuild = readNumField<long int>();
        timeStamp = readNumField<time_t>();

        if (versionMajor * 100 + versionMinor * 10 + versionBuild <= 111)
        {
            isLegacy = true;
            nRows = readNumField<long long int>();
            nCols = readNumField<long long int>();
            return;
        }

        skipDummyHeader();

        short fileVerMajor = readNumField<short>();
        short fileVerMinor = readNumField<short>();

        if (fileVerMajor > fileVersionMajor)
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        sTableName = readStringField();
        sComment = readStringField();

        long long int size;
        string type;
        void* data = readGenericField(type, size);
        deleteGenericData(data, type);

        data = readGenericField(type, size);
        deleteGenericData(data, type);

        data = readGenericField(type, size);
        deleteGenericData(data, type);

        string dataType = readStringField();
        nRows = readNumField<long long int>();
        nCols = readNumField<long long int>();
    }

    void NumeReDataFile::skipDummyHeader()
    {
        readNumField<long long int>();
        readNumField<long long int>();
        readStringField();
        long long int appZeros = 0;
        double data = NAN;
        bool valid = false;
        fFileStream.read((char*)&appZeros, sizeof(long long int));
        fFileStream.read((char*)&data, sizeof(double));
        fFileStream.read((char*)&valid, sizeof(bool));
    }

    void NumeReDataFile::readFile()
    {
        if (!is_open())
            open(ios::binary | ios::in);

        readHeader();

        if (isLegacy)
        {
            readLegacyFormat();
            return;
        }

        long long int stringBlockSize;
        long long int dataarrayrows;
        long long int dataarraycols;
        fileTableHeads = readStringBlock(stringBlockSize);
        fileData = readDataArray(dataarrayrows, dataarraycols);
    }

    void NumeReDataFile::readLegacyFormat()
    {
        size_t nLength = 0;
        bool* bValidEntry = new bool[nCols];
        char** cHeadLine = new char*[nCols];
        long long int* nAppendedZeros = new long long int[nCols];

        createStorage();

        for (long long int i = 0; i < nCols; i++)
        {
            nLength = 0;
            fFileStream.read((char*)&nLength, sizeof(size_t));
            cHeadLine[i] = new char[nLength];
            fFileStream.read(cHeadLine[i], sizeof(char)*nLength);
            fileTableHeads[i].resize(nLength-1);

            for (unsigned int j = 0; j < nLength-1; j++)
            {
                fileTableHeads[i][j] = cHeadLine[i][j];
            }
        }

        fFileStream.read((char*)nAppendedZeros, sizeof(long long int)*nCols);

        for (long long int i = 0; i < nRows; i++)
        {
            fFileStream.read((char*)fileData[i], sizeof(double)*nCols);
        }

        for (long long int i = 0; i < nRows; i++)
        {
            fFileStream.read((char*)bValidEntry, sizeof(bool)*nCols);
            for (long long int j = 0; j < nCols; j++)
            {
                if (!bValidEntry[j])
                    fileData[i][j] = NAN;
            }
        }

        for (long long int i = 0; i < nCols; i++)
            delete[] cHeadLine[i];

        delete[] cHeadLine;
        delete[] bValidEntry;
        delete[] nAppendedZeros;
    }

    void* NumeReDataFile::readGenericField(std::string& type, long long int& size)
    {
        type = readStringField();
        type.erase(0, type.find('=')+1);
        size = 0;

        if (type == "DOUBLE")
        {
            double* data = readDataBlock(size);
            return (void*)data;
        }
        else if (type == "STRING")
        {
            string* data = readStringBlock(size);
            return (void*)data;
        }

        return nullptr;
    }

    void NumeReDataFile::deleteGenericData(void* data, const string& type)
    {
        if (data)
        {
            if (type == "DOUBLE")
                delete[] (double*)data;
            else if (type == "STRING")
                delete[] (string*)data;
        }
    }

    NumeReDataFile& NumeReDataFile::operator=(NumeReDataFile& file)
    {
        assign(file);
        isLegacy = file.isLegacy;
        timeStamp = file.timeStamp;
        sComment = file.sComment;
        versionMajor = file.versionMajor;
        versionMinor = file.versionMinor;
        versionBuild = file.versionBuild;

        return *this;
    }

    std::string NumeReDataFile::getVersionString()
    {
        return toString(versionMajor) + "." + toString(versionMinor) + "." + toString(versionBuild);
    }


    //////////////////////////////////////////////
    // class CacheFile
    //////////////////////////////////////////////
    //
    CacheFile::CacheFile(const string& filename) : NumeReDataFile(filename), nIndexPos(0u)
    {
        //
    }

    CacheFile::~CacheFile()
    {
        if (nIndexPos && vFileIndex.size())
        {
            seekp(nIndexPos);
            writeNumBlock(&vFileIndex[0], vFileIndex.size());
        }
    }

    void CacheFile::reset()
    {
        sComment.clear();
        sTableName.clear();
        clearStorage();
    }

    void CacheFile::readSome()
    {
        reset();

        if (vFileIndex.size() && !fFileStream.eof())
            readFile();
    }

    void CacheFile::writeSome()
    {
        if (vFileIndex.size())
        {
            for (size_t i = 0; i < vFileIndex.size(); i++)
            {
                if (!vFileIndex[i])
                {
                    vFileIndex[i] = tellp();
                    break;
                }
            }

            writeFile();
        }
    }

    void CacheFile::readCacheHeader()
    {
        open(ios::binary | ios::in);

        versionMajor = readNumField<long int>();
        versionMinor = readNumField<long int>();
        versionBuild = readNumField<long int>();
        timeStamp = readNumField<time_t>();

        if (readStringField() != "NUMERECACHEFILE")
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        short fileVerMajor = readNumField<short>();
        short fileVerMinor = readNumField<short>();

        if (fileVerMajor > fileVersionMajor)
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        size_t nNumberOfTables = readNumField<size_t>();

        vFileIndex = vector<size_t>(nNumberOfTables, 0u);
        long long int size = 0;
        size_t* nIndex = readNumBlock<size_t>(size);
        copyArray(nIndex, &vFileIndex[0], nNumberOfTables);
        delete[] nIndex;
    }

    void CacheFile::writeCacheHeader()
    {
        open(ios::binary | ios::out | ios::trunc);

        writeNumField(AutoVersion::MAJOR);
        writeNumField(AutoVersion::MINOR);
        writeNumField(AutoVersion::BUILD);
        writeNumField(time(0));
        writeStringField("NUMERECACHEFILE");
        writeNumField(fileVersionMajor);
        writeNumField(fileVersionMinor);
        writeNumField(vFileIndex.size());
        nIndexPos = tellp();
        writeNumBlock(&vFileIndex[0], vFileIndex.size());
    }


    //////////////////////////////////////////////
    // class CassyLabx
    //////////////////////////////////////////////
    //
    CassyLabx::CassyLabx(const string& filename) : GenericFile(filename)
    {
        //
    }

    CassyLabx::~CassyLabx()
    {
        //
    }

    void CassyLabx::readFile()
    {
        open(ios::in);

        string sLabx = "";
        string sLabx_substr = "";
        string** sDataMatrix = 0;
        long long int nLine = 0;

        while (!fFileStream.eof())
        {
            getline(fFileStream, sLabx_substr);
            StripSpaces(sLabx_substr);
            sLabx += sLabx_substr;
        }

        if (!sLabx.length() || sLabx.find("<allchannels count=") == string::npos)
        {
            throw SyntaxError(SyntaxError::DATAFILE_NOT_EXIST, "", SyntaxError::invalid_position, sFileName);
        }

        sLabx_substr = sLabx.substr(sLabx.find("<allchannels count="));
        sLabx_substr = sLabx_substr.substr(sLabx_substr.find("=\"")+2, sLabx_substr.find("\">")-sLabx_substr.find("=\"")-2);

        nCols = StrToInt(sLabx_substr);

        if (!nCols)
        {
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);
        }

        vector<string> vHeadLines;
        vector<string> vCols;

        sLabx_substr = sLabx.substr(sLabx.find("<allchannels"), sLabx.find("</allchannels>")-sLabx.find("<allchannels"));
        sLabx_substr = sLabx_substr.substr(sLabx_substr.find("<channels"));

        for (unsigned int i = 0; i < nCols; i++)
        {
            vCols.push_back(sLabx_substr.substr(sLabx_substr.find("<values"), sLabx_substr.find("</channel>")-sLabx_substr.find("<values")));

            if (sLabx_substr.find("<unit />") != string::npos && sLabx_substr.find("<unit />") < sLabx_substr.find("<unit>"))
            {
                vHeadLines.push_back(sLabx_substr.substr(sLabx_substr.find("<quantity>")+10, sLabx_substr.find("</quantity>")-sLabx_substr.find("<quantity>")-10));
            }
            else
            {
                vHeadLines.push_back(sLabx_substr.substr(sLabx_substr.find("<quantity>")+10, sLabx_substr.find("</quantity>")-sLabx_substr.find("<quantity>")-10)
                    + "_[" + sLabx_substr.substr(sLabx_substr.find("<unit>")+6, sLabx_substr.find("</unit>")-sLabx_substr.find("<unit>")-6) + "]");
            }

            vHeadLines.back() = utf8parser(vHeadLines.back());

            sLabx_substr = sLabx_substr.substr(sLabx_substr.find("</channels>")+11);

            if (StrToInt(vCols[i].substr(vCols[i].find("count=\"")+7, vCols[i].find("\">")-vCols[i].find("count=\"")-7)) > nLine)
                nLine = StrToInt(vCols[i].substr(vCols[i].find("count=\"")+7, vCols[i].find("\">")-vCols[i].find("count=\"")-7));
        }

        if (!nLine)
        {
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);
        }

        nRows = nLine;

        sDataMatrix = new string*[nRows];

        for (long long int i = 0; i < nRows; i++)
        {
            sDataMatrix[i] = new string[nCols];
        }

        createStorage();

        copyStringArray(&vHeadLines[0], fileTableHeads, vHeadLines.size());

        unsigned int nElements = 0;

        for (long long int i = 0; i < nCols; i++)
        {
            if (vCols[i].find("<values count=\"0\" />") == string::npos)
            {
                nElements = StrToInt(vCols[i].substr(vCols[i].find('"')+1, vCols[i].find('"', vCols[i].find('"')+1)-1-vCols[i].find('"')));
                vCols[i] = vCols[i].substr(vCols[i].find('>')+1);

                for (long long int j = 0; j < nRows; j++)
                {
                    if (j >= nElements)
                    {
                        sDataMatrix[j][i] = "<value />";
                    }
                    else
                    {
                        sDataMatrix[j][i] = vCols[i].substr(vCols[i].find("<value"), vCols[i].find('<', vCols[i].find('/'))-vCols[i].find("<value"));
                        vCols[i] = vCols[i].substr(vCols[i].find('>', vCols[i].find('/'))+1);
                    }
                }
            }
            else
            {
                for (long long int j = 0; j < nRows; j++)
                {
                    sDataMatrix[j][i] = "<value />";
                }
            }
        }

        for (long long int i = 0; i < nRows; i++)
        {
            for (long long int j = 0; j < nCols; j++)
            {
                if (sDataMatrix[i][j] == "<value />")
                {
                    fileData[i][j] = NAN;
                }
                else
                {
                    fileData[i][j] = StrToDb(sDataMatrix[i][j].substr(7, sDataMatrix[i][j].find('<', 7)-7));
                }
            }
        }

        for (long long int i = 0; i < nRows; i++)
        {
            delete[] sDataMatrix[i];
        }

        delete[] sDataMatrix;

        return;

    }


    //////////////////////////////////////////////
    // class CommaSeparatedValues
    //////////////////////////////////////////////
    //
    CommaSeparatedValues::CommaSeparatedValues(const string& filename) : GenericFile(filename)
    {
        //
    }

    CommaSeparatedValues::~CommaSeparatedValues()
    {
        //
    }

    void CommaSeparatedValues::readFile()
    {
        open(ios::in);

		char cSep = 0;

		// --> Benoetigte temporaere Variablen initialisieren <--
		long long int nLine = 0;
		long long int nComment = 0;
		vector<string> vFileData = readTextFile(true);
		vector<string> vHeadLine;

		nLine = vFileData.size();

		if (!nLine)
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

		cSep = findSeparator(vFileData);

		if (!cSep)
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        countColumns(vFileData, cSep);

        vHeadLine.resize(nCols);

        string sValidSymbols = "0123456789.,;-+eE INFAinfa";
        sValidSymbols += cSep;

        for (unsigned int j = 0; j < vFileData[0].length(); j++)
        {
            if (sValidSymbols.find(vFileData[0][j]) == string::npos)
            {
                string __sLine = vFileData[0];

                for (unsigned int n = 0; n < nCols-1; n++)
                {
                    vHeadLine[n] = utf8parser(__sLine.substr(0, __sLine.find(cSep)));
                    StripSpaces(vHeadLine.back());

                    __sLine = __sLine.substr(__sLine.find(cSep)+1);
                }

                vHeadLine.back() = utf8parser(__sLine);

                for (unsigned int n = 0; n < nCols; n++)
                {
                    for (unsigned int k = 0; k < vHeadLine[n].length(); k++)
                    {
                        if (sValidSymbols.find(vHeadLine[n][k]) == string::npos)
                            break;

                        if (k == vHeadLine[n].length()-1)
                        {
                            nComment--;
                            break;
                        }
                    }

                    if (nComment < 0)
                    {
                        for (unsigned int k = 0; k < nCols; k++)
                        {
                            vHeadLine[k] = "";
                        }

                        nComment = -1;
                        break;
                    }
                }

                if (!nComment)
                    vFileData[0] = "";

                nComment++;
            }
        }

        if (nComment) // Problem: Offenbar scheinen die Eintraege nur aus Text zu bestehen --> Anderer Loesungsweg
            nRows = nLine - 1;
        else
            nRows = nLine;	// Die maximale Zahl der Zeilen ergibt die noetige Zahl der Zeilen

        createStorage();

		if (nComment)
            copyStringArray(&vHeadLine[0], fileTableHeads, vHeadLine.size());
		else
		{
			if (nCols == 1)		// Hat nur eine Spalte: Folglich verwenden wir logischerweise den Dateinamen
			{
                if (sFileName.find('/') == string::npos)
                    fileTableHeads[0] = sFileName.substr(0, sFileName.rfind('.'));
                else
                    fileTableHeads[0] = sFileName.substr(sFileName.rfind('/')+1, sFileName.rfind('.')-1-sFileName.rfind('/'));
			}
		}

		// --> Hier werden die Strings in Tokens zerlegt <--
		for (size_t i = nComment; i < vFileData.size(); i++)
        {
            if (!vFileData[i].length())
                continue;

            vector<string> vTokens = tokenize(vFileData[i], string(1, cSep));

            for (size_t j = 0; j < vTokens.size(); j++)
            {
                if (!vTokens[j].length()
                    || vTokens[j] == "---"
                    || vTokens[j] == "NaN"
                    || vTokens[j] == "NAN"
                    || vTokens[j] == "nan")
                {
                    fileData[i][j] = NAN;
                }
                else if (vTokens[j] == "inf")
                    fileData[i][j] = INFINITY;
                else if (vTokens[j] == "-inf")
                    fileData[i][j] = -INFINITY;
                else
                {
                    for (size_t k = 0; k < vTokens[j].length(); k++)
                    {
                        if (sValidSymbols.find(vTokens[j][k]) == string::npos)
                        {
                            if (fileTableHeads[j].length())
                                fileTableHeads[j] += "\\n";

                            fileTableHeads[j] += vTokens[j];
                            fileData[i][j] = NAN;
                            break;
                        }

                        if (k+1 == vTokens[j].length())
                        {
                            replaceDecimalSign(vTokens[j]);
                            fileData[i][j] = StrToDb(vTokens[j]);
                        }
                    }
                }
            }
        }
    }

    void CommaSeparatedValues::writeFile()
    {
        open(ios::out | ios::trunc);

        for (long long int j = 0; j < nCols; j++)
        {
            fFileStream << fileTableHeads[j] + ",";
        }

        fFileStream << "\n";
        fFileStream.precision(nPrecFields);

        for (long long int i = 0; i < nRows; i++)
        {
            for (long long int j = 0; i < nCols; j++)
            {
                if (!isnan(fileData[i][j]))
                    fFileStream << fileData[i][j];

                fFileStream << ",";
            }

            fFileStream << "\n";
        }

        fFileStream.flush();
    }

    char CommaSeparatedValues::findSeparator(const vector<string>& vTextData)
    {
        char cSep = 0;

        if (vTextData[0].find('.') != string::npos && vTextData[0].find(',') != string::npos && vTextData[0].find('\t') != string::npos)
		{
            cSep = ',';
        }
		else if (vTextData[0].find(';') != string::npos && (vTextData[0].find(',') != string::npos || vTextData[0].find('.') != string::npos) && vTextData[0].find('\t') != string::npos)
		{
            cSep = ';';
		}
		else if (vTextData[0].find('\t') != string::npos)
        {
            cSep = '\t';
        }
        else if (vTextData[0].find(';') != string::npos)
        {
            cSep = ';';
        }
        else if (vTextData[0].find(',') != string::npos)
        {
            cSep = ',';
        }
        else if (vTextData[0].find(' ') != string::npos && vTextData.size() > 1 && vTextData[1].find(' ') != string::npos)
        {
            cSep = ' ';
        }
        else
        {
            if (vTextData[0].find(',') != string::npos)
                cSep = ',';
            else if (vTextData[0].find(';') != string::npos)
                cSep = ';';
            else if (vTextData[0].find('\t') != string::npos)
                cSep = '\t';
            else if (vTextData[0].find(' ') != string::npos)
                cSep = ' ';

            size_t cols = 0;

            for (size_t i = 0; i < vTextData.size(); i++)
            {
                size_t nCol = 1;

                for (size_t j = 0; j < vTextData[i].length(); j++)
                {
                    if (vTextData[i][j] == cSep)
                    {
                        nCol++;
                    }
                }

                if (!cols)
                    cols = nCol;

                else if (nCol != cols)
                {
                    if (cSep == ',')
                        cSep = ';';
                    else if (cSep == ';')
                        cSep = ',';
                    else if (cSep == '\t')
                        cSep = ' ';
                    else if (cSep == ' ')
                        cSep = '\t';

                    cols = 0;
                }
                else
                {
                    return cSep;
                }

                if (i+1 == vTextData.size())
                {
                    return 0;
                }
            }
        }

        return cSep;
    }

    void CommaSeparatedValues::countColumns(const std::vector<std::string>& vTextData, char& cSep)
    {
        long long int nCol;

        for (size_t i = 0; i < vTextData.size(); i++)
        {
            nCol = 1;

            for (size_t j = 0; j < vTextData[i].length(); j++)
            {
                if (vTextData[i][j] == cSep)
                {
                    nCol++;
                }
            }

            if (!nCols)
                nCols = nCol;

            else if (nCol != nCols)
            {
                if (cSep == ',')
                    cSep = ';';
                else if (cSep == ';')
                    cSep = ',';
                else if (cSep == '\t')
                    cSep = ' ';
                else if (cSep == ' ')
                    cSep = '\t';

                nCols = 0;
            }
            else
                return;

            if (i+1 == vTextData.size())
                throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);
        }
    }


    //////////////////////////////////////////////
    // class JcampDX
    //////////////////////////////////////////////
    //
    JcampDX::JcampDX(const string& filename) : GenericFile(filename)
    {
        //
    }

    JcampDX::~JcampDX()
    {
        //
    }

    void JcampDX::readFile()
    {
        open(ios::in);

		vector<long long int> vComment;
		vector<long long int> vCols;
		vector<double> vLine;
		vector<vector<string> > vDataMatrix;

		if (!fFileStream.good())
            throw SyntaxError(SyntaxError::DATAFILE_NOT_EXIST, sFileName, SyntaxError::invalid_position, sFileName);

		vector<string> vFileContents = readTextFile(true);

		if (!vFileContents.size())
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

		for (size_t i = 0; i < vFileContents.size(); i++)
        {
            stripTrailingSpaces(vFileContents[i]);
            parseLabel(vFileContents[i]);
            /*if (vFileContents[i].find("##JCAMPDX=") != string::npos)
            {
                if (StrToDb(vFileContents[i].substr(vFileContents[i].find("##JCAMPDX=")+11)) > 5.1)
                {
                    NumeReKernel::print(LineBreak("HINWEIS: Die Versionsnummer dieses JCAMP-DX-Spektrums ist größer als 5.1. Es besteht die Möglichkeit, dass NumeRe diese Datei nicht korrekt lesen kann. Es wird trotzdem versucht, die Daten zu interpretieren.", _option));
                }
            }*/

            if (vFileContents[i].substr(0, 6) == "##END=")
            {
                vFileContents.erase(vFileContents.begin()+i+1, vFileContents.end());
                vDataMatrix.push_back(vFileContents);
                break;
            }
        }

        for (unsigned int i = 0; i < vDataMatrix.size(); i++)
        {
            for (unsigned int j = 0; j < vDataMatrix[i].size(); j++)
            {
                if (vDataMatrix[i][j].substr(0,2) == "##")
                {
                    if (vComment.size() == i)
                        vComment.push_back(1);
                    else
                        vComment[i]++;
                }
            }
        }

        nRows = vDataMatrix[0].size() - vComment[0];

        for (unsigned int i = 0; i < vDataMatrix.size(); i++)
        {
            if (vDataMatrix[i].size() - vComment[i] > nRows)
                nRows = vDataMatrix[i].size() - vComment[i];
        }

        for (unsigned int i = 0; i < vDataMatrix.size(); i++)
        {
            vLine = parseLine(vDataMatrix[i][vComment[i]-1]);
            vCols.push_back(vLine.size());
        }

        for (unsigned int i = 0; i < vCols.size(); i++)
        {
            nCols += vCols[i];
        }

        createStorage();

        for (long long int i = 0; i < vDataMatrix.size(); i++)
        {
            double dXFactor = 1.0;
            double dYFactor = 1.0;

            string sXUnit = "";
            string sYUnit = "";

            string sDataType = "";
            string sXYScheme = "";

            for (long long int j = 0; j < vComment[i] - 1; j++)
            {
                if (vDataMatrix[i][j].find("$$") != string::npos)
                    vDataMatrix[i][j].erase(vDataMatrix[i][j].find("$$"));

                if (vDataMatrix[i][j].substr(0,10) == "##XFACTOR=")
                    dXFactor = StrToDb(vDataMatrix[i][j].substr(10));

                if (vDataMatrix[i][j].substr(0,10) == "##YFACTOR=")
                    dYFactor = StrToDb(vDataMatrix[i][j].substr(10));

                if (vDataMatrix[i][j].substr(0,9) == "##XUNITS=")
                {
                    sXUnit = vDataMatrix[i][j].substr(9);
                    StripSpaces(sXUnit);

                    if (toUpperCase(sXUnit) == "1/CM")
                        sXUnit = "Wellenzahl k [cm^-1]";
                    else if (toUpperCase(sXUnit) == "MICROMETERS")
                        sXUnit = "Wellenlänge lambda [mu m]";
                    else if (toUpperCase(sXUnit) == "NANOMETERS")
                        sXUnit = "Wellenlänge lambda [nm]";
                    else if (toUpperCase(sXUnit) == "SECONDS")
                        sXUnit = "Zeit t [s]";
                    else if (toUpperCase(sXUnit) == "1/S" || toUpperCase(sXUnit) == "1/SECONDS")
                        sXUnit = "Frequenz f [Hz]";
                    else
                        sXUnit = "[" + sXUnit + "]";

                }

                if (vDataMatrix[i][j].substr(0,9) == "##YUNITS=")
                {
                    sYUnit = vDataMatrix[i][j].substr(9);
                    StripSpaces(sYUnit);

                    if (toUpperCase(sYUnit) == "TRANSMITTANCE")
                        sYUnit = "Transmission";
                    else if (toUpperCase(sYUnit) == "REFLECTANCE")
                        sYUnit = "Reflexion";
                    else if (toUpperCase(sYUnit) == "ABSORBANCE")
                        sYUnit = "Absorbtion";
                    else if (toUpperCase(sYUnit) == "KUBELKA-MUNK")
                        sYUnit = "Kubelka-Munk";
                    else if (toUpperCase(sYUnit) == "ARBITRARY UNITS" || sYUnit.substr(0,9) == "Intensity")
                        sYUnit = "Intensität";
                }

                if (vDataMatrix[i][j].substr(0,11) == "##DATATYPE=")
                {
                    sDataType = vDataMatrix[i][j].substr(11);
                    StripSpaces(sDataType);
                }

                if (vDataMatrix[i][j].substr(0,11) == "##XYPOINTS=")
                {
                    sXYScheme = vDataMatrix[i][j].substr(11);
                    StripSpaces(sXYScheme);
                }
            }

            for (long long int j = vComment[i] - 1; j < vDataMatrix[i].size() - 1; j++)
            {
                if (vDataMatrix[i][j].substr(0, 2) == "##")
                    continue;

                if (vDataMatrix[i][j].substr(0, 6) == "##END=")
                    break;

                if (j - vComment[i] + 1 == nRows)
                    break;

                vLine = parseLine(vDataMatrix[i][j]);

                for (unsigned int k = 0; k < vLine.size(); k++)
                {
                    if (k == vCols[i])
                        break;

                    if (j == vComment[i]-1)
                    {
                        if (i)
                        {
                            fileTableHeads[vCols[i-1]+k] = (k % 2 ? sYUnit : sXUnit);
                        }
                        else
                        {
                            fileTableHeads[k] = (k % 2 ? sYUnit : sXUnit);
                        }
                    }

                    if (i)
                    {
                        if (k+1 == vDataMatrix[i][j].length())
                            fileData[j-vComment[i]+1][vCols[i-1]+k] = vLine[k] * (k % 2 ? dYFactor : dXFactor);
                        else
                            fileData[j-vComment[i]+1][vCols[i-1]+k] = vLine[k] * (k % 2 ? dYFactor : dXFactor);
                    }
                    else
                    {
                        if (k+1 == vDataMatrix[i][j].length())
                            fileData[j-vComment[i]+1][k] = vLine[k] * (k % 2 ? dYFactor : dXFactor);
                        else
                            fileData[j-vComment[i]+1][k] = vLine[k] * (k % 2 ? dYFactor : dXFactor);
                    }
                }
            }
        }
    }

    void JcampDX::parseLabel(string& sLine)
    {
        if (sLine.find("##") == string::npos || sLine.find('=') == string::npos)
            return;

        for (unsigned int i = 0; i < sLine.length(); i++)
        {
            if (sLine[i] == ' ')
            {
                sLine.erase(i, 1);
                i--;
            }

            if (sLine[i] == '-')
            {
                sLine.erase(i, 1);
                i--;
            }

            if (sLine[i] == '_')
            {
                sLine.erase(i, 1);
                i--;
            }

            if (sLine[i] >= 'a' && sLine[i] <= 'z')
                sLine[i] += 'A'-'a';

            if (sLine[i] == '=')
                break;
        }
    }

    vector<double> JcampDX::parseLine(const string& sLine)
    {
        vector<double> vLine;
        string sValue = "";
        const string sNumericChars = "0123456789.+-";

        for (unsigned int i = 0; i < sLine.length(); i++)
        {
            if ((sLine[i] >= '0' && sLine[i] <= '9') || sLine[i] == '.')
                sValue += sLine[i];
            else if (sValue.length()
                && (sLine[i] == 'e' || sLine[i] == 'E')
                && sNumericChars.find(sValue[0]) != string::npos)
                sValue += sLine[i];
            else if (sValue.length()
                && (sLine[i] == '+' || sLine[i] == '-')
                && sNumericChars.find(sValue[0]) != string::npos
                && (sValue[sValue.length()-1] == 'e' || sValue[sValue.length()-1] == 'E'))
                sValue += sLine[i];
            else
            {
                if (sValue.length())
                {
                    if ((sValue[0] >= 'J' && sValue[0] <= 'R'))
                    {
                        sValue[0] = toString(sValue[0]-'J'+1)[0];
                        vLine.push_back(vLine[vLine.size()-1]+StrToDb(sValue));
                    }
                    else if ((sValue[0] >= 'j' && sValue[0] <= 'r'))
                    {
                        sValue[0] = toString(sValue[0]-'j'+1)[0];
                        vLine.push_back(vLine[vLine.size()-1]-StrToDb(sValue));
                    }
                    else if (sValue[0] == '%')
                    {
                        sValue[0] = '0';
                        vLine.push_back(vLine[vLine.size()-1]+StrToDb(sValue));
                    }
                    else if ((sValue[0] >= 'S' && sValue[0] >= 'Z') || sValue[0] == 's')
                    {
                        if (sValue[0] == 's')
                        {
                            for (int j = 0; j < 9; j++)
                                vLine.push_back(vLine[vLine.size()-1]);
                        }
                        else
                        {
                            for (int j = 0; j <= sValue[0]-'S'; j++)
                                vLine.push_back(vLine[vLine.size()-1]);
                        }
                    }
                    else
                        vLine.push_back(StrToDb(sValue));

                    sValue.clear();
                }

                if (sLine[i] >= 'A' && sLine[i] <= 'I')
                    sValue += toString(sLine[i]-'A'+1)[0];
                else if (sLine[i] >= 'a' && sLine[i] <= 'i')
                    sValue += toString(sLine[i]-'a'+1)[0];
                else if ((vLine.size()
                    && ((sLine[i] >= 'J' && sLine[i] <= 'R')
                        || (sLine[i] >= 'j' && sLine[i] <= 'r')
                        || sLine[i] == '%'
                        || (sLine[i] >= 'S' && sLine[i] <= 'Z')
                        || sLine[i] == 's'))
                        || sLine[i] == '+'
                        || sLine[i] == '-')
                    sValue += sLine[i];
            }
        }

        if (sValue.length())
        {
            if ((sValue[0] >= 'J' && sValue[0] <= 'R'))
            {
                sValue[0] = toString(sValue[0]-'J'+1)[0];
                vLine.push_back(vLine[vLine.size()-1]+StrToDb(sValue));
            }
            else if ((sValue[0] >= 'j' && sValue[0] <= 'r'))
            {
                sValue[0] = toString(sValue[0]-'j'+1)[0];
                vLine.push_back(vLine[vLine.size()-1]-StrToDb(sValue));
            }
            else if (sValue[0] == '%')
            {
                sValue[0] = '0';
                vLine.push_back(vLine[vLine.size()-1]+StrToDb(sValue));
            }
            else if ((sValue[0] >= 'S' && sValue[0] >= 'Z') || sValue[0] == 's')
            {
                if (sValue[0] == 's')
                {
                    for (int j = 0; j < 9; j++)
                        vLine.push_back(vLine[vLine.size()-1]);
                }
                else
                {
                    for (int j = 0; j <= sValue[0]-'S'; j++)
                        vLine.push_back(vLine[vLine.size()-1]);
                }
            }
            else
                vLine.push_back(StrToDb(sValue));

            sValue.clear();
        }

        return vLine;
    }


    //////////////////////////////////////////////
    // class OpenDocumentSpreadSheet
    //////////////////////////////////////////////
    //
    OpenDocumentSpreadSheet::OpenDocumentSpreadSheet(const string& filename) : GenericFile(filename)
    {
        //
    }

    OpenDocumentSpreadSheet::~OpenDocumentSpreadSheet()
    {
        //
    }

    void OpenDocumentSpreadSheet::readFile()
    {
        string sODS = "";
        string sODS_substr = "";
        vector<string> vTables;
        vector<string> vMatrix;
        long long int nCommentLines = 0;
        long long int nMaxCols = 0;

        sODS = getZipFileItem("content.xml");

        if (!sODS.length())
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        sODS.erase(0,sODS.find("<office:spreadsheet>"));

        if (!sODS.length())
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        sODS.erase(0,sODS.find("<table:table "));

        while (sODS.size() && sODS.find("<table:table ") != string::npos && sODS.find("</table:table>") != string::npos)
        {
            vTables.push_back(sODS.substr(sODS.find("<table:table "), sODS.find("</table:table>")+14-sODS.find("<table:table ")));
            sODS.erase(sODS.find("<table:table "), sODS.find("</table:table>")+14-sODS.find("<table:table "));
        }

        if (!vTables.size())
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        for (unsigned int i = 0; i < vTables.size(); i++)
        {
            unsigned int nPos = 0;
            unsigned int nCount = 0;
            long long int _nCols = 0;
            string sLine = "";

            while (vTables[i].find("<table:table-row ", nPos) != string::npos && vTables[i].find("</table:table-row>", nPos) != string::npos)
            {
                nPos = vTables[i].find("<table:table-row ", nPos);
                sLine = vTables[i].substr(nPos, vTables[i].find("</table:table-row>", nPos)+18-nPos);
                sLine = expandLine(sLine);

                if (sLine.length())
                {
                    if (!i)
                        vMatrix.push_back(sLine);
                    else
                    {
                        if (vMatrix.size() <= nCount)
                        {
                            vMatrix.push_back("<>");

                            for (long long int n = 1; n < nMaxCols; n++)
                                vMatrix[nCount] += "<>";

                            vMatrix[nCount] += sLine;
                        }
                        else
                            vMatrix[nCount] += sLine;

                        nCount++;
                    }
                }

                nPos++;
            }

            for (unsigned int j = 0; j < vMatrix.size(); j++)
            {
                _nCols = 0;

                for (unsigned int n = 0; n < vMatrix[j].length(); n++)
                {
                    if (vMatrix[j][n] == '<')
                        _nCols++;
                }

                if (_nCols > nMaxCols)
                    nMaxCols = _nCols;
            }

            for (unsigned int j = 0; j < vMatrix.size(); j++)
            {
                _nCols = 0;

                for (unsigned int n = 0; n < vMatrix[j].length(); n++)
                {
                    if (vMatrix[j][n] == '<')
                        _nCols++;
                }

                if (_nCols < nMaxCols)
                {
                    for (long long int k = _nCols; k < nMaxCols; k++)
                        vMatrix[j] += "<>";
                }
            }
        }

        for (unsigned int i = 0; i < vMatrix.size(); i++)
        {
            while (vMatrix[i].find(' ') != string::npos)
                vMatrix[i].replace(vMatrix[i].find(' '), 1, "_");
        }

        if (!vMatrix.size() || !nMaxCols)
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        for (unsigned int i = 0; i < vMatrix.size(); i++)
        {
            bool bBreak = false;

            for (unsigned int j = 0; j < vMatrix[i].length(); j++)
            {
                if (vMatrix[i][j] == '<' && vMatrix[i][j+1] != '>')
                {
                    if (isNumeric(vMatrix[i].substr(j+1, vMatrix[i].find('>',j)-j-1)))
                    {
                        bBreak = true;
                        break;
                    }
                }

                if (j == vMatrix[i].length()-1)
                    nCommentLines++;
            }

            if (bBreak)
                break;
        }

        // Allokation
        nRows = vMatrix.size() - nCommentLines;
        nCols = nMaxCols;
        createStorage();

        // Interpretation
        unsigned int nPos = 0;

        for (long long int i = 0; i < nCommentLines; i++)
        {
            nPos = 0;

            for (long long int j = 0; j < nCols; j++)
            {
                nPos = vMatrix[i].find('<', nPos);
                string sEntry = utf8parser(vMatrix[i].substr(nPos,vMatrix[i].find('>', nPos)+1-nPos));
                nPos++;

                if (sEntry == "<>")
                    continue;

                sEntry.erase(0,1);
                sEntry.pop_back();

                if (!fileTableHeads[j].length())
                    fileTableHeads[j] = sEntry;
                else if (fileTableHeads[j] != sEntry)
                    fileTableHeads[j] += "\\n" + sEntry;
            }
        }

        for (long long int i = 0; i < nRows; i++)
        {
            nPos = 0;

            for (long long int j = 0; j < nCols; j++)
            {
                nPos = vMatrix[i+nCommentLines].find('<', nPos);
                string sEntry = utf8parser(vMatrix[i+nCommentLines].substr(nPos,vMatrix[i+nCommentLines].find('>', nPos)+1-nPos));
                nPos++;

                if (sEntry == "<>")
                    continue;

                sEntry.erase(0,1);
                sEntry.pop_back();

                if (isNumeric(sEntry))
                {
                    if (sEntry.find_first_not_of('-') == string::npos)
                        continue;

                    fileData[i][j] = StrToDb(sEntry);
                }
                else if (!fileTableHeads[j].length())
                    fileTableHeads[j] = sEntry;
                else if (fileTableHeads[j] != sEntry)
                    fileTableHeads[j] += "\\n" + sEntry;
            }
        }
    }

    string OpenDocumentSpreadSheet::expandLine(const string& sLine)
    {
        string sExpandedLine = "";

        for (unsigned int i = 0; i < sLine.length(); i++)
        {
            if (sLine.substr(i,17) == "<table:table-cell")
            {
                if (sLine[sLine.find('>',i)-1] != '/')
                {
                    // --> Value-reader
                    string sCellEntry = sLine.substr(i, sLine.find("</table:table-cell>", i)-i);

                    if (sCellEntry.find("office:value-type=") != string::npos && getArgAtPos(sCellEntry, sCellEntry.find("office:value-type=")+18) == "float")
                        sExpandedLine += "<" + getArgAtPos(sCellEntry, sCellEntry.find("office:value=")+13) + ">";
                    else if (sCellEntry.find("<text:p>") != string::npos)
                        sExpandedLine += "<" + sCellEntry.substr(sCellEntry.find("<text:p>")+8, sCellEntry.find("</text:p>")-sCellEntry.find("<text:p>")-8) + ">";
                }
                else
                {
                    if (sLine.find("<table:table-cell", i+1) == string::npos && sLine.find("<table:covered-table-cell", i+1) == string::npos)
                        break;

                    if (sLine.substr(i, sLine.find('>',i)+1-i).find("table:number-columns-repeated=") != string::npos)
                    {
                        string sTemp = getArgAtPos(sLine, sLine.find("table:number-columns-repeated=", i)+30);

                        if (sTemp.front() == '"')
                            sTemp.erase(0, 1);

                        if (sTemp.back() == '"')
                            sTemp.pop_back();

                        for (int j = 0; j < StrToInt(sTemp); j++)
                            sExpandedLine += "<>";
                    }
                    else
                        sExpandedLine += "<>";
                }
            }
            else if (sLine.substr(i,25) == "<table:covered-table-cell")
            {
                string sTemp = getArgAtPos(sLine, sLine.find("table:number-columns-repeated=", i)+30);

                if (sTemp.front() == '"')
                    sTemp.erase(0, 1);

                if (sTemp.back() == '"')
                    sTemp.pop_back();

                for (int j = 0; j < StrToInt(sTemp); j++)
                    sExpandedLine += "<>";
            }
        }

        if (sExpandedLine.length())
        {
            while (sExpandedLine.substr(sExpandedLine.length()-2) == "<>")
            {
                sExpandedLine.erase(sExpandedLine.length()-2);

                if (!sExpandedLine.length() || sExpandedLine.length() < 2)
                    break;
            }
        }

        return sExpandedLine;
    }


    //////////////////////////////////////////////
    // class XLSSpreadSheet
    //////////////////////////////////////////////
    //
    XLSSpreadSheet::XLSSpreadSheet(const string& filename) : GenericFile(filename)
    {
        //
    }

    XLSSpreadSheet::~XLSSpreadSheet()
    {
        //
    }

    void XLSSpreadSheet::readFile()
    {
        YExcel::BasicExcel _excel;
        YExcel::BasicExcelWorksheet* _sheet;
        YExcel::BasicExcelCell* _cell;
        unsigned int nSheets = 0;
        long long int nExcelLines = 0;
        long long int nExcelCols = 0;
        long long int nOffset = 0;
        long long int nCommentLines = 0;
        vector<long long int> vCommentLines;
        bool bBreakSignal = false;
        string sEntry;

        if (!_excel.Load(sFileName.c_str()))
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // get the total number
        nSheets = _excel.GetTotalWorkSheets();

        if (!nSheets)
            return;

        // get the total size
        for (unsigned int n = 0; n < nSheets; n++)
        {
            _sheet = _excel.GetWorksheet(n);
            bBreakSignal = false;
            nCommentLines = 0;

            for (unsigned int i = 0; i < _sheet->GetTotalRows(); i++)
            {
                for (unsigned int j = 0; j < _sheet->GetTotalCols(); j++)
                {
                    if (_sheet->Cell(i,j)->Type() != YExcel::BasicExcelCell::STRING
                        && _sheet->Cell(i,j)->Type() != YExcel::BasicExcelCell::WSTRING
                        && _sheet->Cell(i,j)->Type() != YExcel::BasicExcelCell::UNDEFINED)
                    {
                        bBreakSignal = true;
                        break;
                    }
                }

                if (bBreakSignal)
                    break;

                nCommentLines++;
            }

            vCommentLines.push_back(nCommentLines);

            if (nExcelLines < _sheet->GetTotalRows()-nCommentLines)
                nExcelLines = _sheet->GetTotalRows()-nCommentLines;

            nExcelCols += _sheet->GetTotalCols();
        }

        // create data
        nRows = nExcelLines;
        nCols = nExcelCols;
        createStorage();

        // copy data/strings
        for (unsigned int n = 0; n < nSheets; n++)
        {
            _sheet = _excel.GetWorksheet(n);
            nExcelCols = _sheet->GetTotalCols();
            nExcelLines = _sheet->GetTotalRows();

            for (long long int i = 0; i < vCommentLines[n]; i++)
            {
                if (i >= nExcelLines)
                    break;

                for (long long int j = 0; j < nExcelCols; j++)
                {
                    if (j+nOffset >= nCols)
                        break;

                    _cell = _sheet->Cell(i,j);

                    if (_cell->Type() == YExcel::BasicExcelCell::STRING)
                        sEntry = utf8parser(_cell->GetString());
                    else if (_cell->Type() == YExcel::BasicExcelCell::WSTRING)
                        sEntry = utf8parser(wcstombs(_cell->GetWString()));
                    else
                        continue;

                    while (sEntry.find('\n') != string::npos)
                        sEntry.replace(sEntry.find('\n'), 1, "\\n");

                    while (sEntry.find((char)10) != string::npos)
                        sEntry.replace(sEntry.find((char)10), 1, "\\n");

                    if (!fileTableHeads[j+nOffset].length())
                        fileTableHeads[j+nOffset] = sEntry;
                    else if (fileTableHeads[j+nOffset] != sEntry)
                        fileTableHeads[j+nOffset] += "\\n" + sEntry;
                }
            }

            nOffset += nExcelCols;
        }

        nOffset = 0;

        for (unsigned int n = 0; n < nSheets; n++)
        {
            _sheet = _excel.GetWorksheet(n);
            nExcelCols = _sheet->GetTotalCols();
            nExcelLines = _sheet->GetTotalRows();

            for (long long int i = vCommentLines[n]; i < nExcelLines; i++)
            {
                if (i - vCommentLines[n] >= nRows)
                    break;

                for (long long int j = 0; j < nExcelCols; j++)
                {
                    if (j >= nCols)
                        break;

                    _cell = _sheet->Cell(i,j);
                    sEntry.clear();

                    switch (_cell->Type())
                    {
                        case YExcel::BasicExcelCell::UNDEFINED:
                            fileData[i-vCommentLines[n]][j+nOffset] = NAN;
                            break;
                        case YExcel::BasicExcelCell::INT:
                            fileData[i-vCommentLines[n]][j+nOffset] = (double)_cell->GetInteger();
                            break;
                        case YExcel::BasicExcelCell::DOUBLE:
                            fileData[i-vCommentLines[n]][j+nOffset] = _cell->GetDouble();
                            break;
                        case YExcel::BasicExcelCell::STRING:
                            fileData[i-vCommentLines[n]][j+nOffset] = NAN;
                            sEntry = _cell->GetString();
                            break;
                        case YExcel::BasicExcelCell::WSTRING:
                            fileData[i-vCommentLines[n]][j+nOffset] = NAN;
                            sEntry = wcstombs(_cell->GetWString());
                            break;
                        default:
                            fileData[i-vCommentLines[n]][j+nOffset] = NAN;
                    }

                    if (sEntry.length())
                    {
                        if (!fileTableHeads[j+nOffset].length())
                            fileTableHeads[j+nOffset] = sEntry;
                        else if (fileTableHeads[j+nOffset] != sEntry)
                            fileTableHeads[j+nOffset] += "\\n" + sEntry;
                    }
                }
            }

            nOffset += nExcelCols;
        }
    }

    void XLSSpreadSheet::writeFile()
    {
        YExcel::BasicExcel _excel;
        YExcel::BasicExcelWorksheet* _sheet;
        YExcel::BasicExcelCell* _cell;

        string sHeadLine;
        string sSheetName = getTableName();

        // Create a new sheet
        _excel.New(1);

        // Rename it so that it fits the cache name
        _excel.RenameWorksheet(0u, sSheetName.c_str());

        // Get a pointer to this sheet
        _sheet = _excel.GetWorksheet(0u);

        // Write the headlines in the first row
        for (long long int j = 0; j < nCols; j++)
        {
            // Get the current cell and the headline string
            _cell = _sheet->Cell(0u, j);
            sHeadLine = fileTableHeads[j];

            // Replace newlines with the corresponding character code
            while (sHeadLine.find("\\n") != string::npos)
                sHeadLine.replace(sHeadLine.find("\\n"), 2, 1, (char)10);

            // Write the headline
            _cell->SetString(sHeadLine.c_str());
        }

        // Now write the actual table
        for (long long int i = 0; i < nRows; i++)
        {
            for (long long int j = 0; j < nCols; j++)
            {
                // Get the current cell (skip over the first row, because it contains the headline)
                _cell = _sheet->Cell(1 + i, j);

                // Write the cell contents, if the data table contains valid data
                // otherwise clear the cell
                if (!isnan(fileData[i][j]))
                    _cell->SetDouble(fileData[i][j]);
                else
                    _cell->EraseContents();
            }
        }

        // Save the excel file with the target filename
        _excel.SaveAs(sFileName.c_str());
    }


    //////////////////////////////////////////////
    // class XLSXSpreadSheet
    //////////////////////////////////////////////
    //
    XLSXSpreadSheet::XLSXSpreadSheet(const string& filename) : GenericFile(filename)
    {
        //
    }

    XLSXSpreadSheet::~XLSXSpreadSheet()
    {
        //
    }

    void XLSXSpreadSheet::readFile()
    {
        unsigned int nSheets = 0;
        long long int nExcelLines = 0;
        long long int nExcelCols = 0;
        long long int nOffset = 0;
        int nLine = 0, nCol = 0;
        int nLinemin = 0, nLinemax = 0;
        int nColmin = 0, nColmax = 0;
        bool bBreakSignal = false;

        vector<long long int> vCommentLines;
        string sEntry;
        string sSheetContent;
        string sStringsContent;
        string sCellLocation;

        tinyxml2::XMLDocument _workbook;
        tinyxml2::XMLDocument _sheet;
        tinyxml2::XMLDocument _strings;
        tinyxml2::XMLNode* _node;
        tinyxml2::XMLElement* _element;
        tinyxml2::XMLElement* _stringelement;

        sEntry = getZipFileItem("xl/workbook.xml");

        if (!sEntry.length())
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Get the number of sheets
        _workbook.Parse(sEntry.c_str());

        if (_workbook.ErrorID())
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        _node = _workbook.FirstChildElement()->FirstChildElement("sheets")->FirstChild();

        if (_node)
            nSheets++;

        while ((_node = _node->NextSibling()))
            nSheets++;

        if (!nSheets)
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Walk through the sheets and extract the dimension info
        for (unsigned int i = 0; i < nSheets; i++)
        {
            sSheetContent = getZipFileItem("xl/worksheets/sheet"+toString(i+1)+".xml");

            if (!sSheetContent.length())
                throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

            _sheet.Parse(sSheetContent.c_str());

            if (_sheet.ErrorID())
                throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

            _element = _sheet.FirstChildElement()->FirstChildElement("dimension");
            sCellLocation = _element->Attribute("ref");
            int nLinemin = 0, nLinemax = 0;
            int nColmin = 0, nColmax = 0;

            // Take care of comment lines => todo
            evalIndices(sCellLocation.substr(0, sCellLocation.find(':')), nLinemin, nColmin);
            evalIndices(sCellLocation.substr(sCellLocation.find(':') + 1), nLinemax, nColmax);

            vCommentLines.push_back(0);
            _node = _sheet.FirstChildElement()->FirstChildElement("sheetData")->FirstChild();

            if (!_node)
                continue;

            do
            {
                _element = _node->ToElement()->FirstChildElement("c");

                do
                {
                    if (_element->Attribute("t"))
                    {
                        if (_element->Attribute("t") != string("s"))
                        {
                            bBreakSignal = true;
                            break;
                        }
                    }
                    else
                    {
                        bBreakSignal = true;
                        break;
                    }
                }
                while ((_element = _element->NextSiblingElement()));

                if (!bBreakSignal)
                    vCommentLines[i]++;
                else
                    break;
            }
            while ((_node = _node->NextSibling()));

            bBreakSignal = false;

            if (nExcelLines < nLinemax-nLinemin+1-vCommentLines[i])
                nExcelLines = nLinemax-nLinemin+1-vCommentLines[i];

            nExcelCols += nColmax-nColmin+1;
        }

        nRows = nExcelLines;
        nCols = nExcelCols;

        // Allocate the memory
        createStorage();

        // Walk through the sheets and extract the contents to memory
        sStringsContent = getZipFileItem("xl/sharedStrings.xml");
        _strings.Parse(sStringsContent.c_str());

        for (unsigned int i = 0; i < nSheets; i++)
        {
            sSheetContent = getZipFileItem("xl/worksheets/sheet"+toString(i+1)+".xml");
            _sheet.Parse(sSheetContent.c_str());
            _node = _sheet.FirstChildElement()->FirstChildElement("sheetData")->FirstChild();
            _element = _sheet.FirstChildElement()->FirstChildElement("dimension");

            if (!_node)
                continue;

            sCellLocation = _element->Attribute("ref");
            evalIndices(sCellLocation.substr(0, sCellLocation.find(':')), nLinemin, nColmin);
            evalIndices(sCellLocation.substr(sCellLocation.find(':')+1), nLinemax, nColmax);

            do
            {
                _element = _node->ToElement()->FirstChildElement("c");

                if (!_element)
                    continue;

                do
                {
                    sCellLocation = _element->Attribute("r");
                    evalIndices(sCellLocation, nLine, nCol);
                    nCol -= nColmin;
                    nLine -= nLinemin;

                    if (nCol+nOffset >= nCols || nLine-vCommentLines[i] >= nRows)
                        continue;

                    if (_element->Attribute("t"))
                    {
                        if (_element->Attribute("t") == string("s"))
                        {
                            //Handle text
                            int nPos = 0;
                            _element->FirstChildElement("v")->QueryIntText(&nPos);
                            _stringelement = _strings.FirstChildElement()->FirstChildElement("si");

                            for (int k = 1; k <= nPos; k++)
                            {
                                _stringelement = _stringelement->NextSiblingElement();
                            }

                            if (_stringelement->FirstChildElement()->FirstChild())
                                sEntry = utf8parser(_stringelement->FirstChildElement()->FirstChild()->ToText()->Value());
                            else
                                sEntry.clear();

                            if (sEntry.length())
                            {
                                if (!fileTableHeads[nCol+nOffset].length())
                                    fileTableHeads[nCol+nOffset] = sEntry;
                                else if (fileTableHeads[nCol+nOffset] != sEntry)
                                    fileTableHeads[nCol+nOffset] += "\\n" + sEntry;
                            }

                            continue;
                        }
                    }

                    if (_element->FirstChildElement("v"))
                        _element->FirstChildElement("v")->QueryDoubleText(&fileData[nLine-vCommentLines[i]][nCol+nOffset]);
                }
                while ((_element = _element->NextSiblingElement()));
            }
            while ((_node = _node->NextSibling()));

            nOffset += nColmax-nColmin+1;
        }
    }

    void XLSXSpreadSheet::evalIndices(const string& _sIndices, int& nLine, int& nCol)
    {
        //A1 -> IV65536
        string sIndices = toUpperCase(_sIndices);

        for (size_t i = 0; i < sIndices.length(); i++)
        {
            if (!isalpha(sIndices[i]))
            {
                nLine = StrToInt(sIndices.substr(i))-1;

                if (i == 2)
                {
                    nCol = (sIndices[0]-'A'+1)*26+sIndices[1]-'A';
                }
                else if (i == 1)
                {
                    nCol = sIndices[0]-'A';
                }

                break;
            }
        }
    }


    //////////////////////////////////////////////
    // class IgorBinaryWave
    //////////////////////////////////////////////
    //
    IgorBinaryWave::IgorBinaryWave(const string& filename) : GenericFile(filename), bXZSlice(false)
    {
        //
    }

    IgorBinaryWave::IgorBinaryWave(const IgorBinaryWave& file) : GenericFile(file)
    {
        bXZSlice = file.bXZSlice;
    }

    IgorBinaryWave::~IgorBinaryWave()
    {
        //
    }

    void IgorBinaryWave::readFile()
    {
        CP_FILE_REF _cp_file_ref;
        int nType = 0;
        long nPnts = 0;
        void* vWaveDataPtr = nullptr;
        long long int nFirstCol = 0;
        long int nDim[MAXDIMS];
        double dScalingFactorA[MAXDIMS];
        double dScalingFactorB[MAXDIMS];
        bool bReadComplexData = false;
        int nSliceCounter = 0;
        float* fData = nullptr;
        double* dData = nullptr;
        int8_t* n8_tData = nullptr;
        int16_t* n16_tData = nullptr;
        int32_t* n32_tData = nullptr;
        char* cName = nullptr;

        if (CPOpenFile(sFileName.c_str(), 0, &_cp_file_ref))
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        if (ReadWave(_cp_file_ref, &nType, &nPnts, nDim, dScalingFactorA, dScalingFactorB, &vWaveDataPtr, &cName))
        {
            CPCloseFile(_cp_file_ref);

            if (vWaveDataPtr != NULL)
                free(vWaveDataPtr);

            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);
        }

        if (nType & NT_CMPLX)
            bReadComplexData = true;

        if (nType & NT_FP32)
            fData = (float*)vWaveDataPtr;
        else if (nType & NT_FP64)
            dData = (double*)vWaveDataPtr;
        else if (nType & NT_I8)
            n8_tData = (int8_t*)vWaveDataPtr;
        else if (nType & NT_I16)
            n16_tData = (int16_t*)vWaveDataPtr;
        else if (nType & NT_I32)
            n32_tData = (int32_t*)vWaveDataPtr;
        else
        {
            CPCloseFile(_cp_file_ref);

            if (vWaveDataPtr != nullptr)
                free(vWaveDataPtr);

            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);
        }

        nRows = nDim[0];
        nCols = nDim[1];

        if (nDim[2])
            nCols *= nDim[2];

        if (!nRows)
        {
            CPCloseFile(_cp_file_ref);

            if (vWaveDataPtr != nullptr)
                free(vWaveDataPtr);

            throw SyntaxError(SyntaxError::FILE_IS_EMPTY, sFileName, SyntaxError::invalid_position, sFileName);
        }

        if (!nCols || nCols == 1)
        {
            nFirstCol = 1;
            nCols = 2;

            if (bReadComplexData)
                nCols++;
        }
        else if (nDim[1] && (!nDim[2] || nDim[2] == 1))
        {
            if (bReadComplexData)
                nCols *= 2;

            nCols += 2;
            nFirstCol = 2;

            if (nDim[1] > nDim[0])
                nRows = nDim[1];
        }
        else if (nDim[1] && nDim[2] && (!nDim[3] || nDim[3] == 1))
        {
            if (bReadComplexData)
                nCols *= 2;

            nCols += 3;
            nFirstCol = 3;
            nRows = nDim[2] > nRows ? nDim[2] : nRows;
            nRows = nDim[1] > nRows ? nDim[1] : nRows;
        }

        createStorage();

        for (long long int j = 0; j < nFirstCol; j++)
        {
            fileTableHeads[j] = cName + string("_[")+(char)('x'+j)+string("]");

            for (long long int i = 0; i < nDim[j]; i++)
            {
                fileData[i][j] = dScalingFactorA[j]*(double)i + dScalingFactorB[j];
            }
        }

        for (long long int j = 0; j < nCols-nFirstCol; j++)
        {
            if (bXZSlice && nDim[2] > 1 && j)
            {
                nSliceCounter += nDim[1];

                if (!(j % nDim[2]))
                    nSliceCounter = j/nDim[2];
            }
            else
                nSliceCounter = j;

            if (nCols == 2 && !j)
            {
                fileTableHeads[1] = cName + string("_[y]");
            }
            else if (nCols == 3 && !j && bReadComplexData)
            {
                fileTableHeads[1] = string("Re:_") + cName + string("_[y]");
                fileTableHeads[2] = string("Im:_") + cName + string("_[y]");
            }
            else if (!bReadComplexData)
                fileTableHeads[j+nFirstCol] = cName + string("_["+toString(j+1)+"]");
            else
            {
                fileTableHeads[j+nFirstCol] = string("Re:_") + cName + string("_["+toString(j+1)+"]");
                fileTableHeads[j+nFirstCol+1] = string("Im:_") + cName + string("_["+toString(j+1)+"]");
            }

            for (long long int i = 0; i < (nDim[0]+bReadComplexData*nDim[0]); i++)
            {
                if (dData)
                {
                    fileData[i][nSliceCounter+nFirstCol] = dData[i+j*(nDim[0]+bReadComplexData*nDim[0])];

                    if (bReadComplexData)
                    {
                        fileData[i][nSliceCounter+1+nFirstCol] = dData[i+1+j*(nDim[0]+bReadComplexData*nDim[0])];
                        i++;
                    }
                }
                else if (fData)
                {
                    fileData[i][nSliceCounter+nFirstCol] = fData[i+j*(nDim[0]+bReadComplexData*nDim[0])];

                    if (bReadComplexData)
                    {
                        fileData[i][nSliceCounter+1+nFirstCol] = fData[i+1+j*(nDim[0]+bReadComplexData*nDim[0])];
                        i++;
                    }
                }
                else if (n8_tData)
                {
                    fileData[i][nSliceCounter+nFirstCol] = (double)n8_tData[i+j*(nDim[0]+bReadComplexData*nDim[0])];

                    if (bReadComplexData)
                    {
                        fileData[i][nSliceCounter+1+nFirstCol] = (double)n8_tData[i+1+j*(nDim[0]+bReadComplexData*nDim[0])];
                        i++;
                    }
                }
                else if (n16_tData)
                {
                    fileData[i][nSliceCounter+nFirstCol] = (double)n16_tData[i+j*(nDim[0]+bReadComplexData*nDim[0])];

                    if (bReadComplexData)
                    {
                        fileData[i][nSliceCounter+1+nFirstCol] = (double)n16_tData[i+1+j*(nDim[0]+bReadComplexData*nDim[0])];
                        i++;
                    }
                }
                else if (n32_tData)
                {
                    fileData[i][nSliceCounter+nFirstCol] = (double)n32_tData[i+j*(nDim[0]+bReadComplexData*nDim[0])];

                    if (bReadComplexData)
                    {
                        fileData[i][nSliceCounter+1+nFirstCol] = (double)n32_tData[i+1+j*(nDim[0]+bReadComplexData*nDim[0])];
                        i++;
                    }
                }
                else
                    continue;
            }

            if (bReadComplexData)
                j++;
        }

        CPCloseFile(_cp_file_ref);

        if (vWaveDataPtr != nullptr)
            free(vWaveDataPtr);
    }

    IgorBinaryWave& IgorBinaryWave::operator=(const IgorBinaryWave& file)
    {
        assign(file);
        bXZSlice = file.bXZSlice;

        return *this;
    }
    //
}


