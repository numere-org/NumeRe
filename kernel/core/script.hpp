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


// Header-Datei zur Script-Klasse

#ifndef SCRIPT_HPP
#define SCRIPT_HPP

#include <string>
#include <memory>

#include "io/filesystem.hpp"
#include "io/styledtextfile.hpp"
#include "io/logger.hpp"
#include "maths/define.hpp"
#include "symdef.hpp"
#include "procedure/includer.hpp"

class Script : public FileSystem
{
    private:
        std::unique_ptr<StyledTextFile> m_script;
        std::unique_ptr<Includer> m_include;

        enum
        {
            ENABLE_DEFAULTS = 0x0,
            ENABLE_FULL_LOGGING = 0x1,
            DISABLE_SCREEN_OUTPUT = 0x2
        };

        Logger m_logger;
        std::string sScriptFileName;

        bool bValidScript;
        bool bLastScriptCommand;
        int nLine;
        int nInstallModeFlags;
        bool isInstallMode;
        bool isInInstallSection;

        std::string sHelpID;
        std::string sInstallID;

        std::vector<std::string> vInstallPackages;
        size_t nCurrentPackage;

        FunctionDefinitionManager _localDef;
        SymDefManager _symdefs;

        bool startInstallation(std::string& sScriptCommand);
        bool handleInstallInformation(std::string& sScriptCommand);
        void writeDocumentationArticle(std::string& sScriptCommand);
        void writeLayout(std::string& sScriptCommand);
        void writeProcedure();
        bool writeWholeFile();
        void evaluateInstallInformation(std::string& sInstallInfoString);
        std::string getNextScriptCommandFromScript();
        std::string getNextScriptCommandFromInclude();
        std::string handleIncludeSyntax(std::string& sScriptCommand);
        bool handleLocalDefinitions(std::string& sScriptCommand);

    public:
        Script();
        ~Script();

        std::string getNextScriptCommand();
        inline std::string getScriptFileName() const
            {return sScriptFileName;}
        inline void setPredefinedFuncs(const std::string& sPredefined)
        {
            _localDef.setPredefinedFuncs(sPredefined);
        }
        inline size_t getCurrentLine() const
            {return m_include && m_include->is_open() ? m_include->getCurrentLine() : nLine;}
        void openScript(std::string& _sScriptFileName, int nFromLine);
        void close();
        void returnCommand();
        inline void setInstallProcedures(bool _bInstall = true)
            {
                isInstallMode = _bInstall;
                return;
            }
        inline bool isOpen() const
            {return m_script.get() != nullptr;};
        inline bool isValid() const
            {return bValidScript;};
        inline bool wasLastCommand()
            {
                if (bLastScriptCommand)
                {
                    bLastScriptCommand = false;
                    return true;
                }
                return false;
            }
        inline bool installProcedures() const
            {return isInstallMode;}
};

#endif

