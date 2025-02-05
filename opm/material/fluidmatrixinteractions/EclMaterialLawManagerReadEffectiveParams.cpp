// -*- mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
// vi: set et ts=4 sw=4 sts=4:
/*
  Copyright 2022 Equinor ASA.

  This file is part of the Open Porous Media project (OPM).

  OPM is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  OPM is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with OPM.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <config.h>
#include <opm/material/fluidmatrixinteractions/EclMaterialLawManager.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SgfnTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SgofTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SgwfnTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SlgofTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/Sof2Table.hpp>
#include <opm/input/eclipse/EclipseState/Tables/Sof3Table.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SwfnTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/SwofTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/GsfTable.hpp>
#include <opm/input/eclipse/EclipseState/Tables/WsfTable.hpp>

#include <opm/input/eclipse/EclipseState/Tables/TableManager.hpp>

namespace Opm {

/* constructors*/
template <class Traits>
EclMaterialLawManager<Traits>::InitParams::ReadEffectiveParams::
ReadEffectiveParams(EclMaterialLawManager<Traits>::InitParams& init_params) :
    init_params_{init_params}, parent_{init_params_.parent_},
    eclState_{init_params_.eclState_}
{
}

/* public methods */
template <class Traits>
void
EclMaterialLawManager<Traits>::InitParams::ReadEffectiveParams::
read() {
    auto& gasOilVector = this->parent_.gasOilEffectiveParamVector_;
    auto& oilWaterVector = this->parent_.oilWaterEffectiveParamVector_;
    auto& gasWaterVector = this->parent_.gasWaterEffectiveParamVector_;
    const size_t numSatRegions = this->eclState_.runspec().tabdims().getNumSatTables();
    gasOilVector.resize(numSatRegions);
    oilWaterVector.resize(numSatRegions);
    gasWaterVector.resize(numSatRegions);
    for (unsigned satRegionIdx = 0; satRegionIdx < numSatRegions; ++satRegionIdx) {
        readGasOilParameters_(gasOilVector, satRegionIdx);
        readOilWaterParameters_(oilWaterVector, satRegionIdx);
        readGasWaterParameters_(gasWaterVector, satRegionIdx);
    }

}

/* private methods, alphabetically sorted*/

// Relative permeability values not strictly greater than 'tolcrit' treated as zero.
template <class Traits>
std::vector<double>
EclMaterialLawManager<Traits>::InitParams::ReadEffectiveParams::
normalizeKrValues_(const double tolcrit, const TableColumn& krValues) const
{
    auto kr = krValues.vectorCopy();
    std::transform(kr.begin(), kr.end(), kr.begin(),
        [tolcrit](const double kri)
    {
        return (kri > tolcrit) ? kri : 0.0;
    });

    return kr;
}

template <class Traits>
void
EclMaterialLawManager<Traits>::InitParams::ReadEffectiveParams::
readGasOilParameters_(GasOilEffectiveParamVector& dest, unsigned satRegionIdx)
{
    if (!this->parent_.hasGas || !this->parent_.hasOil)
        // we don't read anything if either the gas or the oil phase is not active
        return;

    dest[satRegionIdx] = std::make_shared<GasOilEffectiveTwoPhaseParams>();

    auto& effParams = *dest[satRegionIdx];

    // the situation for the gas phase is complicated that all saturations are
    // shifted by the connate water saturation.
    const Scalar Swco = this->parent_.unscaledEpsInfo_[satRegionIdx].Swl;
    const auto tolcrit = this->eclState_.runspec().saturationFunctionControls()
        .minimumRelpermMobilityThreshold();

    const auto& tableManager = this->eclState_.getTableManager();

    switch (this->eclState_.runspec().saturationFunctionControls().family()) {
    case SatFuncControls::KeywordFamily::Family_I:
    {
        const TableContainer& sgofTables = tableManager.getSgofTables();
        const TableContainer& slgofTables = tableManager.getSlgofTables();
        if (!sgofTables.empty())
            readGasOilSgof_(effParams, Swco, tolcrit, sgofTables.template getTable<SgofTable>(satRegionIdx));
        else if (!slgofTables.empty())
            readGasOilSlgof_(effParams, Swco, tolcrit, slgofTables.template getTable<SlgofTable>(satRegionIdx));
        else if ( !tableManager.getSgofletTable().empty() ) {
            const auto& letSgofTab = tableManager.getSgofletTable()[satRegionIdx];
            const std::vector<Scalar> dum; // dummy arg to comform with existing interface

            effParams.setApproach(SatCurveMultiplexerApproach::LET);
            auto& realParams = effParams.template getRealParams<SatCurveMultiplexerApproach::LET>();

            // S=(So-Sogcr)/(1-Sogcr-Sgcr-Swco),  krog = Krt*S^L/[S^L+E*(1.0-S)^T]
            const Scalar s_min_w = letSgofTab.s2_critical;
            const Scalar s_max_w = 1.0-letSgofTab.s1_critical-Swco;
            const std::vector<Scalar>& letCoeffsOil = {s_min_w, s_max_w,
                                                        static_cast<Scalar>(letSgofTab.l2_relperm),
                                                        static_cast<Scalar>(letSgofTab.e2_relperm),
                                                        static_cast<Scalar>(letSgofTab.t2_relperm),
                                                        static_cast<Scalar>(letSgofTab.krt2_relperm)};
            realParams.setKrwSamples(letCoeffsOil, dum);

            // S=(1-So-Sgcr-Swco)/(1-Sogcr-Sgcr-Swco), krg = Krt*S^L/[S^L+E*(1.0-S)^T]
            const Scalar s_min_nw = letSgofTab.s1_critical+Swco;
            const Scalar s_max_nw = 1.0-letSgofTab.s2_critical;
            const std::vector<Scalar>& letCoeffsGas = {s_min_nw, s_max_nw,
                                                        static_cast<Scalar>(letSgofTab.l1_relperm),
                                                        static_cast<Scalar>(letSgofTab.e1_relperm),
                                                        static_cast<Scalar>(letSgofTab.t1_relperm),
                                                        static_cast<Scalar>(letSgofTab.krt1_relperm)};
            realParams.setKrnSamples(letCoeffsGas, dum);

            // S=(So-Sorg)/(1-Sorg-Sgl-Swco), Pc = Pct + (pcir_pc-Pct)*(1-S)^L/[(1-S)^L+E*S^T]
            const std::vector<Scalar>& letCoeffsPc = {static_cast<Scalar>(letSgofTab.s2_residual),
                                                        static_cast<Scalar>(letSgofTab.s1_residual+Swco),
                                                        static_cast<Scalar>(letSgofTab.l_pc),
                                                        static_cast<Scalar>(letSgofTab.e_pc),
                                                        static_cast<Scalar>(letSgofTab.t_pc),
                                                        static_cast<Scalar>(letSgofTab.pcir_pc),
                                                        static_cast<Scalar>(letSgofTab.pct_pc)};
            realParams.setPcnwSamples(letCoeffsPc, dum);

            realParams.finalize();
        }
        break;
    }

    case SatFuncControls::KeywordFamily::Family_II:
    {
        const SgfnTable& sgfnTable = tableManager.getSgfnTables().template getTable<SgfnTable>( satRegionIdx );
        if (!this->parent_.hasWater) {
            // oil and gas case
            const Sof2Table& sof2Table = tableManager.getSof2Tables().template getTable<Sof2Table>( satRegionIdx );
            readGasOilFamily2_(effParams, Swco, tolcrit, sof2Table, sgfnTable, /*columnName=*/"KRO");
        }
        else {
            const Sof3Table& sof3Table = tableManager.getSof3Tables().template getTable<Sof3Table>( satRegionIdx );
            readGasOilFamily2_(effParams, Swco, tolcrit, sof3Table, sgfnTable, /* columnName=*/"KROG");
        }
        break;
    }

    case SatFuncControls::KeywordFamily::Family_III:
    {
        throw std::domain_error("Saturation keyword family III is not applicable for a gas-oil system");
    }

    case SatFuncControls::KeywordFamily::Undefined:
        throw std::domain_error("No valid saturation keyword family specified");
    }
}

template <class Traits>
template <class TableType>
void
EclMaterialLawManager<Traits>::InitParams::ReadEffectiveParams::
readGasOilFamily2_(GasOilEffectiveTwoPhaseParams& effParams,
                        const Scalar Swco,
                        const double tolcrit,
                        const TableType& sofTable,
                        const SgfnTable& sgfnTable,
                        const std::string& columnName)
{
    // convert the saturations of the SGFN keyword from gas to oil saturations
    std::vector<double> SoSamples(sgfnTable.numRows());
    std::vector<double> SoColumn = sofTable.getColumn("SO").vectorCopy();
    for (size_t sampleIdx = 0; sampleIdx < sgfnTable.numRows(); ++ sampleIdx) {
        SoSamples[sampleIdx] = (1.0 - Swco) - sgfnTable.get("SG", sampleIdx);
    }

    effParams.setApproach(SatCurveMultiplexerApproach::PiecewiseLinear);
    auto& realParams = effParams.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinear>();

    realParams.setKrwSamples(SoColumn, normalizeKrValues_(tolcrit, sofTable.getColumn(columnName)));
    realParams.setKrnSamples(SoSamples, normalizeKrValues_(tolcrit, sgfnTable.getColumn("KRG")));
    realParams.setPcnwSamples(SoSamples, sgfnTable.getColumn("PCOG").vectorCopy());
    realParams.finalize();
}

template <class Traits>
void
EclMaterialLawManager<Traits>::InitParams::ReadEffectiveParams::
readGasOilSgof_(GasOilEffectiveTwoPhaseParams& effParams,
                        const Scalar Swco,
                        const double tolcrit,
                        const SgofTable& sgofTable)
{
    // convert the saturations of the SGOF keyword from gas to oil saturations
    std::vector<double> SoSamples(sgofTable.numRows());
    for (size_t sampleIdx = 0; sampleIdx < sgofTable.numRows(); ++ sampleIdx) {
        SoSamples[sampleIdx] = (1.0 - Swco) - sgofTable.get("SG", sampleIdx);
    }

    effParams.setApproach(SatCurveMultiplexerApproach::PiecewiseLinear);
    auto& realParams = effParams.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinear>();

    realParams.setKrwSamples(SoSamples, normalizeKrValues_(tolcrit, sgofTable.getColumn("KROG")));
    realParams.setKrnSamples(SoSamples, normalizeKrValues_(tolcrit, sgofTable.getColumn("KRG")));
    realParams.setPcnwSamples(SoSamples, sgofTable.getColumn("PCOG").vectorCopy());
    realParams.finalize();
}

template <class Traits>
void
EclMaterialLawManager<Traits>::InitParams::ReadEffectiveParams::
readGasOilSlgof_(GasOilEffectiveTwoPhaseParams& effParams,
                        const Scalar Swco,
                        const double tolcrit,
                        const SlgofTable& slgofTable)
{
    // convert the saturations of the SLGOF keyword from "liquid" to oil saturations
    std::vector<double> SoSamples(slgofTable.numRows());
    for (size_t sampleIdx = 0; sampleIdx < slgofTable.numRows(); ++ sampleIdx) {
        SoSamples[sampleIdx] = slgofTable.get("SL", sampleIdx) - Swco;
    }

    effParams.setApproach(SatCurveMultiplexerApproach::PiecewiseLinear);
    auto& realParams = effParams.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinear>();

    realParams.setKrwSamples(SoSamples, normalizeKrValues_(tolcrit, slgofTable.getColumn("KROG")));
    realParams.setKrnSamples(SoSamples, normalizeKrValues_(tolcrit, slgofTable.getColumn("KRG")));
    realParams.setPcnwSamples(SoSamples, slgofTable.getColumn("PCOG").vectorCopy());
    realParams.finalize();
}

template <class Traits>
void
EclMaterialLawManager<Traits>::InitParams::ReadEffectiveParams::
readGasWaterParameters_(GasWaterEffectiveParamVector& dest, unsigned satRegionIdx)
{
    if (!this->parent_.hasGas || !this->parent_.hasWater || this->parent_.hasOil)
        // we don't read anything if either the gas or the water phase is not active or if oil is present
        return;

    dest[satRegionIdx] = std::make_shared<GasWaterEffectiveTwoPhaseParams>();

    auto& effParams = *dest[satRegionIdx];

    const auto tolcrit = this->eclState_.runspec().saturationFunctionControls()
        .minimumRelpermMobilityThreshold();

    const auto& tableManager = this->eclState_.getTableManager();

    switch (this->eclState_.runspec().saturationFunctionControls().family()) {
    case SatFuncControls::KeywordFamily::Family_I:
    {
        throw std::domain_error("Saturation keyword family I is not applicable for a gas-water system");
    }

    case SatFuncControls::KeywordFamily::Family_II:
    {
        const TableContainer& sgwfnTables = tableManager.getSgwfnTables();
        effParams.setApproach(SatCurveMultiplexerApproach::PiecewiseLinear);
        auto& realParams = effParams.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinear>();
        if (!sgwfnTables.empty()){
            const SgwfnTable& sgwfnTable = tableManager.getSgwfnTables().template getTable<SgwfnTable>( satRegionIdx );
            std::vector<double> SwSamples(sgwfnTable.numRows());
            for (size_t sampleIdx = 0; sampleIdx < sgwfnTable.numRows(); ++ sampleIdx)
                SwSamples[sampleIdx] = 1 - sgwfnTable.get("SG", sampleIdx);
            realParams.setKrwSamples(SwSamples, normalizeKrValues_(tolcrit, sgwfnTable.getColumn("KRGW")));
            realParams.setKrnSamples(SwSamples, normalizeKrValues_(tolcrit, sgwfnTable.getColumn("KRG")));
            realParams.setPcnwSamples(SwSamples, sgwfnTable.getColumn("PCGW").vectorCopy());
        }
        else {
            const SgfnTable& sgfnTable = tableManager.getSgfnTables().template getTable<SgfnTable>( satRegionIdx );
            const SwfnTable& swfnTable = tableManager.getSwfnTables().template getTable<SwfnTable>( satRegionIdx );

            std::vector<double> SwColumn = swfnTable.getColumn("SW").vectorCopy();

            realParams.setKrwSamples(SwColumn, normalizeKrValues_(tolcrit, swfnTable.getColumn("KRW")));
            std::vector<double> SwSamples(sgfnTable.numRows());
            for (size_t sampleIdx = 0; sampleIdx < sgfnTable.numRows(); ++ sampleIdx)
                SwSamples[sampleIdx] = 1 - sgfnTable.get("SG", sampleIdx);
            realParams.setKrnSamples(SwSamples, normalizeKrValues_(tolcrit, sgfnTable.getColumn("KRG")));
            //Capillary pressure is read from SWFN.
            //For gas-water system the capillary pressure column values are set to 0 in SGFN
            realParams.setPcnwSamples(SwColumn, swfnTable.getColumn("PCOW").vectorCopy());
        }
        realParams.finalize();
        break;
    }

    case SatFuncControls::KeywordFamily::Family_III:
    {
        const GsfTable& gsfTable = tableManager.getGsfTables().template getTable<GsfTable>( satRegionIdx );
        const WsfTable& wsfTable = tableManager.getWsfTables().template getTable<WsfTable>( satRegionIdx );

        effParams.setApproach(SatCurveMultiplexerApproach::PiecewiseLinear);
        auto& realParams = effParams.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinear>();

        std::vector<double> SwColumn = wsfTable.getColumn("SW").vectorCopy();

        realParams.setKrwSamples(SwColumn, normalizeKrValues_(tolcrit, wsfTable.getColumn("KRW")));
        std::vector<double> SwSamples(gsfTable.numRows());
        for (size_t sampleIdx = 0; sampleIdx < gsfTable.numRows(); ++ sampleIdx)
            SwSamples[sampleIdx] = 1 - gsfTable.get("SG", sampleIdx);
        realParams.setKrnSamples(SwSamples, normalizeKrValues_(tolcrit, gsfTable.getColumn("KRG")));
        //Capillary pressure is read from GSF.
        realParams.setPcnwSamples(SwSamples, gsfTable.getColumn("PCGW").vectorCopy());
        realParams.finalize();

        break;
    }
    case SatFuncControls::KeywordFamily::Undefined:
        throw std::domain_error("No valid saturation keyword family specified");
    }
}

template <class Traits>
void
EclMaterialLawManager<Traits>::InitParams::ReadEffectiveParams::
readOilWaterParameters_(OilWaterEffectiveParamVector& dest, unsigned satRegionIdx)
{
    if (!this->parent_.hasOil || !this->parent_.hasWater)
        // we don't read anything if either the water or the oil phase is not active
        return;

    dest[satRegionIdx] = std::make_shared<OilWaterEffectiveTwoPhaseParams>();

    const auto tolcrit = this->eclState_.runspec().saturationFunctionControls()
        .minimumRelpermMobilityThreshold();

    const auto& tableManager = this->eclState_.getTableManager();
    auto& effParams = *dest[satRegionIdx];

    switch (this->eclState_.runspec().saturationFunctionControls().family()) {
    case SatFuncControls::KeywordFamily::Family_I:
    {
        if (tableManager.hasTables("SWOF")) {
            const auto& swofTable = tableManager.getSwofTables().template getTable<SwofTable>(satRegionIdx);
            const std::vector<double> SwColumn = swofTable.getColumn("SW").vectorCopy();

            effParams.setApproach(SatCurveMultiplexerApproach::PiecewiseLinear);
            auto& realParams = effParams.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinear>();

            realParams.setKrwSamples(SwColumn, normalizeKrValues_(tolcrit, swofTable.getColumn("KRW")));
            realParams.setKrnSamples(SwColumn, normalizeKrValues_(tolcrit, swofTable.getColumn("KROW")));
            realParams.setPcnwSamples(SwColumn, swofTable.getColumn("PCOW").vectorCopy());
            realParams.finalize();
        }
        else if ( !tableManager.getSwofletTable().empty() ) {
            const auto& letTab = tableManager.getSwofletTable()[satRegionIdx];
            const std::vector<Scalar> dum; // dummy arg to conform with existing interface

            effParams.setApproach(SatCurveMultiplexerApproach::LET);
            auto& realParams = effParams.template getRealParams<SatCurveMultiplexerApproach::LET>();

            // S=(Sw-Swcr)/(1-Sowcr-Swcr),  krw = Krt*S^L/[S^L+E*(1.0-S)^T]
            const Scalar s_min_w = letTab.s1_critical;
            const Scalar s_max_w = 1.0-letTab.s2_critical;
            const std::vector<Scalar>& letCoeffsWat = {s_min_w, s_max_w,
                                                        static_cast<Scalar>(letTab.l1_relperm),
                                                        static_cast<Scalar>(letTab.e1_relperm),
                                                        static_cast<Scalar>(letTab.t1_relperm),
                                                        static_cast<Scalar>(letTab.krt1_relperm)};
            realParams.setKrwSamples(letCoeffsWat, dum);

            // S=(So-Sowcr)/(1-Sowcr-Swcr), krow = Krt*S^L/[S^L+E*(1.0-S)^T]
            const Scalar s_min_nw = letTab.s2_critical;
            const Scalar s_max_nw = 1.0-letTab.s1_critical;
            const std::vector<Scalar>& letCoeffsOil = {s_min_nw, s_max_nw,
                                                        static_cast<Scalar>(letTab.l2_relperm),
                                                        static_cast<Scalar>(letTab.e2_relperm),
                                                        static_cast<Scalar>(letTab.t2_relperm),
                                                        static_cast<Scalar>(letTab.krt2_relperm)};
            realParams.setKrnSamples(letCoeffsOil, dum);

            // S=(Sw-Swco)/(1-Swco-Sorw), Pc = Pct + (Pcir-Pct)*(1-S)^L/[(1-S)^L+E*S^T]
            const std::vector<Scalar>& letCoeffsPc = {static_cast<Scalar>(letTab.s1_residual),
                                                        static_cast<Scalar>(letTab.s2_residual),
                                                        static_cast<Scalar>(letTab.l_pc),
                                                        static_cast<Scalar>(letTab.e_pc),
                                                        static_cast<Scalar>(letTab.t_pc),
                                                        static_cast<Scalar>(letTab.pcir_pc),
                                                        static_cast<Scalar>(letTab.pct_pc)};
            realParams.setPcnwSamples(letCoeffsPc, dum);

            realParams.finalize();
        }
        break;
    }

    case SatFuncControls::KeywordFamily::Family_II:
    {
        const auto& swfnTable = tableManager.getSwfnTables().template getTable<SwfnTable>(satRegionIdx);
        const std::vector<double> SwColumn = swfnTable.getColumn("SW").vectorCopy();

        effParams.setApproach(SatCurveMultiplexerApproach::PiecewiseLinear);
        auto& realParams = effParams.template getRealParams<SatCurveMultiplexerApproach::PiecewiseLinear>();

        realParams.setKrwSamples(SwColumn, normalizeKrValues_(tolcrit, swfnTable.getColumn("KRW")));
        realParams.setPcnwSamples(SwColumn, swfnTable.getColumn("PCOW").vectorCopy());

        if (!this->parent_.hasGas) {
            const auto& sof2Table = tableManager.getSof2Tables().template getTable<Sof2Table>(satRegionIdx);
            // convert the saturations of the SOF2 keyword from oil to water saturations
            std::vector<double> SwSamples(sof2Table.numRows());
            for (size_t sampleIdx = 0; sampleIdx < sof2Table.numRows(); ++ sampleIdx)
                SwSamples[sampleIdx] = 1 - sof2Table.get("SO", sampleIdx);

            realParams.setKrnSamples(SwSamples, normalizeKrValues_(tolcrit, sof2Table.getColumn("KRO")));
        } else {
            const auto& sof3Table = tableManager.getSof3Tables().template getTable<Sof3Table>(satRegionIdx);
            // convert the saturations of the SOF3 keyword from oil to water saturations
            std::vector<double> SwSamples(sof3Table.numRows());
            for (size_t sampleIdx = 0; sampleIdx < sof3Table.numRows(); ++ sampleIdx)
                SwSamples[sampleIdx] = 1 - sof3Table.get("SO", sampleIdx);

            realParams.setKrnSamples(SwSamples, normalizeKrValues_(tolcrit, sof3Table.getColumn("KROW")));
        }
        realParams.finalize();
        break;
    }

    case SatFuncControls::KeywordFamily::Family_III:
    {
        throw std::domain_error("Saturation keyword family III is not applicable for a oil-water system");
    }

    case SatFuncControls::KeywordFamily::Undefined:
        throw std::domain_error("No valid saturation keyword family specified");
    }
}

// Make some actual code, by realizing the previously defined templated class
template class EclMaterialLawManager<ThreePhaseMaterialTraits<double,0,1,2>>::InitParams::ReadEffectiveParams;
template class EclMaterialLawManager<ThreePhaseMaterialTraits<float,0,1,2>>::InitParams::ReadEffectiveParams;
template class EclMaterialLawManager<ThreePhaseMaterialTraits<double,2,0,1>>::InitParams::ReadEffectiveParams;
template class EclMaterialLawManager<ThreePhaseMaterialTraits<float,2,0,1>>::InitParams::ReadEffectiveParams;


} // namespace Opm
