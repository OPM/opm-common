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
#include <optional>
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

        /// How one direction of the box is subdivided.
        ///
        /// `counts` is NXFIN/NYFIN/NZFIN: how many refined columns each parent
        /// cell is split into, one entry per parent cell. `widths` is
        /// HXFIN/HYFIN/HZFIN: the relative width of each refined column, one
        /// entry per refined column, normalised within each parent cell.
        /// Either may be empty, meaning an equal split.
        struct AxisGrading
        {
            std::vector<int> counts{};
            std::vector<double> widths{};

            bool operator==(const AxisGrading& other) const = default;

            template<class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(counts);
                serializer(widths);
            }
        };

        static Carfin serializationTestObject();

        void update(const DeckRecord& deckRecord);
        void reset();

        /// Attach an N*FIN / H*FIN pair for one direction. Throws if the
        /// counts do not add up to that direction's number of refined columns,
        /// or the widths do not number one per refined column.
        void setGrading(std::size_t dim,
                        std::vector<int> counts,
                        std::vector<double> widths);

        /// Whether any direction was given an explicit subdivision.
        bool isGraded() const;

        /// The block's own MINPV, if it set one. A graded block normally does:
        /// its finest cells are far below the field's threshold, and inheriting
        /// that threshold would delete the cells the refinement exists to make.
        void setMinpv(double minpv);
        const std::optional<double>& MINPV() const;

        /// Refined cells the block's MINPV removes, one entry per refined
        /// Cartesian cell (1 = removed). Derived by EclipseState once the
        /// refined geometry exists, and carried here so it reaches the grid
        /// builder -- on every rank, since the collection is broadcast.
        void setMinpvRemoved(std::vector<int> removed);
        const std::vector<int>& minpvRemoved() const;

        const std::array<AxisGrading, 3>& grading() const;

        /// Reject a box whose refined columns cannot be distributed over its
        /// parent cells: without N*FIN each parent cell takes the same number,
        /// so the count must divide.
        void validateSubdivision() const;

        /// Where each refined column of one direction sits: the parent cell it
        /// lies in (0-based within the box) and its normalised extent within
        /// that cell. One entry per refined column. This is the whole meaning
        /// of N*FIN/H*FIN, and the only place it is expressed.
        struct RefinedColumns
        {
            std::vector<int> parentOffset{};    ///< per refined column
            std::vector<double> fracLo{};       ///< per refined column
            std::vector<double> fracHi{};       ///< per refined column
            std::vector<int> firstColumn{};     ///< per parent cell
            std::vector<int> count{};           ///< per parent cell

            template<class Serializer>
            void serializeOp(Serializer& serializer)
            {
                serializer(parentOffset);
                serializer(fracLo);
                serializer(fracHi);
                serializer(firstColumn);
                serializer(count);
            }
        };

        RefinedColumns refinedColumns(std::size_t dim) const;

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
            serializer(parent_name_grid);
            serializer(m_grading);
            serializer(m_minpv);
            serializer(m_minpv_removed);
        }

    private:
        std::array<AxisGrading, 3> m_grading{};
        std::optional<double> m_minpv{};
        std::vector<int> m_minpv_removed{};

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
        std::size_t lower(int dim) const;
        std::size_t upper(int dim) const;
        std::size_t dimension(int dim) const;
    };
}


#endif
