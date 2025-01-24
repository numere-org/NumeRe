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

/////////////////////////////////////////////////
/// \brief This structure contains the
/// information about a single package.
/////////////////////////////////////////////////
struct PackageInfo
{
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
        std::string m_repoUrl;
        std::vector<std::string> m_list;
        __time64_t m_lastRefreshed;

    public:
        PackageRepo(const std::string& sRepoUrl = "");
        void connect(const std::string& sRepoUrl);
        bool is_connected() const;

        const std::vector<std::string>& fetchList();
        PackageInfo fetchInfo(const std::string& sPackageUrl);
        PackageInfo find(const std::string& pkgId);
        bool download(const std::string& sPackageUrl, const std::string& sTargetFile);
};

#endif // PACKAGEREPO_HPP

