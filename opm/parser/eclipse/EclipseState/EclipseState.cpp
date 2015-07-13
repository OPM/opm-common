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

#include <iostream>
#include <sstream>
#include <math.h>
#include <boost/algorithm/string/join.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/MULTREGTScanner.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Box.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/BoxManager.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/NNC.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/SatfuncPropertyInitializers.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords.hpp>

#include <opm/parser/eclipse/OpmLog/OpmLog.hpp>


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


    static bool isInt(double value) {
        double diff = fabs(nearbyint(value) - value);

        if (diff < 1e-6)
            return true;
        else
            return false;
    }


    EclipseState::EclipseState(DeckConstPtr deck)
        : m_defaultRegion("FLUXNUM")
    {
        m_deckUnitSystem = deck->getActiveUnitSystem();
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

    std::shared_ptr<const UnitSystem> EclipseState::getDeckUnitSystem() const {
        return m_deckUnitSystem;
    }


    EclipseGridConstPtr EclipseState::getEclipseGrid() const {
        return m_eclipseGrid;
    }

    EclipseGridPtr EclipseState::getEclipseGridCopy() const {
        return std::make_shared<EclipseGrid>( m_eclipseGrid->c_ptr() );
    }

    std::shared_ptr<const Tabdims> EclipseState::getTabdims() const {
        return m_tabdims;
    }

    const std::vector<EnkrvdTable>& EclipseState::getEnkrvdTables() const {
        return m_enkrvdTables;
    }

    const std::vector<EnptvdTable>& EclipseState::getEnptvdTables() const {
        return m_enptvdTables;
    }

    const std::vector<GasvisctTable>& EclipseState::getGasvisctTables() const {
        return m_gasvisctTables;
    }

    const std::vector<ImkrvdTable>& EclipseState::getImkrvdTables() const {
        return m_imkrvdTables;
    }

    const std::vector<ImptvdTable>& EclipseState::getImptvdTables() const {
        return m_imptvdTables;
    }

    const std::vector<OilvisctTable>& EclipseState::getOilvisctTables() const {
        return m_oilvisctTables;
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

    const std::vector<PlyshlogTable>& EclipseState::getPlyshlogTables() const {
        return m_plyshlogTables;
    }

    const std::vector<PlydhflfTable>& EclipseState::getPlydhflfTables() const {
        return m_plydhflfTables;
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

    const std::vector<RtempvdTable>& EclipseState::getRtempvdTables() const {
        return m_rtempvdTables;
    }

    const std::vector<WatvisctTable>& EclipseState::getWatvisctTables() const {
        return m_watvisctTables;
    }

    const std::map<int, VFPProdTable>& EclipseState::getVFPProdTables() const {
        return m_vfpprodTables;
    }

    const std::map<int, VFPInjTable>& EclipseState::getVFPInjTables() const {
        return m_vfpinjTables;
    }

    const std::vector<SgofTable>& EclipseState::getSgofTables() const {
        return m_sgofTables;
    }

    const std::vector<Sof2Table>& EclipseState::getSof2Tables() const {
        return m_sof2Tables;
    }

    const std::vector<SwofTable>& EclipseState::getSwofTables() const {
        return m_swofTables;
    }

    const std::vector<SwfnTable>& EclipseState::getSwfnTables() const {
        return m_swfnTables;
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

    void EclipseState::initTabdims(DeckConstPtr deck) {
        /*
          The default values for the various number of tables is
          embedded in the ParserKeyword("TABDIMS") instance; however
          the EclipseState object does not have a dependency on the
          Parser classes, have therefor decided not to add an explicit
          dependency here, and instead duplicated all the default
          values.
        */
        size_t ntsfun = 1;
        size_t ntpvt = 1;
        size_t nssfun = 1;
        size_t nppvt = 1;
        size_t ntfip = 1;
        size_t nrpvt = 1;

        if (deck->hasKeyword("TABDIMS")) {
            auto keyword = deck->getKeyword("TABDIMS");
            auto record = keyword->getRecord(0);
            ntsfun = record->getItem("NTSFUN")->getInt(0);
            ntpvt  = record->getItem("NTPVT")->getInt(0);
            nssfun = record->getItem("NSSFUN")->getInt(0);
            nppvt  = record->getItem("NPPVT")->getInt(0);
            ntfip  = record->getItem("NTFIP")->getInt(0);
            nrpvt  = record->getItem("NRPVT")->getInt(0);
        }
        m_tabdims = std::make_shared<Tabdims>(ntsfun , ntpvt , nssfun , nppvt , ntfip , nrpvt);
    }


    void EclipseState::initTables(DeckConstPtr deck) {
        initTabdims( deck );
        initSimpleTables(deck, "ENKRVD", m_enkrvdTables);
        initSimpleTables(deck, "ENPTVD", m_enptvdTables);
        initSimpleTables(deck, "IMKRVD", m_imkrvdTables);
        initSimpleTables(deck, "IMPTVD", m_imptvdTables);
        initSimpleTables(deck, "OILVISCT", m_oilvisctTables);
        initSimpleTables(deck, "PLYADS", m_plyadsTables);
        initSimpleTables(deck, "PLYMAX", m_plymaxTables);
        initSimpleTables(deck, "PLYROCK", m_plyrockTables);
        initSimpleTables(deck, "PLYVISC", m_plyviscTables);
        initSimpleTables(deck, "PLYDHFLF", m_plydhflfTables);
        initSimpleTables(deck, "PVDG", m_pvdgTables);
        initSimpleTables(deck, "PVDO", m_pvdoTables);
        initSimpleTables(deck, "RSVD", m_rsvdTables);
        initSimpleTables(deck, "RVVD", m_rvvdTables);
        initSimpleTables(deck, "SGOF", m_sgofTables);
        initSimpleTables(deck, "SOF2", m_sof2Tables);
        initSimpleTables(deck, "SWOF", m_swofTables);
        initSimpleTables(deck, "SWFN", m_swfnTables);
        initSimpleTables(deck, "WATVISCT", m_watvisctTables);

        // the number of columns of the GASVSISCT tables depends on the value of the
        // COMPS keyword...

        initGasvisctTables(deck, "GASVISCT", m_gasvisctTables);

        initPlyshlogTables(deck, "PLYSHLOG", m_plyshlogTables);

        initVFPProdTables(deck, m_vfpprodTables);
        initVFPInjTables(deck,  m_vfpinjTables);

        // the ROCKTAB table comes with additional fun because the number of columns
        //depends on the presence of the RKTRMDIR keyword...
        initRocktabTables(deck);

        // the temperature vs depth table. the problem here is that
        // the TEMPVD (E300) and RTEMPVD (E300 + E100) keywords are
        // synonymous, but we want to provide only a single cannonical
        // API here, so we jump through some small hoops...
        if (deck->hasKeyword("TEMPVD") && deck->hasKeyword("RTEMPVD"))
            throw std::invalid_argument("The TEMPVD and RTEMPVD tables are mutually exclusive!");
        else if (deck->hasKeyword("TEMPVD"))
            initSimpleTables(deck, "TEMPVD", m_rtempvdTables);
        else if (deck->hasKeyword("RTEMPVD"))
            initSimpleTables(deck, "RTEMPVD", m_rtempvdTables);

        initFullTables(deck, "PVTG", m_pvtgTables);
        initFullTables(deck, "PVTO", m_pvtoTables);
   }

    void EclipseState::initIOConfig(DeckConstPtr deck) {
        m_ioConfig = std::make_shared<IOConfig>();
        if (Section::hasGRID(deck)) {
            std::shared_ptr<const GRIDSection> gridSection = std::make_shared<const GRIDSection>(deck);
            m_ioConfig->handleGridSection(gridSection);
        }
        if (Section::hasRUNSPEC(deck)) {
            std::shared_ptr<const RUNSPECSection> runspecSection = std::make_shared<const RUNSPECSection>(deck);
            m_ioConfig->handleRunspecSection(runspecSection);
        }
    }

    void EclipseState::initIOConfigPostSchedule(DeckConstPtr deck) {
        if (Section::hasSOLUTION(deck)) {
            std::shared_ptr<const SOLUTIONSection> solutionSection = std::make_shared<const SOLUTIONSection>(deck);
            m_ioConfig->handleSolutionSection(schedule->getTimeMap(), solutionSection);
        }
    }

    void EclipseState::initInitConfig(DeckConstPtr deck){
        m_initConfig = std::make_shared<const InitConfig>(deck);
    }

    void EclipseState::initSimulationConfig(DeckConstPtr deck) {
        m_simulationConfig = std::make_shared<const SimulationConfig>(deck , m_intGridProperties);
    }


    void EclipseState::initSchedule(DeckConstPtr deck) {
        EclipseGridConstPtr grid = getEclipseGrid();
        schedule = ScheduleConstPtr( new Schedule(grid , deck, m_ioConfig) );
    }

    void EclipseState::initNNC(DeckConstPtr deck) {
        EclipseGridConstPtr grid = getEclipseGrid();
        m_nnc = std::make_shared<NNC>( deck, grid);
    }

    void EclipseState::initTransMult() {
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

    void EclipseState::initFaults(DeckConstPtr deck) {
        EclipseGridConstPtr grid = getEclipseGrid();
        std::shared_ptr<GRIDSection> gridSection = std::make_shared<GRIDSection>( deck );

        m_faults = std::make_shared<FaultCollection>(gridSection , grid);
        setMULTFLT(gridSection);

        if (Section::hasEDIT(deck)) {
            std::shared_ptr<EDITSection> editSection = std::make_shared<EDITSection>( deck );
            setMULTFLT(editSection);
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



    void EclipseState::initMULTREGT(DeckConstPtr deck) {
        EclipseGridConstPtr grid = getEclipseGrid();

        std::vector<Opm::DeckKeywordConstPtr> multregtKeywords;
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
            auto record = gridOpts->getRecord(0);
            auto nrmult_item = record->getItem("NRMULT");

            if (nrmult_item->getInt(0) > 0)
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
            DeckKeywordConstPtr titleKeyword = deck->getKeyword("TITLE");
            DeckRecordConstPtr record = titleKeyword->getRecord(0);
            DeckItemPtr item = record->getItem(0);
            std::vector<std::string> itemValue = item->getStringData();
            m_title = boost::algorithm::join(itemValue, " ");
        }
    }

    void EclipseState::initRocktabTables(DeckConstPtr deck) {
        if (!deck->hasKeyword("ROCKTAB"))
            return; // ROCKTAB is not featured by the deck...

        if (deck->numKeywords("ROCKTAB") > 1) {
            complainAboutAmbiguousKeyword(deck, "ROCKTAB");
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

    void EclipseState::initGasvisctTables(DeckConstPtr deck,
                                          const std::string& keywordName,
                                          std::vector<GasvisctTable>& tableVector) {
        if (!deck->hasKeyword(keywordName))
            return; // the table is not featured by the deck...

        if (deck->numKeywords(keywordName) > 1) {
            complainAboutAmbiguousKeyword(deck, keywordName);
            return;
        }

        const auto& tableKeyword = deck->getKeyword(keywordName);
        for (size_t tableIdx = 0; tableIdx < tableKeyword->size(); ++tableIdx) {
            if (tableKeyword->getRecord(tableIdx)->getItem(0)->size() == 0) {
                // for simple tables, an empty record indicates that the previous table
                // should be copied...
                if (tableIdx == 0) {
                    std::string msg = "The first table for keyword " + keywordName + " must be explicitly defined! Ignoring keyword";
                    OpmLog::addMessage(Log::MessageType::Error , Log::fileMessage(tableKeyword->getFileName(),tableKeyword->getLineNumber(),msg));
                    return;
                }
                tableVector.push_back(tableVector.back());
                continue;
            }

            tableVector.push_back(GasvisctTable());
            tableVector[tableIdx].init(deck, tableKeyword, tableIdx);
        }
    }

    void EclipseState::initPlyshlogTables(DeckConstPtr deck,
                                          const std::string& keywordName,
                                          std::vector<PlyshlogTable>& tableVector){

        if (!deck->hasKeyword(keywordName)) {
            return;
        }

        if (!deck->numKeywords(keywordName)) {
            complainAboutAmbiguousKeyword(deck, keywordName);
            return;
        }

        const auto& keyword = deck->getKeyword(keywordName);

        tableVector.push_back(PlyshlogTable());

        tableVector[0].init(keyword);

    }

    void EclipseState::initVFPProdTables(DeckConstPtr deck,
                                          std::map<int, VFPProdTable>& tableMap) {
        if (!deck->hasKeyword(ParserKeywords::VFPPROD::keywordName)) {
            return;
        }

        int num_tables = deck->numKeywords(ParserKeywords::VFPPROD::keywordName);
        const auto& keywords = deck->getKeywordList<ParserKeywords::VFPPROD>();
        const auto unit_system = deck->getActiveUnitSystem();
        for (int i=0; i<num_tables; ++i) {
            const auto& keyword = keywords[i];

            VFPProdTable table;
            table.init(keyword, unit_system);

            //Check that the table in question has a unique ID
            int table_id = table.getTableNum();
            if (tableMap.find(table_id) == tableMap.end()) {
                tableMap.insert(std::make_pair(table_id, std::move(table)));
            }
            else {
                throw std::invalid_argument("Duplicate table numbers for VFPPROD found");
            }
        }
    }

    void EclipseState::initVFPInjTables(DeckConstPtr deck,
                                        std::map<int, VFPInjTable>& tableMap) {
        if (!deck->hasKeyword(ParserKeywords::VFPINJ::keywordName)) {
            return;
        }

        int num_tables = deck->numKeywords(ParserKeywords::VFPINJ::keywordName);
        const auto& keywords = deck->getKeywordList<ParserKeywords::VFPINJ>();
        const auto unit_system = deck->getActiveUnitSystem();
        for (int i=0; i<num_tables; ++i) {
            const auto& keyword = keywords[i];

            VFPInjTable table;
            table.init(keyword, unit_system);

            //Check that the table in question has a unique ID
            int table_id = table.getTableNum();
            if (tableMap.find(table_id) == tableMap.end()) {
                tableMap.insert(std::make_pair(table_id, std::move(table)));
            }
            else {
                throw std::invalid_argument("Duplicate table numbers for VFPINJ found");
            }
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
        if (!m_intGridProperties->supportsKeyword( keyword ))
            throw std::logic_error("Integer grid property " + keyword + " is unsupported!");

         return m_intGridProperties->hasKeyword( keyword );
    }

    bool EclipseState::hasDoubleGridProperty(const std::string& keyword) const {
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

    std::shared_ptr<GridProperty<int> > EclipseState::getIntGridProperty( const std::string& keyword ) const {
        return m_intGridProperties->getKeyword( keyword );
    }

    std::shared_ptr<GridProperty<double> > EclipseState::getDoubleGridProperty( const std::string& keyword ) const {
        auto gridProperty = m_doubleGridProperties->getKeyword( keyword );
        if (gridProperty->postProcessorRunRequired())
            gridProperty->runPostProcessor();

        return gridProperty;
    }

    std::shared_ptr<GridProperty<int> > EclipseState::getDefaultRegion() const {
        return m_intGridProperties->getInitializedKeyword( m_defaultRegion );
    }

    std::shared_ptr<GridProperty<int> > EclipseState::getRegion(DeckItemConstPtr regionItem) const {
        if (regionItem->defaultApplied(0))
            return getDefaultRegion();
        else {
            const std::string regionArray = MULTREGT::RegionNameFromDeckValue( regionItem->getString(0) );
            return m_intGridProperties->getInitializedKeyword( regionArray );
        }
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
            std::string msg = Log::fileMessage(deckKeyword->getFileName(),
                                               deckKeyword->getLineNumber(),
                                               "Tried to load unsupported grid property from keyword: " + deckKeyword->name());
            OpmLog::addMessage(Log::MessageType::Error , msg);
        }
    }


    void EclipseState::initProperties(DeckConstPtr deck) {
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
        const auto SGLLookup = std::make_shared<SGLEndpointInitializer<>>(*deck, *this);
        const auto ISGLLookup = std::make_shared<ISGLEndpointInitializer<>>(*deck, *this);
        const auto SWLLookup = std::make_shared<SWLEndpointInitializer<>>(*deck, *this);
        const auto ISWLLookup = std::make_shared<ISWLEndpointInitializer<>>(*deck, *this);
        const auto SGULookup = std::make_shared<SGUEndpointInitializer<>>(*deck, *this);
        const auto ISGULookup = std::make_shared<ISGUEndpointInitializer<>>(*deck, *this);
        const auto SWULookup = std::make_shared<SWUEndpointInitializer<>>(*deck, *this);
        const auto ISWULookup = std::make_shared<ISWUEndpointInitializer<>>(*deck, *this);
        const auto SGCRLookup = std::make_shared<SGCREndpointInitializer<>>(*deck, *this);
        const auto ISGCRLookup = std::make_shared<ISGCREndpointInitializer<>>(*deck, *this);
        const auto SOWCRLookup = std::make_shared<SOWCREndpointInitializer<>>(*deck, *this);
        const auto ISOWCRLookup = std::make_shared<ISOWCREndpointInitializer<>>(*deck, *this);
        const auto SOGCRLookup = std::make_shared<SOGCREndpointInitializer<>>(*deck, *this);
        const auto ISOGCRLookup = std::make_shared<ISOGCREndpointInitializer<>>(*deck, *this);
        const auto SWCRLookup = std::make_shared<SWCREndpointInitializer<>>(*deck, *this);
        const auto ISWCRLookup = std::make_shared<ISWCREndpointInitializer<>>(*deck, *this);

        const auto PCWLookup = std::make_shared<PCWEndpointInitializer<>>(*deck, *this);
        const auto IPCWLookup = std::make_shared<IPCWEndpointInitializer<>>(*deck, *this);
        const auto PCGLookup = std::make_shared<PCGEndpointInitializer<>>(*deck, *this);
        const auto IPCGLookup = std::make_shared<IPCGEndpointInitializer<>>(*deck, *this);
        const auto KRWLookup = std::make_shared<KRWEndpointInitializer<>>(*deck, *this);
        const auto IKRWLookup = std::make_shared<IKRWEndpointInitializer<>>(*deck, *this);
        const auto KRWRLookup = std::make_shared<KRWREndpointInitializer<>>(*deck, *this);
        const auto IKRWRLookup = std::make_shared<IKRWREndpointInitializer<>>(*deck, *this);
        const auto KROLookup = std::make_shared<KROEndpointInitializer<>>(*deck, *this);
        const auto IKROLookup = std::make_shared<IKROEndpointInitializer<>>(*deck, *this);
        const auto KRORWLookup = std::make_shared<KRORWEndpointInitializer<>>(*deck, *this);
        const auto IKRORWLookup = std::make_shared<IKRORWEndpointInitializer<>>(*deck, *this);
        const auto KRORGLookup = std::make_shared<KRORGEndpointInitializer<>>(*deck, *this);
        const auto IKRORGLookup = std::make_shared<IKRORGEndpointInitializer<>>(*deck, *this);
        const auto KRGLookup = std::make_shared<KRGEndpointInitializer<>>(*deck, *this);
        const auto IKRGLookup = std::make_shared<IKRGEndpointInitializer<>>(*deck, *this);
        const auto KRGRLookup = std::make_shared<KRGREndpointInitializer<>>(*deck, *this);
        const auto IKRGRLookup = std::make_shared<IKRGREndpointInitializer<>>(*deck, *this);

        const auto tempLookup = std::make_shared<GridPropertyTemperatureLookupInitializer<>>(*deck, *this);
        const auto distributeTopLayer = std::make_shared<const GridPropertyPostProcessor::DistributeTopLayer>(*this);
        const auto initPORV = std::make_shared<GridPropertyPostProcessor::InitPORV>(*this);


        // Note that the variants of grid keywords for radial grids
        // are not supported. (and hopefully never will be)
        typedef GridProperties<double>::SupportedKeywordInfo SupportedDoubleKeywordInfo;
        std::shared_ptr<std::vector<SupportedDoubleKeywordInfo> > supportedDoubleKeywords(new std::vector<SupportedDoubleKeywordInfo>{
            // keywords to specify the scaled connate gas
            // saturations.
            SupportedDoubleKeywordInfo( "SGL"    , SGLLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGLX"   , SGLLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGLX-"  , SGLLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGLY"   , SGLLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGLY-"  , SGLLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGLZ"   , SGLLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGLZ-"  , SGLLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGL"   , ISGLLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGLX"  , ISGLLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGLX-" , ISGLLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGLY"  , ISGLLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGLY-" , ISGLLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGLZ"  , ISGLLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGLZ-" , ISGLLookup, "1" ),

            // keywords to specify the connate water saturation.
            SupportedDoubleKeywordInfo( "SWL"    , SWLLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWLX"   , SWLLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWLX-"  , SWLLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWLY"   , SWLLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWLY-"  , SWLLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWLZ"   , SWLLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWLZ-"  , SWLLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWL"   , ISWLLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWLX"  , ISWLLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWLX-" , ISWLLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWLY"  , ISWLLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWLY-" , ISWLLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWLZ"  , ISWLLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWLZ-" , ISWLLookup, "1" ),

            // keywords to specify the maximum gas saturation.
            SupportedDoubleKeywordInfo( "SGU"    , SGULookup, "1" ),
            SupportedDoubleKeywordInfo( "SGUX"   , SGULookup, "1" ),
            SupportedDoubleKeywordInfo( "SGUX-"  , SGULookup, "1" ),
            SupportedDoubleKeywordInfo( "SGUY"   , SGULookup, "1" ),
            SupportedDoubleKeywordInfo( "SGUY-"  , SGULookup, "1" ),
            SupportedDoubleKeywordInfo( "SGUZ"   , SGULookup, "1" ),
            SupportedDoubleKeywordInfo( "SGUZ-"  , SGULookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGU"   , ISGULookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGUX"  , ISGULookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGUX-" , ISGULookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGUY"  , ISGULookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGUY-" , ISGULookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGUZ"  , ISGULookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGUZ-" , ISGULookup, "1" ),

            // keywords to specify the maximum water saturation.
            SupportedDoubleKeywordInfo( "SWU"    , SWULookup, "1" ),
            SupportedDoubleKeywordInfo( "SWUX"   , SWULookup, "1" ),
            SupportedDoubleKeywordInfo( "SWUX-"  , SWULookup, "1" ),
            SupportedDoubleKeywordInfo( "SWUY"   , SWULookup, "1" ),
            SupportedDoubleKeywordInfo( "SWUY-"  , SWULookup, "1" ),
            SupportedDoubleKeywordInfo( "SWUZ"   , SWULookup, "1" ),
            SupportedDoubleKeywordInfo( "SWUZ-"  , SWULookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWU"   , ISWULookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWUX"  , ISWULookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWUX-" , ISWULookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWUY"  , ISWULookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWUY-" , ISWULookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWUZ"  , ISWULookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWUZ-" , ISWULookup, "1" ),

            // keywords to specify the scaled critical gas
            // saturation.
            SupportedDoubleKeywordInfo( "SGCR"     , SGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGCRX"    , SGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGCRX-"   , SGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGCRY"    , SGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGCRY-"   , SGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGCRZ"    , SGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SGCRZ-"   , SGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGCR"    , ISGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGCRX"   , ISGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGCRX-"  , ISGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGCRY"   , ISGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGCRY-"  , ISGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGCRZ"   , ISGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISGCRZ-"  , ISGCRLookup, "1" ),

            // keywords to specify the scaled critical oil-in-water
            // saturation.
            SupportedDoubleKeywordInfo( "SOWCR"    , SOWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOWCRX"   , SOWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOWCRX-"  , SOWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOWCRY"   , SOWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOWCRY-"  , SOWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOWCRZ"   , SOWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOWCRZ-"  , SOWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCR"   , ISOWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCRX"  , ISOWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCRX-" , ISOWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCRY"  , ISOWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCRY-" , ISOWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCRZ"  , ISOWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOWCRZ-" , ISOWCRLookup, "1" ),

            // keywords to specify the scaled critical oil-in-gas
            // saturation.
            SupportedDoubleKeywordInfo( "SOGCR"    , SOGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOGCRX"   , SOGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOGCRX-"  , SOGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOGCRY"   , SOGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOGCRY-"  , SOGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOGCRZ"   , SOGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SOGCRZ-"  , SOGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCR"   , ISOGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCRX"  , ISOGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCRX-" , ISOGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCRY"  , ISOGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCRY-" , ISOGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCRZ"  , ISOGCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISOGCRZ-" , ISOGCRLookup, "1" ),

            // keywords to specify the scaled critical water
            // saturation.
            SupportedDoubleKeywordInfo( "SWCR"     , SWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWCRX"    , SWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWCRX-"   , SWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWCRY"    , SWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWCRY-"   , SWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWCRZ"    , SWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "SWCRZ-"   , SWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWCR"    , ISWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWCRX"   , ISWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWCRX-"  , ISWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWCRY"   , ISWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWCRY-"  , ISWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWCRZ"   , ISWCRLookup, "1" ),
            SupportedDoubleKeywordInfo( "ISWCRZ-"  , ISWCRLookup, "1" ),

            // keywords to specify the scaled oil-water capillary pressure
            SupportedDoubleKeywordInfo( "PCW"    , PCWLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "PCWX"   , PCWLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "PCWX-"  , PCWLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "PCWY"   , PCWLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "PCWY-"  , PCWLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "PCWZ"   , PCWLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "PCWZ-"  , PCWLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "IPCW"   , IPCWLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "IPCWX"  , IPCWLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "IPCWX-" , IPCWLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "IPCWY"  , IPCWLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "IPCWY-" , IPCWLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "IPCWZ"  , IPCWLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "IPCWZ-" , IPCWLookup, "Pressure" ),

            // keywords to specify the scaled gas-oil capillary pressure
            SupportedDoubleKeywordInfo( "PCG"    , PCGLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "PCGX"   , PCGLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "PCGX-"  , PCGLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "PCGY"   , PCGLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "PCGY-"  , PCGLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "PCGZ"   , PCGLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "PCGZ-"  , PCGLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "IPCG"   , IPCGLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "IPCGX"  , IPCGLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "IPCGX-" , IPCGLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "IPCGY"  , IPCGLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "IPCGY-" , IPCGLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "IPCGZ"  , IPCGLookup, "Pressure" ),
            SupportedDoubleKeywordInfo( "IPCGZ-" , IPCGLookup, "Pressure" ),

            // keywords to specify the scaled water relative permeability
            SupportedDoubleKeywordInfo( "KRW"    , KRWLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRWX"   , KRWLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRWX-"  , KRWLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRWY"   , KRWLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRWY-"  , KRWLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRWZ"   , KRWLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRWZ-"  , KRWLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRW"   , IKRWLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRWX"  , IKRWLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRWX-" , IKRWLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRWY"  , IKRWLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRWY-" , IKRWLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRWZ"  , IKRWLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRWZ-" , IKRWLookup, "1" ),

            // keywords to specify the scaled water relative permeability at the critical
            // saturation
            SupportedDoubleKeywordInfo( "KRWR"    , KRWRLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRWRX"   , KRWRLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRWRX-"  , KRWRLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRWRY"   , KRWRLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRWRY-"  , KRWRLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRWRZ"   , KRWRLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRWRZ-"  , KRWRLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRWR"   , IKRWRLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRWRX"  , IKRWRLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRWRX-" , IKRWRLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRWRY"  , IKRWRLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRWRY-" , IKRWRLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRWRZ"  , IKRWRLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRWRZ-" , IKRWRLookup, "1" ),

            // keywords to specify the scaled oil relative permeability
            SupportedDoubleKeywordInfo( "KRO"    , KROLookup, "1" ),
            SupportedDoubleKeywordInfo( "KROX"   , KROLookup, "1" ),
            SupportedDoubleKeywordInfo( "KROX-"  , KROLookup, "1" ),
            SupportedDoubleKeywordInfo( "KROY"   , KROLookup, "1" ),
            SupportedDoubleKeywordInfo( "KROY-"  , KROLookup, "1" ),
            SupportedDoubleKeywordInfo( "KROZ"   , KROLookup, "1" ),
            SupportedDoubleKeywordInfo( "KROZ-"  , KROLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRO"   , IKROLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKROX"  , IKROLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKROX-" , IKROLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKROY"  , IKROLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKROY-" , IKROLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKROZ"  , IKROLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKROZ-" , IKROLookup, "1" ),

            // keywords to specify the scaled water relative permeability at the critical
            // water saturation
            SupportedDoubleKeywordInfo( "KRORW"    , KRORWLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRORWX"   , KRORWLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRORWX-"  , KRORWLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRORWY"   , KRORWLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRORWY-"  , KRORWLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRORWZ"   , KRORWLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRORWZ-"  , KRORWLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRORW"   , IKRORWLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRORWX"  , IKRORWLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRORWX-" , IKRORWLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRORWY"  , IKRORWLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRORWY-" , IKRORWLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRORWZ"  , IKRORWLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRORWZ-" , IKRORWLookup, "1" ),

            // keywords to specify the scaled water relative permeability at the critical
            // water saturation
            SupportedDoubleKeywordInfo( "KRORG"    , KRORGLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRORGX"   , KRORGLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRORGX-"  , KRORGLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRORGY"   , KRORGLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRORGY-"  , KRORGLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRORGZ"   , KRORGLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRORGZ-"  , KRORGLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRORG"   , IKRORGLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRORGX"  , IKRORGLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRORGX-" , IKRORGLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRORGY"  , IKRORGLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRORGY-" , IKRORGLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRORGZ"  , IKRORGLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRORGZ-" , IKRORGLookup, "1" ),

            // keywords to specify the scaled gas relative permeability
            SupportedDoubleKeywordInfo( "KRG"    , KRGLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRGX"   , KRGLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRGX-"  , KRGLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRGY"   , KRGLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRGY-"  , KRGLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRGZ"   , KRGLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRGZ-"  , KRGLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRG"   , IKRGLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRGX"  , IKRGLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRGX-" , IKRGLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRGY"  , IKRGLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRGY-" , IKRGLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRGZ"  , IKRGLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRGZ-" , IKRGLookup, "1" ),

            // keywords to specify the scaled gas relative permeability
            SupportedDoubleKeywordInfo( "KRGR"    , KRGRLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRGRX"   , KRGRLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRGRX-"  , KRGRLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRGRY"   , KRGRLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRGRY-"  , KRGRLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRGRZ"   , KRGRLookup, "1" ),
            SupportedDoubleKeywordInfo( "KRGRZ-"  , KRGRLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRGR"   , IKRGRLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRGRX"  , IKRGRLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRGRX-" , IKRGRLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRGRY"  , IKRGRLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRGRY-" , IKRGRLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRGRZ"  , IKRGRLookup, "1" ),
            SupportedDoubleKeywordInfo( "IKRGRZ-" , IKRGRLookup, "1" ),

            // cell temperature (E300 only, but makes a lot of sense for E100, too)
            SupportedDoubleKeywordInfo( "TEMPI"    , tempLookup, "Temperature" ),

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
            SupportedDoubleKeywordInfo( "PERMYZ", nan,  distributeTopLayer , "Permeability" ), // E300 only
            SupportedDoubleKeywordInfo( "PERMZX", nan,  distributeTopLayer , "Permeability" ), // E300 only

            // the transmissibility keywords for neighboring
            // conections. note that these keywords don't seem to
            // require a post-processor...
            SupportedDoubleKeywordInfo( "TRANX", nan, "Transmissibility" ),
            SupportedDoubleKeywordInfo( "TRANY", nan, "Transmissibility" ),
            SupportedDoubleKeywordInfo( "TRANZ", nan, "Transmissibility" ),

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
        m_intGridProperties = std::make_shared<GridProperties<int> >(m_eclipseGrid , supportedIntKeywords);
        m_doubleGridProperties = std::make_shared<GridProperties<double> >(m_eclipseGrid , supportedDoubleKeywords);

        // actually create the grid property objects. we need to first
        // process all integer grid properties before the double ones
        // as these may be needed in order to initialize the double
        // properties
        processGridProperties(deck, /*enabledTypes=*/IntProperties);
        processGridProperties(deck, /*enabledTypes=*/DoubleProperties);
    }

    double EclipseState::getSIScaling(const std::string &dimensionString) const
    {
        return m_deckUnitSystem->getDimension(dimensionString)->getSIScaling();
    }

    void EclipseState::processGridProperties(Opm::DeckConstPtr deck, int enabledTypes) {

        if (Section::hasGRID(deck)) {
            std::shared_ptr<Opm::GRIDSection> gridSection(new Opm::GRIDSection(deck) );
            scanSection(gridSection, enabledTypes);
        }


        if (Section::hasEDIT(deck)) {
            std::shared_ptr<Opm::EDITSection> editSection(new Opm::EDITSection(deck) );
            scanSection(editSection, enabledTypes);
        }

        if (Section::hasPROPS(deck)) {
            std::shared_ptr<Opm::PROPSSection> propsSection(new Opm::PROPSSection(deck) );
            scanSection(propsSection, enabledTypes);
        }

        if (Section::hasREGIONS(deck)) {
            std::shared_ptr<Opm::REGIONSSection> regionsSection(new Opm::REGIONSSection(deck) );
            scanSection(regionsSection, enabledTypes);
        }

        if (Section::hasSOLUTION(deck)) {
            std::shared_ptr<Opm::SOLUTIONSection> solutionSection(new Opm::SOLUTIONSection(deck) );
            scanSection(solutionSection,  enabledTypes);
        }
    }

    void EclipseState::scanSection(std::shared_ptr<Opm::Section> section,
                                   int enabledTypes) {
        BoxManager boxManager(m_eclipseGrid->getNX( ) , m_eclipseGrid->getNY() , m_eclipseGrid->getNZ());
        for (auto iter = section->begin(); iter != section->end(); ++iter) {
            DeckKeywordConstPtr deckKeyword = *iter;

            if (supportsGridProperty(deckKeyword->name(), enabledTypes) )
                loadGridPropertyFromDeckKeyword(boxManager.getActiveBox(), deckKeyword,  enabledTypes);
            else {
                if (deckKeyword->name() == "ADD")
                    handleADDKeyword(deckKeyword, boxManager, enabledTypes);

                if (deckKeyword->name() == "BOX")
                    handleBOXKeyword(deckKeyword, boxManager);

                if (deckKeyword->name() == "COPY")
                    handleCOPYKeyword(deckKeyword, boxManager, enabledTypes);

                if (deckKeyword->name() == "EQUALS")
                    handleEQUALSKeyword(deckKeyword, boxManager, enabledTypes);

                if (deckKeyword->name() == "ENDBOX")
                    handleENDBOXKeyword(boxManager);

                if (deckKeyword->name() == "EQUALREG")
                    handleEQUALREGKeyword(deckKeyword ,  enabledTypes);

                if (deckKeyword->name() == "ADDREG")
                    handleADDREGKeyword(deckKeyword , enabledTypes);

                if (deckKeyword->name() == "MULTIREG")
                    handleMULTIREGKeyword(deckKeyword , enabledTypes);

                if (deckKeyword->name() == "COPYREG")
                    handleCOPYREGKeyword(deckKeyword , enabledTypes);

                if (deckKeyword->name() == "MULTIPLY")
                    handleMULTIPLYKeyword(deckKeyword, boxManager, enabledTypes);

                boxManager.endKeyword();
            }
        }
        boxManager.endSection();
    }




    void EclipseState::handleBOXKeyword(DeckKeywordConstPtr deckKeyword,  BoxManager& boxManager) {
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


    void EclipseState::handleEQUALREGKeyword(DeckKeywordConstPtr deckKeyword, int enabledTypes) {
        EclipseGridConstPtr grid = getEclipseGrid();
        for (size_t recordIdx = 0; recordIdx < deckKeyword->size(); ++recordIdx) {
            DeckRecordConstPtr record = deckKeyword->getRecord(recordIdx);
            const std::string& targetArray = record->getItem("ARRAY")->getString(0);

            if (!supportsGridProperty( targetArray , IntProperties + DoubleProperties))
                throw std::invalid_argument("Fatal error processing EQUALREG keyword - invalid/undefined keyword: " + targetArray);

            if (supportsGridProperty( targetArray , enabledTypes)) {
                double doubleValue = record->getItem("VALUE")->getRawDouble(0);
                int regionValue = record->getItem("REGION_NUMBER")->getInt(0);
                std::shared_ptr<Opm::GridProperty<int> > regionProperty = getRegion( record->getItem("REGION_NAME") );
                std::vector<bool> mask;

                regionProperty->initMask( regionValue , mask);

                if (m_intGridProperties->supportsKeyword( targetArray )) {
                    if (enabledTypes & IntProperties) {
                        if (isInt( doubleValue )) {
                            std::shared_ptr<Opm::GridProperty<int> > targetProperty = m_intGridProperties->getKeyword(targetArray);
                            int intValue = static_cast<int>( doubleValue + 0.5 );
                            targetProperty->maskedSet( intValue , mask);
                        } else
                            throw std::invalid_argument("Fatal error processing EQUALREG keyword - expected integer value for: " + targetArray);
                    }
                }
                else if (m_doubleGridProperties->supportsKeyword( targetArray )) {
                    if (enabledTypes & DoubleProperties) {
                        std::shared_ptr<Opm::GridProperty<double> > targetProperty = m_doubleGridProperties->getKeyword(targetArray);
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
    }


    void EclipseState::handleADDREGKeyword(DeckKeywordConstPtr deckKeyword, int enabledTypes) {
        EclipseGridConstPtr grid = getEclipseGrid();
        for (size_t recordIdx = 0; recordIdx < deckKeyword->size(); ++recordIdx) {
            DeckRecordConstPtr record = deckKeyword->getRecord(recordIdx);
            const std::string& targetArray = record->getItem("ARRAY")->getString(0);

            if (!supportsGridProperty( targetArray , IntProperties + DoubleProperties))
                throw std::invalid_argument("Fatal error processing ADDREG keyword - invalid/undefined keyword: " + targetArray);

            if (supportsGridProperty( targetArray , enabledTypes)) {
                double doubleValue = record->getItem("SHIFT")->getRawDouble(0);
                int regionValue = record->getItem("REGION_NUMBER")->getInt(0);
                std::shared_ptr<Opm::GridProperty<int> > regionProperty = getRegion( record->getItem("REGION_NAME") );
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




    void EclipseState::handleMULTIREGKeyword(DeckKeywordConstPtr deckKeyword, int enabledTypes) {
        EclipseGridConstPtr grid = getEclipseGrid();
        for (size_t recordIdx = 0; recordIdx < deckKeyword->size(); ++recordIdx) {
            DeckRecordConstPtr record = deckKeyword->getRecord(recordIdx);
            const std::string& targetArray = record->getItem("ARRAY")->getString(0);

            if (!supportsGridProperty( targetArray , IntProperties + DoubleProperties))
                throw std::invalid_argument("Fatal error processing MULTIREG keyword - invalid/undefined keyword: " + targetArray);

            if (supportsGridProperty( targetArray , enabledTypes)) {
                double doubleValue = record->getItem("FACTOR")->getRawDouble(0);
                int regionValue = record->getItem("REGION_NUMBER")->getInt(0);
                std::shared_ptr<Opm::GridProperty<int> > regionProperty = getRegion( record->getItem("REGION_NAME") );
                std::vector<bool> mask;

                regionProperty->initMask( regionValue , mask);

                if (m_intGridProperties->hasKeyword( targetArray )) {
                    if (enabledTypes & IntProperties) {
                        if (isInt( doubleValue )) {
                            std::shared_ptr<Opm::GridProperty<int> > targetProperty = m_intGridProperties->getKeyword( targetArray );
                            int intValue = static_cast<int>( doubleValue + 0.5 );
                            targetProperty->maskedMultiply( intValue , mask);
                        } else
                            throw std::invalid_argument("Fatal error processing MULTIREG keyword - expected integer value for: " + targetArray);
                    }
                }
                else if (m_doubleGridProperties->hasKeyword( targetArray )) {
                    if (enabledTypes & DoubleProperties) {
                        std::shared_ptr<Opm::GridProperty<double> > targetProperty = m_doubleGridProperties->getKeyword(targetArray);
                        targetProperty->maskedMultiply( doubleValue , mask);
                    }
                }
                else {
                    throw std::invalid_argument("Fatal error processing MULTIREG keyword - invalid/undefined keyword: " + targetArray);
                }
            }
        }
    }


    void EclipseState::handleCOPYREGKeyword(DeckKeywordConstPtr deckKeyword, int enabledTypes) {
        EclipseGridConstPtr grid = getEclipseGrid();
        for (size_t recordIdx = 0; recordIdx < deckKeyword->size(); ++recordIdx) {
            DeckRecordConstPtr record = deckKeyword->getRecord(recordIdx);
            const std::string& srcArray    = record->getItem("ARRAY")->getString(0);
            const std::string& targetArray = record->getItem("TARGET_ARRAY")->getString(0);

            if (!supportsGridProperty( targetArray , IntProperties + DoubleProperties))
                throw std::invalid_argument("Fatal error processing MULTIREG keyword - invalid/undefined keyword: " + targetArray);

            if (!supportsGridProperty( srcArray , IntProperties + DoubleProperties))
                throw std::invalid_argument("Fatal error processing MULTIREG keyword - invalid/undefined keyword: " + srcArray);

            if (supportsGridProperty( srcArray , enabledTypes)) {
                int regionValue = record->getItem("REGION_NUMBER")->getInt(0);
                std::shared_ptr<Opm::GridProperty<int> > regionProperty = getRegion( record->getItem("REGION_NAME") );
                std::vector<bool> mask;

                regionProperty->initMask( regionValue , mask );

                if (m_intGridProperties->hasKeyword( srcArray )) {
                    std::shared_ptr<Opm::GridProperty<int> > srcProperty = m_intGridProperties->getInitializedKeyword( srcArray );
                    if (supportsGridProperty( targetArray , IntProperties)) {
                        std::shared_ptr<Opm::GridProperty<int> > targetProperty = m_intGridProperties->getKeyword( targetArray );
                        targetProperty->maskedCopy( *srcProperty , mask );
                    } else
                        throw std::invalid_argument("Fatal error processing COPYREG keyword.");
                } else if (m_doubleGridProperties->hasKeyword( srcArray )) {
                    std::shared_ptr<Opm::GridProperty<double> > srcProperty = m_doubleGridProperties->getInitializedKeyword( srcArray );
                    if (supportsGridProperty( targetArray , DoubleProperties)) {
                        std::shared_ptr<Opm::GridProperty<double> > targetProperty = m_doubleGridProperties->getKeyword( targetArray );
                        targetProperty->maskedCopy( *srcProperty , mask );
                    }
                }
                else {
                    throw std::invalid_argument("Fatal error processing COPYREG keyword - invalid/undefined keyword: " + targetArray);
                }
            }
        }
    }




    void EclipseState::handleMULTIPLYKeyword(DeckKeywordConstPtr deckKeyword, BoxManager& boxManager, int enabledTypes) {
        for (size_t recordIdx = 0; recordIdx < deckKeyword->size(); ++recordIdx) {
            DeckRecordConstPtr record = deckKeyword->getRecord(recordIdx);
            const std::string& field = record->getItem("field")->getString(0);
            double      scaleFactor  = record->getItem("factor")->getRawDouble(0);

            setKeywordBox(deckKeyword, recordIdx, boxManager);

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
    void EclipseState::handleADDKeyword(DeckKeywordConstPtr deckKeyword, BoxManager& boxManager, int enabledTypes) {
        for (size_t recordIdx = 0; recordIdx < deckKeyword->size(); ++recordIdx) {
            DeckRecordConstPtr record = deckKeyword->getRecord(recordIdx);
            const std::string& field = record->getItem("field")->getString(0);
            double      shiftValue  = record->getItem("shift")->getRawDouble(0);

            setKeywordBox(deckKeyword, recordIdx, boxManager);

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


    void EclipseState::handleEQUALSKeyword(DeckKeywordConstPtr deckKeyword, BoxManager& boxManager, int enabledTypes) {
        for (size_t recordIdx = 0; recordIdx < deckKeyword->size(); ++recordIdx) {
            DeckRecordConstPtr record = deckKeyword->getRecord(recordIdx);
            const std::string& field = record->getItem("field")->getString(0);
            double      value  = record->getItem("value")->getRawDouble(0);

            setKeywordBox(deckKeyword, recordIdx, boxManager);

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



    void EclipseState::handleCOPYKeyword(DeckKeywordConstPtr deckKeyword, BoxManager& boxManager, int enabledTypes) {
        for (size_t recordIdx = 0; recordIdx < deckKeyword->size(); ++recordIdx) {
            DeckRecordConstPtr record = deckKeyword->getRecord(recordIdx);
            const std::string& srcField = record->getItem("src")->getString(0);
            const std::string& targetField = record->getItem("target")->getString(0);

            setKeywordBox(deckKeyword, recordIdx, boxManager);

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



    void EclipseState::setKeywordBox(DeckKeywordConstPtr deckKeyword, size_t recordIdx, BoxManager& boxManager) {
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
        } else if (setCount != 0) {
            std::string msg = "BOX modifiers on keywords must be either specified completely or not at all. Ignoring.";
            OpmLog::addMessage(Log::MessageType::Error , Log::fileMessage(deckKeyword->getFileName() , deckKeyword->getLineNumber() , msg));
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

}
