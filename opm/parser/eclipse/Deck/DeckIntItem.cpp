/*
  Copyright 2013 Statoil ASA.

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

#include "DeckIntItem.hpp"
#include <boost/lexical_cast.hpp>
namespace Opm {

    int DeckIntItem::getInt(unsigned int index) const {
        if (index < m_data.size()) {
            return m_data[index];
        } else
            throw std::out_of_range("Out of range, index must be lower than " + boost::lexical_cast<std::string>(m_data.size()));
    }
}

