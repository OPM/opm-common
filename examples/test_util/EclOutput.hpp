/*
   Copyright 2019 Statoil ASA.

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

#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>

#include <examples/test_util/data/EclIOdata.hpp>

namespace EIOD = Opm::ecl;


class EclOutput
{
public:
    explicit EclOutput(std::ofstream& inputFileH);

    void writeBinaryHeader(const std::string& arrName, int size, EIOD::eclArrType arrType);

    template <typename T>
    void writeBinaryArray(const std::vector<T>& data);

    void writeBinaryCharArray(const std::vector<std::string>& data);

    void writeFormattedHeader(const std::string& arrName, int size, EIOD::eclArrType arrType);

    template <typename T>
    void writeFormattedArray(const std::vector<T>& data);

    void writeFormattedCharArray(const std::vector<std::string>& data);

private:
    std::ofstream *ofileH;

    std::string make_real_string(float value) const;
    std::string make_doub_string(double value) const;
};
