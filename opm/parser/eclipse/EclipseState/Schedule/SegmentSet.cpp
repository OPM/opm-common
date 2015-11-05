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
        return m_segments.size();
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

    WellSegment::LengthDepthEnum SegmentSet::lengthDepthType() const {
        return m_length_depth_type;
    }

    WellSegment::CompPressureDropEnum SegmentSet::compPressureDrop() const {
        return m_comp_pressure_drop;
    }

    WellSegment::MultiPhaseModelEnum SegmentSet::multiPhaseModel() const {
        return m_multiphase_model;
    }

    SegmentConstPtr SegmentSet::operator[](size_t idx) const {
        return m_segments[idx];
    }

    int SegmentSet::numberToLocation(const int segment_number) const {
        auto it = m_number_to_location.find(segment_number);
        if (it != m_number_to_location.end()) {
            return it->second;
        } else {
            return -1;
        }
    }

    void SegmentSet::addSegment(SegmentConstPtr new_segment) {
       // decide whether to push_back or insert
       int segment_number = new_segment->segmentNumber();

       const int segment_location = numberToLocation(segment_number);

       if (segment_location < 0) { // it is a new segment
           m_number_to_location[segment_number] = numberSegment();
           m_segments.push_back(new_segment);
       } else { // the segment already exists
           m_segments[segment_location] = new_segment;
       }
   }

    SegmentSet* SegmentSet::shallowCopy() const {
        SegmentSet* copy = new SegmentSet();
        copy->m_well_name = m_well_name;
        copy->m_number_branch = m_number_branch;
        copy->m_depth_top = m_depth_top;
        copy->m_length_top = m_length_top;
        copy->m_volume_top = m_volume_top;
        copy->m_length_depth_type = m_length_depth_type;
        copy->m_comp_pressure_drop = m_comp_pressure_drop;
        copy->m_multiphase_model = m_multiphase_model;
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

        const double invalid_value = Segment::invalidValue(); // meaningless value to indicate unspecified values

        m_depth_top = record1->getItem("DEPTH")->getSIDouble(0);
        m_length_top = record1->getItem("LENGTH")->getSIDouble(0);
        m_length_depth_type = WellSegment::LengthDepthEnumFromString(record1->getItem("INFO_TYPE")->getTrimmedString(0));
        m_volume_top = record1->getItem("WELLBORE_VOLUME")->getSIDouble(0);
        m_comp_pressure_drop = WellSegment::CompPressureDropEnumFromString(record1->getItem("PRESSURE_COMPONENTS")->getTrimmedString(0));
        m_multiphase_model = WellSegment::MultiPhaseModelEnumFromString(record1->getItem("FLOW_MODEL")->getTrimmedString(0));

        // the main branch is 1 instead of 0
        // the segment number for top segment is also 1
        if (m_length_depth_type == WellSegment::INC) {
            SegmentConstPtr top_segment(new Segment(1, 1, 0, 0., 0., invalid_value, invalid_value, invalid_value,
                                                    m_volume_top, false));
            m_segments.push_back(top_segment);
        } else if (m_length_depth_type == WellSegment::ABS) {
            SegmentConstPtr top_segment(new Segment(1, 1, 0, m_length_top, m_depth_top, invalid_value, invalid_value,
                                                    invalid_value, m_volume_top, true));
            m_segments.push_back(top_segment);
        }

        // read all the information out from the DECK first then process to get all the required information
        for (size_t recordIndex = 1; recordIndex < welsegsKeyword->size(); ++recordIndex) {
            DeckRecordConstPtr record = welsegsKeyword->getRecord(recordIndex);
            const int segment1 = record->getItem("SEGMENT1")->getInt(0);
            const int segment2 = record->getItem("SEGMENT2")->getInt(0);
            if ((segment1 < 2) || (segment2 < segment1)) {
                throw std::logic_error("illegal segment number input is found in WELSEGS!\n");
            }

            // how to handle the logical relations between lateral branches and parent branches.
            // so far, the branch number has not been used.
            const int branch = record->getItem("BRANCH")->getInt(0);
            if ((branch < 1)) {
                throw std::logic_error("illegal branch number input is found in WELSEGS!\n");
            }
            const int outlet_segment_readin = record->getItem("JOIN_SEGMENT")->getInt(0);
            double diameter = record->getItem("DIAMETER")->getSIDouble(0);
            DeckItemConstPtr itemArea = record->getItem("AREA");
            double area;
            if (itemArea->hasValue(0)) {
                area = itemArea->getSIDouble(0);
            } else {
                area = M_PI * diameter * diameter / 4.0;
            }

            // if the values are incremental values, then we can just use the values
            // if the values are absolute values, then we need to calculate them during the next process
            // only the value for the last segment in the range is recorded
            const double segment_length = record->getItem("SEGMENT_LENGTH")->getSIDouble(0);
            // the naming is a little confusing here.
            // naming following the definition from the current keyword for the moment
            const double depth_change = record->getItem("DEPTH_CHANGE")->getSIDouble(0);

            DeckItemConstPtr itemVolume = record->getItem("VOLUME");
            double volume;
            if (itemVolume->hasValue(0)) {
                volume = itemVolume->getSIDouble(0);
            } else if (m_length_depth_type == WellSegment::INC) {
                volume = area * segment_length;
            } else {
                volume = invalid_value; // A * L, while L is not determined yet
            }

            const double roughness = record->getItem("ROUGHNESS")->getSIDouble(0);

            for (int i = segment1; i <= segment2; ++i) {
                // for the first or the only segment in the range is the one specified in the WELSEGS
                // from the second segment in the range, the outlet segment is the previous segment in the range
                int outlet_segment = -1;
                if (i == segment1) {
                    outlet_segment = outlet_segment_readin;
                } else {
                    outlet_segment = i - 1;
                }

                if (m_length_depth_type == WellSegment::INC) {
                    m_segments.push_back(std::make_shared<const Segment>(i, branch, outlet_segment, segment_length, depth_change,
                                                                   diameter, roughness, area, volume, false));
                } else if (i == segment2) {
                    m_segments.push_back(std::make_shared<const Segment>(i, branch, outlet_segment, segment_length, depth_change,
                                                                   diameter, roughness, area, volume, true));
                } else {
                    m_segments.push_back(std::make_shared<const Segment>(i, branch, outlet_segment, invalid_value, invalid_value,
                                                                   diameter, roughness, area, volume, false));
                }
            }
        }

        for (size_t i_segment = 0; i_segment < m_segments.size(); ++i_segment){
            const int segment_number = m_segments[i_segment]->segmentNumber();
            const int location = numberToLocation(segment_number);
            if (location >= 0) { // found in the existing m_segments already
                throw std::logic_error("Segments with same segment number are found!\n");
            }
            m_number_to_location[segment_number] = i_segment;
        }

    }

    void SegmentSet::processABS() {
        const double invalid_value = Segment::invalidValue(); // meaningless value to indicate unspecified/uncompleted values

        bool all_ready;
        do {
            all_ready = true;
            for (int i = 1; i < this->numberSegment(); ++i) {
                if ((*this)[i]->dataReady() == false) {
                    all_ready = false;
                    // then looking for unready segment with a ready outlet segment
                    // and looking for the continous unready segments
                    // int index_begin = i;
                    int location_begin = i;

                    int outlet_segment = (*this)[i]->outletSegment();
                    int outlet_location = this->numberToLocation(outlet_segment);

                    while ((*this)[outlet_location]->dataReady() == false) {
                        location_begin = outlet_location;
                        assert(location_begin > 0);
                        outlet_segment = (*this)[location_begin]->outletSegment();
                        outlet_location = this->numberToLocation(outlet_segment);
                    }

                    // begin from location_begin to look for the unready segments continous
                    int location_end = -1;

                    for (int j = location_begin + 1; j < this->numberSegment(); ++j) {
                        if ((*this)[j]->dataReady() == true) {
                            location_end = j;
                            break;
                        }
                    }
                    if (location_end == -1) {
                        throw std::logic_error("One of the range records in WELSEGS is wrong.");
                    }

                    // set the value for the segments in the range
                    int number_segments = location_end - location_begin + 1;
                    assert(number_segments > 1);

                    const double length_outlet = (*this)[outlet_location]->length();
                    const double depth_outlet = (*this)[outlet_location]->depth();

                    const double length_last = (*this)[location_end]->length();
                    const double depth_last = (*this)[location_end]->depth();

                    const double length_segment = (length_last - length_outlet) / number_segments;
                    const double depth_segment = (depth_last - depth_outlet) / number_segments;

                    // the segments in the same range should share the same properties
                    const double volume_segment = (*this)[location_end]->crossArea() * length_segment;

                    for (int k = location_begin; k < location_end; ++k) {
                        SegmentPtr new_segment = std::make_shared<Segment>((*this)[k]);
                        const double temp_length = length_outlet + (k - location_begin + 1) * length_segment;
                        new_segment->setLength(temp_length);
                        const double temp_depth = depth_outlet + (k - location_begin + 1) * depth_segment;
                        new_segment->setDepth(temp_depth);
                        new_segment->setDataReady(true);

                        if (new_segment->volume() < 0.5 * invalid_value) {
                            new_segment->setVolume(volume_segment);
                        }
                        this->addSegment(new_segment);
                    }
                    break;
                }
            }
       } while (!all_ready);

       // then update the volume for all the segments except the top segment
       // this is for the segments specified individually while the volume is not specified.
       // and also the last segment specified with range
       for (int i = 1; i < this->numberSegment(); ++i) {
           if ((*this)[i]->volume() == invalid_value) {
               SegmentPtr new_segment = std::make_shared<Segment>((*this)[i]);
               const int outlet_segment = (*this)[i]->outletSegment();
               const int outlet_location = this->numberToLocation(outlet_segment);
               const double segment_length = (*this)[i]->length() - (*this)[outlet_location]->length();
               const double segment_volume = (*this)[i]->crossArea() * segment_length;
               new_segment->setVolume(segment_volume);
               this->addSegment(new_segment);
           }
       }
    }

    void SegmentSet::processINC(const bool first_time) {

        // update the information inside the SegmentSet to be in ABS way
        if (first_time) {
            SegmentPtr new_top_segment = std::make_shared<Segment>((*this)[0]);
            new_top_segment->setLength(this->lengthTopSegment());
            new_top_segment->setDepth(this->depthTopSegment());
            new_top_segment->setDataReady(true);
            this->addSegment(new_top_segment);
        }

        bool all_ready;
        do {
            all_ready = true;
            for (int i = 1; i < this->numberSegment(); ++i) {
                if ((*this)[i]->dataReady() == false) {
                    all_ready = false;
                    // check the information about the outlet segment
                    // find the outlet segment with true dataReady()
                    int outlet_segment = (*this)[i]->outletSegment();
                    int outlet_location = this->numberToLocation(outlet_segment);

                    if ((*this)[outlet_location]->dataReady() == true) {
                        SegmentPtr new_segment = std::make_shared<Segment>((*this)[i]);
                        const double temp_length = (*this)[i]->length() + (*this)[outlet_location]->length();
                        new_segment->setLength(temp_length);
                        const double temp_depth = (*this)[i]->depth() + (*this)[outlet_location]->depth();
                        new_segment->setDepth(temp_depth);
                        new_segment->setDataReady(true);
                        this->addSegment(new_segment);
                        break;
                    }

                    int current_location;
                    while ((*this)[outlet_location]->dataReady() == false) {
                        current_location = outlet_location;
                        outlet_segment = (*this)[outlet_location]->outletSegment();
                        outlet_location = this->numberToLocation(outlet_segment);
                        assert((outlet_location >= 0) && (outlet_location < this->numberSegment()));
                    }

                    if ((*this)[outlet_location]->dataReady() == true) {
                        SegmentPtr new_segment = std::make_shared<Segment>((*this)[current_location]);
                        const double temp_length = (*this)[current_location]->length() + (*this)[outlet_location]->length();
                        new_segment->setLength(temp_length);
                        const double temp_depth = (*this)[current_location]->depth() + (*this)[outlet_location]->depth();
                        new_segment->setDepth(temp_depth);
                        new_segment->setDataReady(true);
                        this->addSegment(new_segment);
                        break;
                    }
                }
            }
        } while (!all_ready);
    }

}
