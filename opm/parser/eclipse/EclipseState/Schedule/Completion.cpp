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

#include <opm/parser/eclipse/EclipseState/Schedule/Completion.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Util/Value.hpp>

namespace Opm {

    Completion::Completion(int i, int j , int k , CompletionStateEnum state , 
                           const Value<double>& connectionTransmissibilityFactor,
                           const Value<double>& diameter, 
                           const Value<double>& skinFactor, 
                           const CompletionDirection::DirectionEnum direction)
        : m_i(i), m_j(j), m_k(k),
          m_diameter(diameter),
          m_connectionTransmissibilityFactor(connectionTransmissibilityFactor),
          m_skinFactor(skinFactor),
          m_state(state),
          m_direction(direction) 
    {  }


    bool Completion::sameCoordinate(const Completion& other) const {
        if ((m_i == other.m_i) && 
            (m_j == other.m_j) && 
            (m_k == other.m_k))
            return true;
        else
            return false;
    }

    /**
       This will break up one record and return a pair: <name ,
       [Completion1, Completion2, ... , CompletionN]>. The reason it
       will return a list is that the 'K1 K2' structure is
       disentangled, and each completion is returned separately.
    */
    
    std::pair<std::string , std::vector<CompletionPtr> > Completion::completionsFromCOMPDATRecord( DeckRecordConstPtr compdatRecord ) {
        std::vector<CompletionPtr> completions;
        std::string well = compdatRecord->getItem("WELL")->getTrimmedString(0);
        // We change from eclipse's 1 - n, to a 0 - n-1 solution
        int I = compdatRecord->getItem("I")->getInt(0) - 1;
        int J = compdatRecord->getItem("J")->getInt(0) - 1;
        int K1 = compdatRecord->getItem("K1")->getInt(0) - 1;
        int K2 = compdatRecord->getItem("K2")->getInt(0) - 1;
        CompletionStateEnum state = CompletionStateEnumFromString( compdatRecord->getItem("STATE")->getTrimmedString(0) );
        Value<double> connectionTransmissibilityFactor("ConnectionTransmissibilityFactor");
        Value<double> diameter("Diameter");
        Value<double> skinFactor("SkinFactor");

        {
            DeckItemConstPtr connectionTransmissibilityFactorItem = compdatRecord->getItem("CONNECTION_TRANSMISSIBILITY_FACTOR");
            DeckItemConstPtr diameterItem = compdatRecord->getItem("DIAMETER");
            DeckItemConstPtr skinFactorItem = compdatRecord->getItem("SKIN");

            if (connectionTransmissibilityFactorItem->size() > 0 && !connectionTransmissibilityFactorItem->defaultApplied(0))
                connectionTransmissibilityFactor.setValue( connectionTransmissibilityFactorItem->getSIDouble(0));
            
            if (diameterItem->size() > 0 && !diameterItem->defaultApplied(0))
                diameter.setValue( diameterItem->getSIDouble(0));

            if (skinFactorItem->size() > 0 && !skinFactorItem->defaultApplied(0))
                skinFactor.setValue( skinFactorItem->getRawDouble(0));
        }
        
        const CompletionDirection::DirectionEnum& direction = CompletionDirection::DirectionEnumFromString(compdatRecord->getItem("DIR")->getTrimmedString(0));

        for (int k = K1; k <= K2; k++) {
            CompletionPtr completion(new Completion(I , J , k , state , connectionTransmissibilityFactor, diameter, skinFactor, direction ));
            completions.push_back( completion );
        }

        return std::pair<std::string , std::vector<CompletionPtr> >( well , completions );
    }

    /*
      Will return a map:

      {
         "WELL1" : [ Completion1 , Completion2 , ... , CompletionN ],
         "WELL2" : [ Completion1 , Completion2 , ... , CompletionN ],
         ...
      }
    */   
            
    
    std::map<std::string , std::vector< CompletionPtr> > Completion::completionsFromCOMPDATKeyword( DeckKeywordConstPtr compdatKeyword ) {
        std::map<std::string , std::vector< CompletionPtr> > completionMapList;
        for (size_t recordIndex = 0; recordIndex < compdatKeyword->size(); recordIndex++) {
            std::pair<std::string , std::vector< CompletionPtr> > wellCompletionsPair = completionsFromCOMPDATRecord( compdatKeyword->getRecord( recordIndex ));
            std::string well = wellCompletionsPair.first;
            std::vector<CompletionPtr>& newCompletions = wellCompletionsPair.second;
        
            if (completionMapList.find(well) == completionMapList.end()) 
                 completionMapList[well] = std::vector<CompletionPtr>();
            
            {
                std::vector<CompletionPtr>& currentCompletions = completionMapList.find(well)->second;

                for (size_t ic = 0; ic < newCompletions.size(); ic++)
                    currentCompletions.push_back( newCompletions[ic] );
            }
        }
        return completionMapList;
    }

    void Completion::fixDefaultIJ(int wellHeadI , int wellHeadJ) {
        if (m_i < 0)
            m_i = wellHeadI;
        
        if (m_j < 0)
            m_j = wellHeadJ;
    }


    int Completion::getI() const {
        return m_i;
    }

    int Completion::getJ() const {
        return m_j;
    }

    int Completion::getK() const {
        return m_k;
    }

    CompletionStateEnum Completion::getState() const {
        return m_state;
    }

    double Completion::getConnectionTransmissibilityFactor() const {
        return m_connectionTransmissibilityFactor.getValue();
    }

    double Completion::getDiameter() const {
        return m_diameter.getValue();
    }
    
    double Completion::getSkinFactor() const {
        return m_skinFactor.getValue();
    }

    CompletionDirection::DirectionEnum Completion::getDirection() const {
        return m_direction;
    }
}


