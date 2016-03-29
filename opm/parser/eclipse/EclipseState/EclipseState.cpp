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

#include <boost/algorithm/string/join.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Box.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/BoxManager.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FaultCollection.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Fault.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/MULTREGTScanner.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/NNC.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/SatfuncPropertyInitializers.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/TransMult.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/InitConfig.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/SimulationConfig/SimulationConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/M.hpp>
#include <opm/parser/eclipse/Units/Dimension.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>


namespace Opm {

    namespace GridPropertyPostProcessor {

        void distTopLayer( std::vector<double>& values, const Deck&, const EclipseState& m_eclipseState ) {
            EclipseGridConstPtr grid = m_eclipseState.getEclipseGrid();
            size_t layerSize = grid->getNX() * grid->getNY();
            size_t gridSize  = grid->getCartesianSize();

            for( size_t globalIndex = layerSize; globalIndex < gridSize; globalIndex++ ) {
                if( std::isnan( values[ globalIndex ] ) )
                    values[globalIndex] = values[globalIndex - layerSize];
            }
        }

        void initPORV( std::vector<double>& values, const Deck&, const EclipseState& m_eclipseState ) {
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
                else {
                    const auto& poroData = poro->getData();
                    for (size_t globalIndex = 0; globalIndex < porv->getCartesianSize(); globalIndex++) {
                        if (std::isnan(values[globalIndex])) {
                            double cell_poro = poroData[globalIndex];
                            double cell_ntg = ntg->iget(globalIndex);
                            double cell_volume = grid->getCellVolume(globalIndex);
                            values[globalIndex] = cell_poro * cell_volume * cell_ntg;
                        }
                    }
                }
            }

            if (m_eclipseState.hasDeckDoubleGridProperty("MULTPV")) {
                auto multpvData = m_eclipseState.getDoubleGridProperty("MULTPV")->getData();
                for (size_t globalIndex = 0; globalIndex < porv->getCartesianSize(); globalIndex++) {
                    values[globalIndex] *= multpvData[globalIndex];
                }
            }
        }
    }


    static bool isInt(double value) {
        double diff = fabs(nearbyint(value) - value);

        if (diff < 1e-6)
            return true;
        else
            return false;
    }


    EclipseState::EclipseState(DeckConstPtr deck , const ParseContext& parseContext)
        : m_deckUnitSystem( deck->getActiveUnitSystem() ),
          m_defaultRegion("FLUXNUM"),
          m_parseContext( parseContext )
    {
        initPhases(deck);
        initTables(deck);
        initEclipseGrid(deck);
        initGridopts(deck);
        initIOConfig(deck);
        initSchedule(deck);
        initIOConfigPostSchedule(deck);
        initTitle(deck);
        initProperties(deck);
        initInitConfig(deck);
        initSimulationConfig(deck);
        initTransMult();
        initFaults(deck);
        initMULTREGT(deck);
        initNNC(deck);
    }

    const UnitSystem& EclipseState::getDeckUnitSystem() const {
        return m_deckUnitSystem;
    }


    EclipseGridConstPtr EclipseState::getEclipseGrid() const {
        return m_eclipseGrid;
    }

    EclipseGridPtr EclipseState::getEclipseGridCopy() const {
        return std::make_shared<EclipseGrid>( m_eclipseGrid->c_ptr() );
    }


    std::shared_ptr<const TableManager> EclipseState::getTableManager() const {
        return m_tables;
    }


    const ParseContext& EclipseState::getParseContext() const {
        return m_parseContext;
    }


    ScheduleConstPtr EclipseState::getSchedule() const {
        return schedule;
    }

    IOConfigConstPtr EclipseState::getIOConfigConst() const {
        return m_ioConfig;
    }

    IOConfigPtr EclipseState::getIOConfig() const {
        return m_ioConfig;
    }

    InitConfigConstPtr EclipseState::getInitConfig() const {
        return m_initConfig;
    }

    SimulationConfigConstPtr EclipseState::getSimulationConfig() const {
        return m_simulationConfig;
    }

    std::shared_ptr<const FaultCollection> EclipseState::getFaults() const {
        return m_faults;
    }

    std::shared_ptr<const TransMult> EclipseState::getTransMult() const {
        return m_transMult;
    }

    std::shared_ptr<const NNC> EclipseState::getNNC() const {
        return m_nnc;
    }

    bool EclipseState::hasNNC() const {
        return m_nnc->hasNNC();
    }

    std::string EclipseState::getTitle() const {
        return m_title;
    }



    void EclipseState::initTables(DeckConstPtr deck) {
        m_tables = std::make_shared<const TableManager>( *deck );
   }

    void EclipseState::initIOConfig(DeckConstPtr deck) {
        m_ioConfig = std::make_shared<IOConfig>();
        if (Section::hasGRID(*deck)) {
            std::shared_ptr<const GRIDSection> gridSection = std::make_shared<const GRIDSection>(*deck);
            m_ioConfig->handleGridSection(gridSection);
        }
        if (Section::hasRUNSPEC(*deck)) {
            std::shared_ptr<const RUNSPECSection> runspecSection = std::make_shared<const RUNSPECSection>(*deck);
            m_ioConfig->handleRunspecSection(runspecSection);
        }
    }


    // Hmmm - would have thought this should iterate through the SCHEDULE section as well?
    void EclipseState::initIOConfigPostSchedule(DeckConstPtr deck) {
        if (Section::hasSOLUTION(*deck)) {
            std::shared_ptr<const SOLUTIONSection> solutionSection = std::make_shared<const SOLUTIONSection>(*deck);
            m_ioConfig->handleSolutionSection(schedule->getTimeMap(), solutionSection);
        }
        m_ioConfig->initFirstOutput( *this->schedule );
    }

    void EclipseState::initInitConfig(DeckConstPtr deck){
        m_initConfig = std::make_shared<const InitConfig>(deck);
    }

    void EclipseState::initSimulationConfig(DeckConstPtr deck) {
        m_simulationConfig = std::make_shared<const SimulationConfig>(m_parseContext , deck , m_intGridProperties);
    }


    void EclipseState::initSchedule(DeckConstPtr deck) {
        EclipseGridConstPtr grid = getEclipseGrid();
        schedule = ScheduleConstPtr( new Schedule(m_parseContext , grid , deck, m_ioConfig) );
    }

    void EclipseState::initNNC(DeckConstPtr deck) {
        EclipseGridConstPtr grid = getEclipseGrid();
        m_nnc = std::make_shared<NNC>( deck, grid);
    }

    void EclipseState::initTransMult() {
        EclipseGridConstPtr grid = getEclipseGrid();
        m_transMult = std::make_shared<TransMult>( grid->getNX() , grid->getNY() , grid->getNZ());

        if (hasDeckDoubleGridProperty("MULTX"))
            m_transMult->applyMULT(m_doubleGridProperties->getKeyword("MULTX"), FaceDir::XPlus);
        if (hasDeckDoubleGridProperty("MULTX-"))
            m_transMult->applyMULT(m_doubleGridProperties->getKeyword("MULTX-"), FaceDir::XMinus);

        if (hasDeckDoubleGridProperty("MULTY"))
            m_transMult->applyMULT(m_doubleGridProperties->getKeyword("MULTY"), FaceDir::YPlus);
        if (hasDeckDoubleGridProperty("MULTY-"))
            m_transMult->applyMULT(m_doubleGridProperties->getKeyword("MULTY-"), FaceDir::YMinus);

        if (hasDeckDoubleGridProperty("MULTZ"))
            m_transMult->applyMULT(m_doubleGridProperties->getKeyword("MULTZ"), FaceDir::ZPlus);
        if (hasDeckDoubleGridProperty("MULTZ-"))
            m_transMult->applyMULT(m_doubleGridProperties->getKeyword("MULTZ-"), FaceDir::ZMinus);
    }

    void EclipseState::initFaults(DeckConstPtr deck) {
        EclipseGridConstPtr grid = getEclipseGrid();
        std::shared_ptr<GRIDSection> gridSection = std::make_shared<GRIDSection>( *deck );

        m_faults = std::make_shared<FaultCollection>(gridSection , grid);
        setMULTFLT(gridSection);

        if (Section::hasEDIT(*deck)) {
            std::shared_ptr<EDITSection> editSection = std::make_shared<EDITSection>( *deck );
            setMULTFLT(editSection);
        }

        m_transMult->applyMULTFLT( m_faults );
    }



    void EclipseState::setMULTFLT(std::shared_ptr<const Section> section) const {
        for (size_t index=0; index < section->count("MULTFLT"); index++) {
            const auto& faultsKeyword = section->getKeyword("MULTFLT" , index);
            for (auto iter = faultsKeyword.begin(); iter != faultsKeyword.end(); ++iter) {

                const auto& faultRecord = *iter;
                const std::string& faultName = faultRecord.getItem(0).get< std::string >(0);
                double multFlt = faultRecord.getItem(1).get< double >(0);

                m_faults->setTransMult( faultName , multFlt );
            }
        }
    }



    void EclipseState::initMULTREGT(DeckConstPtr deck) {
        EclipseGridConstPtr grid = getEclipseGrid();

        std::vector< const DeckKeyword* > multregtKeywords;
        if (deck->hasKeyword("MULTREGT"))
            multregtKeywords = deck->getKeywordList("MULTREGT");

        std::shared_ptr<MULTREGTScanner> scanner = std::make_shared<MULTREGTScanner>(m_intGridProperties, multregtKeywords , m_defaultRegion);
        m_transMult->setMultregtScanner( scanner );
    }



    void EclipseState::initEclipseGrid(DeckConstPtr deck) {
        m_eclipseGrid = EclipseGridConstPtr( new EclipseGrid(deck));
    }


    void EclipseState::initGridopts(DeckConstPtr deck) {
        if (deck->hasKeyword("GRIDOPTS")) {
            /*
              The EQUALREG, MULTREG, COPYREG, ... keywords are used to
              manipulate vectors based on region values; for instance
              the statement

                EQUALREG
                   PORO  0.25  3    /   -- Region array not specified
                   PERMX 100   3  F /
                /

              will set the PORO field to 0.25 for all cells in region
              3 and the PERMX value to 100 mD for the same cells. The
              fourth optional argument to the EQUALREG keyword is used
              to indicate which REGION array should be used for the
              selection.

              If the REGION array is not indicated (as in the PORO
              case) above, the default region to use in the xxxREG
              keywords depends on the GRIDOPTS keyword:

                1. If GRIDOPTS is present, and the NRMULT item is
                   greater than zero, the xxxREG keywords will default
                   to use the MULTNUM region.

                2. If the GRIDOPTS keyword is not present - or the
                   NRMULT item equals zero, the xxxREG keywords will
                   default to use the FLUXNUM keyword.

              This quite weird behaviour comes from reading the
              GRIDOPTS and MULTNUM documentation, and practical
              experience with ECLIPSE simulations. Ufortunately the
              documentation of the xxxREG keywords does not confirm
              this.
            */

            auto gridOpts = deck->getKeyword("GRIDOPTS");
            const auto& record = gridOpts.getRecord(0);
            const auto& nrmult_item = record.getItem("NRMULT");

            if (nrmult_item.get< int >(0) > 0)
                m_defaultRegion = "MULTNUM";
        }
    }


    void EclipseState::initPhases(DeckConstPtr deck) {
        if (deck->hasKeyword("OIL"))
            phases.insert(Phase::PhaseEnum::OIL);

        if (deck->hasKeyword("GAS"))
            phases.insert(Phase::PhaseEnum::GAS);

        if (deck->hasKeyword("WATER"))
            phases.insert(Phase::PhaseEnum::WATER);

        if (phases.size() < 3)
            OpmLog::addMessage(Log::MessageType::Info , "Only " + std::to_string(static_cast<long long>(phases.size())) + " fluid phases are enabled");
    }

    size_t EclipseState::getNumPhases() const{
        return phases.size();
    }

    bool EclipseState::hasPhase(enum Phase::PhaseEnum phase) const {
         return (phases.count(phase) == 1);
    }

    void EclipseState::initTitle(DeckConstPtr deck){
        if (deck->hasKeyword("TITLE")) {
            const auto& titleKeyword = deck->getKeyword("TITLE");
            const auto& item = titleKeyword.getRecord( 0 ).getItem( 0 );
            std::vector<std::string> itemValue = item.getData< std::string >();
            m_title = boost::algorithm::join(itemValue, " ");
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

    bool EclipseState::hasDeckIntGridProperty(const std::string& keyword) const {
        if (!m_intGridProperties->supportsKeyword( keyword ))
            throw std::logic_error("Integer grid property " + keyword + " is unsupported!");

         return m_intGridProperties->hasKeyword( keyword );
    }

    bool EclipseState::hasDeckDoubleGridProperty(const std::string& keyword) const {
        if (!m_doubleGridProperties->supportsKeyword( keyword ))
            throw std::logic_error("Double grid property " + keyword + " is unsupported!");

         return m_doubleGridProperties->hasKeyword( keyword );
    }


    /*
      1. The public methods getIntGridProperty & getDoubleGridProperty
         will invoke and run the property post processor (if any is
         registered); the post processor will only run one time.

         It is important that post processor is not run prematurely,
         internal functions in EclipseState should therefore ask for
         properties by invoking the getKeyword() method of the
         m_intGridProperties / m_doubleGridProperties() directly and
         not through these methods.

      2. Observe that this will autocreate a property if it has not
         been explicitly added.
    */

    std::shared_ptr<const GridProperty<int> > EclipseState::getIntGridProperty( const std::string& keyword ) const {
        return m_intGridProperties->getKeyword( keyword );
    }

    std::shared_ptr<const GridProperty<double> > EclipseState::getDoubleGridProperty( const std::string& keyword ) const {
        auto gridProperty = m_doubleGridProperties->getKeyword( keyword );

        gridProperty->runPostProcessor();

        return gridProperty;
    }

    std::shared_ptr<const GridProperty<int> > EclipseState::getDefaultRegion() const {
        return m_intGridProperties->getKeyword( m_defaultRegion );
    }

    std::shared_ptr<const GridProperty<int> > EclipseState::getRegion( const DeckItem& regionItem ) const {
        if (regionItem.defaultApplied(0))
            return getDefaultRegion();
        else {
            const std::string regionArray = MULTREGT::RegionNameFromDeckValue( regionItem.get< std::string >(0) );
            return m_intGridProperties->getInitializedKeyword( regionArray );
        }
    }



    /*
       Due to the post processor which might be applied to the
       GridProperty objects it is essential that this method use the
       m_intGridProperties / m_doubleGridProperties fields directly
       and *NOT* use the public methods get< int >GridProperty /
       getDoubleGridProperty.
    */

    void EclipseState::loadGridPropertyFromDeckKeyword(std::shared_ptr<const Box> inputBox,
                                                       const DeckKeyword& deckKeyword,
                                                       int enabledTypes) {
        const std::string& keyword = deckKeyword.name();
        if (m_intGridProperties->supportsKeyword( keyword )) {
            if (enabledTypes & IntProperties) {
                auto gridProperty = getOrCreateIntProperty_( keyword );
                gridProperty->loadFromDeckKeyword( inputBox , deckKeyword );
            }
        } else if (m_doubleGridProperties->supportsKeyword( keyword )) {
            if (enabledTypes & DoubleProperties) {
                auto gridProperty = getOrCreateDoubleProperty_( keyword );
                gridProperty->loadFromDeckKeyword( inputBox , deckKeyword );
            }
        } else {
            std::string msg = Log::fileMessage(deckKeyword.getFileName(),
                                               deckKeyword.getLineNumber(),
                                               "Tried to load unsupported grid property from keyword: " + deckKeyword.name());
            OpmLog::addMessage(Log::MessageType::Error , msg);
        }
    }

    static std::vector< GridProperties< int >::SupportedKeywordInfo >
    makeSupportedIntKeywords() {
        return {    GridProperties< int >::SupportedKeywordInfo( "SATNUM" , 1, "1" ),
                    GridProperties< int >::SupportedKeywordInfo( "IMBNUM" , 1, "1" ),
                    GridProperties< int >::SupportedKeywordInfo( "PVTNUM" , 1, "1" ),
                    GridProperties< int >::SupportedKeywordInfo( "EQLNUM" , 1, "1" ),
                    GridProperties< int >::SupportedKeywordInfo( "ENDNUM" , 1, "1" ),
                    GridProperties< int >::SupportedKeywordInfo( "FLUXNUM" , 1 , "1" ),
                    GridProperties< int >::SupportedKeywordInfo( "MULTNUM", 1 , "1" ),
                    GridProperties< int >::SupportedKeywordInfo( "FIPNUM" , 1, "1" ),
                    GridProperties< int >::SupportedKeywordInfo( "MISCNUM", 1, "1" )
        };
    }

    static std::vector< GridProperties< double >::SupportedKeywordInfo >
    makeSupportedDoubleKeywords(const Deck& deck, const EclipseState& es) {
        GridPropertyInitFunction< double > SGLLookup    ( &SGLEndpoint, deck, es );
        GridPropertyInitFunction< double > ISGLLookup   ( &ISGLEndpoint, deck, es );
        GridPropertyInitFunction< double > SWLLookup    ( &SWLEndpoint, deck, es );
        GridPropertyInitFunction< double > ISWLLookup   ( &ISWLEndpoint, deck, es );
        GridPropertyInitFunction< double > SGULookup    ( &SGUEndpoint, deck, es );
        GridPropertyInitFunction< double > ISGULookup   ( &ISGUEndpoint, deck, es );
        GridPropertyInitFunction< double > SWULookup    ( &SWUEndpoint, deck, es );
        GridPropertyInitFunction< double > ISWULookup   ( &ISWUEndpoint, deck, es );
        GridPropertyInitFunction< double > SGCRLookup   ( &SGCREndpoint, deck, es );
        GridPropertyInitFunction< double > ISGCRLookup  ( &ISGCREndpoint, deck, es );
        GridPropertyInitFunction< double > SOWCRLookup  ( &SOWCREndpoint, deck, es );
        GridPropertyInitFunction< double > ISOWCRLookup ( &ISOWCREndpoint, deck, es );
        GridPropertyInitFunction< double > SOGCRLookup  ( &SOGCREndpoint, deck, es );
        GridPropertyInitFunction< double > ISOGCRLookup ( &ISOGCREndpoint, deck, es );
        GridPropertyInitFunction< double > SWCRLookup   ( &SWCREndpoint, deck, es );
        GridPropertyInitFunction< double > ISWCRLookup  ( &ISWCREndpoint, deck, es );

        GridPropertyInitFunction< double > PCWLookup    ( &PCWEndpoint, deck, es );
        GridPropertyInitFunction< double > IPCWLookup   ( &IPCWEndpoint, deck, es );
        GridPropertyInitFunction< double > PCGLookup    ( &PCGEndpoint, deck, es );
        GridPropertyInitFunction< double > IPCGLookup   ( &IPCGEndpoint, deck, es );
        GridPropertyInitFunction< double > KRWLookup    ( &KRWEndpoint, deck, es );
        GridPropertyInitFunction< double > IKRWLookup   ( &IKRWEndpoint, deck, es );
        GridPropertyInitFunction< double > KRWRLookup   ( &KRWREndpoint, deck, es );
        GridPropertyInitFunction< double > IKRWRLookup  ( &IKRWREndpoint, deck, es );
        GridPropertyInitFunction< double > KROLookup    ( &KROEndpoint, deck, es );
        GridPropertyInitFunction< double > IKROLookup   ( &IKROEndpoint, deck, es );
        GridPropertyInitFunction< double > KRORWLookup  ( &KRORWEndpoint, deck, es );
        GridPropertyInitFunction< double > IKRORWLookup ( &IKRORWEndpoint, deck, es );
        GridPropertyInitFunction< double > KRORGLookup  ( &KRORGEndpoint, deck, es );
        GridPropertyInitFunction< double > IKRORGLookup ( &IKRORGEndpoint, deck, es );
        GridPropertyInitFunction< double > KRGLookup    ( &KRGEndpoint, deck, es );
        GridPropertyInitFunction< double > IKRGLookup   ( &IKRGEndpoint, deck, es );
        GridPropertyInitFunction< double > KRGRLookup   ( &KRGREndpoint, deck, es );
        GridPropertyInitFunction< double > IKRGRLookup  ( &IKRGREndpoint, deck, es );

        GridPropertyInitFunction< double > tempLookup   ( &temperature_lookup, deck, es );
        GridPropertyPostFunction< double > initPORV     ( &GridPropertyPostProcessor::initPORV, deck, es );
        GridPropertyPostFunction< double > distributeTopLayer( &GridPropertyPostProcessor::distTopLayer, deck, es );

        std::vector< GridProperties< double >::SupportedKeywordInfo > supportedDoubleKeywords;

        // keywords to specify the scaled connate gas saturations.
        for( const auto& kw : { "SGL", "SGLX", "SGLX-", "SGLY", "SGLY-", "SGLZ", "SGLZ-" } )
            supportedDoubleKeywords.emplace_back( kw, SGLLookup, "1" );
        for( const auto& kw : { "ISGL", "ISGLX", "ISGLX-", "ISGLY", "ISGLY-", "ISGLZ", "ISGLZ-" } )
            supportedDoubleKeywords.emplace_back( kw, ISGLLookup, "1" );

        // keywords to specify the connate water saturation.
        for( const auto& kw : { "SWL", "SWLX", "SWLX-", "SWLY", "SWLY-", "SWLZ", "SWLZ-" } )
            supportedDoubleKeywords.emplace_back( kw, SWLLookup, "1" );
        for( const auto& kw : { "ISWL", "ISWLX", "ISWLX-", "ISWLY", "ISWLY-", "ISWLZ", "ISWLZ-" } )
            supportedDoubleKeywords.emplace_back( kw, ISWLLookup, "1" );

        // keywords to specify the maximum gas saturation.
        for( const auto& kw : { "SGU", "SGUX", "SGUX-", "SGUY", "SGUY-", "SGUZ", "SGUZ-" } )
            supportedDoubleKeywords.emplace_back( kw, SGULookup, "1" );
        for( const auto& kw : { "ISGU", "ISGUX", "ISGUX-", "ISGUY", "ISGUY-", "ISGUZ", "ISGUZ-" } )
            supportedDoubleKeywords.emplace_back( kw, ISGULookup, "1" );

        // keywords to specify the maximum water saturation.
        for( const auto& kw : { "SWU", "SWUX", "SWUX-", "SWUY", "SWUY-", "SWUZ", "SWUZ-" } )
            supportedDoubleKeywords.emplace_back( kw, SWULookup, "1" );
        for( const auto& kw : { "ISWU", "ISWUX", "ISWUX-", "ISWUY", "ISWUY-", "ISWUZ", "ISWUZ-" } )
            supportedDoubleKeywords.emplace_back( kw, ISWULookup, "1" );

        // keywords to specify the scaled critical gas saturation.
        for( const auto& kw : { "SGCR", "SGCRX", "SGCRX-", "SGCRY", "SGCRY-", "SGCRZ", "SGCRZ-" } )
            supportedDoubleKeywords.emplace_back( kw, SGCRLookup, "1" );
        for( const auto& kw : { "ISGCR", "ISGCRX", "ISGCRX-", "ISGCRY", "ISGCRY-", "ISGCRZ", "ISGCRZ-" } )
            supportedDoubleKeywords.emplace_back( kw, ISGCRLookup, "1" );

        // keywords to specify the scaled critical oil-in-water saturation.
        for( const auto& kw : { "SOWCR", "SOWCRX", "SOWCRX-", "SOWCRY", "SOWCRY-", "SOWCRZ", "SOWCRZ-" } )
            supportedDoubleKeywords.emplace_back( kw, SOWCRLookup, "1" );
        for( const auto& kw : { "ISOWCR", "ISOWCRX", "ISOWCRX-", "ISOWCRY", "ISOWCRY-", "ISOWCRZ", "ISOWCRZ-" } )
            supportedDoubleKeywords.emplace_back( kw, ISOWCRLookup, "1" );

        // keywords to specify the scaled critical oil-in-gas saturation.
        for( const auto& kw : { "SOGCR", "SOGCRX", "SOGCRX-", "SOGCRY", "SOGCRY-", "SOGCRZ", "SOGCRZ-" } )
            supportedDoubleKeywords.emplace_back( kw, SOGCRLookup, "1" );
        for( const auto& kw : { "ISOGCR", "ISOGCRX", "ISOGCRX-", "ISOGCRY", "ISOGCRY-", "ISOGCRZ", "ISOGCRZ-" } )
            supportedDoubleKeywords.emplace_back( kw, ISOGCRLookup, "1" );

        // keywords to specify the scaled critical water saturation.
        for( const auto& kw : { "SWCR", "SWCRX", "SWCRX-", "SWCRY", "SWCRY-", "SWCRZ", "SWCRZ-" } )
            supportedDoubleKeywords.emplace_back( kw, SWCRLookup, "1" );
        for( const auto& kw : { "ISWCR", "ISWCRX", "ISWCRX-", "ISWCRY", "ISWCRY-", "ISWCRZ", "ISWCRZ-" } )
            supportedDoubleKeywords.emplace_back( kw, ISWCRLookup, "1" );

        // keywords to specify the scaled oil-water capillary pressure
        for( const auto& kw : { "PCW", "PCWX", "PCWX-", "PCWY", "PCWY-", "PCWZ", "PCWZ-" } )
            supportedDoubleKeywords.emplace_back( kw, PCWLookup, "1" );
        for( const auto& kw : { "IPCW", "IPCWX", "IPCWX-", "IPCWY", "IPCWY-", "IPCWZ", "IPCWZ-" } )
            supportedDoubleKeywords.emplace_back( kw, IPCWLookup, "1" );

        // keywords to specify the scaled gas-oil capillary pressure
        for( const auto& kw : { "PCG", "PCGX", "PCGX-", "PCGY", "PCGY-", "PCGZ", "PCGZ-" } )
            supportedDoubleKeywords.emplace_back( kw, PCGLookup, "1" );
        for( const auto& kw : { "IPCG", "IPCGX", "IPCGX-", "IPCGY", "IPCGY-", "IPCGZ", "IPCGZ-" } )
            supportedDoubleKeywords.emplace_back( kw, IPCGLookup, "1" );

        // keywords to specify the scaled water relative permeability
        for( const auto& kw : { "KRW", "KRWX", "KRWX-", "KRWY", "KRWY-", "KRWZ", "KRWZ-" } )
            supportedDoubleKeywords.emplace_back( kw, KRWLookup, "1" );
        for( const auto& kw : { "IKRW", "IKRWX", "IKRWX-", "IKRWY", "IKRWY-", "IKRWZ", "IKRWZ-" } )
            supportedDoubleKeywords.emplace_back( kw, IKRWLookup, "1" );

        // keywords to specify the scaled water relative permeability at the critical
        // saturation
        for( const auto& kw : { "KRWR" , "KRWRX" , "KRWRX-" , "KRWRY" , "KRWRY-" , "KRWRZ" , "KRWRZ-"  } )
            supportedDoubleKeywords.emplace_back( kw, KRWRLookup, "1" );
        for( const auto& kw : { "IKRWR" , "IKRWRX" , "IKRWRX-" , "IKRWRY" , "IKRWRY-" , "IKRWRZ" , "IKRWRZ-" } )
            supportedDoubleKeywords.emplace_back( kw, IKRWRLookup, "1" );

        // keywords to specify the scaled oil relative permeability
        for( const auto& kw : { "KRO", "KROX", "KROX-", "KROY", "KROY-", "KROZ", "KROZ-" } )
            supportedDoubleKeywords.emplace_back( kw, KROLookup, "1" );
        for( const auto& kw : { "IKRO", "IKROX", "IKROX-", "IKROY", "IKROY-", "IKROZ", "IKROZ-" } )
            supportedDoubleKeywords.emplace_back( kw, IKROLookup, "1" );

        // keywords to specify the scaled water relative permeability at the critical
        // water saturation
        for( const auto& kw : { "KRORW", "KRORWX", "KRORWX-", "KRORWY", "KRORWY-", "KRORWZ", "KRORWZ-" } )
            supportedDoubleKeywords.emplace_back( kw, KRORWLookup, "1" );
        for( const auto& kw : { "IKRORW", "IKRORWX", "IKRORWX-", "IKRORWY", "IKRORWY-", "IKRORWZ", "IKRORWZ-" } )
            supportedDoubleKeywords.emplace_back( kw, IKRORWLookup, "1" );

        // keywords to specify the scaled water relative permeability at the critical
        // water saturation
        for( const auto& kw : { "KRORG", "KRORGX", "KRORGX-", "KRORGY", "KRORGY-", "KRORGZ", "KRORGZ-" } )
            supportedDoubleKeywords.emplace_back( kw, KRORGLookup, "1" );
        for( const auto& kw : { "IKRORG", "IKRORGX", "IKRORGX-", "IKRORGY", "IKRORGY-", "IKRORGZ", "IKRORGZ-" } )
            supportedDoubleKeywords.emplace_back( kw, IKRORGLookup, "1" );

        // keywords to specify the scaled gas relative permeability
        for( const auto& kw : { "KRG", "KRGX", "KRGX-", "KRGY", "KRGY-", "KRGZ", "KRGZ-" } )
            supportedDoubleKeywords.emplace_back( kw, KRGLookup, "1" );
        for( const auto& kw : { "IKRG", "IKRGX", "IKRGX-", "IKRGY", "IKRGY-", "IKRGZ", "IKRGZ-" } )
            supportedDoubleKeywords.emplace_back( kw, IKRGLookup, "1" );

        // keywords to specify the scaled gas relative permeability
        for( const auto& kw : { "KRGR", "KRGRX", "KRGRX-", "KRGRY", "KRGRY-", "KRGRZ", "KRGRZ-" } )
             supportedDoubleKeywords.emplace_back( kw, KRGRLookup, "1" );
        for( const auto& kw : { "IKRGR", "IKRGRX", "IKRGRX-", "IKRGRY", "IKRGRY-", "IKRGRZ", "IKRGRZ-" } )
            supportedDoubleKeywords.emplace_back( kw, IKRGRLookup, "1" );



        // cell temperature (E300 only, but makes a lot of sense for E100, too)
        supportedDoubleKeywords.emplace_back( "TEMPI", tempLookup, "Temperature" );

        double nan = std::numeric_limits<double>::quiet_NaN();
        // porosity
        supportedDoubleKeywords.emplace_back( "PORO", nan, distributeTopLayer, "1" );

        // pore volume
        supportedDoubleKeywords.emplace_back( "PORV", nan, initPORV, "Volume" );

        // pore volume multipliers
        supportedDoubleKeywords.emplace_back( "MULTPV", 1.0, "1" );

        // the permeability keywords
        for( const auto& kw : { "PERMX", "PERMY", "PERMZ" } )
            supportedDoubleKeywords.emplace_back( kw, nan, distributeTopLayer, "Permeability" );

        /* E300 only */
        for( const auto& kw : { "PERMXY", "PERMYZ", "PERMZX" } )
            supportedDoubleKeywords.emplace_back( kw, nan, distributeTopLayer, "Permeability" );

        /* the transmissibility keywords for neighboring connections. note that
         * these keywords don't seem to require a post-processor
         */
        for( const auto& kw : { "TRANX", "TRANY", "TRANZ" } )
            supportedDoubleKeywords.emplace_back( kw, nan, "Transmissibility" );

        /* gross-to-net thickness (acts as a multiplier for PORO and the
         * permeabilities in the X-Y plane as well as for the well rates.)
         */
        supportedDoubleKeywords.emplace_back( "NTG", 1.0, "1" );

        // transmissibility multipliers
        for( const auto& kw : { "MULTX", "MULTY", "MULTZ", "MULTX-", "MULTY-", "MULTZ-" } )
            supportedDoubleKeywords.emplace_back( kw, 1.0, "1" );

        // initialisation
        supportedDoubleKeywords.emplace_back( "SWATINIT", 0.0, "1");
        supportedDoubleKeywords.emplace_back( "THCONR", 0.0, "1");

        return supportedDoubleKeywords;
    }

    void EclipseState::initProperties(DeckConstPtr deck) {

        // Note that the variants of grid keywords for radial grids
        // are not supported. (and hopefully never will be)

        // register the grid properties
        m_intGridProperties.reset( new GridProperties< int >( m_eclipseGrid , makeSupportedIntKeywords() ) );
        m_doubleGridProperties.reset( new GridProperties< double >( m_eclipseGrid, makeSupportedDoubleKeywords(*deck, *this) ) );

        // actually create the grid property objects. we need to first
        // process all integer grid properties before the double ones
        // as these may be needed in order to initialize the double
        // properties
        processGridProperties(deck, /*enabledTypes=*/IntProperties);
        processGridProperties(deck, /*enabledTypes=*/DoubleProperties);
    }

    double EclipseState::getSIScaling(const std::string &dimensionString) const
    {
        return m_deckUnitSystem.getDimension(dimensionString)->getSIScaling();
    }

    void EclipseState::processGridProperties(Opm::DeckConstPtr deck, int enabledTypes) {

        if (Section::hasGRID(*deck)) {
            std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(*deck) );
            scanSection(gridSection, enabledTypes);
        }


        if (Section::hasEDIT(*deck)) {
            std::shared_ptr<Opm::EDITSection> editSection(new Opm::EDITSection(*deck) );
            scanSection(editSection, enabledTypes);
        }

        if (Section::hasPROPS(*deck)) {
            std::shared_ptr<Opm::PROPSSection> propsSection(new Opm::PROPSSection(*deck) );
            scanSection(propsSection, enabledTypes);
        }

        if (Section::hasREGIONS(*deck)) {
            std::shared_ptr<Opm::REGIONSSection> regionsSection(new Opm::REGIONSSection(*deck) );
            scanSection(regionsSection, enabledTypes);
        }

        if (Section::hasSOLUTION(*deck)) {
            std::shared_ptr<Opm::SOLUTIONSection> solutionSection(new Opm::SOLUTIONSection(*deck) );
            scanSection(solutionSection,  enabledTypes);
        }
    }

    void EclipseState::scanSection(std::shared_ptr<Opm::Section> section,
                                   int enabledTypes) {
        BoxManager boxManager(m_eclipseGrid->getNX( ) , m_eclipseGrid->getNY() , m_eclipseGrid->getNZ());
        for( const auto& deckKeyword : *section ) {

            if (supportsGridProperty(deckKeyword.name(), enabledTypes) )
                loadGridPropertyFromDeckKeyword(boxManager.getActiveBox(), deckKeyword,  enabledTypes);
            else {
                if (deckKeyword.name() == "ADD")
                    handleADDKeyword(deckKeyword, boxManager, enabledTypes);

                if (deckKeyword.name() == "BOX")
                    handleBOXKeyword(deckKeyword, boxManager);

                if (deckKeyword.name() == "COPY")
                    handleCOPYKeyword(deckKeyword, boxManager, enabledTypes);

                if (deckKeyword.name() == "EQUALS")
                    handleEQUALSKeyword(deckKeyword, boxManager, enabledTypes);

                if (deckKeyword.name() == "ENDBOX")
                    handleENDBOXKeyword(boxManager);

                if (deckKeyword.name() == "EQUALREG")
                    handleEQUALREGKeyword(deckKeyword ,  enabledTypes);

                if (deckKeyword.name() == "ADDREG")
                    handleADDREGKeyword(deckKeyword , enabledTypes);

                if (deckKeyword.name() == "MULTIREG")
                    handleMULTIREGKeyword(deckKeyword , enabledTypes);

                if (deckKeyword.name() == "COPYREG")
                    handleCOPYREGKeyword(deckKeyword , enabledTypes);

                if (deckKeyword.name() == "MULTIPLY")
                    handleMULTIPLYKeyword(deckKeyword, boxManager, enabledTypes);

                boxManager.endKeyword();
            }
        }
        boxManager.endSection();
    }




    void EclipseState::handleBOXKeyword( const DeckKeyword& deckKeyword,  BoxManager& boxManager) {
        const auto& record = deckKeyword.getRecord(0);
        int I1 = record.getItem("I1").get< int >(0) - 1;
        int I2 = record.getItem("I2").get< int >(0) - 1;
        int J1 = record.getItem("J1").get< int >(0) - 1;
        int J2 = record.getItem("J2").get< int >(0) - 1;
        int K1 = record.getItem("K1").get< int >(0) - 1;
        int K2 = record.getItem("K2").get< int >(0) - 1;

        boxManager.setInputBox( I1 , I2 , J1 , J2 , K1 , K2 );
    }


    void EclipseState::handleENDBOXKeyword(BoxManager& boxManager) {
        boxManager.endInputBox();
    }


    void EclipseState::handleEQUALREGKeyword( const DeckKeyword& deckKeyword, int enabledTypes) {
        EclipseGridConstPtr grid = getEclipseGrid();
        for( const auto& record : deckKeyword ) {
            const std::string& targetArray = record.getItem("ARRAY").get< std::string >(0);

            if (!supportsGridProperty( targetArray , IntProperties + DoubleProperties))
                throw std::invalid_argument("Fatal error processing EQUALREG keyword - invalid/undefined keyword: " + targetArray);

            double doubleValue = record.getItem("VALUE").template get<double>(0);
            int regionValue = record.getItem("REGION_NUMBER").template get<int>(0);
            std::shared_ptr<const Opm::GridProperty<int> > regionProperty = getRegion( record.getItem("REGION_NAME") );
            std::vector<bool> mask;

            regionProperty->initMask( regionValue , mask);

            if (m_intGridProperties->supportsKeyword( targetArray )) {
                if (enabledTypes & IntProperties) {
                    if (isInt( doubleValue )) {
                        std::shared_ptr<Opm::GridProperty<int> > targetProperty = getOrCreateIntProperty_(targetArray);
                        int intValue = static_cast<int>( doubleValue + 0.5 );
                        targetProperty->maskedSet( intValue , mask);
                    } else
                        throw std::invalid_argument("Fatal error processing EQUALREG keyword - expected integer value for: " + targetArray);
                }
            }
            else if (m_doubleGridProperties->supportsKeyword( targetArray )) {
                if (enabledTypes & DoubleProperties) {
                    std::shared_ptr<Opm::GridProperty<double> > targetProperty = getOrCreateDoubleProperty_(targetArray);
                    const std::string& dimensionString = targetProperty->getDimensionString();
                    double SIValue = doubleValue * getSIScaling( dimensionString );
                    targetProperty->maskedSet( SIValue , mask);
                }
            }
            else {
                throw std::invalid_argument("Fatal error processing EQUALREG keyword - invalid/undefined keyword: " + targetArray);
            }
        }
    }


    void EclipseState::handleADDREGKeyword( const DeckKeyword& deckKeyword, int enabledTypes) {
        EclipseGridConstPtr grid = getEclipseGrid();
        for( const auto& record : deckKeyword ) {
            const std::string& targetArray = record.getItem("ARRAY").get< std::string >(0);

            if (!supportsGridProperty( targetArray , IntProperties + DoubleProperties))
                throw std::invalid_argument("Fatal error processing ADDREG keyword - invalid/undefined keyword: " + targetArray);

            if (supportsGridProperty( targetArray , enabledTypes)) {
                double doubleValue = record.getItem("SHIFT").get< double >(0);
                int regionValue = record.getItem("REGION_NUMBER").get< int >(0);
                std::shared_ptr<const Opm::GridProperty<int> > regionProperty = getRegion( record.getItem("REGION_NAME") );
                std::vector<bool> mask;

                regionProperty->initMask( regionValue , mask);

                if (m_intGridProperties->hasKeyword( targetArray )) {
                    if (enabledTypes & IntProperties) {
                        if (isInt( doubleValue )) {
                            std::shared_ptr<Opm::GridProperty<int> > targetProperty = m_intGridProperties->getKeyword(targetArray);
                            int intValue = static_cast<int>( doubleValue + 0.5 );
                            targetProperty->maskedAdd( intValue , mask);
                        } else
                            throw std::invalid_argument("Fatal error processing ADDREG keyword - expected integer value for: " + targetArray);
                    }
                }
                else if (m_doubleGridProperties->hasKeyword( targetArray )) {
                    if (enabledTypes & DoubleProperties) {
                        std::shared_ptr<Opm::GridProperty<double> > targetProperty = m_doubleGridProperties->getKeyword(targetArray);
                        const std::string& dimensionString = targetProperty->getDimensionString();
                        double SIValue = doubleValue * getSIScaling( dimensionString );
                        targetProperty->maskedAdd( SIValue , mask);
                    }
                }
                else {
                    throw std::invalid_argument("Fatal error processing ADDREG keyword - invalid/undefined keyword: " + targetArray);
                }
            }
        }
    }




    void EclipseState::handleMULTIREGKeyword( const DeckKeyword& deckKeyword, int enabledTypes) {
        EclipseGridConstPtr grid = getEclipseGrid();
        for( const auto& record : deckKeyword ) {
            const std::string& targetArray = record.getItem("ARRAY").get< std::string >(0);

            if (!supportsGridProperty( targetArray , IntProperties + DoubleProperties))
                throw std::invalid_argument("Fatal error processing MULTIREG keyword - invalid/undefined keyword: " + targetArray);

            if (supportsGridProperty( targetArray , enabledTypes)) {
                double doubleValue = record.getItem("FACTOR").get< double >(0);
                int regionValue = record.getItem("REGION_NUMBER").get< int >(0);
                std::shared_ptr<const Opm::GridProperty<int> > regionProperty = getRegion( record.getItem("REGION_NAME") );
                std::vector<bool> mask;

                regionProperty->initMask( regionValue , mask);

                if (enabledTypes & IntProperties) {
                    if (isInt( doubleValue )) {
                        std::shared_ptr<Opm::GridProperty<int> > targetProperty = getOrCreateIntProperty_( targetArray );
                        int intValue = static_cast<int>( doubleValue + 0.5 );
                        targetProperty->maskedMultiply( intValue , mask);
                    } else
                        throw std::invalid_argument("Fatal error processing MULTIREG keyword - expected integer value for: " + targetArray);
                }
                if (enabledTypes & DoubleProperties) {
                    std::shared_ptr<Opm::GridProperty<double> > targetProperty = getOrCreateDoubleProperty_(targetArray);
                    targetProperty->maskedMultiply( doubleValue , mask);
                }
            }
        }
    }


    void EclipseState::handleCOPYREGKeyword( const DeckKeyword& deckKeyword, int enabledTypes) {
        EclipseGridConstPtr grid = getEclipseGrid();
        for( const auto& record : deckKeyword ) {
            const std::string& srcArray    = record.getItem("ARRAY").get< std::string >(0);
            const std::string& targetArray = record.getItem("TARGET_ARRAY").get< std::string >(0);

            if (!supportsGridProperty( targetArray , IntProperties + DoubleProperties))
                throw std::invalid_argument("Fatal error processing MULTIREG keyword - invalid/undefined keyword: " + targetArray);

            if (!supportsGridProperty( srcArray , IntProperties + DoubleProperties))
                throw std::invalid_argument("Fatal error processing MULTIREG keyword - invalid/undefined keyword: " + srcArray);

            if (supportsGridProperty( srcArray , enabledTypes)) {
                int regionValue = record.getItem("REGION_NUMBER").get< int >(0);
                std::shared_ptr<const Opm::GridProperty<int> > regionProperty = getRegion( record.getItem("REGION_NAME") );
                std::vector<bool> mask;

                regionProperty->initMask( regionValue , mask );

                if (m_intGridProperties->hasKeyword( srcArray )) {
                    std::shared_ptr<const Opm::GridProperty<int> > srcProperty = m_intGridProperties->getInitializedKeyword( srcArray );
                    if (supportsGridProperty( targetArray , IntProperties)) {
                        std::shared_ptr<Opm::GridProperty<int> > targetProperty = getOrCreateIntProperty_( targetArray );
                        targetProperty->maskedCopy( *srcProperty , mask );
                    } else
                        throw std::invalid_argument("Fatal error processing COPYREG keyword.");
                } else if (m_doubleGridProperties->hasKeyword( srcArray )) {
                    std::shared_ptr<const Opm::GridProperty<double> > srcProperty = m_doubleGridProperties->getInitializedKeyword( srcArray );
                    if (supportsGridProperty( targetArray , DoubleProperties)) {
                        std::shared_ptr<Opm::GridProperty<double> > targetProperty = getOrCreateDoubleProperty_( targetArray );
                        targetProperty->maskedCopy( *srcProperty , mask );
                    }
                }
                else {
                    throw std::invalid_argument("Fatal error processing COPYREG keyword - invalid/undefined keyword: " + targetArray);
                }
            }
        }
    }




    void EclipseState::handleMULTIPLYKeyword( const DeckKeyword& deckKeyword, BoxManager& boxManager, int enabledTypes) {
        for( const auto& record : deckKeyword ) {
            const std::string& field = record.getItem("field").get< std::string >(0);
            double      scaleFactor  = record.getItem("factor").get< double >(0);

            setKeywordBox(deckKeyword, record, boxManager);

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
    void EclipseState::handleADDKeyword( const DeckKeyword& deckKeyword, BoxManager& boxManager, int enabledTypes) {
        for( const auto& record : deckKeyword ) {
            const std::string& field = record.getItem("field").get< std::string >(0);
            double      shiftValue  = record.getItem("shift").get< double >(0);

            setKeywordBox(deckKeyword, record, boxManager);

            if (m_intGridProperties->hasKeyword( field )) {
                if (enabledTypes & IntProperties) {
                    int intShift = static_cast<int>(shiftValue);
                    std::shared_ptr<GridProperty<int> > property = m_intGridProperties->getKeyword( field );

                    property->add( intShift , boxManager.getActiveBox() );
                }
            } else if (m_doubleGridProperties->hasKeyword( field )) {
                if (enabledTypes & DoubleProperties) {
                    std::shared_ptr<GridProperty<double> > property = m_doubleGridProperties->getKeyword( field );

                    double siShiftValue = shiftValue * getSIScaling(property->getDimensionString());
                    property->add(siShiftValue , boxManager.getActiveBox() );
                }
            } else if (!m_intGridProperties->supportsKeyword(field) &&
                       !m_doubleGridProperties->supportsKeyword(field))
                throw std::invalid_argument("Fatal error processing ADD keyword. Tried to shift not defined keyword " + field);

        }
    }


    void EclipseState::handleEQUALSKeyword( const DeckKeyword& deckKeyword, BoxManager& boxManager, int enabledTypes) {
        for( const auto& record : deckKeyword ) {
            const std::string& field = record.getItem("field").get< std::string >(0);
            double      value  = record.getItem("value").get< double >(0);

            setKeywordBox(deckKeyword, record, boxManager);

            if (m_intGridProperties->supportsKeyword( field )) {
                if (enabledTypes & IntProperties) {
                    int intValue = static_cast<int>(value);
                    std::shared_ptr<GridProperty<int> > property = getOrCreateIntProperty_( field );

                    property->setScalar( intValue , boxManager.getActiveBox() );
                }
            } else if (m_doubleGridProperties->supportsKeyword( field )) {

                if (enabledTypes & DoubleProperties) {
                    std::shared_ptr<GridProperty<double> > property = getOrCreateDoubleProperty_( field );

                    double siValue = value * getSIScaling(property->getKeywordInfo().getDimensionString());
                    property->setScalar( siValue , boxManager.getActiveBox() );
                }
            } else
                throw std::invalid_argument("Fatal error processing EQUALS keyword. Tried to set not defined keyword " + field);

        }
    }



    void EclipseState::handleCOPYKeyword( const DeckKeyword& deckKeyword, BoxManager& boxManager, int enabledTypes) {
        for( const auto& record : deckKeyword ) {
            const std::string& srcField = record.getItem("src").get< std::string >(0);
            const std::string& targetField = record.getItem("target").get< std::string >(0);

            setKeywordBox(deckKeyword, record, boxManager);

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
        std::shared_ptr<GridProperty<int> > target    = getOrCreateIntProperty_( targetField );

        target->copyFrom( *src , inputBox );
    }


    void EclipseState::copyDoubleKeyword(const std::string& srcField , const std::string& targetField , std::shared_ptr<const Box> inputBox) {
        std::shared_ptr<const GridProperty<double> > src = m_doubleGridProperties->getKeyword( srcField );
        std::shared_ptr<GridProperty<double> > target    = getOrCreateDoubleProperty_( targetField );

        target->copyFrom( *src , inputBox );
    }



    void EclipseState::setKeywordBox( const DeckKeyword& deckKeyword, const DeckRecord& deckRecord, BoxManager& boxManager) {
        const auto& I1Item = deckRecord.getItem("I1");
        const auto& I2Item = deckRecord.getItem("I2");
        const auto& J1Item = deckRecord.getItem("J1");
        const auto& J2Item = deckRecord.getItem("J2");
        const auto& K1Item = deckRecord.getItem("K1");
        const auto& K2Item = deckRecord.getItem("K2");

        size_t setCount = 0;

        if (!I1Item.defaultApplied(0))
            setCount++;

        if (!I2Item.defaultApplied(0))
            setCount++;

        if (!J1Item.defaultApplied(0))
            setCount++;

        if (!J2Item.defaultApplied(0))
            setCount++;

        if (!K1Item.defaultApplied(0))
            setCount++;

        if (!K2Item.defaultApplied(0))
            setCount++;

        if (setCount == 6) {
            boxManager.setKeywordBox( I1Item.get< int >(0) - 1,
                                      I2Item.get< int >(0) - 1,
                                      J1Item.get< int >(0) - 1,
                                      J2Item.get< int >(0) - 1,
                                      K1Item.get< int >(0) - 1,
                                      K2Item.get< int >(0) - 1);
        } else if (setCount != 0) {
            std::string msg = "BOX modifiers on keywords must be either specified completely or not at all. Ignoring.";
            OpmLog::addMessage(Log::MessageType::Error , Log::fileMessage(deckKeyword.getFileName() , deckKeyword.getLineNumber() , msg));
        }
    }


    void EclipseState::complainAboutAmbiguousKeyword(DeckConstPtr deck, const std::string& keywordName) const {
        OpmLog::addMessage(Log::MessageType::Error, "The " + keywordName + " keyword must be unique in the deck. Ignoring all!");
        auto keywords = deck->getKeywordList(keywordName);
        for (size_t i = 0; i < keywords.size(); ++i) {
            std::string msg = "Ambiguous keyword "+keywordName+" defined here";
            OpmLog::addMessage(Log::MessageType::Error , Log::fileMessage( keywords[i]->getFileName(), keywords[i]->getLineNumber(),msg));
        }
    }




    void EclipseState::applyModifierDeck( std::shared_ptr<const Deck> deck) {
        using namespace ParserKeywords;
        for (const auto& keyword : *deck) {

            if (keyword.isKeyword<MULTFLT>()) {
                for (const auto& record : keyword) {
                    const std::string& faultName = record.getItem<MULTFLT::fault>().get< std::string >(0);
                    auto fault = m_faults->getFault( faultName );
                    double tmpMultFlt = record.getItem<MULTFLT::factor>().get< double >(0);
                    double oldMultFlt = fault->getTransMult( );
                    double newMultFlt = oldMultFlt * tmpMultFlt;

                    /*
                      This extremely contrived way of doing it is because of difference in
                      behavior and section awareness between the Fault object and the
                      Transmult object:

                      1. MULTFLT keywords found in the SCHEDULE section should apply the
                         transmissibility modifiers cumulatively - i.e. the current
                         transmissibility across the fault should be *multiplied* with the
                         newly entered MULTFLT value, and the resulting transmissibility
                         multplier for this fault should be the product of the newly
                         entered value and the current value.

                      2. The TransMult::applyMULTFLT() implementation will *multiply* the
                         transmissibility across a face with the value in the fault
                         object. Hence the current value has already been multiplied in;
                         we therefor first *set* the MULTFLT value to the new value, then
                         apply it to the TransMult object and then eventually update the
                         MULTFLT value in the fault instance.

                    */
                    fault->setTransMult( tmpMultFlt );
                    m_transMult->applyMULTFLT( fault );
                    fault->setTransMult( newMultFlt );
                }
            }
        }
    }

    std::shared_ptr<GridProperty<int> > EclipseState::getOrCreateIntProperty_(const std::string name)
    {
        if (!m_intGridProperties->hasKeyword(name)) {
            m_intGridProperties->addKeyword(name);
        }
        return m_intGridProperties->getKeyword(name);
    }

    std::shared_ptr<GridProperty<double> > EclipseState::getOrCreateDoubleProperty_(const std::string name)
    {
        if (!m_doubleGridProperties->hasKeyword(name)) {
            m_doubleGridProperties->addKeyword(name);
        }
        return m_doubleGridProperties->getKeyword(name);
    }

}
