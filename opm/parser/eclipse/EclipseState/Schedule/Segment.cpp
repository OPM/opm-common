#include <opm/parser/eclipse/EclipseState/Schedule/Segment.hpp>


namespace Opm {

    Segment::Segment()
    : m_segment_number(-1),
      m_branch(-1),
      m_outlet_segment(-1),
      m_length(-1.e100),
      m_depth(-1.e100),
      m_internal_diameter(-1.e100),
      m_roughness(-1.e100),
      m_cross_area(-1.e100),
      m_volume(-1.e100),
      m_length_x(0.0),
      m_length_y(0.0),
      m_data_ready(false)
    {
    }


    Segment::Segment(int segment_number_in, int branch_in, int outlet_segment_in, double length_in, double depth_in,
                     double internal_diameter_in, double roughness_in, double cross_area_in,
                     double volume_in, double length_x_in, double length_y_in, bool data_ready_in)
    : m_segment_number(segment_number_in),
      m_branch(branch_in),
      m_outlet_segment(outlet_segment_in),
      m_length(length_in),
      m_depth(depth_in),
      m_internal_diameter(internal_diameter_in),
      m_roughness(roughness_in),
      m_cross_area(cross_area_in),
      m_volume(volume_in),
      m_length_x(length_x_in),
      m_length_y(length_y_in),
      m_data_ready(data_ready_in)
    {
    }


    int Segment::segmentNumber() const {
        return m_segment_number;
    }


    int Segment::branchNumber() const {
        return m_branch;
    }


    int Segment::outletSegment() const {
        return m_outlet_segment;
    }


    double Segment::length() const {
        return m_length;
    }


    double Segment::depth() const {
        return m_depth;
    }


    double Segment::internalDiameter() const {
        return m_internal_diameter;
    }


    double Segment::roughness() const {
        return m_roughness;
    }


    double Segment::crossArea() const {
        return m_cross_area;
    }


    double Segment::volume() const {
        return m_volume;
    }


    double Segment::lengthX() const {
        return m_length_x;
    }


    double Segment::lengthY() const {
        return m_length_y;
    }


    bool Segment::dataReady() const {
        return m_data_ready;
    }

    void Segment::setLength(const double length_in) {
        m_length = length_in;
    }

    void Segment::setDepth(const double depth_in) {
        m_depth = depth_in;
    }

    void Segment::setVolume(const double volume_in) {
        m_volume = volume_in;
    }

    void Segment::setLengthX(const int length_x_in) {
        m_length_x = length_x_in;
    }

    void Segment::setLengthY(const int length_y_in) {
        m_length_y = length_y_in;
    }

    void Segment::setDataReady(const bool data_ready_in) {
        m_data_ready = data_ready_in;
    }

}
