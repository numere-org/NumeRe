/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2015  Erik Haenel et al.

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



#ifndef ZIPPP_HPP
#define ZIPPP_HPP

#include <string>
#include <iostream>
#include <windows.h>
#include <tchar.h>
//#include <locale>
//#include <codecvt>
#include "zip.h"
#include "unzip.h"


class Zipfile
{
    private:
        HZIP hZip;
        ZIPENTRY zEntry;
        bool bIsOpen;

    public:
        Zipfile();
        ~Zipfile();
        bool open(const std::string& sZipFilename, int nOpenmode = 0);
        inline bool is_open() const
            {return bIsOpen;}
        bool close();
        bool addFile(const std::string& sFilename);
        std::string getZipContent();
        std::string getZipItem(const std::string& sFilename);
};

#endif

