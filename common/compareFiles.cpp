/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2022  Erik Haenel et al.

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

#include "compareFiles.hpp"
#include "../externals/dtl/dtl/dtl.hpp"
#include <fstream>
#include <sstream>

/////////////////////////////////////////////////
/// \brief Read the content of a file in binary
/// mode and try to detect lines within the file.
///
/// \param file const std::string&
/// \return std::vector<std::string>
///
/////////////////////////////////////////////////
static std::vector<std::string> getFileContent(const std::string& file)
{
    std::vector<std::string> content;
    std::ifstream fileStream(file, std::ios_base::in | std::ios_base::binary);

    while (fileStream.good() && !fileStream.eof())
    {
        std::string sLine;
        std::getline(fileStream, sLine);
        content.push_back(sLine);
    }

    return content;
}


/////////////////////////////////////////////////
/// \brief Compares two files linewise and in
/// binary mode. The differences are returned as
/// a unified diff.
///
/// \param file1 const std::string&
/// \param file2 const std::string&
/// \return std::string
///
/////////////////////////////////////////////////
std::string compareFiles(const std::string& file1, const std::string& file2)
{
    std::vector<std::string> content1 = getFileContent(file1);
    std::vector<std::string> content2 = getFileContent(file2);

    // Calculate the differences between both files
    dtl::Diff<std::string> diffFile(content1, content2);
    diffFile.compose();

    // Convert the differences into unified form
    diffFile.composeUnifiedHunks();

    // Print the differences to a string stream
    std::ostringstream uniDiff;

    // Get revision identifier
    uniDiff << "--- " << file1 << "\n+++ " << file2 << "\n";

    diffFile.printUnifiedFormat(uniDiff);

    // Return the contents of the stream
    return uniDiff.str();
}


