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

#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/SingleNumericalAquifer.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquiferCell.hpp>
#include <opm/input/eclipse/EclipseState/Aquifer/NumericalAquifer/NumericalAquiferConnection.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/input/eclipse/EclipseState/Grid/NNC.hpp>

#include "../AquiferHelpers.hpp"

#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>

#include <fmt/format.h>

namespace Opm {
    SingleNumericalAquifer::SingleNumericalAquifer(const std::size_t aqu_id)
            : id_(aqu_id)
    {
    }

    void SingleNumericalAquifer::addAquiferCell(const NumericalAquiferCell& aqu_cell) {
        cells_.push_back(aqu_cell);
    }

    std::size_t SingleNumericalAquifer::numCells() const {
        return this->cells_.size();
    }

    const NumericalAquiferCell* SingleNumericalAquifer::getCellPrt(const std::size_t index) const {
        return &this->cells_[index];
    }

    void SingleNumericalAquifer::addAquiferConnection(const NumericalAquiferConnection& aqu_con) {
        this->connections_.push_back(aqu_con);
    }

    bool SingleNumericalAquifer::operator==(const SingleNumericalAquifer& other) const {
        return this->cells_ == other.cells_ &&
                this->connections_ == other.connections_ &&
                this->id_ == other.id_;
    }

    std::size_t SingleNumericalAquifer::numConnections() const {
        return this->connections_.size();
    }

    std::size_t SingleNumericalAquifer::id() const {
        return this->id_;
    }

    void SingleNumericalAquifer::applyMinPV(const EclipseGrid& grid) {
        constexpr auto DEFAULT_MINPV = 1.0e-6;
        const auto& minpv_vector = grid.getMinpvVector();
        const bool minpv_active = (grid.getMinpvMode() != MinpvMode::Inactive);
        for (auto& cell : this->cells_) {
            const double minpv = minpv_active ? minpv_vector[cell.global_index] : DEFAULT_MINPV;
            if (cell.poreVolume() < minpv) {
                cell.porosity = minpv/cell.cellVolume(); // Cells with bulk volume < epsilon are not added, so division is OK.
                const auto[I, J, K] = grid.getIJK(cell.global_index);
                OpmLog::warning(fmt::format("Pore volume in numerical aquifer {} cell ({}, {}, {}) "
                                            "below threshold - reset to MINPV (~ {:.5e} by adjusting "
                                            "PORO to {:.5e})",
                                    this->id_, I + 1, J + 1, K + 1, minpv, cell.porosity));
            }
        }
    }

    std::unordered_map<std::size_t, AquiferCellProps> SingleNumericalAquifer::aquiferCellProps() const {
        std::unordered_map<std::size_t, AquiferCellProps> aqucellprops;
        for (const auto& cell : this->cells_) {
            aqucellprops.emplace(std::make_pair(cell.global_index,
                               AquiferCellProps{cell.cellVolume(), cell.poreVolume(), cell.depth,
                                                cell.porosity, cell.sattable, cell.pvttable}));
        }
        return aqucellprops;
    }

    std::vector<NNCdata> SingleNumericalAquifer::aquiferCellNNCs() const {
        std::vector<NNCdata> nncs;
        if (this->cells_.empty())
            return nncs;

        // aquifer cells are connected to each other through NNCs to form the aquifer
        for (std::size_t i = 0; i < this->cells_.size() - 1; ++i) {
            const double trans1 = this->cells_[i].transmissiblity();
            const double trans2 = this->cells_[i + 1].transmissiblity();
            const double tran = 1. / (1. / trans1 + 1. / trans2);
            const std::size_t gc1 = this->cells_[i].global_index;
            const std::size_t gc2 = this->cells_[i + 1].global_index;
            if (gc1 < gc2) {
                nncs.emplace_back(gc1, gc2, tran);
            } else {
                nncs.emplace_back(gc2, gc1, tran);
            }
        }
        return nncs;
    }

    std::vector<NNCdata>
    SingleNumericalAquifer::aquiferConnectionNNCs(const EclipseGrid& grid, const FieldPropsManager& fp) const {
       std::vector<NNCdata> nncs;
        if (this->cells_.empty())
            return nncs;
        // aquifer connections are connected to aquifer cells through NNCs
        const std::vector<double>& ntg = fp.get_double("NTG");
        const auto& cell1 = this->cells_[0];
        // all the connections connect to the first numerical aquifer cell
        const std::size_t gc1 = cell1.global_index;
        for (const auto& con : this->connections_) {
            const std::size_t gc2 = con.global_index;
            // TODO: the following is based on Cartesian grids, it turns out working for more general grids.
            //  We should keep in mind this can be something causing problems for specific grids
            const auto& cell_dims = grid.getCellDims(gc2);
            double face_area = 0;
            std::string perm_string;
            std::string mult_string;
            double d = 0.;
            if (con.face_dir == FaceDir::XPlus || con.face_dir == FaceDir::XMinus) {
                face_area = cell_dims[1] * cell_dims[2];
                perm_string = "PERMX";
                d = cell_dims[0];
                mult_string = "MULTX";
                if (con.face_dir == FaceDir::XMinus)
                    mult_string = mult_string + "-";
            }
            if (con.face_dir == FaceDir::YMinus || con.face_dir == FaceDir::YPlus) {
                face_area = cell_dims[0] * cell_dims[2];
                perm_string = "PERMY";
                d = cell_dims[1];
                mult_string = "MULTY";
                if (con.face_dir == FaceDir::YMinus)
                    mult_string = mult_string + "-";
            }

            if (con.face_dir == FaceDir::ZMinus || con.face_dir == FaceDir::ZPlus) {
                face_area = cell_dims[0] * cell_dims[1];
                perm_string = "PERMZ";
                d = cell_dims[2];
                mult_string = "MULTZ";
                if (con.face_dir == FaceDir::ZMinus)
                    mult_string = mult_string + "-";
            }

            const double trans_cell = (con.trans_option == 0) ?
                                      cell1.transmissiblity() : (2 * cell1.permeability * face_area / cell1.length);

            const double cell_perm = (fp.get_double(perm_string))[grid.activeIndex(gc2)];
            double cell_multxyz = 1.0;
            if (fp.has_double(mult_string))
                cell_multxyz = (fp.get_double(mult_string))[grid.activeIndex(gc2)];

            const double trans_con = 2 * cell_multxyz * cell_perm * face_area * ntg[grid.activeIndex(con.global_index)] / d;

            const double tran = trans_con * trans_cell / (trans_con + trans_cell) * con.trans_multipler;
            if (gc1 < gc2) {
                nncs.emplace_back(gc1, gc2, tran);
            } else {
                nncs.emplace_back(gc2, gc1, tran);
            }
        }
        return nncs;
    }

    const std::vector<NumericalAquiferConnection>& SingleNumericalAquifer::connections() const {
        return this->connections_;
    }

    void SingleNumericalAquifer::postProcessConnections(const EclipseGrid& grid, const std::vector<int>& actnum) {
        std::unordered_set<std::size_t> cell_global_indices;
        for (const auto& cell : this->cells_) {
            cell_global_indices.insert(cell.global_index);
        }

        std::vector<NumericalAquiferConnection> conns;
        std::size_t numOutOfBounds = 0, numInactive = 0, numAdjoining = 0;
        const auto numRequested = this->connections_.size();
        for (const auto& con : this->connections_) {
            const std::size_t i = con.I;
            const std::size_t j = con.J;
            const std::size_t k = con.K;
            // Need to check for out-of-bounds access (eventually gives throw in aquiferConnectionNNCs also).
            if (! (i < grid.getNX() && j < grid.getNY() && k < grid.getNZ()) ) {
                OpmLog::warning(
                    fmt::format(
                        "Connection in numerical aquifer {} has out-of-bounds IJK ({}, {}, {}), allowed range is (1-{}, 1-{}, 1-{}). Connection skipped.",
                                            this->id_, i+1, j+1, k+1, grid.getNX(), grid.getNY(), grid.getNZ()));
                ++numOutOfBounds;
                continue;
            }
            if (!actnum[grid.getGlobalIndex(i, j, k)]) {
                // Silently dropped until now, and indistinguishable in the
                // result from a connection that was never asked for.
                ++numInactive;
                continue;
            }
            if (con.connect_active_cell
               || !AquiferHelpers::neighborCellInsideReservoirAndActive(grid, i, j, k, con.face_dir, actnum, cell_global_indices)) {
                conns.push_back(con);
            } else {
                OpmLog::warning(
                    fmt::format(
                        "Connection in numerical aquifer {} with IJK ({}, {}, {}) adjoins an active cell. Connection "
                        "skipped!", this->id_, i+1, j+1, k+1
                    )
                );
                ++numAdjoining;
            }
        }

        // The per-connection warnings above are message-limited, so on a field
        // deck they say neither how many connections went nor how many are
        // left.  An aquifer that keeps none of them is simply absent from the
        // run: it holds its pore volume and never exchanges with the reservoir,
        // which looks like a model that will not maintain pressure rather than
        // like a deck problem.
        if (conns.size() < numRequested) {
            auto why = std::vector<std::string>{};
            if (numAdjoining > 0) {
                why.push_back(fmt::format("{} adjoin an active cell (AQUCON item 11, "
                                          "ALLOW_INTERNAL_CELLS, permits those)", numAdjoining));
            }
            if (numInactive > 0) {
                why.push_back(fmt::format("{} name an inactive cell", numInactive));
            }
            if (numOutOfBounds > 0) {
                why.push_back(fmt::format("{} are outside the grid", numOutOfBounds));
            }

            auto reasons = std::string{};
            for (std::size_t n = 0; n < why.size(); ++n) {
                reasons += (n == 0) ? "" : (n + 1 == why.size() ? " and " : ", ");
                reasons += why[n];
            }

            OpmLog::warning(fmt::format(
                "Numerical aquifer {}: {} of {} AQUCON connection(s) retained -- {}.{}",
                this->id_, conns.size(), numRequested, reasons,
                conns.empty()
                ? " The aquifer has no connection to the reservoir left and will not"
                  " exchange fluid with it."
                : ""));
        }

        this->connections_ = std::move(conns);
    }
}
