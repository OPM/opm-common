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

#include <opm/input/eclipse/EclipseState/SummaryConfig/RegionVariableSupport.hpp>

#include <opm/output/data/RegionVariableMapping.hpp>

#include <opm/input/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>

namespace {
    void populateOilEfficiencyVariables(const Opm::SummaryConfig&         sumcfg,
                                        Opm::data::RegionVariableMapping& regVarMap)
    {
        // FOEW, ROEW, ROEW_<REG>
        const auto oew_kws = sumcfg.keywords(R"(*OEW*)");

        if (oew_kws.empty()) {
            // No *OEW* vectors requested.  Nothing to do.
            return;
        }

        using RegSet = Opm::data::RegionVariableMapping::RegionSet;

        regVarMap.add(Opm::data::RegionVariableMapping::Variable { "ConnOPT" },
                      /* is_cumulative = */ true);

        for (const auto& oew_kw : oew_kws) {
            if (oew_kw.category() == Opm::SummaryConfigNode::Category::Region) {
                regVarMap.add(RegSet { oew_kw.fip_region() });
            }
            else {
                // Not a region level vector.  Typically FOEW.  Ensure that
                // we have FIPNUM for this case.
                regVarMap.add(RegSet { "FIPNUM" });
            }
        }
    }
} // Anonymous namespace

// ===========================================================================
// Public interface below separator
// ===========================================================================

void Opm::populateRegVarMapping(const SummaryConfig&         sumcfg,
                                data::RegionVariableMapping& regVarMap)
{
    populateOilEfficiencyVariables(sumcfg, regVarMap);
}
