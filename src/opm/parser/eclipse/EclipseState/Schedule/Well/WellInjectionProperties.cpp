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
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQActive.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellInjectionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well2.hpp>

#include "injection.hpp"
#include "../eval_uda.hpp"

namespace Opm {


    Well2::WellInjectionProperties::WellInjectionProperties(const std::string& wname)
        : name(wname),
          injectorType(InjectorType::WATER),
          controlMode(InjectorCMode::CMODE_UNDEFINED)
    {
        temperature=
            Metric::TemperatureOffset
            + ParserKeywords::STCOND::TEMPERATURE::defaultValue;
        BHPH=0.0;
        THPH=0.0;
        VFPTableNumber=0;
        predictionMode=true;
        injectionControls=0;
    }


    void Well2::WellInjectionProperties::handleWCONINJE(const DeckRecord& record, bool availableForGroupControl, const std::string& well_name) {
        this->injectorType = Well2::InjectorTypeFromString( record.getItem("TYPE").getTrimmedString(0) );
        this->predictionMode = true;

        if (!record.getItem("RATE").defaultApplied(0)) {
            this->surfaceInjectionRate = record.getItem("RATE").get<UDAValue>(0);
            this->addInjectionControl(InjectorCMode::RATE);
        } else
            this->dropInjectionControl(InjectorCMode::RATE);


        if (!record.getItem("RESV").defaultApplied(0)) {
            this->reservoirInjectionRate = record.getItem("RESV").get<UDAValue>(0);
            this->addInjectionControl(InjectorCMode::RESV);
        } else
            this->dropInjectionControl(InjectorCMode::RESV);


        if (!record.getItem("THP").defaultApplied(0)) {
            this->THPLimit       = record.getItem("THP").get<UDAValue>(0);
            this->addInjectionControl(InjectorCMode::THP);
        } else
            this->dropInjectionControl(InjectorCMode::THP);

        this->VFPTableNumber = record.getItem("VFP_TABLE").get< int >(0);

        /*
          There is a sensible default BHP limit defined, so the BHPLimit can be
          safely set unconditionally, and we make BHP limit as a constraint based
          on that default value. It is not easy to infer from the manual, while the
          current behavoir agrees with the behovir of Eclipse when BHPLimit is not
          specified while employed during group control.
        */
        this->setBHPLimit(record.getItem("BHP").get<UDAValue>(0).get<double>());
        // BHP control should always be there.
        this->addInjectionControl(InjectorCMode::BHP);

        if (availableForGroupControl)
            this->addInjectionControl(InjectorCMode::GRUP);
        else
            this->dropInjectionControl(InjectorCMode::GRUP);
        {
            const std::string& cmodeString = record.getItem("CMODE").getTrimmedString(0);
            InjectorCMode controlModeArg = InjectorCModeFromString( cmodeString );
            if (this->hasInjectionControl( controlModeArg))
                this->controlMode = controlModeArg;
            else {
                throw std::invalid_argument("Tried to set invalid control: " + cmodeString + " for well: " + well_name);
            }
        }
    }



    void Well2::WellInjectionProperties::handleWELTARG(WELTARGCMode cmode, double newValue, double siFactorG, double siFactorL, double siFactorP) {
        if (cmode == Well2::WELTARGCMode::BHP){
            this->BHPLimit.reset( newValue * siFactorP );
        }
        else if (cmode == WELTARGCMode::ORAT){
            if(this->injectorType == Well2::InjectorType::OIL){
                this->surfaceInjectionRate.reset( newValue * siFactorL );
            }else{
                std::invalid_argument("Well type must be OIL to set the oil rate");
            }
        }
        else if (cmode == WELTARGCMode::WRAT){
            if(this->injectorType == Well2::InjectorType::WATER)
                this->surfaceInjectionRate.reset( newValue * siFactorL );
            else
                std::invalid_argument("Well type must be WATER to set the water rate");
        }
        else if (cmode == WELTARGCMode::GRAT){
            if(this->injectorType == Well2::InjectorType::GAS){
                this->surfaceInjectionRate.reset( newValue * siFactorG );
            }else{
                std::invalid_argument("Well type must be GAS to set the gas rate");
            }
        }
        else if (cmode == WELTARGCMode::THP){
            this->THPLimit.reset( newValue * siFactorP );
        }
        else if (cmode == WELTARGCMode::VFP){
            this->VFPTableNumber = static_cast<int> (newValue);
        }
        else if (cmode == WELTARGCMode::RESV){
            this->reservoirInjectionRate.reset( newValue * siFactorL );
        }
        else if (cmode != WELTARGCMode::GUID){
            throw std::invalid_argument("Invalid keyword (MODE) supplied");
        }
    }


    void Well2::WellInjectionProperties::handleWCONINJH(const DeckRecord& record, bool is_producer, const std::string& well_name) {
        // convert injection rates to SI
        const auto& typeItem = record.getItem("TYPE");
        if (typeItem.defaultApplied(0)) {
            const std::string msg = "Injection type can not be defaulted for keyword WCONINJH";
            throw std::invalid_argument(msg);
        }
        this->injectorType = Well2::InjectorTypeFromString( typeItem.getTrimmedString(0));

        if (!record.getItem("RATE").defaultApplied(0)) {
            double injectionRate = record.getItem("RATE").get<double>(0);
            this->surfaceInjectionRate.reset( injectionRate );
        }
        if ( record.getItem( "BHP" ).hasValue(0) )
            this->BHPH = record.getItem("BHP").getSIDouble(0);
        if ( record.getItem( "THP" ).hasValue(0) )
            this->THPH = record.getItem("THP").getSIDouble(0);

        const std::string& cmodeString = record.getItem("CMODE").getTrimmedString(0);
        const InjectorCMode newControlMode = InjectorCModeFromString( cmodeString );

        if ( !(newControlMode == InjectorCMode::RATE || newControlMode == InjectorCMode::BHP) ) {
            const std::string msg = "Only RATE and BHP control are allowed for WCONINJH for well " + well_name;
            throw std::invalid_argument(msg);
        }

        // when well is under BHP control, we use its historical BHP value as BHP limit
        if (newControlMode == InjectorCMode::BHP) {
            this->setBHPLimit(this->BHPH);
        } else {
            const bool switching_from_producer = is_producer;
            const bool switching_from_prediction = this->predictionMode;
            const bool switching_from_BHP_control = (this->controlMode == InjectorCMode::BHP);
            if (switching_from_prediction ||
                switching_from_BHP_control ||
                switching_from_producer) {
                this->resetDefaultHistoricalBHPLimit();
            }
            // otherwise, we keep its previous BHP limit
        }

        this->addInjectionControl(InjectorCMode::BHP);
        this->addInjectionControl(newControlMode);
        this->controlMode = newControlMode;
        this->predictionMode = false;

        const int VFPTableNumberArg = record.getItem("VFP_TABLE").get< int >(0);
        if (VFPTableNumberArg > 0) {
            this->VFPTableNumber = VFPTableNumberArg;
        }
    }

    bool Well2::WellInjectionProperties::operator==(const Well2::WellInjectionProperties& other) const {
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

    bool Well2::WellInjectionProperties::operator!=(const Well2::WellInjectionProperties& other) const {
        return !(*this == other);
    }

    void Well2::WellInjectionProperties::resetDefaultHistoricalBHPLimit() {
        // this default BHP value is from simulation result,
        // without finding any related document
        BHPLimit.reset( 6891.2 * unit::barsa );
    }

    void Well2::WellInjectionProperties::setBHPLimit(const double limit) {
        BHPLimit.reset( limit );
    }

    std::ostream& operator<<( std::ostream& stream,
                              const Well2::WellInjectionProperties& wp ) {
        return stream
            << "Well2::WellInjectionProperties { "
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
            << "injector type: "    << Well2::InjectorType2String(wp.injectorType) << ", "
            << "control mode: "     << Well2::InjectorCMode2String(wp.controlMode) << " }";
    }


Well2::InjectionControls Well2::WellInjectionProperties::controls(const UnitSystem& unit_sys, const SummaryState& st, double udq_def) const {
        InjectionControls controls(this->injectionControls);

        controls.surface_rate = UDA::eval_well_uda_rate(this->surfaceInjectionRate, this->name, st, udq_def, this->injectorType, unit_sys);
        controls.reservoir_rate = UDA::eval_well_uda(this->reservoirInjectionRate, this->name, st, udq_def);
        controls.bhp_limit = UDA::eval_well_uda(this->BHPLimit, this->name, st, udq_def);
        controls.thp_limit = UDA::eval_well_uda(this->THPLimit, this->name, st, udq_def);

        controls.temperature = this->temperature;
        controls.injector_type = this->injectorType;
        controls.cmode = this->controlMode;
        controls.vfp_table_number = this->VFPTableNumber;
        controls.injector_type = this->injectorType;

        return controls;
    }

    bool Well2::WellInjectionProperties::updateUDQActive(const UDQConfig& udq_config, UDQActive& active) const {
        int update_count = 0;

        update_count += active.update(udq_config, this->surfaceInjectionRate, this->name, UDAControl::WCONINJE_RATE);
        update_count += active.update(udq_config, this->reservoirInjectionRate, this->name, UDAControl::WCONINJE_RESV);
        update_count += active.update(udq_config, this->BHPLimit, this->name, UDAControl::WCONINJE_BHP);
        update_count += active.update(udq_config, this->THPLimit, this->name, UDAControl::WCONINJE_THP);

        return (update_count > 0);
    }

}
