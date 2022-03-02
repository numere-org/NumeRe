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

#include "fileadapter.hpp"
#include "../utils/tools.hpp"
#include "../io/logger.hpp"
#include "memory.hpp"

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief FileAdapted default constructor.
    /////////////////////////////////////////////////
    FileAdapter::FileAdapter() : FileSystem()
    {
        bLoadEmptyCols = false;
        bLoadEmptyColsInNextFile = false;
        sOutputFile = "";
        sDataFile = "";
        sPrefix = "data";
        sSavePath = "<savepath>";
    }


    /////////////////////////////////////////////////
    /// \brief This private member function will
    /// return the current date as a timestamp for
    /// the file name.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string FileAdapter::getDate()
    {
        return getTimeStamp(true);
    }


    /////////////////////////////////////////////////
    /// \brief This member function will condense the
    /// data set in the passed Memory instance, i.e.
    /// it will remove unneeded empty columns in the
    /// table.
    ///
    /// \param _mem Memory*
    /// \return void
    ///
    /////////////////////////////////////////////////
    void FileAdapter::condenseDataSet(Memory* _mem)
    {
        if (!_mem || !_mem->isValid())
            return;

        // Shall we ignore empty columns?
        if (bLoadEmptyCols || bLoadEmptyColsInNextFile)
        {
            bLoadEmptyColsInNextFile = false;
            return;
        }

        // Get an iterator to the beginning
        auto iter = _mem->memArray.begin();

        // Remove all empty columns
        while (iter != _mem->memArray.end())
        {
            if (!iter->get() || !iter->get()->size())
                iter = _mem->memArray.erase(iter);
            else
                ++iter;
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function loads the
    /// contents of the selected file to a new Memory
    /// class instance. This instance is either
    /// appended to the already existing instances or
    /// melted with an existing one, if the existing
    /// one has the same name.
    ///
    /// \param _sFile std::string
    /// \param loadToCache bool
    /// \param overrideTarget bool
    /// \param _nHeadline int
    /// \param sTargetTable const std::string&
    /// \return FileHeaderInfo
    ///
    /////////////////////////////////////////////////
    FileHeaderInfo FileAdapter::openFile(std::string _sFile, bool loadToCache, bool overrideTarget, int _nHeadline, const std::string& sTargetTable)
    {
        FileHeaderInfo info;

        // Ensure that the file name is valid
        std::string sFile = ValidFileName(_sFile);

        // If the file seems not to exist and the user did
        // not provide the extension, try to detect it using
        // wildcard
        if (!fileExists(sFile) && (_sFile.find('.') == string::npos || _sFile.find('.') < _sFile.rfind('/')))
            sFile = ValidFileName(_sFile+".*");

        g_logger.info("Loading file '" + _sFile + "'.");

        // Get an instance of the desired file type
        GenericFile* file = getFileByType(sFile);

        // Ensure that the instance is valid
        if (!file)
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, _sFile, SyntaxError::invalid_position, _sFile);

        // Try to read the contents of the file. This may
        // either result in a read error or the read method
        // is not defined for this function
        try
        {
            // Igor binary waves might contain three-dimensional
            // waves. We select the roll-out mode in this case
            if (file->getExtension() == "ibw" && _nHeadline == -1)
                static_cast<IgorBinaryWave*>(file)->useXZSlicing();

            // Read the file
            if (!file->read())
                throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFile, SyntaxError::invalid_position, sFile);
        }
        catch (...)
        {
            delete file;
            throw;
        }

        g_logger.debug("File read. Preparing memory.");

        // Get the header information structure
        info = file->getFileHeaderInformation();

        if (!info.nCols || !info.nRows)
        {
            delete file;
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFile, SyntaxError::invalid_position, sFile);
        }

        Memory* _mem = new Memory();
        _mem->resizeMemory(info.nRows, info.nCols);

        // If the dimensions were valid and the internal
        // memory was created, copy the data to this
        // memory
        if (_mem->memArray.size())
        {
            // Copy them and delete the file instance
            // afterwards
            file->getData(&_mem->memArray);
            delete file;
            g_logger.debug("Data copied.");
            _mem->convert();
            g_logger.debug("Data converted.");
            _mem->shrink();
            condenseDataSet(_mem);
            _mem->createTableHeaders();
            _mem->setSaveStatus(false);

            NumeRe::TableMetaData meta;
            meta.comment = info.sComment;
            meta.source = sFile;

            _mem->setMetaData(meta);

            // Melt or append the new instance. The
            // melt() member function is responsible
            // for freeing the passed memory.
            if (loadToCache)
            {
                if (sTargetTable.length())
                {
                    melt(_mem, sTargetTable, overrideTarget);
                    info.sTableName = sTargetTable;
                }
                else
                    melt(_mem, info.sTableName, overrideTarget);
            }
            else
                melt(_mem, "data");

            g_logger.info("File sucessfully loaded. Data file dimensions = {" + toString(info.nRows) + ", " + toString(info.nCols) + "}");
        }
        else
        {
            delete file;
            delete _mem;
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, sFile, SyntaxError::invalid_position, sFile);
        }

        if (!loadToCache)
        {
            if (sDataFile.length())
                sDataFile += ";" + sFile;
            else
                sDataFile = sFile;
        }

        // Return the file information header for
        // further processing
        return info;
    }


    /////////////////////////////////////////////////
    /// \brief This member function wraps the saving
    /// functionality of the Memory class. The passed
    /// filename is evaluated here and a generic one
    /// is generated, if necessary.
    ///
    /// \param sTable const std::string&
    /// \param _sFileName std::string
    /// \param nPrecision unsigned short
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool FileAdapter::saveFile(const std::string& sTable, std::string _sFileName, unsigned short nPrecision)
    {
        if (!_sFileName.length())
            sOutputFile = generateFileName();
        else
        {
            string sTemp = sPath;
            setPath(sSavePath, false, sExecutablePath);

            sOutputFile = ValidFileName(_sFileName, ".ndat");
            setPath(sTemp, false, sExecutablePath);
        }

        return saveLayer(sOutputFile, sTable, nPrecision);
    }


    /////////////////////////////////////////////////
    /// \brief This member function will return the
    /// file name of the selected table. Will default
    /// to the table name.
    ///
    /// \param sTable const std::string&
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string FileAdapter::getDataFileName(const std::string& sTable) const
    {
        if (sTable != "data")
            return sTable;

        return sDataFile;
    }


    /////////////////////////////////////////////////
    /// \brief This member function will return a
    /// shortened version of the data file name,
    /// where each "/Path/" string part is
    /// transformed to "/../".
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string FileAdapter::getDataFileNameShort() const
    {
        return shortenFileName(getDataFileName("data"));
    }


    /////////////////////////////////////////////////
    /// \brief This member function will return the
    /// output file name, which was used for saving
    /// the last table.
    ///
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string FileAdapter::getOutputFileName() const
    {
        return sOutputFile;
    }


    /////////////////////////////////////////////////
    /// \brief This function may be used to update
    /// the target saving path of this class.
    ///
    /// \param _sPath const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void FileAdapter::setSavePath(const std::string& _sPath)
    {
        sSavePath = _sPath;
    }


    /////////////////////////////////////////////////
    /// \brief This function is used to set a file
    /// prefix for the saving file name.
    ///
    /// \param _sPrefix const std::string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void FileAdapter::setPrefix(const std::string& _sPrefix)
    {
        sPrefix = _sPrefix;
    }


    /////////////////////////////////////////////////
    /// \brief Set, whether empty columns shall be
    /// loaded.
    ///
    /// \param _bLoadEmptyCols bool
    /// \return void
    ///
    /////////////////////////////////////////////////
    void FileAdapter::setbLoadEmptyCols(bool _bLoadEmptyCols)
    {
        bLoadEmptyCols = _bLoadEmptyCols;
    }


    /////////////////////////////////////////////////
    /// \brief Set, whether empty columns shall be
    /// loaded in the next file.
    ///
    /// \param _bLoadEmptyCols bool
    /// \return void
    ///
    /////////////////////////////////////////////////
    void FileAdapter::setbLoadEmptyColsInNextFile(bool _bLoadEmptyCols)
    {
        bLoadEmptyColsInNextFile = _bLoadEmptyCols;
    }


    /////////////////////////////////////////////////
    /// \brief This member function creates a file
    /// name from the file prefix and the time stamp.
    ///
    /// \param sExtension const std::string&
    /// \return std::string
    ///
    /////////////////////////////////////////////////
    std::string FileAdapter::generateFileName(const std::string& sExtension)
    {
        string sTime;

        if (sSavePath.find('"') != string::npos)
            sTime = sSavePath.substr(1, sSavePath.length()-2);
        else
            sTime = sSavePath;

        while (sTime.find('\\') != string::npos)
            sTime[sTime.find('\\')] = '/';

        sTime += "/" + sPrefix + "_";		// Prefix laden
        sTime += getDate();		// Datum aus der Klasse laden
        sTime += sExtension;
        return sTime;
    }
}

