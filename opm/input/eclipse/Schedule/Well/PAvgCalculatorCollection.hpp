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

#ifndef PAVE_CALC_COLLECTIONHPP
#define PAVE_CALC_COLLECTIONHPP

#include <cstddef>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

namespace Opm {
    template<class Scalar> class PAvgCalculator;
} // namespace Opm

namespace Opm {

/// Collection of WBPn calculation objects, one for each well.
template<class Scalar>
class PAvgCalculatorCollection
{
public:
    /// Wrapper for a WBPn calclation object.
    ///
    /// We use a pointer here to enable polymorphic behaviour at runtime,
    /// e.g., parallel calculation in an MPI-enabled simulation run.
    using CalculatorPtr = std::unique_ptr<PAvgCalculator<Scalar>>;

    /// Predicate for whether or not a particular source location is active.
    ///
    /// One typical use case is determining whether or not a particular cell
    /// defined by its Cartesian (I,J,K) index triple is actually amongst
    /// the model's active cells.
    ///
    /// This predicate will be called with a vector of source location
    /// indices and must return a vector of the same size that holds the
    /// value 'true' if the corresponding source location is active and
    /// 'false' otherwise.
    using ActivePredicate = std::function<
        std::vector<bool>(const std::vector<std::size_t>&)>;

    /// Default constructor.
    PAvgCalculatorCollection() = default;

    /// Destructor.
    ~PAvgCalculatorCollection() = default;

    /// Copy constructor.
    PAvgCalculatorCollection(const PAvgCalculatorCollection&) = delete;

    /// Move constructor.
    PAvgCalculatorCollection(PAvgCalculatorCollection&&) = default;

    /// Assignment operator.
    PAvgCalculatorCollection& operator=(const PAvgCalculatorCollection&) = delete;

    /// Move-assignment operator.
    PAvgCalculatorCollection& operator=(PAvgCalculatorCollection&&) = default;

    /// Assign/register a WBPn calculation object for a single well.
    ///
    /// \param[in] wellID Unique numeric ID representing a single well.
    ///   Typically the \code seqIndex() \endcode of a \c Well object.
    ///
    /// \param[in] calculator Calculation object specific to a single well.
    ///
    /// \return Index by which to refer to the calculation object later,
    ///   e.g., when calculating the WBPn values for a single well.  No
    ///   relation to \p wellID.
    std::size_t setCalculator(const std::size_t wellID, CalculatorPtr calculator);

    /// Discard inactive source locations from all WBPn calculation objects.
    ///
    /// At the end of this call, no source location deemed to be "inactive"
    /// will be amongst those later used for collection of source term
    /// contributions.
    ///
    /// \param[in] isActive Predicate for whether or not a source location
    ///   is "active".  The caller determines what "active" means.  Must
    ///   abide by the protocol outlined above.
    void pruneInactiveWBPCells(ActivePredicate isActive);

    /// Access mutable WBPn calculation object.
    ///
    /// \param[in] i WBPn calculation object index.  Must be one returned
    ///   from a previous call to \c setCalculator.
    ///
    /// \return Mutable WBPn calculation object.
    PAvgCalculator<Scalar>& operator[](const std::size_t i);

    /// Access immutable WBPn calculation object.
    ///
    /// \param[in] i WBPn calculation object index.  Must be one returned
    ///   from a previous call to \c setCalculator.
    ///
    /// \return Immutable WBPn calculation object.
    const PAvgCalculator<Scalar>& operator[](const std::size_t i) const;

    /// Whether or not this collection has any WBPn calculation objects.
    bool empty() const;

    /// Number of WBPn calculation objects owned by this collection.
    std::size_t numCalculators() const;

    /// Union of all distinct/unique cells/source locations contributing to
    /// this complete collection of WBPn calculation objects.
    ///
    /// Mainly intended to configure \c PAvgDynamicSourceData objects.
    std::vector<std::size_t> allWBPCells() const;

private:
    /// Representation of calculator indices.
    using CalcIndex = typename std::vector<CalculatorPtr>::size_type;

    /// Translation table for mapping Well IDs to calculator indices.
    std::unordered_map<std::size_t, CalcIndex> index_{};

    /// Collection of WBPn calculation objects.
    std::vector<CalculatorPtr> calculators_{};
};

} // namespace Opm

#endif // PAVE_CALC_COLLECTIONHPP
