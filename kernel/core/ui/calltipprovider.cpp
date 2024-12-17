/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2022  Erik Haenel et al.

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

#include "calltipprovider.hpp"
#include "language.hpp"
#include "../utils/tools.hpp"
#include "../../kernel.hpp"


#include <fstream>
#include <regex>

extern Language _lang;

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief Converts the language file
    /// documentation layout into something more
    /// fitting for the calltips in the graphical
    /// user interface.
    ///
    /// \param sLine std::string
    /// \return CallTip
    ///
    /////////////////////////////////////////////////
    static CallTip realignLangString(std::string sLine)
    {
        size_t lastpos = std::string::npos;

        for (size_t i = 0; i < sLine.length(); i++)
        {
            size_t pos;

            if (sLine[i] == '(' && (pos = getMatchingParenthesis(StringView(sLine, i))) != std::string::npos)
                i += getMatchingParenthesis(StringView(sLine, i));

            if (sLine[i] == ' ')
            {
                lastpos = i;
                break;
            }
        }

        if (lastpos == std::string::npos)
            return {sLine, "", 0, sLine.length()};

        CallTip _cTip;

        _cTip.sDefinition = sLine.substr(0, sLine.find("- "));
        StripSpaces(_cTip.sDefinition);

        if (sLine.find("- ") != std::string::npos)
            _cTip.sDocumentation = sLine.substr(sLine.find("- ")+2);

        _cTip.nStart = 0;
        _cTip.nEnd = lastpos;

        // Find the first non-whitespace character
        size_t firstpos = _cTip.sDefinition.find_first_not_of(' ', lastpos);

        if (firstpos == std::string::npos)
            return _cTip;

        // Insert separation characters between syntax element
        // and return values for functions
        if (_cTip.sDefinition.find(')') < lastpos || _cTip.sDefinition.find('.') < lastpos)
            _cTip.sDefinition.replace(lastpos, firstpos - lastpos, " -> ");
        else if (firstpos - lastpos > 2)
                _cTip.sDefinition.erase(lastpos, firstpos - lastpos - 1);

        return _cTip;
    }


    /////////////////////////////////////////////////
    /// \brief Appends the new documentation line to
    /// the already existing line.
    ///
    /// \param sDocumentation std::string&
    /// \param sNewDocLine const std::string&
    /// \return void
    ///
    /// This function appends a found documentation
    /// line to the overall documentation and
    /// converts some TeX-commands into plain text
    /// and applies a rudimentary styling.
    /////////////////////////////////////////////////
    void AppendToDocumentation(std::string& sDocumentation, const std::string& sNewDocLine)
    {
        static bool bBeginEnd = false;
        static size_t nLastIndent = 0;

        if (sNewDocLine.find_first_not_of(" \t") == std::string::npos)
        {
            if (sDocumentation.length())
                sDocumentation += "\n    ";

            nLastIndent = 0;
            return;
        }

        // Handle some special TeX commands and rudimentary lists
        if (sNewDocLine.find("\\begin{") != std::string::npos && sNewDocLine.find("\\end{") == std::string::npos)
        {
            if (sDocumentation.length() && sDocumentation.back() != '\n')
                sDocumentation += "\n    ";

            nLastIndent = 0;
            bBeginEnd = true;
        }
        else if (sNewDocLine.find("\\begin{") == std::string::npos && sNewDocLine.find("\\end{") != std::string::npos)
        {
            if (sDocumentation.length() && sDocumentation.back() != '\n')
                sDocumentation += "\n    ";

            nLastIndent = 0;
            bBeginEnd = false;
        }
        else if ((sNewDocLine.length()
                  && (sNewDocLine.substr(sNewDocLine.find_first_not_of(" \t"), 7) == "\\param "
                      || sNewDocLine.substr(sNewDocLine.find_first_not_of(" \t"), 8) == "\\remark ")) || bBeginEnd)
        {
            nLastIndent = 0;

            if (sDocumentation.length() && sDocumentation.back() != '\n')
                sDocumentation += "\n    ";
        }
        else if (sNewDocLine.length()
                 && std::regex_match(sNewDocLine.substr(sNewDocLine.find_first_not_of(" \t"), 10), std::regex("(\\d+\\.|-) +.+")))
        {
            nLastIndent = sNewDocLine.find_first_not_of(" \t", sNewDocLine.find_first_of(".-")+1);

            if (sDocumentation.length() && sDocumentation.back() != '\n')
                sDocumentation += "\n    ";
        }
        else if (nLastIndent && sNewDocLine.find_first_not_of(" \t") != nLastIndent)
        {
            nLastIndent = 0;

            if (sDocumentation.length() && sDocumentation.back() != '\n')
                sDocumentation += "\n    ";
        }
        else
        {
            if (sDocumentation.length() && sDocumentation.back() != ' ')
                sDocumentation += " ";
        }

        sDocumentation += sNewDocLine.substr(sNewDocLine.find_first_not_of(" \t"));
        size_t nPos = sDocumentation.find("\\procedure{");

        if (nPos != std::string::npos)
            sDocumentation.erase(nPos, sDocumentation.find('}', nPos)+1 - nPos);
        else if ((nPos = sDocumentation.find("\\procedure ")) != std::string::npos)
        {
            size_t nPos2 = nPos + 10;
            nPos2 = sDocumentation.find_first_not_of(" \r\n", nPos2);
            nPos2 = sDocumentation.find_first_of(" \r\n", nPos2);
            sDocumentation.erase(nPos, nPos2-nPos);
        }
    }


    /////////////////////////////////////////////////
    /// \brief Checks layout and finishes styling of
    /// the documentation string.
    ///
    /// \param sDocumentation std::string
    /// \param sReturns std::string&
    /// \return std::string
    ///
    /// This function checks the layout of the
    /// found documentations and applies some special
    /// modifications. Furthermore, the results
    /// definition is returned separately.
    /////////////////////////////////////////////////
    static std::string CleanDocumentation(std::string sDocumentation, std::string& sReturns)
    {
        if (sDocumentation.find_first_not_of(" \n") != std::string::npos)
        {
            // Clean whitespace before and after the documentation
            sDocumentation.erase(0, sDocumentation.find_first_not_of(" \n"));

            if (sDocumentation.back() == ' ' || sDocumentation.back() == '\n')
                sDocumentation.erase(sDocumentation.find_last_not_of(" \n")+1);

            size_t nPos = sDocumentation.find("\\param ");

            // Resolve "\param" keywords
            if (nPos != std::string::npos)
            {
                // Insert a headline above the first parameter
                if (nPos > 5 && sDocumentation.substr(nPos-5, 5) != "\n    ")
                    sDocumentation.insert(nPos, "\n    " + toUpperCase(_lang.get("GUI_EDITOR_CALLTIP_PROC_PARAMS")) + "\n    ");
                else
                    sDocumentation.insert(nPos, toUpperCase(_lang.get("GUI_EDITOR_CALLTIP_PROC_PARAMS")) + "\n    ");

                while ((nPos = sDocumentation.find("\\param ")) != std::string::npos)
                {
                    sDocumentation.replace(nPos, 6, "-");
                    size_t spacePos = sDocumentation.find(' ', sDocumentation.find_first_not_of(' ', nPos+1));

                    if (spacePos == std::string::npos)
                        break;

                    sDocumentation.insert(spacePos, ":");

                    if (sDocumentation[sDocumentation.find_first_not_of(' ', nPos+1)] == '_')
                        sDocumentation.erase(sDocumentation.find_first_not_of(' ', nPos+1), 1);
                }
            }

            // Extract \return
            while ((nPos = sDocumentation.find("\\return ")) != std::string::npos)
            {
                // Find the next \return or \remark alternatively
                // This is a candidate for issues with new keywords
                size_t newReturn = std::min(sDocumentation.find("\\return ", nPos+1), sDocumentation.find("\\remark ", nPos+1));

                // Extract the current return statement and
                // remove the corresponding part from the
                // documentation
                std::string sCurrReturn = sDocumentation.substr(nPos+8, newReturn-nPos-8);
                sDocumentation.erase(nPos, newReturn-nPos);

                while (sDocumentation.front() == ' ')
                    sDocumentation.erase(0, 1);

                if (sReturns.length())
                    sReturns += ",";

                sReturns += sCurrReturn.substr(0, sCurrReturn.find(' '));
            }

            // Replace \remark
            while ((nPos = sDocumentation.find("\\remark ")) != std::string::npos)
                sDocumentation.replace(nPos, 7, toUpperCase(_lang.get("GUI_EDITOR_CALLTIP_PROC_REMARK"))+":");

            // Remove doubled exclamation marks
            while ((nPos = sDocumentation.find("!!")) != std::string::npos)
                sDocumentation.erase(nPos, 2);

            // Replace \begin{} and \end{} with line breaks
            // This logic bases upon the replacements done
            // in NumeReEditor::AppendToDocumentation
            size_t nMatch = 0;

            while ((nMatch = sDocumentation.find("\\begin{")) != std::string::npos)
            {
                sDocumentation.erase(nMatch, sDocumentation.find('}', nMatch) + 1 - nMatch);

                if (sDocumentation.substr(nMatch, 5) == "\n    ")
                    sDocumentation.erase(nMatch, 5);
            }

            while ((nMatch = sDocumentation.find("\\end{")) != std::string::npos)
            {
                sDocumentation.erase(nMatch, sDocumentation.find('}', nMatch) + 1 - nMatch + 1);
            }

            // Insert returns
            if (sReturns.length())
            {
                if (sReturns.find(',') == std::string::npos)
                    sReturns = "-> " + sReturns;
                else
                    sReturns = "-> {" + sReturns + "}";
            }
        }
        else
            sDocumentation.clear();

        return sDocumentation;
    }


    /////////////////////////////////////////////////
    /// \brief Search the procedure definition in a
    /// global file.
    ///
    /// \param pathname const std::string&
    /// \param procedurename const std::string&
    /// \return CallTip
    ///
    /// This function searches for the procedure
    /// definition in a selected global procedure
    /// file. It also adds the documentation to the
    /// CallTip, so that it might be shown in the
    /// tooltip.
    /////////////////////////////////////////////////
    CallTip FindProcedureDefinition(const std::string& pathname, const std::string& procedurename)
    {
        if (!fileExists(pathname + ".nprc"))
            return CallTip();
        else
        {
            CallTip _cTip;
            std::ifstream procedure_in;
            std::string sBuffer;
            bool bBlockComment = false;
            std::string sDocumentation;
            bool bDocFound = false;
            procedure_in.open(pathname + ".nprc");

            // Ensure that the file is in good state
            if (!procedure_in.good())
                return CallTip();

            // As long as we're not at the end of the file
            while (!procedure_in.eof())
            {
                // Read one line and strip all spaces
                std::getline(procedure_in, sBuffer);
                StringView sProcCommandLine(sBuffer);
                sProcCommandLine.strip();

                // Ignore empty lines
                if (!sProcCommandLine.length())
                {
                    _cTip.sDocumentation.clear();
                    bDocFound = false;
                    continue;
                }

                // Ignore comment lines
                if (sProcCommandLine.starts_with("##"))
                {
                    // Append each documentation string
                    if (sProcCommandLine.starts_with("##!"))
                        AppendToDocumentation(_cTip.sDocumentation, sProcCommandLine.subview(3).to_string());

                    continue;
                }

                // Erase line comment parts
                if (sProcCommandLine.find("##") != std::string::npos)
                    sProcCommandLine.trim_back(sProcCommandLine.find("##"));

                // Remove block comments and continue
                if (sProcCommandLine.starts_with("#*") && sProcCommandLine.find("*#", 2) == std::string::npos)
                {
                    if (sProcCommandLine.starts_with("#*!"))
                    {
                        bDocFound = true;
                        AppendToDocumentation(_cTip.sDocumentation, sProcCommandLine.subview(3).to_string());
                    }

                    bBlockComment = true;
                    continue;
                }

                // Search for the end of the current block comment
                if (bBlockComment && sProcCommandLine.find("*#") != std::string::npos)
                {
                    bBlockComment = false;

                    if (bDocFound)
                        AppendToDocumentation(_cTip.sDocumentation, sProcCommandLine.get_viewed_string().substr(0, sBuffer.find("*#")));
                    if (sProcCommandLine.find("*#") == sProcCommandLine.length() - 2)
                        continue;
                    else
                        sProcCommandLine = sProcCommandLine.subview(sProcCommandLine.find("*#") + 2);
                }
                else if (bBlockComment && sProcCommandLine.find("*#") == std::string::npos)
                {
                    // if the documentation has a length, append the current block
                    if (bDocFound)
                        AppendToDocumentation(_cTip.sDocumentation, sProcCommandLine.get_viewed_string());

                    continue;
                }

                // Ignore includes
                if (sProcCommandLine[0] != '@' && findCommand(sProcCommandLine).sString != "procedure")
                {
                    _cTip.sDocumentation.clear();
                    bDocFound = false;
                    continue;
                }
                else if (sProcCommandLine[0] == '@')
                {
                    _cTip.sDocumentation.clear();
                    bDocFound = false;
                    continue;
                }

                // Ignore lines without "procedure"
                if (findCommand(sProcCommandLine).sString != "procedure")
                {
                    _cTip.sDocumentation.clear();
                    bDocFound = false;
                    continue;
                }

                // Search for the current procedure name
                if (sProcCommandLine.find(procedurename) == std::string::npos || sProcCommandLine.find('(') == std::string::npos)
                {
                    // clear the documentation string
                    _cTip.sDocumentation.clear();
                    bDocFound = false;
                    continue;
                }
                else
                {
                    // Found the procedure name, now extract the definition
                    if (getMatchingParenthesis(sProcCommandLine.subview(sProcCommandLine.find(procedurename))) == std::string::npos)
                        return CallTip();

                    _cTip.sDefinition = sProcCommandLine.subview(sProcCommandLine.find(procedurename),
                                                                 getMatchingParenthesis(sProcCommandLine.subview(sProcCommandLine.find(procedurename))) + 1).to_string();
                    size_t nFirstParens = _cTip.sDefinition.find('(');
                    std::string sArgList = _cTip.sDefinition.substr(nFirstParens + 1, getMatchingParenthesis(StringView(_cTip.sDefinition, nFirstParens)) - 1);
                    _cTip.sDefinition.erase(nFirstParens + 1);

                    while (sArgList.length())
                    {
                        std::string currentarg = getNextArgument(sArgList, true);

                        if (currentarg.front() == '_')
                            currentarg.erase(0, 1);

                        _cTip.sDefinition += currentarg;

                        if (sArgList.length())
                            _cTip.sDefinition += ", ";
                    }

                    _cTip.sDefinition += ")";

                    if (sProcCommandLine.find("::") != std::string::npos)
                    {
                        std::string sFlags = sProcCommandLine.subview(sProcCommandLine.find("::") + 2).to_string();

                        if (sFlags.find("##") != std::string::npos)
                            sFlags.erase(sFlags.find("##"));

                        StripSpaces(sFlags);
                        _cTip.sDefinition += " :: " + sFlags;
                    }

                    // If no documentation was found, search in the following lines
                    if (!_cTip.sDocumentation.length())
                    {
                        while (!procedure_in.eof())
                        {
                            std::getline(procedure_in, sBuffer);
                            sProcCommandLine = sBuffer;
                            sProcCommandLine.strip();

                            if (sProcCommandLine.starts_with("##!"))
                                AppendToDocumentation(_cTip.sDocumentation, sProcCommandLine.subview(3).to_string());
                            else if (sProcCommandLine.starts_with("#*!"))
                            {
                                AppendToDocumentation(_cTip.sDocumentation, sProcCommandLine.subview(3).to_string());
                                bBlockComment = true;
                            }
                            else if (bBlockComment)
                            {
                                AppendToDocumentation(_cTip.sDocumentation, sBuffer.substr(0, sBuffer.find("*#")));

                                if (sProcCommandLine.find("*#") != std::string::npos)
                                    break;
                            }
                            else
                                break;
                        }
                    }

                    std::string sReturns;
                    // clean the documentation
                    _cTip.sDocumentation = CleanDocumentation(_cTip.sDocumentation, sReturns);
                    _cTip.nStart = 0;
                    _cTip.nEnd = std::min(_cTip.sDefinition.length(), _cTip.sDefinition.find("::"));

                    // if the documentation has a length, append it here
                    if (sReturns.length())
                        _cTip.sDefinition += " " + sReturns;

                    return _cTip;
                }
            }
        }

        return CallTip();
    }


    /////////////////////////////////////////////////
    /// \brief Adds the necessary linebreaks to the
    /// documentation part of the CallTip to fit
    /// inside the desired maximal line length.
    ///
    /// \param _cTip CallTip
    /// \param maxLineLength size_t
    /// \return CallTip
    ///
    /////////////////////////////////////////////////
    CallTip addLinebreaks(CallTip _cTip, size_t maxLineLength)
    {
        // Remove escaped dollar signs
        replaceAll(_cTip.sDefinition, "\\$", "$");
        replaceAll(_cTip.sDocumentation, "\\$", "$");

        if (!_cTip.sDocumentation.length())
            return _cTip;

        _cTip.sDocumentation.insert(0, "    ");

        size_t nIndentPos = 4;
        size_t nLastLineBreak = 0;
        size_t nAddIndent = 0;
        std::string& sReturn = _cTip.sDocumentation;
        static const std::regex expr("    (\\d+\\.|-) +(?=\\S+)");
        std::smatch match;

        nLastLineBreak = 0;

        for (size_t i = 0; i < sReturn.length(); i++)
        {
            if (sReturn[i] == '\n')
            {
                nLastLineBreak = i;
                std::string sCandidate = sReturn.substr(i+1, 15);

                if (std::regex_search(sCandidate, match, expr) && match.position(0) == 0)
                    nAddIndent = match.length(0)-4;
                else
                    nAddIndent = 0;
            }

            if ((i == maxLineLength && !nLastLineBreak)
                    || (nLastLineBreak && i - nLastLineBreak == maxLineLength))
            {
                for (int j = i; j >= 0; j--)
                {
                    if (sReturn[j] == ' ')
                    {
                        sReturn[j] = '\n';
                        sReturn.insert(j + 1, nIndentPos + nAddIndent, ' ');
                        nLastLineBreak = j;
                        break;
                    }
                    else if (sReturn[j] == '-' && j != (int)i)
                    {
                        // --> Minuszeichen: nicht immer ist das Trennen an dieser Stelle sinnvoll. Wir pruefen die einfachsten Faelle <--
                        if (j &&
                                (sReturn[j - 1] == ' '
                                 || sReturn[j - 1] == '('
                                 || sReturn[j + 1] == ')'
                                 || sReturn[j - 1] == '['
                                 || (sReturn[j + 1] >= '0' && sReturn[j + 1] <= '9')
                                 || sReturn[j + 1] == ','
                                 || (sReturn[j + 1] == '"' && sReturn[j - 1] == '"')
                                ))
                            continue;

                        sReturn.insert(j + 1, "\n");
                        sReturn.insert(j + 2, nIndentPos + nAddIndent, ' ');
                        nLastLineBreak = j + 1;
                        break;
                    }
                    else if (sReturn[j] == ',' && j != (int)i && sReturn[j + 1] != ' ')
                    {
                        sReturn.insert(j + 1, "\n");
                        sReturn.insert(j + 2, nIndentPos + nAddIndent, ' ');
                        nLastLineBreak = j + 1;
                        break;
                    }
                }
            }
        }

        return _cTip;
    }


    /////////////////////////////////////////////////
    /// \brief Get the calltip for the selected
    /// command.
    ///
    /// \param sToken std::string
    /// \return CallTip
    ///
    /////////////////////////////////////////////////
    CallTip CallTipProvider::getCommand(std::string sToken) const
    {
        if (sToken == "showf")
            sToken = "show";
        else if (sToken == "view")
            sToken = "edit";
        else if (sToken == "undef")
            sToken = "undefine";
        else if (sToken == "ifndef")
            sToken = "ifndefined";
        else if (sToken == "redef")
            sToken = "redefine";
        else if (sToken == "del")
            sToken = "delete";
        else if (sToken == "search")
            sToken = "find";
        else if (sToken == "vector")
            sToken = "vect";
        else if (sToken == "vector3d")
            sToken = "vect3d";
        else if (sToken == "graph")
            sToken = "plot";
        else if (sToken == "graph3d")
            sToken = "plot3d";
        else if (sToken == "gradient")
            sToken = "grad";
        else if (sToken == "gradient3d")
            sToken = "grad3d";
        else if (sToken == "surface")
            sToken = "surf";
        else if (sToken == "surface3d")
            sToken = "surf3d";
        else if (sToken == "meshgrid")
            sToken = "mesh";
        else if (sToken == "meshgrid3d")
            sToken = "mesh3d";
        else if (sToken == "density")
            sToken = "dens";
        else if (sToken == "density3d")
            sToken = "dens3d";
        else if (sToken == "contour")
            sToken = "cont";
        else if (sToken == "contour3d")
            sToken = "cont3d";
        else if (sToken == "mtrxop")
            sToken = "matop";
        else if (sToken == "man")
            sToken = "help";
        else if (sToken == "credits" || sToken == "info")
            sToken = "about";
        else if (sToken == "integrate2" || sToken == "integrate2d")
            sToken = "integrate";

        return addLinebreaks(realignLangString(_lang.get("PARSERFUNCS_LISTCMD_CMD_" + toUpperCase(sToken) + "_*")), m_maxLineLength);
    }


    /////////////////////////////////////////////////
    /// \brief Get the calltip for the selected
    /// (built-in) function.
    ///
    /// \param sToken std::string
    /// \return CallTip
    ///
    /////////////////////////////////////////////////
    CallTip CallTipProvider::getFunction(std::string sToken) const
    {
        // Handle aliases
        if (sToken == "arcsin")
            sToken = "asin";
        else if (sToken == "arccos")
            sToken = "acos";
        else if (sToken == "arctan")
            sToken = "atan";
        else if (sToken == "arsinh")
            sToken = "asinh";
        else if (sToken == "arcosh")
            sToken = "acosh";
        else if (sToken == "artanh")
            sToken = "atanh";
        else if (sToken == "ceil")
            sToken = "roof";

        if (m_returnUnmatchedTokens
            || _lang.get("PARSERFUNCS_LISTFUNC_FUNC_" + toUpperCase(sToken) + "_[*") != "PARSERFUNCS_LISTFUNC_FUNC_" + toUpperCase(sToken) + "_[*")
            return addLinebreaks(realignLangString(_lang.get("PARSERFUNCS_LISTFUNC_FUNC_" + toUpperCase(sToken) + "_[*")), m_maxLineLength);

        return CallTip();
    }


    /////////////////////////////////////////////////
    /// \brief Get the calltip for the selected
    /// (global) procedure.
    ///
    /// \param sToken std::string
    /// \return CallTip
    ///
    /////////////////////////////////////////////////
    CallTip CallTipProvider::getProcedure(std::string sToken) const
    {
        std::string sProcPath = NumeReKernel::getInstance()->getSettings().getProcPath();
        std::string pathname = sToken;
        std::string procedurename = pathname.substr(pathname.rfind('~') + 1); // contains a "$", if it's not used for the "thisfile~" case

        // Handle the namespaces
        if (pathname.find("$this~") != std::string::npos)
        {
            // This namespace (the current folder)
            return CallTip();
        }
        else if (pathname.find("$thisfile~") != std::string::npos)
        {
            // local namespace
            return CallTip();
        }
        else
        {
            // All other namespaces
            if (pathname.find("$main~") != std::string::npos)
                pathname.erase(pathname.find("$main~") + 1, 5);

            while (pathname.find('~') != std::string::npos)
                pathname[pathname.find('~')] = '/';

            // Add the root folders to the path name
            if (pathname[0] == '$' && pathname.find(':') == std::string::npos)
                pathname.replace(0, 1, sProcPath + "/");
            else if (pathname.find(':') == std::string::npos)
                pathname.insert(0, sProcPath);
            else // pathname.find(':') != string::npos
            {
                // Absolute file paths
                pathname = pathname.substr(pathname.find('\'') + 1, pathname.rfind('\'') - pathname.find('\'') - 1);
            }
        }

        // Find the namespace in absolute procedure paths
        while (procedurename.find('\'') != std::string::npos)
            procedurename.erase(procedurename.find('\''), 1);

        if (procedurename.find('/') != std::string::npos)
            procedurename = "$" + procedurename.substr(procedurename.rfind('/') + 1);

        if (procedurename.find('\\') != std::string::npos)
            procedurename = "$" + procedurename.substr(procedurename.rfind('\\') + 1);

        if (procedurename[0] != '$')
            procedurename.insert(0, 1, '$');

        // Find procedure in a global procedure file
        return addLinebreaks(FindProcedureDefinition(pathname, procedurename), m_maxLineLength);
    }


    /////////////////////////////////////////////////
    /// \brief Get the calltip for the selected
    /// option.
    ///
    /// \param sToken std::string
    /// \return CallTip
    ///
    /////////////////////////////////////////////////
    CallTip CallTipProvider::getOption(std::string sToken) const
    {
        sToken = _lang.get("GUI_EDITOR_CALLTIP_OPT_" + toUpperCase(sToken));

        size_t highlightlength = sToken.length();

        if (sToken.find(' ') != std::string::npos)
            highlightlength = sToken.find(' ');

        return {sToken, "", 0, highlightlength};
    }


    /////////////////////////////////////////////////
    /// \brief Get the calltip for the selected
    /// method.
    ///
    /// \param sToken std::string
    /// \return CallTip
    ///
    /////////////////////////////////////////////////
    CallTip CallTipProvider::getMethod(std::string sToken) const
    {
        static const char* pref = "PARSERFUNCS_LISTFUNC_METHOD_";

        if (!m_returnUnmatchedTokens
            && _lang.get(pref + toUpperCase(sToken) + "_*") == pref + toUpperCase(sToken) + "_*")
            return CallTip();

        if (_lang.get(pref + toUpperCase(sToken) + "_[ANY]") != pref + toUpperCase(sToken) + "_[ANY]")
            sToken = "VAR." + _lang.get(pref + toUpperCase(sToken) + "_[ANY]");
        else if (_lang.get(pref + toUpperCase(sToken) + "_[STRING]") != pref + toUpperCase(sToken) + "_[STRING]")
            sToken = "STRINGVAR." + _lang.get(pref + toUpperCase(sToken) + "_[STRING]");
        else
            sToken = "TABLE()." + _lang.get(pref + toUpperCase(sToken) + "_[DATA]");

        CallTip _cTip = addLinebreaks(realignLangString(sToken), m_maxLineLength);
        _cTip.nStart = _cTip.sDefinition.find('.')+1;

        return _cTip;
    }


    /////////////////////////////////////////////////
    /// \brief Get the calltip for the selected
    /// predefined symbol.
    ///
    /// \param sToken std::string
    /// \return CallTip
    ///
    /////////////////////////////////////////////////
    CallTip CallTipProvider::getPredef(std::string sToken) const
    {
        return addLinebreaks(realignLangString(_lang.get("GUI_EDITOR_CALLTIP_" + toUpperCase(sToken))), m_maxLineLength);
    }


    /////////////////////////////////////////////////
    /// \brief Get the calltip for the selected
    /// constant.
    ///
    /// \param sToken std::string
    /// \return CallTip
    ///
    /////////////////////////////////////////////////
    CallTip CallTipProvider::getConstant(std::string sToken) const
    {
        CallTip _cTip;

        if (sToken.front() != '_')
            sToken.insert(0, 1, '_');

        if (sToken == "_G")
            _cTip.sDefinition = _lang.get("GUI_EDITOR_CALLTIP_CONST_GRAV_*");
        else
            _cTip.sDefinition = _lang.get("GUI_EDITOR_CALLTIP_CONST" + toUpperCase(sToken) + "_*");

        _cTip.nStart = 0;
        _cTip.nEnd = _cTip.sDefinition.find('=');

        return _cTip;
    }



}


