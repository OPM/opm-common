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

#define BOOST_TEST_MODULE ParserTests
#include <stdexcept>
#include <boost/test/unit_test.hpp>
#include <opm/parser/eclipse/RawDeck/StarToken.hpp>


BOOST_AUTO_TEST_CASE(NoStarThrows) {
    BOOST_REQUIRE_THROW(Opm::StarToken<int> st("Hei...") , std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(InvalidMultiplierThrow) {
    BOOST_REQUIRE_THROW( Opm::StarToken<int> st("X*") , std::invalid_argument);
    BOOST_REQUIRE_THROW( Opm::StarToken<int> st("1.25*") , std::invalid_argument);
    BOOST_REQUIRE_THROW( Opm::StarToken<int> st("-3*") , std::invalid_argument);
    BOOST_REQUIRE_THROW( Opm::StarToken<int> st("0*") , std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(MultiplierCOrrect) {
    Opm::StarToken<int> st1("*");
    Opm::StarToken<int> st2("5*");
    Opm::StarToken<int> st3("54*");
    BOOST_REQUIRE_EQUAL(1U , st1.multiplier());
    BOOST_REQUIRE_EQUAL(5U , st2.multiplier());
    BOOST_REQUIRE_EQUAL(54U , st3.multiplier());
}


BOOST_AUTO_TEST_CASE(NoValueGetValueThrow) {
    Opm::StarToken<int> st1("*");
    Opm::StarToken<int> st2("5*");
    BOOST_CHECK_THROW( st1.value() , std::invalid_argument );
    BOOST_CHECK_THROW( st2.value() , std::invalid_argument );
    BOOST_CHECK_EQUAL( false , st1.hasValue());
    BOOST_CHECK_EQUAL( false , st2.hasValue());
}


BOOST_AUTO_TEST_CASE(IntMalformedValueThrows) {
    BOOST_CHECK_THROW( Opm::StarToken<int> st1("1*10X") , std::invalid_argument);
    BOOST_CHECK_THROW( Opm::StarToken<int> st1("1*X") , std::invalid_argument);
    BOOST_CHECK_THROW( Opm::StarToken<int> st1("1*10.25") , std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(StarNoMultiplierThrows) {
    BOOST_CHECK_THROW( Opm::StarToken<int> st1("*10") , std::invalid_argument);
    BOOST_CHECK_THROW( Opm::StarToken<double> st1("*1.0") , std::invalid_argument);
    BOOST_CHECK_THROW( Opm::StarToken<std::string> st1("*String") , std::invalid_argument);
}


BOOST_AUTO_TEST_CASE(DoubleMalformedValueThrows) {
    BOOST_CHECK_THROW( Opm::StarToken<double> st1("1*10X") , std::invalid_argument);
    BOOST_CHECK_THROW( Opm::StarToken<double> st1("1*X") , std::invalid_argument);
    BOOST_CHECK_THROW( Opm::StarToken<double> st1("1*10.25F") , std::invalid_argument);
}



BOOST_AUTO_TEST_CASE(CorrectIntValue) {
    Opm::StarToken<int> st1("1*10");
    Opm::StarToken<int> st2("5*20");
    BOOST_CHECK_EQUAL( true , st1.hasValue());
    BOOST_CHECK_EQUAL( true , st2.hasValue());

    BOOST_CHECK_EQUAL( 10 , st1.value());
    BOOST_CHECK_EQUAL( 20 , st2.value());
}


BOOST_AUTO_TEST_CASE(CorrectDoubleValue) {
    Opm::StarToken<double> st1("1*10.09");
    Opm::StarToken<double> st2("5*20.13");
    BOOST_CHECK_EQUAL( true , st1.hasValue());
    BOOST_CHECK_EQUAL( true , st2.hasValue());

    BOOST_CHECK_EQUAL( 10.09 , st1.value());
    BOOST_CHECK_EQUAL( 20.13 , st2.value());
}



BOOST_AUTO_TEST_CASE(CorrectStringValue) {
    Opm::StarToken<std::string> st1("1*10.09");
    Opm::StarToken<std::string> st2("5*20.13");
    BOOST_CHECK_EQUAL( true , st1.hasValue());
    BOOST_CHECK_EQUAL( true , st2.hasValue());

    BOOST_CHECK_EQUAL( "10.09" , st1.value());
    BOOST_CHECK_EQUAL( "20.13" , st2.value());
}



BOOST_AUTO_TEST_CASE( ContainsStar_WithStar_ReturnsTrue ) {
    BOOST_CHECK_EQUAL( true , Opm::tokenContainsStar("*") );
    BOOST_CHECK_EQUAL( true , Opm::tokenContainsStar("1*") );
    BOOST_CHECK_EQUAL( true , Opm::tokenContainsStar("1*2") );
    
    BOOST_CHECK_EQUAL( false , Opm::tokenContainsStar("12") );
}

BOOST_AUTO_TEST_CASE( readValueToken_basic_validity_tests ) {
    BOOST_CHECK_THROW( Opm::readValueToken<int>("3.3"), std::invalid_argument );
    BOOST_CHECK_THROW( Opm::readValueToken<double>("truls"), std::invalid_argument );
    BOOST_CHECK_EQUAL( "3.3", Opm::readValueToken<std::string>("3.3") );
    BOOST_CHECK_EQUAL( "OLGA", Opm::readValueToken<std::string>("OLGA") );
}
