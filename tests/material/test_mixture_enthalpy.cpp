// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2026 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
/*!
 * \file
 *
 * \brief Tests for the compositional caloric data.
 *
 * CaloricData pins the ideal-gas heat-capacity polynomials on the component
 * classes against external reference values.
 */
#include "config.h"

#define BOOST_TEST_MODULE MixtureEnthalpy
#include <boost/test/unit_test.hpp>

#include <opm/material/components/C1.hpp>
#include <opm/material/components/C10.hpp>
#include <opm/material/components/ComponentCp.hpp>
#include <opm/material/components/SimpleCO2.hpp>

#include <array>
#include <cstddef>

using Scalar = double;

namespace {

const Scalar T0 = Opm::ComponentCp<Scalar>::referenceTemperature();

} // anonymous namespace

BOOST_AUTO_TEST_SUITE(CaloricData)

// The caloric equations (ComponentCp), species-blind: enthalpy is the exact
// integral of the heat-capacity polynomial, zero at the reference datum.
BOOST_AUTO_TEST_CASE(EnthalpyIntegralMatchesHeatCapacity)
{
    const Opm::ComponentCp<double> cp = Opm::C1<double>::idealGasHeatCapacityPolynomial();

    // h(T0) = 0 by construction
    BOOST_CHECK_EQUAL(cp.enthalpyIntegral(T0), 0.0);

    // dh/dT == cp(T), checked against a central finite difference
    const double T = 400.;
    const double dT = 1e-3;
    const double dhdT = (cp.enthalpyIntegral(T + dT) - cp.enthalpyIntegral(T - dT)) / (2. * dT);
    BOOST_CHECK_CLOSE(dhdT, cp.heatCapacity(T), 1e-6); // [%]
}

// The cp polynomials against external reference values, the one check that
// internal consistency cannot replace: a self-consistent test derives its
// expectation from the same coefficients it verifies, so a corrupt row
// passes it. The pinned values are the ideal-gas heat capacities of the
// reference equations of state (Setzmann & Wagner 1991 methane; Lemmon &
// Span 2006 n-decane; Span & Wagner 1996 CO2) at four temperatures spanning
// the fit range.
BOOST_AUTO_TEST_CASE(CardsMatchReferenceIdealGasCp)
{
    constexpr std::array<double, 4> temps{298.15, 350., 450., 600.};
    const struct {
        const char* name;
        Opm::ComponentCp<double> card;
        std::array<double, 4> cpRef; // [J/(mol K)] at temps[]
    } pins[] = {
        {"methane", Opm::C1<double>::idealGasHeatCapacityPolynomial(),
         {35.7085, 37.9628, 43.5052, 52.4919}},
        {"decane", Opm::C10<double>::idealGasHeatCapacityPolynomial(),
         {233.0250, 266.2081, 328.2699, 405.7329}},
        {"carbonDioxide", Opm::SimpleCO2<double>::idealGasHeatCapacityPolynomial(),
         {37.1408, 39.3941, 43.0700, 47.3303}},
    };

    for (const auto& pin : pins) {
        for (std::size_t i = 0; i < temps.size(); ++i) {
            BOOST_TEST_CONTEXT(pin.name << " at T = " << temps[i]) {
                // 1% relative: an order looser than the fit residuals (max
                // 0.3%), an order tighter than the defect class it guards
                BOOST_CHECK_CLOSE(pin.card.heatCapacity(temps[i]),
                                  pin.cpRef[i], 1.0); // [%]
            }
        }
    }
}

// Each polynomial carries the temperature range of its fit. Inside it the
// heat capacity is positive and increasing; outside it the cubics turn over.
BOOST_AUTO_TEST_CASE(CardsCarryTheirFitRange)
{
    const struct {
        const char* name;
        Opm::ComponentCp<double> card;
    } cards[] = {
        {"methane", Opm::C1<double>::idealGasHeatCapacityPolynomial()},
        {"decane", Opm::C10<double>::idealGasHeatCapacityPolynomial()},
        {"carbonDioxide", Opm::SimpleCO2<double>::idealGasHeatCapacityPolynomial()},
    };

    for (const auto& c : cards) {
        BOOST_TEST_CONTEXT(c.name) {
            BOOST_CHECK_EQUAL(c.card.minTemperature, 250.);
            BOOST_CHECK_EQUAL(c.card.maxTemperature, 600.);

            double previous = c.card.heatCapacity(c.card.minTemperature);
            BOOST_CHECK_GT(previous, 0.);
            for (double T = c.card.minTemperature + 10.; T <= c.card.maxTemperature; T += 10.) {
                const double current = c.card.heatCapacity(T);
                BOOST_CHECK_MESSAGE(current > previous,
                                    "cp not increasing at T = " << T);
                previous = current;
            }
        }
    }
}

BOOST_AUTO_TEST_SUITE_END() // CaloricData
