/*
  Copyright 2015 SINTEF ICT, Applied Mathematics.

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

#ifndef SEGMENT_HPP_HEADER_INCLUDED
#define SEGMENT_HPP_HEADER_INCLUDED

#include <opm/input/eclipse/Schedule/MSW/AICD.hpp>
#include <opm/input/eclipse/Schedule/MSW/SICD.hpp>
#include <opm/input/eclipse/Schedule/MSW/Valve.hpp>

#include <optional>
#include <variant>
#include <vector>

#include <cmath>

namespace Opm { namespace RestartIO {
    struct RstSegment;
}} // namespace Opm::RestartIO

namespace Opm {

    class Segment {
    public:
        // Maximum relative roughness to guarantee non-singularity for Re>=4000 in Haaland friction
        // factor calculations (see MSWellHelpers in opm-simulators).
        // cannot be constexpr due to clang not having constexpr std::pow
        static const double MAX_REL_ROUGHNESS; // = 3.7 * std::pow((1.0 - 1.0e-3) - 6.9/4000.0, 9. / 10.);


        enum class SegmentType {
            REGULAR,
            SICD,
            AICD, // Not really supported - just included to complete the enum
            VALVE,
        };

        Segment();

        Segment(const Segment& src, double new_depth, double new_length, double new_volume, double new_x, double new_y);
        Segment(const Segment& src, double new_depth, double new_length, double new_x, double new_y);
        Segment(const Segment& src, double new_depth, double new_length, double new_volume);
        Segment(const Segment& src, double new_depth, double new_length);
        Segment(const Segment& src, double new_volume);
        Segment(const int segment_number_in,
                const int branch_in,
                const int outlet_segment_in,
                const double length_in,
                const double depth_in,
                const double internal_diameter_in,
                const double roughness_in,
                const double cross_area_in,
                const double volume_in,
                const bool data_ready_in,
                const double x_in,
                const double y_in);

        explicit Segment(const RestartIO::RstSegment& rst_segment, const std::string& wname);

        static Segment serializationTestObject();

        int segmentNumber() const;
        int branchNumber() const;
        int outletSegment() const;
        double perfLength() const;
        double totalLength() const;
        double node_X() const;
        double node_Y() const;
        double depth() const;
        double internalDiameter() const;
        double roughness() const;
        double crossArea() const;
        double volume() const;
        bool dataReady() const;

        SegmentType segmentType() const;
        int ecl_type_id() const;

        const std::vector<int>& inletSegments() const;

        static double invalidValue();

        bool operator==( const Segment& ) const;
        bool operator!=( const Segment& ) const;

        const SICD& spiralICD() const;
        const AutoICD& autoICD() const;
        const Valve& valve() const;

        void updatePerfLength(double perf_length);
        void updateSpiralICD(const SICD& spiral_icd);
        void updateAutoICD(const AutoICD& aicd);
        void updateValve(const Valve& valve, const double segment_length);
        void updateValve(const Valve& valve);
        void addInletSegment(const int segment_number);

        bool isRegular() const
        {
            return std::holds_alternative<RegularSegment>(this->m_icd);
        }

        inline bool isSpiralICD() const
        {
            return std::holds_alternative<SICD>(this->m_icd);
        }

        inline bool isAICD() const
        {
            return std::holds_alternative<AutoICD>(this->m_icd);
        }

        inline bool isValve() const
        {
            return std::holds_alternative<Valve>(this->m_icd);
        }

        template<class Serializer>
        void serializeOp(Serializer& serializer)
        {
            serializer(m_segment_number);
            serializer(m_branch);
            serializer(m_outlet_segment);
            serializer(m_inlet_segments);
            serializer(m_total_length);
            serializer(m_depth);
            serializer(m_internal_diameter);
            serializer(m_roughness);
            serializer(m_cross_area);
            serializer(m_volume);
            serializer(m_data_ready);
            serializer(m_x);
            serializer(m_y);
            serializer(m_perf_length);
            serializer(m_icd);
        }

    private:
        // The current serialization of std::variant<> requires that all the
        // types in the variant have a serializeOp() method.  We introduce
        // this RegularSegment to meet this requirement.  Ideally, the ICD
        // variant<> would use std::monostate to represent non-ICD segments.
        struct RegularSegment
        {
            template <class Serializer>
            void serializeOp(Serializer&) {}

            static RegularSegment serializationTestObject() { return {}; }

            bool operator==(const RegularSegment&) const { return true; }
        };

        // segment number
        // it should work as a ID.
        int m_segment_number;

        // branch number
        // for top segment, it should always be 1
        int m_branch;

        // the outlet junction segment
        // for top segment, it should be -1
        int m_outlet_segment;

        // the segments whose outlet segments are the current segment
        std::vector<int> m_inlet_segments;

        // length of the segment node to the bhp reference point.
        // when reading in from deck, with 'INC',
        // it will be incremental length before processing.
        // After processing and in the class Well, it always stores the 'ABS' value.
        // which means the total_length
        double m_total_length;

        // depth of the nodes to the bhp reference point
        // when reading in from deck, with 'INC',
        // it will be the incremental depth before processing.
        // in the class Well, it always stores the 'ABS' value.
        // TODO: to check if it is good to use 'ABS' always.
        double m_depth;

        // tubing internal diameter
        // or the equivalent diameter for annular cross-sections
        // for top segment, it is UNDEFINED
        // we use invalid_value for the top segment
        double m_internal_diameter;

        // effective roughness of the tubing
        // used to calculate the Fanning friction factor
        // for top segment, it is UNDEFINED
        // we use invalid_value for the top segment
        double m_roughness;

        // cross-sectional area for fluid flow
        // not defined for the top segment,
        // we use invalid_value for the top segment.
        double m_cross_area;

        // valume of the segment;
        // it is defined for top segment.
        // TODO: to check if the definition is the same with other segments.
        double m_volume;

        // indicate if the data related to 'INC' or 'ABS' is ready
        // the volume will be updated at a final step.
        bool m_data_ready;

        // Length of segment projected onto the X axis.  Not used in
        // simulations, but needed for the SEG option in WRFTPLT.
        double m_x{};

        // Length of segment projected onto the Y axis.  Not used in
        // simulations, but needed for the SEG option in WRFTPLT.
        double m_y{};

        std::optional<double> m_perf_length;
        std::variant<RegularSegment, SICD, AutoICD, Valve> m_icd;

        // There are three other properties for the segment pertaining to
        // thermal conduction.  These are not currently supported.

        void updateValve__(Valve& valve, const double segment_length);
    };

}

#endif
