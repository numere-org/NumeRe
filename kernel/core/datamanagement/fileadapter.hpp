/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2020  Erik Haenel et al.

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

#ifndef FILEADAPTER_HPP
#define FILEADAPTER_HPP

#include "../io/file.hpp"
#include "../io/filesystem.hpp"
#include "../settings.hpp"

#include <string>

class Memory;

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief This class represents the file input
    /// and output adapter for the MemoryManager
    /// class, separating memory management from file
    /// handling.
    /////////////////////////////////////////////////
    class FileAdapter : public FileSystem
    {
        protected:
            std::string sOutputFile;
            std::string sPrefix;
            std::string sSavePath;
            std::string sDataFile;
            bool bLoadEmptyCols;
            bool bLoadEmptyColsInNextFile;

            std::string getDate();
            void condenseDataSet(Memory* _mem);
            virtual bool saveLayer(std::string _sFileName, const std::string& _sCache, unsigned short nPrecision, std::string sExt = "") = 0;

        public:
            FileAdapter();
            virtual ~FileAdapter() {}

            FileHeaderInfo openFile(std::string _sFile, bool loadToCache = false, bool overrideTarget = false, int _nHeadline = 0, const std::string& sTargetTable = "", std::string sFileFormat = "");
            bool saveFile(const std::string& sTable, std::string _sFileName, unsigned short nPrecision = 7, std::string sFileFormat = "");
            std::string getDataFileName(const std::string& sTable) const;
            std::string getDataFileNameShort() const;
            std::string getOutputFileName() const;
            void setSavePath(const std::string& _sPath);
            void setPrefix(const std::string& _sPrefix);
            void setbLoadEmptyCols(bool _bLoadEmptyCols);
            void setbLoadEmptyColsInNextFile(bool _bLoadEmptyCols);
            std::string generateFileName(const std::string& sExtension = ".ndat");
            virtual void melt(Memory* _mem, const std::string& sTable, bool overrideTarget = false) = 0;
    };
}

#endif // FILEADAPTER_HPP

