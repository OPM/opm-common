/*
  Copyright (C) 2021 Equinor

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

#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <fmt/format.h>

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/E.hpp>

#include <src/opm/parser/eclipse/EclipseState/Grid/Carfin.hpp>


namespace Opm {

  Carfin::Carfin(const std::string& name, size_t i1, size_t i2, size_t j1, size_t j2, size_t k1, size_t k2, size_t nx, size_t ny, size_t nz) :
                    m_name(name), m_i1(i1), m_i2(i2), m_j1(j1), m_j2(j2), m_k1(k1), m_k2(k2), m_nx(nx), m_ny(ny), m_nz(nz)
  {
  }

  Carfin Carfin::serializeObject()
  {
        Carfin result;
        result.m_name = "test";
        result.m_i1 = 2;
        result.m_i2 = 3;
        result.m_j1 = 4;
        result.m_j2 = 5;
        result.m_k1 = 6;
        result.m_k2 = 7;
        result.m_nx = 8;
        result.m_ny = 9;
        result.m_nz = 10;

        return result;
  }

  Carfin::Carfin(const Deck& deck) {
    if (deck.hasKeyword("CARFIN") && deck.hasKeyword("ENDFIN")) 
        init(deck["CARFIN"].back());
  }

  std::string Carfin::getName() const{
      return m_name;
  }
  
  size_t Carfin::getI1() const{
      return m_i1;
  }
  
  size_t Carfin::getI2() const{
      return m_i2;
  }
  
  size_t Carfin::getJ1() const{
      return m_j1;
  }
  
  size_t Carfin::getJ2() const{
      return m_j2;
  }
  
  size_t Carfin::getK1() const{
      return m_k1;
  }
  
  size_t Carfin::getK2() const{
      return m_k2;
  }
  
  size_t Carfin::getNX() const{
      return m_nx;
  }
  
  size_t Carfin::getNY() const{
        return m_ny;
  }
  
  size_t Carfin::getNZ() const{
        return m_nz;
  }

  size_t Carfin::operator[](int dim) const {
        switch (dim) {
        case 1:
            return this->m_i1;
            break;
        case 2:
            return this->m_i2;
            break;
        case 3:
            return this->m_j1;
            break;
        case 4:
            return this->m_j2;
            break;
        case 5:
            return this->m_k1;
            break;
        case 6:
            return this->m_k2;
            break;
        case 7:
            return this->m_nx;
            break;
        case 8:
            return this->m_ny;
            break;
        case 9:
            return this->m_nz;
            break;
        default:
            throw std::invalid_argument("Invalid argument dim:" + std::to_string(dim));
        }
    }

    Carfin::Carfin() :
                    m_name(0), m_i1(0), m_i2(0), m_j1(0), m_j2(0), m_k1(0), m_k2(0), m_nx(0), m_ny(0), m_nz(0)
    {
    }

    inline std::array< int, 9 > readCarfin(const DeckKeyword& keyword) {
        const auto& record = keyword.getRecord(0);
        return { { record.getItem("I1").get<int>(0),
                   record.getItem("I2").get<int>(0),
                   record.getItem("J1").get<int>(0),
                   record.getItem("J2").get<int>(0),
                   record.getItem("K1").get<int>(0),
                   record.getItem("K2").get<int>(0),
                   record.getItem("NX").get<int>(0),
                   record.getItem("NY").get<int>(0),
                   record.getItem("NZ").get<int>(0) } };
    }

    void Carfin::init(const DeckKeyword& keyword) {
        auto dims = readCarfin(keyword);
        m_name = dims[0];
        m_i1 = dims[1];
        m_i2 = dims[2];
        m_j1 = dims[3];
        m_j2 = dims[4];
        m_k1 = dims[5];
        m_k2 = dims[6];
        m_nx = dims[7];
        m_ny = dims[8];
        m_nz = dims[9];
    }

}
