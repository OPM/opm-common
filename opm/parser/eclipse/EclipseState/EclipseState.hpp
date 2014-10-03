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
#include <opm/parser/eclipse/Parser/ParserLog.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Box.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/BoxManager.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/TransMult.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FaultCollection.hpp>

#include <opm/parser/eclipse/EclipseState/Tables/EnkrvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/EnptvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/ImkrvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/ImptvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PlyadsTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PlymaxTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PlyrockTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PlyviscTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PvdgTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PvdoTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PvtgTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PvtoTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/RocktabTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/RsvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/RvvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SwofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TlmixparTable.hpp>

#include <set>
#include <memory>

namespace Opm {
    class EclipseState {
    public:
        enum EnabledTypes {
            IntProperties = 0x01,
            DoubleProperties = 0x02,

            AllProperties = IntProperties | DoubleProperties
        };

        EclipseState(DeckConstPtr deck, ParserLogPtr parserLog = std::make_shared<ParserLog>());

        ScheduleConstPtr getSchedule() const;
        EclipseGridConstPtr getEclipseGrid() const;
        EclipseGridPtr getEclipseGridCopy() const;
        bool hasPhase(enum Phase::PhaseEnum phase) const;
        std::string getTitle() const;
        bool supportsGridProperty(const std::string& keyword, int enabledTypes=AllProperties) const;

        std::shared_ptr<GridProperty<int> > getIntGridProperty( const std::string& keyword ) const;
        std::shared_ptr<GridProperty<double> > getDoubleGridProperty( const std::string& keyword ) const;
        bool hasIntGridProperty(const std::string& keyword) const;
        bool hasDoubleGridProperty(const std::string& keyword) const;

        void loadGridPropertyFromDeckKeyword(std::shared_ptr<const Box> inputBox,
                                             DeckKeywordConstPtr deckKeyword,
                                             ParserLogPtr parserLog,
                                             int enabledTypes = AllProperties);

        std::shared_ptr<const FaultCollection> getFaults() const;
        std::shared_ptr<const TransMult> getTransMult() const;

        // the tables used by the deck. If the tables had some defaulted data in the
        // deck, the objects returned here exhibit the correct values. If the table is
        // not present in the deck, the corresponding vector is of size zero.
        const std::vector<EnkrvdTable>& getEnkrvdTables() const;
        const std::vector<EnptvdTable>& getEnptvdTables() const;
        const std::vector<ImkrvdTable>& getImkrvdTables() const;
        const std::vector<ImptvdTable>& getImptvdTables() const;
        const std::vector<PlyadsTable>& getPlyadsTables() const;
        const std::vector<PlymaxTable>& getPlymaxTables() const;
        const std::vector<PlyrockTable>& getPlyrockTables() const;
        const std::vector<PlyviscTable>& getPlyviscTables() const;
        const std::vector<PvdgTable>& getPvdgTables() const;
        const std::vector<PvdoTable>& getPvdoTables() const;
        const std::vector<PvtgTable>& getPvtgTables() const;
        const std::vector<PvtoTable>& getPvtoTables() const;
        const std::vector<RocktabTable>& getRocktabTables() const;
        const std::vector<RsvdTable>& getRsvdTables() const;
        const std::vector<RvvdTable>& getRvvdTables() const;
        const std::vector<SgofTable>& getSgofTables() const;
        const std::vector<SwofTable>& getSwofTables() const;
        const std::vector<TlmixparTable>& getTlmixparTables() const;

        // the unit system used by the deck. note that it is rarely needed to convert
        // units because internally to opm-parser everything is represented by SI
        // units...
        std::shared_ptr<const UnitSystem> getDeckUnitSystem()  const;

    private:
        void initTables(DeckConstPtr deck, ParserLogPtr parserLog);
        void initSchedule(DeckConstPtr deck, ParserLogPtr parserLog);
        void initEclipseGrid(DeckConstPtr deck, ParserLogPtr parserLog);
        void initPhases(DeckConstPtr deck, ParserLogPtr parserLog);
        void initTitle(DeckConstPtr deck, ParserLogPtr parserLog);
        void initProperties(DeckConstPtr deck, ParserLogPtr parserLog);
        void initTransMult(ParserLogPtr parserLog);
        void initFaults(DeckConstPtr deck, ParserLogPtr parserLog);

        template <class TableType>
        void initSimpleTables(DeckConstPtr deck,
                              ParserLogPtr parserLog,
                              const std::string& keywordName,
                              std::vector<TableType>& tableVector) {
            if (!deck->hasKeyword(keywordName))
                return; // the table is not featured by the deck...

            if (deck->numKeywords(keywordName) > 1) {
                complainAboutAmbiguousKeyword(deck, parserLog, keywordName);
                return;
            }

            const auto& tableKeyword = deck->getKeyword(keywordName);
            for (size_t tableIdx = 0; tableIdx < tableKeyword->size(); ++tableIdx) {
                if (tableKeyword->getRecord(tableIdx)->getItem(0)->size() == 0) {
                    // for simple tables, an empty record indicates that the previous table
                    // should be copied...
                    if (tableIdx == 0) {
                        parserLog->addError(tableKeyword->getFileName(),
                                            tableKeyword->getLineNumber(),
                                            "The first table for keyword "+keywordName+
                                            " must be explicitly defined! Ignoring keyword");
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
                            ParserLogPtr parserLog,
                            const std::string& keywordName,
                            std::vector<TableType>& tableVector) {
            if (!deck->hasKeyword(keywordName))
                return; // the table is not featured by the deck...

            if (deck->numKeywords(keywordName) > 1) {
                complainAboutAmbiguousKeyword(deck, parserLog, keywordName);
                return;
            }

            const auto& tableKeyword = deck->getKeyword(keywordName);

            int numTables = TableType::numTables(tableKeyword);
            for (int tableIdx = 0; tableIdx < numTables; ++tableIdx) {
                tableVector.push_back(TableType());
                tableVector[tableIdx].init(tableKeyword, tableIdx);
            }
        }

        void initRocktabTables(DeckConstPtr deck, ParserLogPtr parserLog);

        void setMULTFLT(std::shared_ptr<const Section> section, ParserLogPtr parserLog) const;
        void initMULTREGT(DeckConstPtr deck, ParserLogPtr parserLog);

        double getSIScaling(const std::string &dimensionString) const;

        void processGridProperties(Opm::DeckConstPtr deck, ParserLogPtr parserLog, int enabledTypes);
        void scanSection(std::shared_ptr<Opm::Section> section, ParserLogPtr parserLog, int enabledTypes);
        void handleADDKeyword(DeckKeywordConstPtr deckKeyword , ParserLogPtr parserLog, BoxManager& boxManager, int enabledTypes);
        void handleBOXKeyword(DeckKeywordConstPtr deckKeyword , ParserLogPtr parserLog, BoxManager& boxManager);
        void handleCOPYKeyword(DeckKeywordConstPtr deckKeyword , ParserLogPtr parserLog, BoxManager& boxManager, int enabledTypes);
        void handleENDBOXKeyword(BoxManager& boxManager);
        void handleEQUALSKeyword(DeckKeywordConstPtr deckKeyword , ParserLogPtr parserLog, BoxManager& boxManager, int enabledTypes);
        void handleMULTIPLYKeyword(DeckKeywordConstPtr deckKeyword , ParserLogPtr parserLog, BoxManager& boxManager, int enabledTypes);
        
        void setKeywordBox(DeckKeywordConstPtr deckKeyword, size_t recordIdx, ParserLogPtr parserLog, BoxManager& boxManager);

        void copyIntKeyword(const std::string& srcField , const std::string& targetField , std::shared_ptr<const Box> inputBox);
        void copyDoubleKeyword(const std::string& srcField , const std::string& targetField , std::shared_ptr<const Box> inputBox);

        void complainAboutAmbiguousKeyword(DeckConstPtr deck, ParserLogPtr parserLog, const std::string& keywordName) const;

        EclipseGridConstPtr m_eclipseGrid;
        ScheduleConstPtr schedule;

        std::vector<EnkrvdTable> m_enkrvdTables;
        std::vector<EnptvdTable> m_enptvdTables;
        std::vector<ImkrvdTable> m_imkrvdTables;
        std::vector<ImptvdTable> m_imptvdTables;
        std::vector<PlyadsTable> m_plyadsTables;
        std::vector<PlymaxTable> m_plymaxTables;
        std::vector<PlyrockTable> m_plyrockTables;
        std::vector<PlyviscTable> m_plyviscTables;
        std::vector<PvdgTable> m_pvdgTables;
        std::vector<PvdoTable> m_pvdoTables;
        std::vector<PvtgTable> m_pvtgTables;
        std::vector<PvtoTable> m_pvtoTables;
        std::vector<RocktabTable> m_rocktabTables;
        std::vector<RsvdTable> m_rsvdTables;
        std::vector<RvvdTable> m_rvvdTables;
        std::vector<SgofTable> m_sgofTables;
        std::vector<SwofTable> m_swofTables;
        std::vector<TlmixparTable> m_tlmixparTables;

        std::set<enum Phase::PhaseEnum> phases;
        std::string m_title;
        std::shared_ptr<const UnitSystem> m_deckUnitSystem;
        std::shared_ptr<GridProperties<int> > m_intGridProperties;
        std::shared_ptr<GridProperties<double> > m_doubleGridProperties;
        std::shared_ptr<TransMult> m_transMult;
        std::shared_ptr<FaultCollection> m_faults;
    };

    typedef std::shared_ptr<EclipseState> EclipseStatePtr;
    typedef std::shared_ptr<const EclipseState> EclipseStateConstPtr;
}

#endif
