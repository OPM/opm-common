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



#include <boost/date_time.hpp>

#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>

namespace Opm {

    Well::Well(const std::string& name , TimeMapConstPtr timeMap) : m_oilRate( new DynamicState<double>( timeMap , 0.0)) {
        m_name = name;
    }

    const std::string& Well::name() const {
        return m_name;
    }


    double Well::getOilRate(size_t timeStep) const {
        return m_oilRate->get( timeStep );
    }

    
    void Well::setOilRate(size_t timeStep, double oilRate) {
        m_oilRate->add( timeStep , oilRate );
    }

    
    void Well::addWELSPECS(DeckRecordConstPtr deckRecord) {
        
    }

}



