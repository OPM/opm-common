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
    parseMode.update(ParseMode::PARSE_UNKNOWN_KEYWORD , InputError::THROW_EXCEPTION );
    BOOST_CHECK_THROW( parser.parseString( deck1 , parseMode ) , std::invalid_argument);

    parseMode.update(ParseMode::PARSE_UNKNOWN_KEYWORD , InputError::IGNORE );
    BOOST_CHECK_NO_THROW( parser.parseString( deck1 , parseMode ) );

    parseMode.update(ParseMode::PARSE_UNKNOWN_KEYWORD , InputError::THROW_EXCEPTION );
    parseMode.update(ParseMode::PARSE_RANDOM_TEXT , InputError::IGNORE );
    BOOST_CHECK_THROW( parser.parseString( deck2 , parseMode ) , std::invalid_argument);

    parseMode.update(ParseMode::PARSE_UNKNOWN_KEYWORD , InputError::IGNORE );
    parseMode.update(ParseMode::PARSE_RANDOM_TEXT , InputError::IGNORE );
    BOOST_CHECK_NO_THROW( parser.parseString( deck2 , parseMode ) );

    parseMode.update(ParseMode::PARSE_UNKNOWN_KEYWORD , InputError::IGNORE );
    parseMode.update(ParseMode::PARSE_RANDOM_TEXT , InputError::THROW_EXCEPTION );
    BOOST_CHECK_THROW( parser.parseString( deck2 , parseMode ) , std::invalid_argument);

    parseMode.update(ParseMode::PARSE_UNKNOWN_KEYWORD , InputError::IGNORE );
    parseMode.update(ParseMode::PARSE_RANDOM_TEXT , InputError::IGNORE );
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

    parseMode.update( ParseMode::PARSE_MISSING_DIMS_KEYWORD , InputError::THROW_EXCEPTION );
    BOOST_CHECK_THROW( parser.parseString( deck , parseMode ) , std::invalid_argument);

    parseMode.update( ParseMode::PARSE_MISSING_DIMS_KEYWORD , InputError::IGNORE );
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

    parseMode.update( ParseMode::UNSUPPORTED_SCHEDULE_GEO_MODIFIER , InputError::IGNORE );
    BOOST_CHECK_NO_THROW( Schedule( parseMode , grid , deck , ioconfig ));

    parseMode.update( ParseMode::UNSUPPORTED_SCHEDULE_GEO_MODIFIER , InputError::THROW_EXCEPTION );
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

    parseMode.update(ParseMode::PARSE_RANDOM_SLASH , InputError::THROW_EXCEPTION);
    parseMode.update(ParseMode::PARSE_RANDOM_TEXT , InputError::IGNORE);
    BOOST_CHECK_THROW( parser.parseString( deck1 , parseMode ) , std::invalid_argument);
    BOOST_CHECK_THROW( parser.parseString( deck2 , parseMode ) , std::invalid_argument);


    parseMode.update(ParseMode::PARSE_RANDOM_SLASH , InputError::IGNORE);
    parseMode.update(ParseMode::PARSE_RANDOM_TEXT , InputError::THROW_EXCEPTION);
    BOOST_CHECK_NO_THROW( parser.parseString( deck1 , parseMode ) );
    BOOST_CHECK_NO_THROW( parser.parseString( deck2 , parseMode ) );
}



BOOST_AUTO_TEST_CASE(TestCOMPORD) {
    const char * deckString =
        "START\n"
        " 10 'JAN' 2000 /\n"
        "RUNSPEC\n"
        "DIMENS\n"
        "  10 10 10 / \n"
        "SCHEDULE\n"
        "COMPORD\n"
        "  '*'  'INPUT' /\n"
        "/\n";

    ParseMode parseMode;
    Parser parser(true);
    auto deck = parser.parseString( deckString , parseMode );

    std::shared_ptr<EclipseGrid> grid = std::make_shared<EclipseGrid>( deck );
    std::shared_ptr<IOConfig> ioconfig = std::make_shared<IOConfig>( "path" );

    parseMode.update( ParseMode::UNSUPPORTED_COMPORD_TYPE , InputError::IGNORE);
    BOOST_CHECK_NO_THROW( Schedule( parseMode , grid , deck , ioconfig ));

    parseMode.update( ParseMode::UNSUPPORTED_COMPORD_TYPE , InputError::THROW_EXCEPTION);
    BOOST_CHECK_THROW( Schedule( parseMode , grid , deck , ioconfig ), std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(TestInvalidKey) {
    ParseMode parseMode;
    BOOST_CHECK_THROW( parseMode.addKey("KEY*") , std::invalid_argument );
    BOOST_CHECK_THROW( parseMode.addKey("KEY:") , std::invalid_argument );
}


BOOST_AUTO_TEST_CASE(TestNew) {
    ParseMode parseMode;

    BOOST_CHECK_EQUAL( false , parseMode.hasKey("NO"));
    parseMode.addKey("NEW_KEY");
    BOOST_CHECK_EQUAL( true , parseMode.hasKey("NEW_KEY"));
    BOOST_CHECK_THROW( parseMode.get("NO") , std::invalid_argument );
    BOOST_CHECK_EQUAL( parseMode.get("NEW_KEY") , InputError::THROW_EXCEPTION );
    parseMode.addKey("KEY2");
    BOOST_CHECK_EQUAL( parseMode.get("NEW_KEY") , InputError::THROW_EXCEPTION );

    BOOST_CHECK_THROW( parseMode.updateKey("NO" , InputError::IGNORE) , std::invalid_argument);

    parseMode.updateKey("NEW_KEY" , InputError::WARN);
    BOOST_CHECK_EQUAL( parseMode.get("NEW_KEY") , InputError::WARN );

    BOOST_CHECK_NO_THROW( parseMode.update("KEY2:NEW_KEY" , InputError::IGNORE));
    BOOST_CHECK_NO_THROW( parseMode.update("UnknownKey" , InputError::IGNORE));
    BOOST_CHECK_EQUAL( parseMode.get("NEW_KEY") , InputError::IGNORE );
    BOOST_CHECK_EQUAL( parseMode.get("KEY2") , InputError::IGNORE );

    parseMode.addKey("SECRET_KEY");
    parseMode.addKey("NEW_KEY2");
    parseMode.addKey("NEW_KEY3");
    parseMode.update("NEW_KEY*" , InputError::WARN);
    BOOST_CHECK_EQUAL( parseMode.get("NEW_KEY") , InputError::WARN );
    BOOST_CHECK_EQUAL( parseMode.get("NEW_KEY2") , InputError::WARN );
    BOOST_CHECK_EQUAL( parseMode.get("NEW_KEY3") , InputError::WARN );

    parseMode.update( InputError::IGNORE );
    BOOST_CHECK_EQUAL( parseMode.get("NEW_KEY3")   , InputError::IGNORE );
    BOOST_CHECK_EQUAL( parseMode.get("SECRET_KEY") , InputError::IGNORE );


}


BOOST_AUTO_TEST_CASE( test_constructor_with_values) {
    ParseMode parseMode( {{ParseMode::PARSE_RANDOM_SLASH , InputError::IGNORE},
                {"UNSUPPORTED_*" , InputError::WARN},
                    {"UNKNWON-IGNORED" , InputError::WARN}});

    BOOST_CHECK_EQUAL( parseMode.get(ParseMode::PARSE_RANDOM_SLASH) , InputError::IGNORE );
    BOOST_CHECK_EQUAL( parseMode.get(ParseMode::PARSE_RANDOM_TEXT) , InputError::THROW_EXCEPTION );
    BOOST_CHECK_EQUAL( parseMode.get(ParseMode::UNSUPPORTED_INITIAL_THPRES) , InputError::WARN );
    BOOST_CHECK_EQUAL( parseMode.get(ParseMode::UNSUPPORTED_COMPORD_TYPE) , InputError::WARN );
}
