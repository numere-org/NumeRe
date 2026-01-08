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
#include "guilang.hpp"
#include "../kernel/core/utils/stringtools.hpp"

#include "terminal/terminal.hpp"
#include "graphviewer.hpp"

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
//    EVT_ENTER_WINDOW(wxMGL::OnEnter)
    EVT_LEAVE_WINDOW(wxMGL::OnLeave)
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






/////////////////////////////////////////////////
/// \brief wxMGL constructor.
///
/// \param parent wxWindow*
/// \param id wxWindowID
/// \param pos const wxPoint&
/// \param size const wxSize&
/// \param style long
/// \param frameless bool
/// \param name const wxString&
///
/////////////////////////////////////////////////
wxMGL::wxMGL(wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, bool frameless, const wxString& name) : wxWindow(parent, id, pos, size, style, name)
{
    AutoResize = true;
    draw_par = nullptr;
    draw_func = 0;
    gr = nullptr;
    popup = nullptr;
    draw_cl = nullptr;
    dAzimutalViewPoint = dPolarViewPoint = dPerspective = 0.0;
    zoom_x0 = zoom_y0 = 0;
    zoom_x1 = zoom_y1 = 1;
    l_x1 = l_y1 = 0;
    start_x = start_y = 0;
    alpha = light = bZoomingMode = zoomactive = bRotatingMode = false;
    drawMode = DM_NONE;
    drawModeActive = false;
    skiprotate = 0;
    nFrameCounter = 0;
    nFramesToSkip = 2;
    timer = new wxTimer(this, TIMER_ID);
    animation = false;

    SetOwnBackgroundColour(*wxWHITE);

    if (!frameless)
    {
        m_parentFrame = static_cast<wxFrame*>(parent);
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

        NumeReWindow* app = static_cast<NumeReWindow*>(m_parentFrame->GetParent());
        statusbar = m_parentFrame->CreateStatusBar(3);
        statusbar->SetBackgroundColour(app->getOptions()->GetSyntaxStyle(Options::UI_THEME).foreground.ChangeLightness(Options::STATUSBAR));
        int nWidths[] = {-2,-1,-1};
        statusbar->SetFieldsCount(3, nWidths);

        // Define the minimal possible client size, below which any resizing
        // action is prohibited.
        m_parentFrame->SetMinClientSize(wxSize(550,100));
    }
    else
        m_parentFrame = nullptr;
}


/////////////////////////////////////////////////
/// \brief Destructor: stops the timer, if it is
/// running and deletes the referenced drawing
/// class.
/////////////////////////////////////////////////
wxMGL::~wxMGL()
{
    timer->Stop();

    if (draw_cl)
        delete draw_cl;

    draw_cl = nullptr;
}


/////////////////////////////////////////////////
/// \brief This member function dis- or enables
/// the animation tools depending on whether an
/// animation is available.
///
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::UpdateTools()
{
    if (!m_parentFrame)
        return;

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


/////////////////////////////////////////////////
/// \brief This member function returns the image
/// aspect ratio.
///
/// \return double
///
/////////////////////////////////////////////////
double wxMGL::GetRatio()
{
    return double(mgl_get_width(gr->Self())) / mgl_get_height(gr->Self());
}


/////////////////////////////////////////////////
/// \brief This member function initializes the
/// toolbar of the GraphViewer Window.
///
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::InitializeToolbar()
{
    if (!m_parentFrame)
        return;

    NumeReWindow* app = static_cast<NumeReWindow*>(m_parentFrame->GetParent());

    toptoolbar = m_parentFrame->CreateToolBar(wxTB_HORIZONTAL | wxTB_FLAT);
    toptoolbar->SetBackgroundColour(app->getOptions()->GetSyntaxStyle(Options::UI_THEME).background.ChangeLightness(Options::TOOLBAR));

    toptoolbar->AddTool(ID_GRAPH_EXPORT, _guilang.get("GUI_GRAPH_EXPORT"), app->getToolbarIcon("save"), _guilang.get("GUI_GRAPH_EXPORT"));
    toptoolbar->AddTool(ID_GRAPH_COPY, _guilang.get("GUI_GRAPH_COPY"), app->getToolbarIcon("copy"), _guilang.get("GUI_GRAPH_COPY"));
    toptoolbar->AddSeparator();

    toptoolbar->AddTool(ID_GRAPH_ROTATE, _guilang.get("GUI_GRAPH_ROTATE"), app->getToolbarIcon("rotate"), _guilang.get("GUI_GRAPH_ROTATE"), wxITEM_CHECK);
    toptoolbar->AddTool(ID_GRAPH_ZOOM, _guilang.get("GUI_GRAPH_ZOOM"), app->getToolbarIcon("zoom"), _guilang.get("GUI_GRAPH_ZOOM"), wxITEM_CHECK);
    toptoolbar->AddTool(ID_GRAPH_RESET, _guilang.get("GUI_GRAPH_RESET"), app->getToolbarIcon("undo"), _guilang.get("GUI_GRAPH_RESET"));
    toptoolbar->AddSeparator();

    toptoolbar->AddTool(ID_GRAPH_PREVIOUS, _guilang.get("GUI_GRAPH_PREVIOUS"), app->getToolbarIcon("back"), _guilang.get("GUI_GRAPH_PREVIOUS"));
    toptoolbar->AddTool(ID_GRAPH_NEXT, _guilang.get("GUI_GRAPH_NEXT"), app->getToolbarIcon("forward"), _guilang.get("GUI_GRAPH_NEXT"));
    toptoolbar->AddTool(ID_GRAPH_RUN, _guilang.get("GUI_GRAPH_RUN"), app->getToolbarIcon("run"), _guilang.get("GUI_GRAPH_RUN"));
    toptoolbar->AddTool(ID_GRAPH_STOP, _guilang.get("GUI_GRAPH_STOP"), app->getToolbarIcon("stop"), _guilang.get("GUI_GRAPH_STOP"));
    toptoolbar->EnableTool(ID_GRAPH_STOP, false);
    toptoolbar->AddSeparator();

    toptoolbar->AddTool(ID_GRAPH_LINE, _guilang.get("GUI_GRAPH_LINE"),  app->getToolbarIcon("add-line"), _guilang.get("GUI_GRAPH_LINE"), wxITEM_CHECK);
    toptoolbar->AddTool(ID_GRAPH_RECT, _guilang.get("GUI_GRAPH_RECT"),  app->getToolbarIcon("add-rect"), _guilang.get("GUI_GRAPH_RECT"), wxITEM_CHECK);
    toptoolbar->AddTool(ID_GRAPH_CIRCLE, _guilang.get("GUI_GRAPH_CIRCLE"),  app->getToolbarIcon("add-circ"), _guilang.get("GUI_GRAPH_CIRCLE"), wxITEM_CHECK);
    toptoolbar->AddTool(ID_GRAPH_TEXT, _guilang.get("GUI_GRAPH_TEXT"), app->getToolbarIcon("add-text"), _guilang.get("GUI_GRAPH_TEXT"), wxITEM_CHECK);

    // Create a narrow text ctrl to insert the styling information
    styling = new wxTextCtrl(toptoolbar, wxID_ANY, "B-__", wxDefaultPosition, wxSize(60,-1));

    toptoolbar->AddControl(styling, "STYLE");
    toptoolbar->Realize();
}


/////////////////////////////////////////////////
/// \brief This member function displays the file
/// dialog during exporting the current image
/// displayed in the window.
///
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::OnExport()
{
    if (!m_parentFrame)
        return;

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


/////////////////////////////////////////////////
/// \brief This member function is the redrawing
/// and painting event handler. It will display
/// the bitmap currently selected by the
/// wxMGL::Repaint() function.
///
/// \param event wxPaintEvent&
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);

    if (!gr && !draw_cl)
    {
        dc.SetBackground(wxBrush(*wxWHITE));
        dc.Clear();
        return;
    }

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
        dc.DrawRectangle(mouse_x0, mouse_y0, mouse_x1-mouse_x0, mouse_y1-mouse_y0);

    // Draw the previews of the drawing objects
    if (drawModeActive)
    {
        switch (drawMode)
        {
            case DM_LINE:
                dc.DrawLine(start_x, start_y, mouse_x1, mouse_y1);
                break;
            case DM_CIRCLE:
                dc.DrawCircle(start_x, start_y, hypot(mouse_x1-start_x, mouse_y1-start_y));
                break;
            case DM_RECT:
                dc.DrawRectangle(start_x, start_y, mouse_x1-start_x, mouse_y1-start_y);
                break;

        }
    }
}


/////////////////////////////////////////////////
/// \brief This member function handles the
/// background erasing process. It is disabled to
/// avoid flickering (kind of a hack).
///
/// \param event wxEraseEvent&
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::OnEraseBackground(wxEraseEvent& event)
{
    // Empty method to avoid flickering
}


/////////////////////////////////////////////////
/// \brief This member function is the resizing
/// event handler.
///
/// \param event wxSizeEvent&
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::OnSize(wxSizeEvent& event)
{
    wxSize ev = event.GetSize();

    // Do nothing, if the graph does not exist
    if (!gr)
    {
        wxWindow::SetSize(ev);
        Refresh();
        return;
    }

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


/////////////////////////////////////////////////
/// \brief This member function is the timer
/// event handling function needed for the
/// animation.
///
/// \param wxTimerEvent&
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::OnNextSlide(wxTimerEvent& )
{
    NextSlide();
}


/////////////////////////////////////////////////
/// \brief This member function can be used to
/// set the perspective effect applied to the
/// drawing.
///
/// \param p int
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::SetPer(int p)
{
    // Convert the integer percentage into a double value
    if (100*dPerspective != p && p >= 0 && p < 100)
    {
        dPerspective = 0.01*p;
        Repaint();
    }
}


/////////////////////////////////////////////////
/// \brief This member function can be used to
/// set the azimutal viewpoint of the drawing.
///
/// \param p int
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::SetPhi(int p)
{
    if (dAzimutalViewPoint != p)
    {
        dAzimutalViewPoint = p;
        Repaint();
    }
}


/////////////////////////////////////////////////
/// \brief This member function can be used to
/// set the polar viewpoint of the drawing.
///
/// \param t int
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::SetTet(int t)
{
    if (dPolarViewPoint != t)
    {
        dPolarViewPoint = t;
        Repaint();
    }
}


/////////////////////////////////////////////////
/// \brief This member function can be used to
/// (de-)activate the zooming mode.
///
/// \param z bool
/// \return void
///
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief This member function can be used to
/// (de-)activate the rotating mode.
///
/// \param r bool
/// \return void
///
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief This member function selects the
/// current drawing mode by dis- and enabling the
/// tools and the other modification modes.
///
/// \param dm int
/// \return void
///
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief This member function draws the drawing
/// object selected by the toolbar after the user
/// released the left mouse button.
///
/// \param end_x int
/// \param end_y int
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::DrawCurrentObject(int end_x, int end_y)
{
    // Get the starting and ending postions
    mglPoint startPoint = gr->CalcXYZ(start_x, start_y);
    mglPoint endPoint = gr->CalcXYZ(end_x, end_y);

    // Get the style from the toolbar
    std::string style = styling->GetLineText(0).ToAscii().data();

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
                gr->Putsw(startPoint, text.ToStdWstring().c_str(), style.c_str());
                Update();
            }

            break;
    }
}


/////////////////////////////////////////////////
/// \brief This member function can be used to
/// reset the view modifications completely.
///
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::Restore()
{
    SetPhi(0);
    SetTet(0);
    SetPer(0);
    zoom_x0 = zoom_y0 = 0;
    zoom_x1 = zoom_y1 = 1;
    bZoomingMode = bRotatingMode = false;
    Repaint();
}


/////////////////////////////////////////////////
/// \brief Applies a zoom with the factor of two
/// around the selected position (passed as pixel
/// coordinates).
///
/// \param x int
/// \param y int
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::ZoomIn(int x, int y)
{
    double w_off = 2.0*x/double(GetSize().GetWidth())-1.0, h_off = -2.0*y/double(GetSize().GetHeight())+1.0;
    mreal d;
    d = (zoom_y1-zoom_y0)/4;
    zoom_y0 = zoom_y0 + d + 2*h_off*d;
    zoom_y1 = zoom_y1 - d + 2*h_off*d;
    d = (zoom_x1-zoom_x0)/4;
    zoom_x0 = zoom_x0 + d + 2*w_off*d;
    zoom_x1 = zoom_x1 - d + 2*w_off*d;
    Repaint();
}


/////////////////////////////////////////////////
/// \brief Applies a zoom with the factor of one
/// half around the selected position (passed as
/// pixel coordinates).
///
/// \param x int
/// \param y int
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::ZoomOut(int x, int y)
{
    double w_off = 2.0*x/double(GetSize().GetWidth())-1.0, h_off = -2.0*y/double(GetSize().GetHeight())+1.0;
    mreal d;
    d = (zoom_y1-zoom_y0)/2;
    zoom_y0 = zoom_y0 - d - h_off*d;
    zoom_y1 = zoom_y1 + d - h_off*d;
    d = (zoom_x1-zoom_x0)/2;
    zoom_x0 = zoom_x0 - d - w_off*d;
    zoom_x1 = zoom_x1 + d - w_off*d;
    Repaint();
}


/////////////////////////////////////////////////
/// \brief This member function tells the
/// internal mglGraph object to draw its contents.
///
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::Update()
{
    if (!draw_cl && !gr)
        return;

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


/////////////////////////////////////////////////
/// \brief This member function converts the RGB
/// image created by the mglGraph object into a
/// platform-independent wxImage object, which
/// can be used to create a bitmap from.
///
/// \return wxImage
///
/////////////////////////////////////////////////
wxImage wxMGL::ConvertFromGraph()
{
    const unsigned char *bb = mgl_get_rgb(gr->Self());
    int w = mgl_get_width(gr->Self());
    int h = mgl_get_height(gr->Self());
    unsigned char *tmp = (unsigned char*)malloc(3*w*h);
    memcpy(tmp, bb, 3*w*h);
    return wxImage(w, h, tmp);
}


/////////////////////////////////////////////////
/// \brief Convert a mglPoint to a string
/// representation.
///
/// \param p mglPoint&
/// \return std::string
///
/////////////////////////////////////////////////
std::string wxMGL::pointToString(mglPoint& p) const
{
    std::string sFormatted = "[";

    if (timeAxes[0].use)
        sFormatted += toString(to_timePoint(p.x), GET_SHORTEST);
    else
        sFormatted += toString(p.x, 5);

    if (timeAxes[1].use)
        sFormatted += " / " + toString(to_timePoint(p.y), GET_SHORTEST);
    else
        sFormatted += " / " + toString(p.y, 5);

    return sFormatted + "]";
}


/////////////////////////////////////////////////
/// \brief This member function sets the current
/// drawing as new bitmap and applies the view
/// modifications in advance.
///
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::Repaint()
{
    if (!draw_cl && !gr)
        return;

    if (bRotatingMode && (skiprotate % nFramesToSkip))
        return;

    if (bZoomingMode)
        gr->Zoom(zoom_x0, zoom_y0, zoom_x1, zoom_y1);

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


/////////////////////////////////////////////////
/// \brief This member function is the event
/// handling function applied, when the user
/// presses the left mouse button.
///
/// \param ev wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
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

    mouse_x1 = mouse_x0 = x;
    mouse_y1 = mouse_y0 = y;
    ev.Skip();

    // Forward the mouse event to the custom window
    if (!m_parentFrame)
    {
        wxMouseEvent event(ev);
        m_parent->GetEventHandler()->ProcessEvent(event);
    }
}


/////////////////////////////////////////////////
/// \brief This member function is the event
/// handling function applied, when the user
/// presses any of the other mouse buttons.
///
/// \param ev wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::OnMouseDown(wxMouseEvent &ev)
{
    mouse_x1 = mouse_x0 = ev.GetX();
    mouse_y1 = mouse_y0 = ev.GetY();
    ev.Skip();
}


/////////////////////////////////////////////////
/// \brief This member function is the event
/// handling function applied, when the user
/// releases the left mouse button.
///
/// \param ev wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::OnMouseLeftUp(wxMouseEvent& ev)
{
    // Apply the zooming
    if (bZoomingMode)
    {
        if (mouse_x0 == mouse_x1 || mouse_y0 == mouse_y1)
        {
            ZoomIn(mouse_x0, mouse_y0);
            mouse_x0 = mouse_x1;
            mouse_y0 = mouse_y1;
            zoomactive = false;
            return;
        }

        int w1 = GetSize().GetWidth(), h1 = GetSize().GetHeight();
        mreal _x1, _x2, _y1, _y2;

        _x1 = zoom_x0 + (zoom_x1-zoom_x0) * mouse_x0/mreal(w1);
        _y1 = zoom_y1 - (zoom_y1-zoom_y0) * mouse_y1/mreal(h1);
        _x2 = zoom_x0 + (zoom_x1-zoom_x0) * mouse_x1/mreal(w1);
        _y2 = zoom_y1 - (zoom_y1-zoom_y0) * mouse_y0/mreal(h1);

        zoom_x0 = _x1;
        zoom_x1 = _x2;
        zoom_y0 = _y1;
        zoom_y1 = _y2;

        if (zoom_x0 > zoom_x1)
        {
            _x1 = zoom_x0;
            zoom_x0 = zoom_x1;
            zoom_x1 = _x1;
        }

        if (zoom_y0 > zoom_y1)
        {
            _x1 = zoom_y0;
            zoom_y0 = zoom_y1;
            zoom_y1 = _x1;
        }

        mouse_x0 = mouse_x1;
        mouse_y0 = mouse_y1;
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
        DrawCurrentObject(ev.GetX(), ev.GetY());
    }
}


/////////////////////////////////////////////////
/// \brief This member function is the event
/// handling function applied, when the user
/// releases the right mouse button.
///
/// \param ev wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
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
    else if (bZoomingMode)
    {
        ZoomOut(mouse_x0, mouse_y0);
        mouse_x0 = mouse_x1;
        mouse_y0 = mouse_y1;
        zoomactive = false;
    }
}


/////////////////////////////////////////////////
/// \brief This member function is the event
/// handling function applied, when the user
/// moves the mouse.
///
/// \param ev wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::OnMouseMove(wxMouseEvent &ev)
{
    if (!m_parentFrame)
        return;

    long w = GetSize().GetWidth(), h = GetSize().GetHeight();
    mouse_x1 = ev.GetX();
    mouse_y1 = ev.GetY();
    mglPoint p = gr->CalcXYZ(mouse_x1, mouse_y1);

    std::string sFormatted = pointToString(p);

    if (zoomactive)
    {
        mglPoint start = gr->CalcXYZ(mouse_x0, mouse_y0);
        sFormatted = pointToString(start) + " --> " + sFormatted;
    }
    else if (drawModeActive && drawMode != DM_NONE && drawMode != DM_TEXT)
    {
        mglPoint start = gr->CalcXYZ(start_x, start_y);
        sFormatted = pointToString(start) + " --> " + sFormatted;
    }

    MousePos = sFormatted;

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
            dAzimutalViewPoint += int((mouse_x0-mouse_x1)*ff);
            dPolarViewPoint += int((mouse_y0-mouse_y1)*ff);

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
            mreal ff = 2.*(mouse_y0-mouse_y1)/w, gg = 0.5*(mouse_x1-mouse_x0)/h;
            mreal cx = (zoom_x0+zoom_x1)/2, cy = (zoom_y0+zoom_y1)/2;
            zoom_x0 = cx+(zoom_x0-cx)*exp(-ff);
            zoom_x1 = cx+(zoom_x1-cx)*exp(-ff);
            zoom_y0 = cy+(zoom_y0-cy)*exp(-ff);
            zoom_y1 = cy+(zoom_y1-cy)*exp(-ff);
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
            mreal dx = (mouse_x0-mouse_x1)*ff*(zoom_x1-zoom_x0), dy = (mouse_y0-mouse_y1)*ff*(zoom_y1-zoom_y0);
            zoom_x0 += dx;
            zoom_x1 += dx;
            zoom_y0 -= dy;
            zoom_y1 -= dy;
        }

        mouse_x0 = mouse_x1;
        mouse_y0 = mouse_y1;
        Update();
    }

    if (bZoomingMode || drawMode != DM_NONE)
        Refresh(0);

    statusbar->SetStatusText(MousePos);
}


/////////////////////////////////////////////////
/// \brief This member function is the event
/// handling function applied, when the user
/// moves the mouse in the window.
///
/// \param event wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::OnEnter(wxMouseEvent& event)
{
    if (m_parentFrame)
        this->SetFocus();

    event.Skip();
}


/////////////////////////////////////////////////
/// \brief Event handler for leaving the window.
/// Will change the drawing mode to normal and
/// redraw the windows contents.
///
/// \param event wxMouseEvent&
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::OnLeave(wxMouseEvent& event)
{
    if (bRotatingMode)
    {
        gr->SetQuality(MGL_DRAW_NORM);
        skiprotate = 0;
        Update();
    }
}


/////////////////////////////////////////////////
/// \brief This member function is the event
/// handling function applied, when the user
/// presses a key.
///
/// \param event wxKeyEvent&
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::OnKeyDown(wxKeyEvent& event)
{
    if (!m_parentFrame)
    {
        event.Skip();
        return;
    }
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


/////////////////////////////////////////////////
/// \brief This member function is the event
/// handling function applied, when the user
/// presses a navigation key.
///
/// \param event wxNavigationKeyEvent&
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::OnNavigationKey(wxNavigationKeyEvent& event)
{
    if (!m_parentFrame)
    {
        event.Skip();
        return;
    }

    // connect the navigation keys with next and previous image
    if (event.GetDirection())
        NextSlide();
    else
        PrevSlide();
}


/////////////////////////////////////////////////
/// \brief This member function is the event
/// handling function for any events emitted by
/// the toolbar.
///
/// \param event wxCommandEvent&
/// \return void
///
/////////////////////////////////////////////////
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
            //SetAlpha(!alpha);
            break;
        case ID_GRAPH_LIGHT:
            //SetLight(!light);
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
            zoom_x0 = zoom_y0 = 0;
            zoom_x1 = zoom_y1 = 1;
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


/////////////////////////////////////////////////
/// \brief This member function is the event
/// handling function applied, when the window is
/// closed.
///
/// \param event wxCloseEvent&
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::OnClose(wxCloseEvent& event)
{
    // Stop the animation before closing
    Animation(false);
}


/////////////////////////////////////////////////
/// \brief This function sets the current bitmap
/// either by explicit conversion from the RGB
/// data in the mglGraph object or by selecting
/// an image from the buffer.
///
/// \return void
///
/////////////////////////////////////////////////
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


/////////////////////////////////////////////////
/// \brief This function is used to append the
/// extension to the filename, if needed.
///
/// \param fname const wxString&
/// \param ext const wxString&
/// \return wxString
///
/////////////////////////////////////////////////
wxString mglSetExtension(const wxString &fname, const wxString& ext)
{
    if (fname.find('.') == std::string::npos)
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
        gr->WritePNG(mglw_str(mglSetExtension(fname,"png")), "", false);
}
//-----------------------------------------------------------------------------
void wxMGL::ExportPNGs(wxString fname)
{
    if (fname.IsEmpty())
        fname = ScriptName;
    if (fname.IsEmpty())
        wxMessageBox(appName, wxT("No filename."),wxOK|wxICON_ERROR ,this);
    else
        gr->WritePNG(mglw_str(mglSetExtension(fname,"png")), "", false);
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


/////////////////////////////////////////////////
/// \brief This member function displays the next
/// slide.
///
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::NextSlide()
{
    if (gr->GetNumFrame() > 1)
    {
        // Fill the animation buffer, if needed
        if (vAnimationBuffer.size() < (size_t)gr->GetNumFrame())
        {
            gr->GetFrame(nFrameCounter);

            if (animation && m_parentFrame)
                statusbar->SetStatusText(_guilang.get("GUI_GRAPH_RENDERING"), 1);
        }
        else if (animation && m_parentFrame)
            statusbar->SetStatusText(_guilang.get("GUI_GRAPH_RENDERING_DONE"), 1);

        nFrameCounter++;

        if (nFrameCounter >= gr->GetNumFrame())
            nFrameCounter = 0;

        if (m_parentFrame)
        {
            wxString slidescount;
            slidescount.Printf("%d / %d", nFrameCounter+1, gr->GetNumFrame());
            statusbar->SetStatusText(slidescount, 2);
        }

        Update();
    }
}


/////////////////////////////////////////////////
/// \brief This member function displays the
/// previous slide.
///
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::PrevSlide()
{
    if (gr->GetNumFrame() > 1)
    {
        nFrameCounter--;

        if (nFrameCounter < 0)
            nFrameCounter = gr->GetNumFrame()-1;

        gr->GetFrame(nFrameCounter);

        if (m_parentFrame)
        {
            wxString slidescount;
            slidescount.Printf("%d / %d", nFrameCounter+1, gr->GetNumFrame());
            statusbar->SetStatusText(slidescount, 2);
        }

        Update();
    }
}


/////////////////////////////////////////////////
/// \brief This member function starts or stops
/// the animation. It will also dis- or enable
/// the corresponding tools from the toolbar.
///
/// \param st bool
/// \return void
///
/////////////////////////////////////////////////
void wxMGL::Animation(bool st)
{
    if (!gr || gr->GetNumFrame() <= 1)
        return;

    if (st)
    {
        if (m_parentFrame)
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
        }

        nFrameCounter = 0;
        animation = true;

        timer->Start(40);
    }
    else
    {
        timer->Stop();

        if (m_parentFrame)
        {
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

            statusbar->SetStatusText("", 1);
        }

        animation = false;

        if (vAnimationBuffer.size())
        {
            vAnimationBuffer.clear();
            vAnimationBuffer.shrink_to_fit();
        }
    }
}


/////////////////////////////////////////////////
/// \brief Return the clicked coordinates as a
/// mglPoint. This is used by the custom
/// GUI event handler.
///
/// \return mglPoint
///
/////////////////////////////////////////////////
mglPoint wxMGL::getClickedCoords()
{
    if (gr)
        return gr->CalcXYZ(mouse_x0, mouse_y0);

    return mglPoint(NAN, NAN, NAN);
}


