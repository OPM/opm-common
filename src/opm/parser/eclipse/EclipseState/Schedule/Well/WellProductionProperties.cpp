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
#include <opm/parser/eclipse/Deck/UDAValue.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQActive.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellProductionProperties.hpp>

#include "../eval_uda.hpp"

namespace Opm {

    Well2::WellProductionProperties::WellProductionProperties(const std::string& name_arg) :
        name(name_arg),
        predictionMode( true )
    {}


    void Well2::WellProductionProperties::init_rates( const DeckRecord& record ) {
        this->OilRate    = record.getItem("ORAT").get<UDAValue>(0);
        this->WaterRate  = record.getItem("WRAT").get<UDAValue>(0);
        this->GasRate    = record.getItem("GRAT").get<UDAValue>(0);
    }


    void Well2::WellProductionProperties::init_history(const DeckRecord& record)
    {
        this->predictionMode = false;
        // update LiquidRate
        this->LiquidRate = UDAValue(this->WaterRate.get<double>() + this->OilRate.get<double>());

        if ( record.getItem( "BHP" ).hasValue(0) )
            this->BHPH = record.getItem("BHP").get<UDAValue>(0).get<double>();
        if ( record.getItem( "THP" ).hasValue(0) )
            this->THPH = record.getItem("THP").get<UDAValue>(0).get<double>();

        const auto& cmodeItem = record.getItem("CMODE");
        if ( cmodeItem.defaultApplied(0) ) {
            const std::string msg = "control mode can not be defaulted for keyword WCONHIST";
            throw std::invalid_argument(msg);
        }

        ProducerCMode cmode;

        if (effectiveHistoryProductionControl(this->whistctl_cmode) )
            cmode = this->whistctl_cmode;
        else
            cmode = ProducerCModeFromString( cmodeItem.getTrimmedString( 0 ) );

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
        if ( !this->hasProductionControl( ProducerCMode::BHP ) )
            this->addProductionControl( ProducerCMode::BHP );

        if (cmode == ProducerCMode::BHP)
            this->setBHPLimit(this->BHPH);

        const auto vfp_table = record.getItem("VFPTable").get< int >(0);
        if (vfp_table != 0)
            this->VFPTableNumber = vfp_table;

        auto alq_value = record.getItem("Lift").get< double >(0); //NOTE: Unit of ALQ is never touched
        if (alq_value != 0.)
            this->ALQValue = alq_value;
    }



    void Well2::WellProductionProperties::handleWCONPROD( const std::string& /* well */, const DeckRecord& record)
    {
        this->predictionMode = true;

        this->BHPLimit       = record.getItem("BHP").get<UDAValue>(0);
        this->THPLimit       = record.getItem("THP").get<UDAValue>(0);
        this->ALQValue       = record.getItem("ALQ").get< double >(0); //NOTE: Unit of ALQ is never touched
        this->VFPTableNumber = record.getItem("VFP_TABLE").get< int >(0);
        this->LiquidRate     = record.getItem("LRAT").get<UDAValue>(0);
        this->ResVRate       = record.getItem("RESV").get<UDAValue>(0);

        using mode = std::pair< const std::string, ProducerCMode >;
        static const mode modes[] = {
            { "ORAT", ProducerCMode::ORAT }, { "WRAT", ProducerCMode::WRAT }, { "GRAT", ProducerCMode::GRAT },
            { "LRAT", ProducerCMode::LRAT }, { "RESV", ProducerCMode::RESV }, { "THP", ProducerCMode::THP }
        };


        this->init_rates(record);

        for( const auto& cmode : modes ) {
            if( !record.getItem( cmode.first ).defaultApplied( 0 ) ) {

                // a zero value THP limit will not be handled as a THP limit
                if (cmode.first == "THP" && this->THPLimit.get<double>() == 0.)
                    continue;

                this->addProductionControl( cmode.second );
            }
        }

        // There is always a BHP constraint, when not specified, will use the default value
        this->addProductionControl( ProducerCMode::BHP );
        {
            const auto& cmodeItem = record.getItem("CMODE");
            if (cmodeItem.hasValue(0)) {
                auto cmode = Well2::ProducerCModeFromString( cmodeItem.getTrimmedString(0) );

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
    void Well2::WellProductionProperties::handleWCONHIST(const DeckRecord& record)
    {
        this->init_rates(record);
        this->LiquidRate.reset(0);
        this->ResVRate.reset(0);

        // when the well is switching to history matching producer from prediction mode
        // or switching from injector to producer
        // or switching from BHP control to RATE control (under history matching mode)
        // we use the defaulted BHP limit, otherwise, we use the previous BHP limit
        if (this->predictionMode)
            this->resetDefaultBHPLimit();

        if (this->controlMode == ProducerCMode::BHP)
            this->resetDefaultBHPLimit();

        this->init_history(record);
    }



void Well2::WellProductionProperties::handleWELTARG(Well2::WELTARGCMode cmode, double newValue, double siFactorG, double siFactorL, double siFactorP) {
        if (cmode == WELTARGCMode::ORAT){
            this->OilRate.assert_numeric("Can not combine UDA and WELTARG");
            this->OilRate.reset( newValue * siFactorL );
        }
        else if (cmode == WELTARGCMode::WRAT){
            this->WaterRate.assert_numeric("Can not combine UDA and WELTARG");
            this->WaterRate.reset( newValue * siFactorL );
        }
        else if (cmode == WELTARGCMode::GRAT){
            this->GasRate.assert_numeric("Can not combine UDA and WELTARG");
            this->GasRate.reset( newValue * siFactorG );
        }
        else if (cmode == WELTARGCMode::LRAT){
            this->LiquidRate.assert_numeric("Can not combine UDA and WELTARG");
            this->LiquidRate.reset( newValue * siFactorL );
        }
        else if (cmode == WELTARGCMode::RESV){
            this->ResVRate.assert_numeric("Can not combine UDA and WELTARG");
            this->ResVRate.reset( newValue * siFactorL );
        }
        else if (cmode == WELTARGCMode::BHP){
            this->BHPLimit.assert_numeric("Can not combine UDA and WELTARG");
            this->BHPLimit.reset( newValue * siFactorP );
        }
        else if (cmode == WELTARGCMode::THP){
            this->THPLimit.assert_numeric("Can not combine UDA and WELTARG");
            this->THPLimit.reset(newValue * siFactorP);
        }
        else if (cmode == WELTARGCMode::VFP)
            this->VFPTableNumber = static_cast<int> (newValue);
        else if (cmode != WELTARGCMode::GUID)
            throw std::invalid_argument("Invalid keyword (MODE) supplied");
    }



    bool Well2::WellProductionProperties::operator==(const Well2::WellProductionProperties& other) const {
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


    bool Well2::WellProductionProperties::operator!=(const Well2::WellProductionProperties& other) const {
        return !(*this == other);
    }


    std::ostream& operator<<( std::ostream& stream, const Well2::WellProductionProperties& wp ) {
        return stream
            << "Well2::WellProductionProperties { "
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
            << "WHISTCTL: "     << Well2::ProducerCMode2String(wp.whistctl_cmode)    << ", "
            << "prediction: "   << wp.predictionMode    << " }";
    }

bool Well2::WellProductionProperties::effectiveHistoryProductionControl(const Well2::ProducerCMode cmode) {
        // Note, not handling CRAT for now
        return ( (cmode == ProducerCMode::LRAT || cmode == ProducerCMode::RESV || cmode == ProducerCMode::ORAT ||
                  cmode == ProducerCMode::WRAT || cmode == ProducerCMode::GRAT || cmode == ProducerCMode::BHP) );
    }

    void Well2::WellProductionProperties::resetDefaultBHPLimit() {
        BHPLimit = UDAValue( 1. * unit::atm );
    }

    void Well2::WellProductionProperties::clearControls() {
        m_productionControls = 0;
    }

    void Well2::WellProductionProperties::setBHPLimit(const double limit) {
        BHPLimit = UDAValue( limit );
    }

    double Well2::WellProductionProperties::getBHPLimit() const {
        return BHPLimit.get<double>();
    }


    Well2::ProductionControls Well2::WellProductionProperties::controls(const SummaryState& st, double udq_undef) const {
        Well2::ProductionControls controls(this->m_productionControls);

        controls.oil_rate = UDA::eval_well_uda(this->OilRate, this->name, st, udq_undef);
        controls.water_rate = UDA::eval_well_uda(this->WaterRate, this->name, st, udq_undef);
        controls.gas_rate = UDA::eval_well_uda(this->GasRate, this->name, st, udq_undef);
        controls.liquid_rate = UDA::eval_well_uda(this->LiquidRate, this->name, st, udq_undef);
        controls.resv_rate = UDA::eval_well_uda(this->ResVRate, this->name, st, udq_undef);
        controls.bhp_limit = UDA::eval_well_uda(this->BHPLimit, this->name, st, udq_undef);
        controls.thp_limit= UDA::eval_well_uda(this->THPLimit, this->name, st, udq_undef);

        controls.bhp_history = this->BHPH;
        controls.thp_history = this->THPH;
        controls.vfp_table_number = this->VFPTableNumber;
        controls.alq_value = this->ALQValue;
        controls.cmode = this->controlMode;

        return controls;
    }

    bool Well2::WellProductionProperties::updateUDQActive(const UDQConfig& udq_config, UDQActive& active) const {
        int update_count = 0;

        update_count += active.update(udq_config, this->OilRate, this->name, UDAControl::WCONPROD_ORAT);
        update_count += active.update(udq_config, this->WaterRate, this->name, UDAControl::WCONPROD_WRAT);
        update_count += active.update(udq_config, this->GasRate, this->name, UDAControl::WCONPROD_GRAT);
        update_count += active.update(udq_config, this->LiquidRate, this->name, UDAControl::WCONPROD_LRAT);
        update_count += active.update(udq_config, this->ResVRate, this->name, UDAControl::WCONPROD_RESV);
        update_count += active.update(udq_config, this->BHPLimit, this->name, UDAControl::WCONPROD_BHP);
        update_count += active.update(udq_config, this->THPLimit, this->name, UDAControl::WCONPROD_THP);

        return (update_count > 0);
    }


} // namespace Opm
