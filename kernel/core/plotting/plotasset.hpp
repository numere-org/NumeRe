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

#include "plotdef.hpp"
#include "../interval.hpp"
#include "../utils/tools.hpp"
#include <mgl2/mgl.h>
#include <string>
#include <vector>
#include <utility>


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

    void create(PlotType _t, size_t nDim, size_t nAxes, const std::vector<size_t>& samples, size_t nLayers = 1);
    void writeData(const mu::value_type& val, size_t layer, size_t x, size_t y = 0, size_t z = 0);
    void writeAxis(double val, size_t pos, PlotCoords c = XCOORD);
    void duplicatePoints();
    void removeNegativeValues(PlotCoords c);
    Interval getAxisInterval(PlotCoords c = XCOORD) const;
    IntervalSet getDataIntervals(size_t layer = 0) const;
    mglData norm(size_t layer) const;
    mglData arg(size_t layer) const;
    bool isComplex(size_t layer = 0) const;
    void applyModulus(PlotCoords c, double mod);
    mglData vectorsToMatrix() const;
    IntervalSet getWeightedRanges(size_t layer = 0, double dLowerPercentage = 1.0, double dUpperPercentage = 1.0) const;

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
        create(_t, 3, 3, samples, 3);
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
};


enum FunctionIntervalType
{
    ALLRANGES = -1,
    ONLYLEFT = -2,
    ONLYRIGHT = -3
};


/////////////////////////////////////////////////
/// \brief This class combines the plotassets
/// into a single structure.
/////////////////////////////////////////////////
class PlotAssetManager
{
    public:
        std::vector<PlotAsset> assets;

        enum DataIntervalNames
        {
            REAL,
            IMAG,
            REIM,
            ABSREIM,
            DATIVLCOUNT
        };

    private:
        std::pair<double,double> m_maxnorm;
        IntervalSet getIntervalsOfType(PlotType t, int coord) const;
        IntervalSet getAxisIntervalsOfType(PlotType t) const;

    public:
        void normalize(int t_animate);
        IntervalSet getDataIntervals(int coord = ALLRANGES) const;
        IntervalSet getFunctionIntervals(int coord = ALLRANGES) const;
        IntervalSet getWeightedFunctionIntervals(int coord = ALLRANGES, double dLowerPercentage = 1.0, double dUpperPercentage = 1.0) const;
        IntervalSet getFunctionAxes() const;
        IntervalSet getDataAxes() const;
        void weightedRange(int coord, Interval& ivl) const;
        bool hasDataPlots() const;
        void applyCoordSys(CoordinateSystem coords, size_t every = 1);
};

#endif // PLOTASSET_HPP


