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



#include "language.hpp"

using namespace std;

void StripSpaces(string&);
string toString(int);
bool fileExists(const string&);
string toString(unsigned int nInt);

/////////////////////////////////////////////////
/// \brief Contructor of the language class.
/////////////////////////////////////////////////
Language::Language() : FileSystem()
{
    mLangStrings.clear();
    sYES = "j";
    sNO = "n";
}


/////////////////////////////////////////////////
/// \brief Copy constructor of the language
/// class.
///
/// \param _lang const Language&
///
/////////////////////////////////////////////////
Language::Language(const Language& _lang) : FileSystem()
{
    mLangStrings = _lang.mLangStrings;
    sYES = _lang.sYES;
    sNO = _lang.sNO;
}


/////////////////////////////////////////////////
/// \brief This private member function decodes a
/// single language file into a map and returns
/// it.
///
/// \param sFile const string&
/// \return map<string,string>
///
/////////////////////////////////////////////////
map<string,string> Language::getLangFileContent(const string& sFile)
{
    map<string,string> mLangFileContent;
    ifstream fFile_in;
    string sLine;

    fFile_in.open(FileSystem::ValidFileName(sFile, ".nlng").c_str());

    // Check, if file has been successfully opened
    if (fFile_in.fail())
    {
        fFile_in.close();
        return mLangFileContent;
    }

    // Read the complete file
    while (!fFile_in.eof())
    {
        getline(fFile_in, sLine);
        StripSpaces(sLine);

        // Ignore empty lines
        if (!sLine.length())
            continue;

        // Ignore comments
        if (sLine.front() == '#')
            continue;

        // Replace included tab characters with
        // whitespaces
        for (size_t i = 0; i < sLine.length(); i++)
        {
            if (sLine[i] == '\t')
                sLine[i] = ' ';
        }

        // Remove whitespaces in front of the first
        // equal sign
        for (unsigned int i = 1; i < sLine.find('='); i++)
        {
            if (sLine[i] == ' ')
            {
                sLine.erase(i, 1);
                i--;
            }
        }

        // Replace tabs
        while (sLine.find("%%TAB%%") != string::npos)
            sLine.replace(sLine.find("%%TAB%%"), 7, "\t");

        // Replace linebreaks
        while (sLine.find("%%LINEBREAK%%") != string::npos)
            sLine.replace(sLine.find("%%LINEBREAK%%"), 13, "\n");

        // Store the token in the map
        mLangFileContent[sLine.substr(0, sLine.find('='))] = sLine.substr(sLine.find_first_not_of(' ', sLine.find('=')+1));
    }

    return mLangFileContent;
}


/////////////////////////////////////////////////
/// \brief This private member function is a
/// simple helper for Language::loadStrings().
///
/// \param sLanguageFileName const string&
/// \return void
///
/////////////////////////////////////////////////
void Language::loadAndInsert(const string& sLanguageFileName)
{
    // Load language file
    map<string,string> mLangFileContent = getLangFileContent(sLanguageFileName);

    // Insert the strings
    for (auto iter = mLangFileContent.begin(); iter != mLangFileContent.end(); ++iter)
        mLangStrings[iter->first] = iter->second;
}


/////////////////////////////////////////////////
/// \brief This member function loads the language
/// files to the internal map and replaces the
/// named tokens with the corresponding language
/// strings afterwards.
///
/// \param bloadUserFiles bool
/// \return void
///
/////////////////////////////////////////////////
void Language::loadStrings(bool bloadUserFiles)
{
    string sLine;
    map<string,string> mLangFileContent;

    // Clear already existing strings first
    if (mLangStrings.size())
        mLangStrings.clear();

    // Load main file
    mLangFileContent = getLangFileContent("<>/lang/main.nlng");
    mLangStrings.insert(mLangFileContent.begin(), mLangFileContent.end());

    // Load errors file
    loadAndInsert("<>/lang/error.nlng");

    // Shall user language files be loaded?
    if (bloadUserFiles)
    {
        // Load user main language file, if it exists
        if (fileExists(FileSystem::ValidFileName("<>/user/lang/main.nlng", ".nlng")))
            loadAndInsert("<>/user/lang/main.nlng");

        // Load user error language file, if it exists
        if (fileExists(FileSystem::ValidFileName("<>/user/lang/error.nlng", ".nlng")))
            loadAndInsert("<>/user/lang/error.nlng");
    }

    // Replace all named tokens with their corresponding
    // language string
    for (auto iter = mLangStrings.begin(); iter != mLangStrings.end(); ++iter)
    {
        // Ignore too short strings
        if ((iter->second).length() < 5)
            continue;

        // Search tokens, check, whether they exist
        // and replace them with their corresponding
        // strings
        for (size_t i = 0; i < (iter->second).length()-3; i++)
        {
            if ((iter->second).substr(i, 2) == "%%" && isalpha((iter->second)[i+3]) && (iter->second).find("%%", i+3) != string::npos)
            {
                sLine = (iter->second).substr(i+2, (iter->second).find("%%", i+3)-i-2);

                if (mLangStrings.find(sLine) != mLangStrings.end())
                    (iter->second).replace(i, (iter->second).find("%%", i+3)-i+2, mLangStrings[sLine]);
            }
        }
    }

    sYES = mLangStrings["COMMON_YES_NO"].front();
    sNO = mLangStrings["COMMON_YES_NO"].back();

    return;
}


/////////////////////////////////////////////////
/// \brief This member function adds the contents
/// of the passed language file into the internal
/// map.
///
/// \param _langstrings const map<string,string>&
/// \return void
///
/////////////////////////////////////////////////
void Language::addToLanguage(const map<string,string>& _langstrings)
{
    for (auto iter = _langstrings.begin(); iter != _langstrings.end(); ++iter)
        mLangStrings[iter->first] = iter->second;
}


/////////////////////////////////////////////////
/// \brief This member function searches the
/// internal language map for an identifier, which
/// starts similar with the passed identifier and
/// returns the found identifier.
///
/// \param sMessage const string&
/// \return string
///
/// The passed language identifier has to contain
/// an asteriks as a wildcard.
/////////////////////////////////////////////////
string Language::getKey(const string& sMessage) const
{
    string sKey = sMessage;

    // Do only something, when a wildcard is available
    if (sKey.find('*') != string::npos)
    {
        sKey.erase(sKey.find('*'));

        // Find a candidate for the passed token
        for (auto iter = mLangStrings.begin(); iter != mLangStrings.end(); ++iter)
        {
            if ((iter->first).substr(0, sKey.length()) == sKey)
            {
                sKey = iter->first;
                return sKey;
            }
        }
    }

    return sMessage;
}


/////////////////////////////////////////////////
/// \brief This member function returns the
/// language string for the passed language
/// identifier and replaces all enumerated tokens
/// with the passed token list.
///
/// \param sMessage const string&
/// \param vTokens const vector<string>&
/// \return string
///
/////////////////////////////////////////////////
string Language::get(const string& sMessage, const vector<string>& vTokens) const
{
    if (!mLangStrings.size())
        return sMessage;

    string sLangString = sMessage;

    // Search for the language string
    if (sLangString.find('*') != string::npos)
    {
        sLangString.erase(sLangString.find('*'));

        // Find a candidate for the passed identifier
        // containing an asteriks
        for (auto iter = mLangStrings.begin(); iter != mLangStrings.end(); ++iter)
        {
            if ((iter->first).substr(0,sLangString.length()) == sLangString)
                sLangString = iter->second;
        }

        if (sMessage.substr(0, sLangString.length()) == sLangString)
            return sMessage;
    }
    else if (mLangStrings.find(sMessage) != mLangStrings.end())
        sLangString = mLangStrings.at(sMessage);
    else
        return sMessage;

    string sToken;

    // Replace all enumerated tokens
    for (size_t i = 0; i < vTokens.size(); i++)
    {
        sToken = "%%" + toString(i+1) + "%%";
        size_t pos = 0;

        // As long as further tokens can be found
        while ((pos = sLangString.find(sToken, pos)) != string::npos)
        {
            sLangString.replace(pos, sToken.length(), vTokens[i]);
            pos += vTokens[i].length();
        }
    }

    return sLangString;
}


/////////////////////////////////////////////////
/// \brief This member function returns a vector
/// of language strings matching to the passed
/// identifier containing wildcards.
///
/// \param sMessageScheme const string&
/// \return vector<string>
///
/////////////////////////////////////////////////
vector<string> Language::getList(const string& sMessageScheme) const
{
    vector<string> vListResults;
    string sPrefix = sMessageScheme.substr(0, sMessageScheme.find('*'));
    string sType = "";

    // Extract the type (available in command and function
    // documentations, for example)
    if (sPrefix.find('[') != string::npos)
    {
        sPrefix.erase(sPrefix.find('['));
        sType = sMessageScheme.substr(sMessageScheme.find('[')+1, sMessageScheme.find(']')-sMessageScheme.find('[')-1);
    }
    else if (sMessageScheme.find('[') != string::npos)
        sType = sMessageScheme.substr(sMessageScheme.find('[')+1, sMessageScheme.find(']')-sMessageScheme.find('[')-1);

    for (auto iter = mLangStrings.begin(); iter != mLangStrings.end(); ++iter)
    {
        if ((iter->first).substr(0, sPrefix.length()) != sPrefix)
            continue;

        // Ensure that the found identifier has the
        // the correct type if a type was selected
        if (sType.length() && (iter->first).find('[') != string::npos)
        {
            string sCurrentType = (iter->first).substr((iter->first).find('['), (iter->first).find(']')+1-(iter->first).find('['));

            if (sCurrentType.find(sType) == string::npos)
                continue;
        }

        // Append the found occurence to the vector
        vListResults.push_back(iter->second);
    }

    return vListResults;
}


