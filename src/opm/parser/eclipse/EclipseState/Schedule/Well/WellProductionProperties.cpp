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

#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>

#include "WellProductionProperties.hpp"


namespace Opm {

    WellProductionProperties::
    WellProductionProperties() : predictionMode( true )
    {}


    void WellProductionProperties::init_rates( const DeckRecord& record ) {
        this->OilRate    = record.getItem("ORAT").getSIDouble(0);
        this->WaterRate  = record.getItem("WRAT").getSIDouble(0);
        this->GasRate    = record.getItem("GRAT").getSIDouble(0);
    }


    void WellProductionProperties::init_history(const DeckRecord& record)
    {
        this->predictionMode = false;
        // update LiquidRate
        this->LiquidRate = this->WaterRate + this->OilRate;

        if ( record.getItem( "BHP" ).hasValue(0) )
            this->BHPH = record.getItem("BHP").getSIDouble(0);
        if ( record.getItem( "THP" ).hasValue(0) )
            this->THPH = record.getItem("THP").getSIDouble(0);

        const auto& cmodeItem = record.getItem("CMODE");
        if ( cmodeItem.defaultApplied(0) ) {
            const std::string msg = "control mode can not be defaulted for keyword WCONHIST";
            throw std::invalid_argument(msg);
        }

        namespace wp = WellProducer;
        auto cmode = wp::CMODE_UNDEFINED;

        if (effectiveHistoryProductionControl(this->whistctl_cmode) )
            cmode = this->whistctl_cmode;
        else
            cmode = wp::ControlModeFromString( cmodeItem.getTrimmedString( 0 ) );

        // clearing the existing targets/limits
        clearControls();

        if (effectiveHistoryProductionControl(cmode)) {
            this->addProductionControl( cmode );
            this->controlMode = cmode;
        } else {
            const std::string cmode_string = cmodeItem.getTrimmedString( 0 );
            const std::string msg = "unsupported control mode " + cmode_string + " for WCONHIST";
            throw std::invalid_argument(msg);
        }

        // always have a BHP control/limit, while the limit value needs to be determined
        // the control mode added above can be a BHP control or a type of RATE control
        if ( !this->hasProductionControl( wp::BHP ) )
            this->addProductionControl( wp::BHP );

        if (cmode == wp::BHP)
            this->setBHPLimit(this->BHPH);

        const auto vfp_table = record.getItem("VFPTable").get< int >(0);
        if (vfp_table != 0)
            this->VFPTableNumber = vfp_table;

        auto alq_value = record.getItem("Lift").get< double >(0); //NOTE: Unit of ALQ is never touched
        if (alq_value != 0.)
            this->ALQValue = alq_value;
    }



    void WellProductionProperties::handleWCONPROD( const DeckRecord& record )
    {
        this->predictionMode = true;

        this->BHPLimit       = record.getItem("BHP"      ).getSIDouble(0);
        this->THPLimit       = record.getItem("THP"      ).getSIDouble(0);
        this->ALQValue       = record.getItem("ALQ"      ).get< double >(0); //NOTE: Unit of ALQ is never touched
        this->VFPTableNumber = record.getItem("VFP_TABLE").get< int >(0);
        this->LiquidRate     = record.getItem("LRAT").getSIDouble(0);
        this->ResVRate       = record.getItem("RESV").getSIDouble(0);

        namespace wp = WellProducer;
        using mode = std::pair< const std::string, wp::ControlModeEnum >;
        static const mode modes[] = {
            { "ORAT", wp::ORAT }, { "WRAT", wp::WRAT }, { "GRAT", wp::GRAT },
            { "LRAT", wp::LRAT }, { "RESV", wp::RESV }, { "THP", wp::THP }
        };


        this->init_rates(record);

        for( const auto& cmode : modes ) {
            if( !record.getItem( cmode.first ).defaultApplied( 0 ) ) {

                // a zero value THP limit will not be handled as a THP limit
                if (cmode.first == "THP" && this->THPLimit == 0.)
                    continue;

                this->addProductionControl( cmode.second );
            }
        }

        // There is always a BHP constraint, when not specified, will use the default value
        this->addProductionControl( wp::BHP );
        {
            const auto& cmodeItem = record.getItem("CMODE");
            if (cmodeItem.hasValue(0)) {
                const WellProducer::ControlModeEnum cmode = WellProducer::ControlModeFromString( cmodeItem.getTrimmedString(0) );

                if (this->hasProductionControl( cmode ))
                    this->controlMode = cmode;
                else
                    throw std::invalid_argument("Trying to set CMODE to: " + cmodeItem.getTrimmedString(0) + " - no value has been specified for this control");
            }
        }
    }

    /*
      This is now purely "history" constructor - i.e. the record should
      originate from the WCONHIST keyword. Predictions are handled with the
      default constructor and the handleWCONPROD() method.
    */
    void WellProductionProperties::handleWCONHIST(const DeckRecord& record)
    {
        this->init_rates(record);
        this->LiquidRate = 0;
        this->ResVRate = 0;

        // when the well is switching to history matching producer from prediction mode
        // or switching from injector to producer
        // or switching from BHP control to RATE control (under history matching mode)
        // we use the defaulted BHP limit, otherwise, we use the previous BHP limit
        if (this->predictionMode)
            this->resetDefaultBHPLimit();

        if (this->controlMode == WellProducer::BHP)
            this->resetDefaultBHPLimit();

        this->init_history(record);
    }



  void WellProductionProperties::handleWELTARG(WellTarget::ControlModeEnum cmode, double newValue, double siFactorG, double siFactorL, double siFactorP) {
        if (cmode == WellTarget::ORAT){
            this->OilRate = newValue * siFactorL;
        }
        else if (cmode == WellTarget::WRAT){
            this->WaterRate = newValue * siFactorL;
        }
        else if (cmode == WellTarget::GRAT){
            this->GasRate = newValue * siFactorG;
        }
        else if (cmode == WellTarget::LRAT){
            this->LiquidRate = newValue * siFactorL;
        }
        else if (cmode == WellTarget::RESV){
            this->ResVRate = newValue * siFactorL;
        }
        else if (cmode == WellTarget::BHP){
            this->BHPLimit = newValue * siFactorP;
        }
        else if (cmode == WellTarget::THP){
            this->THPLimit = newValue * siFactorP;
        }
        else if (cmode == WellTarget::VFP){
            this->VFPTableNumber = static_cast<int> (newValue);
        }
        else if (cmode != WellTarget::GUID){
            throw std::invalid_argument("Invalid keyword (MODE) supplied");
        }
    }



    bool WellProductionProperties::operator==(const WellProductionProperties& other) const {
        return OilRate              == other.OilRate
            && WaterRate            == other.WaterRate
            && GasRate              == other.GasRate
            && LiquidRate           == other.LiquidRate
            && ResVRate             == other.ResVRate
            && BHPLimit             == other.BHPLimit
            && THPLimit             == other.THPLimit
            && BHPH                 == other.BHPH
            && THPH                 == other.THPH
            && VFPTableNumber       == other.VFPTableNumber
            && controlMode          == other.controlMode
            && m_productionControls == other.m_productionControls
            && whistctl_cmode       == other.whistctl_cmode
            && this->predictionMode == other.predictionMode;
    }


    bool WellProductionProperties::operator!=(const WellProductionProperties& other) const {
        return !(*this == other);
    }


    std::ostream& operator<<( std::ostream& stream, const WellProductionProperties& wp ) {
        return stream
            << "WellProductionProperties { "
            << "oil rate: "     << wp.OilRate           << ", "
            << "water rate: "   << wp.WaterRate         << ", "
            << "gas rate: "     << wp.GasRate           << ", "
            << "liquid rate: "  << wp.LiquidRate        << ", "
            << "ResV rate: "    << wp.ResVRate          << ", "
            << "BHP limit: "    << wp.BHPLimit          << ", "
            << "THP limit: "    << wp.THPLimit          << ", "
            << "BHPH: "         << wp.BHPH              << ", "
            << "THPH: "         << wp.THPH              << ", "
            << "VFP table: "    << wp.VFPTableNumber    << ", "
            << "ALQ: "          << wp.ALQValue          << ", "
            << "WHISTCTL: "     << wp.whistctl_cmode    << ", "
            << "prediction: "   << wp.predictionMode    << " }";
    }

    bool WellProductionProperties::effectiveHistoryProductionControl(const WellProducer::ControlModeEnum cmode) {
        // Note, not handling CRAT for now
        namespace wp = WellProducer;
        return ( (cmode == wp::LRAT || cmode == wp::RESV || cmode == wp::ORAT ||
                  cmode == wp::WRAT || cmode == wp::GRAT || cmode == wp::BHP) );
    }

    void WellProductionProperties::resetDefaultBHPLimit() {
        BHPLimit = 1. * unit::atm;
    }

    void WellProductionProperties::clearControls() {
        m_productionControls = 0;
    }

    void WellProductionProperties::setBHPLimit(const double limit) {
        BHPLimit = limit;
    }

    double WellProductionProperties::getBHPLimit() const {
        return BHPLimit;
    }


    ProductionControls WellProductionProperties::controls(const SummaryState& st) const {
        ProductionControls controls(this->m_productionControls);

        controls.oil_rate = this->OilRate;
        controls.water_rate = this->WaterRate;
        controls.gas_rate = this->GasRate;
        controls.liquid_rate = this->LiquidRate;
        controls.resv_rate = this->ResVRate;
        controls.bhp_limit = this->BHPLimit;
        controls.thp_limit= this->THPLimit;
        controls.bhp_history = this->BHPH;
        controls.thp_history = this->THPH;
        controls.vfp_table_number = this->VFPTableNumber;
        controls.alq_value = this->ALQValue;
        controls.cmode = this->controlMode;

        return controls;
    }


} // namespace Opm
