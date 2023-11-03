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


#include "graphviewer.hpp"
#include "NumeReWindow.h"
#include "../common/Options.h"


BEGIN_EVENT_TABLE(GraphViewer, ViewerFrame)
    EVT_KEY_DOWN        (ViewerFrame::OnKeyDown)
    EVT_SET_FOCUS       (ViewerFrame::OnFocus)
//    EVT_ENTER_WINDOW    (ViewerFrame::OnEnter)
    EVT_CLOSE           (ViewerFrame::OnClose)
END_EVENT_TABLE()


GraphViewer::GraphViewer(wxWindow* parent, const wxString& title, GraphHelper* _helper, NumeReTerminal* terminal) : ViewerFrame(parent, title, static_cast<NumeReWindow*>(parent)->getOptions()->getSetting(SETTING_B_FLOATONPARENT).active() ? wxFRAME_FLOAT_ON_PARENT : 0)
{
    wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
    _grapherWindow = new wxMGL(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxBORDER_THEME);

    _grapherWindow->SetDraw(_helper);
    _grapherWindow->SetGraph(_helper->setGrapher());

    for (size_t i = 0; i < 4; i++)
    {
        _grapherWindow->timeAxes[i] = _helper->timeAxes[i];
    }

    sizer->Add(_grapherWindow, 1, wxEXPAND);
    double dHeight = sqrt(640.0*480.0 / _helper->getAspect());
    if (_helper->getHires())
        dHeight = sqrt(1280.0*960.0 / _helper->getAspect());
    _grapherWindow->SetSize((int)lrint(_helper->getAspect()*dHeight), (int)lrint(dHeight));

    this->SetSizer(sizer);
    this->SetClientSize(_grapherWindow->GetSize());

    m_terminal = terminal;
}


