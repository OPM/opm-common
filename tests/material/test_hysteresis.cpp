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
 * \brief This is the unit test for the class which manages the parameters for the ECL
 *        saturation functions.
 *
 * This test requires the presence of opm-parser.
 */
#include "config.h"

#if !HAVE_ECL_INPUT
#error "The test for hysteresis requires eclipse input support in opm-common"
#endif

#include <boost/mpl/list.hpp>

#define BOOST_TEST_MODULE HYSTERESIS
#include <boost/test/unit_test.hpp>

#include <opm/material/fluidmatrixinteractions/EclEpsGridProperties.hpp>
#include <opm/material/fluidstates/SimpleModularFluidState.hpp>
#include <opm/material/fluidmatrixinteractions/EclMaterialLawManager.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include <functional>
#include <string>
#include <vector>
#include <array>

//Test Killogh hysteresis Gas Oil System
static const char* hysterDeckStringKilloughGasOil = R"(
    
    RUNSPEC

    DIMENS
       1 1 1 /
    
    TABDIMS
     2 /
    
    OIL
    GAS
    
    GRID
    
    DX
       1*1000 /
    DY
       1*1000 /
    DZ
       1*50 /
    
    TOPS
       1*0 /

    PORO
      1*0.15 /
    
    EHYSTR
      0.1 2 0.1 1* BOTH /
    
    SATOPTS
      HYSTER /
    
    PROPS
    
    SGOF
    0      0    1.0   0
    1      1.0  0.0   0 /
    0.12   0    1.0   0
    1      1.0  0.0   0 /
    
    REGIONS
    
    SATNUM
    1*1 /
    IMBNUM
    1*2 / )";

//Test Carlson hysteresis Gas Oil System
static constexpr const char* hysterDeckStringCarlsonGasOil = R"(
    
    RUNSPEC

    DIMENS
       1 1 1 /
    
    TABDIMS
     2 /
    
    OIL
    GAS
    
    GRID
    
    DX
       1*1000 /
    DY
       1*1000 /
    DZ
       1*50 /
    
    TOPS
       1*0 /

    PORO
      1*0.15 /
    
    EHYSTR
      0.1 0 0.1 1* BOTH /
    
    SATOPTS
      HYSTER /
    
    PROPS
    
    SGOF
    0      0    1.0   0
    1      1.0  0.0   0 /
    0.12   0    1.0   0
    1      1.0  0.0   0 /
    
    REGIONS
    
    SATNUM
    1*1 /
    IMBNUM
    1*2 / )";

//Test Killogh hysteresis Gas Water System
static constexpr const char* hysterDeckStringKilloughOilWater = R"(
    
    RUNSPEC

    DIMENS
       1 1 1 /
    
    TABDIMS
     2 /
    
    OIL
    WATER
    
    GRID
    
    DX
       1*1000 /
    DY
       1*1000 /
    DZ
       1*50 /
    
    TOPS
       1*0 /

    PORO
      1*0.15 /
    
    EHYSTR
      0.1 2 0.1 1* BOTH /
    
    SATOPTS
      HYSTER /
    
    PROPS
    
    SWOF
    0      0    1.0   0
    1      1.0  0.0   0 /
    0.12   0    1.0   0
    1      1.0  0.0   0 /
    
    REGIONS
    
    SATNUM
    1*1 /

    IMBNUM
    1*2 / )";

template<class Scalar>
struct Fixture {
    enum { numPhases = 3 };
    enum { waterPhaseIdx = 0 };
    enum { oilPhaseIdx = 1 };
    enum { gasPhaseIdx = 2 };
    using MaterialTraits = Opm::ThreePhaseMaterialTraits<Scalar,
                                                         waterPhaseIdx,
                                                         oilPhaseIdx,
                                                         gasPhaseIdx>;

    using FluidState = Opm::SimpleModularFluidState<Scalar,
                                                    /*numPhases=*/3,
                                                    /*numComponents=*/3,
                                                    void,
                                                    /*storePressure=*/false,
                                                    /*storeTemperature=*/false,
                                                    /*storeComposition=*/false,
                                                    /*storeFugacity=*/false,
                                                    /*storeSaturation=*/true,
                                                    /*storeDensity=*/false,
                                                    /*storeViscosity=*/false,
                                                    /*storeEnthalpy=*/false>;
    using MaterialLawManager = Opm::EclMaterialLawManager<MaterialTraits>;
    using MaterialLaw = typename MaterialLawManager::MaterialLaw;
};

namespace Opm
{
class FieldPropsManager;
}

// To support Local Grid Refinement for CpGrid, additional arguments have been added
// in some EclMaterialLawManager(InitParams) member functions. Therefore, we define
// some lambda expressions that does not affect this test file.
std::function<std::vector<int>(const Opm::FieldPropsManager&, const std::string&, bool)> doOldLookup =
    [](const Opm::FieldPropsManager& fieldPropManager, const std::string& propString, bool needsTranslation)
    {
        std::vector<int> dest;
        const auto& intRawData = fieldPropManager.get_int(propString);
        unsigned int numElems =  intRawData.size();
        dest.resize(numElems);
        for (unsigned elemIdx = 0; elemIdx < numElems; ++elemIdx) {
            dest[elemIdx] = intRawData[elemIdx] - needsTranslation;
        }
        return dest;
    };

std::function<unsigned(unsigned)> doNothing = [](unsigned elemIdx){ return elemIdx;};

using Types = boost::mpl::list<float,double>;


BOOST_AUTO_TEST_CASE_TEMPLATE(HysteresisKilloughGasOil, Scalar, Types)
{
    using MaterialLaw = typename Fixture<Scalar>::MaterialLaw;
    using MaterialLawManager = typename Fixture<Scalar>::MaterialLawManager;
    constexpr int numPhases = Fixture<Scalar>::numPhases;

    Opm::Parser parser;

    const auto deck = parser.parseString(hysterDeckStringKilloughGasOil);
    const Opm::EclipseState eclState(deck);

    const auto& eclGrid = eclState.getInputGrid();
    std::size_t n = eclGrid.getCartesianSize();

    MaterialLawManager hysteresis;
    hysteresis.initFromState(eclState);
    hysteresis.initParamsForElements(eclState, n, doOldLookup, doNothing);
    auto& param = hysteresis.materialLawParams(0);
    Scalar Sw = 0.0;
    Scalar tol = 1e-3;
    std::array<Scalar,numPhases> kr = {0.0, 0.0, 0.0};
    for (int i = 0; i <= 100; ++ i) {
        Scalar Sg = Scalar(i) / 100;
        Scalar So = 1 - Sg;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        MaterialLaw::updateHysteresis(param, fs);
        BOOST_CHECK_CLOSE(Sw, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(So, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Sg, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }
    for (int i = 100; i >= 0; -- i) {
        Scalar Sg = Scalar(i) / 100;
        Scalar So = 1 - Sg;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        MaterialLaw::updateHysteresis(param, fs);
        
        Scalar Khyst = (Sg < 0.12)? 0.0 : (Sg - 0.12) * (1 / (1 - 0.12)); 

        BOOST_CHECK_CLOSE(Sw, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(So, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(HysteresisKilloughGasOilScanning, Scalar, Types)
{
    using MaterialLaw = typename Fixture<Scalar>::MaterialLaw;
    using MaterialLawManager = typename Fixture<Scalar>::MaterialLawManager;
    constexpr int numPhases = Fixture<Scalar>::numPhases;

    Opm::Parser parser;

    const auto deck = parser.parseString(hysterDeckStringKilloughGasOil);
    const Opm::EclipseState eclState(deck);

    const auto& eclGrid = eclState.getInputGrid();
    std::size_t n = eclGrid.getCartesianSize();

    MaterialLawManager hysteresis;
    hysteresis.initFromState(eclState);
    hysteresis.initParamsForElements(eclState, n, doOldLookup, doNothing);
    auto& param = hysteresis.materialLawParams(0);
    Scalar Sw = 0.0;
    Scalar tol = 1e-3;
    std::array<Scalar,numPhases> kr = {0.0, 0.0, 0.0};
    for (int i = 0; i <= 50; ++ i) {
        Scalar Sg = Scalar(i) / 100;
        Scalar So = 1 - Sg;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        MaterialLaw::updateHysteresis(param, fs);
        BOOST_CHECK_CLOSE(Sw, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(So, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Sg, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }
    Scalar trappedSg = MaterialLaw::trappedGasSaturation(param);
    Scalar krnSwMdc_out = 0.0;
    Scalar pcSwMdc_out = 0.0;
    MaterialLaw::gasOilHysteresisParams(pcSwMdc_out,
                                        krnSwMdc_out,
                                        param);

    Scalar maxKrg = 1.0 - krnSwMdc_out; 
    Scalar maxSg = 1.0 - krnSwMdc_out;
    BOOST_CHECK_CLOSE(0.5, maxKrg, tol);
    BOOST_CHECK_CLOSE(0.5, maxSg, tol);
    Scalar Sncri = 0.12;
    Scalar killoughScalingParam = 0.1;
    Scalar C = 1 / Sncri - 1.0;
    Scalar Snr = 1 / ( (C + killoughScalingParam) + 1.0/maxSg);
    BOOST_CHECK_CLOSE(Snr, trappedSg, tol);

    for (int i = 50; i >= 0; -- i) {
        Scalar Sg = Scalar(i) / 100;
        Scalar So = 1 - Sg;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        MaterialLaw::updateHysteresis(param, fs);
        Scalar Khyst = (Sg < trappedSg)? 0.0 : (Sg - trappedSg) * ( maxKrg/ (maxSg - trappedSg)); 

        BOOST_CHECK_CLOSE(Sw, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(So, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(HysteresisCarlsonGasOilScanning, Scalar, Types)
{
    using MaterialLaw = typename Fixture<Scalar>::MaterialLaw;
    using MaterialLawManager = typename Fixture<Scalar>::MaterialLawManager;
    constexpr int numPhases = Fixture<Scalar>::numPhases;

    Opm::Parser parser;

    const auto deck = parser.parseString(hysterDeckStringCarlsonGasOil);
    const Opm::EclipseState eclState(deck);

    const auto& eclGrid = eclState.getInputGrid();
    std::size_t n = eclGrid.getCartesianSize();

    MaterialLawManager hysteresis;
    hysteresis.initFromState(eclState);
    hysteresis.initParamsForElements(eclState, n, doOldLookup, doNothing);
    auto& param = hysteresis.materialLawParams(0);    
    Scalar Sw = 0.0;
    Scalar tol = 1e-3;
    std::array<Scalar,numPhases> kr = {0.0, 0.0, 0.0};
    for (int i = 0; i <= 50; ++ i) {
        Scalar Sg = Scalar(i) / 100;
        Scalar So = 1 - Sg;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        MaterialLaw::updateHysteresis(param, fs);
        BOOST_CHECK_CLOSE(Sw, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(So, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Sg, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }
    Scalar trappedSg = MaterialLaw::trappedGasSaturation(param);
    Scalar krnSwMdc_out = 0.0;
    Scalar pcSwMdc_out = 0.0;
    MaterialLaw::gasOilHysteresisParams(pcSwMdc_out,
                                        krnSwMdc_out,
                                        param);

    Scalar maxKrg = 1.0 - krnSwMdc_out; 
    Scalar maxSg = 1.0 - krnSwMdc_out;
    BOOST_CHECK_CLOSE(0.5, maxKrg, tol);
    BOOST_CHECK_CLOSE(0.5, maxSg, tol);
    Scalar Si = 0.5 / ( 1.0/ (1.0 - 0.12)) + 0.12; //inverting the imb curve to find sg at krg(sg)=0.5 
    Scalar delta = 0.5 - Si;
    BOOST_CHECK_CLOSE(0.12+delta, trappedSg, tol);

    // Check that drainage equals imbibition at turning point.
    using MaterialLawGasOil = typename Fixture<Scalar>::MaterialLaw::GasOilMaterialLaw::EffectiveLaw;
    const auto& realParams = param.template getRealParams<Opm::EclMultiplexerApproach::TwoPhase>();
    const auto& drainageParams = realParams.gasOilParams().drainageParams();
    const auto& imbibitionParams = realParams.gasOilParams().imbibitionParams();
    Scalar deltaSwImbKrn = realParams.gasOilParams().deltaSwImbKrn();
    BOOST_CHECK_CLOSE(MaterialLawGasOil::twoPhaseSatKrn(drainageParams, 0.5), 
                      MaterialLawGasOil::twoPhaseSatKrn(imbibitionParams, 0.5 + deltaSwImbKrn), 
                      tol);

    for (int i = 50; i >= 0; -- i) {
        Scalar Sg = Scalar(i) / 100;
        Scalar So = 1 - Sg;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        MaterialLaw::updateHysteresis(param, fs);
        Scalar Khyst = (Sg < trappedSg)? 0.0 : (Sg - trappedSg) * ( maxKrg/ (maxSg - trappedSg)); 

        BOOST_CHECK_CLOSE(Sw, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(So, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        // need to use SMALL to avoid failure between zero and epsilon
        BOOST_CHECK_SMALL(Khyst - kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(HysteresisKilloughOilWater, Scalar, Types)
{
    using MaterialLaw = typename Fixture<Scalar>::MaterialLaw;
    using MaterialLawManager = typename Fixture<Scalar>::MaterialLawManager;
    constexpr int numPhases = Fixture<Scalar>::numPhases;

    Opm::Parser parser;

    const auto deck = parser.parseString(hysterDeckStringKilloughOilWater);
    const Opm::EclipseState eclState(deck);

    const auto& eclGrid = eclState.getInputGrid();
    size_t n = eclGrid.getCartesianSize();

    MaterialLawManager hysteresis;
    hysteresis.initFromState(eclState);
    hysteresis.initParamsForElements(eclState, n, doOldLookup, doNothing);
    auto& param = hysteresis.materialLawParams(0);
    Scalar Sg = 0.0;
    Scalar tol = 1e-3;
    std::array<Scalar,numPhases> kr = {0.0, 0.0, 0.0};
    for (int i = 0; i <= 100; ++ i) {
        Scalar So = Scalar(i) / 100;
        Scalar Sw = 1 - So;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        MaterialLaw::updateHysteresis(param, fs);
        BOOST_CHECK_CLOSE(Sw, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(So, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Sg, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }
    for (int i = 100; i >= 0; -- i) {
        Scalar So = Scalar(i) / 100;
        Scalar Sw = 1 - So;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        MaterialLaw::updateHysteresis(param, fs);
        
        Scalar Khyst = (Sw < 0.12)? 1.0 : So * (1 / (1 - 0.12)); 

        BOOST_CHECK_CLOSE(Sw, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Sg, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }
}