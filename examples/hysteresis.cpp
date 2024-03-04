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
 * \brief A small application to extract relative permeability hysteresis from a history of saturations
 *
 */
#include "config.h"

#include <opm/material/fluidmatrixinteractions/EclEpsGridProperties.hpp>
#include <opm/material/fluidmatrixinteractions/EclMaterialLawManager.hpp>
#include <opm/material/fluidstates/SimpleModularFluidState.hpp>

#include <opm/input/eclipse/Parser/Parser.hpp>
#include <opm/input/eclipse/Deck/Deck.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include <iostream>
#include <iomanip>
#include <fstream>

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

template<class Scalar, class MaterialLawParam, class FluidState>
std::array<Scalar,Fixture<Scalar>::numPhases>  capillaryPressure(const MaterialLawParam& param, const FluidState& fs) {
    using MaterialLaw = typename Fixture<double>::MaterialLaw;
    constexpr int numPhases = Fixture<Scalar>::numPhases;
    std::array<Scalar,numPhases> pc;
    MaterialLaw::capillaryPressures(pc,
                                    param,
                                    fs);

    return pc;
}

template<class Scalar, class MaterialLawParam, class FluidState>
std::array<Scalar,Fixture<Scalar>::numPhases> relativePermeabilities(const MaterialLawParam& param, const FluidState& fs) {
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
        std::cout << "fn_saturation: Data file name that contains saturations (s = water or gas depending on two-phase-system types). Single saturation per line " << std::endl;
        std::cout << "fn_relperm: Data file name that contains [s, kr, kro, krnSwMdc(So at turning point), Sn(trapped s) ]." << std::endl;
        std::cout << "two-phase-system: = {W, G}, W=water-oil, G=gas-oil" << std::endl;
        std::cout << "cellIdx: cell index (default = 0), used to map SATNUM/IMBNUM" << std::endl;
        return EXIT_FAILURE;
    }

    std::string input = argv[1];
    std::string input_csv = argv[2];
    std::vector<double> saturations = readCSVToVector(input_csv);
    std::string output_csv = argv[3];
    std::string two_phase_system = argv[4];
    int phaseIdx = -1;
    if (two_phase_system == "W")
        phaseIdx = Fixture<double>::waterPhaseIdx;
    if (two_phase_system == "G")
        phaseIdx = Fixture<double>::gasPhaseIdx;

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

    double Sw = 0;
    double Sg = 0;
    typename Fixture<double>::FluidState fs;
    std::ofstream outfile;
    outfile.open (output_csv);

    for (const auto& s : saturations) {
        outfile << s << ",";
        if (phaseIdx == Fixture<double>::waterPhaseIdx)
            Sw = s;
        if (phaseIdx == Fixture<double>::gasPhaseIdx)
            Sg = s;
        double So = 1 - s;
        fs.setSaturation(Fixture<double>::waterPhaseIdx, Sw);
        fs.setSaturation(Fixture<double>::oilPhaseIdx, So);
        fs.setSaturation(Fixture<double>::gasPhaseIdx, Sg);
        auto relperm = relativePermeabilities<double>(param, fs);
        outfile << relperm[phaseIdx] << "," << relperm[Fixture<double>::oilPhaseIdx]<< ",";

        MaterialLaw::updateHysteresis(param, fs);
        double krnSwMdc_out = 0.0;
        double pcSwMdc_out = 0.0;
        double trapped_out = 0.0;
        if (phaseIdx == Fixture<double>::waterPhaseIdx) {
            MaterialLaw::oilWaterHysteresisParams(pcSwMdc_out,
                                                krnSwMdc_out,
                                                param);
        }
        if (phaseIdx == Fixture<double>::gasPhaseIdx) {
            MaterialLaw::gasOilHysteresisParams(pcSwMdc_out,
                                                krnSwMdc_out,
                                                param);
            trapped_out = MaterialLaw::trappedGasSaturation(param);
        }
        outfile << krnSwMdc_out << "," << trapped_out << std::endl;
    }

    outfile.close();

    return 0;
}
