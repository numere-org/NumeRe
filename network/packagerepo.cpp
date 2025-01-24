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

#include <fstream>

#include "packagerepo.hpp"
#include "http.h"
#include "../kernel/core/utils/tools.hpp"
#include "../kernel/core/io/logger.hpp"

static std::vector<std::string> fetchListHelper(const std::string& sRepoUrl);
static std::string createSalt();
static std::string getTagValue(const std::string& sTaggedString, const std::string& sTag);


/////////////////////////////////////////////////
/// \brief This function fetches the
/// repository main page as HTML and extracts the
/// list of item links in it. These may be files
/// and folders.
///
/// \param sRepoUrl const std::string&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
static std::vector<std::string> fetchListHelper(const std::string& sRepoUrl)
{
    std::vector<std::string> vRepoContents;
    std::string sRepoContents = url::get(sRepoUrl + createSalt());
    sRepoContents = sRepoContents.substr(sRepoContents.find("<ul>")+4);
    sRepoContents.erase(sRepoContents.find("</ul>"));

    // Extract each <li></li> pairs content
    while (sRepoContents.length() && sRepoContents.find("</li>") != std::string::npos)
    {
        std::string sCurrentPackage = sRepoContents.substr(0, sRepoContents.find("</li>"));
        sRepoContents.erase(0, sRepoContents.find("</li>")+5);

        if (sCurrentPackage.find("href=\"") == std::string::npos)
            break;

        // Get the address
        sCurrentPackage = sCurrentPackage.substr(sCurrentPackage.find("href=\"") + 6);
        sCurrentPackage.erase(sCurrentPackage.find('"'));

        // We do not want to follow parent directory
        // references
        if (sCurrentPackage.find("../") != std::string::npos)
            continue;

        // If the current URL is not a file, it might be a folder,
        // therefore we trigger a recursion here
        if (sCurrentPackage.back() == '/')
        {
            std::vector<std::string> vFolderContents = fetchListHelper(sRepoUrl + sCurrentPackage);

            if (vFolderContents.size())
                vRepoContents.insert(vRepoContents.end(), vFolderContents.begin(), vFolderContents.end());

            continue;
        }

        // We only accept NSCR files at the moment
        if (sCurrentPackage.find(".nscr") == std::string::npos)
            continue;

        // Get the resolved content
        vRepoContents.push_back(sRepoUrl + sCurrentPackage);
    }

    return vRepoContents;
}


/////////////////////////////////////////////////
/// \brief Static helper function to extract the
/// value of an install info tag.
///
/// \param sTaggedString const std::string&
/// \param sTag const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
static std::string getTagValue(const std::string& sTaggedString, const std::string& sTag)
{
    int nTag = findParameter(sTaggedString, sTag, '=');

    if (nTag)
        return getArgAtPos(sTaggedString, nTag+sTag.length());

    return "";
}


/////////////////////////////////////////////////
/// \brief Simple function to make the URL more
/// unique to avoid server caching (which might
/// resolve in a very delated update of the
/// package list as the server will respond with
/// the already cached contents rather than the
/// current file versions).
///
/// \return std::string
///
/////////////////////////////////////////////////
static std::string createSalt()
{
    return "?" + std::to_string(clock());
}



/////////////////////////////////////////////////
/// \brief PackageRepo constructor.
///
/// \param sRepoUrl const std::string&
///
/////////////////////////////////////////////////
PackageRepo::PackageRepo(const std::string& sRepoUrl)
{
    connect(sRepoUrl);
}


/////////////////////////////////////////////////
/// \brief Connect to a new repository.
///
/// \param sRepoUrl const std::string&
/// \return void
///
/////////////////////////////////////////////////
void PackageRepo::connect(const std::string& sRepoUrl)
{
    m_repoUrl = sRepoUrl;
    m_lastRefreshed = 0;
}


/////////////////////////////////////////////////
/// \brief Returns, whether the package
/// repository is connected.
///
/// \return bool
///
/////////////////////////////////////////////////
bool PackageRepo::is_connected() const
{
    return m_repoUrl.length() != 0;
}


/////////////////////////////////////////////////
/// \brief Fetch the list of available packages
/// and return their URLs.
///
/// \return const std::vector<std::string>&
///
/////////////////////////////////////////////////
const std::vector<std::string>& PackageRepo::fetchList()
{
    if (_time64(nullptr) - m_lastRefreshed > 600)
    {
        m_list = fetchListHelper(m_repoUrl);
        m_lastRefreshed = _time64(nullptr);
    }

    return m_list;
}


/////////////////////////////////////////////////
/// \brief Fetch the PackageInfo of a single
/// package.
///
/// \param sPackageUrl const std::string&
/// \return PackageInfo
///
/////////////////////////////////////////////////
PackageInfo PackageRepo::fetchInfo(const std::string& sPackageUrl)
{
    std::string sCurrentPackage = url::get(sPackageUrl + createSalt());
    PackageInfo pkgInfo;

    if (!sCurrentPackage.length())
        return pkgInfo;

    // Get the information
    std::string sInfo = sCurrentPackage.substr(sCurrentPackage.find("<info>"),
                                               sCurrentPackage.find("<endinfo>") - sCurrentPackage.find("<info>"));
    replaceAll(sInfo, "\t", " ");
    replaceAll(sInfo, "\n", " ");
    replaceAll(sInfo, "\r", " ");

    // Fill the package list
    pkgInfo.name = getTagValue(sInfo, "name");
    pkgInfo.version = "v" + getTagValue(sInfo, "version");
    pkgInfo.author = getTagValue(sInfo, "author");
    pkgInfo.type = getTagValue(sInfo, "type").find("PLUGIN") != std::string::npos ? "Plugin" : "Package";
    std::string sReqVersion = getTagValue(sInfo, "requireversion");

    if (sReqVersion.length())
        pkgInfo.requiredVersion = "v" + sReqVersion;

    pkgInfo.requiredPackages = getTagValue(sInfo, "requirepackages");
    pkgInfo.description = getTagValue(sInfo, "desc");

    if (!pkgInfo.description.length())
        pkgInfo.description = getTagValue(sInfo, "plugindesc");

    replaceAll(pkgInfo.description, "\\\"", "\"");
    replaceAll(pkgInfo.description, "\\n", "\n");

    pkgInfo.keyWords = getTagValue(sInfo, "keywords");
    pkgInfo.changeLog = getTagValue(sInfo, "changelog");

    replaceAll(pkgInfo.changeLog, "\\\"", "\"");
    replaceAll(pkgInfo.changeLog, "\\n", "\n");

    pkgInfo.license = getTagValue(sInfo, "license");
    pkgInfo.repoUrl = sPackageUrl;

    return pkgInfo;
}


/////////////////////////////////////////////////
/// \brief Find a package by its ID (the
/// filename) and return the information of the
/// package.
///
/// \param pkgId const std::string&
/// \return PackageInfo
///
/////////////////////////////////////////////////
PackageInfo PackageRepo::find(const std::string& pkgId)
{
    if (_time64(nullptr) - m_lastRefreshed > 600)
        fetchList();

    for (const std::string& pkg : m_list)
    {
        if (pkg.ends_with("/" + pkgId + ".nscr"))
            return fetchInfo(pkg);
    }

    return PackageInfo();
}


/////////////////////////////////////////////////
/// \brief Download a package to a defined
/// location.
///
/// \param sPackageUrl const std::string&
/// \param sTargetFile const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool PackageRepo::download(const std::string& sPackageUrl, const std::string& sTargetFile)
{
    std::string contents = url::get(sPackageUrl);

    std::ofstream file(sTargetFile.c_str(), std::ios::binary | std::ios::out | std::ios::trunc);

    if (file.good())
    {
        file.write(contents.c_str(), contents.length());
        file.close();

        return true;
    }

    return false;
}



