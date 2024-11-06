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

#include "cellvalueshader.hpp"


BEGIN_EVENT_TABLE(CellValueShaderDialog, wxDialog)
    EVT_BUTTON(-1, CellValueShaderDialog::OnButtonClick)
    EVT_CHECKBOX(-1, CellValueShaderDialog::OnCheckBox)
END_EVENT_TABLE()


wxColour CellValueShaderDialog::LEVELCOLOUR  = wxColour(128,128,255);
wxColour CellValueShaderDialog::LOWERCOLOUR  = wxColour(64,64,64);
wxColour CellValueShaderDialog::MINCOLOUR    = wxColour(128,0,0);
wxColour CellValueShaderDialog::MEDCOLOUR    = wxColour(255,128,0);
wxColour CellValueShaderDialog::MAXCOLOUR    = wxColour(255,255,128);
wxColour CellValueShaderDialog::HIGHERCOLOUR = wxColour(255,255,255);
bool CellValueShaderDialog::USEALLCOLOURS = false;

static wxColour fromHSL(unsigned hue, double saturation, double lightness)
{
    double C = (1.0 - std::abs(2*lightness-1)) * saturation;
    double X = C * (1.0 - std::abs(std::fmod(hue / 60.0, 2.0) - 1.0));
    double m = lightness - C/2.0;

    if (hue < 60)
        return wxColour((C+m)*255, (X+m)*255, m*255);

    if (hue < 120)
        return wxColour((X+m)*255, (C+m)*255, m*255);

    if (hue < 180)
        return wxColour(m*255, (C+m)*255, (X+m)*255);

    if (hue < 240)
        return wxColour(m*255, (X+m)*255, (C+m)*255);

    if (hue < 300)
        return wxColour((X+m)*255, m*255, (C+m)*255);

    return wxColour((C+m)*255, m*255, (X+m)*255);
}

wxColour CellValueShaderDialog::CATEGORYCOLOUR[16] = {
    fromHSL(132, 0.5625, 0.8583), fromHSL(48, 1, 0.8042), fromHSL(352, 1, 0.8917), fromHSL(196, 0.725, 0.8583),
    fromHSL(132, 0.5625, 0.8583*0.8), fromHSL(48, 1, 0.8042*0.8), fromHSL(352, 1, 0.8917*0.8), fromHSL(196, 0.725, 0.8583*0.8),
    fromHSL(132, 0.5625, 0.8583*0.6), fromHSL(48, 1, 0.8042*0.6), fromHSL(352, 1, 0.8917*0.6), fromHSL(196, 0.725, 0.8583*0.6),
    fromHSL(132, 0.5625, 0.8583*0.4), fromHSL(48, 1, 0.8042*0.4), fromHSL(352, 1, 0.8917*0.4), fromHSL(196, 0.725, 0.8583*0.4)};


