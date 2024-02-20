/*
   Copyright (C) 2022 Equinor
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

#include "readKeywordCarfin.hpp"
namespace Opm {

    void readKeywordCarfin( const DeckRecord& deckRecord, CarfinManager& carfinManager) {
        const auto& NAMEItem = deckRecord.getItem("NAME");
        const auto& I1Item = deckRecord.getItem("I1");
        const auto& I2Item = deckRecord.getItem("I2");
        const auto& J1Item = deckRecord.getItem("J1");
        const auto& J2Item = deckRecord.getItem("J2");
        const auto& K1Item = deckRecord.getItem("K1");
        const auto& K2Item = deckRecord.getItem("K2");
        const auto& NXItem = deckRecord.getItem("NX");
        const auto& NYItem = deckRecord.getItem("NY");
        const auto& NZItem = deckRecord.getItem("NZ");
 //       const auto& NWMAXItem = deckRecord.getItem("NWMAX");
 //       const auto& PARENTItem = deckRecord.getItem("PARENT");

        const auto& active_carfin = carfinManager.getActiveCarfin();

        const std::string name = NAMEItem.defaultApplied(0) ? active_carfin.NAME() : NAMEItem.get<std::string>(0);
        const int i1 = I1Item.defaultApplied(0) ? active_carfin.I1() : I1Item.get<int>(0) - 1;
        const int i2 = I2Item.defaultApplied(0) ? active_carfin.I2() : I2Item.get<int>(0) - 1;
        const int j1 = J1Item.defaultApplied(0) ? active_carfin.J1() : J1Item.get<int>(0) - 1;
        const int j2 = J2Item.defaultApplied(0) ? active_carfin.J2() : J2Item.get<int>(0) - 1;
        const int k1 = K1Item.defaultApplied(0) ? active_carfin.K1() : K1Item.get<int>(0) - 1;
        const int k2 = K2Item.defaultApplied(0) ? active_carfin.K2() : K2Item.get<int>(0) - 1;
        const int nx = NXItem.defaultApplied(0) ? active_carfin.NX() : NXItem.get<int>(0);
        const int ny = NYItem.defaultApplied(0) ? active_carfin.NY() : NYItem.get<int>(0);
        const int nz = NZItem.defaultApplied(0) ? active_carfin.NZ() : NZItem.get<int>(0);

        carfinManager.readKeywordCarfin(name,i1,i2,j1,j2,k1,k2,nx,ny,nz);

    }
}
