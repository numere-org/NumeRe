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

#include "plotasset.hpp"

mglData duplicatePoints(const mglData& _mData);


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
void PlotAsset::create(PlotType _t, size_t nDim, size_t nAxes, const std::vector<size_t>& samples, size_t nLayers)
{
    if (_t == PT_NONE || !nDim || samples.size() < nDim || samples.size() < nAxes || !nLayers)
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

    if (nAxes)
    {
        axes.resize(nAxes);

        for (size_t a = 0; a < nAxes; a++)
        {
            axes[a].Create(samples[a]);
        }
    }
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
void PlotAsset::writeData(const mu::value_type& val, size_t layer, size_t x, size_t y, size_t z)
{
    if (layer >= data.size()
        || x >= (size_t)data[layer].first.nx
        || y >= (size_t)data[layer].first.ny
        || z >= (size_t)data[layer].first.nz)
        throw SyntaxError(SyntaxError::PLOT_ERROR, "", "");

    data[layer].first.a[x + data[layer].first.nx*y + data[layer].first.nx*data[layer].first.ny*z] = mu::isinf(val) ? NAN : val.real();
    data[layer].second.a[x + data[layer].second.nx*y + data[layer].second.nx*data[layer].second.ny*z] = mu::isinf(val) ? NAN : val.imag();
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
void PlotAsset::writeAxis(double val, size_t pos, PlotCoords c)
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
void PlotAsset::duplicatePoints()
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
/// \param c PlotCoords
/// \return void
///
/////////////////////////////////////////////////
void PlotAsset::removeNegativeValues(PlotCoords c)
{
    if (c < axes.size())
    {
        for (int n = 0; n < axes[c].GetNN(); n++)
        {
            axes[c].a[n] = axes[c].a[n] >= 0.0 ? axes[c].a[n] : NAN;
        }
    }
    else if (c - axes.size() < data.size())
    {
        for (int i = 0; i < data[c-axes.size()].first.GetNN(); i++)
        {
            data[c-axes.size()].first.a[i] = data[c-axes.size()].first.a[i] >= 0.0
                                             ? data[c-axes.size()].first.a[i] : NAN;
            data[c-axes.size()].second.a[i] = data[c-axes.size()].second.a[i] >= 0.0
                                              ? data[c-axes.size()].second.a[i] : NAN;
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
Interval PlotAsset::getAxisInterval(PlotCoords c) const
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
IntervalSet PlotAsset::getDataIntervals(size_t layer) const
{
    IntervalSet ivl;

    if (layer >= data.size())
    {
        // Always return an "valid" intervalset
        ivl.intervals.resize(2);
        return ivl;
    }

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
bool PlotAsset::isComplex(size_t layer) const
{
    if (layer >= data.size())
        return false;

    return data[layer].second.Minimal() != 0.0 || data[layer].second.Maximal() != 0.0;
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
void PlotAsset::applyModulus(PlotCoords c, double mod)
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
    else if (c - axes.size() < data.size())
    {
        for (int n = 0; n < data[c-axes.size()].first.GetNN(); n++)
        {
            data[c-axes.size()].first.a[n] = ::fmod(data[c-axes.size()].first.a[n], mod);
            data[c-axes.size()].second.a[n] = ::fmod(data[c-axes.size()].second.a[n], mod);

            if (data[c-axes.size()].first.a[n] < 0)
                data[c-axes.size()].first.a[n] += mod;

            if (data[c-axes.size()].second.a[n] < 0)
                data[c-axes.size()].second.a[n] += mod;
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
mglData PlotAsset::vectorsToMatrix() const
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


/////////////////////////////////////////////////
/// \brief Get the centralized data quantiles.
///
/// \param layer size_t
/// \param dLowerPercentage double
/// \param dUpperPercentage double
/// \return IntervalSet
///
/////////////////////////////////////////////////
IntervalSet PlotAsset::getWeightedRanges(size_t layer, double dLowerPercentage, double dUpperPercentage) const
{
    IntervalSet ivl;
    ivl.intervals.resize(2);

    if (layer >= data.size())
        return ivl;

    mglData cp(data[layer].first);
    size_t nLength = qSortDouble(cp.a, cp.GetNN());
    size_t lowerpos = rint(fabs(1.0-dLowerPercentage)/2.0*(nLength));
    size_t upperpos = rint((dUpperPercentage/2.0+0.5)*(nLength));

    ivl[0].reset(cp.a[lowerpos], cp.a[upperpos]);

    cp = data[layer].second;
    nLength = qSortDouble(cp.a, cp.GetNN());
    lowerpos = rint(fabs(1.0-dLowerPercentage)/2.0*(nLength));
    upperpos = rint((dUpperPercentage/2.0+0.5)*(nLength));

    ivl[1].reset(cp.a[lowerpos], cp.a[upperpos]);

    return ivl;
}



/////////////////////////////////////////////////
// PLOTASSETMANAGER
/////////////////////////////////////////////////



/////////////////////////////////////////////////
/// \brief Returns the intervals fitting to all
/// selected data and type.
///
/// \param t PlotType
/// \param coord int
/// \return IntervalSet
///
/////////////////////////////////////////////////
IntervalSet PlotAssetManager::getIntervalsOfType(PlotType t, int coord) const
{
    IntervalSet ivl;
    ivl.intervals.resize(2);

    for (size_t i = 0; i < assets.size(); i++)
    {
        if (assets[i].type != t)
            continue;

        if (coord == ALLRANGES
            || (coord == ONLYLEFT && assets[i].boundAxes.find('l') != std::string::npos)
            || (coord == ONLYRIGHT && assets[i].boundAxes.find('r') != std::string::npos))
        {
            IntervalSet dataIntervals;

            for (size_t l = 0; l < assets[i].getLayers(); l++)
            {
                dataIntervals = assets[i].getDataIntervals(l);
                ivl[REAL] = ivl[REAL].combine(dataIntervals[REAL]);
                ivl[IMAG] = ivl[IMAG].combine(dataIntervals[IMAG]);
            }
        }
        else
        {
            if ((int)assets[i].getLayers() <= coord)
                continue;

            IntervalSet dataIntervals = assets[i].getDataIntervals(coord);
            ivl[REAL] = ivl[REAL].combine(dataIntervals[REAL]);
            ivl[IMAG] = ivl[IMAG].combine(dataIntervals[IMAG]);
        }
    }

    return ivl;
}


/////////////////////////////////////////////////
/// \brief Returns the axis intervals of the
/// selected type.
///
/// \param t PlotType
/// \return IntervalSet
///
/////////////////////////////////////////////////
IntervalSet PlotAssetManager::getAxisIntervalsOfType(PlotType t) const
{
    IntervalSet ivl;
    ivl.intervals.resize(3);

    for (size_t i = 0; i < assets.size(); i++)
    {
        if (assets[i].type != t)
            continue;

        for (size_t c = 0; c < min(3u, assets[i].getDim()); c++)
        {
            ivl[c] = ivl[c].combine(assets[i].getAxisInterval((PlotCoords)c));
        }
    }

    return ivl;
}


/////////////////////////////////////////////////
/// \brief Normalize the managed data to be below
/// 1 (or the starting amplitude in an animation).
///
/// \param t_animate int
/// \return void
///
/////////////////////////////////////////////////
void PlotAssetManager::normalize(int t_animate)
{
    std::pair<double, double> maxnorm(0, 0);

    for (const PlotAsset& ass : assets)
    {
        for (size_t layer = 0; layer < ass.getLayers(); layer++)
        {
            IntervalSet ivl = ass.getDataIntervals(layer);

            if (maxnorm.first < max(fabs(ivl[REAL].min()), fabs(ivl[REAL].max())))
                maxnorm.first = max(fabs(ivl[REAL].min()), fabs(ivl[REAL].max()));

            if (maxnorm.second < max(fabs(ivl[IMAG].min()), fabs(ivl[IMAG].max())))
                maxnorm.second = max(fabs(ivl[IMAG].min()), fabs(ivl[IMAG].max()));
        }
    }

    if (!t_animate)
        m_maxnorm = maxnorm;

    for (PlotAsset& ass : assets)
    {
        for (size_t layer = 0; layer < ass.getLayers(); layer++)
        {
            if (maxnorm.first)
                ass.data[layer].first /= maxnorm.first * maxnorm.first / m_maxnorm.first;

            if (maxnorm.second)
                ass.data[layer].second /= maxnorm.second * maxnorm.second / m_maxnorm.second;
        }
    }
}


/////////////////////////////////////////////////
/// \brief Returns the intervals fitting to all
/// selected data.
///
/// \param coord int
/// \return IntervalSet
///
/////////////////////////////////////////////////
IntervalSet PlotAssetManager::getDataIntervals(int coord) const
{
    return getIntervalsOfType(PT_DATA, coord);
}


/////////////////////////////////////////////////
/// \brief Returns the intervals fitting to all
/// selected data.
///
/// \param coord int
/// \return IntervalSet
///
/////////////////////////////////////////////////
IntervalSet PlotAssetManager::getFunctionIntervals(int coord) const
{
    return getIntervalsOfType(PT_FUNCTION, coord);
}


/////////////////////////////////////////////////
/// \brief Returns the central function quantiles.
///
/// \param coord int
/// \param dLowerPercentage double
/// \param dUpperPercentage double
/// \return IntervalSet
///
/////////////////////////////////////////////////
IntervalSet PlotAssetManager::getWeightedFunctionIntervals(int coord, double dLowerPercentage, double dUpperPercentage) const
{
    IntervalSet ivl;
    ivl.intervals.resize(2);

    for (size_t i = 0; i < assets.size(); i++)
    {
        if (assets[i].type != PT_FUNCTION)
            continue;

        if (coord == ALLRANGES
            || (coord == ONLYLEFT && assets[i].boundAxes.find('l') != std::string::npos)
            || (coord == ONLYRIGHT && assets[i].boundAxes.find('r') != std::string::npos))
        {
            IntervalSet dataIntervals;

            for (size_t l = 0; l < assets[i].getLayers(); l++)
            {
                dataIntervals = assets[i].getWeightedRanges(l, dLowerPercentage, dUpperPercentage);
                ivl[REAL] = ivl[REAL].combine(dataIntervals[REAL]);
                ivl[IMAG] = ivl[IMAG].combine(dataIntervals[IMAG]);
            }
        }
        else
        {
            if ((int)assets[i].getLayers() <= coord)
                continue;

            IntervalSet dataIntervals = assets[i].getWeightedRanges(coord, dLowerPercentage, dUpperPercentage);
            ivl[REAL] = ivl[REAL].combine(dataIntervals[REAL]);
            ivl[IMAG] = ivl[IMAG].combine(dataIntervals[IMAG]);
        }
    }

    return ivl;
}


/////////////////////////////////////////////////
/// \brief Returns the axis intervals of data
/// plots.
///
/// \return IntervalSet
///
/////////////////////////////////////////////////
IntervalSet PlotAssetManager::getDataAxes() const
{
    return getAxisIntervalsOfType(PT_DATA);
}


/////////////////////////////////////////////////
/// \brief Returns the axis intervals of function
/// plots.
///
/// \return IntervalSet
///
/////////////////////////////////////////////////
IntervalSet PlotAssetManager::getFunctionAxes() const
{
    return getAxisIntervalsOfType(PT_FUNCTION);
}


/////////////////////////////////////////////////
/// \brief This member function does the
/// "undefined data point" magic, where the range
/// of the plot is chosen so that "infinity"
/// values are ignored.
///
/// \param coord int
/// \param ivl Interval&
/// \return void
///
/////////////////////////////////////////////////
void PlotAssetManager::weightedRange(int coord, Interval& ivl) const
{
    if (log(ivl.range()) > 5)
    {
        const double dPercentage = 0.975;
        const double dSinglePercentageValue = 0.99;
        double dSinglePercentageUse;

        if (coord == ALLRANGES)
            dSinglePercentageUse = dSinglePercentageValue;
        else
            dSinglePercentageUse = dPercentage;

        if (log(fabs(ivl.min())) <= 1)
            ivl = getWeightedFunctionIntervals(coord, 1.0, dSinglePercentageUse)[0];
        else if (log(fabs(ivl.max())) <= 1)
            ivl = getWeightedFunctionIntervals(coord, dSinglePercentageUse, 1.0)[0];
        else
            ivl = getWeightedFunctionIntervals(coord, dPercentage, dPercentage)[0];
    }
}


/////////////////////////////////////////////////
/// \brief Returns true, if the manager contains
/// some data plots.
///
/// \return bool
///
/////////////////////////////////////////////////
bool PlotAssetManager::hasDataPlots() const
{
    for (const PlotAsset& ass : assets)
    {
        if (ass.type == PT_DATA)
            return true;
    }

    return false;
}


/////////////////////////////////////////////////
/// \brief Apply the necessary transformation for
/// the selected coordinate system to the managed
/// assets.
///
/// \param coords CoordinateSystem
/// \return void
///
/////////////////////////////////////////////////
void PlotAssetManager::applyCoordSys(CoordinateSystem coords)
{
    // Do not apply cartesian coordinates
    if (coords == CARTESIAN)
        return;

    for (PlotAsset& ass : assets)
    {
        // Special case for 1D plots, as there are only two
        // possible valid combinations
        if (ass.getDim() == 1)
        {
            switch (coords)
            {
                case POLAR_PZ:
                case SPHERICAL_PT:
                    ass.applyModulus(XCOORD, 2.0*M_PI);
                    ass.removeNegativeValues(YCOORD);
                    break;
                case POLAR_RP:
                case POLAR_RZ:
                case SPHERICAL_RP:
                case SPHERICAL_RT:
                    ass.applyModulus(YCOORD, 2.0*M_PI);
                    ass.removeNegativeValues(XCOORD);
                    break;
                case CARTESIAN: break;
            }
        }
        else
        {
            switch (coords)
            {
                case POLAR_PZ:
                    ass.applyModulus(XCOORD, 2.0*M_PI);
                    ass.removeNegativeValues(ZCOORD);
                    break;
                case POLAR_RP:
                    ass.applyModulus(YCOORD, 2.0*M_PI);
                    ass.removeNegativeValues(XCOORD);
                    break;
                case POLAR_RZ:
                    ass.applyModulus(ZCOORD, 2.0*M_PI);
                    ass.removeNegativeValues(XCOORD);
                    break;
                case SPHERICAL_PT:
                    ass.applyModulus(XCOORD, 2.0*M_PI);
                    ass.applyModulus(YCOORD, 1.00001*M_PI);
                    ass.removeNegativeValues(ZCOORD);
                    break;
                case SPHERICAL_RP:
                    ass.applyModulus(YCOORD, 2.0*M_PI);
                    ass.applyModulus(ZCOORD, 1.00001*M_PI);
                    ass.removeNegativeValues(XCOORD);
                    break;
                case SPHERICAL_RT:
                    ass.applyModulus(ZCOORD, 2.0*M_PI);
                    ass.applyModulus(YCOORD, 1.00001*M_PI);
                    ass.removeNegativeValues(XCOORD);
                    break;
                case CARTESIAN: break;
            }
        }
    }
}


