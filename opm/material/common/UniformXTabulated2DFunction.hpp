// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
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
 * \copydoc Opm::UniformXTabulated2DFunction
 */
#ifndef OPM_UNIFORM_X_TABULATED_2D_FUNCTION_HPP
#define OPM_UNIFORM_X_TABULATED_2D_FUNCTION_HPP

#include <opm/common/Exceptions.hpp>
#include <opm/common/utility/SparseTable.hpp>
#include <opm/common/utility/gpuDecorators.hpp>

#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/common/Valgrind.hpp>

#include <cassert>
#include <cmath>
#include <cstddef>
#include <iosfwd>
#include <limits>
#include <type_traits>
#include <vector>

namespace Opm {

template <class Scalar>
class UniformXTabulated2DFunctionBuilder;

/*!
 * \brief Implements a scalar function that depends on two variables and which is sampled
 *        uniformly in the X direction, but non-uniformly on the Y axis-
 *
 * "Uniform on the X-axis" means that all Y sampling points must be located along a line
 * for this value. This class can be used when the sampling points are calculated at run
 * time.
 *
 * This class is immutable: sample points are appended and finalized via
 * \c UniformXTabulated2DFunctionBuilder, whose \c build() method returns the
 * evaluation-ready object constructed here.
 */
template <class Scalar, template <typename, typename...> class Storage = std::vector>
class UniformXTabulated2DFunction
{
public:
    /*!
     * \brief One tabulated sample point: (x, y) position and the function value there.
     *
     * \note x duplicates xPos_[i] for this point's column. X-axis segment lookups
     *       (xSegmentIndex, xToAlpha, etc.) go through xPos_, not through this field -
     *       it is only read back via operator==.
     */
    struct SamplePoint {
        Scalar x;
        Scalar y;
        Scalar value;

        SamplePoint(Scalar x_, Scalar y_, Scalar value_) : x(x_), y(y_), value(value_) {}
        bool operator==(const SamplePoint& other) const = default;
    };

    static_assert(std::is_trivially_copyable_v<SamplePoint>,
        "SamplePoint must stay trivially copyable, because it needs to support being "
        "transferred to the GPU via memcpy in SparseTable.");

    /*!
     * \brief Indicates how interpolation will be performed.
     *
     * Normal interpolation is done by interpolating vertically between lines of sample
     * points, whereas LeftExtreme or RightExtreme implies guided interpolation, where
     * interpolation is done parallel to a guide line. With LeftExtreme the lowest Y
     * values will be used for the guide, and the guide line slope extends unchanged to
     * infinity. With RightExtreme, the highest Y values are used, and the slope
     * decreases linearly down to 0 (normal interpolation) for y <= 0.
     */
    enum InterpolationPolicy {
        LeftExtreme,
        RightExtreme,
        Vertical
    };

    explicit UniformXTabulated2DFunction(const InterpolationPolicy interpolationGuide = Vertical)
        : interpolationGuide_(interpolationGuide)
    { }

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
     * \brief Returns the value of the X coordinate of the sampling points.
     */
    Scalar xAt(std::size_t i) const
    { return xPos_[i]; }

    /*!
     * \brief Returns the value of the Y coordinate of a sampling point.
     */
    Scalar yAt(std::size_t i, std::size_t j) const
    { return samples_[static_cast<int>(i)][static_cast<int>(j)].y; }

    /*!
     * \brief Returns the value of a sampling point.
     */
    Scalar valueAt(std::size_t i, std::size_t j) const
    { return samples_[static_cast<int>(i)][static_cast<int>(j)].value; }

    /*!
     * \brief Returns the number of sampling points in X direction.
     */
    std::size_t numX() const
    { return xPos_.size(); }

    /*!
     * \brief Returns the minimum of the Y coordinate of the sampling points for a given column.
     */
    Scalar yMin(unsigned i) const
    { return samples_[static_cast<int>(i)].front().y; }

    /*!
     * \brief Returns the maximum of the Y coordinate of the sampling points for a given column.
     */
    Scalar yMax(unsigned i) const
    { return samples_[static_cast<int>(i)].back().y; }

    /*!
     * \brief Returns the number of sampling points in Y direction a given column.
     */
    std::size_t numY(unsigned i) const
    { return static_cast<std::size_t>(samples_.rowSize(static_cast<int>(i))); }

    /*!
     * \brief Return the position on the x-axis of the i-th interval.
     */
    Scalar iToX(unsigned i) const
    {
        assert(i < numX());

        return xPos_.at(i);
    }

    const SparseTable<SamplePoint, Storage>& samples() const
    {
        return samples_;
    }

    const Storage<Scalar>& xPos() const
    {
        return xPos_;
    }

    const Storage<Scalar>& yPos() const
    {
        return yPos_;
    }

    InterpolationPolicy interpolationGuide() const
    {
        return interpolationGuide_;
    }

    /*!
     * \brief Return the position on the y-axis of the j-th interval.
      */
    Scalar jToY(unsigned i, unsigned j) const
    {
        assert(i < numX());
        assert(std::size_t(j) < samples_[static_cast<int>(i)].size());

        return samples_[static_cast<int>(i)][static_cast<int>(j)].y;
    }

    /*!
     * \brief Return the interval index of a given position on the x-axis.
     */
    OPM_HOST_DEVICE template <class Evaluation>
    unsigned xSegmentIndex(const Evaluation& x,
                           [[maybe_unused]] bool extrapolate = false) const
    {
        assert(extrapolate || (xMin() <= x && x <= xMax()));

        // we need at least two sampling points!
        assert(xPos_.size() >= 2);

        if (x <= xPos_[1])
            return 0;
        else if (x >= xPos_[xPos_.size() - 2])
            return xPos_.size() - 2;
        else {
            assert(xPos_.size() >= 3);

            // bisection
            unsigned lowerIdx = 1;
            unsigned upperIdx = xPos_.size() - 2;
            while (lowerIdx + 1 < upperIdx) {
                unsigned pivotIdx = (lowerIdx + upperIdx) / 2;
                if (x < xPos_[pivotIdx])
                    upperIdx = pivotIdx;
                else
                    lowerIdx = pivotIdx;
            }

            return lowerIdx;
        }
    }

    /*!
     * \brief Return the relative position of an x value in an intervall
     *
     * The returned value can be larger than 1 or smaller than zero if it is outside of
     * the range of the segment. In particular this happens for the extrapolation case.
     */
    OPM_HOST_DEVICE template <class Evaluation>
    Evaluation xToAlpha(const Evaluation& x, unsigned segmentIdx) const
    {
        Scalar x1 = xPos_[segmentIdx];
        Scalar x2 = xPos_[segmentIdx + 1];
        return (x - x1)/(x2 - x1);
    }

    /*!
     * \brief Return the interval index of a given position on the y-axis.
     */
    OPM_HOST_DEVICE template <class Evaluation>
    unsigned ySegmentIndex(const Evaluation& y, unsigned xSampleIdx,
                           [[maybe_unused]] bool extrapolate = false) const
    {
        assert(xSampleIdx < numX());
        const auto colSamplePoints = samples_[static_cast<int>(xSampleIdx)];

        assert(colSamplePoints.size() >= 2);
        assert(extrapolate || (yMin(xSampleIdx) <= y && y <= yMax(xSampleIdx)));

        if (y <= colSamplePoints[1].y)
            return 0;
        else if (y >= colSamplePoints[colSamplePoints.size() - 2].y)
            return colSamplePoints.size() - 2;
        else {
            assert(colSamplePoints.size() >= 3);

            // bisection
            unsigned lowerIdx = 1;
            unsigned upperIdx = colSamplePoints.size() - 2;
            while (lowerIdx + 1 < upperIdx) {
                unsigned pivotIdx = (lowerIdx + upperIdx) / 2;
                if (y < colSamplePoints[pivotIdx].y)
                    upperIdx = pivotIdx;
                else
                    lowerIdx = pivotIdx;
            }

            return lowerIdx;
        }
    }

    /*!
     * \brief Return the relative position of an y value in an interval
     *
     * The returned value can be larger than 1 or smaller than zero if it is outside of
     * the range of the segment. In particular this happens for the extrapolation case.
     */
    OPM_HOST_DEVICE template <class Evaluation>
    Evaluation yToBeta(const Evaluation& y, unsigned xSampleIdx, unsigned ySegmentIdx) const
    {
        assert(xSampleIdx < numX());
        assert(ySegmentIdx < numY(xSampleIdx) - 1);

        const auto colSamplePoints = samples_[static_cast<int>(xSampleIdx)];

        Scalar y1 = colSamplePoints[ySegmentIdx].y;
        Scalar y2 = colSamplePoints[ySegmentIdx + 1].y;

        return (y - y1)/(y2 - y1);
    }

    /*!
     * \brief Returns true iff a coordinate lies in the tabulated range
     */
    OPM_HOST_DEVICE template <class Evaluation>
    bool applies(const Evaluation& x, const Evaluation& y) const
    {
        if (x < xMin() || xMax() < x)
            return false;

        unsigned i = xSegmentIndex(x, /*extrapolate=*/false);
        Scalar alpha = xToAlpha(decay<Scalar>(x), i);

        const auto col1SamplePoints = samples_[static_cast<int>(i)];
        const auto col2SamplePoints = samples_[static_cast<int>(i) + 1];

        Scalar minY =
                alpha*col1SamplePoints.front().y +
                (1 - alpha)*col2SamplePoints.front().y;

        Scalar maxY =
                alpha*col1SamplePoints.back().y +
                (1 - alpha)*col2SamplePoints.back().y;

        return minY <= y && y <= maxY;
    }
    /*!
     * \brief Evaluate the function at a given (x,y) position.
     *
     * If this method is called for a value outside of the tabulated
     * range, a \c Opm::NumericalIssue exception is thrown.
     */
    OPM_HOST_DEVICE template <class Evaluation>
    Evaluation eval(const Evaluation& x, const Evaluation& y, bool extrapolate=false) const
    {
        Evaluation alpha, beta1, beta2;
        unsigned i, j1, j2;
        findPoints(i, j1, j2, alpha, beta1, beta2, x, y, extrapolate);
        return eval(i, j1, j2, alpha, beta1, beta2);
    }

    OPM_HOST_DEVICE template <class Evaluation>
    void findPoints(unsigned& i,
                    unsigned& j1,
                    unsigned& j2,
                    Evaluation& alpha,
                    Evaluation& beta1,
                    Evaluation& beta2,
                    const Evaluation& x,
                    const Evaluation& y,
                    bool extrapolate) const
    {
#ifndef NDEBUG
#if !OPM_IS_INSIDE_DEVICE_FUNCTION
        if (!extrapolate && !applies(x, y)) {
            if constexpr (std::is_floating_point_v<Evaluation>) {
                throw NumericalProblem("Attempt to get undefined table value (" +
                                       std::to_string(x) + ", " +
                                       std::to_string(y) + ")");
            } else {
                throw NumericalProblem("Attempt to get undefined table value (" +
                                       std::to_string(x.value()) + ", " +
                                       std::to_string(y.value()) + ")");
            }
        };
#endif
#endif
        // bi-linear interpolation: first, calculate the x and y indices in the lookup
        // table ...
        i = xSegmentIndex(x, extrapolate);
        alpha = xToAlpha(x, i);
        // The 'shift' is used to shift the points used to interpolate within
        // the (i) and (i+1) sets of sample points, so that when approaching
        // the boundary of the domain given by the samples, one gets the same
        // value as one would get by interpolating along the boundary curve
        // itself.
        Evaluation shift = 0.0;
        if (interpolationGuide_ == InterpolationPolicy::Vertical) {
            // Shift is zero, no need to reset it.
        } else {
            // find upper and lower y value
            if (interpolationGuide_ == InterpolationPolicy::LeftExtreme) {
                // The domain is above the boundary curve, up to y = infinity.
                // The shift is therefore the same for all values of y.
                shift = yPos_[i+1] - yPos_[i];
            } else {
                assert(interpolationGuide_ == InterpolationPolicy::RightExtreme);
                // The domain is below the boundary curve, down to y = 0.
                // The shift is therefore no longer the the same for all
                // values of y, since at y = 0 the shift must be zero.
                // The shift is computed by linear interpolation between
                // the maximal value at the domain boundary curve, and zero.
                shift = yPos_[i+1] - yPos_[i];
                auto yEnd = yPos_[i]*(1.0 - alpha) + yPos_[i+1]*alpha;
                if (yEnd > 0.) {
                    shift = shift * y / yEnd;
                } else {
                    shift = 0.;
                }
            }
        }
        auto yLower =  y - alpha*shift;
        auto yUpper =  y + (1-alpha)*shift;

        j1 = ySegmentIndex(yLower, i, extrapolate);
        j2 = ySegmentIndex(yUpper, i + 1, extrapolate);
        beta1 = yToBeta(yLower, i, j1);
        beta2 = yToBeta(yUpper, i + 1, j2);
    }

    OPM_HOST_DEVICE template <class Evaluation>
    Evaluation eval(const unsigned& i, const unsigned& j1, const unsigned& j2, const Evaluation& alpha,const Evaluation& beta1,const Evaluation& beta2) const
    {
        // evaluate the two function values for the same y value ...
        const Evaluation& s1 = valueAt(i, j1)*(1.0 - beta1) + valueAt(i, j1 + 1)*beta1;
        const Evaluation& s2 = valueAt(i + 1, j2)*(1.0 - beta2) + valueAt(i + 1, j2 + 1)*beta2;

        Valgrind::CheckDefined(s1);
        Valgrind::CheckDefined(s2);

        // ... and combine them using the x position
        const Evaluation& result = s1*(1.0 - alpha) + s2*alpha;
        Valgrind::CheckDefined(result);

        return result;
    }

    /*!
     * \brief Print the table for debugging purposes.
     *
     * It will produce the data in CSV format on stdout, so that it can be visualized
     * using e.g. gnuplot.
     */
    void print(std::ostream& os) const;

    bool operator==(const UniformXTabulated2DFunction<Scalar, Storage>& data) const {
        return this->xPos() == data.xPos() &&
               this->yPos() == data.yPos() &&
               this->samples() == data.samples() &&
               this->interpolationGuide() == data.interpolationGuide();
    }

private:
    friend class UniformXTabulated2DFunctionBuilder<Scalar>;

    UniformXTabulated2DFunction(SparseTable<SamplePoint, Storage>&& samples,
                                Storage<Scalar>&& xPos,
                                Storage<Scalar>&& yPos,
                                InterpolationPolicy interpolationGuide)
        : samples_(std::move(samples))
        , xPos_(std::move(xPos))
        , yPos_(std::move(yPos))
        , interpolationGuide_(interpolationGuide)
    { }

    // the table which contains the values of the sample points f(x_i, y_j), stored
    // row-major (one row per x position). Don't use this directly, use
    // getSamplePoint(i,j) instead!
    SparseTable<SamplePoint, Storage> samples_;

    // the position of each vertical line on the x-axis
    Storage<Scalar> xPos_;
    // the position on the y-axis of the guide point
    Storage<Scalar> yPos_;
    InterpolationPolicy interpolationGuide_;
};
} // namespace Opm

#endif
