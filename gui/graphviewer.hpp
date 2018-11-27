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
#include "wx.h"
#include "viewerframe.hpp"
#include "../kernel/core/plotting/graph_helper.hpp"
#include "terminal/wxterm.h"

#ifndef GRAPHVIEWER_HPP
#define GRAPHVIEWER_HPP
class GraphViewer : public ViewerFrame
{
    private:

    public:
        GraphViewer(wxWindow* parent, const wxString& title, GraphHelper* _helper, wxTerm* terminal);
        ~GraphViewer()
            {
                if (_grapherWindow)
                    delete _grapherWindow;
                _grapherWindow = nullptr;
            }


        wxMGL* _grapherWindow;
        wxTerm* m_terminal;

        DECLARE_EVENT_TABLE();
};

#endif // GRAPHVIEWER_HPP

