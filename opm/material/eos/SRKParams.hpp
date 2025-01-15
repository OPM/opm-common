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
#ifndef SRK_PARAMS_HPP
#define SRK_PARAMS_HPP

namespace Opm {

template <class Scalar, class FluidSystem>
class SRKParams
{
    static constexpr Scalar R = Constants<Scalar>::R;

public:

    static Scalar calcOmegaA(Scalar temperature, unsigned compIdx)
    {
        Scalar Tr = temperature / FluidSystem::criticalTemperature(compIdx);
        Scalar omega = FluidSystem::acentricFactor(compIdx);
        Scalar f_omega = 0.48 + omega * (1.574 + omega * (-0.176));
        Valgrind::CheckDefined(f_omega);
        
        Scalar tmp = 1 + f_omega*(1 - sqrt(Tr));
        return 0.4274802 * tmp * tmp;
    }

    static Scalar calcOmegaB()
    {
        return Scalar(0.08664035);
    }

    static Scalar calcm1()
    {
        return Scalar(0.0);
    }

    static Scalar calcm2()
    {
        return Scalar(1.0);
    }

};  // class SRKParams

}  // namespace Opm

#endif