/*
  Copyright (C) 2013 by Andreas Lauser

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

#define BOOST_TEST_MODULE PvtxTableTests

#include <boost/test/unit_test.hpp>

#include <boost/version.hpp>

// generic table classes
#include <opm/input/eclipse/EclipseState/Tables/SimpleTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PvtxTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

// keyword specific table classes
//#include <opm/input/eclipse/EclipseState/Tables/PvtoTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SwofTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SgofTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PlyadsTable.hpp>

#include <opm/input/eclipse/Schedule/VFPProdTable.hpp>
#include <opm/input/eclipse/Schedule/VFPInjTable.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>
#include <opm/input/eclipse/Units/Units.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/P.hpp>

#include <boost/date_time/posix_time/posix_time.hpp>

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace Opm;

namespace {

std::string casePrefix()
{
#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 < 71
    return boost::unit_test::framework::master_test_suite().argv[2];
#else
    return boost::unit_test::framework::master_test_suite().argv[1];
#endif
}

} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(PvtX)

BOOST_AUTO_TEST_CASE( PvtxNumTables1 ) {
    Parser parser;
    std::filesystem::path deckFile(casePrefix() + "TABLES/PVTX1.DATA");
    auto deck =  parser.parseFile(deckFile.string());
    BOOST_CHECK_EQUAL( PvtxTable::numTables( deck.get<ParserKeywords::PVTO>().back()) , 1);

    auto ranges = PvtxTable::recordRanges( deck.get<ParserKeywords::PVTO>().back() );
    auto range = ranges[0];
    BOOST_CHECK_EQUAL( range.first , 0 );
    BOOST_CHECK_EQUAL( range.second , 2 );
}

BOOST_AUTO_TEST_CASE( PvtxNumTables2 ) {
    Parser parser;
    std::filesystem::path deckFile(casePrefix() + "TABLES/PVTO2.DATA");
    auto deck =  parser.parseFile(deckFile.string());
    BOOST_CHECK_EQUAL( PvtxTable::numTables( deck.get<ParserKeywords::PVTO>().back()) , 3);

    auto ranges = PvtxTable::recordRanges( deck.get<ParserKeywords::PVTO>().back() );
    auto range1 = ranges[0];
    BOOST_CHECK_EQUAL( range1.first , 0 );
    BOOST_CHECK_EQUAL( range1.second , 41 );

    auto range2 = ranges[1];
    BOOST_CHECK_EQUAL( range2.first , 42 );
    BOOST_CHECK_EQUAL( range2.second , 43 );

    auto range3 = ranges[2];
    BOOST_CHECK_EQUAL( range3.first , 44 );
    BOOST_CHECK_EQUAL( range3.second , 46 );
}

BOOST_AUTO_TEST_CASE( PvtxNumTables3 ) {
    const char *deckData =
        "TABDIMS\n"
        "1 2 /\n"
        "\n"
        "PVTO\n"
        " 1 2 3 4"
        "   5 6 7/\n"
        " 8 9 10 11 /\n"
        "/\n"
        "12 13 14 15\n"
        "   16 17 18/\n"
        "19 20 21 22/\n"
        "/\n";

    Opm::Parser parser;
    auto deck = parser.parseString(deckData);

    auto ranges = PvtxTable::recordRanges( deck.get<ParserKeywords::PVTO>().back() );
    BOOST_CHECK_EQUAL( 2 ,ranges.size() );

    auto range1 = ranges[0];
    BOOST_CHECK_EQUAL( range1.first , 0 );
    BOOST_CHECK_EQUAL( range1.second , 2 );

    auto range2 = ranges[1];
    BOOST_CHECK_EQUAL( range2.first , 3 );
    BOOST_CHECK_EQUAL( range2.second , 5 );
}

BOOST_AUTO_TEST_CASE( PVTOSaturatedTable ) {
    Parser parser;
    std::filesystem::path deckFile(casePrefix() + "TABLES/PVTX1.DATA");
    auto deck =  parser.parseFile(deckFile.string());
    Opm::TableManager tables(deck);
    const auto& pvtoTables = tables.getPvtoTables( );
    const auto& pvtoTable = pvtoTables[0];

    const auto& saturatedTable = pvtoTable.getSaturatedTable( );
    BOOST_CHECK_EQUAL( saturatedTable.numColumns( ) , 4 );
    BOOST_CHECK_EQUAL( saturatedTable.numRows( ) , 2 );

    BOOST_CHECK_EQUAL( saturatedTable.get(0 , 0) , 20.59 );
    BOOST_CHECK_EQUAL( saturatedTable.get(0 , 1) , 28.19 );

    {
        int num = 0;
        UnitSystem units( UnitSystem::UnitType::UNIT_TYPE_METRIC );
        for (const auto& table :  pvtoTable) {
            if (num == 0) {
                {
                    const auto& col = table.getColumn(0);
                    BOOST_CHECK_EQUAL( col.size() , 5 );
                    BOOST_CHECK_CLOSE( col[0] , units.to_si( UnitSystem::measure::pressure , 50 ) , 1e-3);
                    BOOST_CHECK_CLOSE( col[4] , units.to_si( UnitSystem::measure::pressure , 150) , 1e-3);
                }
                {
                    const auto& col = table.getColumn(2);
                    BOOST_CHECK_CLOSE( col[0] , units.to_si( UnitSystem::measure::viscosity , 1.180) , 1e-3);
                    BOOST_CHECK_CLOSE( col[4] , units.to_si( UnitSystem::measure::viscosity , 1.453) , 1e-3);
                }
            }

            if (num == 1) {
                const auto& col = table.getColumn(0);
                BOOST_CHECK_EQUAL( col.size() , 5 );
                BOOST_CHECK_CLOSE( col[0] , units.to_si( UnitSystem::measure::pressure , 70  ), 1e-3);
                BOOST_CHECK_CLOSE( col[4] , units.to_si( UnitSystem::measure::pressure , 170 ), 1e-3);
            }
            num++;
        }
        BOOST_CHECK_EQUAL( num , pvtoTable.size() );
    }
}

BOOST_AUTO_TEST_CASE( PVTGSaturatedTable )
{
    // Input PVTG Table
    //
    //     PVTG
    // --
    //      20.00    0.00002448   0.061895     0.01299
    //               0.00001224   0.061810     0.01300
    //               0.00000000   0.061725     0.01300 /
    //      40.00    0.00000628   0.030252     0.01383
    //               0.00000314   0.030249     0.01383
    //               0.00000000   0.030245     0.01383 /
    // /
    //
    // Gets padded to low pressure of 1 bar.  Two extra rows inserted, for
    // p=1 bar and p=pLim=2.063 bar.

    const std::filesystem::path deckFile(casePrefix() + "TABLES/PVTX1.DATA");
    const auto deck = Parser{}.parseFile(deckFile.string());
    const Opm::TableManager tables(deck);
    const auto& pvtgTables = tables.getPvtgTables();
    const auto& pvtgTable = pvtgTables[0];

    const auto& saturatedTable = pvtgTable.getSaturatedTable();
    BOOST_CHECK_EQUAL(saturatedTable.numColumns(), 4);
    BOOST_CHECK_EQUAL(saturatedTable.numRows(), 4);

    // Gas Pressure
    BOOST_CHECK_CLOSE(saturatedTable.get(0, 0),  1.0       *unit::barsa, 1.0e-7);
    BOOST_CHECK_CLOSE(saturatedTable.get(0, 1),  2.06266633*unit::barsa, 2.0e-7);
    BOOST_CHECK_CLOSE(saturatedTable.get(0, 2), 20.0       *unit::barsa, 1.0e-7);
    BOOST_CHECK_CLOSE(saturatedTable.get(0, 3), 40.0       *unit::barsa, 1.0e-7);

    // Rv
    BOOST_CHECK_CLOSE(saturatedTable.get(1, 0), 4.08029736e-05, 1.0e-7);
    BOOST_CHECK_CLOSE(saturatedTable.get(1, 1), 4.08029736e-05, 1.0e-7);
    BOOST_CHECK_CLOSE(saturatedTable.get(1, 2), 2.448e-5      , 1.0e-7);
    BOOST_CHECK_CLOSE(saturatedTable.get(1, 3), 6.28e-6       , 1.0e-7);

    // Gas FVF
    BOOST_CHECK_CLOSE(saturatedTable.get(2, 0), 1.1     , 1.0e-7);
    BOOST_CHECK_CLOSE(saturatedTable.get(2, 1), 1.0     , 1.0e-7);
    BOOST_CHECK_CLOSE(saturatedTable.get(2, 2), 0.061895, 1.0e-7);
    BOOST_CHECK_CLOSE(saturatedTable.get(2, 3), 0.030252, 1.0e-7);

    // Gas Viscosity
    constexpr auto cP = prefix::centi*unit::Poise;
    BOOST_CHECK_CLOSE(saturatedTable.get(3, 0), 4.638198e-3*cP, 1.0e-7);
    BOOST_CHECK_CLOSE(saturatedTable.get(3, 1), 4.638198e-3*cP, 1.0e-7);
    BOOST_CHECK_CLOSE(saturatedTable.get(3, 2), 0.01299*cP    , 1.0e-7);
    BOOST_CHECK_CLOSE(saturatedTable.get(3, 3), 0.01383*cP    , 1.0e-7);
}

BOOST_AUTO_TEST_CASE( PVTWTable ) {
    const std::string input = R"(
RUNSPEC

DIMENS
    10 10 10 /

TABDIMS
    1 2 /

PROPS

PVTW
    3600.0000 1.00341 3.00E-06 0.52341 0.00E-01 /
    3900 1 2.67E-06 0.56341 1.20E-07 /
)";

    auto deck = Parser().parseString( input );
    TableManager tables( deck );

    const auto& pvtw = tables.getPvtwTable();

    const auto& rec1 = pvtw[0];
    const auto& rec2 = pvtw.at(1);

    BOOST_CHECK_THROW( pvtw.at(2), std::out_of_range );

    BOOST_CHECK_CLOSE( 3600.00, rec1.reference_pressure / 1e5, 1e-5 );
    BOOST_CHECK_CLOSE( 1.00341, rec1.volume_factor, 1e-5 );
    BOOST_CHECK_CLOSE( 3.0e-06, rec1.compressibility * 1e5, 1e-5 );
    BOOST_CHECK_CLOSE( 0.52341, rec1.viscosity * 1e3, 1e-5 );
    BOOST_CHECK_CLOSE( 0.0e-01, rec1.viscosibility * 1e5, 1e-5 );

    BOOST_CHECK_CLOSE( 3900,     rec2.reference_pressure / 1e5, 1e-5 );
    BOOST_CHECK_CLOSE( 1.0,      rec2.volume_factor, 1e-5 );
    BOOST_CHECK_CLOSE( 2.67e-06, rec2.compressibility * 1e5, 1e-5 );
    BOOST_CHECK_CLOSE( 0.56341,  rec2.viscosity * 1e3, 1e-5 );
    BOOST_CHECK_CLOSE( 1.20e-07, rec2.viscosibility * 1e5, 1e-5 );
}

BOOST_AUTO_TEST_SUITE_END() // PvtX
