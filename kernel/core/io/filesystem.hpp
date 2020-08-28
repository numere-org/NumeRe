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
#include <vector>

//#include "error.hpp"

#ifndef FILESYSTEM_HPP
#define FILESYSTEM_HPP

std::string toSystemCodePage(std::string);
std::string fromSystemCodePage(std::string);

/*
 * Headerdatei zur FileSystem Klasse
 * -> FileSystem dient als PARENT fuer Output und Datafile, da die Methode FileSystem::ValidFileName(string) in beiden Klassen benutzt wird
 */

/////////////////////////////////////////////////
/// \brief This class implements the basic input/
/// output file system and provides
/// functionalities to work with filenames and
/// folder paths.
/////////////////////////////////////////////////
class FileSystem
{
    private:
        std::string cleanPath(std::string sFilePath) const;
        void resolveWildCards(std::string& _sFileName, bool isFile) const;
        int createFolders(const std::string& _sPath) const;

	protected:												// In allen CHILD-Klassen verfuegbar
		std::string sPath;										// String-Variable fuer den Dateipfad
		std::string sExecutablePath;
		std::string sTokens[7][2];
		mutable std::string sValidExtensions;

	public:
        FileSystem();

        FileSystem& assign(const FileSystem& _fSys);

		std::string ValidFileName(std::string _sFileName, const std::string sExtension = ".dat", bool checkExtension = true) const;			// gibt einen gueltigen Dateinamen auf Basis von _sFileName zurueck
		std::string ValidFolderName(std::string _sFileName) const;			// gibt einen gueltigen Ordnernamen auf Basis von _sFileName zurueck
		std::string ValidizeAndPrepareName(const std::string& _sFileName, const std::string& sExtension = ".dat") const;
		int setPath(std::string _sPath, bool bMkDir, std::string _sExePath);			// setzt sPath auf _sPath
		void createRevisionsFolder();
		std::string getPath() const;								// gibt sPath zurueck
		std::vector<std::string> getFileParts(const std::string& sFilePath) const;
        inline void setProgramPath(std::string _sExePath)
            {
                sExecutablePath = _sExePath;
            }
        inline std::string getProgramPath() const
            {return sExecutablePath;}
        inline void declareFileType(const std::string& sFileType)
            {
                if (sValidExtensions.find(sFileType) == std::string::npos)
                {
                    if (sFileType[0] == '.')
                        sValidExtensions += sFileType + ";";
                    else
                        sValidExtensions += "." + sFileType + ";";
                }
            }
        void setTokens(std::string _sTokens);
        bool isFile(const std::string& _sPath) const;
        void initializeFromKernel();
};

#endif

