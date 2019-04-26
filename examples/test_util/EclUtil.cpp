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

#include "EclUtil.hpp"
#include <opm/common/ErrorMacros.hpp>

#include <algorithm>
#include <stdexcept>


namespace EIOD = Opm::ecl;


int Opm::ecl::flipEndianInt(int num)
{
    unsigned int tmp = __builtin_bswap32(num);
    return static_cast<int>(tmp);
}


float Opm::ecl::flipEndianFloat(float num)
{
    float value = num;

    char* floatToConvert = reinterpret_cast<char*>(&value);
    std::reverse(floatToConvert, floatToConvert+4);

    return value;
}


double Opm::ecl::flipEndianDouble(double num)
{
    double value = num;

    char* doubleToConvert = reinterpret_cast<char*>(&value);
    std::reverse(doubleToConvert, doubleToConvert+8);

    return value;
}


std::tuple<int, int> Opm::ecl::block_size_data_binary(eclArrType arrType)
{
    using BlockSizeTuple = std::tuple<int, int>;

    switch (arrType) {
    case EIOD::INTE:
        return BlockSizeTuple{EIOD::sizeOfInte, EIOD::MaxBlockSizeInte};
        break;
    case EIOD::REAL:
        return BlockSizeTuple{EIOD::sizeOfReal, EIOD::MaxBlockSizeReal};
        break;
    case EIOD::DOUB:
        return BlockSizeTuple{EIOD::sizeOfDoub, EIOD::MaxBlockSizeDoub};
        break;
    case EIOD::LOGI:
        return BlockSizeTuple{EIOD::sizeOfLogi, EIOD::MaxBlockSizeLogi};
        break;
    case EIOD::CHAR:
        return BlockSizeTuple{EIOD::sizeOfChar, EIOD::MaxBlockSizeChar};
        break;
    case EIOD::MESS:
        OPM_THROW(std::invalid_argument, "Type 'MESS' have no associated data");
        break;
    default:
        OPM_THROW(std::invalid_argument, "Unknown field type");
        break;
    }
}


std::tuple<int, int, int> Opm::ecl::block_size_data_formatted(EIOD::eclArrType arrType)
{
    using BlockSizeTuple = std::tuple<int, int, int>;

    switch (arrType) {
    case EIOD::INTE:
        return BlockSizeTuple{EIOD::MaxNumBlockInte, EIOD::numColumnsInte, EIOD::columnWidthInte};
        break;
    case EIOD::REAL:
        return BlockSizeTuple{EIOD::MaxNumBlockReal,EIOD::numColumnsReal, EIOD::columnWidthReal};
        break;
    case EIOD::DOUB:
        return BlockSizeTuple{EIOD::MaxNumBlockDoub,EIOD::numColumnsDoub, EIOD::columnWidthDoub};
        break;
    case EIOD::LOGI:
        return BlockSizeTuple{EIOD::MaxNumBlockLogi,EIOD::numColumnsLogi, EIOD::columnWidthLogi};
        break;
    case EIOD::CHAR:
        return BlockSizeTuple{EIOD::MaxNumBlockChar,EIOD::numColumnsChar, EIOD::columnWidthChar};
        break;
    case EIOD::MESS:
        OPM_THROW(std::invalid_argument, "Type 'MESS' have no associated data") ;
        break;
    default:
        OPM_THROW(std::invalid_argument, "Unknown field type");
        break;
    }
}


std::string Opm::ecl::trimr(const std::string &str1)
{
    if (str1 == "        ") {
        return "";
    } else {
        int p = str1.find_last_not_of(" ");

        return str1.substr(0,p+1);
    }
}
