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

#include <stdexcept>
#include <utility>

#include <fmt/format.h>

namespace {

    void assert_dims(std::string name, int l1 , int l2, int nlgr, int nglobal) {

        if ((l1 < 0) || (l2 < 0) || (l1 > l2))
            throw std::invalid_argument(name + ": Invalid index values for lgr");

        if (l2 > nglobal)
            throw std::invalid_argument(name + ": Index values for lgr greater than global grid size");

        if (nlgr % (l2-l1+1) != 0)
            throw std::invalid_argument(name + ": Number of divisions in CARFIN is not a multiple of the number of gridblocks to be refined (I2-I1+1).");
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

    Carfin::Carfin(const GridDims& gridDims,
             IsActive        isActive,
             ActiveIdx       activeIdx,
            const std::string name, const int i1, const int i2,
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

    void Carfin::update(const DeckRecord& deckRecord)
    {

        auto default_count = 0;

        std::string name = "LGR";
        default_count += update_default_name(deckRecord.getItem<ParserKeywords::CARFIN::NAME>(), name);      


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

        if (default_count != 10) {
            this->init(name, i1, i2, j1, j2, k1, k2, nx, ny, nz);
        }
    }

    void Carfin::reset()
    {
        this->init("LGR", 0, this->m_globalGridDims_.getNX() - 1,
                   0, this->m_globalGridDims_.getNY() - 1,
                   0, this->m_globalGridDims_.getNZ() - 1,
                   this->m_globalGridDims_.getNX(), this->m_globalGridDims_.getNY(), this->m_globalGridDims_.getNZ());
    }

    void Carfin::init(const std::string name, const int i1, const int i2,
                   const int j1, const int j2,
                   const int k1, const int k2,
                   const int nx, const int ny,
                   const int nz)
    {
        assert_dims(name, i1 , i2, nx, this->m_globalGridDims_.getNX());
        assert_dims(name, j1 , j2, ny, this->m_globalGridDims_.getNY());
        assert_dims(name, k1 , k2, nz, this->m_globalGridDims_.getNZ());

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

        auto binSize = std::array<std::size_t, 3>{};
        for (auto i = 0*binSize.size(); i < binSize.size(); ++i) {
            binSize[i] = this->m_dims[i] / (this->m_end_offset[i] - this->m_offset[i] + 1);
        }

        for (auto data_index = 0*ncells; data_index != ncells; ++data_index) {
            const auto lgrIJK = lgrdims.getIJK(data_index);
            const auto global_index = this->m_globalGridDims_
                .getGlobalIndex(this->m_offset[0] + (lgrIJK[0] / binSize[0]),
                        this->m_offset[1] + (lgrIJK[1] / binSize[1]),
                        this->m_offset[2] + (lgrIJK[2] / binSize[2]));        

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
            && (this->m_end_offset == other.m_end_offset);
    }

    bool Carfin::equal(const Carfin& other) const
    {
        return *this == other;
    }

    int Carfin::lower(int dim) const {
        return m_offset[dim];
    }

    int Carfin::upper(int dim) const {
        return m_end_offset[dim];
    }

    int Carfin::dimension(int dim) const {
        return m_dims[dim];
    }

 //   std::string Carfin::NAME() const {
 //       return ?;
 //   }

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
