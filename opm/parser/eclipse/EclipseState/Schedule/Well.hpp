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


#ifndef WELL_HPP_
#define WELL_HPP_

#include <boost/shared_ptr.hpp>
#include <string>

namespace Opm {

    class Well {
    public:
        Well(const std::string& name);
        const std::string& name() const;
        
    private:
        std::string m_name;
    };
    typedef boost::shared_ptr<Well> WellPtr;
    typedef boost::shared_ptr<const Well> WellConstPtr;
}



#endif /* WELL_HPP_ */
