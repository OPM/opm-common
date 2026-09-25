/*
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
#ifndef OPM_PARSER_ROCKTABH_TABLE_HPP
#define OPM_PARSER_ROCKTABH_TABLE_HPP

#include <cstddef>
#include <vector>

namespace Opm {

    class DeckKeyword;

    /// Parses a single ROCKTABH table (i.e. one NTROCC region).  Unlike ROCKTAB,
    /// a ROCKTABH region's data is not one monotonic curve: it is a sequence of
    /// several "elastic curves" - reversible pore-volume/transmissibility
    /// multiplier-vs-pressure curves, one per historical pressure reversal
    /// ("turning pressure").  Each elastic curve is its own deck record,
    /// terminated by its own "/"; every region, including the last, must then
    /// be closed by an additional, standalone "/" line (the PVTO/PVTG
    /// "num_tables" convention) - though for the very last region that closing
    /// "/" is consumed to end the keyword and never shows up as an empty
    /// DeckRecord the way internal region separators do (see
    /// PvtxTable::recordRanges(), reused to find these region boundaries).  A
    /// curve's first row is also the point on the "deflation" (virgin/plastic
    /// loading) curve for that turning pressure - i.e. continuity between the
    /// plastic and elastic paths is guaranteed by construction, not by any
    /// runtime shift.
    class RocktabhTable {
    public:
        RocktabhTable() = default;

        /// Build from the elastic-curve records [firstRecord, lastRecord) of
        /// one ROCKTABH region, as identified by PvtxTable::recordRanges().
        RocktabhTable(const DeckKeyword& keyword,
                      std::size_t        firstRecord,
                      std::size_t        lastRecord,
                      bool               hasStressOption,
                      int                tableID);

        static RocktabhTable serializationTestObject();

        /// Number of stacked elastic curves in this table.
        std::size_t numElasticCurves() const;

        /// The pressure/multiplier values at the start (turning point) of elastic
        /// curve curveIdx.  Read off in order of increasing curveIdx, these are
        /// exactly the "deflation curve" - the virgin/plastic loading path.
        double turningPressure(std::size_t curveIdx) const;
        double turningPoreVolumeMultiplier(std::size_t curveIdx) const;
        double turningTransMultiplier(std::size_t curveIdx) const;

        /// The full pressure/multiplier columns of elastic curve curveIdx,
        /// starting at its turning point and extending to higher pressure.
        const std::vector<double>& elasticPressure(std::size_t curveIdx) const;
        const std::vector<double>& elasticPoreVolumeMultiplier(std::size_t curveIdx) const;
        const std::vector<double>& elasticTransMultiplier(std::size_t curveIdx) const;

        bool operator==(const RocktabhTable& data) const;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_curvePressure);
            serializer(m_curvePoreVolumeMultiplier);
            serializer(m_curveTransMultiplier);
        }

    private:
        std::vector<std::vector<double>> m_curvePressure;
        std::vector<std::vector<double>> m_curvePoreVolumeMultiplier;
        std::vector<std::vector<double>> m_curveTransMultiplier;
    };
}

#endif
