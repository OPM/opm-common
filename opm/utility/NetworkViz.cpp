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

#include <opm/utility/NetworkViz.hpp>


#include <opm/input/eclipse/Schedule/Network/ExtNetwork.hpp>
#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <fstream>
#include <iostream>
#include <string>

namespace Opm
{

void writeNetworkStructure(const Schedule& schedule, const std::string& casename)
{
    std::cout << "Writing " << casename << ".gv .... ";  std::cout.flush();
    std::ofstream os(casename + ".gv");
    os << "// This file was written by the 'networkgraph2' utility from OPM.\n";
    os << "// Find the source code at github.com/OPM.\n";
    os << "// Convert output to PDF with 'dot -Tpdf " << casename << ".gv > " << casename << ".pdf'\n";
    os << "strict digraph \"" << casename << "\"\n{\n";
    const std::size_t last = schedule.size() - 1;
    // by default, it is the last report step, while it can be any report step
    const auto& network = schedule[last].network();

    for (const auto& branch : network.branches()) {
        os << "    \"" << branch->uptree_node() << "\" -> \"" << branch->downtree_node() << "\"";
        if (branch->vfp_table().has_value()) {
            os << " [label=\"" << branch->vfp_table().value() << "\"]";
        }
        os << ";\n";
    }

    // Highlight root nodes
    for (const auto& root : network.roots()) {
        const auto& root_node = root.get();
        os << "    \"" << root_node.name() << "\" [shape=doubleoctagon";
        if (root_node.terminal_pressure().has_value()) {
            if (root_node.terminal_pressure().has_value()) {
                // TODO: we should be able to get the Unit conversion here
                os << ", label=\"" << root_node.name() << " : " << root_node.terminal_pressure().value()/1.e5 << "bars\"";
            }
        }
        os << "];\n";
    }

    // Highlight leaf nodes
    for (const auto& leaf : network.leaf_nodes()) {
        os << "    \"" << leaf << "\" [shape=oval, style=filled, fillcolor=orange];\n";
    }

    // Group -> Well relations.
    os << "    node [shape=box]\n";
    for (const auto& leaf : network.leaf_nodes()) {
        const auto& leaf_group = schedule.getGroup(leaf, last);
        const auto& wells = leaf_group.wells();
        if (!wells.empty()) {
            os << "    \"" << leaf << "\" -> {";
            for (const auto& child : wells) {
                os << " \"" << child << '"';
            }
            os << " }\n";
        }
        if (!wells.empty()) {
            for (const auto& child : wells) {
                const auto& w = schedule.getWell(child, last);
                os << "    \"" << w.name() << '"';
                if (w.isProducer() && w.isInjector()) {
                    os << " [color=purple]\n";
                } else if (w.isProducer()) {
                    os << " [color=red]\n";
                } else {
                    os << " [color=blue]\n";
                }
            }
        }
    }
    os << "}\n";

    std::cout << "complete." << std::endl;
    std::cout << "Convert output to PDF with 'dot -Tpdf " << casename << ".gv > " << casename << ".pdf'\n" << std::endl;
}

} // namespace Opm
