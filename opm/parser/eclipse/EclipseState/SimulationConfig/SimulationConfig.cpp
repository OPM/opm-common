/*
  Copyright 2015 Statoil ASA.

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


#include <opm/parser/eclipse/EclipseState/SimulationConfig/SimulationConfig.hpp>
#include <opm/parser/eclipse/Deck/Deck.hpp>


namespace Opm {

    SimulationConfig::SimulationConfig(DeckConstPtr deck, std::shared_ptr<GridProperties<int>> gridProperties) {
        initThresholdPressure(deck, gridProperties);
    }


    void SimulationConfig::initThresholdPressure(DeckConstPtr deck, std::shared_ptr<GridProperties<int>> gridProperties) {
        mThresholdPressure = std::make_shared<const ThresholdPressure>(deck, gridProperties);
    }


    const std::vector<double>& SimulationConfig::getThresholdPressureTable() const {
        return mThresholdPressure->getThresholdPressureTable();
    }

} //namespace Opm
