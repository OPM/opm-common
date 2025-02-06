/*
  Copyright 2016 Statoil ASA.

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

#include <opm/input/eclipse/Schedule/MSW/Segment.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/io/eclipse/rst/segment.hpp>

#include <opm/output/eclipse/VectorItems/msw.hpp>

#include <opm/input/eclipse/Schedule/MSW/AICD.hpp>
#include <opm/input/eclipse/Schedule/MSW/SICD.hpp>
#include <opm/input/eclipse/Schedule/MSW/Valve.hpp>

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>

#include <fmt/format.h>

namespace {

    static constexpr double invalid_value = -1.0e100;

    double if_invalid_value(const double rst_value)
    {
        return (rst_value == 0.0)
            ? invalid_value
            : rst_value;
    }

    bool is_valid_value(const double value)
    {
        return value > invalid_value;
    }

    Opm::Segment::SegmentType segmentTypeFromInt(const int ecl_id)
    {
        using SType = Opm::Segment::SegmentType;
        using IType = Opm::RestartIO::Helpers::VectorItems::ISeg::Value::Type;

        switch (ecl_id) {
        case IType::Regular:   return SType::REGULAR;
        case IType::SpiralICD: return SType::SICD;
        case IType::AutoICD:   return SType::AICD;
        case IType::Valve:     return SType::VALVE;
        }

        throw std::invalid_argument {
            fmt::format("Unhandled integer segment type {}", ecl_id)
        };
    }

} // Anonymous namespace

namespace Opm {

    const double Segment::MAX_REL_ROUGHNESS = 3.7 * std::pow((1.0 - 1.0e-3) - 6.9/4000.0, 9. / 10.);

    Segment::Segment()
        : m_segment_number   (-1)
        , m_branch           (-1)
        , m_outlet_segment   (-1)
        , m_total_length     (invalid_value)
        , m_depth            (invalid_value)
        , m_internal_diameter(invalid_value)
        , m_roughness        (invalid_value)
        , m_cross_area       (invalid_value)
        , m_volume           (invalid_value)
        , m_data_ready       (false)
        , m_x                (0.0)
        , m_y                (0.0)
    {}

    Segment::Segment(const RestartIO::RstSegment& rst_segment, const std::string& wname)
        : m_segment_number   (rst_segment.segment)
        , m_branch           (rst_segment.branch)
        , m_outlet_segment   (rst_segment.outlet_segment)
        , m_total_length     (rst_segment.dist_bhp_ref)
        , m_depth            (rst_segment.node_depth)
        , m_internal_diameter(if_invalid_value(rst_segment.diameter))
        , m_roughness        (if_invalid_value(rst_segment.roughness))
        , m_cross_area       (if_invalid_value(rst_segment.area))
        , m_volume           (rst_segment.volume)
        , m_data_ready       (true)
        , m_x                (0.0)
        , m_y                (0.0)
    {
        if (is_valid_value(m_roughness) && is_valid_value(m_internal_diameter)) {
            const double safe_roughness = m_internal_diameter * std::min(MAX_REL_ROUGHNESS, m_roughness/m_internal_diameter);
            if (m_roughness > safe_roughness) {
                OpmLog::warning(fmt::format("Well {} segment {}: Too high roughness {:.3e} is limited to {:.3e} to avoid singularity in friction factor calculation.",
                                            wname, m_segment_number, m_roughness, safe_roughness));
            }
            m_roughness = safe_roughness;
        }

        const auto segment_type = segmentTypeFromInt(rst_segment.segment_type);

        if (segment_type == SegmentType::SICD) {
            this->m_icd.emplace<SICD>(rst_segment);
        }
        else if (segment_type == SegmentType::AICD) {
            this->m_icd.emplace<AutoICD>(rst_segment);
        }
        else if (segment_type == SegmentType::VALVE) {
            this->m_icd.emplace<Valve>(rst_segment);
        }

        // If none of the above type-specific conditions trigger, then this
        // is a RegularSegment and 'm_icd' is fully formed by the variant's
        // default constructor.
    }

    Segment::Segment(const int    segment_number_in,
                     const int    branch_in,
                     const int    outlet_segment_in,
                     const double length_in,
                     const double depth_in,
                     const double internal_diameter_in,
                     const double roughness_in,
                     const double cross_area_in,
                     const double volume_in,
                     const bool   data_ready_in,
                     const double x_in,
                     const double y_in)
        : m_segment_number   (segment_number_in)
        , m_branch           (branch_in)
        , m_outlet_segment   (outlet_segment_in)
        , m_total_length     (length_in)
        , m_depth            (depth_in)
        , m_internal_diameter(internal_diameter_in)
        , m_roughness        (roughness_in)
        , m_cross_area       (cross_area_in)
        , m_volume           (volume_in)
        , m_data_ready       (data_ready_in)
        , m_x                (x_in)
        , m_y                (y_in)
    {}

    Segment::Segment(const Segment& src,
                     const double   new_depth,
                     const double   new_length,
                     const double   new_volume,
                     const double   new_x,
                     const double   new_y)
        : Segment { src, new_depth, new_length, new_volume }
    {
        this->m_x = new_x;
        this->m_y = new_y;
    }

    Segment::Segment(const Segment& src,
                     const double   new_depth,
                     const double   new_length,
                     const double   new_x,
                     const double   new_y)
        : Segment { src, new_depth, new_length }
    {
        this->m_x = new_x;
        this->m_y = new_y;
    }

    Segment::Segment(const Segment& src,
                     const double   new_depth,
                     const double   new_length,
                     const double   new_volume)
        : Segment { src, new_depth, new_length }
    {
        this->m_volume = new_volume;
    }

    Segment::Segment(const Segment& src,
                     const double   new_depth,
                     const double   new_length)
        : Segment(src)
    {
        this->m_depth = new_depth;
        this->m_total_length = new_length;
        this->m_data_ready = true;
    }

    Segment::Segment(const Segment& src, const double new_volume)
        : Segment(src)
    {
        this->m_volume = new_volume;
    }

    Segment Segment::serializationTestObject()
    {
        Segment result;
        result.m_segment_number = 1;
        result.m_branch = 2;
        result.m_outlet_segment = 3;
        result.m_inlet_segments = {4, 5};
        result.m_total_length = 6.0;
        result.m_depth = 7.0;
        result.m_internal_diameter = 8.0;
        result.m_roughness = 9.0;
        result.m_cross_area = 10.0;
        result.m_volume = 11.0;
        result.m_data_ready = true;
        result.m_x = 12.0;
        result.m_y = 13.0;
        result.m_perf_length = 14.0;
        result.m_icd = SICD::serializationTestObject();
        return result;
    }

    int Segment::segmentNumber() const
    {
        return m_segment_number;
    }

    int Segment::branchNumber() const
    {
        return m_branch;
    }

    int Segment::outletSegment() const
    {
        return m_outlet_segment;
    }

    double Segment::totalLength() const
    {
        return m_total_length;
    }

    double Segment::node_X() const { return this->m_x; }
    double Segment::node_Y() const { return this->m_y; }

    double Segment::depth() const
    {
        return m_depth;
    }

    double Segment::perfLength() const
    {
        return *this->m_perf_length;
    }

    double Segment::internalDiameter() const
    {
        return m_internal_diameter;
    }

    double Segment::roughness() const
    {
        return m_roughness;
    }

    double Segment::crossArea() const
    {
        return m_cross_area;
    }

    double Segment::volume() const
    {
        return m_volume;
    }

    bool Segment::dataReady() const
    {
        return m_data_ready;
    }

    Segment::SegmentType Segment::segmentType() const
    {
        if (this->isRegular())
            return SegmentType::REGULAR;

        if (this->isSpiralICD())
            return SegmentType::SICD;

        if (this->isAICD())
            return SegmentType::AICD;

        if (this->isValve())
            return SegmentType::VALVE;

        throw std::logic_error("This just should not happen ");
    }

    const std::vector<int>& Segment::inletSegments() const
    {
        return m_inlet_segments;
    }

    void Segment::addInletSegment(const int segment_number_in)
    {
        auto segPos = std::find(this->m_inlet_segments.begin(),
                                this->m_inlet_segments.end(),
                                segment_number_in);

        if (segPos == this->m_inlet_segments.end()) {
            this->m_inlet_segments.push_back(segment_number_in);
        }
    }

    double Segment::invalidValue()
    {
        return invalid_value;
    }

    bool Segment::operator==(const Segment& rhs) const
    {
        return this->m_segment_number    == rhs.m_segment_number
            && this->m_branch            == rhs.m_branch
            && this->m_outlet_segment    == rhs.m_outlet_segment
            && this->m_total_length      == rhs.m_total_length
            && this->m_depth             == rhs.m_depth
            && this->m_internal_diameter == rhs.m_internal_diameter
            && this->m_roughness         == rhs.m_roughness
            && this->m_cross_area        == rhs.m_cross_area
            && this->m_volume            == rhs.m_volume
            && this->m_perf_length       == rhs.m_perf_length
            && this->m_icd               == rhs.m_icd
            && this->m_data_ready        == rhs.m_data_ready
            && (this->m_x                == rhs.m_x)
            && (this->m_y                == rhs.m_y)
            ;
    }

    bool Segment::operator!=(const Segment& rhs) const
    {
        return ! (*this == rhs);
    }

    void Segment::updateSpiralICD(const SICD& spiral_icd)
    {
        this->m_icd = spiral_icd;
    }

    const SICD& Segment::spiralICD() const
    {
        return std::get<SICD>(this->m_icd);
    }

    void Segment::updateAutoICD(const AutoICD& aicd)
    {
        this->m_icd = aicd;
    }

    const AutoICD& Segment::autoICD() const
    {
        return std::get<AutoICD>(this->m_icd);
    }

    void Segment::updateValve(const Valve& input_valve)
    {
        // we need to update some values for the valve
        auto Valve = input_valve;
        if (Valve.pipeAdditionalLength() < 0) {
            throw std::logic_error("Bug in handling of pipe length for valves");
        }

        if (Valve.pipeDiameter() < 0.) {
            Valve.setPipeDiameter(m_internal_diameter);
        } else {
            this->m_internal_diameter = Valve.pipeDiameter();
        }

        if (Valve.pipeRoughness() < 0.) {
            Valve.setPipeRoughness(m_roughness);
        } else {
            this->m_roughness = Valve.pipeRoughness();
        }

        if (Valve.pipeCrossArea() < 0.) {
            Valve.setPipeCrossArea(m_cross_area);
        } else {
            this->m_cross_area = Valve.pipeCrossArea();
        }

        if (Valve.conMaxCrossArea() < 0.) {
            Valve.setConMaxCrossArea(Valve.pipeCrossArea());
        }

        this->m_icd= Valve;
    }

    void Segment::updateValve__(Valve& valve, const double segment_length)
    {
        if (valve.pipeAdditionalLength() < 0)
            valve.setPipeAdditionalLength(segment_length);

        this->updateValve(valve);
    }

    void Segment::updateValve(const Valve& valve, const double segment_length)
    {
        auto new_valve = valve;
        this->updateValve__(new_valve, segment_length);
    }

    void Segment::updatePerfLength(double perf_length)
    {
        this->m_perf_length = perf_length;
    }

    const Valve& Segment::valve() const
    {
        return std::get<Valve>(this->m_icd);
    }

    int Segment::ecl_type_id() const
    {
        using IType = RestartIO::Helpers::VectorItems::ISeg::Value::Type;

        switch (this->segmentType()) {
        case SegmentType::REGULAR: return IType::Regular;
        case SegmentType::SICD:    return IType::SpiralICD;
        case SegmentType::AICD:    return IType::AutoICD;
        case SegmentType::VALVE:   return IType::Valve;
        }

        throw std::invalid_argument {
            fmt::format("Unhandled segment type '{}'",
                        static_cast<int>(this->segmentType()))
        };
    }

} // namespace Opm
