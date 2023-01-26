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
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/Schedule/ScheduleGrid.hpp>

#include "MyRigEclipseWellLogExtractor.h"
#include "RigWellLogExtractionTools.h"
#include "RigWellPath.h"

#include "../LibGeometry/cvfBoundingBox.h"
#include "cvfGeometryTools.h"
#include "RigWellLogExtractor.h"
#include "RigCellGeometryTools.h"
#include "../CommonCode/cvfStructGrid.h"

#include <map>

//==================================================================================================
///
//==================================================================================================

 MyRigEclipseWellLogExtractor::MyRigEclipseWellLogExtractor(const RigWellPath* wellpath, const Opm::EclipseGrid& grid, cvf::ref<cvf::BoundingBoxTree>& cellSearchTree)
    : RigWellLogExtractor( wellpath, "" )
      ,m_grid(grid)
      ,m_cellSearchTree(cellSearchTree)
{
    calculateIntersection();
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
void MyRigEclipseWellLogExtractor::calculateIntersection()
{
    std::map<RigMDCellIdxEnterLeaveKey, HexIntersectionInfo> uniqueIntersections;

    bool isCellFaceNormalsOut = 1; //m_caseData->mainGrid()->isFaceNormalsOutwards();

    if ( m_wellPathGeometry->wellPathPoints().empty() ) return;

    for ( size_t wpp = 0; wpp < m_wellPathGeometry->wellPathPoints().size() - 1; ++wpp )
    {
        std::vector<HexIntersectionInfo> intersections;
        cvf::Vec3d                       p1 = m_wellPathGeometry->wellPathPoints()[wpp];
        cvf::Vec3d                       p2 = m_wellPathGeometry->wellPathPoints()[wpp + 1];

        cvf::BoundingBox bb;

        bb.add( p1 );
        bb.add( p2 );

        buildCellSearchTree();
        
        std::vector<size_t> closeCellIndices = findCloseCellIndices( bb );

        cvf::Vec3d hexCorners_opm[8]; //opm numbering
        std::array<double, 3> cornerPointArray;
        for ( const auto& globalCellIndex : closeCellIndices )
        {
             const auto[i,j,k] = m_grid.getIJK(globalCellIndex);
       
            for (std::size_t l = 0; l < 8; l++) {
                cornerPointArray = m_grid.getCornerPos(i,j,k,l);
                hexCorners_opm[l] = cvf::Vec3d(cornerPointArray[0], cornerPointArray[1], cornerPointArray[2]);
            }
            cvf::Vec3d hexCorners[8]; //resinsight numbering, see RigCellGeometryTools.cpp
            hexCorners[0] = hexCorners_opm[0];
            hexCorners[1] = hexCorners_opm[1];
            hexCorners[2] = hexCorners_opm[3];
            hexCorners[3] = hexCorners_opm[2];
            hexCorners[4] = hexCorners_opm[4];
            hexCorners[5] = hexCorners_opm[5];
            hexCorners[6] = hexCorners_opm[7];
            hexCorners[7] = hexCorners_opm[6];
            RigHexIntersectionTools::lineHexCellIntersection( p1, p2, hexCorners, globalCellIndex, &intersections );
        }

        if ( !isCellFaceNormalsOut )
        {
            for ( auto& intersection : intersections )
            {
                intersection.m_isIntersectionEntering = !intersection.m_isIntersectionEntering;
            }
        }

        // Now, with all the intersections of this piece of line, we need to
        // sort them in order, and set the measured depth and corresponding cell index

        // Inserting the intersections in this map will remove identical intersections
        // and sort them according to MD, CellIdx, Leave/enter

        double md1 = m_wellPathGeometry->measuredDepths()[wpp];
        double md2 = m_wellPathGeometry->measuredDepths()[wpp + 1];

        insertIntersectionsInMap( intersections, p1, md1, p2, md2, &uniqueIntersections );
    }

    this->populateReturnArrays( uniqueIntersections );
}


cvf::Vec3d MyRigEclipseWellLogExtractor::calculateLengthInCell( size_t            cellIndex,
                                                                const cvf::Vec3d& startPoint,
                                                                const cvf::Vec3d& endPoint ) const
{
    const auto[i,j,k] = m_grid.getIJK(cellIndex);
    cvf::Vec3d hexCorners_opm[8]; //opm numbering
    std::array<double, 3> cornerPointArray;
    for (std::size_t l = 0; l < 8; l++) {
         cornerPointArray = m_grid.getCornerPos(i,j,k,l);
         hexCorners_opm[l] = cvf::Vec3d(cornerPointArray[0], cornerPointArray[1], cornerPointArray[2]);
    } 
    std::array<cvf::Vec3d, 8> hexCorners;  //resinsight numbering, see RigCellGeometryTools.cpp
    hexCorners[0] = hexCorners_opm[0];
    hexCorners[1] = hexCorners_opm[1];
    hexCorners[2] = hexCorners_opm[3];
    hexCorners[3] = hexCorners_opm[2];
    hexCorners[4] = hexCorners_opm[4];
    hexCorners[5] = hexCorners_opm[5];
    hexCorners[6] = hexCorners_opm[7];
    hexCorners[7] = hexCorners_opm[6];

    return MyRigEclipseWellLogExtractor::calculateLengthInCell( hexCorners, startPoint, endPoint );
}


cvf::Vec3d MyRigEclipseWellLogExtractor::calculateLengthInCell( const std::array<cvf::Vec3d, 8>& hexCorners,
                                                                const cvf::Vec3d&                startPoint,
                                                                const cvf::Vec3d&                endPoint ) const
{

    cvf::Vec3d vec = endPoint - startPoint;
    cvf::Vec3d iAxisDirection;
    cvf::Vec3d jAxisDirection;
    cvf::Vec3d kAxisDirection;

    MyRigEclipseWellLogExtractor::findCellLocalXYZ( hexCorners, iAxisDirection, jAxisDirection, kAxisDirection );

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

//==================================================================================================
///
//==================================================================================================
void MyRigEclipseWellLogExtractor::findCellLocalXYZ( const std::array<cvf::Vec3d, 8>& hexCorners,
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

    // TODO: Should we use face centroids instead of face centers?          

    // cvf::ubyte                         faceVertexIndices[4];
    // cvf::StructGridInterface::FaceEnum face;

    // face = cvf::StructGridInterface::NEG_I;
    // cvf::StructGridInterface::cellFaceVertexIndices( face, faceVertexIndices );
    // cvf::Vec3d faceCenterNegI = cvf::GeometryTools::computeFaceCenter( hexCorners[faceVertexIndices[0]],
    //                                                                    hexCorners[faceVertexIndices[1]],
    //                                                                    hexCorners[faceVertexIndices[2]],
    //                                                                    hexCorners[faceVertexIndices[3]] );
    // TODO: Should we use face centroids instead of face centers?

    // face = cvf::StructGridInterface::POS_I;
    // cvf::StructGridInterface::cellFaceVertexIndices( face, faceVertexIndices );
    // cvf::Vec3d faceCenterPosI = cvf::GeometryTools::computeFaceCenter( hexCorners[faceVertexIndices[0]],
    //                                                                    hexCorners[faceVertexIndices[1]],
    //                                                                    hexCorners[faceVertexIndices[2]],
    //                                                                    hexCorners[faceVertexIndices[3]] );

    // face = cvf::StructGridInterface::NEG_J;
    // cvf::StructGridInterface::cellFaceVertexIndices( face, faceVertexIndices );
    // cvf::Vec3d faceCenterNegJ = cvf::GeometryTools::computeFaceCenter( hexCorners[faceVertexIndices[0]],
    //                                                                    hexCorners[faceVertexIndices[1]],
    //                                                                    hexCorners[faceVertexIndices[2]],
    //                                                                    hexCorners[faceVertexIndices[3]] );

    // face = cvf::StructGridInterface::POS_J;
    // cvf::StructGridInterface::cellFaceVertexIndices( face, faceVertexIndices );
    // cvf::Vec3d faceCenterPosJ = cvf::GeometryTools::computeFaceCenter( hexCorners[faceVertexIndices[0]],
    //                                                                    hexCorners[faceVertexIndices[1]],
    //                                                                    hexCorners[faceVertexIndices[2]],
    //                                                                    hexCorners[faceVertexIndices[3]] );

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

void MyRigEclipseWellLogExtractor::buildCellSearchTree()
{
    if (m_cellSearchTree.isNull()) {

        auto nx = m_grid.getNX();
        auto ny = m_grid.getNY();
        auto nz = m_grid.getNZ();

        size_t cellCount = nx * ny * nz;
        std::vector<size_t>           cellIndicesForBoundingBoxes;
        std::vector<cvf::BoundingBox> cellBoundingBoxes;

// #pragma omp parallel
//         {
            size_t threadCellCount = cellCount;

            std::vector<size_t>           threadIndicesForBoundingBoxes;
            std::vector<cvf::BoundingBox> threadBoundingBoxes;

            threadIndicesForBoundingBoxes.reserve( threadCellCount );
            threadBoundingBoxes.reserve( threadCellCount );

            std::array<double, 3> cornerPointArray;
            cvf::Vec3d cornerPoint;

// #pragma omp for
            for (int cIdx = 0; cIdx < (int)cellCount; ++cIdx) {
                const auto[i,j,k] = m_grid.getIJK(cIdx);
                cvf::BoundingBox cellBB;
                for (std::size_t l = 0; l < 8; l++) {
                     cornerPointArray = m_grid.getCornerPos(i,j,k,l);
                     cornerPoint = cvf::Vec3d(cornerPointArray[0], cornerPointArray[1], cornerPointArray[2]);
                     cellBB.add(cornerPoint);
                }

                if (cellBB.isValid()) {
                    threadIndicesForBoundingBoxes.emplace_back(cIdx);
                    threadBoundingBoxes.emplace_back(cellBB);
                }
            }

            threadIndicesForBoundingBoxes.shrink_to_fit();
            threadBoundingBoxes.shrink_to_fit();
// #pragma omp critical
//          {
             cellIndicesForBoundingBoxes.insert( cellIndicesForBoundingBoxes.end(),
                                                 threadIndicesForBoundingBoxes.begin(),
                                                 threadIndicesForBoundingBoxes.end() );

             cellBoundingBoxes.insert( cellBoundingBoxes.end(), threadBoundingBoxes.begin(), threadBoundingBoxes.end() );
        //  }
    // } #pragma omp parallel
         m_cellSearchTree = new cvf::BoundingBoxTree;
         m_cellSearchTree->buildTreeFromBoundingBoxes( cellBoundingBoxes, &cellIndicesForBoundingBoxes );
    }
}

void MyRigEclipseWellLogExtractor::computeCachedData()
{
    // initAllSubGridsParentGridPointer();
    // initAllSubCellsMainGridCellIndex();

    m_cellSearchTree = nullptr;
    buildCellSearchTree();
}

void MyRigEclipseWellLogExtractor::findIntersectingCells( const cvf::BoundingBox& inputBB, std::vector<size_t>* cellIndices ) const
{
    CVF_ASSERT( m_cellSearchTree.notNull() );

    m_cellSearchTree->findIntersections( inputBB, cellIndices );
}

std::vector<size_t> MyRigEclipseWellLogExtractor::findCloseCellIndices( const cvf::BoundingBox& bb )
{
    std::vector<size_t> closeCells;
    this->findIntersectingCells( bb, &closeCells );
    return closeCells;
}

 cvf::ref<cvf::BoundingBoxTree> MyRigEclipseWellLogExtractor::getCellSearchTree()
 {
    return m_cellSearchTree;
 }
