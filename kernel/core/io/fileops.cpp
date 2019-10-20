/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2018  Erik Haenel et al.

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

#include "fileops.hpp"
#include "../utils/tools.hpp"
#include "../maths/parser_functions.hpp"
#include "../../kernel.hpp"
#include "../ui/language.hpp"

extern Language _lang;

bool removeFile(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option)
{
    if (sCmd.length() < 7)
        return false;
    bool bIgnore = false;
    bool bAll = false;
    string _sCmd = "";
    FileSystem _fSys;
    _fSys.setTokens(_option.getTokenPaths());
    _fSys.setPath(_option.getExePath(), false, _option.getExePath());

    sCmd = fromSystemCodePage(sCmd);

    if (matchParams(sCmd, "ignore") || matchParams(sCmd, "i"))
    {
        if (matchParams(sCmd, "ignore"))
            sCmd.erase(matchParams(sCmd, "ignore")-1,6);
        else
            sCmd.erase(matchParams(sCmd, "i")-1,1);
        bIgnore = true;
    }
    if (matchParams(sCmd, "all") || matchParams(sCmd, "a"))
    {
        if (matchParams(sCmd, "all"))
            sCmd.erase(matchParams(sCmd, "all")-1,3);
        else
            sCmd.erase(matchParams(sCmd, "a")-1,1);
        bAll = true;
    }
    sCmd = sCmd.substr(sCmd.find("remove")+6);
    sCmd = sCmd.substr(0, sCmd.rfind('-'));
    StripSpaces(sCmd);
    while (sCmd[sCmd.length()-1] == '-' && sCmd[sCmd.length()-2] == ' ')
    {
        sCmd.erase(sCmd.length()-1);
        StripSpaces(sCmd);
    }

    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
    {
        string sDummy = "";
        NumeReKernel::getInstance()->getStringParser().evalAndFormat(sCmd, sDummy, true);
    }
    if (sCmd[0] == '"')
        sCmd = sCmd.substr(1);
    if (sCmd[sCmd.length()-1] == '"')
        sCmd = sCmd.substr(0,sCmd.length()-1);
    if (sCmd.length())
        _sCmd = _fSys.ValidFileName(sCmd);
    else
        return false;

    if (!fileExists(_sCmd))
        return false;

    while (_sCmd.length() && fileExists(_sCmd))
    {
        if (_sCmd.substr(_sCmd.rfind('.')) == ".exe"
            || _sCmd.substr(_sCmd.rfind('.')) == ".dll"
            || _sCmd.substr(_sCmd.rfind('.')) == ".vfm")
        {
            return false;
        }
        if (!bIgnore)
        {
            string c = "";
            NumeReKernel::print(LineBreak( _lang.get("BUILTIN_REMOVEFILE_CONFIRM", _sCmd), _option) );
            NumeReKernel::printPreFmt("|\n|<- ");
            NumeReKernel::getline(c);

            if (c != _lang.YES())
            {
                return false;
            }
        }
        remove(_sCmd.c_str());
        if (!bAll || (sCmd.find('*') == string::npos && sCmd.find('?') == string::npos))
            break;
        else
            _sCmd = _fSys.ValidFileName(sCmd);
    }

    return true;
}

bool moveFile(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option)
{
    if (sCmd.length() < 5)
        return false;

    string sTarget = "";
    string _sTarget = "";
    string sDummy = "";
    string sFile = "";
    string _sCmd = "";
    vector<string> vFileList;

    unsigned int nthFile = 1;
    bool bAll = false;
    bool bSuccess = false;

    FileSystem _fSys;
    _fSys.setTokens(_option.getTokenPaths());
    _fSys.setPath(_option.getExePath(), false, _option.getExePath());

    sCmd = fromSystemCodePage(sCmd);

    if (matchParams(sCmd, "all") || matchParams(sCmd, "a"))
    {
        bAll = true;
        if (matchParams(sCmd, "all"))
            sCmd.erase(matchParams(sCmd, "all")-1,3);
        else
            sCmd.erase(matchParams(sCmd, "a")-1,1);
    }
    if (matchParams(sCmd, "target", '=') || matchParams(sCmd, "t", '='))
    {
        unsigned int nPos = 0;
        if (matchParams(sCmd, "target", '='))
            nPos = matchParams(sCmd, "target", '=')+6;
        else
            nPos = matchParams(sCmd, "t", '=')+1;
        sTarget = getArgAtPos(sCmd, nPos);
        StripSpaces(sTarget);
        sCmd = sCmd.substr(0, sCmd.rfind('-', nPos));
        sCmd = sCmd.substr(sCmd.find(' ')+1);
        StripSpaces(sCmd);
    }
    else
    {
        throw SyntaxError(SyntaxError::NO_TARGET, sCmd, SyntaxError::invalid_position);
    }

    StripSpaces(sCmd);
    while (sCmd[sCmd.length()-1] == '-' && sCmd[sCmd.length()-2] == ' ')
    {
        sCmd.erase(sCmd.length()-1);
        StripSpaces(sCmd);
    }

    sCmd = replacePathSeparator(sCmd);
    sTarget = replacePathSeparator(sTarget);

    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
        NumeReKernel::getInstance()->getStringParser().evalAndFormat(sCmd, sDummy, true);

    if (sCmd[0] == '"')
        sCmd = sCmd.substr(1);
    if (sCmd[sCmd.length()-1] == '"')
        sCmd = sCmd.substr(0,sCmd.length()-1);
    /*if (sTarget[0] == '"')
        sTarget = sTarget.substr(1);
    if (sTarget[sTarget.length()-1] == '"')
        sTarget = sTarget.substr(0,sTarget.length()-1);*/

    vFileList = getFileList(sCmd, _option);

    if (!vFileList.size())
        return false;

    _sCmd = sCmd.substr(0,sCmd.rfind('/')) + "/";

    for (unsigned int nFile = 0; nFile < vFileList.size(); nFile++)
    {
        _sTarget = sTarget;
        sFile = _sCmd + vFileList[nFile];
        if (sFile.find('.') != string::npos)
        {
            string sExt = sFile.substr(sFile.rfind('.'));
            if (sExt != ".exe" && sExt != ".sys" && sExt != ".dll")
                _fSys.declareFileType(sExt);
        }
        sFile = _fSys.ValidFileName(sFile);

        if (_sTarget[_sTarget.length()-1] == '*' && _sTarget[_sTarget.length()-2] == '/')
            _sTarget = _sTarget.substr(0, _sTarget.length()-2) + sFile.substr(sFile.rfind('/'));
        else if (_sTarget[_sTarget.length()-1] == '/')
            _sTarget = _sTarget.substr(0, _sTarget.length()-1) + sFile.substr(sFile.rfind('/'));
        if (_sTarget.find('<') != string::npos && _sTarget.find('>', _sTarget.find('<')) != string::npos)
        {
            string sToken = "";
            for (unsigned int i = 0; i < _sTarget.length(); i++)
            {
                if (_sTarget[i] == '<')
                {
                    if (_sTarget.find('>', i) == string::npos)
                        break;
                    sToken = _sTarget.substr(i, _sTarget.find('>', i)+1-i);
                    if (sToken.find('#') != string::npos)
                    {

                        unsigned int nLength = 1;
                        if (sToken.find('~') != string::npos)
                            nLength = sToken.rfind('~')-sToken.find('#')+1;
                        sToken.clear();
                        if (nLength > toString((int)nthFile).length())
                            sToken.append(nLength-toString((int)(nthFile)).length(),'0');
                        sToken += toString((int)(nthFile));

                        _sTarget.replace(i,_sTarget.find('>',i)+1-i,sToken);
                        i += sToken.length();
                    }
                    else if (sToken == "<fname>")
                    {
                        sToken = sDummy.substr(sDummy.rfind('/')+1, sDummy.rfind('.')-1-sDummy.rfind('/'));

                        _sTarget.replace(i,_sTarget.find('>',i)+1-i,sToken);
                        i += sToken.length();
                    }
                }
                if (_sTarget.find('<',i) == string::npos)
                    break;
            }
        }
        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(_sTarget))
            NumeReKernel::getInstance()->getStringParser().evalAndFormat(_sTarget, sDummy, true);

        if (_sTarget[0] == '"')
            _sTarget.erase(0,1);
        if (_sTarget[_sTarget.length()-1] == '"')
            _sTarget.erase(_sTarget.length()-1);
        StripSpaces(_sTarget);

        if (_sTarget.substr(_sTarget.length()-2) == "/*")
            _sTarget.erase(_sTarget.length()-1);
        _sTarget = _fSys.ValidFileName(_sTarget);
        //NumeReKernel::print(_sTarget );

        if (_sTarget.substr(_sTarget.rfind('.')-1) == "*.dat")
            _sTarget = _sTarget.substr(0,_sTarget.rfind('/')) + sFile.substr(sFile.rfind('/'));
        else if (_sTarget.substr(_sTarget.rfind('.')-1) == "/.dat")
            _sTarget = _sTarget.substr(0, _sTarget.rfind('/')) + sFile.substr(sFile.rfind('/'));

        if (_sTarget.substr(_sTarget.rfind('.')) != sFile.substr(sFile.rfind('.')))
        {
            _sTarget = _sTarget.substr(0, _sTarget.rfind('.')) + sFile.substr(sFile.rfind('.'));
        }

        if (!fileExists(sFile))
            continue;

        //NumeReKernel::print(sFile );
        //NumeReKernel::print(_sTarget );

        moveFile(sFile, _sTarget);

        nthFile++;
        bSuccess = true;
        if (!bAll
            || (sCmd.find('*') == string::npos && sCmd.find('?') == string::npos)
            || (sTarget.find('*') == string::npos && (sTarget[sTarget.length()-1] != '/' && sTarget.substr(sTarget.length()-2) != "/\"") && sTarget.find("<#") == string::npos && sTarget.find("<fname>") == string::npos))
        {
            sCmd = sFile;
            break;
        }
    }

    return bSuccess;
}

bool copyFile(string& sCmd, Parser& _parser, Datafile& _data, const Settings& _option)
{
    if (sCmd.length() < 5)
        return false;

    string sTarget = "";
    string _sTarget = "";
    string _sCmd = "";
    string sDummy = "";
    string sFile = "";
    vector<string> vFileList;

    unsigned int nthFile = 1;
    bool bAll = false;
    bool bSuccess = false;
    ifstream File;
    ofstream Target;
    FileSystem _fSys;
    _fSys.setTokens(_option.getTokenPaths());
    _fSys.setPath(_option.getExePath(), false, _option.getExePath());

    sCmd = fromSystemCodePage(sCmd);

    if (matchParams(sCmd, "all") || matchParams(sCmd, "a"))
    {
        bAll = true;
        if (matchParams(sCmd, "all"))
            sCmd.erase(matchParams(sCmd, "all")-1,3);
        else
            sCmd.erase(matchParams(sCmd, "a")-1,3);
        StripSpaces(sCmd);
        while (sCmd[sCmd.length()-1] == '-' && sCmd[sCmd.length()-2] == ' ')
        {
            sCmd.erase(sCmd.length()-1);
            StripSpaces(sCmd);
        }
    }
    if (matchParams(sCmd, "target", '=') || matchParams(sCmd, "t", '='))
    {
        unsigned int nPos = 0;
        if (matchParams(sCmd, "target", '='))
            nPos = matchParams(sCmd, "target", '=')+6;
        else
            nPos = matchParams(sCmd, "t", '=')+1;
        sTarget = sCmd.substr(nPos);
        StripSpaces(sTarget);
        sCmd = sCmd.substr(0, sCmd.rfind('-', nPos));
        sCmd = sCmd.substr(sCmd.find(' ')+1);
        StripSpaces(sCmd);
    }
    else
    {
        throw SyntaxError(SyntaxError::NO_TARGET, sCmd, SyntaxError::invalid_position);
    }
    StripSpaces(sCmd);
    while (sCmd[sCmd.length()-1] == '-' && sCmd[sCmd.length()-2] == ' ')
    {
        sCmd.erase(sCmd.length()-1);
        StripSpaces(sCmd);
    }

    sCmd = replacePathSeparator(sCmd);
    sTarget = replacePathSeparator(sTarget);

    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd))
        NumeReKernel::getInstance()->getStringParser().evalAndFormat(sCmd, sDummy, true);

    if (sCmd[0] == '"')
        sCmd = sCmd.substr(1);
    if (sCmd[sCmd.length()-1] == '"')
        sCmd = sCmd.substr(0,sCmd.length()-1);

    vFileList = getFileList(sCmd, _option);

    if (!vFileList.size())
        return false;

    _sCmd = sCmd.substr(0,sCmd.rfind('/')) + "/";

    for (unsigned int nFile = 0; nFile < vFileList.size(); nFile++)
    {
        _sTarget = sTarget;
        sFile = _sCmd + vFileList[nFile];
        if (sFile.find('.') != string::npos)
        {
            string sExt = sFile.substr(sFile.rfind('.'));
            if (sExt != ".exe" && sExt != ".sys" && sExt != ".dll")
                _fSys.declareFileType(sExt);
        }
        sFile = _fSys.ValidFileName(sFile);

        if (_sTarget[_sTarget.length()-1] == '*' && _sTarget[_sTarget.length()-2] == '/')
            _sTarget = _sTarget.substr(0, _sTarget.length()-2) + sFile.substr(sFile.rfind('/'));
        else if (_sTarget[_sTarget.length()-1] == '/')
            _sTarget = _sTarget.substr(0, _sTarget.length()-1) + sFile.substr(sFile.rfind('/'));
        if (_sTarget.find('<') != string::npos && _sTarget.find('>', _sTarget.find('<')) != string::npos)
        {
            string sToken = "";
            for (unsigned int i = 0; i < _sTarget.length(); i++)
            {
                if (_sTarget[i] == '<')
                {
                    if (_sTarget.find('>', i) == string::npos)
                        break;
                    sToken = _sTarget.substr(i, _sTarget.find('>', i)+1-i);
                    if (sToken.find('#') != string::npos)
                    {

                        unsigned int nLength = 1;
                        if (sToken.find('~') != string::npos)
                            nLength = sToken.rfind('~')-sToken.find('#')+1;
                        sToken.clear();
                        if (nLength > toString((int)nthFile).length())
                            sToken.append(nLength-toString((int)(nthFile)).length(),'0');
                        sToken += toString((int)(nthFile));

                        _sTarget.replace(i,_sTarget.find('>',i)+1-i,sToken);
                        i += sToken.length();
                    }
                    else if (sToken == "<fname>")
                    {
                        sToken = sDummy.substr(sDummy.rfind('/')+1, sDummy.rfind('.')-1-sDummy.rfind('/'));

                        _sTarget.replace(i,_sTarget.find('>',i)+1-i,sToken);
                        i += sToken.length();
                    }
                }
                if (_sTarget.find('<',i) == string::npos)
                    break;
            }
        }
        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(_sTarget))
            NumeReKernel::getInstance()->getStringParser().evalAndFormat(_sTarget, sDummy, true);

        if (_sTarget[0] == '"')
            _sTarget.erase(0,1);
        if (_sTarget[_sTarget.length()-1] == '"')
            _sTarget.erase(_sTarget.length()-1);
        StripSpaces(_sTarget);

        if (_sTarget.substr(_sTarget.length()-2) == "/*")
            _sTarget.erase(_sTarget.length()-1);
        _sTarget = _fSys.ValidFileName(_sTarget);
        //NumeReKernel::print(_sTarget );

        if (_sTarget.substr(_sTarget.rfind('.')-1) == "*.dat")
            _sTarget = _sTarget.substr(0,_sTarget.rfind('/')) + sFile.substr(sFile.rfind('/'));
        else if (_sTarget.substr(_sTarget.rfind('.')-1) == "/.dat")
            _sTarget = _sTarget.substr(0, _sTarget.rfind('/')) + sFile.substr(sFile.rfind('/'));

        if (_sTarget.substr(_sTarget.rfind('.')) != sFile.substr(sFile.rfind('.')))
        {
            _sTarget = _sTarget.substr(0, _sTarget.rfind('.')) + sFile.substr(sFile.rfind('.'));
        }

        if (!fileExists(sFile))
            continue;

        //NumeReKernel::print(sFile );
        //NumeReKernel::print(_sTarget );

        File.open(sFile.c_str(), ios_base::binary);
        if (File.fail())
        {
            File.close();
            continue;
        }
        Target.open(_sTarget.c_str(), ios_base::binary);
        if (Target.fail())
        {
            Target.close();
            continue;
        }

        Target << File.rdbuf();
        File.close();
        Target.close();

        nthFile++;
        bSuccess = true;
        if (!bAll
            || (sCmd.find('*') == string::npos && sCmd.find('?') == string::npos)
            || (sTarget.find('*') == string::npos && (sTarget[sTarget.length()-1] != '/' && sTarget.substr(sTarget.length()-2) != "/\"") && sTarget.find("<#") == string::npos && sTarget.find("<fname>") == string::npos))
        {
            sCmd = sFile;
            break;
        }
    }

    return bSuccess;
}

bool generateTemplate(const string& sFile, const string& sTempl, const vector<string>& vTokens, Settings& _option)
{
    ifstream iTempl_in;
    ofstream oFile_out;
    string sLine;
    string sToken;

    iTempl_in.open(_option.ValidFileName(sTempl, ".nlng").c_str());
    oFile_out.open(_option.ValidFileName(sFile, sFile.substr(sFile.rfind('.'))).c_str());

    if (iTempl_in.fail() || oFile_out.fail())
    {
        iTempl_in.close();
        oFile_out.close();
        return false;
    }

    while (!iTempl_in.eof())
    {
        getline(iTempl_in, sLine);
        for (unsigned int i = 0; i < vTokens.size(); i++)
        {
            sToken = "%%"+toString(i+1)+"%%";
            while (sLine.find(sToken) != string::npos)
            {
                sLine.replace(sLine.find(sToken), sToken.length(), vTokens[i]);
            }
        }
        oFile_out << sLine << endl;
    }
    iTempl_in.close();
    oFile_out.close();
    return true;
}

