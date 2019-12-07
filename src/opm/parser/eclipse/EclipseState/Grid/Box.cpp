/*
  Copyright 2014 Statoil ASA.

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

#include <stdexcept>

#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>

#include <opm/parser/eclipse/EclipseState/Grid/Box.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>

namespace {

    void assert_dims(int len,  int l1 , int l2) {
        if (len <= 0)
            throw std::invalid_argument("Box must have finite size in all directions");

        if ((l1 < 0) || (l2 < 0) || (l1 > l2))
            throw std::invalid_argument("Invalid index values for sub box");

        if (l2 >= len)
            throw std::invalid_argument("Invalid index values for sub box");
    }


    void update_default(int &value, std::size_t& default_count, const Opm::DeckItem& item) {
        if (item.defaultApplied(0))
            default_count += 1;
        else
            value = item.get<int>(0) - 1;
    }
}

namespace Opm {


    Box::Box(std::size_t nx, std::size_t ny, std::size_t nz, const std::vector<int>& actnum_arg) :
        actnum(actnum_arg),
        grid_dims({nx,ny,nz})
    {
        if (this->actnum.size() != nx*ny*nz)
            throw std::invalid_argument("Size mismatch: actnum.size(): " + std::to_string(this->actnum.size()) + " nx*ny*nz: " + std::to_string(nx*ny*nz));
    }

    Box::Box(const EclipseGrid& grid) :
        Box(grid.getNX(), grid.getNY(), grid.getNZ(), grid.getACTNUM())
    {
        this->reset();
    }


    Box::Box(const EclipseGrid& grid, int i1 , int i2 , int j1 , int j2 , int k1 , int k2) :
        Box(grid)
    {
        this->init(i1,i2,j1,j2,k1,k2);
    }


    void Box::update(const DeckRecord& deckRecord) {
        const auto& I1Item = deckRecord.getItem("I1");
        const auto& I2Item = deckRecord.getItem("I2");
        const auto& J1Item = deckRecord.getItem("J1");
        const auto& J2Item = deckRecord.getItem("J2");
        const auto& K1Item = deckRecord.getItem("K1");
        const auto& K2Item = deckRecord.getItem("K2");

        std::size_t default_count = 0;
        int i1 = 0;
        int i2 = this->grid_dims[0] - 1;
        int j1 = 0;
        int j2 = this->grid_dims[1] - 1;
        int k1 = 0;
        int k2 = this->grid_dims[2] - 1;

        update_default(i1, default_count, I1Item);
        update_default(i2, default_count, I2Item);
        update_default(j1, default_count, J1Item);
        update_default(j2, default_count, J2Item);
        update_default(k1, default_count, K1Item);
        update_default(k2, default_count, K2Item);

        if (default_count != 6)
            this->init(i1,i2,j1,j2,k1,k2);
    }


    void Box::reset()
    {
        this->init(0, this->grid_dims[0] , 0, this->grid_dims[1] - 1, 0, this->grid_dims[2] - 1);
    }


    void Box::init(int i1, int i2, int j1, int j2, int k1, int k2) {
        assert_dims(this->grid_dims[0], i1 , i2);
        assert_dims(this->grid_dims[1], j1 , j2);
        assert_dims(this->grid_dims[2], k1 , k2);
        box_stride[0] = 1;
        box_stride[1] = this->grid_dims[0];
        box_stride[2] = this->grid_dims[0] * this->grid_dims[1];

        box_dims[0] = (size_t) (i2 - i1 + 1);
        box_dims[1] = (size_t) (j2 - j1 + 1);
        box_dims[2] = (size_t) (k2 - k1 + 1);

        box_offset[0] = (size_t) i1;
        box_offset[1] = (size_t) j1;
        box_offset[2] = (size_t) k1;

        if (size() == this->grid_dims[0] * this->grid_dims[1] * this->grid_dims[2])
            m_isGlobal = true;
        else
            m_isGlobal = false;

        this->initIndexList();
    }



    size_t Box::size() const {
        return box_dims[0] * box_dims[1] * box_dims[2];
    }


    bool Box::isGlobal() const {
        return m_isGlobal;
    }


    size_t Box::getDim(size_t idim) const {
        if (idim >= 3)
            throw std::invalid_argument("The input dimension value is invalid");

        return box_dims[idim];
    }



    const std::vector<size_t>& Box::getIndexList() const {
        return global_index_list;
    }

    const std::vector<Box::cell_index>& Box::index_list() const {
        return m_index_list;
    }


    void Box::initIndexList() {
        global_index_list.clear();
        m_index_list.clear();

        std::size_t active_index = 0;
        for (std::size_t ik=0; ik < box_dims[2]; ik++) {
            size_t k = ik + box_offset[2];
            for (std::size_t ij=0; ij < box_dims[1]; ij++) {
                size_t j = ij + box_offset[1];
                for (std::size_t ii=0; ii < box_dims[0]; ii++) {
                    size_t i = ii + box_offset[0];
                    size_t g = i * box_stride[0] + j*box_stride[1] + k*box_stride[2];

                    global_index_list.push_back(g);
                    if (this->actnum[g]) {
                        std::size_t global_index = g;
                        std::size_t data_index = ii + ij*this->box_dims[0] + ik*this->box_dims[0]*this->box_dims[1];
                        m_index_list.push_back({global_index, active_index, data_index});

                        active_index++;
                    }
                }
            }
        }
    }

    bool Box::equal(const Box& other) const {

        if (size() != other.size())
            return false;

        {
            for (size_t idim = 0; idim < 3; idim++) {
                if (box_dims[idim] != other.box_dims[idim])
                    return false;

                if (box_stride[idim] != other.box_stride[idim])
                    return false;

                if (box_offset[idim] != other.box_offset[idim])
                    return false;
            }
        }

        return true;
    }


    int Box::lower(int dim) const {
        return box_offset[dim];
    }

    int Box::upper(int dim) const {
        return box_offset[dim] + box_dims[dim] - 1;
    }

    int Box::I1() const {
        return lower(0);
    }

    int Box::I2() const {
        return upper(0);
    }

    int Box::J1() const {
        return lower(1);
    }

    int Box::J2() const {
        return upper(1);
    }

    int Box::K1() const {
        return lower(2);
    }

    int Box::K2() const {
        return upper(2);
    }

}
