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

#include <fstream>
#include <string>
#include <vector>
#include <map>

#include "../filesystem.hpp"
#include "../ui/error.hpp"

using namespace std;

void StripSpaces(string&);
string getArgAtPos(const string&, unsigned int);
string toUpperCase(const string&);
string toString(int);

/////////////////////////////////////////////////
/// \brief This class handles the documentation
/// index and provides lookup functionalities to
/// obtain the corresponding article.
/////////////////////////////////////////////////
class Documentation : public FileSystem
{
    private:
        fstream fDocument;
        map<string,int> mDocumentationIndex;
        vector<vector<string> > vDocIndexTable;
        string sDocIndexFile;

        void updateIndexFile();
        bool loadIndexFile(const string& sIndexFile);
        int findPositionInDocumentationIndex(const string& sTopic);
        vector<string> loadDocumentationArticle(const string& sFileName);

    public:
        Documentation();
        ~Documentation();

        void loadDocIndex(bool bLoadUserLangFiles = true);
        void updateDocIndex(string _sFilename = "<>/update.hlpidx");
        void addToDocIndex(string& _sIndexToAdd, bool bUseUserLangFiles);
        void removeFromDocIndex(const string& _sID, bool bUseUserLangFiles);
        vector<string> getHelpArticle(const string& sTopic);
        vector<string> getDocIndex();
        string getHelpIdxKey(const string& sTopic);
        string getHelpArtclID(const string& sTopic);
        string getHelpArticleTitle(const string& _sIdxKey);
};

#endif
