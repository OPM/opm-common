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
#include <optional>
#include <unordered_map>

#include <opm/parser/eclipse/EclipseState/Grid/GridDims.hpp>

namespace Opm {

class CompletedCells {
public:

    struct Cell {
        std::size_t global_index;
        std::size_t i, j, k;

        struct Props{
            std::size_t active_index;
            double permx;
            double permy;
            double permz;
            int satnum;
            int pvtnum;

            bool operator==(const Props& other) const{
                return this->active_index == other.active_index &&
                       this->permx == other.permx &&
                       this->permy == other.permy &&
                       this->permz == other.permz &&
                       this->satnum == other.satnum &&
                       this->pvtnum == other.pvtnum;
            }

        };

        std::optional<Props> props;
        std::size_t active_index() const;
        bool is_active() const;

        double depth;
        std::array<double, 3> dimensions;

        bool operator==(const Cell& other) const {
            return this->global_index == other.global_index &&
                   this->i == other.i &&
                   this->j == other.j &&
                   this->k == other.k &&
                   this->depth == other.depth &&
                   this->dimensions == other.dimensions && 
                   this->props == other.props;
        }

        static Cell serializeObject() {
            Cell cell(0,1,1,1);
            cell.depth = 12345;
            return cell;
        }

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->global_index);
            serializer(this->i);
            serializer(this->j);
            serializer(this->k);
            serializer(this->depth);
            serializer(this->props);
            serializer.template array<std::array<double,3>, false>(this->dimensions);
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
    CompletedCells(const GridDims& dims);
    CompletedCells(std::size_t nx, std::size_t ny, std::size_t nz);
    const Cell& get(std::size_t i, std::size_t j, std::size_t k) const;
    std::pair<bool, Cell&> try_get(std::size_t i, std::size_t j, std::size_t k);

    bool operator==(const CompletedCells& other) const;
    static CompletedCells serializeObject();

    template<class Serializer>
    void serializeOp(Serializer& serializer)
    {
        this->dims.serializeOp(serializer);
        serializer.map(this->cells);
    }

private:
    GridDims dims;
    std::unordered_map<std::size_t, Cell> cells;
};
}
#endif

