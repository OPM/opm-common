/*
  Copyright (C) 2020 SINTEF Digital

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
#ifndef OPM_AQUIFERHELPERS_HPP
#define OPM_AQUIFERHELPERS_HPP

#include <opm/input/eclipse/EclipseState/Grid/FaceDir.hpp>

#include <cstddef>
#include <optional>
#include <unordered_set>
#include <vector>

namespace Opm {
    class EclipseGrid;
} // namespace Opm

namespace Opm::AquiferHelpers {
    bool neighborCellInsideReservoirAndActive(const EclipseGrid& grid,
                                              int i, int j, int k,
                                              FaceDir::DirEnum faceDir,
                                              const std::vector<int>& actnum,
                                              const std::optional<const std::unordered_set<std::size_t>>& numerical_aquifer_cells = std::nullopt);
} // namespace Opm::AquiferHelpers

#endif // OPM_AQUIFERHELPERS_HPP
