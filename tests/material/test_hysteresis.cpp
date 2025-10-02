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

static const char* hysterDeckStringKillough3pBaker = R"(

    RUNSPEC

    DIMENS
       1 1 1 /

    TABDIMS
     2 /

    OIL
    GAS
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
    0.12   0    1.0   0
    1      1.0  0.0   0 /
    0.12   0    1.0   0
    1      1.0  0.0   0 /

    SGOF
    0      0    1.0   0
    0.88      1.0  0.0   0 /
    0.12   0    1.0   0
    0.88      1.0  0.0   0 /

    REGIONS

    SATNUM
    1*1 /
    IMBNUM
    1*2 / )";

static const char* hysterDeckStringKilloughGasOilWetting = R"(

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
      0.1 4 0.1 1* BOTH /

    SATOPTS
      HYSTER /

    PROPS

    SGOF
    0      0    1.0   0
    1.0    1.0  0.0   0 /
    0.2    0    1.0   0
    1.0    1.0  0.0   0 /

    REGIONS

    SATNUM
    1*1 /
    IMBNUM
    1*2 / )";

static const char* hysterDeckStringKillough3pBakerWetting = R"(

    RUNSPEC

    DIMENS
       1 1 1 /

    TABDIMS
     2 /

    OIL
    GAS
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
      0.1 4 0.1 1* BOTH /

    SATOPTS
      HYSTER /

    PROPS

    SWOF
    0.12   0    1.0   0
    1      1.0  0.0   0 /
    0.12   0    1.0   0
    1      1.0  0.0   0 /

    SGOF
    0      0    1.0   0
    0.88   1.0  0.0   0 /
    0.2     0    1.0   0
    0.88   1.0  0.0   0 /

    REGIONS

    SATNUM
    1*1 /
    IMBNUM
    1*2 / )";

static const char* hysterDeckStringKillough3pStone1Wetting = R"(

    RUNSPEC

    DIMENS
       1 1 1 /

    TABDIMS
     2 /

    OIL
    GAS
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
      0.1 4 0.1 1* BOTH /

    SATOPTS
      HYSTER /

    PROPS

    STONE1

    SWOF
    0.12   0    1.0   0
    1      1.0  0.0   0 /
    0.12   0    1.0   0
    1      1.0  0.0   0 /

    SGOF
    0      0    1.0   0
    0.88   1.0  0.0   0 /
    0.2     0    1.0   0
    0.88   1.0  0.0   0 /

    REGIONS

    SATNUM
    1*1 /
    IMBNUM
    1*2 / )";

static const char* hysterDeckStringKillough3pStone2Wetting = R"(

    RUNSPEC

    DIMENS
       1 1 1 /

    TABDIMS
     2 /

    OIL
    GAS
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
      0.1 4 0.1 1* BOTH /

    SATOPTS
      HYSTER /

    PROPS

    STONE2

    SWOF
    0.12   0    1.0   0
    1      1.0  0.0   0 /
    0.12   0    1.0   0
    1      1.0  0.0   0 /

    SGOF
    0      0    1.0   0
    0.88   1.0  0.0   0 /
    0.2     0    1.0   0
    0.88   1.0  0.0   0 /

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

static constexpr const char* hysterDeckStringKilloughOilWaterScanning = R"(

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
    0      0    1.0   0
    0.88   1.0  0.0   0 /

    REGIONS

    SATNUM
    1*1 /

    IMBNUM
    1*2 / )";

static constexpr const char* hysterDeckStringKilloughWettingOilWater = R"(

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
      0.1 4 0.1 1* BOTH /

    SATOPTS
      HYSTER /

    PROPS

    SWOF
    0.12      0    1.0   0
    1      1.0  0.0   0 /
    0.12     0    1.0   0
    0.8    1.0  0.0   0 /

    REGIONS

    SATNUM
    1*1 /

    IMBNUM
    1*2 / )";


static constexpr const char* hysterDeckStringKilloughWetting3phaseBaker = R"(

    RUNSPEC

    DIMENS
       1 1 1 /

    TABDIMS
     2 /

    OIL
    WATER
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
      0.1 4 0.1 1* BOTH /

    SATOPTS
      HYSTER /

    PROPS

    SWOF
    0      0    1.0   0
    1      1.0  0.0   0 /
    0.12     0    1.0   0
    1.0    1.0  0.0   0 /

    SGOF
    0      0    1.0   0
    1      1.0  0.0   0 /
    0.12     0    1.0   0
    0.88    1.0  0.0   0 /



    REGIONS

    SATNUM
    1*1 /

    IMBNUM
    1*2 / )";

static constexpr const char* hysterDeckStringKillough3phaseBaker = R"(

    RUNSPEC

    DIMENS
       1 1 1 /

    TABDIMS
     2 /

    OIL
    WATER
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

   SWOF
    0      0    1.0   0
    1      1.0  0.0   0 /
    0      0    1.0   0
    0.88   1.0  0.0   0 /

    SGOF
    0      0    1.0   0
    1      1.0  0.0   0 /
    0.12     0    1.0   0
    1.0    1.0  0.0   0 /

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
    static constexpr bool enableHysteresis = true;
    static constexpr bool enableEndpointScaling = true;
    using MaterialTraits = Opm::ThreePhaseMaterialTraits<Scalar,
                                                         waterPhaseIdx,
                                                         oilPhaseIdx,
                                                         gasPhaseIdx,
                                                         enableHysteresis,
                                                         enableEndpointScaling>;

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
    using MaterialLawManager = Opm::EclMaterialLaw::Manager<MaterialTraits>;
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

template<class Scalar>
Scalar linearScaledRelperm(Scalar s, Scalar smin, Scalar smax, Scalar kmax) {
    if (s < smin)
        return 0;

    if (s > smax)
        return kmax;

    Scalar seff = (s - smin)/(smax - smin);
    return kmax * seff;
}

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
        Scalar Khyst = linearScaledRelperm(Sg, Scalar(0.12), Scalar(1.0), Scalar(1.0));
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
    Scalar trappedSg = MaterialLaw::trappedGasSaturation(param, /*maximumTrapping*/true);
    Scalar sgmax_out = 0.0;
    Scalar shmax_out = 0.0;
    Scalar somin_out = 0.0;
    MaterialLaw::gasOilHysteresisParams(sgmax_out,
                                        shmax_out,
                                        somin_out,
                                        param);

    Scalar maxKrg = sgmax_out;
    Scalar maxSg = sgmax_out;
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
        Scalar Khyst = linearScaledRelperm(Sg, trappedSg, maxSg, maxKrg);
        BOOST_CHECK_CLOSE(Sw, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(So, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(HysteresisKillough3pBakerConnateWaterScanning, Scalar, Types)
{
    using MaterialLaw = typename Fixture<Scalar>::MaterialLaw;
    using MaterialLawManager = typename Fixture<Scalar>::MaterialLawManager;
    constexpr int numPhases = Fixture<Scalar>::numPhases;

    Opm::Parser parser;

    const auto deck = parser.parseString(hysterDeckStringKillough3pBaker);
    const Opm::EclipseState eclState(deck);

    const auto& eclGrid = eclState.getInputGrid();
    std::size_t n = eclGrid.getCartesianSize();

    MaterialLawManager hysteresis;
    hysteresis.initFromState(eclState);
    hysteresis.initParamsForElements(eclState, n, doOldLookup, doNothing);
    auto& param = hysteresis.materialLawParams(0);
    Scalar Sw = 0.12;
    Scalar tol = 1e-3;
    std::array<Scalar,numPhases> kr = {0.0, 0.0, 0.0};
    for (int i = 0; i <= 50; ++ i) {
        Scalar Sg = Scalar(i) / 100;
        Scalar So = 1 - Sg - Sw;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        MaterialLaw::updateHysteresis(param, fs);
        BOOST_CHECK_CLOSE(0, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(So/0.88, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Sg/0.88, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }
    Scalar trappedSg = MaterialLaw::trappedGasSaturation(param, /*maximumTrapping*/true);
    Scalar sgmax_out = 0.0;
    Scalar shmax_out = 0.0;
    Scalar somin_out = 0.0;
    MaterialLaw::gasOilHysteresisParams(sgmax_out,
                                        shmax_out,
                                        somin_out,
                                        param);

    Scalar maxKrg = sgmax_out/0.88;
    Scalar maxSg = sgmax_out;
    BOOST_CHECK_CLOSE(0.5, maxSg, tol);
    Scalar Sncri = 0.12;
    Scalar killoughScalingParam = 0.1;
    Scalar C = 1 / Sncri - 1.0/0.88;
    Scalar Snr = maxSg / (1 + killoughScalingParam * (0.88 - maxSg) + C*maxSg );
    BOOST_CHECK_CLOSE(Snr, trappedSg, tol);

    for (int i = 50; i >= 0; -- i) {
        Scalar Sg = Scalar(i) / 100;
        Scalar So = 1 - Sg - Sw;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        MaterialLaw::updateHysteresis(param, fs);
        Scalar Khyst = linearScaledRelperm(Sg, trappedSg, maxSg, maxKrg);

        BOOST_CHECK_CLOSE(0, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(So/0.88, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(HysteresisKilloughGasOilScanningWetting, Scalar, Types)
{
    using MaterialLaw = typename Fixture<Scalar>::MaterialLaw;
    using MaterialLawManager = typename Fixture<Scalar>::MaterialLawManager;
    constexpr int numPhases = Fixture<Scalar>::numPhases;

    Opm::Parser parser;

    const auto deck = parser.parseString(hysterDeckStringKilloughGasOilWetting);
    const Opm::EclipseState eclState(deck);

    const auto& eclGrid = eclState.getInputGrid();
    std::size_t n = eclGrid.getCartesianSize();

    MaterialLawManager hysteresis;
    hysteresis.initFromState(eclState);
    hysteresis.initParamsForElements(eclState, n, doOldLookup, doNothing);
    auto& param = hysteresis.materialLawParams(0);
    Scalar Sw = 0.0;
    Scalar tol = 1e-3;
    Scalar sgmax_out = 0.0;
    Scalar shmax_out = 0.0;
    Scalar somin_out = 0.0;
    Scalar trappedSo = 0.0;
    std::array<Scalar,numPhases> kr = {0.0, 0.0, 0.0};
    for (int i = 0; i <= 50; ++ i) {
        Scalar So = Scalar(i) / 100;
        Scalar Sg = 1 - So;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        MaterialLaw::updateHysteresis(param, fs);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        Scalar Khyst = linearScaledRelperm(Sg, Scalar(0.2), Scalar(1.0), Scalar(1.0));
        trappedSo = MaterialLaw::trappedOilSaturation(param, /*maximumTrapping*/true);
        Scalar KhystO = linearScaledRelperm(So, trappedSo, Scalar(0.8), Scalar(1.0));
        BOOST_CHECK_CLOSE(Sw, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(KhystO, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }
    MaterialLaw::gasOilHysteresisParams(sgmax_out,
                                        shmax_out,
                                        somin_out,
                                        param);
    Scalar maxKrg = sgmax_out;
    Scalar maxSg = sgmax_out;
    BOOST_CHECK_CLOSE(1.0, maxKrg, tol);
    BOOST_CHECK_CLOSE(1.0, maxSg, tol);
    //Scalar Sncri = 0.12;
    //Scalar killoughScalingParam = 0.1;
    //Scalar C = 1 / Sncri - 1.0;
    //Scalar maxSo = 0.5;
    //Scalar maxKro = 0.5;
    //Scalar Snr = 1 / ( (C + killoughScalingParam) + 1.0/maxSo);
    Scalar trappedSg = MaterialLaw::trappedGasSaturation(param, /*maximumTrapping*/true);
    BOOST_CHECK_CLOSE(trappedSg, 0.2, tol);
    trappedSo = MaterialLaw::trappedOilSaturation(param, /*maximumTrapping*/true);
    BOOST_CHECK_SMALL(trappedSo, tol);

    for (int i = 50; i >= 0; -- i) {
        Scalar So = Scalar(i) / 100;
        Scalar Sg = 1 - So;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        MaterialLaw::updateHysteresis(param, fs);
        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        Scalar Khyst = linearScaledRelperm(Sg, Scalar(0.2), Scalar(1.0), Scalar(1.0));
        Scalar KhystO = linearScaledRelperm(So, trappedSo, Scalar(0.8), Scalar(1.0));
        BOOST_CHECK_CLOSE(Sw, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(KhystO, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }

    // test restart
    {
        MaterialLawManager hysteresis_restart;
        hysteresis_restart.initFromState(eclState);
        hysteresis_restart.initParamsForElements(eclState, n, doOldLookup, doNothing);
        Scalar sgmax_out2= 0.0;
        Scalar shmax_out2= 0.0;
        Scalar somin_out2= 0.0;
        hysteresis.gasOilHysteresisParams(sgmax_out2,
                                          shmax_out2,
                                          somin_out2,
                                          0);


        // the maximum number shouldn't change during imbibition
        BOOST_CHECK_CLOSE(sgmax_out, sgmax_out2, tol);
        BOOST_CHECK_CLOSE(shmax_out, shmax_out2, tol);
        hysteresis_restart.setGasOilHysteresisParams(sgmax_out2, shmax_out2, somin_out2, 0);

        Scalar Sg = 0.5;
        Scalar So = 1.0 - Sg;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        const auto& param_restart = hysteresis_restart.materialLawParams(0);
        std::array<Scalar,numPhases> kr_restart = {0.0, 0.0, 0.0};
        MaterialLaw::relativePermeabilities(kr_restart,
                                            param_restart,
                                            fs);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            BOOST_CHECK_CLOSE(kr_restart[phaseIdx], kr[phaseIdx], tol);
        }
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(HysteresisKillough3pBakerConnateWaterScanningWetting, Scalar, Types)
{
    using MaterialLaw = typename Fixture<Scalar>::MaterialLaw;
    using MaterialLawManager = typename Fixture<Scalar>::MaterialLawManager;
    constexpr int numPhases = Fixture<Scalar>::numPhases;

    Opm::Parser parser;

    const auto deck = parser.parseString(hysterDeckStringKillough3pBakerWetting);
    const Opm::EclipseState eclState(deck);

    const auto& eclGrid = eclState.getInputGrid();
    std::size_t n = eclGrid.getCartesianSize();

    MaterialLawManager hysteresis;
    hysteresis.initFromState(eclState);
    hysteresis.initParamsForElements(eclState, n, doOldLookup, doNothing);
    auto& param = hysteresis.materialLawParams(0);
    Scalar Sw = 0.12;
    Scalar tol = 1e-3;
    Scalar trappedSo = 0.0;
    Scalar trappedSg = 0.0;
    std::array<Scalar,numPhases> kr = {0.0, 0.0, 0.0};

    //SWOF
    //0.12   0    1.0   0
    //1      1.0  0.0   0 /
    //0.12   0    1.0   0
    //1      1.0  0.0   0 /
    //SGOF
    //0      0    1.0   0
    //.88   1.0  0.0   0 /
    //0.2      0    1.0   0
    //.88   1.0  0.0   0 /

    for (int i = 0; i <= 50; ++ i) {
        Scalar So = Scalar(i) / 100;
        Scalar Sg = 1 - So - Sw;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        MaterialLaw::updateHysteresis(param, fs);
        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        trappedSg = MaterialLaw::trappedGasSaturation(param, /*maximumTrapping*/true);
        trappedSo = MaterialLaw::trappedOilSaturation(param, /*maximumTrapping*/true);
        Scalar Khyst = linearScaledRelperm(Sg, trappedSg, Scalar(0.88), Scalar(1.0));
        Scalar Khyst0 = linearScaledRelperm(So, trappedSo, Scalar(0.88-0.2), Scalar(1.0));
        BOOST_CHECK_CLOSE(0, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Khyst0, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }
    trappedSg = MaterialLaw::trappedGasSaturation(param, /*maximumTrapping*/true);
    Scalar sgmax_out = 0.0;
    Scalar shmax_out = 0.0;
    Scalar somin_out = 0.0;
    MaterialLaw::gasOilHysteresisParams(sgmax_out,
                                        shmax_out,
                                        somin_out,
                                        param);

    Scalar maxKrg = sgmax_out;
    Scalar maxSg = sgmax_out;
    BOOST_CHECK_CLOSE(1.0 - Sw, maxKrg, tol);
    BOOST_CHECK_CLOSE(1.0 - Sw, maxSg, tol);
    //Scalar Sncri = 0.12;
    //Scalar killoughScalingParam = 0.1;
    //Scalar maxSo = 0.5;
    //Scalar maxKro = 0.5/0.88;
    //Scalar C = 1 / Sncri - 1.0/0.88;
    //Scalar Snr = maxSg / (1 + killoughScalingParam * (0.88 - maxSg) + C*maxSg );
    BOOST_CHECK_CLOSE(trappedSg, 0.2, tol);
    trappedSo = MaterialLaw::trappedOilSaturation(param, /*maximumTrapping*/true);
    BOOST_CHECK_SMALL(trappedSo, tol);

    for (int i = 50; i >= 0; -- i) {
        Scalar So = Scalar(i) / 100;
        Scalar Sg = 1 - So - Sw;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        MaterialLaw::updateHysteresis(param, fs);
        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);

        trappedSg = MaterialLaw::trappedGasSaturation(param, /*maximumTrapping*/true);
        trappedSo = MaterialLaw::trappedOilSaturation(param, /*maximumTrapping*/true);
        Scalar Khyst = linearScaledRelperm(Sg, trappedSg, Scalar(0.88), Scalar(1.0));
        Scalar KhystO = linearScaledRelperm(So, trappedSo, Scalar(0.88-0.2), Scalar(1.0));

        BOOST_CHECK_CLOSE(0, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        if (KhystO < tol) {
            BOOST_CHECK_SMALL(kr[Fixture<Scalar>::oilPhaseIdx], tol);
        } else {
            BOOST_CHECK_CLOSE(KhystO, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        }
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }

    // test restart
    {
        MaterialLawManager hysteresis_restart;
        hysteresis_restart.initFromState(eclState);
        hysteresis_restart.initParamsForElements(eclState, n, doOldLookup, doNothing);
        Scalar sgmax_out2= 0.0;
        Scalar shmax_out2= 0.0;
        Scalar somin_out2= 0.0;
        hysteresis.gasOilHysteresisParams(sgmax_out2,
                                          shmax_out2,
                                          somin_out2,
                                          0);


        // the maximum number shouldn't change during imbibition
        BOOST_CHECK_CLOSE(sgmax_out, sgmax_out2, tol);
        BOOST_CHECK_CLOSE(shmax_out, shmax_out2, tol);
        hysteresis_restart.setGasOilHysteresisParams(sgmax_out2, shmax_out2, somin_out2, 0);

        Scalar Sg = 0.5;
        Scalar So = 1.0 - Sg;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        const auto& param_restart = hysteresis_restart.materialLawParams(0);
        std::array<Scalar,numPhases> kr_restart = {0.0, 0.0, 0.0};
        MaterialLaw::relativePermeabilities(kr_restart,
                                            param_restart,
                                            fs);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            BOOST_CHECK_CLOSE(kr_restart[phaseIdx], kr[phaseIdx], tol);
        }
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(HysteresisKillough3pStone1ConnateWaterScanningWetting, Scalar, Types)
{
    using MaterialLaw = typename Fixture<Scalar>::MaterialLaw;
    using MaterialLawManager = typename Fixture<Scalar>::MaterialLawManager;
    constexpr int numPhases = Fixture<Scalar>::numPhases;

    Opm::Parser parser;

    const auto deck = parser.parseString(hysterDeckStringKillough3pStone1Wetting);
    const Opm::EclipseState eclState(deck);

    const auto& eclGrid = eclState.getInputGrid();
    std::size_t n = eclGrid.getCartesianSize();

    MaterialLawManager hysteresis;
    hysteresis.initFromState(eclState);
    hysteresis.initParamsForElements(eclState, n, doOldLookup, doNothing);
    auto& param = hysteresis.materialLawParams(0);
    Scalar Sw = 0.12;
    Scalar tol = 1e-3;
    Scalar trappedSo = 0.0;
    Scalar trappedSg = 0.0;
    std::array<Scalar,numPhases> kr = {0.0, 0.0, 0.0};
    for (int i = 0; i <= 50; ++ i) {
        Scalar So = Scalar(i) / 100;
        Scalar Sg = 1 - So - Sw;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        MaterialLaw::updateHysteresis(param, fs);
        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);

        trappedSg = MaterialLaw::trappedGasSaturation(param, /*maximumTrapping*/true);
        trappedSo = MaterialLaw::trappedOilSaturation(param, /*maximumTrapping*/true);
        Scalar Khyst = linearScaledRelperm(Sg, trappedSg, Scalar(0.88), Scalar(1.0));
        Scalar KhystO = linearScaledRelperm(So, trappedSo, Scalar(0.88-0.2), Scalar(1.0));

        BOOST_CHECK_CLOSE(0, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        if (So < tol) {
           BOOST_CHECK_SMALL(kr[Fixture<Scalar>::oilPhaseIdx], tol);
        } else {
        BOOST_CHECK_CLOSE(KhystO, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        }
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }
    trappedSg = MaterialLaw::trappedGasSaturation(param, /*maximumTrapping*/true);
    Scalar sgmax_out = 0.0;
    Scalar shmax_out = 0.0;
    Scalar somin_out = 0.0;
    MaterialLaw::gasOilHysteresisParams(sgmax_out,
                                        shmax_out,
                                        somin_out,
                                        param);

    Scalar maxKrg = sgmax_out;
    Scalar maxSg = sgmax_out;
    BOOST_CHECK_CLOSE(1.0 - Sw, maxKrg, tol);
    BOOST_CHECK_CLOSE(1.0 - Sw, maxSg, tol);
    BOOST_CHECK_CLOSE(trappedSg, 0.2, tol);
    trappedSo = MaterialLaw::trappedOilSaturation(param, /*maximumTrapping*/true);
    BOOST_CHECK_SMALL(trappedSo, tol);

    for (int i = 50; i >= 0; -- i) {
        Scalar So = Scalar(i) / 100;
        Scalar Sg = 1 - So - Sw;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        MaterialLaw::updateHysteresis(param, fs);
        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        trappedSg = MaterialLaw::trappedGasSaturation(param, /*maximumTrapping*/true);
        trappedSo = MaterialLaw::trappedOilSaturation(param, /*maximumTrapping*/true);
        Scalar Khyst = linearScaledRelperm(Sg, trappedSg, Scalar(0.88), Scalar(1.0));
        Scalar KhystO = linearScaledRelperm(So, trappedSo, Scalar(0.88-0.2), Scalar(1.0));
        BOOST_CHECK_CLOSE(0, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        if (KhystO < tol) {
            BOOST_CHECK_SMALL(kr[Fixture<Scalar>::oilPhaseIdx], tol);
        } else {
            BOOST_CHECK_CLOSE(KhystO, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        }
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }

    // test restart
    {
        MaterialLawManager hysteresis_restart;
        hysteresis_restart.initFromState(eclState);
        hysteresis_restart.initParamsForElements(eclState, n, doOldLookup, doNothing);
        Scalar sgmax_out2= 0.0;
        Scalar shmax_out2= 0.0;
        Scalar somin_out2= 0.0;
        hysteresis.gasOilHysteresisParams(sgmax_out2,
                                          shmax_out2,
                                          somin_out2,
                                          0);


        // the maximum number shouldn't change during imbibition
        BOOST_CHECK_CLOSE(sgmax_out, sgmax_out2, tol);
        BOOST_CHECK_CLOSE(shmax_out, shmax_out2, tol);
        hysteresis_restart.setGasOilHysteresisParams(sgmax_out2, shmax_out2, somin_out2, 0);

        Scalar Sg = 0.5;
        Scalar So = 1.0 - Sg;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        const auto& param_restart = hysteresis_restart.materialLawParams(0);
        std::array<Scalar,numPhases> kr_restart = {0.0, 0.0, 0.0};
        MaterialLaw::relativePermeabilities(kr_restart,
                                            param_restart,
                                            fs);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
           BOOST_CHECK_CLOSE(kr_restart[phaseIdx], kr[phaseIdx], tol);
        }
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(HysteresisKillough3pStone2ConnateWaterScanningWetting, Scalar, Types)
{
    using MaterialLaw = typename Fixture<Scalar>::MaterialLaw;
    using MaterialLawManager = typename Fixture<Scalar>::MaterialLawManager;
    constexpr int numPhases = Fixture<Scalar>::numPhases;

    Opm::Parser parser;

    const auto deck = parser.parseString(hysterDeckStringKillough3pStone2Wetting);
    const Opm::EclipseState eclState(deck);

    const auto& eclGrid = eclState.getInputGrid();
    std::size_t n = eclGrid.getCartesianSize();

    MaterialLawManager hysteresis;
    hysteresis.initFromState(eclState);
    hysteresis.initParamsForElements(eclState, n, doOldLookup, doNothing);
    auto& param = hysteresis.materialLawParams(0);
    Scalar Sw = 0.12;
    Scalar tol = 1e-3;
    Scalar trappedSo = 0.0;
    Scalar trappedSg = 0.0;
    std::array<Scalar,numPhases> kr = {0.0, 0.0, 0.0};
    for (int i = 0; i <= 50; ++ i) {
        Scalar So = Scalar(i) / 100;
        Scalar Sg = 1 - So - Sw;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        MaterialLaw::updateHysteresis(param, fs);
        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        trappedSg = MaterialLaw::trappedGasSaturation(param, /*maximumTrapping*/true);
        trappedSo = MaterialLaw::trappedOilSaturation(param, /*maximumTrapping*/true);
        Scalar Khyst = linearScaledRelperm(Sg, trappedSg, Scalar(0.88), Scalar(1.0));
        Scalar KhystO = linearScaledRelperm(So, trappedSo, Scalar(0.88-0.2), Scalar(1.0));

        BOOST_CHECK_CLOSE(0, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        if (So < tol) {
           BOOST_CHECK_SMALL(kr[Fixture<Scalar>::oilPhaseIdx], tol);
        } else {
        BOOST_CHECK_CLOSE(KhystO, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        }
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }
    trappedSg = MaterialLaw::trappedGasSaturation(param, /*maximumTrapping*/true);
    Scalar sgmax_out = 0.0;
    Scalar shmax_out = 0.0;
    Scalar somin_out = 0.0;
    MaterialLaw::gasOilHysteresisParams(sgmax_out,
                                        shmax_out,
                                        somin_out,
                                        param);

    Scalar maxKrg = sgmax_out;
    Scalar maxSg = sgmax_out;
    BOOST_CHECK_CLOSE(1.0 - Sw, maxKrg, tol);
    BOOST_CHECK_CLOSE(1.0 - Sw, maxSg, tol);
    BOOST_CHECK_CLOSE(trappedSg, 0.2, tol);
    trappedSo = MaterialLaw::trappedOilSaturation(param, /*maximumTrapping*/true);
    BOOST_CHECK_SMALL(trappedSo, tol);

    for (int i = 50; i >= 0; -- i) {
        Scalar So = Scalar(i) / 100;
        Scalar Sg = 1 - So - Sw;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        MaterialLaw::updateHysteresis(param, fs);
        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        trappedSg = MaterialLaw::trappedGasSaturation(param, /*maximumTrapping*/true);
        trappedSo = MaterialLaw::trappedOilSaturation(param, /*maximumTrapping*/true);
        Scalar Khyst = linearScaledRelperm(Sg, trappedSg, Scalar(0.88), Scalar(1.0));
        Scalar KhystO = linearScaledRelperm(So, trappedSo, Scalar(0.88-0.2), Scalar(1.0));

        BOOST_CHECK_CLOSE(0, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        if (So < tol) {
           BOOST_CHECK_SMALL(kr[Fixture<Scalar>::oilPhaseIdx], tol);
        } else {
        BOOST_CHECK_CLOSE(KhystO, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        }
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }

    // test restart
    {
        MaterialLawManager hysteresis_restart;
        hysteresis_restart.initFromState(eclState);
        hysteresis_restart.initParamsForElements(eclState, n, doOldLookup, doNothing);
        Scalar sgmax_out2= 0.0;
        Scalar shmax_out2= 0.0;
        Scalar somin_out2= 0.0;
        hysteresis.gasOilHysteresisParams(sgmax_out2,
                                          shmax_out2,
                                          somin_out2,
                                          0);


        // the maximum number shouldn't change during imbibition
        BOOST_CHECK_CLOSE(sgmax_out, sgmax_out2, tol);
        BOOST_CHECK_CLOSE(shmax_out, shmax_out2, tol);
        hysteresis_restart.setGasOilHysteresisParams(sgmax_out2, shmax_out2, somin_out2, 0);

        Scalar Sg = 0.5;
        Scalar So = 1.0 - Sg;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        const auto& param_restart = hysteresis_restart.materialLawParams(0);
        std::array<Scalar,numPhases> kr_restart = {0.0, 0.0, 0.0};
        MaterialLaw::relativePermeabilities(kr_restart,
                                            param_restart,
                                            fs);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            BOOST_CHECK_CLOSE(kr_restart[phaseIdx], kr[phaseIdx], tol);
        }
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
    Scalar trappedSg = MaterialLaw::trappedGasSaturation(param, /*maximumTrapping*/true);
    // if false the trapped saturation will be 0 during primary drainage
    Scalar trappedSg_active = MaterialLaw::trappedGasSaturation(param, /*maximumTrapping*/false);
    BOOST_CHECK_CLOSE(0.0, trappedSg_active, tol);
    Scalar sgmax_out = 0.0;
    Scalar shmax_out = 0.0;
    Scalar somin_out = 0.0;
    MaterialLaw::gasOilHysteresisParams(sgmax_out,
                                        shmax_out,
                                        somin_out,
                                        param);

    Scalar maxKrg = sgmax_out;
    Scalar maxSg = sgmax_out;
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

        // get stranded gas saturation
        // that is the distance between the drainage and imbibition at sg
        // i.e. sg_i - sg_d + sg_r where sg_d is the solution of kr_d(s) = kr_i(sg)
        // for linear relperm the solution is just s = kr_i
        Scalar strandedSg = MaterialLaw::strandedGasSaturation(param, Sg, kr[Fixture<Scalar>::gasPhaseIdx]);
        BOOST_CHECK_CLOSE(strandedSg, Sg - kr[Fixture<Scalar>::gasPhaseIdx], tol);

        MaterialLaw::updateHysteresis(param, fs);
        Scalar Khyst = (Sg < trappedSg)? 0.0 : (Sg - trappedSg) * ( maxKrg/ (maxSg - trappedSg));

        BOOST_CHECK_CLOSE(Sw, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(So, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        // need to use SMALL to avoid failure between zero and epsilon
        BOOST_CHECK_SMALL(Khyst - kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }

    // test restart
    {
        MaterialLawManager hysteresis_restart;
        hysteresis_restart.initFromState(eclState);
        hysteresis_restart.initParamsForElements(eclState, n, doOldLookup, doNothing);
        Scalar sgmax_out2= 0.0;
        Scalar shmax_out2= 0.0;
        Scalar somin_out2= 0.0;
        hysteresis.gasOilHysteresisParams(sgmax_out2,
                                          shmax_out2,
                                          somin_out2,
                                          0);


        // the maximum number shouldn't change during imbibition
        BOOST_CHECK_CLOSE(sgmax_out, sgmax_out2, tol);
        BOOST_CHECK_CLOSE(shmax_out, shmax_out2, tol);
        hysteresis_restart.setGasOilHysteresisParams(sgmax_out2, shmax_out2, somin_out2, 0);

        Scalar Sg = 0.5;
        Scalar So = 1.0 - Sg;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        const auto& param_restart = hysteresis_restart.materialLawParams(0);
        std::array<Scalar,numPhases> kr_restart = {0.0, 0.0, 0.0};
        MaterialLaw::relativePermeabilities(kr_restart,
                                            param_restart,
                                            fs);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            BOOST_CHECK_CLOSE(kr_restart[phaseIdx], kr[phaseIdx], tol);
        }
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

BOOST_AUTO_TEST_CASE_TEMPLATE(HysteresisKilloughOilWaterScanning, Scalar, Types)
{
    using MaterialLaw = typename Fixture<Scalar>::MaterialLaw;
    using MaterialLawManager = typename Fixture<Scalar>::MaterialLawManager;
    constexpr int numPhases = Fixture<Scalar>::numPhases;

    Opm::Parser parser;

    const auto deck = parser.parseString(hysterDeckStringKilloughOilWaterScanning);
    const Opm::EclipseState eclState(deck);

    const auto& eclGrid = eclState.getInputGrid();
    std::size_t n = eclGrid.getCartesianSize();

    MaterialLawManager hysteresis;
    hysteresis.initFromState(eclState);
    hysteresis.initParamsForElements(eclState, n, doOldLookup, doNothing);
    auto& param = hysteresis.materialLawParams(0);
    Scalar Sg = 0.0;
    Scalar tol = 1e-3;

    std::array<Scalar,numPhases> kr = {0.0, 0.0, 0.0};
    for (int i = 0; i <= 50; ++ i) {
        Scalar Sw = Scalar(i) / 100;
        Scalar So = 1 - Sw;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        MaterialLaw::updateHysteresis(param, fs);

        Scalar Khyst = (So < 0.12)? 0.0 : (So - 0.12) * ( 1.0/ (1.0 - 0.12));

        BOOST_CHECK_CLOSE(Sw, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Sg, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }

    Scalar trappedSo = MaterialLaw::trappedOilSaturation(param, /*maximumTrapping*/false);
    Scalar somax_out = 0.0;
    Scalar swmax_out = 0.0;
    Scalar swmin_out = 0.0;
    MaterialLaw::oilWaterHysteresisParams(somax_out,
                                          swmax_out,
                                          swmin_out,
                                          param);

    Scalar maxKro = somax_out;
    Scalar maxSo = somax_out;
    BOOST_CHECK_CLOSE(1.0, maxKro, tol);
    BOOST_CHECK_CLOSE(1.0, maxSo, tol);
    BOOST_CHECK_CLOSE(0.12, trappedSo, tol);
    BOOST_CHECK_CLOSE(swmin_out, 1 - somax_out, tol);

    for (int i = 50; i >= 0; -- i) {
        Scalar Sw = Scalar(i) / 100;
        Scalar So = 1 - Sw;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        MaterialLaw::updateHysteresis(param, fs);

        Scalar Khyst = (So < trappedSo)? 0.0 : (So - trappedSo) * ( maxKro/ (maxSo - trappedSo));

        BOOST_CHECK_CLOSE(Sw, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Sg, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }

    // test restart
    {
        MaterialLawManager hysteresis_restart;
        hysteresis_restart.initFromState(eclState);
        hysteresis_restart.initParamsForElements(eclState, n, doOldLookup, doNothing);
        Scalar somax_out2 = 0.0;
        Scalar swmax_out2 = 0.0;
        Scalar swmin_out2 = 0.0;
        hysteresis.oilWaterHysteresisParams(somax_out2,
                                            swmax_out2,
                                            swmin_out2,
                                            0);
        // the maximum oil saturation shouldn't change during imbibition
        BOOST_CHECK_CLOSE(somax_out, somax_out2, tol);
        hysteresis_restart.setOilWaterHysteresisParams(somax_out2, swmax_out2, swmin_out2, 0);

        Scalar So = 0.5;
        Scalar Sw = 1.0 - So;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        const auto& param_restart = hysteresis_restart.materialLawParams(0);
        std::array<Scalar,numPhases> kr_restart = {0.0, 0.0, 0.0};
        MaterialLaw::relativePermeabilities(kr_restart,
                                            param_restart,
                                            fs);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            BOOST_CHECK_CLOSE(kr_restart[phaseIdx], kr[phaseIdx], tol);
        }
    }
}


BOOST_AUTO_TEST_CASE_TEMPLATE(HysteresisKilloughWettingOilWater, Scalar, Types)
{
    using MaterialLaw = typename Fixture<Scalar>::MaterialLaw;
    using MaterialLawManager = typename Fixture<Scalar>::MaterialLawManager;
    constexpr int numPhases = Fixture<Scalar>::numPhases;

    Opm::Parser parser;

    const auto deck = parser.parseString(hysterDeckStringKilloughWettingOilWater);
    const Opm::EclipseState eclState(deck);

    const auto& eclGrid = eclState.getInputGrid();
    std::size_t n = eclGrid.getCartesianSize();

    MaterialLawManager hysteresis;
    hysteresis.initFromState(eclState);
    hysteresis.initParamsForElements(eclState, n, doOldLookup, doNothing);
    auto& param = hysteresis.materialLawParams(0);
    Scalar Sg = 0.0;
    Scalar tol = 1e-3;
    Scalar Swl = 0.12;
    Scalar somax_out = 0.0;
    Scalar swmax_out = 0.0;
    Scalar swmin_out = 0.0;
    Scalar trappedSo = Swl;
    Scalar trappedSw = 0.0;
    //SWOF
    //0.12      0    1.0   0
    //1      1.0  0.0   0 /
    //0.12     0    1.0   0
    //0.8    1.0  0.0   0 /

    std::array<Scalar,numPhases> kr = {0.0, 0.0, 0.0};
    for (int i = 0; i <= 50; ++ i) {
        Scalar Sw = Swl + (Scalar(i) / 100);
        Scalar So = 1 - Sw;

        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        MaterialLaw::updateHysteresis(param, fs);


        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);

        trappedSo = MaterialLaw::trappedOilSaturation(param, /*maximumTrapping*/false);
        trappedSw = MaterialLaw::trappedWaterSaturation(param);
        MaterialLaw::oilWaterHysteresisParams(somax_out,
                                            swmax_out,
                                            swmin_out,
                                            param);
        Scalar Khyst = linearScaledRelperm(So, trappedSo, somax_out, Scalar(1.0));
        Scalar Kw = linearScaledRelperm(Sw, trappedSw, Scalar(0.8), Scalar(1.0));
        BOOST_CHECK_CLOSE(Kw, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Sg, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }

    trappedSo = MaterialLaw::trappedOilSaturation(param, /*maximumTrapping*/false);
    MaterialLaw::oilWaterHysteresisParams(somax_out,
                                        swmax_out,
                                        swmin_out,
                                        param);

    Scalar maxKro = somax_out;
    Scalar maxSo = somax_out;
    BOOST_CHECK_CLOSE(1.0-Swl, maxKro, tol);
    BOOST_CHECK_CLOSE(1.0-Swl, maxSo, tol);
    BOOST_CHECK_CLOSE(1.0-0.8, trappedSo, tol);
    BOOST_CHECK_CLOSE(swmin_out, 1 - somax_out, tol);


    trappedSw = MaterialLaw::trappedWaterSaturation(param);
    Scalar Swcri = 0.12;
    //Scalar Swmaxd = 1.0;
    //Scalar maxSw = 0.5+Swl;
    //Scalar maxKrw = (0.5+Swl)/(1-Swl);
    //Scalar killoughScalingParam = 0.1;
    //Scalar Cw = 1 / (Swcri - Swcrd + 1.0e-12) - 1.0/(Swmaxd - Swcrd);
    //Scalar Swr = 1 / ( (Cw + killoughScalingParam) + 1.0/maxSw);
    BOOST_CHECK_CLOSE(Swcri, trappedSw, tol);

    for (int i = 50; i >= 0; -- i) {
        Scalar Sw = Scalar(i) / 100 + Swl;
        Scalar So = 1 - Sw;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        MaterialLaw::updateHysteresis(param, fs);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        trappedSo = MaterialLaw::trappedOilSaturation(param, /*maximumTrapping*/false);
        MaterialLaw::oilWaterHysteresisParams(somax_out,
                                            swmax_out,
                                            swmin_out,
                                            param);
        Scalar Khyst = linearScaledRelperm(So, trappedSo, somax_out, Scalar(1.0));
        Scalar Kw = linearScaledRelperm(Sw, trappedSw, Scalar(0.8), Scalar(1.0));
        BOOST_CHECK_CLOSE(Kw, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Sg, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }

    // test restart
    {
        MaterialLawManager hysteresis_restart;
        hysteresis_restart.initFromState(eclState);
        hysteresis_restart.initParamsForElements(eclState, n, doOldLookup, doNothing);
        Scalar somax_out2 = 0.0;
        Scalar swmax_out2 = 0.0;
        Scalar swmin_out2 = 0.0;
        hysteresis.oilWaterHysteresisParams(somax_out2,
                                            swmax_out2,
                                            swmin_out2,
                                            0);
        // the maximum oil saturation shouldn't change during imbibition
        BOOST_CHECK_CLOSE(somax_out, somax_out2, tol);
        hysteresis_restart.setOilWaterHysteresisParams(somax_out2, swmax_out2, swmin_out2, 0);

        Scalar So = 0.5;
        Scalar Sw = 1.0 - So;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        const auto& param_restart = hysteresis_restart.materialLawParams(0);
        std::array<Scalar,numPhases> kr_restart = {0.0, 0.0, 0.0};
        MaterialLaw::relativePermeabilities(kr_restart,
                                            param_restart,
                                            fs);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            BOOST_CHECK_CLOSE(kr_restart[phaseIdx], kr[phaseIdx], tol);
        }
    }
}

BOOST_AUTO_TEST_CASE_TEMPLATE(HysteresisKillough3pBakerScanning, Scalar, Types)
{
    using MaterialLaw = typename Fixture<Scalar>::MaterialLaw;
    using MaterialLawManager = typename Fixture<Scalar>::MaterialLawManager;
    constexpr int numPhases = Fixture<Scalar>::numPhases;

    Opm::Parser parser;

    const auto deck = parser.parseString(hysterDeckStringKillough3phaseBaker);
    const Opm::EclipseState eclState(deck);

    const auto& eclGrid = eclState.getInputGrid();
    std::size_t n = eclGrid.getCartesianSize();

    MaterialLawManager hysteresis;
    hysteresis.initFromState(eclState);
    hysteresis.initParamsForElements(eclState, n, doOldLookup, doNothing);
    auto& param = hysteresis.materialLawParams(0);
    Scalar Sg = 0.0;
    Scalar tol = 1e-3;

    std::array<Scalar,numPhases> kr = {0.0, 0.0, 0.0};
    for (int i = 0; i <= 50; ++ i) {
        Scalar Sw = Scalar(i) / 100;
        Scalar So = 1 - Sw;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        MaterialLaw::updateHysteresis(param, fs);

        Scalar Khyst = linearScaledRelperm(So, Scalar(0.12), Scalar(1.0), Scalar(1.0));
        BOOST_CHECK_CLOSE(Sw, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Sg, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }

    Scalar trappedSo = MaterialLaw::trappedOilSaturation(param, /*maximumTrapping*/false);
    Scalar somax_out = 0.0;
    Scalar swmax_out = 0.0;
    Scalar swmin_out = 0.0;
    MaterialLaw::oilWaterHysteresisParams(somax_out,
                                          swmax_out,
                                          swmin_out,
                                          param);

    Scalar maxKro = somax_out;
    Scalar maxSo = somax_out;
    BOOST_CHECK_CLOSE(1.0, maxKro, tol);
    BOOST_CHECK_CLOSE(1.0, maxSo, tol);
    BOOST_CHECK_CLOSE(0.12, trappedSo, tol);
    BOOST_CHECK_CLOSE(swmin_out, 1 - somax_out, tol);

    for (int i = 50; i >= 0; -- i) {
        Scalar Sw = Scalar(i) / 100;
        Scalar So = 1 - Sw;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        MaterialLaw::updateHysteresis(param, fs);

        Scalar Khyst = linearScaledRelperm(So, trappedSo, maxSo, maxKro);
        BOOST_CHECK_CLOSE(Sw, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Sg, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }

    // test restart
    {
        MaterialLawManager hysteresis_restart;
        hysteresis_restart.initFromState(eclState);
        hysteresis_restart.initParamsForElements(eclState, n, doOldLookup, doNothing);
        Scalar somax_out2 = 0.0;
        Scalar swmax_out2 = 0.0;
        Scalar swmin_out2 = 0.0;
        hysteresis.oilWaterHysteresisParams(somax_out2,
                                            swmax_out2,
                                            swmin_out2,
                                            0);
        // the maximum oil saturation shouldn't change during imbibition
        BOOST_CHECK_CLOSE(somax_out, somax_out2, tol);
        hysteresis_restart.setOilWaterHysteresisParams(somax_out2, swmax_out2, swmin_out2, 0);

        Scalar So = 0.5;
        Scalar Sw = 1.0 - So;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        const auto& param_restart = hysteresis_restart.materialLawParams(0);
        std::array<Scalar,numPhases> kr_restart = {0.0, 0.0, 0.0};
        MaterialLaw::relativePermeabilities(kr_restart,
                                            param_restart,
                                            fs);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            BOOST_CHECK_CLOSE(kr_restart[phaseIdx], kr[phaseIdx], tol);
        }
    }
}


BOOST_AUTO_TEST_CASE_TEMPLATE(HysteresisKilloughWetting3phaseBaker, Scalar, Types)
{
    using MaterialLaw = typename Fixture<Scalar>::MaterialLaw;
    using MaterialLawManager = typename Fixture<Scalar>::MaterialLawManager;
    constexpr int numPhases = Fixture<Scalar>::numPhases;

    Opm::Parser parser;

    const auto deck = parser.parseString(hysterDeckStringKilloughWetting3phaseBaker);
    const Opm::EclipseState eclState(deck);

    const auto& eclGrid = eclState.getInputGrid();
    std::size_t n = eclGrid.getCartesianSize();

    MaterialLawManager hysteresis;
    hysteresis.initFromState(eclState);
    hysteresis.initParamsForElements(eclState, n, doOldLookup, doNothing);
    auto& param = hysteresis.materialLawParams(0);
    Scalar Sg = 0.0;
    Scalar tol = 1e-3;

    //SWOF
    //0      0    1.0   0
    //1      1.0  0.0   0 /
    //0.12     0    1.0   0
    //1.0    1.0  0.0   0 /

    //SGOF
    //0      0    1.0   0
    //1      1.0  0.0   0 /
    //0.12     0    1.0   0
    //0.88    1.0  0.0   0 /
    std::array<Scalar,numPhases> kr = {0.0, 0.0, 0.0};
    for (int i = 0; i <= 50; ++ i) {
        Scalar Sw = Scalar(i) / 100;
        Scalar So = 1 - Sw;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        MaterialLaw::updateHysteresis(param, fs);


        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        Scalar Khyst = linearScaledRelperm(Sw, Scalar(0.12), Scalar(1.0), Scalar(1.0));
        Scalar KhystO = linearScaledRelperm(So, Scalar(0.0), Scalar(1.0 - 0.12), Scalar(1.0));

        if (Khyst < tol) {
           BOOST_CHECK_SMALL(kr[Fixture<Scalar>::waterPhaseIdx], tol);
        } else {
            BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        }
        BOOST_CHECK_CLOSE(KhystO, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Sg, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }

    Scalar trappedSo = MaterialLaw::trappedOilSaturation(param, /*maximumTrapping*/false);
    Scalar somax_out = 0.0;
    Scalar swmax_out = 0.0;
    Scalar swmin_out = 0.0;
    MaterialLaw::oilWaterHysteresisParams(somax_out,
                                        swmax_out,
                                        swmin_out,
                                        param);

    Scalar maxKro = somax_out;
    Scalar maxSo = somax_out;
    BOOST_CHECK_CLOSE(1.0, maxKro, tol);
    BOOST_CHECK_CLOSE(1.0, maxSo, tol);
    BOOST_CHECK_SMALL(trappedSo, tol);

    //Scalar maxSw = 0.5;
    Scalar trappedSw = MaterialLaw::trappedWaterSaturation(param);
    //Scalar Swcri = 0.12;
    //Scalar killoughScalingParam = 0.1;
    //Scalar Cw = 1 / Swcri - 1.0;
    //Scalar Swr = 1 / ( (Cw + killoughScalingParam) + 1.0/maxSw);
    //BOOST_CHECK_CLOSE(Swr, trappedSw, tol);
    BOOST_CHECK_SMALL(trappedSw, tol);

    for (int i = 50; i >= 0; -- i) {
        Scalar Sw = Scalar(i) / 100;
        Scalar So = 1 - Sw;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        MaterialLaw::updateHysteresis(param, fs);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        Scalar Khyst = linearScaledRelperm(Sw, Scalar(0.12), Scalar(1.0), Scalar(1.0));
        Scalar KhystOil = linearScaledRelperm(So, Scalar(0.0), Scalar(1.0 - 0.12), Scalar(1.0));
        if (Khyst < tol) {
           BOOST_CHECK_SMALL(kr[Fixture<Scalar>::waterPhaseIdx], tol);
        } else {
            BOOST_CHECK_CLOSE(Khyst, kr[Fixture<Scalar>::waterPhaseIdx], tol);
        }
        BOOST_CHECK_CLOSE(KhystOil, kr[Fixture<Scalar>::oilPhaseIdx], tol);
        BOOST_CHECK_CLOSE(Sg, kr[Fixture<Scalar>::gasPhaseIdx], tol);
    }

    // test restart
    {
        MaterialLawManager hysteresis_restart;
        hysteresis_restart.initFromState(eclState);
        hysteresis_restart.initParamsForElements(eclState, n, doOldLookup, doNothing);
        Scalar somax_out2 = 0.0;
        Scalar swmax_out2 = 0.0;
        Scalar swmin_out2 = 0.0;
        hysteresis.oilWaterHysteresisParams(somax_out2,
                                            swmax_out2,
                                            swmin_out2,
                                            0);
        // the maximum oil saturation shouldn't change during imbibition
        BOOST_CHECK_CLOSE(somax_out, somax_out2, tol);
        hysteresis_restart.setOilWaterHysteresisParams(somax_out2, swmax_out2, swmin_out2, 0);

        Scalar So = 0.5;
        Scalar Sw = 1.0 - So;
        typename Fixture<Scalar>::FluidState fs;
        fs.setSaturation(Fixture<Scalar>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<Scalar>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<Scalar>::gasPhaseIdx, Sg);
        const auto& param_restart = hysteresis_restart.materialLawParams(0);
        std::array<Scalar,numPhases> kr_restart = {0.0, 0.0, 0.0};
        MaterialLaw::relativePermeabilities(kr_restart,
                                            param_restart,
                                            fs);

        MaterialLaw::relativePermeabilities(kr,
                                            param,
                                            fs);
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            BOOST_CHECK_CLOSE(kr_restart[phaseIdx], kr[phaseIdx], tol);
        }
    }
}
