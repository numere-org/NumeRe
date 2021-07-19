/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2014  Erik Haenel et al.

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

// CLASS: DOC_HELPER

#ifndef DOC_HELPER_HPP
#define DOC_HELPER_HPP

#include <string>
#include <vector>
#include <map>

#include "../io/filesystem.hpp"
#include "../ui/error.hpp"

void StripSpaces(std::string&);
std::string toUpperCase(const std::string&);
std::string toString(int);

struct DocumentationEntry;

/////////////////////////////////////////////////
/// \brief This class handles the documentation
/// index and provides lookup functionalities to
/// obtain the corresponding article.
/////////////////////////////////////////////////
class Documentation : public FileSystem
{
    private:
        std::map<std::string,int> mDocumentationIndex;
        std::vector<DocumentationEntry> vDocIndexTable;

        void addEntry(const DocumentationEntry& entry, const std::vector<std::string>& keyWords);
        int findPositionInDocumentationIndex(const std::string& sTopic) const;
        int findPositionUsingIdxKeys(const std::string& sIdxKeys) const;

    public:
        Documentation();
        ~Documentation();

        void createDocumentationIndex(bool bLoadUserLangFiles = true);
        void addFileToDocumentationIndex(const std::string& sFileName);
        void removeFromDocIndex(const std::string& _sID);
        std::vector<std::string> getHelpArticle(const std::string& sTopic);
        std::vector<std::string> getDocIndex() const;
        std::string getHelpIdxKey(const std::string& sTopic);
        std::string getHelpArticleID(const std::string& sTopic);
        std::string getHelpArticleTitle(const std::string& _sIdxKey);
        static std::string getArgAtPos(const std::string& sCmd, unsigned int pos);
};

#endif
