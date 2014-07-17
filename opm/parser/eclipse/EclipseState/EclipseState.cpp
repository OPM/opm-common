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
        m_unitSystem = deck->getActiveUnitSystem();

        initPhases(deck);
        initEclipseGrid(deck);
        initSchedule(deck);
        initTitle(deck);
        initProperties(deck);
        initTransMult();
        initFaults(deck);
    }
    

    EclipseGridConstPtr EclipseState::getEclipseGrid() const {
        return m_eclipseGrid;
    }


    EclipseGridPtr EclipseState::getEclipseGridCopy() const {
        return std::make_shared<EclipseGrid>( m_eclipseGrid->c_ptr() );
    }


    ScheduleConstPtr EclipseState::getSchedule() const {
        return schedule;
    }

    std::shared_ptr<const FaultCollection> EclipseState::getFaults() const {
        return m_faults;
    }

    std::shared_ptr<const TransMult> EclipseState::getTransMult() const {
        return m_transMult;
    }

    std::string EclipseState::getTitle() const {
        return m_title;
    }

    void EclipseState::initSchedule(DeckConstPtr deck) {
        schedule = ScheduleConstPtr( new Schedule(deck) );
    }

    void EclipseState::initTransMult() {
        EclipseGridConstPtr grid = getEclipseGrid();
        m_transMult = std::make_shared<TransMult>( grid->getNX() , grid->getNY() , grid->getNZ());

        if (hasDoubleGridProperty("MULTX"))
            m_transMult->applyMULT(getDoubleGridProperty("MULTX"), FaceDir::XPlus);
        if (hasDoubleGridProperty("MULTX-"))
            m_transMult->applyMULT(getDoubleGridProperty("MULTX-"), FaceDir::XMinus);

        if (hasDoubleGridProperty("MULTY"))
            m_transMult->applyMULT(getDoubleGridProperty("MULTY"), FaceDir::YPlus);
        if (hasDoubleGridProperty("MULTY-"))
            m_transMult->applyMULT(getDoubleGridProperty("MULTY-"), FaceDir::YMinus);

        if (hasDoubleGridProperty("MULTZ"))
            m_transMult->applyMULT(getDoubleGridProperty("MULTZ"), FaceDir::ZPlus);
        if (hasDoubleGridProperty("MULTZ-"))
            m_transMult->applyMULT(getDoubleGridProperty("MULTZ-"), FaceDir::ZMinus);
    }

    void EclipseState::initFaults(DeckConstPtr deck) {
        EclipseGridConstPtr grid = getEclipseGrid();
        m_faults = std::make_shared<FaultCollection>( grid->getNX() , grid->getNY() , grid->getNZ());
        std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );

        for (size_t index=0; index < gridSection->count("FAULTS"); index++) {
            DeckKeywordConstPtr faultsKeyword = gridSection->getKeyword("FAULTS" , index);
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
                if (!m_faults->hasFault(faultName)) {
                    std::shared_ptr<Fault> fault = std::make_shared<Fault>( faultName );
                    m_faults->addFault( fault );
                }

                {
                    std::shared_ptr<Fault> fault = m_faults->getFault( faultName );
                    fault->addFace( face );
                }
            }
        }
        
        setMULTFLT( gridSection );

        if (Section::hasEDIT(deck)) {
            std::shared_ptr<Opm::EDITSection> editSection(new Opm::EDITSection(deck) );
            setMULTFLT( editSection );
        }

        m_transMult->applyMULTFLT( m_faults );
    }


    void EclipseState::setMULTFLT(std::shared_ptr<const Section> section) const {
        for (size_t index=0; index < section->count("MULTFLT"); index++) {
            DeckKeywordConstPtr faultsKeyword = section->getKeyword("MULTFLT" , index);
            for (auto iter = faultsKeyword->begin(); iter != faultsKeyword->end(); ++iter) {
                DeckRecordConstPtr faultRecord = *iter;
                const std::string& faultName = faultRecord->getItem(0)->getString(0);
                double multFlt = faultRecord->getItem(1)->getRawDouble(0);
                
                m_faults->setTransMult( faultName , multFlt );
            }
        }
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
        return (m_intGridProperties->supportsKeyword( keyword ) || m_doubleGridProperties->supportsKeyword( keyword ));
    }   

    bool EclipseState::hasIntGridProperty(const std::string& keyword) const {
         return m_intGridProperties->hasKeyword( keyword );
    }   

    bool EclipseState::hasDoubleGridProperty(const std::string& keyword) const {
         return m_doubleGridProperties->hasKeyword( keyword );
    }   

    /*
      Observe that this will autocreate a property if it has not been explicitly added. 
    */
    std::shared_ptr<GridProperty<int> > EclipseState::getIntGridProperty( const std::string& keyword ) const {
        return m_intGridProperties->getKeyword( keyword );
    }

    std::shared_ptr<GridProperty<double> > EclipseState::getDoubleGridProperty( const std::string& keyword ) const {
        return m_doubleGridProperties->getKeyword( keyword );
    }

    
    
    void EclipseState::loadGridPropertyFromDeckKeyword(std::shared_ptr<const Box> inputBox , DeckKeywordConstPtr deckKeyword) {            
        const std::string& keyword = deckKeyword->name();
        if (m_intGridProperties->supportsKeyword( keyword )) {
            auto gridProperty = m_intGridProperties->getKeyword( keyword );
            gridProperty->loadFromDeckKeyword( inputBox , deckKeyword );
        } else if (m_doubleGridProperties->supportsKeyword( keyword )) {
            auto gridProperty = m_doubleGridProperties->getKeyword( keyword );
            gridProperty->loadFromDeckKeyword( inputBox , deckKeyword );
        } else {
            throw std::invalid_argument("Tried to load from unsupported keyword: " + deckKeyword->name());
        }
    }
        
    

    void EclipseState::initProperties(DeckConstPtr deck) {
        BoxManager boxManager(m_eclipseGrid->getNX( ) , m_eclipseGrid->getNY() , m_eclipseGrid->getNZ());
        typedef GridProperties<int>::SupportedKeywordInfo SupportedIntKeywordInfo;
        static std::vector<SupportedIntKeywordInfo> supportedIntKeywords =
            {SupportedIntKeywordInfo( "SATNUM" , 0 ),
             SupportedIntKeywordInfo( "PVTNUM" , 0 ),
             SupportedIntKeywordInfo( "EQLNUM" , 0 ),
             SupportedIntKeywordInfo( "IMBNUM" , 0 ),
             // TODO: implement regular expression matching for
             // keyword names?
//             SupportedIntKeywordInfo( "FIP???"  , 0 ),
             SupportedIntKeywordInfo( "FIPNUM" , 0 )};

        // Note that the variants of grid keywords for radial grids
        // are not supported. (and hopefully never will be)
        typedef GridProperties<double>::SupportedKeywordInfo SupportedDoubleKeywordInfo;
        static std::vector<SupportedDoubleKeywordInfo> supportedDoubleKeywords = {
            // keywords to specify the scaled connate gas
            // saturations.
            SupportedDoubleKeywordInfo( "SGL"    , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGL"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SGLX"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SGLX-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGLX"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGLX-" , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SGLY"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SGLY-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGLY"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGLY-" , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SGLZ"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SGLZ-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGLZ"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGLZ-" , 0.0, "1" ),

            // keywords to specify the connate water saturation.
            SupportedDoubleKeywordInfo( "SWL"    , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWL"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SWLX"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SWLX-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWLX"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWLX-" , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SWLY"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SWLY-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWLY"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWLY-" , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SWLZ"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SWLZ-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWLZ"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWLZ-" , 0.0, "1" ),

            // keywords to specify the maximum gas saturation.
            SupportedDoubleKeywordInfo( "SGU"    , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGU"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SGUX"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SGUX-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGUX"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGUX-" , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SGUY"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SGUY-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGUY"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGUY-" , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SGUZ"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SGUZ-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGUZ"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGUZ-" , 0.0, "1" ),

            // keywords to specify the maximum water saturation.
            SupportedDoubleKeywordInfo( "SWU"    , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWU"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SWUX"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SWUX-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWUX"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWUX-" , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SWUY"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SWUY-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWUY"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWUY-" , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SWUZ"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SWUZ-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWUZ"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWUZ-" , 0.0, "1" ),

            // keywords to specify the scaled critical gas
            // saturation.
            SupportedDoubleKeywordInfo( "SGCR"     , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGCR"    , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SGCRX"    , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SGCRX-"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGCRX"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGCRX-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SGCRY"    , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SGCRY-"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGCRY"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGCRY-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SGCRZ"    , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SGCRZ-"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGCRZ"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISGCRZ-"  , 0.0, "1" ),

            // keywords to specify the scaled critical oil-in-water
            // saturation.
            SupportedDoubleKeywordInfo( "SOWCR"    , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCR"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SOWCRX"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SOWCRX-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCRX"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCRX-" , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SOWCRY"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SOWCRY-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCRY"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCRY-" , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SOWCRZ"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SOWCRZ-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCRZ"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCRZ-" , 0.0, "1" ),

            // keywords to specify the scaled critical oil-in-gas
            // saturation.
            SupportedDoubleKeywordInfo( "SOGCR"    , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCR"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SOGCRX"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SOGCRX-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCRX"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCRX-" , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SOGCRY"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SOGCRY-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCRY"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCRY-" , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SOGCRZ"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SOGCRZ-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCRZ"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCRZ-" , 0.0, "1" ),

            // keywords to specify the scaled critical water
            // saturation.
            SupportedDoubleKeywordInfo( "SWCR"     , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWCR"    , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SWCRX"    , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SWCRX-"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWCRX"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWCRX-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SWCRY"    , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SWCRY-"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWCRY"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWCRY-"  , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SWCRZ"    , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "SWCRZ-"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWCRZ"   , 0.0, "1" ),
            SupportedDoubleKeywordInfo( "ISWCRZ-"  , 0.0, "1" ),

            // porosity
            SupportedDoubleKeywordInfo( "PORO"  , 0.0, "1" ),

            // the permeability keywords
            SupportedDoubleKeywordInfo( "PERMX" , 0.0, "Permeability" ),
            SupportedDoubleKeywordInfo( "PERMY" , 0.0, "Permeability" ),
            SupportedDoubleKeywordInfo( "PERMZ" , 0.0, "Permeability" ),
            SupportedDoubleKeywordInfo( "PERMXY", 0.0, "Permeability" ), // E300 only
            SupportedDoubleKeywordInfo( "PERMXZ", 0.0, "Permeability" ), // E300 only
            SupportedDoubleKeywordInfo( "PERMYZ", 0.0, "Permeability" ), // E300 only

            // gross-to-net thickness (acts as a multiplier for PORO
            // and the permeabilities in the X-Y plane as well as for
            // the well rates.)
            SupportedDoubleKeywordInfo( "NTG"   , 1.0, "1" ),

            // transmissibility multipliers
            SupportedDoubleKeywordInfo( "MULTX" , 1.0, "1" ),
            SupportedDoubleKeywordInfo( "MULTY" , 1.0, "1" ),
            SupportedDoubleKeywordInfo( "MULTZ" , 1.0, "1" ),
            SupportedDoubleKeywordInfo( "MULTX-", 1.0, "1" ),
            SupportedDoubleKeywordInfo( "MULTY-", 1.0, "1" ),
            SupportedDoubleKeywordInfo( "MULTZ-", 1.0, "1" ),
            
            // initialisation
            SupportedDoubleKeywordInfo( "SWATINIT" , 0.0, "1"),

            // pore volume multipliers
            SupportedDoubleKeywordInfo( "MULTPV", 1.0, "1" )
        };

        m_intGridProperties = std::make_shared<GridProperties<int> >(m_eclipseGrid->getNX() , m_eclipseGrid->getNY() , m_eclipseGrid->getNZ() , supportedIntKeywords); 
        m_doubleGridProperties.reset(new GridProperties<double>(m_eclipseGrid->getNX() , m_eclipseGrid->getNY() , m_eclipseGrid->getNZ() , supportedDoubleKeywords)); 
        
        
        if (Section::hasGRID(deck)) {
            std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
            scanSection( gridSection , boxManager );
        }


        if (Section::hasEDIT(deck)) {
            std::shared_ptr<Opm::EDITSection> editSection(new Opm::EDITSection(deck) );
            scanSection( editSection , boxManager );
        }

        if (Section::hasPROPS(deck)) {
            std::shared_ptr<Opm::PROPSSection> propsSection(new Opm::PROPSSection(deck) );
            scanSection( propsSection , boxManager );
        }

        if (Section::hasREGIONS(deck)) {
            std::shared_ptr<Opm::REGIONSSection> regionsSection(new Opm::REGIONSSection(deck) );
            scanSection( regionsSection , boxManager );
        }

        if (Section::hasSOLUTION(deck)) {
            std::shared_ptr<Opm::SOLUTIONSection> solutionSection(new Opm::SOLUTIONSection(deck) );
            scanSection( solutionSection , boxManager );
        }
    }

    double EclipseState::getSIScaling(const std::string &dimensionString) const
    {
        return m_unitSystem->getDimension(dimensionString)->getSIScaling();
    }

    void EclipseState::scanSection(std::shared_ptr<Opm::Section> section , BoxManager& boxManager) {
        for (auto iter = section->begin(); iter != section->end(); ++iter) {
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
                handleENDBOXKeyword(boxManager);
            
            if (deckKeyword->name() == "MULTIPLY")
                handleMULTIPLYKeyword(deckKeyword , boxManager);
            
            boxManager.endKeyword();
        }
        boxManager.endSection();
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


    void EclipseState::handleENDBOXKeyword(BoxManager& boxManager) {
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
            } else if (m_doubleGridProperties->hasKeyword( field )) {
                std::shared_ptr<GridProperty<double> > property = m_doubleGridProperties->getKeyword( field );
                
                property->scale( scaleFactor , boxManager.getActiveBox() );
            } else
                throw std::invalid_argument("Fatal error processing MULTIPLY keyword. Tried to multiply not defined keyword " + field);

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
            } else if (m_doubleGridProperties->hasKeyword( field )) {
                std::shared_ptr<GridProperty<double> > property = m_doubleGridProperties->getKeyword( field );

                double siShiftValue = shiftValue * getSIScaling(property->getKeywordInfo().getDimensionString());
                property->add(siShiftValue , boxManager.getActiveBox() );
            } else
                throw std::invalid_argument("Fatal error processing ADD keyword. Tried to shift not defined keyword " + field);

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
            } else if (m_doubleGridProperties->supportsKeyword( field )) {
                std::shared_ptr<GridProperty<double> > property = m_doubleGridProperties->getKeyword( field );

                double siValue = value * getSIScaling(property->getKeywordInfo().getDimensionString());
                property->setScalar( siValue , boxManager.getActiveBox() );
            } else
                throw std::invalid_argument("Fatal error processing EQUALS keyword. Tried to set not defined keyword " + field);

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
            else if (m_doubleGridProperties->hasKeyword( srcField ))
                copyDoubleKeyword( srcField , targetField , boxManager.getActiveBox());
            else
                throw std::invalid_argument("Fatal error processing COPY keyword. Tried to copy from not defined keyword " + srcField);
            
        }
    }

    
    void EclipseState::copyIntKeyword(const std::string& srcField , const std::string& targetField , std::shared_ptr<const Box> inputBox) {
        std::shared_ptr<const GridProperty<int> > src = m_intGridProperties->getKeyword( srcField );
        std::shared_ptr<GridProperty<int> > target    = m_intGridProperties->getKeyword( targetField );

        target->copyFrom( *src , inputBox );
    }


    void EclipseState::copyDoubleKeyword(const std::string& srcField , const std::string& targetField , std::shared_ptr<const Box> inputBox) {
        std::shared_ptr<const GridProperty<double> > src = m_doubleGridProperties->getKeyword( srcField );
        std::shared_ptr<GridProperty<double> > target    = m_doubleGridProperties->getKeyword( targetField );
        
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
