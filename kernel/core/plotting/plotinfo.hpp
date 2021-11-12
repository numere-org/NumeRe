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

#include <mgl2/mgl.h>
#include <string>


#ifndef PLOTINFO_HPP
#define PLOTINFO_HPP

#include "../interval.hpp"

using namespace std;


struct PlotInfo
{
    IntervalSet ranges;
    IntervalSet secranges;
    bool b2D;
    bool b3D;
    bool b2DVect;
    bool b3DVect;
    bool bDraw;
    bool bDraw3D;
    string sCommand;
    string sPlotParams;
    int nSamples;
    int nStyleMax;
    unsigned int nMaxPlotDim;
    // Pointer-Variablen
    int* nStyle;
    int* nFunctions;
    string* sLineStyles;
    string* sContStyles;
    string* sPointStyles;
    string* sConPointStyles;

    inline PlotInfo() : nStyle(nullptr), nFunctions(nullptr), sLineStyles(nullptr), sContStyles(nullptr), sPointStyles(nullptr), sConPointStyles(nullptr) {}

    inline ~PlotInfo()
        {
            nStyle = 0;
            if (sLineStyles)
                delete[] sLineStyles;
            if (sContStyles)
                delete[] sContStyles;
            if (sPointStyles)
                delete[] sPointStyles;
            if (sConPointStyles)
                delete[] sConPointStyles;
            nFunctions = nullptr;
            sLineStyles = nullptr;
            sContStyles = nullptr;
            sPointStyles = nullptr;
            sConPointStyles = nullptr;
        }
};


#endif
