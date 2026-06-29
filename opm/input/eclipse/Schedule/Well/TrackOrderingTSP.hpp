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

#ifndef OPM_TRACK_ORDERING_TSP_HPP
#define OPM_TRACK_ORDERING_TSP_HPP

#include <algorithm>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <span>
#include <type_traits>
#include <vector>

namespace Opm {
    class Connection;
} // namespace Opm

namespace Opm {

/// Multi-start nearest-neighbour + 2-opt TSP heuristic for TRACK ordering
/// of well connections.
class TrackOrderingTSP
{
public:
    /// Cost weights controlling the TRACK edge-cost function.
    struct TrackCostWeights
    {
        /// Weight for horizontal logical-grid distance in the I/J plane.
        double w_ij {1.0};

        /// Weight for logical K-index step distance.
        double w_k {0.5};

        /// Weight for geometric depth difference.
        double w_z {0.05};

        /// Weight for direction-misalignment penalty between successive
        /// connections and their step vector.
        double w_dir {0.25};

        /// Scale factor that maps depth differences into the I/J distance
        /// metric used by the direction penalty term.
        ///
        /// Depth differences (dZ in meters) and horizontal grid distances
        /// (hypot(dI, dJ) in grid cells) are measured in different units and
        /// typically different magnitudes. When computing the direction penalty,
        /// we need to determine which component (horizontal vs. vertical) is
        /// dominant in the step vector to decide whether a connection follows
        /// the well trajectory. This requires a dimensionless comparison.
        ///
        /// The z_to_ij_scale factor normalizes depth into equivalent I/J units:
        /// equivalent_ij_distance = |dZ| * z_to_ij_scale. For example, with
        /// z_to_ij_scale = 0.05, a depth increase of 20 meters becomes 1 unit
        /// of equivalent horizontal distance. This ensures that the direction
        /// penalty treats depth and horizontal components on equal footing,
        /// allowing the heuristic to balance vertical and horizontal progression
        /// appropriately.
        double z_to_ij_scale {0.05};

        /// Check if the weights are valid for use in the edge-cost function.
        ///
        /// All weights must be non-negative and at least one weight must be
        /// positive to ensure that the edge-cost function is well-defined
        /// and can effectively guide the TSP heuristic.
        ///
        /// \return Whether or not the weights are valid.
        [[nodiscard]] constexpr bool isValid() const noexcept
        {
            auto&& [ij, k, z, dir, z_scale] = *this;

            []<typename... Ts>(Ts&&...) {
                static_assert((std::same_as<double, std::decay_t<Ts>> && ...),
                              "TrackCostWeights must only contain double members.");
            }(ij, k, z, dir, z_scale);

            const auto all_nonneg = std::ranges::
                all_of(std::initializer_list {ij, k, z, dir, z_scale},
                       [](const auto w) { return w >= 0.0; });

            // Note: z_scale (z_to_ij_scale) is intentionally omitted from
            // the check that at least one weight is positive.  It is not a
            // direct cost weight but rather a scale factor.  If only the
            // z_scale is positive, the edge cost function would be
            // identically zero and thus not well-defined.  The z_scale must
            // still be non-negative, but that requirement is enforced by
            // the all_nonneg check.
            return all_nonneg
                && std::ranges::any_of(std::initializer_list {ij, k, z, dir},
                                       [](const auto w) { return w > 0.0; });
        }
    };

    /// Construct with the well-head logical cartesian indices.
    TrackOrderingTSP(int headI, int headJ);

    /// Set number of start points from which to explore path creation.
    ///
    /// \param[in] numStarts Number of start points.  Note that regardless
    /// of \p numStarts, the algorithm will always use at least one start
    /// point--the connection that's closest to the well head defined in the
    /// constructor.  Furthermore, for any particular ordering problem, the
    /// algorithm will never use more start points than the number of
    /// connections passed to order().
    ///
    /// \return \code *this. \endcode
    TrackOrderingTSP& numberOfStartPoints(std::size_t numStarts);

    /// Return the permutation that puts \p connections in TRACK order,
    /// using default-constructed TrackCostWeights.
    ///
    /// \param[in] connections The connections to order.
    ///
    /// \return Index permutation \p p such that \c connections[p[0]] is
    /// the heel and subsequent entries follow the optimised path.
    std::vector<std::size_t>
    order(std::span<const Connection> connections) const;

    /// Return the permutation that puts \p connections in TRACK order,
    /// using the supplied cost weights.
    ///
    /// \param[in] connections  The connections to order.
    /// \param[in] weights Cost weights for the edge-cost function.
    ///
    /// \return Index permutation \p p such that \c connections[p[0]] is
    /// the heel and subsequent entries follow the optimised path.
    std::vector<std::size_t>
    order(std::span<const Connection> connections,
          const TrackCostWeights&     weights) const;

private:
    /// A start point candidate.
    ///
    /// Distilled distance measure relative to the well head for the current
    /// sequence of Connection objects passed to member function order().
    /// Actual start point identified by the sequence index which doubles as
    /// a tie-breaker for sorting purposes.
    struct CandidateStart
    {
        /// Squared Euclidean IJ distance.
        ///
        /// Primary sort key.
        std::int_least64_t ijdist2 {};

        /// Connection depth difference relative to reference depth.
        ///
        /// Satisfies std::isfinite().
        ///
        /// Secondary sort key.
        double zdiff {};

        /// Connection sequence index.
        ///
        /// Tie-break for sorting.
        std::size_t index {};

        /// Object ordering operator.
        ///
        /// Implies a lexicographical ordering (same as std::tuple<>'s
        /// operator<()) over (ijdist2, zdiff, index).
        auto operator<=>(const CandidateStart&) const = default;
    };

    /// I-axis location of well head.
    int headI_{};

    /// J-axis location of well head.
    int headJ_{};

    /// Number of path search start points.
    ///
    /// Always at least one since we always include the connection that's
    /// closest to the well head as a start point.
    std::size_t numStartPoints_{1};

    /// Determine start point candidates closest to the well head.
    ///
    /// \param[in] connections The connections to search.
    ///
    /// \return Start point candidates.  At most \p numStartPoints_
    /// candidates, ordered by increasing distance to the well head.
    std::vector<CandidateStart>
    determineStartPoints(std::span<const Connection> connections) const;
};

} // namespace Opm

#endif // OPM_TRACK_ORDERING_TSP_HPP
