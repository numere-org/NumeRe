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
#include <utility>

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
    std::vector<std::pair<mglData, mglData>> data;
    std::vector<mglData> axes; // Might be reasonable to use an IntervalSet here to avoid blocking too much memory
    PlotType type;
    std::string legend;
    std::string boundAxes;

    /////////////////////////////////////////////////
    /// \brief PlotAsset constructor. Creates an
    /// empty and invalid asset.
    /////////////////////////////////////////////////
    PlotAsset() : type(PT_NONE)
    {
    }

    /////////////////////////////////////////////////
    /// \brief Prepares the internal memory to fit
    /// the desired elements.
    ///
    /// \param _t PlotType
    /// \param nDim size_t
    /// \param nAxes size_t
    /// \param samples const std::vector<size_t>&
    /// \param nLayers size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void create(PlotType _t, size_t nDim, size_t nAxes, const std::vector<size_t>& samples, size_t nLayers = 1)
    {
        if (_t == PT_NONE || !nDim || samples.size() < nDim || samples.size() < nAxes || !nAxes || !nLayers)
        {
            type = PT_NONE;
            return;
        }

        for (size_t s = 0; s < samples.size(); s++)
        {
            if (!samples[s])
            {
                type = PT_NONE;
                return;
            }
        }

        type = _t;
        data.resize(nLayers);

        for (size_t l = 0; l < nLayers; l++)
        {
            data[l].first.Create(samples[XCOORD],
                                 nDim >= 2 ? samples[YCOORD] : 1,
                                 nDim == 3 ? samples[ZCOORD] : 1);
            data[l].second.Create(samples[XCOORD],
                                  nDim >= 2 ? samples[YCOORD] : 1,
                                  nDim == 3 ? samples[ZCOORD] : 1);
        }

        axes.resize(nAxes);

        for (size_t a = 0; a < nAxes; a++)
        {
            axes[a].Create(samples[a]);
        }
    }

    /////////////////////////////////////////////////
    /// \brief Convenience member function for 1D
    /// plots.
    ///
    /// \param _t PlotType
    /// \param nSamples size_t
    /// \param datarows size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void create1DPlot(PlotType _t, size_t nSamples, size_t datarows = 1)
    {
        create(_t, 1, 1, {nSamples}, datarows);
    }

    /////////////////////////////////////////////////
    /// \brief Convenience member function for 3D
    /// plots.
    ///
    /// \param _t PlotType
    /// \param nSamples size_t
    /// \param datarows size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void create3DPlot(PlotType _t, size_t nSamples, size_t datarows = 1)
    {
        create(_t, 1, 0, {nSamples}, 3*datarows);
    }

    /////////////////////////////////////////////////
    /// \brief Convenience member function for 2D
    /// meshgrid-like plots.
    ///
    /// \param _t PlotType
    /// \param samples const std::vector<size_t>&
    /// \param nLayers size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void create2DMesh(PlotType _t, const std::vector<size_t>& samples, size_t nLayers = 1)
    {
        create(_t, 2, 2, samples, nLayers);
    }

    /////////////////////////////////////////////////
    /// \brief Convenience member function for 3D
    /// meshgrid-like plots.
    ///
    /// \param _t PlotType
    /// \param samples const std::vector<size_t>&
    /// \param nLayers size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void create3DMesh(PlotType _t, const std::vector<size_t>& samples, size_t nLayers = 1)
    {
        create(_t, 3, 3, samples, nLayers);
    }

    /////////////////////////////////////////////////
    /// \brief Convenience member function for 2D
    /// vectorfield plots.
    ///
    /// \param _t PlotType
    /// \param samples const std::vector<size_t>&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void create2DVect(PlotType _t, const std::vector<size_t>& samples)
    {
        create(_t, 2, 2, samples, 2);
    }

    /////////////////////////////////////////////////
    /// \brief Convenience member function for 3D
    /// vectorfield plots.
    ///
    /// \param _t PlotType
    /// \param samples const std::vector<size_t>&
    /// \return void
    ///
    /////////////////////////////////////////////////
    void create3DVect(PlotType _t, const std::vector<size_t>& samples)
    {
        create(_t, 2, 2, samples, 3);
    }

    /////////////////////////////////////////////////
    /// \brief Convenience function to write multi-
    /// dimensional data to the internal data object.
    ///
    /// \param val const mu::value_type&
    /// \param layer size_t
    /// \param x size_t
    /// \param y size_t
    /// \param z size_t
    /// \return void
    ///
    /////////////////////////////////////////////////
    void writeData(const mu::value_type& val, size_t layer, size_t x, size_t y = 0, size_t z = 0)
    {
        if (layer >= data.size()
            || x >= (size_t)data[layer].first.nx
            || y >= (size_t)data[layer].first.ny
            || z >= (size_t)data[layer].first.nz)
            throw SyntaxError(SyntaxError::PLOT_ERROR, "", "");

        data[layer].first.a[x + data[layer].first.nx*y + data[layer].first.ny*z] = mu::isinf(val) ? NAN : val.real();
        data[layer].second.a[x + data[layer].second.nx*y + data[layer].second.ny*z] = mu::isinf(val) ? NAN : val.imag();
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
        for (size_t l = 0; l < data.size(); l++)
        {
            data[l].first = ::duplicatePoints(data[l].first);
            data[l].second = ::duplicatePoints(data[l].second);
        }

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
        for (size_t l = 0; l < data.size(); l++)
        {
            for (int i = 0; i < data[l].first.GetNN(); i++)
            {
                data[l].first.a[i] = data[l].first.a[i] >= 0.0 ? data[l].first.a[i] : NAN;
                data[l].second.a[i] = data[l].second.a[i] >= 0.0 ? data[l].second.a[i] : NAN;
            }
        }
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
    /// \param layer size_t
    /// \return IntervalSet
    ///
    /////////////////////////////////////////////////
    IntervalSet getDataIntervals(size_t layer = 0) const
    {
        IntervalSet ivl;

        if (layer >= data.size())
            return ivl;

        ivl.intervals.push_back(Interval(data[layer].first.Minimal(), data[layer].first.Maximal()));
        ivl.intervals.push_back(Interval(data[layer].second.Minimal(), data[layer].second.Maximal()));
        ivl.setNames({"Re", "Im"});

        return ivl;
    }

    /////////////////////////////////////////////////
    /// \brief Return true, if the internal data is
    /// complex valued.
    ///
    /// \param layer size_t
    /// \return bool
    ///
    /////////////////////////////////////////////////
    bool isComplex(size_t layer = 0) const
    {
        if (layer >= data.size())
            return false;

        return data[layer].second.Minimal() != 0.0 || data[layer].second.Maximal() != 0.0;
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

    /////////////////////////////////////////////////
    /// \brief Return the internal data layers.
    ///
    /// \return size_t
    ///
    /////////////////////////////////////////////////
    size_t getLayers() const
    {
        return data.size();
    }

    /////////////////////////////////////////////////
    /// \brief Apply a modulus to a selected axis
    /// (e.g. for curvilinear coordinates).
    ///
    /// \param c PlotCoords
    /// \param mod double
    /// \return void
    ///
    /////////////////////////////////////////////////
    void applyModulus(PlotCoords c, double mod)
    {
        if (c < axes.size())
        {
            for (int n = 0; n < axes[c].nx; n++)
            {
                axes[c].a[n] = ::fmod(axes[c].a[n], mod);

                if (axes[c].a[n] < 0)
                    axes[c].a[n] += mod;
            }
        }
        else if (c < data.size() && !axes.size())
        {
            for (int n = 0; n < data[c].first.nx; n++)
            {
                data[c].first.a[n] = ::fmod(data[c].first.a[n], mod);
                data[c].second.a[n] = ::fmod(data[c].second.a[n], mod);

                if (data[c].first.a[n] < 0)
                    data[c].first.a[n] += mod;

                if (data[c].second.a[n] < 0)
                    data[c].second.a[n] += mod;
            }
        }
    }

    /////////////////////////////////////////////////
    /// \brief Converts the vectors in multiple
    /// layers into a single matrix.
    ///
    /// \return mglData
    ///
    /////////////////////////////////////////////////
    mglData vectorsToMatrix()
    {
        int maxdim = data[0].first.nx;

        // Find the largest vector
        for (size_t val = 1; val < data.size(); val++)
        {
            if (maxdim < data[val].first.nx)
                maxdim = data[val].first.nx;
        }

        // Create the target array and copy the data
        mglData _mData(maxdim, data.size());

        for (size_t val = 0; val < data.size(); val++)
        {
            for (int col = 0; col < data[val].first.nx; col++)
            {
                _mData.a[col + val * maxdim] = data[val].first.a[col];
            }
        }

        return _mData;
    }
};

#endif // PLOTASSET_HPP


