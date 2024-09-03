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
#include <shobjidl.h>

#include "filesystem.hpp"
#ifndef PARSERSTANDALONE
#include "../../kernel.hpp"
#else
#include "../utils/tools.hpp"
#endif

std::string removeQuotationMarks(const std::string& sString);



/////////////////////////////////////////////////
/// \brief Default constructor.
/////////////////////////////////////////////////
FileSystem::FileSystem()
{
    sPath = "";
    sExecutablePath = "";
    sValidExtensions = ";.dat;.txt;.tmp;.def;.nscr;.png;.gif;.eps;.bps;.svg;.tex;.labx;.csv;.cache;.ndat;.nprc;.nlng;.nlyt;.log;.plugins;.hlpidx;.nhlp;.jdx;.dx;.jcm;.ibw;.ndb;.ods;.jpg;.bmp;.tga;.bps;.prc;.obj;.xyz;.stl;.json;.off;.pdf;.wav;.wave;.xls;.xlsx;.chm;.h;.hpp;.cxx;.cpp;.c;.m;.tif;.tiff;.ini;.xml;.yaml;.yml;.nsi;.dot;.zip;.tar;.gz;";

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
            if (sFilePath.starts_with(sTokens[i][0]))
            {
                if (sFilePath[sTokens[i][0].length()] != '/')
                    sFilePath = sTokens[i][1] + "/" + sFilePath.substr(sTokens[i][0].length());
                else
                    sFilePath = sTokens[i][1] + sFilePath.substr(sTokens[i][0].length());

                break;
            }
        }

        if (sFilePath.starts_with("<this>"))
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

    for (size_t i = 0; i < sFilePath.length(); i++)
    {
        if (sFilePath[i] == (char)142)
            sFilePath[i] = (char)0xC4;
        else if (sFilePath[i] == (char)132)
            sFilePath[i] = (char)0xE4;
        else if (sFilePath[i] == (char)153)
            sFilePath[i] = (char)0xD6;
        else if (sFilePath[i] == (char)148)
            sFilePath[i] = (char)0xF6;
        else if (sFilePath[i] == (char)154)
            sFilePath[i] = (char)0xDC;
        else if (sFilePath[i] == (char)129)
            sFilePath[i] = (char)0xFC;
        else if (sFilePath[i] == (char)225)
            sFilePath[i] = (char)0xDF;
        else if (sFilePath[i] == '~')
        {
#ifndef PARSERSTANDALONE
            NumeReKernel::issueWarning("INTERNAL ISSUE: Replaced a tilde character in \"" + sFilePath + "\" with a slash. This should not happen. Consider informing the development team about this warning and how to recreate it. Thank you.");
#endif
            sFilePath[i] = '/';
        }
        else if (sINVALID_CHARS.find(sFilePath[i]) != std::string::npos)
        {
#ifndef PARSERSTANDALONE
            NumeReKernel::issueWarning("Replaced an invalid character in \"" + sFilePath + "\" with an underscore. This should not happen. Consider informing the development team about this warning and how to recreate it. Thank you.");
#endif
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

        if (hFind == INVALID_HANDLE_VALUE)
            return;

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
                sPathTemp = sPathTemp.substr(0, sPathTemp.rfind('/'));
            else if (sPathTemp.rfind('\\') != std::string::npos)
                sPathTemp = sPathTemp.substr(0, sPathTemp.rfind('\\'));
            else
                sPathTemp = "";

            if (sPathTemp.length())
                _sFileName = sPathTemp + "/" + sNewFileName;
            else
                _sFileName = sNewFileName;
        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function is a helper
/// function for getFileList and getFolderList.
///
/// \param sDir std::string&
/// \param FindFileData void*
/// \return HANDLE
///
/////////////////////////////////////////////////
HANDLE FileSystem::initializeFileHandle(std::string& sDir, void* FindFileData) const
{
    HANDLE hFind = INVALID_HANDLE_VALUE;

    // Initialize the Windows structures
    if (sDir[0] == '.')
    {
        // Only a dot -> root path
        hFind = FindFirstFile((sExecutablePath + "\\" + sDir).c_str(), (WIN32_FIND_DATA*)FindFileData);
        sDir = replacePathSeparator(sExecutablePath + "/" + sDir);
        sDir.erase(sDir.rfind('/') + 1);
    }
    else if (sDir[0] == '<')
    {
        // Get the default paths
        sDir = cleanPath(sDir, false);

        // If the path has a length then initialize the file handle
        if (sDir.length())
        {
            hFind = FindFirstFile(sDir.c_str(), (WIN32_FIND_DATA*)FindFileData);
            sDir = replacePathSeparator(sDir);
            sDir.erase(sDir.rfind('/') + 1);
        }
    }
    else
    {
        // an arbitrary path
        hFind = FindFirstFile(sDir.c_str(), (WIN32_FIND_DATA*)FindFileData);
        if (sDir.find('/') != std::string::npos)
            sDir.erase(sDir.rfind('/') + 1);
    }

    // return the initialized file handle
    return hFind;
}


/////////////////////////////////////////////////
/// \brief This function resolves the possibility
/// to select multiple paths at once by inserting
/// something like this/is/a/<path|with|tokens>/which/will/search/at/different/locations
///
/// \param sDirectory const std::string&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> FileSystem::resolveChooseTokens(const std::string& sDirectory) const
{
    std::vector<std::string> vResolved;
    vResolved.push_back(sDirectory);
    std::string sToken;
    size_t nSize = 0, nth_choose = 0;
    bool bResolvingPath = false;

    // Is there at least one pipe in the directory?
    if (sDirectory.find('|') != std::string::npos)
    {
        // As long as the directory contains pipes
        while (vResolved[0].find('|') != std::string::npos)
        {
            // no opening angle for the token?
            if (!vResolved[0].rfind('<'))
                break;

            // Get the token and remove the remaining part
            sToken = vResolved[0].substr(vResolved[0].rfind('<') + 1);
            sToken.erase(sToken.find('>'));

            // Store the current size of the directory tree
            nSize = vResolved.size();
            nth_choose = 0;

            // As long as a pipe is found in the token or the token has a length
            while (sToken.find('|') != std::string::npos || sToken.length())
            {
                // so lange ein "|" in dem Token gefunden wird, muss der Baum dupliziert werden
                if (sToken.find('|') != std::string::npos)
                {
                    // duplicate the "root" tree
                    for (size_t i = 0; i < nSize; i++)
                        vResolved.push_back(vResolved[i + nth_choose * nSize]);
                }

                // Replace the tokens with the first of the current tokens
                for (size_t i = nth_choose * nSize; i < (nth_choose + 1)*nSize; i++)
                {
                    if (!bResolvingPath && vResolved[i].rfind('/') != std::string::npos && vResolved[i].rfind('/') > vResolved[i].rfind('>'))
                        bResolvingPath = true;
                    vResolved[i].replace(vResolved[i].rfind('<'), vResolved[i].rfind('>') + 1 - vResolved[i].rfind('<'), sToken.substr(0, sToken.find('|')));
                }

                // If we want to resolve a path, then we have to do that with a recursion
                if (bResolvingPath
                        && ((vResolved[nth_choose * nSize].find('*') != std::string::npos && vResolved[nth_choose * nSize].find('*') < vResolved[nth_choose * nSize].rfind('/'))
                            || (vResolved[nth_choose * nSize].find('?') != std::string::npos && vResolved[nth_choose * nSize].find('?') < vResolved[nth_choose * nSize].rfind('/'))))
                {
                    // Platzhalter in Pfaden werden mit einer Rekursion geloest.
                    // Resolve the current tree
                    std::vector<std::string> vFolderList = getFolderList(vResolved[nth_choose * nSize].substr(0, vResolved[nth_choose * nSize].rfind('/')), FULLPATH);

                    // Remove obsolete paths (i.e. paths pointing to itself or to one directory further up
                    for (size_t j = 0; j < vFolderList.size(); j++)
                    {
                        if ((vFolderList[j].length() >= 3 && vFolderList[j].substr(vFolderList[j].length() - 3) == "/..")
                            || (vFolderList[j].length() >= 2 && vFolderList[j].substr(vFolderList[j].length() - 2) == "/."))
                        {
                            vFolderList.erase(vFolderList.begin() + j);

                            // If we erase the current position, we have to decrement the position
                            j--;
                        }
                    }

                    // If we didn't get a result, remove the current token from the token list and continue
                    if (!vFolderList.size())
                    {
                        bResolvingPath = false;
                        nth_choose++;
                        if (sToken.find('|') != std::string::npos)
                            sToken.erase(0, sToken.find('|') + 1);
                        else
                        {
                            sToken.clear();
                            break;
                        }
                        continue;
                    }

                    // Copy the obtained tree to the resolved tree
                    for (size_t j = 0; j < vFolderList.size(); j++)
                    {
                        // Does the tree need to be duplicated?
                        if (vFolderList.size() > 1 && j < vFolderList.size() - 1)
                        {
                            // ggf. Baum duplizieren
                            if (vResolved.size() > (nth_choose + 1)*nSize)
                            {
                                for (size_t k = 0; k < nSize; k++)
                                {
                                    vResolved.push_back(vResolved[k + (nth_choose + 1)*nSize]);
                                    vResolved[k + (nth_choose + 1)*nSize] = vResolved[k + nth_choose * nSize];
                                }
                            }
                            else
                            {
                                for (size_t k = 0; k < nSize; k++)
                                {
                                    vResolved.push_back(vResolved[(nth_choose)*nSize]);
                                }
                            }
                        }

                        // simply replace the path part of the resolved tree
                        for (size_t k = nth_choose * nSize; k < (nth_choose + 1)*nSize; k++)
                        {
                            vResolved[k].replace(0, vResolved[k].rfind('/'), vFolderList[j]);
                        }

                        // Increment the choose token counter
                        if (vFolderList.size() > 1 && j < vFolderList.size() - 1)
                            nth_choose++;
                    }
                }

                // Erase the current token from the token list and continue
                bResolvingPath = false;
                nth_choose++;
                if (sToken.find('|') != std::string::npos)
                    sToken.erase(0, sToken.find('|') + 1);
                else
                {
                    sToken.clear();
                    break;
                }
            }
        }
    }

    // This is not using path tokens but place holders/wildcards in the path part
    if (vResolved[0].find('/') != std::string::npos
            && ((vResolved[0].find('*') != std::string::npos && vResolved[0].find('*') < vResolved[0].rfind('/'))
                || (vResolved[0].find('?') != std::string::npos && vResolved[0].find('?') < vResolved[0].rfind('/'))))
    {
        // Platzhalter in Pfaden werden mit einer Rekursion geloest.
        std::vector<std::string> vFolderList = getFolderList(vResolved[0].substr(0, vResolved[0].rfind('/')), FULLPATH);

        // store the current tree size
        nSize = vResolved.size();

        // Remove obsolete paths (i.e. paths pointing to itself or to one directory further up
        for (size_t j = 0; j < vFolderList.size(); j++)
        {
            if ((vFolderList[j].length() >= 3 && vFolderList[j].substr(vFolderList[j].length() - 3) == "/..")
                || (vFolderList[j].length() >= 2 && vFolderList[j].substr(vFolderList[j].length() - 2) == "/."))
            {
                vFolderList.erase(vFolderList.begin() + j);

                // If we erase the current position, we have to decrement the position
                j--;
            }
        }

        // Return, if no result was found
        if (!vFolderList.size())
            return vResolved;

        // Copy the resolved tree, if it is necessary
        for (size_t i = 0; i < vFolderList.size() - 1; i++)
        {
            // Don't use paths, which weren't resolved
            if (vFolderList[i].find('*') != std::string::npos || vFolderList[i].find('?') != std::string::npos || !vFolderList[i].size())
                continue;

            // ggf. Baum duplizieren
            for (size_t k = 0; k < nSize; k++)
            {
                vResolved.push_back(vResolved[k]);
            }

        }

        // Replace the paths with wildcards with the results obtained by recursion
        for (size_t j = 0; j < vFolderList.size(); j++)
        {
            // Don't use paths, which weren't resolved
            if (vFolderList[j].find('*') != std::string::npos || vFolderList[j].find('?') != std::string::npos || !vFolderList[j].size())
                continue;

            // replace the paths in the resolved tree
            for (size_t k = j * nSize; k < (j + 1)*nSize; k++)
            {
                vResolved[k].replace(0, vResolved[k].rfind('/'), vFolderList[j]);
            }
        }
    }

    // return the resolved tree
    return vResolved;
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
        for (size_t i = 0; i < _sPath.length(); i++)
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
    size_t nPos = _sFileName.find_last_of(':');

    // If there's no colon in the current path, then it is a
    // network address
    if (nPos == std::string::npos)
    {
        if (!_sFileName.starts_with("//"))
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
            sValid = _sFileName;
        else
        {
            if (sValid == ".*")
                sValid = _sFileName.substr(0,nPos);
            else
#ifndef PARSERSTANDALONE
                throw SyntaxError(SyntaxError::INVALID_FILETYPE, _sFileName, _sFileName.substr(nPos), _sFileName);
#else
                throw std::runtime_error("INVALID_FILETYPE");
#endif
        }
    }
    else
        sValid = _sFileName;

    // It's possible, that a new wildcard was added to the
    // file path. Resolve it here
    resolveWildCards(sValid, true, checkExtension);

    // Ensure that the file path separators are unix-like
    for (size_t i = 0; i < sValid.length(); i++)
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
    size_t nPos = _sFileName.find_last_of(':');

    // If there's no colon in the current path, then it is a
    // network address
    if (nPos == std::string::npos)
    {
        if (!_sFileName.starts_with("//"))
            _sFileName = sPath.substr(1, sPath.length()-2) + "/" + _sFileName;
    }

    // Resolve wildcards in the passed file name
    resolveWildCards(_sFileName, false);

    // Fallback for the case that this is actually a
    // file and not a folder
    if (_sFileName.find_first_of("*?") != std::string::npos)
        resolveWildCards(_sFileName, true, false);

    // Ensure that the file path separators are unix-like
    for (size_t i = 0; i < _sFileName.length(); i++)
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
/// \brief Resolve a Windows shell link (*.lnk).
/// Copied and adapted from MSDN.
///
/// \param sLink const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string FileSystem::resolveLink(const std::string& sLink)
{
#warning FIXME (numere#9#10/31/23): It seems that TDM-GCC 9.2.0 lacks the necessary declarations
#ifdef NR_HAVE_GSL2
    HRESULT hres;
    IShellLink* psl;
    CHAR szGotPath[MAX_PATH];
    WIN32_FIND_DATA wfd;

    std::string sFilePath;

    CoInitialize(nullptr);

    // Get a pointer to the IShellLink interface. It is assumed that CoInitialize
    // has already been called.
    hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLinkA, (LPVOID*)&psl);

    if (SUCCEEDED(hres))
    {
        IPersistFile* ppf;

        // Get a pointer to the IPersistFile interface.
        hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);

        if (SUCCEEDED(hres))
        {
            WCHAR wsz[MAX_PATH];

            // Ensure that the string is Unicode.
            MultiByteToWideChar(CP_ACP, 0, sLink.c_str(), -1, wsz, MAX_PATH);

            // Add code here to check return value from MultiByteWideChar
            // for success.

            // Load the shortcut.
            hres = ppf->Load(wsz, STGM_READ);

            if (SUCCEEDED(hres))
            {
                // Resolve the link.
                hres = psl->Resolve(nullptr, SLR_NO_UI);

                if (SUCCEEDED(hres))
                {
                    // Get the path to the link target.
                    hres = psl->GetPath(szGotPath, MAX_PATH, (WIN32_FIND_DATA*)&wfd, SLGP_SHORTPATH);

                    if (SUCCEEDED(hres))
                        sFilePath = szGotPath;
                }
            }

            // Release the pointer to the IPersistFile interface.
            ppf->Release();
        }

        // Release the pointer to the IShellLink interface.
        psl->Release();
    }

    CoUninitialize();
    return sFilePath;
#else
    NumeReKernel::issueWarning("Feature not supported in x86 build.");
    return sLink;
#endif
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
        for (size_t i = 0; i < 6; i++)
        {
            if (sPath.find(sTokens[i][0]) != std::string::npos)
                sPath.replace(sPath.find(sTokens[i][0]), sTokens[i][0].length(), sTokens[i][1]);
        }
    }

    if (sPath.find('~') != std::string::npos)
    {
        for (size_t i = 0; i < sPath.length(); i++)
        {
            if (sPath[i] == '~')
                sPath[i] = '/';
        }
    }

    while (sPath.find('\\') != std::string::npos)
        sPath[sPath.find('\\')] = '/';

    if (sPath.find(':') == std::string::npos)
    {
        if (sPath.length() > 3
            && !sPath.starts_with("..\\")
            && !sPath.starts_with("../")
            && !sPath.starts_with(".\\")
            && !sPath.starts_with("./"))
            sPath = "\"" + sExecutablePath + "\\" + sPath + "\"";
        else if (sPath.length() > 2 && (sPath.starts_with(".\\") || sPath.starts_with("./")))
            sPath = "\"" + sExecutablePath + sPath.substr(1) + "\"";
        else if (sPath.length() > 3 && (sPath.starts_with("..\\") || sPath.starts_with("../")))
        {
            while (sPath.length() > 3 && (sPath.starts_with("..\\") || sPath.starts_with("../")))
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

    if (sPath.front() == '"')
        sPath.erase(0, 1);

    if (sPath.back() == '"')
        sPath.pop_back();


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
/// \brief This function returns a list of files
/// (including their paths, if nFlags & FULLPATH).
///
/// \param sDirectory const std::string&
/// \param nFlags int
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> FileSystem::getFileList(const std::string& sDirectory, int nFlags) const
{
    std::vector<std::string> vFileList;
    std::vector<std::string> vDirList;

    // Replace the Windows-Style Path separators to Unix-Style
    std::string sDir = replacePathSeparator(sDirectory);

    // Get the resolved tree
    vDirList = resolveChooseTokens(sDir);

    // Walk through the resolved tree
    for (size_t i = 0; i < vDirList.size(); i++)
    {
        sDir = vDirList[i];

        // Append a wildcard, if one is missing
        if (sDir.rfind('.') == std::string::npos && sDir.find('*') == std::string::npos && sDir.find('?') == std::string::npos)
        {
            if (sDir[sDir.find_last_not_of(' ')] != '/')
                sDir += '/';
            sDir += "*";
        }
        else if ((sDir.find('.') == std::string::npos
                  || (sDir.find('.') != std::string::npos && sDir.find('/', sDir.rfind('.')) != std::string::npos))
                 && sDir.back() != '*')
            sDir += "*";

        // Declare the Windows structures;
        WIN32_FIND_DATA FindFileData;
        HANDLE hFind = initializeFileHandle(sDir, &FindFileData);

        // Ensure that the structures were initialized correctly
        if (hFind == INVALID_HANDLE_VALUE)
            continue;

        // As long as the FindNextFile function returns non-zero
        // read the contents of FindFileData
        do
        {
            // Ignore directories
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            if (std::string(FindFileData.cFileName).ends_with(".lnk"))
            {
                // Handle links
                std::string sLinkTarget = replacePathSeparator(resolveLink(sDir + FindFileData.cFileName));
                FileInfo info = getFileInfo(sLinkTarget);

                if (info.fileAttributes & FileInfo::ATTR_DIRECTORY)
                {
                    // Push back the link file
                    if (nFlags & FULLPATH)
                        vFileList.push_back(sDir + FindFileData.cFileName);
                    else
                        vFileList.push_back(FindFileData.cFileName);
                }
                else
                {
                    // Push back the target file
                    if (nFlags & FULLPATH)
                        vFileList.push_back(sLinkTarget);
                    else
                        vFileList.push_back(info.name + (info.ext.length() ? "." + info.ext : std::string("")));
                }
            }
            else
            {
                // Push back filenames
                if (nFlags & FULLPATH)
                    vFileList.push_back(sDir + FindFileData.cFileName);
                else
                    vFileList.push_back(FindFileData.cFileName);
            }
        }
        while (FindNextFile(hFind, &FindFileData) != 0);

        // Close the handle
        FindClose(hFind);
    }

    // Return the obtained file list
    return vFileList;
}


/////////////////////////////////////////////////
/// \brief This function returns a list of
/// directories (including their paths, if
/// nFlags & FULLPATH).
///
/// \param sDirectory const std::string&
/// \param nFlags int
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> FileSystem::getFolderList(const std::string& sDirectory, int nFlags) const
{
    std::vector<std::string> vFileList;
    std::vector<std::string> vDirList;

    // Replace the Windows-Style Path separators to Unix-Style
    std::string sDir = replacePathSeparator(sDirectory);

    // Get the resolved tree
    vDirList = resolveChooseTokens(sDir);

    // Walk through the resolved tree
    for (size_t i = 0; i < vDirList.size(); i++)
    {
        sDir = vDirList[i];

        // Append a wildcard, if one is missing
        if (sDir.rfind('.') == std::string::npos && sDir.find('*') == std::string::npos && sDir.find('?') == std::string::npos)
        {
            if (sDir[sDir.find_last_not_of(' ')] != '/')
                sDir += '/';
            sDir += "*";
        }
        else if ((sDir.find('.') == std::string::npos
                  || (sDir.find('.') != std::string::npos && sDir.find('/', sDir.rfind('.')) != std::string::npos))
                 && sDir.back() != '*')
            sDir += "*";

        // Declare the Windows structures;
        WIN32_FIND_DATA FindFileData;
        HANDLE hFind = initializeFileHandle(sDir, &FindFileData);

        // Ensure that the structures were initialized correctly
        if (hFind == INVALID_HANDLE_VALUE)
            continue;

        // As long as the FindNextFile function returns non-zero
        // read the contents of FindFileData
        do
        {
            // USe directories
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // Push back the directories
                if (nFlags & FULLPATH)
                    vFileList.push_back(sDir + FindFileData.cFileName);
                else
                    vFileList.push_back(FindFileData.cFileName);
            }
            else if (std::string(FindFileData.cFileName).ends_with(".lnk"))
            {
                // Handle links
                std::string sLinkTarget = replacePathSeparator(resolveLink(sDir + FindFileData.cFileName));
                FileInfo info = getFileInfo(sLinkTarget);

                if (info.fileAttributes & FileInfo::ATTR_DIRECTORY)
                {
                    // Push back the target file
                    if (nFlags & FULLPATH)
                        vFileList.push_back(sLinkTarget);
                    else
                        vFileList.push_back(sLinkTarget.substr(sLinkTarget.rfind('/')+1));
                }
            }
            else // ignore files
                continue;

        }
        while (FindNextFile(hFind, &FindFileData) != 0);

        // Close the handle
        FindClose(hFind);
    }

    // Return the obtained file list
    return vFileList;
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
/// system time to a sys_time_point.
///
/// \param stUTC SYSTEMTIME
/// \return sys_time_point
///
/////////////////////////////////////////////////
static sys_time_point windowSystemTimeToTimePoint(SYSTEMTIME stUTC)
{
    time_stamp timeStamp;

    //NumeReKernel::print(toString(stUTC.wYear) + "/" + toString(stUTC.wMonth) + "/" + toString(stUTC.wDay) + ", " + toString(stUTC.wHour) + ":" + toString(stUTC.wMinute));
    timeStamp.m_ymd = date::year_month_day(date::year(stUTC.wYear), date::month(stUTC.wMonth), date::day(stUTC.wDay));
    timeStamp.m_hours = std::chrono::hours(stUTC.wHour);
    timeStamp.m_minutes = std::chrono::minutes(stUTC.wMinute);
    timeStamp.m_seconds = std::chrono::seconds(stUTC.wSecond);

    return getTimePointFromTimeStamp(timeStamp);
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
        fInfo.creationTime = windowSystemTimeToTimePoint(stUTC);
        FileTimeToSystemTime(&FindFileData.ftLastWriteTime, &stUTC);
        fInfo.modificationTime = windowSystemTimeToTimePoint(stUTC);

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
/// \brief Return the value of a selected path
/// token.
///
/// \param _sToken const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string FileSystem::getTokenValue(const std::string& _sToken) const
{
    for (int i = 0; i < 7; i++)
    {
        if (sTokens[i][0] == '<' + _sToken + '>')
            return sTokens[i][1];
    }

    return _sToken;
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
#ifndef PARSERSTANDALONE
    NumeReKernel* _instance = NumeReKernel::getInstance();

    if (!_instance)
        return;

    assign(_instance->getFileSystem());
#endif
}

