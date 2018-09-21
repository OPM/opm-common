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

#include <iostream> // @@
#include <algorithm> // @@
#include <iterator> // @@ 

#define BOOST_TEST_MODULE serialize_ICON_TEST
#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <ert/ecl_well/well_const.h> // containts ICON_XXX_INDEX
#include <opm/output/eclipse/WriteRestartHelpers.hpp>

BOOST_AUTO_TEST_CASE( serialize_icon_test )
{
    const Opm::Deck deck(Opm::Parser{}.parseFile("FIRST_SIM.DATA"));
    const Opm::EclipseState state(deck);
    const Opm::Schedule schedule(deck, state);
    const Opm::TimeMap timemap(deck);


    for (size_t tstep = 0; tstep != timemap.numTimesteps(); ++tstep) {

        const size_t ncwmax = schedule.getMaxNumConnectionsForWells(tstep);

        const int ICONZ = 25; // normally obtained from InteHead
        const auto wells = schedule.getWells(tstep);

        const std::vector<int> icondata =
            Opm::RestartIO::Helpers::serialize_ICON(tstep,
                                                    ncwmax,
                                                    ICONZ,
                                                    wells);
        size_t w_offset = 0;
        for (const auto w : wells) {

            size_t c_offset = 0;
            for (const auto c : w->getConnections(tstep)) {

                const size_t offset = w_offset + c_offset;

                BOOST_CHECK_EQUAL(icondata[offset + ICON_IC_INDEX],
                                  c.complnum());
                BOOST_CHECK_EQUAL(icondata[offset + ICON_I_INDEX],
                                  c.getI() + 1);
                BOOST_CHECK_EQUAL(icondata[offset + ICON_J_INDEX],
                                  c.getJ() + 1);
                BOOST_CHECK_EQUAL(icondata[offset + ICON_K_INDEX],
                                  c.getK() + 1);
                BOOST_CHECK_EQUAL(icondata[offset + ICON_DIRECTION_INDEX],
                                  c.dir());

                if (c.state() == Opm::WellCompletion::StateEnum::OPEN)
                    BOOST_CHECK_EQUAL(icondata[offset + ICON_STATUS_INDEX],
                                      1);
                else
                    BOOST_CHECK_EQUAL(icondata[offset + ICON_STATUS_INDEX],
                                      -1000);

                if (c.attachedToSegment())
                    BOOST_CHECK_EQUAL(icondata[offset + ICON_SEGMENT_INDEX],
                                      c.segment());
                else
                    BOOST_CHECK_EQUAL(icondata[offset + ICON_SEGMENT_INDEX],
                                      0);

                c_offset += ICONZ;
            }
            w_offset += (ICONZ * ncwmax);
        }

        std::copy(icondata.begin(),
                  icondata.end(),
                  std::ostream_iterator<int>(std::cout, " "));
        std::cout << std::endl;
        BOOST_CHECK_EQUAL(1, 1);// @@@
    }
}
