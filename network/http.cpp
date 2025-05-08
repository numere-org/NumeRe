
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
#include "../kernel/core/utils/tools.hpp"
#include "curlcpp.hpp"
#include <fstream>
#include <sstream>

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
        std::istream* inStream = static_cast<std::istream*>(stream);
        size_t bytes = inStream->readsome(ptr, size * nmemb);
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
    /// \param httpHeader const std::vector<std::string>&
    /// \return CurlCpp
    ///
    /////////////////////////////////////////////////
    static CurlCpp common_init(const std::string& sUrl, const std::string& sUserName, const std::string& sPassWord, const std::vector<std::string>& httpHeader)
    {
        CurlCpp curl(true);

        if (!curl)
            throw Error("Failed to create CURL connection.");

        if (!curl.setOption(CURLOPT_ERRORBUFFER, errorBuffer))
            throw Error("Failed to set error buffer.");

        if (!curl.setOption(CURLOPT_URL, sUrl.c_str()))
            throw Error("Failed to set URL [" + std::string(errorBuffer) + "].");

        if (httpHeader.size())
        {
            if (!curl.setOption(CURLOPT_USERAGENT, curl_version()))
                throw Error("Failed to set user agent [" + std::string(errorBuffer) + "].");

            if (!curl.setHeader(httpHeader))
                throw Error("Failed to set http header [" + std::string(errorBuffer) + "].");
        }
        else
        {
            if (!curl.setOption(CURLOPT_USERAGENT, "Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:89.0) Gecko/20100101 Firefox/89.0"))
                throw Error("Failed to set user agent [" + std::string(errorBuffer) + "].");
        }

        if (sUserName.length() && sPassWord.length())
        {
            if (!curl.setOption(CURLOPT_USERPWD, (sUserName + ":" + sPassWord).c_str()))
                throw Error("Failed to set username and password [" + std::string(errorBuffer) + "].");
        }

        if (!curl.setOption(CURLOPT_SSL_VERIFYPEER, 0L))
            throw Error("Failed to set SSL peer verify option [" + std::string(errorBuffer) + "].");

        if (!curl.setOption(CURLOPT_SSL_VERIFYHOST, 0L))
            throw Error("Failed to set SSL host verify option [" + std::string(errorBuffer) + "].");

        if (!curl.setOption(CURLOPT_FOLLOWLOCATION, 1L))
            throw Error("Failed to set redirect option [" + std::string(errorBuffer) + "].");

        if (!curl.setOption(CURLOPT_CONNECTTIMEOUT, 10L))
            throw Error("Failed to set connection time-out option [" + std::string(errorBuffer) + "].");

        if (!curl.setOption(CURLOPT_LOW_SPEED_LIMIT, 1024))
            throw Error("Failed to set low-speed option [" + std::string(errorBuffer) + "].");

        if (!curl.setOption(CURLOPT_LOW_SPEED_TIME, 30L))
            throw Error("Failed to set low-speed time option [" + std::string(errorBuffer) + "].");

        if (!curl.setOption(CURLOPT_NOPROGRESS, 0L))
            throw Error("Failed to set progress option [" + std::string(errorBuffer) + "].");

        if (!curl.setOption(CURLOPT_XFERINFOFUNCTION, progress_callback))
            throw Error("Failed to set progress function [" + std::string(errorBuffer) + "].");

        ProgressData data;

        if (!curl.setOption(CURLOPT_XFERINFODATA, &data))
            throw Error("Failed to set progress data [" + std::string(errorBuffer) + "].");

        return curl;
    }


    /////////////////////////////////////////////////
    /// \brief libcurl get connection initialization.
    ///
    /// \param sUrl const std::string&
    /// \param sUserName const std::string&
    /// \param sPassWord const std::string&
    /// \param httpHeader const std::vector<std::string>&
    /// \param buffer std::string*
    /// \return CurlCpp
    /// \throw http::Error
    /////////////////////////////////////////////////
    static CurlCpp get_init(const std::string& sUrl, const std::string& sUserName, const std::string& sPassWord, const std::vector<std::string>& httpHeader, std::string* buffer)
    {
        CurlCpp curl = common_init(sUrl, sUserName, sPassWord, httpHeader);

        if (!curl.setOption(CURLOPT_WRITEFUNCTION, writer))
            throw Error("Failed to set writer [" + std::string(errorBuffer) + "].");

        if (!curl.setOption(CURLOPT_WRITEDATA, buffer))
            throw Error("Failed to set write data [" + std::string(errorBuffer) + "].");

        return curl;
    }


    /////////////////////////////////////////////////
    /// \brief libcurl put connection initialization.
    ///
    /// \param sUrl const std::string&
    /// \param sUserName const std::string&
    /// \param sPassWord const std::string&
    /// \param httpHeader const std::vector<std::string>&
    /// \param filestream std::ifstream*
    /// \param filesize size_t
    /// \return CurlCpp
    /// \throw http::Error
    /////////////////////////////////////////////////
    static CurlCpp put_init(const std::string& sUrl, const std::string& sUserName, const std::string& sPassWord, const std::vector<std::string>& httpHeader, std::ifstream* filestream, size_t filesize)
    {
        CurlCpp curl = common_init(sUrl, sUserName, sPassWord, httpHeader);

        if (!curl.setOption(CURLOPT_UPLOAD, 1L))
            throw Error("Failed to set upload option [" + std::string(errorBuffer) + "].");

        if (!curl.setOption(CURLOPT_READFUNCTION, reader))
            throw Error("Failed to set reader [" + std::string(errorBuffer) + "].");

        if (!curl.setOption(CURLOPT_READDATA, filestream))
            throw Error("Failed to set filestream [" + std::string(errorBuffer) + "].");

        curl.setOption(CURLOPT_INFILESIZE, filesize);

        return curl;
    }


    /////////////////////////////////////////////////
    /// \brief libcurl get connection initialization.
    ///
    /// \param sUrl const std::string&
    /// \param sUserName const std::string&
    /// \param sPassWord const std::string&
    /// \param httpHeader const std::vector<std::string>&
    /// \param sPayLoad const std::string&
    /// \param buffer std::string*
    /// \return CurlCpp
    /// \throw http::Error
    /////////////////////////////////////////////////
    static CurlCpp post_init(const std::string& sUrl, const std::string& sUserName, const std::string& sPassWord, const std::vector<std::string>& httpHeader, const std::string& sPayLoad, std::string* buffer)
    {
        CurlCpp curl = common_init(sUrl, sUserName, sPassWord, httpHeader);

        if (!curl.setOption(CURLOPT_POST, 1L))
            throw Error("Failed to set POST option [" + std::string(errorBuffer) + "].");

        if (!curl.setOption(CURLOPT_COPYPOSTFIELDS, sPayLoad.c_str()))
            throw Error("Failed to set POST option [" + std::string(errorBuffer) + "].");

        if (!curl.setOption(CURLOPT_WRITEFUNCTION, writer))
            throw Error("Failed to set writer [" + std::string(errorBuffer) + "].");

        if (!curl.setOption(CURLOPT_WRITEDATA, buffer))
            throw Error("Failed to set write data [" + std::string(errorBuffer) + "].");

        return curl;
    }


    /////////////////////////////////////////////////
    /// \brief libcurl mail initialization.
    ///
    /// \param sUrl const std::string&
    /// \param sUserName const std::string&
    /// \param sPassWord const std::string&
    /// \param sFromMail const std::string&
    /// \param recipients const std::vector<std::string>&
    /// \param body std::istringstream*
    /// \param bodysize size_t
    /// \return CurlCpp
    ///
    /////////////////////////////////////////////////
    static CurlCpp mail_init(const std::string& sUrl, const std::string& sUserName, const std::string& sPassWord, const std::string& sFromMail, const std::vector<std::string>& recipients, std::istringstream* body, size_t bodysize)
    {
        CurlCpp curl = common_init(sUrl, sUserName, sPassWord, std::vector<std::string>());

        // This line enables the use of STARTTLS
        if (!sUrl.starts_with("smtps://")
            && (sUrl.ends_with(":465") || sUrl.ends_with(":587"))
            && !curl.setOption(CURLOPT_USE_SSL, (long)CURLUSESSL_ALL))
            throw Error("Failed to set SSL option [" + std::string(errorBuffer) + "].");

        if (!curl.setOption(CURLOPT_UPLOAD, 1L))
            throw Error("Failed to set upload option [" + std::string(errorBuffer) + "].");

        if (!curl.setOption(CURLOPT_READFUNCTION, reader))
            throw Error("Failed to set reader [" + std::string(errorBuffer) + "].");

        if (!curl.setOption(CURLOPT_MAIL_FROM, sFromMail.c_str()))
            throw Error("Failed to set from-mail [" + std::string(errorBuffer) + "].");

        if (!curl.setRecipients(recipients))
            throw Error("Failed to set recipients list [" + std::string(errorBuffer) + "].");

        if (!curl.setOption(CURLOPT_READDATA, body))
            throw Error("Failed to set body stream [" + std::string(errorBuffer) + "].");

        curl.setOption(CURLOPT_INFILESIZE, bodysize);

        return curl;
    }


    /////////////////////////////////////////////////
    /// \brief Get the contents of a URL.
    ///
    /// \param sUrl const std::string&
    /// \param sUserName const std::string&
    /// \param sPassWord const std::string&
    /// \param httpHeader const std::vector<std::string>&
    /// \return std::string
    /// \throw http::Error
    /////////////////////////////////////////////////
    std::string get(const std::string& sUrl, const std::string& sUserName, const std::string& sPassWord, const std::vector<std::string>& httpHeader)
    {
        std::string buffer;
        buffer.clear();

        // Initialize CURL connection
        CurlCpp curl = get_init(sUrl, sUserName, sPassWord, httpHeader, &buffer);

        // Retrieve content for the URL
        if (!curl.perform())
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
    /// \param httpHeader const std::vector<std::string>&
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t put(const std::string& sUrl, const std::string& sFileName, const std::string& sUserName, const std::string& sPassWord, const std::vector<std::string>& httpHeader)
    {
        std::ifstream filestream(sFileName.c_str(), std::ios_base::binary);

        if (!filestream.good())
            throw Error("Cannot open file '" + sFileName + "'.");

        filestream.seekg(0, std::ios_base::end);
        size_t s = filestream.tellg();
        filestream.seekg(0, std::ios_base::beg);

        if (!s)
            throw Error("File size of '" + sFileName + "' is zero.");

        // Initialize CURL connection
        CurlCpp curl = put_init(sUrl, sUserName, sPassWord, httpHeader, &filestream, s);

        // Retrieve content for the URL
        if (!curl.perform())
            throw Error("Failed to upload to '" + sUrl + "': " + std::string(errorBuffer) + ".");

        // Return the file size
        return s;
    }


    /////////////////////////////////////////////////
    /// \brief Upload a file to a destination and
    /// return the transmitted bytes.
    ///
    /// \param sUrl const std::string&
    /// \param sUserName const std::string&
    /// \param sPassWord const std::string&
    /// \param httpHeader const std::vector<std::string>&
    /// \param sPayLoad const std::string&
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string post(const std::string& sUrl, const std::string& sUserName, const std::string& sPassWord, const std::vector<std::string>& httpHeader, const std::string& sPayLoad)
    {
        std::string buffer;
        buffer.clear();

        // Initialize CURL connection
        CurlCpp curl = post_init(sUrl, sUserName, sPassWord, httpHeader, sPayLoad, &buffer);

        // Retrieve content for the URL
        if (!curl.perform())
            throw Error("Failed to post to '" + sUrl + "': " + std::string(errorBuffer) + ".");

        // Return the file size
        return buffer;
    }


    /////////////////////////////////////////////////
    /// \brief Send a mail to the recipients list.
    /// The mail body must follow RFC 5322 and is not
    /// modified via this function.
    ///
    /// \param sUrl const std::string&
    /// \param sBody const std::string&
    /// \param sUserName const std::string&
    /// \param sPassWord const std::string&
    /// \param sFromMail const std::string&
    /// \param recipients const std::vector<std::string>&
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t sendMail(const std::string& sUrl, const std::string& sBody, const std::string& sUserName, const std::string& sPassWord, const std::string& sFromMail, const std::vector<std::string>& recipients)
    {
        std::istringstream bodyStream(sBody);

        if (!sBody.length())
            throw Error("No mail body attached.");

        if (!recipients.size())
            throw Error("No mail recipients defined.");

        // Initialize CURL connection
        CurlCpp curl = mail_init(sUrl, sUserName, sPassWord, sFromMail, recipients, &bodyStream, sBody.length());

        // Retrieve content for the URL
        if (!curl.perform())
            throw Error("Failed to send mail via '" + sUrl + "': " + std::string(errorBuffer) + ".");

        // Return the file size
        return sBody.length();
    }

}

