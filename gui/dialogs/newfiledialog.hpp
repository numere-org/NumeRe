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

#ifndef NEWFILEDIALOG_HPP
#define NEWFILEDIALOG_HPP

#include <wx/wx.h>
#include <wx/listctrl.h>
#include "../../common/datastructures.h"

class TextField;

/////////////////////////////////////////////////
/// \brief This class represents the new file
/// dialog with the description of the different
/// file types.
/////////////////////////////////////////////////
class NewFileDialog : public wxDialog
{
private:
    wxListView* m_fileTypes;
    TextField* m_description;
    FileFilterType m_selectedFileType;
    wxArrayString m_fileTypeDesc;
    wxButton* m_okButton;

    void selectItem(int itemId);

    void OnSelectItem(wxListEvent& event);
    void OnActivateItem(wxListEvent& event);
    void OnAbort(wxCommandEvent& event);
    void OnOK(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);

public:
    NewFileDialog(wxWindow* parent, const wxString& iconPath);
    FileFilterType GetSelection() const;

    DECLARE_EVENT_TABLE()
};

#endif // NEWFILEDIALOG_HPP
