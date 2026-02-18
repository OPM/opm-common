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

#define BOOST_TEST_MODULE LgrOutputTests

#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>

#include <opm/io/eclipse/EGrid.hpp>

#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/Carfin.hpp>
#include <opm/input/eclipse/EclipseState/Grid/LgrCollection.hpp>

#include <opm/input/eclipse/Deck/DeckSection.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

using namespace Opm;

namespace {
  template <typename T>
  void check_vec_close(const T& actual, const T& expected, double tol = 1e-5)
  {
      BOOST_REQUIRE_EQUAL(actual.size(), expected.size());
      for (std::size_t i = 0; i < actual.size(); ++i)
          BOOST_CHECK_CLOSE(actual[i], expected[i], tol);
  }


  auto init_deck(const std::string& deck_string) {
    Opm::Parser parser;
    return parser.parseString(deck_string);
  };


std::tuple<std::vector<double>, std::vector<double>> final_test_data()
{
  std::vector<double> coord = {
        0,            0,       2537.5,            0,            0,       2601.5,
    101.6,            0,       2537.5,        101.6,            0,       2601.5,
    203.2,            0,       2537.5,        203.2,            0,       2601.5,
    304.8,            0,       2537.5,        304.8,            0,       2601.5,
    406.4,            0,       2537.5,        406.4,            0,       2601.5,
      508,            0,       2537.5,          508,            0,       2601.5,
    609.6,            0,       2537.5,        609.6,            0,       2601.5,
        0,         50.8,       2537.5,            0,         50.8,       2601.5,
    101.6,         50.8,       2537.5,        101.6,         50.8,       2601.5,
    203.2,         50.8,       2537.5,        203.2,         50.8,       2601.5,
    304.8,         50.8,       2537.5,        304.8,         50.8,       2601.5,
    406.4,         50.8,       2537.5,        406.4,         50.8,       2601.5,
      508,         50.8,       2537.5,          508,         50.8,       2601.5,
    609.6,         50.8,       2537.5,        609.6,         50.8,       2601.5,
        0,        101.6,       2537.5,            0,        101.6,       2601.5,
    101.6,        101.6,       2537.5,        101.6,        101.6,       2601.5,
    203.2,        101.6,       2537.5,        203.2,        101.6,       2601.5,
    304.8,        101.6,       2537.5,        304.8,        101.6,       2601.5,
    406.4,        101.6,       2537.5,        406.4,        101.6,       2601.5,
      508,        101.6,       2537.5,          508,        101.6,       2601.5,
    609.6,        101.6,       2537.5,        609.6,        101.6,       2601.5,
        0,        152.4,       2537.5,            0,        152.4,       2601.5,
    101.6,        152.4,       2537.5,        101.6,        152.4,       2601.5,
    203.2,        152.4,       2537.5,        203.2,        152.4,       2601.5,
    304.8,        152.4,       2537.5,        304.8,        152.4,       2601.5,
    406.4,        152.4,       2537.5,        406.4,        152.4,       2601.5,
      508,        152.4,       2537.5,          508,        152.4,       2601.5,
    609.6,        152.4,       2537.5,        609.6,        152.4,       2601.5,
        0,        203.2,       2537.5,            0,        203.2,       2601.5,
    101.6,        203.2,       2537.5,        101.6,        203.2,       2601.5,
    203.2,        203.2,       2537.5,        203.2,        203.2,       2601.5,
    304.8,        203.2,       2537.5,        304.8,        203.2,       2601.5,
    406.4,        203.2,       2537.5,        406.4,        203.2,       2601.5,
      508,        203.2,       2537.5,          508,        203.2,       2601.5,
    609.6,        203.2,       2537.5,        609.6,        203.2,       2601.5,
        0,          254,       2537.5,            0,          254,       2601.5,
    101.6,          254,       2537.5,        101.6,          254,       2601.5,
    203.2,          254,       2537.5,        203.2,          254,       2601.5,
    304.8,          254,       2537.5,        304.8,          254,       2601.5,
    406.4,          254,       2537.5,        406.4,          254,       2601.5,
      508,          254,       2537.5,          508,          254,       2601.5,
    609.6,          254,       2537.5,        609.6,          254,       2601.5,
        0,        304.8,       2537.5,            0,        304.8,       2601.5,
    101.6,        304.8,       2537.5,        101.6,        304.8,       2601.5,
    203.2,        304.8,       2537.5,        203.2,        304.8,       2601.5,
    304.8,        304.8,       2537.5,        304.8,        304.8,       2601.5,
    406.4,        304.8,       2537.5,        406.4,        304.8,       2601.5,
      508,        304.8,       2537.5,          508,        304.8,       2601.5,
    609.6,        304.8,       2537.5,        609.6,        304.8,       2601.5
};
std::vector<int> gnint = {144, 288, 288,  288, 288, 288,
                          288, 288, 288, 288, 288, 288,
                         288, 288, 288, 288, 288, 288,
                          288, 288, 144};
std::vector<double> gnfloat = {
  2537.5,  2538.2,    2539,  2539.8,  2540.5,    2542,  2543.6,  2545.1,  2546.6,  2550.4,  2554.2,    2558,
  2561.8,  2566.4,    2571,  2575.6,  2580.1,  2585.5,  2590.8,  2596.1,  2601.5,
};

int sum_gnint = std::accumulate(gnint.begin(), gnint.end(), 0);
std::vector<double> zcorn;
zcorn.reserve(sum_gnint);
for (size_t i = 0; i < gnint.size(); ++i) {
    for (size_t j = 0; j < static_cast<size_t>(gnint[i]); ++j) {
        zcorn.push_back(gnfloat[i]);
    }
}
  return std::make_tuple(coord, zcorn);
}


std::array<double,3> dimensions_collumn_lgr(){
    const double i = 500;
    const double j = 500;
    const double k = 20;
    return {i, j, k};
}

std::tuple<double,double, double, double> solution_nestedLGR(){
    const double depth_lgr1 = 8335;
    const double depth_lgr2 = 8335;
    const double vol_lgr1 = 2222222.222222222;
    const double vol_lgr2 = 246913.58024691296;
    return std::make_tuple(depth_lgr1, vol_lgr1, depth_lgr2, vol_lgr2);
}


LgrCollection read_lgr(const std::string& deck_string,
                       const std::size_t nx,
                       const std::size_t ny,
                       const std::size_t nz)
{
    Opm::Parser parser;
    Opm::EclipseGrid eclipse_grid(GridDims(nx,ny,nz));
    Opm::Deck deck = parser.parseString(deck_string);
    const Opm::GRIDSection gridSection ( deck );
    return LgrCollection(gridSection, eclipse_grid);
}

std::pair<std::vector<double>, std::vector<double>>
read_cpg_from_egrid(const std::string& file_path,
                    const std::string& lgr_label)
{
    Opm::EclIO::EGrid egrid_global(file_path, lgr_label);
    egrid_global.load_grid_data();

    const auto& global_coord = egrid_global.get_coord();
    const auto& global_zcorn = egrid_global.get_zcorn();

    return {
        std::piecewise_construct,
        std::forward_as_tuple(global_coord.begin(), global_coord.end()),
        std::forward_as_tuple(global_zcorn.begin(), global_zcorn.end())
    };
}

} // Anonymous namespace

BOOST_AUTO_TEST_CASE(TestLgrOutputBasicLGR) {
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

    Opm::UnitSystem units(1);
    std::vector<Opm::NNCdata> vecNNC;
    std::array<int,3> global_grid_dim = {3,3,1};
    std::vector<double> coord_g, zcorn_g, coord_l, zcorn_l, coord_g_opm, zcorn_g_opm, coord_l_opm, zcorn_l_opm;
    // Intialize LgrCollection from string.
    LgrCollection lgr_col = read_lgr(deck_string,global_grid_dim[0],global_grid_dim[1],global_grid_dim[2]);
    //  Read global COORD and ZCORN from reference simulator output.
    std::tie(coord_g, zcorn_g) = read_cpg_from_egrid("CARFIN5.EGRID", "global");
    //  Read LGR CELL COORD and ZCORN from reference simulator output.
    std::tie(coord_l, zcorn_l) = read_cpg_from_egrid("CARFIN5.EGRID", "LGR1");
    //  Eclipse Grid is intialzied with COORD and ZCORN.
    Opm::EclipseGrid eclipse_grid_file(global_grid_dim, coord_g, zcorn_g);
    //  LgrCollection is used to initalize LGR Cells in the Eclipse Grid.
    eclipse_grid_file.init_lgr_cells(lgr_col);
    // LGR COORD and ZCORN is parsed to EclipseGridLGR children cell. (Simulates the process of recieving the LGR refinement.)
    // Intialize host_cell numbering.
    eclipse_grid_file.init_children_host_cells();
    // Save EclipseGrid.
    eclipse_grid_file.save("OPMCARFIN5.EGRID",false,vecNNC,units);
    // Once the new EGRID is saved, another EclipseGrid Object is created for the sake of comparison.
    std::tie(coord_g_opm, zcorn_g_opm)  = read_cpg_from_egrid("OPMCARFIN5.EGRID", "global");
    //  Read LGR CELL COORD and ZCORN from reference simulator output.
    std::tie(coord_l_opm, zcorn_l_opm) = read_cpg_from_egrid("OPMCARFIN5.EGRID", "LGR1");
    //  Eclipse Grid is intialzied with COORD and ZCORN.
    Opm::EclipseGrid eclipse_grid_OPM(global_grid_dim, coord_g_opm, zcorn_g_opm);
//  LgrCollection is used to initalize LGR Cells in the Eclipse Grid.
    eclipse_grid_OPM.init_lgr_cells(lgr_col);
    // LGR COORD and ZCORN is parsed to EclipseGridLGR children cell. (Simulates the process of recieving the LGR refinement.)
    eclipse_grid_OPM.set_lgr_refinement("LGR1",coord_l_opm,zcorn_l_opm);
    // Intialize host_cell numbering.
    eclipse_grid_OPM.init_children_host_cells();
    BOOST_CHECK_EQUAL( coord_g_opm.size() , coord_g.size());
    BOOST_CHECK_EQUAL( zcorn_g_opm.size() , zcorn_g.size());
    BOOST_CHECK_EQUAL( coord_l_opm.size() , coord_l.size());
    BOOST_CHECK_EQUAL( zcorn_l_opm.size() , zcorn_l.size());
    std::size_t index ;
    for (index = 0; index < coord_g.size(); index++) {
      BOOST_CHECK_EQUAL( coord_g_opm[index] , coord_g[index]);
    }
    for (index = 0; index < zcorn_g.size(); index++) {
      BOOST_CHECK_EQUAL( zcorn_g_opm[index] , zcorn_g[index]);
    }
  }

BOOST_AUTO_TEST_CASE(TestLgrOutputColumnLGR) {
    const std::string deck_string = R"(
RUNSPEC

DIMENS
  3 3 1 /

GRID

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR1'  1  1  1  2  1  1  2  4   1/
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

    Opm::UnitSystem units(1);
    std::vector<Opm::NNCdata> vecNNC;
    std::array<int,3> global_grid_dim = {3,3,1};
    std::vector<double> coord_g, zcorn_g, coord_l, zcorn_l, coord_g_opm, zcorn_g_opm, coord_l_opm, zcorn_l_opm;
    // Intialize LgrCollection from string.
    LgrCollection lgr_col = read_lgr(deck_string,global_grid_dim[0],global_grid_dim[1],global_grid_dim[2]);
    //  Read global COORD and ZCORN from reference simulator output.
    std::tie(coord_g, zcorn_g) = read_cpg_from_egrid("CARFIN-COLUMN.EGRID", "global");
    //  Read LGR CELL COORD and ZCORN from reference simulator output.
    std::tie(coord_l, zcorn_l) = read_cpg_from_egrid("CARFIN-COLUMN.EGRID", "LGR1");
    //  Eclipse Grid is intialzied with COORD and ZCORN.
    Opm::EclipseGrid eclipse_grid_file(global_grid_dim, coord_g, zcorn_g);
    //  LgrCollection is used to initalize LGR Cells in the Eclipse Grid.
    eclipse_grid_file.init_lgr_cells(lgr_col);
    // LGR COORD and ZCORN is parsed to EclipseGridLGR children cell. (Simulates the process of recieving the LGR refinement.)
    const auto& lgr1 = eclipse_grid_file.getLGRCell("LGR1");
    // Intialize host_cell numbering.
    eclipse_grid_file.init_children_host_cells();    // Save EclipseGrid.

    const std::array<double,3> dim_lgr1_calculated = lgr1.getCellDimensions(0,0,0);
    const std::array<double,3> dim_lgr1_value = dimensions_collumn_lgr();


    const double tol = 1e-6;
    BOOST_CHECK_CLOSE( dim_lgr1_calculated[0] , dim_lgr1_value[0], tol);
    BOOST_CHECK_CLOSE( dim_lgr1_calculated[1] , dim_lgr1_value[1], tol);
    BOOST_CHECK_CLOSE( dim_lgr1_calculated[2] , dim_lgr1_value[2], tol);

    eclipse_grid_file.save("OPMCARFIN-COLUMN.EGRID",false,vecNNC,units);

    // Once the new EGRID is saved, another EclipseGrid Object is created for the sake of comparison.
    std::tie(coord_g_opm, zcorn_g_opm)  = read_cpg_from_egrid("OPMCARFIN-COLUMN.EGRID", "global");
    //  Read LGR CELL COORD and ZCORN from reference simulator output.
    std::tie(coord_l_opm, zcorn_l_opm) = read_cpg_from_egrid("OPMCARFIN-COLUMN.EGRID", "LGR1");
    //  Eclipse Grid is intialzied with COORD and ZCORN.
    Opm::EclipseGrid eclipse_grid_OPM(global_grid_dim, coord_g_opm, zcorn_g_opm);
//  LgrCollection is used to initalize LGR Cells in the Eclipse Grid.
    eclipse_grid_OPM.init_lgr_cells(lgr_col);
    // LGR COORD and ZCORN is parsed to EclipseGridLGR children cell. (Simulates the process of recieving the LGR refinement.)
    // Intialize host_cell numbering.
    eclipse_grid_OPM.init_children_host_cells();
    BOOST_CHECK_EQUAL( coord_g_opm.size() , coord_g.size());
    BOOST_CHECK_EQUAL( zcorn_g_opm.size() , zcorn_g.size());
    BOOST_CHECK_EQUAL( coord_l_opm.size() , coord_l.size());
    BOOST_CHECK_EQUAL( zcorn_l_opm.size() , zcorn_l.size());
    std::size_t index ;
    for (index = 0; index < coord_g.size(); index++) {
      BOOST_CHECK_EQUAL( coord_g_opm[index] , coord_g[index]);
    }
    for (index = 0; index < zcorn_g.size(); index++) {
      BOOST_CHECK_EQUAL( zcorn_g_opm[index] , zcorn_g[index]);
    }
  }

BOOST_AUTO_TEST_CASE(TestLgrOutputDoubleLGR) {
    const std::string deck_string = R"(
RUNSPEC

DIMENS
  3 3 1 /

GRID

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR1'  2  2  2  2  1  1  2  2  1  /
ENDFIN

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
'LGR2'  2  2  1  1  1  1  2  2  1 /
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
    Opm::UnitSystem units(1);
    std::vector<Opm::NNCdata> vecNNC;
    std::array<int,3> global_grid_dim = {3,3,1};
    std::vector<double> coord_g, zcorn_g, coord_l1, zcorn_l1, coord_l2, zcorn_l2,
                        coord_g_opm, zcorn_g_opm, coord_l1_opm, zcorn_l1_opm, coord_l2_opm, zcorn_l2_opm;
    // Intialize LgrCollection from string.
    LgrCollection lgr_col = read_lgr(deck_string,global_grid_dim[0],global_grid_dim[1],global_grid_dim[2]);
    //  Read global COORD and ZCORN from reference simulator output.
    std::tie(coord_g, zcorn_g) = read_cpg_from_egrid("CARFIN-DOUBLE.EGRID", "global");
    //  Read LGR CELL COORD and ZCORN from reference simulator output.
    std::tie(coord_l1, zcorn_l1) = read_cpg_from_egrid("CARFIN-DOUBLE.EGRID", "LGR1");
    std::tie(coord_l2, zcorn_l2) = read_cpg_from_egrid("CARFIN-DOUBLE.EGRID", "LGR2");

    //  Eclipse Grid is intialzied with COORD and ZCORN.
    Opm::EclipseGrid eclipse_grid_file(global_grid_dim, coord_g, zcorn_g);
    //  LgrCollection is used to initalize LGR Cells in the Eclipse Grid.
    eclipse_grid_file.init_lgr_cells(lgr_col);
    // LGR COORD and ZCORN is parsed to EclipseGridLGR children cell. (Simulates the process of recieving the LGR refinement.)
    // Intialize host_cell numbering.
    eclipse_grid_file.init_children_host_cells();
    // Save EclipseGrid.
    eclipse_grid_file.save("OPMCARFIN-DOUBLE.EGRID",false,vecNNC,units);
    // Once the new EGRID is saved, another EclipseGrid Object is created for the sake of comparison.
    std::tie(coord_g_opm, zcorn_g_opm)  = read_cpg_from_egrid("OPMCARFIN-DOUBLE.EGRID", "global");
    //  Read LGR CELL COORD and ZCORN from reference simulator output.
    std::tie(coord_l1_opm, zcorn_l1_opm) = read_cpg_from_egrid("OPMCARFIN-DOUBLE.EGRID", "LGR1");
    std::tie(coord_l2_opm, zcorn_l2_opm) = read_cpg_from_egrid("OPMCARFIN-DOUBLE.EGRID", "LGR2");
    //  Eclipse Grid is intialzied with COORD and ZCORN.
    Opm::EclipseGrid eclipse_grid_OPM(global_grid_dim, coord_g_opm, zcorn_g_opm);
//  LgrCollection is used to initalize LGR Cells in the Eclipse Grid.
    eclipse_grid_OPM.init_lgr_cells(lgr_col);
    // Intialize host_cell numbering.
    eclipse_grid_OPM.init_children_host_cells();
    BOOST_CHECK_EQUAL( coord_g_opm.size() , coord_g.size());
    BOOST_CHECK_EQUAL( zcorn_g_opm.size() , zcorn_g.size());
    BOOST_CHECK_EQUAL( coord_l1_opm.size() , coord_l1.size());
    BOOST_CHECK_EQUAL( zcorn_l1_opm.size() , zcorn_l1.size());
    BOOST_CHECK_EQUAL( coord_l2_opm.size() , coord_l2.size());
    BOOST_CHECK_EQUAL( zcorn_l2_opm.size() , zcorn_l2.size());
    std::size_t index ;
    for (index = 0; index < coord_g.size(); index++) {
      BOOST_CHECK_EQUAL( coord_g_opm[index] , coord_g[index]);
    }
    for (index = 0; index < zcorn_g.size(); index++) {
      BOOST_CHECK_EQUAL( zcorn_g_opm[index] , zcorn_g[index]);
    }
  }

BOOST_AUTO_TEST_CASE(TestLgrOutputNESTED) {
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
    Opm::UnitSystem units(1);
    std::vector<Opm::NNCdata> vecNNC;
    std::array<int,3> global_grid_dim = {3,3,1};
    std::vector<double> coord_g, zcorn_g, coord_l1, zcorn_l1, coord_l2, zcorn_l2;
    // Intialize LgrCollection from string.
    LgrCollection lgr_col = read_lgr(deck_string,global_grid_dim[0],global_grid_dim[1],global_grid_dim[2]);
    //  Read global COORD and ZCORN from reference simulator output.
    std::tie(coord_g, zcorn_g) = read_cpg_from_egrid("CARFIN-NESTED.EGRID", "global");
    //  Read LGR CELL COORD and ZCORN from reference simulator output.
    std::tie(coord_l1, zcorn_l1) = read_cpg_from_egrid("CARFIN-NESTED.EGRID", "LGR1");
    std::tie(coord_l2, zcorn_l2) = read_cpg_from_egrid("CARFIN-NESTED.EGRID", "LGR2");

    //  Eclipse Grid is intialzied with COORD and ZCORN.
     Opm::EclipseGrid eclipse_grid_file(global_grid_dim, coord_g, zcorn_g);
    //  LgrCollection is used to initalize LGR Cells in the Eclipse Grid.
    eclipse_grid_file.init_lgr_cells(lgr_col);
    // LGR COORD and ZCORN is parsed to EclipseGridLGR children cell. (Simulates the process of recieving the LGR refinement.)

    const auto& lgr1 = eclipse_grid_file.getLGRCell("LGR1");
    const auto& lgr2 = eclipse_grid_file.getLGRCell("LGR2");

    // Intialize host_cell numbering.
    eclipse_grid_file.init_children_host_cells();
    // Save EclipseGrid.
    eclipse_grid_file.save("OPMCARFIN-NESTED.EGRID",false,vecNNC,units);
    const auto [depth_lgr1, vol_lgr1, depth_lgr2, vol_lgr2] = solution_nestedLGR();
    const double tol = 1e-6;

    BOOST_CHECK_CLOSE(depth_lgr1, lgr1.getCellDepth(0), tol );
    BOOST_CHECK_CLOSE(vol_lgr1, lgr1.getCellVolume(0), tol );
    BOOST_CHECK_CLOSE(depth_lgr2, lgr2.getCellDepth(0), tol );
    BOOST_CHECK_CLOSE(vol_lgr2, lgr2.getCellVolume(0), tol );


  }

  BOOST_AUTO_TEST_CASE(Test_lgr_host_cells_logical) {
    const std::string deck_string = R"(
RUNSPEC

DIMENS
  3 1 1 /

GRID

CARFIN
-- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
LGR1  1  3  1  1  1  1  12  1   1 1*  GLOBAL/
ENDFIN


DX
  3*1000 /
DY
	3*1000 /
DZ
	3*20 /

TOPS
	3*8325 /

PORO
  3*0.15 /

PERMX
  3*1 /

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

    Opm::Parser parser;
    Opm::Deck deck = parser.parseString(deck_string);
    Opm::EclipseState state(deck);
    Opm::EclipseGrid eclipse_grid = state.getInputGrid();
    // Intialize host_cell numbering.
    eclipse_grid.init_children_host_cells();
    std::vector<int> host_vec_sol = { 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3};
    std::vector<int> host_vec = eclipse_grid.getLGRCell(0).save_hostnum();
    BOOST_CHECK_EQUAL_COLLECTIONS(host_vec.begin(), host_vec.end(), host_vec_sol.begin(), host_vec_sol.end());

  }


  BOOST_AUTO_TEST_CASE(TestLGRDepth) {
    const std::string deck_string = R"(
RUNSPEC
TITLE
   SPE1 - CASE 1
DIMENS
   10 10 3 /

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
'LGR1'  5  6  5  6  1  3  6  6  9 /
ENDFIN
CARFIN
'LGR2'  7  8  7  8  1  3  6  6  9 /
ENDFIN
INIT
DX
   	300*1000 /
DY
	300*1000 /
DZ
	100*20 100*30 100*50 /
TOPS
	100*8325 /
PORO
   	300*0.3 /
PERMX
	100*500 100*50 100*200 /
PERMY
	100*500 100*50 100*200 /
PERMZ
	100*500 100*50 100*200 /
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
WELSPECS
	'PROD'	'G1'	10	10	8400	'OIL' /
	'INJ'	'G1'	1	1	8335	'GAS' /
/
COMPDAT
	'PROD'	10	10	3	3	'OPEN'	1*	1*	0.5 /
	'INJ'	1	1	1	1	'OPEN'	1*	1*	0.5 /
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
END
)";
    Opm::UnitSystem units(1);
    std::vector<Opm::NNCdata> vecNNC;
    std::array<int,3> global_grid_dim = {10,10,3};
    std::vector<double> coord_g, zcorn_g, coord_l1, zcorn_l1, coord_l2, zcorn_l2;
    // Intialize LgrCollection from string.
    LgrCollection lgr_col = read_lgr(deck_string,global_grid_dim[0],global_grid_dim[1],global_grid_dim[2]);

    //  Eclipse Grid is intialzied with COORD and ZCORN.
     Opm::EclipseGrid eclipse_grid_file(init_deck(deck_string));
    //  LgrCollection is used to initalize LGR Cells in the Eclipse Grid.
    eclipse_grid_file.init_lgr_cells(lgr_col);

    eclipse_grid_file.save("SPECASE1_CARFIN_TEST.EGRID",false,vecNNC,units);
    auto tol = 1e-6;

    // Coarse grid cells
    auto l1_cell = eclipse_grid_file.getCellDims(0,0,0);
    auto l2_cell = eclipse_grid_file.getCellDims(0,0,1);
    auto l3_cell = eclipse_grid_file.getCellDims(0,0,2);

    // LGR grids
    auto lgr1 = eclipse_grid_file.getLGRCell("LGR1");
    auto lgr2 = eclipse_grid_file.getLGRCell("LGR2");

    // --- Level 1 cells in LGR1 ---
    auto lgr1_l1_cell_01 = lgr1.getCellDims(0,0,0);
    auto lgr1_l1_cell_02 = lgr1.getCellDims(0,0,1);
    auto lgr1_l1_cell_03 = lgr1.getCellDims(0,0,2);

    // Check that Z-dimensions are consistent in LGR1 Level 1
    BOOST_CHECK_CLOSE(lgr1_l1_cell_01[2], lgr1_l1_cell_02[2], tol);
    BOOST_CHECK_CLOSE(lgr1_l1_cell_02[2], lgr1_l1_cell_03[2], tol);
    BOOST_CHECK_CLOSE(lgr1_l1_cell_03[2], lgr1_l1_cell_01[2], tol);

    // --- Level 2 cells in LGR1 ---
    auto lgr1_l2_cell_01 = lgr1.getCellDims(0,0,3);
    auto lgr1_l2_cell_02 = lgr1.getCellDims(0,0,4);
    auto lgr1_l2_cell_03 = lgr1.getCellDims(0,0,5);

    // Check that Z-dimensions are consistent in LGR1 Level 2
    BOOST_CHECK_CLOSE(lgr1_l2_cell_01[2], lgr1_l2_cell_02[2], tol);
    BOOST_CHECK_CLOSE(lgr1_l2_cell_02[2], lgr1_l2_cell_03[2], tol);
    BOOST_CHECK_CLOSE(lgr1_l2_cell_03[2], lgr1_l2_cell_01[2], tol);

      // --- Level 3 cells in LGR1 ---
      auto lgr1_l3_cell_01 = lgr1.getCellDims(0,0,6);
      auto lgr1_l3_cell_02 = lgr1.getCellDims(0,0,7);
      auto lgr1_l3_cell_03 = lgr1.getCellDims(0,0,8);

    // Check that Z-dimensions are consistent between LGR1 Level 3
    BOOST_CHECK_CLOSE(lgr1_l3_cell_01[2], lgr1_l3_cell_02[2], tol);
    BOOST_CHECK_CLOSE(lgr1_l3_cell_02[2], lgr1_l3_cell_03[2], tol);
    BOOST_CHECK_CLOSE(lgr1_l3_cell_03[2], lgr1_l3_cell_01[2], tol);

    // CHECKING IF LGR1 and LGR2 LEVEL 1 CELLS ARE THE SAME
    auto lgr2_l1_cell_01 = lgr2.getCellDims(0,0,0);
    auto lgr2_l1_cell_02 = lgr2.getCellDims(0,0,1);
    auto lgr2_l1_cell_03 = lgr2.getCellDims(0,0,2);
    BOOST_CHECK_CLOSE(lgr1_l1_cell_01[2], lgr2_l1_cell_01[2], tol);
    BOOST_CHECK_CLOSE(lgr1_l1_cell_02[2], lgr2_l1_cell_02[2], tol);
    BOOST_CHECK_CLOSE(lgr1_l1_cell_03[2], lgr2_l1_cell_03[2], tol);

    // --- Level 2 cells in LGR2 ---
    auto lgr2_l2_cell_01 = lgr2.getCellDims(0,0,3);
    auto lgr2_l2_cell_02 = lgr2.getCellDims(0,0,4);
    auto lgr2_l2_cell_03 = lgr2.getCellDims(0,0,5);
    BOOST_CHECK_CLOSE(lgr1_l2_cell_01[2], lgr2_l2_cell_01[2], tol);
    BOOST_CHECK_CLOSE(lgr1_l2_cell_02[2], lgr2_l2_cell_02[2], tol);
    BOOST_CHECK_CLOSE(lgr1_l2_cell_03[2], lgr2_l2_cell_03[2], tol);

    // --- Level 3 cells in LGR2 ---
    auto lgr2_l3_cell_01 = lgr2.getCellDims(0,0,6);
    auto lgr2_l3_cell_02 = lgr2.getCellDims(0,0,7);
    auto lgr2_l3_cell_03 = lgr2.getCellDims(0,0,8);
    BOOST_CHECK_CLOSE(lgr1_l3_cell_01[2], lgr2_l3_cell_01[2], tol);
    BOOST_CHECK_CLOSE(lgr1_l3_cell_02[2], lgr2_l3_cell_02[2], tol);
    BOOST_CHECK_CLOSE(lgr1_l3_cell_03[2], lgr2_l3_cell_03[2], tol);

    // compare levels and coarse cell
    BOOST_CHECK_CLOSE(l1_cell[2], lgr1_l1_cell_01[2] + lgr1_l1_cell_02[2] + lgr1_l1_cell_03[2], tol);
    BOOST_CHECK_CLOSE(l2_cell[2], lgr1_l2_cell_01[2] + lgr1_l2_cell_02[2] + lgr1_l2_cell_03[2], tol);
    BOOST_CHECK_CLOSE(l3_cell[2], lgr1_l3_cell_01[2] + lgr1_l3_cell_02[2] + lgr1_l3_cell_03[2], tol);

  }


  BOOST_AUTO_TEST_CASE(TestFinalDepth) {
    const std::string deck_string = R"(
RUNSPEC
TITLE
   SPE1 - CASE 1
DIMENS
   2 2 5 /
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
'LGR1'  1  2  1  1  1  5  6  6  20 /
ENDFIN
INIT
DX
   	20*1000 /
DY
	20*1000 /
DZ
	4*10 4*20 4*50 4*60 4*70 /
TOPS
	4*8325 /
PORO
   	20*0.3 /
PERMX
	20*500/
PERMY
	20*500/
PERMZ
	20*500 /
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
2  2  3  /
/
BGSAT
1  1  1 /
1  1  2 /
1  1  3 /
2  1  1 /
2  1  2 /
2  1  3 /
2  2  1 /
2  2  2 /
2  2  3 /
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
WELSPECS
	'PROD'	'G1'	2	2	8400	'OIL' /
	'INJ'	'G1'	1	2	8335	'GAS' /
/
COMPDAT
	'PROD'	2	2	1	1	'OPEN'	1*	1*	0.5 /
	'INJ'	1	2	1	1	'OPEN'	1*	1*	0.5 /
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
END
)";
    Opm::UnitSystem units(1);
    std::vector<Opm::NNCdata> vecNNC;
    std::array<int,3> global_grid_dim = {2,2,5};
    std::vector<double> coord_g, zcorn_g, coord_l1, zcorn_l1, coord_l2, zcorn_l2;
    // Intialize LgrCollection from string.
    LgrCollection lgr_col = read_lgr(deck_string,global_grid_dim[0],global_grid_dim[1],global_grid_dim[2]);

    //  Eclipse Grid is intialzied with COORD and ZCORN.
     Opm::EclipseGrid eclipse_grid_file(init_deck(deck_string));
    //  LgrCollection is used to initalize LGR Cells in the Eclipse Grid.
    eclipse_grid_file.init_lgr_cells(lgr_col);

    eclipse_grid_file.save("SPECASE1_CARFIN_TEST_SMALL.EGRID",false,vecNNC,units);

    // LGR1 grids
    const auto& lgr1 = eclipse_grid_file.getLGRCell("LGR1");
    const auto lgr1_coord = lgr1.getCOORD();
    const auto lgr1_zcorn = lgr1.getZCORN();

    auto [expected_coord, expected_zcorn] = final_test_data();
    check_vec_close(lgr1_coord, expected_coord, 1e-2);
    check_vec_close(lgr1_zcorn, expected_zcorn, 1e-2);

    const auto l1_cell = eclipse_grid_file.getCellDims(0,0,0);
    const auto lgr1_l1_cell = lgr1.getCellDims(0,0,0);
    BOOST_CHECK_CLOSE(l1_cell[2], 4*lgr1_l1_cell[2], 1e-6);

    const auto l2_cell = eclipse_grid_file.getCellDims(0,0,1);
    const auto lgr1_l2_cell = lgr1.getCellDims(0,0,5);
    BOOST_CHECK_CLOSE(l2_cell[2], 4*lgr1_l2_cell[2], 1e-6);

    const auto l3_cell = eclipse_grid_file.getCellDims(0,0,2);
    const auto lgr1_l3_cell = lgr1.getCellDims(0,0,9);
    BOOST_CHECK_CLOSE(l3_cell[2], 4*lgr1_l3_cell[2], 1e-6);

    const auto l4_cell = eclipse_grid_file.getCellDims(0,0,3);
    const auto lgr1_l4_cell = lgr1.getCellDims(0,0,13);
    BOOST_CHECK_CLOSE(l4_cell[2], 4*lgr1_l4_cell[2], 1e-6);

    const auto l5_cell = eclipse_grid_file.getCellDims(0,0,4);
    const auto lgr1_l5_cell = lgr1.getCellDims(0,0,17);
    BOOST_CHECK_CLOSE(l5_cell[2], 4*lgr1_l5_cell[2], 1e-6);
  }




    BOOST_AUTO_TEST_CASE(TestFinalDepthDeformed) {
    const std::string deck_string = R"(
RUNSPEC
TITLE
   SPE1 - CASE 1
DIMENS
   2 2 5 /
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
'LGR1'  1  2  1  1  1  5  6  6  20 /
ENDFIN
INIT
COORD
0
0
0.0500000000000000
0
0
0.550000000000000
0.450000000000000
0
-0.0153545513261095
0.500000000000000
0
0.500000000000000
1
0
-0.0500000000000000
1
0
0.450000000000000
0
0.500000000000000
-0.0250000000000000
0
0.500000000000000
0.475000000000000
0.450000000000000
0.500000000000000
0.0791509619741481
0.500000000000000
0.500000000000000
0.575000000000000
1
0.500000000000000
-0.125000000000000
1
0.500000000000000
0.375000000000000
0
1
0.0500000000000000
0
1
0.550000000000000
0.450000000000000
1
0.0309979978301326
0.500000000000000
1
0.500000000000000
1
1
-0.0500000000000000
1
1
0.450000000000000
/
ZCORN
0.0500000000000000
-0.0153545513261095
-0.0153545513261095
-0.0500000000000000
-0.0250000000000000
0.0791509619741481
0.0791509619741481
-0.125000000000000
-0.0250000000000000
0.0791509619741481
0.0791509619741481
-0.125000000000000
0.0500000000000000
0.0309979978301326
0.0309979978301326
-0.0500000000000000
0.150000000000000
0.0876149201408511
0.0876149201408511
0.0500000000000000
0.0750000000000000
0.178910398762863
0.178910398762863
-0.0250000000000000
0.0750000000000000
0.178910398762863
0.178910398762863
-0.0250000000000000
0.150000000000000
0.124918403215579
0.124918403215579
0.0500000000000000
0.150000000000000
0.0876149201408511
0.0876149201408511
0.0500000000000000
0.0750000000000000
0.178910398762863
0.178910398762863
-0.0250000000000000
0.0750000000000000
0.178910398762863
0.178910398762863
-0.0250000000000000
0.150000000000000
0.124918403215579
0.124918403215579
0.0500000000000000
0.250000000000000
0.190651817071996
0.190651817071996
0.150000000000000
0.175000000000000
0.278376959470577
0.278376959470577
0.0750000000000000
0.175000000000000
0.278376959470577
0.278376959470577
0.0750000000000000
0.250000000000000
0.218759014259855
0.218759014259855
0.150000000000000
0.250000000000000
0.190651817071996
0.190651817071996
0.150000000000000
0.175000000000000
0.278376959470577
0.278376959470577
0.0750000000000000
0.175000000000000
0.278376959470577
0.278376959470577
0.0750000000000000
0.250000000000000
0.218759014259855
0.218759014259855
0.150000000000000
0.350000000000000
0.293739533459143
0.293739533459143
0.250000000000000
0.275000000000000
0.377548128575052
0.377548128575052
0.175000000000000
0.275000000000000
0.377548128575052
0.377548128575052
0.175000000000000
0.350000000000000
0.312539518493789
0.312539518493789
0.250000000000000
0.350000000000000
0.293739533459143
0.293739533459143
0.250000000000000
0.275000000000000
0.377548128575052
0.377548128575052
0.175000000000000
0.275000000000000
0.377548128575052
0.377548128575052
0.175000000000000
0.350000000000000
0.312539518493789
0.312539518493789
0.250000000000000
0.450000000000000
0.396861248989208
0.396861248989208
0.350000000000000
0.375000000000000
0.476422542586027
0.476422542586027
0.275000000000000
0.375000000000000
0.476422542586027
0.476422542586027
0.275000000000000
0.450000000000000
0.406279826918605
0.406279826918605
0.350000000000000
0.450000000000000
0.396861248989208
0.396861248989208
0.350000000000000
0.375000000000000
0.476422542586027
0.476422542586027
0.275000000000000
0.375000000000000
0.476422542586027
0.476422542586027
0.275000000000000
0.450000000000000
0.406279826918605
0.406279826918605
0.350000000000000
0.550000000000000
0.500000000000000
0.500000000000000
0.450000000000000
0.475000000000000
0.575000000000000
0.575000000000000
0.375000000000000
0.475000000000000
0.575000000000000
0.575000000000000
0.375000000000000
0.550000000000000
0.500000000000000
0.500000000000000
0.450000000000000
/
PORO
   	20*0.3 /
PERMX
	20*500/
PERMY
	20*500/
PERMZ
	20*500 /
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
2  2  3  /
/
BGSAT
1  1  1 /
1  1  2 /
1  1  3 /
2  1  1 /
2  1  2 /
2  1  3 /
2  2  1 /
2  2  2 /
2  2  3 /
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
WELSPECS
	'PROD'	'G1'	2	2	8400	'OIL' /
	'INJ'	'G1'	1	2	8335	'GAS' /
/
COMPDAT
	'PROD'	2	2	1	1	'OPEN'	1*	1*	0.5 /
	'INJ'	1	2	1	1	'OPEN'	1*	1*	0.5 /
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
END
)";
    Opm::UnitSystem units(1);
    std::vector<Opm::NNCdata> vecNNC;
    std::array<int,3> global_grid_dim = {2,2,5};
    std::vector<double> coord_g, zcorn_g, coord_l1, zcorn_l1, coord_l2, zcorn_l2;
    // Intialize LgrCollection from string.
    LgrCollection lgr_col = read_lgr(deck_string,global_grid_dim[0],global_grid_dim[1],global_grid_dim[2]);

    //  Eclipse Grid is intialzied with COORD and ZCORN.
     Opm::EclipseGrid eclipse_grid_file(init_deck(deck_string));
    //  LgrCollection is used to initalize LGR Cells in the Eclipse Grid.
    eclipse_grid_file.init_lgr_cells(lgr_col);

    eclipse_grid_file.save("SPECASE1_CARFIN_TEST_SMALL-LEKKER.EGRID",false,vecNNC,units);

    // LGR1 grids
    const auto& lgr1 = eclipse_grid_file.getLGRCell("LGR1");
    const auto lgr1_coord = lgr1.getCOORD();
    const auto lgr1_zcorn = lgr1.getZCORN();

    auto [expected_coord, expected_zcorn] = final_test_data();
    check_vec_close(lgr1_coord, expected_coord, 1e-2);
    check_vec_close(lgr1_zcorn, expected_zcorn, 1e-2);

    const auto l1_cell = eclipse_grid_file.getCellDims(0,0,0);
    const auto lgr1_l1_cell = lgr1.getCellDims(0,0,0);
    BOOST_CHECK_CLOSE(l1_cell[2], 4*lgr1_l1_cell[2], 1e-6);

    const auto l2_cell = eclipse_grid_file.getCellDims(0,0,1);
    const auto lgr1_l2_cell = lgr1.getCellDims(0,0,5);
    BOOST_CHECK_CLOSE(l2_cell[2], 4*lgr1_l2_cell[2], 1e-6);

    const auto l3_cell = eclipse_grid_file.getCellDims(0,0,2);
    const auto lgr1_l3_cell = lgr1.getCellDims(0,0,9);
    BOOST_CHECK_CLOSE(l3_cell[2], 4*lgr1_l3_cell[2], 1e-6);

    const auto l4_cell = eclipse_grid_file.getCellDims(0,0,3);
    const auto lgr1_l4_cell = lgr1.getCellDims(0,0,13);
    BOOST_CHECK_CLOSE(l4_cell[2], 4*lgr1_l4_cell[2], 1e-6);

    const auto l5_cell = eclipse_grid_file.getCellDims(0,0,4);
    const auto lgr1_l5_cell = lgr1.getCellDims(0,0,17);
    BOOST_CHECK_CLOSE(l5_cell[2], 4*lgr1_l5_cell[2], 1e-6);
  }
