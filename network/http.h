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
#include <vector>

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

    struct UrlParams
    {
        std::string m_userName;
        std::string m_passWord;
        std::vector<std::string> m_httpHeader;
        std::string m_payLoad;
        std::string m_caCert;

        UrlParams(const std::string& sUserName = "", const std::string& sPassWord = "", const std::string& sCaCert = "",
                  const std::vector<std::string>& httpHeader = std::vector<std::string>(), const std::string& sPayLoad = "")
        {
            m_userName = sUserName;
            m_passWord = sPassWord;
            m_httpHeader = httpHeader;
            m_payLoad = sPayLoad;
            m_caCert = sCaCert;
        }
    };

    std::string get(const std::string& sUrl, const UrlParams& params);

    std::string put(const std::string& sUrl, const std::string& sFileName, const UrlParams& params);

    std::string post(const std::string& sUrl, const UrlParams& params);

    size_t sendMail(const std::string& sUrl, const std::string& sBody,
                    const std::string& sFromMail, const std::vector<std::string>& recipients,
                    const UrlParams& params);
}

#endif // HTTP_H

