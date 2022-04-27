/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2019  Erik Haenel et al.

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
#include <vector>
#include "../NumeReWindow.h"

#ifndef PROCEDUREVIEWER_HPP
#define PROCEDUREVIEWER_HPP

struct ProcedureViewerData;

class ProcedureViewer : public wxListView
{
    private:
        NumeReEditor* m_currentEd;
        void getProcedureListFromEditor();
        void stripSpaces(wxString& sString);
        void emptyControl();

    public:
        ProcedureViewer(wxWindow* parent);

        int nSortColumn;
        std::vector<ProcedureViewerData> vData;

        void setCurrentEditor(NumeReEditor* editor);
        void OnColumnClick(wxListEvent& event);
        void OnItemClick(wxListEvent& event);
        void updateProcedureList(const std::vector<wxString>& vProcedures);

        DECLARE_EVENT_TABLE();
};


#endif // PROCEDUREVIEWER_HPP

