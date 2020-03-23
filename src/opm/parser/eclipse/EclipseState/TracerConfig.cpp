/*
  Copyright (C) 2020 Equinor

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


#include <opm/parser/eclipse/EclipseState/TracerConfig.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>

namespace Opm {

TracerConfig::TracerConfig(const Deck& deck)
{
    tracers.resize(deck.getKeyword("TRACER").size());
    for (size_t tracerIdx = 0; tracerIdx < tracers.size(); ++tracerIdx) {
        auto& tracer = tracers[tracerIdx];
        const auto& tracerRecord = deck.getKeyword("TRACER").getRecord(tracerIdx);
        tracer.name = tracerRecord.getItem("NAME").template get<std::string>(0);
        const std::string& fluidName = tracerRecord.getItem("FLUID").template get<std::string>(0);
        if (fluidName == "WAT")
            tracer.phase = Phase::WATER;
        else if (fluidName == "OIL")
            tracer.phase = Phase::OIL;
        else if (fluidName == "GAS")
            tracer.phase = Phase::GAS;
        else
            throw std::invalid_argument("Tracer: invalid fluid name " + fluidName + " for " + tracer.name);

        if (deck.hasKeyword("TBLKF"  + tracer.name)) {
            tracer.concentration = deck.getKeyword("TBLKF" + tracer.name).getRecord(0).getItem(0).getSIDoubleData();
        }  else if (deck.hasKeyword("TVDPF" + tracer.name)) {
            tracer.tvdpf.init(deck.getKeyword("TVDPF" + tracer.name).getRecord(0).getItem(0));
        } else
            throw std::runtime_error("Uninitialized tracer concentration for tracer " + tracer.name);
    }
}

TracerConfig TracerConfig::serializeObject()
{
    TracerConfig result;
    result.tracers = {{"test", Phase::OIL, {1.0}}};

    return result;
}

size_t TracerConfig::size() const {
    return this->tracers.size();
}

const std::vector<TracerConfig::TracerEntry>::const_iterator TracerConfig::begin() const {
    return this->tracers.begin();
}

const std::vector<TracerConfig::TracerEntry>::const_iterator TracerConfig::end() const {
    return this->tracers.end();
}

bool TracerConfig::operator==(const TracerConfig& other) const {
    return this->tracers == other.tracers;
}

}
