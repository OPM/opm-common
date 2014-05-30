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
#ifndef ECLIPSE_GRIDPROPERTY_HPP_
#define ECLIPSE_GRIDPROPERTY_HPP_

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/Box.hpp>

/*
  This class implemenents a class representing properties which are
  define over an ECLIPSE grid, i.e. with one value for each logical
  cartesian cell in the grid. 

  The class is implemented as a thin wrapper around std::vector<T>;
  where the most relevant specialisations of T are 'int' and 'float'.
*/




namespace Opm {

template <typename T>
class GridProperty {
public:

    GridProperty(size_t nx , size_t ny , size_t nz , const std::string& keyword , T defaultValue = 0) {
        m_nx = nx;
        m_ny = ny;
        m_nz = nz;
        m_keyword = keyword;
        m_data.resize( nx * ny * nz );
        std::fill( m_data.begin() , m_data.end() , defaultValue );
    }
    
    size_t size() const {
        return m_data.size();
    }

    
    T iget(size_t index) const {
        if (index < m_data.size()) {
            return m_data[index];
        } else {
            throw std::invalid_argument("Index out of range \n");
        }
    }


    T iget(size_t i , size_t j , size_t k) const {
        size_t g = i + j*m_nx + k*m_nx*m_ny;
        return iget(g);
    }


    const std::vector<T>& getData() {
        return m_data;
    }

    void loadFromDeckKeyword(std::shared_ptr<const Box> inputBox , DeckKeywordConstPtr deckKeyword);
    void loadFromDeckKeyword(DeckKeywordConstPtr deckKeyword);    

private:

    void setFromVector(const std::vector<T>& data) {
        if (data.size() == m_data.size()) {
            for (size_t i = 0; i < data.size(); i++) 
                m_data[i] = data[i];
        } else
            throw std::invalid_argument("Size mismatch");
    }
    
    
    void setFromVector(std::shared_ptr<const Box> inputBox , const std::vector<T>& data) {
        if (inputBox->isGlobal())
            setFromVector( data );
        else {
            const std::vector<size_t>& indexList = inputBox->getIndexList();
            if (data.size() == indexList.size()) {
                for (size_t i = 0; i < data.size(); i++) {
                    size_t targetIndex = indexList[i];
                    m_data[targetIndex] = data[i];
                }
            } else
                throw std::invalid_argument("Size mismatch");
        }
    }

    size_t      m_nx,m_ny,m_nz;
    std::string m_keyword;
    std::vector<T> m_data;
};

}
#endif
