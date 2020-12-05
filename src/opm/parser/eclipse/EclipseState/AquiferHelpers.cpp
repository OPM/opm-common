//
// Created by paean on 2020-10-09.
//

#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include "AquiferHelpers.hpp"

namespace Opm::AquiferHelpers {
    bool cellInsideReservoirAndActive(const EclipseGrid& grid, const int i, const int j, const int k, const std::vector<int>& actnum)
    {
        if ( i < 0 || j < 0 || k < 0
             || size_t(i) > grid.getNX() - 1
             || size_t(j) > grid.getNY() - 1
             || size_t(k) > grid.getNZ() - 1 )
        {
            return false;
        }

        const size_t globalIndex = grid.getGlobalIndex(i,j,k);
        return actnum[globalIndex];
    }

    bool neighborCellInsideReservoirAndActive(const EclipseGrid& grid,
                                              const int i, const int j, const int k, const Opm::FaceDir::DirEnum faceDir, const std::vector<int>& actnum)
    {
        switch(faceDir) {
            case FaceDir::XMinus:
                return cellInsideReservoirAndActive(grid, i - 1, j, k, actnum);
            case FaceDir::XPlus:
                return cellInsideReservoirAndActive(grid, i + 1, j, k, actnum);
            case FaceDir::YMinus:
                return cellInsideReservoirAndActive(grid, i, j - 1, k, actnum);
            case FaceDir::YPlus:
                return cellInsideReservoirAndActive(grid, i, j + 1, k, actnum);
            case FaceDir::ZMinus:
                return cellInsideReservoirAndActive(grid, i, j, k - 1, actnum);
            case FaceDir::ZPlus:
                return cellInsideReservoirAndActive(grid, i, j, k + 1, actnum);
            default:
                throw std::runtime_error("Unknown FaceDir enum " + std::to_string(faceDir));
        }
    }
}
