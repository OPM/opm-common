/*
  Copyright 2026 Equinor ASA.

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

#ifndef WELL_CONTROL_TRACKER_HPP
#define WELL_CONTROL_TRACKER_HPP

#include <opm/common/OpmLog/KeywordLocation.hpp>

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Opm {

/// Bookkeeping for wells that are declared (WELSPECS/WELSPECL) and
/// completed (COMPDAT/COMPDATL) but never assigned a control through any of
/// the WCONHIST, WCONPROD, WCONINJE, or WCONINJH keywords.
class WellControlTracker
{
public:
    /// \brief Mark a well as defined with its location.
    ///
    /// This is typically called from the WELSPECS/WELSPECL keyword handler.
    ///
    /// \param[in] well The name of the well being defined.
    /// \param[in] location The location of the well's definition keyword.
    void defined(const std::string& well, const KeywordLocation& location);

    /// \brief Mark a well as connected to the simulation model
    ///
    /// This is typically called from the COMPDAT/COMPDATL keyword handler.
    ///
    /// \param[in] well The name of the well being connected.
    void connected(const std::string& well);

    /// \brief Mark a well as controlled in the simulation model.
    ///
    /// This is typically called from the WCONHIST, WCONPROD, WCONINJE,
    /// or WCONINJH keyword handler.
    ///
    /// \param[in] well The name of the well being controlled.
    void controlled(const std::string& well);

    /// Wells that have connections but no control, with the location of
    /// their (first) WELSPECS/WELSPECL entry.
    ///
    /// Wells returned in order of appearance--i.e., effectively sorted by
    /// their definition order.
    std::vector<std::pair<std::string, KeywordLocation>> uncontrolled() const;

private:
    /// \brief Map from well name to the index of its location in the
    /// locations_ vector.
    std::unordered_map<std::string, std::vector<KeywordLocation>::size_type> defined_{};

    /// \brief Locations of the wells as specified in the WELSPECS/WELSPECL
    /// keywords.
    std::vector<KeywordLocation> locations_{};

    /// \brief Flags indicating whether each well is connected to the
    /// simulation model.
    std::vector<bool> connected_{};

    /// \brief Flags indicating whether each well is controlled in the
    /// simulation model.
    std::vector<bool> controlled_{};

    /// \brief Get the index of a well in the locations_ vector.
    ///
    /// \param[in] well The name of the well.
    ///
    /// \return The index of the well in the locations_ vector, or
    /// std::nullopt if the well is not defined (unexpected).
    std::optional<std::vector<KeywordLocation>::size_type>
    wellIndex(const std::string& well) const;
};

} // namespace Opm

#endif // WELL_CONTROL_TRACKER_HPP
