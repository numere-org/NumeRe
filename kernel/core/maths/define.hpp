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
#include "../filesystem.hpp"
#include "../settings.hpp"
#include "../utils/tools.hpp"

using namespace std;

class Define : public FileSystem
{
    private:
        string sFunctions[100][13];         // Funct-Name, Expression, Definition, VarX, VarY, VarZ, VarT, ...
        unsigned int nDefinedFunctions;     // Anzahl der definierten Funktionen
        string sFileName;                   // Dateinamen fuer die Speichern-Funktion
        fstream Defines_def;                // Filestream, zum Speichern und Einlesen der definierten Funktionen

        string sBuilt_In;                   // String, der die Namen der Built-In-Funktionen speichert
        string sCommands;                   // String, der alle NumeRe-Kommandos speichert
        string sCaches;

    public:
        Define();                           // Standard-Konstruktor
        Define(Define& _defined);           // Kopierkonstruktor

        // --> TRUE, wenn es eine Funktion mit dem angegeben Funktionsnamen gibt <--
        bool isDefined(const string& sFunc);

        // --> Zentrale Methode: Definiert eine eigene Funktion <--
        bool defineFunc(const string& sExpr, bool bRedefine = false, bool bFallback = false);

        // --> Entfernt eine definierte Funktion aus dem Funktionenspeicher <--
        bool undefineFunc(const string& sFunc);

        // --> Ruft zuvor definierte Funktionen auf <--
        bool call(string& sExpr, int nRecursion = 0);

        // --> Gibt die Zahl der definierten Funktionen zurueck <--
        unsigned int getDefinedFunctions() const;

        // --> Gibt die zur Definition der _i-ten Funktion verwendete Definition zurueck <--
        string getDefine(unsigned int _i) const;
        string getFunction(unsigned int _i) const;
        string getImplemention(unsigned int _i) const;
        string getComment(unsigned int _i) const;

        bool reset();

        // --> Speichern der Funktionsdefinitionen <--
        bool save(const Settings& _option);

        // --> Laden gespeicherter Funktionsdefinitionen <--
        bool load(const Settings& _option, bool bAutoLoad = false);

        inline string getDefinesName() const
            {
                string sReturn = ";";
                for (unsigned int i = 0; i < nDefinedFunctions; i++)
                {
                    sReturn += sFunctions[i][0] + ";";
                }
                return sReturn;
            }
        inline void setCacheList(const string& sCacheList)
            {
                sCaches = sCacheList;
                return;
            }
        void setPredefinedFuncs(const string& sPredefined);
        inline string getPredefinedFuncs() const
            {
                return sBuilt_In;
            }
        inline int getFunctionIndex(const string& sFuncName)
            {
                if (!nDefinedFunctions)
                    return -1;
                for (unsigned int i = 0; i < nDefinedFunctions; i++)
                {
                    if (sFunctions[i][0] == sFuncName.substr(0, sFuncName.find('(')))
                        return i;
                }
                return -1;
            }
};

#endif
