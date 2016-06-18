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


#include "tools.hpp"


string toString(int nNumber)
{
    return toString((long long int)nNumber);
}

string toString(time_t tTime, bool bOnlyTime)
{
    tm *ltm = localtime(&tTime);
	ostringstream Temp_str;

    if (!bOnlyTime)
    {
        if(ltm->tm_mday < 10)		// 0, falls Tag kleiner als 10
            Temp_str << "0";
        Temp_str << ltm->tm_mday << "."; 	// DD
        if(1+ltm->tm_mon < 10)		// 0, falls Monat kleiner als 10
            Temp_str << "0";
        Temp_str << 1+ltm->tm_mon << "."; // MM-
        Temp_str << 1900+ltm->tm_year << ", "; //YYYY-
  	}
  	if(ltm->tm_hour < 10)
		Temp_str << "0";
    if (bOnlyTime)
        Temp_str << ltm->tm_hour-1; 	// hh
	else
        Temp_str << ltm->tm_hour; 	// hh
	Temp_str << ":";		// ':' im regulaeren Datum
	if(ltm->tm_min < 10)
		Temp_str << "0";
	Temp_str << ltm->tm_min;	// mm
	Temp_str << ":";
	if(ltm->tm_sec < 10)
		Temp_str << "0";
	Temp_str << ltm->tm_sec;	// ss

    return Temp_str.str();
}

/* Diese Funktion vergleicht ganz einfach einen gegebenen Parameter mit der Eingabe. Wird der Parameter gefunden, gibt diese
 * Funktion die Position des ersten char des Parameters +1 zurueck!
 */
int matchParams(const string& sCmd, const string& sParam, const char cFollowing)
{
    // --> Wenn kein '-' im string zu finden ist, ist da auch kein Parameter: FALSE zurueckgeben <--
    if (sCmd.find('-') == string::npos)
        return 0;
    else
    {
        int nLength = 0;
        // --> Wandle den uebergebenen String erst mal in Kleinbuchstaben um <--
        string __sCmd = toLowerCase(sCmd + " ");

        // --> Suche nach dem geforderten Parameter <--
        if (__sCmd.substr(__sCmd.find('-')).find(sParam) != string::npos)
        {
            // --> Wenn du ihn gefunden hast, trenne den String am '-' <--
            int nPos = 0;
            nLength = __sCmd.find('-');
            __sCmd = __sCmd.substr(__sCmd.find('-')) + " ";

            /* --> Es kann nun immer noch sein, dass der gefundene Parameter Teil von
             *     einem anderen Ausdruck ist. Daher untersuche ihn hier und suche ggf.
             *     nach einem anderen Treffer <--
             */
            do
            {
                // --> Speichere die Position des Parameter-Kandidaten <--
                nPos = __sCmd.find(sParam, nPos);

                /* --> Pruefe die Zeichen davor und danach (unter Beachtung eines moeglicherweise
                 *     speziell gewaehlten Zeichens) <--
                 * --> Ein Parameter darf auf jeden Fall kein Teil eines anderen, laengeren Wortes
                 *     sein <--
                 */
                if (cFollowing == ' ')
                {
                    if ((__sCmd[nPos-1] == ' '
                            || __sCmd[nPos-1] == '-')
                        && (__sCmd[nPos+sParam.length()] == ' '
                            || __sCmd[nPos+sParam.length()] == '-'
                            || __sCmd[nPos+sParam.length()] == '"')
                    )
                    {
                        if (__sCmd[nPos-1] == ' ' && __sCmd[__sCmd.find_last_not_of(' ', nPos-1)] == '=')
                            nPos += sParam.length();
                        else
                            return nPos+1+nLength;    // nPos+1+nLength zurueckgeben, wenn Treffer
                    }
                    else
                        nPos += sParam.length();    // Positionsindex um die Laenge des Parameters weitersetzen
                }
                else
                {
                    // --> Wenn ein spezielles Zeichen gewaehlt wurde, wird dies hier gesucht <--
                    if ((__sCmd[nPos-1] == ' ' || __sCmd[nPos-1] == '-')
                        && (__sCmd[nPos+sParam.length()] == cFollowing))
                    {
                        if (__sCmd[nPos-1] == ' ' && __sCmd[__sCmd.find_last_not_of(' ', nPos-1)] == '=')
                            nPos += sParam.length();
                        else
                            return nPos+1+nLength;
                    }
                    else if ((__sCmd[nPos-1] == ' ' || __sCmd[nPos-1] == '-')
                        && (__sCmd[nPos+sParam.length()] == ' '))
                    {
                        if (__sCmd[nPos-1] == ' ' && __sCmd[__sCmd.find_last_not_of(' ', nPos-1)] == '=')
                            nPos += sParam.length();
                        else
                        {
                            /* --> Fehlertoleranz: Leerzeichen zwischen dem Parameter und cFollowing werden ignoriert
                             *     (wenn cFollowing sowieso ein Leerzeichen sein sollte, wurde das ohnehin vorhin schon abgefangen) <--
                             */
                            int nSkip = 0;
                            while (nPos+sParam.length()+nSkip < __sCmd.length() && __sCmd[nPos+sParam.length()+nSkip] == ' ')
                                nSkip++;
                            if (__sCmd[nPos+sParam.length()+nSkip] == cFollowing)
                                return nPos+1+nLength+nSkip; // Wir addieren nSkip, da der Rueckgabewert vorzugsweise zum Navigieren zum Ende des Parameters verwendet wird
                            else
                                nPos += sParam.length();
                        }
                    }
                    else
                        nPos += sParam.length();
                }
            }
            while (__sCmd.find(sParam, nPos) != string::npos);
        }
    }

    // --> Wenn nichts gefunden wurde, gib' FALSE zurueck <--
    return 0;
}

/* Diese Funktion sucht nach einem String-Argument in einem Kommando und schreibt dieses in sArgument. Falls
 * keines gefunden wird, gibt die Funktion FALSE zurueck, anderenfalls TRUE
 */
bool getStringArgument(const string& sCmd, string& sArgument)
{
    unsigned int nPos = 0;
    unsigned int nPos_2 = 0;
    //cerr << sCmd << endl;

    // --> Wenn kein '"' oder kein '#' zu finden ist, gibt es auch kein String-Argument: FALSE zurueckgeben <--
    if (!containsStrings(sCmd))
        return false;
    else
    {
        nPos = sCmd.find('"');
        if (sCmd.find('#') != string::npos && (sCmd.find('#') < nPos || nPos == string::npos))
            nPos = sCmd.find('#');
        if (sCmd.find("to_string(") != string::npos && (sCmd.find("to_string(") < nPos || nPos == string::npos))
            nPos = sCmd.find("to_string(");
        if (sCmd.find("string(") != string::npos && (sCmd.find("string(") < nPos || nPos == string::npos))
            nPos = sCmd.find("string(");
        if (sCmd.find("substr(") != string::npos && (sCmd.find("substr(") < nPos || nPos == string::npos))
            nPos = sCmd.find("substr(");
        if (sCmd.find("strlen(") != string::npos && (sCmd.find("strlen(") < nPos || nPos == string::npos))
            nPos = sCmd.find("strlen(");
        if (sCmd.find("strfnd(") != string::npos && (sCmd.find("strfnd(") < nPos || nPos == string::npos))
            nPos = sCmd.find("strfnd(");
        if (sCmd.find("ascii(") != string::npos && (sCmd.find("ascii(") < nPos || nPos == string::npos))
            nPos = sCmd.find("ascii(");
        if (sCmd.find("to_char(") != string::npos && (sCmd.find("to_char(") < nPos || nPos == string::npos))
            nPos = sCmd.find("to_char(");
        if (sCmd.find("char(") != string::npos && (sCmd.find("char(") < nPos || nPos == string::npos))
            nPos = sCmd.find("char(");

        //cerr << nPos << endl;
        for (unsigned int i = nPos; i < sCmd.length(); i++)
        {
            //cerr << i << endl;
            if (sCmd.substr(i, 7) == "string("
                || sCmd.substr(i, 7) == "substr("
                || sCmd.substr(i, 7) == "strlen("
                || sCmd.substr(i, 7) == "strfnd(")
            {
                i += getMatchingParenthesis(sCmd.substr(i+6))+6;
                continue;
            }
            if (sCmd.substr(i, 5) == "char(")
            {
                i += getMatchingParenthesis(sCmd.substr(i+4))+4;
                continue;
            }
            if (sCmd.substr(i, 6) == "ascii(")
            {
                i += getMatchingParenthesis(sCmd.substr(i+5))+5;
                continue;
            }

            if (sCmd[i] == '#')
            {
                for (unsigned int j = i; j < sCmd.length(); j++)
                {
                    if (sCmd[j] == ' ')
                    {
                        i = j;
                        break;
                    }
                    if (sCmd[j] == '(')
                    {
                        j += getMatchingParenthesis(sCmd.substr(j));
                    }
                    if (j == sCmd.length()-1)
                    {
                        i = j;
                        break;
                    }
                }
            }
            //cerr << i << "   " << sCmd[i] << endl;
            if (sCmd[i] == ' ' && !isInQuotes(sCmd, i))
            {
                if (sCmd.find_first_not_of(' ', i) != string::npos && sCmd[sCmd.find_first_not_of(' ', i)] != '+')
                {
                    nPos_2 = i-1;
                    break;
                }
                else
                {
                    i = sCmd.find_first_not_of(' ', i);
                    if (i < sCmd.length()-1 && sCmd[i] == '+' && sCmd[i+1] == ' ')
                        i++;
                    while (i < sCmd.length()-1 && sCmd[i] == ' ' && sCmd[i+1] == ' ')
                        i++;
                }
            }
            if (i >= sCmd.length()-1 || !containsStrings(sCmd.substr(i)))
            {
                if (i == string::npos)
                    nPos_2 = sCmd.length();
                else
                    nPos_2 = i;
                break;
            }
            //cerr << i << endl;
        }
        //cerr << nPos << "   " << nPos_2 << endl;

        sArgument = sCmd.substr(nPos, nPos_2-nPos+1);
        //cerr << sArgument << endl;
        return true;
    }

    // --> Falls du durch alles durchmarschiert bist, gab es einen Fehler: gib sicherheitshalber FALSE zurueck <--
    return false;
}

// Das Integer-Pendant zur BI_getStringArgument()-Funktion
bool getIntArgument(const string& sCmd, int& nArgument)
{
    unsigned int nPos = 0;
    string sTemp = "";

    // --> Kein '-' => Kein Parameter => Kein INT-Argument <--
    if (sCmd.find('-') == string::npos)
        return false;
    else if (sCmd.find('=') != string::npos)
    {
        nPos = sCmd.find('=')+1;
        while (sCmd[nPos] == ' ' && nPos+1 < sCmd.length())
            nPos++;
        if (nPos+1 == sCmd.length() && sCmd[nPos] == ' ')
            return false;
        nArgument = StrToInt(sCmd.substr(nPos, sCmd.find(' ', nPos)-nPos));
        return true;
    }
    else
    {
        nPos = sCmd.find('-')+1;
        // --> Suche das Leerzeichen nach dem Parameter <--
        while (sCmd[nPos] == ' ')
        {
            nPos++;
        }
        nPos = sCmd.find(' ', nPos);
        while (sCmd[nPos] == ' ')
        {
            nPos++;
        }

        // --> Schneide den Rest zusammen <--
        if (sCmd.length() - nPos <= 2)
            sTemp = sCmd.substr(nPos);
        else
            sTemp = sCmd.substr(nPos, sCmd.find(' ', nPos) - nPos);

        // --> Wandle den String in einen INT um <--
        nArgument = StrToInt(sTemp);

        // --> Ergebnis pruefen und entsprechenden BOOL zurueckgeben <--
        if (nArgument)
            return true;
        else
            return false;
    }
    return false;
}

// Entfernt fuehrende und angehaengte Leerstellen/Tabulatoren
void StripSpaces(string& sToStrip)
{
    if (!sToStrip.length())
        return;
    // --> Am Anfgang und am Ende weder ' ' noch '\t' gefunden? Zurueckkehren <--
    if (sToStrip[0] != ' ' && sToStrip[sToStrip.length()-1] != ' ' && sToStrip[0] != '\t' && sToStrip[sToStrip.length()-1] != '\t')
        return;
    sToStrip.erase(0,sToStrip.find_first_not_of(" \t"));
    if (sToStrip.length() && (sToStrip.back() == ' ' || sToStrip.back() == '\t'))
        sToStrip.erase(sToStrip.find_last_not_of(" \t")+1);

    // --> Zurueckkehren <--
    return;
}

// Aus einem String einen Integer machen
int StrToInt(const string& sString)
{
    int nReturn = 0;
    istringstream Temp(sString);
    Temp >> nReturn;
    return nReturn;
}

// Aus einem String einen Double machen
double StrToDb(const string& sString)
{
    double dReturn = 0.0;
    istringstream Temp(sString);
    Temp >> dReturn;
    return dReturn;
}

// Diese Funktion steuert die Kopfzeile der Console
void SetConsTitle(const Datafile& _data, const Settings& _option, string sScript)
{
    string sConsoleTitle = "";  // Variable fuer den Consolentitel
    if (_data.isValid())
    {
        sConsoleTitle = "[" + _data.getDataFileNameShort() + "] ";   // Datenfile-Namen anzeigen, falls geladen
    }
    if (_data.isValidCache())
    {
        sConsoleTitle += "[";
            if (!_data.getSaveStatus())
                sConsoleTitle += "*";       // Anzeigen, dass die Daten nicht gespeichert sind
        sConsoleTitle += "Cached Data] ";   // Anzeigen, dass Daten im Cache sind
    }
    if (sScript.length())
    {
        sConsoleTitle += "[" + sScript + "] ";
    }
    if (_data.isValid() || _data.isValidCache() || sScript.length())
    {
        sConsoleTitle += "- ";  // Bindestrich zur optischen Trennung
    }
    if ((sConsoleTitle.length() + sVersion.length() > _option.getWindow()-40
            && (!_option.getbDebug() && !_option.getUseDebugger()))
        || (sConsoleTitle.length() + sVersion.length() > _option.getWindow()-55
            && (_option.getbDebug() || _option.getUseDebugger())))
    {
        if (sVersion.length() > _option.getWindow()-65 && sConsoleTitle.length() > _option.getWindow()-60)
            sConsoleTitle += "NumeRe (v " + (sVersion.substr(0,5) + AutoVersion::STATUS_SHORT) + ")";
        else
            sConsoleTitle += "NumeRe (v " + sVersion + ")";
    }
    else
        sConsoleTitle += "NumeRe: Framework für Numerische Rechnungen (v " + sVersion + ")";
    if (_option.getUseDebugger())
        sConsoleTitle += " [Debugger "+_lang.get("COMMON_ACTIVE")+"]";
    if (_option.getbDebug())
        sConsoleTitle += " [DEV-MODE]";
    sConsoleTitle = toSystemCodePage(sConsoleTitle);
    SetConsoleTitle(sConsoleTitle.c_str()); // WINDOWS-Funktion aus <windows.h>
    return;
}

// Liefert die aeusserste, schliessende Klammer.
unsigned int getMatchingParenthesis(const string& sLine)
{
    char cParenthesis = sLine[0];
    char cClosingParenthesis = 0;
    switch (cParenthesis)
    {
        case '(':
            cClosingParenthesis = ')';
            break;
        case '{':
            cClosingParenthesis = '}';
            break;
        case '[':
            cClosingParenthesis = ']';
            break;
        default:
            cParenthesis = '(';
            cClosingParenthesis = ')';
    }
    if (sLine.find(cParenthesis) == string::npos && sLine.find(cClosingParenthesis) == string::npos)
        return string::npos;
    int nOpenParenthesis = 0;
    for (unsigned int i = 0; i < sLine.length(); i++)
    {
        /* --> Zaehle oeffnende und schliessende Klammern und gib die Position der
         *     Klammer zurueck, bei der dein Counter == 0 ist <--
         */
        if (sLine.find(cParenthesis) && i < sLine.find(cParenthesis))
            i = sLine.find(cParenthesis);
        if (sLine[i] == cParenthesis && !isInQuotes(sLine, i))
        {
            nOpenParenthesis++;
        }
        if (sLine[i] == cClosingParenthesis && !isInQuotes(sLine, i))
            nOpenParenthesis--;
        if (!nOpenParenthesis && !isInQuotes(sLine, i, true))
            return i;
    }

    // --> Falls die Klammer nicht schliesst, gebe -1 zurueck (analog zu string::find()) <--
    return string::npos;
}

// Tauscht alle Grossbuchstaben gegen Kleinbuchstaben aus
string toLowerCase(const string& sUpperCase)
{
    string sLowerCase = sUpperCase;
    for (unsigned int i = 0; i < sLowerCase.length(); i++)
    {
        // --> Laufe alle Zeichen im String ab und pruefe, ob ihr CHAR-Wert zwischen A und Z liegt
        if ((int)sLowerCase[i] >= (int)'A' && (int)sLowerCase[i] <= (int)'Z')
        {
            // --> Falls ja, verschiebe den CHAR-Wert um die Differenz aus A und a <--
            sLowerCase[i] = (char)((int)sLowerCase[i] + ((int)'a' - (int)'A'));
        }
        if (sLowerCase[i] == 'Ä')
            sLowerCase[i] = 'ä';
        else if (sLowerCase[i] == 'Ö')
            sLowerCase[i] = 'ö';
        else if (sLowerCase[i] == 'Ü')
            sLowerCase[i] = 'ü';
        else if (sLowerCase[i] == (char)142)
            sLowerCase[i] = (char)132;
        else if (sLowerCase[i] == (char)153)
            sLowerCase[i] = (char)148;
        else if (sLowerCase[i] == (char)154)
            sLowerCase[i] = (char)129;
    }
    return sLowerCase;
}

// Tauscht alle Kleinbuchstaben gegen Grossbuchstaben aus
string toUpperCase(const string& sLowerCase)
{
    string sUpperCase = sLowerCase;
    for (unsigned int i = 0; i < sUpperCase.length(); i++)
    {
        // --> Laufe alle Zeichen im String ab und pruefe, ob ihr CHAR-Wert zwischen a und z liegt
        if ((int)sUpperCase[i] >= (int)'a' && (int)sLowerCase[i] <= (int)'z')
        {
            // --> Falls ja, verschiebe den CHAR-Wert um die Differenz aus a und A <--
            sUpperCase[i] = (char)((int)sUpperCase[i] + ((int)'A' - (int)'a'));
        }
        if (sUpperCase[i] == 'ä')
            sUpperCase[i] = 'Ä';
        else if (sUpperCase[i] == 'ö')
            sUpperCase[i] = 'Ö';
        else if (sUpperCase[i] == 'ü')
            sUpperCase[i] = 'Ü';
        else if (sUpperCase[i] == (char)132)
            sUpperCase[i] = (char)142;
        else if (sUpperCase[i] == (char)148)
            sUpperCase[i] = (char)153;
        else if (sUpperCase[i] == (char)129)
            sUpperCase[i] = (char)154;
    }
    return sUpperCase;
}

// Prueft, ob ein Ausdruck ein Mehrfachausdruck oder nur eine Multi-Argument-Funktion ist
bool isMultiValue(const string& sExpr, bool bIgnoreClosingParenthesis)
{
    // --> Kein Komma? Auf jeden Fall kein Mehrfachausdruck! <--
    if (sExpr.find(',') == string::npos)
        return false;
    else
    {
        int nParenthesis = 0;
        int nV_Parenthesis = 0;

        for (unsigned int i = 0; i < sExpr.length(); i++)
        {
            if (sExpr[i] == '(')
                nParenthesis++;
            if (sExpr[i] == ')')
                nParenthesis--;
            if (sExpr[i] == '{')
            {
                nV_Parenthesis++;
                i++;
            }
            if (sExpr[i] == '}')
            {
                nV_Parenthesis--;
                i++;
            }
            if (sExpr[i] == ','
                && ((!nParenthesis && !bIgnoreClosingParenthesis)
                    || (nParenthesis <= 0 && bIgnoreClosingParenthesis))
                && !nV_Parenthesis)
                return true;
        }
        return false;
    }
    return false;
}

// --> Ersetzt Tokens in einem String mit dem entsprechenden TeX-Befehl <--
string replaceToTeX(const string& sString)
{
    string sReturn = " " + sString + " ";   // Rueckgabe-String
    string sTemp = "";                      // Temporaerer String, erleichert das Einfuegen von strings
    static const unsigned int nSymbols = 90;                // Anzahl an bekannten Symbolen
    unsigned int nPos = 0;                           // Positions-Index-Variable
    unsigned int nPos_2 = 0;                         // Positions-Index-Variable

    // --> 2D-String-Array: links das zu ersetzende Token, rechts die Ersetzung <--
    static const string sCodepage[nSymbols][2] = {
        {"*", "\\cdot  "},
        {"+", " + "},
        {"-", " -- "},
        {",", ", "},
        {"x", "{\\i x}"},
        {"y", "{\\i y}"},
        {"z", "{\\i z}"},
        {"t", "{\\i t}"},
        {"_pi", "\\pi "},
        {"_hbar", "\\hbar "},
        {"_k_boltz", "k_B"},
        {"_2pi", "2\\cdot \\pi "},
        {"_elek_feldkonst", "\\varepsilon "},
        {"_elem_ladung", "e"},
        {"_m_elektron", "m_{e}"},
        {"_m_neutron", "m_{n}"},
        {"_m_proton", "m_{p}"},
        {"_m_sonne", "m_{Sonne}"},
        {"_m_erde", "m_{Erde}"},
        {"_m_muon", "m_{\\mu}"},
        {"_m_tau", "m_{\\tau}"},
        {"_magn_feldkonst", "\\mu "},
        {"_n_avogadro", "N_A"},
        {"_r_erde", "r_{Erde}"},
        {"_r_sonne", "r_{Sonne}"},
        {"_c", "c_{Licht}"},
        {"_e", "e"},
        {"_g", "g"},
        {"_h", "h"},
        {"_R", "R"},
        {"_alpha_fs", "\\alpha_{FS}"},
        {"inf", "\\infty"},
        {"_mu_bohr", "\\mu_{B}"},
        {"_mu_kern", "\\mu_{K}"},
        {"_m_amu", "m_u"},
        {"_r_bohr", "a_0"},
        {"_G", "G"},
        {"_theta_weinberg", "\\theta_{W}"},
        {"alpha", "\\alpha "},
        {"Alpha", "\\Alpha "},
        {"beta", "\\beta "},
        {"Beta", "\\Beta "},
        {"gamma", "\\gamma "},
        {"Gamma", "\\Gamma "},
        {"delta", "\\delta "},
        {"Delta", "\\Delta "},
        {"epsilon", "\\varepsilon "},
        {"Epsilon", "\\Epsilon "},
        {"zeta", "\\zeta "},
        {"Zeta", "\\Zeta "},
        {"eta", "\\eta "},
        {"Eta", "\\Eta "},
        {"\theta", "\\theta "},
        {"theta", "\\vartheta "},
        {"Theta", "\\Theta "},
        {"iota", "\\iota "},
        {"Iota", "\\Iota "},
        {"kappa", "\\kappa "},
        {"Kappa", "\\Kappa "},
        {"lambda", "\\lambda "},
        {"Lambda", "\\Lambda "},
        {"mu", "\\mu"},
        {"Mu", "\\Mu "},
        {"\nu", "\\nu "},
        {"nu", "\\nu "},
        {"Nu", "\\Nu "},
        {"xi", "\\xi "},
        {"Xi", "\\Xi "},
        {"omikron", "o "},
        {"Omikron", "O "},
        {"pi", "\\pi "},
        {"Pi", "\\Pi "},
        {"rho", "\\rho "},
        {"Rho", "\\Rho "},
        {"sigma", "\\sigma "},
        {"Sigma", "\\Sigma "},
        {"\tau", "\\tau "},
        {"tau", "\\tau "},
        {"Tau", "\\Tau "},
        {"ypsilon", "\\upsilon "},
        {"Ypsilon", "\\Upsilon "},
        {"phi", "\\varphi "},
        {"Phi", "\\Phi"},
        {"chi", "\\chi "},
        {"Chi", "\\Chi "},
        {"psi", "\\psi "},
        {"Psi", "\\Psi "},
        {"omega", "\\omega "},
        {"Omega", "\\Omega "},
        {"heaviside", "\\Theta"}
    };


    // --> Ersetze zunaechst die gamma-Funktion <--
    while (sReturn.find("gamma(", nPos) != string::npos)
    {
        nPos = sReturn.find("gamma(", nPos);
        sReturn = sReturn.substr(0,nPos) + "\\Gamma(" + sReturn.substr(nPos+6);
        nPos += 7;
    }

    nPos = 0;
    // --> Laufe durch alle bekannten Symbole <--
    for (unsigned int i = 0; i < nSymbols; i++)
    {
        // --> Positions-Indices zuruecksetzen <--
        nPos = 0;
        nPos_2 = 0;

        // --> So lange in dem String ab der Position nPos das Token auftritt <--
        while (sReturn.find(sCodepage[i][0], nPos) != string::npos)
        {
            // --> Position des Treffers speichern <--
            nPos_2 = sReturn.find(sCodepage[i][0], nPos);
            // --> Falls vor dem Token schon ein '\' ist, wurde das hier schon mal ersetzt <--
            if (sReturn[nPos_2-1] == '\\')
            {
                // --> Positionsindex um die Laenge des Tokens weitersetzen <--
                nPos = nPos_2+sCodepage[i][0].length();
                continue;
            }
            else if (i < 4)
            {
                while (nPos_2 > 0 && sReturn[nPos_2-1] == ' ')
                {
                    sReturn = sReturn.substr(0, nPos_2-1) + sReturn.substr(nPos_2);
                    nPos_2--;
                }
                while (nPos_2 < sReturn.length()-2 && sReturn[nPos_2+1] == ' ')
                {
                    sReturn = sReturn.substr(0, nPos_2+1) + sReturn.substr(nPos_2+2);
                }
                if ((i == 1 || i == 2)
                    && (sReturn[nPos_2-1] == 'e' || sReturn[nPos_2-1] == 'E'))
                {
                    if ((int)sReturn[nPos_2-2] <= (int)'9' && (int)sReturn[nPos_2-2] >= (int)'0'
                        && (int)sReturn[nPos_2+1] <= (int)'9' && (int)sReturn[nPos_2+1] >= (int)'0')
                    {
                        nPos = nPos_2+1;
                        continue;
                    }
                }
                if ((i == 1 || i == 2)
                    && (sReturn[nPos_2-1] == '(' || sReturn[nPos_2-1] == '[' || sReturn[nPos_2-1] == '{' || sReturn[nPos_2-1] == ',' || !nPos_2))
                {
                    if (i == 2)
                    {
                        sReturn.insert(nPos_2,1,'-');
                        nPos_2++;
                    }
                    nPos = nPos_2+1;
                    continue;
                }
            }
            else if (i > 3 && sReturn[nPos_2+sCodepage[i][0].length()] == '_')
            {
                // Wird das Token von '_' abgeschlossen? Pruefen wir, ob es von vorne auch begrenzt ist <--
                if (!checkDelimiter(sReturn.substr(nPos_2-1, sCodepage[i][0].length()+1)+" "))
                {
                    // --> Nein? Den Positionsindex um die Laenge des Tokens weitersetzen <--
                    nPos = nPos_2+sCodepage[i][0].length();
                    continue;
                }
            }
            else if (i > 2 && !checkDelimiter(sReturn.substr(nPos_2-1, sCodepage[i][0].length()+2)))
            {
                // --> Pruefen wir auch getrennt den Fall, ob das Token ueberhaupt begrenzt ist ('_' zaehlt nicht zu den Delimitern) <--
                nPos = nPos_2+sCodepage[i][0].length();
                continue;
            }

            // --> Das war alles nicht der Fall? Schieb den Index um die Laenge der Ersetzung weiter <--
            nPos_2 += sCodepage[i][1].length();

            // --> Kopiere den Teil nach dem Token in sTemp <--
            sTemp = sReturn.substr(sReturn.find(sCodepage[i][0], nPos) + sCodepage[i][0].length());

            // --> Kopiere den Teil vor dem Token, die Ersetzung und sTemp in sReturn <--
            sReturn = sReturn.substr(0,sReturn.find(sCodepage[i][0], nPos)) + sCodepage[i][1] + sTemp;

            // --> Setze den Hauptindex auf nPos_2 <--
            nPos = nPos_2;
        }
    }

    // --> Ersetze nun lange Indices "_INDEX" durch "_{INDEX}" <--
    string sDelimiter = "+-*/, #()&|!_'";
    for (unsigned int i = 0; i < sReturn.length()-1; i++)
    {
        if (sReturn[i] == '^' && sReturn[i+1] != '{' && sReturn[i+1] != '(')
        {
            i++;
            sReturn = sReturn.substr(0,i) + "{" + sReturn.substr(i);
            if (sReturn[i+1] == '-' || sReturn[i+1] == '+')
                i++;
            i++;
            for (unsigned int j = i+1; j < sReturn.length(); j++)
            {
                if (sDelimiter.find(sReturn[j]) != string::npos)
                {
                    sReturn = sReturn.substr(0,j) + "}" + sReturn.substr(j);
                    break;
                }
                else if (j+1 == sReturn.length())
                {
                    sReturn += "}";
                }
            }
        }
    }

    sDelimiter[sDelimiter.length()-1] = '^';
    for (unsigned int i = 0; i < sReturn.length()-1; i++)
    {
        if (sReturn[i] == '_' && sReturn[i+1] != '{')
        {
            for (unsigned int j = 4; j < nSymbols; j++)
            {
                if (sCodepage[j][0][0] != '_')
                    break;
                if (sReturn.substr(i, sCodepage[j][0].length()) == sCodepage[j][0] && (sDelimiter.find(sReturn[i+sCodepage[j][0].length()]) != string::npos || sReturn[i+sCodepage[j][0].length()] == '_'))
                {
                    i++;
                    break;
                }
            }
        }
        if (sReturn[i] == '_' && sReturn[i+1] != '{')
        {
            i++;
            sReturn = sReturn.substr(0,i) + "{" + sReturn.substr(i);
            i++;
            for (unsigned int j = i+1; j < sReturn.length(); j++)
            {
                if (sDelimiter.find(sReturn[j]) != string::npos)
                {
                    sReturn = sReturn.substr(0,j) + "}" + sReturn.substr(j);
                    break;
                }
                else if (j+1 == sReturn.length())
                {
                    sReturn += "}";
                }
            }
        }
    }

    // --> Setze nun den Hauptindex zurueck <--
    nPos = 0;

    // --> Pruefe nun kompliziertere Tokens: zuerst die Wurzel "sqrt()" <--
    while (sReturn.find("sqrt(", nPos) != string::npos)
    {
        // --> Speichere die Position der Klammer <--
        nPos_2 = sReturn.find("sqrt(", nPos)+4;

        // --> Kopiere den Teil ab der Klammer in sTemp <--
        sTemp = sReturn.substr(sReturn.find("sqrt(", nPos)+4);

        // --> Kopiere den Teil vor "sqrt(" in sReturn und haenge "@{\\sqrt{" an <--
        sReturn = sReturn.substr(0,sReturn.find("sqrt(", nPos)) + "@{\\sqrt{";

        // --> Haenge sTemp an sReturn an, wobei die passende schliessende Klammer durch '}}' ersetzt werden muss <--
        sReturn += sTemp.substr(1,getMatchingParenthesis(sTemp)-1) + "}}" + sTemp.substr(getMatchingParenthesis(sTemp)+1);

        // --> nPos auf nPos_2 setzen <--
        nPos = nPos_2;
    }

    // --> Nun "norm(x,y,z,...)" <--
    while (sReturn.find("norm(", nPos) != string::npos)
    {
        // --> Speichere die Position der Klammer <--
        nPos_2 = sReturn.find("norm(", nPos)+4;

        // --> Kopiere den Teil ab der Klammer in sTemp <--
        sTemp = sReturn.substr(sReturn.find("norm(", nPos)+4);

        // --> Kopiere den Teil vor "norm(" in sReturn und haenge "|" an <--
        sReturn = sReturn.substr(0,sReturn.find("norm(", nPos)) + "|";

        // --> Haenge sTemp an sReturn an, wobei die passende schliessende Klammer durch '|' ersetzt werden muss <--
        sReturn += sTemp.substr(1,getMatchingParenthesis(sTemp)-1) + "|" + sTemp.substr(getMatchingParenthesis(sTemp)+1);

        // --> nPos auf nPos_2 setzen <--
        nPos = nPos_2;
    }

    // --> Nun "abs(x,y,z,...)" <--
    while (sReturn.find("abs(", nPos) != string::npos)
    {
        // --> Speichere die Position der Klammer <--
        nPos_2 = sReturn.find("abs(", nPos)+3;

        // --> Kopiere den Teil ab der Klammer in sTemp <--
        sTemp = sReturn.substr(sReturn.find("abs(", nPos)+3);

        // --> Kopiere den Teil vor "norm(" in sReturn und haenge "|" an <--
        sReturn = sReturn.substr(0,sReturn.find("abs(", nPos)) + "|";

        // --> Haenge sTemp an sReturn an, wobei die passende schliessende Klammer durch '|' ersetzt werden muss <--
        sReturn += sTemp.substr(1,getMatchingParenthesis(sTemp)-1) + "|" + sTemp.substr(getMatchingParenthesis(sTemp)+1);

        // --> nPos auf nPos_2 setzen <--
        nPos = nPos_2;
    }

    // --> Hauptindex zuruecksetzen <--
    nPos = 0;

    // --> Ersetzte nun lange Hochzahlen "^(ZAHL)" durch "^{ZAHL}" <--
    while (sReturn.find("^(", nPos) != string::npos)
    {
        nPos_2 = sReturn.find("^(", nPos)+1;
        sTemp = sReturn.substr(nPos_2);
        sReturn = sReturn.substr(0, nPos_2) + "{";
        sReturn += sTemp.substr(1,getMatchingParenthesis(sTemp)-1) + "}" + sTemp.substr(getMatchingParenthesis(sTemp)+1);

        nPos = nPos_2;
    }

    // --> Entferne die Leerzeichen am Anfang und Ende und gib sReturn zurueck <--
    StripSpaces(sReturn);
    return sReturn;
}

// --> Extrahiert das Kommando aus einem Befehl <--
Match findCommand(const string& sCmd, string sCommand)
{
    //cerr << "\"" << sCmd << "\"" << endl;
    Match _mMatch;
    _mMatch.sString = "";
    _mMatch.nPos = string::npos;
    if (!sCommand.length())
    {
        for (unsigned int i = 0; i < sCmd.length(); i++)
        {
            if ((sCmd.substr(i,2) == "--" || sCmd.substr(i,5) == "-set ") && !isInQuotes(sCmd, i))
                break;
            if ((sCmd[i] == ' ' || sCmd[i] == '\t' || sCmd[i] == '-' || sCmd[i] == '=') && _mMatch.nPos == string::npos)
                continue;
            else if ((sCmd[i] == ' ' || sCmd[i] == '-') && _mMatch.nPos != string::npos)
            {
                if (sCmd[i] != '-' && sCmd.find_first_not_of(' ',i) != string::npos && sCmd[sCmd.find_first_not_of(' ',i)] == '=')
                {
                    _mMatch.nPos = string::npos;
                    continue;
                }
                _mMatch.sString = sCmd.substr(_mMatch.nPos,i-_mMatch.nPos);
                if (isInQuotes(sCmd, (i-_mMatch.nPos)/2))
                {
                    _mMatch.sString = "";
                    _mMatch.nPos = string::npos;
                }
                return _mMatch;
            }
            else if ((sCmd[i] == '(') && _mMatch.nPos != string::npos)
            {
                _mMatch.sString = sCmd.substr(_mMatch.nPos,i-_mMatch.nPos);
                if (isInQuotes(sCmd, (i-_mMatch.nPos)/2)
                    || (_mMatch.sString != "if"
                        && _mMatch.sString != "for"
                        && _mMatch.sString != "while"))
                {
                    _mMatch.sString = "";
                    _mMatch.nPos = string::npos;
                }
                return _mMatch;
            }
            if (_mMatch.nPos == string::npos)
                _mMatch.nPos = i;
        }
        if (_mMatch.nPos != string::npos)
            _mMatch.sString = sCmd.substr(_mMatch.nPos);
        else
            _mMatch.sString = sCmd;
        return _mMatch;
    }
    else if (sCommand.length() && (sCmd == sCommand || sCmd == sCommand + " "))
    {
        _mMatch.sString = sCmd;
        StripSpaces(_mMatch.sString);
        _mMatch.nPos = 0;
        return _mMatch;
        //return sCommand;
    }
    else if (sCommand.length()
        && sCmd.find(sCommand) != string::npos
        && sCmd.find(' ', sCmd.find(sCommand)) != string::npos
        && findCommand(sCmd).sString != "help"
        && findCommand(sCmd).sString != "edit"
        && findCommand(sCmd).sString != "new")
    {
        for (unsigned int i = 0; i < sCmd.length(); i++)
        {
            if ((sCmd.substr(i,2) == "--" || sCmd.substr(i,5) == "-set ") && !isInQuotes(sCmd, i))
                break;
            if (sCmd[i] == ' ' || sCmd[i] == '\t' || sCmd[i] == '(')
                continue;
            if (sCmd.substr(i,sCommand.length()) != sCommand)
                continue;
            if (sCmd.substr(i,sCommand.length()) == sCommand && i)
            {
                //cerr << sCmd.substr(i,sCommand.length()) << endl;
                //cerr << sCommand << endl;
                if (i+sCommand.length() == sCmd.length()-1)
                {
                    //cerr << "i+scommand.length()..." << endl;
                    _mMatch.sString = sCmd.substr(i-1) + " ";
                    //cerr << "sCommand.substr(i-1)..." << endl;
                    _mMatch.nPos = i;
                    if (checkDelimiter(_mMatch.sString) && !isInQuotes(sCmd, i))
                    {
                        _mMatch.sString = _mMatch.sString.substr(1,_mMatch.sString.length()-2);
                        //cerr << "_mMatch.sString.substr(...)" << endl;
                    }
                    else
                    {
                        _mMatch.sString = "";
                        _mMatch.nPos = string::npos;
                    }
                    return _mMatch;
                }
                if (sCmd[i+sCommand.length()] == '(' && sCmd.find(sCommand, i+1) != string::npos)
                    continue;
                else if (sCmd[i+sCommand.length()] == '(' && sCmd.find(sCommand, i+1) == string::npos)
                {
                    _mMatch.sString = "";
                    _mMatch.nPos = string::npos;
                    return _mMatch;
                }
                if (sCmd.find('-', i+sCommand.length()) != string::npos)
                {
                    _mMatch.sString = sCmd.substr(i, sCmd.find('-', i+sCommand.length())-i);
                    _mMatch.nPos = i;
                    if (_mMatch.sString.find(' ') != string::npos)
                        _mMatch.sString.erase(_mMatch.sString.find(' '));
                    if (checkDelimiter(sCmd.substr(i-1, _mMatch.sString.length()+2)) && !isInQuotes(sCmd, i))
                    {
                        if (sCmd.length() >= sCommand.length()+_mMatch.nPos+_mMatch.sString.length())
                        {
                            Match _mTemp = findCommand(sCmd.substr(_mMatch.nPos+_mMatch.sString.length()), sCommand);
                            if (_mTemp.sString.length()
                                && _mTemp.sString.length() < _mMatch.sString.length())
                            {
                                _mMatch.nPos += _mTemp.nPos + _mMatch.sString.length();
                                _mMatch.sString = _mTemp.sString;
                            }
                        }
                        return _mMatch;
                    }
                    else
                    {
                        _mMatch.sString = "";
                        _mMatch.nPos = string::npos;
                        continue;
                    }
                }
                if (sCmd.find(' ', i+sCommand.length()) != string::npos)
                {
                    _mMatch.sString = sCmd.substr(i, sCmd.find(' ', i+sCommand.length())-i);
                    _mMatch.nPos = i;
                    if (checkDelimiter(sCmd.substr(i-1, _mMatch.sString.length()+2)) && !isInQuotes(sCmd, i))
                    {
                        if (sCmd.length() >= sCommand.length()+_mMatch.nPos+_mMatch.sString.length())
                        {
                            Match _mTemp = findCommand(sCmd.substr(_mMatch.nPos+_mMatch.sString.length()), sCommand);
                            if (_mTemp.sString.length()
                                && _mTemp.sString.length() < _mMatch.sString.length())
                            {
                                _mMatch.nPos += _mTemp.nPos + _mMatch.sString.length();
                                _mMatch.sString = _mTemp.sString;
                            }
                        }
                        return _mMatch;
                    }
                    else
                    {
                        _mMatch.sString = "";
                        _mMatch.nPos = string::npos;
                        continue;
                    }
                }
            }
            if (sCmd.substr(i,sCommand.length()) == sCommand && !i)
            {
                if (sCommand.length() == sCmd.length()-1)
                {
                    _mMatch.sString = " " + sCommand.substr(0, sCommand.length()+1);
                    _mMatch.nPos = 0;
                    if (checkDelimiter(_mMatch.sString) && !isInQuotes(sCmd, i))
                    {
                        _mMatch.sString = _mMatch.sString.substr(1,_mMatch.sString.length()-2);
                    }
                    else
                    {
                        _mMatch.sString = "";
                        _mMatch.nPos = string::npos;
                        continue;
                    }
                    return _mMatch;
                }
                if (sCmd[i+sCommand.length()] == '(' && sCmd.find(sCommand, i+1) != string::npos)
                    continue;
                else if (sCmd[i+sCommand.length()] == '(' && sCmd.find(sCommand, i+1) == string::npos)
                {
                    _mMatch.sString = "";
                    _mMatch.nPos = string::npos;
                    return _mMatch;
                }
                if (sCmd.find('-', sCommand.length()) != string::npos)
                {
                    _mMatch.sString = sCmd.substr(0, sCmd.find('-', sCommand.length()));
                    _mMatch.nPos = 0;
                    if (_mMatch.sString.find(' ') != string::npos)
                        _mMatch.sString.erase(_mMatch.sString.find(' '));
                    if (sCmd.length() >= sCommand.length()+_mMatch.nPos+_mMatch.sString.length())
                    {
                        Match _mTemp = findCommand(sCmd.substr(_mMatch.nPos+_mMatch.sString.length()), sCommand);
                        if (_mTemp.sString.length()
                            && _mTemp.sString.length() < _mMatch.sString.length())
                        {
                            _mMatch.nPos += _mTemp.nPos + _mMatch.sString.length();
                            _mMatch.sString = _mTemp.sString;
                        }
                    }
                    return _mMatch;
                }
                if (sCmd.find(' ', sCommand.length()) != string::npos)
                {
                    _mMatch.sString = sCmd.substr(0, sCmd.find(' ', sCommand.length()));
                    _mMatch.nPos = 0;
                    if (sCmd.length() >= sCommand.length()+_mMatch.nPos+_mMatch.sString.length())
                    {
                        Match _mTemp = findCommand(sCmd.substr(_mMatch.nPos+_mMatch.sString.length()), sCommand);
                        if (_mTemp.sString.length()
                            && _mTemp.sString.length() < _mMatch.sString.length())
                        {
                            _mMatch.nPos += _mTemp.nPos + _mMatch.sString.length();
                            _mMatch.sString = _mTemp.sString;
                        }
                    }
                    return _mMatch;
                }
            }
        }
    }
    else
        return _mMatch;
    return _mMatch;
}

// --> extrahiert den gesamten Kommandostring aus einer Kommandozeile <--
string extractCommandString(const string& sCmd, const Match& _mMatch)
{
    string sCommandString = "";
    if (_mMatch.nPos == string::npos)
        return "";

    if (_mMatch.nPos)
    {
        for (unsigned int i = _mMatch.nPos; i >= 0; i--)
        {
            if (sCmd[i] == '(' && !isInQuotes(sCmd, i))
            {
                if (getMatchingParenthesis(sCmd.substr(i)) != string::npos)
                {
                    sCommandString = sCmd.substr(_mMatch.nPos,getMatchingParenthesis(sCmd.substr(i))-(_mMatch.nPos-i+1));
                    break;
                }
                else
                    throw UNMATCHED_PARENTHESIS;
            }
            if (!i)
                break;
        }
    }
    if (!sCommandString.length())
        sCommandString = sCmd.substr(_mMatch.nPos);
    return sCommandString;
}

// --> Entfernt ueberzaehlige Kommata in strings <--
void removeArgSep(string& sToClear)
{
    int nSep = 0;
    int nSpaces = -1;

    // --> So lange nSep nicht mit 0 aus dem Schleifendurchlauf herauskommt, wiederhole ihn <--
    do
    {
        // --> nSep und nSpaces zuruecksetzen <--
        nSep = 0;
        nSpaces = -1;

        // --> Jedes Zeichen des Strings ueberpruefen <--
        for (unsigned int i = 0; i < sToClear.length(); i++)
        {
            // --> Gefuehlt unendlich verschiedene Moeglichkeiten <--
            if (sToClear[i] == ',' && !nSep && !nSpaces)
                nSep++;
            else if (sToClear[i] == ',' && !nSep && nSpaces)
                sToClear[i] = ' ';
            else if (sToClear[i] == ',' && nSep)
            {
                sToClear[i] = ' ';
            }
            else if (sToClear[i] != ' ' && nSep)
                nSep--;
            else if (sToClear[i] == ' ' && nSpaces == -1)
                nSpaces = 1;
            else if (sToClear[i] == ' ' && nSpaces)
                nSpaces++;
            else if (sToClear[i] != ' ' && nSpaces)
                nSpaces = 0;
        }

        // --> Ist nSep ungleich 0? <--
        if (nSep)
        {
            // --> Ersetze das letzte ',' durch eine Leerstelle <--
            sToClear[sToClear.rfind(',')] = ' ';
        }

        // --> Umschliessende Leerzeichen entfernen <--
        StripSpaces(sToClear);
    }
    while (nSep);
    return;
}

// --> Eine Datei mit einem externen Programm oeffnen <--
void openExternally(const string& sFile, const string& sProgramm, const string& sPathToFile)
{
    /* --> Dies simuliert im Wesentlichen einen cd zur Datei, den Aufruf mit dem anderen
     *     Programm und die Rueckkehr zum alten Pfad (NumeRe-Stammverzeichnis) <--
     */
    char* cFile = 0;
    if (sFile[0] != '"')
        cFile = (char*)string("\"" + sFile + "\"").c_str();
    else
        cFile = (char*)sFile.c_str();
    int nErrorCode = 0;
    for (unsigned int i = 0; i < sFile.length(); i++)
    {
        if (cFile[i] == '\0')
            break;
        if (cFile[i] == '/')
            cFile[i] = '\\';
    }
    nErrorCode = (int)ShellExecute(NULL, "open", sProgramm.c_str(), cFile, NULL, SW_SHOWNORMAL);
    if (nErrorCode <= 32)
    {
        if (nErrorCode == ERROR_FILE_NOT_FOUND || nErrorCode == SE_ERR_FNF)
        {
            sErrorToken = sProgramm;
            throw EXTERNAL_PROGRAM_NOT_FOUND;
        }
        else
        {
            sErrorToken = sFile;
            throw CANNOT_READ_FILE;
        }

    }
    return;
}

// --> Eine Datei von einem Ort zum anderen Ort verschieben; kann auch zum umbenennen verwendet werden <--
void moveFile(const string& sFile, const string& sNewFileName)
{
    // --> Dateien verschieben geht am einfachsten, wenn man ihren Inhalt in die Zieldatei kopiert <--
    ifstream File(sFile.c_str(), ios_base::binary);
    ofstream NewFile(sNewFileName.c_str(), ios_base::binary);

    if (!File.good())
        throw CANNOT_OPEN_SOURCE;
    if (!NewFile.good())
        throw CANNOT_OPEN_TARGET;

    // --> Schreibe den ReadBuffer in NewFile <--
    NewFile << File.rdbuf();

    // --> Schliesse NewFile und File <--
    NewFile.close();
    File.close();

    // --> Loesche die alte Datei <--
    remove(sFile.c_str());
    return;
}

// --> Generiert eine TeX-Hauptdatei fuer eine gegebene TikZ-Plot-Datei <--
void writeTeXMain(const string& sTeXFile)
{
    string sTemp = sTeXFile;
    if (sTemp.find('\\') != string::npos || sTemp.find('/') != string::npos)
    {
        if (sTemp.find('\\') != string::npos)
            sTemp = sTemp.substr(sTemp.rfind('\\')+1);
        if (sTemp.find('/') != string::npos)
            sTemp = sTemp.substr(sTemp.rfind('/')+1);
    }

    // --> Fuege vor ".tex" den String "main"  ein <--
    ofstream TexMain((sTeXFile.substr(0,sTeXFile.find(".tex"))+"main.tex").c_str());
    if (!TexMain.good())
        throw CANNOT_OPEN_TARGET;
    TexMain << "\\documentclass{scrartcl}    % KOMA-SCRIPT-KLASSE (Kann durch \"article\" ersetzt werden)" << endl << endl;
    TexMain << "% Ein paar hilfreiche Packages:" << endl;
    TexMain << "\\usepackage[utf8]{inputenc} % Sonderzeichen in der Eingabe" << endl;
    TexMain << "\\usepackage[T1]{fontenc}    % Sonderzeichen in der Ausgabe" << endl;
    TexMain << "\\usepackage[ngerman]{babel} % Deutsch als Dokumentsprache" << endl;
    TexMain << "\\usepackage{mathpazo}       % Palatino als Schriftart (passend zum Plot)" << endl;
    TexMain << "\\usepackage{tikz}           % TikZ fuer die eigentliche Graphik" << endl;
    /*TexMain << "\\input{";
    TexMain << sTemp.substr(0,sTemp.find(".tex"));
    TexMain << "colors.tex} % Farb-Definitionen" << endl << endl;*/
    TexMain << "% Eigentliches Dokument" << endl;
    TexMain << "\\begin{document}" << endl;
    TexMain << "\t% Hier bietet sich ein beschreibender Text an" << endl;
    TexMain << "\t\\input{";
    TexMain << sTemp;
    TexMain << "} % Einbinden des Plots" << endl;
    TexMain << "\t% Mit derselben Syntax wie der vorherigen Zeile koennen auch weitere Plots in diese TeX-Ausgabe eingebunden werden" << endl;
    TexMain << "\\end{document}" << endl;
    TexMain << "% EOF" << endl;
    TexMain.close();

    /* --> Die Datei "mglmain.tex" wird bei einem TeX-Export automatisch erstellt (und entspricht in etwa dieser Datei),
     *     allerdings wird diese Datei jedes Mal bei einem TeX-Export ueberschrieben. Daher schreiben wir unsere eigene
     *     und loeschen diese automatische Datei <--
     */
    remove("mglmain.tex");
    return;
}

// --> Entfernt Kommando-Tokens (\n, \t, \") aus strings <--
string removeControlSymbols(const string& sString)
{
    string sReturn = sString;
    //unsigned int nPos = 0;

    // --> Kein '\' zu finden? Sofort zurueck <--
    if (sReturn.find('\\') == string::npos)
        return sReturn;

    /*for (unsigned int i = 0; i < sReturn.length(); i++)
    {
        if (sReturn[i] == '\\' && sReturn.substr(i,2) != "\\n")
            sReturn.erase(i,1);
    }*/
    // --> So lange '\' zu finden ist <--
    /*while (sReturn.find('\\', nPos) != string::npos)
    {
        // --> Speichere die Position des '\' <--
        nPos = sReturn.find('\\', nPos);

        // --> Haben wir das letzte Zeichen erwischt ? <--
        if (nPos+1 == sReturn.length())
        {
            sReturn = sReturn.substr(0,sReturn.length()-1);
            break;
        }

        // --> Wenn auf '\' ein '"' folgt, entferne nur '\', sonst entferne beide Zeichen <--
        if (sReturn[nPos+1] == '"')
            sReturn = sReturn.substr(0,nPos) + sReturn.substr(nPos+1);
        else if (sReturn[nPos+1] == 'n' || sReturn[nPos+1] == 't')
            sReturn = sReturn.substr(0,nPos) + sReturn.substr(nPos+2);
        nPos++;
    }*/
    return sReturn;
}

// --> Ergaenzt \ vor allen Anfuehrungszeichen in einem string <--
string addControlSymbols(const string& sString)
{
    string sReturn = sString;
    unsigned int nPos = 0;

    // --> Keine '"' zu finden? <--
    if (sReturn.find('"') == string::npos)
        return sReturn;

    // --> So lange '"' zu finden ist <--
    while (sReturn.find('"', nPos) != string::npos)
    {
        // --> Speichere die Position des '"' <--
        nPos = sReturn.find('"', nPos);

        // --> Fuege '\' an dieser Stelle ein <--
        if (sReturn[nPos-1] != '\\')
            sReturn.insert(nPos, 1, '\\');
        else
        {
            nPos++;
            continue;
        }

        // --> Schiebe den Positionsindex um 2 weiter (sonst wird das eben gefundene '"' gleich nochmal gefunden) <--
        nPos += 2;

        // --> String-Ende abfangen <--
        if (nPos == sReturn.length())
            break;
    }
    return sReturn;
}

// --> Extrahiert ein Optionswert an der Stelle nPos <--
string getArgAtPos(const string& sCmd, unsigned int nPos)
{
    string sArgument = "";
    //cerr << sCmd << endl;
    //cerr << nPos << endl;
    if (nPos >= sCmd.length())
        return "";
    while (nPos < sCmd.length() && sCmd[nPos] == ' ')
        nPos++;
    if (nPos >= sCmd.length())
        return "";
    if (sCmd[nPos] == '"')
    {
        for (unsigned int i = nPos+1; i < sCmd.length(); i++)
        {
            if (sCmd[i] == '"' && sCmd[i-1] != '\\')
            {
                sArgument = sCmd.substr(nPos+1, i - nPos-1);
                break;
            }
        }
    }
    else
    {
        for (unsigned int i = nPos; i < sCmd.length(); i++)
        {
            if (sCmd[i] == '(' || sCmd[i] == '[')
                i += getMatchingParenthesis(sCmd.substr(i));
            if (sCmd[i] == ' ')
            {
                sArgument = sCmd.substr(nPos, i-nPos);
                StripSpaces(sArgument);
                break;
            }
        }
        if (!sArgument.length())
        {
            sArgument = sCmd.substr(nPos);
            StripSpaces(sArgument);
        }
    }
    return sArgument;
}

// --> Pruefen wir, ob die Position in dem String von Anfuehrungszeichen umgeben ist <--
bool isInQuotes(const string& sExpr, unsigned int nPos, bool bIgnoreVarParser)
{
    //cerr << sExpr << endl;
    int nQuotes = 0;
    if ((sExpr.find('"') == string::npos && sExpr.find('#') == string::npos && sExpr.find("string_cast(") == string::npos) || nPos+1 >= sExpr.length())
        return false;

    // --> Zaehlt schlicht und einfach die Anfuehrungszeichen <--
    for (unsigned int i = 0; i < nPos; i++)
    {
        if (sExpr.substr(i,12) == "string_cast(" && i+12 <= nPos)
        {
            if (getMatchingParenthesis(sExpr.substr(i+11))+i+11 > nPos)
                return true;
            else
                i += getMatchingParenthesis(sExpr.substr(i+11))+11;
        }
        if (sExpr[i] == '"' && nQuotes)
        {
            if (i && sExpr[i-1] == '\\')
                continue;
            nQuotes = 0;
        }
        else if (sExpr[i] == '"')
        {
            if (i && sExpr[i-1] == '\\')
                continue;
            nQuotes = 1;
        }
    }
    //cerr << "nQuotes = " << nQuotes << endl;
    if (nQuotes)
        return true;
    else if (!bIgnoreVarParser)
    {
        if (sExpr.rfind('#', nPos) == string::npos)
            return false;
        else
        {
            if (isInQuotes(sExpr, sExpr.rfind('#', nPos), true))
                return false;
            for (unsigned int i = sExpr.rfind('#', nPos); i < nPos; i++)
            {
                //cerr << i << endl;
                if (sExpr[i] == '(')
                {
                    if (getMatchingParenthesis(sExpr.substr(i, nPos-i)) == string::npos)
                        return true;
                    i += getMatchingParenthesis(sExpr.substr(i, nPos-i));
                    if (i == nPos)
                        return true;
                    continue;
                }
                if (sExpr[i] == ' ' || sExpr[i] == '+' || sExpr[i] == ',' || sExpr[i] == ')')
                    return false;
            }
            if (nPos < sExpr.length()-1 && (sExpr[nPos] == ',' || sExpr[nPos] == '+' || sExpr[nPos] == ' ' || sExpr[nPos] == ')'))
                return false;
            else if (nPos == sExpr.length()-1 && sExpr[nPos] == ')')
                return false;
            else
                return true;
        }
    }
    else
        return false;
}

bool isToStringArg(const string& sExpr, unsigned int nPos)
{
    //cerr << sExpr << endl;
    //cerr << nPos << endl;
    if (sExpr.find("valtostr(") == string::npos && sExpr.find("to_string(") == string::npos && sExpr.find("string_cast(") == string::npos)
        return false;
    for (int i = nPos; i >= 8; i--)
    {
        if (sExpr[i] == '(' && getMatchingParenthesis(sExpr.substr(i))+i > nPos)
        {
            //cerr << "i=" << i << endl;
            if (i > 10 && sExpr.substr(i-11,12) == "string_cast(")
                return true;
            else if (i > 8 && sExpr.substr(i-9,10) == "to_string(")
                return true;
            else if (sExpr.substr(i-8,9) == "valtostr(")
            {
                //cerr << "valtostr(" << endl;
                return true;
            }
            else if (isDelimiter(sExpr[i-1]))
                continue;
            else
                return false;
        }
    }
    return false;
}


bool isDelimiter(char cChar)
{
    string sDelimiter = "+-*/ ^&|!%<>,=";
    if (sDelimiter.find(cChar) != string::npos)
        return true;
    return false;
}
// --> Ergaenzt fehlende Legenden mit den gegebenen Ausdruecken <--
bool addLegends(string& sExpr)
{
    unsigned int nPos = 0;
    unsigned int nPos_2 = 0;
    unsigned int nPos_3 = 0;
    int nQMark = 0;
    string sTemp = "";
    string sLabel = "";

    int nParenthesis = 0;
    //cerr << "sExpr = " << sExpr << endl;
    for (unsigned int i = 0; i < sExpr.length(); i++)
    {
        if (sExpr[i] == '(' && !isInQuotes(sExpr, i, true))
            nParenthesis++;
        if (sExpr[i] == ')' && !isInQuotes(sExpr, i, true))
            nParenthesis--;
    }
    if (nParenthesis)
    {
        throw UNMATCHED_PARENTHESIS;
        /*cerr << "|-> FEHLER: Klammern im Ausdruck passen nicht zusammen!" << endl;
        return false;*/
    }

    // --> Ergaenze am Anfang und am Ende von sExpr je ein Anfuehrungszeichen <--
    sExpr = " " + sExpr + " ";
    //cerr << "sExpr = " << sExpr << endl;

    // --> Mach' so lange, wie ein ',' ab dem Positions-Index nPos gefunden wird <--
    do
    {
        // --> Speichere die Position des ',' in nPos_2 <--
        //nPos_2 = sExpr.find(',', nPos);
        //nPos_3 = nPos;
        //cerr << "bool " << isInQuotes(sExpr, nPos_2) << endl;
        //cerr << "nPos = " << nPos << ", nPos_2 = " << nPos_2 << ", nPos_3 = " << nPos_3 << endl;
        /* --> So lange nPos_2 != -1 und keine Klammer zwischen nPos_3 und nPos_2 aufgeht oder
         *     nPos_2 nicht zwischen zwei Anfuehrungszeichen steht <--
         */
        for (unsigned int i = nPos; i < sExpr.length(); i++)
        {
            if (sExpr[i] == '(' || sExpr[i] == '{')
                i += getMatchingParenthesis(sExpr.substr(i));
            else if (isInQuotes(sExpr, i))
                continue;
            else if (sExpr[i] == ',')
            {
                nPos_2 = i;
                break;
            }
            else if (i == sExpr.length()-1)
                nPos_2 = i;
        }
        //cerr << "nPos = " << nPos << ", nPos_2 = " << nPos_2 << ", nPos_3 = " << nPos_3 << endl;

        // --> Abfangen, dass nPos_2 == -1 sein koennte <--
        if (nPos_2 == string::npos)
            nPos_2 = sExpr.length();

        /* --> Nun koennte es sein, dass bereits eine Legende angegeben worden ist. Dabei gibt es drei
         *     Moeglichkeiten: entweder durch umschliessende Anfuehrungszeichen, durch eine vorangestellte
         *     Raute '#' oder auch beides. Wir muessen hier diese drei Faelle einzeln behandeln <--
         * --> Ebenfalls ist es natuerlich moeglich, dass gar keine Legende angegeben worden ist. Das behandeln
         *     wir im ELSE-Fall <--
         */
        if (sExpr.substr(nPos, nPos_2-nPos).find('"') != string::npos)
        {
            /* --> Hier ist auf jeden Fall '"' vorhanden. Es ist aber nicht gesagt, dass '#' nicht auch
             *     zu finden ist <--
             * --> Speichern wir zunaechst die Position des '"' in nQMark <--
             */
            nQMark = sExpr.substr(nPos, nPos_2-nPos).find('"')+1;

            // --> Pruefe nun, ob in diesem Stringbereich ein zweites '"' zu finden ist <--
            if (sExpr.substr(nPos, nPos_2-nPos).find('"', nQMark) < (unsigned)(nPos_2-nPos))
            {
                // --> Ja? Gibt's denn dann eine Raute? <--
                if (sExpr.substr(nPos, nPos_2-nPos).find('#', nQMark) == string::npos)
                    nPos = nPos_2+1; // Nein? Alles Prima, Positionsindex auf nPos_2+1 setzen
                else
                {
                    // --> Ja? Dann muessen wir (zur Vereinfachung an anderer Stelle) noch zwei Anfuehrungszeichen ergaenzen <--
                    sExpr = sExpr.substr(0,nPos_2) + "+\"\"" + sExpr.substr(nPos_2);
                    nPos = nPos_2 + 4;
                }
                // --> In jedem Fall kann die Funktion mit der naechsten Schleife fortfahren <--
                continue;
            }
            else
                return false;   // Nein? Dann ist irgendwas ganz Falsch: FALSE zurueckgeben!
        }
        else if (sExpr.substr(nPos, nPos_2-nPos).find('#') != string::npos)
        {
            /* --> Hier gibt's nur '#' und keine '"' (werden im ersten Fall schon gefangen). Speichern wir
             *     die Position der Raute in nPos_3 <--
             */
            nPos_3 = sExpr.substr(nPos, nPos_2-nPos).find('#')+nPos;

            /* --> Setze sExpr dann aus dem Teil vor nPos_3 und, wenn noch mindestens Komma ab nPos_3 gefunden werden kann,
             *     dem Teil ab nPos_3 vor dem Komma, dem String '+""' und dem Teil ab dem Komma zusammen, oder, wenn kein
             *     Komma gefunden werden kann, dem Teil nach nPos_3 und dem String '+""' zusammen <--
             * --> An dieser Stelle bietet sich der Ternary (A ? x : y) tatsaechlich einmal an, da er die ganze Sache,
             *     die sonst eine temporaere Variable benoetigt haette, in einem Befehl erledigen kann <--
             */
            for (unsigned int i = nPos_3; i < sExpr.length(); i++)
            {
                if (sExpr[i] == '(')
                    i += getMatchingParenthesis(sExpr.substr(i));
                if (sExpr[i] == ' ' || sExpr[i] == ',')
                {
                    sExpr = sExpr.substr(0, i) + "+\"\"" +sExpr.substr(i);
                    break;
                }
            }
            // --> Speichere die Position des naechsten ',' in nPos und fahre mit der naechsten Schleife fort <--
            nPos = sExpr.find(',', nPos_3)+1;
            continue;
        }
        else
        {
            /* --> Hier gibt's weder '"' noch '#'; d.h., wir muessen die Legende selbst ergaenzen <--
             * --> Schneiden wir zunaechst den gesamten Ausdurck zwischen den zwei Kommata heraus <--
             */
            sLabel = sExpr.substr(nPos, nPos_2-nPos);

            // --> Entfernen wir ueberzaehlige Leerzeichen <--
            StripSpaces(sLabel);

            /* --> Setzen wir den gesamten Ausdruck wieder zusammen, wobei wir den Ausdruck in
             *     Anfuehrungszeichen als Legende einschieben <--
             */
            sTemp = sExpr.substr(0, nPos_2) + " \"" + sLabel + "\"";

            // --> Schiebe den Positionsindex um die Laenge des temporaeren Strings weiter <--
            nPos = sTemp.length()+1;

            // --> Falls nPos_2 != -1 ist, haenge den restlichen String an, sonst nicht <--
            if (nPos_2 != string::npos)
                sExpr = sTemp + sExpr.substr(nPos_2);
            else
                sExpr = sTemp;
        }
    }
    while (sExpr.find(',', nPos-1) != string::npos);

    // --> Hat alles geklappt: TRUE zurueck geben <--
    return true;
}

// --> Prueft, ob der erste und der letzte Char eines strings zu den Delimitern gehoert: z.B. zur Variablen-/Tokendetektion <--
bool checkDelimiter(const string& sString)
{
    bool isDelimitedLeft = false;
    bool isDelimitedRight = false;
    string sDelimiter = "+-*/ ()={}^&|!<>,\\%#~[]?:\";";

    // --> Versuche jeden Delimiter, der dir bekannt ist und setze bei einem Treffer den entsprechenden BOOL auf TRUE <--
    for (unsigned int i = 0; i < sDelimiter.length(); i++)
    {
        if (sDelimiter[i] == sString[0])
            isDelimitedLeft = true;
        if (sDelimiter[i] == sString[sString.length()-1])
            isDelimitedRight = true;
    }

    // --> Gib die Auswertung dieses logischen Ausdrucks zurueck <--
    return (isDelimitedLeft && isDelimitedRight);
}

// --> Funktion, die die Laenge der Zeile anhand der festgelegten Fensterdimensionen bestimmt und die Zeilenumbrueche automatisch erzeugt <--
string LineBreak(string sOutput, const Settings& _option, bool bAllowDashBreaks, int nFirstIndent, int nIndent)
{
    unsigned int nLastLineBreak = 0;     // Variable zum Speichern der Position des letzten Zeilenumbruchs
    string sIndent = "\n|";     // String fuer den Einzug der 2. und der folgenden Zeilen
    sOutput = toSystemCodePage(sOutput);
    // --> Falls der string kuerzer als die Zeilenlaenge ist, braucht nichts getan zu werden <--
    if (sOutput.length() < _option.getWindow() - nFirstIndent && sOutput.find('$') == string::npos && sOutput.find("\\n") == string::npos)
        return sOutput;

    // --> Ergaenze den Einzug um die noetige Zahl an Leerstellen <--
    for (int i = 1; i < nIndent; i++)
    {
        sIndent += " ";
    }

    // --> Laufe alle Zeichen des strings ab <--
    for (unsigned int i = 1; i < sOutput.length(); i++)
    {
        /* --> Stolpere ich ueber ein "$"? Dann muss hier ein Zeilenumbruch hin. Damit muss der Index des
         *     letzten Zeilenumbruchs aktualisiert werden <--
         */
        if (sOutput[i] == '$' && sOutput[i-1] != '\\')
            nLastLineBreak = i;
        if (sOutput[i] == 'n' && sOutput[i-1] == '\\')
        {
            if (i == 1 || sOutput[i-2] != '\\')
                nLastLineBreak = i;
        }
        // --> Ist die maximale Zeilenlaenge erreicht? Dann muss ein Zeilenumbruch eingefuegt werden <--
        if ((i == _option.getWindow() - nFirstIndent && !nLastLineBreak)
            || (nLastLineBreak && i-nLastLineBreak == _option.getWindow() - nIndent))
        {
            // --> Laufe von hier ab rueckwaerts und suche nach entweder: 1 Leerstelle oder 1 Minus-Zeichen (wenn erlaubt) oder dem "$" <--
            for (unsigned int j = i; j > nLastLineBreak; j--)
            {
                if (sOutput[j] == ' ')
                {
                    if (sOutput[j-1] == '\\')
                    {
                        sOutput.insert(j+1, "$");
                        nLastLineBreak = j+1;
                    }
                    else
                    {
                        sOutput[j] = '$';   // Leerzeichen durch "$" ersetzen
                        nLastLineBreak = j;
                    }
                    break;
                }
                else if (sOutput[j] == '-' && bAllowDashBreaks && j != i)
                {
                    // --> Minuszeichen: nicht immer ist das Trennen an dieser Stelle sinnvoll. Wir pruefen die einfachsten Faelle <--
                    if (j &&
                        (sOutput[j-1] == ' '
                        || sOutput[j-1] == '('
                        || sOutput[j+1] == ')'
                        || sOutput[j-1] == '['
                        || (sOutput[j+1] >= '0' && sOutput[j+1] <= '9')
                        || sOutput[j+1] == ','
                        || (sOutput[j+1] == '"' && sOutput[j-1] == '"')
                        ))
                        continue;
                    sOutput[j] = '~';   // '-' durch '~' ersetzen
                    nLastLineBreak = j+1;
                    break;
                }
                else if (sOutput[j] == ',' && bAllowDashBreaks && sOutput[j+1] != ' ' && j != i)
                {
                    sOutput[j] = '%';
                    nLastLineBreak = j+1;
                    break;
                }
                else if (sOutput[j] == '$' && sOutput[j-1] != '\\') // --> Hier ist auf jeden Fall ein Zeilenumbruch gewuenscht <--
                {
                    nLastLineBreak = j;
                    break;
                }
                if (j-1 == nLastLineBreak)
                {
                    string sDelim = "+-*/";
                    for (unsigned int n = i; n > nLastLineBreak; n--)
                    {
                        if (sDelim.find(sOutput[n]) != string::npos)
                        {
                            sOutput = sOutput.substr(0,n) + '$' + sOutput.substr(n);
                            nLastLineBreak = n;
                            break;
                        }
                        if (n-1 == nLastLineBreak)
                        {
                            sOutput = sOutput.substr(0,i-1) + '$' + sOutput.substr(i-1);
                            nLastLineBreak = i;
                        }
                    }
                }
            }
        }
    }
    // --> Laufe jetzt nochmals den gesamten String ab <--
    for (unsigned int i = 0; i < sOutput.length(); i++)
    {
        if (sOutput[i] == '$' && sOutput[i-1] != '\\')
        {
            // --> Ersetze '$' durch den oben erstellten Einzug <--
            sOutput = sOutput.substr(0,i) + sIndent + sOutput.substr(i+1);
        }
        else if (sOutput[i] == 'n' && sOutput[i-1] == '\\' && sOutput[i-2] != '\\')
            sOutput = sOutput.substr(0,i-1) + sIndent + sOutput.substr(i+1);
        else if (sOutput[i] == 'n' && sOutput[i-1] == '\\' && sOutput[i-2] == '\\')
            sOutput.erase(i-1,1);
        else if (sOutput[i] == '$' && sOutput[i-1] == '\\')
        {
            sOutput.erase(i-1,1);
        }
        else if (sOutput[i] == '~' && bAllowDashBreaks)
        {
            if (sOutput[i-1] == '"' && sOutput[i+1] == '"')
                continue;
            // --> Ersetze '~' durch '-' und den oben erstellten Einzug, falls erlaubt <--
            sOutput = sOutput.substr(0,i) + "-" + sIndent + sOutput.substr(i+1);
        }
        else if (sOutput[i] == '%' && bAllowDashBreaks)
        {
            // --> Ersetze '%' durch ',' und den oben erstellten Einzug, falls erlaubt <--
            sOutput = sOutput.substr(0,i) + "," + sIndent + sOutput.substr(i+1);
        }
    }
    return sOutput;
}

// --> Funktion, die die Groesse der WinConsole aendert <--
bool ResizeConsole(const Settings& _option)
{
    COORD cCoord;
    SMALL_RECT srRect;
    bool bBufferError = false;
    bool bWindowError = false;

    // --> Anpassen der Fenstergroesse: Windows-Funktion <--
    cCoord.X = _option.getBuffer();
    cCoord.Y = _option.getBuffer(1);
    srRect.Left = 0;
    srRect.Top = 0;
    srRect.Right = _option.getWindow();
    srRect.Bottom = _option.getWindow(1);

    if (!SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), cCoord))
        bBufferError = true;
    if (!SetConsoleWindowInfo(GetStdHandle(STD_OUTPUT_HANDLE), TRUE, &srRect))
    {
        cerr << "ERROR TRYING TO RESIZE CONSOLE WINDOW!" << endl;
        bWindowError = true;
    }
    if (bBufferError && !bWindowError)
    {
        if (!SetConsoleScreenBufferSize(GetStdHandle(STD_OUTPUT_HANDLE), cCoord))
        {
            cerr << "ERROR TRYING TO RESIZE SCREEN BUFFER!" << endl;
            bBufferError = true;
        }
        else
            bBufferError = false;
    }

    if (bWindowError || bBufferError)
    {
        Sleep(1000);
        return false;
    }
    else
        return true;
}

// --> Funktion, die das Farbthema der WinConsole aendert <--
bool ColorTheme(const Settings& _option)
{
    switch (_option.getColorTheme())
    {
        case 1: // BIOS
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | BACKGROUND_BLUE | BACKGROUND_INTENSITY);
            system("cls");
            return true;
        case 2: // Freaky
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                FOREGROUND_GREEN | FOREGROUND_INTENSITY | BACKGROUND_BLUE | BACKGROUND_RED);
            system("cls");
            return true;
        case 3: // Classic Black
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
            system("cls");
            return true;
        case 4: // Classic Green
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                FOREGROUND_GREEN | FOREGROUND_INTENSITY);
            system("cls");
            return true;

        default: // NumeRe
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE),
                FOREGROUND_BLUE | BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY);
            system("cls");
            return true;
    }
    return false;
}

// --> Linearisiert die Funktion zwischen zwei Punkten (x_0,y_0) und (x_1,y_1) und gibt den Schnittpunkt mit der x-Achse zurueck <--
double Linearize(double x_0, double y_0, double x_1, double y_1)
{
    double b = y_0;
    double m = (y_1-y_0)/(x_1-x_0);
    // y = m*x + b ==> x = 1/m*(y-b) ==> x = -b/m fuer y = 0.0
    return x_0 - b / m;
}

// --> Wandelt einen Literalen-String in einen Ausgabe-String um <--
string toSystemCodePage(string sOutput)
{
    for (unsigned int i = 0; i < sOutput.length(); i++)
    {
        if (sOutput[i] == 'Ä')
            sOutput[i] = (char)142;
        else if (sOutput[i] == 'ä')
            sOutput[i] = (char)132;
        else if (sOutput[i] == 'Ö')
            sOutput[i] = (char)153;
        else if (sOutput[i] == 'ö')
            sOutput[i] = (char)148;
        else if (sOutput[i] == 'Ü')
            sOutput[i] = (char)154;
        else if (sOutput[i] == 'ü')
            sOutput[i] = (char)129;
        else if (sOutput[i] == 'ß')
            sOutput[i] = (char)225;
        else if (sOutput[i] == '°')
            sOutput[i] = (char)248;
        else if (sOutput[i] == (char)249)
            sOutput[i] = (char)196;
        else if (sOutput[i] == (char)171)
            sOutput[i] = (char)174;
        else if (sOutput[i] == (char)187)
            sOutput[i] = (char)175;
        else
            continue;
    }
    return sOutput;
}

string fromSystemCodePage(string sOutput)
{
    for (unsigned int i = 0; i < sOutput.length(); i++)
    {
        if (sOutput[i] == (char)142)
            sOutput[i] = 'Ä';
        else if (sOutput[i] == (char)132)
            sOutput[i] = 'ä';
        else if (sOutput[i] == (char)153)
            sOutput[i] = 'Ö';
        else if (sOutput[i] == (char)148)
            sOutput[i] = 'ö';
        else if (sOutput[i] == (char)154)
            sOutput[i] = 'Ü';
        else if (sOutput[i] == (char)129)
            sOutput[i] = 'ü';
        else if (sOutput[i] == (char)225)
            sOutput[i] = 'ß';
        else if (sOutput[i] == (char)248)
            sOutput[i] = '°';
        else if (sOutput[i] == (char)174)
            sOutput[i] = (char)171;
        else if (sOutput[i] == (char)175)
            sOutput[i] = (char)187;
        else
            continue;
    }
    return sOutput;
}

string utf8parser(const string& sString)
{
    string sReturn = sString;
    if (sReturn.length() < 2)
        return sReturn;
    for (unsigned int i = 0; i < sReturn.length()-1; i++)
    {
        if (sReturn[i] == (char)195)
        {
            if (sReturn[i+1] == (char)132)//Ä
            {
                sReturn.erase(i,1);
                sReturn[i] = (char)142;
            }
            else if (sReturn[i+1] == (char)164)//ä
            {
                sReturn.erase(i,1);
                sReturn[i] = (char)132;
            }
            else if (sReturn[i+1] == (char)150)//Ö
            {
                sReturn.erase(i,1);
                sReturn[i] = (char)153;
            }
            else if (sReturn[i+1] == (char)182)//ö
            {
                sReturn.erase(i,1);
                sReturn[i] = (char)148;
            }
            else if (sReturn[i+1] == (char)156)//Ü
            {
                sReturn.erase(i,1);
                sReturn[i] = (char)154;
            }
            else if (sReturn[i+1] == (char)188)//ü
            {
                sReturn.erase(i,1);
                sReturn[i] = (char)129;
            }
            else if (sReturn[i+1] == (char)159)//ß
            {
                sReturn.erase(i,1);
                sReturn[i] = (char)225;
            }
        }
        if (sReturn[i] == (char)194 && sReturn[i+1] == (char)176)
        {
            sReturn.erase(i,1);
            sReturn[i] = (char)248;
        }
    }
    return sReturn;
}

string getNextArgument(string& sArgList, bool bCut)
{
    if (!sArgList.length())
        return "";
    int nParenthesis = 0;
    int nVektorbrace = 0;
    unsigned int nPos = 0;
    for (unsigned int i = 0; i < sArgList.length(); i++)
    {
        //cerr << nParenthesis << " ";
        if (sArgList[i] == '(' && !isInQuotes(sArgList, i, true))
            nParenthesis++;
        if (sArgList[i] == ')' && !isInQuotes(sArgList, i, true))
            nParenthesis--;
        if (sArgList[i] == '{' && !isInQuotes(sArgList, i, true))
        {
            nVektorbrace++;
        }
        if (sArgList[i] == '}' && !isInQuotes(sArgList, i, true))
        {
            nVektorbrace--;
        }
        if (sArgList[i] == ',' && !nParenthesis && !nVektorbrace && !isInQuotes(sArgList, i, true))
        {
            nPos = i;
            break;
        }
    }
    if (!nPos && sArgList[0] != ',')
        nPos = sArgList.length();
    if (!nPos)
    {
        if (bCut && sArgList[0] == ',')
            sArgList.erase(0,1);
        return "";
    }
    string sArg = sArgList.substr(0,nPos);
    StripSpaces(sArg);
    if (bCut && sArgList.length() > nPos+1)
        sArgList = sArgList.substr(nPos+1);
    else if (bCut)
        sArgList = "";
    return sArg;
}

void make_hline(int nLength)
{
    if (nLength == -1)
    {
        for (int i = 0; i < nLINE_LENGTH; i++)
        {
            cerr << (char)205;
        }
	}
	else if (nLength < -1)
	{
        for (int i = 0; i < nLINE_LENGTH; i++)
        {
            cerr << (char)196;
        }
	}
	else
	{
        for (int i = 0; i < nLength; i++)
        {
            cerr << (char)205;
        }
	}
	cerr << endl;
	return;
}

void make_progressBar(int nStep, int nFirstStep, int nFinalStep, const string& sType)
{
    static int nLastStatusVal = -1;
    int nStatusVal = 0;
    if (abs(nFinalStep-nFirstStep) < 9999
        && abs((nStep-nFirstStep) / (double)(nFinalStep-nFirstStep) * 20)
            > abs((nStep-1-nFirstStep) / (double)(nFinalStep-nFirstStep) * 20))
    {
        nStatusVal = abs((int)((nStep-nFirstStep) / (double)(nFinalStep-nFirstStep) * 20)) * 5;
    }
    else if (abs(nFinalStep-nFirstStep) >= 9999
        && abs((nStep-nFirstStep) / (double)(nFinalStep-nFirstStep) * 100)
            > abs((nStep-1-nFirstStep) / (double)(nFinalStep-nFirstStep) * 100))
    {
        nStatusVal = abs((int)((nStep-nFirstStep) / (double)(nFinalStep-nFirstStep) * 100));
    }
    if (nLastStatusVal < 0)
        nLastStatusVal = nStatusVal;
    else if (nLastStatusVal != nStatusVal)
        nLastStatusVal = nStatusVal;
    else
        return;

    cerr << "\r                                                                                ";
    if (sType == "std")
    {
        cerr << "\r|-> " << _lang.get("COMMON_EVALUATING") << " ... " << nStatusVal << " %";
    }
    else if (sType == "bar")
    {
        cerr << "\r|-> " << (char)186;
        for (int i = 0; i < 20; i++)
        {
            if (i < nStatusVal/5.0)
                cerr << (char)178;
            else
                cerr << (char)176;
        }
        cerr << (char)186 << " (" << nStatusVal << " %)";
    }
    else
    {
        cerr << "\r|";
        for (unsigned int i = 0; i < sType.length(); i++)
        {
            if (sType.substr(i,5) == "<bar>")
            {
                cerr << (char)186;
                for (int j = 0; j < 20; j++)
                {
                    if (j < nStatusVal/5.0)
                        cerr << (char)178;
                    else
                        cerr << (char)176;
                }
                cerr << (char)186;
                i += 4;
                continue;
            }
            if (sType.substr(i,5) == "<Bar>")
            {
                cerr << (char)186;
                for (int j = 0; j < 20; j++)
                {
                    if (j < nStatusVal/5.0)
                        cerr << (char)178;
                    else
                        cerr << " ";
                }
                cerr << (char)186;
                i += 4;
                continue;
            }
            if (sType.substr(i,5) == "<BAR>")
            {
                cerr << (char)186;
                for (int j = 0; j < 20; j++)
                {
                    if (j < nStatusVal/5.0)
                        cerr << (char)219;
                    else
                        cerr << (char)176;
                }
                cerr << (char)186;
                i += 4;
                continue;
            }
            if (sType.substr(i,5) == "<val>")
            {
                cerr << nStatusVal;
                i += 4;
                continue;
            }
            if (sType.substr(i,5) == "<Val>")
            {
                cerr << std::setw(3) << std::setfill(' ') << nStatusVal;
                i += 4;
                continue;
            }
            if (sType.substr(i,5) == "<VAL>")
            {
                cerr << std::setw(3) << std::setfill('0') << nStatusVal;
                i += 4;
                continue;
            }
            cerr << sType[i];
        }
    }
    if (nFinalStep == nStep)
    {
        cerr << endl;
        nLastStatusVal = -1;
    }
    return;
}

bool containsStrings(const string& sLine)
{
    if (!sLine.length())
        return false;
    if (sLine.find('"') != string::npos
        || sLine.find('#') != string::npos
        || sLine.find("string(") != string::npos
        || sLine.find("string_cast(") != string::npos
        || sLine.find("is_data(") != string::npos
        || sLine.find("replace(") != string::npos
        || sLine.find("replaceall(") != string::npos
        || sLine.find("to_cmd(") != string::npos
        || sLine.find("substr(") != string::npos
        || sLine.find("strfnd(") != string::npos
        || sLine.find("strrfnd(") != string::npos
        || sLine.find("strlen(") != string::npos
        || sLine.find("findparam(") != string::npos
        || sLine.find("findfile(") != string::npos
        || sLine.find("getopt(") != string::npos
        || sLine.find("getindices(") != string::npos
        || sLine.find("getmatchingparens(") != string::npos
        || sLine.find("getfilelist(") != string::npos
        || sLine.find("ascii(") != string::npos
        || sLine.find("char(") != string::npos
        || sLine.find("valtostr(") != string::npos
        || sLine.find("split(") != string::npos)
        return true;
    return false;
}

bool fileExists(const string& sFilename)
{
    if (sFilename.length())
    {
        string _sFile = sFilename;
        _sFile = fromSystemCodePage(_sFile);
        ifstream ifFile(_sFile.c_str());
        return ifFile.good();
    }
    else
        return false;
}

void eraseToken(string& sExpr, const string& sToken, bool bTokenHasValue)
{
    unsigned int nLength = sToken.length();
    if (bTokenHasValue)
    {
        if (!matchParams(sExpr, sToken, '='))
            return;
        for (unsigned int i = matchParams(sExpr, sToken, '=') + nLength-1; i < sExpr.length(); i++)
        {
            if (sExpr[i] == '=')
            {
                for (unsigned int j = sExpr.find_first_not_of("= ", i); j < sExpr.length(); j++)
                {
                    if (!isInQuotes(sExpr, j) && (sExpr[j] == '(' || sExpr[j] == '[' || sExpr[j] == '{'))
                        j += getMatchingParenthesis(sExpr.substr(j));
                    if (sExpr[j] == ' ')
                    {
                        sExpr.erase(matchParams(sExpr, sToken, '=')-1, j - matchParams(sExpr, sToken, '=')+1);
                        return;
                    }
                }
            }
        }
    }
    else
    {
        if (!matchParams(sExpr, sToken))
            return;
        sExpr.erase(matchParams(sExpr, sToken)-1, nLength);
    }
    return;
}

vector<string> getDBFileContent(const string& sFilename, Settings& _option)
{
    vector<string> vDBEntries;
    string sLine;
    string sPath = _option.ValidFileName(sFilename, ".ndb");
    ifstream fDB;

    fDB.open(sPath.c_str());
    if (fDB.fail())
        return vDBEntries;

    while (!fDB.eof())
    {
        getline(fDB, sLine);
        StripSpaces(sLine);
        if (sLine.length())
        {
            if (sLine[0] == '#')
                continue;
            vDBEntries.push_back(sLine);
        }
    }
    return vDBEntries;
}

vector<vector<string> > getDataBase(const string& sDatabaseFileName, Settings& _option)
{
    vector<string> vDBEntries = getDBFileContent(sDatabaseFileName, _option);
    vector<vector<string> > vDatabase(vDBEntries.size(), vector<string>());

    for (unsigned int i = 0; i < vDBEntries.size(); i++)
    {
        if (vDBEntries[i].find('~') == string::npos)
            vDatabase[i].push_back(vDBEntries[i]);
        else
        {
            while (vDBEntries[i].find('~') != string::npos)
            {
                if (vDBEntries[i].substr(0, vDBEntries[i].find('~')).size())
                    vDatabase[i].push_back(vDBEntries[i].substr(0, vDBEntries[i].find('~')));
                vDBEntries[i].erase(0,vDBEntries[i].find('~')+1);
            }
            if (vDBEntries[i].size())
                vDatabase[i].push_back(vDBEntries[i]);
        }
    }

    return vDatabase;
}

void printLogo()
{
    int nSleepDur = 5;
    cerr << "                          .....             ...````..` ` ` ```.." << endl; Sleep(nSleepDur);
    cerr << "                    ..JMHWXXwXXWa.     ..`!.......`..WMHdM.  ....?." << endl; Sleep(nSleepDur);
    cerr << "                  .NHXWWWWXXZzXzwM, .?``....^.^.`.dMWK#NMMMc .......1." << endl; Sleep(nSleepDur);
    cerr << "                    ?NWWXX0XUZwwwwJ, ....^.....`.#WHHWWgMMM^  .`^...`.l" << endl; Sleep(nSleepDur);
    cerr << "                     MXWXkXXXVwwzulz   `..^`..,u4WXXHNddMMY^\"&,`....`..4" << endl; Sleep(nSleepDur);
    cerr << "                    JHXSUkXXXXvuwuw,b  `..Y5!! JHWWWKKNMMF     T,.`.^`^.h" << endl; Sleep(nSleepDur);
    cerr << "                   JHfXX0XXXwwwXuXuwJ.x5!`    .#KHHWddMMM!      .,.^....Jr" << endl; Sleep(nSleepDur);
    cerr << "                  .MXWXXX0XXwv0wXwXX|M        dWXUWNNMMMF        ....... N" << endl; Sleep(nSleepDur);
    cerr << "                 .#WWXW0XXwXvXuzwXdXXJ,      .#HHWWfNMM#          ;.^^^.`J" << endl; Sleep(nSleepDur);
    cerr << "                 MXWXXXw\\p1zwzuzudXZXnH      MKHHHWdMMM^          %.....`J" << endl; Sleep(nSleepDur);
    cerr << "                JWXWXXXfJM.wXwzwXXwudXJb    .NWXK#NMMMF          .:.....`J" << endl; Sleep(nSleepDur);
    cerr << "               JNWWZyZXjMMbjwuX0dXZXSXXJ,   MHHHdVgMM@.          ....^.^ F" << endl; Sleep(nSleepDur);
    cerr << "              .NWXWXXw|MMD 4kzuwXwXdXXXkW. .MKHNNddMM!           \\.^...`.F" << endl; Sleep(nSleepDur);
    cerr << "            .uMWWXWXXVJM5   NwwXXuXXdXyXSN #WWk#@NMM$           ,.....^`J" << endl; Sleep(nSleepDur);
    cerr << "           .t#WWVXXXX:M%    JKduZXXXVYVXkaedwV9MMHMM.          .`..^..`.$" << endl; Sleep(nSleepDur);
    cerr << "          .tJWWXWXZXrJF      4kwV1gMMMMHWHkWWXXXXXwkXwdWUu..  .!....^`.F" << endl; Sleep(nSleepDur);
    cerr << "         .D.HWWWXWXu.F        40.MMMMM#HWHWWkkyXVUTwwXXwruXXHG, `^..`.#" << endl; Sleep(nSleepDur);
    cerr << "        .8.NHXWXXWwiM          N,4MMMMFHkHkWXXSkJM@?!&wXwzuZSVXMa. ..M." << endl; Sleep(nSleepDur);
    cerr << "       .@j8HWXWXXXrJ3          .NA,Y4#dHWWWWSXX\\MB    JKuXuXXWWHKH..#." << endl; Sleep(nSleepDur);
    cerr << "       #i@dWWWXWZZ.F`           .NSj#dWWVWkkyXrJM`     Nw0XXWWHWQMMM`" << endl; Sleep(nSleepDur);
    cerr << "      J3PdHWWWXZwj#.             .hMdHWHWWXXkf.Mt     .#XXSXWKHBdMMF" << endl; Sleep(nSleepDur);
    cerr << "      h8dWkWkWX0jH.               J#dWUfXWSXXjMF      JuXWXWWWKdMMM" << endl; Sleep(nSleepDur);
    cerr << "     .EdWWkWXW3.B.               .MkHWWWXXXW\\MP     .HuZZZWWHHKMM#!" << endl; Sleep(nSleepDur);
    cerr << "     JkWWHXYq&#^`               .MHWUkXWXXXrJ#   ..WwXXXXXWWVhdM\"" << endl; Sleep(nSleepDur);
    cerr << "     JgJggMMMF                 .MKdWWWXWSyV.M..JW0wrzXZVTWggM#=" << endl; Sleep(nSleepDur);
    cerr << "     JMMMMM#4                  MMdHkHWXXXSiM m00uwXwuXn?MMM\"" << endl; Sleep(nSleepDur);
    cerr << "     .MMMN#gJr                JMNdHWWWXXX\\JN34ZwXwwuwXXXnd." << endl; Sleep(nSleepDur);
    cerr << "      WMMMMNMN.              JMMqHHWXSSyrJMF.,WwwvwwXXZWWHHQN.." << endl; Sleep(nSleepDur);
    cerr << "      .MMMMMMMN.            .MNdHWHkWXXYJM#.+li4JwzXdZXWWWbNMMMMMa&" << endl; Sleep(nSleepDur);
    cerr << "       .MMMMMMMMN&..        MM#WWWVXWSfJMM +Ov?.MxXXXXWWfKdMMMMMMMM.`" << endl; Sleep(nSleepDur);
    cerr << "        .4MMMMMMMMNMWHMHHH\\MM#QHWXWXYuMM#:?1JM5`` .WkyVWWWMMMMMMMM@." << endl; Sleep(nSleepDur);
    cerr << "          .WMMMMMMMMMNN#HWlWNUKTUqJMMMM5.xY3``       TWkBMMMMMMMM5`" << endl; Sleep(nSleepDur);
    cerr << "            .\"MMMMMMMMMNNKHhdMMMMMMMMH97!`               ?T\"MH9:!" << endl; Sleep(nSleepDur);
    cerr << "                ?T\"MMMMMMN#NgmVMBY:!`     " << std::setfill((char)196) << std::setw(37) << (char)196 << endl; Sleep(nSleepDur);
    cerr << "                     `````````             " << std::setfill(' ') << std::setw(38-sVersion.length()-2) << "v " << sVersion << endl;
    Sleep(100);
    return;
}

string getTimeStamp(bool bGetStamp)
{

	time_t now = time(0);		// Aktuelle Zeit initialisieren
	tm *ltm = localtime(&now);
	ostringstream Temp_str;

	Temp_str << 1900+ltm->tm_year << "-"; //YYYY-
	if(1+ltm->tm_mon < 10)		// 0, falls Monat kleiner als 10
		Temp_str << "0";
	Temp_str << 1+ltm->tm_mon << "-"; // MM-
	if(ltm->tm_mday < 10)		// 0, falls Tag kleiner als 10
		Temp_str << "0";
	Temp_str << ltm->tm_mday; 	// DD
	if(bGetStamp)
		Temp_str << "_";		// Unterstrich im Dateinamen
	else
	{
		Temp_str << ", ";	// Komma im regulaeren Datum
		if (_lang.get("TOOLS_TIMESTAMP_AT") == "TOOLS_TIMESTAMP_AT")
            Temp_str << "at";
        else
            Temp_str << _lang.get("TOOLS_TIMESTAMP_AT");
		Temp_str << " ";
	}
	if(ltm->tm_hour < 10)
		Temp_str << "0";
	Temp_str << ltm->tm_hour; 	// hh
	if(!bGetStamp)
		Temp_str << ":";		// ':' im regulaeren Datum
	if(ltm->tm_min < 10)
		Temp_str << "0";
	Temp_str << ltm->tm_min;	// mm
	if(!bGetStamp)
		Temp_str << ":";
	if(ltm->tm_sec < 10)
		Temp_str << "0";
	Temp_str << ltm->tm_sec;	// ss
	return Temp_str.str();
}

vector<string> resolveChooseTokens(const string& sDirectory, const Settings& _option)
{
    vector<string> vResolved;
    vResolved.push_back(sDirectory);
    string sToken;
    unsigned int nSize = 0, nth_choose = 0;
    bool bResolvingPath = false;

    if (sDirectory.find('|') != string::npos)
    {
        while (vResolved[0].find('|') != string::npos)
        {
            if (!vResolved[0].rfind('<'))
                break;
            sToken = vResolved[0].substr(vResolved[0].rfind('<')+1);
            sToken.erase(sToken.find('>'));
            nSize = vResolved.size();
            nth_choose = 0;
            while (sToken.find('|') != string::npos || sToken.length())
            {
                // so lange ein "|" in dem Token gefunden wird, muss der Baum dupliziert werden
                if (sToken.find('|') != string::npos)
                {
                    for (unsigned int i = 0; i < nSize; i++)
                        vResolved.push_back(vResolved[i+nth_choose*nSize]);
                }
                // Die Tokens ersetzen
                for (unsigned int i = nth_choose*nSize; i < (nth_choose+1)*nSize; i++)
                {
                    if (!bResolvingPath && vResolved[i].rfind('/') != string::npos && vResolved[i].rfind('/') > vResolved[i].rfind('>'))
                        bResolvingPath = true;
                    vResolved[i].replace(vResolved[i].rfind('<'), vResolved[i].rfind('>')+1-vResolved[i].rfind('<'), sToken.substr(0,sToken.find('|')));
                }
                if (bResolvingPath
                    && ((vResolved[nth_choose*nSize].find('*') != string::npos && vResolved[nth_choose*nSize].find('*') < vResolved[nth_choose*nSize].rfind('/'))
                        || (vResolved[nth_choose*nSize].find('?') != string::npos && vResolved[nth_choose*nSize].find('?') < vResolved[nth_choose*nSize].rfind('/'))))
                {
                    // Platzhalter in Pfaden werden mit einer Rekursion geloest.
                    vector<string> vFolderList = getFolderList(vResolved[nth_choose*nSize].substr(0,vResolved[nth_choose*nSize].rfind('/')), _option, 1);
                    for (unsigned int j = 0; j < vFolderList.size(); j++)
                    {
                        if (vFolderList.size() > 1 && j < vFolderList.size()-1)
                        {
                            // ggf. Baum duplizieren
                            for (unsigned int k = 0; k < nSize; k++)
                            {
                                vResolved.push_back(vResolved[k+(nth_choose+1)*nSize]);
                                vResolved[k+(nth_choose+1)*nSize] = vResolved[k+nth_choose*nSize];
                            }
                        }
                        for (unsigned int k = nth_choose*nSize; k < (nth_choose+1)*nSize; k++)
                        {
                            //cerr << vFolderList[j] << endl;
                            vResolved[k].replace(0, vResolved[k].rfind('/'), vFolderList[j]);
                        }
                        if (vFolderList.size() > 1 && j < vFolderList.size()-1)
                            nth_choose++;
                    }
                }
                bResolvingPath = false;
                nth_choose++;
                if (sToken.find('|') != string::npos)
                    sToken.erase(0,sToken.find('|')+1);
                else
                {
                    sToken.clear();
                    break;
                }
            }
        }
    }
    if ((vResolved[0].find('*') != string::npos && vResolved[0].find('*') < vResolved[0].rfind('/'))
        || (vResolved[0].find('?') != string::npos && vResolved[0].find('?') < vResolved[0].rfind('/')))
    {
        // Platzhalter in Pfaden werden mit einer Rekursion geloest.
        vector<string> vFolderList = getFolderList(vResolved[0].substr(0,vResolved[0].rfind('/')), _option, 1);
        //cerr << vFolderList.size();
        nSize = vResolved.size();
        //cerr << nSize << endl;
        for (unsigned int i = 0; i < vFolderList.size()-1; i++)
        {
            // ggf. Baum duplizieren
            for (unsigned int k = 0; k < nSize; k++)
            {
                vResolved.push_back(vResolved[k]);
            }

        }
        for (unsigned int j = 0; j < vFolderList.size(); j++)
        {
            for (unsigned int k = j*nSize; k < (j+1)*nSize; k++)
            {
                //cerr << vFolderList[j] << endl;
                vResolved[k].replace(0, vResolved[k].rfind('/'), vFolderList[j]);
                //cerr << vResolved[k] << endl;
            }
        }
    }

    return vResolved;
}

vector<string> getFileList(const string& sDirectory, const Settings& _option, int nFlags)
{
    vector<string> vFileList;
    vector<string> vDirList;
    string sDir = replacePathSeparator(sDirectory);
    vDirList = resolveChooseTokens(sDir, _option);
    for (unsigned int i = 0; i < vDirList.size(); i++)
    {
        sDir = vDirList[i];
        //cerr << sDir << endl;
        if (sDir.rfind('.') == string::npos && sDir.find('*') == string::npos && sDir.find('?') == string::npos)
        {
            if (sDir[sDir.find_last_not_of(' ')] != '/')
                sDir += '/';
            sDir += "*";
        }
        else if ((sDir.find('.') == string::npos
                || (sDir.find('.') != string::npos && sDir.find('/', sDir.find('.')) != string::npos))
            && sDir.back() != '*')
            sDir += "*";
        //cerr << sDir << endl << sDirectory << endl;
        WIN32_FIND_DATA FindFileData;
        HANDLE hFind = INVALID_HANDLE_VALUE;

        if (sDir[0] == '.')
        {
            hFind = FindFirstFile((_option.getExePath() + "\\" + sDir).c_str(), &FindFileData);
            sDir = replacePathSeparator(_option.getExePath() + "/" + sDir);
            sDir.erase(sDir.rfind('/')+1);
        }
        else if (sDir[0] == '<')
        {
            if (sDir.substr(0,10) == "<loadpath>")
            {
                hFind = FindFirstFile((_option.getLoadPath() + sDir.substr(sDir.find('>')+1)).c_str(), &FindFileData);
                sDir = replacePathSeparator(_option.getLoadPath() + sDir.substr(sDir.find('>')+1));
                sDir.erase(sDir.rfind('/')+1);
            }
            else if (sDir.substr(0,10) == "<savepath>")
            {
                hFind = FindFirstFile((_option.getSavePath() + sDir.substr(sDir.find('>')+1)).c_str(), &FindFileData);
                sDir = replacePathSeparator(_option.getSavePath() + sDir.substr(sDir.find('>')+1));
                sDir.erase(sDir.rfind('/')+1);
            }
            else if (sDir.substr(0,12) == "<scriptpath>")
            {
                hFind = FindFirstFile((_option.getScriptPath() + sDir.substr(sDir.find('>')+1)).c_str(), &FindFileData);
                sDir = replacePathSeparator(_option.getScriptPath() + sDir.substr(sDir.find('>')+1));
                sDir.erase(sDir.rfind('/')+1);
            }
            else if (sDir.substr(0,10) == "<plotpath>")
            {
                hFind = FindFirstFile((_option.getPlotOutputPath() + sDir.substr(sDir.find('>')+1)).c_str(), &FindFileData);
                sDir = replacePathSeparator(_option.getPlotOutputPath() + sDir.substr(sDir.find('>')+1));
                sDir.erase(sDir.rfind('/')+1);
            }
            else if (sDir.substr(0,10) == "<procpath>")
            {
                hFind = FindFirstFile((_option.getProcsPath()+sDir.substr(sDir.find('>')+1)).c_str(), &FindFileData);
                sDir = replacePathSeparator(_option.getProcsPath() + sDir.substr(sDir.find('>')+1));
                sDir.erase(sDir.rfind('/')+1);
            }
            else if (sDir.substr(0,2) == "<>")
            {
                hFind = FindFirstFile((_option.getExePath() + sDir.substr(sDir.find('>')+1)).c_str(), &FindFileData);
                sDir = replacePathSeparator(_option.getExePath() + sDir.substr(sDir.find('>')+1));
                sDir.erase(sDir.rfind('/')+1);
            }
            else if (sDir.substr(0,6) == "<this>")
            {
                hFind = FindFirstFile((_option.getExePath() + sDir.substr(sDir.find('>')+1)).c_str(), &FindFileData);
                sDir = replacePathSeparator(_option.getExePath() + sDir.substr(sDir.find('>')+1));
                sDir.erase(sDir.rfind('/')+1);
            }
        }
        else
        {
            hFind = FindFirstFile(sDir.c_str(), &FindFileData);
            if (sDir.find('/') != string::npos)
                sDir.erase(sDir.rfind('/')+1);
        }
        if (hFind == INVALID_HANDLE_VALUE)
            continue;

        do
        {
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;
            if (nFlags & 1)
                vFileList.push_back(sDir + FindFileData.cFileName);
            else
                vFileList.push_back(FindFileData.cFileName);
        }
        while (FindNextFile(hFind, &FindFileData) != 0);
        FindClose(hFind);
    }
    return vFileList;
}

vector<string> getFolderList(const string& sDirectory, const Settings& _option, int nFlags)
{
    vector<string> vFileList;
    vector<string> vDirList;
    string sDir = replacePathSeparator(sDirectory);
    vDirList = resolveChooseTokens(sDir, _option);
    for (unsigned int i = 0; i < vDirList.size(); i++)
    {
        sDir = vDirList[i];
        if (sDir.rfind('.') == string::npos && sDir.find('*') == string::npos && sDir.find('?') == string::npos)
        {
            if (sDir[sDir.find_last_not_of(' ')] != '/')
                sDir += '/';
            sDir += "*";
        }
        else if ((sDir.find('.') == string::npos
                || (sDir.find('.') != string::npos && sDir.find('/', sDir.find('.')) != string::npos))
            && sDir.back() != '*')
            sDir += "*";
        //cerr << sDir << endl << sDirectory << endl;
        WIN32_FIND_DATA FindFileData;
        HANDLE hFind = INVALID_HANDLE_VALUE;

        if (sDir[0] == '.')
        {
            hFind = FindFirstFile((_option.getExePath() + "\\" + sDir).c_str(), &FindFileData);
            sDir = replacePathSeparator(_option.getExePath() + "/" + sDir);
            sDir.erase(sDir.rfind('/')+1);
        }
        else if (sDir[0] == '<')
        {
            if (sDir.substr(0,10) == "<loadpath>")
            {
                hFind = FindFirstFile((_option.getLoadPath() + sDir.substr(sDir.find('>')+1)).c_str(), &FindFileData);
                sDir = replacePathSeparator(_option.getLoadPath() + sDir.substr(sDir.find('>')+1));
                sDir.erase(sDir.rfind('/')+1);
            }
            else if (sDir.substr(0,10) == "<savepath>")
            {
                hFind = FindFirstFile((_option.getSavePath() + sDir.substr(sDir.find('>')+1)).c_str(), &FindFileData);
                sDir = replacePathSeparator(_option.getSavePath() + sDir.substr(sDir.find('>')+1));
                sDir.erase(sDir.rfind('/')+1);
            }
            else if (sDir.substr(0,12) == "<scriptpath>")
            {
                hFind = FindFirstFile((_option.getScriptPath() + sDir.substr(sDir.find('>')+1)).c_str(), &FindFileData);
                sDir = replacePathSeparator(_option.getScriptPath() + sDir.substr(sDir.find('>')+1));
                sDir.erase(sDir.rfind('/')+1);
            }
            else if (sDir.substr(0,10) == "<plotpath>")
            {
                hFind = FindFirstFile((_option.getPlotOutputPath() + sDir.substr(sDir.find('>')+1)).c_str(), &FindFileData);
                sDir = replacePathSeparator(_option.getPlotOutputPath() + sDir.substr(sDir.find('>')+1));
                sDir.erase(sDir.rfind('/')+1);
            }
            else if (sDir.substr(0,10) == "<procpath>")
            {
                hFind = FindFirstFile((_option.getProcsPath()+sDir.substr(sDir.find('>')+1)).c_str(), &FindFileData);
                sDir = replacePathSeparator(_option.getProcsPath() + sDir.substr(sDir.find('>')+1));
                sDir.erase(sDir.rfind('/')+1);
            }
            else if (sDir.substr(0,2) == "<>")
            {
                hFind = FindFirstFile((_option.getExePath() + sDir.substr(sDir.find('>')+1)).c_str(), &FindFileData);
                sDir = replacePathSeparator(_option.getExePath() + sDir.substr(sDir.find('>')+1));
                sDir.erase(sDir.rfind('/')+1);
            }
            else if (sDir.substr(0,6) == "<this>")
            {
                hFind = FindFirstFile((_option.getExePath() + sDir.substr(sDir.find('>')+1)).c_str(), &FindFileData);
                sDir = replacePathSeparator(_option.getExePath() + sDir.substr(sDir.find('>')+1));
                sDir.erase(sDir.rfind('/')+1);
            }
        }
        else
        {
            hFind = FindFirstFile(sDir.c_str(), &FindFileData);
            if (sDir.find('/') != string::npos)
                sDir.erase(sDir.rfind('/')+1);
        }
        if (hFind == INVALID_HANDLE_VALUE)
            continue;

        do
        {
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (nFlags & 1)
                    vFileList.push_back(sDir + FindFileData.cFileName);
                else
                    vFileList.push_back(FindFileData.cFileName);
            }
            else
                continue;

        }
        while (FindNextFile(hFind, &FindFileData) != 0);
        FindClose(hFind);
    }
    return vFileList;
}

void reduceLogFilesize(const string& sFileName)
{
    fstream fFile;
    unsigned int nLines = 0;
    string sTemp;
    fFile.open(sFileName.c_str(), ios_base::binary | ios_base::in);
    if (fFile.fail())
        return;

    while (!fFile.eof())
    {
        getline(fFile, sTemp);
        nLines++;
    }

    //cerr << nLines << endl;
    fFile.clear();
    fFile.seekg(0);

    if (nLines >= 100000)
    {
        fstream fTemp;
        fTemp.open("$~tempfile.txt", ios_base::binary | ios_base::out);
        if (fTemp.fail())
            return;
        for (unsigned int i = 0; i < nLines; i++)
        {
            getline(fFile, sTemp);
            if (nLines - i > 20000)
                continue;
            fTemp << sTemp << endl;
        }
        fFile.close();
        fTemp.close();
        fTemp.open("$~tempfile.txt", ios_base::binary | ios_base::in);
        fFile.open(sFileName.c_str(), ios_base::trunc | ios_base::binary | ios_base::out);
        fTemp.seekg(0);
        fFile << fTemp.rdbuf();
        fFile.close();
        fTemp.close();
        remove("$~tempfile.txt");
    }
    return;
}

void OprtRplc_setup(map<string,string>& mOprtRplc)
{
    mOprtRplc["("] = "[";
    mOprtRplc[")"] = "]";
    mOprtRplc[":"] = "~";
    mOprtRplc[","] = "_";
    mOprtRplc["+"] = "\\p\\";
    mOprtRplc["-"] = "\\m\\";
    mOprtRplc["*"] = "\\ml\\";
    mOprtRplc["/"] = "\\d\\";
    mOprtRplc["^"] = "\\e\\";
    mOprtRplc["{"] = "\\ob\\";
    mOprtRplc["}"] = "\\cb\\";
    mOprtRplc["&&"] = "\\a\\";
    mOprtRplc["||"] = "\\o\\";
    mOprtRplc["%"] = "\\md\\";
    mOprtRplc["!"] = "\\n\\";
    mOprtRplc["=="] = "\\eq\\";
    mOprtRplc["!="] = "\\ne\\";
    mOprtRplc[">="] = "\\ge\\";
    mOprtRplc["<="] = "\\le\\";
    mOprtRplc["?"] = "\\q\\";
    return;
}

string replaceToVectorname(const string& sExpression)
{
    string sVectorName = sExpression;
    static map<string,string> mOprtRplc;
    // + \p\, - \m\, * \ml\, / \d\, ^ \e\, && \a\, || \o\, ||| \xo\, % \md\, ! \n\, == \eq\, != \ne\, >= \ge\, <= \le\, ? \q\//
    /*mOprtRplc["("] = "[";
    mOprtRplc[")"] = "]";
    mOprtRplc[":"] = "~";
    mOprtRplc[","] = "_";
    mOprtRplc["+"] = "\\p\\";
    mOprtRplc["-"] = "\\m\\";
    mOprtRplc["*"] = "\\ml\\";
    mOprtRplc["/"] = "\\d\\";
    mOprtRplc["^"] = "\\e\\";
    mOprtRplc["{"] = "\\ob\\";
    mOprtRplc["}"] = "\\cb\\";
    mOprtRplc["&&"] = "\\a\\";
    mOprtRplc["||"] = "\\o\\";
    mOprtRplc["%"] = "\\md\\";
    mOprtRplc["!"] = "\\n\\";
    mOprtRplc["=="] = "\\eq\\";
    mOprtRplc["!="] = "\\ne\\";
    mOprtRplc[">="] = "\\ge\\";
    mOprtRplc["<="] = "\\le\\";
    mOprtRplc["?"] = "\\q\\";*/
    if (!mOprtRplc.size())
        OprtRplc_setup(mOprtRplc);

    while (sVectorName.find("|||") != string::npos)
        sVectorName.replace(sVectorName.find("|||"),3,"\\xo\\");
    while (sVectorName.find(' ') != string::npos)
        sVectorName.erase(sVectorName.find(' '),1);
    for (auto iter = mOprtRplc.begin(); iter != mOprtRplc.end(); ++iter)
    {
        while (sVectorName.find(iter->first) != string::npos)
            sVectorName.replace(sVectorName.find(iter->first), (iter->first).length(), iter->second);
    }
    return sVectorName;
}

string getClipboardText()
{
    // Try opening the clipboard
    if (! OpenClipboard(nullptr))
        return "";

    // Get handle of clipboard object for ANSI text
    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == nullptr)
    {
        CloseClipboard();
        return "";
    }

    // Lock the handle to get the actual text pointer
    char * pszText = static_cast<char*>( GlobalLock(hData) );
    if (pszText == nullptr)
    {
        GlobalUnlock(hData);
        CloseClipboard();
        return "";
    }

    // Save text in a string class instance
    string sText( pszText );

    // Release the lock
    GlobalUnlock( hData );

    // Release the clipboard
    CloseClipboard();

    return sText;
}

void evalRecursiveExpressions(string& sExpr)
{
    if (sExpr.substr(0,3) == "if "
        || sExpr.substr(0,3) == "if("
        || sExpr.substr(0,7) == "elseif "
        || sExpr.substr(0,7) == "elseif("
        || sExpr.substr(0,5) == "else "
        || sExpr.substr(0,4) == "for "
        || sExpr.substr(0,4) == "for("
        || sExpr.substr(0,6) == "while "
        || sExpr.substr(0,6) == "while(")
        return;

    if (sExpr.find("+=") != string::npos
        || sExpr.find("-=") != string::npos
        || sExpr.find("*=") != string::npos
        || sExpr.find("/=") != string::npos
        || sExpr.find("^=") != string::npos
        || sExpr.find("++") != string::npos
        || sExpr.find("--") != string::npos)
    {
        unsigned int nArgSepPos = 0;
        for (unsigned int i = 0; i < sExpr.length(); i++)
        {
            if (isInQuotes(sExpr, i, false))
                continue;
            if (sExpr[i] == '(' || sExpr[i] == '{')
                i += getMatchingParenthesis(sExpr.substr(i));
            if (sExpr[i] == ',')
                nArgSepPos = i;
            if (sExpr.substr(i,2) == "+="
                || sExpr.substr(i,2) == "-="
                || sExpr.substr(i,2) == "*="
                || sExpr.substr(i,2) == "/="
                || sExpr.substr(i,2) == "^=")
            {
                if (sExpr.find(',', i) != string::npos)
                {
                    for (unsigned int j = i; j < sExpr.length(); j++)
                    {
                        if (sExpr[j] == '(')
                            j += getMatchingParenthesis(sExpr.substr(j));
                        if (sExpr[j] == ',' || j+1 == sExpr.length())
                        {
                            if (!nArgSepPos && j+1 != sExpr.length())
                                sExpr = sExpr.substr(0, i)
                                    + " = "
                                    + sExpr.substr(0, i)
                                    + sExpr[i]
                                    + "("
                                    + sExpr.substr(i+2, j-i-2)
                                    + ") "
                                    + sExpr.substr(j);
                            else if (nArgSepPos && j+1 != sExpr.length())
                                sExpr = sExpr.substr(0, i)
                                    + " = "
                                    + sExpr.substr(nArgSepPos+1, i-nArgSepPos-1)
                                    + sExpr[i]
                                    + "("
                                    + sExpr.substr(i+2, j-i-2)
                                    + ") "
                                    + sExpr.substr(j);
                            else if (!nArgSepPos && j+1 == sExpr.length())
                                sExpr = sExpr.substr(0, i)
                                    + " = "
                                    + sExpr.substr(0, i)
                                    + sExpr[i]
                                    + "("
                                    + sExpr.substr(i+2)
                                    + ") ";
                            else
                                sExpr = sExpr.substr(0, i)
                                    + " = "
                                    + sExpr.substr(nArgSepPos+1, i-nArgSepPos-1)
                                    + sExpr[i]
                                    + "("
                                    + sExpr.substr(i+2)
                                    + ") ";

                            for (unsigned int k = i; k < sExpr.length(); k++)
                            {
                                if (sExpr[k] == '(')
                                    k += getMatchingParenthesis(sExpr.substr(k));
                                if (sExpr[k] == ',')
                                {
                                    nArgSepPos = k;
                                    i = k;
                                    break;
                                }
                            }
                            //cerr << sExpr << " | nArgSepPos=" << nArgSepPos << endl;
                            break;
                        }
                    }
                }
                else
                {
                    if (!nArgSepPos)
                        sExpr = sExpr.substr(0, i)
                            + " = "
                            + sExpr.substr(0, i)
                            + sExpr[i]
                            + "("
                            + sExpr.substr(i+2)
                            + ")";
                    else
                        sExpr = sExpr.substr(0, i)
                            + " = "
                            + sExpr.substr(nArgSepPos+1, i-nArgSepPos-1)
                            + sExpr[i]
                            + "("
                            + sExpr.substr(i+2)
                            + ")";
                    break;
                }
            }
            if (sExpr.substr(i,2) == "++" || sExpr.substr(i,2) == "--")
            {
                if (!nArgSepPos)
                {
                    sExpr = sExpr.substr(0, i)
                        + " = "
                        + sExpr.substr(0, i)
                        + sExpr[i]
                        + "1"
                        + sExpr.substr(i+2);
                }
                else
                    sExpr = sExpr.substr(0, i)
                        + " = "
                        + sExpr.substr(nArgSepPos+1, i-nArgSepPos-1)
                        + sExpr[i]
                        + "1"
                        + sExpr.substr(i+2);
            }
        }
    }

    return;
}

string generateCacheName(const string& sFilename, Settings& _option)
{
    string sCacheName;
    if (sFilename.find('/') != string::npos)
        sCacheName = _option.ValidFileName(sFilename);
    else
        sCacheName = _option.ValidFileName("<loadpath>/"+sFilename);
    string sValidChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";

    if (sCacheName.find('/') != string::npos)
        sCacheName.erase(0,sCacheName.rfind('/')+1);
    if (sCacheName.find('.') != string::npos)
        sCacheName.erase(sCacheName.rfind('.'));
    if (isdigit(sCacheName.front()))
        sCacheName.insert(0,1,'_');

    for (unsigned int i = 0; i < sCacheName.length(); i++)
    {
        if (sValidChars.find(sCacheName[i]) == string::npos)
            sCacheName[i] = '_';
    }

    if (sCacheName == "data")
        sCacheName = "loaded_data";
    return sCacheName;
}

bool containsDataObject(const string& sExpr)
{
    for (unsigned int i = 0; i < sExpr.length()-5; i++)
    {
        if (!i && sExpr.substr(i,5) == "data(")
            return true;
        else if (i && sExpr.substr(i,5) == "data(" && checkDelimiter(sExpr.substr(i-1,6)))
            return true;
    }
    return false;
}


