/***************************************************************************
 * wx.h.cpp is part of Math Graphic Library
 * Copyright (C) 2007-2014 Alexey Balakin <mathgl.abalakin@gmail.ru>       *
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
#ifndef MGL_WX_H
#define MGL_WX_H
//-----------------------------------------------------------------------------
#include <mgl2/wnd.h>
#include <wx/window.h>
#include <wx/image.h>
#include <wx/timer.h>
#include <wx/bitmap.h>
#include <wx/toolbar.h>
#include <wx/statusbr.h>
#include <wx/textctrl.h>
#include <wx/cursor.h>
#include "../kernel/core/interval.hpp"

#include <vector>

class mglCanvas;
//-----------------------------------------------------------------------------
/// Class is Wx widget which display MathGL graphics
class wxMGL : public wxWindow
{
public:
	wxString appName;	///< Application name for message boxes
	bool AutoResize; 	///< Allow auto resizing (default is false)
	TimeAxis timeAxes[4];

	wxMGL(wxWindow* parent, wxWindowID id = -1, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = 0, bool frameless = false, const wxString& name = wxPanelNameStr);
	virtual ~wxMGL();
	double GetRatio();
	void SetPopup(wxMenu *p)	{	popup = p;	};	///< Set popup menu pointer
	void SetSize(int w, int h);		///< Set window/picture sizes
	void SetGraph(mglGraph* GR)
	{
        if (gr)
            delete gr;
        gr = GR;
        UpdateTools();
    }

	inline HMGL GetGraph()	{	return (HMGL)gr;	}

	/// Set drawing functions and its parameter
	inline void SetDraw(int (*func)(mglBase *gr, void *par), void *par=0)
	{	draw_func = func;	draw_par = par;	}
	inline void SetDraw(mglDraw *dr)
	{
	    if (draw_cl)
        {
            delete draw_cl;
            gr = nullptr;
        }

	    draw_cl = dr;
    }
	inline void SetDraw(int (*draw)(mglGraph *gr))
	{	SetDraw(draw?mgl_draw_graph:0,(void*)draw);	}
	inline void ZoomRegion(mreal xx1,mreal xx2,mreal yy1, mreal yy2)
	{	zoom_x0=xx1;	zoom_y0=yy1;	zoom_x1=xx2;	zoom_y1=yy2;	}

	int GetPer() 	{return dPerspective;};		///< Get perspective value
	int GetPhi() 	{return dAzimutalViewPoint;};		///< Get Phi-angle value
	int GetTet() 	{return dPolarViewPoint;};		///< Get Theta-angle value
	bool GetAlpha()	{return alpha;};	///< Get transparency state
	bool GetLight()	{return light;};	///< Get lightning state
	bool GetZoom()	{return bZoomingMode;};		///< Get mouse zooming state
	bool GetRotate()	{return bRotatingMode;};	///< Get mouse rotation state

	void Repaint();
	void Update();			///< Update picture
	void Copy(); 			///< copy graphics to clipboard
	void Print();			///< Print plot
//	void Stop();			///< Stop execution
	void SetPer(int p);		///< Set perspective value
	void SetPhi(int p);		///< Set Phi-angle value
	void SetTet(int t);		///< Set Theta-angle value
//	void SetAlpha(bool a);	///< Switch on/off transparency
//	void SetLight(bool l);	///< Switch on/off lightning
	void SetZoom(bool z);	///< Switch on/off mouse zooming
	void SetRotate(bool r);	///< Switch on/off mouse rotation
	void SetDrawMode(int dm);
	void ZoomIn(int x, int y);			///< Zoom in graphics
	void ZoomOut(int x, int y);			///< Zoom out graphics
	void Restore();			///< Restore zoom and rotation to default values
//	void Reload();			///< Reload data and execute script
//	void ShiftLeft();		///< Shift graphics to left direction
//	void ShiftRight();		///< Shift graphics to right direction
//	void ShiftUp();			///< Shift graphics to up direction
//	void ShiftDown();		///< Shift graphics to down direction
	void DrawCurrentObject(int end_x, int end_y);
	void ExportPNG(wxString fname = L"");	///< export to PNG file
	void ExportPNGs(wxString fname = L"");	///< export to PNG file (no transparency)
	void ExportJPG(wxString fname = L"");	///< export to JPEG file
	void ExportBPS(wxString fname = L"");	///< export to bitmap EPS file
	void ExportEPS(wxString fname = L"");	///< export to vector EPS file
	void ExportSVG(wxString fname = L"");	///< export to SVG file

	void Adjust();		///< Adjust plot size to fill entire window
	int getNumFrames()
	{
	    if (gr)
            return gr->GetNumFrame();

        return 0;
	}
	void NextSlide();	///< Show next slide
	void PrevSlide();	///< Show previous slide
	void Animation(bool st=true);	///< Start animation
	void AnimateAsynch()
	{
	    CallAfter(&Animation, true);
	}

	enum CurrentDrawMode
	{
        DM_NONE = 0,
        DM_LINE = 1,
        DM_RECT,
        DM_CIRCLE,
        DM_TEXT
	};

	mglPoint getClickedCoords();

protected:
    void InitializeToolbar();
    void OnExport();
	void OnPaint(wxPaintEvent& event);
	void OnEraseBackground(wxEraseEvent& event);
	void OnSize(wxSizeEvent& event);
	void OnNextSlide(wxTimerEvent& evt);	///< Show next slide
	void OnMouseLeftDown(wxMouseEvent &ev);
	void OnMouseDown(wxMouseEvent &ev);
	void OnMouseLeftUp(wxMouseEvent &ev);
	void OnMouseRightUp(wxMouseEvent &ev);
	void OnMouseMove(wxMouseEvent &ev);

	void OnEnter(wxMouseEvent& event);
	void OnLeave(wxMouseEvent& event);
	void OnKeyDown(wxKeyEvent& event);
	void OnNavigationKey(wxNavigationKeyEvent& event);
	void OnMenuEvent(wxCommandEvent& event);
	void OnClose(wxCloseEvent& event);

	void setBitmap();
	wxImage ConvertFromGraph();
	std::string pointToString(mglPoint& p) const;


	mglGraph *gr;		///< pointer to grapher
	void *draw_par;		///< Parameters for drawing function mglCanvasWnd::DrawFunc.
	/// Drawing function for window procedure. It should return the number of frames.
	int (*draw_func)(mglBase *gr, void *par);
	mglDraw *draw_cl;

	wxString MousePos;	///< Last mouse position
	wxBitmap pic;		///< Pixmap for drawing (changed by update)
	double dPolarViewPoint, dAzimutalViewPoint;	///< Rotation angles
	double dPerspective;			///< Value of perspective ( must be in [0,1) )
	bool alpha;			///< Transparency state
	bool light;			///< Lightning state
	bool bZoomingMode;			///< Mouse zoom state
	bool bRotatingMode;		///< Mouse rotation state
	bool zoomactive;
	bool animation;
	int skiprotate;
	int nFramesToSkip;
	int drawMode;       ///< Stores the current draw mode
	bool drawModeActive;///< Flag, if the current draw mode is active => mouse down
	mreal l_x1, l_y1;   ///< start coordinates for line
	mreal start_x, start_y; ///< start coordinates for drawing
	mreal zoom_x0, zoom_x1, zoom_y0, zoom_y1;	///< Zoom in region
	bool showMessage;	///< Flag for showing messages (enabled by each execute())
	wxMenu *popup;		///< Pointer to pop-up menu
	wxTimer *timer;		///< Timer for animation
	int nFrameCounter;
	std::vector<wxImage> vAnimationBuffer;

	DECLARE_EVENT_TABLE()

private:
    void UpdateTools();

	int mouse_x0, mouse_y0, mouse_x1, mouse_y1;		///< Temporary variables for mouse
	wxFrame* m_parentFrame;
	wxToolBar* toptoolbar;
	wxStatusBar* statusbar;


	wxTextCtrl* styling;
};
//-----------------------------------------------------------------------------
#endif
