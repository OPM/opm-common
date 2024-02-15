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
#include <opm/material/fluidmatrixinteractions/EclEpsScalingPoints.hpp>

#include <opm/material/fluidmatrixinteractions/EclEpsConfig.hpp>

#if HAVE_ECL_INPUT
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/SatfuncPropertyInitializers.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/material/common/Means.hpp>
#include <cmath>
#include <stdexcept>
#include <opm/material/fluidmatrixinteractions/EclEpsGridProperties.hpp>
#endif

#include <cassert>
#include <iostream>
#include <string>

namespace Opm {

template<class Scalar>
void EclEpsScalingPointsInfo<Scalar>::print() const
{
    std::cout << "    Swl: " << Swl << '\n'
              << "    Sgl: " << Sgl << '\n'
              << "    Swcr: " << Swcr << '\n'
              << "    Sgcr: " << Sgcr << '\n'
              << "    Sowcr: " << Sowcr << '\n'
              << "    Sogcr: " << Sogcr << '\n'
              << "    Swu: " << Swu << '\n'
              << "    Sgu: " << Sgu << '\n'
              << "    maxPcow: " << maxPcow << '\n'
              << "    maxPcgo: " << maxPcgo << '\n'
              << "    pcowLeverettFactor: " << pcowLeverettFactor << '\n'
              << "    pcgoLeverettFactor: " << pcgoLeverettFactor << '\n'
              << "    Krwr: " << Krwr << '\n'
              << "    Krgr: " << Krgr << '\n'
              << "    Krorw: " << Krorw << '\n'
              << "    Krorg: " << Krorg << '\n'
              << "    maxKrw: " << maxKrw << '\n'
              << "    maxKrg: " << maxKrg << '\n'
              << "    maxKrow: " << maxKrow << '\n'
              << "    maxKrog: " << maxKrog << '\n';
}

#if HAVE_ECL_INPUT
template<class Scalar>
void EclEpsScalingPointsInfo<Scalar>::
extractUnscaled(const satfunc::RawTableEndPoints& rtep,
                const satfunc::RawFunctionValues& rfunc,
                const std::vector<double>::size_type   satRegionIdx)
{
    this->Swl = rtep.connate.water[satRegionIdx];
    this->Sgl = rtep.connate.gas  [satRegionIdx];

    this->Swcr  = rtep.critical.water       [satRegionIdx];
    this->Sgcr  = rtep.critical.gas         [satRegionIdx];
    this->Sowcr = rtep.critical.oil_in_water[satRegionIdx];
    this->Sogcr = rtep.critical.oil_in_gas  [satRegionIdx];

    this->Swu = rtep.maximum.water[satRegionIdx];
    this->Sgu = rtep.maximum.gas  [satRegionIdx];

    this->maxPcgo = rfunc.pc.g[satRegionIdx];
    this->maxPcow = rfunc.pc.w[satRegionIdx];

    // there are no "unscaled" Leverett factors, so we just set them to 1.0
    this->pcowLeverettFactor = 1.0;
    this->pcgoLeverettFactor = 1.0;

    this->Krwr    = rfunc.krw.r [satRegionIdx];
    this->Krgr    = rfunc.krg.r [satRegionIdx];
    this->Krorw   = rfunc.kro.rw[satRegionIdx];
    this->Krorg   = rfunc.kro.rg[satRegionIdx];

    this->maxKrw  = rfunc.krw.max[satRegionIdx];
    this->maxKrow = rfunc.kro.max[satRegionIdx];
    this->maxKrog = rfunc.kro.max[satRegionIdx];
    this->maxKrg  = rfunc.krg.max[satRegionIdx];
}

template<class Scalar>
void EclEpsScalingPointsInfo<Scalar>::
extractScaled(const EclipseState& eclState,
               const EclEpsGridProperties& epsProperties,
               unsigned activeIndex)
{
    // overwrite the unscaled values with the values for the cell if it is
    // explicitly specified by the corresponding keyword.
    update(Swl,     epsProperties.swl(activeIndex));
    update(Sgl,     epsProperties.sgl(activeIndex));
    update(Swcr,    epsProperties.swcr(activeIndex));
    update(Sgcr,    epsProperties.sgcr(activeIndex));

    update(Sowcr,   epsProperties.sowcr(activeIndex));
    update(Sogcr,   epsProperties.sogcr(activeIndex));
    update(Swu,     epsProperties.swu(activeIndex));
    update(Sgu,     epsProperties.sgu(activeIndex));
    update(maxPcow, epsProperties.pcw(activeIndex));
    update(maxPcgo, epsProperties.pcg(activeIndex));

    update(this->Krwr,  epsProperties.krwr(activeIndex));
    update(this->Krgr,  epsProperties.krgr(activeIndex));
    update(this->Krorw, epsProperties.krorw(activeIndex));
    update(this->Krorg, epsProperties.krorg(activeIndex));

    update(maxKrw,  epsProperties.krw(activeIndex));
    update(maxKrg,  epsProperties.krg(activeIndex));
    update(maxKrow, epsProperties.kro(activeIndex));
    update(maxKrog, epsProperties.kro(activeIndex));

    // compute the Leverett capillary pressure scaling factors if applicable.  note
    // that this needs to be done using non-SI units to make it correspond to the
    // documentation.
    pcowLeverettFactor = 1.0;
    pcgoLeverettFactor = 1.0;
    if (eclState.getTableManager().useJFunc()) {
        const auto& jfunc = eclState.getTableManager().getJFunc();
        const auto& jfuncDir = jfunc.direction();

        Scalar perm;
        if (jfuncDir == JFunc::Direction::X)
            perm = epsProperties.permx(activeIndex);
        else if (jfuncDir == JFunc::Direction::Y)
            perm = epsProperties.permy(activeIndex);
        else if (jfuncDir == JFunc::Direction::Z)
            perm = epsProperties.permz(activeIndex);
        else if (jfuncDir == JFunc::Direction::XY)
            // TODO: verify that this really is the arithmetic mean. (the
            // documentation just says that the "average" should be used, IMO the
            // harmonic mean would be more appropriate because that's what's usually
            // applied when calculating the fluxes.)
        {
            double permx = epsProperties.permx(activeIndex);
            double permy = epsProperties.permy(activeIndex);
            perm = arithmeticMean(permx, permy);
        } else
            throw std::runtime_error("Illegal direction indicator for the JFUNC "
                                     "keyword ("+std::to_string(int(jfuncDir))+")");

        // convert permeability from m^2 to mD
        perm *= 1.01325e15;

        Scalar poro = epsProperties.poro(activeIndex);
        Scalar alpha = jfunc.alphaFactor();
        Scalar beta = jfunc.betaFactor();

        // the part of the Leverett capillary pressure which does not depend on
        // surface tension.
        Scalar commonFactor = std::pow(poro, alpha)/std::pow(perm, beta);

        // multiply the documented constant by 10^5 because we want the pressures
        // in [Pa], not in [bar]
        const Scalar Uconst = 0.318316 * 1e5;

        // compute the oil-water Leverett factor.
        const auto& jfuncFlag = jfunc.flag();
        if (jfuncFlag == JFunc::Flag::WATER || jfuncFlag == JFunc::Flag::BOTH) {
            // note that we use the surface tension in terms of [dyn/cm]
            Scalar gamma =
                jfunc.owSurfaceTension();
            pcowLeverettFactor = commonFactor*gamma*Uconst;
        }

        // compute the gas-oil Leverett factor.
        if (jfuncFlag == JFunc::Flag::GAS || jfuncFlag == JFunc::Flag::BOTH) {
            // note that we use the surface tension in terms of [dyn/cm]
            Scalar gamma =
                jfunc.goSurfaceTension();
            pcgoLeverettFactor = commonFactor*gamma*Uconst;
        }
    }
}
#endif

template<class Scalar>
void EclEpsScalingPoints<Scalar>::
init(const EclEpsScalingPointsInfo<Scalar>& epsInfo,
     const EclEpsConfig& config,
     EclTwoPhaseSystemType epsSystemType)
{
    if (epsSystemType == EclTwoPhaseSystemType::OilWater) {
        // saturation scaling for capillary pressure
        saturationPcPoints_[0] = epsInfo.Swl;
        saturationPcPoints_[2] = saturationPcPoints_[1] = epsInfo.Swu;

        // krw saturation scaling endpoints
        saturationKrwPoints_[0] = epsInfo.Swcr;
        saturationKrwPoints_[1] = 1.0 - epsInfo.Sowcr - epsInfo.Sgl;
        saturationKrwPoints_[2] = epsInfo.Swu;

        // krn saturation scaling endpoints (with the non-wetting phase being oil).
        // because opm-material specifies non-wetting phase relperms in terms of the
        // wetting phase saturations, the code here uses 1 minus the values specified
        // by the Eclipse TD and the order of the scaling points is reversed
        saturationKrnPoints_[2] = 1.0 - epsInfo.Sowcr;
        saturationKrnPoints_[1] = epsInfo.Swcr + epsInfo.Sgl;
        saturationKrnPoints_[0] = epsInfo.Swl + epsInfo.Sgl;

        if (config.enableLeverettScaling())
            maxPcnwOrLeverettFactor_ = epsInfo.pcowLeverettFactor;
        else
            maxPcnwOrLeverettFactor_ = epsInfo.maxPcow;

        Krwr_   = epsInfo.Krwr;
        Krnr_   = epsInfo.Krorw;

        maxKrw_ = epsInfo.maxKrw;
        maxKrn_ = epsInfo.maxKrow;
    }
    else {
        assert(epsSystemType == EclTwoPhaseSystemType::GasOil ||
               epsSystemType == EclTwoPhaseSystemType::GasWater);

        // saturation scaling for capillary pressure
        saturationPcPoints_[0] = 1.0 - epsInfo.Swl - epsInfo.Sgu;
        saturationPcPoints_[2] = saturationPcPoints_[1] = 1.0 - epsInfo.Swl - epsInfo.Sgl;

        // krw saturation scaling endpoints
        saturationKrwPoints_[0] = epsInfo.Sogcr;
        saturationKrwPoints_[1] = 1.0 - epsInfo.Sgcr - epsInfo.Swl;
        saturationKrwPoints_[2] = 1.0 - epsInfo.Swl - epsInfo.Sgl;

        // krn saturation scaling endpoints (with the non-wetting phase being gas).
        //
        // As opm-material specifies non-wetting phase relative
        // permeabilities in terms of the wetting phase saturations, the
        // code here uses (1-SWL) minus the values specified by the
        // ECLIPSE TD and the order of the scaling points is reversed.
        saturationKrnPoints_[2] = 1.0 - epsInfo.Swl - epsInfo.Sgcr;
        saturationKrnPoints_[1] = epsInfo.Sogcr;
        saturationKrnPoints_[0] = 1.0 - epsInfo.Swl - epsInfo.Sgu;

        if (config.enableLeverettScaling())
            maxPcnwOrLeverettFactor_ = epsInfo.pcgoLeverettFactor;
        else
            maxPcnwOrLeverettFactor_ = epsInfo.maxPcgo;

        Krwr_   = epsInfo.Krorg;
        Krnr_   = epsInfo.Krgr;

        maxKrw_ = epsInfo.maxKrog;
        maxKrn_ = epsInfo.maxKrg;
    }
}

template<class Scalar>
void EclEpsScalingPoints<Scalar>::print() const
{
    std::cout << "    saturationKrnPoints_[0]: " << saturationKrnPoints_[0] << "\n"
              << "    saturationKrnPoints_[1]: " << saturationKrnPoints_[1] << "\n"
              << "    saturationKrnPoints_[2]: " << saturationKrnPoints_[2] << "\n";

}

template struct EclEpsScalingPointsInfo<float>;
template struct EclEpsScalingPointsInfo<double>;

template class EclEpsScalingPoints<float>;
template class EclEpsScalingPoints<double>;

} // namespace Opm
