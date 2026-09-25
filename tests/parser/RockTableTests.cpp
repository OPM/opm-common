/*
  Copyright (C) 2019 by Norce

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

#define BOOST_TEST_MODULE RockTableTests

#include <boost/test/unit_test.hpp>

#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Parser/InputErrorAction.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/P.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>

// generic table classes
#include <opm/input/eclipse/EclipseState/Tables/SimpleTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

// keyword specific table classes
//#include <opm/input/eclipse/EclipseState/Tables/PvtoTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SwofTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/RockwnodTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/OverburdTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SgofTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PlyadsTable.hpp>
#include <opm/input/eclipse/Schedule/VFPProdTable.hpp>
#include <opm/input/eclipse/Schedule/VFPInjTable.hpp>

#include <array>
#include <iostream>
#include <stdexcept>
#include <tuple>

using namespace Opm;

BOOST_AUTO_TEST_CASE( Rock2d ) {
    const char *input =
            "TABDIMS\n"
            "1 1 /\n"
            "\n"
            "FIELD\n"
            "\n"
            "ROCKCOMP\n"
            " REVERS 2 /\n"
            "\n"
            "OVERBURD\n"
            "1 1.0\n"
            "10 2.0\n"
            " / \n"
            "2 1.1\n"
            "20 2.1\n"
            " / \n"
            "ROCK2D\n"
            " 0.0     0.01\n"
            "         0.02\n"
            "         0.03 / \n"
            "10.0     0.11 \n"
            "         0.12 \n"
            "         0.13/ \n"
            "20.0     0.21\n"
            "         0.22\n"
            "         0.23/\n"
            " / \n"
            "0.7      0.04 \n"
            "         0.05 /\n"
            "10.0     0.14\n"
            "         0.15/\n"
            "20.0     0.24\n"
            "         0.25/\n"
            " / \n"
            "ROCK2DTR\n"
            " 0.0     0.01\n"
            "         0.02\n"
            "         0.03 / \n"
            "10.0     0.11 \n"
            "         0.12 \n"
            "         0.13/ \n"
            "20.0     0.21\n"
            "         0.22\n"
            "         0.23/\n"
            " / \n"
            "0.7      0.04 \n"
            "         0.05 /\n"
            "10.0     0.14\n"
            "         0.15/\n"
            "20.0     0.24\n"
            "         0.25/\n"
            " / \n"
            "ROCKWNOD\n"
            "0.0\n"
            "0.5\n"
            "1.0 /\n"
            "0.9\n"
            "1.0 /\n"
            "\n";

    auto deck = Parser().parseString( input );
    TableManager tables( deck );

    const auto& rock2d = tables.getRock2dTables();
    const auto& rock2dtr = tables.getRock2dtrTables();
    const auto& rockwnod = tables.getRockwnodTables();
    const auto& overburd = tables.getOverburdTables();

    const auto& rec1 = rock2d[0];
    const auto& rec2 = rock2d.at(1);
    const auto& rec1tr = rock2dtr[0];

    const RockwnodTable& rockwnodTable1 = rockwnod.getTable<RockwnodTable>(0);
    const RockwnodTable& rockwnodTable2 = rockwnod.getTable<RockwnodTable>(1);

    const OverburdTable& overburdTable = overburd.getTable<OverburdTable>(0);
    BOOST_CHECK_THROW( std::ignore = rock2d.at(2), std::out_of_range );
    BOOST_REQUIRE_EQUAL(3U, rec1.size());
    BOOST_REQUIRE_EQUAL(3U, rec2.size());
    BOOST_REQUIRE_EQUAL(0.0, rec1.getPressureValue(0));
    BOOST_REQUIRE_EQUAL(0.13, rec1.getPvmultValue(1,2));
    BOOST_REQUIRE_EQUAL(rec1.sizeMultValues(), rockwnodTable1.getSaturationColumn().size());
    BOOST_REQUIRE_EQUAL(rec2.sizeMultValues(), rockwnodTable2.getSaturationColumn().size());
    BOOST_REQUIRE_EQUAL(0.0, rockwnodTable1.getSaturationColumn()[0]);
    // convert from FIELD units to SI units
    BOOST_CHECK_CLOSE(1.0 / 3.28084, overburdTable.getDepthColumn()[0], 1e-4);
    BOOST_CHECK_CLOSE(1.0 * 6894.76, overburdTable.getOverburdenPressureColumn()[0], 1e-4);
    BOOST_REQUIRE_EQUAL(0.0, rec1tr.getPressureValue(0));
    BOOST_REQUIRE_EQUAL(0.13, rec1tr.getTransMultValue(1,2));
}

BOOST_AUTO_TEST_CASE( Rocktabh ) {
    // The full two-table (NTROCC=2) worked example from the OPM Flow reference
    // manual's ROCKTABH section. Each elastic curve is its own record,
    // terminated by its own "/"; an additional, standalone "/" closes each of
    // the two tables (the same NTROCC/"num_tables" convention PVTO/PVTG use,
    // including that the last table's closing "/" is still required in the
    // input even though it - like every table-closing "/" - is not itself an
    // elastic curve). The manual states the resulting deflation curve (first
    // row of each elastic curve) and dilation curve (last row of each elastic
    // curve) for both tables explicitly; both are checked below.
    const char *input =
            "TABDIMS\n"
            "1 /\n"
            "\n"
            "FIELD\n"
            "\n"
            "ROCKCOMP\n"
            " HYSTER 2 /\n"
            "\n"
            "ROCKTABH\n"
            "1500.0  0.9600  0.9800\n"
            "2500.0  0.9700  0.9850\n"
            "3500.0  0.9800  0.9900\n"
            "4500.0  0.9900  0.9950 /\n"
            "2500.0  0.9900  0.9900\n"
            "3500.0  0.9950  0.9950\n"
            "4750.0  0.9980  0.9980 /\n"
            "3500.0  1.0000  1.0000\n"
            "5500.0  1.0100  1.0100 /\n"
            "4500.0  1.0100  1.0100\n"
            "5750.0  1.0200  1.0200 /\n"
            "/\n"
            "1500.0  0.9400  0.9700\n"
            "2250.0  0.9400  0.9700 /\n"
            "2250.0  0.9800  0.9900\n"
            "3250.0  0.9800  0.9900 /\n"
            "3000.0  1.0000  1.0000\n"
            "4250.0  1.0000  1.0000 /\n"
            "4250.0  1.0200  1.0100\n"
            "5250.0  1.0200  1.0100 /\n"
            "/\n";

    auto deck = Parser().parseString( input );
    TableManager tables( deck );

    const auto& rocktabh = tables.getRocktabhTables();
    BOOST_REQUIRE_EQUAL(2U, rocktabh.size());

    // ---- Table number 1 ----
    const auto& table1 = rocktabh[0];
    BOOST_REQUIRE_EQUAL(4U, table1.numElasticCurves());

    // "Here the deflation curve is defined for table number one is: ..."
    const std::array<double, 4> deflationP1  = {1500.0, 2500.0, 3500.0, 4500.0};
    const std::array<double, 4> deflationPv1 = {0.9600, 0.9900, 1.0000, 1.0100};
    for (std::size_t i = 0; i < deflationP1.size(); ++i) {
        BOOST_CHECK_CLOSE(deflationP1[i] * 6894.76, table1.turningPressure(i), 1e-4);
        BOOST_CHECK_EQUAL(deflationPv1[i], table1.turningPoreVolumeMultiplier(i));
    }

    // "And the dilation curve is defined for table number one is: ..." -
    // i.e. the last row of each elastic curve.
    const std::array<double, 4> dilationP1  = {4500.0, 4750.0, 5500.0, 5750.0};
    const std::array<double, 4> dilationPv1 = {0.9900, 0.9980, 1.0100, 1.0200};
    for (std::size_t i = 0; i < dilationP1.size(); ++i) {
        BOOST_CHECK_CLOSE(dilationP1[i] * 6894.76, table1.elasticPressure(i).back(), 1e-4);
        BOOST_CHECK_EQUAL(dilationPv1[i], table1.elasticPoreVolumeMultiplier(i).back());
    }

    // ---- Table number 2 ----
    const auto& table2 = rocktabh[1];
    BOOST_REQUIRE_EQUAL(4U, table2.numElasticCurves());

    // "...and for table number 2: ..."
    const std::array<double, 4> deflationP2  = {1500.0, 2250.0, 3000.0, 4250.0};
    const std::array<double, 4> deflationPv2 = {0.9400, 0.9800, 1.0000, 1.0200};
    for (std::size_t i = 0; i < deflationP2.size(); ++i) {
        BOOST_CHECK_CLOSE(deflationP2[i] * 6894.76, table2.turningPressure(i), 1e-4);
        BOOST_CHECK_EQUAL(deflationPv2[i], table2.turningPoreVolumeMultiplier(i));
    }

    // "...and for table number 2: ..." (dilation curve)
    const std::array<double, 4> dilationP2  = {2250.0, 3250.0, 4250.0, 5250.0};
    const std::array<double, 4> dilationPv2 = {0.9400, 0.9800, 1.0000, 1.0200};
    for (std::size_t i = 0; i < dilationP2.size(); ++i) {
        BOOST_CHECK_CLOSE(dilationP2[i] * 6894.76, table2.elasticPressure(i).back(), 1e-4);
        BOOST_CHECK_EQUAL(dilationPv2[i], table2.elasticPoreVolumeMultiplier(i).back());
    }
}

BOOST_AUTO_TEST_CASE( RocktabhCurveTooShort ) {
    // A one-row elastic curve cannot define a slope and must be rejected.
    const char *input =
            "TABDIMS\n"
            "1 /\n"
            "\n"
            "FIELD\n"
            "\n"
            "ROCKCOMP\n"
            " HYSTER 1 /\n"
            "\n"
            "ROCKTABH\n"
            "1500.0  0.9600  0.9800 /\n"
            "/\n";

    auto deck = Parser().parseString( input );
    BOOST_CHECK_THROW( TableManager tables( deck ), OpmInputError );
}

BOOST_AUTO_TEST_CASE( RocktabhTooFewCurves ) {
    // A single elastic curve (however well-formed) cannot define a pressure
    // reversal/turning point, so it isn't enough for rock compaction
    // hysteresis to be interpolated - at least two are required.
    const char *input =
            "TABDIMS\n"
            "1 /\n"
            "\n"
            "FIELD\n"
            "\n"
            "ROCKCOMP\n"
            " HYSTER 1 /\n"
            "\n"
            "ROCKTABH\n"
            "1500.0  0.9600  0.9800\n"
            "2500.0  0.9700  0.9850 /\n"
            "/\n";

    auto deck = Parser().parseString( input );
    BOOST_CHECK_THROW( TableManager tables( deck ), OpmInputError );
}

BOOST_AUTO_TEST_CASE( RocktabhTurningPressureNotIncreasing ) {
    // The turning pressures (first row of each elastic curve) must themselves
    // increase strictly from one curve to the next - they are the deflation
    // curve, read off in curve order. Here the second curve's turning
    // pressure (1500.0) is not greater than the first's (2500.0).
    const char *input =
            "TABDIMS\n"
            "1 /\n"
            "\n"
            "FIELD\n"
            "\n"
            "ROCKCOMP\n"
            " HYSTER 1 /\n"
            "\n"
            "ROCKTABH\n"
            "2500.0  0.9700  0.9850\n"
            "3500.0  0.9800  0.9900 /\n"
            "1500.0  0.9600  0.9800\n"
            "3500.0  0.9900  0.9900 /\n"
            "/\n";

    auto deck = Parser().parseString( input );
    BOOST_CHECK_THROW( TableManager tables( deck ), OpmInputError );
}

BOOST_AUTO_TEST_CASE( RocktabhRequiresRockcomp ) {
    // ROCKTABH's own record count is normally sized from ROCKCOMP's NTROCC
    // item, so a deck missing ROCKCOMP entirely is rejected already while
    // parsing ROCKTABH - unless the parser is relaxed to warn instead of
    // throwing on that (PARSE_MISSING_DIMS_KEYWORD), in which case it falls
    // back to NTROCC's default of 1 table and parses fine. Emulate that
    // relaxed setting here, and check that TableManager still rejects the
    // deck with a clear error instead of crashing on an unconditional
    // ROCKCOMP lookup.
    const char *input =
            "TABDIMS\n"
            "1 /\n"
            "\n"
            "FIELD\n"
            "\n"
            "ROCKTABH\n"
            "1500.0  0.9600  0.9800\n"
            "2500.0  0.9700  0.9850 /\n"
            "/\n";

    ParseContext parseContext;
    parseContext.update(ParseContext::PARSE_MISSING_DIMS_KEYWORD, InputErrorAction::WARN);

    auto deck = Parser().parseString( input, parseContext );
    BOOST_CHECK_THROW( TableManager tables( deck ), OpmInputError );
}

BOOST_AUTO_TEST_CASE( RocktabhRequiresHysterHysteresis ) {
    // ROCKTABH is Flow's rock compaction hysteresis table, only meaningful
    // when ROCKCOMP's HYSTERESIS item is set to HYSTER. A deck combining
    // ROCKTABH with any other HYSTERESIS mode (here the default, REVERS)
    // must be rejected clearly here, rather than deferred to a confusing
    // "0 ROCKTAB tables provided" failure later in opm-simulators.
    const char *input =
            "TABDIMS\n"
            "1 /\n"
            "\n"
            "FIELD\n"
            "\n"
            "ROCKCOMP\n"
            " REVERS 1 /\n"
            "\n"
            "ROCKTABH\n"
            "1500.0  0.9600  0.9800\n"
            "2500.0  0.9700  0.9850 /\n"
            "/\n";

    auto deck = Parser().parseString( input );
    BOOST_CHECK_THROW( TableManager tables( deck ), OpmInputError );
}

BOOST_AUTO_TEST_CASE( RocktabhDirectionalRejected ) {
    // RKTRMDIR makes ROCKTABH directional (5 columns), which is rejected: the
    // keyword's unit-conversion "dimension" cycle is fixed at 3 entries and
    // cannot be stretched over 5 columns without corrupting later rows.
    const char *input =
            "TABDIMS\n"
            "1 /\n"
            "\n"
            "FIELD\n"
            "\n"
            "ROCKCOMP\n"
            " HYSTER 1 /\n"
            "\n"
            "RKTRMDIR\n"
            "\n"
            "ROCKTABH\n"
            "1000.0 0.9600 0.9650 0.9660 0.9670\n"
            "1500.0 0.9800 0.9850 0.9860 0.9870\n"
            "3000.0 0.9900 0.9950 0.9960 0.9970\n"
            "4500.0 1.0000 1.0000 1.0000 1.0000 /\n"
            "/\n";

    auto deck = Parser().parseString( input );
    BOOST_CHECK_THROW( TableManager tables( deck ), OpmInputError );
}

BOOST_AUTO_TEST_CASE( RocktabhStressOptionRejected ) {
    // ROCKOPTS METHOD=STRESS is rejected for ROCKTABH, the same as it already
    // is for ROCKTAB: Flow's rock compaction evaluator never computes an
    // actual effective-stress quantity, it always looks tables up by pore
    // pressure, so silently accepting STRESS would produce results that look
    // plausible but are physically wrong rather than erroring out. The table
    // itself is a validly-ordered (strictly decreasing) STRESS-convention
    // table, to make sure it's specifically the STRESS option being rejected
    // here, not the turning-pressure ordering.
    const char *input =
            "TABDIMS\n"
            "1 /\n"
            "\n"
            "FIELD\n"
            "\n"
            "ROCKOPTS\n"
            " 'STRESS' /\n"
            "\n"
            "ROCKCOMP\n"
            " HYSTER 1 /\n"
            "\n"
            "ROCKTABH\n"
            "3500.0  0.9800  0.9900\n"
            "2500.0  0.9700  0.9850 /\n"
            "2500.0  0.9900  0.9900\n"
            "1500.0  0.9600  0.9800 /\n"
            "/\n";

    auto deck = Parser().parseString( input );
    BOOST_CHECK_THROW( TableManager tables( deck ), OpmInputError );
}

BOOST_AUTO_TEST_CASE( RocktabAndRocktabhAreMutuallyExclusive ) {
    // ROCKTAB and ROCKTABH are two different rock-compaction models (plain vs.
    // hysteretic) for the same NTROCC regions and must never both appear in a
    // deck. This is enforced by declaring "prohibits" on each keyword against
    // the other, so the Parser itself rejects the combination regardless of
    // which of the two appears first.
    const char *rocktabThenRocktabh =
            "TABDIMS\n"
            "1 /\n"
            "\n"
            "FIELD\n"
            "\n"
            "ROCKCOMP\n"
            " HYSTER 1 /\n"
            "\n"
            "ROCKTAB\n"
            "1500.0  0.9600  0.9800\n"
            "2500.0  0.9700  0.9850 /\n"
            "\n"
            "ROCKTABH\n"
            "1500.0  0.9600  0.9800\n"
            "2500.0  0.9700  0.9850 /\n"
            "/\n";

    BOOST_CHECK_THROW( Parser().parseString( rocktabThenRocktabh ), OpmInputError );

    const char *rocktabhThenRocktab =
            "TABDIMS\n"
            "1 /\n"
            "\n"
            "FIELD\n"
            "\n"
            "ROCKCOMP\n"
            " HYSTER 1 /\n"
            "\n"
            "ROCKTABH\n"
            "1500.0  0.9600  0.9800\n"
            "2500.0  0.9700  0.9850 /\n"
            "/\n"
            "\n"
            "ROCKTAB\n"
            "1500.0  0.9600  0.9800\n"
            "2500.0  0.9700  0.9850 /\n";

    BOOST_CHECK_THROW( Parser().parseString( rocktabhThenRocktab ), OpmInputError );
}
