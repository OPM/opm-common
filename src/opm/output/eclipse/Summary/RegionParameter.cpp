/*
  Copyright (c) 2019 Equinor ASA

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

#include <opm/output/eclipse/Summary/RegionParameter.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well2.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <opm/output/eclipse/RegionCache.hpp>

#include <cstddef>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {
    std::string makeRegionKey(const std::string& kw, const int regID)
    {
        std::ostringstream key;

        // ROPT:17
        key << kw << ':' << regID;

        return key.str();
    }
}

Opm::RegionParameter::RegionParameter(const int                 regionID,
                                      Keyword                   keyword,
                                      const UnitSystem::measure unit)
    : keyword_ (std::move(keyword.value))
    , regionID_(regionID)
    , unit_    (unit)
    , sumKey_  (makeRegionKey(keyword_, regionID_))
{}

void
Opm::RegionParameter::update(const std::size_t    /* reportStep */,
                             const double         /* stepSize */,
                             const InputData&        input,
                             const SimulatorResults& simRes,
                             Opm::SummaryState&      st) const
{
    if (this->regionID_ < 1) {
        // Region result never available in regions whose ID is less than one.
        return;
    }

    auto xrPos = simRes.region.find(this->keyword_);
    if (xrPos == simRes.region.end()) {
        // Region result not available for this keyword at this time.
        return;
    }

    using Ix = std::vector<double>::size_type;
    const auto ix = static_cast<Ix>(this->regionID_ - 1);

    if (ix >= xrPos->second.size()) {
        // Region result not available for this keyword at this time.
        return;
    }

    const auto& usys = input.es.getUnits();

    st.update(this->sumKey_, usys.from_si(this->unit_, xrPos->second[ix]));
}
