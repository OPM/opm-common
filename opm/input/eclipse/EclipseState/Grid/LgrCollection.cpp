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

#include <opm/input/eclipse/EclipseState/Grid/LgrCollection.hpp>

#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/input/eclipse/EclipseState/Grid/Carfin.hpp>
#include <opm/input/eclipse/EclipseState/Grid/CarfinManager.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>

#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>

#include <cstddef>

namespace Opm {

    LgrCollection::LgrCollection()
    {}

    /*
    This class stores all LGR's. At this moment we only support lgr's input from CARFIN blocks
    TODO: Collect also lgrs from RADFIN blocks...
     */

    LgrCollection::LgrCollection(const GRIDSection& gridSection, const EclipseGrid& grid) {
        const auto& lgrKeywords = gridSection.getKeywordList<ParserKeywords::CARFIN>();

        for (const auto& lgrsKeyword : lgrKeywords) {
            OpmLog::info(OpmInputError::format("\nLoading lgrs from {keyword} in {file} line {line}", lgrsKeyword->location()));

            for (const auto& lgrRecord : *lgrsKeyword) {
                addLgr(grid, lgrRecord);
            }
        }
    }

    std::size_t LgrCollection::size() const {
        return m_lgrs.size();
    }

    bool LgrCollection::hasLgr(const std::string& lgrName) const {
        return m_lgrs.count( lgrName ) > 0;
    }

    const Carfin& LgrCollection::getLgr(const std::string& lgrName) const {
        return m_lgrs.get( lgrName );
    }

    Carfin& LgrCollection::getLgr(const std::string& lgrName) {
        return m_lgrs.get( lgrName );
    }

    const Carfin& LgrCollection::getLgr(std::size_t lgrIndex) const {
        return m_lgrs.iget( lgrIndex );
    }

    Carfin& LgrCollection::getLgr(std::size_t lgrIndex) {
        return m_lgrs.iget( lgrIndex );
    }

    void LgrCollection::addLgr(const EclipseGrid& grid, const DeckRecord&  lgrRecord) {
       // A nested CARFIN (PARENT != GLOBAL) addresses cells in its parent LGR's
       // own refined Cartesian space, so its I/J/K range and divisions must be
       // validated against the parent LGR's dimensions - not the global grid.
       // (CARFIN records are processed in deck order, parent before child, so
       // the parent LGR is already in the collection.)
       const auto& parentItem = lgrRecord.getItem<ParserKeywords::CARFIN::PARENT>();
       const std::string parent_name = parentItem.defaultApplied(0)
           ? std::string{"GLOBAL"} : parentItem.get<std::string>(0);

       if (parent_name != "GLOBAL") {
           if (m_lgrs.count(parent_name) == 0) {
               throw std::invalid_argument("Nested CARFIN names parent LGR '" + parent_name
                   + "', which has not been defined yet. Parent LGRs must precede their "
                     "children in the deck.");
           }
           const Carfin& parent = m_lgrs.get(parent_name);
           // The parent LGR is a refined Cartesian block: every local cell is
           // active and the active index is the identity in its local space.
           const GridDims parentDims(static_cast<std::size_t>(parent.NX()),
                                     static_cast<std::size_t>(parent.NY()),
                                     static_cast<std::size_t>(parent.NZ()));
           Carfin lgr(parentDims,
                      [](const std::size_t) { return true; },
                      [](const std::size_t local_index) { return local_index; });
           lgr.update(lgrRecord);
           m_lgrs.insert(std::make_pair(lgr.NAME(), lgr));
           return;
       }

       Carfin lgr(grid,
            [&grid](const std::size_t global_index)
            {
                return grid.cellActive(global_index);
            },
            [&grid](const std::size_t global_index)
            {
                return grid.activeIndex(global_index);
            });
       lgr.update(lgrRecord);
       m_lgrs.insert(std::make_pair(lgr.NAME(), lgr));
    }

    bool LgrCollection::operator==(const LgrCollection& data) const {
        return this->m_lgrs == data.m_lgrs;
    }
}
