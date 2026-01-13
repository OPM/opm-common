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

namespace Opm
{

void writeGroupStructure(const Schedule& schedule, const std::string& casename)
{
    std::cout << "Writing " << casename << ".gv .... ";
    std::cout.flush();
    std::ofstream os(casename + ".gv");
    os << "// This file was written by the 'wellgraph' utility from OPM.\n";
    os << "// Find the source code at github.com/OPM.\n";
    os << "// Convert output to PDF with 'dot -Tpdf " << casename << ".gv > " << casename
       << ".pdf'\n";
    os << "strict digraph \"" << casename << "\"\n{\n";
    const auto groupnames = schedule.groupNames();
    const std::size_t last = schedule.size() - 1;
    // Group -> Group relations.
    for (const auto& gn : groupnames) {
        const auto& g = schedule.getGroup(gn, last);
        const auto& children = g.groups();
        if (!children.empty()) {
            os << "    \"" << gn << "\" -> {";
            for (const auto& child : children) {
                os << " \"" << child << '"';
            }
            os << " }\n";
        }
    }
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
    std::cout << "Convert output to PDF with 'dot -Tpdf " << casename << ".gv > " << casename
              << ".pdf'\n"
              << std::endl;
}

} // end of namespace Opm
