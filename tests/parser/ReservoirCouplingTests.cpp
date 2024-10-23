/*
  Copyright 2024 Equinor ASA.

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
#include "config.h"
#define BOOST_TEST_MODULE ReservoirCouplingTests

#include <boost/test/unit_test.hpp>
#include <opm/input/eclipse/Schedule/ResCoup/ReservoirCouplingInfo.hpp>
#include <opm/input/eclipse/Schedule/ResCoup/GrupSlav.hpp>
#include <opm/input/eclipse/Schedule/ResCoup/MasterGroup.hpp>
#include <opm/input/eclipse/Schedule/ResCoup/Slaves.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/input/eclipse/Python/Python.hpp>
#include <opm/input/eclipse/Units/Units.hpp>
#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/StreamLog.hpp>

#include <sstream>
#include <string>

using namespace Opm;
namespace {
Schedule makeSchedule(const std::string& schedule_string, bool slave_mode) {
    Parser parser;
    auto python = std::make_shared<Python>();
    Deck deck = parser.parseString(schedule_string);
    EclipseGrid grid(10,10,10);
    TableManager table ( deck );
    FieldPropsManager fp( deck, Phases{true, true, true}, grid, table);
    Runspec runspec (deck);
    Schedule schedule(
        deck, grid , fp, runspec, python, /*lowActionParsingStrictness=*/false, slave_mode
    );
    return schedule;
}

void addStringLogger(std::ostringstream& stream_buffer) {
    auto string_logger = std::make_shared<StreamLog>(stream_buffer, Log::DefaultMessageTypes);
    OpmLog::addBackend("MYLOGGER", string_logger);
}

void assertRaisesInputErrorException(const std::string& schedule_string, bool slave_mode, const std::string& exception_string) {
    BOOST_CHECK_THROW(makeSchedule(schedule_string, slave_mode), Opm::OpmInputError);
    try {
        // Now that we know that it will throw the specific exception Opm::OpmInputError,
        //  we can check that the exception message is correct
        makeSchedule(schedule_string, slave_mode);
    }
    catch (const Opm::OpmInputError& e) {
        BOOST_CHECK_EQUAL(std::string(e.what()), exception_string);
    }
}

void checkLastLineStringBuffer(
    std::ostringstream& stream_buffer,
    const std::string& expected,
    size_t offset  // Line offset from the end of the string
) {
    std::string output = stream_buffer.str();
    std::istringstream iss(output);
    std::vector<std::string> lines;
    std::string line;

    // Split the output into lines
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }
    // Calculate the index of the desired line using the offset
    if (lines.size() > offset) {
        auto last_line =  lines[lines.size() - 1 - offset];
        BOOST_CHECK_EQUAL(last_line, expected);
    }
    else {
        BOOST_FAIL("Not enough lines in the output buffer");
    }
}

std::string getMinimumMasterTimeStepDeckString(const std::string &end_of_deck_string)
{
    std::string prefix = R"(
SCHEDULE

GRUPTREE
 'PLAT-A' 'FIELD' /

 'MOD1'   'PLAT-A' /

 'B1_M'   'MOD1' /
 'D1_M'   'MOD1' /
 'C1_M'   'MOD1' /

 'E1_M'   'PLAT-A' /
/

SLAVES
  'RES-1'  'RC-01_MOD1_PRED'   1*  '../mod1'  4 /
  'RES-2'  'RC-01_MOD2_PRED'   1*  '../mod2'  1 /
/

GRUPMAST
  'D1_M' 'RES-1'  'MANI-D'  1*  /
  'B1_M' 'RES-1'  'MANI-B'  1*  /
  'C1_M' 'RES-1'  'MANI-C'  1*  /
  'E1_M' 'RES-2'  'E1'  1*  /
/
)";
    return prefix + end_of_deck_string;
}

std::string getCouplingFileDeckString(const std::string &end_of_deck_string)
{
    return getMinimumMasterTimeStepDeckString(end_of_deck_string);
}

void removeStringLogger() {
    OpmLog::removeBackend("MYLOGGER");
}

}  // namespace

// ----------------------------------------------
// Testing SLAVES keyword (sorted alphabetically)
// ----------------------------------------------
BOOST_AUTO_TEST_SUITE(SlaveTests)

BOOST_AUTO_TEST_CASE(FAIL_NEGATIVE_NUMPROCS) {
    std::string deck_string = R"(
SCHEDULE
SLAVES
  'RES-1'  'RC-01_MOD1_PRED'   1*  '../mod1'  -4 /
  'RES-2'  'RC-01_MOD2_PRED'   1*  '../mod2'  1 /
/

)";

    assertRaisesInputErrorException(deck_string, /*slave_mode=*/false, /*exception_string=*/ "Problem with keyword SLAVES\nIn <memory string> line 3\nNumber of processors must be positive. Got: -4.");
}

BOOST_AUTO_TEST_CASE(SYNTAX_OK) {
    std::string deck_string = R"(

SCHEDULE
SLAVES
  'RES-1'  'RC-01_MOD1_PRED'   1*  '../mod1'  4 /
  'RES-2'  'RC-01_MOD2_PRED'   1*  '../mod2'  1 /
/

)";

    const auto& schedule = makeSchedule(deck_string, /*slave_mode=*/false);
    const auto& rescoup = schedule[0].rescoup();
    BOOST_CHECK(rescoup.hasSlave("RES-1"));
    auto slave = rescoup.slave("RES-1");
    BOOST_CHECK_EQUAL(slave.name(), "RES-1");
    BOOST_CHECK_EQUAL(slave.dataFilename(), "RC-01_MOD1_PRED");
    BOOST_CHECK_EQUAL(slave.directoryPath(), "../mod1");
    BOOST_CHECK_EQUAL(slave.numprocs(), 4);
}


BOOST_AUTO_TEST_CASE(WARN_DUPLICATE_NAME) {
    std::string deck_string = R"(
SCHEDULE
SLAVES
  'RES-1'  'RC-01_MOD1_PRED'   1*  '../mod1'  4 /
  'RES-1'  'RC-01_MOD2_PRED'   1*  '../mod2'  1 /
/

)";
    std::ostringstream stream_buffer;
    addStringLogger(stream_buffer);
    const auto& schedule = makeSchedule(deck_string, /*slave_mode=*/false);
    checkLastLineStringBuffer(
        stream_buffer,
        "Slave reservoir 'RES-1' already defined. Redefining",
        /*offset=*/0
    );
    removeStringLogger();
}
BOOST_AUTO_TEST_SUITE_END()

// ------------------------------------------------
// Testing GRUPMAST keyword (sorted alphabetically)
// ------------------------------------------------

BOOST_AUTO_TEST_SUITE(GrupMastTests)

BOOST_AUTO_TEST_CASE(FAIL_MISSING_MASTER_GROUP) {
        std::string deck_string = R"(
SCHEDULE

SLAVES
  'RES-1'  'RC-01_MOD1_PRED'   1*  '../mod1'  4 /
  'RES-2'  'RC-01_MOD2_PRED'   1*  '../mod2'  1 /
/

GRUPMAST
  'D1_M' 'RES-1'  'MANI-D'  1*  /
  'B1_M' 'RES-1'  'MANI-B'  1*  /
  'C1_M' 'RES-1'  'MANI-C'  1*  /
  'E1_M' 'RES-2'  'E1'  1*  /
/
)";

    assertRaisesInputErrorException(deck_string, /*slave_mode=*/false, /*exception_string=*/"Problem with keyword GRUPMAST\nIn <memory string> line 9\nGroup 'D1_M': Not defined. Master groups should be defined in advance by using GRUPTREE before referenced in GRUPMAST.");
}

BOOST_AUTO_TEST_CASE(FAIL_MISSING_SLAVE) {
        std::string deck_string = R"(

SCHEDULE

GRUPTREE
 'PLAT-A' 'FIELD' /

 'MOD1'   'PLAT-A' /

 'B1_M'   'MOD1' /
 'D1_M'   'MOD1' /
 'C1_M'   'MOD1' /

 'E1_M'   'PLAT-A' /
/

GRUPMAST
  'D1_M' 'RES-1'  'MANI-D'  1*  /
  'B1_M' 'RES-1'  'MANI-B'  1*  /
  'C1_M' 'RES-1'  'MANI-C'  1*  /
  'E1_M' 'RES-2'  'E1'  1*  /
/
)";

    assertRaisesInputErrorException(deck_string, /*slave_mode=*/false, /*exception_string=*/"Problem with keyword GRUPMAST\nIn <memory string> line 17\nSlave reservoir 'RES-1': Not defined. Slave reservoirs should be defined in advance by using SLAVES before referenced in GRUPMAST.");
}

BOOST_AUTO_TEST_CASE(FAIL_SUBORDINATE_GROUPS) {
        std::string deck_string = R"(
SCHEDULE

SLAVES
  'RES-1'  'RC-01_MOD1_PRED'   1*  '../mod1'  4 /
  'RES-2'  'RC-01_MOD2_PRED'   1*  '../mod2'  1 /
/

GRUPTREE
 'PLAT-A' 'FIELD' /

 'MOD1'   'PLAT-A' /

 'B1_M'   'MOD1' /
 'D1_M'   'MOD1' /
 'C1_M'   'MOD1' /

 'E1_M'   'PLAT-A' /
/

GRUPMAST
  'FIELD' 'RES-1'  'MANI-D'  1*  /
  'B1_M' 'RES-1'  'MANI-B'  1*  /
  'C1_M' 'RES-1'  'MANI-C'  1*  /
  'E1_M' 'RES-2'  'E1'  1*  /
/
)";

    assertRaisesInputErrorException(deck_string, /*slave_mode=*/false, /*exception_string=*/"Problem with keyword GRUPMAST\nIn <memory string> line 21\nGroup 'FIELD' has subgroups: A master group cannot contain any wells or subordinate groups.");
}

BOOST_AUTO_TEST_CASE(FAIL_SUBORDINATE_WELLS) {
        std::string deck_string = R"(

SCHEDULE

SLAVES
  'RES-1'  'RC-01_MOD1_PRED'   1*  '../mod1'  4 /
  'RES-2'  'RC-01_MOD2_PRED'   1*  '../mod2'  1 /
/

GRUPTREE
 'PLAT-A' 'FIELD' /

 'MOD1'   'PLAT-A' /

 'B1_M'   'MOD1' /
 'D1_M'   'MOD1' /
 'C1_M'   'MOD1' /

 'E1_M'   'PLAT-A' /
/

WELSPECS
 'C-4H' 'D1_M' 11 17 1* 'GAS' /
/

GRUPMAST
  'D1_M' 'RES-1'  'MANI-D'  1*  /
  'B1_M' 'RES-1'  'MANI-B'  1*  /
  'C1_M' 'RES-1'  'MANI-C'  1*  /
  'E1_M' 'RES-2'  'E1'  1*  /
/
)";

    assertRaisesInputErrorException(deck_string, /*slave_mode=*/false, /*exception_string=*/"Problem with keyword GRUPMAST\nIn <memory string> line 26\nGroup 'D1_M' has wells: A master group cannot contain any wells or subordinate groups.");
}

BOOST_AUTO_TEST_CASE(SYNTAX_OK) {
    std::string deck_string = R"(

SCHEDULE
SLAVES
  'RES-1'  'RC-01_MOD1_PRED'   1*  '../mod1'  4 /
  'RES-2'  'RC-01_MOD2_PRED'   1*  '../mod2'  1 /
/

GRUPTREE
 'PLAT-A' 'FIELD' /

 'MOD1'   'PLAT-A' /

 'B1_M'   'MOD1' /
 'D1_M'   'MOD1' /
 'C1_M'   'MOD1' /

 'E1_M'   'PLAT-A' /
/

GRUPMAST
  'D1_M' 'RES-1'  'MANI-D'  1*  /
  'B1_M' 'RES-1'  'MANI-B'  1*  /
  'C1_M' 'RES-1'  'MANI-C'  1*  /
  'E1_M' 'RES-2'  'E1'  1*  /
/
)";

    const auto& schedule = makeSchedule(deck_string, /*slave_mode=*/false);
    const auto& rescoup = schedule[0].rescoup();
    BOOST_CHECK(rescoup.hasMasterGroup("D1_M"));
    auto master_group = rescoup.masterGroup("D1_M");
    BOOST_CHECK_EQUAL(master_group.name(), "D1_M");
    BOOST_CHECK_EQUAL(master_group.slaveName(), "RES-1");
    BOOST_CHECK_EQUAL(master_group.slaveGroupName(), "MANI-D");
    BOOST_CHECK_EQUAL(master_group.flowLimitFraction(), 1e+20);
}

BOOST_AUTO_TEST_SUITE_END()

// ------------------------------------------------
// Testing GRUPSLAV keyword (sorted alphabetically)
// ------------------------------------------------

BOOST_AUTO_TEST_SUITE(GrupSlavTests)

BOOST_AUTO_TEST_CASE(DEFAULT_APPLIED) {
    std::string deck_string = R"(

SCHEDULE
GRUPTREE
 'PLAT-A'  'FIELD' /  
 'MANI-B'  'PLAT-A'  /
 'MANI-D'  'PLAT-A'  /
 'MANI-C'  'PLAT-A'  /
/

GRUPSLAV
 'MANI-D'  1* /
 'MANI-B'  'B1_M' /
 'MANI-C'  'C1_M' /
/
)";

    const auto& schedule = makeSchedule(deck_string, /*slave_mode=*/true);
    const auto& rescoup = schedule[0].rescoup();
    BOOST_CHECK(rescoup.hasGrupSlav("MANI-D"));
    auto grup_slav = rescoup.grupSlav("MANI-D");
    BOOST_CHECK_EQUAL(grup_slav.name(), "MANI-D");
    BOOST_CHECK_EQUAL(grup_slav.masterGroupName(), "MANI-D");
}

BOOST_AUTO_TEST_CASE(FAIL_MISSING_GROUP) {
    std::string deck_string = R"(

SCHEDULE
GRUPSLAV
 'MANI-D'  'D1_M' /
 'MANI-B'  'B1_M' /
 'MANI-C'  'C1_M' /
/

)";
    assertRaisesInputErrorException(deck_string, /*slave_mode=*/true, /*exception_string=*/"Problem with keyword GRUPSLAV\nIn <memory string> line 4\nGroup 'MANI-D': Not defined. Slave groups should be defined in advance by using GRUPTREE or WELSPECS before referenced in GRUPSLAV.");
}

BOOST_AUTO_TEST_CASE(SYNTAX_OK) {
    std::string deck_string = R"(

SCHEDULE
GRUPTREE
 'PLAT-A'  'FIELD' /  
 'MANI-B'  'PLAT-A'  /
 'MANI-D'  'PLAT-A'  /
 'MANI-C'  'PLAT-A'  /
/

GRUPSLAV
 'MANI-D'  'D1_M' /
 'MANI-B'  'B1_M' /
 'MANI-C'  'C1_M' /
/
)";

    const auto& schedule = makeSchedule(deck_string, /*slave_mode=*/true);
    const auto& rescoup = schedule[0].rescoup();
    BOOST_CHECK(rescoup.hasGrupSlav("MANI-D"));
    auto grup_slav = rescoup.grupSlav("MANI-D");
    BOOST_CHECK_EQUAL(grup_slav.name(), "MANI-D");
    BOOST_CHECK_EQUAL(grup_slav.masterGroupName(), "D1_M");
    BOOST_CHECK_EQUAL(grup_slav.oilProdFlag(), Opm::ReservoirCoupling::GrupSlav::FilterFlag::MAST);
    BOOST_CHECK_EQUAL(grup_slav.liquidProdFlag(), Opm::ReservoirCoupling::GrupSlav::FilterFlag::MAST);
    BOOST_CHECK_EQUAL(grup_slav.gasProdFlag(), Opm::ReservoirCoupling::GrupSlav::FilterFlag::MAST);
    BOOST_CHECK_EQUAL(grup_slav.fluidVolumeProdFlag(), Opm::ReservoirCoupling::GrupSlav::FilterFlag::MAST);
    BOOST_CHECK_EQUAL(grup_slav.oilInjFlag(), Opm::ReservoirCoupling::GrupSlav::FilterFlag::MAST);
    BOOST_CHECK_EQUAL(grup_slav.waterInjFlag(), Opm::ReservoirCoupling::GrupSlav::FilterFlag::MAST);
    BOOST_CHECK_EQUAL(grup_slav.gasInjFlag(), Opm::ReservoirCoupling::GrupSlav::FilterFlag::MAST);
}

BOOST_AUTO_TEST_SUITE_END()

// ------------------------------------------------
// Testing RCMASTS keyword (sorted alphabetically)
// ------------------------------------------------

BOOST_AUTO_TEST_SUITE(MinimumMasterTimeStep)

BOOST_AUTO_TEST_CASE(DEFAULT_APPLIED1) {
    std::string end_of_deck_string = R"(
TUNING
-- TSINIT TSMAXZ TSMINZ
    *       *      0.1   /
/
/
)";
    std::string deck_string = getMinimumMasterTimeStepDeckString(end_of_deck_string);
    const auto& schedule = makeSchedule(deck_string, /*slave_mode=*/false);
    const auto& rescoup = schedule[0].rescoup();
    BOOST_CHECK(rescoup.masterMinTimeStep() == 0.0); // Default value when RCMASTS is not given

}

BOOST_AUTO_TEST_CASE(DEFAULT_APPLIED2) {
    std::string end_of_deck_string = R"(
TUNING
-- TSINIT TSMAXZ TSMINZ
    *       *      0.1   /
/
/

RCMASTS
  * /
)";
    std::string deck_string = getMinimumMasterTimeStepDeckString(end_of_deck_string);
    const auto& schedule = makeSchedule(deck_string, /*slave_mode=*/false);
    const auto& rescoup = schedule[0].rescoup();
    const auto& tuning = schedule[0].tuning();
    // Default value when RCMASTS is given but no value is provided
    BOOST_CHECK(rescoup.masterMinTimeStep() == tuning.TSMINZ);

}

BOOST_AUTO_TEST_CASE(VALUE_PROVIDED) {
    std::string end_of_deck_string = R"(
TUNING
-- TSINIT TSMAXZ TSMINZ
    *       *      0.1   /
/
/

RCMASTS
  0.0001 /
)";
    std::string deck_string = getMinimumMasterTimeStepDeckString(end_of_deck_string);
    const auto& schedule = makeSchedule(deck_string, /*slave_mode=*/false);
    const auto& rescoup = schedule[0].rescoup();
    // NOTE: Metric unit system is used by default, to time is in days
    BOOST_CHECK(rescoup.masterMinTimeStep() == (0.0001 * Opm::unit::day));
}

BOOST_AUTO_TEST_CASE(NEGATIVE_VALUE_PROVIDED) {
    std::string end_of_deck_string = R"(
TUNING
-- TSINIT TSMAXZ TSMINZ
    *       *      0.1   /
/
/

RCMASTS
  -0.1 /
)";
    std::string deck_string = getMinimumMasterTimeStepDeckString(end_of_deck_string);
    assertRaisesInputErrorException(
        deck_string,
        /*slave_mode=*/false,
        /*exception_string=*/"Problem with keyword RCMASTS\nIn <memory string> line 34\nNegative value for RCMASTS is not allowed."
    );

}

BOOST_AUTO_TEST_SUITE_END()

// ------------------------------------------------
// Testing DUMPCUPL keyword (sorted alphabetically)
// ------------------------------------------------

BOOST_AUTO_TEST_SUITE(DumpCouplingFile)

BOOST_AUTO_TEST_CASE(FORMATTED_FILE) {
    std::string end_of_deck_string = R"(
DUMPCUPL
  F /
)";
    std::string deck_string = getCouplingFileDeckString(end_of_deck_string);
    const auto& schedule = makeSchedule(deck_string, /*slave_mode=*/false);
    const auto& rescoup = schedule[0].rescoup();
    BOOST_CHECK(rescoup.couplingFileFlag() ==
                Opm::ReservoirCoupling::CouplingInfo::CouplingFileFlag::FORMATTED);

}

BOOST_AUTO_TEST_CASE(BAD_VALUE) {
    std::string end_of_deck_string = R"(
DUMPCUPL
  S /
)";
    std::string deck_string = getCouplingFileDeckString(end_of_deck_string);
    assertRaisesInputErrorException(
        deck_string,
        /*slave_mode=*/false,
        /*exception_string=*/"Problem with keyword DUMPCUPL\nIn <memory string> line 28\nInvalid DUMPCUPL value: S"
    );
}

BOOST_AUTO_TEST_CASE(DEFAULT_NOT_ALLOWED) {
    std::string end_of_deck_string = R"(
DUMPCUPL
  * /
)";
    std::string deck_string = getCouplingFileDeckString(end_of_deck_string);
    assertRaisesInputErrorException(
        deck_string,
        /*slave_mode=*/false,
        /*exception_string=*/"Problem with keyword DUMPCUPL\nIn <memory string> line 28\nDUMPCUPL keyword cannot be defaulted."
    );
}

BOOST_AUTO_TEST_SUITE_END()
