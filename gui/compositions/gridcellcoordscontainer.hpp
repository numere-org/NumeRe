/*****************************************************************************
    NumeRe: Framework fuer Numerische Rechnungen
    Copyright (C) 2022  Erik Haenel et al.

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

#ifndef GRIDCELLCOORDSCONTAINER_HPP
#define GRIDCELLCOORDSCONTAINER_HPP

#include <wx/grid.h>

/////////////////////////////////////////////////
/// \brief A simple structure to define the
/// needed grid space to enclose all cells
/// contained in the wxGridCellCoordsContainer.
/////////////////////////////////////////////////
struct wxGridCellsExtent
{
    wxGridCellCoords m_topleft;
    wxGridCellCoords m_bottomright;
};


/////////////////////////////////////////////////
/// \brief A class to simplify the access to
/// different types of grid cell coords.
/// Especially useful in the context of selected
/// cells (which may come in different flavors).
/////////////////////////////////////////////////
class wxGridCellCoordsContainer
{
    private:
        wxGridCellsExtent m_extent;
        wxGridCellCoordsArray m_array;
        wxArrayInt m_rowsOrCols;
        bool m_rowsSelected;
        wxGrid* m_grid;

        /////////////////////////////////////////////////
        /// \brief The extent's boundary might be hidden
        /// cells. Shrink it to the largest boundary,
        /// which is visble.
        ///
        /// \return void
        ///
        /////////////////////////////////////////////////
        void fitExtentToShownCells()
        {
            if (m_grid)
            {
                while (!m_grid->IsColShown(m_extent.m_topleft.GetCol())
                       && m_extent.m_topleft.GetCol() < m_extent.m_bottomright.GetCol())
                    m_extent.m_topleft.SetCol(m_extent.m_topleft.GetCol()+1);

                while (!m_grid->IsRowShown(m_extent.m_topleft.GetRow())
                       && m_extent.m_topleft.GetRow() < m_extent.m_bottomright.GetRow())
                    m_extent.m_topleft.SetRow(m_extent.m_topleft.GetRow()+1);

                // Do not include the right frame
                if (m_grid->GetNumberCols() == m_extent.m_bottomright.GetCol()+1)
                    m_extent.m_bottomright.SetCol(m_extent.m_bottomright.GetCol()-1);

                while (!m_grid->IsColShown(m_extent.m_bottomright.GetCol())
                       && m_extent.m_bottomright.GetCol() > m_extent.m_topleft.GetCol())
                    m_extent.m_bottomright.SetCol(m_extent.m_bottomright.GetCol()-1);

                // Do not include the bottom frame
                if (m_grid->GetNumberRows() == m_extent.m_bottomright.GetRow()+1)
                    m_extent.m_bottomright.SetRow(m_extent.m_bottomright.GetRow()-1);

                while (!m_grid->IsRowShown(m_extent.m_bottomright.GetRow())
                       && m_extent.m_bottomright.GetRow() > m_extent.m_topleft.GetRow())
                    m_extent.m_bottomright.SetRow(m_extent.m_bottomright.GetRow()-1);
            }
        }

    public:

        /////////////////////////////////////////////////
        /// \brief Construct an empty
        /// wxGridCellCoordsContainer.
        /////////////////////////////////////////////////
        wxGridCellCoordsContainer() = default;

        /////////////////////////////////////////////////
        /// \brief Construct a wxGridCellCoordsContainer
        /// from an upperleft and a lowerright coordinate
        /// pair.
        ///
        /// \param topleft const wxGridCellCoords&
        /// \param bottomright const wxGridCellCoords&
        /// \param grid wxGrid*
        ///
        /////////////////////////////////////////////////
        wxGridCellCoordsContainer(const wxGridCellCoords& topleft, const wxGridCellCoords& bottomright, wxGrid* grid = nullptr) : m_rowsSelected(false), m_grid(grid)
        {
            m_extent.m_topleft = topleft;
            m_extent.m_bottomright = bottomright;

            fitExtentToShownCells();
        }

        /////////////////////////////////////////////////
        /// \brief Construct a wxGridCellCoordsContainer
        /// from a list of selected grid coordinates.
        ///
        /// \param selected const wxGridCellCoordsArray&
        /// \param grid wxGrid*
        ///
        /////////////////////////////////////////////////
        wxGridCellCoordsContainer(const wxGridCellCoordsArray& selected, wxGrid* grid = nullptr) : m_rowsSelected(false), m_grid(grid)
        {
            m_array = selected;
            m_extent.m_topleft = m_array[0];
            m_extent.m_bottomright = m_array[0];

            for (size_t n = 1; n < m_array.size(); n++)
            {
                m_extent.m_topleft.SetRow(std::min(m_extent.m_topleft.GetRow(), m_array[n].GetRow()));
                m_extent.m_topleft.SetCol(std::min(m_extent.m_topleft.GetCol(), m_array[n].GetCol()));
                m_extent.m_bottomright.SetRow(std::max(m_extent.m_bottomright.GetRow(), m_array[n].GetRow()));
                m_extent.m_bottomright.SetCol(std::max(m_extent.m_bottomright.GetCol(), m_array[n].GetCol()));
            }

            fitExtentToShownCells();
        }

        /////////////////////////////////////////////////
        /// \brief Construct a wxGridCellCoordsContainer
        /// from a set of indices for complete rows or
        /// columns.
        ///
        /// \param selectedRowsOrCols const wxArrayInt&
        /// \param otherDim int
        /// \param rowsSelected bool
        /// \param grid wxGrid*
        ///
        /////////////////////////////////////////////////
        wxGridCellCoordsContainer(const wxArrayInt& selectedRowsOrCols, int otherDim, bool rowsSelected = true, wxGrid* grid = nullptr) : m_rowsSelected(rowsSelected), m_grid(grid)
        {
            m_rowsOrCols = selectedRowsOrCols;
            m_extent.m_topleft = m_rowsSelected ? wxGridCellCoords(m_rowsOrCols[0], 0) : wxGridCellCoords(0, m_rowsOrCols[0]);
            m_extent.m_bottomright = m_rowsSelected ? wxGridCellCoords(m_rowsOrCols[0], otherDim) : wxGridCellCoords(otherDim, m_rowsOrCols[0]);

            for (size_t n = 0; n < m_rowsOrCols.size(); n++)
            {
                if (m_rowsSelected)
                {
                    m_extent.m_topleft.SetRow(std::min(m_extent.m_topleft.GetRow(), m_rowsOrCols[n]));
                    m_extent.m_bottomright.SetRow(std::max(m_extent.m_bottomright.GetRow(), m_rowsOrCols[n]));
                }
                else
                {
                    m_extent.m_topleft.SetCol(std::min(m_extent.m_topleft.GetCol(), m_rowsOrCols[n]));
                    m_extent.m_bottomright.SetCol(std::max(m_extent.m_bottomright.GetCol(), m_rowsOrCols[n]));
                }
            }

            fitExtentToShownCells();
        }

        /////////////////////////////////////////////////
        /// \brief Get the maximal needed enclosing box
        /// in terms of wxGridCellCoordinates.
        ///
        /// \return const wxGridCellsExtent&
        ///
        /////////////////////////////////////////////////
        const wxGridCellsExtent& getExtent() const
        {
            return m_extent;
        }

        /////////////////////////////////////////////////
        /// \brief Get the number of enclosed rows.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t getRows() const
        {
            return m_extent.m_bottomright.GetRow() - m_extent.m_topleft.GetRow()+1;
        }

        /////////////////////////////////////////////////
        /// \brief Get the number of enclosed columns.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t getCols() const
        {
            return m_extent.m_bottomright.GetCol() - m_extent.m_topleft.GetCol()+1;
        }

        /////////////////////////////////////////////////
        /// \brief Does this wxGridCellCoordsContainer
        /// contain the passed coordinates?
        ///
        /// \param cell const wxGridCellCoords&
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool contains(const wxGridCellCoords& cell) const
        {
            if (m_grid
                && (!m_grid->IsRowShown(cell.GetRow()) || !m_grid->IsColShown(cell.GetCol())))
                return false;

            if (isSparse())
            {
                for (size_t n = 0; n < m_array.size(); n++)
                {
                    if (m_array[n] == cell)
                        return true;
                }

                return false;
            }

            if (m_rowsOrCols.size())
                return std::find(m_rowsOrCols.begin(),
                                 m_rowsOrCols.end(),
                                 m_rowsSelected ? cell.GetRow() : cell.GetCol()) != m_rowsOrCols.end();

            return cell.GetRow() >= m_extent.m_topleft.GetRow()
                && cell.GetRow() <= m_extent.m_bottomright.GetRow()
                && cell.GetCol() >= m_extent.m_topleft.GetCol()
                && cell.GetCol() <= m_extent.m_bottomright.GetCol();
        }

        /////////////////////////////////////////////////
        /// \brief Does this wxGridCellCoordsContainer
        /// contain the passed coordinates?
        ///
        /// Same as before but with different argument
        /// types for convenience.
        ///
        /// \param row int
        /// \param col int
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool contains(int row, int col) const
        {
            return contains(wxGridCellCoords(row, col));
        }

        /////////////////////////////////////////////////
        /// \brief Returns true, if the contained
        /// coordinates actually form a contigious block.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isBlock() const
        {
            if (isSparse())
                return false;

            if (m_rowsOrCols.size())
            {
                if (m_rowsSelected && m_rowsOrCols.size() != getRows())
                    return false;

                if (!m_rowsSelected && m_rowsOrCols.size() != getCols())
                    return false;
            }

            return true;
        }

        /////////////////////////////////////////////////
        /// \brief Returns true, if the contained
        /// coordinates originated from complete columns.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool columnsSelected() const
        {
            return m_rowsOrCols.size() && !m_rowsSelected;
        }

        /////////////////////////////////////////////////
        /// \brief Returns true, if the contained
        /// coordinates form a sparse matrix.
        ///
        /// \return bool
        ///
        /////////////////////////////////////////////////
        bool isSparse() const
        {
            return m_array.size();
        }

        /////////////////////////////////////////////////
        /// \brief Get a reference to the internal
        /// wxGridCellCoordsArray (only available, if the
        /// contained coordinates form a sparse matrix).
        ///
        /// \return const wxGridCellCoordsArray&
        ///
        /////////////////////////////////////////////////
        const wxGridCellCoordsArray& getArray() const
        {
            return m_array;
        }

        /////////////////////////////////////////////////
        /// \brief Get the number of cells contained
        /// within this container.
        ///
        /// \return size_t
        ///
        /////////////////////////////////////////////////
        size_t size() const
        {
            if (m_array.size())
                return m_array.size();

            if (m_rowsOrCols.size())
                return m_rowsOrCols.size();

            return getRows() * getCols();
        }
};

#endif // GRIDCELLCOORDSCONTAINER_HPP

