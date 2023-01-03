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
#include <src/opm/input/eclipse/Schedule/Well/WellTrajectory.hpp>

#include "MyRigEclipseWellLogExtractor.h"

// #include "RiaLogging.h"

// #include "RigEclipseCaseData.h"
//#include "RigMainGrid.h"
// #include "RigResultAccessor.h"
#include "RigWellLogExtractionTools.h"
#include "RigWellPath.h"
// #include "RigWellPathIntersectionTools.h"

#include "../LibGeometry/cvfBoundingBox.h"
#include "cvfGeometryTools.h"
#include "RigWellLogExtractor.h"
#include "RigCellGeometryTools.h"
#include "../CommonCode/cvfStructGrid.h"

#include <map>

//==================================================================================================
///
//==================================================================================================

MyRigEclipseWellLogExtractor::MyRigEclipseWellLogExtractor( const RigWellPath*       wellpath)
    : RigWellLogExtractor( wellpath, "" )
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

        
        std::array<int, 3> dims = {3,3,1};
        Opm::EclipseGrid grid(dims[0], dims[1], dims[2], 10, 10, 5);
        std::vector<double> coord;
        coord = grid.getCOORD();
        std::vector<double> zcorn;
        zcorn= grid.getZCORN();
   
        WellTrajectory m_wellTrajectory(dims,coord, zcorn);
        m_wellTrajectory.buildCellSearchTree();
        std::vector<size_t> closeCellIndices = m_wellTrajectory.findCloseCellIndices( bb );

        cvf::Vec3d hexCorners_opm[8]; //opm numbering
        std::array<double, 3> cornerPointArray;
        for ( const auto& globalCellIndex : closeCellIndices )
        {
             const auto[i,j,k] = grid.getIJK(globalCellIndex);
       
            for (std::size_t l = 0; l < 8; l++) {
                cornerPointArray = grid.getCornerPos(i,j,k,l);
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

    // if ( uniqueIntersections.empty() && m_wellPathGeometry->wellPathPoints().size() > 1 )
    // {
    //     // When entering this function, all well path points are either completely outside the grid
    //     // or all well path points are inside one cell

    //     cvf::Vec3d firstPoint = m_wellPathGeometry->wellPathPoints().front();
    //     cvf::Vec3d lastPoint  = m_wellPathGeometry->wellPathPoints().back();

    //     {
    //         cvf::BoundingBox bb;
    //         bb.add( firstPoint );

    //         std::vector<size_t> closeCellIndices = findCloseCellIndices( bb );

    //         cvf::Vec3d hexCorners[8];
    //         for ( const auto& globalCellIndex : closeCellIndices )
    //         {
    //             const RigCell& cell = m_caseData->mainGrid()->globalCellArray()[globalCellIndex];

    //             if ( cell.isInvalid() ) continue;

    //             m_caseData->mainGrid()->cellCornerVertices( globalCellIndex, hexCorners );

    //             if ( RigHexIntersectionTools::isPointInCell( firstPoint, hexCorners ) )
    //             {
    //                 if ( RigHexIntersectionTools::isPointInCell( lastPoint, hexCorners ) )
    //                 {
    //                     {
    //                         // Mark the first well path point as entering the cell

    //                         bool                      isEntering = true;
    //                         HexIntersectionInfo       info( firstPoint,
    //                                                   isEntering,
    //                                                   cvf::StructGridInterface::NO_FACE,
    //                                                   globalCellIndex );
    //                         RigMDCellIdxEnterLeaveKey enterLeaveKey( m_wellPathGeometry->measuredDepths().front(),
    //                                                                  globalCellIndex,
    //                                                                  isEntering );

    //                         uniqueIntersections.insert( std::make_pair( enterLeaveKey, info ) );
    //                     }

    //                     {
    //                         // Mark the last well path point as leaving cell

    //                         bool                isEntering = false;
    //                         HexIntersectionInfo info( lastPoint, isEntering, cvf::StructGridInterface::NO_FACE, globalCellIndex );
    //                         RigMDCellIdxEnterLeaveKey enterLeaveKey( m_wellPathGeometry->measuredDepths().back(),
    //                                                                  globalCellIndex,
    //                                                                  isEntering );

    //                         uniqueIntersections.insert( std::make_pair( enterLeaveKey, info ) );
    //                     }
    //                 }
    //                 else
    //                 {
    //                     // QString txt =
    //                     //     "Detected two points assumed to be in the same cell, but they are in two different cells";
    //                     // RiaLogging::debug( txt );
    //                     std::cout << "error message \n"<< std::endl;
    //                 }
    //             }
    //         }
    //     }
    // }

    this->populateReturnArrays( uniqueIntersections );
}

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
// void RigEclipseWellLogExtractor::curveData( const RigResultAccessor* resultAccessor, std::vector<double>* values )
// {
//     CVF_TIGHT_ASSERT( values );
//     values->resize( m_intersections.size() );

//     for ( size_t cpIdx = 0; cpIdx < m_intersections.size(); ++cpIdx )
//     {
//         size_t                             cellIdx  = m_intersectedCellsGlobIdx[cpIdx];
//         cvf::StructGridInterface::FaceType cellFace = m_intersectedCellFaces[cpIdx];
//         ( *values )[cpIdx]                          = resultAccessor->cellFaceScalarGlobIdx( cellIdx, cellFace );
//     }
// }

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
// std::vector<size_t> RigEclipseWellLogExtractor::findCloseCellIndices( const cvf::BoundingBox& bb )
// {
//     std::vector<size_t> closeCells;
//     m_caseData->mainGrid()->findIntersectingCells( bb, &closeCells );
//     return closeCells;
// }

//--------------------------------------------------------------------------------------------------
///
//--------------------------------------------------------------------------------------------------
// cvf::Vec3d MyRigEclipseWellLogExtractor::calculateLengthInCell( size_t            cellIndex,
//                                                               const cvf::Vec3d& startPoint,
//                                                               const cvf::Vec3d& endPoint ) const
// {
//     std::array<cvf::Vec3d, 8> hexCorners;
//     // m_caseData->mainGrid()->cellCornerVertices( cellIndex, hexCorners.data() );

//     return RigWellPathIntersectionTools::calculateLengthInCell( hexCorners, startPoint, endPoint );
// }


cvf::Vec3d MyRigEclipseWellLogExtractor::calculateLengthInCell( size_t            cellIndex,
                                                                const cvf::Vec3d& startPoint,
                                                                const cvf::Vec3d& endPoint ) const
{
    std::array<cvf::Vec3d, 8> hexCorners;
    //Todo calculate here the hexcorners
    // m_caseData->mainGrid()->cellCornerVertices( cellIndex, hexCorners.data() );

    return MyRigEclipseWellLogExtractor::calculateLengthInCell( hexCorners, startPoint, endPoint );
}


cvf::Vec3d MyRigEclipseWellLogExtractor::calculateLengthInCell( const std::array<cvf::Vec3d, 8>& hexCorners,
                                                                const cvf::Vec3d&                startPoint,
                                                                const cvf::Vec3d&                endPoint ) const
{
    //TODO rewrite to avoid to many dependencyies needed for findCellLocalXYZ

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
//     cvf::ubyte                         faceVertexIndices[4];
//     cvf::StructGridInterface::FaceEnum face;

//     face = cvf::StructGridInterface::NEG_I;
//     cvf::StructGridInterface::cellFaceVertexIndices( face, faceVertexIndices );
//     cvf::Vec3d faceCenterNegI = cvf::GeometryTools::computeFaceCenter( hexCorners[faceVertexIndices[0]],
//                                                                        hexCorners[faceVertexIndices[1]],
//                                                                        hexCorners[faceVertexIndices[2]],
//                                                                        hexCorners[faceVertexIndices[3]] );
//     // TODO: Should we use face centroids instead of face centers?

//     face = cvf::StructGridInterface::POS_I;
//     cvf::StructGridInterface::cellFaceVertexIndices( face, faceVertexIndices );
//     cvf::Vec3d faceCenterPosI = cvf::GeometryTools::computeFaceCenter( hexCorners[faceVertexIndices[0]],
//                                                                        hexCorners[faceVertexIndices[1]],
//                                                                        hexCorners[faceVertexIndices[2]],
//                                                                        hexCorners[faceVertexIndices[3]] );

//     face = cvf::StructGridInterface::NEG_J;
//     cvf::StructGridInterface::cellFaceVertexIndices( face, faceVertexIndices );
//     cvf::Vec3d faceCenterNegJ = cvf::GeometryTools::computeFaceCenter( hexCorners[faceVertexIndices[0]],
//                                                                        hexCorners[faceVertexIndices[1]],
//                                                                        hexCorners[faceVertexIndices[2]],
//                                                                        hexCorners[faceVertexIndices[3]] );

//     face = cvf::StructGridInterface::POS_J;
//     cvf::StructGridInterface::cellFaceVertexIndices( face, faceVertexIndices );
//     cvf::Vec3d faceCenterPosJ = cvf::GeometryTools::computeFaceCenter( hexCorners[faceVertexIndices[0]],
//                                                                        hexCorners[faceVertexIndices[1]],
//                                                                        hexCorners[faceVertexIndices[2]],
//                                                                        hexCorners[faceVertexIndices[3]] );

//     cvf::Vec3d faceCenterCenterVectorI = faceCenterPosI - faceCenterNegI;
//     cvf::Vec3d faceCenterCenterVectorJ = faceCenterPosJ - faceCenterNegJ;

//     localZdirection.cross( faceCenterCenterVectorI, faceCenterCenterVectorJ );
//     localZdirection.normalize();

//     cvf::Vec3d crossPoductJZ;
//     crossPoductJZ.cross( faceCenterCenterVectorJ, localZdirection );
//     localXdirection = faceCenterCenterVectorI + crossPoductJZ;
//     localXdirection.normalize();

//     cvf::Vec3d crossPoductIZ;
//     crossPoductIZ.cross( faceCenterCenterVectorI, localZdirection );
//     localYdirection = faceCenterCenterVectorJ - crossPoductIZ;
//     localYdirection.normalize();

//     // TODO: Check if we end up with 0-vectors, and handle this case...
    localXdirection = {1,0,0};
    localYdirection = {0,1,0};
    localZdirection = {0,0,1};

}

