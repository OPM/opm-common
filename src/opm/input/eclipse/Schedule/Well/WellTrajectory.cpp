//header
//from RigMainGrid

#include <src/opm/input/eclipse/Schedule/Well/WellTrajectory.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <src/opm/input/eclipse/Schedule/WellTrajResInsight/LibGeometry/cvfBoundingBox.h>
#include <src/opm/input/eclipse/Schedule/WellTrajResInsight/LibGeometry/cvfBoundingBoxTree.h>

#include <vector>
#include <iostream>

void WellTrajectory::buildCellSearchTree()
{
    if (m_cellSearchTree.isNull()) {}
        std::cout<<m_coord[0]<<std::endl;
        // std::array<int, 3> dims = {3,3,3};
        // auto nx = m_grid.getNX();

        // const Opm::GridDims gridDims(3,3,3);
        Opm::EclipseGrid grid(m_dims, m_coord, m_zcorn, nullptr);

        auto nx = grid.getNX();
        auto ny = grid.getNY();
        auto nz = grid.getNZ();

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
                const auto[i,j,k] = grid.getIJK(cIdx);
                // for (std::size_t i = 0; i < nx; i++) {
                // for (std::size_t j = 0; j < ny; j++) {
                // for (std::size_t k = 0; k < nz; k++) {     
                cvf::BoundingBox cellBB;
                for (std::size_t l = 0; l < 8; l++) {
                     cornerPointArray = grid.getCornerPos(i,j,k,l);
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


void WellTrajectory::findIntersectingCells( const cvf::BoundingBox& inputBB, std::vector<size_t>* cellIndices ) const
{
    CVF_ASSERT( m_cellSearchTree.notNull() );

    m_cellSearchTree->findIntersections( inputBB, cellIndices );
}

std::vector<size_t> WellTrajectory::findCloseCellIndices( const cvf::BoundingBox& bb )
{
    std::vector<size_t> closeCells;
    this->findIntersectingCells( bb, &closeCells );
    return closeCells;
}

