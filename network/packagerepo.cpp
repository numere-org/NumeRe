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

#include <boost/nowide/fstream.hpp>
#include <regex>
#include <sstream>
#include <fstream>

#include "packagerepo.hpp"
#include "http.h"
#include "../kernel/core/utils/tools.hpp"
#include "../kernel/core/io/logger.hpp"
#include "../kernel/kernel.hpp"

// Fallback configuration, if no configuration is supplied
static const std::string DEFAULTREPOCONFIG =
"{"
    "\"version\": \"1.0.0\","
    "\"name\": \"NumeRe::Packages\","
    "\"url\": \"https://github.com/numere-org/Packages\","
    "\"authentication\": { \"required\": false, \"method\": \"\" },"
    "\"keys\": {\"path\" : \"path\", \"sha\": \"sha\", \"root\": \"tree\"},"
    "\"tree\": \"https://api.github.com/repos/numere-org/Packages/git/trees/HEAD?recursive=true\","
    "\"raw-file\": \"https://raw.githubusercontent.com/numere-org/Packages/refs/heads/main/{path}\""
"}";
static const std::string DEFAULTREPOINDEXFILE = "<>/remotes/10_numere_packages.index";


/////////////////////////////////////////////////
/// \brief Static helper for easy JSON string
/// parsing.
///
/// \param sJsonString const std::string&
/// \param target Json::Value&
/// \return void
///
/////////////////////////////////////////////////
static void parseJson(const std::string& sJsonString, Json::Value& target)
{
    try
    {
        std::istringstream iStr(sJsonString);
        iStr >> target;
    }
    catch (Json::RuntimeError& e)
    {
        throw PackageRepoError(e.what());
    }
    catch (Json::LogicError& e)
    {
        throw PackageRepoError(e.what());
    }
}


/////////////////////////////////////////////////
/// \brief PackageRepo constructor.
///
/// \param sRepoConfig const std::string&
/// \param sIndexFile const std::string&
///
/////////////////////////////////////////////////
PackageRepo::PackageRepo(const std::string& sRepoConfig, const std::string& sIndexFile)
{
    connect(sRepoConfig, sIndexFile);
}


/////////////////////////////////////////////////
/// \brief Connect to a new repository.
///
/// \param sRepoConfig const std::string&
/// \param sIndexFile const std::string&
/// \return void
///
/////////////////////////////////////////////////
void PackageRepo::connect(const std::string& sRepoConfig, const std::string& sIndexFile)
{
    parseJson(sRepoConfig.length() ? sRepoConfig : DEFAULTREPOCONFIG, m_repoConfig);
    m_indexFile = sIndexFile.length() ? sIndexFile : DEFAULTREPOINDEXFILE;

    if (m_indexFile.starts_with("<>"))
        replaceAll(m_indexFile, "<>", NumeReKernel::getInstance()->getSettings().getExePath().c_str());

    // Validate version
    if (!m_repoConfig.isMember("version"))
        throw PackageRepoError("The repository configuration '" + sRepoConfig + "' does not specify a version field.");

    if (m_repoConfig["version"].asString() >= "1.1.0")
        throw PackageRepoError("The version of the repository configuration '" + sRepoConfig
                               + "' is newer than supported by this NumeRe version. Consider updating.");

    // Validate the configuration's contents
    if (!m_repoConfig.isMember("name")
        || !m_repoConfig.isMember("tree")
        || !m_repoConfig.isMember("raw-file")
        || !m_repoConfig.isMember("verify-host")
        || !m_repoConfig.isMember("authentication")
        || !m_repoConfig.isMember("keys"))
        throw PackageRepoError("The repository configuration '" + sRepoConfig + "' lacks required fields.");

    if (!m_repoConfig["authentication"].isMember("required")
        || !m_repoConfig["authentication"].isMember("method")
        || !m_repoConfig["keys"].isMember("path")
        || !m_repoConfig["keys"].isMember("root")
        || !m_repoConfig["keys"].isMember("sha"))
        throw PackageRepoError("The repository configuration '" + sRepoConfig + "' lacks required fields.");

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
    return m_index.size() != 0;
}


/////////////////////////////////////////////////
/// \brief Return the repository name.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string PackageRepo::getRepoName() const
{
    return m_repoConfig["name"].asString();
}


/////////////////////////////////////////////////
/// \brief Determine, whether the internal index
/// needs a refresh.
///
/// \return bool
///
/////////////////////////////////////////////////
bool PackageRepo::needsRefresh() const
{
    return _time64(nullptr) - m_lastRefreshed > 1200;
}


/////////////////////////////////////////////////
/// \brief Fetch the index of available packages
/// and return their URLs.
///
/// \return const std::map<std::string, PackageVersions>&
///
/////////////////////////////////////////////////
const std::map<std::string, PackageVersions>& PackageRepo::fetchIndex() const
{
    static std::string CACERTFILE = NumeReKernel::getInstance()->getCaCertFile();

    if (needsRefresh())
    {
        std::string sResponse;

        try
        {
            url::UrlParams params("", "", m_repoConfig["verify-host"].asBool() ? CACERTFILE : "");

            if (m_repoConfig["authentication"]["required"].asBool())
                params.m_httpHeader.push_back(m_repoConfig["authentication"]["method"].asString());

            sResponse = url::get(m_repoConfig["tree"].asString(), params);

        }
        catch (...)
        {
            // Do we already have an index? Then we're probably rate-limited. Just
            // leave it as it was and set the fetching time to now
            if (m_index.size())
            {
                g_logger.warning("Unable to download an index update from " + m_repoConfig["tree"].asString()
                                 + ". Re-using the current index. Check your rate-limits.");
                m_lastRefreshed = _time64(nullptr);
                return m_index;
            }

            // Did we cache an index in the past?
            if (m_indexFile.length() && fileExists(m_indexFile))
            {
                g_logger.warning("Unable to download the current index from " + m_repoConfig["tree"].asString()
                                 + ". Re-using the cached index from an earlier session. Check your rate-limits.");

                std::ifstream indexFile(boost::nowide::widen(m_indexFile).c_str());
                std::string line;
                sResponse.clear();

                while (indexFile.good() && !indexFile.eof())
                {
                    std::getline(indexFile, line);

                    if (sResponse.length())
                        sResponse += "\n";

                    sResponse += line;
                }
            }
            else
                throw;
        }

        Json::Value tree;
        parseJson(sResponse, tree);

        if (!tree.isArray() && tree.isMember("message"))
            throw PackageRepoError("The repository reported: " + tree["message"].asString());

        // Validate the response
        if (!m_repoConfig["keys"]["root"].isNull() && !tree.isMember(m_repoConfig["keys"]["root"].asString()))
            throw PackageRepoError("The repository response lacks the required tree root field.");

        // Do we have a dedicated root node?
        if (!m_repoConfig["keys"]["root"].isNull())
            tree = tree[m_repoConfig["keys"]["root"].asString()];

        static std::regex packagePath("packages/.+/\\d\\.\\d\\.\\d/.+\\.\\w+");

        std::string pathKey = m_repoConfig["keys"]["path"].asString();
        std::string shaKey = m_repoConfig["keys"]["sha"].asString();
        std::string rawFile = m_repoConfig["raw-file"].asString();

        // Ensure that the response has the relevant keys
        if (!tree.size() || !tree[0].isMember(pathKey) || !tree[0].isMember(shaKey))
            throw PackageRepoError("The tree is empty or the configuration does not match the returned response.");

        m_index.clear();

        for (const Json::Value& val : tree)
        {
            std::string path = val[pathKey].asString();

            if (std::regex_match(path, packagePath))
            {
                g_logger.debug("Found package folder: " + path);
                std::vector<std::string> packageDef = split(path, '/');

                // Create individual fields, if they do not exist alreeady
                m_index[packageDef[1]];
                m_index[packageDef[1]].versions[packageDef[2]];

                // Prepare the file URL
                std::string fileUrl = rawFile;
                replaceAll(fileUrl, "{path}", path.c_str());
                replaceAll(fileUrl, "{sha}", val[shaKey].asCString());

                if (packageDef[3] == "meta.json")
                    m_index[packageDef[1]].versions[packageDef[2]].meta = fileUrl;
                else
                    m_index[packageDef[1]].versions[packageDef[2]].file = fileUrl;
            }
        }

        if (m_index.empty())
            g_logger.warning("The index from " + m_repoConfig["tree"].asString() + " does not list any installable packages.");

        // Write the current index to disk so that we can recycle it
        // if the Github REST API tokens are running out (can happen,
        // if many people sharing the same IP)
        std::ofstream indexFile(boost::nowide::widen(m_indexFile).c_str());
        indexFile << sResponse;

        m_lastRefreshed = _time64(nullptr);
    }

    return m_index;
}


/////////////////////////////////////////////////
/// \brief Fetch the PackageInfo of a single
/// package.
///
/// \param sPackageId const std::string&
/// \return PackageInfo
///
/////////////////////////////////////////////////
PackageInfo PackageRepo::fetchInfo(const std::string& sPackageId) const
{
    if (needsRefresh())
        fetchIndex();

    static std::string CACERTFILE = NumeReKernel::getInstance()->getCaCertFile();

    auto iter = m_index.find(sPackageId);

    if (iter == m_index.end())
        return PackageInfo();

    std::string sMetaFile;

    url::UrlParams params("", "", m_repoConfig["verify-host"].asBool() ? CACERTFILE : "");

    if (m_repoConfig["authentication"]["required"].asBool())
        params.m_httpHeader.push_back(m_repoConfig["authentication"]["method"].asString());

    sMetaFile = url::get(iter->second.getLatest().meta, params);

    Json::Value meta;
    parseJson(sMetaFile, meta);

    // Test the manifest version
    if (meta["manifest-version"].asString() >= "1.1.0")
        throw PackageRepoError("The manifest version of '" + sPackageId
                               + "' is newer than supported by this NumeRe version. Consider updating.");

    PackageInfo pkgInfo;
    pkgInfo.repoName = getRepoName();
    pkgInfo.packageId = sPackageId;
    pkgInfo.author = meta["author"].asString();
    pkgInfo.name = meta["name"].asString();
    pkgInfo.repoUrl = iter->second.getLatest().file;
    pkgInfo.type = sPackageId.starts_with("plgn_") ? "Plugin" : "Package";
    pkgInfo.version = iter->second.versions.rbegin()->first;

    pkgInfo.changeLog = meta.get("changelog", "").asString();
    pkgInfo.description = meta.get("description", "").asString();
    pkgInfo.keyWords = meta.get("keywords", "").asString();
    pkgInfo.license = meta.get("license", "").asString();

    if (meta.isMember("requirements"))
    {
        pkgInfo.requiredPackages = meta["requirements"].get("packages", "").asString();
        pkgInfo.requiredVersion = meta["requirements"].get("version", "").asString();
    }

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
PackageInfo PackageRepo::find(const std::string& pkgId) const
{
    if (needsRefresh())
        fetchIndex();

    auto iter = m_index.find(pkgId);

    if (iter != m_index.end())
        return fetchInfo(pkgId);

    return PackageInfo();
}


/////////////////////////////////////////////////
/// \brief Find package candidates by using only
/// a part of the package id. Does not download
/// any of the necessary data with exception of
/// the repository list.
///
/// \param pkgId const std::string&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> PackageRepo::find_candidates(const std::string& pkgId) const
{
    if (needsRefresh())
        fetchIndex();

    std::vector<std::string> candidates;
    std::string repoName = getRepoName();

    for (const auto& pkg : m_index)
    {
        if (pkg.first.find(pkgId) != std::string::npos)
            candidates.push_back(pkg.first + "@" + repoName);
    }

    return candidates;
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
bool PackageRepo::download(const std::string& sPackageUrl, const std::string& sTargetFile) const
{
    // Test, whether this repository actually provides raw file links like the one
    // used within this call
    const std::string& rawFile = m_repoConfig["raw-file"].asString();

    static std::string CACERTFILE = NumeReKernel::getInstance()->getCaCertFile();

    if (!sPackageUrl.starts_with(rawFile.substr(0, rawFile.find('{'))))
        return false;

    std::string contents;

    try
    {
        url::UrlParams params("", "", m_repoConfig["verify-host"].asBool() ? CACERTFILE : "");

        if (m_repoConfig["authentication"]["required"].asBool())
            params.m_httpHeader.push_back(m_repoConfig["authentication"]["method"].asString());

        contents = url::get(sPackageUrl, params);
    }
    catch (url::Error& e)
    {
        g_logger.warning("Download of " + sPackageUrl + " failed.");
        return false;
    }

    // Ensure that the reponse does not indicate that we
    // were not able to find the desired remote file
    if (contents.find("\"message\":") != std::string::npos)
    {
        // The conversion to JSON may fail
        try
        {
            Json::Value response;
            parseJson(contents, response);

            if ((response["message"].asString() == "Not found" || response["message"].asString() == "401 Unauthorized")
                || response.get("status", "").asString() == "404")
                return false;
        }
        catch (...)
        {
            // Do nothing, because we just assumed that the response is JSON,
            // so everything is fine here
        }
    }

    boost::nowide::ofstream file(sTargetFile.c_str(), std::ios::binary | std::ios::out | std::ios::trunc);

    if (file.good())
    {
        file.write(contents.c_str(), contents.length());
        file.close();

        return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief Return the number of packages in this
/// repository.
///
/// \return size_t
///
/////////////////////////////////////////////////
size_t PackageRepo::size() const
{
    if (needsRefresh())
        fetchIndex();

    return m_index.size();
}







/////////////////////////////////////////////////
/// \brief Imports the remote package repository
/// configurations and prepares the remote
/// connections (but does not perform any
/// networking).
///
/// \param remoteConfigs const std::vector<std::string>&
/// \return bool
///
/////////////////////////////////////////////////
bool PackageRepoManager::importConfigs(const std::vector<std::string>& remoteConfigs)
{
    for (const std::string& cfg : remoteConfigs)
    {
        try
        {
            g_logger.debug("Importing " + cfg + " ...");
            std::ifstream cfgFile(cfg);
            std::string sRepoConfig;
            std::string line;

            while (cfgFile.good() && !cfgFile.eof())
            {
                std::getline(cfgFile, line);
                StripSpaces(line);
                sRepoConfig += line;
            }

            m_remotes.push_back(PackageRepo(sRepoConfig, cfg.substr(0, cfg.length()-10) + "index"));
        }
        catch (std::exception& e)
        {
            g_logger.error(e.what());
        }
        catch (...)
        {
            g_logger.error("Generic error during configuration import.");
        }
    }

    if (!m_remotes.size())
    {
        g_logger.warning("No valid repository configuration files found. Using default repository configuration.");
        m_remotes.push_back(PackageRepo());
    }

    return true;
}


/////////////////////////////////////////////////
/// \brief Separate the PKGID@REMOTE notation.
///
/// \param sIdAndRemote const std::string&
/// \return std::pair<std::string,std::string>
///
/////////////////////////////////////////////////
std::pair<std::string,std::string> getIdAndRemote(const std::string& sIdAndRemote)
{
    size_t separator = sIdAndRemote.find('@');
    return std::make_pair(sIdAndRemote.substr(0, separator), sIdAndRemote.substr(separator+1));
}


/////////////////////////////////////////////////
/// \brief Fetch the PackageInfo of a single
/// package.
///
/// \param sPackageId const std::string&
/// \param sRepoName const std::string&
/// \return PackageInfo
///
/////////////////////////////////////////////////
PackageInfo PackageRepoManager::fetchInfo(const std::string& sPackageId, const std::string& sRepoName) const
{
    if (sRepoName.length())
        return getRemote(sRepoName).fetchInfo(sPackageId);

    if (sPackageId.find('@') != std::string::npos)
    {
        std::pair<std::string,std::string> idAndRemote = getIdAndRemote(sPackageId);
        return getRemote(idAndRemote.second).fetchInfo(idAndRemote.first);
    }

    for (const PackageRepo& remote : m_remotes)
    {
        PackageInfo pkg = remote.find(sPackageId);

        if (pkg.name.length())
            return pkg;
    }

    return PackageInfo();
}


/////////////////////////////////////////////////
/// \brief Find a package by its ID (the
/// filename) and return the information of the
/// package.
///
/// \param sPackageId const std::string&
/// \param sRepoName const std::string&
/// \return PackageInfo
///
/////////////////////////////////////////////////
PackageInfo PackageRepoManager::find(const std::string& sPackageId, const std::string& sRepoName) const
{
    if (sRepoName.length())
        return getRemote(sRepoName).find(sPackageId);

    if (sPackageId.find('@') != std::string::npos)
    {
        std::pair<std::string,std::string> idAndRemote = getIdAndRemote(sPackageId);
        return getRemote(idAndRemote.second).find(idAndRemote.first);
    }

    for (const PackageRepo& remote : m_remotes)
    {
        PackageInfo pkg = remote.find(sPackageId);

        if (pkg.packageId.length())
            return pkg;
    }

    return PackageInfo();
}


/////////////////////////////////////////////////
/// \brief Find package candidates by using only
/// a part of the package id. Does not download
/// any of the necessary data with exception of
/// the repository list.
///
/// \param sPackageId const std::string&
/// \param sRepoName const std::string&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> PackageRepoManager::find_candidates(const std::string& sPackageId, const std::string& sRepoName) const
{
    if (sRepoName.length())
        return getRemote(sRepoName).find_candidates(sPackageId);

    if (sPackageId.find('@') != std::string::npos)
    {
        std::pair<std::string,std::string> idAndRemote = getIdAndRemote(sPackageId);
        return getRemote(idAndRemote.second).find_candidates(idAndRemote.first);
    }

    std::vector<std::string> candidates;

    for (const PackageRepo& remote : m_remotes)
    {
        std::vector<std::string> cands = remote.find_candidates(sPackageId);
        candidates.insert(candidates.end(), cands.begin(), cands.end());
    }

    return candidates;
}


/////////////////////////////////////////////////
/// \brief Resolves all available dependencies of
/// a package. During resolution, packages in the
/// same repository are preferred over any other
/// repository. Only if no match is found in the
/// current repo, then all other repos are
/// checked for the dependency.
///
/// \param sPackageId const std::string&
/// \param sRepoName const std::string&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> PackageRepoManager::resolveDependencies(const std::string& sPackageId, const std::string& sRepoName) const
{
    PackageInfo rootPkg = find(sPackageId, sRepoName);
    std::vector<std::string> vDependencies;

    // Are there any dependencies
    if (rootPkg.requiredPackages.length())
    {
        // Separate all dependencies
        EndlessVector<std::string> pkgs = getAllArguments(rootPkg.requiredPackages);

        for (const std::string& pkg : pkgs)
        {
            // Find each dependency individual; prefer the current repo
            PackageInfo dependency = find(pkg, rootPkg.repoName);

            // If nothing was found, try all other repos
            if (!dependency.packageId.length())
                dependency = find(pkg);

            // If we have a valid match, add the full package ID and resolve the
            // corresponding sub dependencies recursively
            if (dependency.packageId.length())
            {
                vDependencies.push_back(dependency.packageId + "@" + dependency.repoName);
                std::vector<std::string> subDependencies = resolveDependencies(dependency.packageId, dependency.repoName);

                // If there are sub dependencies, then add them only if they are not already
                // part of the dependency list
                if (subDependencies.size())
                {
                    for (const std::string& dep : subDependencies)
                    {
                        if (std::find(vDependencies.begin(), vDependencies.end(), dep) == vDependencies.end())
                            vDependencies.push_back(dep);
                    }
                }
            }
        }
    }

    return vDependencies;
}


/////////////////////////////////////////////////
/// \brief Download a package to a defined
/// location.
///
/// \param sPackageUrl const std::string&
/// \param sTargetFile const std::string&
/// \param sRepoName const std::string&
/// \return bool
///
/////////////////////////////////////////////////
bool PackageRepoManager::download(const std::string& sPackageUrl, const std::string& sTargetFile, const std::string& sRepoName) const
{
    if (sRepoName.length())
        return getRemote(sRepoName).download(sPackageUrl, sTargetFile);

    // This notation works, but is discouraged
    if (sPackageUrl.find('@') != std::string::npos)
    {
        std::pair<std::string,std::string> idAndRemote = getIdAndRemote(sPackageUrl);
        return getRemote(idAndRemote.second).download(idAndRemote.first, sTargetFile);
    }

    // It's better to specify the remote repo instead
    for (const PackageRepo& remote : m_remotes)
    {
        if (remote.download(sPackageUrl, sTargetFile))
            return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief Returns the total number of packages
/// from all remote repositories.
///
/// \return size_t
///
/////////////////////////////////////////////////
size_t PackageRepoManager::size() const
{
    size_t totalSize = 0;

    for (const PackageRepo& remote : m_remotes)
    {
        totalSize += remote.size();
    }

    return totalSize;
}


/////////////////////////////////////////////////
/// \brief Get the full index of all available
/// packages from all available remotes.
///
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
std::vector<std::string> PackageRepoManager::getFullIndex() const
{
    std::vector<std::string> vIndex;
    vIndex.reserve(size());

    for (const PackageRepo& remote : m_remotes)
    {
        const std::map<std::string, PackageVersions>& index = remote.fetchIndex();

        for (const auto& iter : index)
        {
            vIndex.push_back(iter.first + "@" + remote.getRepoName());
        }
    }

    return vIndex;
}




