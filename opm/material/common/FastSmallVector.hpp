// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2019, 2025 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
/*!
 * \file
 *
 * \brief An implementation of vector/array based on small object optimization. It is intended
 *        to be used by the DynamicEvaluation for better efficiency.
 *
 * \copydoc Opm::FastSmallVector
 */
#ifndef OPM_FAST_SMALL_VECTOR_HPP
#define OPM_FAST_SMALL_VECTOR_HPP

#include <array>
#include <algorithm>
#include <vector>

namespace Opm {

/*!
 * \brief An implementation of vector/array based on small object optimization. It is intended
 *        to be used by the DynamicEvaluation for better efficiency.
 */
//! ValueType is the type of the data
//! N is the size of the buffer that willl be allocated during compilation time
template <typename ValueType, unsigned N>
class FastSmallVector
{
public:
    //! default constructor
    FastSmallVector()
        : size_(0)
    {
        dataPtr_ = smallBuf_.data();
    }

    //! constructor based on the number of the element
    explicit FastSmallVector(const size_t numElem)
    {
        init_(numElem);
    }

    //! constructor based on the number of the element, and all the elements
    //! will have the same value
    FastSmallVector(const size_t numElem, const ValueType value)
    {
        init_(numElem);

        std::fill(dataPtr_, dataPtr_ + size_, value);
    }

    //! copy constructor
    FastSmallVector(const FastSmallVector& other)
        : size_(0)
    {
        dataPtr_ = smallBuf_.data();

        (*this) = other;
    }

    //! move constructor
    FastSmallVector(FastSmallVector&& other)
        : size_(0)
    {
        dataPtr_ = smallBuf_.data();

        (*this) = std::move(other);
    }

    //! destructor
    ~FastSmallVector()
    {
    }


    //! move assignment
    FastSmallVector& operator=(FastSmallVector&& other)
    {
        size_ = other.size_;
        if (size_ <= N) {
            smallBuf_ = std::move(other.smallBuf_);
            dataPtr_ = smallBuf_.data();
        }
        else {
            data_ = std::move(other.data_);
            dataPtr_ = data_.data();
        }

        other.dataPtr_ = nullptr;
        other.size_ = 0;

        return (*this);
    }

    //! copy assignment
    FastSmallVector& operator=(const FastSmallVector& other)
    {
        size_ = other.size_;

        if (size_ <= N) {
            smallBuf_ = other.smallBuf_;
            dataPtr_ = smallBuf_.data();
        }
        else if (dataPtr_ != other.dataPtr_) {
            data_ = other.data_;
            dataPtr_ = data_.data();
        }

        return (*this);
    }

    //! access the idx th element
    ValueType& operator[](size_t idx)
    { return dataPtr_[idx]; }

    //! const access the idx th element
    const ValueType& operator[](size_t idx) const
    { return dataPtr_[idx]; }

    using size_type = typename std::vector<ValueType>::size_type;

    //! number of elements
    size_type size() const
    { return size_; }

    //! Iterator type is a plain pointer, so be warned there is no validity checking.
    using Iterator = ValueType*;

    //! ConstIterator type is a plain pointer, so be warned there is no validity checking.
    using ConstIterator = const ValueType*;

    //! To support range-for etc.
    ConstIterator begin() const
    { return dataPtr_; }

    //! To support range-for etc.
    ConstIterator end() const
    { return dataPtr_ + size_; }

    //! To support range-for etc.
    ConstIterator cbegin() const
    { return dataPtr_; }

    //! To support range-for etc.
    ConstIterator cend() const
    { return dataPtr_ + size_; }

    //! To support range-for etc.
    Iterator begin()
    { return dataPtr_; }

    //! To support range-for etc.
    Iterator end()
    { return dataPtr_ + size_; }

    size_type capacity()
    { return size_ <= N ? N : data_.capacity(); }

    void push_back(const ValueType& value)
    {
        if (size_ < N) {
            // Data is contained in smallBuf_
            smallBuf_[size_++] = value;
        } else if (size_ == N) {
            // Must switch from using smallBuf_ to using data_
            data_.reserve(N + 1);
            data_.assign(smallBuf_.begin(), smallBuf_.end());
            data_.push_back(value);
            ++size_;
            dataPtr_ = data_.data();
        } else {
            // Data is contained in data_
            data_.push_back(value);
            ++size_;
            dataPtr_ = data_.data();
        }
    }

private:
    void init_(size_t numElem)
    {
        size_ = numElem;

        if (size_ > N) {
            data_.resize(size_);
            dataPtr_ = data_.data();
        } else
            dataPtr_ = smallBuf_.data();
    }

    std::array<ValueType, N> smallBuf_{};
    std::vector<ValueType> data_;
    size_type size_;
    ValueType* dataPtr_;
};

} // namespace Opm

#endif // OPM_FAST_SMALL_VECTOR_HPP
