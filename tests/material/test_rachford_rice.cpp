// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2026 SINTEF Digital

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

#include "config.h"

#define BOOST_TEST_MODULE RachfordRiceTest
#include <boost/test/unit_test.hpp>

#include <opm/material/constraintsolvers/RachfordRice.hpp>

#include <dune/common/fvector.hh>

#include <array>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{

struct DynamicVector : public std::vector<double>
{
    using std::vector<double>::vector;
    using field_type = double;
};

} // namespace

BOOST_AUTO_TEST_CASE(TwoComponents)
{
    const Dune::FieldVector<double, 2> K {2.0, 0.5};
    const Dune::FieldVector<double, 2> z {0.4, 0.6};

    BOOST_CHECK_CLOSE_FRACTION(Opm::RachfordRice::solve(K, z, 0), 0.8, 1e-12);
}

BOOST_AUTO_TEST_CASE(ReferenceCases)
{
    using Vector = Dune::FieldVector<double, 3>;
    struct TestCase
    {
        Vector K;
        double vaporFraction;
    };

    const Vector z {0.2, 0.5, 0.3};
    const std::array<TestCase, 16> cases {{
        {{19742.008209810265, 104061.44705736745, 0.29692744936348753}, 0.9956234231343956},
        {{0.6247560532583887, 1.754176409580374, 0.00264809842113736}, 0.004634932674127616},
        {{5011.808655921476, 20394.761667099738, 0.2981316471374891}, 0.9973104175796784},
        {{0.699756966810626, 1.7243997063092453, 0.0032527792216226767}, 0.00540010197110916},
        {{1602.907259275084, 5278.701579138767, 0.29886066422781404}, 0.998280924170745},
        {{0.7857993293648643, 1.6860914516428702, 0.0052022817582215155}, 0.003259588429511987},
        {{623.5248499334857, 1730.8998902544013, 0.28483826298498793}, 0.9785380752533385},
        {{0.8738459013983119, 1.6483861730818257, 0.009126227905247714}, 0.003343693200458504},
        {{273.5620907524389, 654.8043484805389, 0.28778776891147734}, 0.9822111573271644},
        {{0.9594432989887618, 1.6087045715588943, 0.01668691940009185}, 0.002619159406464278},
        {{134.27575624313343, 283.4218134512125, 0.2904166028146171}, 0.9850934240313229},
        {{1.1056927210301593, 1.7637285066428006, 0.025528660493607223}, 0.1829805554210638},
        {{1.2988026430219273, 1.9765552313014425, 0.04040594253059525}, 0.33270889928548736},
        {{1.523322259507949, 2.2013619219246285, 0.06723018079131272}, 0.44310335029510134},
        {{1.7176562249206835, 2.3413966156487644, 0.11537246148979083}, 0.5269214180997791},
        {{1.787578933245329, 2.282308523254643, 0.19884032011593508}, 0.6062547183490403},
    }};

    for (std::size_t index = 0; index < cases.size(); ++index) {
        BOOST_TEST_CONTEXT("reference case " << index)
        {
            const auto liquidFraction = Opm::RachfordRice::solve(cases[index].K, z, 0);
            BOOST_CHECK_CLOSE_FRACTION(liquidFraction, 1.0 - cases[index].vaporFraction, 1e-5);
        }
    }
}

BOOST_AUTO_TEST_CASE(BisectionFallback)
{
    const Dune::FieldVector<double, 3> K {0.001, 0.1, 100.0};
    const Dune::FieldVector<double, 3> z {0.05, 0.9, 0.05};

    BOOST_CHECK_CLOSE_FRACTION(Opm::RachfordRice::solve(K, z), 0.9543616156283405, 1e-9);
}

BOOST_AUTO_TEST_CASE(FloatBisectionFallback)
{
    const Dune::FieldVector<float, 3> K {0.001f, 0.1f, 100.0f};
    const Dune::FieldVector<float, 3> z {0.05f, 0.9f, 0.05f};

    BOOST_CHECK_CLOSE_FRACTION(Opm::RachfordRice::solve(K, z), 0.9543616f, 1e-5f);
}

BOOST_AUTO_TEST_CASE(InactiveComponent)
{
    const Dune::FieldVector<double, 2> K {0.0, 2.0};
    const Dune::FieldVector<double, 2> z {0.0, 1.0};

    BOOST_CHECK_EQUAL(Opm::RachfordRice::solve(K, z), 0.0);
}

BOOST_AUTO_TEST_CASE(SinglePhaseLimits)
{
    using Vector = Dune::FieldVector<double, 3>;
    const Vector z {0.2, 0.5, 0.3};

    const Vector liquidK {0.2, 0.5, 0.9};
    BOOST_CHECK_EQUAL(Opm::RachfordRice::solve(liquidK, z), 1.0);

    const Vector vaporK {1.1, 2.0, 5.0};
    BOOST_CHECK_EQUAL(Opm::RachfordRice::solve(vaporK, z), 0.0);

    // Straddling K-values do not guarantee a two-phase root.
    const Vector straddlingLiquidK {2.0, 0.5, 0.8};
    const Vector mostlyHeavy {0.01, 0.89, 0.1};
    BOOST_CHECK_EQUAL(Opm::RachfordRice::solve(straddlingLiquidK, mostlyHeavy), 1.0);

    const Vector straddlingVaporK {2.0, 0.5, 1.2};
    const Vector mostlyLight {0.89, 0.01, 0.1};
    BOOST_CHECK_EQUAL(Opm::RachfordRice::solve(straddlingVaporK, mostlyLight), 0.0);
}

BOOST_AUTO_TEST_CASE(IndeterminatePhaseSplit)
{
    const Dune::FieldVector<double, 3> K {1.0, 1.0, 1.0};
    const Dune::FieldVector<double, 3> z {0.2, 0.5, 0.3};

    BOOST_CHECK_EXCEPTION(Opm::RachfordRice::solve(K, z),
                          std::invalid_argument,
                          [](const std::invalid_argument& error) {
                              return std::string {error.what()}.find("phase split")
                                  != std::string::npos;
                          });
}

BOOST_AUTO_TEST_CASE(InvalidInput)
{
    const DynamicVector empty;
    BOOST_CHECK_EXCEPTION(Opm::RachfordRice::solve(empty, empty, 0),
                          std::invalid_argument,
                          [](const std::invalid_argument& error) {
                              return std::string {error.what()}.find("at least one component")
                                  != std::string::npos;
                          });

    const DynamicVector K {2.0, 0.5};
    const DynamicVector z {1.0};
    BOOST_CHECK_EXCEPTION(Opm::RachfordRice::solve(K, z, 0),
                          std::invalid_argument,
                          [](const std::invalid_argument& error) {
                              return std::string {error.what()}.find("2 equilibrium ratios and 1")
                                  != std::string::npos;
                          });

    const DynamicVector validK {2.0, 0.5};
    const DynamicVector validZ {0.4, 0.6};
    const DynamicVector nanK {std::numeric_limits<double>::quiet_NaN(), 0.5};
    const DynamicVector negativeK {-1.0, 0.5};
    const DynamicVector infiniteZ {std::numeric_limits<double>::infinity(), 0.6};
    const DynamicVector negativeZ {-0.4, 1.4};
    const DynamicVector zeroZ {0.0, 0.0};

    BOOST_CHECK_THROW(Opm::RachfordRice::solve(nanK, validZ), std::invalid_argument);
    BOOST_CHECK_THROW(Opm::RachfordRice::solve(negativeK, validZ), std::invalid_argument);
    BOOST_CHECK_THROW(Opm::RachfordRice::solve(validK, infiniteZ), std::invalid_argument);
    BOOST_CHECK_THROW(Opm::RachfordRice::solve(validK, negativeZ), std::invalid_argument);
    BOOST_CHECK_THROW(Opm::RachfordRice::solve(validK, zeroZ), std::invalid_argument);
}
