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
 * \copydoc Opm::EclThermalConductionLawMultiplexer
 */
#ifndef OPM_ECL_THERMAL_CONDUCTION_LAW_MULTIPLEXER_HPP
#define OPM_ECL_THERMAL_CONDUCTION_LAW_MULTIPLEXER_HPP

#include <opm/common/utility/Visitor.hpp>

#include <opm/material/thermal/EclThermalConductionLawMultiplexerParams.hpp>

#include <opm/material/thermal/EclThconrLaw.hpp>
#include <opm/material/thermal/EclThcLaw.hpp>
#include <opm/material/thermal/NullThermalConductionLaw.hpp>

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
 * \brief Implements the total thermal conductivity and rock enthalpy relations used by ECL.
 */
template <class ScalarT,
          class FluidSystem,
          class ParamsT = EclThermalConductionLawMultiplexerParams<ScalarT,FluidSystem>>
class EclThermalConductionLawMultiplexer
{
    enum { numPhases = FluidSystem::numPhases };

    using ThconrLaw = EclThconrLaw<ScalarT, FluidSystem, typename ParamsT::ThconrLawParams>;
    using ThcLaw = EclThcLaw<ScalarT, typename ParamsT::ThcLawParams>;
    using NullLaw = NullThermalConductionLaw<ScalarT>;

public:
    using Params = ParamsT;
    using Scalar = typename Params::Scalar;

    /*!
     * \brief Given a fluid state, compute the volumetric internal energy of the rock [W/m^3].
     */
    template <class FluidState, class Evaluation = typename FluidState::Scalar>
    static Evaluation thermalConductivity(const Params& params,
                                       const FluidState& fluidState)
    {
        Evaluation result;
        params.visit(VisitorOverloadSet{[&](const auto& prm)
                                        {
                                            using Law = typename remove_cvr_t<decltype(prm)>::Law;
                                            result = Law::thermalConductivity(prm, fluidState);
                                        },
                                        [](const std::monostate&)
                                        {
                                            throw std::runtime_error("Undefined thermal conduction approach.");
                                        }});
        return result;
    }
};

} // namespace Opm

#endif
