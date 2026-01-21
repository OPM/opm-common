/*
  Copyright 2026 Equinor.

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

#ifndef OPM_WELL_TRAJ_INFO_HPP
#define OPM_WELL_TRAJ_INFO_HPP

#include <external/resinsight/LibGeometry/cvfBoundingBoxTree.h>
#include <external/resinsight/ReservoirDataModel/RigWellLogExtractor.h>
#include <external/resinsight/ReservoirDataModel/RigWellPath.h>

namespace Opm {

struct WellTrajInfo
{
    std::vector<external::WellPathCellIntersectionInfo> intersections;
    external::cvf::ref<external::cvf::BoundingBoxTree> cellSearchTree;
    external::cvf::ref<external::RigWellPath> wellPathGeometry;
};

} // namespace Opm

#endif // OPM_WELL_TRAJ_INFO_HPP
