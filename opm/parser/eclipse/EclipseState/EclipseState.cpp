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
#include <opm/parser/eclipse/EclipseState/Grid/MULTREGTScanner.hpp>

#include <opm/parser/eclipse/Parser/ParserLog.hpp>

#include <iostream>
#include <sstream>
#include <boost/algorithm/string/join.hpp>

namespace Opm {

    namespace GridPropertyPostProcessor {

        class DistributeTopLayer : public GridPropertyBasePostProcessor<double>
        {
        public:
            DistributeTopLayer(const EclipseState& eclipseState) : 
                m_eclipseState( eclipseState )
            { }
            
            
            void apply(std::vector<double>& values) const {
                EclipseGridConstPtr grid = m_eclipseState.getEclipseGrid();
                size_t layerSize = grid->getNX() * grid->getNY();
                size_t gridSize  = grid->getCartesianSize();
                
                for (size_t globalIndex = layerSize; globalIndex < gridSize; globalIndex++) {
                    if (std::isnan( values[ globalIndex ] ))
                        values[globalIndex] = values[globalIndex - layerSize];
                }
            }
            
            
        private:
            const EclipseState& m_eclipseState;
        };

        /*-----------------------------------------------------------------*/

        class InitPORV : public GridPropertyBasePostProcessor<double>
        {
        public:
            InitPORV(const EclipseState& eclipseState) : 
                m_eclipseState( eclipseState )
            { }
            
            
            void apply(std::vector<double>& ) const {
                EclipseGridConstPtr grid = m_eclipseState.getEclipseGrid();
                /*
                  Observe that this apply method does not alter the
                  values input vector, instead it fetches the PORV
                  property one more time, and then manipulates that.
                */
                auto porv = m_eclipseState.getDoubleGridProperty("PORV");   
                if (porv->containsNaN()) {
                    auto poro = m_eclipseState.getDoubleGridProperty("PORO");
                    auto ntg = m_eclipseState.getDoubleGridProperty("NTG");   
                    if (poro->containsNaN())
                        throw std::logic_error("Do not have information for the PORV keyword - some defaulted values in PORO");
                    {
                        for (size_t globalIndex = 0; globalIndex < porv->getCartesianSize(); globalIndex++) {
                            if (std::isnan(porv->iget(globalIndex))) {
                                double cell_poro = poro->iget(globalIndex);
                                double cell_ntg = ntg->iget(globalIndex);
                                double cell_volume = grid->getCellVolume(globalIndex);
                                porv->iset( globalIndex , cell_poro * cell_volume * cell_ntg);
                            }
                        }
                    } 
                }
                
                if (m_eclipseState.hasDoubleGridProperty("MULTPV")) {
                    auto multpv = m_eclipseState.getDoubleGridProperty("MULTPV");   
                    porv->multiplyWith( *multpv );
                }
            }
            
            
        private:
            const EclipseState& m_eclipseState;
        };

        
    }


    EclipseState::EclipseState(DeckConstPtr deck, ParserLogPtr parserLog)    
    {
        m_deckUnitSystem = deck->getActiveUnitSystem();

        initPhases(deck, parserLog);
        initTables(deck, parserLog);
        initSchedule(deck, parserLog);
        initTitle(deck, parserLog);

        initEclipseGrid(deck, parserLog);

        initProperties(deck, parserLog);
        initTransMult(parserLog);
        initFaults(deck, parserLog);
        initMULTREGT(deck, parserLog);
    }

    std::shared_ptr<const UnitSystem> EclipseState::getDeckUnitSystem() const {
        return m_deckUnitSystem;
    }

    bool EclipseState::hasEclipseGrid() const {
        return static_cast<bool>(m_eclipseGrid);
    }

    EclipseGridConstPtr EclipseState::getEclipseGrid() const {
        if (!hasEclipseGrid())
            throw std::logic_error("The eclipse grid object cannot be retrieved if no grid is featured by the deck.");

        return m_eclipseGrid;
    }

    EclipseGridPtr EclipseState::getEclipseGridCopy() const {
        return std::make_shared<EclipseGrid>( m_eclipseGrid->c_ptr() );
    }

    const std::vector<EnkrvdTable>& EclipseState::getEnkrvdTables() const {
        return m_enkrvdTables;
    }

    const std::vector<EnptvdTable>& EclipseState::getEnptvdTables() const {
        return m_enptvdTables;
    }

    const std::vector<ImkrvdTable>& EclipseState::getImkrvdTables() const {
        return m_imkrvdTables;
    }

    const std::vector<ImptvdTable>& EclipseState::getImptvdTables() const {
        return m_imptvdTables;
    }

    const std::vector<PlyadsTable>& EclipseState::getPlyadsTables() const {
        return m_plyadsTables;
    }

    const std::vector<PlymaxTable>& EclipseState::getPlymaxTables() const {
        return m_plymaxTables;
    }

    const std::vector<PlyrockTable>& EclipseState::getPlyrockTables() const {
        return m_plyrockTables;
    }

    const std::vector<PlyviscTable>& EclipseState::getPlyviscTables() const {
        return m_plyviscTables;
    }

    const std::vector<PvdgTable>& EclipseState::getPvdgTables() const {
        return m_pvdgTables;
    }

    const std::vector<PvdoTable>& EclipseState::getPvdoTables() const {
        return m_pvdoTables;
    }

    const std::vector<PvtgTable>& EclipseState::getPvtgTables() const {
        return m_pvtgTables;
    }

    const std::vector<PvtoTable>& EclipseState::getPvtoTables() const {
        return m_pvtoTables;
    }

    const std::vector<RocktabTable>& EclipseState::getRocktabTables() const {
        return m_rocktabTables;
    }

    const std::vector<RsvdTable>& EclipseState::getRsvdTables() const {
        return m_rsvdTables;
    }

    const std::vector<RvvdTable>& EclipseState::getRvvdTables() const {
        return m_rvvdTables;
    }

    const std::vector<SgofTable>& EclipseState::getSgofTables() const {
        return m_sgofTables;
    }

    const std::vector<SwofTable>& EclipseState::getSwofTables() const {
        return m_swofTables;
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

    void EclipseState::initTables(DeckConstPtr deck, ParserLogPtr parserLog) {
        initSimpleTables(deck, parserLog, "ENKRVD", m_enkrvdTables);
        initSimpleTables(deck, parserLog, "ENPTVD", m_enptvdTables);
        initSimpleTables(deck, parserLog, "IMKRVD", m_imkrvdTables);
        initSimpleTables(deck, parserLog, "IMPTVD", m_imptvdTables);
        initSimpleTables(deck, parserLog, "PLYADS", m_plyadsTables);
        initSimpleTables(deck, parserLog, "PLYMAX", m_plymaxTables);
        initSimpleTables(deck, parserLog, "PLYROCK", m_plyrockTables);
        initSimpleTables(deck, parserLog, "PLYVISC", m_plyviscTables);
        initSimpleTables(deck, parserLog, "PVDG", m_pvdgTables);
        initSimpleTables(deck, parserLog, "PVDO", m_pvdoTables);
        initSimpleTables(deck, parserLog, "RSVD", m_rsvdTables);
        initSimpleTables(deck, parserLog, "RVVD", m_rvvdTables);
        initSimpleTables(deck, parserLog, "SGOF", m_sgofTables);
        initSimpleTables(deck, parserLog, "SWOF", m_swofTables);

        // the ROCKTAB table comes with additional fun because the number of columns
        //depends on the presence of the RKTRMDIR keyword...
        initRocktabTables(deck, parserLog);

        initFullTables(deck, parserLog, "PVTG", m_pvtgTables);
        initFullTables(deck, parserLog, "PVTO", m_pvtoTables);
   }

    void EclipseState::initSchedule(DeckConstPtr deck, ParserLogPtr parserLog) {
        schedule = ScheduleConstPtr( new Schedule(deck, parserLog) );
    }

    void EclipseState::initTransMult(ParserLogPtr /*parserLog*/) {
        if (!hasEclipseGrid())
            // no checking required here as the class will already
            // refrain from processing the MULT* grid properties if no
            // grid is available...
            return;

        EclipseGridConstPtr grid = getEclipseGrid();
        m_transMult = std::make_shared<TransMult>( grid->getNX() , grid->getNY() , grid->getNZ());

        if (hasDoubleGridProperty("MULTX"))
            m_transMult->applyMULT(m_doubleGridProperties->getKeyword("MULTX"), FaceDir::XPlus);
        if (hasDoubleGridProperty("MULTX-"))
            m_transMult->applyMULT(m_doubleGridProperties->getKeyword("MULTX-"), FaceDir::XMinus);

        if (hasDoubleGridProperty("MULTY"))
            m_transMult->applyMULT(m_doubleGridProperties->getKeyword("MULTY"), FaceDir::YPlus);
        if (hasDoubleGridProperty("MULTY-"))
            m_transMult->applyMULT(m_doubleGridProperties->getKeyword("MULTY-"), FaceDir::YMinus);

        if (hasDoubleGridProperty("MULTZ"))
            m_transMult->applyMULT(m_doubleGridProperties->getKeyword("MULTZ"), FaceDir::ZPlus);
        if (hasDoubleGridProperty("MULTZ-"))
            m_transMult->applyMULT(m_doubleGridProperties->getKeyword("MULTZ-"), FaceDir::ZMinus);
    }

    void EclipseState::initFaults(DeckConstPtr deck, ParserLogPtr parserLog) {
        if (!hasEclipseGrid()) {
            if (deck->hasKeyword("FAULTS") ||
                deck->hasKeyword("MULTFAULT"))
            {
                throw std::logic_error("Grid could not be initialized, but fault transmissibility multipliers have been detected.");
            }
            else
                return;
        }

        EclipseGridConstPtr grid = getEclipseGrid();
        m_faults = std::make_shared<FaultCollection>();
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
        
        setMULTFLT(gridSection, parserLog);

        if (Section::hasEDIT(deck)) {
            std::shared_ptr<Opm::EDITSection> editSection(new Opm::EDITSection(deck) );
            setMULTFLT(editSection, parserLog);
        }

        m_transMult->applyMULTFLT( m_faults );
    }



    void EclipseState::setMULTFLT(std::shared_ptr<const Section> section, ParserLogPtr /*parserLog*/) const {
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


    
    void EclipseState::initMULTREGT(DeckConstPtr deck, ParserLogPtr /*parserLog*/) {
        if (!hasEclipseGrid()) {
            if (deck->hasKeyword("MULTREGT")) {
                throw std::logic_error("Grid could not be initialized, but region transmissibility multipliers have been detected.");
            }
            else
                return;
        }

        EclipseGridConstPtr grid = getEclipseGrid();
        std::shared_ptr<MULTREGTScanner> scanner = std::make_shared<MULTREGTScanner>();

        {
            std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
            for (size_t index=0; index < gridSection->count("MULTREGT"); index++) {
                DeckKeywordConstPtr multregtKeyword = gridSection->getKeyword("MULTREGT" , index);
                scanner->addKeyword( multregtKeyword );
            }
        }            


        if (Section::hasEDIT(deck)) {
            std::shared_ptr<Opm::EDITSection> editSection(new Opm::EDITSection(deck) );
            for (size_t index=0; index < editSection->count("MULTREGT"); index++) {
                DeckKeywordConstPtr multregtKeyword = editSection->getKeyword("MULTREGT" , index);
                scanner->addKeyword( multregtKeyword );
            }
        }

        m_transMult->applyMULTREGT( scanner , m_intGridProperties);
    }
    


    void EclipseState::initEclipseGrid(DeckConstPtr deck, ParserLogPtr parserLog) {
        try {
            m_eclipseGrid = EclipseGridConstPtr( new EclipseGrid(deck, parserLog));
        } catch (const std::exception& e) {
            std::string msg("Could not create a grid: "+std::string(e.what()));
            parserLog->addWarning("", -1, msg);
        }
    }


    void EclipseState::initPhases(DeckConstPtr deck, ParserLogPtr parserLog) {
        if (deck->hasKeyword("OIL"))
            phases.insert(Phase::PhaseEnum::OIL);

        if (deck->hasKeyword("GAS"))
            phases.insert(Phase::PhaseEnum::GAS);

        if (deck->hasKeyword("WATER"))
            phases.insert(Phase::PhaseEnum::WATER);

        if (phases.size() < 3)
            parserLog->addNote("", -1, "Only " + std::to_string(static_cast<long long>(phases.size())) + " fluid phases are enabled");
    }


    bool EclipseState::hasPhase(enum Phase::PhaseEnum phase) const {
         return (phases.count(phase) == 1);
    }

    void EclipseState::initTitle(DeckConstPtr deck, ParserLogPtr /*parserLog*/){
        if (deck->hasKeyword("TITLE")) {
            DeckKeywordConstPtr titleKeyword = deck->getKeyword("TITLE");
            DeckRecordConstPtr record = titleKeyword->getRecord(0);
            DeckItemPtr item = record->getItem(0);
            std::vector<std::string> itemValue = item->getStringData();
            m_title = boost::algorithm::join(itemValue, " ");
        }
    }

    void EclipseState::initRocktabTables(DeckConstPtr deck, ParserLogPtr parserLog) {
        if (!deck->hasKeyword("ROCKTAB"))
            return; // ROCKTAB is not featured by the deck...

        if (deck->numKeywords("ROCKTAB") > 1) {
            complainAboutAmbiguousKeyword(deck, parserLog, "ROCKTAB");
            return;
        }

        const auto rocktabKeyword = deck->getKeyword("ROCKTAB");

        bool isDirectional = deck->hasKeyword("RKTRMDIR");
        bool useStressOption = false;
        if (deck->hasKeyword("ROCKOPTS")) {
            const auto rockoptsKeyword = deck->getKeyword("ROCKOPTS");
            useStressOption = (rockoptsKeyword->getRecord(0)->getItem("METHOD")->getTrimmedString(0) == "STRESS");
        }

        for (size_t tableIdx = 0; tableIdx < rocktabKeyword->size(); ++tableIdx) {
            if (rocktabKeyword->getRecord(tableIdx)->getItem(0)->size() == 0) {
                // for ROCKTAB tables, an empty record indicates that the previous table
                // should be copied...
                if (tableIdx == 0)
                    throw std::invalid_argument("The first table for keyword ROCKTAB"
                                                " must be explicitly defined!");
                m_rocktabTables[tableIdx] = m_rocktabTables[tableIdx - 1];
                continue;
            }

            m_rocktabTables.push_back(RocktabTable());
            m_rocktabTables[tableIdx].init(rocktabKeyword,
                                           isDirectional,
                                           useStressOption,
                                           tableIdx);
        }
    }

    bool EclipseState::supportsGridProperty(const std::string& keyword, int enabledTypes) const {
        bool result = false;
        if (enabledTypes & IntProperties)
            result = result || m_intGridProperties->supportsKeyword( keyword );
        if (enabledTypes & DoubleProperties)
            result = result || m_doubleGridProperties->supportsKeyword( keyword );
        return result;
    }

    bool EclipseState::hasIntGridProperty(const std::string& keyword) const {
         return m_intGridProperties->hasKeyword( keyword );
    }   

    bool EclipseState::hasDoubleGridProperty(const std::string& keyword) const {
         return m_doubleGridProperties->hasKeyword( keyword );
    }   


    /*
      1. The public methods getIntGridProperty & getDoubleGridProperty
         will invoke and run the property post processor (if any is
         registered); the post processor will only run one time. 

         It is important that post processor is not run prematurely,
         internal functions in EclipseState should therefor ask for
         properties by invoking the getKeyword() method of the
         m_intGridProperties / m_doubleGridProperties() directly and
         not through these methods.

      2. Observe that this will autocreate a property if it has not
         been explicitly added.
    */

    std::shared_ptr<GridProperty<int> > EclipseState::getIntGridProperty( const std::string& keyword ) const {
        return m_intGridProperties->getKeyword( keyword );
    }

    std::shared_ptr<GridProperty<double> > EclipseState::getDoubleGridProperty( const std::string& keyword ) const {
        auto gridProperty = m_doubleGridProperties->getKeyword( keyword );
        if (gridProperty->postProcessorRunRequired())
            gridProperty->runPostProcessor();

        return gridProperty;
    }


    /*
       Due to the post processor which might be applied to the
       GridProperty objects it is essential that this method use the
       m_intGridProperties / m_doubleGridProperties fields directly
       and *NOT* use the public methods getIntGridProperty /
       getDoubleGridProperty.
    */

    void EclipseState::loadGridPropertyFromDeckKeyword(std::shared_ptr<const Box> inputBox,
                                                       DeckKeywordConstPtr deckKeyword,
                                                       ParserLogPtr parserLog,
                                                       int enabledTypes) {
        const std::string& keyword = deckKeyword->name();
        if (m_intGridProperties->supportsKeyword( keyword )) {
            if (enabledTypes & IntProperties) {
                auto gridProperty = m_intGridProperties->getKeyword( keyword );
                gridProperty->loadFromDeckKeyword( inputBox , deckKeyword );
            }
        } else if (m_doubleGridProperties->supportsKeyword( keyword )) {
            if (enabledTypes & DoubleProperties) {
                auto gridProperty = m_doubleGridProperties->getKeyword( keyword );
                gridProperty->loadFromDeckKeyword( inputBox , deckKeyword );
            }
        } else {
            parserLog->addError(deckKeyword->getFileName(),
                                deckKeyword->getLineNumber(),
                                "Tried to load unsupported grid property from keyword: " + deckKeyword->name());
        }
    }
        
    
    void EclipseState::initProperties(DeckConstPtr deck, ParserLogPtr parserLog) {
        typedef GridProperties<int>::SupportedKeywordInfo SupportedIntKeywordInfo;
        std::shared_ptr<std::vector<SupportedIntKeywordInfo> > supportedIntKeywords(new std::vector<SupportedIntKeywordInfo>{
            SupportedIntKeywordInfo( "SATNUM" , 1, "1" ),
            SupportedIntKeywordInfo( "IMBNUM" , 1, "1" ),
            SupportedIntKeywordInfo( "PVTNUM" , 1, "1" ),
            SupportedIntKeywordInfo( "EQLNUM" , 1, "1" ),
            SupportedIntKeywordInfo( "ENDNUM" , 1, "1" ),
            SupportedIntKeywordInfo( "FLUXNUM" , 1 , "1" ),
            SupportedIntKeywordInfo( "MULTNUM", 1 , "1" ),
            SupportedIntKeywordInfo( "FIPNUM" , 1, "1" )
            });

        double nan = std::numeric_limits<double>::quiet_NaN();
        const auto eptLookup = std::make_shared<GridPropertyEndpointTableLookupInitializer<>>(*deck, *this);
        const auto distributeTopLayer = std::make_shared<const GridPropertyPostProcessor::DistributeTopLayer>(*this);
        const auto initPORV = std::make_shared<GridPropertyPostProcessor::InitPORV>(*this);


        // Note that the variants of grid keywords for radial grids
        // are not supported. (and hopefully never will be)
        typedef GridProperties<double>::SupportedKeywordInfo SupportedDoubleKeywordInfo;
        std::shared_ptr<std::vector<SupportedDoubleKeywordInfo> > supportedDoubleKeywords(new std::vector<SupportedDoubleKeywordInfo>{
            // keywords to specify the scaled connate gas
            // saturations.
            SupportedDoubleKeywordInfo( "SGL"    , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGL"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGLX"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGLX-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGLX"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGLX-" , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGLY"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGLY-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGLY"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGLY-" , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGLZ"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGLZ-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGLZ"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGLZ-" , eptLookup, "1" ),

            // keywords to specify the connate water saturation.
            SupportedDoubleKeywordInfo( "SWL"    , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWL"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWLX"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWLX-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWLX"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWLX-" , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWLY"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWLY-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWLY"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWLY-" , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWLZ"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWLZ-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWLZ"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWLZ-" , eptLookup, "1" ),

            // keywords to specify the maximum gas saturation.
            SupportedDoubleKeywordInfo( "SGU"    , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGU"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGUX"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGUX-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGUX"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGUX-" , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGUY"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGUY-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGUY"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGUY-" , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGUZ"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGUZ-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGUZ"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGUZ-" , eptLookup, "1" ),

            // keywords to specify the maximum water saturation.
            SupportedDoubleKeywordInfo( "SWU"    , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWU"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWUX"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWUX-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWUX"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWUX-" , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWUY"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWUY-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWUY"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWUY-" , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWUZ"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWUZ-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWUZ"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWUZ-" , eptLookup, "1" ),

            // keywords to specify the scaled critical gas
            // saturation.
            SupportedDoubleKeywordInfo( "SGCR"     , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGCR"    , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGCRX"    , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGCRX-"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGCRX"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGCRX-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGCRY"    , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGCRY-"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGCRY"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGCRY-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGCRZ"    , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGCRZ-"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGCRZ"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGCRZ-"  , eptLookup, "1" ),

            // keywords to specify the scaled critical oil-in-water
            // saturation.
            SupportedDoubleKeywordInfo( "SOWCR"    , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCR"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOWCRX"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOWCRX-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCRX"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCRX-" , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOWCRY"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOWCRY-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCRY"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCRY-" , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOWCRZ"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOWCRZ-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCRZ"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCRZ-" , eptLookup, "1" ),

            // keywords to specify the scaled critical oil-in-gas
            // saturation.
            SupportedDoubleKeywordInfo( "SOGCR"    , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCR"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOGCRX"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOGCRX-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCRX"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCRX-" , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOGCRY"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOGCRY-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCRY"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCRY-" , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOGCRZ"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOGCRZ-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCRZ"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCRZ-" , eptLookup, "1" ),

            // keywords to specify the scaled critical water
            // saturation.
            SupportedDoubleKeywordInfo( "SWCR"     , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWCR"    , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWCRX"    , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWCRX-"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWCRX"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWCRX-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWCRY"    , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWCRY-"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWCRY"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWCRY-"  , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWCRZ"    , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWCRZ-"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWCRZ"   , eptLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWCRZ-"  , eptLookup, "1" ),

            // porosity
            SupportedDoubleKeywordInfo( "PORO"  , nan, distributeTopLayer , "1" ),

            // pore volume
            SupportedDoubleKeywordInfo( "PORV"  , nan, initPORV , "Volume" ),

            // pore volume multipliers
            SupportedDoubleKeywordInfo( "MULTPV", 1.0, "1" ),

            // the permeability keywords
            SupportedDoubleKeywordInfo( "PERMX" , nan,  distributeTopLayer , "Permeability" ),
            SupportedDoubleKeywordInfo( "PERMY" , nan,  distributeTopLayer , "Permeability" ),
            SupportedDoubleKeywordInfo( "PERMZ" , nan,  distributeTopLayer , "Permeability" ),
            SupportedDoubleKeywordInfo( "PERMXY", nan,  distributeTopLayer , "Permeability" ), // E300 only
            SupportedDoubleKeywordInfo( "PERMXZ", nan,  distributeTopLayer , "Permeability" ), // E300 only
            SupportedDoubleKeywordInfo( "PERMYZ", nan,  distributeTopLayer , "Permeability" ), // E300 only

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
            SupportedDoubleKeywordInfo( "SWATINIT" , 0.0, "1")
                });

        // register the grid properties
        m_intGridProperties = std::make_shared<GridProperties<int> >(m_eclipseGrid.get(), supportedIntKeywords);
        m_doubleGridProperties = std::make_shared<GridProperties<double> >(m_eclipseGrid.get(), supportedDoubleKeywords);

        if (!hasEclipseGrid()) {
            // make sure that no grid properties are specified by the deck
            for (size_t kwIdx = 0; kwIdx < deck->size(); ++ kwIdx) {
                const std::string& kwName = deck->getKeyword(kwIdx)->name();
                if (supportsGridProperty(kwName))
                    throw std::logic_error("Grid could not be initialized, but grid property " + kwName + " has been detected.");
            }

            // make sure that no grid property modifier keywords
            // (which can make grid properties appear out of thin air)
            // are present in the deck.
            if (deck->hasKeyword("ADD") ||
                deck->hasKeyword("BOX") ||
                deck->hasKeyword("COPY") ||
                deck->hasKeyword("EQUALS") ||
                deck->hasKeyword("MULTIPLY"))
            {
                throw std::logic_error("Grid could not be initialized, but grid properties have been detected.");
            }

            // if we don't have a grid and also no grid properties, we
            // just skip this method.
            return;
        }

        // actually create the grid property objects. we need to first
        // process all integer grid properties before the double ones
        // as these may be needed in order to initialize the double
        // properties
        processGridProperties(deck, parserLog, /*enabledTypes=*/IntProperties);
        processGridProperties(deck, parserLog, /*enabledTypes=*/DoubleProperties);
    }

    double EclipseState::getSIScaling(const std::string &dimensionString) const
    {
        return m_deckUnitSystem->getDimension(dimensionString)->getSIScaling();
    }

    void EclipseState::processGridProperties(Opm::DeckConstPtr deck, ParserLogPtr parserLog, int enabledTypes) {
        
        if (Section::hasGRID(deck)) {
            std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
            scanSection(gridSection, parserLog, enabledTypes);
        }


        if (Section::hasEDIT(deck)) {
            std::shared_ptr<Opm::EDITSection> editSection(new Opm::EDITSection(deck) );
            scanSection(editSection, parserLog, enabledTypes);
        }

        if (Section::hasPROPS(deck)) {
            std::shared_ptr<Opm::PROPSSection> propsSection(new Opm::PROPSSection(deck) );
            scanSection(propsSection, parserLog, enabledTypes);
        }

        if (Section::hasREGIONS(deck)) {
            std::shared_ptr<Opm::REGIONSSection> regionsSection(new Opm::REGIONSSection(deck) );
            scanSection(regionsSection, parserLog, enabledTypes);
        }

        if (Section::hasSOLUTION(deck)) {
            std::shared_ptr<Opm::SOLUTIONSection> solutionSection(new Opm::SOLUTIONSection(deck) );
            scanSection(solutionSection, parserLog, enabledTypes);
        }
    }

    void EclipseState::scanSection(std::shared_ptr<Opm::Section> section,
                                   ParserLogPtr parserLog,
                                   int enabledTypes) {
        BoxManager boxManager(m_eclipseGrid->getNX( ) , m_eclipseGrid->getNY() , m_eclipseGrid->getNZ());
        for (auto iter = section->begin(); iter != section->end(); ++iter) {
            DeckKeywordConstPtr deckKeyword = *iter;

            if (supportsGridProperty(deckKeyword->name(), enabledTypes) )
                loadGridPropertyFromDeckKeyword(boxManager.getActiveBox(), deckKeyword, parserLog, enabledTypes);
            else {
                if (deckKeyword->name() == "ADD")
                    handleADDKeyword(deckKeyword, parserLog, boxManager, enabledTypes);
            
                if (deckKeyword->name() == "BOX")
                    handleBOXKeyword(deckKeyword, parserLog, boxManager);
            
                if (deckKeyword->name() == "COPY")
                    handleCOPYKeyword(deckKeyword, parserLog, boxManager, enabledTypes);
            
                if (deckKeyword->name() == "EQUALS")
                    handleEQUALSKeyword(deckKeyword, parserLog, boxManager, enabledTypes);
            
                if (deckKeyword->name() == "ENDBOX")
                    handleENDBOXKeyword(boxManager);
            
                if (deckKeyword->name() == "MULTIPLY")
                    handleMULTIPLYKeyword(deckKeyword, parserLog, boxManager, enabledTypes);
            
                boxManager.endKeyword();
            }
        }
        boxManager.endSection();
    }




    void EclipseState::handleBOXKeyword(DeckKeywordConstPtr deckKeyword, ParserLogPtr /*parserLog*/, BoxManager& boxManager) {
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


    void EclipseState::handleMULTIPLYKeyword(DeckKeywordConstPtr deckKeyword, ParserLogPtr parserLog, BoxManager& boxManager, int enabledTypes) {
        for (size_t recordIdx = 0; recordIdx < deckKeyword->size(); ++recordIdx) {
            DeckRecordConstPtr record = deckKeyword->getRecord(recordIdx);
            const std::string& field = record->getItem("field")->getString(0);
            double      scaleFactor  = record->getItem("factor")->getRawDouble(0);
            
            setKeywordBox(deckKeyword, recordIdx, parserLog, boxManager);
            
            if (m_intGridProperties->hasKeyword( field )) {
                if (enabledTypes & IntProperties) {
                    int intFactor = static_cast<int>(scaleFactor);
                    std::shared_ptr<GridProperty<int> > property = m_intGridProperties->getKeyword( field );

                    property->scale( intFactor , boxManager.getActiveBox() );
                }
            } else if (m_doubleGridProperties->hasKeyword( field )) {
                if (enabledTypes & DoubleProperties) {
                    std::shared_ptr<GridProperty<double> > property = m_doubleGridProperties->getKeyword( field );
                    property->scale( scaleFactor , boxManager.getActiveBox() );
                }
            } else if (!m_intGridProperties->supportsKeyword(field) &&
                       !m_doubleGridProperties->supportsKeyword(field))
                throw std::invalid_argument("Fatal error processing MULTIPLY keyword. Tried to multiply not defined keyword " + field);
        }
    }


    /*
      The fine print of the manual says the ADD keyword should support
      some state dependent semantics regarding endpoint scaling arrays
      in the PROPS section. That is not supported. 
    */
    void EclipseState::handleADDKeyword(DeckKeywordConstPtr deckKeyword, ParserLogPtr parserLog, BoxManager& boxManager, int enabledTypes) {
        for (size_t recordIdx = 0; recordIdx < deckKeyword->size(); ++recordIdx) {
            DeckRecordConstPtr record = deckKeyword->getRecord(recordIdx);
            const std::string& field = record->getItem("field")->getString(0);
            double      shiftValue  = record->getItem("shift")->getRawDouble(0);
            
            setKeywordBox(deckKeyword, recordIdx, parserLog, boxManager);
            
            if (m_intGridProperties->hasKeyword( field )) {
                if (enabledTypes & IntProperties) {
                    int intShift = static_cast<int>(shiftValue);
                    std::shared_ptr<GridProperty<int> > property = m_intGridProperties->getKeyword( field );

                    property->add( intShift , boxManager.getActiveBox() );
                }
            } else if (m_doubleGridProperties->hasKeyword( field )) {
                if (enabledTypes & DoubleProperties) {
                    std::shared_ptr<GridProperty<double> > property = m_doubleGridProperties->getKeyword( field );

                    double siShiftValue = shiftValue * getSIScaling(property->getKeywordInfo().getDimensionString());
                    property->add(siShiftValue , boxManager.getActiveBox() );
                }
            } else if (!m_intGridProperties->supportsKeyword(field) &&
                       !m_doubleGridProperties->supportsKeyword(field))
                throw std::invalid_argument("Fatal error processing ADD keyword. Tried to shift not defined keyword " + field);

        }
    }


    void EclipseState::handleEQUALSKeyword(DeckKeywordConstPtr deckKeyword, ParserLogPtr parserLog, BoxManager& boxManager, int enabledTypes) {
        for (size_t recordIdx = 0; recordIdx < deckKeyword->size(); ++recordIdx) {
            DeckRecordConstPtr record = deckKeyword->getRecord(recordIdx);
            const std::string& field = record->getItem("field")->getString(0);
            double      value  = record->getItem("value")->getRawDouble(0);
            
            setKeywordBox(deckKeyword, recordIdx, parserLog, boxManager);
            
            if (m_intGridProperties->supportsKeyword( field )) {
                if (enabledTypes & IntProperties) {
                    int intValue = static_cast<int>(value);
                    std::shared_ptr<GridProperty<int> > property = m_intGridProperties->getKeyword( field );

                    property->setScalar( intValue , boxManager.getActiveBox() );
                }
            } else if (m_doubleGridProperties->supportsKeyword( field )) {

                if (enabledTypes & DoubleProperties) {
                    std::shared_ptr<GridProperty<double> > property = m_doubleGridProperties->getKeyword( field );

                    double siValue = value * getSIScaling(property->getKeywordInfo().getDimensionString());
                    property->setScalar( siValue , boxManager.getActiveBox() );
                }
            } else
                throw std::invalid_argument("Fatal error processing EQUALS keyword. Tried to set not defined keyword " + field);

        }
    }



    void EclipseState::handleCOPYKeyword(DeckKeywordConstPtr deckKeyword, ParserLogPtr parserLog, BoxManager& boxManager, int enabledTypes) {
        for (size_t recordIdx = 0; recordIdx < deckKeyword->size(); ++recordIdx) {
            DeckRecordConstPtr record = deckKeyword->getRecord(recordIdx);
            const std::string& srcField = record->getItem("src")->getString(0);
            const std::string& targetField = record->getItem("target")->getString(0);
            
            setKeywordBox(deckKeyword, recordIdx, parserLog, boxManager);
            
            if (m_intGridProperties->hasKeyword( srcField )) {
                if (enabledTypes & IntProperties)
                    copyIntKeyword( srcField , targetField , boxManager.getActiveBox());
            }
            else if (m_doubleGridProperties->hasKeyword( srcField )) {
                if (enabledTypes & DoubleProperties)
                    copyDoubleKeyword( srcField , targetField , boxManager.getActiveBox());
            }
            else if (!m_intGridProperties->supportsKeyword(srcField) &&
                     !m_doubleGridProperties->supportsKeyword(srcField))
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



    void EclipseState::setKeywordBox(DeckKeywordConstPtr deckKeyword, size_t recordIdx, ParserLogPtr parserLog, BoxManager& boxManager) {
        auto deckRecord = deckKeyword->getRecord(recordIdx);

        DeckItemConstPtr I1Item = deckRecord->getItem("I1");
        DeckItemConstPtr I2Item = deckRecord->getItem("I2");
        DeckItemConstPtr J1Item = deckRecord->getItem("J1");
        DeckItemConstPtr J2Item = deckRecord->getItem("J2");
        DeckItemConstPtr K1Item = deckRecord->getItem("K1");
        DeckItemConstPtr K2Item = deckRecord->getItem("K2");

        size_t setCount = 0;
        
        if (!I1Item->defaultApplied(0))
            setCount++;

        if (!I2Item->defaultApplied(0))
            setCount++;

        if (!J1Item->defaultApplied(0))
            setCount++;

        if (!J2Item->defaultApplied(0))
            setCount++;

        if (!K1Item->defaultApplied(0))
            setCount++;

        if (!K2Item->defaultApplied(0))
            setCount++;
        
        if (setCount == 6) {
            boxManager.setKeywordBox( I1Item->getInt(0) - 1,
                                      I2Item->getInt(0) - 1,
                                      J1Item->getInt(0) - 1,
                                      J2Item->getInt(0) - 1,
                                      K1Item->getInt(0) - 1,
                                      K2Item->getInt(0) - 1);
        } else if (setCount != 0)
            parserLog->addError(deckKeyword->getFileName(),
                                deckKeyword->getLineNumber(),
                                "BOX modifiers on keywords must be either specified completely or not at all. Ignoring.");
    }

    void EclipseState::complainAboutAmbiguousKeyword(DeckConstPtr deck, ParserLogPtr parserLog, const std::string& keywordName) const {
        parserLog->addError("", -1,
                            "The "+keywordName+" keyword must be unique in the deck. Ignoring all!");
        auto keywords = deck->getKeywordList(keywordName);
        for (size_t i = 0; i < keywords.size(); ++i)
            parserLog->addError(keywords[i]->getFileName(), keywords[i]->getLineNumber(),
                                "Ambiguous keyword "+keywordName+" defined here");
    }

}
