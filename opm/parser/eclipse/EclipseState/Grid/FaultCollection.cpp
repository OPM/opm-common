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

#include <stdexcept>

#include <opm/parser/eclipse/EclipseState/Grid/FaultCollection.hpp>

namespace Opm {

    FaultCollection::FaultCollection()
    {

    }

    size_t FaultCollection::size() const {
        return m_faults.size();
    }


    bool FaultCollection::hasFault(const std::string& faultName) const {
        return m_faults.hasKey( faultName );
    }


    std::shared_ptr<Fault> FaultCollection::getFault(const std::string& faultName) const {
        return m_faults.get( faultName );
    }

    std::shared_ptr<Fault> FaultCollection::getFault(size_t faultIndex) const {
        return m_faults.get( faultIndex );
    }


    void FaultCollection::addFault(std::shared_ptr<Fault> fault) {
        m_faults.insert(fault->getName() , fault);
    }



    void FaultCollection::setTransMult(const std::string& faultName , double transMult) {
        std::shared_ptr<Fault> fault = getFault( faultName );
        fault->setTransMult( transMult );
    }



}
