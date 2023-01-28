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

namespace Archive
{
    Type detectType(const std::string& sArchiveFileName)
    {
        std::ifstream file(sArchiveFileName, std::ios_base::binary);

        static const std::string ZIPHEADER("\x50\x4b\x03\x04");
        static const std::string GZHEADER("\x1f\x8b\x08");
        static const std::string TARMAGIC("ustar");
        std::string sExt;

        // Extract extension
        if (sArchiveFileName.find('.') != std::string::npos)
            sExt = toLowerCase(sArchiveFileName.substr(sArchiveFileName.rfind('.')));

        // If file does not already exist, determine the archive
        // type only upon the file extension
        if (!file.good() || file.eof())
        {
            if (sExt == ".zip")
                return ARCHIVE_ZIP;
            else if (sExt == ".tar")
                return ARCHIVE_TAR;
            else if (sExt == ".gz")
                return ARCHIVE_GZ;
            else if (sExt == ".zz")
                return ARCHIVE_ZLIB;

            return ARCHIVE_NONE;
        }

        char magicNumber[6] = {0,0,0,0,0,0};

        file.read(magicNumber, 4);
        g_logger.info("magicNumber = '" + std::string(magicNumber) + "'");

        // Test for zip and gzip
        if (magicNumber == ZIPHEADER)
            return ARCHIVE_ZIP;
        else if (GZHEADER == std::string(magicNumber).substr(0, 3))
            return ARCHIVE_GZ;

        // try to detect TAR
        file.seekg(257, std::ios_base::beg);
        file.read(magicNumber, TARMAGIC.length());
        g_logger.info("magicNumber = '" + std::string(magicNumber) + "'");

        if (magicNumber == TARMAGIC)
            return ARCHIVE_TAR;

        return ARCHIVE_NONE;
    }


    static std::string getGZipFileName(const std::string& sArchiveFileName)
    {
        std::ifstream gzip(sArchiveFileName, std::ios_base::binary);

        if (!gzip.good() || gzip.eof())
            return "";

        gzip.seekg(3, std::ios_base::beg);
        std::uint8_t flg;
        gzip >> flg;

        if (flg & 8)
        {
            gzip.seekg(10, std::ios_base::beg);

            if (flg & 4)
            {
                uint16_t xlen;
                gzip >> xlen;
                gzip.seekg(xlen, std::ios_base::cur);
            }

            std::string fileName;
            char c;
            gzip >> c;

            while (c != 0)
            {
                fileName += c;
                gzip >> c;
            }

            return fileName;
        }

        return "";
    }

    void pack(const std::vector<std::string>& vFileList, const std::string& sTargetFile, Type type)
    {
        if (type == ARCHIVE_AUTO)
            type = detectType(sTargetFile);

        // TODO Throw an error if type undetectable

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
        else if (type == ARCHIVE_TAR)
        {
            wxFFileOutputStream out(sTargetFile);
            wxTarOutputStream outtar(out);

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
                            wxTarEntry* newEntry = new wxTarEntry(vFiles[j].substr(vFileList[i].find_last_of("/\\")+1));

                            outtar.PutNextEntry(newEntry);
                            outtar.Write(file);
                            outtar.CloseEntry();
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
                    wxTarEntry* newEntry = new wxTarEntry(vFileList[i].substr(vFileList[i].find_last_of("/\\")+1));

                    outtar.PutNextEntry(newEntry);
                    outtar.Write(file);
                    outtar.CloseEntry();
                }
            }
        }
        else if (type == ARCHIVE_GZ)
        {
            std::string sFile = vFileList.front();
            bool tempTar = false;

            // Multiple files have to be tar'ed first
            if (vFileList.size() > 1 || !fileExists(sFile))
            {
                sFile = sTargetFile.substr(0, sTargetFile.rfind('.')) + ".tar";
                tempTar = true;
                pack(vFileList, sFile, ARCHIVE_TAR);
            }

            wxFFileOutputStream out(sTargetFile);
            wxZlibOutputStream outzlib(out, -1, wxZLIB_GZIP);

            g_logger.info("Including file: " + sFile);
            wxFFileInputStream file(sFile);
            outzlib.Write(file);

            if (tempTar)
                remove(sFile.c_str());
        }
    }


    std::vector<std::string> unpack(const std::string& sArchiveName, const std::string& sTargetPath)
    {
        Type type = detectType(sArchiveName);

        // TODO Throw an error if type undetectable
        if (type == ARCHIVE_ZIP)
            g_logger.info("ZIP detected.");
        else if (type == ARCHIVE_TAR)
            g_logger.info("TAR detected.");
        else if (type == ARCHIVE_GZ)
            g_logger.info("GZIP detected.");
        else if (type == ARCHIVE_ZLIB)
            g_logger.info("ZLIB detected.");

        FileSystem& _fSys = NumeReKernel::getInstance()->getFileSystem();
        std::vector<std::string> vFiles;

        if (type == ARCHIVE_ZIP)
        {
            wxFFileInputStream in(sArchiveName);
            wxZipInputStream zip(in);
            std::unique_ptr<wxZipEntry> entry;

            while (entry.reset(zip.GetNextEntry()), entry.get() != nullptr)
            {
                std::string entryName = replacePathSeparator(entry->GetName().ToStdString());

                if (sTargetPath.length())
                {
                    entryName = _fSys.ValidizeAndPrepareName(sTargetPath + "/" + entryName, "");

                    g_logger.info("Entry name: " + entryName);
                    wxFileOutputStream stream(sTargetPath + "/" + entry->GetName());
                    zip.Read(stream);
                }
                else
                    vFiles.push_back(entryName);
            }
        }
        else if (type == ARCHIVE_TAR)
        {
            wxFFileInputStream in(sArchiveName);
            wxTarInputStream tar(in);
            std::unique_ptr<wxTarEntry> entry;

            while (entry.reset(tar.GetNextEntry()), entry.get() != nullptr)
            {
                if (entry->IsDir())
                    continue;

                std::string entryName = replacePathSeparator(entry->GetName().ToStdString());

                if (sTargetPath.length())
                {
                    entryName = _fSys.ValidizeAndPrepareName(sTargetPath + "/" + entryName, "");

                    g_logger.info("Entry name: " + entryName);
                    wxFileOutputStream stream(sTargetPath + "/" + entry->GetName());
                    tar.Read(stream);
                }
                else
                    vFiles.push_back(entryName);
            }
        }
        else if (type == ARCHIVE_GZ || type == ARCHIVE_ZLIB)
        {
            wxFFileInputStream in(sArchiveName);
            wxZlibInputStream zlib(in);

            std::string sUnpackedName = getGZipFileName(sArchiveName);

            if (sTargetPath.length())
            {
                if (!sUnpackedName.length())
                {
                    sUnpackedName = sArchiveName.substr(0, sArchiveName.rfind('.'))+".tar";
                    sUnpackedName = _fSys.ValidizeAndPrepareName(sTargetPath + "/" + sUnpackedName.substr(sUnpackedName.rfind('/')+1), "");
                }
                else
                    sUnpackedName = _fSys.ValidizeAndPrepareName(sTargetPath + "/" + sUnpackedName, "");

                g_logger.info("Entry name: " + sUnpackedName);

                wxFileOutputStream stream(sUnpackedName);
                zlib.Read(stream);
            }
            else
                vFiles.push_back(sUnpackedName);
        }

        return vFiles;
    }

}
