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

#include "define.hpp"
#include "../kernel.hpp"

// --> Standard-Konstruktor: Deklariert auch die Inhalte der Built-In-Funktionen- und der Kommando-Strings <--
Define::Define() : FileSystem()
{
    nDefinedFunctions = 0;
    sBuilt_In = ",abs(),acos(),acosh(),Ai(),asin(),asinh(),ascii(),atan(),atanh(),avg(),bessel(),betheweizsaecker(),Bi(),binom(),cache(),char(),cmp(),cnt(),cos(),cosh(),cot(),cross(),data(),date(),dblfacul(),degree(),det(),diag(),diagonalize(),ellipticD(),ellipticE(),ellipticF(),ellipticPi(),eigenvals(),eigenvect(),erf(),erfc(),exp(),faculty(),findfile(),findparam(),floor(),gamma(),gauss(),gcd(),getfilelist(),getfolderlist(),getindices(),getmatchingparens(),getopt(),heaviside(),hermite(),identity(),imY(),invert(),is_data(),is_nan(),is_string(),laguerre(),laguerre_a(),lcm(),legendre(),legendre_a(),ln(),log(),log10(),log2(),matfc(),matfcf(),matfl(),matflf(),max(),med(),min(),neumann(),norm(),num(),one(),pct(),phi(),prd(),radian(),rand(),range(),rect(),repeat(),replace(),replaceall(),rint(),roof(),round(),sbessel(),sign(),sin(),sinc(),sinh(),sneumann(),solve(),split(),sqrt(),std(),strfnd(),strrfnd(),strmatch(),strrmatch(),str_not_match(),str_not_rmatch(),string_cast(),strlen(),student_t(),substr(),sum(),tan(),tanh(),theta(),time(),to_char(),to_cmd(),to_lowercase(),to_string(),to_uppercase(),to_value(),transpose(),valtostr(),version(),Y(),Z(),zero()";
    sCommands = ",for,if,while,endfor,endwhile,endif,else,elseif,continue,break,explicit,procedure,endprocedure,throw,return,"; //",stats,hist,random,test,credits,copy,about,info,help,man,set,get,list,if,else,endif,break,continue,ifndef,undef,integrate,integrate2,for,endfor,define,redefine,resample,ifndef,search,smooth,find,plot,plot3d,mesh,mesh3d,surf,surf3d,cont,cont3d,grad,save,load,show,clear,headedit,script,start,del,quit,diff,extrema,zeroes,repl,dens,dens3d,vect,vect3d,while,endwhile,sort,taylor,global,var,procedure,endprocedure,throw,return,fit,fitw,readline,read,write,explicit,";
    sFileName = "<>/functions.def";
    sCaches = "";

    // --> Speicher-Array der definierten Funktionen korrekt initialisieren <--
    for (int i = 0; i < 100; i++)
    {
        for (int j = 0; j < 13; j++)
        {
            sFunctions[i][j] = "";
        }
    }
}

/* --> Kopierkonstruktor: Ruft zunaechst den Standard-Konstruktor auf, ehe die Werte und
 *     Definitionen des anderen Objekts kopiert werden <--
 */
Define::Define(Define& _defined) : FileSystem()
{
    Define();
    nDefinedFunctions = _defined.nDefinedFunctions;
    sFileName = _defined.sFileName;
    sCaches = _defined.sCaches;

    for (unsigned int i = 0; i < nDefinedFunctions; i++)
    {
        for (int j = 0; j < 13; j++)
        {
            sFunctions[i][j] = _defined.sFunctions[i][j];
        }
    }
}

// --> Diese Methode gibt TRUE zurueck, wenn eine Funktion mit dem angegeben Funktionsnamen bereits definiert wurde <--
bool Define::isDefined(const string& sFunc)
{
    // --> Verwenden wir nur den Teil vor der ersten Klammer, da dieser den Namen der Funktion beinhaelt <--
    string sToLocate = sFunc.substr(0,sFunc.find('('));

    // --> Cyclen wir durch das Array und pruefen wir, ob es bereits eine Funktion mit dem Namen gibt <--
    for (unsigned int i = 0; i < nDefinedFunctions; i++)
    {
        if (sFunctions[i][0] == sToLocate)
            return true;
    }

    /* --> Wenn das nicht der Fall ist: pruefen wir gleich noch, ob die zu suchende Funktion nicht zufaellig
     *     einer Built-In-Funktion oder einem Kommando entspricht <--
     */
    if (sBuilt_In.find("," + sToLocate + "(") != string::npos || sCommands.find("," + sToLocate + ",") != string::npos)
        return true;
    else
        return false;
}

// --> Zentrale Methode: Definieren von eigenen Funktionen <--
bool Define::defineFunc(const string& sExpr, Parser& _parser, const Settings& _option, bool bRedefine, bool bFallback)
{
    int nPos = 0;               // Index-Variable fuer Char-Positionen in strings
    unsigned int nDefine = -1;  // Index-Variable fuer die zu schreibende Position im Array
    string sFallback = "";      // Im Falle einer Umdefinition speichern wir den alten Eintrag in diesen
                                // String, um selbigen im Fehlerfall wiederherstellen zu koennen
    string sFunction = "";      // String, der die eigentliche Definition enthalten wird
    string sFunctionName = "";  // String, der den Funktionsnamen beinhalten wird
    mu::varmap_type mAsVal;

    if (bRedefine && sExpr.find("()") != string::npos && matchParams(sExpr, "comment", '=') && nDefinedFunctions)
    {
        sFunction = sExpr.substr(0,sExpr.find("()"));
        string sComment = sExpr.substr(matchParams(sExpr, "comment", '=')+7);
        StripSpaces(sComment);
        sComment = sComment.substr(sComment.find('"'), sComment.find('"', sComment.find('"')+1)+1-sComment.find('"'));
        for (unsigned int i = 0; i < nDefinedFunctions; i++)
        {
            if (sFunctions[i][0] == sFunction)
            {
                if (sFunctions[i][2].find("-set") != string::npos || sFunctions[i][2].find(" --") != string::npos)
                {
                    if (matchParams(sFunctions[i][2], "comment", '='))
                    {
                        sFunctions[i][2] = sFunctions[i][2].substr(0,matchParams(sFunctions[i][2], "comment", '=')+7) + fromSystemCodePage(sComment)
                            + sFunctions[i][2].substr(sFunctions[i][2].find('"', sFunctions[i][2].find('"', matchParams(sFunctions[i][2], "comment", '='))+1)+1);
                    }
                    else
                        sFunctions[i][2] = sFunctions[i][2] + "comment=" + fromSystemCodePage(sComment);
                }
                else
                {
                    sFunctions[i][2] = sFunctions[i][2] + "-set comment=" + fromSystemCodePage(sComment);
                }
                if (_option.getSystemPrintStatus())
                    NumeReKernel::print(LineBreak(_lang.get("DEFINE_FUNCTION_COMMENTED", sFunction), _option));
                return true;
            }
        }
        NumeReKernel::print(LineBreak(_lang.get("DEFINE_FUNCTION_NOT_EXISTING", sFunction), _option));
        return false;
    }

    /* --> Fehler abfangen: <--
     *     - Keine oeffnende Klammer oder nur oeffnende Klammern nach ':='
     *     - Ausdruck ohne ':='
     *     - Name stimmt mit einer Built-In-Funktion ueberein
     *     - Name stimmt mit einem Kommando ueberein
     */
    if (sExpr.find('(') == string::npos
        || (sExpr.find('(') != string::npos && sExpr.find('(') > sExpr.find(":="))
        || sBuilt_In.find(","+sExpr.substr(0,sExpr.find('(')+1)) != string::npos
        || (sCaches.length() && sCaches.find(";"+sExpr.substr(0,sExpr.find('('))+";") != string::npos)
        || sCommands.find(","+sExpr.substr(0,sExpr.find('('))+",") != string::npos
        || sExpr.find(":=") == string::npos)
    {
        // --> Passende Fehlermeldungen ausgeben <--
        if (sExpr.find(":=") == string::npos)
        {
            throw CANNOT_FIND_DEFINE_OPRT;
        }
        else if (sExpr.find('(') == string::npos || (sExpr.find('(') != string::npos && sExpr.find('(') > sExpr.find(":=")))
        {
            throw CANNOT_FIND_FUNCTION_ARGS;
        }
        else if (sBuilt_In.find(","+sExpr.substr(0,sExpr.find('(')+1)) != string::npos)
        {
            sErrorToken = sExpr.substr(0,sExpr.find('('));
            throw FUNCTION_IS_PREDEFINED;
        }
        else if (sCaches.length() && sCaches.find(";"+sExpr.substr(0,sExpr.find('('))+";") != string::npos)
        {
            sErrorToken = sExpr.substr(0,sExpr.find('('));
            throw CACHE_ALREADY_EXISTS;
        }
        else if (sCommands.find(","+sExpr.substr(0,sExpr.find('('))+",") != string::npos)
        {
            sErrorToken = sExpr.substr(0,sExpr.find('('));
            throw FUNCTION_STRING_IS_COMMAND;
        }
        else
        {
            throw CANNOT_FIND_FUNCTION_ARGS;
        }

        // --> FALSE zurueckgeben <--
        return false;
    }

    // --> Definition in sFunction kopieren und fuehrende/schliessende Leerzeichen entfernen <--
    sFunction = sExpr.substr(sExpr.find(":=")+2);
    if (sFunction.rfind("-set") != string::npos)
    {
        sFunction = sFunction.substr(0, sFunction.rfind("-set"));
    }
    if (sFunction.rfind(" --") != string::npos)
    {
        sFunction = sFunction.substr(0, sFunction.rfind(" --"));
    }
    sFunctionName = sExpr.substr(0, sExpr.find(":="));

    StripSpaces(sFunction);
    StripSpaces(sFunctionName);
    // --> Fehler abfangen: Ziffern am Anfang eines Funktionsnamens und Operatoren in einem Funktionsnamen <--
    if (sFunctionName[0] >= '0' && sFunctionName[0] <= '9')
    {
        throw NO_NUMBER_AT_POS_1;
    }

    string sDelim = "+-*/^!=&| ><()?[]{}$%§~#:.,;";
    for (unsigned int i = 0; i < sFunctionName.find('('); i++)
    {
        if (sDelim.find(sFunctionName[i]) != string::npos)
        {
            sErrorToken = sFunctionName.substr(i,1);
            throw FUNCTION_NAMES_MUST_NOT_CONTAIN_SIGN;
            //cerr << LineBreak("|-> FEHLER: Funktionsnamen dürfen nicht \"" + sFunctionName.substr(i,1) + "\" enthalten!", _option) << endl;
            return false;
        }
    }

    sFunction = " " + sFunction + " ";
    if (matchParams(sExpr, "recursive"))
    {
        string sFuncOccurence = "";
        for (unsigned int i = 0; i < sFunction.length(); i++)
        {
            if (sFunction.substr(i,sFunctionName.find('(')+1) == sFunctionName.substr(0, sFunctionName.find('(')+1)
                && (!i || !isalnum(sFunction[i-1])))
            {
                sFuncOccurence = sFunction.substr(i, sFunction.find(')', i)+1-i);
                if (!call(sFuncOccurence, _option))
                {
                    return false;
                }
                sFunction.replace(i,sFunction.find(')', i)+1-i, sFuncOccurence);
            }
        }
    }
    if (matchParams(sExpr, "asval", '='))
    {
        string sAsVal = getArgAtPos(sExpr, matchParams(sExpr, "asval", '=')+5);
        if (sAsVal.front() == '{')
            sAsVal.erase(0,1);
        if (sAsVal.back() == '}')
            sAsVal.pop_back();
        _parser.SetExpr(sAsVal);
        _parser.Eval();
        mAsVal = _parser.GetUsedVar();
    }

    //cerr << sFunction << endl;

    StripSpaces(sFunction);
    /* --> Dies ist ein Endlosschleifentest: Define::call() gibt FALSE zurueck, wenn die Rekursion nicht nach
     *     einer definierten Anzahl an Schritten abbricht. (Die Anzahl ist dabei von der Anzahl an definierten
     *     Funktionen abhaengig) <--
     */
    if (!call(sFunction, _option))
    {
        return false;
    }


    sFunction = " " + sExpr.substr(sExpr.find(":=")+2) + " ";
    if (sFunction.rfind("-set") != string::npos)
    {
        sFunction = sFunction.substr(0, sFunction.rfind("-set")) + " ";
    }
    if (sFunction.rfind(" --") != string::npos)
    {
        sFunction = sFunction.substr(0, sFunction.rfind(" --")) + " ";
    }

    if (matchParams(sExpr, "recursive"))
    {
        string sFuncOccurence = "";
        for (unsigned int i = 0; i < sFunction.length(); i++)
        {
            if (sFunction.substr(i,sFunctionName.find('(')+1) == sFunctionName.substr(0, sFunctionName.find('(')+1)
                && (!i || !isalnum(sFunction[i-1])))
            {
                sFuncOccurence = sFunction.substr(i, sFunction.find(')', i)+1-i);
                if (!call(sFuncOccurence, _option))
                {
                    return false;
                }
                StripSpaces(sFuncOccurence);
                sFunction.replace(i,sFunction.find(')', i)+1-i, sFuncOccurence);
            }
        }
    }

    // --> Handelt es sich um eine Umdefinition? <--
    if (bRedefine)
    {
        // --> Suche nach der umzudefinierenden Funktion in der Datenbank <--
        for (unsigned int i = 0; i < nDefinedFunctions; i++)
        {
            if (sFunctions[i][0] == sExpr.substr(0,sExpr.find('(')))
            {
                // --> Sobald die Funktion gefunden wurde, koennen wir die Schleife abbrechen <--
                nDefine = i;
                break;
            }
        }
        if (nDefine == string::npos)
        {
            /* --> nDefine hat den Defaultwert, wenn keine bereits definierte Funktion mit der
             *     umzudefinierenden Funktion uebereinstimmt. Definieren wir einfach eine neue. <--
             */
            if (_option.getSystemPrintStatus())
                NumeReKernel::print(LineBreak(_lang.get("DEFINE_NEW_FUNCTION")+" ...", _option));
            nDefine = nDefinedFunctions;
            bRedefine = false;
        }
        else
        {
            // --> Kopieren wir den urspruenglichen Ausdruck in sFallback und geben wir die komplette Zeile frei <--
            sFallback = sFunctions[nDefine][2];
            for (int i = 0; i < 6; i++)
            {
                sFunctions[nDefine][i] = "";
            }
        }
    }
    else
    {
        // --> Gehen wir sicher, dass nicht mehr als 100 Funktionen definiert werden koennen <--
        if (nDefinedFunctions < 100)
            nDefine = nDefinedFunctions; // nDefinedFunctions ist immer der Absolutwert an definierten Funktionen (1,2,3,...)
        else
        {
            NumeReKernel::print(LineBreak(_lang.get("DEFINE_NO_SPACE"), _option));
            return false;
        }
    }


    // --> Gibt offenbar Platz: Kopieren wir schon mal die Eindeutigen Einstaege in die Datenbank <--
    sFunctions[nDefine][0] = sExpr.substr(0,sExpr.find('('));                   // Funktionsname
    sFunctions[nDefine][1] = sFunction;
    /*sFunctions[nDefine][1] = " " + sExpr.substr(sExpr.find(":=")+2) + " ";      // Funktionsausdruck; die Leerzeichen sind dazu da,
                                                                                // sicher zu gehen, dass wenn man das Zeichen VOR
                                                                                // dem Treffer untersuchen moechte, kein SEG-FAULT
                                                                                // auftritt
    if (sFunctions[nDefine][1].rfind("-set") != string::npos)
    {
        sFunctions[nDefine][1] = sFunctions[nDefine][1].substr(0, sFunctions[nDefine][1].rfind("-set")) + " ";
    }
    if (sFunctions[nDefine][1].rfind(" --") != string::npos)
    {
        sFunctions[nDefine][1] = sFunctions[nDefine][1].substr(0, sFunctions[nDefine][1].rfind(" --")) + " ";
    }*/

    /*if (matchParams(sExpr, "recursive"))
    {
        string sFuncOccurence = "";
        for (unsigned int i = 0; i < sFunctions[nDefine][1].length(); i++)
        {
            if (sFunctions[nDefine][1].substr(i,sFunctions[nDefine][0].length()+1) == sFunctions[nDefine][0]+"("
                && (!i || !isalnum(sFunctions[nDefine][1][i-1])))
            {
                sFuncOccurence = sFunctions[nDefine][1].substr(i, sFunctions[nDefine][1].find(')', i)+1-i);
                if (!call(sFuncOccurence, _option))
                {
                    return false;
                }
                sFunctions[nDefine][1].replace(i,sFunctions[nDefine][1].find(')', i)+1-i, sFuncOccurence);
            }
        }
    }*/

    if (matchParams(sExpr, "asval", '='))
    {
        for (auto iter = mAsVal.begin(); iter != mAsVal.end(); ++iter)
        {
            for (unsigned int i = 0; i < sFunctions[nDefine][1].length(); i++)
            {
                if (sFunctions[nDefine][1].substr(i, (iter->first).length()) == iter->first && checkDelimiter(sFunctions[nDefine][1].substr(i-1, (iter->first).length()+2)))
                {
                    sFunctions[nDefine][1].replace(i,(iter->first).length(), toString(*iter->second, _option));
                }
            }
        }
    }

    //cerr << sFunctions[nDefine][1] << endl;

    // Warum mit Parameter?
    sFunctions[nDefine][2] = fromSystemCodePage(sExpr);                                             // Urspruengliche Definition

    // --> Hierein kommt die Argumentliste aus der Definition. Wir werden sie weiter unten weiterverarbeiten <--
    //sFunctions[nDefine][3] = sExpr.substr(sExpr.find('('),sExpr.find(')')-sExpr.find('(')+1);
    sFunctions[nDefine][3] = getArgAtPos(sExpr, sExpr.find('('));
    if (sFunctions[nDefine][3].front() == '('
        && getMatchingParenthesis(sFunctions[nDefine][3]) != string::npos
        && getMatchingParenthesis(sFunctions[nDefine][3]) != sFunctions[nDefine][3].length()-1)
        sFunctions[nDefine][3].erase(getMatchingParenthesis(sFunctions[nDefine][3])+1);
    if (getMatchingParenthesis(sFunctions[nDefine][3]) != string::npos && sFunctions[nDefine][3].front() == '(' && sFunctions[nDefine][3].back() == ')')
    {
        sFunctions[nDefine][3].erase(0,1);
        sFunctions[nDefine][3].pop_back();
    }

    // --> Entfernen wir Leerzeichen um den Funktionsnamen <--
    StripSpaces(sFunctions[nDefine][0]);

    // --> Gehen wir sicher, dass es nicht schon eine Funktion gleichen Namens gibt <--
    if (!bRedefine)
    {
        for (unsigned int i = 0; i < nDefinedFunctions; i++)
        {
            if (sFunctions[nDefine][0] == sFunctions[i][0])
            {
                for (int j = 0; j < 13; j++)
                {
                    sFunctions[nDefine][j] = "";
                }
                throw FUNCTION_ALREADY_EXISTS;
                //cerr << LineBreak("|-> FEHLER: Dieser Funktionsname ist bereits belegt!", _option) << endl;
                return false;
            }
        }
    }

    if (_option.getbDebug())
    {
        cerr << "|-> DEBUG: sFunctions[][0] = " << sFunctions[nDefine][0] << endl;
        cerr << "|-> DEBUG: sFunctions[][1] = " << sFunctions[nDefine][1] << endl;
        cerr << "|-> DEBUG: sFunctions[][2] = " << sFunctions[nDefine][2] << endl;
        cerr << "|-> DEBUG: sFunctions[][3] = " << sFunctions[nDefine][3] << endl;
    }
    if (sFunctions[nDefine][3].find('(') != string::npos || sFunctions[nDefine][3].find(')') != string::npos)
    {
        sErrorToken = "()";
        throw FUNCTION_ARGS_MUST_NOT_CONTAIN_SIGN;
    }

    /* --> Pruefen wir, ob in der Argumentliste nicht das ein oder andere Komma enthalten ist
     *     und teilen den String an dieser Stelle <--
     * --> Wir machen das 9 Mal, da es bei maximal 10 Argumenten auch nur maximal 9
     *     Kommata geben kann <--
     */
    if (sFunctions[nDefine][3].find(',') != string::npos)
    {
        //sFunctions[nDefine][3] = "(" + sFunctions[nDefine][3] + ")";
        for (int i = 3; i < 12; i++)
        {
            try
            {
                parser_SplitArgs(sFunctions[nDefine][i], sFunctions[nDefine][i+1], ',', _option, true);
            }
            catch (errorcode &e)
            {
                if (e == SEPARATOR_NOT_FOUND)
                {
                    sErrorToken = ",";
                    throw FUNCTION_ARGS_MUST_NOT_CONTAIN_SIGN;
                }
                else
                    throw;
            }
            StripSpaces(sFunctions[nDefine][i]);
            StripSpaces(sFunctions[nDefine][i+1]);
            if (sFunctions[nDefine][i] == "...")
            {
                if (bRedefine)
                    defineFunc(sFallback, _parser, _option, true, true);
                else
                {
                    for (int u = 0; u < 6; u++)
                        sFunctions[nDefine][u] = "";
                }
                throw ELLIPSIS_MUST_BE_LAST_ARG;
            }
            if (sFunctions[nDefine][i] != "...")
            {
                for (unsigned int n = 0; n < sDelim.length(); n++)
                {
                    if (sFunctions[nDefine][i].find(sDelim[n]) != string::npos)
                    {
                        sErrorToken = sDelim[n];
                        throw FUNCTION_ARGS_MUST_NOT_CONTAIN_SIGN;
                    }
                }
            }
            if (sFunctions[nDefine][i+1].find(',') != string::npos && i < 11)
            {
                if (sFunctions[nDefine][i] == "...")
                {
                    if (bRedefine)
                        defineFunc(sFallback, _parser, _option, true, true);
                    else
                    {
                        for (int u = 0; u < 6; u++)
                            sFunctions[nDefine][u] = "";
                    }
                    throw ELLIPSIS_MUST_BE_LAST_ARG;
                }
                //sFunctions[nDefine][i+1] = "(" + sFunctions[nDefine][i+1] + ")";
            }
            else if (sFunctions[nDefine][i+1].find(',') != string::npos)
            {
                if (bRedefine)
                {
                    //cerr << LineBreak("|-> FEHLER: Funktionen können nicht mehr als zehn Argumente besitzen! Die Neudefinition wird rückgängig gemacht ...", _option) << endl;
                    defineFunc(sFallback, _parser, _option, true, true);
                }
                else
                {
                    //cerr << LineBreak("|-> FEHLER: Funktionen können nicht mehr als zehn Argumente besitzen!", _option) << endl;
                    for (int u = 0; u < 6; u++)
                        sFunctions[nDefine][u] = "";
                }
                throw TOO_MANY_ARGS_FOR_DEFINE;
            }
            else
                break;
                /*parser_SplitArgs(sFunctions[nDefine][4], sFunctions[nDefine][5], ',', _option, false);
                StripSpaces(sFunctions[nDefine][4]);
                StripSpaces(sFunctions[nDefine][5]);
                if (sFunctions[nDefine][5].find(',') != -1)
                {
                    if (bRedefine)
                    {
                        cerr << LineBreak("|-> FEHLER: Funktionen koennen nicht mehr als zehn Argumente besitzen! Die Neudefinition wird rueckgaengig gemacht ...", _option) << endl;
                        defineFunc(sFallback, _parser, _option, true);
                    }
                    else
                    {
                        cerr << LineBreak("|-> FEHLER: Funktionen koennen nicht mehr als zehn Argumente besitzen!", _option) << endl;
                        for (int u = 0; u < 6; u++)
                            sFunctions[nDefine][u] = "";
                    }
                    return false;
                }
            }*/
        }
    }
    else
    {
        // --> Kein Komma? Dann nur ein Argument. Entfernen wir rasch die Klammern und entfernen ueberzaehlige Leerzeichen <--
        //sFunctions[nDefine][3] = sFunctions[nDefine][3].substr(1,sFunctions[nDefine][3].length()-2);
        StripSpaces(sFunctions[nDefine][3]);
    }

    if (_option.getbDebug())
    {
        for (int i = 3; i < 13; i++)
        {
            cerr << "|-> DEBUG: sFunctions[][" << i << "] = " << sFunctions[nDefine][i] << "." << endl;
        }
    }

    /* --> Einer der zentralen Abschnitte: wir ersetzen jedes Auftreten der Variablen durch ein
     *     sogenanntes Token ">>VAR<<". Dieses ist stets eindeutig und kann rascher ersetzt werden <--
     * --> Wuerden wir das hier nicht machen, muessten wir bei jedem Funktionsaufruf die Variablen eindeutig
     *     identifizieren, was nur unnoetig Rechenzeit kosten wuerde <--
     */
    for (int n = 3; n < 13; n++)
    {
        // --> Gibt's das n-te Element nicht mehr? Schleife abbrechen! <--
        if (!sFunctions[nDefine][n].length())
            break;

        // --> Positions-Index zuruecksetzen <--
        nPos = 0;

        // --> Suche immer die naechste Position, die so "aussieht" wie die aktuelle Variable <--
        while (sFunctions[nDefine][1].find(sFunctions[nDefine][n], nPos) != string::npos)
        {
            // --> Speichere die Position <--
            nPos = sFunctions[nDefine][1].find(sFunctions[nDefine][n], nPos);
            if (_option.getbDebug())
                cerr << "|-> DEBUG: nPos = " << nPos << endl;

            // --> Pruefe, ob die "scheinbare" Variable von Delimitern umgeben ist und damit eine "echte" Variable ist <--
            if (checkDelimiter(sFunctions[nDefine][1].substr(nPos-1, sFunctions[nDefine][n].length() + 2)))
            {
                // --> Ersetze die aktuelle Position, an der VAR aufgetreten ist, durch ">>VAR<<" <--
                sFunctions[nDefine][1].replace(nPos, sFunctions[nDefine][n].length(), ">>" + sFunctions[nDefine][n] + "<<");

                // --> Setze den Positionsindex um die Laenge der VAR + 4 weiter <--
                nPos += sFunctions[nDefine][n].length() + 4;
                if (_option.getbDebug())
                    cerr << "|-> DEBUG: Var replaced!" << endl;
            }
            else        // Wenn sie das nicht ist, dann setze den Positionsindex um die Laenge der aktuellen Variable weiter
                nPos += sFunctions[nDefine][n].length();
        }

        // --> Nachdem alle Variablen ersetzt wurden, ergaenze die Variable selbst zu ">>VAR<<" <--
        sFunctions[nDefine][n] = ">>" + sFunctions[nDefine][n] + "<<";

        if (_option.getbDebug())
        {
            cerr << "|-> DEBUG: sFunctions[][1] = " << sFunctions[nDefine][1] << endl;
        }
    }

    // --> Entferne die umschliessenden Leerzeichen <--
    StripSpaces(sFunctions[nDefine][1]);

    /* --> Hier muessen wir noch abfangen, ob es sich um einen Mehrfachausdruck (ein Ausdruck, der mehrere
     *     Rueckgabewerte besitzt) handelt <--
     */
    if (isMultiValue(sFunctions[nDefine][1]) && sFunctions[nDefine][1].find("{") == string::npos)
    {
        // --> Wenn die Funktion isMultiValue() TRUE liefert und keine "{{" gefunden wurden, ergaenze sie <--
        sFunctions[nDefine][1] = "{" + sFunctions[nDefine][1] + "}";
    }
    else if ((sFunctions[nDefine][1].find("{") != string::npos && sFunctions[nDefine][1].find("}") == string::npos)
        || (sFunctions[nDefine][1].find("{") == string::npos && sFunctions[nDefine][1].find("}") != string::npos))
    {
        // --> Fehlt "{{" oder "}}" auf einer Seite? Fehlermeldung und alles rueckgaengig machen! <--
        //cerr << LineBreak("|-> FEHLER: Unvollständige Vektor-Zuweisung!", _option) << endl;

        if (nDefine == nDefinedFunctions)
        {
            for (int i = 0; i < 13; i++)
            {
                sFunctions[nDefine][i] = "";
            }
        }
        else
        {
            //cerr << LineBreak("|   Die Neudefinition wird rückgängig gemacht ...", _option) << endl;
            defineFunc(sFallback, _parser, _option, true, true);
        }

        throw INCOMPLETE_VECTOR_SYNTAX;
    }

    if (_option.getbDebug())
    {
        for (int i = 0; i < 13; i++)
        {
            cerr << "|-> DEBUG: sFunctions[][" << i << "] = " << sFunctions[nDefine][i] << endl;
        }
    }

    // --> Falls es keine Umdefinition war, erhoehen wir den Funktionenszaehler um 1 <--
    if (!bRedefine)
        nDefinedFunctions++;

    /* --> Jetzt haben wir eine weitere Funktion definiert, pruefen wir nochmal, ob die
     *     Rekursion abbricht (um so was wie "f(x) = c*f(x)+b" abzufangen) <--
     */
    sFunction = sExpr.substr(sExpr.find(":=")+2);
    StripSpaces(sFunction);
    try
    {
        if (!call(sFunction, _option))
        {
            if (!bRedefine)
            {
                for (int i = 0; i < 13; i++)
                {
                    sFunctions[nDefine][i] = "";
                }
                nDefinedFunctions--;
            }
            else
            {
                for (int i = 1; i < 13; i++)
                {
                    sFunctions[nDefine][i] = "";
                }
                NumeReKernel::printPreFmt(LineBreak("|   "+_lang.get("DEFINE_UNDOING_REDEFINE")+" ...", _option,0)+"\n");
                defineFunc(sFallback, _parser, _option, true);
            }
            return false;
        }
    }
    catch (...)
    {
        if (!bRedefine)
        {
            for (int i = 0; i < 13; i++)
            {
                sFunctions[nDefine][i] = "";
            }
            nDefinedFunctions--;
        }
        else
        {
            for (int i = 1; i < 13; i++)
            {
                sFunctions[nDefine][i] = "";
            }
            //cerr << LineBreak("|   Die Neudefinition wird rückgängig gemacht ...", _option) << endl;
            defineFunc(sFallback, _parser, _option, true, true);
        }
        throw;
    }
    // --> Alles hat geklappt! Geben wir eine entsprechende Erfolgsmeldung zurueck! <--
    if (_option.getSystemPrintStatus() && !bFallback)
    {
        if (bRedefine)
        {
            NumeReKernel::print(LineBreak(_lang.get("DEFINE_REDEFINE_SUCCESS", sFunctions[nDefine][0]), _option));
        }
        else
        {
            NumeReKernel::print(LineBreak(_lang.get("DEFINE_NEW_FUNCTION_SUCCESS", sFunctions[nDefine][0]), _option));
            if (nDefinedFunctions > 90)
                NumeReKernel::printPreFmt(toSystemCodePage("|   ("+_lang.get("DEFINE_FREE_SPACE", toString(100-nDefinedFunctions))+")")+"\n");
        }
    }
    // --> Gebe TRUE zurueck <--
    return true;
}

// --> Aufheben einer Funktionendefinition <--
bool Define::undefineFunc(const string& sFunc)
{
    string _sFunc = sFunc;
    string sInput = "";
    StripSpaces(_sFunc);
    if (_sFunc.substr(_sFunc.length()-2,2) != "()")
        return false;
    int nth_function = -1;
    for (unsigned int i = 0; i < nDefinedFunctions; i++)
    {
        if (sFunctions[i][0] + "()" == _sFunc)
            nth_function = i;
    }

    if (nth_function == -1)
        return false;

    for (unsigned int i = (unsigned)nth_function; i < nDefinedFunctions-1; i++)
    {
        for (int j = 0; j < 13; j++)
        {
            sFunctions[i][j] = sFunctions[i+1][j];
        }
    }
    for (int j = 0; j < 13; j++)
    {
        sFunctions[nDefinedFunctions-1][j] = "";
    }
    nDefinedFunctions--;
    sFileName = FileSystem::ValidFileName(sFileName, ".def");

    if (ifstream(sFileName.c_str()).good())
    {
        Defines_def.open(sFileName.c_str());
        Defines_def.clear();
        Defines_def.seekg(0);

        nth_function = 0;
        int nLength = 0;
        while (!Defines_def.eof())
        {
            getline(Defines_def, sInput);

            if (sInput.substr(0,sInput.find(';'))+"()" == _sFunc)
            {

                nLength = sInput.length();

                Defines_def.seekg(nth_function);
                for (int i = 0; i < nLength; i++)
                {
                    Defines_def << " ";
                }
                break;
            }
            nth_function += sInput.length()+2;
        }
        Defines_def.close();
    }
    return true;
}

// --> Weitere, zentrale Methode: Aufrufen der Funktionen <--
bool Define::call(string& sExpr, const Settings& _option, int nRecursion)
{
    unsigned int nPos = 0;          // Diverse Positions-Indices:
    unsigned int nPos_2 = 0;        //  - Fuer Anfang und Ende einer Funktion im String

    string sTemp = "";              // Temporaerer String; erleichtert das Ersetzen
    vector<string> vArg;           // String-Array mit den gegebenen Argumenten
    string sImpFunc = "";           // String fuer die jeweils zu ersetzende Funktion
    string sOperators = "+-*/^&|!?:{";
    bool bDoRecursion = false;      // BOOL; Ist TRUE, wenn eine Rekursion noetig ist

    if (!sExpr.length())
        return true;

    // --> Rekursionsabbruchbedingung: 2xDefinierteFunktionen+1 (da nDefinedFunctions auch 0 sein kann) <--
    if ((unsigned)nRecursion == nDefinedFunctions*2 + 1)
    {
        throw TOO_MANY_FUNCTION_CALLS;
    }

    /* --> Ergaenze ggf. Leerzeichen vor und nach dem Ausdruck, damit die Untersuchung der Zeichen vor und
     *     nach einem Treffer keinen SEG-FAULT wirft <--
     */
    if (sExpr[0] != ' ')
        sExpr = " " + sExpr;
    if (sExpr[sExpr.length()-1] != ' ')
        sExpr += " ";

    // --> Lauf durch die gesamte Datenbank <--
    for (unsigned int i = 0; i < nDefinedFunctions; i++)
    {
        nPos = 0;

        // --> Findest du eine Uebereinstimmung? <--
        if (sExpr.find(sFunctions[i][0]+"(") != string::npos)
        {
            if (_option.getbDebug())
                cerr << "|-> DEBUG: Match: i = " << i << endl;

            /* --> Fuer jede Uebereinstimmung: Pruefe, ob es wirklich eine Uebereinstimmung und nicht nur ein aehnlicher Treffer ist <--
             * --> Suche und merke dir die gegebenen Argumente <--
             * --> Ersetze die Uebereinstimmung mit den entsprechenden Definition aus der Datenbank,
             *     wobei natuerlich die entsprechenden Funktionsargumente durch die gegebenen Funktionsargumente
             *     zu ersetzen sind <--
             */
            do
            {
                if (!checkDelimiter(sExpr.substr(sExpr.find(sFunctions[i][0]+"(", nPos)-1, sFunctions[i][0].length()+2))
                    || isInQuotes(sExpr, sExpr.find(sFunctions[i][0]+"(", nPos), true))
                {
                    nPos = sExpr.find(sFunctions[i][0]+"(", nPos) + sFunctions[i][0].length() + 1;
                    continue;
                }

                // --> Kopiere den Teil vor dem Treffer in den temporaeren String <--
                sTemp = sExpr.substr(0,sExpr.find(sFunctions[i][0] + "(", nPos));

                if (_option.getbDebug())
                    cerr << "|-> DEBUG: sTemp = " << sTemp << endl;

                // --> Speichere die Position der zugehoerigen Argument-Klammer <--
                nPos = sExpr.find(sFunctions[i][0]+"(",nPos) + sFunctions[i][0].length();

                // --> Kopiere den String ab der Argumentklammer in sArg[0] <--
                vArg.push_back(sExpr.substr(nPos));
                if (_option.getbDebug())
                    cerr << "|-> DEBUG: vArg[0] = " << vArg[0] << endl;

                // --> Speichere die Position der zugehoerigen, schliessenden Klammer <--
                nPos_2 = getMatchingParenthesis(vArg[0]);
                // --> Pruefen, ob eine Klammer gefunden wurde <--
                if (nPos_2 == string::npos)
                {
                    throw UNMATCHED_PARENTHESIS;
                }

                // --> Trenne den Teil nach der schliessenden Klammer ab <--
                vArg[0].erase(nPos_2);// = sArg[0].substr(0,nPos_2+1);
                vArg[0].erase(0,1);
                nPos += nPos_2 + 1;

                if (_option.getbDebug())
                {
                    cerr << "|-> DEBUG: vArg[0] = " << vArg[0] << endl;
                }
                if (!vArg[0].length() && vArg.size() == 1)
                {
                    vArg.clear();
                }
                // --> Pruefe, ob mehr als ein Funktionsargument gegeben ist und trenne die liste entsprechend <--
                while (vArg.size() && isMultiValue(vArg.back()))
                {
                    vArg.back() = "(" + vArg.back() + ")";
                    vArg.push_back("");
                    if (!parser_SplitArgs(vArg[vArg.size()-2], vArg.back(), ',', _option, false))
                        return false;
                }

                // --> Pruefe, ob die Argumente Operatoren enthalten; also selbst Ausdruecke sind <--
                for (unsigned int j = 0; j < vArg.size(); j++)
                {
                    StripSpaces(vArg[j]);
                    if (!vArg[j].length())
                        vArg[j] = "0";
                    if (vArg[j][0] == '(' && vArg[j][vArg[j].length()-1] == ')')
                        continue;
                    for (unsigned int n = 0; n < sOperators.length(); n++)
                    {
                        if (vArg[j].find(sOperators[n]) != string::npos)
                        {
                            vArg[j] = "(" + vArg[j] + ")";
                            break;
                        }
                    }
                }

                // --> Kopiere die Funktionsdefinition in sImpFunc <--
                sImpFunc = sFunctions[i][1];
                if (_option.getbDebug())
                {
                    for (unsigned int j = 0; j < vArg.size(); j++)
                    {
                        cerr << "|-> DEBUG: vArg[" << j << "] = " << vArg[j] << endl;
                    }
                    cerr << "|-> DEBUG: sImpFunc = " << sImpFunc << endl;
                }

                // --> Ersetze nun die Argumente der Definition durch die gegeben Argumente <--
                for (unsigned int n = 3; n < 13; n++)
                {
                    // --> Gibt's das n-te Argument nicht? Abbrechen! <--
                    if (!sFunctions[i][n].length() && vArg.size() > n-3 && vArg[n-3].length())
                        throw TOO_MANY_ARGS_FOR_DEFINE;
                    else if (!sFunctions[i][n].length())
                        break;

                    // --> Sind weniger Argumente gegeben, als benoetigt werden? Ergaenze den Rest mit "0.0" <--
                    if (n-3 >= vArg.size())
                        vArg.push_back("0");
                    if (!vArg[n-3].length())
                        vArg[n-3] = "0";

                    if (sFunctions[i][n] == ">>...<<")
                    {
                        if (n-2 < vArg.size())
                        {
                            for (unsigned int k = n-2; k < vArg.size(); k++)
                                vArg[n-3] += "," + vArg[k];
                            vArg.erase(vArg.begin()+n-2, vArg.end());
                        }
                    }
                    // --> Ersetze jedes Auftreten des Arguments durch das gegebene <--
                    while (sImpFunc.find(sFunctions[i][n]) != string::npos)
                    {
                        sImpFunc.replace(sImpFunc.find(sFunctions[i][n]), sFunctions[i][n].length(), vArg[n-3]);
                    }
                }

                /* --> Setze alles wieder zusammen <--
                 * --> WICHTIG: Da wir nicht unnoetig pruefen wollen, ob die Funktion in eine Summe, oder
                 *     ein Produkt oder WasAuchImmer eingefuegt werden soll, ergaenzen wir sicherheitshalber
                 *     Klammern um sImpFunc <--
                 */
                if (_option.getbDebug())
                    cerr << "|-> DEBUG: sImpFunc = " << sImpFunc << endl;
                StripSpaces(sImpFunc);
                while (sExpr[nPos] == ')' && sExpr[nPos+1] == ')' && sTemp[sTemp.length()-1] == '(' && sTemp[sTemp.length()-2] == '(')
                {
                    nPos++;
                    sTemp = sTemp.substr(0, sTemp.length()-1);
                }

                if (sImpFunc.front() == '{' && sImpFunc.back() == '}')
                {
                    if (sExpr[nPos] == ')' && sTemp[sTemp.length()-1] == '(')
                    {
                        if (sTemp[sTemp.length()-2] != ' ')
                        {
                            string sDelim = "+-*/^!?:,!&|#";
                            //char c = sTemp[sTemp.length()-2];
                            if (sDelim.find(sTemp[sTemp.length()-2]) != string::npos)
                            {
                                sTemp = sTemp.substr(0, sTemp.length()-1);
                                nPos++;
                            }
                        }
                        else
                        {
                            sTemp = sTemp.substr(0,sTemp.length()-1);
                            nPos++;
                        }
                    }
                    sExpr = sTemp + sImpFunc + sExpr.substr(nPos);
                }
                else if (sTemp[sTemp.length()-1] == '(' && sExpr[nPos] == ')')
                {
                    sExpr = sTemp + sImpFunc + sExpr.substr(nPos);
                }
                else
                {
                    sExpr = sTemp + "(" + sImpFunc + ")" + sExpr.substr(nPos);
                }
                if (_option.getbDebug())
                    cerr << "|-> DEBUG: sExpr = " << sExpr << endl;

                // --> Alle benoetigten Variablen zureucksetzen! <--
                sImpFunc = "";
                vArg.clear();

                /* --> Wenn auch nur eine einzige Ersetzung vorgenommen wurde, kann es sein, dass
                 *     dadurch eine andere Funktion eingefuegt wurde. Wir setzen also den BOOL bDoRecursion
                 *     auf TRUE <--
                 */
                if (!bDoRecursion)
                    bDoRecursion = true;
            }
            while (sExpr.find(sFunctions[i][0]+"(",nPos) != string::npos);
        }
    }

    // --> Wenn es noetig ist, eine weitere Rekursion durchzufuehren, mach' das hier <--
    if (bDoRecursion)
    {
        if (!call(sExpr, _option, nRecursion+1))
            return false;
    }

    // --> Offenbar hat alles geklappt und die Rekursionen sind erfolgreich abgebrochen: Gib TRUE zureuck! <--
    return true;
}

// --> Gibt einfach nur die Zahl der definierten Funktionen zurueck <--
unsigned int Define::getDefinedFunctions() const
{
    return nDefinedFunctions;
}

// --> Gibt einfach nur die Definition der Funktion _i zurueck <--
string Define::getDefine(unsigned int _i) const
{
    if (_i < nDefinedFunctions)
        return sFunctions[_i][2];
    else
        return "";
}

string Define::getFunction(unsigned int _i) const
{
    if (_i < nDefinedFunctions)
        return sFunctions[_i][2].substr(0,sFunctions[_i][2].find(')')+1);
    else
        return "";
}
string Define::getImplemention(unsigned int _i) const
{
    if (_i >= nDefinedFunctions)
        return "";
    string sImplemention = sFunctions[_i][2].substr(sFunctions[_i][2].find(":=")+2);

    if (matchParams(sImplemention, "comment", '=') || matchParams(sImplemention, "recursive") || matchParams(sImplemention, "asval", '='))
    {
        if (sImplemention.find("-set"))
            sImplemention.erase(sImplemention.rfind("-set"));
        else if (sImplemention.find("--"))
            sImplemention.erase(sImplemention.rfind("--"));
    }
    StripSpaces(sImplemention);
    return sImplemention;
}
string Define::getComment(unsigned int _i) const
{
    if (_i >= nDefinedFunctions)
        return "";
    if (matchParams(sFunctions[_i][2], "comment", '='))
        return getArgAtPos(sFunctions[_i][2], matchParams(sFunctions[_i][2], "comment", '=')+7);
    else
        return "";
}

// --> Speichern der Definitionen: Falls man manche Funktionen wiederverwenden moechte <--
bool Define::save(const Settings& _option)
{

    sFileName = FileSystem::ValidFileName(sFileName, ".def");
    string* sDefines_def = 0;
    string sTemp = "";
    unsigned int nIndex = 0;
    if (nDefinedFunctions)
    {
        if (_option.getSystemPrintStatus())
            NumeReKernel::printPreFmt("|-> " + toSystemCodePage(_lang.get("DEFINE_SAVING_FUNCTIONS")) + " ... ");
        if (ifstream(sFileName.c_str()).good())
        {
            Defines_def.open(sFileName.c_str(), ios_base::in);
            Defines_def.clear();    // Falls es zuvor Probleme beim Lesen gegeben hat, loeschen wir hier die Fehler-Flags
            Defines_def.seekg(0);   // Anfang der Datei suchen

            while (!Defines_def.eof())
            {
                getline(Defines_def, sTemp);
                nIndex++;
            }
            if (!sTemp.length())
                nIndex--;
            Defines_def.clear();
            Defines_def.seekg(0);

            sDefines_def = new string[nIndex+nDefinedFunctions];

            for (unsigned int i = 0; i < nIndex+nDefinedFunctions; i++)
            {
                getline(Defines_def, sDefines_def[i]);
                StripSpaces(sDefines_def[i]);
                if (i >= nIndex)
                    sDefines_def[i] = "";
            }
            Defines_def.close();
            remove(sFileName.c_str());
            // --> Schreiben der Definitionen in die Datei "functions.def": wir kopieren das Array 1:1 <--
            sDefines_def[0] = "# Dieses File speichert die Funktionsdefinitionen. Bearbeiten auf eigene Gefahr!";
            for (unsigned int i = 0; i < nDefinedFunctions; i++)
            {
                int nPos = -1;
                for (unsigned int n = 0; n < nIndex+nDefinedFunctions; n++)
                {
                    if (sDefines_def[n].substr(0,sDefines_def[n].find(';')) == sFunctions[i][0])
                    {
                        nPos = n;
                        sDefines_def[n] = "";
                        break;
                    }
                }
                if (nPos == -1)
                {
                    for (unsigned int n = 0; n < nIndex+nDefinedFunctions; n++)
                    {
                        if (!sDefines_def[n].length())
                        {
                            nPos = n;
                            break;
                        }
                    }
                }

                for (int j = 0; j < 13; j++)
                {
                    if (!sFunctions[i][j].length())
                        break;
                    sDefines_def[nPos] += sFunctions[i][j]+"; ";
                }
            }
        }
        else
        {
            nIndex = 1;
            sDefines_def = new string[nDefinedFunctions+nIndex];
            sDefines_def[0] = "# Dieses File speichert die Funktionsdefinitionen. Bearbeiten auf eigene Gefahr!";
            for (unsigned int i = 0; i < nDefinedFunctions; i++)
            {
                sDefines_def[i+1] = "";
                for (unsigned int j = 0; i < 13; j++)
                {
                    if (!sFunctions[i][j].length())
                        break;
                    sDefines_def[i+1] += sFunctions[i][j] + "; ";
                }
            }
        }
        Defines_def.open(sFileName.c_str(), ios_base::out);
        Defines_def.clear();
        Defines_def.seekg(0);
        for (unsigned int i = 0; i < nIndex+nDefinedFunctions; i++)
        {
            if (i >= 100)
                break;
            if (sDefines_def[i].length())
                Defines_def << sDefines_def[i] << endl;
        }
        // --> Fertig? Schliessen der Datei nicht vergessen! <--
        Defines_def.close();
        if (sDefines_def)
        {
            delete[] sDefines_def;
            sDefines_def = 0;
        }
        if (_option.getSystemPrintStatus())
            NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_SUCCESS")) + ".\n");
        return true;
    }
    else
    {
        NumeReKernel::print(toSystemCodePage(_lang.get("PARSERFUNCS_LISTDEFINE_EMPTY")));
        return false;
    }
}

// --> Laden frueherer Funktionsdefinitionen <--
bool Define::load(const Settings& _option, bool bAutoLoad)
{
    sFileName = FileSystem::ValidFileName(sFileName, ".def");
    map<string,int> mFunctions;
    string** sFunc_Temp = 0;
    // --> Sind bereits Funktionen definiert? Diese werden ueberschrieben! <--
    if (nDefinedFunctions)
    {
        string sTemp = "";
        NumeReKernel::print(LineBreak(_lang.get("DEFINE_ASK_OVERRIDE"), _option));
        NumeReKernel::printPreFmt("|\n|<- ");
        NumeReKernel::getline(sTemp);
        StripSpaces(sTemp);
        if (sTemp != _lang.YES())
        {
            NumeReKernel::print(_lang.get("COMMON_CANCEL") + ".");
            return false;
        }
    }
    if (_option.getSystemPrintStatus() && !bAutoLoad)
    {
        if (!bAutoLoad)
            NumeReKernel::printPreFmt("|-> ");
        NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("DEFINE_LOADING_FUNCTIONS")) + " ... ");
    }
    string sIn = "";
    // --> Oeffne Datei "functions.def" <--
    Defines_def.open(sFileName.c_str(), ios_base::in);

    // --> Dateifehler abfangen! <--
    if (Defines_def.fail())
    {
        NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_FAILURE")) + ".\n");
        //cerr << LineBreak("|-> FEHLER: Aus der Datei \"" + sFileName + "\" kann nicht gelesen werden oder die Datei existiert nicht!", _option) << endl;
        Defines_def.close();
        throw FILE_NOT_EXIST;
    }
    getline(Defines_def, sIn);

    // --> Manchmal kann es sein, dass erst nach dem ersten Leseversuch ein fail-Flag auftritt. Fangen wir hier ab! <--
    if (Defines_def.fail())
    {
        NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_FAILURE")) + ".\n");
        //cerr << LineBreak("|-> FEHLER: Aus der Datei \"" + sFileName + "\" kann nicht gelesen werden oder die Datei existiert nicht!", _option) << endl;
        Defines_def.close();
        throw FILE_NOT_EXIST;
    }

    int i = 0;
    int j = 0;
    // --> So lange das EOF-Flag nicht erscheint, lese wiederholt Zeilen ein <--
    while (!Defines_def.eof())
    {
        getline(Defines_def, sIn);
        if (Defines_def.fail())     // Unwahrscheinlicher Fall: Fail-Flag. Trotzdem abfangen
            break;
        if (sIn.find(';') == string::npos)    // Kein Semikolon? Dann steht da etwas sinnloses. Ueberspringen wir diese Zeile
            continue;
        StripSpaces(sIn);
        if (sIn[0] == '#' || !sIn.length())
            continue;

        // --> Wir teilen die eingelesene Zeile an den Semikola auf und kopieren die jeweiligen Tokens in unser Array <--
        do
        {
            if (j == 13)
                break;
            sFunctions[i][j] = sIn.substr(0,sIn.find(';'));
            sIn = sIn.substr(sIn.find(';')+1);
            StripSpaces(sFunctions[i][j]);
            if (!sIn.length())
                break;
            j++;
        }
        while (sIn.find(';') != string::npos);

        // --> Zahl der definierten Funktionen erhoehen <--
        i++;
        nDefinedFunctions = i;

        // --> Seg-Faults abfangen! <--
        if (i == 100)
            break;
        j = 0;
    }
    // --> Zuruecksetzen des FileStreams und Schliessen der Datei <--
    Defines_def.clear();
    Defines_def.close();

    // --> Kopieren der Funktionsnamen und der Indices in die map <--
    for (unsigned int n = 0; n < nDefinedFunctions; n++)
    {
        if (n < 10)
            mFunctions[toLowerCase(sFunctions[n][0])+"!0"+toString((int)n)] = n;
        else
            mFunctions[toLowerCase(sFunctions[n][0])+"!"+toString((int)n)] = n;
    }
    sFunc_Temp = new string*[nDefinedFunctions];
    for (unsigned int n = 0; n < nDefinedFunctions; n++)
    {
        sFunc_Temp[n] = new string[13];
        for (unsigned int k = 0; k < 13; k++)
        {
            sFunc_Temp[n][k] = sFunctions[n][k];
        }
    }

    map<string,int>::const_iterator item = mFunctions.begin();
    unsigned int nPos = 0;
    for (; item != mFunctions.end(); ++item)
    {
        for (unsigned int k = 0; k < 13; k++)
        {
            sFunctions[nPos][k] = sFunc_Temp[item->second][k];
        }
        nPos++;
    }

    for (unsigned int n = 0; n < nDefinedFunctions; n++)
    {
        delete[] sFunc_Temp[n];
    }
    delete[] sFunc_Temp;
    sFunc_Temp = 0;

    if (bAutoLoad);
        //NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("DEFINE_DONE_AUTOLOADING", toString((int)nDefinedFunctions))));
    else if (_option.getSystemPrintStatus())
    {
        NumeReKernel::printPreFmt(toSystemCodePage(_lang.get("COMMON_SUCCESS")) + ".\n");
        NumeReKernel::print(LineBreak(_lang.get("DEFINE_DONE_LOADING", toString((int)nDefinedFunctions)), _option));
    }
    return true;
}

