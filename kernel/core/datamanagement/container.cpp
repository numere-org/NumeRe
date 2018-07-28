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

#include "container.hpp"

// Clean up function
void Container<class T>::freeStorage()
{
    // Ensure that the storage is not empty
    if (storage)
    {
        // the first dimension are always rows
        // free the memory
        for (size_t i = 0; i < rows; i++)
        {
            delete[] storage[i];
        }
        delete[] storage;

        // reset the members
        storage = nullptr;
        rows = 0;
        cols = 0;
    }
}


// Move-copy assignment constructor
Container<class T>::Container(Container<T>& _container)
{
    // Ensure that the storage is empty
    if (storage)
    {
        freeStorage();
    }

    // Copy the storage and the dimensions
    storage = _container.storage;
    rows = _container.rows;
    cols = _container.cols;

    // reset the passed container
    _container.storage = nullptr;
    _container.rows = 0;
    _container.cols = 0;
}

// Move-copy assignment constructor for external storage
Container<class T>::Container((T**)& extStorage, size_t _rows, size_t _cols)
{
    // Enusre that the storage is empty
    if (storage)
        freeStorage();

    // Copy the storage and the dimensons
    storage = extStorage;
    rows = _rows;
    cols = _cols;

    // reset the external storage
    extStorage = nullptr;
}

// read accessor
T Container<class T>::get(size_t row, size_t col) const
{
    // Ensure that the dimensions are matching
    if (row < rows && col < cols)
        return storage[row][col];

    // Otherwise return an empty class
    return T();
}

// write accessor
void Container<class T>::set(size_t row, size_t col, T val)
{
    // Ensure that the dimensions are matching
    if (row < rows && col < cols)
        storage[row][col] = val;
}

