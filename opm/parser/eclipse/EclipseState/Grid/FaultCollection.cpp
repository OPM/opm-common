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

#include <opm/parser/eclipse/Parser/ParserKeywords.hpp>

#include <opm/parser/eclipse/EclipseState/Grid/FaultCollection.hpp>

namespace Opm {

    FaultCollection::FaultCollection()
    {

    }


    FaultCollection::FaultCollection( std::shared_ptr<const Deck> deck, std::shared_ptr<const EclipseGrid> grid) {
        std::vector<std::shared_ptr<const DeckKeyword> > faultKeywords = deck->getKeywordList<ParserKeywords::FAULTS>();

        for (auto keyword_iter = faultKeywords.begin(); keyword_iter != faultKeywords.end(); ++keyword_iter) {
            std::shared_ptr<const DeckKeyword> faultsKeyword = *keyword_iter;
            for (auto iter = faultsKeyword->begin(); iter != faultsKeyword->end(); ++iter) {
                DeckRecordConstPtr faultRecord = *iter;
                const std::string& faultName = faultRecord->getItem(0)->getString(0);
                int I1 = faultRecord->getItem(1)->getInt(0) - 1;
                int I2 = faultRecord->getItem(2)->getInt(0) - 1;
                int J1 = faultRecord->getItem(3)->getInt(0) - 1;
                int J2 = faultRecord->getItem(4)->getInt(0) - 1;
                int K1 = faultRecord->getItem(5)->getInt(0) - 1;
                int K2 = faultRecord->getItem(6)->getInt(0) - 1;
                FaceDir::DirEnum faceDir = FaceDir::FromString( faultRecord->getItem(7)->getString(0) );
                std::shared_ptr<const FaultFace> face = std::make_shared<const FaultFace>(grid->getNX() , grid->getNY() , grid->getNZ(),
                                                                                          static_cast<size_t>(I1) , static_cast<size_t>(I2) ,
                                                                                          static_cast<size_t>(J1) , static_cast<size_t>(J2) ,
                                                                                          static_cast<size_t>(K1) , static_cast<size_t>(K2) ,
                                                                                          faceDir);
                if (!hasFault(faultName)) {
                    std::shared_ptr<Fault> fault = std::make_shared<Fault>( faultName );
                    addFault( fault );
                }

                {
                    std::shared_ptr<Fault> fault = getFault( faultName );
                    fault->addFace( face );
                }
            }
        }
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
