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

void writeWellStructure(const std::string& well_name,
                        const Opm::WellSegments& segments,
                        const Opm::WellConnections& connections)
{
    const std::string filename = well_name + ".gv";
    std::ofstream os(filename);

    if (!os) {
        throw std::runtime_error("Outputting well segment structure failed. Could not open '" + filename + "'.");
    }

    os << "strict digraph \"" << well_name << "\"\n{\n";
    os << "    rankdir=BT;\n";
    os << "    node [style=filled];\n";

    // Well name
    os << "    0 [label=\"" << well_name << "\""
       << ", shape=doublecircle, fillcolor=lightgrey];\n";
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

        os << "    " << id << " [label=\"Seg " << id << "\\n(Branch " << branch << ")\""
           << ", shape=" << shape << ", fillcolor=" << color << "];\n";

        // pointing to outlet segment
        assert(outlet >= 0);
        os << "    " << id << " -> " << outlet << ";\n";
    }

    // Add connections to the graph
    for (const auto& conn : connections) {
        if (conn.attachedToSegment()) {
            const int seg_id = conn.segment();
            const std::string conn_node = "conn_" + std::to_string(conn.global_index());
            os << "    " << conn_node << " [label=\"(" << conn.getI() << "," << conn.getJ() << ","
               << conn.getK() << ")\""
               << ", shape=ellipse, fillcolor=lightgreen, style=filled];\n"
               << "    " << conn_node << " -> " << seg_id << ";\n";
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
