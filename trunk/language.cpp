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

void StripSpaces(string&);
string toString(int);
bool fileExists(const string&);
inline string toString(unsigned int nInt)
{
    return toString((int)nInt);
}

Language::Language() : FileSystem()
{
    mLangStrings.clear();
    sYES = "j";
    sNO = "n";
}

map<string,string> Language::getLangFileContent(const string& sFile)
{
    map<string,string> mLangFileContent;
    ifstream fFile_in;
    string sLine;

    fFile_in.open(FileSystem::ValidFileName(sFile, ".nlng").c_str());
    if (fFile_in.fail())
    {
        fFile_in.close();
        return mLangFileContent;
    }

    while (!fFile_in.eof())
    {
        getline(fFile_in, sLine);
        StripSpaces(sLine);
        if (!sLine.length())
            continue;
        if (sLine.front() == '#')
            continue;
        for (unsigned int i = 0; i < sLine.length(); i++)
        {
            if (sLine[i] == '\t')
                sLine[i] = ' ';
        }
        for (unsigned int i = 1; i < sLine.find('='); i++)
        {
            if (sLine[i] == ' ')
            {
                sLine.erase(i, 1);
                i--;
            }
        }
        mLangFileContent[sLine.substr(0,sLine.find('='))] = sLine.substr(sLine.find_first_not_of(' ', sLine.find('=')+1));
    }

    return mLangFileContent;
}

void Language::loadStrings(bool bloadUserFiles)
{
    //string sMainStrings = "<>/lang/main.nlng";
    //string sErrorStrings = "<>/lang/error.nlng";
    //ifstream fStrings_in;
    string sLine;
    map<string,string> mLangFileContent;

    mLangFileContent = getLangFileContent("<>/lang/main.nlng");
    mLangStrings.insert(mLangFileContent.begin(), mLangFileContent.end());
    mLangFileContent.clear();

    mLangFileContent = getLangFileContent("<>/lang/error.nlng");
    for (auto iter = mLangFileContent.begin(); iter != mLangFileContent.end(); ++iter)
        mLangStrings[iter->first] = iter->second;
    mLangFileContent.clear();

    if (bloadUserFiles)
    {
        if (fileExists(FileSystem::ValidFileName("<>/user/lang/main.nlng", ".nlng")))
        {
            mLangFileContent = getLangFileContent("<>/user/lang/main.nlng");
            if (mLangFileContent.size())
            {
                for (auto iter = mLangFileContent.begin(); iter != mLangFileContent.end(); ++iter)
                    mLangStrings[iter->first] = iter->second;
                //mLangStrings.insert(mLangFileContent.begin(), mLangFileContent.end());
                mLangFileContent.clear();
            }
        }
        if (fileExists(FileSystem::ValidFileName("<>/user/lang/error.nlng", ".nlng")))
        {
            mLangFileContent = getLangFileContent("<>/user/lang/error.nlng");
            if (mLangFileContent.size())
            {
                for (auto iter = mLangFileContent.begin(); iter != mLangFileContent.end(); ++iter)
                    mLangStrings[iter->first] = iter->second;
                //mLangStrings.insert(mLangFileContent.begin(), mLangFileContent.end());
                mLangFileContent.clear();
            }
        }

    }

    for (auto iter = mLangStrings.begin(); iter != mLangStrings.end(); ++iter)
    {
        if ((iter->second).length() < 5)
            continue;
        for (unsigned int i = 0; i < (iter->second).length()-3; i++)
        {
            if ((iter->second).substr(i,2) == "%%" && isalpha((iter->second)[i+3]) && (iter->second).find("%%", i+3) != string::npos)
            {
                sLine = (iter->second).substr(i+2, (iter->second).find("%%", i+3)-i-2);
                //cerr << sLine << endl;
                if (mLangStrings.find(sLine) != mLangStrings.end())
                {
                    (iter->second).replace(i, (iter->second).find("%%", i+3)-i+2, mLangStrings[sLine]);
                }
            }
        }
    }

    sYES = mLangStrings["COMMON_YES_NO"].front();
    sNO = mLangStrings["COMMON_YES_NO"].back();

    return;
}

string Language::getKey(const string& sMessage)
{
    string sKey = sMessage;
    if (sKey.find('*') != string::npos)
    {
        sKey.erase(sKey.find('*'));
        for (auto iter = mLangStrings.begin(); iter != mLangStrings.end(); ++iter)
        {
            if ((iter->first).substr(0,sKey.length()) == sKey)
            {
                sKey = iter->first;
                return sKey;
            }
        }
    }

    return sMessage;
}

string Language::get(const string& sMessage, const vector<string>& vTokens)
{
    if (!mLangStrings.size())
        return sMessage;
    string sLangString = sMessage;
    if (sLangString.find('*') != string::npos)
    {
        sLangString.erase(sLangString.find('*'));
        for (auto iter = mLangStrings.begin(); iter != mLangStrings.end(); ++iter)
        {
            if ((iter->first).substr(0,sLangString.length()) == sLangString)
            {
                sLangString = iter->second;
            }
        }
        if (sMessage.substr(0,sLangString.length()) == sLangString)
            return sMessage;
    }
    else if (mLangStrings.find(sMessage) != mLangStrings.end())
    {
        sLangString = mLangStrings[sMessage];
    }
    else
        return sMessage;
    //string sLangString = mLangStrings[sMessage];
    string sToken;
    for (unsigned int i = 0; i < vTokens.size(); i++)
    {
        sToken = "%%" + toString(i+1) + "%%";
        while (sLangString.find(sToken) != string::npos)
        {
            sLangString.replace(sLangString.find(sToken), sToken.size(), vTokens[i]);
        }
    }
    return sLangString;
}

vector<string> Language::getList(const string& sMessageScheme)
{
    vector<string> vListResults;
    string sPrefix = sMessageScheme.substr(0,sMessageScheme.find('*'));
    string sType = "";

    if (sPrefix.find('[') != string::npos)
    {
        sPrefix.erase(sPrefix.find('['));
        sType = sMessageScheme.substr(sMessageScheme.find('[')+1, sMessageScheme.find(']')-sMessageScheme.find('[')-1);
    }
    else if (sMessageScheme.find('[') != string::npos)
        sType = sMessageScheme.substr(sMessageScheme.find('[')+1, sMessageScheme.find(']')-sMessageScheme.find('[')-1);

    for (auto iter = mLangStrings.begin(); iter != mLangStrings.end(); ++iter)
    {
        if ((iter->first).substr(0,sPrefix.length()) != sPrefix)
            continue;
        if (sType.length() && (iter->first).find('[') != string::npos)
        {
            string sCurrentType = (iter->first).substr((iter->first).find('['), (iter->first).find(']')+1-(iter->first).find('['));
            if (sCurrentType.find(sType) == string::npos)
                continue;
        }
        vListResults.push_back(iter->second);
    }

    return vListResults;
}


