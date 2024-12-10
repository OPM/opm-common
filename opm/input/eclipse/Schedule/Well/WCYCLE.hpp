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
    struct WellStatusInfo
    {
        std::string name{};
        bool open{false};
    };

    struct WellTimeInfo
    {
        double time{0.0};
        bool cycling{false};
    };
    using WellTimeMap = std::map<std::string,WellTimeInfo>;

    struct Entry
    {
        double on_time{};
        double off_time{};
        double startup_time{};
        double max_time_step{};
        bool controlled_time_step{false};

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(on_time);
            serializer(off_time);
            serializer(startup_time);
            serializer(max_time_step);
            serializer(controlled_time_step);
        }

        bool operator==(const Entry& entry) const;
    };

    WCYCLE() = default;

    void addRecord(const DeckRecord& record);

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(entries_);
    }

    static WCYCLE serializationTestObject();

    bool operator==(const WCYCLE& other) const;

    bool empty() const { return entries_.empty(); }

    void start(const double current_time,
               const WellMatcher& wmatch,
               WellTimeMap& open_times) const;

    double nextTimeStep(const double current_time,
                        const double dt,
                        const WellMatcher& wmatch,
                        const WellTimeMap& open_times,
                        const WellTimeMap& close_times,
                        std::function<bool(const std::string&)> opens_this_step) const;

    std::vector<WellStatusInfo>
    wellStatus(const double current_time,
               const WellMatcher& wmatch,
               WellTimeMap& open_times,
               WellTimeMap& close_times) const;

    std::vector<std::pair<std::string, double>>
    efficiencyScale(const double current_time,
                    const double dt,
                    const WellMatcher& wmatch,
                    const WellTimeMap& open_times) const;

private:
    std::unordered_map<std::string, Entry> entries_;
};

} // namespace Opm

#endif // OPM_WCYCLE_HPP
