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

namespace Opm::FluidStateConcepts {

// Test access through a const reference, as used by the adapters. These
// concepts describe accessors; numerical conversion is handled by decay().

template <class State>
concept HasRs = requires (const State& fs) {
    fs.Rs();
};

template <class State>
concept HasRv = requires (const State& fs) {
    fs.Rv();
};

template <class State>
concept HasRvw = requires (const State& fs) {
    fs.Rvw();
};

template <class State>
concept HasRsw = requires (const State& fs) {
    fs.Rsw();
};

template <class State>
concept HasSaltConcentration = requires (const State& fs) {
    fs.saltConcentration();
};

template <class State>
concept HasSaltSaturation = requires (const State& fs) {
    fs.saltSaturation();
};

template <class State>
concept HasSolventSaturation = requires (const State& fs) {
    fs.solventSaturation();
};

template <class State>
concept HasSolventDensity = requires (const State& fs) {
    fs.solventDensity();
};

template <class State>
concept HasSolventInvB = requires (const State& fs) {
    fs.solventInvB();
};

template <class State>
concept HasRsSolw = requires (const State& fs) {
    fs.rsSolw();
};

template <class State>
concept HasPvtRegionIndex = requires (const State& fs) {
    fs.pvtRegionIndex();
};

template <class State>
concept HasInvB = requires (const State& fs, unsigned phaseIdx) {
    fs.invB(phaseIdx);
};

} // namespace Opm::FluidStateConcepts

namespace Opm::BlackOil
{
template <class FluidSystem, class FluidState, class LhsEval>
OPM_HOST_DEVICE LhsEval
getRs_(const FluidState& fluidState, unsigned regionIdx)
{
    if constexpr (FluidStateConcepts::HasRs<FluidState>) {
        return decay<LhsEval>(fluidState.Rs());
    } else {
        const auto& XoG = decay<LhsEval>(
            fluidState.massFraction(FluidSystem::oilPhaseIdx, FluidSystem::gasCompIdx));
        return FluidSystem::convertXoGToRs(XoG, regionIdx);
    }
}

template <class FluidSystem, class FluidState, class LhsEval>
OPM_HOST_DEVICE LhsEval
getRv_(const FluidState& fluidState, unsigned regionIdx)
{
    if constexpr (FluidStateConcepts::HasRv<FluidState>) {
        return decay<LhsEval>(fluidState.Rv());
    } else {
        const auto& XgO = decay<LhsEval>(
            fluidState.massFraction(FluidSystem::gasPhaseIdx, FluidSystem::oilCompIdx));
        return FluidSystem::convertXgOToRv(XgO, regionIdx);
    }
}

template <class FluidSystem, class FluidState, class LhsEval>
OPM_HOST_DEVICE LhsEval
getRvw_(const FluidState& fluidState, unsigned regionIdx)
{
    if constexpr (FluidStateConcepts::HasRvw<FluidState>) {
        return decay<LhsEval>(fluidState.Rvw());
    } else {
        const auto& XgW = decay<LhsEval>(
            fluidState.massFraction(FluidSystem::gasPhaseIdx, FluidSystem::waterCompIdx));
        return FluidSystem::convertXgWToRvw(XgW, regionIdx);
    }
}

template <class FluidSystem, class FluidState, class LhsEval>
OPM_HOST_DEVICE LhsEval
getRsw_(const FluidState& fluidState, unsigned regionIdx)
{
    if constexpr (FluidStateConcepts::HasRsw<FluidState>) {
        return decay<LhsEval>(fluidState.Rsw());
    } else {
        const auto& XwG = decay<LhsEval>(
            fluidState.massFraction(FluidSystem::waterPhaseIdx, FluidSystem::gasCompIdx));
        return FluidSystem::convertXwGToRsw(XwG, regionIdx);
    }
}

template <class FluidState, class LhsEval>
OPM_HOST_DEVICE LhsEval
getSaltConcentration_(const FluidState& fluidState, unsigned)
{
    if constexpr (FluidStateConcepts::HasSaltConcentration<FluidState>) {
        return decay<LhsEval>(fluidState.saltConcentration());
    } else {
        return 0.0;
    }
}

template <class FluidSystem, class FluidState, class LhsEval>
OPM_HOST_DEVICE LhsEval
getSaltSaturation_(const FluidState& fluidState, unsigned)
{
    if constexpr (FluidStateConcepts::HasSaltSaturation<FluidState>) {
        return decay<LhsEval>(fluidState.saltSaturation());
    } else {
        return 0.0;
    }
}

template <class FluidState, class LhsEval>
OPM_HOST_DEVICE LhsEval
getSolventSaturation_(const FluidState& fluidState, unsigned)
{
    if constexpr (FluidStateConcepts::HasSolventSaturation<FluidState>) {
        return decay<LhsEval>(fluidState.solventSaturation());
    } else {
        return 0.0;
    }
}

template <class FluidState, class LhsEval>
OPM_HOST_DEVICE LhsEval
getSolventDensity_(const FluidState& fluidState, unsigned)
{
    if constexpr (FluidStateConcepts::HasSolventDensity<FluidState>) {
        return decay<LhsEval>(fluidState.solventDensity());
    } else {
        return 0.0;
    }
}

template <class FluidState, class LhsEval>
OPM_HOST_DEVICE LhsEval
getSolventInvB_(const FluidState& fluidState, unsigned)
{
    if constexpr (FluidStateConcepts::HasSolventInvB<FluidState>) {
        return decay<LhsEval>(fluidState.solventInvB());
    } else {
        return 0.0;
    }
}

template <class FluidState, class LhsEval>
OPM_HOST_DEVICE LhsEval
getRsSolw_(const FluidState& fluidState, unsigned)
{
    if constexpr (FluidStateConcepts::HasRsSolw<FluidState>) {
        return decay<LhsEval>(fluidState.rsSolw());
    } else {
        return 0.0;
    }
}

} // namespace Opm::BlackOil
#endif
