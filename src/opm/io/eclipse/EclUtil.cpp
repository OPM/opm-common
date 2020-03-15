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

#include <opm/io/eclipse/EclUtil.hpp>

#include <opm/common/ErrorMacros.hpp>

#include <algorithm>
#include <stdexcept>


int Opm::EclIO::flipEndianInt(int num)
{
    unsigned int tmp = __builtin_bswap32(num);
    return static_cast<int>(tmp);
}

long int Opm::EclIO::flipEndianLongInt(long int num)
{
    unsigned long int tmp = __builtin_bswap64(num);
    return static_cast<long int>(tmp);
}

float Opm::EclIO::flipEndianFloat(float num)
{
    float value = num;

    char* floatToConvert = reinterpret_cast<char*>(&value);
    std::reverse(floatToConvert, floatToConvert+4);

    return value;
}


double Opm::EclIO::flipEndianDouble(double num)
{
    double value = num;

    char* doubleToConvert = reinterpret_cast<char*>(&value);
    std::reverse(doubleToConvert, doubleToConvert+8);

    return value;
}


std::tuple<int, int> Opm::EclIO::block_size_data_binary(eclArrType arrType)
{
    using BlockSizeTuple = std::tuple<int, int>;

    switch (arrType) {
    case INTE:
        return BlockSizeTuple{sizeOfInte, MaxBlockSizeInte};
        break;
    case REAL:
        return BlockSizeTuple{sizeOfReal, MaxBlockSizeReal};
        break;
    case DOUB:
        return BlockSizeTuple{sizeOfDoub, MaxBlockSizeDoub};
        break;
    case LOGI:
        return BlockSizeTuple{sizeOfLogi, MaxBlockSizeLogi};
        break;
    case CHAR:
        return BlockSizeTuple{sizeOfChar, MaxBlockSizeChar};
        break;
    case MESS:
        OPM_THROW(std::invalid_argument, "Type 'MESS' have no associated data");
        break;
    default:
        OPM_THROW(std::invalid_argument, "Unknown field type");
        break;
    }
}


std::tuple<int, int, int> Opm::EclIO::block_size_data_formatted(eclArrType arrType)
{
    using BlockSizeTuple = std::tuple<int, int, int>;

    switch (arrType) {
    case INTE:
        return BlockSizeTuple{MaxNumBlockInte, numColumnsInte, columnWidthInte};
        break;
    case REAL:
        return BlockSizeTuple{MaxNumBlockReal,numColumnsReal, columnWidthReal};
        break;
    case DOUB:
        return BlockSizeTuple{MaxNumBlockDoub,numColumnsDoub, columnWidthDoub};
        break;
    case LOGI:
        return BlockSizeTuple{MaxNumBlockLogi,numColumnsLogi, columnWidthLogi};
        break;
    case CHAR:
        return BlockSizeTuple{MaxNumBlockChar,numColumnsChar, columnWidthChar};
        break;
    case MESS:
        OPM_THROW(std::invalid_argument, "Type 'MESS' have no associated data") ;
        break;
    default:
        OPM_THROW(std::invalid_argument, "Unknown field type");
        break;
    }
}


std::string Opm::EclIO::trimr(const std::string &str1)
{
    if (str1 == "        ") {
        return "";
    } else {
        int p = str1.find_last_not_of(" ");

        return str1.substr(0,p+1);
    }
}
