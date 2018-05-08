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

#include <config.h>

#define BOOST_TEST_MODULE serialize_SCON_TEST
#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <ert/ecl_well/well_const.h> // containts SCON_CF_INDEX
#include <opm/output/eclipse/WriteRestartHelpers.hpp>

BOOST_AUTO_TEST_CASE( serialize_icon_test )
{
    const Opm::Deck deck(Opm::Parser{}.parseFile("tests/FIRST_SIM.DATA"));
    const Opm::EclipseState state(deck);
    const Opm::Schedule schedule(deck, state);
    const Opm::TimeMap timemap(deck);

  
    for (size_t tstep = 0; tstep != timemap.numTimesteps(); ++tstep) {

        const size_t ncwmax = schedule.getMaxNumCompletionsForWells(tstep);

        const int SCONZ = 40; // normally obtained from InteHead
        const auto wells = schedule.getWells(tstep);
        
        const std::vector<double> scondata =
            Opm::RestartIO::Helpers::serialize_SCON(tstep,
                                                    ncwmax,
                                                    SCONZ,
                                                    wells);
        size_t w_offset = 0;
        for (const auto w : wells) {

            size_t c_offset = 0;
            for (const auto c : w->getCompletions(tstep)) {

                const size_t offset = w_offset + c_offset;

                BOOST_CHECK_EQUAL(scondata[offset + SCON_CF_INDEX],
                                  c.getConnectionTransmissibilityFactor());
                BOOST_CHECK_EQUAL(scondata[offset + SCON_KH_INDEX], 
                             Opm::RestartIO::Helpers::UNIMPLEMENTED_VALUE); 

                c_offset += SCONZ;
            }
            w_offset += (SCONZ * ncwmax);
        }
    }
};
  
