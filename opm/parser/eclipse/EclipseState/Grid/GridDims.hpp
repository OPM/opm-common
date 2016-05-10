/*
 * GridDims.hpp
 *
 *  Created on: May 10, 2016
 *      Author: pgdr
 */

#ifndef OPM_PARSER_GRIDDIMS_HPP_
#define OPM_PARSER_GRIDDIMS_HPP_

#include <ert/ecl/ecl_grid.h>
#include <ert/util/ert_unique_ptr.hpp>

namespace Opm {
    class GridDims
    {
    public:

        GridDims() { }

        GridDims(size_t nx, size_t ny, size_t nz, double dx = 1.0, double dy = 1.0, double dz = 1.0) :
                m_nx(nx), m_ny(ny), m_nz(nz)
        {
            m_grid.reset(ecl_grid_alloc_rectangular(nx, ny, nz, dx, dy, dz, nullptr));
        }

        size_t getNX() const {
            return m_nx;
        }

        size_t getNY() const {
            return m_ny;
        }

        size_t getNZ() const {
            return m_nz;
        }

        std::array<int, 3> getNXYZ() const {
            return { {int( m_nx ), int( m_ny ), int( m_nz )}};
        }

        size_t getGlobalIndex(size_t i, size_t j, size_t k) const {
            return (i + j * getNX() + k * getNX() * getNY());
        }

        std::array<int, 3> getIJK(size_t globalIndex) const {
            std::array<int, 3> r = { { 0, 0, 0 } };
            int k = globalIndex / (getNX() * getNY());
            globalIndex -= k * (getNX() * getNY());
            int j = globalIndex / getNX();
            globalIndex -= j * getNX();
            int i = globalIndex;
            r[0] = i;
            r[1] = j;
            r[2] = k;
            return r;
        }

        size_t getCartesianSize() const {
            return m_nx * m_ny * m_nz;
        }

        void assertGlobalIndex(size_t globalIndex) const {
            if (globalIndex >= getCartesianSize())
                throw std::invalid_argument("input index above valid range");
        }

        void assertIJK(size_t i, size_t j, size_t k) const {
            if (i >= getNX() || j >= getNY() || k >= getNZ())
                throw std::invalid_argument("input index above valid range");
        }

    protected:
        ERT::ert_unique_ptr<ecl_grid_type, ecl_grid_free> m_grid;
        size_t m_nx;
        size_t m_ny;
        size_t m_nz;
    };
}

#endif /* OPM_PARSER_GRIDDIMS_HPP_ */
