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

#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>
#include <opm/input/eclipse/EclipseState/Grid/LgrCollection.hpp>
#include <opm/input/eclipse/EclipseState/Grid/CarfinManager.hpp>
#include <opm/input/eclipse/EclipseState/Grid/Carfin.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>


namespace Opm {


    LgrCollection::LgrCollection()
    {}

    /*
    This class stores all LGR's. At this moment we only support lgr's input from CARFIN blocks
    TODO: Collect also lgrs from RADFIN blocks...
     */

    LgrCollection::LgrCollection(const GRIDSection& gridSection, const EclipseGrid& grid) {
        const auto& lgrKeywords = gridSection.getKeywordList<ParserKeywords::CARFIN>();

        for (auto keyword_iter = lgrKeywords.begin(); keyword_iter != lgrKeywords.end(); ++keyword_iter) {
            const auto& lgrsKeyword = *keyword_iter;
            OpmLog::info(OpmInputError::format("\nLoading lgrs from {keyword} in {file} line {line}", lgrsKeyword->location()));

            for (auto iter = lgrsKeyword->begin(); iter != lgrsKeyword->end(); ++iter) {
                const auto& lgrRecord = *iter;
                
                addLgr(grid, lgrRecord);
            }
        }
    }

    size_t LgrCollection::size() const {
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

    void LgrCollection::addLgr(const EclipseGrid& grid, const DeckRecord&  lgrRecord) {
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
}
