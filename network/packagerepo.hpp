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

#ifndef PACKAGEREPO_HPP
#define PACKAGEREPO_HPP

#define REPO_LOCATION "http://svn.code.sf.net/p/numere/plugins/repository/"

#include <string>
#include <vector>
#include <ctime>
#include <json/json.h>

/////////////////////////////////////////////////
/// \brief Wrapper class for package repo
/// specific errors.
/////////////////////////////////////////////////
class PackageRepoError : public std::runtime_error
{
public:
    PackageRepoError(const std::string& what) : std::runtime_error(what)
    { }
};

/////////////////////////////////////////////////
/// \brief Defines the set of package files for a
/// specific version of the package.
/////////////////////////////////////////////////
struct PackageFiles
{
    std::string meta;
    std::string file;
};


/////////////////////////////////////////////////
/// \brief Defines the available versions of the
/// package, defaults to the latest.
/////////////////////////////////////////////////
struct PackageVersions
{
    std::map<std::string, PackageFiles> versions;
    const PackageFiles& getLatest() const
    {
        return versions.rbegin()->second;
    }
};


/////////////////////////////////////////////////
/// \brief This structure contains the
/// information about a single package.
/////////////////////////////////////////////////
struct PackageInfo
{
    std::string packageId;
    std::string name;
    std::string version;
    std::string author;
    std::string type;
    std::string requiredVersion;
    std::string requiredPackages;
    std::string description;
    std::string keyWords;
    std::string changeLog;
    std::string license;
    std::string repoName;
    std::string repoUrl;
};


/////////////////////////////////////////////////
/// \brief This class represent a package
/// repository with the necessary user actions to
/// get the available list of packages and their
/// contents.
/////////////////////////////////////////////////
class PackageRepo
{
    private:
        Json::Value m_repoConfig;
        mutable std::map<std::string,PackageVersions> m_index;
        mutable __time64_t m_lastRefreshed;

        bool needsRefresh() const;

    public:
        PackageRepo(const std::string& sRepoConfig = "");
        void connect(const std::string& sRepoConfig);
        bool is_connected() const;
        std::string getRepoName() const;

        const std::map<std::string, PackageVersions>& fetchIndex() const;
        PackageInfo fetchInfo(const std::string& sPackageId) const;
        PackageInfo find(const std::string& pkgId) const;
        std::vector<std::string> find_candidates(const std::string& pkgId) const;
        bool download(const std::string& sPackageUrl, const std::string& sTargetFile) const;
        size_t size() const;
};


/////////////////////////////////////////////////
/// \brief This class manages all delared package
/// repositories and delegates the corresponding
/// tasks.
/////////////////////////////////////////////////
class PackageRepoManager
{
    private:
        std::vector<PackageRepo> m_remotes;

    public:

        bool importConfigs(const std::vector<std::string>& remoteConfigs);

        /////////////////////////////////////////////////
        /// \brief Return a list of defined remote
        /// repositories.
        ///
        /// \return const std::vector<PackageRepo>&
        ///
        /////////////////////////////////////////////////
        const std::vector<PackageRepo>& getRemotes() const
        {
            return m_remotes;
        }

        /////////////////////////////////////////////////
        /// \brief Get a selected remote repository by
        /// name.
        ///
        /// \param sRepoName const std::string&
        /// \return const PackageRepo&
        ///
        /////////////////////////////////////////////////
        const PackageRepo& getRemote(const std::string& sRepoName) const
        {
            for (const PackageRepo& remote : m_remotes)
            {
                if (remote.getRepoName() == sRepoName)
                    return remote;
            }

            throw std::out_of_range(sRepoName + " is not a valid remote repository.");
        }

        PackageInfo fetchInfo(const std::string& sPackageId, const std::string& sRepoName = "") const;
        PackageInfo find(const std::string& sPackageId, const std::string& sRepoName = "") const;
        std::vector<std::string> find_candidates(const std::string& sPackageId, const std::string& sRepoName = "") const;
        std::vector<std::string> resolveDependencies(const std::string& sPackageId, const std::string& sRepoName = "") const;
        bool download(const std::string& sPackageUrl, const std::string& sTargetFile, const std::string& sRepoName = "") const;
        size_t size() const;
        std::vector<std::string> getFullIndex() const;
};

#endif // PACKAGEREPO_HPP

