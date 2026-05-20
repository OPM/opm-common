/*
  Copyright (C) 2026 SINTEF Digital

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
#ifndef OPM_PARSER_COMPVD_TABLE_HPP
#define OPM_PARSER_COMPVD_TABLE_HPP

#include "SimpleTable.hpp"

#include <cstddef>
#include <vector>

namespace Opm {

    class DeckItem;
    class KeywordLocation;
    class UnitSystem;

    /// Table representation of the E300 COMPVD keyword.
    ///
    /// Each row of the input contains, in order:
    ///   * Depth                                       (Length)
    ///   * NCOMPS total mole fractions z_i             (dimensionless)
    ///   * Phase flag: 0 = vapor (above GOC),
    ///                 1 = liquid (below GOC)          (int)
    ///   * Observed saturation pressure Psat           (Pressure)
    ///
    /// Internally only the depth, mole-fraction, and Psat columns are
    /// stored in the underlying SimpleTable (since SimpleTable is
    /// hard-coded to operate on doubles).  The phase flag is a discrete
    /// label and is therefore stored separately as a `std::vector<int>`
    /// exposed via `phaseFlag(row)` / `phaseFlags()`.
    ///
    /// Mole fractions on each row are checked to sum to 1, and the phase
    /// flag is checked to be either 0 or 1.
    class CompvdTable : public SimpleTable {
    public:
        CompvdTable(const DeckItem& item,
                    const int tableID,
                    const int numComponents,
                    const KeywordLocation& location);

        const TableColumn& getDepthColumn() const;
        const TableColumn& getMoleFractionColumn(int componentIdx) const;
        /// Observed saturation pressure, in SI units (Pa).
        const TableColumn& getSaturationPressureColumn() const;

        /// 0 (vapor) or 1 (liquid) for the given row.
        int phaseFlag(std::size_t rowIdx) const;
        const std::vector<int>& phaseFlags() const { return phaseFlags_; }

        int numComponents() const { return numComponents_; }

    private:
        int numComponents_;
        std::vector<int> phaseFlags_;
    };

}

#endif
