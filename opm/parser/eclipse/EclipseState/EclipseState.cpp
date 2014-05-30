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

#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Box.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/BoxManager.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>

#include <iostream>
#include <boost/algorithm/string/join.hpp>

namespace Opm {
    
    EclipseState::EclipseState(DeckConstPtr deck) 
    {
        initPhases(deck);
        initEclipseGrid(deck);
        initSchedule(deck);
        initTitle(deck);
        initProperties(deck);
    }
    

    EclipseGridConstPtr EclipseState::getEclipseGrid() const {
        return m_eclipseGrid;
    }


    ScheduleConstPtr EclipseState::getSchedule() const {
        return schedule;
    }


    std::string EclipseState::getTitle() const {
        return m_title;
    }

    void EclipseState::initSchedule(DeckConstPtr deck) {
        schedule = ScheduleConstPtr( new Schedule(deck) );
    }

    void EclipseState::initEclipseGrid(DeckConstPtr deck) {
        std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
        std::shared_ptr<Opm::RUNSPECSection> runspecSection(new Opm::RUNSPECSection(deck) );
        
        m_eclipseGrid = EclipseGridConstPtr( new EclipseGrid( runspecSection , gridSection ));
    }


    void EclipseState::initPhases(DeckConstPtr deck) {
        if (deck->hasKeyword("OIL"))
            phases.insert(Phase::PhaseEnum::OIL);

        if (deck->hasKeyword("GAS"))
            phases.insert(Phase::PhaseEnum::GAS);

        if (deck->hasKeyword("WATER"))
            phases.insert(Phase::PhaseEnum::WATER);
    }


    bool EclipseState::hasPhase(enum Phase::PhaseEnum phase) const {
         return (phases.count(phase) == 1);
    }

    void EclipseState::initTitle(DeckConstPtr deck){
        if (deck->hasKeyword("TITLE")) {
            DeckKeywordConstPtr titleKeyword = deck->getKeyword("TITLE");
            DeckRecordConstPtr record = titleKeyword->getRecord(0);
            DeckItemPtr item = record->getItem(0);
            std::vector<std::string> itemValue = item->getStringData();
            m_title = boost::algorithm::join(itemValue, " ");
        }
    }

    bool EclipseState::supportsGridProperty(const std::string& keyword) const {
         return m_intGridProperties->supportsKeyword( keyword );
    }   

    bool EclipseState::hasIntGridProperty(const std::string& keyword) const {
         return m_intGridProperties->hasKeyword( keyword );
    }   

    /*
      Observe that this will autocreate a property if it has not been explicitly added. 
    */
    std::shared_ptr<GridProperty<int> > EclipseState::getIntProperty( const std::string& keyword ) {
        return m_intGridProperties->getKeyword( keyword );
    }

    
    void EclipseState::loadGridPropertyFromDeckKeyword(std::shared_ptr<const Box> inputBox , DeckKeywordConstPtr deckKeyword) {            
        const std::string& keyword = deckKeyword->name();
        auto gridProperty = m_intGridProperties->getKeyword( keyword );
        gridProperty->loadFromDeckKeyword( inputBox , deckKeyword );
    }
    
    

    void EclipseState::initProperties(DeckConstPtr deck) {
        BoxManager boxManager(m_eclipseGrid->getNX( ) , m_eclipseGrid->getNY() , m_eclipseGrid->getNZ());
        std::vector<std::pair<std::string , int> > supportedIntKeywords = {{ "SATNUM" , 0 },
                                                                           { "PVTNUM" , 0 },
                                                                           { "EQLNUM" , 0 },
                                                                           { "FIPNUM" , 0 }};
        m_intGridProperties = std::make_shared<GridProperties<int> >(m_eclipseGrid->getNX() , m_eclipseGrid->getNY() , m_eclipseGrid->getNZ() , supportedIntKeywords); 
        
        if (Section::hasREGIONS(deck)) {
            std::shared_ptr<Opm::REGIONSSection> regionsSection(new Opm::REGIONSSection(deck) );
            
            for (auto iter = regionsSection->begin(); iter != regionsSection->end(); ++iter) {
                DeckKeywordConstPtr deckKeyword = *iter;
                
                if (supportsGridProperty( deckKeyword->name()) )
                    loadGridPropertyFromDeckKeyword( boxManager.getActiveBox() , deckKeyword );

                if (deckKeyword->name() == "ADD")
                    handleADDKeyword(deckKeyword , boxManager);

                if (deckKeyword->name() == "BOX")
                    handleBOXKeyword(deckKeyword , boxManager);
                
                if (deckKeyword->name() == "COPY")
                    handleCOPYKeyword(deckKeyword , boxManager);

                if (deckKeyword->name() == "EQUALS")
                    handleEQUALSKeyword(deckKeyword , boxManager);
                
                if (deckKeyword->name() == "ENDBOX")
                    handleENDBOXKeyword(deckKeyword , boxManager);

                if (deckKeyword->name() == "MULTIPLY")
                    handleMULTIPLYKeyword(deckKeyword , boxManager);

                std::cout << "Looking at kw: " << deckKeyword->name() << std::endl;
                boxManager.endKeyword();
            }
        }
    }


    void EclipseState::handleBOXKeyword(DeckKeywordConstPtr deckKeyword , BoxManager& boxManager) {
        DeckRecordConstPtr record = deckKeyword->getRecord(0);
        int I1 = record->getItem("I1")->getInt(0) - 1;
        int I2 = record->getItem("I2")->getInt(0) - 1;
        int J1 = record->getItem("J1")->getInt(0) - 1;
        int J2 = record->getItem("J2")->getInt(0) - 1;
        int K1 = record->getItem("K1")->getInt(0) - 1;
        int K2 = record->getItem("K2")->getInt(0) - 1;
        
        boxManager.setInputBox( I1 , I2 , J1 , J2 , K1 , K2 );
    }


    void EclipseState::handleENDBOXKeyword(DeckKeywordConstPtr deckKeyword , BoxManager& boxManager) {
        boxManager.endInputBox();
    }


    void EclipseState::handleMULTIPLYKeyword(DeckKeywordConstPtr deckKeyword , BoxManager& boxManager) {
        for (auto iter = deckKeyword->begin(); iter != deckKeyword->end(); ++iter) {
            DeckRecordConstPtr record = *iter;
            const std::string& field = record->getItem("field")->getString(0);
            double      scaleFactor  = record->getItem("factor")->getRawDouble(0);
            
            setKeywordBox( record , boxManager );
            
            if (m_intGridProperties->hasKeyword( field )) {
                int intFactor = static_cast<int>(scaleFactor);
                std::shared_ptr<GridProperty<int> > property = m_intGridProperties->getKeyword( field );

                property->scale( intFactor , boxManager.getActiveBox() );
            } else
                throw std::invalid_argument("Fatal error processing MULTIPLY keyword. Tried to multiply not defined keyword" + field);

        }
    }


    /*
      The fine print of the manual says the ADD keyword should support
      some state dependent semantics regarding endpoint scaling arrays
      in the PROPS section. That is not supported. 
    */
    
    void EclipseState::handleADDKeyword(DeckKeywordConstPtr deckKeyword , BoxManager& boxManager) {
        for (auto iter = deckKeyword->begin(); iter != deckKeyword->end(); ++iter) {
            DeckRecordConstPtr record = *iter;
            const std::string& field = record->getItem("field")->getString(0);
            double      shiftValue  = record->getItem("shift")->getRawDouble(0);
            
            setKeywordBox( record , boxManager );
            
            if (m_intGridProperties->hasKeyword( field )) {
                int intShift = static_cast<int>(shiftValue);
                std::shared_ptr<GridProperty<int> > property = m_intGridProperties->getKeyword( field );
                
                property->add( intShift , boxManager.getActiveBox() );
            } else
                throw std::invalid_argument("Fatal error processing ADD keyword. Tried to multiply not defined keyword" + field);

        }
    }


    void EclipseState::handleEQUALSKeyword(DeckKeywordConstPtr deckKeyword , BoxManager& boxManager) {
        for (auto iter = deckKeyword->begin(); iter != deckKeyword->end(); ++iter) {
            DeckRecordConstPtr record = *iter;
            const std::string& field = record->getItem("field")->getString(0);
            double      value  = record->getItem("value")->getRawDouble(0);
            
            setKeywordBox( record , boxManager );
            
            if (m_intGridProperties->supportsKeyword( field )) {
                int intValue = static_cast<int>(value);
                std::shared_ptr<GridProperty<int> > property = m_intGridProperties->getKeyword( field );

                property->setScalar( intValue , boxManager.getActiveBox() );
            } else
                throw std::invalid_argument("Fatal error processing EQUALS keyword. Tried to set not defined keyword" + field);

        }
    }



    void EclipseState::handleCOPYKeyword(DeckKeywordConstPtr deckKeyword , BoxManager& boxManager) {
        for (auto iter = deckKeyword->begin(); iter != deckKeyword->end(); ++iter) {
            DeckRecordConstPtr record = *iter;
            const std::string& srcField = record->getItem("src")->getString(0);
            const std::string& targetField = record->getItem("target")->getString(0);
            
            setKeywordBox( record , boxManager );
            
            if (m_intGridProperties->hasKeyword( srcField ))
                copyIntKeyword( srcField , targetField , boxManager.getActiveBox());
            else
                throw std::invalid_argument("Fatal error processing COPY keyword. Tried to copy from not defined keyword" + srcField);
            
        }
    }

    
    void EclipseState::copyIntKeyword(const std::string& srcField , const std::string& targetField , std::shared_ptr<const Box> inputBox) {
        std::shared_ptr<const GridProperty<int> > src = m_intGridProperties->getKeyword( srcField );
        std::shared_ptr<GridProperty<int> > target    = m_intGridProperties->getKeyword( targetField );
        
        target->copyFrom( *src , inputBox );
    }



    void EclipseState::setKeywordBox(DeckRecordConstPtr deckRecord , BoxManager& boxManager) {
        DeckItemConstPtr I1Item = deckRecord->getItem("I1");
        DeckItemConstPtr I2Item = deckRecord->getItem("I2");
        DeckItemConstPtr J1Item = deckRecord->getItem("J1");
        DeckItemConstPtr J2Item = deckRecord->getItem("J2");
        DeckItemConstPtr K1Item = deckRecord->getItem("K1");
        DeckItemConstPtr K2Item = deckRecord->getItem("K2");

        size_t defaultCount = 0;
        
        if (I1Item->defaultApplied())
            defaultCount++;

        if (I2Item->defaultApplied())
            defaultCount++;

        if (J1Item->defaultApplied())
            defaultCount++;

        if (J2Item->defaultApplied())
            defaultCount++;

        if (K1Item->defaultApplied())
            defaultCount++;

        if (K2Item->defaultApplied())
            defaultCount++;

        if (defaultCount == 0) {
            boxManager.setKeywordBox( I1Item->getInt(0) - 1,
                                      I2Item->getInt(0) - 1,
                                      J1Item->getInt(0) - 1,
                                      J2Item->getInt(0) - 1,
                                      K1Item->getInt(0) - 1,
                                      K2Item->getInt(0) - 1);
        } else if (defaultCount != 6)
            throw std::invalid_argument("When using BOX modifiers on keywords you must specify the BOX completely.");
    }

}
