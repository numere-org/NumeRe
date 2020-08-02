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

using namespace std;

/////////////////////////////////////////////////
/// \brief This class implements a single
/// function definition. It is managed by the
/// Define class.
/////////////////////////////////////////////////
class FunctionDefinition
{
    public:
        string sName;
        string sSignature;
        string sDefinitionString;
        string sParsedDefinitionString;
        string sComment;
        vector<string> vArguments;

        FunctionDefinition(const string& _sDefinitionString = "");
        FunctionDefinition& operator=(const FunctionDefinition&);
        string parse(const string& _sArgList);
        string exportFunction();
        bool importFunction(const string& _sExportedString);
        string getDefinition() const;
        bool appendComment(const string& _sComment);

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
        map<string, FunctionDefinition> mFunctionsMap;
        string sFileName;                   // Dateinamen fuer die Speichern-Funktion
        fstream Defines_def;                // Filestream, zum Speichern und Einlesen der definierten Funktionen

        string sBuilt_In;                   // String, der die Namen der Built-In-Funktionen speichert
        string sCommands;                   // String, der alle NumeRe-Kommandos speichert
        string sTables;
        bool isLocal;

        string resolveRecursiveDefinitions(string sDefinition);
        map<string, FunctionDefinition>::const_iterator findItemById(size_t id) const;

    public:
        FunctionDefinitionManager(bool _isLocal);                           // Standard-Konstruktor
        FunctionDefinitionManager(FunctionDefinitionManager& _defined);           // Kopierkonstruktor

        // --> TRUE, wenn es eine Funktion mit dem angegeben Funktionsnamen gibt <--
        bool isDefined(const string& sFunc);

        // --> Zentrale Methode: Definiert eine eigene Funktion <--
        bool defineFunc(const string& sExpr, bool bRedefine = false, bool bFallback = false);

        // --> Entfernt eine definierte Funktion aus dem Funktionenspeicher <--
        bool undefineFunc(const string& sFunc);

        // --> Ruft zuvor definierte Funktionen auf <--
        bool call(string& sExpr, int nRecursion = 0);

        // --> Gibt die Zahl der definierten Funktionen zurueck <--
        size_t getDefinedFunctions() const;

        // --> Gibt die zur Definition der _i-ten Funktion verwendete Definition zurueck <--
        string getDefinitionString(size_t _i) const;
        string getFunctionSignature(size_t _i) const;
        string getImplementation(size_t _i) const;
        string getComment(size_t _i) const;

        bool reset();

        // --> Speichern der Funktionsdefinitionen <--
        bool save(const Settings& _option);

        // --> Laden gespeicherter Funktionsdefinitionen <--
        bool load(const Settings& _option, bool bAutoLoad = false);

        /////////////////////////////////////////////////
        /// \brief Returns a list of the names of the
        /// custom defined functions.
        ///
        /// \return string
        ///
        /////////////////////////////////////////////////
        inline string getNamesOfDefinedFunctions() const
        {
            string sReturn = ";";

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
        /// \param sTableList const string&
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void setTableList(const string& sTableList)
        {
            sTables = sTableList;
        }

        void setPredefinedFuncs(const string& sPredefined);

        /////////////////////////////////////////////////
        /// \brief Return a list of the internal defined
        /// default functions.
        ///
        /// \return string
        ///
        /////////////////////////////////////////////////
        inline string getPredefinedFuncs() const
        {
            return sBuilt_In;
        }

        /////////////////////////////////////////////////
        /// \brief Returns the numerical index of the
        /// selected custom defined functions.
        ///
        /// \param sFuncName const string&
        /// \return size_t
        /// \remark This index is only valid as long as
        /// the FunctionDefinitionManager object instance
        /// is not modified (defining/undefining
        /// functions).
        ///
        /////////////////////////////////////////////////
        inline size_t getFunctionIndex(const string& sFuncName)
        {
            if (!mFunctionsMap.size())
                return (size_t)-1;

            size_t i = 0;

            for (auto iter = mFunctionsMap.begin(); iter != mFunctionsMap.end(); ++iter)
            {
                if (iter->first == sFuncName.substr(0, sFuncName.find('(')))
                    return i;

                i++;
            }

            return (size_t)-1;
        }
};

#endif
