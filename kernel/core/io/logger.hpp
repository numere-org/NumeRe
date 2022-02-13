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
#include <vector>

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

        enum LogLevel
        {
            LVL_DEBUG,
            LVL_INFO,
            LVL_CMDLINE,
            LVL_WARNING,
            LVL_ERROR,
            LVL_DISABLED
        };
};



/////////////////////////////////////////////////
/// \brief This class is a specialisation of the
/// Logger to run detached, i.e. as a global
/// instance usable form everywhere.
/////////////////////////////////////////////////
class DetachedLogger : public Logger
{
    private:
        std::vector<std::string> m_buffer;
        Logger::LogLevel m_level;

    public:
        DetachedLogger(Logger::LogLevel lvl = Logger::LVL_INFO);
        ~DetachedLogger();

        bool is_buffering() const;
        bool open(const std::string& sLogFile);
        void setLoggingLevel(Logger::LogLevel lvl);

        void push_info(const std::string& sInfo);
        void write_system_information();
        void push_line(Logger::LogLevel lvl, const std::string& sMessage);

        /////////////////////////////////////////////////
        /// \brief Convenience member function.
        ///
        /// \param sMessage const std::string&
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void debug(const std::string& sMessage)
        {
            push_line(Logger::LVL_DEBUG, sMessage);
        }

        /////////////////////////////////////////////////
        /// \brief Convenience member function.
        ///
        /// \param sMessage const std::string&
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void info(const std::string& sMessage)
        {
            push_line(Logger::LVL_INFO, sMessage);
        }

        /////////////////////////////////////////////////
        /// \brief Convenience member function.
        ///
        /// \param sMessage const std::string&
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void cmdline(const std::string& sMessage)
        {
            push_line(Logger::LVL_CMDLINE, sMessage);
        }

        /////////////////////////////////////////////////
        /// \brief Convenience member function.
        ///
        /// \param sMessage const std::string&
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void warning(const std::string& sMessage)
        {
            push_line(Logger::LVL_WARNING, sMessage);
        }

        /////////////////////////////////////////////////
        /// \brief Convenience member function.
        ///
        /// \param sMessage const std::string&
        /// \return void
        ///
        /////////////////////////////////////////////////
        inline void error(const std::string& sMessage)
        {
            push_line(Logger::LVL_ERROR, sMessage);
        }

};


// Declaration of the global instance
extern DetachedLogger g_logger;

#endif // LOGGER_HPP

