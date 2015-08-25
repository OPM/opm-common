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

#ifndef OPM_SIMULATION_CONFIG_HPP
#define OPM_SIMULATION_CONFIG_HPP

#include <opm/parser/eclipse/Deck/Deck.hpp>
#include <opm/parser/eclipse/EclipseState/SimulationConfig/ThresholdPressure.hpp>
#include <opm/parser/eclipse/Parser/ParseMode.hpp>

namespace Opm {

    class SimulationConfig {

    public:

        SimulationConfig(const ParseMode& parseMode , DeckConstPtr deck, std::shared_ptr<GridProperties<int>> gridProperties);

        std::shared_ptr<const ThresholdPressure> getThresholdPressure() const;


    private:

        void initThresholdPressure(const ParseMode& parseMode , DeckConstPtr deck, std::shared_ptr<GridProperties<int>> gridProperties);

        ThresholdPressureConstPtr m_ThresholdPressure;
    };



    typedef std::shared_ptr<SimulationConfig> SimulationConfigPtr;
    typedef std::shared_ptr<const SimulationConfig> SimulationConfigConstPtr;

} //namespace Opm



#endif
