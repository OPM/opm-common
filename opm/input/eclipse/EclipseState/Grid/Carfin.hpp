/*
  Copyright 2022 Equinor
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

#ifndef CARFIN_HPP_
#define CARFIN_HPP_

#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>

#include <array>
#include <cstddef>
#include <functional>
#include <vector>
#include <string>

namespace Opm {
    class DeckRecord;
}

namespace Opm
{
    class Carfin
    {
    public:
        using IsActive = std::function<bool(const std::size_t globalIdx)>;
        using ActiveIdx = std::function<std::size_t(const std::size_t globalIdx)>;

        struct cell_index
        {
            std::size_t global_index;
            std::size_t active_index;
            std::size_t data_index;

            cell_index(std::size_t g,std::size_t a, std::size_t d)
                : global_index(g)
                , active_index(a)
                , data_index(d)
            {}

            cell_index(std::size_t g, std::size_t d)
                : global_index(g)
                , active_index(g)
                , data_index(d)
            {}
        };

        Carfin() = default;

        explicit Carfin(const GridDims& gridDims,
                        IsActive        isActive,
                        ActiveIdx       activeIdx);

        Carfin(const GridDims& gridDims,
               IsActive        isActive,
               ActiveIdx       activeIdx,
               const std::string& name,
               int i1, int i2,
               int j1, int j2,
               int k1, int k2,
               int nx, int ny,
               int nz);

        void update(const DeckRecord& deckRecord);
        void reset();

        bool isGlobal() const;
        std::size_t size() const;
        std::size_t getDim(std::size_t idim) const;

        const std::vector<cell_index>& index_list() const;
        const std::vector<cell_index>& global_index_list() const;

        bool operator==(const Carfin& other) const;
        bool equal(const Carfin& other) const;

        const std::string& NAME() const;
        const std::string& PARENT_NAME() const;
        int I1() const;
        int I2() const;
        int J1() const;
        int J2() const;
        int K1() const;
        int K2() const;
        int NX() const;
        int NY() const;
        int NZ() const;
        std::size_t num_parent_cells() const;
         
        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_dims);    
            serializer(m_offset);     
            serializer(m_end_offset);     
            serializer(name_grid);           
        }

    private:
        GridDims m_globalGridDims_{};
        IsActive m_globalIsActive_{};
        ActiveIdx m_globalActiveIdx_{};

        std::array<std::size_t, 3> m_dims{};
        std::array<std::size_t, 3> m_offset{};
        std::array<std::size_t, 3> m_end_offset{};
        std::string name_grid;
        std::string parent_name_grid;

        std::vector<cell_index> m_active_index_list;
        std::vector<cell_index> m_global_index_list;

        void init(const std::string& name,
                  int i1, int i2,
                  int j1, int j2,
                  int k1, int k2,
                  int nx, int ny, int nz, const std::string& parent_name = "GLOBAL");
        void initIndexList();
        int lower(int dim) const;
        int upper(int dim) const;
        int dimension(int dim) const;
    };
}


#endif
