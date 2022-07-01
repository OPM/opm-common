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

#include <opm/input/eclipse/EclipseState/Grid/BoxManager.hpp>


namespace Opm {

    BoxManager::BoxManager(const GridDims& gridDims,
                           Box::IsActive   isActive,
                           Box::ActiveIdx  activeIdx)
        : gridDims_  (gridDims)
        , isActive_  (isActive)
        , activeIdx_ (activeIdx)
        , m_globalBox(std::make_unique<Box>(gridDims_, isActive_, activeIdx_))
    {}

    const Box& BoxManager::getActiveBox() const {
        if (m_keywordBox)
            return *m_keywordBox;

        if (m_inputBox)
            return *m_inputBox;

        return *m_globalBox;
    }

    void BoxManager::setInputBox(const int i1, const int i2,
                                 const int j1, const int j2,
                                 const int k1, const int k2)
    {
        this->m_inputBox = this->makeBox(i1, i2, j1, j2, k1, k2);
    }

    void BoxManager::endInputBox()
    {
        if (this->m_keywordBox != nullptr) {
            throw std::invalid_argument {
                "Hmmm - this seems like an internal error - "
                "the SECTION is terminated with an active keyword box"
            };
        }

        this->m_inputBox.reset();
    }

    void BoxManager::endSection()
    {
        this->endInputBox();
    }

    void BoxManager::setKeywordBox(const int i1, const int i2,
                                   const int j1, const int j2,
                                   const int k1, const int k2)
    {
        this->m_keywordBox = this->makeBox(i1, i2, j1, j2, k1, k2);
    }

    void BoxManager::endKeyword()
    {
        this->m_keywordBox.reset();
    }

    const std::vector<Box::cell_index>& BoxManager::index_list() const
    {
        return this->getActiveBox().index_list();
    }

    std::unique_ptr<Box>
    BoxManager::makeBox(const int i1, const int i2,
                        const int j1, const int j2,
                        const int k1, const int k2) const
    {
        return std::make_unique<Box>(this->gridDims_,
                                     this->isActive_,
                                     this->activeIdx_,
                                     i1, i2,
                                     j1, j2,
                                     k1, k2);
    }
}
