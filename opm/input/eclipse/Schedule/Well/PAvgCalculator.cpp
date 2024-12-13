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

#include <opm/input/eclipse/Schedule/Well/PAvgCalculator.hpp>

#include <opm/input/eclipse/EclipseState/Grid/GridDims.hpp>

#include <opm/input/eclipse/Schedule/Well/Connection.hpp>
#include <opm/input/eclipse/Schedule/Well/PAvg.hpp>
#include <opm/input/eclipse/Schedule/Well/PAvgDynamicSourceData.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <cstdio>

#include <fmt/format.h>

namespace {

/// Get linearised, global cell ID from IJK tuple
///
/// \param[in] cellIndexMap Cell index triple map ((I,J,K) <-> global).
///
/// \param[in] i Cell's global Cartesian I index
/// \param[in] j Cell's global Cartesian J index
/// \param[in] k Cell's global Cartesian K index
///
/// \return Linearised global cell ID.  Nullopt if any of the IJK indices
///   are out of bounds or if cell (I,J,K) is inactive.
std::optional<std::size_t>
globalCellIndex(const Opm::GridDims& cellIndexMap,
                const std::size_t    i,
                const std::size_t    j,
                const std::size_t    k)
{
    if ((i >= cellIndexMap.getNX()) ||
        (j >= cellIndexMap.getNY()) ||
        (k >= cellIndexMap.getNZ()))
    {
        return {};
    }

    return cellIndexMap.getGlobalIndex(i, j, k);
}

/// Compute pressure correction/offset
///
/// \param[in] density Mixture density (kg/m^3)
///
/// \param[in] depth Connection depth (m)
///
/// \param[in] gravity Strength of gravity acceleration (m/s^2)
///
/// \param[in] refDepth Reference depth to which to gravity correct a
///   pressure value (m)
template<class Scalar>
Scalar pressureOffset(const Scalar density,
                      const Scalar depth,
                      const Scalar gravity,
                      const Scalar refDepth)
{
    return density * (refDepth - depth) * gravity;
}

template <typename T, typename W>
class WeightedRunningAverage;

template <typename T, typename W>
T value(const WeightedRunningAverage<T, W>& avg);

/// Maintain a running sum with compensated (Kahan) summation
///
///   https://en.wikipedia.org/wiki/Kahan_summation_algorithm
///
/// \tparam T Element type.  Typically built-in arithmetic type like \c
///   double.
template <typename T>
class RunningCompensatedSummation
{
public:
    /// Implicit conversion to T when reading the value.
    operator const T&() const { return this->value_; }

    // Writing should be explicit at the call site.
    T& mutableValue() { return this->value_; }

    /// Accumulation operator.
    ///
    /// Disregards error contribution from sum of other terms.
    ///
    /// \param[in] other Sum of other terms.
    ///
    /// \return \code *this \endcode
    RunningCompensatedSummation&
    operator+=(const RunningCompensatedSummation& other)
    {
        // Intentionally discard other.err_.
        return *this += other.value_;
    }

    /// Accumulation operator
    ///
    /// Accumulates term and estimates error in new sum.
    ///
    /// \param[in] x New term
    ///
    /// \return \code *this \endcode
    RunningCompensatedSummation& operator+=(const T& x)
    {
        const auto t = this->value_;

        this->err_   += x;
        this->value_  = t + this->err_;
        this->err_   += t - this->value_;

        return *this;
    }

    /// Multiplication operator
    ///
    /// Multiplies sum value, leaves error intact.
    ///
    /// \param[in] alpha Scalar factor
    ///
    /// \return \code *this \endcode
    RunningCompensatedSummation& operator*=(const T& alpha)
    {
        this->value_ *= alpha;

        return *this;
    }

    /// Zero out sum value and error estimate
    void clear()
    {
        this->value_ = T{};
        this->err_ = T{};
    }

private:
    /// Sum value
    T value_{};

    /// Error estimate
    T err_{};
};

/// Maintain a weighted running average with compensated summation
///
/// \tparam T Element type
///
/// \tparam W Weighting type.  Typically a built-in arithmetic type such as
///   \c double.
template <typename T, typename W = double>
class WeightedRunningAverage
{
public:
    /// Zero out value and weight members
    void clear()
    {
        this->sum_.clear();
        this->weight_.clear();
    }

    /// Accumulate weighted term into current sum
    ///
    /// \param[in] x Term
    /// \param[in] w Weight of \p x when included in current sum
    /// \return \code *this \endcode
    WeightedRunningAverage& add(const T& x, const W& w = W{1})
    {
        this->sum_    += static_cast<T>(w * x);
        this->weight_ += w;

        return *this;
    }

    /// Accumulate other weighted running average into current sum, while
    /// applying a new weight to the average value.
    ///
    /// \param[in] x Weigthed running average
    ///
    /// \param[in] w Weight applied to value of \p x when included in
    ///   current sum.
    ///
    /// \return \code *this \endcode
    WeightedRunningAverage& add(const WeightedRunningAverage& x, const W& w)
    {
        return this->add(value(x), w);
    }

    /// Multiplication operator
    ///
    /// Multiplies value by constant factor
    ///
    /// \param[in] alpha Multiplication factor
    /// \return \code *this \endcode
    WeightedRunningAverage& operator*=(const W& alpha)
    {
        this->sum_ *= alpha;

        return *this;
    }

    /// Accumulate weighted running average into current average
    ///
    /// \param[in] other Other weighted running average
    /// \return \code *this \endcode
    WeightedRunningAverage& operator+=(const WeightedRunningAverage& other)
    {
        this->sum_    += other.sum_;
        this->weight_ += other.weight_;

        return *this;
    }

    /// Get read/write access to weighted running sum value
    T& mutableSum() { return this->sum_.mutableValue(); }

    /// Get read/write access to total accumulated weight
    W& mutableWeight() { return this->weight_.mutableValue(); }

    /// Get read-only access to weighted running sum value
    const T& sum()  const { return this->sum_; }

    /// Get read-only access to total accumulated weight
    const W& weight() const { return this->weight_; }

private:
    /// Weighted running sum value
    RunningCompensatedSummation<T> sum_{};

    /// Total accumulated weight
    RunningCompensatedSummation<W> weight_{};
};

/// Calculate value of weighted running average
///
/// \tparam T Element type
///
/// \tparam W Weighting type.  Typically a built-in arithmetic type such as
///   \c double.
///
/// \param[in] avg Weighted running average
///
/// \return Value of weighted running average
template <typename T, typename W>
T value(const WeightedRunningAverage<T, W>& avg)
{
    using std::abs;

    const auto w = avg.weight();

    return (abs(w) > W{})
        ? avg.sum() / w
        : T{};
}

/// Zero out collection of weighted running averages
///
/// \tparam T Element type
///
/// \tparam W Weighting type.  Typically a built-in arithmetic type such as
///   \c double.
///
/// \tparam N Number of averages in collection
///
/// \param[in] avg Collection of weighted running averages
template <typename T, typename W, std::size_t N>
void clear(std::array<WeightedRunningAverage<T,W>, N>& avg)
{
    for (auto& a : avg) {
        a.clear();
    }
}

} // Anonymous namespace

namespace Opm {

/// Implementation class for calculator's accumulator
template<class Scalar>
class PAvgCalculator<Scalar>::Accumulator::Impl
{
public:
    /// Add contribution from centre/connecting cell
    ///
    /// \param[in] weight Pressure weighting factor
    /// \param[in] press Pressure value
    void addCentre(const Scalar weight, const Scalar press)
    {
        this->term_[0].add(press, weight);
    }

    /// Add contribution from direct, rectangular, level 1 neighbouring cell
    ///
    /// \param[in] weight Pressure weighting factor
    /// \param[in] press Pressure value
    void addRectangular(const Scalar weight, const Scalar press)
    {
        this->term_[1].add(press, weight);
    }

    /// Add contribution from diagonal, level 2 neighbouring cell
    ///
    /// \param[in] weight Pressure weighting factor
    /// \param[in] press Pressure value
    void addDiagonal(const Scalar weight, const Scalar press)
    {
        this->term_[2].add(press, weight);
    }

    /// Add contribution from other accumulator
    ///
    /// This typically incorporates a set of results from a single reservoir
    /// connection into a larger sum across all connections.
    ///
    /// \param[in] weight Pressure weighting factor
    /// \param[in] other Contribution from other accumulation process.
    void add(const Scalar weight, const Impl& other)
    {
        for (auto i = 0*this->avg_.size(); i < this->avg_.size(); ++i) {
            this->avg_[i].add(other.avg_[i], weight);
        }
    }

    /// Zero out/clear WBP result buffer
    void prepareAccumulation()
    {
        clear(this->avg_);
    }

    /// Zero out/clear WBP term buffer
    void prepareContribution()
    {
        clear(this->term_);
    }

    /// Accumulate current source term into result buffer whilst applying
    /// any user-prescribed term weighting.
    ///
    /// \param[in] innerWeight Weighting factor for inner/connecting cell
    ///   contributions.  Outer cells weighted by 1-innerWeight where
    ///   applicable.  If inner weight factor is negative, no weighting is
    ///   applied.  Typically the F1 weighting factor from the WPAVE
    ///   keyword.
    void commitContribution(const Scalar innerWeight)
    {
        // 1 = Inner only, no weighting
        this->avg_[0] += this->term_[0];

        // 4 = Rectangular only, no weighting
        this->avg_[1] += this->term_[1];

        if (innerWeight < 0.0) {
            // No term weighting.  Avg_[2] (5) and avg_[3] (9) are direct
            // sums of two or more term contributions.
            this->combineDirect();
        }
        else {
            // Term weighting applies to quantites that combine inner and
            // outer blocks (neighbours).
            this->combineWeighted(innerWeight);
        }
    }

    /// Get buffer of intermediate, local results.
    LocalRunningAverages getRunningAverages() const
    {
        LocalRunningAverages a{};

        auto j = 0*a.size();
        for (const auto& avg : this->avg_) {
            a[j++] = avg.sum();
            a[j++] = avg.weight();
        }

        return a;
    }

    /// Assign coalesced/global contributions
    ///
    /// \param[in] avg Buffer of coalesced global contributions.
    void assignRunningAverages(const LocalRunningAverages& a)
    {
        auto j = 0*a.size();
        for (auto& avg : this->avg_) {
            avg.mutableSum()    = a[j++];
            avg.mutableWeight() = a[j++];
        }
    }

    /// Retrieve block-average pressure value of specific type
    ///
    /// \param[in] type Kind of block-average pressure
    ///
    /// \return Value of specified block-average pressure quantity
    Scalar getAverageValue(const typename PAvgCalculatorResult<Scalar>::WBPMode type) const
    {
        return value(this->avg_[static_cast<std::size_t>(type)]);
    }

private:
    /// Result array.
    ///
    /// Combinations of term contributions.  Indices mapped as follows
    ///
    ///   [0] -> WBP  == Centre block
    ///   [1] -> WBP4 == Rectangular neighbours
    ///   [2] -> WBP5 == Inner + Rectangular
    ///   [3] -> WBP9 == Inner + Rectangular + Diagonal
    std::array<WeightedRunningAverage<Scalar>, 4> avg_{};

    /// Term contributions
    ///
    /// Indices mapped as follows
    ///
    ///   [0] -> Centre block
    ///   [1] -> Rectangular neighbours
    ///   [2] -> Diagonal neighbours
    std::array<WeightedRunningAverage<Scalar>, 3> term_{};

    /// Subsume combinations of term values into block-averaged pressures
    ///
    /// No term weighting applied.  Writes to avg_[2] and avg_[3].
    void combineDirect()
    {
        // 5 = Inner + rectangular
        this->avg_[2] += this->term_[0];
        this->avg_[2] += this->term_[1];

        // 9 = Inner + rectangular + diagonal
        this->avg_[3] += this->term_[0];
        this->avg_[3] += this->term_[1];
        this->avg_[3] += this->term_[2];
    }

    /// Subsume weighted combinations of term values into block-averaged
    /// pressures
    ///
    /// Writes to avg_[2] and avg_[3].
    ///
    /// \param[in] innerWeight Weighting factor for inner/connecting cell
    ///   contributions.  Outer cells weighted by 1-innerWeight where
    ///   applicable.  Typically the F1 weighting factor from the WPAVE
    ///   keyword.
    void combineWeighted(const Scalar innerWeight)
    {
        // WBP5 = w*Centre + (1-w)*Rectangular
        this->avg_[2]
            .add(this->term_[0],       innerWeight)
            .add(this->term_[1], 1.0 - innerWeight);

        // WBP9 = w*Centre + (1-w)*(Rectangular + Diagonal)
        auto outer = this->term_[1];
        outer     += this->term_[2];
        this->avg_[3]
            .add(this->term_[0],       innerWeight)
            .add(outer         , 1.0 - innerWeight);
    }
};

template<class Scalar>
PAvgCalculator<Scalar>::Accumulator::Accumulator()
    : pImpl_ { std::make_unique<Impl>() }
{}

template<class Scalar>
PAvgCalculator<Scalar>::Accumulator::~Accumulator() = default;

template<class Scalar>
PAvgCalculator<Scalar>::Accumulator::Accumulator(const Accumulator& rhs)
    : pImpl_ { std::make_unique<Impl>(*rhs.pImpl_) }
{}

template<class Scalar>
PAvgCalculator<Scalar>::Accumulator::Accumulator(Accumulator&& rhs)
    : pImpl_ { std::move(rhs.pImpl_) }
{}

template<class Scalar>
typename PAvgCalculator<Scalar>::Accumulator&
PAvgCalculator<Scalar>::Accumulator::operator=(const Accumulator& rhs)
{
    this->pImpl_ = std::make_unique<Impl>(*rhs.pImpl_);
    return *this;
}

template<class Scalar>
typename PAvgCalculator<Scalar>::Accumulator&
PAvgCalculator<Scalar>::Accumulator::operator=(Accumulator&& rhs)
{
    this->pImpl_ = std::move(rhs.pImpl_);
    return *this;
}

template<class Scalar>
typename PAvgCalculator<Scalar>::Accumulator&
PAvgCalculator<Scalar>::Accumulator::addCentre(const Scalar weight,
                                               const Scalar press)
{
    this->pImpl_->addCentre(weight, press);
    return *this;
}

template<class Scalar>
typename PAvgCalculator<Scalar>::Accumulator&
PAvgCalculator<Scalar>::Accumulator::addRectangular(const Scalar weight,
                                                    const Scalar press)
{
    this->pImpl_->addRectangular(weight, press);
    return *this;
}

template<class Scalar>
typename PAvgCalculator<Scalar>::Accumulator&
PAvgCalculator<Scalar>::Accumulator::addDiagonal(const Scalar weight,
                                                 const Scalar press)
{
    this->pImpl_->addDiagonal(weight, press);
    return *this;
}

template<class Scalar>
typename PAvgCalculator<Scalar>::Accumulator&
PAvgCalculator<Scalar>::Accumulator::add(const Scalar       weight,
                                         const Accumulator& other)
{
    this->pImpl_->add(weight, *other.pImpl_);
    return *this;
}

template<class Scalar>
void PAvgCalculator<Scalar>::Accumulator::prepareAccumulation()
{
    this->pImpl_->prepareAccumulation();
}

template<class Scalar>
void PAvgCalculator<Scalar>::Accumulator::prepareContribution()
{
    this->pImpl_->prepareContribution();
}

template<class Scalar>
void PAvgCalculator<Scalar>::Accumulator::commitContribution(const Scalar innerWeight)
{
    this->pImpl_->commitContribution(innerWeight);
}

template<class Scalar>
typename PAvgCalculator<Scalar>::Accumulator::LocalRunningAverages
PAvgCalculator<Scalar>::Accumulator::getRunningAverages() const
{
    return this->pImpl_->getRunningAverages();
}

template<class Scalar>
void PAvgCalculator<Scalar>::Accumulator::
assignRunningAverages(const LocalRunningAverages& avg)
{
    this->pImpl_->assignRunningAverages(avg);
}

template <class Scalar>
PAvgCalculatorResult<Scalar>
PAvgCalculator<Scalar>::Accumulator::getFinalResult() const
{
    auto result = PAvgCalculatorResult<Scalar>{};

    for (const auto& type : {
            PAvgCalculatorResult<Scalar>::WBPMode::WBP,
            PAvgCalculatorResult<Scalar>::WBPMode::WBP4,
            PAvgCalculatorResult<Scalar>::WBPMode::WBP5,
            PAvgCalculatorResult<Scalar>::WBPMode::WBP9,
        })
    {
        result.set(type, this->pImpl_->getAverageValue(type));
    }

    return result;
}

// ---------------------------------------------------------------------------

template<class Scalar>
PAvgCalculator<Scalar>::PAvgCalculator(const GridDims&        cellIndexMap,
                                       const WellConnections& connections)
    : numInputConns_ { connections.size() }
{
    this->openConns_.reserve(this->numInputConns_);

    auto setupHelperMap = SetupMap{};

    for (const auto& conn : connections) {
        // addConnection() mutates setupHelperMap
        this->addConnection(cellIndexMap, conn, setupHelperMap);
    }
}

template<class Scalar>
PAvgCalculator<Scalar>::~PAvgCalculator() = default;

template<class Scalar>
void PAvgCalculator<Scalar>::pruneInactiveWBPCells(const std::vector<bool>& isActive)
{
    assert (isActive.size() == this->contributingCells_.size());

    auto allIx = std::vector<ContrIndexType>(isActive.size());
    std::iota(allIx.begin(), allIx.end(), ContrIndexType{0});

    auto activeIx = std::vector<ContrIndexType>{};
    activeIx.reserve(allIx.size());
    std::copy_if(allIx.begin(), allIx.end(), std::back_inserter(activeIx),
                 [&isActive](const auto i) { return isActive[i]; });

    if (activeIx.size() == allIx.size()) {
        // All cells active.  Nothing else to do here.
        return;
    }

    // Filter contributingCells_ down to active cells only.
    {
        auto newWBPCells = std::vector<std::size_t>{};
        newWBPCells.reserve(activeIx.size());

        std::transform(activeIx.begin(), activeIx.end(),
                       std::back_inserter(newWBPCells),
                       [this](const auto& origCell)
                       {
                           return this->contributingCells_[origCell];
                       });

        this->contributingCells_.swap(newWBPCells);
    }

    // Filter connections_ down to active cells only.
    this->pruneInactiveConnections(isActive);

    // Re-map/renumber original element indices to active cells only.
    //
    // 1) Establish new element indices.  Note: This loop leaves zeros in
    // inactive cells.  That's intentional and we must use 'isActive' to
    // filter the 'cell' and '*Neighbours' data members of PAvgConnection.
    auto newIndex = std::vector<ContrIndexType>(allIx.size());
    for (auto i = 0*activeIx.size(); i < activeIx.size(); ++i) {
        newIndex[activeIx[i]] = i;
    }

    const auto neighList = std::array {
        &PAvgConnection::rectNeighbours,
        &PAvgConnection::diagNeighbours,
    };

    // 2) Affect element index renumbering.
    for (auto& conn : this->connections_) {
        conn.cell = newIndex[conn.cell]; // Known to be active.

        for (const auto& neighbours : neighList) {
            auto newNeigbour = std::vector<ContrIndexType>{};
            newNeigbour.reserve((conn.*neighbours).size());

            for (const auto& neighbour : conn.*neighbours) {
                if (isActive[neighbour]) {
                    newNeigbour.push_back(newIndex[neighbour]);
                }
            }

            (conn.*neighbours).swap(newNeigbour);
        }
    }
}

template<class Scalar>
void PAvgCalculator<Scalar>::
inferBlockAveragePressures(const Sources& sources,
                           const PAvg&    controls,
                           const Scalar   gravity,
                           const Scalar   refDepth)
{
    this->accumulateLocalContributions(sources, controls, gravity, refDepth);

    this->collectGlobalContributions();

    this->assignResults(controls);
}

template<class Scalar>
std::vector<std::size_t> PAvgCalculator<Scalar>::
allWellConnections() const
{
    auto ix = std::vector<std::size_t>(this->numInputConns_);
    std::iota(ix.begin(), ix.end(), std::size_t{0});

    return ix;
}

template<class Scalar>
void PAvgCalculator<Scalar>::accumulateLocalContributions(const Sources& sources,
                                                          const PAvg&    controls,
                                                          const Scalar   gravity,
                                                          const Scalar   refDepth)
{
    this->accumCTF_.prepareAccumulation();
    this->accumPV_.prepareAccumulation();

    const auto connDP =
        this->connectionPressureOffset(sources, controls, gravity, refDepth);

    if (controls.open_connections()) {
        this->accumulateLocalContribOpen(sources, controls, connDP);
    }
    else {
        this->accumulateLocalContribAll(sources, controls, connDP);
    }
}

template<class Scalar>
void PAvgCalculator<Scalar>::collectGlobalContributions()
{}

template<class Scalar>
void PAvgCalculator<Scalar>::assignResults(const PAvg& controls)
{
    const auto F2 = static_cast<Scalar>(controls.conn_weight());

    this->averagePressures_ =
        linearCombination(F2    , this->accumCTF_.getFinalResult(),
                          1 - F2, this->accumPV_ .getFinalResult());
}

template<class Scalar>
void PAvgCalculator<Scalar>::
addConnection(const GridDims&   cellIndexMap,
              const Connection& conn,
              SetupMap&         setupHelperMap)
{
    const auto& [localCellPos, newCellInserted] = setupHelperMap
        .emplace(conn.global_index(), this->contributingCells_.size());

    if (newCellInserted) {
        this->contributingCells_.push_back(localCellPos->first);
    }

    if (conn.state() == Connection::State::OPEN) {
        // Must be sequenced before connctions_.emplace_back()
        this->openConns_.push_back(this->connections_.size());
    }

    this->inputConn_.push_back(this->connections_.size());

    this->connections_.emplace_back(conn.CF(), conn.depth(),
                                    localCellPos->second);

    if (conn.dir() == Connection::Direction::X) {
        this->addNeighbours_X(cellIndexMap, setupHelperMap);
    }
    else if (conn.dir() == Connection::Direction::Y) {
        this->addNeighbours_Y(cellIndexMap, setupHelperMap);
    }
    else if (conn.dir() == Connection::Direction::Z) {
        this->addNeighbours_Z(cellIndexMap, setupHelperMap);
    }
}

template<class Scalar>
void PAvgCalculator<Scalar>::pruneInactiveConnections(const std::vector<bool>& isActive)
{
    const auto nConn = this->connections_.size();

    auto keep = std::vector<typename std::vector<PAvgConnection>::size_type>{};
    keep.reserve(nConn);

    for (auto connIx = 0*nConn; connIx < nConn; ++connIx) {
        if (isActive[this->connections_[connIx].cell]) {
            keep.push_back(connIx);
        }
    }

    if (keep.size() == nConn) {
        // All existing connections are active.  Nothing to do.
        return;
    }

    // If we get here, not all existing connections are active.  Filter
    // connections_ and openConns_ down to active set only.

    // 1. Extract active subset of open connections in original numbering.
    {
        auto keepOpen = std::vector<typename std::vector<PAvgConnection>::size_type>{};
        keepOpen.reserve(this->openConns_.size());

        std::copy_if(this->openConns_.begin(),this->openConns_.end(),
                     std::back_inserter(keepOpen),
                     [this, &isActive](const auto& openConn)
                     {
                         return isActive[this->connections_[openConn].cell];
                     });

        this->openConns_.swap(keepOpen);
    }

    // 2. Extract active subset of all connections.  Create all->active
    // index map (newConnIDs) in the process.
    auto newConnIDs = std::vector<typename std::vector<PAvgConnection>::size_type>(nConn);

    {
        auto filteredConns = std::vector<PAvgConnection>{};
        auto filteredInput = std::vector<typename std::vector<PAvgConnection>::size_type>{};
        filteredConns.reserve(keep.size());
        filteredInput.reserve(keep.size());

        for (const auto& keepConn : keep) {
            newConnIDs[keepConn] = filteredConns.size();
            filteredConns.push_back(this->connections_[keepConn]);
            filteredInput.push_back(this->inputConn_[keepConn]);
        }

        this->connections_.swap(filteredConns);
        this->inputConn_  .swap(filteredInput);
    }

    // 3. Renumber set of open connections to match new sequence of active
    // connections_.
    std::transform(this->openConns_.begin(), this->openConns_.end(),
                   this->openConns_.begin(),
                   [&newConnIDs](const auto& openConnID)
                   {
                       return newConnIDs[openConnID];
                   });
}

template<class Scalar>
void PAvgCalculator<Scalar>::
addNeighbour(std::optional<std::size_t> neighbour,
             const NeighbourKind        neighbourKind,
             SetupMap&                  setupHelperMap)
{
    if (! neighbour) {
        return;
    }

    const auto& [localCellPos, newCellInserted] = setupHelperMap
        .emplace(*neighbour, this->contributingCells_.size());

    if (newCellInserted) {
        this->contributingCells_.push_back(localCellPos->first);
    }

    auto& neighbours = (neighbourKind == NeighbourKind::Rectangular)
        ? this->connections_.back().rectNeighbours
        : this->connections_.back().diagNeighbours;

    neighbours.push_back(localCellPos->second);
}

template<class Scalar>
std::size_t PAvgCalculator<Scalar>::lastConnsCell() const
{
    return this->contributingCells_[this->connections_.back().cell];
}

template<class Scalar>
void PAvgCalculator<Scalar>::
addNeighbours_X(const GridDims& cellIndexMap, SetupMap& setupHelperMap)
{
    const auto& [i, j, k] = cellIndexMap.getIJK(this->lastConnsCell());

    this->addNeighbour(globalCellIndex(cellIndexMap, i,j  ,k+1), NeighbourKind::Rectangular, setupHelperMap);
    this->addNeighbour(globalCellIndex(cellIndexMap, i,j  ,k-1), NeighbourKind::Rectangular, setupHelperMap);
    this->addNeighbour(globalCellIndex(cellIndexMap, i,j+1,k),   NeighbourKind::Rectangular, setupHelperMap);
    this->addNeighbour(globalCellIndex(cellIndexMap, i,j-1,k),   NeighbourKind::Rectangular, setupHelperMap);

    this->addNeighbour(globalCellIndex(cellIndexMap, i,j+1,k+1), NeighbourKind::Diagonal, setupHelperMap);
    this->addNeighbour(globalCellIndex(cellIndexMap, i,j+1,k-1), NeighbourKind::Diagonal, setupHelperMap);
    this->addNeighbour(globalCellIndex(cellIndexMap, i,j-1,k+1), NeighbourKind::Diagonal, setupHelperMap);
    this->addNeighbour(globalCellIndex(cellIndexMap, i,j-1,k-1), NeighbourKind::Diagonal, setupHelperMap);
}

template<class Scalar>
void PAvgCalculator<Scalar>::
addNeighbours_Y(const GridDims& cellIndexMap, SetupMap& setupHelperMap)
{
    const auto& [i, j, k] = cellIndexMap.getIJK(this->lastConnsCell());

    this->addNeighbour(globalCellIndex(cellIndexMap, i+1,j,k),   NeighbourKind::Rectangular, setupHelperMap);
    this->addNeighbour(globalCellIndex(cellIndexMap, i-1,j,k),   NeighbourKind::Rectangular, setupHelperMap);
    this->addNeighbour(globalCellIndex(cellIndexMap, i  ,j,k+1), NeighbourKind::Rectangular, setupHelperMap);
    this->addNeighbour(globalCellIndex(cellIndexMap, i  ,j,k-1), NeighbourKind::Rectangular, setupHelperMap);

    this->addNeighbour(globalCellIndex(cellIndexMap, i+1,j,k+1), NeighbourKind::Diagonal, setupHelperMap);
    this->addNeighbour(globalCellIndex(cellIndexMap, i-1,j,k+1), NeighbourKind::Diagonal, setupHelperMap);
    this->addNeighbour(globalCellIndex(cellIndexMap, i+1,j,k-1), NeighbourKind::Diagonal, setupHelperMap);
    this->addNeighbour(globalCellIndex(cellIndexMap, i-1,j,k-1), NeighbourKind::Diagonal, setupHelperMap);
}

template<class Scalar>
void PAvgCalculator<Scalar>::
addNeighbours_Z(const GridDims& cellIndexMap, SetupMap& setupHelperMap)
{
    const auto& [i, j, k] = cellIndexMap.getIJK(this->lastConnsCell());

    this->addNeighbour(globalCellIndex(cellIndexMap, i+1,j  ,k), NeighbourKind::Rectangular, setupHelperMap);
    this->addNeighbour(globalCellIndex(cellIndexMap, i-1,j  ,k), NeighbourKind::Rectangular, setupHelperMap);
    this->addNeighbour(globalCellIndex(cellIndexMap, i  ,j+1,k), NeighbourKind::Rectangular, setupHelperMap);
    this->addNeighbour(globalCellIndex(cellIndexMap, i  ,j-1,k), NeighbourKind::Rectangular, setupHelperMap);

    this->addNeighbour(globalCellIndex(cellIndexMap, i+1,j+1,k), NeighbourKind::Diagonal, setupHelperMap);
    this->addNeighbour(globalCellIndex(cellIndexMap, i-1,j+1,k), NeighbourKind::Diagonal, setupHelperMap);
    this->addNeighbour(globalCellIndex(cellIndexMap, i+1,j-1,k), NeighbourKind::Diagonal, setupHelperMap);
    this->addNeighbour(globalCellIndex(cellIndexMap, i-1,j-1,k), NeighbourKind::Diagonal, setupHelperMap);
}

template<class Scalar>
template <typename ConnIndexMap, typename CTFPressureWeightFunction>
void PAvgCalculator<Scalar>::
accumulateLocalContributions(const Sources&             sources,
                             const PAvg&                controls,
                             const std::vector<Scalar>& connDP,
                             ConnIndexMap               connIndex,
                             CTFPressureWeightFunction  ctfPressWeight)
{
    using PressureTermHandler =
        Accumulator& (Accumulator::*)(const Scalar weight,
                                      const Scalar press);

    // PrepareContribution() is not needed for accumCTF_, but on the other
    // hand does no real harm either.  Keep it for symmetry because we *do*
    // need to prepare accumPV_.
    this->accumCTF_.prepareContribution();
    this->accumPV_ .prepareContribution();

    // Intermediate, per connection results pertaining to CTF-weighted sum.
    auto accumCTF_c = Accumulator{};

    auto addContrib = [&sources, &ctfPressWeight, &accumCTF_c, this]
        (const ContrIndexType i, const Scalar dp, PressureTermHandler handler)
    {
        using Item = typename PAvgDynamicSourceData<Scalar>::template SourceDataSpan<const Scalar>::Item;

        const auto src = sources.wellBlocks()[this->contributingCells_[i]];
        const auto p   = src[Item::Pressure] + dp;

        // Use std::invoke() to simplify the calling syntax here.
        std::invoke(handler, accumCTF_c    , ctfPressWeight(src), p);
        std::invoke(handler, this->accumPV_, src[Item::PoreVol] , p);
    };

    const auto handlers = std::array {
        std::pair { &PAvgConnection::rectNeighbours, &Accumulator::addRectangular },
        std::pair { &PAvgConnection::diagNeighbours, &Accumulator::addDiagonal },
    };

    const auto nconn = connDP.size();
    for (auto connID = 0*nconn; connID < nconn; ++connID) {
        accumCTF_c.prepareAccumulation();
        accumCTF_c.prepareContribution();

        const auto& conn = this->connections_[connIndex(connID)];

        // 1) Connecting cell
        addContrib(conn.cell, connDP[connID], &Accumulator::addCentre);

        // 2) Connecting cell's neighbours.
        for (const auto& [neighbours, handler] : handlers) {
            for (const auto& neighIdx : conn.*neighbours) {
                addContrib(neighIdx, connDP[connID], handler);
            }
        }

        accumCTF_c.commitContribution(controls.inner_weight());

        this->accumCTF_.add(conn.ctf, accumCTF_c);
    }

    // Infer {1,4,5,9} values from {Centre, Rectangular, Diagonal} term
    // contributions in PV-based accumulation.  Must happen before
    // collectGlobalContributions(), and this is a reasonable location.
    this->accumPV_.commitContribution();
}

template<class Scalar>
template <typename ConnIndexMap>
void PAvgCalculator<Scalar>::
accumulateLocalContributions(const Sources&             sources,
                             const PAvg&                controls,
                             const std::vector<Scalar>& connDP,
                             ConnIndexMap&&             connIndex)
{
    if (controls.inner_weight() < 0.0) {
        // F1 < 0 => pore-volume weighting of individual cell contributions,
        // no weighting when commiting term.

        this->accumulateLocalContributions(sources, controls, connDP,
                                           std::forward<ConnIndexMap>(connIndex),
                                           [](const auto& src)
                                           {
                                               using Span = std::remove_cv_t<
                                                   std::remove_reference_t<decltype(src)>>;
                                               using Item = typename Span::Item;

                                               return src[Item::PoreVol];
                                           });
    }
    else {
        // F1 >= 0 => unit weighting of individual cell contributions,
        // F1-weighting when committing term.

        this->accumulateLocalContributions(sources, controls, connDP,
                                           std::forward<ConnIndexMap>(connIndex),
                                           [](const auto&) { return 1.0; });
    }
}

template<class Scalar>
void PAvgCalculator<Scalar>::
accumulateLocalContribOpen(const Sources&             sources,
                           const PAvg&                controls,
                           const std::vector<Scalar>& connDP)
{
    assert (connDP.size() == this->openConns_.size());

    this->accumulateLocalContributions(sources, controls, connDP,
                                       [this](const auto i)
                                       { return this->openConns_[i]; });
}

template<class Scalar>
void PAvgCalculator<Scalar>::
accumulateLocalContribAll(const Sources&             sources,
                          const PAvg&                controls,
                          const std::vector<Scalar>& connDP)
{
    assert (connDP.size() == this->connections_.size());

    this->accumulateLocalContributions(sources, controls, connDP,
                                       [](const auto i) { return i; });
}

template<class Scalar>
template <typename ConnIndexMap>
std::vector<Scalar> PAvgCalculator<Scalar>::
connectionPressureOffsetWell(const std::size_t nconn,
                             const Sources&    sources,
                             const Scalar      gravity,
                             const Scalar      refDepth,
                             ConnIndexMap      connIndex) const
{
    auto dp = std::vector<Scalar>(nconn);

    auto density = [&sources, this](const auto connIx)
    {
        using Item = typename PAvgDynamicSourceData<Scalar>::template SourceDataSpan<const Scalar>::Item;

        return sources.wellConns()[this->inputConn_[connIx]][Item::MixtureDensity];
    };

    for (auto connID = 0*nconn; connID < nconn; ++connID) {
        const auto connIx = connIndex(connID);
        const auto depth  = this->connections_[connIx].depth;

        dp[connID] = pressureOffset(density(connIx), depth, gravity, refDepth);
    }

    return dp;
}

template<class Scalar>
template <typename ConnIndexMap>
std::vector<Scalar> PAvgCalculator<Scalar>::
connectionPressureOffsetRes(const std::size_t nconn,
                            const Sources&    sources,
                            const Scalar      gravity,
                            const Scalar      refDepth,
                            ConnIndexMap      connIndex) const
{
    auto dp = std::vector<Scalar>(nconn);

    const auto neighList = std::array {
        &PAvgConnection::rectNeighbours,
        &PAvgConnection::diagNeighbours,
    };

    auto density = WeightedRunningAverage<Scalar, Scalar>{};

    auto includeDensity = [this, &sources, &density](const ContrIndexType i)
    {
        using Item = typename PAvgDynamicSourceData<Scalar>::template SourceDataSpan<const Scalar>::Item;

        const auto src = sources.wellBlocks()[this->contributingCells_[i]];

        density.add(src[Item::MixtureDensity], src[Item::PoreVol]);
    };

    for (auto connID = 0*nconn; connID < nconn; ++connID) {
        density.clear();

        const auto& conn = this->connections_[connIndex(connID)];

        includeDensity(conn.cell);

        for (const auto& neighbours : neighList) {
            for (const auto& neighIdx : conn.*neighbours) {
                includeDensity(neighIdx);
            }
        }

        dp[connID] = pressureOffset(value(density), conn.depth, gravity, refDepth);
    }

    return dp;
}

template<class Scalar>
std::vector<Scalar> PAvgCalculator<Scalar>::
connectionPressureOffset(const Sources& sources,
                         const PAvg&    controls,
                         const Scalar   gravity,
                         const Scalar   refDepth) const
{
    const auto nconn = controls.open_connections()
        ? this->openConns_.size()
        : this->connections_.size();

    if ((controls.depth_correction() == PAvg::DepthCorrection::NONE) ||
        ! std::isnormal(gravity))
    {
        // No depth correction.  Either because the run explicitly requests
        // NONE for this or all wells, or because gravity effects are turned
        // off (gravity == 0) globally; possibly due to the NOGRAV keyword.
        // Unexpected case such as denormalised or non-finite values of
        // 'gravity' go here too.

        return std::vector<Scalar>(nconn, 0.0);
    }

    if (controls.depth_correction() == PAvg::DepthCorrection::RES) {
        if (! controls.open_connections()) {
            return this->connectionPressureOffsetRes(nconn, sources, gravity, refDepth,
                                                     [](const auto i) { return i; });
        }

        return this->connectionPressureOffsetRes(nconn, sources, gravity, refDepth,
                                                 [this](const auto i)
                                                 {
                                                     return this->openConns_[i];
                                                 });
    }

    if (controls.depth_correction() != PAvg::DepthCorrection::WELL) {
        throw std::invalid_argument {
            fmt::format("Unsupported WPAVE depth correction flag '{}'",
                        static_cast<int>(controls.depth_correction()))
        };
    }

    if (! controls.open_connections()) {
        return this->connectionPressureOffsetWell(nconn, sources, gravity, refDepth,
                                                  [](const auto i) { return i; });
    }

    return this->connectionPressureOffsetWell(nconn, sources, gravity, refDepth,
                                              [this](const auto i)
                                              {
                                                  return this->openConns_[i];
                                              });
}

// ---------------------------------------------------------------------------

template <typename Scalar>
PAvgCalculatorResult<Scalar>
linearCombination(const Scalar alpha, PAvgCalculatorResult<Scalar>        x,
                  const Scalar beta , const PAvgCalculatorResult<Scalar>& y)
{
    std::transform(x.wbp_.begin(), x.wbp_.end(),
                   y.wbp_.begin(),
                   x.wbp_.begin(),
                   [alpha, beta](const Scalar xi, const Scalar yi)
                   {
                       return alpha*xi + beta*yi;
                   });

    return x;
}

// ===========================================================================
// Explicit template specialisations.  No other code below this separator.
// ===========================================================================

#define INSTANTIATE_TYPE(T)                                     \
    template class PAvgCalculatorResult<T>;                     \
    template class PAvgCalculator<T>;                           \
    template PAvgCalculatorResult<T>                            \
    linearCombination(const T, PAvgCalculatorResult<T>,         \
                      const T, const PAvgCalculatorResult<T>&)

INSTANTIATE_TYPE(double);
INSTANTIATE_TYPE(float);

#undef INSTANTIATE_TYPE

} // namespace Opm
