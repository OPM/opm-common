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

#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>
#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>
#include <opm/input/eclipse/EclipseState/Grid/LgrCollection.hpp>
#include <opm/input/eclipse/EclipseState/Grid/Carfin.hpp>
#include <opm/input/eclipse/EclipseState/Grid/CarfinManager.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/E.hpp>

namespace Opm {


    LgrCollection::LgrCollection()
    {}

    LgrCollection::LgrCollection(const GRIDSection& gridSection,
                                     const GridDims& grid) {
        const auto& lgrKeywords = gridSection.getKeywordList<ParserKeywords::CARFIN>();

        for (auto keyword_iter = lgrKeywords.begin(); keyword_iter != lgrKeywords.end(); ++keyword_iter) {
            const auto& lgrsKeyword = *keyword_iter;
            OpmLog::info(OpmInputError::format("\nLoading faults from {keyword} in {file} line {line}", lgrsKeyword->location()));

            for (auto iter = lgrsKeyword->begin(); iter != lgrsKeyword->end(); ++iter) {
                const auto& lgrRecord = *iter;
                const std::string& lgrName = lgrRecord.getItem(0).get< std::string >(0);

                addLgr(grid, lgrRecord, lgrName);
            }
        }
    }

    LgrCollection LgrCollection::serializationTestObject()
    {
        LgrCollection result;
        result.m_lgrs.insert({"test", Carfin::serializationTestObject()});

        return result;
    }

    void LgrCollection::addFaultFaces(const GridDims& grid,
                                        const DeckRecord& lgrRecord,
                                        const std::string& lgrName)
    {
        int I1 = faultRecord.getItem(1).get<int>(0) - 1;
        int I2 = faultRecord.getItem(2).get<int>(0) - 1;
        int J1 = faultRecord.getItem(3).get<int>(0) - 1;
        int J2 = faultRecord.getItem(4).get<int>(0) - 1;
        int K1 = faultRecord.getItem(5).get<int>(0) - 1;
        int K2 = faultRecord.getItem(6).get<int>(0) - 1;
        FaceDir::DirEnum faceDir = FaceDir::FromString(faultRecord.getItem(7).get<std::string>(0));
        FaultFace face { grid.getNX(), grid.getNY(), grid.getNZ(),
                         size_t(I1), size_t(I2),
                         size_t(J1), size_t(J2),
                         size_t(K1), size_t(K2),
                         faceDir };

        if (!hasFault(faultName))
            addFault(faultName);

        getFault( faultName ).addFace( face );
    }

    size_t LgrCollection::size() const {
        return m_lgrs.size();
    }

    bool LgrCollection::hasFault(const std::string& faultName) const {
        return m_flgrs.count( faultName ) > 0;
    }

    const Fault& FaultCollection::getFault(const std::string& faultName) const {
        return m_lgrs.get( faultName );
    }

    Fault& LgrCollection::getFault(const std::string& faultName) {
        return m_lgrs.get( faultName );
    }

    Fault& LgrCollection::getFault(size_t faultIndex) {
        return m_lgrs.iget( faultIndex );
    }

    const Fault& LgrCollection::getFault(size_t faultIndex) const {
        return m_lgrs.iget( faultIndex );
    }

    void LgrCollection::addLgr(const std::string& lgrName) {
        Carfin lgr(lgrName);
        m_lgrs.insert(std::make_pair(lgr.getName() , lgr));
    }

    bool LgrCollection::operator==(const LgrCollection& data) const {
        return this->m_lgrs == data.m_lgrs;
    }
}
