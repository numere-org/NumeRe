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



#include "imagepanel.hpp"


BEGIN_EVENT_TABLE(ImagePanel, wxPanel)
// some useful events
/*
 EVT_MOTION(wxImagePanel::mouseMoved)
 EVT_LEFT_DOWN(wxImagePanel::mouseDown)
 EVT_LEFT_UP(wxImagePanel::mouseReleased)
 EVT_RIGHT_DOWN(wxImagePanel::rightClick)
 EVT_LEAVE_WINDOW(wxImagePanel::mouseLeftWindow)
 EVT_KEY_UP(wxImagePanel::keyReleased)
 EVT_MOUSEWHEEL(wxImagePanel::mouseWheelMoved)
 */
    EVT_ENTER_WINDOW    (ImagePanel::OnEnter)
    EVT_SET_FOCUS       (ImagePanel::OnFocus)
    EVT_KILL_FOCUS      (ImagePanel::OnLoseFocus)
    EVT_KEY_DOWN        (ImagePanel::keyPressed)
    // catch paint events
    EVT_PAINT           (ImagePanel::paintEvent)
    //Size event
    EVT_SIZE            (ImagePanel::OnSize)
END_EVENT_TABLE()



ImagePanel::ImagePanel(wxFrame* parent, wxString file, wxBitmapType format) : wxPanel(parent)
{
    // load the file... ideally add a check to see if loading was successful
    wxInitAllImageHandlers();
    image.LoadFile(file, format);
    w = image.GetWidth();
    h = image.GetHeight();
    sized_w = -1;
    sized_h = -1;
}

/*
 * Called by the system of by wxWidgets when the panel needs
 * to be redrawn. You can also trigger this call by
 * calling Refresh()/Update().
 */

void ImagePanel::paintEvent(wxPaintEvent & evt)
{
    // depending on your system you may need to look at double-buffered dcs
    wxPaintDC dc(this);
    render(dc);
}

/*
 * Alternatively, you can use a clientDC to paint on the panel
 * at any time. Using this generally does not free you from
 * catching paint events, since it is possible that e.g. the window
 * manager throws away your drawing when the window comes to the
 * background, and expects you will redraw it when the window comes
 * back (by sending a paint event).
 */
void ImagePanel::paintNow()
{
    // depending on your system you may need to look at double-buffered dcs
    wxClientDC dc(this);
    render(dc);
}

/*
 * Here we do the actual rendering. I put it in a separate
 * method so that it can work no matter what type of DC
 * (e.g. wxPaintDC or wxClientDC) is used.
 */
void ImagePanel::render(wxDC&  dc)
{
    int neww, newh;
    dc.GetSize(&neww, &newh);

    if (neww != sized_w || newh != sized_h)
    {
        if (neww/(double)newh > w/(double)h)
        {
            sized_w = w/(double)h*newh;
            sized_h = newh;
        }
        else
        {
            sized_w = neww;
            sized_h = h/(double)w*neww;
        }
        // BICUBIC may be slow, but in this case it doesn't have to be fast. However, in most cases the images will be mase smaller and the default quality is bad in this case
        resized = wxBitmap(image.Scale(sized_w, sized_h, wxIMAGE_QUALITY_BICUBIC));
        dc.DrawBitmap( resized, 0, 0, false );
    }
    else
    {
        dc.DrawBitmap( resized, 0, 0, false );
    }
}

/*
 * Here we call refresh to tell the panel to draw itself again.
 * So when the user resizes the image panel the image should be resized too.
 */
void ImagePanel::OnSize(wxSizeEvent& event)
{
    Refresh();
    //skip the event.
    event.Skip();
}


void ImagePanel::keyPressed(wxKeyEvent& event)
{
    // connecting the ESC Key with closing the image
    if (event.GetKeyCode() == WXK_ESCAPE)
        m_parent->Close();
}

void ImagePanel::OnEnter(wxMouseEvent& event)
{
    this->SetFocus();
    event.Skip();
}


void ImagePanel::OnFocus(wxFocusEvent& event)
{
    //m_parent->SetTransparent(wxIMAGE_ALPHA_OPAQUE);
    event.Skip();
}

void ImagePanel::OnLoseFocus(wxFocusEvent& event)
{
    //m_parent->SetTransparent(80);
    event.Skip();
}


