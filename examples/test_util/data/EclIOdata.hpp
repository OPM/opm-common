/*
  Copyright 2019 Statoil ASA.

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

#ifndef OPM_ECLIO_DATA_HPP
#define OPM_ECLIO_DATA_HPP

#include <tuple>


namespace Opm {

 namespace ecl {

    // type MESS have no assisiated data 
    
    enum eclArrType {
        INTE, REAL, DOUB, CHAR, LOGI, MESS
    };
    
    const unsigned int true_value = 0xffffffff;
    const unsigned int false_value = 0x00000000;

    const int sizeOfInte =  4;    // number of bytes pr integer (inte) element
    const int sizeOfReal =  4;    // number of bytes pr float (real) element
    const int sizeOfDoub =  8;    // number of bytes pr double (doub) element
    const int sizeOfLogi =  4;    // number of bytes pr bool (logi) element
    const int sizeOfChar =  8;    // number of bytes pr string (char) element

    const int MaxBlockSizeInte = 4000;    // Maximum block size for INTE arrays in binary files  
    const int MaxBlockSizeReal = 4000;    // Maximum block size for REAL arrays in binary files  
    const int MaxBlockSizeDoub = 8000;    // Maximum block size for DOUB arrays in binary files  
    const int MaxBlockSizeLogi = 4000;    // Maximum block size for LOGI arrays in binary files  
    const int MaxBlockSizeChar =  840;    // Maximum block size for CHAR arrays in binary files      

 } // ecl
} // Opm

#endif // OPM_ECLIO_DATA_HPP
