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

    void left_align(std::string& string, std::size_t internal_width) {
        if (string.size() < internal_width) {
            string.append(std::string(internal_width - string.size(), ' '));
        }
    }

    void right_align(std::string& string, std::size_t internal_width) {
        if (string.size() < internal_width) {
            string = std::string(internal_width - string.size(), ' ') + string;
        }
    }

    void centre_align(std::string& string, std::size_t internal_width) {
        if (string.size() < internal_width) {
            std::size_t extra_space { internal_width - string.size() } ;
            std::size_t shift_one { extra_space % 2 } ;

            if (shift_one)
                extra_space--;

            std::size_t left { shift_one + extra_space / 2 }, right { extra_space / 2 } ;

            string = std::string(left, ' ') + string + std::string(right, ' ');
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

            os << ':' << ' ' << string_data << ' ';
        }

        void print_header(std::ostream& os, std::size_t row) const {
            std::string header_line { header[row] } ;
            centre_align(header_line, internal_width);

            os << ':' << ' ' << header_line << ' ';
        }

        constexpr std::size_t total_width() const {
            return internal_width + 2;
        }
    };

    template<typename T, std::size_t header_height>
    struct table: std::vector<column<T, header_height>> {
        using std::vector<column<T, header_height>>::vector;

        std::size_t divider_width() const {
            std::size_t r { 1 + this->size() } ;

            for (const auto& column : *this) {
                r += column.total_width();
            }

            return r;
        }

        void print_divider(std::ostream& os) const {
            os << std::string(divider_width(), '-') << '\n';
        }

        void print_header(std::ostream& os) const {
            print_divider(os);

            for (size_t i { 0 }; i < header_height; ++i) {
                for (const auto& column : *this) {
                    column.print_header(os, i);
                }

                os << ':' << '\n';
            }

            print_divider(os);
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

    constexpr std::size_t well_spec_header_height { 3 } ;

    static const table<Opm::Well, well_spec_header_height> well_specification_columns {
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
    } ;

    void subreport_well_specification_data(std::ostream& os, const Opm::Schedule& schedule, std::size_t report_step) {

        os << "WELL SPECIFICATION DATA\n" << "-----------------------\n" << std::endl;

        well_specification_columns.print_header(os);

        for (const auto& well : schedule.getWells(report_step)) {
            for (const auto& col : well_specification_columns) {
                col.print(os, well);
            }

            os << ':' << '\n';
        }

        well_specification_columns.print_divider(os);

        os << std::endl;
    }

}

void Opm::RptIO::workers::write_WELSPECS(std::ostream& os, unsigned, const Opm::Schedule& schedule, std::size_t report_step) {
    subreport_well_specification_data(os, schedule, report_step);
}
