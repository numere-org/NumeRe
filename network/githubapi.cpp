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

            for (const auto& label : labels)
                json += "\"" + strToJson(label) + "\"";

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
    /// \param sRepoUrl const std::string&
    /// \param sUserAuth const std::string&
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string create(const Issue& issue, const std::string& sRepoUrl, const std::string& sUserAuth)
    {
        // Set Up CURL
        CURL *curl = curl_easy_init();

        if (curl)
        {
            std::string responseBuffer;
            std::string jsonString = issue.jsonUrlEncode();
            static std::string userAgent = std::string("libcurl/") + curl_version();
            std::string sAuthorization = "Authorization: Bearer " + sUserAuth;
            curl_slist* httpHeader = nullptr;

            httpHeader = curl_slist_append(httpHeader, "Accept: application/vnd.github+json");
            httpHeader = curl_slist_append(httpHeader, sAuthorization.c_str());
            httpHeader = curl_slist_append(httpHeader, "X-GitHub-Api-Version: 2022-11-28");

            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, httpHeader);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, userAgent.c_str());
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_URL, sRepoUrl.c_str());
            curl_easy_setopt(curl, CURLOPT_POST, 1L);
            curl_easy_setopt(curl, CURLOPT_COPYPOSTFIELDS, jsonString.c_str());

            // Set up callback and buffer for response
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writer);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBuffer);

            // Perform Transmission
            curl_easy_perform(curl);

            curl_easy_cleanup(curl);
            curl_slist_free_all(httpHeader);

            size_t urlPos = responseBuffer.find("\"html_url\": \"https://github.com/");

            if (urlPos != std::string::npos)
            {
                urlPos += 13;
                return responseBuffer.substr(urlPos, responseBuffer.find('"', urlPos) - urlPos);
            }
        }

        return "";
    }
}


