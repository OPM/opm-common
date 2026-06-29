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

#include <opm/input/eclipse/Schedule/Well/TrackOrderingTSP.hpp>

#include <opm/input/eclipse/Schedule/Well/Connection.hpp>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <span>
#include <utility>
#include <vector>

namespace {

    /// Compute the penalty for taking a step that is not aligned with the
    /// dominant geometric direction of the completion.
    ///
    /// \param[in] dir Completion direction being evaluated.
    /// \param[in] abs_di Absolute logical I-step length.
    /// \param[in] abs_dj Absolute logical J-step length.
    /// \param[in] abs_dz_ij Depth contribution scaled into the I/J metric.
    ///
    /// \return A value in $[0, 1]$ where zero means fully aligned.
    double directionStepPenalty(const Opm::Connection::Direction dir,
                                const double                     abs_di,
                                const double                     abs_dj,
                                const double                     abs_dz_ij)
    {
        constexpr auto eps = 1.0e-12;

        const auto total = abs_di + abs_dj + abs_dz_ij;
        if (total < eps) {
            return 0.0;
        }

        auto dominant = 0.0;
        switch (dir) {
        case Opm::Connection::Direction::X:
            dominant = abs_di;
            break;

        case Opm::Connection::Direction::Y:
            dominant = abs_dj;
            break;

        case Opm::Connection::Direction::Z:
            dominant = abs_dz_ij;
            break;
        }

        return 1.0 - dominant / total;
    }

    /// Compute the weighted edge cost used by the TRACK-ordering heuristic.
    ///
    /// \param[in] a First connection in the edge.
    /// \param[in] b Second connection in the edge.
    /// \param[in] w Cost weights controlling the distance terms.
    ///
    /// \return Weighted cost of moving from \p a to \p b.
    std::int_least64_t
    trackEdgeCost(const Opm::Connection&                         a,
                  const Opm::Connection&                         b,
                  const Opm::TrackOrderingTSP::TrackCostWeights& w)
    {
        constexpr auto max_cost = std::numeric_limits<std::int_least64_t>::max();

        const auto di = std::abs(static_cast<double>(b.getI() - a.getI()));
        const auto dj = std::abs(static_cast<double>(b.getJ() - a.getJ()));
        const auto dk = std::abs(static_cast<double>(b.getK() - a.getK()));
        const auto dz = std::abs(b.depth() - a.depth());

        const auto abs_dz_ij = std::abs(w.z_to_ij_scale) * dz;
        const auto ij = std::hypot(di, dj);

        const auto dir_penalty = 0.5 *
            (directionStepPenalty(a.dir(), di, dj, abs_dz_ij) +
             directionStepPenalty(b.dir(), di, dj, abs_dz_ij));

        const auto raw_cost = w.w_ij  * ij +
                              w.w_z   * dz +
                              w.w_k   * dk +
                              w.w_dir * dir_penalty;

        if (! std::isfinite(raw_cost)) {
            // Handle potential numerical issues by treating non-finite costs as
            // very high.
            return max_cost;
        }

        // The multiplicative factor 1.0e9 effectively imposes a 1.0e-9
        // improvement threshold.
        const auto scaled_cost = 1.0e9 * raw_cost;

        if (scaled_cost > static_cast<double>(max_cost) ||
            scaled_cost < static_cast<double>(std::numeric_limits<std::int_least64_t>::min()))
        {
            // Handle potential overflow by treating excessively high costs as
            // the maximum representable cost.
            return max_cost;
        }

        return static_cast<std::int_least64_t>(scaled_cost);
    }

    /// Build an initial greedy path by repeatedly selecting the cheapest
    /// unvisited successor.
    ///
    /// \param[in] connections Connections to order.
    ///
    /// \param[in] heel Index of well heel connection.  This connection
    /// is always the first path element.
    ///
    /// \param[in] first Index of the first connection other than \p heel
    /// in the path.  Ignored if same as \p heel.
    ///
    /// \param[in] w Cost weights controlling candidate selection.
    ///
    /// \param[in,out] path Greedy path permutation starting at \p heel,
    /// and with \p first as the second element if different from
    /// \p heel.  Path is cleared and rebuilt from scratch.
    void
    initialTrackPath(std::span<const Opm::Connection>               connections,
                     const std::size_t                              heel,
                     const std::size_t                              first,
                     const Opm::TrackOrderingTSP::TrackCostWeights& w,
                     std::vector<std::size_t>&                      path)
    {
        const auto n = static_cast<std::size_t>(connections.size());

        path.clear();
        path.reserve(n);

        auto visited = std::vector<bool>(n, false);

        path.push_back(heel);
        visited[heel] = true;

        if (first != heel) {
            path.push_back(first);
            visited[first] = true;
        }

        while (path.size() < n) {
            const auto cur = path.back();

            auto best = n; // Invalid index used as sentinel.
            auto best_cost = std::numeric_limits<std::int_least64_t>::max();

            for (auto cand = std::size_t{0}; cand < n; ++cand) {
                if (visited[cand]) {
                    continue;
                }

                const auto cost = trackEdgeCost(connections[cur], connections[cand], w);
                if ((best == n) || (cost < best_cost)) {
                    best_cost = cost;
                    best = cand;
                }
            }

            assert (best != n); // Should always find an unvisited candidate.

            visited[best] = true;
            path.push_back(best);
        }
    }

    /// Improve a path with a 2-opt pass while keeping the heel fixed.
    ///
    /// \param[in] connections Connections referenced by the path.
    /// \param[in] w Cost weights controlling the edge costs.
    /// \param[in,out] path Path permutation to improve in place.
    void
    improveTrackPathOpen2Opt(std::span<const Opm::Connection>               connections,
                             const Opm::TrackOrderingTSP::TrackCostWeights& w,
                             std::vector<std::size_t>&                      path)
    {
        if (path.size() < 4) {
            return;
        }

        const auto n = static_cast<std::size_t>(path.size());

        // Fix path[0] as the heel candidate; optimize only the suffix.
        auto improved = true;
        while (improved) {
            improved = false;

            for (auto i = std::size_t{1}; i + 1 < n; ++i) {
                for (auto k = i + 1; k < n; ++k) {
                    const auto a = path[i - 1];
                    const auto b = path[i];
                    const auto c = path[k];

                    const auto max_cost = std::numeric_limits<std::int_least64_t>::max();

                    const auto ab = trackEdgeCost(connections[a], connections[b], w);
                    const auto ac = trackEdgeCost(connections[a], connections[c], w);

                    auto old_cost = ab;
                    auto new_cost = ac;

                    // Interior reversal [i, k] changes two boundary edges:
                    // (a, b), (c, d) -> (a, c), (b, d), where d = path[k+1].
                    // For suffix reversal [i, n-1], there is no trailing edge,
                    // so only (a, b) -> (a, c) changes.
                    if (k + 1 < n) {
                        const auto d = path[k + 1];
                        const auto cd = trackEdgeCost(connections[c], connections[d], w);
                        const auto bd = trackEdgeCost(connections[b], connections[d], w);

                        old_cost = (ab == max_cost || cd == max_cost || ab > (max_cost - cd))
                            ? max_cost
                            : (ab + cd);

                        new_cost = (ac == max_cost || bd == max_cost || ac > (max_cost - bd))
                            ? max_cost
                            : (ac + bd);
                    }

                    if (new_cost < old_cost) {
                        std::reverse(path.begin() + static_cast<std::ptrdiff_t>(i),
                                     path.begin() + static_cast<std::ptrdiff_t>(k + 1));

                        improved = true;
                    }
                }
            }
        }
    }

    /// Compute the total cost of a TRACK-order path.
    ///
    /// \param[in] connections Connections referenced by the path.
    /// \param[in] path Path permutation to evaluate.
    /// \param[in] w Cost weights controlling the edge costs.
    ///
    /// \return Sum of all edge costs along \p path.
    std::int_least64_t
    trackPathCost(std::span<const Opm::Connection>               connections,
                  const std::vector<std::size_t>&                path,
                  const Opm::TrackOrderingTSP::TrackCostWeights& w)
    {
        auto total = std::int_least64_t{0};
        if (path.size() < 2) {
            return total;
        }

        constexpr auto max_cost = std::numeric_limits<std::int_least64_t>::max();

        for (auto i = std::size_t{1}; i < path.size(); ++i) {
            const auto edge_cost = trackEdgeCost(connections[path[i - 1]], connections[path[i]], w);
            if ((edge_cost == max_cost) || (total > (max_cost - edge_cost))) {
                return max_cost;
            }

            total += edge_cost;
        }

        return total;
    }

} // Anonymous namespace

namespace Opm {

    TrackOrderingTSP::TrackOrderingTSP(const int headI, const int headJ)
        : headI_ { headI }
        , headJ_ { headJ }
    {}

    TrackOrderingTSP&
    TrackOrderingTSP::numberOfStartPoints(const std::size_t numStarts)
    {
        // We always use the connection that's closest to the well head as a
        // start point, so the number of start points is always at least
        // one.
        this->numStartPoints_ = std::max(std::size_t{1}, numStarts);

        return *this;
    }

    std::vector<std::size_t>
    TrackOrderingTSP::order(std::span<const Connection> connections) const
    {
        return this->order(connections, TrackCostWeights{});
    }

    std::vector<std::size_t>
    TrackOrderingTSP::order(std::span<const Connection> connections,
                            const TrackCostWeights&     weights) const
    {
        auto best_path = std::vector<std::size_t>{};
        best_path.reserve(connections.size());

        if (connections.empty()) {
            return best_path;
        }

        if (connections.size() == 1) {
            best_path.push_back(0);
            return best_path;
        }

        if (! weights.isValid()) {
            throw std::invalid_argument {
                "Invalid TrackCostWeights: all weights must be "
                "non-negative and at least one weight must be "
                "strictly positive."
            };
        }

        auto curr_path = best_path;
        auto best_cost = std::numeric_limits<std::int_least64_t>::max();

        const auto start_candidates = this->determineStartPoints(connections);
        const auto heel = start_candidates.front().index;

        assert (start_candidates.size() <= this->numStartPoints_);

        auto start_points = start_candidates
            | std::views::transform([](const auto& candidate) { return candidate.index; });

        for (const auto& start_point : start_points) {
            initialTrackPath(connections, heel, start_point, weights, curr_path);
            improveTrackPathOpen2Opt(connections, weights, curr_path);

            const auto cost = trackPathCost(connections, curr_path, weights);
            if (best_path.empty() || (cost < best_cost)) {
                best_cost = cost;
                best_path.swap(curr_path);
            }
        }

        return best_path;
    }

    std::vector<TrackOrderingTSP::CandidateStart>
    TrackOrderingTSP::determineStartPoints(std::span<const Connection> connections) const
    {
        auto start_candidates = std::vector<CandidateStart>{};

        assert (this->numStartPoints_ >= std::size_t{1});

        const auto num_conns = static_cast<std::size_t>(connections.size());
        start_candidates.reserve(num_conns);

        auto candidate_sequence = std::views::iota(std::size_t{0}, num_conns)
            | std::views::transform([surface_z = 0.0, this, connections](const auto idx)
            {
                const auto& c = connections[idx];
                const auto di = static_cast<std::int_least64_t>(c.getI() - this->headI_);
                const auto dj = static_cast<std::int_least64_t>(c.getJ() - this->headJ_);

                return CandidateStart {
                    .ijdist2 = di * di + dj * dj,
                    .zdiff   = std::abs(c.depth() - surface_z),
                    .index   = idx
                };
            });

        for (const auto& candidate : candidate_sequence) {
            start_candidates.push_back(candidate);
        }

        // The path search start points are those 'numStartPoints_'
        // connections that are closest to the well head.
        const auto num_starts = std::min(num_conns, this->numStartPoints_);

        if (auto mid = start_candidates.begin() + num_starts;
            mid != start_candidates.end())
        {
            std::ranges::nth_element(start_candidates, mid);
            start_candidates.erase(mid, start_candidates.end());
        }

        // Sort selected candidates deterministically (ijdist2, zdiff, index).
        std::ranges::sort(start_candidates);

        return start_candidates;
    }

} // namespace Opm
