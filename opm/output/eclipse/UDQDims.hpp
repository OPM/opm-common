/*
  Copyright (c) 2021 Equinor ASA

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

#ifndef OPM_UDQDIMS_HPP
#define OPM_UDQDIMS_HPP

#include <cstddef>
#include <functional>
#include <optional>
#include <vector>

namespace Opm {

class UDQConfig;

} // namespace Opm

namespace Opm {

/// Collection of UDQ and UDA related dimension queries
///
/// Used to size various restart file output arrays.
class UDQDims
{
public:
    /// Constructor
    ///
    /// \param[in] config Collection of run's UDQs.
    ///
    /// \param[in] intehead Current report step's INTEHEAD array.  Queried
    /// for most dimension values.
    explicit UDQDims(const UDQConfig&        config,
                     const std::vector<int>& intehead);

    /// Number of IUDQ elements per UDQ.
    static std::size_t entriesPerIUDQ() { return 3; }

    /// Number of IUAD elements per UDA.
    static std::size_t entriesPerIUAD() { return 5; }

    /// Number of ZUDN elments per UDQ.
    static std::size_t entriesPerZUDN() { return 2; }

    /// Number of ZUDL elments per UDQ.
    static std::size_t entriesPerZUDL() { return 16; }

    /// Total number of UDQs in run of all types/categories.
    std::size_t totalNumUDQs() const;

    /// Total number of UDAs in run.
    std::size_t numIUAD() const;

    /// Number of potential group level injection phase UDAs.
    ///
    /// Zero if no UDAs in run, maximum number of groups otherwise.
    std::size_t numIGPH() const;

    /// Number of well/group IDs involved in UDAs.
    std::size_t numIUAP() const;

    /// Number of field level UDQs
    std::size_t numFieldUDQs() const;

    /// Maximum number of groups in run, including FIELD
    std::size_t maxNumGroups() const;

    /// Number of group level UDQs
    std::size_t numGroupUDQs() const;

    /// Run's maximum number of multi-segmented wells
    std::size_t maxNumMsWells() const;

    /// Run's maximum number of segments per multi-segmented well
    std::size_t maxNumSegments() const;

    /// Number of segment level UDQs.
    std::size_t numSegmentUDQs() const;

    /// Run's maximum number of wells, multi-segmented or otherwise
    std::size_t maxNumWells() const;

    /// Number of well level UDQs.
    std::size_t numWellUDQs() const;

    /// Linear sequence of some array sizes.
    ///
    /// Retained for backwards compatibility but will be removed in the
    /// future.
    [[deprecated("The data vector is not aware of categories other than field, group, or well.  Use named accessors instead.")]]
    const std::vector<int>& data() const
    {
        if (! this->dimensionData_.has_value()) {
            this->collectDimensions();
        }

        return *this->dimensionData_;
    }

private:
    /// Total number of UDQs of all categories
    std::size_t totalNumUDQs_{};

    /// Current report step's INTEHEAD array.  Backend for most size
    /// queries.
    std::reference_wrapper<const std::vector<int>> intehead_;

    /// Backing storage for original linear sequence of selected array sizes.
    mutable std::optional<std::vector<int>> dimensionData_;

    /// Build original sequence of selected array sizes.
    void collectDimensions() const;

    /// Query INTEHEAD for individual dimension item.
    ///
    /// \param[in] i index into INTEHEAD.  Should be one of the named items
    /// in VectorItems/intehead.hpp.
    ///
    /// \returns Corresponding INTEHEAD item.
    std::size_t intehead(const std::vector<int>::size_type i) const;
};

} // namespace Opm

#endif // OPM_UDQDIMS_HPP
