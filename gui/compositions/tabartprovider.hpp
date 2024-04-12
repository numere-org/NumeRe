/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2024  Erik Haenel et al.

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


#ifndef TABARTPROVIDER_HPP
#define TABARTPROVIDER_HPP

#include <wx/aui/auibook.h>
#include <wx/renderer.h>

wxString wxAuiChopText(wxDC& dc, const wxString& text, int max_size);
static void IndentPressedBitmap(wxRect* rect, int button_state)
{
    if (button_state == wxAUI_BUTTON_STATE_PRESSED)
    {
        rect->x++;
        rect->y++;
    }
}

/////////////////////////////////////////////////
/// \brief Class for providing the colour and
/// effects to the tabs of the editor and the
/// other UI elements.
/////////////////////////////////////////////////
class TabArtProvider : public wxAuiDefaultTabArt
{
    private:
        bool m_threeState;

    public:
        /////////////////////////////////////////////////
        /// \brief TabArtProvider constructor. Sets the
        /// standard colours.
        ///
        /// \param activeCol const wxColour&
        /// \param threeState bool
        ///
        /////////////////////////////////////////////////
        TabArtProvider(const wxColour& activeCol, bool threeState = false) : wxAuiDefaultTabArt(), m_threeState(threeState)
        {
            TabArtProvider::SetActiveColour(activeCol); // 204 232 255 // 153 209 255
        }

        /////////////////////////////////////////////////
        /// \brief Draw the background of the tabs and
        /// the separating base line.
        ///
        /// \param dc wxDC&
        /// \param wnd wxWindow*
        /// \param rect const wxRect&
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void DrawBackground(wxDC& dc, wxWindow* WXUNUSED(wnd), const wxRect& rect) override
        {
            // draw background
            wxColor top_color = m_baseColour.ChangeLightness(90);
            wxColor bottom_color = m_activeColour.ChangeLightness(150);
            wxRect r;

            if (m_flags & wxAUI_NB_BOTTOM)
                r = wxRect(rect.x, rect.y, rect.width + 2, rect.height);
            // TODO: else if (m_flags &wxAUI_NB_LEFT) {}
            // TODO: else if (m_flags &wxAUI_NB_RIGHT) {}
            else //for wxAUI_NB_TOP
                r = wxRect(rect.x, rect.y, rect.width + 2, rect.height - 3);

            dc.GradientFillLinear(r, top_color, bottom_color, wxSOUTH);

            // draw base lines
            dc.SetPen(m_borderPen);
            int y = rect.GetHeight();
            int w = rect.GetWidth();

            if (m_flags & wxAUI_NB_BOTTOM)
            {
                dc.SetBrush(wxBrush(bottom_color));
                dc.DrawRectangle(-1, 0, w + 2, 4);
            }
            // TODO: else if (m_flags &wxAUI_NB_LEFT) {}
            // TODO: else if (m_flags &wxAUI_NB_RIGHT) {}
            else //for wxAUI_NB_TOP
            {
                dc.SetBrush(m_baseColourBrush);
                dc.DrawRectangle(-1, y - 4, w + 2, 4);
            }
        }

        /////////////////////////////////////////////////
        /// \brief Draws an individual tab.
        ///
        /// \param dc wxDC& output dc
        /// \param wnd wxWindow*
        /// \param page const wxAuiNotebookPage&
        /// \param in_rect const wxRect& rectangle the tab should be confined to
        /// \param close_button_state int
        /// \param out_tab_rect wxRect* actual output rectangle
        /// \param out_button_rect wxRect*
        /// \param x_extent int* the advance x; where the next tab should start
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void DrawTab(wxDC& dc, wxWindow* wnd, const wxAuiNotebookPage& page, const wxRect& in_rect,
                             int close_button_state, wxRect* out_tab_rect, wxRect* out_button_rect, int* x_extent) override
        {
            wxCoord normal_textx, normal_texty;
            wxCoord selected_textx, selected_texty;
            wxCoord texty;

            wxWindow* focused = wnd->FindFocus();
            bool hasFocus = !m_threeState || (focused && (focused == page.window || focused == wnd || page.window->IsDescendant(focused)));

            // if the caption is empty, measure some temporary text
            wxString caption = page.caption;
            if (caption.empty())
                caption = wxT("Xj");

            dc.SetFont(m_selectedFont);
            dc.GetTextExtent(caption, &selected_textx, &selected_texty);

            dc.SetFont(m_normalFont);
            dc.GetTextExtent(caption, &normal_textx, &normal_texty);

            // figure out the size of the tab
            wxSize tab_size = GetTabSize(dc,
                                         wnd,
                                         page.caption,
                                         page.bitmap,
                                         page.active,
                                         close_button_state,
                                         x_extent);

            wxCoord tab_height = m_tabCtrlHeight - 3;
            wxCoord tab_width = tab_size.x;
            wxCoord tab_x = in_rect.x;
            wxCoord tab_y = in_rect.y + in_rect.height - tab_height;

            caption = page.caption;

            // select pen, brush and font for the tab to be drawn
            if (page.active)
            {
                dc.SetFont(m_selectedFont);
                texty = selected_texty;
            }
            else
            {
                dc.SetFont(m_normalFont);
                texty = normal_texty;
            }

            // create points that will make the tab outline
            int clip_width = tab_width;
            if (tab_x + clip_width > in_rect.x + in_rect.width)
                clip_width = (in_rect.x + in_rect.width) - tab_x;

            // since the above code above doesn't play well with WXDFB or WXCOCOA,
            // we'll just use a rectangle for the clipping region for now --
            dc.SetClippingRegion(tab_x, tab_y, clip_width+1, tab_height-3);

            wxPoint border_points[6];

            if (m_flags &wxAUI_NB_BOTTOM)
            {
                border_points[0] = wxPoint(tab_x,             tab_y);
                border_points[1] = wxPoint(tab_x,             tab_y+tab_height-6);
                border_points[2] = wxPoint(tab_x+2,           tab_y+tab_height-4);
                border_points[3] = wxPoint(tab_x+tab_width-2, tab_y+tab_height-4);
                border_points[4] = wxPoint(tab_x+tab_width,   tab_y+tab_height-6);
                border_points[5] = wxPoint(tab_x+tab_width,   tab_y);
            }
            else //if (m_flags & wxAUI_NB_TOP) {}
            {
                border_points[0] = wxPoint(tab_x,             tab_y+tab_height-4);
                border_points[1] = wxPoint(tab_x,             tab_y+2);
                border_points[2] = wxPoint(tab_x+2,           tab_y);
                border_points[3] = wxPoint(tab_x+tab_width-2, tab_y);
                border_points[4] = wxPoint(tab_x+tab_width,   tab_y+2);
                border_points[5] = wxPoint(tab_x+tab_width,   tab_y+tab_height-4);
            }
            // TODO: else if (m_flags &wxAUI_NB_LEFT) {}
            // TODO: else if (m_flags &wxAUI_NB_RIGHT) {}

            int drawn_tab_yoff = border_points[1].y;
            int drawn_tab_height = border_points[0].y - border_points[1].y;

            if (page.active && hasFocus)
            {
                // draw active tab
                // draw base background color
                wxRect r(tab_x, tab_y, tab_width, tab_height);
                dc.SetPen(wxPen(m_activeColour));
                dc.SetBrush(wxBrush(m_activeColour));
                dc.DrawRectangle(r.x+1, r.y+1, r.width-1, r.height-4);

                // this white helps fill out the gradient at the top of the tab
                dc.SetPen(*wxWHITE_PEN);
                dc.SetBrush(*wxWHITE_BRUSH);
                dc.DrawRectangle(r.x+2, r.y+1, r.width-3, r.height-4);

                // these two points help the rounded corners appear more antialiased
                dc.SetPen(wxPen(m_activeColour));
                dc.DrawPoint(r.x+2, r.y+1);
                dc.DrawPoint(r.x+r.width-2, r.y+1);

                // set rectangle down a bit for gradient drawing
                int offset = r.GetHeight()/6;
                r.height -= offset;
                r.x += 2;
                r.width -= 3;
                r.y += offset;
                r.y -= 2;

                // draw gradient background
                wxColor top_color = *wxWHITE;
                wxColor bottom_color = m_activeColour;
                dc.GradientFillLinear(r, bottom_color, top_color, wxNORTH);
            }
            else if (page.active && !hasFocus)
            {
                wxColour topColour = m_baseColour.ChangeLightness(120);
                // draw semi-active tab
                // draw base background color
                wxRect r(tab_x, tab_y, tab_width, tab_height);
                dc.SetPen(wxPen(m_activeColour));
                dc.SetBrush(wxBrush(m_activeColour));
                dc.DrawRectangle(r.x+1, r.y+1, r.width-1, r.height-4);

                // this white helps fill out the gradient at the top of the tab
                dc.SetPen(wxPen(topColour));
                dc.SetBrush(wxBrush(topColour));
                dc.DrawRectangle(r.x+2, r.y+1, r.width-3, r.height-4);

                // these two points help the rounded corners appear more antialiased
                dc.SetPen(wxPen(m_activeColour));
                dc.DrawPoint(r.x+2, r.y+1);
                dc.DrawPoint(r.x+r.width-2, r.y+1);

                // set rectangle down a bit for gradient drawing
                r.SetHeight(r.GetHeight()/3);
                r.x += 2;
                r.width -= 3;
                r.y += 2*r.height;
                r.y -= 2;

                // draw gradient background
                wxColor top_color = topColour;
                wxColor bottom_color = m_activeColour;
                dc.GradientFillLinear(r, bottom_color, top_color, wxNORTH);
            }
            else
            {
                // draw inactive tab
                wxRect r(tab_x, tab_y+1, tab_width, tab_height-3);

                // start the gradent up a bit and leave the inside border inset
                // by a pixel for a 3D look.  Only the top half of the inactive
                // tab will have a slight gradient
                r.x += 3;
                r.y++;
                r.width -= 4;
                r.height /= 2;
                r.height--;

                // -- draw top gradient fill for glossy look
                wxColor top_color = m_baseColour;
                wxColor bottom_color = top_color.ChangeLightness(160);
                dc.GradientFillLinear(r, bottom_color, top_color, wxNORTH);

                r.y += r.height;
                r.y--;

                // -- draw bottom fill for glossy look
                top_color = m_baseColour;
                bottom_color = m_baseColour;
                dc.GradientFillLinear(r, top_color, bottom_color, wxSOUTH);
            }

            // draw tab outline
            dc.SetPen(m_borderPen);
            dc.SetBrush(*wxTRANSPARENT_BRUSH);
            dc.DrawPolygon(WXSIZEOF(border_points), border_points);

            // there are two horizontal grey lines at the bottom of the tab control,
            // this gets rid of the top one of those lines in the tab control
            if (page.active)
            {
                if (m_flags &wxAUI_NB_BOTTOM)
                    dc.SetPen(wxPen(m_baseColour.ChangeLightness(170)));
                // TODO: else if (m_flags &wxAUI_NB_LEFT) {}
                // TODO: else if (m_flags &wxAUI_NB_RIGHT) {}
                else //for wxAUI_NB_TOP
                    dc.SetPen(m_baseColourPen);

                dc.DrawLine(border_points[0].x+1,
                            border_points[0].y,
                            border_points[5].x,
                            border_points[5].y);
            }

            int text_offset = tab_x + 8;
            int close_button_width = 0;

            if (close_button_state != wxAUI_BUTTON_STATE_HIDDEN)
                close_button_width = m_activeCloseBmp.GetWidth();

            int bitmap_offset = 0;

            if (page.bitmap.IsOk())
            {
                bitmap_offset = tab_x + 8;

                // draw bitmap
                dc.DrawBitmap(page.bitmap,
                              bitmap_offset,
                              drawn_tab_yoff + (drawn_tab_height/2) - (page.bitmap.GetHeight()/2),
                              true);

                text_offset = bitmap_offset + page.bitmap.GetWidth();
                text_offset += 3; // bitmap padding
            }
            else
                text_offset = tab_x + 8;

            wxString draw_text = wxAuiChopText(dc, caption, tab_width - (text_offset-tab_x) - close_button_width);

            // draw tab text
            dc.DrawText(draw_text,
                        text_offset,
                        drawn_tab_yoff + (drawn_tab_height)/2 - (texty/2) - 1);

            // draw focus rectangle
            if (page.active && (wnd->FindFocus() == wnd))
            {
                wxRect focusRectText(text_offset, (drawn_tab_yoff + (drawn_tab_height)/2 - (texty/2) - 1),
                                     selected_textx, selected_texty);

                wxRect focusRect;
                wxRect focusRectBitmap;

                if (page.bitmap.IsOk())
                    focusRectBitmap = wxRect(bitmap_offset, drawn_tab_yoff + (drawn_tab_height/2) - (page.bitmap.GetHeight()/2),
                                             page.bitmap.GetWidth(), page.bitmap.GetHeight());

                if (page.bitmap.IsOk() && draw_text.IsEmpty())
                    focusRect = focusRectBitmap;
                else if (!page.bitmap.IsOk() && !draw_text.IsEmpty())
                    focusRect = focusRectText;
                else if (page.bitmap.IsOk() && !draw_text.IsEmpty())
                    focusRect = focusRectText.Union(focusRectBitmap);

                focusRect.Inflate(2, 2);

                wxRendererNative::Get().DrawFocusRect(wnd, dc, focusRect, 0);
            }

            // draw close button if necessary
            if (close_button_state != wxAUI_BUTTON_STATE_HIDDEN)
            {
                wxBitmap bmp = m_disabledCloseBmp;

                if (close_button_state == wxAUI_BUTTON_STATE_HOVER ||
                    close_button_state == wxAUI_BUTTON_STATE_PRESSED)
                {
                    bmp = m_activeCloseBmp;
                }

                int offsetY = tab_y-1;
                if (m_flags & wxAUI_NB_BOTTOM)
                    offsetY = 1;

                wxRect rect(tab_x + tab_width - close_button_width - 1,
                            offsetY + (tab_height/2) - (bmp.GetHeight()/2),
                            close_button_width,
                            tab_height);

                IndentPressedBitmap(&rect, close_button_state);
                dc.DrawBitmap(bmp, rect.x, rect.y, true);

                *out_button_rect = rect;
            }

            *out_tab_rect = wxRect(tab_x, tab_y, tab_width, tab_height);

            dc.DestroyClippingRegion();
        }

        /////////////////////////////////////////////////
        /// \brief Set the default colour of the tabs.
        ///
        /// \param colour const wxColour&
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void SetColour(const wxColour& colour) override
        {
            m_baseColour = colour;
        }

        /////////////////////////////////////////////////
        /// \brief Set the active colour of the tabs.
        ///
        /// \param colour const wxColour&
        /// \return void
        ///
        /////////////////////////////////////////////////
        virtual void SetActiveColour(const wxColour& colour) override
        {
            m_activeColour = colour;
            m_borderPen = wxPen(m_activeColour.ChangeLightness(75));
            m_baseColourPen = wxPen(m_activeColour);
            m_baseColourBrush = wxBrush(m_activeColour);
        }

        virtual wxAuiTabArt* Clone() override
        {
            return new TabArtProvider(*this);
        }
};

#endif // TABARTPROVIDER_HPP


