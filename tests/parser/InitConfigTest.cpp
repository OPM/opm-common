/*
  Copyright 2015 Statoil ASA.

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



#define BOOST_TEST_MODULE InitConfigTests
#include <sys/stat.h>
#include <sys/types.h>

#include <boost/test/unit_test.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/Parser.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/InitConfig.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>

#include <ert/util/test_work_area.hpp>

using namespace Opm;

const std::string& deckStr =
    "RUNSPEC\n"
    "DIMENS\n"
    " 10 10 10 /\n"
    "SOLUTION\n"
    "RESTART\n"
    "BASE 5\n"
    "/\n"
    "GRID\n"
    "START             -- 0 \n"
    "19 JUN 2007 / \n"
    "SCHEDULE\n"
    "SKIPREST \n";


const std::string& deckStr2 =
    "RUNSPEC\n"
    "DIMENS\n"
    " 10 10 10 /\n"
    "SOLUTION\n"
    "GRID\n"
    "START             -- 0 \n"
    "19 JUN 2007 / \n"
    "SCHEDULE\n";


const std::string& deckStr3 =
    "RUNSPEC\n"
    "DIMENS\n"
    " 10 10 10 /\n"
    "SOLUTION\n"
    "RESTART\n"
    "BASE 5 SAVE UNFORMATTED /\n"
    "GRID\n"
    "START             -- 0 \n"
    "19 JUN 2007 / \n"
    "SCHEDULE\n"
    "SKIPREST \n";

const std::string& deckStr4 =
    "RUNSPEC\n"
    "DIMENS\n"
    " 10 10 10 /\n"
    "SOLUTION\n"
    "RESTART\n"
    "BASE 5 /\n"
    "GRID\n"
    "START             -- 0 \n"
    "19 JUN 2007 / \n"
    "SCHEDULE\n";

const std::string& deckStr5 =
  "RUNSPEC\n"
  "DIMENS\n"
  " 10 10 10 /\n"
  "SOLUTION\n"
  "RESTART\n"
  "'/abs/path/BASE' 5 /\n"
  "GRID\n"
  "START             -- 0 \n"
  "19 JUN 2007 / \n"
  "SCHEDULE\n";

const std::string& deckWithEquil =
    "RUNSPEC\n"
    "DIMENS\n"
    " 10 10 10 /\n"
    "EQLDIMS\n"
    "1  100  20  1  1  /\n"
    "SOLUTION\n"
    "RESTART\n"
    "BASE 5\n"
    "/\n"
    "EQUIL\n"
    "2469   382.4   1705.0  0.0    500    0.0     1     1      20 /\n"
    "GRID\n"
    "START             -- 0 \n"
    "19 JUN 2007 / \n"
    "SCHEDULE\n"
    "SKIPREST \n";

static Deck createDeck(const std::string& input) {
    Opm::Parser parser;
    return parser.parseString(input, Opm::ParseContext());
}

BOOST_AUTO_TEST_CASE(InitConfigTest) {

    Deck deck = createDeck(deckStr);
    InitConfig cfg( deck );
    BOOST_CHECK_EQUAL(cfg.restartRequested(), true);
    BOOST_CHECK_EQUAL(cfg.getRestartStep(), 5);
    BOOST_CHECK_EQUAL(cfg.getRestartRootName(), "BASE");

    Deck deck2 = createDeck(deckStr2);
    InitConfig cfg2( deck2 );
    BOOST_CHECK_EQUAL(cfg2.restartRequested(), false);
    BOOST_CHECK_EQUAL(cfg2.getRestartStep(), 0);
    BOOST_CHECK_EQUAL(cfg2.getRestartRootName(), "");

    cfg2.setRestart( "CASE" , 100);
    BOOST_CHECK_EQUAL(cfg2.restartRequested(), true);
    BOOST_CHECK_EQUAL(cfg2.getRestartStep(), 100);
    BOOST_CHECK_EQUAL(cfg2.getRestartRootName(), "CASE");

    Deck deck3 = createDeck(deckStr3);
    BOOST_CHECK_THROW( InitConfig{ deck3 }, std::runtime_error );

    Deck deck4 = createDeck(deckStr4);
    BOOST_CHECK_NO_THROW( InitConfig{ deck4 } );
}

BOOST_AUTO_TEST_CASE( InitConfigWithoutEquil ) {
    auto deck = createDeck( deckStr );
    InitConfig config( deck );

    BOOST_CHECK( !config.hasEquil() );
    BOOST_CHECK_THROW( config.getEquil(), std::runtime_error );
}

BOOST_AUTO_TEST_CASE( InitConfigWithEquil )  {
    auto deck = createDeck( deckWithEquil );
    InitConfig config( deck );

    BOOST_CHECK( config.hasEquil() );
    BOOST_CHECK_NO_THROW( config.getEquil() );
}

BOOST_AUTO_TEST_CASE( EquilOperations ) {
    auto deck = createDeck( deckWithEquil );
    InitConfig config( deck );

    const auto& equil = config.getEquil();

    BOOST_CHECK( !equil.empty() );
    BOOST_CHECK_EQUAL( 1U, equil.size() );

    BOOST_CHECK_NO_THROW( equil.getRecord( 0 ) );
    BOOST_CHECK_THROW( equil.getRecord( 1 ), std::out_of_range );

    const auto& record = equil.getRecord( 0 );
    BOOST_CHECK_CLOSE( 2469, record.datumDepth(), 1e-12 );
    BOOST_CHECK_CLOSE( 382.4 * unit::barsa, record.datumDepthPressure(), 1e-12 );
    BOOST_CHECK_CLOSE( 1705.0, record.waterOilContactDepth(), 1e-12 );
    BOOST_CHECK_CLOSE( 0.0, record.waterOilContactCapillaryPressure(), 1e-12 );
    BOOST_CHECK_CLOSE( 500, record.gasOilContactDepth(), 1e-12 );
    BOOST_CHECK_CLOSE( 0.0, record.gasOilContactCapillaryPressure(), 1e-12 );
    BOOST_CHECK( !record.liveOilInitConstantRs() );
    BOOST_CHECK( !record.wetGasInitConstantRv() );
    BOOST_CHECK_EQUAL( 20, record.initializationTargetAccuracy() );
}

BOOST_AUTO_TEST_CASE(RestartCWD) {
    test_work_area_type * work_area = test_work_area_alloc("restart_cwd");
    mkdir("simulation", 0777);
    {
        std::fstream fs;
        fs.open ("simulation/CASE.DATA", std::fstream::out);
        fs << deckStr4;
        fs.close();

        fs.open("simulation/CASE5.DATA", std::fstream::out);
        fs << deckStr5;
        fs.close();

        fs.open("CASE5.DATA", std::fstream::out);
        fs << deckStr5;
        fs.close();

        fs.open("CWD_CASE.DATA", std::fstream::out);
        fs << deckStr4;
        fs.close();
    }
    Opm::Parser parser;
    {
        Opm::Deck deck = parser.parseFile("simulation/CASE.DATA", Opm::ParseContext());
        Opm::InitConfig init_config(deck);
        BOOST_CHECK_EQUAL(init_config.getRestartRootName(), "simulation/BASE");
    }
    {
      Opm::Deck deck = parser.parseFile("simulation/CASE5.DATA", Opm::ParseContext());
      Opm::InitConfig init_config(deck);
      BOOST_CHECK_EQUAL(init_config.getRestartRootName(), "/abs/path/BASE");
    }
    {
        Opm::Deck deck = parser.parseFile("CWD_CASE.DATA", Opm::ParseContext());
        Opm::InitConfig init_config(deck);
        BOOST_CHECK_EQUAL(init_config.getRestartRootName(), "BASE");
    }
    {
      Opm::Deck deck = parser.parseFile("CASE5.DATA", Opm::ParseContext());
      Opm::InitConfig init_config(deck);
      BOOST_CHECK_EQUAL(init_config.getRestartRootName(), "/abs/path/BASE");
    }

    test_work_area_free(work_area);
}


