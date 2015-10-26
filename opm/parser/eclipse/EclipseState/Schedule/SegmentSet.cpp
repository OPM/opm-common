#include <iostream>
#include <cassert>
#include <cmath>
#include <map>

#include <opm/parser/eclipse/EclipseState/Schedule/Segment.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SegmentSet.hpp>


namespace Opm {

    SegmentSet::SegmentSet() {
    }

    std::string SegmentSet::wellName() const {
        return m_well_name;
    }

    int SegmentSet::numberBranch() const {
        return m_number_branch;
    }

    int SegmentSet::numberSegment() const {
        return m_number_segment;
    }

    double SegmentSet::depthTopSegment() const {
        return m_depth_top;
    }

    double SegmentSet::lengthTopSegment() const {
        return m_length_top;
    }

    double SegmentSet::volumeTopSegment() const {
        return m_volume_top;
    }

    double SegmentSet::xTop() const {
        return m_x_top;
    }

    double SegmentSet::yTop() const {
        return m_y_top;
    }

    WellSegment::LengthDepthEnum SegmentSet::lengthDepthType() const {
        return m_length_depth_type;
    }

    WellSegment::CompPresureDropEnum SegmentSet::compPressureDrop() const {
        return m_comp_pressure_drop;
    }

    WellSegment::MultiPhaseModelEnum SegmentSet::multiPhaseModel() const {
        return m_multiphase_model;
    }

    std::vector<SegmentPtr>& SegmentSet::Segments() {
        return m_segments;
    }

    SegmentPtr& SegmentSet::operator[](size_t idx) {
        return m_segments[idx];
    }

    SegmentPtr SegmentSet::operator[](size_t idx) const {
        return m_segments[idx];
    }

    bool SegmentSet::numberToLocation(const int segment_number, int& location) const {
         // std::map<int, int>::iterator it;
         auto it = m_number_to_location.find(segment_number);
         if (it != m_number_to_location.end()) {
             location = it->second;
             return true;
         } else {
             return false;
         }
    }

    int SegmentSet::numberToLocation(const int segment_number) const {
        auto it = m_number_to_location.find(segment_number);
        if (it != m_number_to_location.end()) {
            return it->second;
        } else {
            return -1;
        }
    }

    SegmentSet* SegmentSet::shallowCopy() const {
        SegmentSet* copy = new SegmentSet();
        copy->m_well_name = m_well_name;
        copy->m_number_branch = m_number_branch;
        copy->m_number_segment = m_number_segment;
        copy->m_depth_top = m_depth_top;
        copy->m_length_top = m_length_top;
        copy->m_volume_top = m_volume_top;
        copy->m_length_depth_type = m_length_depth_type;
        copy->m_comp_pressure_drop = m_comp_pressure_drop;
        copy->m_multiphase_model = m_multiphase_model;
        copy->m_x_top = m_x_top;
        copy->m_y_top = m_y_top;
        copy->m_number_to_location = m_number_to_location;
        copy->m_segments.resize(m_segments.size());
        for (int i = 0; i < int(m_segments.size()); ++i) {
            copy->m_segments[i] = m_segments[i];
        }
        return copy;
    }

    void SegmentSet::segmentsFromWELSEGSKeyword(DeckKeywordConstPtr welsegsKeyword) {

        // for the first record, which provides the information for the top segment
        // and information for the whole segment set
        DeckRecordConstPtr record1 = welsegsKeyword->getRecord(0);
        m_well_name = record1->getItem("WELL")->getTrimmedString(0);

        m_segments.clear();

        const double meaningless_value = -1.e100; // meaningless value to indicate unspecified values

        m_depth_top = record1->getItem("DEPTH")->getSIDouble(0);
        m_length_top = record1->getItem("LENGTH")->getSIDouble(0);
        m_length_depth_type = WellSegment::LengthDepthEnumFromString(record1->getItem("INFO_TYPE")->getTrimmedString(0));
        m_volume_top = record1->getItem("WELLBORE_VOLUME")->getSIDouble(0);
        m_comp_pressure_drop = WellSegment::CompPressureDropEnumFromString(record1->getItem("PRESSURE_COMPONENTS")->getTrimmedString(0));
        m_multiphase_model = WellSegment::MultiPhaseModelEnumFromString(record1->getItem("FLOW_MODEL")->getTrimmedString(0));
        m_x_top = record1->getItem("TOP_X")->getSIDouble(0);
        m_y_top = record1->getItem("TOP_Y")->getSIDouble(0);

        // the main branch is 1 instead of 0
        // the segment number for top segment is also 1
        if (m_length_depth_type == WellSegment::INC) {
            SegmentPtr top_segment(new Segment(1, 1, 0, 0., 0., meaningless_value, meaningless_value, meaningless_value,
                                               m_volume_top, 0., 0., false));
            m_segments.push_back(top_segment);
        } else if (m_length_depth_type == WellSegment::ABS) {
            SegmentPtr top_segment(new Segment(1, 1, 0, m_length_top, m_depth_top, meaningless_value, meaningless_value,
                                               meaningless_value, m_volume_top, m_x_top, m_y_top, true));
            m_segments.push_back(top_segment);
        }

        // read all the information out from the DECK first then process to get all the required information
        for (size_t recordIndex = 1; recordIndex < welsegsKeyword->size(); ++recordIndex) {
            DeckRecordConstPtr record = welsegsKeyword->getRecord(recordIndex);
            const int segment1 = record->getItem("SEGMENT1")->getInt(0);
            int segment2 = record->getItem("SEGMENT2")->getInt(0);
            if ((segment1 < 2) || (segment2 < segment1)) {
                throw std::logic_error("illegal segment number input is found in WELSEGS!\n");
            }

            // how to handle the logical relations between lateral branches and parent branches.
            // so far, the branch number has not been used.
            const int branch = record->getItem("BRANCH")->getInt(0);
            if ((branch < 1)) {
                throw std::logic_error("illegal branch number input is found in WELSEGS!\n");
            }
            int outlet_segment = record->getItem("JOIN_SEGMENT")->getInt(0);
            double diameter = record->getItem("DIAMETER")->getSIDouble(0);
            DeckItemConstPtr itemArea = record->getItem("AREA");
            double area;
            if (itemArea->hasValue(0)) {
                area = itemArea->getSIDouble(0);
            } else {
                area = M_PI * diameter * diameter / 4.0;
            }

            double segment_length;
            double depth_change;
            // if the values are incremental values, then we can just use the values
            // if the values are absolute values, then we need to calculate them during the next process
            // only the value for the last segment in the range is recorded
            segment_length = record->getItem("SEGMENT_LENGTH")->getSIDouble(0);
            depth_change = record->getItem("DEPTH_CHANGE")->getSIDouble(0);

            DeckItemConstPtr itemVolume = record->getItem("VOLUME");
            double volume;
            if (itemVolume->hasValue(0)) {
                volume = itemVolume->getSIDouble(0);
            } else if (m_length_depth_type == WellSegment::INC) {
                volume = area * segment_length;
            } else {
                volume = meaningless_value; // A * L, while L is not determined yet
            }

            double roughness = record->getItem("ROUGHNESS")->getSIDouble(0);

            double length_x;
            double length_y;

            length_x = record->getItem("LENGTH_X")->getSIDouble(0);
            length_y = record->getItem("LENGTH_Y")->getSIDouble(0);

            for (int i = segment1; i <= segment2; ++i) {
                // from the second segment in the range, the outlet segment is the previous segment in the segment
                if (i != segment1) {
                    outlet_segment = i - 1;
                }

                if (m_length_depth_type == WellSegment::INC) {
                    m_segments.push_back(std::make_shared<Segment>(i, branch, outlet_segment, segment_length, depth_change,
                                                                   diameter, roughness, area, volume, length_x, length_y, false));
                } else if (i == segment2) {
                    m_segments.push_back(std::make_shared<Segment>(i, branch, outlet_segment, segment_length, depth_change,
                                                                   diameter, roughness, area, volume, length_x, length_y, true));
                } else {
                    m_segments.push_back(std::make_shared<Segment>(i, branch, outlet_segment, meaningless_value, meaningless_value,
                                                                   diameter, roughness, area, volume, meaningless_value, meaningless_value, false));
                }
            }
        }

        m_number_segment = m_segments.size();

        for (int i_segment = 0; i_segment < m_number_segment; ++i_segment){
            const int segment_number = m_segments[i_segment]->segmentNumber();
            const int location = numberToLocation(segment_number);
            if (location >= 0) { // found in the existing m_segments already
                throw std::logic_error("Segments with same segment number are found!\n");
            }
            m_number_to_location[segment_number] = i_segment;
        }

    }
}
