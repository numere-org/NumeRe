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

#include <filesystem>

#include "fileops.hpp"
#include "../utils/tools.hpp"
#include "../maths/parser_functions.hpp"
#include "../../kernel.hpp"
#include "../ui/language.hpp"
#include "../utils/filecheck.hpp"

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

    // Get all relevant files or folders
    std::vector<std::string> vFiles;

    if (!is_file(cmdParser.parseExprAsString()))
        vFiles = _fSys.getFolderList(cmdParser.parseExprAsString(), FileSystem::FULLPATH);
    else
        vFiles = _fSys.getFileList(cmdParser.parseExprAsString(), FileSystem::FULLPATH);

    // No files -> No deletion needed
    if (!vFiles.size())
        return bIgnore; // return true, if ignored, false otherwise

    // Delete the first or every file
    for (const std::string& sFile : vFiles)
    {
        if (sFile.ends_with("/.."))
            continue;

        if (sFile.ends_with(".exe")
            || sFile.ends_with(".dll")
            || sFile.ends_with(".sys")
            || sFile.ends_with(".vfm"))
            return false;

        if (!bIgnore)
        {
            std::string c = "";
            NumeReKernel::print(LineBreak(_lang.get("BUILTIN_REMOVEFILE_CONFIRM", sFile), NumeReKernel::getInstance()->getSettings()));
            NumeReKernel::printPreFmt("|\n|<- ");
            NumeReKernel::getline(c);

            if (c != _lang.YES())
                return false;
        }

        // Delete it
        std::filesystem::remove_all(sFile);

        // Delete only the first one
        if (!bAll)
            return true;
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Handle tags in the target filename and
/// resolve them correspondingly.
///
/// \param sCurrentTarget std::string&
/// \param sFile const std::string&
/// \param nthFile size_t
/// \return void
///
/////////////////////////////////////////////////
static void handleFileTags(std::string& sCurrentTarget, const std::string& sFile, size_t nthFile)
{
    if (sCurrentTarget.find('<') == std::string::npos)
        return;

    std::string sToken = "";

    for (size_t i = 0; i < sCurrentTarget.length(); i++)
    {
        if (sCurrentTarget[i] == '<')
        {
            if (sCurrentTarget.find('>', i) == std::string::npos)
                break;

            sToken = sCurrentTarget.substr(i, sCurrentTarget.find('>', i)+1-i);

            if (sToken.find('#') != std::string::npos)
            {
                size_t nLength = 1;

                if (sToken.find('~') != std::string::npos)
                    nLength = sToken.rfind('~')-sToken.find('#')+1;

                sToken.clear();

                if (nLength > toString(nthFile).length())
                    sToken.append(nLength-toString(nthFile).length(),'0');

                sToken += toString(nthFile);
                sCurrentTarget.replace(i,sCurrentTarget.find('>',i)+1-i,sToken);
                i += sToken.length();
            }
            else if (sToken == "<fname>")
            {
                sToken = sFile.substr(sFile.rfind('/')+1, sFile.rfind('.')-1-sFile.rfind('/'));
                sCurrentTarget.replace(i, sCurrentTarget.find('>', i)+1-i, sToken);
                i += sToken.length();
            }
        }

        if (sCurrentTarget.find('<',i) == std::string::npos)
            break;
    }
}


/////////////////////////////////////////////////
/// \brief Prepare the target file name for
/// copying or moving a file.
///
/// \param sTarget std::string
/// \param sFile const std::string&
/// \param nthFile size_t
/// \param _fSys const FileSystem&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string prepareTargetFileName(std::string sTarget, const std::string& sFile, size_t nthFile, const FileSystem& _fSys)
{
    if (sTarget.back() == '/')
        sTarget += '*';

    // Split up the paths into individual folders
    std::vector<std::string> vSplitTarget = split(sTarget, '/');
    std::vector<std::string> vSplitFile = split(sFile, '/');

    int i = vSplitTarget.size()-1;
    int j = vSplitFile.size()-1;

    // Replace all wildcards with their counterparts from the
    // copied/moved filename. Could be a for loop but is more
    // readable in the way it is right now
    while (i >= 0 && j >= 0)
    {
        if (vSplitTarget[i] == "*")
            vSplitTarget[i] = vSplitFile[j];

        i--;
        j--;
    }

    sTarget.clear();

    // Combine the path back into a regular path
    for (const std::string& sPart : vSplitTarget)
    {
        if (sTarget.length())
            sTarget += '/';

        sTarget += sPart;
    }

    // Prepare the file tags
    handleFileTags(sTarget, sFile, nthFile);
    StripSpaces(sTarget);

    // Validate target file name
    sTarget = _fSys.ValidizeAndPrepareName(sTarget);

    // Check for an identical extension
    if (sTarget.substr(sTarget.rfind('.')) != sFile.substr(sFile.rfind('.')))
        sTarget = sTarget.substr(0, sTarget.rfind('.')) + sFile.substr(sFile.rfind('.'));

    return sTarget;
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

    std::string sSource = cmdParser.parseExprAsString();
    std::string sTarget = "";

    size_t nthFile = 1;
    bool bAll = cmdParser.hasParam("all") || cmdParser.hasParam("a");
    bool bSuccess = false;

    FileSystem _fSys;
    _fSys.initializeFromKernel();
    cmdParser.clearReturnValue();

    // Get the target
    if (cmdParser.hasParam("target") || cmdParser.hasParam("t"))
    {
        if (cmdParser.hasParam("target"))
            sTarget = cmdParser.getParsedParameterValueAsString("target", "");
        else
            sTarget = cmdParser.getParsedParameterValueAsString("t", "");

        StripSpaces(sTarget);
    }
    else
        throw SyntaxError(SyntaxError::NO_TARGET, cmdParser.getCommandLine(), SyntaxError::invalid_position);

    // Clean source and target paths
    sSource = replacePathSeparator(sSource);
    sTarget = replacePathSeparator(sTarget);

    // In 'all' case it is necessary to set the return value
    // from the source string
    if (bAll)
        cmdParser.setReturnValue(sSource);

    // Get the source file list an validate
    std::vector<std::string> vFileList = _fSys.getFileList(sSource, FileSystem::FULLPATH);

    if (!vFileList.size())
        return false;

    // Operate on each file in the list
    for (size_t nFile = 0; nFile < vFileList.size(); nFile++)
    {
        std::string sFile = vFileList[nFile];

        if (sFile.find('.') != std::string::npos)
        {
            std::string sExt = sFile.substr(sFile.rfind('.'));

            if (sExt != ".exe" && sExt != ".sys" && sExt != ".dll")
                _fSys.declareFileType(sExt);
        }

        // Resolve the file name
        sFile = _fSys.ValidFileName(sFile);

        if (!fileExists(sFile))
            continue;

        std::string sCurrentTarget = prepareTargetFileName(sTarget, sFile, nthFile, _fSys);

        // Perform the actual file operation
        if (cmdParser.getCommand() == "move")
            moveFile(sFile, sCurrentTarget);
        else
            copyFile(sFile, sCurrentTarget);

        nthFile++;
        bSuccess = true;

        if (!bAll)
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

