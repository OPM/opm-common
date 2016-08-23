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

#include <string>
#include <vector>
#include <unordered_map>

#include <ert/ecl/ecl_sum.h>
#include <ert/ecl/Smspec.hpp>

#include <opm/output/Wells.hpp>
#include <opm/output/Cells.hpp>

namespace Opm {

    class EclipseState;
    class Schedule;
    class SummaryConfig;

namespace out {

class Summary {
    public:
        Summary( const EclipseState&, const SummaryConfig& );
        Summary( const EclipseState&, const SummaryConfig&, const std::string& );
        Summary( const EclipseState&, const SummaryConfig&, const char* basename );

        void add_timestep( int report_step,
                           double secs_elapsed,
                           const std::vector<int>& indexMap,
                           const EclipseState& es,
                           const std::unordered_map<int, std::vector<size_t>>& regionCells,
                           const data::Wells&,
                           const data::Solution&);
        void write();

        ~Summary();

    private:
        class keyword_handlers;

        ERT::ert_unique_ptr< ecl_sum_type, ecl_sum_free > ecl_sum;
        std::unique_ptr< keyword_handlers > handlers;
        const ecl_sum_tstep_type* prev_tstep = nullptr;
        double prev_time_elapsed = 0;
};

}
}

#endif //OPM_OUTPUT_SUMMARY_HPP
