/*
  Copyright 2021 Equinor ASA.

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

#include <opm/input/eclipse/Schedule/ScheduleGrid.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquiferCell.hpp>
#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquifers.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>

#include <opm/input/eclipse/Schedule/CompletedCells.hpp>

#include <cassert>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include <fmt/format.h>

namespace {
    std::vector<Opm::CompletedCells>& emptyCells()
    {
        static auto cells = std::vector<Opm::CompletedCells>{};

        return cells;
    }

    const std::unordered_map<std::string, std::size_t>& emptyLgrLabels()
    {
        static const auto labels = std::unordered_map<std::string, std::size_t>{};

        return labels;
    }
} // Anonymous namespace

// ---------------------------------------------------------------------------

Opm::ScheduleGrid::ScheduleGrid(CompletedCells& completed_cells)
    : cells          { std::ref(completed_cells) }
    , cells_lgr      { std::ref(emptyCells()) }
    , label_to_index { std::cref(emptyLgrLabels()) }
{}

Opm::ScheduleGrid::ScheduleGrid(CompletedCells&              completed_cells,
                                std::vector<CompletedCells>& completed_cells_lgr,
                                const std::unordered_map<std::string, std::size_t>& label_to_index_)
    : cells          { std::ref(completed_cells) }
    , cells_lgr      { std::ref(completed_cells_lgr) }
    , label_to_index { std::cref(label_to_index_) }
{}

Opm::ScheduleGrid::ScheduleGrid(const EclipseGrid&       ecl_grid,
                                const FieldPropsManager& fpm,
                                CompletedCells&          completed_cells)
    : grid           { &ecl_grid }
    , fp             { &fpm }
    , cells          { std::ref(completed_cells) }
    , cells_lgr      { std::ref(emptyCells()) }
    , label_to_index { std::cref(emptyLgrLabels()) }
{}

Opm::ScheduleGrid::ScheduleGrid(const EclipseGrid&           ecl_grid,
                                const FieldPropsManager&     fpm,
                                CompletedCells&              completed_cells,
                                std::vector<CompletedCells>& completed_cells_lgr,
                                const std::unordered_map<std::string, std::size_t>& label_to_index_)
    : grid           { &ecl_grid }
    , fp             { &fpm }
    , cells          { std::ref(completed_cells) }
    , cells_lgr      { std::ref(completed_cells_lgr) }
    , label_to_index { std::cref(label_to_index_) }
{}

void Opm::ScheduleGrid::include_numerical_aquifers(const NumericalAquifers& num_aquifers)
{
    this->num_aqu_cells = num_aquifers.allAquiferCells();
}

const Opm::CompletedCells::Cell&
Opm::ScheduleGrid::get_cell(const std::size_t i,
                            const std::size_t j,
                            const std::size_t k) const
{
    if (this->grid == nullptr) {
        return this->cells.get().get(i, j, k);
    }

    auto [cell_ptr, is_existing_cell] =
        this->cells.get().try_get(i, j, k);

    assert (cell_ptr != nullptr);

    if (! is_existing_cell) {
        // New cell object created.  Populate its property data.
        this->populate_props_from_main_grid(*cell_ptr);
    }

    return *cell_ptr;
}

const Opm::CompletedCells::Cell&
Opm::ScheduleGrid::get_cell(const std::size_t i,
                            const std::size_t j,
                            const std::size_t k,
                            const std::optional<std::string>& tag) const
{
    return ! tag.has_value()
        ? this->get_cell    (i, j, k)
        : this->get_cell_lgr(i, j, k, *tag);
}

const Opm::EclipseGrid* Opm::ScheduleGrid::get_grid() const
{
    return this->grid;
}

int Opm::ScheduleGrid::get_lgr_grid_number(const std::optional<std::string>& lgr_label) const
{
    return lgr_label.has_value()
        ? static_cast<int>(label_to_index.get().at(*lgr_label))
        : 0;
}

// ===========================================================================
// Private member functions and helpers below separator
// ===========================================================================

const Opm::CompletedCells::Cell&
Opm::ScheduleGrid::get_cell_lgr(const std::size_t  i,
                                const std::size_t  j,
                                const std::size_t  k,
                                const std::string& tag) const
{
    const auto tag_index = this->label_to_index.get().at(tag);

    if (tag_index == 0) {
        return this->get_cell(i, j, k);
    }

    auto [cell_ptr, is_existing_cell] =
        this->cells_lgr.get()[tag_index - 1].try_get(i, j, k);

    assert (cell_ptr != nullptr);

    if (! is_existing_cell) {
        // New cell object created.  Populate its property data.
        this->populate_props_lgr(tag, *cell_ptr);
    }

    return *cell_ptr;
}

namespace {
    double try_get_value(const Opm::FieldPropsManager& fp,
                         const std::string&            kw,
                         const std::size_t             active_index)
    {
        if (! fp.has_double(kw)) {
            throw std::logic_error {
                fmt::format("Cell based property '{}' does not exist", kw)
            };
        }

        return fp.try_get<double>(kw)->at(active_index);
    }

    double get_ntg(const Opm::FieldPropsManager& fp,
                   const std::size_t             active_index)
    {
        const auto kw = std::string { "NTG" };

        return fp.has_double(kw)
            ? fp.try_get<double>(kw)->at(active_index)
            : 1.0;
    }

    void populate(const Opm::FieldPropsManager&     fp,
                  const std::size_t                 active_index,
                  Opm::CompletedCells::Cell::Props& props)
    {
        props.permx = try_get_value(fp, "PERMX", active_index);
        props.permy = try_get_value(fp, "PERMY", active_index);
        props.permz = try_get_value(fp, "PERMZ", active_index);
        props.poro  = try_get_value(fp, "PORO" , active_index);
        props.ntg   = get_ntg      (fp,          active_index);

        props.satnum = fp.get_int("SATNUM").at(active_index);
        props.pvtnum = fp.get_int("PVTNUM").at(active_index);
    }
} // Anonymous namespace

void Opm::ScheduleGrid::populate_props_from_main_grid(CompletedCells::Cell& cell) const
{
    cell.depth = this->grid->getCellDepth(cell.global_index);
    cell.dimensions = this->grid->getCellDims(cell.global_index);

    if (const auto* numAquCell = this->get_num_aqu_cell(cell.global_index);
        numAquCell == nullptr)
    {
        // Not in a numerical aquifer.  Pull property values from 'fp'.
        this->populate_props_from_main_grid_cell(cell);
    }
    else {
        // We're in a numerical aquifer.  Pull property values from the
        // aquifer.
        this->populate_props_from_num_aquifer(*numAquCell, cell);
    }
}

void Opm::ScheduleGrid::populate_props_from_main_grid_cell(CompletedCells::Cell& cell) const
{
    if (! this->grid->cellActive(cell.global_index)) {
        return;
    }

    const auto active_index = this->grid->getActiveIndex(cell.global_index);
    const auto porv = try_get_value(*this->fp, "PORV", active_index);

    if (! this->grid->cellActiveAfterMINPV(cell.i, cell.j, cell.k, porv)) {
        return;
    }

    auto& props = cell.props.emplace(CompletedCells::Cell::Props{});

    props.active_index = active_index;

    populate(*this->fp, active_index, props);
}

void Opm::ScheduleGrid::
populate_props_from_num_aquifer(const NumericalAquiferCell& numAquCell,
                                CompletedCells::Cell&       cell) const
{
    auto& props = cell.props.emplace(CompletedCells::Cell::Props{});

    props.active_index = this->grid->getActiveIndex(cell.global_index);

    // Isotropic permeability tensor in numerical aquifer cells.
    props.permx = props.permy = props.permz = numAquCell.permeability;

    props.poro = numAquCell.porosity;
    props.ntg  = 1.0;           // Aquifer cells don't have NTG values.

    props.satnum = numAquCell.sattable;
    props.pvtnum = numAquCell.pvttable;
}

void Opm::ScheduleGrid::
populate_props_lgr(const std::string& tag, CompletedCells::Cell& cell) const
{
    const auto father_global_id = this->grid->
        getLGR_global_father(cell.global_index, tag);

    auto [fi, fj, fk] = this->grid->getIJK(father_global_id);

    // This part relies on the ZCORN and COORDS of the host cells that
    // have not been parsed yet.  The following implementations are a
    // path that compute depths and dimensions of the LGR cells based on
    // the host cells.
    cell.depth = this->grid->getCellDepthLGR(cell.i, cell.j, cell.k, tag);
    cell.dimensions = this->grid->getCellDimensionsLGR(fi, fj, fk, tag);

    const auto& lgr_grid = this->grid->getLGRCell(tag);

    if (!this->grid->cellActive(fi, fj, fk) ||
        !lgr_grid.cellActive(cell.i, cell.j, cell.k))
    {
        return;
    }

    const auto father_active_index = this->grid->getActiveIndex(fi, fj, fk);
    const auto porv = try_get_value(*this->fp, "PORV", father_active_index);

    if (! this->grid->cellActiveAfterMINPV(fi, fj, fk, porv)) {
        return;
    }

    auto& props = cell.props.emplace(CompletedCells::Cell::Props{});

    props.active_index = lgr_grid.getActiveIndex(cell.i, cell.j, cell.k);

    populate(*this->fp, father_active_index, props);
}

const Opm::NumericalAquiferCell*
Opm::ScheduleGrid::get_num_aqu_cell(const std::size_t global_index) const
{
    auto cellPos = this->num_aqu_cells.find(global_index);
    if (cellPos == this->num_aqu_cells.end()) {
        return nullptr;
    }

    return cellPos->second;
}
