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

#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <fstream>
#include <iostream>
#include <string>

#include <fmt/format.h>

namespace Opm
{

namespace
{

void writeWellGroupRelations(const Schedule& schedule,
                             const std::string& casename)
{
    std::string fname = casename + "_well_groups.gv";
    std::cout << "Writing " << fname << " .... ";
    std::cout.flush();
    std::ofstream os(fname);

    if (!os) {
        throw std::runtime_error(fmt::format(
            "Writing the well-group relations for case {0} failed. Could not open '{0}_well_groups.gv'.",
            casename
        ));
    }

    os << "// This file was written using utility function 'writeGroupStructure' from OPM.\n";
    os << "// Find the source code at github.com/OPM.\n";
    os << "// Convert output to PDF with 'dot -Tpdf " << fname << " -o "
       << casename << "_well_groups.pdf'\n";
    os << "strict digraph \"" << casename << "_well_groups\"\n{\n";

    os << "    node [shape=box, style=normal, fillcolor=white];\n";

    const auto& groupnames = schedule.groupNames();
    const std::size_t last = schedule.size() - 1;

    for (const auto& gn : groupnames) {
        const auto& g = schedule.getGroup(gn, last);
        const auto& wells = g.wells();

        if (!wells.empty()) {
            os << "    subgraph \"cluster_wells_" << gn << "\" {\n";
            os << "        label = < <b>Group: " << gn << "</b> >;\n";
            os << "        color = lightgrey;\n";

            std::string previousWell;
            for (const auto& w_name : wells) {
                const auto& w = schedule.getWell(w_name, last);

                std::string color =
                    (w.isProducer() && w.isInjector()) ? "purple" :
                    (w.isProducer() ? "red" : "blue");

                os << "        \"" << w_name
                   << "\" [color=" << color
                   << ", fillcolor=white, style=filled];\n";

                if (!previousWell.empty()) {
                    os << "        \"" << previousWell
                       << "\" -> \"" << w_name
                       << "\" [style=invis];\n";
                }
                previousWell = w_name;
            }
            os << "    }\n";
        }
    }

    os << "}\n";
    std::cout << "complete." << std::endl;
    std::cout << "Convert output to PDF with 'dot -Tpdf " << fname << " -o "
              << casename << "_well_groups.pdf'\n\n";
}

void writeGroupStructure(const Schedule& schedule, const std::string& casename, const bool separateWellGroups)
{
    const auto& groupnames = schedule.groupNames();
    const std::size_t last = schedule.size() - 1;

    // file 1: group structure (group -> group)
    // if separateWellGroups == false, also group -> well relations.
    {
        const std::string fname
            = separateWellGroups ? casename + "_group_structure.gv" : casename + ".gv";
        std::cout << "Writing " << fname << " .... ";
        std::cout.flush();
        std::ofstream os(fname);

        if (!os) {
            throw std::runtime_error(fmt::format("Writing the group structure for case {0} failed. "
                                                 "Could not open {}.", fname));
        }

        os << "// This file was written using utility function 'writeGroupStructure' from OPM.\n";
        os << "// Find the source code at github.com/OPM.\n";
        os << "// Convert output to PDF with 'dot -Tpdf " << fname << " -o " << casename
           << "_group_structure.pdf'\n";
        os << "strict digraph \"" << casename << "_groups\"\n{\n";

        // Group -> Group relations.
        for (const auto& gn : groupnames) {
            const auto& g = schedule.getGroup(gn, last);
            const auto& children = g.groups();
            if (!children.empty()) {
                os << "    \"" << gn << "\" -> {";
                for (const auto& child : children) {
                    const auto& child_group = schedule.getGroup(child, last);
                    const auto& grand_children = child_group.groups();
                    if (grand_children.empty() && !child_group.wells().empty()) {
                        // Leaf groups are drawn with filled orange style.
                        os << "\n    \"" << child << "\" [style=filled, fillcolor=orange];";
                    }
                    os << " \"" << child << '"';
                }
                os << " }\n";
            }
        }

        if (!separateWellGroups) {
            // Group -> Well relations.
            os << "    node [shape=box]\n";
            for (const auto& gn : groupnames) {
                const auto& g = schedule.getGroup(gn, last);
                const auto& children = g.wells();
                if (!children.empty()) {
                    os << "    \"" << gn << "\" -> {";
                    for (const auto& child : children) {
                        os << " \"" << child << '"';
                    }
                    os << " }\n";
                }
            }
            // Color wells by injector or producer.
            for (const auto& w : schedule.getWellsatEnd()) {
                os << "    \"" << w.name() << '"';
                if (w.isProducer() && w.isInjector()) {
                    os << " [color=purple]\n";
                } else if (w.isProducer()) {
                    os << " [color=red]\n";
                } else {
                    os << " [color=blue]\n";
                }
            }
            os << "}\n";
            std::cout << "complete." << std::endl;
            std::cout << "Convert output to PDF with 'dot -Tpdf " << fname << " -o " << casename
                      << ".pdf'\n\n";
        } else {
            os << "}\n";
            std::cout << "complete." << std::endl;
            std::cout << "Convert output to PDF with 'dot -Tpdf " << fname << " -o " << casename
                      << "_group_structure.pdf'\n\n";
        }
    }
}

} // anonymous namespace

void writeWellGroupGraph(const Schedule& schedule, const std::string& casename, const bool separateWellGroups)
{
    writeGroupStructure(schedule, casename, separateWellGroups);
    if (separateWellGroups) {
        writeWellGroupRelations(schedule, casename);
    }
}

} // end of namespace Opm
