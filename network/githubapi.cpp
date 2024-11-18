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

#include "githubapi.hpp"
#include "../kernel/core/utils/tools.hpp"
#include "../kernel/core/io/logger.hpp"
#include <curl/curl.h>

/////////////////////////////////////////////////
/// \brief Tiny helper to replace the characters
/// for the curl transmission.
///
/// \param sPlain std::string
/// \return std::string
///
/////////////////////////////////////////////////
static std::string strToJson(std::string sPlain)
{
    replaceAll(sPlain, "\\", "\\\\");
    replaceAll(sPlain, "\"", "\\\"");
    replaceAll(sPlain, "\n", "\\n");

    return sPlain;
}


/////////////////////////////////////////////////
/// \brief Helper class to wrap cURL into C++
/// RAII.
/////////////////////////////////////////////////
class CurlCpp
{
    private:
        CURL* m_curlInstance;
        curl_slist* m_httpHeader;

    public:
        /////////////////////////////////////////////////
        /// \brief Construct an instance and initialize
        /// it, if desired.
        ///
        /// \param autoInit bool
        ///
        /////////////////////////////////////////////////
        CurlCpp(bool autoInit = false) : m_curlInstance(nullptr), m_httpHeader(nullptr)
        {
            if (autoInit)
                init();
        }

        /////////////////////////////////////////////////
        /// \brief Destructor. Frees all associated
        /// memory.
        ///
        ///
        /////////////////////////////////////////////////
        ~CurlCpp()
        {
            if (m_curlInstance)
                curl_easy_cleanup(m_curlInstance);

            if (m_httpHeader)
                curl_slist_free_all(m_httpHeader);
        }

        /////////////////////////////////////////////////
        /// \brief Initialize this instance.
        ///
        /// \return void
        ///
        /////////////////////////////////////////////////
        void init()
        {
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


namespace GitHub
{
    /////////////////////////////////////////////////
    /// \brief Encode the contents of this issue as
    /// curl-ready JSON.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string Issue::jsonUrlEncode() const
    {
        std::string json = "{\"title\": \"" + strToJson(title) + "\", \"body\": \"" + strToJson(body) + "\"";

        if (assignees.size())
        {
            json += ", \"assignees\": [";

            for (const auto& assignee : assignees)
                json += "\"" + strToJson(assignee) + "\"";

            json += "]";
        }

        if (labels.size())
        {
            json += ", \"labels\": [";

            for (size_t i = 0; i < labels.size(); i++)
            {
                if (i)
                    json += ", ";

                json += "\"" + strToJson(labels[i]) + "\"";
            }

            json += "]";
        }

        return json + "}";
    }

    /////////////////////////////////////////////////
    /// \brief libcurl write callback function.
    ///
    /// \param data char*
    /// \param size size_t
    /// \param nmemb size_t
    /// \param writerData std::string*
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    static size_t writer(char* data, size_t size, size_t nmemb, std::string* writerData)
    {
        if (writerData == NULL)
            return 0;

        writerData->append(data, size * nmemb);

        return size * nmemb;
    }


    /////////////////////////////////////////////////
    /// \brief Create a issue with the passed
    /// contents in the target repository using the
    /// passed authorization.
    ///
    /// \param issue const Issue&
    /// \param sApiUrl const std::string&
    /// \param sUserAuth const std::string&
    /// \param sLogPath const std::string&
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string create(const Issue& issue, const std::string& sApiUrl, const std::string& sUserAuth, const std::string& sLogPath)
    {
        // Set Up CURL
        CurlCpp curl(true);
        Logger logger(sLogPath+"/github.log");

        if (curl)
        {
            std::string responseBuffer;
            std::string jsonString = issue.jsonUrlEncode();
            std::string sAuthorization = "Authorization: Bearer " + sUserAuth;

            curl.setHeader({"Accept: application/vnd.github+json", sAuthorization, "X-GitHub-Api-Version: 2022-11-28"});

            if (!curl.setOption(CURLOPT_USERAGENT, curl_version()))
            {
                logger.push_line("ERROR: Internal cURL problem.");
                return "";
            }

            if (!curl.setOption(CURLOPT_SSL_VERIFYHOST, 0L))
            {
                logger.push_line("ERROR: Internal cURL problem.");
                return "";
            }

            if (!curl.setOption(CURLOPT_SSL_VERIFYPEER, 0L))
            {
                logger.push_line("ERROR: Internal cURL problem.");
                return "";
            }

            if (!curl.setOption(CURLOPT_FOLLOWLOCATION, 1L))
            {
                logger.push_line("ERROR: Internal cURL problem.");
                return "";
            }

            if (!curl.setOption(CURLOPT_URL, sApiUrl.c_str()))
            {
                logger.push_line("ERROR: Internal cURL problem.");
                return "";
            }

            if (!curl.setOption(CURLOPT_POST, 1L))
            {
                logger.push_line("ERROR: Internal cURL problem.");
                return "";
            }

            if (!curl.setOption(CURLOPT_COPYPOSTFIELDS, jsonString.c_str()))
            {
                logger.push_line("ERROR: Internal cURL problem.");
                return "";
            }

            // Set up callback and buffer for response
            if (!curl.setOption(CURLOPT_WRITEFUNCTION, writer))
            {
                logger.push_line("ERROR: Internal cURL problem.");
                return "";
            }

            if (!curl.setOption(CURLOPT_WRITEDATA, &responseBuffer))
            {
                logger.push_line("ERROR: Internal cURL problem.");
                return "";
            }

            // Perform Transmission
            if (!curl.perform())
            {
                logger.push_line("ERROR: Transmission error.");
                return "";
            }

            size_t urlPos = responseBuffer.find("\"html_url\": \"https://github.com/");

            if (urlPos != std::string::npos)
            {
                urlPos += 13;
                return responseBuffer.substr(urlPos, responseBuffer.find('"', urlPos) - urlPos);
            }

            logger.push_line("Error: GitHub API reported a problem.");
            logger.push_line("Payload:");
            logger.push_line(jsonString);
            logger.push_line("API response:");
            logger.push_line(responseBuffer);
        }
        else
            logger.push_line("ERROR: Internal cURL problem.");

        return "";
    }
}


