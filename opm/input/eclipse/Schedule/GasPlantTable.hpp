/*
  Copyright 2026 Equinor ASA.

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

#ifndef OPM_GAS_PLANT_TABLE_HPP
#define OPM_GAS_PLANT_TABLE_HPP

#include <opm/common/OpmLog/KeywordLocation.hpp>

#include <cassert>
#include <cstddef>
#include <unordered_set>
#include <vector>

namespace Opm {

    class DeckRecord;

    /// A single GPTABLE gas-plant recovery table.
    ///
    /// One table per GPTABLE record: a gas plant table number, the lower/upper
    /// heavy-component indices, and the recovery data whose every row holds one
    /// heavy-component mole fraction followed by one recovery fraction per
    /// component (numComponents + 1 dimensionless values per row). Parse and
    /// represent only -- no gas-plant recovery physics.
    class GasPlantTable
    {
    public:
        GasPlantTable() = default;

        /// Build one table from a single GPTABLE record. \p numComponents is the
        /// component count (from COMPS); a defaulted lower/upper component index
        /// resolves to it.
        GasPlantTable(const DeckRecord& record,
                      std::size_t numComponents,
                      const KeywordLocation& location);

        static GasPlantTable serializationTestObject();

        /// Emit the GPTABLE table-number diagnostics shared by the SCHEDULE
        /// handler and the SOLUTION-section seeding. \p seenTableNumbers tracks
        /// the numbers already encountered within a single GPTABLE keyword: a
        /// repeat warns (map_member resolves it last-wins downstream), and a
        /// number above the GPTDIMS maximum (\p maxTables, when one is declared)
        /// warns too. Neither case rejects the table.
        static void warnOnTableNumber(std::unordered_set<int>& seenTableNumbers,
                                      int tableNumber,
                                      std::size_t maxTables,
                                      const KeywordLocation& location);

        /// Gas plant table number. Also the key used by the per-step
        /// ScheduleState container, matching the VFPProdTable convention.
        int name() const { return this->m_table_num; }

        // The component indices are validated as 1-based deck component numbers
        // (each in [1, numComponents]) forming a non-inverted bracket (lower <= upper).
        // The downstream physics interpretation of the bracket is deferred to the
        // first consumer of the recovery data.
        int lowerComponent() const { return this->m_lower_component; }
        int upperComponent() const { return this->m_upper_component; }
        std::size_t numComponents() const { return this->m_num_components; }

        /// Number of values stored per table row: the heavy-component mole
        /// fraction plus one recovery fraction per component.
        std::size_t rowWidth() const { return this->m_num_components + 1; }

        std::size_t numRows() const
        {
            // A table built through the parser always has m_num_components >= 1. A
            // default-constructed object filled via serializeOp on the restart/MPI
            // path bypasses the constructor; guard a zero component count so a
            // component-less object reports no rows rather than a meaningless count.
            if (this->m_num_components == 0) {
                return 0;
            }
            return this->m_data.size() / this->rowWidth();
        }

        /// Heavy-component mole fraction for \p row (the first column of the row).
        double heavyMoleFraction(std::size_t row) const
        {
            assert(row < this->numRows());
            return this->m_data[row * this->rowWidth()];
        }

        /// Recovery fraction of component \p comp for \p row. \p comp is a 0-based
        /// column index in [0, numComponents) -- it is NOT the 1-based deck component
        /// number returned by lowerComponent()/upperComponent(); a caller bracketing
        /// recovery columns by those indices must subtract 1.
        double recovery(std::size_t row, std::size_t comp) const
        {
            assert(row < this->numRows());
            assert(comp < this->m_num_components);
            return this->m_data[row * this->rowWidth() + 1 + comp];
        }

        bool operator==(const GasPlantTable& other) const;

        // serializeOp, operator==, and the member list must stay in lock-step:
        // every member below is also compared in operator== and set in
        // serializationTestObject().
        template <class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(this->m_table_num);
            serializer(this->m_lower_component);
            serializer(this->m_upper_component);
            serializer(this->m_num_components);
            serializer(this->m_data);
        }

    private:
        int m_table_num{};
        int m_lower_component{};
        int m_upper_component{};
        std::size_t m_num_components{};
        std::vector<double> m_data{};
    };

} // namespace Opm

#endif // OPM_GAS_PLANT_TABLE_HPP
