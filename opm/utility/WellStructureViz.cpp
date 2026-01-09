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


#include <opm/utility/WellStructureViz.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>
#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>

#include <cassert>
#include <fstream>
#include <string>

#include <fmt/format.h>

namespace Opm
{
void writeWellStructure(const std::string& well_name,
                        const WellSegments& segments,
                        const WellConnections& connections)
{
    const std::string filename = well_name + ".gv";
    std::ofstream os(filename);

    if (!os) {
        throw std::runtime_error(fmt::format("Outputting well segment structure failed. Could not open '{}'.", filename));
    }

    os << fmt::format("// Convert output to PDF or PNG with 'dot -Tpdf {0}.gv -o {0}.pdf' or 'dot -Tpng {0}.gv -o {0}.png'\n", well_name);

    os << fmt::format("strict digraph \"{}\"\n{{\n", well_name);
    os << "    rankdir=BT;\n";
    os << "    node [style=filled];\n";

    // Well name
    os << fmt::format("    0 [label=\"{}\", shape=doublecircle, fillcolor=lightgrey];\n", well_name);
    for (const auto& segment : segments) {
        const int id = segment.segmentNumber();
        const int outlet = segment.outletSegment();
        const int branch = segment.branchNumber();

        // style for regular segments
        std::string shape = "box";
        std::string color = "white";

        // different colors for different type of segments
        if (segment.isValve()) {
            shape = "diamond";
            color = "lightblue";
        } else if (segment.isSpiralICD()) {
            shape = "box";
            color = "gold";
        } else if (segment.isAICD()) {
            shape = "box";
            color = "orange";
        }
        else if (branch == 1) {
            // main branch / branch 1
            color = "ivory";
        }

        os << fmt::format("    {} [label=\"Seg {}\\n(Branch {})\", shape={}, fillcolor={}];\n",
                  id, id, branch, shape, color);

        // pointing to outlet segment
        assert(outlet >= 0);
        os << fmt::format("    {} -> {};\n", id, outlet);
    }

    // Add connections to the graph
    for (const auto& conn : connections) {
        if (conn.attachedToSegment()) {
            const int seg_id = conn.segment();
            const std::string conn_node = fmt::format("conn_{}", conn.global_index());
            os << fmt::format("    {} [label=\"({},{},{})\", shape=ellipse, fillcolor=lightgreen, style=filled];\n",
                  conn_node, conn.getI() + 1, conn.getJ() + 1, conn.getK() + 1);
            os << fmt::format("    {} -> {};\n", conn_node, seg_id);
        }
    }

    os << "    {\n";
    os << "        rank=sink;\n";
    os << "        Legend [shape=none, margin=0, label=<\n";
    os << "            <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"4\">\n";
    os << "                <TR><TD COLSPAN=\"2\"><B>Legend</B></TD></TR>\n";
    os << "                <TR><TD BGCOLOR=\"white\">Regular Segments</TD></TR>\n";
    os << "                <TR><TD BGCOLOR=\"ivory\">Main Branch</TD></TR>\n";
    os << "                <TR><TD BGCOLOR=\"gold\">SICD</TD></TR>\n";
    os << "                <TR><TD BGCOLOR=\"orange\">AICD</TD></TR>\n";
    os << "                <TR><TD BGCOLOR=\"lightblue\">Valve</TD></TR>\n";
    os << "                <TR><TD BGCOLOR=\"lightgreen\">Connections</TD></TR>\n";
    os << "            </TABLE>\n";
    os << "        >];\n";
    os << "    }\n";

    os << "}\n";
}

} // namespace Opm
