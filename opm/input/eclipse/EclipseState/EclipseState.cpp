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

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/common/ErrorMacros.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/InfoLogger.hpp>
#include <opm/common/OpmLog/LogUtil.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/io/eclipse/rst/aquifer.hpp>
#include <opm/io/eclipse/rst/network.hpp>
#include <opm/io/eclipse/ERst.hpp>

#include <opm/input/eclipse/EclipseState/Grid/Box.hpp>
#include <opm/input/eclipse/EclipseState/Grid/BoxManager.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/Fault.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FaultCollection.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FIPRegionStatistics.hpp>
#include <opm/input/eclipse/EclipseState/Grid/MULTREGTScanner.hpp>
#include <opm/input/eclipse/EclipseState/Grid/NNC.hpp>
#include <opm/input/eclipse/EclipseState/Grid/SatfuncPropertyInitializers.hpp>
#include <opm/input/eclipse/EclipseState/Grid/TransMult.hpp>
#include <opm/input/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/input/eclipse/EclipseState/InitConfig/InitConfig.hpp>
#include <opm/input/eclipse/EclipseState/SimulationConfig/SimulationConfig.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#include <opm/input/eclipse/Units/Dimension.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <opm/input/eclipse/Deck/DeckSection.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/M.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/R.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/T.hpp>

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <cstddef>
#include <filesystem>
#include <map>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace {
    void verify_consistent_restart_information(const Opm::DeckKeyword& restart_keyword,
                                               const Opm::IOConfig&    io_config,
                                               const Opm::InitConfig&  init_config)
    {
        const auto  report_step = init_config.getRestartStep();
        const auto& restart_file = io_config
            .getRestartFileName(init_config.getRestartRootName(), report_step, false);

        if (!std::filesystem::exists(restart_file)) {
            throw Opm::OpmInputError {
                fmt::format("The restart file {} does not exist", restart_file),
                restart_keyword.location()
            };
        }

        if (io_config.getUNIFIN()) {
            const Opm::EclIO::ERst rst{restart_file};

            if (!rst.hasReportStepNumber(report_step)) {
                throw Opm::OpmInputError {
                    fmt::format("Report step {} not found in restart file {}",
                                report_step, restart_file),
                    restart_keyword.location()
                };
            }
        }
    }
}

namespace Opm {

// The field_props and grid both have a relationship to the number of active
// cells, and update eachother through an inelegant dance through the
// EclispeState construction:
//
// 1. The grid is created is with the explicit ACTNUM information found in
//    the deck, including the actual ACTNUM keyword and direct ACTNUM data
//    entered in EQUALS or COPY.
//
// 2. A FieldPropsManager is created based on this initial grid.  In this
//    manager the grid plays an essential role in mapping active/inactive
//    cells.  The FieldPropsManager::actnum() function will create a new
//    ACTNUM vector based on:
//
//      1. The ACTNUM mapping from the original grid.
//      2. Direct ACTNUM manipulations.
//      3. Cells with PORV == 0
//
//    The new actnum vector will be returned by value and not used
//    internally in the fieldprops.
//
// 3. We update the grid with the new ACTNUM provided by the field props
//    manager.
//
// 4. We update the fieldprops with the ACTNUM.  Once we reach this point no
//    deactivated cell must be reactivated as a result of other processing.
//    We do support active cells becoming deactivated though--e.g., through
//    MINPV.
//
// During the EclipseState construction the grid <-> field_props update
// process is done twice, first after the initial field_props processing and
// subsequently after the processing of numerical aquifers.

    EclipseState::EclipseState(const Deck& deck)
    try
        : m_tables(            deck )
        , m_runspec(           deck )
        , m_eclipseConfig(     deck )
        , m_deckUnitSystem(    deck.getActiveUnitSystem() )
        , m_inputGrid(         deck, nullptr )
        , m_inputNnc(          m_inputGrid, deck)
        , m_gridDims(          deck )
        , field_props(         deck, m_runspec.phases(), m_inputGrid, m_tables, m_runspec.numComps())
        , m_simulationConfig(  m_eclipseConfig.init().restartRequested(), deck, field_props)
        , aquifer_config(      m_tables, m_inputGrid, deck, field_props)
        , compositional_config(deck, m_runspec)
        , m_transMult(         GridDims(deck), deck, field_props)
        , tracer_config(       m_deckUnitSystem, deck)
        , wag_hyst_config(     deck)
        , co2_store_config(    deck)
    {
        this->assignRunTitle(deck);
        this->reportNumberOfActivePhases();

        if (field_props.has_double("MINPVV")) {
            this->m_inputGrid.setMINPVV(field_props.get_global_double("MINPVV"));
        }
        this->conveyNumericalAquiferEffects();
        if (field_props.has_double("MINPVV")) {
            field_props.deleteMINPVV();
        }
        this->initLgrs(deck);
        this->aquifer_config.load_connections(deck, this->getInputGrid());

        this->applyMULTXYZ();
        this->initFaults(deck);
        m_simulationConfig.m_ThresholdPressure.readFaults(deck,m_faults);

        if (this->getInitConfig().restartRequested()) {
            verify_consistent_restart_information(deck.get<ParserKeywords::RESTART>().back(),
                                                  this->getIOConfig(), this->getInitConfig());
        }
    }
    catch (const OpmInputError& opm_error) {
        OpmLog::error(opm_error.what());
        throw;
    }
    catch (const std::exception& std_error) {
        OpmLog::error(fmt::format("\nAn error occurred while creating the reservoir properties\n"
                                  "Internal error: {}\n", std_error.what()));
        throw;
    }



    const UnitSystem& EclipseState::getDeckUnitSystem() const {
        return m_deckUnitSystem;
    }

    const UnitSystem& EclipseState::getUnits() const {
        return m_deckUnitSystem;
    }

    const EclipseGrid& EclipseState::getInputGrid() const {
        return m_inputGrid;
    }


    const SimulationConfig& EclipseState::getSimulationConfig() const {
        return m_simulationConfig;
    }


    const FieldPropsManager& EclipseState::fieldProps() const {
        return this->field_props;
    }

    const FieldPropsManager& EclipseState::globalFieldProps() const {
        return this->field_props;
    }

    void EclipseState::computeFipRegionStatistics()
    {
        if (! this->fipRegionStatistics_.has_value()) {
            this->fipRegionStatistics_
                .emplace(declaredMaxRegionID(this->runspec()),
                         this->fieldProps(),
                         [](std::vector<int>&) { /* do nothing*/ });
        }
    }

    const FIPRegionStatistics& EclipseState::fipRegionStatistics() const
    {
        if (! this->fipRegionStatistics_.has_value()) {
            throw std::logic_error {
                "FIP Region Statistics have not been prepared"
            };
        }

        return *this->fipRegionStatistics_;
    }

    const TableManager& EclipseState::getTableManager() const {
        return m_tables;
    }


    /// [[deprecated]] --- use cfg().io()
    const IOConfig& EclipseState::getIOConfig() const {
        return m_eclipseConfig.io();
    }

    /// [[deprecated]] --- use cfg().io()
    IOConfig& EclipseState::getIOConfig() {
        return m_eclipseConfig.io();
    }

    /// [[deprecated]] --- use cfg().init()
    const InitConfig& EclipseState::getInitConfig() const {
        return m_eclipseConfig.init();
    }

    /// [[deprecated]] --- use cfg().init()
    InitConfig& EclipseState::getInitConfig() {
        return m_eclipseConfig.init();
    }
    /// [[deprecated]] --- use cfg()
    const EclipseConfig& EclipseState::getEclipseConfig() const {
        return cfg();
    }

    const EclipseConfig& EclipseState::cfg() const {
        return m_eclipseConfig;
    }

    const GridDims& EclipseState::gridDims() const {
        return m_gridDims;
    }

    const Runspec& EclipseState::runspec() const {
        return this->m_runspec;
    }

    const FaultCollection& EclipseState::getFaults() const {
        return m_faults;
    }

    const LgrCollection& EclipseState::getLgrs() const {
        return m_lgrs;
    }

    const WagHysteresisConfig& EclipseState::getWagHysteresis() const {
        return wag_hyst_config;
    }

    const Co2StoreConfig& EclipseState::getCo2StoreConfig() const {
        return co2_store_config;
    }

    const TransMult& EclipseState::getTransMult() const {
        return m_transMult;
    }

    TransMult& EclipseState::getTransMult() {
        return m_transMult;
    }

    const NNC& EclipseState::getInputNNC() const {
        return m_inputNnc;
    }

    const std::vector<NNCdata>& EclipseState::getPinchNNC() const {
        return m_pinchNnc;
    }

    void EclipseState::setInputNNC(const NNC& nnc) {
        m_inputNnc = nnc;
    }

    void EclipseState::setPinchNNC(std::vector<NNCdata>&& nnc) {
        m_pinchNnc = nnc;
        std::sort(m_pinchNnc.begin(), m_pinchNnc.end());
    }

    void EclipseState::appendInputNNC(const std::vector<NNCdata>& nnc) {
        for (const auto& nnc_data : nnc ) {
            this->m_inputNnc.addNNC(nnc_data.cell1, nnc_data.cell2, nnc_data.trans);
        }
    }

    bool EclipseState::hasInputNNC() const {
        return !m_inputNnc.input().empty();
    }

    bool EclipseState::hasPinchNNC() const {
        return !m_pinchNnc.empty();
    }

    bool EclipseState::hasInputLGR() const {
        return m_lgrs.size() != 0;
    }

    void EclipseState::initLgrs(const Deck& deck) {
        if (!DeckSection::hasGRID(deck))
            return;

        const GRIDSection gridSection ( deck );

        m_lgrs = LgrCollection(gridSection, m_inputGrid);
        m_inputGrid.init_lgr_cells(m_lgrs);
    }


    const std::string& EclipseState::getTitle() const
    {
        return m_title;
    }

    const AquiferConfig& EclipseState::aquifer() const {
        return this->aquifer_config;
    }

    const CompositionalConfig& EclipseState::compositionalConfig() const {
        return this->compositional_config;
    }

    const TracerConfig& EclipseState::tracer() const {
        return this->tracer_config;
    }

    void EclipseState::prune_global_for_schedule_run()
    {
        this->field_props.prune_global_for_schedule_run();
    }

    void EclipseState::reset_actnum(const std::vector<int>& new_actnum) {
        this->field_props.reset_actnum(new_actnum);
    }

    void EclipseState::set_active_indices(const std::vector<int>& indices)
    {
        this->field_props.set_active_indices(indices);
    }

    void EclipseState::pruneDeactivatedAquiferConnections(const std::vector<std::size_t>& deactivated_cells) {
        if (this->aquifer_config.hasAnalyticalAquifer())
            this->aquifer_config.pruneDeactivatedAquiferConnections(deactivated_cells);
    }

    void EclipseState::loadRestartAquifers(const RestartIO::RstAquifer& aquifers) {
        if (aquifers.hasAnalyticAquifers())
            this->aquifer_config.loadFromRestart(aquifers, this->m_tables);
    }

    void EclipseState::appendAqufluxSchedule(const std::unordered_set<int>& ids) {
        this->aquifer_config.appendAqufluxSchedule(ids);
    }

    void EclipseState::loadRestartNetworkPressures(const RestartIO::RstNetwork& network) {
        if (!network.isActive()) return;

        this->m_restart_network_pressures = std::map<std::string, double>{};
        auto& node_pressures = this->m_restart_network_pressures.value();
        for (const auto& node : network.nodes()) {
            node_pressures[node.name] = node.pressure;
        }
    }

    void EclipseState::assignRunTitle(const Deck& deck)
    {
        if (! deck.hasKeyword<ParserKeywords::TITLE>()) {
            return;
        }

        const auto& title = deck[ParserKeywords::TITLE::keywordName]
            .back().getRecord(0).getItem(0);

        this->m_title = fmt::format("{}", fmt::join(title.getData<std::string>(), " "));
    }

    void EclipseState::reportNumberOfActivePhases() const
    {
        const auto nph = this->runspec().phases().size();
        const auto is_single_phase = nph == 1;
        const auto plural1 = is_single_phase ? std::string{""}   : std::string{"s"};
        const auto plural2 = is_single_phase ? std::string{"is"} : std::string{"are"};

        OpmLog::info(fmt::format("{} fluid phase{} {} active", nph, plural1, plural2));
    }

    void EclipseState::conveyNumericalAquiferEffects()
    {
        if (! this->aquifer_config.hasNumericalAquifer()) {
            return;
        }

        auto& numerical_aquifer = this->aquifer_config.mutableNumericalAquifers();

        numerical_aquifer.applyMinPV(this->m_inputGrid);

        // Update field_props for numerical aquifer cells and set the
        // transmissiblity related to aquifer cells to zero.
        this->field_props.apply_numerical_aquifers(numerical_aquifer);

        // Add NNCs between aquifer cells and first aquifer cell and aquifer
        // connections.
        this->appendInputNNC(numerical_aquifer.aquiferCellNNCs());

        this->m_transMult.applyNumericalAquifer(numerical_aquifer.allAquiferCellIds());
    }

    void EclipseState::applyMULTXYZ() {
        static const std::vector<std::pair<std::string, FaceDir::DirEnum>> multipliers = {{"MULTX" , FaceDir::XPlus},
                                                                                          {"MULTX-", FaceDir::XMinus},
                                                                                          {"MULTY" , FaceDir::YPlus},
                                                                                          {"MULTY-", FaceDir::YMinus},
                                                                                          {"MULTZ" , FaceDir::ZPlus},
                                                                                          {"MULTZ-", FaceDir::ZMinus}};

        const auto& fp = this->field_props;
        for (const auto& [field, face] : multipliers) {
            if (fp.has_double(field))
            {
                this->m_transMult.applyMULT(fp.get_global_double(field), face);
            }
        }
    }

    void EclipseState::initFaults(const Deck& deck) {
        if (!DeckSection::hasGRID(deck))
            return;

        const GRIDSection gridSection ( deck );

        m_faults = FaultCollection(gridSection, m_inputGrid);
        setMULTFLT(gridSection);

        if (DeckSection::hasEDIT(deck)) {
            setMULTFLT(EDITSection(deck), true);
        }

        m_transMult.applyMULTFLT( m_faults );
    }


    void EclipseState::setMULTFLT(const DeckSection& section, bool edit) {
        // Set error to false
        bool error = false;
        std::map<std::string,double> prev;
        for (std::size_t index = 0; index < section.count("MULTFLT"); index++) {
            const auto& faultsKeyword = section.getKeyword("MULTFLT" , index);
            OpmLog::info(OpmInputError::format("\nApplying {keyword} in {file} line {line}", faultsKeyword.location()));
            InfoLogger logger("MULTFLT",3);
            for (auto iter = faultsKeyword.begin(); iter != faultsKeyword.end(); ++iter) {
                const auto& faultRecord = *iter;
                const std::string& faultPattern = faultRecord.getItem(0).get< std::string >(0);
                const double multFlt = faultRecord.getItem(1).get< double >(0);
                try
                {
                    for (const auto& faultName : m_faults.getFaults(faultPattern)) {
                        double multFltEdit = multFlt;
                        if (edit) {
                            const auto it = prev.find(faultName);
                            const auto& fault = m_faults.getFault(faultName);
                            if (it == prev.end()) {
                                prev[faultName] = fault.getTransMult();
                                multFltEdit *= m_faults.getFault(faultName).getTransMult();
                            } else
                                multFltEdit *= it->second;
                        }
                        m_faults.setTransMult(faultName, multFltEdit);
                        logger(fmt::format("Setting fault transmissibility multiplier {} for fault {}", multFlt, faultName));
                    }
                }
                catch (const std::exception& std_error)
                {
                    OpmLog::error(fmt::format("\nMULTFLT: Cannot set fault transmissibility multiplier\n" 
                       "MULTFLT(FLTNAME) equals {} and MULT(FLT-TRS) equals {}\n"
                       "Error creating reservoir properties: {}" , faultPattern, multFlt, std_error.what()));
                    error = true;

                }
            }
        }
        // Throw if errors
        if (error) {
            throw std::invalid_argument("Error Processing MULTFLT");
        }
    }


    void EclipseState::complainAboutAmbiguousKeyword(const Deck& deck, const std::string& keywordName) {
        OpmLog::error("The " + keywordName + " keyword must be unique in the deck. Ignoring all!");
        const auto keywords = deck.getKeywordList(keywordName);
        for (const auto* kw : keywords) {
            const std::string msg = "Ambiguous keyword " + keywordName + " defined here";
            OpmLog::error(Log::fileMessage(kw->location(), msg));
        }
    }


    /*
      The apply_schedule_keywords can apply a small set of keywords from the
      Schdule section for transmissibility scaling; the currently supported
      keywords are: {MULTFLT, MULTX, MULTX-, MULTY, MULTY-, MULTZ, MULTZ-}.

      Observe that the multiplier scalars which are in the schedule section are
      applied by multiplying with the transmissibility which has already been
      calculated, i.e. to increase the permeability you must use a multiplier
      greater than one.
    */
    void EclipseState::apply_schedule_keywords(const std::vector<DeckKeyword>& keywords) {
        using namespace ParserKeywords;
        static const std::unordered_set<std::string> multipliers = {"MULTFLT", "MULTX", "MULTX-", "MULTY", "MULTY-", "MULTZ", "MULTZ-"};
        for (const auto& keyword : keywords) {
            if (keyword.is<MULTFLT>()) {
                for (const auto& record : keyword) {
                    const std::string& faultName = record.getItem<MULTFLT::fault>().get< std::string >(0);
                    auto& fault = m_faults.getFault( faultName );
                    auto multflt = record.getItem<MULTFLT::factor>().get< double >(0);

                    fault.setTransMult( multflt );
                    m_transMult.applyMULTFLT( fault );
                }
            }

            if (multipliers.count(keyword.name()) == 1)
                OpmLog::info(fmt::format("Apply transmissibility multiplier: {}", keyword.name()));
        }

        // After loadbalancing field_props is a nullptr on all processes except
        // the one with rank zero. Currently, the simulator should to take care
        // about communicating the field properties. I does not seem to do that,
        // though. Only the transmissibility multipliers will get broadcasted.
        if (this->field_props.is_usable())
        {
            this->field_props.apply_schedule_keywords(keywords);
            this->applyMULTXYZ();
        }
    }


namespace {

template <typename T>
bool rst_cmp_obj(const T& full_arg, const T& rst_arg, const std::string& object_name) {
    if (full_arg == rst_arg)
        return true;

    fmt::print("Difference in {}\n", object_name);
    return false;
}

}
    bool EclipseState::rst_cmp(const EclipseState& full_state, const EclipseState& rst_state) {
        return Runspec::rst_cmp(full_state.m_runspec, rst_state.m_runspec) &&
            EclipseConfig::rst_cmp(full_state.m_eclipseConfig, rst_state.m_eclipseConfig) &&
            UnitSystem::rst_cmp(full_state.m_deckUnitSystem, rst_state.m_deckUnitSystem) &&
            FieldPropsManager::rst_cmp(full_state.field_props, rst_state.field_props) &&
            SimulationConfig::rst_cmp(full_state.m_simulationConfig, rst_state.m_simulationConfig) &&
            rst_cmp_obj(full_state.m_tables, rst_state.m_tables, "Tables") &&
            rst_cmp_obj(full_state.m_inputGrid, rst_state.m_inputGrid, "Inputgrid") &&
            rst_cmp_obj(full_state.m_inputNnc, rst_state.m_inputNnc, "NNC") &&
            rst_cmp_obj(full_state.m_gridDims, rst_state.m_gridDims, "Grid dims") &&
            rst_cmp_obj(full_state.aquifer_config, rst_state.aquifer_config, "AquiferConfig") &&
            rst_cmp_obj(full_state.compositional_config, rst_state.compositional_config, "CompositionalConfig") &&
            rst_cmp_obj(full_state.m_transMult, rst_state.m_transMult, "TransMult") &&
            rst_cmp_obj(full_state.m_faults, rst_state.m_faults, "Faults") &&
            rst_cmp_obj(full_state.m_title, rst_state.m_title, "Title") &&
            rst_cmp_obj(full_state.tracer_config, rst_state.tracer_config, "Tracer");
    }

}
