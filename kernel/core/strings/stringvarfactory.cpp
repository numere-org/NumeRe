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

using namespace std;

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
        {
            if ((sComponent.front() == '(' || sComponent.front() == '{') && getMatchingParenthesis(sComponent) == sComponent.length()-1)
                return false;

            return true;
        }

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

        string strVectName = "_~STRVECT[" + toString(m_mStringVectorVars.size()) + "]";

        // Does it already exist?
        if (m_mStringVectorVars.find(strVectName) != m_mStringVectorVars.end())
            throw SyntaxError(SyntaxError::STRING_ERROR, strVectName, SyntaxError::invalid_position, _lang.get("ERR_NR_3603_OVERWRITE"));

        if (vStringVector.size() == 1 && vStringVector.front().find(strVectName) != std::string::npos)
            throw SyntaxError(SyntaxError::STRING_ERROR, strVectName, SyntaxError::invalid_position, _lang.get("ERR_NR_3603_INCOMPLETE"));

        // save the vector
        m_mStringVectorVars[strVectName] = vStringVector;

        return strVectName;
    }


    /////////////////////////////////////////////////
    /// \brief Check, whether the passed string
    /// identifies a string vector variable.
    ///
    /// \param sVarName const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StringVarFactory::isStringVectorVar(const std::string& sVarName) const
    {
        return m_mStringVectorVars.find(sVarName) != m_mStringVectorVars.end()
            || m_mTempStringVectorVars.find(sVarName) != m_mTempStringVectorVars.end();
    }


    /////////////////////////////////////////////////
    /// \brief Return a reference to the identified
    /// string vector variable or throw if it does
    /// not exist.
    ///
    /// \param sVarName const std::string&
    /// \return const StringVector&
    ///
    /////////////////////////////////////////////////
    const StringVector& StringVarFactory::getStringVectorVar(const std::string& sVarName) const
    {
        auto iter = m_mStringVectorVars.find(sVarName);

        if (iter != m_mStringVectorVars.end())
            return iter->second;

        iter = m_mTempStringVectorVars.find(sVarName);

        if (iter != m_mTempStringVectorVars.end())
            return iter->second;

        throw std::out_of_range("Could not find " + sVarName + " in the string vectors.");
    }


    /////////////////////////////////////////////////
    /// \brief This member function determines, whether
    /// there are string vector variables in the passed
    /// command line
    ///
    /// \param sLine StringView
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StringVarFactory::containsStringVectorVars(StringView sLine)
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
    /// \return StringVector
    ///
    /// For each contained vector, the complete command
    /// line is evaluated and returned as a separate
    /// vector component.
    /////////////////////////////////////////////////
    StringVector StringVarFactory::evaluateStringVectors(string sLine)
    {
        StringVector vRes;
        const map<string, vector<mu::value_type> >& mNumVectorVars = NumeReKernel::getInstance()->getParser().GetVectors();
        //g_logger.info("Evaluating string vector for '" + sLine + "'");
        //g_logger.info("Contains vars: " + toString(containsStringVectorVars(sLine)));
        //g_logger.info("Contains parser vars: " + toString(NumeReKernel::getInstance()->getParser().ContainsVectorVars(sLine, false)));

        // As long as the current vector is not empty
        while (sLine.length())
        {
            // Get-cut the next argument
            string sCurrentComponent = getNextArgument(sLine, true);

            // If the current component does not contain any further
            // vectors, push it back.
            // Otherwise expand the contained vectors
            //if (!containsStringVectorVars(sCurrentComponent))
            //    vRes.push_back(sCurrentComponent);
            //else
            {
                // Expand the contained vectors
                size_t nCurrentComponent = 0;

                // This while loop is terminated with a break command
                while (true)
                {
                    string currentline = sCurrentComponent;
                    bool bHasComponents = false;

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

                    // Replace all internal vectors
                    replaceStringVectorVars(m_mStringVectorVars, currentline, nCurrentComponent, bHasComponents);

                    // Replace all temporary vectors
                    replaceStringVectorVars(m_mTempStringVectorVars, currentline, nCurrentComponent, bHasComponents);

                    // Break the loop, if there are no further vector components
                    if (!bHasComponents)
                        break;

                    // Push back the current line and increment the component
                    vRes.push_generic(currentline);
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
    /// \param vStringVector std::vector<StringVector>&
    /// \return StringVector
    ///
    /// The components are evaluated and inserted into
    /// the original vector, replacing the original
    /// component with the calculated vector n components,
    /// which will enlarge the original vector by n-1
    /// new components.
    /////////////////////////////////////////////////
    StringVector StringVarFactory::expandStringVectorComponents(std::vector<StringVector>& vStringVector)
    {
        // If the vector contains only one element,
        // then return it directly
        if (vStringVector.size() == 1)
            return vStringVector.front();

        // Concatenate all embedded vectors to a single component
        StringVector vRet = vStringVector.front();

        for (size_t i = 1; i < vStringVector.size(); i++)
        {
            vRet.insert(vRet.end(), vStringVector[i].begin(), vStringVector[i].end());
        }

        // Return the concatenated result
        return vRet;
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

        strVectName = "_~TEMPSTRVECT[" + toString(m_mTempStringVectorVars.size()) + "]";

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
    /// \brief Replaces all found vectors of the
    /// passed map with their nCurrentComponent
    /// component.
    ///
    /// \param mVectorVarMap map<string,StringVector>&
    /// \param currentline string&
    /// \param nCurrentComponent size_t
    /// \param bHasComponents bool&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void StringVarFactory::replaceStringVectorVars(map<string,StringVector>& mVectorVarMap, string& currentline, size_t nCurrentComponent, bool& bHasComponents)
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

                    if (!iter->second.is_string(nCurrentComponent))
                        currentline.replace(nMatch, (iter->first).length(), "(" + (iter->second).getMasked(nCurrentComponent) + ")");
                    else
                        currentline.replace(nMatch, (iter->first).length(), (iter->second).getMasked(nCurrentComponent));
                }
                else if ((iter->second).size() == 1)
                {
                    if (!iter->second.is_string(0))
                        currentline.replace(nMatch, (iter->first).length(), "(" + (iter->second).getMasked(0) + ")");
                    else
                        currentline.replace(nMatch, (iter->first).length(), (iter->second).getMasked(0));
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
    /// \param mVectorVarMap map<string,StringVector>&
    /// \param vStringVector const vector<string>&
    /// \return string
    ///
    /////////////////////////////////////////////////
    string StringVarFactory::findVectorInMap(const map<string,StringVector>& mVectorVarMap, const vector<string>& vStringVector)
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
                    if (vStringVector[i] != iter->second.at(i))
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
    /// \param sLine StringView
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StringVarFactory::containsStringVars(StringView sLine) const
    {
        // Do nothing, if no string variables were declared
        if (!m_mStringVars.size())
            return false;

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
                if (sLine.is_delimited_sequence(pos, iter->first.length(), StringViewBase::STRVAR_DELIMITER))
                    return true;

                pos++;
            }
        }

        // No match found
        return false;
    }


    /////////////////////////////////////////////////
    /// \brief Determine, whether the passed string
    /// is the identifier of a string variable.
    ///
    /// \param sVarName const std::string&
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool StringVarFactory::isStringVar(const std::string& sVarName) const
    {
        return m_mStringVars.find(sVarName) != m_mStringVars.end();
    }


    /////////////////////////////////////////////////
    /// \brief This public member function resolves
    /// all string variable occurences and replaces
    /// them with an internal string vector variable
    /// or the standard string
    /// function signature, if the string variable
    /// is connected to a method.
    ///
    /// \param sLine std::string&
    /// \param nPos size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void StringVarFactory::getStringValuesAsInternalVar(std::string& sLine, size_t nPos)
    {
        // Do nothing, if no string variables were declared
        if (!m_mStringVars.size())
            return;

        size_t __nPos;
        sLine += " ";

        // Try to find every string variable into the passed string and
        // replace it correspondingly
        for (auto iter = m_mStringVars.begin(); iter != m_mStringVars.end(); ++iter)
        {
            __nPos = nPos;
            std::string sVectVar;

            // Examine all occurences of the current variable in the
            // string
            while ((__nPos = sLine.find(iter->first, __nPos)) != string::npos)
            {
                __nPos++;

                // Appended opening parenthesis indicates a function
                if (sLine[__nPos+(iter->first).length()-1] == '('
                    || sLine[__nPos+(iter->first).length()-1] == '{')
                    continue;

                // Only create the variable, if it is actually needed
                if (!sVectVar.length())
                    sVectVar = createStringVectorVar(std::vector<std::string>(1, "\""+iter->second+"\""));
                    //sVectVar = "\""+iter->second+"\"";

                // Check, whether the found occurence is correctly
                // delimited and replace it. If the match is at the
                // beginning of the command line, it serves a special
                // treatment
                if (__nPos == 1)
                {
                    // Check with delimiter
                    if (StringView(sLine).is_delimited_sequence(0, iter->first.length(), StringViewBase::STRVAR_DELIMITER)
                        && !isInQuotes(sLine, 0, true))
                    {
                        // Replace it with standard function signature or its value
                        if (sLine[(iter->first).length()] == '.')
                            replaceStringMethod(sLine, 0, (iter->first).length(), sVectVar);
                        else
                            sLine.replace(0, (iter->first).length(), sVectVar);
                    }

                    continue;
                }

                // Check with delimiter
                if (StringView(sLine).is_delimited_sequence(__nPos-1, iter->first.length(), StringViewBase::STRVAR_DELIMITER)
                    && !isInQuotes(sLine, __nPos-1, true))
                {
                    // Replace it with standard function signature or its value
                    if (sLine[__nPos+(iter->first).length()-1] == '.')
                        replaceStringMethod(sLine, __nPos-1, (iter->first).length(), sVectVar);
                    else
                        sLine.replace(__nPos-1, (iter->first).length(), sVectVar);
                }
            }
        }
    }


    /////////////////////////////////////////////////
    /// \brief This public member function resolves
    /// all string variable occurences and replaces
    /// them with their value or the standard string
    /// function signature, if the string variable
    /// is connected to a method.
    ///
    /// \param sLine std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void StringVarFactory::getStringValues(std::string& sLine)
    {
        // Do nothing, if no string variables were declared
        if (!m_mStringVars.size())
            return;

        size_t __nPos;
        sLine += " ";

        // Try to find every string variable into the passed string and
        // replace it correspondingly
        for (auto iter = m_mStringVars.begin(); iter != m_mStringVars.end(); ++iter)
        {
            __nPos = 0;
            std::string sVectVar;

            // Examine all occurences of the current variable in the
            // string
            while ((__nPos = sLine.find(iter->first, __nPos)) != string::npos)
            {
                __nPos++;

                // Appended opening parenthesis indicates a function
                if (sLine[__nPos+(iter->first).length()-1] == '('
                    || sLine[__nPos+(iter->first).length()-1] == '{')
                    continue;

                // Only create the variable, if it is actually needed
                if (!sVectVar.length())
                    sVectVar = "\""+iter->second+"\"";

                // Check, whether the found occurence is correctly
                // delimited and replace it. If the match is at the
                // beginning of the command line, it serves a special
                // treatment
                if (__nPos == 1)
                {
                    // Check with delimiter
                    if (StringView(sLine).is_delimited_sequence(0, iter->first.length(), StringViewBase::STRVAR_DELIMITER)
                        && !isInQuotes(sLine, 0, true))
                    {
                        // Replace it with standard function signature or its value
                        if (sLine[(iter->first).length()] == '.')
                            replaceStringMethod(sLine, 0, (iter->first).length(), sVectVar);
                        else
                            sLine.replace(0, (iter->first).length(), sVectVar);
                    }

                    continue;
                }

                // Check with delimiter
                if (StringView(sLine).is_delimited_sequence(__nPos-1, iter->first.length(), StringViewBase::STRVAR_DELIMITER)
                    && !isInQuotes(sLine, __nPos-1, true))
                {
                    // Replace it with standard function signature or its value
                    if (sLine[__nPos+(iter->first).length()-1] == '.')
                        replaceStringMethod(sLine, __nPos-1, (iter->first).length(), sVectVar);
                    else
                        sLine.replace(__nPos-1, (iter->first).length(), sVectVar);
                }
            }
        }
    }


    /////////////////////////////////////////////////
    /// \brief Returns the value of the selected
    /// string variable.
    ///
    /// \param sVar const std::string&
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string StringVarFactory::getStringValue(const std::string& sVar) const
    {
        auto iter = m_mStringVars.find(sVar);

        if (iter == m_mStringVars.end())
            return "";

        return iter->second;
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
        for (size_t i = 0; i < sVar.length(); i++)
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


    /////////////////////////////////////////////////
    /// \brief This public member function removes all
    /// string variables which don't start with ~,_
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void StringVarFactory::clearStringVar()
    {
	    if (!m_mStringVars.size())
            return;

	    for (auto iter = m_mStringVars.begin(); iter != m_mStringVars.end(); ++iter)
	    {
	        if (!iter->first.starts_with("_~"))
	            m_mStringVars.erase(iter);
	    }
    }

}
