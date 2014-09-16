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
#include <map>
#include <set>

#include <opm/parser/eclipse/EclipseState/Grid/MULTREGTScanner.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FaceDir.hpp>

namespace Opm {

    namespace MULTREGT {
        
        std::string RegionNameFromDeckValue(const std::string& stringValue) {
            if (stringValue == "O")
                return "OPERNUM";
            
            if (stringValue == "F")
                return "FLUXNUM";

            if (stringValue == "M")
                return "MULTNUM";

            throw std::invalid_argument("The input string: " + stringValue + " was invalid. Expected: O/F/M");
        }

        
        
        NNCBehaviourEnum NNCBehaviourFromString(const std::string& stringValue) {
            if (stringValue == "ALL")
                return ALL;

            if (stringValue == "NNC")
                return NNC;

            if (stringValue == "NONNC")
                return NONNC;

            if (stringValue == "NOAQUNNC")
                return NOAQUNNC;
            
            throw std::invalid_argument("The input string: " + stringValue + " was invalid. Expected: ALL/NNC/NONNC/NOAQUNNC");
        }


    }
    

    

    MULTREGTRecord::MULTREGTRecord(DeckRecordConstPtr deckRecord) : 
        m_srcRegion("SRC_REGION"),
        m_targetRegion("TARGET_REGION"),
        m_region("REGION")
    {
        DeckItemConstPtr srcItem = deckRecord->getItem("SRC_REGION");
        DeckItemConstPtr targetItem = deckRecord->getItem("TARGET_REGION");
        DeckItemConstPtr tranItem = deckRecord->getItem("TRAN_MULT");
        DeckItemConstPtr dirItem = deckRecord->getItem("DIRECTIONS");
        DeckItemConstPtr nncItem = deckRecord->getItem("NNC_MULT");
        DeckItemConstPtr defItem = deckRecord->getItem("REGION_DEF");


        if (!srcItem->defaultApplied(0))
            m_srcRegion.setValue( srcItem->getInt(0) );

        if (!targetItem->defaultApplied(0))
            m_targetRegion.setValue( targetItem->getInt(0) );

        m_transMultiplier = tranItem->getRawDouble(0);
        m_directions = FaceDir::FromMULTREGTString( dirItem->getString(0) );
        m_nncBehaviour = MULTREGT::NNCBehaviourFromString( nncItem->getString(0));

        if (!defItem->defaultApplied(0))
            m_region.setValue( MULTREGT::RegionNameFromDeckValue ( defItem->getString(0) ));
    } 


    /*****************************************************************/

    MULTREGTScanner::MULTREGTScanner() {
        
    }


    void MULTREGTScanner::assertKeywordSupported(DeckKeywordConstPtr deckKeyword) {
        for (auto iter = deckKeyword->begin(); iter != deckKeyword->end(); ++iter) {
            MULTREGTRecord record( *iter );
            
            if (record.m_nncBehaviour != MULTREGT::ALL)
                throw std::invalid_argument("Sorry - currently only \'ALL\' is supported for MULTREGT NNC behaviour.");
            
            if (!record.m_srcRegion.hasValue())
                throw std::invalid_argument("Sorry - currently it is not supported with a defaulted source region value.");

            if (!record.m_targetRegion.hasValue())
                throw std::invalid_argument("Sorry - currently it is not supported with a defaulted target region value.");

            if (record.m_srcRegion.getValue() == record.m_targetRegion.getValue())
                throw std::invalid_argument("Sorry - multregt applied internally to a region is not yet supported");
            
        }
    }
    

    
    void MULTREGTScanner::addKeyword(DeckKeywordConstPtr deckKeyword) {
        assertKeywordSupported( deckKeyword );

        for (auto iter = deckKeyword->begin(); iter != deckKeyword->end(); ++iter) {
            MULTREGTRecord record( *iter );
            /*
              The default value for the region item is to use the
              region item on the previous record, or alternatively
              'MULTNUM' for the first record.
            */
            if (!record.m_region.hasValue()) {
                if (m_records.size() > 0) {
                    auto previousRecord = m_records.back();
                    record.m_region.setValue( previousRecord.m_region.getValue() );
                } else
                    record.m_region.setValue( "MULTNUM" );
            }
            m_records.push_back( record );
        }
        
    }
    

    /*
      This function will check the region values in globalIndex1 and
      globalIndex and see if they match the regionvalues specified in
      the deck. The function checks both directions:

      Assume the relevant MULTREGT record looks like:

         1  2   0.10  XYZ  ALL M /

      I.e. we are checking for the boundary between regions 1 and
      2. We assign the transmissibility multiplier to the correct face
      of the cell with value 1:

         -----------
         | 1  | 2  |   =>  MultTrans( i,j,k,FaceDir::XPlus ) *= 0.50
         -----------

         -----------
         | 2  | 1  |   =>  MultTrans( i+1,j,k,FaceDir::XMinus ) *= 0.50
         -----------

    */
    void MULTREGTScanner::checkConnection( MULTREGTSearchMap& map , std::vector< MULTREGTConnection >& connections, std::shared_ptr<GridProperty<int> > region, size_t globalIndex1 , size_t globalIndex2 , FaceDir::DirEnum faceDir1 ,FaceDir::DirEnum faceDir2) {
        int regionValue1 = region->iget(globalIndex1);
        int regionValue2 = region->iget(globalIndex2);
        
        std::pair<int,int> pair{regionValue1 , regionValue2};
        if (map.count(pair) == 1) {
            const MULTREGTRecord * record = map[pair];
            if (record->m_directions & faceDir1) {
                connections.push_back( MULTREGTConnection{ globalIndex1 , faceDir1 , record->m_transMultiplier } );
            }
        }
        
        pair = std::pair<int,int>{regionValue2 , regionValue1};
        if (map.count(pair) == 1) {
            const MULTREGTRecord * record = map[pair];
            if (record->m_directions & faceDir2) {
                connections.push_back( MULTREGTConnection{ globalIndex2 , faceDir2 , record->m_transMultiplier } );
            }
        }
    }





    /*
      Observe that the (REGION1 -> REGION2) pairs behave like keys;
      i.e. for the MULTREGT keyword

        MULTREGT
          2  4   0.75    Z   ALL    M / 
          2  4   2.50   XY   ALL    F / 
        /   

      The first record is completely overweritten by the second
      record, this is because the both have the (2 -> 4) region
      identifiers. This behaviourt is ensured by using a map with
      std::pair<region1,region2> as key.

      This function starts with some initial preprocessing to create a
      map which looks like this:


         searchMap = {"MULTNUM" : {std::pair(1,2) : std::tuple(TransFactor , Face , Region),
                                   std::pair(4,7) : std::tuple(TransFactor , Face , Region),
                                   ...},
                      "FLUXNUM" : {std::pair(4,8) : std::tuple(TransFactor , Face , Region),
                                   std::pair(1,4) : std::tuple(TransFactor , Face , Region),
                                   ...}}     

      Then it will go through the different regions and looking for
      interface with the wanted region values.
    */

    const std::vector< MULTREGTConnection > MULTREGTScanner::scanRegions( std::shared_ptr<Opm::GridProperties<int> > regions) {
        std::vector< MULTREGTConnection > connections;
        std::map<std::string , MULTREGTSearchMap> searchMap;
        {
            MULTREGTSearchMap searchPairs;
            for (std::vector<MULTREGTRecord>::const_iterator record = m_records.begin(); record != m_records.end(); ++record) {
                if (regions->hasKeyword( record->m_region.getValue())) {
                    if (record->m_srcRegion.hasValue() && record->m_targetRegion.hasValue()) {
                        int srcRegion    = record->m_srcRegion.getValue();
                        int targetRegion = record->m_targetRegion.getValue();
                        if (srcRegion != targetRegion) {
                            std::pair<int,int> pair{ srcRegion, targetRegion };
                            searchPairs[pair] = &(*record);
                        }
                    }
                }
                else
                    throw std::logic_error("MULTREGT record is based on region: " + record->m_region.getValue() + " which is not in the deck");
            }


            for (auto iter = searchPairs.begin(); iter != searchPairs.end(); ++iter) {
                const MULTREGTRecord * record = (*iter).second;
                std::pair<int,int> pair = (*iter).first;
                const std::string& keyword = record->m_region.getValue();
                if (searchMap.count(keyword) == 0) 
                    searchMap[keyword] = MULTREGTSearchMap();
                
                searchMap[keyword][pair] = record;
            }
        }

        // Iterate through the different regions
        for (auto iter = searchMap.begin(); iter != searchMap.end(); iter++) {
            std::shared_ptr<GridProperty<int> > region = regions->getKeyword( (*iter).first );
            MULTREGTSearchMap map = (*iter).second;
            

            // Iterate through all the cells in the region.
            for (size_t k=0; k < region->getNZ(); k++) {
                for (size_t j = 0; j < region->getNY(); j++) {
                    for (size_t i = 0; i < region->getNX(); i++) {
                        size_t globalIndex1 = i + j*region->getNX() + k*region->getNY() * region->getNX();

                        // X Direction
                        if ((i + 1) < region->getNX()) {
                            size_t globalIndex2 = globalIndex1 + 1;

                            checkConnection( map , connections, region , globalIndex1 , globalIndex2 , FaceDir::XPlus , FaceDir::XMinus);
                        }

                        // Y Direction
                        if ((j + 1) < region->getNY()) {
                            size_t globalIndex2 = globalIndex1 + region->getNX();

                            checkConnection( map , connections, region , globalIndex1 , globalIndex2 , FaceDir::YPlus , FaceDir::YMinus);
                        }

                        // Z Direction
                        if ((k + 1) < region->getNZ()) {
                            size_t globalIndex2 = globalIndex1 + region->getNX() * region->getNY();

                            checkConnection( map , connections, region , globalIndex1 , globalIndex2 , FaceDir::ZPlus , FaceDir::ZMinus);
                        }
                    }
                }
            }
                
        }
        
        return connections;
    }

}
