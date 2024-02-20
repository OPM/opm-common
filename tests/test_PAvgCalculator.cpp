/*
  Copyright (c) 2023 Equinor ASA

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

#define BOOST_TEST_MODULE test_PAvgCalculator

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Schedule/Well/PAvgCalculator.hpp>

#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include <opm/input/eclipse/Schedule/Well/Connection.hpp>
#include <opm/input/eclipse/Schedule/Well/PAvg.hpp>
#include <opm/input/eclipse/Schedule/Well/PAvgDynamicSourceData.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <opm/input/eclipse/Units/Units.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

// ===========================================================================

namespace {
    std::size_t globIndex(const std::array<int,3>& ijk,
                          const std::array<int,3>& dims)
    {
        return ijk[0] + dims[0]*(ijk[1] + static_cast<std::size_t>(dims[1])*ijk[2]);
    }

    Opm::WellConnections qfsProducer(const std::array<int,3>& dims, const int top = 0)
    {
        auto conns = std::vector<Opm::Connection>{};

        // One cell in from corner in both I- and J- directions.
        const auto i = (dims[0] - 1) - 1;
        const auto j = (dims[1] - 1) - 1;

        for (auto k = top; k < static_cast<int>(dims.size()); ++k) {
            const auto depth = 2000 + (2*k + 1) / static_cast<double>(2);

            auto ctf_props = Opm::Connection::CTFProperties{};
            ctf_props.CF = k / 100.0;
            ctf_props.Kh = 1.0;
            ctf_props.Ke = 1.0;
            ctf_props.rw = 1.0;
            ctf_props.r0 = 0.5;
            ctf_props.re = 0.5;
            ctf_props.connection_length = 1.0;

            conns.emplace_back(i, j, k, globIndex({i, j, k}, dims), k,
                               Opm::Connection::State::OPEN,
                               Opm::Connection::Direction::Z,
                               Opm::Connection::CTFKind::DeckValue,
                               0, depth, ctf_props, k, false);
        }

        return { Opm::Connection::Order::INPUT, i, j, conns };
    }

    Opm::WellConnections centreProducer(const int numLayers = 10,
                                        const int topConn   = 0,
                                        const int numConns  = 4)
    {
        auto conns = std::vector<Opm::Connection>{};

        const auto dims = std::array { 5, 5, numLayers };

        const auto i = 2;
        const auto j = 2;
        const auto kMax = std::min(dims[2] - 1, topConn + numConns);

        const auto state = std::array {
            Opm::Connection::State::OPEN,
            Opm::Connection::State::SHUT,
            Opm::Connection::State::OPEN,
        };

        for (auto k = topConn; k < kMax; ++k) {
            const auto depth = 2000 + (2*k + 1) / static_cast<double>(2);

            auto ctf_props = Opm::Connection::CTFProperties{};

            // 0.03, 0.0, 0.01, 0.02, 0.03, ...
            ctf_props.CF = ((k + 3 - topConn) % 4) / 100.0;

            ctf_props.Kh = 1.0;
            ctf_props.Ke = 1.0;
            ctf_props.rw = 1.0;
            ctf_props.r0 = 0.5;
            ctf_props.re = 0.5;
            ctf_props.connection_length = 1.0;

            conns.emplace_back(i, j, k, globIndex({i, j, k}, dims), k - topConn,

                               // Open, Shut, Open, Open, Shut, ...
                               state[(k - topConn) % state.size()],

                               Opm::Connection::Direction::Z,
                               Opm::Connection::CTFKind::DeckValue,
                               0, depth, ctf_props, k - topConn, false);
        }

        return { Opm::Connection::Order::INPUT, i, j, conns };
    }

    Opm::WellConnections horizontalProducer_X(const std::array<int,3>& dims,
                                              const int                left     = 0,
                                              const int                numConns = 3)
    {
        auto conns = std::vector<Opm::Connection>{};

        const auto j = std::max((dims[1] - 1) - 1, 1);
        const auto k = dims[2] - 1;
        const auto iMax = std::min(dims[0] - 1, left + numConns);

        for (auto i = left; i < iMax; ++i) {
            const auto depth = 2000 + (2*k + 1) / static_cast<double>(2);

            auto ctf_props = Opm::Connection::CTFProperties{};
            ctf_props.CF = i / 100.0;
            ctf_props.Kh = 1.0;
            ctf_props.Ke = 1.0;
            ctf_props.rw = 1.0;
            ctf_props.r0 = 0.5;
            ctf_props.re = 0.5;
            ctf_props.connection_length = 1.0;

            conns.emplace_back(i, j, k, globIndex({i, j, k}, dims), i - left,
                               Opm::Connection::State::OPEN,
                               Opm::Connection::Direction::X,
                               Opm::Connection::CTFKind::DeckValue,
                               0, depth, ctf_props, i - left, false);
        }

        return { Opm::Connection::Order::INPUT, left, j, conns };
    }

    Opm::EclipseGrid shoeBox(const std::array<int,3>& dims)
    {
        return Opm::EclipseGrid(dims[0], dims[1], dims[2]);
    }

    double standardGravity()
    {
        return Opm::unit::gravity;
    }

    double simpleCalculationGravity()
    {
        return 10.0;
    }

    template <typename T>
    std::vector<T> sort(std::vector<T> v)
    {
        std::sort(v.begin(), v.end());
        return v;
    }

    template <typename T>
    std::vector<bool>
    makeActiveMap(const std::vector<T>& v, const std::vector<T>& activeElements)
    {
        auto isActive = std::vector<bool>{};
        isActive.reserve(v.size());

        for (const auto& elm : v) {
            isActive.push_back(std::binary_search(activeElements.begin(),
                                                  activeElements.end(), elm));
        }

        return isActive;
    }
} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(Basic_Operations)

BOOST_AUTO_TEST_CASE(Construct)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in Z direction in column (9,9) of bottom layer,
    // meaning cell (9,9,3) only.
    auto prod = Opm::PAvgCalculator {
        shoeBox(dims), qfsProducer(dims, 2)
    };

    const auto wbpCells = sort(prod.allWBPCells());
    const auto wbpConns = sort(prod.allWellConnections());

    const auto expectCells = sort(std::vector {
        globIndex({7, 7, 2}, dims), globIndex({8, 7, 2}, dims), globIndex({9, 7, 2}, dims),
        globIndex({7, 8, 2}, dims), globIndex({8, 8, 2}, dims), globIndex({9, 8, 2}, dims),
        globIndex({7, 9, 2}, dims), globIndex({8, 9, 2}, dims), globIndex({9, 9, 2}, dims),
    });

    const auto expectConns = std::vector { std::size_t{0} };

    BOOST_CHECK_EQUAL_COLLECTIONS(wbpCells   .begin(), wbpCells   .end(),
                                  expectCells.begin(), expectCells.end());

    BOOST_CHECK_EQUAL_COLLECTIONS(wbpConns   .begin(), wbpConns   .end(),
                                  expectConns.begin(), expectConns.end());
}

BOOST_AUTO_TEST_CASE(Construct_Three_Layers)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in Z direction in column (9,9) of all layers,
    // meaning cells (9,9,1), (9,9,2), and (9,9,3).
    auto prod = Opm::PAvgCalculator {
        shoeBox(dims), qfsProducer(dims)
    };

    const auto wbpCells = sort(prod.allWBPCells());
    const auto wbpConns = sort(prod.allWellConnections());

    const auto expectCells = sort(std::vector {
        globIndex({7, 7, 0}, dims), globIndex({8, 7, 0}, dims), globIndex({9, 7, 0}, dims),
        globIndex({7, 8, 0}, dims), globIndex({8, 8, 0}, dims), globIndex({9, 8, 0}, dims),
        globIndex({7, 9, 0}, dims), globIndex({8, 9, 0}, dims), globIndex({9, 9, 0}, dims),

        globIndex({7, 7, 1}, dims), globIndex({8, 7, 1}, dims), globIndex({9, 7, 1}, dims),
        globIndex({7, 8, 1}, dims), globIndex({8, 8, 1}, dims), globIndex({9, 8, 1}, dims),
        globIndex({7, 9, 1}, dims), globIndex({8, 9, 1}, dims), globIndex({9, 9, 1}, dims),

        globIndex({7, 7, 2}, dims), globIndex({8, 7, 2}, dims), globIndex({9, 7, 2}, dims),
        globIndex({7, 8, 2}, dims), globIndex({8, 8, 2}, dims), globIndex({9, 8, 2}, dims),
        globIndex({7, 9, 2}, dims), globIndex({8, 9, 2}, dims), globIndex({9, 9, 2}, dims),
    });

    const auto expectConns = std::vector {
        std::size_t{0}, std::size_t{1}, std::size_t{2},
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(wbpCells   .begin(), wbpCells   .end(),
                                  expectCells.begin(), expectCells.end());

    BOOST_CHECK_EQUAL_COLLECTIONS(wbpConns   .begin(), wbpConns   .end(),
                                  expectConns.begin(), expectConns.end());
}

BOOST_AUTO_TEST_CASE(Construct_Horizontal_X_5_Cols)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in X direction in columns 3:7 of row (:,9,3)
    // meaning cells (3,9,3), (4,9,3), (5,9,3), (6,9,3), and (7,9,3).
    auto prod = Opm::PAvgCalculator {
        shoeBox(dims), horizontalProducer_X(dims, 2, 5)
    };

    const auto wbpCells = sort(prod.allWBPCells());
    const auto wbpConns = sort(prod.allWellConnections());

    const auto expectCells = sort(std::vector {
        // Column 2
        globIndex({2, 7, 1}, dims), globIndex({2, 8, 1}, dims), globIndex({2, 9, 1}, dims),
        globIndex({2, 7, 2}, dims), globIndex({2, 8, 2}, dims), globIndex({2, 9, 2}, dims),

        // Column 3
        globIndex({3, 7, 1}, dims), globIndex({3, 8, 1}, dims), globIndex({3, 9, 1}, dims),
        globIndex({3, 7, 2}, dims), globIndex({3, 8, 2}, dims), globIndex({3, 9, 2}, dims),

        // Column 4
        globIndex({4, 7, 1}, dims), globIndex({4, 8, 1}, dims), globIndex({4, 9, 1}, dims),
        globIndex({4, 7, 2}, dims), globIndex({4, 8, 2}, dims), globIndex({4, 9, 2}, dims),

        // Column 5
        globIndex({5, 7, 1}, dims), globIndex({5, 8, 1}, dims), globIndex({5, 9, 1}, dims),
        globIndex({5, 7, 2}, dims), globIndex({5, 8, 2}, dims), globIndex({5, 9, 2}, dims),

        // Column 6
        globIndex({6, 7, 1}, dims), globIndex({6, 8, 1}, dims), globIndex({6, 9, 1}, dims),
        globIndex({6, 7, 2}, dims), globIndex({6, 8, 2}, dims), globIndex({6, 9, 2}, dims),
    });

    const auto expectConns = std::vector {
        std::size_t{0}, std::size_t{1}, std::size_t{2}, std::size_t{3}, std::size_t{4},
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(wbpCells   .begin(), wbpCells   .end(),
                                  expectCells.begin(), expectCells.end());

    BOOST_CHECK_EQUAL_COLLECTIONS(wbpConns   .begin(), wbpConns   .end(),
                                  expectConns.begin(), expectConns.end());
}

BOOST_AUTO_TEST_CASE(Prune_Inactive_Cells)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in Z direction in column (9,9) of all layers,
    // meaning cells (9,9,1), (9,9,2), and (9,9,3).
    auto prod = Opm::PAvgCalculator {
        shoeBox(dims), qfsProducer(dims)
    };

    const auto expectCells = sort(std::vector {
        globIndex({7, 7, 0}, dims),                             globIndex({9, 7, 0}, dims),
                                    globIndex({8, 8, 0}, dims),
        globIndex({7, 9, 0}, dims),                             globIndex({9, 9, 0}, dims),

                                    globIndex({8, 7, 1}, dims),
        globIndex({7, 8, 1}, dims), globIndex({8, 8, 1}, dims), globIndex({9, 8, 1}, dims),
                                    globIndex({8, 9, 1}, dims),

        globIndex({7, 7, 2}, dims), globIndex({8, 7, 2}, dims), globIndex({9, 7, 2}, dims),
        globIndex({7, 8, 2}, dims), globIndex({8, 8, 2}, dims), globIndex({9, 8, 2}, dims),
        globIndex({7, 9, 2}, dims), globIndex({8, 9, 2}, dims), globIndex({9, 9, 2}, dims),
    });

    prod.pruneInactiveWBPCells(makeActiveMap(prod.allWBPCells(), expectCells));

    const auto wbpCells = sort(prod.allWBPCells());
    const auto wbpConns = sort(prod.allWellConnections());

    const auto expectConns = std::vector {
        std::size_t{0}, std::size_t{1}, std::size_t{2},
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(wbpCells   .begin(), wbpCells   .end(),
                                  expectCells.begin(), expectCells.end());

    BOOST_CHECK_EQUAL_COLLECTIONS(wbpConns   .begin(), wbpConns   .end(),
                                  expectConns.begin(), expectConns.end());
}

BOOST_AUTO_TEST_CASE(Prune_Inactive_Connections)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in Z direction in column (9,9) of all layers,
    // meaning cells (9,9,1), (9,9,2), and (9,9,3).
    auto prod = Opm::PAvgCalculator {
        shoeBox(dims), qfsProducer(dims)
    };

    const auto expectCells = sort(std::vector {
        globIndex({7, 7, 0}, dims), globIndex({8, 7, 0}, dims), globIndex({9, 7, 0}, dims),
        globIndex({7, 8, 0}, dims),                             globIndex({9, 8, 0}, dims),
        globIndex({7, 9, 0}, dims), globIndex({8, 9, 0}, dims), globIndex({9, 9, 0}, dims),

        globIndex({7, 7, 1}, dims), globIndex({8, 7, 1}, dims), globIndex({9, 7, 1}, dims),
        globIndex({7, 8, 1}, dims),                             globIndex({9, 8, 1}, dims),
        globIndex({7, 9, 1}, dims), globIndex({8, 9, 1}, dims), globIndex({9, 9, 1}, dims),

        globIndex({7, 7, 2}, dims), globIndex({8, 7, 2}, dims), globIndex({9, 7, 2}, dims),
        globIndex({7, 8, 2}, dims),                             globIndex({9, 8, 2}, dims),
        globIndex({7, 9, 2}, dims), globIndex({8, 9, 2}, dims), globIndex({9, 9, 2}, dims),
    });

    prod.pruneInactiveWBPCells(makeActiveMap(prod.allWBPCells(), expectCells));

    const auto wbpCells = sort(prod.allWBPCells());
    const auto wbpConns = sort(prod.allWellConnections());

    const auto expectConns = std::vector {
        std::size_t{0}, std::size_t{1}, std::size_t{2},
    };

    BOOST_CHECK_EQUAL_COLLECTIONS(wbpCells   .begin(), wbpCells   .end(),
                                  expectCells.begin(), expectCells.end());

    // Well connection source locations must be full input set even if all
    // connections are in deactivated cells.
    BOOST_CHECK_EQUAL_COLLECTIONS(wbpConns   .begin(), wbpConns   .end(),
                                  expectConns.begin(), expectConns.end());
}

BOOST_AUTO_TEST_SUITE_END() // Basic_Operations

// ===========================================================================

namespace {
    struct CalculatorSetup
    {
        template <typename... Args>
        CalculatorSetup(Args&&... args)
            : calc        { std::forward<Args>(args)... }
            , wbpCells    { sort(calc.allWBPCells()) }
            , wbpConns    { sort(calc.allWellConnections()) }
            , blockSource { wbpCells }
            , connSource  { wbpConns }
        {
            this->sources.wellBlocks(blockSource).wellConns(connSource);
        }

        Opm::PAvgCalculator          calc;
        std::vector<std::size_t>     wbpCells{};
        std::vector<std::size_t>     wbpConns{};
        Opm::PAvgDynamicSourceData<double> blockSource;
        Opm::PAvgDynamicSourceData<double> connSource;
        Opm::PAvgCalculator::Sources sources{};
    };

    namespace AveragingControls {
        Opm::PAvg defaults()
        {
            // F1=0.5, F2=1, DepthCorr=Well, Conn=Open
            return {};
        }

        namespace F1 {
            Opm::PAvg zero()
            {
                // Default, except set F1 = 0.0;
                const auto F1 = 0.0;
                const auto dflt = defaults();

                return {
                    F1,
                    dflt.conn_weight(),
                    dflt.depth_correction(),
                    dflt.open_connections()
                };
            }

            Opm::PAvg negative()
            {
                // Default, except set F1 < 0.0;
                const auto F1 = -1.0;   // < 0
                const auto dflt = defaults();

                return {
                    F1,
                    dflt.conn_weight(),
                    dflt.depth_correction(),
                    dflt.open_connections()
                };
            }

            Opm::PAvg small()
            {
                // Default, except set F1 to a small value
                const auto F1 = 1.0e-2;
                const auto dflt = defaults();

                return {
                    F1,
                    dflt.conn_weight(),
                    dflt.depth_correction(),
                    dflt.open_connections()
                };
            }

            Opm::PAvg high()
            {
                // Default, except set F1 to a high value
                const auto F1 = 1.0 - 1.0e-2;
                const auto dflt = defaults();

                return {
                    F1,
                    dflt.conn_weight(),
                    dflt.depth_correction(),
                    dflt.open_connections()
                };
            }

            Opm::PAvg max()
            {
                // Default, except set F1 to its maximum value (1.0)
                const auto F1 = 1.0;
                const auto dflt = defaults();

                return {
                    F1,
                    dflt.conn_weight(),
                    dflt.depth_correction(),
                    dflt.open_connections()
                };
            }
        } // namespace F1

        namespace F2 {
            Opm::PAvg zero()
            {
                // Default, except set F2 to zero
                const auto F2 = 0.0;
                const auto dflt = defaults();

                return {
                    dflt.inner_weight(),
                    F2,
                    dflt.depth_correction(),
                    dflt.open_connections()
                };
            }

            Opm::PAvg small()
            {
                // Default, except set F2 to a small value
                const auto F2 = 1.0e-2;
                const auto dflt = defaults();

                return {
                    dflt.inner_weight(),
                    F2,
                    dflt.depth_correction(),
                    dflt.open_connections()
                };
            }

            Opm::PAvg quarter()
            {
                // Default, except set F2 to 1/4
                const auto F2 = 0.25;
                const auto dflt = defaults();

                return {
                    dflt.inner_weight(),
                    F2,
                    dflt.depth_correction(),
                    dflt.open_connections()
                };
            }

            Opm::PAvg mid()
            {
                // Default, except set F2 to 1/2
                const auto F2 = 0.5;
                const auto dflt = defaults();

                return {
                    dflt.inner_weight(),
                    F2,
                    dflt.depth_correction(),
                    dflt.open_connections()
                };
            }

            Opm::PAvg threeQuarters()
            {
                // Default, except set F2 to 3/4
                const auto F2 = 0.75;
                const auto dflt = defaults();

                return {
                    dflt.inner_weight(),
                    F2,
                    dflt.depth_correction(),
                    dflt.open_connections()
                };
            }

            Opm::PAvg high()
            {
                // Default, except set F2 to a high value (near default of 1)
                const auto F2 = 1.0 - 1.0e-2;
                const auto dflt = defaults();

                return {
                    dflt.inner_weight(),
                    F2,
                    dflt.depth_correction(),
                    dflt.open_connections()
                };
            }
        } // namespace F2

        namespace Connections {
            Opm::PAvg open()
            {
                // Same as default
                return defaults();
            }

            Opm::PAvg all()
            {
                // Default, except averaging procedure applies to all
                // connections whether open or shut.
                const auto use_open = false;
                const auto dflt = defaults();

                return {
                    dflt.inner_weight(),
                    dflt.conn_weight(),
                    dflt.depth_correction(),
                    use_open
                };
            }
        } // namespace DepthCorr

        namespace DepthCorrection {
            Opm::PAvg well_open()
            {
                // Same as default
                return defaults();
            }

            Opm::PAvg well_all()
            {
                // Default, except averaging procedure applies to all
                // connections whether open or shut.
                const auto use_open = false;
                const auto dflt = defaults();

                return {
                    dflt.inner_weight(),
                    dflt.conn_weight(),
                    dflt.depth_correction(),
                    use_open
                };
            }

            Opm::PAvg reservoir_open()
            {
                // Default, except mixture density for depth correction is
                // inferred from grid blocks.
                const auto depth_correction = Opm::PAvg::DepthCorrection::RES;
                const auto dflt = defaults();

                return {
                    dflt.inner_weight(),
                    dflt.conn_weight(),
                    depth_correction,
                    dflt.open_connections()
                };
            }

            Opm::PAvg reservoir_all()
            {
                // Default, except mixture density for depth correction is
                // inferred from grid blocks of all connections.
                const auto depth_correction = Opm::PAvg::DepthCorrection::RES;
                const auto use_open = false;
                const auto dflt = defaults();

                return {
                    dflt.inner_weight(),
                    dflt.conn_weight(),
                    depth_correction,
                    use_open
                };
            }

            Opm::PAvg none_open()
            {
                // Default, except we don't use depth-corrected pressure values
                const auto depth_correction = Opm::PAvg::DepthCorrection::NONE;
                const auto dflt = defaults();

                return {
                    dflt.inner_weight(),
                    dflt.conn_weight(),
                    depth_correction,
                    dflt.open_connections()
                };
            }

            Opm::PAvg none_all()
            {
                // Default, except we don't use depth-corrected pressure
                // values and we infer WBPn from all well connections.
                const auto depth_correction = Opm::PAvg::DepthCorrection::NONE;
                const auto use_open = false;
                const auto dflt = defaults();

                return {
                    dflt.inner_weight(),
                    dflt.conn_weight(),
                    depth_correction,
                    use_open
                };
            }
        }
    } // namespace AverageControls
} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(Equal_Pore_Volumes)

namespace {
    struct Setup : public CalculatorSetup
    {
        Setup(const std::array<int,3>& dims, const int top = 2)
            : CalculatorSetup { shoeBox(dims), qfsProducer(dims, top) }
        {}
    };

    void assignCellPress(const std::vector<double>& cellPress,
                         Setup&                     cse)
    {
        using Span = std::remove_cv_t<
            std::remove_reference_t<decltype(cse.blockSource[0])>>;
        using Item = typename Span::Item;

        for (auto block = 0*cellPress.size(); block < cellPress.size(); ++block) {
            cse.blockSource[cse.wbpCells[block]]
                .set(Item::Pressure, cellPress[block])
                .set(Item::PoreVol, 1.25)
                .set(Item::MixtureDensity, 0.1);
        }
    }

    void assignConnPress(const std::vector<double>& connPress,
                         Setup&                     cse)
    {
        using Span = std::remove_cv_t<
            std::remove_reference_t<decltype(cse.connSource[0])>>;
        using Item = typename Span::Item;

        for (auto conn = 0*connPress.size(); conn < connPress.size(); ++conn) {
            cse.connSource[conn]
                .set(Item::Pressure, connPress[conn])
                .set(Item::PoreVol, 0.5)
                .set(Item::MixtureDensity, 0.1);
        }
    }

    void assignPressureBottomLayer(Setup& cse)
    {
        assignCellPress({
            85.0,  90.0,  95.0,
            90.0, 100.0, 110.0,
            90.0, 100.0, 120.0,
        }, cse);

        assignConnPress({80.0}, cse);
    }

    // Special pressure field to generate WBPn = 42 with default WPAVE
    // controls (PAvg{}) and simpleCalculationGravity().
    void assignPressureThreeLayers_Symmetric_42(Setup& cse)
    {
        assignCellPress({
            // K=0 => 40
            20.0, 30.0, 40.0,
            30.0, 40.0, 50.0,
            40.0, 50.0, 60.0,

            // K=1 => 41 (+ 1)
            22.0, 32.0, 42.0,
            32.0, 42.0, 52.0,
            42.0, 52.0, 62.0,

            // K=2 => 42.5 (+ 2)
            24.5, 34.5, 44.5,
            34.5, 44.5, 54.5,
            44.5, 54.5, 64.5,
        }, cse);

        assignConnPress({35.0, 37.0, 39.0}, cse);
    }

    // Random pressure field generated by Octave statement
    //
    //     1234 + fix(100 * rand)
    void assignPressureThreeLayers_Rand_1234(Setup& cse)
    {
        assignCellPress({
            // K=0
            1245.0,  1283.0,  1329.0,
            1268.0,  1292.0,  1256.0,
            1309.0,  1259.0,  1284.0,

            // K=1
            1303.0,  1323.0,  1329.0,
            1288.0,  1247.0,  1248.0,
            1259.0,  1318.0,  1259.0,

            // K=2
            1315.0,  1258.0,  1326.0,
            1268.0,  1253.0,  1259.0,
            1295.0,  1281.0,  1269.0,
        }, cse);

        assignConnPress({1222.0, 1232.0, 1242.0}, cse);
    }
} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(Bottom_Layer)

BOOST_AUTO_TEST_CASE(Default_Control_No_Depth_Difference)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in cell (9,9,3) only
    Setup cse { dims };

    assignPressureBottomLayer(cse);

    const auto controls = AveragingControls::defaults();
    const auto gravity  = standardGravity();
    const auto refDepth = 2002.5; // BHP reference depth => zero depth correction

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 100.00, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4),  97.50, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5),  98.75, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9),  98.75, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Default_Control_Elevate_Two_Cell_Thicknesses)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in cell (9,9,3) only
    Setup cse { dims };

    assignPressureBottomLayer(cse);

    const auto controls = AveragingControls::defaults();
    const auto gravity  = simpleCalculationGravity();
    const auto refDepth = 2000.5; // BHP reference depth - 2m => 2 Pa depth correction

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 100.00 - 2.0, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4),  97.50 - 2.0, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5),  98.75 - 2.0, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9),  98.75 - 2.0, 1.0e-8);
}

BOOST_AUTO_TEST_SUITE_END() // Bottom_Layer

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(All_Layers)

BOOST_AUTO_TEST_CASE(Default_Control_WBPn_42)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in Z direction in cells (9,9,1), (9,9,2), and
    // (9,9,3)
    Setup cse { dims, 0 };

    assignPressureThreeLayers_Symmetric_42(cse);

    const auto controls = AveragingControls::defaults();
    const auto gravity  = simpleCalculationGravity();
    const auto refDepth = 2000.5; // BHP reference depth => depth correction in layers 1, and 2.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 42.0, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 42.0, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 42.0, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 42.0, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Default_Control_Rand_1234)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in Z direction in cells (9,9,1), (9,9,2), and
    // (9,9,3)
    Setup cse { dims, 0 };

    assignPressureThreeLayers_Rand_1234(cse);

    const auto controls = AveragingControls::defaults();
    const auto gravity  = simpleCalculationGravity();
    const auto refDepth = 2000.5; // BHP reference depth => depth correction in layers 1, and 2.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1249.3333333333335, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1274.0833333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1261.7083333333335, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1266.9375000000000, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Default_Control_Rand_1234_Depth_Bottom_of_Centre)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in Z direction in cells (9,9,1), (9,9,2), and
    // (9,9,3)
    Setup cse { dims, 0 };

    assignPressureThreeLayers_Rand_1234(cse);

    const auto controls = AveragingControls::defaults();
    const auto gravity  = simpleCalculationGravity();
    const auto refDepth = 2002.0; // Centre layer bottom depth => depth correction in all layers

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1250.833333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1275.583333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1263.208333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1268.437500000000, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Rand_1234_Centre_F1_Zero)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in Z direction in cells (9,9,1), (9,9,2), and
    // (9,9,3)
    Setup cse { dims, 0 };

    assignPressureThreeLayers_Rand_1234(cse);

    const auto controls = AveragingControls::F1::zero();
    const auto gravity  = simpleCalculationGravity();
    const auto refDepth = 2001.5; // Centre layer centre depth => depth correction in layers 0 and 2.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1250.333333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1275.083333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1275.083333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1285.541666666667, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Rand_1234_Centre_F1_Negative)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in Z direction in cells (9,9,1), (9,9,2), and
    // (9,9,3)
    Setup cse { dims, 0 };

    assignPressureThreeLayers_Rand_1234(cse);

    const auto controls = AveragingControls::F1::negative();
    const auto gravity  = simpleCalculationGravity();
    const auto refDepth = 2001.5; // Centre layer centre depth => depth correction in layers 0 and 2.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1250.333333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1275.083333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1270.133333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1281.629629629630, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Rand_1234_Centre_F1_Small)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in Z direction in cells (9,9,1), (9,9,2), and
    // (9,9,3)
    Setup cse { dims, 0 };

    assignPressureThreeLayers_Rand_1234(cse);

    const auto controls = AveragingControls::F1::small();
    const auto gravity  = simpleCalculationGravity();
    const auto refDepth = 2001.5; // Centre layer centre depth => depth correction in layers 0 and 2.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1250.333333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1275.083333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1274.835833333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1285.189583333334, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Rand_1234_Centre_F1_High)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in Z direction in cells (9,9,1), (9,9,2), and
    // (9,9,3)
    Setup cse { dims, 0 };

    assignPressureThreeLayers_Rand_1234(cse);

    const auto controls = AveragingControls::F1::high();
    const auto gravity  = simpleCalculationGravity();
    const auto refDepth = 2001.5; // Centre layer centre depth => depth correction in layers 0 and 2.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1250.333333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1275.083333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1250.580833333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1250.685416666667, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Rand_1234_Centre_F1_Max)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in Z direction in cells (9,9,1), (9,9,2), and
    // (9,9,3)
    Setup cse { dims, 0 };

    assignPressureThreeLayers_Rand_1234(cse);

    const auto controls = AveragingControls::F1::max();
    const auto gravity  = simpleCalculationGravity();
    const auto refDepth = 2001.5; // Centre layer centre depth => depth correction in layers 0 and 2.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1250.333333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1275.083333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1250.333333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1250.333333333333, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Rand_1234_Top_F2_Zero)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in Z direction in cells (9,9,1), (9,9,2), and
    // (9,9,3)
    Setup cse { dims, 0 };

    assignPressureThreeLayers_Rand_1234(cse);

    const auto controls = AveragingControls::F2::zero();
    const auto gravity  = simpleCalculationGravity();
    const auto refDepth = 2000.0; // Top layer top depth => depth correction in all layers

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1262.500000000000, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1274.250000000000, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1271.900000000000, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1280.833333333333, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Rand_1234_Top_F2_Small)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in Z direction in cells (9,9,1), (9,9,2), and
    // (9,9,3)
    Setup cse { dims, 0 };

    assignPressureThreeLayers_Rand_1234(cse);

    const auto controls = AveragingControls::F2::small();
    const auto gravity  = simpleCalculationGravity();
    const auto refDepth = 2000.0; // Top layer top depth => depth correction in all layers

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1262.363333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1274.243333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1271.793083333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1280.689375000000, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Rand_1234_Top_F2_Quarter)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in Z direction in cells (9,9,1), (9,9,2), and
    // (9,9,3)
    Setup cse { dims, 0 };

    assignPressureThreeLayers_Rand_1234(cse);

    const auto controls = AveragingControls::F2::quarter();
    const auto gravity  = simpleCalculationGravity();
    const auto refDepth = 2000.0; // Top layer top depth => depth correction in all layers

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1259.083333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1274.083333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1269.227083333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1277.234375000000, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Rand_1234_Top_F2_Mid)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in Z direction in cells (9,9,1), (9,9,2), and
    // (9,9,3)
    Setup cse { dims, 0 };

    assignPressureThreeLayers_Rand_1234(cse);

    const auto controls = AveragingControls::F2::mid();
    const auto gravity  = simpleCalculationGravity();
    const auto refDepth = 2000.0; // Top layer top depth => depth correction in all layers

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1255.666666666667, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1273.916666666667, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1266.554166666667, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1273.635416666667, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Rand_1234_Top_F2_Three_Quarters)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in Z direction in cells (9,9,1), (9,9,2), and
    // (9,9,3)
    Setup cse { dims, 0 };

    assignPressureThreeLayers_Rand_1234(cse);

    const auto controls = AveragingControls::F2::threeQuarters();
    const auto gravity  = simpleCalculationGravity();
    const auto refDepth = 2000.0; // Top layer top depth => depth correction in all layers

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1252.250000000000, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1273.750000000000, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1263.881250000000, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1270.036458333333, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Rand_1234_Top_F2_High)
{
    const auto dims = std::array { 10, 10, 3 };

    // Producer connected in Z direction in cells (9,9,1), (9,9,2), and
    // (9,9,3)
    Setup cse { dims, 0 };

    assignPressureThreeLayers_Rand_1234(cse);

    const auto controls = AveragingControls::F2::high();
    const auto gravity  = simpleCalculationGravity();
    const auto refDepth = 2000.0; // Top layer top depth => depth correction in all layers

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1248.970000000000, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1273.590000000000, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1261.315250000000, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1266.581458333334, 1.0e-8);
}

BOOST_AUTO_TEST_SUITE_END() // All_Layers

BOOST_AUTO_TEST_SUITE_END() // Equal_Pore_Volumes

// ===========================================================================

namespace {
    // Octave: 1234 + fix(100 * rand([3, 3, 6]))
    std::vector<double> pressureField()
    {
        return {
            // K=2
            1.302000e+03, 1.308000e+03, 1.279000e+03,
            1.242000e+03, 1.256000e+03, 1.325000e+03,
            1.249000e+03, 1.316000e+03, 1.287000e+03,

            // K=3
            1.333000e+03, 1.241000e+03, 1.278000e+03,
            1.244000e+03, 1.330000e+03, 1.234000e+03,
            1.311000e+03, 1.315000e+03, 1.320000e+03,

            // K=4
            1.242000e+03, 1.273000e+03, 1.259000e+03,
            1.314000e+03, 1.277000e+03, 1.325000e+03,
            1.252000e+03, 1.260000e+03, 1.248000e+03,

            // K=5
            1.247000e+03, 1.320000e+03, 1.291000e+03,
            1.288000e+03, 1.248000e+03, 1.319000e+03,
            1.296000e+03, 1.269000e+03, 1.285000e+03,

            // K=6
            1.274000e+03, 1.241000e+03, 1.257000e+03,
            1.246000e+03, 1.252000e+03, 1.257000e+03,
            1.275000e+03, 1.238000e+03, 1.324000e+03,

            // K=7
            1.328000e+03, 1.283000e+03, 1.282000e+03,
            1.267000e+03, 1.324000e+03, 1.270000e+03,
            1.245000e+03, 1.312000e+03, 1.272000e+03,
        };
    }

    // Octave: fix(1e6 * (123.4 + 56.7*rand([3, 3, 6]))) / 1e6
    std::vector<double> poreVolume()
    {
        return {
            // K=2
            1.301471680e+02, 1.516572410e+02, 1.778174820e+02,
            1.426998700e+02, 1.565846810e+02, 1.360901360e+02,
            1.659968420e+02, 1.378638930e+02, 1.520877640e+02,

            // K=3
            1.630376500e+02, 1.739142140e+02, 1.777918230e+02,
            1.544271200e+02, 1.312600050e+02, 1.318649700e+02,
            1.380007180e+02, 1.710686680e+02, 1.378177990e+02,

            // K=4
            1.695699490e+02, 1.372078650e+02, 1.760892470e+02,
            1.432440790e+02, 1.345469500e+02, 1.376364540e+02,
            1.583297330e+02, 1.502354770e+02, 1.433390940e+02,

            // K=5
            1.705079830e+02, 1.565844730e+02, 1.545693280e+02,
            1.754048800e+02, 1.396070720e+02, 1.663332520e+02,
            1.661364390e+02, 1.449712790e+02, 1.555954870e+02,

            // K=6
            1.277009380e+02, 1.264589710e+02, 1.534962210e+02,
            1.675787810e+02, 1.763584050e+02, 1.307656820e+02,
            1.556523010e+02, 1.500144490e+02, 1.240748470e+02,

            // K=7
            1.425148530e+02, 1.325957360e+02, 1.684359330e+02,
            1.410458920e+02, 1.533678280e+02, 1.327922820e+02,
            1.575323760e+02, 1.383104710e+02, 1.604862840e+02,
        };
    }

    // Octave: 0.1 + round(0.1 * rand([3, 3, 6]), 2)
    std::vector<double> mixtureDensity()
    {
        return {
            // K=2
            0.120, 0.120, 0.120,
            0.140, 0.130, 0.190,
            0.140, 0.120, 0.190,

            // K=3
            0.200, 0.140, 0.110,
            0.130, 0.140, 0.160,
            0.130, 0.160, 0.170,

            // K=4
            0.120, 0.110, 0.130,
            0.130, 0.140, 0.150,
            0.110, 0.130, 0.180,

            // K=5
            0.100, 0.190, 0.170,
            0.150, 0.160, 0.120,
            0.150, 0.200, 0.150,

            // K=6
            0.150, 0.120, 0.150,
            0.160, 0.170, 0.140,
            0.140, 0.200, 0.100,

            // K=7
            0.190, 0.190, 0.180,
            0.110, 0.130, 0.130,
            0.170, 0.110, 0.170,
        };
    }
} // Anonymous namespace

BOOST_AUTO_TEST_SUITE(Open_Shut)

BOOST_AUTO_TEST_SUITE(Equal_Pore_Volumes)

namespace {
    struct Setup : public CalculatorSetup
    {
        Setup()
            : CalculatorSetup { shoeBox({5, 5, 10}), centreProducer(10, 2, 6) }
        {
            using Span = std::remove_cv_t<
                std::remove_reference_t<decltype(this->blockSource[0])>>;
            using Item = typename Span::Item;

            const auto cellPress = pressureField();

            for (auto block = 0*cellPress.size(); block < cellPress.size(); ++block) {
                this->blockSource[this->wbpCells[block]]
                    .set(Item::Pressure, cellPress[block])
                    .set(Item::PoreVol, 1.25)
                    .set(Item::MixtureDensity, 0.1);
            }

            for (auto conn = 0*this->wbpConns.size(); conn < this->wbpConns.size(); ++conn) {
                this->connSource[conn]
                    .set(Item::Pressure, 1222.0)
                    .set(Item::PoreVol, 1.25)
                    .set(Item::MixtureDensity, 0.1);
            }
        }
    };
}

BOOST_AUTO_TEST_CASE(TopOfFormation_OpenConns)
{
    // Producer connected in Z direction in cells (3,3,3), (3,3,4), (3,3,5),
    // (3,3,6), (3,3,7), and (3,3,8).  Connections (3,3,4) and (3,3,7) are
    // shut.
    Setup cse{};

    const auto controls = AveragingControls::Connections::open();
    const auto gravity  = simpleCalculationGravity();
    const auto refDepth = 2000.0; // Top of formation.  Depth correction in all layers.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1253.000000000000, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1293.541666666667, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1273.270833333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1267.572916666667, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(TopOfFormation_AllConns_StandardGravity)
{
    // Producer connected in Z direction in cells (3,3,3), (3,3,4), (3,3,5),
    // (3,3,6), (3,3,7), and (3,3,8).
    Setup cse{};

    const auto controls = AveragingControls::Connections::all();
    const auto gravity  = standardGravity();
    const auto refDepth = 2000.0; // Top of formation.  Depth correction in all layers.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1250.591304166667, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1275.452415277778, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1263.021859722222, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1262.306581944444, 1.0e-8);
}

BOOST_AUTO_TEST_SUITE_END() // Equal_Pore_Volumes

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Variable_Pore_Volumes)

namespace {
    struct Setup : public CalculatorSetup
    {
        Setup()
            : CalculatorSetup { shoeBox({5, 5, 10}), centreProducer(10, 2, 6) }
        {
            using Span = std::remove_cv_t<
                std::remove_reference_t<decltype(this->blockSource[0])>>;
            using Item = typename Span::Item;

            const auto cellPress = pressureField();
            const auto poreVol = poreVolume();

            for (auto block = 0*cellPress.size(); block < cellPress.size(); ++block) {
                this->blockSource[this->wbpCells[block]]
                    .set(Item::Pressure, cellPress[block])
                    .set(Item::PoreVol, poreVol[block])
                    .set(Item::MixtureDensity, 0.1);
            }

            for (auto conn = 0*this->wbpConns.size(); conn < this->wbpConns.size(); ++conn) {
                this->connSource[conn]
                    .set(Item::Pressure, 1222.0)
                    .set(Item::PoreVol, 1.25)
                    .set(Item::MixtureDensity, 0.1);
            }
        }
    };
}

BOOST_AUTO_TEST_CASE(CentreOfFormation_OpenConns)
{
    // Producer connected in Z direction in cells (3,3,3), (3,3,4), (3,3,5),
    // (3,3,6), (3,3,7), and (3,3,8).  Connections (3,3,4) and (3,3,7) are
    // shut.
    Setup cse{};

    const auto controls = AveragingControls::Connections::open();
    const auto gravity  = standardGravity();
    const auto refDepth = 2005.0; // Bottom of layer 5.  Depth correction in all layers.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1257.977442500000, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1298.519109166667, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1278.248275833334, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1272.550359166667, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(BottomOfFormation_AllConns)
{
    // Producer connected in Z direction in cells (3,3,3), (3,3,4), (3,3,5),
    // (3,3,6), (3,3,7), and (3,3,8).  Connections (3,3,4) and (3,3,7) are
    // shut.
    Setup cse{};

    const auto controls = AveragingControls::Connections::all();
    const auto gravity  = standardGravity();
    const auto refDepth = 2010.0; // Bottom of layer 10.  Depth correction in all layers.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1.260397954166667e+03, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1.285259065277778e+03, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1.272828509722222e+03, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1.272113231944445e+03, 1.0e-8);
}

BOOST_AUTO_TEST_SUITE_END() // Variable_Pore_Volumes

BOOST_AUTO_TEST_SUITE_END() // Open_Shut

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Depth_Correction)

namespace {
    struct Setup : public CalculatorSetup
    {
        Setup()
            : CalculatorSetup { shoeBox({5, 5, 10}), centreProducer(10, 2, 6) }
        {
            using Span = std::remove_cv_t<
                std::remove_reference_t<decltype(this->blockSource[0])>>;
            using Item = typename Span::Item;

            const auto cellPress = pressureField();
            const auto poreVol = poreVolume();
            const auto mixDens = mixtureDensity();

            for (auto block = 0*cellPress.size(); block < cellPress.size(); ++block) {
                this->blockSource[this->wbpCells[block]]
                    .set(Item::Pressure, cellPress[block])
                    .set(Item::PoreVol, poreVol[block])
                    .set(Item::MixtureDensity, mixDens[block]);
            }

            const auto wellBoreDens = std::vector {
                0.1, 0.12, 0.14, 0.16, 0.18, 0.2,
            };

            for (auto conn = 0*this->wbpConns.size(); conn < this->wbpConns.size(); ++conn) {
                this->connSource[conn]
                    .set(Item::Pressure, 1222.0)
                    .set(Item::PoreVol, 1.25)
                    .set(Item::MixtureDensity, wellBoreDens[conn]);
            }
        }
    };
}

BOOST_AUTO_TEST_CASE(TopOfFormation_Well_OpenConns)
{
    // Producer connected in Z direction in cells (3,3,3), (3,3,4), (3,3,5),
    // (3,3,6), (3,3,7), and (3,3,8).  Connections (3,3,4) and (3,3,7) are
    // shut.
    Setup cse{};

    const auto controls = AveragingControls::DepthCorrection::well_open();
    const auto gravity  = standardGravity();
    const auto refDepth = 2002.5; // BHP reference depth.  Depth correction in layers 4..8.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1254.806625666667, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1295.348292333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1275.077459000000, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1269.379542333333, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(TopOfFormation_Well_AllConns)
{
    // Producer connected in Z direction in cells (3,3,3), (3,3,4), (3,3,5),
    // (3,3,6), (3,3,7), and (3,3,8).
    Setup cse{};

    const auto controls = AveragingControls::DepthCorrection::well_all();
    const auto gravity  = standardGravity();
    const auto refDepth = 2000.0; // Top of formation.  Depth correction in all layers.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1247.976197500000, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1272.837308611111, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1260.406753055556, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1259.691475277778, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(TopOfFormation_Reservoir_OpenConns)
{
    // Producer connected in Z direction in cells (3,3,3), (3,3,4), (3,3,5),
    // (3,3,6), (3,3,7), and (3,3,8).  Connections (3,3,4) and (3,3,7) are
    // shut.
    Setup cse{};

    const auto controls = AveragingControls::DepthCorrection::reservoir_open();
    const auto gravity  = standardGravity();
    const auto refDepth = 2000.0; // Top of formation.  Depth correction in all layers.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1251.379769151233, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1291.921435817900, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1271.650602484567, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1265.952685817900, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(BHPRefDepth_Reservoir_AllConns)
{
    // Producer connected in Z direction in cells (3,3,3), (3,3,4), (3,3,5),
    // (3,3,6), (3,3,7), and (3,3,8).
    Setup cse{};

    const auto controls = AveragingControls::DepthCorrection::reservoir_all();
    const auto gravity  = standardGravity();
    const auto refDepth = 2002.5; // BHP reference depth.  Depth correction in layers 4..8.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1251.972089001828, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1276.833200112939, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1264.402644557383, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1263.687366779606, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(TopOfFormation_None_OpenConns)
{
    // Producer connected in Z direction in cells (3,3,3), (3,3,4), (3,3,5),
    // (3,3,6), (3,3,7), and (3,3,8).  Connections (3,3,4) and (3,3,7) are
    // shut.
    Setup cse{};

    const auto controls = AveragingControls::DepthCorrection::none_open();
    const auto gravity  = standardGravity();
    const auto refDepth = 2000.0; // Top of formation.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1256.833333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1297.375000000000, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1277.104166666667, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1271.406250000000, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(SeaLevel_None_OpenConns)
{
    // Producer connected in Z direction in cells (3,3,3), (3,3,4), (3,3,5),
    // (3,3,6), (3,3,7), and (3,3,8).  Connections (3,3,4) and (3,3,7) are
    // shut.
    Setup cse{};

    const auto controls = AveragingControls::DepthCorrection::none_open();
    const auto gravity  = standardGravity();
    const auto refDepth = 0.0; // Sea level.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1256.833333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1297.375000000000, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1277.104166666667, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1271.406250000000, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(TopOfFormation_None_AllConns)
{
    // Producer connected in Z direction in cells (3,3,3), (3,3,4), (3,3,5),
    // (3,3,6), (3,3,7), and (3,3,8).
    Setup cse{};

    const auto controls = AveragingControls::DepthCorrection::none_all();
    const auto gravity  = standardGravity();
    const auto refDepth = 2000.0; // Top of formation.  Depth correction in all layers.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1255.222222222222, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1280.083333333333, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1267.652777777778, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1266.937500000000, 1.0e-8);
}

BOOST_AUTO_TEST_SUITE_END() // Depth_Correction

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Integration)

namespace {
    struct Setup : public CalculatorSetup
    {
        Setup()
            : CalculatorSetup { shoeBox({5, 5, 10}), centreProducer(10, 2, 6) }
        {
            using Span = std::remove_cv_t<
                std::remove_reference_t<decltype(this->blockSource[0])>>;
            using Item = typename Span::Item;

            const auto cellPress = pressureField();
            const auto poreVol = poreVolume();
            const auto mixDens = mixtureDensity();

            for (auto block = 0*cellPress.size(); block < cellPress.size(); ++block) {
                this->blockSource[this->wbpCells[block]]
                    .set(Item::Pressure, cellPress[block])
                    .set(Item::PoreVol, poreVol[block])
                    .set(Item::MixtureDensity, mixDens[block]);
            }

            const auto wellBoreDens = std::vector {
                0.1, 0.12, 0.14, 0.16, 0.18, 0.2,
            };

            for (auto conn = 0*this->wbpConns.size(); conn < this->wbpConns.size(); ++conn) {
                this->connSource[conn]
                    .set(Item::Pressure, 1222.0)
                    .set(Item::PoreVol, 1.25)
                    .set(Item::MixtureDensity, wellBoreDens[conn]);
            }
        }
    };
}

BOOST_AUTO_TEST_CASE(All_Specified)
{
    // Producer connected in Z direction in cells (3,3,3), (3,3,4), (3,3,5),
    // (3,3,6), (3,3,7), and (3,3,8).  Connections (3,3,4) and (3,3,7) are
    // shut.
    Setup cse{};

    const auto F1 = 0.875;
    const auto F2 = 0.123;
    const auto depthCorr = Opm::PAvg::DepthCorrection::RES;
    const auto useOpen = false;

    const auto controls = Opm::PAvg { F1, F2, depthCorr, useOpen };
    const auto gravity  = standardGravity();
    const auto refDepth = 2001.0; // Top cell bottom.  Depth correction in all layers.

    cse.calc.inferBlockAveragePressures(cse.sources, controls, gravity, refDepth);

    const auto avgPress = cse.calc.averagePressures();
    using WBPMode = Opm::PAvgCalculator::Result::WBPMode;

    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP) , 1270.833785766429, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP4), 1273.997383589501, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP5), 1271.300397582907, 1.0e-8);
    BOOST_CHECK_CLOSE(avgPress.value(WBPMode::WBP9), 1271.175182912842, 1.0e-8);
}

BOOST_AUTO_TEST_SUITE_END() // Integration
