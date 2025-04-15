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

#define BOOST_TEST_MODULE EclipseIO_LGR
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

const std::string deckStringLGR = std::string { R"(RUNSPEC
    TITLE
        SPE1 - CASE 1
    DIMENS
        3 3 1 /
    EQLDIMS
    /
    TABDIMS
    /
    OIL
    GAS
    WATER
    DISGAS
    FIELD
    START
        1 'JAN' 2015 /
    WELLDIMS
        2 1 1 2 /
    UNIFOUT
    GRID
    CARFIN
    'LGR1'  1  1  1  1  1  1  3  3  1 /
    ENDFIN
    CARFIN
    'LGR2'  3  3  3  3  1  1  3  3  1 /
    ENDFIN
    INIT
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
    ECHO
    PROPS
    PVTW
            4017.55 1.038 3.22E-6 0.318 0.0 /
    ROCK
        14.7 3E-6 /
    SWOF
    0.12	0    		 	1	0
    0.18	4.64876033057851E-008	1	0
    0.24	0.000000186		0.997	0
    0.3	4.18388429752066E-007	0.98	0
    0.36	7.43801652892562E-007	0.7	0
    0.42	1.16219008264463E-006	0.35	0
    0.48	1.67355371900826E-006	0.2	0
    0.54	2.27789256198347E-006	0.09	0
    0.6	2.97520661157025E-006	0.021	0
    0.66	3.7654958677686E-006	0.01	0
    0.72	4.64876033057851E-006	0.001	0
    0.78	0.000005625		0.0001	0
    0.84	6.69421487603306E-006	0	0
    0.91	8.05914256198347E-006	0	0
    1	0.00001			0	0 /
    SGOF
    0	0	1	0
    0.001	0	1	0
    0.02	0	0.997	0
    0.05	0.005	0.980	0
    0.12	0.025	0.700	0
    0.2	0.075	0.350	0
    0.25	0.125	0.200	0
    0.3	0.190	0.090	0
    0.4	0.410	0.021	0
    0.45	0.60	0.010	0
    0.5	0.72	0.001	0
    0.6	0.87	0.0001	0
    0.7	0.94	0.000	0
    0.85	0.98	0.000	0 
    0.88	0.984	0.000	0 /
    DENSITY
                53.66 64.49 0.0533 /
    PVDG
    14.700	166.666	0.008000
    264.70	12.0930	0.009600
    514.70	6.27400	0.011200
    1014.7	3.19700	0.014000
    2014.7	1.61400	0.018900
    2514.7	1.29400	0.020800
    3014.7	1.08000	0.022800
    4014.7	0.81100	0.026800
    5014.7	0.64900	0.030900
    9014.7	0.38600	0.047000 /
    PVTO
    0.0010	14.7	1.0620	1.0400 /
    0.0905	264.7	1.1500	0.9750 /
    0.1800	514.7	1.2070	0.9100 /
    0.3710	1014.7	1.2950	0.8300 /
    0.6360	2014.7	1.4350	0.6950 /
    0.7750	2514.7	1.5000	0.6410 /
    0.9300	3014.7	1.5650	0.5940 /
    1.2700	4014.7	1.6950	0.5100 
        9014.7	1.5790	0.7400 /
    1.6180	5014.7	1.8270	0.4490 
        9014.7	1.7370	0.6310 /	
    /
    SOLUTION
    EQUIL
        8400 4800 8450 0 8300 0 1 0 0 /
    RSVD
    8300 1.270
    8450 1.270 /
    SUMMARY
    FOPR
    WGOR
        'PROD'
    /
    FGOR
    BPR
    1  1  1 /
    10 10 3 /
    /
    BGSAT
    1  1  1 /
    1  1  2 /
    1  1  3 /
    10 1  1 /
    10 1  2 /
    10 1  3 /
    10 10 1 /
    10 10 2 /
    10 10 3 /
    /
    WBHP
        'INJ'
        'PROD'
    /
    WGIR
        'INJ'
        'PROD'
    /
    WGIT
        'INJ'
        'PROD'
    /
    WGPR
        'INJ'
        'PROD'
    /
    WGPT
        'INJ'
        'PROD'
    /
    WOIR
        'INJ'
        'PROD'
    /
    WOIT
        'INJ'
        'PROD'
    /
    WOPR
        'INJ'
        'PROD'
    /
    WOPT
        'INJ'
        'PROD'
    /
    WWIR
        'INJ'
        'PROD'
    /
    WWIT
        'INJ'
        'PROD'
    /
    WWPR
        'INJ'
        'PROD'
    /
    WWPT
        'INJ'
        'PROD'
    /
    SCHEDULE
    RPTSCHED
        'PRES' 'SGAS' 'RS' 'WELLS' /
    RPTRST
        'BASIC=1' /
    DRSDT
        0 /
    WELSPECL
        'PROD'	'G1' 'LGR2'	3	3	8400	'OIL' /
        'INJ'	'G1' 'LGR1'	1	1	8335	'GAS' /
    /
    COMPDATL
        'PROD' 'LGR2'	3	3	1	1	'OPEN'	1*	1*	0.5 /
        'INJ'  'LGR1'   1	1	1	1	'OPEN'	1*	1*	0.5 /
    /
    WCONPROD
        'PROD' 'OPEN' 'ORAT' 20000 4* 1000 /
    /
    WCONINJE
        'INJ'	'GAS'	'OPEN'	'RATE'	100000 1* 9014 /
    /
    TSTEP
    31 28 31 30 31 30 31 31 30 31 30 31 
    /
    )" };
    



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

        std::transform(permx.begin(), permx.end(), permx.begin(),
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


} // Anonymous namespace

BOOST_AUTO_TEST_CASE(EclipseIOLGR_INIT)
{
    const std::string& deckString = deckStringLGR;

    auto write_and_check = []( ) {
        // preparing tested objects
        const auto deck = Parser().parseString( deckString);
        auto es = EclipseState( deck );
        const auto& eclGrid = es.getInputGrid();
        const Schedule schedule(deck, es, std::make_shared<Python>());
        const SummaryConfig summary_config( deck, schedule, es.fieldProps(), es.aquifer());
        const SummaryState st(TimeService::now(), 0.0);
        es.getIOConfig().setBaseName( "FOO" );
        // creating writing object
        EclipseIO eclWriter( es, eclGrid , schedule, summary_config);

        // defining test data
        using measure = UnitSystem::measure;
        using TargetType = data::TargetType;
        std::vector<double> tranx(3*3*1);
        std::vector<double> trany(3*3*1);
        std::vector<double> tranz(3*3*1);
        const data::Solution eGridProps {
            { "TRANX", data::CellData { measure::transmissibility, tranx, TargetType::INIT } },
            { "TRANY", data::CellData { measure::transmissibility, trany, TargetType::INIT } },
            { "TRANZ", data::CellData { measure::transmissibility, tranz, TargetType::INIT } },
        };

        std::map<std::string, std::vector<int>> int_data =  {{"STR_ULONGNAME" , {1,1,1,1,1,1,1,1} } };
        std::vector<int> v(27); v[2] = 67; v[26] = 89;
        int_data["STR_V"] = v;

        // writing the initial file
        eclWriter.writeInitial( );

        BOOST_CHECK_THROW(eclWriter.writeInitial(eGridProps, int_data), std::invalid_argument);
        int_data.erase("STR_ULONGNAME");
        eclWriter.writeInitial(eGridProps, int_data);

        checkInitFile(deck, eGridProps);


    };

    WorkArea work_area("test_ecl_writer");
    write_and_check();
    auto index  = 1;
}

