/*
  Copyright 2020 Statoil ASA.

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

#include "WellPropertiesKeywordHandlers.hpp"

#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQConfig.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>
#include <opm/input/eclipse/Schedule/Well/WellBrineProperties.hpp>
#include <opm/input/eclipse/Schedule/Well/WDFAC.hpp>
#include <opm/input/eclipse/Schedule/Well/WellEconProductionLimits.hpp>
#include <opm/input/eclipse/Schedule/Well/WellFoamProperties.hpp>
#include <opm/input/eclipse/Schedule/Well/WellMICPProperties.hpp>
#include <opm/input/eclipse/Schedule/Well/WellPolymerProperties.hpp>
#include <opm/input/eclipse/Schedule/Well/WellTracerProperties.hpp>
#include <opm/input/eclipse/Schedule/Well/WVFPDP.hpp>
#include <opm/input/eclipse/Schedule/Well/WVFPEXP.hpp>

#include <opm/input/eclipse/Deck/DeckKeyword.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

#include "../HandlerContext.hpp"

#include <algorithm>
#include <memory>
#include <utility>
#include <string>
#include <vector>

#include <fmt/format.h>

namespace Opm {

namespace {

void handleWDFAC(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const auto wellNamePattern = record.getItem<ParserKeywords::WDFAC::WELL>().getTrimmedString(0);

        const auto well_names = handlerContext.wellNames(wellNamePattern, true);
        if (well_names.empty()) {
            handlerContext.invalidNamePattern(wellNamePattern);
        }

        for (const auto& well_name : well_names) {
            auto well = handlerContext.state().wells.get(well_name);
            auto wdfac = std::make_shared<WDFAC>(well.getWDFAC());
            wdfac->updateWDFAC(record);
            wdfac->updateTotalCF(well.getConnections());

            if (well.updateWDFAC(std::move(wdfac))) {
                handlerContext.state().wells.update(std::move(well));

                handlerContext.affected_well(well_name);
                handlerContext.record_well_structure_change();

                handlerContext.state().events()
                    .addEvent(ScheduleEvents::COMPLETION_CHANGE);

                handlerContext.state().wellgroup_events()
                    .addEvent(well_name, ScheduleEvents::COMPLETION_CHANGE);
            }
        }
    }
}

void handleWDFACCOR(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const auto wellNamePattern = record.getItem("WELLNAME").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, true);
        if (well_names.empty()) {
            handlerContext.invalidNamePattern(wellNamePattern);
        }

        for (const auto& well_name : well_names) {
            auto well = handlerContext.state().wells.get(well_name);
            auto conns = std::make_shared<WellConnections>(well.getConnections());

            auto wdfac = std::make_shared<WDFAC>(well.getWDFAC());
            wdfac->updateWDFACCOR(record);

            conns->applyDFactorCorrelation(handlerContext.grid, *wdfac);
            const auto updateConns =
                well.updateConnections(std::move(conns), handlerContext.grid);

            if (well.updateWDFAC(std::move(wdfac)) || updateConns) {
                handlerContext.state().wells.update(std::move(well));

                handlerContext.affected_well(well_name);
                handlerContext.record_well_structure_change();

                if (updateConns) {
                    handlerContext.state().events()
                        .addEvent(ScheduleEvents::COMPLETION_CHANGE);

                    handlerContext.state().wellgroup_events()
                        .addEvent(well_name, ScheduleEvents::COMPLETION_CHANGE);
                }
            }
        }
    }
}

void handleWECON(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);

        for (const auto& well_name : well_names) {
            auto well2 = handlerContext.state().wells.get( well_name );
            auto econ_limits = std::make_shared<WellEconProductionLimits>( record );
            if (well2.updateEconLimits(econ_limits))
                handlerContext.state().wells.update( std::move(well2) );
        }
    }
}

void handleWEFAC(HandlerContext& handlerContext)
{
    using Kw = ParserKeywords::WEFAC;

    for (const auto& record : handlerContext.keyword) {
        const auto wellNamePattern = record.getItem<Kw::WELLNAME>().getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern);

        if (well_names.empty()) {
            handlerContext.invalidNamePattern(wellNamePattern);
        }

        const auto efficiencyFactor = record.getItem<Kw::EFFICIENCY_FACTOR>().get<double>(0);
        const bool useEfficiencyInNetwork =
            DeckItem::to_bool(record.getItem<Kw::USE_WEFAC_IN_NETWORK>().getTrimmedString(0));

        for (const auto& well_name : well_names) {
            auto well2 = handlerContext.state().wells.get(well_name);

            if (well2.updateEfficiencyFactor(efficiencyFactor, useEfficiencyInNetwork)) {
                handlerContext.state().wells.update(std::move(well2));

                handlerContext.affected_well(well_name);
                handlerContext.record_well_structure_change();

                handlerContext.state().events()
                    .addEvent(ScheduleEvents::WELLGROUP_EFFICIENCY_UPDATE);

                handlerContext.state().wellgroup_events()
                    .addEvent(well_name, ScheduleEvents::WELLGROUP_EFFICIENCY_UPDATE);
            }
        }
    }
}

void handleWELPIRuntime(HandlerContext& handlerContext)
{
    using WELL_NAME = ParserKeywords::WELPI::WELL_NAME;
    using PI        = ParserKeywords::WELPI::STEADY_STATE_PRODUCTIVITY_OR_INJECTIVITY_INDEX_VALUE;

    for (const auto& record : handlerContext.keyword) {
        const auto well_names = handlerContext
            .wellNames(record.getItem<WELL_NAME>().getTrimmedString(0), false);

        const auto targetPI = record.getItem<PI>().get<double>(0);

        for (const auto& well_name : well_names) {
            auto new_well = handlerContext.state().wells.get(well_name);

            const auto scalingFactor = new_well.convertDeckPI(targetPI)
                / handlerContext.getWellPI(well_name);

            new_well.updateWellProductivityIndex();

            // Note: Must have a separate 'scalingApplicable' object for
            // each well.
            auto scalingApplicable = std::vector<bool>{};
            new_well.applyWellProdIndexScaling(scalingFactor, scalingApplicable);

            handlerContext.state().wells.update(std::move(new_well));
            handlerContext.state().target_wellpi
                .insert_or_assign(well_name, targetPI);

            handlerContext.welpi_well(well_name);
        }
    }
}

void handleWELPI(HandlerContext& handlerContext)
{
    if (handlerContext.action_mode) {
        handleWELPIRuntime(handlerContext);
        return;
    }

    // Keyword structure
    //
    //   WELPI
    //     W1   123.45 /
    //     W2*  456.78 /
    //     *P   111.222 /
    //     **X* 333.444 /
    //   /
    //
    // Interpretation of productivity index (item 2) depends on well's
    // preferred phase.
    using WELL_NAME = ParserKeywords::WELPI::WELL_NAME;
    using PI = ParserKeywords::WELPI::STEADY_STATE_PRODUCTIVITY_OR_INJECTIVITY_INDEX_VALUE;

    for (const auto& record : handlerContext.keyword) {
        const auto wellNamePattern = record.getItem<WELL_NAME>().getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern);

        const auto rawProdIndex = record.getItem<PI>().get<double>(0);
        for (const auto& well_name : well_names) {
            auto well2 = handlerContext.state().wells.get(well_name);

            // Note: Need to ensure we have an independent copy of
            // well's connections because
            // Well::updateWellProductivityIndex() implicitly mutates
            // internal state in the WellConnections class.
            auto connections = std::make_shared<WellConnections>(well2.getConnections());
            well2.updateConnections(std::move(connections), true);
            if (well2.updateWellProductivityIndex()) {
                handlerContext.state().wells.update(std::move(well2));
            }

            handlerContext.state().wellgroup_events()
                .addEvent(well_name, ScheduleEvents::WELL_PRODUCTIVITY_INDEX);

            handlerContext.state().target_wellpi
                .insert_or_assign(well_name, rawProdIndex);
        }
    }

    handlerContext.state().events()
        .addEvent(ScheduleEvents::WELL_PRODUCTIVITY_INDEX);
}

void handleWFOAM(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);

        for (const auto& well_name : well_names) {
            auto well2 = handlerContext.state().wells.get(well_name);
            auto foam_properties = std::make_shared<WellFoamProperties>(well2.getFoamProperties());
            foam_properties->handleWFOAM(record);
            if (well2.updateFoamProperties(foam_properties)) {
                handlerContext.state().wells.update( std::move(well2) );
            }
        }
    }
}

void handleWINJCLN(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem<ParserKeywords::WINJCLN::WELL_NAME>().getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);
        for (const auto& well_name: well_names) {
            auto well = handlerContext.state().wells(well_name);
            well.handleWINJCLN(record, handlerContext.keyword.location());
            handlerContext.state().wells.update(std::move(well));
        }
    }
}

void handleWINJDAM(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem<ParserKeywords::WINJDAM::WELL_NAME>().getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, true);

        for (const auto& well_name : well_names) {
            auto well = handlerContext.state().wells(well_name);
            if (well.handleWINJDAM(record, handlerContext.keyword.location())) {
                handlerContext.state().wells.update( std::move(well) );
            }
        }
    }
}

void handleWINJFCNC(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem<ParserKeywords::WINJFCNC::WELL>().getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);
        for (const auto& well_name: well_names) {
            auto well = handlerContext.state().wells(well_name);
            const auto filter_conc = record.getItem<ParserKeywords::WINJFCNC::VOL_CONCENTRATION>().get<UDAValue>(0);
            well.setFilterConc(filter_conc );
            handlerContext.state().wells.update(std::move(well));
        }
    }
}

void handleWINJMULT(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL_NAME").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, true);

        for (const auto& well_name : well_names) {
            auto well = handlerContext.state().wells( well_name );
            if (well.isProducer()) {
                const std::string reason = fmt::format("Keyword WINJMULT can only apply to injectors,"
                                                       " but Well {} is a producer", well_name);
                throw OpmInputError(reason, handlerContext.keyword.location());
            }
            if (well.handleWINJMULT(record, handlerContext.keyword.location())) {
                handlerContext.state().wells.update(std::move(well));
            }
        }
    }
}

void handleWINJTEMP(HandlerContext& handlerContext)
{
    // we do not support the "enthalpy" field yet. how to do this is a more difficult
    // question.
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        auto well_names = handlerContext.wellNames(wellNamePattern, false);

        const double temp = record.getItem("TEMPERATURE").getSIDouble(0);

        for (const auto& well_name : well_names) {
            const auto& well = handlerContext.state().wells.get(well_name);
            const double current_temp = well.hasInjTemperature()? well.inj_temperature(): 0.0;
            if (current_temp != temp) {
                auto well2 = handlerContext.state().wells( well_name );
                well2.setWellInjTemperature(temp);
                handlerContext.state().wells.update( std::move(well2) );
            }
        }
    }
}

void handleWMICP(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);

        for (const auto& well_name : well_names) {
            auto well = handlerContext.state().wells( well_name );
            auto micp_properties = std::make_shared<WellMICPProperties>( well.getMICPProperties() );
            micp_properties->handleWMICP(record);
            if (well.updateMICPProperties(micp_properties)) {
                handlerContext.state().wells.update( std::move(well));
            }
        }
    }
}

void handleWPIMULT(HandlerContext& handlerContext)
{
    // from the third item to the seventh item in the WPIMULT record, they are numbers indicate
    // the I, J, K location and completion number range.
    // When defaulted, it assumes it is negative
    // When inputting a negative value, it assumes it is defaulted.
    auto defaultConCompRec = [](const DeckRecord& wpimult)
    {
        return std::all_of(wpimult.begin() + 2, wpimult.end(),
            [](const DeckItem& item)
            {
                return item.defaultApplied(0) || (item.get<int>(0) < 0);
            });
    };

    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto& well_names = handlerContext.wellNames(wellNamePattern);

        // for the record has defaulted connection and completion information, we do not apply it immediately
        // because we only need to apply the last record with defaulted connection and completion information
        // as a result, we here only record the information of the record with defaulted connection and completion
        // information without applying, because there might be multiple WPIMULT keywords here, and we do not know
        // whether it is the last one.
        const bool default_con_comp = defaultConCompRec(record);
        if (default_con_comp) {
            auto& wpimult_global_factor = handlerContext.wpimult_global_factor;
            const auto scaling_factor = record.getItem("WELLPI").get<double>(0);
            for (const auto& wname : well_names) {
                wpimult_global_factor.insert_or_assign(wname, scaling_factor);
            }
            continue;
        }

        // the record with non-defaulted connection and completion information will be applied immediately
        for (const auto& wname : well_names) {
            auto well = handlerContext.state().wells( wname );
            if (well.handleWPIMULT(record))
                handlerContext.state().wells.update( std::move(well));
        }
    }
}

void handleWPMITAB(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, true);

        for (const auto& well_name : well_names) {
            auto well = handlerContext.state().wells( well_name );
            auto polymer_properties = std::make_shared<WellPolymerProperties>( well.getPolymerProperties() );
            polymer_properties->handleWPMITAB(record);
            if (well.updatePolymerProperties(polymer_properties))
                handlerContext.state().wells.update( std::move(well));
        }
    }
}

void handleWPOLYMER(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);

        for (const auto& well_name : well_names) {
            auto well = handlerContext.state().wells( well_name );
            auto polymer_properties = std::make_shared<WellPolymerProperties>( well.getPolymerProperties() );
            polymer_properties->handleWPOLYMER(record);
            if (well.updatePolymerProperties(polymer_properties))
                handlerContext.state().wells.update( std::move(well));
        }
    }
}

void handleWSALT(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, true);

        for (const auto& well_name : well_names) {
            auto well2 = handlerContext.state().wells( well_name );
            auto brine_properties = std::make_shared<WellBrineProperties>(well2.getBrineProperties());
            brine_properties->handleWSALT(record);
            if (well2.updateBrineProperties(brine_properties))
                handlerContext.state().wells.update( std::move(well2) );
        }
    }
}

void handleWSKPTAB(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);

        for (const auto& well_name : well_names) {
            auto well = handlerContext.state().wells( well_name );

            auto polymer_properties = std::make_shared<WellPolymerProperties>(well.getPolymerProperties());
            polymer_properties->handleWSKPTAB(record);
            if (well.updatePolymerProperties(polymer_properties))
                handlerContext.state().wells.update( std::move(well) );
        }
    }
}

void handleWSOLVENT(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);

        const double fraction = record.getItem("SOLVENT_FRACTION").get<UDAValue>(0).getSI();

        for (const auto& well_name : well_names) {
            const auto& well = handlerContext.state().wells.get(well_name);
            const auto& inj = well.getInjectionProperties();
            if (!well.isProducer() && inj.injectorType == InjectorType::GAS) {
                if (well.getSolventFraction() != fraction) {
                    auto well2 = handlerContext.state().wells( well_name );
                    well2.updateSolventFraction(fraction);
                    handlerContext.state().wells.update( std::move(well2) );
                }
            } else {
                throw std::invalid_argument("The WSOLVENT keyword can only be applied to gas injectors");
            }
        }
    }
}

void handleWTEMP(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);
        double temp = record.getItem("TEMP").getSIDouble(0);

        for (const auto& well_name : well_names) {
            const auto& well = handlerContext.state().wells.get(well_name);
            const double current_temp = well.hasInjTemperature()? well.inj_temperature(): 0.0;
            if (current_temp != temp) {
                auto well2 = handlerContext.state().wells( well_name );
                well2.setWellInjTemperature(temp);
                handlerContext.state().wells.update( std::move(well2) );
            }
        }
    }
}

// The WTMULT keyword can optionally use UDA values in three different ways:
//
//   1. The target can be UDA - instead of the standard strings "ORAT",
//      "GRAT", "WRAT", ..., the keyword can be configured with a UDA
//      which is evaluated to an integer and then mapped to one of the
//      common controls.
//
//   2. The scaling factor itself can be a UDA.
//
//   3. The target we aim to scale might already be specified as a UDA.
//
// The current implementation does not support UDA usage in any part of
// WTMULT codepath.
void handleWTMULT(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const auto& factor = record.getItem<ParserKeywords::WTMULT::FACTOR>().get<UDAValue>(0);
        if (factor.is<std::string>()) {
            const auto reason =
                fmt::format("UDA value {} is not supported "
                            "as multiplier", factor.get<std::string>());

            throw OpmInputError {
                reason, handlerContext.keyword.location()
            };
        }

        const auto& control = record.getItem<ParserKeywords::WTMULT::CONTROL>().get<std::string>(0);
        if (handlerContext.state().udq().has_keyword(control)) {
            const auto reason =
                fmt::format("UDA value {} is not supported "
                            "for control target", control);

            throw OpmInputError {
                reason, handlerContext.keyword.location()
            };
        }

        const auto num = record.getItem<ParserKeywords::WTMULT::NUM>().get<int>(0);
        if (num != 1) {
            throw OpmInputError {
                "Only NUM=1 is supported in WTMULT keyword",
                handlerContext.keyword.location()
            };
        }

        const auto cmode = WellWELTARGCModeFromString(control);
        if (cmode == Well::WELTARGCMode::GUID) {
            throw OpmInputError {
                "Multiplying the guide rate is not supported",
                handlerContext.keyword.location()
            };
        }

        const auto& wellNamePattern = record.getItem<ParserKeywords::WTMULT::WELL>().getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern);
        for (const auto& well_name : well_names) {
            auto well = handlerContext.state().wells.get(well_name);

            if (well.isInjector()) {
                const bool update_well = true;

                auto properties = std::make_shared<Well::WellInjectionProperties>(well.getInjectionProperties());
                properties->handleWTMULT(cmode, factor.get<double>());

                well.updateInjection(properties);
                if (update_well) {
                    handlerContext.state().events()
                        .addEvent(ScheduleEvents::INJECTION_UPDATE);

                    handlerContext.state().wellgroup_events()
                        .addEvent(well_name, ScheduleEvents::INJECTION_UPDATE);

                    handlerContext.state().wells.update(std::move(well));
                }
            }
            else {
                const bool update_well = true;

                auto properties = std::make_shared<Well::WellProductionProperties>(well.getProductionProperties());
                properties->handleWTMULT(cmode, factor.get<double>());

                well.updateProduction(properties);
                if (update_well) {
                    handlerContext.state().events()
                        .addEvent(ScheduleEvents::PRODUCTION_UPDATE);

                    handlerContext.state().wellgroup_events()
                        .addEvent(well_name, ScheduleEvents::PRODUCTION_UPDATE);

                    handlerContext.state().wells.update(std::move(well));
                }
            }

            handlerContext.affected_well(well_name);
        }
    }
}

void handleWTRACER(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, false);

        if (well_names.empty()) {
            handlerContext.invalidNamePattern(wellNamePattern);
        }

        const double tracerConcentration = record.getItem("CONCENTRATION").get<UDAValue>(0).getSI();
        const std::string& tracerName = record.getItem("TRACER").getTrimmedString(0);

        for (const auto& well_name : well_names) {
            auto well = handlerContext.state().wells.get( well_name );
            auto wellTracerProperties = std::make_shared<WellTracerProperties>(well.getTracerProperties());
            wellTracerProperties->setConcentration(tracerName, tracerConcentration);
            if (well.updateTracer(wellTracerProperties))
                handlerContext.state().wells.update( std::move(well) );
        }
    }
}

void handleWVFPDP(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, true);
        if (well_names.empty()) {
            handlerContext.invalidNamePattern(wellNamePattern);
        }

        for (const auto& well_name : well_names) {
            auto well = handlerContext.state().wells.get(well_name);
            auto wvfpdp = std::make_shared<WVFPDP>(well.getWVFPDP());
            wvfpdp->update( record );
            if (well.updateWVFPDP(std::move(wvfpdp))) {
                handlerContext.state().wells.update( std::move(well) );
            }
        }
    }
}

void handleWVFPEXP(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, true);
        if (well_names.empty()) {
            handlerContext.invalidNamePattern(wellNamePattern);
        }

        for (const auto& well_name : well_names) {
            auto well = handlerContext.state().wells.get(well_name);
            auto wvfpexp = std::make_shared<WVFPEXP>(well.getWVFPEXP());
            wvfpexp->update( record );
            if (well.updateWVFPEXP(std::move(wvfpexp)))
                handlerContext.state().wells.update( std::move(well) );
        }
    }
}

void handleWWPAVE(HandlerContext& handlerContext)
{
    for (const auto& record : handlerContext.keyword) {
        const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
        const auto well_names = handlerContext.wellNames(wellNamePattern, true);

        if (well_names.empty()) {
            handlerContext.invalidNamePattern(wellNamePattern);
        }

        auto wpave = PAvg(record);

        if (wpave.inner_weight() > 1.0) {
            const auto reason =
                fmt::format("Inner block weighting F1 "
                            "must not exceed one. Got {}",
                            wpave.inner_weight());

            throw OpmInputError {
                reason, handlerContext.keyword.location()
            };
        }

        if ((wpave.conn_weight() < 0.0) ||
            (wpave.conn_weight() > 1.0))
        {
            const auto reason =
                fmt::format("Connection weighting factor F2 "
                            "must be between zero and one, "
                            "inclusive. Got {} instead.",
                            wpave.conn_weight());

            throw OpmInputError {
                reason, handlerContext.keyword.location()
            };
        }

        for (const auto& well_name : well_names) {
            const auto& well = handlerContext.state().wells.get(well_name);
            if (well.pavg() != wpave) {
                auto new_well = handlerContext.state().wells.get(well_name);
                new_well.updateWPAVE(wpave);
                handlerContext.state().wells.update(std::move(new_well));
            }
        }
    }
}

}

std::vector<std::pair<std::string,KeywordHandlers::handler_function>>
getWellPropertiesHandlers()
{
    return {
        { "WDFAC"   ,  &handleWDFAC   },
        { "WDFACCOR", &handleWDFACCOR },
        { "WECON"   , &handleWECON    },
        { "WEFAC"   , &handleWEFAC    },
        { "WELPI"   , &handleWELPI    },
        { "WFOAM"   , &handleWFOAM    },
        { "WINJCLN",  &handleWINJCLN  },
        { "WINJDAM" , &handleWINJDAM  },
        { "WINJFCNC", &handleWINJFCNC },
        { "WINJMULT", &handleWINJMULT },
        { "WINJTEMP", &handleWINJTEMP },
        { "WMICP"   , &handleWMICP    },
        { "WPIMULT" , &handleWPIMULT  },
        { "WPMITAB" , &handleWPMITAB  },
        { "WPOLYMER", &handleWPOLYMER },
        { "WSALT"   , &handleWSALT    },
        { "WSKPTAB" , &handleWSKPTAB  },
        { "WSOLVENT", &handleWSOLVENT },
        { "WTEMP"   , &handleWTEMP    },
        { "WTMULT"  , &handleWTMULT   },
        { "WTRACER" , &handleWTRACER  },
        { "WVFPDP",   &handleWVFPDP   },
        { "WVFPEXP" , &handleWVFPEXP  },
        { "WWPAVE"  , &handleWWPAVE   },
    };
}

}
