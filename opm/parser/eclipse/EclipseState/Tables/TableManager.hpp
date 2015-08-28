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

#ifndef OPM_TABLES_HPP
#define OPM_TABLES_HPP

#include <opm/parser/eclipse/OpmLog/OpmLog.hpp>

#include <opm/parser/eclipse/EclipseState/Tables/Tabdims.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SwofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SlgofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Sof2Table.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Sof3Table.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PvdgTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/PvdoTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SwfnTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgfnTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SsfnTable.hpp>


namespace Opm {

    class Tables {
    public:
        Tables( const Deck& deck );


        std::shared_ptr<const Tabdims> getTabdims() const;
        const std::vector<Sof2Table>& getSof2Tables() const;
        const std::vector<Sof3Table>& getSof3Tables() const;
        const std::vector<SwofTable>& getSwofTables() const;
        const std::vector<SgofTable>& getSgofTables() const;
        const std::vector<SlgofTable>& getSlgofTables() const;
        const std::vector<PvdgTable>& getPvdgTables() const;
        const std::vector<PvdoTable>& getPvdoTables() const;
        const std::vector<SwfnTable>& getSwfnTables() const;
        const std::vector<SgfnTable>& getSgfnTables() const;
        const std::vector<SsfnTable>& getSsfnTables() const;

    private:
        void complainAboutAmbiguousKeyword(const Deck& deck, const std::string& keywordName) const;

        void initTabdims(const Deck& deck);


        template <class TableType>
        void initSimpleTables(const Deck& deck,
                              const std::string& keywordName,
                              std::vector<TableType>& tableVector) {
            if (!deck.hasKeyword(keywordName))
                return; // the table is not featured by the deck...

            if (deck.numKeywords(keywordName) > 1) {
                complainAboutAmbiguousKeyword(deck, keywordName);
                return;
            }

            const auto& tableKeyword = deck.getKeyword(keywordName);
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

        std::vector<SwfnTable> m_swfnTables;
        std::vector<SgfnTable> m_sgfnTables;
        std::vector<SsfnTable> m_ssfnTables;
        std::vector<PvdgTable> m_pvdgTables;
        std::vector<PvdoTable> m_pvdoTables;
        std::vector<Sof2Table> m_sof2Tables;
        std::vector<Sof3Table> m_sof3Tables;
        std::vector<SgofTable> m_sgofTables;
        std::vector<SwofTable> m_swofTables;
        std::vector<SlgofTable> m_slgofTables;
        std::shared_ptr<Tabdims> m_tabdims;
    };
}


#endif
