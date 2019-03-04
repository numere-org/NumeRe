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

#include "wavelet.hpp"
#include "../ui/error.hpp"
#include "resampler.h"
#include <gsl/gsl_wavelet.h>

#include <cmath>

void calculateWavelet(std::vector<double>& data, WaveletType _type, int k, int dir)
{
    gsl_wavelet* wavelet = nullptr;
    gsl_wavelet_workspace* workspace = nullptr;

    size_t sze = 1 << (size_t)std::log2(data.size());

    // assure that the data size is fitting (power of 2)
    if (sze != data.size())
        throw SyntaxError(SyntaxError::WRONG_DATA_SIZE, "", SyntaxError::invalid_position);

    // allocate the wavelet type data
    switch (_type)
    {
        case Daubechies:
            if (k == 2) // special case: Daubechies with k = 2 is the haar wavelet
            {
                wavelet = gsl_wavelet_alloc(gsl_wavelet_haar, 2);
                break;
            }
            if (k < 4 || k > 20 || k % 2) // implemented values for k according https://www.gnu.org/software/gsl/manual/html_node/DWT-Initialization.html
                throw SyntaxError(SyntaxError::INVALID_WAVELET_COEFFICIENT, "", SyntaxError::invalid_position);
            wavelet = gsl_wavelet_alloc(gsl_wavelet_daubechies, k);
            break;
        case CenteredDaubechies:
            if (k == 2) // special case: Daubechies with k = 2 is the haar wavelet
            {
                wavelet = gsl_wavelet_alloc(gsl_wavelet_haar_centered, 2);
                break;
            }
            if (k < 4 || k > 20 || k % 2) // implemented values for k according https://www.gnu.org/software/gsl/manual/html_node/DWT-Initialization.html
                throw SyntaxError(SyntaxError::INVALID_WAVELET_COEFFICIENT, "", SyntaxError::invalid_position);
            wavelet = gsl_wavelet_alloc(gsl_wavelet_daubechies_centered, k);
            break;
        case Haar:
            wavelet = gsl_wavelet_alloc(gsl_wavelet_haar, 2); // implemented values for k according https://www.gnu.org/software/gsl/manual/html_node/DWT-Initialization.html
            break;
        case CenteredHaar:
            wavelet = gsl_wavelet_alloc(gsl_wavelet_haar_centered, 2); // implemented values for k according https://www.gnu.org/software/gsl/manual/html_node/DWT-Initialization.html
            break;
        case BSpline:
            if (k != 103 && k != 105 && !(k >= 202 && k <= 208 && !(k % 2)) && !(k >= 301 && k <= 309 && k % 2)) // implemented values for k according https://www.gnu.org/software/gsl/manual/html_node/DWT-Initialization.html
                throw SyntaxError(SyntaxError::INVALID_WAVELET_COEFFICIENT, "", SyntaxError::invalid_position);
            wavelet = gsl_wavelet_alloc(gsl_wavelet_bspline, k);
            break;
        case CenteredBSpline:
            if (k != 103 && k != 105 && !(k >= 202 && k <= 208 && !(k % 2)) && !(k >= 301 && k <= 309 && k % 2)) // implemented values for k according https://www.gnu.org/software/gsl/manual/html_node/DWT-Initialization.html
                throw SyntaxError(SyntaxError::INVALID_WAVELET_COEFFICIENT, "", SyntaxError::invalid_position);
            wavelet = gsl_wavelet_alloc(gsl_wavelet_bspline_centered, k);
            break;
    }

    // assure that the combination is possible
    if (wavelet == nullptr)
    {
        throw SyntaxError(SyntaxError::INVALID_WAVELET_TYPE, "", SyntaxError::invalid_position);
    }

    // allocate the workspace
    workspace = gsl_wavelet_workspace_alloc(data.size());

    // transform the data (done in the array itself)
    if (dir > 0)
        gsl_wavelet_transform_forward(wavelet, &data[0], 1, data.size(), workspace);
    else
        gsl_wavelet_transform_inverse(wavelet, &data[0], 1, data.size(), workspace);

    // free the data
    gsl_wavelet_free(wavelet);
    gsl_wavelet_workspace_free(workspace);
}


NumeRe::Table decodeWaveletData(const vector<double>& vWaveletData, const vector<double>& vAxisData)
{
    int nLevels = log2(vWaveletData.size());
    int nTimePoints = vWaveletData.size() / 2;
    NumeRe::Table wavelet;
    wavelet.setSize(nTimePoints, nLevels+3);
    double* data = new double[nTimePoints];
    const double* output = nullptr;

    // write the axes
    for (int i = -1; i < nLevels; i++)
    {
        wavelet.setValue(i+1, 1, i);
    }
    // Resample the axisdata
    Resampler axis(vAxisData.size(), 1, nTimePoints, 1, Resampler::BOUNDARY_CLAMP, 1.0, 0.0, "lanczos6");
    axis.put_line(&vAxisData[0]);
    const double* axis_out = axis.get_line();
    for (int i = 0; i < nTimePoints; i++)
    {
        wavelet.setValue(i, 0, axis_out[i]);
    }

    // write the data
    for (int i = 0; i <= nLevels; i++)
    {
        if (i < 2)
        {
            // this case means only to write the data to a whole line
            for (int j = 0; j < nTimePoints; j++)
            {
                wavelet.setValue(j, nLevels-i+2, vWaveletData[i]);
            }
        }
        else
        {
            // copy the coefficients of the current level
            for (int k = 0; k < (1 << ((i-1))); k++)
            {
                data[k] = vWaveletData[(1 << (i-1))+k];
            }
            // resample them
            Resampler res((1 << ((i-1))), 1, nTimePoints, 1, Resampler::BOUNDARY_CLAMP, 1.0, 0.0, "lanczos6");
            res.put_line(data);

            output = res.get_line();
            // write the output to the table
            for (int j = 0; j < nTimePoints; j++)
            {
                wavelet.setValue(j, nLevels-i+2, output[j]);
            }
        }

    }

    delete[] data;
    return wavelet;
}


