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
/*!
 * \file
 *
 * \brief This is the unit test for the black oil fluid system
 *
 * This test requires the presence of opm-parser.
 */
#include "config.h"

#if !HAVE_ECL_INPUT
#error "The test for the black oil fluid system classes requires ecl input support in opm-common"
#endif

#include <opm/material/fluidmatrixinteractions/EclEpsGridProperties.hpp>
#include <opm/material/fluidmatrixinteractions/EclMaterialLawManager.hpp>
#include <opm/material/fluidmatrixinteractions/EclMultiplexerMaterialParams.hpp>
#include <opm/material/fluidmatrixinteractions/EclDefaultMaterial.hpp>
#include <opm/material/fluidsystems/BlackOilFluidSystem.hpp>
#include <opm/material/fluidstates/BlackOilFluidState.hpp>
#include <opm/material/densead/Evaluation.hpp>
#include <opm/material/densead/Math.hpp>

#include <opm/material/common/Valgrind.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>
#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Python/Python.hpp>

#include <dune/common/parallel/mpihelper.hh>
#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/StreamLog.hpp>
#include <opm/common/OpmLog/LogUtil.hpp>
#include <opm/common/utility/TimeService.hpp>

#include <type_traits>
#include <cmath>

// values of strings based on the SPE1 and NORNE cases of opm-data.
// template<typename Evaluation,typename FluidSystem, int numPhases> class FluidAndRockState:
//     public Opm::BlackOilFluidState<Evaluation, FluidSystem>
// {
// public:
//     double referencePorosity_;
//     Evaluation porosity_;
//     Evaluation rockCompTransMultiplier_;
//     std::array<Evaluation,numPhases> mobility_;
// }
    


template <class Evaluation>
inline Opm::time_point::duration testAll(const char * deck_file)
{
    // test the black-oil specific methods of BlackOilFluidSystem. The generic methods
    // for fluid systems are already tested by the generic test for all fluidsystems.
    
    typedef typename Opm::MathToolbox<Evaluation>::Scalar Scalar;
    //typedef typename Evaluation::ValueType simpletype;
    typedef Opm::BlackOilFluidSystem<Scalar> FluidSystem;

    static constexpr int numPhases = FluidSystem::numPhases;

    static constexpr int gasPhaseIdx = FluidSystem::gasPhaseIdx;
    static constexpr int oilPhaseIdx = FluidSystem::oilPhaseIdx;
    static constexpr int waterPhaseIdx = FluidSystem::waterPhaseIdx;
    typedef Opm::ThreePhaseMaterialTraits<Scalar,
                                          /*wettingPhaseIdx=*/waterPhaseIdx,
                                          /*nonWettingPhaseIdx=*/oilPhaseIdx,
                                          /*gasPhaseIdx=*/gasPhaseIdx> MaterialTraits;

    static constexpr int gasCompIdx = FluidSystem::gasCompIdx;
    static constexpr int oilCompIdx = FluidSystem::oilCompIdx;
    static constexpr int waterCompIdx = FluidSystem::waterCompIdx;

    Opm::ParseContext parseContext;//(Opm::InputErrorAction::WARN);
    Opm::ErrorGuard errors;
    Opm::Parser parser;    

    auto deck = parser.parseFile(deck_file, parseContext, errors);
    auto python = std::make_shared<Opm::Python>();
    Opm::EclipseState eclState(deck);
    Opm::Schedule schedule(deck, eclState, python);

    FluidSystem::initFromState(eclState, schedule);
    const auto& eclGrid = eclState.getInputGrid();
    //size_t nc = eclGrid.getCartesianSize();
    size_t nc = eclGrid.getNumActive();
    typedef Opm::EclMaterialLawManager<MaterialTraits> MaterialLawManager;
    MaterialLawManager  materialLawManager;
    typedef typename MaterialLawManager::MaterialLaw MaterialLaw;
    materialLawManager.initFromState(eclState);
    materialLawManager.initParamsForElements(eclState, nc);
    // create a parameter cache
    typedef typename FluidSystem::template ParameterCache<Evaluation> ParamCache;
    
     typedef Opm::BlackOilFluidState<Evaluation, FluidSystem> FluidState;
    
    std::vector<int> pvtnum(nc,0);
    const std::string& name("PVTNUM");
    if (eclState.fieldProps().has_int(name)){
        const auto& numData = eclState.fieldProps().get_int(name);
    
        for (unsigned elemIdx = 0; elemIdx < nc; ++elemIdx) {
            pvtnum[elemIdx] = static_cast<int>(numData[elemIdx]) - 1;
        }
    }
    std::sort(pvtnum.begin(),pvtnum.end());
    
    static const Scalar eps = std::sqrt(std::numeric_limits<Scalar>::epsilon());
    unsigned numevals = 10000*100;
    unsigned num_total = ceil(double(numevals)/double(nc));
    //num_total=23;
    std::vector<FluidState> intQuant(nc);
    std::cout << "Doing evaluation " << num_total << " of nc " << nc << " total " << nc*num_total << std::endl;
    Opm::time_point start;
    start = Opm::TimeService::now();
    const auto& materialParams = materialLawManager.materialLawParams(0).template getRealParams<Opm::EclMultiplexerApproach::Default>();
    const auto& waterpvt = FluidSystem::waterPvt().template getRealPvt<Opm::WaterPvtApproach::ConstantCompressibilityWater>();
    const auto& oilpvt = FluidSystem::oilPvt().template getRealPvt<Opm::OilPvtApproach::LiveOil>();
    const auto& gaspvt = FluidSystem::gasPvt().template getRealPvt<Opm::GasPvtApproach::DryGas>();
    // const auto& waterpvt = FluidSystem::waterPvt();
    // const auto& oilpvt = FluidSystem::oilPvt();
    // const auto& gaspvt = FluidSystem::gasPvt();
    for (unsigned step = 0; step < num_total; ++step) {
        for (unsigned elemIdx = 0; elemIdx < nc; ++elemIdx) {
            // const auto& materialParams =
            //     materialLawManager.materialLawParams(elemIdx).template getRealParams<Opm::EclMultiplexerApproach::Default>();
            //std::array<Evaluation, numPhases> viscosity;
            //const auto& pvtRegionIdx = pvtnum[elemIdx];
            const signed pvtRegionIdx = 0;
            FluidState& fluidState = intQuant[elemIdx];
            Opm::Valgrind::SetUndefined(fluidState);
            Evaluation p = Scalar(elemIdx + nc * step) / num_total * 350e5 + 100e5;

            for (unsigned phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
                if (FluidSystem::phaseIsActive(phaseIdx)) {
                    fluidState.setPressure(phaseIdx, p);
                }
            }
            Evaluation Sw = 0.0;
            if constexpr (true) {
                Sw = Scalar(elemIdx + nc * step) / num_total;
            }
            Evaluation Sg = 0.0;
            if constexpr (true) {
                Sg = Scalar(elemIdx + nc * step) / num_total;
            }
            Evaluation So = 1 - Sw - Sg;

            if (FluidSystem::phaseIsActive(waterPhaseIdx))
                fluidState.setSaturation(waterPhaseIdx, Sw);
            
            if (FluidSystem::phaseIsActive(gasPhaseIdx))
                fluidState.setSaturation(gasPhaseIdx, Sg);
            
            if (FluidSystem::phaseIsActive(oilPhaseIdx))
                fluidState.setSaturation(oilPhaseIdx, So);




            // calculate the phase densities
            Evaluation T=278;
            Evaluation rho;
            if (FluidSystem::phaseIsActive(waterPhaseIdx)) {
                //const auto& waterpvt = FluidSystem::waterPvt();
                //Evaluation salt= 0.0;
                //Evaluation b = waterpvt.saturatedInverseFormationVolumeFactor(pvtRegionIdx, T, p, salt);//, Rsw, saltConcentration);
                //Evaluation mu = waterpvt.saturatedViscosity(pvtRegionIdx, T, p,salt);
                Evaluation b;
                Evaluation mu;
                waterpvt.inverseBAndMu(b,mu,pvtRegionIdx, p);
                fluidState.setInvB(waterPhaseIdx, b);
                //viscosity[waterPhaseIdx] = mu;
            }
            if (FluidSystem::phaseIsActive(gasPhaseIdx)) {
                if (FluidSystem::enableVaporizedOil()) {
                     const Evaluation& RvSat
                         = FluidSystem::saturatedDissolutionFactor(fluidState, gasPhaseIdx, pvtRegionIdx);
                     fluidState.setRv(RvSat);
                }
                //const auto& gaspvt = FluidSystem::gasPvt();//.getRealPvt();
                //Evaluation b = gaspvt.saturatedInverseFormationVolumeFactor(pvtRegionIdx, T, p);
                size_t segIdx = gaspvt.inverseGasB()[pvtRegionIdx].findSegmentIndex_(p,/*extrapolate=*/true);
                Evaluation b  =gaspvt.inverseGasB()[pvtRegionIdx].eval(p, segIdx);
                const Evaluation& invBMu = gaspvt.inverseGasBMu()[pvtRegionIdx].eval(p, segIdx);
                Evaluation mu = b/invBMu;
                //viscosity[oilPhaseIdx] =mu;          
                fluidState.setInvB(gasPhaseIdx, b);
            }
            if (FluidSystem::phaseIsActive(oilPhaseIdx)) {
                //if (FluidSystem::enableDissolvedGas()) {
                     // const Evaluation& RsSat
                     //     = FluidSystem::saturatedDissolutionFactor(fluidState, oilPhaseIdx, pvtRegionIdx);
                     size_t segIdx = oilpvt.saturatedGasDissolutionFactorTable()[pvtRegionIdx].findSegmentIndex_(p,/*extrapolate=*/true);
                     const Evaluation& RsSat = oilpvt.saturatedGasDissolutionFactorTable()[pvtRegionIdx].eval(p, segIdx);
                     fluidState.setRs(RsSat);
                     //}
                
                //const auto& oilpvt = FluidSystem::oilPvt();
                //Evaluation b = oilpvt.saturatedInverseFormationVolumeFactor(pvtRegionIdx, T, p);
                //Evaluation b = oilpvt.inverseSaturatedOilBTable()[pvtRegionIdx].eval(p, /*extrapolate=*/true);
                //size_t segIdx = oilpvt.inverseSaturatedOilBTable()[pvtRegionIdx].findSegmentIndex_(p,/*extrapolate=*/true);
                Evaluation b  =oilpvt.inverseSaturatedOilBTable()[pvtRegionIdx].eval(p, segIdx);
                //Evaluation mu = oilpvt.saturatedViscosity(pvtRegionIdx, T, p);
                //Evaluation invBMu = oilpvt.inverseSaturatedOilBMuTable()[pvtRegionIdx].eval(p, /*extrapolate=*/true);
                Evaluation invBMu = oilpvt.inverseSaturatedOilBMuTable()[pvtRegionIdx].eval(p,segIdx);
                Evaluation mu = b/invBMu;
                //viscosity[oilPhaseIdx] =mu;          
                fluidState.setInvB(oilPhaseIdx, b);
            }
            
            if (FluidSystem::phaseIsActive(waterPhaseIdx)) {
                rho = fluidState.invB(waterPhaseIdx);
                rho *= FluidSystem::referenceDensity(waterPhaseIdx, pvtRegionIdx);
                if (FluidSystem::enableDissolvedGasInWater()) {
                    rho += fluidState.invB(waterPhaseIdx) * fluidState.Rsw()
                        * FluidSystem::referenceDensity(gasPhaseIdx, pvtRegionIdx);
                }
                fluidState.setDensity(waterPhaseIdx, rho);
            }
            
            if (FluidSystem::phaseIsActive(gasPhaseIdx)) {
                rho = fluidState.invB(gasPhaseIdx);
                rho *= FluidSystem::referenceDensity(gasPhaseIdx, pvtRegionIdx);
                if (FluidSystem::enableVaporizedOil()) {
                    rho += fluidState.invB(gasPhaseIdx) * fluidState.Rv()
                        * FluidSystem::referenceDensity(oilPhaseIdx, pvtRegionIdx);
                }
                if (FluidSystem::enableVaporizedWater()) {
                    rho += fluidState.invB(gasPhaseIdx) * fluidState.Rvw()
                        * FluidSystem::referenceDensity(waterPhaseIdx, pvtRegionIdx);
                }
                fluidState.setDensity(gasPhaseIdx, rho);
            }

            if (FluidSystem::phaseIsActive(oilPhaseIdx)) {                
                rho = fluidState.invB(oilPhaseIdx);
                rho *= FluidSystem::referenceDensity(oilPhaseIdx, pvtRegionIdx);
                if (FluidSystem::enableDissolvedGas()) {
                    rho += fluidState.invB(oilPhaseIdx) * fluidState.Rs()
                        * FluidSystem::referenceDensity(gasPhaseIdx, pvtRegionIdx);
                }
                fluidState.setDensity(oilPhaseIdx, rho);
            }

            std::array<Evaluation, numPhases> mobility;
            // for (unsigned phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            //     if (FluidSystem::phaseIsActive(phaseIdx)) {
            //         // const Evaluation mu = FluidSystem::viscosity(fluidState, paramCache, phaseIdx);
            //         const Evaluation mu = FluidSystem::viscosity(fluidState, phaseIdx, pvtRegionIdx);
            //         mobility[phaseIdx] = mu;
            //     }
            // }

            // std::array<Evaluation, numPhases> pC;
            // //MaterialLaw::capillaryPressures(pC, materialParams, fluidState);
            // //MaterialLaw::relativePermeabilities(mobility, materialParams, fluidState);
            // MaterialLaw::DefaultMaterial::capillaryPressures(pC, materialParams, fluidState);
            // MaterialLaw::DefaultMaterial::relativePermeabilities(mobility, materialParams, fluidState);
            
            // rocktabTables = eclState.getTableManager().getRocktabTables();
            // const auto& rocktabTable = rocktabTables.template getTable<RocktabTable>(regionIdx);
            // const auto& pressureColumn = rocktabTable.getPressureColumn();
            // const auto& poroColumn = rocktabTable.getPoreVolumeMultiplierColumn();
            // const auto& transColumn = rocktabTable.getTransmissibilityMultiplierColumn();
           
            // Evaluation porosity_;
            // Evaluation rockCompTransMultiplier_;
            // Scalar rockCompressibility = problem.rockCompressibility(globalSpaceIdx);
            // if (rockCompressibility > 0.0) {
            //     Scalar rockRefPressure = problem.rockReferencePressure(globalSpaceIdx);
            //     Evaluation x;
            //     if (FluidSystem::phaseIsActive(oilPhaseIdx)) {
            //         x = rockCompressibility*(fluidState_.pressure(oilPhaseIdx) - rockRefPressure);
            //     } else if (FluidSystem::phaseIsActive(waterPhaseIdx)){
            //         x = rockCompressibility*(fluidState_.pressure(waterPhaseIdx) - rockRefPressure);
            //     } else {
            //         x = rockCompressibility*(fluidState_.pressure(gasPhaseIdx) - rockRefPressure);
            //     }
            //     porosity_ *= 1.0 + x + 0.5*x*x;
            // }
            // porosity_ *= problem.template rockCompPoroMultiplier<Evaluation>(*this, globalSpaceIdx);
            // rockCompTransMultiplier_ = problem.template rockCompTransMultiplier<Evaluation>(*this, globalSpaceIdx);
        }
    }
    Opm::time_point::duration total_time = Opm::TimeService::now() - start;
    return total_time;
    // make sure that the {oil,gas,water}Pvt() methods are available
    [[maybe_unused]] const auto& gPvt = FluidSystem::gasPvt();
    [[maybe_unused]] const auto& oPvt = FluidSystem::oilPvt();
    [[maybe_unused]] const auto& wPvt = FluidSystem::waterPvt();
}

int main(int argc, char **argv)
{
    
    Dune::MPIHelper::instance(argc, argv);

    typedef Opm::DenseAd::Evaluation<double, 3> TestEval;
    //typedef Opm::DenseAd::Evaluation<float, 3> TestEvalFloat;
    typedef Opm::DenseAd::Evaluation<double, 6> TestEval6;

    const char* deck_file = argv[1];

    
    //auto float_time = testAll<float>(deck_file);
    // auto evalfloat_time = testAll<TestEvalFloat>(deck_file);
    auto double_time = testAll<double>(deck_file);
    auto evaldouble_time = testAll<TestEval>(deck_file);
    auto evaldouble6_time = testAll<TestEval6>(deck_file);

    std::cout << "complete." << std::endl << std::endl;
    std::cout << "Time: " << std::endl;
    // float is slow here ?
    // std::cout << "   float.....: "  << std::chrono::duration<double>(float_time).count()  << " seconds" << std::endl;
    // std::cout << "   evalfloat.....: "  << std::chrono::duration<double>(evalfloat_time).count()  << " seconds" << std::endl;
    std::cout << "   double.....: "  << std::chrono::duration<double>(double_time).count()  << " seconds" << std::endl;
    std::cout << "   eval3 double....: "  << std::chrono::duration<double>(evaldouble_time).count()  << " seconds" << std::endl;
    std::cout << "   eval6 double....: "  << std::chrono::duration<double>(evaldouble6_time).count()  << " seconds" << std::endl;
    return 0;
}
