/*
 Copyright (C) 2023 Equinor
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

#ifndef OPM_PARSER_LGR_COLLECTION_HPP
#define OPM_PARSER_LGR_COLLECTION_HPP

#include <string>

#include <opm/input/eclipse/EclipseState/Util/OrderedMap.hpp>
#include <opm/input/eclipse/EclipseState/Grid/CarfinManager.hpp>
#include <opm/input/eclipse/EclipseState/Grid/Carfin.hpp>

namespace Opm {

    class DeckRecord;
    class GridDims;
    class GRIDSection;


class LgrCollection {
public:
    LgrCollection();
    LgrCollection(const GRIDSection& gridSection, const GridDims& gridDims);


    size_t size() const;
    bool hasLgr(const std::string& lgrName) const;
    Carfin& getLgr(const std::string& lgrName);
    const Carfin& getLgr(const std::string& lgrName) const;

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(m_lgrs);
    }

private:
    OrderedMap<Carfin, 8> m_lgrs;

};
}

#endif // OPM_PARSER_LGR_COLLECTION_HPP
