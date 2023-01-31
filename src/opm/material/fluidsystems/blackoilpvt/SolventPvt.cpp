// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
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

#include <config.h>
#include <opm/material/fluidsystems/blackoilpvt/SolventPvt.hpp>

#include <opm/common/ErrorMacros.hpp>

#if HAVE_ECL_INPUT
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/input/eclipse/EclipseState/Tables/PvdsTable.hpp>
#endif

#include <fmt/format.h>

namespace Opm {

#if HAVE_ECL_INPUT
template<class Scalar>
void SolventPvt<Scalar>::
initFromState(const EclipseState& eclState, const Schedule&)
{
    const auto& pvdsTables = eclState.getTableManager().getPvdsTables();
    const auto& sdensityTables = eclState.getTableManager().getSolventDensityTables();

    if (pvdsTables.size() != sdensityTables.size()) {
        OPM_THROW(std::runtime_error,
                  fmt::format("Table sizes mismatch. PVDS: {}, sDensity: {}\n",
                              pvdsTables.size(), sdensityTables.size()));
    }

    size_t numRegions = pvdsTables.size();
    setNumRegions(numRegions);

    for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
        Scalar rhoRefS = sdensityTables[regionIdx].getSolventDensityColumn().front();

        setReferenceDensity(regionIdx, rhoRefS);

        const auto& pvdsTable = pvdsTables.getTable<PvdsTable>(regionIdx);

        // say 99.97% of all time: "premature optimization is the root of all
        // evil". Eclipse does this "optimization" for apparently no good reason!
        std::vector<Scalar> invB(pvdsTable.numRows());
        const auto& Bg = pvdsTable.getFormationFactorColumn();
        for (unsigned i = 0; i < Bg.size(); ++i) {
            invB[i] = 1.0 / Bg[i];
        }

        size_t numSamples = invB.size();
        inverseSolventB_[regionIdx].setXYArrays(numSamples, pvdsTable.getPressureColumn(), invB);
        solventMu_[regionIdx].setXYArrays(numSamples, pvdsTable.getPressureColumn(), pvdsTable.getViscosityColumn());
    }

    initEnd();
}
#endif

template<class Scalar>
void SolventPvt<Scalar>::setNumRegions(size_t numRegions)
{
    solventReferenceDensity_.resize(numRegions);
    inverseSolventB_.resize(numRegions);
    inverseSolventBMu_.resize(numRegions);
    solventMu_.resize(numRegions);
}

template<class Scalar>
void SolventPvt<Scalar>::
setSolventFormationVolumeFactor(unsigned regionIdx,
                                const SamplingPoints& samplePoints)
{
    SamplingPoints tmp(samplePoints);
    auto it = tmp.begin();
    const auto& endIt = tmp.end();
    for (; it != endIt; ++ it)
        std::get<1>(*it) = 1.0/std::get<1>(*it);

    inverseSolventB_[regionIdx].setContainerOfTuples(tmp);
    assert(inverseSolventB_[regionIdx].monotonic());
}

template<class Scalar>
void SolventPvt<Scalar>::initEnd()
{
    // calculate the final 2D functions which are used for interpolation.
    size_t numRegions = solventMu_.size();
    for (unsigned regionIdx = 0; regionIdx < numRegions; ++ regionIdx) {
        // calculate the table which stores the inverse of the product of the solvent
        // formation volume factor and its viscosity
        const auto& solventMu = solventMu_[regionIdx];
        const auto& invSolventB = inverseSolventB_[regionIdx];
        assert(solventMu.numSamples() == invSolventB.numSamples());

        std::vector<Scalar> pressureValues(solventMu.numSamples());
        std::vector<Scalar> invSolventBMuValues(solventMu.numSamples());
        for (unsigned pIdx = 0; pIdx < solventMu.numSamples(); ++pIdx) {
            pressureValues[pIdx] = invSolventB.xAt(pIdx);
            invSolventBMuValues[pIdx] = invSolventB.valueAt(pIdx) * (1.0/solventMu.valueAt(pIdx));
        }

        inverseSolventBMu_[regionIdx].setXYContainers(pressureValues, invSolventBMuValues);
    }
}

template class SolventPvt<double>;
template class SolventPvt<float>;

} // namespace Opm
