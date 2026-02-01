/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2026  Erik Haenel et al.

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

#include <wx/string.h>
#include "stringconv.hpp"
#include "../kernel/core/ui/language.hpp"

#ifndef GUILANG_HPP
#define GUILANG_HPP


/////////////////////////////////////////////////
/// \brief This class handles the internal
/// language system and returns the language
/// strings of the selected language based upon
/// the language files. This is an adaption for
/// the GUI strings, which are not encoded as
/// UTF-8.
/////////////////////////////////////////////////
class GuiLanguage : public Language
{
    public:
        GuiLanguage() : Language() {}
        GuiLanguage(const GuiLanguage& _lang) : Language(_lang) {}

        /////////////////////////////////////////////////
        /// \brief This member function searches the
        /// internal language map for an identifier, which
        /// starts similar with the passed identifier and
        /// returns the found identifier.
        ///
        /// \param sMessage const std::string&
        /// \return wxString
        ///
        /// The passed language identifier has to contain
        /// an asteriks as a wildcard.
        /////////////////////////////////////////////////
        wxString getKey(const std::string& sMessage) const
        {
            return wxFromUtf8(Language::getKey(sMessage));
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns the
        /// language string for the passed language
        /// identifier and replaces all enumerated tokens
        /// with the passed token list.
        ///
        /// \param sMessage const std::string&
        /// \param vTokens const std::vector<wxString>&
        /// \return wxString
        ///
        /////////////////////////////////////////////////
        wxString get(const std::string& sMessage, const std::vector<wxString>& vTokens) const
        {
            std::vector<std::string> vConvTokens;

            for (const wxString& tok : vTokens)
            {
                vConvTokens.emplace_back(wxToUtf8(tok));
            }

            return wxFromUtf8(Language::get(sMessage, vConvTokens));
        }

        /////////////////////////////////////////////////
        /// \brief Convenience wrapper for a defined
        /// number of tokens.
        ///
        /// \param sMessage const std::string&
        /// \return wxString
        ///
        /////////////////////////////////////////////////
        wxString get(const std::string& sMessage) const
        {
            return wxFromUtf8(Language::get(sMessage));
        }

        /////////////////////////////////////////////////
        /// \brief Convenience wrapper for a defined
        /// number of tokens.
        ///
        /// \param sMessage const std::string&
        /// \param tok1 const wxString&
        /// \return wxString
        ///
        /////////////////////////////////////////////////
        wxString get(const std::string& sMessage, const wxString& tok1) const
        {
            return wxFromUtf8(Language::get(sMessage, wxToUtf8(tok1)));
        }

        /////////////////////////////////////////////////
        /// \brief Convenience wrapper for a defined
        /// number of tokens.
        ///
        /// \param sMessage const std::string&
        /// \param tok1 const wxString&
        /// \param tok2 const wxString&
        /// \return wxString
        ///
        /////////////////////////////////////////////////
        wxString get(const std::string& sMessage, const wxString& tok1, const wxString& tok2) const
        {
            return wxFromUtf8(Language::get(sMessage, wxToUtf8(tok1), wxToUtf8(tok2)));
        }

        /////////////////////////////////////////////////
        /// \brief Convenience wrapper for a defined
        /// number of tokens.
        ///
        /// \param sMessage const std::string&
        /// \param tok1 const wxString&
        /// \param tok2 const wxString&
        /// \param tok3 const wxString&
        /// \return wxString
        ///
        /////////////////////////////////////////////////
        wxString get(const std::string& sMessage, const wxString& tok1, const wxString& tok2, const wxString& tok3) const
        {
            return wxFromUtf8(Language::get(sMessage, wxToUtf8(tok1), wxToUtf8(tok2), wxToUtf8(tok3)));
        }

        /////////////////////////////////////////////////
        /// \brief Convenience wrapper for a defined
        /// number of tokens.
        ///
        /// \param sMessage const std::string&
        /// \param tok1 const wxString&
        /// \param tok2 const wxString&
        /// \param tok3 const wxString&
        /// \param tok4 const wxString&
        /// \return wxString
        ///
        /////////////////////////////////////////////////
        wxString get(const std::string& sMessage, const wxString& tok1, const wxString& tok2, const wxString& tok3, const wxString& tok4) const
        {
            return wxFromUtf8(Language::get(sMessage, wxToUtf8(tok1), wxToUtf8(tok2), wxToUtf8(tok3), wxToUtf8(tok4)));
        }

        /////////////////////////////////////////////////
        /// \brief Convenience wrapper for a defined
        /// number of tokens.
        ///
        /// \param sMessage const std::string&
        /// \param tok1 const wxString&
        /// \param tok2 const wxString&
        /// \param tok3 const wxString&
        /// \param tok4 const wxString&
        /// \param tok5 const wxString&
        /// \return wxString
        ///
        /////////////////////////////////////////////////
        wxString get(const std::string& sMessage, const wxString& tok1, const wxString& tok2, const wxString& tok3, const wxString& tok4, const wxString& tok5) const
        {
            return wxFromUtf8(Language::get(sMessage, wxToUtf8(tok1), wxToUtf8(tok2), wxToUtf8(tok3), wxToUtf8(tok4), wxToUtf8(tok5)));
        }

        /////////////////////////////////////////////////
        /// \brief Convenience wrapper for a defined
        /// number of tokens.
        ///
        /// \param sMessage const std::string&
        /// \param tok1 const wxString&
        /// \param tok2 const wxString&
        /// \param tok3 const wxString&
        /// \param tok4 const wxString&
        /// \param tok5 const wxString&
        /// \param tok6 const wxString&
        /// \return wxString
        ///
        /////////////////////////////////////////////////
        wxString get(const std::string& sMessage, const wxString& tok1, const wxString& tok2, const wxString& tok3, const wxString& tok4, const wxString& tok5, const wxString& tok6) const
        {
            return wxFromUtf8(Language::get(sMessage, wxToUtf8(tok1), wxToUtf8(tok2), wxToUtf8(tok3), wxToUtf8(tok4), wxToUtf8(tok5), wxToUtf8(tok6)));
        }

        /////////////////////////////////////////////////
        /// \brief This member function returns a vector
        /// of language strings matching to the passed
        /// identifier containing wildcards.
        ///
        /// \param sMessageScheme const std::string&
        /// \return std::vector<wxString>
        ///
        /////////////////////////////////////////////////
        std::vector<wxString> getList(const std::string& sMessageScheme) const
        {
            std::vector<std::string> vList = Language::getList(sMessageScheme);
            std::vector<wxString> vConvList;

            for (const std::string& it : vList)
            {
                vConvList.emplace_back(wxFromUtf8(it));
            }

            return vConvList;
        }

        inline wxString YES() const
        {
            return wxFromUtf8(sYES);
        }

        inline wxString NO() const
        {
            return wxFromUtf8(sNO);
        }
};


extern GuiLanguage _guilang;


#endif // GUILANG_HPP

