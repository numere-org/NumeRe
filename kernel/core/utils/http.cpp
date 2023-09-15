
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
#include "tools.hpp"
#include <curl/curl.h>
#include <fstream>

// Further possible features enabled in cURL:
// DICT FILE FTPS GOPHER GOPHERS IMAP IMAPS LDAP MQTT POP3 POP3S RTSP SCP SFTP SMB SMBS SMTP SMTPS TELNET TFTP

namespace url
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
    /// \brief libcurl read callback function.
    ///
    /// \param ptr char*
    /// \param size size_t
    /// \param nmemb size_t
    /// \param stream void*
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    static size_t reader(char *ptr, size_t size, size_t nmemb, void *stream)
    {
        std::ifstream* filestream = static_cast<std::ifstream*>(stream);
        size_t bytes = filestream->readsome(ptr, size * nmemb);
        return bytes;
    }

    /////////////////////////////////////////////////
    /// \brief Simple helper struct for the transfer
    /// progress bar.
    /////////////////////////////////////////////////
    struct ProgressData
    {
        curl_off_t lastVal = 0;
    };


    /////////////////////////////////////////////////
    /// \brief Created and handles the progress bar
    /// visualizing the download process. Note that
    /// for small transfer packages, the progress bar
    /// is not shown.
    ///
    /// \param clientp void*
    /// \param dltotal curl_off_t
    /// \param dlnow curl_off_t
    /// \param ultotal curl_off_t
    /// \param ulnow curl_off_t
    /// \return int
    ///
    /////////////////////////////////////////////////
    static int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
    {
        ProgressData* data = static_cast<ProgressData*>(clientp);

        if (dlnow > 0L && dltotal > 1000000 && data->lastVal < dlnow)
        {
            make_progressBar(dlnow, 0, dltotal, "bar");
            data->lastVal = dlnow;
        }

        if (ulnow > 0L && ultotal > 100000 && data->lastVal < ulnow)
        {
            make_progressBar(ulnow, 0, ultotal, "bar");
            data->lastVal = ulnow;
        }

        return 0;
    }


    /////////////////////////////////////////////////
    /// \brief Perform common CURL initialization.
    ///
    /// \param sUrl const std::string&
    /// \param sUserName const std::string&
    /// \param sPassWord const std::string&
    /// \return CURL*
    ///
    /////////////////////////////////////////////////
    static CURL* common_init(const std::string& sUrl, const std::string& sUserName, const std::string& sPassWord)
    {
        CURLcode code;

        CURL* conn = curl_easy_init();

        if (conn == nullptr)
            throw Error("Failed to create CURL connection.");

        try
        {
            code = curl_easy_setopt(conn, CURLOPT_ERRORBUFFER, errorBuffer);

            if (code != CURLE_OK)
                throw Error("Failed to set error buffer [" + std::to_string(code) + "].");

            code = curl_easy_setopt(conn, CURLOPT_URL, sUrl.c_str());

            if (code != CURLE_OK)
                throw Error("Failed to set URL [" + std::string(errorBuffer) + "].");

            code = curl_easy_setopt(conn, CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:89.0) Gecko/20100101 Firefox/89.0");

            if (code != CURLE_OK)
                throw Error("Failed to set user agent [" + std::string(errorBuffer) + "].");

            if (sUserName.length() && sPassWord.length())
            {
                code = curl_easy_setopt(conn, CURLOPT_USERPWD, (sUserName + ":" + sPassWord).c_str());

                if (code != CURLE_OK)
                    throw Error("Failed to set username and password [" + std::string(errorBuffer) + "].");
            }

            code = curl_easy_setopt(conn, CURLOPT_SSL_VERIFYPEER, 0L);

            if (code != CURLE_OK)
                throw Error("Failed to set SSL peer verify option [" + std::string(errorBuffer) + "].");

            code = curl_easy_setopt(conn, CURLOPT_SSL_VERIFYHOST, 0L);

            if (code != CURLE_OK)
                throw Error("Failed to set SSL host verify option [" + std::string(errorBuffer) + "].");

            code = curl_easy_setopt(conn, CURLOPT_FOLLOWLOCATION, 1L);

            if (code != CURLE_OK)
                throw Error("Failed to set redirect option [" + std::string(errorBuffer) + "].");

            code = curl_easy_setopt(conn, CURLOPT_CONNECTTIMEOUT, 10L);

            if (code != CURLE_OK)
                throw Error("Failed to set connection time-out option [" + std::string(errorBuffer) + "].");

            code = curl_easy_setopt(conn, CURLOPT_LOW_SPEED_LIMIT, 1024);

            if (code != CURLE_OK)
                throw Error("Failed to set low-speed option [" + std::string(errorBuffer) + "].");

            code = curl_easy_setopt(conn, CURLOPT_LOW_SPEED_TIME, 30L);

            if (code != CURLE_OK)
                throw Error("Failed to set low-speed time option [" + std::string(errorBuffer) + "].");

            code = curl_easy_setopt(conn, CURLOPT_NOPROGRESS, 0L);

            if (code != CURLE_OK)
                throw Error("Failed to set progress option [" + std::string(errorBuffer) + "].");

            code = curl_easy_setopt(conn, CURLOPT_XFERINFOFUNCTION, progress_callback);

            if (code != CURLE_OK)
                throw Error("Failed to set progress function [" + std::string(errorBuffer) + "].");

            ProgressData data;

            code = curl_easy_setopt(conn, CURLOPT_XFERINFODATA, &data);

            if (code != CURLE_OK)
                throw Error("Failed to set progress data [" + std::string(errorBuffer) + "].");
        }
        catch (...)
        {
            curl_easy_cleanup(conn);
            throw;
        }

        return conn;
    }


    /////////////////////////////////////////////////
    /// \brief libcurl get connection initialization.
    ///
    /// \param conn CURL*&
    /// \param sUrl const std::string&
    /// \param sUserName const std::string&
    /// \param sPassWord const std::string&
    /// \param buffer std::string*
    /// \return CURL*
    /// \throw http::Error
    /////////////////////////////////////////////////
    static CURL* get_init(const std::string& sUrl, const std::string& sUserName, const std::string& sPassWord, std::string* buffer)
    {
        CURLcode code;

        CURL* conn = common_init(sUrl, sUserName, sPassWord);

        try
        {
            code = curl_easy_setopt(conn, CURLOPT_WRITEFUNCTION, writer);

            if (code != CURLE_OK)
                throw Error("Failed to set writer [" + std::string(errorBuffer) + "].");

            code = curl_easy_setopt(conn, CURLOPT_WRITEDATA, buffer);

            if (code != CURLE_OK)
                throw Error("Failed to set write data [" + std::string(errorBuffer) + "].");
        }
        catch (...)
        {
            curl_easy_cleanup(conn);
            throw;
        }

        return conn;
    }


    /////////////////////////////////////////////////
    /// \brief libcurl put connection initialization.
    ///
    /// \param sUrl const std::string&
    /// \param sUserName const std::string&
    /// \param sPassWord const std::string&
    /// \param filestream std::ifstream*
    /// \param filesize size_t
    /// \return CURL*
    /// \throw http::Error
    /////////////////////////////////////////////////
    static CURL* put_init(const std::string& sUrl, const std::string& sUserName, const std::string& sPassWord, std::ifstream* filestream, size_t filesize)
    {
        CURLcode code;

        CURL* conn = common_init(sUrl, sUserName, sPassWord);

        try
        {
            code = curl_easy_setopt(conn, CURLOPT_UPLOAD, 1L);

            if (code != CURLE_OK)
                throw Error("Failed to set upload option [" + std::string(errorBuffer) + "].");

            code = curl_easy_setopt(conn, CURLOPT_READFUNCTION, reader);

            if (code != CURLE_OK)
                throw Error("Failed to set reader [" + std::string(errorBuffer) + "].");

            code = curl_easy_setopt(conn, CURLOPT_READDATA, filestream);

            if (code != CURLE_OK)
                throw Error("Failed to set filestream [" + std::string(errorBuffer) + "].");

                curl_easy_setopt(conn, CURLOPT_INFILESIZE, filesize);
        }
        catch (...)
        {
            curl_easy_cleanup(conn);
            throw;
        }

        return conn;
    }


    /////////////////////////////////////////////////
    /// \brief Get the contents of a URL.
    ///
    /// \param sUrl const std::string&
    /// \param sUserName const std::string&
    /// \param sPassWord const std::string&
    /// \return std::string
    /// \throw http::Error
    /////////////////////////////////////////////////
    std::string get(const std::string& sUrl, const std::string& sUserName, const std::string& sPassWord)
    {
        CURL* conn = nullptr;
        CURLcode code;
        std::string buffer;
        buffer.clear();

        curl_global_init(CURL_GLOBAL_DEFAULT);

        // Initialize CURL connection
        conn = get_init(sUrl, sUserName, sPassWord, &buffer);

        // Retrieve content for the URL
        code = curl_easy_perform(conn);
        curl_easy_cleanup(conn);

        if (code != CURLE_OK)
            throw Error("Failed to get '" + sUrl + "': " + std::string(errorBuffer) + ".");

        // Return the buffer contents
        return buffer;
    }


    /////////////////////////////////////////////////
    /// \brief Upload a file to a destination and
    /// return the transmitted bytes.
    ///
    /// \param sUrl const std::string&
    /// \param sFileName const std::string&
    /// \param sUserName const std::string&
    /// \param sPassWord const std::string&
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t put(const std::string& sUrl, const std::string& sFileName, const std::string& sUserName, const std::string& sPassWord)
    {
        CURL* conn = nullptr;
        CURLcode code;
        std::ifstream filestream(sFileName.c_str(), std::ios_base::binary);

        if (!filestream.good())
            throw Error("Cannot open file '" + sFileName + "'.");

        filestream.seekg(0, std::ios_base::end);
        size_t s = filestream.tellg();
        filestream.seekg(0, std::ios_base::beg);

        if (!s)
            throw Error("File size of '" + sFileName + "' is zero.");

        curl_global_init(CURL_GLOBAL_DEFAULT);

        // Initialize CURL connection
        conn = put_init(sUrl, sUserName, sPassWord, &filestream, s);

        // Retrieve content for the URL
        code = curl_easy_perform(conn);
        curl_easy_cleanup(conn);

        if (code != CURLE_OK)
            throw Error("Failed to upload to '" + sUrl + "': " + std::string(errorBuffer) + ".");

        // Return the file size
        return s;
    }
}

