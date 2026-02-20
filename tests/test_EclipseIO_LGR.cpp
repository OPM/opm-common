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
#include <boost/test/tools/old/interface.hpp>
#include <opm/io/eclipse/EGrid.hpp>
#include <opm/io/eclipse/EInit.hpp>
#include <opm/io/eclipse/OutputStream.hpp>
#define BOOST_TEST_MODULE EclipseIO_LGR
#include <boost/test/unit_test.hpp>

#include <opm/output/eclipse/EclipseIO.hpp>
#include <opm/output/eclipse/RestartValue.hpp>

#include <opm/output/data/Cells.hpp>

#include <opm/io/eclipse/EclFile.hpp>
#include <opm/io/eclipse/EGrid.hpp>
#include <opm/io/eclipse/ERst.hpp>

#include <opm/output/eclipse/AggregateAquiferData.hpp>
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
#include <opm/input/eclipse/Utility/Functional.hpp>


#include <opm/common/utility/TimeService.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <algorithm>
#include <config.h>
#include <map>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>


#include <tests/WorkArea.hpp>

using namespace Opm;

namespace {

    template<typename Vec>
    void checkVectorsClose(const Vec& obtained, const Vec& expected, double tol, const std::string& context)
    {
        BOOST_REQUIRE_EQUAL(obtained.size(), expected.size());

        for (std::size_t i = 0; i < obtained.size(); ++i) {
            BOOST_TEST_MESSAGE(context << '[' << i << ']');
            BOOST_CHECK_CLOSE(obtained[i], expected[i], tol);
        }
    }

    std::tuple<std::vector<double>, std::vector< double>,
               std::vector<double>, std::vector< double>,
               std::vector<double>, std::vector< double>>
        simulate_lgr_refinement()
    {
        std::vector<double> coord_lgr1 = {
            0.00000000e+00,    0.00000000e+00,    0.00000000e+00,    0.00000000e+00,    0.00000000e+00,    3.04800000e+02,
            2.37032807e+02,    0.00000000e+00,    0.00000000e+00,    2.37032807e+02,    0.00000000e+00,    3.04800000e+02,
            4.74065620e+02,    0.00000000e+00,    0.00000000e+00,    4.74065620e+02,    0.00000000e+00,    3.04800000e+02,
            7.11098400e+02,    0.00000000e+00,    0.00000000e+00,    7.11098400e+02,    0.00000000e+00,    3.04800000e+02,
            0.00000000e+00,    3.55599980e+02,    0.00000000e+00,    0.00000000e+00,    3.55599980e+02,    3.04800000e+02,
            2.37032807e+02,    3.55599980e+02,    0.00000000e+00,    2.37032807e+02,    3.55599980e+02,    3.04800000e+02,
            4.74065620e+02,    3.55599980e+02,    0.00000000e+00,    4.74065620e+02,    3.55599980e+02,    3.04800000e+02,
            7.11098400e+02,    3.55599980e+02,    0.00000000e+00,    7.11098400e+02,    3.55599980e+02,    3.04800000e+02,
            0.00000000e+00,    7.11199990e+02,    0.00000000e+00,    0.00000000e+00,    7.11199990e+02,    3.04800000e+02,
            2.37032807e+02,    7.11199990e+02,    0.00000000e+00,    2.37032807e+02,    7.11199990e+02,    3.04800000e+02,
            4.74065620e+02,    7.11199990e+02,    0.00000000e+00,    4.74065620e+02,    7.11199990e+02,    3.04800000e+02,
            7.11098400e+02,    7.11199990e+02,    0.00000000e+00,    7.11098400e+02,    7.11199990e+02,    3.04800000e+02,
            0.00000000e+00,    1.06680000e+03,    0.00000000e+00,    0.00000000e+00,    1.06680000e+03,    3.04800000e+02,
            2.37032807e+02,    1.06680000e+03,    0.00000000e+00,    2.37032807e+02,    1.06680000e+03,    3.04800000e+02,
            4.74065620e+02,    1.06680000e+03,    0.00000000e+00,    4.74065620e+02,    1.06680000e+03,    3.04800000e+02,
            7.11098400e+02,    1.06680000e+03,    0.00000000e+00,    7.11098400e+02,    1.06680000e+03,    3.04800000e+02
        };

        std::vector<double> coord_lgr2 = {
            1.42219680e+03,    0.00000000e+00,    0.00000000e+00,    1.42219680e+03,    0.00000000e+00,    3.04800000e+02,
            1.65922955e+03,    0.00000000e+00,    0.00000000e+00,    1.65922955e+03,    0.00000000e+00,    3.04800000e+02,
            1.89626245e+03,    0.00000000e+00,    0.00000000e+00,    1.89626245e+03,    0.00000000e+00,    3.04800000e+02,
            2.13329520e+03,    0.00000000e+00,    0.00000000e+00,    2.13329520e+03,    0.00000000e+00,    3.04800000e+02,
            1.42219680e+03,    3.55599980e+02,    0.00000000e+00,    1.42219680e+03,    3.55599980e+02,    3.04800000e+02,
            1.65922955e+03,    3.55599980e+02,    0.00000000e+00,    1.65922955e+03,    3.55599980e+02,    3.04800000e+02,
            1.89626245e+03,    3.55599980e+02,    0.00000000e+00,    1.89626245e+03,    3.55599980e+02,    3.04800000e+02,
            2.13329520e+03,    3.55599980e+02,    0.00000000e+00,    2.13329520e+03,    3.55599980e+02,    3.04800000e+02,
            1.42219680e+03,    7.11199990e+02,    0.00000000e+00,    1.42219680e+03,    7.11199990e+02,    3.04800000e+02,
            1.65922955e+03,    7.11199990e+02,    0.00000000e+00,    1.65922955e+03,    7.11199990e+02,    3.04800000e+02,
            1.89626245e+03,    7.11199990e+02,    0.00000000e+00,    1.89626245e+03,    7.11199990e+02,    3.04800000e+02,
            2.13329520e+03,    7.11199990e+02,    0.00000000e+00,    2.13329520e+03,    7.11199990e+02,    3.04800000e+02,
            1.42219680e+03,    1.06680000e+03,    0.00000000e+00,    1.42219680e+03,    1.06680000e+03,    3.04800000e+02,
            1.65922955e+03,    1.06680000e+03,    0.00000000e+00,    1.65922955e+03,    1.06680000e+03,    3.04800000e+02,
            1.89626245e+03,    1.06680000e+03,    0.00000000e+00,    1.89626245e+03,    1.06680000e+03,    3.04800000e+02,
            2.13329520e+03,    1.06680000e+03,    0.00000000e+00,    2.13329520e+03,    1.06680000e+03,    3.04800000e+02
        };
        std::vector<double> coord_lgr3 = {
            2.84439360e+03,    0.00000000e+00,    0.00000000e+00,    2.84439360e+03,    0.00000000e+00,    3.04800000e+02,
            3.08142650e+03,    0.00000000e+00,    0.00000000e+00,    3.08142650e+03,    0.00000000e+00,    3.04800000e+02,
            3.31845910e+03,    0.00000000e+00,    0.00000000e+00,    3.31845910e+03,    0.00000000e+00,    3.04800000e+02,
            3.55549200e+03,    0.00000000e+00,    0.00000000e+00,    3.55549200e+03,    0.00000000e+00,    3.04800000e+02,
            2.84439360e+03,    3.55599980e+02,    0.00000000e+00,    2.84439360e+03,    3.55599980e+02,    3.04800000e+02,
            3.08142650e+03,    3.55599980e+02,    0.00000000e+00,    3.08142650e+03,    3.55599980e+02,    3.04800000e+02,
            3.31845910e+03,    3.55599980e+02,    0.00000000e+00,    3.31845910e+03,    3.55599980e+02,    3.04800000e+02,
            3.55549200e+03,    3.55599980e+02,    0.00000000e+00,    3.55549200e+03,    3.55599980e+02,    3.04800000e+02,
            2.84439360e+03,    7.11199990e+02,    0.00000000e+00,    2.84439360e+03,    7.11199990e+02,    3.04800000e+02,
            3.08142650e+03,    7.11199990e+02,    0.00000000e+00,    3.08142650e+03,    7.11199990e+02,    3.04800000e+02,
            3.31845910e+03,    7.11199990e+02,    0.00000000e+00,    3.31845910e+03,    7.11199990e+02,    3.04800000e+02,
            3.55549200e+03,    7.11199990e+02,    0.00000000e+00,    3.55549200e+03,    7.11199990e+02,    3.04800000e+02,
            2.84439360e+03,    1.06680000e+03,    0.00000000e+00,    2.84439360e+03,    1.06680000e+03,    3.04800000e+02,
            3.08142650e+03,    1.06680000e+03,    0.00000000e+00,    3.08142650e+03,    1.06680000e+03,    3.04800000e+02,
            3.31845910e+03,    1.06680000e+03,    0.00000000e+00,    3.31845910e+03,    1.06680000e+03,    3.04800000e+02,
            3.55549200e+03,    1.06680000e+03,    0.00000000e+00,    3.55549200e+03,    1.06680000e+03,    3.04800000e+02
        };
        std::vector<double> zcorn_lgr1 = {
            2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,
            2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,
            2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,
            2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,
            2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,
            2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,
            2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,
            2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,
            2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,
            2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,
            2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,
            2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03
        };
        std::vector<double> zcorn_lgr2 = {
            2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,
            2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,
            2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,
            2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,
            2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,
            2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,
            2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,
            2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,
            2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,
            2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,
            2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,
            2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03
        };
        std::vector<double> zcorn_lgr3 = {
            2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,
            2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,
            2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,
            2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,
            2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,
            2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,    2.53746000e+03,
            2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,
            2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,
            2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,
            2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,
            2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,
            2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03,    2.55270000e+03
        };

        return {coord_lgr1, zcorn_lgr1,
                coord_lgr2, zcorn_lgr2,
                coord_lgr3, zcorn_lgr3};
    }

    Opm::SummaryState sim_stateLGR_example04()
    {
        auto state = Opm::SummaryState{Opm::TimeService::now(), 0.0};

        state.update_group_var("G1", "GOPR" ,    1.0);
        state.update_group_var("G1", "GWPR" ,    2.0);
        state.update_group_var("G1", "GGPR" ,    3.0);
        state.update_group_var("G1", "GVPR" ,    4.0);
        state.update_group_var("G1", "GOPT" ,   10.0);
        state.update_group_var("G1", "GWPT" ,   20.0);
        state.update_group_var("G1", "GGPT" ,   30.0);
        state.update_group_var("G1", "GVPT" ,   40.0);
        state.update_group_var("G1", "GWIR" ,    0.0);
        state.update_group_var("G1", "GGIR" ,    0.0);
        state.update_group_var("G1", "GWIT" ,    0.0);
        state.update_group_var("G1", "GGIT" ,    0.0);
        state.update_group_var("G1", "GVIT" ,    0.0);
        state.update_group_var("G1", "GWCT" ,    0.625);
        state.update_group_var("G1", "GGOR" ,  234.5);
        state.update_group_var("G1", "GBHP" ,  314.15);
        state.update_group_var("G1", "GTHP" ,  123.45);
        state.update_group_var("G1", "GOPTH",  345.6);
        state.update_group_var("G1", "GWPTH",  456.7);
        state.update_group_var("G1", "GGPTH",  567.8);
        state.update_group_var("G1", "GWITH",    0.0);
        state.update_group_var("G1", "GGITH",    0.0);
        state.update_group_var("G1", "GGVIR",    0.0);
        state.update_group_var("G1", "GWVIR",    0.0);
        state.update_group_var("G1", "GOPGR",    4.9);
        state.update_group_var("G1", "GWPGR",    3.8);
        state.update_group_var("G1", "GGPGR",    2.7);
        state.update_group_var("G1", "GVPGR",    6.1);

        state.update_group_var("G5", "GOPR" ,    1.0);
        state.update_group_var("G5", "GWPR" ,    2.0);
        state.update_group_var("G5", "GGPR" ,    3.0);
        state.update_group_var("G5", "GVPR" ,    4.0);
        state.update_group_var("G5", "GOPT" ,   10.0);
        state.update_group_var("G5", "GWPT" ,   20.0);
        state.update_group_var("G5", "GGPT" ,   30.0);
        state.update_group_var("G5", "GVPT" ,   40.0);
        state.update_group_var("G5", "GWIR" ,    0.0);
        state.update_group_var("G5", "GGIR" ,    0.0);
        state.update_group_var("G5", "GWIT" ,    0.0);
        state.update_group_var("G5", "GGIT" ,    0.0);
        state.update_group_var("G5", "GVIT" ,    0.0);
        state.update_group_var("G5", "GWCT" ,    0.625);
        state.update_group_var("G5", "GGOR" ,  234.5);
        state.update_group_var("G5", "GBHP" ,  314.15);
        state.update_group_var("G5", "GTHP" ,  123.45);
        state.update_group_var("G5", "GOPTH",  345.6);
        state.update_group_var("G5", "GWPTH",  456.7);
        state.update_group_var("G5", "GGPTH",  567.8);
        state.update_group_var("G5", "GWITH",    0.0);
        state.update_group_var("G5", "GGITH",    0.0);
        state.update_group_var("G5", "GGVIR",    0.0);
        state.update_group_var("G5", "GWVIR",    0.0);
        state.update_group_var("G5", "GOPGR",    4.9);
        state.update_group_var("G5", "GWPGR",    3.8);
        state.update_group_var("G5", "GGPGR",    2.7);
        state.update_group_var("G5", "GVPGR",    6.1);

        state.update_group_var("G3", "GOPR" ,    1.0);
        state.update_group_var("G3", "GWPR" ,    2.0);
        state.update_group_var("G3", "GGPR" ,    3.0);
        state.update_group_var("G3", "GVPR" ,    4.0);
        state.update_group_var("G3", "GOPT" ,   10.0);
        state.update_group_var("G3", "GWPT" ,   20.0);
        state.update_group_var("G3", "GGPT" ,   30.0);
        state.update_group_var("G3", "GVPT" ,   40.0);
        state.update_group_var("G3", "GWIR" ,    0.0);
        state.update_group_var("G3", "GGIR" ,    0.0);
        state.update_group_var("G3", "GWIT" ,    0.0);
        state.update_group_var("G3", "GGIT" ,    0.0);
        state.update_group_var("G3", "GVIT" ,    0.0);
        state.update_group_var("G3", "GWCT" ,    0.625);
        state.update_group_var("G3", "GGOR" ,  234.5);
        state.update_group_var("G3", "GBHP" ,  314.15);
        state.update_group_var("G3", "GTHP" ,  123.45);
        state.update_group_var("G3", "GOPTH",  345.6);
        state.update_group_var("G3", "GWPTH",  456.7);
        state.update_group_var("G3", "GGPTH",  567.8);
        state.update_group_var("G3", "GWITH",    0.0);
        state.update_group_var("G3", "GGITH",    0.0);
        state.update_group_var("G3", "GGVIR",    0.0);
        state.update_group_var("G3", "GWVIR",    0.0);
        state.update_group_var("G3", "GOPGR",    4.9);
        state.update_group_var("G3", "GWPGR",    3.8);
        state.update_group_var("G3", "GGPGR",    2.7);
        state.update_group_var("G3", "GVPGR",    6.1);

        state.update_group_var("G4", "GOPR" ,    1.0);
        state.update_group_var("G4", "GWPR" ,    2.0);
        state.update_group_var("G4", "GGPR" ,    3.0);
        state.update_group_var("G4", "GVPR" ,    4.0);
        state.update_group_var("G4", "GOPT" ,   10.0);
        state.update_group_var("G4", "GWPT" ,   20.0);
        state.update_group_var("G4", "GGPT" ,   30.0);
        state.update_group_var("G4", "GVPT" ,   40.0);
        state.update_group_var("G4", "GWIR" ,    0.0);
        state.update_group_var("G4", "GGIR" ,    0.0);
        state.update_group_var("G4", "GWIT" ,    0.0);
        state.update_group_var("G4", "GGIT" ,    0.0);
        state.update_group_var("G4", "GVIT" ,    0.0);
        state.update_group_var("G4", "GWCT" ,    0.625);
        state.update_group_var("G4", "GGOR" ,  234.5);
        state.update_group_var("G4", "GBHP" ,  314.15);
        state.update_group_var("G4", "GTHP" ,  123.45);
        state.update_group_var("G4", "GOPTH",  345.6);
        state.update_group_var("G4", "GWPTH",  456.7);
        state.update_group_var("G4", "GGPTH",  567.8);
        state.update_group_var("G4", "GWITH",    0.0);
        state.update_group_var("G4", "GGITH",    0.0);
        state.update_group_var("G4", "GGVIR",    0.0);
        state.update_group_var("G4", "GWVIR",    0.0);
        state.update_group_var("G4", "GOPGR",    4.9);
        state.update_group_var("G4", "GWPGR",    3.8);
        state.update_group_var("G4", "GGPGR",    2.7);
        state.update_group_var("G4", "GVPGR",    6.1);

        state.update_well_var("PROD1", "WOPR" ,    1.0);
        state.update_well_var("PROD1", "WWPR" ,    2.0);
        state.update_well_var("PROD1", "WGPR" ,    3.0);
        state.update_well_var("PROD1", "WVPR" ,    4.0);
        state.update_well_var("PROD1", "WOPT" ,   10.0);
        state.update_well_var("PROD1", "WWPT" ,   20.0);
        state.update_well_var("PROD1", "WGPT" ,   30.0);
        state.update_well_var("PROD1", "WVPT" ,   40.0);
        state.update_well_var("PROD1", "WWIR" ,    0.0);
        state.update_well_var("PROD1", "WGIR" ,    0.0);
        state.update_well_var("PROD1", "WWIT" ,    0.0);
        state.update_well_var("PROD1", "WGIT" ,    0.0);
        state.update_well_var("PROD1", "WVIT" ,    0.0);
        state.update_well_var("PROD1", "WWCT" ,    0.625);
        state.update_well_var("PROD1", "WGOR" ,  234.5);
        state.update_well_var("PROD1", "WBHP" ,  314.15);
        state.update_well_var("PROD1", "WTHP" ,  123.45);
        state.update_well_var("PROD1", "WOPTH",  345.6);
        state.update_well_var("PROD1", "WWPTH",  456.7);
        state.update_well_var("PROD1", "WGPTH",  567.8);
        state.update_well_var("PROD1", "WWITH",    0.0);
        state.update_well_var("PROD1", "WGITH",    0.0);
        state.update_well_var("PROD1", "WGVIR",    0.0);
        state.update_well_var("PROD1", "WWVIR",    0.0);
        state.update_well_var("PROD1", "WOPGR",    4.9);
        state.update_well_var("PROD1", "WWPGR",    3.8);
        state.update_well_var("PROD1", "WGPGR",    2.7);
        state.update_well_var("PROD1", "WVPGR",    6.1);

        state.update_well_var("PROD2", "WOPR" ,    1.0);
        state.update_well_var("PROD2", "WWPR" ,    2.0);
        state.update_well_var("PROD2", "WGPR" ,    3.0);
        state.update_well_var("PROD2", "WVPR" ,    4.0);
        state.update_well_var("PROD2", "WOPT" ,   10.0);
        state.update_well_var("PROD2", "WWPT" ,   20.0);
        state.update_well_var("PROD2", "WGPT" ,   30.0);
        state.update_well_var("PROD2", "WVPT" ,   40.0);
        state.update_well_var("PROD2", "WWIR" ,    0.0);
        state.update_well_var("PROD2", "WGIR" ,    0.0);
        state.update_well_var("PROD2", "WWIT" ,    0.0);
        state.update_well_var("PROD2", "WGIT" ,    0.0);
        state.update_well_var("PROD2", "WVIT" ,    0.0);
        state.update_well_var("PROD2", "WWCT" ,    0.625);
        state.update_well_var("PROD2", "WGOR" ,  234.5);
        state.update_well_var("PROD2", "WBHP" ,  314.15);
        state.update_well_var("PROD2", "WTHP" ,  123.45);
        state.update_well_var("PROD2", "WOPTH",  345.6);
        state.update_well_var("PROD2", "WWPTH",  456.7);
        state.update_well_var("PROD2", "WGPTH",  567.8);
        state.update_well_var("PROD2", "WWITH",    0.0);
        state.update_well_var("PROD2", "WGITH",    0.0);
        state.update_well_var("PROD2", "WGVIR",    0.0);
        state.update_well_var("PROD2", "WWVIR",    0.0);
        state.update_well_var("PROD2", "WOPGR",    4.9);
        state.update_well_var("PROD2", "WWPGR",    3.8);
        state.update_well_var("PROD2", "WGPGR",    2.7);
        state.update_well_var("PROD2", "WVPGR",    6.1);

        state.update_well_var("PROD3", "WOPR" ,    1.0);
        state.update_well_var("PROD3", "WWPR" ,    2.0);
        state.update_well_var("PROD3", "WGPR" ,    3.0);
        state.update_well_var("PROD3", "WVPR" ,    4.0);
        state.update_well_var("PROD3", "WOPT" ,   10.0);
        state.update_well_var("PROD3", "WWPT" ,   20.0);
        state.update_well_var("PROD3", "WGPT" ,   30.0);
        state.update_well_var("PROD3", "WVPT" ,   40.0);
        state.update_well_var("PROD3", "WWIR" ,    0.0);
        state.update_well_var("PROD3", "WGIR" ,    0.0);
        state.update_well_var("PROD3", "WWIT" ,    0.0);
        state.update_well_var("PROD3", "WGIT" ,    0.0);
        state.update_well_var("PROD3", "WVIT" ,    0.0);
        state.update_well_var("PROD3", "WWCT" ,    0.625);
        state.update_well_var("PROD3", "WGOR" ,  234.5);
        state.update_well_var("PROD3", "WBHP" ,  314.15);
        state.update_well_var("PROD3", "WTHP" ,  123.45);
        state.update_well_var("PROD3", "WOPTH",  345.6);
        state.update_well_var("PROD3", "WWPTH",  456.7);
        state.update_well_var("PROD3", "WGPTH",  567.8);
        state.update_well_var("PROD3", "WWITH",    0.0);
        state.update_well_var("PROD3", "WGITH",    0.0);
        state.update_well_var("PROD3", "WGVIR",    0.0);
        state.update_well_var("PROD3", "WWVIR",    0.0);
        state.update_well_var("PROD3", "WOPGR",    4.9);
        state.update_well_var("PROD3", "WWPGR",    3.8);
        state.update_well_var("PROD3", "WGPGR",    2.7);
        state.update_well_var("PROD3", "WVPGR",    6.1);

        state.update_well_var("PROD4", "WOPR" ,    1.0);
        state.update_well_var("PROD4", "WWPR" ,    2.0);
        state.update_well_var("PROD4", "WGPR" ,    3.0);
        state.update_well_var("PROD4", "WVPR" ,    4.0);
        state.update_well_var("PROD4", "WOPT" ,   10.0);
        state.update_well_var("PROD4", "WWPT" ,   20.0);
        state.update_well_var("PROD4", "WGPT" ,   30.0);
        state.update_well_var("PROD4", "WVPT" ,   40.0);
        state.update_well_var("PROD4", "WWIR" ,    0.0);
        state.update_well_var("PROD4", "WGIR" ,    0.0);
        state.update_well_var("PROD4", "WWIT" ,    0.0);
        state.update_well_var("PROD4", "WGIT" ,    0.0);
        state.update_well_var("PROD4", "WVIT" ,    0.0);
        state.update_well_var("PROD4", "WWCT" ,    0.625);
        state.update_well_var("PROD4", "WGOR" ,  234.5);
        state.update_well_var("PROD4", "WBHP" ,  314.15);
        state.update_well_var("PROD4", "WTHP" ,  123.45);
        state.update_well_var("PROD4", "WOPTH",  345.6);
        state.update_well_var("PROD4", "WWPTH",  456.7);
        state.update_well_var("PROD4", "WGPTH",  567.8);
        state.update_well_var("PROD4", "WWITH",    0.0);
        state.update_well_var("PROD4", "WGITH",    0.0);
        state.update_well_var("PROD4", "WGVIR",    0.0);
        state.update_well_var("PROD4", "WWVIR",    0.0);
        state.update_well_var("PROD4", "WOPGR",    4.9);
        state.update_well_var("PROD4", "WWPGR",    3.8);
        state.update_well_var("PROD4", "WGPGR",    2.7);
        state.update_well_var("PROD4", "WVPGR",    6.1);

        state.update_well_var("INJ", "WOPR" ,    0.0);
        state.update_well_var("INJ", "WWPR" ,    0.0);
        state.update_well_var("INJ", "WGPR" ,    0.0);
        state.update_well_var("INJ", "WVPR" ,    0.0);
        state.update_well_var("INJ", "WOPT" ,    0.0);
        state.update_well_var("INJ", "WWPT" ,    0.0);
        state.update_well_var("INJ", "WGPT" ,    0.0);
        state.update_well_var("INJ", "WVPT" ,    0.0);
        state.update_well_var("INJ", "WWIR" ,  100.0);
        state.update_well_var("INJ", "WGIR" ,  200.0);
        state.update_well_var("INJ", "WWIT" , 1000.0);
        state.update_well_var("INJ", "WGIT" , 2000.0);
        state.update_well_var("INJ", "WVIT" , 1234.5);
        state.update_well_var("INJ", "WWCT" ,    0.0);
        state.update_well_var("INJ", "WGOR" ,    0.0);
        state.update_well_var("INJ", "WBHP" ,  400.6);
        state.update_well_var("INJ", "WTHP" ,  234.5);
        state.update_well_var("INJ", "WOPTH",    0.0);
        state.update_well_var("INJ", "WWPTH",    0.0);
        state.update_well_var("INJ", "WGPTH",    0.0);
        state.update_well_var("INJ", "WWITH", 1515.0);
        state.update_well_var("INJ", "WGITH", 3030.0);
        state.update_well_var("INJ", "WGVIR", 1234.0);
        state.update_well_var("INJ", "WWVIR", 4321.0);
        state.update_well_var("INJ", "WOIGR",    4.9);
        state.update_well_var("INJ", "WWIGR",    3.8);
        state.update_well_var("INJ", "WGIGR",    2.7);
        state.update_well_var("INJ", "WVIGR",    6.1);

        return state;
    }



    data::GroupAndNetworkValues mkGroups()
    {
        return {};
    }

    data::Wells mkWellsLGR_Global()
    {
        // This function creates a Wells object with two wells, each having two connections matching the one in the LGR_BASESIM2WELLS.DATA
        data::Rates r1, r2, rc1, rc2, rc3;
        r1.set( data::Rates::opt::wat, 5.67 );
        r1.set( data::Rates::opt::oil, 6.78 );
        r1.set( data::Rates::opt::gas, 7.89 );

        r2.set( data::Rates::opt::wat, 8.90 );
        r2.set( data::Rates::opt::oil, 9.01 );
        r2.set( data::Rates::opt::gas, 10.12 );

        rc1.set( data::Rates::opt::wat, 20.41 );
        rc1.set( data::Rates::opt::oil, 21.19 );
        rc1.set( data::Rates::opt::gas, 22.41 );

        rc2.set( data::Rates::opt::wat, 23.19 );
        rc2.set( data::Rates::opt::oil, 24.41 );
        rc2.set( data::Rates::opt::gas, 25.19 );



        data::Well w1, w2;
        w1.rates = r1;
        w1.thp = 1.0;
        w1.bhp = 1.23;
        w1.temperature = 3.45;
        w1.control = 1;

        /*
         *  the completion keys (active indices) and well names correspond to the
         *  input deck. All other entries in the well structures are arbitrary.
         */
        Opm::data::ConnectionFiltrate con_filtrate {0.1, 1, 3, 0.4, 1.e-9, 0.2, 0.05, 10.}; // values are not used in this test
        w1.connections.push_back( { 2, rc1, 30.45, 123.4, 543.21, 0.62, 0.15, 1.0e3, 1.234, 0.0, 1.23, 1, con_filtrate } );

        w2.rates = r2;
        w2.thp = 2.0;
        w2.bhp = 2.34;
        w2.temperature = 4.56;
        w2.control = 2;
        w2.connections.push_back( { 0, rc2, 36.22, 123.4, 256.1, 0.55, 0.0125, 314.15, 3.456, 0.0, 2.46, 2, con_filtrate } );

        {
            data::Wells wellRates;

            wellRates["PROD"] = w1;
            wellRates["INJ"] = w2;

            return wellRates;
        }
    }

    data::Solution mkSolution(int numCells)
    {
        using measure = UnitSystem::measure;

        auto sol = data::Solution {
            { "PRESSURE", data::CellData { measure::pressure,    {}, data::TargetType::RESTART_SOLUTION } },
            { "TEMP",     data::CellData { measure::temperature, {}, data::TargetType::RESTART_SOLUTION } },
            { "SWAT",     data::CellData { measure::identity,    {}, data::TargetType::RESTART_SOLUTION } },
            { "SGAS",     data::CellData { measure::identity,    {}, data::TargetType::RESTART_SOLUTION } },
        };

        sol.data<double>("PRESSURE").assign( numCells, 6.0 );
        sol.data<double>("TEMP").assign( numCells, 7.0 );
        sol.data<double>("SWAT").assign( numCells, 8.0 );
        sol.data<double>("SGAS").assign( numCells, 9.0 );

        fun::iota rsi( 300.0, 300.0 + numCells );
        fun::iota rvi( 400.0, 400.0 + numCells );

        sol.insert("RS", measure::identity,
                   std::vector<double>{ rsi.begin(), rsi.end() },
                   data::TargetType::RESTART_SOLUTION);
        sol.insert("RV", measure::identity,
                   std::vector<double>{ rvi.begin(), rvi.end() },
                   data::TargetType::RESTART_SOLUTION);

        return sol;
    }

    time_t ecl_util_make_date( const int day, const int month, const int year )
    {
        const auto ymd = Opm::TimeStampUTC::YMD{ year, month, day };
        return static_cast<time_t>(asTimeT(Opm::TimeStampUTC{ymd}));
    }

    Opm::Deck msw_sim(const std::string& fname)
    {
        return Opm::Parser{}.parseFile(fname);
    }

    struct SimulationCase
    {
        explicit SimulationCase(const Opm::Deck& deck)
            : es   { deck }
            , grid { es.getInputGrid() }
            , sched{ deck, es, std::make_shared<Opm::Python>() }
        {}

        // Order requirement: 'es' must be declared/initialised before 'sched'.
        // EclipseState calls routines that initialize EclipseGrid LGR cells.
        Opm::EclipseState es;
        Opm::EclipseGrid  grid;
        Opm::Schedule     sched;
    };


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
        110 120 130
        210 220 230
        310 320 330  /
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

    )" };


template <typename T>
T sum(const std::vector<T>& array)
{
    return std::accumulate(array.begin(), array.end(), T(0));
}

template< typename T, typename U >
void compareSequences(const std::vector< T > &src,
                    const std::vector< U > &dst,
                    double tolerance ) {
    BOOST_REQUIRE_EQUAL(src.size(), dst.size());

    for (auto i = 0*src.size(); i < src.size(); ++i) {
        BOOST_CHECK_CLOSE(src[i], dst[i], tolerance);
    }
}

template< typename T, typename U >
void compareSequencesToScalar(const std::vector< T > &src,
                                                  U  &dst,
                                        double tolerance )
{
    for (auto i = 0*src.size(); i < src.size(); ++i) {
        BOOST_CHECK_CLOSE(src[i], dst, tolerance);
    }
}

void checkInitFile(const Deck& deck,[[maybe_unused]] const data::Solution& simProps)
{
    EclIO::EInit initFile { "FOO.INIT" };
    // tests
    auto lgr_names = initFile.list_of_lgrs();

    if (initFile.hasKey("PORO")) {
        const auto& poro_global   = initFile.getInitData<float>("PORO" );
        const auto& poro_lgr1   = initFile.getInitData<float>("PORO", lgr_names[0]);
        const auto& poro_lgr2   = initFile.getInitData<float>("PORO", lgr_names[1]);
        const auto& expect = deck["PORO"].back().getSIDoubleData();

        compareSequences(expect, poro_global, 1e-4);
        compareSequences(expect, poro_lgr1, 1e-4);
        compareSequences(expect, poro_lgr2, 1e-4);

    }

    if (initFile.hasKey("PERMX")) {
        const auto& expect = deck["PERMX"].back().getSIDoubleData();
        auto        permx  = initFile.getInitData<float>("PERMX");
        auto        permx_lgr1  = initFile.getInitData<float>("PERMX", lgr_names[0]);
        auto        permx_lgr2  = initFile.getInitData<float>("PERMX", lgr_names[1]);

        std::ranges::transform(permx, permx.begin(),
                               [](const auto& kx) { return kx * 9.869233e-16; });
        std::ranges::transform(permx_lgr1, permx_lgr1.begin(),
                               [](const auto& kx) { return kx * 9.869233e-16; });
        std::ranges::transform(permx_lgr2, permx_lgr2.begin(),
                               [](const auto& kx) { return kx * 9.869233e-16; });

        compareSequences(expect, permx, 1e-4);
        compareSequencesToScalar(permx_lgr1, expect[0], 1e-4);
        compareSequencesToScalar(permx_lgr2, expect[8], 1e-4);
    }

    if (initFile.hasKey("LGRHEADQ")) {
        auto        lgrheadq_lgr1  = initFile.getInitData<bool>("LGRHEADQ", lgr_names[0]);
        auto        lgrheadq_lgr2  = initFile.getInitData<bool>("LGRHEADQ", lgr_names[1]);
        std::vector<bool> lgrheadq(5, false);
        BOOST_CHECK_EQUAL_COLLECTIONS(lgrheadq_lgr1.begin(), lgrheadq_lgr1.end(), lgrheadq.begin(), lgrheadq.end());
        BOOST_CHECK_EQUAL_COLLECTIONS(lgrheadq_lgr2.begin(), lgrheadq_lgr2.end(), lgrheadq.begin(), lgrheadq.end());
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
}



BOOST_AUTO_TEST_CASE(EclipseIOLGR_Integration)
{
    double tol = 1e-4;
    WorkArea test_area("test_EclioseIO_LGR");
    test_area.copyIn("LGR_GROUP_EX04.DATA");

    const auto deck = msw_sim("LGR_GROUP_EX04.DATA");
    const auto simCase = SimulationCase{deck};
    auto es = simCase.es;
    const auto& eclGrid = es.getInputGrid();

    {
        auto [coord_lgr1, zcorn_lgr1,
                    coord_lgr2, zcorn_lgr2,
                    coord_lgr3, zcorn_lgr3] = simulate_lgr_refinement();

              es.set_lgr_refinement("LGR1", coord_lgr1, zcorn_lgr1);
              es.set_lgr_refinement("LGR2", coord_lgr2, zcorn_lgr2);
              es.set_lgr_refinement("LGR3", coord_lgr3, zcorn_lgr3);
    }


    const Schedule& schedule = simCase.sched;
    const SummaryConfig summary_config( deck, schedule, es.fieldProps(), es.aquifer());
    const SummaryState st = sim_stateLGR_example04();
    es.getIOConfig().setBaseName( "TESTE_LGR_INTEGRATION" );
    EclipseIO eclWriter( es, eclGrid , schedule, summary_config);

    using measure = UnitSystem::measure;
    using TargetType = data::TargetType;
    const auto start_time = ecl_util_make_date( 10, 10, 2008 );
    std::vector<double> tranx(5*1*1);
    std::vector<double> trany(5*1*1);
    std::vector<double> tranz(5*1*1);
    const data::Solution eGridProps {
        { "TRANX", data::CellData { measure::transmissibility, tranx, TargetType::INIT } },
        { "TRANY", data::CellData { measure::transmissibility, trany, TargetType::INIT } },
        { "TRANZ", data::CellData { measure::transmissibility, tranz, TargetType::INIT } },
    };
    eclWriter.writeInitial(eGridProps, {});

    data::Wells wells;
    data::GroupAndNetworkValues grp_nwrk;
    const auto& lgr_labels = simCase.grid.get_all_lgr_labels();
    auto num_lgr_cells = lgr_labels.size();
    std::vector<int> lgr_grid_ids(num_lgr_cells+1);
    std::iota(lgr_grid_ids.begin(), lgr_grid_ids.end(), 1);
    std::vector <std::size_t> num_cells(num_lgr_cells+1);
    num_cells[0] = simCase.grid.getNumActive();
    std::ranges::transform(lgr_labels, num_cells.begin() + 1 ,
                           [&simCase](const std::string& lgr_tag)
                           { return simCase.grid.getLGRCell(lgr_tag).getNumActive(); });

    std::vector<data::Solution> cells(num_lgr_cells+1);
    std::ranges::transform(num_cells, cells.begin(),
                           [](int n) { return mkSolution(n); });
    data::Wells lwells = mkWellsLGR_Global();
    auto groups = mkGroups();

    auto udqState = UDQState{1};
    auto aquiferData = std::optional<Opm::RestartIO::Helpers::AggregateAquiferData>{std::nullopt};
    Action::State action_state;
    WellTestState wtest_state;
    std::vector<RestartValue> restart_value;
    {
        for (std::size_t i = 0; i < num_lgr_cells + 1; ++i) {
            restart_value.emplace_back(cells[i], lwells, groups, data::Aquifers{}, lgr_grid_ids[i]);
        }

        restart_value[0].solution.data<double>("PRESSURE") = std::vector<double> {0*6894.75729, 1*6894.75729, 2*6894.75729, 3*6894.75729, 4*6894.75729 } ;
        restart_value[1].solution.data<double>("PRESSURE") = std::vector<double> {5*6894.75729, 6*6894.75729, 7*6894.75729,
                                                                                  8*6894.75729, 9*6894.75729, 10*6894.75729,
                                                                                  11*6894.75729, 12*6894.75729, 13*6894.75729};
        restart_value[2].solution.data<double>("PRESSURE") = std::vector<double> {14*6894.75729, 15*6894.75729, 16*6894.75729,
                                                                                  17*6894.75729, 18*6894.75729, 19*6894.75729,
                                                                                  20*6894.75729, 21*6894.75729, 22*6894.75729};
        restart_value[3].solution.data<double>("PRESSURE") = std::vector<double> {23*6894.75729, 24*6894.75729, 25*6894.75729,
                                                                                  26*6894.75729, 27*6894.75729, 28*6894.75729,
                                                                                  29*6894.75729, 30*6894.75729, 31*6894.75729};


        eclWriter.writeTimeStep(action_state,
            wtest_state,
            st,
            udqState,
            1,
            false,
            start_time,
            std::move(restart_value));
    }


    {
        const auto egridFile = ::Opm::EclIO::OutputStream::
                     outputFileName({test_area.currentWorkingDirectory(), "TESTE_LGR_INTEGRATION"}, "EGRID");
        Opm::EclIO::EGrid egrid(egridFile,"LGR3");
        std::vector<std::string> lgr_names   = simCase.grid.get_all_lgr_labels();
        std::vector<std::string> expected_lgr = egrid.list_of_lgrs();

        BOOST_CHECK_EQUAL_COLLECTIONS(
            lgr_names.begin(), lgr_names.end(),
            expected_lgr.begin(), expected_lgr.end()
        );

        egrid.load_grid_data();
        std::vector<float> coord_file = egrid.get_coord();
        std::vector<double> coord_obtained(coord_file.begin(), coord_file.end());
        std::vector<float> zcorn_file  = egrid.get_zcorn();
        std::vector<double> zcorn_obtained(zcorn_file.begin(), zcorn_file.end());
        auto [_, __, ___, ____, coord_expected, zcorn_expected] = simulate_lgr_refinement();
        double K = 1/0.3048;
        std::ranges::transform(coord_expected, coord_expected.begin(),
                               [K](double v) { return v * K; });
        std::ranges::transform(zcorn_expected, zcorn_expected.begin(),
                               [K](double v) { return v * K; });

        checkVectorsClose(coord_obtained, coord_expected, tol, "coord_obtained");
        checkVectorsClose(zcorn_obtained, zcorn_expected, tol, "zcorn_obtained");
    }

    {
        const auto egridFile = ::Opm::EclIO::OutputStream::
                     outputFileName({test_area.currentWorkingDirectory(), "TESTE_LGR_INTEGRATION"}, "INIT");
        Opm::EclIO::EGrid init(egridFile, "LGR2");
        init.loadData("DY");
        std::vector<float> dy_file = init.get<float>("DY");
        std::vector<double> dy_obtained(dy_file.begin(), dy_file.end());
        std::vector<double> dy_expected(9, 0.11666666E+04);
        checkVectorsClose(dy_obtained, dy_expected, 1e-4, "dy_obtained");
    }

    {
        const auto rstFile = ::Opm::EclIO::OutputStream::
                     outputFileName({test_area.currentWorkingDirectory(), "TESTE_LGR_INTEGRATION"}, "UNRST");
        EclIO::ERst rst{ rstFile };
        auto global_pressure = rst.getRestartData<float>("PRESSURE",1);
        auto lgr1_pressure = rst.getRestartData<float>("PRESSURE",1, "LGR1");
        auto lgr2_pressure = rst.getRestartData<float>("PRESSURE",1, "LGR2");
        auto lgr3_pressure = rst.getRestartData<float>("PRESSURE",1, "LGR3");

        checkVectorsClose(global_pressure, std::vector<float> {0.0f, 1.0f, 2.0f, 3.0f, 4.0f}, tol, "global_pressure");
        checkVectorsClose(lgr1_pressure, std::vector<float> {5.0f, 6.0f, 7.0f,
                                                                                8.0f, 9.0f, 10.0f,
                                                                                11.0f, 12.0f, 13.0f,}, tol, "lgr1_pressure");
        checkVectorsClose(lgr2_pressure, std::vector<float> {14.0f, 15.0f, 16.0f,
                                                                                17.0f, 18.0f, 19.0f,
                                                                                20.0f, 21.0f, 22.0f,}, tol, "lgr2_pressure");
        checkVectorsClose(lgr3_pressure, std::vector<float> {23.0f, 24.0f, 25.0f,
                                                                                26.0f, 27.0f, 28.0f,
                                                                                29.0f, 30.0f, 31.0f,}, tol, "lgr3_pressure");
    }
}
