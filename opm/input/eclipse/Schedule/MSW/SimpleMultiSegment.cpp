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

#include <opm/input/eclipse/Schedule/MSW/SimpleMultiSegment.hpp>

#include <opm/input/eclipse/Schedule/MSW/Segment.hpp>
#include <opm/input/eclipse/Schedule/MSW/WellSegments.hpp>
#include <opm/input/eclipse/Schedule/Well/Connection.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace Opm {

void makeSimpleMultiSegmentWell(Well& well,
                                const UnitSystem& unit_system,
                                const double tubing_diameter)
{
    if (well.isMultiSegment()) {
        return;
    }
    const auto& conns = well.getConnections();
    if (conns.size() == 0) {
        return;
    }

    const double invalid = Segment::invalidValue();
    const double ref_depth = well.getRefDepth();
    const double area = std::numbers::pi * tubing_diameter * tubing_diameter / 4.0;

    // Top segment (number 1, branch 1, no outlet): absolute (data-ready) values.
    // Depth is the well reference depth (the BHP datum). Its measured-depth is
    // set equal to that depth, matching how a WELSEGS top segment is specified
    // (measured-depth tracks TVD along the tubing). Diameter/roughness/area are
    // left invalid, as a WELSEGS top segment is (the well node, not a flow
    // segment). The volume must be a real value though: segment processing only
    // fills in volumes for segments 1..N (not the top), and the well model reads
    // the top-segment volume into its accumulation term, so an invalid sentinel
    // there makes the well diverge. Use a small nominal wellbore volume.
    std::vector<Segment> top_segment{
        Segment(/*segment_number*/ 1, /*branch*/ 1, /*outlet_segment*/ 0,
                /*depth*/ ref_depth, /*length*/ ref_depth,
                /*internal_diameter*/ invalid, /*roughness*/ invalid,
                /*cross_area*/ invalid, /*volume*/ area,
                /*data_ready*/ true, /*x*/ 0.0, /*y*/ 0.0)
    };
    WellSegments segments(WellSegments::CompPressureDrop::H__, top_segment);

    // One segment per connection, forming a linear tubing (segment c+2 outlets
    // to segment c+1). Depth is the connection centre depth (this drives the
    // hydrostatic head). The measured-depth ("length") accumulates by at least
    // the absolute depth change of each step, so it tracks TVD, is strictly
    // increasing (every tube piece has positive length), and each segment's
    // along-tube length is >= its vertical drop (a physical inclination).
    constexpr double MIN_SEG_LEN = 0.1; // [m]
    std::vector<std::pair<double, double>> lengths_and_depths;
    lengths_and_depths.reserve(conns.size());
    double prev_depth = ref_depth;
    double md = ref_depth; // top-segment measured depth (== ref_depth above)
    for (std::size_t c = 0; c < conns.size(); ++c) {
        const double depth = conns[c].depth();
        md += std::max(std::abs(depth - prev_depth), MIN_SEG_LEN);
        lengths_and_depths.emplace_back(md, depth);
        prev_depth = depth;
    }
    segments.addWellSegmentsFromLengthsAndDepths(well.name(), lengths_and_depths,
                                                 tubing_diameter, unit_system);
    well.updateSegments(std::make_shared<WellSegments>(std::move(segments)));

    // Attach each connection to its own segment (connection c -> segment c+2,
    // in COMPDAT order). Mirrors the COMPSEGS connection-to-segment wiring,
    // including the perforation measured-depth range [outlet MD, segment MD] so
    // the multisegment well can place the perforation along its segment.
    auto new_conns = conns; // intentional copy
    double perf_start = ref_depth; // top-segment measured depth
    for (std::size_t c = 0; c < new_conns.size(); ++c) {
        const auto& cc = new_conns[c];
        const double perf_end = lengths_and_depths[c].first; // this segment's MD
        new_conns.getFromIJK(cc.getI(), cc.getJ(), cc.getK())
            .updateSegment(/*segment_number*/ static_cast<int>(c) + 2,
                           /*center_depth*/ cc.depth(),
                           // Preserve the connection's existing thermal length
                           // (set from COMPDAT); upstream added this argument to
                           // updateSegment. Only used by thermal runs.
                           /*thermal_length*/ cc.thermalLength(),
                           /*compseg_insert_index*/ c,
                           /*perf_range*/ std::make_pair(perf_start, perf_end));
        perf_start = perf_end;
    }
    well.updateConnections(std::make_shared<WellConnections>(std::move(new_conns)),
                           /*force*/ true);
}

} // namespace Opm
