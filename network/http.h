/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2021  Erik Haenel et al.

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


#ifndef HTTP_H
#define HTTP_H

#include <string>
#include <exception>

namespace url
{
    /////////////////////////////////////////////////
    /// \brief A class for URL exceptions.
    /////////////////////////////////////////////////
    class Error : public std::exception
    {
        private:
            std::string m_what;

        public:
            Error(const std::string& what) : m_what(what) {}
            virtual const char* what() const noexcept
            {
                return m_what.c_str();
            }
    };

    std::string get(const std::string& sUrl, const std::string& sUserName = "", const std::string& sPassWord = "");
    size_t put(const std::string& sUrl, const std::string& sFileName, const std::string& sUserName = "", const std::string& sPassWord = "");
}

#endif // HTTP_H

