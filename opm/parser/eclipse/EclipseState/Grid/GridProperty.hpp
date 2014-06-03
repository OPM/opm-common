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
#include <algorithm>
#include <unordered_map>
#include <boost/lexical_cast.hpp>

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
    typedef std::tuple</*name=*/std::string, /*dataType=*/T, /*unit=*/std::string> SupportedKeywordInfo;

    GridProperty(size_t nx , size_t ny , size_t nz , const SupportedKeywordInfo& kwInfo) {
        m_nx = nx;
        m_ny = ny;
        m_nz = nz;
        m_kwInfo = kwInfo;
        m_data.resize( nx * ny * nz );
        std::fill( m_data.begin() , m_data.end() ,  std::get<1>(m_kwInfo));
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


    
    void copyFrom(const GridProperty<T>& src, std::shared_ptr<const Box> inputBox) {
        if (inputBox->isGlobal()) {
            for (size_t i = 0; i < src.size(); ++i)
                m_data[i] = src.m_data[i];
        } else {
            const std::vector<size_t>& indexList = inputBox->getIndexList();
            for (size_t i = 0; i < indexList.size(); i++) {
                size_t targetIndex = indexList[i];
                m_data[targetIndex] = src.m_data[targetIndex];
            }
        }
    }
    
    void scale(T scaleFactor , std::shared_ptr<const Box> inputBox) {
        if (inputBox->isGlobal()) {
            for (size_t i = 0; i < m_data.size(); ++i)
                m_data[i] *= scaleFactor;
        } else {
            const std::vector<size_t>& indexList = inputBox->getIndexList();
            for (size_t i = 0; i < indexList.size(); i++) {
                size_t targetIndex = indexList[i];
                m_data[targetIndex] *= scaleFactor;
            }
        }
    }


    void add(T shiftValue , std::shared_ptr<const Box> inputBox) {
        if (inputBox->isGlobal()) {
            for (size_t i = 0; i < m_data.size(); ++i)
                m_data[i] += shiftValue;
        } else {
            const std::vector<size_t>& indexList = inputBox->getIndexList();
            for (size_t i = 0; i < indexList.size(); i++) {
                size_t targetIndex = indexList[i];
                m_data[targetIndex] += shiftValue;
            }
        }
    }


    

    void setScalar(T value , std::shared_ptr<const Box> inputBox) {
        if (inputBox->isGlobal()) {
            std::fill(m_data.begin(), m_data.end(), value);
        } else {
            const std::vector<size_t>& indexList = inputBox->getIndexList();
            for (size_t i = 0; i < indexList.size(); i++) {
                size_t targetIndex = indexList[i];
                m_data[targetIndex] = value;
            }
        }
    }
    

    const std::string& getKeywordName() const
    {
        return std::get<0>(m_kwInfo);
    }

    const std::string& getDimensionString() const
    {
        return std::get<2>(m_kwInfo);
    }

private:

    void setFromVector(const std::vector<T>& data) {
        if (data.size() == m_data.size()) {
            for (size_t i = 0; i < data.size(); i++) 
                m_data[i] = data[i];
        } else
            throw std::invalid_argument("Size mismatch when setting data for:" + getKeywordName() + " keyword size: " + boost::lexical_cast<std::string>(m_data.size()) + " input size: " + boost::lexical_cast<std::string>(data.size()));
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
                throw std::invalid_argument("Size mismatch when setting data for:" + getKeywordName() + " box size: " + boost::lexical_cast<std::string>(inputBox->size()) + " input size: " + boost::lexical_cast<std::string>(data.size()));
        }
    }

    size_t      m_nx,m_ny,m_nz;
    SupportedKeywordInfo m_kwInfo;
    std::vector<T> m_data;
};

}
#endif
