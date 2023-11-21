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

typedef void* HANDLE;

/*
 * Headerdatei zur FileSystem Klasse
 * -> FileSystem dient als PARENT fuer Output und Datafile, da die Methode FileSystem::ValidFileName(string) in beiden Klassen benutzt wird
 */

/////////////////////////////////////////////////
/// \brief This structure contains all relevant
/// information about a file path.
/////////////////////////////////////////////////
struct FileInfo
{
    std::string drive;
    std::string path;
    std::string name;
    std::string ext;
    size_t filesize;
    size_t fileAttributes;
    double creationTime;
    double modificationTime;

    enum Attributes
    {
        ATTR_READONLY = 0x1,
        ATTR_HIDDEN = 0x2,
        ATTR_SYSTEM = 0x4,
        ATTR_DIRECTORY = 0x10,
        ATTR_ARCHIVE = 0x20,
        ATTR_DEVICE = 0x40,
        ATTR_NORMAL = 0x80,
        ATTR_TEMPORARY = 0x100,
        ATTR_SPARSE_FILE = 0x200,
        ATTR_REPARSE_POINT = 0x400,
        ATTR_COMPRESSED = 0x800,
        ATTR_OFFLINE = 0x1000,
        ATTR_NOT_CONTENT_INDEXED = 0x2000,
        ATTR_ENCRYPTED = 0x4000,
        ATTR_INTEGRITY_STREAM = 0x8000,
        ATTR_VIRTUAL = 0x10000,
        ATTR_NO_SCRUB_DATA = 0x20000,
        ATTR_RECALL_ON_OPEN = 0x40000,
        ATTR_PINNED = 0x80000,
        ATTR_UNPINNED = 0x100000,
        ATTR_RECALL_ON_DATA_ACCESS = 0x400000
    };

    FileInfo()
    {
        filesize = 0;
        fileAttributes = 0;
        creationTime = 0.0;
        modificationTime = 0.0;
    }
};

/////////////////////////////////////////////////
/// \brief This class implements the basic input/
/// output file system and provides
/// functionalities to work with filenames and
/// folder paths.
/////////////////////////////////////////////////
class FileSystem
{
    private:
        std::string cleanPath(std::string sFilePath, bool checkInvalidChars) const;
        void resolveWildCards(std::string& _sFileName, bool isFile, bool checkExtension = true) const;
        HANDLE initializeFileHandle(std::string& sDir, void* FindFileData) const;
        std::vector<std::string> resolveChooseTokens(const std::string& sDirectory) const;

	protected:
		std::string sPath;
		std::string sExecutablePath;
		std::string sTokens[7][2];
		mutable std::string sValidExtensions;

        int createFolders(const std::string& _sPath) const;

	public:
        FileSystem();
        FileSystem& assign(const FileSystem& _fSys);

        enum
        {
            ONLY_NAME,
            FULLPATH
        };

		std::string ValidFileName(std::string _sFileName, const std::string sExtension = ".dat", bool checkExtension = true, bool doCleanPath = true) const;
		std::string ValidFolderName(std::string _sFileName, bool doCleanPath = true, bool appendTrailingSeparator = true) const;
		std::string ValidizeAndPrepareName(const std::string& _sFileName, const std::string& sExtension = ".dat") const;
		static std::string resolveLink(const std::string& sLink);
		int setPath(std::string _sPath, bool bMkDir, std::string _sExePath);
		void createRevisionsFolder();
		std::string getPath() const;
		std::vector<std::string> getFileList(const std::string& sDirectory, int nFlags = ONLY_NAME) const;
        std::vector<std::string> getFolderList(const std::string& sDirectory, int nFlags = ONLY_NAME) const;
		std::vector<std::string> getFileParts(const std::string& sFilePath) const;
		FileInfo getFileInfo(const std::string& sFilePath) const;
        void setTokens(std::string _sTokens);
        bool isFile(const std::string& _sPath) const;
        void initializeFromKernel();

        inline void setProgramPath(std::string _sExePath)
        {
            sExecutablePath = _sExePath;
        }

        inline std::string getProgramPath() const
        {
            return sExecutablePath;
        }

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
};

#endif

