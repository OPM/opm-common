/*
   +   Copyright 2019 Equinor ASA.
   +
   +   This file is part of the Open Porous Media project (OPM).
   +
   +   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
   +   the Free Software Foundation, either version 3 of the License, or
   +   (at your option) any later version.
   +
   +   OPM is distributed in the hope that it will be useful,
   +   but WITHOUT ANY WARRANTY; without even the implied warranty of
   +   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   +   GNU General Public License for more details.
   +
   +   You should have received a copy of the GNU General Public License
   +   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   +   */


#include "config.h"
#include <iostream>
#include <stdio.h>
#include <iomanip>    
#include <math.h>  
#include <algorithm>

#include <examples/test_util/EclFile.hpp>

#define BOOST_TEST_MODULE Test EclFile
#include <boost/test/unit_test.hpp>



BOOST_AUTO_TEST_CASE(TestConstructor){

    EclFile file1("SPE1CASE1.SMSPEC");
    
    BOOST_CHECK_THROW(EclFile file1("XXXX.INIT"), std::runtime_error );

}


