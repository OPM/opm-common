/*
  Copyright (C) 2020 SINTEF Digital

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

#include <opm/parser/eclipse/Parser/ParserKeywords/A.hpp>

#include <fmt/format.h>
#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/KeywordLocation.hpp>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>

#include <opm/parser/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquiferCell.hpp>
#include <opm/parser/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquifers.hpp>

namespace Opm {

    NumericalAquifers::NumericalAquifers(const Deck& deck, const EclipseGrid& grid,
                                         const FieldPropsManager& field_props) {
        using AQUNUM=ParserKeywords::AQUNUM;
        if ( !deck.hasKeyword<AQUNUM>() ) return;

        // there might be multiple keywords of keyword AQUNUM, it is not totally
        // clear about the rules here. For now, we take care of all the keywords
        const auto& aqunum_keywords = deck.getKeywordList<AQUNUM>();
        for (const auto& keyword : aqunum_keywords) {
            for (const auto& record : *keyword) {
                const NumericalAquiferCell aqu_cell(record, grid, field_props);
                if (this->hasCell(aqu_cell.global_index)) {
                    auto error = fmt::format("Numerical aquifer cell at ({}, {}, {}) is declared more than once",
                                             aqu_cell.I + 1, aqu_cell.J + 1, aqu_cell.K + 1);
                    throw OpmInputError(error, keyword->location());
                } else {
                    this->addAquiferCell(aqu_cell);
                }
            }
        }

        this->addAquiferConnections(deck, grid);
    }


    void NumericalAquifers::addAquiferCell(const NumericalAquiferCell& aqu_cell) {
        const size_t id = aqu_cell.aquifer_id;
        if (!this->hasAquifer(id)) {
            this->aquifers_.insert(std::make_pair(id, SingleNumericalAquifer{id}));
        }

        auto& aquifer = this->aquifers_.at(id);
        aquifer.addAquiferCell(aqu_cell);

        this->aquifer_cells_.insert(std::pair{aqu_cell.global_index, aquifer.getCellPrt(aquifer.numCells())});
    }

    bool NumericalAquifers::hasCell(const size_t cell_global_index) const {
        const auto& cells = this->aquifer_cells_;
        return (cells.find(cell_global_index) != cells.end());
    }

    const NumericalAquiferCell& NumericalAquifers::getCell(const size_t cell_global_index) const {
        assert(this->hasCell(cell_global_index));
        return *(this->aquifer_cells_.at(cell_global_index));
    }


    bool NumericalAquifers::hasAquifer(const size_t aquifer_id) const {
        return (this->aquifers_.find(aquifer_id) != this->aquifers_.end());
    }

    void NumericalAquifers::addAquiferConnections(const Deck& deck, const EclipseGrid& grid) {
        NumericalAquiferConnections cons(deck, grid);

        for (auto& pair : this->aquifers_) {
            const size_t aqu_id = pair.first;
            const auto& aqu_cons = cons.getConnections(aqu_id);
            auto& aquifer = pair.second;

            // For now, there is no two aquifers can be connected to one cell
            // aquifer can not connect to aquifer cells
            for (const auto& con : aqu_cons) {
                const auto& aqu_con = con.second;
                const size_t con_global_index = aqu_con.global_index;
                if (this->hasCell(con_global_index)) {
                    const size_t cell_aquifer_id = this->getCell(con_global_index).aquifer_id;
                    auto msg = fmt::format("Problem with keyword AQUCON \n"
                                           "Aquifer connection declared at grid cell ({}, {}, {}), is a aquifer cell "
                                           "of Aquifer {}, and will be removed",
                                           aqu_con.I + 1, aqu_con.J + 1, aqu_con.K + 1, cell_aquifer_id);
                    OpmLog::warning(msg);
                    continue;
                }

                aquifer.addAquiferConnection(con.second);
            }
        }
    }
}