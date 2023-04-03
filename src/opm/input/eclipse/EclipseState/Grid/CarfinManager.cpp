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

#include <stdexcept>

#include <opm/input/eclipse/EclipseState/Grid/CarfinManager.hpp>

namespace Opm {

    CarfinManager::CarfinManager(const GridDims& gridDims,
                           Carfin::IsActive   isActive,
                           Carfin::ActiveIdx  activeIdx)
        : gridDims_  (gridDims)
        , isActive_  (isActive)
        , activeIdx_ (activeIdx)
        , m_globalCarfin(std::make_unique<Carfin>(gridDims_, isActive_, activeIdx_))
    {}

    const Carfin& CarfinManager::getActiveCarfin() const {
        if (m_keywordCarfin)
            return *m_keywordCarfin;

        if (m_inputCarfin)
            return *m_inputCarfin;

        return *m_globalCarfin;
    }

    void CarfinManager::setInputCarfin(std::string name, int i1, int i2, 
                                        int j1, int j2, 
                                        int k1, int k2, 
                                        int nx , int ny , int nz)
    {
        this->m_inputCarfin = this->makeLgr(name, i1, i2, j1, j2, k1, k2, nx, ny, nz);
    }

    void CarfinManager::endInputCarfin()
    {
        if (this->m_keywordCarfin != nullptr) {
            throw std::invalid_argument {
                "the SECTION is terminated with an active keyword Carfin"
            };
        }

        this->m_inputCarfin.reset();
    }

    void CarfinManager::endSection()
    {
        this->endInputCarfin();
    }

    void CarfinManager::readKeywordCarfin(std::string name, int i1, int i2, 
                                          int j1, int j2, 
                                          int k1, int k2, 
                                          int nx , int ny , int nz)
    {
        this->m_keywordCarfin = this->makeLgr(name, i1, i2, j1, j2, k1, k2, nx, ny, nz);
    }

    void CarfinManager::endKeyword()
    {
        this->m_keywordCarfin.reset();
    }

    const std::vector<Carfin::cell_index>& CarfinManager::index_list() const
    {
        return this->getActiveCarfin().index_list();
    }

    std::unique_ptr<Carfin>
    CarfinManager::makeLgr(std::string name, int i1, int i2, 
                            int j1, int j2, 
                            int k1, int k2, 
                            int nx , int ny , int nz) const
    {
        return std::make_unique<Carfin>(this->gridDims_,
                                     this->isActive_,
                                     this->activeIdx_,
                                     name, i1, i2,
                                     j1, j2,
                                     k1, k2,
                                     nx, ny, nz);
    }
}
