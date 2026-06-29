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

#define BOOST_TEST_MODULE TrackOrderingTSPTests
#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Schedule/Well/TrackOrderingTSP.hpp>

#include <opm/input/eclipse/Schedule/Well/Connection.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numeric>
#include <ranges>
#include <span>
#include <stdexcept>
#include <vector>

namespace {

    /// Helper: build a Connection with the minimal fields required for
    /// TrackOrderingTSP (I, J, K, depth, direction).
    Opm::Connection makeConn(const int i, const int j, const int k,
                             const double depth,
                             const Opm::Connection::Direction dir
                                 = Opm::Connection::Direction::Z)
    {
        return Opm::Connection {
            i, j, k,
            /*global_index=*/static_cast<std::size_t>(i * 1000 + j * 100 + k),
            /*complnum=*/1,
            Opm::Connection::State::OPEN,
            dir,
            Opm::Connection::CTFKind::DeckValue,
            /*satTableId=*/0,
            depth,
            Opm::Connection::CTFProperties{},
            /*sort_value=*/0,
            /*defaultSatTabId=*/true
        };
    }

    // -----------------------------------------------------------------------
    // Proxy path cost for an ordered sequence of connections.  Used only
    // for relative comparisons in tests.
    // -----------------------------------------------------------------------
    double pathCostFromOrder(std::span<const Opm::Connection> conns,
                             const std::vector<std::size_t>&  perm)
    {
        if (perm.size() < 2) {
            return 0.0;
        }

        // Replicate enough of the cost function for validation purposes.
        //
        // Using Euclidean XY distance avoids introducing a proxy metric
        // that can reorder paths relative to TrackOrderingTSP's objective.
        auto cost = 0.0;

        for (const std::size_t i : std::views::iota(std::size_t{0}, perm.size() - 1)) {
            const auto& conn_left  = conns[perm[i + 0]];
            const auto& conn_right = conns[perm[i + 1]];

            const auto dx = static_cast<double>(conn_right.getI() - conn_left.getI());
            const auto dy = static_cast<double>(conn_right.getJ() - conn_left.getJ());
            const auto dz = std::abs(conn_right.depth() - conn_left.depth());

            cost += std::hypot(dx, dy) + 0.05 * dz;
        }

        return cost;
    }

} // anonymous namespace

// ---------------------------------------------------------------------------
// Test 1: Empty connection list – no crash; returns empty permutation.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(empty_connections)
{
    const auto tsp = Opm::TrackOrderingTSP { 5, 5 };

    const auto connections = std::vector<Opm::Connection>{};
    const auto perm = tsp.order(connections);

    BOOST_CHECK(perm.empty());
}

// ---------------------------------------------------------------------------
// Test 2: Single connection – returns {0}.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(single_connection)
{
    const auto tsp = Opm::TrackOrderingTSP { 5, 5 };

    const auto connections = std::vector { makeConn(5, 5, 1, 100.0) };
    const auto perm = tsp.order(connections);

    BOOST_REQUIRE_EQUAL(perm.size(), 1u);
    BOOST_CHECK_EQUAL(perm[0], 0u);
}

// ---------------------------------------------------------------------------
// Test 3: Two connections – heel (closest to head) comes first.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(two_connections_heel_first)
{
    // Head at (5,5); connection 0 is far, connection 1 is at the head.
    const auto tsp = Opm::TrackOrderingTSP { 5, 5 };

    const auto connections = std::vector {
        makeConn(10, 10, 1, 200.0),  // index 0, far from head
        makeConn( 5,  5, 1, 100.0),  // index 1, at head
    };
    const auto perm = tsp.order(connections);

    BOOST_REQUIRE_EQUAL(perm.size(), 2u);

    // index 1 (the heel) must come first
    BOOST_CHECK_EQUAL(perm[0], 1u);
    BOOST_CHECK_EQUAL(perm[1], 0u);
}

// ---------------------------------------------------------------------------
// Test 4: Vertical well (same I,J; varying K/depth).
// Expected: output proceeds monotonically from shallowest to deepest.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(vertical_well_top_to_bottom)
{
    // Head at (5,5); connections stored in reversed depth order.
    const auto tsp = Opm::TrackOrderingTSP { 5, 5 };

    const auto connections = std::vector {
        makeConn(5, 5, 4, 400.0),   // index 0, deepest
        makeConn(5, 5, 2, 200.0),   // index 1
        makeConn(5, 5, 1, 100.0),   // index 2, shallowest / heel
        makeConn(5, 5, 3, 300.0),   // index 3
    };
    const auto perm = tsp.order(connections);

    BOOST_REQUIRE_EQUAL(perm.size(), 4u);

    // Verify that depths appear in non-decreasing order in the output.
    for (auto i = std::size_t{1}; i < perm.size(); ++i) {
        BOOST_CHECK_GE(connections[perm[i]].depth(), connections[perm[i-1]].depth());
    }
}

// ---------------------------------------------------------------------------
// Test 5: Horizontal well (same J, K; increasing I).
// Expected: heel-end (closest I to head) is first in the result.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(horizontal_well_heel_first)
{
    // Head at (2,5); connections along I at J=5.
    const auto tsp = Opm::TrackOrderingTSP { 2, 5 };

    const auto connections = std::vector {
        makeConn(6, 5, 1, 150.0, Opm::Connection::Direction::X),   // index 0, far
        makeConn(4, 5, 1, 150.0, Opm::Connection::Direction::X),   // index 1
        makeConn(3, 5, 1, 150.0, Opm::Connection::Direction::X),   // index 2, closest
        makeConn(5, 5, 1, 150.0, Opm::Connection::Direction::X),   // index 3
    };
    const auto perm = tsp.order(connections);

    BOOST_REQUIRE_EQUAL(perm.size(), 4u);

    // First element must be the one with I=3 (closest to head I=2).
    BOOST_CHECK_EQUAL(connections[perm[0]].getI(), 3);
}

// ---------------------------------------------------------------------------
// Test 6: L-shaped path – nearest-neighbour picks a sensible route without
// returning to the start of each arm.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(l_shaped_path_no_backtrack)
{
    // Head at (0,0); arm1 goes along I, arm2 turns along J.
    //   (0,0) -> (1,0) -> (2,0) -> (2,1) -> (2,2)
    const auto tsp = Opm::TrackOrderingTSP { 0, 0 };

    const auto connections = std::vector {
        makeConn(2, 2, 1, 100.0),   // index 0, far end of arm 2
        makeConn(1, 0, 1, 100.0),   // index 1
        makeConn(0, 0, 1, 100.0),   // index 2, heel
        makeConn(2, 0, 1, 100.0),   // index 3, corner
        makeConn(2, 1, 1, 100.0),   // index 4
    };
    const auto perm = tsp.order(connections);

    BOOST_REQUIRE_EQUAL(perm.size(), 5u);

    // Heel must be first.
    BOOST_CHECK_EQUAL(connections[perm[0]].getI(), 0);
    BOOST_CHECK_EQUAL(connections[perm[0]].getJ(), 0);

    // The identity permutation {2,1,3,4,0} follows the natural L path; verify
    // that the cost of the returned permutation is no worse than that.
    const auto natural_perm = std::vector<std::size_t>{ 2, 1, 3, 4, 0 };
    BOOST_CHECK_LE(pathCostFromOrder(connections, perm),
                   pathCostFromOrder(connections, natural_perm) + 1.0e-9);
}

// ---------------------------------------------------------------------------
// Test 7: Direction penalty – with w_dir > 0, an X-directed connection
// incurs a higher cost on a step in the Y direction than on a step in X.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(direction_penalty_favours_aligned_step)
{
    using Dir = Opm::Connection::Direction;
    using W   = Opm::TrackOrderingTSP::TrackCostWeights;

    // Two candidate successors for an X-directed connection at (0,0):
    //   option A: step in X direction (aligned)
    //   option B: step in Y direction (misaligned)
    // Both have equal I/J distance = 1.
    const auto tsp = Opm::TrackOrderingTSP { 0, 0 };

    const auto connections = std::vector {
        makeConn(0, 0, 1, 100.0, Dir::X),  // index 0, heel / start
        makeConn(1, 0, 1, 100.0, Dir::X),  // index 1, aligned step (delta I = 1)
        makeConn(0, 1, 1, 100.0, Dir::X),  // index 2, misaligned step (delta J = 1)
    };

    // Use high w_dir to amplify the direction penalty.
    W weights{};
    weights.w_dir = 10.0;

    const auto perm = tsp.order(connections, weights);

    BOOST_REQUIRE_EQUAL(perm.size(), 3u);

    // Heel first.
    BOOST_CHECK_EQUAL(perm[0], 0u);

    // Aligned step (index 1) must precede misaligned step (index 2).
    BOOST_CHECK_EQUAL(perm[1], 1u);
    BOOST_CHECK_EQUAL(perm[2], 2u);
}

// ---------------------------------------------------------------------------
// Test 8: 2-opt improvement – a deliberately crossed path must be uncrossed.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(two_opt_decrosses_path)
{
    // Non-collinear layout with deterministic single-start NN path
    //   0 -> 1 -> 2 -> 5 -> 4 -> 3
    // where edge (2,5) crosses edge (4,3).  Open-path 2-opt should
    // remove the crossing and yield a strictly lower path cost.
    //
    // Coordinates:
    //   0: (0,0)  heel (closest to head)
    //   1: (0,1)
    //   2: (0,2)
    //   3: (0,4)
    //   4: (1,0)
    //   5: (1,1)
    const auto tsp = Opm::TrackOrderingTSP { 0, 0 };

    const auto connections = std::array {
        makeConn(0, 0, 1, 100.0),   // index 0, heel
        makeConn(0, 1, 1, 100.0),   // index 1
        makeConn(0, 2, 1, 100.0),   // index 2
        makeConn(0, 4, 1, 100.0),   // index 3
        makeConn(1, 0, 1, 100.0),   // index 4
        makeConn(1, 1, 1, 100.0),   // index 5
    };

    const auto perm = tsp.order(connections);
    BOOST_REQUIRE_EQUAL(perm.size(), connections.size());

    // Crossed NN order for this point set.
    const auto crossed_perm = std::vector<std::size_t>{ 0, 1, 2, 5, 4, 3 };

    // 2-opt should strictly improve over the crossed path.
    BOOST_CHECK_LT(pathCostFromOrder(connections, perm),
                   pathCostFromOrder(connections, crossed_perm));
}

// ---------------------------------------------------------------------------
// Test 9: Reproducibility – same input yields same output across two calls.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(reproducibility)
{
    const auto tsp = Opm::TrackOrderingTSP { 3, 3 };

    const auto connections = std::vector {
        makeConn(5, 7, 2, 300.0),
        makeConn(3, 3, 1, 100.0),
        makeConn(6, 4, 3, 400.0),
        makeConn(4, 5, 2, 200.0),
        makeConn(3, 4, 1, 150.0),
    };

    const auto perm1 = tsp.order(connections);
    const auto perm2 = tsp.order(connections);

    BOOST_CHECK_EQUAL_COLLECTIONS(perm1.begin(), perm1.end(),
                                  perm2.begin(), perm2.end());
}

// ---------------------------------------------------------------------------
// Test 10: Custom weights (w_dir=0) – direction no longer influences ordering.
// Use a setup where w_dir > 0 changes the first choice relative to pure
// geometry, then verify that w_dir=0 follows the direction-agnostic path.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(custom_weights_zero_dir)
{
    using Dir = Opm::Connection::Direction;

    const auto tsp = Opm::TrackOrderingTSP { 0, 0 };

    // Candidate 1: farther but aligned with X-direction stepping.
    // Candidate 2: nearer but misaligned with X-direction stepping.
    //
    // With w_dir=0 the nearer point should be selected first.
    // With sufficiently large w_dir the aligned point should be selected first.
    const auto connections = std::vector {
        makeConn(0, 0, 1, 100.0, Dir::X),  // index 0, heel
        makeConn(2, 0, 1, 100.0, Dir::X),  // index 1, aligned but farther
        makeConn(0, 1, 1, 100.0, Dir::X),  // index 2, misaligned but nearer
    };

    // Same geometry as above, but different per-connection directions.
    // If w_dir=0, this must produce the same ordering as 'connections'.
    const auto connections_flipped_dirs = std::vector {
        makeConn(0, 0, 1, 100.0, Dir::Z),
        makeConn(2, 0, 1, 100.0, Dir::Y),
        makeConn(0, 1, 1, 100.0, Dir::Z),
    };

    Opm::TrackOrderingTSP::TrackCostWeights w_with_dir{};
    w_with_dir.w_dir = 2.0;

    Opm::TrackOrderingTSP::TrackCostWeights w_no_dir{};
    w_no_dir.w_dir = 0.0;

    const auto perm_with = tsp.order(connections, w_with_dir);
    const auto perm_no = tsp.order(connections, w_no_dir);
    const auto perm_no_flipped = tsp.order(connections_flipped_dirs, w_no_dir);

    const auto expected_no_dir = std::vector<std::size_t>{ 0, 2, 1 };

    // Direction-agnostic ordering: choose the nearer candidate first.
    BOOST_REQUIRE_EQUAL(perm_no.size(), 3u);
    BOOST_CHECK_EQUAL_COLLECTIONS(perm_no.begin(), perm_no.end(),
                                  expected_no_dir.begin(), expected_no_dir.end());

    // With w_dir=0, changing per-connection directions must not matter.
    BOOST_CHECK_EQUAL_COLLECTIONS(perm_no.begin(), perm_no.end(),
                                  perm_no_flipped.begin(), perm_no_flipped.end());

    // With w_dir>0, ordering should change for this setup.
    BOOST_CHECK(! std::ranges::equal(perm_with, perm_no));
}

// ---------------------------------------------------------------------------
// Test 11: Custom weights invalid (negative w_dir) – must throw std::invalid_argument.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(custom_weights_invalid_negative_dir)
{
    const auto tsp = Opm::TrackOrderingTSP { 0, 0 };

    const auto connections = std::vector {
        makeConn(0, 0, 1, 100.0),
        makeConn(1, 0, 1, 100.0),
    };

    Opm::TrackOrderingTSP::TrackCostWeights invalid_weights{};
    invalid_weights.w_dir = -1.0; // Negative weight is invalid.

    BOOST_CHECK_THROW(tsp.order(connections, invalid_weights), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// Test 12: Custom weights invalid (all zero) – must throw std::invalid_argument.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(custom_weights_invalid_all_zero)
{
    const auto tsp = Opm::TrackOrderingTSP { 0, 0 };

    const auto connections = std::vector {
        makeConn(0, 0, 1, 100.0),
        makeConn(1, 0, 1, 100.0),
    };

    Opm::TrackOrderingTSP::TrackCostWeights invalid_weights{};
    invalid_weights.w_ij = 0.0;
    invalid_weights.w_k = 0.0;
    invalid_weights.w_z = 0.0;
    invalid_weights.w_dir = 0.0;
    invalid_weights.z_to_ij_scale = 0.0; // All weights zero is invalid.

    BOOST_CHECK_THROW(tsp.order(connections, invalid_weights), std::invalid_argument);
}

// ---------------------------------------------------------------------------
// Test 13: numberOfStartPoints(0) is silently clamped to 1.
// Setting zero start points must produce the same result as the default
// (one start point).
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(num_starts_zero_clamped_to_one)
{
    const auto connections = std::vector {
        makeConn(0, 0, 1, 100.0),   // heel
        makeConn(6, 0, 1, 100.0),
        makeConn(2, 0, 1, 100.0),
        makeConn(9, 0, 1, 100.0),
        makeConn(4, 0, 1, 100.0),
    };

    const auto perm_default = Opm::TrackOrderingTSP { 0, 0 }
        .order(connections);

    const auto perm_zero = Opm::TrackOrderingTSP { 0, 0 }
        .numberOfStartPoints(0)
        .order(connections);

    BOOST_CHECK_EQUAL_COLLECTIONS(perm_default.begin(), perm_default.end(),
                                  perm_zero.begin(),    perm_zero.end());
}

// ---------------------------------------------------------------------------
// Test 14: numberOfStartPoints(1) is identical to the default.
// An explicit setting of one start point must reproduce the default output
// exactly.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(num_starts_one_same_as_default)
{
    const auto connections = std::vector {
        makeConn(0, 0, 1, 100.0),   // heel
        makeConn(6, 0, 1, 100.0),
        makeConn(2, 0, 1, 100.0),
        makeConn(9, 0, 1, 100.0),
        makeConn(4, 0, 1, 100.0),
    };

    const auto perm_default = Opm::TrackOrderingTSP { 0, 0 }
        .order(connections);

    const auto perm_one = Opm::TrackOrderingTSP { 0, 0 }
        .numberOfStartPoints(1)
        .order(connections);

    BOOST_CHECK_EQUAL_COLLECTIONS(perm_default.begin(), perm_default.end(),
                                  perm_one.begin(),     perm_one.end());
}

// ---------------------------------------------------------------------------
// Test 15: numberOfStartPoints larger than the connection count.
// The algorithm must not crash and must return a valid permutation even
// when the requested number of start points exceeds the number of
// connections.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(num_starts_exceeds_connection_count)
{
    const auto connections = std::vector {
        makeConn(0, 0, 1, 100.0),
        makeConn(3, 0, 1, 100.0),
        makeConn(1, 0, 1, 100.0),
        makeConn(2, 0, 1, 100.0),
    };

    const auto perm = Opm::TrackOrderingTSP { 0, 0 }
        .numberOfStartPoints(1'000)
        .order(connections);

    // Result must be a valid permutation of {0, 1, 2, 3}.
    BOOST_REQUIRE_EQUAL(perm.size(), connections.size());

    auto sorted_perm = perm;
    std::ranges::sort(sorted_perm);
    for (auto i = std::size_t{0}; i < sorted_perm.size(); ++i) {
        BOOST_CHECK_EQUAL(sorted_perm[i], i);
    }
}

// ---------------------------------------------------------------------------
// Test 16: More start points never worsen the result.
// The minimum path cost over N start points is always ≤ the cost from a
// single start point, since the single-start result is a candidate in the
// multi-start search.
//
// Connection layout: collinear along I with J = K = depth constant, so
// the proxy cost (Manhattan I distance) and the actual edge cost are
// proportional and rank paths identically.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(more_starts_never_worsens_result)
{
    // All connections share J=0, K=1, depth=100 so the only cost
    // contributor between pairs is the I-axis distance.
    const auto connections = std::vector {
        makeConn(0, 0, 1, 100.0),   // heel (closest to head at I=0)
        makeConn(7, 0, 1, 100.0),
        makeConn(2, 0, 1, 100.0),
        makeConn(9, 0, 1, 100.0),
        makeConn(4, 0, 1, 100.0),
        makeConn(6, 0, 1, 100.0),
    };

    const auto perm_one = Opm::TrackOrderingTSP { 0, 0 }
        .numberOfStartPoints(1)
        .order(connections);

    const auto perm_all = Opm::TrackOrderingTSP { 0, 0 }
        .numberOfStartPoints(connections.size())
        .order(connections);

    // More starts search a strictly larger space, so cost can only stay
    // equal or improve.
    BOOST_CHECK_LE(pathCostFromOrder(connections, perm_all),
                   pathCostFromOrder(connections, perm_one) + 1.0e-9);
}

// ---------------------------------------------------------------------------
// Test 17: Fluent interface – numberOfStartPoints() returns *this.
// The setter must be usable in a call chain with the constructor and
// order().
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(fluent_api_chaining)
{
    const auto connections = std::vector {
        makeConn(0, 0, 1, 100.0),   // heel
        makeConn(2, 0, 1, 100.0),
        makeConn(1, 0, 1, 100.0),
    };

    // Construct, configure, and order in a single expression.
    const auto perm = Opm::TrackOrderingTSP { 0, 0 }
        .numberOfStartPoints(2)
        .order(connections);

    BOOST_REQUIRE_EQUAL(perm.size(), 3u);

    // Verify it produces a valid permutation.
    auto sorted_perm = perm;
    std::ranges::sort(sorted_perm);
    for (auto i = std::size_t{0}; i < sorted_perm.size(); ++i) {
        BOOST_CHECK_EQUAL(sorted_perm[i], i);
    }
}

// ---------------------------------------------------------------------------
// Test 18: Open 2-opt must allow suffix-to-end reversal (k = n - 1).
//
// This is a regression test for a case where interior-only 2-opt gets stuck
// in a worse local minimum and requires reversing [i, n-1] to improve.
// ---------------------------------------------------------------------------
BOOST_AUTO_TEST_CASE(two_opt_suffix_reversal_regression)
{
    const auto tsp = Opm::TrackOrderingTSP { 0, 0 };

    // Reproducer from review discussion:
    //   idx:  0      1      2(heel) 3      4      5
    //   IJ : (5,1), (3,4), (2,1),  (0,4), (6,0), (4,6)
    const auto connections = std::array {
        makeConn(5, 1, 1, 100.0),   // index 0
        makeConn(3, 4, 1, 100.0),   // index 1
        makeConn(2, 1, 1, 100.0),   // index 2, heel
        makeConn(0, 4, 1, 100.0),   // index 3
        makeConn(6, 0, 1, 100.0),   // index 4
        makeConn(4, 6, 1, 100.0),   // index 5
    };

    const auto perm = tsp.order(connections);
    BOOST_REQUIRE_EQUAL(perm.size(), connections.size());

    // Expected best ordering for this geometry under default weights.
    const auto expected_perm = std::vector<std::size_t>{ 2, 3, 5, 1, 0, 4 };
    BOOST_CHECK_EQUAL_COLLECTIONS(perm.begin(), perm.end(),
                                  expected_perm.begin(), expected_perm.end());

    // Confirm strict improvement over the interior-only local minimum seen
    // before enabling suffix-to-end reversals.
    const auto interior_only_perm = std::vector<std::size_t>{ 2, 4, 0, 1, 5, 3 };
    BOOST_CHECK_LT(pathCostFromOrder(connections, perm),
                   pathCostFromOrder(connections, interior_only_perm));
}
