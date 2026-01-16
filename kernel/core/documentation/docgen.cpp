/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2020  Erik Haenel et al.

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

#include "docgen.hpp"
#include "../../../common/datastructures.h"
#include "../../kernel.hpp"
#include "../utils/tools.hpp"
#include "../procedure/dependency.hpp"
#include "../ui/winlayout.hpp"
#include <boost/nowide/fstream.hpp>

#include <regex>


/////////////////////////////////////////////////
/// \brief This member function finds all
/// dependent procedure files. If the current
/// file is not a procedure, the recursion
/// terminates.
///
/// \param sFile const std::string&
/// \param fileSet std::set<std::string>&
/// \param vFiles std::vector<std::string>&
/// \return void
///
/////////////////////////////////////////////////
void DocumentationGenerator::followBranch(const std::string& sFile, std::set<std::string>& fileSet, std::vector<std::string>& vFiles) const
{
    // Add the current file to the set
    fileSet.insert(sFile);
    vFiles.push_back(sFile);

    // Is the current file a window layout?
    if (sFile.find(".nlyt") != std::string::npos)
    {
        // Get all event procedures
        std::set<std::string> eventProcs = getEventProcedures(sFile);

        // Get the dependencies for each event procedure
        for (const auto& eventProc : eventProcs)
        {
            if (fileSet.find(eventProc) == fileSet.end())
                followBranch(eventProc, fileSet, vFiles);
        }

        // Return. Nothing more to be done
        return;
    }
    else if (sFile.find(".nprc") == std::string::npos)
        return;

    // Get a reference to the procedure library
    // and calculate the dependencies of the current file
    ProcedureLibrary& procLib = NumeReKernel::getInstance()->getProcedureLibrary();
    Dependencies* dep = procLib.getProcedureContents(replacePathSeparator(sFile))->getDependencies();

    // If the returned dependencies are not already part of
    // the set, call this function recursively using the dependencies,
    // which are not part of the set yet
    for (auto iter = dep->getDependencyMap().begin(); iter != dep->getDependencyMap().end(); ++iter)
    {
        for (auto listiter = iter->second.begin(); listiter != iter->second.end(); ++listiter)
        {
            if (fileSet.find(listiter->getFileName()) == fileSet.end())
                followBranch(listiter->getFileName(), fileSet, vFiles);
        }
    }
}


/////////////////////////////////////////////////
/// \brief Writes the content of the current code
/// file to a LaTeX file.
///
/// \param sFileName const std::string&
/// \return std::string
///
/// The contents of the current code file are
/// converted into a LaTeX file, where the code
/// sections are printed as listings and the
/// documentation strings are used as normal text.
/////////////////////////////////////////////////
std::string DocumentationGenerator::convertToLaTeX(const std::string& sFileName) const
{
    std::string sFileContents;
    std::string sLaTeXFileName = createLaTeXFileName(sFileName) + ".tex";
    std::string sMainProc = createMainProcedure(sFileName);
    boost::nowide::ofstream file_out;

    bool bTextMode = true;
    int startpos = 0;

    sFileContents += "% Created by NumeRe from the source of " + sFileName + "\n\n";

    StyledTextFile file(sFileName);

    // Go through the whole file and convert the
    // syntax elements correspondingly
    for (int i = 0; i < file.getLastPosition(); i++)
    {
        // Determine the type of documentation,
        // into which the current and the following
        // characters shall be converted
        if (file.getStyleAt(i) == StyledTextFile::COMMENT_DOC_LINE) // That's a documentation
        {
            if (!bTextMode)
            {
                if (i - startpos > 1)
                    sFileContents += getStrippedRange(file, startpos, i) + "\n";

                bTextMode = true;
                sFileContents += "\\end{lstlisting}\n";
            }

            int j = i;

            // Find all combined documentation comment lines
            while (file.getStyleAt(j) == StyledTextFile::COMMENT_DOC_LINE)
                j = file.PositionFromLine(file.LineFromPosition(j) + 1);

            sFileContents += parseDocumentation(file, sMainProc, i + 3, file.getLineEndPosition(file.LineFromPosition(j) - 1)) + "\n";
            i = j-1;
            startpos = j-1;
        }
        else if (file.getStyleAt(i) == StyledTextFile::COMMENT_LINE && file.getTextRange(i, i + 3) == "##~") // ignore that (escaped comment)
        {
            if (i - startpos > 1)
                sFileContents += getStrippedRange(file, startpos, i) + "\n";

            i = file.getLineEndPosition(file.LineFromPosition(i)) + 1;
            startpos = i;
        }
        else if (file.getStyleAt(i) == StyledTextFile::COMMENT_DOC_BLOCK) // that's also a documentation
        {
            if (!bTextMode)
            {
                if (i - startpos > 1)
                    sFileContents += getStrippedRange(file, startpos, i) + "\n";

                bTextMode = true;
                sFileContents += "\\end{lstlisting}\n";
            }

            for (int j = i + 3; j < file.getLastPosition(); j++)
            {
                if (file.getStyleAt(j+3) != StyledTextFile::COMMENT_DOC_BLOCK || j + 1 == file.getLastPosition())
                {
                    sFileContents += parseDocumentation(file, sMainProc, i + 3, j) + "\n";
                    i = j + 2;
                    break;
                }
            }

            startpos = i;
        }
        else if (file.getStyleAt(i) == StyledTextFile::COMMENT_BLOCK && file.getTextRange(i, i + 3) == "#**") // ignore that, that's also an escaped comment
        {
            if (i - startpos > 1)
                sFileContents += getStrippedRange(file, startpos, i) + "\n";

            for (int j = i + 3; j < file.getLastPosition(); j++)
            {
                if (file.getStyleAt(j+3) != StyledTextFile::COMMENT_BLOCK || j + 1 == file.getLastPosition())
                {
                    i = j + 2;
                    break;
                }
            }

            startpos = i + 1;
        }
        else // a normal code fragment
        {
            if (bTextMode)
            {
                // Find the next code line containing not only whitespace or
                // hidden comments or the terminal section of block comments
                // (which are from a previous documentation block)
                int line = file.LineFromPosition(i);

                // The position might be already at the end of the line
                // (at the end of a documentation block)
                if (i == file.getLineEndPosition(line))
                    line++;

                while (line < file.getLinesCount())
                {
                    std::string sLine = file.getLine(line);
                    StripSpaces(sLine);

                    if (sLine.length() && !sLine.starts_with("##~") && sLine != "*#")
                    {
                        i = file.PositionFromLine(line);
                        break;
                    }

                    line++;
                }

                startpos = i;
                bTextMode = false;
                sFileContents += "\\begin{lstlisting}[firstnumber=" + toString(file.LineFromPosition(i)+1) + "]\n";
            }

            if (i + 1 == file.getLastPosition())
            {
                if (i - startpos > 1)
                {
                    sFileContents += getStrippedRange(file, startpos, i) + "\n";
                }
            }
        }
    }

    // Append a trailing \end{lstlisting}, if needed
    if (!bTextMode)
        sFileContents += "\\end{lstlisting}\n";

    if (!sFileContents.length())
        return "";

    // Write the converted documentation to the
    // target LaTeX file
    file_out.open(sLaTeXFileName.c_str());

    if (!file_out.good())
        return "";

    file_out << sFileContents;
    file_out.close();

    return sLaTeXFileName;
}


/////////////////////////////////////////////////
/// \brief Gets the contents of the selected range.
///
/// \param file StyledTextFile representation of
/// the loaded file
/// \param pos1 int first character to extract
/// \param pos2 int last character to extract
/// \param encode bool encode umlauts
/// \return std::string
///
/// The content between both positions is returned
/// without leading and trailing whitespaces,
/// tabulators and line ending characters. Carriage
/// returns in the middle of the selection are omitted
/// and only line endings are kept. If the encoding
/// option is activated, then umlauts are converted
/// into their two-letter representation.
/////////////////////////////////////////////////
std::string DocumentationGenerator::getStrippedRange(const StyledTextFile& file, int pos1, int pos2, bool encode) const
{
    std::string sTextRange = file.getTextRange(pos1, pos2);
    bool isDocumentation = file.getStyleAt(pos1) == StyledTextFile::COMMENT_DOC_LINE
        || file.getStyleAt(pos1) == StyledTextFile::COMMENT_DOC_BLOCK;

    // Remove leading whitespaces
    while ((!isDocumentation && sTextRange.front() == ' ') || sTextRange.front() == '\r' || sTextRange.front() == '\n' )
        sTextRange.erase(0, 1);

    // Remove trailing whitespaces
    while (sTextRange.back() == ' ' || sTextRange.back() == '\t' || sTextRange.back() == '\r' || sTextRange.back() == '\n')
        sTextRange.erase(sTextRange.length() - 1);

    // Convert CR LF into LF only
    while (sTextRange.find("\r\n") != std::string::npos)
        sTextRange.replace(sTextRange.find("\r\n"), 2, "\n");

    // Remove documentation comment sequences
    size_t nCommentSeq;

    while ((nCommentSeq = sTextRange.find("\n##!")) != std::string::npos)
        sTextRange.erase(nCommentSeq+1, 3);
        //sTextRange.erase(nCommentSeq+1, sTextRange.find_first_not_of(" \t", nCommentSeq+4) - nCommentSeq-1);

    // Convert umlauts, if the encode flag
    // has been set
    if (encode)
    {
        for (size_t i = 0; i < sTextRange.length(); i++)
        {
            switch (sTextRange[i])
            {
                case (char)0xC4:
                    sTextRange.replace(i, 1, "Ae");
                    break;
                case (char)0xE4:
                    sTextRange.replace(i, 1, "ae");
                    break;
                case (char)0xD6:
                    sTextRange.replace(i, 1, "Oe");
                    break;
                case (char)0xF6:
                    sTextRange.replace(i, 1, "oe");
                    break;
                case (char)0xDC:
                    sTextRange.replace(i, 1, "Ue");
                    break;
                case (char)0xFC:
                    sTextRange.replace(i, 1, "ue");
                    break;
                case (char)0xDF:
                    sTextRange.replace(i, 1, "ss");
                    break;
            }
        }
    }

    if (sTextRange.find_first_not_of('\t') == std::string::npos)
        return "";

    return sTextRange;
}


/////////////////////////////////////////////////
/// \brief Converts the documentation into LaTeX code.
///
/// \param file StyledTextFile representation of
/// the loaded file
/// \param sMainProc const std::string&
/// \param pos1 int
/// \param pos2 int
/// \return string
///
/// The documentation extracted from the code
/// comments is converted to LaTeX text. This
/// includes:
/// \li unordered lists
/// \li german umlauts
/// \li inline code sequences
/////////////////////////////////////////////////
std::string DocumentationGenerator::parseDocumentation(const StyledTextFile& file, const std::string& sMainProc, int pos1, int pos2) const
{
    // Get the text range
    std::string sTextRange = getStrippedRange(file, pos1, pos2, false);

    // Handle procedure names
    if (sTextRange.find("\\procedure ") != std::string::npos)
    {
        // Find start and end of procedure name
        size_t nPosStart = sTextRange.find("\\procedure ")+10;
        nPosStart = sTextRange.find_first_not_of(' ', nPosStart);
        size_t nPosEnd = sTextRange.find_first_not_of(" \r\n", nPosStart);
        nPosEnd = sTextRange.find_first_of(" \r\n", nPosEnd);

        std::string sProcedure = sTextRange.substr(nPosStart, nPosEnd-nPosStart);
        std::string sMain;
        std::string sDependencies;

        if (sMainProc.length())
        {
            if (sMainProc != sProcedure)
            {
                if (sMainProc.find('~') != std::string::npos && sMainProc.substr(sMainProc.rfind('~')+1) == sProcedure)
                    sProcedure = sMainProc;
                else
                {
                    sProcedure.insert(0, "thisfile~");
                    sMain = sMainProc;

                    if (sMain.find('~') != std::string::npos)
                        sMain.erase(0, sMain.rfind('~')+1);

                    replaceAll(sMain, "_", "\\_");
                    sMain.insert(0, "(): local member of \\$");
                }
            }

            ProcedureLibrary& _procLib = NumeReKernel::getInstance()->getProcedureLibrary();
            std::map<std::string, DependencyList>& mDependencies = _procLib.getProcedureContents(file.getFileName())->getDependencies()->getDependencyMap();
            std::map<std::string, DependencyList>::iterator iter;

            if (sProcedure.find("thisfile~") != std::string::npos)
                iter = mDependencies.find("$" + sMainProc + "::" + sProcedure);
            else
                iter = mDependencies.find("$" + sProcedure);

            if (iter != mDependencies.end())
            {
                DependencyList _depList = iter->second;

                for (auto listiter = _depList.begin(); listiter != _depList.end(); ++listiter)
                {
                    if (sDependencies.length())
                        sDependencies += ", ";

                    if (listiter->getType() == Dependency::NPRC)
                    {
                        if (listiter->getProcedureName().find("::thisfile~") != std::string::npos)
                            sDependencies += "!!$" + listiter->getProcedureName().substr(listiter->getProcedureName().find("::")+2) + "()!!";
                        else
                            sDependencies += "!!" + listiter->getProcedureName() + "()!!";
                    }
                    else
                        sDependencies += "!!" + listiter->getProcedureName() + "!!";
                }

                if (sDependencies.length())
                    sDependencies = "\n\\depends{" + sDependencies + "}";
            }

        }

        // Insert procedure call with namespace
        // and braces around the procedure name
        sTextRange.insert(nPosEnd, sMain + "}{$" + sProcedure + "()}" + sDependencies);
        sTextRange[nPosStart-1] = '{';
    }

    // Handle layout names
    if (sTextRange.find("\\layout ") != std::string::npos)
    {
        // Find start and end of layout name
        size_t nPosStart = sTextRange.find("\\layout ")+7;
        nPosStart = sTextRange.find_first_not_of(' ', nPosStart);
        size_t nPosEnd = sTextRange.find_first_not_of(" \r\n", nPosStart);
        nPosEnd = sTextRange.find_first_of(" \r\n", nPosEnd);

        std::string sLayout = sTextRange.substr(nPosStart, nPosEnd-nPosStart);
        replaceAll(sLayout, "_", "\\_");

        // Insert braces around the layout name
        sTextRange.replace(nPosStart-1, nPosEnd-nPosStart+1, "{" + sLayout + "}");
    }

    // Handle unordered lists
    if (sTextRange.find("- ") != std::string::npos) // thats an unordered list
    {
        size_t nItemizeStart = 0;
        size_t nLength = 0;

        while ((nItemizeStart = findListItem(sTextRange, nLength)) != std::string::npos)
        {
            std::string sItemStart = "\n" + std::string(nLength-3, ' ') + "- ";
            std::string sIndent = "\n" + std::string(nLength-1, ' ');

            for (size_t i = nItemizeStart; i < sTextRange.length(); i++)
            {
                if (sTextRange.substr(i, nLength) == sItemStart)
                {
                    sTextRange.replace(i + 1, nLength-2, "\t\\item");
                    continue;
                }

                if ((sTextRange[i] == '\n' && sTextRange.substr(i, nLength) != sIndent) || i + 1 == sTextRange.length())
                {
                    if (sTextRange[i] == '\n')
                        sTextRange.insert(i+1, "\\end{itemize}\n");
                    else
                        sTextRange += "\n\\end{itemize}\n";

                    sTextRange.insert(nItemizeStart + 1, "\\begin{itemize}\n");
                    break;
                }
            }
        }
    }

    // Handle ordered lists
    if (std::regex_search(sTextRange, std::regex("( *\\d+\\. +)(?=\\S+)"))) // thats an ordered list
    {
        size_t nItemizeStart = 0;
        size_t nLength = 0;
        size_t currentLength;

        while ((nItemizeStart = findEnumItem(sTextRange, nLength)) != std::string::npos)
        {
            std::string sIndent = "\n" + std::string(nLength, ' ');

            for (size_t i = nItemizeStart; i < sTextRange.length(); i++)
            {
                if (sTextRange[i] == '\n' && sTextRange.substr(i, nLength+1) != sIndent)
                {
                    // This either a new enumeration item or the end
                    if (findEnumItem(sTextRange.substr(i+1), currentLength) == 0
                        && currentLength == nLength)
                        sTextRange.replace(i + 1, nLength-1, "\t\\item");
                    else
                    {
                        sTextRange.insert(i+1, "\\end{enumerate}\n");
                        sTextRange.replace(nItemizeStart, nLength-1, "\t\\item");
                        sTextRange.insert(nItemizeStart, "\\begin{enumerate}\n");
                        break;
                    }
                }
                else if (i+1 == sTextRange.length())
                {
                    // The last character, needs now the environment
                    sTextRange.replace(nItemizeStart, nLength-1, "\t\\item");
                    sTextRange.insert(nItemizeStart, "\\begin{enumerate}\n");
                    sTextRange += "\n\\end{enumerate}\n";
                    break;
                }
            }
        }
    }

    // Handle parameter lists
    if (sTextRange.find("\\param ") != std::string::npos)
    {
        while (sTextRange.find("\\param ") != std::string::npos)
        {
            size_t nItemizeStart = sTextRange.find("\\param ");
            size_t nLastParam = nItemizeStart;
            std::vector<std::string> vParameters;

            for (size_t i = nItemizeStart+1; i < sTextRange.length(); i++)
            {
                if (sTextRange.substr(i, 7) == "\\param ")
                {
                    vParameters.push_back(sTextRange.substr(nLastParam, i - nLastParam));
                    nLastParam = i;
                    continue;
                }

                if (i + 1 == sTextRange.length()
                    || (sTextRange[i] == '\n' && sTextRange[i+1] == '\n')
                    || (sTextRange[i+1] == '\\' && sTextRange.substr(i+1, 7) != "\\param "))
                {
                    vParameters.push_back(sTextRange.substr(nLastParam, i - nLastParam+1));
                    sTextRange.replace(nItemizeStart, i+1 - nItemizeStart, "\\parameters\n\\noindent\n" + createParametersTable(vParameters));

                    break;
                }
            }
        }
    }

    // Handle return value lists
    if (sTextRange.find("\\return ") != std::string::npos)
    {
        while (sTextRange.find("\\return ") != std::string::npos)
        {
            size_t nItemizeStart = sTextRange.find("\\return ");
            size_t nLastParam = nItemizeStart;
            std::vector<std::string> vReturns;

            for (size_t i = nItemizeStart+1; i < sTextRange.length(); i++)
            {
                if (sTextRange.substr(i, 8) == "\\return ")
                {
                    vReturns.push_back(sTextRange.substr(nLastParam, i - nLastParam));
                    nLastParam = i;
                    continue;
                }

                if (i + 1 == sTextRange.length()
                    || (sTextRange[i] == '\n' && sTextRange[i+1] == '\n')
                    || (sTextRange[i+1] == '\\' && sTextRange.substr(i+1, 8) != "\\return "))
                {
                    vReturns.push_back(sTextRange.substr(nLastParam, i - nLastParam+1));
                    sTextRange.replace(nItemizeStart, i+1 - nItemizeStart, "\\returns\n\\noindent\n" + createReturnsTable(vReturns));

                    break;
                }
            }
        }
    }

    // Convert umlauts in LaTeX command sequences
    for (size_t i = 0; i < sTextRange.length(); i++)
    {
        switch (sTextRange[i])
        {
            case (char)0xC4:
                sTextRange.replace(i, 1, "\\\"A");
                break;
            case (char)0xE4:
                sTextRange.replace(i, 1, "\\\"a");
                break;
            case (char)0xD6:
                sTextRange.replace(i, 1, "\\\"O");
                break;
            case (char)0xF6:
                sTextRange.replace(i, 1, "\\\"o");
                break;
            case (char)0xDC:
                sTextRange.replace(i, 1, "\\\"U");
                break;
            case (char)0xFC:
                sTextRange.replace(i, 1, "\\\"u");
                break;
            case (char)0xDF:
                sTextRange.replace(i, 1, "\\ss ");
                break;
        }
    }

    // Handle inline code sequences
    for (size_t i = 0; i < sTextRange.length(); i++)
    {
        if (sTextRange.substr(i, 2) == "!!")
        {
            for (size_t j = i + 2; j < sTextRange.length(); j++)
            {
                if (sTextRange.substr(j, 2) == "!!")
                {
                    sTextRange.replace(j, 2, "`");
                    sTextRange.replace(i, 2, "\\lstinline[keepspaces=true]`");
                    break;
                }
            }
        }
    }

    return sTextRange;
}


/////////////////////////////////////////////////
/// \brief This member function creates a table
/// out of the declared parameters in the
/// documentation comments.
///
/// \param vParams const std::vector<std::string>&
/// \return std::string
///
/////////////////////////////////////////////////
std::string DocumentationGenerator::createParametersTable(const std::vector<std::string>& vParams) const
{
    // Table heads
    std::string sTable = "\\begin{tabular}{p{0.22\\textwidth}p{0.15\\textwidth}p{0.55\\textwidth}}\n\t\\toprule\n\tParameter & Defaults to & Description\\\\\n\t\\midrule\n";
    std::string sParameter;

    size_t nDescPos;

    // Write every single parameter line
    for (size_t i = 0; i < vParams.size(); i++)
    {
        sParameter = vParams[i];
        replaceAll(sParameter, "\n", " ");

        size_t nDefaultPos = sParameter.find("(!!", 8);

        if (nDefaultPos != std::string::npos)
            nDescPos = sParameter.find_first_not_of(' ', sParameter.find("!!)")+3);
        else
            nDescPos = sParameter.find_first_not_of(' ', sParameter.find(' ', 8));

        sTable += "\t!!" + sParameter.substr(7, sParameter.find(' ', 8) - 7) + "!! & ";

        if (nDefaultPos != std::string::npos)
        {
            // Exclude the initial equal sign, if possibl
            if (sParameter[nDefaultPos+3] == '=')
                sTable += "!!" + sParameter.substr(nDefaultPos+4, sParameter.find("!!)")-nDefaultPos -2) + " & ";
            else
                sTable += sParameter.substr(nDefaultPos+1, sParameter.find("!!)")+1-nDefaultPos) + " & ";
        }
        else
            sTable += "& ";

        sTable += sParameter.substr(nDescPos) + "\\\\\n";
    }

    // Append the table foot and return
    return sTable + "\t\\bottomrule\n\\end{tabular}\n";
}


/////////////////////////////////////////////////
/// \brief This member function creates a table
/// out of the declared return values in the
/// documentation comments.
///
/// \param vReturns const std::vector<std::string>&
/// \return std::string
///
/////////////////////////////////////////////////
std::string DocumentationGenerator::createReturnsTable(const std::vector<std::string>& vReturns) const
{
    // Table heads
    std::string sTable = "\\begin{tabular}{p{0.22\\textwidth}p{0.725\\textwidth}}\n\t\\toprule\n\tReturn type & Description\\\\\n\t\\midrule\n";
    std::string sReturnValue;

    size_t nDescPos;

    // Write every single parameter line
    for (size_t i = 0; i < vReturns.size(); i++)
    {
        sReturnValue = vReturns[i];
        replaceAll(sReturnValue, "\n", " ");

        nDescPos = sReturnValue.find_first_not_of(' ', sReturnValue.find(' ', 9));

        sTable += "\t!!" + sReturnValue.substr(8, sReturnValue.find(' ', 9) - 8) + "!! & ";

        if (nDescPos != std::string::npos)
            sTable += sReturnValue.substr(nDescPos) + "\\\\\n";
        else
            sTable += "[Return value description] \\\\\n";
    }

    // Append the table foot and return
    return sTable + "\t\\bottomrule\n\\end{tabular}\n";
}


/////////////////////////////////////////////////
/// \brief This helper function finds the
/// next list item in the passed documentation
/// string.
///
/// \param sTextRange const std::string&
/// \param nLength size_t&
/// \return size_t
///
/////////////////////////////////////////////////
size_t DocumentationGenerator::findListItem(const std::string& sTextRange, size_t& nLength) const
{
    size_t nItemCandidate = sTextRange.find("- ");

    if (nItemCandidate == std::string::npos)
        return nItemCandidate;

    size_t nItemStart = sTextRange.find_last_not_of("- ", nItemCandidate);

    if (nItemStart == std::string::npos || sTextRange[nItemStart] != '\n')
        return std::string::npos;

    nLength = nItemCandidate - nItemStart + 2;

    return nItemStart;
}


/////////////////////////////////////////////////
/// \brief This helper function finds the
/// next enum item in the passed documentation
/// string.
///
/// \param sTextRange const std::string&
/// \param nLength size_t&
/// \return size_t
///
/////////////////////////////////////////////////
size_t DocumentationGenerator::findEnumItem(const std::string& sTextRange, size_t& nLength) const
{
    std::smatch match;

    // Regex for WHITESPACES DIGITS . WHITESPACE(S) ANYCHARACTER
    static std::regex expr("( *\\d+\\. +)(?=\\S+)");

    if (std::regex_search(sTextRange, match, expr))
    {
        if (match.position(0) && sTextRange[match.position(0)-1] != '\n')
            return std::string::npos;

        nLength = match.length(0);
        return match.position(0);
    }

    return std::string::npos;
}


/////////////////////////////////////////////////
/// \brief This method converts the passed
/// filename into a main procedure for the
/// current file.
///
/// \param sFileName std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string DocumentationGenerator::createMainProcedure(std::string sFileName) const
{
    std::vector<std::string> vPaths = NumeReKernel::getInstance()->getPathSettings();

    if (sFileName.find(".nprc") == std::string::npos)
        return "";

    sFileName.erase(sFileName.rfind('.'));

    // Try to detect the corresponding namespace
    if (sFileName.starts_with(vPaths[PROCPATH]))
    {
        sFileName.erase(0, vPaths[PROCPATH].length());

        if (sFileName.front() == '/')
            sFileName.erase(0, 1);

        replaceAll(sFileName, "/", "~");
    }
    else
    {
        sFileName.erase(0, sFileName.rfind('/')+1);
        sFileName.insert(0, "this~");
    }

    return sFileName;
}


/////////////////////////////////////////////////
/// \brief This method will create a target
/// filename for the TeX files and create the
/// needed folder structure on-the-fly.
///
/// \param sFileName std::string
/// \return std::string
/// \remark The returned filename does not
/// contain any file extension.
///
/////////////////////////////////////////////////
std::string DocumentationGenerator::createLaTeXFileName(std::string sFileName) const
{
    std::vector<std::string> vPaths = NumeReKernel::getInstance()->getPathSettings();

    if (sFileName.starts_with(vPaths[PROCPATH]))
    {
        // This is one of the default folders
        sFileName.erase(0, vPaths[PROCPATH].length());
        sFileName = getPath() + "/procedures" + sFileName;
    }
    else if (sFileName.starts_with(vPaths[SCRIPTPATH]))
    {
        // This is one of the default folders
        sFileName.erase(0, vPaths[SCRIPTPATH].length());
        sFileName = getPath() + "/scripts" + sFileName;
    }
    else if (sFileName.starts_with(vPaths[EXEPATH]))
    {
        // This is one of the default folders
        sFileName.erase(0, vPaths[EXEPATH].length());
        sFileName = getPath() + sFileName;
    }
    else
    {
        // This is not in one of the default folders
        sFileName.erase(0, sFileName.rfind('/'));
        sFileName = getPath() + "/externals" + sFileName;
    }

    createFolders(sFileName.substr(0, sFileName.rfind('/')));
    sFileName[sFileName.rfind('.')] = '_';

    return sFileName;
}


/////////////////////////////////////////////////
/// \brief This member function removes the path
/// parts of the default paths and masks
/// underscores, which will confuse the LaTeX
/// parser.
///
/// \param sFileName std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string DocumentationGenerator::prepareFileNameForLaTeX(std::string sFileName) const
{
    std::vector<std::string> vPaths = NumeReKernel::getInstance()->getPathSettings();

    // Remove standard path parts
    for (size_t i = LOADPATH; i < PATH_LAST; i++)
    {
        if (sFileName.starts_with(vPaths[i]))
        {
            sFileName.erase(0, vPaths[i].length());

            if (sFileName.front() == '/')
                sFileName.erase(0, 1);

            break;
        }
    }

    // Mask underscores
    for (size_t i = 0; i < sFileName.length(); i++)
    {
        if (sFileName[i] == '_' && (!i || sFileName[i-1] != '\\'))
        {
            sFileName.insert(i, 1, '\\');
            i++;
        }
    }

    return sFileName;
}


/////////////////////////////////////////////////
/// \brief This member function replaces the
/// passed whitespace-separated keyword list with
/// a comma-separated list.
///
/// \param sKeyWordList std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string DocumentationGenerator::constructKeyWords(std::string sKeyWordList) const
{
    for (size_t i = 0; i < sKeyWordList.length(); i++)
    {
        if (sKeyWordList[i] == ' ')
            sKeyWordList[i] = ',';
    }

    if (sKeyWordList.back() == ',')
        sKeyWordList.erase(sKeyWordList.length()-1);

    return sKeyWordList;
}


/////////////////////////////////////////////////
/// \brief This member function writes the LaTeX
/// header file used to highlight the code
/// snippets, which are part of the created code
/// documentation. The contents of the header
/// file are read from the corresponding template
/// file in the "lang" folder.
///
/// \return void
///
/////////////////////////////////////////////////
void DocumentationGenerator::createStyleFile() const
{
    boost::nowide::ifstream fHeaderTemplate;

    // Open the header template
    fHeaderTemplate.open((NumeReKernel::getInstance()->getSettings().getExePath() + "/lang/tmpl_header.tex").c_str());

    // Ensure that the file exists
    if (!fHeaderTemplate.good())
        return;

    std::string sLine;
    std::string sTemplate;

    // Read all contents of the template into a
    // single string separated with line break
    // characters
    while (!fHeaderTemplate.eof())
    {
        std::getline(fHeaderTemplate, sLine);
        sTemplate += sLine + "\n";
    }

    // Replace the syntax list placeholders with
    // the current set of syntax elements
    size_t pos;

    if ((pos = sTemplate.find("NSCRCOMMANDS")) != std::string::npos)
        sTemplate.replace(pos, 12, constructKeyWords(m_syntax->getCommands() + m_syntax->getNPRCCommands()));

    if ((pos = sTemplate.find("NSCRFUNCTIONS")) != std::string::npos)
        sTemplate.replace(pos, 13, constructKeyWords(m_syntax->getFunctions()));

    if ((pos = sTemplate.find("NSCRCONSTANTS")) != std::string::npos)
        sTemplate.replace(pos, 13, constructKeyWords(m_syntax->getConstants()));

    if ((pos = sTemplate.find("NSCROPTIONS")) != std::string::npos)
        sTemplate.replace(pos, 11, constructKeyWords(m_syntax->getOptions()));

    if ((pos = sTemplate.find("NSCRMETHODS")) != std::string::npos)
        sTemplate.replace(pos, 11, constructKeyWords(m_syntax->getMethods()));

    if ((pos = sTemplate.find("NSCRSPECIALVALS")) != std::string::npos)
        sTemplate.replace(pos, 15, constructKeyWords(m_syntax->getSpecial()));

    // Now open the target header file
    boost::nowide::ofstream fHeader;
    fHeader.open((getPath() + "/numereheader.tex").c_str());

    // Ensure that the file is writable
    if (!fHeader.good())
        return;

    // Write the header file contents
    fHeader << sTemplate;
}


/////////////////////////////////////////////////
/// \brief This member function creates a main
/// LaTeX file including the perviously created
/// LaTeX documentation files.
///
/// \param sFileName const std::string&
/// \param vIncludesList const std::vector<std::string>&
/// \param vFiles const std::vector<std::string>&
/// \return std::string
///
/////////////////////////////////////////////////
std::string DocumentationGenerator::createMainFile(const std::string& sFileName, const std::vector<std::string>& vIncludesList, const std::vector<std::string>& vFiles) const
{
    boost::nowide::ofstream fMain;
    std::string sLaTeXMainFile = createLaTeXFileName(sFileName) + "_main.tex";

    fMain.open(sLaTeXMainFile.c_str());

    if (!fMain.good())
        return "";

    // Write the preamble
    fMain << "\\documentclass[DIV=17]{scrartcl}" << std::endl;
    fMain << "% Main file for the documentation file " << sFileName << std::endl << std::endl;
    fMain << "\\input{" << getPath() << "/numereheader}" << std::endl << std::endl;
    fMain << "\\ohead{" << prepareFileNameForLaTeX(sFileName) << "}" << std::endl;
    fMain << "\\ihead{Documentation}" << std::endl;
    fMain << "\\ifoot{NumeRe: Free numerical software}" << std::endl;
    fMain << "\\ofoot{Get it at: www.numere.org}" << std::endl;
    fMain << "\\pagestyle{scrheadings}" << std::endl;
    fMain << "\\subject{Documentation}" << std::endl;
    fMain << "\\title{" << prepareFileNameForLaTeX(sFileName) << "}" << std::endl;

    if (vFiles.size() > 1)
        fMain << "\\subtitle{And all called procedures}" << std::endl;

    fMain << "\\begin{document}" << std::endl;
    fMain << "    \\maketitle" << std::endl;

    // Create an overview, if necessary
    if (vFiles.size() > 1)
    {
        fMain << "    \\addsec{Files included in this documentation}\n    \\begin{itemize}" << std::endl;

        for (size_t i = 0; i < vFiles.size(); i++)
            fMain << "        \\item " << prepareFileNameForLaTeX(vFiles[i]) << std::endl;

        fMain << "    \\end{itemize}" << std::endl;
    }

    // Reference all created files
    for (size_t i = 0; i < vIncludesList.size(); i++)
        fMain << "    \\input{" << vIncludesList[i] << "}" << std::endl;

    fMain << "\\end{document}" << std::endl;

    return sLaTeXMainFile;
}


/////////////////////////////////////////////////
/// \brief DocumentationGenerator constructor.
///
/// \param _syntax NumeReSyntax*
/// \param sRootPath const std::string&
///
/////////////////////////////////////////////////
DocumentationGenerator::DocumentationGenerator(NumeReSyntax* _syntax, const std::string& sRootPath) : FileSystem()
{
    m_syntax = _syntax;
    setPath(sRootPath, true, NumeReKernel::getInstance()->getSettings().getExePath());
}


/////////////////////////////////////////////////
/// \brief This member function creates a LaTeX
/// documentation from a single file and stores
/// it. It's not compilable due to missing main
/// and style files.
///
/// \param sFileName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string DocumentationGenerator::createDocumentation(const std::string& sFileName) const
{
    if (sFileName.find(".nscr") == std::string::npos
        && sFileName.find(".nlyt") == std::string::npos
        && sFileName.find(".nprc") == std::string::npos)
        return "";

    return convertToLaTeX(replacePathSeparator(sFileName));
}


/////////////////////////////////////////////////
/// \brief This member function creates a LaTeX
/// documentation from the passed file and all
/// dependent files. All necessary files for
/// compilation are created.
///
/// \param sFileName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string DocumentationGenerator::createFullDocumentation(const std::string& sFileName) const
{
    if (sFileName.find(".nscr") == std::string::npos
        && sFileName.find(".nlyt") == std::string::npos
        && sFileName.find(".nprc") == std::string::npos)
        return "";

    std::set<std::string> fileSet;
    std::vector<std::string> vIncludesList;
    std::vector<std::string> vFiles;

    // Find all dependent files
    followBranch(replacePathSeparator(sFileName), fileSet, vFiles);

    // Create documentations for all files
    for (size_t i = 0; i < vFiles.size(); i++)
    {
        vIncludesList.push_back(convertToLaTeX(vFiles[i]));
    }

    // Create style and main files
    createStyleFile();
    return createMainFile(replacePathSeparator(sFileName), vIncludesList, vFiles);
}

