/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2017  Erik Haenel et al.

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

#include <wx/wx.h>
#include <wx/tipdlg.h>
#include <vector>
#include <string>
#include <cmath>
#include <ctime>

#include "../stringconv.hpp"
#include "../compositions/grouppanel.hpp"


/////////////////////////////////////////////////
/// \brief Custom implementation of a tip
/// provider.
/////////////////////////////////////////////////
class MyTipProvider : public wxTipProvider
{
    private:
        std::vector<std::string> vTip;
        size_t nth_tip;
    public:
        MyTipProvider(const std::vector<std::string>& vTipList);

        /////////////////////////////////////////////////
        /// \brief Get the next tip.
        ///
        /// \return virtual wxString
        ///
        /////////////////////////////////////////////////
        virtual wxString GetTip()
        {
            nth_tip++;

            if (nth_tip >= vTip.size())
                nth_tip = 0;

            return wxFromUtf8(vTip[nth_tip]);
        }

        /////////////////////////////////////////////////
        /// \brief Get the index of the current tip and
        /// the count of available tips.
        ///
        /// \return std::pair<std::string,std::string>
        ///
        /////////////////////////////////////////////////
        std::pair<std::string,std::string> getIndex() const
        {
            return std::make_pair(std::to_string(nth_tip+1), std::to_string(vTip.size()));
        }
};



/////////////////////////////////////////////////
/// \brief Implements a custom tip dialog.
/////////////////////////////////////////////////
class TipDialog : public wxDialog
{
    public:
        TipDialog(wxWindow *parent, MyTipProvider *tipProvider, const wxArrayString& text, bool showAtStartup);

        /////////////////////////////////////////////////
        /// \brief Shall we show this dialog again next
        /// time?
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool ShowTipsOnStartup() const
        {
            return m_checkbox->GetValue();
        }

        void SetTipText();


        /////////////////////////////////////////////////
        /// \brief Button handler for clicking on "Next".
        ///
        /// \param event wxCommandEvent&
        ///
        /////////////////////////////////////////////////
        void OnNextTip(wxCommandEvent& WXUNUSED(event))
        {
            SetTipText();
        }

    private:
        MyTipProvider *m_tipProvider;

        TextField *m_text;
        wxCheckBox *m_checkbox;
        wxArrayString m_textSnippets;
        wxStaticText* m_header;

        DECLARE_EVENT_TABLE()
        wxDECLARE_NO_COPY_CLASS(TipDialog);
};



bool ShowTip(wxWindow* parent, MyTipProvider* tipProvider, const wxArrayString& text, bool showAtStartUp = true);
