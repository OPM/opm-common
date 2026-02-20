/*
  Copyright 2015 Statoil ASA.

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

#ifndef OPM_PARSER_PVTX_TABLE_HPP
#define OPM_PARSER_PVTX_TABLE_HPP

#include <opm/input/eclipse/EclipseState/Tables/ColumnSchema.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SimpleTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableColumn.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableSchema.hpp>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace Opm {

    class DeckKeyword;

} // namespace Opm

namespace Opm {

    /// Base class for PVTG and PVTO tables.
    ///
    /// Maintains an internal representation of FVF/viscosity values versus
    /// pressure (PVTO) or composition (Rv, PVTG) for each of a set of
    /// composition (Rs, PVTO) or pressure (PVTG) nodes.
    ///
    /// PVTO
    /// --  Rs     Pressure    Bo        Viscosity
    /// --          (bar)                  (cP)
    ///
    ///  [ 20.59  {  50.00    1.10615     1.180 } ]        |
    ///           {  75.00    1.10164     1.247 }          |
    ///           { 100.00    1.09744     1.315 }          |
    ///           { 125.00    1.09351     1.384 }          |
    ///           { 150.00    1.08984     1.453 }/         |
    ///                                                    |
    ///  [ 28.19  {  70.00    1.12522     1.066 } ]        |
    ///           {  95.00    1.12047     1.124 }          |
    ///           { 120.00    1.11604     1.182 }          |-- PVT region 1
    ///           { 145.00    1.11191     1.241 }          |
    ///           { 170.00    1.10804     1.300 }/         |
    ///                                                    |
    ///  [ 36.01  {  90.00    1.14458     0.964 } ]        |
    ///           { 115.00    1.13959     1.014 }          |
    ///           { 140.00    1.13494     1.064 }          |
    ///           { 165.00    1.13060     1.115 }          |
    ///           { 190.00    1.12653     1.166 }/         |
    /// /                                                  |
    ///
    ///
    ///   404.60    594.29    1.97527     0.21564          |
    ///             619.29    1.96301     0.21981          |
    ///             644.29    1.95143     0.22393          |-- PVT region 2
    ///             669.29    1.94046     0.22801          |
    ///             694.29    1.93005     0.23204 /        |
    /// /                                                  |
    ///
    ///
    ///   404.60    594.29    1.97527     0.21564          |
    ///             619.29    1.96301     0.21981          |
    ///             644.29    1.95143     0.22393          |
    ///             669.29    1.94046     0.22801          |
    ///             694.29    1.93005     0.23204 /        |-- PVT region 3
    ///   404.60    594.29    1.97527     0.21564          |
    ///             619.29    1.96301     0.21981          |
    ///             644.29    1.95143     0.22393          |
    ///             669.29    1.94046     0.22801          |
    ///             694.29    1.93005     0.23204 /        |
    /// /                                                  |
    ///
    ///
    /// Saturated states are marked with [ ... ], while the corresponding
    /// under-saturated tables are marked with { ... }.  Thus, for PVT
    /// region 1 the table of saturated properties is
    ///
    ///    Rs        Pressure    Bo          Viscosity
    ///    20.59     50.00       1.10615     1.180
    ///    28.19     70.00       1.12522     1.066
    ///    36.01     90.00       1.14458     0.964
    ///
    /// This table is available through member function getSaturatedTable().
    ///
    /// For each composition (Rs) value there is a table of under-saturated
    /// properties.  These tables may be retrieved through member function
    /// getUnderSaturatedTable(index) in which 'index' identifies the
    /// composition node.  In the example above the under-saturated table in
    /// PVT region 1 for Rs=28.19 (i.e., index = 1) is
    ///
    ///     Pressure     Bo          Viscosity
    ///        70.00     1.12522     1.066
    ///        95.00     1.12047     1.124
    ///       120.00     1.11604     1.182
    ///       145.00     1.11191     1.241
    ///       170.00     1.10804     1.300
    ///
    class PvtxTable
    {
    public:
        /// Number of complete tables in input PVTx keyword.
        ///
        /// This is effectively the number of regions for which input PVT
        /// data is provided and should typically match the run's number of
        /// PVT regions.
        ///
        /// \param[in] keyword Input PVTx keyword--typically PVTO or PVTG.
        ///
        /// \return Number of complete tables in \p keyword.
        static std::size_t numTables(const DeckKeyword& keyword);

        /// Identify which input records pertain to which PVT regions
        ///
        /// \param[in] keyword Input PVTx keyword--typically PVTO or PVTG.
        ///
        /// \return Keyword index ranges.  One range for each PVT region.
        /// Each range is defined by a pair of start/end indices into the
        /// input \p keyword.
        static std::vector<std::pair<std::size_t, std::size_t>>
        recordRanges(const DeckKeyword& keyword);

        /// Default constructor.
        ///
        /// Resulting object is mostly usable as a target for a
        /// deserialisation operation.
        PvtxTable() = default;

        /// Constructor.
        ///
        /// Forms an empty table object that must be populated in a
        /// subsequent call to init().
        ///
        /// \param[in] columnName Name of primary ("outer") lookup key.
        /// User controlled name for the composition/pressure/etc node.
        explicit PvtxTable(const std::string& columnName);

        /// Virtual destructor.
        ///
        /// This type is intended to be a base class for others--i.e., those
        /// that define the specifics of a particular PVTx table.
        virtual ~PvtxTable() = default;

        /// Create a serialisation test object.
        static PvtxTable serializationTestObject();

        /// Retrieve derived table of saturated states.
        ///
        /// Generated from first row of each sub-table, along with the
        /// associated composition (PVTO) or pressure (PVTG) information.
        const SimpleTable& getSaturatedTable() const;

        /// Retrieve sub-table for a single composition or pressure node.
        ///
        /// \param[in] tableNumber Node index in the range 0..size()-1.
        const SimpleTable& getUnderSaturatedTable(std::size_t tableNumber) const;

        /// Retrieve composition/pressure node value at input point.
        ///
        /// \param[in] index Node index in the range 0..size()-1.
        ///
        /// \return Input composition (Rs, PVTO) or pressure (Pg, PVTG) at
        /// \p index.
        double getArgValue(std::size_t index) const;

        /// Number of sub-tables.
        ///
        /// Effectively the number of composition (PVTO) or pressure (PVTG)
        /// nodes in the input table.
        std::size_t size() const;

        /// Start of sequence of sub-tables.
        ///
        /// One sub-table for each composition (PVTO) or pressure (PVTG)
        /// node.
        auto begin() const { return this->m_underSaturatedTables.begin(); }

        /// Start of sequence of sub-tables.
        ///
        /// One sub-table for each composition (PVTO) or pressure (PVTG)
        /// node.
        auto cbegin() const { return this->m_underSaturatedTables.cbegin(); }

        /// End of sequence of sub-tables.
        auto end() const { return this->m_underSaturatedTables.end(); }

        /// End of sequence of sub-tables.
        auto cend() const { return this->m_underSaturatedTables.cend(); }

        /// Equality predicate
        ///
        /// \param[in] data Object against which \code *this \endcode will be
        /// tested for equality.
        ///
        /// \return Whether or not \code *this \endcode is the same as \p
        /// data.
        bool operator==(const PvtxTable& data) const;

        /// Convert between byte array and object representation.
        ///
        /// \tparam Serializer Byte array conversion protocol.
        ///
        /// \param[in,out] serializer Byte array conversion object.
        template <class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_outerColumnSchema);
            serializer(m_outerColumn);
            serializer(m_underSaturatedSchema);
            serializer(m_saturatedSchema);
            serializer(m_underSaturatedTables);
            serializer(m_saturatedTable);
        }

    protected:
        /// Table description of primary lookup key.
        ///
        /// Typically the composition (Rs, PVTO) or the pressure (Pg, PVTG).
        ColumnSchema m_outerColumnSchema{};

        /// Primary lookup key values.
        TableColumn m_outerColumn{};

        /// Table description of under-saturated states.
        TableSchema m_underSaturatedSchema{};

        /// Table description of saturated states.
        TableSchema m_saturatedSchema{};

        /// Under-saturated sub-tables.
        ///
        /// One table for each value of the primary lookup key.
        std::vector<SimpleTable> m_underSaturatedTables{};

        /// Inferred table of saturated states.
        SimpleTable m_saturatedTable{};

        /// Populate internal data structures from PVTx input table data.
        ///
        /// Fills the under-saturated sub-tables and generates the inferred
        /// "saturated" table for a single PVT region.  Clients--i.e.,
        /// derived classes--must define the table "schema" members (i.e.,
        /// m_underSaturatedSchema and m_saturatedSchema) prior to calling
        /// init().
        ///
        /// \param[in] keyword Input PVTx table data.  Typically containing
        /// PVTO or PVTG (or similar) tables.
        ///
        /// \param[in] tableIdx Zero-based region index for which to
        /// internalise table data.
        void init(const DeckKeyword& keyword, std::size_t tableIdx);

    private:
        /// Populate collection of under-saturated tables.
        ///
        /// Inserts values into \c m_underSaturatedTables.  This is the
        /// first main stage of init().
        ///
        /// \param[in] keyword Input PVTx table data.  Typically containing
        /// PVTO or PVTG tables.
        ///
        /// \param[in] tableIdx Zero-based region index for which to
        /// internalise table data.
        ///
        /// \param[in] first Starting index in keyword range for PVT region
        /// \p tableIdx.
        ///
        /// \param[in] last One-past-end index in keyword range for PVT
        /// region \p tableIdx.
        void populateUndersaturatedTables(const DeckKeyword& keyword,
                                          const std::size_t  tableIdx,
                                          const std::size_t  first,
                                          const std::size_t  last);

        /// Populate derived table of saturated states.
        ///
        /// Inserts values into \c m_saturatedTable.  This is the final
        /// stage of init().
        ///
        /// \param[in] tableName Name of table/keyword we're internalising.
        /// Typically \code "PVTO" \endcode or \code "PVTG" \endcode.
        void populateSaturatedTable(const std::string& tableName);

        /// Fill in any missing under-saturated states.
        ///
        /// Takes scaled copies of under-saturated curves at higher
        /// composition/pressure nodes.  Amends m_underSaturatedTables.
        void populateMissingUndersaturatedStates();

        /// Identify missing under-saturated states in
        /// m_underSaturatedTables.
        ///
        /// \return Pairs of source/destination indices.  The
        /// under-saturated destination entries in m_underSaturatedTables
        /// will be scaled copies of the under-saturated source entries in
        /// m_underSaturatedTables.
        std::vector<std::pair<std::size_t, std::size_t>>
        missingUSatTables() const;

        /// Generate scaled copies of under-saturated state curves.
        ///
        /// Intended to amend a specific entry in m_underSaturatedTables
        /// based on source values in another specific entry in
        /// m_underSaturatedTables.
        ///
        /// Virtual function in order to call back into the derived type for
        /// type-specific copying.
        ///
        /// \param[in] src Index of source table with full set of
        /// under-saturated states.
        ///
        /// \param[in] dest Index of destination table with no
        /// under-saturated states.
        virtual void makeScaledUSatTableCopy(const std::size_t src,
                                             const std::size_t dest);
    };

} // namespace Opm

#endif // OPM_PARSER_PVTX_TABLE_HPP
