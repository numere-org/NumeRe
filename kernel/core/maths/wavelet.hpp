/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2018  Erik Haenel et al.

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


#ifndef WAVELET_HPP
#define WAVELET_HPP

#include <vector>
#include "../datamanagement/table.hpp"

enum WaveletType
{
    Daubechies,
    CenteredDaubechies,
    Haar,
    CenteredHaar,
    BSpline,
    CenteredBSpline
};

void calculateWavelet(std::vector<double>& data, WaveletType _type, int k, int dir);
NumeRe::Table decodeWaveletData(const std::vector<double>& vWaveletData, const std::vector<double>& vAxisData);

#endif // WAVELET_HPP

