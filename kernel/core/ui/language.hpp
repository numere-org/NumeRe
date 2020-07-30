/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2016  Erik Haenel et al.

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

#ifndef LANGUAGE_HPP
#define LANGUAGE_HPP

#include <string>
#include <map>
#include <vector>
#include <iostream>
#include <fstream>


#include "../filesystem.hpp"

/////////////////////////////////////////////////
/// \brief This class handles the internal
/// language system and returns the language
/// strings of the selected language based upon
/// the language files.
/////////////////////////////////////////////////
class Language : public FileSystem
{
    private:
        map<string,string> mLangStrings;
        string sYES;
        string sNO;

        map<string,string> getLangFileContent(const string& sFile);
        void loadAndInsert(const string& sLanguageFileName);

    public:
        Language();
        Language(const Language& _lang);
        void loadStrings(bool bloadUserFiles = true);
        void addToLanguage(const map<string,string>& _langstrings);
        string getKey(const string& sMessage) const;
        string get(const string& sMessage, const vector<string>& vTokens) const; //_lang.get("GREETING",vTokens);

        /////////////////////////////////////////////////
        /// \brief Convenience wrapper for a defined
        /// number of tokens.
        ///
        /// \param sMessage const string&
        /// \return string
        ///
        /////////////////////////////////////////////////
        string get(const string& sMessage) const
        {
            vector<string> vTokens;
            return get(sMessage, vTokens);
        }

        /////////////////////////////////////////////////
        /// \brief Convenience wrapper for a defined
        /// number of tokens.
        ///
        /// \param sMessage const string&
        /// \param sTok1 const string&
        /// \return string
        ///
        /////////////////////////////////////////////////
        string get(const string& sMessage, const string& sTok1) const
        {
            vector<string> vTokens;
            vTokens.push_back(sTok1);
            return get(sMessage, vTokens);
        }

        /////////////////////////////////////////////////
        /// \brief Convenience wrapper for a defined
        /// number of tokens.
        ///
        /// \param sMessage const string&
        /// \param sTok1 const string&
        /// \param sTok2 const string&
        /// \return string
        ///
        /////////////////////////////////////////////////
        string get(const string& sMessage, const string& sTok1, const string& sTok2) const
        {
            vector<string> vTokens;
            vTokens.push_back(sTok1);
            vTokens.push_back(sTok2);
            return get(sMessage, vTokens);
        }

        /////////////////////////////////////////////////
        /// \brief Convenience wrapper for a defined
        /// number of tokens.
        ///
        /// \param sMessage const string&
        /// \param sTok1 const string&
        /// \param sTok2 const string&
        /// \param sTok3 const string&
        /// \return string
        ///
        /////////////////////////////////////////////////
        string get(const string& sMessage, const string& sTok1, const string& sTok2, const string& sTok3) const
        {
            vector<string> vTokens;
            vTokens.push_back(sTok1);
            vTokens.push_back(sTok2);
            vTokens.push_back(sTok3);
            return get(sMessage, vTokens);
        }

        /////////////////////////////////////////////////
        /// \brief Convenience wrapper for a defined
        /// number of tokens.
        ///
        /// \param sMessage const string&
        /// \param sTok1 const string&
        /// \param sTok2 const string&
        /// \param sTok3 const string&
        /// \param sTok4 const string&
        /// \return string
        ///
        /////////////////////////////////////////////////
        string get(const string& sMessage, const string& sTok1, const string& sTok2, const string& sTok3, const string& sTok4) const
        {
            vector<string> vTokens;
            vTokens.push_back(sTok1);
            vTokens.push_back(sTok2);
            vTokens.push_back(sTok3);
            vTokens.push_back(sTok4);
            return get(sMessage, vTokens);
        }

        /////////////////////////////////////////////////
        /// \brief Convenience wrapper for a defined
        /// number of tokens.
        ///
        /// \param sMessage const string&
        /// \param sTok1 const string&
        /// \param sTok2 const string&
        /// \param sTok3 const string&
        /// \param sTok4 const string&
        /// \param sTok5 const string&
        /// \return string
        ///
        /////////////////////////////////////////////////
        string get(const string& sMessage, const string& sTok1, const string& sTok2, const string& sTok3, const string& sTok4, const string& sTok5) const
        {
            vector<string> vTokens;
            vTokens.push_back(sTok1);
            vTokens.push_back(sTok2);
            vTokens.push_back(sTok3);
            vTokens.push_back(sTok4);
            vTokens.push_back(sTok5);
            return get(sMessage, vTokens);
        }

        /////////////////////////////////////////////////
        /// \brief Convenience wrapper for a defined
        /// number of tokens.
        ///
        /// \param sMessage const string&
        /// \param sTok1 const string&
        /// \param sTok2 const string&
        /// \param sTok3 const string&
        /// \param sTok4 const string&
        /// \param sTok5 const string&
        /// \param sTok6 const string&
        /// \return string
        ///
        /////////////////////////////////////////////////
        string get(const string& sMessage, const string& sTok1, const string& sTok2, const string& sTok3, const string& sTok4, const string& sTok5, const string& sTok6) const
        {
            vector<string> vTokens;
            vTokens.push_back(sTok1);
            vTokens.push_back(sTok2);
            vTokens.push_back(sTok3);
            vTokens.push_back(sTok4);
            vTokens.push_back(sTok5);
            vTokens.push_back(sTok6);
            return get(sMessage, vTokens);
        }
        vector<string> getList(const string& sMessageScheme) const;

        inline string YES() const
        {
            return sYES;
        }
        inline string NO() const
        {
            return sNO;
        }
};

#endif // LANGUAGE_HPP

