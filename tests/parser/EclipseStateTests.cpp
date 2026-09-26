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

#define BOOST_TEST_MODULE EclipseStateTests

#include <boost/test/unit_test.hpp>

#include <boost/version.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/Box.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FaultCollection.hpp>
#include <opm/input/eclipse/EclipseState/Grid/Fault.hpp>
#include <opm/input/eclipse/EclipseState/Grid/TransMult.hpp>
#include <opm/input/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/input/eclipse/EclipseState/SimulationConfig/SimulationConfig.hpp>
#include <opm/input/eclipse/EclipseState/SimulationConfig/ThresholdPressure.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <opm/input/eclipse/Units/Units.hpp>

#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/EclipseState/Grid/NNC.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/E.hpp>

#include <cstddef>
#include <filesystem>

using namespace Opm;

inline std::string prepath() {
    int idx;
#if BOOST_VERSION / 100000 == 1 && BOOST_VERSION / 100 % 1000 < 71
    idx = 2;
#else
    idx = 1;
#endif
    const std::filesystem::path path {
        boost::unit_test::framework::master_test_suite().argv[idx]
    };
    return std::filesystem::canonical(path).generic_string();
}

static Deck createDeckTOP() {
    const char *deckData =
"RUNSPEC\n"
"\n"
"DIMENS\n"
" 10 10 10 /\n"
"GRID\n"
"DX\n"
"1000*0.25 /\n"
"DYV\n"
"10*0.25 /\n"
"DZ\n"
"1000*0.25 /\n"
"TOPS\n"
"1000*0.25 /\n"
"BOX\n"
"1 10 1 10 1 1 /\n"
"PORO \n"
"100*0.10 /\n"
"PERMX \n"
"100*0.25 /\n"
"ENDBOX\n"
"EDIT\n"
"OIL\n"
"\n"
"GAS\n"
"\n"
"TITLE\n"
"The title\n"
"\n"
"START\n"
"8 MAR 1998 /\n"
"\n"
"PROPS\n"
"REGIONS\n"
"SWAT\n"
"1000*1 /\n"
"SATNUM\n"
"1000*2 /\n"
"\n";

    Parser parser;
    return parser.parseString( deckData );
}

BOOST_AUTO_TEST_CASE(GetPOROTOPBased) {
    auto deck = createDeckTOP();
    EclipseState state(deck );
    const auto& fp = state.fieldProps();

    const auto& poro  = fp.get_double( "PORO" );
    const auto& permx = fp.get_double( "PERMX" );

    for (std::size_t i=0; i < poro.size(); i++) {
        BOOST_CHECK_EQUAL( 0.10 , poro[i]);
        BOOST_CHECK_EQUAL( 0.25 * Metric::Permeability , permx[i]);
    }
}

static Deck createDeck() {
const char *deckData =
"RUNSPEC\n"
"\n"
"DIMENS\n"
" 10 10 10 /\n"
"GRID\n"
"DX\n"
"1000*0.25 /\n"
"DY\n"
"1000*0.25 /\n"
"DZ\n"
"1000*0.25 /\n"
"TOPS\n"
"100*0.25 /\n"
"FAULTS \n"
"  'F1'  1  1  1  4   1  4  'X' / \n"
"  'F2'  5  5  1  4   1  4  'X-' / \n"
"/\n"
"MULTFLT \n"
"  'F1' 0.50 / \n"
"  'F2' 0.50 / \n"
"/\n"
"PORO\n"
"  1000*0.15 /"
"EDIT\n"
"MULTFLT /\n"
"  'F2' 0.25 / \n"
"/\n"
"OIL\n"
"\n"
"GAS\n"
"\n"
"TITLE\n"
"The title\n"
"\n"
"START\n"
"8 MAR 1998 /\n"
"\n"
"PROPS\n"
"REGIONS\n"
"SWAT\n"
"1000*1 /\n"
"SATNUM\n"
"1000*2 /\n"
"\n";

    Parser parser;
    return parser.parseString( deckData );
}

static Deck createDeckNoFaults() {
const char *deckData =
"RUNSPEC\n"
"\n"
"DIMENS\n"
" 10 10 10 /\n"
"GRID\n"
"DX\n"
"1000*0.25 /\n"
"DY\n"
"1000*0.25 /\n"
"DZ\n"
"1000*0.25 /\n"
"TOPS\n"
"100*0.25 /\n"
"PORO\n"
"  1000*0.15 /"
"PROPS\n"
"-- multiply one layer for each face\n"
"MULTX\n"
" 100*1 100*10 800*1 /\n"
"MULTX-\n"
" 200*1 100*11 700*1 /\n"
"MULTY\n"
" 300*1 100*12 600*1 /\n"
"MULTY-\n"
" 400*1 100*13 500*1 /\n"
"MULTZ\n"
" 500*1 100*14 400*1 /\n"
"MULTZ-\n"
" 600*1 100*15 300*1 /\n"
"\n";

    Parser parser;
    return parser.parseString( deckData );
}

BOOST_AUTO_TEST_CASE(CreateSchedule) {
    auto deck = createDeck();
    auto python = std::make_shared<Python>();
    EclipseState state(deck);
    Schedule schedule(deck, state, python);
    BOOST_CHECK_EQUAL(schedule.getStartTime(), asTimeT(TimeStampUTC( 1998 , 3 , 8)));
}

static Deck createDeckSimConfig() {
const std::string& inputStr = "RUNSPEC\n"
                "EQLOPTS\n"
                "THPRES /\n "
                "DIMENS\n"
                "10 3 4 /\n"
                "\n"
                "GRID\n"
                "DX\n"
                "120*0.25 /\n"
                "DY\n"
                "120*0.25 /\n"
                "DZ\n"
                "120*0.25 /\n"
                "TOPS\n"
                "30*0.25 /\n"
                "PORO\n"
                "  120*0.15/ \n"
                "REGIONS\n"
                "EQLNUM\n"
                "10*1 10*2 100*3 /\n "
                "\n"
                "SOLUTION\n"
                "THPRES\n"
                "1 2 12.0/\n"
                "1 3 5.0/\n"
                "2 3 7.0/\n"
                "/\n"
                "\n";

    Parser parser;
    return parser.parseString( inputStr );
}

BOOST_AUTO_TEST_CASE(CreateSimulationConfig) {

    auto deck = createDeckSimConfig();
    EclipseState state(deck);
    const auto& simConf = state.getSimulationConfig();

    BOOST_CHECK(simConf.useThresholdPressure());
    BOOST_CHECK_EQUAL(simConf.getThresholdPressure().size(), 3);
}

BOOST_AUTO_TEST_CASE(PhasesCorrect) {
    auto deck = createDeck();
    EclipseState state( deck );
    const auto& phases = state.runspec().phases();
    BOOST_CHECK(  phases.active( Phase::OIL ) );
    BOOST_CHECK(  phases.active( Phase::GAS ) );
    BOOST_CHECK( !phases.active( Phase::WATER ) );
}

BOOST_AUTO_TEST_CASE(TitleCorrect) {
    auto deck = createDeck();
    EclipseState state( deck );

    BOOST_CHECK_EQUAL( state.getTitle(), "The title" );
}

BOOST_AUTO_TEST_CASE(IntProperties) {
    auto deck = createDeck();
    EclipseState state( deck );

    BOOST_CHECK_EQUAL( false, state.fieldProps().supported<int>( "NONO" ) );
    BOOST_CHECK_EQUAL( true,  state.fieldProps().supported<int>( "SATNUM" ) );
    BOOST_CHECK_EQUAL( true,  state.fieldProps().has_int( "SATNUM" ) );
}

BOOST_AUTO_TEST_CASE(GetProperty) {
    auto deck = createDeck();
    EclipseState state(deck);

    const auto& satnum = state.fieldProps().get_global_int("SATNUM");
    BOOST_CHECK_EQUAL(1000U , satnum.size() );
    for (std::size_t i=0; i < satnum.size(); i++)
        BOOST_CHECK_EQUAL( 2 , satnum[i]);
}

BOOST_AUTO_TEST_CASE(GetTransMult) {
    auto deck = createDeck();
    EclipseState state( deck );
    const auto& transMult = state.getTransMult();

    BOOST_CHECK_EQUAL( 1.0, transMult.getMultiplier( 1, 0, 0, FaceDir::XPlus ) );
    BOOST_CHECK_THROW( transMult.getMultiplier( 1000, FaceDir::XPlus ), std::invalid_argument );
}

BOOST_AUTO_TEST_CASE(GetFaults) {
    auto deck = createDeck();
    EclipseState state( deck );
    const auto& faults = state.getFaults();

    BOOST_CHECK( faults.hasFault( "F1" ) );
    BOOST_CHECK( faults.hasFault( "F2" ) );

    const auto& F1 = faults.getFault( "F1" );
    const auto& F2 = faults.getFault( "F2" );

    BOOST_CHECK_EQUAL( 0.50, F1.getTransMult() );
    BOOST_CHECK_EQUAL( 0.25, F2.getTransMult() );

    const auto& transMult = state.getTransMult();
    BOOST_CHECK_EQUAL( transMult.getMultiplier( 0, 0, 0, FaceDir::XPlus ), 0.50 );
    BOOST_CHECK_EQUAL( transMult.getMultiplier( 4, 3, 0, FaceDir::XMinus ), 0.25 );
    BOOST_CHECK_EQUAL( transMult.getMultiplier( 4, 3, 0, FaceDir::ZPlus ), 1.00 );
}

BOOST_AUTO_TEST_CASE(FaceTransMults) {
    auto deck = createDeckNoFaults();
    EclipseState state(deck);
    const auto& transMult = state.getTransMult();

    for (int i = 0; i < 10; ++ i) {
        for (int j = 0; j < 10; ++ j) {
            for (int k = 0; k < 10; ++ k) {
                if (k == 1)
                    BOOST_CHECK_EQUAL(transMult.getMultiplier(i, j, k, FaceDir::XPlus), 10.0);
                else
                    BOOST_CHECK_EQUAL(transMult.getMultiplier(i, j, k, FaceDir::XPlus), 1.0);

                if (k == 2)
                    BOOST_CHECK_EQUAL(transMult.getMultiplier(i, j, k, FaceDir::XMinus), 11.0);
                else
                    BOOST_CHECK_EQUAL(transMult.getMultiplier(i, j, k, FaceDir::XMinus), 1.0);

                if (k == 3)
                    BOOST_CHECK_EQUAL(transMult.getMultiplier(i, j, k, FaceDir::YPlus), 12.0);
                else
                    BOOST_CHECK_EQUAL(transMult.getMultiplier(i, j, k, FaceDir::YPlus), 1.0);

                if (k == 4)
                    BOOST_CHECK_EQUAL(transMult.getMultiplier(i, j, k, FaceDir::YMinus), 13.0);
                else
                    BOOST_CHECK_EQUAL(transMult.getMultiplier(i, j, k, FaceDir::YMinus), 1.0);

                if (k == 5)
                    BOOST_CHECK_EQUAL(transMult.getMultiplier(i, j, k, FaceDir::ZPlus), 14.0);
                else
                    BOOST_CHECK_EQUAL(transMult.getMultiplier(i, j, k, FaceDir::ZPlus), 1.0);

                if (k == 6)
                    BOOST_CHECK_EQUAL(transMult.getMultiplier(i, j, k, FaceDir::ZMinus), 15.0);
                else
                    BOOST_CHECK_EQUAL(transMult.getMultiplier(i, j, k, FaceDir::ZMinus), 1.0);
            }
        }
    }
}

static Deck createDeckNoGridOpts() {
    const char *deckData =
        "RUNSPEC\n"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "DX\n"
        "1000*0.25 /\n"
        "DY\n"
        "1000*0.25 /\n"
        "DZ\n"
        "1000*0.25 /\n"
        "TOPS\n"
        "100*0.25 /\n"
        "PORO\n"
        "  1000*0.15 /\n"
        "FLUXNUM\n"
        "  1000*1 /\n"
        "MULTNUM\n"
        "  1000*1 /\n";

    Parser parser;
    return parser.parseString(deckData) ;
}

static Deck createDeckWithGridOpts() {
    const char *deckData =
        "RUNSPEC\n"
        "GRIDOPTS\n"
        "  'YES'   10 /"
        "\n"
        "DIMENS\n"
        " 10 10 10 /\n"
        "GRID\n"
        "DX\n"
        "1000*0.25 /\n"
        "DY\n"
        "1000*0.25 /\n"
        "DZ\n"
        "1000*0.25 /\n"
        "TOPS\n"
        "100*0.25 /\n"
        "PORO\n"
        "  1000*0.15 /\n"
        "FLUXNUM\n"
        "  1000*1 /\n"
        "MULTNUM\n"
        "  1000*1 /\n";

    Parser parser;
    return parser.parseString( deckData );
}

BOOST_AUTO_TEST_CASE(NoGridOptsDefaultRegion) {
    auto deck = createDeckNoGridOpts();
    EclipseState state(deck);
    const auto& fp = state.fieldProps();
    const auto& multnum = fp.get_int("MULTNUM");
    const auto& fluxnum = fp.get_int("FLUXNUM");
    const auto  default_kw = fp.default_region();
    const auto& def_pro = fp.get_int(default_kw);

    BOOST_CHECK_EQUAL( &fluxnum  , &def_pro );
    BOOST_CHECK_NE( &fluxnum  , &multnum );
}

BOOST_AUTO_TEST_CASE(WithGridOptsDefaultRegion) {
    auto deck = createDeckWithGridOpts();
    EclipseState state(deck);
    const auto& fp = state.fieldProps();
    const auto& multnum = fp.get_int("MULTNUM");
    const auto& fluxnum = fp.get_int("FLUXNUM");
    const auto  default_kw = fp.default_region();
    const auto& def_pro = fp.get_int(default_kw);

    BOOST_CHECK_EQUAL( &multnum , &def_pro );
    BOOST_CHECK_NE( &fluxnum  , &multnum );
}

BOOST_AUTO_TEST_CASE(TestIOConfigBaseName) {
    Parser parser;
    auto deck = parser.parseFile(prepath() + "/IOConfig/SPE1CASE2.DATA");
    EclipseState state(deck);
    const auto& io = state.cfg().io();
    BOOST_CHECK_EQUAL(io.getBaseName(), "SPE1CASE2");
    BOOST_CHECK_EQUAL(io.getOutputDir(), prepath() + "/IOConfig");

    Parser parser2;
    auto deck2 = createDeckWithGridOpts();
    EclipseState state2(deck2);
    const auto& io2 = state2.cfg().io();
    BOOST_CHECK_EQUAL(io2.getBaseName(), "");
    BOOST_CHECK_EQUAL(io2.getOutputDir(), ".");
}

BOOST_AUTO_TEST_CASE(TestBox) {
    const char * regionData =
                "START             --\n"
                "10 MAI 2007 /\n"
                "RUNSPEC\n"
                "DIMENS\n"
                "2 2 1 /\n"
                "GRID\n"
                "DX\n"
                "4*0.25 /\n"
                "BOX\n"
                "1* 1 1 1 1 1 /\n"
                "DY\n"
                "4*0.25 /\n"
                "DZ\n"
                "4*0.25 /\n"
                "TOPS\n"
                "4*0.25 /\n"
                "ENDBOX\n"
                "PORO\n"
                "  4*0.15 /\n"
                "REGIONS\n"
                "OPERNUM\n"
                "3 3 1 2 /\n"
                "FIPNUM\n"
                "1 1 2 3 /\n";
    Parser parser;
    auto deck = parser.parseString(regionData);
    EclipseState state(deck);

}

BOOST_AUTO_TEST_CASE(THCO2MIX) {
    const auto deck_string = R"(
RUNSPEC

DIMENS
 2 2 1 /

GRID

DX
 4*1 /
DY
 4*1 /
DZ
 4*1 /
TOPS
 4*0.0 /

PORO
 4*0.3 /

PROPS

THCO2MIX
  NONE IDEAL IDEAL  /

)";

    Parser parser;
    const auto& deck = parser.parseString(deck_string);
    EclipseState state(deck);
    Co2StoreConfig config = state.getCo2StoreConfig();
    BOOST_CHECK( config.brine_type == Co2StoreConfig::SaltMixingType::NONE);
    BOOST_CHECK( config.liquid_type == Co2StoreConfig::LiquidMixingType::IDEAL);
    BOOST_CHECK( config.gas_type == Co2StoreConfig::GasMixingType::IDEAL);
}

BOOST_AUTO_TEST_CASE(EzrokhiTablesTest) {
    const auto deck_string = R"(
        RUNSPEC

        DIMENS
        2 2 1 /

        TABDIMS
        8* 1 /

        GRID

        DX
        4*1 /
        DY
        4*1 /
        DZ
        4*1 /
        TOPS
        4*0.0 /

        PORO
        4*0.3 /

        PROPS

        CNAMES
        H2O CO2 NACL /

        DENAQA
        1.0 2.0 3.0
        4.0 5.0 6.0
        7.0 8.0 9.0
        /

        VISCAQA
        11.0 12.0 13.0
        14.0 15.0 16.0
        17.0 18.0 19.0
        /

    )";

    Opm::Parser parser;
    auto deck = parser.parseString(deck_string);
    EclipseState state(deck);
    Co2StoreConfig config = state.getCo2StoreConfig();

    const auto& denaqa = config.getDenaqaTables();
    const double epsilon = 0.00001;
    BOOST_CHECK_CLOSE(1.0, denaqa[0].getC0("H2O"), epsilon);
    BOOST_CHECK_CLOSE(2.0, denaqa[0].getC1("H2O"), epsilon);
    BOOST_CHECK_CLOSE(3.0, denaqa[0].getC2("H2O"), epsilon);

    BOOST_CHECK_CLOSE(4.0, denaqa[0].getC0("CO2"), epsilon);
    BOOST_CHECK_CLOSE(5.0, denaqa[0].getC1("CO2"), epsilon);
    BOOST_CHECK_CLOSE(6.0, denaqa[0].getC2("CO2"), epsilon);

    BOOST_CHECK_CLOSE(7.0, denaqa[0].getC0("NACL"), epsilon);
    BOOST_CHECK_CLOSE(8.0, denaqa[0].getC1("NACL"), epsilon);
    BOOST_CHECK_CLOSE(9.0, denaqa[0].getC2("NACL"), epsilon);

    const auto& viscaqa = config.getViscaqaTables();
    BOOST_CHECK_CLOSE(11.0, viscaqa[0].getC0("H2O"), epsilon);
    BOOST_CHECK_CLOSE(12.0, viscaqa[0].getC1("H2O"), epsilon);
    BOOST_CHECK_CLOSE(13.0, viscaqa[0].getC2("H2O"), epsilon);

    BOOST_CHECK_CLOSE(14.0, viscaqa[0].getC0("CO2"), epsilon);
    BOOST_CHECK_CLOSE(15.0, viscaqa[0].getC1("CO2"), epsilon);
    BOOST_CHECK_CLOSE(16.0, viscaqa[0].getC2("CO2"), epsilon);

    BOOST_CHECK_CLOSE(17.0, viscaqa[0].getC0("NACL"), epsilon);
    BOOST_CHECK_CLOSE(18.0, viscaqa[0].getC1("NACL"), epsilon);
    BOOST_CHECK_CLOSE(19.0, viscaqa[0].getC2("NACL"), epsilon);
}

BOOST_AUTO_TEST_CASE(SALTMFTest) {
    const auto deck_salinity_input = R"(
        RUNSPEC

        DIMENS
        2 2 1 /

        GRID

        DX
        4*1 /
        DY
        4*1 /
        DZ
        4*1 /
        TOPS
        4*0.0 /

        PORO
        4*0.3 /

        PROPS

        SALINITY
        0.7 /
    )";
    const auto deck_saltmf_input = R"(
        RUNSPEC

        DIMENS
        2 2 1 /

        GRID

        DX
        4*1 /
        DY
        4*1 /
        DZ
        4*1 /
        TOPS
        4*0.0 /

        PORO
        4*0.3 /

        PROPS

        SALTMF
        0.012443215484890378/
    )";

    Opm::Parser parser_salinity;
    Opm::Parser parser_saltmf;
    auto deck_salinity = parser_salinity.parseString(deck_salinity_input);
    auto deck_saltmf = parser_saltmf.parseString(deck_saltmf_input);
    EclipseState state_salinity(deck_salinity);
    EclipseState state_saltmf(deck_saltmf);
    Co2StoreConfig config_salinity = state_salinity.getCo2StoreConfig();
    Co2StoreConfig config_saltmf = state_saltmf.getCo2StoreConfig();

    const auto& salinity = config_salinity.salinity();
    const auto& saltmf = config_saltmf.salinity();
    const double epsilon = 0.00001;
    BOOST_CHECK_CLOSE(salinity, saltmf, epsilon);
}

namespace {

Deck createDualPorosityStateDeck(const std::string& dimens,
                                 const std::string& gridProps,
                                 bool dualporo = true)
{
    const std::string deckData =
        "RUNSPEC\n"
        "\n"
        "OIL\n"
        "WATER\n"
        "DIMENS\n"
        " " + dimens + " /\n" +
        (dualporo ? "DUALPORO\n" : "") +
        "GRID\n" +
        gridProps +
        "\n";
    Parser parser;
    return parser.parseString(deckData);
}

const std::string dpMinProps =
    "DX\n 2*100 /\n"
    "DY\n 2*100 /\n"
    "DZ\n 2*10 /\n"
    "TOPS\n 2*2000 /\n"
    "PORO\n 0.20 0.01 /\n"
    "PERMX\n 1.0 1000.0 /\n";

// 1 mD in SI times bulk volume (1e5 m3) times sigma (0.12 1/m2).
constexpr double dpMinExpectedTrans = 9.869232667160130e-16 * 1.0e5 * 0.12;

} // anonymous namespace

BOOST_AUTO_TEST_CASE(DualPorositySigmaNNCFromScalarSigma) {
    // One block, sigma as a single field value.
    auto deck = createDualPorosityStateDeck("1 1 2", dpMinProps + "SIGMA\n 0.12 /\n");
    EclipseState es(deck);

    const auto& nnc = es.getInputNNC().input();
    BOOST_REQUIRE_EQUAL(nnc.size(), 1U);
    BOOST_CHECK_EQUAL(nnc[0].cell1, 0U);
    BOOST_CHECK_EQUAL(nnc[0].cell2, 1U);
    BOOST_CHECK_CLOSE(nnc[0].trans, dpMinExpectedTrans, 1e-4);
}

BOOST_AUTO_TEST_CASE(DualPorositySigmaNNCFromSigmav) {
    // Cell-by-cell sigma: the matrix cell's value drives the coupling, the
    // fracture-half entry carries no meaning.
    auto deck = createDualPorosityStateDeck("1 1 2", dpMinProps + "SIGMAV\n 0.12 0.0 /\n");
    EclipseState es(deck);

    const auto& nnc = es.getInputNNC().input();
    BOOST_REQUIRE_EQUAL(nnc.size(), 1U);
    BOOST_CHECK_EQUAL(nnc[0].cell1, 0U);
    BOOST_CHECK_EQUAL(nnc[0].cell2, 1U);
    BOOST_CHECK_CLOSE(nnc[0].trans, dpMinExpectedTrans, 1e-4);
}

BOOST_AUTO_TEST_CASE(DualPorosityNoSigmaNoCoupling) {
    // Without SIGMA/SIGMAV there is no coupling — and no failure.
    auto deck = createDualPorosityStateDeck("1 1 2", dpMinProps);
    EclipseState es(deck);
    BOOST_CHECK(es.getInputNNC().input().empty());
}

BOOST_AUTO_TEST_CASE(DualPorosityInactiveTwinNoCoupling) {
    // An inactive fracture twin suppresses the pair's connection.
    auto deck = createDualPorosityStateDeck(
        "1 1 2", dpMinProps + "SIGMA\n 0.12 /\n" + "ACTNUM\n 1 0 /\n");
    EclipseState es(deck);
    BOOST_CHECK(es.getInputNNC().input().empty());
}

BOOST_AUTO_TEST_CASE(DualPorositySigmaNNCMultiBlock) {
    // 2x1x4: four matrix cells, each coupled to its twin four cells later.
    const std::string props =
        "DX\n 8*100 /\n"
        "DY\n 8*100 /\n"
        "DZ\n 8*10 /\n"
        "TOPS\n 2*2000 2*2010 2*2000 2*2010 /\n"
        "PORO\n 4*0.20 4*0.01 /\n"
        "PERMX\n 4*1.0 4*1000.0 /\n"
        "SIGMA\n 0.12 /\n";
    auto deck = createDualPorosityStateDeck("2 1 4", props);
    EclipseState es(deck);

    const auto& nnc = es.getInputNNC().input();
    BOOST_REQUIRE_EQUAL(nnc.size(), 4U);
    for (std::size_t n = 0; n < 4; ++n) {
        BOOST_CHECK_EQUAL(nnc[n].cell1, n);
        BOOST_CHECK_EQUAL(nnc[n].cell2, n + 4);
        BOOST_CHECK_CLOSE(nnc[n].trans, dpMinExpectedTrans, 1e-4);
    }
}

BOOST_AUTO_TEST_CASE(DualPorositySinglePorosityNoInjectedNNC) {
    // Without DUALPORO nothing is injected, SIGMA or not: the keyword still
    // aborts later in the simulator, but the state must not invent NNCs.
    auto deck = createDualPorosityStateDeck("1 1 2", dpMinProps, false);
    EclipseState es(deck);
    BOOST_CHECK(es.getInputNNC().input().empty());
}

BOOST_AUTO_TEST_CASE(DualPorosityCouplingCannotBeEdited) {
    // The coupling transmissibility is computed from the shape factor and the matrix
    // permeability. An EDITNNC naming the same pair would rescale it silently, because
    // edits are applied after the coupling is built. Refuse instead.
    const char* deckData =
        "RUNSPEC\n"
        "OIL\nWATER\n"
        // Two matrix layers, so a twin pair is NOT also a geometric neighbour: at one
        // matrix layer the twins are adjacent cells and the edit is treated as an
        // ordinary neighbour multiplier rather than an NNC edit.
        "DIMENS\n 1 1 4 /\n"
        "DUALPORO\n"
        "GRID\n"
        "DX\n 4*100 /\n"
        "DY\n 4*100 /\n"
        "DZ\n 4*10 /\n"
        "TOPS\n 4*2000 /\n"
        "PORO\n 2*0.2 2*0.01 /\n"
        "PERMX\n 2*1.0 2*1000.0 /\n"
        "SIGMA\n 0.1 /\n"
        "EDIT\n"
        "EDITNNC\n"
        " 1 1 1 1 1 3 0.5 /\n"
        "/\n"
        "\n";

    const auto deck = Opm::Parser{}.parseString(deckData);
    BOOST_CHECK_THROW(Opm::EclipseState{ deck }, Opm::OpmInputError);
}

