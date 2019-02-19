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


class EclOutput // 
{

private:

    std::ofstream *ofileH;
    
    const int flipEndianInt(const int &num) const ;
    const float flipEndianFloat(const float &num) const;
    const double flipEndianDouble(const double &num) const;
     
    const std::string trimr(const std::string &str1) const;

    const std::string make_real_string(const float &value) const;
    const std::string make_doub_string(const double &value) const;

    std::tuple<const int, const int> block_size_data_binary(Opm::ecl::eclArrType arrType);
    std::tuple<const int, const int, const int> block_size_data_formatted(EIOD::eclArrType arrType);

    
public:

    EclOutput(std::ofstream &inputFileH);

    void writeBinaryHeader(const std::string &arrName,const int &size,const EIOD::eclArrType &arrType);
   // void writeBinaryHeader(const std::string &arrName,const int &size,const std::string arrType);

    template <typename T>
    void writeBinaryArray(const std::vector<T> &data);
    
    void writeBinaryCharArray(const std::vector<std::string> &data);

    void writeFormattedHeader(const std::string &arrName,const int &size,const EIOD::eclArrType &arrType);

    template <typename T>
    void writeFormattedArray(const std::vector<T> &data);

    void writeFormattedCharArray(const std::vector<std::string> &data);
    
};
