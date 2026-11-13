/*
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

#include <config.h>

#define BOOST_TEST_MODULE InactivePhaseTests

#include <boost/test/unit_test.hpp>

#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquifers.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Phase.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>

#include <memory>
#include <string>

namespace {

// Active phases in the run: GAS and WATER only.  OIL is inactive.
constexpr auto gas_water_runspec = R"(RUNSPEC
GAS
WATER
)";

// Active phases in the run: OIL and WATER only.  GAS is inactive.
constexpr auto oil_water_runspec = R"(RUNSPEC
OIL
WATER
)";

// Active phase in the run: GAS only.  Neither OIL nor WATER is active.
constexpr auto gas_runspec = R"(RUNSPEC
GAS
)";

Opm::Schedule makeSchedule(const std::string& runspec,
                           const std::string& schedule)
{
    const auto deck = Opm::Parser{}.parseString(runspec + "SCHEDULE\n" + schedule);

    Opm::EclipseGrid grid(10, 10, 10);
    const Opm::TableManager table(deck);
    const Opm::Runspec rspec(deck);
    const Opm::FieldPropsManager fp(deck, rspec.phases(), grid, table);

    return { deck, grid, fp, Opm::NumericalAquifers{}, rspec,
             std::make_shared<Opm::Python>() };
}

// Deck with two local grid refinements, LGR1 and LGR2, on a 3x3x1 grid.
Opm::Schedule makeLgrSchedule(const std::string& phases,
                              const std::string& schedule)
{
    const auto deck = Opm::Parser{}.parseString("RUNSPEC\n" + phases + R"(DIMENS
3 3 1 /
GRID
CARFIN
'LGR1'  1  1  1  1  1  1  3  3  1 /
ENDFIN
CARFIN
'LGR2'  3  3  3  3  1  1  3  3  1 /
ENDFIN
DX
   9*1000 /
DY
   9*1000 /
DZ
   9*50 /
TOPS
   9*8325 /
PORO
   9*0.3 /
PERMX
   9*500 /
PERMY
   9*200 /
PERMZ
   9*200 /
SCHEDULE
)" + schedule);

    const Opm::EclipseState es { deck };
    return { deck, es, std::make_shared<Opm::Python>() };
}

// Expect an OpmInputError whose message contains 'fragment'.
void expectInputError(const std::string& runspec,
                      const std::string& schedule,
                      const std::string& fragment)
{
    try {
        makeSchedule(runspec, schedule);
        BOOST_ERROR("Schedule construction did not throw; expected message containing: "
                    << fragment);
    }
    catch (const Opm::OpmInputError& e) {
        const auto msg = std::string { e.what() };
        BOOST_CHECK_MESSAGE(msg.find(fragment) != std::string::npos,
                            "Message \"" << msg << "\" does not contain \""
                            << fragment << '"');
    }
    catch (const std::exception& e) {
        BOOST_ERROR("Expected OpmInputError, got other exception: " << e.what());
    }
}

} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(WELSPECS_PreferredPhase)

BOOST_AUTO_TEST_CASE(Inactive_Oil_Is_Rejected)
{
    // The scenario from the original bug report: OIL preferred phase in a
    // GAS/WATER run.
    expectInputError(gas_water_runspec, R"(WELSPECS
  'PROBE' 'FIELD' 4 1 2000.0 'OIL' /
/
)", "OIL is not active in the run");
}

BOOST_AUTO_TEST_CASE(Inactive_Gas_Is_Rejected)
{
    expectInputError(oil_water_runspec, R"(WELSPECS
  'PROBE' 'FIELD' 4 1 2000.0 'GAS' /
/
)", "GAS is not active in the run");
}

BOOST_AUTO_TEST_CASE(Error_Names_Well_And_Keyword)
{
    expectInputError(gas_water_runspec, R"(WELSPECS
  'PROBE' 'FIELD' 4 1 2000.0 'OIL' /
/
)", "PROBE");

    expectInputError(gas_water_runspec, R"(WELSPECS
  'PROBE' 'FIELD' 4 1 2000.0 'OIL' /
/
)", "WELSPECS");
}

BOOST_AUTO_TEST_CASE(Active_Phases_Are_Accepted)
{
    BOOST_CHECK_NO_THROW(makeSchedule(gas_water_runspec, R"(WELSPECS
  'PG' 'FIELD' 4 1 2000.0 'GAS' /
  'PW' 'FIELD' 5 1 2000.0 'WATER' /
/
)"));

    const auto sched = makeSchedule(gas_water_runspec, R"(WELSPECS
  'PG' 'FIELD' 4 1 2000.0 'GAS' /
  'PW' 'FIELD' 5 1 2000.0 'WATER' /
/
)");

    BOOST_CHECK(sched.getWell("PG", 0).getPreferredPhase() == Opm::Phase::GAS);
    BOOST_CHECK(sched.getWell("PW", 0).getPreferredPhase() == Opm::Phase::WATER);
}

BOOST_AUTO_TEST_CASE(Second_Inactive_Record_Is_Rejected)
{
    // First record is fine, second is not.
    expectInputError(gas_water_runspec, R"(WELSPECS
  'PG' 'FIELD' 4 1 2000.0 'GAS' /
  'PO' 'FIELD' 5 1 2000.0 'OIL' /
/
)", "well PO");
}

BOOST_AUTO_TEST_CASE(Inactive_Phase_Rejected_In_Later_Report_Step)
{
    expectInputError(gas_water_runspec, R"(DATES
 1 FEB 2020 /
/
WELSPECS
  'PROBE' 'FIELD' 4 1 2000.0 'OIL' /
/
)", "OIL is not active in the run");
}

BOOST_AUTO_TEST_CASE(LIQ_Maps_To_Water_When_Oil_Inactive)
{
    const auto sched = makeSchedule(gas_water_runspec, R"(WELSPECS
  'PL' 'FIELD' 4 1 2000.0 'LIQ' /
/
)");

    BOOST_CHECK(sched.getWell("PL", 0).getPreferredPhase() == Opm::Phase::WATER);
}

BOOST_AUTO_TEST_CASE(LIQ_Maps_To_Oil_When_Oil_Active)
{
    const auto sched = makeSchedule(oil_water_runspec, R"(WELSPECS
  'PL' 'FIELD' 4 1 2000.0 'LIQ' /
/
)");

    BOOST_CHECK(sched.getWell("PL", 0).getPreferredPhase() == Opm::Phase::OIL);
}

BOOST_AUTO_TEST_CASE(LIQ_Rejected_When_Neither_Oil_Nor_Water_Active)
{
    expectInputError(gas_runspec, R"(WELSPECS
  'PL' 'FIELD' 4 1 2000.0 'LIQ' /
/
)", "Neither OIL nor WATER are active");
}

BOOST_AUTO_TEST_SUITE_END()     // WELSPECS_PreferredPhase

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(WELSPECL_PreferredPhase)

BOOST_AUTO_TEST_CASE(Inactive_Oil_Is_Rejected)
{
    try {
        makeLgrSchedule("GAS\nWATER\n", R"(WELSPECL
  'PROBE' 'FIELD' 'LGR1' 1 1 2000.0 'OIL' /
/
)");
        BOOST_ERROR("Schedule construction did not throw");
    }
    catch (const Opm::OpmInputError& e) {
        const auto msg = std::string { e.what() };
        BOOST_CHECK_MESSAGE(msg.find("OIL is not active in the run") != std::string::npos,
                            "Unexpected message: " << msg);
        BOOST_CHECK_MESSAGE(msg.find("WELSPECL") != std::string::npos,
                            "Unexpected message: " << msg);
    }
}

BOOST_AUTO_TEST_CASE(Active_Phase_Is_Accepted)
{
    const auto sched = makeLgrSchedule("GAS\nWATER\n", R"(WELSPECL
  'PROBE' 'FIELD' 'LGR1' 1 1 2000.0 'GAS' /
/
)");

    BOOST_CHECK(sched.getWell("PROBE", 0).getPreferredPhase() == Opm::Phase::GAS);
}

BOOST_AUTO_TEST_SUITE_END()     // WELSPECL_PreferredPhase

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(WCONINJE_WCONINJH_InjectionPhase)

BOOST_AUTO_TEST_CASE(WCONINJE_Inactive_Phase_Is_Rejected)
{
    expectInputError(gas_water_runspec, R"(WELSPECS
  'I1' 'FIELD' 4 1 2000.0 'GAS' /
/
WCONINJE
  'I1' 'OIL' 'OPEN' 'RATE' 100.0 /
/
)", "OIL is not active in the run and cannot be injected in OIL injector I1");
}

BOOST_AUTO_TEST_CASE(WCONINJE_Active_Phase_Is_Accepted)
{
    const auto sched = makeSchedule(gas_water_runspec, R"(WELSPECS
  'I1' 'FIELD' 4 1 2000.0 'GAS' /
/
WCONINJE
  'I1' 'WATER' 'OPEN' 'RATE' 100.0 /
/
)");

    BOOST_CHECK(sched.getWell("I1", 0).getPreferredPhase() == Opm::Phase::WATER);
}

BOOST_AUTO_TEST_CASE(WCONINJE_Inactive_Phase_Rejected_In_Later_Report_Step)
{
    expectInputError(gas_water_runspec, R"(WELSPECS
  'I1' 'FIELD' 4 1 2000.0 'GAS' /
/
WCONINJE
  'I1' 'GAS' 'OPEN' 'RATE' 100.0 /
/
DATES
 1 FEB 2020 /
/
WCONINJE
  'I1' 'OIL' 'OPEN' 'RATE' 100.0 /
/
)", "OIL is not active in the run");
}

BOOST_AUTO_TEST_CASE(WCONINJH_Inactive_Phase_Is_Rejected)
{
    expectInputError(gas_water_runspec, R"(WELSPECS
  'I1' 'FIELD' 4 1 2000.0 'GAS' /
/
WCONINJH
  'I1' 'OIL' 'OPEN' 100.0 /
/
)", "OIL is not active in the run and cannot be injected in OIL injector I1");
}

BOOST_AUTO_TEST_CASE(WCONINJH_Active_Phase_Is_Accepted)
{
    BOOST_CHECK_NO_THROW(makeSchedule(gas_water_runspec, R"(WELSPECS
  'I1' 'FIELD' 4 1 2000.0 'GAS' /
/
WCONINJH
  'I1' 'WATER' 'OPEN' 100.0 /
/
)"));
}

BOOST_AUTO_TEST_SUITE_END()     // WCONINJE_WCONINJH_InjectionPhase

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(GCONINJE_InjectionPhase)

BOOST_AUTO_TEST_CASE(Inactive_Phase_Is_Rejected)
{
    expectInputError(gas_water_runspec, R"(GRUPTREE
  'G1' 'FIELD' /
/
GCONINJE
  'G1' 'OIL' 'RATE' 100.0 /
/
)", "OIL is not active in the run and cannot be used for injection controls in group G1");
}

BOOST_AUTO_TEST_CASE(Inactive_Phase_Is_Rejected_For_FIELD)
{
    expectInputError(oil_water_runspec, R"(GCONINJE
  'FIELD' 'GAS' 'RATE' 100.0 /
/
)", "GAS is not active in the run");
}

BOOST_AUTO_TEST_CASE(Multiple_Matching_Groups_Use_Plural_Message)
{
    expectInputError(gas_water_runspec, R"(GRUPTREE
  'G1' 'FIELD' /
  'G2' 'FIELD' /
/
GCONINJE
  'G*' 'OIL' 'RATE' 100.0 /
/
)", "in groups G1, G2");
}

BOOST_AUTO_TEST_CASE(Active_Phases_Are_Accepted)
{
    BOOST_CHECK_NO_THROW(makeSchedule(gas_water_runspec, R"(GRUPTREE
  'G1' 'FIELD' /
/
GCONINJE
  'G1' 'GAS'   'RATE' 100.0 /
  'G1' 'WATER' 'RATE' 100.0 /
/
)"));
}

BOOST_AUTO_TEST_SUITE_END()     // GCONINJE_InjectionPhase
