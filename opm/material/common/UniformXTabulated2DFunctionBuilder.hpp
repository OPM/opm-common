// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2026 Equinor ASA

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
 * \copydoc Opm::UniformXTabulated2DFunctionBuilder
 */
#ifndef OPM_UNIFORM_X_TABULATED_2D_FUNCTION_BUILDER_HPP
#define OPM_UNIFORM_X_TABULATED_2D_FUNCTION_BUILDER_HPP

#include <opm/material/common/UniformXTabulated2DFunction.hpp>

#include <cassert>
#include <cstddef>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace Opm {

/*!
 * \brief Append-only builder for \c UniformXTabulated2DFunction.
 *
 * Sample points are appended column by column (one column per X position), in
 * ascending or descending Y order. Calling the rvalue-qualified \c build() flattens
 * the columns into a \c SparseTable and returns the immutable, evaluation-ready
 * \c UniformXTabulated2DFunction.
 */
template <class Scalar>
class UniformXTabulated2DFunctionBuilder
{
public:
    using TabulatedFunction = UniformXTabulated2DFunction<Scalar>;
    using SamplePoint = typename TabulatedFunction::SamplePoint;
    using InterpolationPolicy = typename TabulatedFunction::InterpolationPolicy;

    explicit UniformXTabulated2DFunctionBuilder(const InterpolationPolicy interpolationGuide = TabulatedFunction::Vertical)
        : interpolationGuide_(interpolationGuide)
    { }

    /*!
     * \brief Returns the value of the X coordinate of the sampling points.
     */
    Scalar xAt(std::size_t i) const
    { return xPos_[i]; }

    /*!
     * \brief Returns the value of the Y coordinate of a sampling point.
     */
    Scalar yAt(std::size_t i, std::size_t j) const
    { return std::get<1>(samples_[i][j]); }

    /*!
     * \brief Returns the value of a sampling point.
     */
    Scalar valueAt(std::size_t i, std::size_t j) const
    { return std::get<2>(samples_[i][j]); }

    /*!
     * \brief Returns the number of sampling points in X direction.
     */
    std::size_t numX() const
    { return xPos_.size(); }

    /*!
     * \brief Returns the minimum of the Y coordinate of the sampling points for a given column.
     */
    Scalar yMin(unsigned i) const
    { return std::get<1>(samples_.at(i).front()); }

    /*!
     * \brief Returns the maximum of the Y coordinate of the sampling points for a given column.
     */
    Scalar yMax(unsigned i) const
    { return std::get<1>(samples_.at(i).back()); }

    /*!
     * \brief Returns the minimum of the X coordinate of the sampling points.
     */
    Scalar xMin() const
    { return xPos_.front(); }

    /*!
     * \brief Returns the maximum of the X coordinate of the sampling points.
     */
    Scalar xMax() const
    { return xPos_.back(); }

    /*!
     * \brief Returns the number of sampling points in Y direction a given column.
     */
    std::size_t numY(unsigned i) const
    { return samples_.at(i).size(); }

    /*!
     * \brief Return the position on the x-axis of the i-th interval.
     */
    Scalar iToX(unsigned i) const
    {
        assert(i < numX());

        return xPos_.at(i);
    }

    /*!
     * \brief Set the x-position of a vertical line.
     *
     * Returns the i index of that line.
     */
    std::size_t appendXPos(Scalar nextX)
    {
        if (xPos_.empty() || xPos_.back() < nextX) {
            xPos_.push_back(nextX);
            yPos_.push_back(std::numeric_limits<Scalar>::lowest() / 2);
            samples_.push_back({});
            return xPos_.size() - 1;
        }
        else if (xPos_.front() > nextX) {
            // this is slow, but so what?
            xPos_.insert(xPos_.begin(), nextX);
            yPos_.insert(yPos_.begin(), std::numeric_limits<Scalar>::lowest() / 2);
            samples_.insert(samples_.begin(), std::vector<SamplePoint>());
            return 0;
        }
        throw std::invalid_argument("Sampling points should be specified either monotonically "
                                    "ascending or descending.");
    }

    /*!
     * \brief Append a sample point.
     *
     * Returns the i index of the new point within its line.
     */
    std::size_t appendSamplePoint(std::size_t i, Scalar y, Scalar value)
    {
        assert(i < numX());
        Scalar x = iToX(i);
        if (samples_[i].empty()) {
            samples_[i].emplace_back(x, y, value);
            yPos_[i] = y;
            return 0;
        }
        else if (std::get<1>(samples_[i].back()) < y) {
            samples_[i].emplace_back(x, y, value);
            if (interpolationGuide_ == InterpolationPolicy::RightExtreme) {
                yPos_[i] = y;
            }
            return samples_[i].size() - 1;
        }
        else if (std::get<1>(samples_[i].front()) > y) {
            // slow, but we still don't care...
            samples_[i].emplace(samples_[i].begin(), x, y, value);
            if (interpolationGuide_ == InterpolationPolicy::LeftExtreme) {
                yPos_[i] = y;
            }
            return 0;
        }

        throw std::invalid_argument("Sampling points must be specified in either monotonically "
                                    "ascending or descending order.");
    }

    /*!
     * \brief Finalize the table, moving the appended data into an immutable,
     *        SparseTable-backed \c UniformXTabulated2DFunction.
     *
     * This consumes the builder; a builder which has been built from must not be used
     * afterwards.
     */
    TabulatedFunction build() &&
    {
        std::vector<int> rowSizes(samples_.size());
        std::size_t totalSize = 0;
        for (std::size_t i = 0; i < samples_.size(); ++i) {
            rowSizes[i] = static_cast<int>(samples_[i].size());
            totalSize += samples_[i].size();
        }

        std::vector<SamplePoint> flatData;
        flatData.reserve(totalSize);
        for (auto& row : samples_) {
            for (auto& samplePoint : row) {
                flatData.push_back(std::move(samplePoint));
            }
        }

        std::vector<int> rowStarts(rowSizes.size() + 1, 0);
        std::partial_sum(rowSizes.begin(), rowSizes.end(), rowStarts.begin() + 1);

        SparseTable<SamplePoint> table(std::move(flatData), std::move(rowStarts));

        return TabulatedFunction(std::move(table),
                                 std::move(xPos_),
                                 std::move(yPos_),
                                 interpolationGuide_);
    }

private:
    // the vector which contains the values of the sample points
    // f(x_i, y_j). don't use this directly, use getSamplePoint(i,j)
    // instead!
    std::vector<std::vector<SamplePoint>> samples_;

    // the position of each vertical line on the x-axis
    std::vector<Scalar> xPos_;
    // the position on the y-axis of the guide point
    std::vector<Scalar> yPos_;
    InterpolationPolicy interpolationGuide_;
};

} // namespace Opm

#endif
