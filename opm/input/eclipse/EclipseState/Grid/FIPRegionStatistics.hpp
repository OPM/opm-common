/*
  Copyright 2024 Equinor ASA.

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

#ifndef FIP_REGION_STATISTICS_HPP
#define FIP_REGION_STATISTICS_HPP

#include <cstddef>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

/// \file Component for deriving basic descriptive statistics about a
/// model's fluid-in-place regions.

namespace Opm {
    class FieldPropsManager;
} // namespace Opm

namespace Opm {

    /// Basic descriptive statistics about a model's fluid-in-place regions
    class FIPRegionStatistics
    {
    public:
        /// Default constructor
        FIPRegionStatistics() = default;

        /// Constructor.
        ///
        /// \param[in] declaredMaxRegID Model's declared maximum FIP region
        ///   ID.  Usually the maximum of TABDIMS(5) and REGDIMS(1).
        ///
        /// \param[in] fldPropsMgr Model's field properties.  In particular,
        ///   for read-only access to the model's defined FIP* arrays.
        ///
        /// \param[in] computeGlobalMax Call-back function which computes
        ///   the global maximum for each region set given an array of local
        ///   maximum region IDs.  Should be MPI-aware in a parallel run.
        ///   Called as
        ///   \code
        ///      auto localMax = computeLocalMax(all_fip_regions);
        ///      computeGlobalMax(localMax)
        ///   \endcode
        ///   and is expected to replace the local maximum in each element
        ///   of 'localMax' with the corresponding global maximum across the
        ///   complete model.
        explicit FIPRegionStatistics(const std::size_t                      declaredMaxRegID,
                                     const FieldPropsManager&               fldPropsMgr,
                                     std::function<void(std::vector<int>&)> computeGlobalMax);

        /// Equality predicate
        ///
        /// \param[in] that Object to which \code *this \endcode will be
        ///   compared for equality.
        bool operator==(const FIPRegionStatistics& that) const;

        /// Serialisation test object.
        static FIPRegionStatistics serializationTestObject();

        /// Retrieve model's declared maximum fluid-in-place region ID.
        ///
        /// \return Constructor argument \c declaredMaxRegID
        int declaredMaximumRegionID() const
        {
            return this->minimumMaximumRegionID_;
        }

        /// Get list of named region sets, without the initial 'FIP' name
        /// prefix.
        ///
        /// As an example, the standard 'FIPNUM' region set will be
        /// represented by the name 'NUM' in this array.
        const std::vector<std::string>& regionSets() const
        {
            return this->regionSets_;
        }

        /// Get global maximum region ID of a named region set
        ///
        /// \param[in] regionSet Named region set, with or without the 'FIP'
        ///   region set name prefix.
        ///
        /// \return Model's global maximum region ID in \p regionSet.
        ///   Negative value (-1) if \p regionSet is not a known region set
        ///   name.
        int maximumRegionID(std::string_view regionSet) const;

        /// Serialisation operator
        ///
        /// \tparam Serializer Protocol for serialising and deserialising
        ///   objects between memory and character buffers.
        ///
        /// \param[in,out] serializer Serialisation object.
        template <typename Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->minimumMaximumRegionID_);
            serializer(this->regionSets_);
            serializer(this->maxRegionID_);
        }

    private:
        /// Model's declared maximum fluid-in-place region ID.
        int minimumMaximumRegionID_{};

        /// Model's named 'FIP' region sets, including 'FIPNUM'.  Sorted
        /// alphabetically to enable binary search when looking up aspects
        /// of the individual region sets.
        std::vector<std::string> regionSets_{};

        /// Collection of maximum region IDs across model.  Stored in the
        /// order of the regionSets_.
        std::vector<int> maxRegionID_{};
    };
} // namespace Opm

#endif // FIP_REGION_STATISTICS_HPP
