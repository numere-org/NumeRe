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



#ifndef PLOTINFO_HPP
#define PLOTINFO_HPP

#include <mgl2/mgl.h>
#include <string>
#include <vector>
#include "../interval.hpp"

/////////////////////////////////////////////////
/// \brief This structure governs the needed
/// parameters for plotting.
/////////////////////////////////////////////////
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
    std::string sCommand;
    std::string sPlotParams;
    int nSamples;
    int nStyle;
    int nStyleMax;
    unsigned int nMaxPlotDim;
    int nFunctions;
    std::vector<std::string> sLineStyles;
    std::vector<std::string> sContStyles;
    std::vector<std::string> sPointStyles;
    std::vector<std::string> sConPointStyles;

    /////////////////////////////////////////////////
    /// \brief Simple constructor to initialize the
    /// structure into a valid state.
    /////////////////////////////////////////////////
    PlotInfo() : nStyle(0), nFunctions(0) {}

    /////////////////////////////////////////////////
    /// \brief Returns the ID of the next style.
    ///
    /// \return int
    ///
    /////////////////////////////////////////////////
    int nextStyle() const
    {
        if (nStyle == nStyleMax - 1)
            return 0;

        return nStyle+1;
    }
};


#endif
