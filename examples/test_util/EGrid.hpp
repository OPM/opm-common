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

#include <array>
#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <ctime>
#include <map>


class EGrid : public EclFile
{
public:
    explicit EGrid(const std::string& filename);

    int global_index(int i, int j, int k) const;
    int active_index(int i, int j, int k) const;

    const std::array<int, 3>& dimension() const { return nijk; }

    std::array<int, 3> ijk_from_active_index(int actInd) const;
    std::array<int, 3> ijk_from_global_index(int globInd) const;

    void getCellCorners(int globindex, std::vector<double>& X, std::vector<double>& Y, std::vector<double>& Z) const;
    void getCellCorners(const std::array<int, 3>& ijk, std::vector<double>& X, std::vector<double>& Y, std::vector<double>& Z) const;

    int activeCells() const { return nactive; }
    int totalNumberOfCells() const { return nijk[0] * nijk[1] * nijk[2]; }

private:
    std::array<int, 3> nijk;
    int nNNC, nactive;

    std::vector<int> act_index;
    std::vector<int> glob_index;
    std::vector<float> coord_array;
    std::vector<float> zcorn_array;
};

#endif
