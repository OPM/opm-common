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

#include <opm/output/eclipse/report/WellSpecification.hpp>

#include <opm/common/utility/String.hpp>

#include <opm/input/eclipse/EclipseState/Grid/EclipseGrid.hpp>

#include <opm/input/eclipse/Schedule/Group/GTNode.hpp>
#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/Well/WList.hpp>
#include <opm/input/eclipse/Schedule/Well/WListManager.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <opm/input/eclipse/Units/UnitSystem.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <ctime>
#include <functional>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <fmt/chrono.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

namespace {

    constexpr char field_separator   {  ':' };
    constexpr char field_padding     {  ' ' };
    constexpr char record_separator  { '\n' };
    constexpr char section_separator { '\n' };
    constexpr char divider_character {  '-' };

    void left_align(std::string& string,
                    const std::size_t width,
                    const std::size_t = 0)
    {
        if (string.size() < width) {
            string.append(std::string(width - string.size(), field_padding));
        }
    }

    void left_header(std::string& string,
                     const std::size_t width,
                     const std::size_t line_number)
    {
        if (line_number == 0) {
            left_align(string, width, line_number);
        } else {
            string.clear();
        }
    }

    void right_align(std::string& string,
                     const std::size_t width,
                     const std::size_t = 0)
    {
        if (string.size() < width) {
            string = std::string(width - string.size(), field_padding) + string;
        }
    }

    void centre_align(std::string& string,
                      const std::size_t width,
                      const std::size_t = 0)
    {
        if (string.size() < width) {
            std::size_t extra_space = width - string.size();
            const std::size_t shift_one = extra_space % 2;

            if (shift_one) {
                extra_space--;
            }

            const std::size_t left = shift_one + extra_space / 2;
            const std::size_t right = extra_space / 2;

            string = std::string(left, field_padding) + string + std::string(right, field_padding);
        }
    }

    std::string underline(const std::string& string)
    {
        return std::string(string.size(), divider_character);
    }

    struct context {
        const Opm::Schedule& sched;
        Opm::PrtFile::Reports::BlockDepthCallback blockDepth;
    };

    std::string format_number(const Opm::UnitSystem&         unit_system,
                              const Opm::UnitSystem::measure measure,
                              const double                   number,
                              const std::size_t              width)
    {
        return std::to_string(unit_system.from_si(measure, number)).substr(0, width);
    }

    template <typename T>
    const std::string& unimplemented(const T&, const context&, std::size_t, std::size_t)
    {
        static const std::string s{};

        return s;
    }

    template <typename T, std::size_t header_height>
    struct column
    {
        using fetch_function = std::function<std::string(const T&, const context&, std::size_t, std::size_t)>;
        using format_function = std::function<void(std::string&, std::size_t, std::size_t)>;

        std::size_t internal_width;
        std::array<std::string, header_height> header;

        fetch_function fetch { unimplemented<T> };
        format_function format { centre_align };

        std::optional<Opm::UnitSystem::measure> dimension{};

        void print(std::ostream& os,
                   const T& data,
                   const context& ctx,
                   const std::size_t sub_report,
                   const std::size_t line_number) const
        {
            std::string string_data = fetch(data, ctx, sub_report, line_number);

            format(string_data, internal_width, line_number);
            centre_align(string_data, total_width());

            os << string_data;
        }

        std::string header_line(const std::size_t row, const context& ctx) const
        {
            if ((row == header_height) && dimension) {
                return ctx.sched.getUnits().name(dimension.value());
            }
            else {
                return header[row];
            }
        }

        void print_header(std::ostream& os,
                          const std::size_t row,
                          const context& ctx) const
        {
            std::string line = header_line(row, ctx);

            centre_align(line, total_width());

            os << line;
        }

        constexpr std::size_t total_width() const
        {
            return internal_width + 2;
        }
    };

    template <typename T, std::size_t header_height>
    struct table: std::vector<column<T, header_height>>
    {
        using std::vector<column<T, header_height>>::vector;

        std::size_t total_width() const
        {
            return std::accumulate(this->begin(), this->end(), 1 + this->size(),
                                   [](const auto acc, const auto& column)
                                   {
                                       return acc + column.total_width();
                                   });
        }

        void print_divider(std::ostream& os, const char padding = divider_character) const
        {
            os << std::string(total_width(), padding) << record_separator;
        }

        void print_header(std::ostream& os, context ctx) const
        {
            print_divider(os);

            for (std::size_t i = 0; i < header_height; ++i) {
                for (const auto& column : *this) {
                    os << field_separator;

                    column.print_header(os, i, ctx);
                }

                os << field_separator << record_separator;
            }

            print_divider(os);
        }

        void print_data(std::ostream& os,
                        const std::vector<T>& lines,
                        const context& ctx,
                        const std::size_t sub_report) const
        {
            std::size_t line_number = 0;

            for (const auto& line : lines) {
                for (const auto& column : *this) {
                    os << field_separator;
                    column.print(os, line, ctx, sub_report, line_number);
                }

                os << field_separator << record_separator;

                ++line_number;
            }
        }
    };

    template <typename InputType, typename OutputType, std::size_t header_height>
    struct report
    {
        std::string title;
        std::string decor;
        table<OutputType, header_height> column_definition;
        const context ctx;

        report(const std::string&                      _title,
               const table<OutputType, header_height>& _coldef,
               const context&                          _ctx)
            : title              { _title           }
            , decor              { underline(title) }
            , column_definition  { _coldef          }
            , ctx                { _ctx             }
        {
            centre_align(title, column_definition.total_width());
            centre_align(decor, column_definition.total_width());
        }

        void print_header(std::ostream& os) const
        {
            os << title << record_separator
               << decor << record_separator
               << section_separator;

            column_definition.print_header(os, ctx);
        }

        void print_data(std::ostream& os,
                        const std::vector<OutputType>& data,
                        const std::size_t sub_report = 0,
                        const char bottom_border = '-') const
        {
            column_definition.print_data(os, data, this->ctx, sub_report);
            column_definition.print_divider(os, bottom_border);
        }

        void print_footer(std::ostream& os,
                          const std::vector<std::pair<int, std::string>>& footnotes) const
        {
            for (const auto& fnote: footnotes) {
                os << fnote.first << ": " << fnote.second << record_separator;
            }

            os << section_separator;
        }
    };
}

namespace {

    struct WellWrapper
    {
        const Opm::Well& well;

        std::string well_name(const context&, std::size_t, std::size_t) const
        {
            return well.name();
        }

        std::string group_name(const context&, std::size_t, std::size_t) const
        {
            return well.groupName();
        }

        std::string wellhead_location(const context&, std::size_t, std::size_t) const
        {
            auto i = std::to_string(well.getHeadI() + 1);
            auto j = std::to_string(well.getHeadJ() + 1);

            right_align(i, 3);
            right_align(j, 3);

            return i + ", " + j;
        }

        std::string reference_depth(const context& ctx, std::size_t, std::size_t) const
        {
            const auto& usys = ctx.sched.getUnits();

            return well.hasRefDepth()
                ? format_number(usys, Opm::UnitSystem::measure::length, well.getRefDepth(), 6)
                : format_number(usys, Opm::UnitSystem::measure::identity, -1.0e+20, 9);
        }

        std::string preferred_phase(const context&, std::size_t, std::size_t) const
        {
            std::ostringstream ss;

            ss << well.getPreferredPhase();

            return ss.str().substr(0, 3);
        }

        std::string pvt_tab(const context&, std::size_t, std::size_t) const
        {
            return std::to_string( well.pvt_table_number() );
        }

        std::string shutin_instruction(const context&, std::size_t, std::size_t) const
        {
            // Shut-in instruction is 'SHUT' if the well automatically shuts
            // in, and 'STOP' otherwise.
            return well.getAutomaticShutIn() ? "SHUT" : "STOP";
        }

        std::string region_number(const context&, std::size_t, std::size_t) const
        {
            return std::to_string( well.fip_region_number() );
        }

        std::string dens_calc(const context&, std::size_t, std::size_t) const
        {
            return well.segmented_density_calculation() ? "SEG" : "AVG";
        }

        // Don't know what the D-FACTOR represents, but all examples just
        // show 0; we have therefore hardcoded that for now.
        std::string D_factor(const context&, std::size_t, std::size_t) const
        {
            return "0";
        }

        std::string cross_flow(const context&, std::size_t, std::size_t) const
        {
            return well.getAllowCrossFlow() ? "YES" : "NO";
        }

        std::string drainage_radius(const context& ctx, std::size_t, std::size_t) const
        {
            return (well.getDrainageRadius() == 0)
                ? std::string { "P.EQUIV.R" }
                : format_number(ctx.sched.getUnits(),
                                Opm::UnitSystem::measure::length,
                                well.getDrainageRadius(), 6);
        }

        std::string gas_inflow(const context&, std::size_t, std::size_t) const
        {
            return Opm::WellGasInflowEquation2String(well.gas_inflow_equation());
        }
    };

    const table<WellWrapper, 3> well_specification_table {
       {  8, { "WELL"       , "NAME"       ,               }, &WellWrapper::well_name         , left_align  },
       {  8, { "GROUP"      , "NAME"       ,               }, &WellWrapper::group_name        , left_align  },
       {  8, { "WELLHEAD"   , "LOCATION"   , "( I, J )"    }, &WellWrapper::wellhead_location , left_align  },
       {  8, { "B.H.REF"    , "DEPTH"      , "METRES"      }, &WellWrapper::reference_depth   , right_align, Opm::UnitSystem::measure::length },
       {  5, { "PREF-"      , "ERRED"      , "PHASE"       }, &WellWrapper::preferred_phase   ,             },
       {  8, { "DRAINAGE"   , "RADIUS"     , "METRES"      }, &WellWrapper::drainage_radius   , right_align, Opm::UnitSystem::measure::length },
       {  4, { "GAS"        , "INFL"       , "EQUN"        }, &WellWrapper::gas_inflow        ,             },
       {  7, { "SHUT-IN"    , "INSTRCT"    ,               }, &WellWrapper::shutin_instruction,             },
       {  5, { "CROSS"      , "FLOW"       , "ABLTY"       }, &WellWrapper::cross_flow        ,             },
       {  3, { "PVT"        , "TAB"        ,               }, &WellWrapper::pvt_tab           ,             },
       {  4, { "WELL"       , "DENS"       , "CALC"        }, &WellWrapper::dens_calc         ,             },
       {  3, { "FIP"        , "REG"        ,               }, &WellWrapper::region_number     ,             },
       { 11, { "WELL"       , "D-FACTOR 1" , "DAY/SM3"     }, &WellWrapper::D_factor          ,             },
    };


    void report_well_specification_data(std::ostream& os,
                                        const std::vector<Opm::Well>& data,
                                        const context& ctx)
    {
        report<Opm::Well, WellWrapper, 3> well_specification {
            "WELL SPECIFICATION DATA", well_specification_table, ctx
        };

        std::vector<WellWrapper> wrapper_data;
        wrapper_data.reserve(data.size());

        std::transform(data.begin(), data.end(),
                       std::back_inserter(wrapper_data),
                       [](const Opm::Well& well)
                       { return WellWrapper { well }; });

        well_specification.print_header(os);
        well_specification.print_data(os, wrapper_data);

        well_specification.print_footer(os, {{1, "The WELL D-FACTOR is not implemented "
                                                  "- and the report will always show "
                                                  "the default value 0."}});
    }

}

namespace {

    struct GroupWrapper
    {
        const Opm::GTNode& node;

        const std::string& group_name(const context&, std::size_t, std::size_t) const
        {
            return node.group().name();
        }

        std::string group_level(const context&, std::size_t, std::size_t) const
        {
            return std::to_string(node.level());
        }

        const std::string& group_parent(const context&, std::size_t, std::size_t) const
        {
            return node.parent_name();
        }
    };

    void report_group_levels_data(std::ostream& os,
                                  const context& ctx,
                                  const std::size_t report_step)
    {
        const table<GroupWrapper, 2> group_levels_table {
            { 8, { "GROUP"   , "NAME"     }, &GroupWrapper::group_name  , left_align },
            { 5, { "LEVEL"   ,            }, &GroupWrapper::group_level ,            },
            { 8, { "PARENT"  , "GROUP"    }, &GroupWrapper::group_parent, left_align },
        };

        const report<Opm::GTNode, GroupWrapper, 2> group_levels {
            "GROUP LEVELS", group_levels_table, ctx
        };

        group_levels.print_header(os);

        const Opm::GTNode root = ctx.sched.groupTree(report_step);

        std::vector<const Opm::GTNode*> nodes = root.all_nodes();

        std::vector<GroupWrapper> data {};
        data.reserve(nodes.size() - 1);

        std::transform(++nodes.begin(), nodes.end(),
                       std::back_inserter(data),
                       [](const Opm::GTNode* node)
                       { return GroupWrapper { *node }; });

        group_levels.print_data(os, data);
        group_levels.print_footer(os, {});
    }
}

namespace {

    struct WellConnection
    {
        const Opm::Well& well;
        const Opm::Connection& connection;

        const std::string& well_name(const context&, std::size_t, std::size_t) const {
            return well.name();
        }

        std::string grid_block(const context&, std::size_t, std::size_t) const
        {
            const std::array<int,3> ijk {
                connection.getI() + 1, connection.getJ() + 1, connection.getK() + 1
            };

            auto compose_coordinates =
                [](const std::string& out, const int in) -> std::string
                {
                    constexpr auto delimiter { ',' };

                    std::string coordinate_part = std::to_string(in);
                    right_align(coordinate_part, 3);

                    return out.empty()
                        ? coordinate_part
                        : out + delimiter + coordinate_part;
                };

            return std::accumulate(std::begin(ijk), std::end(ijk),
                                   std::string {}, compose_coordinates);
        }

        std::string cmpl_no(const context&, std::size_t, std::size_t) const
        {
            return std::to_string(connection.complnum());
        }

        std::string centre_depth(const context& ctx, std::size_t, std::size_t) const
        {
            return format_number(ctx.sched.getUnits(),
                                 Opm::UnitSystem::measure::length,
                                 connection.depth(), 6);
        }

        std::string open_shut(const context&, std::size_t, std::size_t) const
        {
            return Opm::Connection::State2String(connection.state());
        }

        std::string sat_tab(const context&, std::size_t, std::size_t) const
        {
            return std::to_string(connection.satTableId());
        }

        std::string conn_factor(const context& ctx, std::size_t, std::size_t) const
        {
            return format_number(ctx.sched.getUnits(),
                                 Opm::UnitSystem::measure::transmissibility,
                                 connection.CF(), 10);
        }

        std::string int_diam(const context& ctx, std::size_t, std::size_t) const
        {
            return format_number(ctx.sched.getUnits(),
                                 Opm::UnitSystem::measure::length,
                                 connection.rw() * 2, 8);
        }

        std::string kh_value(const context& ctx, std::size_t, std::size_t) const
        {
            return format_number(ctx.sched.getUnits(),
                                 Opm::UnitSystem::measure::effective_Kh,
                                 connection.Kh(), 9);
        }

        std::string skin_factor(const context&, std::size_t, std::size_t) const
        {
            return std::to_string(connection.skinFactor()).substr(0, 8);
        }

        std::string sat_scaling(const context&, std::size_t, std::size_t) const
        {
            return "";
        }

        const std::string dfactor(const context& ctx, std::size_t, std::size_t) const
        {
            return format_number(ctx.sched.getUnits(),
                                 Opm::UnitSystem::measure::dfactor,
                                 connection.dFactor(), 12);
        }

    };

    const table<WellConnection, 3> connection_table {
       {  7, {"WELL"                   ,"NAME"                     ,                         }, &WellConnection::well_name       , left_align  },
       { 12, {"GRID"                   ,"BLOCK"                    ,                         }, &WellConnection::grid_block      ,             },
       {  3, {"CMPL"                   ,"NO#"                      ,                         }, &WellConnection::cmpl_no         , right_align },
       {  7, {"CENTRE"                 ,"DEPTH"                    ,"METRES"                 }, &WellConnection::centre_depth    , right_align, Opm::UnitSystem::measure::length           },
       {  3, {"OPEN"                   ,"SHUT"                     ,                         }, &WellConnection::open_shut       ,             },
       {  3, {"SAT"                    ,"TAB"                      ,                         }, &WellConnection::sat_tab         ,             },
       { 11, {"CONNECTION"             ,"FACTOR*"                  ,"CPM3/D/B"               }, &WellConnection::conn_factor     , right_align, Opm::UnitSystem::measure::transmissibility },
       {  6, {"INT"                    ,"DIAM"                     ,"METRES"                 }, &WellConnection::int_diam        , right_align, Opm::UnitSystem::measure::length           },
       {  7, {"K  H"                   ,"VALUE"                    ,"MD.METRE"               }, &WellConnection::kh_value        , right_align },
       {  6, {"SKIN"                   ,"FACTOR"                   ,                         }, &WellConnection::skin_factor     , right_align },
       { 10, {"CONNECTION"             ,"D-FACTOR"                 ,"DAY/SM3"                }, &WellConnection::dfactor         ,             },
       { 23, {"SATURATION SCALING DATA","SWMIN SWMAX SGMIN SGMAX 1",                         }, &WellConnection::sat_scaling     ,             }};

}

namespace {

    struct SegmentConnection
    {
        const Opm::Well& well;
        const Opm::Connection& connection;
        const Opm::Segment& segment;

        const std::pair<double,double>& perf_range;

        const std::string& well_name(const context&, std::size_t, std::size_t) const
        {
            return well.name();
        }

        std::string connection_grid(const context& ctx,
                                    const std::size_t sub_report,
                                    const std::size_t n) const
        {
            const WellConnection wc { well, connection };

            return wc.grid_block(ctx, sub_report, n);
        }

        std::string segment_number(const context&, std::size_t, std::size_t) const
        {
            return std::to_string(segment.segmentNumber());
        }

        std::string branch_id(const context&, std::size_t, std::size_t) const
        {
            return std::to_string(segment.branchNumber());
        }

        std::string perf_start_length(const context& ctx, std::size_t, std::size_t) const
        {
            return format_number(ctx.sched.getUnits(),
                                 Opm::UnitSystem::measure::length,
                                 perf_range.first, 6);
        }

        std::string perf_mid_length(const context& ctx, std::size_t, std::size_t) const
        {
            return format_number(ctx.sched.getUnits(),
                                 Opm::UnitSystem::measure::length,
                                 (perf_range.first + perf_range.second) / 2.0, 6);
        }

        std::string perf_end_length(const context& ctx, std::size_t, std::size_t) const
        {
            return format_number(ctx.sched.getUnits(),
                                 Opm::UnitSystem::measure::length,
                                 perf_range.second, 6);
        }

        std::string length_end_segmt(const context& ctx, std::size_t, std::size_t) const
        {
            return format_number(ctx.sched.getUnits(),
                                 Opm::UnitSystem::measure::length,
                                 segment.totalLength(), 6);
        }

        std::string connection_depth(const context& ctx, std::size_t, std::size_t) const
        {
            return format_number(ctx.sched.getUnits(),
                                 Opm::UnitSystem::measure::length,
                                 connection.depth(), 6);
        }

        std::string segment_depth(const context& ctx, std::size_t, std::size_t) const
        {
            return format_number(ctx.sched.getUnits(),
                                 Opm::UnitSystem::measure::length,
                                 segment.depth(), 6);
        }

        std::string grid_block_depth(const context& ctx, std::size_t, std::size_t) const
        {
            return format_number(ctx.sched.getUnits(),
                                 Opm::UnitSystem::measure::length,
                                 ctx.blockDepth(connection.global_index()), 6);
        }

        static void ws_format(std::string& string,
                              const std::size_t,
                              const std::size_t i)
        {
            if (i == 0) {
                left_align(string, 8, i);
            } else {
                right_align(string, 8, i);
            }
        }

    };

    struct WellSegment
    {
        const Opm::Well& well;
        const Opm::Segment& segment;

        std::string well_name_seg(const context&,
                                  const std::size_t sub_report,
                                  const std::size_t n) const
        {
            if (sub_report > 0) {
                return "";
            }

            if (n == 0) {
                return well.name();
            }

            if (n == 1) {
                return Opm::WellSegments::CompPressureDropToString
                    (well.getSegments().compPressureDrop());
            }

            return "";
        }

        std::string segment_number(const context&, std::size_t, std::size_t) const
        {
            return std::to_string(segment.segmentNumber());
        }

        std::string branch_number(const context&, std::size_t, const std::size_t n) const
        {
            return (n == 0)
                ? std::to_string(segment.branchNumber())
                : "";
        }

        std::string main_inlet(const context&, std::size_t, std::size_t) const
        {
            const auto& inlets = segment.inletSegments();

            return inlets.empty() ? "0" : std::to_string(inlets.front());
        }

        std::string outlet(const context&, std::size_t, std::size_t) const
        {
            return std::to_string(segment.outletSegment());
        }

        std::string total_length(const context& ctx, std::size_t, std::size_t) const
        {
            return format_number(ctx.sched.getUnits(),
                                 Opm::UnitSystem::measure::length,
                                 segment.totalLength(), 6);
        }

        std::string length(const context& ctx,
                           const std::size_t sub_report,
                           const std::size_t line_number) const
        {
            if (segment.segmentNumber() == 1) {
                return total_length(ctx, sub_report, line_number);
            }

            const auto& segments = well.getSegments();
            const auto& outlet_segment = segments.getFromSegmentNumber(segment.outletSegment());

            return format_number(ctx.sched.getUnits(),
                                 Opm::UnitSystem::measure::length,
                                 segment.totalLength() - outlet_segment.totalLength(), 6);
        }

        std::string t_v_depth(const context& ctx, std::size_t, std::size_t) const
        {
            return format_number(ctx.sched.getUnits(),
                                 Opm::UnitSystem::measure::length,
                                 segment.depth(), 6);
        }

        std::string depth_change(const context& ctx,
                                 const std::size_t sub_report,
                                 const std::size_t line_number) const
        {
            if (segment.segmentNumber() == 1) {
                return t_v_depth(ctx, sub_report, line_number);
            }

            const auto& segments = well.getSegments();
            const auto& outlet_segment = segments.getFromSegmentNumber(segment.outletSegment());

            return format_number(ctx.sched.getUnits(),
                                 Opm::UnitSystem::measure::length,
                                 segment.depth() - outlet_segment.depth(), 6);
        }

        std::string internal_diameter(const context& ctx, std::size_t, std::size_t) const
        {
            const auto number = segment.internalDiameter();

            return (number == Opm::Segment::invalidValue())
                ? "0"
                : format_number(ctx.sched.getUnits(),
                                Opm::UnitSystem::measure::length,
                                number, 6);
        }

        std::string roughness(const context& ctx, std::size_t, std::size_t) const
        {
            const auto number = segment.roughness();

            return (number == Opm::Segment::invalidValue())
                ? "0"
                : format_number(ctx.sched.getUnits(),
                                Opm::UnitSystem::measure::length,
                                number, 8);
        }

        std::string cross_section(const context&, std::size_t, std::size_t) const
        {
            const auto number = segment.crossArea();

            return (number == Opm::Segment::invalidValue())
                ? "0"
                : std::to_string(number).substr(0, 7);
        }

        std::string volume(const context& ctx, std::size_t, std::size_t) const
        {
            return format_number(ctx.sched.getUnits(),
                                 Opm::UnitSystem::measure::volume,
                                 segment.volume(), 5);
        }

        std::string pressure_drop_mult(const context&, std::size_t, std::size_t) const
        {
            return std::to_string(1.0).substr(0, 5);
        }

        static void ws_format(std::string& string, std::size_t, std::size_t i) {
            if (i == 0) {
                left_align(string, 8, i);
            } else {
                right_align(string, 8, i);
            }
        }
    };

    const table<SegmentConnection, 3> msw_connection_table {
        {  8, {"WELL"       , "NAME"       ,              }, &SegmentConnection::well_name        , left_header },
        {  9, {"CONNECTION" , ""           ,              }, &SegmentConnection::connection_grid  ,             },
        {  5, {"SEGMENT"    , "NUMBER"     ,              }, &SegmentConnection::segment_number   , right_align },
        {  8, {"BRANCH"     , "ID"         ,              }, &SegmentConnection::branch_id        ,             },
        {  9, {"TUB LENGTH" , "START PERFS", "METRES"     }, &SegmentConnection::perf_start_length, right_align, Opm::UnitSystem::measure::length },
        {  9, {"TUB LENGTH" , "END PERFS"  , "METRES"     }, &SegmentConnection::perf_end_length  , right_align, Opm::UnitSystem::measure::length },
        {  9, {"TUB LENGTH" , "CENTR PERFS", "METRES"     }, &SegmentConnection::perf_mid_length  , right_align, Opm::UnitSystem::measure::length },
        {  9, {"TUB LENGTH" , "END SEGMT"  , "METRES"     }, &SegmentConnection::length_end_segmt , right_align, Opm::UnitSystem::measure::length },
        {  8, {"CONNECTION" , "DEPTH"      , "METRES"     }, &SegmentConnection::connection_depth , right_align, Opm::UnitSystem::measure::length },
        {  8, {"SEGMENT"    , "DEPTH"      , "METRES"     }, &SegmentConnection::segment_depth    , right_align, Opm::UnitSystem::measure::length },
        {  9, {"GRID BLOCK" , "DEPTH"      , "METRES"     }, &SegmentConnection::grid_block_depth , right_align, Opm::UnitSystem::measure::length },
    };

    const table<WellSegment, 3> msw_well_table {
        {  6, { "WELLNAME"  , "AND"        , "SEG TYPE"   }, &WellSegment::well_name_seg       , &WellSegment::ws_format },
        {  3, { "SEG"       , "NO"         , ""           }, &WellSegment::segment_number      , right_align             },
        {  3, { "BRN"       , "NO"         , ""           }, &WellSegment::branch_number       , right_align             },
        {  5, { "MAIN"      , "INLET"      , "SEGMENT"    }, &WellSegment::main_inlet          , right_align             },
        {  5, { ""          , "OUTLET"     , "SEGMENT"    }, &WellSegment::outlet              , right_align             },
        {  7, { "SEGMENT"   , "LENGTH"     , "METRES"     }, &WellSegment::length              , right_align            , Opm::UnitSystem::measure::length },
        {  8, { "TOT LENGTH", "TO END"     , "METRES"     }, &WellSegment::total_length        , right_align            , Opm::UnitSystem::measure::length },
        {  8, { "DEPTH"     , "CHANGE"     , "METRES"     }, &WellSegment::depth_change        , right_align            , Opm::UnitSystem::measure::length },
        {  8, { "T.V. DEPTH", "AT END"     , "METRES"     }, &WellSegment::t_v_depth           , right_align            , Opm::UnitSystem::measure::length },
        {  6, { "DIA OR F"  , "SCALING"    , "METRES"     }, &WellSegment::internal_diameter   , right_align            , Opm::UnitSystem::measure::length },
        {  8, { "VFP TAB OR", "ABS ROUGHN" , "METRES"     }, &WellSegment::roughness           , right_align            , Opm::UnitSystem::measure::length },
        {  7, { "AREA"      , "X-SECTN"    , "M**2"       }, &WellSegment::cross_section       , right_align            },
        {  7, { "VOLUME"    , ""           , "M3"         }, &WellSegment::volume              , right_align            , Opm::UnitSystem::measure::volume },
        {  8, { "P DROP"    , "MULT"       , "FACTOR 1"   }, &WellSegment::pressure_drop_mult  , right_align             },
    };
}

namespace {

    const std::string hierarchy_title     { "HIERARCHICAL DESCRIPTION OF GROUP CONTROL STRUCTURE" };
    const std::string hierarchy_underline { underline(hierarchy_title)                            };

    constexpr char horizontal_line  { '-' };
    constexpr char vertical_line    { '|' };
    constexpr char indent_character { ' ' };

    std::string decorate_hierarchy_name(const std::string& name,
                                        const bool first_line,
                                        const bool last_child)
    {
        if (first_line) {
            return std::string(1, vertical_line) + std::string(3, horizontal_line) + name;
        } else if (last_child) {
            return                                 std::string(4, indent_character) + name;
        } else {
            return std::string(1, vertical_line) + std::string(3, indent_character) + name;
        }
    }

    std::vector<std::string> lines_for_node(const Opm::GTNode& node)
    {
        std::vector<std::string> lines { node.group().name() };

        const auto& children = node.groups();

        if (children.empty()) {
            return lines;
        }

        lines.push_back(std::string(1, vertical_line));

        std::size_t i = 0;
        for (const auto& child : children) {
            ++i;
            std::vector<std::string> child_lines = lines_for_node(child);

            bool first_line = true;
            for (const auto& line : child_lines) {
                lines.push_back(decorate_hierarchy_name(line, first_line, i == children.size()));

                first_line = false;
            }
        }

        return lines;
    }

    void report_group_hierarchy_data(std::ostream& os,
                                     const context& ctx,
                                     const std::size_t report_step = 0)
    {
        os << hierarchy_title     << record_separator
           << hierarchy_underline << record_separator
           << section_separator;

        for (const auto& line : lines_for_node(ctx.sched.groupTree(report_step))) {
            os << line << record_separator;
        }

        os << section_separator << std::flush;
    }

}

namespace {

    void report_well_connection_data(std::ostream& os,
                                     const std::vector<Opm::Well>& data,
                                     const context& ctx)
    {
        const report<Opm::Well, WellConnection, 3> well_connection {
            "WELL CONNECTION DATA", connection_table, ctx
        };

        well_connection.print_header(os);

        std::size_t sub_report = 0;
        for (const auto& well : data) {
            const auto& connections = well.getConnections();

            std::vector<WellConnection> wrapper_data{};
            wrapper_data.reserve(connections.size());

            std::transform(connections.begin(),
                           connections.end(),
                           std::back_inserter(wrapper_data),
                           [&well](const Opm::Connection& connection)
                           { return WellConnection { well, connection }; });

            well_connection.print_data(os, wrapper_data, sub_report);
            ++sub_report;
        }

        well_connection.print_footer(os, {
                {1, "The saturation scaling data has not "
                 "been implemented in the report and will "
                 "always be blank."}
            });
    }

    void report_mswell_segment_data(std::ostream&    os,
                                    const Opm::Well& well,
                                    const context&   ctx)
    {
        const report<Opm::Well, WellSegment, 3> msw_data {
            "MULTI-SEGMENT WELL: SEGMENT STRUCTURE", msw_well_table, ctx
        };

        msw_data.print_header(os);

        std::size_t sub_report {0};
        const auto& segments = well.getSegments();

        for (const auto& branch : segments.branches()) {
            const auto& branch_segments = segments.branchSegments(branch);

            std::vector<WellSegment> wrapper_data;
            wrapper_data.reserve(branch_segments.size());

            std::transform(branch_segments.begin(),
                           branch_segments.end(),
                           std::back_inserter(wrapper_data),
                           [&well](const Opm::Segment& segment)
                           { return WellSegment { well, segment }; });

            const auto separator = (sub_report + 1 == segments.branches().size())
                ? '=' : '-';

            msw_data.print_data(os, wrapper_data, sub_report, separator);

            ++sub_report;
        }

        msw_data.print_footer(os, {{1, "The pressure drop multiplier is not "
                                        "implemented in opm/flow and will "
                                        "always show the default value 1.0." }});
    }

    void report_mswell_connection_data(std::ostream&    os,
                                       const Opm::Well& well,
                                       const context&   ctx)
    {
        const report<Opm::Well, SegmentConnection, 3> msw_connection {
            "MULTI-SEGMENT WELL: CONNECTION DATA", msw_connection_table, ctx
        };

        msw_connection.print_header(os);

        const auto& connections = well.getConnections();
        const auto& segments = well.getSegments();

        std::vector<SegmentConnection> wrapper_data;
        wrapper_data.reserve(connections.size());

        std::transform(connections.begin(), connections.end(),
                       std::back_inserter(wrapper_data),
                       [&well, &segments] (const Opm::Connection& connection)
                       {
                           return SegmentConnection {
                               well, connection,
                               segments.getFromSegmentNumber(connection.segment()),
                               *connection.perf_range()
                           };
                       });

        msw_connection.print_data(os, wrapper_data, 0, '=');

        msw_connection.print_footer(os, {});
    }

    void emitGroupHierarchy(const context&    ctx,
                            const std::size_t reportStep,
                            std::ostream&     os)
    {
        report_group_hierarchy_data(os, ctx, reportStep);
        report_group_levels_data(os, ctx, reportStep);
    }

    void emitWellspec(const std::vector<std::string>& changedWells,
                      const context&                  ctx,
                      const Opm::ScheduleState&       sched,
                      std::ostream&                   os)
    {
        auto changed_wells = std::vector<Opm::Well>{};
        changed_wells.reserve(changedWells.size());

        std::transform(changedWells.begin(), changedWells.end(),
                       std::back_inserter(changed_wells),
                       [&sched](const std::string& wname)
                       { return sched.wells(wname); });

        report_well_specification_data(os, changed_wells, ctx);
        report_well_connection_data(os, changed_wells, ctx);

        for (const auto& well : changed_wells) {
            if (! well.isMultiSegment()) {
                continue;
            }

            report_mswell_segment_data(os, well, ctx);
            report_mswell_connection_data(os, well, ctx);
        }
    }

    /// Emit well list report for a single well list.
    ///
    /// Contains at least the well list name.  If the well list is empty,
    /// then the only line emitted will be the one containing the well list
    /// name followed by an empty column of well names.  Otherwise, the
    /// wells on the named well list will be printed in the report sheet,
    /// with at most a specified number of wells per line.
    ///
    /// \param[in] indent Leading blanks that indent each line.
    ///
    /// \param[in] wellsPerLine Maximum number of well names printed on each
    /// line.
    ///
    /// \param[in] wlistName Well list name.
    ///
    /// \param[in] wlistWells Wells associated to the named well list.
    ///
    /// \param[in,out] Stream to which to write the well list report.
    void writeWellListWells(const std::string&                        indent,
                            const std::vector<std::string>::size_type wellsPerLine,
                            const std::string&                        wlistName,
                            const std::vector<std::string>&           wlistWells,
                            std::ostream&                             os)
    {
        const auto numRptLines = wlistWells.empty()
            ? std::vector<std::string>::size_type{1}
            : (wlistWells.size() + wellsPerLine - 1) / wellsPerLine;

        for (auto line = 0*numRptLines; line < numRptLines; ++line) {
            const auto start =          (line + 0) * wellsPerLine;
            const auto end   = std::min((line + 1) * wellsPerLine, wlistWells.size());

            os << indent
               << fmt::format(": {:<8s} :{:<100s}:\n",
                              ((line == 0) ? wlistName : ""),
                              fmt::format(" {:<8s}",
                                          fmt::join(wlistWells.begin() + start,
                                                    wlistWells.begin() + end, " ")));
        }
    }

    /// Emit well list report.
    ///
    /// Will generate a printed sheet of the form shown below detailing the
    /// contents of all current well lists.
    ///
    ///          WELL LISTS (Date)
    ///          -----------------
    ///
    ///    -------------------------------
    ///    :  LIST  :                    :
    ///    -------------------------------
    ///    :        :                    :
    ///    :  *A    : A1   A2 [---]  A10 :
    ///    :        : A11  A12           :
    ///    :        :                    :
    ///    :  *B    : B1   B2            :
    ///    :        :                    :
    ///    -------------------------------
    ///
    /// The first column ('LIST') is 10 characters wide and the well name
    /// column is 100 characters wide.  Well names are each printed in 10
    /// characters, with at most 10 well names per line.
    ///
    /// \param[in] schedule Collection of dynamic simulation objects.
    ///
    /// \param[in] reportStep Zero-based report step index for which to
    /// generate the well list report.
    ///
    /// \param[in,out] Stream to which to write the well list report.
    void emitWellLists(const Opm::Schedule& schedule,
                       const std::size_t    reportStep,
                       std::ostream&        os)
    {
        const auto indent = std::string(std::string::size_type{10}, field_padding);

        // Sheet header.
        //
        //  WELL LISTS (Date)
        //  -----------------
        //
        {
            // Upper case for "Jan" -> "JAN" &c.  The rest of the sheet is
            // upper case, so mixed case month names would look out of place.
            const auto header = ::Opm::uppercase
                (fmt::format("WELL LISTS ({:%d-%b-%Y})",
                             fmt::gmtime(schedule.simTime(reportStep))));

            os << "\n\n"
               << indent << std::string(std::string::size_type{40}, field_padding)
               << header << '\n'
               << indent << std::string(std::string::size_type{40}, field_padding)
               << std::string(header.size(), divider_character) << "\n\n";
        }

        const auto hline = std::string(std::string::size_type{113}, divider_character);
        const auto blank = fmt::format(": {0:8s} : {0:98s} :", "");

        // Column headers.
        //
        //  ----------------------------
        //  :  LIST  :                 :
        //  ----------------------------
        //  :        :                 :
        //
        // Note: First line in report sheet is intentionally left blank.
        {
            os << indent << hline << '\n'
               << indent << fmt::format(":   LIST   : {0:98s} :\n", "")
               << indent << hline << '\n'
               << indent << blank << '\n';
        }

        // Body of well list report.
        //
        // One or more report lines for each well list at the current time.
        // At most wellsPerLine well names per report line.  First (or only)
        // report line has the well list name in the 'LIST' column.  Empty
        // well lists reported as a line containing just the well list name.
        // Each individual well list ends in a blank line.
        constexpr auto wellsPerLine = std::vector<std::string>::size_type{10};

        for (const auto& [wlname, wlist] : schedule[reportStep].wlist_manager()) {
            writeWellListWells(indent, wellsPerLine, wlname, wlist.wells(), os);

            // Blank line after each well list.
            os << indent << blank << '\n';
        }

        // Final horizontal line in well list report.
        os << indent << hline << "\n\n\n";
    }
}

// ===========================================================================

void Opm::PrtFile::Reports::wellSpecification(const std::vector<std::string>& changedWells,
                                              const bool                      changedWellLists,
                                              const std::size_t               reportStep,
                                              const Schedule&                 schedule,
                                              BlockDepthCallback              blockDepth,
                                              std::ostream&                   os)
{
    const context ctx { schedule, std::move(blockDepth) };

    if (! changedWells.empty()) {
        emitWellspec(changedWells, ctx, schedule[reportStep], os);
    }

    if (changedWellLists) {
        emitWellLists(schedule, reportStep, os);
    }

    emitGroupHierarchy(ctx, reportStep, os);
}
