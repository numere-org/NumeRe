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


#ifndef FNDRPLDIALOG_HPP
#define FNDRPLDIALOG_HPP

#include <wx/fdrepdlg.h>
#include <wx/wx.h>
#include <wx/intl.h>
#include <wx/log.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/radiobox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/settings.h>

#include "../compositions/viewerbook.hpp"
#include "../compositions/viewerpanel.hpp"

#define wxFR_WRAPAROUND 8

class FindReplaceDialog : public wxFindReplaceDialogBase
{
    public:
        FindReplaceDialog() { Init(); }

        FindReplaceDialog(wxWindow *parent,
                                   wxFindReplaceData *data,
                                   const wxString& title,
                                   int style = 0)
        {
            Init();

            (void)Create(parent, data, title, style);
        }

        bool Create(wxWindow *parent,
                    wxFindReplaceData *data,
                    const wxString& title,
                    int style = 0);

        inline void toggleSkipFocus()
            {
                m_skipFocusing = !m_skipFocusing;
                m_tabs->toggleSkipFocus();
                (static_cast<ViewerPanel*>(m_tabs->GetCurrentPage()))->toggleSkipFocus();
            }

        void SetFocus();
    protected:
        void Init();

        void InitPage();
        void OnPageChange(wxNotebookEvent& event);
        void SendEvent(const wxEventType& evtType);

        void OnFind(wxCommandEvent& event);
        void OnReplace(wxCommandEvent& event);
        void OnReplaceAll(wxCommandEvent& event);
        void OnCancel(wxCommandEvent& event);

        void OnEnter(wxMouseEvent& event);
        void OnGainFocus(wxFocusEvent& event);
        void OnLoseFocus(wxFocusEvent& event);

        void OnUpdateFindUI(wxUpdateUIEvent& event);

        void OnCloseWindow(wxCloseEvent& event);


        wxCheckBox *m_chkCase,
                   *m_chkWord,
                   *m_chkWrapAround,
                   *m_chkCaseRepl,
                   *m_chkWordRepl,
                   *m_chkWrapAroundRepl;

        wxRadioBox *m_radioDir,
                   *m_radioDirRepl;

        wxTextCtrl *m_textFind,
                   *m_textFindRepl,
                   *m_textRepl;

        ViewerBook *m_tabs;

        bool m_skipFocusing;

    private:
        DECLARE_DYNAMIC_CLASS(FindReplaceDialog)

        DECLARE_EVENT_TABLE()
};
#endif

