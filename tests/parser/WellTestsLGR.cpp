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


#define BOOST_TEST_MODULE WellTestLGR
#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Units/Units.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/Python/Python.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>

#define private public
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#undef private

#include <opm/input/eclipse/Schedule/ScheduleTypes.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQActive.hpp>
#include <opm/input/eclipse/Schedule/Well/Connection.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/F.hpp>
#define TOLERANCE_PERCENT 0.01  
using namespace Opm;

std::unordered_map<std::string, std::size_t> create_label_mapper(const EclipseGrid& ecl_grid)
{
    std::unordered_map<std::string, std::size_t> label_to_index;
    std::size_t index = 0;
    for (const std::string& label : ecl_grid.get_all_lgr_labels())
    {
        label_to_index[label] = index;
        index++;
    }
    return label_to_index;
}


BOOST_AUTO_TEST_CASE(WellLGR)
{
    const auto deck = Parser{}.parseString(R"(RUNSPEC
DIMENS
3 3 1 /
GRID
CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR1'  1  1  1  1  1  1  3  3  1 /
ENDFIN
CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
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

SCHEDULE
WELSPECL
-- Item #: 1	 2	3	4	5	 6 7
	'PROD'	'G1' 'LGR2'	3	2	8400	'OIL' /
	'INJ'	'G1' 'LGR1'	1	1	8335	'GAS' /
/
COMPDATL
-- Item #: 1	2	3	4	5	6	7	8	9 10
	'PROD' 'LGR2'	3	1	1	1	'OPEN'	1*	1*	0.5 /
	'INJ'  'LGR1'   1	1	1	1	'OPEN'	1*	1*	0.5 /
/
)");

    auto es    = EclipseState { deck };
    auto& grid = es.getInputGrid();
    auto test1 = grid.get_lgr_children_gridim();
    auto test2 = grid.get_all_lgr_labels();

    const auto sched = Schedule { deck, es };
    
    BOOST_CHECK_EQUAL(sched.getWell("PROD", 0).is_lgr_well(), true);
    BOOST_CHECK_EQUAL(sched.getWell("INJ", 0).is_lgr_well(), true);
    BOOST_CHECK_EQUAL(sched.getWell("PROD", 0).get_lgr_well_tag().value(), "LGR2");
    BOOST_CHECK_EQUAL(sched.getWell("INJ", 0).get_lgr_well_tag().value(), "LGR1");
}



BOOST_AUTO_TEST_CASE(WellLGRDepthHalf)
{
    const auto deck = Parser{}.parseString(R"(RUNSPEC
DIMENS
3 3 1 /
GRID
CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR1'  1  1  1  1  1  1  2  1  2 /
ENDFIN
CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR2'  3  3  3  3  1  1  2  1  2 /
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

SCHEDULE
WELSPECL
-- Item #: 1	 2	3	4	5	 6 7
	'PROD'	'G1' 'LGR2'	2	1	8400	'OIL' /
	'INJ'	'G1' 'LGR1'	2	1	8335	'GAS' /
/
COMPDATL
-- Item #: 1	2	3	4	5	6	7	8	9 10
	'PROD' 'LGR2'	2	1	1	1	'OPEN'	1*	1*	0.5 /
	'INJ'  'LGR1'   2	1	2	2	'OPEN'	1*	1*	0.5 /
/
)");

    auto es    = EclipseState { deck };
    auto& grid = es.getInputGrid();
    auto test1 = grid.get_lgr_children_gridim();
    auto test2 = grid.get_all_lgr_labels();

    const auto sched = Schedule { deck, es };
    
    auto mapper = create_label_mapper(grid);    

    auto cell1 = sched.completed_cells_lgr[mapper["LGR2"]].get(1, 0, 0);
    auto cell2 = sched.completed_cells_lgr[mapper["LGR1"]].get(1, 0, 1);

    auto dim_original =  grid.getCellDimensions(0, 0,0);

    auto dim_cell1 = cell1.dimensions;
    auto dim_cell2 = cell2.dimensions;

    BOOST_CHECK_EQUAL(cell1.depth,8337.5);
    BOOST_CHECK_EQUAL(cell2.depth,8362.5);    
    BOOST_CHECK_EQUAL(dim_original[0]/2,dim_cell1[0]);
    BOOST_CHECK_EQUAL(dim_original[1]/1,dim_cell1[1]);
    BOOST_CHECK_EQUAL(dim_original[2]/2,dim_cell1[2]);
    BOOST_CHECK_EQUAL_COLLECTIONS(dim_cell1.begin(), dim_cell1.end(), dim_cell2.begin(), dim_cell2.end());
}


BOOST_AUTO_TEST_CASE(WellLGRDepthThird)
{
    const auto deck = Parser{}.parseString(R"(RUNSPEC
DIMENS
3 3 1 /
GRID
CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR1'  1  1  1  1  1  1  3  1  3 /
ENDFIN
CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR2'  3  3  3  3  1  1  3  1  3 /
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

SCHEDULE
WELSPECL
-- Item #: 1	 2	3	4	5	 6 7
	'PROD'	'G1' 'LGR2'	2	1	8400	'OIL' /
	'INJ'	'G1' 'LGR1'	2	1	8335	'GAS' /
/
COMPDATL
-- Item #: 1	2	3	4	5	6	7	8	9 10
	'PROD' 'LGR2'	3	1	1	1	'OPEN'	1*	1*	0.5 /
	'INJ'  'LGR1'   3	1	3	3	'OPEN'	1*	1*	0.5 /
/
)");

    auto es    = EclipseState { deck };
    auto& grid = es.getInputGrid();
    auto test1 = grid.get_lgr_children_gridim();
    auto test2 = grid.get_all_lgr_labels();

    const auto sched = Schedule { deck, es };
    
    auto mapper = create_label_mapper(grid);    

    auto cell1 = sched.completed_cells_lgr[mapper["LGR2"]].get(2, 0, 0);
    auto cell2 = sched.completed_cells_lgr[mapper["LGR1"]].get(2, 0, 2);
    auto dim_original =  grid.getCellDimensions(0, 0,0);

    auto dim_cell1 = cell1.dimensions;
    auto dim_cell2 = cell2.dimensions;

    BOOST_CHECK_CLOSE(cell1.depth, 8333.333333, TOLERANCE_PERCENT);
    BOOST_CHECK_CLOSE(cell2.depth,8366.666666, TOLERANCE_PERCENT);    

    BOOST_CHECK_EQUAL(dim_original[0]/3,dim_cell1[0]);
    BOOST_CHECK_EQUAL(dim_original[1]/1,dim_cell1[1]);
    BOOST_CHECK_EQUAL(dim_original[2]/3,dim_cell1[2]);
    BOOST_CHECK_EQUAL_COLLECTIONS(dim_cell1.begin(), dim_cell1.end(), dim_cell2.begin(), dim_cell2.end());
}
