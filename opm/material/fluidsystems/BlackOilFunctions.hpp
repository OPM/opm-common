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
 * \copydoc Opm::BlackOilFluidSystem
 */
#ifndef OPM_MATERIAL_FLUIDSYSTEMS_BLACKOILFUNCTIONS_HEADER_INCLUDED
#define OPM_MATERIAL_FLUIDSYSTEMS_BLACKOILFUNCTIONS_HEADER_INCLUDED

#include <opm/common/TimingMacros.hpp>

#include <opm/material/Constants.hpp>
#include <opm/material/fluidsystems/BaseFluidSystem.hpp>

#include <opm/material/common/HasMemberGeneratorMacros.hpp>
#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/common/Valgrind.hpp>
#include <opm/material/fluidsystems/NullParameterCache.hpp>

#include <opm/common/utility/gpuDecorators.hpp>

#include <array>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace Opm::BlackOil
{
OPM_GENERATE_HAS_MEMBER(Rs, ) // Creates 'HasMember_Rs<T>'.
OPM_GENERATE_HAS_MEMBER(Rv, ) // Creates 'HasMember_Rv<T>'.
OPM_GENERATE_HAS_MEMBER(Rvw, ) // Creates 'HasMember_Rvw<T>'.
OPM_GENERATE_HAS_MEMBER(Rsw, ) // Creates 'HasMember_Rsw<T>'.
OPM_GENERATE_HAS_MEMBER(saltConcentration, )
OPM_GENERATE_HAS_MEMBER(saltSaturation, )

template <class FluidSystem, class FluidState, class LhsEval>
LhsEval
OPM_HOST_DEVICE getRs_(typename std::enable_if<!HasMember_Rs<FluidState>::value, const FluidState&>::type fluidState,
       unsigned regionIdx)
{
    const auto& XoG = decay<LhsEval>(fluidState.massFraction(FluidSystem::oilPhaseIdx, FluidSystem::gasCompIdx));
    return FluidSystem::convertXoGToRs(XoG, regionIdx);
}

template <class FluidSystem, class FluidState, class LhsEval>
auto
OPM_HOST_DEVICE getRs_(typename std::enable_if<HasMember_Rs<FluidState>::value, const FluidState&>::type fluidState, unsigned)
    -> decltype(decay<LhsEval>(fluidState.Rs()))
{
    return decay<LhsEval>(fluidState.Rs());
}

template <class FluidSystem, class FluidState, class LhsEval>
LhsEval
OPM_HOST_DEVICE getRv_(typename std::enable_if<!HasMember_Rv<FluidState>::value, const FluidState&>::type fluidState,
       unsigned regionIdx)
{
    const auto& XgO = decay<LhsEval>(fluidState.massFraction(FluidSystem::gasPhaseIdx, FluidSystem::oilCompIdx));
    return FluidSystem::convertXgOToRv(XgO, regionIdx);
}

template <class FluidSystem, class FluidState, class LhsEval>
auto
OPM_HOST_DEVICE getRv_(typename std::enable_if<HasMember_Rv<FluidState>::value, const FluidState&>::type fluidState, unsigned)
    -> decltype(decay<LhsEval>(fluidState.Rv()))
{
    return decay<LhsEval>(fluidState.Rv());
}

template <class FluidSystem, class FluidState, class LhsEval>
LhsEval
OPM_HOST_DEVICE getRvw_(typename std::enable_if<!HasMember_Rvw<FluidState>::value, const FluidState&>::type fluidState,
        unsigned regionIdx)
{
    const auto& XgW = decay<LhsEval>(fluidState.massFraction(FluidSystem::gasPhaseIdx, FluidSystem::waterCompIdx));
    return FluidSystem::convertXgWToRvw(XgW, regionIdx);
}

template <class FluidSystem, class FluidState, class LhsEval>
auto
OPM_HOST_DEVICE getRvw_(typename std::enable_if<HasMember_Rvw<FluidState>::value, const FluidState&>::type fluidState, unsigned)
    -> decltype(decay<LhsEval>(fluidState.Rvw()))
{
    return decay<LhsEval>(fluidState.Rvw());
}

template <class FluidSystem, class FluidState, class LhsEval>
LhsEval
OPM_HOST_DEVICE getRsw_(typename std::enable_if<!HasMember_Rsw<FluidState>::value, const FluidState&>::type fluidState,
        unsigned regionIdx)
{
    const auto& XwG = decay<LhsEval>(fluidState.massFraction(FluidSystem::waterPhaseIdx, FluidSystem::gasCompIdx));
    return FluidSystem::convertXwGToRsw(XwG, regionIdx);
}

template <class FluidSystem, class FluidState, class LhsEval>
auto
OPM_HOST_DEVICE getRsw_(typename std::enable_if<HasMember_Rsw<FluidState>::value, const FluidState&>::type fluidState, unsigned)
    -> decltype(decay<LhsEval>(fluidState.Rsw()))
{
    return decay<LhsEval>(fluidState.Rsw());
}

template <class FluidState, class LhsEval>
LhsEval
OPM_HOST_DEVICE getSaltConcentration_(typename std::enable_if<!HasMember_saltConcentration<FluidState>::value, const FluidState&>::type,
                      unsigned)
{
    return 0.0;
}

template <class FluidState, class LhsEval>
auto
OPM_HOST_DEVICE getSaltConcentration_(
    typename std::enable_if<HasMember_saltConcentration<FluidState>::value, const FluidState&>::type fluidState,
    unsigned) -> decltype(decay<LhsEval>(fluidState.saltConcentration()))
{
    return decay<LhsEval>(fluidState.saltConcentration());
}

template <class FluidSystem, class FluidState, class LhsEval>
LhsEval
OPM_HOST_DEVICE getSaltSaturation_(typename std::enable_if<!HasMember_saltSaturation<FluidState>::value, const FluidState&>::type,
                   unsigned)
{
    return 0.0;
}

template <class FluidSystem, class FluidState, class LhsEval>
auto
OPM_HOST_DEVICE getSaltSaturation_(
    typename std::enable_if<HasMember_saltSaturation<FluidState>::value, const FluidState&>::type fluidState, unsigned)
    -> decltype(decay<LhsEval>(fluidState.saltSaturation()))
{
    return decay<LhsEval>(fluidState.saltSaturation());
}

} // namespace Opm::BlackOil
#endif
