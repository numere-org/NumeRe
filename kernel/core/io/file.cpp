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
#include "../utils/tools.hpp"
#include "../version.h"

namespace NumeRe
{
    using namespace std;

    NumeReDataFile::NumeReDataFile(const string& filename)
        : GenericFile(filename, ios::binary | ios::in | ios::out),
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
        writeStringField(sComment);
        writeDataBlock(nullptr, 0);
        writeDataBlock(nullptr, 0);
        writeDataBlock(nullptr, 0);
        writeStringField("DATATYPE=DOUBLE");
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

        sComment = readStringField();

        long long int size;
        readDataBlock(size);
        readDataBlock(size);
        readDataBlock(size);

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

    std::string NumeReDataFile::getVersionString()
    {
        return toString(versionMajor) + "." + toString(versionMinor) + "." + toString(versionBuild);
    }




    CassyLabx::CassyLabx(const string& filename) : GenericFile(filename, ios::in)
    {
        //
    }

    CassyLabx::~CassyLabx()
    {
        //
    }

    void CassyLabx::readFile()
    {
        string sLabx = "";
        string sLabx_substr = "";
        string* sCols = 0;
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
            if (sCols[i].find("<values count=\"0\" />") == string::npos)
            {
                nElements = StrToInt(sCols[i].substr(sCols[i].find('"')+1, sCols[i].find('"', sCols[i].find('"')+1)-1-sCols[i].find('"')));
                sCols[i] = sCols[i].substr(sCols[i].find('>')+1);

                for (long long int j = 0; j < nRows; j++)
                {
                    if (j >= nElements)
                    {
                        sDataMatrix[j][i] = "<value />";
                    }
                    else
                    {
                        sDataMatrix[j][i] = sCols[i].substr(sCols[i].find("<value"), sCols[i].find('<', sCols[i].find('/'))-sCols[i].find("<value"));
                        sCols[i] = sCols[i].substr(sCols[i].find('>', sCols[i].find('/'))+1);
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





    CommaSeparatedValues::CommaSeparatedValues(const string& filename) : GenericFile(filename, ios::in | ios::out)
    {
        //
    }

    CommaSeparatedValues::~CommaSeparatedValues()
    {
        //
    }


    void CommaSeparatedValues::readFile()
    {
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

        string sValidSymbols = "0123456789.,;-+eE INFAinfa";
        sValidSymbols += cSep;

        for (unsigned int j = 0; j < vFileData[0].length(); j++)
        {
            if (sValidSymbols.find(vFileData[0][j]) == string::npos)
            {
                string __sLine = vFileData[0];

                for (unsigned int n = 0; n < nCols-1; n++)
                {
                    vHeadLine.push_back(__sLine.substr(0,__sLine.find(cSep)));
                    StripSpaces(vHeadLine.back());

                    if (!vHeadLine.back().length())
                        vHeadLine.back() = "Spalte_" + toString((int)n+1);

                    __sLine = __sLine.substr(__sLine.find(cSep)+1);
                }

                vHeadLine.push_back(__sLine);

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
                            vHeadLine[k] = "Spalte_" + toString((int)k+1);
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
			else
			{
				for (long long int i = 0; i < nCols; i++)
				{
					fileTableHeads[i] = "Spalte_" + toString(i+1);
				}
			}
		}

		// --> Hier werden die Strings in Tokens zerlegt <--
		size_t n = 0;

		for (size_t i = 0; i < vFileData.size(); i++)
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
                    fileData[n][j] = NAN;
                }
                else if (vTokens[j] == "inf")
                    fileData[n][j] = INFINITY;
                else if (vTokens[j] == "-inf")
                    fileData[n][j] = -INFINITY;
                else
                {
                    for (size_t k = 0; k < vTokens[j].length(); k++)
                    {
                        if (sValidSymbols.find(vTokens[j][k]) == string::npos)
                        {
                            fileTableHeads[j] += "\\n" + vTokens[j];
                            fileData[n][j] = NAN;
                            break;
                        }

                        if (k+1 == vTokens[j].length())
                        {
                            replaceDecimalSign(vTokens[j]);
                            fileData[n][j] = StrToDb(vTokens[j]);
                        }
                    }
                }
            }
        }
    }

    void CommaSeparatedValues::writeFile()
    {
        for (long long int j = 0; j < nCols; j++)
        {
            fFileStream << fileTableHeads[j] + ",";
        }

        fFileStream << "\n";

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





    JcampDX::JcampDX(const string& filename) : GenericFile(filename, ios::in)
    {
        //
    }

    JcampDX::~JcampDX()
    {
        //
    }

    void JcampDX::readFile()
    {
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




    OpenDocumentSpreadSheet::OpenDocumentSpreadSheet(const string& filename) : GenericFile(filename, ios::in)
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
                string sEntry = vMatrix[i].substr(nPos,vMatrix[i].find('>', nPos)+1-nPos);
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
                string sEntry = vMatrix[i+nCommentLines].substr(nPos,vMatrix[i+nCommentLines].find('>', nPos)+1-nPos);
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


    //
}


