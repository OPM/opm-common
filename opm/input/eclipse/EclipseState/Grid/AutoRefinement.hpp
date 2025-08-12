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

#ifndef OPM_INPUT_GRID_AUTOREFINEMENT_HPP
#define OPM_INPUT_GRID_AUTOREFINEMENT_HPP

namespace Opm {

class AutoRefinement {
public:
    AutoRefinement();

    AutoRefinement(int nx,
                   int ny,
                   int nz,
                   double option_trans_mult);

    int NX() const;
    int NY() const;
    int NZ() const;
    double OPTION_TRANS_MULT() const;

private:
    int nx_;
    int ny_;
    int nz_;
    double option_trans_mult_{0.};

};
}

#endif // OPM_INPUT_GRID_AUTOREFINEMENT_HPP
