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

#include <iosfwd>
#include <memory>

#include <opm/parser/eclipse/Deck/UDAValue.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/ProductionControls.hpp>

namespace Opm {

    class DeckRecord;
    class SummaryState;

    class WellProductionProperties {
    public:
        // the rates serve as limits under prediction mode
        // while they are observed rates under historical mode
        std::string name;
        UDAValue  OilRate;
        UDAValue  WaterRate;
        UDAValue  GasRate;
        UDAValue  LiquidRate;
        UDAValue  ResVRate;
        // BHP and THP limit
        UDAValue  BHPLimit;
        UDAValue  THPLimit;
        // historical BHP and THP under historical mode
        double  BHPH        = 0.0;
        double  THPH        = 0.0;
        int     VFPTableNumber = 0;
        double  ALQValue    = 0.0;
        bool    predictionMode = false;
        WellProducer::ControlModeEnum controlMode = WellProducer::CMODE_UNDEFINED;
        WellProducer::ControlModeEnum whistctl_cmode = WellProducer::CMODE_UNDEFINED;

        bool operator==(const WellProductionProperties& other) const;
        bool operator!=(const WellProductionProperties& other) const;
        WellProductionProperties(const std::string& name_arg);

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

        // this is used to check whether the specified control mode is an effective history matching production mode
        static bool effectiveHistoryProductionControl(const WellProducer::ControlModeEnum cmode);
        void handleWCONPROD( const DeckRecord& record);
        void handleWCONHIST( const DeckRecord& record);
        void handleWELTARG(WellTarget::ControlModeEnum cmode, double newValue, double siFactorG, double siFactorL, double siFactorP);
        void resetDefaultBHPLimit();
        void clearControls();

        ProductionControls controls(const SummaryState& st, double udq_default) const;
    private:
        int m_productionControls = 0;
        void init_rates( const DeckRecord& record );

        void init_history(const DeckRecord& record);

        WellProductionProperties(const DeckRecord& record);

        void setBHPLimit(const double limit);

        double getBHPLimit() const;
    };

    std::ostream& operator<<( std::ostream&, const WellProductionProperties& );

} // namespace Opm

#endif  // WELLPRODUCTIONPROPERTIES_HPP_HEADER_INCLUDED
