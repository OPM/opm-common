//header
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <src/opm/input/eclipse/Schedule/WellTrajResInsight/LibCore/cvfVector3.h>
#include <src/opm/input/eclipse/Schedule/WellTrajResInsight/LibGeometry/cvfBoundingBoxTree.h>
#include <vector>

#ifndef WELLTRAJECTORY_HPP_
#define WELLTRAJECTORY_HPP_

class EclipseGrid;
class WellTrajectory {

    public:
        WellTrajectory(std::array<int, 3> dims, std::vector<double> coord, std::vector<double> zcorn)
        //WellTrajectory(const double* coord, const double* zcorn) 
        : m_dims(dims)
        , m_coord(coord)
        , m_zcorn(zcorn)
        {};

        void buildCellSearchTree();
        void findIntersectingCells( const cvf::BoundingBox& inputBB, std::vector<size_t>* cellIndices ) const;
        std::vector<size_t> findCloseCellIndices( const cvf::BoundingBox& bb );


    private:
    // std::vector<cvf::Vec3d>       m_nodes; ///< Global vertex table
    std::array<int, 3>            m_dims;
    std::vector<double>           m_coord;
    std::vector<double>           m_zcorn;
    cvf::ref<cvf::BoundingBoxTree> m_cellSearchTree;
    
};

#endif /* WELLTRAJECTORY_HPP_ */