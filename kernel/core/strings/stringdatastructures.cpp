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

#include "stringdatastructures.hpp"
#include "../../kernel.hpp"


/////////////////////////////////////////////////
/// \brief Static member function to convert the
/// contained string into a pur C++ string.
///
/// \param sStr StringView
/// \return StringView
///
/////////////////////////////////////////////////
StringView StringVector::makePureString(StringView sStr)
{
    if (sStr.length() && sStr.front() == '"')
        return sStr.subview(1, sStr.length()-2);

    return sStr;
}


/////////////////////////////////////////////////
/// \brief Static member function to convert the
/// string literal to an internal string for this
/// class.
///
/// \param sStr const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string StringVector::makeLocalString(const std::string& sStr)
{
    if (sStr.front() == '"')
        return "\"" + toInternalString(sStr) + "\"";

    return sStr;
}


/////////////////////////////////////////////////
/// \brief Get either the i-th element or the 1st
/// element, if this vector has only one
/// component (and is a singleton).
///
/// \param i size_t
/// \return StringView
///
/////////////////////////////////////////////////
StringView StringVector::getVectorized(size_t i) const
{
    if (size() == 1)
        return makePureString(front());
    else if (i < size())
        return makePureString(at(i));

    return StringView(m_sDUMMY);
}


/////////////////////////////////////////////////
/// \brief Get either the i-th element or the 1st
/// element converted as numerical value, if this
/// vector has only one component (and is a
/// singleton).
///
/// \param i size_t
/// \return mu::value_type
///
/////////////////////////////////////////////////
mu::value_type StringVector::getNumericalVectorized(size_t i) const
{
    if (i >= size() && size() > 1)
        return NAN;

    if (is_string(i))
        return NAN;

    if (getVectorized(i) == "true")
        return 1.0;

    if (getVectorized(i) == "false")
        return 0.0;

    // Use the numerical parser here
    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();
    _parser.SetExpr(getVectorized(i));
    return _parser.Eval();
}


/////////////////////////////////////////////////
/// \brief Get either the i-th element or the 1st
/// element converted as boolean, if this vector
/// has only one component (and is a singleton).
///
/// \param i size_t
/// \return bool
///
/////////////////////////////////////////////////
bool StringVector::getBooleanVectorized(size_t i) const
{
    if (i >= size() && size() > 1)
        return false;

    if (is_string(i))
        return getVectorized(i).length() > 0;

    if (getVectorized(i) == "true")
        return true;

    if (getVectorized(i) == "false")
        return false;

    // Use the numerical parser here
    mu::Parser& _parser = NumeReKernel::getInstance()->getParser();
    _parser.SetExpr(getVectorized(i));
    return _parser.Eval() != 0.0;
}


/////////////////////////////////////////////////
/// \brief Assign another StringVector instance.
///
/// \param sVect const StringVector&
/// \return void
///
/////////////////////////////////////////////////
void StringVector::assign(const StringVector& sVect)
{
    std::vector<std::string>::assign(sVect.begin(), sVect.end());
}


/////////////////////////////////////////////////
/// \brief Assign an instance of a boolean vector.
///
/// \param vect const std::vector<bool>&
/// \return void
///
/////////////////////////////////////////////////
void StringVector::assign(const std::vector<bool>& vect)
{
    resize(vect.size());

    for (size_t i = 0; i < vect.size(); i++)
    {
        std::vector<std::string>::operator[](i) = vect[i] ? "true" : "false";
    }
}


/////////////////////////////////////////////////
/// \brief Default constructor.
/////////////////////////////////////////////////
StringVector::StringVector() : std::vector<std::string>()
{ }


/////////////////////////////////////////////////
/// \brief Constructor from a
/// std::vector<std::string>. It is assumed that
/// the vector is already prepared for the local
/// string definition.
///
/// \param vect const std::vector<std::string>&
///
/////////////////////////////////////////////////
StringVector::StringVector(const std::vector<std::string>& vect) : std::vector<std::string>(vect)
{ }


/////////////////////////////////////////////////
/// \brief Create a StringVector instance from a
/// string.
///
/// \param sStr const std::string&
///
/////////////////////////////////////////////////
StringVector::StringVector(const std::string& sStr) : std::vector<std::string>(1, sStr)
{ }


/////////////////////////////////////////////////
/// \brief Create a StringVector instance from a
/// string.
///
/// \param sStr const char*
///
/////////////////////////////////////////////////
StringVector::StringVector(const char* sStr) : std::vector<std::string>(1, std::string(sStr))
{ }


/////////////////////////////////////////////////
/// \brief Create a StringVector instance from a
/// boolean.
///
/// \param sStr const std::string&
///
/////////////////////////////////////////////////
StringVector::StringVector(bool val) : std::vector<std::string>(1, toString(val))
{ }


/////////////////////////////////////////////////
/// \brief Copy a StringVector instance.
///
/// \param vect const StringVector&
///
/////////////////////////////////////////////////
StringVector::StringVector(const StringVector& vect) : std::vector<std::string>()
{
    assign(vect);
}


/////////////////////////////////////////////////
/// \brief Create a StringVector instance from a
/// std::vector<bool>.
///
/// \param vect const std::vector<bool>&
///
/////////////////////////////////////////////////
StringVector::StringVector(const std::vector<bool>& vect) : std::vector<std::string>()
{
    assign(vect);
}


/////////////////////////////////////////////////
/// \brief Create a StringVector with n elements.
///
/// \param n size_t
/// \param sStr const std::string&
///
/////////////////////////////////////////////////
StringVector::StringVector(size_t n, const std::string& sStr) : std::vector<std::string>(n, sStr)
{ }


/////////////////////////////////////////////////
/// \brief Static member function to create a
/// StringVector with one component with zero
/// length.
///
/// \return StringVector
///
/////////////////////////////////////////////////
StringVector StringVector::empty_string()
{
    return convert_internal("");
}


/////////////////////////////////////////////////
/// \brief Static member function to create a
/// StringVector instance from an internal string.
///
/// \param sInternal const std::string&
/// \return StringVector
///
/////////////////////////////////////////////////
StringVector StringVector::convert_internal(const std::string& sInternal)
{
    return StringVector("\"" + sInternal + "\"");
}


/////////////////////////////////////////////////
/// \brief Static member function to create a
/// StringVector instance from a string literal.
///
/// \param sLiteral const std::string&
/// \return StringVector
///
/////////////////////////////////////////////////
StringVector StringVector::convert_literal(const std::string& sLiteral)
{
    return StringVector(makeLocalString(sLiteral));
}


/////////////////////////////////////////////////
/// \brief Assignment operator overload for a
/// StringVector instance.
///
/// \param sVect const StringVector&
/// \return StringVector&
///
/////////////////////////////////////////////////
StringVector& StringVector::operator=(const StringVector& sVect)
{
    assign(sVect);
    return *this;
}


/////////////////////////////////////////////////
/// \brief Assignment operator overload for a
/// std::vector<bool> instance.
///
/// \param vect const std::vector<bool>&
/// \return StringVector&
///
/////////////////////////////////////////////////
StringVector& StringVector::operator=(const std::vector<bool>& vect)
{
    assign(vect);
    return *this;
}


/////////////////////////////////////////////////
/// \brief Append a string to the end of this
/// vector. Will be stored as local string.
///
/// \param sStr const std::string&
/// \return void
///
/////////////////////////////////////////////////
void StringVector::push_back(const std::string& sStr)
{
    std::vector<std::string>::push_back("\"" + sStr + "\"");
}


/////////////////////////////////////////////////
/// \brief Append a string to the end of this
/// vector. Will be stored as local string.
///
/// \param sStr const char*
/// \return void
///
/////////////////////////////////////////////////
void StringVector::push_back(const char* sStr)
{
    std::vector<std::string>::push_back("\"" + std::string(sStr) + "\"");
}


/////////////////////////////////////////////////
/// \brief Append a mu::value_type to the end of
/// this vector.
///
/// \param vVal const mu::value_type&
/// \return void
///
/////////////////////////////////////////////////
void StringVector::push_back(const mu::value_type& vVal)
{
    std::vector<std::string>::push_back(toString(vVal, 20));
}


/////////////////////////////////////////////////
/// \brief Append a size_t to the end of
/// this vector.
///
/// \param nVal size_t
/// \return void
///
/////////////////////////////////////////////////
void StringVector::push_back(size_t nVal)
{
    std::vector<std::string>::push_back(toString(nVal));
}


/////////////////////////////////////////////////
/// \brief Append an int to the end of
/// this vector.
///
/// \param nVal int
/// \return void
///
/////////////////////////////////////////////////
void StringVector::push_back(int nVal)
{
    std::vector<std::string>::push_back(toString(nVal));
}


/////////////////////////////////////////////////
/// \brief Append a long long int to the end of
/// this vector.
///
/// \param nVal long long int
/// \return void
///
/////////////////////////////////////////////////
void StringVector::push_back(long long int nVal)
{
    std::vector<std::string>::push_back(toString(nVal));
}


/////////////////////////////////////////////////
/// \brief Append a boolean to the end of
/// this vector.
///
/// \param nVal bool
/// \return void
///
/////////////////////////////////////////////////
void StringVector::push_back(bool nVal)
{
    std::vector<std::string>::push_back(toString(nVal));
}


/////////////////////////////////////////////////
/// \brief Append a generic string value to the
/// end of this vector. Depending on the
/// existence of surrounding quotation marks, it
/// will be considered as local string or as a
/// generic value.
///
/// \param sStr const std::string&
/// \return void
///
/////////////////////////////////////////////////
void StringVector::push_generic(const std::string& sStr)
{
    std::vector<std::string>::push_back(makeLocalString(sStr));
}


/////////////////////////////////////////////////
/// \brief Check whether the i-th element
/// represents a string or a numeric value.
///
/// \param i size_t
/// \return bool
///
/////////////////////////////////////////////////
bool StringVector::is_string(size_t i) const
{
    if (i < size())
        return at(i).front() == '"';
    else if (size() == 1)
        return front().front() == '"';

    return false;
}


/////////////////////////////////////////////////
/// \brief Convert the i-th element to a string.
///
/// \param i size_t
/// \param minChars size_t Only used for
/// non-string values
/// \return void
///
/////////////////////////////////////////////////
void StringVector::convert_to_string(size_t i, size_t minChars)
{
    if (i < size() && !is_string(i))
    {
        size_t len = std::vector<std::string>::operator[](i).length();
        std::vector<std::string>::operator[](i) = "\"" + (len < minChars ? std::string(minChars-len, '0') : "") + std::vector<std::string>::operator[](i) + "\"";
    }
}


/////////////////////////////////////////////////
/// \brief Return a view to the i-th element.
///
/// \param i size_t
/// \return StringView
///
/////////////////////////////////////////////////
StringView StringVector::operator[](size_t i) const
{
    if (i < size())
        return makePureString(at(i));

    return StringView(m_sDUMMY);
}


/////////////////////////////////////////////////
/// \brief Create a StringArg instance from the
/// i-th or the 1st element, if this vector is a
/// singleton.
///
/// \param i size_t
/// \return StringArg
///
/////////////////////////////////////////////////
StringArg StringVector::getArg(size_t i) const
{
    if (i < size())
        return StringArg(at(i));
    else if (size() == 1)
        return StringArg(at(0));

    return StringArg();
}


/////////////////////////////////////////////////
/// \brief Return the contained string with the
/// internal quotation marks as escaped variants.
///
/// \param i size_t
/// \return std::string
///
/////////////////////////////////////////////////
std::string StringVector::getMasked(size_t i) const
{
    if (i < size())// && is_string(i))
    {
        const char NEWSTRING = (char)23;
        std::string sRet = at(i);

        // Go through the complete string without the first
        // and the last character
        for (size_t i = 1; i < sRet.length() - 1; i++)
        {
            // Escape backslashes
            if (sRet[i] == '\\' && sRet[i + 1] != '"' && sRet[i + 1] != ' ')
            {
                sRet.insert(i + 1, " ");
                i++;
            }

            // Escape quotation marks
            if (sRet[i] == '"' && sRet[i - 1] != '\\' && sRet[i + 1] != NEWSTRING && sRet.find('"', i + 1) != std::string::npos)
            {
                sRet.insert(i, "\\");
                i++;
            }

            // Replace the new string character with a comma
            if (sRet[i] == NEWSTRING)
            {
                sRet[i] = ',';
                if (sRet[i + 1] == '"')
                    i++;
            }
        }

        return sRet;
    }
    //else if (i < size())
    //    return at(i);

    return m_sDUMMY;
}


/////////////////////////////////////////////////
/// \brief Return a reference to the i-th element
/// in this string.
///
/// \param i size_t
/// \return std::string&
///
/////////////////////////////////////////////////
std::string& StringVector::getRef(size_t i)
{
    if (i < size())
        return std::vector<std::string>::operator[](i);

    throw std::out_of_range("Requested element " + toString(i) + ", which is greater than this->size().");
}


/////////////////////////////////////////////////
/// \brief Equality operator overload.
///
/// \param sVect const StringVector&
/// \return std::vector<bool>
///
/////////////////////////////////////////////////
std::vector<bool> StringVector::operator==(const StringVector& sVect) const
{
    std::vector<bool> vRet(std::max(size(), sVect.size()));

    for (size_t i = 0; i < vRet.size(); i++)
    {
        if (is_string(i) != sVect.is_string(i))
            vRet[i] = false;
        else if (is_string(i))
            vRet[i] = getVectorized(i) == sVect.getVectorized(i);
        else
            vRet[i] = getNumericalVectorized(i) == sVect.getNumericalVectorized(i);
    }

    return vRet;
}


/////////////////////////////////////////////////
/// \brief Inequality operator overload.
///
/// \param sVect const StringVector&
/// \return std::vector<bool>
///
/////////////////////////////////////////////////
std::vector<bool> StringVector::operator!=(const StringVector& sVect) const
{
    std::vector<bool> vRet(std::max(size(), sVect.size()));

    for (size_t i = 0; i < vRet.size(); i++)
    {
        if (is_string(i) != sVect.is_string(i))
            vRet[i] = true;
        else if (is_string(i))
            vRet[i] = getVectorized(i) != sVect.getVectorized(i);
        else
            vRet[i] = getNumericalVectorized(i) != sVect.getNumericalVectorized(i);
    }

    return vRet;
}


/////////////////////////////////////////////////
/// \brief Less-than operator overload.
///
/// \param sVect const StringVector&
/// \return std::vector<bool>
///
/////////////////////////////////////////////////
std::vector<bool> StringVector::operator<(const StringVector& sVect) const
{
    std::vector<bool> vRet(std::max(size(), sVect.size()));

    for (size_t i = 0; i < vRet.size(); i++)
    {
        if (is_string(i) != sVect.is_string(i))
            vRet[i] = false;
        else if (is_string(i))
            vRet[i] = getVectorized(i) < sVect.getVectorized(i);
        else
            vRet[i] = getNumericalVectorized(i).real() < sVect.getNumericalVectorized(i).real();
    }

    return vRet;
}


/////////////////////////////////////////////////
/// \brief Lesser-equal operator overload.
///
/// \param sVect const StringVector&
/// \return std::vector<bool>
///
/////////////////////////////////////////////////
std::vector<bool> StringVector::operator<=(const StringVector& sVect) const
{
    std::vector<bool> vRet(std::max(size(), sVect.size()));

    for (size_t i = 0; i < vRet.size(); i++)
    {
        if (is_string(i) != sVect.is_string(i))
            vRet[i] = false;
        else if (is_string(i))
            vRet[i] = getVectorized(i) <= sVect.getVectorized(i);
        else
            vRet[i] = getNumericalVectorized(i).real() <= sVect.getNumericalVectorized(i).real();
    }

    return vRet;
}


/////////////////////////////////////////////////
/// \brief Greater-than operator overload.
///
/// \param sVect const StringVector&
/// \return std::vector<bool>
///
/////////////////////////////////////////////////
std::vector<bool> StringVector::operator>(const StringVector& sVect) const
{
    std::vector<bool> vRet(std::max(size(), sVect.size()));

    for (size_t i = 0; i < vRet.size(); i++)
    {
        if (is_string(i) != sVect.is_string(i))
            vRet[i] = false;
        else if (is_string(i))
            vRet[i] = getVectorized(i) > sVect.getVectorized(i);
        else
            vRet[i] = getNumericalVectorized(i).real() > sVect.getNumericalVectorized(i).real();
    }

    return vRet;
}


/////////////////////////////////////////////////
/// \brief Greater-equal operator overload.
///
/// \param sVect const StringVector&
/// \return std::vector<bool>
///
/////////////////////////////////////////////////
std::vector<bool> StringVector::operator>=(const StringVector& sVect) const
{
    std::vector<bool> vRet(std::max(size(), sVect.size()));

    for (size_t i = 0; i < vRet.size(); i++)
    {
        if (is_string(i) != sVect.is_string(i))
            vRet[i] = false;
        else if (is_string(i))
            vRet[i] = getVectorized(i) >= sVect.getVectorized(i);
        else
            vRet[i] = getNumericalVectorized(i).real() >= sVect.getNumericalVectorized(i).real();
    }

    return vRet;
}


/////////////////////////////////////////////////
/// \brief This member function represents an AND
/// operator.
///
/// \param sVect const StringVector&
/// \return std::vector<bool>
///
/////////////////////////////////////////////////
std::vector<bool> StringVector::and_f(const StringVector& sVect) const
{
    std::vector<bool> vRet(std::max(size(), sVect.size()));

    for (size_t i = 0; i < vRet.size(); i++)
    {
        vRet[i] = getBooleanVectorized(i) && sVect.getBooleanVectorized(i);
    }

    return vRet;
}


/////////////////////////////////////////////////
/// \brief This member function represents an OR
/// operator.
///
/// \param sVect const StringVector&
/// \return std::vector<bool>
///
/////////////////////////////////////////////////
std::vector<bool> StringVector::or_f(const StringVector& sVect) const
{
    std::vector<bool> vRet(std::max(size(), sVect.size()));

    for (size_t i = 0; i < vRet.size(); i++)
    {
        vRet[i] = getBooleanVectorized(i) || sVect.getBooleanVectorized(i);
    }

    return vRet;
}


/////////////////////////////////////////////////
/// \brief This member function represents an XOR
/// operator.
///
/// \param sVect const StringVector&
/// \return std::vector<bool>
///
/////////////////////////////////////////////////
std::vector<bool> StringVector::xor_f(const StringVector& sVect) const
{
    std::vector<bool> vRet(std::max(size(), sVect.size()));

    for (size_t i = 0; i < vRet.size(); i++)
    {
        vRet[i] = getBooleanVectorized(i) xor sVect.getBooleanVectorized(i);
    }

    return vRet;
}


/////////////////////////////////////////////////
/// \brief Concatenate two StringVectors.
///
/// \param sVect const StringVector&
/// \return StringVector
///
/////////////////////////////////////////////////
StringVector StringVector::operator+(const StringVector& sVect) const
{
    StringVector vRet(std::max(size(), sVect.size()));

    for (size_t i = 0; i < vRet.size(); i++)
    {
        if (is_string(i) && sVect.is_string(i))
            vRet.getRef(i) = "\"" + (getVectorized(i) + sVect.getVectorized(i)) + "\"";
        else
            vRet.getRef(i) = getVectorized(i) + std::string("+") + sVect.getVectorized(i).to_string();
    }

    return vRet;
}


/////////////////////////////////////////////////
/// \brief Append a StringVector to this instance.
///
/// \param sVect const StringVector&
/// \return StringVector&
///
/////////////////////////////////////////////////
StringVector& StringVector::operator+=(const StringVector& sVect)
{
    assign(operator+(sVect));
    return *this;
}


/////////////////////////////////////////////////
/// \brief Concatenate a StringVector and a
/// string literal.
///
/// \param sLiteral const std::string&
/// \return StringVector
///
/////////////////////////////////////////////////
StringVector StringVector::operator+(const std::string& sLiteral) const
{
    return operator+(StringVector(sLiteral));
}


/////////////////////////////////////////////////
/// \brief Append a string literal to this
/// StringVector instance.
///
/// \param sLiteral const std::string&
/// \return StringVector&
///
/////////////////////////////////////////////////
StringVector& StringVector::operator+=(const std::string& sLiteral)
{
    assign(operator+(StringVector(sLiteral)));
    return *this;
}


/////////////////////////////////////////////////
/// \brief Assign the results of an if-else
/// construct to this StringVector instance.
///
/// \param sLogicals const StringVector&
/// \param sIfBranch const StringVector&
/// \param sElseBranch const StringVector&
/// \return StringVector&
///
/////////////////////////////////////////////////
StringVector& StringVector::evalIfElse(const StringVector& sLogicals, const StringVector& sIfBranch, const StringVector& sElseBranch)
{
    // Only one logical result?
    if (sLogicals.size() == 1)
    {
        if (sLogicals.getBooleanVectorized(0))
            assign(sIfBranch);
        else
            assign(sElseBranch);

        return *this;
    }

    StringVector vRet(std::max({sLogicals.size(), sIfBranch.size(), sElseBranch.size()}));

    for (size_t i = 0; i < vRet.size(); i++)
    {
        if (sLogicals.getBooleanVectorized(i))
            vRet.getRef(i) = sIfBranch.getArg(i).getRef();
        else
            vRet.getRef(i) = sElseBranch.getArg(i).getRef();
    }

    assign(vRet);
    return *this;
}

