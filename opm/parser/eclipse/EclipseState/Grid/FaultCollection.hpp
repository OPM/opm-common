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
#ifndef FAULT_COLLECTION_HPP_
#define FAULT_COLLECTION_HPP_

#include <cstddef>
#include <string>
#include <memory>
#include <map>


#include <opm/parser/eclipse/EclipseState/Util/OrderedMap.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>

#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Fault.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FaultFace.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FaceDir.hpp>

namespace Opm {


class FaultCollection {
public:
    FaultCollection();
    FaultCollection( std::shared_ptr<const Deck> deck, std::shared_ptr<const EclipseGrid> grid);

    size_t size() const;
    bool hasFault(const std::string& faultName) const;
    std::shared_ptr<Fault>  getFault(const std::string& faultName);
    std::shared_ptr<const Fault>  getFault(const std::string& faultName) const;
    std::shared_ptr<Fault>  getFault(size_t faultIndex);
    std::shared_ptr<const Fault>  getFault(size_t faultIndex) const;
    void addFault(std::shared_ptr<Fault> fault);
    void setTransMult(const std::string& faultName , double transMult);

private:
    OrderedMap<std::shared_ptr<Fault > > m_faults;
};
}


#endif
