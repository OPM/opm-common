/*
  Copyright 2016  Statoil ASA.

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

#ifndef OPM_PARSER_GRIDDIMS_HPP
#define OPM_PARSER_GRIDDIMS_HPP

#include <array>
#include <cstddef>

namespace Opm {
    class Deck;
    class DeckKeyword;

    class GridDims
    {
    public:
        GridDims();
        explicit GridDims(const std::array<int, 3>& xyz);
        GridDims(std::size_t nx, std::size_t ny, std::size_t nz);

        static GridDims serializationTestObject();

        explicit GridDims(const Deck& deck);

        std::size_t getNX() const;
        std::size_t getNY() const;
        std::size_t getNZ() const;
        std::size_t operator[](int dim) const;

        std::array<int, 3> getNXYZ() const;

        std::size_t getGlobalIndex(std::size_t i, std::size_t j, std::size_t k) const;

        std::array<int, 3> getIJK(std::size_t globalIndex) const;

        std::size_t getCartesianSize() const;

        void assertGlobalIndex(std::size_t globalIndex) const;

        void assertIJK(std::size_t i, std::size_t j, std::size_t k) const;

        bool operator==(const GridDims& data) const;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_nx);
            serializer(m_ny);
            serializer(m_nz);
        }

    protected:
        std::size_t m_nx;
        std::size_t m_ny;
        std::size_t m_nz;

    private:
        void init(const DeckKeyword& keyword);
        void binary_init(const Deck& deck);
    };
}

#endif /* OPM_PARSER_GRIDDIMS_HPP */
