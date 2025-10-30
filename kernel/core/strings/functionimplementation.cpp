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
#include <wx/utils.h>

#include "functionimplementation.hpp"
#include "../utils/tools.hpp"
#include "../utils/filecheck.hpp"
#include "../../../common/compareFiles.hpp"
#include "../ParserLib/muHelpers.hpp"
#ifndef PARSERSTANDALONE
#include "../../../database/dbinternals.hpp"
#include "../../kernel.hpp"
#include "../../versioninformation.hpp"
#endif

#include <regex>
#include <sstream>
#include <libsha.hpp>


/////////////////////////////////////////////////
/// \brief Simple helper to create a LaTeX
/// exponent from a string.
///
/// \param sExp const std::string&
/// \param negative bool
/// \return std::string
///
/////////////////////////////////////////////////
static std::string createLaTeXExponent(const std::string& sExp, bool negative)
{
    return "\\cdot10^{" + (negative ? "-"+sExp : sExp) + "}";
}


/////////////////////////////////////////////////
/// \brief This function converts a number
/// into a tex string.
///
/// \param number const mu::Value&
/// \param precision size_t
/// \return string
///
/////////////////////////////////////////////////
static std::string formatNumberToTex(const mu::Value& number, size_t precision = 0)
{
#ifndef PARSERSTANDALONE
    // Use the default precision if precision is default value
    if (precision == 0)
        precision = NumeReKernel::getInstance()->getSettings().getPrecision();
#endif
    std::string sNumber = toString(number.getNum().asCF64(), precision);

    // Handle floating point numbers with
    // exponents correctly
    while (sNumber.find('e') != std::string::npos)
    {
        // Find first exponent start and value
        size_t firstExp = sNumber.find('e');
        size_t expBegin = sNumber.find_first_not_of("e0+-", firstExp);
        size_t expEnd = sNumber.find_first_not_of("0123456789", expBegin);

        // Get the modified string where the first exponent is replaced by the tex string format
        sNumber.replace(firstExp, expEnd-firstExp,
                        createLaTeXExponent(sNumber.substr(expBegin, expEnd-expBegin), sNumber[firstExp+1] == '-'));
    }

    // Consider some special values
    replaceAll(sNumber, "inf", "\\infty");
    replaceAll(sNumber, "nan", "---");

    // Return the formatted string in math mode
    return "$" + sNumber + "$";
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_tex()
/// function.
///
/// \param a1 const mu::Array&
/// \param a2 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_to_tex(const mu::Array& a1, const mu::Array& a2)
{
    mu::MatrixView a1View(a1);
    mu::MatrixView a2View(a2);
    mu::Array ret = a1View.prepare(a2View);

    size_t elems = a1View.size();

    if (!a2.isDefault())
    {
        for (size_t i = 0; i < elems; i++)
        {
            ret.emplace_back(formatNumberToTex(a1View.get(i), a2View.get(i).getNum().asI64()));
        }
    }
    else
    {
        for (size_t i = 0; i < elems; i++)
        {
            ret.emplace_back(formatNumberToTex(a1View.get(i), 0));
        }
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_uppercase()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_to_uppercase(const mu::Array& a)
{
    return mu::apply(toUpperCase, a);
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_lowercase()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_to_lowercase(const mu::Array& a)
{
    return mu::apply(toLowerCase, a);
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_ansi()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_utf8ToAnsi(const mu::Array& a)
{
    return mu::apply(utf8ToAnsi, a);
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_utf8()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_ansiToUtf8(const mu::Array& a)
{
    return mu::apply(ansiToUtf8, a);
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_html()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_toHtml(const mu::Array& a)
{
    mu::Array ret;
    ret.copyDims(a);

    for (const mu::Value& val : a)
    {
        ret.emplace_back(mu::to_html(val));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the getenvvar()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_getenvvar(const mu::Array& a)
{
    mu::Array ret;
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a.get(i).getStr().length())
            ret.emplace_back("");

        const char* sVarValue = getenv(a.get(i).getStr().c_str());

        if (!sVarValue)
            ret.emplace_back("");
        else
            ret.emplace_back(sVarValue);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the getfileparts()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_getFileParts(const mu::Array& a)
{
    mu::Array ret;
    ret.copyDims(a);
    FileSystem _fSys;
    _fSys.initializeFromKernel();

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a.get(i).getPath().length())
            ret.emplace_back("");

        ret.emplace_back(_fSys.getFileParts(a.get(i).getPath()));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the getfilediff()
/// function.
///
/// \param a1 const mu::Array&
/// \param a2 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_getFileDiffs(const mu::Array& a1, const mu::Array& a2)
{
    mu::MatrixView a1View(a1);
    mu::MatrixView a2View(a2);
    mu::Array ret = a1View.prepare(a2View);

    size_t elems = a1View.size();

#ifndef PARSERSTANDALONE
    FileSystem _fSys;
    _fSys.initializeFromKernel();

    for (size_t i = 0; i < elems; i++)
    {
        if (!a1View.get(i).getPath().length() || !a2View.get(i).getPath().length())
            ret.emplace_back("");

        std::string sDiffs = compareFiles(_fSys.ValidFileName(a1View.get(i).getPath(), "", false, false),
                                          _fSys.ValidFileName(a2View.get(i).getPath(), "", false, false));
        replaceAll(sDiffs, "\r\n", "\n");
        ret.emplace_back(split(sDiffs, '\n'));
    }
#endif // PARSERSTANDALONE

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the getfilelist()
/// function.
///
/// \param a1 const mu::Array&
/// \param a2 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_getfilelist(const mu::Array& a1, const mu::Array& a2)
{
    mu::MatrixView a1View(a1);
    mu::MatrixView a2View(a2);
    mu::Array ret = a1View.prepare(a2View);

    size_t elems = a1View.size();

#ifndef PARSERSTANDALONE
    FileSystem& _fSys = NumeReKernel::getInstance()->getFileSystem();
    int defaultSettings = FileSystem::FOLLOW_LINKS;

    for (size_t i = 0; i < elems; i++)
    {
        std::vector<std::string> vFileList = _fSys.getFileList(a1View.get(i).getPath(),
                                                               (a2.isDefault() ? 0 : a2View.get(i).getNum().asI64()) | defaultSettings);

        if (!vFileList.size())
            ret.emplace_back("");
        else
            ret.emplace_back(vFileList);
    }

    if (!ret.size())
        ret.emplace_back("");
#endif

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the getfolderlist()
/// function.
///
/// \param a1 const mu::Array&
/// \param a2 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_getfolderlist(const mu::Array& a1, const mu::Array& a2)
{
    mu::MatrixView a1View(a1);
    mu::MatrixView a2View(a2);
    mu::Array ret = a1View.prepare(a2View);

    size_t elems = a1View.size();

#ifndef PARSERSTANDALONE
    FileSystem& _fSys = NumeReKernel::getInstance()->getFileSystem();
    int defaultSettings = FileSystem::FOLLOW_LINKS | FileSystem::NO_RELATIVE_PATH;

    for (size_t i = 0; i < elems; i++)
    {
        std::vector<std::string> vFolderList = _fSys.getFolderList(a1View.get(i).getPath(),
                                                                   (a2.isDefault() ? 0 : a2View.get(i).getNum().asI64()) | defaultSettings);

        if (!vFolderList.size())
            ret.emplace_back("");
        else
            ret.emplace_back(vFolderList);
    }

    if (!ret.size())
        ret.emplace_back("");
#endif

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the strlen()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_strlen(const mu::Array& a)
{
    mu::Array ret;
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        ret.emplace_back(a.get(i).getStr().length());
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the firstch()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_firstch(const mu::Array& a)
{
    mu::Array ret;
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a.get(i).getStr().length())
            ret.emplace_back("");
        else
            ret.emplace_back(std::string(1, a.get(i).getStr().front()));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the lastch()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_lastch(const mu::Array& a)
{
    mu::Array ret;
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a.get(i).getStr().length())
            ret.emplace_back("");
        else
            ret.emplace_back(std::string(1, a.get(i).getStr().back()));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the
/// getmatchingparens() function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_getmatchingparens(const mu::Array& a)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        ret.emplace_back(getMatchingParenthesis(a.get(i).getStr())+1);
    }
#endif
    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the ascii() function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_ascii(const mu::Array& a)
{
    mu::Array ret;
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        mu::Array current;
        current.reserve(a.get(i).getStr().length());

        for (size_t j = 0; j < a.get(i).getStr().length(); j++)
        {
            current.emplace_back((uint8_t)a.get(i).getStr()[j]);
        }

        ret.emplace_back(current);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_blank()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_isblank(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a.get(i).getStr().length())
        {
            ret.emplace_back(false);
            continue;
        }

        mu::Array current;
        current.reserve(a.get(i).getStr().length());

        for (size_t j = 0; j < a.get(i).getStr().length(); j++)
        {
            if (isblank(a.get(i).getStr()[j])
                && _umlauts.lower.find(a.get(i).getStr()[j]) == std::string::npos
                && _umlauts.upper.find(a.get(i).getStr()[j]) == std::string::npos)
                current.emplace_back(true);
            else
                current.emplace_back(false);
        }

        ret.emplace_back(current);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_alnum()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_isalnum(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a.get(i).getStr().length())
        {
            ret.emplace_back(false);
            continue;
        }

        mu::Array current;
        current.reserve(a.get(i).getStr().length());

        for (size_t j = 0; j < a.get(i).getStr().length(); j++)
        {
            if (isalnum(a.get(i).getStr()[j])
                || _umlauts.lower.find(a.get(i).getStr()[j]) != std::string::npos
                || _umlauts.upper.find(a.get(i).getStr()[j]) != std::string::npos)
                current.emplace_back(true);
            else
                current.emplace_back(false);
        }

        ret.emplace_back(current);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_alpha()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_isalpha(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a.get(i).getStr().length())
        {
            ret.emplace_back(false);
            continue;
        }

        mu::Array current;
        current.reserve(a.get(i).getStr().length());

        for (size_t j = 0; j < a.get(i).getStr().length(); j++)
        {
            if (isalpha(a.get(i).getStr()[j])
                || _umlauts.lower.find(a.get(i).getStr()[j]) != std::string::npos
                || _umlauts.upper.find(a.get(i).getStr()[j]) != std::string::npos)
                current.emplace_back(true);
            else
                current.emplace_back(false);
        }

        ret.emplace_back(current);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_cntrl()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_iscntrl(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a.get(i).getStr().length())
        {
            ret.emplace_back(false);
            continue;
        }

        mu::Array current;
        current.reserve(a.get(i).getStr().length());

        for (size_t j = 0; j < a.get(i).getStr().length(); j++)
        {
            if (iscntrl(a.get(i).getStr()[j])
                && _umlauts.lower.find(a.get(i).getStr()[j]) == std::string::npos
                && _umlauts.upper.find(a.get(i).getStr()[j]) == std::string::npos)
                current.emplace_back(true);
            else
                current.emplace_back(false);
        }

        ret.emplace_back(current);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_digit()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_isdigit(const mu::Array& a)
{
    mu::Array ret;
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a.get(i).getStr().length())
        {
            ret.emplace_back(false);
            continue;
        }

        mu::Array current;
        current.reserve(a.get(i).getStr().length());

        for (size_t j = 0; j < a.get(i).getStr().length(); j++)
        {
            if (isdigit(a.get(i).getStr()[j]))
                current.emplace_back(true);
            else
                current.emplace_back(false);
        }

        ret.emplace_back(current);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_graph()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_isgraph(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a.get(i).getStr().length())
        {
            ret.emplace_back(false);
            continue;
        }

        mu::Array current;
        current.reserve(a.get(i).getStr().length());

        for (size_t j = 0; j < a.get(i).getStr().length(); j++)
        {
            if (isgraph(a.get(i).getStr()[j])
                || _umlauts.lower.find(a.get(i).getStr()[j]) != std::string::npos
                || _umlauts.upper.find(a.get(i).getStr()[j]) != std::string::npos)
                current.emplace_back(true);
            else
                current.emplace_back(false);
        }

        ret.emplace_back(current);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_lower()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_islower(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a.get(i).getStr().length())
        {
            ret.emplace_back(false);
            continue;
        }

        mu::Array current;
        current.reserve(a.get(i).getStr().length());

        for (size_t j = 0; j < a.get(i).getStr().length(); j++)
        {
            if (islower(a.get(i).getStr()[j])
                || _umlauts.lower.find(a.get(i).getStr()[j]) != std::string::npos)
                current.emplace_back(true);
            else
                current.emplace_back(false);
        }

        ret.emplace_back(current);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_print()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_isprint(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a.get(i).getStr().length())
        {
            ret.emplace_back(false);
            continue;
        }

        mu::Array current;
        current.reserve(a.get(i).getStr().length());

        for (size_t j = 0; j < a.get(i).getStr().length(); j++)
        {
            if (isprint(a.get(i).getStr()[j])
                || _umlauts.lower.find(a.get(i).getStr()[j]) != std::string::npos
                || _umlauts.upper.find(a.get(i).getStr()[j]) != std::string::npos)
                current.emplace_back(true);
            else
                current.emplace_back(false);
        }

        ret.emplace_back(current);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_punct()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_ispunct(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a.get(i).getStr().length())
        {
            ret.emplace_back(false);
            continue;
        }

        mu::Array current;
        current.reserve(a.get(i).getStr().length());

        for (size_t j = 0; j < a.get(i).getStr().length(); j++)
        {
            if (ispunct(a.get(i).getStr()[j])
                && _umlauts.lower.find(a.get(i).getStr()[j]) == std::string::npos
                && _umlauts.upper.find(a.get(i).getStr()[j]) == std::string::npos)
                current.emplace_back(true);
            else
                current.emplace_back(false);
        }

        ret.emplace_back(current);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_space()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_isspace(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a.get(i).getStr().length())
        {
            ret.emplace_back(false);
            continue;
        }

        mu::Array current;
        current.reserve(a.get(i).getStr().length());

        for (size_t j = 0; j < a.get(i).getStr().length(); j++)
        {
            if (isspace(a.get(i).getStr()[j])
                && _umlauts.lower.find(a.get(i).getStr()[j]) == std::string::npos
                && _umlauts.upper.find(a.get(i).getStr()[j]) == std::string::npos)
                current.emplace_back(true);
            else
                current.emplace_back(false);
        }

        ret.emplace_back(current);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_upper()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_isupper(const mu::Array& a)
{
    static Umlauts _umlauts;
    mu::Array ret;
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a.get(i).getStr().length())
        {
            ret.emplace_back(false);
            continue;
        }

        mu::Array current;
        current.reserve(a.get(i).getStr().length());

        for (size_t j = 0; j < a.get(i).getStr().length(); j++)
        {
            if (isupper(a.get(i).getStr()[j])
                || _umlauts.upper.find(a.get(i).getStr()[j]) != std::string::npos)
                current.emplace_back(true);
            else
                current.emplace_back(false);
        }

        ret.emplace_back(current);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_xdigit()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_isxdigit(const mu::Array& a)
{
    mu::Array ret;
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        if (!a.get(i).getStr().length())
        {
            ret.emplace_back(false);
            continue;
        }

        mu::Array current;
        current.reserve(a.get(i).getStr().length());

        for (size_t j = 0; j < a.get(i).getStr().length(); j++)
        {
            if (isxdigit(a.get(i).getStr()[j]))
                current.emplace_back(true);
            else
                current.emplace_back(false);
        }

        ret.emplace_back(current);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_dirpath()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_isdir(const mu::Array& a)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        ret.emplace_back(is_dir(a.get(i).getPath()));
    }
#endif // PARSERSTANDALONE
    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation if the is_filepath()
/// function.
///
/// \param a const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_isfile(const mu::Array& a)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    ret.copyDims(a);

    for (size_t i = 0; i < a.size(); i++)
    {
        ret.emplace_back(is_file(a.get(i).getPath()));
    }
#endif // PARSERSTANDALONE
    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_char()
/// function.
///
/// \param arrs const mu::MultiArgFuncParams&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_to_char(const mu::MultiArgFuncParams& arrs)
{
    std::string sToChar;

    if (arrs.count() == 1)
    {
        for (size_t i = 0; i < arrs[0].size(); i++)
        {
            if (arrs[0].get(i).isValid())
                sToChar += char(arrs[0].get(i).getNum().asI64());
        }
    }
    else
    {
        for (size_t i = 0; i < arrs.count(); i++)
        {
            if (arrs[i].front().isValid())
                sToChar += char(arrs[i].front().getNum().asI64());
        }
    }

    return mu::Value(sToChar);
}


/////////////////////////////////////////////////
/// \brief Implementation of the findfile()
/// function.
///
/// \param a1 const mu::Array&
/// \param a2 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_findfile(const mu::Array& a1, const mu::Array& a2)
{
    mu::MatrixView a1View(a1);
    mu::MatrixView a2View(a2);
    mu::Array ret = a1View.prepare(a2View);

    size_t elems = a1View.size();

#ifndef PARSERSTANDALONE
    FileSystem _fSys;
    _fSys.initializeFromKernel();

    static std::string sExePath = NumeReKernel::getInstance()->getSettings().getExePath();

    for (size_t i = 0; i < elems; i++)
    {
        if (!a2.isDefault())
            _fSys.setPath(a2View.get(i).getPath(), false, sExePath);
        else
            _fSys.setPath(sExePath, false, sExePath);

        std::string sExtension = ".dat";
        std::string sPath = a1View.get(i).getPath();

        if (sPath.rfind('.') != std::string::npos)
        {
            sExtension = sPath.substr(sPath.rfind('.'));

            if (sExtension.find('*') != std::string::npos || sExtension.find('?') != std::string::npos)
                sExtension = ".dat";
            else
                _fSys.declareFileType(sExtension);
        }

        std::string sFile = _fSys.ValidFileName(sPath, sExtension);

        ret.emplace_back(fileExists(sFile));
    }
#endif

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the split() function.
///
/// \param a1 const mu::Array&
/// \param a2 const mu::Array&
/// \param a3 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_split(const mu::Array& a1, const mu::Array& a2, const mu::Array& a3)
{
    mu::MatrixView a1View(a1);
    mu::MatrixView a2View(a2);
    mu::MatrixView a3View(a3);
    mu::Array ret = a1View.prepare(a2View, a3View);

    size_t elems = a1View.size();

    for (size_t i = 0; i < elems; i++)
    {
        ret.push_back(mu::Value(split_impl(a1View.get(i).getStr(),
                                           a2View.get(i).getStr(),
                                           a3.isDefault() ? false : (bool)a3View.get(i))));
    }

    if (!ret.size())
        ret.emplace_back("");

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_time()
/// function.
///
/// \param a1 const mu::Array&
/// \param a2 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_to_time(const mu::Array& a1, const mu::Array& a2)
{
    mu::MatrixView a1View(a1);
    mu::MatrixView a2View(a2);
    mu::Array ret = a1View.prepare(a2View);

    size_t elems = a1View.size();

    for (size_t i = 0; i < elems; i++)
    {
        std::string sTime = a2View.get(i).getStr() + " ";

        if (!a1View.get(i).getStr().length() && isConvertible(sTime, CONVTYPE_DATE_TIME))
        {
            ret.emplace_back(StrToTime(sTime));
            continue;
        }

        std::string sPattern = a1View.get(i).getStr() + " ";

        if (sTime.length() != sPattern.length())
        {
            ret.emplace_back(NAN);
            continue;
        }

        time_stamp timeStruct = getTimeStampFromTimePoint(sys_time_now());
        time_zone tz = getCurrentTimeZone();
        timeStruct.m_hours = std::chrono::hours::zero();
        timeStruct.m_minutes = std::chrono::minutes::zero();
        timeStruct.m_seconds = std::chrono::seconds::zero();
        timeStruct.m_millisecs = std::chrono::milliseconds::zero();
        timeStruct.m_microsecs = std::chrono::microseconds::zero();

        char cCurrentChar = sPattern.front();
        std::string sCurrentElement;
        date::year y{1970u};// timeStruct.m_ymd.year();

        if (sPattern.find_first_of("MD") != std::string::npos)
            y = timeStruct.m_ymd.year();

        date::month m{1u};// timeStruct.m_ymd.month();
        date::day d{1u};//timeStruct.m_ymd.day();

        for (size_t i = 0; i < sPattern.length(); i++)
        {
            if (sPattern[i] != cCurrentChar)
            {
                switch (cCurrentChar)
                {
                    case 'y':
                    case 'Y': // year is either four or two chars long. The structure expects the time to start at the year 1900
                        if (sCurrentElement.length() > 2)
                            y = date::year(StrToInt(sCurrentElement));
                        else
                            y = date::year(StrToInt(sCurrentElement) + 2000);
                        break;
                    case 'M':
                        m = date::month(StrToInt(sCurrentElement));
                        break;
                    case 'D':
                        d = date::day(StrToInt(sCurrentElement));
                        break;
                    case 'H':
                        timeStruct.m_hours = std::chrono::hours(StrToInt(sCurrentElement));
                        break;
                    case 'h':
                        timeStruct.m_hours = std::chrono::hours(StrToInt(sCurrentElement) + (tz.Bias + tz.DayLightBias).count() / 60);
                        break;
                    case 'm':
                        timeStruct.m_minutes = std::chrono::minutes(StrToInt(sCurrentElement));
                        break;
                    case 's':
                        timeStruct.m_seconds = std::chrono::seconds(StrToInt(sCurrentElement));
                        break;
                    case 'i':
                        sCurrentElement.append(3-sCurrentElement.size(), '0');
                        timeStruct.m_millisecs = std::chrono::milliseconds(StrToInt(sCurrentElement));
                        break;
                    case 'u':
                        sCurrentElement.append(3-sCurrentElement.size(), '0');
                        timeStruct.m_microsecs = std::chrono::microseconds(StrToInt(sCurrentElement));
                        break;
                }

                cCurrentChar = sPattern[i];
                sCurrentElement.clear();
            }

            sCurrentElement += sTime[i];
        }

        timeStruct.m_ymd = date::year_month_day(y,m,d);

        ret.emplace_back(getTimePointFromTimeStamp(timeStruct));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the strfnd()
/// function.
///
/// \param what const mu::Array&
/// \param where const mu::Array&
/// \param from const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_strfnd(const mu::Array& what, const mu::Array& where, const mu::Array& from)
{
    mu::MatrixView whatView(what);
    mu::MatrixView whereView(where);
    mu::MatrixView fromView(from);
    mu::Array ret = whatView.prepare(whereView, fromView);

    size_t elems = whatView.size();

    for (size_t i = 0; i < elems; i++)
    {
        if (!from.isDefault())
            ret.emplace_back(strfnd_impl(whereView.get(i).getStr(), whatView.get(i).getStr(), fromView.get(i).getNum().asI64()-1));
        else
            ret.emplace_back(strfnd_impl(whereView.get(i).getStr(), whatView.get(i).getStr()));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the strrfnd()
/// function.
///
/// \param what const mu::Array&
/// \param where const mu::Array&
/// \param from const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_strrfnd(const mu::Array& what, const mu::Array& where, const mu::Array& from)
{
    mu::MatrixView whatView(what);
    mu::MatrixView whereView(where);
    mu::MatrixView fromView(from);
    mu::Array ret = whatView.prepare(whereView, fromView);

    size_t elems = whatView.size();

    for (size_t i = 0; i < elems; i++)
    {
        if (!from.isDefault())
            ret.emplace_back(strrfnd_impl(whereView.get(i).getStr(), whatView.get(i).getStr(), fromView.get(i).getNum().asI64()-1));
        else
            ret.emplace_back(strrfnd_impl(whereView.get(i).getStr(), whatView.get(i).getStr()));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the strmatch()
/// function.
///
/// \param chars const mu::Array&
/// \param where const mu::Array&
/// \param from const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_strmatch(const mu::Array& chars, const mu::Array& where, const mu::Array& from)
{
    mu::MatrixView charsView(chars);
    mu::MatrixView whereView(where);
    mu::MatrixView fromView(from);
    mu::Array ret = charsView.prepare(whereView, fromView);

    size_t elems = charsView.size();

    for (size_t i = 0; i < elems; i++)
    {
        if (!from.isDefault())
            ret.emplace_back(strmatch_impl(whereView.get(i).getStr(), charsView.get(i).getStr(), fromView.get(i).getNum().asI64()-1));
        else
            ret.emplace_back(strmatch_impl(whereView.get(i).getStr(), charsView.get(i).getStr()));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the strrmatch()
/// function.
///
/// \param chars const mu::Array&
/// \param where const mu::Array&
/// \param from const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_strrmatch(const mu::Array& chars, const mu::Array& where, const mu::Array& from)
{
    mu::MatrixView charsView(chars);
    mu::MatrixView whereView(where);
    mu::MatrixView fromView(from);
    mu::Array ret = charsView.prepare(whereView, fromView);

    size_t elems = charsView.size();

    for (size_t i = 0; i < elems; i++)
    {
        if (!from.isDefault())
            ret.emplace_back(strrmatch_impl(whereView.get(i).getStr(), charsView.get(i).getStr(), fromView.get(i).getNum().asI64()-1));
        else
            ret.emplace_back(strrmatch_impl(whereView.get(i).getStr(), charsView.get(i).getStr()));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the str_not_match()
/// function.
///
/// \param chars const mu::Array&
/// \param where const mu::Array&
/// \param from const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_str_not_match(const mu::Array& chars, const mu::Array& where, const mu::Array& from)
{
    mu::MatrixView charsView(chars);
    mu::MatrixView whereView(where);
    mu::MatrixView fromView(from);
    mu::Array ret = charsView.prepare(whereView, fromView);

    size_t elems = charsView.size();

    for (size_t i = 0; i < elems; i++)
    {
        if (!from.isDefault())
            ret.emplace_back(str_not_match_impl(whereView.get(i).getStr(), charsView.get(i).getStr(), fromView.get(i).getNum().asI64()-1));
        else
            ret.emplace_back(str_not_match_impl(whereView.get(i).getStr(), charsView.get(i).getStr()));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the str_not_rmatch()
/// function.
///
/// \param chars const mu::Array&
/// \param where const mu::Array&
/// \param from const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_str_not_rmatch(const mu::Array& chars, const mu::Array& where, const mu::Array& from)
{
    mu::MatrixView charsView(chars);
    mu::MatrixView whereView(where);
    mu::MatrixView fromView(from);
    mu::Array ret = charsView.prepare(whereView, fromView);

    size_t elems = charsView.size();

    for (size_t i = 0; i < elems; i++)
    {
        if (!from.isDefault())
            ret.emplace_back(str_not_rmatch_impl(whereView.get(i).getStr(), charsView.get(i).getStr(), fromView.get(i).getNum().asI64()-1));
        else
            ret.emplace_back(str_not_rmatch_impl(whereView.get(i).getStr(), charsView.get(i).getStr()));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the strfndall()
/// function.
///
/// \param what const mu::Array&
/// \param where const mu::Array&
/// \param from const mu::Array&
/// \param to const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_strfndall(const mu::Array& what, const mu::Array& where, const mu::Array& from, const mu::Array& to)
{
    mu::MatrixView whatView(what);
    mu::MatrixView whereView(where);
    mu::MatrixView fromView(from);
    mu::MatrixView toView(to);
    mu::Array ret = whatView.prepare(whereView, fromView, toView);

    size_t elems = whatView.size();

    for (size_t i = 0; i < elems; i++)
    {
        if (!whereView.get(i).getStr().length())
        {
            ret.emplace_back(0u);
            continue;
        }

        size_t pos_start = 0;
        size_t pos_last;

        mu::Array current;

        if (!from.isDefault()
            && fromView.get(i).getNum().asI64() > 0
            && fromView.get(i).getNum().asI64() <= (int64_t)whereView.get(i).getStr().length())
            pos_start = fromView.get(i).getNum().asI64()-1;

        if (!to.isDefault()
            && toView.get(i).getNum().asI64() > 0
            && toView.get(i).getNum().asI64() <= (int64_t)whereView.get(i).getStr().length())
            pos_last = toView.get(i).getNum().asI64() - whatView.get(i).getStr().length();
        else
            pos_last = whereView.get(i).getStr().length() - whatView.get(i).getStr().length();

        while (pos_start <= pos_last)
        {
            pos_start = whereView.get(i).getStr().find(whatView.get(i).getStr(), pos_start);

            if (pos_start <= pos_last)
            {
                pos_start++;
                current.emplace_back(pos_start);
            }
        }

        if (!current.size())
            ret.emplace_back(0u);
        else
            ret.emplace_back(current);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the strmatchall()
/// function.
///
/// \param chars const mu::Array&
/// \param where const mu::Array&
/// \param from const mu::Array&
/// \param to const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_strmatchall(const mu::Array& chars, const mu::Array& where, const mu::Array& from, const mu::Array& to)
{
    mu::MatrixView charsView(chars);
    mu::MatrixView whereView(where);
    mu::MatrixView fromView(from);
    mu::MatrixView toView(to);
    mu::Array ret = charsView.prepare(whereView, fromView, toView);

    size_t elems = charsView.size();

    for (size_t i = 0; i < elems; i++)
    {
        if (!whereView.get(i).getStr().length())
        {
            ret.emplace_back(0u);
            continue;
        }

        size_t pos_start = 0;
        size_t pos_last;

        mu::Array current;

        if (!from.isDefault()
            && fromView.get(i).getNum().asI64() > 0
            && fromView.get(i).getNum().asI64() <= (int64_t)whereView.get(i).getStr().length())
            pos_start = fromView.get(i).getNum().asI64()-1;

        if (!to.isDefault()
            && toView.get(i).getNum().asI64() > 0
            && toView.get(i).getNum().asI64() <= (int64_t)whereView.get(i).getStr().length())
            pos_last = toView.get(i).getNum().asI64()-1;
        else
            pos_last = whereView.get(i).getStr().length()-1;

        for (size_t j = 0; j < charsView.get(i).getStr().length(); j++)
        {
            size_t match = whereView.get(i).getStr().find(charsView.get(i).getStr()[j], pos_start);

            if (match <= pos_last)
                current.emplace_back(match+1);
            else
                current.emplace_back(0u);
        }

        ret.emplace_back(current);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the findparam
/// function.
///
/// \param par const mu::Array&
/// \param line const mu::Array&
/// \param following const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_findparam(const mu::Array& par, const mu::Array& line, const mu::Array& following)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    ret.reserve(std::max({par.size(), line.size(), following.size()}));

    for (size_t i = 0; i < std::max({par.size(), line.size(), following.size()}); i++)
    {
        if (!line.get(i).getStr().length())
        {
            ret.emplace_back(0);
            continue;
        }

        size_t nMatch;

        if (!following.isDefault())
            nMatch = findParameter(line.get(i).getStr(), par.get(i).getStr(), following.get(i).getStr().front());
        else
            nMatch = findParameter(line.get(i).getStr(), par.get(i).getStr());

        if (nMatch)
            ret.emplace_back(nMatch); // findParameter returns already pos+1
        else
            ret.emplace_back(0);
    }
#endif
    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the substr()
/// function.
///
/// \param sStr const mu::Array&
/// \param pos const mu::Array&
/// \param len const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_substr(const mu::Array& sStr, const mu::Array& pos, const mu::Array& len)
{
    mu::MatrixView strView(sStr);
    mu::MatrixView posView(pos);
    mu::MatrixView lenView(len);
    mu::Array ret = strView.prepare(posView, lenView);

    size_t elems = strView.size();

    for (size_t i = 0; i < elems; i++)
    {
        if (!len.isDefault())
            ret.emplace_back(substr_impl(strView.get(i).getStr(), posView.get(i).getNum().asI64()-1, lenView.get(i).getNum().asI64()));
        else
            ret.emplace_back(substr_impl(strView.get(i).getStr(), posView.get(i).getNum().asI64()-1));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the repeat()
/// function.
///
/// \param sStr const mu::Array&
/// \param rep const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_repeat(const mu::Array& sStr, const mu::Array& rep)
{
    mu::MatrixView strView(sStr);
    mu::MatrixView repView(rep);
    mu::Array ret = strView.prepare(repView);

    size_t elems = strView.size();

    for (size_t i = 0; i < elems; i++)
    {
        ret.emplace_back(strRepeat(strView.get(i).getStr(), repView.get(i).getNum().asI64()));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Creates a padding string full of \c 0.
///
/// \param nTime int
/// \param nLength size_t
/// \return std::string
///
/////////////////////////////////////////////////
static std::string padWithZeros(int nTime, size_t nLength)
{
    std::string sPadded = toString(nTime);
    sPadded.insert(0, nLength - sPadded.length(), '0');
    return sPadded;
}


/////////////////////////////////////////////////
/// \brief Implementation of the timeformat()
/// function.
///
/// \param fmt const mu::Array&
/// \param time const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_timeformat(const mu::Array& fmt, const mu::Array& time)
{
    mu::MatrixView fmtView(fmt);
    mu::MatrixView timeView(time);
    mu::Array ret = fmtView.prepare(timeView);

    size_t elems = fmtView.size();

    for (size_t i = 0; i < elems; i++)
    {
        if (!fmtView.get(i).getStr().length())
        {
            ret.emplace_back(toString(to_timePoint(timeView.get(i).getNum().asF64()), 0));
            continue;
        }

        std::string sFormattedTime = fmtView.get(i).getStr() + " "; // contains pattern
        sys_time_point nTime = to_timePoint(timeView.get(i).getNum().asF64());
        time_stamp timeStruct = getTimeStampFromTimePoint(nTime);
        time_zone tz = getCurrentTimeZone();

        char cCurrentChar = sFormattedTime.front();
        size_t currentElementStart = 0;

        for (size_t i = 0; i < sFormattedTime.length(); i++)
        {
            if (cCurrentChar != sFormattedTime[i])
            {
                switch (cCurrentChar)
                {
                    case 'Y':
                    case 'y':
                        if (i - currentElementStart > 2)
                            sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(int(timeStruct.m_ymd.year()), i - currentElementStart));
                        else
                            sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(int(timeStruct.m_ymd.year()) - 100 * (int(timeStruct.m_ymd.year()) / 100), i - currentElementStart));
                        break;
                    case 'M':
                        sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(unsigned(timeStruct.m_ymd.month()), i - currentElementStart));
                        break;
                    case 'D':
                        sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(unsigned(timeStruct.m_ymd.day()), i - currentElementStart));
                        break;
                    case 'd':
                        sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros((date::sys_days(timeStruct.m_ymd) - date::sys_days(timeStruct.m_ymd.year()/1u/1u)).count()+1, i - currentElementStart));
                        break;
                    case 'H':
                        sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct.m_hours.count(), i - currentElementStart));
                        break;
                    case 'h':
                        if (timeStruct.m_hours.count() - (tz.Bias + tz.DayLightBias).count() / 60 < 0)
                            sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct.m_hours.count() + 24 - (tz.Bias + tz.DayLightBias).count() / 60, i - currentElementStart));
                        else if (timeStruct.m_hours.count() - (tz.Bias + tz.DayLightBias).count() / 60  >= 24)
                            sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct.m_hours.count() - 24 - (tz.Bias + tz.DayLightBias).count() / 60, i - currentElementStart));
                        else
                            sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct.m_hours.count() - (tz.Bias + tz.DayLightBias).count() / 60, i - currentElementStart));
                        break;
                    case 'm':
                        sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct.m_minutes.count(), i - currentElementStart));
                        break;
                    case 's':
                        sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct.m_seconds.count(), i - currentElementStart));
                        break;
                    case 'i':
                        sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct.m_millisecs.count(), i - currentElementStart));
                        break;
                    case 'u':
                        sFormattedTime.replace(currentElementStart, i - currentElementStart, padWithZeros(timeStruct.m_microsecs.count(), i - currentElementStart));
                        break;
                }

                currentElementStart = i;
                cCurrentChar = sFormattedTime[i];
            }
        }

        ret.emplace_back(sFormattedTime.substr(0, sFormattedTime.length()-1));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the weekday()
/// function.
///
/// \param daynum const mu::Array&
/// \param opts const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_weekday(const mu::Array& daynum, const mu::Array& opts)
{
    mu::MatrixView dayNumView(daynum);
    mu::MatrixView optsView(opts);
    mu::Array ret = dayNumView.prepare(optsView);

    size_t elems = dayNumView.size();

    for (size_t i = 0; i < elems; i++)
    {
        sys_time_point nTime = to_timePoint(dayNumView.get(i).getNum().asF64());

        size_t day = getWeekDay(nTime);

        if (opts.isDefault()|| !optsView.get(i))
        {
            ret.emplace_back(day);
            continue;
        }

        static std::vector<std::string> weekdays = _lang.getList("COMMON_WEEKDAY_*");

        if (weekdays.size() >= 7)
            ret.emplace_back(weekdays[day-1]);
        else
            ret.emplace_back("UNDEFINED");
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the char() function.
///
/// \param sStr const mu::Array&
/// \param pos const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_char(const mu::Array& sStr, const mu::Array& pos)
{
    mu::MatrixView strView(sStr);
    mu::MatrixView posView(pos);
    mu::Array ret = strView.prepare(posView);

    size_t elems = strView.size();

    for (size_t i = 0; i < elems; i++)
    {
        const std::string& s = strView.get(i).getStr();
        int64_t p = posView.get(i).getNum().asI64();

        if (p <= 1)
            ret.emplace_back(s.substr(0, 1));
        else if (p >= (int64_t)s.length())
            ret.emplace_back(s.substr(s.length()-1, 1));
        else
            ret.emplace_back(s.substr(p-1, 1));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the getopt()
/// function.
///
/// \param sStr const mu::Array&
/// \param pos const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_getopt(const mu::Array& sStr, const mu::Array& pos)
{
    mu::MatrixView strView(sStr);
    mu::MatrixView posView(pos);
    mu::Array ret = strView.prepare(posView);

    size_t elems = strView.size();

#ifndef PARSERSTANDALONE
    for (size_t i = 0; i < elems; i++)
    {
        const std::string& s = strView.get(i).getStr();
        size_t p = posView.get(i).getNum().asUI64();

        if (p < 1)
            p = 1;

        if (p > s.length())
            ret.emplace_back("");
        else
            ret.emplace_back(getArgAtPos(s, p-1));
    }
#endif

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the replace()
/// function.
///
/// \param where const mu::Array&
/// \param from const mu::Array&
/// \param len const mu::Array&
/// \param rep const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_replace(const mu::Array& where, const mu::Array& from, const mu::Array& len, const mu::Array& rep)
{
    mu::MatrixView whereView(where);
    mu::MatrixView fromView(from);
    mu::MatrixView lenView(len);
    mu::MatrixView repView(rep);
    mu::Array ret = whereView.prepare(fromView, lenView, repView);

    size_t elems = whereView.size();

    for (size_t i = 0; i < elems; i++)
    {
        std::string s = whereView.get(i).getStr();
        const std::string& r = repView.get(i).getStr();

        int64_t p = std::min((int64_t)s.length(), std::max(1LL, fromView.get(i).getNum().asI64()));
        int64_t l = lenView.get(i).getNum().asI64();

        if (!s.length())
            ret.emplace_back("");
        else
            ret.emplace_back(s.replace(p-1, l, r));
    }

    return ret;
}


enum NumberBase
{
    LOG,
    BIN,
    OCT,
    HEX
};


/////////////////////////////////////////////////
/// \brief Static helper function for converting
/// number bases into the decimal base.
///
/// \param value StringView
/// \param base ValueBase
/// \return int64_t
///
/////////////////////////////////////////////////
static int64_t convertBaseToDecimal(StringView value, NumberBase base)
{
    std::stringstream stream;
    int64_t ret = 0;

    if (base == HEX)
        stream.setf(std::ios::hex, std::ios::basefield);
    else if (base == OCT)
        stream.setf(std::ios::oct, std::ios::basefield);
    else if (base == BIN)
    {
        for (int i = value.length() - 1; i >= 0; i--)
        {
            if (value[i] == '1')
                ret += intPower(2, value.length()-1 - i);
        }

        return ret;
    }
    else if (base == LOG)
    {
        if (toLowerCase(value.to_string()) == "true" || value == "1")
            return 1LL;

        return 0LL;
    }

    stream << value.to_string();
    stream >> ret;

    return ret;
}


/////////////////////////////////////////////////
/// \brief Simple helper to parse the exponents
/// in LaTeX format.
///
/// \param sExpr std::string&
/// \return double
///
/////////////////////////////////////////////////
static double extractLaTeXExponent(std::string& sExpr)
{
    double val;

    if (sExpr.find('{') != std::string::npos)
    {
        // a^{xyz}
        val = std::stod(sExpr.substr(sExpr.find('{')+1, sExpr.find('}')));
        sExpr.erase(0, sExpr.find('}')+1);
    }
    else
    {
        // a^b
        val = std::stod(sExpr.substr(sExpr.find_first_not_of("^ "), 1));
        sExpr.erase(0, sExpr.find_first_not_of("^ ")+1);
    }

    return val;
}


/////////////////////////////////////////////////
/// \brief Implementation of the textparse()
/// function.
///
/// \param sStr const mu::Array&
/// \param pattern const mu::Array&
/// \param p1 const mu::Array&
/// \param p2 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_textparse(const mu::Array& sStr, const mu::Array& pattern, const mu::Array& p1, const mu::Array& p2)
{
    mu::MatrixView strView(sStr);
    mu::MatrixView patternView(pattern);
    mu::MatrixView p1View(p1);
    mu::MatrixView p2View(p2);
    mu::Array ret = strView.prepare(patternView, p1View, p2View);

    size_t elems = strView.size();

    for (size_t i = 0; i < elems; i++)
    {
        StringView sSearchString = strView.get(i).getStr();
        StringView sPattern = patternView.get(i).getStr();

        int64_t pos1 = 1;
        int64_t pos2 = sSearchString.length();

        if (!p1.isDefault())
            pos1 = std::max(pos1, p1View.get(i).getNum().asI64());

        if (!p2.isDefault())
            pos2 = std::min(pos2, std::max(1LL, p2View.get(i).getNum().asI64()));

        // Exclude border cases
        if (!sSearchString.length() || pos1 > (int64_t)sSearchString.length() || pos2-pos1 <= 0)
        {
            ret.emplace_back("");
            continue;
        }
        else if (!sPattern.length())
        {
            ret.emplace_back(sSearchString.to_string());
            continue;
        }

        sSearchString = sSearchString.subview(pos1-1, pos2-pos1+1);

        // Examples for text, which shall be parsed
        // 2018-09-21: Message VAL=12452
        // %s: %s VAL=%f
        // {sDate, sMessage, fValue} = textparse("2018-09-21: Message VAL=12452", "%s: %s VAL=%f");

        size_t lastPosition = 0;
        static const std::string sIDENTIFIERCHARS = "sfaLlthbo";

        std::vector<StringView> vPatterns;
        mu::Array current;

        // Tokenize the search pattern
        while (sPattern.length())
        {
            // Handle single char pattern stubs first
            if (sPattern.length() == 1)
            {
                vPatterns.push_back(sPattern);
                break;
            }

            for (size_t i = 0; i < sPattern.length()-1; i++)
            {
                if (sPattern[i] == '%' && sIDENTIFIERCHARS.find(sPattern[i+1]) != std::string::npos)
                {
                    vPatterns.push_back(sPattern.subview(0, i));
                    vPatterns.push_back(sPattern.subview(i, 2));

                    if (vPatterns.back() == "%s")
                        current.emplace_back("");
                    else if (vPatterns.back() != "%a")
                        current.emplace_back(std::complex<double>(NAN));

                    sPattern.trim_front(i+2);
                    break;
                }

                if (i+2 == sPattern.length())
                {
                    vPatterns.push_back(sPattern);
                    sPattern.clear();
                    break;
                }
            }
        }

        // If the search string starts with whitespaces and the
        // pattern doesn't start with a percentage sign, search
        // for the first non-whitespace character
        if (!vPatterns.front().length() && sSearchString.front() == ' ')
        {
            lastPosition = sSearchString.find_first_not_of(' ');

            if (lastPosition == std::string::npos)
                continue;
        }
        else if (vPatterns.front().length())
        {
            lastPosition = sSearchString.find(vPatterns.front().to_string());

            if (lastPosition == std::string::npos)
                continue;

            lastPosition += vPatterns.front().length();
        }

        size_t nth_token = 0;

        // Patterns and identifiers are alternating, starting with
        // identifiers
        for (size_t n = 1; n < vPatterns.size(); n+=2)
        {
            size_t pos = sSearchString.length() + lastPosition;

            //if ((int64_t)pos >= pos2)
            //    break;

            StringView nextPattern;

            if (vPatterns.size() > n+1)
                nextPattern = vPatterns[n+1];

            if (nextPattern.length())
                pos = sSearchString.find(nextPattern.to_string(), lastPosition);

            if (pos == std::string::npos)
                break;

            // Append the found token
            if (vPatterns[n] == "%s")
                current[nth_token] = sSearchString.subview(lastPosition, pos - lastPosition).to_string();
            else if (vPatterns[n] == "%h")
                current[nth_token] = mu::Value(convertBaseToDecimal(sSearchString.subview(lastPosition, pos - lastPosition), HEX));
            else if (vPatterns[n] == "%o")
                current[nth_token] = mu::Value(convertBaseToDecimal(sSearchString.subview(lastPosition, pos - lastPosition), OCT));
            else if (vPatterns[n] == "%b")
                current[nth_token] = mu::Value(convertBaseToDecimal(sSearchString.subview(lastPosition, pos - lastPosition), BIN));
            else if (vPatterns[n] == "%l")
                current[nth_token] = mu::Value(convertBaseToDecimal(sSearchString.subview(lastPosition, pos - lastPosition), LOG));
            else if (vPatterns[n] == "%t")
                current[nth_token] = mu::Value(StrToTime(sSearchString.subview(lastPosition, pos - lastPosition).to_string()));
            else if (vPatterns[n] == "%f")
            {
                std::string sFloatingPoint = sSearchString.subview(lastPosition, pos - lastPosition).to_string();

                if (sFloatingPoint.find('.') == std::string::npos)
                    replaceAll(sFloatingPoint, ",", ".");

                current[nth_token] = isConvertible(sFloatingPoint, CONVTYPE_VALUE) ? StrToCmplx(sFloatingPoint) : NAN;
            }
            else if (vPatterns[n] == "%L")
            {
                StringView sLaTeXView = sSearchString.subview(lastPosition, pos - lastPosition);
                sLaTeXView.strip();

                if (sLaTeXView.front() == '$' && sLaTeXView.back() == '$')
                {
                    sLaTeXView.trim_front(1);
                    sLaTeXView.trim_back(1);
                    sLaTeXView.strip();
                }

                std::string sLaTeXFormatted = sLaTeXView.to_string();

                replaceAll(sLaTeXFormatted, "{,}", ".");
                replaceAll(sLaTeXFormatted, "\\times", "*");
                replaceAll(sLaTeXFormatted, "\\cdot", "*");
                replaceAll(sLaTeXFormatted, "2\\pi", "6.283185");
                replaceAll(sLaTeXFormatted, "\\pi", "3.1415926");
                replaceAll(sLaTeXFormatted, "\\infty", "inf");
                replaceAll(sLaTeXFormatted, "---", "nan");
                replaceAll(sLaTeXFormatted, "\\,", " ");
                // 1.0*10^{-5} 1.0*10^2 1.0*10^3 2.5^{0.5}

                if (sLaTeXFormatted.find('^') != std::string::npos)
                {
                    std::complex<double> vVal = 0;
                    std::complex<double> vValFinal = 0;

                    size_t nOpPos = std::string::npos;

                    // Go through all exponents and parse them into complex double format
                    while (sLaTeXFormatted.length())
                    {
                        nOpPos = sLaTeXFormatted.find_first_of("*^");
                        vVal = StrToCmplx(sLaTeXFormatted.substr(0, nOpPos));

                        if (sLaTeXFormatted[nOpPos] == '*')
                        {
                            sLaTeXFormatted.erase(0, nOpPos+1);
                            nOpPos = sLaTeXFormatted.find('^');
                            std::complex<double> vBase = StrToCmplx(sLaTeXFormatted.substr(0, nOpPos));
                            sLaTeXFormatted.erase(0, nOpPos+1);
                            vVal *= std::pow(vBase, extractLaTeXExponent(sLaTeXFormatted));
                        }
                        else
                        {
                            sLaTeXFormatted.erase(0, nOpPos+1);
                            vVal = std::pow(vVal, extractLaTeXExponent(sLaTeXFormatted));
                        }

                        if (sLaTeXFormatted.find_first_not_of(" *") != std::string::npos
                            && tolower(sLaTeXFormatted[sLaTeXFormatted.find_first_not_of(" *")]) == 'i')
                        {
                            vVal = std::complex<double>(vVal.imag(), vVal.real());
                            sLaTeXFormatted.erase(0, sLaTeXFormatted.find_first_of("iI")+1);
                        }

                        vValFinal += vVal;

                        if (sLaTeXFormatted.find_first_not_of(' ') == std::string::npos)
                            break;
                    }

                    current[nth_token] = vValFinal;
                }
                else // This can handle simple multiplications
                    current[nth_token] = StrToCmplx(sLaTeXFormatted);
            }

            if (vPatterns[n] != "%a")
                nth_token++;

            // Store the position of the separator
            lastPosition = pos + nextPattern.length();
        }

        ret.emplace_back(current);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the locate()
/// function.
///
/// \param arr const mu::Array&
/// \param tofind const mu::Array&
/// \param tol const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_locate(const mu::Array& arr, const mu::Array& tofind, const mu::Array& tol)
{
    if (!arr.size())
        return mu::Value(false);

    mu::Array ret;
    ret.reserve(std::max(tofind.size(), tol.size()));

    for (size_t j = 0; j < std::max(tofind.size(), tol.size()); j++)
    {
        int64_t t = 0;
        size_t offset = ret.size();
        StringView sView = tofind.get(j).getStr();

        if (!tol.isDefault())
            t = tol.get(j).getNum().asI64();

        // Examine the whole string array
        for (size_t i = 0; i < arr.size(); i++)
        {
            StringView arg = arr.get(i).getStr();

            // Apply the chosen matching method
            if (t == 1)
            {
                // Remove surrounding whitespaces and compare
                arg.strip();

                if (arg == sView)
                    ret.emplace_back(i+1);
            }
            else if (t == 2)
            {
                // Take the first non-whitespace characters
                if (arg.find_first_not_of(' ') != std::string::npos
                    && arg.subview(arg.find_first_not_of(' '), sView.length()) == sView)
                    ret.emplace_back(i+1);
            }
            else if (t == 3)
            {
                // Take the last non-whitespace characters
                if (arg.find_last_not_of(' ') != std::string::npos
                    && arg.find_last_not_of(' ')+1 >= sView.length()
                    && arg.subview(arg.find_last_not_of(' ')-sView.length()+1, sView.length()) == sView)
                    ret.emplace_back(i+1);
            }
            else if (t == 4)
            {
                // Search anywhere in the string
                if (arg.find(sView.to_string()) != std::string::npos)
                    ret.emplace_back(i+1);
            }
            else if (t == 5)
            {
                // Search any of the characters in the string
                if (arg.find_first_of(sView.to_string()) != std::string::npos)
                    ret.emplace_back(i+1);
            }
            else
            {
                // Simply compare
                if (arg == sView)
                    ret.emplace_back(i+1);
            }
        }

        if (ret.size() == offset)
            ret.emplace_back(false);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the strunique()
/// function.
///
/// \param arr const mu::Array&
/// \param opts const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_strunique(const mu::Array& arr, const mu::Array& opts)
{
    mu::Array ret;
    ret.reserve(std::max((size_t)1, opts.size()));

    for (size_t j = 0; j < std::max((size_t)1, opts.size()); j++)
    {
        int64_t o = arr.size() > 1 ? 0 : 1;

        if (!opts.isDefault())
            o = opts.get(j).getNum().asI64();

        // Separate unique strings from unique chars
        if (o == 0)
        {
            // Create a copy of all values
            std::vector<std::string> vFuncArgs;

            for (size_t i = 0; i < arr.size(); i++)
            {
                vFuncArgs.push_back(arr.get(i).getStr());
            }

            // Sort and isolate the unique values
            std::sort(vFuncArgs.begin(), vFuncArgs.end());
            auto iter = std::unique(vFuncArgs.begin(), vFuncArgs.end());

            // Copy together
            for (auto it = vFuncArgs.begin(); it != iter; ++it)
            {
                ret.emplace_back(*it);
            }
        }
        else
        {
            // Examine each value independently
            for (size_t i = 0; i < arr.size(); i++)
            {
                // Get a quotation mark free copy
                std::string sArg = arr.get(i).getStr();

                // Sort and isolate the unique chars
                std::sort(sArg.begin(), sArg.end());
                auto iter = std::unique(sArg.begin(), sArg.end());

                // Append the string with unique characters
                ret.emplace_back(std::string(sArg.begin(), iter));
            }
        }
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the strjoin()
/// function.
///
/// \param arr const mu::Array&
/// \param sep const mu::Array&
/// \param opts const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_strjoin(const mu::Array& arr, const mu::Array& sep, const mu::Array& opts)
{
    mu::Array ret;
    ret.reserve(std::max({(size_t)1, sep.size(), opts.size()}));

    for (size_t j = 0; j < std::max({(size_t)1, sep.size(), opts.size()}); j++)
    {
        std::string sJoined;
        std::string sSeparator;
        int64_t keepEmpty = 0;

        if (!sep.isDefault())
            sSeparator = sep.get(j).getStr();

        if (!opts.isDefault())
            keepEmpty = opts.get(j).getNum().asI64();

        for (size_t i = 0; i < arr.size(); i++)
        {
            // Only insert the separator if either the string as well
            // as the following one have a length or if also empty strings
            // shall be kept and this is the second string to be added
            if ((sJoined.length() && arr.get(i).getStr().length())
                || (i && keepEmpty))
                sJoined += sSeparator;

            sJoined += arr.get(i).getStr();
        }

        ret.emplace_back(sJoined);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the getkeyval()
/// function.
///
/// \param kvlist const mu::Array&
/// \param key const mu::Array&
/// \param defs const mu::Array&
/// \param opts const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_getkeyval(const mu::Array& kvlist, const mu::Array& key, const mu::Array& defs, const mu::Array& opts)
{
    mu::Array ret;
    ret.reserve(std::max({key.size(), defs.size(), opts.size()}));

    for (size_t j = 0; j < std::max({key.size(), defs.size(), opts.size()}); j++)
    {
        StringView sKey = key.get(j).getStr();
        int64_t o = 0;
        bool found = false;

        if (!opts.isDefault())
            o = opts.get(j).getNum().asI64();

        // Examine the whole string array
        for (size_t i = 0; i < kvlist.size(); i+=2)
        {
            // Remove the masked strings
            StringView arg = kvlist.get(i).getStr();

            // Remove surrounding whitespaces and compare
            arg.strip();

            if (arg == sKey)
            {
                ret.emplace_back(kvlist.get(i+1));
                found = true;
            }
        }

        // set values to the default values and probably
        // issue a warning, if no values were found
        if (!found)
        {
#ifndef PARSERSTANDALONE
            if (o)
                NumeReKernel::issueWarning(_lang.get("PARSERFUNCS_LISTFUNC_GETKEYVAL_WARNING", "\"" + sKey.to_string() + "\""));
#endif
            if (!defs.isDefault())
                ret.emplace_back(defs.get(j));
            else
                ret.emplace_back("");
        }
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the findtoken()
/// function.
///
/// \param sStr const mu::Array&
/// \param tok const mu::Array&
/// \param sep const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_findtoken(const mu::Array& sStr, const mu::Array& tok, const mu::Array& sep)
{
    mu::MatrixView strView(sStr);
    mu::MatrixView tokView(tok);
    mu::MatrixView sepView(sep);
    mu::Array ret = strView.prepare(tokView, sepView);

    size_t elems = strView.size();

    for (size_t i = 0; i < elems; i++)
    {
        StringView sView1 = strView.get(i).getStr();
        const std::string& t = tokView.get(i).getStr();
        std::string s = " \t";

        // Define default arguments
        if (!sep.isDefault())
            s = sepView.get(i).getStr();

        size_t nMatch = 0;

        // search the first match of the token, which is surrounded by the
        // defined separator characters
        while ((nMatch = sView1.find(t, nMatch)) != std::string::npos)
        {
            if ((!nMatch || s.find(sView1[nMatch-1]) != std::string::npos)
                && (nMatch + t.length() >= sView1.length() || s.find(sView1[nMatch+t.length()]) != std::string::npos))
            {
                ret.emplace_back(mu::Value(nMatch+1));
                break;
            }

            nMatch++;
        }

        if (nMatch == std::string::npos)
            ret.emplace_back(mu::Value(0));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the replaceall()
/// function.
///
/// \param sStr const mu::Array&
/// \param fnd const mu::Array&
/// \param rep const mu::Array&
/// \param pos1 const mu::Array&
/// \param pos2 const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_replaceall(const mu::Array& sStr, const mu::Array& fnd, const mu::Array& rep, const mu::Array& pos1, const mu::Array& pos2)
{
    mu::MatrixView strView(sStr);
    mu::MatrixView fndView(fnd);
    mu::MatrixView repView(rep);
    mu::MatrixView pos1View(pos1);
    mu::MatrixView pos2View(pos2);
    mu::Array ret = strView.prepare(fndView, repView, pos1View, pos2View);

    size_t elems = strView.size();

    for (size_t i = 0; i < elems; i++)
    {
        std::string s = strView.get(i).getStr();
        StringView f = fndView.get(i).getStr();
        StringView r = repView.get(i).getStr();

        if (!s.length() || !f.length())
        {
            ret.emplace_back(s);
            continue;
        }

        int64_t p1 = 1;
        int64_t p2 = s.length()+1;

        if (!pos1.isDefault())
            p1 = std::max(p1, std::min((int64_t)s.length(), pos1View.get(i).getNum().asI64()));

        if (!pos2.isDefault())
            p2 = std::max(p1, std::min((int64_t)s.length(), pos2View.get(i).getNum().asI64()));

        // Using the slower version to enable replacement of null characters
        replaceAll(s, f, r, p1-1, p2-1);

        ret.emplace_back(s);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the strip() function.
///
/// \param sStr const mu::Array&
/// \param frnt const mu::Array&
/// \param bck const mu::Array&
/// \param opts const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_strip(const mu::Array& sStr, const mu::Array& frnt, const mu::Array& bck, const mu::Array& opts)
{
    mu::MatrixView strView(sStr);
    mu::MatrixView frntView(frnt);
    mu::MatrixView bckView(bck);
    mu::MatrixView optsView(opts);
    mu::Array ret = strView.prepare(frntView, bckView, optsView);

    size_t elems = strView.size();

    for (size_t i = 0; i < elems; i++)
    {
        StringView s = strView.get(i).getStr();
        StringView f = frntView.get(i).getStr();
        StringView b = bckView.get(i).getStr();

        int64_t stripAll = 0;

        if (!opts.isDefault())
            stripAll = optsView.get(i).getNum().asI64();

        if (!s.length())
        {
            ret.emplace_back("");
            continue;
        }

        while (f.length()
               && s.length() >= f.length()
               && s.subview(0, f.length()) == f)
        {
            s.trim_front(f.length());

            if (!stripAll)
                break;
        }

        while (b.length()
               && s.length() >= b.length()
               && s.subview(s.length() - b.length()) == b)
        {
            s.trim_back(b.length());

            if (!stripAll)
                break;
        }

        ret.emplace_back(s.to_string());

    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the regex() function.
///
/// \param rgx const mu::Array&
/// \param sStr const mu::Array&
/// \param pos const mu::Array&
/// \param len const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_regex(const mu::Array& rgx, const mu::Array& sStr, const mu::Array& pos, const mu::Array& len)
{
    mu::MatrixView rgxView(rgx);
    mu::MatrixView strView(sStr);
    mu::MatrixView posView(pos);
    mu::MatrixView lenView(len);
    mu::Array ret = rgxView.prepare(strView, posView, lenView);

    size_t elems = rgxView.size();

#ifndef PARSERSTANDALONE
    for (size_t i = 0; i < elems; i++)
    {
        StringView r = rgxView.get(i).getStr();
        StringView s = strView.get(i).getStr();

        int64_t p = 1;
        int64_t l = s.length();
        mu::Array current;
        current.reserve(2);

        if (!r.length())
        {
            current.emplace_back(0);
            current.emplace_back(0);
            ret.emplace_back(current);
            continue;
        }

        if (!pos.isDefault())
            p = std::max(p, std::min(l, posView.get(i).getNum().asI64()));

        if (!len.isDefault())
            p = std::min(l, lenView.get(i).getNum().asI64());

        try
        {
            std::smatch match;
            std::regex expr(r.to_string());
            std::string sSubView = s.subview(p-1, l).to_string();

            if (std::regex_search(sSubView, match, expr))
            {
                current.emplace_back(match.position(0) + (size_t)p);
                current.emplace_back(match.length(0));
            }
            else
            {
                current.emplace_back(0);
                current.emplace_back(0);
            }

            ret.emplace_back(current);
        }
        catch (std::regex_error& e)
        {
            std::string message;

            switch (e.code())
            {
                case std::regex_constants::error_collate:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_COLLATE");
                    break;
                case std::regex_constants::error_ctype:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_CTYPE");
                    break;
                case std::regex_constants::error_escape:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_ESCAPE");
                    break;
                case std::regex_constants::error_backref:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_BACKREF");
                    break;
                case std::regex_constants::error_brack:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_BRACK");
                    break;
                case std::regex_constants::error_paren:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_PAREN");
                    break;
                case std::regex_constants::error_brace:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_BRACE");
                    break;
                case std::regex_constants::error_badbrace:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_BADBRACE");
                    break;
                case std::regex_constants::error_range:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_RANGE");
                    break;
                case std::regex_constants::error_space:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_SPACE");
                    break;
                case std::regex_constants::error_badrepeat:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_BADREPEAT");
                    break;
                case std::regex_constants::error_complexity:
                    message =_lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_COMPLEXITY");
                    break;
                case std::regex_constants::error_stack:
                    message = _lang.get("ERR_NR_"+toString(SyntaxError::INVALID_REGEX)+"_STACK");
                    break;
            }

            throw SyntaxError(SyntaxError::INVALID_REGEX, r.to_string(), SyntaxError::invalid_position, message);
        }
    }
#endif // PARSERSTANDALONE
    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the basetodec()
/// function.
///
/// \param base const mu::Array&
/// \param val const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_basetodec(const mu::Array& base, const mu::Array& val)
{
    mu::MatrixView baseView(base);
    mu::MatrixView valView(val);
    mu::Array ret = baseView.prepare(valView);

    size_t elems = baseView.size();

    for (size_t i = 0; i < elems; i++)
    {
        StringView b = baseView.get(i).getStr();
        StringView v = valView.get(i).getStr();

        if (b == "hex")
            ret.emplace_back(convertBaseToDecimal(v, HEX));
        else if (b == "oct")
            ret.emplace_back(convertBaseToDecimal(v, OCT));
        else if (b == "bin")
            ret.emplace_back(convertBaseToDecimal(v, BIN));
        else
            ret.emplace_back(v.to_string());
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the dectobase()
/// function.
///
/// \param base const mu::Array&
/// \param val const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_dectobase(const mu::Array& base, const mu::Array& val)
{
    mu::MatrixView baseView(base);
    mu::MatrixView valView(val);
    mu::Array ret = baseView.prepare(valView);

    size_t elems = baseView.size();

    for (size_t i = 0; i < elems; i++)
    {
        StringView b = baseView.get(i).getStr();
        int64_t v = valView.get(i).getNum().asI64();
        std::stringstream stream;

        if (b == "hex")
            stream.setf(std::ios::hex, std::ios::basefield);
        else if (b == "oct")
            stream.setf(std::ios::oct, std::ios::basefield);
        else if (b == "bin")
        {
            int i = 0;
            std::string bin;

            while ((1 << i) <= v)
            {
                if (v & (1 << i))
                    bin.insert(0, "1");
                else
                    bin.insert(0, "0");

                i++;
            }

            if (!bin.length())
                bin = "0";

            ret.emplace_back(bin);
            continue;
        }

        stream.setf(std::ios::showbase);
        stream << v;
        std::string conv;
        stream >> conv;

        if (conv == "0" && b == "hex")
            ret.emplace_back("0x0");
        else
            ret.emplace_back(conv);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the justify()
/// function.
///
/// \param arr const mu::Array&
/// \param align const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_justify(const mu::Array& arr, const mu::Array& align)
{
    mu::Array ret;
    ret.reserve(std::max(arr.size(), align.size()));

    for (size_t j = 0; j < std::max((size_t)1, align.size()); j++)
    {
        int64_t a = -1;

        if (!align.isDefault())
            a = align.get(j).getNum().asI64();

        // Find the string of max length
        size_t maxLength = 0;

        // Examine the whole string array
        for (size_t i = 0; i < arr.size(); i++)
        {
            // Remove the masked strings
            StringView sStr = arr.get(i).getStr();

            // Remove surrounding whitespaces
            sStr.strip();

            if (sStr.length() > maxLength)
                maxLength = sStr.length();
        }

        // Fill all string with as many whitespaces as necessary
        for (size_t i = 0; i < arr.size(); i++)
        {
            StringView view = arr.get(i).getStr();
            view.strip();

            std::string sStr = view.to_string();

            if (a == 1)
                sStr.insert(0, maxLength - sStr.size(), ' ');
            else if (a == -1)
                sStr.append(maxLength - sStr.size(), ' ');
            else if (a == 0)
            {
                size_t leftSpace = (maxLength - sStr.size()) / 2;
                size_t rightSpace = maxLength - leftSpace - sStr.size();
                sStr.insert(0, leftSpace, ' ');
                sStr.append(rightSpace, ' ');
            }

            // Append the string with the justified result
            ret.emplace_back(sStr);
        }
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the getlasterror()
/// function.
///
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_getlasterror()
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    ret.emplace_back(errorTypeToString(getLastErrorType()));
    ret.emplace_back(getLastErrorMessage());
#endif // PARSERSTANDALONE
    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the geterrormsg()
/// function.
///
/// \param errCode const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_geterrormessage(const mu::Array& errCode)
{
    mu::Array msg;
    msg.reserve(errCode.size());

    for (size_t i = 0; i < errCode.size(); i++)
    {
        const std::string& errC = errCode.get(i).getStr();

        if (errC.starts_with("expression#"))
        {
            std::string errM = _lang.get("ERR_MUP_" + errC.substr(errC.find('#')+1) + "_*");

            if (errM.starts_with("ERR_MUP_"))
                msg.emplace_back(errC);
            else
                msg.emplace_back(errM);
        }
        else if (errC.starts_with("syntax#"))
        {
            std::string errM = _lang.get("ERR_NR_" + errC.substr(errC.find('#')+1) + "_0_*", "$TOK$", "$ID1$", "$ID2$", "$ID3$", "$ID4$");

            if (errM.starts_with("ERR_NR_"))
                msg.emplace_back(errC);
            else
                msg.emplace_back(errM);
        }
        else
            msg.emplace_back(errC);
    }

    return msg;
}


/////////////////////////////////////////////////
/// \brief Implementation of the getversioninfo()
/// function.
///
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_getversioninfo()
{
    mu::Array sVersionInfo;
#ifndef PARSERSTANDALONE
    sVersionInfo.emplace_back("Version");
    sVersionInfo.emplace_back(getVersion());
    sVersionInfo.emplace_back("BuildDate");
    sVersionInfo.emplace_back(getBuildDate());
    sVersionInfo.emplace_back("FullVersion");
    sVersionInfo.emplace_back(getFullVersionWithArchitecture());
    sVersionInfo.emplace_back("FileVersion");
    sVersionInfo.emplace_back(getFileVersion());
    sVersionInfo.emplace_back("Architecture");
#ifdef __GNUWIN64__
    sVersionInfo.emplace_back("64 bit");
#else
    sVersionInfo.emplace_back("32 bit");
#endif
#endif // PARSERSTANDALONE
    return sVersionInfo;
}


/////////////////////////////////////////////////
/// \brief Implementation of the getuilang()
/// function.
///
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_getuilang()
{
    return mu::Value(_lang.get("LANGUAGE"));
}


/////////////////////////////////////////////////
/// \brief Implementation of the getuserinfo()
/// function.
///
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_getuserinfo()
{
    mu::Array userinfo;
#ifndef PARSERSTANDALONE
    userinfo.push_back("GivenName");
    userinfo.push_back(getUserDisplayName(true));
    userinfo.push_back("FullName");
    userinfo.push_back(getUserDisplayName(false));
    userinfo.push_back("UserId");
    userinfo.push_back(wxGetUserId().ToStdString());
    userinfo.push_back("UserProfile");
    userinfo.push_back(replacePathSeparator(getenv("USERPROFILE")));
#endif
    return userinfo;
}


/////////////////////////////////////////////////
/// \brief Implementation of the uuid() function.
///
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_getuuid()
{
    return mu::Value(getUuidV4());
}


/////////////////////////////////////////////////
/// \brief Implementation of the getodbcdrivers()
/// function.
///
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_getodbcdrivers()
{
#ifndef PARSERSTANDALONE
    return mu::Array(getOdbcDrivers());
#else
    return mu::Array();
#endif
}


/////////////////////////////////////////////////
/// \brief Implementation of the getfileinfo()
/// function.
///
/// \param file const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_getfileinfo(const mu::Array& file)
{
    mu::Array sFileInfo;
#ifndef PARSERSTANDALONE
    sFileInfo.copyDims(file);

    for (size_t i = 0; i < file.size(); i++)
    {
        FileInfo fInfo = NumeReKernel::getInstance()->getFileSystem().getFileInfo(file.get(i).getPath());
        mu::Array currentFile;
        currentFile.reserve(16); // 8 key-value pairs

        currentFile.emplace_back("Drive");
        currentFile.emplace_back(fInfo.drive);
        currentFile.emplace_back("Path");
        currentFile.emplace_back(fInfo.path);
        currentFile.emplace_back("Name");
        currentFile.emplace_back(fInfo.name);
        currentFile.emplace_back("FileExt");
        currentFile.emplace_back(fInfo.ext);
        currentFile.emplace_back("Size");
        currentFile.emplace_back(fInfo.filesize);

        std::string sAttr = fInfo.fileAttributes & FileInfo::ATTR_READONLY ? "readonly," : "";
        sAttr += fInfo.fileAttributes & FileInfo::ATTR_HIDDEN ? "hidden," : "";
        sAttr += fInfo.fileAttributes & FileInfo::ATTR_SYSTEM ? "systemfile," : "";
        sAttr += fInfo.fileAttributes & FileInfo::ATTR_DIRECTORY ? "directory," : "";
        //sAttr += fInfo.fileAttributes & FileInfo::ATTR_ARCHIVE ? "archive," : "";
        //sAttr += fInfo.fileAttributes & FileInfo::ATTR_DEVICE ? "device," : "";
        sAttr += fInfo.fileAttributes & FileInfo::ATTR_TEMPORARY ? "temp," : "";
        sAttr += fInfo.fileAttributes & FileInfo::ATTR_COMPRESSED ? "compressed," : "";
        sAttr += fInfo.fileAttributes & FileInfo::ATTR_OFFLINE ? "offline," : "";
        sAttr += fInfo.fileAttributes & FileInfo::ATTR_ENCRYPTED ? "encrypted," : "";

        if (sAttr.length())
            sAttr.pop_back();
        else
            sAttr = "none";

        currentFile.emplace_back("Attributes");
        currentFile.emplace_back(sAttr);
        currentFile.emplace_back("CreationTime");
        currentFile.emplace_back(fInfo.creationTime);
        currentFile.emplace_back("ModificationTime");
        currentFile.emplace_back(fInfo.modificationTime);

        sFileInfo.emplace_back(currentFile);
    }
#endif // PARSERSTANDALONE
    return sFileInfo;
}


/////////////////////////////////////////////////
/// \brief Implementation of the sha256()
/// function.
///
/// \param sStr const mu::Array&
/// \param opts const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_sha256(const mu::Array& sStr, const mu::Array& opts)
{
    mu::MatrixView strView(sStr);
    mu::MatrixView optsView(opts);
    mu::Array ret = strView.prepare(optsView);

    size_t elems = strView.size();

    for (size_t i = 0; i < elems; i++)
    {
        if (opts.isDefault() || optsView.get(i).getNum().asI64() == 0)
            ret.emplace_back(sha256(strView.get(i).getStr()));
        else
        {
#ifndef PARSERSTANDALONE
            std::string sFileName = NumeReKernel::getInstance()->getFileSystem().ValidFileName(strView.get(i).getPath(),
                                                                                               ".dat", false, true);

            // Ensure that the file actually exist
            if (fileExists(sFileName))
            {
                std::fstream file(sFileName, std::ios_base::in | std::ios_base::binary);
                ret.emplace_back(sha256(file));
            }
            else
                ret.emplace_back(mu::Value());
#endif // PARSERSTANDALONE
        }
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the encode_base_n()
/// function.
///
/// \param sStr const mu::Array&
/// \param isFile const mu::Array&
/// \param n const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_encode_base_n(const mu::Array& sStr, const mu::Array& isFile, const mu::Array& n)
{
    mu::MatrixView strView(sStr);
    mu::MatrixView isFileView(isFile);
    mu::MatrixView nView(n);
    mu::Array ret = strView.prepare(isFileView, nView);

    size_t elems = strView.size();

    for (size_t i = 0; i < elems; i++)
    {
        bool useFile = false;
        int encoding = 64;

        if (!isFile.isDefault())
            useFile = isFileView.get(i).getNum().asI64() != 0;

        if (!n.isDefault())
            encoding = nView.get(i).getNum().asI64();

        if (!useFile)
            ret.emplace_back(encode_base_n(strView.get(i).getStr(), false, encoding));
        else
        {
#ifndef PARSERSTANDALONE
            std::string sFileName = NumeReKernel::getInstance()->getFileSystem().ValidFileName(strView.get(i).getPath(),
                                                                                               ".dat", false, true);

            // Ensure that the file actually exist
            if (fileExists(sFileName))
                ret.emplace_back(encode_base_n(sFileName, true, encoding));
            else
                ret.emplace_back(mu::Value());
#endif // PARSERSTANDALONE
        }
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the decode_base_n()
/// function.
///
/// \param sStr const mu::Array&
/// \param n const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_decode_base_n(const mu::Array& sStr, const mu::Array& n)
{
    mu::MatrixView strView(sStr);
    mu::MatrixView nView(n);
    mu::Array ret = strView.prepare(nView);

    size_t elems = strView.size();

    for (size_t i = 0; i < elems; i++)
    {
        int encoding = 64;

        if (!n.isDefault())
            encoding = nView.get(i).getNum().asI64();

        ret.emplace_back(decode_base_n(strView.get(i).getStr(), encoding));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the startswith()
/// function.
///
/// \param sStr const mu::Array&
/// \param with const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_startswith(const mu::Array& sStr, const mu::Array& with)
{
    mu::MatrixView strView(sStr);
    mu::MatrixView withView(with);
    mu::Array ret = strView.prepare(withView);

    size_t elems = strView.size();

    for (size_t i = 0; i < elems; i++)
    {
        if (!withView.get(i).getStr().length())
            ret.emplace_back(false);
        else
            ret.emplace_back(strView.get(i).getStr().starts_with(withView.get(i).getStr()));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the endswith()
/// function.
///
/// \param sStr const mu::Array&
/// \param with const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_endswith(const mu::Array& sStr, const mu::Array& with)
{
    mu::MatrixView strView(sStr);
    mu::MatrixView withView(with);
    mu::Array ret = strView.prepare(withView);

    size_t elems = strView.size();

    for (size_t i = 0; i < elems; i++)
    {
        if (!withView.get(i).getStr().length())
            ret.emplace_back(false);
        else
            ret.emplace_back(strView.get(i).getStr().ends_with(withView.get(i).getStr()));
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_value()
/// function.
///
/// \param sStr const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_to_value(const mu::Array& sStr)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    ret.copyDims(sStr);

    mu::Parser p = NumeReKernel::getInstance()->getParser(); // Get a copy

    for (size_t i = 0; i < sStr.size(); i++)
    {
        if (sStr.get(i).isString())
        {
            p.SetExpr(sStr.get(i).getStr());
            int res;
            const mu::StackItem* vals = p.Eval(res);
            mu::Array results;

            for (int n = 0; n < res; n++)
            {
                results.insert(results.end(), vals[n].get().begin(), vals[n].get().end());
            }

            ret.emplace_back(results);
        }
        else
            ret.emplace_back(sStr.get(i));
    }
#endif // PARSERSTANDALONE
    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the to_string()
/// function.
///
/// \param vals const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_to_string(const mu::Array& vals)
{
    mu::Array ret;
    ret.copyDims(vals);

    for (const auto& v : vals)
    {
        ret.emplace_back(v.printVal());
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the getindices()
/// function.
///
/// \param tab const mu::Array&
/// \param opts const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_getindices(const mu::Array& tab, const mu::Array& opts)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    ret.reserve(std::max(tab.size(), opts.size()));

    for (size_t i = 0; i < std::max(tab.size(), opts.size()); i++)
    {
        int64_t nType = 0;

        if (!opts.isDefault())
            nType = opts.get(i).getNum().asI64();

        // Because the object might be a constructed table, we
        // disable the access caching for this expression
        MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();
        DataAccessParser _accessParser(tab.get(i).getStr(), false, true);

        if (!_accessParser.getDataObject().length() || !isValidIndexSet(_accessParser.getIndices()))
        {
            ret.emplace_back(NAN);
            continue;
        }

        if (nType > -1)
        {
            if (nType == 2 && _accessParser.getIndices().row.isOpenEnd())
                _accessParser.getIndices().row.setRange(_accessParser.getIndices().row.front(),
                                                        _accessParser.getIndices().row.front() + 1);
            else if (nType == 1 && _accessParser.getIndices().col.isOpenEnd())
                _accessParser.getIndices().col.setRange(_accessParser.getIndices().col.front(),
                                                        _accessParser.getIndices().col.front() + 1);

            if (_accessParser.isCluster())
            {
                if (_accessParser.getIndices().row.isOpenEnd())
                    _accessParser.getIndices().row.setRange(0, _data.getCluster(_accessParser.getDataObject()).size()-1);

                if (_accessParser.getIndices().col.isOpenEnd())
                    _accessParser.getIndices().col.back() = VectorIndex::INVALID;
            }
            else
            {
                if (_accessParser.getIndices().row.isOpenEnd())
                    _accessParser.getIndices().row.setRange(0, _data.getLines(_accessParser.getDataObject(), false)-1);

                if (_accessParser.getIndices().col.isOpenEnd())
                    _accessParser.getIndices().col.setRange(0, _data.getCols(_accessParser.getDataObject(), false)-1);
            }
        }

        _accessParser.getIndices().row.linearize();
        _accessParser.getIndices().col.linearize();

        mu::Array current;
        current.reserve(4);

        current.emplace_back(_accessParser.getIndices().row.front() + 1);
        current.emplace_back(_accessParser.getIndices().row.last() + 1);
        current.emplace_back(_accessParser.getIndices().col.front() + 1);
        current.emplace_back(_accessParser.getIndices().col.last() + 1);
        ret.emplace_back(current);
    }
#endif // PARSERSTANDALONE
    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_data()
/// function.
///
/// \param sStr const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_is_data(const mu::Array& sStr)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    ret.copyDims(sStr);

    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    for (size_t i = 0; i < sStr.size(); i++)
    {
        ret.emplace_back(_data.isTable(sStr.get(i).getStr()) || _data.isCluster(sStr.get(i).getStr()));
    }
#endif // PARSERSTANDALONE
    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_table()
/// function.
///
/// \param sStr const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_is_table(const mu::Array& sStr)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    ret.copyDims(sStr);
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    for (size_t i = 0; i < sStr.size(); i++)
    {
        ret.emplace_back(_data.isTable(sStr.get(i).getStr()));
    }
#endif // PARSERSTANDALONE
    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the is_cluster()
/// function.
///
/// \param sStr const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_is_cluster(const mu::Array& sStr)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    ret.copyDims(sStr);
    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    for (size_t i = 0; i < sStr.size(); i++)
    {
        ret.emplace_back(_data.isCluster(sStr.get(i).getStr()));
    }
#endif // PARSERSTANDALONE
    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the findcolumn()
/// function.
///
/// \param tab const mu::Array&
/// \param col const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_findcolumn(const mu::Array& tab, const mu::Array& col)
{
    mu::Array ret;
#ifndef PARSERSTANDALONE
    ret.reserve(std::max(tab.size(), col.size()));

    MemoryManager& _data = NumeReKernel::getInstance()->getMemoryManager();

    for (size_t i = 0; i < std::max(tab.size(), col.size()); i++)
    {
        if (!_data.isTable(tab.get(i).getStr()))
        {
            ret.emplace_back(NAN);
            continue;
        }

        std::vector<size_t> cols = _data.findCols(tab.get(i).getStr().substr(0, tab.get(i).getStr().find('(')),
                                                  {col.get(i).getStr()}, false, false);

        if (cols.size())
            ret.emplace_back(cols);
        else
            ret.emplace_back(NAN);
    }
#endif // PARSERSTANDALONE
    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the valtostr()
/// function.
///
/// \param vals const mu::Array&
/// \param cfill const mu::Array&
/// \param len const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_valtostr(const mu::Array& vals, const mu::Array& cfill, const mu::Array& len)
{
    mu::MatrixView valsView(vals);
    mu::MatrixView fillView(cfill);
    mu::MatrixView lenView(len);
    mu::Array ret = valsView.prepare(fillView, lenView);

    size_t elems = valsView.size();

    for (size_t i = 0; i < elems; i++)
    {
#ifndef PARSERSTANDALONE
        std::string v = valsView.get(i).printVal(NumeReKernel::getInstance()->getSettings().getPrecision());
#else
        std::string v = valsView.get(i).printVal();
#endif // PARSERSTANDALONE

        if (!len.isDefault()
            && !cfill.isDefault()
            && fillView.get(i).getStr().length()
            && (int64_t)v.length() < lenView.get(i).getNum().asI64())
        {
            int64_t l = lenView.get(i).getNum().asI64();
            const std::string& sChar = fillView.get(i).getStr();

            while ((int64_t)v.length() < l)
            {
                v.insert(0, 1, sChar.front());
            }
        }

        ret.emplace_back(v);
    }

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implementation of the gettypeof()
/// function.
///
/// \param vals const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_gettypeof(const mu::Array& vals)
{
    return mu::Value(vals.getCommonTypeAsString());
}


/////////////////////////////////////////////////
/// \brief Implements the readxml() function.
///
/// \param xmlFiles const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_readxml(const mu::Array& xmlFiles)
{
    mu::Array ret;
    ret.copyDims(xmlFiles);
#ifndef PARSERSTANDALONE
    FileSystem& _fSys = NumeReKernel::getInstance()->getFileSystem();

    for (const mu::Value& file : xmlFiles)
    {
        std::string validFile = _fSys.ValidFileName(file.getPath(), ".xml", false, true);

        if (!fileExists(validFile))
            throw SyntaxError(SyntaxError::FILE_NOT_EXIST, "readxml(\"" + validFile + "\")", validFile);

        ret.emplace_back(mu::DictStruct());
        ret.back().getDictStruct().importXml(validFile);
    }
#endif

    return ret;
}


/////////////////////////////////////////////////
/// \brief Implements the readjson() function.
///
/// \param jsonFiles const mu::Array&
/// \return mu::Array
///
/////////////////////////////////////////////////
mu::Array strfnc_readjson(const mu::Array& jsonFiles)
{
    mu::Array ret;
    ret.copyDims(jsonFiles);
#ifndef PARSERSTANDALONE
    FileSystem& _fSys = NumeReKernel::getInstance()->getFileSystem();

    for (const mu::Value& file : jsonFiles)
    {
        std::string validFile = _fSys.ValidFileName(file.getPath(), ".json", false, true);

        if (!fileExists(validFile))
            throw SyntaxError(SyntaxError::FILE_NOT_EXIST, "readjson(\"" + validFile + "\")", validFile);

        ret.emplace_back(mu::DictStruct());
        ret.back().getDictStruct().importJson(validFile);
    }
#endif

    return ret;
}












