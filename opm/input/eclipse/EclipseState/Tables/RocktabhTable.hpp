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

#include <opm/input/eclipse/EclipseState/Tables/SimpleTable.hpp>

#include <cstddef>
#include <vector>

namespace Opm {

    class DeckKeyword;

    /// Parses a single ROCKTABH table (i.e. one NTROCC region).  A region's
    /// data is not one monotonic curve: it is a sequence of "elastic curves" -
    /// reversible pore-volume/transmissibility multiplier-vs-pressure curves,
    /// one per historical pressure reversal ("turning pressure").  Each
    /// elastic curve is its own deck record, terminated by its own "/"; every
    /// region, including the last, must then be closed by an additional,
    /// standalone "/" line of its own - a blank record with no value at all.
    /// TableManager::initRocktabhTables() finds each region's span of records
    /// by scanning the keyword for these blank records and passes it to the
    /// constructor below as [firstRecord, lastRecord).  The one exception is
    /// the very last region: its closing "/" ends the keyword and so is never
    /// turned into a blank record the way the earlier ones are, which is why
    /// that last range simply runs to the keyword's last real record.
    ///
    /// Each elastic curve, and the derived deflationCurve()/dilationCurve()
    /// tables, are plain SimpleTable objects with columns, in order: "PO"
    /// (pressure), "PV_MULT" (pore volume multiplier) and "PV_MULT_TRAN"
    /// (transmissibility multiplier).  Pressure-monotonicity (increasing, or
    /// decreasing under the STRESS option) is enforced at load time by the
    /// column schema shared by all of them.
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

        /// Elastic curve curveIdx, starting at its turning (reversal)
        /// pressure and extending to higher pressure.
        const SimpleTable& elasticCurve(std::size_t curveIdx) const;

        /// The "deflation" (virgin/plastic loading) curve: one row per
        /// elastic curve, taken from that curve's first row - i.e. read off
        /// in order of increasing curveIdx, the historical turning points.
        const SimpleTable& deflationCurve() const;

        /// The "dilation" curve: one row per elastic curve, taken from that
        /// curve's last row. The reference manual invokes it for both
        /// HYSTERESIS options: under HYSTER, "the dilation curves are
        /// extrapolated to infinite pressure" - i.e. above an elastic
        /// curve's last tabulated point its own trend simply continues,
        /// which Flow's HYSTER support gets for free by evaluating
        /// elasticCurve() with extrapolation enabled and therefore never
        /// needs to consult this table. Under BOBERG (which Flow does not
        /// currently implement), "the last points of each elastic curve are
        /// used as the dilation curves" instead, i.e. as a hard cap once
        /// pressure exceeds that point - that is what this accessor is for.
        const SimpleTable& dilationCurve() const;

        bool operator==(const RocktabhTable& data) const;

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_elasticCurves);
            serializer(m_deflationCurve);
            serializer(m_dilationCurve);
        }

    private:
        std::vector<SimpleTable> m_elasticCurves;
        SimpleTable m_deflationCurve;
        SimpleTable m_dilationCurve;
    };
}

#endif
