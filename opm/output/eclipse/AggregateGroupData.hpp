/*
  Copyright (c) 2018 Statoil ASA

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

#ifndef OPM_AGGREGATE_GROUP_DATA_HPP
#define OPM_AGGREGATE_GROUP_DATA_HPP

#include <opm/output/eclipse/WindowedArray.hpp>

#include <opm/io/eclipse/PaddedOutputString.hpp>

#include <cstddef>
#include <string>
#include <vector>
#include <map>

namespace Opm {
class Schedule;
class SummaryState;
class TracerConfig;
} // namespace Opm

namespace Opm::RestartIO::Helpers {

class AggregateGroupData
{
public:
    explicit AggregateGroupData(const std::vector<int>& inteHead);

    void captureDeclaredGroupData(const Schedule&         sched,
                                  const TracerConfig&     tracer,
                                  const std::size_t       simStep,
                                  const SummaryState&     sumState,
                                  const std::vector<int>& inteHead);

    void captureDeclaredGroupDataLGR(const Schedule&     sched,
                                     const std::size_t   simStep,
                                     const SummaryState& sumState,
                                     const std::string&  lgr_tag);

    const std::vector<int>& getIGroup() const
    {
        return this->iGroup_.data();
    }

    const std::vector<float>& getSGroup() const
    {
        return this->sGroup_.data();
    }

    const std::vector<double>& getXGroup() const
    {
        return this->xGroup_.data();
    }

    const std::vector<EclIO::PaddedOutputString<8>>& getZGroup() const
    {
        return this->zGroup_.data();
    }

private:
    /// Aggregate 'IWEL' array (Integer) for all wells.
    WindowedArray<int> iGroup_;

    /// Aggregate 'SWEL' array (Real) for all wells.
    WindowedArray<float> sGroup_;

    /// Aggregate 'XWEL' array (Double Precision) for all wells.
    WindowedArray<double> xGroup_;

    /// Aggregate 'ZWEL' array (Character) for all wells.
    WindowedArray<EclIO::PaddedOutputString<8>> zGroup_;

    /// Maximum number of wells in a group.
    int nWGMax_;

    /// Maximum number of groups
    int nGMaxz_;
};

} // Opm::RestartIO::Helpers

#endif // OPM_AGGREGATE_GROUP_DATA_HPP
