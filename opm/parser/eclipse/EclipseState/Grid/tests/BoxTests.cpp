/*
  Copyright 2013 Statoil ASA.

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

#include <stdexcept>
#include <iostream>
#include <boost/filesystem.hpp>

#define BOOST_TEST_MODULE EclipseGridTests
#include <boost/test/unit_test.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <opm/parser/eclipse/EclipseState/Grid/Box.hpp>

BOOST_AUTO_TEST_CASE(CreateBox) {
    BOOST_CHECK_THROW( new Opm::Box(-1,0,0) , std::invalid_argument);
    BOOST_CHECK_THROW( new Opm::Box(10,0,10) , std::invalid_argument);
    BOOST_CHECK_THROW( new Opm::Box(10,10,-1) , std::invalid_argument);

    Opm::Box box(4,3,2);
    BOOST_CHECK_EQUAL( 24U , box.size() );
    BOOST_CHECK( box.isGlobal() );
    BOOST_CHECK_EQUAL( 4 , box.getDim(0) );
    BOOST_CHECK_EQUAL( 3 , box.getDim(1) );
    BOOST_CHECK_EQUAL( 2 , box.getDim(2) );
    
    BOOST_CHECK_THROW( box.getDim(5) , std::invalid_argument);


    {
        size_t i,j,k;
        const std::vector<size_t>& indexList = box.getIndexList();

        for (k=0; k < box.getDim(2); k++) {
            for (j=0; j < box.getDim(1); j++) {
                for (i=0; i < box.getDim(0); i++) {
                    size_t g = i + j*box.getDim(0) + k*box.getDim(0)*box.getDim(1);
                    BOOST_CHECK_EQUAL( indexList[g] , g);
                    
                }
            }
        }
    }   
    
}
