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
#include <iostream>
#include <cassert>
#include <cmath>
#include <map>

#ifdef _WIN32
#define _USE_MATH_DEFINES
#include <math.h>
#endif

#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/Segment.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/WellSegments.hpp>


namespace Opm {

    const std::string& WellSegments::wellName() const {
        return m_well_name;
    }


  int WellSegments::size() const {
        return m_segments.size();
    }

    double WellSegments::depthTopSegment() const {
        return m_depth_top;
    }

    double WellSegments::lengthTopSegment() const {
        return m_length_top;
    }

    double WellSegments::volumeTopSegment() const {
        return m_volume_top;
    }


    WellSegments::CompPressureDrop WellSegments::compPressureDrop() const {
        return m_comp_pressure_drop;
    }

    WellSegments::MultiPhaseModel WellSegments::multiPhaseModel() const {
        return m_multiphase_model;
    }

    const Segment& WellSegments::operator[](size_t idx) const {
        return m_segments[idx];
    }

    int WellSegments::segmentNumberToIndex(const int segment_number) const {
        const auto it = segment_number_to_index.find(segment_number);
        if (it != segment_number_to_index.end()) {
            return it->second;
        } else {
            return -1;
        }
    }

    void WellSegments::addSegment( Segment new_segment ) {
       // decide whether to push_back or insert
       const int segment_number = new_segment.segmentNumber();

       const int segment_index = segmentNumberToIndex(segment_number);

       if (segment_index < 0) { // it is a new segment
           segment_number_to_index[segment_number] = size();
           m_segments.push_back(new_segment);
       } else { // the segment already exists
           m_segments[segment_index] = new_segment;
       }
   }

    void WellSegments::loadWELSEGS( const DeckKeyword& welsegsKeyword ) {

        // for the first record, which provides the information for the top segment
        // and information for the whole segment set
        const auto& record1 = welsegsKeyword.getRecord(0);
        m_well_name = record1.getItem("WELL").getTrimmedString(0);

        m_segments.clear();

        const double invalid_value = Segment::invalidValue(); // meaningless value to indicate unspecified values

        m_depth_top = record1.getItem("DEPTH").getSIDouble(0);
        m_length_top = record1.getItem("LENGTH").getSIDouble(0);
        m_length_depth_type = LengthDepthFromString(record1.getItem("INFO_TYPE").getTrimmedString(0));
        m_volume_top = record1.getItem("WELLBORE_VOLUME").getSIDouble(0);
        m_comp_pressure_drop = CompPressureDropFromString(record1.getItem("PRESSURE_COMPONENTS").getTrimmedString(0));
        m_multiphase_model = MultiPhaseModelFromString(record1.getItem("FLOW_MODEL").getTrimmedString(0));

        // the main branch is 1 instead of 0
        // the segment number for top segment is also 1
        if (m_length_depth_type == LengthDepth::INC) {
            m_segments.emplace_back( 1, 1, 0, 0., 0.,
                                     invalid_value, invalid_value, invalid_value,
                                     m_volume_top, false );

        } else if (m_length_depth_type == LengthDepth::ABS) {
            m_segments.emplace_back( 1, 1, 0, m_length_top, m_depth_top,
                                     invalid_value, invalid_value, invalid_value,
                                     m_volume_top, true );
        }

        // read all the information out from the DECK first then process to get all the required information
        for (size_t recordIndex = 1; recordIndex < welsegsKeyword.size(); ++recordIndex) {
            const auto& record = welsegsKeyword.getRecord(recordIndex);
            const int segment1 = record.getItem("SEGMENT1").get< int >(0);
            const int segment2 = record.getItem("SEGMENT2").get< int >(0);
            if ((segment1 < 2) || (segment2 < segment1)) {
                throw std::logic_error("illegal segment number input is found in WELSEGS!\n");
            }

            // how to handle the logical relations between lateral branches and parent branches.
            // so far, the branch number has not been used.
            const int branch = record.getItem("BRANCH").get< int >(0);
            if ((branch < 1)) {
                throw std::logic_error("illegal branch number input is found in WELSEGS!\n");
            }
            const int outlet_segment_readin = record.getItem("JOIN_SEGMENT").get< int >(0);
            double diameter = record.getItem("DIAMETER").getSIDouble(0);
            const auto& itemArea = record.getItem("AREA");
            double area;
            if (itemArea.hasValue(0)) {
                area = itemArea.getSIDouble(0);
            } else {
                area = M_PI * diameter * diameter / 4.0;
            }

            // if the values are incremental values, then we can just use the values
            // if the values are absolute values, then we need to calculate them during the next process
            // only the value for the last segment in the range is recorded
            const double segment_length = record.getItem("SEGMENT_LENGTH").getSIDouble(0);
            // the naming is a little confusing here.
            // naming following the definition from the current keyword for the moment
            const double depth_change = record.getItem("DEPTH_CHANGE").getSIDouble(0);

            const auto& itemVolume = record.getItem("VOLUME");
            double volume;
            if (itemVolume.hasValue(0)) {
                volume = itemVolume.getSIDouble(0);
            } else if (m_length_depth_type == LengthDepth::INC) {
                volume = area * segment_length;
            } else {
                volume = invalid_value; // A * L, while L is not determined yet
            }

            const double roughness = record.getItem("ROUGHNESS").getSIDouble(0);

            for (int i = segment1; i <= segment2; ++i) {
                // for the first or the only segment in the range is the one specified in the WELSEGS
                // from the second segment in the range, the outlet segment is the previous segment in the range
                int outlet_segment;
                if (i == segment1) {
                    outlet_segment = outlet_segment_readin;
                } else {
                    outlet_segment = i - 1;
                }

                if (m_length_depth_type == LengthDepth::INC) {
                    m_segments.emplace_back( i, branch, outlet_segment, segment_length, depth_change,
                                             diameter, roughness, area, volume, false );
                } else if (i == segment2) {
                    m_segments.emplace_back( i, branch, outlet_segment, segment_length, depth_change,
                                             diameter, roughness, area, volume, true );
                } else {
                    m_segments.emplace_back( i, branch, outlet_segment, invalid_value, invalid_value,
                                             diameter, roughness, area, volume, false );
                }
            }
        }

        for (size_t i_segment = 0; i_segment < m_segments.size(); ++i_segment) {
            const int segment_number = m_segments[i_segment].segmentNumber();
            const int index = segmentNumberToIndex(segment_number);
            if (index >= 0) { // found in the existing m_segments already
                throw std::logic_error("Segments with same segment number are found!\n");
            }
            segment_number_to_index[segment_number] = i_segment;
        }

        for (size_t i_segment = 0; i_segment < m_segments.size(); ++i_segment) {
            const int segment_number = m_segments[i_segment].segmentNumber();
            const int outlet_segment = m_segments[i_segment].outletSegment();
            if (outlet_segment <= 0) { // no outlet segment
                continue;
            }
            const int outlet_segment_index = segment_number_to_index[outlet_segment];
            m_segments[outlet_segment_index].addInletSegment(segment_number);
        }

    }

    const Segment& WellSegments::getFromSegmentNumber(const int segment_number) const {
        // the index of segment in the vector of segments
        const int segment_index = segmentNumberToIndex(segment_number);
        if (segment_index < 0) {
            throw std::runtime_error("Could not indexate the segment " + std::to_string(segment_number)
                                     + " when trying to get the segment ");
        }
        return m_segments[segment_index];
    }

    void WellSegments::process(bool first_time) {
        if (this->m_length_depth_type == LengthDepth::ABS)
            this->processABS();
        else if (this->m_length_depth_type == LengthDepth::INC)
            this->processINC(first_time);
        else
            throw std::logic_error("Invalid llength/depth/type in segment data structure");
    }


    void WellSegments::processABS() {
        const double invalid_value = Segment::invalidValue(); // meaningless value to indicate unspecified/uncompleted values

        orderSegments();

        int current_index= 1;
        while (current_index< size()) {
            if (m_segments[current_index].dataReady()) {
                current_index++;
                continue;
            }

            const int range_begin = current_index;
            const int outlet_segment = m_segments[range_begin].outletSegment();
            const int outlet_index= segmentNumberToIndex(outlet_segment);

            assert(m_segments[outlet_index].dataReady() == true);

            int range_end = range_begin + 1;
            for (; range_end < size(); ++range_end) {
                if (m_segments[range_end].dataReady() == true) {
                    break;
                }
            }

            if (range_end >= size()) {
                throw std::logic_error(" One range records in WELSEGS is wrong. ");
            }

            // set the length and depth values in the range.
            int number_segments = range_end - range_begin + 1;
            assert(number_segments > 1); //if only 1, the information should be complete

            const double length_outlet = m_segments[outlet_index].totalLength();
            const double depth_outlet = m_segments[outlet_index].depth();

            const double length_last = m_segments[range_end].totalLength();
            const double depth_last = m_segments[range_end].depth();

            // incremental length and depth for the segments within the range
            const double length_inc = (length_last - length_outlet) / number_segments;
            const double depth_inc = (depth_last - depth_outlet) / number_segments;
            const double volume_segment = m_segments[range_end].crossArea() * length_inc;

            for (int k = range_begin; k <= range_end; ++k) {
                Segment new_segment = m_segments[k];
                const double temp_length = length_outlet + (k - range_begin + 1) * length_inc;
                const double temp_depth = depth_outlet + (k - range_end + 1) * depth_inc;
                if (k != range_end) {
                    new_segment.setDepthAndLength(temp_depth, temp_length);
                }

                if (new_segment.volume() < 0.5 * invalid_value) {
                    new_segment.setVolume(volume_segment);
                }
                addSegment(new_segment);
            }
            current_index= range_end + 1;
        }

        // then update the volume for all the segments except the top segment
        // this is for the segments specified individually while the volume is not specified.
        for (int i = 1; i < size(); ++i) {
            assert(m_segments[i].dataReady());
            if (m_segments[i].volume() == invalid_value) {
                Segment new_segment = m_segments[i];
                const int outlet_segment = m_segments[i].outletSegment();
                const int outlet_index = segmentNumberToIndex(outlet_segment);
                const double segment_length = m_segments[i].totalLength() - m_segments[outlet_index].totalLength();
                const double segment_volume = m_segments[i].crossArea() * segment_length;
                new_segment.setVolume(segment_volume);
                addSegment(new_segment);
            }
        }
    }

    void WellSegments::processINC(const bool first_time) {

        // update the information inside the WellSegments to be in ABS way
        if (first_time) {
            Segment new_top_segment = (*this)[0];
            new_top_segment.setDepthAndLength(depthTopSegment(), lengthTopSegment());
            this->addSegment(new_top_segment);
        }

        orderSegments();

        // begin with the second segment
        for (int i_index= 1; i_index< size(); ++i_index) {
            if( m_segments[i_index].dataReady() ) continue;

            // find its outlet segment
            const int outlet_segment = m_segments[i_index].outletSegment();
            const int outlet_index= segmentNumberToIndex(outlet_segment);

            // assert some information of the outlet_segment
            assert(outlet_index>= 0);
            assert(m_segments[outlet_index].dataReady());

            const double outlet_depth = m_segments[outlet_index].depth();
            const double outlet_length = m_segments[outlet_index].totalLength();
            const double temp_depth = outlet_depth + m_segments[i_index].depth();
            const double temp_length = outlet_length + m_segments[i_index].totalLength();

            // applying the calculated length and depth to the current segment
            Segment new_segment = this->m_segments[i_index];
            new_segment.setDepthAndLength(temp_depth, temp_length);
            addSegment(new_segment);
        }
    }

    void WellSegments::orderSegments() {
        // re-ordering the segments to make later use easier.
        // two principles
        // 1. the index of the outlet segment will be stored in the lower index than the segment.
        // 2. the segments belong to the same branch will be continuously stored.

        // top segment will always be the first one
        // before this index, the reordering is done.
        int current_index= 1;

        // clear the mapping from segment number to store index
        segment_number_to_index.clear();
        // for the top segment
        segment_number_to_index[1] = 0;

        while (current_index< size()) {
            // the branch number of the last segment that is done re-ordering
            const int last_branch_number = m_segments[current_index-1].branchNumber();
            // the one need to be swapped to the current_index.
            int target_segment_index= -1;

            // looking for target_segment_index
            for (int i_index= current_index; i_index< size(); ++i_index) {
                const int outlet_segment_number = m_segments[i_index].outletSegment();
                const int outlet_segment_index = segmentNumberToIndex(outlet_segment_number);
                if (outlet_segment_index < 0) { // not found the outlet_segment in the done re-ordering segments
                    continue;
                }
                if (target_segment_index< 0) { // first time found a candidate
                    target_segment_index= i_index;
                } else { // there is already a candidate, chosing the one with the same branch number with last_branch_number
                    const int old_target_segment_index_branch = m_segments[target_segment_index].branchNumber();
                    const int new_target_segment_index_branch = m_segments[i_index].branchNumber();
                    if (new_target_segment_index_branch == last_branch_number) {
                        if (old_target_segment_index_branch != last_branch_number) {
                            target_segment_index= i_index;
                        } else {
                            throw std::logic_error("two segments in the same branch share the same outlet segment !!\n");
                        }
                    }
                }
            }

            if (target_segment_index< 0) {
                throw std::logic_error("could not find candidate segment to swap in before the re-odering process get done !!\n");
            }
            assert(target_segment_index>= current_index);
            if (target_segment_index> current_index) {
                std::swap(m_segments[current_index], m_segments[target_segment_index]);
            }
            const int segment_number = m_segments[current_index].segmentNumber();
            segment_number_to_index[segment_number] = current_index;
            current_index++;
        }
    }

    bool WellSegments::operator==( const WellSegments& rhs ) const {
        return this->m_well_name == rhs.m_well_name
            && this->m_depth_top == rhs.m_depth_top
            && this->m_length_top == rhs.m_length_top
            && this->m_volume_top == rhs.m_volume_top
            && this->m_length_depth_type == rhs.m_length_depth_type
            && this->m_comp_pressure_drop == rhs.m_comp_pressure_drop
            && this->m_multiphase_model == rhs.m_multiphase_model
            && this->m_segments.size() == rhs.m_segments.size()
            && this->segment_number_to_index.size() == rhs.segment_number_to_index.size()
            && std::equal( this->m_segments.begin(),
                           this->m_segments.end(),
                           rhs.m_segments.begin() )
            && std::equal( this->segment_number_to_index.begin(),
                           this->segment_number_to_index.end(),
                           rhs.segment_number_to_index.begin() );
    }

    double WellSegments::segmentLength(const int segment_number) const {
        double segment_length;

        const Segment& segment = getFromSegmentNumber(segment_number);
        if (segment_number == 1) {// top segment
            segment_length = segment.totalLength();
        } else {
            // other segments
            const int outlet_segment_number = segment.outletSegment();
            const Segment &outlet_segment = getFromSegmentNumber(outlet_segment_number);

            segment_length = segment.totalLength() - outlet_segment.totalLength();
        }

        if (segment_length <= 0.)
            throw std::runtime_error(" non positive segemnt length is obtained for segment "
                                     + std::to_string(segment_number));

        return segment_length;
    }

    bool WellSegments::updateWSEGSICD(const std::vector<std::pair<int, SpiralICD> >& sicd_pairs) {
        if (m_comp_pressure_drop == CompPressureDrop::H__) {
            const std::string msg = "to use spiral ICD segment for well " + m_well_name
                                  + " , you have to activate the frictional pressure drop calculation";
            throw std::runtime_error(msg);
        }

        for (const auto& pair_elem : sicd_pairs) {
            const int segment_number = pair_elem.first;
            const SpiralICD& spiral_icd = pair_elem.second;
            Segment segment = this->getFromSegmentNumber(segment_number);
            segment.updateSpiralICD(spiral_icd);
            this->addSegment(segment);
        }

        return true;
    }

    bool WellSegments::operator!=( const WellSegments& rhs ) const {
        return !( *this == rhs );
    }

    std::ostream& operator<<( std::ostream& stream, const WellSegments& well_segments) {
        return stream
            << well_segments.wellName() << " { top: {" <<
            " L: " << well_segments.lengthTopSegment() <<
            " D: " << well_segments.depthTopSegment() <<
            " V: " << well_segments.volumeTopSegment() << " }}";
    }

const std::string WellSegments::LengthDepthToString(LengthDepth enumValue) {
    switch (enumValue) {
    case LengthDepth::INC:
        return "INC";
    case LengthDepth::ABS:
        return "ABS";
    default:
        throw std::invalid_argument("unhandled LengthDepth value");
    }
}


WellSegments::LengthDepth WellSegments::LengthDepthFromString(const std::string& string_value ) {
    if (string_value == "INC") {
        return LengthDepth::INC;
    } else if (string_value == "ABS") {
        return LengthDepth::ABS;
    } else {
        throw std::invalid_argument("Unknown enum string_value: " + string_value + " for LengthDepth");
    }
}


const std::string WellSegments::CompPressureDropToString(CompPressureDrop enumValue) {
    switch (enumValue) {
    case CompPressureDrop::HFA:
        return "HFA";
    case CompPressureDrop::HF_:
        return "HF-";
    case CompPressureDrop::H__:
        return "H--";
    default:
        throw std::invalid_argument("unhandled CompPressureDrop value");
    }
}

WellSegments::CompPressureDrop WellSegments::CompPressureDropFromString( const std::string& string_value ) {

    if (string_value == "HFA") {
        return CompPressureDrop::HFA;
    } else if (string_value == "HF-") {
        return CompPressureDrop::HF_;
    } else if (string_value == "H--") {
        return CompPressureDrop::H__;
    } else {
        throw std::invalid_argument("Unknown enum string_value: " + string_value + " for CompPressureDrop");
    }
}

const std::string WellSegments::MultiPhaseModelToString(MultiPhaseModel enumValue) {
    switch (enumValue) {
    case MultiPhaseModel::HO:
        return "HO";
    case MultiPhaseModel::DF:
        return "DF";
    default:
        throw std::invalid_argument("unhandled MultiPhaseModel value");
    }
}

WellSegments::MultiPhaseModel WellSegments::MultiPhaseModelFromString(const std::string& string_value ) {

    if ((string_value == "HO") || (string_value == "H0")) {
        return MultiPhaseModel::HO;
    } else if (string_value == "DF") {
        return MultiPhaseModel::DF;
    } else {
        throw std::invalid_argument("Unknown enum string_value: " + string_value + " for MultiPhaseModel");
    }

}
}
