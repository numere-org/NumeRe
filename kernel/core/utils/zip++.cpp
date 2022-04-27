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



#include "zip++.hpp"

Zipfile::Zipfile()
{
    hZip = NULL;
    bIsOpen = false;
}

Zipfile::~Zipfile()
{
    if (bIsOpen && hZip != NULL)
    {
        CloseZip(hZip);
        hZip = NULL;
    }
}

bool Zipfile::open(const std::string& sFilename, int nOpenmode)
{
    if (bIsOpen && hZip != NULL)
    {
        CloseZip(hZip);
        hZip = NULL;
    }
    if (!nOpenmode)
        hZip = OpenZip(sFilename.c_str(), 0);
    else
        hZip = CreateZip(sFilename.c_str(), 0);
    if (hZip != NULL)
    {
        bIsOpen = true;
    }
    else
        bIsOpen = false;

    return bIsOpen;
}

bool Zipfile::close()
{
    if (bIsOpen && hZip != NULL)
    {
        CloseZip(hZip);
        hZip = NULL;
        bIsOpen = false;
    }
    return !bIsOpen;
}

bool Zipfile::addFile(const std::string& sFilename)
{
    if (bIsOpen && hZip != NULL)
    {
        return !(bool)ZipAdd(hZip, sFilename.c_str(), sFilename.c_str());
    }
    return false;
}

std::string Zipfile::getZipContent()
{
    if (bIsOpen && hZip != NULL)
    {
        std::string sZipfilecontent = "";
        char cBuffer[1024];
        int i = 0;
        unsigned long int totalsize = 0;
        GetZipItem(hZip, i, &zEntry);
        ZRESULT zRes = ZR_MORE;

        while (zRes == ZR_MORE)
        {
            zRes = UnzipItem(hZip, i, cBuffer, 1024);
            unsigned long int bufsize = 1024;
            if (zRes == ZR_OK)
                bufsize = zEntry.unc_size-totalsize;
            totalsize += bufsize;
            sZipfilecontent.append(cBuffer, bufsize);
        }

        return sZipfilecontent;
    }
    return "";
}


std::string Zipfile::getZipItem(const std::string& sFilename)
{
    if (bIsOpen && hZip != NULL)
    {
        std::string sFilecontent = "";
        //wstring wFilecontent = L"";
        //wchar_t* wBuffer = 0;
        char* cBuffer = 0;
        int nIndex = 0;
        if (FindZipItem(hZip, sFilename.c_str(), true, &nIndex, &zEntry) != ZR_OK)
            return sFilecontent;
        cBuffer = new char[zEntry.unc_size];
        UnzipItem(hZip, nIndex, cBuffer, zEntry.unc_size);

        sFilecontent.append(cBuffer, zEntry.unc_size);
        delete[] cBuffer;

        //sFilecontent = wstring_convert<codecvt_utf8<wchar_t> >().to_bytes(wFilecontent);
        return sFilecontent;
    }
    return "";
}

