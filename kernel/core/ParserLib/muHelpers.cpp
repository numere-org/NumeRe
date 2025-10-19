/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2024  Erik Haenel et al.

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

#include "muHelpers.hpp"
#ifdef PARSERSTANDALONE
#include <iostream>
#else
#include "../../kernel.hpp"
#endif


namespace mu
{
    /////////////////////////////////////////////////
    /// \brief Print a string on the terminal.
    ///
    /// \param msg const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void print(const std::string& msg)
    {
#ifdef PARSERSTANDALONE
        std::cout << "|-> " << msg << std::endl;
#else
        NumeReKernel::print(msg);
#endif
    }


    /////////////////////////////////////////////////
    /// \brief Print a formatted string on the
    /// terminal.
    ///
    /// \param msg const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void printFormatted(const std::string& msg)
    {
#ifdef PARSERSTANDALONE
        std::cout << msg;
#else
        NumeReKernel::printPreFmt(msg);
#endif
    }


    /////////////////////////////////////////////////
    /// \brief Print a string on the terminal.
    ///
    /// \param msg const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void warning(const std::string& msg)
    {
#ifdef PARSERSTANDALONE
        std::cout << "|-> " << msg << std::endl;
#else
        NumeReKernel::issueWarning(msg);
#endif
    }


    /////////////////////////////////////////////////
    /// \brief Toggle the table printing mode (i.e.
    /// delay/refresh after) to avoid flickering.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void toggleTableMode()
    {
#ifdef PARSERSTANDALONE
        //
#else
        NumeReKernel::toggleTableStatus();
#endif
    }


    /////////////////////////////////////////////////
    /// \brief Implements the val2str operator.
    ///
    /// \param arr const Array&
    /// \param nLen size_t
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array val2Str(const Array& arr, size_t nLen)
    {
        Array res;
        size_t elems = arr.size();
        res.copyDims(arr);

        for (size_t i = 0; i < elems; i++)
        {
#ifndef PARSERSTANDALONE
            std::string sStr = arr.get(i).printVal(NumeReKernel::getInstance()->getSettings().getPrecision());
#else
            std::string sStr = arr.get(i).printVal();
#endif // PARSERSTANDALONE

            if (sStr.length() < nLen && sStr.length() > 0)
                sStr.insert(0, std::string(nLen-sStr.length(), '0'));

            res.push_back(sStr);
        }

        return res;
    }


    /////////////////////////////////////////////////
    /// \brief Implements the path tokens within the
    /// parser.
    ///
    /// \param arr const Array&
    /// \return Array
    ///
    /////////////////////////////////////////////////
    Array getPathToken(const Array& arr)
    {
#ifndef PARSERSTANDALONE
        return mu::Value(NumeReKernel::getInstance()->getFileSystem().getTokenValue(arr.front().getStr()));
#else
        return arr;
#endif // PARSERSTANDALONE
    }


    /////////////////////////////////////////////////
    /// \brief Simple helper to create an HTML
    /// exponent from a string.
    ///
    /// \param sExp const std::string&
    /// \param negative bool
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    static std::string createHtmlExponent(const std::string& sExp, bool negative)
    {
        return "&times;10<sup>" + (negative ? "-"+sExp : sExp) + "</sup>";
    }


    /////////////////////////////////////////////////
    /// \brief This function converts a number
    /// into a tex string.
    ///
    /// \param number const Value&
    /// \return string
    ///
    /////////////////////////////////////////////////
    static std::string formatNumberToHtml(const Value& number)
    {
        std::string sNumber = number.printVal();

        size_t firstExp = 0;

        // Handle floating point numbers with
        // exponents correctly
        while ((firstExp = sNumber.find('e', firstExp) )!= std::string::npos)
        {
            // Find first exponent start and value
            size_t expBegin = sNumber.find_first_not_of("e0+-", firstExp);
            size_t expEnd = sNumber.find_first_not_of("0123456789", expBegin);

            if (expEnd == std::string::npos)
            {
                // Get the modified string where the first exponent is replaced by the tex string format
                sNumber.replace(firstExp, expEnd,
                                createHtmlExponent(sNumber.substr(expBegin), sNumber[firstExp+1] == '-'));
            }
            else
            {
                // Get the modified string where the first exponent is replaced by the tex string format
                sNumber.replace(firstExp, expEnd-firstExp,
                                createHtmlExponent(sNumber.substr(expBegin, expEnd-expBegin), sNumber[firstExp+1] == '-'));
            }

            // Jump over the "e" in &times;
            firstExp += 20;
        }

#ifndef PARSERSTANDALONE
        // Consider some special values
        replaceAll(sNumber, "inf", "&infin;");
#endif

        // Return the formatted string in math mode
        return sNumber;
    }


    /////////////////////////////////////////////////
    /// \brief Converts the value's contents into an
    /// HTML representation.
    ///
    /// \param val const Value&
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string to_html(const Value& val)
    {
#ifndef PARSERSTANDALONE
        if (!val.isValid())
            return "&mdash;";

        if (val.isString())
            return markupToHtml(val.printVal());

        if (val.isNumerical())
        {
            if (val.getNum().getType() == LOGICAL)
                return (bool)val ? "&#9745;" : "&#9744;";

            return formatNumberToHtml(val);
        }

        return val.printVal();
#else
        return val.print();
#endif // PARSERSTANDALONE
    }
}

