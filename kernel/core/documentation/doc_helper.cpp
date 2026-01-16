/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2016  Erik Haenel et al.

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
#include <boost/nowide/fstream.hpp>

#include "doc_helper.hpp"
#include "docfile.hpp"
#include "../utils/tools.hpp"
#include "../../kernel.hpp"
#include <algorithm>

using namespace std;


/////////////////////////////////////////////////
/// \brief Strips comments from the documentation
/// article file.
///
/// \param sLine std::string&
/// \param fDocument boost::nowide::ifstream&
/// \return void
///
/////////////////////////////////////////////////
static void stripComments(std::string& sLine, boost::nowide::ifstream& fDocument)
{
    while (sLine.find("<!--") != std::string::npos)
    {
        if (sLine.find("-->", sLine.find("<!--")+4) != std::string::npos)
            sLine.erase(sLine.find("<!--"), sLine.find("-->", sLine.find("<!--")+4)+3 - sLine.find("<!--"));
        else
        {
            sLine.erase(sLine.find("<!--"));
            std::string sLineTemp = "";

            while (!fDocument.eof())
            {
                std::getline(fDocument, sLineTemp);
                StripSpaces(sLineTemp);

                if (!sLineTemp.length())
                    continue;

                if (sLineTemp.find("-->") != std::string::npos)
                {
                    sLine += sLineTemp.substr(sLineTemp.find("-->")+3);
                    break;
                }
            }
        }
    }
}


/////////////////////////////////////////////////
/// \brief Searches an article in a file by the
/// selected ID.
///
/// \param sLine std::string&
/// \param sArticleID const std::string&
/// \param fDocument boost::nowide::ifstream&
/// \return void
///
/////////////////////////////////////////////////
static void findArticleById(std::string& sLine, const std::string& sArticleID, boost::nowide::ifstream& fDocument)
{
    while (sLine.find("<article ") != std::string::npos && !fDocument.eof())
    {
        // Does the article contain the necessary ID?
        if (Documentation::getArgAtPos(sLine, sLine.find("id=")+3) != sArticleID)
        {
            while (!fDocument.eof())
            {
                std::getline(fDocument, sLine);
                StripSpaces(sLine);

                // Remove comments
                if (sLine.find("<!--") != std::string::npos)
                    stripComments(sLine, fDocument);

                if (!sLine.length())
                    continue;

                // The next article starts
                if (sLine.find("<article") != std::string::npos)
                    break;
            }
        }
        else
        {
            sLine.erase(0, sLine.find('>', sLine.find("<article "))+1);
            StripSpaces(sLine);
            return;
        }
    }
}


/////////////////////////////////////////////////
/// \brief This static function loads and
/// prepares the selected documentation article.
///
/// \param sFileName const std::string&
/// \param sArticleID const std::string&
/// \return vector<string>
///
/////////////////////////////////////////////////
static std::vector<std::string> loadDocumentationArticle(const std::string& sFileName, const std::string& sArticleID)
{
    boost::nowide::ifstream fDocument;
    fDocument.open(sFileName.c_str(), std::ios_base::in);

    if (fDocument.fail())
    {
        fDocument.close();
        throw SyntaxError(SyntaxError::HLP_FILE_MISSING, "", SyntaxError::invalid_position, sFileName);
    }

    std::string sLine;
    std::vector<std::string> vReturn;

    while (!fDocument.eof())
    {
        std::getline(fDocument, sLine);
        StripSpaces(sLine);

        // Remove comments
        if (sLine.find("<!--") != std::string::npos)
            stripComments(sLine, fDocument);

        if (!sLine.length())
            continue;

        // Find the selected article
        if (sLine.find("<article ") != std::string::npos)
        {
            findArticleById(sLine, sArticleID, fDocument);

            if (fDocument.eof())
            {
                vReturn.push_back("NO_ENTRY_FOUND");
                return vReturn;
            }

            if (!sLine.length())
                continue;
        }

        // Extract the title
        if (sLine.find("<title ") != std::string::npos)
        {
            vReturn.push_back(getArgAtPos(sLine, sLine.find("string=", sLine.find("<title "))+7));
            sLine.erase(0, sLine.find("/>", sLine.find("<title "))+2);
            StripSpaces(sLine);

            if (!sLine.length())
                continue;
        }

        // Extract the contents
        if (sLine.find("<contents>") != std::string::npos)
        {
            sLine.erase(0, sLine.find("<contents>")+10);

            if (sLine.length())
                vReturn.push_back(sLine);

            while (!fDocument.eof())
            {
                std::getline(fDocument, sLine);
                StripSpaces(sLine);

                if (!sLine.length())
                    continue;

                if (sLine.find("<!--") != std::string::npos)
                    stripComments(sLine, fDocument);

                if (sLine.find("</contents>") != std::string::npos)
                {
                    sLine.erase(sLine.find("</contents>"));

                    if (sLine.length())
                        vReturn.push_back(sLine);

                    return vReturn;
                }

                vReturn.push_back(sLine);
            }
        }
    }

    fDocument.close();

    if (!vReturn.size())
        vReturn.push_back("NO_ENTRY_FOUND");

    return vReturn;
}







/////////////////////////////////////////////////
/// \brief The default constructor
/////////////////////////////////////////////////
Documentation::Documentation() : FileSystem()
{
    vDocIndexTable.reserve(128);
}


/////////////////////////////////////////////////
/// \brief The destructor closes the internal
/// file stream, if it is still open.
/////////////////////////////////////////////////
Documentation::~Documentation()
{
}


/////////////////////////////////////////////////
/// \brief Add a new entry to the documentation
/// index respecting already existing versions of
/// the new entry. Older versions will be
/// overwritten.
///
/// \param entry const DocumentationEntry&
/// \param keyWords const std::vector<std::string>&
/// \return void
///
/////////////////////////////////////////////////
void Documentation::addEntry(const DocumentationEntry& entry, const std::vector<std::string>& keyWords)
{
    EndlessVector<std::string> keys = getAllArguments(entry.sIdxKeys);
    int nIndex = findPositionUsingIdxKeys(entry.sIdxKeys);

    // No entry found?
    if (nIndex == -1)
    {
        nIndex = vDocIndexTable.size();
        vDocIndexTable.push_back(entry);

        // Insert all keys
        for (const std::string& key : keys)
            mDocumentationIndex[key] = nIndex;
    }
    else
    {
        vDocIndexTable[nIndex] = entry;

        // synchronize all keys
        for (const std::string& key : keys)
            mDocumentationIndex[key] = nIndex;
    }

    // Insert the keywords to the map and use the
    // identified index as reference
    for (const std::string& kw : keyWords)
        mDocumentationIndex[kw] = nIndex;
}


/////////////////////////////////////////////////
/// \brief This private member function returns
/// the position of the queried topic in the
/// documentation index table.
///
/// \param sTopic const std::string&
/// \return int
///
/////////////////////////////////////////////////
int Documentation::findPositionInDocumentationIndex(const std::string& sTopic) const
{
    // Is this a UUID?
    if (sTopic.length() == 38 && sTopic.front() == '{' && sTopic.back() == '}')
    {
        // Search by UUID
        for (size_t i = 0; i < vDocIndexTable.size(); i++)
        {
            if (vDocIndexTable[i].sUUID == sTopic)
                return i;
        }

        return -1;
    }

    int nIndex = -1;
    auto iter = mDocumentationIndex.begin();
    auto firstIndex = mDocumentationIndex.end();
    auto secondIndex = mDocumentationIndex.end();

    for (int i = sTopic.length(); i > 0; i--)
    {
        if (firstIndex != mDocumentationIndex.end())
            iter = firstIndex;
        else
            iter = mDocumentationIndex.begin();

        for (; iter != secondIndex; ++iter)
        {
            if (iter->first[0] < sTopic[0])
                continue;

            if (iter->first[0] == sTopic[0] && firstIndex == mDocumentationIndex.end())
                firstIndex = iter;

            if (iter->first[0] > sTopic[0])
            {
                secondIndex = iter;
                break;
            }

            if (iter->first == sTopic.substr(0,i))
            {
                nIndex = iter->second;
                break;
            }
        }

        if (nIndex != -1)
            break;
    }

    return nIndex;
}


/////////////////////////////////////////////////
/// \brief Finds the best match of a
/// documentation entry using a list of index
/// keys. If multiple matches have been found,
/// the entry with the most matches will be
/// returned. If nothing is found, the function
/// will return -1.
///
/// \param sIdxKeys const std::string&
/// \return int
///
/////////////////////////////////////////////////
int Documentation::findPositionUsingIdxKeys(const std::string& sIdxKeys) const
{
    EndlessVector<std::string> keys = getAllArguments(sIdxKeys);
    int nIndex = -1;

    // is this item already known? (user lang file)
    std::map<int,size_t> keyCount;
    for (const std::string& key : keys)
    {
        if (mDocumentationIndex.find(key) != mDocumentationIndex.end())
        {
            keyCount[mDocumentationIndex.at(key)]++;
        }
    }

    // Find the entry with most key matches
    if (keyCount.size())
    {
        auto el = std::max_element(keyCount.begin(), keyCount.end(),
                                   [](const pair<int,size_t>& p1, const pair<int,size_t>& p2) { return p1.second < p2.second; });
        nIndex = el->first;
    }

    return nIndex;
}


/////////////////////////////////////////////////
/// \brief Make the candidate string to be shown
/// if candidates are required.
///
/// \param id int
/// \return std::string
///
/////////////////////////////////////////////////
std::string Documentation::makeCandidate(int id) const
{
    std::vector<std::string> vDoc = getHelpArticle(vDocIndexTable[id].sUUID);
    size_t extract = 1;

    if (vDoc[extract] == "<syntax>")
        extract = std::find(vDoc.begin(), vDoc.end(), "</syntax>")+1-vDoc.begin();

    return "<item node=\"*\"><a href=\"nhlp://" + vDocIndexTable[id].sUUID + "?type=doc\">" + vDocIndexTable[id].sTitle + "</a>: " + vDoc[extract] + "</item>";
}


/////////////////////////////////////////////////
/// \brief This static member is a fallback for
/// the XML-parsing logic-stuff.
///
/// \param sCmd const std::string&
/// \param nPos size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string Documentation::getArgAtPos(const std::string& sCmd, size_t nPos)
{
    std::string sArgument = "";

    // If the position is greater than the string length
    // return an empty string
    if (nPos >= sCmd.length())
        return "";

    // Jump over whitespaces
    while (nPos < sCmd.length() && sCmd[nPos] == ' ')
        nPos++;

    // Ensure that the position is smaller than the length of the string
    if (nPos >= sCmd.length())
        return "";

    // Extract the option value
    // Determine the delimiter first
    if (sCmd[nPos] == '"')
    {
        // This option value is surrounded with quotation marks
        // Go through the string and find the next quotation
        // mark, which is not escaped by a backslash
        for (size_t i = nPos + 1; i < sCmd.length(); i++)
        {
            if (isQuotationMark(sCmd, i))
            {
                sArgument = sCmd.substr(nPos+1, i - nPos - 1);
                break;
            }
        }
    }
    else
    {
        // This option value is not surrounded with quotation marks
        // Go through the string and find the next whitespace
        for (size_t i = nPos; i < sCmd.length(); i++)
        {
            // Jump over parentheses, if you find one
            if (sCmd[i] == '(' || sCmd[i] == '[' || sCmd[i] == '{')
                i += getMatchingParenthesis(StringView(sCmd, i));

            // Whitespace. Stop the loop here
            if (sCmd[i] == ' ')
            {
                sArgument = sCmd.substr(nPos, i - nPos);
                StripSpaces(sArgument);
                break;
            }
        }

        // Special case: obviously there's no whitespace any more
        // simply use the remaining string completely
        if (!sArgument.length())
        {
            sArgument = sCmd.substr(nPos);
            StripSpaces(sArgument);
        }
    }

    // return the found option value
    return sArgument;
}


/////////////////////////////////////////////////
/// \brief This member function loads the index
/// files to memory.
///
/// \param bLoadUserLangFiles bool
/// \return void
///
/////////////////////////////////////////////////
void Documentation::createDocumentationIndex(bool bLoadUserLangFiles)
{
    // Add standard documentation files
    std::vector<std::string> vFiles = getFileList(ValidFolderName("<>") + "docs/*.nhlp", FileSystem::FULLPATH);

    for (const std::string& file : vFiles)
    {
        addFileToDocumentationIndex(file);
    }

    // Add plugin documentation files
    vFiles = getFileList(ValidFolderName("<>") + "docs/plugins/*.nhlp", FileSystem::FULLPATH);

    for (const std::string& file : vFiles)
    {
        addFileToDocumentationIndex(file);
    }

    if (!bLoadUserLangFiles)
        return;

    // Add user documentation files
    vFiles = getFileList(ValidFolderName("<>") + "user/docs/*.nhlp", FileSystem::FULLPATH);

    for (const std::string& file : vFiles)
    {
        addFileToDocumentationIndex(file);
    }
}


/////////////////////////////////////////////////
/// \brief This member function is used to add
/// documentation index entries to the index
/// during a plugin or package installation.
///
/// \param sFileName const std::string&
/// \return void
///
/////////////////////////////////////////////////
void Documentation::addFileToDocumentationIndex(const std::string& sFileName)
{
    if (!sFileName.length())
        return;

    try
    {
        DocumentationFile docFile(sFileName);

        // Parse the file and add it to the documentation
        // index
        for (DocumentationArticle& article : docFile.getArticles())
        {
            if (article.m_keywords.size())
                addEntry(article.m_docEntry, article.m_keywords);
        }
    }
    catch (SyntaxError& e)
    {
        // Catch all to avoid issues during start-up
        g_logger.error("Error while trying to import '" + sFileName + "'. Error code: "
                       + toString((size_t)e.errorcode) + " " + e.getExpr());
    }
    catch (...)
    {
        // Catch all to avoid issues during start-up
        g_logger.error("Error while trying to import '" + sFileName + "'.");
    }
}


/////////////////////////////////////////////////
/// \brief This member function removes the index
/// entry passed to the function from the
/// documentation index.
///
/// \param _sID const string&
/// \return void
///
/////////////////////////////////////////////////
void Documentation::removeFromDocIndex(const string& _sID)
{
    if (!vDocIndexTable.size())
        throw SyntaxError(SyntaxError::INVALID_HLPIDX, "", SyntaxError::invalid_position);

    if (_sID == "<<NO_HLP_ENTRY>>")
        return;

    // Search the documentation ID in the documentation
    // index table
    for (size_t i = 0; i < vDocIndexTable.size(); i++)
    {
        if (vDocIndexTable[i].sArticleId == _sID)
        {
            // Search the keywords corresponding to the
            // position in the documentation index table
            for (auto iter = mDocumentationIndex.begin(); iter != mDocumentationIndex.end(); ++iter)
            {
                if (iter->second == (int)i)
                {
                    auto curiter = iter;
                    --iter;
                    mDocumentationIndex.erase(curiter);
                }
                else if (iter->second > (int)i)
                {
                    // Reassign the correct postion in the
                    // table to the following entries
                    (iter->second)--;
                }
            }

            remove(vDocIndexTable[i].sDocFilePath.c_str());
            vDocIndexTable.erase(vDocIndexTable.begin()+i);
            break;
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// documentation article, which corresponds to
/// the passed documentation topic of the the
/// whole documentation index in readable form.
///
/// \param sTopic const std::string&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> Documentation::getHelpArticle(const std::string& sTopic) const
{
    std::vector<std::string> vReturn;

    if (!vDocIndexTable.size())
        throw SyntaxError(SyntaxError::INVALID_HLPIDX, "", SyntaxError::invalid_position);

    // Return the documentation article or
    // the whole index
    if (sTopic != "idx" && sTopic != "index")
    {
        // Find the documentation article
        int nIndex = findPositionInDocumentationIndex(sTopic);

        if (nIndex != -1)
        {
            if (vDocIndexTable[nIndex].sDocFilePath.find("<>") != std::string::npos)
                vDocIndexTable[nIndex].sDocFilePath = FileSystem::ValidFileName(vDocIndexTable[nIndex].sDocFilePath, ".nhlp");

            return loadDocumentationArticle(vDocIndexTable[nIndex].sDocFilePath, vDocIndexTable[nIndex].sArticleId);
        }
        else
            vReturn.push_back("NO_ENTRY_FOUND");
    }
    else
    {
        string sKeyList;
        vReturn.push_back("Index");
        std::map<std::string,std::string> mIdx;

        for (size_t i = 0; i < vDocIndexTable.size(); i++)
        {
            EndlessVector<std::string> keylist = getAllArguments(vDocIndexTable[i].sIdxKeys);

            for (const std::string& key : keylist)
            {
                mIdx[key] = vDocIndexTable[i].sTitle;
            }
        }

        vReturn.push_back(_lang.get("DOCHELPER_KEYWORDS_AND_ARTICLES")+ ":");
        vReturn.push_back("<list>");

        for (auto iter = mIdx.begin(); iter != mIdx.end(); ++iter)
        {
            vReturn.push_back("<item node=\"" + iter->first + "\">" + iter->second + "</item>");
        }

        vReturn.push_back("</list>");
        vReturn.push_back(_lang.get("DOCHELPER_KEYWORDS_AND_ARTICLES_NUMBERS", toString((int)vReturn.size()-4), toString((int)vDocIndexTable.size())));
    }

    return vReturn;
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// key list for the documentation index prepared
/// for the corresponding tree in the
/// documentation browser.
///
/// \return std::vector<DocIndexEntry>
///
/////////////////////////////////////////////////
std::vector<DocIndexEntry> Documentation::getDocIndex() const
{
    std::vector<DocIndexEntry> vReturn;

    // Go through the index table and extract the
    // key list
    for (size_t i = 0; i < vDocIndexTable.size(); i++)
    {
        std::string sKeyWords;

        // Connect all keywords
        for (const auto& iter : mDocumentationIndex)
        {
            if (iter.second == (int)i)
            {
                if (sKeyWords.length())
                    sKeyWords += " ";

                sKeyWords += iter.first;
            }
        }

        EndlessVector<std::string> keylist = getAllArguments(vDocIndexTable[i].sIdxKeys);
        DocIndexEntry entry;
        entry.sUUID = vDocIndexTable[i].sUUID;
        entry.sSecondaryKeyWords = sKeyWords;

        for (const std::string& key : keylist)
        {
            entry.sKey = key;

            // Append the keywords to the key
            if (std::find(vReturn.begin(), vReturn.end(), entry) == vReturn.end())
                vReturn.push_back(entry);
        }
    }

    std::sort(vReturn.begin(), vReturn.end());

    return vReturn;
}


/////////////////////////////////////////////////
/// \brief This member function returns an index
/// key, which corresponds to the queried topic.
///
/// \param sTopic const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string Documentation::getHelpIdxKey(const std::string& sTopic) const
{
    int nIndex = findPositionInDocumentationIndex(sTopic);
    std::string sReturn = "";

    if (nIndex == -1)
        return "<<NONE>>";

    sReturn = vDocIndexTable[nIndex].sIdxKeys;

    if (sReturn.find(',') != std::string::npos)
        sReturn.erase(sReturn.find(','));

    return sReturn;
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// article ID corresponding to the queried topic.
///
/// \param sTopic const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string Documentation::getHelpArticleID(const std::string& sTopic) const
{
    int nIndex = findPositionInDocumentationIndex(sTopic);

    if (nIndex != -1)
        return vDocIndexTable[nIndex].sArticleId;

    return "NO_ENTRY_FOUND";
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// documentation article title corresponding to
/// the queried index key.
///
/// \param _sIdxKey const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string Documentation::getHelpArticleTitle(const std::string& _sIdxKey) const
{
    int nIndex = findPositionInDocumentationIndex(_sIdxKey);

    if (nIndex == -1)
        return "<<NONE>>";

    return vDocIndexTable[nIndex].sTitle;
}


/////////////////////////////////////////////////
/// \brief Create a list of possible matches
/// based upon the selected topic.
///
/// \param sTopic const std::string&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> Documentation::getCandidates(const std::string& sTopic) const
{
    // Is this a UUID?
    if (sTopic.length() == 38 && sTopic.front() == '{' && sTopic.back() == '}')
    {
        // Search by UUID
        for (size_t i = 0; i < vDocIndexTable.size(); i++)
        {
            if (vDocIndexTable[i].sUUID == sTopic)
                return {makeCandidate(i)};
        }

        return {};
    }

    std::vector<std::string> candidates;
    auto iter = mDocumentationIndex.begin();
    auto firstIndex = mDocumentationIndex.end();
    auto secondIndex = mDocumentationIndex.end();

    for (int i = sTopic.length(); i > sTopic.length() / 2; i--)
    {
        if (firstIndex != mDocumentationIndex.end())
            iter = firstIndex;
        else
            iter = mDocumentationIndex.begin();

        for (; iter != secondIndex; ++iter)
        {
            if (iter->first[0] < sTopic[0])
                continue;

            if (iter->first[0] == sTopic[0] && firstIndex == mDocumentationIndex.end())
                firstIndex = iter;

            if (iter->first[0] > sTopic[0])
            {
                secondIndex = iter;
                break;
            }

            if (iter->first == sTopic.substr(0, i))
                candidates.push_back(makeCandidate(iter->second));
        }
    }

    return candidates;
}

