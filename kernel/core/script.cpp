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


// Implementation der Script-Klasse

#include "script.hpp"
#include "../kernel.hpp"
#include "../versioninformation.hpp"
#include "utils/tools.hpp"
#include "documentation/docfile.hpp"

#include <algorithm>

using namespace std;


/////////////////////////////////////////////////
/// \brief Default constructor
/////////////////////////////////////////////////
Script::Script() : FileSystem(), _localDef(true)
{
    bValidScript = false;
    bLastScriptCommand = false;
    isInstallMode = false;
    nInstallModeFlags = ENABLE_DEFAULTS;
    nCurrentPackage = 0;
    nLine = 0;
    m_repo.connect(REPO_LOCATION);
}


/////////////////////////////////////////////////
/// \brief Destructor
/////////////////////////////////////////////////
Script::~Script()
{
    if (m_script)
    {
        Script::close();
    }
}


/////////////////////////////////////////////////
/// \brief This member function opens the script
/// with the passed file name.
///
/// \param _sScriptFileName std::string&
/// \return void
///
/////////////////////////////////////////////////
void Script::openScript(std::string& _sScriptFileName, int nFromLine)
{
    // Close an already opened script
    if (m_script)
        close();

    _sScriptFileName = ValidFileName(_sScriptFileName, ".nscr");

    // Open the script, if the script file name exists
    if (_sScriptFileName.length())
    {
        m_script.reset(new StyledTextFile(_sScriptFileName));

        // Ensure that the script exists and is a valid file
        if (m_script->getLastPosition() == -1)
        {
            // If the user tried to install something, we're
            // looking it up before sending out the non-existent
            // file error
            if (isInstallMode && handleRepositoryInstall(getFileParts(_sScriptFileName)[2]))
                return;

            close();
            throw SyntaxError(SyntaxError::SCRIPT_NOT_EXIST, "", SyntaxError::invalid_position, _sScriptFileName);
        }

        // Ensure that the starting line is within the valid regime
        if (nFromLine < 0 || nFromLine >= m_script->getLinesCount())
        {
            close();
            return;
        }

        // Set the defaults
        sScriptFileName = _sScriptFileName;
        bValidScript = true;
        nInstallModeFlags = ENABLE_DEFAULTS;
        sHelpID = "";
        sInstallID = "";
        nLine = 0;
        _localDef.reset();
        _symdefs.clear();
        int nBlockStartLine = 0;
        static FlowCtrl flowCtrlSyntaxHelper(true);

        // Advance to the requested start line and parse file specific
        // constants, includes and function definitions
        while (nLine < nFromLine && isOpen())
        {
            std::string sCmdline = getNextScriptCommand();

            // Detect possible flow control blocks
            if (flowCtrlSyntaxHelper.getCurrentBlockDepth() > 0)
                flowCtrlSyntaxHelper.addToControlFlowBlock(sCmdline, nLine);
            else if (flowCtrlSyntaxHelper.isFlowCtrlStatement(findCommand(sCmdline).sString))
            {
                flowCtrlSyntaxHelper.addToControlFlowBlock(sCmdline, nLine);

                // Store the start position, if this block is longer than
                // one line. Note that the current line has incremented after
                // we read a line from the script
                if (flowCtrlSyntaxHelper.getCurrentBlockDepth() > 0)
                    nBlockStartLine = nLine-1;
            }
        }

        if (!isOpen())
            return;

        // If we're still inside a block, go back to its start position
        if (nLine >= nFromLine && flowCtrlSyntaxHelper.getCurrentBlockDepth() > 0)
        {
            flowCtrlSyntaxHelper.reset();
            nLine = nBlockStartLine;
        }
        else if (nLine > nFromLine)
            nLine = nFromLine; // This is a quick fix for problems, which arise from empty lines
    }
}


/////////////////////////////////////////////////
/// \brief This member function closes an opened
/// script.
///
/// \return void
///
/////////////////////////////////////////////////
void Script::close()
{
    // If a script is open
    if (m_script)
    {
        // Close the file streams
        m_script.reset();

        if (m_include)
            m_include.reset();

        // If the file stream of the installation log
        // is still open, add a installation failed message
        // to it and close it afterwards
        if (m_logger.is_open())
        {
            m_logger.push("--- INSTALLATION FAILED ---\n\n\n");
            m_logger.close();
            NumeReKernel::installationDone();
        }

        // If this is a chained installation (a.k.a. installing
        // multiple packages), then we don't want to reset the
        // flags but open the next script instead
        if (vInstallPackages.size() > nCurrentPackage+1)
        {
            bLastScriptCommand = false;
            nCurrentPackage++;

            // Open the file and hope, it
            // actually exist
            openScript(vInstallPackages[nCurrentPackage], 0);

            return;
        }

        if (vInstallPackages.size())
        {
            sScriptFileName = vInstallPackages[0];
            NumeReKernel::installationDone();
        }

        // This was the last package
        vInstallPackages.clear();
        nCurrentPackage = 0;

        // Reset the flags
        bValidScript = false;
        isInstallMode = false;
        nInstallModeFlags = ENABLE_DEFAULTS;
        sHelpID.clear();
        sInstallID.clear();
        nLine = 0;
        _localDef.reset();
        _symdefs.clear();
    }
}


/////////////////////////////////////////////////
/// \brief This member function closes the script,
///  if the code reached a "return" statement.
///
/// \return void
///
/// This function has to be called from the
/// outside, because this class won't track this
/// command internally.
/////////////////////////////////////////////////
void Script::returnCommand()
{
    if (m_script)
    {
        if (m_logger.is_open())
        {
            m_logger.push("--- INSTALLATION TERMINATED SUCCESSFULLY ---\n\n\n");
            m_logger.close();

            if (!(nInstallModeFlags & DISABLE_SCREEN_OUTPUT))
                NumeReKernel::print(toSystemCodePage(_lang.get("SCRIPT_INSTALL_SUCCESS")));

            NumeReKernel::installationDone();
        }

        close();
    }
}


/////////////////////////////////////////////////
/// \brief This member function starts the
/// current installation section.
///
/// \param sScriptCommand string&
/// \return bool
///
/////////////////////////////////////////////////
bool Script::startInstallation(string& sScriptCommand)
{
    // Open the installation logfile
    if (!m_logger.open(sTokens[0][1] + "\\install.log"))
        throw SyntaxError(SyntaxError::CANNOT_OPEN_LOGFILE, sScriptCommand, SyntaxError::invalid_position, sTokens[0][1] + "\\install.log");

    // Write the first line
    m_logger.push_line("--- INSTALLATION " + getTimeStamp(false) + " ---");
    isInInstallSection = true;

    // Remove the install tag and strip the white spaces
    sScriptCommand = sScriptCommand.substr(9);
    StripSpaces(sScriptCommand);

    if (!sScriptCommand.length())
        return false;

    // Try to find the install information tag set
    if (sScriptCommand.find("<info>") == string::npos)
    {
        sScriptCommand = "";
        return false;
    }
    else
    {
        // If the install information tags were found
        // handle them here
        return handleInstallInformation(sScriptCommand);
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// install information tags of the current
/// installation section.
///
/// \param sScriptCommand string&
/// \return bool
///
/////////////////////////////////////////////////
bool Script::handleInstallInformation(string& sScriptCommand)
{
    double fRequiredVersion = getFloatingPointVersion();

    // If the current install information string is incomplete
    // (i.e. no "<endinfo>" tag), then search for the corresponding
    // tag in the next lines
    if (sScriptCommand.find("<endinfo>") == string::npos)
    {
        std::string sTemp;

        // Read lines from the script until the "<endinfo>" tag was found
        while (nLine < m_script->getLinesCount())
        {
            sTemp = m_script->getStrippedLine(nLine);
            nLine++;
            StripSpaces(sTemp);

            if (sTemp.find("<endinfo>") == string::npos)
                sScriptCommand += " " + sTemp;
            else
            {
                sScriptCommand += " " + sTemp.substr(0, sTemp.find("<endinfo>") + 9);
                break;
            }
        }
    }

    // Ensure that an "<endinfo>" tag was found
    if (sScriptCommand.find("<endinfo>") == string::npos)
    {
        throw SyntaxError(SyntaxError::SCRIPT_NOT_EXIST, "", "");
    }

    // Extract the install information string and the installation ID
    std::string sInstallInfoString = sScriptCommand.substr(sScriptCommand.find("<info>")+6, sScriptCommand.find("<endinfo>")-sScriptCommand.find("<info>")-6);
    sScriptCommand = sScriptCommand.substr(sScriptCommand.find("<endinfo>")+9);
    sInstallID = getArgAtPos(sInstallInfoString, sInstallInfoString.find("name=")+5);

    // Check whether it is necessary to install this package
    if (sInstallInfoString.find("-version=") != std::string::npos)
    {
        size_t nPackageVersion = versionToFloat(getArgAtPos(sInstallInfoString, sInstallInfoString.find("-version=")+9));

        // Get all installed packages
        const std::vector<Package>& vInstalledPackages = NumeReKernel::getInstance()->getInstalledPackages();

        // Try to detect a match
        for (const auto& package : vInstalledPackages)
        {
            if (package.getName() == sInstallID)
            {
                if (versionToFloat(package.sVersion) >= nPackageVersion)
                {
                    isInInstallSection = false;
                    NumeReKernel::print(_lang.get("SCRIPT_ALREADY_IMSTALLED", package.getName()));
                    return false;
                }

                break;
            }
        }
    }

    // Determine, whether the current installation needs additional packages
    if (sInstallInfoString.find("requirepackages=") != string::npos)
    {
        // Get the required packages list
        string sInstallPackages = getArgAtPos(sInstallInfoString, sInstallInfoString.find("requirepackages=")+16);

        // Read all required packages
        while (sInstallPackages.length())
        {
            // Get the next dependency
            string sPackage = getNextArgument(sInstallPackages, true);

            // Try to find the package in the packages
            // folder first before using the main folder
            if (fileExists(ValidFileName("packages/" + sPackage, ".nscr")))
                sPackage = ValidFileName("packages/" + sPackage, ".nscr");
            else
                sPackage = ValidFileName(sPackage, ".nscr");

            if (!sPackage.length())
                continue;

            // If this is the first package, simply append it
            if (!vInstallPackages.size())
            {
                vInstallPackages.push_back(sScriptFileName);
                vInstallPackages.push_back(sPackage);
            }
            else if (std::find(vInstallPackages.begin(), vInstallPackages.end(), sPackage) == vInstallPackages.end())
                vInstallPackages.push_back(sPackage);
        }
    }

    // Determine, whether a version of NumeRe is required
    if (sInstallInfoString.find("requireversion=") != string::npos)
        fRequiredVersion = versionToFloat(getArgAtPos(sInstallInfoString, sInstallInfoString.find("requireversion=")+15));

    // Throw an error, if the current version if NumeRe is too old
    if (fRequiredVersion > getFloatingPointVersion())
        throw SyntaxError(SyntaxError::INSUFFICIENT_NUMERE_VERSION, sScriptCommand, SyntaxError::invalid_position,
                          getArgAtPos(sInstallInfoString, sInstallInfoString.find("requireversion=")+15));

    // Examine the license information
    if (sInstallInfoString.find("license=") != std::string::npos)
    {
        std::string sLicense = getArgAtPos(sInstallInfoString, sInstallInfoString.find("license=")+8);
        NumeReKernel::print(LineBreak("EULA: " + _lang.get("SCRIPT_INSTALL_LICENSE_AGREEMENT", sInstallID, sLicense),
                                      NumeReKernel::getInstance()->getSettings()));
        NumeReKernel::printPreFmt("|<- ");

        std::string sAnswer;
        NumeReKernel::getline(sAnswer);

        if (sAnswer.substr(0, 1) != _lang.YES())
        {
            NumeReKernel::print(_lang.get("SCRIPT_INSTALL_ABORT"));
            close();
            return false;
        }
    }

    evaluateInstallInformation(sInstallInfoString);

    if (sInstallInfoString.length())
        NumeReKernel::getInstance()->getProcedureInterpreter().declareNewPackage(sInstallInfoString);

    if (!sScriptCommand.length())
        return false;
    return true;
}


/////////////////////////////////////////////////
/// \brief This member function writes the
/// appended documentation article to the target
/// file.
///
/// \param sScriptCommand string&
/// \return void
///
/////////////////////////////////////////////////
void Script::writeDocumentationArticle(string& sScriptCommand)
{
    std::vector<std::string> vDocFileContents;

    // Depending on whether the whole file was written in
    // one line or multiple script lines
    if (sScriptCommand.find("</helpfile>") != std::string::npos)
    {
        sScriptCommand.erase(sScriptCommand.find("</helpfile>")+11);
        sScriptCommand.erase(0, 10);

        vDocFileContents.push_back(sScriptCommand);
    }
    else
    {
        vDocFileContents.push_back(sScriptCommand.substr(10));
        std::string sTemp;

        // Read the contents linewise from the script
        while (nLine < m_script->getLinesCount())
        {
            sTemp = m_script->getStrippedLine(nLine);
            nLine++;
            StripSpaces(sTemp);

            // Try to find the end of the current documentation article
            if (sTemp.find("</helpfile>") == std::string::npos)
                vDocFileContents.push_back(sTemp);
            else
            {
                // Append the last line
                vDocFileContents.push_back(sTemp.substr(0,sTemp.find("</helpfile>")));
                break;
            }
        }
    }

    DocumentationFile docFile(vDocFileContents);
    std::vector<DocumentationArticle>& vArticles = docFile.getArticles();

    for (size_t i = 0; i < vArticles.size(); i++)
    {
        if (!sHelpID.length() && vArticles[i].m_docEntry.sArticleId.length())
        {
            sHelpID = vArticles[i].m_docEntry.sArticleId;

            // Ensure that the article ID start with the plugin prefix
            if (sHelpID.substr(0, 5) != "plgn_" && sHelpID.substr(0, 4) != "pkg_")
                sHelpID = "pkg_" + sHelpID;
        }

        std::string sId = vArticles[i].m_docEntry.sArticleId;

        if (sId.substr(0, 5) != "plgn_" && sId.substr(0, 4) != "pkg_")
        {
            std::string sNewId = "pkg_" + sId;
            vArticles[i].m_docEntry.sArticleId = sId;
        }

        if (!vArticles[i].m_keywords.size())
            vArticles[i].m_keywords.push_back(sInstallID);
    }

    std::string sHelpfileName = "<>/docs/plugins/" + sHelpID + ".nhlp";
    sHelpfileName = FileSystem::ValidFileName(sHelpfileName, ".nhlp");

    docFile.print(sHelpfileName);

    NumeReKernel::getInstance()->getProcedureInterpreter().addHelpIndex(sInstallID, sHelpID);
    NumeReKernel::getInstance()->getSettings().addFileToDocumentationIndex(sHelpfileName);
    sScriptCommand.clear();
}


/////////////////////////////////////////////////
/// \brief This member function writes the
/// embedded window layout to the target file.
///
/// \param sScriptCommand std::string&
/// \return void
///
/////////////////////////////////////////////////
void Script::writeLayout(std::string& sScriptCommand)
{
    // create a valid file name
    std::string sLayoutFileName = getArgAtPos(sScriptCommand, sScriptCommand.find_first_not_of(' ', 6));
    sLayoutFileName = FileSystem::ValidFileName(sLayoutFileName, ".nlyt");
    ofstream fLayoutFile(sLayoutFileName);

    m_logger.push_line(">> Installing layout: \"" + sLayoutFileName + "\" ...");

    // Remove the file name
    size_t nQuotes = 0;

    for (size_t i = sScriptCommand.find_first_not_of(' ', 6); i < sScriptCommand.length(); i++)
    {
        if (isQuotationMark(sScriptCommand, i))
            nQuotes++;

        if (!(nQuotes % 2) && sScriptCommand[i] == ' ')
        {
            sScriptCommand = "layout " + sScriptCommand.substr(i);
            break;
        }
        else if (i+1 == sScriptCommand.length())
            sScriptCommand = "layout";
    }

    // Depending on whether the whole file was written in
    // one line or multiple script lines
    if (sScriptCommand.find("endlayout") != string::npos)
    {
        sScriptCommand.erase(sScriptCommand.find("endlayout")+9);

        // Write the contents to the documentation article file
        if (!fLayoutFile.fail())
        {
            if (nInstallModeFlags & ENABLE_FULL_LOGGING)
                m_logger.push_line(">> >> Copying: " + sScriptCommand + " ...");

            fLayoutFile << sScriptCommand << endl;
        }
        else
        {
            fLayoutFile.close();
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sScriptCommand, SyntaxError::invalid_position, sLayoutFileName);
        }

        fLayoutFile.close();
    }
    else
    {
        // Write the contents linewise to the documentation article file
        if (!fLayoutFile.fail())
        {
            if (nInstallModeFlags & ENABLE_FULL_LOGGING)
                m_logger.push_line(">> >> Copying: " + sScriptCommand + " ...");

            fLayoutFile << sScriptCommand << endl;

            string sTemp;
            size_t nIndent = 1;

            // Read the contents linewise from the script
            while (nLine < m_script->getLinesCount())
            {
                sTemp = m_script->getLine(nLine);
                nLine++;
                StripSpaces(sTemp);

                if (nInstallModeFlags & ENABLE_FULL_LOGGING)
                    m_logger.push_line(">> >> Copying: " + sTemp + " ...");

                if (sTemp.substr(0, 8) == "endgroup" || sTemp.substr(0, 9) == "endlayout")
                    nIndent--;

                sTemp.insert(0, nIndent, '\t');

                // Try to find the end of the current documentation article
                if (sTemp.find("endlayout") == string::npos)
                {
                    // Write the current line
                    fLayoutFile << sTemp << endl;
                }
                else
                {
                    // Write the last line
                    fLayoutFile << sTemp.substr(0, sTemp.find("endlayout")+9) << endl;
                    break;
                }

                if (sTemp.substr(nIndent, 5) == "group")
                    nIndent++;
            }
        }
        else
        {
            fLayoutFile.close();
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sScriptCommand, SyntaxError::invalid_position, sLayoutFileName);
        }

        fLayoutFile.close();
    }

    sScriptCommand.clear();
}


/////////////////////////////////////////////////
/// \brief Writes a procedure including the
/// comments to a procedure file.
///
/// \return void
///
/////////////////////////////////////////////////
void Script::writeProcedure()
{
    std::string sDocumentation;
    Procedure& _procedure = NumeReKernel::getInstance()->getProcedureInterpreter();

    // We detected the start of the procedure in
    // the previous line. Let's compensate for
    // this
    nLine--;

    if (nLine)
    {
        int line = m_script->findDocStartLine(nLine-1);

        if (line > -1)
        {
            while (line < nLine)
            {
                std::string sLine = m_script->getLine(line);
                StripSpaces(sLine);
                sDocumentation += sLine + "\n";
                line++;
            }
        }
    }

    // Write first line including prefixed documentation
    std::string sLine = m_script->getLine(nLine);
    StripSpaces(sLine);

    m_logger.push_line(">> Installing: \"" + sLine.substr(sLine.find('$'), sLine.find('(', sLine.find('$'))-sLine.find('$')) + "\" ...");

    if (!(nInstallModeFlags & DISABLE_SCREEN_OUTPUT))
        NumeReKernel::print(toSystemCodePage(_lang.get("SCRIPT_INSTALLING_PROC", sLine.substr(sLine.find('$'), sLine.find('(', sLine.find('$'))-sLine.find('$')))));

    if (nInstallModeFlags & ENABLE_FULL_LOGGING)
        m_logger.push_line(">> >> Copying: " + sLine + " ...");

    _procedure.writeProcedure(sLine + sDocumentation);
    nLine++;

    // Write remaining lines
    while (nLine < m_script->getLinesCount())
    {
        sLine = m_script->getLine(nLine);
        nLine++;
        StripSpaces(sLine);

        if (nInstallModeFlags & ENABLE_FULL_LOGGING)
            m_logger.push_line(">> >> Copying: " + sLine + " ...");

        _procedure.writeProcedure(sLine);

        if (findCommand(m_script->getStrippedLine(nLine-1)).sString == "endprocedure")
            break;
    }
}


/////////////////////////////////////////////////
/// \brief Writes the contents of a whole file to
/// the target file, which has been specified by
/// the XML-like tags.
///
/// \return bool
///
/////////////////////////////////////////////////
bool Script::writeWholeFile()
{
    std::string sStartLine = m_script->getStrippedLine(nLine-1);
    size_t pos = sStartLine.find("<file ");

    if (pos == std::string::npos || sStartLine.find("name=", pos) == std::string::npos)
        return false;

    std::string sFileName = ValidizeAndPrepareName(Documentation::getArgAtPos(sStartLine, sStartLine.find("name=", pos)+5), ".nscr");
    m_logger.push_line(">> Writing file: \"" + sFileName + "\" ...");
    std::vector<std::string> vFileContents;
    size_t nIndent = UINT_MAX;

    // Buffer the files contents and determine the minimal indent, which may be
    // removed
    while (nLine < m_script->getLinesCount())
    {
        // Get the contents and replace all tab characters with 4 whitespaces
        // (more secure, because users might prefer whitespaces)
        vFileContents.push_back(m_script->getLine(nLine));
        replaceAll(vFileContents.back(), "\t", "    ");

        // Find the first non-whitespace character
        size_t nFirstChar = vFileContents.back().find_first_not_of(' ');

        // Determine, if it is smaller than the already
        // determined index
        if (nFirstChar < nIndent)
            nIndent = nFirstChar;

        if (nInstallModeFlags & ENABLE_FULL_LOGGING)
            m_logger.push_line(">> >> Copying: " + vFileContents.back() + " ...");

        nLine++;

        if (m_script->getStrippedLine(nLine).find("<endfile>") != std::string::npos)
            break;
    }

    nLine++;

    std::ofstream wholeFile(sFileName);

    // Now write the buffer to the file while removing the
    // superfluous indent
    for (size_t i = 0; i < vFileContents.size(); i++)
    {
        wholeFile << vFileContents[i].substr(nIndent) + "\n";
    }

    wholeFile.close();

    return true;
}


/////////////////////////////////////////////////
/// \brief This member function evaluates the
/// flags from the installation information
/// string and also removes unnecessary comments.
///
/// \param sInstallInfoString std::string&
/// \return void
///
/////////////////////////////////////////////////
void Script::evaluateInstallInformation(std::string& sInstallInfoString)
{
    if (sInstallInfoString.length())
    {
        // Evaluate the flag list
        if (findParameter(sInstallInfoString, "flags", '='))
        {
            string sParam = getArgAtPos(sInstallInfoString, findParameter(sInstallInfoString, "flags", '=')+5);

            if (sParam.find("ENABLE_FULL_LOGGING") != string::npos)
                nInstallModeFlags |= ENABLE_FULL_LOGGING;

            if (sParam.find("DISABLE_SCREEN_OUTPUT") != string::npos)
                nInstallModeFlags |= DISABLE_SCREEN_OUTPUT;
        }

        if (!(nInstallModeFlags & DISABLE_SCREEN_OUTPUT))
            NumeReKernel::print(toSystemCodePage(_lang.get("SCRIPT_START_INSTALL")) + " ...");
    }

    // Write the installation information string to the
    // installation logfile
    if (sInstallInfoString.length())
        m_logger.push_line("Installinfo: " + sInstallInfoString);
}


/////////////////////////////////////////////////
/// \brief This member function returns the next
/// valid line from the currently opened script.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string Script::getNextScriptCommandFromScript()
{
    std::string sScriptCommand;
    NumeReKernel* instance = NumeReKernel::getInstance();

    // Search for the next valid and non-empty line
    // in the current script
    while (m_script && nLine < m_script->getLinesCount() && !sScriptCommand.length())
    {
        string sCurrentLine;

        // Compose lines, which were broken using the "\\" operator
        do
        {
            sCurrentLine = m_script->getStrippedLine(nLine);
            nLine++;
            StripSpaces(sCurrentLine);

            if (sScriptCommand.length() > 2 && sScriptCommand.substr(sScriptCommand.length()-2) == "\\\\")
                sScriptCommand.erase(sScriptCommand.length()-2);

            sScriptCommand += sCurrentLine;

            if (instance->getDebugger().getBreakpointManager().isBreakpoint(sScriptFileName, nLine-1)
                || sScriptCommand.starts_with("|>"))
            {
                // Add a breakpoint, if the user has set it in the editor
                if (!sScriptCommand.starts_with("|>"))
                    sScriptCommand.insert(0, "|> ");
                else if (!instance->getDebugger().getBreakpointManager().isBreakpoint(sScriptFileName, nLine-1))
                    instance->getDebugger().getBreakpointManager().addBreakpoint(sScriptFileName, nLine-1, Breakpoint(true));

                Breakpoint bp = instance->getDebugger().getBreakpointManager().getBreakpoint(sScriptFileName, nLine-1);

                if (bp.m_isConditional)
                {
                    bp.m_condition = bp.m_originalCondition;
                    handleLocalDefinitions(bp.m_condition);
                    instance->getDebugger().getBreakpointManager().addBreakpoint(sScriptFileName, nLine-1, bp);
                }
            }
        }
        while (nLine < m_script->getLinesCount() && sScriptCommand.length() > 2 && sScriptCommand.substr(sScriptCommand.length()-2) == "\\\\");

        // Ignore empty lines
        if (!sScriptCommand.length())
            continue;

        // If we find the installation section, then either jump over it
        // or execute it, if the user wants to do so
        if (sScriptCommand.substr(0,9) == "<install>" && !isInstallMode)
        {
            // jump over the installation section
            while (nLine < m_script->getLinesCount())
            {
                sScriptCommand = m_script->getStrippedLine(nLine);
                nLine++;
                StripSpaces(sScriptCommand);

                if (sScriptCommand.substr(0,12) == "<endinstall>")
                    break;
            }

            sScriptCommand.clear();
            continue;
        }
        else if (sScriptCommand.substr(0,9) == "<install>")
        {
            // Execute the installation section
            if (!startInstallation(sScriptCommand))
            {
                // Obviously already installed
                if (!isInInstallSection)
                {
                    bool shallReturn = false;

                    // jump over the installation section
                    while (nLine < m_script->getLinesCount())
                    {
                        sScriptCommand = m_script->getStrippedLine(nLine);
                        nLine++;
                        StripSpaces(sScriptCommand);

                        if (sScriptCommand.substr(0,12) == "<endinstall>")
                            break;

                        if (findCommand(sScriptCommand).sString == "return")
                            shallReturn = true;
                        else if (sScriptCommand.length())
                            shallReturn = false;
                    }

                    if (m_logger.is_open())
                    {
                        m_logger.push("--- NOT INSTALLED: NEWER OR SAME VERSION ALREADY AVAILABLE ---\n\n\n");
                        m_logger.close();
                    }

                    if (shallReturn)
                    {
                        bLastScriptCommand = true;
                        close();
                        return "";
                    }

                    sScriptCommand.clear();
                }

                continue;
            }
        }

        // Get the installation information
        if (sScriptCommand.substr(0,6) == "<info>" && isInstallMode && isInInstallSection)
        {
            if (!handleInstallInformation(sScriptCommand))
            {
                // Obviously already installed
                if (!isInInstallSection)
                {
                    bool shallReturn = false;

                    // jump over the installation section
                    while (nLine < m_script->getLinesCount())
                    {
                        sScriptCommand = m_script->getStrippedLine(nLine);
                        nLine++;
                        StripSpaces(sScriptCommand);

                        if (sScriptCommand.substr(0,12) == "<endinstall>")
                            break;

                        if (findCommand(sScriptCommand).sString == "return")
                            shallReturn = true;
                        else if (sScriptCommand.length())
                            shallReturn = false;
                    }

                    if (m_logger.is_open())
                    {
                        m_logger.push("--- NOT INSTALLED: NEWER OR SAME VERSION ALREADY AVAILABLE ---\n\n\n");
                        m_logger.close();
                    }

                    if (shallReturn)
                    {
                        bLastScriptCommand = true;
                        close();
                        return "";
                    }

                    sScriptCommand.clear();
                }

                continue;
            }
        }

        // Write a whole file from script to file
        if (sScriptCommand.substr(0,6) == "<file " && isInstallMode && isInInstallSection)
        {
            if (writeWholeFile())
            {
                sScriptCommand.clear();
                continue;
            }
        }

        // Write the documentation articles to their corresponding files
        if (sScriptCommand.substr(0,10) == "<helpfile>" && isInstallMode && isInInstallSection)
        {
            writeDocumentationArticle(sScriptCommand);
            continue;
        }

        // Write window layouts
        if (findCommand(sScriptCommand).sString == "layout" && isInstallMode && isInInstallSection)
        {
            writeLayout(sScriptCommand);
            continue;
        }

        // Write procedures
        if (findCommand(sScriptCommand).sString == "procedure" && isInstallMode && isInInstallSection)
        {
            writeProcedure();
            sScriptCommand.clear();
            continue;
        }

        // End the installation
        if (sScriptCommand.substr(0, 12) == "<endinstall>" && isInstallMode && isInInstallSection)
        {
            if (m_logger.is_open())
            {
                m_logger.push("--- INSTALLATION TERMINATED SUCCESSFULLY ---\n\n\n");
                m_logger.close();

                if (!(nInstallModeFlags & DISABLE_SCREEN_OUTPUT))
                    NumeReKernel::print(toSystemCodePage(_lang.get("SCRIPT_INSTALL_SUCCESS")));

                NumeReKernel::installationDone();
            }

            isInInstallSection = false;
            sScriptCommand = sScriptCommand.substr(12);

            if (!sScriptCommand.length())
                continue;
        }
        else if (isInstallMode && isInInstallSection && (nInstallModeFlags & ENABLE_FULL_LOGGING))
            m_logger.push_line(">> Evaluating: " + sScriptCommand + " ...");
    }

    // close the script, if this is the last command
    if (!m_script || nLine >= m_script->getLinesCount())
    {
        bLastScriptCommand = true;
        Script::close();
    }

    return sScriptCommand;
}


/////////////////////////////////////////////////
/// \brief This member function returns the next
/// valid line from the included script.
///
/// \return string
///
/////////////////////////////////////////////////
string Script::getNextScriptCommandFromInclude()
{
    if (m_include && m_include->is_open())
        return m_include->getNextLine();

    return "";
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// script include syntax ("@SCRIPT") and
/// prepares the included file stream.
///
/// \param sScriptCommand string&
/// \return string
///
/////////////////////////////////////////////////
string Script::handleIncludeSyntax(string& sScriptCommand)
{
    // Only accept the including syntax, if we're currently
    // not from another included file
    if (!m_include && Includer::is_including_syntax(sScriptCommand))
    {
        // Open the include file
        std::string sFileName = m_script->getFileName();
        m_include.reset(new Includer(sScriptCommand, sFileName.substr(0, sFileName.rfind('/'))));

        // Ensure that the file is valid
        if (!m_include->is_open())
        {
            m_include.reset();
            throw SyntaxError(SyntaxError::SCRIPT_NOT_EXIST, sScriptCommand, SyntaxError::invalid_position, sScriptCommand);
        }

        return "";
    }

    return sScriptCommand;
}


/////////////////////////////////////////////////
/// \brief This private member function handles
/// the definition and replacement of local
/// functions.
///
/// \param sScriptCommand string&
/// \return bool
///
/////////////////////////////////////////////////
bool Script::handleLocalDefinitions(string& sScriptCommand)
{
    std::string sCommand = findCommand(sScriptCommand).sString;

    // If the current command contains the command "lclfunc",
    // then this is a definition
    if (sCommand == "lclfunc")
    {
        _localDef.defineFunc(sScriptCommand.substr(sCommand.length()));
        sScriptCommand.clear();
        return false;
    }
    else if (sCommand == SYMDEF_COMMAND)
    {
        _symdefs.createSymbol(sScriptCommand.substr(sCommand.length()));
        sScriptCommand.clear();
        return false;
    }
    else
    {
        // Simply replace the current call
        _localDef.call(sScriptCommand);
        _symdefs.resolveSymbols(sScriptCommand);
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Handle the installation of a package,
/// if the corresponding file cannot be found
/// locally.
///
/// \param sPkgId const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool Script::handleRepositoryInstall(const std::string& sPkgId)
{
    std::string sPackageFileName = ValidFileName("packages/" + sPkgId, ".nscr");
    close();
    isInstallMode = true;

    // Search in the local storage first
    if (fileExists(sPackageFileName))
    {
        openScript(sPackageFileName, 0);
        return true;
    }

    NumeReKernel::printPreFmt("|-> PACKAGE REPOSITORY: " + _lang.get("SCRIPT_REPO_SEARCHING", sPkgId));
    PackageInfo pkgInfo = m_repo.find(sPkgId);

    if (pkgInfo.repoUrl.length())
    {
        NumeReKernel::printPreFmt(" " + _lang.get("SCRIPT_REPO_FOUND", pkgInfo.name, pkgInfo.version));

        if (m_repo.download(pkgInfo.repoUrl, sPackageFileName))
        {
            NumeReKernel::printPreFmt(" " + _lang.get("SCRIPT_REPO_DOWNLOADED") + "\n");
            openScript(sPackageFileName, 0);
            return true;
        }
        else
            NumeReKernel::printPreFmt(" " + _lang.get("SCRIPT_REPO_ERROR") + "\n");
    }
    else
        NumeReKernel::printPreFmt(" " + _lang.get("SCRIPT_REPO_NOTFOUND") + "\n");

    return false;
}


/////////////////////////////////////////////////
/// \brief This member function is the main
/// interface to the internal managed script. It
/// will always return the next valid script line.
///
/// \return string
///
/////////////////////////////////////////////////
string Script::getNextScriptCommand()
{
    std::string sScriptCommand = "";

    // Get the next script command
    if (m_script)
    {
        if (!m_include || !m_include->is_open())
        {
            // Delete the includer, if it is not needed anymore
            if (m_include)
                m_include.reset();

            // Get the next script command from the currently opened script
            sScriptCommand = getNextScriptCommandFromScript();
        }
        else
        {
            // Get the next include string from the included script
            sScriptCommand = getNextScriptCommandFromInclude();
        }
    }
    else
        return "";

    // Replace "<this>" path tokens with the current script file path
    while (sScriptCommand.find("<this>") != string::npos)
        sScriptCommand.replace(sScriptCommand.find("<this>"), 6, sScriptFileName.substr(0, sScriptFileName.rfind('/')));

    // Handle the include syntax ("@SOMESCRIPT") and everything, what
    // belongs to it
    sScriptCommand = handleIncludeSyntax(sScriptCommand);

    // Ensure that procedures are not written accidentally
    if (!isInstallMode
        && sScriptCommand.find("procedure") != string::npos
        && sScriptCommand.find('$', sScriptCommand.find("procedure")) != string::npos)
        throw SyntaxError(SyntaxError::PROCEDURE_WITHOUT_INSTALL_FOUND, sScriptCommand, SyntaxError::invalid_position);

    // If we're not installing, replace all local functions
    if (!isInstallMode)
    {
        if (!handleLocalDefinitions(sScriptCommand))
            return "";
    }

    // Return the script command for evaluation
    return sScriptCommand;
}



