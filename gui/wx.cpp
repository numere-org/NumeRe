/***************************************************************************
 * wx.cpp is part of Math Graphic Library                              *
 * Copyright (C) 2007-2016 Alexey Balakin <mathgl.abalakin@gmail.ru>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 3 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
//-----------------------------------------------------------------------------
#include <wx/dcclient.h>
#include <wx/msgdlg.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/menu.h>
#include <wx/scrolwin.h>
#include <wx/artprov.h>

#include <cmath>

#include <mgl2/canvas_wnd.h>
#include "wx.h"

#include "../common/datastructures.h"
#include "../kernel/core/ui/language.hpp"

#include "terminal/terminal.hpp"
#include "graphviewer.hpp"

#include "icons/addtext.xpm"
#include "icons/addline.xpm"
#include "icons/addrect.xpm"
#include "icons/addcirc.xpm"
#include "icons/rotate_new.xpm"
#include "icons/zoom_new.xpm"
#include "icons/newstart1.xpm"
#include "icons/newstop1.xpm"

extern Language _guilang;

//-----------------------------------------------------------------------------
class mglCanvasWX : public mglCanvasWnd
{
friend class wxMGL;
public:
    int sshow;			///< Current state of animation switch (toggle button)
    wxMGL *WMGL;		///< Control which draw graphics
    wxWindow *Wnd;		///< Pointer to window

    mglCanvasWX();
    virtual ~mglCanvasWX();

    /// Create a window for plotting. Now implemeted only for GLUT.
    void Window(int argc, char **argv, int (*draw)(mglBase *gr, void *p), const char *title,
                void *par=NULL, void (*reload)(void *p)=NULL, bool maximize=false);
    /// Switch on/off transparency (do not overwrite switches in user drawing function)
    void ToggleAlpha();
    /// Switch on/off lighting (do not overwrite switches in user drawing function)
    void ToggleLight();
    void ToggleRotate();	///< Switch on/off rotation by mouse
    void ToggleZoom();		///< Switch on/off zooming by mouse
    void ToggleNo();		///< Switch off all zooming and rotation
    void Update();			///< Update picture by calling user drawing function
    void Adjust();			///< Adjust size of bitmap to window size
    void GotoFrame(int d);	///< Show arbitrary frame (use relative step)
    void Animation();	///< Run animation (I'm too lasy to change it)

protected:
    wxScrolledWindow *scroll;	///< Scrolling area
    wxMenu *popup;			///< Popup menu
//	wxSpinCtrl *tet, *phi;	///< Spin box for angles // TODO

    void MakeMenu();		///< Create menu, toolbar and popup menu
};

//-----------------------------------------------------------------------------
const wxString ScriptName(L"default");

enum
{
    TIMER_ID=1000,
    LAST_ID
};

BEGIN_EVENT_TABLE(wxMGL, wxWindow)
    EVT_TIMER	    (TIMER_ID,	wxMGL::OnNextSlide)
    EVT_PAINT	    (wxMGL::OnPaint)
    EVT_ERASE_BACKGROUND(wxMGL::OnEraseBackground)
    EVT_SIZE	    (wxMGL::OnSize)
    EVT_LEFT_DOWN	(wxMGL::OnMouseLeftDown)
    EVT_RIGHT_DOWN	(wxMGL::OnMouseDown)
    EVT_MIDDLE_DOWN	(wxMGL::OnMouseDown)
    EVT_LEFT_UP		(wxMGL::OnMouseLeftUp)
    EVT_RIGHT_UP	(wxMGL::OnMouseRightUp)
    EVT_MOTION		(wxMGL::OnMouseMove)
    EVT_KEY_DOWN    (wxMGL::OnKeyDown)
    EVT_NAVIGATION_KEY(wxMGL::OnNavigationKey)
    EVT_ENTER_WINDOW(wxMGL::OnEnter)
    EVT_CLOSE       (wxMGL::OnClose)

    EVT_MENU        (ID_GRAPH_EXPORT, wxMGL::OnMenuEvent)
    EVT_MENU        (ID_GRAPH_COPY, wxMGL::OnMenuEvent)
    EVT_MENU        (ID_GRAPH_LIGHT, wxMGL::OnMenuEvent)
    EVT_MENU        (ID_GRAPH_ALPHA, wxMGL::OnMenuEvent)
    EVT_MENU        (ID_GRAPH_RESET, wxMGL::OnMenuEvent)
    EVT_MENU        (ID_GRAPH_ROTATE, wxMGL::OnMenuEvent)
    EVT_MENU        (ID_GRAPH_ZOOM, wxMGL::OnMenuEvent)
    EVT_MENU_RANGE  (ID_GRAPH_NEXT, ID_GRAPH_STOP, wxMGL::OnMenuEvent)
    EVT_MENU_RANGE  (ID_GRAPH_DRAW_FIRST, ID_GRAPH_DRAW_LAST, wxMGL::OnMenuEvent)
END_EVENT_TABLE()
//-----------------------------------------------------------------------------
//
//		class wxMathGL
//
//-----------------------------------------------------------------------------
wxMGL::wxMGL(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name) : wxWindow(parent, id, pos, size, style, name)
{
    AutoResize = true;
    draw_par = nullptr;
    draw_func = 0;
    gr = nullptr;
    popup = nullptr;
    dAzimutalViewPoint = dPolarViewPoint = dPerspective = 0.0;
    x1 = y1 = 0;
    x2 = y2 = 1;
    l_x1 = l_y1 = 0;
    start_x = start_y = 0;
    alpha = light = bZoomingMode = zoomactive = bRotatingMode = false;
    drawMode = DM_NONE;
    drawModeActive = false;
    skiprotate = 0;
    nFrameCounter = 0;
    nFramesToSkip = 2;
    timer = new wxTimer(this, TIMER_ID);
    m_parentFrame = static_cast<wxFrame*>(parent);
    animation = false;

    SetOwnBackgroundColour(*wxWHITE);

    // Initialize the toolbar
    InitializeToolbar();

    m_parentFrame->Bind(wxEVT_MENU, &wxMGL::OnMenuEvent, this, ID_GRAPH_EXPORT);
    m_parentFrame->Bind(wxEVT_MENU, &wxMGL::OnMenuEvent, this, ID_GRAPH_COPY);
    m_parentFrame->Bind(wxEVT_MENU, &wxMGL::OnMenuEvent, this, ID_GRAPH_ROTATE);
    m_parentFrame->Bind(wxEVT_MENU, &wxMGL::OnMenuEvent, this, ID_GRAPH_ZOOM);
    m_parentFrame->Bind(wxEVT_MENU, &wxMGL::OnMenuEvent, this, ID_GRAPH_RESET);
    m_parentFrame->Bind(wxEVT_MENU, &wxMGL::OnMenuEvent, this, ID_GRAPH_NEXT);
    m_parentFrame->Bind(wxEVT_MENU, &wxMGL::OnMenuEvent, this, ID_GRAPH_PREVIOUS);
    m_parentFrame->Bind(wxEVT_MENU, &wxMGL::OnMenuEvent, this, ID_GRAPH_RUN);
    m_parentFrame->Bind(wxEVT_MENU, &wxMGL::OnMenuEvent, this, ID_GRAPH_STOP);

    for (int i = ID_GRAPH_DRAW_FIRST+1; i < ID_GRAPH_DRAW_LAST; i++)
        m_parentFrame->Bind(wxEVT_MENU, &wxMGL::OnMenuEvent, this, i);

    statusbar = m_parentFrame->CreateStatusBar(3);
    int nWidths[] = {-2,-1,-1};
    statusbar->SetFieldsCount(3, nWidths);
}

// Destructor: stops the timer, if it is running
// and deletes the referenced drawing class
wxMGL::~wxMGL()
{
    timer->Stop();

    if (draw_cl)
        delete draw_cl;

    draw_cl = nullptr;
}

// This member function dis- or enables the animation tools
// depending on whether an animation is available
void wxMGL::UpdateTools()
{
    if (gr->GetNumFrame() <= 1)
    {
        toptoolbar->EnableTool(ID_GRAPH_NEXT, false);
        toptoolbar->EnableTool(ID_GRAPH_PREVIOUS, false);
        toptoolbar->EnableTool(ID_GRAPH_RUN, false);
        toptoolbar->EnableTool(ID_GRAPH_STOP, false);
    }
    else
    {
        wxString slidescount;
        slidescount.Printf("%d / %d", gr->GetNumFrame(), gr->GetNumFrame());
        statusbar->SetStatusText(slidescount, 2);
        nFrameCounter = gr->GetNumFrame()-1;
    }
}

// This member function returns the image ratio
double wxMGL::GetRatio()
{
    return double(mgl_get_width(gr->Self())) / mgl_get_height(gr->Self());
}

// This member function initializes the toolbar of the
// GraphViewer Window
void wxMGL::InitializeToolbar()
{
    toptoolbar = m_parentFrame->CreateToolBar(wxTB_HORIZONTAL | wxTB_FLAT);
    toptoolbar->AddTool(ID_GRAPH_EXPORT, _guilang.get("GUI_GRAPH_EXPORT"), wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_TOOLBAR), _guilang.get("GUI_GRAPH_EXPORT"));
    toptoolbar->AddTool(ID_GRAPH_COPY, _guilang.get("GUI_GRAPH_COPY"), wxArtProvider::GetBitmap(wxART_COPY, wxART_TOOLBAR), _guilang.get("GUI_GRAPH_COPY"));
    toptoolbar->AddSeparator();

    wxBitmap bm_rotate(rotate_new_xpm);
    toptoolbar->AddTool(ID_GRAPH_ROTATE, _guilang.get("GUI_GRAPH_ROTATE"), bm_rotate, _guilang.get("GUI_GRAPH_ROTATE"), wxITEM_CHECK);
    wxBitmap bm_zoom(zoom_new_xpm);
    toptoolbar->AddTool(ID_GRAPH_ZOOM, _guilang.get("GUI_GRAPH_ZOOM"), bm_zoom, _guilang.get("GUI_GRAPH_ZOOM"), wxITEM_CHECK);
    toptoolbar->AddTool(ID_GRAPH_RESET, _guilang.get("GUI_GRAPH_RESET"), wxArtProvider::GetBitmap(wxART_UNDO, wxART_TOOLBAR), _guilang.get("GUI_GRAPH_RESET"));
    toptoolbar->AddSeparator();

    toptoolbar->AddTool(ID_GRAPH_PREVIOUS, _guilang.get("GUI_GRAPH_PREVIOUS"), wxArtProvider::GetBitmap(wxART_GO_BACK, wxART_TOOLBAR), _guilang.get("GUI_GRAPH_PREVIOUS"));
    toptoolbar->AddTool(ID_GRAPH_NEXT, _guilang.get("GUI_GRAPH_NEXT"), wxArtProvider::GetBitmap(wxART_GO_FORWARD, wxART_TOOLBAR), _guilang.get("GUI_GRAPH_NEXT"));
    wxBitmap bm_run(newstart1_xpm);
    toptoolbar->AddTool(ID_GRAPH_RUN, _guilang.get("GUI_GRAPH_RUN"), bm_run, _guilang.get("GUI_GRAPH_RUN"));
    wxBitmap bm_stop(newstop1_xpm);
    toptoolbar->AddTool(ID_GRAPH_STOP, _guilang.get("GUI_GRAPH_STOP"), bm_stop, _guilang.get("GUI_GRAPH_STOP"));
    toptoolbar->EnableTool(ID_GRAPH_STOP, false);
    toptoolbar->AddSeparator();

    wxBitmap bm_addline(addline);
    toptoolbar->AddTool(ID_GRAPH_LINE, _guilang.get("GUI_GRAPH_LINE"),  bm_addline, _guilang.get("GUI_GRAPH_LINE"), wxITEM_CHECK);
    wxBitmap bm_addrect(addrect);
    toptoolbar->AddTool(ID_GRAPH_RECT, _guilang.get("GUI_GRAPH_RECT"),  bm_addrect, _guilang.get("GUI_GRAPH_RECT"), wxITEM_CHECK);
    wxBitmap bm_addcirc(addcirc);
    toptoolbar->AddTool(ID_GRAPH_CIRCLE, _guilang.get("GUI_GRAPH_CIRCLE"),  bm_addcirc, _guilang.get("GUI_GRAPH_CIRCLE"), wxITEM_CHECK);
    wxBitmap bm_addtext(addtext);
    toptoolbar->AddTool(ID_GRAPH_TEXT, _guilang.get("GUI_GRAPH_TEXT"), bm_addtext, _guilang.get("GUI_GRAPH_TEXT"), wxITEM_CHECK);

    styling = new wxTextCtrl(toptoolbar, wxID_ANY, "B-__");

    toptoolbar->AddControl(styling, "STYLE");
    toptoolbar->Realize();
}

// This member function displays the file dialog during
// exporting the current image displayed in the window
void wxMGL::OnExport()
{
    static const wxString fileFilter = "PNG (*.png)|*.png|JPG (*.jpg)|*.jpg;*.jpeg|EPS (*.eps)|*.eps|SVG (*.svg)|*.svg";

    wxFileName fileName;
    wxString path = static_cast<GraphViewer*>(m_parentFrame)->m_terminal->getPathSettings()[PLOTPATH];
    wxFileDialog dlg(this, _guilang.get("GUI_DLG_SAVEAS"), path, "graph", fileFilter, wxFD_SAVE | wxFD_OVERWRITE_PROMPT | wxFD_CHANGE_DIR);

    // ie, user clicked cancel
    if (dlg.ShowModal() != wxID_OK)
    {
        return;
    }

    fileName = wxFileName(dlg.GetPath());

    if (fileName.GetExt() == "png")
        ExportPNG(fileName.GetFullPath());
    else if (fileName.GetExt() == "jpg")
        ExportJPG(fileName.GetFullPath());
    else if (fileName.GetExt() == "eps")
        ExportEPS(fileName.GetFullPath());
    else if (fileName.GetExt() == "svg")
        ExportSVG(fileName.GetFullPath());
}

// This member function is the redrawing and painting
// event handler. It will display the bitmap currently
// selected by the wxMGL::Repaint() function
void wxMGL::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);

    if (!pic.IsOk())
        Update();

    // Draw the bitmap, which was selected by the
    // Repaint function
    dc.DrawBitmap(pic, 0, 0, false);

    // Prepare the brushes for the drawing tools
    dc.SetBrush(*wxTRANSPARENT_BRUSH);
    dc.SetPen(*wxBLACK_DASHED_PEN);

    // Draw the zooming rectangle
    if (zoomactive)
        dc.DrawRectangle(x0, y0, xe-x0, ye-y0);

    // Draw the previews of the drawing objects
    if (drawModeActive)
    {
        switch (drawMode)
        {
            case DM_LINE:
                dc.DrawLine(start_x, start_y, xe, ye);
                break;
            case DM_CIRCLE:
                dc.DrawCircle(start_x, start_y, hypot(xe-start_x, ye-start_y));
                break;
            case DM_RECT:
                dc.DrawRectangle(start_x, start_y, xe-start_x, ye-start_y);
                break;

        }
    }
}

// This member function handles the background erasing
// process. It is disabled to avoid flickering
void wxMGL::OnEraseBackground(wxEraseEvent& event)
{
    // Empty method to avoid flickering
}

// This member function is the resizing event handler
void wxMGL::OnSize(wxSizeEvent& event)
{
    wxSize ev = event.GetSize();

    // Do nothing, if the graph does not exist
    if (!gr)
        return;

    // Do nothing if the size did not change
    if (mgl_get_width(gr->Self()) == ev.GetWidth() && mgl_get_height(gr->Self()) == ev.GetHeight())
        return;

    // Resize the image canvas
    if (AutoResize && ev.GetWidth() > 0 && ev.GetHeight() > 0)
    {
        gr->SetSize(ev.GetWidth(), ev.GetHeight(), false);
        Update();
    }
    else
        SetSize(mgl_get_width(gr->Self()), mgl_get_height(gr->Self()));
}

// This member function is the timer event handling
// function needed for the animation
void wxMGL::OnNextSlide(wxTimerEvent& )
{
    NextSlide();
}

// This member function can be used to set the
// perspective effect applied to the drawing
void wxMGL::SetPer(int p)
{
    // Convert the integer percentage into a double value
    if (100*dPerspective != p && p >= 0 && p < 100)
    {
        dPerspective = 0.01*p;
        Repaint();
    }
}

// This member function can be used to set the
// azimutal viewpoint of the drawing
void wxMGL::SetPhi(int p)
{
    if (dAzimutalViewPoint != p)
    {
        dAzimutalViewPoint = p;
        Repaint();
    }
}

// This member function can be used to set the
// polar viewpoint of the drawing
void wxMGL::SetTet(int t)
{
    if (dPolarViewPoint != t)
    {
        dPolarViewPoint = t;
        Repaint();
    }
}

// Unneeded member function
void wxMGL::SetAlpha(bool a)
{
    if (alpha != a)
    {
        alpha = a;
        Update();
    }
    toptoolbar->ToggleTool(ID_GRAPH_ALPHA, alpha);
}

// Unneeded member function
void wxMGL::SetLight(bool l)
{
    if (light != l)
    {
        light = l;
        Update();
    }
    toptoolbar->ToggleTool(ID_GRAPH_LIGHT, light);
}

// This member function can be used to (de-)
// activate the zooming mode
void wxMGL::SetZoom(bool z)
{
    if (bZoomingMode != z)
    {
        SetDrawMode(DM_NONE);
        bZoomingMode = z;
        bRotatingMode = false;
        Update();
    }

    if (bZoomingMode)
        SetCursor(wxCursor(wxCURSOR_MAGNIFIER));

    toptoolbar->ToggleTool(ID_GRAPH_ZOOM, bZoomingMode);
    statusbar->SetStatusText(_guilang.get("GUI_GRAPH_ZOOM"), 1);
}

// This member function can be used to (de-)
// activate the rotating mode
void wxMGL::SetRotate(bool r)
{
    if (bRotatingMode != r)
    {
        SetDrawMode(DM_NONE);
        bZoomingMode = false;
        bRotatingMode = r;
        Update();
    }

    if (bRotatingMode)
        SetCursor(wxCursor(wxCURSOR_HAND));

    toptoolbar->ToggleTool(ID_GRAPH_ROTATE, bRotatingMode);
    statusbar->SetStatusText(_guilang.get("GUI_GRAPH_ROTATE"), 1);
}

// This member function selects the current drawing mode
// by dis- and enabling the tools and the other modification
// modes
void wxMGL::SetDrawMode(int dm)
{
    if (drawMode != dm)
    {
        bZoomingMode = false;
        bRotatingMode = false;
        drawMode = dm;
        Update();
    }

    // Set the correct cursor
    if (dm == DM_NONE)
        SetCursor(wxCursor(wxCURSOR_ARROW));
    else if (dm == DM_TEXT)
        SetCursor(wxCursor(wxCURSOR_IBEAM));
    else
        SetCursor(wxCursor(wxCURSOR_CROSS));

    // Unselect the view modification tools
    toptoolbar->ToggleTool(ID_GRAPH_ZOOM, false);
    toptoolbar->ToggleTool(ID_GRAPH_ROTATE, false);

    // Unselect the drawing tools first
    for (int i = ID_GRAPH_DRAW_FIRST+1; i < ID_GRAPH_DRAW_LAST; i++)
        toptoolbar->ToggleTool(i, false);

    // Select the correct drawing tool and
    // display a corresponding message in the
    //  status bar
    switch (dm)
    {
        case DM_LINE:
            toptoolbar->ToggleTool(ID_GRAPH_LINE, true);
            statusbar->SetStatusText(_guilang.get("GUI_GRAPH_LINE"), 1);
            break;
        case DM_CIRCLE:
            toptoolbar->ToggleTool(ID_GRAPH_CIRCLE, true);
            statusbar->SetStatusText(_guilang.get("GUI_GRAPH_CIRCLE"), 1);
            break;
        case DM_RECT:
            toptoolbar->ToggleTool(ID_GRAPH_RECT, true);
            statusbar->SetStatusText(_guilang.get("GUI_GRAPH_RECT"), 1);
            break;
        case DM_TEXT:
            toptoolbar->ToggleTool(ID_GRAPH_TEXT, true);
            statusbar->SetStatusText(_guilang.get("GUI_GRAPH_TEXT"), 1);
            break;
    }
}

// Unneeded member function
void wxMGL::ShiftDown()
{
    mreal d = (y2-y1)/3;
    y1 += d;
    y2 += d;
    Repaint();
}

// Unneeded member function
void wxMGL::ShiftUp()
{
    mreal d = (y2-y1)/3;
    y1 -= d;
    y2 -= d;
    Repaint();
}

// Unneeded member function
void wxMGL::ShiftRight()
{
    mreal d = (x2-x1)/3;
    x1 -= d;
    x2 -= d;
    Repaint();
}

// Unneeded member function
void wxMGL::ShiftLeft()
{
    mreal d = (x2-x1)/3;
    x1 += d;
    x2 += d;
    Repaint();
}

// This member function draws the drawing object
// selected by the toolbar after the user released
// the left mouse button
void wxMGL::DrawCurrentObject(int end_x, int end_y)
{
    // Get the starting and ending postions
    mglPoint startPoint = gr->CalcXYZ(start_x, start_y);
    mglPoint endPoint = gr->CalcXYZ(end_x, end_y);

    // Get the style from the toolbar
    std::string style = styling->GetLineText(0).ToStdString();

    // Draw the object
    switch (drawMode)
    {
        case DM_LINE:
            gr->Line(startPoint, endPoint, style.c_str());
            Update();
            break;
        case DM_CIRCLE:
            gr->Circle(startPoint, hypot(endPoint.x - startPoint.x, endPoint.y - startPoint.y), style.c_str());
            Update();
            break;
        case DM_RECT:
            gr->Face(startPoint, mglPoint(startPoint.x, endPoint.y), mglPoint(endPoint.x, startPoint.y), endPoint, style.c_str());
            Update();
            break;
        case DM_TEXT:
            wxString text;
            wxTextEntryDialog dialog(this, _guilang.get("GUI_DLG_DRAWTEXT_QUESTION"), _guilang.get("GUI_DLG_DRAWTEXT"), wxEmptyString, wxOK | wxCANCEL | wxCENTRE);

            if (dialog.ShowModal() == wxID_OK)
            {
                text = dialog.GetValue();
                gr->Puts(startPoint, text.ToStdString().c_str(), style.c_str());
                Update();
            }

            break;
    }
}

// This member funciton can be used to reset the
// the view modifications completely
void wxMGL::Restore()
{
    SetPhi(0);
    SetTet(0);
    SetPer(0);
    x1 = y1 = 0;
    x2 = y2 = 1;
    bZoomingMode = bRotatingMode = false;
    Repaint();
}

// Unneeded member function
void wxMGL::ZoomIn()
{
    mreal d;
    d = (y2-y1)/4;
    y1 += d;
    y2 -= d;
    d = (x2-x1)/4;
    x1 += d;
    x2 -= d;
    Repaint();
}

// Unneeded member function
void wxMGL::ZoomOut()
{
    mreal d;
    d = (y2-y1)/2;
    y1 -= d;
    y2 += d;
    d = (x2-x1)/2;
    x1 -= d;
    x2 += d;
    Repaint();
}

// This member function tells the internal
// mglGraph object to draw its contents
void wxMGL::Update()
{
    if (draw_func || draw_cl)
    {
        if (!vAnimationBuffer.size() || vAnimationBuffer.size() < (size_t)gr->GetNumFrame())
        {
            if (bRotatingMode)
                gr->Zoom(0,0,1,1);

            if (draw_func)
                draw_func(gr->Self(), draw_par);	// drawing itself
            else if (draw_cl)
            {
                draw_cl->Draw(gr);
            }

            const char *buf = mgl_get_mess(gr->Self());

            if (*buf)
            {
                wxMessageDialog dlg(this, wxString(buf,wxConvLocal), appName, wxOK);
                dlg.ShowModal();
            }
        }
    }
    else if (mgl_get_num_frame(gr->Self())>0)
    {
        mgl_set_alpha(gr->Self(),alpha);
        mgl_set_light(gr->Self(),light);
        mgl_get_frame(gr->Self(),0);
    }

    MousePos.Empty();
    Repaint();
}

// This member function converts the RGB image
// created by the mglGraph object into a
// platform-independent wxImage object, which can
// be used to create a bitmap from
wxImage wxMGL::ConvertFromGraph()
{
    const unsigned char *bb = mgl_get_rgb(gr->Self());
    int w = mgl_get_width(gr->Self());
    int h = mgl_get_height(gr->Self());
    unsigned char *tmp = (unsigned char*)malloc(3*w*h);
    memcpy(tmp, bb, 3*w*h);
    return wxImage(w, h, tmp);
}

// This member function sets the current drawing
// as new bitmap and applies the view modifications
// in advance
void wxMGL::Repaint()
{
    if (bRotatingMode && (skiprotate % nFramesToSkip))
        return;

    if (bZoomingMode)
        gr->Zoom(x1, y1, x2, y2);

    if (bRotatingMode)
        gr->View(0, -dPolarViewPoint, -dAzimutalViewPoint);

    if (dPerspective)
        gr->Perspective(dPerspective);

    // Set the new bitmap
    setBitmap();

    wxSize sz = GetSize();

    // Resize the current window, if the picture does
    // not fit
    if (pic.GetWidth() != sz.GetWidth() || pic.GetHeight() != sz.GetHeight())
        SetSize(pic.GetWidth(), pic.GetHeight());

    Refresh();
    wxWindow::Update();
}

// This member function is the event handling function
// applied, when the user presses the left mouse button
void wxMGL::OnMouseLeftDown(wxMouseEvent &ev)
{
    long x=ev.GetX(), y=ev.GetY();

    if (!bZoomingMode && !bRotatingMode)
    {
        Refresh();
    }

    if (bZoomingMode)
        zoomactive = true;

    if (drawMode != DM_NONE)
    {
        start_x = x;
        start_y = y;
        drawModeActive = true;
    }

    xe = x0 = x;
    ye = y0 = y;
    ev.Skip();
}

// This member function is the event handling function
// applied, when the user presses any of the other mouse
// buttons
void wxMGL::OnMouseDown(wxMouseEvent &ev)
{
    xe = x0 = ev.GetX();
    ye = y0 = ev.GetY();
    ev.Skip();
}

// This member function is the event handling function
// applied, when the user releases the left mouse button
void wxMGL::OnMouseLeftUp(wxMouseEvent& event)
{
    // Apply the zooming
    if (bZoomingMode)
    {
        int w1 = GetSize().GetWidth(), h1 = GetSize().GetHeight();
        mreal _x1, _x2, _y1, _y2;
        _x1 = x1+(x2-x1)*(x0-GetPosition().x)/mreal(w1);
        _y1 = y2-(y2-y1)*(ye-GetPosition().y)/mreal(h1);
        _x2 = x1+(x2-x1)*(xe-GetPosition().x)/mreal(w1);
        _y2 = y2-(y2-y1)*(y0-GetPosition().y)/mreal(h1);
        x1 = _x1;
        x2 = _x2;
        y1 = _y1;
        y2 = _y2;

        if (x1 > x2)
        {
            _x1 = x1;
            x1 = x2;
            x2 = _x1;
        }

        if (y1 > y2)
        {
            _x1 = y1;
            y1 = y2;
            y2 = _x1;
        }

        x0 = xe;
        y0 = ye;
        zoomactive = false;
        Update();
    }

    // Refresh the drawing after rotation
    if (bRotatingMode)
    {
        gr->SetQuality(MGL_DRAW_NORM);
        skiprotate = 0;
        Update();
    }

    // Draw the current drawing object
    if (drawMode != DM_NONE)
    {
        drawModeActive = false;
        DrawCurrentObject(event.GetX(), event.GetY());
    }
}

// This member function is the event handling function
// applied, when the user releases the right mouse button
void wxMGL::OnMouseRightUp(wxMouseEvent &ev)
{
    if (popup && !bRotatingMode)
        PopupMenu(popup, ev.GetPosition());

    if (bRotatingMode)
    {
        gr->SetQuality(MGL_DRAW_NORM);
        skiprotate = 0;
        Update();
    }
}

// This member function is the event handling function
// applied, when the user moves the mouse
void wxMGL::OnMouseMove(wxMouseEvent &ev)
{
    long w = GetSize().GetWidth(), h = GetSize().GetHeight();
    xe = ev.GetX();
    ye = ev.GetY();
    mglPoint p = gr->CalcXYZ(xe, ye);

    if (zoomactive)
    {
        mglPoint start = gr->CalcXYZ(x0,y0);
        MousePos.Printf(wxT("[%.4g, %.4g] --> [%.4g, %.4g]"), start.x, start.y, p.x, p.y);
    }
    else if (drawModeActive && drawMode != DM_NONE && drawMode != DM_TEXT)
    {
        mglPoint start = gr->CalcXYZ(start_x, start_y);
        MousePos.Printf(wxT("[%.4g, %.4g] --> [%.4g, %.4g]"), start.x, start.y, p.x, p.y);
    }
    else
        MousePos.Printf(wxT("[%.4g, %.4g]"), p.x, p.y);

    // Apply the transformations related to the rotation
    // mode
    if (bRotatingMode && ev.ButtonIsDown(wxMOUSE_BTN_ANY))
    {
        gr->SetQuality(MGL_DRAW_DOTS);
        skiprotate++;

        // Apply the rotation effect
        if (ev.ButtonIsDown(wxMOUSE_BTN_LEFT))
        {
            mreal ff = 240/sqrt(mreal(w*h));
            dAzimutalViewPoint += int((x0-xe)*ff);
            dPolarViewPoint += int((y0-ye)*ff);

            if (dAzimutalViewPoint > 180)
                dAzimutalViewPoint -= 360;

            if (dAzimutalViewPoint < -180)
                dAzimutalViewPoint += 360;

            if (dPolarViewPoint > 180)
                dPolarViewPoint -= 360;

            if (dPolarViewPoint < -180)
                dPolarViewPoint += 360;
        }

        // Apply the perspective effect
        if (ev.ButtonIsDown(wxMOUSE_BTN_RIGHT))
        {
            mreal ff = 2.*(y0-ye)/w, gg = 0.5*(xe-x0)/h;
            mreal cx = (x1+x2)/2, cy = (y1+y2)/2;
            x1 = cx+(x1-cx)*exp(-ff);
            x2 = cx+(x2-cx)*exp(-ff);
            y1 = cy+(y1-cy)*exp(-ff);
            y2 = cy+(y2-cy)*exp(-ff);
            dPerspective = dPerspective + gg;

            if (dPerspective < 0)
                dPerspective = 0;

            if (dPerspective >= 1)
                dPerspective = 0.9999;
        }

        // Apply the shift effect (probably not used)
        if (ev.ButtonIsDown(wxMOUSE_BTN_MIDDLE))
        {
            mreal ff = 1./sqrt(mreal(w*h));
            mreal dx = (x0-xe)*ff*(x2-x1), dy = (y0-ye)*ff*(y2-y1);
            x1 += dx;
            x2 += dx;
            y1 -= dy;
            y2 -= dy;
        }

        x0 = xe;
        y0 = ye;
        Update();
    }

    if (bZoomingMode || drawMode != DM_NONE)
        Refresh(0);

    statusbar->SetStatusText(MousePos);
}

// This member function is the event handling function
// applied, when the user moves the mouse in the window
void wxMGL::OnEnter(wxMouseEvent& event)
{
    this->SetFocus();
    event.Skip();
}

// This member function is the event handling function
// applied, when the user presses a key
void wxMGL::OnKeyDown(wxKeyEvent& event)
{
    // connecting the ESC Key with closing the viewer
    if (event.GetKeyCode() == WXK_ESCAPE)
        m_parent->Close();

    // Start or stop the animation
    if (event.GetKeyCode() == ' ')
    {
        if (timer->IsRunning())
            Animation(false);
        else
            Animation(true);
    }

    // Next Slide
    if (event.GetKeyCode() == WXK_RIGHT || event.GetKeyCode() == WXK_DOWN)
        NextSlide();

    // Previous slide
    if (event.GetKeyCode() == WXK_LEFT || event.GetKeyCode() == WXK_UP)
        PrevSlide();
}

// This member function is the event handling function
// applied, when the user presses a navigation key
void wxMGL::OnNavigationKey(wxNavigationKeyEvent& event)
{
    // connect the navigation keys with next and previous image
    if (event.GetDirection())
        NextSlide();
    else
        PrevSlide();
}

// This member function is the event handling function
// for any events emitted by the toolbar
void wxMGL::OnMenuEvent(wxCommandEvent& event)
{
    switch (event.GetId())
    {
        case ID_GRAPH_EXPORT:
            OnExport();
            break;
        case ID_GRAPH_COPY:
            this->Copy();
            break;
        case ID_GRAPH_ALPHA:
            SetAlpha(!alpha);
            break;
        case ID_GRAPH_LIGHT:
            SetLight(!light);
            break;
        case ID_GRAPH_ROTATE:
            SetRotate(!bRotatingMode);
            break;
        case ID_GRAPH_ZOOM:
            SetZoom(!bZoomingMode);
            break;
        case ID_GRAPH_CIRCLE:
            SetDrawMode(DM_CIRCLE);
            break;
        case ID_GRAPH_RECT:
            SetDrawMode(DM_RECT);
            break;
        case ID_GRAPH_TEXT:
            SetDrawMode(DM_TEXT);
            break;
        case ID_GRAPH_LINE:
            SetDrawMode(DM_LINE);
            break;
        case ID_GRAPH_RESET:
            dAzimutalViewPoint = dPolarViewPoint = dPerspective = 0;
            x1 = y1 = 0;
            x2 = y2 = 1;
            Update();
            break;
        case ID_GRAPH_NEXT:
            NextSlide();
            break;
        case ID_GRAPH_PREVIOUS:
            PrevSlide();
            break;
        case ID_GRAPH_RUN:
            Animation(true);
            break;
        case ID_GRAPH_STOP:
            Animation(false);
            break;
    }
}

// This member function is the event handling function
// applied, when the window is closed
void wxMGL::OnClose(wxCloseEvent& event)
{
    // Stop the animation before closing
    Animation(false);
}

// This function sets the current bitmap either by
// explicit conversion from the RGB data in the mglGraph
// object or by selecting an image from the buffer
void wxMGL::setBitmap()
{
    if (animation && vAnimationBuffer.size() == (size_t)gr->GetNumFrame())
    {
        pic = wxBitmap(vAnimationBuffer[nFrameCounter]);
    }
    else if (animation)
    {
        vAnimationBuffer.emplace_back(ConvertFromGraph());
        pic = wxBitmap(vAnimationBuffer.back());
    }
    else
        pic = wxBitmap(ConvertFromGraph());
}

// This function is used to append the extension to the
// filename, if needed
wxString mglSetExtension(const wxString &fname, const wxString& ext)
{
    if (fname.find('.') == string::npos)
        return fname + "." + ext;
    return fname;
}
//-----------------------------------------------------------------------------
// NOTE: this is replacement for wxString::char_str() which is for v.2.8 or later
const char* mglw_str(const wxString &str)
{
    return str.c_str();
}
//-----------------------------------------------------------------------------
void wxMGL::ExportPNG(wxString fname)
{
    if (fname.IsEmpty())
        fname = ScriptName;
    if (fname.IsEmpty())
        wxMessageBox(appName, wxT("No filename."),wxOK|wxICON_ERROR ,this);
    else
        gr->WriteFrame(mglw_str(mglSetExtension(fname,"png")));
}
//-----------------------------------------------------------------------------
void wxMGL::ExportPNGs(wxString fname)
{
    if (fname.IsEmpty())
        fname = ScriptName;
    if (fname.IsEmpty())
        wxMessageBox(appName, wxT("No filename."),wxOK|wxICON_ERROR ,this);
    else
        gr->WriteFrame(mglw_str(mglSetExtension(fname,"png")));
    //mgl_write_png_solid(gr->Self(),mglw_str(mglSetExtension(fname,"png")), mglw_str(appName));
}
//-----------------------------------------------------------------------------
void wxMGL::ExportJPG(wxString fname)
{
    if (fname.IsEmpty())
        fname = ScriptName;
    if (fname.IsEmpty())
        wxMessageBox(appName, wxT("No filename."),wxOK|wxICON_ERROR ,this);
    else
        gr->WriteFrame(mglw_str(mglSetExtension(fname,"jpg")));
    //mgl_write_jpg(gr->Self(),mglw_str(mglSetExtension(fname,"jpg")), mglw_str(appName));
}
//-----------------------------------------------------------------------------
void wxMGL::ExportBPS(wxString fname)
{
    if(fname.IsEmpty())	fname = ScriptName;
    if(fname.IsEmpty())	wxMessageBox(appName, wxT("No filename."),wxOK|wxICON_ERROR ,this);
    else
    {
        setlocale(LC_ALL, "C");
        mgl_write_bps(gr->Self(),mglw_str(mglSetExtension(fname,"eps")), mglw_str(appName));
        setlocale(LC_ALL, "");
    }
}
//-----------------------------------------------------------------------------
void wxMGL::ExportEPS(wxString fname)
{
    if(fname.IsEmpty())
        fname = ScriptName;
    if(fname.IsEmpty())
        wxMessageBox(appName, wxT("No filename."),wxOK|wxICON_ERROR ,this);
    else
    {
        setlocale(LC_ALL, "C");
        gr->WriteFrame(mglw_str(mglSetExtension(fname,"eps")));
        //mgl_write_eps(gr->Self(),mglw_str(mglSetExtension(fname,"eps")), mglw_str(appName));
        setlocale(LC_ALL, "");
    }
}
//-----------------------------------------------------------------------------
void wxMGL::ExportSVG(wxString fname)
{
    if(fname.IsEmpty())
        fname = ScriptName;
    if(fname.IsEmpty())
        wxMessageBox(appName, wxT("No filename."),wxOK|wxICON_ERROR ,this);
    else
    {
        setlocale(LC_ALL, "C");
        gr->WriteFrame(mglw_str(mglSetExtension(fname,"svg")));
        //mgl_write_svg(gr->Self(),mglw_str(mglSetExtension(fname,"svg")), mglw_str(appName));
        setlocale(LC_ALL, "");
    }
}
//-----------------------------------------------------------------------------
void wxMGL::Copy()
{
    if (wxTheClipboard->Open())
    {
        wxTheClipboard->SetData( new wxBitmapDataObject(pic) );
        wxTheClipboard->Close();
    }
}
//-----------------------------------------------------------------------------
void wxMGL::SetSize(int w, int h)
{
    gr->SetSize(w, h, false);
    wxWindow::SetSize(w, h);
    Update();
}
//-----------------------------------------------------------------------------
void wxMGL::Adjust()
{
    wxSize sz=GetSize();
    gr->SetSize(sz.GetWidth(), sz.GetHeight(), false);
    Repaint();
}

// This member function displays the next slide
void wxMGL::NextSlide()
{
    if (gr->GetNumFrame() > 1)
    {
        // Fill the animation buffer, if needed
        if (vAnimationBuffer.size() < (size_t)gr->GetNumFrame())
        {
            gr->GetFrame(nFrameCounter);

            if (animation)
                statusbar->SetStatusText(_guilang.get("GUI_GRAPH_RENDERING"), 1);
        }
        else if (animation)
            statusbar->SetStatusText(_guilang.get("GUI_GRAPH_RENDERING_DONE"), 1);

        nFrameCounter++;

        if (nFrameCounter >= gr->GetNumFrame())
            nFrameCounter = 0;

        wxString slidescount;
        slidescount.Printf("%d / %d", nFrameCounter+1, gr->GetNumFrame());
        statusbar->SetStatusText(slidescount, 2);
        Update();
    }
}

// This member function displays the previous slide
void wxMGL::PrevSlide()
{
    if (gr->GetNumFrame() > 1)
    {
        nFrameCounter--;

        if (nFrameCounter < 0)
            nFrameCounter = gr->GetNumFrame()-1;

        gr->GetFrame(nFrameCounter);
        wxString slidescount;
        slidescount.Printf("%d / %d", nFrameCounter+1, gr->GetNumFrame());
        statusbar->SetStatusText(slidescount, 2);
        Update();
    }
}

// This member function starts or stops the animation. It
// will also dis- or enable the corresponding tools from
// the toolbar
void wxMGL::Animation(bool st)
{
    if (gr->GetNumFrame() <= 1)
        return;

    if (st)
    {
        bZoomingMode = zoomactive = bRotatingMode = false;
        skiprotate = 0;
        SetDrawMode(DM_NONE);
        toptoolbar->EnableTool(ID_GRAPH_EXPORT, false);
        toptoolbar->EnableTool(ID_GRAPH_COPY, false);
        toptoolbar->EnableTool(ID_GRAPH_ROTATE, false);
        toptoolbar->EnableTool(ID_GRAPH_ZOOM, false);
        toptoolbar->EnableTool(ID_GRAPH_RESET, false);

        toptoolbar->EnableTool(ID_GRAPH_NEXT, false);
        toptoolbar->EnableTool(ID_GRAPH_PREVIOUS, false);
        toptoolbar->EnableTool(ID_GRAPH_RUN, false);
        toptoolbar->EnableTool(ID_GRAPH_STOP, true);

        for (int i = ID_GRAPH_DRAW_FIRST+1; i < ID_GRAPH_DRAW_LAST; i++)
            toptoolbar->EnableTool(i, false);

        nFrameCounter = 0;
        animation = true;

        timer->Start(40);
    }
    else
    {
        timer->Stop();
        toptoolbar->EnableTool(ID_GRAPH_EXPORT, true);
        toptoolbar->EnableTool(ID_GRAPH_COPY, true);
        toptoolbar->EnableTool(ID_GRAPH_ROTATE, true);
        toptoolbar->EnableTool(ID_GRAPH_ZOOM, true);
        toptoolbar->EnableTool(ID_GRAPH_RESET, true);

        toptoolbar->EnableTool(ID_GRAPH_NEXT, true);
        toptoolbar->EnableTool(ID_GRAPH_PREVIOUS, true);
        toptoolbar->EnableTool(ID_GRAPH_RUN, true);
        toptoolbar->EnableTool(ID_GRAPH_STOP, false);

        for (int i = ID_GRAPH_DRAW_FIRST+1; i < ID_GRAPH_DRAW_LAST; i++)
            toptoolbar->EnableTool(i, true);

        animation = false;

        statusbar->SetStatusText("", 1);

        if (vAnimationBuffer.size())
        {
            vAnimationBuffer.clear();
            vAnimationBuffer.shrink_to_fit();
        }
    }
}

