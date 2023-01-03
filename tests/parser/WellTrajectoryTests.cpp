/*
  Copyright ....
*/
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <src/opm/input/eclipse/Schedule/Well/WellTrajectory.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <iostream>

#include <src/opm/input/eclipse/Schedule/WellTrajResInsight/LibCore/cvfVector3.h>
#include <src/opm/input/eclipse/Schedule/WellTrajResInsight/LibGeometry/cvfBoundingBox.h>
#include <src/opm/input/eclipse/Schedule/WellTrajResInsight/LibGeometry/cvfBoundingBoxTree.h>
#include <src/opm/input/eclipse/Schedule/WellTrajResInsight/ReservoirDataModel/RigHexIntersectionTools.h>
#include <src/opm/input/eclipse/Schedule/WellTrajResInsight/ReservoirDataModel/RigWellLogExtractionTools.h>
#include <src/opm/input/eclipse/Schedule/WellTrajResInsight/ReservoirDataModel/RigWellLogExtractor.h>
#include <src/opm/input/eclipse/Schedule/WellTrajResInsight/ReservoirDataModel/RigWellPath.h>
#include <src/opm/input/eclipse/Schedule/WellTrajResInsight/ReservoirDataModel/MyRigEclipseWellLogExtractor.h>


// using namespace Opm;
// namespace {

// EclipseGrid makeGrid()
// {
//     EclipseGrid grid(3, 3, 2, 10, 10, 5);

//     // std::vector<int> actnum(27, 1);
//     // for (const std::size_t layer : { 0, 1, 2 })
//     //     actnum[grid.getGlobalIndex(0, 0, layer)] = 0;

//     // grid.resetACTNUM(actnum);
//     return grid;
// }
// } 

// class Test : public RigWellLogExtractor {
//     public :
//         // static void insertIntersectionsInMap(const std::vector<HexIntersectionInfo>&, cvf::Vec3d, double, cvf::Vec3d, double, std::map<RigMDCellIdxEnterLeaveKey, HexIntersectionInfo>*)
//       static void insertIntersectionsInMap1() { RigWellLogExtractor::insertIntersectionsInMap(const std::vector<HexIntersectionInfo>&, cvf::Vec3d, double, cvf::Vec3d, double, std::map<RigMDCellIdxEnterLeaveKey, HexIntersectionInfo>);}
// };

int main(int argc, char** argv)
{
    cvf::Vec3d p1(1,1,2);
    cvf::Vec3d p2(11,11,2);

    double  md1(0);
    double  md2(11);

    std::cout << "test \n"<< std::endl;
    // const auto& grid = makeGrid();

    // auto dim = grid.getNXYZ();
    // size_t idx = grid.getGlobalIndex(1, 1, 1);

    cvf::BoundingBox bb;
    bb.add( p1 );
    bb.add( p2 );

    std::vector<cvf::BoundingBox> bbs;
    bbs.push_back(cvf::BoundingBox(cvf::Vec3d(0,0,0), cvf::Vec3d(1,1,1)));
    bbs.push_back(cvf::BoundingBox(cvf::Vec3d(1,0,0), cvf::Vec3d(2,1,1)));
    bbs.push_back(cvf::BoundingBox(cvf::Vec3d(2,0,0), cvf::Vec3d(3,1,1)));
    bbs.push_back(cvf::BoundingBox(cvf::Vec3d(3,0,0), cvf::Vec3d(4,1,1)));
    bbs.push_back(cvf::BoundingBox(cvf::Vec3d(4,0,0), cvf::Vec3d(5,1,1)));
    bbs.push_back(cvf::BoundingBox(cvf::Vec3d(0.5,0.5,0), cvf::Vec3d(5.5,1.5,1)));

    std::vector<size_t> ids;
    ids.push_back(10);
    ids.push_back(11);
    ids.push_back(12);
    ids.push_back(13);
    ids.push_back(14);
    ids.push_back(15);

    // cvf::BoundingBoxTree bbtree;
    // bbtree.buildTreeFromBoundingBoxes(bbs, &ids);
    // std::vector<size_t> intIds;
    // bbtree.findIntersections(cvf::BoundingBox(cvf::Vec3d(0.25,0.25,0.25), cvf::Vec3d(4.5,0.4,0.4)), &intIds);

    // cvf::ref<cvf::BoundingBoxTree> m_cellSearchTree;
    // m_cellSearchTree = nullptr;
    // m_cellSearchTree = new cvf::BoundingBoxTree;
    // m_cellSearchTree->buildTreeFromBoundingBoxes( bbs, nullptr );
 
    // std::array<int, 3> dims = {3,3,1};
    // Opm::EclipseGrid grid(dims[0], dims[1], dims[2], 10, 10, 5);
    // std::vector<double> coord;
    // coord = grid.getCOORD();
    // std::vector<double> zcorn;
    // zcorn= grid.getZCORN();
   
    // WellTrajectory m_wellTrajectory(dims,coord, zcorn);
    // m_wellTrajectory.buildCellSearchTree();

  

    //------------------------------------------------------
    // std::vector<size_t> closeCellIndices =  m_wellTrajectory.findCloseCellIndices( bb );

    // std::array<double, 3> cornerPointArray;
    // std::vector<HexIntersectionInfo> intersections;
    //  cvf::Vec3d hexCorners_opm[8]; //opm numbering
    // for ( const auto& globalCellIndex : closeCellIndices ) {
    //     const auto[i,j,k] = grid.getIJK(globalCellIndex);
       
    //     for (std::size_t l = 0; l < 8; l++) {
    //          cornerPointArray = grid.getCornerPos(i,j,k,l);
    //          hexCorners_opm[l] = cvf::Vec3d(cornerPointArray[0], cornerPointArray[1], cornerPointArray[2]);
    //     }
    //     cvf::Vec3d hexCorners[8]; //resinsight numbering, see ApplicationLibCode\ReservoirDataModel\RigCellGeometryTools.cpp
    //     hexCorners[0] = hexCorners_opm[0];
    //     hexCorners[1] = hexCorners_opm[1];
    //     hexCorners[2] = hexCorners_opm[3];
    //     hexCorners[3] = hexCorners_opm[2];
    //     hexCorners[4] = hexCorners_opm[4];
    //     hexCorners[5] = hexCorners_opm[5];
    //     hexCorners[6] = hexCorners_opm[7];
    //     hexCorners[7] = hexCorners_opm[6];

    //     RigHexIntersectionTools::lineHexCellIntersection( p1, p2, hexCorners, globalCellIndex, &intersections );
    // }

    // for (size_t intIdx = 0; intIdx < intersections.size(); ++intIdx) {
    //     intersections[intIdx].m_isIntersectionEntering =
    //         !intersections[intIdx].m_isIntersectionEntering;
    // }

    // std::map<RigMDCellIdxEnterLeaveKey, HexIntersectionInfo> uniqueIntersections;

    // RigWellLogExtractor::insertIntersectionsInMap( intersections, p1, md1, p2, md2, &uniqueIntersections );
    //----------------------------------------------------------------------------------------------------------


    std::vector<cvf::Vec3d> points = {p1, p2};
    std::vector<double> md = {md1, md2};
    RigWellPath m_wellPath(points, md);
    // RigWellPath*  wellPathGeometry = m_wellPath->wellPathGeometry();
    // int ii = m_wellPath->m_wellPathGeometry.size();
    
   
    cvf::ref<RigWellPath> wellPathGeometry = new RigWellPath;
    wellPathGeometry->setWellPathPoints( points );
    wellPathGeometry->setMeasuredDepths( md );
    cvf::ref<MyRigEclipseWellLogExtractor> e = new MyRigEclipseWellLogExtractor( wellPathGeometry.p());
    auto intersections = e->cellIntersectionInfosAlongWellPath();

    // m_wellPathGeometry.setDatumElevation(100.0);
    // RigWellPath*  wellPathGeometry1 = nullptr;
    // const std::string&   wellCaseErrorMsgName ="";

    // RigWellLogExtractor rwle( wellPathGeometry.p() , "");
    // rwle.populateReturnArrays(uniqueIntersections);

    std::cout << "end \n"<< std::endl;
}
