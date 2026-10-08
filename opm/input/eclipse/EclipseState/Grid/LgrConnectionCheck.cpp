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

#include <opm/input/eclipse/EclipseState/Grid/LgrConnectionCheck.hpp>

#include <opm/common/ErrorMacros.hpp>
#include <opm/input/eclipse/EclipseState/EclipseState.hpp>
#include <opm/input/eclipse/EclipseState/Grid/LgrCollection.hpp>
#include <opm/input/eclipse/EclipseState/Grid/NNC.hpp>
#include <opm/input/eclipse/EclipseState/Grid/TransMult.hpp>

#include <fmt/format.h>

#include <cstddef>
#include <stdexcept>
#include <string>

namespace Opm {

std::vector<LgrCellBox> lgrCellBoxes(const LgrCollection& lgrs)
{
    std::vector<LgrCellBox> boxes;
    for (std::size_t i = 0; i < lgrs.size(); ++i) {
        const auto& lgr = lgrs.getLgr(i);
        if (lgr.PARENT_NAME() != "GLOBAL") {
            continue;
        }
        boxes.push_back({ { lgr.I1(), lgr.J1(), lgr.K1() },
                          { lgr.I2() + 1, lgr.J2() + 1, lgr.K2() + 1 } });
    }
    return boxes;
}

void refuseDeckConnectionsInsideBoxes(const EclipseState& eclState,
                                      const std::array<int,3>& cartDims,
                                      const std::vector<LgrCellBox>& boxes)
{
    if (boxes.empty()) {
        return;
    }
    auto ijk = [&cartDims](std::size_t cart) {
        const int nx = cartDims[0], ny = cartDims[1];
        const int i = static_cast<int>(cart % nx);
        const int j = static_cast<int>((cart / nx) % ny);
        const int k = static_cast<int>(cart / (static_cast<std::size_t>(nx) * ny));
        return std::array<int,3>{ i, j, k };
    };
    auto covered = [&](std::size_t cart) {
        const auto c = ijk(cart);
        for (const auto& b : boxes) {
            bool in = true;
            for (int d = 0; d < 3; ++d) {
                in = in && (c[d] >= b.start[d]) && (c[d] < b.end[d]);
            }
            if (in) {
                return true;
            }
        }
        return false;
    };
    auto ijkString = [&](std::size_t cart) {
        const auto c = ijk(cart);
        return fmt::format("({},{},{})", c[0] + 1, c[1] + 1, c[2] + 1);
    };

    const auto& nnc = eclState.getInputNNC();
    for (const auto& entry : nnc.input()) {
        if (covered(entry.cell1) || covered(entry.cell2)) {
            OPM_THROW(std::invalid_argument,
                      fmt::format("An explicit connection -- the NNC keyword or a "
                                  "numerical aquifer -- names cell {} or {}, which "
                                  "a refinement box covers. Its transmissibility is "
                                  "an absolute one and there is no rule yet for "
                                  "dividing it among the faces the connection became, "
                                  "so it would be applied to one arbitrary pair of "
                                  "children or to none. Move the box off the "
                                  "connection.",
                                  ijkString(entry.cell1), ijkString(entry.cell2)));
        }
    }

    const auto& transMult = eclState.getTransMult();
    for (const auto* list : { &nnc.input(), &nnc.editr() }) {
        for (const auto& entry : *list) {
            const auto mult = transMult.getRegionMultiplierNNC(entry.cell1, entry.cell2);
            if ((mult != 1.0) && (covered(entry.cell1) || covered(entry.cell2))) {
                OPM_THROW(std::invalid_argument,
                          fmt::format("MULTREGT gives the connection {} -- {} a "
                                      "multiplier of {}, and a refinement box covers "
                                      "one of those cells. The connection became "
                                      "several faces there and the multiplier reaches "
                                      "at most one of them, so the region boundary "
                                      "would be left open. Move the box off the "
                                      "connection, or set the multiplier to 1.",
                                      ijkString(entry.cell1), ijkString(entry.cell2), mult));
            }
        }
    }
}

} // namespace Opm
