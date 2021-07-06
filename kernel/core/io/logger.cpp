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

#include "logger.hpp"

/////////////////////////////////////////////////
/// \brief Empty default constructor.
/////////////////////////////////////////////////
Logger::Logger()
{
    //
}


/////////////////////////////////////////////////
/// \brief Generic constructor. Will open the
/// target file, if possible.
///
/// \param sLogFile const std::string&
///
/////////////////////////////////////////////////
Logger::Logger(const std::string& sLogFile)
{
    open(sLogFile);
}


/////////////////////////////////////////////////
/// \brief Open the target logging file for
/// writing.
///
/// \param sLogFile const std::string&
/// \return bool
///
/// \warning The logger will not validate file
/// paths.
/////////////////////////////////////////////////
bool Logger::open(const std::string& sLogFile)
{
    if (m_logFile.is_open())
        m_logFile.close();

    m_logFile.open(sLogFile, std::ios_base::out | std::ios_base::app | std::ios_base::ate);
    m_sLogFile = sLogFile;

    return m_logFile.good();
}


/////////////////////////////////////////////////
/// \brief Close the logger stream.
///
/// \return void
///
/////////////////////////////////////////////////
void Logger::close()
{
    m_logFile.close();
}


/////////////////////////////////////////////////
/// \brief Check, whether the logger stream is
/// currently open.
///
/// \return bool
///
/////////////////////////////////////////////////
bool Logger::is_open() const
{
    return m_logFile.is_open();
}


/////////////////////////////////////////////////
/// \brief Ensures that the stream is open and
/// tries to re-open it otherwise.
///
/// \return bool
///
/////////////////////////////////////////////////
bool Logger::ensure_open()
{
    if (!m_logFile.is_open() && m_sLogFile.length())
        m_logFile.open(m_sLogFile, std::ios_base::out | std::ios_base::app | std::ios_base::ate);

    return m_logFile.is_open();
}


/////////////////////////////////////////////////
/// \brief Push a message to the logger stream.
/// Will automatically re-open a file, if the
/// stream had been closed.
///
/// \param sMessage const std::string&
/// \return void
///
/////////////////////////////////////////////////
void Logger::push(const std::string& sMessage)
{
    if (ensure_open())
        m_logFile << sMessage;
}


/////////////////////////////////////////////////
/// \brief Push a line to the logger stream. The
/// stream will automatically append the line
/// termination characters.
///
/// \param sMessage const std::string&
/// \return void
///
/////////////////////////////////////////////////
void Logger::push_line(const std::string& sMessage)
{
    if (ensure_open())
        m_logFile << sMessage << std::endl;
}

