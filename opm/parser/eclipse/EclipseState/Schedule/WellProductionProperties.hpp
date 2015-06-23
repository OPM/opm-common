/*
  Copyright 2013 Statoil ASA.

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

#ifndef WELLPRODUCTIONPROPERTIES_HPP_HEADER_INCLUDED
#define WELLPRODUCTIONPROPERTIES_HPP_HEADER_INCLUDED

#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>

namespace Opm {
    class WellProductionProperties {
    public:
        double  OilRate;
        double  WaterRate;
        double  GasRate;
        double  LiquidRate;
        double  ResVRate;
        double  BHPLimit;
        double  THPLimit;
        int     VFPTableNumber;
        double  ALQValue;
        bool    predictionMode;

        WellProducer::ControlModeEnum controlMode;

        bool operator==(const WellProductionProperties& other) const;
        bool operator!=(const WellProductionProperties& other) const;
        WellProductionProperties();

        static WellProductionProperties history(DeckRecordConstPtr record);

        static WellProductionProperties prediction(DeckRecordConstPtr record);

        bool hasProductionControl(WellProducer::ControlModeEnum controlModeArg) const {
            return (m_productionControls & controlModeArg) != 0;
        }

        void dropProductionControl(WellProducer::ControlModeEnum controlModeArg) {
            if (hasProductionControl(controlModeArg))
                m_productionControls -= controlModeArg;
        }

        void addProductionControl(WellProducer::ControlModeEnum controlModeArg) {
            if (! hasProductionControl(controlModeArg))
                m_productionControls += controlModeArg;
        }

    private:
        int m_productionControls;

        WellProductionProperties(DeckRecordConstPtr record);

        void init();
    };
} // namespace Opm

#endif  // WELLPRODUCTIONPROPERTIES_HPP_HEADER_INCLUDED
