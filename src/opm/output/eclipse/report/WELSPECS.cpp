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

    void left_align(std::string& string, std::size_t width) {
        if (string.size() < width) {
            string.append(std::string(width - string.size(), field_padding));
        }
    }

    void right_align(std::string& string, std::size_t width) {
        if (string.size() < width) {
            string = std::string(width - string.size(), field_padding) + string;
        }
    }

    void centre_align(std::string& string, std::size_t width) {
        if (string.size() < width) {
            std::size_t extra_space { width - string.size() } ;
            std::size_t shift_one { extra_space % 2 } ;

            if (shift_one)
                extra_space--;

            std::size_t left { shift_one + extra_space / 2 }, right { extra_space / 2 } ;

            string = std::string(left, field_padding) + string + std::string(right, field_padding);
        }
    }

    template<typename T, std::size_t header_height>
    struct column {
        using fetch_function = std::function<std::string(const T&)>;
        using format_function = std::function<void(std::string&, std::size_t)>;

        std::size_t internal_width;
        std::array<std::string, header_height> header;

        fetch_function fetch;
        format_function format = centre_align;

        void print(std::ostream& os, const T& data) const {
            std::string string_data { fetch(data) } ;
            format(string_data, internal_width);
            centre_align(string_data, total_width());

            os << field_separator << string_data;
        }

        void print_header(std::ostream& os, std::size_t row) const {
            std::string header_line { header[row] } ;
            centre_align(header_line, total_width());

            os << field_separator << header_line;
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
                    column.print_header(os, i);
                }

                os << field_separator << record_separator;
            }

            print_divider(os);
        }

        void print_data(std::ostream& os, const std::vector<std::reference_wrapper<const T>> lines) const {
            for (const auto& line : lines) {
                for (const auto& column : *this) {
                    column.print(os, line);
                }

                os << field_separator << record_separator;
            }
        }
    };

    template<typename OutputType, std::size_t header_height, typename InputType = OutputType>
    struct subreport {
        using transform_function = std::function<const std::vector<std::reference_wrapper<const OutputType>>(const InputType&)>;

        static std::vector<std::reference_wrapper<const OutputType>> noop_transform(const InputType&in) {
            return { in };
        }

        std::string title;
        std::string decor;
        table<OutputType, header_height> column_definition;
        transform_function transform;

        subreport(const std::string& _title, const table<OutputType, header_height>& _coldef, transform_function _tf = &subreport::noop_transform)
            : title              { _title           }
            , decor              { underline(title) }
            , column_definition  { _coldef          }
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

    std::string wellhead_location(const Opm::Well& well) {
        auto i { std::to_string(well.getHeadI()) }, j { std::to_string(well.getHeadJ()) } ;

        right_align(i, 3);
        right_align(j, 3);

        return i + ", " + j;
    }

    std::string reference_depth(const Opm::Well& well) {
        return std::to_string(well.getRefDepth()).substr(0,6);
    }

    std::string preferred_phase(const Opm::Well& well) {
        std::ostringstream ss;

        ss << well.getPreferredPhase();

        return ss.str();
    }

    std::string drainage_radius(const Opm::Well&) {
        return "P.EQIV.R"; // From `well.getDrainageRadius()` somehow
    }

    std::string gas_infl_equn(const Opm::Well&) {
        return "STD";
    }

    std::string shut_status(const Opm::Well& well) {
        return Opm::Well::Status2String(well.getStatus());
    }

    std::string cross_flow(const Opm::Well& well) {
        return well.getAllowCrossFlow()
            ? "YES"
            : "NO";
    }

    std::string pvt_tab(const Opm::Well&) {
        return "1";
    }

    std::string dens_calc(const Opm::Well&) {
        return "SEG";
    }

    std::string fip_reg(const Opm::Well&) {
        return "0";
    }

    std::string d_factor(const Opm::Well&) {
        return "0";
    }

    const subreport<Opm::Well, 3> well_specification { "WELL SPECIFICATION DATA", {
        {  8, { "WELL"       , "NAME"       ,               }, &Opm::Well::name     , left_align  },
        {  8, { "GROUP"      , "NAME"       ,               }, &Opm::Well::groupName, left_align  },
        {  8, { "WELLHEAD"   , "LOCATION"   , "( I, J )"    }, wellhead_location    , left_align  },
        {  8, { "B.H.REF"    , "DEPTH"      , "METRES"      }, reference_depth      , right_align },
        {  5, { "PREF-"      , "ERRED"      , "PHASE"       }, preferred_phase      ,             },
        {  8, { "DRAINAGE"   , "RADIUS"     , "METRES"      }, drainage_radius      ,             },
        {  4, { "GAS"        , "INFL"       , "EQUN"        }, gas_infl_equn        ,             },
        {  7, { "SHUT-IN"    , "INSTRCT"    ,               }, shut_status          ,             },
        {  5, { "CROSS"      , "FLOW"       , "ABLTY"       }, cross_flow           ,             },
        {  3, { "PVT"        , "TAB"        ,               }, pvt_tab              ,             },
        {  4, { "WELL"       , "DENS"       , "CALC"        }, dens_calc            ,             },
        {  3, { "FIP"        , "REG"        ,               }, fip_reg              ,             },
        { 11, { "WELL"       , "D-FACTOR"   , "DAY/SM3"     }, d_factor             , right_align },
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

        const std::string& well_name() const {
            return well.name();
        }

        std::string grid_block() const {
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

        std::string cmpl_no() const {
            return std::to_string(connection.complnum());
        }

        std::string centre_depth() const {
            return std::to_string(connection.depth()).substr(0, 6);
        }

        std::string open_shut() const {
            return Opm::Connection::State2String(connection.state());
        }

        std::string sat_tab() const {
            return std::to_string(connection.satTableId());
        }

        std::string conn_factor() const {
            return std::to_string(connection.CF()).substr(0, 10);
        }

        std::string int_diam() const {
            return std::to_string(std::min(connection.r0(), connection.rw()) * 2).substr(0, 8);
        }

        std::string kh_value() const {
            return std::to_string(connection.Kh()).substr(0, 9);
        }

        const std::string &noop() const {
            static const std::string s {};
            return s;
        }

        static std::vector<std::reference_wrapper<const WellConnection>> transform(const Opm::Well& well, std::vector<WellConnection>& out) {
            const auto &connections { well.getConnections() } ;

            for (const auto& connection : connections) {
                out.push_back({ well, connection });
            }

            return { out.begin(), out.end() } ;
        }
    };

    const table<WellConnection, 3> well_connection_columns {
        {  7, {"WELL"                   ,"NAME"                   ,                         }, &WellConnection::well_name       , left_align  },
        { 12, {"GRID"                   ,"BLOCK"                  ,                         }, &WellConnection::grid_block      ,             },
        {  3, {"CMPL"                   ,"NO#"                    ,                         }, &WellConnection::cmpl_no         , right_align },
        {  7, {"CENTRE"                 ,"DEPTH"                  ,"METRES"                 }, &WellConnection::centre_depth    , right_align },
        {  3, {"OPEN"                   ,"SHUT"                   ,                         }, &WellConnection::open_shut       ,             },
        {  3, {"SAT"                    ,"TAB"                    ,                         }, &WellConnection::sat_tab         ,             },
        {  8, {"CONNECTION"             ,"FACTOR*"                ,"CPM3/D/B"               }, &WellConnection::conn_factor     , right_align },
        {  6, {"INT"                    ,"DIAM"                   ,"METRES"                 }, &WellConnection::int_diam        , right_align },
        {  7, {"K  H"                   ,"VALUE"                  ,"MD.METRE"               }, &WellConnection::kh_value        , right_align },
        {  6, {"SKIN"                   ,"FACTOR"                 ,                         }, &WellConnection::noop            ,             },
        { 10, {"CONNECTION"             ,"D-FACTOR"               ,"DAY/SM3"                }, &WellConnection::noop            ,             },
        { 23, {"SATURATION SCALING DATA","SWMIN SWMAX SGMIN SGMAX",                         }, &WellConnection::noop            ,             },
    };

    void subreport_well_connection_data(std::ostream& os, const std::vector<Opm::Well>& data) {
        subreport<WellConnection, 3, Opm::Well> well_connection {
            "WELL CONNECTION DATA",
            well_connection_columns,
            std::bind(&WellConnection::transform, std::placeholders::_1, std::vector<WellConnection> { })
        };

        well_connection.print(os, data);

        os << std::endl;
    }
}

void Opm::RptIO::workers::write_WELSPECS(std::ostream& os, unsigned, const Opm::Schedule& schedule, std::size_t report_step) {
    subreport_well_specification_data(os, schedule.getWells(report_step));
    subreport_well_connection_data(os, schedule.getWells(report_step));
}
