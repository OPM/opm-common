/*
 Copyright (C) 2023 Equinor
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

#include <optional>
#include <utility>
#define BOOST_TEST_MODULE LgrOutputTests

#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>
#include <opm/input/eclipse/EclipseState/Grid/LgrCollection.hpp>
#include <opm/input/eclipse/EclipseState/Grid/Carfin.hpp>
#include <opm/io/eclipse/EGrid.hpp>
#include <filesystem>

using namespace Opm;

LgrCollection read_lgr(const std::string& deck_string, std::size_t nx, std::size_t ny, std::size_t nz){
    Opm::Parser parser;
    Opm::EclipseGrid eclipse_grid(GridDims(nx,ny,nz));    
    Opm::Deck deck = parser.parseString(deck_string);
    const Opm::GRIDSection gridSection ( deck );
    return LgrCollection(gridSection, eclipse_grid);
}

std::pair<std::vector<double>, std::vector<double>> read_cpg_from_egrid(const std::string& file_path, const std::string& lgr_label){
    Opm::EclIO::EGrid egrid_global(file_path, lgr_label);
    // Opm::EclIO::EGrid egrid_global(file_path);
    egrid_global.load_grid_data();
    // auto global_coord = egrid_global.get<float>("COORD");
    // auto global_zcorn = egrid_global.get<float>("ZCORN");
    auto global_coord = egrid_global.get_coord();
    auto global_zcorn  = egrid_global.get_zcorn();
    std::vector<double> coord_g(global_coord.begin(), global_coord.end());
    std::vector<double> zcorn_g(global_zcorn.begin(), global_zcorn.end());
    return std::make_pair(coord_g, zcorn_g);
}


BOOST_AUTO_TEST_CASE(TestLgrOutput) { 
    const std::string deck_string = R"(
RUNSPEC

DIMENS
  3 3 1 /

GRID

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR1'  2  2  2  2  1  1  3  3   /
ENDFIN

DX 
  9*1000 /
DY
	9*1000 /
DZ
	9*20 /

TOPS
	9*8325 /

PORO
  9*0.15 /

PERMX
  9*1 /

COPY
  PERMX PERMZ /
  PERMX PERMY /
/

EDIT

OIL
GAS

TITLE
Opm-the-best-open-source-simulator

START
16 JUN 1988 /

PROPS

REGIONS

SOLUTION

SCHEDULE
)";
\
    Opm::UnitSystem units(1);
    std::vector<Opm::NNCdata> vecNNC;
    std::array<int,3> global_grid_dim = {3,3,1};
    LgrCollection lgr_col = read_lgr(deck_string,global_grid_dim[0],global_grid_dim[1],global_grid_dim[2]);
    auto [coord_g, zcorn_g] = read_cpg_from_egrid("CARFIN5.EGRID", "global");
    Opm::EclipseGrid eclipse_grid_file(global_grid_dim, coord_g, zcorn_g);    

    eclipse_grid_file.init_lgr_cells(lgr_col);

    auto [coord_l, zcorn_l] = read_cpg_from_egrid("CARFIN5.EGRID", "LGR1");
    eclipse_grid_file.lgr_children_cells[0].set_lgr_refinement(coord_l,zcorn_l);
    eclipse_grid_file.init_children_host_cells();

    //eclipse_grid_file.save("output.FEGRID",true,std::nullopt);

    eclipse_grid_file.save("output.FEGRID",true,vecNNC,units);
    auto index = 1;

    
  //   Opm::EclipseGrid eclipse_grid = state.getInputGrid();



  //   Opm::EclIO::EGrid egrid_global("CARFIN5.EGRID");
  //   egrid_global.load_grid_data();
  //   auto global_coord = egrid_global.get_coord();
  //   auto global_zcorn = egrid_global.get_zcorn();

  //   std::vector<double> coord_g(global_coord.begin(), global_coord.end());
  //   std::vector<double> zcorn_g(global_zcorn.begin(), global_zcorn.end());
  //   std::array<int,3> dims = {3,3, 1};

  //   Opm::EclipseGrid eclipse_grid_file(dims, coord_g, zcorn_g);




  //   Opm::EclIO::EGrid egrid_lgr1("CARFIN5.EGRID","LGR1");
  //   egrid_lgr1.load_grid_data();
  //   BOOST_CHECK_EQUAL( eclipse_grid.getTotalActiveLGR() , 21U );
  //   BOOST_CHECK_EQUAL( eclipse_grid.lgr_children_cells[0].getTotalActiveLGR() , 8U );
  //   BOOST_CHECK_EQUAL( eclipse_grid.lgr_children_cells[1].getTotalActiveLGR() , 8U );
    
  //   BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR1",0,0,0), 0U);
  //   BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR1",1,3,0), 7U);

  //   BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",1,0,0), 8U);
  //   BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",1,1,0), 17U);
  //   BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",2,2,0), 20U);


  //   BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR2",0,0,0), 9U);
  //   BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR2",1,3,0), 16U);
  }





BOOST_AUTO_TEST_CASE(TestLgrNested) { 
    const std::string deck_string = R"(
RUNSPEC

DIMENS
  3 3 1 /

GRID

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
LGR1  2  2  2  2  1  1  3  3   1 1*  GLOBAL/
ENDFIN

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
LGR2  2  2  2  2  1  1  3  3   1 1*  LGR1/
ENDFIN


DX 
  9*1000 /
DY
	9*1000 /
DZ
	9*20 /

TOPS
	9*8325 /

PORO
  9*0.15 /

PERMX
  9*1 /

COPY
  PERMX PERMZ /
  PERMX PERMY /
/

EDIT

OIL
GAS

TITLE
The title

START
16 JUN 1988 /

PROPS

REGIONS

SOLUTION

SCHEDULE
)";
\
    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(deck_string);
    Opm::EclipseState state(deck);
    Opm::EclipseGrid eclipse_grid = state.getInputGrid();

    BOOST_CHECK_EQUAL( eclipse_grid.getTotalActiveLGR() , 25U );
    BOOST_CHECK_EQUAL( eclipse_grid.lgr_children_cells[0].getTotalActiveLGR() , 17U );
    BOOST_CHECK_EQUAL( eclipse_grid.lgr_children_cells[0].lgr_children_cells[0].getTotalActiveLGR() , 9U );
    
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",0,0,0), 0U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",2,2,0), 24U);

    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR1",0,0,0), 4U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR1",2,2,0), 20U);

    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR2",0,0,0), 8U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR2",2,2,0), 16U);

    BOOST_CHECK_THROW(eclipse_grid.getActiveIndexLGR("GLOBAL",1,1,0), std::invalid_argument);
    BOOST_CHECK_THROW(eclipse_grid.getActiveIndexLGR("LGR1",1,1,0), std::invalid_argument);
    BOOST_CHECK_THROW(eclipse_grid.getActiveIndexLGR("LGR3",1,1,0), std::invalid_argument);
}
BOOST_AUTO_TEST_CASE(TestGLOBALinactivecells) { 
    const std::string deck_string = R"(
RUNSPEC

DIMENS
  3 3 1 /

GRID

ACTNUM
1 0 1 
1 1 1 
1 1 1 
/

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR1'  2  2  2  2  1  1  3  3   1/
ENDFIN


DX 
  9*1000 /
DY
	9*1000 /
DZ
	9*20 /

TOPS
	9*8325 /

PORO
  9*0.15 /

PERMX
  9*1 /

COPY
  PERMX PERMZ /
  PERMX PERMY /
/

EDIT

OIL
GAS

TITLE
The title

START
16 JUN 1988 /

PROPS

REGIONS

SOLUTION

SCHEDULE
)";
\
    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(deck_string);
    Opm::EclipseState state(deck);
    Opm::EclipseGrid eclipse_grid = state.getInputGrid();

    BOOST_CHECK_EQUAL( eclipse_grid.getTotalActiveLGR() , 16U );
    BOOST_CHECK_EQUAL( eclipse_grid.lgr_children_cells[0].getTotalActiveLGR() , 9U );    
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",0,0,0), 0U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",2,2,0), 15U);
   
   
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",0U), 0U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("GLOBAL",8U), 15U);

    
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR1",0,0,0), 3U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR1",2,2,0), 11U);
   
   
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR1",0), 3U);
    BOOST_CHECK_EQUAL(eclipse_grid.getActiveIndexLGR("LGR1",8), 11U);

}

BOOST_AUTO_TEST_CASE(TestLGRinactivecells) { 
    const std::string deck_string = R"(
RUNSPEC

DIMENS
  3 3 1 /

GRID

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR1'  2  2  2  2  1  1  3  3   1/
ACTNUM
1 0 1 
1 1 1 
1 1 1 
/
ENDFIN

DX 
  9*1000 /
DY
	9*1000 /
DZ
	9*20 /

TOPS
	9*8325 /

PORO
  9*0.15 /

PERMX
  9*1 /

COPY
  PERMX PERMZ /
  PERMX PERMY /
/

EDIT

OIL
GAS

TITLE
The title

START
16 JUN 1988 /

PROPS

REGIONS

SOLUTION

SCHEDULE
)";
\
    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(deck_string);
    Opm::EclipseState state(deck);
    Opm::LgrCollection lgrs = state.getLgrs();
    // LGR Inactive Cells Not yet Implemented
}
