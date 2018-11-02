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

#ifndef OPM_OUTPUT_SUMMARY_HPP
#define OPM_OUTPUT_SUMMARY_HPP

#include <map>
#include <string>
#include <vector>

#include <ert/ecl/ecl_sum.h>

#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>

#include <opm/output/data/Wells.hpp>
#include <opm/output/eclipse/RegionCache.hpp>

namespace Opm {

    class EclipseState;
    class Schedule;
    class SummaryConfig;
    class EclipseGrid;
    class Schedule;

namespace out {

class Summary {
    public:
        Summary( const EclipseState&, const SummaryConfig&, const EclipseGrid&, const Schedule& );
        Summary( const EclipseState&, const SummaryConfig&, const EclipseGrid&, const Schedule&, const std::string& );
        Summary( const EclipseState&, const SummaryConfig&, const EclipseGrid&, const Schedule&, const char* basename );

        void add_timestep(int report_step,
                           double secs_elapsed,
                           const EclipseState& es,
                           const Schedule& schedule,
                           const data::Wells&,
                           const std::map<std::string, double>& single_values,
                           const std::map<std::string, std::vector<double>>& region_values = {},
                           const std::map<std::pair<std::string, int>, double>& block_values = {});

        void write();

        ~Summary();

        const SummaryState& get_restart_vectors() const;

        void reset_cumulative_quantities(const SummaryState& rstrt);

    private:
        class keyword_handlers;

        const EclipseGrid& grid;
        out::RegionCache regionCache;
        ERT::ert_unique_ptr< ecl_sum_type, ecl_sum_free > ecl_sum;
        std::unique_ptr< keyword_handlers > handlers;
        double prev_time_elapsed = 0;
        SummaryState prev_state;
};

}
}

#endif //OPM_OUTPUT_SUMMARY_HPP
