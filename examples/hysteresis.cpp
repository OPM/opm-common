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
 * \brief A small application to extract relative permeability hysteresis
 * from a history of saturations
 *
 */
#include "config.h"

#include <opm/material/fluidmatrixinteractions/EclEpsGridProperties.hpp>
#include <opm/material/fluidmatrixinteractions/EclMaterialLawManager.hpp>
#include <opm/material/fluidstates/SimpleModularFluidState.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/input/eclipse/EclipseState/Grid/FieldPropsManager.hpp>

#include <opm/input/eclipse/Deck/Deck.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>

#include <array>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace {

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

// To support Local Grid Refinement for CpGrid, additional arguments have been added
// in some EclMaterialLawManager(InitParams) member functions. Therefore, we define
// some lambda expressions that does not affect this test file.
std::function<std::vector<int>(const Opm::FieldPropsManager&, const std::string&, bool)> doOldLookup =
    [](const Opm::FieldPropsManager& fieldPropManager, const std::string& propString, bool needsTranslation)
    {
        std::vector<int> dest{};
        const auto& intRawData = fieldPropManager.get_int(propString);
        const unsigned int numElems = intRawData.size();
        dest.resize(numElems);
        for (unsigned elemIdx = 0; elemIdx < numElems; ++elemIdx) {
            dest[elemIdx] = intRawData[elemIdx] - needsTranslation;
        }
        return dest;
    };

std::function<unsigned(unsigned)> doNothing = [](unsigned elemIdx){ return elemIdx; };

template<class Scalar, class MaterialLawParam, class FluidState>
std::array<Scalar,Fixture<Scalar>::numPhases>
capillaryPressure(const MaterialLawParam& param, const FluidState& fs)
{
    using MaterialLaw = typename Fixture<double>::MaterialLaw;
    constexpr int numPhases = Fixture<Scalar>::numPhases;
    std::array<Scalar,numPhases> pc;
    MaterialLaw::capillaryPressures(pc,
                                    param,
                                    fs);

    return pc;
}

template<class Scalar, class MaterialLawParam, class FluidState>
std::array<Scalar,Fixture<Scalar>::numPhases>
relativePermeabilities(const MaterialLawParam& param, const FluidState& fs)
{
    using MaterialLaw = typename Fixture<double>::MaterialLaw;
    constexpr int numPhases = Fixture<Scalar>::numPhases;
    std::array<Scalar,numPhases> kr;
    MaterialLaw::relativePermeabilities(kr,
                                        param,
                                        fs);
    return kr;
}

std::vector<double> readCSVToVector(const std::string& fname)
{
    // Open the File
    std::ifstream file(fname);
    std::vector<double> vector;
    std::string value;
    while (getline(file, value, '\n' )) {
        vector.push_back(std::stod(value));
    }
    file.close();
    return vector;
}

} // Anonymous namespace

int main(int argc, char **argv)
{
    bool help = false;
    for (int i = 1; i < argc; ++i) {
        std::string tmp = argv[i];
        help = help || (tmp  == "--h") || (tmp  == "--help");
    }

    if (argc < 5 || help) {
        std::cout << "USAGE:" << std::endl;
        std::cout << "hysteresis <fn_data> <fn_saturation> <fn_relperm> <wphase> <cellIdx>"<< std::endl;
        std::cout << "fn_data: Data file name that contains SGOF, EHYSTR etc. " << std::endl;
        std::cout << "fn_saturation: Data file name that contains saturations (s = water or gas depending on two-phase-system type). Single saturation per line " << std::endl;
        std::cout << "fn_relperm: Data file name that contains [s, kr, kro, krnSwMdc(So at turning point), Sn(trapped s) ]." << std::endl;
        std::cout << "two-phase-system: = {WO, GO, GW}, WO=water-oil, GO=gas-oil, GW=gas-water" << std::endl;
        std::cout << "cellIdx: cell index (default = 0), used to map SATNUM/IMBNUM" << std::endl;
        return help ? EXIT_SUCCESS : EXIT_FAILURE;
    }

    std::string input = argv[1];
    std::string input_csv = argv[2];
    std::vector<double> saturations = readCSVToVector(input_csv);
    std::string output_csv = argv[3];
    std::string two_phase_system = argv[4];
    double cellIdx = 0;
    if (argc > 5)
        cellIdx = std::stod(argv[5]);

    using MaterialLawManager = typename Fixture<double>::MaterialLawManager;
    using MaterialLaw = typename Fixture<double>::MaterialLaw;

    Opm::Parser parser;
    const auto deck = parser.parseFile(input);
    const Opm::EclipseState eclState(deck);

    MaterialLawManager materialLawManager;
    materialLawManager.initFromState(eclState);
    const auto& satnum = eclState.fieldProps().get_int("SATNUM");
    size_t n = satnum.size();
    materialLawManager.initParamsForElements(eclState, n, doOldLookup, doNothing);
    auto& param = materialLawManager.materialLawParams(cellIdx);

    const auto& ph = eclState.runspec().phases();
    bool hasGas = ph.active(Opm::Phase::GAS);
    bool hasOil = ph.active(Opm::Phase::OIL);
    bool hasWater = ph.active(Opm::Phase::WATER);

    int phaseIdx1 = -1; // saturations
    int phaseIdx2 = -1; // 1 - saturations
    int phaseIdx3 = -1; // 0

    if (two_phase_system == "WO" && hasWater && hasOil) {
        phaseIdx1 = Fixture<double>::waterPhaseIdx;
        phaseIdx2 = Fixture<double>::oilPhaseIdx;
        phaseIdx3 = Fixture<double>::gasPhaseIdx;
    } else if (two_phase_system == "GO" && hasGas && hasOil) {
        phaseIdx1 = Fixture<double>::gasPhaseIdx;
        phaseIdx2 = Fixture<double>::oilPhaseIdx;
        phaseIdx3 = Fixture<double>::waterPhaseIdx;
    } else if (two_phase_system == "GW" && hasGas && hasWater) {
        phaseIdx1 = Fixture<double>::gasPhaseIdx;
        phaseIdx2 = Fixture<double>::waterPhaseIdx;
        phaseIdx3 = Fixture<double>::oilPhaseIdx;
    } else {
        std::cout << "Invalid or inconsistent two-phase-system. " << std::endl;
        std::cout << "Valid two-phase-system: = {WO, GO, GW}, WO=water-oil, GO=gas-oil, GW=gas-water" << std::endl;
        std::cout << "Also make sure that the input deck is valid for the given two-phase-sytem." << std::endl;
        return EXIT_FAILURE;
    }

    typename Fixture<double>::FluidState fs;
    std::ofstream outfile;
    outfile.open (output_csv);

    for (const auto& s : saturations) {
        outfile << s << ",";
        fs.setSaturation(phaseIdx1, s);
        fs.setSaturation(phaseIdx2, 1 - s);
        fs.setSaturation(phaseIdx3, 0);
        auto relperm = relativePermeabilities<double>(param, fs);
        outfile << relperm[phaseIdx1] << "," << relperm[phaseIdx2]<< ",";

        MaterialLaw::updateHysteresis(param, fs);
        double somax_out = 0.0;
        if (two_phase_system == "WO") {
            double swmax_out = 0.0;
            double swmin_out = 0.0;
            MaterialLaw::oilWaterHysteresisParams(somax_out,
                                                  swmax_out,
                                                  swmin_out,
                                                  param);
        }
        if (two_phase_system == "GO") {
            double shmax_out = 0.0;
            double sowmin_out = 0.0;
            MaterialLaw::gasOilHysteresisParams(somax_out,
                                                shmax_out,
                                                sowmin_out,
                                                param);
        }
        if (two_phase_system == "GW") {
            // The GW hysteresis params is not possible to get directly from the 3p MaterialLaw
            //MaterialLaw::gasWaterHysteresisParams(pcSwMdc_out,
            //                                    somax_out,
            //                                    param);
        }

        double trapped_out = MaterialLaw::trappedGasSaturation(param, /*maximumTrapping*/ false);

        outfile << somax_out << "," << trapped_out << std::endl;
    }

    outfile.close();

    return 0;
}
