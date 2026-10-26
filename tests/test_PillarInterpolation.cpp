/*
  Copyright 2026 Equinor ASA.

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

#define BOOST_TEST_MODULE PillarInterpolation
#include <boost/test/unit_test.hpp>

#include <opm/common/utility/numeric/PillarInterpolation.hpp>

#include <algorithm>
#include <array>
#include <limits>

namespace {
using Opm::PillarInterpolation::Pillar;
using Opm::PillarInterpolation::bilinear;

// Independent analytic line equations, with different slopes in x and y.
constexpr std::array<double, 4> x0 {0, 100, -5, 110};
constexpr std::array<double, 4> dx {0.2, -0.3, 0.4, 0.1};
constexpr std::array<double, 4> y0 {0, 5, 100, 105};
constexpr std::array<double, 4> dy {-0.1, 0.4, 0.2, -0.3};

std::array<Pillar, 4> lines(const std::array<double, 4>& top,
                            const std::array<double, 4>& bottom)
{
    std::array<Pillar, 4> result;
    for (int i = 0; i < 4; ++i) {
        result[i] = {{x0[i] + dx[i]*top[i], y0[i] + dy[i]*top[i], top[i]},
                     {x0[i] + dx[i]*bottom[i], y0[i] + dy[i]*bottom[i], bottom[i]}};
    }
    return result;
}

double atDepth(const Pillar& p, int dim, double z)
{
    return p.top[dim] + (z - p.top[2])/(p.bottom[2] - p.top[2])
        *(p.bottom[dim] - p.top[dim]);
}

void checkLine(const Pillar& p, double u, double v)
{
    const std::array<double, 4> weights {(1-u)*(1-v), u*(1-v), (1-u)*v, u*v};
    for (double z : {-100.0, 0.0, 40.0, 200.0}) {
        double x = 0.0;
        double y = 0.0;
        for (int i = 0; i < 4; ++i) {
            x += weights[i]*(x0[i] + dx[i]*z);
            y += weights[i]*(y0[i] + dy[i]*z);
        }
        BOOST_CHECK_SMALL(atDepth(p, 0, z) - x, 1e-10);
        BOOST_CHECK_SMALL(atDepth(p, 1, z) - y, 1e-10);
    }
}
// Determine support from the position on the host face, independently of
// the implementation's weight calculation.
void checkEndpointDepths(const Pillar& p, const std::array<Pillar, 4>& pillars,
                         double u, double v)
{
    double top = std::numeric_limits<double>::max();
    double bottom = std::numeric_limits<double>::lowest();
    for (int j = 0; j < 2; ++j) {
        for (int i = 0; i < 2; ++i) {
            if ((u == 0.0 && i == 1) || (u == 1.0 && i == 0)
                || (v == 0.0 && j == 1) || (v == 1.0 && j == 0)) {
                continue;
            }
            top = std::min(top, pillars[i + 2*j].top[2]);
            bottom = std::max(bottom, pillars[i + 2*j].bottom[2]);
        }
    }
    BOOST_CHECK_EQUAL(p.top[2], top);
    BOOST_CHECK_EQUAL(p.bottom[2], bottom);
}
} // namespace

BOOST_AUTO_TEST_CASE(HorizontalPlaneInterpolation)
{
    const auto pillars = lines({10, 30, -10, 0}, {90, 100, 80, 70});
    // Interior, edges, and all four original pillar positions.
    for (double u : {0.0, 0.25, 0.5, 1.0}) {
        for (double v : {0.0, 0.3, 0.5, 1.0}) {
            const auto p = bilinear(pillars, u, v);
            checkEndpointDepths(p, pillars, u, v);
            checkLine(p, u, v);
        }
    }
}

BOOST_AUTO_TEST_CASE(EndpointRepresentationDoesNotChangeTheLine)
{
    const auto first = lines({10, 30, -10, 0}, {90, 100, 80, 70});
    const auto moved = lines({-40, 20, 0, 10}, {50, 140, 60, 120});
    for (double u : {0.0, 0.25, 1.0}) {
        for (double v : {0.0, 0.7, 1.0}) {
            const auto p = bilinear(first, u, v);
            const auto q = bilinear(moved, u, v);
            checkEndpointDepths(p, first, u, v);
            checkEndpointDepths(q, moved, u, v);
            for (double z : {-100.0, 40.0, 200.0}) {
                for (int d = 0; d < 2; ++d) {
                    BOOST_CHECK_SMALL(atDepth(p, d, z) - atDepth(q, d, z), 1e-10);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(CommonEndpointPlanesPreserveEndpointInterpolation)
{
    const auto pillars = lines({0, 0, 0, 0}, {100, 100, 100, 100});
    const auto p = bilinear(pillars, 0.25, 0.75);
    const std::array<double, 4> weights {0.1875, 0.0625, 0.5625, 0.1875};
    for (int d = 0; d < 3; ++d) {
        double expectedTop = 0.0;
        double expectedBottom = 0.0;
        for (int i = 0; i < 4; ++i) {
            expectedTop += weights[i]*pillars[i].top[d];
            expectedBottom += weights[i]*pillars[i].bottom[d];
        }
        BOOST_CHECK_SMALL(p.top[d] - expectedTop, 1e-10);
        BOOST_CHECK_SMALL(p.bottom[d] - expectedBottom, 1e-10);
    }
}

BOOST_AUTO_TEST_CASE(DegeneratePillarUsesEclipseGridConvention)
{
    auto pillars = lines({0, 0, 0, 0}, {100, 100, 100, 100});
    pillars[0] = {{3, 7, 20}, {3, 7, 20}};
    const auto p = bilinear(pillars, 0, 0);
    BOOST_CHECK_EQUAL(p.top[0], 3);
    BOOST_CHECK_EQUAL(p.top[1], 7);
    BOOST_CHECK_EQUAL(p.top[2], 20);
    BOOST_CHECK_EQUAL(p.bottom[0], 3);
    BOOST_CHECK_EQUAL(p.bottom[1], 7);
    BOOST_CHECK_EQUAL(p.bottom[2], 20);
}

BOOST_AUTO_TEST_CASE(PlanarConvergingPillarsStayOrdered)
{
    const std::array<Pillar, 4> pillars {{
        {{0, 0, 0}, {0, 0, 600}}, {{100, 0, 0}, {40, 0, 600}},
        {{0, 100, 0}, {0, 100, 600}}, {{100, 100, 0}, {40, 100, 600}}
    }};
    const auto midpoint = bilinear(pillars, 0.5, 0.5);
    BOOST_CHECK_EQUAL(midpoint.bottom[0], 20);
    BOOST_CHECK_GT(40 - midpoint.bottom[0], 0);
}

BOOST_AUTO_TEST_CASE(CoordImportExport)
{
    const std::array<double, 6> coord {1, 2, 3, 4, 5, 6};
    const auto pillar = Pillar::fromCoord(coord.data());
    const std::array<double, 3> top {1, 2, 3};
    const std::array<double, 3> bottom {4, 5, 6};
    BOOST_CHECK_EQUAL_COLLECTIONS(pillar.top.begin(), pillar.top.end(), top.begin(), top.end());
    BOOST_CHECK_EQUAL_COLLECTIONS(pillar.bottom.begin(), pillar.bottom.end(), bottom.begin(), bottom.end());

    std::array<double, 8> stored {-1, 0, 0, 0, 0, 0, 0, -2};
    pillar.writeCoord(stored.data() + 1);
    BOOST_CHECK_EQUAL_COLLECTIONS(stored.begin() + 1, stored.end() - 1, coord.begin(), coord.end());
    BOOST_CHECK_EQUAL(stored.front(), -1);
    BOOST_CHECK_EQUAL(stored.back(), -2);
}

BOOST_AUTO_TEST_CASE(LineEvaluation)
{
    const Pillar pillar {{1, 2, 10}, {5, -6, 30}};
    for (double z : {-10.0, 10.0, 20.0, 30.0, 50.0}) {
        const auto point = pillar.atDepth(z);
        BOOST_CHECK_SMALL(point[0] - (0.2*z - 1), 1e-12);
        BOOST_CHECK_SMALL(point[1] - (6 - 0.4*z), 1e-12);
        BOOST_CHECK_EQUAL(point[2], z);
    }
    const Pillar degenerate {{3, 7, 20}, {9, 11, 20}};
    for (double z : {-10.0, 20.0, 50.0}) {
        const auto point = degenerate.atDepth(z);
        BOOST_CHECK_EQUAL(point[0], 3);
        BOOST_CHECK_EQUAL(point[1], 7);
        BOOST_CHECK_EQUAL(point[2], z);
    }
}

// Each side has different off-face pillars, but the shared line must have
// exactly the same representation from either column.
BOOST_AUTO_TEST_CASE(SharedPillarIsTheSameFromEitherColumn)
{
    const Pillar p0 {{0, 0, -20}, {0, 0, 120}};
    const Pillar p1 {{100, 0, -50}, {103, 6, 170}};
    const Pillar p2 {{200, 0, -100}, {206, 12, 240}};
    const Pillar q0 {{0, 100, -10}, {-3, 104, 130}};
    const Pillar q1 {{100, 100, -40}, {101, 107, 160}};
    const Pillar q2 {{200, 100, -70}, {205, 111, 210}};
    for (double v : {0.0, 0.3, 0.7, 1.0}) {
        const auto left = bilinear({p0, p1, q0, q1}, 1.0, v);
        const auto right = bilinear({p1, p2, q1, q2}, 0.0, v);
        BOOST_CHECK(left.top == right.top);
        BOOST_CHECK(left.bottom == right.bottom);
    }
    for (double u : {0.0, 0.3, 0.7, 1.0}) {
        const auto front = bilinear({p0, q0, p1, q1}, u, 1.0);
        const auto back = bilinear({p1, q1, p2, q2}, u, 0.0);
        BOOST_CHECK(front.top == back.top);
        BOOST_CHECK(front.bottom == back.bottom);
    }
}

BOOST_AUTO_TEST_CASE(ZeroWeightPillarsAreNotEvaluated)
{
    const Pillar active {{3, 7, 0}, {4, 9, 100}};
    // All inputs are finite, but evaluating the unused lines would overflow.
    const double large = std::numeric_limits<double>::max();
    const Pillar unused {{large, large, 10}, {-large, -large, 20}};
    const auto p = bilinear({active, unused, unused, unused}, 0.0, 0.0);
    BOOST_CHECK(p.top == active.top);
    BOOST_CHECK(p.bottom == active.bottom);
}
