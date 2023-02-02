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

#include <iostream>
#include <fstream>
#include <sstream>
#include <windows.h>

#include "filesystem.hpp"
#include "../../kernel.hpp"
#include "../utils/datetimetools.hpp"

std::string removeQuotationMarks(const std::string& sString);


/////////////////////////////////////////////////
/// \brief Default constructor.
/////////////////////////////////////////////////
FileSystem::FileSystem()
{
    sPath = "";
    sExecutablePath = "";
    sValidExtensions = ";.dat;.txt;.tmp;.def;.nscr;.png;.gif;.eps;.bps;.svg;.tex;.labx;.csv;.cache;.ndat;.nprc;.nlng;.nlyt;.log;.plugins;.hlpidx;.nhlp;.jdx;.dx;.jcm;.ibw;.ndb;.ods;.jpg;.bmp;.tga;.bps;.prc;.obj;.xyz;.stl;.json;.off;.pdf;.wav;.wave;.xls;.xlsx;.chm;.h;.hpp;.cxx;.cpp;.c;.m;.tif;.tiff;.ini;.xml;.yaml;.yml;.nsi;.dot;";

    for (int i = 0; i < 7; i++)
    {
        sTokens[i][0] = "";
        sTokens[i][1] = "";
    }
}


/////////////////////////////////////////////////
/// \brief Assignment member function to copy the
/// settings from another FileSystem instance.
///
/// \param _fSys const FileSystem&
/// \return FileSystem&
///
/////////////////////////////////////////////////
FileSystem& FileSystem::assign(const FileSystem& _fSys)
{
    sPath = _fSys.sPath;
    sExecutablePath = _fSys.sExecutablePath;
    sValidExtensions = _fSys.sValidExtensions;

    for (int i = 0; i < 7; i++)
    {
        sTokens[i][0] = _fSys.sTokens[i][0];
        sTokens[i][1] = _fSys.sTokens[i][1];
    }

    return *this;
}


/////////////////////////////////////////////////
/// \brief This function cleans the passed file
/// path, i.e. replaces the character encoding of
/// umlauts, replaces path separators and path
/// placeholders.
///
/// \param sFilePath std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string FileSystem::cleanPath(std::string sFilePath, bool checkInvalidChars) const
{
    sFilePath = replacePathSeparator(removeQuotationMarks(sFilePath));
    StripSpaces(sFilePath);

    if (sFilePath[0] == '<')
    {
        for (int i = 0; i < 7; i++)
        {
            if (sFilePath.substr(0,sTokens[i][0].length()) == sTokens[i][0])
            {
                if (sFilePath[sTokens[i][0].length()] != '/')
                    sFilePath = sTokens[i][1] + "/" + sFilePath.substr(sTokens[i][0].length());
                else
                    sFilePath = sTokens[i][1] + sFilePath.substr(sTokens[i][0].length());

                break;
            }
        }

        if (sFilePath.substr(0,6) == "<this>")
        {
            if (sFilePath[6] != '/')
                sFilePath = sTokens[0][1] + "/" + sFilePath.substr(6);
            else
                sFilePath = sTokens[0][1] + sFilePath.substr(6);
        }
    }

    if (!checkInvalidChars)
        return sFilePath;

    const std::string sINVALID_CHARS = "\"#%&<>|";

    for (unsigned int i = 0; i < sFilePath.length(); i++)
    {
        if (sFilePath[i] == (char)142)
            sFilePath[i] = 'Ä';
        else if (sFilePath[i] == (char)132)
            sFilePath[i] = 'ä';
        else if (sFilePath[i] == (char)153)
            sFilePath[i] = 'Ö';
        else if (sFilePath[i] == (char)148)
            sFilePath[i] = 'ö';
        else if (sFilePath[i] == (char)154)
            sFilePath[i] = 'Ü';
        else if (sFilePath[i] == (char)129)
            sFilePath[i] = 'ü';
        else if (sFilePath[i] == (char)225)
            sFilePath[i] = 'ß';
        else if (sFilePath[i] == '~')
        {
            NumeReKernel::issueWarning("INTERNAL ISSUE: Replaced a tilde character in \"" + sFilePath + "\" with a slash. This should not happen. Consider informing the development team about this warning and how to recreate it. Thank you.");
            sFilePath[i] = '/';
        }
        else if (sINVALID_CHARS.find(sFilePath[i]) != std::string::npos)
        {
            NumeReKernel::issueWarning("Replaced an invalid character in \"" + sFilePath + "\" with an underscore. This should not happen. Consider informing the development team about this warning and how to recreate it. Thank you.");
            sFilePath[i] = '_';
        }
    }

    return sFilePath;
}


/////////////////////////////////////////////////
/// \brief This member function resolves all
/// wildcards, which may be found in the passed
/// filename.
///
/// \param _sFileName std::string&
/// \param isFile bool
/// \param checkExtenstion bool
/// \return void
///
/////////////////////////////////////////////////
void FileSystem::resolveWildCards(std::string& _sFileName, bool isFile, bool checkExtension) const
{
    if (_sFileName.find_first_of("*?") != std::string::npos)
    {
        WIN32_FIND_DATA FindFileData;
        HANDLE hFind = INVALID_HANDLE_VALUE;
        hFind = FindFirstFile(_sFileName.c_str(), &FindFileData);
        std::string sNewFileName = "";

        do
        {
            if (!isFile && FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                sNewFileName = FindFileData.cFileName;
                break;
            }
            else if (isFile && FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            sNewFileName = FindFileData.cFileName;

            if (sNewFileName.length() > 4
                && sNewFileName.find('.') != std::string::npos
                && (!checkExtension || sValidExtensions.find(";"+toLowerCase(sNewFileName.substr(sNewFileName.rfind('.')))+";") != std::string::npos))
                break;
            else if (sNewFileName.find('.') != std::string::npos)
                sNewFileName = "";
            else
                sNewFileName += ".*";
        }
        while (FindNextFile(hFind, &FindFileData) != 0);

        FindClose(hFind);

        if (sNewFileName.length() > 4)
        {
            std::string sPathTemp = _sFileName;

            if (sPathTemp.rfind('/') != std::string::npos && sPathTemp.rfind('\\') != std::string::npos)
            {
                if (sPathTemp.rfind('/') < sPathTemp.rfind('\\'))
                    sPathTemp = sPathTemp.substr(0, sPathTemp.rfind('\\'));
                else
                    sPathTemp = sPathTemp.substr(0, sPathTemp.rfind('/'));
            }
            else if (sPathTemp.rfind('/') != std::string::npos)
            {
                sPathTemp = sPathTemp.substr(0, sPathTemp.rfind('/'));
            }
            else if (sPathTemp.rfind('\\') != std::string::npos)
            {
                sPathTemp = sPathTemp.substr(0, sPathTemp.rfind('\\'));
            }
            else
            {
                sPathTemp = "";
            }

            if (sPathTemp.length())
                _sFileName = sPathTemp + "/" + sNewFileName;
            else
                _sFileName = sNewFileName;
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function creates all
/// missing directories in the passed path.
///
/// \param _sPath const std::string&
/// \return int
///
/////////////////////////////////////////////////
int FileSystem::createFolders(const std::string& _sPath) const
{
    // Create the folder (returns false, if there's more
    // than one folder to be created)
    if (CreateDirectory(_sPath.c_str(), nullptr))
    {
        return 1;
    }

    // If there's more than one folder to be created
    // create them recursively here
    if (GetLastError() == ERROR_PATH_NOT_FOUND)
    {
        for (unsigned int i = 0; i < _sPath.length(); i++)
        {
            if (_sPath[i] == '/' || _sPath[i] == '\\')
            {
                CreateDirectory(_sPath.substr(0,i).c_str(), nullptr);
            }
        }

        CreateDirectory(_sPath.c_str(), nullptr);
    }

    // Note that the folder might already exist
    if (GetLastError() == ERROR_ALREADY_EXISTS)
    {
        return -1;
    }

    return 1;
}


/////////////////////////////////////////////////
/// \brief This member function evaluates,
/// whether the passed filename is a valid
/// filename. One may supply a preferred filename
/// extension.
///
/// \param _sFileName std::string
/// \param sExtension const std::string
/// \param checkExtension bool
/// \return std::string
///
/////////////////////////////////////////////////
std::string FileSystem::ValidFileName(std::string _sFileName, const std::string sExtension, bool checkExtension, bool doCleanPath) const
{
    std::string sValid = "";
    sValidExtensions = toLowerCase(sValidExtensions);

    _sFileName = cleanPath(_sFileName, doCleanPath);

    // Find the position of the last colon in the string
    // should be directly after the drive letter
    unsigned int nPos = _sFileName.find_last_of(':');

    // If there's no colon in the current path, then it is a
    // network address
    if (nPos == std::string::npos)
    {
        if (_sFileName.substr(0, 2) != "//")
            _sFileName = sPath.substr(1, sPath.length()-2) + "/" + _sFileName;
    }

    // Resolve wildcards in the passed file name
    resolveWildCards(_sFileName, true, checkExtension);

    // Find the last dot to identify the extension
    nPos = _sFileName.find_last_of(".");

    // If the position of the last dot is either
    // zero or one, it is a relative path. The
    // consecutive character should be a path
    // separator. In this case, we'll add the
    // default extension
    if (nPos == std::string::npos
        || (nPos == 0 || nPos == 1)
        || (_sFileName.find('/', nPos) != std::string::npos || _sFileName.find('\\', nPos) != std::string::npos))
        sValid = _sFileName + sExtension;
    else if (checkExtension && sExtension.length())
    {
        // Extract the string part after the last
        // dot in the file path
        sValid = _sFileName.substr(nPos);

        // Remove the possible trailing quotation
        // mark from the extension
        if (sValid.back() == '"')
            sValid.pop_back();

        // Ensure that the found extension is valid.
        // Otherwise the extension will be exchanged
        // automatically
        if (sValidExtensions.find(";"+toLowerCase(sValid)+";") != std::string::npos)
        {
            sValid = _sFileName;
        }
        else
        {
            if (sValid == ".*")
                sValid = _sFileName.substr(0,nPos);
            else
                throw SyntaxError(SyntaxError::INVALID_FILETYPE, _sFileName, _sFileName.substr(nPos), _sFileName);
        }
    }
    else
        sValid = _sFileName;

    // It's possible, that a new wildcard was added to the
    // file path. Resolve it here
    resolveWildCards(sValid, true, checkExtension);

    // Ensure that the file path separators are unix-like
    for (unsigned int i = 0; i < sValid.length(); i++)
    {
        if (sValid[i] == '\\')
            sValid[i] = '/';
    }

    return sValid;
}


/////////////////////////////////////////////////
/// \brief This member function evaluates,
/// whether the passed foldername is a valid
/// foldername.
///
/// \param _sFileName std::string
/// \param doCleanPath bool
/// \param appendTrailingSeparator bool
/// \return std::string
///
/////////////////////////////////////////////////
std::string FileSystem::ValidFolderName(std::string _sFileName, bool doCleanPath, bool appendTrailingSeparator) const
{
    _sFileName = cleanPath(_sFileName, doCleanPath);

    // Find the position of the last colon in the string
    // should be directly after the drive letter
    unsigned int nPos = _sFileName.find_last_of(':');

    // If there's no colon in the current path, then it is a
    // network address
    if (nPos == std::string::npos)
    {
        if (_sFileName.substr(0,2) != "//")
            _sFileName = sPath.substr(1, sPath.length()-2) + "/" + _sFileName;
    }

    // Resolve wildcards in the passed file name
    resolveWildCards(_sFileName, false);

    // Fallback for the case that this is actually a
    // file and not a folder
    if (_sFileName.find_first_of("*?") != std::string::npos)
        resolveWildCards(_sFileName, true, false);

    // Ensure that the file path separators are unix-like
    for (unsigned int i = 0; i < _sFileName.length(); i++)
    {
        if (_sFileName[i] == '\\')
            _sFileName[i] = '/';
    }

    // Catch the case, where the file detection may have failed
    if (fileExists(_sFileName))
        return _sFileName;

    // Append a trailing path separator, if it is missing
    if (appendTrailingSeparator && _sFileName.back() != '/')
        _sFileName += "/";
    else if (!appendTrailingSeparator && _sFileName.back() == '/')
        _sFileName.pop_back();

    return _sFileName;
}


/////////////////////////////////////////////////
/// \brief This member function validizes the
/// passed file name and creates the needed
/// folders on-the-fly.
///
/// \param _sFileName const std::string&
/// \param sExtension const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string FileSystem::ValidizeAndPrepareName(const std::string& _sFileName, const std::string& sExtension) const
{
    std::string sValid = ValidFileName(_sFileName, sExtension, sExtension.length());
    createFolders(sValid.substr(0, sValid.rfind('/')));
    return sValid;
}


/////////////////////////////////////////////////
/// \brief This member function may be used to
/// set the preferred file path of the current
/// FileSystem instance.
///
/// \param _sPath std::string
/// \param bMkDir bool
/// \param _sExePath std::string
/// \return int
///
/////////////////////////////////////////////////
int FileSystem::setPath(std::string _sPath, bool bMkDir, std::string _sExePath)
{

    sExecutablePath = fromSystemCodePage(_sExePath);

    if (sExecutablePath[0] == '"')
        sExecutablePath = sExecutablePath.substr(1);

    if (sExecutablePath[sExecutablePath.length()-1] == '"')
        sExecutablePath = sExecutablePath.substr(0,sExecutablePath.length()-1);

    sPath = fromSystemCodePage(_sPath);

    if (sPath.find('<') != std::string::npos)
    {
        for (unsigned int i = 0; i < 6; i++)
        {
            if (sPath.find(sTokens[i][0]) != std::string::npos)
                sPath.replace(sPath.find(sTokens[i][0]), sTokens[i][0].length(), sTokens[i][1]);
        }
    }

    if (sPath.find('~') != std::string::npos)
    {
        for (unsigned int i = 0; i < sPath.length(); i++)
        {
            if (sPath[i] == '~')
                sPath[i] = '/';
        }
    }

    while (sPath.find('\\') != std::string::npos)
        sPath[sPath.find('\\')] = '/';

    if (sPath.find(':') == std::string::npos)
    {
        if (sPath.length() > 3 && sPath.substr(0,3) != "..\\" && sPath.substr(0,3) != "../" && sPath.substr(0,2) != ".\\" && sPath.substr(0,2) != "./")
            sPath = "\"" + sExecutablePath + "\\" + sPath + "\"";
        else if (sPath.length() > 2 && (sPath.substr(0,2) == ".\\" || sPath.substr(0,2) == "./"))
            sPath = "\"" + sExecutablePath + sPath.substr(1) + "\"";
        else if (sPath.length() > 3 && (sPath.substr(0,3) == "..\\" || sPath.substr(0,3) == "../"))
        {
            while (sPath.length() > 3 && (sPath.substr(0,3) == "..\\" || sPath.substr(0,3) == "../"))
            {
                if (sExecutablePath.find('\\') != std::string::npos)
                    sExecutablePath = sExecutablePath.substr(0,sExecutablePath.rfind('\\'));
                else
                {
                    sPath = _sPath;
                    break;
                }

                sPath = sPath.substr(3);
            }

            sPath = "\"" + sExecutablePath + "\\" + sPath + "\"";
        }
        else
            sPath = "\"" + sExecutablePath + "\\" + sPath + "\"";
    }

    if (sPath[0] == '"')
        sPath = sPath.substr(1);

    if (sPath[sPath.length()-1] == '"')
        sPath = sPath.substr(0, sPath.length()-1);


    if (bMkDir)
    {
        int nReturn = createFolders(sPath);
        sPath = "\"" + sPath + "\"";
        return nReturn;
    }

    sPath = "\"" + sPath + "\"";

    return 1;
}


/////////////////////////////////////////////////
/// \brief This member function creates the
/// hidden revisions folders for the version
/// control system.
///
/// \return void
///
/////////////////////////////////////////////////
void FileSystem::createRevisionsFolder()
{
    std::string sRevisionsPath = sPath.substr(1, sPath.length()-2) + "/.revisions";
    createFolders(sRevisionsPath);
    SetFileAttributesA(sRevisionsPath.c_str(), FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
}


/////////////////////////////////////////////////
/// \brief Returns the default path of this
/// FileSystem instance.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string FileSystem::getPath() const
{
    if (sPath[0] == '"' && sPath[sPath.length()-1] == '"')
        return sPath.substr(1,sPath.length()-2);
    return sPath;
}


/////////////////////////////////////////////////
/// \brief This member function separates all
/// path parts into single strings: the drive
/// letter, the path, the name and the extension.
/// This function may also handle folder paths
/// and network paths.
///
/// \param sFilePath const std::string&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> FileSystem::getFileParts(const std::string& sFilePath) const
{
    std::vector<std::string> vFileParts;
    // Create a valid file path first
    std::string sValidName = sFilePath;

    if (isFile(sValidName))
        sValidName = ValidFileName(sValidName, ".dat", false);
    else
        sValidName = ValidFolderName(sValidName);

    // Does it contain a drive letter? Then the second
    // character will always be a colon
    if (sValidName[1] == ':')
    {
        vFileParts.push_back(sValidName.substr(0, 1));
        // extract everything from the fourth character
        // to the last path separator
        if (sValidName.length() > 2)
            vFileParts.push_back(sValidName.substr(3, sValidName.rfind('/') - 3));
    }
    else
    {
        vFileParts.push_back("");
        vFileParts.push_back(sValidName.substr(0, sValidName.rfind('/')));
    }

    // Is it a file or a folder?
    if (sValidName.find('.') != std::string::npos && sValidName.rfind('/')+1 < sValidName.rfind('.'))
    {
        // file
        vFileParts.push_back(sValidName.substr(sValidName.rfind('/')+1, sValidName.rfind('.') - sValidName.rfind('/')-1));
        vFileParts.push_back(sValidName.substr(sValidName.rfind('.')+1));
    }
    else
    {
        // folder
        vFileParts.push_back(sValidName.substr(sValidName.rfind('/')+1));
        vFileParts.push_back("");
    }

    // Return the separated paths
    return vFileParts;
}


/////////////////////////////////////////////////
/// \brief Static function to convert Windows UTC
/// system time to a double.
///
/// \param stUTC SYSTEMTIME
/// \return double
///
/////////////////////////////////////////////////
static double windowSystemTimeToDouble(SYSTEMTIME stUTC)
{
    time_stamp timeStamp;

    //NumeReKernel::print(toString(stUTC.wYear) + "/" + toString(stUTC.wMonth) + "/" + toString(stUTC.wDay) + ", " + toString(stUTC.wHour) + ":" + toString(stUTC.wMinute));
    timeStamp.m_ymd = date::year_month_day(date::year(stUTC.wYear), date::month(stUTC.wMonth), date::day(stUTC.wDay));
    timeStamp.m_hours = std::chrono::hours(stUTC.wHour);
    timeStamp.m_minutes = std::chrono::minutes(stUTC.wMinute);
    timeStamp.m_seconds = std::chrono::seconds(stUTC.wSecond);

    return to_double(getTimePointFromTimeStamp(timeStamp));
}


/////////////////////////////////////////////////
/// \brief Return the file information about the
/// passed file path.
///
/// \param sFilePath const std::string&
/// \return FileInfo
///
/////////////////////////////////////////////////
FileInfo FileSystem::getFileInfo(const std::string& sFilePath) const
{
    FileInfo fInfo;
    std::vector<std::string> vFileParts = getFileParts(sFilePath);

    // Insert the splitted path
    fInfo.drive = vFileParts[0];
    fInfo.path = vFileParts[1];
    fInfo.name = vFileParts[2];
    fInfo.ext = vFileParts[3];

    WIN32_FIND_DATA FindFileData;
    HANDLE hFind = INVALID_HANDLE_VALUE;

    if (isFile(sFilePath))
        hFind = FindFirstFile(ValidFileName(sFilePath, ".dat", false).c_str(), &FindFileData);
    else
        hFind = FindFirstFile(ValidFolderName(sFilePath, true, false).c_str(), &FindFileData);

    // Only fill in the remainig information, if a corresponding
    // file could be found
    if (hFind != INVALID_HANDLE_VALUE)
    {
        // Insert the file information
        LARGE_INTEGER fileSize;
        fileSize.LowPart = FindFileData.nFileSizeLow;
        fileSize.HighPart = FindFileData.nFileSizeHigh;
        fInfo.filesize = size_t(fileSize.QuadPart);

        SYSTEMTIME stUTC;
        FileTimeToSystemTime(&FindFileData.ftCreationTime, &stUTC);
        fInfo.creationTime = windowSystemTimeToDouble(stUTC);
        FileTimeToSystemTime(&FindFileData.ftLastWriteTime, &stUTC);
        fInfo.modificationTime = windowSystemTimeToDouble(stUTC);

        fInfo.fileAttributes = FindFileData.dwFileAttributes;
        FindClose(hFind);
    }

    return fInfo;
}


/////////////////////////////////////////////////
/// \brief This member function may be used to
/// update the path placeholders of the current
/// FileSystem instance.
///
/// \param _sTokens std::string
/// \return void
///
/////////////////////////////////////////////////
void FileSystem::setTokens(std::string _sTokens)
{
    for (int i = 0; i < 7; i++)
    {
        sTokens[i][0] = _sTokens.substr(0,_sTokens.find('='));
        sTokens[i][1] = _sTokens.substr(_sTokens.find('=')+1, _sTokens.find(';')-1-_sTokens.find('='));
        _sTokens = _sTokens.substr(_sTokens.find(';')+1);

        if (!_sTokens.length())
            break;
    }
}


/////////////////////////////////////////////////
/// \brief This function determines, whether a
/// path name indicates a file or a folder.
///
/// \param _sPath const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool FileSystem::isFile(const std::string& _sPath) const
{
    if (fileExists(_sPath))
        return true;

    if (_sPath.rfind('.') != std::string::npos)
    {
        std::string sExt = _sPath.substr(_sPath.rfind('.'));

        if (sValidExtensions.find(";" + sExt + ";") != std::string::npos)
            return true;

        if (sExt.find('/') != std::string::npos || sExt.find('\\') != std::string::npos)
            return false;

        if (sExt.length() < 6 || sExt == ".*")
            return true;

        if (_sPath.find_last_of("\\/", _sPath.length() - sExt.length()) != std::string::npos)
            return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief Member function to remote-initialize
/// the class from the kernel. Cannot be used
/// during kernel start-up.
///
/// \return void
///
/////////////////////////////////////////////////
void FileSystem::initializeFromKernel()
{
    NumeReKernel* _instance = NumeReKernel::getInstance();

    if (!_instance)
        return;

    assign(_instance->getFileSystem());
}

