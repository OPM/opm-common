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

#include <filesystem>
#include <set>

#include <fmt/format.h>

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/InfoLogger.hpp>
#include <opm/common/OpmLog/LogUtil.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/io/eclipse/rst/aquifer.hpp>
#include <opm/io/eclipse/ERst.hpp>

#include <opm/parser/eclipse/Deck/DeckSection.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Box.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/BoxManager.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FaultCollection.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Fault.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/MULTREGTScanner.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/NNC.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/SatfuncPropertyInitializers.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/TransMult.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/InitConfig.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/parser/eclipse/EclipseState/SimulationConfig/SimulationConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/M.hpp>
#include <opm/parser/eclipse/Units/Dimension.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

namespace Opm {


namespace {

/*
  The field_props and grid both have a relationship to the number of active
  cells, and update eachother through an inelegant dance through the
  EclispeState construction:

  1. The grid is created is with the explicit ACTNUM keyword found in the deck.
     This does *not* include ACTNUM data which is entered via e.g. EQUALS or
     COPY keywords.

  2. A FieldPropsManager is created based on the initial grid. In this manager
     the grid plays an essential role in mapping active/inactive cells. While
     assembling the property information the fieldprops manager can encounter
     ACTNUM modifications and also PORO / PORV. The FieldPropsManager::actnum()
     function will create a new actnum vector based on:

       1. The ACTNUM from the original grid.
       2. Direct ACTNUM manipulations.
       3. Cells with PORV == 0

     The new actnum vector will be returned by value and not used internally in
     the fieldprops.

  3. We update the grid with the new ACTNUM provided by the field props manager.

  4. We update the fieldprops with the ACTNUM.

  During the EclipseState construction the grid <-> field_props update process
  is done twice, first after the initial field_props processing and subsequently
  after the processing of numerical aquifers.
*/


void update_active_cells(EclipseGrid& grid, FieldPropsManager& fp) {
    grid.resetACTNUM(fp.actnum());
    fp.reset_actnum(grid.getACTNUM());
}

AquiferConfig load_aquifers(const Deck& deck, const TableManager& tables, NNC& input_nnc, EclipseGrid& grid, FieldPropsManager& fp) {
    auto aquifer_config = AquiferConfig(tables, grid, deck, fp);

    if (aquifer_config.hasNumericalAquifer()) {
        const auto& numerical_aquifer = aquifer_config.numericalAquifers();
        // update field_props for numerical aquifer cells, and set the transmissiblity related to aquifer cells to
        // be zero
        fp.apply_numerical_aquifers(numerical_aquifer);
        update_active_cells(grid, fp);
        aquifer_config.load_connections(deck, grid);

        // add NNCs between aquifer cells and first aquifer cell and aquifer connections
        const auto& aquifer_cell_nncs = numerical_aquifer.aquiferCellNNCs();
        for (const auto& nnc_data : aquifer_cell_nncs) {
            input_nnc.addNNC(nnc_data.cell1, nnc_data.cell2, nnc_data.trans);
        }
    } else
        aquifer_config.load_connections(deck, grid);

    return aquifer_config;
}


}



    EclipseState::EclipseState(const Deck& deck)
    try
        : m_tables(            deck ),
          m_runspec(           deck ),
          m_eclipseConfig(     deck ),
          m_deckUnitSystem(    deck.getActiveUnitSystem() ),
          m_inputGrid(         deck, nullptr ),
          m_inputNnc(          m_inputGrid, deck),
          m_gridDims(          deck ),
          field_props(         deck, m_runspec.phases(), m_inputGrid, m_tables),
          m_simulationConfig(  m_eclipseConfig.init().restartRequested(), deck, field_props),
          m_transMult(         GridDims(deck), deck, field_props),
          tracer_config(       m_deckUnitSystem, deck)
    {
        update_active_cells(this->m_inputGrid, this->field_props);
        this->aquifer_config = load_aquifers(deck, this->m_tables, this->m_inputNnc, this->m_inputGrid, this->field_props);

        if( this->runspec().phases().size() < 3 )
            OpmLog::info(fmt::format("Only {} fluid phases are enabled",  this->runspec().phases().size() ));

        if (deck.hasKeyword( "TITLE" )) {
            const auto& titleKeyword = deck.getKeyword( "TITLE" );
            const auto& item = titleKeyword.getRecord( 0 ).getItem( 0 );
            std::vector<std::string> itemValue = item.getData<std::string>();
            for (const auto& entry : itemValue)
                m_title += entry + ' ';
            m_title.pop_back();
        }

        this->initTransMult();
        this->initFaults(deck);
        if (deck.hasKeyword( "MICPPARA" )) {
          this->initPara(deck);
        }

        const auto& init_config = this->getInitConfig();
        if (init_config.restartRequested()) {
            const auto& io_config = this->getIOConfig();
            const int report_step = init_config.getRestartStep();
            const auto& restart_file = io_config.getRestartFileName( init_config.getRestartRootName(), report_step, false);
            if (!std::filesystem::exists(restart_file))
                throw std::logic_error(fmt::format("The restart file: {} does not exist", restart_file));

            if (io_config.getUNIFIN()) {
                EclIO::ERst rst{restart_file};
                if (!rst.hasReportStepNumber(report_step))
                    throw std::logic_error(fmt::format("Report step: {} not found in restart file: {}", report_step, restart_file));
            }
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

    const MICPpara& EclipseState::getMICPpara() const {
        return m_micppara;
    }

    const TransMult& EclipseState::getTransMult() const {
        return m_transMult;
    }

    const NNC& EclipseState::getInputNNC() const {
        return m_inputNnc;
    }

    void EclipseState::setInputNNC(const NNC& nnc) {
        m_inputNnc = nnc;
    }

    void EclipseState::appendInputNNC(const std::vector<NNCdata>& nnc) {
        for (const auto& nnc_data : nnc ) {
            this->m_inputNnc.addNNC(nnc_data.cell1, nnc_data.cell2, nnc_data.trans);
        }
    }

    bool EclipseState::hasInputNNC() const {
        return !m_inputNnc.input().empty();
    }

    std::string EclipseState::getTitle() const {
        return m_title;
    }

    const AquiferConfig& EclipseState::aquifer() const {
        return this->aquifer_config;
    }

    const TracerConfig& EclipseState::tracer() const {
        return this->tracer_config;
    }

    void EclipseState::reset_actnum(const std::vector<int>& new_actnum) {
        this->field_props.reset_actnum(new_actnum);
    }

    void EclipseState::pruneDeactivatedAquiferConnections(const std::vector<std::size_t>& deactivated_cells) {
        if (this->aquifer_config.hasAnalyticalAquifer())
            this->aquifer_config.pruneDeactivatedAquiferConnections(deactivated_cells);
    }

    void EclipseState::loadRestartAquifers(const RestartIO::RstAquifer& aquifers) {
        if (aquifers.hasAnalyticAquifers())
            this->aquifer_config.loadFromRestart(aquifers, this->m_tables);
    }

    void EclipseState::initTransMult() {
        const auto& fp = this->field_props;
        if (fp.has_double("MULTX"))  this->m_transMult.applyMULT(fp.get_global_double("MULTX") , FaceDir::XPlus);
        if (fp.has_double("MULTX-")) this->m_transMult.applyMULT(fp.get_global_double("MULTX-"), FaceDir::XMinus);

        if (fp.has_double("MULTY"))  this->m_transMult.applyMULT(fp.get_global_double("MULTY") , FaceDir::YPlus);
        if (fp.has_double("MULTY-")) this->m_transMult.applyMULT(fp.get_global_double("MULTY-"), FaceDir::YMinus);

        if (fp.has_double("MULTZ"))  this->m_transMult.applyMULT(fp.get_global_double("MULTZ") , FaceDir::ZPlus);
        if (fp.has_double("MULTZ-")) this->m_transMult.applyMULT(fp.get_global_double("MULTZ-"), FaceDir::ZMinus);
    }

    void EclipseState::initFaults(const Deck& deck) {
        if (!DeckSection::hasGRID(deck))
            return;

        const GRIDSection gridSection ( deck );

        m_faults = FaultCollection(gridSection, m_inputGrid);
        setMULTFLT(gridSection);

        if (DeckSection::hasEDIT(deck)) {
            setMULTFLT(EDITSection ( deck ));
        }

        m_transMult.applyMULTFLT( m_faults );
    }

    void EclipseState::initPara(const Deck& deck) {
        using namespace ParserKeywords;


            const auto& keyword = deck.getKeyword<MICPPARA>();
            const auto& record = keyword.getRecord(0);
            double density_biofilm = record.getItem<MICPPARA::DENSITY_BIOFILM>().getSIDouble(0);
            double density_calcite = record.getItem<MICPPARA::DENSITY_CALCITE>().getSIDouble(0);
            double detachment_rate = record.getItem<MICPPARA::DETACHMENT_RATE>().getSIDouble(0);
            double critical_porosity = record.getItem<MICPPARA::CRITICAL_POROSITY>().get< double >(0);
            double fitting_factor = record.getItem<MICPPARA::FITTING_FACTOR>().get< double >(0);
            double half_velocity_oxygen = record.getItem<MICPPARA::HALF_VELOCITY_OXYGEN>().getSIDouble(0);
            double half_velocity_urea = record.getItem<MICPPARA::HALF_VELOCITY_UREA>().getSIDouble(0);
            double maximum_growth_rate = record.getItem<MICPPARA::MAXIMUM_GROWTH_RATE>().getSIDouble(0);
            double maximum_oxygen_concentration = record.getItem<MICPPARA::MAXIMUM_OXYGEN_CONCENTRATION>().getSIDouble(0);
            double maximum_urea_concentration = record.getItem<MICPPARA::MAXIMUM_UREA_CONCENTRATION>().getSIDouble(0);
            double maximum_urea_utilization = record.getItem<MICPPARA::MAXIMUM_UREA_UTILIZATION>().getSIDouble(0);
            double microbial_attachment_rate = record.getItem<MICPPARA::MICROBIAL_ATTACHMENT_RATE>().getSIDouble(0);
            double microbial_death_rate = record.getItem<MICPPARA::MICROBIAL_DEATH_RATE>().getSIDouble(0);
            double minimum_permeability = record.getItem<MICPPARA::MINIMUM_PERMEABILITY>().getSIDouble(0);
            double oxygen_consumption_factor = record.getItem<MICPPARA::OXYGEN_CONSUMPTION_FACTOR>().get< double >(0);
            double tolerance_before_clogging = record.getItem<MICPPARA::TOLERANCE_BEFORE_CLOGGING>().get< double >(0);
            double yield_growth_coefficient = record.getItem<MICPPARA::YIELD_GROWTH_COEFFICIENT>().get< double >(0);

            m_micppara = MICPpara(density_biofilm, density_calcite, detachment_rate, critical_porosity, fitting_factor, half_velocity_oxygen, half_velocity_urea, maximum_growth_rate, maximum_oxygen_concentration, maximum_urea_concentration, maximum_urea_utilization, microbial_attachment_rate, microbial_death_rate, minimum_permeability, oxygen_consumption_factor , tolerance_before_clogging, yield_growth_coefficient);
    }

    void EclipseState::setMULTFLT(const DeckSection& section) {
        for (size_t index=0; index < section.count("MULTFLT"); index++) {
            const auto& faultsKeyword = section.getKeyword("MULTFLT" , index);
            OpmLog::info(OpmInputError::format("\nApplying {keyword} in {file} line {line}", faultsKeyword.location()));
            InfoLogger logger("MULTFLT",3);
            for (auto iter = faultsKeyword.begin(); iter != faultsKeyword.end(); ++iter) {
                const auto& faultRecord = *iter;
                const std::string& faultName = faultRecord.getItem(0).get< std::string >(0);
                double multFlt = faultRecord.getItem(1).get< double >(0);
                m_faults.setTransMult( faultName , multFlt );

                logger(fmt::format("Setting fault transmissibility multiplier {} for fault {}", multFlt, faultName));
            }
        }
    }

    void EclipseState::complainAboutAmbiguousKeyword(const Deck& deck, const std::string& keywordName) {
        OpmLog::error("The " + keywordName + " keyword must be unique in the deck. Ignoring all!");
        auto keywords = deck.getKeywordList(keywordName);
        for (size_t i = 0; i < keywords.size(); ++i) {
            std::string msg = "Ambiguous keyword "+keywordName+" defined here";
            OpmLog::error(Log::fileMessage(keywords[i]->location(), msg));
        }
    }

    void EclipseState::apply_geo_keywords(const std::vector<DeckKeyword>& keywords) {
        using namespace ParserKeywords;
        for (const auto& keyword : keywords) {

            if (keyword.isKeyword<MULTFLT>()) {
                for (const auto& record : keyword) {
                    const std::string& faultName = record.getItem<MULTFLT::fault>().get< std::string >(0);
                    auto& fault = m_faults.getFault( faultName );
                    double tmpMultFlt = record.getItem<MULTFLT::factor>().get< double >(0);
                    double oldMultFlt = fault.getTransMult( );
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
                    fault.setTransMult( tmpMultFlt );
                    m_transMult.applyMULTFLT( fault );
                    fault.setTransMult( newMultFlt );
                }
            }
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
            rst_cmp_obj(full_state.m_transMult, rst_state.m_transMult, "TransMult") &&
            rst_cmp_obj(full_state.m_faults, rst_state.m_faults, "Faults") &&
            rst_cmp_obj(full_state.m_title, rst_state.m_title, "Title") &&
            rst_cmp_obj(full_state.tracer_config, rst_state.tracer_config, "Tracer");
    }

}
