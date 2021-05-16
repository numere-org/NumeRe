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

#include "doc_helper.hpp"
#include "../version.h"
#include "../utils/tools.hpp"
#include "../../kernel.hpp"
#include <algorithm>

bool fileExists(const string&);

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
/// \brief The default constructor
/////////////////////////////////////////////////
Documentation::Documentation() : FileSystem()
{
    vDocIndexTable.reserve(128);
    sDocIndexFile = "<>/numere.hlpidx";
}


/////////////////////////////////////////////////
/// \brief The destructor closes the internal
/// file stream, if it is still open.
/////////////////////////////////////////////////
Documentation::~Documentation()
{
    if (fDocument.is_open())
        fDocument.close();
}


/////////////////////////////////////////////////
/// \brief Add a new entry to the documentation
/// index respecting already existing versions of
/// the new entry. Older versions will be
/// overwritten.
///
/// \param entry const DocumentationEntry&
/// \param sKeyWords std::string
/// \return void
///
/////////////////////////////////////////////////
void Documentation::addEntry(const DocumentationEntry& entry, std::string sKeyWords)
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
    while (sKeyWords.find("<keyword>") != string::npos)
    {
        std::string sKeyWord = sKeyWords.substr(sKeyWords.find('>')+1, sKeyWords.find("</keyword>")-1-sKeyWords.find('>'));
        bool bDelete = sKeyWords.substr(0, sKeyWords.find('>')).find("delete=\"true\"") != string::npos;
        sKeyWords.erase(0, sKeyWords.find("</keyword>")+10);

        EndlessVector<std::string> keywords = getAllArguments(sKeyWord);

        for (const std::string& keyword : keywords)
        {
            if (bDelete && mDocumentationIndex.find(keyword) != mDocumentationIndex.end())
                mDocumentationIndex.erase(mDocumentationIndex.find(keyword));
            else
                mDocumentationIndex[keyword] = nIndex;
        }
    }
}


/////////////////////////////////////////////////
/// \brief This private member function updates
/// the documentation index file after an update
/// or an installation of a plugin.
///
/// \return void
///
/////////////////////////////////////////////////
void Documentation::updateIndexFile()
{
    if (!vDocIndexTable.size())
        throw SyntaxError(SyntaxError::INVALID_HLPIDX, "", SyntaxError::invalid_position);

    // Create a valid file name if it still contains
    // a path placeholder
    if (sDocIndexFile.find("<>") != string::npos)
        sDocIndexFile = FileSystem::ValidFileName(sDocIndexFile, ".hlpidx");

    if (fDocument.is_open())
        fDocument.close();

    // Open the documentation index file and truncate its
    // contents
    fDocument.open(sDocIndexFile.c_str(), ios_base::out | ios_base::trunc);

    if (fDocument.fail())
    {
        fDocument.close();
        throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sDocIndexFile);
    }

    // Write the copyright comment
    fDocument << "<!--\n";
    fDocument << "  NumeRe: Framework fuer Numerische Rechnungen\n"
              << "  Copyright (C) " << AutoVersion::YEAR << "  Erik Haenel et al.\n\n"
              << "  This program is free software: you can redistribute it and/or modify\n"
              << "  it under the terms of the GNU General Public License as published by\n"
              << "  the Free Software Foundation, either version 3 of the License, or\n"
              << "  (at your option) any later version.\n\n"
              << "  This program is distributed in the hope that it will be useful,\n"
              << "  but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
              << "  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
              << "  GNU General Public License for more details.\n\n"
              << "  You should have received a copy of the GNU General Public License\n"
              << "  along with this program.  If not, see <http://www.gnu.org/licenses/>.\n";
    fDocument << "-->" << endl << endl;

    fDocument << "<helpindex>" << endl;

    // Write the single documentation index entries
    for (size_t i = 0; i < vDocIndexTable.size(); i++)
    {
        fDocument << "\t<article id=\"" << vDocIndexTable[i].sArticleId << "\">\n";
        fDocument << "\t\t<file path=\"" << vDocIndexTable[i].sDocFilePath << "\" />\n";
        fDocument << "\t\t<title string=\"" << vDocIndexTable[i].sTitle << "\" idxkey=\"" << vDocIndexTable[i].sIdxKeys << "\" />\n";
        fDocument << "\t\t<keywords>\n";

        // Write the keywords, which correspond to the
        // current article
        for (auto mDocIndexIterator = mDocumentationIndex.begin(); mDocIndexIterator != mDocumentationIndex.end(); ++mDocIndexIterator)
        {
            if (mDocIndexIterator->second == (int)i)
                fDocument << "\t\t\t<keyword>" << mDocIndexIterator->first << "</keyword>\n";
        }

        fDocument << "\t\t</keywords>\n";
        fDocument << "\t</article>\n";
    }

    fDocument << "</helpindex>" << endl;
    fDocument.close();
}


/////////////////////////////////////////////////
/// \brief This member function opens the passed
/// index file and reads its contents to memory.
///
/// \param sIndexFile const string&
/// \return bool
///
/// If any of the read article IDs is already in
/// internal storage, it's links and keywords are
/// update.
/////////////////////////////////////////////////
bool Documentation::loadIndexFile(const string& sIndexFile)
{
    // Open the documentation index file for reading
    fDocument.open(FileSystem::ValidFileName(sIndexFile, ".hlpidx").c_str(), ios_base::in);

    if (fDocument.fail())
    {
        fDocument.close();
        return false;
    }

    string sLine;
    string sDocIndex;
    string sKeyWord;
    vector<string> vEntry;

    // Read the index file's contents completely to memory
    while (!fDocument.eof())
    {
        getline(fDocument, sLine);
        StripSpaces(sLine);

        if (sLine.length())
            sDocIndex += sLine;
    }

    fDocument.close();

    // Ignore comment blocks
    while (sDocIndex.length() && sDocIndex.find("<!--") != string::npos)
    {
        unsigned int nEndPos = sDocIndex.find("-->", sDocIndex.find("<!--")+4);

        if (nEndPos == string::npos)
            sDocIndex.erase(sDocIndex.find("<!--"));
        else
            sDocIndex.erase(sDocIndex.find("<!--"), nEndPos - sDocIndex.find("<!--")+3);
    }

    if (!sDocIndex.length())
        return false;

    while (sDocIndex.length() > 26)
    {
        if (sDocIndex.find("<article ") == string::npos || sDocIndex.find("</article>") == string::npos)
            break;

        sLine = sDocIndex.substr(sDocIndex.find("<article "), sDocIndex.find("</article>")-sDocIndex.find("<article "));
        sDocIndex.erase(0, sDocIndex.find("</article>")+10);

        // Shall this entry be deleted?
        if (sLine.substr(0, sLine.find('>')).find("delete=\"true\"") != string::npos)
        {
            removeFromDocIndex(getArgAtPos(sLine, sLine.find("id=")+3), false);
            continue;
        }

        if (sLine.find("<file ") == string::npos || sLine.find("<keywords>") == string::npos)
            continue;

        DocumentationEntry entry;

        entry.sArticleId = getArgAtPos(sLine, sLine.find("id=")+3);
        entry.sDocFilePath = getArgAtPos(sLine, sLine.find("path=")+5);
        entry.sTitle = getArgAtPos(sLine, sLine.find("string=")+7);
        entry.sIdxKeys = getArgAtPos(sLine, sLine.find("idxkey=")+7);

        addEntry(entry, sLine.substr(sLine.find("<keywords>")+10));

        if (sDocIndex == "</helpindex>")
            break;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This private member function returns
/// the position of the queried topic in the
/// documentation index table.
///
/// \param sTopic const string&
/// \return int
///
/////////////////////////////////////////////////
int Documentation::findPositionInDocumentationIndex(const string& sTopic) const
{
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
/// \brief This private member function loads and
/// prepares the selected documentation article.
///
/// \param sFileName const string&
/// \param sArticleID const string&
/// \return vector<string>
///
/////////////////////////////////////////////////
vector<string> Documentation::loadDocumentationArticle(const string& sFileName, const string& sArticleID)
{
    fDocument.open(sFileName.c_str(), ios_base::in);

    if (fDocument.fail())
    {
        fDocument.close();
        throw SyntaxError(SyntaxError::HLP_FILE_MISSING, "", SyntaxError::invalid_position, sFileName);
    }

    string sLine;
    vector<string> vReturn;

    while (!fDocument.eof())
    {
        getline(fDocument, sLine);
        StripSpaces(sLine);

        if (!sLine.length())
            continue;

        while (sLine.find("<!--") != string::npos)
        {
            if (sLine.find("-->", sLine.find("<!--")+4) != string::npos)
                sLine.erase(sLine.find("<!--"), sLine.find("-->", sLine.find("<!--")+4)+3 - sLine.find("<!--"));
            else
            {
                sLine.erase(sLine.find("<!--"));
                string sLineTemp = "";

                while (!fDocument.eof())
                {
                    getline(fDocument, sLineTemp);
                    StripSpaces(sLineTemp);

                    if (!sLineTemp.length())
                        continue;

                    if (sLineTemp.find("-->") != string::npos)
                    {
                        sLine += sLineTemp.substr(sLineTemp.find("-->")+3);
                        break;
                    }
                }
            }
        }

        if (!sLine.length())
            continue;

        if (sLine.find("<article ") != string::npos)
        {
            if (getArgAtPos(sLine, sLine.find("id=")+3) != sArticleID)
            {
                while (!fDocument.eof())
                {
                    getline(fDocument, sLine);
                    StripSpaces(sLine);

                    if (!sLine.length())
                        continue;

                    if (sLine.find("</article>") != string::npos)
                        break;
                }

                continue;
            }

            sLine.erase(0, sLine.find('>', sLine.find("<article "))+1);
            StripSpaces(sLine);

            if (!sLine.length())
                continue;
        }

        if (sLine.find("<title ") != string::npos)
        {
            vReturn.push_back(getArgAtPos(sLine, sLine.find("string=", sLine.find("<title "))+7));
            sLine.erase(0, sLine.find("/>", sLine.find("<title "))+2);
            StripSpaces(sLine);

            if (!sLine.length())
                continue;
        }

        if (sLine.find("</article>") != string::npos)
        {
            sLine.erase(sLine.find("</article>"));
            StripSpaces(sLine);

            if (!sLine.length())
                break;
        }

        vReturn.push_back(sLine);
    }

    fDocument.close();

    if (!vReturn.size())
        vReturn.push_back("NO_ENTRY_FOUND");

    return vReturn;
}


/////////////////////////////////////////////////
/// \brief This static member is a fallback for
/// the XML-parsing logic-stuff.
///
/// \param sCmd const string&
/// \param nPos unsigned int
/// \return string
///
/////////////////////////////////////////////////
string Documentation::getArgAtPos(const string& sCmd, unsigned int nPos)
{
    string sArgument = "";

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
        for (unsigned int i = nPos + 1; i < sCmd.length(); i++)
        {
            if (sCmd[i] == '"' && sCmd[i - 1] != '\\')
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
        for (unsigned int i = nPos; i < sCmd.length(); i++)
        {
            // Jump over parentheses, if you find one
            if (sCmd[i] == '(' || sCmd[i] == '[' || sCmd[i] == '{')
                i += getMatchingParenthesis(sCmd.substr(i));

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
void Documentation::loadDocIndex(bool bLoadUserLangFiles)
{
    if (sDocIndexFile.find("<>") != string::npos)
        sDocIndexFile = FileSystem::ValidFileName(sDocIndexFile, ".hlpidx");

    if (!bLoadUserLangFiles)
    {
        if (!loadIndexFile(sDocIndexFile))
        {
            if (fileExists(FileSystem::ValidFileName("<>/update.hlpidx", ".hlpidx")))
                loadIndexFile("<>/update.hlpidx");
        }
    }
    else if (fileExists(FileSystem::ValidFileName("<>/user/numere.hlpidx", ".hlpidx")))
        loadIndexFile("<>/user/numere.hlpidx");
}


/////////////////////////////////////////////////
/// \brief This member function updates the
/// documentation index file based upon the
/// passed update file.
///
/// \param _sFilename string
/// \return void
///
/////////////////////////////////////////////////
void Documentation::updateDocIndex(string _sFilename)
{
    _sFilename = FileSystem::ValidFileName(_sFilename, ".hlpidx");

    // Load the updated help index file
    loadIndexFile(_sFilename);

    try
    {
        // update the own index file and remove the
        // update file
        updateIndexFile();
        remove(_sFilename.c_str());
    }
    catch (...)
    {
        NumeReKernel::print(" ERROR: Documentation could not be written.");
    }

    return;
}


/////////////////////////////////////////////////
/// \brief This member function is used to add
/// documentation index entries to the index
/// during a plugin or package installation.
///
/// \param _sIndexToAdd string&
/// \param bUseUserLangFiles bool
/// \return void
///
/////////////////////////////////////////////////
void Documentation::addToDocIndex(string& _sIndexToAdd, bool bUseUserLangFiles)
{
    if (!vDocIndexTable.size())
        throw SyntaxError(SyntaxError::INVALID_HLPIDX, "", SyntaxError::invalid_position);

    // Load default documentation index, if the user uses
    // custom language files
    if (bUseUserLangFiles)
    {
        vDocIndexTable.clear();
        mDocumentationIndex.clear();
        loadDocIndex(false);
    }

    if (!vDocIndexTable.size())
        throw SyntaxError(SyntaxError::INVALID_HLPIDX, "", SyntaxError::invalid_position);

    string sKeyWord;
    string sLine;

    // Parse the added documentation index entry and
    // insert it in the index
    while (_sIndexToAdd.length() > 26)
    {
        if (_sIndexToAdd.find("<article ") == string::npos || _sIndexToAdd.find("</article>") == string::npos)
            break;

        sLine = _sIndexToAdd.substr(_sIndexToAdd.find("<article "), _sIndexToAdd.find("</article>")-_sIndexToAdd.find("<article "));
        _sIndexToAdd.erase(0,_sIndexToAdd.find("</article>")+10);

        if (sLine.find("<keywords>") == string::npos)
            continue;

        DocumentationEntry entry;

        entry.sArticleId = getArgAtPos(sLine, sLine.find("id=")+3);
        entry.sDocFilePath = "<>/docs/plugins/" + entry.sArticleId;
        entry.sTitle = getArgAtPos(sLine, sLine.find("string=")+7);
        entry.sIdxKeys = getArgAtPos(sLine, sLine.find("idxkey=")+7);

        addEntry(entry, sLine.substr(sLine.find("<keywords>")+10));

        if (_sIndexToAdd == "</helpindex>")
            break;
    }

    // Update the index file with the new content
    updateIndexFile();

    // Load custom language files, if necessary
    if (bUseUserLangFiles)
        loadDocIndex(true);
}


/////////////////////////////////////////////////
/// \brief This member function removes the index
/// entry passed to the function from the
/// documentation index.
///
/// \param _sID const string&
/// \param bUseUserLangFiles bool
/// \return void
///
/////////////////////////////////////////////////
void Documentation::removeFromDocIndex(const string& _sID, bool bUseUserLangFiles)
{
    if (!vDocIndexTable.size())
        throw SyntaxError(SyntaxError::INVALID_HLPIDX, "", SyntaxError::invalid_position);

    if (_sID == "<<NO_HLP_ENTRY>>")
        return;

    // Load default index file, if the user
    // uses custom language files
    if (bUseUserLangFiles)
    {
        vDocIndexTable.clear();
        mDocumentationIndex.clear();
        loadDocIndex(false);
    }

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

            vDocIndexTable.erase(vDocIndexTable.begin()+i);
            break;
        }
    }

    // Update the file after the removal
    updateIndexFile();

    // Load custom languge files, if necessary
    if (bUseUserLangFiles)
        loadDocIndex(true);
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// documentation article, which corresponds to
/// the passed documentation topic of the the
/// whole documentation index in readable form.
///
/// \param sTopic const string&
/// \return vector<string>
///
/////////////////////////////////////////////////
vector<string> Documentation::getHelpArticle(const string& sTopic)
{
    vector<string> vReturn;

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
            if (fDocument.is_open())
                fDocument.close();

            if (vDocIndexTable[nIndex].sDocFilePath.find("<>") != string::npos)
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
        map<string,string> mIdx;

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
/// \return vector<string>
///
/////////////////////////////////////////////////
vector<string> Documentation::getDocIndex() const
{
    std::vector<std::string> vReturn;

    // Go through the index table and extract the
    // key list
    for (size_t i = 0; i < vDocIndexTable.size(); i++)
    {
        EndlessVector<std::string> keylist = getAllArguments(vDocIndexTable[i].sIdxKeys);

        for (const std::string& key : keylist)
        {
            if (std::find(vReturn.begin(), vReturn.end(), key) == vReturn.end())
                vReturn.push_back(key);
        }
    }

    std::sort(vReturn.begin(), vReturn.end());

    return vReturn;
}


/////////////////////////////////////////////////
/// \brief This member function returns an index
/// key, which corresponds to the queried topic.
///
/// \param sTopic const string&
/// \return string
///
/////////////////////////////////////////////////
string Documentation::getHelpIdxKey(const string& sTopic)
{
    int nIndex = findPositionInDocumentationIndex(sTopic);
    string sReturn = "";

    if (nIndex == -1)
        return "<<NONE>>";

    sReturn = vDocIndexTable[nIndex].sIdxKeys;

    if (sReturn.find(',') != string::npos)
        sReturn.erase(sReturn.find(','));

    return sReturn;
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// article ID corresponding to the queried topic.
///
/// \param sTopic const string&
/// \return string
///
/////////////////////////////////////////////////
string Documentation::getHelpArtclID(const string& sTopic)
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
/// \param _sIdxKey const string&
/// \return string
///
/////////////////////////////////////////////////
string Documentation::getHelpArticleTitle(const string& _sIdxKey)
{
    int nIndex = findPositionInDocumentationIndex(_sIdxKey);

    if (nIndex == -1)
        return "<<NONE>>";

    return vDocIndexTable[nIndex].sTitle;
}

