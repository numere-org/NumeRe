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
#include "../utils/stringtools.hpp"
#include "../io/logger.hpp"
#include "../structures.hpp"


bool fileExists(const std::string&);
Language _lang;


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
/// \param sFile std::string
/// \return std::map<std::string,std::string>
///
/////////////////////////////////////////////////
std::map<std::string,std::string> Language::getLangFileContent(std::string sFile) const
{
    std::map<std::string,std::string> mLangFileContent;
    boost::nowide::ifstream fFile_in;
    std::string sLine;

    try
    {
        sFile = FileSystem::ValidFileName(sFile, ".nlng");
    }
    catch (...)
    {
        g_logger.error("Language file path '" + sFile + "' is invalid.");
        return mLangFileContent;
    }

    fFile_in.open(sFile);

    // Check, if file has been successfully opened
    if (fFile_in.fail())
    {
        fFile_in.close();
        g_logger.error("Could not find or load language file '" + sFile + "'.");
        return mLangFileContent;
    }

    // Read the complete file
    while (!fFile_in.eof())
    {
        std::getline(fFile_in, sLine);
        StripSpaces(sLine);

        // Ignore empty lines
        if (!sLine.length())
            continue;

        // Ignore comments
        if (sLine.front() == '#')
            continue;

        // Ensure that the language string corresponds to
        // its syntax requirements
        size_t eq_pos = sLine.find('=');

        if (eq_pos == std::string::npos || eq_pos == 0 || eq_pos+1 == sLine.length())
        {
            g_logger.error("Erroneous language string '" + sLine + "' has been found in '" + sFile + "'.");
            continue;
        }

        // Replace included tab characters with
        // whitespaces
        replaceAll(sLine, "\t", " ");

        // Remove whitespaces in front of the first
        // equal sign
        for (size_t i = 1; i < sLine.find('='); i++)
        {
            if (sLine[i] == ' ')
            {
                sLine.erase(i, 1);
                i--;
            }
        }

        eq_pos = sLine.find('=');

        // Replace tabs
        replaceAll(sLine, "%%TAB%%", "\t", eq_pos+1);

        // Replace linebreaks
        replaceAll(sLine, "%%LINEBREAK%%", "\n", eq_pos+1);

        // Replace itemisations
        replaceAll(sLine, "%%ITEMIZE%%", "\n    - ", eq_pos+1);

        // Replace end of itemisations
        replaceAll(sLine, "%%ITEMIZE_END%%", "\n    ", eq_pos+1);

        // Store the token in the map
        mLangFileContent[sLine.substr(0, eq_pos)] = sLine.substr(sLine.find_first_not_of(' ', eq_pos+1));
    }

    return mLangFileContent;
}


/////////////////////////////////////////////////
/// \brief This private member function is a
/// simple helper for Language::loadStrings().
///
/// \param sLanguageFileName const std::string&
/// \return void
///
/////////////////////////////////////////////////
void Language::loadAndInsert(const std::string& sLanguageFileName)
{
    // Load language file
    std::map<std::string,std::string> mLangFileContent = getLangFileContent(sLanguageFileName);

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
    std::string sLine;
    std::map<std::string,std::string> mLangFileContent;

    // Clear already existing strings first
    if (mLangStrings.size())
        mLangStrings.clear();

    // Load main file
    mLangFileContent = getLangFileContent("<>/lang/main.nlng");
    mLangStrings.insert(mLangFileContent.begin(), mLangFileContent.end());

    // Load errors file
    loadAndInsert("<>/lang/error.nlng");

    // Load NumeRe specific syntax language files
    loadAndInsert("<>/lang/numere.nlng");

    // Shall user language files be loaded?
    if (bloadUserFiles)
    {
        // Load user main language file, if it exists
        if (fileExists(FileSystem::ValidFileName("<>/user/lang/main.nlng", ".nlng")))
            loadAndInsert("<>/user/lang/main.nlng");

        // Load user error language file, if it exists
        if (fileExists(FileSystem::ValidFileName("<>/user/lang/error.nlng", ".nlng")))
            loadAndInsert("<>/user/lang/error.nlng");

        // Load user NumeRe specific syntax language file, if it exists
        if (fileExists(FileSystem::ValidFileName("<>/user/lang/numere.nlng", ".nlng")))
            loadAndInsert("<>/user/lang/numere.nlng");
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
            if ((iter->second).substr(i, 2) == "%%" && isalpha((iter->second)[i+3]) && (iter->second).find("%%", i+3) != std::string::npos)
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
/// \param _langstrings const std::map<std::string,std::string>&
/// \return void
///
/////////////////////////////////////////////////
void Language::addToLanguage(const std::map<std::string,std::string>& _langstrings)
{
    for (auto iter = _langstrings.begin(); iter != _langstrings.end(); ++iter)
        mLangStrings[iter->first] = iter->second;
}


/////////////////////////////////////////////////
/// \brief Determine, whether the language table
/// contains a string for the selected ID.
///
/// \param sId const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool Language::containsString(const std::string& sId) const
{
    // Search for the language string
    if (sId.find('*') != std::string::npos)
    {
        std::string sPrefix = sId.substr(0, sId.find('*'));

        // Find a candidate for the passed identifier
        // containing an asteriks
        for (auto iter = mLangStrings.begin(); iter != mLangStrings.end(); ++iter)
        {
            if ((iter->first).starts_with(sPrefix))
                return true;
        }
    }
    else if (mLangStrings.find(sId) != mLangStrings.end())
        return true;

    return false;
}


/////////////////////////////////////////////////
/// \brief This member function searches the
/// internal language map for an identifier, which
/// starts similar with the passed identifier and
/// returns the found identifier.
///
/// \param sMessage const std::string&
/// \return std::string
///
/// The passed language identifier has to contain
/// an asteriks as a wildcard.
/////////////////////////////////////////////////
std::string Language::getKey(const std::string& sMessage) const
{
    std::string sKey = sMessage;

    // Do only something, when a wildcard is available
    if (sKey.find('*') != std::string::npos)
    {
        sKey.erase(sKey.find('*'));

        // Find a candidate for the passed token
        for (auto iter = mLangStrings.begin(); iter != mLangStrings.end(); ++iter)
        {
            if ((iter->first).starts_with(sKey))
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
/// \param sMessage const std::string&
/// \param vTokens const std::vector<std::string>&
/// \return string
///
/////////////////////////////////////////////////
std::string Language::get(const std::string& sMessage, const std::vector<std::string>& vTokens) const
{
    if (!mLangStrings.size())
        return sMessage;

    std::string sLangString = sMessage;

    // Search for the language string
    if (sLangString.find('*') != std::string::npos)
    {
        sLangString.erase(sLangString.find('*'));

        // Find a candidate for the passed identifier
        // containing an asteriks
        for (auto iter = mLangStrings.begin(); iter != mLangStrings.end(); ++iter)
        {
            if ((iter->first).starts_with(sLangString))
                sLangString = iter->second;
        }

        if (sMessage.starts_with(sLangString))
            return sMessage;
    }
    else if (mLangStrings.find(sMessage) != mLangStrings.end())
        sLangString = mLangStrings.at(sMessage);
    else
        return sMessage;

    std::string sToken;

    // Replace all enumerated tokens
    for (size_t i = 0; i < vTokens.size(); i++)
    {
        sToken = "%%" + toString(i+1) + "%%";
        size_t pos = 0;

        // As long as further tokens can be found
        while ((pos = sLangString.find(sToken, pos)) != std::string::npos)
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
/// \param sMessageScheme const std::string&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> Language::getList(const std::string& sMessageScheme) const
{
    std::vector<std::string> vListResults;
    std::string sPrefix = sMessageScheme.substr(0, sMessageScheme.find('*'));
    std::string sType = "";

    // Extract the type (available in command and function
    // documentations, for example)
    if (sPrefix.find('[') != std::string::npos)
    {
        sPrefix.erase(sPrefix.find('['));
        sType = sMessageScheme.substr(sMessageScheme.find('[')+1, sMessageScheme.find(']')-sMessageScheme.find('[')-1);
    }
    else if (sMessageScheme.find('[') != std::string::npos)
        sType = sMessageScheme.substr(sMessageScheme.find('[')+1, sMessageScheme.find(']')-sMessageScheme.find('[')-1);

    for (auto iter = mLangStrings.begin(); iter != mLangStrings.end(); ++iter)
    {
        if (!(iter->first).starts_with(sPrefix))
            continue;

        // Ensure that the found identifier has the
        // the correct type if a type was selected
        if (sType.length() && (iter->first).find('[') != std::string::npos)
        {
            std::string sCurrentType = (iter->first).substr((iter->first).find('['), (iter->first).find(']')+1-(iter->first).find('['));

            if (sCurrentType.find(sType) == std::string::npos)
                continue;
        }

        // Append the found occurence to the vector
        vListResults.push_back(iter->second);
    }

    return vListResults;
}


