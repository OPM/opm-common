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
