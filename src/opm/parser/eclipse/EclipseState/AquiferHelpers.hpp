//
// Created by paean on 2020-10-09.
//

#ifndef OPM_AQUIFERHELPERS_HPP
#define OPM_AQUIFERHELPERS_HPP

#include <opm/parser/eclipse/EclipseState/Grid/FaceDir.hpp>

namespace Opm {
    class EclipseGrid;
namespace AquiferHelpers {

    bool cellInsideReservoirAndActive(const EclipseGrid &grid, int i, int j, int k);
    bool neighborCellInsideReservoirAndActive(const EclipseGrid &grid, int i, int j, int k, FaceDir::DirEnum faceDir);
}
}

#endif //OPM_AQUIFERHELPERS_HPP
