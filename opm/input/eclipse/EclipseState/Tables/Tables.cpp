/*
   Copyright (C) 2013 by Andreas Lauser

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

#include <opm/input/eclipse/EclipseState/Tables/SimpleTable.hpp>

#include <opm/input/eclipse/EclipseState/Tables/ColumnSchema.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableColumn.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableSchema.hpp>

#include <opm/input/eclipse/EclipseState/Tables/AqutabTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/BiofilmTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/DiffMICPTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/EnkrvdTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/EnptvdTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/FlatTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/FoamadsTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/FoammobTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/GasvisctTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/GsfTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/ImkrvdTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/ImptvdTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/MiscTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/MsfnTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/OilvisctTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/OverburdTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PbvdTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PdvdTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PcfactTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PermfactTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PlyadsTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PlydhflfTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PlymaxTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PlyrockTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PlyshlogTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PlyviscTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PmiscTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PvdgTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PvdoTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PvdsTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PvtgTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PvtgwTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PvtgwoTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PvtoTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PvtsolTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/RocktabTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/RockwnodTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/RsvdTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/RtempvdTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/RvvdTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/RvwvdTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/RwgsaltTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SaltSolubilityTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SaltpvdTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SaltvdTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SgcwmisTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SgfnTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SgofTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SgwfnTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SlgofTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/Sof2Table.hpp>
#include <opm/input/eclipse/EclipseState/Tables/Sof3Table.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SorwmisTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SpecheatTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SpecrockTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SsfnTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SwfnTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SwofTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableContainer.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TlpmixpaTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TracerVdTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/WatvisctTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/WsfTable.hpp>

#include <opm/input/eclipse/Units/Dimension.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>
#include <opm/input/eclipse/Units/Units.hpp>

#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/D.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/G.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/P.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/R.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/V.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

#include <stddef.h>

#include <fmt/format.h>

namespace {

    /// Simple query interface for extracting tabulated values of saturated
    /// gas.
    ///
    /// In particular, supports querying the tabulated gas pressure and its
    /// associated formation volume factor, viscosity and vaporised oil
    /// concentration ("Rv") values.
    struct GasPropertyTableInterface
    {
        /// Retrieve gas pressure at saturated conditions
        ///
        /// \param[in] row Row index in saturated gas property table.
        ///   Typically 0 or 1 in the intended/primary use case.
        virtual double pressure(const std::size_t row) const = 0;

        /// Retrieve formation volume factor for gas at saturated
        /// conditions.
        ///
        /// \param[in] row Row index in saturated gas property table.
        ///   Typically 0 or 1 in the intended/primary use case.
        virtual double fvf(const std::size_t row) const = 0;

        /// Retrieve phase viscosity for gas at saturated conditions.
        ///
        /// \param[in] row Row index in saturated gas property table.
        ///   Typically 0 or 1 in the intended/primary use case.
        virtual double viscosity(const std::size_t row) const = 0;

        /// Retrieve vaporised oil concentration ("Rv") for gas at saturated
        /// conditions.
        ///
        /// \param[in] row Row index in saturated gas property table.
        ///   Typically 0 or 1 in the intended/primary use case.
        virtual double vaporisedOil(const std::size_t row) const = 0;
    };

    /// Padding for gas property tables at low pressure
    class LowPressureTablePadding
    {
    public:
        /// Constructor
        ///
        /// \param[in] Tabulated gas properties at saturated conditions
        explicit LowPressureTablePadding(const GasPropertyTableInterface& prop);

        /// Whether or not input table needs padding at low pressures
        bool inputNeedsPadding() const { return this->needPadding_; }

        /// Low pressure padding rows for input table.  Needed only if
        /// inputNeedsPadding() returns \c true.
        Opm::SimpleTable padding() const;

    private:
        /// Gas pressure values in rows zero and one of input table.
        std::array<double, 2> p_{};

        /// Interpolated gas property values at "limiting" pressure
        struct {
            /// Limiting pressure
            double p {0.0};

            /// Vaporised oil concentration at limiting pressure
            double rv {0.0};

            /// Formation volume factor at limiting pressure
            double fvf {0.0};

            /// Gas viscosity at limiting pressure
            double mu {0.0};
        } limit_{};

        /// Whether or not input table needs padding at low pressure values.
        bool needPadding_{false};
    };

    LowPressureTablePadding::LowPressureTablePadding(const GasPropertyTableInterface& prop)
        : p_{ prop.pressure(0), prop.pressure(1) }
    {
        auto linInterp = [](const std::array<double, 2>& xi,
                            const std::array<double, 2>& yi,
                            const double                  x)
        {
            const auto t = (x - xi[0]) / (xi[1] - xi[0]);

            return (1.0 - t)*yi[0] + t*yi[1];
        };

        const auto rv = std::array {
            prop.vaporisedOil(0),
            prop.vaporisedOil(1),
        };

        const auto b = std::array {
            1.0 / prop.fvf(0),  // 1 / B(0)
            1.0 / prop.fvf(1),  // 1 / B(1)
        };

        const auto recipBmu = std::array {
            b[0] / prop.viscosity(0), // 1 / (B(0) * mu(0))
            b[1] / prop.viscosity(1), // 1 / (B(1) * mu(1))
        };

        this->limit_.fvf = std::max(10.0 / b[0], 1.0);
        this->limit_.p   = linInterp(b, this->p_, 1.0 / this->limit_.fvf);
        this->limit_.rv  = linInterp(this->p_, rv, this->limit_.p);
        this->limit_.mu  = 1.0 /
            (this->limit_.fvf * linInterp(this->p_, recipBmu, this->limit_.p));

        const auto p0 = 1.0*Opm::unit::barsa;

        this->needPadding_ = (this->p_[0] > p0) &&
            (linInterp(this->p_, b, p0) < 1.0 / this->limit_.fvf);
    }

    Opm::SimpleTable LowPressureTablePadding::padding() const
    {
        auto padSchema = Opm::TableSchema{};
        padSchema.addColumn(Opm::ColumnSchema("PG" , Opm::Table::STRICTLY_INCREASING, Opm::Table::DEFAULT_NONE));
        padSchema.addColumn(Opm::ColumnSchema("RV" , Opm::Table::RANDOM, Opm::Table::DEFAULT_NONE));
        padSchema.addColumn(Opm::ColumnSchema("BG" , Opm::Table::RANDOM, Opm::Table::DEFAULT_LINEAR));
        padSchema.addColumn(Opm::ColumnSchema("MUG", Opm::Table::RANDOM, Opm::Table::DEFAULT_LINEAR));

        auto padTable = Opm::SimpleTable { std::move(padSchema) };

        const auto p0 = 1.0*Opm::unit::barsa;

        if (this->limit_.p < this->p_[0]) {
            if (p0 < this->limit_.p) {
                padTable.addRow({ p0, this->limit_.rv, 1.1*this->limit_.fvf, this->limit_.mu }, "PAD");
            }

            padTable.addRow({ this->limit_.p, this->limit_.rv, this->limit_.fvf, this->limit_.mu }, "PAD");
        }

        return padTable;
    }

    // -------------------------------------------------------------------------

    /// Gas property query interface implementation for dry gas
    class DryGasTable : public GasPropertyTableInterface
    {
    public:
        /// Constructor
        ///
        /// \param[in] pvdg Tabulated gas property values for dry gas
        explicit DryGasTable(const Opm::PvdgTable& pvdg)
            : pvdg_ { pvdg }
        {}

        /// Retrieve gas pressure at saturated conditions
        ///
        /// \param[in] row Row index in saturated gas property table.
        ///   Typically 0 or 1 in the intended/primary use case.
        double pressure(const std::size_t row) const override;

        /// Retrieve formation volume factor for gas at saturated
        /// conditions.
        ///
        /// \param[in] row Row index in saturated gas property table.
        ///   Typically 0 or 1 in the intended/primary use case.
        double fvf(const std::size_t row) const override;

        /// Retrieve phase viscosity for gas at saturated conditions.
        ///
        /// \param[in] row Row index in saturated gas property table.
        ///   Typically 0 or 1 in the intended/primary use case.
        double viscosity(const std::size_t row) const override;

        /// Retrieve vaporised oil concentration ("Rv") for gas at saturated
        /// conditions.
        ///
        /// \param[in] row Row index in saturated gas property table.
        ///   Typically 0 or 1 in the intended/primary use case.  Ignored
        ///   for dry gas, as the vaporised oil concentration is always zero
        ///   in this case.
        double vaporisedOil([[maybe_unused]] const std::size_t row) const override
        {
            return 0.0;
        }

    private:
        /// Underlying property table.
        std::reference_wrapper<const Opm::PvdgTable> pvdg_;
    };

    double DryGasTable::pressure(const std::size_t row) const
    {
        return this->pvdg_.get().getPressureColumn()[row];
    }

    double DryGasTable::fvf(const std::size_t row) const
    {
        return this->pvdg_.get().getFormationFactorColumn()[row];
    }

    double DryGasTable::viscosity(const std::size_t row) const
    {
        return this->pvdg_.get().getViscosityColumn()[row];
    }

    // -------------------------------------------------------------------------

    /// Gas property query interface implementation for wet gas
    class WetGasTable : public GasPropertyTableInterface
    {
    public:
        /// Constructor
        ///
        /// \param[in] pvtg Tabulated gas property values for wet gas
        explicit WetGasTable(const Opm::PvtgTable& pvtg)
            : satTable_ { pvtg.getSaturatedTable() }
        {}

        /// Retrieve gas pressure at saturated conditions
        ///
        /// \param[in] row Row index in saturated gas property table.
        ///   Typically 0 or 1 in the intended/primary use case.
        double pressure(const std::size_t row) const override;

        /// Retrieve formation volume factor for gas at saturated
        /// conditions.
        ///
        /// \param[in] row Row index in saturated gas property table.
        ///   Typically 0 or 1 in the intended/primary use case.
        double fvf(const std::size_t row) const override;

        /// Retrieve phase viscosity for gas at saturated conditions.
        ///
        /// \param[in] row Row index in saturated gas property table.
        ///   Typically 0 or 1 in the intended/primary use case.
        double viscosity(const std::size_t row) const override;

        /// Retrieve vaporised oil concentration ("Rv") for gas at saturated
        /// conditions.
        ///
        /// \param[in] row Row index in saturated gas property table.
        ///   Typically 0 or 1 in the intended/primary use case.
        double vaporisedOil(const std::size_t row) const override;

    private:
        /// Underlying property table for wet gas at saturated conditions.
        std::reference_wrapper<const Opm::SimpleTable> satTable_;
    };

    double WetGasTable::pressure(const std::size_t row) const
    {
        return this->satTable_.get().getColumn(0)[row];
    }

    double WetGasTable::fvf(const std::size_t row) const
    {
        return this->satTable_.get().getColumn(2)[row];
    }

    double WetGasTable::viscosity(const std::size_t row) const
    {
        return this->satTable_.get().getColumn(3)[row];
    }

    double WetGasTable::vaporisedOil(const std::size_t row) const
    {
        return this->satTable_.get().getColumn(1)[row];
    }

} // Anonymous namespace

namespace Opm
{

namespace {
    DeckItem createPvtgItemZero(const DeckKeyword& pvtgTableInput)
    {
        return pvtgTableInput[0].getItem(0).emptyStructuralCopy();
    }

    DeckItem createPvtgItemOne(const DeckKeyword& pvtgTableInput)
    {
        return pvtgTableInput[0].getItem(1).emptyStructuralCopy();
    }

    DeckKeyword paddedPVTGTable(const SimpleTable& padding,
                                const DeckKeyword& pvtgTableInput,
                                const PvtgTable&   pvtgTable)
    {
        auto paddedTable = pvtgTableInput.emptyStructuralCopy();

        // Padding
        {
            const auto nrow = padding.numRows();
            const auto ncol = padding.numColumns();

            for (auto row = 0*nrow; row < nrow; ++row) {
                auto recordItems = std::vector {
                    createPvtgItemZero(pvtgTableInput),
                    createPvtgItemOne (pvtgTableInput)
                };

                {
                    auto& pg = recordItems.front();
                    const auto& dim = pg.getActiveDimensions();
                    pg.push_back(dim.front().convertSiToRaw(padding.get(0, row)));
                }

                {
                    auto& rest = recordItems.back();
                    const auto& dim = rest.getActiveDimensions();
                    auto n = 0*dim.size();

                    for (auto col = 1 + 0*ncol; col < ncol; ++col, n = (n + 1) % dim.size()) {
                        rest.push_back(dim[n].convertSiToRaw(padding.get(col, row)));
                    }
                }

                paddedTable.addRecord(DeckRecord { std::move(recordItems) });
            }
        }

        // Original input table
        for (auto pgIx = 0*pvtgTable.size(); pgIx < pvtgTable.size(); ++pgIx) {
            auto recordItems = std::vector {
                createPvtgItemZero(pvtgTableInput),
                createPvtgItemOne (pvtgTableInput)
            };

            {
                auto& pg = recordItems.front();
                const auto& dim = pg.getActiveDimensions();
                pg.push_back(dim.front().convertSiToRaw(pvtgTable.getArgValue(pgIx)));
            }

            {
                auto& rest = recordItems.back();
                const auto& dim = rest.getActiveDimensions();

                const auto& pgTable = pvtgTable.getUnderSaturatedTable(pgIx);
                const auto nrow = pgTable.numRows();
                const auto ncol = pgTable.numColumns();

                for (auto row = 0*nrow; row < nrow; ++row) {
                    for (auto col = 0*ncol; col < ncol; ++col) {
                        rest.push_back(dim[col].convertSiToRaw(pgTable.get(col, row)));
                    }
                }
            }

            paddedTable.addRecord(DeckRecord { std::move(recordItems) });
        }

        // Resulting padded table holds values for just a single table/PVT
        // region, even if pvtgTableInput holds tables for multiple PVT
        // regions.
        return paddedTable;
    }
} // Anonymous namespace

PvtgTable::PvtgTable(const DeckKeyword& keyword, size_t tableIdx)
    : PvtxTable("P")
{
    m_underSaturatedSchema.addColumn(ColumnSchema("RV", Table::STRICTLY_DECREASING, Table::DEFAULT_NONE));
    m_underSaturatedSchema.addColumn(ColumnSchema("BG", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_underSaturatedSchema.addColumn(ColumnSchema("MUG", Table::RANDOM, Table::DEFAULT_LINEAR));

    m_saturatedSchema.addColumn(ColumnSchema("PG", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_saturatedSchema.addColumn(ColumnSchema("RV", Table::RANDOM, Table::DEFAULT_NONE));
    m_saturatedSchema.addColumn(ColumnSchema("BG", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_saturatedSchema.addColumn(ColumnSchema("MUG", Table::RANDOM, Table::DEFAULT_LINEAR));

    // Run full table initialisation first.  The downside to this is that we
    // will end up throwing away and redoing that work if the table needs
    // padding at low pressure values.  On the other hand, the full table
    // initialisation procedure also checks consistency and replaces any
    // defaulted values which means we don't have to have special logic in
    // place to handle those complexities if the table does need padding.

    PvtxTable::init(keyword, tableIdx);

    if (this->size() <= 1) {
        // At most a single pressure node in the input PVTG data.  There is
        // not enough information to perform table padding, even if it might
        // be needed.  We might for instance be running in the context of a
        // unit test with incomplete data or the input might just be very
        // sparse.  In any case we can't perform table padding so just
        // return here.
        return;
    }

    if (const auto tablePadding = LowPressureTablePadding { WetGasTable { *this } };
        tablePadding.inputNeedsPadding())
    {
        // Note: The padded PVTG table holds values for a single table/PVT
        // region, even if 'keyword' holds tables for multiple PVT regions.
        // We therefore unconditionally pass '0' as the 'tableIdx' in this
        // case.  Moreover, PvtxTable::init() expects that both the outer
        // column and the array of undersaturated tables are empty, so clear
        // those here, once we've used their contents to form the padding
        // table.

        const auto paddedTable =
            paddedPVTGTable(tablePadding.padding(), keyword, *this);

        this->m_outerColumn = TableColumn { this->m_outerColumnSchema };
        this->m_underSaturatedTables.clear();

        PvtxTable::init(paddedTable, 0);
    }
}

PvtgTable
PvtgTable::serializationTestObject()
{
    PvtgTable result;
    static_cast<PvtxTable&>(result) = PvtxTable::serializationTestObject();

    return result;
}

bool
PvtgTable::operator==(const PvtgTable& data) const
{
    return static_cast<const PvtxTable&>(*this) == static_cast<const PvtxTable&>(data);
}

PvtgwTable::PvtgwTable(const DeckKeyword& keyword, size_t tableIdx)
    : PvtxTable("P")
{
    m_underSaturatedSchema.addColumn(ColumnSchema("RW", Table::STRICTLY_DECREASING, Table::DEFAULT_NONE));
    m_underSaturatedSchema.addColumn(ColumnSchema("BG", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_underSaturatedSchema.addColumn(ColumnSchema("MUG", Table::RANDOM, Table::DEFAULT_LINEAR));

    m_saturatedSchema.addColumn(ColumnSchema("PG", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_saturatedSchema.addColumn(ColumnSchema("RW", Table::RANDOM, Table::DEFAULT_NONE));
    m_saturatedSchema.addColumn(ColumnSchema("BG", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_saturatedSchema.addColumn(ColumnSchema("MUG", Table::RANDOM, Table::DEFAULT_LINEAR));

    PvtxTable::init(keyword, tableIdx);
}

PvtgwTable
PvtgwTable::serializationTestObject()
{
    PvtgwTable result;
    static_cast<PvtxTable&>(result) = PvtxTable::serializationTestObject();

    return result;
}

bool
PvtgwTable::operator==(const PvtgwTable& data) const
{
    return static_cast<const PvtxTable&>(*this) == static_cast<const PvtxTable&>(data);
}

PvtgwoTable::PvtgwoTable(const DeckKeyword& keyword, size_t tableIdx)
    : PvtxTable("P")
{

    m_underSaturatedSchema.addColumn(ColumnSchema("RV", Table::STRICTLY_DECREASING, Table::DEFAULT_NONE));
    m_underSaturatedSchema.addColumn(ColumnSchema("RW", Table::STRICTLY_DECREASING, Table::DEFAULT_NONE));
    m_underSaturatedSchema.addColumn(ColumnSchema("BG", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_underSaturatedSchema.addColumn(ColumnSchema("MUG", Table::RANDOM, Table::DEFAULT_LINEAR));

    m_saturatedSchema.addColumn(ColumnSchema("PG", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_saturatedSchema.addColumn(ColumnSchema("RV", Table::RANDOM, Table::DEFAULT_NONE));
    m_saturatedSchema.addColumn(ColumnSchema("RW", Table::RANDOM, Table::DEFAULT_NONE));
    m_saturatedSchema.addColumn(ColumnSchema("BG", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_saturatedSchema.addColumn(ColumnSchema("MUG", Table::RANDOM, Table::DEFAULT_LINEAR));

    PvtxTable::init(keyword, tableIdx);
}

PvtgwoTable
PvtgwoTable::serializationTestObject()
{
    PvtgwoTable result;
    static_cast<PvtxTable&>(result) = PvtxTable::serializationTestObject();

    return result;
}

bool
PvtgwoTable::operator==(const PvtgwoTable& data) const
{
    return static_cast<const PvtxTable&>(*this) == static_cast<const PvtxTable&>(data);
}

PvtoTable::PvtoTable(const DeckKeyword& keyword, size_t tableIdx)
    : PvtxTable("RS")
{
    m_underSaturatedSchema.addColumn(ColumnSchema("P", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_underSaturatedSchema.addColumn(ColumnSchema("BO", Table::STRICTLY_DECREASING, Table::DEFAULT_LINEAR));
    m_underSaturatedSchema.addColumn(ColumnSchema("MU", Table::RANDOM, Table::DEFAULT_LINEAR));

    m_saturatedSchema.addColumn(ColumnSchema("RS", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_saturatedSchema.addColumn(ColumnSchema("P", Table::RANDOM, Table::DEFAULT_NONE));
    m_saturatedSchema.addColumn(ColumnSchema("BO", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_saturatedSchema.addColumn(ColumnSchema("MU", Table::RANDOM, Table::DEFAULT_LINEAR));

    PvtxTable::init(keyword, tableIdx);
}

PvtoTable
PvtoTable::serializationTestObject()
{
    PvtoTable result;
    static_cast<PvtxTable&>(result) = PvtxTable::serializationTestObject();

    return result;
}

bool
PvtoTable::operator==(const PvtoTable& data) const
{
    return static_cast<const PvtxTable&>(*this) == static_cast<const PvtxTable&>(data);
}

std::vector<PvtoTable::FlippedFVF>
PvtoTable::nonMonotonicSaturatedFVF() const
{
    auto nonmonoFVF = std::vector<FlippedFVF> {};

    const auto& Rs = this->m_saturatedTable.getColumn("RS");
    const auto& Bo = this->m_saturatedTable.getColumn("BO");

    const auto nrec = Rs.size();
    for (auto rec = 1 + 0 * nrec; rec < nrec; ++rec) {
        if (Bo[rec] > Bo[rec - 1]) {
            // Normal case, Bo increases monotonically.
            continue;
        }

        auto& flip = nonmonoFVF.emplace_back();

        flip.i = rec;
        flip.Rs[0] = Rs[rec - 1];
        flip.Rs[1] = Rs[rec];
        flip.Bo[0] = Bo[rec - 1];
        flip.Bo[1] = Bo[rec];
    }

    return nonmonoFVF;
}

PvtsolTable::PvtsolTable(const DeckKeyword& keyword, size_t tableIdx)
    : PvtxTable("ZCO2")
{
    m_underSaturatedSchema.addColumn(ColumnSchema("P", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_underSaturatedSchema.addColumn(ColumnSchema("B_O", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_underSaturatedSchema.addColumn(ColumnSchema("B_G", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_underSaturatedSchema.addColumn(ColumnSchema("RS", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_underSaturatedSchema.addColumn(ColumnSchema("RV", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_underSaturatedSchema.addColumn(ColumnSchema("XVOL", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_underSaturatedSchema.addColumn(ColumnSchema("YVOL", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_underSaturatedSchema.addColumn(ColumnSchema("MU_O", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_underSaturatedSchema.addColumn(ColumnSchema("MU_G", Table::RANDOM, Table::DEFAULT_LINEAR));

    m_saturatedSchema.addColumn(ColumnSchema("ZCO2", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_saturatedSchema.addColumn(ColumnSchema("P", Table::INCREASING, Table::DEFAULT_NONE));
    m_saturatedSchema.addColumn(ColumnSchema("B_O", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_saturatedSchema.addColumn(ColumnSchema("B_G", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_saturatedSchema.addColumn(ColumnSchema("RS", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_saturatedSchema.addColumn(ColumnSchema("RV", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_saturatedSchema.addColumn(ColumnSchema("XVOL", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_saturatedSchema.addColumn(ColumnSchema("YVOL", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_saturatedSchema.addColumn(ColumnSchema("MU_O", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_saturatedSchema.addColumn(ColumnSchema("MU_G", Table::RANDOM, Table::DEFAULT_LINEAR));

    PvtxTable::init(keyword, tableIdx);
}

PvtsolTable
PvtsolTable::serializationTestObject()
{
    PvtsolTable result;
    static_cast<PvtxTable&>(result) = PvtxTable::serializationTestObject();

    return result;
}

bool
PvtsolTable::operator==(const PvtsolTable& data) const
{
    return static_cast<const PvtxTable&>(*this) == static_cast<const PvtxTable&>(data);
}

RwgsaltTable::RwgsaltTable(const DeckKeyword& keyword, size_t tableIdx)
    : PvtxTable("P")
{

    m_underSaturatedSchema.addColumn(ColumnSchema("C_SALT", Table::INCREASING, Table::DEFAULT_NONE));
    m_underSaturatedSchema.addColumn(ColumnSchema("RVW", Table::RANDOM, Table::DEFAULT_LINEAR));

    m_saturatedSchema.addColumn(ColumnSchema("PG", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_saturatedSchema.addColumn(ColumnSchema("C_SALT", Table::INCREASING, Table::DEFAULT_NONE));
    m_saturatedSchema.addColumn(ColumnSchema("RVW", Table::RANDOM, Table::DEFAULT_LINEAR));

    PvtxTable::init(keyword, tableIdx);
}

RwgsaltTable
RwgsaltTable::serializationTestObject()
{
    RwgsaltTable result;
    static_cast<PvtxTable&>(result) = PvtxTable::serializationTestObject();

    return result;
}

bool
RwgsaltTable::operator==(const RwgsaltTable& data) const
{
    return static_cast<const PvtxTable&>(*this) == static_cast<const PvtxTable&>(data);
}

SpecheatTable::SpecheatTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("TEMPERATURE", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("CV_OIL", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("CV_WATER", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("CV_GAS", Table::RANDOM, Table::DEFAULT_LINEAR));

    SimpleTable::init("SPECHEAT", item, tableID);
}

const TableColumn&
SpecheatTable::getTemperatureColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
SpecheatTable::getCvOilColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
SpecheatTable::getCvWaterColumn() const
{
    return SimpleTable::getColumn(2);
}

const TableColumn&
SpecheatTable::getCvGasColumn() const
{
    return SimpleTable::getColumn(3);
}

SpecrockTable::SpecrockTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("TEMPERATURE", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("CV_ROCK", Table::RANDOM, Table::DEFAULT_LINEAR));

    SimpleTable::init("SPEROCK", item, tableID);
}

const TableColumn&
SpecrockTable::getTemperatureColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
SpecrockTable::getCvRockColumn() const
{
    return SimpleTable::getColumn(1);
}

SwofTable::SwofTable(const DeckItem& item, const bool jfunc, const int tableID)
{

    m_schema.addColumn(ColumnSchema("SW", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("KRW", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("KROW", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("PCOW", Table::RANDOM, Table::DEFAULT_LINEAR));

    m_jfunc = jfunc;
    SimpleTable::init("SWOF", item, tableID);
}

const TableColumn&
SwofTable::getSwColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
SwofTable::getKrwColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
SwofTable::getKrowColumn() const
{
    return SimpleTable::getColumn(2);
}

const TableColumn&
SwofTable::getPcowColumn() const
{
    SimpleTable::assertJFuncPressure(false);
    return SimpleTable::getColumn(3);
}

const TableColumn&
SwofTable::getJFuncColumn() const
{
    SimpleTable::assertJFuncPressure(true);
    return SimpleTable::getColumn(3);
}

SgwfnTable::SgwfnTable(const DeckItem& item, const int tableID)
{

    m_schema.addColumn(ColumnSchema("SG", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("KRG", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("KRGW", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("PCGW", Table::RANDOM, Table::DEFAULT_LINEAR));

    SimpleTable::init("SGWFN", item, tableID);
}

const TableColumn&
SgwfnTable::getSgColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
SgwfnTable::getKrgColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
SgwfnTable::getKrgwColumn() const
{
    return SimpleTable::getColumn(2);
}

const TableColumn&
SgwfnTable::getPcgwColumn() const
{
    return SimpleTable::getColumn(3);
}

SgofTable::SgofTable(const DeckItem& item, const bool jfunc, const int tableID)
{
    m_schema.addColumn(ColumnSchema("SG", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("KRG", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("KROG", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("PCOG", Table::RANDOM, Table::DEFAULT_LINEAR));

    m_jfunc = jfunc;
    SimpleTable::init("SGOF", item, tableID);
}

const TableColumn&
SgofTable::getSgColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
SgofTable::getKrgColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
SgofTable::getKrogColumn() const
{
    return SimpleTable::getColumn(2);
}

const TableColumn&
SgofTable::getPcogColumn() const
{
    SimpleTable::assertJFuncPressure(false);
    return SimpleTable::getColumn(3);
}

const TableColumn&
SgofTable::getJFuncColumn() const
{
    SimpleTable::assertJFuncPressure(true);
    return SimpleTable::getColumn(3);
}

SlgofTable::SlgofTable(const DeckItem& item, const bool jfunc, const int tableID)
{
    m_schema.addColumn(ColumnSchema("SL", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("KRG", Table::DECREASING, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("KROG", Table::INCREASING, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("PCOG", Table::DECREASING, Table::DEFAULT_LINEAR));

    m_jfunc = jfunc;
    SimpleTable::init("SLGOF", item, tableID);

    if (getSlColumn().back() != 1.0) {
        throw std::invalid_argument("The last saturation of the SLGOF keyword must be 1!");
    }
}

const TableColumn&
SlgofTable::getSlColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
SlgofTable::getKrgColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
SlgofTable::getKrogColumn() const
{
    return SimpleTable::getColumn(2);
}

const TableColumn&
SlgofTable::getPcogColumn() const
{
    SimpleTable::assertJFuncPressure(false);
    return SimpleTable::getColumn(3);
}

const TableColumn&
SlgofTable::getJFuncColumn() const
{
    SimpleTable::assertJFuncPressure(true);
    return SimpleTable::getColumn(3);
}

Sof2Table::Sof2Table(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("SO", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("KRO", Table::INCREASING, Table::DEFAULT_LINEAR));

    SimpleTable::init("SOF2", item, tableID);
}

const TableColumn&
Sof2Table::getSoColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
Sof2Table::getKroColumn() const
{
    return SimpleTable::getColumn(1);
}

Sof3Table::Sof3Table(const DeckItem& item, const int tableID)
{

    m_schema.addColumn(ColumnSchema("SO", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("KROW", Table::INCREASING, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("KROG", Table::INCREASING, Table::DEFAULT_LINEAR));

    SimpleTable::init("SOF3", item, tableID);
}


const TableColumn&
Sof3Table::getSoColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
Sof3Table::getKrowColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
Sof3Table::getKrogColumn() const
{
    return SimpleTable::getColumn(2);
}

namespace {

    DeckItem paddedPVDGTable(const SimpleTable& padding,
                             const DeckItem&    pvdgTableInput,
                             const PvdgTable&   pvdgTable)
    {
        auto paddedTable = pvdgTableInput.emptyStructuralCopy();

        auto addTableValue = [&paddedTable,
                              &dim = pvdgTableInput.getActiveDimensions(),
                              n = std::size_t{0}]
            (const double value) mutable
        {
            paddedTable.push_back(dim[n].convertSiToRaw(value));
            n = (n + 1) % dim.size();
        };

        // Padding
        {
            const auto columns = std::array {
                &padding.getColumn("PG"),
                &padding.getColumn("BG"),
                &padding.getColumn("MUG"),
            };

            const auto nrow = padding.numRows();
            for (auto row = 0*nrow; row < nrow; ++row) {
                for (auto* column : columns) {
                    addTableValue((*column)[row]);
                }
            }
        }

        // Original table
        {
            const auto nrow = pvdgTable.numRows();
            const auto ncol = pvdgTable.numColumns();
            for (auto row = 0*nrow; row < nrow; ++row) {
                for (auto col = 0*ncol; col < ncol; ++col) {
                    // Yes, argument order is  (col, row)
                    addTableValue(pvdgTable.get(col, row));
                }
            }
        }

        return paddedTable;
    }

} // Anonymous namespace

PvdgTable::PvdgTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("P", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("BG", Table::STRICTLY_DECREASING, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("MUG", Table::INCREASING, Table::DEFAULT_LINEAR));

    // Run full table initialisation first.  The downside to this is that we
    // will end up throwing away and redoing that work if the table needs
    // padding at low pressure values.  On the other hand, the full table
    // initialisation procedure also checks consistency and replaces any
    // defaulted values which means we don't have to have special logic in
    // place to handle those complexities if the table does need padding.
    const auto tableName = std::string { "PVDG" };

    SimpleTable::init(tableName, item, tableID);

    if (this->numRows() <= 1) {
        // At most a single pressure node in the input PVDG data.  There is
        // not enough information to perform table padding, even if it might
        // be needed.  We might for instance be running in the context of a
        // unit test with incomplete data or the input might just be very
        // sparse.  In any case we can't perform table padding so just
        // return here.
        return;
    }

    if (const auto tablePadding = LowPressureTablePadding { DryGasTable { *this } };
        tablePadding.inputNeedsPadding())
    {
        SimpleTable::init(tableName, paddedPVDGTable(tablePadding.padding(), item, *this), tableID);
    }
}

const TableColumn&
PvdgTable::getPressureColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
PvdgTable::getFormationFactorColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
PvdgTable::getViscosityColumn() const
{
    return SimpleTable::getColumn(2);
}

PvdoTable::PvdoTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("P", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("BO", Table::STRICTLY_DECREASING, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("MUO", Table::INCREASING, Table::DEFAULT_LINEAR));

    SimpleTable::init("PVDO", item, tableID);
}


const TableColumn&
PvdoTable::getPressureColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
PvdoTable::getFormationFactorColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
PvdoTable::getViscosityColumn() const
{
    return SimpleTable::getColumn(2);
}

SwfnTable::SwfnTable(const DeckItem& item, const bool jfunc, const int tableID)
{
    m_schema.addColumn(ColumnSchema("SW", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("KRW", Table::INCREASING, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("PCOW", Table::DECREASING, Table::DEFAULT_LINEAR));

    m_jfunc = jfunc;
    SimpleTable::init("SWFN", item, tableID);
}

const TableColumn&
SwfnTable::getSwColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
SwfnTable::getKrwColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
SwfnTable::getPcowColumn() const
{
    SimpleTable::assertJFuncPressure(false);
    return SimpleTable::getColumn(2);
}

const TableColumn&
SwfnTable::getJFuncColumn() const
{
    SimpleTable::assertJFuncPressure(true);
    return SimpleTable::getColumn(2);
}


SgfnTable::SgfnTable(const DeckItem& item, const bool jfunc, const int tableID)
{
    m_schema.addColumn(ColumnSchema("SG", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("KRG", Table::INCREASING, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("PCOG", Table::INCREASING, Table::DEFAULT_LINEAR));

    m_jfunc = jfunc;
    SimpleTable::init("SGFN", item, tableID);
}


const TableColumn&
SgfnTable::getSgColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
SgfnTable::getKrgColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
SgfnTable::getPcogColumn() const
{
    SimpleTable::assertJFuncPressure(false);
    return SimpleTable::getColumn(2);
}

const TableColumn&
SgfnTable::getJFuncColumn() const
{
    SimpleTable::assertJFuncPressure(true);
    return SimpleTable::getColumn(2);
}

GsfTable::GsfTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("SG", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("KRG", Table::INCREASING, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("PCGW", Table::INCREASING, Table::DEFAULT_LINEAR));

    SimpleTable::init("GSF", item, tableID);
}


const TableColumn&
GsfTable::getSgColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
GsfTable::getKrgColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
GsfTable::getPcgwColumn() const
{
    return SimpleTable::getColumn(2);
}

WsfTable::WsfTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("SW", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("KRW", Table::INCREASING, Table::DEFAULT_LINEAR));

    SimpleTable::init("WSF", item, tableID);
}


const TableColumn&
WsfTable::getSwColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
WsfTable::getKrwColumn() const
{
    return SimpleTable::getColumn(1);
}

SsfnTable::SsfnTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("SolventFraction", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("GasRelPermMultiplier", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("SolventRelPermMultiplier", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));

    SimpleTable::init("SSFN", item, tableID);
}

const TableColumn&
SsfnTable::getSolventFractionColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
SsfnTable::getGasRelPermMultiplierColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
SsfnTable::getSolventRelPermMultiplierColumn() const
{
    return SimpleTable::getColumn(2);
}

PvdsTable::PvdsTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("P", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("BG", Table::STRICTLY_DECREASING, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("MUG", Table::INCREASING, Table::DEFAULT_LINEAR));

    SimpleTable::init("PVDS", item, tableID);
}

const TableColumn&
PvdsTable::getPressureColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
PvdsTable::getFormationFactorColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
PvdsTable::getViscosityColumn() const
{
    return SimpleTable::getColumn(2);
}

PlyadsTable::PlyadsTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("PolymerConcentration", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("AdsorbedPolymer", Table::INCREASING, Table::DEFAULT_NONE));

    SimpleTable::init("PLYADS", item, tableID);
}


const TableColumn&
PlyadsTable::getPolymerConcentrationColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
PlyadsTable::getAdsorbedPolymerColumn() const
{
    return SimpleTable::getColumn(1);
}

FoamadsTable::FoamadsTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("FoamConcentration", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("AdsorbedFoam", Table::INCREASING, Table::DEFAULT_NONE));

    SimpleTable::init("FOAMADS", item, tableID);
}

const TableColumn&
FoamadsTable::getFoamConcentrationColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
FoamadsTable::getAdsorbedFoamColumn() const
{
    return SimpleTable::getColumn(1);
}

FoammobTable::FoammobTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("FoamConcentration", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("MobilityMultiplier", Table::DECREASING, Table::DEFAULT_NONE));

    SimpleTable::init("FOAMMOB", item, tableID);
}

const TableColumn&
FoammobTable::getFoamConcentrationColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
FoammobTable::getMobilityMultiplierColumn() const
{
    return SimpleTable::getColumn(1);
}

PlymaxTable::PlymaxTable(const DeckRecord& record)
{
    m_schema.addColumn(ColumnSchema("C_POLYMER", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("C_POLYMER_MAX", Table::RANDOM, Table::DEFAULT_NONE));

    addColumns();
    for (size_t colIdx = 0; colIdx < record.size(); colIdx++) {
        auto& column = getColumn(colIdx);

        column.addValue(record.getItem(colIdx).getSIDouble(0), "PLYMAX");
    }
}

const TableColumn&
PlymaxTable::getPolymerConcentrationColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
PlymaxTable::getMaxPolymerConcentrationColumn() const
{
    return SimpleTable::getColumn(1);
}

PlyrockTable::PlyrockTable(const DeckRecord& record)
{
    m_schema.addColumn(ColumnSchema("DeadPoreVolume", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("ResidualResistanceFactor", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("RockDensityFactor", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("AdsorbtionIndex", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("MaxAdsorbtion", Table::RANDOM, Table::DEFAULT_NONE));

    addColumns();
    for (size_t colIdx = 0; colIdx < record.size(); colIdx++) {
        auto& column = getColumn(colIdx);

        column.addValue(record.getItem(colIdx).getSIDouble(0), "PLYROCK");
    }
}

const TableColumn&
PlyrockTable::getDeadPoreVolumeColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
PlyrockTable::getResidualResistanceFactorColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
PlyrockTable::getRockDensityFactorColumn() const
{
    return SimpleTable::getColumn(2);
}

const TableColumn&
PlyrockTable::getAdsorbtionIndexColumn() const
{
    return SimpleTable::getColumn(3);
}

const TableColumn&
PlyrockTable::getMaxAdsorbtionColumn() const
{
    return SimpleTable::getColumn(4);
}

PlyviscTable::PlyviscTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("PolymerConcentration", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("ViscosityMultiplier", Table::INCREASING, Table::DEFAULT_NONE));
    SimpleTable::init("PLYVISC", item, tableID);
}

const TableColumn&
PlyviscTable::getPolymerConcentrationColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
PlyviscTable::getViscosityMultiplierColumn() const
{
    return SimpleTable::getColumn(1);
}

PlydhflfTable::PlydhflfTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("Temperature", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("PolymerHalflife", Table::STRICTLY_DECREASING, Table::DEFAULT_NONE));

    SimpleTable::init("PLYDHFLFT", item, tableID);
}

const TableColumn&
PlydhflfTable::getTemperatureColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
PlydhflfTable::getPolymerHalflifeColumn() const
{
    return SimpleTable::getColumn(1);
}

PlyshlogTable::PlyshlogTable(const DeckRecord& indexRecord, const DeckRecord& dataRecord)
{

    {
        const auto& item = indexRecord.getItem<ParserKeywords::PLYSHLOG::REF_POLYMER_CONCENTRATION>();
        setRefPolymerConcentration(item.get<double>(0));
    }

    {
        const auto& item = indexRecord.getItem<ParserKeywords::PLYSHLOG::REF_SALINITY>();
        if (item.hasValue(0)) {
            setHasRefSalinity(true);
            setRefSalinity(item.get<double>(0));
        } else
            setHasRefSalinity(false);
    }

    {
        const auto& item = indexRecord.getItem<ParserKeywords::PLYSHLOG::REF_TEMPERATURE>();
        if (item.hasValue(0)) {
            setHasRefTemperature(true);
            setRefTemperature(item.get<double>(0));
        } else
            setHasRefTemperature(false);
    }

    m_schema.addColumn(ColumnSchema("WaterVelocity", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("ShearMultiplier", Table::RANDOM, Table::DEFAULT_NONE));

    SimpleTable::init("PLYSHLOG", dataRecord.getItem<ParserKeywords::PLYSHLOG::DATA>(), 1);
}

PlyshlogTable
PlyshlogTable::serializationTestObject()
{
    PlyshlogTable result;
    static_cast<SimpleTable&>(result) = SimpleTable::serializationTestObject();
    result.m_refPolymerConcentration = 1.0;
    result.m_refSalinity = 2.0;
    result.m_refTemperature = 3.0;
    result.m_hasRefSalinity = true;
    result.m_hasRefTemperature = true;

    return result;
}

double
PlyshlogTable::getRefPolymerConcentration() const
{
    return m_refPolymerConcentration;
}
double
PlyshlogTable::getRefSalinity() const
{
    return m_refSalinity;
}

double
PlyshlogTable::getRefTemperature() const
{
    return m_refTemperature;
}

void
PlyshlogTable::setRefPolymerConcentration(const double refPlymerConcentration)
{
    m_refPolymerConcentration = refPlymerConcentration;
}

void
PlyshlogTable::setRefSalinity(const double refSalinity)
{
    m_refSalinity = refSalinity;
}

void
PlyshlogTable::setRefTemperature(const double refTemperature)
{
    m_refTemperature = refTemperature;
}

bool
PlyshlogTable::hasRefSalinity() const
{
    return m_hasRefSalinity;
}

bool
PlyshlogTable::hasRefTemperature() const
{
    return m_hasRefTemperature;
}

void
PlyshlogTable::setHasRefSalinity(const bool has)
{
    m_hasRefSalinity = has;
}

void
PlyshlogTable::setHasRefTemperature(const bool has)
{
    m_hasRefTemperature = has;
}

const TableColumn&
PlyshlogTable::getWaterVelocityColumn() const
{
    return getColumn(0);
}

const TableColumn&
PlyshlogTable::getShearMultiplierColumn() const
{
    return getColumn(1);
}

bool
PlyshlogTable::operator==(const PlyshlogTable& data) const
{
    return this->SimpleTable::operator==(data) && m_refPolymerConcentration == data.m_refPolymerConcentration
        && m_refSalinity == data.m_refSalinity && m_refTemperature == data.m_refTemperature
        && m_hasRefSalinity == data.m_hasRefSalinity && m_hasRefTemperature == data.m_hasRefTemperature;
}

OilvisctTable::OilvisctTable(const DeckItem& item, const int tableID)
{

    m_schema.addColumn(ColumnSchema("Temperature", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("Viscosity", Table::DECREASING, Table::DEFAULT_NONE));
    SimpleTable::init("OILVISCT", item, tableID);
}

const TableColumn&
OilvisctTable::getTemperatureColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
OilvisctTable::getOilViscosityColumn() const
{
    return SimpleTable::getColumn(1);
}

WatvisctTable::WatvisctTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("Temperature", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("Viscosity", Table::DECREASING, Table::DEFAULT_NONE));

    SimpleTable::init("WATVISCT", item, tableID);
}

const TableColumn&
WatvisctTable::getTemperatureColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
WatvisctTable::getWaterViscosityColumn() const
{
    return SimpleTable::getColumn(1);
}

GasvisctTable::GasvisctTable(const DeckItem& item, const int tableID)
{

    m_schema.addColumn(ColumnSchema("Temperature", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("Viscosity", Table::RANDOM, Table::DEFAULT_NONE));
    SimpleTable::init("GASVISCT", item, tableID);
}

const TableColumn&
GasvisctTable::getTemperatureColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
GasvisctTable::getGasViscosityColumn() const
{
    return SimpleTable::getColumn(1);
}

RtempvdTable::RtempvdTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("Depth", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("Temperature", Table::RANDOM, Table::DEFAULT_NONE));
    SimpleTable::init("RTEMPVD", item, tableID);
}

const TableColumn&
RtempvdTable::getDepthColumn() const
{
    return SimpleTable::getColumn(0);
}


const TableColumn&
RtempvdTable::getTemperatureColumn() const
{
    return SimpleTable::getColumn(1);
}

RocktabTable::RocktabTable(const DeckItem& item, bool isDirectional, bool hasStressOption, const int tableID)
    : m_isDirectional(isDirectional)
{

    Table::ColumnOrderEnum POOrder;

    if (!hasStressOption)
        POOrder = Table::STRICTLY_INCREASING;
    else
        POOrder = Table::STRICTLY_DECREASING;

    m_schema.addColumn(ColumnSchema("PO", POOrder, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("PV_MULT", Table::RANDOM, Table::DEFAULT_LINEAR));

    if (isDirectional) {
        m_schema.addColumn(ColumnSchema("PV_MULT_TRANX", Table::RANDOM, Table::DEFAULT_LINEAR));
        m_schema.addColumn(ColumnSchema("PV_MULT_TRANY", Table::RANDOM, Table::DEFAULT_LINEAR));
        m_schema.addColumn(ColumnSchema("PV_MULT_TRANZ", Table::RANDOM, Table::DEFAULT_LINEAR));
    } else
        m_schema.addColumn(ColumnSchema("PV_MULT_TRAN", Table::RANDOM, Table::DEFAULT_LINEAR));

    SimpleTable::init("ROCKTAB", item, tableID);
}

RocktabTable
RocktabTable::serializationTestObject()
{
    RocktabTable result;
    static_cast<SimpleTable&>(result) = Opm::SimpleTable::serializationTestObject();
    result.m_isDirectional = true;

    return result;
}

const TableColumn&
RocktabTable::getPressureColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
RocktabTable::getPoreVolumeMultiplierColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
RocktabTable::getTransmissibilityMultiplierColumn() const
{
    return SimpleTable::getColumn(2);
}

const TableColumn&
RocktabTable::getTransmissibilityMultiplierXColumn() const
{
    return SimpleTable::getColumn(2);
}

const TableColumn&
RocktabTable::getTransmissibilityMultiplierYColumn() const
{
    if (!m_isDirectional)
        return SimpleTable::getColumn(2);
    return SimpleTable::getColumn(3);
}

const TableColumn&
RocktabTable::getTransmissibilityMultiplierZColumn() const
{
    if (!m_isDirectional)
        return SimpleTable::getColumn(2);
    return SimpleTable::getColumn(4);
}

bool
RocktabTable::operator==(const RocktabTable& data) const
{
    return this->SimpleTable::operator==(data) && m_isDirectional == data.m_isDirectional;
}

RsvdTable::RsvdTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("DEPTH", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("RS", Table::RANDOM, Table::DEFAULT_NONE));

    SimpleTable::init("RSVD", item, tableID);
}

const TableColumn&
RsvdTable::getDepthColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
RsvdTable::getRsColumn() const
{
    return SimpleTable::getColumn(1);
}

RvvdTable::RvvdTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("DEPTH", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("RV", Table::RANDOM, Table::DEFAULT_LINEAR));
    SimpleTable::init("RVVD", item, tableID);
}

const TableColumn&
RvvdTable::getDepthColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
RvvdTable::getRvColumn() const
{
    return SimpleTable::getColumn(1);
}

RvwvdTable::RvwvdTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("DEPTH", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("RVWVD", Table::RANDOM, Table::DEFAULT_LINEAR));
    SimpleTable::init("RVWVD", item, tableID);
}

const TableColumn&
RvwvdTable::getDepthColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
RvwvdTable::getRvwvdColumn() const
{
    return SimpleTable::getColumn(1);
}

PbvdTable::PbvdTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("DEPTH", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("PBUB", Table::RANDOM, Table::DEFAULT_NONE));

    SimpleTable::init("PBVD", item, tableID);
}

const TableColumn&
PbvdTable::getDepthColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
PbvdTable::getPbubColumn() const
{
    return SimpleTable::getColumn(1);
}

PdvdTable::PdvdTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("DEPTH", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("PDEW", Table::RANDOM, Table::DEFAULT_NONE));

    SimpleTable::init("PDVD", item, tableID);
}

const TableColumn&
PdvdTable::getDepthColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
PdvdTable::getPdewColumn() const
{
    return SimpleTable::getColumn(1);
}

SaltvdTable::SaltvdTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("DEPTH", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("SALT", Table::RANDOM, Table::DEFAULT_NONE));

    SimpleTable::init("SALTVD", item, tableID);
}

const TableColumn&
SaltvdTable::getDepthColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
SaltvdTable::getSaltColumn() const
{
    return SimpleTable::getColumn(1);
}

SaltpvdTable::SaltpvdTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("DEPTH", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("SALTP", Table::RANDOM, Table::DEFAULT_NONE));

    SimpleTable::init("SALTPVD", item, tableID);
}

const TableColumn&
SaltpvdTable::getDepthColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
SaltpvdTable::getSaltpColumn() const
{
    return SimpleTable::getColumn(1);
}


SaltsolTable::SaltsolTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("SALTSOLUBILITY", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("SALTDENSITY", Table::RANDOM, Table::DEFAULT_NONE));

    SimpleTable::init("SALTSOL", item, tableID);
}

const TableColumn&
SaltsolTable::getSaltsolColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
SaltsolTable::getSaltdenColumn() const
{
    return SimpleTable::getColumn(1);
}


PermfactTable::PermfactTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("POROSITYCHANGE", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("PERMEABILITYMULTIPLIER", Table::RANDOM, Table::DEFAULT_NONE));

    SimpleTable::init("PERMFACT", item, tableID);
}

const TableColumn&
PermfactTable::getPorosityChangeColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
PermfactTable::getPermeabilityMultiplierColumn() const
{
    return SimpleTable::getColumn(1);
}

PcfactTable::PcfactTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("POROSITYCHANGE", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("PCMULTIPLIER", Table::RANDOM, Table::DEFAULT_NONE));

    SimpleTable::init("PCFACT", item, tableID);
}

const TableColumn&
PcfactTable::getPorosityChangeColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
PcfactTable::getPcMultiplierColumn() const
{
    return SimpleTable::getColumn(1);
}

BiofilmTable::BiofilmTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("DENSITY_BIOFILM", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("MICROBIAL_DEATH_RATE", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("MAXIMUM_GROWTH_RATE", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("HALF_VELOCITY_OXYGEN", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("YIELD_GROWTH_COEFFICIENT", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("OXYGEN_CONSUMPTION_FACTOR", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("MICROBIAL_ATTACHMENT_RATE", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("DETACHMENT_RATE", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("DETACHMENT_EXPONENT", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("MAXIMUM_UREA_UTILIZATION", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("HALF_VELOCITY_UREA", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("DENSITY_CALCITE", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("YIELD_UREA_TO_CALCITE_COEFFICIENT", Table::RANDOM, Table::DEFAULT_NONE));

    SimpleTable::init("BIOFPARA", item, tableID);
}

const TableColumn&
BiofilmTable::getDensityBiofilm() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
BiofilmTable::getMicrobialDeathRate() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
BiofilmTable::getMaximumGrowthRate() const
{
    return SimpleTable::getColumn(2);
}

const TableColumn&
BiofilmTable::getHalfVelocityOxygen() const
{
    return SimpleTable::getColumn(3);
}

const TableColumn&
BiofilmTable::getYieldGrowthCoefficient() const
{
    return SimpleTable::getColumn(4);
}

const TableColumn&
BiofilmTable::getOxygenConsumptionFactor() const
{
    return SimpleTable::getColumn(5);
}

const TableColumn&
BiofilmTable::getMicrobialAttachmentRate() const
{
    return SimpleTable::getColumn(6);
}

const TableColumn&
BiofilmTable::getDetachmentRate() const
{
    return SimpleTable::getColumn(7);
}

const TableColumn&
BiofilmTable::getDetachmentExponent() const
{
    return SimpleTable::getColumn(8);
}

const TableColumn&
BiofilmTable::getMaximumUreaUtilization() const
{
    return SimpleTable::getColumn(9);
}

const TableColumn&
BiofilmTable::getHalfVelocityUrea() const
{
    return SimpleTable::getColumn(10);
}

const TableColumn&
BiofilmTable::getDensityCalcite() const
{
    return SimpleTable::getColumn(11);
}

const TableColumn&
BiofilmTable::getYieldUreaToCalciteCoefficient() const
{
    return SimpleTable::getColumn(12);
}

DiffMICPTable::DiffMICPTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("MICROBIAL_DIFFUSION", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("OXYGEN_DIFFUSION", Table::RANDOM, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("UREA_DIFFUSION", Table::RANDOM, Table::DEFAULT_NONE));

    SimpleTable::init("DIFFMICP", item, tableID);
}

const TableColumn&
DiffMICPTable::getMicrobialDiffusion() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
DiffMICPTable::getOxygenDiffusion() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
DiffMICPTable::getUreaDiffusion() const
{
    return SimpleTable::getColumn(2);
}

AqutabTable::AqutabTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("TD", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("PD", Table::RANDOM, Table::DEFAULT_LINEAR));
    SimpleTable::init("AQUTAB", item, tableID);
}

const TableColumn&
AqutabTable::getTimeColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
AqutabTable::getPressureColumn() const
{
    return SimpleTable::getColumn(1);
}

EnkrvdTable::EnkrvdTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("DEPTH", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("KRWMAX", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("KRGMAX", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("KROMAX", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("KRWCRIT", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("KRGCRIT", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("KROCRITG", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("KROCRITW", Table::RANDOM, Table::DEFAULT_LINEAR));

    SimpleTable::init("ENKRVD", item, tableID);
}

const TableColumn&
EnkrvdTable::getDepthColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
EnkrvdTable::getKrwmaxColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
EnkrvdTable::getKrgmaxColumn() const
{
    return SimpleTable::getColumn(2);
}

const TableColumn&
EnkrvdTable::getKromaxColumn() const
{
    return SimpleTable::getColumn(3);
}

const TableColumn&
EnkrvdTable::getKrwcritColumn() const
{
    return SimpleTable::getColumn(4);
}

const TableColumn&
EnkrvdTable::getKrgcritColumn() const
{
    return SimpleTable::getColumn(5);
}

const TableColumn&
EnkrvdTable::getKrocritgColumn() const
{
    return SimpleTable::getColumn(6);
}

const TableColumn&
EnkrvdTable::getKrocritwColumn() const
{
    return SimpleTable::getColumn(7);
}

EnptvdTable::EnptvdTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("DEPTH", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("SWCO", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("SWCRIT", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("SWMAX", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("SGCO", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("SGCRIT", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("SGMAX", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("SOWCRIT", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("SOGCRIT", Table::RANDOM, Table::DEFAULT_LINEAR));

    SimpleTable::init("ENPTVD", item, tableID);
}

const TableColumn&
EnptvdTable::getDepthColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
EnptvdTable::getSwcoColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
EnptvdTable::getSwcritColumn() const
{
    return SimpleTable::getColumn(2);
}

const TableColumn&
EnptvdTable::getSwmaxColumn() const
{
    return SimpleTable::getColumn(3);
}

const TableColumn&
EnptvdTable::getSgcoColumn() const
{
    return SimpleTable::getColumn(4);
}

const TableColumn&
EnptvdTable::getSgcritColumn() const
{
    return SimpleTable::getColumn(5);
}

const TableColumn&
EnptvdTable::getSgmaxColumn() const
{
    return SimpleTable::getColumn(6);
}

const TableColumn&
EnptvdTable::getSowcritColumn() const
{
    return SimpleTable::getColumn(7);
}

const TableColumn&
EnptvdTable::getSogcritColumn() const
{
    return SimpleTable::getColumn(8);
}

ImkrvdTable::ImkrvdTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("DEPTH", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("KRWMAX", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("KRGMAX", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("KROMAX", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("KRWCRIT", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("KRGCRIT", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("KROCRITG", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("KROCRITW", Table::RANDOM, Table::DEFAULT_LINEAR));

    SimpleTable::init("IMKRVD", item, tableID);
}

const TableColumn&
ImkrvdTable::getDepthColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
ImkrvdTable::getKrwmaxColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
ImkrvdTable::getKrgmaxColumn() const
{
    return SimpleTable::getColumn(2);
}

const TableColumn&
ImkrvdTable::getKromaxColumn() const
{
    return SimpleTable::getColumn(3);
}

const TableColumn&
ImkrvdTable::getKrwcritColumn() const
{
    return SimpleTable::getColumn(4);
}

const TableColumn&
ImkrvdTable::getKrgcritColumn() const
{
    return SimpleTable::getColumn(5);
}

const TableColumn&
ImkrvdTable::getKrocritgColumn() const
{
    return SimpleTable::getColumn(6);
}

const TableColumn&
ImkrvdTable::getKrocritwColumn() const
{
    return SimpleTable::getColumn(7);
}


ImptvdTable::ImptvdTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("DEPTH", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("SWCO", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("SWCRIT", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("SWMAX", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("SGCO", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("SGCRIT", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("SGMAX", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("SOWCRIT", Table::RANDOM, Table::DEFAULT_LINEAR));
    m_schema.addColumn(ColumnSchema("SOGCRIT", Table::RANDOM, Table::DEFAULT_LINEAR));

    SimpleTable::init("IMPVTD", item, tableID);
}

const TableColumn&
ImptvdTable::getDepthColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
ImptvdTable::getSwcoColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
ImptvdTable::getSwcritColumn() const
{
    return SimpleTable::getColumn(2);
}

const TableColumn&
ImptvdTable::getSwmaxColumn() const
{
    return SimpleTable::getColumn(3);
}

const TableColumn&
ImptvdTable::getSgcoColumn() const
{
    return SimpleTable::getColumn(4);
}

const TableColumn&
ImptvdTable::getSgcritColumn() const
{
    return SimpleTable::getColumn(5);
}

const TableColumn&
ImptvdTable::getSgmaxColumn() const
{
    return SimpleTable::getColumn(6);
}

const TableColumn&
ImptvdTable::getSowcritColumn() const
{
    return SimpleTable::getColumn(7);
}

const TableColumn&
ImptvdTable::getSogcritColumn() const
{
    return SimpleTable::getColumn(8);
}

SorwmisTable::SorwmisTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("WaterSaturation", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("MiscibleResidualOilSaturation", Table::INCREASING, Table::DEFAULT_NONE));

    SimpleTable::init("SORWMIS", item, tableID);
}

const TableColumn&
SorwmisTable::getWaterSaturationColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
SorwmisTable::getMiscibleResidualOilColumn() const
{
    return SimpleTable::getColumn(1);
}

SgcwmisTable::SgcwmisTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("WaterSaturation", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("MiscibleResidualGasSaturation", Table::INCREASING, Table::DEFAULT_NONE));
    SimpleTable::init("SGCWMIS", item, tableID);
}

const TableColumn&
SgcwmisTable::getWaterSaturationColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
SgcwmisTable::getMiscibleResidualGasColumn() const
{
    return SimpleTable::getColumn(1);
}

MiscTable::MiscTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("SolventFraction", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("Miscibility", Table::INCREASING, Table::DEFAULT_NONE));
    SimpleTable::init("MISC", item, tableID);
}

const TableColumn&
MiscTable::getSolventFractionColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
MiscTable::getMiscibilityColumn() const
{
    return SimpleTable::getColumn(1);
}


PmiscTable::PmiscTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("OilPhasePressure", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("Miscibility", Table::INCREASING, Table::DEFAULT_NONE));
    SimpleTable::init("PMISC", item, tableID);
}

const TableColumn&
PmiscTable::getOilPhasePressureColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
PmiscTable::getMiscibilityColumn() const
{
    return SimpleTable::getColumn(1);
}

TlpmixpaTable::TlpmixpaTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("OilPhasePressure", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("Miscibility", Table::INCREASING, Table::DEFAULT_NONE));
    SimpleTable::init("TLPMIXPA", item, tableID);
}

const TableColumn&
TlpmixpaTable::getOilPhasePressureColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
TlpmixpaTable::getMiscibilityColumn() const
{
    return SimpleTable::getColumn(1);
}

MsfnTable::MsfnTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("GasPhaseFraction", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("GasSolventRelpermMultiplier", Table::INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("OilRelpermMultiplier", Table::DECREASING, Table::DEFAULT_NONE));

    SimpleTable::init("MSFN", item, tableID);

    getColumn("GasPhaseFraction").assertUnitRange();
}


const TableColumn&
MsfnTable::getGasPhaseFractionColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
MsfnTable::getGasSolventRelpermMultiplierColumn() const
{
    return SimpleTable::getColumn(1);
}

const TableColumn&
MsfnTable::getOilRelpermMultiplierColumn() const
{
    return SimpleTable::getColumn(2);
}

RockwnodTable::RockwnodTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("Saturation", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    SimpleTable::init("ROCKWNOD", item, tableID);
}

const TableColumn&
RockwnodTable::getSaturationColumn() const
{
    return SimpleTable::getColumn(0);
}

OverburdTable::OverburdTable(const DeckItem& item, const int tableID)
{
    m_schema.addColumn(ColumnSchema("Depth", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(ColumnSchema("OverburdenPressure", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    SimpleTable::init("OVERBURD", item, tableID);
}

const TableColumn&
OverburdTable::getDepthColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
OverburdTable::getOverburdenPressureColumn() const
{
    return SimpleTable::getColumn(1);
}

TracerVdTable::TracerVdTable(const Opm::DeckItem& item, double inv_volume, const int tableID)
{
    m_schema.addColumn(Opm::ColumnSchema("DEPTH", Table::STRICTLY_INCREASING, Table::DEFAULT_NONE));
    m_schema.addColumn(Opm::ColumnSchema("TRACER_CONCENTRATION", Table::RANDOM, Table::DEFAULT_NONE));
    SimpleTable::init("TRACERVD", item, tableID, inv_volume);
}

const TableColumn&
TracerVdTable::getDepthColumn() const
{
    return SimpleTable::getColumn(0);
}

const TableColumn&
TracerVdTable::getTracerConcentration() const
{
    return SimpleTable::getColumn(1);
}

namespace
{
    /*
     * Create a compile-time sequence of integers [0,N). In C++14 this can be
     * replaced by std::index_sequence.
     */
    template <std::size_t...>
    struct seq {
        using type = seq;
    };

    template <std::size_t N, std::size_t... Is>
    struct mkseq : mkseq<N - 1, N - 1, Is...> {
    };

    template <std::size_t... Is>
    struct mkseq<0u, Is...> : seq<Is...> {
        using type = seq<Is...>;
    };

    // Convenince function for creating a 'flat table', e.g. PVTW and
    // DENSITY.  Assumes the following:
    //
    // 1. The table has vector semantics with no other to enforce
    // 2. That the following struct is implemented:
    // struct record {
    //  static constexpr std::size_t size = [number-of-members]
    //  double members ...;
    // };
    // 3. The table is declared as
    // struct table : public FlatTable< table > {
    //  using FlatTable< table >::FlatTable;
    // }
    //
    // If some field can *not* be defaulted, e.g. 0, specialise the
    // flat_props struct (in this namespace) as such:
    // template<> struct<record, 0> {
    //  static constexpr bool can_default() { return false; }
    //  static constexpr const char* errmsg() { "error message"; }
    // };
    // and the parser will throw std::invalid_argument if the field is
    // defaulted in the input.

    template <typename T, std::size_t N>
    struct flat_props
    {
        static constexpr bool can_default()
        {
            return true;
        }

        static constexpr const char* errmsg()
        {
            return "";
        }
    };

    template <typename T, std::size_t N>
    double flat_get(const DeckRecord& rec)
    {
        const auto& item = rec.getItem(N);

        if (item.defaultApplied(0) && !flat_props<T, N>::can_default()) {
            throw std::invalid_argument {
                flat_props<T, N>::errmsg()
            };
        }

        return item.getSIDouble(0);
    }

    template <typename T, std::size_t... Is>
    T flat_get(const DeckRecord& record, seq<Is...>)
    {
        return { flat_get<T, Is>(record)... };
    }

    template <typename T, std::size_t... Is>
    std::vector<T> flat_records(const DeckKeyword& kw, seq<Is...> s)
    {
        auto xs = std::vector<T> {};
        xs.reserve(kw.size());

        for (const auto& record : kw) {
            xs.push_back(flat_get<T>(record, s));
        }

        return xs;
    }

    template <>
    struct flat_props<PVTWRecord, 0>
    {
        static constexpr bool can_default()
        {
            return false;
        }

        static constexpr const char* errmsg()
        {
            return "PVTW reference pressure cannot be defaulted";
        }
    };

    constexpr const char* pvcdo_err[] = {
        "PVCDO reference pressure cannot be defaulted",
        "PVCDO oil volume factor cannot be defaulted",
        "PVCDO compressibility cannot be defaulted",
        "PVCDO viscosity cannot be defaulted",
        "PVCDO viscosibility cannot be defaulted",
    };

    template <std::size_t N>
    struct flat_props<PVCDORecord, N>
    {
        static constexpr bool can_default()
        {
            return false;
        }

        static constexpr const char* errmsg()
        {
            return pvcdo_err[N];
        }
    };

    bool all_defaulted(const DeckRecord& record)
    {
        return std::all_of(record.begin(), record.end(), [](const DeckItem& item) {
            const auto& vstat = item.getValueStatus();
            return std::all_of(vstat.begin(), vstat.end(), &value::defaulted);
        });
    }

} // Anonymous namespace

// ------------------------------------------------------------------------

template <typename RecordType>
FlatTableWithCopy<RecordType>::FlatTableWithCopy(const DeckKeyword& kw,
                                                 std::string_view   expect)
{
    if (!expect.empty() && (kw.name() != expect)) {
        throw std::invalid_argument {
            fmt::format("Keyword {} cannot be used to "
                        "initialise {} table structures",
                        kw.name(), expect)
        };
    }

    this->table_.reserve(kw.size());

    for (const auto& record : kw) {
        if (all_defaulted(record)) {
            // All-defaulted records imply table in region R is equal to
            // table in region R-1.  Table must not be defaulted in region 1
            // (i.e., when PVTNUM=1).
            if (this->table_.empty()) {
                throw OpmInputError {
                    "First record cannot be defaulted",
                    kw.location()
                };
            }

            this->table_.push_back(this->table_.back());
        }
        else {
            this->table_.push_back(flat_get<RecordType>(record, mkseq<RecordType::size>{}));
        }
    }
}

template <typename RecordType>
FlatTableWithCopy<RecordType>::
FlatTableWithCopy(std::initializer_list<RecordType> records)
    : table_{ records }
{}

// ------------------------------------------------------------------------

GravityTable::GravityTable(const DeckKeyword& kw)
    : FlatTableWithCopy(kw, ParserKeywords::GRAVITY::keywordName)
{}

GravityTable::GravityTable(std::initializer_list<GRAVITYRecord> records)
    : FlatTableWithCopy(records)
{}

// ------------------------------------------------------------------------

DensityTable::DensityTable(const DeckKeyword& kw)
    : FlatTableWithCopy(kw, ParserKeywords::DENSITY::keywordName)
{}

DensityTable::DensityTable(std::initializer_list<DENSITYRecord> records)
    : FlatTableWithCopy(records)
{}

DensityTable::DensityTable(const GravityTable& gravity)
{
    this->table_.reserve(gravity.size());

    constexpr auto default_air_density =
        1.22 * unit::kilogram / unit::cubic(unit::meter);

    constexpr auto default_water_density =
        1000.0 * unit::kilogram / unit::cubic(unit::meter);

    // Degrees API defined as
    //
    //   API = (141.5 / SG) - 131.5
    //
    // with SG being the specific gravity of oil relative to pure water.

    std::transform(gravity.begin(), gravity.end(),
                   std::back_inserter(this->table_),
        [](const GRAVITYRecord& record)
    {
        return DENSITYRecord {
            (141.5 / (record.oil_api + 131.5)) * default_water_density,
            record.water_sg * default_water_density,
            record.gas_sg * default_air_density
        };
    });
}

// ------------------------------------------------------------------------

PvtwTable::PvtwTable(const DeckKeyword& kw)
    : FlatTableWithCopy(kw, ParserKeywords::PVTW::keywordName)
{}

PvtwTable::PvtwTable(std::initializer_list<PVTWRecord> records)
    : FlatTableWithCopy(records)
{}

// ------------------------------------------------------------------------

RockTable::RockTable(const DeckKeyword& kw)
    : FlatTableWithCopy(kw, ParserKeywords::ROCK::keywordName)
{}

RockTable::RockTable(std::initializer_list<ROCKRecord> records)
    : FlatTableWithCopy(records)
{}

// ------------------------------------------------------------------------

template <typename T>
FlatTable<T>::FlatTable(const DeckKeyword& kw)
    : std::vector<T>(flat_records<T>(kw, mkseq<T::size>{}))
{}

template FlatTable<DiffCoeffRecord>::FlatTable(const DeckKeyword&);
template FlatTable<DiffCoeffWatRecord>::FlatTable(const DeckKeyword&);
template FlatTable<DiffCoeffGasRecord>::FlatTable(const DeckKeyword&);
template FlatTable<PVCDORecord>::FlatTable(const DeckKeyword&);
template FlatTable<PlmixparRecord>::FlatTable(const DeckKeyword&);
template FlatTable<PlyvmhRecord>::FlatTable(const DeckKeyword&);
template FlatTable<SatFuncLETRecord>::FlatTable(const DeckKeyword&);
template FlatTable<ShrateRecord>::FlatTable(const DeckKeyword&);
template FlatTable<Stone1exRecord>::FlatTable(const DeckKeyword&);
template FlatTable<TlmixparRecord>::FlatTable(const DeckKeyword&);
template FlatTable<VISCREFRecord>::FlatTable(const DeckKeyword&);
template FlatTable<WATDENTRecord>::FlatTable(const DeckKeyword&);

} // namespace Opm
