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
#define BOOST_TEST_MODULE ParseModeTests
#include <boost/test/unit_test.hpp>


#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords.hpp>
#include <opm/parser/eclipse/Parser/InputErrorAction.hpp>
#include <opm/parser/eclipse/Parser/ParseMode.hpp>

#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>

using namespace Opm;


BOOST_AUTO_TEST_CASE(TestUnkownKeyword) {
    const char * deck1 =
        "RUNSPEC\n"
        "DIMENS\n"
        "  10 10 10 /n"
        "\n";

   const char * deck2 =
       "1rdomTX\n"
       "RUNSPEC\n"
        "DIMENS\n"
        "  10 10 10 /n"
        "\n";


    ParseMode parseMode;
    Parser parser(false);


    parser.addKeyword<ParserKeywords::DIMENS>();
    parseMode.unknownKeyword = InputError::THROW_EXCEPTION;
    BOOST_CHECK_THROW( parser.parseString( deck1 , parseMode ) , std::invalid_argument);

    parseMode.unknownKeyword = InputError::IGNORE;
    BOOST_CHECK_NO_THROW( parser.parseString( deck1 , parseMode ) );

    parseMode.randomText = InputError::IGNORE;
    parseMode.unknownKeyword = InputError::THROW_EXCEPTION;
    BOOST_CHECK_THROW( parser.parseString( deck2 , parseMode ) , std::invalid_argument);

    parseMode.randomText = InputError::IGNORE;
    parseMode.unknownKeyword = InputError::IGNORE;
    BOOST_CHECK_NO_THROW( parser.parseString( deck2 , parseMode ) );

    parseMode.randomText = InputError::THROW_EXCEPTION;
    parseMode.unknownKeyword = InputError::IGNORE;
    BOOST_CHECK_THROW( parser.parseString( deck2 , parseMode ) , std::invalid_argument);

    parseMode.randomText = InputError::IGNORE;
    parseMode.unknownKeyword = InputError::IGNORE;
    BOOST_CHECK_NO_THROW( parser.parseString( deck2 , parseMode ) );
}


BOOST_AUTO_TEST_CASE( CheckMissingSizeKeyword) {
    const char * deck =
        "SOLUTION\n"
        "EQUIL\n"
        "  10 10 10 10 / \n"
        "\n";


    ParseMode parseMode;
    Parser parser(false);

    parser.addKeyword<ParserKeywords::EQUIL>();
    parser.addKeyword<ParserKeywords::EQLDIMS>();
    parser.addKeyword<ParserKeywords::SOLUTION>();

    parseMode.missingDIMSKeyword = InputError::THROW_EXCEPTION;
    BOOST_CHECK_THROW( parser.parseString( deck , parseMode ) , std::invalid_argument);

    parseMode.missingDIMSKeyword = InputError::IGNORE;
    BOOST_CHECK_NO_THROW( parser.parseString( deck , parseMode ) );
}


BOOST_AUTO_TEST_CASE( CheckUnsoppertedInSCHEDULE ) {
    const char * deckString =
        "START\n"
        " 10 'JAN' 2000 /\n"
        "RUNSPEC\n"
        "DIMENS\n"
        "  10 10 10 / \n"
        "SCHEDULE\n"
        "MULTFLT\n"
        "   'F1' 100 /\n"
        "/\n"
        "\n";

    ParseMode parseMode;
    Parser parser(true);

    auto deck = parser.parseString( deckString , parseMode );
    std::shared_ptr<EclipseGrid> grid = std::make_shared<EclipseGrid>( deck );
    std::shared_ptr<IOConfig> ioconfig = std::make_shared<IOConfig>( "path" );

    parseMode.unsupportedScheduleGeoModifiers = InputError::IGNORE;
    BOOST_CHECK_NO_THROW( Schedule( parseMode , grid , deck , ioconfig ));

    parseMode.unsupportedScheduleGeoModifiers = InputError::THROW_EXCEPTION;
    BOOST_CHECK_THROW( Schedule( parseMode , grid , deck , ioconfig ), std::invalid_argument );
}



BOOST_AUTO_TEST_CASE(TestRandomSlash) {
    const char * deck1 =
        "SCHEDULE\n"
        "TSTEP\n"
        "  10 10 10 /\n"
        "/\n";

    const char * deck2 =
        "SCHEDULE\n"
        "TSTEP\n"
        "  10 10 10 /\n"
        "   /\n";



    ParseMode parseMode;
    Parser parser(false);


    parser.addKeyword<ParserKeywords::TSTEP>();
    parser.addKeyword<ParserKeywords::SCHEDULE>();
    parseMode.randomSlash = InputError::THROW_EXCEPTION;
    parseMode.randomText = InputError::IGNORE;
    BOOST_CHECK_THROW( parser.parseString( deck1 , parseMode ) , std::invalid_argument);
    BOOST_CHECK_THROW( parser.parseString( deck2 , parseMode ) , std::invalid_argument);

    parseMode.randomSlash = InputError::IGNORE;
    parseMode.randomText = InputError::THROW_EXCEPTION;

    BOOST_CHECK_NO_THROW( parser.parseString( deck1 , parseMode ) );
    BOOST_CHECK_NO_THROW( parser.parseString( deck2 , parseMode ) );
}

