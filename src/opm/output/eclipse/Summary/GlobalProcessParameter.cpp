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

#include <opm/output/eclipse/Summary/GlobalProcessParameter.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <cstddef>
#include <string>
#include <utility>

Opm::GlobalProcessParameter::
GlobalProcessParameter(Keyword                   keyword,
                       const UnitSystem::measure unit)
    : keyword_{ std::move(keyword.value) }
    , unit_   (unit)
{}

void
Opm::GlobalProcessParameter::update(const std::size_t    /* reportStep */,
                                    const double         /* stepSize */,
                                    const InputData&        input,
                                    const SimulatorResults& simRes,
                                    Opm::SummaryState&      st) const
{
    auto xPos = simRes.single.find(this->keyword_);
    if (xPos == simRes.single.end()) {
        return;
    }

    const auto& usys = input.es.getUnits();

    st.update(this->keyword_, usys.from_si(this->unit_, xPos->second));
}
