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

#ifndef EGRID_HPP
#define EGRID_HPP

#include "EclFile.hpp"

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <ctime>
#include <map>



class EGrid : public EclFile
{

private:
    int nI,nJ,nK,nNNC,nactive;

    std::vector<int> act_index;
    std::vector<int> glob_index;
    std::vector<float> coord_array;
    std::vector<float> zcorn_array;

public:

    int global_index(const int &i,const int &j,const int &k) const;
    int active_index(const int &i,const int &j,const int &k) const;


    void dimension(int &i,int &j,int &k);

    void ijk_from_active_index(int actInd, int &i,int &j,int &k);
    void ijk_from_global_index(int globInd, int &i,int &j,int &k);

    void getCellCorner(const int &globindex,std::vector<double> &X, std::vector<double>& Y, std::vector<double>& Z) const;
    void getCellCorner(const int &i, const int &j, const int &k, std::vector<double> &X, std::vector<double>& Y, std::vector<double>& Z) const;

    int activeCells() const {return nactive;};
    int totalNumberOfCells() const {return (nI*nJ*nK);};

    EGrid(const std::string &filename);


};

#endif
