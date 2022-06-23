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
 * \brief This is test for the ChiFlash flash solver.
 */
#include "config.h"

#include <opm/material/constraintsolvers/ChiFlash.hpp>
#include <opm/material/fluidsystems/Co2BrineFluidSystem.hh>
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/constraintsolvers/ComputeFromReferencePhase.hpp>
#include <opm/material/fluidstates/CompositionalFluidState.hpp>
#include <opm/material/fluidmatrixinteractions/LinearMaterial.hpp>

#include <dune/common/parallel/mpihelper.hh>
// the following include should be removed later
// #include <opm/material/fluidsystems/chifluid/chiwoms.h>

void testCo2BrineFlash()
{
    using Scalar = double;
    using FluidSystem = Opm::Co2BrineFluidSystem<Scalar>;

    constexpr auto numComponents = FluidSystem::numComponents;
    using Evaluation = Opm::DenseAd::Evaluation<double, numComponents>;
    typedef Dune::FieldVector<Evaluation, numComponents> ComponentVector;
    typedef Opm::CompositionalFluidState<Evaluation, FluidSystem> FluidState;

    // input
    Evaluation p_init = Evaluation::createVariable(10e5, 0); // 10 bar
    ComponentVector comp;
    comp[0] = Evaluation::createVariable(0.5, 1);
    comp[1] = 1. - comp[0];//Evaluation::createVariable(0.1, 1);
    //comp[2] = 0;//1. - comp[0] - comp[1];
    ComponentVector sat;
    sat[0] = 1.0; sat[1] = 1.0-sat[0];
    // TODO: should we put the derivative against the temperature here?
    Scalar temp = 300.0;
    // From co2-compositional branch, it uses
    // typedef typename FluidSystem::template ParameterCache<Scalar> ParameterCache;

    FluidState fs;
    // TODO: no capillary pressure for now

    fs.setPressure(FluidSystem::oilPhaseIdx, p_init);
    fs.setPressure(FluidSystem::gasPhaseIdx, p_init);

    fs.setMoleFraction(FluidSystem::oilPhaseIdx, FluidSystem::Comp0Idx, comp[0]);
    fs.setMoleFraction(FluidSystem::oilPhaseIdx, FluidSystem::Comp1Idx, comp[1]);
    //fs.setMoleFraction(FluidSystem::oilPhaseIdx, FluidSystem::Comp2Idx, comp[2]);

    fs.setMoleFraction(FluidSystem::gasPhaseIdx, FluidSystem::Comp0Idx, comp[0]);
    fs.setMoleFraction(FluidSystem::gasPhaseIdx, FluidSystem::Comp1Idx, comp[1]);
    //fs.setMoleFraction(FluidSystem::gasPhaseIdx, FluidSystem::Comp2Idx, comp[2]);

    // It is used here only for calculate the z
    fs.setSaturation(FluidSystem::oilPhaseIdx, sat[0]);
    fs.setSaturation(FluidSystem::gasPhaseIdx, sat[1]);

    fs.setTemperature(temp);

    // ParameterCache paramCache;
    {
        typename FluidSystem::template ParameterCache<Evaluation> paramCache;
        paramCache.updatePhase(fs, FluidSystem::oilPhaseIdx);
        paramCache.updatePhase(fs, FluidSystem::gasPhaseIdx);
        fs.setDensity(FluidSystem::oilPhaseIdx, FluidSystem::density(fs, paramCache, FluidSystem::oilPhaseIdx));
        fs.setDensity(FluidSystem::gasPhaseIdx, FluidSystem::density(fs, paramCache, FluidSystem::gasPhaseIdx));
    }

    ComponentVector zInit(0.); // TODO; zInit needs to be normalized.
    {
        Scalar sumMoles = 0.0;
        for (unsigned phaseIdx = 0; phaseIdx < FluidSystem::numPhases; ++phaseIdx) {
            for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
                Scalar tmp = Opm::getValue(fs.molarity(phaseIdx, compIdx) * fs.saturation(phaseIdx));
                zInit[compIdx] += Opm::max(tmp, 1e-8);
                sumMoles += tmp;
            }
        }
        zInit /= sumMoles;
        // initialize the derivatives
        // TODO: the derivative eventually should be from the reservoir flow equations
        Evaluation z_last = 1.;
        for (unsigned compIdx = 0; compIdx < numComponents - 1; ++compIdx) {
            zInit[compIdx] = Evaluation::createVariable(Opm::getValue(zInit[compIdx]), compIdx + 1);
            z_last -= zInit[compIdx];
        }
        zInit[numComponents - 1] = z_last;
    }

    // TODO: only, p, z need the derivatives.
    const double flash_tolerance = 1.e-12; // just to test the setup in co2-compositional
    const int flash_verbosity = 1;
    //const std::string flash_twophase_method = "ssi"; // "ssi"
    //const std::string flash_twophase_method = "newton";
     const std::string flash_twophase_method = "newton";

    // TODO: should we set these?
    // Set initial K and L
    for (unsigned compIdx = 0; compIdx < numComponents; ++compIdx) {
        const Evaluation Ktmp = fs.wilsonK_(compIdx);
        fs.setKvalue(compIdx, Ktmp);
    }
    const Evaluation Ltmp = 1.;
    fs.setLvalue(Ltmp);

    const int spatialIdx = 0;
    using Flash = Opm::ChiFlash<double, FluidSystem>;
    // TODO: here the zInit does not have the proper derivatives
    Flash::solve(fs, zInit, spatialIdx, flash_verbosity, flash_twophase_method, flash_tolerance);

}

int main(int argc, char **argv)
{
    Dune::MPIHelper::instance(argc, argv);

    testCo2BrineFlash();

    return 0;
}
