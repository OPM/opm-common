/*
  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 2 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.

  Consult the COPYING file in the top-level source directory of this
  module for the precise wording of the license and the list of
  copyright holders.
*/
/*!
 * \file
 * \copydoc Opm::EclEpsTwoPhaseLawPoints
 */
#ifndef OPM_ECL_EPS_GRID_PROPERTIES_HPP
#define OPM_ECL_EPS_GRID_PROPERTIES_HPP

#include <opm/material/fluidmatrixinteractions/EclEpsConfig.hpp>

#include <cstddef>
#include <vector>

namespace Opm {

#if HAVE_ECL_INPUT
class EclipseState;
#endif

/*!
 * \brief Collects all grid properties which are relevant for end point scaling.
 *
 * This class is used for both, the drainage and the imbibition variants of the ECL
 * keywords.
 */

class EclEpsGridProperties
{

public:
#if HAVE_ECL_INPUT
    EclEpsGridProperties(const EclipseState& eclState,
                         bool useImbibition);
#endif

    int satRegion(const std::size_t active_index) const
    {
        return (*this->satnum_)[active_index] - 1;
    }

    double permx(const std::size_t active_index) const
    {
        return this->perm(this->permx_, active_index);
    }

    double permy(const std::size_t active_index) const
    {
        return this->perm(this->permy_, active_index);
    }

    double permz(const std::size_t active_index) const
    {
        return this->perm(this->permy_, active_index);
    }

    double poro(const std::size_t active_index) const
    {
        return (*this->poro_)[active_index];
    }

    const double* swl(const std::size_t active_index) const
    {
        return this->satfunc(this->swl_, active_index);
    }

    const double* sgl(const std::size_t active_index) const
    {
        return this->satfunc(this->sgl_, active_index);
    }

    const double* swcr(const std::size_t active_index) const
    {
        return this->satfunc(this->swcr_, active_index);
    }

    const double* sgcr(const std::size_t active_index) const
    {
        return this->satfunc(this->sgcr_, active_index);
    }

    const double* sowcr(const std::size_t active_index) const
    {
        return this->satfunc(this->sowcr_, active_index);
    }

    const double* sogcr(const std::size_t active_index) const
    {
        return this->satfunc(this->sogcr_, active_index);
    }

    const double* swu(const std::size_t active_index) const
    {
        return this->satfunc(this->swu_, active_index);
    }

    const double* sgu(const std::size_t active_index) const
    {
        return this->satfunc(this->sgu_, active_index);
    }

    const double* pcw(const std::size_t active_index) const
    {
        return this->satfunc(this->pcw_, active_index);
    }

    const double* pcg(const std::size_t active_index) const
    {
        return this->satfunc(this->pcg_, active_index);
    }

    const double* krw(const std::size_t active_index) const
    {
        return this->satfunc(this->krw_, active_index);
    }

    const double* krwr(const std::size_t active_index) const
    {
        return this->satfunc(this->krwr_, active_index);
    }

    const double* krg(const std::size_t active_index) const
    {
        return this->satfunc(this->krg_, active_index);
    }

    const double* krgr(const std::size_t active_index) const
    {
        return this->satfunc(this->krgr_, active_index);
    }

    const double* kro(const std::size_t active_index) const
    {
        return this->satfunc(this->kro_, active_index);
    }

    const double* krorg(const std::size_t active_index) const
    {
        return this->satfunc(this->krorg_, active_index);
    }

    const double* krorw(const std::size_t active_index) const
    {
        return this->satfunc(this->krorw_, active_index);
    }

private:
    const std::vector<int>* satnum_ { nullptr };

    const std::vector<double>* swl_ { nullptr };
    const std::vector<double>* sgl_ { nullptr };
    const std::vector<double>* swcr_ { nullptr };
    const std::vector<double>* sgcr_ { nullptr };
    const std::vector<double>* sowcr_ { nullptr };
    const std::vector<double>* sogcr_ { nullptr };
    const std::vector<double>* swu_ { nullptr };
    const std::vector<double>* sgu_ { nullptr };

    const std::vector<double>* pcw_ { nullptr };
    const std::vector<double>* pcg_ { nullptr };

    const std::vector<double>* krw_ { nullptr };
    const std::vector<double>* krwr_ { nullptr };
    const std::vector<double>* kro_ { nullptr };
    const std::vector<double>* krorg_ { nullptr };
    const std::vector<double>* krorw_ { nullptr };
    const std::vector<double>* krg_ { nullptr };
    const std::vector<double>* krgr_ { nullptr };

    const std::vector<double>* permx_ { nullptr };
    const std::vector<double>* permy_ { nullptr };
    const std::vector<double>* permz_ { nullptr };
    const std::vector<double>* poro_ { nullptr };

    const double*
    satfunc(const std::vector<double>* data,
            const std::size_t          active_index) const
    {
        return ((data == nullptr) || data->empty())
            ? nullptr
            : &(*data)[active_index];
    }

    double perm(const std::vector<double>* data,
                const std::size_t          active_index) const
    {
        return ((data == nullptr) || data->empty())
            ? 0.0
            : (*data)[active_index];
    }
};

} // namespace Opm

#endif
