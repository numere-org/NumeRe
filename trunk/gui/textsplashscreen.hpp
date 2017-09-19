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

#ifndef TEXTSPLASHSCREEN_HPP
#define TEXTSPLASHSCREEN_HPP
#include <wx/wx.h>
#include <wx/splash.h>

class TextSplashScreen : public wxSplashScreen
{
    public:
        TextSplashScreen(const wxBitmap& bitmap, long splashStyle, int milliseconds, wxWindow* parent)
            : wxSplashScreen (bitmap,  splashStyle, milliseconds, parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxNO_BORDER)
        {
            wxASSERT(bitmap.IsOk());

            m_noTextBitmap = bitmap;
        }

        void SetText(const wxString& text)
        {
            wxBitmap bitmap(m_noTextBitmap);

            if (!text.empty()) {
                wxMemoryDC memDC;

                memDC.SelectObject(bitmap);

                memDC.SetBackgroundMode(wxTRANSPARENT);
                memDC.SetTextForeground(wxColor(0,0,0));
                memDC.DrawText(text, 50,50);//450, 150);

                memDC.SelectObject(wxNullBitmap);
            }
            m_window->SetBitmap(bitmap);
            m_window->Refresh();
            m_window->Update();
        }
    private:
        wxBitmap m_noTextBitmap;
};



#endif // TEXTSPLASHSCREEN_HPP

