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

#include "KeywordHandlers.hpp"

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/LogUtil.hpp>
#include <opm/common/utility/OpmInputError.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/EclipseState/Aquifer/AquiferFlux.hpp>

#include <opm/input/eclipse/Schedule/Action/Actions.hpp>
#include <opm/input/eclipse/Schedule/Action/PyAction.hpp>
#include <opm/input/eclipse/Schedule/Events.hpp>
#include <opm/input/eclipse/Schedule/ScheduleState.hpp>
#include <opm/input/eclipse/Schedule/ScheduleStatic.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/Tuning.hpp>

#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/E.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/F.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/N.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/P.hpp>

#include "GasLiftOptKeywordHandlers.hpp"
#include "Group/GroupKeywordHandlers.hpp"
#include "Group/GuideRateKeywordHandlers.hpp"
#include "HandlerContext.hpp"
#include "MixingRateControlKeywordHandlers.hpp"
#include "MSW/MSWKeywordHandlers.hpp"
#include "Network/NetworkKeywordHandlers.hpp"
#include "ResCoup/ReservoirCouplingKeywordHandlers.hpp"
#include "RXXKeywordHandlers.hpp"
#include "UDQ/UDQKeywordHandlers.hpp"
#include "Well/GridIndependentWellKeywordHandlers.hpp"
#include "Well/WellCompletionKeywordHandlers.hpp"
#include "Well/WellKeywordHandlers.hpp"
#include "Well/WellPropertiesKeywordHandlers.hpp"

#include <fmt/format.h>

#include <exception>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Opm {

namespace {

void handleAQUCT(const HandlerContext& handlerContext)
{
    throw OpmInputError("AQUCT is not supported as SCHEDULE keyword", handlerContext.keyword.location());
}

void handleAQUFETP(const HandlerContext& handlerContext)
{
    throw OpmInputError("AQUFETP is not supported as SCHEDULE keyword", handlerContext.keyword.location());
}

void handleAQUFLUX(HandlerContext& handlerContext)
{
    auto& aqufluxs = handlerContext.state().aqufluxs;
    for (const auto& record : handlerContext.keyword) {
        const auto aquifer = SingleAquiferFlux { record };
        aqufluxs.insert_or_assign(aquifer.id, aquifer);
    }
}

void handleBCProp(HandlerContext& handlerContext)
{
    auto& bcprop = handlerContext.state().bcprop;
    for (const auto& record : handlerContext.keyword) {
        bcprop.updateBCProp(record);
    }
}

void handleSource(HandlerContext& handlerContext)
{
    auto new_source = handlerContext.state().source.get();
    for (const auto& record : handlerContext.keyword) {
        new_source.updateSource(record);
    }
    handlerContext.state().source.update(std::move(new_source));
}

void handleEXIT(HandlerContext& handlerContext)
{
    using ES = ParserKeywords::EXIT;
    int status = handlerContext.keyword.getRecord(0).getItem<ES::STATUS_CODE>().get<int>(0);
    OpmLog::info("Simulation exit with status: " +
                 std::to_string(status) +
                 " requested by an action keyword at report_step: " +
                 std::to_string(handlerContext.currentStep));
    handlerContext.setExitCode(status);
}

void handleFBHPDEF(HandlerContext& handlerContext)
{
    using FBHP = ParserKeywords::FBHPDEF;
    const auto& record = handlerContext.keyword.getRecord(0);
    ScheduleState::BHPDefaults bhp_defaults;
    const auto& prod_limit = record.getItem<FBHP::TARGET_BHP>();
    const auto& inj_limit = record.getItem<FBHP::LIMIT_BHP>();
    if (!(prod_limit.defaultApplied(0) && inj_limit.defaultApplied(0))) {
        bhp_defaults.prod_target = prod_limit.getSIDouble(0);
        bhp_defaults.inj_limit = inj_limit.getSIDouble(0);
    }
    handlerContext.state().bhp_defaults.update(std::move(bhp_defaults));
}

void handleMESSAGES(HandlerContext& handlerContext)
{
    handlerContext.state().message_limits().update( handlerContext.keyword );
}

void handleGEOKeyword(HandlerContext& handlerContext)
{
    handlerContext.state().geo_keywords().push_back(handlerContext.keyword);
    handlerContext.state().events().addEvent( ScheduleEvents::GEO_MODIFIER );
    handlerContext.record_tran_change();
}

void handleMXUNSUPP(HandlerContext& handlerContext)
{
    std::string msg_fmt = fmt::format("Problem with keyword {{keyword}} at report step {}\n"
                                      "In {{file}} line {{line}}\n"
                                      "OPM does not support grid property modifier {} in the Schedule section", handlerContext.currentStep, handlerContext.keyword.name());
    OpmLog::warning(OpmInputError::format(msg_fmt, handlerContext.keyword.location()));
}

void handleNEXTSTEP(HandlerContext& handlerContext)
{
    const auto& record = handlerContext.keyword[0];
    auto next_tstep = record.getItem<ParserKeywords::NEXTSTEP::MAX_STEP>().getSIDouble(0);
    auto apply_to_all = DeckItem::to_bool( record.getItem<ParserKeywords::NEXTSTEP::APPLY_TO_ALL>().get<std::string>(0) );

    handlerContext.state().next_tstep = NextStep{next_tstep, apply_to_all};
    handlerContext.state().events().addEvent(ScheduleEvents::TUNING_CHANGE);
}

void handleNUPCOL(HandlerContext& handlerContext)
{
    const int nupcol = handlerContext.keyword.getRecord(0).getItem("NUM_ITER").get<int>(0);

    if (handlerContext.keyword.getRecord(0).getItem("NUM_ITER").defaultApplied(0)) {
        std::string msg = "OPM Flow uses 12 as default NUPCOL value";
        OpmLog::note(msg);
    }

    handlerContext.state().update_nupcol(nupcol);
}

void handlePYACTION(HandlerContext& handlerContext)
{
    if (!handlerContext.static_schedule().m_python_handle->enabled()) {
        //Must have a real Python instance here - to ensure that IMPORT works
        const auto& loc = handlerContext.keyword.location();
        throw OpmInputError("This version of flow is built without support for Python. The Keyword PYACTION is ignored.", loc);
    }

    const auto& keyword = handlerContext.keyword;
    const auto& name = keyword.getRecord(0).getItem<ParserKeywords::PYACTION::NAME>().get<std::string>(0);
    const auto& run_count = Action::PyAction::from_string( keyword.getRecord(0).getItem<ParserKeywords::PYACTION::RUN_COUNT>().get<std::string>(0) );
    const auto& module_arg = keyword.getRecord(1).getItem<ParserKeywords::PYACTION::FILENAME>().get<std::string>(0);
    std::string module;
    if (handlerContext.static_schedule().m_input_path.empty()) {
        module = module_arg;
    } else {
        module = handlerContext.static_schedule().m_input_path + "/" + module_arg;
    }

    Action::PyAction pyaction(handlerContext.static_schedule().m_python_handle, name, run_count, module);
    auto new_actions = handlerContext.state().actions.get();
    new_actions.add(pyaction);
    handlerContext.state().actions.update( std::move(new_actions) );
}

void handleSUMTHIN(HandlerContext& handlerContext)
{
    auto value = handlerContext.keyword.getRecord(0).getItem(0).getSIDouble(0);
    handlerContext.state().update_sumthin( value );
}

void handleTUNING(HandlerContext& handlerContext)
{
    const auto numrecords = handlerContext.keyword.size();
    auto tuning = handlerContext.state().tuning();

    // local helpers
    auto nondefault_or_previous_double = [](const Opm::DeckRecord& rec, const std::string& item_name, double previous_value) {
        const auto& deck_item = rec.getItem(item_name);
        return deck_item.defaultApplied(0) ? previous_value : rec.getItem(item_name).get< double >(0);
    };

    auto nondefault_or_previous_int = [](const Opm::DeckRecord& rec, const std::string& item_name, int previous_value) {
        const auto& deck_item = rec.getItem(item_name);
        return deck_item.defaultApplied(0) ? previous_value : rec.getItem(item_name).get< int >(0);
    };

    auto nondefault_or_previous_sidouble = [](const Opm::DeckRecord& rec, const std::string& item_name, double previous_value) {
        const auto& deck_item = rec.getItem(item_name);
        return deck_item.defaultApplied(0) ? previous_value : rec.getItem(item_name).getSIDouble(0);
    };

    // \Note No TSTINIT value should not be used unless explicitly non-defaulted, hence removing value by default
    // \Note (exception is the first time step, which is handled by the Tuning constructor)
    tuning.TSINIT = std::nullopt;

    if (numrecords > 0) {
        const auto& record1 = handlerContext.keyword.getRecord(0);

        // \Note A value indicates TSINIT was set in this record
        if (const auto& deck_item = record1.getItem("TSINIT"); !deck_item.defaultApplied(0))
            tuning.TSINIT = std::optional<double>{ record1.getItem("TSINIT").getSIDouble(0) };

        tuning.TSMAXZ = nondefault_or_previous_sidouble(record1, "TSMAXZ", tuning.TSMAXZ);
        tuning.TSMINZ = nondefault_or_previous_sidouble(record1, "TSMINZ", tuning.TSMINZ);
        tuning.TSMCHP = nondefault_or_previous_sidouble(record1, "TSMCHP", tuning.TSMCHP);
        tuning.TSFMAX = nondefault_or_previous_double(record1, "TSFMAX", tuning.TSFMAX);
        tuning.TSFMIN = nondefault_or_previous_double(record1, "TSFMIN", tuning.TSFMIN);
        tuning.TSFCNV = nondefault_or_previous_double(record1, "TSFCNV", tuning.TSFCNV);
        tuning.TFDIFF = nondefault_or_previous_double(record1, "TFDIFF", tuning.TFDIFF);
        tuning.THRUPT = nondefault_or_previous_double(record1, "THRUPT", tuning.THRUPT);

        const auto& TMAXWCdeckItem = record1.getItem("TMAXWC");
        if (TMAXWCdeckItem.hasValue(0)) {
            tuning.TMAXWC_has_value = true;
            tuning.TMAXWC = nondefault_or_previous_sidouble(record1, "TMAXWC", tuning.TMAXWC);
        }
    }

    if (numrecords > 1) {
        const auto& record2 = handlerContext.keyword.getRecord(1);

        tuning.TRGTTE = nondefault_or_previous_double(record2, "TRGTTE", tuning.TRGTTE);
        tuning.TRGCNV = nondefault_or_previous_double(record2, "TRGCNV", tuning.TRGCNV);
        tuning.TRGMBE = nondefault_or_previous_double(record2, "TRGMBE", tuning.TRGMBE);
        tuning.TRGLCV = nondefault_or_previous_double(record2, "TRGLCV", tuning.TRGLCV);
        tuning.XXXTTE = nondefault_or_previous_double(record2, "XXXTTE", tuning.XXXTTE);
        tuning.XXXCNV = nondefault_or_previous_double(record2, "XXXCNV", tuning.XXXCNV);
        tuning.XXXMBE = nondefault_or_previous_double(record2, "XXXMBE", tuning.XXXMBE);
        tuning.XXXLCV = nondefault_or_previous_double(record2, "XXXLCV", tuning.XXXLCV);
        tuning.XXXWFL = nondefault_or_previous_double(record2, "XXXWFL", tuning.XXXWFL);
        tuning.TRGFIP = nondefault_or_previous_double(record2, "TRGFIP", tuning.TRGFIP);

        const auto& TRGSFTdeckItem = record2.getItem("TRGSFT");
        if (TRGSFTdeckItem.hasValue(0)) {
            tuning.TRGSFT_has_value = true;
            tuning.TRGSFT = nondefault_or_previous_double(record2, "TRGSFT", tuning.TRGSFT);
        }
        else {
            tuning.TRGSFT_has_value = false;
        }

        tuning.THIONX = nondefault_or_previous_double(record2, "THIONX", tuning.THIONX);
        tuning.TRWGHT = nondefault_or_previous_int(record2, "TRWGHT", tuning.TRWGHT);

        // Check for no supported records from the deck to write as a warning.
        // We check if the deck values are different from the default ones, since the method
        // record.getItem("").hasValue(0) does not differentiate between deck and default values.
        // An alternative is to remove the default values for the not supported items in TUNING
        // and use record.getItem("").hasValue(0).
        tuning.TRGTTE_has_value = !record2.getItem("TRGTTE").defaultApplied(0);
        tuning.TRGLCV_has_value = !record2.getItem("TRGLCV").defaultApplied(0);
        tuning.XXXTTE_has_value = !record2.getItem("XXXTTE").defaultApplied(0);
        tuning.XXXLCV_has_value = !record2.getItem("XXXLCV").defaultApplied(0);
        tuning.XXXWFL_has_value = !record2.getItem("XXXWFL").defaultApplied(0);
        tuning.TRGFIP_has_value = !record2.getItem("TRGFIP").defaultApplied(0);
        tuning.THIONX_has_value = !record2.getItem("THIONX").defaultApplied(0);
        tuning.TRWGHT_has_value = !record2.getItem("TRWGHT").defaultApplied(0);
    }

    if (numrecords > 2) {
        const auto& record3 = handlerContext.keyword.getRecord(2);

        tuning.NEWTMX = nondefault_or_previous_int(record3, "NEWTMX", tuning.NEWTMX);
        tuning.NEWTMN = nondefault_or_previous_int(record3, "NEWTMN", tuning.NEWTMN);
        tuning.LITMAX = nondefault_or_previous_int(record3, "LITMAX", tuning.LITMAX);
        tuning.LITMIN = nondefault_or_previous_int(record3, "LITMIN", tuning.LITMIN);
        tuning.MXWSIT = nondefault_or_previous_int(record3, "MXWSIT", tuning.MXWSIT);
        tuning.MXWPIT = nondefault_or_previous_int(record3, "MXWPIT", tuning.MXWPIT);
        tuning.DDPLIM = nondefault_or_previous_sidouble(record3, "DDPLIM", tuning.DDPLIM);
        tuning.DDSLIM = nondefault_or_previous_double(record3, "DDSLIM", tuning.DDSLIM);
        tuning.TRGDPR = nondefault_or_previous_sidouble(record3, "TRGDPR", tuning.TRGDPR);

        const auto& XXXDPRdeckItem = record3.getItem("XXXDPR");
        if (XXXDPRdeckItem.hasValue(0)) {
            tuning.XXXDPR_has_value = true;
            tuning.XXXDPR = nondefault_or_previous_sidouble(record3, "XXXDPR", tuning.XXXDPR);
        }
        else {
            tuning.XXXDPR_has_value = false;
        }

        tuning.MNWRFP = nondefault_or_previous_int(record3, "MNWRFP", tuning.MNWRFP);

        // Check for no supported records from the deck to write as a warning.
        tuning.LITMAX_has_value = !record3.getItem("LITMAX").defaultApplied(0);
        tuning.LITMIN_has_value = !record3.getItem("LITMIN").defaultApplied(0);
        tuning.MXWSIT_has_value = !record3.getItem("MXWSIT").defaultApplied(0);
        tuning.MXWPIT_has_value = !record3.getItem("MXWPIT").defaultApplied(0);
        tuning.DDPLIM_has_value = !record3.getItem("DDPLIM").defaultApplied(0);
        tuning.DDSLIM_has_value = !record3.getItem("DDSLIM").defaultApplied(0);
        tuning.TRGDPR_has_value = !record3.getItem("TRGDPR").defaultApplied(0);
        tuning.MNWRFP_has_value = !record3.getItem("MNWRFP").defaultApplied(0);
    }

    handlerContext.state().update_tuning( std::move( tuning ));
    handlerContext.state().events().addEvent(ScheduleEvents::TUNING_CHANGE);
}

void handleTUNINGDP(HandlerContext& handlerContext)
{
    // Get TUNINGDP state and records
    auto tuning_dp = handlerContext.state().tuning_dp();
    const auto& record = handlerContext.keyword.getRecord(0);

    // Update defaults if first time handling TUNINGDP
    if (!tuning_dp.defaults_updated) {
        tuning_dp.set_defaults();
    }

    // Local helpers
    auto nondefault_or_previous_double = [](const Opm::DeckRecord& rec, const std::string& item_name, double previous_value) {
        const auto& deck_item = rec.getItem(item_name);
        return deck_item.defaultApplied(0) ? previous_value : rec.getItem(item_name).get< double >(0);
    };

    auto nondefault_or_previous_sidouble = [](const Opm::DeckRecord& rec, const std::string& item_name, double previous_value) {
        const auto& deck_item = rec.getItem(item_name);
        return deck_item.defaultApplied(0) ? previous_value : rec.getItem(item_name).getSIDouble(0);
    };

    // Parse records
    // NOTE: TRGLCV and XXXLCV are the same as in TUNING and must be parsed the same way
    tuning_dp.TRGLCV = nondefault_or_previous_double(record, "TRGLCV", tuning_dp.TRGLCV);
    tuning_dp.XXXLCV = nondefault_or_previous_double(record, "XXXLCV", tuning_dp.XXXLCV);
    tuning_dp.TRGDDP = nondefault_or_previous_sidouble(record, "TRGDDP", tuning_dp.TRGDDP);
    tuning_dp.TRGDDS = nondefault_or_previous_double(record, "TRGDDS", tuning_dp.TRGDDS);
    tuning_dp.TRGDDRS = nondefault_or_previous_sidouble(record, "TRGDDRS", tuning_dp.TRGDDRS);
    tuning_dp.TRGDDRV = nondefault_or_previous_sidouble(record, "TRGDDRV", tuning_dp.TRGDDRV);
    tuning_dp.TRGDDT = nondefault_or_previous_sidouble(record, "TRGDDT", tuning_dp.TRGDDT);

    // See handleTUNING for TRGLCV_has_value and XXXLCV_has_value
    tuning_dp.TRGLCV_has_value = !record.getItem("TRGLCV").defaultApplied(0);
    tuning_dp.XXXLCV_has_value = !record.getItem("XXXLCV").defaultApplied(0);

    // Update state and events
    handlerContext.state().update_tuning_dp( std::move(tuning_dp) );
    handlerContext.state().events().addEvent(ScheduleEvents::TUNINGDP_CHANGE);
}

void handleVFPINJ(HandlerContext& handlerContext)
{
    auto table = VFPInjTable(handlerContext.keyword,
                             handlerContext.static_schedule().m_unit_system);
    handlerContext.state().events().addEvent( ScheduleEvents::VFPINJ_UPDATE );
    handlerContext.state().vfpinj.update( std::move(table) );
}

void handleVFPPROD(HandlerContext& handlerContext)
{
    auto table = VFPProdTable(handlerContext.keyword,
                              handlerContext.static_schedule().gaslift_opt_active,
                              handlerContext.static_schedule().m_unit_system);
    handlerContext.state().events().addEvent( ScheduleEvents::VFPPROD_UPDATE );
    handlerContext.state().vfpprod.update( std::move(table) );
}

}

const KeywordHandlers& KeywordHandlers::getInstance()
{
    static KeywordHandlers instance{};
    return instance;
}

KeywordHandlers::KeywordHandlers()
    : handler_functions{
        { "AQUCT",    &handleAQUCT      },
        { "AQUFETP",  &handleAQUFETP    },
        { "AQUFLUX",  &handleAQUFLUX    },
        { "BCPROP",   &handleBCProp     },
        { "BOX",      &handleGEOKeyword },
        { "ENDBOX"  , &handleGEOKeyword },
        { "EXIT",     &handleEXIT       },
        { "FBHPDEF",  &handleFBHPDEF    },
        { "MESSAGES", &handleMESSAGES   },
        { "MULTFLT" , &handleGEOKeyword },
        { "MULTPV"  , &handleMXUNSUPP   },
        { "MULTR"   , &handleMXUNSUPP   },
        { "MULTR-"  , &handleMXUNSUPP   },
        { "MULTREGT", &handleMXUNSUPP   },
        { "MULTSIG" , &handleMXUNSUPP   },
        { "MULTSIGV", &handleMXUNSUPP   },
        { "MULTTHT" , &handleMXUNSUPP   },
        { "MULTTHT-", &handleMXUNSUPP   },
        { "MULTX"   , &handleGEOKeyword },
        { "MULTX-"  , &handleGEOKeyword },
        { "MULTY"   , &handleGEOKeyword },
        { "MULTY-"  , &handleGEOKeyword },
        { "MULTZ"   , &handleGEOKeyword },
        { "MULTZ-"  , &handleGEOKeyword },
        { "NEXT",     &handleNEXTSTEP   },
        { "NEXTSTEP", &handleNEXTSTEP   },
        { "NUPCOL"  , &handleNUPCOL     },
        { "PYACTION", &handlePYACTION   },
        { "SOURCE",   &handleSource },
        { "SUMTHIN" , &handleSUMTHIN    },
        { "TUNING"  , &handleTUNING     },
        { "TUNINGDP", &handleTUNINGDP   },
        { "VFPINJ"  , &handleVFPINJ     },
        { "VFPPROD" , &handleVFPPROD    },
    }
{
    for (const auto& handlerFactory : {getGasLiftOptHandlers,
                                       getGridIndependentWellKeywordHandlers,
                                       getGroupHandlers,
                                       getGuideRateHandlers,
                                       getMixingRateControlHandlers,
                                       getMSWHandlers,
                                       getNetworkHandlers,
                                       getUDQHandlers,
                                       getReservoirCouplingHandlers,
                                       getRXXHandlers,
                                       getWellCompletionHandlers,
                                       getWellHandlers,
                                       getWellPropertiesHandlers}) {
        for (const auto& [keyword, handler] : std::invoke(handlerFactory)) {
            handler_functions.emplace(keyword, handler);
        }
    }
}

bool KeywordHandlers::handleKeyword(HandlerContext& handlerContext) const
{
    auto function_iterator = handler_functions.find(handlerContext.keyword.name());
    if (function_iterator == handler_functions.end()) {
        return false;
    }

    try {
        std::invoke(function_iterator->second, handlerContext);
    } catch (const OpmInputError&) {
        throw;
    } catch (const std::logic_error& e) {
        // Rethrow as OpmInputError to provide more context,
        // but add "Internal error: " to the string, as that
        // is what logic_error signifies.
        const OpmInputError opm_error {
            std::string("Internal error: ") + e.what(),
            handlerContext.keyword.location()
        };
        OpmLog::error(opm_error.what());
        std::throw_with_nested(opm_error);
    } catch (const std::exception& e) {
        // Rethrow as OpmInputError to provide more context.
        const OpmInputError opm_error { e, handlerContext.keyword.location() } ;
        OpmLog::error(opm_error.what());
        std::throw_with_nested(opm_error);
    }

    return true;
}

}
