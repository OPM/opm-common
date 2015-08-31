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

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/OpmLog/OpmLog.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Box.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/BoxManager.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/TransMult.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FaultCollection.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/NNC.hpp>

#include <opm/parser/eclipse/EclipseState/Tables/Tables.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/EnkrvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/EnptvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/GasvisctTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/ImkrvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/ImptvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/OilvisctTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PlyadsTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PlymaxTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PlyrockTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PlyviscTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PlyshlogTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PlydhflfTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PvtgTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PvtoTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/RocktabTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/RsvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/RvvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/RtempvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/WatvisctTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/VFPProdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/VFPInjTable.hpp>
#include <opm/parser/eclipse/EclipseState/InitConfig/InitConfig.hpp>
#include <opm/parser/eclipse/EclipseState/SimulationConfig/SimulationConfig.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>

#include <opm/parser/eclipse/Parser/ParseMode.hpp>

#include <set>
#include <memory>
#include <iostream>
#include <map>
#include <vector>

namespace Opm {
    class EclipseState {
    public:
        enum EnabledTypes {
            IntProperties = 0x01,
            DoubleProperties = 0x02,

            AllProperties = IntProperties | DoubleProperties
        };

        EclipseState(DeckConstPtr deck , const ParseMode& parseMode);

        ScheduleConstPtr getSchedule() const;
        IOConfigConstPtr getIOConfigConst() const;
        IOConfigPtr getIOConfig() const;
        InitConfigConstPtr getInitConfig() const;
        SimulationConfigConstPtr getSimulationConfig() const;
        EclipseGridConstPtr getEclipseGrid() const;
        EclipseGridPtr getEclipseGridCopy() const;
        bool hasPhase(enum Phase::PhaseEnum phase) const;
        std::string getTitle() const;
        bool supportsGridProperty(const std::string& keyword, int enabledTypes=AllProperties) const;

        std::shared_ptr<GridProperty<int> > getRegion(DeckItemConstPtr regionItem) const;
        std::shared_ptr<GridProperty<int> > getDefaultRegion() const;
        std::shared_ptr<GridProperty<int> > getIntGridProperty( const std::string& keyword ) const;
        std::shared_ptr<GridProperty<double> > getDoubleGridProperty( const std::string& keyword ) const;
        bool hasIntGridProperty(const std::string& keyword) const;
        bool hasDoubleGridProperty(const std::string& keyword) const;

        void loadGridPropertyFromDeckKeyword(std::shared_ptr<const Box> inputBox,
                                             DeckKeywordConstPtr deckKeyword,
                                             int enabledTypes = AllProperties);

        std::shared_ptr<const FaultCollection> getFaults() const;
        std::shared_ptr<const TransMult> getTransMult() const;
        std::shared_ptr<const NNC> getNNC() const;
        bool hasNNC() const;

        std::shared_ptr<const Tables> getTables() const;
        // the tables used by the deck. If the tables had some defaulted data in the
        // deck, the objects returned here exhibit the correct values. If the table is
        // not present in the deck, the corresponding vector is of size zero.
        const std::vector<EnkrvdTable>& getEnkrvdTables() const;
        const std::vector<EnptvdTable>& getEnptvdTables() const;
        const std::vector<GasvisctTable>& getGasvisctTables() const;
        const std::vector<ImkrvdTable>& getImkrvdTables() const;
        const std::vector<ImptvdTable>& getImptvdTables() const;
        const std::vector<OilvisctTable>& getOilvisctTables() const;
        const std::vector<PlyadsTable>& getPlyadsTables() const;
        const std::vector<PlymaxTable>& getPlymaxTables() const;
        const std::vector<PlyrockTable>& getPlyrockTables() const;
        const std::vector<PlyviscTable>& getPlyviscTables() const;
        const std::vector<PlydhflfTable>& getPlydhflfTables() const;
        const std::vector<PlyshlogTable>& getPlyshlogTables() const;
        const std::vector<PvtgTable>& getPvtgTables() const;
        const std::vector<PvtoTable>& getPvtoTables() const;
        const std::vector<RocktabTable>& getRocktabTables() const;
        const std::vector<RsvdTable>& getRsvdTables() const;
        const std::vector<RvvdTable>& getRvvdTables() const;
        const std::vector<RtempvdTable>& getRtempvdTables() const;
        const std::vector<WatvisctTable>& getWatvisctTables() const;
        const std::map<int, VFPProdTable>& getVFPProdTables() const;
        const std::map<int, VFPInjTable>& getVFPInjTables() const;
        size_t getNumPhases() const;

        // the unit system used by the deck. note that it is rarely needed to convert
        // units because internally to opm-parser everything is represented by SI
        // units...
        std::shared_ptr<const UnitSystem> getDeckUnitSystem()  const;

    private:
        void initTabdims(DeckConstPtr deck);
        void initTables(DeckConstPtr deck);
        void initIOConfig(DeckConstPtr deck);
        void initSchedule(DeckConstPtr deck , const ParseMode& parseMode);
        void initIOConfigPostSchedule(DeckConstPtr deck);
        void initInitConfig(DeckConstPtr deck);
        void initSimulationConfig(DeckConstPtr deck, const ParseMode& parseMode);
        void initEclipseGrid(DeckConstPtr deck);
        void initGridopts(DeckConstPtr deck);
        void initPhases(DeckConstPtr deck);
        void initTitle(DeckConstPtr deck);
        void initProperties(DeckConstPtr deck);
        void initTransMult();
        void initFaults(DeckConstPtr deck);
        void initNNC(DeckConstPtr deck);


        template <class TableType>
        void initSimpleTables(DeckConstPtr deck,
                              const std::string& keywordName,
                              std::vector<TableType>& tableVector) {
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
                        std::string msg = "The first table for keyword "+keywordName+" must be explicitly defined! Ignoring keyword";
                        OpmLog::addMessage(Log::MessageType::Warning , Log::fileMessage( tableKeyword->getFileName(), tableKeyword->getLineNumber(), msg));
                        return;
                    }
                    tableVector.push_back(tableVector.back());
                    continue;
                }

                tableVector.push_back(TableType());
                tableVector[tableIdx].init(tableKeyword, tableIdx);
            }
        }

        template <class TableType>
        void initFullTables(DeckConstPtr deck,
                            const std::string& keywordName,
                            std::vector<TableType>& tableVector) {
            if (!deck->hasKeyword(keywordName))
                return; // the table is not featured by the deck...

            if (deck->numKeywords(keywordName) > 1) {
                complainAboutAmbiguousKeyword(deck, keywordName);
                return;
            }

            const auto& tableKeyword = deck->getKeyword(keywordName);

            int numTables = TableType::numTables(tableKeyword);
            for (int tableIdx = 0; tableIdx < numTables; ++tableIdx) {
                tableVector.push_back(TableType());
                tableVector[tableIdx].init(tableKeyword, tableIdx);
            }
        }

        void initRocktabTables(DeckConstPtr deck);
        void initGasvisctTables(DeckConstPtr deck,
                                const std::string& keywordName,
                                std::vector<GasvisctTable>& tableVector);

        void initPlyshlogTables(DeckConstPtr deck,
                                const std::string& keywordName,
                                std::vector<PlyshlogTable>& tableVector);

        void initVFPProdTables(DeckConstPtr deck,
                               std::map<int, VFPProdTable>& tableMap);

        void initVFPInjTables(DeckConstPtr deck,
                              std::map<int, VFPInjTable>& tableMap);

        void setMULTFLT(std::shared_ptr<const Section> section) const;
        void initMULTREGT(DeckConstPtr deck);

        double getSIScaling(const std::string &dimensionString) const;

        void processGridProperties(Opm::DeckConstPtr deck, int enabledTypes);
        void scanSection(std::shared_ptr<Opm::Section> section , int enabledTypes);
        void handleADDKeyword(DeckKeywordConstPtr deckKeyword  , BoxManager& boxManager, int enabledTypes);
        void handleBOXKeyword(DeckKeywordConstPtr deckKeyword  , BoxManager& boxManager);
        void handleCOPYKeyword(DeckKeywordConstPtr deckKeyword , BoxManager& boxManager, int enabledTypes);
        void handleENDBOXKeyword(BoxManager& boxManager);
        void handleEQUALSKeyword(DeckKeywordConstPtr deckKeyword   , BoxManager& boxManager, int enabledTypes);
        void handleMULTIPLYKeyword(DeckKeywordConstPtr deckKeyword , BoxManager& boxManager, int enabledTypes);

        void handleEQUALREGKeyword(DeckKeywordConstPtr deckKeyword, int enabledTypes);
        void handleMULTIREGKeyword(DeckKeywordConstPtr deckKeyword, int enabledTypes);
        void handleADDREGKeyword(DeckKeywordConstPtr deckKeyword  , int enabledTypes);
        void handleCOPYREGKeyword(DeckKeywordConstPtr deckKeyword , int enabledTypes);

        void setKeywordBox(DeckKeywordConstPtr deckKeyword, size_t recordIdx, BoxManager& boxManager);

        void copyIntKeyword(const std::string& srcField , const std::string& targetField , std::shared_ptr<const Box> inputBox);
        void copyDoubleKeyword(const std::string& srcField , const std::string& targetField , std::shared_ptr<const Box> inputBox);

        void complainAboutAmbiguousKeyword(DeckConstPtr deck, const std::string& keywordName) const;

        EclipseGridConstPtr      m_eclipseGrid;
        IOConfigPtr              m_ioConfig;
        InitConfigConstPtr       m_initConfig;
        ScheduleConstPtr         schedule;
        SimulationConfigConstPtr m_simulationConfig;

        std::shared_ptr<const Tables> m_tables;
        std::vector<EnkrvdTable> m_enkrvdTables;
        std::vector<EnptvdTable> m_enptvdTables;
        std::vector<GasvisctTable> m_gasvisctTables;
        std::vector<ImkrvdTable> m_imkrvdTables;
        std::vector<ImptvdTable> m_imptvdTables;
        std::vector<OilvisctTable> m_oilvisctTables;
        std::vector<PlyadsTable> m_plyadsTables;
        std::vector<PlymaxTable> m_plymaxTables;
        std::vector<PlyrockTable> m_plyrockTables;
        std::vector<PlyviscTable> m_plyviscTables;
        std::vector<PlydhflfTable> m_plydhflfTables;
        std::vector<PlyshlogTable> m_plyshlogTables;
        std::vector<PvtgTable> m_pvtgTables;
        std::vector<PvtoTable> m_pvtoTables;
        std::vector<RocktabTable> m_rocktabTables;
        std::vector<RsvdTable> m_rsvdTables;
        std::vector<RvvdTable> m_rvvdTables;
        std::vector<RtempvdTable> m_rtempvdTables;
        std::vector<WatvisctTable> m_watvisctTables;
        std::map<int, VFPProdTable> m_vfpprodTables;
        std::map<int, VFPInjTable> m_vfpinjTables;

        std::set<enum Phase::PhaseEnum> phases;
        std::string m_title;
        std::shared_ptr<const UnitSystem> m_deckUnitSystem;
        std::shared_ptr<GridProperties<int> > m_intGridProperties;
        std::shared_ptr<GridProperties<double> > m_doubleGridProperties;
        std::shared_ptr<TransMult> m_transMult;
        std::shared_ptr<FaultCollection> m_faults;
        std::shared_ptr<NNC> m_nnc;
        std::string m_defaultRegion;
    };

    typedef std::shared_ptr<EclipseState> EclipseStatePtr;
    typedef std::shared_ptr<const EclipseState> EclipseStateConstPtr;
}

#endif
