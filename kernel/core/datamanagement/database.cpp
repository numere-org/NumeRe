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

#include <fstream>

#include "database.hpp"
#include "../ui/error.hpp"
#include "../utils/tools.hpp"


using namespace std;

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief This member function reads the
    /// contents of a database file linewise to a
    /// vector.
    ///
    /// \return vector<string>
    ///
    /////////////////////////////////////////////////
    vector<string> DataBase::getDBFileContent()
    {
        vector<string> vDBEntries;
        string sLine;
        string sFile = ValidFileName(m_dataBaseFile, ".ndb");
        ifstream fDB;

        // open the ifstream
        fDB.open(sFile.c_str());
        if (fDB.fail())
            return vDBEntries;

        // Read the entire file
        while (!fDB.eof())
        {
            // Get a line and strip the spaces
            getline(fDB, sLine);
            StripSpaces(sLine);

            // If the line has a length and doesn't start with the "#", then append it to the vector
            if (sLine.length())
            {
                if (sLine[0] == '#')
                    continue;
                vDBEntries.push_back(sLine);
            }
        }

        // Return the read data base
        return vDBEntries;
    }


    /////////////////////////////////////////////////
    /// \brief This function opens up a NumeRe Data
    /// base file and reads its contents to the
    /// internal vector<vector> matrix. Each field of
    /// the matrix contains a field of the database.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void DataBase::readDataBase()
    {
        // Read the database to memory
        vector<string> vDBEntries = getDBFileContent();
        m_dataBase = vector<vector<string>>(vDBEntries.size(), vector<string>());

        // Go through the database in memory
        for (unsigned int i = 0; i < vDBEntries.size(); i++)
        {
            // If no field separator was found, simply append the overall line
            if (vDBEntries[i].find('~') == string::npos)
                m_dataBase[i].push_back(vDBEntries[i]);
            else
            {
                // Split the database fields at the separators
                while (vDBEntries[i].find('~') != string::npos)
                {
                    // Append the next field
                    if (vDBEntries[i].substr(0, vDBEntries[i].find('~')).size())
                        m_dataBase[i].push_back(vDBEntries[i].substr(0, vDBEntries[i].find('~')));

                    // Erase the already appended field
                    vDBEntries[i].erase(0, vDBEntries[i].find('~') + 1);
                }

                // If there's one field remaining, append it here
                if (vDBEntries[i].size())
                    m_dataBase[i].push_back(vDBEntries[i]);
            }
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function will return the
    /// id of the searched record (value in the first
    /// column of the matrix) or create a new record,
    /// if the searched one may not be found.
    ///
    /// \param sRecord const string&
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t DataBase::findOrCreateRecord(const string& sRecord)
    {
        // Search for the record and return its ID
        for (size_t i = 0; i < m_dataBase.size(); i++)
        {
            if (m_dataBase[i][0] == sRecord)
                return i;
        }

        // Create a new one, because the searched
        // one is not found
        m_dataBase.push_back(vector<string>());
        return m_dataBase.size()-1;
    }


    /////////////////////////////////////////////////
    /// \brief The default constructor will
    /// initialize the FileSystem base class using
    /// the information from the kernel.
    ///
    /// \remark This class cannot be instantiated
    /// without an already running NumeReKernel
    /// instance.
    /////////////////////////////////////////////////
    DataBase::DataBase() : FileSystem()
    {
        initializeFromKernel();
    }


    /////////////////////////////////////////////////
    /// \brief This constructor will open the passed
    /// database file and read its contents to
    /// memory.
    ///
    /// \param sDataBaseFile const string&
    ///
    /////////////////////////////////////////////////
    DataBase::DataBase(const string& sDataBaseFile) : DataBase()
    {
        m_dataBaseFile = sDataBaseFile;
        readDataBase();
    }


    /////////////////////////////////////////////////
    /// \brief This is the copy constructor.
    ///
    /// \param data const DataBase&
    ///
    /////////////////////////////////////////////////
    DataBase::DataBase(const DataBase& data) : DataBase()
    {
        m_dataBase = data.m_dataBase;
        m_dataBaseFile = data.m_dataBaseFile;
    }


    /////////////////////////////////////////////////
    /// \brief This constructor instantiates this
    /// class using a vector<string> as the first
    /// column.
    ///
    /// \param vDataColumn const vector<string>&
    ///
    /////////////////////////////////////////////////
    DataBase::DataBase(const vector<string>& vDataColumn) : DataBase()
    {
        m_dataBaseFile = "Copied database";
        m_dataBase = vector<vector<string>>(vDataColumn.size(), vector<string>(1, ""));

        for (size_t i = 0; i < vDataColumn.size(); i++)
            m_dataBase[i][0] = vDataColumn[i];
    }


    /////////////////////////////////////////////////
    /// \brief This member function will use the
    /// passed database file name to update its
    /// internal contents (i.e. if a new version of
    /// data is available). This function can also be
    /// used to read a database, if the internal
    /// database is still empty.
    ///
    /// \param sDataBaseFile const string&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void DataBase::addData(const string& sDataBaseFile)
    {
        // If the internal database is still empty, use the
        // passed file as default database
        if (!m_dataBase.size())
        {
            m_dataBaseFile = sDataBaseFile;
            readDataBase();
            return;
        }

        // Read the database to memory
        DataBase data(sDataBaseFile);

        // Go through the database in memory
        for (size_t i = 0; i < data.size(); i++)
        {
            // If no field separator was found, simply append the overall line
            m_dataBase[findOrCreateRecord(data[i][0])] = data[i];
        }
    }


    /////////////////////////////////////////////////
    /// \brief This member function will return the
    /// contents of the selected database field, or
    /// an empty string, if the field does not exist.
    ///
    /// \param i size_t
    /// \param j size_t
    /// \return string
    ///
    /////////////////////////////////////////////////
    string DataBase::getElement(size_t i, size_t j) const
    {
        if (i < m_dataBase.size() && j < m_dataBase[i].size())
            return m_dataBase[i][j];

        return "";
    }


    /////////////////////////////////////////////////
    /// \brief This member function will return the
    /// whole selected column of the database as a
    /// vector<string>. Some elements in the returned
    /// vector might be empty strings, which
    /// indicates that the read field did not exist.
    ///
    /// \param j size_t
    /// \return vector<string>
    ///
    /////////////////////////////////////////////////
    vector<string> DataBase::getColumn(size_t j) const
    {
        vector<string> vColumn(m_dataBase.size(), "");

        // Fill the vector with the elements of the
        // selected column
        for (size_t i = 0; i < m_dataBase.size(); i++)
            vColumn[i] = getElement(i, j);

        return vColumn;
    }


    /////////////////////////////////////////////////
    /// \brief This is an overload of the array
    /// access operator, which can be used with read
    /// and write permissions. Use
    /// DataBase::getElement() in const cases.
    ///
    /// \param i size_t
    /// \return vector<string>&
    ///
    /////////////////////////////////////////////////
    vector<string>& DataBase::operator[](size_t i)
    {
        if (i < m_dataBase.size())
            return m_dataBase[i];

        throw SyntaxError(SyntaxError::TOO_FEW_LINES, m_dataBaseFile, SyntaxError::invalid_position);
    }


    /////////////////////////////////////////////////
    /// \brief This is an overload of the assignment
    /// operator.
    ///
    /// \param data const DataBase&
    /// \return DataBase&
    ///
    /////////////////////////////////////////////////
    DataBase& DataBase::operator=(const DataBase& data)
    {
        m_dataBase = data.m_dataBase;
        m_dataBaseFile = data.m_dataBaseFile;

        return *this;
    }

    /////////////////////////////////////////////////
    /// \brief This member function can be used to
    /// select and random record.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t DataBase::randomRecord() const
    {
        srand(time(NULL));
        size_t nthRecord = (rand() % m_dataBase.size());

        if (nthRecord >= m_dataBase.size())
            nthRecord = m_dataBase.size() - 1;

        return nthRecord;
    }


    /////////////////////////////////////////////////
    /// \brief This member function can be used to
    /// search a string in the managed database. The
    /// return value is a map with the record id as
    /// key and a list of (field-based) occurences as
    /// a vector. If only the first occurence in each
    /// record is searched, set the boolean flag to
    /// true.
    ///
    /// \param _sSearchString const string&
    /// \param findOnlyFirst bool
    /// \return map<size_t,vector<size_t>>
    ///
    /// \remark The search is case insensitive.
    /////////////////////////////////////////////////
    map<size_t,vector<size_t>> DataBase::find(const string& _sSearchString, bool findOnlyFirst) const
    {
        map<size_t,vector<size_t>> mMatches;

        // Transform the searched string to lower case
        string lower = toLowerCase(_sSearchString);

        if (lower.length() && lower != " ")
        {
            // Search through the database
            for (size_t i = 0; i < m_dataBase.size(); i++)
            {
                for (size_t j = 0; j < m_dataBase[i].size(); j++)
                {
                    if (toLowerCase(m_dataBase[i][j]).find(lower) != string::npos)
                    {
                        // Store the match (will create a new vector automatically)
                        mMatches[i].push_back(j);

                        if (findOnlyFirst)
                            break;
                    }
                }
            }
        }

        return mMatches;
    }


    /////////////////////////////////////////////////
    /// \brief This member function will search
    /// multiple search strings in the database and
    /// returns a map, where the key is a percentage
    /// value for the relevance of the corresponding
    /// records and a list of records as value. The
    /// weighting of each column can be passed as
    /// vector. Otherwise all columns are weighted
    /// identical.
    ///
    /// \param _sSearchString const string&
    /// \param vWeighting vector<double>
    /// \return map<double,vector<size_t>>
    ///
    /////////////////////////////////////////////////
    map<double,vector<size_t>> DataBase::findRecordsUsingRelevance(const string& _sSearchString, vector<double> vWeighting) const
    {
        vector<string> vKeyWords;
        string sTemp = _sSearchString;
        map<double,vector<size_t>> mRelevance;

        if (sTemp.back() != ' ')
            sTemp += " ";

        // Create a keyword list to search independently
        do
        {
            vKeyWords.push_back(sTemp.substr(0, sTemp.find(" ")));
            sTemp.erase(0, sTemp.find(" ")+1);
        }
        while (sTemp.length());

        // Create the result map with the first
        // keyword
        map<size_t,vector<size_t>> mMatches = find(vKeyWords.front());

        // Search all following keywords and append
        // their results to the existing map
        for (size_t i = 1; i < vKeyWords.size(); i++)
        {
            map<size_t,vector<size_t>> mCurMatches = find(vKeyWords[i]);

            for (auto iter = mCurMatches.begin(); iter != mCurMatches.end(); ++iter)
            {
                // Will automatically create a new vector,
                // if it does not exist
                mMatches[iter->first].insert(mMatches[iter->first].end(), iter->second.begin(), iter->second.end());
            }
        }

        // Prepare the weighting, if necessary
        if (!vWeighting.size())
            vWeighting.assign(getCols(), 1.0);

        // Calculate the relevance values and append
        // their corresponding record IDs
        for (auto iter = mMatches.begin(); iter != mMatches.end(); ++iter)
        {
            double weight = 0.0;

            for (size_t i = 0; i < iter->second.size(); i++)
            {
                weight += vWeighting[iter->second[i]];
            }

            mRelevance[weight].push_back(iter->first);
        }

        return mRelevance;
    }
}

