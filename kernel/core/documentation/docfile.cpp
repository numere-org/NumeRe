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

#include <fstream>

#include "docfile.hpp"
#include "../../../externals/tinyxml2/tinyxml2.h"
#include "../utils/tools.hpp" // for stripSpaces()


/////////////////////////////////////////////////
/// \brief Parse the documentation article header
/// into the DocumentationEntry structure and the
/// list of keywords associated with this article.
///
/// \param sCombinedArticle const std::string&
/// \param docEntry DocumentationEntry&
/// \param keyWords std::vector<std::string>&
/// \return void
///
/////////////////////////////////////////////////
static void parseHeader(const std::string& sCombinedArticle, DocumentationEntry& docEntry, std::vector<std::string>& keyWords)
{
    // Parse the whole string as XML
    tinyxml2::XMLDocument doc;
    tinyxml2::XMLError e = doc.Parse(sCombinedArticle.c_str(), sCombinedArticle.length());

    if (e != tinyxml2::XML_SUCCESS)
        throw SyntaxError(SyntaxError::HLPIDX_ENTRY_IS_MISSING, doc.ErrorStr(), SyntaxError::invalid_position);

    // Try to find the article section
    // (has to be available at this location)
    tinyxml2::XMLElement* article = doc.FirstChildElement("article");

    // Get ID
    if (article->Attribute("id"))
        docEntry.sArticleId = article->Attribute("id");

    // Add a random-based {UUID} for uniqueness. The article ID might be shared.
    docEntry.sUUID = "{" + getUuidV4() + "}";

    tinyxml2::XMLElement* iter = article->FirstChildElement("title");

    // Get title and IDX keys
    if (iter && iter->Attribute("string"))
        docEntry.sTitle = iter->Attribute("string");

    if (iter && iter->Attribute("idxkey"))
        docEntry.sIdxKeys = iter->Attribute("idxkey");

    // Get the keywords sections
    iter = article->FirstChildElement("keywords");

    // Parse keywords
    if (iter)
    {
        iter = iter->FirstChildElement("keyword");

        while (iter)
        {
            keyWords.push_back(iter->GetText());
            iter = iter->NextSiblingElement();
        }
    }
}


/////////////////////////////////////////////////
/// \brief Create a DocumentationArticle
/// structure from the selected part of the
/// documentation file containing exactly one
/// article.
///
/// \param vArticleContents const std::vector<std::string>&
/// \param sFileName const std::string&
///
/////////////////////////////////////////////////
DocumentationArticle::DocumentationArticle(const std::vector<std::string>& vArticleContents, const std::string& sFileName)
{
    std::string sCombinedArticle;

    for (const auto& text : vArticleContents)
    {
        sCombinedArticle += text;
    }

    // Only complete sets with both <article> and </article>
    // will enter this function
    sCombinedArticle.erase(0, sCombinedArticle.find("<article "));
    sCombinedArticle.erase(sCombinedArticle.find("</article>")+10);

    // Parse the documentation article header information
    parseHeader(sCombinedArticle, m_docEntry, m_keywords);

    m_docEntry.sDocFilePath = sFileName;

    // Extract the contents section of the documentation article
    // into the internal vector
    for (size_t i = 0; i < vArticleContents.size(); i++)
    {
        if (vArticleContents[i].find("<contents>") != std::string::npos)
        {
            if (vArticleContents[i].substr(vArticleContents[i].find("<contents>")+10).length())
                m_contents.push_back(vArticleContents[i].substr(vArticleContents[i].find("<contents>")+10));

            for (size_t j = i+1; j < vArticleContents.size(); j++)
            {
                if (vArticleContents[j].find("</contents>") != std::string::npos)
                {
                    if (vArticleContents[j].find("</contents>"))
                        m_contents.push_back(vArticleContents[j].substr(0, vArticleContents[j].find("</contents>")));

                    break;
                }

                m_contents.push_back(vArticleContents[j]);
            }

            break;
        }
    }

}


/////////////////////////////////////////////////
/// \brief Calculate the indentation level
/// difference from the passed line.
///
/// \param sText const std::string&
/// \return int
///
/////////////////////////////////////////////////
static int indentationLevelDiff(const std::string& sText)
{
    int diff = 0;
    std::vector<std::string> vTagStack;

    // Go through the line
    for (size_t i = 0; i < sText.length(); i++)
    {
        // Examine all tags
        if (sText[i] == '<')
        {
            std::string tag = sText.substr(i, sText.find('>', i) + 1 - i);
            std::string tagVal = tag.substr(tag.find_first_not_of("</ "));
            tagVal.erase(tagVal.find_first_of(" />"));

            // Ignore self-terminating tags
            if (tag.find("/>") != std::string::npos)
                continue;

            // Is it an opening or closing tag?
            if (tag.starts_with("</"))
            {
                // Do only decrement matched pairs
                if (vTagStack.size() && tagVal == vTagStack.back())
                {
                    vTagStack.pop_back();
                    diff--;
                }
                else if (!vTagStack.size()) // may be matched against a previous line
                    diff--;
                //do not decrement if unmatched
            }
            else
            {
                // Always increment
                vTagStack.push_back(tagVal);
                diff++;
            }
        }
    }

    return diff;
}


/////////////////////////////////////////////////
/// \brief Format the documentation article into
/// a single string, which then can be printed to
/// a file.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string DocumentationArticle::format()
{
    // Format the header
    std::string sFormat = "<article id=\"" + m_docEntry.sArticleId + "\" >\n\t<title string=\"" + ansiToUtf8(m_docEntry.sTitle) + "\" idxkey=\"" + m_docEntry.sIdxKeys + "\" />\n\t<keywords>\n";

    // Add the keywords
    for (const auto& keyw : m_keywords)
    {
        sFormat += "\t\t<keyword>" + keyw + "</keyword>\n";
    }

    // Terminate the header and start the
    // contents section
    sFormat += "\t</keywords>\n\t<contents>\n";

    int indentationLevel = 0;

    // Add the contents
    for (const auto& text : m_contents)
    {
        int indLevelDiff = indentationLevelDiff(text);

        if (indLevelDiff < 0)
            indentationLevel += indLevelDiff;

        // Not below zero!
        indentationLevel = std::max(0, indentationLevel);

        sFormat.append(indentationLevel+2, '\t');
        sFormat += ansiToUtf8(text) + "\n";

        if (indLevelDiff > 0)
            indentationLevel += indLevelDiff;
    }

    return sFormat + "\t</contents>\n</article>";
}





/////////////////////////////////////////////////
/// \brief Create a DocumentationFile from the an
/// actual file.
///
/// \param sFileName const std::string&
///
/////////////////////////////////////////////////
DocumentationFile::DocumentationFile(const std::string& sFileName)
{
    parse(loadFromFile(sFileName), sFileName);
}


/////////////////////////////////////////////////
/// \brief Create a DocumentationFile from the
/// passed file contents.
///
/// \param vFileContents const std::vector<std::string>&
///
/////////////////////////////////////////////////
DocumentationFile::DocumentationFile(const std::vector<std::string>& vFileContents)
{
    parse(vFileContents, "");
}


/////////////////////////////////////////////////
/// \brief Load the documentation file contents
/// from the specified file.
///
/// \param sFileName const std::string&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> DocumentationFile::loadFromFile(const std::string& sFileName)
{
    std::vector<std::string> vFileContents;
    std::ifstream file(sFileName);

    if (!file.good())
        throw SyntaxError(SyntaxError::HLP_FILE_MISSING, "", SyntaxError::invalid_position, sFileName);

    while (!file.eof())
    {
        vFileContents.push_back("");
        std::getline(file, vFileContents.back());
        StripSpaces(vFileContents.back());
    }

    return vFileContents;
}


/////////////////////////////////////////////////
/// \brief Parse the documentation file into
/// single articles. Only complete
/// <article>..</article> sections are parsed.
///
/// \param vFileContents const std::vector<std::string>&
/// \param sFileName const std::string&
/// \return void
///
/////////////////////////////////////////////////
void DocumentationFile::parse(const std::vector<std::string>& vFileContents, const std::string& sFileName)
{
    for (size_t i = 0; i < vFileContents.size(); i++)
    {
        // Start of an article section
        if (vFileContents[i].find("<article ") != std::string::npos)
        {
            // Search for its end
            for (size_t j = i; j < vFileContents.size(); j++)
            {
                if (vFileContents[j].find("</article>") != std::string::npos
                    && (j > i || vFileContents[i].find("<article ") < vFileContents[j].find("</article>")))
                {
                    m_articles.push_back(DocumentationArticle(std::vector<std::string>(vFileContents.begin()+i, vFileContents.begin()+j+1), sFileName));

                    if (j > i)
                        i = j-1;
                }
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief Print the documentation file to the
/// specified file.
///
/// \param sFileName const std::string&
/// \return void
///
/////////////////////////////////////////////////
void DocumentationFile::print(const std::string& sFileName)
{
    std::ofstream file(sFileName, std::ios_base::out | std::ios_base::trunc);

    if (!file.good())
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFileName, SyntaxError::invalid_position, sFileName);

    for (auto& article : m_articles)
    {
        file << article.format() << std::endl;
    }
}


