//===========================================================================
//
// File: SparseTable.hpp
//
// Created: Fri Apr 24 09:50:27 2009
//
// Author(s): Atgeirr F Rasmussen <atgeirr@sintef.no>
//
// $Date$
//
// $Revision$
//
//===========================================================================

/*
  Copyright 2009, 2010 SINTEF ICT, Applied Mathematics.
  Copyright 2009, 2010 Statoil ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef OPM_SPARSETABLE_HEADER
#define OPM_SPARSETABLE_HEADER

#include <vector>
#include <numeric>
#include <algorithm>
#include <ostream>
#include <type_traits>
#include <opm/common/ErrorMacros.hpp>
#include <opm/grid/utility/IteratorRange.hpp>

#include <opm/common/utility/gpuistl_if_available.hpp>

namespace Opm
{


    template<class>
inline constexpr bool always_false_v = false;

// Poison iterator is a helper class that will allow for compilation only when it is not used.
// Its intention is to be used so that we can have a SparseTable of GPU data, which requires the
// GPUBuffer intermediate storage type, which does not support iterators.
template<class T>
struct PoisonIterator {
    // iterator traits so it type-checks where an iterator is required
    using iterator_category = std::input_iterator_tag;
    using value_type        = T;
    using difference_type   = std::ptrdiff_t;
    using pointer           = T*;
    using reference         = T&;

    PoisonIterator() = default;

    // Dereference
    reference operator*() const {
        static_assert(always_false_v<T>, "PoisonIterator: operator*() is not allowed.");
        return *ptr_;
    }

    pointer operator->() const {
        static_assert(always_false_v<T>, "PoisonIterator: operator->() is not allowed.");
        return ptr_;
    }

    // Pre-increment
    PoisonIterator& operator++() {
        static_assert(always_false_v<T>, "PoisonIterator: operator++() is not allowed.");
        return *this;
    }

    // Post-increment
    PoisonIterator operator++(int) {
        static_assert(always_false_v<T>, "PoisonIterator: operator++(int) is not allowed.");
        return *this;
    }

    // Equality/inequality
    friend bool operator==(const PoisonIterator&, const PoisonIterator&) {
        static_assert(always_false_v<T>, "PoisonIterator: operator== is not allowed.");
        return true;
    }

    friend bool operator!=(const PoisonIterator&, const PoisonIterator&) {
        static_assert(always_false_v<T>, "PoisonIterator: operator!= is not allowed.");
        return false;
    }

private:
    T* ptr_ = nullptr; // placeholder to keep types consistent
};

    /// A SparseTable stores a table with rows of varying size
    /// as efficiently as possible.
    /// It is supposed to behave similarly to a vector of vectors.
    /// Its behaviour is similar to compressed row sparse matrices.
    template <typename T, template <typename, typename...> class Storage = std::vector>
    class SparseTable
    {
    public:
        /// Default constructor. Yields an empty SparseTable.
        SparseTable()
            : row_start_(1, 0)
        {
        }

        /// A constructor taking all the data for the table and row sizes.
        /// \param data_beg The start of the table data.
        /// \param data_end One-beyond-end of the table data.
        /// \param rowsize_beg The start of the row length data.
        /// \param rowsize_end One beyond the end of the row length data.
        template <typename DataIter, typename IntegerIter>
        SparseTable(DataIter data_beg, DataIter data_end,
                    IntegerIter rowsize_beg, IntegerIter rowsize_end)
            : data_(data_beg, data_end)
        {
	    setRowStartsFromSizes(rowsize_beg, rowsize_end);
        }

        SparseTable (Storage<T>&& data, Storage<int>&& row_starts)
            : data_(std::move(data))
            , row_start_(std::move(row_starts))
        {
            // removed for non-default template instantiations
            // because we cannot access the zero'th element if Storage is a GpuBuffer
            if constexpr (std::is_same_v<Storage<T>, std::vector<T>>) {
                OPM_ERROR_IF(row_start_.size() == 0 || row_start_[0] != 0,
                             "Invalid row_start array");
            }
        }


        /// Sets the table to contain the given data, organized into
	/// rows as indicated by the given row sizes.
        /// \param data_beg The start of the table data.
        /// \param data_end One-beyond-end of the table data.
        /// \param rowsize_beg The start of the row length data.
        /// \param rowsize_end One beyond the end of the row length data.
        template <typename DataIter, typename IntegerIter>
        void assign(DataIter data_beg, DataIter data_end,
                    IntegerIter rowsize_beg, IntegerIter rowsize_end)
        {
	    data_.assign(data_beg, data_end);
	    setRowStartsFromSizes(rowsize_beg, rowsize_end);
        }


        /// Request storage for table of given size.
        /// \param rowsize_beg Start of row size data.
        /// \param rowsize_end One beyond end of row size data.
        template <typename IntegerIter>
        void allocate(IntegerIter rowsize_beg, IntegerIter rowsize_end)
        {
            typedef typename Storage<T>::size_type sz_t;

            sz_t ndata = std::accumulate(rowsize_beg, rowsize_end, sz_t(0));
            data_.resize(ndata);
            setRowStartsFromSizes(rowsize_beg, rowsize_end);
        }


        /// Appends a row to the table.
        template <typename DataIter>
        void appendRow(DataIter row_beg, DataIter row_end)
        {
            data_.insert(data_.end(), row_beg, row_end);
            row_start_.push_back(data_.size());
        }

        /// True if the table contains no rows.
        OPM_HOST_DEVICE bool empty() const
        {
            return row_start_.size()==1;
        }

        /// Returns the number of rows in the table.
        OPM_HOST_DEVICE int size() const
        {
            return row_start_.size() - 1;
        }

        /// Allocate storage for table of expected size
        void reserve(int exptd_nrows, int exptd_ndata)
        {
            row_start_.reserve(exptd_nrows + 1);
            data_.reserve(exptd_ndata);
        }

        /// Swap contents for other SparseTable<T>
        void swap(SparseTable<T>& other)
        {
            row_start_.swap(other.row_start_);
            data_.swap(other.data_);
        }

        /// Returns the number of data elements.
        OPM_HOST_DEVICE int dataSize() const
        {
            return data_.size();
        }

        /// Returns the size of a table row.
        OPM_HOST_DEVICE int rowSize(int row) const
        {
#ifndef NDEBUG
            OPM_ERROR_IF(row < 0 || row >= size(),
                         "Row index " + std::to_string(row) + " is out of range");
#endif
            return row_start_[row + 1] - row_start_[row];
        }

        /// Makes the table empty().
        void clear()
        {
            data_.clear();
            row_start_.resize(1);
        }

        // Helper templates to select iterator range types only if (const_)iterator exists.
        // Default: PoisonIterator (for non-traversable types)
        template<class U, class = void>
        struct row_type_helper {
            using const_type = iterator_range<PoisonIterator<T>>;
            using mutable_type = mutable_iterator_range<PoisonIterator<T>>;
        };

        // If Storage has const_iterator, use it (e.g. std::vector)
        template<class U>
        struct row_type_helper<U, std::void_t<typename U::const_iterator>> {
            using const_type = iterator_range<typename U::const_iterator>;
            using mutable_type = mutable_iterator_range<typename U::iterator>;
        };

#if HAVE_CUDA
        // Specialization for GpuView: use its iterator
        template<typename TT>
        struct row_type_helper<gpuistl::GpuView<TT>> {
            using const_type = iterator_range<typename gpuistl::GpuView<TT>::iterator>;
            using mutable_type = mutable_iterator_range<typename gpuistl::GpuView<TT>::iterator>;
        };

        // Specialization for GpuBuffer: always PoisonIterator
        template<typename TT>
        struct row_type_helper<gpuistl::GpuBuffer<TT>> {
            using const_type = iterator_range<PoisonIterator<TT>>;
            using mutable_type = mutable_iterator_range<PoisonIterator<TT>>;
        };
#endif // HAVE_CUDA

        using row_type = typename row_type_helper<Storage<T>>::const_type;
        using mutable_row_type = typename row_type_helper<Storage<T>>::mutable_type;

        /// Returns a row of the table.
        OPM_HOST_DEVICE row_type operator[](int row) const
        {
            assert(row >= 0 && row < size());
            return row_type{data_.begin()+ row_start_[row],
                            data_.begin() + row_start_[row + 1]};
        }

        /// Returns a mutable row of the table.
        OPM_HOST_DEVICE mutable_row_type operator[](int row)
        {
            assert(row >= 0 && row < size());
            return mutable_row_type{data_.begin() + row_start_[row],
                                    data_.begin() + row_start_[row + 1]};
        }

        /// Iterator for iterating over the container as a whole,
        /// i.e. row by row.
        class Iterator
        {
        public:
            OPM_HOST_DEVICE Iterator(const SparseTable& table, const int begin_row_index)
                : table_(table)
                , row_index_(begin_row_index)
            {
            }
            OPM_HOST_DEVICE Iterator& operator++()
            {
                ++row_index_;
                return *this;
            }
            OPM_HOST_DEVICE row_type operator*() const
            {
                return table_[row_index_];
            }
            OPM_HOST_DEVICE bool operator==(const Iterator& other)
            {
                assert(&table_ == &other.table_);
                return row_index_ == other.row_index_;
            }
            OPM_HOST_DEVICE bool operator!=(const Iterator& other)
            {
                return !(*this == other);
            }
        private:
            const SparseTable& table_;
            int row_index_;
        };

        /// Iterator access.
        OPM_HOST_DEVICE Iterator begin() const
        {
            return Iterator(*this, 0);
        }
        OPM_HOST_DEVICE Iterator end() const
        {
            return Iterator(*this, size());
        }

        /// Equality.
        OPM_HOST_DEVICE bool operator==(const SparseTable& other) const
        {
            return data_ == other.data_ && row_start_ == other.row_start_;
        }

        template<class charT, class traits>
        void print(std::basic_ostream<charT, traits>& os) const
        {
            os << "Number of rows: " << size() << '\n';

            os << "Row starts = [";
            std::copy(row_start_.begin(), row_start_.end(),
                      std::ostream_iterator<int>(os, " "));
            os << "\b]\n";

            os << "Data values = [";
            std::copy(data_.begin(), data_.end(),
                      std::ostream_iterator<T>(os, " "));
            os << "\b]\n";
        }
        const T data(int i)const {
        	return data_[i];
        }

        // Get pointer to start of databuffer
        // This is useful for getting access to the buffer itself so we can copy to GPU easily
        const T* dataPtr() const
        {
            return data_.data();
        }

        // Access the data being stored directly (for instance used for copying to GPU)
        const Storage<T>& dataStorage() const
        {
            return data_;
        }

        // Access indices of where all rows start
        const Storage<int>& rowStarts() const
        {
            return row_start_;
        }
    private:
        Storage<T> data_;
        // Like in the compressed row sparse matrix format,
        // row_start_.size() is equal to the number of rows + 1.
        Storage<int> row_start_;

	template <class IntegerIter>
	void setRowStartsFromSizes(IntegerIter rowsize_beg, IntegerIter rowsize_end)
	{
#ifndef NDEBUG
            // Check that all row sizes given are nonnegative.
            for (auto it = rowsize_beg; it != rowsize_end; ++it) {
                if (*it < 0) {
                    OPM_THROW(std::runtime_error, "Negative row size given.");
                }
            }
#endif
            // Since we do not store the row sizes, but cumulative row sizes,
            // we have to create the cumulative ones.
            int num_rows = rowsize_end - rowsize_beg;
            row_start_.resize(num_rows + 1);
            row_start_[0] = 0;
            std::partial_sum(rowsize_beg, rowsize_end, row_start_.begin() + 1);
            // Check that data_ and row_start_ match.
            if (int(data_.size()) != row_start_.back()) {
                OPM_THROW(std::runtime_error, "End of row start indices different from data size.");
            }

	}
    };

} // namespace Opm

#if HAVE_CUDA
namespace Opm::gpuistl {

template <class T>
auto copy_to_gpu(const SparseTable<T>& cpu_table)
{
    return SparseTable<T, GpuBuffer>(
        GpuBuffer<T>(cpu_table.dataStorage()),
        GpuBuffer<int>(cpu_table.rowStarts())
    );
}

template <class T>
auto make_view(SparseTable<T, GpuBuffer>& buffer_table)
{
    return SparseTable<T, GpuView>(
        GpuView<T>(const_cast<T*>(buffer_table.dataStorage().data()),
                   buffer_table.dataStorage().size()),
        GpuView<int>(const_cast<int*>(buffer_table.rowStarts().data()),
                     buffer_table.rowStarts().size())
    );
}

} // namespace Opm::gpuistl
#endif // HAVE_CUDA

#endif // OPM_SPARSETABLE_HEADER
