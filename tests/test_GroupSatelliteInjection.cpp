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

#define BOOST_TEST_MODULE Group_Satellite_Injection

#include <boost/test/unit_test.hpp>

#include <opm/input/eclipse/Schedule/Group/GroupSatelliteInjection.hpp>

#include <opm/input/eclipse/EclipseState/Phase.hpp>

BOOST_AUTO_TEST_SUITE(Rate_Object)

BOOST_AUTO_TEST_CASE(Construction)
{
    const auto r = Opm::GroupSatelliteInjection::Rate{};

    BOOST_CHECK_MESSAGE(! r.surface().has_value(),
                        "Default Rate object must NOT have surface injection rate");

    BOOST_CHECK_MESSAGE(! r.reservoir().has_value(),
                        "Default Rate object must NOT have reservoir injection rate");

    BOOST_CHECK_MESSAGE(! r.calorific().has_value(),
                        "Default Rate object must NOT have mean calorific value");
}

BOOST_AUTO_TEST_CASE(Surface_Rate)
{
    const auto r = Opm::GroupSatelliteInjection::Rate{}.surface(12.34);

    BOOST_REQUIRE_MESSAGE(r.surface().has_value(),
                          "Rate object must have surface injection rate");

    BOOST_CHECK_CLOSE(*r.surface(), 12.34, 1.0e-8);

    BOOST_CHECK_MESSAGE(! r.reservoir().has_value(),
                        "Rate object must NOT have reservoir injection rate");

    BOOST_CHECK_MESSAGE(! r.calorific().has_value(),
                        "Rate object must NOT have mean calorific value");
}

BOOST_AUTO_TEST_CASE(Reservoir_Rate)
{
    const auto r = Opm::GroupSatelliteInjection::Rate{}.reservoir(12.34);

    BOOST_CHECK_MESSAGE(! r.surface().has_value(),
                        "Rate object must NOT have surface injection rate");

    BOOST_REQUIRE_MESSAGE(r.reservoir().has_value(),
                          "Rate object must have reservoir injection rate");

    BOOST_CHECK_CLOSE(*r.reservoir(), 12.34, 1.0e-8);

    BOOST_CHECK_MESSAGE(! r.calorific().has_value(),
                        "Rate object must NOT have mean calorific value");
}

BOOST_AUTO_TEST_CASE(Mean_Calorific)
{
    const auto r = Opm::GroupSatelliteInjection::Rate{}.calorific(12.34);

    BOOST_CHECK_MESSAGE(! r.surface().has_value(),
                        "Rate object must NOT have surface injection rate");

    BOOST_CHECK_MESSAGE(! r.reservoir().has_value(),
                        "Rate object must NOT have mean reservoir injection rate");

    BOOST_REQUIRE_MESSAGE(r.calorific().has_value(),
                          "Rate object must have mean calorific value");

    BOOST_CHECK_CLOSE(*r.calorific(), 12.34, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(All)
{
    const auto r = Opm::GroupSatelliteInjection::Rate{}
        .surface(12.34).reservoir(567.8).calorific(9.1011);

    BOOST_REQUIRE_MESSAGE(r.surface().has_value(),
                          "Rate object must have surface injection rate");

    BOOST_CHECK_CLOSE(*r.surface(), 12.34, 1.0e-8);

    BOOST_REQUIRE_MESSAGE(r.reservoir().has_value(),
                          "Rate object must have mean reservoir injection rate");

    BOOST_CHECK_CLOSE(*r.reservoir(), 567.8, 1.0e-8);

    BOOST_REQUIRE_MESSAGE(r.calorific().has_value(),
                          "Rate object must have mean calorific value");

    BOOST_CHECK_CLOSE(*r.calorific(), 9.1011, 1.0e-8);
}

BOOST_AUTO_TEST_SUITE_END()     // Rate_Object

// ===========================================================================

BOOST_AUTO_TEST_SUITE(Injection_Object)

namespace {
    Opm::GroupSatelliteInjection::Rate gas()
    {
        return Opm::GroupSatelliteInjection::Rate{}
            .surface  (12345.6)
            .reservoir(78910.11)
            .calorific(1.21314);
    }

    Opm::GroupSatelliteInjection::Rate water()
    {
        // Note: .surface() rate and .calorific() values intentionally
        // omitted.
        return Opm::GroupSatelliteInjection::Rate{}
            .reservoir(789.1011);
    }
} // Anonymous namespace

BOOST_AUTO_TEST_CASE(Construction)
{
    const auto i = Opm::GroupSatelliteInjection { "SAT" };

    BOOST_CHECK_EQUAL(i.name(), "SAT");

    BOOST_CHECK_MESSAGE(! i.rateIndex(Opm::Phase::GAS).has_value(),
                        "Default object must NOT have injection rates for GAS");

    BOOST_CHECK_MESSAGE(! i.rateIndex(Opm::Phase::WATER).has_value(),
                        "Default object must NOT have injection rates for WATER");

    BOOST_CHECK_MESSAGE(! i.rateIndex(Opm::Phase::OIL).has_value(),
                        "Default object must NOT have injection rates for OIL");
}

BOOST_AUTO_TEST_CASE(Gas)
{
    auto i = Opm::GroupSatelliteInjection { "SAT" };

    i.rate(Opm::Phase::GAS) = gas();

    BOOST_CHECK_MESSAGE(! i.rateIndex(Opm::Phase::WATER).has_value(),
                        "Injection object must NOT have injection rates for WATER");

    BOOST_CHECK_MESSAGE(! i.rateIndex(Opm::Phase::OIL).has_value(),
                        "Injection object must NOT have injection rates for OIL");

    BOOST_REQUIRE_MESSAGE(i.rateIndex(Opm::Phase::GAS).has_value(),
                          "Injection object must have injection rates for GAS");

    const auto& rate = i[ *i.rateIndex(Opm::Phase::GAS) ];

    BOOST_REQUIRE_MESSAGE(rate.surface().has_value(),
                          "Rate object for GAS must have surface injection rates");

    BOOST_CHECK_CLOSE(*rate.surface(), 12345.6, 1.0e-8);

    BOOST_REQUIRE_MESSAGE(rate.reservoir().has_value(),
                          "Rate object for GAS must have reservoir injection rates");

    BOOST_CHECK_CLOSE(*rate.reservoir(), 78910.11, 1.0e-8);

    BOOST_REQUIRE_MESSAGE(rate.calorific().has_value(),
                          "Rate object for GAS must have mean calorific value");

    BOOST_CHECK_CLOSE(*rate.calorific(), 1.21314, 1.0e-8);
}

BOOST_AUTO_TEST_CASE(Water)
{
    auto i = Opm::GroupSatelliteInjection { "SAT" };

    i.rate(Opm::Phase::WATER) = water();

    BOOST_REQUIRE_MESSAGE(i.rateIndex(Opm::Phase::WATER).has_value(),
                          "Injection object must have injection rates for WATER");

    BOOST_CHECK_MESSAGE(! i.rateIndex(Opm::Phase::GAS).has_value(),
                        "Injection object must NOT have injection rates for GAS");

    BOOST_CHECK_MESSAGE(! i.rateIndex(Opm::Phase::OIL).has_value(),
                        "Injection object must NOT have injection rates for OIL");

    const auto& rate = i[ *i.rateIndex(Opm::Phase::WATER) ];

    BOOST_CHECK_MESSAGE(! rate.surface().has_value(),
                        "Rate object for WATER must NOT have surface injection rates");

    BOOST_REQUIRE_MESSAGE(rate.reservoir().has_value(),
                          "Rate object for WATER must have reservoir injection rates");

    BOOST_CHECK_CLOSE(*rate.reservoir(), 789.1011, 1.0e-8);

    BOOST_CHECK_MESSAGE(! rate.calorific().has_value(),
                        "Rate object for WATER must NOT have mean calorific value");
}

BOOST_AUTO_TEST_CASE(Gas_Water_In_Order)
{
    auto i = Opm::GroupSatelliteInjection { "SAT" };

    i.rate(Opm::Phase::GAS) = gas();
    i.rate(Opm::Phase::WATER) = water();

    BOOST_REQUIRE_MESSAGE(i.rateIndex(Opm::Phase::GAS).has_value(),
                          "Injection object must have injection rates for GAS");

    BOOST_REQUIRE_MESSAGE(i.rateIndex(Opm::Phase::WATER).has_value(),
                          "Injection object must have injection rates for WATER");

    {
        const auto& qg = i[ *i.rateIndex(Opm::Phase::GAS) ];

        BOOST_REQUIRE_MESSAGE(qg.surface().has_value(),
                              "Rate object for GAS must have surface injection rates");

        BOOST_CHECK_CLOSE(*qg.surface(), 12345.6, 1.0e-8);

        BOOST_REQUIRE_MESSAGE(qg.reservoir().has_value(),
                              "Rate object for GAS must have reservoir injection rates");

        BOOST_CHECK_CLOSE(*qg.reservoir(), 78910.11, 1.0e-8);

        BOOST_REQUIRE_MESSAGE(qg.calorific().has_value(),
                              "Rate object for GAS must have mean calorific value");

        BOOST_CHECK_CLOSE(*qg.calorific(), 1.21314, 1.0e-8);
    }

    {
        const auto& qw = i[ *i.rateIndex(Opm::Phase::WATER) ];

        BOOST_CHECK_MESSAGE(! qw.surface().has_value(),
                            "Rate object for WATER must NOT have surface injection rates");

        BOOST_REQUIRE_MESSAGE(qw.reservoir().has_value(),
                              "Rate object for WATER must have reservoir injection rates");

        BOOST_CHECK_CLOSE(*qw.reservoir(), 789.1011, 1.0e-8);

        BOOST_CHECK_MESSAGE(! qw.calorific().has_value(),
                            "Rate object for WATER must NOT have mean calorific value");
    }
}

BOOST_AUTO_TEST_CASE(Gas_Water_Reverse_Order)
{
    auto i = Opm::GroupSatelliteInjection { "SAT" };

    i.rate(Opm::Phase::GAS) = gas();
    i.rate(Opm::Phase::WATER) = water();

    BOOST_REQUIRE_MESSAGE(i.rateIndex(Opm::Phase::GAS).has_value(),
                          "Injection object must have injection rates for GAS");

    BOOST_REQUIRE_MESSAGE(i.rateIndex(Opm::Phase::WATER).has_value(),
                          "Injection object must have injection rates for WATER");

    {
        const auto& qw = i[ *i.rateIndex(Opm::Phase::WATER) ];

        BOOST_CHECK_MESSAGE(! qw.surface().has_value(),
                            "Rate object for WATER must NOT have surface injection rates");

        BOOST_REQUIRE_MESSAGE(qw.reservoir().has_value(),
                              "Rate object for WATER must have reservoir injection rates");

        BOOST_CHECK_CLOSE(*qw.reservoir(), 789.1011, 1.0e-8);

        BOOST_CHECK_MESSAGE(! qw.calorific().has_value(),
                            "Rate object for WATER must NOT have mean calorific value");
    }

    {
        const auto& qg = i[ *i.rateIndex(Opm::Phase::GAS) ];

        BOOST_REQUIRE_MESSAGE(qg.surface().has_value(),
                              "Rate object for GAS must have surface injection rates");

        BOOST_CHECK_CLOSE(*qg.surface(), 12345.6, 1.0e-8);

        BOOST_REQUIRE_MESSAGE(qg.reservoir().has_value(),
                              "Rate object for GAS must have reservoir injection rates");

        BOOST_CHECK_CLOSE(*qg.reservoir(), 78910.11, 1.0e-8);

        BOOST_REQUIRE_MESSAGE(qg.calorific().has_value(),
                              "Rate object for GAS must have mean calorific value");

        BOOST_CHECK_CLOSE(*qg.calorific(), 1.21314, 1.0e-8);
    }
}

BOOST_AUTO_TEST_SUITE_END()     // Injection_Object
