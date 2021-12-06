/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2021  Erik Haenel et al.

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

#ifndef PLOTDEF_HPP
#define PLOTDEF_HPP


enum PlotType
{
    PT_NONE,
    PT_FUNCTION,
    PT_DATA
};


enum ComplexType
{
    CPLX_NONE,
    CPLX_REIM,
    CPLX_PLANE
};


enum PlotCoords
{
    XCOORD = 0,
    YCOORD = 1,
    ZCOORD = 2,
    TCOORD = 3
};


enum PlotRanges
{
    XRANGE = XCOORD,
    YRANGE = YCOORD,
    ZRANGE = ZCOORD,
    CRANGE = 3,
    TRANGE = 4
};


enum CoordinateSystem
{
    CARTESIAN = 0,
    POLAR_PZ = 10,
    POLAR_RP,
    POLAR_RZ,
    SPHERICAL_PT = 100,
    SPHERICAL_RP,
    SPHERICAL_RT
};


#endif // PLOTDEF_HPP

