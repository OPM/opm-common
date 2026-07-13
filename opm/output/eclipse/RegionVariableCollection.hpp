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

#ifndef OPM_OUTPUT_REGION_VARIABLE_COLLECTION_HPP
#define OPM_OUTPUT_REGION_VARIABLE_COLLECTION_HPP

#include <opm/output/data/RegionVariableMapping.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace Opm {
    class FieldPropsManager;
} // namespace Opm

namespace Opm::data {
    class RegionsetVariableDescriptor;
    class RegionVariableValues;
} // namespace Opm::data

/// \file Component that gives a view into a sequence of (numerical) values
/// keyed by region sets and region indices.

namespace Opm {

    /// Management structure for the numerical values of all region level
    /// variables for all region sets.
    class RegionVariableCollection
    {
    public:
        /// Constructor.
        ///
        /// \param[in] descr Empty container of region sets.  Typically a
        /// default-constructed object (\code make_unique<>() \endcode) of
        /// type \c RegionsetVariableDescriptor or a type derived from this.
        ///
        /// \param[in] vals Empty container of variable values.  Typically
        /// a default-constructed object (\code make_unique<>() \endcode) of
        /// type \c RegionVariableValues or a type derived from this.
        explicit RegionVariableCollection(std::unique_ptr<data::RegionsetVariableDescriptor> descr,
                                          std::unique_ptr<data::RegionVariableValues>        vals);

        /// Copy constructor.
        ///
        /// \param[in] rhs Source object from which to form an independent
        /// copy.
        RegionVariableCollection(const RegionVariableCollection& rhs);

        /// Move constructor.
        ///
        /// \param[in,out] rhs Source object from which to form a new
        /// object.  Left in an empty state on return from the move
        /// constructor.
        RegionVariableCollection(RegionVariableCollection&& rhs);

        /// Destructor.
        ~RegionVariableCollection();

        /// Assignment operator.
        ///
        /// \param[in] rhs Source object whose value will overwrite \code
        /// *this \endcode.
        ///
        /// \return *this.
        RegionVariableCollection& operator=(const RegionVariableCollection& rhs);

        /// Move-assignment operator.
        ///
        /// \param[in] rhs Source object which will be moved into \code
        /// *this \endcode.  Left in an empty state on return from the move
        /// constructor.
        ///
        /// \return *this.
        RegionVariableCollection& operator=(RegionVariableCollection&& rhs);

        /// Build internal structures and allocate value storage for all
        /// variables and all region sets.
        ///
        /// \param[in] declaredMaxRegID Run's declared maximum region ID.
        /// Typically entered in the TABDIMS or the REGDIMS keyword.
        ///
        /// \param[in] fpMgr Run's static properties, particularly the
        /// region definition arrays such as FIPNUM.  Note that this
        /// RegionVariableCollection object will store iterators to the
        /// region definition arrays so the \p fpMgr must outlive the
        /// RegionVariableCollection.
        ///
        /// \param[in] rvarMap Mapping from named region sets and named
        /// region variables to numeric indices.
        void initialise(const int                          declaredMaxRegID,
                        const FieldPropsManager&           fpMgr,
                        const data::RegionVariableMapping& rvarMap);

        /// Add contribution to a single variable for all region sets.
        ///
        /// \param[in] var_ix Variable index.  Assumed to be in the range
        /// \code 0..rvarMap.numVariables()-1 \endcode with \c rvarMap being
        /// the mapping object passed to initialise().
        ///
        /// \param[in] cell_ix Active cell index to which to attribute this
        /// cell level contribution.
        ///
        /// \param[in] x Numerical value of cell level contribution to this
        /// region level variable.
        void addCellValue(const std::size_t var_ix,
                          const std::size_t cell_ix,
                          const double      x);

        /// Prepare internal value arrays for accumulating
        /// per-variable/per-region values.
        ///
        /// Essentially zeroes an internal increment buffer.
        ///
        /// Must be called before the first call to addCellValue().
        void prepareValueAccumulation();

        /// Aggregate current increments into internal value array.
        ///
        /// Adds increment values for cumulative quantities and overwrites
        /// current values for non-cumulative quantities.
        ///
        /// Must be called after the last call to addCellValue().
        void commitValues();

        /// Translate region set name to numeric region set identifier.
        ///
        /// Knows about special purpose "FIELD" region set.
        ///
        /// \param[in] varMap Mapping from named region sets and named
        /// region variables to numeric indices.  Must be the same mapping
        /// object used to initialise() this RegionVariableCollection.
        ///
        /// \param[in] regionSet Region set name.  Should be one of the
        /// region sets registered in the \p varMap or the special purpose
        /// region set named "FIELD".
        ///
        /// \return Numeric region set identifier.  Nullopt if \p regionSet
        /// does not exist in \p varMap and is not "FIELD".
        std::optional<std::size_t>
        regionSetIndex(const data::RegionVariableMapping& varMap,
                       const std::string&                 regionSet) const;

        /// Translate region variable name to numeric region variable
        /// identifier.
        ///
        /// \param[in] varMap Mapping from named region sets and named
        /// region variables to numeric indices.  Must be the same mapping
        /// object used to initialise() this RegionVariableCollection.
        ///
        /// \param[in] variable Region variable name.  Should be one of the
        /// region variables registered in the \p varMap.
        ///
        /// \return Numeric region variable identifier.  Nullopt if \p
        /// variable does not exist in \p varMap.
        std::optional<std::size_t>
        variableIndex(const data::RegionVariableMapping& varMap,
                      const std::string&                 variable) const;

        /// Current numerical values of all region level variables for all
        /// known region sets.
        const data::RegionVariableValues& regionVariableValues() const;

    private:
        /// Region sets.
        std::unique_ptr<data::RegionsetVariableDescriptor> descr_{};

        /// Numerical values of all variables for all region sets.
        std::unique_ptr<data::RegionVariableValues> vals_{};

        /// Region IDs.
        ///
        /// One entry for each region set registered in the variable mapping
        /// passed to initialise().
        std::vector<std::span<const int>> regSet_{};

        /// Construct region set descriptor and region ID sequences.
        ///
        /// Writes to \code descr_ \endcode and \code regSet_ \endcode.
        ///
        /// \param[in] declaredMaxRegID Run's declared maximum region ID.
        /// Typically entered in the TABDIMS or the REGDIMS keyword.
        ///
        /// \param[in] fpMgr Run's static properties, particularly the
        /// region definition arrays such as FIPNUM.  Note that this
        /// RegionVariableCollection object will store iterators to the
        /// region definition arrays so the \p fpMgr must outlive the
        /// RegionVariableCollection.
        ///
        /// \param[in] rvarMap Mapping from named region sets and named
        /// region variables to numeric indices.
        void initialiseRegionDescriptors(const int                          declaredMaxRegID,
                                         const FieldPropsManager&           fpMgr,
                                         const data::RegionVariableMapping& rvarMap);

        /// Construct region variable value object
        ///
        /// Writes to \code vals_ \endcode
        ///
        /// \param[in] rvarMap Mapping from named region sets and named
        /// region variables to numeric indices.
        void initialiseRegionValues(const data::RegionVariableMapping& rvarMap);
    };

} // namespace Opm

#endif // OPM_OUTPUT_REGION_VARIABLE_COLLECTION_HPP
