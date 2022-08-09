/*
  Copyright 2022 Equinor ASA

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

#define BOOST_TEST_MODULE test_rst_netbalan

#include <boost/test/unit_test.hpp>

#include <opm/io/eclipse/rst/netbalan.hpp>

#include <opm/output/eclipse/InteHEAD.hpp>
#include <opm/output/eclipse/DoubHEAD.hpp>

#include <opm/input/eclipse/Schedule/Network/Balance.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>
#include <opm/input/eclipse/Units/Units.hpp>

#include <cstddef>
#include <utility>
#include <vector>

using NBDims   = Opm::RestartIO::InteHEAD::NetBalanceDims;
using NBParams = Opm::RestartIO::DoubHEAD::NetBalanceParams;

namespace {
    Opm::RestartIO::RstNetbalan restart(const NBDims& dims, const NBParams& params)
    {
        const auto intehead = Opm::RestartIO::InteHEAD{}.netBalanceData(dims).data();
        const auto doubhead = Opm::RestartIO::DoubHEAD{}.netBalParams(params).data();

        return Opm::RestartIO::RstNetbalan {
            intehead, doubhead, Opm::UnitSystem::newMETRIC()
        };
    }

    NBDims defaultDims()
    {
        return { 0, 10 };
    }

    NBParams defaultParams()
    {
        return NBParams { Opm::UnitSystem::newMETRIC() };
    }

    NBParams norneParams()
    {
        // Using
        //
        // NETBALAN
        //    0.0  0.2  6* /
        //
        // as defined in the NORNE_ATW2013.DATA simulation model (opm-tests).

        auto params = NBParams { Opm::UnitSystem::newMETRIC() };

        params.convTolNodPres = 0.2; // (barsa)

        return params;
    }

    NBDims iotaDims()
    {
        // Using synthetic
        //
        // NETBALAN
        //    1.0 2.0 3 4.0 5 6.0 7.0 8.0 /

        return { 3, 5 };
    }

    NBParams iotaParams()
    {
        // Using synthetic
        //
        // NETBALAN
        //    1.0 2.0 3 4.0 5 6.0 7.0 8.0 /

        auto params = NBParams { Opm::UnitSystem::newMETRIC() };

        params.balancingInterval = 1.0;
        params.convTolNodPres = 2.0;
        params.convTolTHPCalc = 4.0;
        params.targBranchBalError = 6.0;
        params.maxBranchBalError = 7.0;
        params.minTimeStepSize = 8.0;

        return params;
    }

    NBParams nupcolParams()
    {
        auto params = NBParams { Opm::UnitSystem::newMETRIC() };

        params.balancingInterval = -1.0;

        return params;
    }
} // namespace

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Restart)

BOOST_AUTO_TEST_CASE(No_Active_Network)
{
    const auto netbalan = restart(defaultDims(), defaultParams());

    BOOST_CHECK_CLOSE(netbalan.interval(), 0.0, 1.0e-7);
    BOOST_CHECK_CLOSE(netbalan.pressureTolerance(), 0.0*Opm::unit::barsa, 1.0e-7);
    BOOST_CHECK_EQUAL(netbalan.pressureMaxIter(), std::size_t{0});
    BOOST_CHECK_CLOSE(netbalan.thpTolerance(), 0.01*Opm::unit::barsa, 1.0e-7);
    BOOST_CHECK_EQUAL(netbalan.thpMaxIter(), std::size_t{10});

    BOOST_CHECK_MESSAGE(! netbalan.targetBalanceError().has_value(),
                        "Inactive network must not have branch "
                        "target balance error at restart");

    BOOST_CHECK_MESSAGE(! netbalan.maxBalanceError().has_value(),
                        "Inactive network must not have maximum "
                        "balance error tolerance at restart");

    BOOST_CHECK_MESSAGE(! netbalan.minTstep().has_value(),
                        "Inactive network must not have a minimum "
                        "timestep value at restart");
}

BOOST_AUTO_TEST_CASE(Norne)
{
    // Using
    //
    // NETBALAN
    //    0.0  0.2  6* /
    //
    // as defined in the NORNE_ATW2013.DATA simulation model (opm-tests)

    const auto netbalan = restart(defaultDims(), norneParams());

    BOOST_CHECK_CLOSE(netbalan.interval(), 0.0, 1.0e-7);
    BOOST_CHECK_CLOSE(netbalan.pressureTolerance(), 0.2*Opm::unit::barsa, 1.0e-7);
    BOOST_CHECK_EQUAL(netbalan.pressureMaxIter(), std::size_t{0});
    BOOST_CHECK_CLOSE(netbalan.thpTolerance(), 0.01*Opm::unit::barsa, 1.0e-7);
    BOOST_CHECK_EQUAL(netbalan.thpMaxIter(), std::size_t{10});

    BOOST_CHECK_MESSAGE(! netbalan.targetBalanceError().has_value(),
                        "Norne network must not have branch "
                        "target balance error at restart");

    BOOST_CHECK_MESSAGE(! netbalan.maxBalanceError().has_value(),
                        "Norne network must not have maximum "
                        "balance error tolerance at restart");

    BOOST_CHECK_MESSAGE(! netbalan.minTstep().has_value(),
                        "Norne network must not have a minimum "
                        "timestep value at restart");
}

BOOST_AUTO_TEST_CASE(Iota)
{
    const auto netbalan = restart(iotaDims(), iotaParams());

    BOOST_CHECK_CLOSE(netbalan.interval(), 1.0*Opm::unit::day, 1.0e-7);
    BOOST_CHECK_CLOSE(netbalan.pressureTolerance(), 2.0*Opm::unit::barsa, 1.0e-7);
    BOOST_CHECK_EQUAL(netbalan.pressureMaxIter(), std::size_t{3});
    BOOST_CHECK_CLOSE(netbalan.thpTolerance(), 4.0*Opm::unit::barsa, 1.0e-7);
    BOOST_CHECK_EQUAL(netbalan.thpMaxIter(), std::size_t{5});

    BOOST_CHECK_MESSAGE(netbalan.targetBalanceError().has_value(),
                        "IOTA network must have branch "
                        "target balance error at restart");

    BOOST_CHECK_CLOSE(netbalan.targetBalanceError().value(), 6.0*Opm::unit::barsa, 1.0e-7);

    BOOST_CHECK_MESSAGE(netbalan.maxBalanceError().has_value(),
                        "IOTA network must have maximum "
                        "balance error tolerance at restart");

    BOOST_CHECK_CLOSE(netbalan.maxBalanceError().value(), 7.0*Opm::unit::barsa, 1.0e-7);

    BOOST_CHECK_MESSAGE(netbalan.minTstep().has_value(),
                        "IOTA network must have a minimum "
                        "timestep value at restart");

    BOOST_CHECK_CLOSE(netbalan.minTstep().value(), 8.0*Opm::unit::day, 1.0e-7);
}

BOOST_AUTO_TEST_CASE(Nupcol)
{
    const auto netbalan = restart(defaultDims(), nupcolParams());

    BOOST_CHECK_CLOSE(netbalan.interval(), -1.0*Opm::unit::day, 1.0e-7);
}

BOOST_AUTO_TEST_SUITE_END() // Restart

// ---------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(Balance_Object)

BOOST_AUTO_TEST_CASE(No_Active_Network)
{
    const auto netbalan = Opm::Network::Balance {
        restart(defaultDims(), defaultParams())
    };

    BOOST_CHECK_MESSAGE(netbalan.mode() == Opm::Network::Balance::CalcMode::TimeStepStart,
                        "Inactive network must have \"TimeStepStart\" "
                        "NETBALAN calculation mode");

    BOOST_CHECK_CLOSE(netbalan.interval(), 0.0, 1.0e-7);
    BOOST_CHECK_CLOSE(netbalan.pressure_tolerance(), 0.0, 1.0e-7);
    BOOST_CHECK_EQUAL(netbalan.pressure_max_iter(), std::size_t{0});
    BOOST_CHECK_CLOSE(netbalan.thp_tolerance(), 0.01*Opm::unit::barsa, 1.0e-7);
    BOOST_CHECK_EQUAL(netbalan.thp_max_iter(), std::size_t{10});

    BOOST_CHECK_MESSAGE(! netbalan.target_balance_error().has_value(),
                        "Inactive network must not have branch "
                        "target balance error at restart");

    BOOST_CHECK_MESSAGE(! netbalan.max_balance_error().has_value(),
                        "Inactive network must not have maximum "
                        "balance error tolerance at restart");

    BOOST_CHECK_MESSAGE(! netbalan.min_tstep().has_value(),
                        "Inactive network must not have a minimum "
                        "timestep value at restart");
}

BOOST_AUTO_TEST_CASE(Norne)
{
    const auto netbalan = Opm::Network::Balance {
        restart(defaultDims(), norneParams())
    };

    BOOST_CHECK_MESSAGE(netbalan.mode() == Opm::Network::Balance::CalcMode::TimeStepStart,
                        "Norne network must have \"TimeStepStart\" "
                        "NETBALAN calculation mode");

    BOOST_CHECK_CLOSE(netbalan.interval(), 0.0, 1.0e-7);
    BOOST_CHECK_CLOSE(netbalan.pressure_tolerance(), 0.2*Opm::unit::barsa, 1.0e-7);
    BOOST_CHECK_EQUAL(netbalan.pressure_max_iter(), std::size_t{0});
    BOOST_CHECK_CLOSE(netbalan.thp_tolerance(), 0.01*Opm::unit::barsa, 1.0e-7);
    BOOST_CHECK_EQUAL(netbalan.thp_max_iter(), std::size_t{10});

    BOOST_CHECK_MESSAGE(! netbalan.target_balance_error().has_value(),
                        "Norne network must not have branch "
                        "target balance error at restart");

    BOOST_CHECK_MESSAGE(! netbalan.max_balance_error().has_value(),
                        "Norne network must not have maximum "
                        "balance error tolerance at restart");

    BOOST_CHECK_MESSAGE(! netbalan.min_tstep().has_value(),
                        "Norne network must not have a minimum "
                        "timestep value at restart");
}

BOOST_AUTO_TEST_CASE(Iota)
{
    const auto netbalan = Opm::Network::Balance {
        restart(iotaDims(), iotaParams())
    };

    BOOST_CHECK_MESSAGE(netbalan.mode() == Opm::Network::Balance::CalcMode::TimeInterval,
                        "IOTA network must have \"TimeInterval\" "
                        "NETBALAN calculation mode");

    BOOST_CHECK_CLOSE(netbalan.interval(), 1.0*Opm::unit::day, 1.0e-7);
    BOOST_CHECK_CLOSE(netbalan.pressure_tolerance(), 2.0*Opm::unit::barsa, 1.0e-7);
    BOOST_CHECK_EQUAL(netbalan.pressure_max_iter(), std::size_t{3});
    BOOST_CHECK_CLOSE(netbalan.thp_tolerance(), 4.0*Opm::unit::barsa, 1.0e-7);
    BOOST_CHECK_EQUAL(netbalan.thp_max_iter(), std::size_t{5});

    BOOST_CHECK_MESSAGE(netbalan.target_balance_error().has_value(),
                        "IOTA network must have branch "
                        "target balance error at restart");

    BOOST_CHECK_CLOSE(netbalan.target_balance_error().value(), 6.0*Opm::unit::barsa, 1.0e-7);

    BOOST_CHECK_MESSAGE(netbalan.max_balance_error().has_value(),
                        "IOTA network must have maximum "
                        "balance error tolerance at restart");

    BOOST_CHECK_CLOSE(netbalan.max_balance_error().value(), 7.0*Opm::unit::barsa, 1.0e-7);

    BOOST_CHECK_MESSAGE(netbalan.min_tstep().has_value(),
                        "IOTA network must have a minimum "
                        "timestep value at restart");

    BOOST_CHECK_CLOSE(netbalan.min_tstep().value(), 8.0*Opm::unit::day, 1.0e-7);
}

BOOST_AUTO_TEST_CASE(Nupcol)
{
    const auto netbalan = Opm::Network::Balance {
        restart(defaultDims(), nupcolParams())
    };

    BOOST_CHECK_MESSAGE(netbalan.mode() == Opm::Network::Balance::CalcMode::NUPCOL,
                        "NUPCOL network must have \"NUPCOL\" "
                        "NETBALAN calculation mode");
}

BOOST_AUTO_TEST_SUITE_END() // Balance_Object
