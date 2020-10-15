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

        // Do empty columns exist?
        for (long long int i = 0; i < _mem->nCols; i++)
        {
            if (_mem->nAppendedZeroes[i] == _mem->nLines)
                break;

            if (i+1 == _mem->nCols)
                return;
        }

        long long int nColTemp = _mem->nCols;
        long long int nLinesTemp = _mem->nLines;
        long long int nEmptyCols = 0;

        // Count the number of empty columns
        for (long long int i = 0; i < nColTemp; i++)
        {
            if (_mem->nAppendedZeroes[i] == nLinesTemp)
                nEmptyCols++;
        }

        // Ensure that we have at least a single
        // non-empty column
        if (nEmptyCols != nColTemp)
        {
            // Create a new memory buffer for the
            // condensed data set
            double** dDataTemp = new double*[_mem->nLines];

            for (long long int i = 0; i < _mem->nLines; i++)
                dDataTemp[i] = new double[_mem->nCols];

            string* sHeadTemp = new string[_mem->nCols];
            long long int* nAppendedTemp = new long long int[_mem->nCols];

            // Copy the contents to the buffer
            for (long long int i = 0; i < _mem->nLines; i++)
            {
                for (long long int j = 0; j < _mem->nCols; j++)
                {
                    dDataTemp[i][j] = _mem->dMemTable[i][j];

                    if (!i)
                    {
                        nAppendedTemp[j] = _mem->nAppendedZeroes[j];
                        sHeadTemp[j] = _mem->sHeadLine[j];
                    }
                }
            }

            // Clear the original table and create a new
            // table
            _mem->clear();
            _mem->Allocate(nLinesTemp, nColTemp-nEmptyCols);

            // Write the non-empty columns in the new
            // table
            for (long long int i = 0; i < _mem->nLines; i++)
            {
                long long int nSkip = 0;

                for (long long int j = 0; j < nColTemp; j++)
                {
                    if (nAppendedTemp[j] == _mem->nLines)
                    {
                        nSkip++;
                        continue;
                    }

                    _mem->dMemTable[i][j-nSkip] = dDataTemp[i][j];

                    if (!i)
                    {
                        _mem->sHeadLine[j-nSkip] = sHeadTemp[j];
                        _mem->nAppendedZeroes[j-nSkip] = nAppendedTemp[j];
                    }
                }
            }

            // Free the buffer
            delete[] sHeadTemp;
            delete[] nAppendedTemp;

            for (long long int i = 0; i < nLinesTemp; i++)
                delete[] dDataTemp[i];

            delete[] dDataTemp;
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
    /// \param _nHeadline int
    /// \return FileHeaderInfo
    ///
    /////////////////////////////////////////////////
    FileHeaderInfo FileAdapter::openFile(std::string _sFile, bool loadToCache, int _nHeadline)
    {
        FileHeaderInfo info;

        // Ensure that the file name is valid
        std::string sFile = ValidFileName(_sFile);

        // If the file seems not to exist and the user did
        // not provide the extension, try to detect it using
        // wildcard
        if (!fileExists(sFile) && (_sFile.find('.') == string::npos || _sFile.find('.') < _sFile.rfind('/')))
            sFile = ValidFileName(_sFile+".*");

        // Get an instance of the desired file type
        GenericFile<double>* file = getFileByType(sFile);

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

        // Get the header information structure
        info = file->getFileHeaderInformation();

        if (!info.nCols || !info.nRows)
        {
            delete file;
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, _sFile, SyntaxError::invalid_position, _sFile);
        }

        Memory* _mem = new Memory(info.nRows, info.nCols);

        // If the dimensions were valid and the internal
        // memory was created, copy the data to this
        // memory
        if (_mem->dMemTable && _mem->sHeadLine)
        {
            // Copy them and delete the file instance
            // afterwards
            _mem->bValidData = true;
            file->getData(_mem->dMemTable);
            file->getColumnHeadings(_mem->sHeadLine);
            delete file;

            _mem->countAppendedZeroes();
            condenseDataSet(_mem);
            _mem->createTableHeaders();
            _mem->setSaveStatus(false);

            // Melt or append the new instance. The
            // melt() member function is responsible
            // for freeing the passed memory.
            if (loadToCache)
                melt(_mem, info.sTableName);
            else
                melt(_mem, "data");
        }
        else
        {
            delete file;
            delete _mem;
            throw SyntaxError(SyntaxError::CANNOT_READ_FILE, _sFile, SyntaxError::invalid_position, _sFile);
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
            generateFileName();
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
        string sFileName = getDataFileName("data");
        size_t nPos = std::string::npos;

        while (sFileName.find('\\') != string::npos)
        {
            sFileName[sFileName.find('\\')] = '/';
        }

        while (sFileName.rfind('/', nPos) != string::npos)
        {
            nPos = sFileName.rfind('/', nPos);

            if (nPos != 0 && nPos-1 != ':')
            {
                size_t nPos_2 = sFileName.rfind('/', nPos-1);

                if (nPos_2 != string::npos)
                {
                    sFileName = sFileName.substr(0,nPos_2+1) + ".." + sFileName.substr(nPos);
                    nPos = nPos_2;
                }
                else
                    break;
            }
            else
                break;
        }

        return sFileName;
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
    /// \return void
    ///
    /////////////////////////////////////////////////
    void FileAdapter::generateFileName()
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
        sTime += ".ndat";
        sOutputFile = sTime;			// Dateinamen an sFileName zuweisen
    }
}

