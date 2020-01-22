/*
  Copyright 2020 Equinor ASA

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

#include <opm/io/eclipse/rst/state.hpp>
#include <opm/io/eclipse/ERst.hpp>




int main(int argc, char ** argv) {
    for (int iarg = 1; iarg < argc; iarg++) {
        Opm::EclIO::ERst rst_file(argv[iarg]);
        for (int report_step : rst_file.listOfReportStepNumbers()) {
            if (report_step > 0)
                const auto& state = Opm::RestartIO::RstState::load(rst_file, report_step);
        }
    }
}
