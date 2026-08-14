/*
  Copyright 2026 Equinor ASA.

  This file is part of the Open Porous Media Project (OPM).

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

#ifndef OPM_OUTPUT_DATA_REGION_VARIABLE_VIEW_HPP
#define OPM_OUTPUT_DATA_REGION_VARIABLE_VIEW_HPP

#include <opm/output/data/RegionsetVariableDescriptor.hpp>

#include <cassert>
#include <cstddef>
#include <functional>
#include <span>
#include <stdexcept>
#include <type_traits>

/// \file Component that gives a view into a sequence of (numerical) values
/// keyed by region sets and region indices.

namespace Opm::data {

    /// Linear sequence of read-only or read/write values associated to a
    /// collection of region sets and regions within each region set.
    ///
    /// The value range is expected to have one scalar element for each
    /// region in each region set.
    ///
    /// \tparam T Element type of the underlying value range.  If \c T is
    /// const-qualified, the view provides read-only access.  Otherwise, the
    /// view provides read/write access.
    template <typename T>
    class RegionVariableView
    {
    public:
        /// Constructor.
        ///
        /// \param[in] values Range of values for a single region level variable.
        /// The range must have one scalar value for each region in each region
        /// set known to \p variableDescriptor.  Expected to outlive the view object.
        ///
        /// \param[in] variableDescriptor Collection of region sets and
        /// associated region indices per region set.  The value range
        /// \p values must have one scalar value for each region in each
        /// region set known to \p variableDescriptor.  If not, this constructor
        /// will throw an exception of type \code std::logic_error \endcode.
        explicit RegionVariableView(std::span<T>                       values,
                                    const RegionsetVariableDescriptor& variableDescriptor)
            : values_             { values }
            , variableDescriptor_ { std::cref(variableDescriptor) }
        {
            const auto numElements = values.size();

            if (numElements != this->variableDescriptor_.get().numVariableSlots()) {
                throw std::logic_error {
                    "Element range does not match expected number of values"
                };
            }
        }

        /// Read/write access to an element in the value range.
        ///
        /// Inaccessible if element type \c T is const-qualified.
        ///
        /// \param[in] regSetID Region set ID.  Must be in the range 0..#S-1
        /// with S being the number of region sets known to the variable
        /// descriptor from which view was constructed.
        ///
        /// \param[in] region Region index within the specific \p regSetID.
        ///
        /// \return Mutable element within the value range.
        T& element(const std::size_t regSetID, const std::size_t region)
        requires (! std::is_const_v<T>)
        {
            return this->values_[ this->index(regSetID, region) ];
        }

        /// Read-only access to an element in the value range.
        ///
        /// \param[in] regSetID Region set ID.  Must be in the range 0..#S-1
        /// with S being the number of region sets known to the variable
        /// descriptor from which view was constructed.
        ///
        /// \param[in] region Region index within the specific \p regSetID.
        ///
        /// \return Read-only element within the value range.
        std::conditional_t<std::is_arithmetic_v<std::remove_cvref_t<T>>,
                           std::remove_cvref_t<T>,
                           const std::remove_cvref_t<T>&>
        element(const std::size_t regSetID, const std::size_t region) const
        {
            return this->values_[ this->index(regSetID, region) ];
        }

    private:
        /// Convenience type alias for a wrapped variable descriptor.
        using VarDesc = std::reference_wrapper<const RegionsetVariableDescriptor>;

        /// Value range.
        std::span<T> values_;

        /// Collection of region sets and associated region IDs.
        VarDesc variableDescriptor_;

        /// Translate a pair of region set and region indices to a linear index.
        ///
        /// \param[in] regSetID Region set ID.  Must be in the range 0..#S-1
        /// with S being the number of region sets known to the variable
        /// descriptor from which view was constructed.
        ///
        /// \param[in] region Region index within the specific \p regSetID.
        ///
        /// \return Linear index within value range corresponding to the
        /// index pair (\p regSetID, \p region).
        std::size_t index(const std::size_t regSetID, const std::size_t region) const
        {
            const auto& d = this->variableDescriptor_.get();

            assert (regSetID < d.numRegionSets());

            const auto begin = d.startIndex(regSetID);
#ifndef NDEBUG
            const auto end = (regSetID + 1 < d.numRegionSets())
                ? d.startIndex(regSetID + 1)
                : d.numVariableSlots();

            assert (begin + region < end);
#endif
            return begin + region;
        }
    };

} // namespace Opm::data

#endif // OPM_OUTPUT_DATA_REGION_VARIABLE_VIEW_HPP
