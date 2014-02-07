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

#include <opm/parser/eclipse/Units/Dimension.hpp>

#include <string>
#include <stdexcept>
#include <cmath>

namespace Opm {

    Dimension::Dimension() {
        
    }

    Dimension::Dimension(const std::string& name, double SIfactor) 
    {
        for (auto iter = name.begin(); iter != name.end(); ++iter) {
            if (!isalpha(*iter) && (*iter) != '1')
                throw std::invalid_argument("Invalid dimension name");
        }
        m_name = name;
        m_SIfactor = SIfactor;
    }

    
    double Dimension::getSIScaling() const {
        if (!std::isfinite(m_SIfactor))
            throw std::logic_error("The DeckItem contains a field with a context dependent unit. "
                                   "Use getRawDoubleData() and convert the returned value manually!");
        return m_SIfactor;
    }

    const std::string& Dimension::getName() const {
        return m_name;
    }


    Dimension * Dimension::newComposite(const std::string& dim , double SIfactor) {
        Dimension * dimension = new Dimension();
        dimension->m_name = dim;
        dimension->m_SIfactor = SIfactor;

        return dimension;
    }


    bool Dimension::equal(const Dimension& other) const {
        if (m_name != other.m_name)
            return false;
        if (m_SIfactor == other.m_SIfactor)
            return true;
        if (std::isnan(m_SIfactor) && std::isnan(other.m_SIfactor))
            return true;
        return false;
    }

}


