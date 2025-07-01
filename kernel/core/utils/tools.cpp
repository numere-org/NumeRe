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
#ifndef PARSERSTANDALONE
#include "../../kernel.hpp"
#include "../io/file.hpp"
#endif
#include <libsha.hpp>
#include <cstdlib>
#include <omp.h>
#include <windows.h>
#define SECURITY_WIN32
#include <secext.h>
#include <lmcons.h>
#include "../../../externals/stduuid/include/uuid.h"



/////////////////////////////////////////////////
/// \brief This class represents a thread safe
/// random number generator (it is a container
/// for multiple generator instances, each for
/// every possible OMP thread).
/////////////////////////////////////////////////
class ThreadsafeRandGen
{
    private:
        std::vector<std::mt19937> m_randGenArray;

    public:
        /////////////////////////////////////////////////
        /// \brief Create an instance of this thread safe
        /// random number generator by filling the
        /// internal array with the number of maximal
        /// possible OMP threads.
        /////////////////////////////////////////////////
        ThreadsafeRandGen()
        {
            double t = time(0);

            for (int i = 0; i < omp_get_max_threads(); i++)
            {
                m_randGenArray.push_back(std::mt19937(t+i));
            }
        }

        /////////////////////////////////////////////////
        /// \brief Returns an instance of a random number
        /// generator for the current thread.
        ///
        /// \return std::mt19937&
        ///
        /////////////////////////////////////////////////
        std::mt19937& getGenerator()
        {
            int threadId = omp_get_thread_num();

            if (threadId >= (int)m_randGenArray.size())
                throw std::out_of_range("Requested a random number generator for thread " + std::to_string(threadId)
                                        + ", which is larger than the number of available generators (i.e. "
                                        + std::to_string(m_randGenArray.size()) + ")");

            return m_randGenArray[threadId];
        }

};


/// File static random number generator, which will be usable via the scoped getRandGenInstance
/// function, which will return a reference to this random number generator
static ThreadsafeRandGen randGenerator;


/////////////////////////////////////////////////
/// \brief Returns a reference to the central
/// random number generator instance, which is
/// globally accessible by all random number
/// functions.
///
/// \return std::mt19937&
///
/////////////////////////////////////////////////
std::mt19937& getRandGenInstance()
{
    return randGenerator.getGenerator();
}


/////////////////////////////////////////////////
/// \brief This class represents a thread safe
/// GSL random number generator (it is a container
/// for multiple generator instances, each for
/// every possible OMP thread).
/////////////////////////////////////////////////
class ThreadsafeGslRandGen
{
    private:
        std::vector<gsl_rng*> m_randGenArray;

    public:
        /////////////////////////////////////////////////
        /// \brief Create an instance of this thread safe
        /// random number generator by filling the
        /// internal array with the number of maximal
        /// possible OMP threads.
        /////////////////////////////////////////////////
        ThreadsafeGslRandGen()
        {
            // Get the current time as a seed value
            double seed = time(0);
            const gsl_rng_type* T = gsl_rng_default;
            for (int i = 0; i < omp_get_max_threads(); i++)
            {
                // Allocate the random number generator
                gsl_rng* r = gsl_rng_alloc(T);
                gsl_rng_set(r, seed + i);
                // Push the generator into the array
                m_randGenArray.push_back(r);
            }
        }

        /////////////////////////////////////////////////
        /// \brief Destructor for the thread safe
        /// random number generator that removes the
        /// rngs from memory.
        /////////////////////////////////////////////////
        ~ThreadsafeGslRandGen()
        {
            for (size_t i = 0; i < m_randGenArray.size(); i++)
            {
                // Free the memory
                gsl_rng_free(m_randGenArray[i]);
            }
        }

        /////////////////////////////////////////////////
        /// \brief Returns an instance of a random number
        /// generator for the current thread.
        ///
        /// \return gsl_rng
        ///
        /////////////////////////////////////////////////
        const gsl_rng* getGenerator()
        {
            int threadId = omp_get_thread_num();

            if (threadId >= (int)m_randGenArray.size())
                throw std::out_of_range("Requested a random number generator for thread " + std::to_string(threadId)
                                        + ", which is larger than the number of available generators (i.e. "
                                        + std::to_string(m_randGenArray.size()) + ")");

            return m_randGenArray[threadId];
        }

};


/// File static GSL random number generator, which will be usable via the scoped getRandGenInstance
/// function, which will return a reference to this random number generator
static ThreadsafeGslRandGen gslRandGenerator;


/////////////////////////////////////////////////
/// \brief Returns a reference to the central
/// random number generator instance, which is
/// globally accessible by all random number
/// functions.
///
/// \return gsl_rng
///
/////////////////////////////////////////////////
const gsl_rng* getGslRandGenInstance()
{
    return gslRandGenerator.getGenerator();
}

#ifndef PARSERSTANDALONE

/////////////////////////////////////////////////
/// \brief Get the user display name for Windows.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string getUserDisplayName(bool informal)
{
    unsigned long len = UNLEN+1;
    std::string sUserName(len, '\0');

    if (!GetUserNameExA(NameDisplay, sUserName.data(), &len))
        return "";

    sUserName.resize(len);

    // Return an informal name if a whitespace is found and no
    // comma is used for the inverted NAME, GIVEN-NAME order
    if (informal)
    {
        if (sUserName.find(',') == std::string::npos)
            sUserName.erase(0, sUserName.find(',')+1);

        StripSpaces(sUserName);

        if (sUserName.find(' ') != std::string::npos)
            sUserName.erase(sUserName.find(' '));
    }

    return sUserName;
}


using namespace std;

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
                    && __sCmd.substr(i, sParam.length()) == toLowerCase(sParam))
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

        if (sCmd.find("firstch(") != string::npos && (sCmd.find("firstch(") < nPos || nPos == string::npos))
            nPos = sCmd.find("firstch(");

        if (sCmd.find("lastch(") != string::npos && (sCmd.find("lastch(") < nPos || nPos == string::npos))
            nPos = sCmd.find("lastch(");
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
            i += getMatchingParenthesis(StringView(sCmd, i));
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
                    j += getMatchingParenthesis(StringView(sCmd, j));

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
        if (i >= sCmd.length() - 1 || (ispunct(sCmd[i]) && sCmd[i] != '.' && sCmd[i] != '_' && sCmd[i] != '+' && sCmd[i] != '#'))
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
/// \brief Determines, if a string contains
/// multiple comma-separated expressions.
///
/// \param sExpr const string&
/// \param bIgnoreClosingParenthesis bool
/// \return bool
///
/////////////////////////////////////////////////
bool isMultiValue(const string& sExpr, bool bIgnoreClosingParenthesis)
{
    // --> Kein Komma? Auf jeden Fall kein Mehrfachausdruck! <--
    if (sExpr.find(',') == string::npos)
        return false;
    else
    {
        size_t nQuotationMarks = 0;

        // Go through the string
        for (size_t i = 0; i < sExpr.length(); i++)
        {
            // Jump over parentheses
            if ((sExpr[i] == '(' || sExpr[i] == '{' || sExpr[i] == '[') && !(nQuotationMarks % 2))
                i += getMatchingParenthesis(StringView(sExpr, i));

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


/////////////////////////////////////////////////
/// \brief Static helper returning the TeX code
/// page.
///
/// \return std::map<std::string, std::string>
///
/////////////////////////////////////////////////
static std::map<std::string, std::string> getTeXCodePage()
{
    std::map<std::string, std::string> mCodePage;

    mCodePage["_pi"] = "\\pi ";
    mCodePage["_hbar"] = "\\hbar ";
    mCodePage["_k_boltz"] = "k_B";
    mCodePage["_2pi"] = "2\\cdot \\pi ";
    mCodePage["_elek_feldkonst"] = "\\varepsilon ";
    mCodePage["_electric_const"] = "\\varepsilon ";
    mCodePage["_elem_ladung"] = "e";
    mCodePage["_elem_charge"] = "e";
    mCodePage["_m_elektron"] = "m_{e}";
    mCodePage["_m_electron"] = "m_{e}";
    mCodePage["_m_neutron"] = "m_{n}";
    mCodePage["_m_proton"] = "m_{p}";
    mCodePage["_m_sun"] = "m_{Sun}";
    mCodePage["_m_sonne"] = "m_{Sonne}";
    mCodePage["_m_earth"] = "m_{Earth}";
    mCodePage["_m_erde"] = "m_{Erde}";
    mCodePage["_m_muon"] = "m_{\\mu}";
    mCodePage["_m_tau"] = "m_{\\tau}";
    mCodePage["_magn_feldkonst"] = "\\mu ";
    mCodePage["_magnetic_const"] = "\\mu ";
    mCodePage["_n_avogadro"] = "N_A";
    mCodePage["_r_earth"] = "r_{Earth}";
    mCodePage["_r_erde"] = "r_{Erde}";
    mCodePage["_r_sun"] = "r_{Sun}";
    mCodePage["_r_sonne"] = "r_{Sonne}";
    mCodePage["_c"] = "c_{Vac}";
    mCodePage["_e"] = "e";
    mCodePage["_g"] = "g";
    mCodePage["_h"] = "h";
    mCodePage["_R"] = "R";
    mCodePage["_alpha_fs"] = "\\alpha_{FS}";
    mCodePage["_mu_bohr"] = "\\mu_{B}";
    mCodePage["_mu_kern"] = "\\mu_{K}";
    mCodePage["_mu_nuclear"] = "\\mu_{N}";
    mCodePage["_m_amu"] = "m_u";
    mCodePage["_r_bohr"] = "a_0";
    mCodePage["_G"] = "G";
    mCodePage["_theta_weinberg"] = "\\theta_{W}";
    mCodePage["_mu_e"] = "\\mu_{e}";
    mCodePage["_mu_p"] = "\\mu_{p}";
    mCodePage["_mu_n"] = "\\mu_{n}";
    mCodePage["_gamma_e"] = "\\gamma_{e}";
    mCodePage["_gamma_p"] = "\\gamma_{p}";
    mCodePage["_gamma_n"] = "\\gamma_{n}";
    mCodePage["_stefan_boltzmann"] = "\\sigma ";
    mCodePage["_rydberg"] = "R_{\\infty}";
    mCodePage["_hartree"] = "E_{h}";
    mCodePage["_wien"] = "b_{Energie}";
    mCodePage["_lande_e"] = "g_{e}";
    mCodePage["_feigenbaum_alpha"] = "\\alpha ";
    mCodePage["_feigenbaum_delta"] = "\\delta ";
    mCodePage["inf"] = "\\infty";
    mCodePage["alpha"] = "\\alpha ";
    mCodePage["Alpha"] = "\\Alpha ";
    mCodePage["beta"] = "\\beta ";
    mCodePage["Beta"] = "\\Beta ";
    mCodePage["gamma"] = "\\gamma ";
    mCodePage["Gamma"] = "\\Gamma ";
    mCodePage["delta"] = "\\delta ";
    mCodePage["Delta"] = "\\Delta ";
    mCodePage["epsilon"] = "\\varepsilon ";
    mCodePage["Epsilon"] = "\\Epsilon ";
    mCodePage["zeta"] = "\\zeta ";
    mCodePage["Zeta"] = "\\Zeta ";
    mCodePage["eta"] = "\\eta ";
    mCodePage["Eta"] = "\\Eta ";
    mCodePage["\theta"] = "\\theta ";
    mCodePage["theta"] = "\\vartheta ";
    mCodePage["Theta"] = "\\Theta ";
    mCodePage["iota"] = "\\iota ";
    mCodePage["Iota"] = "\\Iota ";
    mCodePage["kappa"] = "\\kappa ";
    mCodePage["Kappa"] = "\\Kappa ";
    mCodePage["lambda"] = "\\lambda ";
    mCodePage["Lambda"] = "\\Lambda ";
    mCodePage["mu"] = "\\mu";
    mCodePage["Mu"] = "\\Mu ";
    mCodePage["\nu"] = "\\nu ";
    mCodePage["nu"] = "\\nu ";
    mCodePage["Nu"] = "\\Nu ";
    mCodePage["xi"] = "\\xi ";
    mCodePage["Xi"] = "\\Xi ";
    mCodePage["omikron"] = "o ";
    mCodePage["Omikron"] = "O ";
    mCodePage["pi"] = "\\pi ";
    mCodePage["Pi"] = "\\Pi ";
    mCodePage["rho"] = "\\rho ";
    mCodePage["Rho"] = "\\Rho ";
    mCodePage["sigma"] = "\\sigma ";
    mCodePage["Sigma"] = "\\Sigma ";
    mCodePage["\tau"] = "\\tau ";
    mCodePage["tau"] = "\\tau ";
    mCodePage["Tau"] = "\\Tau ";
    mCodePage["ypsilon"] = "\\upsilon ";
    mCodePage["Ypsilon"] = "\\Upsilon ";
    mCodePage["phi"] = "\\varphi ";
    mCodePage["Phi"] = "\\Phi";
    mCodePage["chi"] = "\\chi ";
    mCodePage["Chi"] = "\\Chi ";
    mCodePage["psi"] = "\\psi ";
    mCodePage["Psi"] = "\\Psi ";
    mCodePage["omega"] = "\\omega ";
    mCodePage["Omega"] = "\\Omega ";
    mCodePage["heaviside"] = "\\Theta";
    mCodePage["Li2"] = "Li_2";
    mCodePage["Cl2"] = "Cl_2";

    return mCodePage;
}


static std::map<std::string, std::string> getTeXCodePageSpecial()
{
    std::map<std::string, std::string> mCodePage;

    mCodePage["*"] = "\\cdot  ";
    mCodePage["+"] = " + ";
    mCodePage["-"] = " -- ";
    mCodePage[","] = ", ";
    mCodePage["x"] = "{\\i x}";
    mCodePage["y"] = "{\\i y}";
    mCodePage["z"] = "{\\i z}";
    mCodePage["t"] = "{\\i t}";

    return mCodePage;
}


/////////////////////////////////////////////////
/// \brief This function is a static helper
/// function for the function replaceToTeX
/// further down
///
/// \param sReturn std::string&
/// \param std::map<std::string const
/// \param mCodePage std::string>&
/// \return void
///
/////////////////////////////////////////////////
static void handleTeXIndicesAndExponents(std::string& sReturn, const std::map<std::string,std::string>& mCodePage)
{
    static std::string sDelimiter = "+-*/, #()&|!_'";

    // Handle the exponents
    for (size_t i = 0; i < sReturn.length() - 1; i++)
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
            for (size_t j = i + 1; j < sReturn.length(); j++)
            {
                if (sDelimiter.find(sReturn[j]) != std::string::npos)
                {
                    sReturn = sReturn.substr(0, j) + "}" + sReturn.substr(j);
                    break;
                }
                else if (j + 1 == sReturn.length())
                    sReturn += "}";
            }
        }
    }

    // Append the exponent operator
    sDelimiter[sDelimiter.length() - 1] = '^';

    // Handle the indices
    for (size_t i = 0; i < sReturn.length() - 1; i++)
    {
        // Ignore constants
        if (sReturn[i] == '_' && sReturn[i + 1] != '{')
        {
            for (const auto& iter : mCodePage)
            {
                if (iter.first.front() != '_')
                    break;

                if (sReturn.substr(i, iter.first.length()) == iter.first
                    && sDelimiter.find(sReturn[i + iter.first.length()]) != std::string::npos)
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
            for (size_t j = i + 1; j < sReturn.length(); j++)
            {
                if (sDelimiter.find(sReturn[j]) != std::string::npos)
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


/////////////////////////////////////////////////
/// \brief Replace the special tokens with their
/// TeX counterparts, so that they can be handled
/// by a LaTeX interpreter (like such in the
/// plotting lib).
///
/// \param sString const std::string&
/// \param replaceForTeXFile bool true for TeX
/// files. Will replace fewer characters.
/// \return std::string
///
/////////////////////////////////////////////////
std::string replaceToTeX(const std::string& sString, bool replaceForTeXFile)
{
    std::string sReturn = " " + sString + " ";            // Rueckgabe-String
    size_t nPos = 0;                           // Positions-Index-Variable

    static std::map<std::string, std::string> mCodePage = getTeXCodePage();
    static std::map<std::string, std::string> mSpecialSymbols = getTeXCodePageSpecial();

    // --> Ersetze zunaechst die gamma-Funktion <--
    while ((nPos = sReturn.find("gamma(", nPos)) != std::string::npos)
    {
        if (nPos && !isDelimiter(sReturn[nPos-1]))
        {
            nPos++;
            continue;
        }

        sReturn.replace(nPos, nPos+6, "\\Gamma(");
        nPos += 7;
    }

    // --> Laufe durch alle bekannten Symbole <--
    for (const auto& iter : mCodePage)
    {
        // --> Positions-Indices zuruecksetzen <--
        nPos = 0;

        // --> So lange in dem String ab der Position nPos das Token auftritt <--
        while ((nPos = sReturn.find(iter.first, nPos)) != string::npos)
        {
            // --> Falls vor dem Token schon ein '\' ist, wurde das hier schon mal ersetzt <--
            if (sReturn[nPos - 1] == '\\')
            {
                // --> Positionsindex um die Laenge des Tokens weitersetzen <--
                nPos += iter.first.length();
                continue;
            }

            if (sReturn[nPos + iter.first.length()] == '_')
            {
                // Wird das Token von '_' abgeschlossen? Pruefen wir, ob es von vorne auch begrenzt ist <--
                if (!isDelimiter(sReturn[nPos-1]))
                {
                    // --> Nein? Den Positionsindex um die Laenge des Tokens weitersetzen <--
                    nPos += iter.first.length();
                    continue;
                }
            }
            else if (!StringView(sReturn).is_delimited_sequence(nPos, iter.first.length()))
            {
                // --> Pruefen wir auch getrennt den Fall, ob das Token ueberhaupt begrenzt ist ('_' zaehlt nicht zu den Delimitern) <--
                nPos += iter.first.length();
                continue;
            }

            sReturn.replace(nPos, iter.first.length(), iter.second);
            nPos += iter.second.length();
        }
    }

    for (const auto& iter : mSpecialSymbols)
    {
        // --> Positions-Indices zuruecksetzen <--
        nPos = 0;

        // --> So lange in dem String ab der Position nPos das Token auftritt <--
        while ((nPos = sReturn.find(iter.first, nPos)) != string::npos)
        {
            // --> Falls vor dem Token schon ein '\' ist, wurde das hier schon mal ersetzt <--
            if (sReturn[nPos - 1] == '\\')
            {
                // --> Positionsindex um die Laenge des Tokens weitersetzen <--
                nPos += iter.first.length();
                continue;
            }

            if (iter.first == "*"
                || (!replaceForTeXFile && !isalpha(iter.first.front())))
            {
                // remove obsolete whitespaces around the operators
                // first in front of the operator
                if (sReturn[nPos - 1] == ' ')
                {
                    size_t nPos_2 = sReturn.find_last_not_of(' ', nPos-1)+1;
                    sReturn.erase(nPos_2, nPos - nPos_2);
                    nPos = nPos_2;
                }

                // after the operator
                if (sReturn[nPos + 1] == ' ')
                    sReturn.erase(nPos+1, sReturn.find_first_not_of(' ', nPos+1) - nPos-1);

                // Identify exponents and ignore them
                if (iter.first == "+" || iter.first == "-")
                {
                    if (sReturn[nPos-1] == 'e' || sReturn[nPos-1] == 'E')
                    {
                        if (isdigit(sReturn[nPos-2]) && isdigit(sReturn[nPos+1]) )
                        {
                            nPos++;
                            continue;
                        }
                    }

                    // Don't insert whitespaces directly after opening parentheses and commas
                    if (!nPos
                        || sReturn[nPos-1] == '('
                        || sReturn[nPos-1] == '['
                        || sReturn[nPos-1] == '{'
                        || sReturn[nPos-1] == ',')
                    {
                        if (iter.first == "-")
                        {
                            sReturn.insert(nPos, 1, '-');
                            nPos++;
                        }

                        nPos++;
                        continue;
                    }
                }
            }
            else if (sReturn[nPos + iter.first.length()] == '_')
            {
                // Wird das Token von '_' abgeschlossen? Pruefen wir, ob es von vorne auch begrenzt ist <--
                if (!isDelimiter(sReturn[nPos-1]))
                {
                    // --> Nein? Den Positionsindex um die Laenge des Tokens weitersetzen <--
                    nPos += iter.first.length();
                    continue;
                }
            }
            else if (!StringView(sReturn).is_delimited_sequence(nPos, iter.first.length()))
            {
                // --> Pruefen wir auch getrennt den Fall, ob das Token ueberhaupt begrenzt ist ('_' zaehlt nicht zu den Delimitern) <--
                nPos += iter.first.length();
                continue;
            }

            sReturn.replace(nPos, iter.first.length(), iter.second);
            nPos += iter.second.length();
        }
    }

    // --> Ersetze nun lange Indices "_INDEX" durch "_{INDEX}" <--
    handleTeXIndicesAndExponents(sReturn, mCodePage);

    nPos = 0;

    // replace "sqrt()"
    while ((nPos = sReturn.find("sqrt(", nPos)) != string::npos)
    {
        size_t nBrace = getMatchingParenthesis(StringView(sReturn, nPos+4));
        sReturn[nBrace+nPos+4] = '}';
        sReturn.replace(nPos, 5, "\\sqrt{");
    }

    nPos = 0;

    // replace "norm()"
    while ((nPos = sReturn.find("norm(", nPos)) != string::npos)
    {
        size_t nBrace = getMatchingParenthesis(StringView(sReturn, nPos+4));
        sReturn[nBrace+nPos+4] = '|';
        sReturn.replace(nPos, 5, "|");
    }

    nPos = 0;

    // replace "abs()"
    while ((nPos = sReturn.find("abs(", nPos)) != string::npos)
    {
        size_t nBrace = getMatchingParenthesis(StringView(sReturn, nPos+3));
        sReturn[nPos+3+nBrace] = '|';
        sReturn.replace(nPos, 4, "|");
    }

    nPos = 0;

    // Long exponents
    while ((nPos = sReturn.find("^(", nPos)) != string::npos)
    {
        size_t nBrace = getMatchingParenthesis(StringView(sReturn, nPos+1));
        sReturn[nBrace+nPos+1] = '}';
        sReturn[nPos+1] = '{';
    }

    // --> Entferne die Leerzeichen am Anfang und Ende und gib sReturn zurueck <--
    StripSpaces(sReturn);
    return sReturn;
}


/////////////////////////////////////////////////
/// \brief This is a static helper function for
/// the findCommand function.
///
/// \param sCmd StringView
/// \return Match
///
/////////////////////////////////////////////////
static Match findCasualCommand(StringView sCmd)
{
    Match _mMatch;
    _mMatch.sString = "";
    _mMatch.nPos = string::npos;
    size_t nStart = 0;

    // Jump over breakpoints
    if (sCmd.subview(0,2) == "|>")
        nStart = 2;

    // Go through the complete command line
    for (size_t i = nStart; i < sCmd.length(); i++)
    {
        // Break the loop, if one recognizes typical initializers of the parameter list
        if ((sCmd.subview(i, 2) == "--" || sCmd.subview(i, 5) == "-set ") && !isInQuotes(sCmd, i))
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
            _mMatch.sString = sCmd.subview(_mMatch.nPos, i - _mMatch.nPos).to_string();

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
            _mMatch.sString = sCmd.subview(_mMatch.nPos, i - _mMatch.nPos).to_string();

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
        _mMatch.sString = sCmd.subview(_mMatch.nPos).to_string();
    else
        _mMatch.sString = sCmd.to_string();

    // Return the found match
    return _mMatch;
}


/////////////////////////////////////////////////
/// \brief This is a static helper function for
/// the findCommandWithReturnValue function.
///
/// \param _mMatch Match&
/// \param position size_t
/// \param character char
/// \param sCmd StringView
/// \param sCommand const std::string&
/// \return bool
///
/////////////////////////////////////////////////
static bool findShortestMatchForCommand(Match& _mMatch, size_t position, char character, StringView sCmd, const std::string& sCommand)
{
    // Store the command string and the match position
    _mMatch.sString = sCmd.subview(position, sCmd.find(character, position + sCommand.length()) - position).to_string();
    _mMatch.nPos = position;

    // If there's a whitespace in the match string, erase it end everything after it
    if (_mMatch.sString.find(' ') != string::npos)
        _mMatch.sString.erase(_mMatch.sString.find(' '));

    // Ensure that the found command is a single word
    if (!isInQuotes(sCmd, position)
        && isDelimiter(sCmd[position+_mMatch.sString.length()])
        && (!position || isDelimiter(sCmd[position-1])))
    {
        // If the command line is longer than the match position and the length of both strings
        if (sCmd.length() >= sCommand.length() + _mMatch.nPos + _mMatch.sString.length())
        {
            // Try to find the command further back by calling findCommand recursively
            Match _mTemp = findCommand(sCmd.subview(_mMatch.nPos + _mMatch.sString.length()), sCommand);

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


/////////////////////////////////////////////////
/// \brief This is a static helper function for
/// the findCommand function.
///
/// \param sCmd StringView
/// \param sCommand const string&
/// \return Match
///
/////////////////////////////////////////////////
static Match findCommandWithReturnValue(StringView sCmd, const string& sCommand)
{
    Match _mMatch;
    _mMatch.sString = "";
    _mMatch.nPos = string::npos;
    size_t nStart = 0;

    // Jump over breakpoints
    if (sCmd.subview(0,2) == "|>")
        nStart = 2;

    // Go through the complete command line
    for (size_t i = nStart; i < sCmd.length(); i++)
    {
        // Break the loop, if we find typical parameter string initializers
        if ((sCmd.subview(i, 2) == "--" || sCmd.subview(i, 5) == "-set ") && !isInQuotes(sCmd, i))
            break;

        // Jump over some special characters
        if (sCmd[i] == ' ' || sCmd[i] == '\t' || sCmd[i] == '(')
            continue;

        // Jump, if this is not our searched command
        if (sCmd.subview(i, sCommand.length()) != sCommand)
            continue;

        // This is our command and it is not at the beginning of the line
        if (sCmd.match(sCommand, i) && i)
        {
            // Is the command filling the rest of the command line?
            if (i + sCommand.length() == sCmd.length() - 1)
            {
                // Store the position
                _mMatch.nPos = i;

                // Ensure that the command is not part of a larger word
                if (sCmd.is_delimited_sequence(i) && !isInQuotes(sCmd, i))
                {
                    // It is not
                    // Remove the additional characters
                    _mMatch.sString = sCmd.subview(i).to_string();
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
        if (sCmd.starts_with(sCommand))
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


/////////////////////////////////////////////////
/// \brief This function is very important for
/// the command handler.
///
/// It extracts the command out of the complete
/// command line. It is also capable of finding a
/// command, which is not at the start of the
/// line (i.e. commands, which are returning
/// values).
///
/// \param sCmd StringView
/// \param sCommand const std::string&
/// \return Match
///
/////////////////////////////////////////////////
Match findCommand(StringView sCmd, const std::string& sCommand)
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
        _mMatch.sString = sCmd.to_string();
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


/////////////////////////////////////////////////
/// \brief Extracts the whole command string from
/// a command line (which might contain more than
/// one).
///
/// \param sCmd const string&
/// \param _mMatch const Match&
/// \return string
///
/////////////////////////////////////////////////
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
                if (getMatchingParenthesis(StringView(sCmd, i)) != string::npos)
                {
                    // Use the contents of the parenthesis
                    // However, only extract the command and not the characters in front of it
                    sCommandString = sCmd.substr(_mMatch.nPos, getMatchingParenthesis(StringView(sCmd, i)) - (_mMatch.nPos - i + 1));
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


/////////////////////////////////////////////////
/// \brief Opens a file with the program, which
/// is associated by default within the Windows
/// shell.
///
/// \param sFile const string&
/// \return void
///
/////////////////////////////////////////////////
void openExternally(const string& sFile)
{
    std::string _sFile;

    int nErrorCode = 0;

    // Add quotation marks if there are none and convert the string to a character pointer
    if (_sFile[0] != '"')
        _sFile = "\"" + sFile + "\"";
    else
        _sFile = sFile;

    replaceAll(_sFile, "/", "\\");

    // Invoke the Windows shell
    nErrorCode = (size_t)ShellExecute(nullptr, "open", sFile.c_str(), nullptr, nullptr, SW_SHOWNORMAL);

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


/////////////////////////////////////////////////
/// \brief This static function detects, whether
/// the current parameter expression is continued
/// after a whitespace. This is done by checking
/// for operators and following chars, with
/// special focus on minus characters.
///
/// \param sCmd const std::string&
/// \param pos size_t
/// \return size_t
///
/////////////////////////////////////////////////
static size_t isContinued(const std::string& sCmd, size_t pos)
{
    if (sCmd.substr(pos, 2) == "  ")
        return std::string::npos;

    size_t nextChar = sCmd.find_first_not_of(' ', pos+1);
    const static std::string sBinaryOperators = "+-*/^&|?:!=<>";

    if (nextChar == std::string::npos || sBinaryOperators.find(sCmd[nextChar]) == std::string::npos || sCmd.length() <= nextChar+2)
        return std::string::npos;

    // This is a combination like par=VAL -par2=VAL2
    if (sCmd[nextChar] == '-' && std::isalpha(sCmd[nextChar+1]))
        return std::string::npos;

    // Advance over multiple combinations of operators
    while (sBinaryOperators.find(sCmd[nextChar]) != std::string::npos)
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

    // Read data
    if (instance->getMemoryManager().containsTables(sArg))
        getDataElements(sArg, instance->getParser(), instance->getMemoryManager());

    // Ensure that a string evaluation is necessary (buggy fix, hard to derive a more general one)
    if (flags & ARGEXTRACT_ASSTRING && !containsStrings(sArg))
        return;

    // evaluation
    instance->getParser().SetExpr(sArg);

    int results;
    int nPrec = instance->getSettings().getPrecision();
    const mu::StackItem* v = instance->getParser().Eval(results);

    sArg.clear();

    for (int i = 0; i < results; i++)
    {
        if (sArg.length())
            sArg += ",";

        if (flags & ARGEXTRACT_ASINT)
            sArg += toString(v[i].get().getAsScalarInt());
        else if (flags & ARGEXTRACT_ASSTRING)
            sArg += v[i].get().print(nPrec);
        else
            sArg += v[i].get().printVals(nPrec);
    }
}


/////////////////////////////////////////////////
/// \brief Extracts a options value at the
/// selected position and applies automatic
/// parsing, if necessary.
///
/// \param sCmd const string&
/// \param nPos size_t
/// \param extraction int
/// \return string
///
/////////////////////////////////////////////////
string getArgAtPos(const string& sCmd, size_t nPos, int extraction)
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
            i += getMatchingParenthesis(StringView(sCmd, i));

        if (sCmd[i] == ' ')
        {
            size_t cont = isContinued(sCmd, i);

            if (cont < sCmd.length())
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

    StripSpaces(sArgument);

    // Strip the argument, if necessary
    if (extraction & ARGEXTRACT_STRIPPED)
    {
        if (sArgument.front() == '"' && sArgument.back() == '"')
            sArgument = sArgument.substr(1, sArgument.length()-2);
    }

    // return the found option value
    return sArgument;
}


/////////////////////////////////////////////////
/// \brief Checks, whether the position in the
/// passed expression is part of a string literal.
///
/// \param sExpr StringView
/// \param nPos size_t
/// \param bool bIgnoreVarParser  = false
/// \return bool
///
/////////////////////////////////////////////////
bool isInQuotes(StringView sExpr, size_t nPos, bool bIgnoreVarParser /* = false*/)
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
        for (size_t i = sExpr.rfind('#', nPos); i < nPos; i++)
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


/////////////////////////////////////////////////
/// \brief This function determines, whether the
/// current position is part of an argument of
/// the three value to string conversion
/// functions.
///
/// \param sExpr const string&
/// \param nPos size_t
/// \return bool
///
/////////////////////////////////////////////////
bool isToStringArg(const string& sExpr, size_t nPos)
{
    // Ensure that at least one of the three functions is available in the string
    if (sExpr.find("valtostr(") == string::npos && sExpr.find("to_string(") == string::npos && sExpr.find("string_cast(") == string::npos)
        return false;

    // Go inversely through the string
    for (int i = nPos; i >= 8; i--)
    {
        // An opening parenthesis was found with its counterpart after the current position
        if (sExpr[i] == '(' && getMatchingParenthesis(StringView(sExpr, i)) + i > nPos)
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
/// \brief This function adds the missing legend
/// strings to the expressions in the passed
/// string.
///
/// \param sExpr string&
/// \return bool
///
/////////////////////////////////////////////////
bool addLegends(string& sExpr)
{
    // Validate the number of parentheses
    if (!validateParenthesisNumber(sExpr))
        throw SyntaxError(SyntaxError::UNMATCHED_PARENTHESIS, sExpr, sExpr.find_first_of("({[]})"));

    EndlessVector<std::string> args = getAllArguments(sExpr);

    std::string sTemp;
    static const std::string sOPERATORS = "+-*/^?:!|&<>=";

    for (std::string arg : args)
    {
        if (sTemp.length())
            sTemp += ", ";

        // Find the end of the current expression first
        for (size_t i = 0; i < arg.length(); i++)
        {
            if (arg[i] == '{' || arg[i] == '(')
                i += getMatchingParenthesis(StringView(arg, i));

            if (arg[i] == '"')
            {
                if (arg.find('"', i+1) != std::string::npos)
                {
                    if (arg.find('#', i+1) != std::string::npos)
                        sTemp += arg + "+\"\"";
                    else
                        sTemp += arg;

                    break;
                }
                else
                    return false;   // Something's wrong here
            }
            else if (arg[i] == '#')
            {
                for (size_t j = i+1; j < arg.length(); j++)
                {
                    if (arg[j] == '(')
                        j += getMatchingParenthesis(StringView(arg, j));

                    if (arg[j] == ' ')
                    {
                        sTemp += arg.insert(j, "+\"\"");
                        break;
                    }
                    else if (j+1 == arg.length())
                    {
                        sTemp += arg + "+\"\"";
                        break;
                    }
                }

                break;
            }
            else if (arg[i] == ' ')
            {
                // The last character could be a delimiter
                if (i && sOPERATORS.find(arg[i-1]) != std::string::npos)
                {
                    size_t following = arg.find_first_not_of(" " + sOPERATORS, i);

                    if (following != std::string::npos)
                    {
                        i = following - 1;
                        continue;
                    }
                }

                size_t following = arg.find_first_not_of(" ", i);

                // The found character could be a delimiter
                if (sOPERATORS.find(arg[following]) != std::string::npos)
                {
                    following = arg.find_first_not_of(" " + sOPERATORS, following);

                    if (following != std::string::npos)
                    {
                        i = following - 1;
                        continue;
                    }
                }

                if (std::isalpha(arg[following]) || arg[following] == '_')
                {
                    sTemp += arg.substr(0, i) + " \"\"+" + arg.substr(following);
                    break;
                }
                else
                    i = following-1;
            }
            else if (i+1 == arg.length())
            {
                std::string sLabel = arg;

                StripSpaces(sLabel);

                sTemp += arg + " " + toExternalString(sLabel);
            }
        }
    }

    sExpr = sTemp;

    return true;
}


/////////////////////////////////////////////////
/// \brief Checks, whether the first and the last
/// character in the passed string is a delimiter
/// character.
///
/// \param sString StringView
/// \param stringdelim bool
/// \return bool
///
/////////////////////////////////////////////////
bool checkDelimiter(StringView sString, bool stringdelim)
{
    // --> Gib die Auswertung dieses logischen Ausdrucks zurueck <--
    //return isDelimiter(sString.front()) && (isDelimiter(sString.back()) || (sString.back() == '.' && stringdelim));
    return sString.is_delimited_sequence(1, sString.length()-2,
                                         stringdelim ? StringViewBase::STRING_DELIMITER : StringViewBase::STD_DELIMITER);
}


/////////////////////////////////////////////////
/// \brief This function splits a string into
/// multiple lines if the line is longer than the
/// desired line width.
///
/// \param sOutput std::string
/// \param lineWidth size_t
/// \param bAllowDashBreaks bool
/// \param nFirstIndent int
/// \param nIndent int
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> splitIntoLines(std::string sOutput, size_t lineWidth, bool bAllowDashBreaks, int nFirstIndent, int nIndent)
{
    // Set the return vector of strings
    std::vector<std::string> outputInLines;

    // Variable to store the position of the last line break
    size_t nLastLineBreak = 0;

    // Convert the output to the system code page
    sOutput = toSystemCodePage(sOutput);

    // Check if the output already contains the line starting chars
    if ((sOutput.starts_with("|   ") || sOutput.starts_with("|-> ")) && nFirstIndent == 4)
        nFirstIndent = 0;

    // Check if string is already shorter than the line length and does not have any new line commands
    if (sOutput.length() < lineWidth - nFirstIndent && sOutput.find('$') == string::npos && sOutput.find("\\n") == string::npos && sOutput.find('\n') == string::npos)
    {
        outputInLines.push_back(sOutput);
        return outputInLines;
    }

    // Go through the whole string
    for (size_t i = 1; i < sOutput.length(); i++)
    {
        // Check for the '$' sign. If found update the last line break
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
        // Check for explicit line break
        if (sOutput[i] == '\n')
        {
            nLastLineBreak = i;
        }

        // Check if max line length is reached. In that case, split the string at a fitting position
        if ((i == lineWidth - nFirstIndent && !nLastLineBreak) || (nLastLineBreak && i - nLastLineBreak == lineWidth - nIndent))
        {
            // Go backwards from the current position and look for 1 space or 1 minus sign (if allowed) or the "$"
            for (size_t j = i; j > nLastLineBreak; j--)
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
                        sOutput[j] = '$';   // Replace " " with a "$"
                        nLastLineBreak = j;
                    }
                    break;
                }
                else if (sOutput[j] == '-' && bAllowDashBreaks && j != i)
                {
                    // Check if a line break is applicable for the "-" used
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
                else if (sOutput[j] == '$' && sOutput[j - 1] != '\\') // Definitely a line break is required here
                {
                    nLastLineBreak = j;
                    break;
                }
                if (j - 1 == nLastLineBreak)
                {
                    string sDelim = "+-*/";
                    for (size_t n = i; n > nLastLineBreak; n--)
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

    // Go through the whole string again
    size_t lastBreak = 0;   // lastBreak is the position of the first relevant char for the new line
    for (size_t i = 0; i < sOutput.length(); i++)
    {
        if (sOutput[i] == '$' && sOutput[i - 1] != '\\')
        {
            // Split the string at the '$' sign
            outputInLines.push_back(sOutput.substr(lastBreak, i - lastBreak));
            lastBreak = i + 1;
            continue;
        }
        else if (sOutput[i] == '\n')
        {
            outputInLines.push_back(sOutput.substr(lastBreak, i - lastBreak));
            lastBreak = i + 1;
            continue;
        }
        else if (sOutput[i] == 'n' && sOutput[i - 1] == '\\' && sOutput[i - 2] != '\\')
            sOutput = sOutput.substr(0, i - 1) + "\n" + sOutput.substr(i + 1);
        else if (sOutput[i] == 'n' && sOutput[i - 1] == '\\' && sOutput[i - 2] == '\\')
            sOutput.erase(i - 1, 1);
        else if (sOutput[i] == '$' && sOutput[i - 1] == '\\')
            sOutput.erase(i - 1, 1);
        else if (sOutput[i] == '%' && bAllowDashBreaks)
        {
            // Replace '%' with ',' and the indent if allowed
            sOutput = sOutput.substr(0, i) + ",\n" + sOutput.substr(i + 1);
        }
    }

    // Add the rest of the string
    outputInLines.push_back(sOutput.substr(lastBreak));

    return outputInLines;
}


/////////////////////////////////////////////////
/// \brief This function outputs a string by
/// adding the required prefixes and joining
/// the various strings together.
///
/// \param stringInLines std::vector<std::string>
/// \param nFirstIndent int
/// \param nIndent int
/// \return std::string
///
/////////////////////////////////////////////////
std::string outputString(std::vector<std::string> stringInLines, int nFirstIndent, int nIndent)
{
    std::string sOutput = "";

    // Check if vector is empty and return empty string if so
    if (!stringInLines.size())
        return sOutput;

    // Check if the output already contains the line starting chars
    if ((sOutput.starts_with("|   ") || sOutput.starts_with("|-> ")) && nFirstIndent == 4)
        nFirstIndent = 0;

    // Add the first line with the corresponding indent
    sOutput.append(stringInLines[0]);

    // Prepare the prefix for all strings except the first one
    std::string sIndent = "\n|";
    for (int i = 1; i < nIndent; i++)
        sIndent += " ";

    // Add the prefixes and join the strings
    for (size_t i = 1; i < stringInLines.size(); i++)
        sOutput.append(sIndent + stringInLines[i]);

    return sOutput;
}


/////////////////////////////////////////////////
/// \brief This function takes a string, splits
/// it into multiple lines if it is too long
/// and returns the result.
///
/// \param sOutput std::string
/// \param _option const Settings&
/// \param bAllowDashBreaks bool
/// \param nFirstIndent int
/// \param nIndent int
/// \return std::string
///
/////////////////////////////////////////////////
std::string LineBreak(std::string sOutput, const Settings& _option, bool bAllowDashBreaks, int nFirstIndent, int nIndent)
{
    return outputString(splitIntoLines(sOutput, _option.getWindow(), bAllowDashBreaks, nFirstIndent, nIndent), nFirstIndent, nIndent);
}


/////////////////////////////////////////////////
/// \brief Determines the root of the line
/// connecting the two passed points.
///
/// \param x_0 double
/// \param y_0 double
/// \param x_1 double
/// \param y_1 double
/// \return double
///
/////////////////////////////////////////////////
double Linearize(double x_0, double y_0, double x_1, double y_1)
{
    double b = y_0;
    double m = (y_1 - y_0) / (x_1 - x_0);
    // y = m*x + b ==> x = 1/m*(y-b) ==> x = -b/m fuer y = 0.0
    return x_0 - b / m;
}


/////////////////////////////////////////////////
/// \brief Wrapper for the static member function
/// of the kernel.
///
/// \param nStep int
/// \param nFirstStep int
/// \param nFinalStep int
/// \param sType const string&
/// \return void
///
/////////////////////////////////////////////////
void make_progressBar(int nStep, int nFirstStep, int nFinalStep, const string& sType)
{
    NumeReKernel::progressBar(nStep, nFirstStep, nFinalStep, sType);
    return;
}


/////////////////////////////////////////////////
/// \brief This function is a static helper
/// function for containsStrings to search
/// through the list of known clusters for
/// string-containing clusters.
///
/// \param sLine StringView
/// \return bool
///
/////////////////////////////////////////////////
static bool containsStringClusters(StringView sLine)
{
    const mu::varmap_type vars = NumeReKernel::getInstance()->getParser().GetVar();

    for (auto iter = vars.begin(); iter != vars.end(); ++iter)
    {
        if (iter->second->getCommonType() == mu::TYPE_STRING || iter->second->getCommonType() == mu::TYPE_CLUSTER)
        {
            size_t pos = sLine.find(iter->first);

            if (pos != std::string::npos && sLine.is_delimited_sequence(pos, iter->first.length()))
                return true;
        }
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief This function checks, whether the
/// passed expression contains strings or
/// valtostring parser.
///
/// \param sLine StringView
/// \return bool
///
/////////////////////////////////////////////////
bool containsStrings(StringView sLine)
{
    if (!sLine.length())
        return false;

    // It's only necessary to check the following functions.
    // All other string functions need strings as input
    if (sLine.find_first_of("\"#") != std::string::npos
        || sLine.find("string_cast(") != std::string::npos
        || sLine.find("char(") != std::string::npos
        || sLine.find("getlasterror(") != std::string::npos
        || sLine.find("getuilang(") != std::string::npos
        || sLine.find("getversioninfo(") != std::string::npos
        || sLine.find("valtostr(") != std::string::npos
        || sLine.find("weekday(") != std::string::npos
        || sLine.find("to_tex(") != std::string::npos)
        return true;

    return containsStringClusters(sLine);
}


/////////////////////////////////////////////////
/// \brief This function erases option tokens
/// including their possible value from a
/// parameter string.
///
/// \param sExpr string&
/// \param sToken const string&
/// \param bTokenHasValue bool
/// \return void
///
/////////////////////////////////////////////////
void eraseToken(string& sExpr, const string& sToken, bool bTokenHasValue)
{
    size_t nLength = sToken.length();

    // If the option token has a value, then the erase process is a bit more complex
    if (bTokenHasValue)
    {
        // Is the token actually available?
        if (!findParameter(sExpr, sToken, '='))
            return;

        // Search for the option value
        for (size_t i = findParameter(sExpr, sToken, '=') + nLength - 1; i < sExpr.length(); i++)
        {
            // Assignment operator found
            if (sExpr[i] == '=')
            {
                // Find the end of the option value
                for (size_t j = sExpr.find_first_not_of("= ", i); j < sExpr.length(); j++)
                {
                    // jump over parentheses
                    if (!isInQuotes(sExpr, j) && (sExpr[j] == '(' || sExpr[j] == '[' || sExpr[j] == '{'))
                        j += getMatchingParenthesis(StringView(sExpr, j));

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
}


/////////////////////////////////////////////////
/// \brief This function returns a list of files
/// (including their paths, if nFlags & 1).
///
/// \param sDirectory const string&
/// \param nFlags int
/// \return vector<string>
///
/////////////////////////////////////////////////
vector<string> getFileList(const string& sDirectory, int nFlags)
{
    return NumeReKernel::getInstance()->getFileSystem().getFileList(sDirectory, nFlags);
}


/////////////////////////////////////////////////
/// \brief This function returns a list of
/// directories (including their paths, if
/// nFlags & 1).
///
/// \param sDirectory const string&
/// \param nFlags int
/// \return vector<string>
///
/////////////////////////////////////////////////
vector<string> getFolderList(const string& sDirectory, int nFlags)
{
    return NumeReKernel::getInstance()->getFileSystem().getFolderList(sDirectory, nFlags);
}


/////////////////////////////////////////////////
/// \brief This function is used to reduce the
/// log file size from >= 100.000 to 20.000 lines
/// to reduce the needed disk space.
///
/// \param sFileName const string&
/// \return void
///
/////////////////////////////////////////////////
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
        for (size_t i = 0; i < nLines; i++)
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
}


/////////////////////////////////////////////////
/// \brief This function is a variable
/// initializer for the function replaceToVector
/// name. It will only be called once.
///
/// \param map<string
/// \param mOprtRplc string>&
/// \return void
///
/////////////////////////////////////////////////
static void OprtRplc_setup(map<string, string>& mOprtRplc)
{
    mOprtRplc["("] = "`";
    mOprtRplc[")"] = "`";
    mOprtRplc[":"] = "~c~";
    mOprtRplc[","] = "_";
    mOprtRplc["."] = "_";
    mOprtRplc["+"] = "~p~";
    mOprtRplc["-"] = "~m~";
    mOprtRplc["*"] = "~ml~";
    mOprtRplc["/"] = "~d~";
    mOprtRplc["^"] = "~e~";
    mOprtRplc["{"] = "~ob~";
    mOprtRplc["}"] = "~cb~";
    mOprtRplc["&"] = "~a~";
    mOprtRplc["|"] = "~o~";
    mOprtRplc["%"] = "~md~";
    mOprtRplc["!"] = "~n~";
    mOprtRplc["="] = "~eq~";
    mOprtRplc[">"] = "~g~";
    mOprtRplc["<"] = "~l~";
    mOprtRplc["?"] = "~q~";
}


/////////////////////////////////////////////////
/// \brief This function replaces a data access
/// expression (i.e. the contents of the object
/// argument parentheses) into a vector name,
/// which is valid for the parser.
///
/// \param sExpression const string&
/// \return string
///
/////////////////////////////////////////////////
string replaceToVectorname(const string& sExpression)
{
    string sVectorName = sExpression;
    static map<string, string> mOprtRplc;

    // If the map wasn't initialized do that now
    if (!mOprtRplc.size())
        OprtRplc_setup(mOprtRplc);

    if (sVectorName.find_first_of("\"#") != std::string::npos)
        return "_~" + sha256(sVectorName) + "`_`";

    // Remove whitespaces
    while (sVectorName.find(' ') != string::npos)
        sVectorName.erase(sVectorName.find(' '), 1);

    // Replace mathematical operators
    for (auto iter = mOprtRplc.begin(); iter != mOprtRplc.end(); ++iter)
    {
        while (sVectorName.find(iter->first) != string::npos)
            sVectorName.replace(sVectorName.find(iter->first), (iter->first).length(), iter->second);
    }

    if (sVectorName.find('`') == std::string::npos)
        sVectorName += "`_`";

    // return the new vector name
    return "_~" + sVectorName;
}


/////////////////////////////////////////////////
/// \brief This function is a static helper
/// function for evalRecursiveExpressions.
///
/// \param sExpr string&
/// \param nPos size_t&
/// \param nArgSepPos size_t&
/// \return bool
///
/////////////////////////////////////////////////
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
        for (size_t j = nPos; j < sExpr.length(); j++)
        {
            // Jump over parentheses
            if (!(nQuotes % 2)
                && (sExpr[j] == '(' || sExpr[j] == '[' || sExpr[j] == '{'))
                j += getMatchingParenthesis(StringView(sExpr, j));

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
                for (size_t k = nPos; k < sExpr.length(); k++)
                {
                    // Jump over parentheses
                    if (!(nQuotes % 2)
                        && (sExpr[k] == '(' || sExpr[k] == '[' || sExpr[k] == '{'))
                        k += getMatchingParenthesis(StringView(sExpr, k));

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


/////////////////////////////////////////////////
/// \brief This function is a static helper
/// function for evalRecursiveExpressions.
///
/// \param sExpr string&
/// \param nPos size_t
/// \param nArgSepPos size_t
/// \return void
///
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief This function converts the recursive
/// assignments like VAR += VAL into
/// VAR = VAR + (VAL)
///
/// \param sExpr string&
/// \return void
///
/////////////////////////////////////////////////
void evalRecursiveExpressions(string& sExpr)
{
    // Ignore flow control blocks
    if (sExpr.starts_with("if ")
        || sExpr.starts_with("if(")
        || sExpr.starts_with("elseif ")
        || sExpr.starts_with("elseif(")
        || sExpr.starts_with("else ")
        || sExpr.starts_with("switch ")
        || sExpr.starts_with("switch(")
        || sExpr.starts_with("case ")
        || sExpr.starts_with("default:")
        || sExpr.starts_with("for ")
        || sExpr.starts_with("for(")
        || sExpr.starts_with("while ")
        || sExpr.starts_with("while("))
        return;

    size_t nArgSepPos = 0;
    int nQuotes = 0;
    bool bAnswerSuppressor = false;

    // Find the trailing semicolon
    if (sExpr.find_last_not_of(" \t") != string::npos && sExpr[sExpr.find_last_not_of(" \t")] == ';')
    {
        sExpr.erase(sExpr.find_last_not_of(" \t"));
        bAnswerSuppressor = true;
    }

    // Go through the complete expression
    for (size_t i = 0; i < sExpr.length(); i++)
    {
        // Jump over parentheses
        if (!(nQuotes % 2)
            && (sExpr[i] == '(' || sExpr[i] == '{' || sExpr[i] == '['))
        {
            size_t parens = getMatchingParenthesis(StringView(sExpr, i));
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



/////////////////////////////////////////////////
/// \brief This function opens a NumeRe-Datafile
/// file, reads its header and converts it to a
/// human-readable string.
///
/// \param sFileName const string&
/// \return string
///
/////////////////////////////////////////////////
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

    if (info.sComment.length() > 204)
        sFileInfo += "\n" + info.sComment.substr(0, 200) + "...";
    else if (info.sComment.length())
        sFileInfo += "\n" + info.sComment;

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

        if (sCommandLine.starts_with("this~") || sCommandLine == "this")
            sCommandLine.replace(0, 4, sThisNameSpace);

        while (sCommandLine.back() == '~')
            sCommandLine.pop_back();

        if (sCommandLine != "main")
            sNameSpace = sCommandLine;
    }

    return sNameSpace;
}


/////////////////////////////////////////////////
/// \brief This function is used to validate the
/// number of parentheses, i.e. whether there's a
/// closing parenthesis for each opened one.
///
/// \param sCmd StringView
/// \return bool
///
/////////////////////////////////////////////////
bool validateParenthesisNumber(StringView sCmd)
{
    int nParCount = 0;
    int nVectCount = 0;
    int nBracketCount = 0;
    int nQuotes = 0;

    // Go through the whole string
    for (size_t i = 0; i < sCmd.length(); i++)
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
    }

    // If one of the counters is nonzero, something is wrong
    return !((bool)nParCount || (bool)nVectCount || (bool)nBracketCount);
}


/////////////////////////////////////////////////
/// \brief This function adds quotation marks
/// around the value of the specified parameter.
///
/// \param sToAdd string&
/// \param sParam const string&
/// \return void
///
/////////////////////////////////////////////////
void addArgumentQuotes(string& sToAdd, const string& sParam)
{
    // Check, whether the parameter exists
    if (findParameter(sToAdd, sParam, '='))
    {
        // Store the position of the equal sign of the parameter token
        addArgumentQuotes(sToAdd, findParameter(sToAdd, sParam, '=') + sParam.length());
    }
}


/////////////////////////////////////////////////
/// \brief This function adds quotation marks
/// around the value of the specified parameter.
///
/// \param sToAdd string&
/// \param pos size_t
/// \return void
///
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief This function evaluates, whether the
/// desired position is part of the argument of a
/// to_cmd() function.
///
/// \param sCmd StringView
/// \param nPos size_t
/// \return bool
///
/////////////////////////////////////////////////
bool isToCmd(StringView sCmd, size_t nPos)
{
    // Exclude boundary cases
    if (nPos < 6 || nPos >= sCmd.length())
        return false;

    size_t quotes = isInQuotes(sCmd, nPos);

    // Go through the whole string and try to find the functions arguments
    for (int i = nPos - 6; i >= 0; i--)
    {
        if (sCmd[i] == '"' && (!i || sCmd[i-1] != '\\'))
            quotes++;

        if (!(quotes % 2) && sCmd.match("to_cmd(", i))
        {
            // function found -> try to find the matching parenthesis
            // If it is left of the desired position, then return true
            size_t matchingParens = getMatchingParenthesis(sCmd.subview(i + 6));

            if (matchingParens > nPos - i - 6
                && matchingParens != std::string::npos)
                return true;
        }
    }

    // End was reached -> return false
    return false;
}


/////////////////////////////////////////////////
/// \brief Check, whether one of the casting
/// functions can be found in the expression.
///
/// \param sCmd StringView
/// \return bool
///
/////////////////////////////////////////////////
bool containsCastingFunctions(StringView sCmd)
{
    return sCmd.find("to_string(") != std::string::npos || sCmd.find("string_cast(") != std::string::npos;
}


/////////////////////////////////////////////////
/// \brief Evaluate the casting functions in the
/// passed expression.
///
/// \param sCmd std::string&
/// \return void
///
/////////////////////////////////////////////////
void evaluateCastingFunctions(std::string& sCmd)
{
    size_t p;

    while ((p = sCmd.find("string_cast(")) != std::string::npos)
    {
        size_t len = getMatchingParenthesis(StringView(sCmd, p+11));
        std::string sArg = sCmd.substr(p+12, len-1);
        sCmd.replace(p, len+12, toExternalString(sArg));
    }

    while ((p = sCmd.find("to_string(")) != std::string::npos)
    {
        size_t len = getMatchingParenthesis(StringView(sCmd, p+9));
        std::string sArg = sCmd.substr(p+10, len-1);
        sCmd.replace(p, len+10, ((sArg.front() == '"' && sArg.back() == '"')
                                 || NumeReKernel::getInstance()->getParser().ContainsStringVars(sArg)) ? sArg : toExternalString(sArg));
    }
}


/////////////////////////////////////////////////
/// \brief This function counts the number of
/// escaped dollar signs.
///
/// \param sLine const string&
/// \return size_t
///
/////////////////////////////////////////////////
size_t countEscapeSymbols(const string& sLine)
{
    size_t nCount = 0;

    // Go through the line
    for (size_t i = 0; i < sLine.length(); i++)
    {
        // If we find a escaped dollar sign, increment the counter
        if (sLine.substr(i, 2) == "\\$")
            nCount++;
    }

    // return the number of escaped dollar signs
    return nCount;
}


/////////////////////////////////////////////////
/// \brief This is a static helper function for
/// the standard qsort algorithm.
///
/// \param p1 const void*
/// \param p2 const void*
/// \return int
///
/////////////////////////////////////////////////
static int compareDouble(const void* p1, const void* p2)
{
    if (isnan(*(double*)p1) && isnan(*(double*)p2))
        return 0;

    if (isnan(*(double*)p1))
        return 1;

    if (isnan(*(double*)p2))
        return -1;

    return *(double*)p1 == *(double*)p2
        ? 0
        : -(*(double*)p1 < *(double*)p2) + (*(double*)p1 > *(double*)p2); // Returns -1 for smaller and +1 for larger
}


/////////////////////////////////////////////////
/// \brief This is a wrapper for the standard
/// qsort algorithm. It returns the number of
/// valid elements and sorts the elements
/// in-place.
///
/// \param dArray double*
/// \param nlength size_t
/// \return size_t
///
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief This static function replaces all
/// search-oriented methods in the current string
/// variable access.
///
/// \param sLine MutableStringView
/// \param nPos size_t
/// \param nFinalPos size_t
/// \param sReplacement const std::string&
/// \param sMethod StringView
/// \param sArgument std::string&
/// \return void
///
/////////////////////////////////////////////////
static void replaceSearchMethods(MutableStringView sLine, size_t nPos, size_t nFinalPos, const std::string& sReplacement, StringView sMethod, std::string& sArgument)
{
    // Prepare the argument (use empty one or construct one
    // from argument and variable value)
    if (sArgument == "()")
        sArgument = "(" + sReplacement + ", " + sReplacement + ")";
    else if (sArgument.find(',') == std::string::npos)
        sArgument.insert(sArgument.length()-1, ", " + sReplacement + "");
    else
    {
        // If we have a comma, it could be part of the only argument,
        // so we remove all parentheses first and recombine everything
        // afterwards
        std::string sTemp = "(";
        sArgument.erase(0, 1);
        sArgument.pop_back();
        sTemp += getNextArgument(sArgument, true);
        sTemp += ", " + sReplacement;

        // If the argument now has no length, it was the only one
        // so simply close the parenthesis. Otherwise append the
        // missing arguments and then close the parenthesis
        if (!sArgument.length())
            sArgument = sTemp + ")";
        else
            sArgument = sTemp + ", " + sArgument + ")";
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


/////////////////////////////////////////////////
/// \brief This static function replaces all
/// access-oriented methods and the string
/// splitter in the current string variable
/// access.
///
/// \param sLine MutableStringView
/// \param nPos size_t
/// \param nFinalPos size_t
/// \param sReplacement const std::string&
/// \param sMethod StringView
/// \param sArgument std::string&
/// \return void
///
/////////////////////////////////////////////////
static void replaceAccessMethods(MutableStringView sLine, size_t nPos, size_t nFinalPos, const std::string& sReplacement, StringView sMethod, std::string& sArgument)
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
    else if (sMethod == "first")
        sLine.replace(nPos, nFinalPos-nPos, "firstch(" + sReplacement + ")");
    else if (sMethod == "last")
        sLine.replace(nPos, nFinalPos-nPos, "lastch(" + sReplacement + ")");
    else if (sMethod == "startsw")
        sLine.replace(nPos, nFinalPos-nPos, "startswith" + sArgument );
    else if (sMethod == "endsw")
        sLine.replace(nPos, nFinalPos-nPos, "endswith" + sArgument );
}


/////////////////////////////////////////////////
/// \brief This function searches the indicated
/// string variable occurence for possible string
/// methods and replaces them with the standard
/// string function.
///
/// \param sLine MutableStringView
/// \param nPos size_t
/// \param nLength size_t
/// \param sReplacement const std::string&
/// \return void
///
/////////////////////////////////////////////////
void replaceStringMethod(MutableStringView sLine, size_t nPos, size_t nLength, const std::string& sReplacement)
{
    // Does the string variable name end with a dot?
    if (sLine[nPos+nLength] != '.')
    {
        sLine.replace(nPos, nLength, sReplacement);
        return;
    }

    static const std::string sDELIMITER = "+-*/ ={}^&|!,\\%#?:\";";
    StringView sMethod;
    std::string sArgument = "";
    size_t nFinalPos = 0;

    // Find the end of the appended method. This is either
    // the closing parenthesis or a delimiter character. Split
    // the found method into its name and its argument
    for (size_t i = nPos+nLength+1; i < sLine.length(); i++)
    {
        if (sLine[i] == '(')
        {
            // Method ends with closing parenthesis
            sMethod = sLine.subview(nPos+nLength+1, i-(nPos+nLength+1));
            sArgument = sLine.subview(i, getMatchingParenthesis(sLine.subview(i))+1).to_string();
            nFinalPos = i += getMatchingParenthesis(sLine.subview(i))+1;
            break;
        }
        else if (sDELIMITER.find(sLine[i]) != std::string::npos)
        {
            // Method ends with a delimiter
            sMethod = sLine.subview(nPos+nLength+1, i-(nPos+nLength+1));
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
    else if (sMethod == "at"
             || sMethod == "sub"
             || sMethod == "splt"
             || sMethod == "first"
             || sMethod == "last"
             || sMethod == "startsw"
             || sMethod == "endsw")
    {
        // Access methods and splitter
        replaceAccessMethods(sLine, nPos, nFinalPos, sReplacement, sMethod, sArgument);
    }
    else if (sMethod == "fnd"
             || sMethod == "rfnd"
             || sMethod == "mtch"
             || sMethod == "rmtch"
             || sMethod == "nmtch"
             || sMethod == "nrmtch"
             )
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

enum
{
    EXEPATH,
    WORKPATH,
    LOADPATH,
    SAVEPATH,
    SCRIPTPATH,
    PROCPATH,
    PLOTPATH,
    PATH_LAST
};

/////////////////////////////////////////////////
/// \brief Removes the part of the path belonging
/// to a default path or shortens it otherwise.
///
/// \param sFullPath const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string removeDefaultPath(const std::string& sFullPath)
{
    std::string sPath = replacePathSeparator(sFullPath);
    std::vector<std::string> vPaths = NumeReKernel::getInstance()->getPathSettings();
    size_t pos = 0;

    for (int i = LOADPATH; i < PATH_LAST; i++)
    {
        if (sPath.starts_with(vPaths[i]))
        {
            pos = vPaths[i].length();

            while (sPath[pos] == '/')
                pos++;

            break;
        }
    }

    // Nothing found-must be an absolute path. We will
    // replace /PATH/ with /../
    if (!pos)
        return shortenFileName(sPath);

    return sPath.substr(pos);
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
    // Increment the version by one (corresponds to
    // the build count)
    return floatToVersion(versionToFloat(_sVer)+1.0);
}

#endif


/////////////////////////////////////////////////
/// \brief This function checks, whether the file
/// with the passed file name exists.
///
/// \param sFilename const string&
/// \return bool
///
/////////////////////////////////////////////////
bool fileExists(const std::string& sFilename)
{
    if (sFilename.length())
    {
        std::string _sFile = sFilename;
        _sFile = fromSystemCodePage(_sFile);

        // Open the ifstream (ifstream doesn't create a file)
        std::ifstream ifFile(_sFile.c_str());
        return ifFile.good(); // If the stream is good(), the file exists
    }
    else
        return false;
}


/////////////////////////////////////////////////
/// \brief This function calculates the power of
/// a value with the specialization that the
/// exponent is an integer.
///
/// \param dNumber double
/// \param nExponent int64_t
/// \return double
///
/////////////////////////////////////////////////
double intPower(double dNumber, int64_t nExponent)
{
    // 0^0 is undefined
    if (nExponent == 0 && dNumber == 0.0)
        return NAN;

    long double dResult = 1.0L;

    // An exponent of zero returns always 1
    if (!nExponent)
        return 1.0;

    // Calculuate the exponentation
    for (int64_t i = abs(nExponent); i > 0; i--)
    {
        dResult *= (long double)dNumber;
    }

    // Apply the sign of the exponent
    if (nExponent > 0)
        return dResult;
    else
        return 1.0 / dResult;
}


/////////////////////////////////////////////////
/// \brief This function calculates the power of
/// a value with the specialization that the
/// exponent is an integer. Function overload for
/// complex-valued bases.
///
/// \param dNumber const std::complex<double>&
/// \param nExponent int64_t
/// \return std::complex<double>
///
/////////////////////////////////////////////////
std::complex<double> intPower(const std::complex<double>& dNumber, int64_t nExponent)
{
    if (dNumber.imag() == 0.0)
        return intPower(dNumber.real(), nExponent);

    std::complex<double> dResult = 1.0;

    // An exponent of zero returns always 1
    if (!nExponent)
        return 1.0;

    // Calculuate the exponentation
    for (int64_t i = abs(nExponent); i > 0; i--)
    {
        dResult *= dNumber;
    }

    // Apply the sign of the exponent
    if (nExponent > 0)
        return dResult;
    else
        return 1.0 / dResult;
}


/////////////////////////////////////////////////
/// \brief Returns the position of the closing
/// parenthesis.
///
/// \param sLine StringView
/// \return size_t The position of the closing parenthesis
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
size_t getMatchingParenthesis(const StringView& sLine)
{
    size_t pos = sLine.find_first_of("([{");

    if (pos == std::string::npos)
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
    return std::string::npos;
}


/////////////////////////////////////////////////
/// \brief This function determines, if the
/// passed character is a delimiter character.
///
/// \param c char
/// \return bool
///
/////////////////////////////////////////////////
bool isDelimiter(char c)
{
    // Characters converted to a single logical expression (should be faster in principle)
    return c >= 32 && c <= 125 && c != 36 && c != 39 && c != 46 && (c < 48 || c > 57) && (c < 64 || c > 90) && (c < 95 || c > 122);
    // Only construct the string once
    //static string sDelimiter = "+-*/ ^&|!%<>,=\\#?:;()[]{}\"";

    // Try to find the current character
    /*if (sDelimiter.find(cChar) != string::npos)
        return true;
    return false;*/
}


/////////////////////////////////////////////////
/// \brief This function gets the first argument
/// in the passed argument list if the boolean
/// bCut is true then the argument is erased from
/// the string
///
/// \param sArgList StringView&
/// \param cSep char
/// \return StringView
///
/////////////////////////////////////////////////
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
        if ((sArgList[i] == '(' || sArgList[i] == '[' || sArgList[i] == '{' )
             && (nMatching = getMatchingParenthesis(sArgList.subview(i))) != std::string::npos)
            i += nMatching;

        // A separator was found -> break the loop
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


std::string getNextArgument(std::string& sArgList, bool bCut)
{
    StringView argList(sArgList);
    std::string sArg = getNextCommandLineToken(argList, ',').to_string();

    if (bCut)
        sArgList = argList.to_string();

    return sArg;
}

std::string getNextIndex(std::string& sArgList, bool bCut)
{
    StringView argList(sArgList);
    std::string sArg = getNextCommandLineToken(argList, ':').to_string();

    if (bCut)
        sArgList = argList.to_string();

    return sArg;
}

std::string getNextSemiColonSeparatedToken(std::string& sArgList, bool bCut)
{
    StringView argList(sArgList);
    std::string sArg = getNextCommandLineToken(argList, ';').to_string();

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
/// \param sArgList std::string
/// \return EndlessVector<std::string>
///
/////////////////////////////////////////////////
EndlessVector<std::string> getAllArguments(std::string sArgList)
{
    EndlessVector<std::string> vArgs;

    while (sArgList.length())
        vArgs.push_back(getNextArgument(sArgList, true));

    return vArgs;
}


/////////////////////////////////////////////////
/// \brief Splits up the complete index list
/// and returns them as an EndlessVector.
///
/// \param sArgList std::string
/// \return EndlessVector<string>
///
/////////////////////////////////////////////////
EndlessVector<std::string> getAllIndices(std::string sArgList)
{
    EndlessVector<std::string> vIndices;

    while (sArgList.length())
        vIndices.push_back(getNextIndex(sArgList, true));

    return vIndices;
}


/////////////////////////////////////////////////
/// \brief Splits up the complete index list
/// and returns them as an EndlessVector.
///
/// \param sArgList string
/// \return EndlessVector<std::string>
///
/////////////////////////////////////////////////
EndlessVector<std::string> getAllSemiColonSeparatedTokens(std::string sArgList)
{
    EndlessVector<std::string> vIndices;

    while (sArgList.length())
        vIndices.push_back(getNextSemiColonSeparatedToken(sArgList, true));

    return vIndices;
}


/////////////////////////////////////////////////
/// \brief Determines, whether the content of a
/// complex value is actually a regular integer.
///
/// \param number const std::complex<double>&
/// \return bool
///
/////////////////////////////////////////////////
bool isInt(const std::complex<double>& number)
{
    return number.imag() == 0.0 && fabs(number.real() - rint(number.real())) < 1e-12;
}


/////////////////////////////////////////////////
/// \brief Return a new string-encoded UUID in V4
/// format.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string getUuidV4()
{
    std::mt19937& generator = getRandGenInstance();
    uuids::uuid_random_generator gen{generator};

    const uuids::uuid id = gen();

    return uuids::to_string(id);
}




