/*
  Copyright 2015 IRIS
  Copyright 2018--2023 Equinor ASA

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

#include <opm/input/eclipse/EclipseState/Grid/NNC.hpp>

#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/E.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/N.hpp>

#include <algorithm>
#include <cstddef>
#include <deque>
#include <stdexcept>
#include <string>
#include <utility>

namespace Opm
{

namespace {

std::optional<std::size_t> global_index(const EclipseGrid& grid, const DeckRecord& record, std::size_t item_offset) {
    std::size_t i = static_cast<std::size_t>(record.getItem(0 + item_offset).get< int >(0)-1);
    std::size_t j = static_cast<std::size_t>(record.getItem(1 + item_offset).get< int >(0)-1);
    std::size_t k = static_cast<std::size_t>(record.getItem(2 + item_offset).get< int >(0)-1);

    if (i >= grid.getNX())
        return {};

    if (j >= grid.getNY())
        return {};

    if (k >= grid.getNZ())
        return {};

    if (!grid.cellActive(i,j,k))
        return {};

    return grid.getGlobalIndex(i,j,k);
}

std::optional<std::pair<std::size_t, std::size_t>> make_index_pair(const EclipseGrid& grid, const DeckRecord& record) {
    auto g1 = global_index(grid, record, 0);
    auto g2 = global_index(grid, record, 3);
    if (!g1)
        return {};

    if (!g2)
        return {};

    if (*g1 < *g2)
        return std::make_pair(*g1, *g2);
    else
        return std::make_pair(*g2, *g1);
}

bool is_neighbor(const EclipseGrid& grid, std::size_t g1, std::size_t g2) {
    auto diff = g2 - g1;
    if (diff == 0)
        return true;

    if (diff == 1)
        return true;

    if (diff == grid.getNX())
        return true;

    if (diff == grid.getNY())
        return true;

    return false;
}

}

    NNC::NNC(const EclipseGrid& grid, const Deck& deck) {
        this->load_input(grid, deck);
        this->load_edit(grid, deck);
        this->load_editr(grid, deck);
    }

    void NNC::load_input(const EclipseGrid& grid, const Deck& deck) {
        for (const auto& keyword_ptr : deck.getKeywordList<ParserKeywords::NNC>()) {
            for (const auto& record : *keyword_ptr) {
                auto index_pair = make_index_pair(grid, record);
                if (!index_pair)
                    continue;

                auto [g1, g2] = index_pair.value();
                double trans = record.getItem(6).getSIDouble(0);
                this->m_input.emplace_back(g1, g2, trans);
            }

            if (!this->m_nnc_location)
                this->m_nnc_location = keyword_ptr->location();
        }

        std::sort(this->m_input.begin(), this->m_input.end());
    }

    void NNC::load_edit(const EclipseGrid& grid, const Deck& deck) {
        std::vector<NNCdata> nnc_edit;
        for (const auto& keyword_ptr : deck.getKeywordList<ParserKeywords::EDITNNC>()) {
            for (const auto& record : *keyword_ptr) {
                double tran_mult = record.getItem(6).get<double>(0);
                if (tran_mult == 1.0)
                    continue;

                auto index_pair = make_index_pair(grid, record);
                if (!index_pair)
                    continue;

                auto [g1, g2] = index_pair.value();
                if (is_neighbor(grid, g1, g2))
                    continue;

                nnc_edit.emplace_back( g1, g2, tran_mult);
            }

            if (!this->m_edit_location)
                this->m_edit_location = keyword_ptr->location();
        }

        std::sort(nnc_edit.begin(), nnc_edit.end());

        if ( EclipseGrid::hasCornerPointKeywords(deck) ) {
            // For corner-point grids we keep all valid EDITNNC,
            // to handle overlaping cases with generated and inputed NNCs.
            for (const auto& current_edit : nnc_edit) {
                this->add_edit(current_edit);
            }
        }
        else {
            // If we have a corresponding NNC already, then we apply
            // the multiplier from EDITNNC to it. Otherwise we internalize
            // it into m_edit (valid for non-corner-point grids).
            auto current_input = this->m_input.begin();
            for (const auto& current_edit : nnc_edit) {
                if (current_input == this->m_input.end()) {
                    this->add_edit(current_edit);
                    continue;
                }

                if (current_input->cell1 != current_edit.cell1 || current_input->cell2 != current_edit.cell2) {
                    current_input = std::lower_bound(this->m_input.begin(),
                                                    this->m_input.end(),
                                                    NNCdata(current_edit.cell1, current_edit.cell2, 0));
                    if (current_input == this->m_input.end()) {
                        this->add_edit(current_edit);
                        continue;
                    }

                }

                bool edit_processed = false;
                while (current_input != this->m_input.end()
                    && current_input->cell1 == current_edit.cell1
                    && current_input->cell2 == current_edit.cell2)
                {
                    current_input->trans *= current_edit.trans;
                    ++current_input;
                    edit_processed = true;
                }

                if (!edit_processed)
                    this->add_edit(current_edit);
            }
        }
    }

    void NNC::load_editr(const EclipseGrid& grid, const Deck& deck) {
        // Will contain EDITNNCR data in reverse order to be able
        // use unique to only keep the last one from the data file.
        // For that we stable_sort it later. This sort is stable and hence
        // entries for the cell pairs will be consecutive and the last
        // entry that was specified in the data file will come first and
        // will kept by std::unique.
        std::deque<NNCdata> nnc_editr;

        const auto& keyword_list = deck.getKeywordList<ParserKeywords::EDITNNCR>();

        if (keyword_list.empty()) {
            return;
        }

        for (const auto& keyword_ptr : deck.getKeywordList<ParserKeywords::EDITNNCR>()) {
            const auto& records = *keyword_ptr;
            if (records.empty()) {
                continue;
            }
            for (const auto& record : records) {
                auto index_pair = make_index_pair(grid, record);
                if (!index_pair)
                    continue;

                auto [g1, g2] = index_pair.value();
                if (is_neighbor(grid, g1, g2))
                    continue;

                double trans = record.getItem(6).getSIDouble(0);
                nnc_editr.emplace_front(g1, g2, trans);
            }

            if (!this->m_editr_location)
                this->m_editr_location = keyword_ptr->location();
        }

        if (nnc_editr.empty()) {
            return;
        }

        // Sort the deck to make entries for the same cell pair consecutive.
        // Will keep the insertion order of entries for same cell pair as sort
        // is stable.
        std::stable_sort(nnc_editr.begin(), nnc_editr.end());

        // remove duplicates (based on only cell1 and cell2 members)
        // recall that entries for the same cell pairs are consecutive after
        // the stable_sort above, and that the last entry specified in the data file
        // comes first in the deck (because we used emplace_front, and sort is stable)
        auto equality = [](const NNCdata& d1, const NNCdata& d2){
            return d1.cell1 == d2.cell1 && d1.cell2 == d2.cell2;
        };
        auto new_end = std::unique(nnc_editr.begin(), nnc_editr.end(), equality);
        nnc_editr.erase(new_end, nnc_editr.end());

        // Remove corresponding EDITNNC entries in m_edit as EDITNNCR
        // will overwrite transmissibilities anyway
        std::vector<NNCdata> slim_edit;
        slim_edit.reserve(m_edit.size());
        std::set_difference(m_edit.begin(), m_edit.end(),
                            nnc_editr.begin(), nnc_editr.end(),
                            std::back_inserter(slim_edit));
        m_edit = std::move(slim_edit);

        // NNCs are left untouched as they are also needed for
        // grid construction. Transmissibilities are overwritten
        // in the simulator by EDITNNCR, anyway.

        // Create new container to not use excess memory
        // and convert to std::vector
        m_editr.assign(nnc_editr.begin(), nnc_editr.end());
    }

    NNC NNC::serializationTestObject()
    {
        NNC result;
        result.m_input= {{1,2,1.0},{2,3,2.0}};
        result.m_edit= {{1,2,1.0},{2,3,2.0}};
        result.m_editr= {{1,2,1.0},{2,3,2.0}};
        result.m_nnc_location = {"NNC?", "File", 123};
        result.m_edit_location = {"EDITNNC?", "File", 123};
        result.m_editr_location = {"EDITNNCR?", "File", 123};
        return result;
    }

    bool NNC::addNNC(const std::size_t cell1, const std::size_t cell2, const double trans) {
        if (cell1 > cell2)
            return this->addNNC(cell2, cell1, trans);

        auto nnc = NNCdata(cell1, cell2, trans);
        auto insert_iter = std::lower_bound(this->m_input.begin(), this->m_input.end(), nnc);
        this->m_input.insert( insert_iter, nnc);
        return true;
    }

    void NNC::merge(const std::vector<NNCdata>& data) {
        auto old_size = m_input.size();
        m_input.insert(m_input.end(), data.begin(), data.end());

        std::for_each(m_input.begin() + old_size, m_input.end(), [](NNCdata& item){
            if (item.cell1 > item.cell2)
                std::swap(item.cell1, item.cell2);
        });
        std::sort(m_input.begin() + old_size, m_input.end());
        std::inplace_merge(m_input.begin(), m_input.begin() + old_size, m_input.end());
    }

    bool NNC::operator==(const NNC& data) const {
        return m_input == data.m_input &&
               m_edit == data.m_edit &&
               m_editr == data.m_editr &&
               m_edit_location == data.m_edit_location &&
               m_editr_location == data.m_editr_location &&
               m_nnc_location == data.m_nnc_location;

    }

    void NNC::add_edit(const NNCdata& edit_node) {
        if (!this->m_edit.empty()) {
            auto& back = this->m_edit.back();
            if (back.cell1 == edit_node.cell1 && back.cell2 == edit_node.cell2) {
                back.trans *= edit_node.trans;
                return;
            }
        }

        this->m_edit.push_back(edit_node);
    }

   /*
     In principle we can have multiple NNC keywords, and to provide a good error
     message we should be able to return the location of the offending NNC. That
     will require some bookkeeping of which NNC originated in which NNC
     keyword/location. For now we just return the location of the first NNC
     keyword, but we should be ready for a more elaborate implementation without
     any API change.
    */
    KeywordLocation NNC::input_location(const NNCdata& /* nnc */) const {
        if (this->m_nnc_location)
            return *this->m_nnc_location;
        else
            return {};
    }

    KeywordLocation NNC::edit_location(const NNCdata& /* nnc */) const {
        if (this->m_edit_location)
            return *this->m_edit_location;
        else
            return {};
    }

    KeywordLocation NNC::editr_location(const NNCdata& /* nnc */) const {
        if (this->m_editr_location)
            return *this->m_editr_location;
        else
            return {};
    }

// ===========================================================================
// NNCDataContainer — flat, unsorted NNC storage for a single grid
//
// Difference from NNC:
//   NNC is a deck-parsing class.  It reads the NNC, EDITNNC and EDITNNCR
//   keywords from a deck and keeps three separate, sorted vectors (input,
//   edit, editr) together with source-location metadata.  It is the
//   authoritative representation of what was written in the input file.
//
//   NNCDataContainer is a plain runtime container.  It holds a single flat
//   vector of NNCdata entries without any keyword-level structure, sorting
//   guarantees, or location bookkeeping.  It is used to pass NNC data
//   between the grid-construction and simulation layers after deck parsing
//   is complete.
//
//   NNCDataContainerDiffGrid is a specialisation of NNCDataContainer for
//   connections that cross a grid boundary (e.g. LGR-to-global).  It omits
//   the cell-ordering constraint (cell1 <= cell2) because the two cell
//   indices belong to different grids and swapping them would destroy the
//   grid-association information.
// ===========================================================================

/// Inserts an NNC entry (cell1, cell2, trans) into the container, enforcing
/// cell1 <= cell2.
bool NNCDataContainer::addNNC(const std::size_t cell1, const std::size_t cell2, const double trans) {
    if (cell1 > cell2)
        return this->addNNC(cell2, cell1, trans);
    this->nnc_container.emplace_back(cell1,cell2, trans);
    return true;
}

/// Inserts a pre-built NNCdata entry directly into the container.
bool NNCDataContainer::addNNC(const NNCdata nnc_data)
{
    this->nnc_container.push_back(std::move(nnc_data));
    return true;
}

bool NNCDataContainer::operator==(const NNCDataContainer& other) const
{
    return nnc_container == other.nnc_container;
}

// ===========================================================================
// NNCDataContainerDiffGrid — NNC storage for connections between two grids
// ===========================================================================

/// Inserts a cross-grid NNC entry without enforcing any cell ordering, since
/// cell1 and cell2 belong to different grids and swapping them would lose
/// the grid-association information.
bool NNCDataContainerDiffGrid::addNNC(const std::size_t cell1, const std::size_t cell2, const double trans)
{
    nnc_container.emplace_back(cell1, cell2, trans);
    return true;
}

/// Swaps cell1 and cell2 in every entry when grid1 > grid2, so that the
/// container is always stored in normalised (min_grid, max_grid) key order.
void NNCDataContainerDiffGrid::swap_adj(std::size_t grid1, std::size_t grid2)
{
    if (grid1 > grid2) {
        for (auto& item : nnc_container) {
            std::swap(item.cell1, item.cell2);
        }
    }
}

bool NNCDataContainerDiffGrid::operator==(const NNCDataContainerDiffGrid& other) const
{
    return NNCDataContainer::operator==(other);
}

// ===========================================================================
// NNCCollection — indexed collection of same-grid and cross-grid NNCs
// ===========================================================================

/// Default constructor: the global grid (grid 0) always exists, initially
/// with an empty NNC container.
NNCCollection::NNCCollection()
{
    m_sameGridNNCs.emplace(std::size_t{0}, NNCDataContainer{});
}

/// Constructs an NNCCollection pre-populated with @p nnc_global as the
/// global (grid 0) same-grid NNC.
NNCCollection::NNCCollection(NNCDataContainer nnc_global)
{
    addNNC(std::move(nnc_global));
}

/// Adds a cross-grid NNC between @p grid1 and @p grid2.  The key is
/// normalised to (min, max) and cell indices are swapped accordingly.
/// Throws std::runtime_error if an entry for this grid pair already exists.
void NNCCollection::addNNC(std::size_t grid1, std::size_t grid2, NNCDataContainerDiffGrid nnc)
{
    if (grid1 > grid2) {
        nnc.swap_adj(grid1, grid2);
        std::swap(grid1, grid2);
    }

    const auto key = std::make_pair(grid1, grid2);
    if (m_diffGridNNCs.count(key)) {
        throw std::runtime_error(
            "NNCCollection::addNNC: cross-grid NNC already exists for grid pair ("
            + std::to_string(grid1) + ", " + std::to_string(grid2) + ")");
    }
    m_diffGridNNCs.emplace(key, std::move(nnc));
}

/// Returns a const reference to the cross-grid NNC for the (grid1, grid2)
/// pair.  Grid order is normalised automatically.
/// Throws std::runtime_error if no entry exists.
const NNCDataContainerDiffGrid& NNCCollection::getNNC(std::size_t grid1, std::size_t grid2) const
{
    if (grid1 > grid2) std::swap(grid1, grid2);
    const auto key = std::make_pair(grid1, grid2);
    const auto it  = m_diffGridNNCs.find(key);
    if (it == m_diffGridNNCs.end()) {
        throw std::runtime_error(
            "NNCCollection::getNNC: no cross-grid NNC for grid pair ("
            + std::to_string(grid1) + ", " + std::to_string(grid2) + ")");
    }
    return it->second;
}

/// Returns a mutable reference to the cross-grid NNC for the (grid1, grid2) pair.
NNCDataContainerDiffGrid& NNCCollection::getNNC(std::size_t grid1, std::size_t grid2)
{
    return const_cast<NNCDataContainerDiffGrid&>(std::as_const(*this).getNNC(grid1, grid2));
}

bool NNCCollection::hasCrossGridNNC(std::size_t grid1, std::size_t grid2) const
{
    const auto key = std::minmax(grid1, grid2);
    return m_diffGridNNCs.count(key) > 0;
}

/// Adds a same-grid NNC for @p grid.
/// For grid 0 (global), replaces the existing empty entry inserted by the
/// constructor.  For all other grids, throws std::runtime_error if an entry
/// already exists.
void NNCCollection::addNNC(std::size_t grid, NNCDataContainer nnc)
{
    if (grid == 0) {
        m_sameGridNNCs[std::size_t{0}] = std::move(nnc);
        return;
    }
    if (m_sameGridNNCs.count(grid)) {
        throw std::runtime_error(
            "NNCCollection::addNNC: same-grid NNC already exists for grid "
            + std::to_string(grid));
    }
    m_sameGridNNCs.emplace(grid, std::move(nnc));
}

/// Returns a const reference to the same-grid NNC for @p grid.
/// Throws std::runtime_error if no entry exists.
const NNCDataContainer& NNCCollection::getNNC(std::size_t grid) const
{
    const auto it = m_sameGridNNCs.find(grid);
    if (it == m_sameGridNNCs.end()) {
        throw std::runtime_error(
            "NNCCollection::getNNC: no same-grid NNC for grid "
            + std::to_string(grid));
    }
    return it->second;
}

/// Returns a mutable reference to the same-grid NNC for @p grid.
NNCDataContainer& NNCCollection::getNNC(std::size_t grid)
{
    return const_cast<NNCDataContainer&>(std::as_const(*this).getNNC(grid));
}

bool NNCCollection::hasSameGridNNC(std::size_t grid) const
{
    auto it = m_sameGridNNCs.find(grid);
    return it != m_sameGridNNCs.end() && !it->second.input().empty();
}

/// Replaces the global (grid 0) same-grid NNC.
void NNCCollection::addNNC(NNCDataContainer nnc)
{
    m_sameGridNNCs[std::size_t{0}] = std::move(nnc);
}

/// Returns a mutable reference to the global (grid 0) same-grid NNC.
/// Grid 0 is always present (inserted by the constructor); never throws.
NNCDataContainer& NNCCollection::getGlobalNNC()
{
    return getNNC(std::size_t{0});
}

/// Returns a const reference to the global (grid 0) same-grid NNC.
/// Grid 0 is always present (inserted by the constructor); never throws.
const NNCDataContainer& NNCCollection::getGlobalNNC() const
{
    return getNNC(std::size_t{0});
}

/// Builds an NNCCollection from the three output containers produced by
/// EclGenericWriter::exportNncStructure_().
///
/// Index convention (matches the layout written by exportNncStructure_):
///
///   outputNnc[level]
///     → same-grid NNCs for grid \p level (0 = main grid).
///
///   outputNncGlobalLocal[i]
///     → cross-grid NNCs between main grid (level 0) and level i+1.
///
///   outputAmalgamatedNnc[i][j]
///     → cross-grid NNCs between level i+1 and level i+j+2.
NNCCollection NNCCollection::fromLGROutputContainers(
    const std::vector<std::vector<NNCdata>>& outputNnc,
    const std::vector<std::vector<NNCdata>>& outputNncGlobalLocal,
    const std::vector<std::vector<std::vector<NNCdata>>>& outputAmalgamatedNnc)
{
    NNCCollection result;

    // --- same-grid NNCs ---------------------------------------------------
    for (std::size_t level = 0; level < outputNnc.size(); ++level) {
        if (outputNnc[level].empty())
            continue;
        NNCDataContainer container;
        for (const auto& nnc : outputNnc[level])
            container.addNNC(nnc.cell1, nnc.cell2, nnc.trans);
        result.addNNC(level, std::move(container));
    }

    // --- global-to-local NNCs (level 0  <->  level i+1) ------------------
    for (std::size_t i = 0; i < outputNncGlobalLocal.size(); ++i) {
        if (outputNncGlobalLocal[i].empty())
            continue;
        NNCDataContainerDiffGrid container;
        for (const auto& nnc : outputNncGlobalLocal[i])
            container.addNNC(nnc.cell1, nnc.cell2, nnc.trans);
        result.addNNC(std::size_t{0}, i + 1, std::move(container));
    }

    // --- amalgamated NNCs (level i+1  <->  level i+j+2) ------------------
    for (std::size_t i = 0; i < outputAmalgamatedNnc.size(); ++i) {
        const std::size_t smallerLevel = i + 1;
        for (std::size_t j = 0; j < outputAmalgamatedNnc[i].size(); ++j) {
            if (outputAmalgamatedNnc[i][j].empty())
                continue;
            const std::size_t largerLevel = smallerLevel + j + 1;
            NNCDataContainerDiffGrid container;
            for (const auto& nnc : outputAmalgamatedNnc[i][j])
                container.addNNC(nnc.cell1, nnc.cell2, nnc.trans);
            result.addNNC(smallerLevel, largerLevel, std::move(container));
        }
    }

    return result;
}

} // namespace Opm
