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

enum ArchiveType
{
    ARCHIVE_AUTO,
    ARCHIVE_TAR,
    ARCHIVE_TAR_GZ,
    ARCHIVE_ZIP,
    ARCHIVE_NONE
};

void packArchive(const std::vector<std::string>& vFileList, const std::string& sTargetFile, ArchiveType type = ARCHIVE_AUTO);
void unpackArchive(const std::string& sArchiveName, const std::string& sTargetPath);

#endif // ARCHIVE_HPP


