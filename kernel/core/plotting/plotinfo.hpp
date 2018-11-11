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

using namespace std;


struct PlotInfo
{
    double dRanges[3][2];
    double dSecAxisRanges[2][2];
    double dColorRanges[2];
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
    /*inline PlotInfo(PlotInfo& __pinfo)
        {
            for (int i = 0; i < 3; i++)
            {
                dRanges[i][0] = __pinfo.dRanges[i][0];
                dRanges[i][1] = __pinfo.dRanges[i][1];
            }
            for (int i = 0; i < 2; i++)
            {
                dSecAxisRanges[i][0] = __pinfo.dSecAxisRanges[i][0];
                dSecAxisRanges[i][1] = __pinfo.dSecAxisRanges[i][1];
                dColorRanges[i] = __pinfo.dColorRanges[i];
            }
            b2D = __pinfo.b2D;
            b3D = __pinfo.b3D;
            b2DVect = __pinfo.b2DVect;
            b3DVect = __pinfo.b3DVect;
            bDraw = __pinfo.bDraw;
            bDraw3D = __pinfo.bDraw3D;
            sCommand = __pinfo.sCommand;
            sPlotParams = __pinfo.sPlotParams;
            nSamples = __pinfo.nSamples;
            nStyleMax = __pinfo.nStyleMax;
            nMaxPlotDim = __pinfo.nMaxPlotDim;
            // Pointer-Variablen
            nStyle = __pinfo.nStyle;
            nFunctions = __pinfo.nFunctions;
            sLineStyles = __pinfo.sLineStyles;
            sContStyles = __pinfo.sContStyles;
            sPointStyles = __pinfo.sPointStyles;
            sConPointStyles = __pinfo.sConPointStyles;

            // pointer take-over
            __pinfo.sLineStyles = nullptr;
            __pinfo.sContStyles = nullptr;
            __pinfo.sPointStyles = nullptr;
            __pinfo.sConPointStyles = nullptr;
        }*/

};


#endif
