/*
   Copyright 2019 Equinor ASA.

   This file is part of the Open Porous Media project (OPM).

   OPM is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   OPM is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with OPM.  If not, see <http://www.gnu.org/licenses/>.
   */

#include "EGrid.hpp"

#include <opm/common/ErrorMacros.hpp>
#include <string>
#include <string.h>
#include <sstream>
#include <iterator>
#include <iomanip>
#include <algorithm>
#include <numeric>


EGrid::EGrid(const std::string &filename) : EclFile(filename)
{
   loadData();

   auto gridhead = get<int>("GRIDHEAD");

   nI = gridhead[1];
   nJ = gridhead[2];
   nK = gridhead[3];

   if (hasKey("ACTNUM")) {
       auto actnum = get<int>("ACTNUM");

       nactive = 0;
       for (unsigned int i = 0; i < actnum.size(); i++) {
           if (actnum[i] > 0) {
               act_index.push_back(nactive);
               glob_index.push_back(i);
               nactive++;
           } else {
               act_index.push_back(-1);
           }
        }
   } else {
       int nCells = nI * nJ * nK;
       act_index.resize(nCells);
       glob_index.resize(nCells);
       std::iota(act_index.begin(), act_index.end(), 0);
       std::iota(glob_index.begin(), glob_index.end(), 0);
   }

   coord_array = get<float>("COORD");
   zcorn_array = get<float>("ZCORN");
}


int EGrid::global_index(int i, int j, int k) const
{
    if ((i < 0) || (i >= nI) || (j < 0) || (j >= nJ) || (k < 0) || (k >= nK)) {
        OPM_THROW(std::invalid_argument, "i, j or/and k out of range");
    }

    return i + j*nI + k*nI*nJ;
}


int EGrid::active_index(int i, int j, int k) const
{
    int n = i + j*nI + k*nI*nJ;

    if ((i < 0) || (i >= nI) || (j < 0) || (j >= nJ) || (k < 0) || (k >= nK)) {
        OPM_THROW(std::invalid_argument, "i, j or/and k out of range");
    }

    return act_index[n];
}


void EGrid::ijk_from_active_index(int actInd, int& i, int& j, int& k)
{
    if (actInd < 0 || actInd >= nactive) {
        OPM_THROW(std::invalid_argument, "active index out of range");
    }

    int _glob = glob_index[actInd];

    k = _glob / (nI*nJ);

    int rest = _glob % (nI*nJ);

    j = rest / nI;
    i = rest % nI;
}


void EGrid::ijk_from_global_index(int globInd, int& i, int& j, int& k)
{
    if (globInd < 0 || globInd >= nI*nJ*nK) {
        OPM_THROW(std::invalid_argument, "global index out of range");
    }

    k = globInd / (nI*nJ);

    int rest = globInd % (nI*nJ);

    j = rest / nI;
    i = rest % nI;
}


void EGrid::getCellCorner(int i, int j, int k,
                          std::vector<double>& X,
                          std::vector<double>& Y,
                          std::vector<double>& Z) const
{
    std::vector<int> zind;
    std::vector<int> pind;

    if (X.size() < 8 || Y.size() < 8 || Z.size() < 8) {
        OPM_THROW(std::invalid_argument,
                  "In routine cellConrner. X, Y and Z should be a vector of size 8");
    }

    if (i < 0 || i >= nI || j < 0 || j >= nJ || k < 0 || k >= nK) {
        OPM_THROW(std::invalid_argument, "i, j and/or k out of range");
    }

   // calculate indices for grid pillars in COORD arrray
   pind.push_back(j*(nI+1)*6 + i*6);
   pind.push_back(pind[0] + 6);
   pind.push_back(pind[0] + (nI+1)*6);
   pind.push_back(pind[2] + 6);

   // get depths from zcorn array in ZCORN array
   zind.push_back(k*nI*nJ*8 + j*nI*4 + i*2);
   zind.push_back(zind[0] + 1);
   zind.push_back(zind[0] + nI*2);
   zind.push_back(zind[2] + 1);

   for (int n = 0; n < 4; n++) {
       zind.push_back(zind[n] + nI*nJ*4);
   }

   for (int n = 0; n< 8; n++){
       Z[n] = zcorn_array[zind[n]];
   }

   for (int  n = 0; n < 4; n++) {
       double xt = coord_array[pind[n]];
       double yt = coord_array[pind[n] + 1];
       double zt = coord_array[pind[n] + 2];

       double xb = coord_array[pind[n] + 3];
       double yb = coord_array[pind[n] + 4];
       double zb = coord_array[pind[n]+5];

       X[n] = xt + (xb-xt) / (zt-zb) * (zt - Z[n]);
       X[n+4] = xt + (xb-xt) / (zt-zb) * (zt-Z[n+4]);

       Y[n] = yt+(yb-yt)/(zt-zb)*(zt-Z[n]);
       Y[n+4] = yt+(yb-yt)/(zt-zb)*(zt-Z[n+4]);
   }
}


void EGrid::getCellCorner(int globindex, std::vector<double>& X,
                          std::vector<double>& Y, std::vector<double>& Z) const
{
    if (globindex < 0 || globindex >= nI*nJ*nK) {
        OPM_THROW(std::invalid_argument, "global index out of range");
    }

    int k = globindex / (nI*nJ);
    int rest = globindex % (nI*nJ);

    int j = rest / nI;
    int i = rest % nI;

    getCellCorner(i, j, k, X, Y, Z);
}
