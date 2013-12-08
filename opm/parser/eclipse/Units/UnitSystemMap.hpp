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

#ifndef UNITSYSTEMMAP_H
#define UNITSYSTEMMAP_H

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <memory>
#include <vector>
#include <string>

namespace Opm {

    class UnitSystemMap {
    public:
        UnitSystemMap();
        bool hasSystem(const std::string& name) const;
        std::shared_ptr<const UnitSystem> getSystem( const std::string& name) const;
        void addSystem(std::shared_ptr<UnitSystem> system);
        
    private:
        static std::string makeRegularName(const std::string& name);
        
        std::map<std::string , std::shared_ptr<UnitSystem> > m_systemMap;
    };
        
 
}


#endif
