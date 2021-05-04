
/***************************************************************************
 *                                  _   _ ____  _
 *  Project                     ___| | | |  _ \| |
 *                             / __| | | | |_) | |
 *                            | (__| |_| |  _ <| |___
 *                             \___|\___/|_| \_\_____|
 *
 * Copyright (C) 1998 - 2020, Daniel Stenberg, <daniel@haxx.se>, et al.
 *
 * This software is licensed as described in the file COPYING, which
 * you should have received as part of this distribution. The terms
 * are also available at https://curl.se/docs/copyright.html.
 *
 * You may opt to use, copy, modify, merge, publish, distribute and/or sell
 * copies of the Software, and permit persons to whom the Software is
 * furnished to do so, under the terms of the COPYING file.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#include "http.h"
#include <curl/curl.h>

namespace http
{
    /////////////////////////////////////////////////
    /// \brief libcurl variable for error strings.
    ///
    /////////////////////////////////////////////////
    static char errorBuffer[CURL_ERROR_SIZE];


    /////////////////////////////////////////////////
    /// \brief libcurl write callback function.
    ///
    /// \param data char*
    /// \param size size_t
    /// \param nmemb size_t
    /// \param writerData std::string*
    /// \return int
    ///
    /////////////////////////////////////////////////
    static int writer(char* data, size_t size, size_t nmemb, std::string* writerData)
    {
        if (writerData == NULL)
            return 0;

        writerData->append(data, size * nmemb);

        return size * nmemb;
    }


    /////////////////////////////////////////////////
    /// \brief libcurl connection initialization.
    ///
    /// \param conn CURL*&
    /// \param url const char*
    /// \param buffer std::string*
    /// \return bool
    /// \throw http::Error
    /////////////////////////////////////////////////
    static bool init(CURL*& conn, const char* url, std::string* buffer)
    {
        CURLcode code;

        conn = curl_easy_init();

        if (conn == NULL)
            throw Error("Failed to create CURL connection.");

        code = curl_easy_setopt(conn, CURLOPT_ERRORBUFFER, errorBuffer);

        if (code != CURLE_OK)
            throw Error("Failed to set error buffer [" + std::to_string(code) + "].");

        code = curl_easy_setopt(conn, CURLOPT_URL, url);

        if (code != CURLE_OK)
            throw Error("Failed to set URL [" + std::string(errorBuffer) + "].");

        code = curl_easy_setopt(conn, CURLOPT_FOLLOWLOCATION, 1L);

        if (code != CURLE_OK)
            throw Error("Failed to set redirect option [" + std::string(errorBuffer) + "].");

        code = curl_easy_setopt(conn, CURLOPT_CONNECTTIMEOUT, 5L);

        if (code != CURLE_OK)
            throw Error("Failed to set connection time-out option [" + std::string(errorBuffer) + "].");

        code = curl_easy_setopt(conn, CURLOPT_TIMEOUT, 10L);

        if (code != CURLE_OK)
            throw Error("Failed to set time-out option [" + std::string(errorBuffer) + "].");

        code = curl_easy_setopt(conn, CURLOPT_WRITEFUNCTION, writer);

        if (code != CURLE_OK)
            throw Error("Failed to set writer [" + std::string(errorBuffer) + "].");

        code = curl_easy_setopt(conn, CURLOPT_WRITEDATA, buffer);

        if (code != CURLE_OK)
            throw Error("Failed to set write data [" + std::string(errorBuffer) + "].");

        return true;
    }


    /////////////////////////////////////////////////
    /// \brief Get the contents of a URL.
    ///
    /// \param url const std::string&
    /// \return std::string
    /// \throw http::Error
    /////////////////////////////////////////////////
    std::string get(const std::string& url)
    {
        CURL* conn = NULL;
        CURLcode code;
        std::string buffer;
        buffer.clear();

        curl_global_init(CURL_GLOBAL_DEFAULT);

        // Initialize CURL connection
        if (!init(conn, url.c_str(), &buffer))
            throw Error("Connection initializion failed.");

        // Retrieve content for the URL
        code = curl_easy_perform(conn);
        curl_easy_cleanup(conn);

        if (code != CURLE_OK)
            throw Error("Failed to get '" + url + "': " + std::string(errorBuffer) + ".");

        // Return the buffer contents
        return buffer;
    }
}

