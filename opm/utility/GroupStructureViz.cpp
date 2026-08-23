/*
  Copyright 2026 Equinor ASA.

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

#include <opm/utility/GroupStructureViz.hpp>

#include <opm/input/eclipse/Schedule/Group/Group.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>

#include <cstddef>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/format.h>

namespace
{

enum class VizWellType
{
    PRODUCER,
    INJECTOR,
};

struct WellInfo
{
    std::string name{};
    VizWellType type;
};

struct GroupChildren
{
    std::vector<std::size_t> groups{};
    std::vector<std::size_t> wells{};
};

struct GroupHierarchy
{
    std::vector<std::string> groups{};
    std::vector<WellInfo> wells{};
    std::vector<GroupChildren> group_children{};
};

VizWellType getWellType(const bool isProducer, const bool isInjector)
{
    if (isProducer == isInjector) {
        // Both producer and injector or neither.
        throw std::invalid_argument {"Invalid well type"};
    }

    if (isProducer) {
        return VizWellType::PRODUCER;
    }
    else {
        return VizWellType::INJECTOR;
    }
}

std::string_view wellTypeToColor(const VizWellType type)
{
    switch (type) {
    case VizWellType::PRODUCER: return "red";
    case VizWellType::INJECTOR: return "blue";
    default:
        throw std::invalid_argument {"Invalid well type"};
    }
}

GroupHierarchy buildGroupHierarchy(const Opm::Schedule& schedule)
{
    if (schedule.size() == 0) {
        throw std::invalid_argument{"Cannot build GroupHierarchy from an empty Schedule"};
    }

    GroupHierarchy h { .groups = schedule.groupNames() };
    auto groupIndex = std::unordered_map<std::string, std::size_t> {};
    for (auto i = std::size_t{0}; const auto& gn : h.groups) {
        groupIndex.insert_or_assign(gn, i++);
    }

    auto wellIndex = std::unordered_map<std::string, std::size_t> {};
    for (const auto& wname : schedule.wellNames()) {
        const auto& w = schedule.getWellatEnd(wname);

        wellIndex.insert_or_assign(wname, h.wells.size());

        const auto wtype = getWellType(w.isProducer(), w.isInjector());

        h.wells.emplace_back(wname, wtype);
    }

    for (const std::size_t last = schedule.size() - 1;
         const auto& gn : h.groups)
    {
        const auto& g = schedule.getGroup(gn, last);

        auto& children = h.group_children.emplace_back();

        for (const auto& child_group : g.groups()) {
            children.groups.push_back(groupIndex.at(child_group));
        }

        for (const auto& child_well : g.wells()) {
            children.wells.push_back(wellIndex.at(child_well));
        }
    }

    return h;
}

void writeWellGroupRelations(const GroupHierarchy& h, const std::string& casename)
{
    const auto fname = casename + "_well_groups.gv";
    std::ofstream os(fname);

    if (!os) {
        throw std::runtime_error {
            fmt::format("Writing the well-group relations for case "
                        "{0} failed. Could not open '{1}'.", casename, fname)
        };
    }

    std::cout << "Writing " << fname << " .... ";
    std::cout.flush();

    os << "// This file was written using utility function 'writeWellGroupRelations' from OPM.\n";
    os << "// Find the source code at github.com/OPM.\n";
    os << "// Convert output to PDF with 'dot -Tpdf " << fname << " -o "
       << casename << "_well_groups.pdf'\n";
    os << "strict digraph \"" << casename << "_well_groups\"\n{\n";

    os << "    node [shape=box, style=normal, fillcolor=white];\n";

    for (auto gidx = std::size_t{0}; gidx < h.groups.size(); ++gidx) {
        if (h.group_children[gidx].wells.empty()) {
            continue;
        }

        {
            const auto& group = h.groups[gidx];

            os << "    subgraph \"cluster_wells_" << group << "\" {\n";
            os << "        label = < <b>Group: " << group << "</b> >;\n";
            os << "        color = lightgrey;\n";
        }

        for (auto previousWell = std::string{};
            const auto& widx : h.group_children[gidx].wells)
        {
            const auto& well = h.wells[widx];
            const auto& w_name = well.name;

            os << "        \"" << w_name
               << "\" [color=" << wellTypeToColor(well.type)
               << ", fillcolor=white, style=filled];\n";

            if (!previousWell.empty()) {
                os << "        \""
                   << previousWell
                   << "\" -> \"" << w_name
                   << "\" [style=invis];\n";
            }

            previousWell = w_name;
        }

        os << "    }\n";
    }

    os << "}\n";

    std::cout << "complete." << std::endl;
    std::cout << "Convert output to PDF with 'dot -Tpdf " << fname << " -o "
              << casename << "_well_groups.pdf'\n\n";
}

void writeGroupGroupStructure(const GroupHierarchy& h,
                              std::ostream&         os)
{
    for (std::size_t gidx = 0; gidx < h.groups.size(); ++gidx) {
        if (h.group_children[gidx].groups.empty()) {
            continue;
        }

        os << "    \"" << h.groups[gidx] << "\" -> {";

        for (const auto& child_group_idx : h.group_children[gidx].groups) {
            const auto& child = h.group_children[child_group_idx];

            const auto isNodeGroup = !child.groups.empty();
            const auto isWellGroup = !child.wells.empty();

            if (!isNodeGroup && isWellGroup) {
                // Leaf groups are drawn with filled orange style.
                os << "\n    \"" << h.groups[child_group_idx]
                   << "\" [style=filled, fillcolor=orange];";
            }

            os << " \"" << h.groups[child_group_idx] << '"';
        }

        os << " }\n";
    }
}

void writeGroupWellStructure(const GroupHierarchy& h,
                             std::ostream&         os)
{
    // Group -> Well relations.
    os << "    node [shape=box]\n";

    for (std::size_t gidx = 0; gidx < h.groups.size(); ++gidx) {
        if (h.group_children[gidx].wells.empty()) {
            continue;
        }

        os << "    \"" << h.groups[gidx] << "\" -> {";
        for (const auto& child_well_idx : h.group_children[gidx].wells) {
            os << " \"" << h.wells[child_well_idx].name << '"';
        }

        os << " }\n";
    }

    // Color wells by injector or producer.
    for (const auto& well : h.wells) {
        os << "    \"" << well.name
           << "\" [color="
           << wellTypeToColor(well.type) << "]\n";
    }
}

void writeGroupStructure(const GroupHierarchy& h,
                         const std::string&    casename,
                         const bool            separateWellGroups)
{
    // file 1: group structure (group -> group)
    // if separateWellGroups == false, also group -> well relations.
    const auto fname = separateWellGroups
        ? casename + "_group_structure.gv"
        : casename + ".gv";

    std::ofstream os(fname);
    if (!os) {
        throw std::runtime_error {
            fmt::format("Writing the group structure for case {0} failed. "
                        "Could not open '{1}'.", casename, fname)
        };
    }

    std::cout << "Writing " << fname << " .... ";
    std::cout.flush();

    const auto* suffix = separateWellGroups ? "_group_structure" : "";

    os << "// This file was written using utility function 'writeGroupStructure' from OPM.\n"
       << "// Find the source code at github.com/OPM.\n"
       << "// Convert output to PDF with 'dot -Tpdf "
       << fname << " -o " << casename << suffix << ".pdf'\n";

    os << "strict digraph \"" << casename << "_groups\"\n{\n";

    writeGroupGroupStructure(h, os);

    if (!separateWellGroups) {
        writeGroupWellStructure(h, os);
    }

    os << "}\n";

    std::cout << "complete." << std::endl
              << "Convert output to PDF with 'dot -Tpdf "
              << fname << " -o " << casename << suffix << ".pdf'\n\n";
}

void writeWellGroupGraphImpl(const GroupHierarchy& h,
                             const std::string&    casename,
                             const bool            separateWellGroups)
{
    writeGroupStructure(h, casename, separateWellGroups);

    if (separateWellGroups) {
        writeWellGroupRelations(h, casename);
    }
}

} // anonymous namespace

void Opm::writeWellGroupGraph(const Schedule&    schedule,
                              const std::string& casename,
                              const bool         separateWellGroups)
{
    writeWellGroupGraphImpl(buildGroupHierarchy(schedule),
                            casename, separateWellGroups);
}
