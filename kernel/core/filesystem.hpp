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


#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <windows.h>
#include <vector>

//#include "error.hpp"

#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP
using namespace std;

string toSystemCodePage(string);
string fromSystemCodePage(string);

/*
 * Headerdatei zur FileSystem Klasse
 * -> FileSystem dient als PARENT fuer Output und Datafile, da die Methode FileSystem::ValidFileName(string) in beiden Klassen benutzt wird
 */

class FileSystem
{
    private:
        string cleanPath(string sFilePath) const;
        void resolveWildCards(string& _sFileName, bool isFile) const;
        int createFolders(const string& _sPath) const;

	protected:												// In allen CHILD-Klassen verfuegbar
		string sPath;										// String-Variable fuer den Dateipfad
		string sWhere;
		string sTokens[7][2];
		mutable string sValidExtensions;

	public:
        FileSystem();

		string ValidFileName(string _sFileName, const string sExtension = ".dat", bool checkExtension = true) const;			// gibt einen gueltigen Dateinamen auf Basis von _sFileName zurueck
		string ValidFolderName(string _sFileName) const;			// gibt einen gueltigen Ordnernamen auf Basis von _sFileName zurueck
		string ValidizeAndPrepareName(const string& _sFileName, const string& sExtension = ".dat") const;
		int setPath(string _sPath, bool bMkDir, string _sWhere);			// setzt sPath auf _sPath
		void createRevisionsFolder();
		string getPath() const;								// gibt sPath zurueck
		vector<string> getFileParts(const string& sFilePath) const;
        inline void setProgramPath(string _sWhere)
            {
                sWhere = _sWhere;
                return;
            }
        inline string getProgramPath() const
            {return sWhere;}
        inline void declareFileType(const string& sFileType)
            {
                if (sValidExtensions.find(sFileType) == string::npos)
                {
                    if (sFileType[0] == '.')
                        sValidExtensions += sFileType + ";";
                    else
                        sValidExtensions += "." + sFileType + ";";
                }
                return;
            }
        void setTokens(string _sTokens);
        bool isFile(const string& _sPath) const;
};

#endif
