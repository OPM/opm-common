/*
  Copyright 2015 Statoil ASA.

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

#include <opm/parser/eclipse/Parser/ParserKeywords.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/parser/eclipse/OpmLog/OpmLog.hpp>

namespace Opm {

    TableManager::TableManager( const Deck& deck ) {
        initDims( deck );
        initSimpleTables( deck );
        initFullTables(deck, "PVTG", m_pvtgTables);
        initFullTables(deck, "PVTO", m_pvtoTables);

        initVFPProdTables(deck, m_vfpprodTables);
        initVFPInjTables(deck,  m_vfpinjTables);
    }


    void TableManager::initDims(const Deck& deck) {
        using namespace Opm::ParserKeywords;
        if (deck.hasKeyword<TABDIMS>()) {
            auto keyword = deck.getKeyword<TABDIMS>();
            auto record = keyword->getRecord(0);
            int ntsfun = record->getItem<TABDIMS::NTSFUN>()->getInt(0);
            int ntpvt  = record->getItem<TABDIMS::NTPVT>()->getInt(0);
            int nssfun = record->getItem<TABDIMS::NSSFUN>()->getInt(0);
            int nppvt  = record->getItem<TABDIMS::NPPVT>()->getInt(0);
            int ntfip  = record->getItem<TABDIMS::NTFIP>()->getInt(0);
            int nrpvt  = record->getItem<TABDIMS::NRPVT>()->getInt(0);

            m_tabdims = std::make_shared<Tabdims>(ntsfun , ntpvt , nssfun , nppvt , ntfip , nrpvt);
        } else
            m_tabdims = std::make_shared<Tabdims>();

        if (deck.hasKeyword<EQLDIMS>()) {
            auto keyword = deck.getKeyword<EQLDIMS>();
            auto record = keyword->getRecord(0);
            int ntsequl   = record->getItem<EQLDIMS::NTEQUL>()->getInt(0);
            int nodes_p   = record->getItem<EQLDIMS::DEPTH_NODES_P>()->getInt(0);
            int nodes_tab = record->getItem<EQLDIMS::DEPTH_NODES_TAB>()->getInt(0);
            int nttrvd    = record->getItem<EQLDIMS::NTTRVD>()->getInt(0);
            int ntsrvd    = record->getItem<EQLDIMS::NSTRVD>()->getInt(0);

            m_eqldims = std::make_shared<Eqldims>(ntsequl , nodes_p , nodes_tab , nttrvd , ntsrvd );
        } else
            m_eqldims = std::make_shared<Eqldims>();

        if (deck.hasKeyword<REGDIMS>()) {
            auto keyword = deck.getKeyword<REGDIMS>();
            auto record = keyword->getRecord(0);
            int ntfip  = record->getItem<REGDIMS::NTFIP>()->getInt(0);
            int nmfipr = record->getItem<REGDIMS::NMFIPR>()->getInt(0);
            int nrfreg = record->getItem<REGDIMS::NRFREG>()->getInt(0);
            int ntfreg = record->getItem<REGDIMS::NTFREG>()->getInt(0);
            int nplmix = record->getItem<REGDIMS::NPLMIX>()->getInt(0);
            m_regdims = std::make_shared<Regdims>( ntfip , nmfipr , nrfreg , ntfreg , nplmix );
        } else
            m_regdims = std::make_shared<Regdims>();
    }


    void TableManager::addTables( const std::string& tableName , size_t numTables) {
        m_simpleTables.emplace(std::make_pair(tableName , TableContainer( numTables )));
    }


    bool TableManager::hasTables( const std::string& tableName ) const {
        auto pair = m_simpleTables.find( tableName );
        if (pair == m_simpleTables.end())
            return false;
        else {
            const auto& tables = pair->second;
            return !tables.empty();
        }
    }


    const TableContainer& TableManager::getTables( const std::string& tableName ) const {
        auto pair = m_simpleTables.find( tableName );
        if (pair == m_simpleTables.end())
            throw std::invalid_argument("No such table collection: " + tableName);
        else
            return pair->second;
    }

    TableContainer& TableManager::forceGetTables( const std::string& tableName , size_t numTables )  {
        auto pair = m_simpleTables.find( tableName );
        if (pair == m_simpleTables.end()) {
            addTables( tableName , numTables );
            pair = m_simpleTables.find( tableName );
        }
        return pair->second;
    }


    const TableContainer& TableManager::operator[](const std::string& tableName) const {
        return getTables(tableName);
    }

    void TableManager::initSimpleTables(const Deck& deck) {
        addTables( "SWOF" , m_tabdims->getNumSatTables() );
        addTables( "SGWFN", m_tabdims->getNumSatTables() );
        addTables( "SGOF",  m_tabdims->getNumSatTables() );
        addTables( "SLGOF", m_tabdims->getNumSatTables() );
        addTables( "SOF2",  m_tabdims->getNumSatTables() );
        addTables( "SOF3",  m_tabdims->getNumSatTables() );
        addTables( "SWFN",  m_tabdims->getNumSatTables() );
        addTables( "SGFN",  m_tabdims->getNumSatTables() );
        addTables( "SSFN",  m_tabdims->getNumSatTables() );

        addTables( "PLYADS", m_tabdims->getNumSatTables() );
        addTables( "PLYROCK", m_tabdims->getNumSatTables());
        addTables( "PLYVISC", m_tabdims->getNumPVTTables());
        addTables( "PLYDHFLF", m_tabdims->getNumPVTTables());

        addTables( "PVDG", m_tabdims->getNumPVTTables());
        addTables( "PVDO", m_tabdims->getNumPVTTables());
        addTables( "PVDS", m_tabdims->getNumPVTTables());

        addTables( "OILVISCT", m_tabdims->getNumPVTTables());
        addTables( "WATVISCT", m_tabdims->getNumPVTTables());
        addTables( "GASVISCT", m_tabdims->getNumPVTTables());

        addTables( "PLYMAX", m_regdims->getNPLMIX());
        addTables( "RSVD", m_eqldims->getNumEquilRegions());
        addTables( "RVVD", m_eqldims->getNumEquilRegions());

        {
            size_t numMiscibleTables = ParserKeywords::MISCIBLE::NTMISC::defaultValue;
            if (deck.hasKeyword<ParserKeywords::MISCIBLE>()) {
                auto keyword = deck.getKeyword<ParserKeywords::MISCIBLE>();
                auto record = keyword->getRecord(0);
                numMiscibleTables =  static_cast<size_t>(record->getItem<ParserKeywords::MISCIBLE::NTMISC>()->getInt(0));
            }
            addTables( "SORWMIS", numMiscibleTables);
            addTables( "SGCWMIS", numMiscibleTables);
            addTables( "MISC", numMiscibleTables);
            addTables( "PMISC", numMiscibleTables);


        }

        {
            size_t numEndScaleTables = ParserKeywords::ENDSCALE::NUM_TABLES::defaultValue;

            if (deck.hasKeyword<ParserKeywords::ENDSCALE>()) {
                auto keyword = deck.getKeyword<ParserKeywords::ENDSCALE>();
                auto record = keyword->getRecord(0);
                numEndScaleTables = static_cast<size_t>(record->getItem<ParserKeywords::ENDSCALE::NUM_TABLES>()->getInt(0));
            }

            addTables( "ENKRVD", numEndScaleTables);
            addTables( "ENPTVD", numEndScaleTables);
            addTables( "IMKRVD", numEndScaleTables);
            addTables( "IMPTVD", numEndScaleTables);
        }
        {
            size_t numRocktabTables = ParserKeywords::ROCKCOMP::NTROCC::defaultValue;

            if (deck.hasKeyword<ParserKeywords::ROCKCOMP>()) {
                auto keyword = deck.getKeyword<ParserKeywords::ROCKCOMP>();
                auto record = keyword->getRecord(0);
                numRocktabTables = static_cast<size_t>(record->getItem<ParserKeywords::ROCKCOMP::NTROCC>()->getInt(0));
            }
            addTables( "ROCKTAB", numRocktabTables);
        }

        initSimpleTableContainer<SwofTable>(deck, "SWOF" , m_tabdims->getNumSatTables());
        initSimpleTableContainer<SgwfnTable>(deck, "SGWFN", m_tabdims->getNumSatTables());
        initSimpleTableContainer<SgofTable>(deck, "SGOF" , m_tabdims->getNumSatTables());
        initSimpleTableContainer<SlgofTable>(deck, "SLGOF" , m_tabdims->getNumSatTables());
        initSimpleTableContainer<Sof2Table>(deck, "SOF2" , m_tabdims->getNumSatTables());
        initSimpleTableContainer<Sof3Table>(deck, "SOF3" , m_tabdims->getNumSatTables());
        initSimpleTableContainer<SwfnTable>(deck, "SWFN" , m_tabdims->getNumSatTables());
        initSimpleTableContainer<SgfnTable>(deck, "SGFN" , m_tabdims->getNumSatTables());
        initSimpleTableContainer<SsfnTable>(deck, "SSFN" , m_tabdims->getNumSatTables());

        initSimpleTableContainer<RsvdTable>(deck, "RSVD" , m_eqldims->getNumEquilRegions());
        initSimpleTableContainer<RvvdTable>(deck, "RVVD" , m_eqldims->getNumEquilRegions());
        {
            size_t numEndScaleTables = ParserKeywords::ENDSCALE::NUM_TABLES::defaultValue;

            if (deck.hasKeyword<ParserKeywords::ENDSCALE>()) {
                auto keyword = deck.getKeyword<ParserKeywords::ENDSCALE>();
                auto record = keyword->getRecord(0);
                numEndScaleTables = static_cast<size_t>(record->getItem<ParserKeywords::ENDSCALE::NUM_TABLES>()->getInt(0));
            }

            initSimpleTableContainer<EnkrvdTable>( deck , "ENKRVD", numEndScaleTables);
            initSimpleTableContainer<EnptvdTable>( deck , "ENPTVD", numEndScaleTables);
            initSimpleTableContainer<ImkrvdTable>( deck , "IMKRVD", numEndScaleTables);
            initSimpleTableContainer<ImptvdTable>( deck , "IMPTVD", numEndScaleTables);
        }

        {
            size_t numMiscibleTables = ParserKeywords::MISCIBLE::NTMISC::defaultValue;
            if (deck.hasKeyword<ParserKeywords::MISCIBLE>()) {
                auto keyword = deck.getKeyword<ParserKeywords::MISCIBLE>();
                auto record = keyword->getRecord(0);
                numMiscibleTables =  static_cast<size_t>(record->getItem<ParserKeywords::MISCIBLE::NTMISC>()->getInt(0));
            }
            initSimpleTableContainer<SorwmisTable>(deck, "SORWMIS", numMiscibleTables);
            initSimpleTableContainer<SgcwmisTable>(deck, "SGCWMIS", numMiscibleTables);
            initSimpleTableContainer<MiscTable>(deck, "MISC", numMiscibleTables);
            initSimpleTableContainer<PmiscTable>(deck, "PMISC", numMiscibleTables);

        }

        initSimpleTableContainer<PvdgTable>(deck, "PVDG", m_tabdims->getNumPVTTables());
        initSimpleTableContainer<PvdoTable>(deck, "PVDO", m_tabdims->getNumPVTTables());
        initSimpleTableContainer<PvdsTable>(deck, "PVDS", m_tabdims->getNumPVTTables());
        initSimpleTableContainer<OilvisctTable>(deck, "OILVISCT", m_tabdims->getNumPVTTables());
        initSimpleTableContainer<WatvisctTable>(deck, "WATVISCT", m_tabdims->getNumPVTTables());

        initSimpleTableContainer<PlyadsTable>(deck, "PLYADS", m_tabdims->getNumSatTables());
        initSimpleTableContainer<PlyviscTable>(deck, "PLYVISC", m_tabdims->getNumPVTTables());
        initSimpleTableContainer<PlydhflfTable>(deck, "PLYDHFLF", m_tabdims->getNumPVTTables());
        initPlyrockTables(deck);
        initPlymaxTables(deck);
        initGasvisctTables(deck);
        initRTempTables(deck);
        initRocktabTables(deck);
        initPlyshlogTables(deck);
    }


    void TableManager::initRTempTables(const Deck& deck) {
        // the temperature vs depth table. the problem here is that
        // the TEMPVD (E300) and RTEMPVD (E300 + E100) keywords are
        // synonymous, but we want to provide only a single cannonical
        // API here, so we jump through some small hoops...
        if (deck.hasKeyword("TEMPVD") && deck.hasKeyword("RTEMPVD"))
            throw std::invalid_argument("The TEMPVD and RTEMPVD tables are mutually exclusive!");
        else if (deck.hasKeyword("TEMPVD"))
            initSimpleTableContainer<RtempvdTable>(deck, "TEMPVD", "RTEMPVD", m_eqldims->getNumEquilRegions());
        else if (deck.hasKeyword("RTEMPVD"))
            initSimpleTableContainer<RtempvdTable>(deck, "RTEMPVD", "RTEMPVD" , m_eqldims->getNumEquilRegions());
    }


    void TableManager::initGasvisctTables(const Deck& deck) {

        const std::string keywordName = "GASVISCT";
        size_t numTables = m_tabdims->getNumPVTTables();

        if (!deck.hasKeyword(keywordName))
            return; // the table is not featured by the deck...

        auto& container = forceGetTables(keywordName , numTables);

        if (deck.numKeywords(keywordName) > 1) {
            complainAboutAmbiguousKeyword(deck, keywordName);
            return;
        }

        const auto& tableKeyword = deck.getKeyword(keywordName);
        for (size_t tableIdx = 0; tableIdx < tableKeyword->size(); ++tableIdx) {
            const auto tableRecord = tableKeyword->getRecord( tableIdx );
            const auto dataItem = tableRecord->getItem( 0 );
            if (dataItem->size() > 0) {
                std::shared_ptr<GasvisctTable> table = std::make_shared<GasvisctTable>();
                table->init(deck , dataItem );
                container.addTable( tableIdx , table );
            }
        }
    }


    void TableManager::initPlyshlogTables(const Deck& deck) {
        const std::string keywordName = "PLYSHLOG";

        if (!deck.hasKeyword(keywordName)) {
            return;
        }

        if (!deck.numKeywords(keywordName)) {
            complainAboutAmbiguousKeyword(deck, keywordName);
            return;
        }
        size_t numTables = m_tabdims->getNumPVTTables();
        auto& container = forceGetTables(keywordName , numTables);
        const auto& tableKeyword = deck.getKeyword(keywordName);

        if (tableKeyword->size() > 2) {
            std::string msg = "The Parser does currently NOT support the alternating record schema used in PLYSHLOG";
            throw std::invalid_argument( msg );
        }

        for (size_t tableIdx = 0; tableIdx < tableKeyword->size(); tableIdx += 2) {
            const auto indexRecord = tableKeyword->getRecord( tableIdx );
            const auto dataRecord = tableKeyword->getRecord( tableIdx + 1);
            const auto dataItem = dataRecord->getItem( 0 );
            if (dataItem->size() > 0) {
                std::shared_ptr<PlyshlogTable> table = std::make_shared<PlyshlogTable>();
                table->init(indexRecord , dataRecord);
                container.addTable( tableIdx , table );
            }
        }
    }


    void TableManager::initPlyrockTables(const Deck& deck) {
        size_t numTables = m_tabdims->getNumSatTables();
        const std::string keywordName = "PLYROCK";
        if (!deck.hasKeyword(keywordName)) {
            return;
        }

        if (!deck.numKeywords(keywordName)) {
            complainAboutAmbiguousKeyword(deck, keywordName);
            return;
        }

        const auto& keyword = deck.getKeyword<ParserKeywords::PLYROCK>();
        auto& container = forceGetTables(keywordName , numTables);
        for (size_t tableIdx = 0; tableIdx < keyword->size(); ++tableIdx) {
            const auto tableRecord = keyword->getRecord( tableIdx );
            std::shared_ptr<PlyrockTable> table = std::make_shared<PlyrockTable>();
            table->init( tableRecord );
            container.addTable( tableIdx , table );
        }
    }


    void TableManager::initPlymaxTables(const Deck& deck) {
        size_t numTables = m_regdims->getNPLMIX();
        const std::string keywordName = "PLYMAX";
        if (!deck.hasKeyword(keywordName)) {
            return;
        }

        if (!deck.numKeywords(keywordName)) {
            complainAboutAmbiguousKeyword(deck, keywordName);
            return;
        }

        const auto& keyword = deck.getKeyword<ParserKeywords::PLYMAX>();
        auto& container = forceGetTables(keywordName , numTables);
        for (size_t tableIdx = 0; tableIdx < keyword->size(); ++tableIdx) {
            const auto tableRecord = keyword->getRecord( tableIdx );
            std::shared_ptr<PlymaxTable> table = std::make_shared<PlymaxTable>();
            table->init( tableRecord );
            container.addTable( tableIdx , table );
        }
    }



    void TableManager::initRocktabTables(const Deck& deck) {
        if (!deck.hasKeyword("ROCKTAB"))
            return; // ROCKTAB is not featured by the deck...

        if (deck.numKeywords("ROCKTAB") > 1) {
            complainAboutAmbiguousKeyword(deck, "ROCKTAB");
            return;
        }
        const auto& rockcompKeyword = deck.getKeyword<ParserKeywords::ROCKCOMP>();
        const auto& record = rockcompKeyword->getRecord( 0 );
        size_t numTables = record->getItem<ParserKeywords::ROCKCOMP::NTROCC>()->getInt(0);
        auto& container = forceGetTables("ROCKTAB" , numTables);
        const auto rocktabKeyword = deck.getKeyword("ROCKTAB");

        bool isDirectional = deck.hasKeyword<ParserKeywords::RKTRMDIR>();
        bool useStressOption = false;
        if (deck.hasKeyword<ParserKeywords::ROCKOPTS>()) {
            const auto rockoptsKeyword = deck.getKeyword<ParserKeywords::ROCKOPTS>();
            const auto rockoptsRecord = rockoptsKeyword->getRecord(0);
            const auto item = rockoptsRecord->getItem<ParserKeywords::ROCKOPTS::METHOD>();
            useStressOption = (item->getTrimmedString(0) == "STRESS");
        }

        for (size_t tableIdx = 0; tableIdx < rocktabKeyword->size(); ++tableIdx) {
            const auto tableRecord = rocktabKeyword->getRecord( tableIdx );
            const auto dataItem = tableRecord->getItem( 0 );
            if (dataItem->size() > 0) {
                std::shared_ptr<RocktabTable> table = std::make_shared<RocktabTable>();
                table->init(dataItem , isDirectional, useStressOption);
                container.addTable( tableIdx , table );
            }
        }
    }



    void TableManager::initVFPProdTables(const Deck& deck,
                                          std::map<int, VFPProdTable>& tableMap) {
        if (!deck.hasKeyword(ParserKeywords::VFPPROD::keywordName)) {
            return;
        }

        int num_tables = deck.numKeywords(ParserKeywords::VFPPROD::keywordName);
        const auto& keywords = deck.getKeywordList<ParserKeywords::VFPPROD>();
        const auto unit_system = deck.getActiveUnitSystem();
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

    void TableManager::initVFPInjTables(const Deck& deck,
                                        std::map<int, VFPInjTable>& tableMap) {
        if (!deck.hasKeyword(ParserKeywords::VFPINJ::keywordName)) {
            return;
        }

        int num_tables = deck.numKeywords(ParserKeywords::VFPINJ::keywordName);
        const auto& keywords = deck.getKeywordList<ParserKeywords::VFPINJ>();
        const auto unit_system = deck.getActiveUnitSystem();
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

    std::shared_ptr<const Tabdims> TableManager::getTabdims() const {
        return m_tabdims;
    }


    /*
      const std::vector<SwofTable>& TableManager::getSwofTables() const {
        return m_swofTables;
        }
    */

    const TableContainer& TableManager::getSwofTables() const {
        return getTables("SWOF");
    }

    const TableContainer& TableManager::getSgwfnTables() const {
        return getTables("SGWFN");
    }
    
    const TableContainer& TableManager::getSlgofTables() const {
        return getTables("SLGOF");
    }


    const TableContainer& TableManager::getSgofTables() const {
        return getTables("SGOF");
    }

    const TableContainer& TableManager::getSof2Tables() const {
        return getTables("SOF2");
    }

    const TableContainer& TableManager::getSof3Tables() const {
        return getTables("SOF3");
    }

    const TableContainer& TableManager::getSwfnTables() const {
        return getTables("SWFN");
    }

    const TableContainer& TableManager::getSgfnTables() const {
        return getTables("SGFN");
    }

    const TableContainer& TableManager::getSsfnTables() const {
        return getTables("SSFN");
    }

    const TableContainer& TableManager::getRsvdTables() const {
        return getTables("RSVD");
    }

    const TableContainer& TableManager::getRvvdTables() const {
        return getTables("RVVD");
    }

    const TableContainer& TableManager::getEnkrvdTables() const {
        return getTables("ENKRVD");
    }

    const TableContainer& TableManager::getEnptvdTables() const {
        return getTables("ENPTVD");
    }


    const TableContainer& TableManager::getImkrvdTables() const {
        return getTables("IMKRVD");
    }

    const TableContainer& TableManager::getImptvdTables() const {
        return getTables("IMPTVD");
    }

    const TableContainer& TableManager::getPvdgTables() const {
        return getTables("PVDG");
    }

    const TableContainer& TableManager::getPvdoTables() const {
        return getTables("PVDO");
    }

    const TableContainer& TableManager::getPvdsTables() const {
        return getTables("PVDS");
    }

    const TableContainer& TableManager::getOilvisctTables() const {
        return getTables("OILVISCT");
    }

    const TableContainer& TableManager::getWatvisctTables() const {
        return getTables("WATVISCT");
    }

    const TableContainer& TableManager::getGasvisctTables() const {
        return getTables("GASVISCT");
    }

    const TableContainer& TableManager::getRtempvdTables() const {
        return getTables("RTEMPVD");
    }

    const TableContainer& TableManager::getRocktabTables() const {
        return getTables("ROCKTAB");
    }


    const TableContainer& TableManager::getPlyadsTables() const {
        return getTables("PLYADS");
    }

    const TableContainer& TableManager::getPlyviscTables() const {
        return getTables("PLYVISC");
    }

    const TableContainer& TableManager::getPlydhflfTables() const {
        return getTables("PLYDHFL");
    }

    const TableContainer& TableManager::getPlymaxTables() const {
        return getTables("PLYMAX");
    }

    const TableContainer& TableManager::getPlyrockTables() const {
        return getTables("PLYROCK");
    }

    const TableContainer& TableManager::getPlyshlogTables() const {
        return getTables("PLYSHLOG");
    }

    const std::vector<PvtgTable>& TableManager::getPvtgTables() const {
        return m_pvtgTables;
    }


    const std::vector<PvtoTable>& TableManager::getPvtoTables() const {
        return m_pvtoTables;
    }


    const std::map<int, VFPProdTable>& TableManager::getVFPProdTables() const {
        return m_vfpprodTables;
    }

    const std::map<int, VFPInjTable>& TableManager::getVFPInjTables() const {
        return m_vfpinjTables;
    }


    void TableManager::complainAboutAmbiguousKeyword(const Deck& deck, const std::string& keywordName) const {
        OpmLog::addMessage(Log::MessageType::Error, "The " + keywordName + " keyword must be unique in the deck. Ignoring all!");
        auto keywords = deck.getKeywordList(keywordName);
        for (size_t i = 0; i < keywords.size(); ++i) {
            std::string msg = "Ambiguous keyword "+keywordName+" defined here";
            OpmLog::addMessage(Log::MessageType::Error , Log::fileMessage( keywords[i]->getFileName(), keywords[i]->getLineNumber(),msg));
        }
    }
}


