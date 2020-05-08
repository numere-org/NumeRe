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
#include <wx/sizer.h>

#include "../../common/datastructures.h"

class ImagePanel : public wxPanel
{
    wxImage image;
    wxBitmap resized;
    wxToolBar* toptoolbar;
    wxString currentFile;
    wxFrame* parentFrame;
    int w, h, sized_w, sized_h;
    wxArrayString getFileList(const wxString& dirname);
    void LoadImage(const wxString& filename, wxBitmapType = wxBITMAP_TYPE_ANY, bool doUpdateFrame = true);
    bool LoadNextImage(const wxFileName& filename);


public:
    ImagePanel(wxFrame* parent, wxString file, wxBitmapType format);

    void paintEvent(wxPaintEvent & evt);
    void paintNow();
    void OnSize(wxSizeEvent& event);
    void render(wxDC& dc);

    double getRelation() const
        {return w/(double)h;}

    wxToolBar* getToolbar() {return toptoolbar;}

    // some useful events
    /*
     void mouseMoved(wxMouseEvent& event);
     void mouseDown(wxMouseEvent& event);
     void mouseWheelMoved(wxMouseEvent& event);
     void mouseReleased(wxMouseEvent& event);
     void rightClick(wxMouseEvent& event);
     void mouseLeftWindow(wxMouseEvent& event);*/
    void keyPressed(wxKeyEvent& event);
    void OnNavigationKey(wxNavigationKeyEvent& event);
    void OnEnter(wxMouseEvent& event);
    void OnFocus(wxFocusEvent& event);
    void OnLoseFocus(wxFocusEvent& event);
    //void keyReleased(wxKeyEvent& event);

    void OnSaveAs(wxCommandEvent& event);
    void OnCopy(wxCommandEvent& event);
    void OnNextImage(wxCommandEvent& event);
    void OnPreviousImage(wxCommandEvent& event);


    DECLARE_EVENT_TABLE()
};


