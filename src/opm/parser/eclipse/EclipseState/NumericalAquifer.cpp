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
#include <opm/parser/eclipse/EclipseState/Aqucon.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/FieldPropsManager.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/TranCalculator.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/NNC.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/common/OpmLog/OpmLog.hpp>

#include <iostream>

#include "AquiferHelpers.hpp"

namespace Opm {

NumericalAquifers::NumericalAquifers(const Deck& deck, const EclipseGrid& grid, const FieldPropsManager& field_props)
{
    using AQUNUM=ParserKeywords::AQUNUM;
    if ( !deck.hasKeyword<AQUNUM>() ) return;

    // there might be multiple keywords of keyword AQUNUM, it is not totally
    // clear about the rules here. For now, we take care of all the keywords
    const auto& aqunum_keywords = deck.getKeywordList<AQUNUM>();
    for (const auto& keyword : aqunum_keywords) {
        for (const auto& record : *keyword) {
            const NumericalAquiferCell aqu_cell(record, grid, field_props);
            // TODO: we should check whether the grid cell is active or NOT
            // NOT sure what to do if the cell is inactive
            if (this->hasCell(aqu_cell.global_index)) {
                std::ostringstream ss;
                ss << "NUMERICAL AQUIFER CELL AT GRID CELL { " << aqu_cell.I + 1
                   << " " << aqu_cell.J + 1 << " " << aqu_cell.K + 1 << " } IS DECLARED MORE THAN ONCE";
                throw std::logic_error(ss.str());
            } else {
                this->addAquiferCell(aqu_cell);
                auto& cells = this->aquifer_cells_;
                cells.insert(std::pair{aqu_cell.global_index, aqu_cell});
            }
        }
    }

    // handle connections
    this->addAquiferConnections(deck, grid);
}

bool NumericalAquifers::hasAquifer(const size_t aquifer_id) const {
    return (this->aquifers_.find(aquifer_id) != this->aquifers_.end());
}

void NumericalAquifers::addAquiferCell(const NumericalAquiferCell& aqu_cell) {
    const size_t id = aqu_cell.aquifer_id;
    if (!this->hasAquifer(id)) {
        this->aquifers_.insert(std::make_pair(id, SingleNumericalAquifer{id}));
    }

    this->aquifers_.at(id).addAquiferCell(aqu_cell);
}

void NumericalAquifers::
addAquiferConnections(const Deck &deck, const EclipseGrid &grid) {
    NumericalAquiferConnections cons(deck, grid);

    std::set<size_t> con_set;
    for (auto& pair : this->aquifers_) {
        const size_t aqu_id = pair.first;
        const auto& aqu_cons = cons.getConnections(aqu_id);

        // For now, there is no two aquifers can be connected to one cell
        // aquifer can not connect to aquifer cells
        auto& aquifer = pair.second;
        for (const auto& con : aqu_cons) {
            const auto& aqu_con = con.second;
            const size_t con_global_index = aqu_con.global_index;
            if (this->hasCell(con_global_index)) {
                const size_t cell_aquifer_id = this->getCell(con_global_index).aquifer_id;
                std::ostringstream ss;
                ss << "AQUIFER CONNECTION DECLARD AT GRID CELL { " << aqu_con.I + 1 << " " << aqu_con.J + 1 << " "
                    << aqu_con.K + 1 << " } IS A AQUIFER CELL OF AQUIFER " << cell_aquifer_id << ", AND WILL BE REMOVED";
                OpmLog::warning(ss.str());
                continue;
            }

            if (con_set.find(con_global_index) != con_set.end()) {
                // TODO: not totally sure whether a cell can be connected two different aquifers
                std::ostringstream ss;
                ss << "AQUIFER CONNECTION AT GRID CELL { " << aqu_con.I + 1 << " " << aqu_con.J + 1 << " "
                   << aqu_con.K + 1 << " } IS DECLARED MORE THAN ONCE";
                throw std::logic_error(ss.str());
            }
            aquifer.addAquiferConnection(con.second);
            con_set.insert(con_global_index);
        }
    }
}

bool NumericalAquifers::empty() const {
    return this->aquifers_.empty();
}

void NumericalAquifers::updateCellProps(const EclipseGrid& grid,
                                        std::vector<double>& pore_volume,
                                        std::vector<int>& satnum,
                                        std::vector<int>& pvtnum,
                                        std::vector<double>& cell_depth) const {
    for (const auto& iter : this->aquifers_) {
        iter.second.updateCellProps(grid, pore_volume, satnum, pvtnum, cell_depth);
    }
}

std::array<std::set<size_t>, 3>
NumericalAquifers::transToRemove(const EclipseGrid& grid) const {
    std::array<std::set<size_t>, 3> trans;
    for (const auto& pair : this->aquifers_) {
        auto trans_aquifer = pair.second.transToRemove(grid);
        for (size_t i = 0; i < 3; ++i) {
            trans[i].merge(trans_aquifer[i]);
        }
    }
    return  trans;
}

void NumericalAquifers::
appendNNC(const EclipseGrid &grid, const FieldPropsManager &fp, NNC &nnc) const {
    for (const auto& pair : this->aquifers_) {
        pair.second.appendNNC(grid, fp, nnc);
    }
}

const std::unordered_map<size_t, const NumericalAquiferCell>& NumericalAquifers::
aquiferCells() const {
    return this->aquifer_cells_;
}

bool NumericalAquifers::hasCell(const size_t cell_global_index) const {
    const auto& cells = this->aquifer_cells_;
    return (cells.find(cell_global_index) != cells.end());
}

const NumericalAquiferCell& NumericalAquifers::getCell(const size_t cell_global_index) const {
    assert(this->hasCell(cell_global_index));
    return this->aquifer_cells_.at(cell_global_index);
}

using AQUNUM = ParserKeywords::AQUNUM;
NumericalAquiferCell::NumericalAquiferCell(const DeckRecord& record, const EclipseGrid& grid, const FieldPropsManager& field_props)
   : aquifer_id( record.getItem<AQUNUM::AQUIFER_ID>().get<int>(0) )
   , I ( record.getItem<AQUNUM::I>().get<int>(0) - 1 )
   , J ( record.getItem<AQUNUM::J>().get<int>(0) - 1 )
   , K ( record.getItem<AQUNUM::K>().get<int>(0) - 1 )
   , area (record.getItem<AQUNUM::CROSS_SECTION>().getSIDouble(0) )
   , length ( record.getItem<AQUNUM::LENGTH>().getSIDouble(0) )
   , permeability( record.getItem<AQUNUM::PERM>().getSIDouble(0) )
{
    const auto& cell_depth = field_props.cellDepth();
    const auto& poro = field_props.get_double("PORO");
    const auto& pvtnum = field_props.get_int("PVTNUM");
    const auto& satnum = field_props.get_int("SATNUM");

    this->global_index = grid.getGlobalIndex(I, J, K);
    const size_t active_index = grid.activeIndex(this->global_index);

    if ( !record.getItem<AQUNUM::PORO>().defaultApplied(0) ) {
        this->porosity = record.getItem<AQUNUM::PORO>().getSIDouble(0);
    } else {
        this->porosity = poro[active_index];
    }

    if ( !record.getItem<AQUNUM::DEPTH>().defaultApplied(0) ) {
        this->depth = record.getItem<AQUNUM::DEPTH>().getSIDouble(0);
    } else {
        this->depth = cell_depth[active_index];
    }

    if ( !record.getItem<AQUNUM::INITIAL_PRESSURE>().defaultApplied(0) ) {
        this->init_pressure = record.getItem<AQUNUM::INITIAL_PRESSURE>().getSIDouble(0);
    } else {
        this->init_pressure = -1.e-300;
    }

    if ( !record.getItem<AQUNUM::PVT_TABLE_NUM>().defaultApplied(0) ) {
        this->pvttable = record.getItem<AQUNUM::PVT_TABLE_NUM>().get<int>(0);
    } else {
        this->pvttable = pvtnum[active_index];
    }

    if ( !record.getItem<AQUNUM::SAT_TABLE_NUM>().defaultApplied(0) ) {
        this->sattable = record.getItem<AQUNUM::SAT_TABLE_NUM>().get<int>(0);
    } else {
        this->sattable = satnum[active_index];
    }

    this->pore_volume = this->length * this->area * this->porosity;
    this->transmissibility = 2. * this->permeability * this->area / this->length;
}

void SingleNumericalAquifer::addAquiferCell(const NumericalAquiferCell& aqu_cell) {
    cells_.push_back(aqu_cell);
}

SingleNumericalAquifer::SingleNumericalAquifer(const size_t aqu_id)
: id_(aqu_id)
{
}

void SingleNumericalAquifer::addAquiferConnection(const NumAquiferCon& aqu_con) {
    this->connections_.push_back(aqu_con);
}

void SingleNumericalAquifer::updateCellProps(const EclipseGrid& grid,
                                             std::vector<double>& pore_volume,
                                             std::vector<int>& satnum,
                                             std::vector<int>& pvtnum,
                                             std::vector<double>& cell_depth) const {
    for (const auto& cell : this->cells_) {
        const size_t activel_index =  grid.activeIndex(cell.global_index);
        pore_volume[activel_index] = cell.pore_volume;
        satnum[activel_index] = cell.sattable;
        pvtnum[activel_index] = cell.pvttable;
        cell_depth[activel_index] = cell.depth;
    }
}

std::array<std::set<size_t>, 3> SingleNumericalAquifer::transToRemove(const EclipseGrid& grid) const {
    std::array<std::set<size_t>, 3> trans;
    for (const auto& cell : this->cells_) {
        const size_t i = cell.I;
        const size_t j = cell.J;
        const size_t k = cell.K;
        if (AquiferHelpers::neighborCellInsideReservoirAndActive(grid, i, j, k, FaceDir::XPlus)) {
            trans[0].insert(cell.global_index);
        }
        if (AquiferHelpers::neighborCellInsideReservoirAndActive(grid, i, j, k, FaceDir::XMinus)) {
            trans[0].insert(grid.getGlobalIndex(i-1, j, k));
        }
        if (AquiferHelpers::neighborCellInsideReservoirAndActive(grid, i, j, k, FaceDir::YPlus)) {
            trans[1].insert(cell.global_index);
        }
        if (AquiferHelpers::neighborCellInsideReservoirAndActive(grid, i, j, k, FaceDir::YMinus)) {
            trans[1].insert(grid.getGlobalIndex(i, j-1, k));
        }
        if (AquiferHelpers::neighborCellInsideReservoirAndActive(grid, i, j, k, FaceDir::ZPlus)) {
            trans[2].insert(cell.global_index);
        }
        if (AquiferHelpers::neighborCellInsideReservoirAndActive(grid, i, j, k, FaceDir::ZMinus)) {
            trans[2].insert(grid.getGlobalIndex(i, j, k-1));
        }
    }
    return trans;
}

void SingleNumericalAquifer::
appendNNC(const EclipseGrid &grid, const FieldPropsManager &fp, NNC &nnc) const {
    // adding the NNC between the numerical cells
    for (size_t i = 0; i < this->cells_.size() - 1; ++i) {
        const double trans1 = this->cells_[i].transmissibility;
        const double trans2 = this->cells_[i+1].transmissibility;
        const double tran = 1. / ( 1./trans1 + 1./trans2);
        const size_t gc1 = this->cells_[i].global_index;
        const size_t gc2 = this->cells_[i+1].global_index;
        nnc.addNNC(gc1, gc2, tran);
    }

    const std::vector<double>& ntg = fp.get_double("NTG");
    const auto& cell1 = this->cells_[0];
    // all the connections connect to the first numerical aquifer cell
    const size_t gc1 = cell1.global_index;
    for (const auto& con : this->connections_) {
        const size_t gc2 = con.global_index;
        // TODO: the following only works for Cartesian grids, more tests need to done
        // for more general grid
        const auto& cell_dims = grid.getCellDims(gc2);
        double face_area = 0;
        std::string perm_string;
        double d = 0.;
        if (con.face_dir == FaceDir::XPlus || con.face_dir == FaceDir::XMinus) {
            face_area = cell_dims[1] * cell_dims[2];
            perm_string = "PERMX";
            d = cell_dims[0];
        }
        if (con.face_dir == FaceDir::YMinus || con.face_dir == FaceDir::YPlus) {
            face_area = cell_dims[0] * cell_dims[2];
            perm_string = "PERMY";
            d = cell_dims[1];
        }

        if (con.face_dir == FaceDir::ZMinus || con.face_dir == FaceDir::ZPlus) {
            face_area = cell_dims[0] * cell_dims[1];
            perm_string = "PERMZ";
            d = cell_dims[2];
        }

        const double trans_cell = (con.trans_option == 0) ?
                              cell1.transmissibility : (2 * cell1.permeability * face_area / cell1.length);

        const auto& cell_perm = (fp.get_double(perm_string))[gc2];
        const double trans_con = 2 * cell_perm * face_area * ntg[con.global_index] / d;

        const double tran = trans_con * trans_cell / (trans_con + trans_cell) * con.trans_multipler;
        nnc.addNNC(gc1, gc2, tran);
    }
}

size_t SingleNumericalAquifer::numCells() const {
    return this->cells_.size();
}

}