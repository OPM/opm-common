/*
  Copyright (c) 2018 Equinor ASA

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

#ifndef OPM_OUTPUT_ECLIPSE_VECTOR_GROUP_HPP
#define OPM_OUTPUT_ECLIPSE_VECTOR_GROUP_HPP

#include <vector>

namespace Opm { namespace RestartIO { namespace Helpers { namespace VectorItems {

    namespace XGroup {
        enum index : std::vector<double>::size_type {
            OilPrRate  =  0, // Group's oil production rate
            WatPrRate  =  1, // Group's water production rate
            GasPrRate  =  2, // Group's gas production rate
            LiqPrRate  =  3, // Group's liquid production rate

            WatInjRate =  5, // Group's water injection rate
            GasInjRate =  6, // Group's gas injection rate

            WatCut     =  8, // Group's producing water cut
            GORatio    =  9, // Group's producing gas/oil ratio

            OilPrTotal  = 10, // Group's total cumulative oil production
            WatPrTotal  = 11, // Group's total cumulative water production
            GasPrTotal  = 12, // Group's total cumulative gas production
            VoidPrTotal = 13, // Group's total cumulative reservoir
                              // voidage production

            WatInjTotal = 15, // Group's total cumulative water injection
            GasInjTotal = 16, // Group's total cumulative gas injection

            OilPrPot = 22, // Group's oil production potential
            WatPrPot = 23, // Group's water production potential
        };
    } // XGroup

}}}} // Opm::RestartIO::Helpers::VectorItems

#endif // OPM_OUTPUT_ECLIPSE_VECTOR_GROUP_HPP
