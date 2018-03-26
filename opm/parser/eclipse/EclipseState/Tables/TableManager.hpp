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

#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp> // Phase::PhaseEnum
#include <opm/parser/eclipse/EclipseState/Tables/PvtgTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PvtoTable.hpp>

#include <opm/parser/eclipse/EclipseState/Tables/FlatTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/VFPProdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/VFPInjTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SorwmisTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgcwmisTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/MiscTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PmiscTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/MsfnTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/JFunc.hpp>

#include <opm/parser/eclipse/EclipseState/Tables/Tabdims.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableContainer.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/VFPInjTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/VFPProdTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Aqudims.hpp>

namespace Opm {

    class Eqldims;
    class Regdims;
    
    class TableManager {
    public:
        explicit TableManager( const Deck& deck );

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

        const TableContainer& getSorwmisTables() const;
        const TableContainer& getSgcwmisTables() const;
        const TableContainer& getMiscTables() const;
        const TableContainer& getPmiscTables() const;
        const TableContainer& getMsfnTables() const;
        const TableContainer& getTlpmixpaTables() const;

        const JFunc& getJFunc() const;

        const std::vector<PvtgTable>& getPvtgTables() const;
        const std::vector<PvtoTable>& getPvtoTables() const;
        const PvtwTable& getPvtwTable() const;
        const PvcdoTable& getPvcdoTable() const;
        const DensityTable& getDensityTable() const;
        const RockTable& getRockTable() const;
        const ViscrefTable& getViscrefTable() const;
        const WatdentTable& getWatdentTable() const;
        const std::map<int, VFPProdTable>& getVFPProdTables() const;
        const std::map<int, VFPInjTable>& getVFPInjTables() const;

        /// deck has keyword "IMPTVD" --- Imbition end-point versus depth tables
        bool useImptvd() const;

        /// deck has keyword "ENPTVD" --- Saturation end-point versus depth tables
        bool useEnptvd() const;

        /// deck has keyword "EQLNUM" --- Equilibriation region numbers
        bool useEqlnum() const;

        /// deck has keyword "JFUNC" --- Use Leverett's J Function for capillary pressure
        bool useJFunc() const;

        double rtemp() const;
    private:
        TableContainer& forceGetTables( const std::string& tableName , size_t numTables);

        void complainAboutAmbiguousKeyword(const Deck& deck, const std::string& keywordName);

        void addTables( const std::string& tableName , size_t numTables);
        void initSimpleTables(const Deck& deck);
        void initRTempTables(const Deck& deck);
        void initDims(const Deck& deck);
        void initRocktabTables(const Deck& deck);
        void initGasvisctTables(const Deck& deck);

        void initVFPProdTables(const Deck& deck,
                               std::map<int, VFPProdTable>& tableMap);

        void initVFPInjTables(const Deck& deck,
                              std::map<int, VFPInjTable>& tableMap);


        void initPlymaxTables(const Deck& deck);
        void initPlyrockTables(const Deck& deck);
        void initPlyshlogTables(const Deck& deck);




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
                const auto& dataItem = tableKeyword.getRecord( tableIdx ).getItem( 0 );
                if (dataItem.size() > 0) {
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
                const auto& dataItem = tableKeyword.getRecord( tableIdx ).getItem( 0 );
                if (dataItem.size() > 0) {
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
                const auto& dataItem = tableKeyword.getRecord( tableIdx ).getItem( 0 );
                if (dataItem.size() == 0) {
                    // for simple tables, an empty record indicates that the previous table
                    // should be copied...
                    if (tableIdx == 0) {
                        std::string msg = "The first table for keyword "+keywordName+" must be explicitly defined! Ignoring keyword";
                        OpmLog::warning(Log::fileMessage(tableKeyword.getFileName(), tableKeyword.getLineNumber(), msg));
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
        std::map<int, VFPProdTable> m_vfpprodTables;
        std::map<int, VFPInjTable> m_vfpinjTables;
        std::vector<PvtgTable> m_pvtgTables;
        std::vector<PvtoTable> m_pvtoTables;
        PvtwTable m_pvtwTable;
        PvcdoTable m_pvcdoTable;
        DensityTable m_densityTable;
        RockTable m_rockTable;
        ViscrefTable m_viscrefTable;
        WatdentTable m_watdentTable;

        Tabdims m_tabdims;
        std::shared_ptr<Regdims> m_regdims;
        std::shared_ptr<Eqldims> m_eqldims;
        Aqudims m_aqudims;

        const bool hasImptvd;// if deck has keyword IMPTVD
        const bool hasEnptvd;// if deck has keyword ENPTVD
        const bool hasEqlnum;// if deck has keyword EQLNUM


        const JFunc m_jfunc;

        double m_rtemp;
    };
}


#endif
