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


#include "filesystem.hpp"
#include "../kernel.hpp"

string toLowerCase(const string&);

// --> Standard-Konstruktor <--
FileSystem::FileSystem()
{
    sPath = "";
    sWhere = "";
    sValidExtensions = ";.dat;.txt;.tmp;.def;.nscr;.png;.gif;.eps;.svg;.tex;.labx;.csv;.cache;.ndat;.nprc;.nlng;.log;.plugins;.hlpidx;.nhlp;.jdx;.dx;.jcm;.ibw;.ndb;.ods;.jpg;.bmp;.tga;.bps;.prc;.obj;.xyz;.stl;.json;.off;.pdf;.wav;.wave;.xls;.xlsx;.chm;";
    for (int i = 0; i < 7; i++)
    {
        sTokens[i][0] = "";
        sTokens[i][1] = "";
    }
}

// clean the passed file path
string FileSystem::cleanPath(string sFilePath) const
{
    for (size_t i = 0; i < sFilePath.length(); i++)
    {
        if (sFilePath[i] == '\\')
            sFilePath[i] = '/';
    }

    sFilePath.erase(0, sFilePath.find_first_not_of(" \t"));
    if (sFilePath.find_last_not_of(" \t") != string::npos)
        sFilePath.erase(sFilePath.find_last_not_of(" \t")+1);

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

	for (unsigned int i = 0; i < sFilePath.length(); i++)
	{
        if (sFilePath[i] == (char)142)
            sFilePath[i] = '�';
        else if (sFilePath[i] == (char)132)
            sFilePath[i] = '�';
        else if (sFilePath[i] == (char)153)
            sFilePath[i] = '�';
        else if (sFilePath[i] == (char)148)
            sFilePath[i] = '�';
        else if (sFilePath[i] == (char)154)
            sFilePath[i] = '�';
        else if (sFilePath[i] == (char)129)
            sFilePath[i] = '�';
        else if (sFilePath[i] == (char)225)
            sFilePath[i] = '�';
        else
            continue;
	}

	return sFilePath;
}

// Resolve wildcards in paths
void FileSystem::resolveWildCards(string& _sFileName, bool isFile) const
{
    if (_sFileName.find('*') != string::npos || _sFileName.find('?') != string::npos)
	{
        WIN32_FIND_DATA FindFileData;
        HANDLE hFind = INVALID_HANDLE_VALUE;
        hFind = FindFirstFile(_sFileName.c_str(), &FindFileData);
        string sNewFileName = "";

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
                && sNewFileName.find('.') != string::npos
                && sValidExtensions.find(";"+toLowerCase(sNewFileName.substr(sNewFileName.rfind('.')))+";") != string::npos)
                break;
            else if (sNewFileName.find('.') != string::npos)
                sNewFileName = "";
            else if (sNewFileName.find('.') == string::npos)
                sNewFileName += ".*";
        }
        while (FindNextFile(hFind, &FindFileData) != 0);

        FindClose(hFind);

        if (sNewFileName.length() > 4)
        {
            string sPathTemp = _sFileName;

            if (sPathTemp.rfind('/') != string::npos && sPathTemp.rfind('\\') != string::npos)
            {
                if (sPathTemp.rfind('/') < sPathTemp.rfind('\\'))
                    sPathTemp = sPathTemp.substr(0, sPathTemp.rfind('\\'));
                else
                    sPathTemp = sPathTemp.substr(0, sPathTemp.rfind('/'));
            }
            else if (sPathTemp.rfind('/') != string::npos)
            {
                sPathTemp = sPathTemp.substr(0, sPathTemp.rfind('/'));
            }
            else if (sPathTemp.rfind('\\') != string::npos)
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


// --> Pruefe den string _sFileName, ob er als Dateiname verwendet werden kann
string FileSystem::ValidFileName(string _sFileName, const string sExtension, bool checkExtension) const
{
	string sValid = "";
	sValidExtensions = toLowerCase(sValidExtensions);

	_sFileName = cleanPath(_sFileName);

    // Find the position of the last colon in the string
    // should be directly after the drive letter
	unsigned int nPos = _sFileName.find_last_of(':');

	// If there's no colon in the current path, then it is a
	// network address
	if (nPos == string::npos)
	{
	    if (_sFileName.substr(0, 2) != "//")
            _sFileName = sPath.substr(1, sPath.length()-2) + "/" + _sFileName;
    }

    // Resolve wildcards in the passed file name
    resolveWildCards(_sFileName, true);

    // Find the last dot to identify the extension
    nPos = _sFileName.find_last_of(".");

    // If the position of the last dot is either
    // zero or one, it is a relative path. The
    // consecutive character should be a path
    // separator. In this case, we'll add the
    // default extension
	if (nPos == string::npos
        || (nPos == 0 || nPos == 1)
        || (_sFileName.find('/', nPos) != string::npos || _sFileName.find('\\', nPos) != string::npos))
		sValid = _sFileName + sExtension;
	else if (checkExtension)
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
		if (sValidExtensions.find(";"+toLowerCase(sValid)+";") != string::npos)
		{
			sValid = _sFileName;
		}
		else
		{
            if (sValid == ".*")
                sValid = _sFileName.substr(0,nPos);
            else
            {
                NumeReKernel::printPreFmt("|-> WARNUNG: Dieser Datentyp ist unbekannt oder geschuetzt! Die Endung wurde automatisch durch \".dat\" ersetzt!\n");
                sValid = _sFileName.substr(0, nPos) + ".dat";
            }
		}
	}
	else
        sValid = _sFileName;

	// It's possible, that a new wildcard was added to the
	// file path. Resolve it here
	resolveWildCards(sValid, true);

	// Ensure that the file path separators are unix-like
	for (unsigned int i = 0; i < sValid.length(); i++)
	{
        if (sValid[i] == '\\')
            sValid[i] = '/';
	}

	return sValid;
}

// --> Pruefe den string _sFileName, ob er als Dateiname verwendet werden kann
string FileSystem::ValidFolderName(string _sFileName) const
{
	_sFileName = cleanPath(_sFileName);

    // Find the position of the last colon in the string
    // should be directly after the drive letter
	unsigned int nPos = _sFileName.find_last_of(':');

	// If there's no colon in the current path, then it is a
	// network address
	if (nPos == string::npos)
	{
	    if (_sFileName.substr(0,2) != "//")
            _sFileName = sPath.substr(1, sPath.length()-2) + "/" + _sFileName;
    }

    // Resolve wildcards in the passed file name
    resolveWildCards(_sFileName, false);

	// Ensure that the file path separators are unix-like
	for (unsigned int i = 0; i < _sFileName.length(); i++)
	{
        if (_sFileName[i] == '\\')
            _sFileName[i] = '/';
	}

	// Append a trailing path separator, if it is missing
	if (_sFileName.back() != '/')
        _sFileName += "/";
	return _sFileName;
}

int FileSystem::setPath(string _sPath, bool bMkDir, string _sWhere)
{

    sWhere = fromSystemCodePage(_sWhere);
    if (sWhere[0] == '"')
        sWhere = sWhere.substr(1);
    if (sWhere[sWhere.length()-1] == '"')
        sWhere = sWhere.substr(0,sWhere.length()-1);
	sPath = fromSystemCodePage(_sPath);

    //cerr << sWhere << "\\" << sPath << endl;

    if (sPath.find('<') != string::npos)
    {
        for (unsigned int i = 0; i < 6; i++)
        {
            if (sPath.find(sTokens[i][0]) != string::npos)
            {
                sPath.replace(sPath.find(sTokens[i][0]), sTokens[i][0].length(), sTokens[i][1]);
            }
        }
    }
    if (sPath.find('~') != string::npos)
    {
        for (unsigned int i = 0; i < sPath.length(); i++)
        {
            if (sPath[i] == '~')
                sPath[i] = '/';
        }
    }
    while (sPath.find('\\') != string::npos)
        sPath[sPath.find('\\')] = '/';
	if (sPath.find(':') == string::npos)
	{
        if (sPath.length() > 3 && sPath.substr(0,3) != "..\\" && sPath.substr(0,3) != "../" && sPath.substr(0,2) != ".\\" && sPath.substr(0,2) != "./")
            sPath = "\"" + sWhere + "\\" + sPath + "\"";
        else if (sPath.length() > 2 && (sPath.substr(0,2) == ".\\" || sPath.substr(0,2) == "./"))
            sPath = "\"" + sWhere + sPath.substr(1) + "\"";
        else if (sPath.length() > 3 && (sPath.substr(0,3) == "..\\" || sPath.substr(0,3) == "../"))
        {
            while (sPath.length() > 3 && (sPath.substr(0,3) == "..\\" || sPath.substr(0,3) == "../"))
            {
                if (sWhere.find('\\') != string::npos)
                    sWhere = sWhere.substr(0,sWhere.rfind('\\'));
                else
                {
                    sPath = _sPath;
                    break;
                }
                sPath = sPath.substr(3);
            }
            sPath = "\"" + sWhere + "\\" + sPath + "\"";
        }
        else
            sPath = "\"" + sWhere + "\\" + sPath + "\"";
	}
	if (sPath[0] == '"')
        sPath = sPath.substr(1);
    if (sPath[sPath.length()-1] == '"')
        sPath = sPath.substr(0, sPath.length()-1);


	if (bMkDir)
	{
		if (CreateDirectory(sPath.c_str(), NULL))
		{
            sPath = "\"" + sPath + "\"";
            return 1;
		}
		if (GetLastError() == ERROR_PATH_NOT_FOUND)
		{
            for (unsigned int i = 0; i < sPath.length(); i++)
            {
                if (sPath[i] == '/' || sPath[i] == '\\')
                {
                    CreateDirectory(sPath.substr(0,i).c_str(), NULL);
                }
            }
            CreateDirectory(sPath.c_str(), NULL);
		}
        if (GetLastError() == ERROR_ALREADY_EXISTS)
        {
            sPath = "\"" + sPath + "\"";
            return -1;
        }
	}
    sPath = "\"" + sPath + "\"";

	return 1;
}

string FileSystem::getPath() const
{
    if (sPath[0] == '"' && sPath[sPath.length()-1] == '"')
        return sPath.substr(1,sPath.length()-2);
    return sPath;
}

// C:/Software/NumeRe/numere.exe -> C, Software/NumeRe, numere, exe
vector<string> FileSystem::getFileParts(const string& sFilePath) const
{
    vector<string> vFileParts;
    // Create a valid file path first
    string sValidName = sFilePath;

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
        vFileParts.push_back(sValidName.substr(3, sValidName.rfind('/') - 3));
    }
    else
    {
        vFileParts.push_back("");
        vFileParts.push_back(sValidName.substr(0, sValidName.rfind('/')));
    }

    // Is it a file or a folder?
    if (sValidName.find('.') != string::npos)
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

void FileSystem::setTokens(string _sTokens)
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

// Determine, whether a path name indicates a file or a folder
bool FileSystem::isFile(const string& _sPath) const
{
    if (fileExists(_sPath))
        return true;

    if (_sPath.rfind('.') != string::npos)
    {
        string sExt = _sPath.substr(_sPath.rfind('.'));

        if (sValidExtensions.find(";" + sExt + ";") != string::npos)
            return true;

        if (sExt.find('/') != string::npos || sExt.find('\\') != string::npos)
            return false;

        if (sExt.length() < 6 || sExt == ".*")
            return true;

        if (_sPath.find_last_of("\\/", _sPath.length() - sExt.length()) != string::npos)
            return true;
    }

    return false;
}

