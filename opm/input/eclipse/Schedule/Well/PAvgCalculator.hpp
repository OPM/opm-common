/*
  Copyright 2020, 2023 Equinor ASA.

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

#ifndef PAVG_CALCULATOR_HPP
#define PAVG_CALCULATOR_HPP

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Opm {

class Connection;
class GridDims;
class PAvg;
template<class Scalar> class PAvgDynamicSourceData;
class WellConnections;

} // namespace Opm

namespace Opm {

/// Facility for deriving well-level pressure values from selected
/// block-averaging procedures.  Applicable to stopped wells which don't
/// have a flowing bottom-hole pressure.  Mainly useful for reporting.
class PAvgCalculator
{
protected:
    class Accumulator;

public:
    /// Result of block-averaging well pressure procedure
    class Result
    {
    private:
        /// Enclosing type's accumulator object can access internal data
        /// members.
        friend class Accumulator;

        /// Grant internal data member access to combination function.
        friend Result
        linearCombination(const double alpha, Result x,
                          const double beta , const Result& y);

    public:
        /// Kind of block-averaged well pressure
        enum class WBPMode
        {
            WBP,  //< Connecting cells
            WBP4, //< Immediate neighbours
            WBP5, //< Connecting cells and immediate neighbours
            WBP9, //< Connecting cells, immediate, and diagonal neighbours
        };

        /// Retrieve numerical value of specific block-averaged well pressure.
        ///
        /// \param[in] type Block-averaged pressure kind.
        /// \return Block-averaged pressure.
        double value(const WBPMode type) const
        {
            return this->wbp_[this->index(type)];
        }

    private:
        /// Number of block-averaged pressure kinds/modes.
        static constexpr auto NumModes =
            static_cast<std::size_t>(WBPMode::WBP9) + 1;

        /// Storage type for block-averaged well pressure results.
        using WBPStore = std::array<double, NumModes>;

        /// Block-averaged well pressure results.
        WBPStore wbp_{};

        /// Assign single block-averaged pressure result.
        ///
        /// \param[in] type Block-averaged pressure kind.
        /// \param[in] wbp Block-averaged pressure value.
        /// \return \code *this \endcode.
        Result& set(const WBPMode type, const double wbp)
        {
            this->wbp_[this->index(type)] = wbp;

            return *this;
        }

        /// Convert block-averaged pressure kind to linear index
        ///
        /// \param[in] mode Block-averaged pressure kind.
        /// \return Linear index corresponding to \p mode.
        constexpr WBPStore::size_type index(const WBPMode mode) const
        {
            return static_cast<WBPStore::size_type>(mode);
        }
    };

    /// References to source contributions owned by other party
    class Sources
    {
    public:
        /// Provide reference to cell-level contributions (pressure,
        /// pore-volume, mixture density) owned by other party.
        ///
        /// \param[in] wbSrc Cell-level contributions
        /// \return \code *this \endcode.
        Sources& wellBlocks(const PAvgDynamicSourceData<double>& wbSrc)
        {
            this->wb_ = &wbSrc;
            return *this;
        }

        /// Provide reference to connection-level contributions (pressure,
        /// pore-volume, mixture density) owned by other party.
        ///
        /// \param[in] wcSrc Connection-level contributions
        /// \return \code *this \endcode.
        Sources& wellConns(const PAvgDynamicSourceData<double>& wcSrc)
        {
            this->wc_ = &wcSrc;
            return *this;
        }

        /// Get read-only access to cell-level contributions.
        const PAvgDynamicSourceData<double>& wellBlocks() const
        {
            return *this->wb_;
        }

        /// Get read-only access to connection-level contributions.
        const PAvgDynamicSourceData<double>& wellConns() const
        {
            return *this->wc_;
        }

    private:
        /// Cell-level contributions.
        const PAvgDynamicSourceData<double>* wb_{nullptr};

        /// Connection-level contributions.
        const PAvgDynamicSourceData<double>* wc_{nullptr};
    };

    /// Constructor
    ///
    /// \param[in] cellIndexMap Cell index triple map ((I,J,K) <-> global).
    ///
    /// \param[in] connections List of reservoir connections for single
    ///   well.
    PAvgCalculator(const GridDims&        cellIndexMap,
                   const WellConnections& connections);

    /// Destructor.
    virtual ~PAvgCalculator();

    /// Finish construction by pruning inactive cells.
    ///
    /// \param[in] isActive Linearised predicate for whether or not given
    ///   cell amongst \code allWBPCells() \endcode is actually active in
    ///   the model.
    ///
    ///   Assumed to have the same size--number of elements--as the return
    ///   value from member function \code allWBPCells() \endcode, and
    ///   organise its elements such that \code isActive[i] \endcode holds
    ///   the active status of \code allWBPCells()[i] \endcode.
    void pruneInactiveWBPCells(const std::vector<bool>& isActive);

    /// Compute block-average well-level pressure values from collection of
    /// source contributions and user-defined averaging procedure controls.
    ///
    /// \param[in] sources Connection and cell-level raw data.
    ///
    /// \param[in] controls Averaging procedure controls.
    ///
    /// \param[in] gravity Strength of gravity in SI units [m/s^2].
    ///
    /// \param[in] refDepth Well's reference depth for block-average
    ///   pressure calculation.  Often, but not always, equal to the well's
    ///   bottom-hole pressure reference depth.
    void inferBlockAveragePressures(const Sources& sources,
                                    const PAvg&    controls,
                                    const double   gravity,
                                    const double   refDepth);

    /// List of all cells, global indices in natural ordering, that
    /// contribute to the block-average pressures in this well.
    const std::vector<std::size_t>& allWBPCells() const
    {
        return this->contributingCells_;
    }

    /// List all reservoir connections that potentially contribute to this
    /// block-averaging pressure calculation.
    ///
    /// Convenience method only.  Mainly intended to aid in constructing
    /// PAvgDynamicSourceData objects for the current well's reservoir
    /// connections.
    ///
    /// \return Vector of the numbers 0 .. n-1 in increasing order with n
    ///   being the number of connections in the input set provided to the
    ///   object constructor.
    std::vector<std::size_t> allWellConnections() const;

    /// Block-average pressure derived from selection of source cells.
    ///
    /// \param[in] mode Source cell selection.
    ///
    /// \return Block-average pressure
    const Result& averagePressures() const
    {
        return this->averagePressures_;
    }

protected:
    /// Accumulate weighted running averages of cell contributions to WBP
    class Accumulator
    {
    public:
        /// Collection of running averages and their associate weights.
        ///
        /// Intended primarily as a means of exchanging intermediate results
        /// in a parallel run.
        using LocalRunningAverages = std::array<double, 8>;

        /// Constructor
        Accumulator();

        /// Destructor
        ~Accumulator();

        /// Copy constructor
        ///
        /// \param[in] rhs Source object
        Accumulator(const Accumulator& rhs);

        /// Move constructor
        ///
        /// \param[inout] rhs Source object.  Nullified on exit.
        Accumulator(Accumulator&& rhs);

        /// Assignment operator
        ///
        /// \param[in] rhs Source object
        Accumulator& operator=(const Accumulator& rhs);

        /// Move assignment operator
        ///
        /// \param[inout] rhs Source object.  Nullified on exit.
        Accumulator& operator=(Accumulator&& rhs);

        /// Add contribution from centre/connecting cell
        ///
        /// \param[in] weight Pressure weighting factor
        /// \param[in] press Pressure value
        /// \return \code *this \endcode
        Accumulator& addCentre(const double weight,
                               const double press);

        /// Add contribution from direct, rectangular, level 1 neighbouring
        /// cell
        ///
        /// \param[in] weight Pressure weighting factor
        /// \param[in] press Pressure value
        /// \return \code *this \endcode
        Accumulator& addRectangular(const double weight,
                                    const double press);

        /// Add contribution from diagonal, level 2 neighbouring cell
        ///
        /// \param[in] weight Pressure weighting factor
        /// \param[in] press Pressure value
        /// \return \code *this \endcode
        Accumulator& addDiagonal(const double weight,
                                 const double press);

        /// Add contribution from other accumulator
        ///
        /// This typically incorporates a set of results from a single
        /// reservoir connection into a larger sum across all connections.
        ///
        /// \param[in] weight Pressure weighting factor
        /// \param[in] other Contribution from other accumulation process.
        /// \return \code *this \endcode
        Accumulator& add(const double       weight,
                         const Accumulator& other);

        /// Zero out/clear WBP result buffer
        void prepareAccumulation();

        /// Zero out/clear WBP term buffer
        void prepareContribution();

        /// Accumulate current source term into result buffer whilst
        /// applying any user-prescribed term weighting.
        ///
        /// \param[in] innerWeight Weighting factor for inner/connecting
        ///   cell contributions.  Outer cells weighted by 1-innerWeight
        ///   where applicable.  If inner weight factor is negative, no
        ///   weighting is applied.  Typically the F1 weighting factor from
        ///   the WPAVE keyword.  Default value (-1) mainly applicable to
        ///   PV-weighted accumulations.
        void commitContribution(const double innerWeight = -1.0);

        // Please note that member functions \c getRunningAverages() and \c
        // assignRunningAverages() are concessions to parallel/MPI runs, and
        // especially for simulation runs with distributed wells.  In this
        // situation we need a way to access, communicate/collect/sum, and
        // assign partial results.  Moreover, the \c LocalRunningAverages
        // should be treated opaquely apart from applying a global reduction
        // operation.  In other words, the intended/expected use case is
        //
        //   Accumulator a{}
        //   ...
        //   auto avg = a.getRunningAverages()
        //   MPI_Allreduce(avg, MPI_SUM)
        //   a.assignRunningAverages(avg)
        //
        // Any other use is probably a bug and the above is the canonical
        // implementation of member function collectGlobalContributions() in
        // MPI aware sub classes of PAvgCalculator.

        /// Get buffer of intermediate, local results.
        LocalRunningAverages getRunningAverages() const;

        /// Assign coalesced/global contributions
        ///
        /// \param[in] avg Buffer of coalesced global contributions.
        void assignRunningAverages(const LocalRunningAverages& avg);

        /// Calculate final WBP results from individual contributions
        ///
        /// \return New result object.
        Result getFinalResult() const;

    private:
        /// Implementation class
        class Impl;

        /// Pointer to implementation object.
        std::unique_ptr<Impl> pImpl_;
    };

    /// Average pressures weighted by connection transmissibility factor.
    Accumulator accumCTF_{};

    /// Average pressures weighted by pore-volume
    Accumulator accumPV_{};

private:
    /// Type representing enumeration of locally contributing cells.
    using ContrIndexType = std::vector<std::size_t>::size_type;

    /// Type for translating (linearised) global cell indices to enumerated
    /// local, contributing cells.
    ///
    /// Only used during construction/setup.
    using SetupMap = std::unordered_map<std::size_t, ContrIndexType>;

    /// Type of neighbour of connection's connecting cell.
    enum class NeighbourKind
    {
        /// Neighbour is of the direct, rectangular, level 1 kind.
        Rectangular,

        /// Neighbour is of the diagonal level 2 kind.
        Diagonal,
    };

    /// Well's reservoir connection, stripped to hold only information
    /// necessary to infer block-averaged pressures.
    struct PAvgConnection
    {
        /// Constructor.
        ///
        /// \param[in] ctf_arg Connection's transmissiblity factor
        ///
        /// \param[in] depth_arg Connection's depth
        ///
        /// \param[in] cell_arg Connection's connecting cell.  Enumerated
        ///   local contributing cell.
        PAvgConnection(const double         ctf_arg,
                       const double         depth_arg,
                       const ContrIndexType cell_arg)
            : ctf  (ctf_arg)
            , depth(depth_arg)
            , cell (cell_arg)
        {}

        /// Connection transmissiblity factor.
        double ctf{};

        /// Connection's depth.
        double depth{};

        /// Index into \c contributingCells_ of connection's cell.
        ContrIndexType cell{};

        /// Connecting cell's immediate (level-1) neighbours
        ///   ((i-1,j), (i+1,j), (i,j-1), and (i,j+1))
        /// Indices into \c contributingCells_.
        std::vector<ContrIndexType> rectNeighbours{};

        /// Connecting cell's diagnoal (level-2) neighbours
        ///   ((i-1,j-1), (i+1,j-1), (i-1,j+1), and (i+1,j+1))
        /// Indices into \c contributingCells_.
        std::vector<ContrIndexType> diagNeighbours{};
    };

    /// Number of input connections.
    ///
    /// Saved copy of \code .size() \endcode from \c connections constructor
    /// parameter.
    std::vector<PAvgConnection>::size_type numInputConns_{};

    /// Set of well/reservoir connections from which the block-average
    /// pressures derive.
    std::vector<PAvgConnection> connections_{};

    /// List of indices into \c connections_ that represent open connections.
    std::vector<std::vector<PAvgConnection>::size_type> openConns_{};

    /// Map \c connections_ indices to input indices (0..numInputConns_-1).
    ///
    /// In particular, \code connections_[i] \endcode corresponds to input
    /// connection \code inputConn_[i] \endcode.
    ///
    /// Needed to handle connections to inactive cells.  See
    /// pruneInactiveConnections() and connectionPressureOffsetWell() for
    /// details.
    std::vector<std::vector<PAvgConnection>::size_type> inputConn_{};

    /// Collection of all (global) cell indices that potentially contribute
    /// to this block-average well pressure calculation.
    std::vector<std::size_t> contributingCells_{};

    /// Well level pressure values derived from block-averaging procedures.
    ///
    /// Cached end result from \code inferBlockAveragePressures() \endcode.
    Result averagePressures_{};

    /// Include reservoir connection and all direction-dependent level 1 and
    /// level 2 neighbours of connection's connecting cell into known cell
    /// set.
    ///
    /// Writes to \c connections_, \c openConns_, and \c contributingCells_.
    ///
    /// \param[in] grid Collection of active cells.
    ///
    /// \param[in] conn Specific reservoir connection
    ///
    /// \param[inout] setupHelperMap Translation between linearised global
    ///   cell indices and enumerated local contributing cells.  Updated as
    ///   new contributing cells are discovered.
    void addConnection(const GridDims&   cellIndexMap,
                       const Connection& conn,
                       SetupMap&         setupHelperMap);

    /// Finish construction by pruning inactive PAvgConnections.
    ///
    /// \param[in] isActive Linearised predicate for whether or not given
    ///   cell amongst \code allWBPCells() \endcode is actually active in
    ///   the model.
    ///
    ///   Assumed to have the same size--number of elements--as the return
    ///   value from member function \code allWBPCells() \endcode, and
    ///   organise its elements such that \code isActive[i] \endcode holds
    ///   the active status of \code allWBPCells()[i] \endcode.
    void pruneInactiveConnections(const std::vector<bool>& isActive);

    /// Top-level entry point for accumulating local WBP contributions.
    ///
    /// Will dispatch to lower-level entry points depending on control's
    /// flag for whether to average over the set of open or the set of all
    /// reservoir connections.  Writes to \c accumCTF_ and \c accumPV_.
    ///
    /// \param[in] sources Connection and cell-level raw data.
    ///
    /// \param[in] controls Averaging procedure controls.
    ///
    /// \param[in] gravity Strength of gravity in SI units [m/s^2].
    ///
    /// \param[in] refDepth Well's reference depth for block-average
    ///   pressure calculation.  Often, but not always, equal to the well's
    ///   bottom-hole pressure reference depth.
    void accumulateLocalContributions(const Sources& sources,
                                      const PAvg&    controls,
                                      const double   gravity,
                                      const double   refDepth);

    /// Communicate local contributions and collect global (off-rank)
    /// contributions.
    ///
    /// Intended as an MPI-aware customisation point.  Typically a no-op in
    /// a sequential run.
    ///
    /// Reads from and writes to data members \c accumCTF_ and \c accumPV_.
    virtual void collectGlobalContributions();

    /// Form final result object from all accumulated contributions.
    ///
    /// Writes to \c averagePressures_.
    ///
    /// \param[in] controls Averaging procedure controls.  Needed for
    ///   weighting factor F2 (== conn_weight()) between CTF-weighted
    ///   average (\c accumCTF_) and PV-weighted average (\c accumPV_).
    void assignResults(const PAvg& controls);

    /// Include individual neighbour of currently latest connection's
    /// connecting cell into known cell set.
    ///
    /// Writes to \code connections_.back() \endcode and \c
    /// contributingCells_.
    ///
    /// \param[in] neighbour Global, linearised cell index.  Nullopt if
    ///    neighbour happens to be in an inactive cell or outside the
    ///    model's Cartesian dimensions.  In that case, no change is made to
    ///    any data member.
    ///
    /// \param[in] neighbourKind Classification for \p neighbour.
    ///
    /// \param[inout] setupHelperMap Translation between linearised global
    ///   cell indices and enumerated local contributing cells.  Updated as
    ///   new contributing cells are discovered.
    void addNeighbour(std::optional<std::size_t> neighbour,
                      NeighbourKind              neighbourKind,
                      SetupMap&                  setupHelperMap);

    /// Global index of currently latest connection's connecting cell.
    ///
    /// Convenience function for inferring connecting cell's IJK index.
    std::size_t lastConnsCell() const;

    /// Include all level 1 and level 2 neighbours orthogonal to X axis of
    /// currently latest connection's connecting cell into known cell set.
    ///
    /// Writes to \code connections_.back() \endcode and \c
    /// contributingCells_.
    ///
    /// \param[in] grid Collection of active cells.
    ///
    /// \param[inout] setupHelperMap Translation between linearised global
    ///   cell indices and enumerated local contributing cells.  Updated as
    ///   new contributing cells are discovered.
    void addNeighbours_X(const GridDims& grid, SetupMap& setupHelperMap);

    /// Include all level 1 and level 2 neighbours orthogonal to Y axis of
    /// currently latest connection's connecting cell into known cell set.
    ///
    /// Writes to \code connections_.back() \endcode and \c
    /// contributingCells_.
    ///
    /// \param[in] grid Collection of active cells.
    ///
    /// \param[inout] setupHelperMap Translation between linearised global
    ///   cell indices and enumerated local contributing cells.  Updated as
    ///   new contributing cells are discovered.
    void addNeighbours_Y(const GridDims& grid, SetupMap& setupHelperMap);

    /// Include all level 1 and level 2 neighbours orthogonal to Z axis of
    /// currently latest connection's connecting cell into known cell set.
    ///
    /// Writes to \code connections_.back() \endcode and \c
    /// contributingCells_.
    ///
    /// \param[in] grid Collection of active cells.
    ///
    /// \param[inout] setupHelperMap Translation between linearised global
    ///   cell indices and enumerated local contributing cells.  Updated as
    ///   new contributing cells are discovered.
    void addNeighbours_Z(const GridDims& grid, SetupMap& setupHelperMap);

    /// Calculation routine for accumulating local WBP contributions.
    ///
    /// \tparam ConnIndexMap Callable type translating from requested set of
    ///   connections to linear index into all known reservoir connections.
    ///   Must provide a call operator such that
    /// \code
    ///   connections_[ix(i)]
    /// \endcode
    ///   is well formed for an object \c ix of type \p ConnIndexMap and an
    ///   index \c i in 0 .. n-1, with 'n' being the number of connections
    ///   in the current connection subset.  Will typically just be the
    ///   identity mapping \code [](i){return i} \endcode or the open
    ///   connection mapping \code [](i){return openConns_[i]} \endcode.
    ///
    /// \tparam CTFPressureWeightFunction Callable type generating term
    ///   weighting values for the CTF-weighted connection contributions.
    ///   Must provide a call operator such that
    /// \code
    ///   double w = weight(src)
    /// \endcode
    ///   is well formed for an object \c weight of type \p
    ///   CTFPressureWeightFunction and a \c src object of type \code
    ///   PAvgDynamicSourceData::SourceDataSpan<const double> \endcode.
    ///   Will typically be a lambda that returns the pore volume in \c src
    ///   if the weighting factor F1 in WPAVE is negative, or a lambda that
    ///   just returns the number one (1.0) otherwise.
    ///
    /// \param[in] sources Connection and cell-level raw data.
    ///
    /// \param[in] controls Averaging procedure controls.
    ///
    /// \param[in] connDP Pressure correction term for each reservoir
    ///   connection.
    ///
    /// \param[in] connIndex Translation method from active connection index
    ///   to index into all known reservoir connections.
    ///
    /// \param[in] ctfPressWeight Pressure weighting method for CTF term's
    ///   individual contributions.
    template <typename ConnIndexMap, typename CTFPressureWeightFunction>
    void accumulateLocalContributions(const Sources&             sources,
                                      const PAvg&                controls,
                                      const std::vector<double>& connDP,
                                      ConnIndexMap               connIndex,
                                      CTFPressureWeightFunction  ctfPressWeight);

    /// Final dispatch level before going to calculation routine which
    /// accumulates the local WBP contributions.
    ///
    /// Dispatches based on sign of averaging procedure's F1 weighting term
    /// (== inner_weight()).  If F1 < 0, then invoke calculation routine
    /// with a pore-volume based weighting of the individual cell
    /// contributions.  Otherwise, use unit weighting of the individual cell
    /// contributions and weight the accumulated CTF contributions for a
    /// single reservoir connection according to F1.
    ///
    /// \tparam ConnIndexMap Callable type translating from requested set of
    ///   connections to linear index into all known reservoir connections.
    ///   Must provide a call operator such that
    /// \code
    ///   connections_[ix(i)]
    /// \endcode
    ///   is well formed for an object \c ix of type \p ConnIndexMap and an
    ///   index \c i in 0 .. n-1, with 'n' being the number of connections
    ///   in the current connection subset.  Will typically just be the
    ///   identity mapping \code [](i){return i} \endcode or the open
    ///   connection mapping \code [](i){return openConns_[i]} \endcode.
    ///
    /// \param[in] sources Connection and cell-level raw data.
    ///
    /// \param[in] controls Averaging procedure controls.
    ///
    /// \param[in] connDP Pressure correction term for each reservoir
    ///   connection.
    ///
    /// \param[in] connIndex Translation method from active connection index
    ///   to index into all known reservoir connections.
    template <typename ConnIndexMap>
    void accumulateLocalContributions(const Sources&             sources,
                                      const PAvg&                controls,
                                      const std::vector<double>& connDP,
                                      ConnIndexMap&&             connIndex);

    /// First dispatch level before going to calculation routine which
    /// accumulates the local WBP contributions.
    ///
    /// Invokes final dispatch level on set of open connections only.
    ///
    /// \param[in] sources Connection and cell-level raw data.
    ///
    /// \param[in] controls Averaging procedure controls.
    ///
    /// \param[in] connDP Pressure correction term for each reservoir
    ///   connection.
    void accumulateLocalContribOpen(const Sources&             sources,
                                    const PAvg&                controls,
                                    const std::vector<double>& connDP);

    /// First dispatch level before going to calculation routine which
    /// accumulates the local WBP contributions.
    ///
    /// Invokes final dispatch level on set of all known connections.
    ///
    /// \param[in] sources Connection and cell-level raw data.
    ///
    /// \param[in] controls Averaging procedure controls.
    ///
    /// \param[in] connDP Pressure correction term for each reservoir
    ///   connection.
    void accumulateLocalContribAll(const Sources&             sources,
                                   const PAvg&                controls,
                                   const std::vector<double>& connDP);

    /// Compute pressure correction term/offset using Well method
    ///
    /// Uses mixture density from well bore.
    ///
    /// \tparam ConnIndexMap Callable type translating from requested set of
    ///   connections to linear index into all known reservoir connections.
    ///   Must provide a call operator such that
    /// \code
    ///   connections_[ix(i)]
    /// \endcode
    ///   is well formed for an object \c ix of type \p ConnIndexMap and an
    ///   index \c i in 0 .. n-1, with 'n' being the number of connections
    ///   in the current connection subset.  Will typically just be the
    ///   identity mapping \code [](i){return i} \endcode or the open
    ///   connection mapping \code [](i){return openConns_[i]} \endcode.
    ///
    /// \param[in] nconn Number of elements in active connection subset.
    ///
    /// \param[in] sources Connection and cell-level raw data.
    ///
    /// \param[in] gravity Strength of gravity in SI units [m/s^2].
    ///
    /// \param[in] refDepth Well's reference depth for block-average
    ///   pressure calculation.  Often, but not always, equal to the well's
    ///   bottom-hole pressure reference depth.
    ///
    /// \param[in] connIndex Translation method from active connection index
    ///   to index into all known reservoir connections.
    ///
    /// \return Pressure correction term for each active connection.
    template <typename ConnIndexMap>
    std::vector<double>
    connectionPressureOffsetWell(const std::size_t nconn,
                                 const Sources&    sources,
                                 const double      gravity,
                                 const double      refDepth,
                                 ConnIndexMap      connIndex) const;

    /// Compute pressure correction term/offset using Reservoir method
    ///
    /// Uses pore-volume weighted mixture density from connecting cell and
    /// its level 1 and level 2 neighbours.
    ///
    /// \tparam ConnIndexMap Callable type translating from requested set of
    ///   connections to linear index into all known reservoir connections.
    ///   Must provide a call operator such that
    /// \code
    ///   connections_[ix(i)]
    /// \endcode
    ///   is well formed for an object \c ix of type \p ConnIndexMap and an
    ///   index \c i in 0 .. n-1, with 'n' being the number of connections
    ///   in the current connection subset.  Will typically just be the
    ///   identity mapping \code [](i){return i} \endcode or the open
    ///   connection mapping \code [](i){return openConns_[i]} \endcode.
    ///
    /// \param[in] nconn Number of elements in active connection subset.
    ///
    /// \param[in] sources Connection and cell-level raw data.
    ///
    /// \param[in] gravity Strength of gravity in SI units [m/s^2].
    ///
    /// \param[in] refDepth Well's reference depth for block-average
    ///   pressure calculation.  Often, but not always, equal to the well's
    ///   bottom-hole pressure reference depth.
    ///
    /// \param[in] connIndex Translation method from active connection index
    ///   to index into all known reservoir connections.
    ///
    /// \return Pressure correction term for each active connection.
    template <typename ConnIndexMap>
    std::vector<double>
    connectionPressureOffsetRes(const std::size_t nconn,
                                const Sources&    sources,
                                const double      gravity,
                                const double      refDepth,
                                ConnIndexMap      connIndex) const;

    /// Top-level entry point for computing the pressure correction
    /// term/offset of each active reservoir connection
    ///
    /// Will dispatch to lower level calculation routines based on algorithm
    /// selection parameter in procedure controls.
    ///
    /// \param[in] sources Connection and cell-level raw data.
    ///
    /// \param[in] controls Averaging procedure controls.  This function uses
    ///   the depth correction and open connections flags.
    ///
    /// \param[in] gravity Strength of gravity in SI units [m/s^2].
    ///
    /// \param[in] refDepth Well's reference depth for block-average
    ///   pressure calculation.  Often, but not always, equal to the well's
    ///   bottom-hole pressure reference depth.
    ///
    /// \return Pressure correction term for each active connection.
    std::vector<double>
    connectionPressureOffset(const Sources& sources,
                             const PAvg&    controls,
                             const double   gravity,
                             const double   refDepth) const;
};

/// Form linear combination of WBP result objects.
///
/// Typically the very last step of computing the block-averaged well
/// pressure values; namely a weighted averaged of the CTF-weighted and the
/// PV-weighted contributions.
///
/// \param[in] alpha Coefficient in linear combination.  Typically the F2
///    weighting factor from the WPAVE (or WWPAVE) keyword.
///
/// \param[in] x First WBP result.  Typically the CTF-weighted WBP result.
///
/// \param[in] beta Coefficient in linear combination.  Typically 1-F2, with
///    F2 from WPAVE.
///
/// \param[in] y Second WBP result.  Typically the PV-weighted WBP result.
///
/// \return \code alpha*x + beta*y \endcode.
PAvgCalculator::Result
linearCombination(const double alpha, PAvgCalculator::Result        x,
                  const double beta , const PAvgCalculator::Result& y);

} // namespace Opm

#endif // PAVG_CALCULATOR_HPP
