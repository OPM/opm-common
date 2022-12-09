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

    unsigned satRegion(std::size_t active_index) const {
        return this->compressed_satnum[active_index] - 1;
    }

    double permx(std::size_t active_index) const {
        return this->compressed_permx[active_index];
    }

    double permy(std::size_t active_index) const {
        return this->compressed_permy[active_index];
    }

    double permz(std::size_t active_index) const {
        return this->compressed_permz[active_index];
    }

    double poro(std::size_t active_index) const {
        return this->compressed_poro[active_index];
    }

    const double * swl(std::size_t active_index) const {
        return this->satfunc(this->compressed_swl, active_index);
    }

    const double * sgl(std::size_t active_index) const {
        return this->satfunc(this->compressed_sgl, active_index);
    }

    const double * swcr(std::size_t active_index) const {
        return this->satfunc(this->compressed_swcr, active_index);
    }

    const double * sgcr(std::size_t active_index) const {
        return this->satfunc(this->compressed_sgcr, active_index);
    }

    const double * sowcr(std::size_t active_index) const {
        return this->satfunc(this->compressed_sowcr, active_index);
    }

    const double * sogcr(std::size_t active_index) const {
        return this->satfunc(this->compressed_sogcr, active_index);
    }

    const double * swu(std::size_t active_index) const {
        return this->satfunc(this->compressed_swu, active_index);
    }

    const double * sgu(std::size_t active_index) const {
        return this->satfunc(this->compressed_sgu, active_index);
    }

    const double * pcw(std::size_t active_index) const {
        return this->satfunc(this->compressed_pcw, active_index);
    }

    const double * pcg(std::size_t active_index) const {
        return this->satfunc(this->compressed_pcg, active_index);
    }

    const double * krw(std::size_t active_index) const {
        return this->satfunc(this->compressed_krw, active_index);
    }

    const double * krwr(std::size_t active_index) const {
        return this->satfunc(this->compressed_krwr, active_index);
    }

    const double * krg(std::size_t active_index) const {
        return this->satfunc(this->compressed_krg, active_index);
    }

    const double * krgr(std::size_t active_index) const {
        return this->satfunc(this->compressed_krgr, active_index);
    }

    const double * kro(std::size_t active_index) const {
        return this->satfunc(this->compressed_kro, active_index);
    }

    const double * krorg(std::size_t active_index) const {
        return this->satfunc(this->compressed_krorg, active_index);
    }

    const double * krorw(std::size_t active_index) const {
        return this->satfunc(this->compressed_krorw, active_index);
    }

private:
    const double *
    satfunc(const std::vector<double>& data,
            const std::size_t          active_index) const
    {
        return data.empty() ? nullptr : &data[active_index];
    }


    std::vector<int> compressed_satnum;
    std::vector<double> compressed_swl;
    std::vector<double> compressed_sgl;
    std::vector<double> compressed_swcr;
    std::vector<double> compressed_sgcr;
    std::vector<double> compressed_sowcr;
    std::vector<double> compressed_sogcr;
    std::vector<double> compressed_swu;
    std::vector<double> compressed_sgu;
    std::vector<double> compressed_pcw;
    std::vector<double> compressed_pcg;
    std::vector<double> compressed_krw;
    std::vector<double> compressed_krwr;
    std::vector<double> compressed_kro;
    std::vector<double> compressed_krorg;
    std::vector<double> compressed_krorw;
    std::vector<double> compressed_krg;
    std::vector<double> compressed_krgr;

    std::vector<double> compressed_permx;
    std::vector<double> compressed_permy;
    std::vector<double> compressed_permz;
    std::vector<double> compressed_poro;
};
}
#endif

