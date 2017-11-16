/*
  Copyright 2016 Statoil ASA.

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

#include <opm/output/eclipse/Tables.hpp>

#include <ert/ecl/FortIO.hpp>
#include <ert/ecl/EclKW.hpp>
#include <ert/ecl/ecl_kw_magic.h>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgfnTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SgofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Sof2Table.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Sof3Table.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SwfnTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/SwofTable.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/Tabdims.hpp>
#include <opm/parser/eclipse/EclipseState/Tables/TableContainer.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <opm/output/eclipse/LinearisedOutputTable.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <vector>

/// Functions to facilitate generating TAB vector entries for tabulated
/// saturation functions.
namespace { namespace SatFunc {
    namespace detail {
        /// Create linearised, padded TAB vector entries for a collection of
        /// tabulated saturation functions corresponding to a single input
        /// keyword.
        ///
        /// \tparam BuildDependent Callable entity that extracts the
        ///    independent and primary dependent variates of a single
        ///    saturation function table into a linearised output table.
        ///    Must implement a function call operator of the form
        ///    \code
        ///       std::size_t
        ///       operator()(const std::size_t      tableID,
        ///                  const std::size_t      primID,
        ///                  LinearisedOutputTable& table);
        ///    \endcode
        ///    that will assign the independent variate of the sub-table
        ///    primID within the table identified as tableID to column zero
        ///    of 'table' and all dependent variates to columns one &c of
        ///    'table'.  The function call operator must return the number
        ///    of active (used) rows within the sub-table through its return
        ///    value.
        ///
        /// \param[in] numTab Number of tables in this table collection.
        ///
        /// \param[in] numRows Number of rows to allocate for each padded
        ///    output table.
        ///
        /// \param[in] numDep Number of dependent variates (columns) in each
        ///    table of this collection of input tables.  Total number of
        ///    columns in the result vector will be 1 + 2*numDep to account
        ///    for the independent variate, the dependent variates and the
        ///    derivatives of the dependent variates with respect to the
        ///    independent variate.
        ///
        /// \param[in] buildDeps Function object that implements the
        ///    protocol outlined for \code BuildDependent::operator()()
        ///    \endcode.  Typically a lambda expression.
        ///
        /// \return Linearised, padded TAB vector entries for a collection
        ///    of tabulated saturation functions corresponding to a single
        ///    input keyword.  Derivatives included as additional columns.
        template <class BuildDependent>
        std::vector<double>
        createSatfuncTable(const std::size_t numTab,
                           const std::size_t numRows,
                           const std::size_t numDep,
                           BuildDependent&&  buildDeps)
        {
            const auto numPrim = std::size_t{1};
            const auto numCols = 1 + 2*numDep;

            auto descr = ::Opm::DifferentiateOutputTable::Descriptor{};
            descr.primID = 0 * numPrim;

            auto linTable = ::Opm::LinearisedOutputTable {
                numTab, numPrim, numRows, numCols
            };

            for (descr.tableID = 0*numTab;
                 descr.tableID < 1*numTab; ++descr.tableID)
            {
                descr.numActRows =
                    buildDeps(descr.tableID, descr.primID, linTable);

                // Derivatives.  Use values already stored in linTable to
                // take advantage of any unit conversion already applied.
                // We don't have to do anything special for the units here.
                //
                // Note: argument 'descr' implies argument-dependent lookup
                //    whence we unambiguously invoke function calcSlopes()
                //    from namespace ::Opm::DifferentiateOutputTable.
                calcSlopes(numDep, descr, linTable);
            }

            return linTable.getDataDestructively();
        }
    } // detail

    /// Functions to create linearised, padded, and normalised SGFN output
    /// tables from various input saturation function keywords.
    namespace Gas {
        /// Create linearised and padded 'TAB' vector entries of normalised
        /// SGFN tables for all saturation function regions from Family Two
        /// table data (SGFN keyword).
        ///
        /// \param[in] numRows Number of rows to allocate in the output
        ///    vector for each table.  Expected to be equal to the number of
        ///    declared saturation nodes in the simulation run's TABDIMS
        ///    keyword (Item 3).
        ///
        /// \param[in] units Active unit system.  Needed to convert SI
        ///    convention capillary pressure values (Pascal) to declared
        ///    conventions of the run specification.
        ///
        /// \param[in] sgfn Collection of SGFN tables for all saturation
        ///    regions.
        ///
        /// \return Linearised and padded 'TAB' vector values for output
        ///    SGFN tables.  A unit-converted copy of the input table \p
        ///    sgfn with added derivatives.
        std::vector<double>
        fromSGFN(const std::size_t          numRows,
                 const Opm::UnitSystem&     units,
                 const Opm::TableContainer& sgfn)
        {
            using SGFN = ::Opm::SgfnTable;

            const auto numTab = sgfn.size();
            const auto numDep = std::size_t{2}; // Krg, Pcgo

            return detail::createSatfuncTable(numTab, numRows, numDep,
                [&units, &sgfn](const std::size_t           tableID,
                                const std::size_t           primID,
                                Opm::LinearisedOutputTable& linTable)
                -> std::size_t
            {
                const auto& t = sgfn.getTable<SGFN>(tableID);

                auto numActRows = std::size_t{0};

                // Sg
                {
                    const auto& Sg = t.getSgColumn();

                    numActRows = Sg.size();
                    std::copy(std::begin(Sg), std::end(Sg),
                              linTable.column(tableID, primID, 0));
                }

                // Krg(Sg)
                {
                    const auto& kr = t.getKrgColumn();
                    std::copy(std::begin(kr), std::end(kr),
                              linTable.column(tableID, primID, 1));
                }

                // Pcgo(Sg)
                {
                    const auto uPress = ::Opm::UnitSystem::measure::pressure;

                    const auto& pc = t.getPcogColumn();
                    std::transform(std::begin(pc), std::end(pc),
                                   linTable.column(tableID, primID, 2),
                                   [&units, uPress](const double Pc) -> double
                                   {
                                       return units.from_si(uPress, Pc);
                                   });
                }

                // Inform createSatfuncTable() of number of active rows in
                // this table.  Needed to compute slopes of piecewise linear
                // interpolants.
                return numActRows;
            });
        }

        /// Create linearised and padded 'TAB' vector entries of normalised
        /// SGFN tables for all saturation function regions from Family One
        /// table data (SGOF keyword).
        ///
        /// \param[in] numRows Number of rows to allocate in the output
        ///    vector for each table.  Expected to be equal to the number of
        ///    declared saturation nodes in the simulation run's TABDIMS
        ///    keyword (Item 3).
        ///
        /// \param[in] units Active unit system.  Needed to convert SI
        ///    convention capillary pressure values (Pascal) to declared
        ///    conventions of the run specification.
        ///
        /// \param[in] swof Collection of SGOF tables for all saturation
        ///    regions.
        ///
        /// \return Linearised and padded 'TAB' vector values for output
        ///    SGFN tables.  Corresponds to unit-converted copies of columns
        ///    1, 2, and 4--with added derivatives--of the input SGOF tables.
        std::vector<double>
        fromSGOF(const std::size_t          numRows,
                 const Opm::UnitSystem&     units,
                 const Opm::TableContainer& sgof)
        {
            using SGOF = ::Opm::SgofTable;

            const auto numTab = sgof.size();
            const auto numDep = std::size_t{2}; // Krg, Pcgo

            return detail::createSatfuncTable(numTab, numRows, numDep,
                [&units, &sgof](const std::size_t           tableID,
                                const std::size_t           primID,
                                Opm::LinearisedOutputTable& linTable)
                -> std::size_t
            {
                const auto& t = sgof.getTable<SGOF>(tableID);

                auto numActRows = std::size_t{0};

                // Sg
                {
                    const auto& Sg = t.getSgColumn();

                    numActRows = Sg.size();
                    std::copy(std::begin(Sg), std::end(Sg),
                              linTable.column(tableID, primID, 0));
                }

                // Krg(Sg)
                {
                    const auto& kr = t.getKrgColumn();
                    std::copy(std::begin(kr), std::end(kr),
                              linTable.column(tableID, primID, 1));
                }

                // Pcgo(Sg)
                {
                    const auto uPress = ::Opm::UnitSystem::measure::pressure;

                    const auto& pc = t.getPcogColumn();
                    std::transform(std::begin(pc), std::end(pc),
                                   linTable.column(tableID, primID, 2),
                                   [&units, uPress](const double Pc) -> double
                                   {
                                       return units.from_si(uPress, Pc);
                                   });
                }

                // Inform createSatfuncTable() of number of active rows in
                // this table.  Needed to compute slopes of piecewise linear
                // interpolants.
                return numActRows;
            });
        }
    } // Gas

    /// Functions to create linearised, padded, and normalised SOFN output
    /// tables from various input saturation function keywords, depending on
    /// number of active phases.
    namespace Oil {
        /// Form normalised SOFN output tables for two-phase runs.
        namespace TwoPhase {
            /// Create linearised and padded 'TAB' vector entries of
            /// normalised two-phase SOFN tables for all saturation function
            /// regions from Family Two table data (SOF2 keyword).
            ///
            /// \param[in] numRows Number of rows to allocate in the output
            ///    vector for each table.  Expected to be equal to the
            ///    number of declared saturation nodes in the simulation
            ///    run's TABDIMS keyword (Item 3).
            ///
            /// \param[in] sof2 Collection of SOF2 tables for all saturation
            ///   regions.
            ///
            /// \return Linearised and padded 'TAB' vector values for
            ///   three-phase SOFN tables.  Essentially just a padded copy
            ///   of the input SOF2 table--with added derivatives.
            std::vector<double>
            fromSOF2(const std::size_t          numRows,
                     const Opm::TableContainer& sof2)
            {
                using SOF2 = ::Opm::Sof2Table;

                const auto numTab = sof2.size();
                const auto numDep = std::size_t{1}; // Kro

                return detail::createSatfuncTable(numTab, numRows, numDep,
                    [&sof2](const std::size_t           tableID,
                            const std::size_t           primID,
                            Opm::LinearisedOutputTable& linTable)
                    -> std::size_t
                {
                    const auto& t = sof2.getTable<SOF2>(tableID);

                    auto numActRows = std::size_t{0};

                    // So
                    {
                        const auto& So = t.getSoColumn();

                        numActRows = So.size();
                        std::copy(std::begin(So), std::end(So),
                                  linTable.column(tableID, primID, 0));
                    }

                    // Kro(So)
                    {
                        const auto& kr = t.getKroColumn();
                        std::copy(std::begin(kr), std::end(kr),
                                  linTable.column(tableID, primID, 1));
                    }

                    // Inform createSatfuncTable() of number of active rows
                    // in this table.  Needed to compute slopes of piecewise
                    // linear interpolants.
                    return numActRows;
                });
            }

            /// Create linearised and padded 'TAB' vector entries of
            /// normalised two-phase SOFN tables for all saturation function
            /// regions from Family One table data (SGOF keyword--G/O System).
            ///
            /// \param[in] numRows Number of rows to allocate in the output
            ///    vector for each table.  Expected to be equal to the
            ///    number of declared saturation nodes in the simulation
            ///    run's TABDIMS keyword (Item 3).
            ///
            /// \param[in] sgof Collection of SGOF tables for all saturation
            ///    regions.
            ///
            /// \return Linearised and padded 'TAB' vector values for
            ///    two-phase SOFN tables.  Corresponds to translated (1-Sg),
            ///    reverse saturation column (column 1) and reverse column
            ///    of relative permeability for oil (column 3) from the
            ///    input SGOF table--with added derivatives.
            std::vector<double>
            fromSGOF(const std::size_t          numRows,
                     const Opm::TableContainer& sgof)
            {
                using SGOF = ::Opm::SgofTable;

                const auto numTab = sgof.size();
                const auto numDep = std::size_t{1}; // Kro

                return detail::createSatfuncTable(numTab, numRows, numDep,
                    [&sgof](const std::size_t           tableID,
                            const std::size_t           primID,
                            Opm::LinearisedOutputTable& linTable)
                    -> std::size_t
                {
                    const auto& t = sgof.getTable<SGOF>(tableID);

                    auto numActRows = std::size_t{0};

                    // So
                    {
                        const auto& Sg = t.getSgColumn();
                        numActRows = Sg.size();

                        auto So = std::vector<double>{};
                        So.reserve(numActRows);

                        // Two-phase system => So = 1-Sg
                        std::transform(std::begin(Sg), std::end(Sg),
                                       std::back_inserter(So),
                                       [](const double sg)
                                       {
                                           return 1.0 - sg;
                                       });

                        std::copy(So.rbegin(), So.rend(),
                                  linTable.column(tableID, primID, 0));
                    }

                    // Kro(So)
                    {
                        const auto& kr = t.getKrogColumn();

                        const auto krog = std::vector<double> {
                            std::begin(kr), std::end(kr)
                        };

                        std::copy(krog.rbegin(), krog.rend(),
                                  linTable.column(tableID, primID, 1));
                    }

                    // Inform createSatfuncTable() of number of active rows
                    // in this table.  Needed to compute slopes of piecewise
                    // linear interpolants.
                    return numActRows;
                });
            }

            /// Create linearised and padded 'TAB' vector entries for
            /// normalised two-phase SOFN tables for all saturation function
            /// regions from Family One table data (SWOF keyword--O/W System).
            ///
            /// \param[in] numRows Number of rows to allocate in the output
            ///    vector for each table.  Expected to be equal to the
            ///    number of declared saturation nodes in the simulation
            ///    run's TABDIMS keyword (Item 3).
            ///
            /// \param[in] swof Collection of SWOF tables for all saturation
            ///    regions.
            ///
            /// \return Linearised and padded 'TAB' vector values for
            ///    two-phase SOFN tables.  Corresponds to translated (1-Sw),
            ///    reverse saturation column (column 1) and reverse column
            ///    of relative permeability for oil (column 3) from the
            ///    input SWOF table--with added derivatives.
            std::vector<double>
            fromSWOF(const std::size_t          numRows,
                     const Opm::TableContainer& swof)
            {
                using SWOF = ::Opm::SwofTable;

                const auto numTab = swof.size();
                const auto numDep = std::size_t{1}; // Kro

                return detail::createSatfuncTable(numTab, numRows, numDep,
                    [&swof](const std::size_t           tableID,
                            const std::size_t           primID,
                            Opm::LinearisedOutputTable& linTable)
                    -> std::size_t
                {
                    const auto& t = swof.getTable<SWOF>(tableID);

                    auto numActRows = std::size_t{0};

                    // So
                    {
                        const auto& Sw = t.getSwColumn();
                        numActRows = Sw.size();

                        auto So = std::vector<double>{};
                        So.reserve(numActRows);

                        // Two-phase system => So = 1-Sw
                        std::transform(std::begin(Sw), std::end(Sw),
                                       std::back_inserter(So),
                                       [](const double sw)
                                       {
                                           return 1.0 - sw;
                                       });

                        std::copy(So.rbegin(), So.rend(),
                                  linTable.column(tableID, primID, 0));
                    }

                    // Kro(So)
                    {
                        const auto& kr = t.getKrowColumn();

                        const auto krow = std::vector<double> {
                            std::begin(kr), std::end(kr)
                        };

                        std::copy(krow.rbegin(), krow.rend(),
                                  linTable.column(tableID, primID, 1));
                    }

                    // Inform createSatfuncTable() of number of active rows
                    // in this table.  Needed to compute slopes of piecewise
                    // linear interpolants.
                    return numActRows;
                });
            }
        } // TwoPhase

        /// Form normalised SOFN output tables for three-phase runs.
        namespace ThreePhase {
            /// Facility to provide oil saturation and relative permeability
            /// look-up based on data in a Family One table.
            class DerivedKroFunction
            {
            public:
                /// Constructor
                ///
                /// \param[in] s Phase saturation values.  Increasing Sg in
                ///   the case of SGOF or increasing Sw in the case of SWOF.
                ///
                /// \param[in] kro Relative permeability for oil.  Should be
                ///   the decreasing KrOG column in the case of SGOF or the
                ///   decreasing KrOW column in the case of SWOF.
                ///
                /// \param[in] So_off Oil saturation offset through which to
                ///   convert input phase saturation values to saturation
                ///   values for oil.  Should be (1 - Sw_conn) in the case
                ///   of SGOF and 1.0 for the case of SWOF.
                DerivedKroFunction(std::vector<double> s,
                                   std::vector<double> kro,
                                   const double        So_off)
                    : s_     (std::move(s))
                    , kro_   (std::move(kro))
                    , So_off_(So_off)
                {}

                /// Get oil saturation at particular node.
                ///
                /// \param[in] i Saturation node identifier.
                ///
                /// \return Oil saturation at node \p i.
                double So(const std::size_t i) const
                {
                    return this->So_off_ - this->s_[i];
                }

                /// Get relative permeability for oil at particular node.
                ///
                /// \param[in] i Saturation node identifier.
                ///
                /// \return Relative permeability for oil at node \p i.
                double Kro(const std::size_t i) const
                {
                    return this->kro_[i];
                }

                /// Get relative permeability for oil at particular oil
                /// saturation.
                ///
                /// Uses piecewise linear interpolation in the input KrO
                /// table.
                ///
                /// \param[in] So Oil saturation.
                ///
                /// \return Relative permeability for oil at So.
                double Kro(const double So) const
                {
                    const auto s = this->So_off_ - So;

                    auto b = std::begin(this->s_);
                    auto e = std::end  (this->s_);
                    auto p = std::lower_bound(b, e, s);

                    if (p == b) { return this->kro_.front(); }
                    if (p == e) { return this->kro_.back (); }

                    const auto i = p - b; // *Right-hand* end-point.

                    const auto sl = this->s_  [i - 1];
                    const auto yl = this->kro_[i - 1];

                    const auto sr = this->s_  [i - 0];
                    const auto yr = this->kro_[i - 0];

                    const auto t = (s - sl) / (sr - sl);

                    return t*yr + (1.0 - t)*yl;
                }

                /// Retrieve number of active saturation nodes in this
                /// table.
                std::vector<double>::size_type size() const
                {
                    return this->s_.size();
                }

            private:
                /// Input phase saturation.  Sg or Sw.
                std::vector<double> s_;

                /// Input relative permeabilty for oil.  KrOG or KrOW.
                std::vector<double> kro_;

                /// Oil saturation offset through which to convert between
                /// input phase saturation and oil saturation.
                double So_off_;
            };

            /// Pair of saturation node index and saturation function table.
            struct TableElement {
                /// Which numeric table to use for look-up.
                std::size_t function;

                /// Saturation node ID within 'function'.
                std::size_t index;
            };

            // S{G,W}OF tables have KrOX data in terms of increasing phase
            // saturation for Gas and Water, respectively, so we need to
            // traverse those tables in the opposite direction in order to
            // generate the KrOX values in terms of increasing phase
            // saturation for Oil.
            std::vector<TableElement>
            makeReverseRange(const std::size_t function,
                             const std::size_t n)
            {
                auto ret = std::vector<TableElement>{};
                ret.reserve(n);

                for (auto i = n; i > 0; --i) {
                    ret.push_back( TableElement {
                            function, i - 1
                        });
                }

                return ret;
            }

            // Join derived KrO functions on common saturation values for
            // oil.  Heavy lifting by std::set_union() to avoid outputting
            // common oil saturation values more than once.  Relies on input
            // tables having sorted phase saturation values (required by ECL
            // format).
            std::vector<TableElement>
            mergeTables(const std::vector<DerivedKroFunction>& t)
            {
                auto ret = std::vector<TableElement>{};

                const auto t0 = makeReverseRange(0, t[0].size());
                const auto t1 = makeReverseRange(1, t[1].size());

                ret.reserve(t0.size() + t1.size());

                std::set_union(std::begin(t0), std::end(t0),
                               std::begin(t1), std::end(t1),
                               std::back_inserter(ret),
                    [&t](const TableElement& e1, const TableElement& e2)
                {
                    return t[e1.function].So(e1.index)
                        <  t[e2.function].So(e2.index);
                });

                return ret;
            }

            // Create collection of individual columns of single SOF3 table
            // through joining input SGOF and SWOF tables on increasing oil
            // saturation and appropriate KrOX columns.
            std::array<std::vector<double>, 3>
            makeSOF3Table(const Opm::SgofTable& sgof,
                          const Opm::SwofTable& swof)
            {
                auto ret = std::array<std::vector<double>, 3>{};

                auto tbl = std::vector<DerivedKroFunction>{};
                tbl.reserve(2);

                // Note: Order between Krow(So) and Krog(So) matters
                // here.  This order must match the expected column
                // order in SOF3--i.e. [ So, Krow, Krog ].

                // 1) Krow(So)
                {
                    // So = 1.0 - Sw
                    const auto& Sw     = swof.getSwColumn();
                    const auto& Krow   = swof.getKrowColumn();
                    const auto& So_off = 1.0;

                    tbl.emplace_back(Sw  .vectorCopy(),
                                     Krow.vectorCopy(), So_off);
                }

                // 2) Krog(So)
                {
                    // So = (1.0 - Sw_conn) - Sg
                    const auto& Sg     = sgof.getSgColumn();
                    const auto& Krog   = sgof.getKrogColumn();
                    const auto  So_off =
                        1.0 - swof.getSwColumn()[0];

                    tbl.emplace_back(Sg  .vectorCopy(),
                                     Krog.vectorCopy(), So_off);
                }

                const auto mrg = mergeTables(tbl);

                for (auto& col : ret) { col.reserve(mrg.size()); }

                for (const auto& row : mrg) {
                    const auto self  =     row.function;
                    const auto other = 1 - row.function;

                    // 1) Assign So
                    ret[0].push_back(tbl[self].So(row.index));

                    // 2) Assign Kro for "self" column (the one that got
                    //    picked for this row).
                    ret[1 + self].push_back(tbl[self].Kro(row.index));

                    // 3) Assign Kro for "other" column (the one that
                    //    did not get picked for this row).
                    ret[1 + other].push_back(tbl[other].Kro(ret[0].back()));
                }

                return ret;
            }

            /// Create linearised and padded 'TAB' vector entries of
            /// normalised three-phase SOFN tables for all saturation
            /// function regions from Family One table data.
            ///
            /// \param[in] numRows Number of rows to allocate in the output
            ///    vector for each table.  Expected to be twice the number
            ///    of declared saturation nodes in the simulation run's
            ///    TABDIMS keyword (Item 3).
            ///
            /// \param[in] sgof Collection of SGOF tables for all saturation
            ///    regions.
            ///
            /// \param[in] swof Collection of SWOF tables for all saturation
            ///    regions.
            ///
            /// \return Linearised and padded 'TAB' vector values for
            ///    three-phase SOFN tables.  Corresponds to column 1 from
            ///    both of the input SGOF and SWOF tables, as well as column
            ///    3 from the input SWOF table and column 3 from the input
            ///    SGOF table--expanded so as to have values for all oil
            ///    saturation nodes.  Derivatives added in columns 4 and 5.
            std::vector<double>
            fromSGOFandSWOF(const std::size_t          numRows,
                            const Opm::TableContainer& sgof,
                            const Opm::TableContainer& swof)
            {
                using SGOF = ::Opm::SgofTable;
                using SWOF = ::Opm::SwofTable;

                const auto numTab = sgof.size();
                const auto numDep = std::size_t{2}; // Krow, Krog

                return detail::createSatfuncTable(numTab, numRows, numDep,
                     [&sgof, &swof](const std::size_t           tableID,
                                    const std::size_t           primID,
                                    Opm::LinearisedOutputTable& linTable)
                    -> std::size_t
                {
                    const auto sof3 =
                        makeSOF3Table(sgof.getTable<SGOF>(tableID),
                                      swof.getTable<SWOF>(tableID));

                    auto numActRows = std::size_t{0};

                    // So
                    {
                        const auto& So = sof3[0];

                        numActRows = So.size();
                        std::copy(std::begin(So), std::end(So),
                                  linTable.column(tableID, primID, 0));
                    }

                    // Krow(So)
                    {
                        const auto& krow = sof3[1];
                        std::copy(std::begin(krow), std::end(krow),
                                  linTable.column(tableID, primID, 1));
                    }

                    // Krog(So)
                    {
                        const auto& krog = sof3[2];
                        std::copy(std::begin(krog), std::end(krog),
                                  linTable.column(tableID, primID, 2));
                    }

                    // Inform createSatfuncTable() of number of active rows
                    // in this table.  Needed to compute slopes of piecewise
                    // linear interpolants.
                    return numActRows;
                });
            }

            /// Create linearised and padded 'TAB' vector entries of
            /// normalised three-phase SOFN tables for all saturation
            /// function regions from Family Two table data (SOF3 keyword).
            ///
            /// \param[in] numRows Number of rows to allocate in the output
            ///    vector for each table.  Expected to be equal to the
            ///    number of declared saturation nodes in the simulation
            ///    run's TABDIMS keyword (Item 3).
            ///
            /// \param[in] sof3 Collection of SOF3 tables for all saturation
            ///    regions.
            ///
            /// \return Linearised and padded 'TAB' vector values for output
            ///    three-phase SOFN tables.  Essentially a padded copy of
            ///    the input SOF3 tables, \p sof3, with added derivatives.
            std::vector<double>
            fromSOF3(const std::size_t          numRows,
                     const Opm::TableContainer& sof3)
            {
                using SOF3 = ::Opm::Sof3Table;

                const auto numTab = sof3.size();
                const auto numDep = std::size_t{2}; // Krow, Krog

                return detail::createSatfuncTable(numTab, numRows, numDep,
                    [&sof3](const std::size_t           tableID,
                            const std::size_t           primID,
                            Opm::LinearisedOutputTable& linTable)
                    -> std::size_t
                {
                    const auto& t = sof3.getTable<SOF3>(tableID);

                    auto numActRows = std::size_t{0};

                    // So
                    {
                        const auto& So = t.getSoColumn();

                        numActRows = So.size();
                        std::copy(std::begin(So), std::end(So),
                                  linTable.column(tableID, primID, 0));
                    }

                    // Krow(So)
                    {
                        const auto& kr = t.getKrowColumn();
                        std::copy(std::begin(kr), std::end(kr),
                                  linTable.column(tableID, primID, 1));
                    }

                    // Krog(So)
                    {
                        const auto& kr = t.getKrogColumn();
                        std::copy(std::begin(kr), std::end(kr),
                                  linTable.column(tableID, primID, 2));
                    }

                    // Inform createSatfuncTable() of number of active rows
                    // in this table.  Needed to compute slopes of piecewise
                    // linear interpolants.
                    return numActRows;
                });
            }
        } // ThreePhase
    } // Oil

    /// Functions to create linearised, padded, and normalised SWFN output
    /// tables from various input saturation function keywords.
    namespace Water {
        /// Create linearised and padded 'TAB' vector entries of normalised
        /// SWFN tables for all saturation function regions from Family Two
        /// table data (SWFN keyword).
        ///
        /// \param[in] numRows Number of rows to allocate for each table in
        ///    the output vector.  Expected to be equal to the number of
        ///    declared saturation nodes in the simulation run's TABDIMS
        ///    keyword (Item 3).
        ///
        /// \param[in] units Active unit system.  Needed to convert SI
        ///    convention capillary pressure values (Pascal) to declared
        ///    conventions of the run specification.
        ///
        /// \param[in] swfn Collection of SWFN tables for all saturation
        ///    regions.
        ///
        /// \return Linearised and padded 'TAB' vector values for output
        ///    SWFN tables.  A unit-converted copy of the input table \p
        ///    swfn with added derivatives.
        std::vector<double>
        fromSWFN(const std::size_t          numRows,
                 const Opm::UnitSystem&     units,
                 const Opm::TableContainer& swfn)
        {
            using SWFN = ::Opm::SwfnTable;

            const auto numTab = swfn.size();
            const auto numDep = std::size_t{2}; // Krw, Pcow

            return detail::createSatfuncTable(numTab, numRows, numDep,
                [&swfn, &units](const std::size_t           tableID,
                                const std::size_t           primID,
                                Opm::LinearisedOutputTable& linTable)
                -> std::size_t
            {
                const auto& t = swfn.getTable<SWFN>(tableID);

                auto numActRows = std::size_t{0};

                // Sw
                {
                    const auto& Sw = t.getSwColumn();

                    numActRows = Sw.size();
                    std::copy(std::begin(Sw), std::end(Sw),
                              linTable.column(tableID, primID, 0));
                }

                // Krw(Sw)
                {
                    const auto& kr = t.getKrwColumn();
                    std::copy(std::begin(kr), std::end(kr),
                              linTable.column(tableID, primID, 1));
                }

                // Pcow(Sw)
                {
                    const auto uPress = ::Opm::UnitSystem::measure::pressure;

                    const auto& pc = t.getPcowColumn();
                    std::transform(std::begin(pc), std::end(pc),
                                   linTable.column(tableID, primID, 2),
                                   [&units, uPress](const double Pc) -> double
                                   {
                                       return units.from_si(uPress, Pc);
                                   });
                }

                // Inform createSatfuncTable() of number of active rows in
                // this table.  Needed to compute slopes of piecewise linear
                // interpolants.
                return numActRows;
            });
        }

        /// Create linearised and padded 'TAB' vector entries of normalised
        /// SWFN tables for all saturation function regions from Family One
        /// table data (SWOF keyword).
        ///
        /// \param[in] numRows Number of rows to allocate for each table in
        ///    the output vector.  Expected to be equal to the number of
        ///    declared saturation nodes in the simulation run's TABDIMS
        ///    keyword (Item 3).
        ///
        /// \param[in] units Active unit system.  Needed to convert SI
        ///    convention capillary pressure values (Pascal) to declared
        ///    conventions of the run specification.
        ///
        /// \param[in] swof Collection of SWOF tables for all saturation
        ///    regions.
        ///
        /// \return Linearised and padded 'TAB' vector values for output
        ///    SWFN tables.  Corresponds to unit-converted copies of columns
        ///    1, 2, and 4--with added derivatives--of the input SWOF tables.
        std::vector<double>
        fromSWOF(const std::size_t          numRows,
                 const Opm::UnitSystem&     units,
                 const Opm::TableContainer& swof)
        {
            using SWOF = ::Opm::SwofTable;

            const auto numTab = swof.size();
            const auto numDep = std::size_t{2}; // Krw, Pcow

            return detail::createSatfuncTable(numTab, numRows, numDep,
                [&swof, &units](const std::size_t           tableID,
                                const std::size_t           primID,
                                Opm::LinearisedOutputTable& linTable)
                -> std::size_t
            {
                const auto& t = swof.getTable<SWOF>(tableID);

                auto numActRows = std::size_t{0};

                // Sw
                {
                    const auto& Sw = t.getSwColumn();

                    numActRows = Sw.size();
                    std::copy(std::begin(Sw), std::end(Sw),
                              linTable.column(tableID, primID, 0));
                }

                // Krw(Sw)
                {
                    const auto& kr = t.getKrwColumn();
                    std::copy(std::begin(kr), std::end(kr),
                              linTable.column(tableID, primID, 1));
                }

                // Pcow(Sw)
                {
                    const auto uPress = ::Opm::UnitSystem::measure::pressure;

                    const auto& pc = t.getPcowColumn();
                    std::transform(std::begin(pc), std::end(pc),
                                   linTable.column(tableID, primID, 2),
                                   [&units, uPress](const double Pc) -> double
                                   {
                                       return units.from_si(uPress, Pc);
                                   });
                }

                // Inform createSatfuncTable() of number of active rows in
                // this table.  Needed to compute slopes of piecewise linear
                // interpolants.
                return numActRows;
            });
        }
    } // Water
}} // Anonymous::SatFunc

namespace Opm {

    Tables::Tables(const UnitSystem& units)
        : units_  (units)
        , tabdims_(TABDIMS_SIZE, 0)
    {
        // Initialize subset of base pointers and dimensions to 1 to honour
        // requirements of TABDIMS protocol.  The magic constant 59 is
        // derived from the file-formats documentation.
        std::fill_n(std::begin(this->tabdims_), 59, 1);
    }

    void Tables::addData(const std::size_t          offset_index,
                         const std::vector<double>& new_data)
    {
        this->tabdims_[ offset_index ] = this->data_.size() + 1;

        this->data_.insert(this->data_.end(), new_data.begin(), new_data.end());

        this->tabdims_[ TABDIMS_TAB_SIZE_ITEM ] = this->data_.size();
    }


    namespace {
        struct PvtxDims {
            size_t num_tables;
            size_t outer_size;
            size_t inner_size;
            size_t num_columns = 3;
            size_t data_size;
        };

        template <class TableType>
        PvtxDims tableDims( const std::vector<TableType>& pvtxTables) {
            PvtxDims dims;

            dims.num_tables = pvtxTables.size();
            dims.inner_size = 0;
            dims.outer_size = 0;
            for (const auto& table : pvtxTables) {
                dims.outer_size = std::max( dims.outer_size, table.size());
                for (const auto& underSatTable : table)
                    dims.inner_size = std::max(dims.inner_size, underSatTable.numRows() );
            }
            dims.data_size = dims.num_tables * dims.outer_size * dims.inner_size * dims.num_columns;

            return dims;
        }
    }

    void Tables::addPVTO( const std::vector<PvtoTable>& pvtoTables)
    {
        const double default_value = 2e20;
        PvtxDims dims = tableDims( pvtoTables );
        this->tabdims_[ TABDIMS_NTPVTO_ITEM ] = dims.num_tables;
        this->tabdims_[ TABDIMS_NRPVTO_ITEM ] = dims.outer_size;
        this->tabdims_[ TABDIMS_NPPVTO_ITEM ] = dims.inner_size;

        {
            std::vector<double> pvtoData( dims.data_size , default_value );
            std::vector<double> rs_values( dims.num_tables * dims.outer_size  , default_value );
            size_t composition_stride = dims.inner_size;
            size_t table_stride = dims.outer_size * composition_stride;
            size_t column_stride = table_stride * pvtoTables.size();

            size_t table_index = 0;
            for (const auto& table : pvtoTables) {
                size_t composition_index = 0;
                for (const auto& underSatTable : table) {
                    const auto& p  = underSatTable.getColumn("P");
                    const auto& bo = underSatTable.getColumn("BO");
                    const auto& mu = underSatTable.getColumn("MU");

                    for (size_t row = 0; row < p.size(); row++) {
                        size_t data_index = row + composition_stride * composition_index + table_stride * table_index;

                        pvtoData[ data_index ]                  = this->units_.from_si( UnitSystem::measure::pressure, p[row]);
                        pvtoData[ data_index + column_stride ]  = 1.0 / bo[row];
                        pvtoData[ data_index + 2*column_stride] = this->units_.from_si( UnitSystem::measure::viscosity , mu[row]) / bo[row];
                    }
                    composition_index++;
                }

                /*
                  The RS values which apply for one inner table each
                  are added as a separate data vector to the TABS
                  array.
                */
                {
                    const auto& sat_table = table.getSaturatedTable();
                    const auto& rs = sat_table.getColumn("RS");
                    for (size_t index = 0; index < rs.size(); index++)
                        rs_values[index + table_index * dims.outer_size ] = rs[index];
                }
                table_index++;
            }

            this->addData( TABDIMS_IBPVTO_OFFSET_ITEM , pvtoData );
            this->addData( TABDIMS_JBPVTO_OFFSET_ITEM , rs_values );
        }
    }

    void Tables::addPVTG( const std::vector<PvtgTable>& pvtgTables) {
        const double default_value = -2e20;
        PvtxDims dims = tableDims( pvtgTables );
        this->tabdims_[ TABDIMS_NTPVTG_ITEM ] = dims.num_tables;
        this->tabdims_[ TABDIMS_NRPVTG_ITEM ] = dims.outer_size;
        this->tabdims_[ TABDIMS_NPPVTG_ITEM ] = dims.inner_size;

        {
            std::vector<double> pvtgData( dims.data_size , default_value );
            std::vector<double> p_values( dims.num_tables * dims.outer_size  , default_value );
            size_t composition_stride = dims.inner_size;
            size_t table_stride = dims.outer_size * composition_stride;
            size_t column_stride = table_stride * dims.num_tables;

            size_t table_index = 0;
            for (const auto& table : pvtgTables) {
                size_t composition_index = 0;
                for (const auto& underSatTable : table) {
                    const auto& col0 = underSatTable.getColumn(0);
                    const auto& col1 = underSatTable.getColumn(1);
                    const auto& col2 = underSatTable.getColumn(2);

                    for (size_t row = 0; row < col0.size(); row++) {
                        size_t data_index = row + composition_stride * composition_index + table_stride * table_index;

                        pvtgData[ data_index ]                  = this->units_.from_si( UnitSystem::measure::gas_oil_ratio, col0[row]);
                        pvtgData[ data_index + column_stride ]  = this->units_.from_si( UnitSystem::measure::gas_oil_ratio, col1[row]);
                        pvtgData[ data_index + 2*column_stride] = this->units_.from_si( UnitSystem::measure::viscosity , col2[row]);
                    }

                    composition_index++;
                }

                {
                    const auto& sat_table = table.getSaturatedTable();
                    const auto& p = sat_table.getColumn("PG");
                    for (size_t index = 0; index < p.size(); index++)
                        p_values[index + table_index * dims.outer_size ] =
                            this->units_.from_si( UnitSystem::measure::pressure , p[index]);
                }

                table_index++;
            }

            this->addData( TABDIMS_IBPVTG_OFFSET_ITEM , pvtgData );
            this->addData( TABDIMS_JBPVTG_OFFSET_ITEM , p_values );
        }
    }

    void Tables::addPVTW( const PvtwTable& pvtwTable)
    {
        if (pvtwTable.size() > 0) {
            const double default_value = -2e20;
            const size_t num_columns = pvtwTable[0].size;
            std::vector<double> pvtwData( pvtwTable.size() * num_columns , default_value);

            this->tabdims_[ TABDIMS_NTPVTW_ITEM ] = pvtwTable.size();
            for (size_t table_num = 0; table_num < pvtwTable.size(); table_num++) {
                const auto& record = pvtwTable[table_num];
                pvtwData[ table_num * num_columns ]    = this->units_.from_si( UnitSystem::measure::pressure , record.reference_pressure);
                pvtwData[ table_num * num_columns + 1] = 1.0 / record.volume_factor;
                pvtwData[ table_num * num_columns + 2] = this->units_.to_si( UnitSystem::measure::pressure, record.compressibility);
                pvtwData[ table_num * num_columns + 3] = record.volume_factor / this->units_.from_si( UnitSystem::measure::viscosity , record.viscosity);


                // The last column should contain information about
                // the viscosibility, however there is clearly a
                // not-yet-identified transformation involved, we
                // therefor leave this item defaulted.

                // pvtwData[ table_num * num_columns + 4] = record.viscosibility;
            }
            this->addData( TABDIMS_IBPVTW_OFFSET_ITEM , pvtwData );
        }
    }

    void Tables::addDensity( const DensityTable& density)
    {
        if (density.size() > 0) {
            const double default_value = -2e20;
            const size_t num_columns = density[0].size;
            std::vector<double> densityData( density.size() * num_columns , default_value);

            this->tabdims_[ TABDIMS_NTDENS_ITEM ] = density.size();
            for (size_t table_num = 0; table_num < density.size(); table_num++) {
                const auto& record = density[table_num];
                densityData[ table_num * num_columns ]    = this->units_.from_si( UnitSystem::measure::density , record.oil);
                densityData[ table_num * num_columns + 1] = this->units_.from_si( UnitSystem::measure::density , record.water);
                densityData[ table_num * num_columns + 2] = this->units_.from_si( UnitSystem::measure::density , record.gas);
            }
            this->addData( TABDIMS_IBDENS_OFFSET_ITEM , densityData );
        }
    }

    void Tables::addSatFunc(const EclipseState& es)
    {
        const auto& tabMgr = es.getTableManager();
        const auto& phases = es.runspec().phases();
        const auto  gas    = phases.active(Phase::GAS);
        const auto  oil    = phases.active(Phase::OIL);
        const auto  wat    = phases.active(Phase::WATER);
        const auto  threeP = gas && oil && wat;

        const auto famI =       // SGOF and/or SWOF
            (gas && tabMgr.hasTables("SGOF")) ||
            (wat && tabMgr.hasTables("SWOF"));

        const auto famII =      // SGFN, SOF{2,3}, SWFN
            (gas && tabMgr.hasTables("SGFN")) ||
            (oil && ((threeP && tabMgr.hasTables("SOF3")) ||
                     tabMgr.hasTables("SOF2"))) ||
            (wat && tabMgr.hasTables("SWFN"));

        if ((famI + famII) != 1) {
            // Both Fam I && Fam II or neither of them.  Can't have that.
            return;             // Logging here?
        }
        else if (famI) {
            this->addSatFunc_FamilyOne(es, gas, oil, wat);
        }
        else {
            // Family II
            this->addSatFunc_FamilyTwo(es, gas, oil, wat);
        }
    }

    const std::vector<int>& Tables::tabdims() const
    {
        return this->tabdims_;
    }

    const std::vector<double>& Tables::tab() const
    {
        return this->data_;
    }

    void Tables::addSatFunc_FamilyOne(const EclipseState& es,
                                      const bool          gas,
                                      const bool          oil,
                                      const bool          wat)
    {
        const auto& tabMgr = es.getTableManager();
        const auto& tabd   = es.runspec().tabdims();
        const auto  nssfun = tabd.getNumSatNodes();

        if (gas) {
            const auto& tables = tabMgr.getSgofTables();

            const auto sgfn =
                SatFunc::Gas::fromSGOF(nssfun, this->units_, tables);

            this->addData(TABDIMS_IBSGFN_OFFSET_ITEM, sgfn);
            this->tabdims_[TABDIMS_NSSGFN_ITEM] = nssfun;
            this->tabdims_[TABDIMS_NTSGFN_ITEM] = tables.size();
        }

        if (oil) {
            if (gas && !wat) {  // 2p G/O System
                const auto& tables = tabMgr.getSgofTables();

                const auto sofn =
                    SatFunc::Oil::TwoPhase::fromSGOF(nssfun, tables);

                this->addData(TABDIMS_IBSOFN_OFFSET_ITEM, sofn);
                this->tabdims_[TABDIMS_NSSOFN_ITEM] = nssfun;
                this->tabdims_[TABDIMS_NTSOFN_ITEM] = tables.size();
            }
            else if (wat && !gas) { // 2p O/W System
                const auto& tables = tabMgr.getSwofTables();

                const auto sofn =
                    SatFunc::Oil::TwoPhase::fromSWOF(nssfun, tables);

                this->addData(TABDIMS_IBSOFN_OFFSET_ITEM, sofn);
                this->tabdims_[TABDIMS_NSSOFN_ITEM] = nssfun;
                this->tabdims_[TABDIMS_NTSOFN_ITEM] = tables.size();
            }
            else {              // 3p G/O/W System
                const auto& sgof = tabMgr.getSgofTables();
                const auto& swof = tabMgr.getSwofTables();

                // Allocate 2*nssfun rows to account for table merging.

                const auto numRows = 2 * nssfun;

                const auto sofn = SatFunc::Oil::ThreePhase::
                    fromSGOFandSWOF(numRows, sgof, swof);

                this->addData(TABDIMS_IBSOFN_OFFSET_ITEM, sofn);
                this->tabdims_[TABDIMS_NSSOFN_ITEM] = numRows;
                this->tabdims_[TABDIMS_NTSOFN_ITEM] = sgof.size();
            }
        }

        if (wat) {
            const auto& tables = tabMgr.getSwofTables();

            const auto swfn =
                SatFunc::Water::fromSWOF(nssfun, this->units_, tables);

            this->addData(TABDIMS_IBSWFN_OFFSET_ITEM, swfn);
            this->tabdims_[TABDIMS_NSSWFN_ITEM] = nssfun;
            this->tabdims_[TABDIMS_NTSWFN_ITEM] = tables.size();
        }
    }

    void Tables::addSatFunc_FamilyTwo(const EclipseState& es,
                                      const bool          gas,
                                      const bool          oil,
                                      const bool          wat)
    {
        const auto& tabMgr = es.getTableManager();
        const auto& tabd   = es.runspec().tabdims();
        const auto  nssfun = tabd.getNumSatNodes();

        if (gas) {
            const auto& tables = tabMgr.getSgfnTables();

            const auto sgfn =
                SatFunc::Gas::fromSGFN(nssfun, this->units_, tables);

            this->addData(TABDIMS_IBSGFN_OFFSET_ITEM, sgfn);
            this->tabdims_[TABDIMS_NSSGFN_ITEM] = nssfun;
            this->tabdims_[TABDIMS_NTSGFN_ITEM] = tables.size();
        }

        if (oil) {
            if (gas + wat == 1) { // 2p G/O or O/W System
                const auto& tables = tabMgr.getSof2Tables();

                const auto sofn =
                    SatFunc::Oil::TwoPhase::fromSOF2(nssfun, tables);

                this->addData(TABDIMS_IBSOFN_OFFSET_ITEM, sofn);
                this->tabdims_[TABDIMS_NSSOFN_ITEM] = nssfun;
                this->tabdims_[TABDIMS_NTSOFN_ITEM] = tables.size();
            }
            else {              // 3p G/O/W System
                const auto& tables = tabMgr.getSof3Tables();

                const auto sofn =
                    SatFunc::Oil::ThreePhase::fromSOF3(nssfun, tables);

                this->addData(TABDIMS_IBSOFN_OFFSET_ITEM, sofn);
                this->tabdims_[TABDIMS_NSSOFN_ITEM] = nssfun;
                this->tabdims_[TABDIMS_NTSOFN_ITEM] = tables.size();
            }
        }

        if (wat) {
            const auto& tables = tabMgr.getSwfnTables();

            const auto swfn =
                SatFunc::Water::fromSWFN(nssfun, this->units_, tables);

            this->addData(TABDIMS_IBSWFN_OFFSET_ITEM, swfn);
            this->tabdims_[TABDIMS_NSSWFN_ITEM] = nssfun;
            this->tabdims_[TABDIMS_NTSWFN_ITEM] = tables.size();
        }
    }

    void fwrite(const Tables& tables,
                ERT::FortIO&  fortio)
    {
        {
            ERT::EclKW<int> tabdims("TABDIMS", tables.tabdims());
            tabdims.fwrite(fortio);
        }

        {
            ERT::EclKW<double> tab("TAB", tables.tab());
            tab.fwrite(fortio);
        }
    }
}
