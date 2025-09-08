/*
  Copyright 2023 Equinor.

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

#ifndef OPM_WCYCLE_HPP
#define OPM_WCYCLE_HPP

#include <functional>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>

namespace Opm {

class DeckRecord;
class WellMatcher;

class WCYCLE
{
public:
    /// A vector of wells and efficiency factor scalings to apply.
    using WellEfficiencyVec = std::vector<std::pair<std::string, double>>;
    /// A map from well name to a time stamp.
    using WellTimeMap = std::map<std::string, double>;
    /// A vector of wells and their open status.
    using WellIsOpenMap = std::map<std::string, bool>;

    /// Represents a single record in a WCYCLE keyword.
    struct Entry
    {
        double on_time{}; //!< Length of well open period
        double off_time{};  //!< Length of well closed period
        double startup_time{}; //!< Time interval to scale up well efficiency factor
        double max_time_step{}; //!< Maximum time step when a cycling well opens
        bool controlled_time_step{false}; //!< Whether or not to

        /// Convert between byte array and object representation.
        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(on_time);
            serializer(off_time);
            serializer(startup_time);
            serializer(max_time_step);
            serializer(controlled_time_step);
        }

        /// Equality predicate
        bool operator==(const Entry& entry) const;
    };

    /// Parse a record for a WCYCLE keyword.
    /// \param[in] record Record to parse
    void addRecord(const DeckRecord& record);

    /// Convert between byte array and object representation.
    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(entries_);
    }

    /// Create non-defaulted object suitable for testing the serialisation
    /// operation.
    static WCYCLE serializationTestObject();

    /// Returns true if \this has no entries.
    bool empty() const { return entries_.empty(); }

    /// Returns an iterator to first entry.
    auto begin() const { return entries_.begin(); }
    /// Returns an iterator to one past the last entry.
    auto end() const { return entries_.end(); }

    /// Returns a time step adjusted according to cycling wells.
    ///
    /// \param[in] current_time Current time level of simulator
    /// \param[in] dt Currently suggested time step for simulator
    /// \param[in] wmatch Well matcher handling WLIST resolution
    /// \param[in] open_times Times at which cycling wells were opened
    /// \param[in] close_times Times at which cycling wells were closed
    /// \param[in] opens_this_step Callback to check if a well will open at
    ///                            at current time step.
    /// \return Adjusted time step
    ///
    /// \details The callback is required to handle the situation where
    ///          we are doing the first time step of a new report step,
    ///          and the well opens at the current report step. The time step
    ///          has to be chosen before the new opening time is registered,
    ///          and we end up cycling a well that should not be cycled.
    double nextTimeStep(const double current_time,
                        const double dt,
                        const WellMatcher& wmatch,
                        const WellTimeMap& open_times,
                        const WellTimeMap& close_times,
                        std::function<bool(const std::string&)> opens_this_step) const;

    /// Returns status (open/shut) for cycling wells
    /// \param[in] current_time Current time level of simulator
    /// \param[in] wmatch Well matcher handling WLIST resolution
    /// \param[in] open_times Times at which cycling wells were opened
    /// \param[in] close_times Times at which cycling wells were closed
    /// \return Map of open/closed status of cycling wells
    WellIsOpenMap wellStatus(const double current_time,
                             const WellMatcher& wmatch,
                             WellTimeMap& open_times,
                             WellTimeMap& close_times) const;

    /// Returns efficiency factor scaling factors for cycling wells
    /// \param[in] current_time Current time level of simulator
    /// \param[in] dt Current time step for simulator
    /// \param[in] wmatch Well matcher handling WLIST resolution
    /// \param[in] open_times Times at which cycling wells were opened
    /// \param[in] schedule_open Callback to check if well is opened by the schedule
    /// \return Vector of efficiency scaling factors for cycling wells
    WellEfficiencyVec efficiencyScale(const double current_time,
                                      const double dt,
                                      const WellMatcher& wmatch,
                                      const WellTimeMap& open_times,
                                      std::function<bool(const std::string&)> schedule_open) const;

    /// Equality predicate
    bool operator==(const WCYCLE& that) const;

private:
    std::unordered_map<std::string, Entry> entries_; //!< Map of WCYCLE entries
};

} // namespace Opm

#endif // OPM_WCYCLE_HPP
