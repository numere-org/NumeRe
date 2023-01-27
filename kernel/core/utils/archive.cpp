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

#include "archive.hpp"
#include "stringtools.hpp"
#include "../io/logger.hpp"

#include <wx/zipstrm.h>
#include <wx/tarstrm.h>
#include <wx/zstream.h>
#include <wx/wfstream.h>
#include <wx/stream.h>
#include <wx/dir.h>

#include <fstream>
#include <cstring>
#include <memory>

#include "../../kernel.hpp"

static ArchiveType detectArchiveType(const std::string& sArchiveFileName)
{
    std::ifstream file(sArchiveFileName, std::ios_base::binary);

    static const char ZIPHEADER[] = {0x50, 0x4b, 0x03, 0x04, 0};
    static const char GZHEADER[] = {0x1f, 0x8b, 0x08, 0};

    if (!file.good() || file.eof())
    {
        if (sArchiveFileName.find('.') != std::string::npos && toLowerCase(sArchiveFileName.substr(sArchiveFileName.rfind('.'))) == ".zip")
            return ARCHIVE_ZIP;
        else if (sArchiveFileName.find('.') != std::string::npos && toLowerCase(sArchiveFileName.substr(sArchiveFileName.rfind('.'))) == ".tar")
            return ARCHIVE_TAR;

        return ARCHIVE_NONE;
    }

    char magicNumber[5] = {0,0,0,0,0};

    file.read(magicNumber, 4);
    g_logger.info("magicNumber = '" + std::string(magicNumber) + "'");

    if (strcmp(magicNumber, ZIPHEADER) == 0)
        return ARCHIVE_ZIP;
    else if (strcmp(magicNumber, GZHEADER) == 0)
        return ARCHIVE_TAR_GZ;

    return ARCHIVE_NONE;
}


void packArchive(const std::vector<std::string>& vFileList, const std::string& sTargetFile, ArchiveType type)
{
    if (type == ARCHIVE_AUTO)
        type = detectArchiveType(sTargetFile);

    const Settings& _option = NumeReKernel::getInstance()->getSettings();

    if (type == ARCHIVE_ZIP)
    {
        wxFFileOutputStream out(sTargetFile);
        wxZipOutputStream outzip(out, 6);

        for (size_t i = 0; i < vFileList.size(); i++)
        {
            if (!fileExists(vFileList[i]))
            {
                // Handle the recursion
                g_logger.info("Including directory: " + vFileList[i]);
                std::string sDirectory = vFileList[i] + "/*";
                std::vector<std::string> vFiles = getFileList(sDirectory, _option, 1);

                while (vFiles.size() || getFolderList(sDirectory, _option).size() > 2)
                {
                    for (size_t j = 0; j < vFiles.size(); j++)
                    {
                        // Files are simple packed together without additional paths
                        g_logger.info("Including file: " + vFiles[j]);
                        wxFFileInputStream file(vFiles[j]);
                        wxZipEntry* newEntry = new wxZipEntry(vFiles[j].substr(vFileList[i].find_last_of("/\\")+1));

                        outzip.PutNextEntry(newEntry);
                        outzip.Write(file);
                        outzip.CloseEntry();
                    }

                    sDirectory += "/*";
                    vFiles = getFileList(sDirectory, _option, 1);
                }
            }
            else
            {
                // Files are simple packed together without additional paths
                g_logger.info("Including file: " + vFileList[i]);
                wxFFileInputStream file(vFileList[i]);
                wxZipEntry* newEntry = new wxZipEntry(vFileList[i].substr(vFileList[i].find_last_of("/\\")+1));

                outzip.PutNextEntry(newEntry);
                outzip.Write(file);
                outzip.CloseEntry();
            }
        }
    }
}


void unpackArchive(const std::string& sArchiveName, const std::string& sTargetPath)
{
    ArchiveType type = detectArchiveType(sArchiveName);
    FileSystem& _fSys = NumeReKernel::getInstance()->getFileSystem();

    if (type == ARCHIVE_ZIP)
    {
        wxFFileInputStream in(sArchiveName);
        wxZipInputStream zip(in);
        std::unique_ptr<wxZipEntry> entry;

        while (entry.reset(zip.GetNextEntry()), entry.get() != nullptr)
        {
            std::string entryName = entry->GetName().ToStdString();
            entryName = _fSys.ValidizeAndPrepareName(sTargetPath + "/" + entryName, "");

            g_logger.info("Entry name: " + entryName);
            wxFileOutputStream stream(sTargetPath + "/" + entry->GetName());
            zip.Read(stream);
        }
    }
}


