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

#include "RigWellLogExtractor.h"
#include <src/opm/input/eclipse/Schedule/WellTrajResInsight/LibGeometry/cvfBoundingBoxTree.h>

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
class MyRigEclipseWellLogExtractor : public RigWellLogExtractor
{
public:
    MyRigEclipseWellLogExtractor( const RigWellPath*       wellpath );

    // void                      curveData( const RigResultAccessor* resultAccessor, std::vector<double>* values );
    // const RigEclipseCaseData* caseData() { return m_caseData.p(); }

private:
    void                calculateIntersection();
    std::vector<size_t> findCloseCellIndices( const cvf::BoundingBox& bb );
    cvf::Vec3d
        calculateLengthInCell( size_t cellIndex, const cvf::Vec3d& startPoint, const cvf::Vec3d& endPoint ) const override;

    cvf::Vec3d calculateLengthInCell( const std::array<cvf::Vec3d, 8>& hexCorners,
                                                                    const cvf::Vec3d&                startPoint,
                                                                    const cvf::Vec3d&                endPoint ) const;

    void findCellLocalXYZ( const std::array<cvf::Vec3d, 8>& hexCorners,
                                             cvf::Vec3d&                      localXdirection,
                                             cvf::Vec3d&                      localYdirection,
                                             cvf::Vec3d&                      localZdirection ) const;
    cvf::ref<cvf::BoundingBoxTree> m_cellSearchTree;
};
