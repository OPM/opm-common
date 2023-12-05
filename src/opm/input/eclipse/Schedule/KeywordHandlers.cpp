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

#include <opm/input/eclipse/Schedule/Schedule.hpp>

#include <opm/common/OpmLog/OpmLog.hpp>
#include <opm/common/OpmLog/LogUtil.hpp>
#include <opm/common/utility/OpmInputError.hpp>
#include <opm/common/utility/numeric/cmp.hpp>
#include <opm/common/utility/String.hpp>

#include <opm/input/eclipse/Python/Python.hpp>

#include <opm/input/eclipse/EclipseState/Phase.hpp>
#include <opm/input/eclipse/EclipseState/Aquifer/AquiferFlux.hpp>

#include <opm/input/eclipse/Schedule/Action/ActionResult.hpp>
#include <opm/input/eclipse/Schedule/Action/Actions.hpp>
#include <opm/input/eclipse/Schedule/Action/ActionX.hpp>
#include <opm/input/eclipse/Schedule/Action/PyAction.hpp>
#include <opm/input/eclipse/Schedule/Action/SimulatorUpdate.hpp>
#include <opm/input/eclipse/Schedule/Events.hpp>
#include <opm/input/eclipse/Schedule/OilVaporizationProperties.hpp>
#include <opm/input/eclipse/Schedule/RFTConfig.hpp>
#include <opm/input/eclipse/Schedule/RPTConfig.hpp>
#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Schedule/Tuning.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQActive.hpp>
#include <opm/input/eclipse/Schedule/UDQ/UDQConfig.hpp>
#include <opm/input/eclipse/Schedule/Well/NameOrder.hpp>
#include <opm/input/eclipse/Schedule/Well/WList.hpp>
#include <opm/input/eclipse/Schedule/Well/WListManager.hpp>
#include <opm/input/eclipse/Schedule/Well/Well.hpp>
#include <opm/input/eclipse/Schedule/Well/WellConnections.hpp>
#include <opm/input/eclipse/Schedule/Well/WellEconProductionLimits.hpp>
#include <opm/input/eclipse/Schedule/Well/WellTestConfig.hpp>

#include <opm/input/eclipse/Schedule/SummaryState.hpp>
#include <opm/input/eclipse/Units/Dimension.hpp>
#include <opm/input/eclipse/Units/UnitSystem.hpp>
#include <opm/input/eclipse/Units/Units.hpp>

#include <opm/input/eclipse/Deck/DeckItem.hpp>
#include <opm/input/eclipse/Deck/DeckKeyword.hpp>
#include <opm/input/eclipse/Deck/DeckRecord.hpp>
#include <opm/input/eclipse/Deck/DeckSection.hpp>

#include <opm/input/eclipse/Parser/ErrorGuard.hpp>
#include <opm/input/eclipse/Parser/ParseContext.hpp>

#include <opm/input/eclipse/Parser/ParserKeywords/D.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/E.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/F.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/N.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/P.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/T.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/V.hpp>
#include <opm/input/eclipse/Parser/ParserKeywords/W.hpp>

#include "GasLiftOptKeywordHandlers.hpp"
#include "Group/GroupKeywordHandlers.hpp"
#include "Group/GuideRateKeywordHandlers.hpp"
#include "HandlerContext.hpp"
#include "MSW/MSWKeywordHandlers.hpp"
#include "Network/NetworkKeywordHandlers.hpp"
#include "UDQ/UDQKeywordHandlers.hpp"
#include "Well/WellCompletionKeywordHandlers.hpp"
#include "Well/WellPropertiesKeywordHandlers.hpp"
#include "Well/injection.hpp"

#include <algorithm>
#include <exception>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <fmt/format.h>

#include <external/resinsight/LibGeometry/cvfBoundingBoxTree.h>

namespace Opm {

namespace {

    /*
      The function trim_wgname() is used to trim the leading and trailing spaces
      away from the group and well arguments given in the WELSPECS and GRUPTREE
      keywords. If the deck argument contains a leading or trailing space that is
      treated as an input error, and the action taken is regulated by the setting
      ParseContext::PARSE_WGNAME_SPACE.

      Observe that the spaces are trimmed *unconditionally* - i.e. if the
      ParseContext::PARSE_WGNAME_SPACE setting is set to InputError::IGNORE that
      means that we do not inform the user about "our fix", but it is *not* possible
      to configure the parser to leave the spaces intact.
    */
    std::string trim_wgname(const DeckKeyword& keyword,
                            const std::string& wgname_arg,
                            const ParseContext& parseContext,
                            ErrorGuard& errors)
    {
        std::string wgname = trim_copy(wgname_arg);
        if (wgname != wgname_arg)  {
            const auto& location = keyword.location();
            std::string msg_fmt = fmt::format("Problem with keyword {{keyword}}\n"
                                              "In {{file}} line {{line}}\n"
                                              "Illegal space in {} when defining WELL/GROUP.", wgname_arg);
            parseContext.handleError(ParseContext::PARSE_WGNAME_SPACE, msg_fmt, location, errors);
        }
        return wgname;
    }

    void handleAQUCT(HandlerContext& handlerContext)
    {
        throw OpmInputError("AQUCT is not supported as SCHEDULE keyword", handlerContext.keyword.location());
    }

    void handleAQUFETP(HandlerContext& handlerContext)
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

    void handleWELTRAJ(HandlerContext& handlerContext)
    {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto wellnames = handlerContext.wellNames(wellNamePattern, false);

            for (const auto& name : wellnames) {
                auto well2 = handlerContext.state().wells.get(name);
                auto connections = std::make_shared<WellConnections>(WellConnections(well2.getConnections()));
                connections->loadWELTRAJ(record, handlerContext.grid, name, handlerContext.keyword.location());
                if (well2.updateConnections(connections, handlerContext.grid)) {
                    handlerContext.state().wells.update( well2 );
                    handlerContext.record_well_structure_change();
                }
                handlerContext.state().wellgroup_events().addEvent( name, ScheduleEvents::COMPLETION_CHANGE);
                const auto& md = connections->getMD();
                if (!std::is_sorted(std::begin(md), std::end(md))) {
                    auto msg = fmt::format("Well {} measured depth column is not strictly increasing", name);
                    throw OpmInputError(msg, handlerContext.keyword.location());
                }
            }
        }
        handlerContext.state().events().addEvent(ScheduleEvents::COMPLETION_CHANGE);
    }

    void handleDRSDT(HandlerContext& handlerContext)
    {
        const std::size_t numPvtRegions = handlerContext.static_schedule().m_runspec.tabdims().getNumPVTTables();
        std::vector<double> maximums(numPvtRegions);
        std::vector<std::string> options(numPvtRegions);
        for (const auto& record : handlerContext.keyword) {
            const auto& max = record.getItem<ParserKeywords::DRSDT::DRSDT_MAX>().getSIDouble(0);
            const auto& option = record.getItem<ParserKeywords::DRSDT::OPTION>().get< std::string >(0);
            std::fill(maximums.begin(), maximums.end(), max);
            std::fill(options.begin(), options.end(), option);
            auto& ovp = handlerContext.state().oilvap();
            OilVaporizationProperties::updateDRSDT(ovp, maximums, options);
        }
    }

    void handleDRSDTCON(HandlerContext& handlerContext)
    {
        const std::size_t numPvtRegions = handlerContext.static_schedule().m_runspec.tabdims().getNumPVTTables();
        std::vector<double> maximums(numPvtRegions);
        std::vector<std::string> options(numPvtRegions);
        for (const auto& record : handlerContext.keyword) {
            const auto& max = record.getItem<ParserKeywords::DRSDTCON::DRSDT_MAX>().getSIDouble(0);
            const auto& option = record.getItem<ParserKeywords::DRSDTCON::OPTION>().get< std::string >(0);
            std::fill(maximums.begin(), maximums.end(), max);
            std::fill(options.begin(), options.end(), option);
            auto& ovp = handlerContext.state().oilvap();
            OilVaporizationProperties::updateDRSDTCON(ovp, maximums, options);
        }
    }

    void handleDRSDTR(HandlerContext& handlerContext)
    {
        const std::size_t numPvtRegions = handlerContext.static_schedule().m_runspec.tabdims().getNumPVTTables();
        std::vector<double> maximums(numPvtRegions);
        std::vector<std::string> options(numPvtRegions);
        std::size_t pvtRegionIdx = 0;
        for (const auto& record : handlerContext.keyword) {
            const auto& max = record.getItem<ParserKeywords::DRSDTR::DRSDT_MAX>().getSIDouble(0);
            const auto& option = record.getItem<ParserKeywords::DRSDTR::OPTION>().get< std::string >(0);
            maximums[pvtRegionIdx] = max;
            options[pvtRegionIdx] = option;
            pvtRegionIdx++;
        }
        auto& ovp = handlerContext.state().oilvap();
        OilVaporizationProperties::updateDRSDT(ovp, maximums, options);
    }

    void handleDRVDT(HandlerContext& handlerContext)
    {
        const std::size_t numPvtRegions = handlerContext.static_schedule().m_runspec.tabdims().getNumPVTTables();
        std::vector<double> maximums(numPvtRegions);
        for (const auto& record : handlerContext.keyword) {
            const auto& max = record.getItem<ParserKeywords::DRVDTR::DRVDT_MAX>().getSIDouble(0);
            std::fill(maximums.begin(), maximums.end(), max);
            auto& ovp = handlerContext.state().oilvap();
            OilVaporizationProperties::updateDRVDT(ovp, maximums);
        }
    }

    void handleDRVDTR(HandlerContext& handlerContext)
    {
        std::size_t numPvtRegions = handlerContext.static_schedule().m_runspec.tabdims().getNumPVTTables();
        std::size_t pvtRegionIdx = 0;
        std::vector<double> maximums(numPvtRegions);
        for (const auto& record : handlerContext.keyword) {
            const auto& max = record.getItem<ParserKeywords::DRVDTR::DRVDT_MAX>().getSIDouble(0);
            maximums[pvtRegionIdx] = max;
            pvtRegionIdx++;
        }
        auto& ovp = handlerContext.state().oilvap();
        OilVaporizationProperties::updateDRVDT(ovp, maximums);
    }

    void handleEXIT(HandlerContext& handlerContext)
    {
        if (handlerContext.actionx_mode) {
            using ES = ParserKeywords::EXIT;
            int status = handlerContext.keyword.getRecord(0).getItem<ES::STATUS_CODE>().get<int>(0);
            OpmLog::info("Simulation exit with status: " +
                         std::to_string(status) +
                         " requested as part of ACTIONX at report_step: " +
                         std::to_string(handlerContext.currentStep));
            handlerContext.setExitCode(status);
        }
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
            OpmLog::warning(fmt::format("This version of flow is built without support for Python. "
                                        "Keyword PYACTION in file: {} line: {} is ignored.",
                                        loc.filename, loc.lineno));
            return;
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

    void handleRPTONLY(HandlerContext& handlerContext)
    {
        handlerContext.state().rptonly(true);
    }

    void handleRPTONLYO(HandlerContext& handlerContext)
    {
        handlerContext.state().rptonly(false);
    }

    void handleRPTSCHED(HandlerContext& handlerContext)
    {
        handlerContext.state().rpt_config.update( RPTConfig(handlerContext.keyword ));
        auto rst_config = handlerContext.state().rst_config();
        rst_config.update(handlerContext.keyword, handlerContext.parseContext, handlerContext.errors);
        handlerContext.state().rst_config.update(std::move(rst_config));
    }

    void handleRPTRST(HandlerContext& handlerContext)
    {
        auto rst_config = handlerContext.state().rst_config();
        rst_config.update(handlerContext.keyword, handlerContext.parseContext, handlerContext.errors);
        handlerContext.state().rst_config.update(std::move(rst_config));
    }

    /*
      We do not really handle the SAVE keyword, we just interpret it as: Write a
      normal restart file at this report step.
    */
    void handleSAVE(HandlerContext& handlerContext)
    {
        handlerContext.state().updateSAVE(true);
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

            tuning.THIONX = nondefault_or_previous_double(record2, "THIONX", tuning.THIONX);
            tuning.TRWGHT = nondefault_or_previous_int(record2, "TRWGHT", tuning.TRWGHT);
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
        }

        handlerContext.state().update_tuning( std::move( tuning ));
        handlerContext.state().events().addEvent(ScheduleEvents::TUNING_CHANGE);
    }

    void handleVAPPARS(HandlerContext& handlerContext)
    {
        for (const auto& record : handlerContext.keyword) {
            double vap1 = record.getItem("OIL_VAP_PROPENSITY").get< double >(0);
            double vap2 = record.getItem("OIL_DENSITY_PROPENSITY").get< double >(0);
            auto& ovp = handlerContext.state().oilvap();
            OilVaporizationProperties::updateVAPPARS(ovp, vap1, vap2);
        }
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

    void handleWCONHIST(HandlerContext& handlerContext)
    {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = handlerContext.wellNames(wellNamePattern, false);

            const Well::Status status = WellStatusFromString(record.getItem("STATUS").getTrimmedString(0));

            for (const auto& well_name : well_names) {
                handlerContext.updateWellStatus(well_name, status,
                                                handlerContext.keyword.location());

                std::optional<VFPProdTable::ALQ_TYPE> alq_type;
                auto well2 = handlerContext.state().wells.get( well_name );
                const bool switching_from_injector = !well2.isProducer();
                auto properties = std::make_shared<Well::WellProductionProperties>(well2.getProductionProperties());
                bool update_well = false;

                auto table_nr = record.getItem("VFP_TABLE").get< int >(0);
                if(record.getItem("VFP_TABLE").defaultApplied(0))
                    table_nr = properties->VFPTableNumber;

                if (table_nr != 0) {
                    const auto& vfpprod = handlerContext.state().vfpprod;
                    if (vfpprod.has(table_nr))
                        alq_type = handlerContext.state().vfpprod(table_nr).getALQType();
                    else {
                        std::string reason = fmt::format("Problem with well:{} VFP table: {} not defined", well_name, table_nr);
                        throw OpmInputError(reason, handlerContext.keyword.location());
                    }
                }
                double default_bhp;
                if (handlerContext.state().bhp_defaults.get().prod_target) {
                    default_bhp = *handlerContext.state().bhp_defaults.get().prod_target;
                } else {
                    default_bhp = UnitSystem::newMETRIC().to_si(UnitSystem::measure::pressure,
                                                                ParserKeywords::FBHPDEF::TARGET_BHP::defaultValue);
                }

                properties->handleWCONHIST(alq_type,
                                           default_bhp,
                                           handlerContext.static_schedule().m_unit_system, record);

                if (switching_from_injector) {
                    if (properties->bhp_hist_limit_defaulted) {
                        properties->setBHPLimit(default_bhp);
                    }

                    auto inj_props = std::make_shared<Well::WellInjectionProperties>(well2.getInjectionProperties());
                    inj_props->resetBHPLimit();
                    well2.updateInjection(inj_props);
                    update_well = true;
                    handlerContext.state().wellgroup_events().addEvent( well2.name(), ScheduleEvents::WELL_SWITCHED_INJECTOR_PRODUCER);
                }

                if (well2.updateProduction(properties))
                    update_well = true;

                if (well2.updatePrediction(false))
                    update_well = true;

                if (well2.updateHasProduced())
                    update_well = true;

                if (update_well) {
                    handlerContext.state().events().addEvent( ScheduleEvents::PRODUCTION_UPDATE );
                    handlerContext.state().wellgroup_events().addEvent( well2.name(), ScheduleEvents::PRODUCTION_UPDATE);
                    handlerContext.state().wells.update( well2 );
                }

                if (!well2.getAllowCrossFlow()) {
                    // The numerical content of the rate UDAValues is accessed unconditionally;
                    // since this is in history mode use of UDA values is not allowed anyway.
                    const auto& oil_rate = properties->OilRate;
                    const auto& water_rate = properties->WaterRate;
                    const auto& gas_rate = properties->GasRate;
                    if (oil_rate.zero() && water_rate.zero() && gas_rate.zero()) {
                        std::string msg =
                            "Well " + well2.name() + " is a history matched well with zero rate where crossflow is banned. " +
                            "This well will be closed at " + std::to_string(handlerContext.elapsed_seconds() / (60*60*24)) + " days";
                        OpmLog::note(msg);
                        handlerContext.updateWellStatus(well_name,  Well::Status::SHUT);
                    }
                }
            }
        }
    }

    void handleWCONPROD(HandlerContext& handlerContext)
    {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = handlerContext.wellNames(wellNamePattern, false);

            const Well::Status status = WellStatusFromString(record.getItem("STATUS").getTrimmedString(0));

            for (const auto& well_name : well_names) {
                bool update_well = handlerContext.updateWellStatus(well_name, status,
                                                                   handlerContext.keyword.location());
                std::optional<VFPProdTable::ALQ_TYPE> alq_type;
                auto well2 = handlerContext.state().wells.get( well_name );
                const bool switching_from_injector = !well2.isProducer();
                auto properties = std::make_shared<Well::WellProductionProperties>(well2.getProductionProperties());
                properties->clearControls();
                if (well2.isAvailableForGroupControl())
                    properties->addProductionControl(Well::ProducerCMode::GRUP);

                auto table_nr = record.getItem("VFP_TABLE").get< int >(0);
                if(record.getItem("VFP_TABLE").defaultApplied(0))
                    table_nr = properties->VFPTableNumber;

                if (table_nr != 0) {
                    const auto& vfpprod = handlerContext.state().vfpprod;
                    if (vfpprod.has(table_nr))
                        alq_type = handlerContext.state().vfpprod(table_nr).getALQType();
                    else {
                        std::string reason = fmt::format("Problem with well:{} VFP table: {} not defined", well_name, table_nr);
                        throw OpmInputError(reason, handlerContext.keyword.location());
                    }
                }

                double default_bhp_target;
                if (handlerContext.state().bhp_defaults.get().prod_target) {
                    default_bhp_target = *handlerContext.state().bhp_defaults.get().prod_target;
                } else {
                    default_bhp_target = UnitSystem::newMETRIC().to_si(UnitSystem::measure::pressure,
                                                                       ParserKeywords::WCONPROD::BHP::defaultValue.get<double>());
                }

                properties->handleWCONPROD(alq_type, default_bhp_target,
                                           handlerContext.static_schedule().m_unit_system,
                                           well_name, record);

                if (switching_from_injector) {
                    if (properties->bhp_hist_limit_defaulted) {
                        properties->setBHPLimit(default_bhp_target);
                    }
                    update_well = true;
                    handlerContext.state().wellgroup_events().addEvent( well2.name(), ScheduleEvents::WELL_SWITCHED_INJECTOR_PRODUCER);
                }

                if (well2.updateProduction(properties))
                    update_well = true;

                if (well2.updatePrediction(true))
                    update_well = true;

                if (well2.updateHasProduced())
                    update_well = true;

                if (well2.getStatus() == WellStatus::OPEN) {
                    handlerContext.state().wellgroup_events().addEvent(well2.name(), ScheduleEvents::REQUEST_OPEN_WELL);
                }

                if (update_well) {
                    handlerContext.state().events().addEvent( ScheduleEvents::PRODUCTION_UPDATE );
                    handlerContext.state().wellgroup_events().addEvent( well2.name(), ScheduleEvents::PRODUCTION_UPDATE);
                    handlerContext.state().wells.update( std::move(well2) );
                }

                auto udq_active = handlerContext.state().udq_active.get();
                if (properties->updateUDQActive(handlerContext.state().udq.get(), udq_active))
                    handlerContext.state().udq_active.update( std::move(udq_active));

                handlerContext.affected_well(well_name);
            }
        }
    }

    void handleWCONINJE(HandlerContext& handlerContext)
    {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = handlerContext.wellNames(wellNamePattern);

            const Well::Status status = WellStatusFromString(record.getItem("STATUS").getTrimmedString(0));

            for (const auto& well_name : well_names) {
                handlerContext.updateWellStatus(well_name, status,
                                                handlerContext.keyword.location());

                bool update_well = false;
                auto well2 = handlerContext.state().wells.get( well_name );

                auto injection = std::make_shared<Well::WellInjectionProperties>(well2.getInjectionProperties());
                auto previousInjectorType = injection->injectorType;

                double default_bhp_limit;
                const auto& usys = handlerContext.static_schedule().m_unit_system;
                if (handlerContext.state().bhp_defaults.get().inj_limit) {
                    default_bhp_limit = usys.from_si(UnitSystem::measure::pressure,
                                                     *handlerContext.state().bhp_defaults.get().inj_limit);
                } else {
                    default_bhp_limit = UnitSystem::newMETRIC().to_si(UnitSystem::measure::pressure,
                                                                      ParserKeywords::WCONINJE::BHP::defaultValue.get<double>());
                    default_bhp_limit = usys.from_si(UnitSystem::measure::pressure,
                                                     default_bhp_limit);
                }

                injection->handleWCONINJE(record, default_bhp_limit,
                                          well2.isAvailableForGroupControl(), well_name);

                const bool switching_from_producer = well2.isProducer();
                if (well2.updateInjection(injection))
                    update_well = true;

                if (switching_from_producer)
                    handlerContext.state().wellgroup_events().addEvent( well2.name(), ScheduleEvents::WELL_SWITCHED_INJECTOR_PRODUCER);

                if (well2.updatePrediction(true))
                    update_well = true;

                if (well2.updateHasInjected())
                    update_well = true;

                const bool crossFlow = well2.getAllowCrossFlow();
                if (update_well) {
                    handlerContext.state().events().addEvent(ScheduleEvents::INJECTION_UPDATE);
                    handlerContext.state().wellgroup_events().addEvent( well_name, ScheduleEvents::INJECTION_UPDATE);
                    if(previousInjectorType != injection->injectorType)
                        handlerContext.state().wellgroup_events().addEvent( well_name, ScheduleEvents::INJECTION_TYPE_CHANGED);
                    handlerContext.state().wells.update( std::move(well2) );
                }

                // if the well has zero surface rate limit or reservior rate limit, while does not allow crossflow,
                // it should be turned off.
                if ( ! crossFlow ) {
                    std::string msg =
                        "Well " + well_name + " is an injector with zero rate where crossflow is banned. " +
                        "This well will be closed at " + std::to_string(handlerContext.elapsed_seconds() / (60*60*24)) + " days";

                    if (injection->surfaceInjectionRate.is<double>()) {
                        if (injection->hasInjectionControl(Well::InjectorCMode::RATE) && injection->surfaceInjectionRate.zero()) {
                            OpmLog::note(msg);
                            handlerContext.updateWellStatus(well_name, Well::Status::SHUT);
                        }
                    }

                    if (injection->reservoirInjectionRate.is<double>()) {
                        if (injection->hasInjectionControl(Well::InjectorCMode::RESV) && injection->reservoirInjectionRate.zero()) {
                            OpmLog::note(msg);
                            handlerContext.updateWellStatus(well_name, Well::Status::SHUT);
                        }
                    }
                }

                if (handlerContext.state().wells.get( well_name ).getStatus() == Well::Status::OPEN) {
                    handlerContext.state().wellgroup_events().addEvent(well_name, ScheduleEvents::REQUEST_OPEN_WELL);
                }

                auto udq_active = handlerContext.state().udq_active.get();
                if (injection->updateUDQActive(handlerContext.state().udq.get(), udq_active))
                    handlerContext.state().udq_active.update( std::move(udq_active) );

                handlerContext.affected_well(well_name);
            }
        }
    }

    void handleWCONINJH(HandlerContext& handlerContext)
    {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = handlerContext.wellNames(wellNamePattern, false);
            const Well::Status status = WellStatusFromString( record.getItem("STATUS").getTrimmedString(0));

            for (const auto& well_name : well_names) {
                handlerContext.updateWellStatus(well_name, status,
                                                handlerContext.keyword.location());
                bool update_well = false;
                auto well2 = handlerContext.state().wells.get( well_name );
                auto injection = std::make_shared<Well::WellInjectionProperties>(well2.getInjectionProperties());
                auto previousInjectorType = injection->injectorType;

                double default_bhp_limit;
                if (handlerContext.state().bhp_defaults.get().inj_limit) {
                    default_bhp_limit = *handlerContext.state().bhp_defaults.get().inj_limit;
                } else {
                    default_bhp_limit = UnitSystem::newMETRIC().to_si(UnitSystem::measure::pressure,
                                                                      6891.2);
                }

                injection->handleWCONINJH(record, default_bhp_limit,
                                          well2.isProducer(), well_name,
                                          handlerContext.keyword.location());

                const bool switching_from_producer = well2.isProducer();
                if (well2.updateInjection(injection))
                    update_well = true;

                if (switching_from_producer)
                    handlerContext.state().wellgroup_events().addEvent( well2.name(), ScheduleEvents::WELL_SWITCHED_INJECTOR_PRODUCER);

                if (well2.updatePrediction(false))
                    update_well = true;

                if (well2.updateHasInjected())
                    update_well = true;

                const bool crossFlow = well2.getAllowCrossFlow();
                if (update_well) {
                    handlerContext.state().events().addEvent( ScheduleEvents::INJECTION_UPDATE );
                    handlerContext.state().wellgroup_events().addEvent( well_name, ScheduleEvents::INJECTION_UPDATE);
                    if(previousInjectorType != injection->injectorType)
                        handlerContext.state().wellgroup_events().addEvent( well_name, ScheduleEvents::INJECTION_TYPE_CHANGED);
                    handlerContext.state().wells.update( std::move(well2) );
                }

                if ( ! crossFlow && (injection->surfaceInjectionRate.zero())) {
                    std::string msg =
                        "Well " + well_name + " is an injector with zero rate where crossflow is banned. " +
                        "This well will be closed at " + std::to_string(handlerContext.elapsed_seconds() / (60*60*24)) + " days";
                    OpmLog::note(msg);
                    handlerContext.updateWellStatus(well_name, Well::Status::SHUT);
                }
            }
        }
    }

    void handleWELOPEN(HandlerContext& handlerContext)
    {
        const auto& keyword = handlerContext.keyword;

        auto conn_defaulted = []( const DeckRecord& rec ) {
            auto defaulted = []( const DeckItem& item ) {
                return item.defaultApplied( 0 );
            };

            return std::all_of( rec.begin() + 2, rec.end(), defaulted );
        };

        constexpr auto open = Well::Status::OPEN;

        for (const auto& record : keyword) {
            const auto& wellNamePattern = record.getItem( "WELL" ).getTrimmedString(0);
            const auto& status_str = record.getItem( "STATUS" ).getTrimmedString( 0 );
            const auto well_names = handlerContext.wellNames(wellNamePattern);

            /* if all records are defaulted or just the status is set, only
             * well status is updated
             */
            if (conn_defaulted(record)) {
                const auto new_well_status = WellStatusFromString(status_str);
                for (const auto& wname : well_names) {
                    const auto did_update_well_status =
                        handlerContext.updateWellStatus(wname, new_well_status);

                    handlerContext.affected_well(wname);

                    if (did_update_well_status) {
                        handlerContext.record_well_structure_change();
                    }

                    if (did_update_well_status && (new_well_status == open)) {
                        // Record possible well injection/production status change
                        auto well2 = handlerContext.state().wells.get(wname);

                        const auto did_flow_update =
                            (well2.isProducer() && well2.updateHasProduced())
                            ||
                            (well2.isInjector() && well2.updateHasInjected());

                        if (did_flow_update) {
                            handlerContext.state().wells.update(std::move(well2));
                        }
                    }

                    if (new_well_status == open) {
                        handlerContext.state().wellgroup_events().addEvent( wname, ScheduleEvents::REQUEST_OPEN_WELL);
                    }
                }
                continue;
            }

            /*
              Some of the connection information has been entered, in this case
              we *only* update the status of the connections, and not the well
              itself. Unless all connections are shut - then the well is also
              shut.
             */
            for (const auto& wname : well_names) {
                {
                    auto well = handlerContext.state().wells.get(wname);
                    handlerContext.state().wells.update( std::move(well) );
                }

                const auto connection_status = Connection::StateFromString( status_str );
                {
                    auto well = handlerContext.state().wells.get(wname);
                    well.handleWELOPENConnections(record, connection_status);
                    handlerContext.state().wells.update( std::move(well) );
                }

                handlerContext.affected_well(wname);
                handlerContext.record_well_structure_change();

                handlerContext.state().events().addEvent(ScheduleEvents::COMPLETION_CHANGE);
            }
        }
    }

    void handleWELSPECS(HandlerContext& handlerContext)
    {
        using Kw = ParserKeywords::WELSPECS;

        auto getTrimmedName = [&handlerContext](const auto& item)
        {
            return trim_wgname(handlerContext.keyword,
                               item.template get<std::string>(0),
                               handlerContext.parseContext,
                               handlerContext.errors);
        };

        auto fieldWells = std::vector<std::string>{};
        for (const auto& record : handlerContext.keyword) {
            if (const auto fip_region_number = record.getItem<Kw::FIP_REGION>().get<int>(0);
                fip_region_number != Kw::FIP_REGION::defaultValue)
            {
                const auto& location = handlerContext.keyword.location();
                const auto msg = fmt::format("Non-defaulted FIP region {} in WELSPECS keyword "
                                             "in file {} line {} is not supported. "
                                             "Reset to default value {}.",
                                             fip_region_number,
                                             location.filename,
                                             location.lineno,
                                             Kw::FIP_REGION::defaultValue);
                OpmLog::warning(msg);
            }

            if (const auto& density_calc_type = record.getItem<Kw::DENSITY_CALC>().get<std::string>(0);
                density_calc_type != Kw::DENSITY_CALC::defaultValue)
            {
                const auto& location = handlerContext.keyword.location();
                const auto msg = fmt::format("Non-defaulted density calculation method '{}' "
                                             "in WELSPECS keyword in file {} line {} is "
                                             "not supported. Reset to default value {}.",
                                             density_calc_type,
                                             location.filename,
                                             location.lineno,
                                             Kw::DENSITY_CALC::defaultValue);
                OpmLog::warning(msg);
            }

            const auto wellName = getTrimmedName(record.getItem<Kw::WELL>());
            const auto groupName = getTrimmedName(record.getItem<Kw::GROUP>());

            // We might get here from an ACTIONX context, or we might get
            // called on a well (list) template, to reassign certain well
            // properties--e.g, the well's controlling group--so check if
            // 'wellName' matches any existing well names through pattern
            // matching before treating the wellName as a simple well name.
            //
            // An empty list of well names is okay since that means we're
            // creating a new well in this case.
            const auto allowEmptyWellList = true;
            const auto existingWells = handlerContext.wellNames(wellName, allowEmptyWellList);

            if (groupName == "FIELD") {
                if (existingWells.empty()) {
                    fieldWells.push_back(wellName);
                }
                else {
                    for (const auto& existingWell : existingWells) {
                        fieldWells.push_back(existingWell);
                    }
                }
            }

            if (! handlerContext.state().groups.has(groupName)) {
                handlerContext.addGroup(groupName);
            }

            if (existingWells.empty()) {
                // 'wellName' does not match any existing wells.  Create a
                // new Well object for this well.
                handlerContext.welspecsCreateNewWell(record,
                                                     wellName,
                                                     groupName);
            }
            else {
                // 'wellName' matches one or more existing wells.  Assign
                // new properties for those wells.
                handlerContext.welspecsUpdateExistingWells(record,
                                                           existingWells,
                                                           groupName);
            }
        }

        if (! fieldWells.empty()) {
            std::sort(fieldWells.begin(), fieldWells.end());
            fieldWells.erase(std::unique(fieldWells.begin(), fieldWells.end()),
                             fieldWells.end());

            const auto* plural = (fieldWells.size() == 1) ? "" : "s";

            const auto msg_fmt = fmt::format(R"(Well{0} parented directly to 'FIELD'; this is allowed but discouraged.
Well{0} entered with 'FIELD' parent group:
 * {1})", plural, fmt::join(fieldWells, "\n * "));

            handlerContext.parseContext.handleError(ParseContext::SCHEDULE_WELL_IN_FIELD_GROUP,
                                                    msg_fmt,
                                                    handlerContext.keyword.location(),
                                                    handlerContext.errors);
        }

        if (! handlerContext.keyword.empty()) {
            handlerContext.record_well_structure_change();
        }
    }

    /*
      The documentation for the WELTARG keyword says that the well
      must have been fully specified and initialized using one of the
      WCONxxxx keywords prior to modifying the well using the WELTARG
      keyword.

      The following implementation of handling the WELTARG keyword
      does not check or enforce in any way that this is done (i.e. it
      is not checked or verified that the well is initialized with any
      WCONxxxx keyword).

      Update: See the discussion following the definitions of the SI factors, due
      to a bad design we currently need the well to be specified with
      WCONPROD / WCONHIST before WELTARG is applied, if not the units for the
      rates will be wrong.
    */
    void handleWELTARG(HandlerContext& handlerContext)
    {
        const double SiFactorP = handlerContext.static_schedule().m_unit_system.parse("Pressure").getSIScaling();
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = handlerContext.wellNames(wellNamePattern);
            if (well_names.empty()) {
                handlerContext.invalidNamePattern( wellNamePattern);
            }

            const auto cmode = WellWELTARGCModeFromString(record.getItem("CMODE").getTrimmedString(0));
            const auto new_arg = record.getItem("NEW_VALUE").get<UDAValue>(0);

            for (const auto& well_name : well_names) {
                auto well2 = handlerContext.state().wells.get(well_name);
                bool update = false;
                if (well2.isProducer()) {
                    auto prop = std::make_shared<Well::WellProductionProperties>(well2.getProductionProperties());
                    prop->handleWELTARG(cmode, new_arg, SiFactorP);
                    update = well2.updateProduction(prop);
                    if (cmode == Well::WELTARGCMode::GUID)
                        update |= well2.updateWellGuideRate(new_arg.get<double>());

                    auto udq_active = handlerContext.state().udq_active.get();
                    if (prop->updateUDQActive(handlerContext.state().udq.get(), cmode, udq_active))
                        handlerContext.state().udq_active.update( std::move(udq_active));
                }
                else {
                    auto inj = std::make_shared<Well::WellInjectionProperties>(well2.getInjectionProperties());
                    inj->handleWELTARG(cmode, new_arg, SiFactorP);
                    update = well2.updateInjection(inj);
                    if (cmode == Well::WELTARGCMode::GUID)
                        update |= well2.updateWellGuideRate(new_arg.get<double>());

                    auto udq_active = handlerContext.state().udq_active.get();
                    if (inj->updateUDQActive(handlerContext.state().udq.get(), cmode, udq_active))
                        handlerContext.state().udq_active.update(std::move(udq_active));
                }

                if (update)
                {
                    if (well2.isProducer()) {
                        handlerContext.state().wellgroup_events().addEvent( well_name, ScheduleEvents::PRODUCTION_UPDATE);
                        handlerContext.state().events().addEvent( ScheduleEvents::PRODUCTION_UPDATE );
                    } else {
                        handlerContext.state().wellgroup_events().addEvent( well_name, ScheduleEvents::INJECTION_UPDATE);
                        handlerContext.state().events().addEvent( ScheduleEvents::INJECTION_UPDATE );
                    }
                    handlerContext.state().wells.update( std::move(well2) );
                }

                handlerContext.affected_well(well_name);
            }
        }
    }

    void handleWHISTCTL(HandlerContext& handlerContext)
    {
        const auto& record = handlerContext.keyword.getRecord(0);
        const std::string& cmodeString = record.getItem("CMODE").getTrimmedString(0);
        const auto controlMode = WellProducerCModeFromString(cmodeString);

        if (controlMode != Well::ProducerCMode::NONE) {
            if (!Well::WellProductionProperties::effectiveHistoryProductionControl(controlMode) ) {
                std::string msg = "The WHISTCTL keyword specifies an un-supported control mode " + cmodeString
                    + ", which makes WHISTCTL keyword not affect the simulation at all";
                OpmLog::warning(msg);
            } else
                handlerContext.state().update_whistctl( controlMode );
        }

        const std::string bhp_terminate = record.getItem("BPH_TERMINATE").getTrimmedString(0);
        if (bhp_terminate == "YES") {
            std::string msg_fmt = "Problem with {keyword}\n"
                                  "In {file} line {line}\n"
                                  "Setting item 2 in {keyword} to 'YES' to stop the run is not supported";
            handlerContext.parseContext.handleError( ParseContext::UNSUPPORTED_TERMINATE_IF_BHP , msg_fmt, handlerContext.keyword.location(), handlerContext.errors );
        }

        for (const auto& well_ref : handlerContext.state().wells()) {
            auto well2 = well_ref.get();
            auto prop = std::make_shared<Well::WellProductionProperties>(well2.getProductionProperties());

            if (prop->whistctl_cmode != controlMode) {
                prop->whistctl_cmode = controlMode;
                well2.updateProduction(prop);
                handlerContext.state().wells.update( std::move(well2) );
            }
        }
    }

    void handleWLIST(HandlerContext& handlerContext)
    {
        const std::string legal_actions = "NEW:ADD:DEL:MOV";
        for (const auto& record : handlerContext.keyword) {
            const std::string& name = record.getItem("NAME").getTrimmedString(0);
            const std::string& action = record.getItem("ACTION").getTrimmedString(0);
            const std::vector<std::string>& well_args = record.getItem("WELLS").getData<std::string>();
            std::vector<std::string> wells;
            auto new_wlm = handlerContext.state().wlist_manager.get();

            if (legal_actions.find(action) == std::string::npos)
                throw std::invalid_argument("The action:" + action + " is not recognized.");

            for (const auto& well_arg : well_args) {
                // does not use overload for context to avoid throw
                const auto names = handlerContext.wellNames(well_arg, true);
                if (names.empty() && well_arg.find("*") == std::string::npos) {
                    std::string msg_fmt = "Problem with {keyword}\n"
                                          "In {file} line {line}\n"
                                          "The well '" + well_arg + "' has not been defined with WELSPECS and will not be added to the list.";
                    const auto& parseContext = handlerContext.parseContext;
                    parseContext.handleError(ParseContext::SCHEDULE_INVALID_NAME, msg_fmt, handlerContext.keyword.location(),handlerContext.errors);
                    continue;
                }

                std::move(names.begin(), names.end(), std::back_inserter(wells));
            }

            if (name[0] != '*')
                throw std::invalid_argument("The list name in WLIST must start with a '*'");

            if (action == "NEW") {
                new_wlm.newList(name, wells);
            }

            if (!new_wlm.hasList(name))
                throw std::invalid_argument("Invalid well list: " + name);

            if (action == "MOV") {
                for (const auto& well : wells) {
                    new_wlm.delWell(well);
                }
            }

            if (action == "DEL") {
                for (const auto& well : wells) {
                    new_wlm.delWListWell(well, name);
                }
            } else if (action != "NEW"){
                for (const auto& well : wells) {
                    new_wlm.addWListWell(well, name);
                }
            }
            handlerContext.state().wlist_manager.update( std::move(new_wlm) );
        }
    }

    void handleWTEST(HandlerContext& handlerContext)
    {
        auto new_config = handlerContext.state().wtest_config.get();
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = handlerContext.wellNames(wellNamePattern);
            if (well_names.empty()) {
                handlerContext.invalidNamePattern(wellNamePattern);
            }

            const double test_interval = record.getItem("INTERVAL").getSIDouble(0);
            const std::string& reasons = record.getItem("REASON").get<std::string>(0);
            const int num_test = record.getItem("TEST_NUM").get<int>(0);
            const double startup_time = record.getItem("START_TIME").getSIDouble(0);

            for (const auto& well_name : well_names) {
                if (reasons.empty())
                    new_config.drop_well(well_name);
                else
                    new_config.add_well(well_name, reasons, test_interval, num_test, startup_time, handlerContext.currentStep);
            }
        }
        handlerContext.state().wtest_config.update( std::move(new_config) );
    }

    void handleWPAVE(HandlerContext& handlerContext)
    {
        auto wpave = PAvg( handlerContext.keyword.getRecord(0) );

        if (wpave.inner_weight() > 1.0) {
            const auto reason =
                fmt::format("Inner block weighting F1 "
                            "must not exceed 1.0. Got {}",
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
                            "must be between zero and one "
                            "inclusive. Got {} instead.",
                            wpave.conn_weight());

            throw OpmInputError {
                reason, handlerContext.keyword.location()
            };
        }

        for (const auto& wname : handlerContext.state().well_order()) {
            const auto& well = handlerContext.state().wells.get(wname);
            if (well.pavg() != wpave) {
                auto new_well = handlerContext.state().wells.get(wname);
                new_well.updateWPAVE(wpave);
                handlerContext.state().wells.update(std::move(new_well));
            }
        }

        handlerContext.state().pavg.update(std::move(wpave));
    }

    void handleWPAVEDEP(HandlerContext& handlerContext)
    {
        for (const auto& record : handlerContext.keyword) {
            const std::string& wellNamePattern = record.getItem<ParserKeywords::WPAVEDEP::WELL>().getTrimmedString(0);
            const auto well_names = handlerContext.wellNames(wellNamePattern, false);

            if (well_names.empty()) {
                handlerContext.invalidNamePattern(wellNamePattern);
            }

            const auto& item = record.getItem<ParserKeywords::WPAVEDEP::REFDEPTH>();
            if (item.hasValue(0)) {
                auto ref_depth = item.getSIDouble(0);
                for (const auto& well_name : well_names) {
                    auto well = handlerContext.state().wells.get(well_name);
                    well.updateWPaveRefDepth( ref_depth );
                    handlerContext.state().wells.update( std::move(well) );
                }
            }
        }
    }

    void handleWRFT(HandlerContext& handlerContext)
    {
        auto new_rft = handlerContext.state().rft_config();

        for (const auto& record : handlerContext.keyword) {
            const auto& item = record.getItem<ParserKeywords::WRFT::WELL>();
            if (! item.hasValue(0)) {
                continue;
            }

            const auto wellNamePattern = record.getItem<ParserKeywords::WRFT::WELL>().getTrimmedString(0);
            const auto well_names = handlerContext.wellNames(wellNamePattern, false);

            if (well_names.empty()) {
                handlerContext.invalidNamePattern(wellNamePattern);
            }

            for (const auto& well_name : well_names) {
                new_rft.update(well_name, RFTConfig::RFT::YES);
            }
        }

        new_rft.first_open(true);

        handlerContext.state().rft_config.update(std::move(new_rft));
    }

    void handleWRFTPLT(HandlerContext& handlerContext)
    {
        auto new_rft = handlerContext.state().rft_config();

        const auto rftKey = [](const DeckItem& key)
        {
            return RFTConfig::RFTFromString(key.getTrimmedString(0));
        };

        const auto pltKey = [](const DeckItem& key)
        {
            return RFTConfig::PLTFromString(key.getTrimmedString(0));
        };

        for (const auto& record : handlerContext.keyword) {
            const auto wellNamePattern = record.getItem<ParserKeywords::WRFTPLT::WELL>().getTrimmedString(0);
            const auto well_names = handlerContext.wellNames(wellNamePattern, false);

            if (well_names.empty()) {
                handlerContext.invalidNamePattern(wellNamePattern);
                continue;
            }

            const auto RFTKey = rftKey(record.getItem<ParserKeywords::WRFTPLT::OUTPUT_RFT>());
            const auto PLTKey = pltKey(record.getItem<ParserKeywords::WRFTPLT::OUTPUT_PLT>());
            const auto SEGKey = pltKey(record.getItem<ParserKeywords::WRFTPLT::OUTPUT_SEGMENT>());

            for (const auto& well_name : well_names) {
                new_rft.update(well_name, RFTKey);
                new_rft.update(well_name, PLTKey);
                new_rft.update_segment(well_name, SEGKey);
            }
        }

        handlerContext.state().rft_config.update(std::move(new_rft));
    }

}

const KeywordHandlers& KeywordHandlers::getInstance()
{
    static KeywordHandlers instance{};
    return instance;
}

KeywordHandlers::KeywordHandlers()
{
    handler_functions = {
        { "AQUCT",    &handleAQUCT      },
        { "AQUFETP",  &handleAQUFETP    },
        { "AQUFLUX",  &handleAQUFLUX    },
        { "BCPROP",   &handleBCProp     },
        { "BOX",      &handleGEOKeyword },
        { "DRSDT"   , &handleDRSDT      },
        { "DRSDTCON", &handleDRSDTCON   },
        { "DRSDTR"  , &handleDRSDTR     },
        { "DRVDT"   , &handleDRVDT      },
        { "DRVDTR"  , &handleDRVDTR     },
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
        { "RPTONLY" , &handleRPTONLY    },
        { "RPTONLYO", &handleRPTONLYO   },
        { "RPTRST"  , &handleRPTRST     },
        { "RPTSCHED", &handleRPTSCHED   },
        { "SAVE"    , &handleSAVE       },
        { "SUMTHIN" , &handleSUMTHIN    },
        { "TUNING"  , &handleTUNING     },
        { "VAPPARS" , &handleVAPPARS    },
        { "VFPINJ"  , &handleVFPINJ     },
        { "VFPPROD" , &handleVFPPROD    },
        { "WCONHIST", &handleWCONHIST   },
        { "WCONINJE", &handleWCONINJE   },
        { "WCONINJH", &handleWCONINJH   },
        { "WCONPROD", &handleWCONPROD   },
        { "WELOPEN" , &handleWELOPEN    },
        { "WELSPECS", &handleWELSPECS   },
        { "WELTARG" , &handleWELTARG    },
        { "WELTRAJ" , &handleWELTRAJ    },
        { "WHISTCTL", &handleWHISTCTL   },
        { "WLIST"   , &handleWLIST      },
        { "WPAVE"   , &handleWPAVE      },
        { "WPAVEDEP", &handleWPAVEDEP   },
        { "WRFT"    , &handleWRFT       },
        { "WRFTPLT" , &handleWRFTPLT    },
        { "WTEST"   , &handleWTEST      },
    };

    for (const auto& handlerFactory : {getGasLiftOptHandlers,
                                       getGroupHandlers,
                                       getGuideRateHandlers,
                                       getMSWHandlers,
                                       getNetworkHandlers,
                                       getUDQHandlers,
                                       getWellCompletionHandlers,
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
