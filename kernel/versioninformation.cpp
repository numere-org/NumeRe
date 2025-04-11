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

#include "versioninformation.hpp"
#include "core/version.h"
#include "core/utils/stringtools.hpp"

#ifdef NIGHTLY
// Solution extracted from https://stackoverflow.com/a/64718070
constexpr unsigned int compileYear = (__DATE__[7] - '0') * 1000 + (__DATE__[8] - '0') * 100 + (__DATE__[9] - '0') * 10 + (__DATE__[10] - '0');;
constexpr unsigned int compileMonth = (__DATE__[0] == 'J') ? ((__DATE__[1] == 'a') ? 1 : ((__DATE__[2] == 'n') ? 6 : 7))    // Jan, Jun or Jul
                                : (__DATE__[0] == 'F') ? 2                                                              // Feb
                                : (__DATE__[0] == 'M') ? ((__DATE__[2] == 'r') ? 3 : 5)                                 // Mar or May
                                : (__DATE__[0] == 'A') ? ((__DATE__[1] == 'p') ? 4 : 8)                                 // Apr or Aug
                                : (__DATE__[0] == 'S') ? 9                                                              // Sep
                                : (__DATE__[0] == 'O') ? 10                                                             // Oct
                                : (__DATE__[0] == 'N') ? 11                                                             // Nov
                                : (__DATE__[0] == 'D') ? 12                                                             // Dec
                                : 0;
constexpr unsigned int compileDay = (__DATE__[4] == ' ') ? (__DATE__[5] - '0') : (__DATE__[4] - '0') * 10 + (__DATE__[5] - '0');

constexpr char NIGHTLYVER[] = // YYMMDD
{   (compileYear % 100)/10 + '0', compileYear % 10 + '0', // YY
    compileMonth/10 + '0', compileMonth % 10 + '0', // MM
    compileDay/10 + '0', compileDay % 10 + '0', // DD
    0
};

#endif // NIGHTLY

/* --> STATUS: Versionsname des Programms; Aktuell "Ampere", danach "Angstroem". Ab 1.0 Namen mit "B",
 *     z.B.: Biot(1774), Boltzmann(1844), Becquerel(1852), Bragg(1862), Bohr(1885), Brillouin(1889),
 *     de Broglie(1892, Bose(1894), Bloch(1905), Bethe(1906)) <--
 * --> de Coulomb(1736), Carnot(1796), P.Curie(1859), M.Curie(1867), A.Compton(1892), Cherenkov(1904),
 *     Casimir(1909), Chandrasekhar(1910), Chamberlain(1920), Cabibbo(1935) <--
 */

/////////////////////////////////////////////////
/// \brief Return the current build date as
/// sys_time_point.
///
/// \return sys_time_point
///
/////////////////////////////////////////////////
sys_time_point getBuildDate()
{
    time_stamp ts;
#ifdef NIGHTLY
    ts.m_ymd = date::year(compileYear) / date::month(compileMonth) / date::day(compileDay);
#else
    ts.m_ymd = date::year(std::atoi(AutoVersion::YEAR)) / date::month(std::atoi(AutoVersion::MONTH)) / date::day(std::atoi(AutoVersion::DATE));
#endif
    return getTimePointFromTimeStamp(ts);
}


/////////////////////////////////////////////////
/// \brief Return the current build date as
/// string.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string printBuildDate()
{
    return toString(getBuildDate(), GET_ONLY_DATE);
}


/////////////////////////////////////////////////
/// \brief Get only the build year (for copyright
/// mentions and similar).
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string getBuildYear()
{
#ifdef NIGHTLY
    return toString((int)compileYear);
#else
    return AutoVersion::YEAR;
#endif
}


/////////////////////////////////////////////////
/// \brief Return the human-readable version of
/// NumeRe.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string getVersion()
{
#ifdef __GNUWIN64__
#  ifdef DO_LOG
    return toString((int)AutoVersion::MAJOR) + "." + toString((int)AutoVersion::MINOR) + "." + toString((int)AutoVersion::BUILD) + " \"" + getVersionName() + "\" (x64-DEBUG)";
#  else
    return toString((int)AutoVersion::MAJOR) + "." + toString((int)AutoVersion::MINOR) + "." + toString((int)AutoVersion::BUILD) + " \"" + getVersionName() + "\" (x64)";
#  endif
#else
#  ifdef DO_LOG
    return toString((int)AutoVersion::MAJOR) + "." + toString((int)AutoVersion::MINOR) + "." + toString((int)AutoVersion::BUILD) + " \"" + getVersionName() + "\" (x86-DEBUG)";
#  else
    return toString((int)AutoVersion::MAJOR) + "." + toString((int)AutoVersion::MINOR) + "." + toString((int)AutoVersion::BUILD) + " \"" + getVersionName() + "\" (x86)";
#  endif
#endif
}


/////////////////////////////////////////////////
/// \brief Get the version name.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string getVersionName()
{
#ifdef NIGHTLY
    return "Nightly/" + std::string(NIGHTLYVER);
#else
    return AutoVersion::STATUS;
#endif // NIGHTLY
}


/////////////////////////////////////////////////
/// \brief Get the Ubuntu style subversion.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string getSubVersion()
{
#ifdef NIGHTLY
    return AutoVersion::UBUNTU_VERSION_STYLE + ("n" + std::string(NIGHTLYVER));
#else
    return AutoVersion::UBUNTU_VERSION_STYLE;
#endif // NIGHTLY
}


/////////////////////////////////////////////////
/// \brief Get the full version string containing
/// the subversion.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string getFullVersion()
{
    return toString((int)AutoVersion::MAJOR) + "."
            + toString((int)AutoVersion::MINOR) + "."
            + toString((int)AutoVersion::BUILD) + "."
#ifdef NIGHTLY
            + toString((int)(std::stod(AutoVersion::UBUNTU_VERSION_STYLE)*100)) + NIGHTLYVER;
#else
            + toString((int)(std::stod(AutoVersion::UBUNTU_VERSION_STYLE)*100));
#endif // NIGHTLY
}


/////////////////////////////////////////////////
/// \brief Get the full version string with the
/// architecture information appended.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string getFullVersionWithArchitecture()
{
    return getFullVersion()
#ifdef NIGHTLY
            + "n"
#endif // NIGHTLY
#ifdef __GNUWIN64__
            + "-x64"
#endif
            ;
}


/////////////////////////////////////////////////
/// \brief Get the installer file version.
///
/// \return std::string
///
/////////////////////////////////////////////////
std::string getFileVersion()
{
    return toString((int)AutoVersion::MAJOR) + toString((int)AutoVersion::MINOR) + toString((int)AutoVersion::BUILD)
            + (std::string(AutoVersion::STATUS_SHORT).find("rc") != std::string::npos ? AutoVersion::STATUS_SHORT : "")
#ifdef NIGHTLY
            + "_n" + NIGHTLYVER
#endif // NIGHTLY
#ifdef __GNUWIN64__
            + "_x64"
#endif
        ;
}


/////////////////////////////////////////////////
/// \brief Get the version as floating point
/// number.
///
/// \return double
///
/////////////////////////////////////////////////
double getFloatingPointVersion()
{
#ifdef NIGHTLY
    return 100.0*AutoVersion::MAJOR+10.0*AutoVersion::MINOR + AutoVersion::BUILD + std::atof(AutoVersion::UBUNTU_VERSION_STYLE) / 100.0 + std::atof(NIGHTLYVER) / 10000000000.0;
#else
    return 100.0*AutoVersion::MAJOR+10.0*AutoVersion::MINOR + AutoVersion::BUILD + std::atof(AutoVersion::UBUNTU_VERSION_STYLE) / 100.0;
#endif // NIGHTLY
}


