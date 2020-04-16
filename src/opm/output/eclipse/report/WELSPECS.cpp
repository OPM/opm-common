/*
  Copyright (c) 2020 Equinor ASA

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

#include <opm/output/eclipse/WriteRPT.hpp>

#include <algorithm>
#include <functional>

#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

namespace {

    constexpr char field_separator   {  ':' } ;
    constexpr char field_padding     {  ' ' } ;
    constexpr char record_separator  { '\n' } ;
    constexpr char section_separator { '\n' } ;
    constexpr char divider_character {  '-' } ;

    void left_align(std::string& string, std::size_t width, std::size_t = 0) {
        if (string.size() < width) {
            string.append(std::string(width - string.size(), field_padding));
        }
    }

    void left_header(std::string& string, std::size_t width, std::size_t line_number) {
        if (line_number == 0) {
            left_align(string, width, line_number);
        } else {
            string = std::string(field_padding, width);
        }
    }

    void right_align(std::string& string, std::size_t width, std::size_t = 0) {
        if (string.size() < width) {
            string = std::string(width - string.size(), field_padding) + string;
        }
    }

    void centre_align(std::string& string, std::size_t width, std::size_t = 0) {
        if (string.size() < width) {
            std::size_t extra_space { width - string.size() } ;
            std::size_t shift_one { extra_space % 2 } ;

            if (shift_one) {
                extra_space--;
            }

            std::size_t left { shift_one + extra_space / 2 }, right { extra_space / 2 } ;

            string = std::string(left, field_padding) + string + std::string(right, field_padding);
        }
    }

    template<typename T>
    const std::string& unimplemented(const T&, std::size_t) {
        static const std::string s { } ;

        return s;
    }

    template<typename T, std::size_t header_height>
    struct column {
        using fetch_function = std::function<std::string(const T&, std::size_t)>;
        using format_function = std::function<void(std::string&, std::size_t, std::size_t)>;

        std::size_t internal_width;
        std::array<std::string, header_height> header;

        fetch_function fetch = unimplemented<T>;
        format_function format = centre_align;

        void print(std::ostream& os, const T& data, std::size_t line_number) const {
            std::string string_data { fetch(data, line_number) } ;
            format(string_data, internal_width, line_number);
            centre_align(string_data, total_width());

            os << string_data;
        }

        void print_header(std::ostream& os, std::size_t row) const {
            std::string header_line { header[row] } ;
            centre_align(header_line, total_width());

            os << header_line;
        }

        constexpr std::size_t total_width() const {
            return internal_width + 2;
        }
    };

    template<typename T, std::size_t header_height>
    struct table: std::vector<column<T, header_height>> {
        using std::vector<column<T, header_height>>::vector;

        std::size_t total_width() const {
            std::size_t r { 1 + this->size() } ;

            for (const auto& column : *this) {
                r += column.total_width();
            }

            return r;
        }

        void print_divider(std::ostream& os, char padding = divider_character) const {
            os << std::string(total_width(), padding) << record_separator;
        }

        void print_header(std::ostream& os) const {
            print_divider(os);

            for (size_t i { 0 }; i < header_height; ++i) {
                for (const auto& column : *this) {
                    os << field_separator;

                    column.print_header(os, i);
                }

                os << field_separator << record_separator;
            }

            print_divider(os);
        }

        void print_data(std::ostream& os, const std::vector<T>& lines) const {
            std::size_t line_number { 0 } ;
            for (const auto& line : lines) {
                for (const auto& column : *this) {
                    os << field_separator;

                    column.print(os, line, line_number);
                }

                os << field_separator << record_separator;

                ++line_number;
            }
        }
    };

    template<typename InputType, typename OutputType, std::size_t header_height>
    struct subreport {
        using transform_function = std::function<std::vector<OutputType>(const InputType&)>;

        std::string title;
        std::string decor;
        table<OutputType, header_height> column_definition;
        char bottom_border;
        transform_function transform;

        subreport(const std::string& _title, const table<OutputType, header_height>& _coldef, char _bottom_border = '-', transform_function _tf = &OutputType::transform)
            : title              { _title           }
            , decor              { underline(title) }
            , column_definition  { _coldef          }
            , bottom_border      { _bottom_border   }
            , transform          { _tf              }
        {
            centre_align(title, column_definition.total_width());
            centre_align(decor, column_definition.total_width());
        }

        std::string underline(const std::string& string) const {
            return std::string(string.size(), divider_character);
        }

        void print(std::ostream& os, const std::vector<InputType>& data) const {
            os << title << record_separator;
            os << decor << record_separator;

            os << section_separator;

            column_definition.print_header(os);
            for (const auto element : data) {
                column_definition.print_data(os, transform(element));
            }
            column_definition.print_divider(os);

            os << section_separator << std::flush;
        }
    };

}

namespace {

    inline std::string box_line(const std::pair<std::string,std::string>& textp, std::size_t line) {
        if (line == 1 || line == 2) {
            std::string text { line == 1 ? textp.first : textp.second } ;
            left_align(text, 72);

            return "*" + text + "*";
        } else {
            return std::string(74, '*');
        }
    }

    std::string wrap_string_for_header(const std::string& string) {
        std::string r { string } ;
        left_align(r, 27);
        centre_align(r, 29);

        return r;
    }

    const std::string header_days_string { "WELSPECS AT       0.00 DAYS" } ;
    std::string header_days(const Opm::Schedule& , std::size_t ) {
        return wrap_string_for_header(header_days_string); // TODO: Calculate properly
    }

    const std::string report_line_string { "REPORT   0     31 DEC 2007"  } ;
    std::string report_line(const Opm::Schedule& , std::size_t ) {
        return wrap_string_for_header(report_line_string); // TODO: Calculate properly
    }

    const std::string header_version_string { "FLOW" } ;
    std::string version_string() {
        return wrap_string_for_header(header_version_string); // TODO: Include in build setup and fetch
    }

    const std::string header_run_time_string { "RUN AT 12:41 ON 12 SEP 2016" } ;
    std::string run_time() {
        return wrap_string_for_header(header_run_time_string); // TODO: Calculate properly
    }


    void write_report_header(std::ostream& os, const Opm::Schedule& schedule, std::size_t report_step) {
        const static std::string filler { std::string(29, ' ') } ;

        const std::pair<std::string,std::string> box_text { "", "" } ;
        os <<
            filler                             << box_line(box_text, 0) << filler           << record_separator <<
            header_days(schedule, report_step) << box_line(box_text, 1) << version_string() << record_separator <<
            report_line(schedule, report_step) << box_line(box_text, 2) << run_time()       << record_separator <<
            filler                             << box_line(box_text, 3) << filler           << record_separator <<
            section_separator;
    }

}

namespace {

    struct WellWrapper {
        const Opm::Well& well;

        static std::vector<WellWrapper> transform(const Opm::Well& well) {
            return {{ well }} ;
        }

        std::string well_name(std::size_t) const {
            return well.name();
        }

        std::string group_name(std::size_t) const {
            return well.groupName();
        }

        std::string wellhead_location(std::size_t) const {
            auto i { std::to_string(well.getHeadI()) }, j { std::to_string(well.getHeadJ()) } ;

            right_align(i, 3);
            right_align(j, 3);

            return i + ", " + j;
        }

        std::string reference_depth(std::size_t) const {
            return std::to_string(well.getRefDepth()).substr(0,6);
        }

        std::string preferred_phase(std::size_t) const {
            std::ostringstream ss;

            ss << well.getPreferredPhase();

            return ss.str();
        }

        std::string pvt_tab(std::size_t) const {
            return std::to_string( well.pvt_table_number() );
        }

        std::string shut_status(std::size_t) const {
            return Opm::Well::Status2String(well.getStatus());
        }

        std::string region_number(std::size_t) const {
            return std::to_string( well.fip_region_number() );
        }

        std::string dens_calc(std::size_t) const {
            if (well.segmented_density_calculation())
                return "SEG";
            return "AVG";
        }

        /*
          Don't know what the D-FACTOR represents, but all examples just show 0;
          we have therefor hardcoded that for now.
        */
        std::string D_factor(std::size_t) const {
            return "0?";
        }

        std::string cross_flow(std::size_t) const {
            return well.getAllowCrossFlow() ? "YES" : "NO";
        }
    };

    const subreport<Opm::Well, WellWrapper, 3> well_specification { "WELL SPECIFICATION DATA", {
        {  8, { "WELL"       , "NAME"       ,               }, &WellWrapper::well_name        , left_align  },
        {  8, { "GROUP"      , "NAME"       ,               }, &WellWrapper::group_name       , left_align  },
        {  8, { "WELLHEAD"   , "LOCATION"   , "( I, J )"    }, &WellWrapper::wellhead_location, left_align  },
        {  8, { "B.H.REF"    , "DEPTH"      , "METRES"      }, &WellWrapper::reference_depth  , right_align },
        {  5, { "PREF-"      , "ERRED"      , "PHASE"       }, &WellWrapper::preferred_phase  ,             },
        {  8, { "DRAINAGE"   , "RADIUS"     , "METRES"      }, unimplemented<WellWrapper>     ,             },
        {  4, { "GAS"        , "INFL"       , "EQUN"        }, unimplemented<WellWrapper>     ,             },
        {  7, { "SHUT-IN"    , "INSTRCT"    ,               }, &WellWrapper::shut_status      ,             },
        {  5, { "CROSS"      , "FLOW"       , "ABLTY"       }, &WellWrapper::cross_flow       ,             },
        {  3, { "PVT"        , "TAB"        ,               }, &WellWrapper::pvt_tab          ,             },
        {  4, { "WELL"       , "DENS"       , "CALC"        }, &WellWrapper::dens_calc        ,             },
        {  3, { "FIP"        , "REG"        ,               }, &WellWrapper::region_number    ,             },
        { 11, { "WELL"       , "D-FACTOR"   , "DAY/SM3"     }, &WellWrapper::D_factor         ,             },
    }};

    void subreport_well_specification_data(std::ostream& os, const std::vector<Opm::Well>& data) {
        well_specification.print(os, data);

        os << std::endl;
    }

}

namespace {

    struct WellConnection {
        const Opm::Well& well;
        const Opm::Connection& connection;

        const std::string& well_name(std::size_t) const {
            return well.name();
        }

        std::string grid_block(std::size_t) const {
            const std::array<int,3> ijk { connection.getI(), connection.getJ(), connection.getK() } ;

            auto compose_coordinates = [](std::string& out, int in) -> std::string {
                constexpr auto delimiter { ',' } ;
                std::string coordinate_part { std::to_string(in) } ;
                right_align(coordinate_part, 3);

                return out.empty()
                    ? coordinate_part
                    : out + delimiter + coordinate_part;
            };

            return std::accumulate(std::begin(ijk), std::end(ijk), std::string {}, compose_coordinates);
        }

        std::string cmpl_no(std::size_t) const {
            return std::to_string(connection.complnum());
        }

        std::string centre_depth(std::size_t) const {
            return std::to_string(connection.depth()).substr(0, 6);
        }

        std::string open_shut(std::size_t) const {
            return Opm::Connection::State2String(connection.state());
        }

        std::string sat_tab(std::size_t) const {
            return std::to_string(connection.satTableId());
        }

        std::string conn_factor(std::size_t) const {
            return std::to_string(connection.CF()).substr(0, 10);
        }

        std::string int_diam(std::size_t) const {
            return std::to_string(connection.rw() * 2).substr(0, 8);
        }

        std::string kh_value(std::size_t) const {
            return std::to_string(connection.Kh()).substr(0, 9);
        }

        std::string skin_factor(std::size_t) const {
            return std::to_string(connection.skinFactor()).substr(0, 8);
        }

        static std::vector<WellConnection> transform(const Opm::Well& well) {
            const auto &connections { well.getConnections() } ;
            std::vector<WellConnection> out;

            for (const auto& connection : connections) {
                out.push_back({ well, connection });
            }

            return out;
        }
    };

    const subreport<Opm::Well, WellConnection, 3> well_connection { "WELL CONNECTION DATA", {
       {  7, {"WELL"                   ,"NAME"                   ,                         }, &WellConnection::well_name       , left_align  },
       { 12, {"GRID"                   ,"BLOCK"                  ,                         }, &WellConnection::grid_block      ,             },
       {  3, {"CMPL"                   ,"NO#"                    ,                         }, &WellConnection::cmpl_no         , right_align },
       {  7, {"CENTRE"                 ,"DEPTH"                  ,"METRES"                 }, &WellConnection::centre_depth    , right_align },
       {  3, {"OPEN"                   ,"SHUT"                   ,                         }, &WellConnection::open_shut       ,             },
       {  3, {"SAT"                    ,"TAB"                    ,                         }, &WellConnection::sat_tab         ,             },
       {  8, {"CONNECTION"             ,"FACTOR*"                ,"CPM3/D/B"               }, &WellConnection::conn_factor     , right_align },
       {  6, {"INT"                    ,"DIAM"                   ,"METRES"                 }, &WellConnection::int_diam        , right_align },
       {  7, {"K  H"                   ,"VALUE"                  ,"MD.METRE"               }, &WellConnection::kh_value        , right_align },
       {  6, {"SKIN"                   ,"FACTOR"                 ,                         }, &WellConnection::skin_factor     , right_align },
       { 10, {"CONNECTION"             ,"D-FACTOR"               ,"DAY/SM3"                }, unimplemented<WellConnection>    ,             },
       { 23, {"SATURATION SCALING DATA","SWMIN SWMAX SGMIN SGMAX",                         }, unimplemented<WellConnection>    ,             },
    }};

}

namespace {

    struct WellSegment {
        const Opm::Well& well;
        const Opm::Connection& connection;
        const Opm::Segment& segment;

        const std::string& well_name(std::size_t) const {
            return well.name();
        }

        std::string connection_grid(std::size_t n) const {
            const WellConnection wc { well, connection } ;

            return wc.grid_block(n);
        }

        std::string segment_number(std::size_t) const {
            return std::to_string(segment.segmentNumber());
        }

        std::string branch_id(std::size_t) const {
            return std::to_string(segment.branchNumber());
        }

        std::string connection_depth(std::size_t) const {
            return std::to_string(connection.depth()).substr(0, 8);
        }

        std::string segment_depth(std::size_t) const {
            return std::to_string(segment.depth()).substr(0, 8);
        }

        std::string length_end_segmt(std::size_t) const {
            return std::to_string(segment.totalLength()).substr(0, 9);
        }

        static std::vector<WellSegment> transform(const Opm::Well& well) {
            const auto &connections { well.getConnections() } ;
            std::vector<WellSegment> out;

            for (const auto& connection : connections) {
                out.push_back({ well, connection, well.getSegments().getFromSegmentNumber(connection.segment()) });
            }

            return out;
        }

        static void ws_format(std::string& string, std::size_t, std::size_t i) {
            if (i == 1) {
                return left_align(string, 8, i);
            } else {
                return right_align(string, 8, i);
            }
        }
    };

    const subreport<Opm::Well, WellSegment, 3> well_multisegment_connection { "MULTI-SEGMENT WELL: CONNECTION DATA", {
        { 8, {"WELL"       , "NAME"       ,              }, &WellSegment::well_name       , left_header },
        { 9, {"CONNECTION" , ""           ,              }, &WellSegment::connection_grid ,             },
        { 5, {"SEGMENT"    , "NUMBER"     ,              }, &WellSegment::segment_number  ,             },
        { 8, {"BRANCH"     , "ID"         ,              }, &WellSegment::branch_id       ,             },
        { 9, {"TUB LENGTH" , "START PERFS", "METRES"     }, unimplemented<WellSegment>    , right_align },
        { 9, {"TUB LENGTH" , "END PERFS"  , "METRES"     }, unimplemented<WellSegment>    , right_align },
        { 9, {"TUB LENGTH" , "CENTR PERFS", "METRES"     }, unimplemented<WellSegment>    , right_align },
        { 9, {"TUB LENGTH" , "END SEGMT"  , "METRES"     }, &WellSegment::length_end_segmt, right_align },
        { 8, {"CONNECTION" , "DEPTH"      , "METRES"     }, &WellSegment::connection_depth, right_align },
        { 8, {"SEGMENT"    , "DEPTH"      , "METRES"     }, &WellSegment::segment_depth   , right_align },
        { 9, {"GRID BLOCK" , "DEPTH"      , "METRES"     }, unimplemented<WellSegment>    , right_align },
    }, '='};



    const subreport<Opm::Well, WellSegment, 3> well_multisegment_data { "MULTI-SEGMENT WELL: SEGMENT STRUCTURE", {
        { 6, { "WELLNAME"  , "AND"        , "SEG TYPE"   }, unimplemented<WellSegment>    , &WellSegment::ws_format },
        { 3, { "SEG"       , "NO"         , ""           }, unimplemented<WellSegment>    , right_align             },
        { 3, { "BRN"       , "NO"         , ""           }, unimplemented<WellSegment>    , right_align             },
        { 5, { "MAIN"      , "INLET"      , "SEGMENT"    }, unimplemented<WellSegment>    , right_align             },
        { 5, { ""          , "OUTLET"     , "SEGMENT"    }, unimplemented<WellSegment>    , right_align             },
        { 7, { "SEGMENT"   , "LENGTH"     , "METRES"     }, unimplemented<WellSegment>    , right_align             },
        { 8, { "TOT LENGTH", "TO END"     , "METRES"     }, unimplemented<WellSegment>    , right_align             },
        { 9, { "DEPTH"     , "CHANGE"     , "METRES"     }, unimplemented<WellSegment>    , right_align             },
        { 8, { "T.V. DEPTH", "AT END"     , "METRES"     }, unimplemented<WellSegment>    , right_align             },
        { 6, { "DIAMETER"  , ""           , "METRES"     }, unimplemented<WellSegment>    , right_align             },
        { 8, { "VFP TAB OR", "ABS ROUGHN" , "METRES"     }, unimplemented<WellSegment>    , right_align             },
        { 7, { "AREA"      , "X-SECTN"    , "M**2"       }, unimplemented<WellSegment>    , right_align             },
        { 7, { "VOLUME"    , ""           , "M3"         }, unimplemented<WellSegment>    , right_align             },
        { 8, { "P DROP"    , "MULT"       , "FACTOR"     }, unimplemented<WellSegment>    , right_align             },
    }, '='};

}

namespace {

    void subreport_well_connection_data(std::ostream& os, const std::vector<Opm::Well>& data) {
        well_connection.print(os, data);

        for (const auto& well : data) {
            if (well.isMultiSegment()) {
                well_multisegment_connection.print(os, { well });
                well_multisegment_data.print(os, { well });
            }
        }

        os << std::endl;
    }

}

void Opm::RptIO::workers::write_WELSPECS(std::ostream& os, unsigned, const Opm::Schedule& schedule, std::size_t report_step) {
    write_report_header(os, schedule, report_step);

    subreport_well_specification_data(os, schedule.getWells(report_step));
    subreport_well_connection_data(os, schedule.getWells(report_step));
}
