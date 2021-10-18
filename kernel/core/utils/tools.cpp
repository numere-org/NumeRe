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
#include "../../kernel.hpp"
#include "../io/file.hpp"
#include <cstdlib>


/////////////////////////////////////////////////
/// \brief This function searches the passed
/// parameter in the passed command string. If
/// something is found, the position of the
/// parameter+1 is returned.
///
/// \param sCmd const string&
/// \param sParam const string&
/// \param cFollowing const char
/// \return int
///
/// This function is one of the most important
/// ones in the whole application as it provides
/// the support for the defined command syntax.
/////////////////////////////////////////////////
int findParameter(const string& sCmd, const string& sParam, const char cFollowing)
{
    // --> Wenn kein '-' im string zu finden ist, ist da auch kein Parameter: FALSE zurueckgeben <--
    if (sCmd.find('-') == string::npos)
        return 0;
    else
    {
        // There's a minus sign.
        // Transform everything to lower case to avoid case sensitivity
        string __sCmd = toLowerCase(sCmd + " ");
        int nQuotes = 0;
        size_t nParamStart = string::npos;
        bool isOptionValue = false;

        // Go through the complete string
        for (size_t i = 0; i < __sCmd.length(); i++)
        {
            // Count the quotation marks, so that we're not inside of a string
            if (__sCmd[i] == '"'
                    && (!i || (i && __sCmd[i - 1] != '\\')))
                nQuotes++;

            // If all quotation marks are pairs and we found a minus sign, then we're probably at the start of a parameter
            if (!(nQuotes % 2) && __sCmd[i] == '-' && nParamStart == string::npos)
            {
                nParamStart = i;
            }

            // Detect option values and force NumeRe to ignore
            // them in this context
            if (nParamStart != string::npos
                && !(nQuotes % 2)
                && !isOptionValue
                && __sCmd[i] == '=')
            {
                isOptionValue = true;

                // jump over the following white spaces
                while (i+1 < __sCmd.length() && __sCmd[i+1] == ' ')
                    i++;
            }
            else if (nParamStart != string::npos
                && !(nQuotes % 2)
                && isOptionValue
                && __sCmd[i] == ' ')
                isOptionValue = false;

            // If we found a paramter start and the substring with the length of the searched parameter
            // is matching to the searched parameter, then we go into this block
            if (nParamStart != string::npos
                    && !(nQuotes % 2)
                    && !isOptionValue
                    && __sCmd.substr(i, sParam.length()) == sParam)
            {
                /* --> Pruefe die Zeichen davor und danach (unter Beachtung eines moeglicherweise
                 *     speziell gewaehlten Zeichens) <--
                 * --> Ein Parameter darf auf jeden Fall kein Teil eines anderen, laengeren Wortes
                 *     sein <--
                 */
                if (cFollowing == ' ')
                {
                    // Examine the characters before and after the found parameter match
                    if ((__sCmd[i - 1] == ' '
                            || __sCmd[i - 1] == '-')
                            && (__sCmd[i + sParam.length()] == ' '
                                || __sCmd[i + sParam.length()] == '-'
                                || __sCmd[i + sParam.length()] == '='
                                || __sCmd[i + sParam.length()] == '"')
                       )
                    {
                        // Avoid the identification of an option value as parameter
                        if (__sCmd[i - 1] == ' ' && __sCmd[__sCmd.find_last_not_of(' ', i - 1)] == '=')
                            i += sParam.length() - 1; // Add the parameter length to the loop index and continue
                        else
                            return i + 1;  // i+1 zurueckgeben, wenn Treffer
                    }
                    else
                        i += sParam.length() - 1;  // Positionsindex um die Laenge des Parameters weitersetzen
                }
                else
                {
                    // --> Wenn ein spezielles Zeichen gewaehlt wurde, wird dies hier gesucht <--
                    if ((__sCmd[i - 1] == ' ' || __sCmd[i - 1] == '-')
                            && (__sCmd[i + sParam.length()] == cFollowing))
                    {
                        // The following char matches explicit to the searched character
                        // Avoid the identification of an option value as parameter
                        if (__sCmd[i - 1] == ' ' && __sCmd[__sCmd.find_last_not_of(' ', i - 1)] == '=')
                            i += sParam.length() - 1;
                        else
                            return i + 1;
                    }
                    else if ((__sCmd[i - 1] == ' ' || __sCmd[i - 1] == '-')
                             && (__sCmd[i + sParam.length()] == ' '))
                    {
                        // The following character is a white space. We'll handle that further down
                        // First ensure that we did't find an option value
                        if (__sCmd[i - 1] == ' ' && __sCmd[__sCmd.find_last_not_of(' ', i - 1)] == '=')
                            i += sParam.length() - 1;
                        else
                        {
                            /* --> Fehlertoleranz: Leerzeichen zwischen dem Parameter und cFollowing werden ignoriert
                             *     (wenn cFollowing sowieso ein Leerzeichen sein sollte, wurde das ohnehin vorhin schon abgefangen) <--
                             */
                            int nSkip = 0;
                            while (i + sParam.length() + nSkip < __sCmd.length() && __sCmd[i + sParam.length() + nSkip] == ' ')
                                nSkip++;
                            if (__sCmd[i + sParam.length() + nSkip] == cFollowing)
                                return i + 1 + nSkip; // Wir addieren nSkip, da der Rueckgabewert vorzugsweise zum Navigieren zum Ende des Parameters verwendet wird
                            else
                                i += sParam.length() - 1;
                        }
                    }
                    else
                        i += sParam.length() - 1;
                }
            }
        }
    }

    // --> Wenn nichts gefunden wurde, gib' FALSE zurueck <--
    return 0;
}


/////////////////////////////////////////////////
/// \brief This function searches for the first
/// string value in the passed expression and
/// writes that into sArgument. If nothing was
/// found, this function returns false, true
/// otherwise.
///
/// \param sCmd const string&
/// \param sArgument string&
/// \return bool
///
/////////////////////////////////////////////////
bool getStringArgument(const string& sCmd, string& sArgument)
{
    size_t nPos = 0;

    // --> Wenn kein '"' oder kein '#' zu finden ist, gibt es auch kein String-Argument: FALSE zurueckgeben <--
    if (!containsStrings(sCmd))
        return false;

    if (sCmd.front() != '=')
    {
        // If the first character is not an equal sign, then try to find the first string operation part
        // This block only handles a small set of all available string functions
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
    }

    // Try to find the end of the string block
    // and cut out the identified argument
    sArgument = extractStringToken(sCmd, nPos);
    return true;
}


/////////////////////////////////////////////////
/// \brief This function extracts a string token
/// at the selected position. If the position is
/// start of an option value, the possible equal
/// sign is ignored.
///
/// \param sCmd const string&
/// \param nPos size_t
/// \return string
///
/////////////////////////////////////////////////
string extractStringToken(const string& sCmd, size_t nPos)
{
    // Increment the first position, if the command string begins with an equal sign
    if (sCmd.front() == '=')
        nPos++;

    size_t nPos_2 = 0;
    size_t nQuotes = isInQuotes(sCmd, nPos, true);

    for (size_t i = nPos; i < sCmd.length(); i++)
    {
        if (sCmd[i] == '"' && (!i || sCmd[i-1] != '\\'))
        {
            nQuotes++;
            continue;
        }

        if (nQuotes % 2)
            continue;

        // Jump over each parenthesis block
        if (sCmd[i] == '(' || sCmd[i] == '[' || sCmd[i] == '{')
        {
            i += getMatchingParenthesis(sCmd.substr(i));
            continue;
        }

        // Handle the variable to string parser
        if (sCmd[i] == '#')
        {
            for (size_t j = i; j < sCmd.length(); j++)
            {
                if (sCmd[j] == ' ')
                {
                    i = j;
                    break;
                }
                // jump over parentheses
                if (sCmd[j] == '(')
                    j += getMatchingParenthesis(sCmd.substr(j));

                if (j == sCmd.length() - 1)
                {
                    i = j;
                    break;
                }
            }
        }

        // If there's a whitespace
        if (sCmd[i] == ' ')
        {
            // Try to find a trailing plus sign
            if (sCmd.find_first_not_of(' ', i) != string::npos && sCmd[sCmd.find_first_not_of(' ', i)] != '+')
            {
                // None was found - break the loop
                nPos_2 = i - 1;
                break;
            }
            else
            {
                // A plus sign was found
                // Jump over the following whitespaces around the plus sign
                i = sCmd.find_first_not_of(" +", i)-1;
                continue;
            }
        }

        // If it's the last character or the current character is a
        // punctuation character
        if (i >= sCmd.length() - 1 || (ispunct(sCmd[i]) && sCmd[i] != '_' && sCmd[i] != '+' && sCmd[i] != '#'))
        {
            if (i == string::npos)
                nPos_2 = sCmd.length();
            else
                nPos_2 = i;
            break;
        }
    }

    // Cut out the identified argument
    if (!nPos_2)
        return sCmd.substr(nPos);

    return sCmd.substr(nPos, nPos_2 - nPos + 1);
}


/////////////////////////////////////////////////
/// \brief Returns the position of the closing
/// parenthesis.
///
/// \param sLine StringView
/// \return unsigned int The position of the closing parenthesis
///
/// This function determines the type of the
/// desired parenthesis by itself by using the
/// first parenthesis-like character that might be
/// found in the passed string. This function also
/// considers quotation marks, so that parentheses
/// in strings are not considered as matching. If
/// the function does not find the matching
/// parenthesis, it returns string::npos
/////////////////////////////////////////////////
//unsigned int getMatchingParenthesis(const string& sLine)
unsigned int getMatchingParenthesis(const StringView& sLine)
{
    size_t pos = sLine.find_first_of("([{");

    if (pos == string::npos)
        return pos;

    // Get the opening parenthesis
    char cParenthesis = sLine[pos];
    char cClosingParenthesis = 0;

    // Depending on the opening parenthesis, determine the closing one
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
            // Default case will handle that the user didn't pass a string, which begins with a parenthesis
            cParenthesis = '(';
            cClosingParenthesis = ')';
    }

    int nOpenParenthesis = 0;
    size_t nQuotes = 0;

    // Go through the string and count the opening and closing parentheses
    // Consider also whether the the current position is part of a larger string
    for (size_t i = 0; i < sLine.length(); i++)
    {
        // Count unmasked quotation marks
        if (sLine[i] == '"' && (!i || sLine[i-1] != '\\'))
            nQuotes++;

        if (nQuotes % 2)
            continue;

        // Increment the counter at opening paretheses
        if (sLine[i] == cParenthesis)
            nOpenParenthesis++;

        // Decrement the counter at closing parentheses
        if (sLine[i] == cClosingParenthesis)
            nOpenParenthesis--;

        // All parentheses are closed -> Return the position of this parenthesis
        if (!nOpenParenthesis && i > pos)
            return i;
    }

    // --> Falls die Klammer nicht schliesst, gebe -1 zurueck (analog zu string::find()) <--
    return string::npos;
}

// Prueft, ob ein Ausdruck ein Mehrfachausdruck oder nur eine Multi-Argument-Funktion ist
bool isMultiValue(const string& sExpr, bool bIgnoreClosingParenthesis)
{
    // --> Kein Komma? Auf jeden Fall kein Mehrfachausdruck! <--
    if (sExpr.find(',') == string::npos)
        return false;
    else
    {
        size_t nQuotationMarks = 0;

        // Go through the string
        for (unsigned int i = 0; i < sExpr.length(); i++)
        {
            // Jump over parentheses
            if ((sExpr[i] == '(' || sExpr[i] == '{' || sExpr[i] == '[') && !(nQuotationMarks % 2))
                i += getMatchingParenthesis(sExpr.substr(i));

            // Count quotation marks
            if (sExpr[i] == '"')
                nQuotationMarks++;

            if (sExpr[i] == ',' && !(nQuotationMarks % 2))
                return true;
        }

        // If the string contains a comma and the "ignore the closing parenthesis" flag is set, enter a recursion
        // This will cut out the argument of the first opening parenthesis
        // If no opening parenthesis is available, then leave this block
        if (sExpr.find_first_of("([{") != string::npos && bIgnoreClosingParenthesis)
            return isMultiValue(sExpr.substr(sExpr.find_first_of("([{")+1), true);
    }

    // Nothing found - fallback
    return false;
}

// This function is a static helper function for the function replaceToTeX further down
static void handleTeXIndicesAndExponents(string& sReturn, string sCodepage[][2] /* The way 2D arrays are passed into a function */, size_t nSymbols)
{
    string sDelimiter = "+-*/, #()&|!_'";

    // Handle the exponents
    for (unsigned int i = 0; i < sReturn.length() - 1; i++)
    {
        // Insert braces
        if (sReturn[i] == '^' && sReturn[i + 1] != '{' && sReturn[i + 1] != '(')
        {
            i++;
            sReturn = sReturn.substr(0, i) + "{" + sReturn.substr(i);
            if (sReturn[i + 1] == '-' || sReturn[i + 1] == '+')
                i++;
            i++;

            // Find the end of the current brace
            for (unsigned int j = i + 1; j < sReturn.length(); j++)
            {
                if (sDelimiter.find(sReturn[j]) != string::npos)
                {
                    sReturn = sReturn.substr(0, j) + "}" + sReturn.substr(j);
                    break;
                }
                else if (j + 1 == sReturn.length())
                {
                    sReturn += "}";
                }
            }
        }
    }

    // Append the exponent operator
    sDelimiter[sDelimiter.length() - 1] = '^';

    // Handle the indices
    for (unsigned int i = 0; i < sReturn.length() - 1; i++)
    {
        // Handle constants
        if (sReturn[i] == '_' && sReturn[i + 1] != '{')
        {
            for (unsigned int j = 8; j < nSymbols; j++)
            {
                if (sCodepage[j][0][0] != '_')
                    break;
                if (sReturn.substr(i, sCodepage[j][0].length()) == sCodepage[j][0] && (sDelimiter.find(sReturn[i + sCodepage[j][0].length()]) != string::npos || sReturn[i + sCodepage[j][0].length()] == '_'))
                {
                    i++;
                    break;
                }
            }
        }

        // Insert braces
        if (sReturn[i] == '_' && sReturn[i + 1] != '{')
        {
            i++;
            sReturn = sReturn.substr(0, i) + "{" + sReturn.substr(i);
            i++;

            // Find the end of the current brace
            for (unsigned int j = i + 1; j < sReturn.length(); j++)
            {
                if (sDelimiter.find(sReturn[j]) != string::npos)
                {
                    sReturn = sReturn.substr(0, j) + "}" + sReturn.substr(j);
                    break;
                }
                else if (j + 1 == sReturn.length())
                {
                    sReturn += "}";
                }
            }
        }
    }
}


// --> Ersetzt Tokens in einem String mit dem entsprechenden TeX-Befehl <--
#warning TODO (numere#3#08/15/21): Rework this function
string replaceToTeX(const string& sString, bool replaceForTeXFile) // bool-flag for true TeX files. The graph needs more tweaking
{
    string sReturn = " " + sString + " ";            // Rueckgabe-String
    string sTemp = "";                               // Temporaerer String, erleichert das Einfuegen von strings
    static const unsigned int nSymbols = 105;        // Anzahl an bekannten Symbolen
    unsigned int nPos = 0;                           // Positions-Index-Variable
    unsigned int nPos_2 = 0;                         // Positions-Index-Variable

    // --> 2D-String-Array: links das zu ersetzende Token, rechts die Ersetzung <--
    // The order is essentially: all constants have to be in a group starting from position 8
    static string sCodepage[nSymbols][2] =
    {
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
        {"_mu_bohr", "\\mu_{B}"},
        {"_mu_kern", "\\mu_{K}"},
        {"_m_amu", "m_u"},
        {"_r_bohr", "a_0"},
        {"_G", "G"},
        {"_theta_weinberg", "\\theta_{W}"},
        {"_mu_e", "\\mu_{e}"},
        {"_mu_p", "\\mu_{p}"},
        {"_mu_n", "\\mu_{n}"},
        {"_gamma_e", "\\gamma_{e}"},
        {"_gamma_p", "\\gamma_{p}"},
        {"_gamma_n", "\\gamma_{n}"},
        {"_stefan_boltzmann", "\\sigma "},
        {"_rydberg", "R_{\\infty}"},
        {"_hartree", "E_{h}"},
        {"_wien", "b_{Energie}"},
        {"_lande_e", "g_{e}"},
        {"_feigenbaum_alpha", "\\alpha "},
        {"_feigenbaum_delta", "\\delta "},
        {"inf", "\\infty"},
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
        {"heaviside", "\\Theta"},
        {"Li2", "Li_2"},
        {"Cl2", "Cl_2"}
    };


    // --> Ersetze zunaechst die gamma-Funktion <--
    while (sReturn.find("gamma(", nPos) != string::npos)
    {
        nPos = sReturn.find("gamma(", nPos);
        sReturn = sReturn.substr(0, nPos) + "\\Gamma(" + sReturn.substr(nPos + 6);
        nPos += 7;
    }

    nPos = 0;
    // --> Laufe durch alle bekannten Symbole <--
    for (unsigned int i = 0; i < nSymbols; i++)
    {
        // --> Positions-Indices zuruecksetzen <--
        nPos = 0;
        nPos_2 = 0;

        // Jump over the operators and the variables
        // if replacing the tokens for a TeX file
        if (replaceForTeXFile)
        {
            if (i == 1)
                i = 8;
        }

        // --> So lange in dem String ab der Position nPos das Token auftritt <--
        while (sReturn.find(sCodepage[i][0], nPos) != string::npos)
        {
            // --> Position des Treffers speichern <--
            nPos_2 = sReturn.find(sCodepage[i][0], nPos);
            // --> Falls vor dem Token schon ein '\' ist, wurde das hier schon mal ersetzt <--
            if (sReturn[nPos_2 - 1] == '\\')
            {
                // --> Positionsindex um die Laenge des Tokens weitersetzen <--
                nPos = nPos_2 + sCodepage[i][0].length();
                continue;
            }
            else if (i < 4) // only operators
            {
                // remove obsolete whitespaces around the operators
                // first in front of the operator
                while (nPos_2 > 0 && sReturn[nPos_2 - 1] == ' ')
                {
                    sReturn = sReturn.substr(0, nPos_2 - 1) + sReturn.substr(nPos_2);
                    nPos_2--;
                }

                // after the operator
                while (nPos_2 < sReturn.length() - 2 && sReturn[nPos_2 + 1] == ' ')
                {
                    sReturn = sReturn.substr(0, nPos_2 + 1) + sReturn.substr(nPos_2 + 2);
                }

                // Identify exponents and ignore them
                if ((i == 1 || i == 2)
                        && (sReturn[nPos_2 - 1] == 'e' || sReturn[nPos_2 - 1] == 'E'))
                {
                    if ((int)sReturn[nPos_2 - 2] <= (int)'9' && (int)sReturn[nPos_2 - 2] >= (int)'0'
                            && (int)sReturn[nPos_2 + 1] <= (int)'9' && (int)sReturn[nPos_2 + 1] >= (int)'0')
                    {
                        nPos = nPos_2 + 1;
                        continue;
                    }
                }

                // Don't insert whitespaces directly after opening parentheses and commas
                if ((i == 1 || i == 2)
                        && (sReturn[nPos_2 - 1] == '(' || sReturn[nPos_2 - 1] == '[' || sReturn[nPos_2 - 1] == '{' || sReturn[nPos_2 - 1] == ',' || !nPos_2))
                {
                    if (i == 2)
                    {
                        sReturn.insert(nPos_2, 1, '-');
                        nPos_2++;
                    }
                    nPos = nPos_2 + 1;
                    continue;
                }
            }
            else if (i > 3 && sReturn[nPos_2 + sCodepage[i][0].length()] == '_')
            {
                // Wird das Token von '_' abgeschlossen? Pruefen wir, ob es von vorne auch begrenzt ist <--
                if (!checkDelimiter(sReturn.substr(nPos_2 - 1, sCodepage[i][0].length() + 1) + " "))
                {
                    // --> Nein? Den Positionsindex um die Laenge des Tokens weitersetzen <--
                    nPos = nPos_2 + sCodepage[i][0].length();
                    continue;
                }
            }
            else if (i > 2 && !checkDelimiter(sReturn.substr(nPos_2 - 1, sCodepage[i][0].length() + 2)))
            {
                // --> Pruefen wir auch getrennt den Fall, ob das Token ueberhaupt begrenzt ist ('_' zaehlt nicht zu den Delimitern) <--
                nPos = nPos_2 + sCodepage[i][0].length();
                continue;
            }

            // --> Das war alles nicht der Fall? Schieb den Index um die Laenge der Ersetzung weiter <--
            nPos_2 += sCodepage[i][1].length();

            // --> Kopiere den Teil nach dem Token in sTemp <--
            sTemp = sReturn.substr(sReturn.find(sCodepage[i][0], nPos) + sCodepage[i][0].length());

            // --> Kopiere den Teil vor dem Token, die Ersetzung und sTemp in sReturn <--
            sReturn = sReturn.substr(0, sReturn.find(sCodepage[i][0], nPos)) + sCodepage[i][1] + sTemp;

            // --> Setze den Hauptindex auf nPos_2 <--
            nPos = nPos_2;
        }
    }

    // --> Ersetze nun lange Indices "_INDEX" durch "_{INDEX}" <--
    handleTeXIndicesAndExponents(sReturn, sCodepage, nSymbols);

    // --> Setze nun den Hauptindex zurueck <--
    nPos = 0;

    // --> Pruefe nun kompliziertere Tokens: zuerst die Wurzel "sqrt()" <--
    while (sReturn.find("sqrt(", nPos) != string::npos)
    {
        // --> Speichere die Position der Klammer <--
        nPos_2 = sReturn.find("sqrt(", nPos) + 4;

        // --> Kopiere den Teil ab der Klammer in sTemp <--
        sTemp = sReturn.substr(sReturn.find("sqrt(", nPos) + 4);

        // --> Kopiere den Teil vor "sqrt(" in sReturn und haenge "@{\\sqrt{" an <--
        sReturn = sReturn.substr(0, sReturn.find("sqrt(", nPos)) + "@{\\sqrt{";

        // --> Haenge sTemp an sReturn an, wobei die passende schliessende Klammer durch '}}' ersetzt werden muss <--
        sReturn += sTemp.substr(1, getMatchingParenthesis(sTemp) - 1) + "}}" + sTemp.substr(getMatchingParenthesis(sTemp) + 1);

        // --> nPos auf nPos_2 setzen <--
        nPos = nPos_2;
    }

    // --> Nun "norm(x,y,z,...)" <--
    while (sReturn.find("norm(", nPos) != string::npos)
    {
        // --> Speichere die Position der Klammer <--
        nPos_2 = sReturn.find("norm(", nPos) + 4;

        // --> Kopiere den Teil ab der Klammer in sTemp <--
        sTemp = sReturn.substr(sReturn.find("norm(", nPos) + 4);

        // --> Kopiere den Teil vor "norm(" in sReturn und haenge "|" an <--
        sReturn = sReturn.substr(0, sReturn.find("norm(", nPos)) + "|";

        // --> Haenge sTemp an sReturn an, wobei die passende schliessende Klammer durch '|' ersetzt werden muss <--
        sReturn += sTemp.substr(1, getMatchingParenthesis(sTemp) - 1) + "|" + sTemp.substr(getMatchingParenthesis(sTemp) + 1);

        // --> nPos auf nPos_2 setzen <--
        nPos = nPos_2;
    }

    // --> Nun "abs(x,y,z,...)" <--
    while (sReturn.find("abs(", nPos) != string::npos)
    {
        // --> Speichere die Position der Klammer <--
        nPos_2 = sReturn.find("abs(", nPos) + 3;

        // --> Kopiere den Teil ab der Klammer in sTemp <--
        sTemp = sReturn.substr(sReturn.find("abs(", nPos) + 3);

        // --> Kopiere den Teil vor "abs(" in sReturn und haenge "|" an <--
        sReturn = sReturn.substr(0, sReturn.find("abs(", nPos)) + "|";

        // --> Haenge sTemp an sReturn an, wobei die passende schliessende Klammer durch '|' ersetzt werden muss <--
        sReturn += sTemp.substr(1, getMatchingParenthesis(sTemp) - 1) + "|" + sTemp.substr(getMatchingParenthesis(sTemp) + 1);

        // --> nPos auf nPos_2 setzen <--
        nPos = nPos_2;
    }

    // --> Hauptindex zuruecksetzen <--
    nPos = 0;

    // --> Ersetzte nun lange Hochzahlen "^(ZAHL)" durch "^{ZAHL}" <--
    while (sReturn.find("^(", nPos) != string::npos)
    {
        nPos_2 = sReturn.find("^(", nPos) + 1;
        sTemp = sReturn.substr(nPos_2);
        sReturn = sReturn.substr(0, nPos_2) + "{";
        sReturn += sTemp.substr(1, getMatchingParenthesis(sTemp) - 1) + "}" + sTemp.substr(getMatchingParenthesis(sTemp) + 1);

        nPos = nPos_2;
    }

    // --> Entferne die Leerzeichen am Anfang und Ende und gib sReturn zurueck <--
    StripSpaces(sReturn);
    return sReturn;
}


// This is a static helper function for the findCommand function further down
static Match findCasualCommand(const string& sCmd)
{
    Match _mMatch;
    _mMatch.sString = "";
    _mMatch.nPos = string::npos;
    size_t nStart = 0;

    // Jump over breakpoints
    if (sCmd.substr(0,2) == "|>")
        nStart = 2;

    // Go through the complete command line
    for (unsigned int i = nStart; i < sCmd.length(); i++)
    {
        // Break the loop, if one recognizes typical initializers of the parameter list
        if ((sCmd.substr(i, 2) == "--" || sCmd.substr(i, 5) == "-set ") && !isInQuotes(sCmd, i))
            break;

        // Jump over some special characters, if one didn't find any command yet
        if ((sCmd[i] == ' ' || sCmd[i] == '\t' || sCmd[i] == '-' || sCmd[i] == '=') && _mMatch.nPos == string::npos)
            continue;
        else if ((sCmd[i] == ' ' || sCmd[i] == '-' || sCmd[i] == ';' || sCmd[i] == ':') && _mMatch.nPos != string::npos)
        {
            // If we found a command start, then we examine these characters further
            if (sCmd[i] != '-' && sCmd.find_first_not_of(' ', i) != string::npos && sCmd[sCmd.find_first_not_of(' ', i)] == '=')
            {
                // This is probably no commmand, because there's a equal sign following it
                // Reset the match position and continue
                _mMatch.nPos = string::npos;
                continue;
            }

            // Store the command string
            _mMatch.sString = sCmd.substr(_mMatch.nPos, i - _mMatch.nPos);

            // Ensure that this is not inside of quotation marks
            if (isInQuotes(sCmd, (i - _mMatch.nPos) / 2))
            {
                _mMatch.sString = "";
                _mMatch.nPos = string::npos;
            }

            // Return the found match
            return _mMatch;
        }
        else if ((sCmd[i] == '(') && _mMatch.nPos != string::npos)
        {
            // We found an opening parenthesis although we already found a match position
            // Store the command string
            _mMatch.sString = sCmd.substr(_mMatch.nPos, i - _mMatch.nPos);

            // Ensure that this is not inside of quotation marks
            // Also ensure that the found command string is one of the
            // control flow commands (the only ones, which require parentheses)
            if (isInQuotes(sCmd, (i - _mMatch.nPos) / 2)
                    || (_mMatch.sString != "if"
                        && _mMatch.sString != "elseif"
                        && _mMatch.sString != "switch"
                        && _mMatch.sString != "for"
                        && _mMatch.sString != "while"))
            {
                _mMatch.sString = "";
                _mMatch.nPos = string::npos;
            }

            // Return the found match
            return _mMatch;
        }

        // No other case invoked. The current character
        // is a candidate for a command string match
        if (_mMatch.nPos == string::npos)
            _mMatch.nPos = i;
    }

    // Special case: We walked through the complete command line and didn't find
    // the end of the command. Simply use the rest of the command line as the command
    if (_mMatch.nPos != string::npos)
        _mMatch.sString = sCmd.substr(_mMatch.nPos);
    else
        _mMatch.sString = sCmd;

    // Return the found match
    return _mMatch;
}

// This is a static helper function for the findCommandWithReturnValue function further down
static bool findShortestMatchForCommand(Match& _mMatch, size_t position, char character, const string& sCmd, const string& sCommand)
{
    // Store the command string and the match position
    _mMatch.sString = sCmd.substr(position, sCmd.find(character, position + sCommand.length()) - position);
    _mMatch.nPos = position;

    // If there's a whitespace in the match string, erase it end everything after it
    if (_mMatch.sString.find(' ') != string::npos)
        _mMatch.sString.erase(_mMatch.sString.find(' '));

    // Ensure that the found command is a single word
    if (!isInQuotes(sCmd, position)
        && ((position && checkDelimiter(sCmd.substr(position - 1, _mMatch.sString.length() + 2)))
            || (!position && checkDelimiter(" " + sCmd.substr(position, _mMatch.sString.length() + 1)))))
    {
        // If the command line is longer than the match position and the length of both strings
        if (sCmd.length() >= sCommand.length() + _mMatch.nPos + _mMatch.sString.length())
        {
            // Try to find the command further back by calling findCommand recursively
            Match _mTemp = findCommand(sCmd.substr(_mMatch.nPos + _mMatch.sString.length()), sCommand);

            // If a match was found and it's length is shorter than the current one
            // then use the new match
            if (_mTemp.sString.length()
                    && _mTemp.sString.length() < _mMatch.sString.length())
            {
                _mMatch.nPos += _mTemp.nPos + _mMatch.sString.length();
                _mMatch.sString = _mTemp.sString;
            }
        }

        // Return true
        return true;
    }
    else
    {
        // It is not a single word
        // reset the match and return false
        _mMatch.sString = "";
        _mMatch.nPos = string::npos;
        return false;
    }
}

// This is a static helper function for the findCommand function further down
static Match findCommandWithReturnValue(const string& sCmd, const string& sCommand)
{
    Match _mMatch;
    _mMatch.sString = "";
    _mMatch.nPos = string::npos;
    size_t nStart = 0;

    // Jump over breakpoints
    if (sCmd.substr(0,2) == "|>")
        nStart = 2;

    // Go through the complete command line
    for (unsigned int i = nStart; i < sCmd.length(); i++)
    {
        // Break the loop, if we find typical parameter string initializers
        if ((sCmd.substr(i, 2) == "--" || sCmd.substr(i, 5) == "-set ") && !isInQuotes(sCmd, i))
            break;

        // Jump over some special characters
        if (sCmd[i] == ' ' || sCmd[i] == '\t' || sCmd[i] == '(')
            continue;

        // Jump, if this is not our searched command
        if (sCmd.substr(i, sCommand.length()) != sCommand)
            continue;

        // This is our command and it is not at the beginning of the line
        if (sCmd.substr(i, sCommand.length()) == sCommand && i)
        {
            // Is the command filling the rest of the command line?
            if (i + sCommand.length() == sCmd.length() - 1)
            {
                // Store the command string with the previous character
                _mMatch.sString = sCmd.substr(i - 1) + " ";

                // Store the position
                _mMatch.nPos = i;

                // Ensure that the command is not part of a larger word
                if (checkDelimiter(_mMatch.sString) && !isInQuotes(sCmd, i))
                {
                    // It is not
                    // Remove the additional characters
                    _mMatch.sString = _mMatch.sString.substr(1, _mMatch.sString.length() - 2);
                }
                else
                {
                    // It is
                    // Reset the match
                    _mMatch.sString = "";
                    _mMatch.nPos = string::npos;
                }

                // return the found match
                return _mMatch;
            }

            // Continue, if the command ends with a parenthesis and there's another match
            // Otherwise reset the match
            if (sCmd[i + sCommand.length()] == '(' && sCmd.find(sCommand, i + 1) != string::npos)
                continue;
            else if (sCmd[i + sCommand.length()] == '(' && sCmd.find(sCommand, i + 1) == string::npos)
            {
                // Reset the match
                _mMatch.sString = "";
                _mMatch.nPos = string::npos;

                // Return the resetted match
                return _mMatch;
            }

            // There's a minus sign after the command
            if (sCmd.find('-', i + sCommand.length()) != string::npos)
            {
                // If the match is not a single word, continue
                if (!findShortestMatchForCommand(_mMatch, i, '-', sCmd, sCommand))
                    continue;
                return _mMatch;
            }

            // There's a whitespace after the command
            if (sCmd.find(' ', i + sCommand.length()) != string::npos)
            {
                // If the match is not a single word, continue
                if (!findShortestMatchForCommand(_mMatch, i, ' ', sCmd, sCommand))
                    continue;
                return _mMatch;
            }
        }

        // This is our command and it is at the beginning of the line
        if (sCmd.substr(i, sCommand.length()) == sCommand && !i)
        {
            // If the command lenght and the command line length are nearly the same
            if (sCommand.length() == sCmd.length() - 1)
            {
                // Store string and match position
                // Add surrounding characters
                _mMatch.sString = " " + sCommand.substr(0, sCommand.length() + 1);
                _mMatch.nPos = 0;

                // Ensure that the found string is not part of a larger word
                if (checkDelimiter(_mMatch.sString) && !isInQuotes(sCmd, i))
                {
                    // It is not
                    // remove the additional characters
                    _mMatch.sString = _mMatch.sString.substr(1, _mMatch.sString.length() - 2);
                }
                else
                {
                    // It is
                    // Reset the match and continue
                    _mMatch.sString = "";
                    _mMatch.nPos = string::npos;
                    continue;
                }

                // Return the found match
                return _mMatch;
            }

            // Continue, if the command ends with a parenthesis and there's another match
            // Otherwise reset the match
            if (sCmd[i + sCommand.length()] == '(' && sCmd.find(sCommand, i + 1) != string::npos)
                continue;
            else if (sCmd[i + sCommand.length()] == '(' && sCmd.find(sCommand, i + 1) == string::npos)
            {
                _mMatch.sString = "";
                _mMatch.nPos = string::npos;
                return _mMatch;
            }

            // There's a minus sign after the command
            if (sCmd.find('-', i + sCommand.length()) != string::npos)
            {
                // If the match is not a single word, continue
                if (!findShortestMatchForCommand(_mMatch, i, '-', sCmd, sCommand))
                    continue;
                return _mMatch;
            }

            // There's a whitespace after the command
            if (sCmd.find(' ', i + sCommand.length()) != string::npos)
            {
                // If the match is not a single word, continue
                if (!findShortestMatchForCommand(_mMatch, i, ' ', sCmd, sCommand))
                    continue;
                return _mMatch;
            }
        }
    }

    // Return the found match
    return _mMatch;
}



// This function is very important for the command handler.
// It extracts the command out of the complete command line.
// It is also capable of finding an command, which is not at the start of the line (i.e.
// commands, which are returning values)
Match findCommand(const string& sCmd, string sCommand)
{
    Match _mMatch;
    _mMatch.sString = "";
    _mMatch.nPos = string::npos;

    // There are two possible use cases:
    // If one defines a command, for which this function shall look for,
    // then it will also find commands after other special characters
    if (!sCommand.length())
    {
        // find a causal command
        return findCasualCommand(sCmd);
    }
    else if (sCommand.length() && (sCmd == sCommand || sCmd == sCommand + " "))
    {
        // the command line is identical to the searched command
        _mMatch.sString = sCmd;
        StripSpaces(_mMatch.sString);
        _mMatch.nPos = 0;

        // Return the found match
        return _mMatch;
    }
    else if (sCommand.length()
             && sCmd.find(sCommand) != string::npos
             && sCmd.find(' ', sCmd.find(sCommand)) != string::npos
             && findCommand(sCmd).sString != "help"
             && findCommand(sCmd).sString != "edit"
             && findCommand(sCmd).sString != "new")
    {
        // find a command with a return value
        return findCommandWithReturnValue(sCmd, sCommand);
    }

    // Nothing was found
    return _mMatch;
}


// --> extrahiert den gesamten Kommandostring aus einer Kommandozeile <--
string extractCommandString(const string& sCmd, const Match& _mMatch)
{
    string sCommandString = "";

    // if not match was found, return an empty string
    if (_mMatch.nPos == string::npos)
        return "";

    // A match is available
    if (_mMatch.nPos)
    {
        // go through the string inversely. Start at the match position
        for (int i = _mMatch.nPos; i >= 0; i--)
        {
            // An opening parenthesis was found, which is not part of a string
            if (sCmd[i] == '(' && !isInQuotes(sCmd, i))
            {
                // Find the matching parenthesis
                if (getMatchingParenthesis(sCmd.substr(i)) != string::npos)
                {
                    // Use the contents of the parenthesis
                    // However, only extract the command and not the characters in front of it
                    sCommandString = sCmd.substr(_mMatch.nPos, getMatchingParenthesis(sCmd.substr(i)) - (_mMatch.nPos - i + 1));
                    break;
                }
                else
                    throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sCmd, i);
            }
        }
    }

    // If the command string length is zero, use the complete command string
    // from the position to the end
    if (!sCommandString.length())
        sCommandString = sCmd.substr(_mMatch.nPos);

    // Return the extracted string
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
                sToClear[i] = ' ';
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
void openExternally(const string& sFile)
{
    /* --> Dies simuliert im Wesentlichen einen cd zur Datei, den Aufruf mit dem anderen
     *     Programm und die Rueckkehr zum alten Pfad (NumeRe-Stammverzeichnis) <--
     */

    std::string _sFile;

    int nErrorCode = 0;

    // Add quotation marks if there are none and convert the string to a character pointer
    if (_sFile[0] != '"')
        _sFile = "\"" + sFile + "\"";
    else
        _sFile = sFile;

    replaceAll(_sFile, "/", "\\");

    // Invoke the Windows shell
    nErrorCode = (int)ShellExecute(nullptr, "open", sFile.c_str(), nullptr, nullptr, SW_SHOWNORMAL);

    // Examine the return value
    if (nErrorCode <= 32)
    {
        if (nErrorCode == SE_ERR_NOASSOC)
            throw SyntaxError(SyntaxError::EXTERNAL_PROGRAM_NOT_FOUND, "", SyntaxError::invalid_position, sFile);
        else
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, "", SyntaxError::invalid_position, sFile);
    }
}


/////////////////////////////////////////////////
/// \brief Move a file from one to another
/// location. Can be used for renaming as well.
///
/// \param sFile const string&
/// \param sNewFileName const string&
/// \return void
///
/////////////////////////////////////////////////
void moveFile(const string& sFile, const string& sNewFileName)
{
    // Copy first
    copyFile(sFile, sNewFileName);

    // remove old file
    remove(sFile.c_str());
}


/////////////////////////////////////////////////
/// \brief Copy the file's contents to another
/// file.
///
/// \param sFile const string&
/// \param sTarget const string&
/// \return void
///
/////////////////////////////////////////////////
void copyFile(const string& sFile, const string& sTarget)
{
    // Open two file streams to copy the contents
    ifstream File(sFile.c_str(), ios_base::binary);
    ofstream NewFile(sTarget.c_str(), ios_base::binary);

    if (!File.good())
        throw SyntaxError(SyntaxError::CANNOT_OPEN_SOURCE, "", SyntaxError::invalid_position, sFile);

    if (!NewFile.good())
        throw SyntaxError(SyntaxError::CANNOT_OPEN_TARGET, "", SyntaxError::invalid_position, sTarget);

    // Copy the file
    NewFile << File.rdbuf();
}


// --> Generiert eine TeX-Hauptdatei fuer eine gegebene TikZ-Plot-Datei <--
void writeTeXMain(const string& sTeXFile)
{
    string sTemp = sTeXFile;

    // Extract the path
    if (sTemp.find('\\') != string::npos || sTemp.find('/') != string::npos)
    {
        if (sTemp.find('\\') != string::npos)
            sTemp = sTemp.substr(sTemp.rfind('\\') + 1);
        if (sTemp.find('/') != string::npos)
            sTemp = sTemp.substr(sTemp.rfind('/') + 1);
    }

    // --> Fuege vor ".tex" den String "main"  ein <--
    ofstream TexMain((sTeXFile.substr(0, sTeXFile.find(".tex")) + "main.tex").c_str());
    if (!TexMain.good())
        throw SyntaxError(SyntaxError::CANNOT_OPEN_TARGET, "", SyntaxError::invalid_position, sTeXFile.substr(0, sTeXFile.find(".tex")) + "main.tex");

    // Write the files contents
    TexMain << "\\documentclass{scrartcl}    % KOMA-SCRIPT-KLASSE (Kann durch \"article\" ersetzt werden)" << endl << endl;
    TexMain << "% Ein paar hilfreiche Packages:" << endl;
    TexMain << "\\usepackage[utf8]{inputenc} % Sonderzeichen in der Eingabe" << endl;
    TexMain << "\\usepackage[T1]{fontenc}    % Sonderzeichen in der Ausgabe" << endl;
    TexMain << "\\usepackage[ngerman]{babel} % Deutsch als Dokumentsprache" << endl;
    TexMain << "\\usepackage{mathpazo}       % Palatino als Schriftart (passend zum Plot)" << endl;
    TexMain << "\\usepackage{tikz}           % TikZ fuer die eigentliche Graphik" << endl;
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
        if (sReturn[nPos - 1] != '\\')
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


/////////////////////////////////////////////////
/// \brief This static function detects, whether
/// the current string expression is continued.
///
/// \param sCmd const std::string&
/// \param pos size_t
/// \return size_t
///
/////////////////////////////////////////////////
static size_t isStringContinuation(const std::string& sCmd, size_t pos)
{
    if (sCmd.substr(pos, 2) == "  ")
        return std::string::npos;

    size_t nextChar = sCmd.find_first_not_of(' ', pos+1);

    if (nextChar == std::string::npos || sCmd[nextChar] != '+')
        return std::string::npos;

    nextChar++;

    if (sCmd.length() > nextChar && sCmd[nextChar] != ' ')
        return nextChar;

    nextChar++;

    if (sCmd.length() > nextChar && sCmd[nextChar] != ' ')
        return nextChar;

    return std::string::npos;
}


/////////////////////////////////////////////////
/// \brief This static function is a helper for
/// getArgAtPos to parse the expressions in the
/// string.
///
/// \param sArg std::string&
/// \param flags int
/// \return void
///
/////////////////////////////////////////////////
static void parseArg(std::string& sArg, int flags)
{
    NumeReKernel* instance = NumeReKernel::getInstance();

    // Function call
    if (!instance->getDefinitions().call(sArg))
        throw SyntaxError(SyntaxError::FUNCTION_ERROR, sArg, "");

    // String evaluation
    if (instance->getStringParser().isStringExpression(sArg))
    {
        std::string dummy;

        NumeRe::StringParser::StringParserRetVal _ret = instance->getStringParser().evalAndFormat(sArg, dummy, true);

        if (_ret == NumeRe::StringParser::STRING_SUCCESS)
            return;
    }

    if (flags & ARGEXTRACT_ASSTRING)
        return;

    // Read data
    if (instance->getMemoryManager().containsTablesOrClusters(sArg))
        getDataElements(sArg, instance->getParser(), instance->getMemoryManager(), instance->getSettings());

    // Numerical evaluation
    instance->getParser().SetExpr(sArg);

    int results;
    int nPrec = instance->getSettings().getPrecision();
    mu::value_type* v = instance->getParser().Eval(results);

    sArg.clear();

    for (int i = 0; i < results; i++)
    {
        if (sArg.length())
            sArg += ",";

        if (flags & ARGEXTRACT_ASINT)
            sArg += toString(intCast(v[i]));
        else
            sArg += toString(v[i], nPrec);
    }
}


/////////////////////////////////////////////////
/// \brief Extracts a options value at the
/// selected position and applies automatic
/// parsing, if necessary.
///
/// \param sCmd const string&
/// \param nPos unsigned int
/// \param extraction int
/// \return string
///
/////////////////////////////////////////////////
string getArgAtPos(const string& sCmd, unsigned int nPos, int extraction)
{
    string sArgument = "";

    // If the position is greater than the string length
    // return an empty string
    if (nPos >= sCmd.length())
        return "";

    // Jump over whitespaces
    while (nPos < sCmd.length() && sCmd[nPos] == ' ')
        nPos++;

    // Ensure that the position is smaller than the length of the string
    if (nPos >= sCmd.length())
        return "";

    size_t nQuotes = 0;

    for (size_t i = nPos; i < sCmd.length(); i++)
    {
        if (sCmd[i] == '"' && (!i || sCmd[i - 1] != '\\'))
            nQuotes++;

        if (nQuotes % 2)
            continue;

        if (sCmd[i] == '(' || sCmd[i] == '[' || sCmd[i] == '{')
            i += getMatchingParenthesis(sCmd.substr(i));

        if (sCmd[i] == ' ')
        {
            size_t cont = isStringContinuation(sCmd, i);

            if (cont < sCmd.length() && NumeReKernel::getInstance()->getStringParser().isStringExpression(sCmd.substr(nPos, i-nPos)))
                i = cont-1;
            else
            {
                sArgument = sCmd.substr(nPos, i-nPos);
                break;
            }
        }
        else if (i+1 == sCmd.length())
            sArgument = sCmd.substr(nPos);
    }

    // Parse the argument, if necessary
    if (extraction & ARGEXTRACT_PARSED)
        parseArg(sArgument, extraction);

    // Strip the argument, if necessary
    if (extraction & ARGEXTRACT_STRIPPED)
    {
        StripSpaces(sArgument);

        if (sArgument.front() == '"' && sArgument.back() == '"')
            sArgument = sArgument.substr(1, sArgument.length()-2);
    }

    // return the found option value
    return sArgument;
}

// --> Pruefen wir, ob die Position in dem String von Anfuehrungszeichen umgeben ist <--
bool isInQuotes(StringView sExpr, unsigned int nPos, bool bIgnoreVarParser /* = false*/)
{
    int nQuotes = 0;

    // --> Zaehlt schlicht und einfach die Anfuehrungszeichen <--
    for (size_t i = 0; i < nPos; i++)
    {
        // Parse the special string_cast function
        if (sExpr.subview(i, 12) == "string_cast(" && i + 12 <= nPos)
        {
            // Argument of "string_cast"?
            if (getMatchingParenthesis(sExpr.subview(i + 11)) + i + 11 > nPos)
                return true;
            else
                i += getMatchingParenthesis(sExpr.subview(i + 11)) + 11;
        }

        // Count the quotation marks
        if (sExpr[i] == '"' && (!i || sExpr[i-1] != '\\'))
            nQuotes++;
    }

    // Simplest case: The number of quotation marks is odd
    // Therefore the position is inside of quotation marks
    if (nQuotes % 2) // nQuotes % 2 == 1, wenn eine ungerade Zahl an Anfuehrungszeichen aufgetreten ist => die Position befindet sich als hinter einem geoeffneten Anfuehrungszeichen.
        return true;

    // Shall the variable to string parser be examined, too?
    if (!bIgnoreVarParser)
    {
        // No variable to string parser was found
        if (sExpr.rfind('#', nPos) == string::npos)
            return false;

        // Ensure that the variable to string parser is not part of quotation marks itself
        // Do this by calling the function on its position recursively
        if (isInQuotes(sExpr, sExpr.rfind('#', nPos), true))
            return false;

        // If it is not part of quotation marks, examine the contents of the
        // variable to string parser (i.e. everything between "#" and the current position)
        for (unsigned int i = sExpr.rfind('#', nPos); i < nPos; i++)
        {
            // Parenthesis is found
            // Jump over it
            if (sExpr[i] == '(')
            {
                // Has no closing parenthesis -> the closing one must be after the current position
                if (getMatchingParenthesis(sExpr.subview(i, nPos - i)) == string::npos)
                    return true;

                // Advance the index variabel
                i += getMatchingParenthesis(sExpr.subview(i, nPos - i));

                // Probably we're now at the current position. Then
                // this is the closing parenthesis of the variable to string parser
                if (i == nPos)
                    return true;
                continue;
            }

            // Whitespaces, commas, string concatenators (plus sign) and closing parentheses
            // are indicators for the end of the variable to string arguments. This means
            // that the current position is not part of any string
            if (sExpr[i] == ' ' || sExpr[i] == '+' || sExpr[i] == ',' || sExpr[i] == ')')
                return false;
        }

        // Exclude special characters, which are the terminators of the variable to string parser by themselves
        if (nPos < sExpr.length() - 1 && (sExpr[nPos] == ',' || sExpr[nPos] == '+' || sExpr[nPos] == ' ' || sExpr[nPos] == ')'))
            return false;
        else if (nPos == sExpr.length() - 1 && sExpr[nPos] == ')')
            return false;
        else
            return true;

    }

    // return false
    return false;
}

// This function determines, whether the current position is part of an argument of the three
// value to string conversion functions
bool isToStringArg(const string& sExpr, unsigned int nPos)
{
    // Ensure that at least one of the three functions is available in the string
    if (sExpr.find("valtostr(") == string::npos && sExpr.find("to_string(") == string::npos && sExpr.find("string_cast(") == string::npos)
        return false;

    // Go inversely through the string
    for (int i = nPos; i >= 8; i--)
    {
        // An opening parenthesis was found with its counterpart after the current position
        if (sExpr[i] == '(' && getMatchingParenthesis(sExpr.substr(i)) + i > nPos)
        {
            // Is there one of the three functions left from the current position?
            if (i > 10 && sExpr.substr(i - 11, 12) == "string_cast(")
                return true;
            else if (i > 8 && sExpr.substr(i - 9, 10) == "to_string(")
                return true;
            else if (sExpr.substr(i - 8, 9) == "valtostr(")
                return true;
            else if (isDelimiter(sExpr[i - 1]))
                continue; // There's a delimiter left of the parenthesis, continue
            else
                return false;
        }
    }

    // No part of any of the functions' arguments
    return false;
}


/////////////////////////////////////////////////
/// \brief Casts doubles to integers and avoids
/// rounding errors.
///
/// \param number double
/// \return long long int
///
/////////////////////////////////////////////////
long long int intCast(double number)
{
    // if quite close, use rint
    if (fabs(number - rint(number)) < 1e-7)
        return rint(number);
    // otherwise truncate
    return static_cast<int>(number);
}


/////////////////////////////////////////////////
/// \brief Casts the real part of the complex
/// number to an integer and avoids rounding
/// errors.
///
/// \param number const std::complex<double>&
/// \return long long int
///
/////////////////////////////////////////////////
long long int intCast(const std::complex<double>& number)
{
    return intCast(number.real());
}


// This function determines, if the passed character is a delimiter character
bool isDelimiter(char cChar)
{
    // Only construct the string once
    static string sDelimiter = "+-*/ ^&|!%<>,=\\#?:;()[]{}\"";

    // Try to find the current character
    if (sDelimiter.find(cChar) != string::npos)
        return true;
    return false;
}

// --> Ergaenzt fehlende Legenden mit den gegebenen Ausdruecken <--
bool addLegends(string& sExpr)
{
    // Validate the number of parentheses
    if (!validateParenthesisNumber(sExpr))
        throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sExpr, '(');

    EndlessVector<std::string> args = getAllArguments(sExpr);

    std::string sTemp;

    for (size_t i = 0; i < args.size(); i++)
    {
        /* --> Nun koennte es sein, dass bereits eine Legende angegeben worden ist. Dabei gibt es drei
         *     Moeglichkeiten: entweder durch umschliessende Anfuehrungszeichen, durch eine vorangestellte
         *     Raute '#' oder auch beides. Wir muessen hier diese drei Faelle einzeln behandeln <--
         * --> Ebenfalls ist es natuerlich moeglich, dass gar keine Legende angegeben worden ist. Das behandeln
         *     wir im ELSE-Fall <--
         */
        if (args[i].find('"') != std::string::npos)
        {
            /* --> Hier ist auf jeden Fall '"' vorhanden. Es ist aber nicht gesagt, dass '#' nicht auch
             *     zu finden ist <--
             * --> Speichern wir zunaechst die Position des '"' in nPos <--
             */
            size_t nPos = args[i].find('"') + 1;

            // --> Pruefe nun, ob in diesem Stringbereich ein zweites '"' zu finden ist <--
            if (args[i].find('"', nPos) != std::string::npos)
            {
                // --> Ja? Gibt's denn dann eine Raute? <--
                if (args[i].find('#', nPos) != std::string::npos)
                {
                    // --> Ja? Dann muessen wir (zur Vereinfachung an anderer Stelle) noch zwei Anfuehrungszeichen ergaenzen <--
                    sTemp += args[i] + "+\"\"";
                }
                else
                    sTemp += args[i];
            }
            else
                return false;   // Nein? Dann ist irgendwas ganz Falsch: FALSE zurueckgeben!
        }
        else if (args[i].find('#') != string::npos)
        {
            /* --> Hier gibt's nur '#' und keine '"' (werden im ersten Fall schon gefangen). Speichern wir
             *     die Position der Raute in nPos <--
             */
            size_t nPos = args[i].find('#');

            /* --> Setze sExpr dann aus dem Teil vor nPos und, wenn noch mindestens Komma ab nPos gefunden werden kann,
             *     dem Teil ab nPos vor dem Komma, dem String '+""' und dem Teil ab dem Komma zusammen, oder, wenn kein
             *     Komma gefunden werden kann, dem Teil nach nPos und dem String '+""' zusammen <--
             * --> An dieser Stelle bietet sich der Ternary (A ? x : y) tatsaechlich einmal an, da er die ganze Sache,
             *     die sonst eine temporaere Variable benoetigt haette, in einem Befehl erledigen kann <--
             */
            for (size_t j = nPos; j < args[i].length(); j++)
            {
                if (args[i][j] == '(')
                    j += getMatchingParenthesis(args[i].substr(j));

                if (args[i][j] == ' ')
                {
                    sTemp += args[i].insert(j, "+\"\"");
                    break;
                }
                else if (j+1 == args[i].length())
                {
                    sTemp += args[i] + "+\"\"";
                    break;
                }
            }
        }
        else
        {
            /* --> Hier gibt's weder '"' noch '#'; d.h., wir muessen die Legende selbst ergaenzen <--
             * --> Schneiden wir zunaechst den gesamten Ausdurck zwischen den zwei Kommata heraus <--
             */
            std::string sLabel = args[i];

            // --> Entfernen wir ueberzaehlige Leerzeichen <--
            StripSpaces(sLabel);

            /* --> Setzen wir den gesamten Ausdruck wieder zusammen, wobei wir den Ausdruck in
             *     Anfuehrungszeichen als Legende einschieben <--
             */
            sTemp += args[i] + " \"" + sLabel + "\"";
        }

        if (i+1 < args.size())
            sTemp += ", ";
    }

    sExpr = sTemp;

    return true;
}

// --> Prueft, ob der erste und der letzte Char eines strings zu den Delimitern gehoert: z.B. zur Variablen-/Tokendetektion <--
bool checkDelimiter(const string& sString, bool stringdelim)
{
    // --> Gib die Auswertung dieses logischen Ausdrucks zurueck <--
    return isDelimiter(sString.front()) && (isDelimiter(sString.back()) || (sString.back() == '.' && stringdelim));
}

// --> Funktion, die die Laenge der Zeile anhand der festgelegten Fensterdimensionen bestimmt und die Zeilenumbrueche automatisch erzeugt <--
string LineBreak(string sOutput, const Settings& _option, bool bAllowDashBreaks, int nFirstIndent, int nIndent)
{
    unsigned int nLastLineBreak = 0;     // Variable zum Speichern der Position des letzten Zeilenumbruchs
    string sIndent = "\n|";     // String fuer den Einzug der 2. und der folgenden Zeilen
    //cerr << sOutput << endl;
    sOutput = toSystemCodePage(sOutput);
    // --> Falls der string kuerzer als die Zeilenlaenge ist, braucht nichts getan zu werden <--
    if (sOutput.length() < _option.getWindow() - nFirstIndent && sOutput.find('$') == string::npos && sOutput.find("\\n") == string::npos && sOutput.find('\n') == string::npos)
        return sOutput;

    // --> Ergaenze den Einzug um die noetige Zahl an Leerstellen <--
    for (int i = 1; i < nIndent; i++)
    {
        sIndent += " ";
    }

    if ((sOutput.substr(0, 4) == "|   " || sOutput.substr(0, 4) == "|-> ") && nFirstIndent == 4)
        nFirstIndent = 0;

    // --> Laufe alle Zeichen des strings ab <--
    for (unsigned int i = 1; i < sOutput.length(); i++)
    {
        /* --> Stolpere ich ueber ein "$"? Dann muss hier ein Zeilenumbruch hin. Damit muss der Index des
         *     letzten Zeilenumbruchs aktualisiert werden <--
         */
        if (sOutput[i] == '$' && sOutput[i - 1] != '\\')
            nLastLineBreak = i;
        if (sOutput[i] == 'n' && sOutput[i - 1] == '\\')
        {
            if ((i == 1 || sOutput[i - 2] != '\\')
                    && !(sOutput.substr(i, 2) == "nu" && checkDelimiter(sOutput.substr(i - 1, 4)))
                    && !(sOutput.substr(i, 3) == "neq" && checkDelimiter(sOutput.substr(i - 1, 5))))
                nLastLineBreak = i;
            else if (i != 1 && sOutput[i - 2] != '\\')
                sOutput.insert(i, "\\");
        }
        if (sOutput[i] == '\n')
        {
            nLastLineBreak = i;
        }
        // --> Ist die maximale Zeilenlaenge erreicht? Dann muss ein Zeilenumbruch eingefuegt werden <--
        if ((i == _option.getWindow() - nFirstIndent && !nLastLineBreak)
                || (nLastLineBreak && i - nLastLineBreak == _option.getWindow() - nIndent))
        {
            // --> Laufe von hier ab rueckwaerts und suche nach entweder: 1 Leerstelle oder 1 Minus-Zeichen (wenn erlaubt) oder dem "$" <--
            for (unsigned int j = i; j > nLastLineBreak; j--)
            {
                if (sOutput[j] == ' ')
                {
                    if (sOutput[j - 1] == '\\')
                    {
                        sOutput.insert(j + 1, "$");
                        nLastLineBreak = j + 1;
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
                            (sOutput[j - 1] == ' '
                             || sOutput[j - 1] == '('
                             || sOutput[j + 1] == ')'
                             || sOutput[j - 1] == '['
                             || (sOutput[j + 1] >= '0' && sOutput[j + 1] <= '9')
                             || sOutput[j + 1] == ','
                             || (sOutput[j + 1] == '"' && sOutput[j - 1] == '"')
                            ))
                        continue;
                    sOutput.insert(j + 1, 1, '\n');   // Insert a linebreak
                    nLastLineBreak = j + 1;
                    break;
                }
                else if (sOutput[j] == ',' && bAllowDashBreaks && sOutput[j + 1] != ' ' && j != i)
                {
                    sOutput[j] = '%';
                    nLastLineBreak = j + 1;
                    break;
                }
                else if (sOutput[j] == '$' && sOutput[j - 1] != '\\') // --> Hier ist auf jeden Fall ein Zeilenumbruch gewuenscht <--
                {
                    nLastLineBreak = j;
                    break;
                }
                if (j - 1 == nLastLineBreak)
                {
                    string sDelim = "+-*/";
                    for (unsigned int n = i; n > nLastLineBreak; n--)
                    {
                        if (sDelim.find(sOutput[n]) != string::npos)
                        {
                            sOutput = sOutput.substr(0, n) + '$' + sOutput.substr(n);
                            nLastLineBreak = n;
                            break;
                        }
                        if (n - 1 == nLastLineBreak)
                        {
                            sOutput = sOutput.substr(0, i - 1) + '$' + sOutput.substr(i - 1);
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
        if (sOutput[i] == '$' && sOutput[i - 1] != '\\')
        {
            // --> Ersetze '$' durch den oben erstellten Einzug <--
            sOutput.replace(i, 1, sIndent);
            i += sIndent.length() - 1;
            continue;
        }
        else if (sOutput[i] == '\n' && (i > sOutput.length() - sIndent.length() || sOutput.substr(i, sIndent.length()) != sIndent))
        {
            sOutput.replace(i, 1, sIndent);
            i += sIndent.length();
            continue;
        }
        else if (sOutput[i] == 'n' && sOutput[i - 1] == '\\' && sOutput[i - 2] != '\\')
            sOutput = sOutput.substr(0, i - 1) + sIndent + sOutput.substr(i + 1);
        else if (sOutput[i] == 'n' && sOutput[i - 1] == '\\' && sOutput[i - 2] == '\\')
            sOutput.erase(i - 1, 1);
        else if (sOutput[i] == '$' && sOutput[i - 1] == '\\')
        {
            sOutput.erase(i - 1, 1);
        }
        else if (sOutput[i] == '%' && bAllowDashBreaks)
        {
            // --> Ersetze '%' durch ',' und den oben erstellten Einzug, falls erlaubt <--
            sOutput = sOutput.substr(0, i) + "," + sIndent + sOutput.substr(i + 1);
        }
    }
    return sOutput;
}

// --> Linearisiert die Funktion zwischen zwei Punkten (x_0,y_0) und (x_1,y_1) und gibt den Schnittpunkt mit der x-Achse zurueck <--
double Linearize(double x_0, double y_0, double x_1, double y_1)
{
    double b = y_0;
    double m = (y_1 - y_0) / (x_1 - x_0);
    // y = m*x + b ==> x = 1/m*(y-b) ==> x = -b/m fuer y = 0.0
    return x_0 - b / m;
}

// This function gets the first argument in the passed argument list
// if the boolean bCut is true then the argument is erased from the string
static StringView getNextCommandLineToken(StringView& sArgList, char cSep)
{
    if (!sArgList.length())
        return StringView();

    size_t nPos = 0;
    size_t nMatching = 0;
    size_t nQuotes = 0;

    // Go through the complete string
    for (size_t i = 0; i < sArgList.length(); i++)
    {
        if (sArgList[i] == '"' && (!i || sArgList[i-1] != '\\'))
            nQuotes++;

        if (nQuotes % 2)
            continue;

        // Jump over parentheses
        if ((sArgList[i] == '(' || sArgList[i] == '[' || sArgList[i] == '{' ) && (nMatching = getMatchingParenthesis(sArgList.subview(i))) != string::npos)
            i += nMatching;

        // A comma was found -> break the loop
        if (sArgList[i] == cSep)
        {
            nPos = i;
            break;
        }
    }

    // If no comma was found, simply use the complete string
    if (!nPos && sArgList[0] != cSep)
        nPos = sArgList.length();

    // If the comma was at the first position, then return nothing
    if (!nPos)
    {
        if (sArgList[0] == cSep)
            sArgList.trim_front(1);
        return StringView();
    }

    // Get the first argument
    StringView sArg = sArgList.subview(0, nPos);
    sArg.strip();

    sArgList.trim_front(nPos+1);

    // return the first argument
    return sArg;
}

string getNextArgument(string& sArgList, bool bCut)
{
    StringView argList(sArgList);
    string sArg = getNextCommandLineToken(argList, ',').to_string();

    if (bCut)
        sArgList = argList.to_string();

    return sArg;
}

string getNextIndex(string& sArgList, bool bCut)
{
    StringView argList(sArgList);
    string sArg = getNextCommandLineToken(argList, ':').to_string();

    if (bCut)
        sArgList = argList.to_string();

    return sArg;
}

string getNextSemiColonSeparatedToken(string& sArgList, bool bCut)
{
    StringView argList(sArgList);
    string sArg = getNextCommandLineToken(argList, ';').to_string();

    if (bCut)
        sArgList = argList.to_string();

    return sArg;
}

StringView getNextViewedArgument(StringView& sView)
{
    return getNextCommandLineToken(sView, ',');
}

StringView getNextViewedIndex(StringView& sView)
{
    return getNextCommandLineToken(sView, ':');
}


/////////////////////////////////////////////////
/// \brief Splits up the complete argument list
/// and returns them as an EndlessVector.
///
/// \param sArgList StringView
/// \return EndlessVector<StringView>
///
/////////////////////////////////////////////////
EndlessVector<StringView> getAllArguments(StringView sArgList)
{
    EndlessVector<StringView> vArgs;

    while (sArgList.length())
        vArgs.push_back(getNextViewedArgument(sArgList));

    return vArgs;
}


/////////////////////////////////////////////////
/// \brief Splits up the complete argument list
/// and returns them as an EndlessVector.
///
/// \param sArgList string
/// \return EndlessVector<string>
///
/////////////////////////////////////////////////
EndlessVector<string> getAllArguments(string sArgList)
{
    EndlessVector<string> vArgs;

    while (sArgList.length())
        vArgs.push_back(getNextArgument(sArgList, true));

    return vArgs;
}

/////////////////////////////////////////////////
/// \brief Splits up the complete index list
/// and returns them as an EndlessVector.
///
/// \param sArgList string
/// \return EndlessVector<string>
///
/////////////////////////////////////////////////
EndlessVector<string> getAllIndices(string sArgList)
{
    EndlessVector<string> vIndices;

    while (sArgList.length())
        vIndices.push_back(getNextIndex(sArgList, true));

    return vIndices;
}

/////////////////////////////////////////////////
/// \brief Splits up the complete index list
/// and returns them as an EndlessVector.
///
/// \param sArgList string
/// \return EndlessVector<string>
///
/////////////////////////////////////////////////
EndlessVector<string> getAllSemiColonSeparatedTokens(string sArgList)
{
    EndlessVector<string> vIndices;

    while (sArgList.length())
        vIndices.push_back(getNextSemiColonSeparatedToken(sArgList, true));

    return vIndices;
}

// Wrapper for the static member function of the kernel
void make_progressBar(int nStep, int nFirstStep, int nFinalStep, const string& sType)
{
    NumeReKernel::progressBar(nStep, nFirstStep, nFinalStep, sType);
    return;
}

static bool containsStringClusters(const string& sLine)
{
    const map<string,NumeRe::Cluster>& mClusterMap = NumeReKernel::getInstance()->getMemoryManager().getClusterMap();

    for (auto iter = mClusterMap.begin(); iter != mClusterMap.end(); ++iter)
    {
        if (iter->second.isString())
        {
            size_t pos = sLine.find(iter->first + "{");

            if (pos != string::npos && (!pos || isDelimiter(sLine[pos-1])))
                return true;
        }
    }

    return false;
}

// This function checks, whether the passed expression contains strings or valtostring parser
bool containsStrings(const string& sLine)
{
    if (!sLine.length())
        return false;

    // It's only necessary to check the following functions.
    // All other string functions need strings as input
    if (sLine.find('"') != string::npos
            || sLine.find('#') != string::npos
            || sLine.find("string(") != string::npos
            || sLine.find("string_cast(") != string::npos
            || sLine.find("char(") != string::npos
            || sLine.find("valtostr(") != string::npos)
        return true;

    return containsStringClusters(sLine);

    //return false;
}

// This function checks, whether the file with the passed file name exists
bool fileExists(const string& sFilename)
{
    if (sFilename.length())
    {
        string _sFile = sFilename;
        _sFile = fromSystemCodePage(_sFile);
        // Open the ifstream (ifstream doesn't create a file)
        ifstream ifFile(_sFile.c_str());
        return ifFile.good(); // If the stream is good(), the file exists
    }
    else
        return false;
}

// This function erases option tokens from a parameter string
void eraseToken(string& sExpr, const string& sToken, bool bTokenHasValue)
{
    unsigned int nLength = sToken.length();

    // If the option token has a value, then the erase process is a bit more complex
    if (bTokenHasValue)
    {
        // Is the token actually available?
        if (!findParameter(sExpr, sToken, '='))
            return;

        // Search for the option value
        for (unsigned int i = findParameter(sExpr, sToken, '=') + nLength - 1; i < sExpr.length(); i++)
        {
            // Assignment operator found
            if (sExpr[i] == '=')
            {
                // Find the end of the option value
                for (unsigned int j = sExpr.find_first_not_of("= ", i); j < sExpr.length(); j++)
                {
                    // jump over parentheses
                    if (!isInQuotes(sExpr, j) && (sExpr[j] == '(' || sExpr[j] == '[' || sExpr[j] == '{'))
                        j += getMatchingParenthesis(sExpr.substr(j));

                    // White space found -> end of option value
                    // erase token and its value together
                    if (sExpr[j] == ' ')
                    {
                        sExpr.erase(findParameter(sExpr, sToken, '=') - 1, j - findParameter(sExpr, sToken, '=') + 1);
                        return;
                    }
                }
            }
        }
    }
    else
    {
        // Is the token actually available?
        if (!findParameter(sExpr, sToken))
            return;

        // Simply erase the token
        sExpr.erase(findParameter(sExpr, sToken) - 1, nLength);
    }
    return;
}

// This function resolves the possibility to select multiple paths at once by inserting something
// like this/is/a/<path|with|tokens>/which/will/search/at/different/locations
vector<string> resolveChooseTokens(const string& sDirectory, const Settings& _option)
{
    vector<string> vResolved;
    vResolved.push_back(sDirectory);
    string sToken;
    unsigned int nSize = 0, nth_choose = 0;
    bool bResolvingPath = false;

    // Is there at least one pipe in the directory?
    if (sDirectory.find('|') != string::npos)
    {
        // As long as the directory contains pipes
        while (vResolved[0].find('|') != string::npos)
        {
            // no opening angle for the token?
            if (!vResolved[0].rfind('<'))
                break;

            // Get the token and remove the remaining part
            sToken = vResolved[0].substr(vResolved[0].rfind('<') + 1);
            sToken.erase(sToken.find('>'));

            // Store the current size of the directory tree
            nSize = vResolved.size();
            nth_choose = 0;

            // As long as a pipe is found in the token or the token has a length
            while (sToken.find('|') != string::npos || sToken.length())
            {
                // so lange ein "|" in dem Token gefunden wird, muss der Baum dupliziert werden
                if (sToken.find('|') != string::npos)
                {
                    // duplicate the "root" tree
                    for (unsigned int i = 0; i < nSize; i++)
                        vResolved.push_back(vResolved[i + nth_choose * nSize]);
                }

                // Replace the tokens with the first of the current tokens
                for (unsigned int i = nth_choose * nSize; i < (nth_choose + 1)*nSize; i++)
                {
                    if (!bResolvingPath && vResolved[i].rfind('/') != string::npos && vResolved[i].rfind('/') > vResolved[i].rfind('>'))
                        bResolvingPath = true;
                    vResolved[i].replace(vResolved[i].rfind('<'), vResolved[i].rfind('>') + 1 - vResolved[i].rfind('<'), sToken.substr(0, sToken.find('|')));
                }

                // If we want to resolve a path, then we have to do that with a recursion
                if (bResolvingPath
                        && ((vResolved[nth_choose * nSize].find('*') != string::npos && vResolved[nth_choose * nSize].find('*') < vResolved[nth_choose * nSize].rfind('/'))
                            || (vResolved[nth_choose * nSize].find('?') != string::npos && vResolved[nth_choose * nSize].find('?') < vResolved[nth_choose * nSize].rfind('/'))))
                {
                    // Platzhalter in Pfaden werden mit einer Rekursion geloest.
                    // Resolve the current tree
                    vector<string> vFolderList = getFolderList(vResolved[nth_choose * nSize].substr(0, vResolved[nth_choose * nSize].rfind('/')), _option, 1);

                    // Remove obsolete paths (i.e. paths pointing to itself or to one directory further up
                    for (unsigned int j = 0; j < vFolderList.size(); j++)
                    {
                        if ((vFolderList[j].length() >= 3 && vFolderList[j].substr(vFolderList[j].length() - 3) == "/..")
                            || (vFolderList[j].length() >= 2 && vFolderList[j].substr(vFolderList[j].length() - 2) == "/."))
                        {
                            vFolderList.erase(vFolderList.begin() + j);

                            // If we erase the current position, we have to decrement the position
                            j--;
                        }
                    }

                    // If we didn't get a result, remove the current token from the token list and continue
                    if (!vFolderList.size())
                    {
                        bResolvingPath = false;
                        nth_choose++;
                        if (sToken.find('|') != string::npos)
                            sToken.erase(0, sToken.find('|') + 1);
                        else
                        {
                            sToken.clear();
                            break;
                        }
                        continue;
                    }

                    // Copy the obtained tree to the resolved tree
                    for (unsigned int j = 0; j < vFolderList.size(); j++)
                    {
                        // Does the tree need to be duplicated?
                        if (vFolderList.size() > 1 && j < vFolderList.size() - 1)
                        {
                            // ggf. Baum duplizieren
                            if (vResolved.size() > (nth_choose + 1)*nSize)
                            {
                                for (unsigned int k = 0; k < nSize; k++)
                                {
                                    vResolved.push_back(vResolved[k + (nth_choose + 1)*nSize]);
                                    vResolved[k + (nth_choose + 1)*nSize] = vResolved[k + nth_choose * nSize];
                                }
                            }
                            else
                            {
                                for (unsigned int k = 0; k < nSize; k++)
                                {
                                    vResolved.push_back(vResolved[(nth_choose)*nSize]);
                                }
                            }
                        }

                        // simply replace the path part of the resolved tree
                        for (unsigned int k = nth_choose * nSize; k < (nth_choose + 1)*nSize; k++)
                        {
                            vResolved[k].replace(0, vResolved[k].rfind('/'), vFolderList[j]);
                        }

                        // Increment the choose token counter
                        if (vFolderList.size() > 1 && j < vFolderList.size() - 1)
                            nth_choose++;
                    }
                }

                // Erase the current token from the token list and continue
                bResolvingPath = false;
                nth_choose++;
                if (sToken.find('|') != string::npos)
                    sToken.erase(0, sToken.find('|') + 1);
                else
                {
                    sToken.clear();
                    break;
                }
            }
        }
    }

    // This is not using path tokens but place holders/wildcards in the path part
    if (vResolved[0].find('/') != string::npos
            && ((vResolved[0].find('*') != string::npos && vResolved[0].find('*') < vResolved[0].rfind('/'))
                || (vResolved[0].find('?') != string::npos && vResolved[0].find('?') < vResolved[0].rfind('/'))))
    {
        // Platzhalter in Pfaden werden mit einer Rekursion geloest.
        vector<string> vFolderList = getFolderList(vResolved[0].substr(0, vResolved[0].rfind('/')), _option, 1);

        // store the current tree size
        nSize = vResolved.size();

        // Remove obsolete paths (i.e. paths pointing to itself or to one directory further up
        for (unsigned int j = 0; j < vFolderList.size(); j++)
        {
            if ((vFolderList[j].length() >= 3 && vFolderList[j].substr(vFolderList[j].length() - 3) == "/..")
                || (vFolderList[j].length() >= 2 && vFolderList[j].substr(vFolderList[j].length() - 2) == "/."))
            {
                vFolderList.erase(vFolderList.begin() + j);

                // If we erase the current position, we have to decrement the position
                j--;
            }
        }

        // Return, if no result was found
        if (!vFolderList.size())
            return vResolved;

        // Copy the resolved tree, if it is necessary
        for (unsigned int i = 0; i < vFolderList.size() - 1; i++)
        {
            // Don't use paths, which weren't resolved
            if (vFolderList[i].find('*') != string::npos || vFolderList[i].find('?') != string::npos || !vFolderList[i].size())
                continue;

            // ggf. Baum duplizieren
            for (unsigned int k = 0; k < nSize; k++)
            {
                vResolved.push_back(vResolved[k]);
            }

        }

        // Replace the paths with wildcards with the results obtained by recursion
        for (unsigned int j = 0; j < vFolderList.size(); j++)
        {
            // Don't use paths, which weren't resolved
            if (vFolderList[j].find('*') != string::npos || vFolderList[j].find('?') != string::npos || !vFolderList[j].size())
                continue;

            // replace the paths in the resolved tree
            for (unsigned int k = j * nSize; k < (j + 1)*nSize; k++)
            {
                vResolved[k].replace(0, vResolved[k].rfind('/'), vFolderList[j]);
            }
        }
    }

    // return the resolved tree
    return vResolved;
}

// This function is a static helper function for getFileList and getFolderList
static HANDLE initializeFileHandle(string& sDir, WIN32_FIND_DATA* FindFileData, const Settings& _option)
{
    HANDLE hFind = INVALID_HANDLE_VALUE;
    string sPath;

    // Initialize the Windows structures
    if (sDir[0] == '.')
    {
        // Only a dot -> root path
        hFind = FindFirstFile((_option.getExePath() + "\\" + sDir).c_str(), FindFileData);
        sDir = replacePathSeparator(_option.getExePath() + "/" + sDir);
        sDir.erase(sDir.rfind('/') + 1);
    }
    else if (sDir[0] == '<')
    {
        // Get the default paths
        if (sDir.substr(0, 10) == "<loadpath>")
        {
            sPath = _option.getLoadPath() + sDir.substr(sDir.find('>') + 1);
        }
        else if (sDir.substr(0, 10) == "<savepath>")
        {
            sPath = _option.getSavePath() + sDir.substr(sDir.find('>') + 1);
        }
        else if (sDir.substr(0, 12) == "<scriptpath>")
        {
            sPath = _option.getScriptPath() + sDir.substr(sDir.find('>') + 1);
        }
        else if (sDir.substr(0, 10) == "<plotpath>")
        {
            sPath = _option.getPlotPath() + sDir.substr(sDir.find('>') + 1);
        }
        else if (sDir.substr(0, 10) == "<procpath>")
        {
            sPath = _option.getProcPath() + sDir.substr(sDir.find('>') + 1);
        }
        else if (sDir.substr(0, 2) == "<>")
        {
            sPath = _option.getExePath() + sDir.substr(sDir.find('>') + 1);
        }
        else if (sDir.substr(0, 6) == "<this>")
        {
            sPath = _option.getExePath() + sDir.substr(sDir.find('>') + 1);
        }
        else if (sDir.substr(0, 4) == "<wp>")
        {
            sPath = _option.getWorkPath() + sDir.substr(sDir.find('>') + 1);
        }

        // If the path has a length then initialize the file handle
        if (sPath.length())
        {
            hFind = FindFirstFile(sPath.c_str(), FindFileData);
            sDir = replacePathSeparator(sPath);
            sDir.erase(sDir.rfind('/') + 1);
        }
    }
    else
    {
        // an arbitrary path
        hFind = FindFirstFile(sDir.c_str(), FindFileData);
        if (sDir.find('/') != string::npos)
            sDir.erase(sDir.rfind('/') + 1);
    }

    // return the initialized file handle
    return hFind;
}

// This file returns a list of files (including their paths, if nFlags & 1)
vector<string> getFileList(const string& sDirectory, const Settings& _option, int nFlags)
{
    vector<string> vFileList;
    vector<string> vDirList;

    // Replace the Windows-Style Path separators to Unix-Style
    string sDir = replacePathSeparator(sDirectory);

    // Get the resolved tree
    vDirList = resolveChooseTokens(sDir, _option);

    // Walk through the resolved tree
    for (unsigned int i = 0; i < vDirList.size(); i++)
    {
        sDir = vDirList[i];

        // Append a wildcard, if one is missing
        if (sDir.rfind('.') == string::npos && sDir.find('*') == string::npos && sDir.find('?') == string::npos)
        {
            if (sDir[sDir.find_last_not_of(' ')] != '/')
                sDir += '/';
            sDir += "*";
        }
        else if ((sDir.find('.') == string::npos
                  || (sDir.find('.') != string::npos && sDir.find('/', sDir.rfind('.')) != string::npos))
                 && sDir.back() != '*')
            sDir += "*";

        // Declare the Windows structures;
        WIN32_FIND_DATA FindFileData;
        HANDLE hFind = initializeFileHandle(sDir, &FindFileData, _option);

        // Ensure that the structures were initialized correctly
        if (hFind == INVALID_HANDLE_VALUE)
            continue;

        // As long as the FindNextFile function returns non-zero
        // read the contents of FindFileData
        do
        {
            // Ignore directories
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                continue;

            // Push back filenames
            if (nFlags & 1)
                vFileList.push_back(sDir + FindFileData.cFileName);
            else
                vFileList.push_back(FindFileData.cFileName);
        }
        while (FindNextFile(hFind, &FindFileData) != 0);

        // Close the handle
        FindClose(hFind);
    }

    // Return the obtained file list
    return vFileList;
}

// This file returns a list of directories (including their paths, if nFlags & 1)
vector<string> getFolderList(const string& sDirectory, const Settings& _option, int nFlags)
{
    vector<string> vFileList;
    vector<string> vDirList;

    // Replace the Windows-Style Path separators to Unix-Style
    string sDir = replacePathSeparator(sDirectory);

    // Get the resolved tree
    vDirList = resolveChooseTokens(sDir, _option);

    // Walk through the resolved tree
    for (unsigned int i = 0; i < vDirList.size(); i++)
    {
        sDir = vDirList[i];

        // Append a wildcard, if one is missing
        if (sDir.rfind('.') == string::npos && sDir.find('*') == string::npos && sDir.find('?') == string::npos)
        {
            if (sDir[sDir.find_last_not_of(' ')] != '/')
                sDir += '/';
            sDir += "*";
        }
        else if ((sDir.find('.') == string::npos
                  || (sDir.find('.') != string::npos && sDir.find('/', sDir.rfind('.')) != string::npos))
                 && sDir.back() != '*')
            sDir += "*";

        // Declare the Windows structures;
        WIN32_FIND_DATA FindFileData;
        HANDLE hFind = initializeFileHandle(sDir, &FindFileData, _option);

        // Ensure that the structures were initialized correctly
        if (hFind == INVALID_HANDLE_VALUE)
            continue;

        // As long as the FindNextFile function returns non-zero
        // read the contents of FindFileData
        do
        {
            // USe directories
            if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                // Push back the directories
                if (nFlags & 1)
                    vFileList.push_back(sDir + FindFileData.cFileName);
                else
                    vFileList.push_back(FindFileData.cFileName);
            }
            else // ignore files
                continue;

        }
        while (FindNextFile(hFind, &FindFileData) != 0);

        // Close the handle
        FindClose(hFind);
    }

    // Return the obtained file list
    return vFileList;
}

// This function is used to reduce the log file size from >= 100.000 to 20.000 lines
void reduceLogFilesize(const string& sFileName)
{
    fstream fFile;
    size_t nLines = 0;
    string sTemp;
    const size_t MAXLINES = 100000;
    const size_t MINLINES = 20000;

    // Open the logfile and check, whether the file stream is OK
    fFile.open(sFileName.c_str(), ios_base::binary | ios_base::in);
    if (fFile.fail())
        return;

    // Count the lines in the log file
    while (!fFile.eof())
    {
        getline(fFile, sTemp);
        nLines++;
    }

    // Clear the file state and return to the first position
    fFile.clear();
    fFile.seekg(0);

    // If the number of lines is larger than the predefined lines
    if (nLines >= MAXLINES)
    {
        fstream fTemp;

        // Open a temporary file in binary mode and check, whether the file is good
        fTemp.open("$~tempfile.txt", ios_base::binary | ios_base::out);
        if (fTemp.fail())
            return;

        // Copy the last 20.000 lines of the log file to the temporary file
        for (unsigned int i = 0; i < nLines; i++)
        {
            getline(fFile, sTemp);
            if (nLines - i > MINLINES)
                continue;
            fTemp << sTemp << endl;
        }

        // Close the file streams
        fFile.close();
        fTemp.close();

        // Re-open the file streams with exchanged read/write flags
        // and a truncate flag on the log file
        fTemp.open("$~tempfile.txt", ios_base::binary | ios_base::in);
        fFile.open(sFileName.c_str(), ios_base::trunc | ios_base::binary | ios_base::out);
        fTemp.seekg(0);

        // Copy the entire content of the temporary file
        // (i.e. the last 20.000 lines of the original log file)
        fFile << fTemp.rdbuf();

        // Close the file streams and remove the temporary file
        fFile.close();
        fTemp.close();
        remove("$~tempfile.txt");
    }
    return;
}

// This function is a variable initializer for the function replaceToVector name
// It will only be called once
static void OprtRplc_setup(map<string, string>& mOprtRplc)
{
    mOprtRplc["("] = "[";
    mOprtRplc[")"] = "]";
    mOprtRplc[":"] = "~";
    mOprtRplc[","] = "_";
    mOprtRplc["."] = "_";
    mOprtRplc["+"] = "\\p\\";
    mOprtRplc["-"] = "\\m\\";
    mOprtRplc["*"] = "\\ml\\";
    mOprtRplc["/"] = "\\d\\";
    mOprtRplc["^"] = "\\e\\";
    mOprtRplc["{"] = "\\ob\\";
    mOprtRplc["}"] = "\\cb\\";
    mOprtRplc["&"] = "\\a\\";
    mOprtRplc["|"] = "\\o\\";
    mOprtRplc["%"] = "\\md\\";
    mOprtRplc["!"] = "\\n\\";
    mOprtRplc["="] = "\\eq\\";
    mOprtRplc[">"] = "\\g\\";
    mOprtRplc["<"] = "\\l\\";
    mOprtRplc["?"] = "\\q\\";
    return;
}

// This function replaces a data access expression (i.e. the contents of the object argument parentheses)
// into a vector name, which is valid for the parser
string replaceToVectorname(const string& sExpression)
{
    string sVectorName = sExpression;
    static map<string, string> mOprtRplc;

    // If the map wasn't initialized do that now
    if (!mOprtRplc.size())
        OprtRplc_setup(mOprtRplc);

    // Remove whitespaces
    while (sVectorName.find(' ') != string::npos)
        sVectorName.erase(sVectorName.find(' '), 1);

    // Replace mathematical operators
    for (auto iter = mOprtRplc.begin(); iter != mOprtRplc.end(); ++iter)
    {
        while (sVectorName.find(iter->first) != string::npos)
            sVectorName.replace(sVectorName.find(iter->first), (iter->first).length(), iter->second);
    }

    if (sVectorName.find('[') == string::npos)
        sVectorName += "[]";

    // return the new vector name
    return "_~" + sVectorName;
}

// This function accesses the windows clipboard and tries to convert the
// contents into usual text
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
    char* pszText = static_cast<char*>( GlobalLock(hData) );
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


// This function is a static helper function for evalRecursiveExpressions further down
static bool handleRecursiveOperators(string& sExpr, size_t& nPos, size_t& nArgSepPos)
{
    size_t nLength = 2;
    if (sExpr.substr(nPos, 3) == "**=")
        nLength = 3;
    // Do we find another comma?
    if (sExpr.find(',', nPos) != string::npos)
    {
        int nQuotes = 0;

        // Go through the remaining expression and try to find the end
        // of the current expression part
        for (unsigned int j = nPos; j < sExpr.length(); j++)
        {
            // Jump over parentheses
            if (!(nQuotes % 2)
                && (sExpr[j] == '(' || sExpr[j] == '[' || sExpr[j] == '{'))
                j += getMatchingParenthesis(sExpr.substr(j));

            // Count the not escaped parentheses
            if (sExpr[j] == '"')
            {
                if (j && sExpr[j - 1] == '\\')
                    continue;
                nQuotes++;
            }

            // continue, if we're inside of quotation marks
            if (nQuotes % 2)
                continue;

            // Do we find a comma or did we reach the end of the whole expression?
            if (sExpr[j] == ',' || j + 1 == sExpr.length())
            {
                // Do we have a comma now?
                if (j + 1 != sExpr.length())
                {
                    // The comma case
                    if (!nArgSepPos)
                    {
                        sExpr = sExpr.substr(0, nPos)
                                + " = "
                                + sExpr.substr(0, nPos)
                                + sExpr.substr(nPos, nLength-1)
                                + "("
                                + sExpr.substr(nPos + nLength, j - nPos - nLength)
                                + ") "
                                + sExpr.substr(j);
                    }
                    else
                    {
                        sExpr = sExpr.substr(0, nPos)
                                + " = "
                                + sExpr.substr(nArgSepPos + 1, nPos - nArgSepPos - 1)
                                + sExpr.substr(nPos, nLength-1)
                                + "("
                                + sExpr.substr(nPos + nLength, j - nPos - nLength)
                                + ") "
                                + sExpr.substr(j);
                    }
                }
                else
                {
                    // The end-of-expression case
                    if (!nArgSepPos)
                    {
                        sExpr = sExpr.substr(0, nPos)
                                + " = "
                                + sExpr.substr(0, nPos)
                                + sExpr.substr(nPos, nLength-1)
                                + "("
                                + sExpr.substr(nPos + nLength)
                                + ") ";
                    }
                    else
                    {
                        sExpr = sExpr.substr(0, nPos)
                                + " = "
                                + sExpr.substr(nArgSepPos + 1, nPos - nArgSepPos - 1)
                                + sExpr.substr(nPos, nLength-1)
                                + "("
                                + sExpr.substr(nPos + nLength)
                                + ") ";
                    }

                    // return false means: this is the end of the expression
                    return false;
                }

                // Go through the expression and try to find the next argument separator
                for (unsigned int k = nPos; k < sExpr.length(); k++)
                {
                    // Jump over parentheses
                    if (!(nQuotes % 2)
                        && (sExpr[k] == '(' || sExpr[k] == '[' || sExpr[k] == '{'))
                        k += getMatchingParenthesis(sExpr.substr(k));

                    // Count the quotation marks, which are not escaped
                    if (sExpr[k] == '"')
                    {
                        if (k && sExpr[k - 1] == '\\')
                            continue;
                        nQuotes++;
                    }

                    // Contine, if we're inside of quotation marks
                    if (nQuotes % 2)
                        continue;

                    // Do we find a comma?
                    // Update all index variables and break
                    // the current loop
                    if (sExpr[k] == ',')
                    {
                        nArgSepPos = k;
                        nPos = k;
                        break;
                    }
                }

                // break this loop
                break;
            }
        }
    }
    else
    {
        // Simple case: no further comma
        if (!nArgSepPos)
        {
            sExpr = sExpr.substr(0, nPos)
                    + " = "
                    + sExpr.substr(0, nPos)
                    + sExpr.substr(nPos, nLength-1)
                    + "("
                    + sExpr.substr(nPos + nLength)
                    + ")";
        }
        else
        {
            sExpr = sExpr.substr(0, nPos)
                    + " = "
                    + sExpr.substr(nArgSepPos + 1, nPos - nArgSepPos - 1)
                    + sExpr.substr(nPos, nLength-1)
                    + "("
                    + sExpr.substr(nPos + nLength)
                    + ")";
        }

        // return false means: was last expression
        return false;
    }

    // return true means: continue the detection of recursive expressions
    return true;
}

// This function is a static helper function for evalRecursiveExpressions further down
static void handleIncAndDecOperators(string& sExpr, size_t nPos, size_t nArgSepPos)
{
    if (!nArgSepPos)
    {
        sExpr = sExpr.substr(0, nPos)
                + " = "
                + sExpr.substr(0, nPos)
                + sExpr[nPos]
                + "1"
                + sExpr.substr(nPos + 2);
    }
    else
    {
        sExpr = sExpr.substr(0, nPos)
                + " = "
                + sExpr.substr(nArgSepPos + 1, nPos - nArgSepPos - 1)
                + sExpr[nPos]
                + "1"
                + sExpr.substr(nPos + 2);
    }
}

// This function converts the recursive assignments like VAR += VAL into VAR = VAR + (VAL)
void evalRecursiveExpressions(string& sExpr)
{
    // Ignore flow control blocks
    if (sExpr.substr(0, 3) == "if "
            || sExpr.substr(0, 3) == "if("
            || sExpr.substr(0, 7) == "elseif "
            || sExpr.substr(0, 7) == "elseif("
            || sExpr.substr(0, 5) == "else "
            || sExpr.substr(0, 7) == "switch "
            || sExpr.substr(0, 7) == "switch("
            || sExpr.substr(0, 5) == "case "
            || sExpr.substr(0, 8) == "default:"
            || sExpr.substr(0, 4) == "for "
            || sExpr.substr(0, 4) == "for("
            || sExpr.substr(0, 6) == "while "
            || sExpr.substr(0, 6) == "while(")
        return;

    unsigned int nArgSepPos = 0;
    int nQuotes = 0;
    bool bAnswerSuppressor = false;

    // Find the trailing semicolon
    if (sExpr.find_last_not_of(" \t") != string::npos && sExpr[sExpr.find_last_not_of(" \t")] == ';')
    {
        sExpr.erase(sExpr.find_last_not_of(" \t"));
        bAnswerSuppressor = true;
    }

    // Go through the complete expression
    for (unsigned int i = 0; i < sExpr.length(); i++)
    {
        // Jump over parentheses
        if (!(nQuotes % 2)
            && (sExpr[i] == '(' || sExpr[i] == '{' || sExpr[i] == '['))
        {
            size_t parens = getMatchingParenthesis(sExpr.substr(i));
            if (parens != string::npos)
                i += parens;
        }

        // Count the quatation marks, which are not escaped
        if (sExpr[i] == '"')
        {
            if (i && sExpr[i - 1] == '\\')
                continue;
            nQuotes++;
        }

        // Continue, if we're inside of quotation marks
        if (nQuotes % 2) // nQuotes % 2 == 1, wenn eine ungerade Zahl an Anfuehrungszeichen aufgetreten ist => die Position befindet sich als hinter einem geoeffneten Anfuehrungszeichen.
            continue;

        // If there's a comma, store its position as
        // argument/expression separator and continue
        if (sExpr[i] == ',')
        {
            nArgSepPos = i;
            continue;
        }

        // Do we find a recursive expression?
        if (sExpr.substr(i, 2) == "+="
                || sExpr.substr(i, 2) == "-="
                || sExpr.substr(i, 3) == "**="
                || sExpr.substr(i, 2) == "*="
                || sExpr.substr(i, 2) == "/="
                || sExpr.substr(i, 2) == "^=")
        {
            // if the handler returns false, we may break the loop
            if (!handleRecursiveOperators(sExpr, i, nArgSepPos))
                break;
        }

        // Do we find a increment or decrement operator?
        if (sExpr.substr(i, 2) == "++" || sExpr.substr(i, 2) == "--")
        {
            // Handle this type of operators
            handleIncAndDecOperators(sExpr, i, nArgSepPos);
        }
    }

    // Add the trailing semicolon
    if (bAnswerSuppressor)
        sExpr += ";";

    return;
}

// This function creates a valid cache name out of a file name
string generateCacheName(const string& sFilename, Settings& _option)
{
    string sCacheName;

    // Get a valid file name
    if (sFilename.find('/') != string::npos)
        sCacheName = _option.ValidFileName(sFilename);
    else
        sCacheName = _option.ValidFileName("<loadpath>/" + sFilename);

    // Construct only once
    static string sValidChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_";

    // remove path and extensions from the filename
    if (sCacheName.find('/') != string::npos)
        sCacheName.erase(0, sCacheName.rfind('/') + 1);
    if (sCacheName.find('.') != string::npos)
        sCacheName.erase(sCacheName.rfind('.'));

    // Avoid digits at the beginning of the cache name. Add an underscore as prefix
    if (isdigit(sCacheName.front()))
        sCacheName.insert(0, 1, '_');

    // Replace all invalid characters with underscores
    for (unsigned int i = 0; i < sCacheName.length(); i++)
    {
        if (sValidChars.find(sCacheName[i]) == string::npos)
            sCacheName[i] = '_';
    }

    // Handle the special case
    if (sCacheName == "data")
        sCacheName = "loaded_data";

    // Return the new cache name
    return sCacheName;
}

// This function opens a NumeRe-Datafile file and reads the header of the file
string getFileInfo(const string& sFileName)
{
    NumeRe::NumeReDataFile file(sFileName);
    string sFileInfo;

    try
    {
        file.readFileInformation();
    }
    catch (...)
    {
        return "";
    }

    NumeRe::FileHeaderInfo info = file.getFileHeaderInformation();

    sFileInfo = toString(info.timeStamp, 0)
                + " (NDAT v" + toString(info.fileVersion, 5) + ") | "
                + toString(info.nRows) + " x " + toString(info.nCols) + " | \"" + info.sTableName + "()\"";

    // Return the read file meta information
    return sFileInfo;
}

string decodeNameSpace(string sCommandLine, const string& sThisNameSpace)
{
    string sNameSpace = "";

    if (sCommandLine.find("namespace") != string::npos)
    {
        sCommandLine.erase(0, sCommandLine.find("namespace") + 9);

        while (sCommandLine.back() == '\r' || sCommandLine.back() == '\n')
            sCommandLine.pop_back();

        StripSpaces(sCommandLine);

        if (sCommandLine.substr(0, 5) == "this~" || sCommandLine == "this")
            sCommandLine.replace(0, 4, sThisNameSpace);

        while (sCommandLine.back() == '~')
            sCommandLine.pop_back();

        if (sCommandLine != "main")
            sNameSpace = sCommandLine;
    }

    return sNameSpace;
}

// This function is used to validate the number of parentheses, i.e. whether there's
// a closing parenthesis for each opened parenthesis
bool validateParenthesisNumber(const string& sCmd)
{
    int nParCount = 0;
    int nVectCount = 0;
    int nBracketCount = 0;
    int nQuotes = 0;

    // Go through the whole string
    for (unsigned int i = 0; i < sCmd.length(); i++)
    {
        // This function counts the quotation marks by itself
        // because it's much faster
        if (sCmd[i] == '"' && (!i || sCmd[i - 1] != '\\'))
            nQuotes++;

        // If we're not in quotation marks
        if (!(nQuotes % 2))
        {
            // Count the opening and closing parentheses
            if (sCmd[i] == '(')
                nParCount++;
            else if (sCmd[i] == ')')
                nParCount--;
            else if (sCmd[i] == '{')
                nVectCount++;
            else if (sCmd[i] == '}')
                nVectCount--;
            else if (sCmd[i] == '[')
                nBracketCount++;
            else if (sCmd[i] == ']')
                nBracketCount--;
        }

        // If one of the counters is smaller than zero, something is wrong
        if (nParCount < 0 || nVectCount < 0 || nBracketCount < 0)
            return false;
    }

    // If one of the counters is nonzero, something is wrong
    return !((bool)nParCount || (bool)nVectCount || (bool)nBracketCount);
}

// This function adds quotation marks around the value of the specified parameter
void addArgumentQuotes(string& sToAdd, const string& sParam)
{
    // Check, whether the parameter exists
    if (findParameter(sToAdd, sParam, '='))
    {
        // Store the position of the equal sign of the parameter token
        addArgumentQuotes(sToAdd, findParameter(sToAdd, sParam, '=') + sParam.length());
    }
}

// This function adds quotation marks around the value of the specified parameter
void addArgumentQuotes(string& sToAdd, size_t pos)
{
    // Jump over following whitespaces
    while (sToAdd[pos] == ' ')
        pos++;

    // Only if the value doesn't contain strings
    if (!containsStrings(sToAdd.substr(pos, sToAdd.find(' ', pos) - pos)))
    {
        // Add surrounding quotation marks
        sToAdd = sToAdd.substr(0, pos)
                 + "\"" + getArgAtPos(sToAdd, pos) + "\""
                 + sToAdd.substr(sToAdd.find(' ', sToAdd.find(getArgAtPos(sToAdd, pos)) + getArgAtPos(sToAdd, pos).length()));
    }
}

// This function calculates the power of a value with the specialization that
// the exponent is an integer
double intPower(double dNumber, int nExponent)
{
    long double dResult = 1.0L;

    // An exponent of zero returns always 1
    if (!nExponent)
        return 1.0;

    // Calculuate the exponentation
    for (int i = abs(nExponent); i > 0; i--)
    {
        dResult *= (long double)dNumber;
    }

    // Apply the sign of the exponent
    if (nExponent > 0)
        return dResult;
    else
        return 1.0 / dResult;
}

std::complex<double> intPower(const std::complex<double>& dNumber, int nExponent)
{
    std::complex<double> dResult = 1.0L;

    // An exponent of zero returns always 1
    if (!nExponent)
        return 1.0;

    // Calculuate the exponentation
    for (int i = abs(nExponent); i > 0; i--)
    {
        dResult *= dNumber;
    }

    // Apply the sign of the exponent
    if (nExponent > 0)
        return dResult;
    else
        return 1.0 / dResult;
}


// This function evaluates, whether the desired position is inside of a to_cmd() function
bool isToCmd(const string& sCmd, unsigned int nPos)
{
    // Exclude border cases
    if (nPos < 6 || nPos >= sCmd.length())
        return false;
    if (sCmd.find("to_cmd(") == string::npos || sCmd.find("to_cmd(") > nPos)
        return false;

    // Go through the whole string and try to find the functions arguments
    for (int i = nPos - 6; i >= 0; i--)
    {
        if (sCmd.substr(i, 7) == "to_cmd(" && !isInQuotes(sCmd, i))
        {
            // function found -> try to find the matching parenthesis
            // If it is left of the desired position, then return true
            if (getMatchingParenthesis(sCmd.substr(i + 6)) > nPos - i - 6 && getMatchingParenthesis(sCmd.substr(i + 6)) != string::npos)
                return true;
        }
    }

    // End was reached -> return false
    return false;
}

// This function counts the number of escaped dollar signs
unsigned int countEscapeSymbols(const string& sLine)
{
    unsigned int nCount = 0;

    // Go through the line
    for (unsigned int i = 0; i < sLine.length(); i++)
    {
        // If we find a escaped dollar sign, increment the counter
        if (sLine.substr(i, 2) == "\\$")
            nCount++;
    }

    // return the number of escaped dollar signs
    return nCount;
}


// This is a static helper function for the standard qsort algorithm
static int compareDouble(const void* p1, const void* p2)
{
    if (isnan(*(double*)p1) && isnan(*(double*)p2))
        return 0;
    if (isnan(*(double*)p1))
        return 1;
    if (isnan(*(double*)p2))
        return -1;
    if (isinf(*(double*)p1) && isinf(*(double*)p2))
        return 0;
    if (isinf(*(double*)p1))
        return 1;
    if (isinf(*(double*)p2))
        return -1;
    if (*(double*)p1 < * (double*)p2)
        return -1;
    if (*(double*)p1 == *(double*)p2)
        return 0;
    if (*(double*)p1 > *(double*)p2)
        return 1;
    return 0;
}

// This is a wrapper for the standard qsort algorithm
// It returns the number of valid elements
size_t qSortDouble(double* dArray, size_t nlength)
{
    // Sort the passed array using the compareDouble helper function
    qsort((void*)dArray, nlength, sizeof(double), compareDouble);

    // Exclude the NaNs in the array
    for (int i = nlength - 1; i >= 0; i--)
    {
        if (!isnan(dArray[i]) && !isinf(dArray[i]))
            return i + 1;
    }
    return 0;
}

// This static function replaces all search-oriented methods
// in the current string variable access
static void replaceSearchMethods(string& sLine, size_t nPos, size_t nFinalPos, const string& sReplacement, const string& sMethod, string& sArgument)
{
    // Prepare the argument (use empty one or construct one
    // from argument and variable value)
    if (sArgument == "()")
        sArgument = "(" + sReplacement + ", " + sReplacement + ")";
    else if (sArgument.find(',') == string::npos)
        sArgument.insert(sArgument.length()-1, ", " + sReplacement + "");
    else
    {
        string sTemp = "(";
        sArgument.erase(0,1);
        sTemp += getNextArgument(sArgument, true);
        sTemp += ", " + sReplacement;

        if (sArgument[sArgument.find_first_not_of(' ')] == ')')
            sArgument = sTemp + ")";
        else
            sArgument = sTemp + ", " + sArgument;
    }

    // Replace the method with its standard function signature
    if (sMethod == "fnd")
        sLine.replace(nPos, nFinalPos-nPos, "strfnd" + sArgument);
    else if (sMethod == "rfnd")
        sLine.replace(nPos, nFinalPos-nPos, "strrfnd" + sArgument);
    else if (sMethod == "mtch")
        sLine.replace(nPos, nFinalPos-nPos, "strmatch" + sArgument);
    else if (sMethod == "rmtch")
        sLine.replace(nPos, nFinalPos-nPos, "strrmatch" + sArgument);
    else if (sMethod == "nmtch")
        sLine.replace(nPos, nFinalPos-nPos, "str_not_match" + sArgument);
    else if (sMethod == "nrmtch")
        sLine.replace(nPos, nFinalPos-nPos, "str_not_rmatch" + sArgument);
}

// This static function replaces all access-oriented methods
// and the string splitter in the current string variable access
static void replaceAccessMethods(string& sLine, size_t nPos, size_t nFinalPos, const string& sReplacement, const string& sMethod, string& sArgument)
{
    // Prepare the argument (use empty one or construct one
    // from argument and variable value)
    if (sArgument == "()")
        sArgument = "(" + sReplacement + ", 1)";
    else
        sArgument.insert(1, sReplacement + ", ");

    // Replace the method with its standard function signature
    if (sMethod == "at")
        sLine.replace(nPos, nFinalPos-nPos, "char" + sArgument);
    else if (sMethod == "sub")
        sLine.replace(nPos, nFinalPos-nPos, "substr" + sArgument);
    else if (sMethod == "splt")
        sLine.replace(nPos, nFinalPos-nPos, "split" + sArgument);
}

// This function searches the indicated string
// variable occurence for possible string methods and replaces
// them with the standard string function.
void replaceStringMethod(string& sLine, size_t nPos, size_t nLength, const string& sReplacement)
{
    // Does the string variable name end with a dot?
    if (sLine[nPos+nLength] != '.')
    {
        sLine.replace(nPos, nLength, sReplacement);
        return;
    }

    static const string sDELIMITER = "+-*/ ={}^&|!,\\%#?:\";";
    string sMethod = "";
    string sArgument = "";
    size_t nFinalPos = 0;

    // Find the end of the appended method. This is either
    // the closing parenthesis or a delimiter character. Split
    // the found method into its name and its argument
    for (size_t i = nPos+nLength+1; i < sLine.length(); i++)
    {
        if (sLine[i] == '(')
        {
            // Method ends with closing parenthesis
            sMethod = sLine.substr(nPos+nLength+1, i-(nPos+nLength+1));
            sArgument = sLine.substr(i, getMatchingParenthesis(sLine.substr(i))+1);
            nFinalPos = i += getMatchingParenthesis(sLine.substr(i))+1;
            break;
        }
        else if (sDELIMITER.find(sLine[i]) != string::npos)
        {
            // Method ends with a delimiter
            sMethod = sLine.substr(nPos+nLength+1, i-(nPos+nLength+1));
            nFinalPos = i;
            break;
        }
    }

    // If the argument list is empty, use an empty
    // parenthesis instead
    if (!sArgument.length())
        sArgument = "()";

    // This if-else constrct replaces the found
    // method occurences with their standard string function
    if (sMethod == "len")
    {
        // String length methods
        sLine.replace(nPos, nFinalPos-nPos, "strlen(" + sReplacement + ")");
    }
    else if (sMethod == "at" || sMethod == "sub" || sMethod == "splt")
    {
        // Access methods and splitter
        replaceAccessMethods(sLine, nPos, nFinalPos, sReplacement, sMethod, sArgument);
    }
    else if (sMethod == "fnd" || sMethod == "rfnd" || sMethod == "mtch" || sMethod == "rmtch" || sMethod == "nmtch" || sMethod == "nrmtch")
    {
        // All search-oriented methods
        replaceSearchMethods(sLine, nPos, nFinalPos, sReplacement, sMethod, sArgument);
    }
}


/////////////////////////////////////////////////
/// \brief This function will return a shortened
/// version of the data file name, where each
/// "/Path/" string part is transformed to
/// "/../".
///
/// \param sFullFileName const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string shortenFileName(const std::string& sFullFileName)
{
    size_t nPos = std::string::npos;

    std::string sFileName = replacePathSeparator(sFullFileName);

    while (sFileName.rfind('/', nPos) != string::npos)
    {
        nPos = sFileName.rfind('/', nPos);

        if (nPos != 0 && nPos-1 != ':')
        {
            size_t nPos_2 = sFileName.rfind('/', nPos-1);

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


/////////////////////////////////////////////////
/// \brief Increments a MAJOR.MINOR.BUILD version
/// string by one build count.
///
/// \param _sVer std::string
/// \return std::string
///
/////////////////////////////////////////////////
std::string incrementVersion(std::string _sVer)
{
	// Remove the dots in the version string
    for (unsigned int n = 0; n < _sVer.length(); n++)
    {
        if (_sVer[n] == '.')
        {
            _sVer.erase(n, 1);
            n--;
        }
    }

    // Increment the version by one (corresponds to
    // the build count)
    int nVersion = StrToInt(_sVer);
    nVersion++;
    _sVer = toString(nVersion);

    // Prepend zeroes, if the length is shorter than
    // three
    if (_sVer.length() < 3)
        _sVer.insert(0, 3-_sVer.length(), '0');

    // Convert the version string into the M.m.b format
    for (unsigned int n = 1; n < _sVer.length(); n++)
    {
        if (n % 2)
            _sVer.insert(n, 1, '.');
    }

    return _sVer;
}

