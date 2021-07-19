/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2021  Erik Haenel et al.

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

#ifndef DOCFILE_HPP
#define DOCFILE_HPP

#include <string>
#include <vector>

/////////////////////////////////////////////////
/// \brief This structure defines a single entry
/// in the documentation index.
/////////////////////////////////////////////////
struct DocumentationEntry
{
    std::string sArticleId;
    std::string sDocFilePath;
    std::string sTitle;
    std::string sIdxKeys;
};



/////////////////////////////////////////////////
/// \brief This structure defines a single
/// article in a documentation file.
/////////////////////////////////////////////////
struct DocumentationArticle
{
    DocumentationEntry m_docEntry;
    std::vector<std::string> m_keywords;
    std::vector<std::string> m_contents;

    DocumentationArticle(const std::vector<std::string>& vArticleContents, const std::string& sFileName);
    std::string format();
};



/////////////////////////////////////////////////
/// \brief This class represents a whole
/// documentation file with its contained
/// documentation articles.
/////////////////////////////////////////////////
class DocumentationFile
{
    private:
        std::vector<DocumentationArticle> m_articles;

        std::vector<std::string> loadFromFile(const std::string& sFileName);
        void parse(const std::vector<std::string>& vFileContents, const std::string& sFileName);

    public:
        DocumentationFile(const std::string& sFileName);
        DocumentationFile(const std::vector<std::string>& vFileContents);

        void print(const std::string& sFileName);

        /////////////////////////////////////////////////
        /// \brief Get all articles present in the
        /// current documentation file.
        ///
        /// \return std::vector<DocumentationArticle>&
        ///
        /////////////////////////////////////////////////
        std::vector<DocumentationArticle>& getArticles()
        {
            return m_articles;
        }

};

#endif // DOCFILE_HPP



