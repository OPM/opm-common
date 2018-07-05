/*
  Copyright (c) 2018 Statoil ASA

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

#ifndef OPM_WINDOWED_ARRAY_HPP
#define OPM_WINDOWED_ARRAY_HPP

#include <cassert>
#include <iterator>
#include <type_traits>
#include <vector>

#include <boost/range/iterator_range.hpp>

namespace Opm { namespace RestartIO { namespace Helpers {

    template <typename T>
    class WindowedArray
    {
    public:
        using WriteWindow = boost::iterator_range<
            typename std::vector<T>::iterator>;

        using ReadWindow = boost::iterator_range<
            typename std::vector<T>::const_iterator>;

        using Idx = typename std::vector<T>::size_type;

        struct NumWindows { Idx value; };
        struct WindowSize { Idx value; };

        explicit WindowedArray(const NumWindows n, const WindowSize sz)
            : x_         (n.value * sz.value)
            , windowSize_(sz.value)
        {}

        Idx numWindows() const
        {
            return this->x_.size() / this->windowSize_;
        }

        Idx windowSize() const
        {
            return this->windowSize_;
        }

        WriteWindow operator[](const Idx window)
        {
            assert ((window < this->numWindows()) &&
                    "Window ID Out of Bounds");

            auto b = std::begin(this->x_) + window*this->windowSize_;
            auto e = b + this->windowSize_;

            return { b, e };
        }

        ReadWindow operator[](const Idx window) const
        {
            assert ((window < this->numWindows()) &&
                    "Window ID Out of Bounds");

            auto b = std::begin(this->x_) + window*this->windowSize_;
            auto e = b + this->windowSize_;

            return { b, e };
        }

        const std::vector<T>& data() const
        {
            return this->x_;
        }

        std::vector<T> getDataDestructively()
        {
            return std::move(this->x_);
        }

    private:
        std::vector<T> x_;

        Idx windowSize_;
    };

    template <typename T>
    class WindowedMatrix
    {
    private:
        using NumWindows  = typename WindowedArray<T>::NumWindows;

    public:
        using WriteWindow = typename WindowedArray<T>::WriteWindow;
        using ReadWindow  = typename WindowedArray<T>::ReadWindow;
        using WindowSize  = typename WindowedArray<T>::WindowSize;
        using Idx         = typename WindowedArray<T>::Idx;

        struct NumRows { Idx value; };
        struct NumCols { Idx value; };

        explicit WindowedMatrix(const NumRows& nRows,
                                const NumCols& nCols,
                                const WindowSize& sz)
            : data_   (NumWindows{ nRows.value * nCols.value }, sz)
            , numCols_(nCols.value)
        {}

        Idx numCols() const
        {
            return this->numCols_;
        }

        Idx numRows() const
        {
            return this->data_.numWindows() / this->numCols();
        }

        Idx windowSize() const
        {
            return this->data_.windowSize();
        }

        WriteWindow operator()(const Idx row, const Idx col)
        {
            return this->data_[ this->i(row, col) ];
        }

        ReadWindow operator()(const Idx row, const Idx col) const
        {
            return this->data_[ this->i(row, col) ];
        }

        auto data() const
            -> decltype(std::declval<const WindowedArray<T>>().data())
        {
            return this->data_.data();
        }

        auto getDataDestructively()
            -> decltype(std::declval<WindowedArray<T>>()
                        .getDataDestructively())
        {
            return this->data_.getDataDestructively();
        }

    private:
        WindowedArray<T> data_;

        Idx numCols_;

        /// Row major (C) order.
        Idx i(const Idx row, const Idx col) const
        {
            return row*this->numCols() + col;
        }
    };

}}} // Opm::RestartIO::Helpers

#endif // OPM_WINDOW_ARRAY_HPP
