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

using namespace std;

class MyTipProvider : public wxTipProvider
{
    private:
        std::vector<std::string> vTip;
        unsigned int nth_tip;
    public:
        MyTipProvider(const std::vector<std::string>& vTipList);

        virtual wxString GetTip()
            {
                nth_tip++;
                if (nth_tip >= vTip.size())
                    nth_tip = 0;
                return vTip[nth_tip];
            }
};

class TipDialog : public wxDialog
{
    public:
        /* text defines all strings:
         * 0 = title, 1 = Did you know, 2 = &Next tip, 3 = &Show tips at startup
         */
        TipDialog(wxWindow *parent, wxTipProvider *tipProvider, const wxArrayString& text, bool showAtStartup);

        // the tip dialog has "Show tips on startup" checkbox - return true if it
        // was checked (or wasn't unchecked)
        bool ShowTipsOnStartup() const { return m_checkbox->GetValue(); }

        // sets the (next) tip text
        void SetTipText() { m_text->SetValue(m_tipProvider->GetTip()); }

        // "Next" button handler
        void OnNextTip(wxCommandEvent& WXUNUSED(event)) { SetTipText(); }

    private:
        wxTipProvider *m_tipProvider;

        wxTextCtrl *m_text;
        wxCheckBox *m_checkbox;

        DECLARE_EVENT_TABLE()
        wxDECLARE_NO_COPY_CLASS(TipDialog);
};


bool ShowTip(wxWindow* parent, wxTipProvider* tipProvider, const wxArrayString& text, bool showAtStartUp = true);
