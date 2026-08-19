/*
  Copyright 2022 Equinor
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
#include <opm/input/eclipse/EclipseState/Grid/Carfin.hpp>
#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp> //CARFIN

#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <string_view>
#include <utility>

#include <fmt/format.h>

namespace {

    void assert_dims(const std::string& name, int l1 , int l2, int nlgr, int nglobal)
    {
        if ((l1 < 0) || (l2 < 0) || (l1 > l2))
            throw std::invalid_argument(name + ": Invalid index values for lgr");

        if (l2 >= nglobal)
            throw std::invalid_argument(name + ": Index values for lgr greater than global grid size");
    }

    bool update_default_index(const Opm::DeckItem& item,
                              int&                 value)
    {
        if (item.defaultApplied(0)) {
            return true;
        }

        value = item.get<int>(0) - 1;
        return false;
    }

    bool update_default(const Opm::DeckItem& item,
                        int&                 value)
    {
        if (item.defaultApplied(0)) {
            return true;
        }

        value = item.get<int>(0);
        return false;
    }

    bool update_default_name(const Opm::DeckItem& item,
                             std::string&        value)
    {
        if (item.defaultApplied(0)) {
            return true;
        }

        value = item.get<std::string>(0);
        return false;
    }
}

namespace Opm
{

    Carfin::Carfin(const GridDims& gridDims,
             IsActive        isActive,
             ActiveIdx       activeIdx)
        : m_globalGridDims_ (gridDims)
        , m_globalIsActive_ (std::move(isActive))
        , m_globalActiveIdx_(std::move(activeIdx))
    {
        this->reset();
    }

    /// @brief Construct a Carfin object with local grid refinement parameters.
    /// @param gridDims The dimensions of the global grid.
    /// @param isActive A functor to determine if a cell is active.
    /// @param activeIdx A functor to get the active index of a cell.
    /// @param name The name of the local grid refinement.
    /// @param i1 The starting index in the i-direction.
    /// @param i2 The ending index in the i-direction.
    /// @param j1 The starting index in the j-direction.
    /// @param j2 The ending index in the j-direction.
    /// @param k1 The starting index in the k-direction.
    /// @param k2 The ending index in the k-direction.
    /// @param nx The number of refined cells in the i-direction.
    /// @param ny The number of refined cells in the j-direction.
    /// @param nz The number of refined cells in the k-direction.
    ///
    /// Note that the indices i1, i2, j1, j2, k1, k2 are 0-based and inclusive.
    /// For example for a global grid with dimensions (1, 3, 1) the (j1, j2) indices
    /// need to be in the range [0, 2]. The ending indices give the index of the last
    /// cell to be refined, and not one-beyond last as typical of C++ iterator ranges.
    Carfin::Carfin(const GridDims& gridDims,
                   IsActive        isActive,
                   ActiveIdx       activeIdx,
                   const std::string& name,
                   const int i1, const int i2,
                   const int j1, const int j2,
                   const int k1, const int k2,
                   const int nx, const int ny,
                   const int nz)
        : m_globalGridDims_ (gridDims)
        , m_globalIsActive_ (std::move(isActive))
        , m_globalActiveIdx_(std::move(activeIdx))
    {
        this->init(name,i1,i2,j1,j2,k1,k2,nx,ny,nz);
    }

    Carfin Carfin::serializationTestObject()
    {
        auto allActive = [](const std::size_t) { return true; };
        auto identity  = [](const std::size_t globalIdx) { return globalIdx; };

        auto lgr = Carfin {
            GridDims { 10, 10, 10 }, allActive, identity,
            "LGRCHILD", 1, 4, 1, 4, 1, 4, 4, 4, 4
        };

        // Not reachable through the public constructors, which always leave the
        // parent as GLOBAL; set it directly so the round trip covers a nested
        // CARFIN.
        lgr.parent_name_grid = "LGRPARENT";

        return lgr;
    }

    void Carfin::setGrading(const std::size_t dim,
                           std::vector<int> counts,
                           std::vector<double> widths)
    {
        const auto direction = std::string_view{"XYZ"}.substr(dim, 1);
        const auto nparent = this->upper(dim) - this->lower(dim) + 1;
        const auto nrefined = this->m_dims[dim];

        if (!counts.empty()) {
            if (counts.size() != nparent) {
                throw std::invalid_argument {
                    fmt::format("CARFIN '{}': N{}FIN has {} entries but the box spans "
                                "{} cells in {}. One entry per parent cell is required.",
                                this->name_grid, direction, counts.size(), nparent, direction)
                };
            }

            const auto total = std::accumulate(counts.begin(), counts.end(), std::size_t{0},
                                               [](const std::size_t acc, const int n)
                                               { return acc + std::max(n, 0); });

            if (std::ranges::any_of(counts, [](const int n) { return n < 1; })) {
                throw std::invalid_argument {
                    fmt::format("CARFIN '{}': N{}FIN gives a parent cell fewer than one "
                                "refined cell.", this->name_grid, direction)
                };
            }

            if (total != nrefined) {
                throw std::invalid_argument {
                    fmt::format("CARFIN '{}': N{}FIN adds up to {} but the record asks for "
                                "{} refined cells in {}. The subdivisions must account for "
                                "every refined cell.",
                                this->name_grid, direction, total, nrefined, direction)
                };
            }
        }

        if (!widths.empty()) {
            if (widths.size() != nrefined) {
                throw std::invalid_argument {
                    fmt::format("CARFIN '{}': H{}FIN has {} entries but the box has {} "
                                "refined cells in {}. One width per refined cell is "
                                "required.",
                                this->name_grid, direction, widths.size(), nrefined, direction)
                };
            }

            if (std::ranges::any_of(widths, [](const double w) { return !(w > 0.0); })) {
                throw std::invalid_argument {
                    fmt::format("CARFIN '{}': H{}FIN gives a refined cell a width that is "
                                "not positive.", this->name_grid, direction)
                };
            }
        }

        this->m_grading[dim] = AxisGrading{ std::move(counts), std::move(widths) };
    }

    bool Carfin::isGraded() const
    {
        return std::ranges::any_of(this->m_grading, [](const AxisGrading& axis)
                                   { return !axis.counts.empty() || !axis.widths.empty(); });
    }

    void Carfin::setMinpv(const double minpv)
    {
        this->m_minpv = minpv;
    }

    const std::optional<double>& Carfin::MINPV() const
    {
        return this->m_minpv;
    }

    void Carfin::setMinpvRemoved(std::vector<int> removed)
    {
        this->m_minpv_removed = std::move(removed);
    }

    const std::vector<int>& Carfin::minpvRemoved() const
    {
        return this->m_minpv_removed;
    }

    const std::array<Carfin::AxisGrading, 3>& Carfin::grading() const
    {
        return this->m_grading;
    }

    void Carfin::validateSubdivision() const
    {
        for (std::size_t dim = 0; dim < 3; ++dim) {
            if (!this->m_grading[dim].counts.empty()) {
                continue;       // N*FIN says how they are distributed
            }

            const auto nparent = this->upper(dim) - this->lower(dim) + 1;
            if (this->m_dims[dim] % nparent != 0) {
                const auto direction = std::string_view{"XYZ"}.substr(dim, 1);
                throw std::invalid_argument {
                    fmt::format("CARFIN '{}': {} refined cells in {} do not divide evenly "
                                "over the {} parent cells. Give N{}FIN to distribute them, "
                                "or choose a multiple.",
                                this->name_grid, this->m_dims[dim], direction,
                                nparent, direction)
                };
            }
        }
    }

    Carfin::RefinedColumns Carfin::refinedColumns(const std::size_t dim) const
    {
        const auto nparent = this->upper(dim) - this->lower(dim) + 1;
        const auto nrefined = this->m_dims[dim];

        // Without N*FIN every parent cell takes the same share.
        auto counts = this->m_grading[dim].counts;
        if (counts.empty()) {
            counts.assign(nparent, static_cast<int>(nrefined / nparent));
        }

        RefinedColumns columns;
        columns.parentOffset.reserve(nrefined);
        columns.fracLo.reserve(nrefined);
        columns.fracHi.reserve(nrefined);
        columns.firstColumn.reserve(counts.size());
        columns.count.assign(counts.begin(), counts.end());

        const auto& widths = this->m_grading[dim].widths;
        std::size_t column = 0;

        for (std::size_t parent = 0; parent < counts.size(); ++parent) {
            const auto n = static_cast<std::size_t>(counts[parent]);
            columns.firstColumn.push_back(static_cast<int>(column));

            // H*FIN widths are relative and normalised within the parent cell,
            // so a cell's share is its width over the sum across that cell.
            double total = static_cast<double>(n);
            if (!widths.empty()) {
                total = std::accumulate(widths.begin() + column,
                                        widths.begin() + column + n, 0.0);
            }

            double cumulative = 0.0;
            for (std::size_t sub = 0; sub < n; ++sub, ++column) {
                const double share = widths.empty() ? 1.0 : widths[column];

                columns.parentOffset.push_back(static_cast<int>(parent));
                columns.fracLo.push_back(cumulative / total);
                cumulative += share;
                // The last share closes the cell exactly, without rounding.
                columns.fracHi.push_back((sub + 1 == n) ? 1.0 : cumulative / total);
            }
        }

        return columns;
    }

    void Carfin::update(const DeckRecord& deckRecord)
    {

        auto default_count = 0;

        std::string name = "LGR";
        default_count += update_default_name(deckRecord.getItem<ParserKeywords::CARFIN::NAME>(), name);

        std::string parent_name = "GLOBAL";
        default_count += update_default_name(deckRecord.getItem<ParserKeywords::CARFIN::PARENT>(), parent_name);

        int i1 = 0;
        int i2 = this->m_globalGridDims_.getNX() - 1;
        default_count += update_default_index(deckRecord.getItem<ParserKeywords::CARFIN::I1>(), i1);
        default_count += update_default_index(deckRecord.getItem<ParserKeywords::CARFIN::I2>(), i2);

        int j1 = 0;
        int j2 = this->m_globalGridDims_.getNY() - 1;
        default_count += update_default_index(deckRecord.getItem<ParserKeywords::CARFIN::J1>(), j1);
        default_count += update_default_index(deckRecord.getItem<ParserKeywords::CARFIN::J2>(), j2);

        int k1 = 0;
        int k2 = this->m_globalGridDims_.getNZ() - 1;
        default_count += update_default_index(deckRecord.getItem<ParserKeywords::CARFIN::K1>(), k1);
        default_count += update_default_index(deckRecord.getItem<ParserKeywords::CARFIN::K2>(), k2);

        int nx = this->m_globalGridDims_.getNX();
        int ny = this->m_globalGridDims_.getNY();
        int nz = this->m_globalGridDims_.getNZ();
        default_count += update_default(deckRecord.getItem<ParserKeywords::CARFIN::NX>(), nx);
        default_count += update_default(deckRecord.getItem<ParserKeywords::CARFIN::NY>(), ny);
        default_count += update_default(deckRecord.getItem<ParserKeywords::CARFIN::NZ>(), nz);

        if (default_count != 11) {
            this->init(name, i1, i2, j1, j2, k1, k2, nx, ny, nz, parent_name);
        }
    }

    void Carfin::reset()
    {
        this->init("LGR", 0, this->m_globalGridDims_.getNX() - 1,
                   0, this->m_globalGridDims_.getNY() - 1,
                   0, this->m_globalGridDims_.getNZ() - 1,
                   this->m_globalGridDims_.getNX(), this->m_globalGridDims_.getNY(), this->m_globalGridDims_.getNZ());
    }

    void Carfin::init(const std::string& name,
                      const int i1, const int i2,
                      const int j1, const int j2,
                      const int k1, const int k2,
                      const int nx, const int ny,
                      const int nz, const std::string& parent_name)
    {
        assert_dims(name, i1 , i2, nx, this->m_globalGridDims_.getNX());
        assert_dims(name, j1 , j2, ny, this->m_globalGridDims_.getNY());
        assert_dims(name, k1 , k2, nz, this->m_globalGridDims_.getNZ());

        this->name_grid = name;
        this->parent_name_grid = parent_name;
        this->m_dims[0] = nx;
        this->m_dims[1] = ny;
        this->m_dims[2] = nz;

        this->m_offset[0] = static_cast<std::size_t>(i1);
        this->m_offset[1] = static_cast<std::size_t>(j1);
        this->m_offset[2] = static_cast<std::size_t>(k1);

        this->m_end_offset[0] = static_cast<std::size_t>(i2);
        this->m_end_offset[1] = static_cast<std::size_t>(j2);
        this->m_end_offset[2] = static_cast<std::size_t>(k2);

        this->initIndexList();
    }

    std::size_t Carfin::size() const
    {
        return m_dims[0] * m_dims[1] * m_dims[2];
    }

    std::size_t Carfin::num_parent_cells() const
    {
        return (upper(0) - lower(0) + 1) *
               (upper(1) - lower(1) + 1) *
               (upper(2) - lower(2) + 1);
    }



    bool Carfin::isGlobal() const
    {
        return this->size() == this->m_globalGridDims_.getCartesianSize();
    }

    std::size_t Carfin::getDim(std::size_t idim) const
    {
        if (idim >= 3) {
            throw std::invalid_argument("The input dimension value is invalid");
        }

        return m_dims[idim];
    }

    const std::vector<Carfin::cell_index>& Carfin::index_list() const {
        return this->m_active_index_list;
    }

    const std::vector<Carfin::cell_index>& Carfin::global_index_list() const {
        return this->m_global_index_list;
    }

    void Carfin::initIndexList()
    {
        this->m_active_index_list.clear();
        this->m_global_index_list.clear();

        const auto lgrdims = GridDims(this->m_dims[0], this->m_dims[1], this->m_dims[2]);
        const auto ncells = lgrdims.getCartesianSize();

        const auto columns = std::array {
            this->refinedColumns(0), this->refinedColumns(1), this->refinedColumns(2)
        };

        for (auto data_index = 0*ncells; data_index != ncells; ++data_index) {
            const auto lgrIJK = lgrdims.getIJK(data_index);
            const auto global_index = this->m_globalGridDims_
                .getGlobalIndex(this->m_offset[0] + columns[0].parentOffset[lgrIJK[0]],
                        this->m_offset[1] + columns[1].parentOffset[lgrIJK[1]],
                        this->m_offset[2] + columns[2].parentOffset[lgrIJK[2]]);

            if (this->m_globalIsActive_(global_index)) {
                const auto active_index = this->m_globalActiveIdx_(global_index);
                this->m_active_index_list.emplace_back(global_index, active_index, data_index);
            }

            this->m_global_index_list.emplace_back(global_index, data_index);
        }
    }

    bool Carfin::operator==(const Carfin& other) const
    {
        return (this->m_dims == other.m_dims)
            && (this->m_offset == other.m_offset)
            && (this->m_end_offset == other.m_end_offset)
            && (this->name_grid == other.name_grid)
            && (this->parent_name_grid == other.parent_name_grid);
    }

    bool Carfin::equal(const Carfin& other) const
    {
        return *this == other;
    }

    std::size_t Carfin::lower(int dim) const {
        return m_offset[dim];
    }

    std::size_t Carfin::upper(int dim) const {
        return m_end_offset[dim];
    }

    std::size_t Carfin::dimension(int dim) const {
        return m_dims[dim];
    }

    const std::string& Carfin::NAME() const
    {
        return name_grid;
    }
    const std::string& Carfin::PARENT_NAME() const
    {
        return parent_name_grid;
    }

    int Carfin::I1() const {
        return lower(0);
    }

    int Carfin::I2() const {
        return upper(0);
    }

    int Carfin::J1() const {
        return lower(1);
    }

    int Carfin::J2() const {
        return upper(1);
    }

    int Carfin::K1() const {
        return lower(2);
    }

    int Carfin::K2() const {
        return upper(2);
    }

    int Carfin::NX() const {
        return dimension(0);
    }

    int Carfin::NY() const {
        return dimension(1);
    }

    int Carfin::NZ() const {
        return dimension(2);
    }

}
