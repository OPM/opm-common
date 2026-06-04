/*
  Copyright 2026 Equinor ASA.

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

#include <opm/input/eclipse/EclipseState/InitConfig/FieldSep.hpp>

#include <opm/common/OpmLog/KeywordLocation.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/F.hpp>

#include <cassert>
#include <cstddef>
#include <optional>
#include <vector>

#include <fmt/core.h>

namespace {

    // FIELDSEP items 8/9/10 (surface EoS number, NGL-density T/P) have no JSON
    // default; a defaulted record leaves them without a value.  Express the
    // absence directly as std::nullopt instead of a sentinel -- 0 is a valid
    // (0-based) EoS number and so cannot double as "not given".
    std::optional<int> optInt(const Opm::DeckItem& item)
    {
        return item.hasValue(0) ? std::optional<int>{ item.get<int>(0) }
                                : std::nullopt;
    }

    std::optional<double> optSIDouble(const Opm::DeckItem& item)
    {
        return item.hasValue(0) ? std::optional<double>{ item.getSIDouble(0) }
                                : std::nullopt;
    }

    // STAGE (item 1) has no JSON default and must be supplied explicitly.
    int requiredStage(const Opm::DeckItem& item, const Opm::KeywordLocation& location)
    {
        if (! item.hasValue(0)) {
            throw Opm::OpmInputError {
                "The stage index (item 1) of a FIELDSEP record must be specified.",
                location
            };
        }

        return item.get<int>(0);
    }

} // Anonymous namespace

namespace Opm {

    FieldSepRecord::FieldSepRecord(const DeckRecord& record, const KeywordLocation& location)
        : m_stage                  (requiredStage(record.getItem<ParserKeywords::FIELDSEP::STAGE>(), location))
        , m_temperature            (record.getItem<ParserKeywords::FIELDSEP::TEMPERATURE>().getSIDouble(0))
        , m_pressure               (record.getItem<ParserKeywords::FIELDSEP::PRESSURE>().getSIDouble(0))
        , m_liquid_destination     (record.getItem<ParserKeywords::FIELDSEP::LIQ_DESTINATION>().get<int>(0))
        , m_vapor_destination      (record.getItem<ParserKeywords::FIELDSEP::VAP_DESTINATION>().get<int>(0))
        , m_kvalue_table_number    (record.getItem<ParserKeywords::FIELDSEP::KVAL_TABLE_NUM>().get<int>(0))
        , m_gas_plant_table_number (record.getItem<ParserKeywords::FIELDSEP::GAS_PLANT_TABLE_NUM>().get<int>(0))
        , m_surface_eos_number     (optInt(record.getItem<ParserKeywords::FIELDSEP::SURFACE_EOS_NUMBER>()))
        , m_ngl_density_temperature(optSIDouble(record.getItem<ParserKeywords::FIELDSEP::NGL_DENSITY_TEMP>()))
        , m_ngl_density_pressure   (optSIDouble(record.getItem<ParserKeywords::FIELDSEP::NGL_DENSITY_PRES>()))
    {}

    FieldSepRecord FieldSepRecord::serializationTestObject()
    {
        FieldSepRecord result;
        result.m_stage = 1;
        result.m_temperature = 323.15;
        result.m_pressure = 1.0e6;
        result.m_liquid_destination = 0;
        result.m_vapor_destination = 0;
        result.m_kvalue_table_number = 0;
        result.m_gas_plant_table_number = 7;
        result.m_surface_eos_number = 2;
        result.m_ngl_density_temperature = 288.71;
        result.m_ngl_density_pressure = 101325.0;

        return result;
    }

    int FieldSepRecord::stage() const {
        return this->m_stage;
    }

    double FieldSepRecord::temperature() const {
        return this->m_temperature;
    }

    double FieldSepRecord::pressure() const {
        return this->m_pressure;
    }

    int FieldSepRecord::liquidDestination() const {
        return this->m_liquid_destination;
    }

    int FieldSepRecord::vaporDestination() const {
        return this->m_vapor_destination;
    }

    int FieldSepRecord::kvalueTableNumber() const {
        return this->m_kvalue_table_number;
    }

    int FieldSepRecord::gasPlantTableNumber() const {
        return this->m_gas_plant_table_number;
    }

    std::optional<int> FieldSepRecord::surfaceEosNumber() const {
        return this->m_surface_eos_number;
    }

    std::optional<double> FieldSepRecord::nglDensityTemperature() const {
        return this->m_ngl_density_temperature;
    }

    std::optional<double> FieldSepRecord::nglDensityPressure() const {
        return this->m_ngl_density_pressure;
    }

    bool FieldSepRecord::operator==(const FieldSepRecord& data) const {
        return this->m_stage == data.m_stage
            && this->m_temperature == data.m_temperature
            && this->m_pressure == data.m_pressure
            && this->m_liquid_destination == data.m_liquid_destination
            && this->m_vapor_destination == data.m_vapor_destination
            && this->m_kvalue_table_number == data.m_kvalue_table_number
            && this->m_gas_plant_table_number == data.m_gas_plant_table_number
            && this->m_surface_eos_number == data.m_surface_eos_number
            && this->m_ngl_density_temperature == data.m_ngl_density_temperature
            && this->m_ngl_density_pressure == data.m_ngl_density_pressure;
    }

    /* ----------------------------------------------------------------- */

    FieldSep::FieldSep(const DeckKeyword& keyword)
    {
        int previous_stage = 0;
        for (const auto& record : keyword) {
            this->m_records.emplace_back(record, keyword.location());

            const int current_stage = this->m_records.back().stage();
            if (current_stage < 1) {
                throw OpmInputError {
                    fmt::format("FIELDSEP stage indices must be positive, "
                                "but got stage {}.", current_stage),
                    keyword.location()
                };
            }

            // previous_stage starts at 0, so this also rejects a non-increasing
            // first stage.
            if (current_stage <= previous_stage) {
                throw OpmInputError {
                    fmt::format("FIELDSEP stages must be given in strictly increasing "
                                "order, but stage {} follows stage {}.",
                                current_stage, previous_stage),
                    keyword.location()
                };
            }

            previous_stage = current_stage;
        }
    }

    FieldSep FieldSep::serializationTestObject()
    {
        FieldSep result;
        result.m_records = { FieldSepRecord::serializationTestObject() };

        return result;
    }

    const FieldSepRecord& FieldSep::getRecord(std::size_t id) const {
        // Callers are expected to index within size(); an out-of-range id is a
        // programming error rather than a recoverable input condition.
        assert(id < this->m_records.size());
        return this->m_records[id];
    }

    bool FieldSep::operator==(const FieldSep& data) const {
        return this->m_records == data.m_records;
    }

} // namespace Opm
