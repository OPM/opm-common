// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2022 NORCE.
  Copyright 2022 SINTEF Digital, Mathematics and Cybernetics.

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
#ifndef PR_PARAMS_HPP
#define PR_PARAMS_HPP

#include <opm/material/eos/CubicEOSParams.hpp>

#include <cmath>

namespace Opm {

template <class Scalar, class FluidSystem, unsigned phaseIdx>
class PRParams : public CubicEOSParams<Scalar, FluidSystem, phaseIdx>
{
    static constexpr Scalar R = Constants<Scalar>::R;

    // using Toolbox = MathToolbox<Scalar>;

public:
    Scalar calcOmegaA(Scalar temperature, unsigned compIdx) override
    {
        Scalar Tr = temperature / FluidSystem::criticalTemperature(compIdx);
        Scalar omega = FluidSystem::acentricFactor(compIdx);
        Scalar f_omega;
        if (omega < 0.49) 
            f_omega = 0.37464  + omega*(1.54226 + omega*(-0.26992));
        else              
            f_omega = 0.379642 + omega*(1.48503 + omega*(-0.164423 + omega*0.016666));
        Valgrind::CheckDefined(f_omega);
        
        Scalar tmp = 1 + f_omega*(1 - sqrt(Tr));
        return 0.457235529 * tmp * tmp;
    }

    Scalar calcOmegaB([[maybe_unused]] Scalar temperature, [[maybe_unused]] unsigned compIdx) override
    {
        return Scalar(0.077796074);
    }

    Scalar m1() const
    {
        return Scalar(1 + std::sqrt(2));
    }

    Scalar m2() const
    {
        return Scalar(1 - std::sqrt(2));
    }
};  // class PRParams

}  // namespace Opm

#endif