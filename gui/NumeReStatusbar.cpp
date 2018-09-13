/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2018  Erik Haenel et al.

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

#include "NumeReStatusbar.hpp"

// Constructor
NumeReStatusbar::NumeReStatusbar(wxWindow* parent, wxWindowID id, long style) : wxStatusBar(parent, id, style)
{
    // Get the average width of a character in the current font
    int charWidth = parent->GetCharWidth();
    int paddingPix = 3;

    // Define the sizes in chars or pixels
    int barSizePix = 120;
	int filetypeChars = 20;
	int readEditChars = 15;
	int lineColChars = 20;
	int debuggerchars = 25;

	// define status bar widths and styles
	const int widths[] = {barSizePix, -1, filetypeChars * charWidth, readEditChars * charWidth, lineColChars * charWidth, debuggerchars * charWidth};
    const int styles[] = {wxSB_SUNKEN, wxSB_RAISED, wxSB_RAISED, wxSB_RAISED, wxSB_RAISED, wxSB_RAISED};

    // Set the number of fields and their styles
	SetFieldsCount(WXSIZEOF(widths), widths);
	SetStatusStyles(WXSIZEOF(styles), styles);

	// Place the gauge as a busy indicator
    m_busyIndicator = new wxGauge(this, wxID_ANY, 100, wxPoint(paddingPix, paddingPix), wxSize(barSizePix + paddingPix, GetSize().GetHeight() - 2*paddingPix), wxGA_HORIZONTAL | wxGA_SMOOTH);

    // Set the indicator to zero
    m_busyIndicator->SetValue(0);
}

// Switch to ready
void NumeReStatusbar::Ready()
{
    if (m_busyIndicator)
    {
        // Define the range, switch the indicator from zero to 100
        m_busyIndicator->SetRange(100);
        m_busyIndicator->SetValue(0);
        m_busyIndicator->SetValue(100);
    }
}

// Switch to busy
void NumeReStatusbar::Busy()
{
    // Let the gauge pulse
    if (m_busyIndicator)
        m_busyIndicator->Pulse();
}

// Update a status text
void NumeReStatusbar::SetStatus(NumeReStatusbar::StatusField field, const wxString& sStatusText)
{
    if (sStatusText != GetStatusText((int)field))
        SetStatusText(sStatusText, (int)field);
}

// Get a status text
wxString NumeReStatusbar::GetStatus(NumeReStatusbar::StatusField field)
{
    return GetStatusText((int)field);
}

