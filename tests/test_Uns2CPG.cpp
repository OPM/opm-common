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

#define BOOST_TEST_MODULE TestUnstoCPG

#include <boost/test/unit_test.hpp>
#include <boost/test/test_tools.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/LgrCollection.hpp>
#include <opm/input/eclipse/EclipseState/Grid/Carfin.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/common/utility/numeric/GridUtil.hpp>



using namespace Opm;

bool are_equal(const std::vector<double>& a, const std::vector<double>& b, double epsilon = 1e-8) {
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(),
                     [epsilon](double x, double y) {
                         return std::abs(x - y) < epsilon;
                     });
}

BOOST_AUTO_TEST_CASE(TestGrid2CPG1) { 
std::size_t nx, ny, nz;
nx = 2;
ny=  1;
nz = 2;   
std::vector<std::array<double, 3>> coord = {
{1.00000000,1.00000000,-0.05000000},
{1.00000000,0.00000000,-0.05000000},
{0.45000000,0.00000000,-0.01535455},
{0.45000000,1.00000000,0.03099800},
{0.00000000,1.00000000,0.05000000},
{0.00000000,0.00000000,0.05000000},
{1.00000000,1.00000000,0.20000000},
{1.00000000,0.00000000,0.20000000},
{0.47498716,0.00000000,0.24219037},
{0.47501669,1.00000000,0.26565554},
{0.00000000,0.00000000,0.30000000},
{0.00000000,1.00000000,0.30000000},
{1.00000000,1.00000000,0.45000000},
{1.00000000,0.00000000,0.45000000},
{0.50000000,0.00000000,0.50000000},
{0.50000000,1.00000000,0.50000000},
{0.00000000,0.00000000,0.55000000},
{0.00000000,1.00000000,0.55000000},
};

std::vector<std::array<std::size_t, 8>> element = {
{5,2,4,3,10,8,11,9},
{2,1,3,0,8,7,9,6},
{10,8,11,9,16,14,17,15},
{8,7,9,6,14,13,15,12},
};

std::vector<double> pCOORDS ={
    0.00000000,0.00000000,0.05000000,0.00000000,0.00000000,
    0.55000000,0.45000000,0.00000000,-0.01535455,0.50000000,
    0.00000000,0.50000000,1.00000000,0.00000000,-0.05000000,
    1.00000000,0.00000000,0.45000000,0.00000000,1.00000000,
    0.05000000,0.00000000,1.00000000,0.55000000,0.45000000,
    1.00000000,0.03099800,0.50000000,1.00000000,0.50000000,
    1.00000000,1.00000000,-0.05000000,1.00000000,1.00000000,
    0.45000000};

std::vector<double> pZCORN = {
0.05000000,-0.01535455,-0.01535455,-0.05000000,0.05000000,
0.03099800,0.03099800,-0.05000000,0.30000000,0.24219037,
0.24219037,0.20000000,0.30000000,0.26565554,0.26565554,
0.20000000,0.30000000,0.24219037,0.24219037,0.20000000,
0.30000000,0.26565554,0.26565554,0.20000000,0.55000000,
0.50000000,0.50000000,0.45000000,0.55000000,0.50000000,
0.50000000,0.45000000
};

auto [COORDS, ZCORN] = 
                        Opm::GridUtil::convertUnsToCPG(coord,element, nx, ny,nz);
BOOST_CHECK(are_equal(pCOORDS, COORDS));
BOOST_CHECK(are_equal(pZCORN, ZCORN));

}
