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

#ifndef PLOTASSET_HPP
#define PLOTASSET_HPP

#include "../interval.hpp"
#include <mgl2/mgl.h>
#include <string>
#include <vector>

mglData duplicatePoints(const mglData& _mData);

enum PlotType
{
    PT_NONE,
    PT_FUNCTION,
    PT_DATA
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


/////////////////////////////////////////////////
/// \brief A single plot data asset to be
/// plotted. Contains the data, its axes and its
/// type.
/////////////////////////////////////////////////
struct PlotAsset
{
    mglData redata;
    mglData imdata;
    std::vector<mglData> axes; // Might be reasonable to use an IntervalSet here to avoid blocking too much memory
    PlotType type;
    std::string legend;
    std::string boundAxes;

    /////////////////////////////////////////////////
    /// \brief PlotAsset constructor. Prepares the
    /// internal memory to fit the desired elements.
    ///
    /// \param _t PlotType
    /// \param nDim size_t
    /// \param nSamples size_t
    ///
    /////////////////////////////////////////////////
    PlotAsset(PlotType _t, size_t nDim, size_t nSamples) : type(_t)
    {
        redata.Create(nSamples, nDim >= 2 ? nSamples : 1, nDim == 3 ? nSamples : 1);
        imdata.Create(nSamples, nDim >= 2 ? nSamples : 1, nDim == 3 ? nSamples : 1);
        axes.resize(nDim, mglData(nSamples));
    }

    /////////////////////////////////////////////////
    /// \brief Convenience function to write multi-
    /// dimensional data to the internal data object.
    ///
    /// \param val const mu::value_type&
    /// \param x size_t
    /// \param y size_t
    /// \param z size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void writeData(const mu::value_type& val, size_t x, size_t y = 0, size_t z = 0)
    {
        if (x >= (size_t)redata.nx
            || y >= (size_t)redata.ny
            || z >= (size_t)redata.nz)
            throw SyntaxError(SyntaxError::PLOT_ERROR, "", "");

        redata.a[x + redata.nx*y + redata.ny*z] = mu::isinf(val) ? NAN : val.real();
        imdata.a[x + imdata.nx*y + imdata.ny*z] = mu::isinf(val) ? NAN : val.imag();
    }

    /////////////////////////////////////////////////
    /// \brief Convenience function to write the axis
    /// values.
    ///
    /// \param val double
    /// \param pos size_t
    /// \param c PlotCoords c
    /// \return void
    ///
    /////////////////////////////////////////////////
    void writeAxis(double val, size_t pos, PlotCoords c = XCOORD)
    {
        if (c >= axes.size())
            return;

        if (pos >= (size_t)axes[c].nx)
            throw SyntaxError(SyntaxError::PLOT_ERROR, "", "");

        axes[c].a[pos] = isinf(val) ? NAN : val;
    }

    /////////////////////////////////////////////////
    /// \brief This function is a fix for the
    /// MathGL bug, which connects points outside of
    /// the data range.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void duplicatePoints()
    {
        redata = ::duplicatePoints(redata);
        imdata = ::duplicatePoints(imdata);

        for (size_t n = 0; n < axes.size(); n++)
        {
            axes[n] = ::duplicatePoints(axes[n]);
        }
    }

    /////////////////////////////////////////////////
    /// \brief This function is a fix for the
    /// MathGL bug to connect points, which are out
    /// of data range. This fix is used in curved
    /// coordinates case, where the calculated
    /// coordinate is r or rho.
    ///
    /// \return void
    ///
    /////////////////////////////////////////////////
    void removeNegativeValues()
    {
        for (int i = 0; i < redata.GetNN(); i++)
            redata.a[i] = redata.a[i] >= 0.0 ? redata.a[i] : NAN;

        for (int i = 0; i < imdata.GetNN(); i++)
            imdata.a[i] = imdata.a[i] >= 0.0 ? imdata.a[i] : NAN;
    }

    /////////////////////////////////////////////////
    /// \brief Return the interval, which is governed
    /// by the corresponding axis.
    ///
    /// \param c PlotCoords
    /// \return Interval
    ///
    /////////////////////////////////////////////////
    Interval getAxisInterval(PlotCoords c = XCOORD) const
    {
        if (c >= axes.size())
            return Interval();

        return Interval(axes[c].Minimal(), axes[c].Maximal());
    }

    /////////////////////////////////////////////////
    /// \brief Return the internal data arrays for
    /// real and imaginary values.
    ///
    /// \return IntervalSet
    ///
    /////////////////////////////////////////////////
    IntervalSet getDataIntervals() const
    {
        IntervalSet ivl;
        ivl.intervals.push_back(Interval(redata.Minimal(), redata.Maximal()));
        ivl.intervals.push_back(Interval(imdata.Minimal(), imdata.Maximal()));
        ivl.setNames({"Re", "Im"});

        return ivl;
    }

    /////////////////////////////////////////////////
    /// \brief Return true, if the internal data is
    /// complex valued.
    ///
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool isComplex() const
    {
        return imdata.Minimal() != 0.0 || imdata.Maximal() != 0.0;
    }

    /////////////////////////////////////////////////
    /// \brief Return the internal data dimensions.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t getDim() const
    {
        return axes.size();
    }
};

#endif // PLOTASSET_HPP


