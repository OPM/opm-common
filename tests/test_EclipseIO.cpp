/*
  Copyright 2014 Andreas Lauser

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

#define BOOST_TEST_MODULE EclipseIO
#include <boost/test/unit_test.hpp>

#include <opm/output/eclipse/EclipseIO.hpp>
#include <opm/output/eclipse/RestartValue.hpp>

#include <opm/output/data/Cells.hpp>

#include <opm/io/eclipse/EclFile.hpp>
#include <opm/io/eclipse/EGrid.hpp>
#include <opm/io/eclipse/ERst.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/Schedule/Action/State.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQState.hpp>
#include <opm/input/eclipse/Schedule/Well/WellTestState.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>
#include <opm/input/eclipse/Units/Units.hpp>

#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <algorithm>
#include <fstream>
#include <ios>
#include <map>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include <time.h>

#include <tests/WorkArea.hpp>

using namespace Opm;

namespace {

bool keywordExists(const std::vector<EclIO::EclFile::EclEntry>& knownVec,
                   const std::string&                           arrayname)
{
    return std::any_of(knownVec.begin(), knownVec.end(),
        [&arrayname](const EclIO::EclFile::EclEntry& entry) -> bool
    {
        return std::get<0>(entry) == arrayname;
    });
}

template <typename T>
T sum(const std::vector<T>& array)
{
    return std::accumulate(array.begin(), array.end(), T(0));
}

data::Solution createBlackoilState( int timeStepIdx, int numCells ) {

    std::vector< double > pressure( numCells );
    std::vector< double > swat( numCells );
    std::vector< double > sgas( numCells );
    std::vector< double > rs( numCells );
    std::vector< double > rv( numCells );

    for( int cellIdx = 0; cellIdx < numCells; ++cellIdx) {

        pressure[cellIdx] = timeStepIdx*1e5 + 1e4 + cellIdx;
        sgas[cellIdx] = timeStepIdx*1e5 +2.2e4 + cellIdx;
        swat[cellIdx] = timeStepIdx*1e5 +2.3e4 + cellIdx;

        // oil vaporization factor
        rv[cellIdx] = timeStepIdx*1e5 +3e4 + cellIdx;
        // gas dissolution factor
        rs[cellIdx] = timeStepIdx*1e5 + 4e4 + cellIdx;
    }

    data::Solution solution;

    solution.insert( "PRESSURE" , UnitSystem::measure::pressure , pressure, data::TargetType::RESTART_SOLUTION );
    solution.insert( "SWAT" , UnitSystem::measure::identity , swat, data::TargetType::RESTART_SOLUTION );
    solution.insert( "SGAS" , UnitSystem::measure::identity , sgas, data::TargetType::RESTART_SOLUTION );
    solution.insert( "RS" , UnitSystem::measure::identity , rs, data::TargetType::RESTART_SOLUTION );
    solution.insert( "RV" , UnitSystem::measure::identity , rv, data::TargetType::RESTART_SOLUTION );

    return solution;
}

template< typename T, typename U >
void compareErtData(const std::vector< T > &src,
                    const std::vector< U > &dst,
                    double tolerance ) {
    BOOST_REQUIRE_EQUAL(src.size(), dst.size());

    for (size_t i = 0; i < src.size(); ++i)
        BOOST_CHECK_CLOSE(src[i], dst[i], tolerance);
}

void compareErtData(const std::vector<int> &src, const std::vector<int> &dst)
{
    BOOST_CHECK_EQUAL_COLLECTIONS( src.begin(), src.end(),
                                   dst.begin(), dst.end() );
}

void checkEgridFile(const EclipseGrid& eclGrid)
{
    auto egridFile = EclIO::EGrid("FOO.EGRID");

    {
        const auto& coord  = egridFile.get<float>("COORD");
        const auto& expect = eclGrid.getCOORD();
        compareErtData(expect, coord, 1e-6);
    }

    {
        const auto& zcorn  = egridFile.get<float>("ZCORN");
        const auto& expect = eclGrid.getZCORN();
        compareErtData(expect, zcorn, 1e-6);
    }

    if (egridFile.hasKey("ACTNUM")) {
        const auto& actnum = egridFile.get<int>("ACTNUM");
        auto expect = eclGrid.getACTNUM();

        if (expect.empty()) {
            const auto numCells = eclGrid.getNX() * eclGrid.getNY() * eclGrid.getNZ();
            expect.assign(numCells, 1);
        }

        compareErtData(expect, actnum);
    }
}

void checkInitFile(const Deck& deck, const data::Solution& simProps)
{
    EclIO::EclFile initFile { "FOO.INIT" };

    if (initFile.hasKey("PORO")) {
        const auto& poro   = initFile.get<float>("PORO");
        const auto& expect = deck["PORO"].back().getSIDoubleData();

        compareErtData(expect, poro, 1e-4);
    }

    if (initFile.hasKey("PERMX")) {
        const auto& expect = deck["PERMX"].back().getSIDoubleData();
        auto        permx  = initFile.get<float>("PERMX");

        std::ranges::transform(permx, permx.begin(),
                               [](const auto& kx) { return kx * 9.869233e-16; });

        compareErtData(expect, permx, 1e-4);
    }

    // These arrays should always be in the INIT file, irrespective of
    // keyword presence in the inut deck.
    BOOST_CHECK_MESSAGE( initFile.hasKey("NTG"), R"(INIT file must have "NTG" array)" );
    BOOST_CHECK_MESSAGE( initFile.hasKey("FIPNUM"), R"(INIT file must have "FIPNUM" array)");
    BOOST_CHECK_MESSAGE( initFile.hasKey("SATNUM"), R"(INIT file must have "SATNUM" array)");
    std::array<std::string, 3> multipliers{"MULTX", "MULTY", "MULTZ"};
    for (const auto& mult: multipliers) {
       BOOST_CHECK_MESSAGE( initFile.hasKey(mult), R"(INIT file must have ")" + mult + R"(" array)" );
    }

    for (const auto& prop : simProps) {
        BOOST_CHECK_MESSAGE( initFile.hasKey(prop.first), R"(INIT file must have ")" + prop.first + R"(" array)" );
    }
}

void checkRestartFile( int timeStepIdx ) {
    EclIO::ERst rstFile{ "FOO.UNRST" };

    for (int i = 1; i <= timeStepIdx; ++i) {
        if (! rstFile.hasReportStepNumber(i))
            continue;

        auto sol = createBlackoilState( i, 3 * 3 * 3 );

        rstFile.loadReportStepNumber(i);

        const auto& knownVec = rstFile.listOfRstArrays(i);

        if (keywordExists(knownVec, "PRESSURE")) {
            const auto& press = rstFile.getRestartData<float>("PRESSURE", i, 0);
            std::ranges::transform(sol.data<double>("PRESSURE"), sol.data<double>("PRESSURE").begin(),
                                   [](const auto& x) { return x / Metric::Pressure; });

            compareErtData( sol.data<double>("PRESSURE"), press, 1e-4 );
        }

        if (keywordExists(knownVec, "SWAT")) {
            const auto& swat = rstFile.getRestartData<float>("SWAT", i, 0);
            compareErtData( sol.data<double>("SWAT"), swat, 1e-4 );
        }

        if (keywordExists(knownVec, "SGAS")) {
            const auto& sgas = rstFile.getRestartData<float>("SGAS", i, 0);
            compareErtData( sol.data<double>("SGAS"), sgas, 1e-4 );
        }

        if (keywordExists(knownVec, "KRO")) {
            const auto& kro = rstFile.getRestartData<float>("KRO", i, 0);
            BOOST_CHECK_CLOSE(1.0 * i * kro.size(), sum(kro), 1.0e-8);
        }

        if (keywordExists(knownVec, "KRG")) {
            const auto& krg = rstFile.getRestartData<float>("KRG", i, 0);
            BOOST_CHECK_CLOSE(10.0 * i * krg.size(), sum(krg), 1.0e-8);
        }
    }
}

time_t ecl_util_make_date( const int day, const int month, const int year )
{
    const auto ymd = Opm::TimeStampUTC::YMD{ year, month, day };
    return static_cast<time_t>(asTimeT(Opm::TimeStampUTC{ymd}));
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(EclipseIOIntegration)
{
    const auto deckString = std::string { R"(RUNSPEC
UNIFOUT
OIL
GAS
WATER
METRIC
DIMENS
3 3 3/
GRID
PORO
27*0.3 /
PERMX
27*1 /
INIT
DXV
1.0 2.0 3.0 /
DYV
4.0 5.0 6.0 /
DZV
7.0 8.0 9.0 /
TOPS
9*100 /
PORO
  27*0.15 /
PROPS
SWATINIT
 9*0.1   -- K=1
 9*0.5   -- K=2
 9*1.0 / -- K=3
REGIONS
SATNUM
27*2 /
FIPNUM
27*3 /
SOLUTION
RPTRST
BASIC=2
/
SCHEDULE
TSTEP
1.0 2.0 3.0 4.0 5.0 6.0 7.0 /
WELSPECS
'INJ' 'G' 1 1 2000 'GAS' /
'PROD' 'G' 3 3 1000 'OIL' /
/
)" };

    auto write_and_check = [&deckString]( int first = 1, int last = 5 ) {
        const auto deck = Parser().parseString( deckString);
        auto es = EclipseState( deck );
        const auto& eclGrid = es.getInputGrid();
        const Schedule schedule(deck, es, std::make_shared<Python>());
        const SummaryConfig summary_config( deck, schedule, es.fieldProps(), es.aquifer());
        const SummaryState st(TimeService::now(), 0.0);
        es.getIOConfig().setBaseName( "FOO" );

        EclipseIO eclWriter( es, eclGrid , schedule, summary_config);

        using measure = UnitSystem::measure;
        using TargetType = data::TargetType;
        const auto start_time = ecl_util_make_date( 10, 10, 2008 );
        std::vector<double> tranx(3*3*3);
        std::vector<double> trany(3*3*3);
        std::vector<double> tranz(3*3*3);
        const data::Solution eGridProps {
            { "TRANX", data::CellData { measure::transmissibility, tranx, TargetType::INIT } },
            { "TRANY", data::CellData { measure::transmissibility, trany, TargetType::INIT } },
            { "TRANZ", data::CellData { measure::transmissibility, tranz, TargetType::INIT } },
        };

        std::map<std::string, std::vector<int>> int_data =  {{"STR_ULONGNAME" , {1,1,1,1,1,1,1,1} } };

        std::vector<int> v(27); v[2] = 67; v[26] = 89;
        int_data["STR_V"] = v;

        eclWriter.writeInitial( );

        BOOST_CHECK_THROW(eclWriter.writeInitial(eGridProps, int_data), std::invalid_argument);

        int_data.erase("STR_ULONGNAME");
        eclWriter.writeInitial(eGridProps, int_data);

        data::Wells wells;
        data::GroupAndNetworkValues grp_nwrk;

        for (int i = first; i < last; ++i) {
            data::Solution sol = createBlackoilState(i, 3 * 3 * 3);
            sol.insert("KRO", measure::identity, std::vector<double>(3*3*3, i), TargetType::RESTART_AUXILIARY);
            sol.insert("KRG", measure::identity, std::vector<double>(3*3*3, i*10), TargetType::RESTART_AUXILIARY);

            Action::State action_state;
            WellTestState wtest_state;
            UDQState udq_state(1);
            RestartValue restart_value(sol, wells, grp_nwrk, {});
            auto first_step = ecl_util_make_date(10 + i, 11, 2008);
            eclWriter.writeTimeStep(action_state,
                                    wtest_state,
                                    st,
                                    udq_state,
                                    i,
                                    false,
                                    first_step - start_time,
                                    std::move(restart_value));

            checkRestartFile(i);
        }

        checkInitFile(deck, eGridProps);
        checkEgridFile(eclGrid);

        EclIO::EclFile initFile("FOO.INIT");

        {
            BOOST_CHECK_MESSAGE(initFile.hasKey("STR_V"), R"(INIT file must have "STR_V" array)" );

            const auto& kw = initFile.get<int>("STR_V");
            BOOST_CHECK_EQUAL(67, kw[ 2]);
            BOOST_CHECK_EQUAL(89, kw[26]);
        }

        {
            BOOST_CHECK_MESSAGE(initFile.hasKey("SWATINIT"),
                                R"(INIT file must have "SWATINIT" array)");

            const auto& kw = initFile.get<float>("SWATINIT");

            auto offset = std::size_t{0};
            for (auto i = 0; i < 9; ++i) {
                BOOST_CHECK_CLOSE(kw[offset + i], 0.1f, 1.0e-8);
            }

            offset += 9;
            for (auto i = 0; i < 9; ++i) {
                BOOST_CHECK_CLOSE(kw[offset + i], 0.5f, 1.0e-8);
            }

            offset += 9;
            for (auto i = 0; i < 9; ++i) {
                BOOST_CHECK_CLOSE(kw[offset + i], 1.0f, 1.0e-8);
            }
        }

        std::streampos file_size = 0;
        {
            std::ifstream file("FOO.UNRST", std::ios::binary);

            file_size = file.tellg();
            file.seekg(0, std::ios::end);
            file_size = file.tellg() - file_size;
        }

        return file_size;
    };

    // Write the file and calculate the file size. FOO.UNRST should be
    // overwitten for every time step, i.e. the file size should not change
    // between runs.  This is to verify that UNRST files are properly
    // overwritten, which they used not to.
    //
    //  * https://github.com/OPM/opm-simulators/issues/753
    //  * https://github.com/OPM/opm-output/pull/61

    WorkArea work_area("test_ecl_writer");
    const auto file_size = write_and_check();

    for (int i = 0; i < 3; ++i) {
        BOOST_CHECK_EQUAL(file_size, write_and_check());
    }

    // Check that "restarting" and writing over previous timesteps does not
    // change the file size, if the total amount of steps are the same.
    BOOST_CHECK_EQUAL(file_size, write_and_check(3, 5));

    // Verify that adding steps from restart also increases file size
    BOOST_CHECK(file_size < write_and_check(3, 7));

    // Verify that restarting a simulation, then writing fewer steps truncates
    // the file
    BOOST_CHECK_EQUAL(file_size, write_and_check(3, 5));
}

namespace {

std::pair<std::string,std::array<std::array<std::vector<float>,2>,3>>
createMULTXYZDECK(std::array<std::bitset<2>,3> doxyz, bool write_all_multminus)
{
     auto deckString = std::string { R"(RUNSPEC
DIMENS
 3 2 3  /
)"};
     if (write_all_multminus)
         deckString += std::string { R"(GRIDOPTS
YES /
)"};
     deckString += std::string { R"(
OIL
WATER
GAS
DISGAS
VAPOIL

METRIC

START
 01  'NOV' 2018 /

FAULTDIM
 10 /         -- max. number os fault segments


UNIFIN
UNIFOUT

GRID
NEWTRAN

GRIDFILE
0  1 /

INIT

SPECGRID
 3 2 3 1 F /

COORD
  2000.0000  2000.0000  2000.0000   2000.0000  2000.0000  2009.0000
  2100.0000  2000.0000  2000.0000   2100.0000  2000.0000  2009.0000
  2200.0000  2000.0000  2000.0000   2200.0000  2000.0000  2009.0000
  2300.0000  2000.0000  2000.0000   2300.0000  2000.0000  2009.0000
  2000.0000  2100.0000  2000.0000   2000.0000  2100.0000  2009.0000
  2100.0000  2100.0000  2000.0000   2100.0000  2100.0000  2009.0000
  2200.0000  2100.0000  2000.0000   2200.0000  2100.0000  2009.0000
  2300.0000  2100.0000  2000.0000   2300.0000  2100.0000  2009.0000
  2000.0000  2200.0000  2000.0000   2000.0000  2200.0000  2009.0000
  2100.0000  2200.0000  2000.0000   2100.0000  2200.0000  2009.0000
  2200.0000  2200.0000  2000.0000   2200.0000  2200.0000  2009.0000
  2300.0000  2200.0000  2000.0000   2300.0000  2200.0000  2009.0000
/

ZCORN
  2000.0000  2000.0000  2000.0000  2000.0000  2000.0000  2000.0000
  2000.0000  2000.0000  2000.0000  2000.0000  2000.0000  2000.0000
  2000.0000  2000.0000  2000.0000  2000.0000  2000.0000  2000.0000
  2000.0000  2000.0000  2000.0000  2000.0000  2000.0000  2000.0000
  2002.5000  2002.5000  2002.5000  2002.5000  2002.5000  2002.5000
  2002.5000  2002.5000  2002.5000  2002.5000  2002.5000  2002.5000
  2002.5000  2002.5000  2002.5000  2002.5000  2002.5000  2002.5000
  2002.5000  2002.5000  2002.5000  2002.5000  2002.5000  2002.5000
  2002.5000  2002.5000  2002.5000  2002.5000  2002.5000  2002.5000
  2002.5000  2002.5000  2002.5000  2002.5000  2002.5000  2002.5000
  2002.5000  2002.5000  2002.5000  2002.5000  2002.5000  2002.5000
  2002.5000  2002.5000  2002.5000  2002.5000  2002.5000  2002.5000
  2005.5000  2005.5000  2005.5000  2005.5000  2005.5000  2005.5000
  2005.5000  2005.5000  2005.5000  2005.5000  2005.5000  2005.5000
  2005.5000  2005.5000  2005.5000  2005.5000  2005.5000  2005.5000
  2005.5000  2005.5000  2005.5000  2005.5000  2005.5000  2005.5000
  2005.5000  2005.5000  2005.5000  2005.5000  2005.5000  2005.5000
  2005.5000  2005.5000  2005.5000  2005.5000  2005.5000  2005.5000
  2005.5000  2005.5000  2005.5000  2005.5000  2005.5000  2005.5000
  2005.5000  2005.5000  2005.5000  2005.5000  2005.5000  2005.5000
  2009.0000  2009.0000  2009.0000  2009.0000  2009.0000  2009.0000
  2009.0000  2009.0000  2009.0000  2009.0000  2009.0000  2009.0000
  2009.0000  2009.0000  2009.0000  2009.0000  2009.0000  2009.0000
  2009.0000  2009.0000  2009.0000  2009.0000  2009.0000  2009.0000
/

NTG
 18*0.9 /

PORO
 18*0.25 /

PERMX
 18*100.0 /

PERMZ
 18*10.0 /

COPY
 PERMX PERMY /
/
)"};

     std::array<std::array<std::vector<float>, 2>, 3> exspected_multipliers;

     for(auto&& pair: exspected_multipliers)
         for(auto&& entry: pair)
             entry.resize(18, 1.);

     if (doxyz[0].test(0)) {
         deckString += std::string{ R"(MULTX
 18*0.5 /
)"};
         exspected_multipliers[0][0].assign(18, .5);
     }
     if (doxyz[0].test(1)) {
         deckString += std::string{ R"(MULTX-
 18*2.0 /
)"};
         exspected_multipliers[0][1].assign(18, 2.);
     }
     if (doxyz[1].test(0)) {
         deckString += std::string{ R"(MULTY
 18*0.1435 /
)"};
         exspected_multipliers[1][0].assign(18, .1435);
     }
     if (doxyz[1].test(0)) {
         deckString += std::string{ R"(MULTY-
 18*2.1435 /
)"};
         exspected_multipliers[1][1].assign(18, 2.1435);
     }
     if (doxyz[2].test(0)) {
         deckString += std::string{ R"(MULTZ
 18*0.34325 /
)"};
         exspected_multipliers[2][0].assign(18, 0.34325);
     }

     if (doxyz[2].test(1)) {
         deckString += std::string{ R"(MULTZ-
 18*0.554325 /
)"};
         exspected_multipliers[2][1].assign(18, 0.554325);
     }
     if(doxyz[0].any() || doxyz[1].any() || doxyz[2].any())
     {
         deckString += std::string{ R"(EQUALS
)"};
         if (doxyz[0].test(0)) {
             deckString += std::string{ R"('MULTX' 0.87794567  1 1 1 1 1 1 /
)"};
             exspected_multipliers[0][0][0] = 0.87794567;
         }
         if (doxyz[0].test(1)) {
             deckString += std::string{ R"('MULTX-' 0.7447794567  2 2 1 1 1 1 /
)"};
             exspected_multipliers[0][1][1] = 0.7447794567;
         }
         if (doxyz[1].test(0)) {
             deckString += std::string{ R"('MULTY' 0.94567  3 3 1 1 1 1 /
)"};
             exspected_multipliers[1][0][2] = 0.94567;
         }
         if (doxyz[1].test(0)) {
             deckString += std::string{ R"('MULTY-' 0.6647794567  1 1  2 2 1 1 /
)"};
             exspected_multipliers[1][1][3] = 0.6647794567;
         }
         if (doxyz[2].test(0)) {
             deckString += std::string{ R"('MULTZ' 0.094567  2 2 2 2 1 1 /
)"};
             exspected_multipliers[2][0][4] = 0.094567;
         }
         if (doxyz[2].test(1)) {
             deckString += std::string{ R"('MULTZ-' 0.089567  3 3 2 2 1 1 /
)"};
             exspected_multipliers[2][1][5] = 0.089567;
         }

         deckString += std::string{ R"(/
)"};
     }
     std::vector<double> edit_mult_x(18, 1);
     std::vector<double> edit_mult_z(18, 1);
     if (doxyz[0].test(0) || doxyz[2].test(1))
     {
         // add an EDIT section
         deckString += std::string{ R"(
EDIT
)"};

         if (doxyz[0].test(0))
         {
             deckString += std::string{ R"(
MULTX
 18*100 /
MULTX
 18*0.1 /
)"};
             edit_mult_x.assign(18, .1);
         }

         if (doxyz[2].test(1))
         {
             deckString += std::string{ R"(
MULTZ-
 18*30 /
MULTZ-
 18*0.3 /
)"};
             edit_mult_z.assign(18, .3);
         }
         deckString += std::string{ R"(
EQUALS
)"};
         if (doxyz[0].test(0))
         {
             deckString += std::string{ R"(
 'MULTX' 1.5 3 3 2 2 1 1 / -- Should not overwrite the value of GRID section but of EDIT section
)"};
             edit_mult_x[5] = 1.5;
         }

         if (doxyz[2].test(1))
         {
             deckString += std::string{ R"(
 'MULTZ-' 3.0 1 1  2 2 1 1 /
)"};
             edit_mult_z[3] = 3.0;
         }

             deckString += std::string{ R"(/
)"};
         auto mult = edit_mult_x.begin();
         if (doxyz[0].test(0)) {
             std::ranges::transform(exspected_multipliers[0][0],
                                    exspected_multipliers[0][0].begin(),
                                    [&mult](const auto& val) { return val * (*mult++); });
         }
         mult = edit_mult_z.begin();
         if (doxyz[2].test(1)) {
             std::ranges::transform(exspected_multipliers[2][1],
                                    exspected_multipliers[2][1].begin(),
                                    [&mult](const auto& val) { return val * (*mult++); });
         }
     }

     return {deckString, exspected_multipliers};
}

/// \brief Test the MULTXYZ writing
///
/// \param doxyz Inidicates for each direction which multipliers should be test
///              no bit set means none, 1st bit set positive, 2nd bit set negative
/// \param write_all_mult_minus If true we will request that even defaulted MULT?- arrays will be
///                  written via first record of GRIDOPTS
void testMultxyz(std::array<std::bitset<2>,3> doxyz, bool write_all_multminus = false)
{
    const auto [deckString, exspectedMult] = createMULTXYZDECK(doxyz, write_all_multminus);
    const auto deck = Parser().parseString(deckString);
    auto es = EclipseState( deck );
    const auto& eclGrid = es.getInputGrid();
    const Schedule schedule(deck, es, std::make_shared<Python>());
    const SummaryConfig summary_config( deck, schedule, es.fieldProps(), es.aquifer());
    const SummaryState st(TimeService::now(), 0.0);
    es.getIOConfig().setBaseName( "MULTXFOO" );
    EclipseIO eclWriter( es, eclGrid , schedule, summary_config);
    eclWriter.writeInitial( );

    EclIO::EclFile initFile { "MULTXFOO.INIT" };

    std::array<std::string, 6> multipliers{"MULTX", "MULTX-", "MULTY", "MULTY-", "MULTZ", "MULTZ-"};
    int i=0;
    for (const auto& mult: multipliers) {
        if (i%2==0 || write_all_multminus || doxyz[i/2].test(1)) {
            BOOST_CHECK_MESSAGE( initFile.hasKey(mult), R"(INIT file must have ")" + mult + R"(" array)" );
            const auto& multValues   = initFile.get<float>(mult);
            auto exspect = exspectedMult[i/2][i%2].begin();
            BOOST_CHECK(multValues.size() == exspectedMult[i/2][i%2].size());
            for (auto multVal = multValues.begin(); multVal != multValues.end(); ++multVal, ++exspect) {
                BOOST_CHECK_CLOSE(*multVal, *exspect, 1e-8);
            }
        }
        ++i;
    }
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(MULTXYZInit)
{
    testMultxyz({ '0', '0' , '0'});
    testMultxyz({ '0', '0' , '1'}, true);
    testMultxyz({ '0', '0' , '2'});
    testMultxyz({ '0', '0' , '3'});
    testMultxyz({ '1', '0' , '0'});
    testMultxyz({ '1', '0' , '1'});
    testMultxyz({ '1', '0' , '3'});
    testMultxyz({ '2', '0' , '0'}, true);
    testMultxyz({ '1', '1' , '0'});
    testMultxyz({ '3', '3' , '0'});
    testMultxyz({ '3', '3' , '1'});
    testMultxyz({ '3', '3' , '2'}, true);
    testMultxyz({ '3', '3' , '3'});
}

namespace {

std::pair<std::string,std::vector<float>>
createMULTPVDECK(bool edit)
{
    auto deckString = std::string { R"(RUNSPEC

TITLE
   1D OIL WATER



DIMENS
   100 1 1 /

EQLDIMS
/
TABDIMS
  2 1 100 /

OIL
WATER

ENDSCALE
/

METRIC

START
   1 'JAN' 2024 /

WELLDIMS
   3 3 2 2 /

UNIFIN
UNIFOUT

GRID

INIT

DX
  100*1 /
DY
        100*10 /
DZ
  100*1 /

TOPS
  100*2000 /

PORO
  100*0.3 /

MULTPV
  100*5 /  -- overwritten by the next MULTPV keyword

MULTPV
  100*10 /  -- partly overwritten by the next BOX statements

BOX
  69 75 1 1 1 1 /
MULTPV
  2*1 5*1.5 /
ENDBOX

BOX
  76 85 1 1 1 1 /
MULTPV
  5*1.5 5*1 /
ENDBOX

EDIT
)"};
    std::vector<float> multpv(68, 10.);
    multpv.insert(multpv.end(), 2, 1);
    multpv.insert(multpv.end(), 10, 1.5);
    multpv.insert(multpv.end(), 5, 1.);
    multpv.insert(multpv.end(), 15, 10.);

    if (edit) {
        deckString += std::string { R"(MULTPV
  75*10 25*10 / -- overwritten by the next
MULTPV
  75*0.8 25*1 /
)"};
        std::transform(multpv.begin(), multpv.begin() + 75, multpv.begin(),
                       [](float f) { return f*.8; });
    }
    deckString += std::string { R"(PROPS

SOLUTION
SCHEDULE
)"};
    return { deckString, multpv };
}

void checkMULTPV(const std::pair<std::string, std::vector<float>>& deckAndValues)
{
    const auto [deckString, expectedMultPV] = deckAndValues;
    const auto deck = Parser().parseString(deckString);
    auto es = EclipseState( deck );
    const auto& eclGrid = es.getInputGrid();
    const Schedule schedule(deck, es, std::make_shared<Python>());
    const SummaryConfig summary_config( deck, schedule, es.fieldProps(), es.aquifer());
    const SummaryState st(TimeService::now(), 0.0);
    es.getIOConfig().setBaseName( "MULTPVFOO" );
    EclipseIO eclWriter( es, eclGrid , schedule, summary_config);
    eclWriter.writeInitial( );

    EclIO::EclFile initFile { "MULTPVFOO.INIT" };
    BOOST_CHECK_MESSAGE( initFile.hasKey("MULTPV"),
                         R"(INIT file must have MULTPV array)" );
    const auto& multPVValues = initFile.get<float>("MULTPV");
    auto expect = expectedMultPV.begin();
    BOOST_CHECK(multPVValues.size() == expectedMultPV.size());

    for (auto multVal = multPVValues.begin(); multVal != multPVValues.end();
         ++multVal, ++expect) {
        BOOST_CHECK_CLOSE(*multVal, *expect, 1e-8);
    }
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(MULTPVInit)
{
    checkMULTPV(createMULTPVDECK(false));
    checkMULTPV(createMULTPVDECK(true));

}

namespace {

std::pair<std::string,std::vector<float>>
createMULTPVBOXDECK()
{
    auto deckString = std::string { R"(RUNSPEC

TITLE
   1D OIL WATER



DIMENS
   100 1 1 /

EQLDIMS
/
TABDIMS
  2 1 100 /

OIL
WATER

ENDSCALE
/

METRIC

START
   1 'JAN' 2024 /

WELLDIMS
   3 3 2 2 /

UNIFIN
UNIFOUT

GRID

INIT

DX
  100*1 /
DY
        100*10 /
DZ
  100*1 /

TOPS
  100*2000 /

PORO
  100*0.3 /

BOX
  11 100 1 1 1 1 /
MULTPV
  90*1.5 /
ENDBOX

EDIT
PROPS
SOLUTION
SCHEDULE
)"};
    std::vector<float> expected(10, 1.);
    expected.insert(expected.end(), 90, 1.5);
    return { deckString, expected };
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(MULTPVBOXInit)
{
    checkMULTPV(createMULTPVBOXDECK());
}
