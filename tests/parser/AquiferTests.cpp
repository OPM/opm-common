/*
Copyright 2017 TNO.
Copyright 2020 Equinor.

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

#define BOOST_TEST_MODULE AquiferCTTest

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/Aquancon.hpp>
#include <opm/input/eclipse/EclipseState/Aquifer/AquiferCT.hpp>
#include <opm/input/eclipse/EclipseState/Aquifer/Aquifetp.hpp>
#include <opm/input/eclipse/EclipseState/Aquifer/AquiferFlux.hpp>
#include <opm/input/eclipse/EclipseState/Aquifer/AquiferConfig.hpp>
#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquiferRegionSplit.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/input/eclipse/Units/Units.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>

#include <opm/common/utility/OpmInputError.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <utility>
#include <memory>
#include <stdexcept>

using namespace Opm;

namespace {

EclipseGrid makeGrid()
{
    EclipseGrid grid(3, 3, 3);

    std::vector<int> actnum(27, 1);
    for (const std::size_t layer : { 0, 1, 2 })
        actnum[grid.getGlobalIndex(0, 0, layer)] = 0;

    grid.resetACTNUM(actnum);
    return grid;
}

Deck createAquiferCTDeck()
{
    return Parser{}.parseString(R"(DIMENS
3 3 3 /

AQUDIMS
1* 1* 2 100 1 1000 /

GRID
ACTNUM
 0 8*1 0 8*1 0 8*1 /

DXV
1 1 1 /

DYV
1 1 1 /

DZV
1 1 1 /

BOX
  1 3 1 3 1 1 /

TOPS
  9*100 /

ENDBOX

PORO
  27*0.15 /

PROPS
AQUTAB
 0.01 0.112
 0.05 0.229 /

SOLUTION
AQUCT
   1 2000.0 1.5 100 .3 3.0e-5 330 10 360.0 1 2 1* 20/
/
)");
}

Deck createAquiferCTDeckWithAqantrc()
{
    return Parser{}.parseString(R"(RUNSPEC
WATER
DIMENS
3 3 3 /

AQUDIMS
1* 1* 2 100 1 1000 /

TRACERS
 1* 1 /

GRID
ACTNUM
 0 8*1 0 8*1 0 8*1 /

DXV
1 1 1 /

DYV
1 1 1 /

DZV
1 1 1 /

BOX
  1 3 1 3 1 1 /

TOPS
  9*100 /

ENDBOX

PORO
  27*0.15 /

PROPS
TRACER
 WAQ WAT sm3 /
/
AQUTAB
 0.01 0.112
 0.05 0.229 /

SOLUTION
AQUCT
   1 2000.0 1.5 100 .3 3.0e-5 330 10 360.0 1 2 1* 20/
/
AQANTRC
 1 WAQ 1.0 /
/
)");
}

Deck createAquiferCTDeckWithUnknownAqantrc()
{
    return Parser{}.parseString(R"(RUNSPEC
WATER
DIMENS
3 3 3 /

AQUDIMS
1* 1* 2 100 1 1000 /

TRACERS
 1* 1 /

GRID
ACTNUM
 0 8*1 0 8*1 0 8*1 /

DXV
1 1 1 /

DYV
1 1 1 /

DZV
1 1 1 /

BOX
  1 3 1 3 1 1 /

TOPS
  9*100 /

ENDBOX

PORO
  27*0.15 /

PROPS
TRACER
 WAQ WAT sm3 /
/
AQUTAB
 0.01 0.112
 0.05 0.229 /

SOLUTION
AQUCT
   1 2000.0 1.5 100 .3 3.0e-5 330 10 360.0 1 2 1* 20/
/
AQANTRC
 99 WAQ 1.0 /
/
)");
}

Deck createAquiferCTDeckDefaultP0()
{
    return Parser{}.parseString(R"(DIMENS
3 3 3 /

AQUDIMS
1* 1* 2 100 1 1000 /

GRID
ACTNUM
 0 8*1 0 8*1 0 8*1 /

DXV
1 1 1 /

DYV
1 1 1 /

DZV
1 1 1 /

BOX
  1 3 1 3 1 1 /

TOPS
  9*100 /

ENDBOX

PORO
  27*0.15 /

PROPS
AQUTAB
 0.01 0.112
 0.05 0.229 /

SOLUTION
AQUCT
   1 2000.0 1* 100 .3 3.0e-5 330 10 360.0 1 2 /
/
)");
}

AquiferCT init_aquiferct(const Deck& deck)
{
    EclipseState eclState( deck );
    return AquiferCT(eclState.getTableManager(), deck);
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(AQANTRC_In_AquiferConfig)
{
    const auto deck = createAquiferCTDeckWithAqantrc();
    const EclipseState state(deck);
    const auto& tracers = state.aquifer().aquiferTracers();

    BOOST_REQUIRE_EQUAL(tracers.size(), 1U);
    BOOST_CHECK_EQUAL(tracers[0].aquifer_id, 1);
    BOOST_CHECK_EQUAL(tracers[0].tracer_name, "WAQ");
    BOOST_CHECK_CLOSE(tracers[0].concentration, 1.0, 1.0e-10);
}

BOOST_AUTO_TEST_CASE(AQANTRC_UnknownAquiferId)
{
    const auto deck = createAquiferCTDeckWithUnknownAqantrc();
    BOOST_CHECK_THROW(EclipseState{deck}, OpmInputError);
}

BOOST_AUTO_TEST_CASE(AquiferCTTest)
{
    {
        const auto aquiferct = init_aquiferct(createAquiferCTDeck());
        BOOST_REQUIRE_EQUAL(aquiferct.size(), 1U);

        for (const auto& it : aquiferct) {
            BOOST_CHECK_EQUAL(it.aquiferID, 1);
            BOOST_CHECK_CLOSE(it.porosity, 0.3, 1.0e-8);
            BOOST_CHECK_EQUAL(it.inftableID, 2);
            BOOST_CHECK_MESSAGE(it.initial_pressure.has_value(), "Initial pressure must be defined in CT aquifer");
            BOOST_CHECK_CLOSE(it.initial_pressure.value(), 1.5e5, 1e-6);
            BOOST_CHECK_CLOSE(it.initial_temperature.value(), 20 + 273.15, 1e-6);
        }
    }

    {
        const auto aquiferct = init_aquiferct(createAquiferCTDeckDefaultP0());
        for (const auto& it : aquiferct) {
            BOOST_CHECK_EQUAL(it.aquiferID, 1);
            BOOST_CHECK_CLOSE(it.porosity, 0.3, 1.0e-8);
            BOOST_CHECK_EQUAL(it.inftableID, 2);
            BOOST_CHECK_MESSAGE(! it.initial_pressure.has_value(), "Initial pressure must NOT be defined in CT aquifer");
            BOOST_CHECK_MESSAGE(! it.initial_temperature.has_value(), "Initial temperature is NOT supposed to be defined in these aquifers");
        }

        auto data = aquiferct.data();
        AquiferCT aq2(data);
        BOOST_CHECK( aq2 == aquiferct );
    }
}

namespace {

Deck createAQUANCONDeck_DEFAULT_INFLUX2()
{
    return Parser{}.parseString(R"(DIMENS
3 3 3 /

GRID
ACTNUM
 0 8*1 0 8*1 0 8*1 /

DXV
1 1 1 /

DYV
1 1 1 /

DZV
1 1 1 /

BOX
  1 3 1 3 1 1 /

TOPS
  9*100 /

ENDBOX

PORO
  27*0.15 /

SOLUTION
AQUANCON
   1      2  2  1    1   1  1  J-  1.0 /
   1      2  2  1    1   1  1  J-   /
/
)");
}

Deck createAQUANCONDeck_DEFAULT_INFLUX1()
{
    return Parser{}.parseString(R"(DIMENS
3 3 3 /

GRID
ACTNUM
 0 8*1 0 8*1 0 8*1 /

DXV
1 1 1 /

DYV
1 1 1 /

DZV
1 1 1 /

BOX
  1 3 1 3 1 1 /

TOPS
  9*100 /

ENDBOX

PORO
  27*0.15 /

SOLUTION
AQUANCON
   1      1  3  1    1   1  1  J-   /
/
AQUANCON
   2      1  1  2    2   1  1  J-   /
/
)");
}

Deck createAQUANCONDeck_DEFAULT_ILLEGAL()
{
    return Parser{}.parseString(R"(DIMENS
3 3 3 /

GRID
ACTNUM
 0 8*1 0 8*1 0 8*1 /

DXV
1 1 1 /

DYV
1 1 1 /

DZV
1 1 1 /

BOX
  1 3 1 3 1 1 /

TOPS
  9*100 /

ENDBOX

PORO
  27*0.15 /

SOLUTION
AQUANCON
   1      1  3  1    1   1  1  J-   /
/
AQUANCON
   2      1  2  1    2   1  1  J-   /
/
)");
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(AquanconTest_DEFAULT_INFLUX)
{
    auto deck1 = createAQUANCONDeck_DEFAULT_INFLUX1();
    const auto& grid = makeGrid();
    Aquancon aqcon(grid, deck1);

    const auto& cells_aq1 = aqcon.getConnections(1);
    /*
      The cells I = 0..2 are connected to aquifer 1; cell I==0 is inactive and
      not counted here ==> a total of 2 cells are connected to aquifer 1.
    */
    BOOST_CHECK_EQUAL(cells_aq1.size(), 2U);

    const auto& cells_aq2 = aqcon.getConnections(2);
    BOOST_CHECK_EQUAL(cells_aq2.size(), 1U);
    BOOST_CHECK(aqcon.active());

    auto deck2 = createAQUANCONDeck_DEFAULT_INFLUX2();

    // The cell (2,1,1) is attached to both aquifer 1 and aquifer 2 - that is illegal.
    auto deck3 = createAQUANCONDeck_DEFAULT_ILLEGAL();
    BOOST_CHECK_THROW(Aquancon( grid, deck3), std::invalid_argument);
}


// allowing aquifer exists inside the reservoir

namespace {

Deck createAQUANCONDeck_ALLOW_INSIDE_AQUAN_OR_NOT()
{
    return Parser{}.parseString(R"(DIMENS
3 3 3 /

GRID
ACTNUM
 0 8*1 0 8*1 0 8*1 /

DXV
1 1 1 /

DYV
1 1 1 /

DZV
1 1 1 /

BOX
  1 3 1 3 1 1 /

TOPS
  9*100 /

ENDBOX

PORO
  27*0.15 /

SOLUTION
AQUFETP
  1 20.0 1000.0 2000. 0.000001 200.0 /
  2 20.0 1000.0 2000. 0.000001 200.0 /
/
AQUANCON
   1      1  1   1   1   1  1  J-  2* YES /
   1      2  2   1   1   1  1  J-  2* YES /
   1      2  2   2   2   1  1  J-  2* YES /
   2      1  1   1   1   3  3  J-  2* NO /
   2      2  2   1   1   3  3  J-  2* NO /
   2      2  2   2   2   3  3  J-  2* NO /
/
)");
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(AquanconTest_ALLOW_AQUIFER_INSIDE_OR_NOT)
{
    auto deck = createAQUANCONDeck_ALLOW_INSIDE_AQUAN_OR_NOT();
    const EclipseState eclState( deck );
    const Aquancon aqucon( eclState.getInputGrid(), deck);

    const auto& data = aqucon.data();
    const Aquancon aq2(data);

    BOOST_CHECK(aqucon == aq2);
    const auto cells1 = aqucon.getConnections(1);
    const auto cells2 = aqucon.getConnections(2);
    BOOST_CHECK_EQUAL(cells1.size() , 2U);
    BOOST_CHECK_EQUAL(cells2.size() , 1U);
}

namespace {

Deck createAquifetpDeck()
{
    return Parser{}.parseString(R"(RUNSPEC
DIMENS
3 3 3 /

AQUDIMS
1* 1* 2 100 1 1000 /

GRID
ACTNUM
 0 8*1 0 8*1 0 8*1 /

DXV
1 1 1 /

DYV
1 1 1 /

DZV
1 1 1 /

BOX
  1 3 1 3 1 1 /

TOPS
  9*100 /

ENDBOX

PORO
  27*0.15 /

PROPS
AQUTAB
 0.01 0.112
 0.05 0.229 /

SOLUTION
AQUFETP
1  70000.0  4.0e3 2.0e9 1.0e-5	500 1 0	20 /
/
)");
}

Deck createNullAquifetpDeck()
{
    return Parser{}.parseString(R"(RUNSPEC
DIMENS
3 3 3 /

AQUDIMS
1* 1* 2 100 1 1000 /

GRID
ACTNUM
 0 8*1 0 8*1 0 8*1 /

DXV
1 1 1 /

DYV
1 1 1 /

DZV
1 1 1 /

BOX
  1 3 1 3 1 1 /

TOPS
  9*100 /

ENDBOX

PORO
  27*0.15 /

PROPS
AQUTAB
 0.01 0.112
 0.05 0.229 /

SOLUTION
)");
}

Deck createAquifetpDeck_defaultPressure()
{
    return Parser{}.parseString(R"(DIMENS
3 3 3 /

AQUDIMS
1* 1* 2 100 1 1000 /

GRID
ACTNUM
 0 8*1 0 8*1 0 8*1 /

DXV
1 1 1 /

DYV
1 1 1 /

DZV
1 1 1 /

BOX
  1 3 1 3 1 1 /

TOPS
  9*100 /

ENDBOX

PORO
  27*0.15 /

PROPS
AQUTAB
 0.01 0.112
 0.05 0.229 /

SOLUTION
AQUFETP
1  70000.0  1* 2.0e9 1.0e-5	500 1 0	1* /
/
)");
}

Aquifetp init_aquifetp(Deck& deck)
{
    return { TableManager{ deck }, deck };
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(AquifetpTest)
{
    auto aqufetp_deck = createAquifetpDeck();
    const auto& aquifetp = init_aquifetp(aqufetp_deck);
    for (const auto& it : aquifetp) {
        BOOST_CHECK_EQUAL(it.aquiferID, 1);
        BOOST_CHECK_CLOSE(it.initial_watvolume, 2.0e9, 1.0e-8);
        BOOST_CHECK_CLOSE(it.prod_index, 500/86400e5, 1.0e-8);
        BOOST_CHECK_MESSAGE(it.initial_pressure.has_value(), "Fetkovich aquifer must have initial pressure value");
        BOOST_CHECK_CLOSE(it.initial_temperature.value(), 20 + 273.15, 1.0e-8);
    }
    const auto& data = aquifetp.data();
    Aquifetp aq2(data);
    BOOST_CHECK_MESSAGE(aq2 == aquifetp, "Copy constructor must produce equal object");

    auto aqufetp_deck_null = createNullAquifetpDeck();
    const auto& aquifetp_null = init_aquifetp(aqufetp_deck_null);
    BOOST_CHECK_EQUAL(aquifetp_null.size(), 0U);

    auto aqufetp_deck_default = createAquifetpDeck_defaultPressure();
    const auto& aquifetp_default = init_aquifetp(aqufetp_deck_default);
    for (const auto& it : aquifetp_default) {
        BOOST_CHECK_EQUAL(it.aquiferID, 1);
        BOOST_CHECK_CLOSE(it.initial_watvolume, 2.0e9, 1.0e-8);
        BOOST_CHECK_CLOSE(it.prod_index, 500/86400e5, 1.0e-8);
        BOOST_CHECK_MESSAGE(!it.initial_pressure.has_value(), "Fetkovich aquifer mut NOT have initial pressure value when defaulted");
        BOOST_CHECK_MESSAGE(!it.initial_temperature.has_value(), "Initial temperature is NOT supposed to be defined in these aquifers");
    }
}

BOOST_AUTO_TEST_CASE(TEST_CREATE)
{
    Opm::Aqudims aqudims;

    BOOST_CHECK_EQUAL( aqudims.getNumAqunum() , 1U );
    BOOST_CHECK_EQUAL( aqudims.getNumConnectionNumericalAquifer() , 1U );
    BOOST_CHECK_EQUAL( aqudims.getNumInfluenceTablesCT() , 1U );
    BOOST_CHECK_EQUAL( aqudims.getNumRowsInfluenceTable() , 36U );
    BOOST_CHECK_EQUAL( aqudims.getNumAnalyticAquifers() , 1U );
    BOOST_CHECK_EQUAL( aqudims.getNumRowsAquancon() , 1U );
    BOOST_CHECK_EQUAL( aqudims.getNumAquiferLists() , 0U );
    BOOST_CHECK_EQUAL( aqudims.getNumAnalyticAquifersSingleList() , 0U );
}

BOOST_AUTO_TEST_CASE(Test_Aquifer_Config)
{
    const std::string deck_string = R"(
DIMENS
   3 3 3 /
GRID
DX
  27*1 /
DY
  27*1 /
DZ
  27*1 /
BOX
  1 3 1 3 1 1 /
TOPS
  9*1 /
ENDBOX
PORO
  27*1 /
)";
    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(deck_string);
    Opm::EclipseState ecl_state(deck);
    Opm::TableManager tables;
    Opm::AquiferConfig conf(tables, ecl_state.getInputGrid(), deck, ecl_state.fieldProps());
    BOOST_CHECK(!conf.active());


    const auto& fetp  = conf.fetp();
    const auto& ct    = conf.ct();
    const auto& aquflux = conf.aquflux();
    const auto& conn  = conf.connections();
    Opm::AquiferConfig conf2(fetp, ct, aquflux, conn);
    BOOST_CHECK( conf == conf2 );
}

BOOST_AUTO_TEST_CASE(Test_Aquifer_Config_Active)
{
    const auto deck = Parser{}.parseString(R"(
START             -- 0
10 MAY 2007 /
RUNSPEC

DIMENS
 10 10 10 /
REGDIMS
  3/
AQUDIMS
4 4 1* 1* 3 200 1* 1* /
GRID
DXV
   10*400 /
DYV
   10*400 /
DZV
   10*400 /
BOX
   1 10 1 10 1 1 /
TOPS
   100*2202 /
ENDBOX
PERMX
  1000*0.25 /
COPY
  PERMX PERMY /
  PERMX PERMZ /
/
PORO
   1000*0.15 /
AQUNUM
  4       1 1 1      15000  5000  0.3  30  2700  / aq cell
  5       2 1 1     150000  9000  0.3  30  2700  / aq cell
  6       3 1 1     150000  9000  0.3  30  2700  / aq cell
  7       4 1 1     150000  9000  0.3  30  2700  / aq cell
/
AQUCON
-- #    I1 I2  J1 J2   K1  K2    Face
   4    1  1   16 18   19  20   'I-'    / connecting cells
   5    2  2   16 18   19  20   'I-'    / connecting cells
   6    3  3   16 18   19  20   'I-'    / connecting cells
   7    4  4   16 18   19  20   'I-'    / connecting cells
/
REGIONS
FIPNUM
200*1 300*2 500*3 /
FIPREG
200*10 300*20 500*30 /
SOLUTION
AQUCT
1    2040     1*    1000   .3    3.0e-5     1330     10     360.0   1   1* /
2    2040     1*    1000   .3    3.0e-5     1330     10     360.0   1   1* /
3    2040     1*    1000   .3    3.0e-5     1330     10     360.0   1   1* /
/
AQUANCON
1     1   10     10    2    10  10   'I-'      0.88      1  /
2     9   10     10    10    10  10   'I+'      0.88      1  /
3     9   9      8    10    9   8   'I+'      0.88      1  /
/

END
)");

    const auto es = EclipseState { deck };

    const auto& aquConfig = es.aquifer();

    BOOST_CHECK_MESSAGE(aquConfig.hasAnalyticalAquifer(),
                        "Aquifer configuration object must have analytic aquifers");

    BOOST_CHECK_MESSAGE(aquConfig.hasNumericalAquifer(),
                        "Aquifer configuration object must have numeric aquifers");

    BOOST_CHECK_MESSAGE(  aquConfig.hasAquifer(1), "Configuration object must have Aquifer ID 1");
    BOOST_CHECK_MESSAGE(  aquConfig.hasAquifer(2), "Configuration object must have Aquifer ID 2");
    BOOST_CHECK_MESSAGE(  aquConfig.hasAquifer(3), "Configuration object must have Aquifer ID 3");
    BOOST_CHECK_MESSAGE(  aquConfig.hasAquifer(4), "Configuration object must have Aquifer ID 4");
    BOOST_CHECK_MESSAGE(  aquConfig.hasAquifer(5), "Configuration object must have Aquifer ID 5");
    BOOST_CHECK_MESSAGE(  aquConfig.hasAquifer(6), "Configuration object must have Aquifer ID 6");
    BOOST_CHECK_MESSAGE(  aquConfig.hasAquifer(7), "Configuration object must have Aquifer ID 7");
    BOOST_CHECK_MESSAGE(! aquConfig.hasAquifer(8), "Configuration object must NOT have Aquifer ID 8");

    BOOST_CHECK_MESSAGE(  aquConfig.hasAnalyticalAquifer(1), "Configuration object must have Analytical Aquifer ID 1");
    BOOST_CHECK_MESSAGE(  aquConfig.hasAnalyticalAquifer(2), "Configuration object must have Analytical Aquifer ID 2");
    BOOST_CHECK_MESSAGE(  aquConfig.hasAnalyticalAquifer(3), "Configuration object must have Analytical Aquifer ID 3");
    BOOST_CHECK_MESSAGE(  !aquConfig.hasAnalyticalAquifer(4), "Configuration object must NOT have Analytical Aquifer ID 4");
    BOOST_CHECK_MESSAGE(  !aquConfig.hasAnalyticalAquifer(5), "Configuration object must NOT have Analytical Aquifer ID 5");
    BOOST_CHECK_MESSAGE(  !aquConfig.hasAnalyticalAquifer(6), "Configuration object must NOT have Analytical Aquifer ID 6");
    BOOST_CHECK_MESSAGE(  !aquConfig.hasAnalyticalAquifer(7), "Configuration object must NOT have Analytical Aquifer ID 7");

    {
        const auto expect = std::vector<int>{ 1, 2, 3 };
        const auto analytic = analyticAquiferIDs(aquConfig);

        BOOST_CHECK_EQUAL_COLLECTIONS(analytic.begin(), analytic.end(),
                                      expect  .begin(), expect  .end());
    }

    {
        const auto expect = std::vector<int>{ 4, 5, 6, 7 };
        const auto numeric = numericAquiferIDs(aquConfig);

        BOOST_CHECK_EQUAL_COLLECTIONS(numeric.begin(), numeric.end(),
                                      expect .begin(), expect .end());
    }
}

namespace {

Deck createNumericalAquiferDeck()
{
    const char *deckData = R"(
DIMENS
 8 15 3 /
AQUDIMS
    3      2      1*       1*     1*       50      1*      1*  /
GRID

DX
  360*10./
DY
   360*10./
DZ
   360*1./
BOX
   1 8 1 15 1 1 /
TOPS
   120*100./
ENDBOX

PORO
   0. 0.25 0. 357*0.25/
PERMX
    360*1000./
PERMY
    360*1000./
PERMZ
    360*10./

BOX
1	8 15 15 3 3 /

MULTY
 1e9 1e-9 1.0 2.0 3.0 4.0 5.0 6.0/

ENDBOX

-- setting the three cells for numerical aquifer to be inactive
ACTNUM
0 1 0 0 356*1 /

AQUNUM
--AQnr.  I  J  K     Area      Length PHI      K     Depth  Initial.Pr	PVTNUM   SATNUM
   1     1  1  1   1000000.0   10000   0.25   400    2585.00   285.00	 2   2  /
   1     3  1  1   1500000.0   20000   0.24   600    2585.00   285.00	 3   *  /
   1     4  1  1   2000000.0   30000   *   700    2585.00   285.00	 *   3  /
/
AQUCON
--  Connect numerical aquifer to the reservoir
--  Id.nr  I1	I2     J1  J2	 K1  K2    Face    Trans.mult.  Trans.opt.
     1     1	8      15    15	  3   3   'J+'      1.00      1  /
/
    )";

    Parser parser;
    return parser.parseString(deckData);
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(NumericalAquiferTest)
{
    const Opm::Deck numaquifer_deck = createNumericalAquiferDeck();
    const Opm::EclipseState ecl_state(numaquifer_deck);
    const Opm::EclipseGrid& grid = ecl_state.getInputGrid();

    Opm::NumericalAquifers num_aqu{numaquifer_deck, grid, ecl_state.fieldProps()};

    BOOST_CHECK_EQUAL(num_aqu.numRecords(), 3);
    {
        const auto mD = unit::convert::from(1.0, prefix::milli*unit::darcy);

        const auto* c1 = num_aqu.getAquifer(1).getCellPrt(0);
        const auto* c2 = num_aqu.getAquifer(1).getCellPrt(1);
        const auto* c3 = num_aqu.getAquifer(1).getCellPrt(2);

        BOOST_CHECK_EQUAL(c1->record_id, std::size_t{0});
        BOOST_CHECK_EQUAL(c1->I, std::size_t{0});
        BOOST_CHECK_EQUAL(c1->J, std::size_t{0});
        BOOST_CHECK_EQUAL(c1->K, std::size_t{0});

        BOOST_CHECK_CLOSE(c1->area, 1.0e6, 1.0e-10);
        BOOST_CHECK_CLOSE(c1->length, 10.0e3, 1.0e-10);
        BOOST_CHECK_CLOSE(c1->porosity, 0.25, 1.0e-10);
        BOOST_CHECK_CLOSE(c1->permeability, 400*mD, 1.0e-10);
        BOOST_CHECK_CLOSE(c1->depth, 2585.0, 1.0e-10);
        BOOST_CHECK_MESSAGE(c1->init_pressure.has_value(), "Cell 1 must have an initial pressure");
        BOOST_CHECK_CLOSE(c1->init_pressure.value(), 285.0*unit::barsa, 1.0e-10);
        BOOST_CHECK_EQUAL(c1->pvttable, 2);
        BOOST_CHECK_EQUAL(c1->sattable, 2);
        BOOST_CHECK_EQUAL(c1->global_index, 0);

        BOOST_CHECK_EQUAL(c2->record_id, std::size_t{1});
        BOOST_CHECK_EQUAL(c2->I, std::size_t{2});
        BOOST_CHECK_EQUAL(c2->J, std::size_t{0});
        BOOST_CHECK_EQUAL(c2->K, std::size_t{0});

        BOOST_CHECK_CLOSE(c2->area, 1.5e6, 1.0e-10);
        BOOST_CHECK_CLOSE(c2->length, 20.0e3, 1.0e-10);
        BOOST_CHECK_CLOSE(c2->porosity, 0.24, 1.0e-10);
        BOOST_CHECK_CLOSE(c2->permeability, 600*mD, 1.0e-10);
        BOOST_CHECK_CLOSE(c2->depth, 2585.0, 1.0e-10);
        BOOST_CHECK_MESSAGE(c2->init_pressure.has_value(), "Cell 2 must have an initial pressure");
        BOOST_CHECK_CLOSE(c2->init_pressure.value(), 285.0*unit::barsa, 1.0e-10);
        BOOST_CHECK_EQUAL(c2->pvttable, 3);
        BOOST_CHECK_EQUAL(c2->sattable, 1);
        BOOST_CHECK_EQUAL(c2->global_index, 2);

        BOOST_CHECK_EQUAL(c3->record_id, std::size_t{2});
        BOOST_CHECK_EQUAL(c3->I, std::size_t{3});
        BOOST_CHECK_EQUAL(c3->J, std::size_t{0});
        BOOST_CHECK_EQUAL(c3->K, std::size_t{0});

        BOOST_CHECK_CLOSE(c3->area, 2.0e6, 1.0e-10);
        BOOST_CHECK_CLOSE(c3->length, 30.0e3, 1.0e-10);
        BOOST_CHECK_CLOSE(c3->porosity, 0.25, 1.0e-10);
        BOOST_CHECK_CLOSE(c3->permeability, 700*mD, 1.0e-10);
        BOOST_CHECK_CLOSE(c3->depth, 2585.0, 1.0e-10);
        BOOST_CHECK_MESSAGE(c3->init_pressure.has_value(), "Cell 3 must have an initial pressure");
        BOOST_CHECK_CLOSE(c3->init_pressure.value(), 285.0*unit::barsa, 1.0e-10);
        BOOST_CHECK_EQUAL(c3->pvttable, 1);
        BOOST_CHECK_EQUAL(c3->sattable, 3);
        BOOST_CHECK_EQUAL(c3->global_index, 3);
    }

    {
        const auto numAquCells = num_aqu.allAquiferCellIds();
        const auto expect = std::vector { 0, 2, 3, };

        BOOST_CHECK_EQUAL_COLLECTIONS(numAquCells.begin(), numAquCells.end(),
                                      expect     .begin(), expect     .end());
    }

    // using processed actnum for numerical aquifer connection generation
    std::vector<int> new_actnum(360, 1);
    new_actnum[0] = 0;
    new_actnum[1] = 0;
    new_actnum[3] = 0;
    num_aqu.postProcessConnections(grid, new_actnum);
    BOOST_CHECK(num_aqu.hasAquifer(1));
    BOOST_CHECK(num_aqu.size() == 1);
    const auto all_aquifer_cells = num_aqu.allAquiferCells();
    BOOST_CHECK(all_aquifer_cells.count(0) > 0);
    BOOST_CHECK(all_aquifer_cells.count(2) > 0);
    BOOST_CHECK(all_aquifer_cells.count(3) > 0);
    BOOST_CHECK(all_aquifer_cells.count(1) == 0);

    const auto& aquifer = num_aqu.getAquifer(1);
    BOOST_CHECK(aquifer.numCells() == 3);
    BOOST_CHECK(aquifer.numConnections() == 8 );

    const auto& nncs = aquifer.aquiferConnectionNNCs(grid, ecl_state.fieldProps());
    BOOST_CHECK(nncs.size() == 8 );
    // get the half transmissibilites by using small/large multipler m
    // mab/(ma+b) -> b for ma >> b and -> ma for ma << b
    const double taq = nncs[0].trans;
    const double tcell = nncs[1].trans*1.0e9;
    // now check the multiplier for the rest of the connection i > 2 where m = 1->6
    for (int i = 2; i < 8; ++i) {
      const double mult = (i - 1);
      const double t = mult*tcell*taq / (taq + mult*tcell);
      BOOST_CHECK_CLOSE(t, nncs[i].trans, 1.0e-6);
    }
    BOOST_CHECK(grid.getNumActive() == 360);
    // the three aquifer cells are active
    BOOST_CHECK(grid.cellActive(0, 0, 0));
    BOOST_CHECK(grid.cellActive(2, 0, 0));
    BOOST_CHECK(grid.cellActive(3, 0, 0));

    // checking the pore volume of the aquifer cells
    const auto& porv_data = ecl_state.fieldProps().porv(true);
    BOOST_CHECK_CLOSE(porv_data[0], 2500000000, 1.e-10);
    BOOST_CHECK_CLOSE(porv_data[2], 7200000000, 1.e-10);
    BOOST_CHECK_CLOSE(porv_data[3], 15000000000, 1.e-10);

    const auto& pvtnum = ecl_state.fieldProps().get_int("PVTNUM");
    BOOST_CHECK_EQUAL(pvtnum[0], 2);
    BOOST_CHECK_EQUAL(pvtnum[1], 1); // none aquifer cell
    BOOST_CHECK_EQUAL(pvtnum[2], 3);
    BOOST_CHECK_EQUAL(pvtnum[3], 1);

    const auto& satnum = ecl_state.fieldProps().get_int("SATNUM");
    BOOST_CHECK_EQUAL(satnum[0], 2);
    BOOST_CHECK_EQUAL(satnum[1], 1); // none aquifer cell
    BOOST_CHECK_EQUAL(satnum[2], 1);
    BOOST_CHECK_EQUAL(satnum[3], 3);


    const auto& permx = ecl_state.fieldProps().get_double("PERMX");
    BOOST_CHECK_SMALL(permx[0], 1.e-20);
    BOOST_CHECK_SMALL(permx[2], 1.e-20);
    BOOST_CHECK_SMALL(permx[3], 1.e-20);
    const auto& permy = ecl_state.fieldProps().get_double("PERMY");
    BOOST_CHECK_SMALL(permy[0], 1.e-20);
    BOOST_CHECK_SMALL(permy[2], 1.e-20);
    BOOST_CHECK_SMALL(permy[3], 1.e-20);
    const auto& permz = ecl_state.fieldProps().get_double("PERMZ");
    BOOST_CHECK_SMALL(permz[0], 1.e-20);
    BOOST_CHECK_SMALL(permz[2], 1.e-20);
    BOOST_CHECK_SMALL(permz[3], 1.e-20);
    const auto& poro = ecl_state.fieldProps().get_double("PORO");
    BOOST_CHECK_CLOSE(poro[0], 0.25, 1.e-10);
    BOOST_CHECK_CLOSE(poro[2], 0.24, 1.e-10);
    BOOST_CHECK_CLOSE(poro[3], 0.25, 1.e-10);
}

namespace {

std::pair<Opm::Aquancon, Opm::EclipseGrid> load_aquifer(const std::string& aqucon)
{
    const std::string data1 = R"(
DIMENS
 8 15 3 /
AQUDIMS
    3      2      1*       1*     1*       50      1*      1*  /
GRID

DX
  360*10./
DY
   360*10./
DZ
   360*1./
BOX
   1 8 1 15 1 1 /
TOPS
   120*100./
ENDBOX

PORO
   0. 0.25 0. 357*0.25/
PERMX
    360*1000./
PERMY
    360*1000./
PERMZ
    360*10./
-- setting the three cells for numerical aquifer to be inactive
ACTNUM
0 1 0 0 356*1 /

AQUNUM
--AQnr.  I  J  K     Area      Length PHI      K     Depth  Initial.Pr	PVTNUM   SATNUM
   1     1  1  1   1000000.0   10000   0.25   400    2585.00   285.00	 2   2  /
   1     3  1  1   1500000.0   20000   0.24   600    2585.00   285.00	 3   *  /
   1     4  1  1   2000000.0   30000   *   700    2585.00   285.00	 *   3  /
/
)";
    Opm::Parser parser;
    auto deck = parser.parseString( data1 + aqucon );
    auto grid = Opm::EclipseGrid( deck );

    return { Opm::Aquancon(grid, deck), grid };
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(AQUCONN_FUNKYNESS )
{
   {
       const std::string aq = R"(
AQUANCON
     1     1	8      15    15	  3   3   'J+'      1.00      2  /
/
)";
       const auto& [aquconn, _] = load_aquifer(aq);
       (void)_;
       auto cell1 = aquconn.getConnections(1)[0];
       BOOST_CHECK_EQUAL(cell1.influx_coeff, 2.0);
   }


   {
       const std::string aq = R"(
AQUANCON
     1     1	8      15    15	  3   3   'J+'      *      2  /
/
)";
       const auto& [aquconn, grid] = load_aquifer(aq);
       const auto& dims = grid.getCellDims(0,14,2);
       auto cell1 = aquconn.getConnections(1)[0];
       BOOST_CHECK_EQUAL(cell1.influx_coeff, 2.0 * dims[0]*dims[2]);
   }

   {
       const std::string aq = R"(
AQUANCON
     1     1	8      15    15	  3   3   'I+'      *      3  /
/
)";
       const auto& [aquconn, grid] = load_aquifer(aq);
       const auto& dims = grid.getCellDims(0,14,2);
       auto cell1 = aquconn.getConnections(1)[0];
       BOOST_CHECK_EQUAL(cell1.influx_coeff, 3.0 * dims[1]*dims[2]);
   }

   {
       const std::string aq = R"(
AQUANCON
     1     1	8      15    15	  3   3   'I+'      *        3  /
     1     1	8      15    15	  3   3   'I+'      100      4  /
/
)";
       const auto& [aquconn, grid] = load_aquifer(aq);
       const auto& dims = grid.getCellDims(0,14,2);
       auto cell1 = aquconn.getConnections(1)[0];
       BOOST_CHECK_EQUAL(cell1.influx_coeff, 4 * ( 100 + 3.0 * dims[1]*dims[2]));
   }

   {
       const std::string aq = R"(
AQUANCON
     1     1	8      15    15	  3   3   'I+'      100      4  /
     1     1	8      15    15	  3   3   'I+'      *        3  /
     1     1	8      15    15	  3   3   'I+'      77       2  /
/
)";
       const auto& [aquconn, grid] = load_aquifer(aq);
       auto cell1 = aquconn.getConnections(1)[0];
       BOOST_CHECK_EQUAL(cell1.influx_coeff, 2*(77 + 3*(0 + 4*100)));
   }
}

namespace {

// Aquifer 1 connects cells of EQLNUM regions 1 (K=1-2) and 2 (K=4-5).
// Aquifer 3 has its cell in region 2 but connects to region 1 cells only.
// Row J=1 and layer K=3 are inactive.
std::string regionSplitDeck(const std::string& aqunum,
                            const std::string& aqucon,
                            const std::string& regionEdits,
                            const std::string& gridEdits = "")
{
    return std::string { R"(
RUNSPEC
DIMENS
 5 4 5 /
TABDIMS
 1 2 /
EQLDIMS
 2 /
AQUDIMS
 1* 1* 1* 1* 10 50 /
METRIC
GRID
DX
 100*100 /
DY
 100*100 /
DZ
 100*10 /
BOX
 1 5 1 4 1 1 /
TOPS
 20*2000 /
ENDBOX
PORO
 100*0.25 /
PERMX
 100*100 /
PERMY
 100*100 /
PERMZ
 100*10 /
ACTNUM
 5*0 15*1
 5*0 15*1
 20*0
 5*0 15*1
 5*0 15*1 /
)" } + gridEdits + aqunum + aqucon + R"(
REGIONS
EQLNUM
 40*1 60*2 /
PVTNUM
 40*1 60*2 /
)" + regionEdits;
}

const std::string regionSplitAqunum = R"(
AQUNUM
 1 1 1 5 1.0e6 1000 0.25 500 1* 1* 2 1 /
 3 5 1 5 2.0e6 2000 0.20 400 1* 250 /
/
)";

const std::string regionSplitAqucon1 = R"(
AQUCON
 1 1 5 2 2 1 5 'J-' /
/
)";

const std::string regionSplitAqucon3 = R"(
AQUCON
 3 1 5 4 4 1 2 'J+' /
/
)";

struct RegionSplitCase
{
    explicit RegionSplitCase(const std::string& deckString)
        : deck { Opm::Parser{}.parseString(deckString) }
        , state { deck }
        , aquifers { deck, state.getInputGrid(), state.fieldProps() }
    {
        aquifers.postProcessConnections(state.getInputGrid(), state.getInputGrid().getACTNUM());
    }

    int eqlnum(std::size_t globalIndex) const
    {
        const auto& grid = state.getInputGrid();
        return state.fieldProps().get_int("EQLNUM")[grid.activeIndex(globalIndex)];
    }

    Opm::Deck deck;
    Opm::EclipseState state;
    Opm::NumericalAquifers aquifers;
};

std::string keywordBlock(const std::string& text, const std::string& keyword)
{
    const auto start = text.find("\n" + keyword + "\n");
    BOOST_REQUIRE(start != std::string::npos);
    const auto end = text.find("\n/\n", start + 1);
    BOOST_REQUIRE(end != std::string::npos);
    return text.substr(start, end + 3 - start);
}

// Deck records with the aquifer ID of each record.
using AquiferRecords = std::vector<std::pair<std::size_t, std::string>>;

struct RegionSplitInput
{
    AquiferRecords aqunum{};
    AquiferRecords aqucon{};
    std::string regionEdits{};
    std::string gridEdits{};
};

// Records of a keyword in the suggested deck changes, without the keyword
// line and the terminating slash.
std::string suggestedRecords(const std::string& changes, const std::string& keyword)
{
    if (changes.find("\n" + keyword + "\n") == std::string::npos) {
        return "";
    }
    const auto block = keywordBlock(changes, keyword);
    const auto first = block.find('\n', 1) + 1;
    return block.substr(first, block.size() - 2 - first);
}

// Deck from the input, with the suggested deck changes applied if given:
// the AQUNUM/AQUCON records of the replaced aquifers are removed and the
// suggested records and EQUALS appended.
std::string regionSplitDeck(const RegionSplitInput& input,
                            const Opm::NumericalAquiferRegionSplit* split = nullptr,
                            const std::string& changes = "")
{
    auto keyword = [](const std::string& name, const AquiferRecords& records,
                      const std::vector<std::size_t>& replaced, const std::string& suggested)
    {
        auto out = "\n" + name + "\n";
        for (const auto& [id, record] : records) {
            if (std::ranges::find(replaced, id) == replaced.end()) {
                out += record + "\n";
            }
        }
        return out + suggested + "/\n";
    };

    const std::vector<std::size_t> none{};
    const auto equals = suggestedRecords(changes, "EQUALS");
    return regionSplitDeck(
        keyword("AQUNUM", input.aqunum, split ? split->replacedAqunumIds() : none,
                suggestedRecords(changes, "AQUNUM")),
        keyword("AQUCON", input.aqucon, split ? split->replacedAquconIds() : none,
                suggestedRecords(changes, "AQUCON")),
        input.regionEdits + (equals.empty() ? "" : "EQUALS\n" + equals + "/\n"),
        input.gridEdits);
}

struct AppliedRegionSplit
{
    std::unique_ptr<RegionSplitCase> original;
    std::unique_ptr<Opm::NumericalAquiferRegionSplit> split;
    std::unique_ptr<RegionSplitCase> fixed;
};

// Applies the suggested deck changes for the given aquifers, and checks
// that the resulting deck is accepted, that every aquifer is then in a
// single EQLNUM region, and that each cell keeps its number of aquifer
// connections.
AppliedRegionSplit applyRegionSplit(const RegionSplitInput& input,
                                    const std::vector<std::size_t>& aquiferIds)
{
    AppliedRegionSplit result;
    result.original = std::make_unique<RegionSplitCase>(regionSplitDeck(input));
    const auto& original = *result.original;

    result.split = std::make_unique<Opm::NumericalAquiferRegionSplit>(
        original.aquifers, aquiferIds, original.state.getInputGrid(),
        original.state.fieldProps(), original.state.getLgrs());
    const auto changes = result.split->deckChanges(original.state.getDeckUnitSystem());
    BOOST_TEST_MESSAGE(changes);

    BOOST_REQUIRE_NO_THROW(result.fixed = std::make_unique<RegionSplitCase>(
                               regionSplitDeck(input, result.split.get(), changes)));
    const auto& fixed = *result.fixed;

    auto connectionCounts = [](const RegionSplitCase& rsc) {
        std::map<std::size_t, std::size_t> counts;
        for (const auto& [id, aquifer] : rsc.aquifers.aquifers()) {
            for (const auto& con : aquifer.connections()) {
                ++counts[con.global_index];
            }
        }
        return counts;
    };

    for (const auto& [id, aquifer] : fixed.aquifers.aquifers()) {
        std::set<int> regions;
        for (std::size_t c = 0; c < aquifer.numCells(); ++c) {
            regions.insert(fixed.eqlnum(aquifer.getCellPrt(c)->global_index));
        }
        for (const auto& con : aquifer.connections()) {
            regions.insert(fixed.eqlnum(con.global_index));
        }
        BOOST_CHECK_MESSAGE(regions.size() == 1, "Aquifer " << id << " spans several EQLNUM regions");
    }

    const auto before = connectionCounts(original);
    const auto after = connectionCounts(fixed);
    BOOST_CHECK_MESSAGE(before == after, "Applied deck changes alter the aquifer connections");

    return result;
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(NumericalAquiferRegionSplitRecords)
{
    const RegionSplitCase rsc { regionSplitDeck(regionSplitAqunum,
                                                regionSplitAqucon1 + regionSplitAqucon3, "") };
    const auto split = Opm::NumericalAquiferRegionSplit {
        rsc.aquifers, {1, 3}, rsc.state.getInputGrid(), rsc.state.fieldProps(), rsc.state.getLgrs()
    };

    BOOST_CHECK(!split.empty());
    BOOST_CHECK(split.unresolvedIds().empty());

    // Aquifer 3 is not split, but gets the PVT table of its connected cells
    // and its initial pressure defaulted.  New aquifer 4 takes the region 1
    // connections of aquifer 1, in the first free inactive cell which is not
    // next to a connected cell.
    const auto expectAqunum = std::vector<std::size_t>{3};
    const auto expectAqucon = std::vector<std::size_t>{1};
    BOOST_CHECK_EQUAL_COLLECTIONS(split.replacedAqunumIds().begin(), split.replacedAqunumIds().end(),
                                  expectAqunum.begin(), expectAqunum.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(split.replacedAquconIds().begin(), split.replacedAquconIds().end(),
                                  expectAqucon.begin(), expectAqucon.end());

    auto aqunumOf = [&split](std::size_t id) {
        std::vector<Opm::NumericalAquiferRegionSplit::AqunumRecord> recs;
        std::ranges::copy_if(split.aqunumRecords(), std::back_inserter(recs),
                             [id](const auto& rec) { return rec.id == id; });
        return recs;
    };

    BOOST_CHECK_EQUAL(split.aqunumRecords().size(), std::size_t{2});
    const auto aqu3 = aqunumOf(3);
    BOOST_REQUIRE_EQUAL(aqu3.size(), std::size_t{1});
    BOOST_CHECK_EQUAL(aqu3[0].pvtTable, 1);
    BOOST_CHECK_EQUAL(aqu3[0].region, 1);
    BOOST_CHECK(aqu3[0].hadInitialPressure);
    BOOST_CHECK(aqu3[0].ijk == (std::array<std::size_t, 3>{4, 0, 4}));

    const auto aqu4 = aqunumOf(4);
    BOOST_REQUIRE_EQUAL(aqu4.size(), std::size_t{1});
    BOOST_CHECK_EQUAL(aqu4[0].sourceId, std::size_t{1});
    BOOST_CHECK_EQUAL(aqu4[0].region, 1);
    BOOST_CHECK_EQUAL(aqu4[0].pvtTable, 1);
    BOOST_CHECK_EQUAL(aqu4[0].satTable, 1);
    BOOST_CHECK(aqu4[0].ijk == (std::array<std::size_t, 3>{0, 0, 2}));
    BOOST_CHECK_CLOSE(aqu4[0].area, 1.0e6, 1.0e-10);
    BOOST_CHECK_CLOSE(aqu4[0].depth, rsc.aquifers.getAquifer(1).getCellPrt(0)->depth, 1.0e-10);

    // Aquifer 1 keeps the connections to the region of its own cell.  Cell
    // (5,2,5) is not connected (it adjoins the cell of aquifer 3), so its
    // layer needs a separate box.
    using Box = std::array<std::size_t, 6>;
    std::map<std::size_t, std::vector<Box>> boxes;
    for (const auto& rec : split.aquconRecords()) {
        BOOST_CHECK(rec.face == Opm::FaceDir::YMinus);
        boxes[rec.id].push_back(rec.box);
    }
    BOOST_REQUIRE_EQUAL(boxes.size(), std::size_t{2});
    BOOST_CHECK(boxes[4] == (std::vector<Box>{{0, 4, 1, 1, 0, 1}}));
    BOOST_CHECK(boxes[1] == (std::vector<Box>{{0, 4, 1, 1, 3, 3}, {0, 3, 1, 1, 4, 4}}));

    std::map<std::size_t, int> eqlnum;
    for (const auto& rec : split.eqlnumRecords()) {
        eqlnum[rec.aquiferId] = rec.region;
    }
    BOOST_CHECK((eqlnum == std::map<std::size_t, int>{{3, 1}, {4, 1}}));
}

BOOST_AUTO_TEST_CASE(NumericalAquiferRegionSplitAppliedToDeck)
{
    const auto input = RegionSplitInput {
        { {1, " 1 1 1 5 1.0e6 1000 0.25 500 1* 1* 2 1 /"},
          {3, " 3 5 1 5 2.0e6 2000 0.20 400 1* 250 /"} },
        { {1, " 1 1 5 2 2 1 5 'J-' /"},
          {3, " 3 1 5 4 4 1 2 'J+' /"} },
    };
    const auto applied = applyRegionSplit(input, {1, 3});
    const auto& original = *applied.original;
    const auto& fixed = *applied.fixed;

    BOOST_CHECK_EQUAL(fixed.aquifers.size(), std::size_t{3});

    // The split aquifer is a copy of the original one.
    const auto* orig = original.aquifers.getAquifer(1).getCellPrt(0);
    const auto* copy = fixed.aquifers.getAquifer(4).getCellPrt(0);
    BOOST_CHECK_CLOSE(copy->area, orig->area, 1.0e-8);
    BOOST_CHECK_CLOSE(copy->length, orig->length, 1.0e-8);
    BOOST_CHECK_CLOSE(copy->porosity, orig->porosity, 1.0e-8);
    BOOST_CHECK_CLOSE(copy->permeability, orig->permeability, 1.0e-8);
    BOOST_CHECK_CLOSE(copy->depth, orig->depth, 1.0e-8);
    BOOST_CHECK(!fixed.aquifers.getAquifer(3).getCellPrt(0)->init_pressure.has_value());
}

BOOST_AUTO_TEST_CASE(NumericalAquiferRegionSplitTransferredConnectionNextToSourceAquifer)
{
    // The cell of aquifer 1, (2,1,1), is in EQLNUM region 2 and adjoins
    // connected cell (2,2,1) of region 1, which is retained without
    // ALLOW_INTERNAL_CELLS since it adjoins the aquifer's own cell.  The
    // region 1 connections move to a new aquifer, for which (2,1,1) is an
    // active cell of another aquifer.
    const auto input = RegionSplitInput {
        { {1, " 1 2 1 1 1.0e6 1000 0.25 500 /"} },
        { {1, " 1 1 5 2 2 1 5 'J-' /"} },
        "EQUALS\n 'EQLNUM' 2 2 2 1 1 1 1 /\n/\n",
    };
    const auto applied = applyRegionSplit(input, {1});

    const auto& grid = applied.original->state.getInputGrid();
    const auto adjoining = grid.getGlobalIndex(1, 1, 0);
    const auto& originalCons = applied.original->aquifers.getAquifer(1).connections();
    BOOST_REQUIRE(std::ranges::any_of(originalCons, [adjoining](const auto& con)
                                      { return con.global_index == adjoining; }));

    // Only the record of the adjoining cell allows internal cells.
    std::size_t numInternal = 0;
    for (const auto& rec : applied.split->aquconRecords()) {
        if (rec.allowInternalCells) {
            ++numInternal;
            BOOST_CHECK_EQUAL(rec.id, std::size_t{2});
            BOOST_CHECK(rec.box == (std::array<std::size_t, 6>{1, 1, 1, 1, 0, 0}));
        }
    }
    BOOST_CHECK_EQUAL(numInternal, std::size_t{1});

    const auto& fixedCons = applied.fixed->aquifers.getAquifer(2).connections();
    BOOST_CHECK(std::ranges::any_of(fixedCons, [adjoining](const auto& con)
                                    { return con.global_index == adjoining; }));
}

BOOST_AUTO_TEST_CASE(NumericalAquiferRegionSplitAvoidsRefinements)
{
    // Column I=1 is not connected.  The refinement box (1,1-2,1) contains
    // the cell which would otherwise host the new aquifer.
    auto input = RegionSplitInput {
        { {1, " 1 1 1 5 1.0e6 1000 0.25 500 1* 1* 2 1 /"} },
        { {1, " 1 2 5 2 2 1 5 'J-' /"} },
    };

    auto newAquiferCells = [](const Opm::NumericalAquiferRegionSplit& split) {
        std::vector<std::array<std::size_t, 3>> cells;
        for (const auto& rec : split.aqunumRecords()) {
            if (rec.id != rec.sourceId) {
                cells.push_back(rec.ijk);
            }
        }
        return cells;
    };

    {
        const RegionSplitCase rsc { regionSplitDeck(input) };
        const auto split = Opm::NumericalAquiferRegionSplit {
            rsc.aquifers, {1}, rsc.state.getInputGrid(), rsc.state.fieldProps(), rsc.state.getLgrs()
        };
        BOOST_CHECK(newAquiferCells(split) == (std::vector<std::array<std::size_t, 3>>{{0, 0, 0}}));
    }

    input.gridEdits = R"(
CARFIN
'LGR1' 1 1 1 2 1 1 2 2 2 /
ENDFIN
)";
    const auto applied = applyRegionSplit(input, {1});
    BOOST_REQUIRE_EQUAL(applied.original->state.getLgrs().size(), std::size_t{1});
    BOOST_CHECK(newAquiferCells(*applied.split) == (std::vector<std::array<std::size_t, 3>>{{0, 0, 1}}));
}

BOOST_AUTO_TEST_CASE(NumericalAquiferRegionSplitConsistentAquifer)
{
    // Aquifer 1 restricted to region 2: nothing to change.
    const RegionSplitCase rsc { regionSplitDeck(regionSplitAqunum, R"(
AQUCON
 1 1 5 2 2 4 5 'J-' /
/
)", "") };
    const auto split = Opm::NumericalAquiferRegionSplit {
        rsc.aquifers, {1}, rsc.state.getInputGrid(), rsc.state.fieldProps(), rsc.state.getLgrs()
    };
    BOOST_CHECK(split.empty());
    BOOST_CHECK(split.deckChanges(rsc.state.getDeckUnitSystem()).empty());
}
