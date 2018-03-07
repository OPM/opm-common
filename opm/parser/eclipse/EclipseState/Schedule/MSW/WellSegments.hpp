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

#ifndef SEGMENTSET_HPP_HEADER_INCLUDED
#define SEGMENTSET_HPP_HEADER_INCLUDED

#include <vector>
#include <map>

#include <opm/parser/eclipse/EclipseState/Schedule/MSW/Segment.hpp>

namespace Opm {

    class DeckKeyword;

    class WellSegments {
    public:
        enum class LengthDepth{
            INC = 0,
            ABS = 1
        };
        static const std::string LengthDepthToString(LengthDepth enumValue);
        static LengthDepth LengthDepthFromString(const std::string& stringValue);


        enum class CompPressureDrop {
            HFA = 0,
            HF_ = 1,
            H__ = 2
        };
        static const std::string CompPressureDropToString(CompPressureDrop enumValue);
        static CompPressureDrop CompPressureDropFromString(const std::string& stringValue);


        enum class MultiPhaseModel {
            HO = 0,
            DF = 1
        };
        static const std::string MultiPhaseModelToString(MultiPhaseModel enumValue);
        static MultiPhaseModel MultiPhaseModelFromString(const std::string& stringValue);


        WellSegments() = default;

        const std::string& wellName() const;
        int size() const;
        double depthTopSegment() const;
        double lengthTopSegment() const;
        double volumeTopSegment() const;

        CompPressureDrop compPressureDrop() const;
        MultiPhaseModel multiPhaseModel() const;

        // mapping the segment number to the index in the vector of segments
        int segmentNumberToIndex(const int segment_number) const;

        void addSegment(Segment new_segment);

        void loadWELSEGS( const DeckKeyword& welsegsKeyword);

        const Segment& getFromSegmentNumber(const int segment_number) const;

        const Segment& operator[](size_t idx) const;
        void orderSegments();
        void process(bool first_time);

        bool operator==( const WellSegments& ) const;
        bool operator!=( const WellSegments& ) const;

        double segmentLength(const int segment_number) const;

    private:
        void processABS();
        void processINC(const bool first_time);

        std::string m_well_name;
        // depth of the nodal point of the top segment
        // it is taken as the BHP reference depth of the well
        // BHP reference depth data from elsewhere will be ignored for multi-segmented wells
        double m_depth_top;
        // length down the tubing to the nodal point of the top segment
        double m_length_top;
        // effective wellbore volume of the top segment
        double m_volume_top;
        // type of the tubing length and depth information
        LengthDepth m_length_depth_type;
        // components of the pressure drop to be included
        CompPressureDrop m_comp_pressure_drop;
        // multi-phase flow model
        MultiPhaseModel m_multiphase_model;
        // There are X and Y cooridnate of the nodal point of the top segment
        // Since they are not used for simulations and we are not supporting plotting,
        // we are not handling them at the moment.
        // There are other three properties for segment related to thermal conduction,
        // while they are not supported by the keyword at the moment.

        std::vector< Segment > m_segments;
        // the mapping from the segment number to the
        // storage index in the vector
        std::map<int, int> segment_number_to_index;
    };
    std::ostream& operator<<( std::ostream&, const WellSegments& );
}

#endif
