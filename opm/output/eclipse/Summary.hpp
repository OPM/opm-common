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

#include <ert/ecl/ecl_sum.h>
#include <ert/util/ert_unique_ptr.hpp>

#include <opm/output/Wells.hpp>

namespace Opm {

    class EclipseState;
    class SummaryConfig;

namespace out {

class Summary {
    public:
        Summary( const EclipseState&, const SummaryConfig& );
        Summary( const EclipseState&, const SummaryConfig&, const std::string& );
        Summary( const EclipseState&, const SummaryConfig&, const char* basename );

        void add_timestep( int report_step, double step_duration,
                           const EclipseState&, const data::Wells& );
        void write();

    private:
        ERT::ert_unique_ptr< ecl_sum_type, ecl_sum_free > ecl_sum;
        std::map< const char*, std::vector< smspec_node_type* > > wvar;
        std::map< const char*, std::vector< smspec_node_type* > > gvar;
        const ecl_sum_tstep_type* prev_tstep = nullptr;
        double duration = 0;
        const double* conversions;
};

}
}

#endif //OPM_OUTPUT_SUMMARY_HPP
