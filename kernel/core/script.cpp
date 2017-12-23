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

Script::Script() : FileSystem()
{
    sScriptFileName = "";
    sIncludeFileName = "";
    sInstallInfoString = "";
    sHelpID = "";
    sInstallID = "";
    bValidScript = false;
    bScriptOpen = false;
    bReadFromInclude = false;
    bAppendNextLine = false;
    bAutoStart = false;
    bLastScriptCommand = false;
    bBlockComment = false;
    bInstallProcedures = false;
    bENABLE_FULL_LOGGING = false;
    bDISABLE_SCREEN_OUTPUT = false;
    bIsInstallingProcedure = false;
    nCurrentPackage = 0;
    nLine = 0;
    nIncludeLine = 0;
    nIncludeType = 0;
}

Script::Script(const string& _sScriptFileName) : FileSystem()
{
    Script();
    sScriptFileName = _sScriptFileName;
}

Script::~Script()
{
    if (bScriptOpen)
    {
        Script::close();
    }
}

string Script::stripLineComments(const string& sLine)
{
    size_t nQuotes = 0;

    for (size_t i = 0; i < sLine.length(); i++)
    {
        if (sLine[i] == '"'
            && (!i || (i && sLine[i-1] != '\\')))
            nQuotes++;
        if (sLine.substr(i,2) == "##" && !(nQuotes % 2))
            return sLine.substr(0,i);
    }
    return sLine;
}

string Script::stripBlockComments(const string& sLine)
{
    size_t nQuotes = 0;
    string sReturn = sLine;

    for (size_t i = 0; i < sReturn.length(); i++)
    {
        if (sReturn[i] == '"'
            && (!i || (i && sReturn[i-1] != '\\')))
            nQuotes++;
        if (sReturn.substr(i,2) == "#*" && !(nQuotes % 2))
        {
            for (size_t j = i+2; j < sReturn.length(); j++)
            {
                if (sReturn.substr(j,2) == "*#")
                {
                    sReturn.erase(i,j+2-i);
                    break;
                }
                if (j+1 == sReturn.length())
                {
                    sReturn.erase(i);
                    bBlockComment = true;
                    break;
                }
            }
        }
    }
    return sReturn;
}

void Script::openScript()
{
    if (fScript.is_open())
        fScript.close();
    if (sScriptFileName.length())
    {
        fScript.open(sScriptFileName.c_str(), ios_base::in);
        bScriptOpen = true;
        if (fScript.fail())
        {
            string sErrorToken = sScriptFileName;
            close();
            sScriptFileName = "";
            throw SyntaxError(SyntaxError::SCRIPT_NOT_EXIST, "", SyntaxError::invalid_position, sErrorToken);
        }
        bReadFromInclude = false;
        bAppendNextLine = false;
        bValidScript = true;
        bAutoStart = false;
        bENABLE_FULL_LOGGING = false;
        bDISABLE_SCREEN_OUTPUT = false;
        bIsInstallingProcedure = false;
        sInstallInfoString = "";
        sHelpID = "";
        sInstallID = "";
        nLine = 0;
        nIncludeLine = 0;
    }
    return;
}

void Script::openScript(string& _sScriptFileName)
{
    setScriptFileName(_sScriptFileName);
    openScript();
    return;
}

void Script::close()
{
    if (bScriptOpen)
    {
        fScript.close();
        if (fInclude.is_open())
            fInclude.close();
        if (fLogFile.is_open())
        {
            fLogFile << "--- INSTALLATION FAILED ---" << endl << endl << endl;
            fLogFile.close();
        }
        bBlockComment = false;
        bAppendNextLine = false;
        bReadFromInclude = false;
        bAutoStart = false;
        if (vInstallPackages.size() > nCurrentPackage+1)
        {
            bLastScriptCommand = false;
            nCurrentPackage++;
            openScript(vInstallPackages[nCurrentPackage]);
            return;
        }
        if (vInstallPackages.size())
            sScriptFileName = vInstallPackages[0];
        vInstallPackages.clear();
        nCurrentPackage = 0;
        bScriptOpen = false;
        sIncludeFileName = "";
        bValidScript = false;
        bInstallProcedures = false;
        bENABLE_FULL_LOGGING = false;
        bDISABLE_SCREEN_OUTPUT = false;
        bIsInstallingProcedure = false;
        sHelpID = "";
        sInstallID = "";
        nLine = 0;
    }
    return;
}

void Script::restart()
{
    if (bScriptOpen)
    {
        fScript.clear();
        if (fInclude.is_open())
            fInclude.close();
        fScript.seekg(0);
    }
    else if (sScriptFileName.length())
    {
        openScript();
        fScript.clear();
        if (fInclude.is_open())
            fInclude.close();
        fScript.seekg(0);
    }
    return;
}

string Script::getNextScriptCommand()
{
    string sScriptCommand = "";
    bool bFirstPassedInstallCommand = false;
    if (bScriptOpen)
    {
        if (!bReadFromInclude)
        {
            while (!fScript.eof() && (!sScriptCommand.length() || sScriptCommand.substr(0,2) == "##"))
            {
                getline(fScript, sScriptCommand);
                nLine++;
                StripSpaces(sScriptCommand);
                if (NumeReKernel::_messenger.isBreakpoint(sScriptFileName, nLine-1) && sScriptCommand.substr(0,2) != "|>")
                    sScriptCommand.insert(0, "|> ");
                if (!sScriptCommand.length())
                    continue;
                if (bBlockComment && sScriptCommand.find("*#") != string::npos)
                {
                    bBlockComment = false;
                    if (sScriptCommand.find("*#") == sScriptCommand.length()-2)
                    {
                        sScriptCommand = "";
                        continue;
                    }
                    else
                        sScriptCommand = sScriptCommand.substr(sScriptCommand.find("*#")+2);
                }
                else if (bBlockComment && sScriptCommand.find("*#") == string::npos)
                {
                    sScriptCommand = "";
                    continue;
                }
                if (sScriptCommand.find("##") != string::npos)
                {
                    sScriptCommand = stripLineComments(sScriptCommand);
                    if (!sScriptCommand.length())
                        continue;
                }
                if (sScriptCommand.substr(0,9) == "<install>" && !bInstallProcedures)
                {
                    while (!fScript.eof())
                    {
                        getline(fScript, sScriptCommand);
                        nLine++;
                        StripSpaces(sScriptCommand);
                        if (sScriptCommand.substr(0,12) == "<endinstall>")
                            break;
                    }
                    sScriptCommand = "";
                    continue;
                }
                else if (sScriptCommand.substr(0,9) == "<install>")
                {
                    fLogFile.open((sTokens[0][1] + "\\install.log").c_str(), /*ios_base::ate |*/ ios_base::in | ios_base::out | ios_base::app);
                    if (fLogFile.fail())
                        throw SyntaxError(SyntaxError::CANNOT_OPEN_LOGFILE, sScriptCommand, SyntaxError::invalid_position, sTokens[0][1] + "\\install.log");
                    fLogFile << "--- INSTALLATION " << getTimeStamp(false) << " ---" << endl;
                    sInstallInfoString = "-flags=ENABLE_DEFAULTS -type=TYPE_UNSPECIFIED -name=<AUTO> -author=<AUTO>";
                    sInstallID = getArgAtPos(sInstallInfoString, sInstallInfoString.find("name=")+5);
                    bFirstPassedInstallCommand = true;
                    sScriptCommand = sScriptCommand.substr(9);
                    StripSpaces(sScriptCommand);
                    if (!sScriptCommand.length())
                        continue;
                    if (sScriptCommand.find("<info>") == string::npos)
                    {
                        sScriptCommand = "";
                        continue;
                    }
                    else
                    {
                        unsigned int nNumereVersion = AutoVersion::MAJOR*100+AutoVersion::MINOR*10+AutoVersion::BUILD;
                        unsigned int nRequiredVersion = nNumereVersion;
                        if (sScriptCommand.find("<endinfo>") != string::npos)
                        {
                            sInstallInfoString = sScriptCommand.substr(sScriptCommand.find("<info>")+6, sScriptCommand.find("<endinfo>")-sScriptCommand.find("<info>")-6);
                            sScriptCommand = sScriptCommand.substr(sScriptCommand.find("<endinfo>")+9);
                            sInstallID = getArgAtPos(sInstallInfoString, sInstallInfoString.find("name=")+5);
                            if (sInstallInfoString.find("requirepackages=") != string::npos)
                            {
                                string sInstallPackages = getArgAtPos(sInstallInfoString, sInstallInfoString.find("requirepackages=")+16);
                                while (sInstallPackages.length())
                                {
                                    string sPackage = ValidFileName(sInstallPackages.substr(0,sInstallPackages.find(',')),".nscr");
                                    if (sInstallPackages.find(',') != string::npos)
                                        sInstallPackages.erase(0,sInstallPackages.find(',')+1);
                                    else
                                        sInstallPackages.clear();
                                    if (!sPackage.length())
                                        continue;
                                    if (!vInstallPackages.size())
                                    {
                                        vInstallPackages.push_back(sScriptFileName);
                                        vInstallPackages.push_back(sPackage);
                                    }
                                    for (unsigned int i = 0; i < vInstallPackages.size(); i++)
                                    {
                                        if (vInstallPackages[i] == sPackage)
                                            break;
                                        else if (i+1 == vInstallPackages.size())
                                        {
                                            vInstallPackages.push_back(sPackage);
                                            break;
                                        }
                                    }
                                }
                            }
                            if (sInstallInfoString.find("requireversion=") != string::npos)
                            {
                                string sTemp = getArgAtPos(sInstallInfoString, sInstallInfoString.find("requireversion=")+15);
                                while (sTemp.find('.') != string::npos)
                                    sTemp.erase(sTemp.find('.'),1);
                                nRequiredVersion = (unsigned int)StrToInt(sTemp);
                            }
                            if (!sScriptCommand.length())
                                continue;
                        }
                        else
                        {
                            string sTemp = "";
                            sScriptCommand = sScriptCommand.substr(6);
                            while (!fScript.eof())
                            {
                                getline(fScript, sTemp);
                                nLine++;
                                StripSpaces(sTemp);
                                if (sTemp.find("##") != string::npos)
                                {
                                    sTemp = stripLineComments(sTemp);
                                }
                                if (sTemp.find("<endinfo>") == string::npos)
                                    sScriptCommand += " " + sTemp;
                                else
                                {
                                    sScriptCommand += " " + sTemp.substr(0,sTemp.find("<endinfo>"));
                                    break;
                                }
                            }
                            sInstallInfoString = sScriptCommand;
                            sScriptCommand = sTemp.substr(sTemp.find("<endinfo>")+9);
                            sInstallID = getArgAtPos(sInstallInfoString, sInstallInfoString.find("name=")+5);
                            if (sInstallInfoString.find("requirepackages=") != string::npos)
                            {
                                string sInstallPackages = getArgAtPos(sInstallInfoString, sInstallInfoString.find("requirepackages=")+16);
                                while (sInstallPackages.length())
                                {
                                    string sPackage = ValidFileName(sInstallPackages.substr(0,sInstallPackages.find(',')),".nscr");
                                    if (sInstallPackages.find(',') != string::npos)
                                        sInstallPackages.erase(0,sInstallPackages.find(',')+1);
                                    else
                                        sInstallPackages.clear();
                                    if (!sPackage.length())
                                        continue;
                                    if (!vInstallPackages.size())
                                    {
                                        vInstallPackages.push_back(sScriptFileName);
                                        vInstallPackages.push_back(sPackage);
                                    }
                                    for (unsigned int i = 0; i < vInstallPackages.size(); i++)
                                    {
                                        if (vInstallPackages[i] == sPackage)
                                            break;
                                        else if (i+1 == vInstallPackages.size())
                                        {
                                            vInstallPackages.push_back(sPackage);
                                            break;
                                        }
                                    }
                                }
                            }
                            if (sInstallInfoString.find("requireversion=") != string::npos)
                            {
                                string sTemp = getArgAtPos(sInstallInfoString, sInstallInfoString.find("requireversion=")+15);
                                while (sTemp.find('.') != string::npos)
                                    sTemp.erase(sTemp.find('.'),1);
                                nRequiredVersion = (unsigned int)StrToInt(sTemp);
                            }
                            if (!sScriptCommand.length())
                                continue;
                        }
                        if (nRequiredVersion > nNumereVersion)
                            throw SyntaxError(SyntaxError::INSUFFICIENT_NUMERE_VERSION, sScriptCommand, SyntaxError::invalid_position, toString(nRequiredVersion));
                        if (!sScriptCommand.length())
                            continue;
                    }
                }

                if (sScriptCommand.substr(0,6) == "<info>" && bInstallProcedures)
                {
                    unsigned int nNumereVersion = AutoVersion::MAJOR*100+AutoVersion::MINOR*10+AutoVersion::BUILD;
                    unsigned int nRequiredVersion = nNumereVersion;
                    bFirstPassedInstallCommand = true;
                    if (sScriptCommand.find("<endinfo>") != string::npos)
                    {
                        sInstallInfoString = sScriptCommand.substr(sScriptCommand.find("<info>")+6, sScriptCommand.find("<endinfo>")-sScriptCommand.find("<info>")-6);
                        sScriptCommand = sScriptCommand.substr(sScriptCommand.find("<endinfo>")+9);
                        sInstallID = getArgAtPos(sInstallInfoString, sInstallInfoString.find("name=")+5);
                        if (sInstallInfoString.find("requirepackages=") != string::npos)
                        {
                            string sInstallPackages = getArgAtPos(sInstallInfoString, sInstallInfoString.find("requirepackages=")+16);
                            while (sInstallPackages.length())
                            {
                                string sPackage = ValidFileName(sInstallPackages.substr(0,sInstallPackages.find(',')),".nscr");
                                if (sInstallPackages.find(',') != string::npos)
                                    sInstallPackages.erase(0,sInstallPackages.find(',')+1);
                                else
                                    sInstallPackages.clear();
                                if (!sPackage.length())
                                    continue;
                                if (!vInstallPackages.size())
                                {
                                    vInstallPackages.push_back(sScriptFileName);
                                    vInstallPackages.push_back(sPackage);
                                }
                                for (unsigned int i = 0; i < vInstallPackages.size(); i++)
                                {
                                    if (vInstallPackages[i] == sPackage)
                                        break;
                                    else if (i+1 == vInstallPackages.size())
                                    {
                                        vInstallPackages.push_back(sPackage);
                                        break;
                                    }
                                }
                            }
                        }
                        if (sInstallInfoString.find("requireversion=") != string::npos)
                        {
                            string sTemp = getArgAtPos(sInstallInfoString, sInstallInfoString.find("requireversion=")+15);
                            while (sTemp.find('.') != string::npos)
                                sTemp.erase(sTemp.find('.'),1);
                            nRequiredVersion = (unsigned int)StrToInt(sTemp);
                        }
                    }
                    else
                    {
                        string sTemp = "";
                        sScriptCommand = sScriptCommand.substr(6);
                        while (!fScript.eof())
                        {
                            getline(fScript, sTemp);
                            nLine++;
                            StripSpaces(sTemp);
                            if (sTemp.find("##") != string::npos)
                                sTemp = stripLineComments(sTemp);
                            if (sTemp.find("<endinfo>") == string::npos)
                                sScriptCommand += " " + sTemp;
                            else
                            {
                                sScriptCommand += " " + sTemp.substr(0,sTemp.find("<endinfo>"));
                                break;
                            }
                        }
                        sInstallInfoString = sScriptCommand;
                        sScriptCommand = sTemp.substr(sTemp.find("<endinfo>")+9);
                        sInstallID = getArgAtPos(sInstallInfoString, sInstallInfoString.find("name=")+5);
                        if (sInstallInfoString.find("requirepackages=") != string::npos)
                        {
                            string sInstallPackages = getArgAtPos(sInstallInfoString, sInstallInfoString.find("requirepackages=")+16);
                            while (sInstallPackages.length())
                            {
                                string sPackage = ValidFileName(sInstallPackages.substr(0,sInstallPackages.find(',')),".nscr");
                                if (sInstallPackages.find(',') != string::npos)
                                    sInstallPackages.erase(0,sInstallPackages.find(',')+1);
                                else
                                    sInstallPackages.clear();
                                if (!sPackage.length())
                                    continue;
                                if (!vInstallPackages.size())
                                {
                                    vInstallPackages.push_back(sScriptFileName);
                                    vInstallPackages.push_back(sPackage);
                                }
                                for (unsigned int i = 0; i < vInstallPackages.size(); i++)
                                {
                                    if (vInstallPackages[i] == sPackage)
                                        break;
                                    else if (i+1 == vInstallPackages.size())
                                    {
                                        vInstallPackages.push_back(sPackage);
                                        break;
                                    }
                                }
                            }
                        }
                        if (sInstallInfoString.find("requireversion=") != string::npos)
                        {
                            string sTemp = getArgAtPos(sInstallInfoString, sInstallInfoString.find("requireversion=")+15);
                            while (sTemp.find('.') != string::npos)
                                sTemp.erase(sTemp.find('.'),1);
                            nRequiredVersion = (unsigned int)StrToInt(sTemp);
                        }
                    }
                    if (nRequiredVersion > nNumereVersion)
                        throw SyntaxError(SyntaxError::INSUFFICIENT_NUMERE_VERSION, sScriptCommand, SyntaxError::invalid_position, toString(nRequiredVersion));
                    if (!sScriptCommand.length())
                        continue;
                }

                if (sScriptCommand.substr(0,11) == "<helpindex>" && bInstallProcedures)
                {
                    if (sScriptCommand.find("</helpindex>") != string::npos)
                    {
                        sScriptCommand.erase(sScriptCommand.find("</helpindex>")+12);
                    }
                    else
                    {
                        string sTemp = "";
                        while (!fScript.eof())
                        {
                            getline(fScript, sTemp);
                            nLine++;
                            StripSpaces(sTemp);
                            if (sTemp.find("##") != string::npos)
                                sTemp = stripLineComments(sTemp);
                            if (sTemp.find("</helpindex>") == string::npos)
                                sScriptCommand += sTemp;
                            else
                            {
                                sScriptCommand += sTemp.substr(0,sTemp.find("</helpindex>")+12);
                                break;
                            }
                        }
                    }
                    sHelpID = getArgAtPos(sScriptCommand, sScriptCommand.find("id=")+3);
                    StripSpaces(sHelpID);
                    if (sHelpID.substr(0,5) != "plgn_")
                    {
                        sScriptCommand.insert(sScriptCommand.find(sHelpID, sScriptCommand.find("id=")+3), "plgn_");
                        sHelpID = "plgn_" + sHelpID;
                    }
                    return sInstallID + " <<>>" + sScriptCommand;
                }

                if (sScriptCommand.substr(0,10) == "<helpfile>" && bInstallProcedures)
                {
                    if (!sHelpID.length())
                    {
                        throw SyntaxError(SyntaxError::HLPIDX_ENTRY_IS_MISSING, sScriptCommand, SyntaxError::invalid_position);
                    }
                    if (sScriptCommand.find("</helpfile>") != string::npos)
                    {
                        sScriptCommand.erase(sScriptCommand.find("</helpfile>")+11);
                        string sHelpfileName = "<>/docs/plugins/" + sHelpID + ".nhlp";
                        sHelpfileName = FileSystem::ValidFileName(sHelpfileName, ".nhlp");
                        ofstream fHelpfile;
                        fHelpfile.open(sHelpfileName.c_str());
                        if (!fHelpfile.fail())
                            fHelpfile << sScriptCommand.substr(10, sScriptCommand.find("</helpfile>")) << endl;
                        else
                        {
                            fHelpfile.close();
                            //sErrorToken = sHelpfileName;
                            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sScriptCommand, SyntaxError::invalid_position, sHelpfileName);
                        }
                        fHelpfile.close();
                    }
                    else
                    {
                        string sHelpfileName = "<>/docs/plugins/" + sHelpID + ".nhlp";
                        sHelpfileName = FileSystem::ValidFileName(sHelpfileName, ".nhlp");
                        ofstream fHelpfile;
                        fHelpfile.open(sHelpfileName.c_str());
                        if (!fHelpfile.fail())
                        {
                            if (sScriptCommand.length() > 10 && sScriptCommand.find("<article") != string::npos)
                            {
                                if (getArgAtPos(sScriptCommand, sScriptCommand.find("id=")+3).substr(0,5) != "plgn_"
                                    && "plgn_"+getArgAtPos(sScriptCommand, sScriptCommand.find("id=")+3) == sHelpID)
                                {
                                    sScriptCommand.insert(sScriptCommand.find(getArgAtPos(sScriptCommand, sScriptCommand.find("id=")+3)), "plgn_");
                                }
                            }
                            if (sScriptCommand.length() > 10)
                                fHelpfile << sScriptCommand.substr(10) << endl;

                            string sTemp;
                            while (!fScript.eof())
                            {
                                getline(fScript, sTemp);
                                nLine++;
                                StripSpaces(sTemp);
                                if (sTemp.find("##") != string::npos)
                                    sTemp = stripLineComments(sTemp);
                                if (sTemp.find("</helpfile>") == string::npos)
                                {
                                    if (sTemp.length() > 10 && sTemp.find("<article") != string::npos)
                                    {
                                        if (getArgAtPos(sTemp, sTemp.find("id=")+3).substr(0,5) != "plgn_"
                                            && "plgn_"+getArgAtPos(sTemp, sTemp.find("id=")+3) == sHelpID)
                                        {
                                            sTemp.insert(sTemp.find(getArgAtPos(sTemp, sTemp.find("id=")+3)), "plgn_");
                                        }
                                    }
                                    fHelpfile << sTemp << endl;
                                }
                                else
                                {
                                    fHelpfile << sTemp.substr(0,sTemp.find("</helpfile>")) << endl;
                                    break;
                                }
                            }
                        }
                        else
                        {
                            fHelpfile.close();
                            //sErrorToken = sHelpfileName;
                            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sScriptCommand, SyntaxError::invalid_position, sHelpfileName);
                        }
                        fHelpfile.close();
                    }
                    sScriptCommand.clear();
                    continue;
                }

                if (sInstallInfoString.length() && !bFirstPassedInstallCommand)
                    sInstallInfoString = "";

                if (sInstallInfoString.length())
                {
                    if (sInstallInfoString.find("#*") != string::npos && sInstallInfoString.find("*#", sInstallInfoString.find("#*")+2) != string::npos)
                    {
                        for (unsigned int i = 0; i < sInstallInfoString.length(); i++)
                        {
                            if (sInstallInfoString.substr(i,2) == "#*")
                            {
                                if (sInstallInfoString.find("*#", i+2) == string::npos)
                                {
                                    sInstallInfoString.erase(i);
                                    break;
                                }
                                for (unsigned int j = i; j < sInstallInfoString.length(); j++)
                                {
                                    if (sInstallInfoString.substr(j,2) == "*#")
                                    {
                                        sInstallInfoString.erase(i,j+2-i);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    if (matchParams(sInstallInfoString, "flags", '='))
                    {
                        string sParam = getArgAtPos(sInstallInfoString, matchParams(sInstallInfoString, "flags", '=')+5);
                        if (sParam.find("ENABLE_FULL_LOGGING") != string::npos)
                        {
                            bENABLE_FULL_LOGGING = true;
                        }
                        if (sParam.find("DISABLE_SCREEN_OUTPUT") != string::npos)
                            bDISABLE_SCREEN_OUTPUT = true;
                    }
                    if (!bDISABLE_SCREEN_OUTPUT)
                        NumeReKernel::print(toSystemCodePage(_lang.get("SCRIPT_START_INSTALL")) + " ...");
                }

                if (sInstallInfoString.length())
                {
                    fLogFile << "Installinfo: " << sInstallInfoString << endl;
                    if (!bFirstPassedInstallCommand)
                        sInstallInfoString.clear();
                }

                if (sScriptCommand.substr(0,12) == "<endinstall>")
                {
                    if (fLogFile.is_open())
                    {
                        fLogFile << "--- INSTALLATION TERMINATED SUCCESSFULLY ---" << endl;
                        fLogFile << endl << endl;
                        fLogFile.close();
                        if (!bDISABLE_SCREEN_OUTPUT)
                            NumeReKernel::print(toSystemCodePage(_lang.get("SCRIPT_INSTALL_SUCCESS")));
                    }
                    bIsInstallingProcedure = false;
                    sScriptCommand = sScriptCommand.substr(12);
                    if (!sScriptCommand.length())
                        continue;
                }

                if (sScriptCommand.substr(0,2) == "#*" && sScriptCommand.find("*#",2) == string::npos)
                {
                    bBlockComment = true;
                    sScriptCommand = "";
                    continue;
                }
                if (bBlockComment && sScriptCommand.find("*#") != string::npos)
                {
                    bBlockComment = false;
                    if (sScriptCommand.find("*#") == sScriptCommand.length()-2)
                    {
                        sScriptCommand = "";
                        continue;
                    }
                    else
                        sScriptCommand = sScriptCommand.substr(sScriptCommand.find("*#")+2);
                }
                else if (bBlockComment && sScriptCommand.find("*#") == string::npos)
                {
                    sScriptCommand = "";
                    continue;
                }
            }

            if (fScript.eof())
            {
                bLastScriptCommand = true;
                Script::close();
            }
        }
        else
        {
            bool bSkipNextLine = false;
            while (!fInclude.eof() && (!sScriptCommand.length() || sScriptCommand.substr(0,2) == "##"))
            {
                getline(fInclude, sScriptCommand);
                nIncludeLine++;
                StripSpaces(sScriptCommand);
                if (!sScriptCommand.length())
                    continue;

                if (sScriptCommand.substr(0,9) == "<install>"
                    || (findCommand(sScriptCommand).sString == "global" && sScriptCommand.find("<install>") != string::npos))
                {
                    while (!fInclude.eof())
                    {
                        getline(fInclude, sScriptCommand);
                        nIncludeLine++;
                        StripSpaces(sScriptCommand);
                        if (sScriptCommand.substr(0,12) == "<endinstall>"
                            || (findCommand(sScriptCommand).sString == "global" && sScriptCommand.find("<endinstall>") != string::npos))
                            break;
                    }
                    sScriptCommand = "";
                    continue;
                }


                if (sScriptCommand.substr(0,2) == "#*" && sScriptCommand.find("*#",2) == string::npos)
                {
                    bBlockComment = true;
                    sScriptCommand = "";
                    continue;
                }
                if (bBlockComment && sScriptCommand.find("*#") != string::npos)
                {
                    bBlockComment = false;
                    if (sScriptCommand.find("*#") == sScriptCommand.length()-2)
                    {
                        sScriptCommand = "";
                        continue;
                    }
                    else
                        sScriptCommand = sScriptCommand.substr(sScriptCommand.find("*#")+2);
                }
                else if (bBlockComment && sScriptCommand.find("*#") == string::npos)
                {
                    sScriptCommand = "";
                    continue;
                }
                if (bSkipNextLine && sScriptCommand.length() > 2 && sScriptCommand.substr(sScriptCommand.length()-2) == "\\\\")
                {
                    sScriptCommand = "";
                    continue;
                }
                else if (bSkipNextLine)
                {
                    bSkipNextLine = false;
                    sScriptCommand = "";
                    continue;
                }
                if (findCommand(sScriptCommand).sString != "define"
                    && findCommand(sScriptCommand).sString != "ifndef"
                    && findCommand(sScriptCommand).sString != "ifndefined"
                    && findCommand(sScriptCommand).sString != "redefine"
                    && findCommand(sScriptCommand).sString != "redef"
                    && findCommand(sScriptCommand).sString != "global"
                    && !bAppendNextLine)
                {
                    if (sScriptCommand.length() > 2 && sScriptCommand.substr(sScriptCommand.length()-2) == "\\\\")
                        bSkipNextLine = true;
                    sScriptCommand = "";
                    continue;
                }
                if (nIncludeType == 1
                    && findCommand(sScriptCommand).sString != "define"
                    && findCommand(sScriptCommand).sString != "ifndef"
                    && findCommand(sScriptCommand).sString != "ifndefined"
                    && findCommand(sScriptCommand).sString != "redefine"
                    && findCommand(sScriptCommand).sString != "redef"
                    && !bAppendNextLine)
                {
                    if (sScriptCommand.length() > 2 && sScriptCommand.substr(sScriptCommand.length()-2) == "\\\\")
                        bSkipNextLine = true;
                    sScriptCommand = "";
                    continue;
                }
                else if (nIncludeType == 2
                    && findCommand(sScriptCommand).sString != "global"
                    && !bAppendNextLine)
                {
                    if (sScriptCommand.length() > 2 && sScriptCommand.substr(sScriptCommand.length()-2) == "\\\\")
                        bSkipNextLine = true;
                    sScriptCommand = "";
                    continue;
                }
                if (sScriptCommand.length() > 2 && sScriptCommand.substr(sScriptCommand.length()-2) == "\\\\")
                    bAppendNextLine = true;
                else
                    bAppendNextLine = false;
            }

            if (fInclude.eof())
            {
                fInclude.close();
                nIncludeLine = 0;
                bReadFromInclude = false;
                nIncludeType = 0;
            }

        }
    }
    else
        return "";

    if (sScriptCommand.find("##") != string::npos)
    {
        sScriptCommand = stripLineComments(sScriptCommand);
    }

    if (sScriptCommand.find("#*") != string::npos)
        sScriptCommand = stripBlockComments(sScriptCommand);
    /*while (sScriptCommand.find("#*") != string::npos)
    {
        if (sScriptCommand.find("*#", sScriptCommand.find("#*")+2) != string::npos)
            sScriptCommand = sScriptCommand.substr(0,sScriptCommand.find("#*")) + sScriptCommand.substr(sScriptCommand.find("*#", sScriptCommand.find("#*")+2)+2);
        else
        {
            sScriptCommand = sScriptCommand.substr(0,sScriptCommand.find("#*"));
            bBlockComment = true;
            break;
        }
    }*/

    //cerr << sScriptFileName.substr(0,sScriptFileName.rfind('/')) << endl;
    while (sScriptCommand.find("<this>") != string::npos)
        sScriptCommand.replace(sScriptCommand.find("<this>"), 6, sScriptFileName.substr(0,sScriptFileName.rfind('/')));


    //cerr << sScriptCommand << endl;
    if (!bReadFromInclude && sScriptCommand[0] == '@' && sScriptCommand[1] != ' ')
    {
        if (sScriptCommand[1] == '"')
            sIncludeFileName = sScriptCommand.substr(2,sScriptCommand.find('"', 2)-2);
        else
            sIncludeFileName = sScriptCommand.substr(1,sScriptCommand.find(' ')-1);
        if (sScriptCommand.find(':') != string::npos)
        {
            if (sScriptCommand.find("defines", sScriptCommand.find(':')+1) != string::npos)
            {
                nIncludeType = 1;
            }
            else if (sScriptCommand.find("globals", sScriptCommand.find(':')+1) != string::npos)
            {
                nIncludeType = 2;
            }
            else if (sScriptCommand.find("procedures", sScriptCommand.find(':')+1) != string::npos)
            {
                nIncludeType = 3;
            }
        }
        //cerr << sIncludeFileName << endl;
        if (sIncludeFileName.find(':') != string::npos)
        {
            for (int __i = sIncludeFileName.length()-1; __i >= 0; __i--)
            {
                if (sIncludeFileName[__i] == ':'
                    && (__i > 1
                        || (__i == 1 && sIncludeFileName.length() > (unsigned int)__i+1 && sIncludeFileName[__i+1] != '/')))
                {
                    sIncludeFileName.erase(sIncludeFileName.find(':'));
                    break;
                }
            }
        }
        if (sIncludeFileName.length())
            sIncludeFileName = FileSystem::ValidFileName(sIncludeFileName, ".nscr");
        else
            return "";
        //cerr << sIncludeFileName << endl;
        bReadFromInclude = true;
        fInclude.clear();
        fInclude.open(sIncludeFileName.c_str());
        if (fInclude.fail())
        {
            //sErrorToken = sIncludeFileName;
            bReadFromInclude = false;
            fInclude.close();
            throw SyntaxError(SyntaxError::SCRIPT_NOT_EXIST, sScriptCommand, SyntaxError::invalid_position, sIncludeFileName);
        }
        return "";
    }

    if (!bInstallProcedures
        && sScriptCommand.find("procedure") != string::npos
        && sScriptCommand.find('$', sScriptCommand.find("procedure")) != string::npos)
        throw SyntaxError(SyntaxError::PROCEDURE_WITHOUT_INSTALL_FOUND, sScriptCommand, SyntaxError::invalid_position);

    if (fLogFile.is_open() && bInstallProcedures)
    {
        if (sScriptCommand.find("procedure") != string::npos && sScriptCommand.find('$', sScriptCommand.find("procedure")) != string::npos)
        {
            bIsInstallingProcedure = true;
            fLogFile << ">> Installing: \"" << sScriptCommand.substr(sScriptCommand.find('$'), sScriptCommand.find('(', sScriptCommand.find('$'))-sScriptCommand.find('$')) << "\" ..." << endl;
            if (!bDISABLE_SCREEN_OUTPUT)
            {
                NumeReKernel::print(toSystemCodePage(_lang.get("SCRIPT_INSTALLING_PROC", sScriptCommand.substr(sScriptCommand.find('$'), sScriptCommand.find('(', sScriptCommand.find('$'))-sScriptCommand.find('$')))));
                //cerr << "|-> Installiere \"" << sScriptCommand.substr(sScriptCommand.find('$'), sScriptCommand.find('(', sScriptCommand.find('$'))-sScriptCommand.find('$')) << "\" ..." << endl;
            }
        }
        if (bENABLE_FULL_LOGGING)
        {
            if (bIsInstallingProcedure)
                fLogFile << ">> >> Copying: " << sScriptCommand << " ..." << endl;
            else
                fLogFile << ">> Evaluating: " << sScriptCommand << " ..." << endl;
        }
        if (sScriptCommand.find("endprocedure") != string::npos)
            bIsInstallingProcedure = false;
    }

    return sScriptCommand;
}

void Script::setScriptFileName(string& _sScriptFileName)
{
    if (_sScriptFileName.length())
        _sScriptFileName = FileSystem::ValidFileName(_sScriptFileName, ".nscr");
    sScriptFileName = _sScriptFileName;
    return;
}

// --> gibt den Wert von sScriptFileName (ggf. gekuerzt) zurueck <--
string Script::getScriptFileNameShort() const
{
    string sFileName = sScriptFileName;
    unsigned int nPos = -1;
    unsigned int nPos_2 = 0;
    while (sFileName.find('\\') != string::npos)
    {
        sFileName[sFileName.find('\\')] = '/';
    }
    while (sFileName.rfind('/', nPos) != string::npos)
    {
        nPos = sFileName.rfind('/', nPos);
        if (nPos != 0 && nPos-1 != ':')
        {
            nPos_2 = sFileName.rfind('/', nPos-1);
            if (nPos_2 != string::npos)
            {
                sFileName = sFileName.substr(0,nPos_2+1) + ".." + sFileName.substr(nPos);
                nPos = nPos_2;
            }
            else
                break;
        }
        else
            break;
    }
    return sFileName;
}

