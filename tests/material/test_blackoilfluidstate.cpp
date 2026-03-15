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
 * \brief This test ensures that the API of the black-oil fluid state conforms to the
 *        fluid state specification
 */
#include "config.h"

#include <boost/mpl/list.hpp>

#define BOOST_TEST_MODULE BlackOilFluidState
#include <boost/test/unit_test.hpp>

#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/densead/Math.hpp>
#include <opm/material/fluidstates/BlackOilFluidState.hpp>
#include <opm/material/fluidsystems/BlackOilFluidSystem.hpp>
#include <opm/material/checkFluidSystem.hpp>

using Types = boost::mpl::list<float,double>;

BOOST_AUTO_TEST_CASE_TEMPLATE(ApiConformance, Scalar, Types)
{
    using FluidSystem = Opm::BlackOilFluidSystem<Scalar>;
    using Evaluation = Opm::DenseAd::Evaluation<Scalar, 2>;
    using FluidStateScalar = Opm::BlackOilFluidState<Scalar, FluidSystem>;
    using FluidState = Opm::BlackOilFluidState<Evaluation, FluidSystem>;

    FluidStateScalar fss{};
    checkFluidState<Scalar>(fss);
    FluidState fs{};
    checkFluidState<Evaluation>(fs);
}

BOOST_AUTO_TEST_CASE_TEMPLATE(SolventSupport, Scalar, Types)
{
    using FluidSystem = Opm::BlackOilFluidSystem<Scalar>;

    // BlackOilFluidState with solvent enabled
    //   template params: Scalar, FluidSystem, storeTemp, storeEnthalpy,
    //                    enableDissolution, enableVapwat, enableBrine,
    //                    enableSaltPrecipitation, enableDissolutionInWater,
    //                    enableSolvent
    using SolventFluidState = Opm::BlackOilFluidState<Scalar, FluidSystem,
                                                      false, false, true, false, false, false, false,
                                                      true>;

    // When solvent is disabled (default), getters should return 0
    using NoSolventFluidState = Opm::BlackOilFluidState<Scalar, FluidSystem>;

    // Test solvent-enabled fluid state
    {
        SolventFluidState fs{};

        // Initially solvent properties should be value-initialized (zero)
        BOOST_CHECK_EQUAL(fs.solventSaturation(), Scalar(0.0));
        BOOST_CHECK_EQUAL(fs.solventDensity(), Scalar(0.0));
        BOOST_CHECK_EQUAL(fs.solventInvB(), Scalar(0.0));
        BOOST_CHECK_EQUAL(fs.rsSolw(), Scalar(0.0));

        // Set and retrieve solvent saturation
        fs.setSolventSaturation(Scalar(0.15));
        BOOST_CHECK_EQUAL(fs.solventSaturation(), Scalar(0.15));

        // Set and retrieve solvent density
        fs.setSolventDensity(Scalar(250.0));
        BOOST_CHECK_EQUAL(fs.solventDensity(), Scalar(250.0));

        // Set and retrieve solvent inverse formation volume factor
        fs.setSolventInvB(Scalar(1.5));
        BOOST_CHECK_EQUAL(fs.solventInvB(), Scalar(1.5));

        // Set and retrieve rsSolw (solvent dissolution factor in water)
        fs.setRsSolw(Scalar(0.05));
        BOOST_CHECK_EQUAL(fs.rsSolw(), Scalar(0.05));
    }

    // Test solvent-disabled fluid state: getters should return 0
    {
        NoSolventFluidState fs{};

        BOOST_CHECK_EQUAL(fs.solventSaturation(), Scalar(0.0));
        BOOST_CHECK_EQUAL(fs.solventDensity(), Scalar(0.0));
        BOOST_CHECK_EQUAL(fs.solventInvB(), Scalar(0.0));
        BOOST_CHECK_EQUAL(fs.rsSolw(), Scalar(0.0));
    }

    // Test assign between solvent-enabled fluid states
    {
        SolventFluidState fs1{};
        fs1.setSolventSaturation(Scalar(0.2));
        fs1.setSolventDensity(Scalar(300.0));
        fs1.setSolventInvB(Scalar(1.8));
        fs1.setRsSolw(Scalar(0.1));
        fs1.setPvtRegionIndex(0);

        SolventFluidState fs2{};
        fs2.assign(fs1);

        BOOST_CHECK_EQUAL(fs2.solventSaturation(), Scalar(0.2));
        BOOST_CHECK_EQUAL(fs2.solventDensity(), Scalar(300.0));
        BOOST_CHECK_EQUAL(fs2.solventInvB(), Scalar(1.8));
        BOOST_CHECK_EQUAL(fs2.rsSolw(), Scalar(0.1));
    }
}
