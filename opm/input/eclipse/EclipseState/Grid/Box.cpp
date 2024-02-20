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

#include <opm/input/eclipse/EclipseState/Grid/Box.hpp>
#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/B.hpp> // BOX

#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <stdexcept>
#include <utility>

#include <fmt/format.h>

namespace {

    void assert_dims(const int len, const int l1, const int l2)
    {
        if (len <= 0) {
            throw std::invalid_argument {
                "Box must have finite size in all directions"
            };
        }

        if ((l1 < 0) || (l2 < 0) || (l1 > l2)) {
            throw std::invalid_argument {
                "Invalid index values for sub box"
            };
        }

        if (l2 >= len) {
            throw std::invalid_argument {
                "Invalid index values for sub box"
            };
        }
    }

    bool update_default(const Opm::DeckItem& item,
                        int&                 value)
    {
        if (item.defaultApplied(0)) {
            return true;
        }

        value = item.get<int>(0) - 1;
        return false;
    }
}

namespace Opm
{
    Box::Box(const GridDims& gridDims,
             IsActive        isActive,
             ActiveIdx       activeIdx)
        : m_globalGridDims_ (gridDims)
        , m_globalIsActive_ (std::move(isActive))
        , m_globalActiveIdx_(std::move(activeIdx))
    {
        this->reset();
    }

    Box::Box(const GridDims& gridDims,
             IsActive        isActive,
             ActiveIdx       activeIdx,
             const int i1, const int i2,
             const int j1, const int j2,
             const int k1, const int k2)
        : m_globalGridDims_ (gridDims)
        , m_globalIsActive_ (std::move(isActive))
        , m_globalActiveIdx_(std::move(activeIdx))
    {
        this->init(i1, i2, j1, j2, k1, k2);
    }

    void Box::update(const DeckRecord& deckRecord)
    {
        auto default_count = 0;

        int i1 = 0;
        int i2 = this->m_globalGridDims_.getNX() - 1;
        default_count += update_default(deckRecord.getItem<ParserKeywords::BOX::I1>(), i1);
        default_count += update_default(deckRecord.getItem<ParserKeywords::BOX::I2>(), i2);

        int j1 = 0;
        int j2 = this->m_globalGridDims_.getNY() - 1;
        default_count += update_default(deckRecord.getItem<ParserKeywords::BOX::J1>(), j1);
        default_count += update_default(deckRecord.getItem<ParserKeywords::BOX::J2>(), j2);

        int k1 = 0;
        int k2 = this->m_globalGridDims_.getNZ() - 1;
        default_count += update_default(deckRecord.getItem<ParserKeywords::BOX::K1>(), k1);
        default_count += update_default(deckRecord.getItem<ParserKeywords::BOX::K2>(), k2);

        if (default_count != 6) {
            this->init(i1, i2, j1, j2, k1, k2);
        }
    }

    void Box::reset()
    {
        this->init(0, this->m_globalGridDims_.getNX() - 1,
                   0, this->m_globalGridDims_.getNY() - 1,
                   0, this->m_globalGridDims_.getNZ() - 1);
    }

    void Box::init(const int i1, const int i2,
                   const int j1, const int j2,
                   const int k1, const int k2)
    {
        assert_dims(this->m_globalGridDims_.getNX(), i1, i2);
        assert_dims(this->m_globalGridDims_.getNY(), j1, j2);
        assert_dims(this->m_globalGridDims_.getNZ(), k1, k2);

        this->m_dims[0] = static_cast<std::size_t>(i2 - i1 + 1);
        this->m_dims[1] = static_cast<std::size_t>(j2 - j1 + 1);
        this->m_dims[2] = static_cast<std::size_t>(k2 - k1 + 1);

        this->m_offset[0] = static_cast<std::size_t>(i1);
        this->m_offset[1] = static_cast<std::size_t>(j1);
        this->m_offset[2] = static_cast<std::size_t>(k1);

        this->initIndexList();
    }

    std::size_t Box::size() const
    {
        return m_dims[0] * m_dims[1] * m_dims[2];
    }

    bool Box::isGlobal() const
    {
        return this->size() == this->m_globalGridDims_.getCartesianSize();
    }

    std::size_t Box::getDim(std::size_t idim) const
    {
        if (idim >= 3) {
            throw std::invalid_argument("The input dimension value is invalid");
        }

        return m_dims[idim];
    }

    const std::vector<Box::cell_index>& Box::index_list() const {
        return this->m_active_index_list;
    }

    const std::vector<Box::cell_index>& Box::global_index_list() const {
        return this->m_global_index_list;
    }

    void Box::initIndexList()
    {
        this->m_active_index_list.clear();
        this->m_global_index_list.clear();

        const auto boxdims = GridDims(this->m_dims[0], this->m_dims[1], this->m_dims[2]);
        const auto ncells = boxdims.getCartesianSize();

        for (auto data_index = 0*ncells; data_index != ncells; ++data_index) {
            const auto boxIJK = boxdims.getIJK(data_index);
            const auto global_index = this->m_globalGridDims_
                .getGlobalIndex(boxIJK[0] + this->m_offset[0],
                                boxIJK[1] + this->m_offset[1],
                                boxIJK[2] + this->m_offset[2]);

            if (this->m_globalIsActive_(global_index)) {
                const auto active_index = this->m_globalActiveIdx_(global_index);
                this->m_active_index_list.emplace_back(global_index, active_index, data_index);
            }

            this->m_global_index_list.emplace_back(global_index, data_index);
        }
    }

    bool Box::operator==(const Box& other) const
    {
        return (this->m_dims == other.m_dims)
            && (this->m_offset == other.m_offset);
    }

    bool Box::equal(const Box& other) const
    {
        return *this == other;
    }

    int Box::lower(int dim) const {
        return m_offset[dim];
    }

    int Box::upper(int dim) const {
        return m_offset[dim] + m_dims[dim] - 1;
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
