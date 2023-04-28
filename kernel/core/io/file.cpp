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

#include <libsha.hpp>
#include <libzygo.hpp>

#include "file.hpp"
#include "../datamanagement/tablecolumnimpl.hpp"
#include "../IgorLib/ReadWave.h"
#include "../utils/tools.hpp"
#include "../utils/BasicExcel.hpp"
#include "../utils/tinyxml2.h"
#include "../ui/language.hpp"
#include "../version.h"
#include "../../kernel.hpp"

#define DEFAULT_PRECISION 14

extern Language _lang;

namespace NumeRe
{
    using namespace std;

    // This function determines the correct class to be used for the filename
    // passed to this function. If there's no fitting file type, a null pointer
    // is returned. The calling function is responsible for clearing the
    // created instance. The returned pointer is of the type of GenericFile
    // but references an instance of a derived class
    GenericFile* getFileByType(const string& filename)
    {
        FileSystem _fSys;
        _fSys.initializeFromKernel();

        // Get the extension of the filename
        string sExt = toLowerCase(_fSys.getFileParts(filename).back());

        // Create an instance of the selected file type
        if (sExt == "ndat")
            return new NumeReDataFile(filename);

        if (sExt == "dat" && ZygoLib::DatFile::isDatFile(filename))
            return new ZygoDat(filename);

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


    /////////////////////////////////////////////////
    /// \brief This method reads the data in the
    /// referenced text file to memory.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void TextDataFile::readFile()
    {
        open(ios::in);

		long long int nLine = 0;
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
		// in the files contents.
		for (size_t i = 0; i < vFileContents.size(); i++)
		{
			if (vFileContents[i][0] == '#')
				nComment++;
		}

		// If no comments have been found, use all non-numeric lines
		if (!nComment)
        {
            for (size_t i = 0; i < vFileContents.size(); i++)
            {
                if (!isNumeric(vFileContents[i]))
                    nComment++;
            }
        }

		// If now all lines are comments, we have a string table
		if (vFileContents.size() == nComment)
            nComment = 1;

		// Determine now, how many columns are found in the file
		for (size_t i = 0; i < vFileContents.size(); i++)
        {
            size_t elem = tokenize(vFileContents[i], " ", true).size();

            if (elem > nCols)
                nCols = elem;
        }

		// Set the final dimensions
		nRows = vFileContents.size() - nComment;

		// Something's wrong here!
		if (!nCols || !nRows)
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

		// Create the target storage
		createStorage();

		for (TblColPtr& col : *fileData)
        {
            col.reset(new StringColumn);
        }

		nLine = 0;

		// Copy the data from the file to the internal memory
		for (size_t i = 0; i < vFileContents.size(); i++)
        {
            if (vFileContents[i][0] == '#' || (i < nComment && !isNumeric(vFileContents[i])))
            {
                // ignore table heads
                continue;
            }

            // Tokenize the current line
            vector<string> vLine = tokenize(vFileContents[i], " ", true);

            // Ensure that the number of columns is matching
            // If it does not match, then we did not determine
            // the columns correctly
            if (vLine.size() > nCols)
                throw SyntaxError(SyntaxError::COL_COUNTS_DOESNT_MATCH, sFileName, SyntaxError::invalid_position, sFileName);

            // Go through the already tokenized line and store
            // the converted values
            for (size_t j = 0; j < vLine.size(); j++)
            {
                replaceAll(vLine[j], "\1", " ");

                // Do not add tokenizer placeholders
                if (vLine[j] != "_")
                    fileData->at(j)->setValue(nLine, vLine[j]);
            }

            nLine++;
        }

        // Decode the table headlines in this member function
        if (nComment)
        {
            decodeTableHeads(vFileContents, nComment);
        }
    }


    /////////////////////////////////////////////////
    /// \brief This method writes the data in memory
    /// to the referenced text file.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief This member function writes the header
    /// lines or the text files.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief This member function is used to write
    /// the table heads into the target file.
    ///
    /// \param vColumnWidth const vector<size_t>&
    /// \param nNumberOfLines size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void TextDataFile::writeTableHeads(const vector<size_t>& vColumnWidth, size_t nNumberOfLines)
    {
        for (size_t i = 0; i < nNumberOfLines; i++)
        {
            fFileStream << "# ";

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


    /////////////////////////////////////////////////
    /// \brief This member function is used to write
    /// the data in memory to the target text file.
    ///
    /// \param vColumnWidth const vector<size_t>&
    /// \return void
    ///
    /////////////////////////////////////////////////
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
                if (!fileData->at(j)->isValid(i))
                    fFileStream << "---";
                else
                {
                    if (fileData->at(j)->m_type == TableColumn::TYPE_VALUE)
                        fFileStream << toString(fileData->at(j)->getValue(i), nPrecFields);
                    else
                        fFileStream << fileData->at(j)->getValueAsInternalString(i);
                }
            }

            fFileStream.width(0);
            fFileStream << "\n";
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function draws a separator
    /// based upon the overall width of the columns.
    ///
    /// \param vColumnWidth const vector<size_t>&
    /// \return void
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief This member function decodes the table
    /// heads in the text file and stores them in
    /// memory.
    ///
    /// \param vFileContents vector<string>&
    /// \param nComment long longint
    /// \return void
    ///
    /////////////////////////////////////////////////
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
        if (nComment >= 13 && vFileContents[2].substr(0, 21) == "# NumeRe: Framework f")
        {
            // This is a NumeRe-created text file
            // Find the first actual headline
            for (size_t k = 11; k < vFileContents.size(); k++)
            {
                // Is this the first separator line?
                if (vFileContents[k] == sCommentSign)
                {
                    // Now search for the last empty line right
                    // before the sepator line
                    for (int kk = k-1; kk >= 0; kk--)
                    {
                        if (vFileContents[kk] == "#")
                        {
                            _nHeadline = kk+2;
                            break;
                        }
                    }

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
                            if (tokenize(vFileContents[i-1], " ", true).size() <= nCols+1)
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
                            if (tokenize(vFileContents[i-2], " ", true).size() <= nCols+1)
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
                            if ((vFileContents[k][0] != '#' && isNumeric(vFileContents[k])) || i+nComment == k)
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

                                replaceAll(vLine[j], "\1", " ");

                                // Append the current token to the
                                // corresponding column heading and
                                // insert the linebreak character,
                                // if needed
                                if (!vHeadline[j].length())
                                    vHeadline[j] = utf8parser(vLine[j]);
                                else
                                {
                                    vHeadline[j] += "\n";
                                    vHeadline[j] += utf8parser(vLine[j]);
                                }
                            }

                            if (bBreakSignal)
                                break;
                        }

                        // Copy the decoded column headings to
                        // the internal memory
                        for (size_t col = 0; col < std::min(vHeadline.size(), (size_t)nCols); col++)
                        {
                            if (!fileData->at(col))
                                fileData->at(col).reset(new StringColumn);

                            fileData->at(col)->m_sHeadLine = vHeadline[col];
                        }

                        // Return here
                        return;
                    }
                }
            }
		}
    }


    /////////////////////////////////////////////////
    /// \brief This member function calculates the
    /// widths of the columns and also determines the
    /// number of lines needed for the column heads.
    ///
    /// \param nNumberOfLines size_t&
    /// \return vector<size_t>
    ///
    /////////////////////////////////////////////////
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
            if (!fileData->at(j))
            {
                vColumnWidths.push_back(TableColumn::getDefaultColumnHead(j).length());
                continue;
            }

            pair<size_t, size_t> pCellExtents = calculateCellExtents(fileData->at(j)->m_sHeadLine);
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
        versionBuild(0), fileVersionRead(1.0f)
    {
        // Empty constructor
    }


    /////////////////////////////////////////////////
    /// \brief This copy constructor extents the copy
    /// constructor of the parent class.
    ///
    /// \param file const NumeReDataFile&
    ///
    /////////////////////////////////////////////////
    NumeReDataFile::NumeReDataFile(const NumeReDataFile& file) : GenericFile(file)
    {
        isLegacy = file.isLegacy;
        timeStamp = file.timeStamp;
        sComment = file.sComment;
        versionMajor = file.versionMajor;
        versionMinor = file.versionMinor;
        versionBuild = file.versionBuild;
        fileVersionRead = file.fileVersionRead;
    }


    NumeReDataFile::~NumeReDataFile()
    {
        // Empty destructor
    }


    /////////////////////////////////////////////////
    /// \brief This member function writest the new
    /// standard header for NDAT files. It includes
    /// a dummy section, which older versions of
    /// NumeRe may read without errors, but which
    /// won't contain any reasonable information.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void NumeReDataFile::writeHeader()
    {
        writeNumField(AutoVersion::MAJOR);
        writeNumField(AutoVersion::MINOR);
        writeNumField(AutoVersion::BUILD);
        writeNumField(time(0));

        writeDummyHeader();

        writeNumField(fileSpecVersionMajor);
        writeNumField(fileSpecVersionMinor);
        checkPos = tellp();
        writeStringField("SHA-256:" + sha256(""));
        writeNumField<size_t>(10);
        checkStart = tellp();
        writeStringField(getTableName());
        writeStringField(sComment);

        // The following fields are placeholders
        // for further changes. They may be filled
        // in future versions and will be ignored
        // in older versions of NumeRe
        writeStringField("FTYPE=LLINT");
        long long int t = _time64(0);
        writeNumBlock<long long int>(&t, 1);
        writeStringField("FTYPE=DOUBLE");
        writeNumBlock<double>(nullptr, 0);
        writeStringField("FTYPE=DOUBLE");
        writeNumBlock<double>(nullptr, 0);
        writeStringField("FTYPE=DOUBLE");
        writeNumBlock<double>(nullptr, 0);

        // Finally, write the dimensions of the
        // target data
        writeNumField(nRows);
        writeNumField(nCols);
    }


    /////////////////////////////////////////////////
    /// \brief This member function will write the
    /// dummy header, which is readable in older
    /// versions of NumeRe. In a nutshell, this is
    /// the legacy format of NDAT files for a 1x1
    /// data set.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void NumeReDataFile::writeDummyHeader()
    {
        writeNumField(1LL);
        writeNumField(1LL);
        writeStringField("THIS_FILE_NEEDS_AT_LEAST_VERSION_v1.1.5 ");
        long long int appZeros = 0;
        double data = 1.15;
        bool valid = true;
        fFileStream.write((char*)&appZeros, sizeof(long long int));
        fFileStream.write((char*)&data, sizeof(double));
        fFileStream.write((char*)&valid, sizeof(bool));
    }


    /////////////////////////////////////////////////
    /// \brief This member function will write the
    /// data in the internal storage into the target
    /// file.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void NumeReDataFile::writeFile()
    {
        // Open the file in binary mode and truncate
        // its contents (this will only be done, if
        // it's not already open, because otherwise
        // the cache file would be overwritten by each
        // table in memory completely)
        if (!is_open())
            open(ios::binary | ios::in | ios::out | ios::trunc);

        // Write the file header
        writeHeader();

        // Write the columns
        for (TblColPtr& col : *fileData)
            writeColumn(col);

        size_t posEnd = tellp();

        seekp(checkStart);
        std::string checkSum = sha256(fFileStream, checkStart, posEnd-checkStart);

        // Update the checksum and file end
        seekp(checkPos);
        writeStringField("SHA-256:" + checkSum);
        writeNumField<size_t>(posEnd);

        // Go back to the end
        seekp(posEnd);
    }


    /////////////////////////////////////////////////
    /// \brief Writes a single column to the file.
    ///
    /// \param col const TblColPtr&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void NumeReDataFile::writeColumn(const TblColPtr& col)
    {
        if (!col)
        {
            writeStringField("");
            writeStringField("DTYPE=NONE");
            return;
        }

        writeStringField(col->m_sHeadLine);

        if (col->m_type == TableColumn::TYPE_VALUE
            || col->m_type == TableColumn::TYPE_DATETIME
            || col->m_type == TableColumn::TYPE_LOGICAL)
        {
            // All these colums write complex values
            writeStringField("DTYPE=COMPLEX");

            // Note the type of the column
            if (col->m_type == TableColumn::TYPE_VALUE)
                writeStringField("CTYPE=VALUE");
            else if (col->m_type == TableColumn::TYPE_DATETIME)
                writeStringField("CTYPE=DATETIME");
            else if (col->m_type == TableColumn::TYPE_LOGICAL)
                writeStringField("CTYPE=LOGICAL");

            size_t lenPos = tellp();
            // Store the position of the next column
            writeNumField<size_t>(lenPos);

            std::vector<mu::value_type> values = col->getValue(VectorIndex(0, VectorIndex::OPEN_END));
            writeNumBlock<mu::value_type>(&values[0], values.size());

            size_t endPos = tellp();
            seekp(lenPos);
            writeNumField<size_t>(endPos-lenPos);
            seekp(endPos);
        }
        else if (col->m_type == TableColumn::TYPE_STRING
                 || col->m_type == TableColumn::TYPE_CATEGORICAL)
        {
            // All these colums write strings as values
            writeStringField("DTYPE=STRING");

            // Note the type of the column
            if (col->m_type == TableColumn::TYPE_STRING)
                writeStringField("CTYPE=STRING");
            else if (col->m_type == TableColumn::TYPE_CATEGORICAL)
                writeStringField("CTYPE=CATEGORICAL");

            size_t lenPos = tellp();
            // Store the position of the next column
            writeNumField<size_t>(lenPos);

            std::vector<std::string> values = col->getValueAsInternalString(VectorIndex(0, VectorIndex::OPEN_END));
            writeStringBlock(&values[0], values.size());

            size_t endPos = tellp();
            seekp(lenPos);
            writeNumField<size_t>(endPos-lenPos);
            seekp(endPos);
        }
        else if (col->m_type == TableColumn::TYPE_NONE)
        {
            writeStringField("DTYPE=NONE");
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function will read the
    /// header in the selected file. It will
    /// automatically detect, whether the file is in
    /// legacy format or not. If the file format is
    /// newer than expected, it will throw an error.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void NumeReDataFile::readHeader()
    {
        // Read the basic information
        versionMajor = readNumField<long int>();
        versionMinor = readNumField<long int>();
        versionBuild = readNumField<long int>();
        time_t oldTime = readNumField<time_t>();

        // Detect, whether this file was created in
        // legacy format (earlier than v1.1.2)
        if (versionMajor * 100 + versionMinor * 10 + versionBuild <= 111)
        {
            isLegacy = true;
            fileVersionRead = 1.0f;
            nRows = readNumField<long long int>();
            nCols = readNumField<long long int>();
            timeStamp = oldTime;
            return;
        }

        // Omitt the dummy header
        skipDummyHeader();

        // Read the file version number (which is
        // independent on the version number of
        // NumeRe)
        short fileVerMajor = readNumField<short>();
        short fileVerMinor = readNumField<short>();
        fileVersionRead = fileVerMajor + fileVerMinor / 100.0;

        // Ensure that the major version of the file
        // is not larger than the major version
        // implemented in this class (minor version
        // changes shall be downwards compatible)
        if (fileVerMajor > fileSpecVersionMajor)
            throw SyntaxError(SyntaxError::INSUFFICIENT_NUMERE_VERSION, sFileName, SyntaxError::invalid_position, sFileName);

        // Read checksum
        if (fileVersionRead >= 4.0)
        {
            std::string sha_check = readStringField();
            size_t fileEnd = readNumField<size_t>();

            size_t checkStart = tellg();

            std::string sha = "SHA-256:" + sha256(fFileStream, checkStart, fileEnd-checkStart);

            // Is it corrupted?
            if (sha_check != sha)
                NumeReKernel::issueWarning(_lang.get("COMMON_DATAFILE_CORRUPTED", sFileName));

            seekg(checkStart);
        }

        // Read the table name and the comment
        sTableName = readStringField();
        sComment = readStringField();

        // Read now the three empty fields of the
        // header. We created special functions for
        // reading and deleting arbitary data types
        long long int size;
        string type;
        void* data = readGenericField(type, size);

        // Version 2.1 introduces 64 bit time stamps
        if (fileVersionRead >= 2.01 && type == "LLINT" && size == 1u)
            timeStamp = *(long long int*)data;
        else
            timeStamp = oldTime;

        deleteGenericData(data, type);

        data = readGenericField(type, size);
        deleteGenericData(data, type);

        data = readGenericField(type, size);
        deleteGenericData(data, type);

        // Version 3.0 introduces another generic field
        if (fileVersionRead >= 3.00)
        {
            data = readGenericField(type, size);
            deleteGenericData(data, type);
        }
        else
        {
            // Determine the data type of the table
            string dataType = readStringField();

            // Ensure that the data type is DOUBLE
            if (dataType != "DTYPE=DOUBLE")
                throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);
        }

        // Read the dimensions of the table
        nRows = readNumField<long long int>();
        nCols = readNumField<long long int>();
    }


    /////////////////////////////////////////////////
    /// \brief This function jumps over the dummy
    /// section in the new file format, because it
    /// does not conatin any valid information.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief This member function will read the
    /// contents of the target file.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
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

        // Create empty storage
        createStorage();

        // Version 3.0 introduces column-like layout and v4.0 added
        // more columns and improved backwards compatibility
        if (fileVersionRead >= 4.00)
        {
            if (fileData)
            {
                for (TblColPtr& col : *fileData)
                {
                    readColumnV4(col);
                }
            }

            return;
        }
        else if (fileVersionRead >= 3.00)
        {
            if (fileData)
            {
                for (TblColPtr& col : *fileData)
                {
                    readColumn(col);
                }
            }

            return;
        }

        long long int stringBlockSize;
        long long int dataarrayrows;
        long long int dataarraycols;

        // Read the table column headers and the
        // table data and copy it to a ValueColumn
        // array
        std::string* sHeads = readStringBlock(stringBlockSize);
        double** data = readDataArray<double>(dataarrayrows, dataarraycols);

        for (long long int j = 0; j < dataarraycols; j++)
        {
            fileData->at(j).reset(new ValueColumn);
            fileData->at(j)->m_sHeadLine = sHeads[j];

            for (long long int i = 0; i < dataarrayrows; i++)
            {
                fileData->at(j)->setValue(i, data[i][j]);
            }
        }

        // Free up memory
        delete[] sHeads;

        for (long long int i = 0; i < dataarrayrows; i++)
            delete[] data[i];

        delete[] data;
    }


    /////////////////////////////////////////////////
    /// \brief Reads a single column from file.
    ///
    /// \param col TblColPtr&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void NumeReDataFile::readColumn(TblColPtr& col)
    {
        std::string sHeadLine = readStringField();
        std::string sDataType = readStringField();

        if (sDataType == "DTYPE=NONE")
            return;

        if (sDataType == "DTYPE=COMPLEX")
        {
            col.reset(new ValueColumn);
            col->m_sHeadLine = sHeadLine;
            long long int size = 0;
            mu::value_type* values = readNumBlock<mu::value_type>(size);
            col->setValue(VectorIndex(0, VectorIndex::OPEN_END), std::vector<mu::value_type>(values, values+size));
            delete[] values;
        }
        else if (sDataType == "DTYPE=LOGICAL")
        {
            col.reset(new LogicalColumn);
            col->m_sHeadLine = sHeadLine;
            long long int size = 0;
            mu::value_type* values = readNumBlock<mu::value_type>(size);
            col->setValue(VectorIndex(0, VectorIndex::OPEN_END), std::vector<mu::value_type>(values, values+size));
            delete[] values;
        }
        else if (sDataType == "DTYPE=DATETIME")
        {
            col.reset(new DateTimeColumn);
            col->m_sHeadLine = sHeadLine;
            long long int size = 0;
            mu::value_type* values = readNumBlock<mu::value_type>(size);
            col->setValue(VectorIndex(0, VectorIndex::OPEN_END), std::vector<mu::value_type>(values, values+size));
            delete[] values;
        }
        else if (sDataType == "DTYPE=STRING")
        {
            col.reset(new StringColumn);
            col->m_sHeadLine = sHeadLine;
            long long int size = 0;
            std::string* strings = readStringBlock(size);
            col->setValue(VectorIndex(0, VectorIndex::OPEN_END), std::vector<std::string>(strings, strings+size));
            delete[] strings;
        }
        else if (sDataType == "DTYPE=CATEGORICAL")
        {
            col.reset(new CategoricalColumn);
            col->m_sHeadLine = sHeadLine;
            long long int size = 0;
            std::string* strings = readStringBlock(size);
            col->setValue(VectorIndex(0, VectorIndex::OPEN_END), std::vector<std::string>(strings, strings+size));
            delete[] strings;
        }
    }


    /////////////////////////////////////////////////
    /// \brief Reads a single column from file in v4
    /// format.
    ///
    /// \param col TblColPtr&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void NumeReDataFile::readColumnV4(TblColPtr& col)
    {
        std::string sHeadLine = readStringField();
        std::string sDataType = readStringField();

        if (sDataType == "DTYPE=NONE")
            return;

        if (sDataType == "DTYPE=COMPLEX")
        {
            // Get the actual column type
            std::string sColType = readStringField();

            // Jump over the colum end information
            readNumField<size_t>();

            // Create the column for the corresponding CTYPE
            if (sColType == "CTYPE=VALUE")
                col.reset(new ValueColumn);
            else if (sColType == "CTYPE=DATETIME")
                col.reset(new DateTimeColumn);
            else if (sColType == "CTYPE=LOGICAL")
                col.reset(new LogicalColumn);
            else // All others fall back to a generic value column
                col.reset(new ValueColumn);

            col->m_sHeadLine = sHeadLine;
            long long int size = 0;
            mu::value_type* values = readNumBlock<mu::value_type>(size);
            col->setValue(VectorIndex(0, VectorIndex::OPEN_END), std::vector<mu::value_type>(values, values+size));
            delete[] values;
        }
        else if (sDataType == "DTYPE=STRING")
        {
            // Get the actual column type
            std::string sColType = readStringField();

            // Jump over the colum end information
            readNumField<size_t>();

            // Create the column for the corresponding CTYPE
            if (sColType == "CTYPE=STRING")
                col.reset(new StringColumn);
            else if (sColType == "CTYPE=CATEGORICAL")
                col.reset(new CategoricalColumn);
            else // All others fall back to a generic string column
                col.reset(new StringColumn);

            col->m_sHeadLine = sHeadLine;
            long long int size = 0;
            std::string* strings = readStringBlock(size);
            col->setValue(VectorIndex(0, VectorIndex::OPEN_END), std::vector<std::string>(strings, strings+size));
            delete[] strings;
        }
        else
        {
            // In all other cases: just jump over this column
            seekg(readNumField<size_t>());
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function reads the data
    /// section of the target file in legacy format.
    /// The function readHeader() determines, whether
    /// the target file is in legacy mode.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
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
            fileData->at(i).reset(new ValueColumn);

            nLength = 0;
            fFileStream.read((char*)&nLength, sizeof(size_t));
            cHeadLine[i] = new char[nLength];
            fFileStream.read(cHeadLine[i], sizeof(char)*nLength);
            fileData->at(i)->m_sHeadLine.resize(nLength-1);

            for (unsigned int j = 0; j < nLength-1; j++)
            {
                fileData->at(i)->m_sHeadLine[j] = cHeadLine[i][j];
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
        double* data = new double[nCols];

        for (long long int i = 0; i < nRows; i++)
        {
            fFileStream.read((char*)data, sizeof(double)*nCols);

            for (long long int j = 0; j < nCols; j++)
            {
                fileData->at(j)->setValue(i, data[j]);
            }
        }

        delete[] data;

        // Read the validation block and apply
        // the contained information on the data
        // in the internal storage
        for (long long int i = 0; i < nRows; i++)
        {
            fFileStream.read((char*)bValidEntry, sizeof(bool)*nCols);
            for (long long int j = 0; j < nCols; j++)
            {
                if (!bValidEntry[j])
                    fileData->at(j)->setValue(i, NAN);
            }
        }

        // Free the created memory
        for (long long int i = 0; i < nCols; i++)
            delete[] cHeadLine[i];

        delete[] cHeadLine;
        delete[] bValidEntry;
        delete[] nAppendedZeros;
    }


    /////////////////////////////////////////////////
    /// \brief This member function will read a
    /// generic field from the header (the three
    /// fields, which can be used in future versions
    /// of NumeRe).
    ///
    /// \param type std::string&
    /// \param size long longint&
    /// \return void*
    ///
    /////////////////////////////////////////////////
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
        else if (type == "INT")
        {
            int* data = readNumBlock<int>(size);
            return (void*)data;
        }
        else if (type == "LINT")
        {
            long int* data = readNumBlock<long int>(size);
            return (void*)data;
        }
        else if (type == "LLINT")
        {
            long long int* data = readNumBlock<long long int>(size);
            return (void*)data;
        }
        else if (type == "UINT")
        {
            size_t* data = readNumBlock<size_t>(size);
            return (void*)data;
        }
        else if (type == "BYTE")
        {
            char* data = readNumBlock<char>(size);
            return (void*)data;
        }
        else if (type == "STRING")
        {
            string* data = readStringBlock(size);
            return (void*)data;
        }

        return nullptr;
    }


    /////////////////////////////////////////////////
    /// \brief This member function will delete the
    /// data array obtained from the generic fields
    /// but convert them into their original type
    /// first, because deleting of void* is undefined
    /// behavior (the length of the field in memory
    /// is not defined).
    ///
    /// \param data void*
    /// \param type const string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void NumeReDataFile::deleteGenericData(void* data, const string& type)
    {
        if (data)
        {
            if (type == "DOUBLE")
                delete[] (double*)data;
            else if (type == "INT")
                delete[] (int*)data;
            else if (type == "LINT")
                delete[] (long int*)data;
            else if (type == "LLINT")
                delete[] (long long int*)data;
            else if (type == "UINT")
                delete[] (size_t*)data;
            else if (type == "BYTE")
                delete[] (char*)data;
            else if (type == "STRING")
                delete[] (string*)data;
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function is an overload
    /// for the assignment operator. It extends the
    /// already available assingnment operator of
    /// the parent class.
    ///
    /// \param file NumeReDataFile&
    /// \return NumeReDataFile&
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief This simple member function returns
    /// the version string associated with the
    /// current file type.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief This destructor will write the offsets
    /// for the different tables to the file before
    /// the file stream will be closed.
    /////////////////////////////////////////////////
    CacheFile::~CacheFile()
    {

        if (nIndexPos && vFileIndex.size())
        {
            seekp(nIndexPos);
            writeNumBlock(&vFileIndex[0], vFileIndex.size());
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function will reset the
    /// string information and the internal storage.
    /// This is used before the next table will be
    /// read from the cache file to memory.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void CacheFile::reset()
    {
        sComment.clear();
        sTableName.clear();
        clearStorage();
    }


    /////////////////////////////////////////////////
    /// \brief This member function will read the
    /// next table, which is availale in the cache
    /// file, to the internal stoage. It uses the
    /// readFile() member function from its parent
    /// class.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void CacheFile::readSome()
    {
        reset();
        size_t pos = tellg();

        if (std::find(vFileIndex.begin(), vFileIndex.end(), pos) == vFileIndex.end())
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "numere.cache", "numere.cache");

        if (vFileIndex.size() && !fFileStream.eof())
            readFile();
    }


    /////////////////////////////////////////////////
    /// \brief This member function will write the
    /// current contents in the internal storage to
    /// the target file. Before writing, the function
    /// stores the current byte position in the
    /// target file to create the file index. The
    /// funciton uses the member function writeFile()
    /// from the parent class for writing the data.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief This member function will read the
    /// cache file header and ensure that the version
    /// of the file is not newer than expected.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
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
        readNumField<short>();

        // Ensure that the file major version is
        // not larger than the one currently
        // implemented in this class
        if (fileVerMajor > fileSpecVersionMajor)
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


    /////////////////////////////////////////////////
    /// \brief This member function will write the
    /// standard cache file header to the cache file.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void CacheFile::writeCacheHeader()
    {
        // Open the file in binary mode and truncate
        // all its contents
        open(ios::binary | ios::in | ios::out | ios::trunc);

        writeNumField(AutoVersion::MAJOR);
        writeNumField(AutoVersion::MINOR);
        writeNumField(AutoVersion::BUILD);
        writeNumField(time(0));
        writeStringField("NUMERECACHEFILE");
        writeNumField(fileSpecVersionMajor);
        writeNumField(fileSpecVersionMinor);

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


    /////////////////////////////////////////////////
    /// \brief This member function will read the
    /// contents of the associated LABX file to the
    /// internal storage.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
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

        for (long long int i = 0; i < nCols; i++)
        {
            fileData->at(i).reset(new ValueColumn);
            fileData->at(i)->m_sHeadLine = vHeadLines[i];
        }

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
                nElements = StrToInt(vCols[i].substr(vCols[i].find('"')+1,
                                                     vCols[i].find('"', vCols[i].find('"')+1)-1-vCols[i].find('"')));
                vCols[i].erase(0, vCols[i].find('>')+1);

                // Copy the elements into the internal
                // storage
                for (long long int j = 0; j < min(nElements, nRows); j++)
                {
                    fileData->at(i)->setValue(j, extractValueFromTag(vCols[i].substr(vCols[i].find("<value"),
                                                                                     vCols[i].find('<', vCols[i].find('/'))-vCols[i].find("<value"))));
                    vCols[i].erase(0, vCols[i].find('>', vCols[i].find('/'))+1);
                }
            }
        }

    }


    /////////////////////////////////////////////////
    /// \brief This simple member function extracts
    /// the numerical value of the XML tag string.
    ///
    /// \param sTag const string&
    /// \return double
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief This member function is used to read
    /// the target file to memory.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
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
        string sValidSymbols = "0123456789.,;-+eE INFAinfa/";
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
                if (isConvertible(vTokens[i], CONVTYPE_VALUE))
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
            {
                vFileData[0].clear();
                nComment++;
            }
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

        // We start with string-only and try to
        // convert them later
        for (long long int j = 0; j < nCols; j++)
        {
            fileData->at(j).reset(new StringColumn);
        }

        // Copy the already decoded table heads to
        // the internal storage or create a dummy
        // column head, if the data contains only
        // one column
		if (nComment)
        {
            for (long long int j = 0; j < nCols; j++)
            {
                fileData->at(j)->m_sHeadLine = vHeadLine[j];
            }
        }
		else
		{
		    // If the file contains only one column,
		    // then we'll use the file name as table
		    // column head
			if (nCols == 1)
			{
                if (sFileName.find('/') == string::npos)
                    fileData->at(0)->m_sHeadLine = sFileName.substr(0, sFileName.rfind('.'));
                else
                    fileData->at(0)->m_sHeadLine = sFileName.substr(sFileName.rfind('/')+1, sFileName.rfind('.')-1-sFileName.rfind('/'));
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
                // Ensure that enough space is available
                if (j >= nCols)
                    break;

                fileData->at(j)->setValue(i-nComment, vTokens[j]);
            }
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function is used to write
    /// the contents in the internal storage to the
    /// target file.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void CommaSeparatedValues::writeFile()
    {
        // Open the file in text mode and truncate
        // all its contents
        open(ios::out | ios::trunc);

        // Write the table heads to the file
        for (long long int j = 0; j < nCols; j++)
        {
            if (fileData->at(j))
                fFileStream << fileData->at(j)->m_sHeadLine;

            fFileStream << ",";
        }

        fFileStream << "\n";
        fFileStream.precision(nPrecFields);

        // Write the data to the file
        for (long long int i = 0; i < nRows; i++)
        {
            for (long long int j = 0; j < nCols; j++)
            {
                if (fileData->at(j) && fileData->at(j)->isValid(i))
                {
                    if (fileData->at(j)->m_type == TableColumn::TYPE_VALUE)
                        fFileStream << toString(fileData->at(j)->getValue(i), DEFAULT_PRECISION);
                    else
                        fFileStream << fileData->at(j)->getValueAsInternalString(i);
                }

                fFileStream << ",";
            }

            fFileStream << "\n";
        }

        fFileStream.flush();
    }


    /////////////////////////////////////////////////
    /// \brief This member function determines the
    /// separator character used for the current file
    /// name. It does so by some simple heuristic:
    /// the most common characters are checked first
    /// and the uncommon ones afterwards.
    ///
    /// \param vTextData const vector<string>&
    /// \return char
    ///
    /////////////////////////////////////////////////
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
            else
                cSep = ','; // Fallback if this file does only contain a single column

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


    /////////////////////////////////////////////////
    /// \brief This member function determines the
    /// number of columns available in the current
    /// file and alters the separator character, if
    /// the column counts are not consistent between
    /// the different lines.
    ///
    /// \param vTextData const std::vector<std::string>&
    /// \param cSep char&
    /// \return void
    ///
    /////////////////////////////////////////////////
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

            if (nCols < nCol)
                nCols = nCol;
            else if (abs(nCol - nCols) > 1)
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


    /////////////////////////////////////////////////
    /// \brief This member function is used to write
    /// the contents of the internal storage to the
    /// file.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
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

                if (!fileData->at(j))
                    fFileStream << "---";
                else if (fileData->at(j)->m_type == TableColumn::TYPE_VALUE)
                    fFileStream << formatNumber(fileData->at(j)->getValue(i));
                else
                    fFileStream << fileData->at(j)->getValueAsInternalString(i);

                if (j+1 < nCols)
                    fFileStream << " &";
                else
                    fFileStream << "\\\\\n";
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


    /////////////////////////////////////////////////
    /// \brief This member function writes the legal
    /// header to the file.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief This member function writes the table
    /// column heads to the file. The number of lines
    /// needed for the heads is considered in this
    /// case.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief This member function calculates the
    /// number of lines needed for the complete table
    /// column heads.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t LaTeXTable::countHeadLines()
    {
        size_t headlines = 0u;

        // Get the cell extents of each table
        // column head and store the maximal
        // value
        for (long long int i = 0; i < nCols; i++)
        {
            if (!fileData->at(i))
                continue;

            auto extents = calculateCellExtents(fileData->at(i)->m_sHeadLine);

            if (extents.second > headlines)
                headlines = extents.second;
        }

        return headlines;
    }


    /////////////////////////////////////////////////
    /// \brief This member function replaces all non-
    /// ASCII characters into their corresponding
    /// LaTeX entities.
    ///
    /// \param _sText const string&
    /// \return string
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief This member function formats a complex
    /// as LaTeX number string.
    ///
    /// \param number const mu::value_type&
    /// \return string
    ///
    /////////////////////////////////////////////////
    string LaTeXTable::formatNumber(const mu::value_type& number)
    {
        string sNumber = toString(number, nPrecFields);

        // Handle floating point numbers with
        // exponents correctly
        while (sNumber.find('e') != std::string::npos)
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



    /////////////////////////////////////////////////
    /// \brief Structure for storing the JCAMP-DX
    /// meta data.
    /////////////////////////////////////////////////
    struct JcampDX::MetaData
    {
        enum DataFormat
        {
            NO_FORMAT,
            XY, // (XY), (X,Y)
            XY_XY, // (XY..XY), (X,Y..X,Y)
            XppY_Y, // ((X++)(Y..Y))
            XPPY_Y // (X++(Y..Y))
        };

        size_t m_points;
        DataFormat m_format;
        double m_xFactor;
        double m_yFactor;
        double m_firstX;
        double m_lastX;
        double m_deltaX;

        std::string m_xUnit;
        std::string m_yUnit;
        std::string m_symbol;

        MetaData() : m_points(0), m_format(NO_FORMAT), m_xFactor(1), m_yFactor(1), m_firstX(0), m_lastX(0), m_deltaX(0) {}
    };


    /////////////////////////////////////////////////
    /// \brief This member function is used to read
    /// the conents of the JCAMP-DX file to the
    /// internal storage.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void JcampDX::readFile()
    {
        // Open the file in text mode
        open(ios::in);

        // Create some temporary buffer variables
		size_t nTableStart = 0;
        std::vector<MetaData> vMeta(1u);

		nRows = 0;
		nCols = 0;

		// Read the contents of the file to the
		// vector variable
		std::vector<std::string> vFileContents = readTextFile(true);

		// Ensure that contents are available
		if (!vFileContents.size())
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Find the end tag in the file and omit
        // all following lines
		for (size_t i = 0; i < vFileContents.size(); i++)
        {
            StripSpaces(vFileContents[i]);

            // Copy all meta data before the actual table into
            // a common string
            if (vFileContents[i].substr(0, 2) == "##"
                && !nTableStart
                && vFileContents[i].substr(0, 6) != "##END=")
            {
                if (sComment.length())
                    sComment += "\n";

                sComment += vFileContents[i].substr(2);
            }

            parseLabel(vFileContents[i]);

            // Here starts a n-tuples table
            if (vFileContents[i].substr(0, 10) == "##NTUPLES=")
            {
                EndlessVector<std::string> symbol;
                EndlessVector<std::string> varType;
                EndlessVector<std::string> varDim;
                EndlessVector<std::string> units;
                EndlessVector<std::string> firstVal;
                EndlessVector<std::string> lastVal;
                EndlessVector<std::string> factor;

                for (size_t j = i+1; j < vFileContents.size(); j++)
                {
                    StripSpaces(vFileContents[j]);

                    // Copy all meta data before the actual table into
                    // a common string
                    if (vFileContents[j].substr(0, 2) == "##"
                        && vFileContents[j].substr(0, 7) != "##PAGE=")
                    {
                        if (sComment.length())
                            sComment += "\n";

                        sComment += vFileContents[j].substr(2);
                    }

                    parseLabel(vFileContents[j]);

                    if (vFileContents[j].substr(0, 9) == "##SYMBOL=")
                        symbol = getAllArguments(vFileContents[j].substr(9));
                    else if (vFileContents[j].substr(0, 10) == "##VARTYPE=")
                        varType = getAllArguments(vFileContents[j].substr(10));
                    else if (vFileContents[j].substr(0, 9) == "##VARDIM=")
                        varDim = getAllArguments(vFileContents[j].substr(9));
                    else if (vFileContents[j].substr(0, 8) == "##UNITS=")
                        units = getAllArguments(vFileContents[j].substr(8));
                    else if (vFileContents[j].substr(0, 8) == "##FIRST=")
                        firstVal = getAllArguments(vFileContents[j].substr(8));
                    else if (vFileContents[j].substr(0, 7) == "##LAST=")
                        lastVal = getAllArguments(vFileContents[j].substr(7));
                    else if (vFileContents[j].substr(0, 9) == "##FACTOR=")
                        factor = getAllArguments(vFileContents[j].substr(9));
                    else if (vFileContents[j].substr(0, 7) == "##PAGE=")
                    {
                        nTableStart = j;
                        i = j;

                        // Decode the read meta table
                        size_t dependentCount = 0;

                        // Create MetaData objects for all dependent
                        // variables first
                        for (size_t n = 0; n < varType.size(); n++)
                        {
                            if (varType[n] == "DEPENDENT")
                            {
                                dependentCount++;

                                if (dependentCount > vMeta.size())
                                    vMeta.push_back(MetaData());

                                vMeta.back().m_symbol = symbol[n];
                                vMeta.back().m_yUnit = units[n];

                                if (varDim[n].size())
                                    vMeta.back().m_points = StrToInt(varDim[n]);

                                if (factor[n].size())
                                    vMeta.back().m_yFactor = StrToDb(factor[n]);
                            }
                        }

                        // Insert the values of the main independent variable
                        // into the MetaData objects for all dependent variables
                        for (size_t n = 0; n < varType.size(); n++)
                        {
                            if (varType[n] == "INDEPENDENT" && symbol[n] == "X")
                            {
                                for (MetaData& meta : vMeta)
                                {
                                    meta.m_xUnit = units[n];

                                    if (factor[n].size())
                                        meta.m_xFactor = StrToDb(factor[n]);

                                    if (firstVal[n].size())
                                        meta.m_firstX = StrToDb(firstVal[n]);

                                    if (lastVal[n].size())
                                        meta.m_lastX = StrToDb(lastVal[n]);
                                }

                                break;
                            }
                        }

                        break;
                    }
                }
            }

            // Erase everything after the end tag
            if (vFileContents[i].substr(0, 6) == "##END=")
            {
                vFileContents.erase(vFileContents.begin()+i+1, vFileContents.end());
                break;
            }
        }

        size_t page = 0;

        // Read all pages available in the file
        do
        {
            nTableStart = readTable(vFileContents, nTableStart, vMeta.size() > page ? vMeta[page] : vMeta.front());
            page++;
        }
        while (nTableStart + 1 < vFileContents.size());

        // Find maximal number of rows
        for (long long int j = 0; j < nCols; j++)
        {
            if (fileData->at(j)->size() > nRows)
                nRows = fileData->at(j)->size();
        }
    }


    /////////////////////////////////////////////////
    /// \brief Reads a single table from the
    /// currently opened JCAMP-DX file.
    ///
    /// \param vFileContents std::vector<std::string>&
    /// \param nTableStart size_t
    /// \param meta JcampDX::MetaData
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t JcampDX::readTable(std::vector<std::string>& vFileContents, size_t nTableStart, JcampDX::MetaData meta)
    {
        std::vector<double> vLine;
		long long int nPageOffset = nCols;
		size_t nDataStart = 0;

        // Find the start of the data table
        for (size_t i = nTableStart; i < vFileContents.size(); i++)
        {
            // Determine the needed number of rows
            if (vFileContents[i].substr(0, 10) == "##NPOINTS=")
                meta.m_points = StrToInt(vFileContents[i].substr(10));

            if (vFileContents[i].substr(0,11) == "##XYPOINTS="
                || vFileContents[i].substr(0, 9) == "##XYDATA="
                || vFileContents[i].substr(0, 12) == "##PEAKTABLE="
                || vFileContents[i].substr(0, 12) == "##DATATABLE=")
            {
                nDataStart = i+1;
                std::string sXYScheme = vFileContents[i].substr(vFileContents[i].find('=')+1);
                StripSpaces(sXYScheme);

                // Remove all internal whitespaces
                while (sXYScheme.find(' ') != std::string::npos)
                    sXYScheme.erase(sXYScheme.find(' '), 1);

                // Remove all trailing information like "PEAKS"
                if (sXYScheme.rfind(')')+1 < sXYScheme.length())
                    sXYScheme.erase(sXYScheme.rfind(')')+1);

                // If other variables are used than Y, replace them here
                if (sXYScheme.find('Y') == std::string::npos && meta.m_symbol.length())
                    replaceAll(sXYScheme, meta.m_symbol.c_str(), "Y");

                // Detect the data format
                if (sXYScheme == "(XY..XY)" || sXYScheme == "(X,Y..X,Y)")
                    meta.m_format = MetaData::XY_XY;
                else if (sXYScheme == "(XY)" || sXYScheme == "(X,Y)")
                    meta.m_format = MetaData::XY;
                else if (sXYScheme == "(X++(Y..Y))")
                    meta.m_format = MetaData::XPPY_Y;
                else if (sXYScheme == "(X++)(Y..Y)")
                    meta.m_format = MetaData::XppY_Y;

                break;
            }
        }

        if (!meta.m_points || !nDataStart || meta.m_format == MetaData::NO_FORMAT)
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Determine the number of needed columns based upon the data
        // table type and the first row, if necessary
        nCols += 2;

        // Prepare the internal storage
        if (!fileData)
            fileData = new TableColumnArray;

        fileData->resize(nCols);

        // We only create numerical columns for this data type
        for (long long int j = nPageOffset; j < nCols; j++)
        {
            fileData->at(j).reset(new ValueColumn);
        }

        // Prepare some decoding variables
        std::string sDataType = "";

        // Go through the label section first
        // and decode the header information of
        // the data set
        for (size_t j = nTableStart; j < nDataStart-1; j++)
        {
            // Omit comments
            if (vFileContents[j].find("$$") != string::npos)
                vFileContents[j].erase(vFileContents[j].find("$$"));

            // Get the x and y scaling factors
            if (vFileContents[j].substr(0,10) == "##XFACTOR=")
                meta.m_xFactor = StrToDb(vFileContents[j].substr(10));

            if (vFileContents[j].substr(0,10) == "##YFACTOR=")
                meta.m_yFactor = StrToDb(vFileContents[j].substr(10));

            if (vFileContents[j].substr(0,9) == "##FIRSTX=")
                meta.m_firstX = StrToDb(vFileContents[j].substr(9));

            if (vFileContents[j].substr(0,8) == "##LASTX=")
                meta.m_lastX = StrToDb(vFileContents[j].substr(8));

            // Extract the x units
            if (vFileContents[j].substr(0,9) == "##XUNITS=")
            {
                meta.m_xUnit = vFileContents[j].substr(9);
                StripSpaces(meta.m_xUnit);
            }

            // Extract the y units
            if (vFileContents[j].substr(0,9) == "##YUNITS=")
            {
                meta.m_yUnit = vFileContents[j].substr(9);
                StripSpaces(meta.m_yUnit);
            }

            // Get the data type (currently unused)
            if (vFileContents[j].substr(0,11) == "##DATATYPE=")
            {
                sDataType = vFileContents[j].substr(11);
                StripSpaces(sDataType);
            }
        }

        if (meta.m_xUnit.length())
        {
            if (toUpperCase(meta.m_xUnit) == "1/CM")
                meta.m_xUnit = "Wellenzahl k [cm^-1]";
            else if (toUpperCase(meta.m_xUnit) == "MICROMETERS")
                meta.m_xUnit = "Wellenlänge lambda [mu m]";
            else if (toUpperCase(meta.m_xUnit) == "NANOMETERS")
                meta.m_xUnit = "Wellenlänge lambda [nm]";
            else if (toUpperCase(meta.m_xUnit) == "SECONDS")
                meta.m_xUnit = "Zeit t [s]";
            else if (toUpperCase(meta.m_xUnit) == "1/S" || toUpperCase(meta.m_xUnit) == "1/SECONDS")
                meta.m_xUnit = "Frequenz f [Hz]";
            else
                meta.m_xUnit = "[" + meta.m_xUnit + "]";
        }

        if (meta.m_yUnit.length())
        {
            if (toUpperCase(meta.m_yUnit) == "TRANSMITTANCE")
                meta.m_yUnit = "Transmission";
            else if (toUpperCase(meta.m_yUnit) == "REFLECTANCE")
                meta.m_yUnit = "Reflexion";
            else if (toUpperCase(meta.m_yUnit) == "ABSORBANCE")
                meta.m_yUnit = "Absorbtion";
            else if (toUpperCase(meta.m_yUnit) == "KUBELKA-MUNK")
                meta.m_yUnit = "Kubelka-Munk";
            else if (toUpperCase(meta.m_yUnit) == "ARBITRARY UNITS" || meta.m_yUnit.substr(0,9) == "Intensity")
                meta.m_yUnit = "Intensität";
        }

        meta.m_deltaX = (meta.m_lastX - meta.m_firstX) / (meta.m_points - 1);
        size_t currentRow = 0;
        fileData->at(nPageOffset + 0)->m_sHeadLine = meta.m_xUnit;
        fileData->at(nPageOffset + 1)->m_sHeadLine = meta.m_yUnit;

        // Now go through the actual data section
        // of the file and convert it into
        // numerical values
        for (size_t j = nDataStart; j < vFileContents.size() - 1; j++)
        {
            // Abort at the end tag
            if (vFileContents[j].substr(0, 6) == "##END="
                || vFileContents[j].substr(0, 7) == "##PAGE=")
                return j;

            // Ignore lables
            if (vFileContents[j].substr(0, 2) == "##")
                continue;

            // Omit comments
            if (vFileContents[j].find("$$") != string::npos)
            {
                vFileContents[j].erase(vFileContents[j].find("$$"));
                StripSpaces(vFileContents[j]);
            }

            // Abort, if we have enough lines
            if (currentRow >= meta.m_points)
                return j;

            // Decode the current line
            vLine = parseLine(vFileContents[j]);

            if (!vLine.size())
            {
                NumeReKernel::issueWarning("Parsing of line " + toString(j+1) + " yield no result.");
                continue;
            }

            // Interpret the line depending on the JDX format
            if (meta.m_format == MetaData::XY || meta.m_format == MetaData::XY_XY) // XY pairs
            {
                for (size_t k = 0; k < vLine.size(); k++)
                {
                    fileData->at(nPageOffset + k % 2)->setValue(currentRow, vLine[k] * (k % 2 ? meta.m_yFactor : meta.m_xFactor));
                    currentRow += k % 2;
                }
            }
            else if (meta.m_format == MetaData::XPPY_Y) // first value has to be identical to calculated one
            {
                if (std::abs(meta.m_firstX+meta.m_deltaX*currentRow - vLine[0]*meta.m_xFactor) > std::abs(meta.m_deltaX) * 1e-1)
                    NumeReKernel::issueWarning("Missing points in JCAMP-DX file in line " + toString(j+1)
                                               + ". Expected: " + toString(meta.m_firstX+meta.m_deltaX*currentRow)
                                               + " Found: " + toString(vLine[0]*meta.m_xFactor));

                fileData->at(nPageOffset + 0)->setValue(currentRow, meta.m_firstX + meta.m_deltaX * currentRow);
                fileData->at(nPageOffset + 1)->setValue(currentRow, vLine[1] * meta.m_yFactor);
                currentRow++;

                for (size_t k = 2; k < vLine.size(); k++)
                {
                    fileData->at(nPageOffset + 0)->setValue(currentRow, meta.m_firstX + meta.m_deltaX * currentRow);
                    fileData->at(nPageOffset + 1)->setValue(currentRow, vLine[k] * meta.m_yFactor);
                    currentRow++;
                }
            }
            else if (meta.m_format == MetaData::XppY_Y) // No X value
            {
                for (size_t k = 0; k < vLine.size(); k++)
                {
                    fileData->at(nPageOffset + 0)->setValue(currentRow, meta.m_firstX + meta.m_deltaX * currentRow);
                    fileData->at(nPageOffset + 1)->setValue(currentRow, vLine[k] * meta.m_yFactor);
                    currentRow++;
                }
            }
        }

        return vFileContents.size();
    }


    /////////////////////////////////////////////////
    /// \brief This member function parses JCAMP-DX
    /// labels by removing whitespaces, minus
    /// characters and underscores from the label
    /// name itself and converting it into upper case.
    ///
    /// \param sLine string&
    /// \return void
    ///
    /////////////////////////////////////////////////
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



    /////////////////////////////////////////////////
    /// \brief This member function parses the
    /// current data line into numerical values by
    /// decoding the JCAMP-DX encoding scheme. The
    /// JCAMP-DX format is a textual data format, but
    /// the text data may be compressed to save some
    /// storage space.
    /// Reference: http://www.jcamp-dx.org/protocols.html
    ///
    /// \param sLine const std::string&
    /// \return std::vector<double>
    ///
    /////////////////////////////////////////////////
    std::vector<double> JcampDX::parseLine(const std::string& sLine)
    {
        std::vector<double> vLine;
        std::string sValue = "";
        const std::string sNumericChars = "0123456789.+-";
        double lastDiff = 0;

        // Go through the complete line and uncompress
        // the data into usual text strings, which will
        // be stored in another string
        for (size_t i = sLine.find_first_not_of(" \t"); i < sLine.length(); i++)
        {
            // The first three cases are the simplest
            // cases, where the data is not compressed
            if ((sLine[i] >= '0' && sLine[i] <= '9') || sLine[i] == '.')
                sValue += sLine[i];
            else if (sValue.length()
                     && (sLine[i] == 'e' || sLine[i] == 'E')
                     && sLine.length() > i+2
                     && (sLine[i] == '+' || sLine[i] == '-')
                     && sNumericChars.find(sValue[0]) != std::string::npos)
                sValue += sLine[i];
            else if (sValue.length()
                     && (sLine[i] == '+' || sLine[i] == '-')
                     && sNumericChars.find(sValue[0]) != std::string::npos
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
                        lastDiff = StrToDb(sValue);
                        vLine.push_back(vLine.back()+lastDiff);
                    }
                    else if ((sValue[0] >= 'j' && sValue[0] <= 'r'))
                    {
                        // Negative DIF digits
                        sValue[0] = toString(sValue[0]-'j'+1)[0];
                        lastDiff = -StrToDb(sValue);
                        vLine.push_back(vLine.back()+lastDiff);
                    }
                    else if (sValue[0] == '%')
                    {
                        // A zero
                        lastDiff = 0;
                        vLine.push_back(vLine.back());
                    }
                    else if ((sValue[0] >= 'S' && sValue[0] <= 'Z') || sValue[0] == 's')
                    {
                        // DUP digits
                        if (sValue[0] == 's')
                            sValue[0] = '9';
                        else
                            sValue[0] = sValue[0]-'S'+'1';

                        int iter = StrToInt(sValue);

                        for (int j = 0; j < iter-1; j++)
                            vLine.push_back(vLine.back()+lastDiff);

                        lastDiff = 0;
                    }
                    else
                    {
                        lastDiff = 0;
                        vLine.push_back(StrToDb(sValue)); // Simply convert into a double
                    }

                    sValue.clear();
                }

                // We convert SQZ digits directly in
                // their plain format
                if (sLine[i] >= 'A' && sLine[i] <= 'I')
                    sValue += toString(sLine[i]-'A'+1)[0];
                else if (sLine[i] >= 'a' && sLine[i] <= 'i')
                    sValue += toString('a'-sLine[i]-1);
                else if (sLine[i] == '@')
                    vLine.push_back(0);
                else if (sLine[i] == '%' && vLine.size() && sLine.size() > i+1)
                {
                    vLine.push_back(vLine.back());
                    lastDiff = 0;
                }
                else if ((vLine.size()
                    && ((sLine[i] >= 'J' && sLine[i] <= 'R')
                        || (sLine[i] >= 'j' && sLine[i] <= 'r')
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
            if (vLine.size() == 1)
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
                    vLine.push_back(vLine.back());
                }
            }

            if ((sValue[0] >= 'S' && sValue[0] <= 'Z') || sValue[0] == 's')
            {
                // DUP digits
                if (sValue[0] == 's')
                    sValue[0] = '9';
                else
                    sValue[0] = sValue[0]-'S'+'1';

                int iter = StrToInt(sValue);

                if (iter == 1)
                    vLine.pop_back();
                else
                {
                    for (int j = 0; j < iter-2; j++)
                        vLine.push_back(vLine.back()+lastDiff);
                }

                lastDiff = 0;
            }
            else if (isdigit(sValue[0]) || sValue[0] == '+' || sValue[0] == '-')
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


    /////////////////////////////////////////////////
    /// \brief This member function is used to read
    /// the targed file into the internal storage.
    /// ODS is a ZIP file containing the data
    /// formatted as XML.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void OpenDocumentSpreadSheet::readFile()
    {
        std::string sODS;

        // Get the contents of the embedded
        // XML file
        sODS = getZipFileItem("content.xml");

        // Ensure that the embedded file is
        // not empty
        if (!sODS.length())
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Parse the XML file
        tinyxml2::XMLDocument odsDocument;
        odsDocument.Parse(sODS.c_str());

        // Ensure again that the file is not empty and that
        // the necessary elements are present
        if (!odsDocument.FirstChildElement()
            || !odsDocument.FirstChildElement()->FirstChildElement("office:body")
            || !odsDocument.FirstChildElement()->FirstChildElement("office:body")->FirstChildElement()
            || !odsDocument.FirstChildElement()->FirstChildElement("office:body")->FirstChildElement()->FirstChildElement("table:table"))
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        std::vector<size_t> vTableSizes;
        std::vector<size_t> vCommentRows;

        // Get the contained table section
        tinyxml2::XMLElement* table = odsDocument.FirstChildElement()->FirstChildElement("office:body")
                                                                     ->FirstChildElement()
                                                                     ->FirstChildElement("table:table");

        // Find the column count of every table as well as the needed number of
        // table head lines
        do
        {
            tinyxml2::XMLElement* row = table->FirstChildElement("table:table-row");
            vTableSizes.push_back(0);
            vCommentRows.push_back(0);
            bool isString = true;

            // Read every row
            while (row)
            {
                tinyxml2::XMLElement* cell = row->FirstChildElement();
                size_t cellcount = 0;

                // Examine each cell
                while (cell)
                {
                    // Interpret the repeating statement only if this is not the last cell
                    // in this row
                    if (cell->NextSiblingElement())
                        cellcount += cell->IntAttribute("table:number-columns-repeated", 1);
                    else
                        cellcount++;

                    if (!(cell->Attribute("office:value-type", "string")
                        || cell->Attribute("table:number-columns-repeated")
                        || !cell->FirstChildElement()))
                        isString = false;

                    cell = cell->NextSiblingElement();
                }

                // Count comment rows
                if (isString)
                    vCommentRows.back()++;

                // Use the maximal number of cells
                if (cellcount > vTableSizes.back())
                    vTableSizes.back() = cellcount;

                row = row->NextSiblingElement("table:table-row");
            }
        } while ((table = table->NextSiblingElement("table:table")));

        // Find the total amount of needed columns
        nCols = std::accumulate(vTableSizes.begin(), vTableSizes.end(), 0);

        // Ensure that we found at least a single
        // column
        if (!nCols)
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Prepare the table column array and create string columns
        createStorage();

        for (long long int i = 0; i < nCols; i++)
            fileData->at(i).reset(new StringColumn);

        // Return to the first table
        table = odsDocument.FirstChildElement()->FirstChildElement("office:body")
                                               ->FirstChildElement()
                                               ->FirstChildElement("table:table");

        size_t nTableId = 0;
        size_t nOffSet = 0;

        // Read and parse every table in the file
        do
        {
            tinyxml2::XMLElement* row = table->FirstChildElement("table:table-row");
            size_t rowCount = 0;

            // Read every row
            while (row)
            {
                tinyxml2::XMLElement* cell = row->FirstChildElement();
                int colCount = 0;

                // Examine each cell
                while (cell)
                {
                    // Does this row still belong to the previously identified
                    // table headlines?
                    if (vCommentRows[nTableId])
                    {
                        // Ensure that it contains the text element and write
                        // that to the current columns headline
                        if (cell->FirstChildElement("text:p"))
                        {
                            if (fileData->at(colCount + nOffSet)->m_sHeadLine.length())
                                fileData->at(colCount + nOffSet)->m_sHeadLine += "\n";

                            fileData->at(colCount + nOffSet)->m_sHeadLine += utf8parser(cell->FirstChildElement("text:p")->GetText());
                        }
                    }
                    else
                    {
                        // Some data types need some special pre-processing
                        if (cell->Attribute("office:value-type", "string"))
                            fileData->at(colCount + nOffSet)->setValue(rowCount, utf8parser(cell->FirstChildElement()->GetText()));
                        else if (cell->Attribute("office:value-type", "boolean"))
                            fileData->at(colCount + nOffSet)->setValue(rowCount, cell->Attribute("office:boolean-value"));
                        else if (cell->FirstChildElement("text:p"))
                            fileData->at(colCount + nOffSet)->setValue(rowCount, cell->FirstChildElement("text:p")->GetText());
                    }

                    colCount += cell->IntAttribute("table:number-columns-repeated", 1);
                    cell = cell->NextSiblingElement();
                }

                if (vCommentRows[nTableId])
                    vCommentRows[nTableId]--;
                else
                    rowCount++;

                row = row->NextSiblingElement("table:table-row");
            }

            nOffSet += vTableSizes[nTableId];
            nTableId++;
        } while ((table = table->NextSiblingElement("table:table")));

        // Now calculate the total number of rows in this data set
        for (TblColPtr& col : *fileData)
        {
            nRows = col->size() > nRows ? col->size() : nRows;
        }
    }



    /////////////////////////////////////////////////
    /// \brief Static helper function to convert
    /// MS-Excel time values to the acutal UNIX epoch.
    ///
    /// \param sXlsTime std::string
    /// \param isTimeVal bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    static std::string convertExcelTimeToEpoch(std::string sXlsTime, bool isTimeVal)
    {
        static const double epochOffset = to_double(StrToTime("1899-12-30"));
        return toString(to_timePoint(StrToDb(sXlsTime)*24*3600 + (!isTimeVal)*epochOffset),
                        GET_MILLISECONDS | GET_FULL_PRECISION | GET_UNBIASED_TIME | (isTimeVal)*GET_ONLY_TIME);
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


    /////////////////////////////////////////////////
    /// \brief This member function is used to read
    /// the data from the XLS spreadsheet ino the
    /// internal storage.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
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

            // String lines are only used complete as comments, if their consecutive number is
            // smaller than 4 or 10% of the total number of rows (whichever is larger)
            // Otherwise only the first line is used
            if (nCommentLines)
            {
                if (nCommentLines >= std::max(4.0, 0.1*_sheet->GetTotalRows()))
                    nCommentLines = 1;
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

        // We create a string-only table and try to convert it
        // afterwards
        for (long long int j = 0; j < nCols; j++)
        {
            fileData->at(j).reset(new StringColumn);
        }

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
                    for (size_t i = 0; i < sEntry.length(); i++)
                    {
                        if (sEntry[i] == (char)13)
                            sEntry[i] = '\n';
                    }

                    // Append the string to the current table
                    // column head, if it is not empty
                    if (!fileData->at(j+nOffset)->m_sHeadLine.length())
                        fileData->at(j+nOffset)->m_sHeadLine = sEntry;
                    else if (fileData->at(j+nOffset)->m_sHeadLine != sEntry)
                        fileData->at(j+nOffset)->m_sHeadLine += "\n" + sEntry;
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

                    // Select the type of the cell and
                    // store the value, if it's a number
                    switch (_cell->Type())
                    {
                        case YExcel::BasicExcelCell::UNDEFINED:
                            fileData->at(j+nOffset)->setValue(i-vCommentLines[n], "");
                            break;
                        case YExcel::BasicExcelCell::INT:
                            fileData->at(j+nOffset)->setValue(i-vCommentLines[n], (double)_cell->GetInteger());
                            break;
                        case YExcel::BasicExcelCell::DOUBLE:
                            fileData->at(j+nOffset)->setValue(i-vCommentLines[n], _cell->GetDouble());
                            break;
                        case YExcel::BasicExcelCell::STRING:
                            fileData->at(j+nOffset)->setValue(i-vCommentLines[n], utf8parser(_cell->GetString()));
                            break;
                        case YExcel::BasicExcelCell::WSTRING:
                            fileData->at(j+nOffset)->setValue(i-vCommentLines[n], utf8parser(wcstombs(_cell->GetWString())));
                            break;
                        default:
                            fileData->at(j+nOffset)->setValue(i-vCommentLines[n], "");
                    }
                }
            }

            nOffset += nExcelCols;
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function is used to write
    /// the data in the internal storage to the
    /// target XLS spreadsheet.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
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

        // Try to pre-allocate to improve speed
        _cell = _sheet->Cell(nRows, nCols-1); // includes the headline

        if (!_cell)
            throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sFileName, SyntaxError::invalid_position, sFileName);

        // Write the headlines in the first row
        for (long long int j = 0; j < nCols; j++)
        {
            if (!fileData->at(j))
                continue;

            // Get the current cell and the headline string
            _cell = _sheet->Cell(0u, j);

            if (!_cell)
                throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sFileName, SyntaxError::invalid_position, sFileName);

            sHeadLine = fileData->at(j)->m_sHeadLine;

            // Replace newlines with the corresponding character code
            for (size_t i = 0; i < sHeadLine.length(); i++)
            {
                if (sHeadLine[i] == '\n')
                    sHeadLine[i] = (char)13;
            }

            // Write the headline
            _cell->SetString(sHeadLine.c_str());
        }

        // Now write the actual table
        for (long long int j = 0; j < nCols; j++)
        {
            if (!fileData->at(j))
                continue;

            for (long long int i = 0; i < nRows; i++)
            {
                // Write the cell contents, if the data table contains valid data
                // otherwise clear the cell
                if (!fileData->at(j)->isValid(i))
                    continue;

                // Get the current cell (skip over the first row, because it contains the headline)
                _cell = _sheet->Cell(1 + i, j);

                if (!_cell)
                    throw SyntaxError(SyntaxError::CANNOT_SAVE_FILE, sFileName, SyntaxError::invalid_position, sFileName);

                if (fileData->at(j)->m_type == TableColumn::TYPE_VALUE || fileData->at(j)->m_type == TableColumn::TYPE_LOGICAL)
                    _cell->SetDouble(fileData->at(j)->getValue(i).real());
                else
                    _cell->SetString(fileData->at(j)->getValueAsInternalString(i).c_str());
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


    /////////////////////////////////////////////////
    /// \brief This member function is used to read
    /// the data from the XLSX spreadsheet into the
    /// internal storage. XLSX is a ZIP file
    /// containing the data formatted as XML.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void XLSXSpreadSheet::readFile()
    {
        unsigned int nSheets = 0;
        long long int nExcelLines = 0;
        long long int nExcelCols = 0;
        long long int nOffset = 0;
        int nRow = 0, nCol = 0;
        int nRowmin = 0, nRowmax = 0;
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
        tinyxml2::XMLDocument _styles;
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

            // Determine the dimensions of this sheet
            evalIndices(sCellLocation.substr(0, sCellLocation.find(':')), nRowmin, nColmin);
            evalIndices(sCellLocation.substr(sCellLocation.find(':') + 1), nRowmax, nColmax);
            tinyxml2::XMLElement* cols = _sheet.FirstChildElement()->FirstChildElement("cols");

            // We use the columns identifiers to check and probably
            // update the necessary columns
            if (cols && cols->FirstChildElement())
            {
                cols = cols->FirstChildElement();

                do
                {
                    if (cols->IntAttribute("max") > nColmax+1)
                        nColmax = cols->IntAttribute("max")-1;

                } while ((cols = cols->NextSiblingElement()));
            }

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
                int currentRow = _node->ToElement()->IntAttribute("r");
                _element = _node->ToElement()->FirstChildElement("c");

                // Row does not contain any cells
                if (!_element)
                {
                    vCommentLines[i] = currentRow;
                    continue;
                }

                size_t cellCount = 0;

                do
                {
                    cellCount++;

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
                    else if (_element->FirstChildElement()) // Ensure that the cell is not empty
                    {
                        bBreakSignal = true;
                        break;
                    }
                }
                while ((_element = _element->NextSiblingElement()));

                // Only use the first row as headline candidates
                if (!bBreakSignal)
                    vCommentLines[i] = currentRow;

                // Search for the first nearly complete line of strings
                if (bBreakSignal || 4/3.0 * cellCount >= nColmax-nColmin+1)
                    break;
            }
            while ((_node = _node->NextSibling()));

            bBreakSignal = false;

            // Calculate the maximal number of needed
            // rows to store all sheets next to each
            // other
            if (nExcelLines < nRowmax-nRowmin+1-vCommentLines[i])
                nExcelLines = nRowmax-nRowmin+1-vCommentLines[i];

            // Add the number of columns to the total
            // number of columns
            nExcelCols += nColmax-nColmin+1;

            g_logger.info("headlines=" + toString(vCommentLines[i]));
        }

        // Set the dimensions of the final table
        nRows = nExcelLines;
        nCols = nExcelCols;

        // Allocate the memory
        createStorage();

        // We create a text-only table first
        for (long long int j = 0; j < nCols; j++)
        {
            fileData->at(j).reset(new StringColumn);
        }

        // Walk through the sheets and extract the
        // contents to memory
        //
        // Get the contents of the shared strings
        // XML file and parse it
        sStringsContent = getZipFileItem("xl/sharedStrings.xml");
        _strings.Parse(sStringsContent.c_str());
        std::string sStylesContent = getZipFileItem("xl/styles.xml");
        _styles.Parse(sStylesContent.c_str());
        tinyxml2::XMLElement* cellXfs = _styles.FirstChildElement()->FirstChildElement("cellXfs");

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
            evalIndices(sCellLocation.substr(0, sCellLocation.find(':')), nRowmin, nColmin);
            evalIndices(sCellLocation.substr(sCellLocation.find(':')+1), nRowmax, nColmax);

            tinyxml2::XMLElement* cols = _sheet.FirstChildElement()->FirstChildElement("cols");

            if (cols && cols->FirstChildElement())
            {
                cols = cols->FirstChildElement();

                do
                {
                    if (cols->IntAttribute("max") > nColmax+1)
                        nColmax = cols->IntAttribute("max")-1;

                } while ((cols = cols->NextSiblingElement()));
            }

            // Go through each cell and store its
            // value at the correct position in the
            // final table. If we hit a textual cell,
            // then we store its contents as a table
            // column head
            do
            {
                _element = _node->ToElement()->FirstChildElement("c");

                // Does not contain any cells
                if (!_element)
                    continue;

                // Go through the cells of the current
                // row
                do
                {
                    sCellLocation = _element->Attribute("r");
                    evalIndices(sCellLocation, nRow, nCol);
                    nCol -= nColmin;
                    nRow -= nRowmin;

                    if (nCol+nOffset >= nCols)
                        continue;

                    // catch textual cells and store them
                    // in the corresponding table column
                    // head
                    if (_element->Attribute("t") && _element->Attribute("t") == string("s"))
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
                            if (nRow - vCommentLines[i] < 0)
                            {
                                if (!fileData->at(nCol+nOffset)->m_sHeadLine.length())
                                    fileData->at(nCol+nOffset)->m_sHeadLine = sEntry;
                                else if (fileData->at(nCol+nOffset)->m_sHeadLine != sEntry)
                                    fileData->at(nCol+nOffset)->m_sHeadLine += "\n" + sEntry;
                            }
                            else
                                fileData->at(nCol+nOffset)->setValue(nRow-vCommentLines[i], sEntry);
                        }

                        continue;
                    }
                    else if (_element->FirstChildElement("v"))
                    {
                        std::string sValue = utf8parser(_element->FirstChildElement("v")->GetText());

                        // Decode styles
                        if (_element->Attribute("t") && _element->Attribute("t") == string("b"))
                            sValue = sValue == "0" ? "false" : "true";
                        else if (_element->Attribute("s") && cellXfs && cellXfs->FirstChildElement())
                        {
                            // Style ID
                            int styleId = _element->IntAttribute("s");
                            tinyxml2::XMLElement* style = cellXfs->FirstChildElement();

                            // Find the correct style ID
                            while (styleId && style)
                            {
                                style = style->NextSiblingElement();
                                styleId--;
                            }

                            if (!styleId && style && style->Attribute("numFmtId"))
                            {
                                styleId = style->IntAttribute("numFmtId");

                                // Those are time formats: 14-22, 45-47
                                if ((styleId >= 14 && styleId <= 22) || (styleId >= 45 && styleId <= 47))
                                    sValue = convertExcelTimeToEpoch(sValue, (styleId >= 18 && styleId <= 21)
                                                                              || (styleId >= 45 && styleId >= 47));
                            }
                        }

                        // Write the decoded string to the table column array
                        fileData->at(nCol+nOffset)->setValue(nRow-vCommentLines[i], sValue);
                    }
                }
                while ((_element = _element->NextSiblingElement()));
            }
            while ((_node = _node->NextSibling()));

            nOffset += nColmax-nColmin+1;
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function converts the
    /// usual Excel indices into numerical ones.
    ///
    /// \param _sIndices const string&
    /// \param nLine int&
    /// \param nCol int&
    /// \return void
    ///
    /////////////////////////////////////////////////
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


    /////////////////////////////////////////////////
    /// \brief This copy constructor extends the copy
    /// constructor of the GenericFile class.
    ///
    /// \param file const IgorBinaryWave&
    ///
    /////////////////////////////////////////////////
    IgorBinaryWave::IgorBinaryWave(const IgorBinaryWave& file) : GenericFile(file)
    {
        bXZSlice = file.bXZSlice;
    }


    IgorBinaryWave::~IgorBinaryWave()
    {
        // Empty destructor
    }


    /////////////////////////////////////////////////
    /// \brief This member function is used to read
    /// the contents of the IBW file into the
    /// internal storage. We use the IBW library to
    /// read the binary file.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
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
        }
        else if (nDim[1] && (!nDim[2] || nDim[2] == 1))
        {
            nCols += 2;
            nFirstCol = 2;

            if (nDim[1] > nDim[0])
                nRows = nDim[1];
        }
        else if (nDim[1] && nDim[2] && (!nDim[3] || nDim[3] == 1))
        {
            nCols += 3;
            nFirstCol = 3;
            nRows = nDim[2] > nRows ? nDim[2] : nRows;
            nRows = nDim[1] > nRows ? nDim[1] : nRows;
        }

        // Create the internal storage using the
        // final dimensions
        createStorage();

        // We create plain value column tables
        for (long long int j = 0; j < nCols; j++)
        {
            fileData->at(j).reset(new ValueColumn);
        }

        // Fill the x column and its corresponding
        // table column head
        for (long long int j = 0; j < nFirstCol; j++)
        {
            fileData->at(j)->m_sHeadLine = cName + string("_[")+(char)('x'+j)+string("]");

            for (long long int i = 0; i < nDim[j]; i++)
            {
                fileData->at(j)->setValue(i, dScalingFactorA[j]*(double)i + dScalingFactorB[j]);
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
                fileData->at(1)->m_sHeadLine = cName + string("_[y]");
            else
                fileData->at(j+nFirstCol)->m_sHeadLine = cName + string("_["+toString(j+1)+"]");

            // Write the actual data to the table.
            // We have to take care about the type
            // of the data and whether the data is
            // complex or not
            long long int nInsertion = 0;

            for (long long int i = 0; i < (nDim[0]+bReadComplexData*nDim[0]); i++)
            {
                if (dData)
                {
                    if (bReadComplexData)
                        fileData->at(nSliceCounter+nFirstCol)->setValue(nInsertion, mu::value_type(dData[i+j*2*nDim[0]], dData[i+1+j*2*nDim[0]]));
                    else
                        fileData->at(nSliceCounter+nFirstCol)->setValue(nInsertion, dData[i+j*(nDim[0])]);
                }
                else if (fData)
                {
                    if (bReadComplexData)
                        fileData->at(nSliceCounter+nFirstCol)->setValue(nInsertion, mu::value_type(fData[i+j*2*nDim[0]], fData[i+1+j*2*nDim[0]]));
                    else
                        fileData->at(nSliceCounter+nFirstCol)->setValue(nInsertion, (double)fData[i+j*(nDim[0])]);
                }
                else if (n8_tData)
                {
                    if (bReadComplexData)
                        fileData->at(nSliceCounter+nFirstCol)->setValue(nInsertion, mu::value_type(n8_tData[i+j*2*nDim[0]], n8_tData[i+1+j*2*nDim[0]]));
                    else
                        fileData->at(nSliceCounter+nFirstCol)->setValue(nInsertion, (double)n8_tData[i+j*(nDim[0])]);

                }
                else if (n16_tData)
                {
                    if (bReadComplexData)
                        fileData->at(nSliceCounter+nFirstCol)->setValue(nInsertion, mu::value_type(n16_tData[i+j*2*nDim[0]], n16_tData[i+1+j*2*nDim[0]]));
                    else
                        fileData->at(nSliceCounter+nFirstCol)->setValue(nInsertion, (double)n16_tData[i+j*(nDim[0])]);
                }
                else if (n32_tData)
                {
                    if (bReadComplexData)
                        fileData->at(nSliceCounter+nFirstCol)->setValue(nInsertion, mu::value_type(n32_tData[i+j*2*nDim[0]], n32_tData[i+1+j*2*nDim[0]]));
                    else
                        fileData->at(nSliceCounter+nFirstCol)->setValue(nInsertion, (double)n32_tData[i+j*(nDim[0])]);
                }

                nInsertion++;
                i += bReadComplexData;
            }
        }

        // Close the file reference and
        // free the allocated memory
        CPCloseFile(_cp_file_ref);

        if (vWaveDataPtr != nullptr)
            free(vWaveDataPtr);
    }


    /////////////////////////////////////////////////
    /// \brief This is an overload for the assignment
    /// operator of the GenericFile class.
    ///
    /// \param file const IgorBinaryWave&
    /// \return IgorBinaryWave&
    ///
    /////////////////////////////////////////////////
    IgorBinaryWave& IgorBinaryWave::operator=(const IgorBinaryWave& file)
    {
        assign(file);
        bXZSlice = file.bXZSlice;

        return *this;
    }


    //////////////////////////////////////////////
    // class ZygoDat
    //////////////////////////////////////////////
    //
    ZygoDat::ZygoDat(const string& filename) : GenericFile(filename)
    {
        // Empty constructor
    }


    /////////////////////////////////////////////////
    /// \brief This copy constructor extends the copy
    /// constructor of the GenericFile class.
    ///
    /// \param file const ZygoDat&
    ///
    /////////////////////////////////////////////////
    ZygoDat::ZygoDat(const ZygoDat& file) : GenericFile(file)
    {
        // Empty constructor
    }


    ZygoDat::~ZygoDat()
    {
        // Empty destructor
    }


    /////////////////////////////////////////////////
    /// \brief This member function is used to read
    /// the contents of the Zygo Dat file into the
    /// internal storage. We use the Zygo library to
    /// read the binary file.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void ZygoDat::readFile()
    {
        // Use the ZygoLib to open the file
        ZygoLib::DatFile zygofile(sFileName, false);
        std::vector<ZygoLib::InterferogramData> fileContent;

        // Try to read its contents
        try
        {
            fileContent = zygofile.read();
        }
        catch (...)
        {
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);
        }

        // Determine the needed dimensions for the final
        // storage
        for (const ZygoLib::InterferogramData& layer : fileContent)
        {
            if (std::max(layer.header.cn_height, layer.header.cn_width) > nRows)
                nRows = std::max(layer.header.cn_height, layer.header.cn_width);

            nCols += 2+layer.header.cn_height;
        }

        // Ensure that we actually read something
        if (!nRows)
            throw SyntaxError(SyntaxError::FILE_IS_EMPTY, sFileName, SyntaxError::invalid_position, sFileName);

        // Create the internal storage using the
        // final dimensions
        createStorage();

        // We create plain value column tables
        for (long long int j = 0; j < nCols; j++)
        {
            fileData->at(j).reset(new ValueColumn);
        }

        uint16_t colOffset = 0;

        // Copy each layer to the target data
        for (const ZygoLib::InterferogramData& layer : fileContent)
        {
            fileData->at(colOffset)->m_sHeadLine = "x";
            fileData->at(colOffset+1)->m_sHeadLine = "y";

            // Write the x column
            for (uint16_t i = 0; i < layer.header.cn_width; i++)
            {
                fileData->at(colOffset)->setValue(i, i * layer.header.camera_res);
            }

            // Write the y column
            for (uint16_t j = 0; j < layer.header.cn_height; j++)
            {
                fileData->at(colOffset+1)->setValue(j, j * layer.header.camera_res);
            }

            // Write the phase data and consider the needed transposition of height and width
            for (uint16_t i = 0; i < layer.header.cn_width; i++)
            {
                for (uint16_t j = 0; j < layer.header.cn_height; j++)
                {
                    if (!i)
                        fileData->at(j+colOffset+2)->m_sHeadLine = "z(x(:),y(" + toString(j+1) + "))";

                    fileData->at(j+colOffset+2)->setValue(i, layer.phaseMatrix[j][i]);
                }
            }

            // The layers are put next to each other
            colOffset += 2+layer.header.cn_height;
        }
    }


    /////////////////////////////////////////////////
    /// \brief This is an overload for the assignment
    /// operator of the GenericFile class.
    ///
    /// \param file const ZygoDat&
    /// \return ZygoDat&
    ///
    /////////////////////////////////////////////////
    ZygoDat& ZygoDat::operator=(const ZygoDat& file)
    {
        assign(file);
        return *this;
    }
}


