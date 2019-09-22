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

#include <opm/output/eclipse/Summary/BlockParameter.hpp>

#include <opm/output/eclipse/Summary/SummaryParameter.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>

#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <cstddef>
#include <map>
#include <sstream>
#include <string>
#include <utility>

namespace {
    std::string makeBlockKey(const std::string& kw, const int block)
    {
        std::ostringstream os;

        os << kw << ':' << block;

        return os.str();
    }
}

Opm::BlockParameter::BlockParameter(const int                 num,
                                    const UnitSystem::measure m,
                                    std::string               keyword)
    : num_    (num)
    , m_      (m)
    , keyword_(std::move(keyword))
    , sumKey_ (makeBlockKey(keyword_, num_))
{}

void Opm::BlockParameter::update(const std::size_t    /* reportStep */,
                                 const double         /* stepSize */,
                                 const InputData&        input,
                                 const SimulatorResults& simRes,
                                 SummaryState&           st) const
{
    auto valPos = simRes.block.find({this->keyword_, this->num_});

    if (valPos == simRes.block.end()) {
        return;
    }

    const auto& usys = input.es.getUnits();

    st.update(this->sumKey_, usys.from_si(this->m_, valPos->second));
}
