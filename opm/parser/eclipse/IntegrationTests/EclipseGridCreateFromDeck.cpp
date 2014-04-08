/*
  Copyright 2014 Statoil ASA.

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

#define BOOST_TEST_MODULE ScheduleIntegrationTests
#include <math.h>

#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>

using namespace Opm;


BOOST_AUTO_TEST_CASE(CreateCPGrid) {
    ParserPtr parser(new Parser());
    boost::filesystem::path scheduleFile("testdata/integration_tests/GRID/CORNERPOINT.DATA");
    DeckPtr deck =  parser->parseFile(scheduleFile.string());
    std::shared_ptr<RUNSPECSection> runspecSection(new RUNSPECSection(deck) );
    std::shared_ptr<GRIDSection> gridSection(new GRIDSection(deck) );
    std::shared_ptr<EclipseGrid> grid(new EclipseGrid( runspecSection , gridSection ));

    BOOST_CHECK_EQUAL( 10 , grid->getNX( ));
    BOOST_CHECK_EQUAL( 10 , grid->getNY( ));
    BOOST_CHECK_EQUAL(  5 , grid->getNZ( ));
    BOOST_CHECK_EQUAL( 500 , grid->getNumActive() );
}


BOOST_AUTO_TEST_CASE(CreateCPActnumGrid) {
    ParserPtr parser(new Parser());
    boost::filesystem::path scheduleFile("testdata/integration_tests/GRID/CORNERPOINT_ACTNUM.DATA");
    DeckPtr deck =  parser->parseFile(scheduleFile.string());
    std::shared_ptr<RUNSPECSection> runspecSection(new RUNSPECSection(deck) );
    std::shared_ptr<GRIDSection> gridSection(new GRIDSection(deck) );
    std::shared_ptr<EclipseGrid> grid(new EclipseGrid( runspecSection , gridSection ));

    BOOST_CHECK_EQUAL( 10 , grid->getNX( ));
    BOOST_CHECK_EQUAL( 10 , grid->getNY( ));
    BOOST_CHECK_EQUAL(  5 , grid->getNZ( ));
    BOOST_CHECK_EQUAL( 100 , grid->getNumActive() );
}

