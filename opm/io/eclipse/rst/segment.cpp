/*
  Copyright 2020 Equinor ASA.

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

#include <opm/io/eclipse/rst/segment.hpp>

#include <opm/output/eclipse/VectorItems/msw.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>

namespace VI = ::Opm::RestartIO::Helpers::VectorItems;
using M  = ::Opm::UnitSystem::measure;

namespace {

double area_to_si(const Opm::UnitSystem& unit_system, const double raw_value)
{
    return unit_system.to_si(M::length, unit_system.to_si(M::length, raw_value));
}

double
load_device_base_strength(const Opm::UnitSystem& unit_system,
                          const int              segment_type,
                          const double           base_strength_raw)
{
    using VI::ISeg::Value::Type;

    auto unit = M::identity;
    if (segment_type == Type::SpiralICD) {
        unit = M::icd_strength;
    }
    else if (segment_type == Type::AutoICD) {
        unit = M::aicd_strength;
    }

    return unit_system.to_si(unit, base_strength_raw);
}

double
load_icd_scaling_factor(const Opm::UnitSystem& unit_system,
                        const int*             iseg,
                        const double*          rseg)
{
    const auto scalingFactor = rseg[VI::RSeg::ScalingFactor];
    const auto scalingMethod = iseg[VI::ISeg::ICDScalingMode];

    if ((scalingMethod == 1) ||
        ((scalingMethod < 0) && (rseg[VI::RSeg::ICDLength] < 0.0)))
    {
        // Scaling factor is a length.  Convert to SI.
        return unit_system.to_si(M::length, scalingFactor);
    }

    // Scaling factor is a relative measure.  No unit conversion needed.
    return scalingFactor;
}

} // Anonymous namespace

Opm::RestartIO::RstSegment::RstSegment(const UnitSystem& unit_system,
                                       const int         segment_number,
                                       const int*        iseg,
                                       const double*     rseg)
    : segment(                                                   segment_number)
    , outlet_segment(                                            iseg[VI::ISeg::OutSeg])
    , branch(                                                    iseg[VI::ISeg::BranchNo])
    , segment_type(                                              iseg[VI::ISeg::SegmentType])
    , icd_scaling_mode(                                          iseg[VI::ISeg::ICDScalingMode])
    , icd_status(                                                iseg[VI::ISeg::ICDOpenShutFlag])
    , dist_outlet(            unit_system.to_si(M::length,       rseg[VI::RSeg::DistOutlet]))
    , outlet_dz(              unit_system.to_si(M::length,       rseg[VI::RSeg::OutletDepthDiff]))
    , diameter(               unit_system.to_si(M::length,       rseg[VI::RSeg::SegDiam]))
    , roughness(              unit_system.to_si(M::length,       rseg[VI::RSeg::SegRough]))
    , area(                   area_to_si(unit_system,            rseg[VI::RSeg::SegArea]))
    , volume(                 unit_system.to_si(M::volume,       rseg[VI::RSeg::SegVolume]))
    , dist_bhp_ref(           unit_system.to_si(M::length,       rseg[VI::RSeg::DistBHPRef]))
    , node_depth(             unit_system.to_si(M::length,       rseg[VI::RSeg::SegNodeDepth]))
    , total_flow(             unit_system.to_si(M::rate,         rseg[VI::RSeg::TotFlowRate]))
    , water_flow_fraction(                                       rseg[VI::RSeg::WatFlowFract])
    , gas_flow_fraction(                                         rseg[VI::RSeg::GasFlowFract])
    , pressure(               unit_system.to_si(M::pressure,     rseg[VI::RSeg::Pressure]))
    , valve_length(           unit_system.to_si(M::length,       rseg[VI::RSeg::ValveLength]))
    , valve_area(             area_to_si(unit_system,            rseg[VI::RSeg::ValveArea]))
    , valve_flow_coeff(                                          rseg[VI::RSeg::ValveFlowCoeff])
    , valve_max_area(         area_to_si(unit_system,            rseg[VI::RSeg::ValveMaxArea]))
    , base_strength(load_device_base_strength(unit_system, segment_type, rseg[VI::RSeg::DeviceBaseStrength]))
    , fluid_density(          unit_system.to_si(M::density,      rseg[VI::RSeg::CalibrFluidDensity]))
    , fluid_viscosity(        unit_system.to_si(M::viscosity,    rseg[VI::RSeg::CalibrFluidViscosity]))
    , critical_water_fraction(                                   rseg[VI::RSeg::CriticalWaterFraction])
    , transition_region_width(unit_system.to_si(M::length,       rseg[VI::RSeg::TransitionRegWidth]))
    , max_emulsion_ratio(                                        rseg[VI::RSeg::MaxEmulsionRatio])
    , max_valid_flow_rate(    unit_system.to_si(M::rate,         rseg[VI::RSeg::MaxValidFlowRate]))
    , icd_length(             unit_system.to_si(M::length,       rseg[VI::RSeg::ICDLength]))
    , icd_scaling_factor(load_icd_scaling_factor(unit_system, iseg, rseg))
    , valve_area_fraction(                                       rseg[VI::RSeg::ValveAreaFraction])
    , aicd_flowrate_exponent(                                    rseg[VI::RSeg::FlowRateExponent])
    , aicd_viscosity_exponent(                                   rseg[VI::RSeg::ViscFuncExponent])
    , aicd_oil_dens_exponent(                                    rseg[VI::RSeg::flowFractionOilDensityExponent])
    , aicd_wat_dens_exponent(                                    rseg[VI::RSeg::flowFractionWaterDensityExponent])
    , aicd_gas_dens_exponent(                                    rseg[VI::RSeg::flowFractionGasDensityExponent])
    , aicd_oil_visc_exponent(                                    rseg[VI::RSeg::flowFractionOilViscosityExponent])
    , aicd_wat_visc_exponent(                                    rseg[VI::RSeg::flowFractionWaterViscosityExponent])
    , aicd_gas_visc_exponent(                                    rseg[VI::RSeg::flowFractionGasViscosityExponent])
{
    if (iseg[VI::ISeg::InSegCurBranch] != 0) {
        this->inflow_segments.push_back(iseg[VI::ISeg::InSegCurBranch]);
    }
}
