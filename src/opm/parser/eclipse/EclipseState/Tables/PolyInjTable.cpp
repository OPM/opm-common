/*
  Copyright 2018 Statoil ASA.

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

// TODO: this will go to Tables.cpp later.

#include <opm/parser/eclipse/EclipseState/Tables/PolyInjTable.hpp>


namespace Opm{

    int PolyInjTable::getTableNumber() const
    {
        return m_table_number;
    }

    const std::vector<double>& PolyInjTable::getThroughputs() const
    {
        return m_throughputs;
    }

    const std::vector<double>& PolyInjTable::getVelocities() const
    {
        return m_velocities;
    }

    const std::vector<std::vector<double>>& PolyInjTable::getTableData() const
    {
        return m_data;
    }
}
