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

std::string removeQuotationMarks(const std::string&);

/////////////////////////////////////////////////
/// \brief Removes one or more files from the
/// disk.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool removeFile(CommandLineParser& cmdParser)
{
    if (!cmdParser.getExpr().length())
        return false;

    bool bIgnore = cmdParser.hasParam("ignore") || cmdParser.hasParam("i");
    bool bAll = cmdParser.hasParam("all") || cmdParser.hasParam("a");
    std::string _sCmd = "";
    FileSystem _fSys;
    _fSys.initializeFromKernel();

    //sCmd = fromSystemCodePage(sCmd);

    // Get all relevant files
    std::vector<std::string> vFiles = getFileList(cmdParser.parseExprAsString(), NumeReKernel::getInstance()->getSettings(), 1);

    // No files -> No deletion needed
    if (!vFiles.size())
        return bIgnore; // return true, if ignored, false otherwise

    // Delete the first or every file
    for (const std::string& sFile : vFiles)
    {
        if (sFile.ends_with(".exe")
            || sFile.ends_with(".dll")
            || sFile.ends_with(".sys")
            || sFile.ends_with(".vfm"))
            return false;

        if (!bIgnore)
        {
            std::string c = "";
            NumeReKernel::print(LineBreak( _lang.get("BUILTIN_REMOVEFILE_CONFIRM", _sCmd), NumeReKernel::getInstance()->getSettings()) );
            NumeReKernel::printPreFmt("|\n|<- ");
            NumeReKernel::getline(c);

            if (c != _lang.YES())
                return false;
        }

        // Delete it
        remove(sFile.c_str());

        // Delete only the first one
        if (!bAll)
            return true;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Moves or copies files from one
/// location to another. Supports also wildcards
/// and file lists.
///
/// \param cmdParser CommandLineParser&
/// \return bool
///
/////////////////////////////////////////////////
bool moveOrCopyFiles(CommandLineParser& cmdParser)
{
    if (!cmdParser.getExpr().length())
        return false;

    std::string sSource = cmdParser.getExpr();
    std::string sTarget = "";
    std::string _sTarget = "";
    std::string sDummy = "";
    std::string sFile = "";
    std::vector<std::string> vFileList;

    size_t nthFile = 1;
    bool bAll = cmdParser.hasParam("all") || cmdParser.hasParam("a");
    bool bSuccess = false;

    FileSystem _fSys;
    _fSys.initializeFromKernel();
    cmdParser.clearReturnValue();

    //sCmd = fromSystemCodePage(sCmd);

    // Get the target
    if (cmdParser.hasParam("target") || cmdParser.hasParam("t"))
    {
        if (cmdParser.hasParam("target"))
            sTarget = cmdParser.getParameterValueAsString("target", "");
        else
            sTarget = cmdParser.getParameterValueAsString("t", "");

        StripSpaces(sTarget);
    }
    else
        throw SyntaxError(SyntaxError::NO_TARGET, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Clean source and target paths
    sSource = replacePathSeparator(sSource);
    sTarget = replacePathSeparator(sTarget);

    if (NumeReKernel::getInstance()->getStringParser().isStringExpression(sSource))
        NumeReKernel::getInstance()->getStringParser().evalAndFormat(sSource, sDummy, true);

    sSource = removeQuotationMarks(sSource);

    // In 'all' case it is necessary to set the return value
    // from the source string
    if (bAll)
        cmdParser.setReturnValue(sSource);

    // Get the source file list an validate
    vFileList = getFileList(sSource, NumeReKernel::getInstance()->getSettings(), 1);

    if (!vFileList.size())
        return false;

    // Operate on each file in the list
    for (size_t nFile = 0; nFile < vFileList.size(); nFile++)
    {
        _sTarget = sTarget;
        sFile = vFileList[nFile];

        if (sFile.find('.') != std::string::npos)
        {
            std::string sExt = sFile.substr(sFile.rfind('.'));

            if (sExt != ".exe" && sExt != ".sys" && sExt != ".dll")
                _fSys.declareFileType(sExt);
        }

        // Resolve the file name
        sFile = _fSys.ValidFileName(sFile);

        if (_sTarget[_sTarget.length()-1] == '*' && _sTarget[_sTarget.length()-2] == '/')
            _sTarget = _sTarget.substr(0, _sTarget.length()-2) + sFile.substr(sFile.rfind('/'));
        else if (_sTarget[_sTarget.length()-1] == '/')
            _sTarget = _sTarget.substr(0, _sTarget.length()-1) + sFile.substr(sFile.rfind('/'));

        // Prepare the file tags
        if (_sTarget.find('<') != std::string::npos && _sTarget.find('>', _sTarget.find('<')) != std::string::npos)
        {
            std::string sToken = "";

            for (size_t i = 0; i < _sTarget.length(); i++)
            {
                if (_sTarget[i] == '<')
                {
                    if (_sTarget.find('>', i) == std::string::npos)
                        break;

                    sToken = _sTarget.substr(i, _sTarget.find('>', i)+1-i);

                    if (sToken.find('#') != std::string::npos)
                    {
                        size_t nLength = 1;

                        if (sToken.find('~') != std::string::npos)
                            nLength = sToken.rfind('~')-sToken.find('#')+1;

                        sToken.clear();

                        if (nLength > toString(nthFile).length())
                            sToken.append(nLength-toString(nthFile).length(),'0');

                        sToken += toString(nthFile);
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

                if (_sTarget.find('<',i) == std::string::npos)
                    break;
            }
        }

        if (NumeReKernel::getInstance()->getStringParser().isStringExpression(_sTarget))
            NumeReKernel::getInstance()->getStringParser().evalAndFormat(_sTarget, sDummy, true);

        _sTarget = removeQuotationMarks(_sTarget);
        StripSpaces(_sTarget);

        if (_sTarget.ends_with("/*"))
            _sTarget.pop_back();

        // Validate target file name
        _sTarget = _fSys.ValidFileName(_sTarget);

        if (_sTarget.ends_with("*.dat"))
            _sTarget = _sTarget.substr(0,_sTarget.rfind('/')) + sFile.substr(sFile.rfind('/'));
        else if (_sTarget.ends_with("/.dat"))
            _sTarget = _sTarget.substr(0, _sTarget.rfind('/')) + sFile.substr(sFile.rfind('/'));

        if (_sTarget.substr(_sTarget.rfind('.')) != sFile.substr(sFile.rfind('.')))
            _sTarget = _sTarget.substr(0, _sTarget.rfind('.')) + sFile.substr(sFile.rfind('.'));

        if (!fileExists(sFile))
            continue;

        // Perform the actual file operation
        if (cmdParser.getCommand() == "move")
            moveFile(sFile, _sTarget);
        else
            copyFile(sFile, _sTarget);

        nthFile++;
        bSuccess = true;

        if (!bAll
            || (cmdParser.getCommandLine().find_first_of("?*") == std::string::npos)
            || (sTarget.find('*') == std::string::npos && (sTarget.back() != '/' && !sTarget.ends_with("/\"")) && sTarget.find("<#") == std::string::npos && sTarget.find("<fname>") == std::string::npos))
        {
            cmdParser.setReturnValue(sFile);
            break;
        }
    }

    return bSuccess;
}


bool generateTemplate(const std::string& sFile, const std::string& sTempl, const std::vector<std::string>& vTokens, Settings& _option)
{
    std::ifstream iTempl_in;
    std::ofstream oFile_out;
    std::string sLine;
    std::string sToken;

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
        std::getline(iTempl_in, sLine);
        for (size_t i = 0; i < vTokens.size(); i++)
        {
            sToken = "%%"+toString(i+1)+"%%";
            while (sLine.find(sToken) != std::string::npos)
            {
                sLine.replace(sLine.find(sToken), sToken.length(), vTokens[i]);
            }
        }
        oFile_out << sLine << std::endl;
    }
    iTempl_in.close();
    oFile_out.close();
    return true;
}

