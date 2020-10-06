/*
  Copyright (C) 2020 Equinor ASA
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

  ?You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <opm/parser/eclipse/Parser/ParserKeywords/A.hpp>

#include <opm/parser/eclipse/EclipseState/NumericalAquifer.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>

namespace Opm {

NumericalAquifers::NumericalAquifers(const Deck& deck)
{
    using AQUNUM=ParserKeywords::AQUNUM;
    if ( !deck.hasKeyword<AQUNUM>() ) return;
    using CellIndices = std::array<int, 3>;
    // TODO: with a map, we might change the order of the input.
    // TODO: maybe we should use a vector here, and another variable to handle the the duplication
    std::vector<NumericalAquiferCell> aquifer_cells;
    std::map<CellIndices, size_t> cell_index;

    // there might be multiple keywords of keyword AQUNUM, it is not totally
    // clear about the rules here. For now, we take care of all the keywords
    const auto& aqunum_keywords = deck.getKeywordList<AQUNUM>();
    for (const auto& keyword : aqunum_keywords) {
        for (const auto& record : *keyword) {
            const NumericalAquiferCell aqu_cell(record);
            CellIndices cell_indices {aqu_cell.I, aqu_cell.J, aqu_cell.K};
            // Not sure how to handle duplicated input for aquifer cells yet, throw here
            // until we
            if (cell_index.find(cell_indices) != cell_index.end()) {
                throw;
            }
            aquifer_cells.push_back(aqu_cell);
        }
    }

    for (const auto& aqu_cell : aquifer_cells) {
        this->addAquiferCell(aqu_cell);
    }
}

bool NumericalAquifers::hasAquifer(const int aquifer_id) const {
    return (this->aquifers_.find(aquifer_id) != this->aquifers_.end());
}

void NumericalAquifers::addAquiferCell(const NumericalAquiferCell& aqu_cell) {
    const int id = aqu_cell.aquifer_id;
    if (this->hasAquifer(id)) {
        this->aquifers_.at(id).addAquiferCell(aqu_cell);
    } else {
        this->aquifers_.insert(std::make_pair(id, SingleNumericalAquifer{id}));
        this->aquifers_.at(id).addAquiferCell(aqu_cell);
    }
}


using AQUNUM = ParserKeywords::AQUNUM;
NumericalAquiferCell::NumericalAquiferCell(const DeckRecord& record)
   : aquifer_id( record.getItem<AQUNUM::AQUIFER_ID>().get<int>(0) )
   , I ( record.getItem<AQUNUM::I>().get<int>(0) - 1 )
   , J ( record.getItem<AQUNUM::J>().get<int>(0) - 1 )
   , K ( record.getItem<AQUNUM::K>().get<int>(0) - 1 )
   , area (record.getItem<AQUNUM::CROSS_SECTION>().getSIDouble(0) )
   , length ( record.getItem<AQUNUM::LENGTH>().getSIDouble(0) )
   , porosity ( record.getItem<AQUNUM::PORO>().getSIDouble(0) )
   , permeability( record.getItem<AQUNUM::PERM>().getSIDouble(0) )
{
    // TODO: test with has Value?
    if ( !record.getItem<AQUNUM::DEPTH>().defaultApplied(0) ) {
        this->depth = record.getItem<AQUNUM::DEPTH>().getSIDouble(0);
    }
    if ( !record.getItem<AQUNUM::INITIAL_PRESSURE>().defaultApplied(0) ) {
        this->init_pressure = record.getItem<AQUNUM::INITIAL_PRESSURE>().getSIDouble(0);
    }

    if ( !record.getItem<AQUNUM::PVT_TABLE_NUM>().defaultApplied(0) ) {
        this->pvttable = record.getItem<AQUNUM::PVT_TABLE_NUM>().get<int>(0);
    }

    if ( !record.getItem<AQUNUM::SAT_TABLE_NUM>().defaultApplied(0) ) {
        this->sattable = record.getItem<AQUNUM::SAT_TABLE_NUM>().get<int>(0);
    }
}

void SingleNumericalAquifer::addAquiferCell(const NumericalAquiferCell& aqu_cell) {
    cells_.push_back(aqu_cell);
}

SingleNumericalAquifer::SingleNumericalAquifer(const int aqu_id)
: id_(aqu_id)
{
}

}