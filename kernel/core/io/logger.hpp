/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2021  Erik Haenel et al.

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

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <fstream>
#include <string>

/////////////////////////////////////////////////
/// \brief This class represents a simple logging
/// functionality, which might be extended in the
/// future to handle more generic logging
/// formats.
/////////////////////////////////////////////////
class Logger
{
    private:
        std::ofstream m_logFile;
        std::string m_sLogFile;

        bool ensure_open();

    public:
        Logger();
        Logger(const std::string& sLogFile);

        bool open(const std::string& sLogFile);
        void close();
        bool is_open() const;

        void push(const std::string& sMessage);
        void push_line(const std::string& sMessage);
};

#endif // LOGGER_HPP

