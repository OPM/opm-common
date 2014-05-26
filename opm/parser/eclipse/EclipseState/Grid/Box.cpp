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

#include <opm/parser/eclipse/EclipseState/Grid/Box.hpp>

namespace Opm {

    Box::Box(int nx , int ny , int nz) {
        if (nx <= 0)
            throw std::invalid_argument("The input nx value is invalid");

        if (ny <= 0)
            throw std::invalid_argument("The input ny value is invalid");

        if (nz <= 0)
            throw std::invalid_argument("The input nz value is invalid");

        m_dims[0] = (size_t) nx;
        m_dims[1] = (size_t) ny;
        m_dims[2] = (size_t) nz;

        m_offset = {0,0,0};

        m_stride[0] = 1;
        m_stride[1] = m_dims[0];
        m_stride[2] = m_dims[0] * m_dims[1];

        m_isGlobal = true;
    }

    
    size_t Box::size() const {
        return m_dims[0] * m_dims[1] * m_dims[2];
    }


    bool Box::isGlobal() const {
        return m_isGlobal;
    }


    size_t Box::getDim(size_t idim) const {
        if (idim >= 3)
            throw std::invalid_argument("The input dimension value is invalid");

        return m_dims[idim];
    }


    const std::vector<size_t>& Box::getIndexList() {
        assertIndexList();
        return m_indexList;
    }

    
    void Box::assertIndexList() {
        if (m_indexList.size() > 0) 
            return;

        {
            m_indexList.resize( size() );

            size_t ii,ij,ik;
            size_t l = 0;
            
            for (ik=0; ik < m_dims[2]; ik++) {
                size_t k = ik + m_offset[2];
                for (ij=0; ij < m_dims[1]; ij++) {
                    size_t j = ij + m_offset[1];
                    for (ii=0; ii < m_dims[0]; ii++) {
                        size_t i = ii + m_offset[0];
                        size_t g = i * m_stride[0] + j*m_stride[1] + k*m_stride[2];
                        
                        m_indexList[l] = g;
                        l++;
                    }
                }
            }
        }
    }



}

