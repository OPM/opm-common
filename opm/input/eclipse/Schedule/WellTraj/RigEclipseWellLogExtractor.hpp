/////////////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) Statoil ASA
//  Copyright (C) Ceetron Solutions AS
//
//  ResInsight is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  ResInsight is distributed in the hope that it will be useful, but WITHOUT ANY
//  WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.
//
//  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
//  for more details.
//
/////////////////////////////////////////////////////////////////////////////////

#pragma once

#include <external/resinsight/ReservoirDataModel/RigWellLogExtractor.h>
#include <external/resinsight/LibGeometry/cvfBoundingBoxTree.h>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/Schedule/ScheduleGrid.hpp>

namespace Opm {
class EclipseGrid;
class ScheduleGrid;
}

namespace external {

class RigEclipseCaseData;
class RigWellPath;
class RigResultAccessor;

namespace cvf
{
class BoundingBox;
}


//==================================================================================================
///
//==================================================================================================
class RigEclipseWellLogExtractor : public RigWellLogExtractor
{
public:
    RigEclipseWellLogExtractor( const RigWellPath* wellpath, const Opm::EclipseGrid& grid, cvf::ref<cvf::BoundingBoxTree>& cellSearchTree);

    cvf::ref<cvf::BoundingBoxTree> getCellSearchTree();
private:
    void                calculateIntersection();
    std::vector<size_t> findCloseCellIndices( const cvf::BoundingBox& bb );
    cvf::Vec3d
        calculateLengthInCell( size_t cellIndex, const cvf::Vec3d& startPoint, const cvf::Vec3d& endPoint ) const override;

    cvf::Vec3d calculateLengthInCell( const std::array<cvf::Vec3d, 8>& hexCorners,
                                      const cvf::Vec3d&                startPoint,
                                      const cvf::Vec3d&                endPoint ) const;

    void hexCornersOpmToResinsight( cvf::Vec3d    hexCorners[8],
                                    size_t        cellIndex ) const;

    void findCellLocalXYZ( const std::array<cvf::Vec3d, 8>& hexCorners,
                           cvf::Vec3d&                      localXdirection,
                           cvf::Vec3d&                      localYdirection,
                           cvf::Vec3d&                      localZdirection ) const;
    void buildCellSearchTree();
    void findIntersectingCells( const cvf::BoundingBox& inputBB, std::vector<size_t>* cellIndices ) const;
    void computeCachedData();

    const Opm::EclipseGrid& m_grid;
    cvf::ref<cvf::BoundingBoxTree> m_cellSearchTree;
};
} //namespace external
