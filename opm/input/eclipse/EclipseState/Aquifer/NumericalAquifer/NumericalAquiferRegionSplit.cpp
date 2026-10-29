/*
  Copyright 2026 Equinor ASA.

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

#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquiferRegionSplit.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquiferCell.hpp>
#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquiferConnection.hpp>
#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquifers.hpp>
#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/SingleNumericalAquifer.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/Carfin.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Grid/LgrCollection.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <algorithm>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

namespace {

// Connection properties which must be equal for connections in one AQUCON record.
using ConnectionAttributes = std::tuple<Opm::FaceDir::DirEnum, double, int, bool, double, double>;

ConnectionAttributes attributes(const Opm::NumericalAquiferConnection& con)
{
    return { con.face_dir, con.trans_multipler, con.trans_option,
             con.connect_active_cell, con.ve_frac_relperm, con.ve_frac_cappress };
}

template <class Map>
auto mostFrequent(const Map& counts)
{
    // Ties are resolved in favour of the smallest key.
    auto best = counts.begin();
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        if (it->second > best->second) {
            best = it;
        }
    }
    return best->first;
}

// Cell on the other side of the connecting face, if inside the grid.
std::optional<std::size_t> faceNeighbour(const Opm::EclipseGrid& grid,
                                         const Opm::NumericalAquiferConnection& con)
{
    auto i = static_cast<long>(con.I), j = static_cast<long>(con.J), k = static_cast<long>(con.K);
    switch (con.face_dir) {
    case Opm::FaceDir::XMinus: --i; break;
    case Opm::FaceDir::XPlus:  ++i; break;
    case Opm::FaceDir::YMinus: --j; break;
    case Opm::FaceDir::YPlus:  ++j; break;
    case Opm::FaceDir::ZMinus: --k; break;
    case Opm::FaceDir::ZPlus:  ++k; break;
    default: return std::nullopt;
    }
    if (i < 0 || j < 0 || k < 0 ||
        i >= static_cast<long>(grid.getNX()) ||
        j >= static_cast<long>(grid.getNY()) ||
        k >= static_cast<long>(grid.getNZ()))
    {
        return std::nullopt;
    }
    return grid.getGlobalIndex(i, j, k);
}

std::string aquconFace(const Opm::FaceDir::DirEnum face)
{
    switch (face) {
    case Opm::FaceDir::XPlus:  return "I+";
    case Opm::FaceDir::XMinus: return "I-";
    case Opm::FaceDir::YPlus:  return "J+";
    case Opm::FaceDir::YMinus: return "J-";
    case Opm::FaceDir::ZPlus:  return "K+";
    case Opm::FaceDir::ZMinus: return "K-";
    default:                   return "?";
    }
}

std::string idList(const std::vector<std::size_t>& ids)
{
    return fmt::format("{}", fmt::join(ids, ", "));
}

/// Covers a set of cells by disjoint boxes.  Boxes may also contain cells
/// which are allowed to be included without effect (inactive cells), but no
/// other cells, and no cell which already belongs to a box (\p boxed).
class BoxCover
{
public:
    BoxCover(const Opm::EclipseGrid& grid,
             const std::unordered_set<std::size_t>& filler)
        : grid_(grid)
        , filler_(filler)
    {}

    std::vector<std::array<std::size_t, 6>>
    cover(const std::vector<std::size_t>& cells, std::unordered_set<std::size_t>& boxed) const
    {
        const auto nx = grid_.getNX();
        const auto ny = grid_.getNY();
        const auto nz = grid_.getNZ();

        const std::unordered_set<std::size_t> target(cells.begin(), cells.end());
        auto allowed = [&](std::size_t i, std::size_t j, std::size_t k) {
            const auto g = grid_.getGlobalIndex(i, j, k);
            return !boxed.contains(g) && (target.contains(g) || filler_.contains(g));
        };
        auto planeAllowed = [&](std::size_t i1, std::size_t i2,
                                std::size_t j1, std::size_t j2,
                                std::size_t k1, std::size_t k2)
        {
            for (auto k = k1; k <= k2; ++k) {
                for (auto j = j1; j <= j2; ++j) {
                    for (auto i = i1; i <= i2; ++i) {
                        if (!allowed(i, j, k)) {
                            return false;
                        }
                    }
                }
            }
            return true;
        };

        auto sorted = cells;
        std::ranges::sort(sorted);

        std::vector<std::array<std::size_t, 6>> boxes;
        for (const auto g : sorted) {
            if (boxed.contains(g)) {
                continue;
            }

            const auto [ii, jj, kk] = grid_.getIJK(g);
            const auto i0 = static_cast<std::size_t>(ii);
            const auto j0 = static_cast<std::size_t>(jj);
            const auto k0 = static_cast<std::size_t>(kk);

            auto i1 = i0, j1 = j0, k1 = k0;
            while (i1 + 1 < nx && allowed(i1 + 1, j0, k0)) { ++i1; }
            while (j1 + 1 < ny && planeAllowed(i0, i1, j1 + 1, j1 + 1, k0, k0)) { ++j1; }
            while (k1 + 1 < nz && planeAllowed(i0, i1, j0, j1, k1 + 1, k1 + 1)) { ++k1; }

            // Shrink to the cells to be covered.
            std::array<std::size_t, 3> hi{i0, j0, k0};
            for (auto k = k0; k <= k1; ++k) {
                for (auto j = j0; j <= j1; ++j) {
                    for (auto i = i0; i <= i1; ++i) {
                        if (target.contains(grid_.getGlobalIndex(i, j, k))) {
                            hi = { std::max(hi[0], i), std::max(hi[1], j), std::max(hi[2], k) };
                        }
                    }
                }
            }

            for (auto k = k0; k <= hi[2]; ++k) {
                for (auto j = j0; j <= hi[1]; ++j) {
                    for (auto i = i0; i <= hi[0]; ++i) {
                        boxed.insert(grid_.getGlobalIndex(i, j, k));
                    }
                }
            }
            boxes.push_back({i0, hi[0], j0, hi[1], k0, hi[2]});
        }

        return boxes;
    }

private:
    const Opm::EclipseGrid& grid_;
    const std::unordered_set<std::size_t>& filler_;
};

} // Anonymous namespace

namespace Opm {

NumericalAquiferRegionSplit::
NumericalAquiferRegionSplit(const NumericalAquifers& aquifers,
                            const std::vector<std::size_t>& aquiferIds,
                            const EclipseGrid& grid,
                            const FieldPropsManager& fieldProps,
                            const LgrCollection& lgrs)
{
    const auto& pvtnum = fieldProps.get_int("PVTNUM");
    const std::vector<int> unitEqlnum(grid.getNumActive(), 1);
    const auto& eqlnum = fieldProps.has_int("EQLNUM") ? fieldProps.get_int("EQLNUM") : unitEqlnum;

    auto regionOf = [&](std::size_t globalIndex) { return eqlnum[grid.activeIndex(globalIndex)]; };
    auto pvtOf = [&](std::size_t globalIndex) { return pvtnum[grid.activeIndex(globalIndex)]; };

    // Cells which may be included in AQUCON boxes without creating
    // connections are the inactive ones.  New aquifer cells are placed in
    // inactive cells which are not already aquifer cells.
    std::unordered_set<std::size_t> usedCells;
    for (const auto& [id, aquifer] : aquifers.aquifers()) {
        for (std::size_t c = 0; c < aquifer.numCells(); ++c) {
            usedCells.insert(aquifer.getCellPrt(c)->global_index);
        }
    }
    std::unordered_set<std::size_t> inactive;
    for (std::size_t g = 0; g < grid.getCartesianSize(); ++g) {
        if (!grid.cellActive(g) && !usedCells.contains(g)) {
            inactive.insert(g);
        }
    }

    // An aquifer cell next to a connected cell would make the connection
    // adjoin an active cell, whereby it would be dropped.
    std::unordered_set<std::size_t> nextToConnection;
    {
        const auto nx = grid.getNX(), ny = grid.getNY(), nz = grid.getNZ();
        for (const auto& [id, aquifer] : aquifers.aquifers()) {
            for (const auto& con : aquifer.connections()) {
                const auto i = con.I, j = con.J, k = con.K;
                if (i > 0)      { nextToConnection.insert(grid.getGlobalIndex(i - 1, j, k)); }
                if (i + 1 < nx) { nextToConnection.insert(grid.getGlobalIndex(i + 1, j, k)); }
                if (j > 0)      { nextToConnection.insert(grid.getGlobalIndex(i, j - 1, k)); }
                if (j + 1 < ny) { nextToConnection.insert(grid.getGlobalIndex(i, j + 1, k)); }
                if (k > 0)      { nextToConnection.insert(grid.getGlobalIndex(i, j, k - 1)); }
                if (k + 1 < nz) { nextToConnection.insert(grid.getGlobalIndex(i, j, k + 1)); }
            }
        }
    }

    // Aquifer cells may not lie inside local grid refinements (see
    // EclipseState::checkNumericalAquifersOutsideLgrs()).
    auto insideLgr = [&grid, &lgrs](std::size_t globalIndex) {
        const auto [i, j, k] = grid.getIJK(globalIndex);
        for (std::size_t n = 0; n < lgrs.size(); ++n) {
            const auto& lgr = lgrs.getLgr(n);
            if ((i >= lgr.I1()) && (i <= lgr.I2()) &&
                (j >= lgr.J1()) && (j <= lgr.J2()) &&
                (k >= lgr.K1()) && (k <= lgr.K2()))
            {
                return true;
            }
        }
        return false;
    };

    std::size_t nextId = 1;
    for (const auto& [id, aquifer] : aquifers.aquifers()) {
        nextId = std::max(nextId, id + 1);
    }

    // Cells inside generated AQUCON boxes must not become aquifer cells.
    std::unordered_set<std::size_t> inAquconBox;

    std::set<std::size_t> replacedAqunum;
    std::set<std::size_t> replacedAqucon;

    auto requested = aquiferIds;
    std::ranges::sort(requested);
    requested.erase(std::unique(requested.begin(), requested.end()), requested.end());

    for (const auto id : requested) {
        if (!aquifers.hasAquifer(id)) {
            continue;
        }
        const auto& aquifer = aquifers.getAquifer(id);
        if (aquifer.numCells() == 0) {
            continue;
        }

        // Connections of active cells grouped by the EQLNUM region of the cell.
        std::map<int, std::vector<const NumericalAquiferConnection*>> connectionsByRegion;
        for (const auto& con : aquifer.connections()) {
            if (grid.cellActive(con.global_index) && !usedCells.contains(con.global_index)) {
                connectionsByRegion[regionOf(con.global_index)].push_back(&con);
            }
        }
        if (connectionsByRegion.empty()) {
            continue;
        }

        std::map<int, std::size_t> hostRegions;
        for (std::size_t c = 0; c < aquifer.numCells(); ++c) {
            ++hostRegions[regionOf(aquifer.getCellPrt(c)->global_index)];
        }

        // The aquifer keeps the connections to the region of its own cells
        // if possible, otherwise those to the region with most connections.
        std::map<int, std::size_t> connectionCount;
        for (const auto& [region, cons] : connectionsByRegion) {
            connectionCount[region] = cons.size();
        }
        int keepRegion = mostFrequent(connectionCount);
        if (hostRegions.size() == 1 && connectionsByRegion.contains(hostRegions.begin()->first)) {
            keepRegion = hostRegions.begin()->first;
        }

        // Host cells for the new aquifers.
        std::map<int, std::vector<std::size_t>> newHosts;
        bool resolved = true;
        {
            const auto first = aquifer.getCellPrt(0)->global_index;
            const auto ncart = grid.getCartesianSize();
            std::size_t offset = 1;
            for (const auto& [region, cons] : connectionsByRegion) {
                if (region == keepRegion) {
                    continue;
                }
                auto& hosts = newHosts[region];
                for (; offset < ncart && hosts.size() < aquifer.numCells(); ++offset) {
                    const auto candidate = (first + offset) % ncart;
                    if (inactive.contains(candidate) &&
                        !inAquconBox.contains(candidate) &&
                        !nextToConnection.contains(candidate) &&
                        !insideLgr(candidate))
                    {
                        hosts.push_back(candidate);
                    }
                }
                if (hosts.size() < aquifer.numCells()) {
                    resolved = false;
                }
            }
        }
        if (!resolved) {
            this->unresolved_.push_back(id);
            continue;
        }

        auto dominantPvt = [&](int region) {
            std::map<int, std::size_t> counts;
            for (const auto* con : connectionsByRegion.at(region)) {
                ++counts[pvtOf(con->global_index)];
            }
            return mostFrequent(counts);
        };

        // Original aquifer: EQLNUM of its cells, and AQUNUM records if its
        // PVT table or initial pressure changes.
        {
            const int pvt = dominantPvt(keepRegion);
            bool replace = false;
            for (std::size_t c = 0; c < aquifer.numCells(); ++c) {
                const auto* cell = aquifer.getCellPrt(c);
                replace = replace || (cell->pvttable != pvt) || cell->init_pressure.has_value();
                if (regionOf(cell->global_index) != keepRegion) {
                    this->eqlnum_.push_back({keepRegion, id, {cell->I, cell->J, cell->K}});
                }
            }
            if (replace) {
                replacedAqunum.insert(id);
                for (std::size_t c = 0; c < aquifer.numCells(); ++c) {
                    const auto* cell = aquifer.getCellPrt(c);
                    this->aqunum_.push_back({id, id, cell->global_index, {cell->I, cell->J, cell->K},
                                             cell->area, cell->length, cell->porosity,
                                             cell->permeability, cell->depth, pvt, cell->sattable,
                                             keepRegion, cell->init_pressure.has_value()});
                }
            }
        }

        if (newHosts.empty()) {
            continue;
        }

        // New aquifers, one per remaining region, as copies of the original.
        replacedAqucon.insert(id);
        std::map<int, std::size_t> idOfRegion{{keepRegion, id}};
        for (const auto& [region, hosts] : newHosts) {
            const auto newId = nextId++;
            idOfRegion[region] = newId;
            const int pvt = dominantPvt(region);
            for (std::size_t c = 0; c < aquifer.numCells(); ++c) {
                const auto* cell = aquifer.getCellPrt(c);
                const auto [i, j, k] = grid.getIJK(hosts[c]);
                const std::array<std::size_t, 3> ijk{static_cast<std::size_t>(i),
                                                     static_cast<std::size_t>(j),
                                                     static_cast<std::size_t>(k)};
                this->aqunum_.push_back({newId, id, hosts[c], ijk,
                                         cell->area, cell->length, cell->porosity,
                                         cell->permeability, cell->depth, pvt, cell->sattable,
                                         region, cell->init_pressure.has_value()});
                this->eqlnum_.push_back({region, newId, ijk});
                usedCells.insert(hosts[c]);
                inactive.erase(hosts[c]);
            }
        }

        // A connection adjoining a cell of its aquifer is kept even without
        // ALLOW_INTERNAL_CELLS, but only for the aquifer owning that cell.
        std::unordered_set<std::size_t> sourceCells;
        for (std::size_t c = 0; c < aquifer.numCells(); ++c) {
            sourceCells.insert(aquifer.getCellPrt(c)->global_index);
        }

        // AQUCON records.  Boxes of one aquifer must be disjoint, since a
        // cell may only be declared once as a connection of an aquifer.
        const BoxCover boxCover(grid, inactive);
        for (const auto& [region, cons] : connectionsByRegion) {
            const auto aquId = idOfRegion.at(region);
            std::map<ConnectionAttributes, std::vector<std::size_t>> groups;
            for (const auto* con : cons) {
                auto attr = attributes(*con);
                if (aquId != id) {
                    const auto neighbour = faceNeighbour(grid, *con);
                    if (neighbour.has_value() && sourceCells.contains(*neighbour)) {
                        std::get<3>(attr) = true;
                    }
                }
                groups[attr].push_back(con->global_index);
            }
            std::unordered_set<std::size_t> boxed;
            for (const auto& [attr, cells] : groups) {
                const auto& [face, mult, option, allowInternal, veFrac, veFracP] = attr;
                for (const auto& box : boxCover.cover(cells, boxed)) {
                    this->aqucon_.push_back({aquId, box, face, mult, option,
                                             allowInternal, veFrac, veFracP});
                }
            }
            inAquconBox.insert(boxed.begin(), boxed.end());
        }
    }

    this->replacedAqunum_.assign(replacedAqunum.begin(), replacedAqunum.end());
    this->replacedAqucon_.assign(replacedAqucon.begin(), replacedAqucon.end());
}

bool NumericalAquiferRegionSplit::empty() const
{
    return this->aqunum_.empty() && this->aqucon_.empty() && this->eqlnum_.empty();
}

std::string NumericalAquiferRegionSplit::deckChanges(const UnitSystem& units) const
{
    using M = UnitSystem::measure;

    std::string out;
    if (this->empty()) {
        if (!this->unresolved_.empty()) {
            out += fmt::format("No free inactive cells outside local grid refinements were "
                               "found for new numerical aquifers to replace aquifer(s) {}.\n",
                               idList(this->unresolved_));
        }
        return out;
    }

    out += "The following deck changes give each numerical aquifer connections to a single "
           "equilibration region, with the aquifer cells in that region:\n";

    if (!this->aqunum_.empty()) {
        std::vector<std::size_t> newIds;
        for (const auto& rec : this->aqunum_) {
            if (rec.id != rec.sourceId &&
                std::ranges::find(newIds, rec.id) == newIds.end())
            {
                newIds.push_back(rec.id);
            }
        }

        out += "\n* GRID section:";
        if (!this->replacedAqunum_.empty()) {
            out += fmt::format(" replace the AQUNUM records of aquifer(s) {}", idList(this->replacedAqunum_));
        }
        if (!newIds.empty()) {
            out += fmt::format("{} add AQUNUM records for new aquifer(s) {}",
                               this->replacedAqunum_.empty() ? "" : " and", idList(newIds));
        }
        out += ":\n\nAQUNUM\n";

        std::size_t lastId = 0;
        bool anyPressure = false;
        for (const auto& rec : this->aqunum_) {
            if (rec.id != lastId) {
                out += (rec.id == rec.sourceId)
                    ? fmt::format("-- aquifer {}: cells in EQLNUM region {}\n", rec.id, rec.region)
                    : fmt::format("-- new aquifer {}: part of aquifer {} connected to EQLNUM region {}\n",
                                  rec.id, rec.sourceId, rec.region);
                lastId = rec.id;
            }
            anyPressure = anyPressure || rec.hadInitialPressure;
            out += fmt::format(" {} {} {} {} {:.10g} {:.10g} {:.10g} {:.10g} {:.10g} 1* {} {} /\n",
                               rec.id, rec.ijk[0] + 1, rec.ijk[1] + 1, rec.ijk[2] + 1,
                               units.from_si(M::area, rec.area),
                               units.from_si(M::length, rec.length),
                               rec.porosity,
                               units.from_si(M::permeability, rec.permeability),
                               units.from_si(M::length, rec.depth),
                               rec.pvtTable, rec.satTable);
        }
        out += "/\n";
        if (anyPressure) {
            out += "-- Initial aquifer pressures are defaulted, so that the aquifer cells are\n"
                   "-- equilibrated together with the reservoir cells they connect to.\n";
        }
    }

    if (!this->aqucon_.empty()) {
        out += fmt::format("\n* GRID section: replace all AQUCON records of aquifer(s) {} by:\n\nAQUCON\n",
                           idList(this->replacedAqucon_));
        for (const auto& rec : this->aqucon_) {
            out += fmt::format(" {} {} {} {} {} {} {} '{}' {:.10g} {} '{}' {:.10g} {:.10g} /\n",
                               rec.id,
                               rec.box[0] + 1, rec.box[1] + 1,
                               rec.box[2] + 1, rec.box[3] + 1,
                               rec.box[4] + 1, rec.box[5] + 1,
                               aquconFace(rec.face), rec.transMult, rec.transOption,
                               rec.allowInternalCells ? "YES" : "NO",
                               rec.veFrac, rec.veFracP);
        }
        out += "/\n";
    }

    if (!this->eqlnum_.empty()) {
        out += "\n* REGIONS section, after EQLNUM has been defined:\n\nEQUALS\n";
        for (const auto& rec : this->eqlnum_) {
            out += fmt::format(" 'EQLNUM' {} {} {} {} {} {} {} / -- cell of aquifer {}\n",
                               rec.region,
                               rec.ijk[0] + 1, rec.ijk[0] + 1,
                               rec.ijk[1] + 1, rec.ijk[1] + 1,
                               rec.ijk[2] + 1, rec.ijk[2] + 1,
                               rec.aquiferId);
        }
        out += "/\n";
    }

    if (!this->aqucon_.empty()) {
        out += "\nIncrease the numerical aquifer dimensions in AQUDIMS if needed.\n";
    }

    if (!this->unresolved_.empty()) {
        out += fmt::format("\nNo free inactive cells outside local grid refinements were "
                           "found for new numerical aquifers to replace aquifer(s) {}.\n",
                           idList(this->unresolved_));
    }

    return out;
}

} // namespace Opm
