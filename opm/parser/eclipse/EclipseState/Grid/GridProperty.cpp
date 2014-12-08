/*
  Copyright 2014 Statoil ASA.

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

#include <opm/parser/eclipse/EclipseState/Grid/Box.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperty.hpp>

namespace Opm {

template<>
void GridProperty<int>::setDataPoint(size_t sourceIdx, size_t targetIdx, Opm::DeckItemConstPtr deckItem) {
    m_data[targetIdx] = deckItem->getInt(sourceIdx);
}

template<>
void GridProperty<double>::setDataPoint(size_t sourceIdx, size_t targetIdx, Opm::DeckItemConstPtr deckItem) {
    m_data[targetIdx] = deckItem->getSIDouble(sourceIdx);
}

template<>
bool GridProperty<int>::containsNaN( ) {
    throw std::logic_error("Only <double> and can be meaningfully queried for nan");
}

template<>
bool GridProperty<double>::containsNaN( ) {
    bool return_value = false;
    size_t size = m_data.size();
    size_t index = 0;
    while (true) {
        if (std::isnan(m_data[index])) {
            return_value = true;
            break;
        }

        index++;
        if (index == size)
            break;
    }
    return return_value;
}
}



