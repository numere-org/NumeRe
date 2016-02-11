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

#include "doc_helper.hpp"

Documentation::Documentation() : FileSystem()
{
    vDocIndexTable.reserve(128);
    sDocIndexFile = "<>/numere.hlpidx";
}

Documentation::~Documentation()
{
    if (fDocument.is_open())
        fDocument.close();
}

void Documentation::updateIndexFile()
{
    if (!vDocIndexTable.size())
        throw INVALID_HLPIDX;
    if (sDocIndexFile.find("<>") != string::npos)
        sDocIndexFile = FileSystem::ValidFileName(sDocIndexFile, ".hlpidx");
    if (fDocument.is_open())
        fDocument.close();
    fDocument.open(sDocIndexFile.c_str(), ios_base::out | ios_base::trunc);
    if (fDocument.fail())
    {
        fDocument.close();
        sErrorToken = sDocIndexFile;
        throw CANNOT_READ_FILE;
    }
    fDocument << "<helpindex>" << endl;
    map<string,int>::iterator mDocIndexIterator;
    for (int i = 0; i < (int)vDocIndexTable.size(); i++)
    {
        fDocument << "\t<article id=\"" << vDocIndexTable[i][0] << "\">" << endl;
        fDocument << "\t\t<file path=\"" << vDocIndexTable[i][1] << "\" />" << endl;
        fDocument << "\t\t<title string=\"" << vDocIndexTable[i][2] << "\" idxkey=\"" << vDocIndexTable[i][3] << "\" />" << endl;
        fDocument << "\t\t<keywords>" << endl;
        mDocIndexIterator = mDocumentationIndex.begin();
        for (;mDocIndexIterator != mDocumentationIndex.end(); ++mDocIndexIterator)
        {
            if (mDocIndexIterator->second == i)
                fDocument << "\t\t\t<keyword>" << mDocIndexIterator->first << "</keyword>" << endl;
        }
        fDocument << "\t\t</keywords>" << endl;
        fDocument << "\t</article>" << endl;
    }
    fDocument << "</helpindex>" << endl;
    fDocument.close();
    return;
}

void Documentation::loadDocIndex()
{
    if (sDocIndexFile.find("<>") != string::npos)
        sDocIndexFile = FileSystem::ValidFileName(sDocIndexFile, ".hlpidx");

    fDocument.open(sDocIndexFile.c_str(), ios_base::in);
    if (fDocument.fail())
    {
        fDocument.close();
        string sUpdateFile = FileSystem::ValidFileName("<>/update.hlpidx", ".hlpidx");
        fDocument.open(sUpdateFile.c_str(), ios_base::in);
        if (fDocument.fail())
        {
            fDocument.close();
            cerr << endl << " FEHLER: Dokumentationsverzeichnis konnte nicht gefunden werden." << endl;
            return;
        }
    }

    string sLine = "";
    string sDocIndex = "";
    string sKeyWord = "";
    int nIndex = 0;
    vector<string> vEntry;

    while (!fDocument.eof())
    {
        getline(fDocument, sLine);
        StripSpaces(sLine);
        if (sLine.length())
            sDocIndex += sLine;
    }
    fDocument.close();
    if (!sDocIndex.length())
    {
        cerr << endl << " FEHLER: Dokumentationsverzeichnis konnte nicht gelesen werden." << endl;
        return;
    }
    while (sDocIndex.length() > 26)
    {
        if (sDocIndex.find("<article ") == string::npos || sDocIndex.find("</article>") == string::npos)
            break;
        sLine = sDocIndex.substr(sDocIndex.find("<article "), sDocIndex.find("</article>")-sDocIndex.find("<article "));
        sDocIndex.erase(0,sDocIndex.find("</article>")+10);
        if (sLine.find("<file ") == string::npos || sLine.find("<keywords>") == string::npos)
            continue;
        vEntry.push_back(getArgAtPos(sLine, sLine.find("id=")+3));
        vEntry.push_back(getArgAtPos(sLine, sLine.find("path=")+5));
        vEntry.push_back(getArgAtPos(sLine, sLine.find("string=")+7));
        vEntry.push_back(getArgAtPos(sLine, sLine.find("idxkey=")+7));
        vDocIndexTable.push_back(vEntry);
        vEntry.clear();

        while (sLine.find("<keyword>") != string::npos)
        {
            sKeyWord = sLine.substr(sLine.find("<keyword>")+9, sLine.find("</keyword>")-9-sLine.find("<keyword>"));
            sLine.erase(0,sLine.find("</keyword>")+10);

            mDocumentationIndex[sKeyWord] = nIndex;
        }

        nIndex++;
        if (sDocIndex == "</helpindex>")
            break;
    }
    return;
}

void Documentation::updateDocIndex(string _sFilename)
{
    _sFilename = FileSystem::ValidFileName(_sFilename, ".hlpidx");
    if (!vDocIndexTable.size())
    {
        cerr << endl << "FEHLER: Es wurde kein Dokumentationsverzeichnis geladen." << endl;
        return;
    }
    string sLine = "";
    string sDocIndex = "";
    string sKeyWord = "";
    int nIndex = vDocIndexTable.size();
    vector<string> vEntry;

    if (fDocument.is_open())
        fDocument.close();
    fDocument.open(_sFilename.c_str(), ios_base::in);
    if (fDocument.fail())
    {
        fDocument.close();
        cerr << endl << " FEHLER: Dokumentationsverzeichnis konnte nicht aktualisiert werden." << endl;
        return;
    }

    while (!fDocument.eof())
    {
        getline(fDocument, sLine);
        StripSpaces(sLine);
        if (sLine.length())
            sDocIndex += sLine;
    }
    fDocument.close();
    if (!sDocIndex.length())
    {
        cerr << endl << " FEHLER: Dokumentationsverzeichnis-Update konnte nicht gelesen werden." << endl;
        return;
    }
    while (sDocIndex.length() > 26)
    {
        if (sDocIndex.find("<article ") == string::npos || sDocIndex.find("</article>") == string::npos)
            break;
        sLine = sDocIndex.substr(sDocIndex.find("<article "), sDocIndex.find("</article>")-sDocIndex.find("<article "));
        sDocIndex.erase(0,sDocIndex.find("</article>")+10);
        if (sLine.find("<file ") == string::npos || sLine.find("<keywords>") == string::npos)
            continue;
        vEntry.push_back(getArgAtPos(sLine, sLine.find("id=")+3));
        vEntry.push_back(getArgAtPos(sLine, sLine.find("path=")+5));
        vEntry.push_back(getArgAtPos(sLine, sLine.find("string=")+7));
        vEntry.push_back(getArgAtPos(sLine, sLine.find("idxkey=")+7));
        for (unsigned int i = 0; i < vDocIndexTable.size(); i++)
        {
            if (vEntry[0] == vDocIndexTable[i][0])
            {
                vDocIndexTable[i] = vEntry;
                vEntry.clear();

                while (sLine.find("<keyword>") != string::npos)
                {
                    sKeyWord = sLine.substr(sLine.find("<keyword>")+9, sLine.find("</keyword>")-9-sLine.find("<keyword>"));
                    sLine.erase(0,sLine.find("</keyword>")+10);

                    mDocumentationIndex[sKeyWord] = i;
                }

                break;
            }
        }
        if (vEntry.size())
        {
            vDocIndexTable.push_back(vEntry);
            vEntry.clear();
        }
        else
            continue;
        while (sLine.find("<keyword>") != string::npos)
        {
            sKeyWord = sLine.substr(sLine.find("<keyword>")+9, sLine.find("</keyword>")-9-sLine.find("<keyword>"));
            sLine.erase(0,sLine.find("</keyword>")+10);

            mDocumentationIndex[sKeyWord] = nIndex;
        }

        nIndex++;
        if (sDocIndex == "</helpindex>")
            break;
    }

    try
    {
        updateIndexFile();
        remove(_sFilename.c_str());
    }
    catch (...)
    {
        cerr << endl << " FEHLER: Dokumentationsverzeichnis konnte nicht geschrieben werden." << endl;
    }

    return;
}

void Documentation::addToDocIndex(string& _sIndexToAdd)
{
    if (!vDocIndexTable.size())
        throw INVALID_HLPIDX;

    string sKeyWord = "";
    string sLine = "";
    vector<string> vEntry;
    int nIndex = vDocIndexTable.size();

    while (_sIndexToAdd.length() > 26)
    {
        if (_sIndexToAdd.find("<article ") == string::npos || _sIndexToAdd.find("</article>") == string::npos)
            break;
        sLine = _sIndexToAdd.substr(_sIndexToAdd.find("<article "), _sIndexToAdd.find("</article>")-_sIndexToAdd.find("<article "));
        _sIndexToAdd.erase(0,_sIndexToAdd.find("</article>")+10);
        if (sLine.find("<keywords>") == string::npos)
            continue;
        vEntry.push_back(getArgAtPos(sLine, sLine.find("id=")+3));
        vEntry.push_back("<>/docs/plugins/" + vEntry[0]);
        vEntry.push_back(getArgAtPos(sLine, sLine.find("string=")+7));
        vEntry.push_back(getArgAtPos(sLine, sLine.find("idxkey=")+7));
        for (unsigned int i = 0; i < vDocIndexTable.size(); i++)
        {
            if (vEntry[0] == vDocIndexTable[i][0])
            {
                vDocIndexTable[i] = vEntry;
                vEntry.clear();

                while (sLine.find("<keyword>") != string::npos)
                {
                    sKeyWord = sLine.substr(sLine.find("<keyword>")+9, sLine.find("</keyword>")-9-sLine.find("<keyword>"));
                    sLine.erase(0,sLine.find("</keyword>")+10);

                    mDocumentationIndex[sKeyWord] = i;
                }

                break;
            }
        }
        if (vEntry.size())
        {
            vDocIndexTable.push_back(vEntry);
            vEntry.clear();
        }
        else
            continue;
        while (sLine.find("<keyword>") != string::npos)
        {
            sKeyWord = sLine.substr(sLine.find("<keyword>")+9, sLine.find("</keyword>")-9-sLine.find("<keyword>"));
            sLine.erase(0,sLine.find("</keyword>")+10);

            mDocumentationIndex[sKeyWord] = nIndex;
        }

        nIndex++;
        if (_sIndexToAdd == "</helpindex>")
            break;
    }

    updateIndexFile();

    return;
}

void Documentation::removeFromDocIndex(const string& _sID)
{
    if (!vDocIndexTable.size())
        throw INVALID_HLPIDX;
    if (_sID == "<<NO_HLP_ENTRY>>")
        return;
    map<string,int>::iterator iter = mDocumentationIndex.begin();
    vector<vector<string> >::iterator vIter = vDocIndexTable.begin();

    for (unsigned int i = 0; i < vDocIndexTable.size(); i++)
    {
        if (vDocIndexTable[i][0] == _sID)
        {
            for (; iter != mDocumentationIndex.end(); ++iter)
            {
                if (iter->second == (int)i)
                    mDocumentationIndex.erase(iter);
                else if (iter->second > (int)i)
                {
                    (iter->second)--;
                }
            }
            vDocIndexTable.erase(vIter+i);
            break;
        }
    }

    updateIndexFile();

    return;
}

vector<string> Documentation::getHelpArticle(const string& _sTheme)
{
    vector<string> vReturn;
    int nIndex = -1;
    string sLine = "";

    if (!vDocIndexTable.size())
        throw INVALID_HLPIDX;

    if (_sTheme != "idx" && _sTheme != "index")
    {
        map<string,int>::iterator iter = mDocumentationIndex.begin();
        map<string,int>::iterator firstIndex = mDocumentationIndex.end();
        map<string,int>::iterator secondIndex = mDocumentationIndex.end();
        for (unsigned int i = _sTheme.length(); i > 0; i--)
        {
            if (firstIndex != mDocumentationIndex.end())
                iter = firstIndex;
            else
                iter = mDocumentationIndex.begin();
            //int n = 0;
            for (; iter != secondIndex; ++iter)
            {
                if (iter->first[0] < _sTheme[0])
                    continue;
                if (iter->first[0] == _sTheme[0] && firstIndex == mDocumentationIndex.end())
                    firstIndex = iter;
                if (iter->first[0] > _sTheme[0])
                {
                    secondIndex = iter;
                    break;
                }
                //cerr << iter->first << endl;
                if (iter->first == _sTheme.substr(0,i))
                {
                    nIndex = iter->second;
                    break;
                }
                //n++;
            }
            if (nIndex != -1)
                break;
        }

        if (nIndex != -1)
        {
            if (fDocument.is_open())
                fDocument.close();
            if (vDocIndexTable[nIndex][1].find("<>") != string::npos)
                vDocIndexTable[nIndex][1] = FileSystem::ValidFileName(vDocIndexTable[nIndex][1], ".nhlp");
            fDocument.open(vDocIndexTable[nIndex][1].c_str(), ios_base::in);
            if (fDocument.fail())
            {
                fDocument.close();
                sErrorToken = vDocIndexTable[nIndex][1];
                throw HLP_FILE_MISSING;
            }

            while (!fDocument.eof())
            {
                getline(fDocument, sLine);
                StripSpaces(sLine);
                if (!sLine.length())
                    continue;
                if (sLine.find("<article ") != string::npos)
                {
                    if (getArgAtPos(sLine, sLine.find("id=")+3) != vDocIndexTable[nIndex][0])
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
                    vReturn.push_back(toUpperCase(getArgAtPos(sLine, sLine.find("string=", sLine.find("<title "))+7)));
                    sLine.erase(0, sLine.find("/>", sLine.find("<title "))+2);
                    StripSpaces(sLine);
                    if (!sLine.length())
                        continue;
                }
                if (sLine.find("</article>") != string::npos)
                {
                    sLine.erase(sLine.find("</article>"));
                    StripSpaces(sLine);
                    if (!sLine.length());
                        break;
                }
                vReturn.push_back(sLine);
            }
            if (!vReturn.size())
                vReturn.push_back("NO_ENTRY_FOUND");
        }
        else
        {
            vReturn.push_back("NO_ENTRY_FOUND");
        }
    }
    else
    {
        string sKeyList = "";
        vReturn.push_back("INDEX");
        map<string,string> mIdx;
        for (unsigned int i = 0; i < vDocIndexTable.size(); i++)
        {
            sKeyList = vDocIndexTable[i][3];
            while (sKeyList.find(',') != string::npos)
            {
                mIdx[sKeyList.substr(0,sKeyList.find(','))] = vDocIndexTable[i][2];
                sKeyList.erase(0,sKeyList.find(',')+1);
                StripSpaces(sKeyList);
            }
            mIdx[sKeyList] = vDocIndexTable[i][2];
            //mIdx[vDocIndexTable[i][2]] = vDocIndexTable[i][3];
        }
        map<string,string>::iterator iter = mIdx.begin();
        vReturn.push_back("Schlüsselwörter und verknüpfte Artikel dieser Dokumentation:");
        vReturn.push_back("<list>");
        for (; iter != mIdx.end(); ++iter)
        {
            vReturn.push_back("<item node=\"" + iter->first + "\">" + iter->second + "</item>");
        }
        vReturn.push_back("</list>");
        vReturn.push_back("\r|   -- " + toString((int)vReturn.size()-4) + " Schlüsselwörter und " + toString(vDocIndexTable.size()) + " Artikel --");
        //map<string,int>::iterator iter = mDocumentationIndex.begin();
        /*string sEntry = "";
        for (; iter != mDocumentationIndex.end(); ++iter)
        {
            sEntry = "-" + vDocIndexTable[iter->second][1];
            sEntry.append(18-sEntry.length(), ' ');
            sEntry += vDocIndexTable[iter->second][3];
            vReturn.push_back(sEntry);
        }*/
    }

    return vReturn;
}

string Documentation::getHelpIdxKey(const string& _sTheme)
{
    int nIndex = -1;
    string sReturn = "";
    map<string,int>::iterator iter = mDocumentationIndex.begin();
    map<string,int>::iterator firstIndex = mDocumentationIndex.end();
    map<string,int>::iterator secondIndex = mDocumentationIndex.end();

    for (unsigned int i = _sTheme.length(); i > 0; i--)
    {
        if (firstIndex != mDocumentationIndex.end())
            iter = firstIndex;
        else
            iter = mDocumentationIndex.begin();
        //int n = 0;
        for (; iter != secondIndex; ++iter)
        {
            if (iter->first[0] < _sTheme[0])
                continue;
            if (iter->first[0] == _sTheme[0] && firstIndex == mDocumentationIndex.end())
                firstIndex = iter;
            if (iter->first[0] > _sTheme[0])
            {
                secondIndex = iter;
                break;
            }
            //cerr << iter->first << endl;
            if (iter->first == _sTheme.substr(0,i))
            {
                nIndex = iter->second;
                break;
            }
            //n++;
        }
        if (nIndex != -1)
            break;
    }
    if (nIndex == -1)
        return "<<NONE>>";
    sReturn = vDocIndexTable[nIndex][3];
    if (sReturn.find(',') != string::npos)
        sReturn.erase(sReturn.find(','));
    return sReturn;
}

string Documentation::getHelpArtclID(const string& _sTheme)
{
    map<string,int>::iterator iter = mDocumentationIndex.begin();
    map<string,int>::iterator firstIndex = mDocumentationIndex.end();
    map<string,int>::iterator secondIndex = mDocumentationIndex.end();

    for (unsigned int i = _sTheme.length(); i > 0; i--)
    {
        if (firstIndex != mDocumentationIndex.end())
            iter = firstIndex;
        else
            iter = mDocumentationIndex.begin();
        //int n = 0;
        for (; iter != secondIndex; ++iter)
        {
            if (iter->first[0] < _sTheme[0])
                continue;
            if (iter->first[0] == _sTheme[0] && firstIndex == mDocumentationIndex.end())
                firstIndex = iter;
            if (iter->first[0] > _sTheme[0])
            {
                secondIndex = iter;
                break;
            }
            //cerr << iter->first << endl;
            if (iter->first == _sTheme.substr(0,i))
            {
                return vDocIndexTable[iter->second][0];
            }
            //n++;
        }
    }
    return "NO_ENTRY_FOUND";
}

string Documentation::getHelpArticleTitle(const string& _sIdxKey)
{
    int nIndex = -1;
    map<string,int>::iterator iter = mDocumentationIndex.begin();
    map<string,int>::iterator firstIndex = mDocumentationIndex.end();
    map<string,int>::iterator secondIndex = mDocumentationIndex.end();

    for (unsigned int i = _sIdxKey.length(); i > 0; i--)
    {
        if (firstIndex != mDocumentationIndex.end())
            iter = firstIndex;
        else
            iter = mDocumentationIndex.begin();
        //int n = 0;
        for (; iter != secondIndex; ++iter)
        {
            if (iter->first[0] < _sIdxKey[0])
                continue;
            if (iter->first[0] == _sIdxKey[0] && firstIndex == mDocumentationIndex.end())
                firstIndex = iter;
            if (iter->first[0] > _sIdxKey[0])
            {
                secondIndex = iter;
                break;
            }
            //cerr << iter->first << endl;
            if (iter->first == _sIdxKey.substr(0,i))
            {
                nIndex = iter->second;
                break;
            }
            //n++;
        }
        if (nIndex != -1)
            break;
    }
    if (nIndex == -1)
        return "<<NONE>>";
    return vDocIndexTable[nIndex][2];
}

