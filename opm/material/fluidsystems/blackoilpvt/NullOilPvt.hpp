/*
  Copyright 2025 Equinor ASA
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
#ifndef OPM_NULL_OIL_PVT_HPP
#define OPM_NULL_OIL_PVT_HPP

#include <opm/common/utility/gpuDecorators.hpp>
#include <opm/material/fluidsystems/blackoilpvt/OilPvtMultiplexer.hpp>

namespace Opm
{

/*!
 * \brief Null object for oil PVT calculations
 *
 * This class provides empty implementations for all oil PVT methods,
 * returning default values (0.0, false, etc.) for all calculations.
 * Used when oil phase is not active or not supported.
 */
template <class Scalar>
class NullOilPvt
{
public:
    // Approach method
    OPM_HOST_DEVICE static constexpr auto approach()
    {
        return OilPvtApproach::NoOil;
    } // NoOil approach

    OPM_HOST_DEVICE static constexpr bool isActive()
    {
        return false;
    }

    // Formation volume factor methods
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation inverseFormationVolumeFactor(unsigned /*regionIdx*/,
                                                            const Evaluation& /*temperature*/,
                                                            const Evaluation& /*pressure*/,
                                                            const Evaluation& /*Rs*/) const
    {
        return 1.0;
    }

    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedInverseFormationVolumeFactor(unsigned /*regionIdx*/,
                                                                     const Evaluation& /*temperature*/,
                                                                     const Evaluation& /*pressure*/) const
    {
        return 1.0;
    }

    // Dissolution factor methods
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedGasDissolutionFactor(unsigned /*regionIdx*/,
                                                             const Evaluation& /*temperature*/,
                                                             const Evaluation& /*pressure*/) const
    {
        return 0.0;
    }

    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedGasDissolutionFactor(unsigned /*regionIdx*/,
                                                             const Evaluation& /*temperature*/,
                                                             const Evaluation& /*pressure*/,
                                                             const Evaluation& /*oilSaturation*/,
                                                             const Evaluation& /*maxOilSaturation*/) const
    {
        return 0.0;
    }

    // Saturation pressure method
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturationPressure(unsigned /*regionIdx*/,
                                                  const Evaluation& /*temperature*/,
                                                  const Evaluation& /*Rs*/) const
    {
        return 0.0;
    }

    // Viscosity methods
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation viscosity(unsigned /*regionIdx*/,
                                         const Evaluation& /*temperature*/,
                                         const Evaluation& /*pressure*/,
                                         const Evaluation& /*Rs*/) const
    {
        return 0.0;
    }

    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation saturatedViscosity(unsigned /*regionIdx*/,
                                                  const Evaluation& /*temperature*/,
                                                  const Evaluation& /*pressure*/) const
    {
        return 0.0;
    }

    // Internal energy methods
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation internalEnergy(unsigned /*regionIdx*/,
                                              const Evaluation& /*temperature*/,
                                              const Evaluation& /*pressure*/,
                                              const Evaluation& /*Rs*/) const
    {
        return 0.0;
    }

    // Vaporization parameter methods
    OPM_HOST_DEVICE void setVapPars(const Scalar /*par1*/, const Scalar /*par2*/) const
    {
    }

    // Heat of vaporization
    OPM_HOST_DEVICE Scalar hVap(unsigned /*regionIdx*/) const
    {
        return 0.0;
    }

    // Mixing energy flag
    OPM_HOST_DEVICE static constexpr bool mixingEnergy()
    {
        return false;
    }

    // Diffusion coefficient
    template <class Evaluation>
    OPM_HOST_DEVICE Evaluation diffusionCoefficient(const Evaluation& /*temperature*/,
                                                    const Evaluation& /*pressure*/,
                                                    unsigned /*compIdx*/) const
    {
        return 0.0;
    }

    OPM_HOST_DEVICE Scalar oilReferenceDensity(unsigned /*regionIdx*/) const
    {
        // TODO: switch from assert to the proper way of handling errors on gpu
        assert(false && "NullOilPvt::oilReferenceDensity() should not be called.");
        return Scalar(0);
    }
};

} // namespace Opm

#endif // OPM_NULL_OIL_PVT_HPP
