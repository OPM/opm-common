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
#ifndef OPM_ITERATOR_RANGE_HEADER
#define OPM_ITERATOR_RANGE_HEADER

#include <iterator>
#include <type_traits>

#include <opm/common/utility/gpuDecorators.hpp>

namespace Opm {

template <class DataType>
struct iterator_range_pod {
    OPM_HOST_DEVICE iterator_range_pod(const DataType* begin, const DataType* end) : begin_(begin), end_(end) {}
    iterator_range_pod() = default;

    OPM_HOST_DEVICE size_t size() const { return std::distance(begin_,end_); }
    OPM_HOST_DEVICE bool empty() const { return begin_ == end_; }
    OPM_HOST_DEVICE bool operator==(const iterator_range_pod<DataType>& rhs) const
    { return (begin_ == rhs.begin_) && (end_ == rhs.end_); }

    OPM_HOST_DEVICE const DataType& operator[](int idx) const { return begin_[idx]; }

    OPM_HOST_DEVICE const DataType* begin() const { return begin_; }
    OPM_HOST_DEVICE const DataType* end() const { return end_; }

protected:
    const DataType* begin_;
    const DataType* end_;

};

template <class Iter>
struct iterator_range {
    OPM_HOST_DEVICE iterator_range(Iter begin, Iter end) : begin_(begin), end_(end) {}
    iterator_range() = default;

    OPM_HOST_DEVICE size_t size() const { return std::distance(begin_,end_); }
    OPM_HOST_DEVICE bool empty() const { return begin_ == end_; }
    OPM_HOST_DEVICE bool operator==(const iterator_range<Iter>& rhs) const
    { return (begin_ == rhs.begin_) && (end_ == rhs.end_); }

    OPM_HOST_DEVICE const typename Iter::value_type& operator[](int idx) const
    { return *(begin_+ idx); }

    OPM_HOST_DEVICE Iter begin() const { return begin_; }
    OPM_HOST_DEVICE Iter end() const { return end_; }

protected:
    Iter begin_, end_;
};

template<typename Iter>
struct mutable_iterator_range {
    OPM_HOST_DEVICE mutable_iterator_range(Iter begin, Iter end) : begin_(begin), end_(end) {}
    mutable_iterator_range() = default;

    OPM_HOST_DEVICE size_t size() const { return std::distance(begin_,end_); }
    OPM_HOST_DEVICE bool empty() const { return begin_ == end_; }
    OPM_HOST_DEVICE bool operator==(const Iter& rhs) const
    { return (begin_ == rhs.begin_) && (end_ == rhs.end_); }

    OPM_HOST_DEVICE typename Iter::value_type& operator[](int idx)
    { return begin_[idx]; }

    OPM_HOST_DEVICE Iter begin() const { return begin_; }
    OPM_HOST_DEVICE Iter end() const { return end_; }

protected:
    Iter begin_, end_;
};

}

#endif // OPM_ITERATOR_RANGE_HEADER
