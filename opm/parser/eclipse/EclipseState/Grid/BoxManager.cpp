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

#include <opm/parser/eclipse/EclipseState/Grid/BoxManager.hpp>


namespace Opm {

    BoxManager::BoxManager(int nx , int ny , int nz) :
        m_globalBox( nx, ny, nz )
    {}

    const Box& BoxManager::getGlobalBox() const {
        return m_globalBox;
    }


    const Box& BoxManager::getInputBox() const {
        return m_inputBox;
    }


    const Box& BoxManager::getKeywordBox() const {
        return m_keywordBox;
    }

    const Box& BoxManager::getActiveBox() const {
        if (m_keywordBox)
            return m_keywordBox;

        if (m_inputBox)
            return m_inputBox;

        return m_globalBox;
    }


    void BoxManager::setInputBox( int i1,int i2 , int j1 , int j2 , int k1 , int k2) {
        this->m_inputBox = Box( this->m_globalBox, i1, i2, j1, j2, k1, k2 );
    }

    void BoxManager::endInputBox() {
        if(m_keywordBox)
            throw std::invalid_argument("Hmmm - this seems like an internal error - the SECTION is terminated with an active keyword box");

        m_inputBox = Box{};
    }

    void BoxManager::endSection() {
        endInputBox();
    }

    void BoxManager::setKeywordBox( int i1,int i2 , int j1 , int j2 , int k1 , int k2) {
        this->m_keywordBox = Box( this->m_globalBox, i1, i2, j1, j2, k1, k2 );
    }

    void BoxManager::endKeyword() {
        this->m_keywordBox = Box{};
    }

}
