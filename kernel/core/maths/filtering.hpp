/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2020  Erik Haenel et al.

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

#ifndef FILTERING_HPP
#define FILTERING_HPP

#include <utility>
#include <vector>
#include <cmath>
#include <queue>
#include "../io/file.hpp"
#include "../utils/tools.hpp"
#include "../ParserLib/muParserDef.h"

//void showMatrix(const vector<vector<double> >&);

namespace NumeRe
{
    /////////////////////////////////////////////////
    /// \brief This structure contains the necessary
    /// information to create an instance of one of
    /// the following filters.
    /////////////////////////////////////////////////
    struct FilterSettings
    {
        enum FilterType
        {
            FILTER_NONE,
            FILTER_WEIGHTED_LINEAR,
            FILTER_GAUSSIAN,
            FILTER_SAVITZKY_GOLAY
        };

        FilterType type;
        size_t row;
        size_t col;
        double alpha;

        FilterSettings(FilterType _type = FILTER_NONE, size_t _row = 1u, size_t _col = 1u, double _alpha = NAN) : type(_type), row(std::max(_row, 1u)), col(std::max(_col, 1u)), alpha(_alpha)
        {
            //
        }
    };


    /////////////////////////////////////////////////
    /// \brief This function is a simple helper to
    /// implement a power of two.
    ///
    /// \param val double
    /// \return double
    ///
    /////////////////////////////////////////////////
    inline double pow2(double val)
    {
        return val * val;
    }


    /////////////////////////////////////////////////
    /// \brief This function is a simple helper to
    /// implement a power of three.
    ///
    /// \param val double
    /// \return double
    ///
    /////////////////////////////////////////////////
    inline double pow3(double val)
    {
        return val * val * val;
    }


    /////////////////////////////////////////////////
    /// \brief This function is a simple helper to
    /// implement a power of four.
    ///
    /// \param val double
    /// \return double
    ///
    /////////////////////////////////////////////////
    inline double pow4(double val)
    {
        return val * val * val * val;
    }


    /////////////////////////////////////////////////
    /// \brief Typedef for simplifying the usage of
    /// the buffer.
    /////////////////////////////////////////////////
    using FilterBuffer = std::queue<mu::value_type>;
    using FilterBuffer2D = std::queue<std::vector<mu::value_type>>;


    /////////////////////////////////////////////////
    /// \brief This is an abstract base class for any
    /// type of a data filter. Requires some methods
    /// to be implemented by its child classes.
    /////////////////////////////////////////////////
    class Filter
    {
        protected:
            FilterSettings::FilterType m_type;
            std::pair<size_t, size_t> m_windowSize;
            bool m_isConvolution;
            FilterBuffer m_buffer;
            FilterBuffer2D m_buffer2D;

        public:
            /////////////////////////////////////////////////
            /// \brief Filter base constructor. Will set the
            /// used window sizes.
            ///
            /// \param row size_t
            /// \param col size_t
            ///
            /////////////////////////////////////////////////
            Filter(size_t row, size_t col) : m_type(FilterSettings::FILTER_NONE), m_isConvolution(false)
            {
                // This expression avoids that someone tries to
                // create a filter with a zero dimension.
                m_windowSize = std::make_pair(std::max(row, 1u), std::max(col, 1u));
            }

            /////////////////////////////////////////////////
            /// \brief Empty virtual abstract destructor.
            /////////////////////////////////////////////////
            virtual ~Filter() {}

            /////////////////////////////////////////////////
            /// \brief Virtual operator() override. Has to be
            /// implemented in the child classes and shall
            /// return the kernel value at position (i,j).
            ///
            /// \param i size_t
            /// \param j size_t
            /// \return double
            ///
            /////////////////////////////////////////////////
            virtual double operator()(size_t i, size_t j) const = 0;

            /////////////////////////////////////////////////
            /// \brief Virtual method for applying the filter
            /// to a distinct value. Has to be implemented in
            /// all child classes.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \param val const mu::value_type&
            /// \return mu::value_type
            ///
            /////////////////////////////////////////////////
            virtual mu::value_type apply(size_t i, size_t j, const mu::value_type& val) const = 0;

            /////////////////////////////////////////////////
            /// \brief This method returns, whether the
            /// current filter is a convolution, ie. whether
            /// the returned value may be used directly or
            /// if all values of a window have to be
            /// accumulated first.
            ///
            /// \return bool
            ///
            /////////////////////////////////////////////////
            bool isConvolution() const
            {
                return m_isConvolution;
            }

            /////////////////////////////////////////////////
            /// \brief This method returns the type of the
            /// current filter as a value of the FilterType
            /// enumeration.
            ///
            /// \return FilterType
            ///
            /////////////////////////////////////////////////
            FilterSettings::FilterType getType() const
            {
                return m_type;
            }

            /////////////////////////////////////////////////
            /// \brief This method returns the window size of
            /// the current filter as a std::pair in the
            /// order (row,col).
            ///
            /// \return std::pair<size_t,size_t>
            ///
            /////////////////////////////////////////////////
            std::pair<size_t,size_t> getWindowSize() const
            {
                return m_windowSize;
            }

            /////////////////////////////////////////////////
            /// \brief This method returns the internal
            /// filtering buffer queue to store already
            /// smoothed points avoiding leakage effects.
            ///
            /// \return FilterBuffer&
            ///
            /////////////////////////////////////////////////
            FilterBuffer& getBuffer()
            {
                return m_buffer;
            }

            /////////////////////////////////////////////////
            /// \brief This method returns the internal
            /// filtering buffer queue for 2D data to store
            /// already smoothed points avoiding leakage
            /// effects.
            ///
            /// \return FilterBuffer2D&
            ///
            /////////////////////////////////////////////////
            FilterBuffer2D& get2DBuffer()
            {
                return m_buffer2D;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class implements a weighted
    /// linear smoothing filter, which applies
    /// something like a "convergent sliding average"
    /// filter.
    /////////////////////////////////////////////////
    class WeightedLinearFilter : public Filter
    {
        private:
            bool is2D;
            std::vector<std::vector<double> > m_filterKernel;

            /////////////////////////////////////////////////
            /// \brief This method will create the filter's
            /// kernel for the selected window size.
            ///
            /// \return void
            ///
            /////////////////////////////////////////////////
            void createKernel()
            {
                if (!is2D)
                    is2D = m_windowSize.first > 1 && m_windowSize.second > 1;

                if (!(m_windowSize.first % 2))
                    m_windowSize.first++;

                m_filterKernel = std::vector<std::vector<double> >(m_windowSize.first, std::vector<double>(m_windowSize.second, 0.0));

                double mean_row = m_windowSize.first > 1 ? 0.5 : 0.0;
                double mean_col = m_windowSize.second > 1 ? 0.5 : 0.0;
                double sum = 0;

                // Calculate the filter values
                for (size_t i = 0; i < m_windowSize.first; i++)
                {
                    for (size_t j = 0; j < m_windowSize.second; j++)
                    {
                        if (sqrt(pow2(i/((double)std::max(1u, m_windowSize.first-1))-mean_row) + pow2(j/((double)std::max(1u, m_windowSize.second-1))-mean_col)) <= 0.5)
                        {
                            m_filterKernel[i][j] = fabs(sqrt(pow2(i/((double)std::max(1u, m_windowSize.first-1))-mean_row) + pow2(j/((double)std::max(1u, m_windowSize.second-1))-mean_col)) - 0.5);
                            sum += m_filterKernel[i][j];
                        }
                    }
                }

                for (size_t i = 0; i < m_windowSize.first; i++)
                {
                    for (size_t j = 0; j < m_windowSize.second; j++)
                    {
                        m_filterKernel[i][j] /= sum;
                    }
                }
            }

        public:
            /////////////////////////////////////////////////
            /// \brief Filter constructor. Will automatically
            /// create the filter kernel.
            ///
            /// \param row size_t
            /// \param col size_t
            /// \param force2D bool
            ///
            /////////////////////////////////////////////////
            WeightedLinearFilter(size_t row, size_t col, bool force2D = false) : Filter(row, col)
            {
                m_type = FilterSettings::FILTER_WEIGHTED_LINEAR;
                m_isConvolution = true;
                is2D = force2D;

                createKernel();
            }

            /////////////////////////////////////////////////
            /// \brief Filter destructor. Will clear the
            /// previously calculated filter kernel.
            /////////////////////////////////////////////////
            virtual ~WeightedLinearFilter() override
            {
                m_filterKernel.clear();
            }

            /////////////////////////////////////////////////
            /// \brief Override for the operator(). Returns
            /// the filter kernel at the desired position.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \return double
            ///
            /////////////////////////////////////////////////
            virtual double operator()(size_t i, size_t j) const override
            {
                if (i >= m_windowSize.first || j >= m_windowSize.second)
                    return NAN;

                return m_filterKernel[i][j];
            }

            /////////////////////////////////////////////////
            /// \brief Override for the abstract apply method
            /// of the base class. Applies the filter to the
            /// value at the selected position and returns
            /// the new value.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \param val const mu::value_type&
            /// \return virtual mu::value_type
            ///
            /////////////////////////////////////////////////
            virtual mu::value_type apply(size_t i, size_t j, const mu::value_type& val) const override
            {
                if (i >= m_windowSize.first || j >= m_windowSize.second)
                    return NAN;

                return m_filterKernel[i][j]*val;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class implements a gaussian
    /// smoothing or blurring filter.
    /////////////////////////////////////////////////
    class GaussianFilter : public Filter
    {
        private:
            std::vector<std::vector<double> > m_filterKernel;

            /////////////////////////////////////////////////
            /// \brief This method will create the filter's
            /// kernel for the selected window size.
            ///
            /// \return void
            ///
            /////////////////////////////////////////////////
            void createKernel(double sigma)
            {
                m_filterKernel = std::vector<std::vector<double> >(m_windowSize.first, std::vector<double>(m_windowSize.second, 0.0));

                double sum = 0.0;
                double mean_row = (m_windowSize.first - 1) * 0.5;
                double mean_col = (m_windowSize.second - 1) * 0.5;

                for (size_t i = 0; i < m_windowSize.first; i++)
                {
                    for (size_t j = 0; j < m_windowSize.second; j++)
                    {
                        m_filterKernel[i][j] = exp(-(pow2(i-mean_row) + pow2(j-mean_col)) / (2*pow2(sigma))) / (2*M_PI*pow2(sigma));
                        sum += m_filterKernel[i][j];
                    }
                }

                for (size_t i = 0; i < m_windowSize.first; i++)
                {
                    for (size_t j = 0; j < m_windowSize.second; j++)
                    {
                        m_filterKernel[i][j] /= sum;
                    }
                }
            }

        public:
            /////////////////////////////////////////////////
            /// \brief Filter constructor. Will automatically
            /// create the filter kernel.
            ///
            /// \param row
            /// \param col
            ///
            /////////////////////////////////////////////////
            GaussianFilter(size_t row, size_t col, double sigma) : Filter(row, col)
            {
                m_type = FilterSettings::FILTER_GAUSSIAN;
                m_isConvolution = true;

                createKernel(sigma);
            }

            /////////////////////////////////////////////////
            /// \brief Filter destructor. Will clear the
            /// previously calculated filter kernel.
            /////////////////////////////////////////////////
            virtual ~GaussianFilter() override
            {
                m_filterKernel.clear();
            }

            /////////////////////////////////////////////////
            /// \brief Override for the operator(). Returns
            /// the filter kernel at the desired position.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \return double
            ///
            /////////////////////////////////////////////////
            virtual double operator()(size_t i, size_t j) const override
            {
                if (i >= m_windowSize.first || j >= m_windowSize.second)
                    return NAN;

                return m_filterKernel[i][j];
            }

            /////////////////////////////////////////////////
            /// \brief Override for the abstract apply method
            /// of the base class. Applies the filter to the
            /// value at the selected position and returns
            /// the new value.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \param val const mu::value_type&
            /// \return virtual mu::value_type
            ///
            /////////////////////////////////////////////////
            virtual mu::value_type apply(size_t i, size_t j, const mu::value_type& val) const override
            {
                if (i >= m_windowSize.first || j >= m_windowSize.second)
                    return NAN;

                if (mu::isnan(val))
                    return 0.0;

                // Gaussian is symmetric, therefore the convolution does not
                // need to be inverted
                return m_filterKernel[i][j] * val;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class implements a Savitzky-Golay
    /// filter, which is a polynomial smoothing
    /// filter.
    /////////////////////////////////////////////////
    class SavitzkyGolayFilter : public Filter
    {
        private:
            std::vector<std::vector<double>> m_filterKernel;

            /////////////////////////////////////////////////
            /// \brief This private member function finds the
            /// column, which either fits perfectly or is the
            /// nearest possibility to the selected window
            /// size.
            ///
            /// \param _view const FileView&
            /// \return long long int
            ///
            /////////////////////////////////////////////////
            long long int findColumn(const FileView& _view)
            {
                std::vector<size_t> vWindowSizes;

                // Decode all contained window sizes
                for (long long int j = 0; j < _view.getCols(); j++)
                {
                    vWindowSizes.push_back(StrToInt(_view.getColumnHead(j).substr(0, _view.getColumnHead(j).find('x'))));
                }

                // Window size is already smaller than the first
                // available window size
                if (m_windowSize.first < vWindowSizes.front())
                {
                    m_windowSize.first = vWindowSizes.front();
                    m_windowSize.second = m_windowSize.first;
                    m_filterKernel = std::vector<std::vector<double> >(m_windowSize.first, std::vector<double>(m_windowSize.second, 0.0));

                    return 0;
                }

                for (long long int j = 0; j < vWindowSizes.size(); j++)
                {
                    // Found a perfect match?
                    if (m_windowSize.first == vWindowSizes[j])
                    {
                        m_windowSize.second = m_windowSize.first;
                        m_filterKernel = std::vector<std::vector<double> >(m_windowSize.first, std::vector<double>(m_windowSize.second, 0.0));

                        return j;
                    }

                    // Is there a nearest match? (We assume
                    // ascending order of the window sizes)
                    if (m_windowSize.first > vWindowSizes[j] && j+1 < vWindowSizes.size() && m_windowSize.first < vWindowSizes[j+1])
                    {
                        // If the following fits better, increment the index
                        if (m_windowSize.first - vWindowSizes[j] > vWindowSizes[j+1] - m_windowSize.first)
                            j++;

                        m_windowSize.first = vWindowSizes[j];
                        m_windowSize.second = m_windowSize.first;
                        m_filterKernel = std::vector<std::vector<double> >(m_windowSize.first, std::vector<double>(m_windowSize.second, 0.0));

                        return j;
                    }
                    else if (m_windowSize.first > vWindowSizes[j] && j+1 == vWindowSizes.size())
                    {
                        m_windowSize.first = vWindowSizes[j];
                        m_windowSize.second = m_windowSize.first;
                        m_filterKernel = std::vector<std::vector<double> >(m_windowSize.first, std::vector<double>(m_windowSize.second, 0.0));

                        return j;
                    }
                }

                return 0;
            }

            /////////////////////////////////////////////////
            /// \brief This method will create the filter's
            /// kernel for the selected window size.
            ///
            /// \return void
            ///
            /////////////////////////////////////////////////
            void createKernel()
            {
                // Ensure odd numbers
                if (!(m_windowSize.first % 2))
                    m_windowSize.first++;

                // Create a 2D kernel
                if (m_windowSize.second > 1)
                {
                    // Create a file instance
                    GenericFile* _file = getFileByType("<>/params/savitzky_golay_coeffs_2D.dat");

                    if (!_file)
                        return;

                    // Read the contents and assign it to a view
                    try
                    {
                        _file->read();
                    }
                    catch (...)
                    {
                        delete _file;
                        throw;
                    }

                    FileView _view(_file);

                    // Find the possible window sizes
                    long long int j = findColumn(_view);

                    // First element in the column is the
                    // central element in the matrix
                    m_filterKernel[m_windowSize.first/2][m_windowSize.first/2] = _view.getElement(m_windowSize.first*m_windowSize.first/2, j).real();

                    for (long long int i = 0; i < m_windowSize.first*m_windowSize.first/2; i++)
                    {
                        // left part
                        m_filterKernel[m_windowSize.first - 1 - i % m_windowSize.first][i / m_windowSize.first] = _view.getElement(i, j).real();
                        // right part
                        m_filterKernel[i % m_windowSize.first][m_windowSize.first - i / m_windowSize.first - 1] = _view.getElement(i, j).real();
                        // middle column
                    }

                    delete _file;
                    return;
                }

                // Create a 1D kernel
                m_filterKernel = std::vector<std::vector<double> >(m_windowSize.first, std::vector<double>(m_windowSize.second, 0.0));

                for (size_t i = 0; i < m_windowSize.first; i++)
                {
                    m_filterKernel[i][0] = (3.0*pow2(m_windowSize.first) - 7.0 - 20.0*pow2((int)i - (int)m_windowSize.first/2)) / 4.0 / (m_windowSize.first * (pow2(m_windowSize.first) - 4.0) / 3.0);
                }
            }

        public:
            /////////////////////////////////////////////////
            /// \brief Filter constructor. Will automatically
            /// create the filter kernel.
            ///
            /// \param row
            /// \param col
            ///
            /////////////////////////////////////////////////
            SavitzkyGolayFilter(size_t row, size_t col) : Filter(row, col)
            {
                m_type = FilterSettings::FILTER_SAVITZKY_GOLAY;
                m_isConvolution = true;

                createKernel();
            }

            /////////////////////////////////////////////////
            /// \brief Filter destructor. Will clear the
            /// previously calculated filter kernel.
            /////////////////////////////////////////////////
            virtual ~SavitzkyGolayFilter() override
            {
                m_filterKernel.clear();
            }

            /////////////////////////////////////////////////
            /// \brief Override for the operator(). Returns
            /// the filter kernel at the desired position.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \return double
            ///
            /////////////////////////////////////////////////
            virtual double operator()(size_t i, size_t j) const override
            {
                if (i >= m_windowSize.first || j >= m_windowSize.second)
                    return NAN;

                return m_filterKernel[i][j];
            }

            /////////////////////////////////////////////////
            /// \brief Override for the abstract apply method
            /// of the base class. Applies the filter to the
            /// value at the selected position and returns
            /// the new value.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \param val const mu::value_type&
            /// \return virtual mu::value_type
            ///
            /////////////////////////////////////////////////
            virtual mu::value_type apply(size_t i, size_t j, const mu::value_type& val) const override
            {
                if (i >= m_windowSize.first || j >= m_windowSize.second)
                    return NAN;

                if (mu::isnan(val))
                    return 0.0;

                return m_filterKernel[i][j] * val;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This class implements a Savitzky-Golay
    /// filter for differentiation providing a
    /// derivative up to degree three.
    /////////////////////////////////////////////////
    class SavitzkyGolayDiffFilter : public Filter
    {
        private:
            std::vector<double> m_filterKernel;

            /////////////////////////////////////////////////
            /// \brief This method will create the filter's
            /// kernel for the selected window size.
            ///
            /// \param nthDerivative size_t
            /// \return void
            ///
            /////////////////////////////////////////////////
            void createKernel(size_t nthDerivative)
            {
                // Ensure odd numbers
                if (!(m_windowSize.first % 2))
                    m_windowSize.first++;

                // Create a 1D kernel
                m_filterKernel = std::vector<double>(m_windowSize.first, 0.0);
                double m = m_windowSize.first;

                for (size_t i = 0; i < m_windowSize.first; i++)
                {
                    double I = (int)i - floor(m/2.0);

                    if (nthDerivative == 1)
                        m_filterKernel[i] = (5.0*(3.0*pow4(m) - 18.0*pow2(m) + 31.0)*I - 28.0*(3.0*pow2(m)-7.0)*pow3(I))
                                             / (m * (pow2(m) - 1.0)*(3.0*pow4(m) - 39.0*pow2(m) + 108.0) / 15.0);
                    else if (nthDerivative == 2)
                        m_filterKernel[i] = (12.0*m*pow2(I) - m * (pow2(m) - 1.0))
                                             / (pow2(m) * (pow2(m) - 1.0) * (pow2(m) - 4.0) / 30.0);
                    else if (nthDerivative == 3)
                        m_filterKernel[i] = (-(3.0*pow2(m) - 7.0)*I + 20.0*pow3(I))
                                             / (m * (pow2(m) - 1.0)*(3.0*pow4(m)-39.0*pow2(m) + 108.0) / 420.0);
                    else
                        m_filterKernel[i] = (5.0*(3.0*pow4(m) - 18.0*pow2(m) + 31.0)*I - 28.0*(3.0*pow2(m)-7.0)*pow3(I))
                                             / (m * (pow2(m) - 1.0)*(3.0*pow4(m) - 39.0*pow2(m) + 108.0) / 15.0);
                }
            }

        public:
            /////////////////////////////////////////////////
            /// \brief Filter constructor. Will automatically
            /// create the filter kernel.
            ///
            /// \param row size_t
            /// \param nthDerivative size_t
            ///
            /////////////////////////////////////////////////
            SavitzkyGolayDiffFilter(size_t row, size_t nthDerivative) : Filter(row, 1)
            {
                m_type = FilterSettings::FILTER_SAVITZKY_GOLAY;
                m_isConvolution = true;

                createKernel(nthDerivative);
            }

            /////////////////////////////////////////////////
            /// \brief Filter destructor. Will clear the
            /// previously calculated filter kernel.
            /////////////////////////////////////////////////
            virtual ~SavitzkyGolayDiffFilter() override
            {
                m_filterKernel.clear();
            }

            /////////////////////////////////////////////////
            /// \brief Override for the operator(). Returns
            /// the filter kernel at the desired position.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \return double
            ///
            /////////////////////////////////////////////////
            virtual double operator()(size_t i, size_t j) const override
            {
                if (i >= m_windowSize.first)
                    return NAN;

                return m_filterKernel[i];
            }

            /////////////////////////////////////////////////
            /// \brief Override for the abstract apply method
            /// of the base class. Applies the filter to the
            /// value at the selected position and returns
            /// the new value.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \param val const mu::value_type&
            /// \return virtual mu::value_type
            ///
            /////////////////////////////////////////////////
            virtual mu::value_type apply(size_t i, size_t j, const mu::value_type& val) const override
            {
                if (i >= m_windowSize.first)
                    return NAN;

                if (mu::isnan(val))
                    return 0.0;

                return m_filterKernel[i] * val;
            }
    };


    /////////////////////////////////////////////////
    /// \brief This function creates an instance of
    /// the filter specified by the passed
    /// FilterSettings structure.
    ///
    /// The Filter will be created on the heap. The
    /// calling function is responsible for freeing
    /// its memory.
    ///
    /// \param _settings const FilterSettings&
    /// \return Filter*
    ///
    /////////////////////////////////////////////////
    inline Filter* createFilter(const FilterSettings& _settings)
    {
        switch (_settings.type)
        {
            case FilterSettings::FILTER_NONE:
                return nullptr;
            case FilterSettings::FILTER_WEIGHTED_LINEAR:
                return new WeightedLinearFilter(_settings.row, _settings.col);
            case FilterSettings::FILTER_GAUSSIAN:
                return new GaussianFilter(_settings.row, _settings.col, (std::max(_settings.row, _settings.col)-1)/(2*_settings.alpha));
            case FilterSettings::FILTER_SAVITZKY_GOLAY:
                return new SavitzkyGolayFilter(_settings.row, _settings.col);
        }

        return nullptr;
    }


    /////////////////////////////////////////////////
    /// \brief This class is a specialized
    /// WeightedLinearFilter used to retouch missing
    /// data values.
    /////////////////////////////////////////////////
    class RetouchRegion : public Filter
    {
        private:
            std::vector<mu::value_type> m_left;
            std::vector<mu::value_type> m_right;
            std::vector<mu::value_type> m_top;
            std::vector<mu::value_type> m_bottom;
            bool is2D;
            std::vector<std::vector<double> > m_filterKernel;
            mu::value_type m_fallback;
            bool m_invertedKernel;

            /////////////////////////////////////////////////
            /// \brief This method will create the filter's
            /// kernel for the selected window size.
            ///
            /// \return void
            ///
            /////////////////////////////////////////////////
            void createKernel()
            {
                m_filterKernel = std::vector<std::vector<double> >(m_windowSize.first, std::vector<double>(m_windowSize.second, 0.0));

                if (!is2D)
                    is2D = m_windowSize.first > 1 && m_windowSize.second > 1;

                double mean_row = m_windowSize.first > 1 ? 0.5 : 0.0;
                double mean_col = m_windowSize.second > 1 ? 0.5 : 0.0;

                // Calculate the filter values
                for (size_t i = 0; i < m_windowSize.first; i++)
                {
                    for (size_t j = 0; j < m_windowSize.second; j++)
                    {
                        if (sqrt(pow2(i/((double)std::max(1u, m_windowSize.first-1))-mean_row) + pow2(j/((double)std::max(1u, m_windowSize.second-1))-mean_col)) <= 0.5)
                        {
                            m_filterKernel[i][j] = fabs(sqrt(pow2(i/((double)std::max(1u, m_windowSize.first-1))-mean_row) + pow2(j/((double)std::max(1u, m_windowSize.second-1))-mean_col)) - 0.5);
                        }
                    }
                }
            }

            /////////////////////////////////////////////////
            /// \brief This method will return the correct
            /// value for the left interval boundary.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \return mu::value_type
            ///
            /////////////////////////////////////////////////
            mu::value_type left(size_t i, size_t j) const
            {
                if (!is2D)
                    return validize(m_left.front());

                return validize(m_left[i+1]);
            }

            /////////////////////////////////////////////////
            /// \brief This method will return the correct
            /// value for the right interval boundary.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \return mu::value_type
            ///
            /////////////////////////////////////////////////
            mu::value_type right(size_t i, size_t j) const
            {
                if (!is2D)
                    return validize(m_right.front());

                return validize(m_right[i+1]);
            }

            /////////////////////////////////////////////////
            /// \brief This method will return the correct
            /// value for the top interval boundary.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \return mu::value_type
            ///
            /////////////////////////////////////////////////
            mu::value_type top(size_t i, size_t j) const
            {
                if (!is2D)
                    return 0.0;

                return validize(m_top[j+1]);
            }

            /////////////////////////////////////////////////
            /// \brief This method will return the correct
            /// value for the bottom interval boundary.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \return mu::value_type
            ///
            /////////////////////////////////////////////////
            mu::value_type bottom(size_t i, size_t j) const
            {
                if (!is2D)
                    return 0.0;

                return validize(m_bottom[j+1]);
            }

            /////////////////////////////////////////////////
            /// \brief This method will return the correct
            /// value for the topleft diagonal interval
            /// boundary.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \return mu::value_type
            ///
            /////////////////////////////////////////////////
            mu::value_type topleft(size_t i, size_t j) const
            {
                if (i >= j)
                    return validize(m_left[i-j]);

                return validize(m_top[j-i]);
            }

            /////////////////////////////////////////////////
            /// \brief This method will return the correct
            /// value for the topright diagonal interval
            /// boundary.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \return mu::value_type
            ///
            /////////////////////////////////////////////////
            mu::value_type topright(size_t i, size_t j) const
            {
                if (i+j <= m_windowSize.second-1)
                    return validize(m_top[i+j+2]); // size(m_top) + 2 == m_order

                return validize(m_right[i+j-m_windowSize.second+1]);
            }

            /////////////////////////////////////////////////
            /// \brief This method will return the correct
            /// value for the bottomleft diagonal interval
            /// boundary.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \return mu::value_type
            ///
            /////////////////////////////////////////////////
            mu::value_type bottomleft(size_t i, size_t j) const
            {
                if (i+j <= m_windowSize.first-1)
                    return validize(m_left[i+j+2]); // size(m_left) + 2 == m_order

                return validize(m_bottom[i+j-m_windowSize.first+1]);
            }

            /////////////////////////////////////////////////
            /// \brief This method will return the correct
            /// value for the bottomright diagonal interval
            /// boundary.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \return mu::value_type
            ///
            /////////////////////////////////////////////////
            mu::value_type bottomright(size_t i, size_t j) const
            {
                if (i >= j)
                    return validize(m_bottom[m_windowSize.second-(i-j)+1]); // size(m_bottom) + 2 == m_order

                return validize(m_right[m_windowSize.first-(j-i)+1]);
            }

            /////////////////////////////////////////////////
            /// \brief This method checks, whether the passed
            /// value is a valid value and returns it. If it
            /// is not, it will be replaced by the fallback
            /// value.
            ///
            /// \param val mu::value_type
            /// \return mu::value_type
            ///
            /////////////////////////////////////////////////
            mu::value_type validize(mu::value_type val) const
            {
                if (mu::isnan(val))
                    return m_fallback;

                return val;
            }

        public:
            /////////////////////////////////////////////////
            /// \brief Filter constructor. Will automatically
            /// create the filter kernel.
            ///
            /// \param _row size_t
            /// \param _col size_t
            /// \param _dMedian const mu::value_type&
            ///
            /////////////////////////////////////////////////
            RetouchRegion(size_t _row, size_t _col, const mu::value_type& _dMedian) : Filter(_row, _col)
            {
                m_type = FilterSettings::FILTER_WEIGHTED_LINEAR;
                m_isConvolution = false;
                m_fallback = _dMedian;
                m_invertedKernel = true;
                is2D = true;

                createKernel();
            }

            /////////////////////////////////////////////////
            /// \brief Filter destructor. Will clear the
            /// previously calculated filter kernel.
            /////////////////////////////////////////////////
            virtual ~RetouchRegion() override
            {
                m_filterKernel.clear();
            }

            /////////////////////////////////////////////////
            /// \brief Override for the operator(). Returns
            /// the filter kernel at the desired position.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \return double
            ///
            /////////////////////////////////////////////////
            virtual double operator()(size_t i, size_t j) const override
            {
                if (i >= m_windowSize.first || j >= m_windowSize.second)
                    return NAN;

                return m_filterKernel[i][j];
            }

            /////////////////////////////////////////////////
            /// \brief Override for the abstract apply method
            /// of the base class. Applies the filter to the
            /// value at the selected position and returns
            /// the new value.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \param val const mu::value_type&
            /// \return virtual mu::value_type
            ///
            /////////////////////////////////////////////////
            virtual mu::value_type apply(size_t i, size_t j, const mu::value_type& val) const override
            {
                if (i >= m_windowSize.first || j >= m_windowSize.second)
                    return NAN;

                if (!is2D)
                {
                    if (m_invertedKernel)
                        return m_filterKernel[i][j]*val + (1-m_filterKernel[i][j])*(left(i,j) + (right(i,j) - left(i,j))/(m_windowSize.first + 1.0)*(i + 1.0));

                    return (1-m_filterKernel[i][j])*val + m_filterKernel[i][j]*(left(i,j) + (right(i,j) - left(i,j))/(m_windowSize.first + 1.0)*(i + 1.0));
                }

                // cross hair: summarize the linearily interpolated values along the rows and cols at the desired position
                // Summarize implies that the value is not averaged yet
                mu::value_type dAverage = top(i,j) + (bottom(i,j) - top(i,j)) / (m_windowSize.first + 1.0) * (i+1.0)
                                        + left(i,j) + (right(i,j) - left(i,j)) / (m_windowSize.second + 1.0) * (j+1.0);

                // Additional weighting because are the nearest neighbours
                dAverage /= 2.0;

                // Calculate along columns
                // Find the diagonal neighbours and interpolate the value
                if (i >= j)
                    dAverage += topleft(i,j) + (bottomright(i,j) - topleft(i,j)) / (m_windowSize.second - fabs(i-j) + 1.0) * (j+1.0);
                else
                    dAverage += topleft(i,j) + (bottomright(i,j) - topleft(i,j)) / (m_windowSize.first - fabs(i-j) + 1.0) * (i+1.0);

                // calculate along rows
                // Find the diagonal neighbours and interpolate the value
                if (i + j <= m_windowSize.first + 1)
                    dAverage += bottomleft(i,j) + (topright(i,j) - bottomleft(i,j)) / (i+j+2.0) * (j+1.0);
                else
                    dAverage += bottomleft(i,j) + (topright(i,j) - bottomleft(i,j)) / (double)(m_windowSize.first + m_windowSize.second - i - j) * (double)(m_windowSize.first-i);

                // Restore the desired average
                dAverage /= 6.0;

                if (m_invertedKernel)
                    return (1-m_filterKernel[i][j])*dAverage + m_filterKernel[i][j]*val;

                return (1-m_filterKernel[i][j])*val + m_filterKernel[i][j]*dAverage;
            }

            /////////////////////////////////////////////////
            /// \brief This method is used to update the
            /// internal filter boundaries.
            ///
            /// \param left const std::vector<mu::value_type>&
            /// \param right const std::vector<mu::value_type>&
            /// \param top const std::vector<mu::value_type>&
            /// \param bottom const std::vector<mu::value_type>&
            /// \return void
            ///
            /////////////////////////////////////////////////
            void setBoundaries(const std::vector<mu::value_type>& left, const std::vector<mu::value_type>& right, const std::vector<mu::value_type>& top = std::vector<mu::value_type>(), const std::vector<mu::value_type>& bottom = std::vector<mu::value_type>())
            {
                m_left = left;
                m_right = right;
                m_top = top;
                m_bottom = bottom;
            }

            /////////////////////////////////////////////////
            /// \brief This method is a wrapper to retouch
            /// only invalid values. The default value of
            /// invalid values is the median value declared
            /// at construction time.
            ///
            /// \param i size_t
            /// \param j size_t
            /// \param val const mu::value_type&
            /// \param med const mu::value_type&
            /// \return mu::value_type
            ///
            /////////////////////////////////////////////////
            mu::value_type retouch(size_t i, size_t j, const mu::value_type& val, const mu::value_type& med)
            {
                if (mu::isnan(val) && !mu::isnan(med))
                    return 0.5*(apply(i, j, m_fallback) + med);
                else if (mu::isnan(val) && mu::isnan(med))
                    return apply(i, j, m_fallback);

                return val;
            }
    };
}



#endif // FILTERING_HPP


