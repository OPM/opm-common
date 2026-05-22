/*
  Copyright 2020 Equinor ASA.

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

#define BOOST_TEST_MODULE NetworkTests

#include <boost/test/unit_test.hpp>

#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquifers.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Phase.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Network/Branch.hpp>
#include <opm/input/eclipse/Schedule/Network/ExtNetwork.hpp>
#include <opm/input/eclipse/Schedule/Network/Node.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <opm/input/eclipse/Units/Units.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include <fmt/format.h>

using namespace Opm;

namespace {
Schedule make_schedule(const std::string& schedule_string)
{
    const auto deck = Parser{}.parseString(schedule_string);
    EclipseGrid grid(10,10,10);
    const TableManager table ( deck );
    const FieldPropsManager fp( deck, Phases{true, true, true}, grid, table);
    const Runspec runspec (deck);

    return {
        deck, grid, fp, NumericalAquifers{},
        runspec, std::make_shared<Python>()
    };
}
}

BOOST_AUTO_TEST_SUITE(Basic_Functionality)

BOOST_AUTO_TEST_CASE(CreateNetwork) {
    Network::ExtNetwork network;
    BOOST_CHECK( !network.active() );
    auto schedule = make_schedule("SCHEDULE\n");
    auto network2 = schedule[0].network.get();
    BOOST_CHECK( !network2.active() );
}



BOOST_AUTO_TEST_CASE(Branch) {
    // Test illegal construction.
    BOOST_CHECK_THROW( Network::Branch("down", "up", 100, Network::Branch::AlqEQ::ALQ_INPUT), std::logic_error);

    // Test ALQ behaviour.
    Network::Branch b("down", "up", 101, 864.0);
    // Assume we have a GRAT ALQ type in a METRIC unit system:
    //   864 m^3/day, equal to 0.01 m^3/s
    const auto alq_type = VFPProdTable::ALQ_TYPE::ALQ_GRAT;
    const UnitSystem usys; // Defaults to METRIC.
    const Dimension dim = VFPProdTable::ALQDimension(alq_type, usys);
    const auto alq_val = b.alq_value(dim);
    BOOST_REQUIRE(alq_val.has_value());
    BOOST_CHECK_CLOSE(*alq_val, 0.01, 1e-8);
}


BOOST_AUTO_TEST_CASE(INVALID_DOWNTREE_NODE) {
    std::string deck_string = R"(
RUNSPEC
NETWORK
 3 2 /

SCHEDULE

GRUPTREE
 'PROD'    'FIELD' /

 'M5S'    'PLAT-A'  /
 'M5N'    'PLAT-A'  /

 'C1'     'M5N'  /
 'F1'     'M5N'  /
 'B1'     'M5S'  /
 'G1'     'M5S'  /
/

BRANPROP
--  Downtree  Uptree   #VFP    ALQ
    B1X        PLAT-A    5      1*      /
    C1         PLAT-A    4      1*      /
/

NODEPROP
--  Node_name Pr    autoChock?      addGasLift?     Group_name
     PLAT-A 21.0   NO     NO    1*  /
     B1    1*  NO     NO    1*  /
     C1    1*  NO     NO    1*  /
/
)";

    const auto& schedule = make_schedule(deck_string);
    const auto& network = schedule[0].network.get();
    BOOST_CHECK(network.has_node("B1X"));
}


BOOST_AUTO_TEST_CASE(INVALID_UPTREE_NODE) {
    std::string deck_string = R"(
RUNSPEC
NETWORK
 3 2 /

SCHEDULE

GRUPTREE
 'PROD'    'FIELD' /

 'M5S'    'PLAT-A'  /
 'M5N'    'PLAT-A'  /

 'C1'     'M5N'  /
 'F1'     'M5N'  /
 'B1'     'M5S'  /
 'G1'     'M5S'  /
/

BRANPROP
--  Downtree  Uptree   #VFP    ALQ
    B1         PLAT-AX    5      1*      /
    C1         PLAT-AX    4      1*      /
/

NODEPROP
--  Node_name Pr    autoChock?      addGasLift?     Group_name
     PLAT-A 21.0   NO     NO    1*  /
     B1    1*  NO     NO    1*  /
     C1    1*  NO     NO    1*  /
/
)";

    const auto& schedule = make_schedule(deck_string);
    const auto& network = schedule[0].network.get();
    BOOST_CHECK(network.has_node("PLAT-AX"));
}

BOOST_AUTO_TEST_CASE(INVALID_VFP_NODE) {
    std::string deck_string = R"(
SCHEDULE

GRUPTREE
 'PROD'    'FIELD' /

 'M5S'    'PLAT-A'  /
 'M5N'    'PLAT-A'  /

 'C1'     'M5N'  /
 'F1'     'M5N'  /
 'B1'     'M5S'  /
 'G1'     'M5S'  /
/

BRANPROP
--  Downtree  Uptree   #VFP    ALQ
    B1         PLAT-A    5      1*      /
    C1         PLAT-A    4      1*      /  --This is a choke branch - must have VFP=9999
/

NODEPROP
--  Node_name Pr    autoChock?      addGasLift?     Group_name
     PLAT-A 21.0   NO     NO    1*  /
     B1    1*  NO     NO    1*  /
     C1    1*  YES    NO    1*  /
/
)";

    BOOST_CHECK_THROW( make_schedule(deck_string), std::exception);
}
BOOST_AUTO_TEST_CASE(INVALID_AUTOCHOKE_NODE) {
    std::string deck_string = R"(
SCHEDULE

GRUPTREE
 'PROD'    'FIELD' /

 'M5S'    'PLAT-A'  /
 'M5N'    'PLAT-A'  /

 'C1'     'M5N'  /
 'F1'     'M5N'  /
 'B1'     'M5S'  /
 'G1'     'M5S'  /
/

BRANPROP
--  Downtree  Uptree   #VFP    ALQ
    B1         PLAT-A    5      1*      /
    C1         PLAT-A    9999      1*      /
/

NODEPROP
--  Node_name Pr    autoChock?      addGasLift?   Group_name
     PLAT-A 21.0   NO     NO    1*  /
     B1    1*  NO     NO    1*  /
     C1    1*  YES    NO    'GROUP' /  --This is an autochoke well-group - must have defaulted target group_name
/
)";

    BOOST_CHECK_THROW( make_schedule(deck_string), std::exception);
}

BOOST_AUTO_TEST_CASE(OK) {
    std::string deck_string = R"(
RUNSPEC
NETWORK
 3 2 /

SCHEDULE

GRUPTREE
 'PROD'    'FIELD' /

 'M5S'    'PLAT-A'  /
 'M5N'    'PLAT-A'  /

 'C1'     'M5N'  /
 'F1'     'M5N'  /
 'B1'     'M5S'  /
 'G1'     'M5S'  /
/

BRANPROP
--  Downtree  Uptree   #VFP    ALQ
    B1         PLAT-A    9999      1*      /
    C1         PLAT-A    9999      1*      /
/

NODEPROP
--  Node_name Pr    autoChock?      addGasLift?     Group_name
     PLAT-A 21.0   NO     NO    1*  /
     B1    1*  YES      NO    1*  /
     C1    1*  YES     NO     1* /
/

TSTEP
  10 /

BRANPROP
--  Downtree  Uptree   #VFP    ALQ
    C1         PLAT-A    0 1*      /
/


)";

    auto sched = make_schedule(deck_string);
    {
        const auto& network = sched[0].network.get();
        const auto& b1 = network.node("B1");
        BOOST_CHECK(b1.as_choke());
        BOOST_CHECK(!b1.add_gas_lift_gas());
        BOOST_CHECK(b1.name() == b1.target_group());
        BOOST_CHECK(!b1.terminal_pressure());

        const auto& p = network.node("PLAT-A");
        BOOST_CHECK(p.terminal_pressure());
        BOOST_CHECK_EQUAL(p.terminal_pressure().value(), 21 * 100000);
        BOOST_CHECK(p == network.roots()[0]);

        BOOST_CHECK_THROW(network.node("NO_SUCH_NODE"), std::out_of_range);



        BOOST_CHECK_EQUAL(network.downtree_branches("PLAT-A").size(), 2U);
        for (const auto& b : network.downtree_branches("PLAT-A")) {
            BOOST_CHECK_EQUAL(b.uptree_node(), "PLAT-A");
            BOOST_CHECK(b.downtree_node() == "B1" || b.downtree_node() == "C1");
        }


        const auto& platform_uptree = network.uptree_branch("PLAT-A");
        BOOST_CHECK(!platform_uptree.has_value());

        const auto& B1_uptree = network.uptree_branch("B1");
        BOOST_CHECK(B1_uptree.has_value());
        BOOST_CHECK_EQUAL(B1_uptree->downtree_node(), "B1");
        BOOST_CHECK_EQUAL(B1_uptree->uptree_node(), "PLAT-A");

        BOOST_CHECK(network.active());
    }
    {
        const auto& network = sched[1].network.get();
        const auto& b1 = network.node("B1");
        BOOST_CHECK(b1.as_choke());
        BOOST_CHECK(!b1.add_gas_lift_gas());
        BOOST_CHECK(b1.name() == b1.target_group());
        BOOST_CHECK(!b1.terminal_pressure());

        BOOST_CHECK_EQUAL(network.downtree_branches("PLAT-A").size(), 1U);
        for (const auto& b : network.downtree_branches("PLAT-A")) {
            BOOST_CHECK_EQUAL(b.uptree_node(), "PLAT-A");
            BOOST_CHECK(b.downtree_node() == "B1");
        }


        const auto& platform_uptree = network.uptree_branch("PLAT-A");
        BOOST_CHECK(!platform_uptree.has_value());

        const auto& B1_uptree = network.uptree_branch("B1");
        BOOST_CHECK(B1_uptree.has_value());
        BOOST_CHECK_EQUAL(B1_uptree->downtree_node(), "B1");
        BOOST_CHECK_EQUAL(B1_uptree->uptree_node(), "PLAT-A");

        BOOST_CHECK( network.has_node("C1") );
        BOOST_CHECK( !network.uptree_branch("C1") );

        BOOST_CHECK(network.active());
    }
}

BOOST_AUTO_TEST_CASE(NodeNames) {
    const auto sched = make_schedule(R"(
RUNSPEC
NETWORK
 3 2 /

SCHEDULE

GRUPTREE
 'PROD'    'FIELD' /

 'M5S'    'PLAT-A'  /
 'M5N'    'PLAT-A'  /

 'C1'     'M5N'  /
 'F1'     'M5N'  /
 'B1'     'M5S'  /
 'G1'     'M5S'  /
/

BRANPROP
--  Downtree  Uptree   #VFP    ALQ
    B1         PLAT-A    9999      1*      /
    C1         PLAT-A    9999      1*      /
/

NODEPROP
--  Node_name Pr    autoChock?      addGasLift?     Group_name
     PLAT-A 21.0   NO     NO    1*  /
     B1    1*  YES      NO    1*  /
     C1    1*  YES     NO     1* /
/

TSTEP
  10 /

BRANPROP
--  Downtree  Uptree   #VFP    ALQ
    C1         PLAT-A    0 1*      /
/
)");

    const auto expect = std::vector<std::string> {
        "B1", "C1", "PLAT-A"
    };

    auto nodes = sched[0].network.get().node_names();
    std::ranges::sort(nodes);

    BOOST_CHECK_EQUAL_COLLECTIONS(nodes.begin(), nodes.end(), expect.begin(), expect.end());
}

BOOST_AUTO_TEST_CASE(DefaultedNodes) {
    const auto sched = make_schedule(R"(
RUNSPEC
NETWORK
 3 2 /

SCHEDULE

GRUPTREE
 'PROD'    'FIELD' /

 'M5S'    'PLAT-A'  /
 'M5N'    'PLAT-A'  /

 'C1'     'M5N'  /
 'F1'     'M5N'  /
 'B1'     'M5S'  /
 'G1'     'M5S'  /
/

BRANPROP
--  Downtree  Uptree   #VFP    ALQ
    B1         PLAT-A    9999      1*      /
    C1         PLAT-A    9999      1*      /
/

NODEPROP
--  Node_name Pr    autoChock?      addGasLift?     Group_name
     PLAT-A 21.0   NO     NO    1*  /
--     B1    1*  YES      NO    1*  /
--     C1    1*  YES     NO     'GROUP' /
/

TSTEP
  10 /

BRANPROP
--  Downtree  Uptree   #VFP    ALQ
    C1         PLAT-A    0 1*      /
/
)");

    // The difference between this test and the NodeNames test above is that
    // the NODEPROP entries for B1 and C1 have been commented out.

    const auto expect = std::vector<std::string> {
        "B1", "C1", "PLAT-A"
    };

    auto nodes = sched[0].network.get().node_names();
    std::ranges::sort(nodes);

    BOOST_CHECK_EQUAL_COLLECTIONS(nodes.begin(), nodes.end(), expect.begin(), expect.end());
}

BOOST_AUTO_TEST_CASE(StandardNetwork) {

    const std::string input = R"(

SCHEDULE

GRUPTREE
'PROD'    'FIELD' /

'M5S'    'PLAT-A'  /
'M5N'    'PLAT-A'  /

'C1'     'M5N'  /
'F1'     'M5N'  /
'B1'     'M5S'  /
'G1'     'M5S'  /
/

GRUPNET
'PLAT-A'     21.000  5* /
'M5S'  1*    3  1*        'NO'  2* /
'B1'  1*     5  1*        'NO'  2* /
'C1'  1*     4  1*        'NO'  2* /
/)";

    auto schedule = make_schedule(input);

    const auto& network = schedule[0].network.get();
    const auto& b1 = network.node("B1");
    const auto upbranch = network.uptree_branch(b1.name());
    const auto vfp_table = (*upbranch).vfp_table();
    BOOST_CHECK_EQUAL(vfp_table.value(), 5);
    BOOST_CHECK(!b1.terminal_pressure());

    const auto& p = network.node("PLAT-A");
    BOOST_CHECK(p.terminal_pressure());
    BOOST_CHECK_EQUAL(p.terminal_pressure().value(), 21 * 100000);
    BOOST_CHECK(p == network.roots()[0]);
}

BOOST_AUTO_TEST_SUITE_END()     // Basic_Functionality

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Keyword_Consistency)

BOOST_AUTO_TEST_SUITE(Standard_Networks)

BOOST_AUTO_TEST_CASE(Reject_NETWORK_Keyword)
{
    BOOST_CHECK_THROW(const auto deck = Parser{}.parseString(R"(
GRUPNET
 FIELD 12.34 /
/
NETWORK
 3 2 /
)"), OpmInputError);
}

BOOST_AUTO_TEST_CASE(Reject_BRANPROP_Keyword)
{
    BOOST_CHECK_THROW(const auto deck = Parser{}.parseString(R"(
GRUPNET
 FIELD 12.34 /
/
BRANPROP
 'D' 'U' 9999 /
/
)"), OpmInputError);
}

BOOST_AUTO_TEST_CASE(Reject_NODEPROP_Keyword)
{
    BOOST_CHECK_THROW(const auto deck = Parser{}.parseString(R"(
GRUPNET
 FIELD 12.34 /
/
NODEPROP
 'D' 23.45 /
/
)"), OpmInputError);
}

BOOST_AUTO_TEST_SUITE_END()     // Standard_Networks

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Extended_Network)

BOOST_AUTO_TEST_CASE(Network_Reject_Grupnet)
{
    BOOST_CHECK_THROW(const auto deck = Parser{}.parseString(R"(
NETWORK
 3 2 /
GRUPNET
 FIELD 12.34 /
/
)"), OpmInputError);
}

BOOST_AUTO_TEST_CASE(Branprop_Requires_Network)
{
    BOOST_CHECK_THROW(const auto deck = Parser{}.parseString(R"(
BRANPROP
 'D' 'U' 9999 /
/
)"), OpmInputError);
}

BOOST_AUTO_TEST_CASE(Nodeprop_Requires_Network)
{
    BOOST_CHECK_THROW(const auto deck = Parser{}.parseString(R"(
NODEPROP
 'D' 23.45 /
/
)"), OpmInputError);
}

BOOST_AUTO_TEST_CASE(Nodeprop_Requires_Branprop)
{
    BOOST_CHECK_THROW(const auto deck = Parser{}.parseString(R"(
NETWORK
 3 2 /
NODEPROP
 'D' 23.45 /
/
)"), OpmInputError);
}

BOOST_AUTO_TEST_SUITE_END()     // Extended_Network

BOOST_AUTO_TEST_SUITE_END()     // Keyword_Consistency

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Injection_Networks)

namespace {
    struct Case
    {
        explicit Case(const std::string& input)
            : Case { Parser{}.parseString(input) }
        {}

        explicit Case(const Deck& deck)
            : es    { deck }
            , sched { deck, es }
        {}

        EclipseState es{};
        Schedule sched{};
    };

    Case twoD_5x1x2(std::string_view gnetInje)
    {
        // Group tree structure from opm-tests/network/GNETINJE_GAS-01.DATA
        //
        //                                 FIELD
        //                                   |
        //                                 PLAT-A
        //                                   |
        //                   +---------------+
        //                   |
        //                  M5S -------------------------- M5N
        //                   |                              |
        //         +---------+---------+                    +------------+
        //         |                   |                    |            |
        //        B1                  G1                   C1           F1
        //         |                   |                    |            |
        //    +----+-----+         +---+--+             +---+--+      +--+---+
        //    |    |     |         |      |             |      |      |      |
        //  B-1H  B-2H  B-3H     G-3H    G-4H         C-1H   C-2H    F-1H   F-2H

        return Case { fmt::format(R"(RUNSPEC
DIMENS
  5 1 2 /
START
  22 MAY 2026 /
OIL
GAS
WATER
METRIC
TABDIMS
/
GRID
DXV
  5*100 /
DYV
  100 /
DZV
  2*10 /
DEPTHZ
  12*2000 /
EQUALS
  PERMX 100 /
  PERMY 100 /
  PERMZ  10 /
  PORO    0.3 /
/
PROPS
DENSITY
  850  1023  0.1 /
SCHEDULE
GRUPTREE
 'PLAT-A' 'FIELD' /

 'M5S'    'PLAT-A'  /
 'M5N'    'M5S'  /

 'C1'     'M5N'  /
 'F1'     'M5N'  /
 'B1'     'M5S'  /
 'G1'     'M5S'  /
 /
{}
DATES
  1 JUN 2026 /
  1 JUL 2026 /
/
END
)", gnetInje) };
    }

} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(Gas)

BOOST_AUTO_TEST_SUITE(Single_Network_Definition)

namespace {
    auto wellDefined()
    {
        return twoD_5x1x2(R"(GNETINJE
 'PLAT-A' 'GAS'  340.0  /
 'M5S'    'GAS'   1*   3   /
 'G1'     'GAS'   1*  9999 /
 'M5N'    'GAS'   1*   2   /
 'F1'     'GAS'   1*  9999 /
/
)");
    }

    auto fieldTerminal()
    {
        return twoD_5x1x2(R"(GNETINJE
 'FIELD'  'GAS'  340.0  /
 'PLAT-A' 'GAS'   1*  9999 /
 'M5S'    'GAS'   1*   3   /
 'G1'     'GAS'   1*  9999 /
 'M5N'    'GAS'   1*   2   /
 'F1'     'GAS'   1*  9999 /
/
)");
    }
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(Existence)
{
    const auto cse = wellDefined();

    BOOST_REQUIRE_EQUAL(cse.sched.size(), std::size_t{3});

    BOOST_CHECK_MESSAGE(cse.sched[0].injectionNetwork.has(Phase::GAS),
                        R"(There must be an injection network for the "GAS" phase at time zero.)");

    BOOST_CHECK_MESSAGE(cse.sched.back().injectionNetwork.has(Phase::GAS),
                        R"(There must be an injection network for the "GAS" phase.)");

    BOOST_CHECK_MESSAGE(! cse.sched.back().injectionNetwork.has(Phase::WATER),
                        R"(There must NOT be an injection network for the "WATER" phase.)");
}

BOOST_AUTO_TEST_CASE(Topology)
{
    using namespace std::string_literals;

    const auto injNetwork = wellDefined().sched.back().injectionNetwork.get(Phase::GAS);

    BOOST_REQUIRE_MESSAGE(std::ranges::is_permutation
                          (injNetwork.node_names(), std::array {
                              "PLAT-A"s, "M5S"s, "G1"s, "M5N"s, "F1"s,
                          }),
                          R"(Injection network does not have the expected nodes.)");

    // F1 and G1 are leaf nodes
    BOOST_CHECK_MESSAGE(injNetwork.downtree_branches("F1"s).empty(),
                        R"(Injection network node "F1" must be a leaf node.)");

    BOOST_CHECK_MESSAGE(injNetwork.downtree_branches("G1"s).empty(),
                        R"(Injection network node "G1" must be a leaf node.)");

    {
        const auto f1UpBranch = injNetwork.uptree_branch("F1"s);

        BOOST_REQUIRE_MESSAGE(f1UpBranch.has_value(),
                              R"(Injection network node "F1" must be connected to a branch)");

        BOOST_CHECK_EQUAL(f1UpBranch->downtree_node(), "F1"s);
        BOOST_CHECK_EQUAL(f1UpBranch->uptree_node(), "M5N"s);
    }

    {
        const auto g1UpBranch = injNetwork.uptree_branch("G1"s);

        BOOST_REQUIRE_MESSAGE(g1UpBranch.has_value(),
                              R"(Injection network node "G1" must have an upstream branch)");

        BOOST_CHECK_EQUAL(g1UpBranch->downtree_node(), "G1"s);
        BOOST_CHECK_EQUAL(g1UpBranch->uptree_node(), "M5S"s);
    }

    {
        const auto m5nUpBranch = injNetwork.uptree_branch("M5N"s);

        BOOST_REQUIRE_MESSAGE(m5nUpBranch.has_value(),
                              R"(Injection network node "M5N" must have an upstream branch)");

        BOOST_CHECK_EQUAL(m5nUpBranch->downtree_node(), "M5N"s);
        BOOST_CHECK_EQUAL(m5nUpBranch->uptree_node(), "M5S"s);
    }

    {
        const auto m5sUpBranch = injNetwork.uptree_branch("M5S"s);

        BOOST_REQUIRE_MESSAGE(m5sUpBranch.has_value(),
                              R"(Injection network node "M5S" must have an upstream branch)");

        BOOST_CHECK_EQUAL(m5sUpBranch->downtree_node(), "M5S"s);
        BOOST_CHECK_EQUAL(m5sUpBranch->uptree_node(), "PLAT-A"s);
    }

    {
        const auto plat_aUpBranch = injNetwork.uptree_branch("PLAT-A"s);

        BOOST_REQUIRE_MESSAGE(! plat_aUpBranch.has_value(),
                              R"(Injection network node "PLAT-A" must NOT have an upstream branch)");
    }

    {
        const auto f1DownBranch = injNetwork.downtree_branches("F1"s);
        BOOST_CHECK_MESSAGE(f1DownBranch.empty(),
                            R"(Injection network node "F1" must NOT have downstream branches)");
    }

    {
        const auto g1DownBranch = injNetwork.downtree_branches("G1"s);
        BOOST_CHECK_MESSAGE(g1DownBranch.empty(),
                            R"(Injection network node "G1" must NOT have downstream branches)");
    }

    {
        const auto m5nDownBranch = injNetwork.downtree_branches("M5N"s);

        BOOST_REQUIRE_EQUAL(m5nDownBranch.size(), std::size_t{1});

        BOOST_CHECK_EQUAL(m5nDownBranch.front().uptree_node(), "M5N"s);
        BOOST_CHECK_EQUAL(m5nDownBranch.front().downtree_node(), "F1"s);
    }

    {
        const auto m5sDownBranch = injNetwork.downtree_branches("M5S"s);

        BOOST_REQUIRE_EQUAL(m5sDownBranch.size(), std::size_t{2});

        BOOST_CHECK_MESSAGE(std::ranges::equal(m5sDownBranch, std::array { "M5S"s, "M5S"s },
                                               std::ranges::equal_to{},
                                               &Network::Branch::uptree_node),
                            R"(Upstream node must be "M5S")");

        BOOST_CHECK_MESSAGE(std::ranges::is_permutation(m5sDownBranch, std::array { "M5N"s, "G1"s },
                                                        std::ranges::equal_to{},
                                                        &Network::Branch::downtree_node),
                            R"(Downstream nodes must be "M5N" and "G1")");
    }

    {
        const auto plat_aDownBranch = injNetwork.downtree_branches("PLAT-A"s);

        BOOST_REQUIRE_EQUAL(plat_aDownBranch.size(), std::size_t{1});

        BOOST_CHECK_EQUAL(plat_aDownBranch.front().uptree_node(), "PLAT-A"s);
        BOOST_CHECK_EQUAL(plat_aDownBranch.front().downtree_node(), "M5S"s);
    }
}

BOOST_AUTO_TEST_CASE(Properties)
{
    using namespace std::string_literals;

    const auto injNetwork = wellDefined().sched.back().injectionNetwork.get(Phase::GAS);

    // =======================================================================
    // Node properties
    // =======================================================================

    {
        const auto f1 = injNetwork.node("F1"s);

        BOOST_CHECK_EQUAL(f1.name(), "F1"s);
        BOOST_CHECK_MESSAGE(! f1.terminal_pressure().has_value(),
                            R"(Injection network node "F1" must NOT have a terminal pressure)");
    }

    {
        const auto g1 = injNetwork.node("G1"s);

        BOOST_CHECK_EQUAL(g1.name(), "G1"s);
        BOOST_CHECK_MESSAGE(! g1.terminal_pressure().has_value(),
                            R"(Injection network node "G1" must NOT have a terminal pressure)");
    }

    {
        const auto m5n = injNetwork.node("M5N"s);

        BOOST_CHECK_EQUAL(m5n.name(), "M5N"s);
        BOOST_CHECK_MESSAGE(! m5n.terminal_pressure().has_value(),
                            R"(Injection network node "M5N" must NOT have a terminal pressure)");
    }

    {
        const auto m5s = injNetwork.node("M5S"s);

        BOOST_CHECK_EQUAL(m5s.name(), "M5S"s);
        BOOST_CHECK_MESSAGE(! m5s.terminal_pressure().has_value(),
                            R"(Injection network node "M5S" must NOT have a terminal pressure)");
    }

    {
        const auto plat_a = injNetwork.node("PLAT-A"s);

        BOOST_CHECK_EQUAL(plat_a.name(), "PLAT-A"s);

        BOOST_CHECK_MESSAGE(plat_a.terminal_pressure().has_value(),
                            R"(Injection network node "PLAT-A" must have a terminal pressure)");

        BOOST_CHECK_CLOSE(plat_a.terminal_pressure().value(),
                          340.0*unit::barsa, 1.0e-6);
    }

    // =======================================================================
    // Branch properties
    // =======================================================================

    {
        const auto f1UpBranch = injNetwork.uptree_branch("F1"s);

        BOOST_REQUIRE_MESSAGE(f1UpBranch.has_value(),
                              R"(There must be an upstream branch attached to injection node "F1")");

        // VFP table 9999 => no flow line.
        BOOST_REQUIRE_MESSAGE(! f1UpBranch->vfp_table().has_value(),
                              R"(There must NOT be a flow line (VFP table) attached to "F1"'s upstream branch.)");
    }

    {
        const auto g1UpBranch = injNetwork.uptree_branch("G1"s);

        BOOST_REQUIRE_MESSAGE(g1UpBranch.has_value(),
                              R"(There must be an upstream branch attached to injection node "G1")");

        // VFP table 9999 => no flow line.
        BOOST_REQUIRE_MESSAGE(! g1UpBranch->vfp_table().has_value(),
                              R"(There must NOT be a flow line (VFP table) attached to "G1"'s upstream branch.)");
    }

    {
        const auto m5nUpBranch = injNetwork.uptree_branch("M5N"s);

        BOOST_REQUIRE_MESSAGE(m5nUpBranch.has_value(),
                              R"(There must be an upstream branch attached to injection node "M5N")");

        BOOST_REQUIRE_MESSAGE(m5nUpBranch->vfp_table().has_value(),
                              R"(There must be a flow line (VFP table) attached to "M5N"'s upstream branch.)");

        BOOST_CHECK_EQUAL(m5nUpBranch->vfp_table().value(), 2);
    }

    {
        const auto m5sUpBranch = injNetwork.uptree_branch("M5S"s);

        BOOST_REQUIRE_MESSAGE(m5sUpBranch.has_value(),
                              R"(There must be an upstream branch attached to injection node "M5S")");

        BOOST_REQUIRE_MESSAGE(m5sUpBranch->vfp_table().has_value(),
                              R"(There must be a flow line (VFP table) attached to "M5S"'s upstream branch.)");

        BOOST_CHECK_EQUAL(m5sUpBranch->vfp_table().value(), 3);
    }
}

BOOST_AUTO_TEST_CASE(Existence_Field_Terminal)
{
    const auto cse = fieldTerminal();

    BOOST_REQUIRE_EQUAL(cse.sched.size(), std::size_t{3});

    BOOST_CHECK_MESSAGE(cse.sched[0].injectionNetwork.has(Phase::GAS),
                        R"(There must be an injection network for the "GAS" phase at time zero.)");

    BOOST_CHECK_MESSAGE(cse.sched.back().injectionNetwork.has(Phase::GAS),
                        R"(There must be an injection network for the "GAS" phase.)");

    BOOST_CHECK_MESSAGE(! cse.sched.back().injectionNetwork.has(Phase::WATER),
                        R"(There must NOT be an injection network for the "WATER" phase.)");
}

BOOST_AUTO_TEST_CASE(Topology_Field_Terminal)
{
    using namespace std::string_literals;

    const auto injNetwork = fieldTerminal().sched.back().injectionNetwork.get(Phase::GAS);

    BOOST_REQUIRE_MESSAGE(std::ranges::is_permutation
                          (injNetwork.node_names(), std::array {
                              "FIELD"s, "PLAT-A"s, "M5S"s, "G1"s, "M5N"s, "F1"s,
                          }),
                          R"(Injection network does not have the expected nodes when FIELD is terminal.)");

    // F1 and G1 are leaf nodes
    BOOST_CHECK_MESSAGE(injNetwork.downtree_branches("F1"s).empty(),
                        R"(Injection network node "F1" must be a leaf node.)");

    BOOST_CHECK_MESSAGE(injNetwork.downtree_branches("G1"s).empty(),
                        R"(Injection network node "G1" must be a leaf node.)");

    {
        const auto f1UpBranch = injNetwork.uptree_branch("F1"s);

        BOOST_REQUIRE_MESSAGE(f1UpBranch.has_value(),
                              R"(Injection network node "F1" must be connected to a branch)");

        BOOST_CHECK_EQUAL(f1UpBranch->downtree_node(), "F1"s);
        BOOST_CHECK_EQUAL(f1UpBranch->uptree_node(), "M5N"s);
    }

    {
        const auto g1UpBranch = injNetwork.uptree_branch("G1"s);

        BOOST_REQUIRE_MESSAGE(g1UpBranch.has_value(),
                              R"(Injection network node "G1" must have an upstream branch)");

        BOOST_CHECK_EQUAL(g1UpBranch->downtree_node(), "G1"s);
        BOOST_CHECK_EQUAL(g1UpBranch->uptree_node(), "M5S"s);
    }

    {
        const auto m5nUpBranch = injNetwork.uptree_branch("M5N"s);

        BOOST_REQUIRE_MESSAGE(m5nUpBranch.has_value(),
                              R"(Injection network node "M5N" must have an upstream branch)");

        BOOST_CHECK_EQUAL(m5nUpBranch->downtree_node(), "M5N"s);
        BOOST_CHECK_EQUAL(m5nUpBranch->uptree_node(), "M5S"s);
    }

    {
        const auto m5sUpBranch = injNetwork.uptree_branch("M5S"s);

        BOOST_REQUIRE_MESSAGE(m5sUpBranch.has_value(),
                              R"(Injection network node "M5S" must have an upstream branch)");

        BOOST_CHECK_EQUAL(m5sUpBranch->downtree_node(), "M5S"s);
        BOOST_CHECK_EQUAL(m5sUpBranch->uptree_node(), "PLAT-A"s);
    }

    {
        const auto plat_aUpBranch = injNetwork.uptree_branch("PLAT-A"s);

        BOOST_REQUIRE_MESSAGE(plat_aUpBranch.has_value(),
                              R"(Injection network node "PLAT-A" must have an upstream branch when FIELD is terminal)");

        BOOST_CHECK_EQUAL(plat_aUpBranch->downtree_node(), "PLAT-A"s);
        BOOST_CHECK_EQUAL(plat_aUpBranch->uptree_node(), "FIELD"s);
    }

    {
        const auto f1DownBranch = injNetwork.downtree_branches("F1"s);
        BOOST_CHECK_MESSAGE(f1DownBranch.empty(),
                            R"(Injection network node "F1" must NOT have downstream branches)");
    }

    {
        const auto g1DownBranch = injNetwork.downtree_branches("G1"s);
        BOOST_CHECK_MESSAGE(g1DownBranch.empty(),
                            R"(Injection network node "G1" must NOT have downstream branches)");
    }

    {
        const auto m5nDownBranch = injNetwork.downtree_branches("M5N"s);

        BOOST_REQUIRE_EQUAL(m5nDownBranch.size(), std::size_t{1});

        BOOST_CHECK_EQUAL(m5nDownBranch.front().uptree_node(), "M5N"s);
        BOOST_CHECK_EQUAL(m5nDownBranch.front().downtree_node(), "F1"s);
    }

    {
        const auto m5sDownBranch = injNetwork.downtree_branches("M5S"s);

        BOOST_REQUIRE_EQUAL(m5sDownBranch.size(), std::size_t{2});

        BOOST_CHECK_MESSAGE(std::ranges::equal(m5sDownBranch, std::array { "M5S"s, "M5S"s },
                                               std::ranges::equal_to{},
                                               &Network::Branch::uptree_node),
                            R"(Upstream node must be "M5S")");

        BOOST_CHECK_MESSAGE(std::ranges::is_permutation(m5sDownBranch, std::array { "M5N"s, "G1"s },
                                                        std::ranges::equal_to{},
                                                        &Network::Branch::downtree_node),
                            R"(Downstream nodes must be "M5N" and "G1")");
    }

    {
        const auto plat_aDownBranch = injNetwork.downtree_branches("PLAT-A"s);

        BOOST_REQUIRE_EQUAL(plat_aDownBranch.size(), std::size_t{1});

        BOOST_CHECK_EQUAL(plat_aDownBranch.front().uptree_node(), "PLAT-A"s);
        BOOST_CHECK_EQUAL(plat_aDownBranch.front().downtree_node(), "M5S"s);
    }

    {
        const auto fieldDownBranch = injNetwork.downtree_branches("FIELD"s);

        BOOST_REQUIRE_EQUAL(fieldDownBranch.size(), std::size_t{1});

        BOOST_CHECK_EQUAL(fieldDownBranch.front().uptree_node(), "FIELD"s);
        BOOST_CHECK_EQUAL(fieldDownBranch.front().downtree_node(), "PLAT-A"s);
    }
}

BOOST_AUTO_TEST_CASE(Properties_Field_Terminal)
{
    using namespace std::string_literals;

    const auto injNetwork = fieldTerminal().sched.back().injectionNetwork.get(Phase::GAS);

    // =======================================================================
    // Node properties
    // =======================================================================

    {
        const auto f1 = injNetwork.node("F1"s);

        BOOST_CHECK_EQUAL(f1.name(), "F1"s);
        BOOST_CHECK_MESSAGE(! f1.terminal_pressure().has_value(),
                            R"(Injection network node "F1" must NOT have a terminal pressure)");
    }

    {
        const auto g1 = injNetwork.node("G1"s);

        BOOST_CHECK_EQUAL(g1.name(), "G1"s);
        BOOST_CHECK_MESSAGE(! g1.terminal_pressure().has_value(),
                            R"(Injection network node "G1" must NOT have a terminal pressure)");
    }

    {
        const auto m5n = injNetwork.node("M5N"s);

        BOOST_CHECK_EQUAL(m5n.name(), "M5N"s);
        BOOST_CHECK_MESSAGE(! m5n.terminal_pressure().has_value(),
                            R"(Injection network node "M5N" must NOT have a terminal pressure)");
    }

    {
        const auto m5s = injNetwork.node("M5S"s);

        BOOST_CHECK_EQUAL(m5s.name(), "M5S"s);
        BOOST_CHECK_MESSAGE(! m5s.terminal_pressure().has_value(),
                            R"(Injection network node "M5S" must NOT have a terminal pressure)");
    }

    {
        const auto plat_a = injNetwork.node("PLAT-A"s);

        BOOST_CHECK_EQUAL(plat_a.name(), "PLAT-A"s);

        BOOST_CHECK_MESSAGE(! plat_a.terminal_pressure().has_value(),
                            R"(Injection network node "PLAT-A" must NOT have a terminal pressure when FIELD is terminal)");
    }

    {
        const auto field = injNetwork.node("FIELD"s);

        BOOST_CHECK_EQUAL(field.name(), "FIELD"s);

        BOOST_CHECK_MESSAGE(field.terminal_pressure().has_value(),
                            R"(Injection network node "FIELD" must have a terminal pressure when FIELD is terminal)");

        BOOST_CHECK_CLOSE(field.terminal_pressure().value(),
                          340.0*unit::barsa, 1.0e-6);
    }

    // =======================================================================
    // Branch properties
    // =======================================================================

    {
        const auto f1UpBranch = injNetwork.uptree_branch("F1"s);

        BOOST_REQUIRE_MESSAGE(f1UpBranch.has_value(),
                              R"(There must be an upstream branch attached to injection node "F1")");

        // VFP table 9999 => no flow line.
        BOOST_REQUIRE_MESSAGE(! f1UpBranch->vfp_table().has_value(),
                              R"(There must NOT be a flow line (VFP table) attached to "F1"'s upstream branch.)");
    }

    {
        const auto g1UpBranch = injNetwork.uptree_branch("G1"s);

        BOOST_REQUIRE_MESSAGE(g1UpBranch.has_value(),
                              R"(There must be an upstream branch attached to injection node "G1")");

        // VFP table 9999 => no flow line.
        BOOST_REQUIRE_MESSAGE(! g1UpBranch->vfp_table().has_value(),
                              R"(There must NOT be a flow line (VFP table) attached to "G1"'s upstream branch.)");
    }

    {
        const auto m5nUpBranch = injNetwork.uptree_branch("M5N"s);

        BOOST_REQUIRE_MESSAGE(m5nUpBranch.has_value(),
                              R"(There must be an upstream branch attached to injection node "M5N")");

        BOOST_REQUIRE_MESSAGE(m5nUpBranch->vfp_table().has_value(),
                              R"(There must be a flow line (VFP table) attached to "M5N"'s upstream branch.)");

        BOOST_CHECK_EQUAL(m5nUpBranch->vfp_table().value(), 2);
    }

    {
        const auto m5sUpBranch = injNetwork.uptree_branch("M5S"s);

        BOOST_REQUIRE_MESSAGE(m5sUpBranch.has_value(),
                              R"(There must be an upstream branch attached to injection node "M5S")");

        BOOST_REQUIRE_MESSAGE(m5sUpBranch->vfp_table().has_value(),
                              R"(There must be a flow line (VFP table) attached to "M5S"'s upstream branch.)");

        BOOST_CHECK_EQUAL(m5sUpBranch->vfp_table().value(), 3);
    }

    {
        const auto plat_aUpBranch = injNetwork.uptree_branch("PLAT-A"s);

        BOOST_REQUIRE_MESSAGE(plat_aUpBranch.has_value(),
                              R"(There must be an upstream branch attached to injection node "PLAT-A")");

        // VFP table 9999 => no flow line.
        BOOST_REQUIRE_MESSAGE(! plat_aUpBranch->vfp_table().has_value(),
                              R"(There must NOT be a flow line (VFP table) attached to "PLAT-A"'s upstream branch.)");
    }
}

BOOST_AUTO_TEST_CASE(Reject_InvalidPhase_SOLVENT)
{
    const auto cse = twoD_5x1x2(R"(GNETINJE
 'PLAT-A' 'SOLVENT'  340.0  /
/)");

    BOOST_CHECK(! cse.sched.back().injectionNetwork.has(Phase::GAS));
}

BOOST_AUTO_TEST_CASE(Reject_InvalidPhase_Defaulted)
{
    const auto cse = twoD_5x1x2(R"(GNETINJE
 'PLAT-A'   1*   340.0  /
/)");

    BOOST_CHECK(! cse.sched.back().injectionNetwork.has(Phase::GAS));
}

BOOST_AUTO_TEST_CASE(Reject_InvalidPhase_Typo)
{
    const auto cse = twoD_5x1x2(R"(GNETINJE
 'PLAT-A' 'GAZ'  340.0  /
/)");

    BOOST_CHECK(! cse.sched.back().injectionNetwork.has(Phase::GAS));
}

BOOST_AUTO_TEST_CASE(Reject_NegativeVFPTable)
{
    BOOST_CHECK_THROW(twoD_5x1x2(R"(GNETINJE
 'M5S'  'GAS'  1*  -1  /
/)"), OpmInputError);
}

BOOST_AUTO_TEST_CASE(Reject_TerminalPressureWithPositiveVFP)
{
    BOOST_CHECK_THROW(twoD_5x1x2(R"(GNETINJE
 'PLAT-A'  'GAS'  340.0  3  /
/)"), OpmInputError);
}

BOOST_AUTO_TEST_CASE(Reject_MalformedRecord_WrongType)
{
    BOOST_CHECK_THROW(twoD_5x1x2(R"(GNETINJE
 'M5S'  'GAS'  'NOT-A-NUMBER'  3 /
/)"), std::exception);
}

BOOST_AUTO_TEST_CASE(Reject_MalformedRecord_MissingItem)
{
    BOOST_CHECK_THROW(twoD_5x1x2(R"(GNETINJE
 'M5S  'GAS'  1*  3 /
/)"), std::exception);
}

BOOST_AUTO_TEST_CASE(Reject_UnmatchedGroupPattern)
{
    BOOST_CHECK_THROW(twoD_5x1x2(R"(GNETINJE
 'NO-SUCH*' 'GAS'  340.0  /
/)"), OpmInputError);
}

BOOST_AUTO_TEST_SUITE_END()     // Single_Network_Definition

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Network_With_Updates)

namespace {

    auto networkUpdateChangeFlowLines()
    {
        return twoD_5x1x2(R"(GNETINJE
 'PLAT-A' 'GAS'  340.0  /
 'M5S'    'GAS'   1*   3   /
 'G1'     'GAS'   1*  9999 /
 'M5N'    'GAS'   1*   2   /
 'F1'     'GAS'   1*  9999 /
/
TSTEP
  2*2 /
GNETINJE
 'PLAT-A' 'GAS'  314.159 /
 'M5N'    'GAS'   1*  17 /
 'F1'     'GAS'   1*  29 /
/
)");
    }

    auto networkUpdateDropBranches()
    {
        return twoD_5x1x2(R"(GNETINJE
 'PLAT-A' 'GAS'  340.0  /
 'M5S'    'GAS'   1*   3   /
 'G1'     'GAS'   1*  9999 /
 'M5N'    'GAS'   1*   2   /
 'F1'     'GAS'   1*  9999 /
/
TSTEP
  2*2 /
GNETINJE
 'M5N'    'GAS'  123.4 / -- Drop link to M5S => M5N is new root
 'G1'     'GAS'  /       -- Remove G1 from network
/
)");
    }

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(Topology_Updated_Flow_Lines)
{
    using namespace std::string_literals;

    const auto injNetwork = networkUpdateChangeFlowLines().sched.back()
        .injectionNetwork.get(Phase::GAS);

    BOOST_REQUIRE_MESSAGE(std::ranges::is_permutation
                          (injNetwork.node_names(), std::array {
                              "PLAT-A"s, "M5S"s, "G1"s, "M5N"s, "F1"s,
                          }),
                          R"(Injection network does not have the expected nodes.)");

    // F1 and G1 are leaf nodes
    BOOST_CHECK_MESSAGE(injNetwork.downtree_branches("F1"s).empty(),
                        R"(Injection network node "F1" must be a leaf node.)");

    BOOST_CHECK_MESSAGE(injNetwork.downtree_branches("G1"s).empty(),
                        R"(Injection network node "G1" must be a leaf node.)");

    {
        const auto f1UpBranch = injNetwork.uptree_branch("F1"s);

        BOOST_REQUIRE_MESSAGE(f1UpBranch.has_value(),
                              R"(Injection network node "F1" must be connected to a branch)");

        BOOST_CHECK_EQUAL(f1UpBranch->downtree_node(), "F1"s);
        BOOST_CHECK_EQUAL(f1UpBranch->uptree_node(), "M5N"s);
    }

    {
        const auto g1UpBranch = injNetwork.uptree_branch("G1"s);

        BOOST_REQUIRE_MESSAGE(g1UpBranch.has_value(),
                              R"(Injection network node "G1" must have an upstream branch)");

        BOOST_CHECK_EQUAL(g1UpBranch->downtree_node(), "G1"s);
        BOOST_CHECK_EQUAL(g1UpBranch->uptree_node(), "M5S"s);
    }

    {
        const auto m5nUpBranch = injNetwork.uptree_branch("M5N"s);

        BOOST_REQUIRE_MESSAGE(m5nUpBranch.has_value(),
                              R"(Injection network node "M5N" must have an upstream branch)");

        BOOST_CHECK_EQUAL(m5nUpBranch->downtree_node(), "M5N"s);
        BOOST_CHECK_EQUAL(m5nUpBranch->uptree_node(), "M5S"s);
    }

    {
        const auto m5sUpBranch = injNetwork.uptree_branch("M5S"s);

        BOOST_REQUIRE_MESSAGE(m5sUpBranch.has_value(),
                              R"(Injection network node "M5S" must have an upstream branch)");

        BOOST_CHECK_EQUAL(m5sUpBranch->downtree_node(), "M5S"s);
        BOOST_CHECK_EQUAL(m5sUpBranch->uptree_node(), "PLAT-A"s);
    }

    {
        const auto plat_aUpBranch = injNetwork.uptree_branch("PLAT-A"s);

        BOOST_REQUIRE_MESSAGE(! plat_aUpBranch.has_value(),
                              R"(Injection network node "PLAT-A" must NOT have an upstream branch)");
    }

    {
        const auto f1DownBranch = injNetwork.downtree_branches("F1"s);
        BOOST_CHECK_MESSAGE(f1DownBranch.empty(),
                            R"(Injection network node "F1" must NOT have downstream branches)");
    }

    {
        const auto g1DownBranch = injNetwork.downtree_branches("G1"s);
        BOOST_CHECK_MESSAGE(g1DownBranch.empty(),
                            R"(Injection network node "G1" must NOT have downstream branches)");
    }

    {
        const auto m5nDownBranch = injNetwork.downtree_branches("M5N"s);

        BOOST_REQUIRE_EQUAL(m5nDownBranch.size(), std::size_t{1});

        BOOST_CHECK_EQUAL(m5nDownBranch.front().uptree_node(), "M5N"s);
        BOOST_CHECK_EQUAL(m5nDownBranch.front().downtree_node(), "F1"s);
    }

    {
        const auto m5sDownBranch = injNetwork.downtree_branches("M5S"s);

        BOOST_REQUIRE_EQUAL(m5sDownBranch.size(), std::size_t{2});

        BOOST_CHECK_MESSAGE(std::ranges::equal(m5sDownBranch, std::array { "M5S"s, "M5S"s },
                                               std::ranges::equal_to{},
                                               &Network::Branch::uptree_node),
                            R"(Upstream node must be "M5S")");

        BOOST_CHECK_MESSAGE(std::ranges::is_permutation(m5sDownBranch, std::array { "M5N"s, "G1"s },
                                                        std::ranges::equal_to{},
                                                        &Network::Branch::downtree_node),
                            R"(Downstream nodes must be "M5N" and "G1")");
    }

    {
        const auto plat_aDownBranch = injNetwork.downtree_branches("PLAT-A"s);

        BOOST_REQUIRE_EQUAL(plat_aDownBranch.size(), std::size_t{1});

        BOOST_CHECK_EQUAL(plat_aDownBranch.front().uptree_node(), "PLAT-A"s);
        BOOST_CHECK_EQUAL(plat_aDownBranch.front().downtree_node(), "M5S"s);
    }
}

BOOST_AUTO_TEST_CASE(Properties_Updated_Flow_Lines)
{
    using namespace std::string_literals;

    const auto injNetwork_1 = networkUpdateChangeFlowLines().sched[0].injectionNetwork.get(Phase::GAS);
    const auto injNetwork_2 = networkUpdateChangeFlowLines().sched.back().injectionNetwork.get(Phase::GAS);

    // =======================================================================
    // Node properties
    // =======================================================================

    {
        const auto f1_1 = injNetwork_1.node("F1"s);
        const auto f1_2 = injNetwork_2.node("F1"s);

        BOOST_CHECK_EQUAL(f1_1.name(), "F1"s);
        BOOST_CHECK_MESSAGE(! f1_1.terminal_pressure().has_value(),
                            R"(Injection network node "F1" must NOT have a terminal pressure at start)");

        BOOST_CHECK_EQUAL(f1_2.name(), "F1"s);
        BOOST_CHECK_MESSAGE(! f1_2.terminal_pressure().has_value(),
                            R"(Injection network node "F1" must NOT have a terminal pressure at end)");
    }

    {
        const auto g1_1 = injNetwork_1.node("G1"s);
        const auto g1_2 = injNetwork_2.node("G1"s);

        BOOST_CHECK_EQUAL(g1_1.name(), "G1"s);
        BOOST_CHECK_MESSAGE(! g1_1.terminal_pressure().has_value(),
                            R"(Injection network node "G1" must NOT have a terminal pressure at start)");

        BOOST_CHECK_EQUAL(g1_2.name(), "G1"s);
        BOOST_CHECK_MESSAGE(! g1_2.terminal_pressure().has_value(),
                            R"(Injection network node "G1" must NOT have a terminal pressure at end)");
    }

    {
        const auto m5n_1 = injNetwork_1.node("M5N"s);
        const auto m5n_2 = injNetwork_2.node("M5N"s);

        BOOST_CHECK_EQUAL(m5n_1.name(), "M5N"s);
        BOOST_CHECK_MESSAGE(! m5n_1.terminal_pressure().has_value(),
                            R"(Injection network node "M5N" must NOT have a terminal pressure at start)");

        BOOST_CHECK_EQUAL(m5n_2.name(), "M5N"s);
        BOOST_CHECK_MESSAGE(! m5n_2.terminal_pressure().has_value(),
                            R"(Injection network node "M5N" must NOT have a terminal pressure at end)");
    }

    {
        const auto m5s_1 = injNetwork_1.node("M5S"s);
        const auto m5s_2 = injNetwork_2.node("M5S"s);

        BOOST_CHECK_EQUAL(m5s_1.name(), "M5S"s);
        BOOST_CHECK_MESSAGE(! m5s_1.terminal_pressure().has_value(),
                            R"(Injection network node "M5S" must NOT have a terminal pressure at start)");

        BOOST_CHECK_EQUAL(m5s_2.name(), "M5S"s);
        BOOST_CHECK_MESSAGE(! m5s_2.terminal_pressure().has_value(),
                            R"(Injection network node "M5S" must NOT have a terminal pressure at end)");
    }

    {
        const auto plat_a_1 = injNetwork_1.node("PLAT-A"s);
        const auto plat_a_2 = injNetwork_2.node("PLAT-A"s);

        BOOST_CHECK_EQUAL(plat_a_1.name(), "PLAT-A"s);

        BOOST_CHECK_MESSAGE(plat_a_1.terminal_pressure().has_value(),
                            R"(Injection network node "PLAT-A" must have a terminal pressure at start)");

        BOOST_CHECK_CLOSE(plat_a_1.terminal_pressure().value(),
                          340.0*unit::barsa, 1.0e-6);

        BOOST_CHECK_EQUAL(plat_a_2.name(), "PLAT-A"s);

        BOOST_CHECK_MESSAGE(plat_a_2.terminal_pressure().has_value(),
                            R"(Injection network node "PLAT-A" must have a terminal pressure at end)");

        BOOST_CHECK_CLOSE(plat_a_2.terminal_pressure().value(),
                          314.159*unit::barsa, 1.0e-6);
    }

    // =======================================================================
    // Branch properties
    // =======================================================================

    {
        const auto f1UpBranch_1 = injNetwork_1.uptree_branch("F1"s);
        const auto f1UpBranch_2 = injNetwork_2.uptree_branch("F1"s);

        BOOST_REQUIRE_MESSAGE(f1UpBranch_1.has_value(),
                              R"(There must be an upstream branch attached to injection node "F1" at start)");

        // VFP table 9999 => no flow line.
        BOOST_REQUIRE_MESSAGE(! f1UpBranch_1->vfp_table().has_value(),
                              R"(There must NOT be a flow line (VFP table) attached to "F1"'s upstream branch at start.)");

        BOOST_REQUIRE_MESSAGE(f1UpBranch_2.has_value(),
                              R"(There must be an upstream branch attached to injection node "F1" at end)");

        // VFP table 29 => must have flow line.
        BOOST_REQUIRE_MESSAGE(f1UpBranch_2->vfp_table().has_value(),
                              R"(There must be a flow line (VFP table) attached to "F1"'s upstream branch at end.)");

        BOOST_REQUIRE_EQUAL(*f1UpBranch_2->vfp_table(), 29);
    }

    {
        const auto g1UpBranch_1 = injNetwork_1.uptree_branch("G1"s);
        const auto g1UpBranch_2 = injNetwork_2.uptree_branch("G1"s);

        BOOST_REQUIRE_MESSAGE(g1UpBranch_1.has_value(),
                              R"(There must be an upstream branch attached to injection node "G1" at start)");

        // VFP table 9999 => no flow line.
        BOOST_REQUIRE_MESSAGE(! g1UpBranch_1->vfp_table().has_value(),
                              R"(There must NOT be a flow line (VFP table) attached to "G1"'s upstream branch at start.)");

        BOOST_REQUIRE_MESSAGE(g1UpBranch_2.has_value(),
                              R"(There must be an upstream branch attached to injection node "G1" at end)");

        // VFP table 9999 => no flow line.
        BOOST_REQUIRE_MESSAGE(! g1UpBranch_2->vfp_table().has_value(),
                              R"(There must NOT be a flow line (VFP table) attached to "G1"'s upstream branch at end.)");
    }

    {
        const auto m5nUpBranch_1 = injNetwork_1.uptree_branch("M5N"s);
        const auto m5nUpBranch_2 = injNetwork_2.uptree_branch("M5N"s);

        BOOST_REQUIRE_MESSAGE(m5nUpBranch_1.has_value(),
                              R"(There must be an upstream branch attached to injection node "M5N" at start)");

        BOOST_REQUIRE_MESSAGE(m5nUpBranch_1->vfp_table().has_value(),
                              R"(There must be a flow line (VFP table) attached to "M5N"'s upstream branch at start.)");

        BOOST_CHECK_EQUAL(m5nUpBranch_1->vfp_table().value(), 2);

        BOOST_REQUIRE_MESSAGE(m5nUpBranch_2.has_value(),
                              R"(There must be an upstream branch attached to injection node "M5N" at end)");

        BOOST_REQUIRE_MESSAGE(m5nUpBranch_2->vfp_table().has_value(),
                              R"(There must be a flow line (VFP table) attached to "M5N"'s upstream branch at end.)");

        BOOST_CHECK_EQUAL(m5nUpBranch_2->vfp_table().value(), 17);
    }

    {
        const auto m5sUpBranch_1 = injNetwork_1.uptree_branch("M5S"s);
        const auto m5sUpBranch_2 = injNetwork_2.uptree_branch("M5S"s);

        BOOST_REQUIRE_MESSAGE(m5sUpBranch_1.has_value(),
                              R"(There must be an upstream branch attached to injection node "M5S" at start)");

        BOOST_REQUIRE_MESSAGE(m5sUpBranch_1->vfp_table().has_value(),
                              R"(There must be a flow line (VFP table) attached to "M5S"'s upstream branch at start.)");

        BOOST_CHECK_EQUAL(m5sUpBranch_1->vfp_table().value(), 3);

        BOOST_REQUIRE_MESSAGE(m5sUpBranch_2.has_value(),
                              R"(There must be an upstream branch attached to injection node "M5S" at end)");

        BOOST_REQUIRE_MESSAGE(m5sUpBranch_2->vfp_table().has_value(),
                              R"(There must be a flow line (VFP table) attached to "M5S"'s upstream branch at end.)");

        BOOST_CHECK_EQUAL(m5sUpBranch_2->vfp_table().value(), 3);
    }
}

BOOST_AUTO_TEST_CASE(Topology_Dropped_Branches)
{
    using namespace std::string_literals;

    const auto injNetwork = networkUpdateDropBranches().sched.back()
        .injectionNetwork.get(Phase::GAS);

    // Note: Even when we remove branches, the original nodes remain in the
    // network object.  This is arguably a semantic pitfall and we might
    // consider adding a way to prune nodes that cannot be reached.
    BOOST_REQUIRE_MESSAGE(std::ranges::is_permutation
                          (injNetwork.node_names(), std::array {
                              "PLAT-A"s, "M5S"s, "G1"s, "M5N"s, "F1"s,
                          }),
                          R"(Injection network does not have the expected nodes.)");

    // F1 and G1 must both exist, but G1 must *not* have any branches.
    BOOST_CHECK_MESSAGE(injNetwork.downtree_branches("F1"s).empty(),
                        R"(Injection network node "F1" must be a leaf node.)");

    BOOST_CHECK_MESSAGE(injNetwork.downtree_branches("G1"s).empty(),
                        R"(Injection network node "G1" must NOT have any downstream branches at end.)");

    BOOST_CHECK_MESSAGE(! injNetwork.uptree_branch("G1"s).has_value(),
                        R"(Injection network node "G1" must NOT have an upstream branch at end.)");

    {
        const auto roots = injNetwork.roots();

        BOOST_REQUIRE_EQUAL(roots.size(), std::size_t{2});

        BOOST_CHECK_MESSAGE(std::ranges::is_permutation
                            (roots, std::array { "PLAT-A"s, "M5N"s },
                             std::ranges::equal_to{},
                             [](const auto& node) { return node.get().name(); }),
                            R"(Injection network must have root nodes "PLAT-A" and "M5N" at end.)");
    }

    {
        const auto f1UpBranch = injNetwork.uptree_branch("F1"s);

        BOOST_REQUIRE_MESSAGE(f1UpBranch.has_value(),
                              R"(Injection network node "F1" must be connected to a branch)");

        BOOST_CHECK_EQUAL(f1UpBranch->downtree_node(), "F1"s);
        BOOST_CHECK_EQUAL(f1UpBranch->uptree_node(), "M5N"s);
    }

    BOOST_REQUIRE_MESSAGE(! injNetwork.uptree_branch("M5N"s).has_value(),
                          R"(Injection network node "M5N" must NOT have an upstream branch at end.)");

    {
        const auto m5sUpBranch = injNetwork.uptree_branch("M5S"s);

        BOOST_REQUIRE_MESSAGE(m5sUpBranch.has_value(),
                              R"(Injection network node "M5S" must have an upstream branch at end.)");

        BOOST_CHECK_EQUAL(m5sUpBranch->downtree_node(), "M5S"s);
        BOOST_CHECK_EQUAL(m5sUpBranch->uptree_node(), "PLAT-A"s);
    }

    BOOST_REQUIRE_MESSAGE(! injNetwork.uptree_branch("PLAT-A"s).has_value(),
                          R"(Injection network node "PLAT-A" must NOT have an upstream branch)");

    {
        const auto m5nDownBranch = injNetwork.downtree_branches("M5N"s);

        BOOST_REQUIRE_EQUAL(m5nDownBranch.size(), std::size_t{1});

        BOOST_CHECK_EQUAL(m5nDownBranch.front().uptree_node(), "M5N"s);
        BOOST_CHECK_EQUAL(m5nDownBranch.front().downtree_node(), "F1"s);
    }

    BOOST_REQUIRE_MESSAGE(injNetwork.downtree_branches("M5S"s).empty(),
                          R"(Injection network node "M5S" must NOT have any downstream branches at end.)");

    {
        const auto plat_aDownBranch = injNetwork.downtree_branches("PLAT-A"s);

        BOOST_REQUIRE_EQUAL(plat_aDownBranch.size(), std::size_t{1});

        BOOST_CHECK_EQUAL(plat_aDownBranch.front().uptree_node(), "PLAT-A"s);
        BOOST_CHECK_EQUAL(plat_aDownBranch.front().downtree_node(), "M5S"s);
    }
}

BOOST_AUTO_TEST_CASE(Properties_Dropped_Branches)
{
    using namespace std::string_literals;

    const auto injNetwork_1 = networkUpdateDropBranches().sched[0].injectionNetwork.get(Phase::GAS);
    const auto injNetwork_2 = networkUpdateDropBranches().sched.back().injectionNetwork.get(Phase::GAS);

    // =======================================================================
    // Node properties
    // =======================================================================

    {
        const auto f1_1 = injNetwork_1.node("F1"s);
        const auto f1_2 = injNetwork_2.node("F1"s);

        BOOST_CHECK_EQUAL(f1_1.name(), "F1"s);
        BOOST_CHECK_MESSAGE(! f1_1.terminal_pressure().has_value(),
                            R"(Injection network node "F1" must NOT have a terminal pressure at start)");

        BOOST_CHECK_EQUAL(f1_2.name(), "F1"s);
        BOOST_CHECK_MESSAGE(! f1_2.terminal_pressure().has_value(),
                            R"(Injection network node "F1" must NOT have a terminal pressure at end)");
    }

    {
        const auto g1_1 = injNetwork_1.node("G1"s);
        const auto g1_2 = injNetwork_2.node("G1"s);

        BOOST_CHECK_EQUAL(g1_1.name(), "G1"s);
        BOOST_CHECK_MESSAGE(! g1_1.terminal_pressure().has_value(),
                            R"(Injection network node "G1" must NOT have a terminal pressure at start)");

        BOOST_CHECK_EQUAL(g1_2.name(), "G1"s);
        BOOST_CHECK_MESSAGE(! g1_2.terminal_pressure().has_value(),
                            R"(Injection network node "G1" must NOT have a terminal pressure at end)");
    }

    {
        const auto m5n_1 = injNetwork_1.node("M5N"s);
        const auto m5n_2 = injNetwork_2.node("M5N"s);

        BOOST_CHECK_EQUAL(m5n_1.name(), "M5N"s);
        BOOST_CHECK_MESSAGE(! m5n_1.terminal_pressure().has_value(),
                            R"(Injection network node "M5N" must NOT have a terminal pressure at start)");

        BOOST_CHECK_EQUAL(m5n_2.name(), "M5N"s);
        BOOST_CHECK_MESSAGE(m5n_2.terminal_pressure().has_value(),
                            R"(Injection network node "M5N" must have a terminal pressure at end)");

        BOOST_CHECK_CLOSE(*m5n_2.terminal_pressure(),
                          123.4*unit::barsa, 1.0e-6);
    }

    {
        const auto m5s_1 = injNetwork_1.node("M5S"s);
        const auto m5s_2 = injNetwork_2.node("M5S"s);

        BOOST_CHECK_EQUAL(m5s_1.name(), "M5S"s);
        BOOST_CHECK_MESSAGE(! m5s_1.terminal_pressure().has_value(),
                            R"(Injection network node "M5S" must NOT have a terminal pressure at start)");

        BOOST_CHECK_EQUAL(m5s_2.name(), "M5S"s);
        BOOST_CHECK_MESSAGE(! m5s_2.terminal_pressure().has_value(),
                            R"(Injection network node "M5S" must NOT have a terminal pressure at end)");
    }

    {
        const auto plat_a_1 = injNetwork_1.node("PLAT-A"s);
        const auto plat_a_2 = injNetwork_2.node("PLAT-A"s);

        BOOST_CHECK_EQUAL(plat_a_1.name(), "PLAT-A"s);

        BOOST_CHECK_MESSAGE(plat_a_1.terminal_pressure().has_value(),
                            R"(Injection network node "PLAT-A" must have a terminal pressure at start)");

        BOOST_CHECK_CLOSE(plat_a_1.terminal_pressure().value(),
                          340.0*unit::barsa, 1.0e-6);

        BOOST_CHECK_EQUAL(plat_a_2.name(), "PLAT-A"s);

        BOOST_CHECK_MESSAGE(plat_a_2.terminal_pressure().has_value(),
                            R"(Injection network node "PLAT-A" must have a terminal pressure at end)");

        BOOST_CHECK_CLOSE(plat_a_2.terminal_pressure().value(),
                          340.0*unit::barsa, 1.0e-6);
    }

    // =======================================================================
    // Branch properties
    // =======================================================================

    {
        const auto f1UpBranch_1 = injNetwork_1.uptree_branch("F1"s);
        const auto f1UpBranch_2 = injNetwork_2.uptree_branch("F1"s);

        BOOST_REQUIRE_MESSAGE(f1UpBranch_1.has_value(),
                              R"(There must be an upstream branch attached to injection node "F1" at start)");

        // VFP table 9999 => no flow line.
        BOOST_REQUIRE_MESSAGE(! f1UpBranch_1->vfp_table().has_value(),
                              R"(There must NOT be a flow line (VFP table) attached to "F1"'s upstream branch at start.)");

        BOOST_REQUIRE_MESSAGE(f1UpBranch_2.has_value(),
                              R"(There must be an upstream branch attached to injection node "F1" at end)");

        // VFP table 9999 => no flow line.
        BOOST_REQUIRE_MESSAGE(! f1UpBranch_2->vfp_table().has_value(),
                              R"(There must NOT be a flow line (VFP table) attached to "F1"'s upstream branch at end.)");
    }

    {
        const auto g1UpBranch_1 = injNetwork_1.uptree_branch("G1"s);
        const auto g1UpBranch_2 = injNetwork_2.uptree_branch("G1"s);

        BOOST_REQUIRE_MESSAGE(g1UpBranch_1.has_value(),
                              R"(There must be an upstream branch attached to injection node "G1" at start)");

        // VFP table 9999 => no flow line.
        BOOST_REQUIRE_MESSAGE(! g1UpBranch_1->vfp_table().has_value(),
                              R"(There must NOT be a flow line (VFP table) attached to "G1"'s upstream branch at start.)");

        BOOST_REQUIRE_MESSAGE(! g1UpBranch_2.has_value(),
                              R"(There must NOT be an upstream branch attached to injection node "G1" at end)");
    }

    {
        const auto m5nUpBranch_1 = injNetwork_1.uptree_branch("M5N"s);
        const auto m5nUpBranch_2 = injNetwork_2.uptree_branch("M5N"s);

        BOOST_REQUIRE_MESSAGE(m5nUpBranch_1.has_value(),
                              R"(There must be an upstream branch attached to injection node "M5N" at start)");

        BOOST_REQUIRE_MESSAGE(m5nUpBranch_1->vfp_table().has_value(),
                              R"(There must be a flow line (VFP table) attached to "M5N"'s upstream branch at start.)");

        BOOST_CHECK_EQUAL(m5nUpBranch_1->vfp_table().value(), 2);

        BOOST_REQUIRE_MESSAGE(! m5nUpBranch_2.has_value(),
                              R"(There must NOT be an upstream branch attached to injection node "M5N" at end)");
    }

    {
        const auto m5sUpBranch_1 = injNetwork_1.uptree_branch("M5S"s);
        const auto m5sUpBranch_2 = injNetwork_2.uptree_branch("M5S"s);

        BOOST_REQUIRE_MESSAGE(m5sUpBranch_1.has_value(),
                              R"(There must be an upstream branch attached to injection node "M5S" at start)");

        BOOST_REQUIRE_MESSAGE(m5sUpBranch_1->vfp_table().has_value(),
                              R"(There must be a flow line (VFP table) attached to "M5S"'s upstream branch at start.)");

        BOOST_CHECK_EQUAL(m5sUpBranch_1->vfp_table().value(), 3);

        BOOST_REQUIRE_MESSAGE(m5sUpBranch_2.has_value(),
                              R"(There must be an upstream branch attached to injection node "M5S" at end)");

        BOOST_REQUIRE_MESSAGE(m5sUpBranch_2->vfp_table().has_value(),
                              R"(There must be a flow line (VFP table) attached to "M5S"'s upstream branch at end.)");

        BOOST_CHECK_EQUAL(m5sUpBranch_2->vfp_table().value(), 3);
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Network_With_Updates

BOOST_AUTO_TEST_SUITE_END()     // Gas

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Water)

namespace {
    auto wellDefined()
    {
        return twoD_5x1x2(R"(GNETINJE
 'PLAT-A' 'WAT'  160.0  /
 'M5S'    'WAT'   1*   3   /
 'G1'     'WAT'   1*  9999 /
 'M5N'    'WAT'   1*   4   /
 'F1'     'WAT'   1*  9999 /
/
)");
    }

    auto recordMatchMultipleGroups()
    {
        return twoD_5x1x2(R"(GNETINJE
 'PLAT-A' 'WAT'  160.0  /
 'M5*'    'WAT'   1*  1729 /
 'G1'     'WAT'   1*  9999 /
 'F1'     'WAT'   1*  9999 /
/
)");
    }
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(Existence)
{
    const auto cse = wellDefined();

    BOOST_REQUIRE_EQUAL(cse.sched.size(), std::size_t{3});

    BOOST_CHECK_MESSAGE(cse.sched[0].injectionNetwork.has(Phase::WATER),
                        R"(There must be an injection network for the "WATER" phase at time zero.)");

    BOOST_CHECK_MESSAGE(cse.sched.back().injectionNetwork.has(Phase::WATER),
                        R"(There must be an injection network for the "WATER" phase.)");

    BOOST_CHECK_MESSAGE(! cse.sched.back().injectionNetwork.has(Phase::GAS),
                        R"(There must NOT be an injection network for the "GAS" phase.)");
}

BOOST_AUTO_TEST_CASE(Topology)
{
    using namespace std::string_literals;

    const auto injNetwork = wellDefined().sched.back().injectionNetwork.get(Phase::WATER);

    BOOST_REQUIRE_MESSAGE(std::ranges::is_permutation
                          (injNetwork.node_names(), std::array {
                              "PLAT-A"s, "M5S"s, "G1"s, "M5N"s, "F1"s,
                          }),
                          R"(Injection network does not have the expected nodes.)");

    // F1 and G1 are leaf nodes
    BOOST_CHECK_MESSAGE(injNetwork.downtree_branches("F1"s).empty(),
                        R"(Injection network node "F1" must be a leaf node.)");

    BOOST_CHECK_MESSAGE(injNetwork.downtree_branches("G1"s).empty(),
                        R"(Injection network node "G1" must be a leaf node.)");

    {
        const auto f1UpBranch = injNetwork.uptree_branch("F1"s);

        BOOST_REQUIRE_MESSAGE(f1UpBranch.has_value(),
                              R"(Injection network node "F1" must be connected to a branch)");

        BOOST_CHECK_EQUAL(f1UpBranch->downtree_node(), "F1"s);
        BOOST_CHECK_EQUAL(f1UpBranch->uptree_node(), "M5N"s);
    }

    {
        const auto g1UpBranch = injNetwork.uptree_branch("G1"s);

        BOOST_REQUIRE_MESSAGE(g1UpBranch.has_value(),
                              R"(Injection network node "G1" must have an upstream branch)");

        BOOST_CHECK_EQUAL(g1UpBranch->downtree_node(), "G1"s);
        BOOST_CHECK_EQUAL(g1UpBranch->uptree_node(), "M5S"s);
    }

    {
        const auto m5nUpBranch = injNetwork.uptree_branch("M5N"s);

        BOOST_REQUIRE_MESSAGE(m5nUpBranch.has_value(),
                              R"(Injection network node "M5N" must have an upstream branch)");

        BOOST_CHECK_EQUAL(m5nUpBranch->downtree_node(), "M5N"s);
        BOOST_CHECK_EQUAL(m5nUpBranch->uptree_node(), "M5S"s);
    }

    {
        const auto m5sUpBranch = injNetwork.uptree_branch("M5S"s);

        BOOST_REQUIRE_MESSAGE(m5sUpBranch.has_value(),
                              R"(Injection network node "M5S" must have an upstream branch)");

        BOOST_CHECK_EQUAL(m5sUpBranch->downtree_node(), "M5S"s);
        BOOST_CHECK_EQUAL(m5sUpBranch->uptree_node(), "PLAT-A"s);
    }

    {
        const auto plat_aUpBranch = injNetwork.uptree_branch("PLAT-A"s);

        BOOST_REQUIRE_MESSAGE(! plat_aUpBranch.has_value(),
                              R"(Injection network node "PLAT-A" must NOT have an upstream branch)");
    }

    {
        const auto f1DownBranch = injNetwork.downtree_branches("F1"s);
        BOOST_CHECK_MESSAGE(f1DownBranch.empty(),
                            R"(Injection network node "F1" must NOT have downstream branches)");
    }

    {
        const auto g1DownBranch = injNetwork.downtree_branches("G1"s);
        BOOST_CHECK_MESSAGE(g1DownBranch.empty(),
                            R"(Injection network node "G1" must NOT have downstream branches)");
    }

    {
        const auto m5nDownBranch = injNetwork.downtree_branches("M5N"s);

        BOOST_REQUIRE_EQUAL(m5nDownBranch.size(), std::size_t{1});

        BOOST_CHECK_EQUAL(m5nDownBranch.front().uptree_node(), "M5N"s);
        BOOST_CHECK_EQUAL(m5nDownBranch.front().downtree_node(), "F1"s);
    }

    {
        const auto m5sDownBranch = injNetwork.downtree_branches("M5S"s);

        BOOST_REQUIRE_EQUAL(m5sDownBranch.size(), std::size_t{2});

        BOOST_CHECK_MESSAGE(std::ranges::equal(m5sDownBranch, std::array { "M5S"s, "M5S"s },
                                               std::ranges::equal_to{},
                                               &Network::Branch::uptree_node),
                            R"(Upstream node must be "M5S")");

        BOOST_CHECK_MESSAGE(std::ranges::is_permutation(m5sDownBranch, std::array { "M5N"s, "G1"s },
                                                        std::ranges::equal_to{},
                                                        &Network::Branch::downtree_node),
                            R"(Downstream nodes must be "M5N" and "G1")");
    }

    {
        const auto plat_aDownBranch = injNetwork.downtree_branches("PLAT-A"s);

        BOOST_REQUIRE_EQUAL(plat_aDownBranch.size(), std::size_t{1});

        BOOST_CHECK_EQUAL(plat_aDownBranch.front().uptree_node(), "PLAT-A"s);
        BOOST_CHECK_EQUAL(plat_aDownBranch.front().downtree_node(), "M5S"s);
    }
}

BOOST_AUTO_TEST_CASE(Properties)
{
    using namespace std::string_literals;

    const auto injNetwork = wellDefined().sched.back().injectionNetwork.get(Phase::WATER);

    // =======================================================================
    // Node properties
    // =======================================================================

    {
        const auto f1 = injNetwork.node("F1"s);

        BOOST_CHECK_EQUAL(f1.name(), "F1"s);
        BOOST_CHECK_MESSAGE(! f1.terminal_pressure().has_value(),
                            R"(Injection network node "F1" must NOT have a terminal pressure)");
    }

    {
        const auto g1 = injNetwork.node("G1"s);

        BOOST_CHECK_EQUAL(g1.name(), "G1"s);
        BOOST_CHECK_MESSAGE(! g1.terminal_pressure().has_value(),
                            R"(Injection network node "G1" must NOT have a terminal pressure)");
    }

    {
        const auto m5n = injNetwork.node("M5N"s);

        BOOST_CHECK_EQUAL(m5n.name(), "M5N"s);
        BOOST_CHECK_MESSAGE(! m5n.terminal_pressure().has_value(),
                            R"(Injection network node "M5N" must NOT have a terminal pressure)");
    }

    {
        const auto m5s = injNetwork.node("M5S"s);

        BOOST_CHECK_EQUAL(m5s.name(), "M5S"s);
        BOOST_CHECK_MESSAGE(! m5s.terminal_pressure().has_value(),
                            R"(Injection network node "M5S" must NOT have a terminal pressure)");
    }

    {
        const auto plat_a = injNetwork.node("PLAT-A"s);

        BOOST_CHECK_EQUAL(plat_a.name(), "PLAT-A"s);

        BOOST_CHECK_MESSAGE(plat_a.terminal_pressure().has_value(),
                            R"(Injection network node "PLAT-A" must have a terminal pressure)");

        BOOST_CHECK_CLOSE(plat_a.terminal_pressure().value(),
                          160.0*unit::barsa, 1.0e-6);
    }

    // =======================================================================
    // Branch properties
    // =======================================================================

    {
        const auto f1UpBranch = injNetwork.uptree_branch("F1"s);

        BOOST_REQUIRE_MESSAGE(f1UpBranch.has_value(),
                              R"(There must be an upstream branch attached to injection node "F1")");

        // VFP table 9999 => no flow line.
        BOOST_REQUIRE_MESSAGE(! f1UpBranch->vfp_table().has_value(),
                              R"(There must NOT be a flow line (VFP table) attached to "F1"'s upstream branch.)");
    }

    {
        const auto g1UpBranch = injNetwork.uptree_branch("G1"s);

        BOOST_REQUIRE_MESSAGE(g1UpBranch.has_value(),
                              R"(There must be an upstream branch attached to injection node "G1")");

        // VFP table 9999 => no flow line.
        BOOST_REQUIRE_MESSAGE(! g1UpBranch->vfp_table().has_value(),
                              R"(There must NOT be a flow line (VFP table) attached to "G1"'s upstream branch.)");
    }

    {
        const auto m5nUpBranch = injNetwork.uptree_branch("M5N"s);

        BOOST_REQUIRE_MESSAGE(m5nUpBranch.has_value(),
                              R"(There must be an upstream branch attached to injection node "M5N")");

        BOOST_REQUIRE_MESSAGE(m5nUpBranch->vfp_table().has_value(),
                              R"(There must be a flow line (VFP table) attached to "M5N"'s upstream branch.)");

        BOOST_CHECK_EQUAL(m5nUpBranch->vfp_table().value(), 4);
    }

    {
        const auto m5sUpBranch = injNetwork.uptree_branch("M5S"s);

        BOOST_REQUIRE_MESSAGE(m5sUpBranch.has_value(),
                              R"(There must be an upstream branch attached to injection node "M5S")");

        BOOST_REQUIRE_MESSAGE(m5sUpBranch->vfp_table().has_value(),
                              R"(There must be a flow line (VFP table) attached to "M5S"'s upstream branch.)");

        BOOST_CHECK_EQUAL(m5sUpBranch->vfp_table().value(), 3);
    }
}

BOOST_AUTO_TEST_CASE(Existence_Match_Multi)
{
    const auto cse = recordMatchMultipleGroups();

    BOOST_REQUIRE_EQUAL(cse.sched.size(), std::size_t{3});

    BOOST_CHECK_MESSAGE(cse.sched[0].injectionNetwork.has(Phase::WATER),
                        R"(There must be an injection network for the "WATER" phase at time zero.)");

    BOOST_CHECK_MESSAGE(cse.sched.back().injectionNetwork.has(Phase::WATER),
                        R"(There must be an injection network for the "WATER" phase.)");

    BOOST_CHECK_MESSAGE(! cse.sched.back().injectionNetwork.has(Phase::GAS),
                        R"(There must NOT be an injection network for the "GAS" phase.)");
}

BOOST_AUTO_TEST_CASE(Topology_Match_Multi)
{
    using namespace std::string_literals;

    const auto injNetwork = recordMatchMultipleGroups().sched.back().injectionNetwork.get(Phase::WATER);

    BOOST_REQUIRE_MESSAGE(std::ranges::is_permutation
                          (injNetwork.node_names(), std::array {
                              "PLAT-A"s, "M5S"s, "G1"s, "M5N"s, "F1"s,
                          }),
                          R"(Injection network does not have the expected nodes.)");

    // F1 and G1 are leaf nodes
    BOOST_CHECK_MESSAGE(injNetwork.downtree_branches("F1"s).empty(),
                        R"(Injection network node "F1" must be a leaf node.)");

    BOOST_CHECK_MESSAGE(injNetwork.downtree_branches("G1"s).empty(),
                        R"(Injection network node "G1" must be a leaf node.)");

    {
        const auto f1UpBranch = injNetwork.uptree_branch("F1"s);

        BOOST_REQUIRE_MESSAGE(f1UpBranch.has_value(),
                              R"(Injection network node "F1" must be connected to a branch)");

        BOOST_CHECK_EQUAL(f1UpBranch->downtree_node(), "F1"s);
        BOOST_CHECK_EQUAL(f1UpBranch->uptree_node(), "M5N"s);
    }

    {
        const auto g1UpBranch = injNetwork.uptree_branch("G1"s);

        BOOST_REQUIRE_MESSAGE(g1UpBranch.has_value(),
                              R"(Injection network node "G1" must have an upstream branch)");

        BOOST_CHECK_EQUAL(g1UpBranch->downtree_node(), "G1"s);
        BOOST_CHECK_EQUAL(g1UpBranch->uptree_node(), "M5S"s);
    }

    {
        const auto m5nUpBranch = injNetwork.uptree_branch("M5N"s);

        BOOST_REQUIRE_MESSAGE(m5nUpBranch.has_value(),
                              R"(Injection network node "M5N" must have an upstream branch)");

        BOOST_CHECK_EQUAL(m5nUpBranch->downtree_node(), "M5N"s);
        BOOST_CHECK_EQUAL(m5nUpBranch->uptree_node(), "M5S"s);
    }

    {
        const auto m5sUpBranch = injNetwork.uptree_branch("M5S"s);

        BOOST_REQUIRE_MESSAGE(m5sUpBranch.has_value(),
                              R"(Injection network node "M5S" must have an upstream branch)");

        BOOST_CHECK_EQUAL(m5sUpBranch->downtree_node(), "M5S"s);
        BOOST_CHECK_EQUAL(m5sUpBranch->uptree_node(), "PLAT-A"s);
    }

    {
        const auto plat_aUpBranch = injNetwork.uptree_branch("PLAT-A"s);

        BOOST_REQUIRE_MESSAGE(! plat_aUpBranch.has_value(),
                              R"(Injection network node "PLAT-A" must NOT have an upstream branch)");
    }

    {
        const auto f1DownBranch = injNetwork.downtree_branches("F1"s);
        BOOST_CHECK_MESSAGE(f1DownBranch.empty(),
                            R"(Injection network node "F1" must NOT have downstream branches)");
    }

    {
        const auto g1DownBranch = injNetwork.downtree_branches("G1"s);
        BOOST_CHECK_MESSAGE(g1DownBranch.empty(),
                            R"(Injection network node "G1" must NOT have downstream branches)");
    }

    {
        const auto m5nDownBranch = injNetwork.downtree_branches("M5N"s);

        BOOST_REQUIRE_EQUAL(m5nDownBranch.size(), std::size_t{1});

        BOOST_CHECK_EQUAL(m5nDownBranch.front().uptree_node(), "M5N"s);
        BOOST_CHECK_EQUAL(m5nDownBranch.front().downtree_node(), "F1"s);
    }

    {
        const auto m5sDownBranch = injNetwork.downtree_branches("M5S"s);

        BOOST_REQUIRE_EQUAL(m5sDownBranch.size(), std::size_t{2});

        BOOST_CHECK_MESSAGE(std::ranges::equal(m5sDownBranch, std::array { "M5S"s, "M5S"s },
                                               std::ranges::equal_to{},
                                               &Network::Branch::uptree_node),
                            R"(Upstream node must be "M5S")");

        BOOST_CHECK_MESSAGE(std::ranges::is_permutation(m5sDownBranch, std::array { "M5N"s, "G1"s },
                                                        std::ranges::equal_to{},
                                                        &Network::Branch::downtree_node),
                            R"(Downstream nodes must be "M5N" and "G1")");
    }

    {
        const auto plat_aDownBranch = injNetwork.downtree_branches("PLAT-A"s);

        BOOST_REQUIRE_EQUAL(plat_aDownBranch.size(), std::size_t{1});

        BOOST_CHECK_EQUAL(plat_aDownBranch.front().uptree_node(), "PLAT-A"s);
        BOOST_CHECK_EQUAL(plat_aDownBranch.front().downtree_node(), "M5S"s);
    }
}

BOOST_AUTO_TEST_CASE(Properties_Match_Multi)
{
    using namespace std::string_literals;

    const auto injNetwork = recordMatchMultipleGroups().sched.back().injectionNetwork.get(Phase::WATER);

    // =======================================================================
    // Node properties
    // =======================================================================

    {
        const auto f1 = injNetwork.node("F1"s);

        BOOST_CHECK_EQUAL(f1.name(), "F1"s);
        BOOST_CHECK_MESSAGE(! f1.terminal_pressure().has_value(),
                            R"(Injection network node "F1" must NOT have a terminal pressure)");
    }

    {
        const auto g1 = injNetwork.node("G1"s);

        BOOST_CHECK_EQUAL(g1.name(), "G1"s);
        BOOST_CHECK_MESSAGE(! g1.terminal_pressure().has_value(),
                            R"(Injection network node "G1" must NOT have a terminal pressure)");
    }

    {
        const auto m5n = injNetwork.node("M5N"s);

        BOOST_CHECK_EQUAL(m5n.name(), "M5N"s);
        BOOST_CHECK_MESSAGE(! m5n.terminal_pressure().has_value(),
                            R"(Injection network node "M5N" must NOT have a terminal pressure)");
    }

    {
        const auto m5s = injNetwork.node("M5S"s);

        BOOST_CHECK_EQUAL(m5s.name(), "M5S"s);
        BOOST_CHECK_MESSAGE(! m5s.terminal_pressure().has_value(),
                            R"(Injection network node "M5S" must NOT have a terminal pressure)");
    }

    {
        const auto plat_a = injNetwork.node("PLAT-A"s);

        BOOST_CHECK_EQUAL(plat_a.name(), "PLAT-A"s);

        BOOST_CHECK_MESSAGE(plat_a.terminal_pressure().has_value(),
                            R"(Injection network node "PLAT-A" must have a terminal pressure)");

        BOOST_CHECK_CLOSE(plat_a.terminal_pressure().value(),
                          160.0*unit::barsa, 1.0e-6);
    }

    // =======================================================================
    // Branch properties
    // =======================================================================

    {
        const auto f1UpBranch = injNetwork.uptree_branch("F1"s);

        BOOST_REQUIRE_MESSAGE(f1UpBranch.has_value(),
                              R"(There must be an upstream branch attached to injection node "F1")");

        // VFP table 9999 => no flow line.
        BOOST_REQUIRE_MESSAGE(! f1UpBranch->vfp_table().has_value(),
                              R"(There must NOT be a flow line (VFP table) attached to "F1"'s upstream branch.)");
    }

    {
        const auto g1UpBranch = injNetwork.uptree_branch("G1"s);

        BOOST_REQUIRE_MESSAGE(g1UpBranch.has_value(),
                              R"(There must be an upstream branch attached to injection node "G1")");

        // VFP table 9999 => no flow line.
        BOOST_REQUIRE_MESSAGE(! g1UpBranch->vfp_table().has_value(),
                              R"(There must NOT be a flow line (VFP table) attached to "G1"'s upstream branch.)");
    }

    {
        const auto m5nUpBranch = injNetwork.uptree_branch("M5N"s);

        BOOST_REQUIRE_MESSAGE(m5nUpBranch.has_value(),
                              R"(There must be an upstream branch attached to injection node "M5N")");

        BOOST_REQUIRE_MESSAGE(m5nUpBranch->vfp_table().has_value(),
                              R"(There must be a flow line (VFP table) attached to "M5N"'s upstream branch.)");

        BOOST_CHECK_EQUAL(m5nUpBranch->vfp_table().value(), 1729);
    }

    {
        const auto m5sUpBranch = injNetwork.uptree_branch("M5S"s);

        BOOST_REQUIRE_MESSAGE(m5sUpBranch.has_value(),
                              R"(There must be an upstream branch attached to injection node "M5S")");

        BOOST_REQUIRE_MESSAGE(m5sUpBranch->vfp_table().has_value(),
                              R"(There must be a flow line (VFP table) attached to "M5S"'s upstream branch.)");

        BOOST_CHECK_EQUAL(m5sUpBranch->vfp_table().value(), 1729);
    }
}

BOOST_AUTO_TEST_CASE(Reject_InvalidPhase_SOLVENT)
{
    const auto cse = twoD_5x1x2(R"(GNETINJE
 'PLAT-A' 'SOLVENT'  160.0  /
/)");

    BOOST_CHECK(! cse.sched.back().injectionNetwork.has(Phase::WATER));
}

BOOST_AUTO_TEST_CASE(Reject_InvalidPhase_Defaulted)
{
    const auto cse = twoD_5x1x2(R"(GNETINJE
 'PLAT-A'   1*   160.0  /
/)");

    BOOST_CHECK(! cse.sched.back().injectionNetwork.has(Phase::WATER));
}

BOOST_AUTO_TEST_CASE(Reject_InvalidPhase_Typo)
{
    const auto cse = twoD_5x1x2(R"(GNETINJE
 'PLAT-A' 'WATRR'  160.0  /
/)");

    BOOST_CHECK(! cse.sched.back().injectionNetwork.has(Phase::WATER));
}

BOOST_AUTO_TEST_CASE(Reject_NegativeVFPTable)
{
    BOOST_CHECK_THROW(twoD_5x1x2(R"(GNETINJE
 'M5S'  'WAT'  1*  -1  /
/)"), OpmInputError);
}

BOOST_AUTO_TEST_CASE(Reject_TerminalPressureWithPositiveVFP)
{
    BOOST_CHECK_THROW(twoD_5x1x2(R"(GNETINJE
 'PLAT-A'  'WAT'  160.0  3  /
/)"), OpmInputError);
}

BOOST_AUTO_TEST_CASE(Reject_MalformedRecord_WrongType)
{
    BOOST_CHECK_THROW(twoD_5x1x2(R"(GNETINJE
 'M5S'  'WAT'  'NOT-A-NUMBER'  3 /
/)"), std::exception);
}

BOOST_AUTO_TEST_CASE(Reject_MalformedRecord_MissingItem)
{
    BOOST_CHECK_THROW(twoD_5x1x2(R"(GNETINJE
 'M5S  'WAT'  1*  3 /
/)"), std::exception);
}

BOOST_AUTO_TEST_SUITE_END()     // Water

BOOST_AUTO_TEST_SUITE_END()     // Injection_Networks
