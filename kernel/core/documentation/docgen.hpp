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


#ifndef DOCGEN_HPP
#define DOCGEN_HPP

#include <string>
#include <set>
#include <vector>
#include <utility>

#include "../io/filesystem.hpp"
#include "../io/styledtextfile.hpp"
#include "../../syntax.hpp"

/////////////////////////////////////////////////
/// \brief This class handles the creation of
/// LaTeX documentation from code files. It will
/// detect documentation keywords, but the
/// resulting files will not be compiled
/// automatically. This is in the responsibility
/// of the calling function.
/////////////////////////////////////////////////
class DocumentationGenerator : public FileSystem
{
    private:
        NumeReSyntax* m_syntax;

        void followBranch(const std::string& sFile, std::set<std::string>& fileSet, std::vector<std::string>& vFiles) const;

        std::string convertToLaTeX(const std::string& sFileName) const;
        std::string getStrippedRange(const StyledTextFile& file, int pos1, int pos2, bool encode = true) const;
        std::string parseDocumentation(const StyledTextFile& file, const std::string& sMainProc, int pos1, int pos2) const;
        std::string createParametersTable(const std::vector<std::string>& vParams) const;
        size_t findListItem(const std::string& sTextRange, size_t& nLength) const;

        std::string createMainProcedure(std::string sFileName) const;
        std::string createLaTeXFileName(std::string sFileName) const;
        std::string prepareFileNameForLaTeX(std::string sFileName) const;
        std::string constructKeyWords(std::string sKeyWordList) const;
        void createStyleFile() const;
        std::string createMainFile(const std::string& sFileName, const std::vector<std::string>& vIncludesList, const std::vector<std::string>& vFiles) const;

    public:
        DocumentationGenerator(NumeReSyntax* _syntax, const std::string& sRootPath);

        std::string createDocumentation(const std::string& sFileName) const;
        std::string createFullDocumentation(const std::string& sFileName) const;
};


#endif // DOCGEN_HPP


