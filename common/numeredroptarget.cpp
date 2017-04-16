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


#include "numeredroptarget.hpp"
#include "../gui/NumeReWindow.h"
#include "../editor/editor.h"




#if 1 //wxUSE_DRAG_AND_DROP

NumeReDropTarget::NumeReDropTarget(wxWindow* topwindow, wxWindow* owner, parentType type) : m_owner(owner), m_topWindow(topwindow), m_type(type)
{
    wxDataObjectComposite* dataobj = new wxDataObjectComposite();
    dataobj->Add(new wxTextDataObject(), true);
    dataobj->Add(new wxFileDataObject());
    SetDataObject(dataobj);
}

wxDragResult NumeReDropTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult defaultDragResult)
{
    if (m_type == EDITOR)
    {
        NumeReEditor* edit = static_cast<NumeReEditor*>(m_owner);
        defaultDragResult = edit->DoDragOver(x, y, defaultDragResult);
    }
    return defaultDragResult;
}

wxDragResult NumeReDropTarget::OnData(wxCoord x, wxCoord y, wxDragResult defaultDragResult)
{
    this->GetData();
    wxDataObjectComposite* dataobj = static_cast<wxDataObjectComposite*>(GetDataObject());
    wxDataFormat format = dataobj->GetReceivedFormat();
    wxDataObject* data = dataobj->GetObject(format);
    switch (format.GetType())
    {
        case wxDF_FILENAME:
        {
            wxFileDataObject* filedata = static_cast<wxFileDataObject*>(data);
            NumeReWindow* top = static_cast<NumeReWindow*>(m_topWindow);
            top->OpenSourceFile(filedata->GetFilenames());
            break;
        }
        case wxDF_TEXT:
        case wxDF_UNICODETEXT:
        {
            if (m_type == EDITOR)
            {
                wxTextDataObject* textdata = static_cast<wxTextDataObject*>(data);
                NumeReEditor* edit = static_cast<NumeReEditor*>(m_owner);
                //defaultDragResult = edit->DoDragOver(x, y, defaultDragResult);
                edit->DoDropText(x, y, textdata->GetText());
                //todo
            }
            break;
        }
    }
    return defaultDragResult;
}

#endif //wxUSE_DRAG_AND_DROP
