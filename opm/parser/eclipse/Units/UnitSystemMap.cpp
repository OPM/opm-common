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

#include <opm/parser/eclipse/Units/UnitSystemMap.hpp>

#include <vector>
#include <iostream>
#include <string>
#include <algorithm>
#include <stdexcept>

namespace Opm {

    UnitSystemMap::UnitSystemMap() {
        
    }

    std::string UnitSystemMap::makeRegularName(const std::string& name) {
        std::string regularName;
        for (auto iter = name.begin(); iter != name.end(); ++iter) 
            regularName.push_back( std::tolower( *iter ));
                
        return regularName;
    }


    bool UnitSystemMap::hasSystem(const std::string& name) const {
        std::string regularName = makeRegularName(name); 
        return (m_systemMap.find(regularName) != m_systemMap.end());
    }
    

    void UnitSystemMap::addSystem(std::shared_ptr<UnitSystem> system) {
        std::string regularName = makeRegularName(system->getName()); 
        if (m_systemMap.find(regularName) != m_systemMap.end())
            m_systemMap.erase( regularName );
        
        m_systemMap.insert( std::make_pair( regularName , system));
    }


    std::shared_ptr<const UnitSystem> UnitSystemMap::getSystem( const std::string& name) const {
        std::string regularName = makeRegularName(name); 
        if (m_systemMap.find(regularName) != m_systemMap.end()) 
            return m_systemMap.at( regularName );
        else
            throw std::invalid_argument( "Does not have a unit system: " + name);
    }

}
