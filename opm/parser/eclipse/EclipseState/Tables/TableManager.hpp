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

#ifndef OPM_TABLE_MANAGER_HPP
#define OPM_TABLE_MANAGER_HPP

#include <set>

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/LogUtil.hpp>

#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>

#include <opm/parser/eclipse/EclipseState/Tables/PvtgTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PvtoTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Rock2dTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Rock2dtrTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PvtwsaltTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/BrineDensityTable.hpp>


#include <opm/parser/eclipse/EclipseState/Tables/FlatTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SorwmisTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgcwmisTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/MiscTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PmiscTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/MsfnTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/JFunc.hpp>

#include <opm/parser/eclipse/EclipseState/Tables/Tabdims.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableContainer.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Aqudims.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PlymwinjTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SkprwatTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SkprpolyTable.hpp>

namespace Opm {

    class Eqldims;
    class Regdims;
    class TableManager {
    public:
        explicit TableManager( const Deck& deck );
        TableManager() = default;
        TableManager(const std::map<std::string, TableContainer>& simpleTables,
                     const std::vector<PvtgTable>& pvtgTables,
                     const std::vector<PvtoTable>& pvtoTables,
                     const std::vector<Rock2dTable>& rock2dTables,
                     const std::vector<Rock2dtrTable>& rock2dtrTables,
                     const PvtwTable& pvtwTable,
                     const PvcdoTable& pvcdoTable,
                     const DensityTable& densityTable,
                     const RockTable& rockTable,
                     const ViscrefTable& viscrefTable,
                     const WatdentTable& watdentTable,
                     const std::map<int, PlymwinjTable>& plymwinjTables,
                     const std::map<int, SkprwatTable>& skprwatTables,
                     const std::map<int, SkprpolyTable>& skprpolyTables,
                     const Tabdims& tabdims,
                     const Regdims& regdims,
                     const Eqldims& eqldims,
                     const Aqudims& aqudims,
                     bool useImptvd,
                     bool useEnptvd,
                     bool useEqlnum,
                     std::shared_ptr<JFunc> jfunc_param,
                     double rtemp);

        TableManager& operator=(const TableManager& data);

        const TableContainer& getTables( const std::string& tableName ) const;
        const TableContainer& operator[](const std::string& tableName) const;
        bool hasTables( const std::string& tableName ) const;

        const Tabdims& getTabdims() const;
        const Eqldims& getEqldims() const;
        const Aqudims& getAqudims() const;
        const Regdims& getRegdims() const;

        /*
          WIll return max{ Tabdims::NTFIP , Regdims::NTFIP }.
        */
        size_t numFIPRegions() const;

        const TableContainer& getSwofTables() const;
        const TableContainer& getSgwfnTables() const;
        const TableContainer& getSof2Tables() const;
        const TableContainer& getSof3Tables() const;
        const TableContainer& getSgofTables() const;
        const TableContainer& getSlgofTables() const;
        const TableContainer& getSwfnTables() const;
        const TableContainer& getSgfnTables() const;
        const TableContainer& getSsfnTables() const;
        const TableContainer& getRsvdTables() const;
        const TableContainer& getRvvdTables() const;
        const TableContainer& getPbvdTables() const;
        const TableContainer& getPdvdTables() const;
        const TableContainer& getSaltvdTables() const;
        const TableContainer& getEnkrvdTables() const;
        const TableContainer& getEnptvdTables() const;
        const TableContainer& getImkrvdTables() const;
        const TableContainer& getImptvdTables() const;
        const TableContainer& getPvdgTables() const;
        const TableContainer& getPvdoTables() const;
        const TableContainer& getPvdsTables() const;
        const TableContainer& getSpecheatTables() const;
        const TableContainer& getSpecrockTables() const;
        const TableContainer& getWatvisctTables() const;
        const TableContainer& getOilvisctTables() const;
        const TableContainer& getGasvisctTables() const;
        const TableContainer& getRtempvdTables() const;
        const TableContainer& getRocktabTables() const;
        const TableContainer& getPlyadsTables() const;
        const TableContainer& getPlyviscTables() const;
        const TableContainer& getPlydhflfTables() const;
        const TableContainer& getPlymaxTables() const;
        const TableContainer& getPlyrockTables() const;
        const TableContainer& getPlyshlogTables() const;
        const TableContainer& getAqutabTables() const;
        const TableContainer& getFoamadsTables() const;
        const TableContainer& getFoammobTables() const;

        const TableContainer& getSorwmisTables() const;
        const TableContainer& getSgcwmisTables() const;
        const TableContainer& getMiscTables() const;
        const TableContainer& getPmiscTables() const;
        const TableContainer& getMsfnTables() const;
        const TableContainer& getTlpmixpaTables() const;

        const JFunc& getJFunc() const;

        const std::vector<PvtgTable>& getPvtgTables() const;
        const std::vector<PvtoTable>& getPvtoTables() const;
        const std::vector<Rock2dTable>& getRock2dTables() const;
        const std::vector<Rock2dtrTable>& getRock2dtrTables() const;
        const TableContainer& getRockwnodTables() const;
        const TableContainer& getOverburdTables() const;

        const PvtwTable& getPvtwTable() const;
        const std::vector<PvtwsaltTable>& getPvtwSaltTables() const;
        const std::vector<BrineDensityTable>& getBrineDensityTables() const;

        const PvcdoTable& getPvcdoTable() const;
        const DensityTable& getDensityTable() const;
        const RockTable& getRockTable() const;
        const ViscrefTable& getViscrefTable() const;
        const WatdentTable& getWatdentTable() const;
        const std::map<int, PlymwinjTable>& getPlymwinjTables() const;
        const std::map<int, SkprwatTable>& getSkprwatTables() const;
        const std::map<int, SkprpolyTable>& getSkprpolyTables() const;
        const std::map<std::string, TableContainer>& getSimpleTables() const;

        /// deck has keyword "IMPTVD" --- Imbition end-point versus depth tables
        bool useImptvd() const;

        /// deck has keyword "ENPTVD" --- Saturation end-point versus depth tables
        bool useEnptvd() const;

        /// deck has keyword "EQLNUM" --- Equilibriation region numbers
        bool useEqlnum() const;

        /// deck has keyword "JFUNC" --- Use Leverett's J Function for capillary pressure
        bool useJFunc() const;

        double rtemp() const;

        bool operator==(const TableManager& data) const;
    private:
        TableContainer& forceGetTables( const std::string& tableName , size_t numTables);

        void complainAboutAmbiguousKeyword(const Deck& deck, const std::string& keywordName);

        void addTables( const std::string& tableName , size_t numTables);
        void initSimpleTables(const Deck& deck);
        void initRTempTables(const Deck& deck);
        void initDims(const Deck& deck);
        void initRocktabTables(const Deck& deck);
        void initGasvisctTables(const Deck& deck);

        void initPlymaxTables(const Deck& deck);
        void initPlyrockTables(const Deck& deck);
        void initPlyshlogTables(const Deck& deck);

        void initPlymwinjTables(const Deck& deck);
        void initSkprwatTables(const Deck& deck);
        void initSkprpolyTables(const Deck& deck);

        //void initRockTables(const Deck& deck, const std::string& keywordName);

        template <class TableType>
        void initRockTables(const Deck& deck, const std::string& keywordName, std::vector<TableType>& rocktable ) {

            if (!deck.hasKeyword(keywordName))
                return;

            if (!deck.hasKeyword("ROCKCOMP")) {
                OpmLog::error("ROCKCOMP must be present if ROCK2DTR is used");
            }

            if (!deck.hasKeyword("ROCKWNOD")) {
                OpmLog::error("ROCKWNOD must be present if ROCK2DTR is used");
            }

            const auto& rockcompKeyword = deck.getKeyword("ROCKCOMP");
            const auto& record = rockcompKeyword.getRecord( 0 );
            size_t numTables = record.getItem("NTROCC").get< int >(0);
            rocktable.resize(numTables);

            const auto& keyword = deck.getKeyword(keywordName);
            size_t numEntries = keyword.size();
            size_t regionIdx = 0;
            size_t tableIdx = 0;
            for (unsigned lineIdx = 0; lineIdx < numEntries; ++lineIdx) {
                if (keyword.getRecord(lineIdx).getItem("PRESSURE").hasValue(0)) {
                    rocktable[regionIdx].init(keyword.getRecord(lineIdx), tableIdx);
                    tableIdx++;
                } else { // next region
                    tableIdx = 0;
                    regionIdx++;
                }
            }
            assert(regionIdx == numTables - 1 );
        }


        template <class TableType>
        void initPvtwsaltTables(const Deck& deck,  std::vector<TableType>& pvtwtables ) {

            size_t numTables = m_tabdims.getNumPVTTables();
            pvtwtables.resize(numTables);

            const auto& keyword = deck.getKeyword("PVTWSALT");
            size_t numEntries = keyword.size();
            size_t regionIdx = 0;
            for (unsigned lineIdx = 0; lineIdx < numEntries; ++lineIdx) {
                pvtwtables[regionIdx].init(keyword.getRecord(lineIdx), keyword.getRecord(lineIdx+1));
                ++regionIdx;
                ++lineIdx;
            }
            assert(regionIdx == numTables);
        }

        template <class TableType>
        void initBrineTables(const Deck& deck,  std::vector<TableType>& brinetables ) {

            size_t numTables = m_tabdims.getNumPVTTables();
            brinetables.resize(numTables);

            const auto& keyword = deck.getKeyword("BDENSITY");
            size_t numEntries = keyword.size();
            assert(numEntries == numTables);
            for (unsigned lineIdx = 0; lineIdx < numEntries; ++lineIdx) {
                brinetables[lineIdx].init(keyword.getRecord(lineIdx));
            }
        }


        /**
         * JFUNC
         */
        template <class TableType>
        void initSimpleTableContainerWithJFunc(const Deck& deck,
                                      const std::string& keywordName,
                                      const std::string& tableName,
                                      size_t numTables) {
            if (!deck.hasKeyword(keywordName))
                return; // the table is not featured by the deck...

            auto& container = forceGetTables(tableName , numTables);

            if (deck.count(keywordName) > 1) {
                complainAboutAmbiguousKeyword(deck, keywordName);
                return;
            }

            const auto& tableKeyword = deck.getKeyword(keywordName);
            for (size_t tableIdx = 0; tableIdx < tableKeyword.size(); ++tableIdx) {
                const auto& dataItem = tableKeyword.getRecord( tableIdx ).getItem("DATA");
                if (dataItem.data_size() > 0) {
                    std::shared_ptr<TableType> table = std::make_shared<TableType>( dataItem, useJFunc() );
                    container.addTable( tableIdx , table );
                }
            }
        }



        template <class TableType>
        void initSimpleTableContainer(const Deck& deck,
                                      const std::string& keywordName,
                                      const std::string& tableName,
                                      size_t numTables) {
            if (!deck.hasKeyword(keywordName))
                return; // the table is not featured by the deck...

            auto& container = forceGetTables(tableName , numTables);

            if (deck.count(keywordName) > 1) {
                complainAboutAmbiguousKeyword(deck, keywordName);
                return;
            }

            const auto& tableKeyword = deck.getKeyword(keywordName);
            for (size_t tableIdx = 0; tableIdx < tableKeyword.size(); ++tableIdx) {
                const auto& dataItem = tableKeyword.getRecord( tableIdx ).getItem("DATA");
                if (dataItem.data_size() > 0) {
                    std::shared_ptr<TableType> table = std::make_shared<TableType>( dataItem );
                    container.addTable( tableIdx , table );
                }
            }
        }

        template <class TableType>
        void initSimpleTableContainer(const Deck& deck,
                                      const std::string& keywordName,
                                      size_t numTables) {
            initSimpleTableContainer<TableType>(deck , keywordName , keywordName , numTables);
        }


        template <class TableType>
        void initSimpleTableContainerWithJFunc(const Deck& deck,
                                      const std::string& keywordName,
                                      size_t numTables) {
            initSimpleTableContainerWithJFunc<TableType>(deck , keywordName , keywordName , numTables);
        }



        template <class TableType>
        void initSimpleTable(const Deck& deck,
                              const std::string& keywordName,
                              std::vector<TableType>& tableVector) {
            if (!deck.hasKeyword(keywordName))
                return; // the table is not featured by the deck...

            if (deck.count(keywordName) > 1) {
                complainAboutAmbiguousKeyword(deck, keywordName);
                return;
            }

            const auto& tableKeyword = deck.getKeyword(keywordName);
            for (size_t tableIdx = 0; tableIdx < tableKeyword.size(); ++tableIdx) {
                const auto& dataItem = tableKeyword.getRecord( tableIdx ).getItem("DATA");
                if (dataItem.data_size() == 0) {
                    // for simple tables, an empty record indicates that the previous table
                    // should be copied...
                    if (tableIdx == 0) {
                        std::string msg = "The first table for keyword "+keywordName+" must be explicitly defined! Ignoring keyword";
                        const auto& location = tableKeyword.location();
                        OpmLog::warning(Log::fileMessage(location, msg));
                        return;
                    }
                    tableVector.push_back(tableVector.back());
                    continue;
                }

                tableVector.push_back(TableType());
                tableVector[tableIdx].init(dataItem);
            }
        }


        template <class TableType>
        void initFullTables(const Deck& deck,
                            const std::string& keywordName,
                            std::vector<TableType>& tableVector) {
            if (!deck.hasKeyword(keywordName))
                return; // the table is not featured by the deck...

            if (deck.count(keywordName) > 1) {
                complainAboutAmbiguousKeyword(deck, keywordName);
                return;
            }

            const auto& tableKeyword = deck.getKeyword(keywordName);

            int numTables = TableType::numTables( tableKeyword );
            for (int tableIdx = 0; tableIdx < numTables; ++tableIdx)
                tableVector.emplace_back( tableKeyword , tableIdx );
        }

        std::map<std::string , TableContainer> m_simpleTables;
        std::vector<PvtgTable> m_pvtgTables;
        std::vector<PvtoTable> m_pvtoTables;
        std::vector<Rock2dTable> m_rock2dTables;
        std::vector<Rock2dtrTable> m_rock2dtrTables;
        PvtwTable m_pvtwTable;
        PvcdoTable m_pvcdoTable;
        DensityTable m_densityTable;
        RockTable m_rockTable;
        ViscrefTable m_viscrefTable;
        WatdentTable m_watdentTable;
        std::vector<PvtwsaltTable> m_pvtwsaltTables;
        std::vector<BrineDensityTable> m_bdensityTables;
        std::map<int, PlymwinjTable> m_plymwinjTables;
        std::map<int, SkprwatTable> m_skprwatTables;
        std::map<int, SkprpolyTable> m_skprpolyTables;

        Tabdims m_tabdims;
        std::shared_ptr<Regdims> m_regdims;
        std::shared_ptr<Eqldims> m_eqldims;
        Aqudims m_aqudims;

        bool hasImptvd = false;// if deck has keyword IMPTVD
        bool hasEnptvd = false;// if deck has keyword ENPTVD
        bool hasEqlnum = false;// if deck has keyword EQLNUM
        std::shared_ptr<JFunc> jfunc;

        double m_rtemp;
    };
}


#endif
