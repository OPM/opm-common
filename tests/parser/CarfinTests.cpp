/*
 Copyright (C) 2022 Equinor
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

#define BOOST_TEST_MODULE CarfinManagerTests

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/EclipseState/Grid/Carfin.hpp>
#include <opm/input/eclipse/EclipseState/Grid/CarfinManager.hpp>
#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>

#include <stdexcept>
#include <iostream>

namespace {
    Opm::Carfin::IsActive allActive()
    {
        return [](const std::size_t) { return true; };
    }

    Opm::Carfin::ActiveIdx identityMapping()
    {
        return [](const std::size_t i) { return i; };
    }
}

BOOST_AUTO_TEST_CASE(TestKeywordCarfin) {

   Opm::GridDims gridDims(10,7,6);

    // J2 < J1
    BOOST_CHECK_THROW( Opm::Carfin(gridDims, allActive(), identityMapping(), "LGR",1,1,4,3,2,2,2,2,2), std::invalid_argument);

    // J2 > nyglobal
    BOOST_CHECK_THROW( Opm::Carfin(gridDims, allActive(), identityMapping(), "LGR",1,1,3,8,2,2,2,12,2), std::invalid_argument);

    // J2 == nyglobal
    BOOST_CHECK_THROW( Opm::Carfin(gridDims, allActive(), identityMapping(), "LGR",1,1,3,7,2,2,2,10,2), std::invalid_argument);

    // nlgr % (l2-l1+1) != 0. Only an error once the box is known not to carry
    // an N*FIN distributing the refined cells over its parent cells, which the
    // CARFIN record alone does not say.
    const Opm::Carfin uneven(gridDims, allActive(), identityMapping(), "LGR",1,1,3,4,2,2,2,5,2);
    BOOST_CHECK_THROW( uneven.validateSubdivision(), std::invalid_argument);

    Opm::Carfin graded(gridDims, allActive(), identityMapping(), "LGR",1,1,3,4,2,2,2,5,2);
    graded.setGrading(1, {2, 3}, {});
    BOOST_CHECK_NO_THROW( graded.validateSubdivision() );

    // N*FIN that does not account for every refined cell is rejected.
    BOOST_CHECK_THROW( graded.setGrading(1, {2, 2}, {}), std::invalid_argument);
    // ... as is one entry per refined cell where a parent cell is expected.
    BOOST_CHECK_THROW( graded.setGrading(1, {1, 1, 3}, {}), std::invalid_argument);
    // H*FIN takes one width per refined cell, not per parent cell.
    BOOST_CHECK_THROW( graded.setGrading(1, {2, 3}, {1.0, 2.0}), std::invalid_argument);
    BOOST_CHECK_NO_THROW( graded.setGrading(1, {2, 3}, {1.0, 2.0, 3.0, 1.0, 1.0}) );

}

BOOST_AUTO_TEST_CASE(CreateLgr) {
    Opm::Carfin lgr(Opm::GridDims{ 4, 3, 2 }, allActive(), identityMapping());
    BOOST_CHECK_EQUAL( 24U , lgr.size() );
    BOOST_CHECK( lgr.isGlobal() );
    BOOST_CHECK_EQUAL( 4U , lgr.getDim(0) );
    BOOST_CHECK_EQUAL( 3U , lgr.getDim(1) );
    BOOST_CHECK_EQUAL( 2U , lgr.getDim(2) );

    BOOST_CHECK_THROW( lgr.getDim(5) , std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(CreateCarfinManager) {
    Opm::GridDims gridDims(10,10,10);
    Opm::CarfinManager carfinManager(gridDims, allActive(), identityMapping());
    Opm::Carfin lgr(gridDims, allActive(), identityMapping());

    BOOST_CHECK( lgr.equal( carfinManager.getActiveCarfin()) );
}

BOOST_AUTO_TEST_CASE(TestInputCarfin) {
    Opm::GridDims gridDims(10,10,10);
    Opm::CarfinManager carfinManager(gridDims, allActive(), identityMapping());
    Opm::Carfin inputLgr( gridDims, allActive(), identityMapping(), "LGR", 1,4,1,4,1,4,4,4,4);
    Opm::Carfin globalLgr( gridDims, allActive(), identityMapping() );

    carfinManager.setInputCarfin("LGR", 1,4,1,4,1,4,4,4,4);
    BOOST_CHECK( inputLgr.equal( carfinManager.getActiveCarfin()) );
    carfinManager.endSection();
    BOOST_CHECK( carfinManager.getActiveCarfin().equal(globalLgr));
}
