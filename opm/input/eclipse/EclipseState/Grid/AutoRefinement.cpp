/*
 Copyright (C) 2025 Equinor
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

#include <opm/input/eclipse/EclipseState/Grid/AutoRefinement.hpp>


#include <stdexcept>

namespace Opm {


AutoRefinement::AutoRefinement()
{}

AutoRefinement::AutoRefinement(int nx,
                               int ny,
                               int nz,
                               double option_trans_mult)
{
    bool invalid = invalidRefinementFactor(nx) || invalidRefinementFactor(ny) || invalidRefinementFactor(nz);
    bool notYet = (option_trans_mult>0);
    if (invalid) {
        throw std::invalid_argument("Refinement factors must be odd and positive.");
    }
    else if (notYet) {
        throw std::invalid_argument("Only OPTION_TRANS_MULT 0 is supported for now.");
    }
    nx_ = nx;
    ny_ = ny;
    nz_ = nz;
}

int AutoRefinement::NX() const
{
    return nx_;
}

int AutoRefinement::NY() const
{
    return ny_;
}

int AutoRefinement::NZ() const
{
    return nz_;
}

double AutoRefinement::OPTION_TRANS_MULT() const
{
    // Throw if not equal to zero?
    return option_trans_mult_;
}

} // namespace Opm

