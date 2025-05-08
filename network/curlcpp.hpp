/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2025  Erik Haenel et al.

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


#ifndef CURLCPP_HPP
#define CURLCPP_HPP

#include <curl/curl.h>

/////////////////////////////////////////////////
/// \brief Helper class to wrap cURL into C++
/// RAII.
/////////////////////////////////////////////////
class CurlCpp
{
    private:
        CURL* m_curlInstance;
        curl_slist* m_httpHeader;
        curl_slist* m_recipients;

        /////////////////////////////////////////////////
        /// \brief Reset this instance.
        ///
        /// \return void
        ///
        /////////////////////////////////////////////////
        void reset()
        {
            if (m_curlInstance)
                curl_easy_cleanup(m_curlInstance);

            if (m_httpHeader)
                curl_slist_free_all(m_httpHeader);

            if (m_recipients)
                curl_slist_free_all(m_recipients);
        }

    public:
        /////////////////////////////////////////////////
        /// \brief Construct an instance and initialize
        /// it, if desired.
        ///
        /// \param autoInit bool
        ///
        /////////////////////////////////////////////////
        CurlCpp(bool autoInit = false) : m_curlInstance(nullptr), m_httpHeader(nullptr), m_recipients(nullptr)
        {
            if (autoInit)
                init();
        }

        /////////////////////////////////////////////////
        /// \brief Move constructor.
        ///
        /// \param curl CurlCpp&&
        ///
        /////////////////////////////////////////////////
        CurlCpp(CurlCpp&& curl)
        {
            m_curlInstance = curl.m_curlInstance;
            m_httpHeader = curl.m_httpHeader;
            m_recipients = curl.m_recipients;

            curl.m_curlInstance = nullptr;
            curl.m_httpHeader = nullptr;
            curl.m_recipients = nullptr;
        }

        // No copy constructor
        CurlCpp(const CurlCpp&) = delete;

        /////////////////////////////////////////////////
        /// \brief Destructor. Frees all associated
        /// memory.
        ///
        ///
        /////////////////////////////////////////////////
        ~CurlCpp()
        {
            reset();
        }

        /////////////////////////////////////////////////
        /// \brief Move assignment operator.
        ///
        /// \param curl CurlCpp&&
        /// \return CurlCpp&
        ///
        /////////////////////////////////////////////////
        CurlCpp& operator=(CurlCpp&& curl)
        {
            reset();

            m_curlInstance = curl.m_curlInstance;
            m_httpHeader = curl.m_httpHeader;
            m_recipients = curl.m_recipients;

            curl.m_curlInstance = nullptr;
            curl.m_httpHeader = nullptr;
            curl.m_recipients = nullptr;

            return *this;
        }

        // No copy assignment
        CurlCpp& operator=(const CurlCpp&) = delete;

        /////////////////////////////////////////////////
        /// \brief Initialize this instance.
        ///
        /// \return void
        ///
        /////////////////////////////////////////////////
        void init()
        {
            curl_global_init(CURL_GLOBAL_DEFAULT);

            if (!m_curlInstance)
                m_curlInstance = curl_easy_init();
        }

        /////////////////////////////////////////////////
        /// \brief Set a cURL option together with a
        /// value.
        ///
        /// \param option CURLoption
        /// \param val T
        /// \return bool
        ///
        /////////////////////////////////////////////////
        template <typename T>
        bool setOption(CURLoption option, T val)
        {
            if (!m_curlInstance)
                return false;

            return curl_easy_setopt(m_curlInstance, option, val) == CURLE_OK;
        }

        /////////////////////////////////////////////////
        /// \brief Set a HTTP header.
        ///
        /// \param httpHeader const std::vector<std::string>&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool setHeader(const std::vector<std::string>& httpHeader)
        {
            for (const auto& headerString : httpHeader)
            {
                m_httpHeader = curl_slist_append(m_httpHeader, headerString.c_str());
            }

            if (!m_curlInstance)
                m_curlInstance = curl_easy_init();

            return setOption(CURLOPT_HTTPHEADER, m_httpHeader);
        }

        /////////////////////////////////////////////////
        /// \brief Set the list of mail recipients.
        ///
        /// \param recipients const std::vector<std::string>&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool setRecipients(const std::vector<std::string>& recipients)
        {
            for (const auto& recipient : recipients)
            {
                m_recipients = curl_slist_append(m_recipients, recipient.c_str());
            }

            if (!m_curlInstance)
                m_curlInstance = curl_easy_init();

            return setOption(CURLOPT_MAIL_RCPT, m_recipients);
        }

        /////////////////////////////////////////////////
        /// \brief Perform the transmission.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool perform()
        {
            if (!m_curlInstance)
                return false;

            return curl_easy_perform(m_curlInstance) == CURLE_OK;
        }

        /////////////////////////////////////////////////
        /// \brief Returns true, if the instance is valid.
        ///
        /// \return operator
        ///
        /////////////////////////////////////////////////
        operator bool() const
        {
            return m_curlInstance != nullptr;
        }
};


#endif // CURLCPP_HPP


