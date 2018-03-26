/*
  Copyright 2015 Statoil ASA.
  Copyright 2018 IRIS

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

#include <opm/common/OpmLog/LogUtil.hpp>

#include <opm/parser/eclipse/Parser/ParserKeywords/E.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/M.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/V.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/T.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/E.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/M.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/P.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/T.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/V.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/A.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp> // Phase::PhaseEnum
#include <opm/parser/eclipse/EclipseState/Tables/EnkrvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/EnptvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/GasvisctTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/ImkrvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/ImptvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/MiscTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/MsfnTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/OilvisctTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PlyadsTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PlydhflfTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PlymaxTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PlyrockTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PlyshlogTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PlyviscTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PmiscTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TlpmixpaTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PvdgTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PvdoTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PvdsTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/RocktabTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/RsvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PbvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PdvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/RtempvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/RvvdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgcwmisTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgfnTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgwfnTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SlgofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Sof2Table.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Sof3Table.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SorwmisTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SpecheatTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SpecrockTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SsfnTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SwfnTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SwofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableContainer.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/WatvisctTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/AqutabTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/JFunc.hpp>

#include <opm/parser/eclipse/EclipseState/Tables/Tabdims.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Eqldims.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Regdims.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Aqudims.hpp>

#include <opm/parser/eclipse/Units/Units.hpp>

namespace Opm {

    TableManager::TableManager( const Deck& deck )
        :
        m_tabdims( Tabdims(deck)),
        m_aqudims( Aqudims(deck)),
        hasImptvd (deck.hasKeyword("IMPTVD")),
        hasEnptvd (deck.hasKeyword("ENPTVD")),
        hasEqlnum (deck.hasKeyword("EQLNUM")),
        m_jfunc( deck )
    {
        // determine the default resevoir temperature in Kelvin
        m_rtemp = ParserKeywords::RTEMP::TEMP::defaultValue;
        m_rtemp += Metric::TemperatureOffset; // <- default values always use METRIC as the unit system!

        initDims( deck );
        initSimpleTables( deck );
        initFullTables(deck, "PVTG", m_pvtgTables);
        initFullTables(deck, "PVTO", m_pvtoTables);
        if( deck.hasKeyword( "PVTW" ) )
            this->m_pvtwTable = PvtwTable( deck.getKeyword( "PVTW" ) );

        if( deck.hasKeyword( "PVCDO" ) )
            this->m_pvcdoTable = PvcdoTable( deck.getKeyword( "PVCDO" ) );

        if( deck.hasKeyword( "DENSITY" ) )
            this->m_densityTable = DensityTable( deck.getKeyword( "DENSITY" ) );

        if( deck.hasKeyword( "ROCK" ) )
            this->m_rockTable = RockTable( deck.getKeyword( "ROCK" ) );

        if( deck.hasKeyword( "VISCREF" ) )
            this->m_viscrefTable = ViscrefTable( deck.getKeyword( "VISCREF" ) );

        if( deck.hasKeyword( "WATDENT" ) )
            this->m_watdentTable = WatdentTable( deck.getKeyword( "WATDENT" ) );

        initVFPProdTables(deck, m_vfpprodTables);
        initVFPInjTables(deck,  m_vfpinjTables);

        if( deck.hasKeyword( "RTEMP" ) )
            m_rtemp = deck.getKeyword("RTEMP").getRecord(0).getItem("TEMP").getSIDouble( 0 );
        else if (deck.hasKeyword( "RTEMPA" ) )
            m_rtemp = deck.getKeyword("RTEMPA").getRecord(0).getItem("TEMP").getSIDouble( 0 );
    }

    void TableManager::initDims(const Deck& deck) {
        using namespace Opm::ParserKeywords;

        if (deck.hasKeyword<EQLDIMS>()) {
            const auto& keyword = deck.getKeyword<EQLDIMS>();
            const auto& record = keyword.getRecord(0);
            int ntsequl   = record.getItem<EQLDIMS::NTEQUL>().get< int >(0);
            int nodes_p   = record.getItem<EQLDIMS::DEPTH_NODES_P>().get< int >(0);
            int nodes_tab = record.getItem<EQLDIMS::DEPTH_NODES_TAB>().get< int >(0);
            int nttrvd    = record.getItem<EQLDIMS::NTTRVD>().get< int >(0);
            int ntsrvd    = record.getItem<EQLDIMS::NSTRVD>().get< int >(0);

            m_eqldims = std::make_shared<Eqldims>(ntsequl , nodes_p , nodes_tab , nttrvd , ntsrvd );
        } else
            m_eqldims = std::make_shared<Eqldims>();

        if (deck.hasKeyword<REGDIMS>()) {
            const auto& keyword = deck.getKeyword<REGDIMS>();
            const auto& record = keyword.getRecord(0);
            int ntfip  = record.getItem<REGDIMS::NTFIP>().get< int >(0);
            int nmfipr = record.getItem<REGDIMS::NMFIPR>().get< int >(0);
            int nrfreg = record.getItem<REGDIMS::NRFREG>().get< int >(0);
            int ntfreg = record.getItem<REGDIMS::NTFREG>().get< int >(0);
            int nplmix = record.getItem<REGDIMS::NPLMIX>().get< int >(0);
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
        addTables( "SWOF" , m_tabdims.getNumSatTables() );
        addTables( "SGWFN", m_tabdims.getNumSatTables() );
        addTables( "SGOF",  m_tabdims.getNumSatTables() );
        addTables( "SLGOF", m_tabdims.getNumSatTables() );
        addTables( "SOF2",  m_tabdims.getNumSatTables() );
        addTables( "SOF3",  m_tabdims.getNumSatTables() );
        addTables( "SWFN",  m_tabdims.getNumSatTables() );
        addTables( "SGFN",  m_tabdims.getNumSatTables() );
        addTables( "SSFN",  m_tabdims.getNumSatTables() );
        addTables( "MSFN",  m_tabdims.getNumSatTables() );

        addTables( "PLYADS", m_tabdims.getNumSatTables() );
        addTables( "PLYROCK", m_tabdims.getNumSatTables());
        addTables( "PLYVISC", m_tabdims.getNumPVTTables());
        addTables( "PLYDHFLF", m_tabdims.getNumPVTTables());

        addTables( "PVDG", m_tabdims.getNumPVTTables());
        addTables( "PVDO", m_tabdims.getNumPVTTables());
        addTables( "PVDS", m_tabdims.getNumPVTTables());

        addTables( "SPECHEAT", m_tabdims.getNumPVTTables());
        addTables( "SPECROCK", m_tabdims.getNumSatTables());

        addTables( "OILVISCT", m_tabdims.getNumPVTTables());
        addTables( "WATVISCT", m_tabdims.getNumPVTTables());
        addTables( "GASVISCT", m_tabdims.getNumPVTTables());

        addTables( "PLYMAX", m_regdims->getNPLMIX());
        addTables( "RSVD", m_eqldims->getNumEquilRegions());
        addTables( "RVVD", m_eqldims->getNumEquilRegions());
        addTables( "PBVD", m_eqldims->getNumEquilRegions());
        addTables( "PDVD", m_eqldims->getNumEquilRegions());

        addTables( "AQUTAB", m_aqudims.getNumInfluenceTablesCT());
        {
            size_t numMiscibleTables = ParserKeywords::MISCIBLE::NTMISC::defaultValue;
            if (deck.hasKeyword<ParserKeywords::MISCIBLE>()) {
                const auto& keyword = deck.getKeyword<ParserKeywords::MISCIBLE>();
                const auto& record = keyword.getRecord(0);
                numMiscibleTables =  static_cast<size_t>(record.getItem<ParserKeywords::MISCIBLE::NTMISC>().get< int >(0));
            }
            addTables( "SORWMIS", numMiscibleTables);
            addTables( "SGCWMIS", numMiscibleTables);
            addTables( "MISC",    numMiscibleTables);
            addTables( "PMISC",   numMiscibleTables);
            addTables( "TLPMIXPA",numMiscibleTables);
        }

        {
            size_t numEndScaleTables = ParserKeywords::ENDSCALE::NUM_TABLES::defaultValue;

            if (deck.hasKeyword<ParserKeywords::ENDSCALE>()) {
                const auto& keyword = deck.getKeyword<ParserKeywords::ENDSCALE>();
                const auto& record = keyword.getRecord(0);
                numEndScaleTables = static_cast<size_t>(record.getItem<ParserKeywords::ENDSCALE::NUM_TABLES>().get< int >(0));
            }

            addTables( "ENKRVD", numEndScaleTables);
            addTables( "ENPTVD", numEndScaleTables);
            addTables( "IMKRVD", numEndScaleTables);
            addTables( "IMPTVD", numEndScaleTables);
        }
        {
            size_t numRocktabTables = ParserKeywords::ROCKCOMP::NTROCC::defaultValue;

            if (deck.hasKeyword<ParserKeywords::ROCKCOMP>()) {
                const auto& keyword = deck.getKeyword<ParserKeywords::ROCKCOMP>();
                const auto& record = keyword.getRecord(0);
                numRocktabTables = static_cast<size_t>(record.getItem<ParserKeywords::ROCKCOMP::NTROCC>().get< int >(0));
            }
            addTables( "ROCKTAB", numRocktabTables);
        }


        initSimpleTableContainer<SgwfnTable>(deck, "SGWFN", m_tabdims.getNumSatTables());
        initSimpleTableContainer<Sof2Table>(deck, "SOF2" , m_tabdims.getNumSatTables());
        initSimpleTableContainer<Sof3Table>(deck, "SOF3" , m_tabdims.getNumSatTables());
        {
            initSimpleTableContainerWithJFunc<SwofTable>(deck, "SWOF", m_tabdims.getNumSatTables());
            initSimpleTableContainerWithJFunc<SgofTable>(deck, "SGOF", m_tabdims.getNumSatTables());
            initSimpleTableContainerWithJFunc<SwfnTable>(deck, "SWFN", m_tabdims.getNumSatTables());
            initSimpleTableContainerWithJFunc<SgfnTable>(deck, "SGFN", m_tabdims.getNumSatTables());
            initSimpleTableContainerWithJFunc<SlgofTable>(deck, "SLGOF", m_tabdims.getNumSatTables());

        }
        initSimpleTableContainer<SsfnTable>(deck, "SSFN" , m_tabdims.getNumSatTables());
        initSimpleTableContainer<MsfnTable>(deck, "MSFN" , m_tabdims.getNumSatTables());

        initSimpleTableContainer<RsvdTable>(deck, "RSVD" , m_eqldims->getNumEquilRegions());
        initSimpleTableContainer<RvvdTable>(deck, "RVVD" , m_eqldims->getNumEquilRegions());
        initSimpleTableContainer<PbvdTable>(deck, "PBVD" , m_eqldims->getNumEquilRegions());
        initSimpleTableContainer<PdvdTable>(deck, "PDVD" , m_eqldims->getNumEquilRegions());
        initSimpleTableContainer<AqutabTable>(deck, "AQUTAB" , m_aqudims.getNumInfluenceTablesCT());
        {
            size_t numEndScaleTables = ParserKeywords::ENDSCALE::NUM_TABLES::defaultValue;

            if (deck.hasKeyword<ParserKeywords::ENDSCALE>()) {
                const auto& keyword = deck.getKeyword<ParserKeywords::ENDSCALE>();
                const auto& record = keyword.getRecord(0);
                numEndScaleTables = static_cast<size_t>(record.getItem<ParserKeywords::ENDSCALE::NUM_TABLES>().get< int >(0));
            }

            initSimpleTableContainer<EnkrvdTable>( deck , "ENKRVD", numEndScaleTables);
            initSimpleTableContainer<EnptvdTable>( deck , "ENPTVD", numEndScaleTables);
            initSimpleTableContainer<ImkrvdTable>( deck , "IMKRVD", numEndScaleTables);
            initSimpleTableContainer<ImptvdTable>( deck , "IMPTVD", numEndScaleTables);
        }

        {
            size_t numMiscibleTables = ParserKeywords::MISCIBLE::NTMISC::defaultValue;
            if (deck.hasKeyword<ParserKeywords::MISCIBLE>()) {
                const auto& keyword = deck.getKeyword<ParserKeywords::MISCIBLE>();
                const auto& record = keyword.getRecord(0);
                numMiscibleTables =  static_cast<size_t>(record.getItem<ParserKeywords::MISCIBLE::NTMISC>().get< int >(0));
            }
            initSimpleTableContainer<SorwmisTable>(deck, "SORWMIS", numMiscibleTables);
            initSimpleTableContainer<SgcwmisTable>(deck, "SGCWMIS", numMiscibleTables);
            initSimpleTableContainer<MiscTable>(deck, "MISC", numMiscibleTables);
            initSimpleTableContainer<PmiscTable>(deck, "PMISC", numMiscibleTables);
            initSimpleTableContainer<TlpmixpaTable>(deck, "TLPMIXPA", numMiscibleTables);

        }

        initSimpleTableContainer<PvdgTable>(deck, "PVDG", m_tabdims.getNumPVTTables());
        initSimpleTableContainer<PvdoTable>(deck, "PVDO", m_tabdims.getNumPVTTables());
        initSimpleTableContainer<PvdsTable>(deck, "PVDS", m_tabdims.getNumPVTTables());
        initSimpleTableContainer<SpecheatTable>(deck, "SPECHEAT", m_tabdims.getNumPVTTables());
        initSimpleTableContainer<SpecrockTable>(deck, "SPECROCK", m_tabdims.getNumSatTables());
        initSimpleTableContainer<OilvisctTable>(deck, "OILVISCT", m_tabdims.getNumPVTTables());
        initSimpleTableContainer<WatvisctTable>(deck, "WATVISCT", m_tabdims.getNumPVTTables());

        initSimpleTableContainer<PlyadsTable>(deck, "PLYADS", m_tabdims.getNumSatTables());
        initSimpleTableContainer<PlyviscTable>(deck, "PLYVISC", m_tabdims.getNumPVTTables());
        initSimpleTableContainer<PlydhflfTable>(deck, "PLYDHFLF", m_tabdims.getNumPVTTables());
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
        size_t numTables = m_tabdims.getNumPVTTables();

        if (!deck.hasKeyword(keywordName))
            return; // the table is not featured by the deck...

        auto& container = forceGetTables(keywordName , numTables);

        if (deck.count(keywordName) > 1) {
            complainAboutAmbiguousKeyword(deck, keywordName);
            return;
        }

        const auto& tableKeyword = deck.getKeyword(keywordName);
        for (size_t tableIdx = 0; tableIdx < tableKeyword.size(); ++tableIdx) {
            const auto& tableRecord = tableKeyword.getRecord( tableIdx );
            const auto& dataItem = tableRecord.getItem( 0 );
            if (dataItem.size() > 0) {
                std::shared_ptr<GasvisctTable> table = std::make_shared<GasvisctTable>( deck , dataItem );
                container.addTable( tableIdx , table );
            }
        }
    }


    void TableManager::initPlyshlogTables(const Deck& deck) {
        const std::string keywordName = "PLYSHLOG";

        if (!deck.hasKeyword(keywordName)) {
            return;
        }

        if (!deck.count(keywordName)) {
            complainAboutAmbiguousKeyword(deck, keywordName);
            return;
        }
        size_t numTables = m_tabdims.getNumPVTTables();
        auto& container = forceGetTables(keywordName , numTables);
        const auto& tableKeyword = deck.getKeyword(keywordName);

        if (tableKeyword.size() > 2) {
            std::string msg = "The Parser does currently NOT support the alternating record schema used in PLYSHLOG";
            throw std::invalid_argument( msg );
        }

        for (size_t tableIdx = 0; tableIdx < tableKeyword.size(); tableIdx += 2) {
            const auto& indexRecord = tableKeyword.getRecord( tableIdx );
            const auto& dataRecord = tableKeyword.getRecord( tableIdx + 1);
            const auto& dataItem = dataRecord.getItem( 0 );
            if (dataItem.size() > 0) {
                std::shared_ptr<PlyshlogTable> table = std::make_shared<PlyshlogTable>(indexRecord , dataRecord);
                container.addTable( tableIdx , table );
            }
        }
    }


    void TableManager::initPlyrockTables(const Deck& deck) {
        size_t numTables = m_tabdims.getNumSatTables();
        const std::string keywordName = "PLYROCK";
        if (!deck.hasKeyword(keywordName)) {
            return;
        }

        if (!deck.count(keywordName)) {
            complainAboutAmbiguousKeyword(deck, keywordName);
            return;
        }

        const auto& keyword = deck.getKeyword<ParserKeywords::PLYROCK>();
        auto& container = forceGetTables(keywordName , numTables);
        for (size_t tableIdx = 0; tableIdx < keyword.size(); ++tableIdx) {
            const auto& tableRecord = keyword.getRecord( tableIdx );
            std::shared_ptr<PlyrockTable> table = std::make_shared<PlyrockTable>(tableRecord);
            container.addTable( tableIdx , table );
        }
    }


    void TableManager::initPlymaxTables(const Deck& deck) {
        size_t numTables = m_regdims->getNPLMIX();
        const std::string keywordName = "PLYMAX";
        if (!deck.hasKeyword(keywordName)) {
            return;
        }

        if (!deck.count(keywordName)) {
            complainAboutAmbiguousKeyword(deck, keywordName);
            return;
        }

        const auto& keyword = deck.getKeyword<ParserKeywords::PLYMAX>();
        auto& container = forceGetTables(keywordName , numTables);
        for (size_t tableIdx = 0; tableIdx < keyword.size(); ++tableIdx) {
            const auto& tableRecord = keyword.getRecord( tableIdx );
            std::shared_ptr<PlymaxTable> table = std::make_shared<PlymaxTable>( tableRecord );
            container.addTable( tableIdx , table );
        }
    }



    void TableManager::initRocktabTables(const Deck& deck) {
        if (!deck.hasKeyword("ROCKTAB"))
            return; // ROCKTAB is not featured by the deck...

        if (deck.count("ROCKTAB") > 1) {
            complainAboutAmbiguousKeyword(deck, "ROCKTAB");
            return;
        }
        const auto& rockcompKeyword = deck.getKeyword<ParserKeywords::ROCKCOMP>();
        const auto& record = rockcompKeyword.getRecord( 0 );
        size_t numTables = record.getItem<ParserKeywords::ROCKCOMP::NTROCC>().get< int >(0);
        auto& container = forceGetTables("ROCKTAB" , numTables);
        const auto rocktabKeyword = deck.getKeyword("ROCKTAB");

        bool isDirectional = deck.hasKeyword<ParserKeywords::RKTRMDIR>();
        bool useStressOption = false;
        if (deck.hasKeyword<ParserKeywords::ROCKOPTS>()) {
            const auto rockoptsKeyword = deck.getKeyword<ParserKeywords::ROCKOPTS>();
            const auto& rockoptsRecord = rockoptsKeyword.getRecord(0);
            const auto& item = rockoptsRecord.getItem<ParserKeywords::ROCKOPTS::METHOD>();
            useStressOption = (item.getTrimmedString(0) == "STRESS");
        }

        for (size_t tableIdx = 0; tableIdx < rocktabKeyword.size(); ++tableIdx) {
            const auto& tableRecord = rocktabKeyword.getRecord( tableIdx );
            const auto& dataItem = tableRecord.getItem( 0 );
            if (dataItem.size() > 0) {
                std::shared_ptr<RocktabTable> table = std::make_shared<RocktabTable>( dataItem , isDirectional, useStressOption );
                container.addTable( tableIdx , table );
            }
        }
    }



    void TableManager::initVFPProdTables(const Deck& deck,
                                          std::map<int, VFPProdTable>& tableMap) {
        if (!deck.hasKeyword(ParserKeywords::VFPPROD::keywordName)) {
            return;
        }

        int num_tables = deck.count(ParserKeywords::VFPPROD::keywordName);
        const auto& keywords = deck.getKeywordList<ParserKeywords::VFPPROD>();
        const auto& unit_system = deck.getActiveUnitSystem();
        for (int i=0; i<num_tables; ++i) {
            const auto& keyword = *keywords[i];

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

        int num_tables = deck.count(ParserKeywords::VFPINJ::keywordName);
        const auto& keywords = deck.getKeywordList<ParserKeywords::VFPINJ>();
        const auto& unit_system = deck.getActiveUnitSystem();
        for (int i=0; i<num_tables; ++i) {
            const auto& keyword = *keywords[i];

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

    size_t TableManager::numFIPRegions() const {
        size_t ntfip = m_tabdims.getNumFIPRegions();
        if (m_regdims->getNTFIP( ) > ntfip)
            return m_regdims->getNTFIP( );
        else
            return ntfip;
    }

    const Tabdims& TableManager::getTabdims() const {
        return m_tabdims;
    }

    const Eqldims& TableManager::getEqldims() const {
        return *m_eqldims;
    }
    
    const Aqudims& TableManager::getAqudims() const {
        return m_aqudims;
    }

    const Regdims& TableManager::getRegdims() const {
        return *this->m_regdims;
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

    const TableContainer& TableManager::getPbvdTables() const {
        return getTables("PBVD");
    }

    const TableContainer& TableManager::getPdvdTables() const {
        return getTables("PDVD");
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

    const TableContainer& TableManager::getSpecheatTables() const {
        return getTables("SPECHEAT");
    }

    const TableContainer& TableManager::getSpecrockTables() const {
        return getTables("SPECROCK");
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
    
    const TableContainer& TableManager::getAqutabTables() const {
        return getTables("AQUTAB");
    } 

    const std::vector<PvtgTable>& TableManager::getPvtgTables() const {
        return m_pvtgTables;
    }

    const std::vector<PvtoTable>& TableManager::getPvtoTables() const {
        return m_pvtoTables;
    }

    const PvtwTable& TableManager::getPvtwTable() const {
        return this->m_pvtwTable;
    }

    const PvcdoTable& TableManager::getPvcdoTable() const {
        return this->m_pvcdoTable;
    }

    const DensityTable& TableManager::getDensityTable() const {
        return this->m_densityTable;
    }

    const RockTable& TableManager::getRockTable() const {
        return this->m_rockTable;
    }

    const ViscrefTable& TableManager::getViscrefTable() const {
        return this->m_viscrefTable;
    }

    const WatdentTable& TableManager::getWatdentTable() const {
        return this->m_watdentTable;
    }

    const TableContainer& TableManager::getMsfnTables() const {
        return getTables("MSFN");
    }
    const TableContainer& TableManager::getPmiscTables() const {
        return getTables("PMISC");
    }
    const TableContainer& TableManager::getMiscTables() const {
        return getTables("MISC");
    }
    const TableContainer& TableManager::getSgcwmisTables() const {
        return getTables("SGCWMIS");
    }
    const TableContainer& TableManager::getSorwmisTables() const {
        return getTables("SORWMIS");
    }
    const TableContainer& TableManager::getTlpmixpaTables() const {
        return getTables("TLPMIXPA");
    }

    const JFunc& TableManager::getJFunc() const {
        if (!useJFunc())
            throw std::invalid_argument("Cannot get JFUNC table when JFUNC not in deck");
        return m_jfunc;
    }

    const std::map<int, VFPProdTable>& TableManager::getVFPProdTables() const {
        return m_vfpprodTables;
    }

    const std::map<int, VFPInjTable>& TableManager::getVFPInjTables() const {
        return m_vfpinjTables;
    }

    bool TableManager::useImptvd() const {
        return hasImptvd;
    }

    bool TableManager::useEnptvd() const {
        return hasEnptvd;
    }

    bool TableManager::useEqlnum() const {
        return hasEqlnum;
    }

    bool TableManager::useJFunc() const {
        return m_jfunc;
    }


    void TableManager::complainAboutAmbiguousKeyword(const Deck& deck, const std::string& keywordName) {
        OpmLog::error("The " + keywordName + " keyword must be unique in the deck. Ignoring all!");
        const auto& keywords = deck.getKeywordList(keywordName);
        for (size_t i = 0; i < keywords.size(); ++i) {
            std::string msg = "Ambiguous keyword "+keywordName+" defined here";
            OpmLog::error(Log::fileMessage(keywords[i]->getFileName(), keywords[i]->getLineNumber(), msg));
        }
    }

    double TableManager::rtemp() const {
        return this->m_rtemp;
    }

}


