/*
  Copyright 2016 Statoil ASA.

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
#include <iostream>
#include <string>
#include <vector>

#include <opm/parser/eclipse/Units/Units.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/S.hpp>

#include "WellInjectionProperties.hpp"
#include "injection.hpp"

namespace Opm {

    WellInjectionProperties::WellInjectionProperties(const std::string& wname)
      : name(wname),
        injectorType(WellInjector::WATER),
        controlMode(WellInjector::CMODE_UNDEFINED) {
        surfaceInjectionRate=0.0;
        reservoirInjectionRate=0.0;
        temperature=
            Metric::TemperatureOffset
            + ParserKeywords::STCOND::TEMPERATURE::defaultValue;
        BHPLimit=0.0;
        THPLimit=0.0;
        BHPH=0.0;
        THPH=0.0;
        VFPTableNumber=0;
        predictionMode=true;
        injectionControls=0;
    }


    void WellInjectionProperties::handleWCONINJE(const DeckRecord& record, bool availableForGroupControl, const std::string& well_name, const UnitSystem& unit_system) {
        this->injectorType = WellInjector::TypeFromString( record.getItem("TYPE").getTrimmedString(0) );
        this->predictionMode = true;

        if (!record.getItem("RATE").defaultApplied(0)) {
            this->surfaceInjectionRate = injection::rateToSI(record.getItem("RATE").get< double >(0) , this->injectorType, unit_system);
            this->addInjectionControl(WellInjector::RATE);
        } else
            this->dropInjectionControl(WellInjector::RATE);


        if (!record.getItem("RESV").defaultApplied(0)) {
            this->reservoirInjectionRate = record.getItem("RESV").getSIDouble(0);
            this->addInjectionControl(WellInjector::RESV);
        } else
            this->dropInjectionControl(WellInjector::RESV);


        if (!record.getItem("THP").defaultApplied(0)) {
            this->THPLimit       = record.getItem("THP").getSIDouble(0);
            this->addInjectionControl(WellInjector::THP);
        } else
            this->dropInjectionControl(WellInjector::THP);

        this->VFPTableNumber = record.getItem("VFP_TABLE").get< int >(0);

        /*
          There is a sensible default BHP limit defined, so the BHPLimit can be
          safely set unconditionally, and we make BHP limit as a constraint based
          on that default value. It is not easy to infer from the manual, while the
          current behavoir agrees with the behovir of Eclipse when BHPLimit is not
          specified while employed during group control.
        */
        this->setBHPLimit(record.getItem("BHP").getSIDouble(0));
        // BHP control should always be there.
        this->addInjectionControl(WellInjector::BHP);

        if (availableForGroupControl)
            this->addInjectionControl(WellInjector::GRUP);
        else
            this->dropInjectionControl(WellInjector::GRUP);
        {
            const std::string& cmodeString = record.getItem("CMODE").getTrimmedString(0);
            WellInjector::ControlModeEnum controlModeArg = WellInjector::ControlModeFromString( cmodeString );
            if (this->hasInjectionControl( controlModeArg))
                this->controlMode = controlModeArg;
            else {
                throw std::invalid_argument("Tried to set invalid control: " + cmodeString + " for well: " + well_name);
            }
        }
    }



    void WellInjectionProperties::handleWELTARG(WellTarget::ControlModeEnum cmode, double newValue, double siFactorG, double siFactorL, double siFactorP) {
        if (cmode == WellTarget::BHP){
            this->BHPLimit = newValue * siFactorP;
        }
        else if (cmode == WellTarget::ORAT){
            if(this->injectorType == WellInjector::TypeEnum::OIL){
                this->surfaceInjectionRate = newValue * siFactorL;
            }else{
                std::invalid_argument("Well type must be OIL to set the oil rate");
            }
        }
        else if (cmode == WellTarget::WRAT){
            if(this->injectorType == WellInjector::TypeEnum::WATER){
                this->surfaceInjectionRate = newValue * siFactorL;
            }else{
                std::invalid_argument("Well type must be WATER to set the water rate");
            }
        }
        else if (cmode == WellTarget::GRAT){
            if(this->injectorType == WellInjector::TypeEnum::GAS){
                this->surfaceInjectionRate = newValue * siFactorG;
            }else{
                std::invalid_argument("Well type must be GAS to set the gas rate");
            }
        }
        else if (cmode == WellTarget::THP){
            this->THPLimit = newValue * siFactorP;
        }
        else if (cmode == WellTarget::VFP){
            this->VFPTableNumber = static_cast<int> (newValue);
        }
        else if (cmode == WellTarget::RESV){
            this->reservoirInjectionRate = newValue * siFactorL;
        }
        else if (cmode != WellTarget::GUID){
            throw std::invalid_argument("Invalid keyword (MODE) supplied");
        }
    }


    void WellInjectionProperties::handleWCONINJH(const DeckRecord& record, bool is_producer, const std::string& well_name, const UnitSystem& unit_system) {
        // convert injection rates to SI
        const auto& typeItem = record.getItem("TYPE");
        if (typeItem.defaultApplied(0)) {
            const std::string msg = "Injection type can not be defaulted for keyword WCONINJH";
            throw std::invalid_argument(msg);
        }
        this->injectorType = WellInjector::TypeFromString( typeItem.getTrimmedString(0));
        double injectionRate = record.getItem("RATE").get< double >(0);
        injectionRate = injection::rateToSI(injectionRate, this->injectorType, unit_system);

        if (!record.getItem("RATE").defaultApplied(0))
            this->surfaceInjectionRate = injectionRate;
        if ( record.getItem( "BHP" ).hasValue(0) )
            this->BHPH = record.getItem("BHP").getSIDouble(0);
        if ( record.getItem( "THP" ).hasValue(0) )
            this->THPH = record.getItem("THP").getSIDouble(0);

        const std::string& cmodeString = record.getItem("CMODE").getTrimmedString(0);
        const WellInjector::ControlModeEnum newControlMode = WellInjector::ControlModeFromString( cmodeString );

        if ( !(newControlMode == WellInjector::RATE || newControlMode == WellInjector::BHP) ) {
            const std::string msg = "Only RATE and BHP control are allowed for WCONINJH for well " + well_name;
            throw std::invalid_argument(msg);
        }

        // when well is under BHP control, we use its historical BHP value as BHP limit
        if (newControlMode == WellInjector::BHP) {
            this->setBHPLimit(this->BHPH);
        } else {
            const bool switching_from_producer = is_producer;
            const bool switching_from_prediction = this->predictionMode;
            const bool switching_from_BHP_control = (this->controlMode == WellInjector::BHP);
            if (switching_from_prediction ||
                switching_from_BHP_control ||
                switching_from_producer) {
                this->resetDefaultHistoricalBHPLimit();
            }
            // otherwise, we keep its previous BHP limit
        }

        this->addInjectionControl(WellInjector::BHP);
        this->addInjectionControl(newControlMode);
        this->controlMode = newControlMode;
        this->predictionMode = false;

        const int VFPTableNumberArg = record.getItem("VFP_TABLE").get< int >(0);
        if (VFPTableNumberArg > 0) {
            this->VFPTableNumber = VFPTableNumberArg;
        }
    }

    bool WellInjectionProperties::operator==(const WellInjectionProperties& other) const {
        if ((surfaceInjectionRate == other.surfaceInjectionRate) &&
            (reservoirInjectionRate == other.reservoirInjectionRate) &&
            (temperature == other.temperature) &&
            (BHPLimit == other.BHPLimit) &&
            (THPLimit == other.THPLimit) &&
            (BHPH == other.BHPH) &&
            (THPH == other.THPH) &&
            (VFPTableNumber == other.VFPTableNumber) &&
            (predictionMode == other.predictionMode) &&
            (injectionControls == other.injectionControls) &&
            (injectorType == other.injectorType) &&
            (controlMode == other.controlMode))
            return true;
        else
            return false;
    }

    bool WellInjectionProperties::operator!=(const WellInjectionProperties& other) const {
        return !(*this == other);
    }

    void WellInjectionProperties::resetDefaultHistoricalBHPLimit() {
        // this default BHP value is from simulation result,
        // without finding any related document
        BHPLimit = 6891.2 * unit::barsa;
    }

    void WellInjectionProperties::setBHPLimit(const double limit) {
        BHPLimit = limit;
    }

    std::ostream& operator<<( std::ostream& stream,
                              const WellInjectionProperties& wp ) {
        return stream
            << "WellInjectionProperties { "
            << "surfacerate: "      << wp.surfaceInjectionRate << ", "
            << "reservoir rate "    << wp.reservoirInjectionRate << ", "
            << "temperature: "      << wp.temperature << ", "
            << "BHP limit: "        << wp.BHPLimit << ", "
            << "THP limit: "        << wp.THPLimit << ", "
            << "BHPH: "             << wp.BHPH << ", "
            << "THPH: "             << wp.THPH << ", "
            << "VFP table: "        << wp.VFPTableNumber << ", "
            << "prediction mode: "  << wp.predictionMode << ", "
            << "injection ctrl: "   << wp.injectionControls << ", "
            << "injector type: "    << wp.injectorType << ", "
            << "control mode: "     << wp.controlMode << " }";
    }


    InjectionControls WellInjectionProperties::controls(const SummaryState&) const {
        InjectionControls controls(this->injectionControls);

        controls.surface_rate = this->surfaceInjectionRate;
        controls.reservoir_rate = this->reservoirInjectionRate;
        controls.bhp_limit = this->BHPLimit;
        controls.thp_limit = this->THPLimit;
        controls.temperature = this->temperature;
        controls.injector_type = this->injectorType;
        controls.cmode = this->controlMode;
        controls.vfp_table_number = this->VFPTableNumber;
        controls.injector_type = this->injectorType;

        return controls;
    }
}
