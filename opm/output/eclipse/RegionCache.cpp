/*
  Copyright 2016 Statoil ASA.

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

#include <opm/output/eclipse/RegionCache.hpp>

#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/Well/Connection.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>
#include <opm/input/eclipse/Schedule/Well/WellMatcher.hpp>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <set>
#include <string>
#include <vector>

Opm::out::RegionCache::RegionCache(const std::set<std::string>& fip_regions,
                                   const FieldPropsManager&     fp,
                                   const EclipseGrid&           grid,
                                   const Schedule&              schedule)
{
    this->buildCache(fip_regions, fp, grid, schedule);
}

void Opm::out::RegionCache::buildCache(const std::set<std::string>& fip_regions,
                                       const FieldPropsManager&     fp,
                                       const EclipseGrid&           grid,
                                       const Schedule&              schedule)
{
    if (fip_regions.empty()) {
        return;
    }

    auto regions = std::vector<std::reference_wrapper<const std::vector<int>>>{};
    std::ranges::transform(fip_regions, std::back_inserter(regions),
                           [&fp](const auto& fipReg)
                           { return std::cref(fp.get_int(fipReg)); });

    for (const auto& wname : schedule.back().well_order()) {
        const auto& conns = schedule.back().wells(wname).getConnections();
        if (conns.empty()) { continue; }

        auto regID = regions.begin();
        for (const auto& fipReg : fip_regions) {
            auto first = true;

            for (const auto& conn : conns) {
                if (! grid.cellActive(conn.global_index())) {
                    continue;
                }

                const auto region = regID->get()[grid.activeIndex(conn.global_index())];
                const auto reg_pair = std::make_pair(fipReg, region);

                this->connection_map[reg_pair]
                    .emplace_back(wname, conn.global_index());

                if (first) {
                    this->well_map[reg_pair].push_back(wname);

                    first = false;
                }
            }

            ++regID;
        }
    }
}


const std::vector<std::pair<std::string, std::size_t>>&
Opm::out::RegionCache::connections(const std::string& region_name,
                                   const int          region_id) const
{
    const auto key = std::make_pair(region_name, region_id);
    auto iter = this->connection_map.find(key);

    return (iter == this->connection_map.end())
        ? this->connections_empty
        : iter->second;
}

std::vector<std::string>
Opm::out::RegionCache::wells(const std::string& region_name,
                             const int          region_id) const
{
    const auto key = std::make_pair(region_name, region_id);
    auto iter = this->well_map.find(key);

    return (iter == this->well_map.end())
        ? std::vector<std::string> {}
        : iter->second;
}
