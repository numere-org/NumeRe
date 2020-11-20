/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

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

#include "stringvarfactory.hpp"
#include "../../kernel.hpp"

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief This member function is used to determine,
    /// whether a string vector component is numerical
    /// parsable.
    ///
    /// \param sComponent const string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StringVarFactory::isNumericCandidate(const string& sComponent)
    {
        if (sComponent.front() != '"' && sComponent.find_first_of("+-*/^!&|<>=% ?:,") != string::npos)
            return true;

        return false;
    }


    /////////////////////////////////////////////////
    /// \brief This member function is used to create
    /// a string vector variable.
    ///
    /// \param vStringVector const vector<string>&
    /// \return string
    ///
    /////////////////////////////////////////////////
    string StringVarFactory::createStringVectorVar(const vector<string>& vStringVector)
    {
        // Return, if empty (something else went wrong)
        if (!vStringVector.size())
            return "";

        string strVectName = "_~STRVECT[" + toString((int)m_mStringVectorVars.size()) + "]";

        // Does it already exist?
        if (m_mStringVectorVars.find(strVectName) != m_mStringVectorVars.end())
            throw SyntaxError(SyntaxError::STRING_ERROR, strVectName, SyntaxError::invalid_position);

        // save the vector
        m_mStringVectorVars[strVectName] = vStringVector;

        return strVectName;
    }


    /////////////////////////////////////////////////
    /// \brief This member function determines, whether
    /// there are string vector variables in the passed
    /// command line
    ///
    /// \param sLine const string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StringVarFactory::containsStringVectorVars(const string& sLine)
    {
        // If the command line or the string vector vars are empty return false
        if (!sLine.length() || (!m_mStringVectorVars.size() && !m_mTempStringVectorVars.size()))
            return false;

        // Try to find at least one of the known string vector vars
        for (auto iter = m_mTempStringVectorVars.begin(); iter != m_mTempStringVectorVars.end(); ++iter)
        {
            // There's one: return true
            if (sLine.find(iter->first) != string::npos)
                return true;
        }

        // Try to find at least one of the known string vector vars
        for (auto iter = m_mStringVectorVars.begin(); iter != m_mStringVectorVars.end(); ++iter)
        {
            // There's one: return true
            if (sLine.find(iter->first) != string::npos)
                return true;
        }

        // Nothing found
        return false;
    }


    /////////////////////////////////////////////////
    /// \brief This member function evaluates the
    /// passed string vector and returns it's evaluated
    /// components in separate vector components.
    ///
    /// \param sLine string
    /// \return vector<string>
    ///
    /// For each contained vector, the complete command
    /// line is evaluated and returned as a separate
    /// vector component.
    /////////////////////////////////////////////////
    vector<string> StringVarFactory::evaluateStringVectors(string sLine)
    {
        vector<string> vRes;
        const map<string, vector<double> >& mNumVectorVars = NumeReKernel::getInstance()->getParser().GetVectors();

        // As long as the current vector is not empty
        while (sLine.length())
        {
            // Get-cut the next argument
            string sCurrentComponent = getNextArgument(sLine, true);

            // If the current component does not contain any further
            // vectors, push it back.
            // Otherwise expand the contained vectors
            if (!containsStringVectorVars(sCurrentComponent))
                vRes.push_back(sCurrentComponent);
            else
            {
                // Expand the contained vectors
                size_t nCurrentComponent = 0;

                // This while loop is terminated with a break command
                while (true)
                {
                    string currentline = sCurrentComponent;
                    bool bHasComponents = false;

                    // Replace all internal vectors
                    replaceStringVectorVars(m_mStringVectorVars, currentline, nCurrentComponent, bHasComponents);

                    // Replace all temporary vectors
                    replaceStringVectorVars(m_mTempStringVectorVars, currentline, nCurrentComponent, bHasComponents);

                    // Replace all found numerical vectors with their nCurrentComponent-th component
                    for (auto iter = mNumVectorVars.begin(); iter != mNumVectorVars.end(); ++iter)
                    {
                        size_t nMatch = 0;

                        // Vector found: replace its occurence with its nCurrentComponent-th value
                        while ((nMatch = currentline.find(iter->first, nMatch)) != string::npos)
                        {
                            // Do not replace vector variables, which are part
                            // of a string
                            if (isInQuotes(currentline, nMatch)
                                || (nMatch && !isDelimiter(currentline[nMatch-1]))
                                || (nMatch + iter->first.length() < currentline.length() && !isDelimiter(currentline[nMatch+iter->first.length()])))
                            {
                                nMatch++;
                                continue;
                            }

                            // Handle the size of the current vector correspondingly
                            if ((iter->second).size() > nCurrentComponent)
                            {
                                bHasComponents = true;
                                currentline.replace(nMatch, (iter->first).length(), toCmdString((iter->second)[nCurrentComponent]));
                            }
                            else if ((iter->second).size() == 1)
                                currentline.replace(nMatch, (iter->first).length(), toCmdString((iter->second)[0]));
                            else
                                currentline.replace(nMatch, (iter->first).length(), "nan");
                        }
                    }

                    // Break the loop, if there are no further vector components
                    if (!bHasComponents)
                        break;

                    // Push back the current line and increment the component
                    vRes.push_back(currentline);
                    nCurrentComponent++;
                }
            }
        }

        return vRes;
    }


    /////////////////////////////////////////////////
    /// \brief This member function expands the
    /// internal multi-expressions in the veector into
    /// separate components by enlarging the dimension
    /// of the vector.
    ///
    /// \param vStringVector vector<string>&
    /// \return void
    ///
    /// The components are evaluated and inserted into
    /// the original vector, replacing the original
    /// component with the calculated vector n components,
    /// which will enlarge the original vector by n-1
    /// new components.
    /////////////////////////////////////////////////
    void StringVarFactory::expandStringVectorComponents(vector<string>& vStringVector)
    {
        // Examine each vector component
        for (size_t i = 0; i < vStringVector.size(); i++)
        {
            // If the next argument is not equal to the current component itself
            if (getNextArgument(vStringVector[i], false) != vStringVector[i])
            {
                // Store the first part of the multi-expression in the current component
                string sComponent = vStringVector[i];
                vStringVector[i] = getNextArgument(sComponent, true);
                size_t nComponent = 1;

                // As long as the component is not empty
                while (sComponent.length())
                {
                    // Get the next argument and insert it after the
                    // previous argument into the string vector
                    vStringVector.insert(vStringVector.begin() + i + nComponent, getNextArgument(sComponent, true));
                    nComponent++;
                }

                // Jump over the already expanded components
                i += nComponent;
            }
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function removes all
    /// internal string vector variables.
    ///
    /// \return void
    ///
    /// This member function is called automatically
    /// from the parser.
    /////////////////////////////////////////////////
    void StringVarFactory::removeStringVectorVars()
    {
        if (m_mStringVectorVars.size())
            m_mStringVectorVars.clear();
    }


    /////////////////////////////////////////////////
    /// \brief This member function is used to create
    /// a temporary string vector variable.
    ///
    /// \param vStringVector const vector<string>&
    /// \return string
    ///
    /// Temporary string vector variables differ in
    /// their name and not cleared by default during
    /// an evaluation done in the parser core.
    /////////////////////////////////////////////////
    string StringVarFactory::createTempStringVectorVar(const vector<string>& vStringVector)
    {
        // Return, if empty (something else went wrong)
        if (!vStringVector.size())
            return "";

        // Does it already exist?
        string strVectName = findVectorInMap(m_mTempStringVectorVars, vStringVector);

        if (strVectName.length())
            return strVectName;

        strVectName = "_~TEMPSTRVECT[" + toString((int)m_mTempStringVectorVars.size()) + "]";

        // save the vector
        m_mTempStringVectorVars[strVectName] = vStringVector;

        return strVectName;
    }


    /////////////////////////////////////////////////
    /// \brief This member function removes all
    /// temporary string vector variables.
    ///
    /// \return void
    ///
    /// This member function is not called
    /// automatically by the parser. It has to be used
    /// from the outside.
    /////////////////////////////////////////////////
    void StringVarFactory::removeTempStringVectorVars()
    {
        if (m_mTempStringVectorVars.size())
            m_mTempStringVectorVars.clear();
    }


    /////////////////////////////////////////////////
    /// \brief This private member function examines
    /// the first and the last character of the passed
    /// string and determines, whether it is a
    /// delimiter or not.
    ///
    /// \param sToken const string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StringVarFactory::checkStringvarDelimiter(const string& sToken) const
    {
        static const string sDELIMITER = "+-*/ ()={}^&|!<>,\\%#[]?:\";";

        // The opening parenthesis at the end indicates a function.
        // This is obviously not a string variable
        if (sToken.back() == '(')
            return false;

        // If the first and the last character are part of the
        // delimiter list (or the last character is a dot), then
        // we indicate the current token as delimited.
        return sDELIMITER.find(sToken.front()) != string::npos && (sDELIMITER.find(sToken.back()) != string::npos || sToken.back() == '.');
    }


    /////////////////////////////////////////////////
    /// \brief Replaces all found vectors of the
    /// passed map with their nCurrentComponent
    /// component.
    ///
    /// \param mVectorVarMap map<string,vector<string> >&
    /// \param currentline string&
    /// \param nCurrentComponent size_t
    /// \param bHasComponents bool&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void StringVarFactory::replaceStringVectorVars(map<string,vector<string> >& mVectorVarMap, string& currentline, size_t nCurrentComponent, bool& bHasComponents)
    {
        // Replace all found vectors with their nCurrentComponent-th component
        for (auto iter = mVectorVarMap.begin(); iter != mVectorVarMap.end(); ++iter)
        {
            size_t nMatch = 0;

            // Vector found: replace its occurence with its nCurrentComponent-th value
            while ((nMatch = currentline.find(iter->first)) != string::npos)
            {
                // Handle the size of the current vector correspondingly
                if ((iter->second).size() > nCurrentComponent)
                {
                    bHasComponents = true;

                    if (isNumericCandidate((iter->second)[nCurrentComponent]))
                        currentline.replace(nMatch, (iter->first).length(), "(" + (iter->second)[nCurrentComponent] + ")");
                    else
                        currentline.replace(nMatch, (iter->first).length(), (iter->second)[nCurrentComponent]);
                }
                else if ((iter->second).size() == 1)
                {
                    if (isNumericCandidate((iter->second)[0]))
                        currentline.replace(nMatch, (iter->first).length(), "(" + (iter->second)[0] + ")");
                    else
                        currentline.replace(nMatch, (iter->first).length(), (iter->second)[0]);
                }
                else
                    currentline.replace(nMatch, (iter->first).length(), "\"\"");
            }
        }
    }


    /////////////////////////////////////////////////
    /// \brief Searches for the passed string in the
    /// passed map and returns the key string from the
    /// map, if anything found, otherwise an empty
    /// string.
    ///
    /// \param mVectorVarMap map<string,vector<string> >&
    /// \param vStringVector const vector<string>&
    /// \return string
    ///
    /////////////////////////////////////////////////
    string StringVarFactory::findVectorInMap(const map<string,vector<string> >& mVectorVarMap, const vector<string>& vStringVector)
    {
        // Go through the map
        for (auto iter = mVectorVarMap.begin(); iter != mVectorVarMap.end(); ++iter)
        {
            // Compare first size and the first component
            if (iter->second.size() == vStringVector.size() && iter->second[0] == vStringVector[0])
            {
                // If both appear similar, compare every
                // component
                for (size_t i = 1; i < vStringVector.size(); i++)
                {
                    // Break if a component is not equal
                    if (vStringVector[i] != iter->second[i])
                        break;

                    // Return the vector name, if all components
                    // appear equal
                    if (i == vStringVector.size()-1)
                        return iter->first;
                }
            }
        }

        // Return an empty string, if nothing was found
        return "";
    }


    /////////////////////////////////////////////////
    /// \brief This public member function determines,
    /// whether the passed string line contains string
    /// variables as part of the expression.
    ///
    /// \param _sLine const string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StringVarFactory::containsStringVars(const string& _sLine) const
    {
        // Do nothing, if no string variables were declared
        if (!m_mStringVars.size())
            return false;

        // Add whitespaces for safety
        string sLine = " " + _sLine + " ";

        // Search for the first match of all declared string variables
        for (auto iter = m_mStringVars.begin(); iter != m_mStringVars.end(); ++iter)
        {
            size_t pos = 0;

            // Examine all possible candidates for string variables,
            // because the first one might be a false-positive
            while ((pos = sLine.find(iter->first, pos)) != std::string::npos)
            {
                // Compare the located match to the delimiters and return
                // true, if the match is delimited on both sides
                if (sLine[pos+(iter->first).length()] != '('
                    && checkStringvarDelimiter(sLine.substr(pos-1, (iter->first).length()+2))
                    )
                    return true;

                pos++;
            }
        }

        // No match found
        return false;
    }


    /////////////////////////////////////////////////
    /// \brief This public member function resolves
    /// all string variable occurences and replaces
    /// them with their value or the standard string
    /// function signature, if the string variable
    /// is connected to a method.
    ///
    /// \param sLine string&
    /// \param nPos unsigned int
    /// \return void
    ///
    /////////////////////////////////////////////////
    void StringVarFactory::getStringValues(string& sLine, unsigned int nPos)
    {
        // Do nothing, if no string variables were declared
        if (!m_mStringVars.size())
            return;

        unsigned int __nPos;
        sLine += " ";

        // Try to find every string variable into the passed string and
        // replace it correspondingly
        for (auto iter = m_mStringVars.begin(); iter != m_mStringVars.end(); ++iter)
        {
            __nPos = nPos;

            // Examine all occurences of the current variable in the
            // string
            while (sLine.find(iter->first, __nPos) != string::npos)
            {
                __nPos = sLine.find(iter->first, __nPos)+1;

                // Appended opening parenthesis indicates a function
                if (sLine[__nPos+(iter->first).length()-1] == '(')
                    continue;

                // Check, whether the found occurence is correctly
                // delimited and replace it. If the match is at the
                // beginning of the command line, it serves a special
                // treatment
                if (__nPos == 1)
                {
                    // Check with delimiter
                    if (checkStringvarDelimiter(" " + sLine.substr(0, (iter->first).length()+1)) && !isInQuotes(sLine, 0, true))
                    {
                        // Replace it with standard function signature or its value
                        if (sLine[(iter->first).length()] == '.')
                            replaceStringMethod(sLine, 0, (iter->first).length(), "\"" + iter->second + "\"");
                        else
                            sLine.replace(0, (iter->first).length(), "\"" + iter->second + "\"");
                    }

                    continue;
                }

                // Check with delimiter
                if (checkStringvarDelimiter(sLine.substr(__nPos-2, (iter->first).length()+2)) && !isInQuotes(sLine, __nPos-1, true))
                {
                    // Replace it with standard function signature or its value
                    if (sLine[__nPos+(iter->first).length()-1] == '.')
                        replaceStringMethod(sLine, __nPos-1, (iter->first).length(), "\"" + iter->second + "\"");
                    else
                        sLine.replace(__nPos-1, (iter->first).length(), "\"" + iter->second + "\"");
                }
            }
        }

        return;
    }


    /////////////////////////////////////////////////
    /// \brief This public member function creates or
    /// updates a string variable and fills it with
    /// the passed value.
    ///
    /// \param sVar const string&
    /// \param sValue const string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void StringVarFactory::setStringValue(const string& sVar, const string& sValue)
    {
        static const string sVALIDCHARACTERS = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz_1234567890~";

        // Variable names cannot start with a number
        if (sVar[0] >= '0' && sVar[0] <= '9')
            throw SyntaxError(SyntaxError::STRINGVARS_MUSTNT_BEGIN_WITH_A_NUMBER, "", SyntaxError::invalid_position, sVar);

        // Compare every character to the list of valid characters,
        // to ensure that the name is absolutly valid
        for (unsigned int i = 0; i < sVar.length(); i++)
        {
            if (sVALIDCHARACTERS.find(sVar[i]) == string::npos)
                throw SyntaxError(SyntaxError::STRINGVARS_MUSTNT_CONTAIN, "", SyntaxError::invalid_position, sVar.substr(i,1));
        }

        // Create or update the variable name with the passed value.
        // Omit the surrounding quotation marks
        if (sValue[0] == '"' && sValue[sValue.length()-1] == '"')
            m_mStringVars[sVar] = sValue.substr(1,sValue.length()-2);
        else
            m_mStringVars[sVar] = sValue;

        // If the current string variables value contains further
        // quotation marks, mask them correctly
        if (m_mStringVars[sVar].find('"') != string::npos)
        {
            unsigned int nPos = 0;

            while (m_mStringVars[sVar].find('"', nPos) != string::npos)
            {
                nPos = m_mStringVars[sVar].find('"', nPos);

                // Is the found quotation mark masked by a backslash?
                if (m_mStringVars[sVar][nPos-1] == '\\')
                {
                    nPos++;
                    continue;
                }
                else
                {
                    m_mStringVars[sVar].insert(nPos,1,'\\');
                    nPos += 2;
                    continue;
                }
            }
        }

        return;
    }


    /////////////////////////////////////////////////
    /// \brief This public member function removes the
    /// selected string variable from memory.
    ///
    /// \param sVar const string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void StringVarFactory::removeStringVar(const string& sVar)
    {
        // Do nothing, if no string variables were declared
        if (!m_mStringVars.size())
            return;

        // Delete the string variable, if it exists
        auto iter = m_mStringVars.find(sVar);
        if (iter != m_mStringVars.end())
            m_mStringVars.erase(iter);
    }

}

