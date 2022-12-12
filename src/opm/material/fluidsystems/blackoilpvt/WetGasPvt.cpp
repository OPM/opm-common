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
#include <opm/material/fluidsystems/blackoilpvt/WetGasPvt.hpp>

#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#include <opm/input/eclipse/Schedule/OilVaporizationProperties.hpp>

namespace Opm {

template<class Scalar>
void WetGasPvt<Scalar>::
initFromState(const EclipseState& eclState, const Schedule& schedule)
{
    const auto& pvtgTables = eclState.getTableManager().getPvtgTables();
    const auto& densityTable = eclState.getTableManager().getDensityTable();

    assert(pvtgTables.size() == densityTable.size());

    size_t numRegions = pvtgTables.size();
    setNumRegions(numRegions);

    for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
        Scalar rhoRefO = densityTable[regionIdx].oil;
        Scalar rhoRefG = densityTable[regionIdx].gas;
        Scalar rhoRefW = densityTable[regionIdx].water;

        setReferenceDensities(regionIdx, rhoRefO, rhoRefG, rhoRefW);
    }

    for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
        const auto& pvtgTable = pvtgTables[regionIdx];

        const auto& saturatedTable = pvtgTable.getSaturatedTable();
        assert(saturatedTable.numRows() > 1);

        auto& gasMu = gasMu_[regionIdx];
        auto& invGasB = inverseGasB_[regionIdx];
        auto& invSatGasB = inverseSaturatedGasB_[regionIdx];
        auto& invSatGasBMu = inverseSaturatedGasBMu_[regionIdx];
        auto& oilVaporizationFac = saturatedOilVaporizationFactorTable_[regionIdx];

        oilVaporizationFac.setXYArrays(saturatedTable.numRows(),
                                       saturatedTable.getColumn("PG"),
                                       saturatedTable.getColumn("RV"));

        std::vector<Scalar> invSatGasBArray;
        std::vector<Scalar> invSatGasBMuArray;

        // extract the table for the gas dissolution and the oil formation volume factors
        for (unsigned outerIdx = 0; outerIdx < saturatedTable.numRows(); ++outerIdx) {
            Scalar pg = saturatedTable.get("PG" , outerIdx);
            Scalar B = saturatedTable.get("BG" , outerIdx);
            Scalar mu = saturatedTable.get("MUG" , outerIdx);

            invGasB.appendXPos(pg);
            gasMu.appendXPos(pg);

            invSatGasBArray.push_back(1.0 / B);
            invSatGasBMuArray.push_back(1.0 / (mu*B));

            assert(invGasB.numX() == outerIdx + 1);
            assert(gasMu.numX() == outerIdx + 1);

            const auto& underSaturatedTable = pvtgTable.getUnderSaturatedTable(outerIdx);
            size_t numRows = underSaturatedTable.numRows();
            for (size_t innerIdx = 0; innerIdx < numRows; ++innerIdx) {
                Scalar Rv = underSaturatedTable.get("RV" , innerIdx);
                Scalar Bg = underSaturatedTable.get("BG" , innerIdx);
                Scalar mug = underSaturatedTable.get("MUG" , innerIdx);

                invGasB.appendSamplePoint(outerIdx, Rv, 1.0 / Bg);
                gasMu.appendSamplePoint(outerIdx, Rv, mug);
            }
        }

        {
            std::vector<double> tmpPressure =  saturatedTable.getColumn("PG").vectorCopy( );

            invSatGasB.setXYContainers(tmpPressure, invSatGasBArray);
            invSatGasBMu.setXYContainers(tmpPressure, invSatGasBMuArray);
        }

        // make sure to have at least two sample points per gas pressure value
        for (unsigned xIdx = 0; xIdx < invGasB.numX(); ++xIdx) {
           // a single sample point is definitely needed
            assert(invGasB.numY(xIdx) > 0);

            // everything is fine if the current table has two or more sampling points
            // for a given mole fraction
            if (invGasB.numY(xIdx) > 1)
                continue;

            // find the master table which will be used as a template to extend the
            // current line. We define master table as the first table which has values
            // for undersaturated gas...
            size_t masterTableIdx = xIdx + 1;
            for (; masterTableIdx < saturatedTable.numRows(); ++masterTableIdx)
            {
                if (pvtgTable.getUnderSaturatedTable(masterTableIdx).numRows() > 1)
                    break;
            }

            if (masterTableIdx >= saturatedTable.numRows())
                throw std::runtime_error("PVTG tables are invalid: The last table must exhibit at least one "
                          "entry for undersaturated gas!");


            // extend the current table using the master table.
            extendPvtgTable_(regionIdx,
                             xIdx,
                             pvtgTable.getUnderSaturatedTable(xIdx),
                             pvtgTable.getUnderSaturatedTable(masterTableIdx));
        }
    }

    vapPar1_ = 0.0;
    const auto& oilVap = schedule[0].oilvap();
    if (oilVap.getType() == OilVaporizationProperties::OilVaporization::VAPPARS) {
        vapPar1_ = oilVap.vap1();
    }

    initEnd();
}

template<class Scalar>
void WetGasPvt<Scalar>::
extendPvtgTable_(unsigned regionIdx,
                 unsigned xIdx,
                 const SimpleTable& curTable,
                 const SimpleTable& masterTable)
{
    std::vector<double> RvArray = curTable.getColumn("RV").vectorCopy();
    std::vector<double> gasBArray = curTable.getColumn("BG").vectorCopy();
    std::vector<double> gasMuArray = curTable.getColumn("MUG").vectorCopy();

    auto& invGasB = inverseGasB_[regionIdx];
    auto& gasMu = gasMu_[regionIdx];

    for (size_t newRowIdx = 1; newRowIdx < masterTable.numRows(); ++newRowIdx) {
        const auto& RVColumn = masterTable.getColumn("RV");
        const auto& BGColumn = masterTable.getColumn("BG");
        const auto& viscosityColumn = masterTable.getColumn("MUG");

        // compute the gas pressure for the new entry
        Scalar diffRv = RVColumn[newRowIdx] - RVColumn[newRowIdx - 1];
        Scalar newRv = RvArray.back() + diffRv;

        // calculate the compressibility of the master table
        Scalar B1 = BGColumn[newRowIdx];
        Scalar B2 = BGColumn[newRowIdx - 1];
        Scalar x = (B1 - B2) / ((B1 + B2) / 2.0);

        // calculate the gas formation volume factor which exhibits the same
        // "compressibility" for the new value of Rv
        Scalar newBg = gasBArray.back()*(1.0 + x / 2.0) / (1.0 - x / 2.0);

        // calculate the "viscosibility" of the master table
        Scalar mu1 = viscosityColumn[newRowIdx];
        Scalar mu2 = viscosityColumn[newRowIdx - 1];
        Scalar xMu = (mu1 - mu2) / ((mu1 + mu2) / 2.0);

        // calculate the gas formation volume factor which exhibits the same
        // compressibility for the new pressure
        Scalar newMug = gasMuArray.back() * (1.0 + xMu / 2.0) / (1.0 - xMu / 2.0);

        // append the new values to the arrays which we use to compute the additional
        // values ...
        RvArray.push_back(newRv);
        gasBArray.push_back(newBg);
        gasMuArray.push_back(newMug);

        // ... and register them with the internal table objects
        invGasB.appendSamplePoint(xIdx, newRv, 1.0 / newBg);
        gasMu.appendSamplePoint(xIdx, newRv, newMug);
    }
}

template class WetGasPvt<double>;
template class WetGasPvt<float>;

} // namespace Opm
