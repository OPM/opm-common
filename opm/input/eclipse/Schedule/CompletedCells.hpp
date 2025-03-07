/*
  Copyright 2021 Equinor ASA.

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

#ifndef COMPLETED_CELLS
#define COMPLETED_CELLS

#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>

#include <array>
#include <cstddef>
#include <optional>
#include <unordered_map>
#include <utility>

namespace Opm {

class CompletedCells
{
public:
    struct Cell
    {
        std::size_t global_index{};
        std::size_t i{}, j{}, k{};

        struct Props
        {
            std::size_t active_index{};
            double permx{};
            double permy{};
            double permz{};
            double poro{};
            int satnum{};
            int pvtnum{};
            double ntg{};

            bool operator==(const Props& other) const;

            static Props serializationTestObject();

            template<class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(this->active_index);
                serializer(this->permx);
                serializer(this->permy);
                serializer(this->permz);
                serializer(this->poro);
                serializer(this->satnum);
                serializer(this->pvtnum);
                serializer(this->ntg);
            }
        };

        std::optional<Props> props{};
        std::size_t active_index() const;
        bool is_active() const;

        double depth{};
        std::array<double, 3> dimensions{};

        bool operator==(const Cell& other) const;

        static Cell serializationTestObject();

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->global_index);
            serializer(this->i);
            serializer(this->j);
            serializer(this->k);
            serializer(this->props);
            serializer(this->depth);
            serializer(this->dimensions);
        }

        Cell(std::size_t g, std::size_t i_, std::size_t j_, std::size_t k_)
            : global_index(g)
            , i(i_)
            , j(j_)
            , k(k_)
        {}

        Cell() = default;
    };

    CompletedCells() = default;
    ~CompletedCells() = default;
    explicit CompletedCells(const GridDims& dims);
    CompletedCells(std::size_t nx, std::size_t ny, std::size_t nz);

    const Cell& get(std::size_t i, std::size_t j, std::size_t k) const;
    std::pair<bool, Cell&> try_get(std::size_t i, std::size_t j, std::size_t k);

    bool operator==(const CompletedCells& other) const;
    static CompletedCells serializationTestObject();

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        serializer(this->dims);
        serializer(this->cells);
    }

private:
    GridDims dims;
    std::unordered_map<std::size_t, Cell> cells;
};
}

#endif  // COMPLETED_CELLS
