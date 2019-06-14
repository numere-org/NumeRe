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

    // This function determines the correct class to be used for the filename
    // passed to this function. If there's no fitting file type, a null pointer
    // is returned. The calling function is responsible for clearing the
    // created instance. The returned pointer is of the type of GenericFile<double>
    // but references an instance of a derived class
    GenericFile<double>* getFileByType(const string& filename)
    {
        FileSystem _fSys;

        // Get the extension of the filename
        string sExt = toLowerCase(_fSys.getFileParts(filename).back());

        // Create an instance of the selected file type
        if (sExt == "ndat")
            return new NumeReDataFile(filename);

        if (sExt == "txt" || sExt == "dat" || !sExt.length())
            return new TextDataFile(filename);

        if (sExt == "csv")
            return new CommaSeparatedValues(filename);

        if (sExt == "tex")
            return new LaTeXTable(filename);

        if (sExt == "labx")
            return new CassyLabx(filename);

        if (sExt == "ibw")
            return new IgorBinaryWave(filename);

        if (sExt == "ods")
            return new OpenDocumentSpreadSheet(filename);

        if (sExt == "xls")
            return new XLSSpreadSheet(filename);

        if (sExt == "xlsx")
            return new XLSXSpreadSheet(filename);

        if (sExt == "jdx" || sExt == "dx" || sExt == "jcm")
            return new JcampDX(filename);

        // If no filetype matches, return a null pointer
        return nullptr;
    }


    //////////////////////////////////////////////
    // class TextDataFile
    //////////////////////////////////////////////
    //
    TextDataFile::TextDataFile(const string& filename) : GenericFile(filename)
    {
        // Empty constructor
    }

    TextDataFile::~TextDataFile()
    {
        // Empty destructor
    }

    // This method reads the data in the referenced text file
    // to memory
    void TextDataFile::readFile()
    {
        open(ios::in);

		long long int nLine = 0;
		long long int nCol = 0;
		long long int nComment = 0;

		// Read the text file contents to memory
		vector<string> vFileContents = readTextFile(true);

		// Ensure that the file was not empty
		if (!vFileContents.size())
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Replace tabulator characters with whitespaces but
        // insert placeholders for empty table headlines
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

		// Count the number of comment lines, which are found
		// in the files contents. Determine also, how many
		// columns are found in the file
		for (size_t i = 0; i < vFileContents.size(); i++)
		{
			if (vFileContents[i][0] == '#' || !isNumeric(vFileContents[i]))
			{
				nComment++;
				continue;
			}
			else if (!nCol)
			{
			    // Determine the number of columns by applying
			    // the tokenizer to the files contents
				nCol = tokenize(vFileContents[i], " ", true).size();
			}
		}

		// Set the final dimensions
		nRows = vFileContents.size() - nComment;
		nCols = nCol;

		// Create the target storage
		createStorage();

		nLine = 0;

		// Copy the data from the file to the internal memory
		for (size_t i = 0; i < vFileContents.size(); i++)
        {
            if (vFileContents[i][0] == '#' || !isNumeric(vFileContents[i]))
            {
                // ignore table heads
                continue;
            }

            // Tokenize the current line
            vector<string> vLine = tokenize(vFileContents[i], " ", true);

            // Ensure that the number of columns is matching
            // If it does not match, then we did not determine
            // the columns correctly
            if (vLine.size() != nCols)
                throw SyntaxError(SyntaxError::COL_COUNTS_DOESNT_MATCH, sFileName, SyntaxError::invalid_position, sFileName);

            // Go through the already tokenized line and store
            // the converted values
            for (size_t j = 0; j < vLine.size(); j++)
            {
                // Replace comma with a dot
                replaceDecimalSign(vLine[j]);

                // Handle special values and convert the text
                // string into a numerical value
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

        // Decode the table headlines in this member function
        if (nComment)
        {
            decodeTableHeads(vFileContents, nComment);
        }
    }

    // This method writes the data in memory to the referenced text
    // file
    void TextDataFile::writeFile()
    {
        // Open the file
        open(ios::out | ios::trunc);

        // Write the header
        writeHeader();

        // Get the necessary column widths and the number
        // of lines, which are needed for the column heads
        size_t nNumberOfHeadlines = 1u;
        vector<size_t> vColumns =  calculateColumnWidths(nNumberOfHeadlines);

        // Write the column heads and append a separator
        writeTableHeads(vColumns, nNumberOfHeadlines);
        addSeparator(vColumns);

        // Write the actual data
        writeTableContents(vColumns);
    }

    // This member function writes the header lines for the
    // text files
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

    // This member function is used to write the table heads
    // into the target file
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

    // This member function is used to write the data in
    // memory to the target text file
    void TextDataFile::writeTableContents(const vector<size_t>& vColumnWidth)
    {
        for (long long int i = 0; i < nRows; i++)
        {
            for (long long int j = 0; j < nCols; j++)
            {
                fFileStream.width(vColumnWidth[j]+2);
                fFileStream.fill(' ');
                fFileStream.precision(nPrecFields);

                // Handle NaNs correctly
                if (isnan(fileData[i][j]))
                    fFileStream << "---";
                else
                    fFileStream << fileData[i][j];
            }

            fFileStream.width(0);
            fFileStream << "\n";
        }
    }

    // This member function draws a separator line based
    // upon the overall width of the columns
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

    // This member function decodes the table heads in the
    // text file and stores them in memory
    void TextDataFile::decodeTableHeads(vector<string>& vFileContents, long long int nComment)
    {
		long long int _nHeadline = 0;
        string sCommentSign = "#";

        // If the length of the file is larger than 14 lines, then
        // append equal signs to the comment sign
        if (vFileContents.size() > 14)
            sCommentSign.append(vFileContents[13].length()-1, '=');

        // This section will locate the first line in the comment section,
        // where the table heads are located. Depending on the format of
        // the comment section in the file, the following code tries
        // different approaches
        if (nComment >= 15 && vFileContents[2].substr(0, 21) == "# NumeRe: Framework f")
        {
            // This is a NumeRe-created text file
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
            // That's a custom created text file with only one possible
            // candidate for the table heads
            for (size_t i = 0; i < vFileContents.size(); i++)
            {
                if (vFileContents[i][0] == '#')
                {
                    // Starting with a "#" sign
                    if (vFileContents[i].find(' ') != string::npos)
                    {
                        // Insert a whitespace character right after
                        // the comment character to make it separatable
                        // using the tokenizer
                        if (vFileContents[i][1] != ' ')
                        {
                            for (unsigned int n = 0; n < vFileContents[i].length(); n++)
                            {
                                if (vFileContents[i][n] != '#')
                                {
                                    if (vFileContents[i][n] != ' ')
                                        vFileContents[i].insert(n, " ");

                                    break;
                                }
                            }
                        }

                        // Considering the comment character, which is the
                        // first token, does the number of elements match to
                        // the number of columns?
                        if (nCols + 1 == tokenize(vFileContents[i], " ", true).size())
                            _nHeadline = 1;
                    }

                    break;
                }
                else if (!isNumeric(vFileContents[i]))
                {
                    // Simply a non-numeric line
                    if (nCols == tokenize(vFileContents[i], " ", true).size())
                        _nHeadline = 1;
                }
            }
        }
        else if (vFileContents[0][0] == '#' || !isNumeric(vFileContents[0]))
        {
            // This is the most generic approach: every line, which contains
            // non-numeric characters is examined to locate the correct line
            // containing the table column heads
            for (size_t i = 0; i < vFileContents.size(); i++)
            {
                if (vFileContents[i][0] != '#' && isNumeric(vFileContents[i]))
                {
                    if (vFileContents[i-1][0] == '#')
                    {
                        if ((nCols > 1 && vFileContents[i-1].find(' ') != string::npos)
                            || (nCols == 1 && vFileContents[i-1].length() > 1))
                        {
                            // Insert a whitespace character right after
                            // the comment character to make it separatable
                            // using the tokenizer
                            if (vFileContents[i-1][1] != ' ')
                            {
                                for (unsigned int n = 0; n < vFileContents[i-1].length(); n++)
                                {
                                    if (vFileContents[i-1][n] != '#')
                                    {
                                        if (vFileContents[i-1][n] != ' ')
                                            vFileContents[i-1].insert(n, " ");

                                        break;
                                    }
                                }
                            }

                            // Considering the comment character, which is the
                            // first token, does the number of elements match to
                            // the number of columns?
                            if (tokenize(vFileContents[i-1], " ", true).size() == nCols+1)
                            {
                                _nHeadline = i;
                                break;
                            }
                        }

                        if (i > 1
                            && vFileContents[i-2][0] == '#'
                            && ((vFileContents[i-2].find(' ') != string::npos && nCols > 1)
                                || (nCols == 1 && vFileContents[i-2].length() > 1)))
                        {
                            // Insert a whitespace character right after
                            // the comment character to make it separatable
                            // using the tokenizer
                            if (vFileContents[i-2][1] != ' ')
                            {
                                for (size_t n = 0; n < vFileContents[i-2].length(); n++)
                                {
                                    if (vFileContents[i-2][n] != '#')
                                    {
                                        if (vFileContents[i-2][n] != ' ')
                                            vFileContents[i-2].insert(n, " ");

                                        break;
                                    }
                                }
                            }

                            // Considering the comment character, which is the
                            // first token, does the number of elements match to
                            // the number of columns?
                            if (tokenize(vFileContents[i-2], " ", true).size() == nCols+1)
                                _nHeadline = i-1;
                        }
                    }
                    else if (!isNumeric(vFileContents[i-1]))
                    {
                        if ((vFileContents[i-1].find(' ') != string::npos && nCols > 1)
                            || (nCols == 1 && vFileContents[i-1].length() > 1))
                        {
                            // Insert a whitespace character right after
                            // the comment character to make it separatable
                            // using the tokenizer
                            if (vFileContents[i-1][1] != ' ')
                            {
                                for (size_t n = 0; n < vFileContents[i-1].length(); n++)
                                {
                                    if (vFileContents[i-1][n] != '#')
                                    {
                                        if (vFileContents[i-1][n] != ' ')
                                            vFileContents[i-1].insert(n, " ");

                                        break;
                                    }
                                }
                            }

                            // Considering the comment character, which is the
                            // first token, does the number of elements match to
                            // the number of columns?
                            if (tokenize(vFileContents[i-1], " ", true).size() == nCols+1)
                            {
                                _nHeadline = i;
                                break;
                            }
                        }

                        if (i > 1
                            && vFileContents[i-2][0] == '#'
                            && ((vFileContents[i-2].find(' ') != string::npos && nCols > 1)
                                || (nCols == 1 && vFileContents[i-2].length() > 1)))
                        {
                            // Insert a whitespace character right after
                            // the comment character to make it separatable
                            // using the tokenizer
                            if (vFileContents[i-2][1] != ' ')
                            {
                                for (unsigned int n = 0; n < vFileContents[i-2].length(); n++)
                                {
                                    if (vFileContents[i-2][n] != '#')
                                    {
                                        if (vFileContents[i-2][n] != ' ')
                                            vFileContents[i-2].insert(n, " ");

                                        break;
                                    }
                                }
                            }

                            // Considering the comment character, which is the
                            // first token, does the number of elements match to
                            // the number of columns?
                            if (tokenize(vFileContents[i-2], " ", true).size() == nCols+1)
                                _nHeadline = i-1;
                        }
                    }

                    break;
                }
            }
        }

        // This section will extract the identified table
        // column heads and store them in the internal
        // storage
        if (_nHeadline)
		{
            long long int n = 0;

            // Go through the contents of the whole file and
            // search for the non-numeric lines in the file
            for (size_t i = 0; i < vFileContents.size(); i++)
            {
                // Is this a non-numeric line?
                if (vFileContents[i][0] == '#' || !isNumeric(vFileContents[i]))
                {
                    // increment the counter variable first,
                    // because we counted from 1 previously
                    n++;

                    // Does the counter match the selected
                    // headline?
                    if (n == _nHeadline)
                    {
                        // This section creates the needed "empty" tokens,
                        // which will be used to determine the association
                        // of single strings (i.e. a single string in the
                        // second line of a column head)
                        for (size_t k = i+1; k < vFileContents.size(); k++)
                        {
                            // If the current line contains the placeholders,
                            // which we introduced before, we abort at this
                            // line, because we already know the number of
                            // tokens in this line
                            if (vFileContents[k].find(" _ ") != string::npos
                                || (vFileContents[k].find_first_not_of(" #") != string::npos && vFileContents[k][vFileContents[k].find_first_not_of(" #")] == '_')
                                || vFileContents[k].back() == '_')
                            {
                                break;
                            }

                            // Abort on non-numeric lines
                            if (vFileContents[k][0] != '#' && isNumeric(vFileContents[k]))
                                break;

                            // Abort on separator lines
                            if (vFileContents[k].substr(0, 4) == "#===" || vFileContents[k].substr(0, 5) == "# ===")
                                break;

                            // Ensure that the current line has the same length
                            // as the first line in the current headline set. Also,
                            // transform multiple whitespaces into underscores, to mirror
                            // the "tokens" from the previous lines
                            if (vFileContents[k].length() == vFileContents[i].length())
                            {
                                // Mirror the tokens from the previous lines
                                for (unsigned int l = 0; l < vFileContents[k].length(); l++)
                                {
                                    if (vFileContents[i][l] != ' ' && vFileContents[k][l] == ' ')
                                        vFileContents[k][l] = '_';
                                }
                            }
                            else if (vFileContents[k].length() < vFileContents[i].length() && vFileContents[i].back() != ' ' && vFileContents[k].back() != ' ')
                            {
                                vFileContents[k].append(vFileContents[i].length() - vFileContents[k].length(), ' ');

                                // Mirror the tokens from the previous lines
                                for (unsigned int l = 0; l < vFileContents[k].length(); l++)
                                {
                                    if (vFileContents[i][l] != ' ' && vFileContents[k][l] == ' ')
                                        vFileContents[k][l] = '_';
                                }
                            }
                            else if (vFileContents[k].length() > vFileContents[i].length() && vFileContents[k].back() != ' ' && vFileContents[i].back() != ' ')
                            {
                                // Mirror the tokens from the previous lines
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

                        // Go now through the selected headlines and
                        // break them down into the single columns. Based
                        // upon the created tokens from the previous code
                        // section, we can ensure that we may keep the
                        // association of the different lines
                        for (size_t k = i; k < vFileContents.size(); k++)
                        {
                            // Abort on numeric lines
                            if (vFileContents[k][0] != '#' && isNumeric(vFileContents[k]))
                                break;

                            // Abort on separator lines
                            if (vFileContents[k].substr(0, 4) == "#===" || vFileContents[k].substr(0, 5) == "# ===")
                                break;

                            // Tokenize the current line
                            vector<string> vLine = tokenize(vFileContents[k], " ", true);

                            // Remove the comment character from the list
                            // of tokens
                            if (vLine.front() == "#")
                                vLine.erase(vLine.begin());

                            // Distribute the tokens into the single column
                            // headings and insert line break characters
                            // whereever needed. Of course, we will ignore
                            // the inserted placeholders in this case
                            for (size_t j = 0; j < vLine.size(); j++)
                            {
                                if (k == i)
                                    vHeadline.push_back("");

                                if (k != i && j >= vHeadline.size())
                                {
                                    bBreakSignal = true;
                                    break;
                                }

                                // Ignore placeholder tokens
                                if (vLine[j].find_first_not_of('_') == string::npos)
                                    continue;

                                // Strip all underscores from the
                                // current token
                                while (vLine[j].front() == '_')
                                    vLine[j].erase(0,1);

                                while (vLine[j].back() == '_')
                                    vLine[j].pop_back();

                                // Append the current token to the
                                // corresponding column heading and
                                // insert the linebreak character,
                                // if needed
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

                        // Remove all empty columns from the list of
                        // column headings
                        for (auto iter = vHeadline.begin(); iter != vHeadline.end(); ++iter)
                        {
                            if (!(iter->length()))
                            {
                                iter = vHeadline.erase(iter);
                                iter--;
                            }
                        }

                        // Copy the decoded column headings to
                        // the internal memory
                        if (vHeadline.size() <= nCols)
                        {
                            copyStringArray(&vHeadline[0], fileTableHeads, vHeadline.size());
                        }

                        // Return here
                        return;
                    }
                }
            }
		}
    }

    // This member function calculates the widths of the columns
    // and also determines the number of lines needed for the
    // column heads
    vector<size_t> TextDataFile::calculateColumnWidths(size_t& nNumberOfLines)
    {
        vector<size_t> vColumnWidths;
        const size_t NUMBERFIELDLENGTH = nPrecFields + 7;

        // Go through the column heads in memory, determine
        // their cell extents and store the maximal value of
        // the extents and the needed width for the numerical
        // value
        for (long long int j = 0; j < nCols; j++)
        {
            pair<size_t, size_t> pCellExtents = calculateCellExtents(fileTableHeads[j]);
            vColumnWidths.push_back(max(NUMBERFIELDLENGTH, pCellExtents.first));

            if (nNumberOfLines < pCellExtents.second)
                nNumberOfLines = pCellExtents.second;
        }

        return vColumnWidths;
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
        // Empty constructor
    }

    // This copy constructor extents the copy constructor
    // of the parent class
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
        // Empty destructor
    }

    // This member function writes the new standard
    // header for NDAT files. It includes a dummy
    // section, which older versions of NumeRe may
    // read without errors, but which won't contain
    // any reasonable information
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

        // The following fields are placeholders
        // for further changes. They may be filled
        // in future versions and will be ignored
        // in older versions of NumeRe
        writeStringField("FTYPE=DOUBLE");
        writeNumBlock<double>(nullptr, 0);
        writeStringField("FTYPE=DOUBLE");
        writeNumBlock<double>(nullptr, 0);
        writeStringField("FTYPE=DOUBLE");
        writeNumBlock<double>(nullptr, 0);
        writeStringField("DTYPE=DOUBLE");

        // Finally, write the dimensions of the
        // target data
        writeNumField(nRows);
        writeNumField(nCols);
    }

    // This member function will write the dummy
    // header, which is readable in older versions
    // of NumeRe. In principle, this is the old
    // legacy format of NDAT files for a 1x1 data set
    void NumeReDataFile::writeDummyHeader()
    {
        writeNumField(1LL);
        writeNumField(1LL);
        writeStringField("THIS_FILE_NEEDS_AT_LEAST_VERSION_v1.1.2 ");
        long long int appZeros = 0;
        double data = 1.12;
        bool valid = true;
        fFileStream.write((char*)&appZeros, sizeof(long long int));
        fFileStream.write((char*)&data, sizeof(double));
        fFileStream.write((char*)&valid, sizeof(bool));
    }

    // This member funcion will write the data in
    // the internal storage into the target file
    void NumeReDataFile::writeFile()
    {
        // Open the file in binary mode and truncate
        // its contents (this will only be done, if
        // it's not already open, because otherwise
        // the cache file would be overwritten by each
        // table in memory completely)
        if (!is_open())
            open(ios::binary | ios::out | ios::trunc);

        // Write the file header
        writeHeader();

        // Write the table column headers
        writeStringBlock(fileTableHeads, nCols);

        // Write the actual data array
        writeDataArray(fileData, nRows, nCols);
    }

    // This member function will read the header
    // in the selected file. It will automatically
    // detect, whether the file is in legacy format
    // or not. If the file format is newer than
    // expected, it will throw an error
    void NumeReDataFile::readHeader()
    {
        // Read the basic information
        versionMajor = readNumField<long int>();
        versionMinor = readNumField<long int>();
        versionBuild = readNumField<long int>();
        timeStamp = readNumField<time_t>();

        // Detect, whether this file was created in
        // legacy format (earlier than v1.1.2)
        if (versionMajor * 100 + versionMinor * 10 + versionBuild <= 111)
        {
            isLegacy = true;
            nRows = readNumField<long long int>();
            nCols = readNumField<long long int>();
            return;
        }

        // Omitt the dummy header
        skipDummyHeader();

        // Read the file version number (which is
        // independent on the version number of
        // NumeRe)
        short fileVerMajor = readNumField<short>();
        short fileVerMinor = readNumField<short>();

        // Ensure that the major version of the file
        // is not larger than the major version
        // implemented in this class (minor version
        // changes shall be downwards compatible)
        if (fileVerMajor > fileVersionMajor)
            throw SyntaxError(SyntaxError::INSUFFICIENT_NUMERE_VERSION, sFileName, SyntaxError::invalid_position, sFileName);

        // Read the table name and the comment
        sTableName = readStringField();
        sComment = readStringField();

        // Read now the three empty fields of the
        // header. We created special functions for
        // reading and deleting arbitary data types
        long long int size;
        string type;
        void* data = readGenericField(type, size);
        deleteGenericData(data, type);

        data = readGenericField(type, size);
        deleteGenericData(data, type);

        data = readGenericField(type, size);
        deleteGenericData(data, type);

        // Determine the data type of the table
        string dataType = readStringField();

        // Ensure that the data type is DOUBLE
        if (dataType != "DTYPE=DOUBLE")
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Read the dimensions of the table
        nRows = readNumField<long long int>();
        nCols = readNumField<long long int>();
    }

    // This function jumps over the dummy section
    // in the new file format, because it doesn't
    // contain any valid information
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

    // This member function will read the contents
    // of the target file
    void NumeReDataFile::readFile()
    {
        // Open the file in binary mode, if it's not
        // already open
        if (!is_open())
            open(ios::binary | ios::in);

        // Read the file header and determine,
        // whether the file is in legacy mode
        readHeader();

        // If the file is in legacy mode, read
        // the remaining file in legacy mode as
        // well
        if (isLegacy)
        {
            readLegacyFormat();
            return;
        }

        long long int stringBlockSize;
        long long int dataarrayrows;
        long long int dataarraycols;

        // Read the table column headers and the
        // table data directly to the internal
        // memory (we avoid unneeded copying of
        // the data)
        fileTableHeads = readStringBlock(stringBlockSize);
        fileData = readDataArray(dataarrayrows, dataarraycols);
    }

    // This member function reads the data section
    // of the target file in legacy format. The
    // function "readHeader()" determines, whether
    // the target file is in legacy mode
    void NumeReDataFile::readLegacyFormat()
    {
        // Prepare some POD arrays to read the
        // blocks to memory
        size_t nLength = 0;
        bool* bValidEntry = new bool[nCols];
        char** cHeadLine = new char*[nCols];
        long long int* nAppendedZeros = new long long int[nCols];

        // Create the internal storage
        createStorage();

        // Read the table heads to the internal
        // storage
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

        // Read the appended zeros field, which
        // is ignored in this implementation,
        // because the calling data object will
        // determine this information by itself
        fFileStream.read((char*)nAppendedZeros, sizeof(long long int)*nCols);

        // Read the table data linewise to the
        // internal memory. We cannot read the
        // whole block at once, because the
        // two dimensional array is probably
        // not layed out as continous block
        // in memory
        for (long long int i = 0; i < nRows; i++)
        {
            fFileStream.read((char*)fileData[i], sizeof(double)*nCols);
        }

        // Read the validation block and apply
        // the contained information on the data
        // in the internal storage
        for (long long int i = 0; i < nRows; i++)
        {
            fFileStream.read((char*)bValidEntry, sizeof(bool)*nCols);
            for (long long int j = 0; j < nCols; j++)
            {
                if (!bValidEntry[j])
                    fileData[i][j] = NAN;
            }
        }

        // Free the created memory
        for (long long int i = 0; i < nCols; i++)
            delete[] cHeadLine[i];

        delete[] cHeadLine;
        delete[] bValidEntry;
        delete[] nAppendedZeros;
    }

    // This member function will read a generic
    // field from the header (the three fields,
    // which can be used in future versions of NumeRe)
    void* NumeReDataFile::readGenericField(std::string& type, long long int& size)
    {
        // Determine the field data type
        type = readStringField();
        type.erase(0, type.find('=')+1);
        size = 0;

        // Read the data block depending on the
        // data type and convert the pointer to
        // a void*
        if (type == "DOUBLE")
        {
            double* data = readNumBlock<double>(size);
            return (void*)data;
        }
        else if (type == "STRING")
        {
            string* data = readStringBlock(size);
            return (void*)data;
        }

        return nullptr;
    }

    // This member function will delete the data array
    // obtained from the generic fields but convert
    // them into their original type first, because
    // deleting of void* is undefined (the length of
    // the field in memory is not defined)
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

    // This member function is an overload for the
    // assignment operator. It extends the already
    // available assignment operator from the parent
    // class
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

    // This simple member function returns the
    // version string associated with the current
    // file type
    std::string NumeReDataFile::getVersionString()
    {
        return toString((int)versionMajor) + "." + toString((int)versionMinor) + "." + toString((int)versionBuild);
    }


    //////////////////////////////////////////////
    // class CacheFile
    //////////////////////////////////////////////
    //
    CacheFile::CacheFile(const string& filename) : NumeReDataFile(filename), nIndexPos(0u)
    {
        // Empty constructor
    }

    // This destructor will write the offsets for
    // the different tables to the file before
    // the file stream will be closed
    CacheFile::~CacheFile()
    {

        if (nIndexPos && vFileIndex.size())
        {
            seekp(nIndexPos);
            writeNumBlock(&vFileIndex[0], vFileIndex.size());
        }
    }

    // This member function will reset the string
    // information and the internal storage. This
    // is used before the next table will be read
    // from the cache file to memory
    void CacheFile::reset()
    {
        sComment.clear();
        sTableName.clear();
        clearStorage();
    }

    // This member function will read the next table,
    // which is available in the cache file, to the
    // internal storage. It uses the "readFile()"
    // function from its parent class
    void CacheFile::readSome()
    {
        reset();

        if (vFileIndex.size() && !fFileStream.eof())
            readFile();
    }

    // This member function will write the current
    // contents in the internal storage to the
    // target file. Before writing, the function
    // stores the current byte position in the
    // target file to create the file index. The
    // function uses the function "writeFile()"
    // from the parent class for writing the data
    void CacheFile::writeSome()
    {
        if (vFileIndex.size())
        {
            // Store the current position in the
            // file in the file index array
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

    // This member function will read the cache file
    // header and ensure that the version of the
    // file is not newer than expected
    void CacheFile::readCacheHeader()
    {
        // Open the file in binary mode
        open(ios::binary | ios::in);

        // Read the basic information
        versionMajor = readNumField<long int>();
        versionMinor = readNumField<long int>();
        versionBuild = readNumField<long int>();
        timeStamp = readNumField<time_t>();

        // Ensure that this file contains
        // the "NUMERECACHEFILE" string
        if (readStringField() != "NUMERECACHEFILE")
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Read the file version information
        short fileVerMajor = readNumField<short>();
        short fileVerMinor = readNumField<short>();

        // Ensure that the file major version is
        // not larger than the one currently
        // implemented in this class
        if (fileVerMajor > fileVersionMajor)
            throw SyntaxError(SyntaxError::INSUFFICIENT_NUMERE_VERSION, sFileName, SyntaxError::invalid_position, sFileName);

        // Read the number of available tables
        // in the cache file
        size_t nNumberOfTables = readNumField<size_t>();

        // Create the file index array. This array
        // may be used to support memory paging
        // in a future version of NumeRe
        vFileIndex = vector<size_t>(nNumberOfTables, 0u);

        // Read the file index information in
        // the file to the newly created file
        // index array
        long long int size = 0;
        size_t* nIndex = readNumBlock<size_t>(size);
        copyArray(nIndex, &vFileIndex[0], nNumberOfTables);
        delete[] nIndex;
    }

    // This member function will write the standard
    // cache file header to the cache file
    void CacheFile::writeCacheHeader()
    {
        // Open the file in binary mode and truncate
        // all its contents
        open(ios::binary | ios::out | ios::trunc);

        writeNumField(AutoVersion::MAJOR);
        writeNumField(AutoVersion::MINOR);
        writeNumField(AutoVersion::BUILD);
        writeNumField(time(0));
        writeStringField("NUMERECACHEFILE");
        writeNumField(fileVersionMajor);
        writeNumField(fileVersionMinor);

        // Write the length of the file index array
        writeNumField(vFileIndex.size());

        // Store the current position in the file
        // to update the array in the future (done
        // in the destructor)
        nIndexPos = tellp();

        // Write the file index to the file
        writeNumBlock(&vFileIndex[0], vFileIndex.size());
    }


    //////////////////////////////////////////////
    // class CassyLabx
    //////////////////////////////////////////////
    //
    CassyLabx::CassyLabx(const string& filename) : GenericFile(filename)
    {
        // Empty constructor
    }

    CassyLabx::~CassyLabx()
    {
        // Empty destructor
    }

    // This member function will read the contents
    // of the associated LABX file to the internal
    // storage
    void CassyLabx::readFile()
    {
        // Open the filestream in text mode
        open(ios::in);

        string sLabx = "";
        string sLabx_substr = "";
        long long int nLine = 0;

        // Get the whole content of the file in a
        // single string
        while (!fFileStream.eof())
        {
            getline(fFileStream, sLabx_substr);
            StripSpaces(sLabx_substr);
            sLabx += sLabx_substr;
        }

        // Ensure that the least minimal information
        // is available in the file
        if (!sLabx.length() || sLabx.find("<allchannels count=") == string::npos)
            throw SyntaxError(SyntaxError::DATAFILE_NOT_EXIST, "", SyntaxError::invalid_position, sFileName);

        // Extract the information on the number of
        // data sets in the file
        sLabx_substr = sLabx.substr(sLabx.find("<allchannels count="));
        sLabx_substr = sLabx_substr.substr(sLabx_substr.find("=\"")+2, sLabx_substr.find("\">")-sLabx_substr.find("=\"")-2);
        nCols = StrToInt(sLabx_substr);

        // Ensure that there is at least one column
        // of data available
        if (!nCols)
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        vector<string> vHeadLines;
        vector<string> vCols;

        // Start extracting the actual data from the
        // file
        sLabx_substr = sLabx.substr(sLabx.find("<allchannels"), sLabx.find("</allchannels>")-sLabx.find("<allchannels"));
        sLabx_substr = sLabx_substr.substr(sLabx_substr.find("<channels"));

        // Distribute the different data columns
        // across the previously created vector
        for (long long int i = 0; i < nCols; i++)
        {
            // Push the current column to the
            // column vector
            vCols.push_back(sLabx_substr.substr(sLabx_substr.find("<values"), sLabx_substr.find("</channel>")-sLabx_substr.find("<values")));

            // Store the headline of the current
            // column in the corresponding
            // headline vector. Consider also the
            // possible available unit
            if (sLabx_substr.find("<unit />") != string::npos && sLabx_substr.find("<unit />") < sLabx_substr.find("<unit>"))
                vHeadLines.push_back(sLabx_substr.substr(sLabx_substr.find("<quantity>")+10, sLabx_substr.find("</quantity>")-sLabx_substr.find("<quantity>")-10));
            else
            {
                vHeadLines.push_back(sLabx_substr.substr(sLabx_substr.find("<quantity>")+10, sLabx_substr.find("</quantity>")-sLabx_substr.find("<quantity>")-10)
                    + "_[" + sLabx_substr.substr(sLabx_substr.find("<unit>")+6, sLabx_substr.find("</unit>")-sLabx_substr.find("<unit>")-6) + "]");
            }

            // Convert UTF8 to WinCP1252 and erase the
            // already extracted part from the string
            vHeadLines.back() = utf8parser(vHeadLines.back());
            sLabx_substr.erase(0, sLabx_substr.find("</channels>")+11);

            // Determine the maximal number of rows needed
            // for the overall data table
            if (StrToInt(vCols[i].substr(vCols[i].find("count=\"")+7, vCols[i].find("\">")-vCols[i].find("count=\"")-7)) > nLine)
                nLine = StrToInt(vCols[i].substr(vCols[i].find("count=\"")+7, vCols[i].find("\">")-vCols[i].find("count=\"")-7));
        }

        // Ensure that the columns are not empty
        if (!nLine)
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        nRows = nLine;

        // Prepare the internal storage and copy
        // the already decoded headlines
        createStorage();
        copyStringArray(&vHeadLines[0], fileTableHeads, vHeadLines.size());
        long long int nElements = 0;

        // Extract the numerical values directly
        // from the XML string into the internal
        // storage
        for (long long int i = 0; i < nCols; i++)
        {
            // Only do something, if the current
            // column contains some values
            if (vCols[i].find("<values count=\"0\" />") == string::npos)
            {
                // Get the number of elements in
                // the current column
                nElements = StrToInt(vCols[i].substr(vCols[i].find('"')+1, vCols[i].find('"', vCols[i].find('"')+1)-1-vCols[i].find('"')));
                vCols[i].erase(0, vCols[i].find('>')+1);

                // Copy the elements into the internal
                // storage
                for (long long int j = 0; j < min(nElements, nRows); j++)
                {
                    fileData[j][i] = extractValueFromTag(vCols[i].substr(vCols[i].find("<value"), vCols[i].find('<', vCols[i].find('/'))-vCols[i].find("<value")));
                    vCols[i].erase(vCols[i].find('>', vCols[i].find('/'))+1);
                }
            }
        }

    }

    // This simple member function extracts the
    // numerical value of the XML tag string
    double CassyLabx::extractValueFromTag(const string& sTag)
    {
        return StrToDb(sTag.substr(7, sTag.find('<', 7)-7));
    }


    //////////////////////////////////////////////
    // class CommaSeparatedValues
    //////////////////////////////////////////////
    //
    CommaSeparatedValues::CommaSeparatedValues(const string& filename) : GenericFile(filename)
    {
        // Empty constructor
    }

    CommaSeparatedValues::~CommaSeparatedValues()
    {
        // Empty destructor
    }

    // This member function is used to read the
    // target file to the internal storage
    void CommaSeparatedValues::readFile()
    {
        // Open the file stream in text mode
        open(ios::in);

        // Create the needed variabels
		char cSep = 0;
        long long int nLine = 0;
		long long int nComment = 0;
		vector<string> vHeadLine;

		// Read the whole file to a vector and
		// get the number of lines available in
		// the data file
		vector<string> vFileData = readTextFile(true);
		nLine = vFileData.size();

		// Ensure that there is at least one
		// line available
		if (!nLine)
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Determine, which character is used
        // as cell separator
		cSep = findSeparator(vFileData);

		// Ensure that we were able to determine
		// the separator
		if (!cSep)
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Count the number of columns available
        // in the file and ensure that the identified
        // separator character is correct
        countColumns(vFileData, cSep);

        vHeadLine.resize(nCols);

        // Define the set of valid letters numeric and
        // append the separator character
        string sValidSymbols = "0123456789.,;-+eE INFAinfa";
        sValidSymbols += cSep;

        // Search for non-numeric characters in the first
        // line of the file. Extract the possible table
        // column heads and note, when the table heads were
        // extracted from the file
        if (vFileData[0].find_first_not_of(sValidSymbols) != string::npos)
        {
            // Tokenize the current line using the
            // separator character
            vector<string> vTokens = tokenize(vFileData[0], string(1, cSep));

            for (size_t i = 0; i < vTokens.size(); i++)
            {
                if (vTokens[i].find_first_not_of(sValidSymbols) == string::npos)
                {
                    vTokens.clear();
                    break;
                }
            }

            // Copy the tokenized strings to the target
            // table column headline vector
            for (size_t n = 0; n < (vTokens.size() < nCols ? vTokens.size() : nCols); n++)
            {
                vHeadLine[n] = utf8parser(vTokens[n]);
                StripSpaces(vHeadLine[n]);
            }

            // Clear the first line, if the tokens were
            // not erased before
            if (vTokens.size())
                vFileData[0].clear();

            nComment++;
        }

        // Store the number of lines of the file,
        // which contain numeric data, in the rows
        // variable
        if (nComment)
            nRows = nLine - 1;
        else
            nRows = nLine;

        // Prepare the internal storage
        createStorage();

        // Copy the already decoded table heads to
        // the internal storage or create a dummy
        // column head, if the data contains only
        // one column
		if (nComment)
            copyStringArray(&vHeadLine[0], fileTableHeads, vHeadLine.size());
		else
		{
		    // If the file contains only one column,
		    // then we'll use the file name as table
		    // column head
			if (nCols == 1)
			{
                if (sFileName.find('/') == string::npos)
                    fileTableHeads[0] = sFileName.substr(0, sFileName.rfind('.'));
                else
                    fileTableHeads[0] = sFileName.substr(sFileName.rfind('/')+1, sFileName.rfind('.')-1-sFileName.rfind('/'));
			}
		}

		// Decode now the whole data table into single
		// tokens, which can be distributed into their
		// corresponding cells
		for (size_t i = nComment; i < vFileData.size(); i++)
        {
            // Ignore empty lines
            if (!vFileData[i].length())
                continue;

            // Tokenize the current line
            vector<string> vTokens = tokenize(vFileData[i], string(1, cSep));

            // Decode each token
            for (size_t j = 0; j < vTokens.size(); j++)
            {
                // Handle special values first
                if (!vTokens[j].length()
                    || vTokens[j] == "---"
                    || toLowerCase(vTokens[j]) == "nan")
                {
                    fileData[i][j] = NAN;
                }
                else if (toLowerCase(vTokens[j]) == "inf")
                    fileData[i][j] = INFINITY;
                else if (toLowerCase(vTokens[j]) == "-inf")
                    fileData[i][j] = -INFINITY;
                else
                {
                    // In all other cases, ensure that the current
                    // token is not a string token, which should
                    // be stored in the table column header
                    if (vTokens[j].find_first_not_of(sValidSymbols) != string::npos)
                    {
                        // Append a linebreak character, if the
                        // current table head is not empty
                        if (fileTableHeads[j].length())
                            fileTableHeads[j] += "\\n";

                        fileTableHeads[j] += vTokens[j];
                        fileData[i][j] = NAN;
                    }
                    else
                    {
                        replaceDecimalSign(vTokens[j]);
                        fileData[i][j] = StrToDb(vTokens[j]);
                    }
                }
            }
        }
    }

    // This member function is used to write the
    // contents in the internal storage to the
    // target file
    void CommaSeparatedValues::writeFile()
    {
        // Open the file in text mode and truncate
        // all its contents
        open(ios::out | ios::trunc);

        // Write the table heads to the file
        for (long long int j = 0; j < nCols; j++)
        {
            fFileStream << fileTableHeads[j] + ",";
        }

        fFileStream << "\n";
        fFileStream.precision(nPrecFields);

        // Write the data to the file
        for (long long int i = 0; i < nRows; i++)
        {
            for (long long int j = 0; j < nCols; j++)
            {
                if (!isnan(fileData[i][j]))
                    fFileStream << fileData[i][j];

                fFileStream << ",";
            }

            fFileStream << "\n";
        }

        fFileStream.flush();
    }

    // This member function determines the
    // separator character used for the current
    // file name. It does so by some simple
    // heuristic: the most common characters
    // are checked first and the uncommon ones
    // afterwards
    char CommaSeparatedValues::findSeparator(const vector<string>& vTextData)
    {
        char cSep = 0;

        if (vTextData[0].find('.') != string::npos && vTextData[0].find(',') != string::npos && vTextData[0].find('\t') != string::npos)
            cSep = ',';
		else if (vTextData[0].find(';') != string::npos && (vTextData[0].find(',') != string::npos || vTextData[0].find('.') != string::npos) && vTextData[0].find('\t') != string::npos)
            cSep = ';';
		else if (vTextData[0].find('\t') != string::npos)
            cSep = '\t';
        else if (vTextData[0].find(';') != string::npos)
            cSep = ';';
        else if (vTextData[0].find(',') != string::npos)
            cSep = ',';
        else if (vTextData[0].find(' ') != string::npos && vTextData.size() > 1 && vTextData[1].find(' ') != string::npos)
            cSep = ' ';
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

            // To ensure the selection of the current
            // separator character, we try to determine
            // the number of columns. If the this number
            // does not alter between the lines, then we
            // found the correct character
            for (size_t i = 0; i < vTextData.size(); i++)
            {
                size_t nCol = 1;

                for (size_t j = 0; j < vTextData[i].length(); j++)
                {
                    if (vTextData[i][j] == cSep)
                        nCol++;
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
                    return cSep;

                if (i+1 == vTextData.size())
                    return 0;
            }
        }

        return cSep;
    }

    // This member function determines the number of
    // columns available in the current file and alters
    // the separator character, if the column counts
    // are not consistent between the different lines
    void CommaSeparatedValues::countColumns(const std::vector<std::string>& vTextData, char& cSep)
    {
        long long int nCol;

        for (size_t i = 0; i < vTextData.size(); i++)
        {
            nCol = 1;

            for (size_t j = 0; j < vTextData[i].length(); j++)
            {
                if (vTextData[i][j] == cSep)
                    nCol++;
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
    // class LaTeXTable
    //////////////////////////////////////////////
    //
    LaTeXTable::LaTeXTable(const string& filename) : GenericFile(filename)
    {
        // Empty constructor
    }

    LaTeXTable::~LaTeXTable()
    {
        // Empty destructor
    }

    // This member function is used to
    // write the contents of the internal
    // storage to the file
    void LaTeXTable::writeFile()
    {
        // Open the file in text mode and
        // truncate all its contents
        open(ios::trunc | ios::out);

        // Prepare a label for the table
        string sLabel = sFileName;

        if (sLabel.find('/') != string::npos)
            sLabel.erase(0, sLabel.rfind('/')+1);

        if (sLabel.find(".tex") != string::npos)
            sLabel.erase(sLabel.rfind(".tex"));

        while (sLabel.find(' ') != string::npos)
            sLabel[sLabel.find(' ')] = '_';

        // Write the legal header stuff
        writeHeader();

        // Depending on the number of lines
        // in the current table, the layout
        // switches between usual and longtables
        //
        // Write the table column headers and
        // all needed layout stuff
        if (nRows < 30)
        {
            fFileStream << "\\begin{table}[htb]\n";
            fFileStream << "\\centering\n";
            string sPrint = "\\begin{tabular}{";

            for (long long int j = 0; j < nCols; j++)
                sPrint += "c";

            sPrint += "}";
            fFileStream << sPrint + "\n";
            fFileStream << "\\toprule\n";

            writeTableHeads();
        }
        else
        {
            string sPrint = "\\begin{longtable}{";

            for (long long int j = 0; j < nCols; j++)
                sPrint += "c";

            sPrint += "}";
            fFileStream << sPrint + "\n";
            fFileStream << "\\caption{" + _lang.get("OUTPUT_FORMAT_TEX_HEAD", "NumeRe")+"}\n";
            fFileStream << "\\label{tab:" + sLabel + "}\\\\\n";
            fFileStream << "\\toprule\n";

            writeTableHeads();

            fFileStream << sPrint + "\n";
            fFileStream << "\\midrule\n";
            fFileStream << "\\endfirsthead\n";
            fFileStream << "\\caption{"+_lang.get("OUTPUT_FORMAT_TEXLONG_CAPTION")+"}\\\\\n";
            fFileStream << "\\toprule\n";
            fFileStream << sPrint;
            fFileStream << "\\midrule\n";
            fFileStream << "\\endhead\n";
            fFileStream << "\\midrule\n";
            fFileStream << "\\multicolumn{" + toString(nCols) + "}{c}{--- \\emph{"+_lang.get("OUTPUT_FORMAT_TEXLONG_FOOT")+"} ---}\\\\\n";
            fFileStream << "\\bottomrule\n";
            fFileStream << "\\endfoot\n";
            fFileStream << "\\bottomrule\n";
            fFileStream << "\\endlastfoot\n";
        }

        // Write the actual data formatted as
        // LaTeX numbers into the file and insert
        // the needed column separators
        for (long long int i = 0; i < nRows; i++)
        {
            for (long long int j = 0; j < nCols; j++)
            {
                fFileStream.width(nPrecFields+20);
                fFileStream.fill(' ');

                fFileStream << formatNumber(fileData[i][j]);

                if (j+1 < nCols)
                    fFileStream << " &";
                else
                    fFileStream << "\\\n";
            }
        }

        // Finalize the table. This also depends
        // on the number of lines in the internal
        // storage
        if (nRows < 30)
        {
            fFileStream << "\\bottomrule\n";
            fFileStream << "\\end{tabular}\n";
            fFileStream << "\\caption{"+ _lang.get("OUTPUT_FORMAT_TEX_HEAD", "NumeRe")+"}\n";
            fFileStream << "\\label{tab:" + sLabel + "}\n";
            fFileStream << "\\end{table}\n";
        }
        else
        {
            fFileStream << "\\end{longtable}\n";
        }

        fFileStream << flush;
    }

    // This member function writes the
    // legal header to the file
    void LaTeXTable::writeHeader()
    {
        string sBuild = AutoVersion::YEAR;
        sBuild += "-";
        sBuild += AutoVersion::MONTH;
        sBuild += "-";
        sBuild += AutoVersion::DATE;

        fFileStream << "%\n";
        fFileStream << "% " + _lang.get("OUTPUT_PRINTLEGAL_LINE1") << "\n";
        fFileStream << "% NumeRe: Framework für Numerische Rechnungen" << "\n";
        fFileStream << "%=============================================" << "\n";
        fFileStream << "% " + _lang.get("OUTPUT_PRINTLEGAL_LINE2", sVersion, sBuild) << "\n";
        fFileStream << "% " + _lang.get("OUTPUT_PRINTLEGAL_LINE3", sBuild.substr(0, 4)) << "\n";
        fFileStream << "%\n";
        fFileStream << "% " + _lang.get("OUTPUT_PRINTLEGAL_LINE4", getTimeStamp(false)) << "\n";
        fFileStream << "%\n";
        fFileStream << "% " + _lang.get("OUTPUT_PRINTLEGAL_TEX") << "\n%" << endl;
    }

    // This member funciton writes the
    // table column heads to the file. The
    // number of lines needed for the
    // heads is considered in this case
    void LaTeXTable::writeTableHeads()
    {
        string sPrint = "";

        // Copy the contents to the string and
        // consider the number of lines needed
        // for the headlines
        for (size_t i = 0; i < countHeadLines(); i++)
        {
            for (long long int j = 0; j < nCols; j++)
                sPrint += getLineFromHead(j, i) + " & ";

            sPrint = sPrint.substr(0, sPrint.length()-2) + "\\\\\n";
        }

        // Remove the underscores from the whole
        // string, because they would interfere
        // with LaTeX
        for (size_t i = 0; i < sPrint.length(); i++)
        {
            if (sPrint[i] == '_')
                sPrint[i] = ' ';
        }

        fFileStream << sPrint;
    }

    // This member function calculates the
    // number of lines needed for the complete
    // table column heads
    size_t LaTeXTable::countHeadLines()
    {
        size_t headlines = 0u;

        // Get the cell extents of each table
        // column head and store the maximal
        // value
        for (long long int i = 0; i < nCols; i++)
        {
            auto extents = calculateCellExtents(fileTableHeads[i]);

            if (extents.second > headlines)
                headlines = extents.second;
        }

        return headlines;
    }

    // This member function replaces all
    // non-ASCII characters into their
    // corresponding LaTeX entities
    string LaTeXTable::replaceNonASCII(const string& _sText)
    {
        string sReturn = _sText;

        for (unsigned int i = 0; i < sReturn.length(); i++)
        {
            if (sReturn[i] == 'Ä' || sReturn[i] == (char)142)
                sReturn.replace(i,1,"\\\"A");

            if (sReturn[i] == 'ä' || sReturn[i] == (char)132)
                sReturn.replace(i,1,"\\\"a");

            if (sReturn[i] == 'Ö' || sReturn[i] == (char)153)
                sReturn.replace(i,1,"\\\"O");

            if (sReturn[i] == 'ö' || sReturn[i] == (char)148)
                sReturn.replace(i,1,"\\\"o");

            if (sReturn[i] == 'Ü' || sReturn[i] == (char)154)
                sReturn.replace(i,1,"\\\"U");

            if (sReturn[i] == 'ü' || sReturn[i] == (char)129)
                sReturn.replace(i,1,"\\\"u");

            if (sReturn[i] == 'ß' || sReturn[i] == (char)225)
                sReturn.replace(i,1,"\\ss ");

            if (sReturn[i] == '°' || sReturn[i] == (char)248)
                sReturn.replace(i,1,"$^\\circ$");

            if (sReturn[i] == (char)196 || sReturn[i] == (char)249)
                sReturn.replace(i,1,"\\pm ");

            if (sReturn[i] == (char)171 || sReturn[i] == (char)174)
                sReturn.replace(i,1,"\"<");

            if (sReturn[i] == (char)187 || sReturn[i] == (char)175)
                sReturn.replace(i,1,"\">");

            if ((!i && sReturn[i] == '_') || (i && sReturn[i] == '_' && sReturn[i-1] != '\\'))
                sReturn.insert(i,1,'\\');
        }

        if (sReturn.find("+/-") != string::npos)
        {
            sReturn = sReturn.substr(0, sReturn.find("+/-"))
                    + "$\\pm$"
                    + sReturn.substr(sReturn.find("+/-")+3);
        }

        return sReturn;
    }

    // This member function formats a double
    // as LaTeX number string.
    string LaTeXTable::formatNumber(double number)
    {
        string sNumber = toString(number, nPrecFields);

        // Handle floating point numbers with
        // exponents correctly
        if (sNumber.find('e') != string::npos)
        {
            sNumber = sNumber.substr(0, sNumber.find('e'))
                    + "\\cdot10^{"
                    + (sNumber[sNumber.find('e')+1] == '-' ? "-" : "")
                    + sNumber.substr(sNumber.find_first_not_of('0', sNumber.find('e')+2))
                    + "}";
        }

        // Consider some special values
        if (sNumber == "inf")
            sNumber = "\\infty";

        if (sNumber == "-inf")
            sNumber = "-\\infty";

        if (sNumber == "nan")
            return "---";

        // Return the formatted string in math mode
        return "$" + sNumber + "$";
    }


    //////////////////////////////////////////////
    // class JcampDX
    //////////////////////////////////////////////
    //
    JcampDX::JcampDX(const string& filename) : GenericFile(filename)
    {
        // Empty constructor
    }

    JcampDX::~JcampDX()
    {
        // Empty destructor
    }

    // This member function is used to read the
    // contents of the JCAMP-DX file to the
    // internal storage
    void JcampDX::readFile()
    {
        // Open the file in text mode
        open(ios::in);

        // Create some temporary buffer variables
		long long int nComment = 0;
		vector<double> vLine;

		// Read the contents of the file to the
		// vector variable
		vector<string> vFileContents = readTextFile(true);

		// Ensure that contents are available
		if (!vFileContents.size())
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Find the end tag in the file and omit
        // all following lines
		for (size_t i = 0; i < vFileContents.size(); i++)
        {
            stripTrailingSpaces(vFileContents[i]);
            parseLabel(vFileContents[i]);

            // Erase everything after the end tag
            if (vFileContents[i].substr(0, 6) == "##END=")
            {
                vFileContents.erase(vFileContents.begin()+i+1, vFileContents.end());
                break;
            }
        }

        // Count the number of labels (comments) in
        // the current file
        for (unsigned int i = 0; i < vFileContents.size(); i++)
        {
            if (vFileContents[i].substr(0,2) == "##")
                nComment++;
        }

        // Determine the dimensions of the data
        // set using the comments and the decoded
        // last line of the tags (we ignore the closing
        // "##END=" tag
        nRows = vFileContents.size() - nComment;
        nCols = parseLine(vFileContents[nComment-1]).size();

        // Prepare the internal storage
        createStorage();

        // Prepare some decoding variables
        double dXFactor = 1.0;
        double dYFactor = 1.0;

        string sXUnit = "";
        string sYUnit = "";

        string sDataType = "";
        string sXYScheme = "";

        // Go through the label section first
        // and decode the header information of
        // the data set
        for (long long int j = 0; j < nComment-1; j++)
        {
            // Omit comments
            if (vFileContents[j].find("$$") != string::npos)
                vFileContents[j].erase(vFileContents[j].find("$$"));

            // Get the x and y scaling factors
            if (vFileContents[j].substr(0,10) == "##XFACTOR=")
                dXFactor = StrToDb(vFileContents[j].substr(10));

            if (vFileContents[j].substr(0,10) == "##YFACTOR=")
                dYFactor = StrToDb(vFileContents[j].substr(10));

            // Extract the x units
            if (vFileContents[j].substr(0,9) == "##XUNITS=")
            {
                sXUnit = vFileContents[j].substr(9);
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

            // Extract the y units
            if (vFileContents[j].substr(0,9) == "##YUNITS=")
            {
                sYUnit = vFileContents[j].substr(9);
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

            // Get the data type (currently unused)
            if (vFileContents[j].substr(0,11) == "##DATATYPE=")
            {
                sDataType = vFileContents[j].substr(11);
                StripSpaces(sDataType);
            }

            // Get the data encoding scheme (currently
            // unused as well)
            if (vFileContents[j].substr(0,11) == "##XYPOINTS=")
            {
                sXYScheme = vFileContents[j].substr(11);
                StripSpaces(sXYScheme);
            }
        }

        // Now go through the actual data section
        // of the file and convert it into
        // numerical values
        for (long long int j = nComment-1; j < vFileContents.size() - 1; j++)
        {
            // Ignore lables
            if (vFileContents[j].substr(0, 2) == "##")
                continue;

            // Abort at the end tag
            if (vFileContents[j].substr(0, 6) == "##END=")
                break;

            // Abort, if we read enough lines
            if (nComment + 1 == nRows)
                break;

            // Decode the current line
            vLine = parseLine(vFileContents[j]);

            for (unsigned int k = 0; k < vLine.size(); k++)
            {
                if (k == nCols)
                    break;

                // Store the table column heads
                if (j == nComment-1)
                    fileTableHeads[k] = (k % 2 ? sYUnit : sXUnit);

                // Write the data to the internal
                // storage
                fileData[j-nComment+1][k] = vLine[k] * (k % 2 ? dYFactor : dXFactor);
            }
        }
    }

    // This member function parses JCAMP-DX labels
    // by removing whitespaces, minus characters and
    // underscores from the label name itself and
    // converting it into upper case
    void JcampDX::parseLabel(string& sLine)
    {
        if (sLine.find("##") == string::npos || sLine.find('=') == string::npos)
            return;

        for (size_t i = 0; i < sLine.length(); i++)
        {
            // Remove some characters
            if (sLine[i] == ' ' || sLine[i] == '-' || sLine[i] == '_')
            {
                sLine.erase(i, 1);
                i--;
            }

            // Transform to uppercase
            if (sLine[i] >= 'a' && sLine[i] <= 'z')
                sLine[i] += 'A'-'a';

            // Stop at the equal sign
            if (sLine[i] == '=')
                break;
        }
    }

    // This member function parses the current data
    // line into numerical values by decoding the
    // JCAMP-DX encoding scheme. The JCAMP-DX format
    // is a textual data format, but the text data may
    // compressed to save some storage space.
    // Reference: http://www.jcamp-dx.org/protocols.html
    vector<double> JcampDX::parseLine(const string& sLine)
    {
        vector<double> vLine;
        string sValue = "";
        const string sNumericChars = "0123456789.+-";

        // Go through the complete line and uncompress
        // the data into usual text strings, which will
        // be stored in another string
        for (unsigned int i = 0; i < sLine.length(); i++)
        {
            // The first three cases are the simplest
            // cases, where the data is not compressed
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
                // This section is for compressed data
                // DIFDUP form. Only the first character
                // is compressed
                if (sValue.length())
                {
                    if ((sValue[0] >= 'J' && sValue[0] <= 'R'))
                    {
                        // Positive DIF digits
                        sValue[0] = toString(sValue[0]-'J'+1)[0];
                        vLine.push_back(vLine.back()+StrToDb(sValue));
                    }
                    else if ((sValue[0] >= 'j' && sValue[0] <= 'r'))
                    {
                        // Negative DIF digits
                        sValue[0] = toString(sValue[0]-'j'+1)[0];
                        vLine.push_back(vLine.back()-StrToDb(sValue));
                    }
                    else if (sValue[0] == '%')
                    {
                        // A zero
                        sValue[0] = '0';
                        vLine.push_back(vLine.back()+StrToDb(sValue));
                    }
                    else if ((sValue[0] >= 'S' && sValue[0] >= 'Z') || sValue[0] == 's')
                    {
                        // DUP digits
                        if (sValue[0] == 's')
                        {
                            for (int j = 0; j < 9; j++)
                                vLine.push_back(vLine.back());
                        }
                        else
                        {
                            for (int j = 0; j <= sValue[0]-'S'; j++)
                                vLine.push_back(vLine.back());
                        }
                    }
                    else
                        vLine.push_back(StrToDb(sValue)); // Simply convert into a double

                    sValue.clear();
                }

                // We convert SQZ digits directly in
                // their plain format
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

        // This section is for compressed data
        // DIFDUP form. Only the first character
        // is compressed
        if (sValue.length())
        {
            if ((sValue[0] >= 'J' && sValue[0] <= 'R'))
            {
                // Positive DIF digits
                sValue[0] = toString(sValue[0]-'J'+1)[0];
                vLine.push_back(vLine.back()+StrToDb(sValue));
            }
            else if ((sValue[0] >= 'j' && sValue[0] <= 'r'))
            {
                // Negative DIF digits
                sValue[0] = toString(sValue[0]-'j'+1)[0];
                vLine.push_back(vLine.back()-StrToDb(sValue));
            }
            else if (sValue[0] == '%')
            {
                // A zero
                sValue[0] = '0';
                vLine.push_back(vLine.back()+StrToDb(sValue));
            }
            else if ((sValue[0] >= 'S' && sValue[0] >= 'Z') || sValue[0] == 's')
            {
                // DUP digits
                if (sValue[0] == 's')
                {
                    for (int j = 0; j < 9; j++)
                        vLine.push_back(vLine.back());
                }
                else
                {
                    for (int j = 0; j <= sValue[0]-'S'; j++)
                        vLine.push_back(vLine.back());
                }
            }
            else
                vLine.push_back(StrToDb(sValue)); // Simply convert into a double

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
        // Empty constructor
    }

    OpenDocumentSpreadSheet::~OpenDocumentSpreadSheet()
    {
        // Empty destructor
    }

    // This member function is used to read the
    // target file into the internal storage. ODS
    // is a ZIP file containing the data formatted
    // as XML
    void OpenDocumentSpreadSheet::readFile()
    {
        string sODS = "";
        string sODS_substr = "";
        vector<string> vTables;
        vector<string> vMatrix;
        long long int nCommentLines = 0;
        long long int nMaxCols = 0;

        // Get the contents of the embedded
        // XML file
        sODS = getZipFileItem("content.xml");

        // Ensure that the embedded file is
        // not empty
        if (!sODS.length())
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Remove the obsolete beginning of
        // the file, which we don't need
        sODS.erase(0, sODS.find("<office:spreadsheet>"));

        // Ensure again that the file is not empty
        if (!sODS.length())
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Remove the part until the first table
        sODS.erase(0, sODS.find("<table:table "));

        // Extract the different tables from the
        // whole string and store them in different
        // vector components
        while (sODS.size() && sODS.find("<table:table ") != string::npos && sODS.find("</table:table>") != string::npos)
        {
            vTables.push_back(sODS.substr(sODS.find("<table:table "), sODS.find("</table:table>")+14-sODS.find("<table:table ")));
            sODS.erase(sODS.find("<table:table "), sODS.find("</table:table>")+14-sODS.find("<table:table "));
        }

        // Ensure that we found at least a single
        // table
        if (!vTables.size())
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Decode all tables and store them next
        // to each other in the vMatrix vector
        // variable
        for (unsigned int i = 0; i < vTables.size(); i++)
        {
            unsigned int nPos = 0;
            unsigned int nCount = 0;
            long long int _nCols = 0;
            string sLine = "";

            // This section decodes a single table and stores
            // the lines in the vMatrix vector variable. If
            // this is not the first table, then the lines
            // are appended to the already existing ones
            while (vTables[i].find("<table:table-row ", nPos) != string::npos && vTables[i].find("</table:table-row>", nPos) != string::npos)
            {
                // Extract the next line from the current
                // table and expand the line
                nPos = vTables[i].find("<table:table-row ", nPos);
                sLine = vTables[i].substr(nPos, vTables[i].find("</table:table-row>", nPos)+18-nPos);
                sLine = expandLine(sLine);

                // If the line is not empty, store it
                // at the corresponding line in the vMatrix
                // variable
                if (sLine.length())
                {
                    if (!i) // first table
                        vMatrix.push_back(sLine);
                    else
                    {
                        // Extent the number if rows with
                        // empty cells, if the following tables
                        // contain more rows
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

            // Determine the current number of columns,
            // which are contained in the vMatrix variable
            // up to now
            for (unsigned int j = 0; j < vMatrix.size(); j++)
            {
                _nCols = 0;

                // Each opening left angle brace corresponds
                // to a single cell
                for (unsigned int n = 0; n < vMatrix[j].length(); n++)
                {
                    if (vMatrix[j][n] == '<')
                        _nCols++;
                }

                if (_nCols > nMaxCols)
                    nMaxCols = _nCols;
            }

            // Append empty cells to matrix rows, if their
            // number of columns is not sufficient to resemble
            // a full rectangle matrix
            for (unsigned int j = 0; j < vMatrix.size(); j++)
            {
                _nCols = 0;

                // Each opening left angle brace corresponds
                // to a single cell
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

        // Replace all whitespaces in the matrix
        // with underscores
        for (unsigned int i = 0; i < vMatrix.size(); i++)
        {
            while (vMatrix[i].find(' ') != string::npos)
                vMatrix[i].replace(vMatrix[i].find(' '), 1, "_");
        }

        // Ensure that the matrix is not empty
        if (!vMatrix.size() || !nMaxCols)
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Try to detect the number of (pure)
        // text lines in the matrix. Those will be
        // used as table column heads. If a single
        // cell is numeric, we do not use this row
        // as a text line
        for (unsigned int i = 0; i < vMatrix.size(); i++)
        {
            bool bBreak = false;

            for (unsigned int j = 0; j < vMatrix[i].length(); j++)
            {
                // Only examine non-empty cells
                if (vMatrix[i][j] == '<' && vMatrix[i][j+1] != '>')
                {
                    // If this cell is numeric, leave the
                    // whole loop
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

        // Set the dimensions of the final table
        // and create the internal storage
        nRows = vMatrix.size() - nCommentLines;
        nCols = nMaxCols;
        createStorage();

        unsigned int nPos = 0;

        // Store the pure text lines as table
        // column heads
        for (long long int i = 0; i < nCommentLines; i++)
        {
            nPos = 0;

            for (long long int j = 0; j < nCols; j++)
            {
                nPos = vMatrix[i].find('<', nPos);
                string sEntry = utf8parser(vMatrix[i].substr(nPos,vMatrix[i].find('>', nPos)+1-nPos));
                nPos++;

                // Omit empty cells
                if (sEntry == "<>")
                    continue;

                // Remove the left and right angles
                sEntry.erase(0, 1);
                sEntry.pop_back();

                if (!fileTableHeads[j].length())
                    fileTableHeads[j] = sEntry;
                else if (fileTableHeads[j] != sEntry)
                    fileTableHeads[j] += "\\n" + sEntry;
            }
        }

        // Store the actual data in the internal
        // storage. If we hit a text-only cell,
        // then we'll append it to the corresponding
        // table column head
        for (long long int i = 0; i < nRows; i++)
        {
            nPos = 0;

            for (long long int j = 0; j < nCols; j++)
            {
                nPos = vMatrix[i+nCommentLines].find('<', nPos);
                string sEntry = utf8parser(vMatrix[i+nCommentLines].substr(nPos,vMatrix[i+nCommentLines].find('>', nPos)+1-nPos));
                nPos++;

                // Omit empty cells
                if (sEntry == "<>")
                    continue;

                // Remove left and right angles
                sEntry.erase(0, 1);
                sEntry.pop_back();

                // Store the content as double value,
                // if it is numeric, and as part of
                // the table column head, otherwise
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

    // This member function is used by readFile()
    // to expand the XML-based table row string into
    // the intermediate cell format. Compressed cells
    // are extended as well
    string OpenDocumentSpreadSheet::expandLine(const string& sLine)
    {
        string sExpandedLine = "";

        for (unsigned int i = 0; i < sLine.length(); i++)
        {
            if (sLine.substr(i, 17) == "<table:table-cell")
            {
                if (sLine[sLine.find('>',i)-1] != '/')
                {
                    // Read the value of the current cell
                    string sCellEntry = sLine.substr(i, sLine.find("</table:table-cell>", i)-i);

                    // Extract the value into a simpler,
                    // intermediate cell format: "<VALUE>"
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
                        // If there are some empty cells,
                        // which are compressed, then we
                        // expand them here
                        string sTemp = getArgAtPos(sLine, sLine.find("table:number-columns-repeated=", i)+30);

                        if (sTemp.front() == '"')
                            sTemp.erase(0, 1);

                        if (sTemp.back() == '"')
                            sTemp.pop_back();

                        // Create the corresponding number
                        // of empty cells
                        for (int j = 0; j < StrToInt(sTemp); j++)
                            sExpandedLine += "<>";
                    }
                    else
                        sExpandedLine += "<>";
                }
            }
            else if (sLine.substr(i,25) == "<table:covered-table-cell")
            {
                // If there are some empty cells,
                // which are compressed, then we
                // expand them here
                string sTemp = getArgAtPos(sLine, sLine.find("table:number-columns-repeated=", i)+30);

                if (sTemp.front() == '"')
                    sTemp.erase(0, 1);

                if (sTemp.back() == '"')
                    sTemp.pop_back();

                // Create the corresponding number
                // of empty cells
                for (int j = 0; j < StrToInt(sTemp); j++)
                    sExpandedLine += "<>";
            }
        }

        // Remove all trailing empty cells from the
        // current line. If they are necessary to
        // create a rectangular table, then they
        // will be added again
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
        // Empty constructor
    }

    XLSSpreadSheet::~XLSSpreadSheet()
    {
        // Empty destructor
    }

    // This member function is used to read the
    // data from the XLS spreadsheet into the
    // internal storage
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

        // Ensure that the XLS file is readable
        if (!_excel.Load(sFileName.c_str()))
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // get the total number of sheets
        nSheets = _excel.GetTotalWorkSheets();

        // Ensure that we have at least a single
        // sheet in the file
        if (!nSheets)
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // get the total size needed for all sheets,
        // which will be stored next to each other. We
        // consider pure text lines as table column
        // heads
        for (unsigned int n = 0; n < nSheets; n++)
        {
            _sheet = _excel.GetWorksheet(n);
            bBreakSignal = false;
            nCommentLines = 0;

            for (unsigned int i = 0; i < _sheet->GetTotalRows(); i++)
            {
                for (unsigned int j = 0; j < _sheet->GetTotalCols(); j++)
                {
                    // Abort the loop, if we find a single
                    // non-textual cell in the current row
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

            // Find the maximal number of needed rows
            if (nExcelLines < _sheet->GetTotalRows()-nCommentLines)
                nExcelLines = _sheet->GetTotalRows()-nCommentLines;

            // Add the number of columns of the current
            // sheet to the total number of columns
            nExcelCols += _sheet->GetTotalCols();
        }

        // Set the dimensions of the needed internal
        // storage and create it
        nRows = nExcelLines;
        nCols = nExcelCols;
        createStorage();

        // Copy the pure text lines into the corresponding
        // table column heads
        for (unsigned int n = 0; n < nSheets; n++)
        {
            _sheet = _excel.GetWorksheet(n);
            nExcelCols = _sheet->GetTotalCols();
            nExcelLines = _sheet->GetTotalRows();

            // We only use the pure text lines from the top
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

                    // Replace line break characters with their
                    // corresponding masked character
                    while (sEntry.find('\n') != string::npos)
                        sEntry.replace(sEntry.find('\n'), 1, "\\n");

                    while (sEntry.find((char)10) != string::npos)
                        sEntry.replace(sEntry.find((char)10), 1, "\\n");

                    // Append the string to the current table
                    // column head, if it is not empty
                    if (!fileTableHeads[j+nOffset].length())
                        fileTableHeads[j+nOffset] = sEntry;
                    else if (fileTableHeads[j+nOffset] != sEntry)
                        fileTableHeads[j+nOffset] += "\\n" + sEntry;
                }
            }

            nOffset += nExcelCols;
        }

        nOffset = 0;

        // Copy now the data to the internal storage.
        // We consider textual cells as part of the
        // corresponding table column heads and append
        // them automatically
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

                    // Select the type of the cell and
                    // store the value, if it's a number
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

                    // If the entry is not empty, then it
                    // is a textual cell. We append it to
                    // the corresponding table column head
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

    // This member function is used to write
    // the data in the internal storage to the
    // target XLS spreadsheet
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
        // Empty constructor
    }

    XLSXSpreadSheet::~XLSXSpreadSheet()
    {
        // Empty destructor
    }

    // This member function is used to read the
    // data from the XLSX spreadsheet into the
    // internal storage. XLSX is a ZIP file
    // containing the data formatted as XML
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

        // We use TinyXML-2 as XML libary in this case
        tinyxml2::XMLDocument _workbook;
        tinyxml2::XMLDocument _sheet;
        tinyxml2::XMLDocument _strings;
        tinyxml2::XMLNode* _node;
        tinyxml2::XMLElement* _element;
        tinyxml2::XMLElement* _stringelement;

        // Get the content of the workbool XML file
        sEntry = getZipFileItem("xl/workbook.xml");

        // Ensure that a workbook XML is available
        if (!sEntry.length())
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Parse the file to obtain the number of
        // sheets, which are associated with this
        // workbook
        _workbook.Parse(sEntry.c_str());

        // Ensure that we could parse the file correctly
        if (_workbook.ErrorID())
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Search the first child of the "sheets" node
        // in the current workbook and count all siblings
        _node = _workbook.FirstChildElement()->FirstChildElement("sheets")->FirstChild();

        if (_node)
            nSheets++;

        while ((_node = _node->NextSibling()))
            nSheets++;

        // Ensure that we have at least one sheet in
        // the workbook
        if (!nSheets)
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Walk through the sheets and extract the
        // dimension info
        for (unsigned int i = 0; i < nSheets; i++)
        {
            // Get the file contents of the current
            // sheet
            sSheetContent = getZipFileItem("xl/worksheets/sheet"+toString(i+1)+".xml");

            // Ensure that the sheet is not empty
            if (!sSheetContent.length())
                throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

            // Parse the sheet and catch parsing
            // errors
            _sheet.Parse(sSheetContent.c_str());

            if (_sheet.ErrorID())
                throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

            // Get the dimensions of the current sheet
            _element = _sheet.FirstChildElement()->FirstChildElement("dimension");
            sCellLocation = _element->Attribute("ref");
            int nLinemin = 0, nLinemax = 0;
            int nColmin = 0, nColmax = 0;

            // Take care of comment lines => todo
            evalIndices(sCellLocation.substr(0, sCellLocation.find(':')), nLinemin, nColmin);
            evalIndices(sCellLocation.substr(sCellLocation.find(':') + 1), nLinemax, nColmax);

            vCommentLines.push_back(0);

            // Search the data section of the sheet
            _node = _sheet.FirstChildElement()->FirstChildElement("sheetData")->FirstChild();

            if (!_node)
                continue;

            // Find pure textual lines in the current
            // sheet, which will be used as table column
            // heads
            do
            {
                // Find the next cell
                _element = _node->ToElement()->FirstChildElement("c");

                do
                {
                    if (_element->Attribute("t"))
                    {
                        // If the attribute signalizes a
                        // non-string element, we abort here
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

            // Calculate the maximal number of needed
            // rows to store all sheets next to each
            // other
            if (nExcelLines < nLinemax-nLinemin+1-vCommentLines[i])
                nExcelLines = nLinemax-nLinemin+1-vCommentLines[i];

            // Add the number of columns to the total
            // number of columns
            nExcelCols += nColmax-nColmin+1;
        }

        // Set the dimensions of the final table
        nRows = nExcelLines;
        nCols = nExcelCols;

        // Allocate the memory
        createStorage();

        // Walk through the sheets and extract the
        // contents to memory
        //
        // Get the contents of the shared strings
        // XML file and parse it
        sStringsContent = getZipFileItem("xl/sharedStrings.xml");
        _strings.Parse(sStringsContent.c_str());

        // Go through all sheets
        for (unsigned int i = 0; i < nSheets; i++)
        {
            // Get the file contents of the current
            // sheet and parse it
            sSheetContent = getZipFileItem("xl/worksheets/sheet"+toString(i+1)+".xml");
            _sheet.Parse(sSheetContent.c_str());

            // Search the data section and the dimensions
            // of the current sheet
            _node = _sheet.FirstChildElement()->FirstChildElement("sheetData")->FirstChild();
            _element = _sheet.FirstChildElement()->FirstChildElement("dimension");

            // Ensure that data is available
            if (!_node)
                continue;

            // Extract the target indices
            sCellLocation = _element->Attribute("ref");
            evalIndices(sCellLocation.substr(0, sCellLocation.find(':')), nLinemin, nColmin);
            evalIndices(sCellLocation.substr(sCellLocation.find(':')+1), nLinemax, nColmax);

            // Go through each cell and store its
            // value at the correct position in the
            // final table. If we hit a textual cell,
            // then we store its contents as a table
            // column head
            do
            {
                _element = _node->ToElement()->FirstChildElement("c");

                if (!_element)
                    continue;

                // Go through the cells of the current
                // row
                do
                {
                    sCellLocation = _element->Attribute("r");
                    evalIndices(sCellLocation, nLine, nCol);
                    nCol -= nColmin;
                    nLine -= nLinemin;

                    if (nCol+nOffset >= nCols || nLine-vCommentLines[i] >= nRows)
                        continue;

                    // catch textual cells and store them
                    // in the corresponding table column
                    // head
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

                            // If the string is not empty, then
                            // we'll add it to the correct table
                            // column head
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

                    // If the current cell contains a value
                    // tag, then we'll obtain that value and
                    // store it in the target table
                    if (_element->FirstChildElement("v"))
                        _element->FirstChildElement("v")->QueryDoubleText(&fileData[nLine-vCommentLines[i]][nCol+nOffset]);
                }
                while ((_element = _element->NextSiblingElement()));
            }
            while ((_node = _node->NextSibling()));

            nOffset += nColmax-nColmin+1;
        }
    }

    // This member function converts the usual
    // Excel indices in to numerical ones
    void XLSXSpreadSheet::evalIndices(const string& _sIndices, int& nLine, int& nCol)
    {
        //A1 -> IV65536
        string sIndices = toUpperCase(_sIndices);

        for (size_t i = 0; i < sIndices.length(); i++)
        {
            // Find the first character,
            // which is a digit: that's a
            // line index
            if (isdigit(sIndices[i]))
            {
                // Convert the line index
                nLine = StrToInt(sIndices.substr(i))-1;

                // Convert the column index.
                // This index might be two
                // characters long
                if (i == 2)
                    nCol = (sIndices[0]-'A'+1)*26+sIndices[1]-'A';
                else if (i == 1)
                    nCol = sIndices[0]-'A';

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
        // Empty constructor
    }

    // This copy constructor extents the copy
    // constructor of the GenericFile class
    IgorBinaryWave::IgorBinaryWave(const IgorBinaryWave& file) : GenericFile(file)
    {
        bXZSlice = file.bXZSlice;
    }

    IgorBinaryWave::~IgorBinaryWave()
    {
        // Empty destructor
    }

    // This member function is used to read
    // the contents of the IBW file into the
    // internal storage. We use the IBW library
    // read the binary file
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

        // Try to open the target file
        if (CPOpenFile(sFileName.c_str(), 0, &_cp_file_ref))
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Try to read the data from the
        // file into the passed void*
        if (ReadWave(_cp_file_ref, &nType, &nPnts, nDim, dScalingFactorA, dScalingFactorB, &vWaveDataPtr, &cName))
        {
            CPCloseFile(_cp_file_ref);

            if (vWaveDataPtr != NULL)
                free(vWaveDataPtr);

            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);
        }

        // Detect complex data
        if (nType & NT_CMPLX)
            bReadComplexData = true;

        // Convert the pointer into the correct
        // type
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

        // Obtain the final dimensions of
        // the whol table. A three-dimensional
        // wave will be rolled out in the
        // third dimension
        nRows = nDim[0];
        nCols = nDim[1];

        if (nDim[2])
            nCols *= nDim[2];

        // Ensure that data is available
        if (!nRows)
        {
            CPCloseFile(_cp_file_ref);

            if (vWaveDataPtr != nullptr)
                free(vWaveDataPtr);

            throw SyntaxError(SyntaxError::FILE_IS_EMPTY, sFileName, SyntaxError::invalid_position, sFileName);
        }

        // Alter the column dimensions to
        // respect the data conventions of IGOR
        // (they do not store x-values)
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

        // Create the internal storage using the
        // final dimensions
        createStorage();

        // Fill the x column and its corresponding
        // table column head
        for (long long int j = 0; j < nFirstCol; j++)
        {
            fileTableHeads[j] = cName + string("_[")+(char)('x'+j)+string("]");

            for (long long int i = 0; i < nDim[j]; i++)
            {
                fileData[i][j] = dScalingFactorA[j]*(double)i + dScalingFactorB[j];
            }
        }

        // Fill the remaining data into the table
        // next to the x column
        for (long long int j = 0; j < nCols-nFirstCol; j++)
        {
            // Take respect on three-dimensional
            // waves and roll them out correctly
            if (bXZSlice && nDim[2] > 1 && j)
            {
                nSliceCounter += nDim[1];

                if (!(j % nDim[2]))
                    nSliceCounter = j/nDim[2];
            }
            else
                nSliceCounter = j;

            // Write the corresponding table column
            // head
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

            // Write the actual data to the table.
            // We have to take care about the type
            // of the data and whether the data is
            // complex or not
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

        // Close the file reference and
        // free the allocated memory
        CPCloseFile(_cp_file_ref);

        if (vWaveDataPtr != nullptr)
            free(vWaveDataPtr);
    }

    // This is an overload for the assignment
    // operator of the GenericFile class
    IgorBinaryWave& IgorBinaryWave::operator=(const IgorBinaryWave& file)
    {
        assign(file);
        bXZSlice = file.bXZSlice;

        return *this;
    }
}


