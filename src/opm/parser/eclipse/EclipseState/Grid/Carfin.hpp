/*
  Copyright 2021 Equinor

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


#ifndef OPM_PARSER_CARFIN_HPP
#define OPM_PARSER_CARFIN_HPP

#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <string>

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>

namespace Opm {
class Deck;
class DeckKeyword;

/*
  The Carfin class internalizes the CARFIN keyword in the GRID section. In the deck 
  the CARFIN keyword comes together with a 'ENDFIN' kewyord and then a list of
  regular keywords in the between. Each Carfin/Endfin block defines one LGR. E.g.:

  CARFIN
  -- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
 'LGR1'  5  6  5  6  1  3  6  6  9 /
  ENDFIN

  CARFIN
  -- NAME I1-I2 J1-J2 K1-K2 NX NY NZ
 'LGR2'  7  8  7  8  1  3  6  6  9 /
  ENDFIN

*/

class Carfin 
{
  public:

        Carfin();
        Carfin(const std::string& name, size_t i1, size_t i2, size_t j1, size_t j2, size_t k1, size_t k2, size_t nx, size_t ny, size_t nz);
        Carfin(const Deck& deck);

        static Carfin serializeObject();

        std::string getName() const;
        size_t getI1() const;
        size_t getI2() const;
        size_t getJ1() const;
        size_t getJ2() const;
        size_t getK1() const;
        size_t getK2() const;
        size_t getNX() const;
        size_t getNY() const;
        size_t getNZ() const;
        size_t operator[](int dim) const;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_name);
            serializer(m_i1);
            serializer(m_i2);
            serializer(m_j1);
            serializer(m_j2);
            serializer(m_k1);
            serializer(m_k2);
            serializer(m_nx);
            serializer(m_ny);
            serializer(m_nz);
        }

    protected:
        std::string m_name;
        size_t m_i1;
        size_t m_i2;
        size_t m_j1;
        size_t m_j2;
        size_t m_k1;
        size_t m_k2;
        size_t m_nx;
        size_t m_ny;
        size_t m_nz;

    private:
        void init(const DeckKeyword& keyword);
    };
}

#endif
