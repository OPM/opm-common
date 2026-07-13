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

#ifndef OPM_OUTPUT_DATA_REGION_VARIABLE_VALUES_HPP
#define OPM_OUTPUT_DATA_REGION_VARIABLE_VALUES_HPP

#include <opm/output/data/RegionsetVariableDescriptor.hpp>
#include <opm/output/data/RegionVariableView.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

/// \file Component to collect per-region values of region level summary
/// quantities.

namespace Opm::data {

    /// Numerical values for set of region level summary variables defined
    /// over a collection of region sets.
    class RegionVariableValues
    {
    public:
        /// Virtual destructor to support inheritance.
        virtual ~RegionVariableValues() = default;

        /// Create a deep copy of this object.
        ///
        /// Intended for when an object holds a pointer to a base class and
        /// needs to make a copy of the derived class object.  Derived classes
        /// should override this function to return a copy of the derived class object.
        ///
        /// \return A deep copy of this object.
        virtual std::unique_ptr<RegionVariableValues> clone() const;

        /// Define variable collection.
        ///
        /// \param[in] descr Collection of region sets, possibly including
        /// the FIELD.  The value collection retains a reference to this
        /// descriptor, whence the lifetime of \p descr must exceed the
        /// lifetime of \code *this \endcode.  The value collection will
        /// have one value for each variable for each distinct region in all
        /// region sets known by \p descr.
        ///
        /// \param[in] is_cumulative Whether or not a particular variable
        /// represents a cumulative quantity.  The value collection will
        /// define \code is_cumulative.size() \endcode distinct variables.
        void defineVariables(const RegionsetVariableDescriptor& descr,
                             const std::vector<bool>&           is_cumulative);

        /// Prepare internal value arrays for accumulating
        /// per-variable/per-region values.
        ///
        /// Essentially zeroes an internal increment buffer.
        ///
        /// Must be called before the first call to addRegionValue().
        void prepareValueAccumulation();

        /// Aggregate current increments into internal value array.
        ///
        /// Adds increment values for cumulative quantities and overwrites
        /// current values for non-cumulative quantities.
        ///
        /// Must be called after the last call to addRegionValue().
        void commitValues();

        /// Add contribution to single region of single variable.
        ///
        /// \param[in] var_ix Variable index.  Assumed to be an index into
        /// the \c is_cumulative parameter of member function
        /// defineVariables().
        ///
        /// \param[in] regset_ix Region set index.  Assumed to be an index
        /// in the range 0...descr.numRegionSets()-1 with \c descr being the
        /// region set descriptor in defineVariables().
        ///
        /// \param[in] region_ix Region index.  Assumed to be a valid region
        /// index in the region set represented by \p regset_ix.
        ///
        /// \param[in] x Numerical value of contribution to region variable.
        /// Could for instance be a fluid volume from a cell or a connection
        /// level flow rate.
        void addRegionValue(const std::size_t var_ix,
                            const std::size_t regset_ix,
                            const std::size_t region_ix,
                            const double      x);

        /// Numerical values of a single variable for all region sets.
        ///
        /// Read-only access.
        ///
        /// \param[in] var_ix Variable index.  Assumed to be an index into
        /// the \c is_cumulative parameter of member function
        /// defineVariables().
        ///
        /// \return Numerical values of a single variable.  Nullopt if \p
        /// var_ix is out of bounds.
        std::optional<RegionVariableView<const double>>
        values(const std::size_t var_ix) const;

    protected:
        /// Value buffer for accumulating per-region contributions to all
        /// variables.
        ///
        /// Gets zeroed in prepareValueAccumulation() and aggregated into
        /// values_ in commitValues().  Directly accessible to derived
        /// classes in order to support parallel communication.
        std::vector<double> increment_{};

        /// Compute canonical increment_ values.
        ///
        /// Intended as a hook for derived classes to implement parallel
        /// communication of increment_ values.  The default implementation
        /// does nothing.
        virtual void communicateIncrement() {}

    private:
        /// Type alias for a reseatable reference to a region set descriptor.
        using DescrRef = std::reference_wrapper<const RegionsetVariableDescriptor>;

        /// Region set descriptor.
        std::optional<DescrRef> descr_{};

        /// Storage indices for individual variables.
        ///
        /// We reorder the variables to have cumulatives stored contiguously
        /// as that simplifies the implementation of commitValues().
        std::vector<std::vector<bool>::size_type> storage_ix_{};

        /// Partition point between cumulatives and non-cumulatives.
        ///
        /// Index into storage_ix_.
        std::size_t end_cum_{};

        /// Numerical values for all variables for all regions in all region
        /// sets.
        std::vector<double> values_{};

        /// Reorder variables to have cumulatives stored contiguously.
        ///
        /// \param[in] is_cumulative Whether or not a particular variable
        /// represents a cumulative quantity.
        void partitionVariables(const std::vector<bool>& is_cumulative);

        /// Allocate storage for increment_ and values_.
        void allocateValues();
    };

} // namespace Opm::data

#endif // OPM_OUTPUT_DATA_REGION_VARIABLE_VALUES_HPP
