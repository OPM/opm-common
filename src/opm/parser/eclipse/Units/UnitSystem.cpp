/*
  Copyright 2013 Statoil ASA.

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


#include <iostream>
#include <stdexcept>
#include <boost/algorithm/string.hpp>

#include <opm/parser/eclipse/Units/Dimension.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <vector>
#include <limits>


namespace Opm {

namespace {
    /*
     * It is VERY important that the measure enum has the same order as the
     * metric and field arrays. C++ does not support designated initializers, so
     * this cannot be done in a declaration-order independent matter.
     */

    // =================================================================
    // METRIC Unit Conventions

    static const double from_metric_offset[] = {
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        Metric::TemperatureOffset,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0
    };

    static const double to_metric[] = {
        1,
        1 / Metric::Length,
        1 / Metric::Time,
        1 / Metric::Density,
        1 / Metric::Pressure,
        1 / Metric::AbsoluteTemperature,
        1 / Metric::Temperature,
        1 / Metric::Viscosity,
        1 / Metric::Permeability,
        1 / Metric::LiquidSurfaceVolume,
        1 / Metric::GasSurfaceVolume,
        1 / Metric::ReservoirVolume,
        1 / ( Metric::LiquidSurfaceVolume / Metric::Time ),
        1 / ( Metric::GasSurfaceVolume / Metric::Time ),
        1 / ( Metric::ReservoirVolume / Metric::Time ),
        1 / Metric::Transmissibility,
        1 / (Metric::Permeability * Metric::Length),
        1 / Metric::Mass,
        1 / ( Metric::Mass / Metric::Time ),
        1, /* gas-oil ratio */
        1, /* oil-gas ratio */
        1, /* water cut */
        1, /* gas formation volume factor */
        1, /* oil formation volume factor */
        1, /* water formation volume factor */
        1, /* gas inverse formation volume factor */
        1, /* oil inverse formation volume factor */
        1, /* water inverse formation volume factor */
        1 / Metric::Energy
    };

    static const double from_metric[] = {
        1,
        Metric::Length,
        Metric::Time,
        Metric::Density,
        Metric::Pressure,
        Metric::AbsoluteTemperature,
        Metric::Temperature,
        Metric::Viscosity,
        Metric::Permeability,
        Metric::LiquidSurfaceVolume,
        Metric::GasSurfaceVolume,
        Metric::ReservoirVolume,
        Metric::LiquidSurfaceVolume / Metric::Time,
        Metric::GasSurfaceVolume / Metric::Time,
        Metric::ReservoirVolume / Metric::Time,
        Metric::Transmissibility,
        Metric::Permeability * Metric::Length,
        Metric::Mass,
        Metric::Mass / Metric::Time,
        1, /* gas-oil ratio */
        1, /* oil-gas ratio */
        1, /* water cut */
        1, /* gas formation volume factor */
        1, /* oil formation volume factor */
        1, /* water formation volume factor */
        1, /* gas inverse formation volume factor */
        1, /* oil inverse formation volume factor */
        1, /* water inverse formation volume factor */
        Metric::Energy
    };

    static constexpr const char* metric_names[] = {
        "",
        "M",
        "DAY",
        "KG/M3",
        "BARSA",
        "K",
        "C",
        "CP",
        "MD",
        "SM3",
        "SM3",
        "RM3",
        "SM3/DAY",
        "SM3/DAY",
        "RM3/DAY",
        "CPR3/DAY/BARS",
        "MDM",
        "KG",
        "KG/DAY",
        "SM3/SM3",
        "SM3/SM3",
        "SM3/SM3",
        "RM3/SM3", /* gas formation volume factor */
        "RM3/SM3", /* oil formation volume factor */
        "RM3/SM3", /* water formation volume factor */
        "SM3/RM3", /* gas inverse formation volume factor */
        "SM3/RM3", /* oil inverse formation volume factor */
        "SM3/RM3", /* water inverse formation volume factor */
        "KJ", /* energy */
    };

    // =================================================================
    // FIELD Unit Conventions

    static const double from_field_offset[] = {
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        Field::TemperatureOffset,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0
    };

    static const double to_field[] = {
        1,
        1 / Field::Length,
        1 / Field::Time,
        1 / Field::Density,
        1 / Field::Pressure,
        1 / Field::AbsoluteTemperature,
        1 / Field::Temperature,
        1 / Field::Viscosity,
        1 / Field::Permeability,
        1 / Field::LiquidSurfaceVolume,
        1 / Field::GasSurfaceVolume,
        1 / Field::ReservoirVolume,
        1 / ( Field::LiquidSurfaceVolume / Field::Time ),
        1 / ( Field::GasSurfaceVolume / Field::Time ),
        1 / ( Field::ReservoirVolume / Field::Time ),
        1 / Field::Transmissibility,
        1 / (Field::Permeability * Field::Length),
        1 / Field::Mass,
        1 / ( Field::Mass / Field::Time ),
        1 / ( Field::GasSurfaceVolume / Field::LiquidSurfaceVolume ), /* gas-oil ratio */
        1 / ( Field::LiquidSurfaceVolume / Field::GasSurfaceVolume ), /* oil-gas ratio */
        1, /* water cut */
        1 / (Field::ReservoirVolume / Field::GasSurfaceVolume), /* gas formation volume factor */
        1, /* oil formation volume factor */
        1, /* water formation volume factor */
        1 / (Field::GasSurfaceVolume / Field::ReservoirVolume), /* gas inverse formation volume factor */
        1, /* oil inverse formation volume factor */
        1, /* water inverse formation volume factor */
        1 / Field::Energy
    };

    static const double from_field[] = {
         1,
         Field::Length,
         Field::Time,
         Field::Density,
         Field::Pressure,
         Field::AbsoluteTemperature,
         Field::Temperature,
         Field::Viscosity,
         Field::Permeability,
         Field::LiquidSurfaceVolume,
         Field::GasSurfaceVolume,
         Field::ReservoirVolume,
         Field::LiquidSurfaceVolume / Field::Time,
         Field::GasSurfaceVolume / Field::Time,
         Field::ReservoirVolume / Field::Time,
         Field::Transmissibility,
         Field::Permeability * Field::Length,
         Field::Mass,
         Field::Mass / Field::Time,
         Field::GasSurfaceVolume / Field::LiquidSurfaceVolume, /* gas-oil ratio */
         Field::LiquidSurfaceVolume / Field::GasSurfaceVolume, /* oil-gas ratio */
         1, /* water cut */
         Field::ReservoirVolume / Field::GasSurfaceVolume, /* gas formation volume factor */
         1, /* oil formation volume factor */
         1, /* water formation volume factor */
         Field::GasSurfaceVolume / Field::ReservoirVolume, /* gas inverse formation volume factor */
         1, /* oil inverse formation volume factor */
         1, /* water inverse formation volume factor */
         Field::Energy
    };

    static constexpr const char* field_names[] = {
        "",
        "FT",
        "DAY",
        "LB/FT3",
        "PSIA",
        "R",
        "F",
        "CP",
        "MD",
        "STB",
        "MSCF",
        "RB",
        "STB/DAY",
        "MSCF/DAY",
        "RB/DAY",
        "CPRB/DAY/PSI",
        "MDFT",
        "LB",
        "LB/DAY"
        "MSCF/STB",
        "STB/MSCF",
        "STB/STB",
        "RB/MSCF", /* gas formation volume factor */
        "RB/STB", /* oil formation volume factor */
        "RB/STB", /* water formation volume factor */
        "MSCF/RB", /* gas inverse formation volume factor */
        "STB/RB", /* oil inverse formation volume factor */
        "STB/RB", /* water inverse formation volume factor */
        "BTU", /* energy */
    };

    // =================================================================
    // LAB Unit Conventions

    static const double from_lab_offset[] = {
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        Lab::TemperatureOffset,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0
    };

    static const double to_lab[] = {
        1,
        1 / Lab::Length,
        1 / Lab::Time,
        1 / Lab::Density,
        1 / Lab::Pressure,
        1 / Lab::AbsoluteTemperature,
        1 / Lab::Temperature,
        1 / Lab::Viscosity,
        1 / Lab::Permeability,
        1 / Lab::LiquidSurfaceVolume,
        1 / Lab::GasSurfaceVolume,
        1 / Lab::ReservoirVolume,
        1 / ( Lab::LiquidSurfaceVolume / Lab::Time ),
        1 / ( Lab::GasSurfaceVolume / Lab::Time ),
        1 / ( Lab::ReservoirVolume / Lab::Time ),
        1 / Lab::Transmissibility,
        1 / (Lab::Permeability * Lab::Length),
        1 / Lab::Mass,
        1 / ( Lab::Mass / Lab::Time ),
        1 / Lab::GasDissolutionFactor, /* gas-oil ratio */
        1 / Lab::OilDissolutionFactor, /* oil-gas ratio */
        1, /* water cut */
        1, /* gas formation volume factor */
        1, /* oil formation volume factor */
        1, /* water formation volume factor */
        1, /* gas inverse formation volume factor */
        1, /* oil inverse formation volume factor */
        1, /* water inverse formation volume factor */
        1 / Lab::Energy
    };

    static const double from_lab[] = {
        1,
        Lab::Length,
        Lab::Time,
        Lab::Density,
        Lab::Pressure,
        Lab::AbsoluteTemperature,
        Lab::Temperature,
        Lab::Viscosity,
        Lab::Permeability,
        Lab::LiquidSurfaceVolume,
        Lab::GasSurfaceVolume,
        Lab::ReservoirVolume,
        Lab::LiquidSurfaceVolume / Lab::Time,
        Lab::GasSurfaceVolume / Lab::Time,
        Lab::ReservoirVolume / Lab::Time,
        Lab::Transmissibility,
        Lab::Permeability * Lab::Length,
        Lab::Mass,
        Lab::Mass / Lab::Time,
        Lab::GasDissolutionFactor,  /* gas-oil ratio */
        Lab::OilDissolutionFactor,  /* oil-gas ratio */
        1, /* water cut */
        1, /* gas formation volume factor */
        1, /* oil formation volume factor */
        1, /* water formation volume factor */
        1, /* gas inverse formation volume factor */
        1, /* oil inverse formation volume factor */
        1, /* water inverse formation volume factor */
        Lab::Energy
    };

    static constexpr const char* lab_names[] = {
        "",
        "CM",
        "HR",
        "G/CC",
        "ATM",
        "K",
        "C",
        "CP",
        "MD",
        "SCC",
        "SCC",
        "RCC",
        "SCC/HR",
        "SCC/HR",
        "RCC/HR",
        "CPRCC/HR/ATM",
        "MDCC",
        "G",
        "G/HR",
        "SCC/SCC",
        "SCC/SCC",
        "SCC/SCC",
        "RCC/SCC", /* gas formation volume factor */
        "RCC/SCC", /* oil formation volume factor */
        "RCC/SCC", /* water formation volume factor */
        "SCC/RCC", /* gas formation volume factor */
        "SCC/RCC", /* oil inverse formation volume factor */
        "SCC/RCC", /* water inverse formation volume factor */
        "J", /* energy */
    };

    // =================================================================
    // PVT-M Unit Conventions

    static const double from_pvt_m_offset[] = {
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        PVT_M::TemperatureOffset,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0
    };

    static const double to_pvt_m[] = {
        1,
        1 / PVT_M::Length,
        1 / PVT_M::Time,
        1 / PVT_M::Density,
        1 / PVT_M::Pressure,
        1 / PVT_M::AbsoluteTemperature,
        1 / PVT_M::Temperature,
        1 / PVT_M::Viscosity,
        1 / PVT_M::Permeability,
        1 / PVT_M::LiquidSurfaceVolume,
        1 / PVT_M::GasSurfaceVolume,
        1 / PVT_M::ReservoirVolume,
        1 / ( PVT_M::LiquidSurfaceVolume / PVT_M::Time ),
        1 / ( PVT_M::GasSurfaceVolume / PVT_M::Time ),
        1 / ( PVT_M::ReservoirVolume / PVT_M::Time ),
        1 / PVT_M::Transmissibility,
        1 / (PVT_M::Permeability * PVT_M::Length),
        1 / PVT_M::Mass,
        1 / ( PVT_M::Mass / PVT_M::Time ),
        1 / (PVT_M::GasSurfaceVolume / PVT_M::LiquidSurfaceVolume), // Rs
        1 / (PVT_M::LiquidSurfaceVolume / PVT_M::GasSurfaceVolume), // Rv
        1, /* water cut */
        1 / (PVT_M::ReservoirVolume / PVT_M::GasSurfaceVolume), /* Bg */
        1 / (PVT_M::ReservoirVolume / PVT_M::LiquidSurfaceVolume), /* Bo */
        1 / (PVT_M::ReservoirVolume / PVT_M::LiquidSurfaceVolume), /* Bw */
        1 / (PVT_M::GasSurfaceVolume / PVT_M::ReservoirVolume), /* 1/Bg */
        1 / (PVT_M::LiquidSurfaceVolume / PVT_M::ReservoirVolume), /* 1/Bo */
        1 / (PVT_M::LiquidSurfaceVolume / PVT_M::ReservoirVolume), /* 1/Bw */
        1 / PVT_M::Energy
    };

    static const double from_pvt_m[] = {
        1,
        PVT_M::Length,
        PVT_M::Time,
        PVT_M::Density,
        PVT_M::Pressure,
        PVT_M::AbsoluteTemperature,
        PVT_M::Temperature,
        PVT_M::Viscosity,
        PVT_M::Permeability,
        PVT_M::LiquidSurfaceVolume,
        PVT_M::GasSurfaceVolume,
        PVT_M::ReservoirVolume,
        PVT_M::LiquidSurfaceVolume / PVT_M::Time,
        PVT_M::GasSurfaceVolume / PVT_M::Time,
        PVT_M::ReservoirVolume / PVT_M::Time,
        PVT_M::Transmissibility,
        PVT_M::Permeability * PVT_M::Length,
        PVT_M::Mass,
        PVT_M::Mass / PVT_M::Time,
        PVT_M::GasSurfaceVolume / PVT_M::LiquidSurfaceVolume, // Rs
        PVT_M::LiquidSurfaceVolume / PVT_M::GasSurfaceVolume, // Rv
        1, /* water cut */
        PVT_M::ReservoirVolume / PVT_M::GasSurfaceVolume, /* Bg */
        PVT_M::ReservoirVolume / PVT_M::LiquidSurfaceVolume, /* Bo */
        PVT_M::ReservoirVolume / PVT_M::LiquidSurfaceVolume, /* Bw */
        PVT_M::GasSurfaceVolume / PVT_M::ReservoirVolume, /* 1/Bg */
        PVT_M::LiquidSurfaceVolume / PVT_M::ReservoirVolume, /* 1/Bo */
        PVT_M::LiquidSurfaceVolume / PVT_M::ReservoirVolume, /* 1/Bw */
        PVT_M::Energy
    };

    static constexpr const char* pvt_m_names[] = {
        "",
        "M",
        "DAY",
        "KG/M3",
        "ATM",
        "K",
        "C",
        "CP",
        "MD",
        "SM3",
        "SM3",
        "RM3",
        "SM3/DAY",
        "SM3/DAY",
        "RM3/DAY",
        "CPR3/DAY/ATM",
        "MDM",
        "KG",
        "KG/DAY",
        "SM3/SM3",
        "SM3/SM3",
        "SM3/SM3",
        "RM3/SM3", /* gas formation volume factor */
        "RM3/SM3", /* oil formation volume factor */
        "RM3/SM3", /* water formation volume factor */
        "SM3/RM3", /* gas inverse formation volume factor */
        "SM3/RM3", /* oil inverse formation volume factor */
        "SM3/RM3", /* water inverse formation volume factor */
        "KJ" /* energy */
    };

    // =================================================================
    // INPUT Unit Conventions

    static const double from_input_offset[] = {
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0,
        0.0
    };

    static const double to_input[] = {
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1
    };

    static const double from_input[] = {
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1
    };

    static constexpr const char* input_names[] = {
        "",
        "M",
        "DAY",
        "KG/M3",
        "BARSA",
        "K",
        "C",
        "CP",
        "MD",
        "SM3",
        "SM3",
        "RM3",
        "SM3/DAY",
        "SM3/DAY",
        "RM3/DAY",
        "CPR3/DAY/BARS",
        "MDM",
        "KG",
        "KG/DAY",
        "SM3/SM3",
        "SM3/SM3",
        "SM3/SM3",
        "RM3/SM3", /* gas formation volume factor */
        "RM3/SM3", /* oil formation volume factor */
        "RM3/SM3", /* water formation volume factor */
        "SM3/RM3", /* gas inverse formation volume factor */
        "SM3/RM3", /* oil inverse formation volume factor */
        "SM3/RM3", /* water inverse formation volume factor */
        "KJ", /* energy */
    };

} // namespace Anonymous

    UnitSystem::UnitSystem(const UnitType unit) :
        m_unittype( unit )
    {
        switch(unit) {
            case(UnitType::UNIT_TYPE_METRIC):
                m_name = "Metric";
                this->measure_table_from_si = to_metric;
                this->measure_table_to_si = from_metric;
                this->measure_table_to_si_offset = from_metric_offset;
                this->unit_name_table = metric_names;
                break;
            case(UnitType::UNIT_TYPE_FIELD):
                m_name = "Field";
                this->measure_table_from_si = to_field;
                this->measure_table_to_si = from_field;
                this->measure_table_to_si_offset = from_field_offset;
                this->unit_name_table = field_names;
                break;
            case(UnitType::UNIT_TYPE_LAB):
                m_name = "Lab";
                this->measure_table_from_si = to_lab;
                this->measure_table_to_si = from_lab;
                this->measure_table_to_si_offset = from_lab_offset;
                this->unit_name_table = lab_names;
                break;
            case(UnitType::UNIT_TYPE_PVT_M):
                m_name = "PVT-M";
                this->measure_table_from_si = to_pvt_m;
                this->measure_table_to_si = from_pvt_m;
                this->measure_table_to_si_offset = from_pvt_m_offset;
                this->unit_name_table = pvt_m_names;
                break;
            case(UnitType::UNIT_TYPE_INPUT):
                m_name = "Input";
                this->measure_table_from_si = to_input;
                this->measure_table_to_si = from_input;
                this->measure_table_to_si_offset = from_input_offset;
                this->unit_name_table = input_names;
                break;
            default:
                throw std::runtime_error("Tried to construct UnitSystem with unknown unit family.");
                break;
        };
    }

    namespace {

        UnitSystem::UnitType fromEclType(ert_ecl_unit_enum unit_type) {
            switch ( unit_type ) {
            case(ECL_METRIC_UNITS): return UnitSystem::UnitType::UNIT_TYPE_METRIC;
            case(ECL_FIELD_UNITS):  return UnitSystem::UnitType::UNIT_TYPE_FIELD;
            case(ECL_LAB_UNITS):    return UnitSystem::UnitType::UNIT_TYPE_LAB;
            case(ECL_PVT_M_UNITS):  return UnitSystem::UnitType::UNIT_TYPE_PVT_M;
            default:
                throw std::runtime_error("What has happened here?");
            }
        }

    }

    UnitSystem::UnitSystem(const ert_ecl_unit_enum unit_type)
        : UnitSystem( fromEclType( unit_type ))
    {
    }


    bool UnitSystem::hasDimension(const std::string& dimension) const {
        return (m_dimensions.find( dimension ) != m_dimensions.end());
    }


    const Dimension& UnitSystem::getNewDimension(const std::string& dimension) {
        if( !hasDimension( dimension ) )
            this->addDimension( parse( dimension ) );

        return getDimension( dimension );
    }


    const Dimension& UnitSystem::getDimension(const std::string& dimension) const {
        return this->m_dimensions.at( dimension );
    }


    void UnitSystem::addDimension( Dimension dimension ) {
        this->m_dimensions[ dimension.getName() ] = std::move( dimension );
    }

    void UnitSystem::addDimension(const std::string& dimension , double SIfactor, double SIoffset) {
        this->addDimension( Dimension { dimension, SIfactor, SIoffset } );
    }

    const std::string& UnitSystem::getName() const {
        return m_name;
    }

    UnitSystem::UnitType UnitSystem::getType() const {
        return m_unittype;
    }


    ert_ecl_unit_enum UnitSystem::getEclType() const {
        switch ( m_unittype ) {
          case UnitType::UNIT_TYPE_METRIC: return ECL_METRIC_UNITS;
          case UnitType::UNIT_TYPE_FIELD:  return ECL_FIELD_UNITS;
          case UnitType::UNIT_TYPE_LAB:    return ECL_LAB_UNITS;
          case UnitType::UNIT_TYPE_PVT_M:  return ECL_PVT_M_UNITS;
          case UnitType::UNIT_TYPE_INPUT:  throw std::runtime_error("UNIT_TYPE_INPUT has no counterpart in the ert_ecl_unit_enum type.");
        default:
            throw std::runtime_error("What has happened here?");
        }
    }



    Dimension UnitSystem::parseFactor(const std::string& dimension) const {
        std::vector<std::string> dimensionList;
        boost::split(dimensionList , dimension , boost::is_any_of("*"));

        double SIfactor = 1.0;
        for( const auto& x : dimensionList ) {
            auto dim = getDimension( x );

            // all constituing dimension must be compositable. The
            // only exception is if there is the "composite" dimension
            // consists of exactly a single atomic dimension...
            if (dimensionList.size() > 1 && !dim.isCompositable())
                throw std::invalid_argument("Composite dimensions currently cannot require a conversion offset");

            SIfactor *= dim.getSIScaling();
        }
        return Dimension::newComposite( dimension , SIfactor );
    }

    Dimension UnitSystem::parse(const std::string& dimension) const {
        const size_t divCount = std::count( dimension.begin() , dimension.end() , '/' );

        if( divCount > 1 )
                throw std::invalid_argument("Dimension string can only have one division sign '/'");

        const bool haveDivisor = divCount == 1;
        if( !haveDivisor ) return parseFactor( dimension );

        std::vector<std::string> parts;
        boost::split(parts , dimension , boost::is_any_of("/"));
        Dimension dividend = parseFactor( parts[0] );
        Dimension divisor = parseFactor( parts[1] );

        if (dividend.getSIOffset() != 0.0 || divisor.getSIOffset() != 0.0)
            throw std::invalid_argument("Composite dimensions cannot currently require a conversion offset");

        return Dimension::newComposite( dimension, dividend.getSIScaling() / divisor.getSIScaling() );
    }


    bool UnitSystem::equal(const UnitSystem& other) const {
        return *this == other;
    }

    bool UnitSystem::operator==( const UnitSystem& rhs ) const {
        return this->m_name == rhs.m_name
            && this->m_unittype == rhs.m_unittype
            && this->m_dimensions.size() == rhs.m_dimensions.size()
            && std::equal( this->m_dimensions.begin(),
                           this->m_dimensions.end(),
                           rhs.m_dimensions.begin() )
            && this->measure_table_to_si_offset == rhs.measure_table_to_si_offset
            && this->measure_table_from_si == rhs.measure_table_from_si
            && this->measure_table_to_si == rhs.measure_table_to_si
            && this->unit_name_table == rhs.unit_name_table;
    }

    bool UnitSystem::operator!=( const UnitSystem& rhs ) const {
        return !( *this == rhs );
    }

    double UnitSystem::from_si( measure m, double val ) const {
        return
            this->measure_table_from_si[ static_cast< int >( m ) ]
            * (val - this->measure_table_to_si_offset[ static_cast< int >( m ) ]);
    }

    double UnitSystem::to_si( measure m, double val ) const {
        return
            this->measure_table_to_si[ static_cast< int >( m ) ]*val
            + this->measure_table_to_si_offset[ static_cast< int >( m ) ];
    }

    void UnitSystem::from_si( measure m, std::vector<double>& data ) const {
        double factor = this->measure_table_from_si[ static_cast< int >( m ) ];
        double offset = this->measure_table_to_si_offset[ static_cast< int >( m ) ];
        auto scale = [=](double x) { return (x - offset) * factor; };
        std::transform( data.begin() , data.end() , data.begin() , scale);
    }


    void UnitSystem::to_si( measure m, std::vector<double>& data) const {
        double factor = this->measure_table_to_si[ static_cast< int >( m ) ];
        double offset = this->measure_table_to_si_offset[ static_cast< int >( m ) ];
        auto scale = [=](double x) { return x * factor + offset; };
        std::transform( data.begin() , data.end() , data.begin() , scale);
    }



    const char* UnitSystem::name( measure m ) const {
        return this->unit_name_table[ static_cast< int >( m ) ];
    }

    UnitSystem UnitSystem::newMETRIC() {
        UnitSystem system( UnitType::UNIT_TYPE_METRIC );

        system.addDimension("1"         , 1.0);
        system.addDimension("Pressure"  , Metric::Pressure );
        system.addDimension("Temperature", Metric::Temperature, Metric::TemperatureOffset);
        system.addDimension("AbsoluteTemperature", Metric::AbsoluteTemperature);
        system.addDimension("Length"    , Metric::Length);
        system.addDimension("Time"      , Metric::Time );
        system.addDimension("Mass"         , Metric::Mass );
        system.addDimension("Permeability", Metric::Permeability );
        system.addDimension("Transmissibility", Metric::Transmissibility );
        system.addDimension("GasDissolutionFactor", Metric::GasDissolutionFactor);
        system.addDimension("OilDissolutionFactor", Metric::OilDissolutionFactor);
        system.addDimension("LiquidSurfaceVolume", Metric::LiquidSurfaceVolume );
        system.addDimension("GasSurfaceVolume" , Metric::GasSurfaceVolume );
        system.addDimension("ReservoirVolume", Metric::ReservoirVolume );
        system.addDimension("Density"   , Metric::Density );
        system.addDimension("PolymerDensity", Metric::PolymerDensity);
        system.addDimension("Salinity", Metric::Salinity);
        system.addDimension("Viscosity" , Metric::Viscosity);
        system.addDimension("Timestep"  , Metric::Timestep);
        system.addDimension("SurfaceTension"  , Metric::SurfaceTension);
        system.addDimension("Energy", Metric::Energy);
        system.addDimension("ContextDependent", std::numeric_limits<double>::quiet_NaN());
        return system;
    }



    UnitSystem UnitSystem::newFIELD() {
        UnitSystem system( UnitType::UNIT_TYPE_FIELD );

        system.addDimension("1"    , 1.0);
        system.addDimension("Pressure", Field::Pressure );
        system.addDimension("Temperature", Field::Temperature, Field::TemperatureOffset);
        system.addDimension("AbsoluteTemperature", Field::AbsoluteTemperature);
        system.addDimension("Length", Field::Length);
        system.addDimension("Time" , Field::Time);
        system.addDimension("Mass", Field::Mass);
        system.addDimension("Permeability", Field::Permeability );
        system.addDimension("Transmissibility", Field::Transmissibility );
        system.addDimension("GasDissolutionFactor" , Field::GasDissolutionFactor);
        system.addDimension("OilDissolutionFactor", Field::OilDissolutionFactor);
        system.addDimension("LiquidSurfaceVolume", Field::LiquidSurfaceVolume );
        system.addDimension("GasSurfaceVolume", Field::GasSurfaceVolume );
        system.addDimension("ReservoirVolume", Field::ReservoirVolume );
        system.addDimension("Density", Field::Density );
        system.addDimension("PolymerDensity", Field::PolymerDensity);
        system.addDimension("Salinity", Field::Salinity);
        system.addDimension("Viscosity", Field::Viscosity);
        system.addDimension("Timestep", Field::Timestep);
        system.addDimension("SurfaceTension"  , Field::SurfaceTension);
        system.addDimension("Energy", Field::Energy);
        system.addDimension("ContextDependent", std::numeric_limits<double>::quiet_NaN());
        return system;
    }



    UnitSystem UnitSystem::newLAB() {
        UnitSystem system( UnitType::UNIT_TYPE_LAB );

        system.addDimension("1"    , 1.0);
        system.addDimension("Pressure", Lab::Pressure );
        system.addDimension("Temperature", Lab::Temperature, Lab::TemperatureOffset);
        system.addDimension("AbsoluteTemperature", Lab::AbsoluteTemperature);
        system.addDimension("Length", Lab::Length);
        system.addDimension("Time" , Lab::Time);
        system.addDimension("Mass", Lab::Mass);
        system.addDimension("Permeability", Lab::Permeability );
        system.addDimension("Transmissibility", Lab::Transmissibility );
        system.addDimension("GasDissolutionFactor" , Lab::GasDissolutionFactor);
        system.addDimension("OilDissolutionFactor", Lab::OilDissolutionFactor);
        system.addDimension("LiquidSurfaceVolume", Lab::LiquidSurfaceVolume );
        system.addDimension("GasSurfaceVolume", Lab::GasSurfaceVolume );
        system.addDimension("ReservoirVolume", Lab::ReservoirVolume );
        system.addDimension("Density", Lab::Density );
        system.addDimension("PolymerDensity", Lab::PolymerDensity);
        system.addDimension("Salinity", Lab::Salinity);
        system.addDimension("Viscosity", Lab::Viscosity);
        system.addDimension("Timestep", Lab::Timestep);
        system.addDimension("SurfaceTension"  , Lab::SurfaceTension);
        system.addDimension("Energy", Lab::Energy);
        system.addDimension("ContextDependent", std::numeric_limits<double>::quiet_NaN());
        return system;
    }


    UnitSystem UnitSystem::newPVT_M() {
        UnitSystem system( UnitType::UNIT_TYPE_PVT_M );

        system.addDimension("1"         , 1.0);
        system.addDimension("Pressure"  , PVT_M::Pressure );
        system.addDimension("Temperature", PVT_M::Temperature, PVT_M::TemperatureOffset);
        system.addDimension("AbsoluteTemperature", PVT_M::AbsoluteTemperature);
        system.addDimension("Length"    , PVT_M::Length);
        system.addDimension("Time"      , PVT_M::Time );
        system.addDimension("Mass"         , PVT_M::Mass );
        system.addDimension("Permeability", PVT_M::Permeability );
        system.addDimension("Transmissibility", PVT_M::Transmissibility );
        system.addDimension("GasDissolutionFactor", PVT_M::GasDissolutionFactor);
        system.addDimension("OilDissolutionFactor", PVT_M::OilDissolutionFactor);
        system.addDimension("LiquidSurfaceVolume", PVT_M::LiquidSurfaceVolume );
        system.addDimension("GasSurfaceVolume" , PVT_M::GasSurfaceVolume );
        system.addDimension("ReservoirVolume", PVT_M::ReservoirVolume );
        system.addDimension("Density"   , PVT_M::Density );
        system.addDimension("PolymerDensity", PVT_M::PolymerDensity);
        system.addDimension("Salinity", PVT_M::Salinity);
        system.addDimension("Viscosity" , PVT_M::Viscosity);
        system.addDimension("Timestep"  , PVT_M::Timestep);
        system.addDimension("SurfaceTension"  , PVT_M::SurfaceTension);
        system.addDimension("Energy", PVT_M::Energy);
        system.addDimension("ContextDependent", std::numeric_limits<double>::quiet_NaN());
        return system;
    }

    UnitSystem UnitSystem::newINPUT() {
        UnitSystem system( UnitType::UNIT_TYPE_INPUT );

        system.addDimension("1"         , 1.0);
        system.addDimension("Pressure"  , 1.0);
        system.addDimension("Temperature", 1.0);
        system.addDimension("AbsoluteTemperature", 1.0, 0.0);
        system.addDimension("Length"    , 1.0);
        system.addDimension("Time"      , 1.0);
        system.addDimension("Mass"         , 1.0);
        system.addDimension("Permeability", 1.0);
        system.addDimension("Transmissibility", 1.0);
        system.addDimension("GasDissolutionFactor", 1.0);
        system.addDimension("OilDissolutionFactor", 1.0);
        system.addDimension("LiquidSurfaceVolume", 1.0);
        system.addDimension("GasSurfaceVolume" , 1.0);
        system.addDimension("ReservoirVolume", 1.0);
        system.addDimension("Density"   , 1.0);
        system.addDimension("PolymerDensity", 1.0);
        system.addDimension("Salinity", 1.0);
        system.addDimension("Viscosity" , 1.0);
        system.addDimension("Timestep"  , 1.0);
        system.addDimension("SurfaceTension"  , 1.0);
        system.addDimension("Energy", 1.0);
        system.addDimension("ContextDependent", 1.0);
        return system;
     }


}
