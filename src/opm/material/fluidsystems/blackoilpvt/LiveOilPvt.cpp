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
#include <opm/material/fluidsystems/blackoilpvt/LiveOilPvt.hpp>

#include <opm/common/ErrorMacros.hpp>

#if HAVE_ECL_INPUT
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/Schedule/OilVaporizationProperties.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>
#endif

#include <fmt/format.h>

namespace Opm {

#if HAVE_ECL_INPUT
template<class Scalar>
void LiveOilPvt<Scalar>::
initFromState(const EclipseState& eclState, const Schedule& schedule)
{
    const auto& pvtoTables = eclState.getTableManager().getPvtoTables();
    const auto& densityTable = eclState.getTableManager().getDensityTable();

    if (pvtoTables.size() != densityTable.size()) {
        OPM_THROW(std::runtime_error,
                  fmt::format("Table sizes mismatch. PVTO: {}, DensityTable: {}\n",
                              pvtoTables.size(), densityTable.size()));
    }

    size_t numRegions = pvtoTables.size();
    setNumRegions(numRegions);

    for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
        Scalar rhoRefO = densityTable[regionIdx].oil;
        Scalar rhoRefG = densityTable[regionIdx].gas;
        Scalar rhoRefW = densityTable[regionIdx].water;

        setReferenceDensities(regionIdx, rhoRefO, rhoRefG, rhoRefW);
    }

    // initialize the internal table objects
    for (unsigned regionIdx = 0; regionIdx < numRegions; ++regionIdx) {
        const auto& pvtoTable = pvtoTables[regionIdx];

        const auto& saturatedTable = pvtoTable.getSaturatedTable();
        if (saturatedTable.numRows() < 2) {
            OPM_THROW(std::runtime_error, "Saturated PVTO must have at least two rows.");
        }

        auto& oilMu = oilMuTable_[regionIdx];
        auto& satOilMu = saturatedOilMuTable_[regionIdx];
        auto& invOilB = inverseOilBTable_[regionIdx];
        auto& invSatOilB = inverseSaturatedOilBTable_[regionIdx];
        auto& gasDissolutionFac = saturatedGasDissolutionFactorTable_[regionIdx];
        std::vector<Scalar> invSatOilBArray;
        std::vector<Scalar> satOilMuArray;

        // extract the table for the gas dissolution and the oil formation volume factors
        for (unsigned outerIdx = 0; outerIdx < saturatedTable.numRows(); ++outerIdx) {
            Scalar Rs    = saturatedTable.get("RS", outerIdx);
            Scalar BoSat = saturatedTable.get("BO", outerIdx);
            Scalar muoSat = saturatedTable.get("MU", outerIdx);

            satOilMuArray.push_back(muoSat);
            invSatOilBArray.push_back(1.0 / BoSat);

            invOilB.appendXPos(Rs);
            oilMu.appendXPos(Rs);

            assert(invOilB.numX() == outerIdx + 1);
            assert(oilMu.numX() == outerIdx + 1);

            const auto& underSaturatedTable = pvtoTable.getUnderSaturatedTable(outerIdx);
            size_t numRows = underSaturatedTable.numRows();
            for (unsigned innerIdx = 0; innerIdx < numRows; ++innerIdx) {
                Scalar po = underSaturatedTable.get("P", innerIdx);
                Scalar Bo = underSaturatedTable.get("BO", innerIdx);
                Scalar muo = underSaturatedTable.get("MU", innerIdx);

                invOilB.appendSamplePoint(outerIdx, po, 1.0 / Bo);
                oilMu.appendSamplePoint(outerIdx, po, muo);
            }
        }

        // update the tables for the formation volume factor and for the gas
        // dissolution factor of saturated oil
        {
            const auto& tmpPressureColumn = saturatedTable.getColumn("P");
            const auto& tmpGasSolubilityColumn = saturatedTable.getColumn("RS");

            invSatOilB.setXYContainers(tmpPressureColumn, invSatOilBArray);
            satOilMu.setXYContainers(tmpPressureColumn, satOilMuArray);
            gasDissolutionFac.setXYContainers(tmpPressureColumn, tmpGasSolubilityColumn);
        }

        updateSaturationPressure_(regionIdx);
        // make sure to have at least two sample points per Rs value
        for (unsigned xIdx = 0; xIdx < invOilB.numX(); ++xIdx) {
            // a single sample point is definitely needed
            assert(invOilB.numY(xIdx) > 0);

            // everything is fine if the current table has two or more sampling points
            // for a given mole fraction
            if (invOilB.numY(xIdx) > 1)
                continue;

            // find the master table which will be used as a template to extend the
            // current line. We define master table as the first table which has values
            // for undersaturated oil...
            size_t masterTableIdx = xIdx + 1;
            for (; masterTableIdx < saturatedTable.numRows(); ++masterTableIdx)
            {
                if (pvtoTable.getUnderSaturatedTable(masterTableIdx).numRows() > 1)
                    break;
            }

            if (masterTableIdx >= saturatedTable.numRows())
                OPM_THROW(std::runtime_error,
                          "PVTO tables are invalid: "
                          "The last table must exhibit at least one "
                          "entry for undersaturated oil!");

            // extend the current table using the master table.
            extendPvtoTable_(regionIdx,
                             xIdx,
                             pvtoTable.getUnderSaturatedTable(xIdx),
                             pvtoTable.getUnderSaturatedTable(masterTableIdx));
        }
    }

    vapPar2_ = 0.0;
    const auto& oilVap = schedule[0].oilvap();
    if (oilVap.getType() == OilVaporizationProperties::OilVaporization::VAPPARS) {
        vapPar2_ = oilVap.vap2();
    }

    initEnd();
}

template<class Scalar>
void LiveOilPvt<Scalar>::
extendPvtoTable_(unsigned regionIdx,
                 unsigned xIdx,
                 const SimpleTable& curTable,
                 const SimpleTable& masterTable)
{
    std::vector<double> pressuresArray = curTable.getColumn("P").vectorCopy();
    std::vector<double> oilBArray = curTable.getColumn("BO").vectorCopy();
    std::vector<double> oilMuArray = curTable.getColumn("MU").vectorCopy();

    auto& invOilB = inverseOilBTable_[regionIdx];
    auto& oilMu = oilMuTable_[regionIdx];

    for (unsigned newRowIdx = 1; newRowIdx < masterTable.numRows(); ++newRowIdx) {
        const auto& pressureColumn = masterTable.getColumn("P");
        const auto& BOColumn = masterTable.getColumn("BO");
        const auto& viscosityColumn = masterTable.getColumn("MU");

        // compute the oil pressure for the new entry
        Scalar diffPo = pressureColumn[newRowIdx] - pressureColumn[newRowIdx - 1];
        Scalar newPo = pressuresArray.back() + diffPo;

        // calculate the compressibility of the master table
        Scalar B1 = BOColumn[newRowIdx];
        Scalar B2 = BOColumn[newRowIdx - 1];
        Scalar x = (B1 - B2) / ((B1 + B2) / 2.0);

        // calculate the oil formation volume factor which exhibits the same
        // compressibility for the new pressure
        Scalar newBo = oilBArray.back()*(1.0 + x / 2.0) / (1.0 - x / 2.0);

        // calculate the "viscosibility" of the master table
        Scalar mu1 = viscosityColumn[newRowIdx];
        Scalar mu2 = viscosityColumn[newRowIdx - 1];
        Scalar xMu = (mu1 - mu2) / ((mu1 + mu2) / 2.0);

        // calculate the oil formation volume factor which exhibits the same
        // compressibility for the new pressure
        Scalar newMuo = oilMuArray.back()*(1.0 + xMu / 2.0) / (1.0 - xMu / 2.0);

        // append the new values to the arrays which we use to compute the additional
        // values ...
        pressuresArray.push_back(newPo);
        oilBArray.push_back(newBo);
        oilMuArray.push_back(newMuo);

        // ... and register them with the internal table objects
        invOilB.appendSamplePoint(xIdx, newPo, 1.0 / newBo);
        oilMu.appendSamplePoint(xIdx, newPo, newMuo);
    }
}
#endif

template<class Scalar>
void LiveOilPvt<Scalar>::setNumRegions(size_t numRegions)
{
    oilReferenceDensity_.resize(numRegions);
    gasReferenceDensity_.resize(numRegions);
    inverseOilBTable_.resize(numRegions, TabulatedTwoDFunction{TabulatedTwoDFunction::InterpolationPolicy::LeftExtreme});
    inverseOilBMuTable_.resize(numRegions, TabulatedTwoDFunction{TabulatedTwoDFunction::InterpolationPolicy::LeftExtreme});
    inverseSaturatedOilBTable_.resize(numRegions);
    inverseSaturatedOilBMuTable_.resize(numRegions);
    oilMuTable_.resize(numRegions, TabulatedTwoDFunction{TabulatedTwoDFunction::InterpolationPolicy::LeftExtreme});
    saturatedOilMuTable_.resize(numRegions);
    saturatedGasDissolutionFactorTable_.resize(numRegions);
    saturationPressure_.resize(numRegions);
}

template<class Scalar>
void LiveOilPvt<Scalar>::
setReferenceDensities(unsigned regionIdx,
                      Scalar rhoRefOil,
                      Scalar rhoRefGas,
                      Scalar)
{
    oilReferenceDensity_[regionIdx] = rhoRefOil;
    gasReferenceDensity_[regionIdx] = rhoRefGas;
}

template<class Scalar>
void LiveOilPvt<Scalar>::
setSaturatedOilFormationVolumeFactor(unsigned regionIdx,
                                     const SamplingPoints& samplePoints)
{
    constexpr const Scalar T = 273.15 + 15.56; // [K]
    auto& invOilB = inverseOilBTable_[regionIdx];

    updateSaturationPressure_(regionIdx);

    // calculate a table of estimated densities of undersatured gas
    for (size_t pIdx = 0; pIdx < samplePoints.size(); ++pIdx) {
        Scalar p1 = std::get<0>(samplePoints[pIdx]);
        Scalar p2 = p1 * 2.0;

        Scalar Bo1 = std::get<1>(samplePoints[pIdx]);
        Scalar drhoo_dp = (1.1200 - 1.1189)/((5000 - 4000)*6894.76);
        Scalar Bo2 = Bo1/(1.0 + (p2 - p1)*drhoo_dp);

        Scalar Rs = saturatedGasDissolutionFactor(regionIdx, T, p1);

        invOilB.appendXPos(Rs);
        invOilB.appendSamplePoint(pIdx, p1, 1.0/Bo1);
        invOilB.appendSamplePoint(pIdx, p2, 1.0/Bo2);
    }
}

template<class Scalar>
void LiveOilPvt<Scalar>::
setSaturatedOilViscosity(unsigned regionIdx,
                         const SamplingPoints& samplePoints)
{
    constexpr const Scalar T = 273.15 + 15.56; // [K]

    // update the table for the saturated oil
    saturatedOilMuTable_[regionIdx].setContainerOfTuples(samplePoints);

    // calculate a table of estimated viscosities depending on pressure and gas mass
    // fraction for untersaturated oil to make the other code happy
    for (size_t pIdx = 0; pIdx < samplePoints.size(); ++pIdx) {
        Scalar p1 = std::get<0>(samplePoints[pIdx]);
        Scalar p2 = p1 * 2.0;

        // no pressure dependence of the viscosity
        Scalar mu1 = std::get<1>(samplePoints[pIdx]);
        Scalar mu2 = mu1;

        Scalar Rs = saturatedGasDissolutionFactor(regionIdx, T, p1);

        oilMuTable_[regionIdx].appendXPos(Rs);
        oilMuTable_[regionIdx].appendSamplePoint(pIdx, p1, mu1);
        oilMuTable_[regionIdx].appendSamplePoint(pIdx, p2, mu2);
    }
}

template<class Scalar>
void LiveOilPvt<Scalar>::initEnd()
{
    // calculate the final 2D functions which are used for interpolation.
    size_t numRegions = oilMuTable_.size();
    for (unsigned regionIdx = 0; regionIdx < numRegions; ++ regionIdx) {
        // calculate the table which stores the inverse of the product of the oil
        // formation volume factor and the oil viscosity
        const auto& oilMu = oilMuTable_[regionIdx];
        const auto& satOilMu = saturatedOilMuTable_[regionIdx];
        const auto& invOilB = inverseOilBTable_[regionIdx];
        assert(oilMu.numX() == invOilB.numX());

        auto& invOilBMu = inverseOilBMuTable_[regionIdx];
        auto& invSatOilB = inverseSaturatedOilBTable_[regionIdx];
        auto& invSatOilBMu = inverseSaturatedOilBMuTable_[regionIdx];

        std::vector<Scalar> satPressuresArray;
        std::vector<Scalar> invSatOilBArray;
        std::vector<Scalar> invSatOilBMuArray;
        for (unsigned rsIdx = 0; rsIdx < oilMu.numX(); ++rsIdx) {
            invOilBMu.appendXPos(oilMu.xAt(rsIdx));

            assert(oilMu.numY(rsIdx) == invOilB.numY(rsIdx));

            size_t numPressures = oilMu.numY(rsIdx);
            for (unsigned pIdx = 0; pIdx < numPressures; ++pIdx)
                invOilBMu.appendSamplePoint(rsIdx,
                                            oilMu.yAt(rsIdx, pIdx),
                                            invOilB.valueAt(rsIdx, pIdx)
                                            / oilMu.valueAt(rsIdx, pIdx));

            // the sampling points in UniformXTabulated2DFunction are always sorted
            // in ascending order. Thus, the value for saturated oil is the first one
            // (i.e., the one for the lowest pressure value)
            satPressuresArray.push_back(oilMu.yAt(rsIdx, 0));
            invSatOilBArray.push_back(invOilB.valueAt(rsIdx, 0));
            invSatOilBMuArray.push_back(invSatOilBArray.back()/satOilMu.valueAt(rsIdx));
        }

        invSatOilB.setXYContainers(satPressuresArray, invSatOilBArray);
        invSatOilBMu.setXYContainers(satPressuresArray, invSatOilBMuArray);

        updateSaturationPressure_(regionIdx);
    }
}

template<class Scalar>
void LiveOilPvt<Scalar>::updateSaturationPressure_(unsigned regionIdx)
{
    const auto& gasDissolutionFac = saturatedGasDissolutionFactorTable_[regionIdx];

    // create the function representing saturation pressure depending of the mass
    // fraction in gas
    size_t n = gasDissolutionFac.numSamples();
    const Scalar delta = (gasDissolutionFac.xMax() -
                          gasDissolutionFac.xMin()) / Scalar(n + 1);

    SamplingPoints pSatSamplePoints;
    for (size_t i = 0; i <= n; ++ i) {
        const Scalar pSat = gasDissolutionFac.xMin() + i*delta;
        const Scalar Rs = saturatedGasDissolutionFactor(regionIdx,
                                                        Scalar(1e30),
                                                        pSat);
        pSatSamplePoints.emplace_back(Rs, pSat);
    }

    //Prune duplicate Rs values (can occur, and will cause problems in further interpolation)
    auto x_coord_comparator = [](const auto& a, const auto& b) { return a.first == b.first; };
    auto last = std::unique(pSatSamplePoints.begin(), pSatSamplePoints.end(), x_coord_comparator);
    if (std::distance(pSatSamplePoints.begin(), last) > 1) // only remove them if there are more than two points
        pSatSamplePoints.erase(last, pSatSamplePoints.end());

    saturationPressure_[regionIdx].setContainerOfTuples(pSatSamplePoints);
}

template class LiveOilPvt<double>;
template class LiveOilPvt<float>;

} // namespace Opm
