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

#ifndef INCLUDER_HPP
#define INCLUDER_HPP

#include <string>
#include "../io/filesystem.hpp"
#include "../io/styledtextfile.hpp"

/////////////////////////////////////////////////
/// \brief This class represents a file, which
/// can be included into other files using the
/// \c @ syntax.
/////////////////////////////////////////////////
class Includer : public FileSystem
{
    public:
        enum IncludeType
        {
            INCLUDE_ALL,
            INCLUDE_DEFINES,
            INCLUDE_DECLARATIONS,
            INCLUDE_GLOBALS
        };

    private:
        StyledTextFile* m_include;
        int nIncludeLine;
        IncludeType m_type;

        void openIncludedFile(const std::string& sIncludingString);

    public:
        Includer(const std::string& sIncludingString, const std::string& sSearchPath);
        ~Includer();

        int getCurrentLine() const
        {
            return nIncludeLine;
        }

        IncludeType getIncludedType() const
        {
            return m_type;
        }

        std::string getNextLine();
        bool is_open() const;
        static bool is_including_syntax(const std::string& sLine);
};

#endif // INCLUDER_HPP


