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

string toLowerCase(const string&);

// --> Standard-Konstruktor <--
FileSystem::FileSystem()
{
    sPath = "";
    sWhere = "";
    sValidExtensions = ";.dat;.txt;.tmp;.def;.nscr;.png;.gif;.eps;.svg;.tex;.labx;.csv;.cache;.ndat;.nprc;.nlng;.log;.plugins;.hlpidx;.nhlp;.jdx;.dx;.jcm;.ibw;.ndb;.ods;.jpg;.bmp;.tga;.bps;.prc;.obj;.xyz;.stl;.json;.off;.pdf;.wav;.wave;";
    for (int i = 0; i < 6; i++)
    {
        sTokens[i][0] = "";
        sTokens[i][1] = "";
    }
}

// --> Pruefe den string _sFileName, ob er als Dateiname verwendet werden kann
string FileSystem::ValidFileName(string _sFileName, const string sExtension)
{
	string sValid = "";			// Variable fuer den gueltigen Dateinamen
	sValidExtensions = toLowerCase(sValidExtensions);

    while (_sFileName.find('\\') != string::npos)
        _sFileName[_sFileName.find('\\')] = '/';

    while (_sFileName.front() == ' ')
    {
        _sFileName.erase(0,1);
    }
    while (_sFileName.back() ==  ' ')
        _sFileName.pop_back();
	if (_sFileName[0] == '<')
	{
        for (int i = 0; i < 6; i++)
        {
            if (_sFileName.substr(0,sTokens[i][0].length()) == sTokens[i][0])
            {
                if (_sFileName[sTokens[i][0].length()] != '/')
                    _sFileName = sTokens[i][1] + "/" + _sFileName.substr(sTokens[i][0].length());
                else
                    _sFileName = sTokens[i][1] + _sFileName.substr(sTokens[i][0].length());
                break;
            }
        }
        if (_sFileName.substr(0,6) == "<this>")
        {
            if (_sFileName[6] != '/')
                _sFileName = sTokens[0][1] + "/" + _sFileName.substr(6);
            else
                _sFileName = sTokens[0][1] + _sFileName.substr(6);
        }
	}

	for (unsigned int i = 0; i < _sFileName.length(); i++)
	{
        if (_sFileName[i] == (char)142)
            _sFileName[i] = 'Ä';
        else if (_sFileName[i] == (char)132)
            _sFileName[i] = 'ä';
        else if (_sFileName[i] == (char)153)
            _sFileName[i] = 'Ö';
        else if (_sFileName[i] == (char)148)
            _sFileName[i] = 'ö';
        else if (_sFileName[i] == (char)154)
            _sFileName[i] = 'Ü';
        else if (_sFileName[i] == (char)129)
            _sFileName[i] = 'ü';
        else if (_sFileName[i] == (char)225)
            _sFileName[i] = 'ß';
        else
            continue;
	}

	unsigned int nPos = _sFileName.find_last_of("/");	// Suchen wir mal nach eventuellen relativen Pfadangaben.
	if (nPos == string::npos)						// Nichts gefunden...? Dann nehmen wir den default
	{
		nPos = _sFileName.find_last_of("\\");
		if (nPos == string::npos)
		{
			_sFileName = sPath.substr(1,sPath.length()-2) + "/" + _sFileName;
		}
	}


	if (_sFileName.find('*') != string::npos || _sFileName.find('?') != string::npos)
	{
        WIN32_FIND_DATA FindFileData;
        HANDLE hFind = INVALID_HANDLE_VALUE;
        //LARGE_INTEGER Filesize;
        hFind = FindFirstFile(_sFileName.c_str(), &FindFileData);
        //cerr << _sFileName << endl;
        string sNewFileName = "";
        do
        {
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;
            sNewFileName = FindFileData.cFileName;
            //cerr << sNewFileName << endl;
            //cerr << sNewFileName.substr(sNewFileName.rfind('.')) << endl;
            if (sNewFileName.length() > 4
                && sNewFileName.find('.') != string::npos
                && sValidExtensions.find(";"+toLowerCase(sNewFileName.substr(sNewFileName.rfind('.')))+";") != string::npos)
                break;
            else
                sNewFileName = "";
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

    //cerr << _sFileName << endl;

    nPos = _sFileName.find_last_of("."); // Suche nach dem letzten Auftreten eines Punkts in dem Dateinamen

								// nPos == 0 || nPos == 1 sind fuer relative Pfadangaben TRUE. Zur Sicherheit pruefen wir dann noch das
								//		folgende Zeichen: Sollte dann / oder \ sein
	if (nPos == string::npos
        || (nPos == 0 || nPos == 1)
        || (_sFileName.find('/', nPos) != string::npos || _sFileName.find('\\', nPos) != string::npos))
		sValid = _sFileName + sExtension; // Keiner gefunden? Kein Problem. Haengen wir einfach ".dat" an.
	else						// Einen gefunden...? Oje...
	{
		sValid = _sFileName.substr(nPos); // Genauer anschauen. Wie sieht der substr denn aus?
		if (sValid[sValid.length()-1] == '"')
            sValid = sValid.substr(0,sValid.length()-1);
		if (sValidExtensions.find(";"+toLowerCase(sValid)+";") != string::npos) // Nun, ich akzeptiere entweder "dat" oder "txt" ...
		{
			sValid = _sFileName;
		}
		else					// Ist es das nicht? Dann ersetzen wir die Endung doch einfach...
		{
			cerr << "|-> WARNUNG: Dieser Datentyp ist unbekannt oder geschuetzt! Die Endung wurde" << endl;
			cerr << "|   automatisch durch \".dat\" ersetzt!" << endl;
			sValid = _sFileName.substr(0,nPos) + ".dat";
		}
	}

	if (sValid.find('*') != string::npos || sValid.find('?') != string::npos)
	{
        WIN32_FIND_DATA FindFileData;
        HANDLE hFind = INVALID_HANDLE_VALUE;
        //LARGE_INTEGER Filesize;
        hFind = FindFirstFile(sValid.c_str(), &FindFileData);
        //cerr << sValid << endl;
        string sNewFileName = "";
        do
        {
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;
            sNewFileName = FindFileData.cFileName;
            if (sNewFileName.length() > 4
                && sNewFileName.find('.') != string::npos
                && sValidExtensions.find(";"+toLowerCase(sNewFileName.substr(sNewFileName.rfind('.')))+";") != string::npos)
                break;
            else
                sNewFileName = "";
        }
        while (FindNextFile(hFind, &FindFileData) != 0);
        FindClose(hFind);

        if (sNewFileName.length() > 4)
        {
            string sPathTemp = sValid;
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
                sValid = sPathTemp + "/" + sNewFileName;
            else
                sValid = sNewFileName;
        }
	}

	for (unsigned int i = 0; i < sValid.length(); i++)
	{
        if (sValid[i] == '\\')
            sValid[i] = '/';
	}

	return sValid;
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

void FileSystem::setTokens(string _sTokens)
{
    for (int i = 0; i < 6; i++)
    {
        sTokens[i][0] = _sTokens.substr(0,_sTokens.find('='));
        sTokens[i][1] = _sTokens.substr(_sTokens.find('=')+1, _sTokens.find(';')-1-_sTokens.find('='));
        _sTokens = _sTokens.substr(_sTokens.find(';')+1);
    }
}
