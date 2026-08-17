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
 * \brief Tests for the compositional caloric data and enthalpy machinery.
 *
 * This file starts with the per-component caloric identity cards (the
 * ideal-gas heat-capacity polynomials on the component classes) pinned
 * against external reference values; the mixture-enthalpy model suites
 * land as siblings in this file.
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

BOOST_AUTO_TEST_SUITE(CaloricData)

// The caloric equations (ComponentCp), species-blind: enthalpy is the exact
// integral of the heat-capacity polynomial, zero at the reference datum.
BOOST_AUTO_TEST_CASE(EnthalpyIntegralMatchesHeatCapacity)
{
    const Opm::ComponentCp<double> cp = Opm::C1<double>::idealGasHeatCapacityPolynomial();
    const double T0 = 298.15;

    // h(T0) = 0 by construction
    BOOST_CHECK_EQUAL(cp.enthalpyIntegral(T0, T0), 0.0);

    // dh/dT == cp(T), checked against a central finite difference
    const double T = 400.;
    const double dT = 1e-3;
    const double dhdT = (cp.enthalpyIntegral(T + dT, T0) - cp.enthalpyIntegral(T - dT, T0)) / (2. * dT);
    BOOST_CHECK_CLOSE(dhdT, cp.heatCapacity(T), 1e-6); // [%]
}

// The identity cards against EXTERNAL reference values — the one check no
// amount of internal consistency can substitute: a self-consistent test
// manufactures its expectation from the same coefficients it verifies, so a
// corrupt row passes it. The pinned values are the reference-EoS ideal-gas
// heat capacities (Setzmann & Wagner 1991 methane; Lemmon & Span 2006
// n-decane; Span & Wagner 1996 CO2), tabulated at four temperatures spanning
// the fit window.
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

// The triple points added on the identity cards, against the reference-EoS
// fluid values (papers cited on the classes).
BOOST_AUTO_TEST_CASE(TriplePointsMatchReferenceFluids)
{
    BOOST_CHECK_CLOSE(Opm::C1<double>::tripleTemperature(), 90.6941, 1e-6);
    BOOST_CHECK_CLOSE(Opm::C1<double>::triplePressure(), 11696.06, 0.01);
    BOOST_CHECK_CLOSE(Opm::C10<double>::tripleTemperature(), 243.5, 1e-6);
    BOOST_CHECK_CLOSE(Opm::C10<double>::triplePressure(), 1.4042, 0.02);
}

BOOST_AUTO_TEST_SUITE_END() // CaloricData
