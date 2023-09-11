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


// --> CLASS: DEFINE <--
#ifndef DEFINE_HPP
#define DEFINE_HPP

//#include <cstring>
#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>

#include "../ui/error.hpp"
#include "../io/filesystem.hpp"
#include "../settings.hpp"
#include "../utils/tools.hpp"


/////////////////////////////////////////////////
/// \brief This class implements a single
/// function definition. It is managed by the
/// Define class.
/////////////////////////////////////////////////
class FunctionDefinition
{
    public:
        std::string sName;
        std::string sSignature;
        std::string sDefinitionString;
        std::string sParsedDefinitionString;
        std::string sComment;
        std::vector<std::string> vArguments;

        FunctionDefinition(const std::string& _sDefinitionString = "");
        FunctionDefinition& operator=(const FunctionDefinition&);
        std::string parse(const std::string& _sArgList);
        std::string exportFunction() const;
        bool importFunction(const std::string& _sExportedString);
        std::string getDefinition() const;
        bool appendComment(const std::string& _sComment);

    private:
        bool decodeDefinition();
        bool splitAndValidateArguments();
        bool convertToValues();
        bool replaceArgumentOccurences();
};



/////////////////////////////////////////////////
/// \brief This class implements the function
/// definition managing instance.
/////////////////////////////////////////////////
class FunctionDefinitionManager : public FileSystem
{
    private:
        std::map<std::string, FunctionDefinition> mFunctionsMap;
        std::string sFileName;                   // Dateinamen fuer die Speichern-Funktion

        std::string sBuilt_In;                   // String, der die Namen der Built-In-Funktionen speichert
        std::string sCommands;                   // String, der alle NumeRe-Kommandos speichert
        std::string sTables;
        bool isLocal;

        std::string resolveRecursiveDefinitions(std::string sDefinition);
        std::map<std::string, FunctionDefinition>::const_iterator findItemById(size_t id) const;

    public:
        FunctionDefinitionManager(bool _isLocal);                           // Standard-Konstruktor
        FunctionDefinitionManager(const FunctionDefinitionManager& _defined);           // Kopierkonstruktor

        // --> TRUE, wenn es eine Funktion mit dem angegeben Funktionsnamen gibt <--
        bool isDefined(const std::string& sFunc);

        // --> Zentrale Methode: Definiert eine eigene Funktion <--
        bool defineFunc(const std::string& sExpr, bool bRedefine = false, bool bFallback = false);

        // --> Entfernt eine definierte Funktion aus dem Funktionenspeicher <--
        bool undefineFunc(const std::string& sFunc);

        // --> Ruft zuvor definierte Funktionen auf <--
        bool call(std::string& sExpr, int nRecursion = 0);

        // --> Gibt die Zahl der definierten Funktionen zurueck <--
        size_t getDefinedFunctions() const;

        // --> Gibt die zur Definition der _i-ten Funktion verwendete Definition zurueck <--
        std::string getDefinitionString(size_t _i) const;
        std::string getFunctionSignature(size_t _i) const;
        std::string getImplementation(size_t _i) const;
        std::string getComment(size_t _i) const;

        bool reset();

        // --> Speichern der Funktionsdefinitionen <--
        bool save(const Settings& _option);

        // --> Laden gespeicherter Funktionsdefinitionen <--
        bool load(const Settings& _option, bool bAutoLoad = false);

        /////////////////////////////////////////////////
        /// \brief Returns a list of the names of the
        /// custom defined functions.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string getNamesOfDefinedFunctions() const
        {
            std::string sReturn = ";";

            for (auto iter = mFunctionsMap.begin(); iter != mFunctionsMap.end(); ++iter)
            {
                sReturn += iter->first + ";";
            }

            return sReturn;
        }

        /////////////////////////////////////////////////
        /// \brief Sets the internal table list. This
        /// list is used to avoid redefinition of an
        /// already existing table as a function.
        ///
        /// \param sTableList const std::string&
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void setTableList(const std::string& sTableList)
        {
            sTables = sTableList;
        }

        void setPredefinedFuncs(const std::string& sPredefined);

        /////////////////////////////////////////////////
        /// \brief Return a list of the internal defined
        /// default functions.
        ///
        /// \return std::string
        ///
        /////////////////////////////////////////////////
        inline std::string getPredefinedFuncs() const
        {
            return sBuilt_In;
        }

        /////////////////////////////////////////////////
        /// \brief Returns the numerical index of the
        /// selected custom defined functions.
        ///
        /// \param sFuncName const std::string&
        /// \return size_t
        /// \remark This index is only valid as long as
        /// the FunctionDefinitionManager object instance
        /// is not modified (defining/undefining
        /// functions).
        ///
        /////////////////////////////////////////////////
        inline size_t getFunctionIndex(const std::string& sFuncName)
        {
            if (!mFunctionsMap.size())
                return (size_t)-1;

            size_t i = 0;

            for (auto iter = mFunctionsMap.begin(); iter != mFunctionsMap.end(); ++iter)
            {
                if (sFuncName.starts_with(iter->first+"("))
                    return i;

                i++;
            }

            return (size_t)-1;
        }
};

#endif
