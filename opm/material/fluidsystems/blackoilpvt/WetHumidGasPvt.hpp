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
 * \copydoc Opm::WetHumidGasPvt
 */
#ifndef OPM_WET_HUMID_GAS_PVT_HPP
#define OPM_WET_HUMID_GAS_PVT_HPP

#include <opm/material/Constants.hpp>
#include <opm/material/common/MathToolbox.hpp>
#include <opm/material/common/OpmFinal.hpp>
#include <opm/material/common/UniformXTabulated2DFunction.hpp>
#include <opm/material/common/Tabulated1DFunction.hpp>

#if HAVE_ECL_INPUT
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

#endif

#if HAVE_OPM_COMMON
#include <opm/common/OpmLog/OpmLog.hpp>
#endif

namespace Opm {

/*!
 * \brief This class represents the Pressure-Volume-Temperature relations of the gas phase
 *        with vaporized oil and vaporized water.
 */
template <class Scalar>
class WetHumidGasPvt
{
    typedef std::vector<std::pair<Scalar, Scalar> > SamplingPoints;

public:
    typedef UniformXTabulated2DFunction<Scalar> TabulatedTwoDFunction;
    typedef Tabulated1DFunction<Scalar> TabulatedOneDFunction;

    WetHumidGasPvt()
    {
        vapPar1_ = 0.0;
    }

    WetHumidGasPvt(const std::vector<Scalar>& gasReferenceDensity,
              const std::vector<Scalar>& oilReferenceDensity,
              const std::vector<Scalar>& waterReferenceDensity,
              const std::vector<TabulatedTwoDFunction>& inverseGasBRvwSat,
              const std::vector<TabulatedTwoDFunction>& inverseGasBRvSat,
              const std::vector<TabulatedOneDFunction>& inverseSaturatedGasB,
              const std::vector<TabulatedTwoDFunction>& gasMuRvwSat,
              const std::vector<TabulatedTwoDFunction>& gasMuRvSat,
              const std::vector<TabulatedTwoDFunction>& inverseGasBMuRvwSat,
              const std::vector<TabulatedTwoDFunction>& inverseGasBMuRvSat,
              const std::vector<TabulatedOneDFunction>& inverseSaturatedGasBMu,
              const std::vector<TabulatedOneDFunction>& saturatedWaterVaporizationFactorTable,
              const std::vector<TabulatedOneDFunction>& saturatedOilVaporizationFactorTable,
              const std::vector<TabulatedOneDFunction>& saturationPressure,
              Scalar vapPar1)
        : gasReferenceDensity_(gasReferenceDensity)
        , oilReferenceDensity_(oilReferenceDensity)
        , waterReferenceDensity_(waterReferenceDensity)
        , inverseGasBRvwSat_(inverseGasBRvwSat) // inverse of Bg evaluated at saturated water-gas ratio (Rvw) values; pvtg
        , inverseGasBRvSat_(inverseGasBRvSat) // inverse of Bg evaluated at saturated oil-gas ratio (Rv) values; pvtgw
        , inverseSaturatedGasB_(inverseSaturatedGasB) // evaluated at saturated water-gas ratio (Rvw) and oil-gas ratio (Rv) values; pvtgw
        , gasMuRvwSat_(gasMuRvwSat) // Mug evaluated at saturated water-gas ratio (Rvw) values; pvtg
        , gasMuRvSat_(gasMuRvSat) // Mug evaluated at saturated oil-gas ratio (Rv) values; pvtgw
        , inverseGasBMuRvwSat_(inverseGasBMuRvwSat) // Bg^-1*Mug evaluated at saturated water-gas ratio (Rvw) values; pvtg
        , inverseGasBMuRvSat_(inverseGasBMuRvSat) // Bg^-1*Mug evaluated at saturated oil-gas ratio (Rv) values; pvtgw
        , inverseSaturatedGasBMu_(inverseSaturatedGasBMu) //pvtgw
        , saturatedWaterVaporizationFactorTable_(saturatedWaterVaporizationFactorTable) //pvtgw
        , saturatedOilVaporizationFactorTable_(saturatedOilVaporizationFactorTable) //pvtg
        , saturationPressure_(saturationPressure)
        , vapPar1_(vapPar1)
    {
    }


#if HAVE_ECL_INPUT
    /*!
     * \brief Initialize the parameters for wet gas using an ECL deck.
     *
     * This method assumes that the deck features valid DENSITY and PVTG keywords.
     */
    void initFromState(const EclipseState& eclState, const Schedule& schedule)
    {
        const auto& pvtgwTables = eclState.getTableManager().getPvtgwTables();
        const auto& pvtgTables = eclState.getTableManager().getPvtgTables();
        const auto& densityTable = eclState.getTableManager().getDensityTable();

        assert(pvtgwTables.size() == densityTable.size());
        assert(pvtgTables.size() == densityTable.size());

        size_t numRegions = pvtgwTables.size();
        setNumRegions(numRegions);

        for (unsigned regionIdx = 0; regionIdx < numRegions; ++ regionIdx) {
            Scalar rhoRefO = densityTable[regionIdx].oil;
            Scalar rhoRefG = densityTable[regionIdx].gas;
            Scalar rhoRefW = densityTable[regionIdx].water;

            setReferenceDensities(regionIdx, rhoRefO, rhoRefG, rhoRefW);
        }
        // Table PVTGW
        for (unsigned regionIdx = 0; regionIdx < numRegions; ++ regionIdx) {
            const auto& pvtgwTable = pvtgwTables[regionIdx];

            const auto& saturatedTable = pvtgwTable.getSaturatedTable();
            assert(saturatedTable.numRows() > 1);

            // PVTGW table contains values at saturated Rv 
            auto& gasMuRvSat = gasMuRvSat_[regionIdx];
            auto& invGasBRvSat = inverseGasBRvSat_[regionIdx];
            auto& invSatGasB = inverseSaturatedGasB_[regionIdx];
            auto& invSatGasBMu = inverseSaturatedGasBMu_[regionIdx];
            auto& waterVaporizationFac = saturatedWaterVaporizationFactorTable_[regionIdx];

            waterVaporizationFac.setXYArrays(saturatedTable.numRows(),
                                           saturatedTable.getColumn("PG"),
                                           saturatedTable.getColumn("RW"));

            std::vector<Scalar> invSatGasBArray;
            std::vector<Scalar> invSatGasBMuArray;

            // extract the table for the gas viscosity and formation volume factors
            for (unsigned outerIdx = 0; outerIdx < saturatedTable.numRows(); ++ outerIdx) {
                Scalar pg = saturatedTable.get("PG" , outerIdx);
                Scalar B = saturatedTable.get("BG" , outerIdx);
                Scalar mu = saturatedTable.get("MUG" , outerIdx);

                invGasBRvSat.appendXPos(pg);
                gasMuRvSat.appendXPos(pg);

                invSatGasBArray.push_back(1.0/B);
                invSatGasBMuArray.push_back(1.0/(mu*B));

                assert(invGasBRvSat.numX() == outerIdx + 1);
                assert(gasMuRvSat.numX() == outerIdx + 1);

                const auto& underSaturatedTable = pvtgwTable.getUnderSaturatedTable(outerIdx);
                size_t numRows = underSaturatedTable.numRows();
                for (size_t innerIdx = 0; innerIdx < numRows; ++ innerIdx) {
                    Scalar Rw = underSaturatedTable.get("RW" , innerIdx);
                    Scalar Bg = underSaturatedTable.get("BG" , innerIdx);
                    Scalar mug = underSaturatedTable.get("MUG" , innerIdx);

                    invGasBRvSat.appendSamplePoint(outerIdx, Rw, 1.0/Bg);
                    gasMuRvSat.appendSamplePoint(outerIdx, Rw, mug);
                }
            }

            {
                std::vector<double> tmpPressure =  saturatedTable.getColumn("PG").vectorCopy( );

                invSatGasB.setXYContainers(tmpPressure, invSatGasBArray);
                invSatGasBMu.setXYContainers(tmpPressure, invSatGasBMuArray);
            }

            // make sure to have at least two sample points per gas pressure value
            for (unsigned xIdx = 0; xIdx < invGasBRvSat.numX(); ++xIdx) {
               // a single sample point is definitely needed
                assert(invGasBRvSat.numY(xIdx) > 0);

                // everything is fine if the current table has two or more sampling points
                // for a given mole fraction
                if (invGasBRvSat.numY(xIdx) > 1)
                    continue;

                // find the master table which will be used as a template to extend the
                // current line. We define master table as the first table which has values
                // for undersaturated gas...
                size_t masterTableIdx = xIdx + 1;
                for (; masterTableIdx < saturatedTable.numRows(); ++masterTableIdx)
                {
                    if (pvtgwTable.getUnderSaturatedTable(masterTableIdx).numRows() > 1)
                        break;
                }

                if (masterTableIdx >= saturatedTable.numRows())
                    throw std::runtime_error("PVTGW tables are invalid: The last table must exhibit at least one "
                              "entry for undersaturated gas!");


                // extend the current table using the master table.
                extendPvtgwTable_(regionIdx,
                                 xIdx,
                                 pvtgwTable.getUnderSaturatedTable(xIdx),
                                 pvtgwTable.getUnderSaturatedTable(masterTableIdx));
            }
        }

        // Table PVTG
        for (unsigned regionIdx = 0; regionIdx < numRegions; ++ regionIdx) {
            const auto& pvtgTable = pvtgTables[regionIdx];

            const auto& saturatedTable = pvtgTable.getSaturatedTable();
            assert(saturatedTable.numRows() > 1);
            // PVTG table contains values at saturated Rvw
            auto& gasMuRvwSat = gasMuRvwSat_[regionIdx];
            auto& invGasBRvwSat = inverseGasBRvwSat_[regionIdx];
            auto& invSatGasB = inverseSaturatedGasB_[regionIdx];
            auto& invSatGasBMu = inverseSaturatedGasBMu_[regionIdx];
            auto& oilVaporizationFac = saturatedOilVaporizationFactorTable_[regionIdx];

            oilVaporizationFac.setXYArrays(saturatedTable.numRows(),
                                           saturatedTable.getColumn("PG"),
                                           saturatedTable.getColumn("RV"));

            std::vector<Scalar> invSatGasBArray;
            std::vector<Scalar> invSatGasBMuArray;

            //// extract the table for the gas viscosity and formation volume factors
            for (unsigned outerIdx = 0; outerIdx < saturatedTable.numRows(); ++ outerIdx) {
                Scalar pg = saturatedTable.get("PG" , outerIdx);
                Scalar B = saturatedTable.get("BG" , outerIdx);
                Scalar mu = saturatedTable.get("MUG" , outerIdx);

                invGasBRvwSat.appendXPos(pg);
                gasMuRvwSat.appendXPos(pg);

                invSatGasBArray.push_back(1.0/B);
                invSatGasBMuArray.push_back(1.0/(mu*B));

                assert(invGasBRvwSat.numX() == outerIdx + 1);
                assert(gasMuRvwSat.numX() == outerIdx + 1);

                const auto& underSaturatedTable = pvtgTable.getUnderSaturatedTable(outerIdx);
                size_t numRows = underSaturatedTable.numRows();
                for (size_t innerIdx = 0; innerIdx < numRows; ++ innerIdx) {
                    Scalar Rv = underSaturatedTable.get("RV" , innerIdx);
                    Scalar Bg = underSaturatedTable.get("BG" , innerIdx);
                    Scalar mug = underSaturatedTable.get("MUG" , innerIdx);

                    invGasBRvwSat.appendSamplePoint(outerIdx, Rv, 1.0/Bg);
                    gasMuRvwSat.appendSamplePoint(outerIdx, Rv, mug);
                }
            }

            {
                std::vector<double> tmpPressure =  saturatedTable.getColumn("PG").vectorCopy( );

                invSatGasB.setXYContainers(tmpPressure, invSatGasBArray);
                invSatGasBMu.setXYContainers(tmpPressure, invSatGasBMuArray);
            }

            // make sure to have at least two sample points per gas pressure value
            for (unsigned xIdx = 0; xIdx < invGasBRvwSat.numX(); ++xIdx) {
               // a single sample point is definitely needed
                assert(invGasBRvwSat.numY(xIdx) > 0);

                // everything is fine if the current table has two or more sampling points
                // for a given mole fraction
                if (invGasBRvwSat.numY(xIdx) > 1)
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
        } //end PVTGW
        vapPar1_ = 0.0;
        const auto& oilVap = schedule[0].oilvap();
        if (oilVap.getType() == OilVaporizationProperties::OilVaporization::VAPPARS) {
            vapPar1_ = oilVap.vap1();
        }
        
        initEnd();
    }

private:
    void extendPvtgwTable_(unsigned regionIdx,
                          unsigned xIdx,
                          const SimpleTable& curTable,
                          const SimpleTable& masterTable)
    {
        std::vector<double> RvArray = curTable.getColumn("RW").vectorCopy();
        std::vector<double> gasBArray = curTable.getColumn("BG").vectorCopy();
        std::vector<double> gasMuArray = curTable.getColumn("MUG").vectorCopy();

        auto& invGasBRvSat = inverseGasBRvSat_[regionIdx];
        auto& gasMuRvSat = gasMuRvSat_[regionIdx];

        for (size_t newRowIdx = 1; newRowIdx < masterTable.numRows(); ++ newRowIdx) {
            const auto& RVColumn = masterTable.getColumn("RW");
            const auto& BGColumn = masterTable.getColumn("BG");
            const auto& viscosityColumn = masterTable.getColumn("MUG");

            // compute the gas pressure for the new entry
            Scalar diffRv = RVColumn[newRowIdx] - RVColumn[newRowIdx - 1];
            Scalar newRv = RvArray.back() + diffRv;

            // calculate the compressibility of the master table
            Scalar B1 = BGColumn[newRowIdx];
            Scalar B2 = BGColumn[newRowIdx - 1];
            Scalar x = (B1 - B2)/( (B1 + B2)/2.0 );

            // calculate the gas formation volume factor which exhibits the same
            // "compressibility" for the new value of Rv
            Scalar newBg = gasBArray.back()*(1.0 + x/2.0)/(1.0 - x/2.0);

            // calculate the "viscosibility" of the master table
            Scalar mu1 = viscosityColumn[newRowIdx];
            Scalar mu2 = viscosityColumn[newRowIdx - 1];
            Scalar xMu = (mu1 - mu2)/( (mu1 + mu2)/2.0 );

            // calculate the gas formation volume factor which exhibits the same
            // compressibility for the new pressure
            Scalar newMug = gasMuArray.back()*(1.0 + xMu/2)/(1.0 - xMu/2.0);

            // append the new values to the arrays which we use to compute the additional
            // values ...
            RvArray.push_back(newRv);
            gasBArray.push_back(newBg);
            gasMuArray.push_back(newMug);

            // ... and register them with the internal table objects
            invGasBRvSat.appendSamplePoint(xIdx, newRv, 1.0/newBg);
            gasMuRvSat.appendSamplePoint(xIdx, newRv, newMug);
        }
    }
    void extendPvtgTable_(unsigned regionIdx,
                          unsigned xIdx,
                          const SimpleTable& curTable,
                          const SimpleTable& masterTable)
    {
        std::vector<double> RvArray = curTable.getColumn("RV").vectorCopy();
        std::vector<double> gasBArray = curTable.getColumn("BG").vectorCopy();
        std::vector<double> gasMuArray = curTable.getColumn("MUG").vectorCopy();

        auto& invGasBRvwSat= inverseGasBRvwSat_[regionIdx];
        auto& gasMuRvwSat = gasMuRvwSat_[regionIdx];

        for (size_t newRowIdx = 1; newRowIdx < masterTable.numRows(); ++ newRowIdx) {
            const auto& RVColumn = masterTable.getColumn("RV");
            const auto& BGColumn = masterTable.getColumn("BG");
            const auto& viscosityColumn = masterTable.getColumn("MUG");

            // compute the gas pressure for the new entry
            Scalar diffRv = RVColumn[newRowIdx] - RVColumn[newRowIdx - 1];
            Scalar newRv = RvArray.back() + diffRv;

            // calculate the compressibility of the master table
            Scalar B1 = BGColumn[newRowIdx];
            Scalar B2 = BGColumn[newRowIdx - 1];
            Scalar x = (B1 - B2)/( (B1 + B2)/2.0 );

            // calculate the gas formation volume factor which exhibits the same
            // "compressibility" for the new value of Rv
            Scalar newBg = gasBArray.back()*(1.0 + x/2.0)/(1.0 - x/2.0);

            // calculate the "viscosibility" of the master table
            Scalar mu1 = viscosityColumn[newRowIdx];
            Scalar mu2 = viscosityColumn[newRowIdx - 1];
            Scalar xMu = (mu1 - mu2)/( (mu1 + mu2)/2.0 );

            // calculate the gas formation volume factor which exhibits the same
            // compressibility for the new pressure
            Scalar newMug = gasMuArray.back()*(1.0 + xMu/2)/(1.0 - xMu/2.0);

            // append the new values to the arrays which we use to compute the additional
            // values ...
            RvArray.push_back(newRv);
            gasBArray.push_back(newBg);
            gasMuArray.push_back(newMug);

            // ... and register them with the internal table objects
            invGasBRvwSat.appendSamplePoint(xIdx, newRv, 1.0/newBg);
            gasMuRvwSat.appendSamplePoint(xIdx, newRv, newMug);
        }
    }

public:
#endif // HAVE_ECL_INPUT

    void setNumRegions(size_t numRegions)
    {
        waterReferenceDensity_.resize(numRegions);
        oilReferenceDensity_.resize(numRegions);
        gasReferenceDensity_.resize(numRegions);
        inverseGasBRvwSat_.resize(numRegions, TabulatedTwoDFunction{TabulatedTwoDFunction::InterpolationPolicy::RightExtreme});
        inverseGasBRvSat_.resize(numRegions, TabulatedTwoDFunction{TabulatedTwoDFunction::InterpolationPolicy::RightExtreme});
        inverseGasBMuRvwSat_.resize(numRegions, TabulatedTwoDFunction{TabulatedTwoDFunction::InterpolationPolicy::RightExtreme});
        inverseGasBMuRvSat_.resize(numRegions, TabulatedTwoDFunction{TabulatedTwoDFunction::InterpolationPolicy::RightExtreme});
        inverseSaturatedGasB_.resize(numRegions);
        inverseSaturatedGasBMu_.resize(numRegions);
        gasMuRvwSat_.resize(numRegions, TabulatedTwoDFunction{TabulatedTwoDFunction::InterpolationPolicy::RightExtreme});
        gasMuRvSat_.resize(numRegions, TabulatedTwoDFunction{TabulatedTwoDFunction::InterpolationPolicy::RightExtreme});
        saturatedWaterVaporizationFactorTable_.resize(numRegions);
        saturatedOilVaporizationFactorTable_.resize(numRegions);
        saturationPressure_.resize(numRegions);
    }

    /*!
     * \brief Initialize the reference densities of all fluids for a given PVT region
     */
    void setReferenceDensities(unsigned regionIdx,
                               Scalar rhoRefOil,
                               Scalar rhoRefGas,
                               Scalar rhoRefWater)
    {
        waterReferenceDensity_[regionIdx] = rhoRefWater;
        oilReferenceDensity_[regionIdx] = rhoRefOil;
        gasReferenceDensity_[regionIdx] = rhoRefGas;
    }

    /*!
     * \brief Initialize the function for the water vaporization factor \f$R_v\f$
     *
     * \param samplePoints A container of (x,y) values.
     */
    void setSaturatedGasWaterVaporizationFactor(unsigned regionIdx, const SamplingPoints& samplePoints)
    { saturatedWaterVaporizationFactorTable_[regionIdx].setContainerOfTuples(samplePoints); }

    /*!
     * \brief Initialize the function for the oil vaporization factor \f$R_v\f$
     *
     * \param samplePoints A container of (x,y) values.
     */
    void setSaturatedGasOilVaporizationFactor(unsigned regionIdx, const SamplingPoints& samplePoints)
    { saturatedOilVaporizationFactorTable_[regionIdx].setContainerOfTuples(samplePoints); }


    /*!
     * \brief Finish initializing the gas phase PVT properties.
     */
    void initEnd()
    {

        //PVTGW
        // calculate the final 2D functions which are used for interpolation.
        size_t numRegions = gasMuRvSat_.size();
        for (unsigned regionIdx = 0; regionIdx < numRegions; ++ regionIdx) {
            // calculate the table which stores the inverse of the product of the gas
            // formation volume factor and the gas viscosity
            const auto& gasMuRvSat = gasMuRvSat_[regionIdx];
            const auto& invGasBRvSat = inverseGasBRvSat_[regionIdx];
            assert(gasMuRvSat.numX() == invGasBRvSat.numX());

            auto& invGasBMuRvSat = inverseGasBMuRvSat_[regionIdx];
            auto& invSatGasB = inverseSaturatedGasB_[regionIdx];
            auto& invSatGasBMu = inverseSaturatedGasBMu_[regionIdx];

            std::vector<Scalar> satPressuresArray;
            std::vector<Scalar> invSatGasBArray;
            std::vector<Scalar> invSatGasBMuArray;
            for (size_t pIdx = 0; pIdx < gasMuRvSat.numX(); ++pIdx) {
                invGasBMuRvSat.appendXPos(gasMuRvSat.xAt(pIdx));

                assert(gasMuRvSat.numY(pIdx) == invGasBRvSat.numY(pIdx));

                size_t numRw = gasMuRvSat.numY(pIdx);
                for (size_t RwIdx = 0; RwIdx < numRw; ++RwIdx)
                    invGasBMuRvSat.appendSamplePoint(pIdx,
                                                gasMuRvSat.yAt(pIdx, RwIdx),
                                                invGasBRvSat.valueAt(pIdx, RwIdx)
                                                / gasMuRvSat.valueAt(pIdx, RwIdx));

                // the sampling points in UniformXTabulated2DFunction are always sorted
                // in ascending order. Thus, the value for saturated gas is the last one
                // (i.e., the one with the largest Rw value)
                satPressuresArray.push_back(gasMuRvSat.xAt(pIdx));
                invSatGasBArray.push_back(invGasBRvSat.valueAt(pIdx, numRw - 1));
                invSatGasBMuArray.push_back(invGasBMuRvSat.valueAt(pIdx, numRw - 1));
            }

            invSatGasB.setXYContainers(satPressuresArray, invSatGasBArray);
            invSatGasBMu.setXYContainers(satPressuresArray, invSatGasBMuArray);
        }

        //PVTG
        // calculate the final 2D functions which are used for interpolation.
        //size_t numRegions = gasMuRvwSat_.size();
        for (unsigned regionIdx = 0; regionIdx < numRegions; ++ regionIdx) {
            // calculate the table which stores the inverse of the product of the gas
            // formation volume factor and the gas viscosity
            const auto& gasMuRvwSat = gasMuRvwSat_[regionIdx];
            const auto& invGasBRvwSat = inverseGasBRvwSat_[regionIdx];
            assert(gasMuRvwSat.numX() == invGasBRvwSat.numX());

            auto& invGasBMuRvwSat = inverseGasBMuRvwSat_[regionIdx];
            auto& invSatGasB = inverseSaturatedGasB_[regionIdx];
            auto& invSatGasBMu = inverseSaturatedGasBMu_[regionIdx];

            std::vector<Scalar> satPressuresArray;
            std::vector<Scalar> invSatGasBArray;
            std::vector<Scalar> invSatGasBMuArray;
            for (size_t pIdx = 0; pIdx < gasMuRvwSat.numX(); ++pIdx) {
                invGasBMuRvwSat.appendXPos(gasMuRvwSat.xAt(pIdx));

                assert(gasMuRvwSat.numY(pIdx) == invGasBRvwSat.numY(pIdx));

                size_t numRw = gasMuRvwSat.numY(pIdx);
                for (size_t RwIdx = 0; RwIdx < numRw; ++RwIdx)
                    invGasBMuRvwSat.appendSamplePoint(pIdx,
                                                gasMuRvwSat.yAt(pIdx, RwIdx),
                                                invGasBRvwSat.valueAt(pIdx, RwIdx)
                                                / gasMuRvwSat.valueAt(pIdx, RwIdx));

                // the sampling points in UniformXTabulated2DFunction are always sorted
                // in ascending order. Thus, the value for saturated gas is the last one
                // (i.e., the one with the largest Rw value)
                satPressuresArray.push_back(gasMuRvwSat.xAt(pIdx));
                invSatGasBArray.push_back(invGasBRvwSat.valueAt(pIdx, numRw - 1));
                invSatGasBMuArray.push_back(invGasBMuRvwSat.valueAt(pIdx, numRw - 1));
            }

            invSatGasB.setXYContainers(satPressuresArray, invSatGasBArray);
            invSatGasBMu.setXYContainers(satPressuresArray, invSatGasBMuArray);
            
            updateSaturationPressure_(regionIdx);
        }
    }

    /*!
     * \brief Return the number of PVT regions which are considered by this PVT-object.
     */
    unsigned numRegions() const
    { return gasReferenceDensity_.size(); }

    /*!
     * \brief Returns the specific enthalpy [J/kg] of gas given a set of parameters.
     */
    template <class Evaluation>
    Evaluation internalEnergy(unsigned,
                        const Evaluation&,
                        const Evaluation&,
                        const Evaluation&) const
    {
        throw std::runtime_error("Requested the enthalpy of gas but the thermal option is not enabled");
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of the fluid phase given a set of parameters.
     */
    template <class Evaluation>
    Evaluation viscosity(unsigned regionIdx,
                         const Evaluation& /*temperature*/,
                         const Evaluation& pressure,
                         const Evaluation& Rv,
                         const Evaluation& Rvw) const
    {
        const Evaluation& temperature = 1E30;

        if (Rv >= (1.0 - 1e-10)*saturatedOilVaporizationFactor(regionIdx, temperature, pressure)) {
            const Evaluation& invBg = inverseGasBRvSat_[regionIdx].eval(pressure, Rvw, /*extrapolate=*/true);
            const Evaluation& invMugBg = inverseGasBMuRvSat_[regionIdx].eval(pressure, Rvw, /*extrapolate=*/true);
            return invBg/invMugBg;
        }
        else {
            // for Rv undersaturated viscosity is evaluated at saturated Rvw values
            const Evaluation& invBg = inverseGasBRvwSat_[regionIdx].eval(pressure, Rv, /*extrapolate=*/true);
            const Evaluation& invMugBg = inverseGasBMuRvwSat_[regionIdx].eval(pressure, Rv, /*extrapolate=*/true);
            return invBg/invMugBg;
        }
    }

    /*!
     * \brief Returns the dynamic viscosity [Pa s] of oil saturated gas at a given pressure.
     */
    template <class Evaluation>
    Evaluation saturatedViscosity(unsigned regionIdx,
                                  const Evaluation& /*temperature*/,
                                  const Evaluation& pressure) const
    {
        const Evaluation& invBg = inverseSaturatedGasB_[regionIdx].eval(pressure, /*extrapolate=*/true);
        const Evaluation& invMugBg = inverseSaturatedGasBMu_[regionIdx].eval(pressure, /*extrapolate=*/true);

        return invBg/invMugBg;
    }

    /*!
     * \brief Returns the formation volume factor [-] of the fluid phase.
     */
    // template <class Evaluation>
    // Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
    //                                         const Evaluation& /*temperature*/,
    //                                         const Evaluation& pressure,
    //                                         const Evaluation& Rw) const
    // { return inverseGasB_[regionIdx].eval(pressure, Rw, /*extrapolate=*/true); }

    template <class Evaluation>
    Evaluation inverseFormationVolumeFactor(unsigned regionIdx,
                                            const Evaluation& /*temperature*/,
                                            const Evaluation& pressure,
                                            const Evaluation& Rv,
                                            const Evaluation& Rvw) const
    {
        const Evaluation& temperature = 1E30;
        
        if (Rv >= (1.0 - 1e-10)*saturatedOilVaporizationFactor(regionIdx, temperature, pressure)) {
            return inverseGasBRvSat_[regionIdx].eval(pressure, Rvw, /*extrapolate=*/true);
        }
        else {
            // for Rv undersaturated Bg^-1 is evaluated at saturated Rvw values
            return inverseGasBRvwSat_[regionIdx].eval(pressure, Rv, /*extrapolate=*/true);
        }

    }


    /*!
     * \brief Returns the formation volume factor [-] of water saturated gas at a given pressure.
     */
    template <class Evaluation>
    Evaluation saturatedInverseFormationVolumeFactor(unsigned regionIdx,
                                                     const Evaluation& /*temperature*/,
                                                     const Evaluation& pressure) const
    { return inverseSaturatedGasB_[regionIdx].eval(pressure, /*extrapolate=*/true); }

    /*!
     * \brief Returns the water vaporization factor \f$R_vw\f$ [m^3/m^3] of the water phase.
     */
    template <class Evaluation>
    Evaluation saturatedWaterVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& /*temperature*/,
                                              const Evaluation& pressure) const
    {
        return saturatedWaterVaporizationFactorTable_[regionIdx].eval(pressure, /*extrapolate=*/true);
    }

   template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& /*temperature*/,
                                              const Evaluation& pressure) const
    {
        return saturatedOilVaporizationFactorTable_[regionIdx].eval(pressure, /*extrapolate=*/true);
    }

    /*!
     * \brief Returns the oil vaporization factor \f$R_v\f$ [m^3/m^3] of the gas phase.
     *
     * This variant of the method prevents all the oil to be vaporized even if the gas
     * phase is still not saturated. This is physically quite dubious but it corresponds
     * to how the Eclipse 100 simulator handles this. (cf the VAPPARS keyword.)
     */
    template <class Evaluation>
    Evaluation saturatedOilVaporizationFactor(unsigned regionIdx,
                                              const Evaluation& /*temperature*/,
                                              const Evaluation& pressure,
                                              const Evaluation& oilSaturation,
                                              Evaluation maxOilSaturation) const
    {
        Evaluation tmp =
            saturatedOilVaporizationFactorTable_[regionIdx].eval(pressure, /*extrapolate=*/true);

        // apply the vaporization parameters for the gas phase (cf. the Eclipse VAPPARS
        // keyword)
        maxOilSaturation = min(maxOilSaturation, Scalar(1.0));
        if (vapPar1_ > 0.0 && maxOilSaturation > 0.01 && oilSaturation < maxOilSaturation) {
            static const Scalar eps = 0.001;
            const Evaluation& So = max(oilSaturation, eps);
            tmp *= max(1e-3, pow(So/maxOilSaturation, vapPar1_));
        }

        return tmp;
    }

    /*!
     * \brief Returns the saturation pressure of the gas phase [Pa]
     *        depending on its mass fraction of the water component
     *
     * \param Rw The surface volume of water component dissolved in what will yield one
     *           cubic meter of gas at the surface [-]
     */
    //PJPE assume dependence on Rv
    template <class Evaluation>
    Evaluation saturationPressure(unsigned regionIdx,
                                  const Evaluation&,
                                  const Evaluation& Rw) const
    {
        typedef MathToolbox<Evaluation> Toolbox;

        const auto& RwTable = saturatedWaterVaporizationFactorTable_[regionIdx];
        const Scalar eps = std::numeric_limits<typename Toolbox::Scalar>::epsilon()*1e6;

        // use the tabulated saturation pressure function to get a pretty good initial value
        Evaluation pSat = saturationPressure_[regionIdx].eval(Rw, /*extrapolate=*/true);

        // Newton method to do the remaining work. If the initial
        // value is good, this should only take two to three
        // iterations...
        bool onProbation = false;
        for (unsigned i = 0; i < 20; ++i) {
            const Evaluation& f = RwTable.eval(pSat, /*extrapolate=*/true) - Rw;
            const Evaluation& fPrime = RwTable.evalDerivative(pSat, /*extrapolate=*/true);

            // If the derivative is "zero" Newton will not converge,
            // so simply return our initial guess.
            if (std::abs(scalarValue(fPrime)) < 1.0e-30) {
                return pSat;
            }

            const Evaluation& delta = f/fPrime;

            pSat -= delta;

            if (pSat < 0.0) {
                // if the pressure is lower than 0 Pascals, we set it back to 0. if this
                // happens twice, we give up and just return 0 Pa...
                if (onProbation)
                    return 0.0;

                onProbation = true;
                pSat = 0.0;
            }

            if (std::abs(scalarValue(delta)) < std::abs(scalarValue(pSat))*eps)
                return pSat;
        }

        std::stringstream errlog;
        errlog << "Finding saturation pressure did not converge:"
               << " pSat = " << pSat
               << ", Rw = " << Rw;
#if HAVE_OPM_COMMON
        OpmLog::debug("Wet gas saturation pressure", errlog.str());
#endif
        throw NumericalIssue(errlog.str());
    }

    template <class Evaluation>
    Evaluation diffusionCoefficient(const Evaluation& /*temperature*/,
                                    const Evaluation& /*pressure*/,
                                    unsigned /*compIdx*/) const
    {
        throw std::runtime_error("Not implemented: The PVT model does not provide a diffusionCoefficient()");
    }

    const Scalar gasReferenceDensity(unsigned regionIdx) const
    { return gasReferenceDensity_[regionIdx]; }

     const Scalar oilReferenceDensity(unsigned regionIdx) const
    { return oilReferenceDensity_[regionIdx]; }

    const Scalar waterReferenceDensity(unsigned regionIdx) const
    { return waterReferenceDensity_[regionIdx]; }

    const std::vector<TabulatedTwoDFunction>& inverseGasB() const {
        return inverseGasBRvSat_;
    }

    const std::vector<TabulatedOneDFunction>& inverseSaturatedGasB() const {
        return inverseSaturatedGasB_;
    }

    const std::vector<TabulatedTwoDFunction>& gasMu() const {
        return gasMuRvSat_;
    }

    const std::vector<TabulatedTwoDFunction>& inverseGasBMu() const {
        return inverseGasBMuRvSat_;
    }

    const std::vector<TabulatedOneDFunction>& inverseSaturatedGasBMu() const {
        return inverseSaturatedGasBMu_;
    }

    const std::vector<TabulatedOneDFunction>& saturatedWaterVaporizationFactorTable() const {
        return saturatedWaterVaporizationFactorTable_;
    }
     
    const std::vector<TabulatedOneDFunction>& saturatedOilVaporizationFactorTable() const {
        return saturatedOilVaporizationFactorTable_;
    }


    const std::vector<TabulatedOneDFunction>& saturationPressure() const {
        return saturationPressure_;
    }

    Scalar vapPar1() const {
        return vapPar1_;
    }

    bool operator==(const WetHumidGasPvt<Scalar>& data) const
    {
        return this->gasReferenceDensity_ == data.gasReferenceDensity_ &&
               this->oilReferenceDensity_ == data.oilReferenceDensity_ &&
               this->waterReferenceDensity_ == data.waterReferenceDensity_ &&
               this->inverseGasB() == data.inverseGasB() &&
               this->inverseSaturatedGasB() == data.inverseSaturatedGasB() &&
               this->gasMu() == data.gasMu() &&
               this->inverseGasBMu() == data.inverseGasBMu() &&
               this->inverseSaturatedGasBMu() == data.inverseSaturatedGasBMu() &&
               this->saturatedWaterVaporizationFactorTable() == data.saturatedWaterVaporizationFactorTable() &&
               this->saturationPressure() == data.saturationPressure() &&
               this->vapPar1() == data.vapPar1();
    }

private:
    void updateSaturationPressure_(unsigned regionIdx)
    {
        typedef std::pair<Scalar, Scalar> Pair;
        const auto& oilVaporizationFac = saturatedOilVaporizationFactorTable_[regionIdx];

        // create the taublated function representing saturation pressure depending of
        // Rv
        size_t n = oilVaporizationFac.numSamples();
        Scalar delta = (oilVaporizationFac.xMax() - oilVaporizationFac.xMin())/Scalar(n + 1);

        SamplingPoints pSatSamplePoints;
        Scalar Rv = 0;
        for (size_t i = 0; i <= n; ++ i) {
            Scalar pSat = oilVaporizationFac.xMin() + Scalar(i)*delta;
            Rv = saturatedOilVaporizationFactor(regionIdx, /*temperature=*/Scalar(1e30), pSat);

            Pair val(Rv, pSat);
            pSatSamplePoints.push_back(val);
        }

        //Prune duplicate Rv values (can occur, and will cause problems in further interpolation)
        auto x_coord_comparator = [](const Pair& a, const Pair& b) { return a.first == b.first; };
        auto last = std::unique(pSatSamplePoints.begin(), pSatSamplePoints.end(), x_coord_comparator);
        pSatSamplePoints.erase(last, pSatSamplePoints.end());

        saturationPressure_[regionIdx].setContainerOfTuples(pSatSamplePoints);
    }

    std::vector<Scalar> gasReferenceDensity_;
    std::vector<Scalar> waterReferenceDensity_;
    std::vector<Scalar> oilReferenceDensity_;
    std::vector<TabulatedTwoDFunction> inverseGasBRvwSat_;
    std::vector<TabulatedTwoDFunction> inverseGasBRvSat_;
    std::vector<TabulatedOneDFunction> inverseSaturatedGasB_;
    std::vector<TabulatedTwoDFunction> gasMuRvwSat_;
    std::vector<TabulatedTwoDFunction> gasMuRvSat_;
    std::vector<TabulatedTwoDFunction> inverseGasBMuRvwSat_;
    std::vector<TabulatedTwoDFunction> inverseGasBMuRvSat_;
    std::vector<TabulatedOneDFunction> inverseSaturatedGasBMu_;
    std::vector<TabulatedOneDFunction> saturatedWaterVaporizationFactorTable_;
    std::vector<TabulatedOneDFunction> saturatedOilVaporizationFactorTable_;
    std::vector<TabulatedOneDFunction> saturationPressure_;

    Scalar vapPar1_;
};

} // namespace Opm

#endif
