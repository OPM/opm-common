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

#ifndef OPM_ECLIPSE_STATE_HPP
#define OPM_ECLIPSE_STATE_HPP

#include <opm/input/eclipse/EclipseState/Aquifer/AquiferConfig.hpp>
#include <opm/input/eclipse/EclipseState/Compositional/CompositionalConfig.hpp>
#include <opm/input/eclipse/EclipseState/EclipseConfig.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FIPRegionStatistics.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FaultCollection.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Grid/LgrCollection.hpp>
#include <opm/input/eclipse/EclipseState/Grid/NNC.hpp>
#include <opm/input/eclipse/EclipseState/Grid/TransMult.hpp>
#include <opm/input/eclipse/EclipseState/Runspec.hpp>
#include <opm/input/eclipse/EclipseState/SimulationConfig/SimulationConfig.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/input/eclipse/EclipseState/TracerConfig.hpp>
#include <opm/input/eclipse/EclipseState/Co2StoreConfig.hpp>
#include <opm/input/eclipse/EclipseState/WagHysteresisConfig.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

namespace Opm {
    class Deck;
    class DeckKeyword;
    class InitConfig;
    class IOConfig;
    class DeckSection;
} // namespace Opm

namespace Opm { namespace RestartIO {
    class RstAquifer;
    class RstNetwork;
}} // namespace Opm::RestartIO

namespace Opm {

    class EclipseState {
    public:
        enum EnabledTypes {
            IntProperties = 0x01,
            DoubleProperties = 0x02,

            AllProperties = IntProperties | DoubleProperties
        };

        EclipseState() = default;
        explicit EclipseState(const Deck& deck);
        virtual ~EclipseState() = default;

        const IOConfig& getIOConfig() const;
        IOConfig& getIOConfig();

        const InitConfig& getInitConfig() const;
        InitConfig& getInitConfig();

        const SimulationConfig& getSimulationConfig() const;
        virtual const EclipseGrid& getInputGrid() const;

        const FaultCollection& getFaults() const;
        const TransMult& getTransMult() const;
        TransMult& getTransMult();

        /// non-neighboring connections
        /// the non-standard adjacencies as specified in input deck
        const NNC& getInputNNC() const;
        /// \brief Get sorted vector of NNCs created by PINCH
        const std::vector<NNCdata>& getPinchNNC() const;
        void appendInputNNC(const std::vector<NNCdata>& nnc);
        void setInputNNC(const NNC& nnc);
        /// \brief Set NNCs created by PINCH
        void setPinchNNC(std::vector<NNCdata>&& nnc);
        bool hasInputNNC() const;
        bool hasPinchNNC() const;

        // The potentially parallelized field properties
        virtual const FieldPropsManager& fieldProps() const;
        // Always the non-parallel field properties
        virtual const FieldPropsManager& globalFieldProps() const;

        virtual void computeFipRegionStatistics();
        const FIPRegionStatistics& fipRegionStatistics() const;

        const TableManager& getTableManager() const;
        const EclipseConfig& getEclipseConfig() const;
        const EclipseConfig& cfg() const;
        const GridDims& gridDims() const;

        const LgrCollection& getLgrs() const;
        bool hasInputLGR() const;

        // the unit system used by the deck. note that it is rarely needed
        // to convert units because internally to opm-parser everything is
        // represented by SI units.
        const UnitSystem& getDeckUnitSystem() const;
        const UnitSystem& getUnits() const;

        const std::string& getTitle() const;

        void apply_schedule_keywords(const std::vector<DeckKeyword>& keywords);

        const Runspec& runspec() const;
        const AquiferConfig& aquifer() const;
        const CompositionalConfig& compositionalConfig() const;
        const TracerConfig& tracer() const;
        const WagHysteresisConfig& getWagHysteresis() const;
        const Co2StoreConfig& getCo2StoreConfig() const;

        void prune_global_for_schedule_run();
        void reset_actnum(const std::vector<int>& new_actnum);
        void set_active_indices(const std::vector<int>& indices);
        void pruneDeactivatedAquiferConnections(const std::vector<std::size_t>& deactivated_cells);
        void loadRestartAquifers(const RestartIO::RstAquifer& aquifers);
        // TODO: it is possible that the aquifer are opened through SCHEDULE and not specified in the SOLUTION section
        // For the ease of the implementation, we create inactive aquifer in the AquiferConfig.
        // At the moment, it only works for Constant Flux Aquifers(AQUFLUX) as indicated by the function name
        // When we know and decide to handle the same for AQUFETP and AQUCT, this part will be refactored
        void appendAqufluxSchedule(const std::unordered_set<int>& ids);

        void loadRestartNetworkPressures(const RestartIO::RstNetwork& network);
        const std::optional<std::map<std::string, double> >& getRestartNetworkPressures() const { return this->m_restart_network_pressures; }

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            // FieldPropsManager is handled through a different mechanism.
            // Do not add the member (i.e., field_props) to this list.
            serializer(m_tables);
            serializer(m_runspec);
            serializer(m_eclipseConfig);
            serializer(m_deckUnitSystem);
            serializer(m_inputNnc);
            serializer(m_pinchNnc);
            serializer(m_gridDims);
            serializer(m_lgrs);
            serializer(m_simulationConfig);
            serializer(aquifer_config);
            serializer(compositional_config),
            serializer(m_transMult);
            serializer(m_faults);
            serializer(m_title);
            serializer(tracer_config);
            serializer(wag_hyst_config);
            serializer(co2_store_config);
            serializer(this->fipRegionStatistics_);
        }

        static bool rst_cmp(const EclipseState& full_state, const EclipseState& rst_state);

    private:
        void initIOConfigPostSchedule(const Deck& deck);
        void assignRunTitle(const Deck& deck);
        void reportNumberOfActivePhases() const;
        void initLgrs(const Deck& deck);
        void conveyNumericalAquiferEffects();
        void applyMULTXYZ();
        void initFaults(const Deck& deck);
        void initPara(const Deck& deck);

        void setMULTFLT(const Opm::DeckSection& section, bool edit = false);

        void complainAboutAmbiguousKeyword(const Deck& deck,
                                           const std::string& keywordName);

     protected:
        TableManager m_tables;
        Runspec m_runspec;
        EclipseConfig m_eclipseConfig;
        UnitSystem m_deckUnitSystem;
        EclipseGrid m_inputGrid;
        NNC m_inputNnc;
        std::vector<NNCdata> m_pinchNnc;
        GridDims m_gridDims;
        FieldPropsManager field_props;
        LgrCollection m_lgrs;
        SimulationConfig m_simulationConfig;
        AquiferConfig aquifer_config;
        CompositionalConfig compositional_config;
        TransMult m_transMult;
        TracerConfig tracer_config;
        WagHysteresisConfig wag_hyst_config;
        Co2StoreConfig co2_store_config;

        std::string m_title{};
        FaultCollection m_faults{};

        std::optional<std::map<std::string, double> > m_restart_network_pressures{std::nullopt};

        std::optional<FIPRegionStatistics> fipRegionStatistics_{std::nullopt};
    };
} // namespace Opm

#endif // OPM_ECLIPSE_STATE_HPP
