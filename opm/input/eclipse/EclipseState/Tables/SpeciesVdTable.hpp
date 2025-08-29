/*
  Copyright 2020 Equinor ASA.

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
#ifndef SPECIES_VD_TABLE_HPP
#define SPECIES_VD_TABLE_HPP

#include <opm/input/eclipse/EclipseState/Tables/SimpleTable.hpp>

namespace Opm {

class SpeciesVdTable : public SimpleTable
{
public:
    SpeciesVdTable() = default;
    SpeciesVdTable(const Opm::DeckItem& item, double inv_volume, const int tableID);
};

} // namespace Opm

#endif