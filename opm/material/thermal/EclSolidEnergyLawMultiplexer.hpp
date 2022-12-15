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
 * \copydoc Opm::EclSolidEnergyLawMultiplexer
 */
#ifndef OPM_ECL_SOLID_ENERGY_LAW_MULTIPLEXER_HPP
#define OPM_ECL_SOLID_ENERGY_LAW_MULTIPLEXER_HPP

#include <opm/common/utility/Visitor.hpp>

#include <opm/material/thermal/EclSolidEnergyLawMultiplexerParams.hpp>
#include <opm/material/thermal/EclHeatcrLaw.hpp>
#include <opm/material/thermal/EclSpecrockLaw.hpp>
#include <opm/material/thermal/NullSolidEnergyLaw.hpp>

#include <string>
#include <stdexcept>

namespace {
template<class T>
using remove_cvr_t = std::remove_cv_t<std::remove_reference_t<T>>;
}

namespace Opm
{
/*!
 * \ingroup material
 *
 * \brief Provides the energy storage relation of rock
 */
template <class ScalarT,
          class FluidSystem,
          class ParamsT = EclSolidEnergyLawMultiplexerParams<ScalarT,FluidSystem>>
class EclSolidEnergyLawMultiplexer
{
    enum { numPhases = FluidSystem::numPhases };

    using HeatcrLaw = EclHeatcrLaw<ScalarT, FluidSystem, typename ParamsT::HeatcrLawParams>;
    using SpecrockLaw = EclSpecrockLaw<ScalarT, typename ParamsT::SpecrockLawParams>;
    using NullLaw = NullSolidEnergyLaw<ScalarT>;

public:
    using Params = ParamsT;
    using Scalar = typename Params::Scalar;

    /*!
     * \brief Given a fluid state, compute the volumetric internal energy of the rock [W/m^3].
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation solidInternalEnergy(const Params& params, const FluidState& fluidState)
    {
        Evaluation result;
        params.visit(VisitorOverloadSet{[&](const auto& prm)
                                        {
                                            using Law = typename remove_cvr_t<decltype(prm)>::Law;
                                            result = Law::solidInternalEnergy(prm, fluidState);
                                        },
                                        [](const std::monostate&)
                                        {
                                            throw std::runtime_error("Undefined solid energy approach.");
                                         }});
        return result;
    }
};

} // namespace Opm

#endif
