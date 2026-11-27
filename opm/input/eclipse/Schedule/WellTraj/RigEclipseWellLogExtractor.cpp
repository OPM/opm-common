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

// This file is based on the following files of ResInsight:
//     ApplicationLibCode\ReservoirDataModel\RigEclipseWellLogExtractor.cpp
//     ApplicationLibCode\ReservoirDataModel\RigCellGeometryTools.cpp
//     ApplicationLibCode\ReservoirDataModel\RigMainGrid.cpp
//     ApplicationLibCode\ReservoirDataModel\RigWellPathIntersectionTools.cpp

#include "RigEclipseWellLogExtractor.hpp"

#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include <opm/input/eclipse/Schedule/ScheduleGrid.hpp>

#include <external/resinsight/ReservoirDataModel/RigWellLogExtractionTools.h>
#include <external/resinsight/ReservoirDataModel/RigWellPath.h>
#include <external/resinsight/ReservoirDataModel/cvfGeometryTools.h>
#include <external/resinsight/ReservoirDataModel/RigWellLogExtractor.h>
#include <external/resinsight/ReservoirDataModel/RigCellGeometryTools.h>

#include <external/resinsight/CommonCode/cvfStructGrid.h>

#include <external/resinsight/LibGeometry/cvfBoundingBox.h>

#include <external/resinsight/cafPdmCore/cafAssert.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <map>
#include <utility>

namespace external {

namespace {

double signedVolume( const cvf::Vec3d& a, const cvf::Vec3d& b, const cvf::Vec3d& c, const cvf::Vec3d& d )
{
    return ( b - a ) * ( ( c - a ) ^ ( d - a ) );
}

// The old ray-casting point-in-cell check misses points vertically below a face
// centre or a face diagonal. Split the cell into 24 tetrahedra, each formed by
// the cell centre and one of the face triangles that
// RigHexIntersectionTools::lineHexCellIntersection uses. Boundary points count
// as inside.
bool isPointInsideCell( const cvf::Vec3d& point, const std::array<cvf::Vec3d, 8>& hexCorners )
{
    cvf::Vec3d cellCenter = cvf::Vec3d::ZERO;
    for ( int i = 0; i < 8; ++i )
    {
        cellCenter += hexCorners[i];
    }
    cellCenter /= 8.0;

    for ( int face = 0; face < 6; ++face )
    {
        cvf::ubyte faceVertexIndices[4];
        cvf::StructGridInterface::cellFaceVertexIndices( static_cast<cvf::StructGridInterface::FaceType>( face ),
                                                         faceVertexIndices );
        cvf::Vec3d faceCenter = cvf::GeometryTools::computeFaceCenter( hexCorners[faceVertexIndices[0]],
                                                                       hexCorners[faceVertexIndices[1]],
                                                                       hexCorners[faceVertexIndices[2]],
                                                                       hexCorners[faceVertexIndices[3]] );

        for ( int i = 0; i < 4; ++i )
        {
            int next = i < 3 ? i + 1 : 0;

            const cvf::Vec3d& a = hexCorners[faceVertexIndices[i]];
            const cvf::Vec3d& b = hexCorners[faceVertexIndices[next]];

            // The point is in the tetrahedron if replacing any of its vertices by
            // the point does not flip the sign of the volume.
            const double volume = signedVolume( cellCenter, faceCenter, a, b );
            if ( volume != 0.0 && signedVolume( point, faceCenter, a, b ) * volume >= 0.0 &&
                 signedVolume( cellCenter, point, a, b ) * volume >= 0.0 &&
                 signedVolume( cellCenter, faceCenter, point, b ) * volume >= 0.0 &&
                 signedVolume( cellCenter, faceCenter, a, point ) * volume >= 0.0 )
            {
                return true;
            }
        }
    }
    return false;
}

}

 RigEclipseWellLogExtractor::RigEclipseWellLogExtractor( const RigWellPath* wellpath,
                                                         const Opm::EclipseGrid& grid,
                                                         RigEclipseGridSearchData& gridSearchData )
    : RigWellLogExtractor( wellpath, "" )
      ,m_grid(grid)
      ,m_gridSearchData(gridSearchData)
{
    calculateIntersection();
}

// Modified version of ApplicationLibCode\ReservoirDataModel\RigEclipseWellLogExtractor.cpp
void RigEclipseWellLogExtractor::calculateIntersection()
{
    std::map<RigMDCellIdxEnterLeaveKey, HexIntersectionInfo> uniqueIntersections;

    if ( m_wellPathGeometry->wellPathPoints().empty() ) return;

    buildCellSearchTree();

    if ( !m_gridSearchData.lengthThreshold.has_value() )
    {
        m_gridSearchData.lengthThreshold = computeLengthThreshold();
    }
    double tolerance = m_gridSearchData.lengthThreshold.value();

    for ( std::size_t wpp = 0; wpp < m_wellPathGeometry->wellPathPoints().size() - 1; ++wpp )
    {
        std::vector<HexIntersectionInfo> intersections;
        cvf::Vec3d                       p1 = m_wellPathGeometry->wellPathPoints()[wpp];
        cvf::Vec3d                       p2 = m_wellPathGeometry->wellPathPoints()[wpp + 1];

        cvf::BoundingBox bb;

        bb.add( p1 );
        bb.add( p2 );

        std::vector<std::size_t> closeCellIndices = findCloseCellIndices( bb );

        for ( const auto& globalCellIndex : closeCellIndices )
        {
            std::array<cvf::Vec3d, 8> hexCorners = hexCornersOpmToResinsight( globalCellIndex );
            RigHexIntersectionTools::lineHexCellIntersection( p1, p2, hexCorners, globalCellIndex, &intersections );
        }

        // Now, with all the intersections of this piece of line, we need to
        // sort them in order, and set the measured depth and corresponding cell index

        // Inserting the intersections in this map will remove identical intersections
        // and sort them according to MD, CellIdx, Leave/enter

        double md1 = m_wellPathGeometry->measuredDepths()[wpp];
        double md2 = m_wellPathGeometry->measuredDepths()[wpp + 1];

        insertIntersectionsInMap( intersections, p1, md1, p2, md2, tolerance, &uniqueIntersections );
    }

    if ( uniqueIntersections.empty() && m_wellPathGeometry->wellPathPoints().size() > 1 )
    {
        // When entering this function, all well path points are either completely outside the grid
        // or all well path points are inside one cell

        cvf::Vec3d firstPoint = m_wellPathGeometry->wellPathPoints().front();
        cvf::Vec3d lastPoint  = m_wellPathGeometry->wellPathPoints().back();

        {
            cvf::BoundingBox bb;
            bb.add( firstPoint );

            std::vector<std::size_t> closeCellIndices = findCloseCellIndices( bb );

            for ( const auto& globalCellIndex : closeCellIndices )
            {
                if ( !m_grid.cellActive( globalCellIndex ) )
                {
                    continue;
                }

                std::array<cvf::Vec3d, 8> hexCorners = hexCornersOpmToResinsight( globalCellIndex );
                if ( isPointInsideCell( firstPoint, hexCorners ) )
                {
                    if ( isPointInsideCell( lastPoint, hexCorners ) )
                    {
                        {
                            // Mark the first well path point as entering the cell

                            bool                      isEntering = true;
                            HexIntersectionInfo       info( firstPoint, isEntering, cvf::StructGridInterface::NO_FACE, globalCellIndex );
                            RigMDCellIdxEnterLeaveKey enterLeaveKey( m_wellPathGeometry->measuredDepths().front(),
                                                                     globalCellIndex,
                                                                     isEntering,
                                                                     tolerance );

                            uniqueIntersections.insert( std::make_pair( enterLeaveKey, info ) );
                        }

                        {
                            // Mark the last well path point as leaving cell

                            bool                      isEntering = false;
                            HexIntersectionInfo       info( lastPoint, isEntering, cvf::StructGridInterface::NO_FACE, globalCellIndex );
                            RigMDCellIdxEnterLeaveKey enterLeaveKey( m_wellPathGeometry->measuredDepths().back(),
                                                                     globalCellIndex,
                                                                     isEntering,
                                                                     tolerance );

                            uniqueIntersections.insert( std::make_pair( enterLeaveKey, info ) );
                        }

                        // Points on a shared face may be inside multiple cells; take the first active one.
                        break;
                    }
                }
            }
        }
    }

    this->populateReturnArrays( uniqueIntersections );
}

// Modified version of ApplicationLibCode\ReservoirDataModel\RigEclipseWellLogExtractor.cpp
cvf::Vec3d RigEclipseWellLogExtractor::calculateLengthInCell( std::size_t            cellIndex,
                                                              const cvf::Vec3d& startPoint,
                                                              const cvf::Vec3d& endPoint ) const
{
    std::array<cvf::Vec3d, 8> hexCorners = hexCornersOpmToResinsight( cellIndex );

    return RigEclipseWellLogExtractor::calculateLengthInCell( hexCorners, startPoint, endPoint );
}

// Modified version of ApplicationLibCode\ReservoirDataModel\RigWellPathIntersectionTools.cpp
cvf::Vec3d RigEclipseWellLogExtractor::calculateLengthInCell( const std::array<cvf::Vec3d, 8>& hexCorners,
                                                              const cvf::Vec3d&                startPoint,
                                                              const cvf::Vec3d&                endPoint ) const
{

    cvf::Vec3d vec = endPoint - startPoint;
    cvf::Vec3d iAxisDirection;
    cvf::Vec3d jAxisDirection;
    cvf::Vec3d kAxisDirection;

    RigEclipseWellLogExtractor::findCellLocalXYZ( hexCorners, iAxisDirection, jAxisDirection, kAxisDirection );

    cvf::Mat3d localCellCoordinateSystem( iAxisDirection.x(),
                                          jAxisDirection.x(),
                                          kAxisDirection.x(),
                                          iAxisDirection.y(),
                                          jAxisDirection.y(),
                                          kAxisDirection.y(),
                                          iAxisDirection.z(),
                                          jAxisDirection.z(),
                                          kAxisDirection.z() );

    auto signedVector = vec.getTransformedVector( localCellCoordinateSystem.getInverted() );

    return { std::fabs( signedVector.x() ), std::fabs( signedVector.y() ), std::fabs( signedVector.z() ) };
}

// Modified version of ApplicationLibCode\ReservoirDataModel\RigCellGeometryTools.cpp
void RigEclipseWellLogExtractor::findCellLocalXYZ( const std::array<cvf::Vec3d, 8>& hexCorners,
                                                   cvf::Vec3d&                      localXdirection,
                                                   cvf::Vec3d&                      localYdirection,
                                                   cvf::Vec3d&                      localZdirection ) const
{

    cvf::Vec3d faceCenterNegI = cvf::GeometryTools::computeFaceCenter( hexCorners[0],
                                                                       hexCorners[4],
                                                                       hexCorners[7],
                                                                       hexCorners[3] );

    cvf::Vec3d faceCenterPosI = cvf::GeometryTools::computeFaceCenter( hexCorners[1],
                                                                       hexCorners[2],
                                                                       hexCorners[6],
                                                                       hexCorners[5] );

    cvf::Vec3d faceCenterNegJ = cvf::GeometryTools::computeFaceCenter( hexCorners[0],
                                                                       hexCorners[1],
                                                                       hexCorners[5],
                                                                       hexCorners[4] );

    cvf::Vec3d faceCenterPosJ = cvf::GeometryTools::computeFaceCenter( hexCorners[3],
                                                                       hexCorners[7],
                                                                       hexCorners[6],
                                                                       hexCorners[2] );

    cvf::Vec3d faceCenterCenterVectorI = faceCenterPosI - faceCenterNegI;
    cvf::Vec3d faceCenterCenterVectorJ = faceCenterPosJ - faceCenterNegJ;

    localZdirection.cross( faceCenterCenterVectorI, faceCenterCenterVectorJ );
    localZdirection.normalize();

    cvf::Vec3d crossPoductJZ;
    crossPoductJZ.cross( faceCenterCenterVectorJ, localZdirection );
    localXdirection = faceCenterCenterVectorI + crossPoductJZ;
    localXdirection.normalize();

    cvf::Vec3d crossPoductIZ;
    crossPoductIZ.cross( faceCenterCenterVectorI, localZdirection );
    localYdirection = faceCenterCenterVectorJ - crossPoductIZ;
    localYdirection.normalize();
}

// Convert opm to resinsight numbering of cornerpoints, see RigCellGeometryTools.cpp
std::array<cvf::Vec3d, 8> RigEclipseWellLogExtractor::hexCornersOpmToResinsight( std::size_t cellIndex ) const
{
    const auto[i,j,k] = m_grid.getIJK(cellIndex);
    std::array<std::size_t, 8> opm2resinsight = {0, 1, 3, 2, 4, 5, 7, 6};

    std::array<cvf::Vec3d, 8> hexCorners;
    for (std::size_t l = 0; l < 8; l++) {
         std::array<double, 3> cornerPointArray;
         cornerPointArray = m_grid.getCornerPos(i,j,k,l);
         hexCorners[opm2resinsight[l]]= cvf::Vec3d(cornerPointArray[0], cornerPointArray[1], cornerPointArray[2]);
    }
    return hexCorners;
}

// Modified version of ApplicationLibCode\ReservoirDataModel\RigMainGrid.cpp
cvf::BoundingBox RigEclipseWellLogExtractor::cellBoundingBox( std::size_t cellIndex ) const
{
    cvf::BoundingBox cellBB;
    for ( const auto& corner : hexCornersOpmToResinsight( cellIndex ) )
    {
        cellBB.add( corner );
    }
    return cellBB;
}

// Modified version of ApplicationLibCode\ReservoirDataModel\RigMainGrid.cpp
void RigEclipseWellLogExtractor::buildCellSearchTree()
{
    if (m_gridSearchData.cellSearchTree.isNull()) {

        auto nx = m_grid.getNX();
        auto ny = m_grid.getNY();
        auto nz = m_grid.getNZ();

        // Each leaf of the tree contains several cells along K. A tree with one cell per leaf becomes very deep for large
        // grids and is slow to build. For very large grids, increase the number of cells per leaf to limit the memory use.
        const std::size_t minCellsPerBoundingBox = 8;
        const double      maxNumberOfLeafNodes   = 4000000;
        const double      factor                 = std::ceil( static_cast<double>( nx * ny * nz ) / maxNumberOfLeafNodes );
        const std::size_t cellsPerBoundingBox    = std::max( minCellsPerBoundingBox, static_cast<std::size_t>( factor ) );

        std::vector<std::vector<int>> cellIndicesForBoundingBoxes;
        std::vector<cvf::BoundingBox> cellBoundingBoxes;

        for ( std::size_t i = 0; i < nx; i++ )
        {
            for ( std::size_t j = 0; j < ny; j++ )
            {
                std::size_t k = 0;
                while ( k < nz )
                {
                    std::size_t kCount = 0;

                    std::vector<int> aggregatedCellIndices;
                    cvf::BoundingBox accumulatedBB;

                    while ( ( kCount < cellsPerBoundingBox ) && ( k + kCount < nz ) )
                    {
                        std::size_t cellIdx = m_grid.getGlobalIndex( i, j, k + kCount );

                        cvf::BoundingBox cellBB = cellBoundingBox( cellIdx );
                        if ( cellBB.isValid() )
                        {
                            aggregatedCellIndices.push_back( static_cast<int>( cellIdx ) );
                            accumulatedBB.add( cellBB );
                        }
                        kCount++;
                    }

                    k += kCount;

                    cellIndicesForBoundingBoxes.emplace_back( std::move( aggregatedCellIndices ) );
                    cellBoundingBoxes.emplace_back( accumulatedBB );
                }
            }
        }

        m_gridSearchData.cellSearchTree = new cvf::BoundingBoxTree;
        m_gridSearchData.cellSearchTree->buildTreeFromBoundingBoxesOptimized( cellBoundingBoxes, cellIndicesForBoundingBoxes );
    }
}

// Modified version of ApplicationLibCode\ReservoirDataModel\RigMainGrid.cpp
std::vector<std::size_t> RigEclipseWellLogExtractor::findIntersectingCells( const cvf::BoundingBox& inputBB ) const
{
    CAF_ASSERT( m_gridSearchData.cellSearchTree.notNull() );

    std::vector<std::size_t> cellIndices;
    m_gridSearchData.cellSearchTree->findIntersections( inputBB, &cellIndices );

    // A leaf of the tree contains several cells. Remove the cells not intersecting the input bounding box, so
    // the result is the same as for a tree with one cell per leaf.
    std::erase_if( cellIndices, [&]( std::size_t cellIndex ) { return !cellBoundingBox( cellIndex ).intersects( inputBB ); } );

    return cellIndices;
}

// Modified version of ApplicationLibCode\ReservoirDataModel\RigEclipseWellLogExtractor.cpp
std::vector<std::size_t> RigEclipseWellLogExtractor::findCloseCellIndices( const cvf::BoundingBox& bb )
{
    return this->findIntersectingCells( bb );
}

// Modified version of ApplicationLibCode\ReservoirDataModel\RigEclipseWellLogExtractor.cpp
double RigEclipseWellLogExtractor::computeLengthThreshold() const
{
    // Default length tolerance for most common grid sizes
    double tolerance = 0.01;

    // For grids with very thin z-layers, reduce the tolerance to be able to find the intersections
    // If not, the intersection will be considered as non-valid cell edge intersection and discarded
    // https://github.com/OPM/ResInsight/issues/9244

    const std::size_t numActive = m_grid.getNumActive();
    if ( numActive > 0 )
    {
        double sumDZ = 0.0;
        for ( std::size_t activeIndex = 0; activeIndex < numActive; ++activeIndex )
        {
            sumDZ += std::abs( m_grid.getCellThickness( m_grid.getGlobalIndex( activeIndex ) ) );
        }
        const double averageDZ = sumDZ / numActive;

        const double scaleFactor = 0.05;
        tolerance                = std::min( tolerance, averageDZ * scaleFactor );
    }

    return tolerance;
}
 } //namespace external
