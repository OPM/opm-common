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
#include <opm/material/fluidsystems/blackoilpvt/BrineH2Pvt.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <fmt/format.h>

namespace Opm {

template<class Scalar>
BrineH2Pvt<Scalar>::
BrineH2Pvt(const std::vector<Scalar>& salinity,
           Scalar T_ref,
           Scalar P_ref)
    : salinity_(salinity)
{
    std::size_t num_regions = salinity_.size();
    h2ReferenceDensity_.resize(num_regions);
    brineReferenceDensity_.resize(num_regions);
    for (std::size_t i = 0; i < num_regions; ++i) {
        h2ReferenceDensity_[i] = H2::gasDensity(T_ref, P_ref, true);
        brineReferenceDensity_[i] = Brine::liquidDensity(T_ref, P_ref, salinity_[i], true);
    }
}

#if HAVE_ECL_INPUT
template<class Scalar>
void BrineH2Pvt<Scalar>::
initFromState(const EclipseState& eclState, const Schedule&)
{
    using Meas = UnitSystem::measure;
    auto usys = eclState.getDeckUnitSystem();
    bool h2sol = eclState.runspec().h2Sol();
    if( !h2sol && !eclState.getTableManager().getDensityTable().empty()) {
        OpmLog::warning("H2STORE is enabled but DENSITY is in the deck. \n"
                        "The surface density is computed based on H2-BRINE PVT "
                        "at standard conditions (STCOND) and DENSITY is ignored.");
    }

    if (!h2sol && (eclState.getTableManager().hasTables("PVDO") ||
        !eclState.getTableManager().getPvtgTables().empty()))
    {
        OpmLog::warning("H2STORE is enabled but PVDO or PVTO is in the deck. \n"
                        "H2 PVT properties are calculated internally, "
                        "and PVDO/PVTO input is ignored.");
    }

    if (eclState.getTableManager().hasTables("PVTW")) {
        OpmLog::warning("H2STORE or HSOL is enabled but PVTW is in the deck.\n"
                        "BRINE PVT properties are computed based on the Hu et al. "
                        "pvt model and PVTW input is ignored.");
    }
    OpmLog::info("H2STORE/HSOL is enabled.");
    // enable h2 dissolution into brine for h2sol case with DISGASW
    // or h2store case with DISGASW or DISGAS    
    bool h2sol_dis = h2sol && eclState.getSimulationConfig().hasDISGASW();
    bool h2storage_dis = eclState.runspec().h2Storage() && (eclState.getSimulationConfig().hasDISGASW() || eclState.getSimulationConfig().hasDISGAS());
    setEnableDissolvedGas(h2sol_dis || h2storage_dis);

    // Check if BRINE has been activated (varying salt concentration in brine)
    setEnableSaltConcentration(eclState.runspec().phases().active(Phase::BRINE));

    // We only supported single pvt region for the H2-brine module
    std::size_t regions = 1;
    setNumRegions(regions);
    std::size_t regionIdx = 0;

    // Currently we only support constant salinity
    const Scalar molality = eclState.getTableManager().salinity(); // mol/kg
    const Scalar MmNaCl = 58.44e-3; // molar mass of NaCl [kg/mol]
    salinity_[regionIdx] = 1 / ( 1 + 1 / (molality*MmNaCl)); // convert to mass fraction

    // set the surface conditions using the STCOND keyword
    Scalar T_ref = eclState.getTableManager().stCond().temperature;
    Scalar P_ref = eclState.getTableManager().stCond().pressure;

    brineReferenceDensity_[regionIdx] = Brine::liquidDensity(T_ref, P_ref, salinity_[regionIdx], extrapolate);
    h2ReferenceDensity_[regionIdx] = H2::gasDensity(T_ref, P_ref, extrapolate);

    OpmLog::info(fmt::format("The surface density of H2 is {:.6f} {}.",
                             usys.from_si(Meas::density, h2ReferenceDensity_[0]), usys.name(Meas::density)));
    OpmLog::info(fmt::format("The surface density of brine is {:.6f} {}.",
                             usys.from_si(Meas::density, brineReferenceDensity_[0]), usys.name(Meas::density)));
    OpmLog::info(fmt::format("The surface densities are computed using the reference pressure ({:.3f} {}) and reference temperature ({:.2f} {}).",
                             usys.from_si(Meas::pressure , P_ref), usys.name(Meas::pressure),
                             usys.from_si(Meas::temperature , T_ref), usys.name(Meas::temperature)));
}
#endif

template<class Scalar>
void BrineH2Pvt<Scalar>::
setNumRegions(std::size_t numRegions)
{
    brineReferenceDensity_.resize(numRegions);
    h2ReferenceDensity_.resize(numRegions);
    salinity_.resize(numRegions);
}

template<class Scalar>
void BrineH2Pvt<Scalar>::
setReferenceDensities(unsigned regionIdx,
                      Scalar rhoRefBrine,
                      Scalar rhoRefH2,
                      Scalar /*rhoRefWater*/)
{
    brineReferenceDensity_[regionIdx] = rhoRefBrine;
    h2ReferenceDensity_[regionIdx] = rhoRefH2;
}

template class BrineH2Pvt<double>;
template class BrineH2Pvt<float>;

} // namespace Opm
