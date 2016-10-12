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

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Completion.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperty.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include <opm/output/eclipse/RegionCache.hpp>

namespace Opm {
namespace out {

    RegionCache::RegionCache(const EclipseState& state, const EclipseGrid& grid) {
        const auto& properties = state.get3DProperties();
        const auto& fipnum = properties.getIntGridProperty("FIPNUM");
        const auto& region_values = properties.getRegions( "FIPNUM" );

        for (auto region_id : region_values)
            this->cell_map.emplace( region_id , fipnum.cellsEqual( region_id , grid ));


        {
            const auto& schedule = state.getSchedule();
            const auto& wells = schedule.getWells();
            for (const auto& well : wells) {
                const auto& completions = well->getCompletions( );
                for (const auto& c : completions) {
                    size_t global_index = grid.getGlobalIndex( c.getI() , c.getJ() , c.getK());
                    size_t active_index = grid.activeIndex( global_index );
                    int region_id =fipnum.iget( global_index );
                    auto& well_index_list = this->completion_map[ region_id ];
                    well_index_list.push_back( { well->name() , active_index } );
                }
            }
        }
    }


    const std::vector<size_t>& RegionCache::cells( int region_id ) const {
        const auto iter = this->cell_map.find( region_id );
        if (iter == this->cell_map.end())
            return this->cells_empty;
        else
            return iter->second;
    }



    const std::vector<std::pair<std::string,size_t>>& RegionCache::completions( int region_id ) const {
        const auto iter = this->completion_map.find( region_id );
        if (iter == this->completion_map.end())
            return this->completions_empty;
        else
            return iter->second;
    }

}
}
