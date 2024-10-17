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

        virtual void DrawBackground(wxDC& dc, wxWindow* WXUNUSED(wnd), const wxRect& rect) override;
        virtual void DrawTab(wxDC& dc, wxWindow* wnd, const wxAuiNotebookPage& page, const wxRect& in_rect,
                             int close_button_state, wxRect* out_tab_rect, wxRect* out_button_rect, int* x_extent) override;

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


