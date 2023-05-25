// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
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
 * \brief This is a program to test the tabulation class for of
 *        individual components.
 *
 */
#include "config.h"

#define BOOST_TEST_MODULE Tabulation
#include <boost/test/unit_test.hpp>

#include <opm/material/components/H2O.hpp>
#include <opm/material/components/TabulatedComponent.hpp>

#include <iostream>
#include <tuple>

using Types = std::tuple<float,double>;

BOOST_AUTO_TEST_CASE_TEMPLATE(H2O, Scalar, Types)
{
    using IapwsH2O = Opm::H2O<Scalar>;
    using TabulatedH2O = Opm::TabulatedComponent<Scalar, IapwsH2O>;

    Scalar tempMin = 274.15;
    Scalar tempMax = 622.15;
    unsigned nTemp = static_cast<unsigned>(tempMax - tempMin) * 6/8;

    Scalar pMin = 10.00;
    Scalar pMax = IapwsH2O::vaporPressure(tempMax*1.1);
    unsigned nPress = 50;

    std::cout << "Creating tabulation with " << nTemp*nPress << " entries per quantity\n";
    TabulatedH2O::init(tempMin, tempMax, nTemp,
                       pMin, pMax, nPress);

    std::cout << "Checking tabulation\n";
    unsigned m = nTemp*3;
    unsigned n = nPress*3;
    for (unsigned i = 0; i < m; ++i) {
        Scalar T = tempMin + (tempMax - tempMin)*Scalar(i)/m;

        if (i % std::max<unsigned>(1, m/1000) == 0) {
            std::cout << Scalar(i)/m*100 << "% done        \r";
            std::cout.flush();
        }

        BOOST_CHECK_CLOSE_FRACTION(TabulatedH2O::vaporPressure(T),
                                   IapwsH2O::vaporPressure(T),
                                   Scalar(1e-3));

        for (unsigned j = 0; j < n; ++j) {
            Scalar p = pMin + (pMax - pMin)*Scalar(j)/n;
            if (p < IapwsH2O::vaporPressure(T) * 1.001) {
                Scalar tol;
                if constexpr (std::is_same_v<Scalar,double>) {
                    tol = 4e-3;
                    if (p > IapwsH2O::vaporPressure(T))
                        tol = 1e-2;
                } else {
                    tol = 1.62e-2;
                    if (p > IapwsH2O::vaporPressure(T))
                        tol = 1.8e-2;
                }

                Scalar rho = IapwsH2O::gasDensity(T,p);
                BOOST_CHECK_CLOSE_FRACTION(TabulatedH2O::gasEnthalpy(T,p), IapwsH2O::gasEnthalpy(T,p), tol);
                BOOST_CHECK_CLOSE_FRACTION(TabulatedH2O::gasInternalEnergy(T,p), IapwsH2O::gasInternalEnergy(T,p), tol);
                BOOST_CHECK_CLOSE_FRACTION(TabulatedH2O::gasDensity(T,p), rho, tol);
                BOOST_CHECK_CLOSE_FRACTION(TabulatedH2O::gasViscosity(T,p), IapwsH2O::gasViscosity(T,p), tol);
            }

            if (p > IapwsH2O::vaporPressure(T) / 1.001) {
                Scalar tol = 1e-3;
                if (p < IapwsH2O::vaporPressure(T))
                    tol = 1e-2;
                Scalar rho = IapwsH2O::liquidDensity(T,p);
                BOOST_CHECK_CLOSE_FRACTION(TabulatedH2O::liquidEnthalpy(T,p), IapwsH2O::liquidEnthalpy(T,p), tol);
                BOOST_CHECK_CLOSE_FRACTION(TabulatedH2O::liquidInternalEnergy(T,p), IapwsH2O::liquidInternalEnergy(T,p), tol);
                BOOST_CHECK_CLOSE_FRACTION(TabulatedH2O::liquidDensity(T,p), rho, tol);
                BOOST_CHECK_CLOSE_FRACTION(TabulatedH2O::liquidViscosity(T,p), IapwsH2O::liquidViscosity(T,p), tol);
            }
        }
    }
}
