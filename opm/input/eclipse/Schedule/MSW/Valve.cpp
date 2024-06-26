/*
  Copyright 2019 Equinor.

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

#include <opm/input/eclipse/Schedule/MSW/Valve.hpp>

#include <opm/io/eclipse/rst/segment.hpp>

#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include "icd_convert.hpp"

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace Opm {

    ValveUDAEval::ValveUDAEval(const SummaryState& summary_state_,
                               const std::string& well_name_,
                               const std::size_t segment_number_)
        : summary_state(summary_state_)
        , well_name(well_name_)
        , segment_number(segment_number_)
    {}

    double ValveUDAEval::value(const UDAValue& value, const double udq_default) const
    {
        if (value.is<double>()) {
            return value.getSI();
        }

        const std::string& string_var = value.get<std::string>();

        double output_value = udq_default;
        if (summary_state.has_segment_var(well_name, string_var, segment_number)) {
            output_value = summary_state.get_segment_var(well_name, string_var, segment_number);
        }
        else if (summary_state.has(string_var)) {
            output_value = summary_state.get(string_var);
        }

        return value.get_dim().convertRawToSi(output_value);
    }

    // Note: The pipe diameter, roughness, and cross-sectional area are not
    // available in OPM's restart file so we use default WSEGVALV values
    // here--i.e., the values from the enclosing segment.  If, however,
    // these parameters were originally assigned non-default values in the
    // base run's input file, those will *not* be picked up in a restarted
    // run.
    Valve::Valve(const RestartIO::RstSegment& rstSegment)
        : m_con_flow_coeff        (rstSegment.valve_flow_coeff)
        , m_con_cross_area        (rstSegment.valve_area)
        , m_con_cross_area_value  (rstSegment.valve_area)
        , m_con_max_cross_area    (rstSegment.valve_max_area)
        , m_pipe_additional_length(rstSegment.valve_length)
        , m_pipe_diameter         (rstSegment.diameter)  // Enclosing segment's value
        , m_pipe_roughness        (rstSegment.roughness) // Enclosing segment's value
        , m_pipe_cross_area       (rstSegment.area)      // Enclosing segment's value
        , m_status                (from_int<ICDStatus>(rstSegment.icd_status))
    {}

    Valve::Valve(const double    conFlowCoeff,
                 const double    conCrossA,
                 const double    conMaxCrossA,
                 const double    pipeAddLength,
                 const double    pipeDiam,
                 const double    pipeRough,
                 const double    pipeCrossA,
                 const ICDStatus stat)
        : m_con_flow_coeff        (conFlowCoeff)
        , m_con_cross_area        (UDAValue(conCrossA))
        , m_con_cross_area_value  (conCrossA)
        , m_con_max_cross_area    (conMaxCrossA)
        , m_pipe_additional_length(pipeAddLength)
        , m_pipe_diameter         (pipeDiam)
        , m_pipe_roughness        (pipeRough)
        , m_pipe_cross_area       (pipeCrossA)
        , m_status                (stat)
    {}

    Valve::Valve(const DeckRecord& record, const double udq_default)
        : m_con_flow_coeff(record.getItem("CV").get<double>(0))
        , m_con_cross_area(record.getItem("AREA").get<UDAValue>(0))
        , m_con_cross_area_value(m_con_cross_area.is<double>() ? m_con_cross_area.getSI() : -1.0)
        , m_udq_default(udq_default)
    {
        // We initialize negative values for the values are defaulted
        const double value_for_default = -1.0e100;

        // TODO: we assume that the value input for this keyword is always positive
        if (record.getItem("EXTRA_LENGTH").defaultApplied(0)) {
            m_pipe_additional_length = value_for_default;
        }
        else {
            m_pipe_additional_length = record.getItem("EXTRA_LENGTH").getSIDouble(0);
        }

        if (record.getItem("PIPE_D").defaultApplied(0)) {
            m_pipe_diameter = value_for_default;
        }
        else {
            m_pipe_diameter = record.getItem("PIPE_D").getSIDouble(0);
        }

        if (record.getItem("ROUGHNESS").defaultApplied(0)) {
            m_pipe_roughness = value_for_default;
        }
        else {
            m_pipe_roughness = record.getItem("ROUGHNESS").getSIDouble(0);
        }

        if (record.getItem("PIPE_A").defaultApplied(0)) {
            m_pipe_cross_area = value_for_default;
        }
        else {
            m_pipe_cross_area = record.getItem("PIPE_A").getSIDouble(0);
        }

        if (record.getItem("STATUS").getTrimmedString(0) == "OPEN") {
            m_status = ICDStatus::OPEN;
        }
        else {
            m_status = ICDStatus::SHUT;
            // TODO: should we check illegal input here
        }

        if (record.getItem("MAX_A").defaultApplied(0)) {
            m_con_max_cross_area = value_for_default;
        }
        else {
            m_con_max_cross_area = record.getItem("MAX_A").getSIDouble(0);
        }
    }

    Valve Valve::serializationTestObject()
    {
        Valve result;
        result.m_con_flow_coeff = 1.0;
        result.m_con_cross_area = UDAValue(2.0);
        result.m_con_cross_area_value = 2.0;
        result.m_con_max_cross_area = 3.0;
        result.m_pipe_additional_length = 4.0;
        result.m_pipe_diameter = 5.0;
        result.m_pipe_roughness = 6.0;
        result.m_pipe_cross_area = 7.0;
        result.m_status = ICDStatus::OPEN;

        return result;
    }

    std::map<std::string, std::vector<std::pair<int, Valve>>>
    Valve::fromWSEGVALV(const DeckKeyword& keyword, const double udq_default)
    {
        auto res = std::map<std::string, std::vector<std::pair<int, Valve>>>{};

        for (const DeckRecord& record : keyword) {
            const std::string well_name = record.getItem("WELL").getTrimmedString(0);

            const int segment_number = record.getItem("SEGMENT_NUMBER").get<int>(0);

            res[well_name].emplace_back(std::piecewise_construct,
                                        std::forward_as_tuple(segment_number),
                                        std::forward_as_tuple(record, udq_default));
        }

        return res;
    }

    ICDStatus Valve::status() const
    {
        return m_status;
    }

    int Valve::ecl_status() const
    {
        return to_int(this->status());
    }

    double Valve::conFlowCoefficient() const
    {
        return m_con_flow_coeff;
    }

    double Valve::conCrossArea(const std::optional<const ValveUDAEval>& uda_eval_optional) const
    {
        m_con_cross_area_value = uda_eval_optional.has_value()
            ? uda_eval_optional.value().value(m_con_cross_area, m_udq_default)
            : m_con_cross_area.getSI();

        return m_con_cross_area_value;
    }

    double Valve::pipeAdditionalLength() const
    {
        return m_pipe_additional_length;
    }

    double Valve::pipeDiameter() const
    {
        return m_pipe_diameter;
    }

    double Valve::pipeRoughness() const
    {
        return m_pipe_roughness;
    }

    double Valve::pipeCrossArea() const
    {
        return m_pipe_cross_area;
    }

    double Valve::conMaxCrossArea() const
    {
        return m_con_max_cross_area;
    }

    void Valve::setPipeDiameter(const double dia)
    {
        m_pipe_diameter = dia;
    }

    void Valve::setPipeRoughness(const double rou)
    {
        m_pipe_roughness = rou;
    }

    void Valve::setPipeCrossArea(const double area)
    {
        m_pipe_cross_area = area;
    }

    void Valve::setConMaxCrossArea(const double area)
    {
        m_con_max_cross_area = area;
    }

    void Valve::setPipeAdditionalLength(const double length)
    {
        m_pipe_additional_length = length;
    }

    bool Valve::operator==(const Valve& data) const
    {
        return (this->conFlowCoefficient() == data.conFlowCoefficient())
            && (this->m_con_cross_area == data.m_con_cross_area)
            && (this->conCrossAreaValue() == data.conCrossAreaValue())
            && (this->conMaxCrossArea() == data.conMaxCrossArea())
            && (this->pipeAdditionalLength() == data.pipeAdditionalLength())
            && (this->pipeDiameter() == data.pipeDiameter())
            && (this->pipeRoughness() == data.pipeRoughness())
            && (this->pipeCrossArea() == data.pipeCrossArea())
            && (this->status() == data.status())
            ;
    }

} // namespace Opm
