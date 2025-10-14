/*
  Copyright 2018 NORCE.

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
#include <opm/input/eclipse/Schedule/Well/WellTracerProperties.hpp>

#include <opm/input/eclipse/Deck/UDAValue.hpp>
#include "../eval_uda.hpp"

#include <string>
#include <vector>
#include <map>

namespace Opm {

    WellTracerProperties WellTracerProperties::serializationTestObject()
    {
        WellTracerProperties result;
        result.m_tracerConcentrations = {{"test", UDAValue(1.0)}, {"test2", UDAValue(2.0)}};
        result.m_udq_undefined = 3.0;

        return result;
    }

    bool WellTracerProperties::operator==(const WellTracerProperties& other) const {
        return this->m_tracerConcentrations == other.m_tracerConcentrations &&
               this->m_udq_undefined == other.m_udq_undefined;
    }

    void WellTracerProperties::setConcentration(const std::string& name, const UDAValue& concentration, double udq_undefined) {
        m_tracerConcentrations[name] = concentration;
        m_udq_undefined = udq_undefined;
    }

    double WellTracerProperties::getConcentration(const std::string& wname, const std::string& name, const SummaryState& st) const {
        auto it = m_tracerConcentrations.find(name);
        if (it == m_tracerConcentrations.end())
            return 0.0;
        return UDA::eval_well_uda(it->second, wname, st, m_udq_undefined);
    }

    bool WellTracerProperties::operator!=(const WellTracerProperties& other) const {
        return !(*this == other);
    }

}
