/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2023  Erik Haenel et al.

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

#ifndef ARCHIVE_HPP
#define ARCHIVE_HPP

#include <string>
#include <vector>

namespace Archive
{
    enum Type
    {
        ARCHIVE_AUTO,
        ARCHIVE_TAR,
        ARCHIVE_GZ,
        ARCHIVE_ZLIB,
        ARCHIVE_ZIP,
        ARCHIVE_NONE
    };

    Type detectType(const std::string& sArchiveFileName);
    void pack(const std::vector<std::string>& vFileList, const std::string& sTargetFile, Type type = ARCHIVE_AUTO);
    std::vector<std::string> unpack(const std::string& sArchiveName, const std::string& sTargetPath);
}

#endif // ARCHIVE_HPP


