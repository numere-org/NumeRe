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


#ifndef CONTAINER_HPP
#define CONTAINER_HPP

// required for size_t
#include <cstddef>
#include <vector>

#include "../ui/error.hpp"

using namespace std;

// declare the namespace
namespace NumeRe
{
	// Template class for a generic container, which will
	// handle the copying of a two-dimensional pointer
	// Will take ownership of the passed pointer as long
	// as it exists and is not copied
	// During the copy process, the storage pointer will
	// move to the copied object
	template <class T>
	class Container
	{
		private:
			// Internal storage including the dimensions
			// variables are declared as mutable to allow
			// to be const copy constructed although the
			// ownership of the storage will move from
			// one to another container
			mutable T** storage;
			mutable size_t rows;
			mutable size_t cols;

			// Clean up function
			void freeStorage()
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

			// preparation function
			void prepareStorage(size_t _rows, size_t _cols)
			{
			    // Clean the storage
			    if (storage)
                    freeStorage();

                // Create a new storage
                storage = new T*[_rows];
                for (size_t i = 0; i < _rows; i++)
                    storage[i] = new T[_cols];
                rows = _rows;
                cols = _cols;
			}

		public:
			// default constructor
			Container() : storage(nullptr), rows(0), cols(0) {}

			// default preparation constructor
			Container(size_t _rows, size_t _cols) : Container()
			{
			    prepareStorage(_rows, _cols);
			}

			// Copy constructor, will move the contents
			Container(const Container<T>& _container) : Container()
			{
			    // Avoid copying empty storage
			    if (!_container.storage)
                    return;

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

			// special constructor for an external pointer
			Container(T** (&extStorage), size_t _rows, size_t _cols) : Container()
			{
			    // avoid copying empty data fields
			    if(!extStorage)
                    return;

				// Copy the storage and the dimensons
				storage = extStorage;
				rows = _rows;
				cols = _cols;

				// reset the external storage
				extStorage = nullptr;
			}

			// special vector constructor
			Container(const vector<vector<T> >& extStorage) : Container()
			{
			    // Prepare the storage
			    prepareStorage(extStorage.size(), extStorage[0].size());

			    // Copy the contents
			    for (size_t i = 0; i < extStorage.size(); i++)
                {
                    for (size_t j = 0; j < extStorage[i].size(); j++)
                        this->set(i, j, extStorage[i][j]);
                }
			}

			// Destuctor: will free memory, if available
			~Container()
			{
				freeStorage();
			}

			// Assignment operator
			Container<T>& operator=(const Container<T>& _container)
			{
			    // Avoid copying empty storage
			    if (!_container.storage)
                    return *this;

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

				return *this;
			}


			// Accessors
			// read accessor
			T& get(size_t row, size_t col)
			{
				// Ensure that the dimensions are matching
				if (storage && row < rows && col < cols)
					return storage[row][col];

				// Otherwise throw an error
				throw SyntaxError(SyntaxError::INVALID_DATA_ACCESS, "", SyntaxError::invalid_position);
			}

			// write accessor
			void set(size_t row, size_t col, T val)
			{
				// Ensure that the dimensions are matching
				if (storage && row < rows && col < cols)
					storage[row][col] = val;
			}

			// dimensions
			size_t getRows() const
			{
				return rows;
			}
			size_t getCols() const
			{
				return cols;
			}
	};

} // namespace NumeRe

#endif // CONTAINER_HPP

