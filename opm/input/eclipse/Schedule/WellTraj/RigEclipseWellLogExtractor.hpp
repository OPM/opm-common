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

#include <array>
#include <cstddef>
#include <optional>
#include <vector>

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

// Data of a grid that is expensive to compute, and can be shared by the
// extractors for the same grid.
struct RigEclipseGridSearchData
{
    cvf::ref<cvf::BoundingBoxTree> cellSearchTree;
    std::optional<double>          lengthThreshold;
};

//==================================================================================================
///
//==================================================================================================
class RigEclipseWellLogExtractor : public RigWellLogExtractor
{
public:
    RigEclipseWellLogExtractor( const RigWellPath* wellpath, const Opm::EclipseGrid& grid, RigEclipseGridSearchData& gridSearchData);

private:
    void                calculateIntersection();
    std::vector<std::size_t> findCloseCellIndices( const cvf::BoundingBox& bb );
    cvf::Vec3d
        calculateLengthInCell( std::size_t cellIndex, const cvf::Vec3d& startPoint, const cvf::Vec3d& endPoint ) const override;

    cvf::Vec3d calculateLengthInCell( const std::array<cvf::Vec3d, 8>& hexCorners,
                                      const cvf::Vec3d&                startPoint,
                                      const cvf::Vec3d&                endPoint ) const;

    std::array<cvf::Vec3d, 8> hexCornersOpmToResinsight( std::size_t cellIndex ) const;
    cvf::BoundingBox cellBoundingBox( std::size_t cellIndex ) const;

    void findCellLocalXYZ( const std::array<cvf::Vec3d, 8>& hexCorners,
                           cvf::Vec3d&                      localXdirection,
                           cvf::Vec3d&                      localYdirection,
                           cvf::Vec3d&                      localZdirection ) const;
    void buildCellSearchTree();
    std::vector<std::size_t> findIntersectingCells( const cvf::BoundingBox& inputBB ) const;
    double computeLengthThreshold() const;
    void computeCachedData();

    const Opm::EclipseGrid& m_grid;
    RigEclipseGridSearchData& m_gridSearchData;
};
} //namespace external
