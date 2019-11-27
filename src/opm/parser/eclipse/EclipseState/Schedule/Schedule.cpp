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

#include <fnmatch.h>
#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <opm/common/OpmLog/LogUtil.hpp>

#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckSection.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/G.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/L.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/N.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/V.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/W.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionX.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionResult.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicVector.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Events.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/SpiralICD.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/updatingConnectionsWithSegments.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/Valve.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/WellSegments.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/OilVaporizationProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQActive.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Tuning.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WList.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WListManager.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellFoamProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellInjectionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellPolymerProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellProductionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellBrineProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellConnections.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/Units/Dimension.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>

#include "Well/injection.hpp"

namespace Opm {


namespace {

    bool name_match(const std::string& pattern, const std::string& name) {
        int flags = 0;
        return (fnmatch(pattern.c_str(), name.c_str(), flags) == 0);
    }


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

    std::string trim_wgname(const DeckKeyword& keyword, const std::string& wgname_arg, const ParseContext& parseContext, ErrorGuard errors) {
        std::string wgname = boost::algorithm::trim_copy(wgname_arg);
        if (wgname != wgname_arg)  {
            const auto& location = keyword.location();
            std::string msg = "Illegal space: \"" + wgname_arg + "\" found when defining WELL/GROUP in keyword: " + keyword.name() + " at " + location.filename + ":" + std::to_string(location.lineno);
            parseContext.handleError(ParseContext::PARSE_WGNAME_SPACE, msg, errors);
        }
        return wgname;
    }


}

    Schedule::Schedule( const Deck& deck,
                        const EclipseGrid& grid,
                        const FieldPropsManager& fp,
                        const Eclipse3DProperties& eclipseProperties,
                        const Runspec &runspec,
                        const ParseContext& parseContext,
                        ErrorGuard& errors) :
        m_timeMap( deck ),
        m_oilvaporizationproperties( this->m_timeMap, OilVaporizationProperties(runspec.tabdims().getNumPVTTables()) ),
        m_events( this->m_timeMap ),
        m_modifierDeck( this->m_timeMap, Deck{} ),
        m_tuning( this->m_timeMap ),
        m_messageLimits( this->m_timeMap ),
        m_runspec( runspec ),
        wtest_config(this->m_timeMap, std::make_shared<WellTestConfig>() ),
        wlist_manager( this->m_timeMap, std::make_shared<WListManager>()),
        udq_config(this->m_timeMap, std::make_shared<UDQConfig>(deck)),
        udq_active(this->m_timeMap, std::make_shared<UDQActive>()),
        guide_rate_config(this->m_timeMap, std::make_shared<GuideRateConfig>()),
        gconsale(this->m_timeMap, std::make_shared<GConSale>() ),
        gconsump(this->m_timeMap, std::make_shared<GConSump>() ),
        global_whistctl_mode(this->m_timeMap, Well::ProducerCMode::CMODE_UNDEFINED),
        m_actions(this->m_timeMap, std::make_shared<Action::Actions>()),
        rft_config(this->m_timeMap),
        m_nupcol(this->m_timeMap, ParserKeywords::NUPCOL::NUM_ITER::defaultValue)
    {
        addGroup( "FIELD", 0, deck.getActiveUnitSystem());

        /*
          We can have the MESSAGES keyword anywhere in the deck, we
          must therefor also scan the part of the deck prior to the
          SCHEDULE section to initialize valid MessageLimits object.
        */
        for (size_t keywordIdx = 0; keywordIdx < deck.size(); ++keywordIdx) {
            const auto& keyword = deck.getKeyword(keywordIdx);
            if (keyword.name() == "SCHEDULE")
                break;

            if (keyword.name() == "MESSAGES")
                handleMESSAGES(keyword, 0);
        }

        if (DeckSection::hasSCHEDULE(deck))
            iterateScheduleSection( parseContext, errors, SCHEDULESection( deck ), grid, fp, eclipseProperties );
    }


    template <typename T>
    Schedule::Schedule( const Deck& deck,
                        const EclipseGrid& grid,
                        const FieldPropsManager& fp,
                        const Eclipse3DProperties& eclipseProperties,
                        const Runspec &runspec,
                        const ParseContext& parseContext,
                        T&& errors) :
        Schedule(deck, grid, fp, eclipseProperties, runspec, parseContext, errors)
    {}


    Schedule::Schedule( const Deck& deck,
                        const EclipseGrid& grid,
                        const FieldPropsManager& fp,
                        const Eclipse3DProperties& eclipseProperties,
                        const Runspec &runspec) :
        Schedule(deck, grid, fp, eclipseProperties, runspec, ParseContext(), ErrorGuard())
    {}


    Schedule::Schedule(const Deck& deck, const EclipseState& es, const ParseContext& parse_context, ErrorGuard& errors) :
        Schedule(deck,
                 es.getInputGrid(),
                 es.fieldProps(),
                 es.get3DProperties(),
                 es.runspec(),
                 parse_context,
                 errors)
    {}



    template <typename T>
    Schedule::Schedule(const Deck& deck, const EclipseState& es, const ParseContext& parse_context, T&& errors) :
        Schedule(deck,
                 es.getInputGrid(),
                 es.fieldProps(),
                 es.get3DProperties(),
                 es.runspec(),
                 parse_context,
                 errors)
    {}


    Schedule::Schedule(const Deck& deck, const EclipseState& es) :
        Schedule(deck, es, ParseContext(), ErrorGuard())
    {}



    Schedule::Schedule(const TimeMap& timeMap,
                       const WellMap& wellsStatic,
                       const GroupMap& group,
                       const DynamicState<OilVaporizationProperties>& oilVapProps,
                       const Events& events,
                       const DynamicVector<Deck>& modifierDeck,
                       const Tuning& tuning,
                       const MessageLimits& messageLimits,
                       const Runspec& runspec,
                       const VFPProdMap& vfpProdTables,
                       const VFPInjMap& vfpInjTables,
                       const DynamicState<std::shared_ptr<WellTestConfig>>& wtestConfig,
                       const DynamicState<std::shared_ptr<WListManager>>& wListManager,
                       const DynamicState<std::shared_ptr<UDQConfig>>& udqConfig,
                       const DynamicState<std::shared_ptr<UDQActive>>& udqActive,
                       const DynamicState<std::shared_ptr<GuideRateConfig>>& guideRateConfig,
                       const DynamicState<std::shared_ptr<GConSale>>& gconSale,
                       const DynamicState<std::shared_ptr<GConSump>>& gconSump,
                       const DynamicState<Well::ProducerCMode>& globalWhistCtlMode,
                       const DynamicState<std::shared_ptr<Action::Actions>>& actions,
                       const RFTConfig& rftconfig,
                       const DynamicState<int>& nupCol,
                       const std::map<std::string,Events>& wellGroupEvents) :
        m_timeMap(timeMap),
        wells_static(wellsStatic),
        groups(group),
        m_oilvaporizationproperties(oilVapProps),
        m_events(events),
        m_modifierDeck(modifierDeck),
        m_tuning(tuning),
        m_messageLimits(messageLimits),
        m_runspec(runspec),
        vfpprod_tables(vfpProdTables),
        vfpinj_tables(vfpInjTables),
        wtest_config(wtestConfig),
        wlist_manager(wListManager),
        udq_config(udqConfig),
        udq_active(udqActive),
        guide_rate_config(guideRateConfig),
        gconsale(gconSale),
        gconsump(gconSump),
        global_whistctl_mode(globalWhistCtlMode),
        m_actions(actions),
        rft_config(rftconfig),
        m_nupcol(nupCol),
        wellgroup_events(wellGroupEvents)
    {}



    std::time_t Schedule::getStartTime() const {
        return this->posixStartTime( );
    }

    time_t Schedule::posixStartTime() const {
        return m_timeMap.getStartTime( 0 );
    }

    time_t Schedule::posixEndTime() const {
        return this->m_timeMap.getEndTime();
    }


    void Schedule::handleKeyword(size_t& currentStep,
                                 const SCHEDULESection& section,
                                 size_t keywordIdx,
                                 const DeckKeyword& keyword,
                                 const ParseContext& parseContext,
                                 ErrorGuard& errors,
                                 const EclipseGrid& grid,
                                 const FieldPropsManager& fp,
                                 const Eclipse3DProperties& eclipseProperties,
                                 const UnitSystem& unit_system,
                                 std::vector<std::pair<const DeckKeyword*, size_t > >& rftProperties) {
    /*
      geoModifiers is a list of geo modifiers which can be found in the schedule
      section. This is only partly supported, support is indicated by the bool
      value. The keywords which are supported will be assembled in a per-timestep
      'minideck', whereas ParseContext::UNSUPPORTED_SCHEDULE_GEO_MODIFIER will be
      consulted for the others.
    */

        const std::map<std::string,bool> geoModifiers = {{"MULTFLT"  , true},
                                                         {"MULTPV"   , false},
                                                         {"MULTX"    , false},
                                                         {"MULTX-"   , false},
                                                         {"MULTY"    , false},
                                                         {"MULTY-"   , false},
                                                         {"MULTZ"    , false},
                                                         {"MULTZ-"   , false},
                                                         {"MULTREGT" , false},
                                                         {"MULTR"    , false},
                                                         {"MULTR-"   , false},
                                                         {"MULTSIG"  , false},
                                                         {"MULTSIGV" , false},
                                                         {"MULTTHT"  , false},
                                                         {"MULTTHT-" , false}};

        if (keyword.name() == "DATES") {
            checkIfAllConnectionsIsShut(currentStep);
            currentStep += keyword.size();
        }

        else if (keyword.name() == "TSTEP") {
            checkIfAllConnectionsIsShut(currentStep);
            currentStep += keyword.getRecord(0).getItem(0).data_size(); // This is a bit weird API.
        }

        else if (keyword.name() == "UDQ")
            handleUDQ(keyword, currentStep);

        else if (keyword.name() == "WLIST")
            handleWLIST( keyword, currentStep );

        else if (keyword.name() == "WELSPECS")
            handleWELSPECS( section, keywordIdx, currentStep, unit_system, parseContext, errors);

        else if (keyword.name() == "WHISTCTL")
            handleWHISTCTL(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WCONHIST")
            handleWCONHIST(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WCONPROD")
            handleWCONPROD(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WCONINJE")
            handleWCONINJE(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WFOAM")
            handleWFOAM(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WPOLYMER")
            handleWPOLYMER(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WSALT")
            handleWSALT(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WSOLVENT")
            handleWSOLVENT(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WTRACER")
            handleWTRACER(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WTEST")
            handleWTEST(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WTEMP")
            handleWTEMP(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WPMITAB")
            handleWPMITAB(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WSKPTAB")
            handleWSKPTAB(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WINJTEMP")
            handleWINJTEMP(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WCONINJH")
            handleWCONINJH(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WGRUPCON")
            handleWGRUPCON(keyword, currentStep);

        else if (keyword.name() == "COMPDAT")
            handleCOMPDAT(keyword, currentStep, grid, fp, eclipseProperties, parseContext, errors);

        else if (keyword.name() == "WELSEGS")
            handleWELSEGS(keyword, currentStep);

        else if (keyword.name() == "COMPSEGS")
            handleCOMPSEGS(keyword, currentStep, grid, parseContext, errors);

        else if (keyword.name() == "WSEGSICD")
            handleWSEGSICD(keyword, currentStep);

        else if (keyword.name() == "WSEGVALV")
            handleWSEGVALV(keyword, currentStep); //, parseContext, errors);

        else if (keyword.name() == "WELOPEN")
            handleWELOPEN(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WELTARG")
            handleWELTARG(section, keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "GRUPTREE")
            handleGRUPTREE(keyword, currentStep, unit_system, parseContext, errors);

        else if (keyword.name() == "GRUPNET")
            handleGRUPNET(keyword, currentStep, unit_system);

        else if (keyword.name() == "GCONINJE")
            handleGCONINJE(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "GCONPROD")
            handleGCONPROD(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "GEFAC")
            handleGEFAC(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "GCONSALE")
            handleGCONSALE(keyword, currentStep, unit_system);

        else if (keyword.name() == "GCONSUMP")
            handleGCONSUMP(keyword, currentStep, unit_system);

        else if (keyword.name() == "GUIDERAT")
            handleGUIDERAT(keyword, currentStep);

        else if (keyword.name() == "LINCOM")
            handleLINCOM(keyword, currentStep);

        else if (keyword.name() == "TUNING")
            handleTUNING(keyword, currentStep);

        else if (keyword.name() == "WRFT")
            rftProperties.push_back( std::make_pair( &keyword , currentStep ));

        else if (keyword.name() == "WRFTPLT")
            rftProperties.push_back( std::make_pair( &keyword , currentStep ));

        else if (keyword.name() == "WPIMULT")
            handleWPIMULT(keyword, currentStep);

        else if (keyword.name() == "COMPORD")
            handleCOMPORD(parseContext, errors , keyword, currentStep);

        else if (keyword.name() == "COMPLUMP")
            handleCOMPLUMP(keyword, currentStep);

        else if (keyword.name() == "DRSDT")
            handleDRSDT(keyword, currentStep);

        else if (keyword.name() == "DRVDT")
            handleDRVDT(keyword, currentStep);

        else if (keyword.name() == "DRSDTR")
            handleDRSDTR(keyword, currentStep);

        else if (keyword.name() == "DRVDTR")
            handleDRVDTR(keyword, currentStep);

        else if (keyword.name() == "VAPPARS")
            handleVAPPARS(keyword, currentStep);

        else if (keyword.name() == "WECON")
            handleWECON(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "MESSAGES")
            handleMESSAGES(keyword, currentStep);

        else if (keyword.name() == "WEFAC")
            handleWEFAC(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "VFPINJ")
            handleVFPINJ(keyword, unit_system, currentStep);

        else if (keyword.name() == "VFPPROD")
            handleVFPPROD(keyword, unit_system, currentStep);

        else if (keyword.name() == "NUPCOL")
            handleNUPCOL(keyword, currentStep);

        else if (geoModifiers.find( keyword.name() ) != geoModifiers.end()) {
            bool supported = geoModifiers.at( keyword.name() );
            if (supported) {
                this->m_modifierDeck[ currentStep ].addKeyword( keyword );
                m_events.addEvent( ScheduleEvents::GEO_MODIFIER , currentStep);
            } else {
                std::string msg = "OPM does not support grid property modifier " + keyword.name() + " in the Schedule section. Error at report: " + std::to_string( currentStep );
                parseContext.handleError( ParseContext::UNSUPPORTED_SCHEDULE_GEO_MODIFIER , msg, errors );
            }
        }
    }


    void Schedule::iterateScheduleSection(const ParseContext& parseContext , ErrorGuard& errors, const SCHEDULESection& section , const EclipseGrid& grid,
                                          const FieldPropsManager& fp, const Eclipse3DProperties& eclipseProperties) {
        size_t currentStep = 0;
        const auto& unit_system = section.unitSystem();
        std::vector<std::pair< const DeckKeyword* , size_t> > rftProperties;
        size_t keywordIdx = 0;

        while (true) {
            const auto& keyword = section.getKeyword(keywordIdx);
            if (keyword.name() == "ACTIONX") {
                Action::ActionX action(keyword, this->m_timeMap.getStartTime(currentStep + 1));
                while (true) {
                    keywordIdx++;
                    if (keywordIdx == section.size())
                        throw std::invalid_argument("Invalid ACTIONX section - missing ENDACTIO");

                    const auto& action_keyword = section.getKeyword(keywordIdx);
                    if (action_keyword.name() == "ENDACTIO")
                        break;

                    if (Action::ActionX::valid_keyword(action_keyword.name()))
                        action.addKeyword(action_keyword);
                    else {
                        std::string msg = "The keyword " + action_keyword.name() + " is not supported in a ACTIONX block.";
                        parseContext.handleError( ParseContext::ACTIONX_ILLEGAL_KEYWORD, msg, errors);
                    }
                }
                this->addACTIONX(action, currentStep);
            } else
                this->handleKeyword(currentStep, section, keywordIdx, keyword, parseContext, errors, grid, fp, eclipseProperties, unit_system, rftProperties);

            keywordIdx++;
            if (keywordIdx == section.size())
                break;
        }

        checkIfAllConnectionsIsShut(currentStep);

        for (auto rftPair = rftProperties.begin(); rftPair != rftProperties.end(); ++rftPair) {
            const DeckKeyword& keyword = *rftPair->first;
            size_t timeStep = rftPair->second;
            if (keyword.name() == "WRFT")
                handleWRFT(keyword,  timeStep);

            if (keyword.name() == "WRFTPLT")
                handleWRFTPLT(keyword, timeStep);
        }

        checkUnhandledKeywords(section);
    }


    void Schedule::addACTIONX(const Action::ActionX& action, std::size_t currentStep) {
        auto new_actions = std::make_shared<Action::Actions>( this->actions(currentStep) );
        new_actions->add(action);
        this->m_actions.update(currentStep, new_actions);
    }


    void Schedule::checkUnhandledKeywords(const SCHEDULESection& /*section*/) const
    {
    }



    void Schedule::handleWHISTCTL(const DeckKeyword& keyword, std::size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        const auto& record = keyword.getRecord(0);
        const std::string& cmodeString = record.getItem("CMODE").getTrimmedString(0);
        const auto controlMode = Well::ProducerCModeFromString( cmodeString );

        if (controlMode != Well::ProducerCMode::NONE) {
            if (!Well::WellProductionProperties::effectiveHistoryProductionControl(controlMode) ) {
                std::string msg = "The WHISTCTL keyword specifies an un-supported control mode " + cmodeString
                    + ", which makes WHISTCTL keyword not affect the simulation at all";
                OpmLog::warning(msg);
            } else
                this->global_whistctl_mode.update(currentStep, controlMode);
        }

        const std::string bhp_terminate = record.getItem("BPH_TERMINATE").getTrimmedString(0);
        if (bhp_terminate == "YES") {
            std::string msg = "The WHISTCTL keyword does not handle 'YES'. i.e. to terminate the run";
            OpmLog::error(msg);
            parseContext.handleError( ParseContext::UNSUPPORTED_TERMINATE_IF_BHP , msg, errors );
        }


        for (auto& well_pair : this->wells_static) {
            auto& dynamic_state = well_pair.second;
            auto well2 = std::make_shared<Well>(*dynamic_state[currentStep]);
            auto prop = std::make_shared<Well::WellProductionProperties>(well2->getProductionProperties());

            if (prop->whistctl_cmode != controlMode) {
                prop->whistctl_cmode = controlMode;
                well2->updateProduction(prop);
                this->updateWell(well2, currentStep);
            }
        }

        for (auto& well_pair : this->wells_static) {
            auto& dynamic_state = well_pair.second;
            auto well2 = std::make_shared<Well>(*dynamic_state[currentStep]);
            auto prop = std::make_shared<Well::WellProductionProperties>(well2->getProductionProperties());

            if (prop->whistctl_cmode != controlMode) {
                prop->whistctl_cmode = controlMode;
                well2->updateProduction(prop);
                this->updateWell(well2, currentStep);
            }
        }

        for (auto& well_pair : this->wells_static) {
            auto& dynamic_state = well_pair.second;
            auto well2 = std::make_shared<Well>(*dynamic_state[currentStep]);
            auto prop = std::make_shared<Well::WellProductionProperties>(well2->getProductionProperties());

            if (prop->whistctl_cmode != controlMode) {
                prop->whistctl_cmode = controlMode;
                well2->updateProduction(prop);
                this->updateWell(well2, currentStep);
            }
        }
    }

    void Schedule::handleNUPCOL( const DeckKeyword& keyword, size_t currentStep) {
        int nupcol = keyword.getRecord(0).getItem("NUM_ITER").get<int>(0);
        if (keyword.getRecord(0).getItem("NUM_ITER").defaultApplied(0)) {
            std::string msg = "OPM Flow uses 12 as default NUPCOL value";
            OpmLog::note(msg);
        }

        this->m_nupcol.update(currentStep, nupcol);
    }



    /*
      The COMPORD keyword is handled together with the WELSPECS keyword in the
      handleWELSPECS function.
    */
    void Schedule::handleCOMPORD(const ParseContext& parseContext, ErrorGuard& errors, const DeckKeyword& compordKeyword, size_t /* currentStep */) {
        for (const auto& record : compordKeyword) {
            const auto& methodItem = record.getItem<ParserKeywords::COMPORD::ORDER_TYPE>();
            if ((methodItem.get< std::string >(0) != "TRACK")  && (methodItem.get< std::string >(0) != "INPUT")) {
                std::string msg = "The COMPORD keyword only handles 'TRACK' or 'INPUT' order.";
                OpmLog::error(msg);
                parseContext.handleError( ParseContext::UNSUPPORTED_COMPORD_TYPE , msg, errors );
            }
        }
    }

    void Schedule::handleVFPINJ(const DeckKeyword& vfpinjKeyword, const UnitSystem& unit_system, size_t currentStep) {
        std::shared_ptr<VFPInjTable> table = std::make_shared<VFPInjTable>(vfpinjKeyword, unit_system);
        int table_id = table->getTableNum();

        const auto iter = vfpinj_tables.find(table_id);
        if (iter == vfpinj_tables.end()) {
            std::pair<int, DynamicState<std::shared_ptr<VFPInjTable> > > pair = std::make_pair(table_id, DynamicState<std::shared_ptr<VFPInjTable> >(this->m_timeMap, nullptr));
            vfpinj_tables.insert( pair );
        }
        auto & table_state = vfpinj_tables.at(table_id);
        table_state.update(currentStep, table);
        this->m_events.addEvent( ScheduleEvents::VFPINJ_UPDATE , currentStep);
    }

    void Schedule::handleVFPPROD(const DeckKeyword& vfpprodKeyword, const UnitSystem& unit_system, size_t currentStep) {
        std::shared_ptr<VFPProdTable> table = std::make_shared<VFPProdTable>(vfpprodKeyword, unit_system);
        int table_id = table->getTableNum();

        const auto iter = vfpprod_tables.find(table_id);
        if (iter == vfpprod_tables.end()) {
            std::pair<int, DynamicState<std::shared_ptr<VFPProdTable> > > pair = std::make_pair(table_id, DynamicState<std::shared_ptr<VFPProdTable> >(this->m_timeMap, nullptr));
            vfpprod_tables.insert( pair );
        }
        auto & table_state = vfpprod_tables.at(table_id);
        table_state.update(currentStep, table);
        this->m_events.addEvent( ScheduleEvents::VFPPROD_UPDATE , currentStep);
    }


    void Schedule::handleWELSPECS( const SCHEDULESection& section,
                                   size_t index,
                                   size_t currentStep,
                                   const UnitSystem& unit_system,
                                   const ParseContext& parseContext,
                                   ErrorGuard& errors) {
        const auto COMPORD_in_timestep = [&]() -> const DeckKeyword* {
            auto itr = section.begin() + index;
            for( ; itr != section.end(); ++itr ) {
                if( itr->name() == "DATES" ) return nullptr;
                if( itr->name() == "TSTEP" ) return nullptr;
                if( itr->name() == "COMPORD" ) return std::addressof( *itr );
            }

            return nullptr;
        };

        const auto& keyword = section.getKeyword( index );

        for (size_t recordNr = 0; recordNr < keyword.size(); recordNr++) {
            const auto& record = keyword.getRecord(recordNr);
            const std::string& wellName = trim_wgname(keyword, record.getItem("WELL").get<std::string>(0), parseContext, errors);
            const std::string& groupName = trim_wgname(keyword, record.getItem("GROUP").get<std::string>(0), parseContext, errors);

            if (!hasGroup(groupName))
                addGroup(groupName , currentStep, unit_system);

            if (!hasWell(wellName)) {
                auto wellConnectionOrder = Connection::Order::TRACK;

                if( const auto* compordp = COMPORD_in_timestep() ) {
                     const auto& compord = *compordp;

                    for (size_t compordRecordNr = 0; compordRecordNr < compord.size(); compordRecordNr++) {
                        const auto& compordRecord = compord.getRecord(compordRecordNr);

                        const std::string& wellNamePattern = compordRecord.getItem(0).getTrimmedString(0);
                        if (Well::wellNameInWellNamePattern(wellName, wellNamePattern)) {
                            const std::string& compordString = compordRecord.getItem(1).getTrimmedString(0);
                            wellConnectionOrder = Connection::OrderFromString(compordString);
                        }
                    }
                }
                this->addWell(wellName, record, currentStep, wellConnectionOrder, unit_system);
                this->addWellToGroup(groupName, wellName, currentStep);
            } else {
                const auto headI = record.getItem( "HEAD_I" ).get< int >( 0 ) - 1;
                const auto headJ = record.getItem( "HEAD_J" ).get< int >( 0 ) - 1;
                const auto& refDepthItem = record.getItem( "REF_DEPTH" );
                double drainageRadius = record.getItem( "D_RADIUS" ).getSIDouble(0);
                double refDepth = refDepthItem.hasValue( 0 )
                    ? refDepthItem.getSIDouble( 0 )
                    : -1.0;
                {
                    bool update = false;
                    auto well2 = std::shared_ptr<Well>(new Well( this->getWell(wellName, currentStep)));
                    update = well2->updateHead(headI, headJ);
                    update |= well2->updateRefDepth(refDepth);
                    update |= well2->updateDrainageRadius(drainageRadius);

                    if (update) {
                        this->updateWell(well2, currentStep);
                        this->addWellGroupEvent(well2->name(), ScheduleEvents::WELL_WELSPECS_UPDATE, currentStep);
                    }
                }
            }

            this->addWellToGroup(groupName, wellName, currentStep);
        }
    }

    void Schedule::handleVAPPARS( const DeckKeyword& keyword, size_t currentStep){
        size_t numPvtRegions = m_runspec.tabdims().getNumPVTTables();
        std::vector<double> vap(numPvtRegions);
        std::vector<double> density(numPvtRegions);
        for( const auto& record : keyword ) {
            std::fill(vap.begin(), vap.end(), record.getItem("OIL_VAP_PROPENSITY").get< double >(0));
            std::fill(density.begin(), density.end(), record.getItem("OIL_DENSITY_PROPENSITY").get< double >(0));
            OilVaporizationProperties ovp = this->m_oilvaporizationproperties.get(currentStep);
            OilVaporizationProperties::updateVAPPARS(ovp, vap, density);
            this->m_oilvaporizationproperties.update( currentStep, ovp );
        }
    }

    void Schedule::handleDRVDT( const DeckKeyword& keyword, size_t currentStep){
        size_t numPvtRegions = m_runspec.tabdims().getNumPVTTables();
        std::vector<double> max(numPvtRegions);
        for( const auto& record : keyword ) {
            std::fill(max.begin(), max.end(), record.getItem("DRVDT_MAX").getSIDouble(0));
            OilVaporizationProperties ovp = this->m_oilvaporizationproperties.get(currentStep);
            OilVaporizationProperties::updateDRVDT(ovp, max);
            this->m_oilvaporizationproperties.update( currentStep, ovp );
        }
    }


    void Schedule::handleDRSDT( const DeckKeyword& keyword, size_t currentStep){
        size_t numPvtRegions = m_runspec.tabdims().getNumPVTTables();
        std::vector<double> max(numPvtRegions);
        std::vector<std::string> options(numPvtRegions);
        for( const auto& record : keyword ) {
            std::fill(max.begin(), max.end(), record.getItem("DRSDT_MAX").getSIDouble(0));
            std::fill(options.begin(), options.end(), record.getItem("Option").get< std::string >(0));
            OilVaporizationProperties ovp = this->m_oilvaporizationproperties.get(currentStep);
            OilVaporizationProperties::updateDRSDT(ovp, max, options);
            this->m_oilvaporizationproperties.update( currentStep, ovp );
        }
    }

    void Schedule::handleDRSDTR( const DeckKeyword& keyword, size_t currentStep){
        size_t numPvtRegions = m_runspec.tabdims().getNumPVTTables();
        std::vector<double> max(numPvtRegions);
        std::vector<std::string> options(numPvtRegions);
        size_t pvtRegionIdx = 0;
        for( const auto& record : keyword ) {
            max[pvtRegionIdx] = record.getItem("DRSDT_MAX").getSIDouble(0);
            options[pvtRegionIdx] = record.getItem("Option").get< std::string >(0);
            pvtRegionIdx++;
        }
        OilVaporizationProperties ovp = this->m_oilvaporizationproperties.get(currentStep);
        OilVaporizationProperties::updateDRSDT(ovp, max, options);
        this->m_oilvaporizationproperties.update( currentStep, ovp );
    }

    void Schedule::handleDRVDTR( const DeckKeyword& keyword, size_t currentStep){
        size_t numPvtRegions = m_runspec.tabdims().getNumPVTTables();
        size_t pvtRegionIdx = 0;
        std::vector<double> max(numPvtRegions);
        for( const auto& record : keyword ) {
            max[pvtRegionIdx] = record.getItem("DRVDT_MAX").getSIDouble(0);
            pvtRegionIdx++;
        }
        OilVaporizationProperties ovp = this->m_oilvaporizationproperties.get(currentStep);
        OilVaporizationProperties::updateDRVDT(ovp, max);
        this->m_oilvaporizationproperties.update( currentStep, ovp );
    }

    void Schedule::handleWCONHIST( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& wellNamePattern =
                record.getItem("WELL").getTrimmedString(0);

            const Well::Status status = Well::StatusFromString(record.getItem("STATUS").getTrimmedString(0));

            auto well_names = this->wellNames(wellNamePattern, currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for( const auto& well_name : well_names) {
                updateWellStatus( well_name , currentStep , status );
                {
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well2 = std::make_shared<Well>(*dynamic_state[currentStep]);
                    bool switching_from_injector = !well2->isProducer();
                    auto properties = std::make_shared<Well::WellProductionProperties>(well2->getProductionProperties());
                    bool update_well = false;
                    properties->handleWCONHIST(record);

                    if (switching_from_injector) {
                        properties->resetDefaultBHPLimit();
                        well2->updateProducer(true);

                        auto inj_props = std::make_shared<Well::WellInjectionProperties>(well2->getInjectionProperties());
                        inj_props->resetBHPLimit();
                        well2->updateInjection(inj_props);
                    }

                    if (well2->updateProducer(true))
                        update_well = true;

                    if (well2->updateProduction(properties))
                        update_well = true;

                    if (well2->updatePrediction(false))
                        update_well = true;

                    if (update_well) {
                        m_events.addEvent( ScheduleEvents::PRODUCTION_UPDATE , currentStep);
                        this->addWellGroupEvent( well2->name(), ScheduleEvents::PRODUCTION_UPDATE, currentStep);
                        this->updateWell(well2, currentStep);
                    }
                    if ( !well2->getAllowCrossFlow()) {
                        // The numerical content of the rate UDAValues is accessed unconditionally;
                        // since this is in history mode use of UDA values is not allowed anyway.
                        const auto& oil_rate = properties->OilRate;
                        const auto& water_rate = properties->WaterRate;
                        const auto& gas_rate = properties->GasRate;
                        if (oil_rate.zero() && water_rate.zero() && gas_rate.zero()) {
                            std::string msg =
                                "Well " + well2->name() + " is a history matched well with zero rate where crossflow is banned. " +
                                "This well will be closed at " + std::to_string ( m_timeMap.getTimePassedUntil(currentStep) / (60*60*24) ) + " days";
                            OpmLog::note(msg);
                            updateWellStatus( well_name, currentStep, Well::Status::SHUT );
                        }
                    }
                }
            }
        }
    }


    void Schedule::handleWCONPROD( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& wellNamePattern =
                record.getItem("WELL").getTrimmedString(0);

            const Well::Status status = Well::StatusFromString(record.getItem("STATUS").getTrimmedString(0));
            auto well_names = this->wellNames(wellNamePattern, currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for( const auto& well_name : well_names) {

                updateWellStatus( well_name , currentStep , status );
                {
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well2 = std::make_shared<Well>(*dynamic_state[currentStep]);
                    bool switching_from_injector = !well2->isProducer();
                    auto properties = std::make_shared<Well::WellProductionProperties>(well2->getProductionProperties());
                    bool update_well = switching_from_injector;
                    properties->clearControls();
                    if (well2->isAvailableForGroupControl())
                        properties->addProductionControl(Well::ProducerCMode::GRUP);

                    properties->handleWCONPROD(well_name, record);

                    if (switching_from_injector)
                        properties->resetDefaultBHPLimit();

                    if (well2->updateProducer(true))
                        update_well = true;

                    if (well2->updateProduction(properties))
                        update_well = true;

                    if (well2->updatePrediction(true))
                        update_well = true;

                    if (update_well) {
                        m_events.addEvent( ScheduleEvents::PRODUCTION_UPDATE , currentStep);
                        this->addWellGroupEvent( well2->name(), ScheduleEvents::PRODUCTION_UPDATE, currentStep);
                        this->updateWell(well2, currentStep);
                    }

                    auto udq = std::make_shared<UDQActive>(this->udqActive(currentStep));
                    if (properties->updateUDQActive(this->getUDQConfig(currentStep), *udq))
                        this->updateUDQActive(currentStep, udq);
                }
            }
        }
    }


    void Schedule::updateWell(std::shared_ptr<Well> well, size_t reportStep) {
        auto& dynamic_state = this->wells_static.at(well->name());
        dynamic_state.update(reportStep, well);
    }


    /*
      Function is quite dangerous - because if this is called while holding a
      Well pointer that will go stale and needs to be refreshed.
    */
    bool Schedule::updateWellStatus( const std::string& well_name, size_t reportStep , Well::Status status) {
        bool update = false;
        {
            auto& dynamic_state = this->wells_static.at(well_name);
            auto well2 = std::make_shared<Well>(*dynamic_state[reportStep]);
            if (well2->updateStatus(status)) {
                m_events.addEvent( ScheduleEvents::WELL_STATUS_CHANGE, reportStep );
                this->addWellGroupEvent( well2->name(), ScheduleEvents::WELL_STATUS_CHANGE, reportStep);
                this->updateWell(well2, reportStep);
                update = true;
            }
        }
        return update;
    }


    void Schedule::handleWPIMULT( const DeckKeyword& keyword, size_t currentStep) {
        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto& well_names = this->wellNames(wellNamePattern, currentStep);
            for (const auto& wname : well_names) {
                {
                    auto& dynamic_state = this->wells_static.at(wname);
                    auto well_ptr = std::make_shared<Well>( *dynamic_state[currentStep] );
                    if (well_ptr->handleWPIMULT(record))
                        this->updateWell(well_ptr, currentStep);
                }
            }
        }
    }




    void Schedule::handleWCONINJE( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);

            auto well_names = wellNames(wellNamePattern, currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for( const auto& well_name : well_names ) {
                Well::Status status = Well::StatusFromString( record.getItem("STATUS").getTrimmedString(0));
                updateWellStatus( well_name , currentStep , status );
                {
                    bool update_well = false;
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well2 = std::make_shared<Well>(*dynamic_state[currentStep]);
                    auto injection = std::make_shared<Well::WellInjectionProperties>(well2->getInjectionProperties());
                    injection->handleWCONINJE(record, well2->isAvailableForGroupControl(), well_name);

                    if (well2->updateProducer(false))
                        update_well = true;

                    if (well2->updateInjection(injection))
                        update_well = true;

                    if (well2->updatePrediction(true))
                        update_well = true;

                    if (update_well) {
                        this->updateWell(well2, currentStep);
                        m_events.addEvent( ScheduleEvents::INJECTION_UPDATE , currentStep );
                        this->addWellGroupEvent( well_name, ScheduleEvents::INJECTION_UPDATE, currentStep);
                    }

                    // if the well has zero surface rate limit or reservior rate limit, while does not allow crossflow,
                    // it should be turned off.
                    if ( ! well2->getAllowCrossFlow() ) {
                         std::string msg =
                         "Well " + well_name + " is an injector with zero rate where crossflow is banned. " +
                         "This well will be closed at " + std::to_string ( m_timeMap.getTimePassedUntil(currentStep) / (60*60*24) ) + " days";

                         if (injection->surfaceInjectionRate.is<double>()) {
                             if (injection->hasInjectionControl(Well::InjectorCMode::RATE) && injection->surfaceInjectionRate.zero()) {
                                 OpmLog::note(msg);
                                 updateWellStatus( well_name, currentStep, Well::Status::SHUT );
                             }
                         }

                         if (injection->reservoirInjectionRate.is<double>()) {
                             if (injection->hasInjectionControl(Well::InjectorCMode::RESV) && injection->reservoirInjectionRate.zero()) {
                                 OpmLog::note(msg);
                                 updateWellStatus( well_name, currentStep, Well::Status::SHUT );
                             }
                         }
                    }

                    auto udq = std::make_shared<UDQActive>(this->udqActive(currentStep));
                    if (injection->updateUDQActive(this->getUDQConfig(currentStep), *udq))
                        this->updateUDQActive(currentStep, udq);
                }
            }
        }
    }

    void Schedule::handleWCONINJH(const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            Well::Status status = Well::StatusFromString( record.getItem("STATUS").getTrimmedString(0));
            const auto well_names = wellNames( wellNamePattern, currentStep );

            if (well_names.empty())
                invalidNamePattern( wellNamePattern, parseContext, errors, keyword);

            for (const auto& well_name : well_names) {
                updateWellStatus( well_name, currentStep, status );
                {
                    bool update_well = false;
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well2 = std::make_shared<Well>(*dynamic_state[currentStep]);
                    auto injection = std::make_shared<Well::WellInjectionProperties>(well2->getInjectionProperties());
                    injection->handleWCONINJH(record, well2->isProducer(), well_name);

                    if (well2->updateProducer(false))
                        update_well = true;

                    if (well2->updateInjection(injection))
                        update_well = true;

                    if (well2->updatePrediction(false))
                        update_well = true;

                    if (update_well) {
                        this->updateWell(well2, currentStep);
                        m_events.addEvent( ScheduleEvents::INJECTION_UPDATE , currentStep );
                        this->addWellGroupEvent( well_name, ScheduleEvents::INJECTION_UPDATE, currentStep);
                    }

                    if ( ! well2->getAllowCrossFlow() && (injection->surfaceInjectionRate.zero())) {
                        std::string msg =
                            "Well " + well_name + " is an injector with zero rate where crossflow is banned. " +
                            "This well will be closed at " + std::to_string ( m_timeMap.getTimePassedUntil(currentStep) / (60*60*24) ) + " days";
                        OpmLog::note(msg);
                        updateWellStatus( well_name, currentStep, Well::Status::SHUT );
                    }
                }
            }
        }
    }

    void Schedule::handleWFOAM( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, currentStep );

            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for (const auto& well_name : well_names) {
                const auto& dynamic_state = this->wells_static.at(well_name);
                auto well2 = std::make_shared<Well>(*dynamic_state[currentStep]);
                auto foam_properties = std::make_shared<WellFoamProperties>(well2->getFoamProperties());
                foam_properties->handleWFOAM(record);
                if (well2->updateFoamProperties(foam_properties))
                    this->updateWell(well2, currentStep);
            }
        }
    }

    void Schedule::handleWPOLYMER( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames( wellNamePattern, currentStep );

            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for( const auto& well_name : well_names) {
                {
                    const auto& dynamic_state = this->wells_static.at(well_name);
                    auto well2 = std::make_shared<Well>(*dynamic_state[currentStep]);
                    auto polymer_properties = std::make_shared<WellPolymerProperties>( well2->getPolymerProperties() );
                    polymer_properties->handleWPOLYMER(record);
                    if (well2->updatePolymerProperties(polymer_properties))
                        this->updateWell(well2, currentStep);
                }
            }
        }
    }

    void Schedule::handleWSALT( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, currentStep );

            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for (const auto& well_name : well_names) {
                const auto& dynamic_state = this->wells_static.at(well_name);
                auto well2 = std::make_shared<Well>(*dynamic_state[currentStep]);
                auto brine_properties = std::make_shared<WellBrineProperties>(well2->getBrineProperties());
                brine_properties->handleWSALT(record);
                if (well2->updateBrineProperties(brine_properties))
                    this->updateWell(well2, currentStep);
            }
        }
    }


    void Schedule::handleWPMITAB( const DeckKeyword& keyword,  const size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {

        for (const auto& record : keyword) {

            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames( wellNamePattern, currentStep );

            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for (const auto& well_name : well_names) {
                {
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well2 = std::make_shared<Well>(*dynamic_state[currentStep]);
                    auto polymer_properties = std::make_shared<WellPolymerProperties>( well2->getPolymerProperties() );
                    polymer_properties->handleWPMITAB(record);
                    if (well2->updatePolymerProperties(polymer_properties))
                        this->updateWell(well2, currentStep);
                }
            }
        }
    }


    void Schedule::handleWSKPTAB( const DeckKeyword& keyword,  const size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {


        for (const auto& record : keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, currentStep);

            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for (const auto& well_name : well_names) {
                {
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well2 = std::make_shared<Well>(*dynamic_state[currentStep]);
                    auto polymer_properties = std::make_shared<WellPolymerProperties>( well2->getPolymerProperties() );
                    polymer_properties->handleWSKPTAB(record);
                    if (well2->updatePolymerProperties(polymer_properties))
                        this->updateWell(well2, currentStep);
                }
            }
        }
    }


    void Schedule::handleWECON( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames( wellNamePattern , currentStep);

            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for(const auto& well_name : well_names) {
                {
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well2 = std::make_shared<Well>(*dynamic_state[currentStep]);
                    auto econ_limits = std::make_shared<WellEconProductionLimits>( record );
                    if (well2->updateEconLimits(econ_limits))
                        this->updateWell(well2, currentStep);
                }
            }
        }
    }

    void Schedule::handleWEFAC( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELLNAME").getTrimmedString(0);
            const double& efficiencyFactor = record.getItem("EFFICIENCY_FACTOR").get< double >(0);
            const auto well_names = wellNames( wellNamePattern, currentStep );

            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for(const auto& well_name : well_names) {
                {
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well2 = std::make_shared<Well>(*dynamic_state[currentStep]);
                    if (well2->updateEfficiencyFactor(efficiencyFactor))
                        this->updateWell(well2, currentStep);
                }
            }
        }
    }


    void Schedule::handleWLIST(const DeckKeyword& keyword, size_t currentStep) {
        const std::string legal_actions = "NEW:ADD:DEL:MOV";
        const auto& current = *this->wlist_manager.get(currentStep);
        std::shared_ptr<WListManager> new_wlm(new WListManager(current));
        for (const auto& record : keyword) {
            const std::string& name = record.getItem("NAME").getTrimmedString(0);
            const std::string& action = record.getItem("ACTION").getTrimmedString(0);
            const std::vector<std::string>& wells = record.getItem("WELLS").getData<std::string>();

            if (legal_actions.find(action) == std::string::npos)
                throw std::invalid_argument("The action:" + action + " is not recognized.");

            for (const auto& well : wells) {
                if (!this->hasWell(well))
                    throw std::invalid_argument("The well: " + well + " has not been defined in the WELSPECS");
            }

            if (name[0] != '*')
                throw std::invalid_argument("The list name in WLIST must start with a '*'");

            if (action == "NEW")
                new_wlm->newList(name);

            if (!new_wlm->hasList(name))
                throw std::invalid_argument("Invalid well list: " + name);

            auto& wlist = new_wlm->getList(name);
            if (action == "MOV") {
                for (const auto& well : wells)
                    new_wlm->delWell(well);
            }

            if (action == "DEL") {
                for (const auto& well : wells)
                    wlist.del(well);
            } else {
                for (const auto& well : wells)
                    wlist.add(well);
            }

        }
        this->wlist_manager.update(currentStep, new_wlm);
    }

    void Schedule::handleUDQ(const DeckKeyword& keyword, size_t currentStep) {
        const auto& current = *this->udq_config.get(currentStep);
        std::shared_ptr<UDQConfig> new_udq = std::make_shared<UDQConfig>(current);
        for (const auto& record : keyword)
            new_udq->add_record(record);

        this->udq_config.update(currentStep, new_udq);
    }


    void Schedule::handleWTEST(const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        const auto& current = *this->wtest_config.get(currentStep);
        std::shared_ptr<WellTestConfig> new_config(new WellTestConfig(current));
        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames( wellNamePattern , currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            double test_interval = record.getItem("INTERVAL").getSIDouble(0);
            const std::string& reasons = record.getItem("REASON").get<std::string>(0);
            int num_test = record.getItem("TEST_NUM").get<int>(0);
            double startup_time = record.getItem("START_TIME").getSIDouble(0);

            for(const auto& well_name : well_names) {
                if (reasons.empty())
                    new_config->drop_well(well_name);
                else
                    new_config->add_well(well_name, reasons, test_interval, num_test, startup_time, currentStep);
            }
        }
        this->wtest_config.update(currentStep, new_config);
    }

    void Schedule::handleWSOLVENT( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {

        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames( wellNamePattern , currentStep);
            double fraction = record.getItem("SOLVENT_FRACTION").get< UDAValue >(0).getSI();

            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for(const auto& well_name : well_names) {
                {
                    const auto& well = this->getWell(well_name, currentStep);
                    const auto& inj = well.getInjectionProperties();
                    if (!well.isProducer() && inj.injectorType == Well::InjectorType::GAS) {
                        if (well.getSolventFraction() != fraction) {
                            auto new_well = std::make_shared<Well>(well);
                            new_well->updateSolventFraction(fraction);
                            this->updateWell(new_well, currentStep);
                        }
                    } else
                        throw std::invalid_argument("The WSOLVENT keyword can only be applied to gas injectors");
                }
            }
        }
    }

    void Schedule::handleWTRACER( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {

        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames( wellNamePattern, currentStep );

            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for(const auto& well_name : well_names) {
                double tracerConcentration = record.getItem("CONCENTRATION").get< UDAValue >(0).getSI();
                const std::string& tracerName = record.getItem("TRACER").getTrimmedString(0);
                {
                    auto well = std::make_shared<Well>( this->getWell(well_name, currentStep));
                    auto wellTracerProperties = std::make_shared<WellTracerProperties>( well->getTracerProperties() );
                    wellTracerProperties->setConcentration(tracerName, tracerConcentration);
                    if (well->updateTracer(wellTracerProperties))
                        this->updateWell(well, currentStep);
                }
            }
        }
    }

    void Schedule::handleWTEMP( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames( wellNamePattern, currentStep );
            double temp = record.getItem("TEMP").getSIDouble(0);
            if (well_names.empty())
                invalidNamePattern( wellNamePattern, parseContext, errors, keyword);

            for (const auto& well_name : well_names) {
                // TODO: Is this the right approach? Setting the well temperature only
                // has an effect on injectors, but specifying it for producers won't hurt
                // and wells can also switch their injector/producer status. Note that
                // modifying the injector properties for producer wells currently leads
                // to a very weird segmentation fault downstream. For now, let's take the
                // water route.
                {
                    const auto& well = this->getWell(well_name, currentStep);
                    double current_temp = well.getInjectionProperties().temperature;
                    if (current_temp != temp && !well.isProducer()) {
                        auto& dynamic_state = this->wells_static.at(well_name);
                        auto well_ptr = std::make_shared<Well>( *dynamic_state[currentStep] );
                        auto inj = std::make_shared<Well::WellInjectionProperties>(well_ptr->getInjectionProperties());
                        inj->temperature = temp;
                        well_ptr->updateInjection(inj);
                        this->updateWell(well_ptr, currentStep);
                    }
                }
            }
        }
    }

    void Schedule::handleWINJTEMP( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        // we do not support the "enthalpy" field yet. how to do this is a more difficult
        // question.
        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            auto well_names = wellNames( wellNamePattern , currentStep);
            double temp = record.getItem("TEMPERATURE").getSIDouble(0);

            if (well_names.empty())
                invalidNamePattern( wellNamePattern, parseContext, errors, keyword);

            for (const auto& well_name : well_names) {
                // TODO: Is this the right approach? Setting the well temperature only
                // has an effect on injectors, but specifying it for producers won't hurt
                // and wells can also switch their injector/producer status. Note that
                // modifying the injector properties for producer wells currently leads
                // to a very weird segmentation fault downstream. For now, let's take the
                // water route.
                {
                    const auto& well = this->getWell(well_name, currentStep);
                    double current_temp = well.getInjectionProperties().temperature;
                    if (current_temp != temp && !well.isProducer()) {
                        auto& dynamic_state = this->wells_static.at(well_name);
                        auto well_ptr = std::make_shared<Well>( *dynamic_state[currentStep] );
                        auto inj = std::make_shared<Well::WellInjectionProperties>(well_ptr->getInjectionProperties());
                        inj->temperature = temp;
                        well_ptr->updateInjection(inj);
                        this->updateWell(well_ptr, currentStep);
                    }
                }
            }
        }
    }

    void Schedule::handleCOMPLUMP( const DeckKeyword& keyword,
                                   size_t timestep ) {

        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, timestep);
            for (const auto& wname : well_names) {
                {
                    auto& dynamic_state = this->wells_static.at(wname);
                    auto well_ptr = std::make_shared<Well>( *dynamic_state[timestep] );
                    if (well_ptr->handleCOMPLUMP(record))
                        this->updateWell(well_ptr, timestep);
                }
            }
        }
    }

    void Schedule::handleWELOPEN( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors, const std::vector<std::string>& matching_wells) {

        auto conn_defaulted = []( const DeckRecord& rec ) {
            auto defaulted = []( const DeckItem& item ) {
                return item.defaultApplied( 0 );
            };

            return std::all_of( rec.begin() + 2, rec.end(), defaulted );
        };

        constexpr auto open = Well::Status::OPEN;
        bool action_mode = !matching_wells.empty();

        for( const auto& record : keyword ) {
            const auto& wellNamePattern = record.getItem( "WELL" ).getTrimmedString(0);
            const auto& status_str = record.getItem( "STATUS" ).getTrimmedString( 0 );
            const auto well_names = this->wellNames(wellNamePattern, currentStep, matching_wells);
            if (well_names.empty())
                invalidNamePattern( wellNamePattern, parseContext, errors, keyword);

            /* if all records are defaulted or just the status is set, only
             * well status is updated
             */
            if( conn_defaulted( record ) ) {
                const auto well_status = Well::StatusFromString( status_str );
                for (const auto& wname : well_names) {
                    {
                        const auto& well = this->getWell(wname, currentStep);
                        if( well_status == open && !well.canOpen() ) {
                            auto days = m_timeMap.getTimePassedUntil( currentStep ) / (60 * 60 * 24);
                            std::string msg = "Well " + wname
                                + " where crossflow is banned has zero total rate."
                                + " This well is prevented from opening at "
                                + std::to_string( days ) + " days";
                            OpmLog::note(msg);
                        } else {
                            this->updateWellStatus( wname, currentStep, well_status );
                            if (well_status == open)
                                this->rft_config.addWellOpen(wname, currentStep);

                            OpmLog::info(Well::Status2String(well_status) + " well: " + wname + " at report step: " + std::to_string(currentStep));
                        }
                    }
                }

                continue;
            }

            for (const auto& wname : well_names) {
                const auto comp_status = Connection::StateFromString( status_str );
                {
                    auto& dynamic_state = this->wells_static.at(wname);
                    auto well_ptr = std::make_shared<Well>( *dynamic_state[currentStep] );
                    if (well_ptr->handleWELOPEN(record, comp_status, action_mode)) {
                        // The updateWell call breaks test at line 825 and 831 in ScheduleTests
                        this->updateWell(well_ptr, currentStep);
                        const auto well_status = Well::StatusFromString( status_str );
                        OpmLog::info(Well::Status2String(well_status) + " well: " + wname + " at report step: " + std::to_string(currentStep));
                    }
                }
                m_events.addEvent( ScheduleEvents::COMPLETION_CHANGE, currentStep );
            }
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

      Update: See the discussion following the defintions of the SI factors, due
        to a bad design we currently need the well to be specified with WCONPROD
        / WCONHIST before WELTARG is applied, if not the units for the rates
        will be wrong.
    */

    void Schedule::handleWELTARG( const SCHEDULESection& section ,
                                  const DeckKeyword& keyword,
                                  size_t currentStep,
                                  const ParseContext& parseContext, ErrorGuard& errors) {
        Opm::UnitSystem unitSystem = section.unitSystem();
        const double SiFactorP = unitSystem.parse("Pressure").getSIScaling();
        for( const auto& record : keyword ) {

            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto cmode = Well::WELTARGCModeFromString(record.getItem("CMODE").getTrimmedString(0));
            double newValue = record.getItem("NEW_VALUE").get< double >(0);

            const auto well_names = wellNames( wellNamePattern, currentStep );

            if( well_names.empty() )
                invalidNamePattern( wellNamePattern, parseContext, errors, keyword);

            for(const auto& well_name : well_names) {
                {
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well2 = std::make_shared<Well>(*dynamic_state[currentStep]);
                    bool update = false;
                    if (well2->isProducer()) {
                        auto prop = std::make_shared<Well::WellProductionProperties>(well2->getProductionProperties());
                        prop->handleWELTARG(cmode, newValue, SiFactorP);
                        update = well2->updateProduction(prop);
                        if (cmode == Well::WELTARGCMode::GUID)
                            update |= well2->updateWellGuideRate(newValue);
                    } else {
                        auto inj = std::make_shared<Well::WellInjectionProperties>(well2->getInjectionProperties());
                        inj->handleWELTARG(cmode, newValue, SiFactorP);
                        update = well2->updateInjection(inj);
                        if (cmode == Well::WELTARGCMode::GUID)
                            update |= well2->updateWellGuideRate(newValue);
                    }
                    if (update)
                        this->updateWell(well2, currentStep);
                }
            }
        }
    }

    void Schedule::handleGCONINJE( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& groupNamePattern = record.getItem("GROUP").getTrimmedString(0);
            const auto group_names = this->groupNames(groupNamePattern);

            if (group_names.empty())
                invalidNamePattern(groupNamePattern, parseContext, errors, keyword);

            for (const auto& group_name : group_names){
                Group::InjectionCMode controlMode = Group::InjectionCModeFromString( record.getItem("CONTROL_MODE").getTrimmedString(0) );
                Phase phase = get_phase( record.getItem("PHASE").getTrimmedString(0));
                auto surfaceInjectionRate = record.getItem("SURFACE_TARGET").get< UDAValue >(0);
                auto reservoirInjectionRate = record.getItem("RESV_TARGET").get<UDAValue>(0);
                auto reinj_target = record.getItem("REINJ_TARGET").get<UDAValue>(0);
                auto voidage_target = record.getItem("VOIDAGE_TARGET").get<UDAValue>(0);
                std::string reinj_group = group_name;
                if (!record.getItem("REINJECT_GROUP").defaultApplied(0))
                    reinj_group = record.getItem("REINJECT_GROUP").getTrimmedString(0);

                std::string voidage_group = group_name;
                if (!record.getItem("VOIDAGE_GROUP").defaultApplied(0))
                    voidage_group = record.getItem("VOIDAGE_GROUP").getTrimmedString(0);;

                //surfaceInjectionRate = injection::rateToSI(surfaceInjectionRate, phase, section.unitSystem());
                {
                    auto group_ptr = std::make_shared<Group>(this->getGroup(group_name, currentStep));
                    Group::GroupInjectionProperties injection;
                    injection.phase = phase;
                    injection.cmode = controlMode;
                    injection.surface_max_rate = surfaceInjectionRate;
                    injection.resv_max_rate = reservoirInjectionRate;
                    injection.target_reinj_fraction = reinj_target;
                    injection.target_void_fraction = voidage_target;
                    injection.injection_controls = 0;
                    injection.reinj_group = reinj_group;
                    injection.voidage_group = voidage_group;

                    if (!record.getItem("SURFACE_TARGET").defaultApplied(0))
                        injection.injection_controls += static_cast<int>(Group::InjectionCMode::RATE);

                    if (!record.getItem("RESV_TARGET").defaultApplied(0))
                        injection.injection_controls += static_cast<int>(Group::InjectionCMode::RESV);

                    if (!record.getItem("REINJ_TARGET").defaultApplied(0))
                        injection.injection_controls += static_cast<int>(Group::InjectionCMode::REIN);

                    if (!record.getItem("VOIDAGE_TARGET").defaultApplied(0))
                        injection.injection_controls += static_cast<int>(Group::InjectionCMode::VREP);

                    if (group_ptr->updateInjection(injection)) {
                        this->updateGroup(std::move(group_ptr), currentStep);
                        m_events.addEvent( ScheduleEvents::GROUP_INJECTION_UPDATE , currentStep);
                        this->addWellGroupEvent(group_name, ScheduleEvents::GROUP_INJECTION_UPDATE, currentStep);
                    }
                }
            }
        }
    }


    void Schedule::handleGCONPROD( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& groupNamePattern = record.getItem("GROUP").getTrimmedString(0);
            const auto group_names = this->groupNames(groupNamePattern);

            if (group_names.empty())
                invalidNamePattern(groupNamePattern, parseContext, errors, keyword);

            for (const auto& group_name : group_names){
                Group::ProductionCMode controlMode = Group::ProductionCModeFromString( record.getItem("CONTROL_MODE").getTrimmedString(0) );
                Group::ExceedAction exceedAction = Group::ExceedActionFromString(record.getItem("EXCEED_PROC").getTrimmedString(0) );
                auto oil_target = record.getItem("OIL_TARGET").get<UDAValue>(0);
                auto gas_target = record.getItem("GAS_TARGET").get<UDAValue>(0);
                auto water_target = record.getItem("WATER_TARGET").get<UDAValue>(0);
                auto liquid_target = record.getItem("LIQUID_TARGET").get<UDAValue>(0);
                auto guide_rate_def = Group::GuideRateTarget::NO_GUIDE_RATE;
                double guide_rate = 0;
                if (group_name != "FIELD") {
                    if (record.getItem("GUIDE_RATE_DEF").hasValue(0)) {
                        std::string guide_rate_str = record.getItem("GUIDE_RATE_DEF").getTrimmedString(0);
                        guide_rate_def = Group::GuideRateTargetFromString( guide_rate_str );

                        if ((guide_rate_def == Group::GuideRateTarget::INJV ||
                             guide_rate_def == Group::GuideRateTarget::POTN ||
                             guide_rate_def == Group::GuideRateTarget::FORM)) {
                            std::string msg = "The supplied guide_rate value will be ignored";
                            parseContext.handleError(ParseContext::SCHEDULE_IGNORED_GUIDE_RATE, msg, errors);
                        } else {
                            guide_rate = record.getItem("GUIDE_RATE").get<double>(0);
                            if (guide_rate == 0)
                                guide_rate_def = Group::GuideRateTarget::POTN;
                        }
                    }
                }
                auto resv_target = record.getItem("RESERVOIR_FLUID_TARGET").getSIDouble(0);
                {
                    auto group_ptr = std::make_shared<Group>(this->getGroup(group_name, currentStep));
                    Group::GroupProductionProperties production;
                    production.cmode = controlMode;
                    production.oil_target = oil_target;
                    production.gas_target = gas_target;
                    production.water_target = water_target;
                    production.liquid_target = liquid_target;
                    production.guide_rate = guide_rate;
                    production.guide_rate_def = guide_rate_def;
                    production.resv_target = resv_target;
                    if ((production.cmode == Group::ProductionCMode::ORAT) ||
                        (production.cmode == Group::ProductionCMode::WRAT) ||
                        (production.cmode == Group::ProductionCMode::GRAT) ||
                        (production.cmode == Group::ProductionCMode::LRAT))
                        production.exceed_action = Group::ExceedAction::RATE;
                    else
                        production.exceed_action = exceedAction;

                    production.production_controls = 0;

                    if (!record.getItem("OIL_TARGET").defaultApplied(0))
                        production.production_controls += static_cast<int>(Group::ProductionCMode::ORAT);

                    if (!record.getItem("GAS_TARGET").defaultApplied(0))
                        production.production_controls += static_cast<int>(Group::ProductionCMode::GRAT);

                    if (!record.getItem("WATER_TARGET").defaultApplied(0))
                        production.production_controls += static_cast<int>(Group::ProductionCMode::WRAT);

                    if (!record.getItem("LIQUID_TARGET").defaultApplied(0))
                        production.production_controls += static_cast<int>(Group::ProductionCMode::LRAT);

                    if (!record.getItem("RESERVOIR_FLUID_TARGET").defaultApplied(0))
                        production.production_controls += static_cast<int>(Group::ProductionCMode::RESV);

                    if (group_ptr->updateProduction(production)) {
                        auto new_config = std::make_shared<GuideRateConfig>( this->guideRateConfig(currentStep) );
                        new_config->update_group(*group_ptr);
                        this->guide_rate_config.update( currentStep, std::move(new_config) );

                        this->updateGroup(std::move(group_ptr), currentStep);
                        m_events.addEvent( ScheduleEvents::GROUP_PRODUCTION_UPDATE , currentStep);
                        this->addWellGroupEvent(group_name, ScheduleEvents::GROUP_PRODUCTION_UPDATE, currentStep);
                    }
                }
            }
        }
    }


    void Schedule::handleGEFAC( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& groupNamePattern = record.getItem("GROUP").getTrimmedString(0);
            const auto group_names = this->groupNames(groupNamePattern);

            if (group_names.empty())
                invalidNamePattern(groupNamePattern, parseContext, errors, keyword);

            for (const auto& group_name : group_names){
                bool transfer = DeckItem::to_bool(record.getItem("TRANSFER_EXT_NET").getTrimmedString(0));
                auto gefac = record.getItem("EFFICIENCY_FACTOR").get< double >(0);
                {
                    auto group_ptr = std::make_shared<Group>(this->getGroup(group_name, currentStep));
                    if (group_ptr->update_gefac(gefac, transfer))
                        this->updateGroup(std::move(group_ptr), currentStep);
                }
            }
        }
    }

    void Schedule::handleGCONSALE( const DeckKeyword& keyword, size_t currentStep, const UnitSystem& unit_system) {
        const auto& current = *this->gconsale.get(currentStep);
        std::shared_ptr<GConSale> new_gconsale(new GConSale(current));
        for( const auto& record : keyword ) {
            const std::string& groupName = record.getItem("GROUP").getTrimmedString(0);
            auto sales_target = record.getItem("SALES_TARGET").get<UDAValue>(0);
            auto max_rate = record.getItem("MAX_SALES_RATE").get<UDAValue>(0);
            auto min_rate = record.getItem("MIN_SALES_RATE").get<UDAValue>(0);
            std::string procedure = record.getItem("MAX_PROC").getTrimmedString(0);
            auto udqconfig = this->getUDQConfig(currentStep).params().undefinedValue();

            new_gconsale->add(groupName, sales_target, max_rate, min_rate, procedure, udqconfig, unit_system);

            auto group_ptr = std::make_shared<Group>(this->getGroup(groupName, currentStep));
            Group::GroupInjectionProperties injection;
            injection.phase = Phase::GAS;
            if (group_ptr->updateInjection(injection)) {
                this->updateGroup(std::move(group_ptr), currentStep);
            }
        }
        this->gconsale.update(currentStep, new_gconsale);
    }


    void Schedule::handleGCONSUMP( const DeckKeyword& keyword, size_t currentStep, const UnitSystem& unit_system) {
        const auto& current = *this->gconsump.get(currentStep);
        std::shared_ptr<GConSump> new_gconsump(new GConSump(current));
        for( const auto& record : keyword ) {
            const std::string& groupName = record.getItem("GROUP").getTrimmedString(0);
            auto consumption_rate = record.getItem("GAS_CONSUMP_RATE").get<UDAValue>(0);
            auto import_rate = record.getItem("GAS_IMPORT_RATE").get<UDAValue>(0);

            std::string network_node_name;
            auto network_node = record.getItem("NETWORK_NODE");
            if (!network_node.defaultApplied(0))
                network_node_name = network_node.getTrimmedString(0);

            auto udqconfig = this->getUDQConfig(currentStep).params().undefinedValue();

            new_gconsump->add(groupName, consumption_rate, import_rate, network_node_name, udqconfig, unit_system);
        }
        this->gconsump.update(currentStep, new_gconsump);
    }


    void Schedule::handleGUIDERAT( const DeckKeyword& keyword, size_t currentStep) {
        const auto& record = keyword.getRecord(0);

        double min_calc_delay = record.getItem<ParserKeywords::GUIDERAT::MIN_CALC_TIME>().getSIDouble(0);
        auto phase = GuideRateModel::TargetFromString(record.getItem<ParserKeywords::GUIDERAT::NOMINATED_PHASE>().getTrimmedString(0));
        double A = record.getItem<ParserKeywords::GUIDERAT::A>().get<double>(0);
        double B = record.getItem<ParserKeywords::GUIDERAT::B>().get<double>(0);
        double C = record.getItem<ParserKeywords::GUIDERAT::C>().get<double>(0);
        double D = record.getItem<ParserKeywords::GUIDERAT::D>().get<double>(0);
        double E = record.getItem<ParserKeywords::GUIDERAT::E>().get<double>(0);
        double F = record.getItem<ParserKeywords::GUIDERAT::F>().get<double>(0);
        bool allow_increase = DeckItem::to_bool( record.getItem<ParserKeywords::GUIDERAT::ALLOW_INCREASE>().getTrimmedString(0));
        double damping_factor = record.getItem<ParserKeywords::GUIDERAT::DAMPING_FACTOR>().get<double>(0);
        bool use_free_gas = DeckItem::to_bool( record.getItem<ParserKeywords::GUIDERAT::USE_FREE_GAS>().getTrimmedString(0));

        GuideRateModel new_model = GuideRateModel(min_calc_delay, phase, A, B, C, D, E, F, allow_increase, damping_factor, use_free_gas);
        auto new_config = std::make_shared<GuideRateConfig>( this->guideRateConfig(currentStep) );
        if (new_config->update_model(new_model))
            this->guide_rate_config.update( currentStep, new_config );
    }


    void Schedule::handleLINCOM( const DeckKeyword& keyword, size_t currentStep) {
        const auto& record = keyword.getRecord(0);
        auto alpha = record.getItem<ParserKeywords::LINCOM::ALPHA>().get<UDAValue>(0);
        auto beta  = record.getItem<ParserKeywords::LINCOM::BETA>().get<UDAValue>(0);
        auto gamma = record.getItem<ParserKeywords::LINCOM::GAMMA>().get<UDAValue>(0);
        auto new_config = std::make_shared<GuideRateConfig>( this->guideRateConfig(currentStep) );
        auto new_model = new_config->model();

        if (new_model.updateLINCOM(alpha, beta, gamma)) {
            new_config->update_model(new_model);
            this->guide_rate_config.update( currentStep, new_config );
        }
    }


    void Schedule::handleTUNING( const DeckKeyword& keyword, size_t currentStep) {

        int numrecords = keyword.size();

        if (numrecords > 0) {
            const auto& record1 = keyword.getRecord(0);

            double TSINIT = record1.getItem("TSINIT").getSIDouble(0);
            this->m_tuning.setTSINIT(currentStep, TSINIT);

            double TSMAXZ = record1.getItem("TSMAXZ").getSIDouble(0);
            this->m_tuning.setTSMAXZ(currentStep, TSMAXZ);

            double TSMINZ = record1.getItem("TSMINZ").getSIDouble(0);
            this->m_tuning.setTSMINZ(currentStep, TSMINZ);

            double TSMCHP = record1.getItem("TSMCHP").getSIDouble(0);
            this->m_tuning.setTSMCHP(currentStep, TSMCHP);

            double TSFMAX = record1.getItem("TSFMAX").get< double >(0);
            this->m_tuning.setTSFMAX(currentStep, TSFMAX);

            double TSFMIN = record1.getItem("TSFMIN").get< double >(0);
            this->m_tuning.setTSFMIN(currentStep, TSFMIN);

            double TSFCNV = record1.getItem("TSFCNV").get< double >(0);
            this->m_tuning.setTSFCNV(currentStep, TSFCNV);

            double TFDIFF = record1.getItem("TFDIFF").get< double >(0);
            this->m_tuning.setTFDIFF(currentStep, TFDIFF);

            double THRUPT = record1.getItem("THRUPT").get< double >(0);
            this->m_tuning.setTHRUPT(currentStep, THRUPT);

            const auto& TMAXWCdeckItem = record1.getItem("TMAXWC");
            if (TMAXWCdeckItem.hasValue(0)) {
                double TMAXWC = TMAXWCdeckItem.getSIDouble(0);
                this->m_tuning.setTMAXWC(currentStep, TMAXWC);
            }
        }


        if (numrecords > 1) {
            const auto& record2 = keyword.getRecord(1);

            double TRGTTE = record2.getItem("TRGTTE").get< double >(0);
            this->m_tuning.setTRGTTE(currentStep, TRGTTE);

            double TRGCNV = record2.getItem("TRGCNV").get< double >(0);
            this->m_tuning.setTRGCNV(currentStep, TRGCNV);

            double TRGMBE = record2.getItem("TRGMBE").get< double >(0);
            this->m_tuning.setTRGMBE(currentStep, TRGMBE);

            double TRGLCV = record2.getItem("TRGLCV").get< double >(0);
            this->m_tuning.setTRGLCV(currentStep, TRGLCV);

            double XXXTTE = record2.getItem("XXXTTE").get< double >(0);
            this->m_tuning.setXXXTTE(currentStep, XXXTTE);

            double XXXCNV = record2.getItem("XXXCNV").get< double >(0);
            this->m_tuning.setXXXCNV(currentStep, XXXCNV);

            double XXXMBE = record2.getItem("XXXMBE").get< double >(0);
            this->m_tuning.setXXXMBE(currentStep, XXXMBE);

            double XXXLCV = record2.getItem("XXXLCV").get< double >(0);
            this->m_tuning.setXXXLCV(currentStep, XXXLCV);

            double XXXWFL = record2.getItem("XXXWFL").get< double >(0);
            this->m_tuning.setXXXWFL(currentStep, XXXWFL);

            double TRGFIP = record2.getItem("TRGFIP").get< double >(0);
            this->m_tuning.setTRGFIP(currentStep, TRGFIP);

            const auto& TRGSFTdeckItem = record2.getItem("TRGSFT");
            if (TRGSFTdeckItem.hasValue(0)) {
                double TRGSFT = TRGSFTdeckItem.get< double >(0);
                this->m_tuning.setTRGSFT(currentStep, TRGSFT);
            }

            double THIONX = record2.getItem("THIONX").get< double >(0);
            this->m_tuning.setTHIONX(currentStep, THIONX);

            int TRWGHT = record2.getItem("TRWGHT").get< int >(0);
            this->m_tuning.setTRWGHT(currentStep, TRWGHT);
        }


        if (numrecords > 2) {
            const auto& record3 = keyword.getRecord(2);

            int NEWTMX = record3.getItem("NEWTMX").get< int >(0);
            this->m_tuning.setNEWTMX(currentStep, NEWTMX);

            int NEWTMN = record3.getItem("NEWTMN").get< int >(0);
            this->m_tuning.setNEWTMN(currentStep, NEWTMN);

            int LITMAX = record3.getItem("LITMAX").get< int >(0);
            this->m_tuning.setLITMAX(currentStep, LITMAX);

            int LITMIN = record3.getItem("LITMIN").get< int >(0);
            this->m_tuning.setLITMIN(currentStep, LITMIN);

            int MXWSIT = record3.getItem("MXWSIT").get< int >(0);
            this->m_tuning.setMXWSIT(currentStep, MXWSIT);

            int MXWPIT = record3.getItem("MXWPIT").get< int >(0);
            this->m_tuning.setMXWPIT(currentStep, MXWPIT);

            double DDPLIM = record3.getItem("DDPLIM").getSIDouble(0);
            this->m_tuning.setDDPLIM(currentStep, DDPLIM);

            double DDSLIM = record3.getItem("DDSLIM").get< double >(0);
            this->m_tuning.setDDSLIM(currentStep, DDSLIM);

            double TRGDPR = record3.getItem("TRGDPR").getSIDouble(0);
            this->m_tuning.setTRGDPR(currentStep, TRGDPR);

            const auto& XXXDPRdeckItem = record3.getItem("XXXDPR");
            if (XXXDPRdeckItem.hasValue(0)) {
                double XXXDPR = XXXDPRdeckItem.getSIDouble(0);
                this->m_tuning.setXXXDPR(currentStep, XXXDPR);
            }
        }
        m_events.addEvent( ScheduleEvents::TUNING_CHANGE , currentStep);

    }


    void Schedule::handleMESSAGES( const DeckKeyword& keyword, size_t currentStep) {
        const auto& record = keyword.getRecord(0);
        using  set_limit_fptr = decltype( std::mem_fn( &MessageLimits::setMessagePrintLimit ) );
        static const std::pair<std::string , set_limit_fptr> setters[] = {
            {"MESSAGE_PRINT_LIMIT" , std::mem_fn( &MessageLimits::setMessagePrintLimit)},
            {"COMMENT_PRINT_LIMIT" , std::mem_fn( &MessageLimits::setCommentPrintLimit)},
            {"WARNING_PRINT_LIMIT" , std::mem_fn( &MessageLimits::setWarningPrintLimit)},
            {"PROBLEM_PRINT_LIMIT" , std::mem_fn( &MessageLimits::setProblemPrintLimit)},
            {"ERROR_PRINT_LIMIT"   , std::mem_fn( &MessageLimits::setErrorPrintLimit)},
            {"BUG_PRINT_LIMIT"     , std::mem_fn( &MessageLimits::setBugPrintLimit)},
            {"MESSAGE_STOP_LIMIT"  , std::mem_fn( &MessageLimits::setMessageStopLimit)},
            {"COMMENT_STOP_LIMIT"  , std::mem_fn( &MessageLimits::setCommentStopLimit)},
            {"WARNING_STOP_LIMIT"  , std::mem_fn( &MessageLimits::setWarningStopLimit)},
            {"PROBLEM_STOP_LIMIT"  , std::mem_fn( &MessageLimits::setProblemStopLimit)},
            {"ERROR_STOP_LIMIT"    , std::mem_fn( &MessageLimits::setErrorStopLimit)},
            {"BUG_STOP_LIMIT"      , std::mem_fn( &MessageLimits::setBugStopLimit)}};

        for (const auto& pair : setters) {
            const auto& item = record.getItem( pair.first );
            if (!item.defaultApplied(0)) {
                const set_limit_fptr& fptr = pair.second;
                int value = item.get<int>(0);
                fptr( this->m_messageLimits , currentStep , value );
            }
        }
    }

#ifdef ENABLE_3DPROPS_TESTING
    void Schedule::handleCOMPDAT( const DeckKeyword& keyword, size_t currentStep, const EclipseGrid& grid, const FieldPropsManager& fp, const Eclipse3DProperties&, const ParseContext& parseContext, ErrorGuard& errors) {
#else
    void Schedule::handleCOMPDAT( const DeckKeyword& keyword, size_t currentStep, const EclipseGrid& grid, const FieldPropsManager& , const Eclipse3DProperties& eclipseProperties, const ParseContext& parseContext, ErrorGuard& errors) {
#endif
        for (const auto& record : keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            auto wellnames = this->wellNames(wellNamePattern, currentStep);
            if (wellnames.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for (const auto& name : wellnames) {
                {
                    auto well2 = std::shared_ptr<Well>(new Well( this->getWell(name, currentStep)));
                    auto connections = std::shared_ptr<WellConnections>( new WellConnections( well2->getConnections()));
#ifdef ENABLE_3DPROPS_TESTING
                    connections->loadCOMPDAT(record, grid, fp);
#else
                    connections->loadCOMPDAT(record, grid, eclipseProperties);
#endif
                    /*
                      This block implements the following dubious logic.

                        1. All competions are shut.
                        2. A new open completion is added.
                        3. A currently SHUT well is opened.

                      This code assumes that the reason the well is initially
                      shut is due to all the shut completions, if the well was
                      explicitly shut for another reason the explicit opening of
                      the well might be in error?
                    */
                    /*if (all_shut0) {
                        if (!connections->allConnectionsShut()) {
                            if (well2->getStatus() == WellCommon::StatusEnum::SHUT) {
                                printf("Running all_shut inner loop\n");
                                if (this->updateWellStatus(well2->name(), currentStep, WellCommon::StatusEnum::OPEN))
                                    // Refresh pointer if the status has updated current slot. Ugly
                                    well2 = std::shared_ptr<Well>(new Well(this->getWell(name, currentStep)));
                            }
                        }
                    }
                    */
                    if (well2->updateConnections(connections))
                        this->updateWell(well2, currentStep);

                    if (well2->getStatus() == Well::Status::SHUT) {
                        std::string msg =
                            "All completions in well " + well2->name() + " is shut at " + std::to_string ( m_timeMap.getTimePassedUntil(currentStep) / (60*60*24) ) + " days. \n" +
                            "The well is therefore also shut.";
                        OpmLog::note(msg);
                    }
                }
                this->addWellGroupEvent(name, ScheduleEvents::COMPLETION_CHANGE, currentStep);
            }
        }
        m_events.addEvent(ScheduleEvents::COMPLETION_CHANGE, currentStep);
    }




    void Schedule::handleWELSEGS( const DeckKeyword& keyword, size_t currentStep) {
        const auto& record1 = keyword.getRecord(0);
        const auto& wname = record1.getItem("WELL").getTrimmedString(0);
        {
            auto& dynamic_state = this->wells_static.at(wname);
            auto well_ptr = std::make_shared<Well>( *dynamic_state[currentStep] );
            if (well_ptr->handleWELSEGS(keyword))
                this->updateWell(well_ptr, currentStep);
        }
    }

    void Schedule::handleCOMPSEGS( const DeckKeyword& keyword, size_t currentStep, const EclipseGrid& grid,
                                   const ParseContext& parseContext, ErrorGuard& errors) {
        const auto& record1 = keyword.getRecord(0);
        const std::string& well_name = record1.getItem("WELL").getTrimmedString(0);
        {
            auto& dynamic_state = this->wells_static.at(well_name);
            auto well_ptr = std::make_shared<Well>( *dynamic_state[currentStep] );
            if (well_ptr->handleCOMPSEGS(keyword, grid, parseContext, errors))
                this->updateWell(well_ptr, currentStep);
        }
    }

    void Schedule::handleWSEGSICD( const DeckKeyword& keyword, size_t currentStep) {

        const std::map<std::string, std::vector<std::pair<int, SpiralICD> > > spiral_icds =
                                SpiralICD::fromWSEGSICD(keyword);

        for (const auto& map_elem : spiral_icds) {
            const std::string& well_name_pattern = map_elem.first;
            const auto well_names = this->wellNames(well_name_pattern, currentStep);
            const std::vector<std::pair<int, SpiralICD> >& sicd_pairs = map_elem.second;

            for (const auto& well_name : well_names) {
                auto& dynamic_state = this->wells_static.at(well_name);
                auto well_ptr = std::make_shared<Well>( *dynamic_state[currentStep] );
                if (well_ptr -> updateWSEGSICD(sicd_pairs) )
                    this->updateWell(well_ptr, currentStep);
            }
        }
    }
	    	    
    void Schedule::handleWSEGVALV( const DeckKeyword& keyword, size_t currentStep) {
        const std::map<std::string, std::vector<std::pair<int, Valve> > > valves = Valve::fromWSEGVALV(keyword);

        for (const auto& map_elem : valves) {
            const std::string& well_name_pattern = map_elem.first;
            const auto well_names = this->wellNames(well_name_pattern, currentStep);
            const std::vector<std::pair<int, Valve> >& valve_pairs = map_elem.second;

            for (const auto& well_name : well_names) {
                auto& dynamic_state = this->wells_static.at(well_name);
                auto well_ptr = std::make_shared<Well>( *dynamic_state[currentStep] );
                if (well_ptr -> updateWSEGVALV(valve_pairs) )
                    this->updateWell(well_ptr, currentStep);
            }
        }
    }

    void Schedule::handleWGRUPCON( const DeckKeyword& keyword, size_t currentStep) {
        for( const auto& record : keyword ) {
            const auto well_names = this->wellNames(record.getItem("WELL").getTrimmedString(0), currentStep);
            for (const auto& well_name : well_names) {
                bool availableForGroupControl = DeckItem::to_bool(record.getItem("GROUP_CONTROLLED").getTrimmedString(0));
                double guide_rate = record.getItem("GUIDE_RATE").get< double >(0);
                double scaling_factor = record.getItem("SCALING_FACTOR").get< double >(0);
                auto phase = Well::GuideRateTarget::UNDEFINED;
                if (!record.getItem("PHASE").defaultApplied(0)) {
                    std::string guideRatePhase = record.getItem("PHASE").getTrimmedString(0);
                    phase = Well::GuideRateTargetFromString(guideRatePhase);
                }

                {
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well_ptr = std::make_shared<Well>( *dynamic_state[currentStep] );
                    if (well_ptr->updateWellGuideRate(availableForGroupControl, guide_rate, phase, scaling_factor)) {
                        this->updateWell(well_ptr, currentStep);

                        auto new_config = std::make_shared<GuideRateConfig>( this->guideRateConfig(currentStep) );
                        new_config->update_well(*well_ptr);
                        this->guide_rate_config.update( currentStep, std::move(new_config) );
                    }
                }
            }
        }
    }

void Schedule::handleGRUPTREE( const DeckKeyword& keyword, size_t currentStep, const UnitSystem& unit_system, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& childName = trim_wgname(keyword, record.getItem("CHILD_GROUP").get<std::string>(0), parseContext, errors);
            const std::string& parentName = trim_wgname(keyword, record.getItem("PARENT_GROUP").get<std::string>(0), parseContext, errors);

            if (!hasGroup(parentName))
                addGroup( parentName , currentStep, unit_system );

            if (!hasGroup(childName))
                addGroup( childName , currentStep, unit_system );

            this->addGroupToGroup(parentName, childName, currentStep);
        }
    }


    void Schedule::handleGRUPNET( const DeckKeyword& keyword, size_t currentStep, const UnitSystem& unit_system) {
        for( const auto& record : keyword ) {
            const auto& groupName = record.getItem("NAME").getTrimmedString(0);

            if (!hasGroup(groupName))
                addGroup(groupName , currentStep, unit_system);

            int table = record.getItem("VFP_TABLE").get< int >(0);
            {
                auto group_ptr = std::make_shared<Group>( this->getGroup(groupName, currentStep) );
                if (group_ptr->updateNetVFPTable(table))
                    this->updateGroup(std::move(group_ptr), currentStep);
            }
        }
    }

    void Schedule::handleWRFT( const DeckKeyword& keyword, size_t currentStep) {

        /* Rule for handling RFT: Request current RFT data output for specified wells, plus output when
         * any well is subsequently opened
         */

        for( const auto& record : keyword ) {

            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, currentStep);
            for(const auto& well_name : well_names)
                this->rft_config.updateRFT(well_name, currentStep, RFTConfig::RFT::YES);

        }

        this->rft_config.setWellOpenRFT(currentStep);
    }

    void Schedule::handleWRFTPLT( const DeckKeyword& keyword,  size_t currentStep) {
        for( const auto& record : keyword ) {

            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);

            RFTConfig::RFT RFTKey = RFTConfig::RFTFromString(record.getItem("OUTPUT_RFT").getTrimmedString(0));
            RFTConfig::PLT PLTKey = RFTConfig::PLTFromString(record.getItem("OUTPUT_PLT").getTrimmedString(0));
            const auto well_names = wellNames(wellNamePattern, currentStep);
            for(const auto& well_name : well_names) {
                this->rft_config.updateRFT(well_name, currentStep, RFTKey);
                this->rft_config.updatePLT(well_name, currentStep, PLTKey);
            }
        }
    }

    const RFTConfig& Schedule::rftConfig() const {
        return this->rft_config;
    }

    void Schedule::invalidNamePattern( const std::string& namePattern,  const ParseContext& parseContext, ErrorGuard& errors, const DeckKeyword& keyword ) const {
        std::string msg = "Error when handling " + keyword.name() +". No names match " +
                          namePattern;
        parseContext.handleError( ParseContext::SCHEDULE_INVALID_NAME, msg, errors );
    }

    const TimeMap& Schedule::getTimeMap() const {
        return this->m_timeMap;
    }


    GTNode Schedule::groupTree(const std::string& root_node, std::size_t report_step, const GTNode * parent) const {
        auto root_group = this->getGroup(root_node, report_step);
        GTNode tree(root_group, parent);

        for (const auto& wname : root_group.wells()) {
            const auto& well = this->getWell(wname, report_step);
            tree.add_well(well);
        }

        for (const auto& gname : root_group.groups()) {
            auto child_group = this->groupTree(gname, report_step, std::addressof(tree));
            tree.add_group(child_group);
        }

        return tree;
    }

    GTNode Schedule::groupTree(const std::string& root_node, std::size_t report_step) const {
        return this->groupTree(root_node, report_step, nullptr);
    }


    GTNode Schedule::groupTree(std::size_t report_step) const {
        return this->groupTree("FIELD", report_step);
    }

    void Schedule::addWell(const std::string& wellName,
                           const DeckRecord& record,
                           size_t timeStep,
                           Connection::Order wellConnectionOrder,
                           const UnitSystem& unit_system) {

        // We change from eclipse's 1 - n, to a 0 - n-1 solution
        int headI = record.getItem("HEAD_I").get< int >(0) - 1;
        int headJ = record.getItem("HEAD_J").get< int >(0) - 1;

        const std::string phaseStr = record.getItem("PHASE").getTrimmedString(0);
        Phase preferredPhase;
        if (phaseStr == "LIQ") {
            // We need a workaround in case the preferred phase is "LIQ",
            // which is not proper phase and will cause the get_phase()
            // function to throw. In that case we choose to treat it as OIL.
            preferredPhase = Phase::OIL;
            OpmLog::warning("LIQ_PREFERRED_PHASE",
                            "LIQ preferred phase not supported for well " + wellName + ", using OIL instead");
        } else {
            // Normal case.
            preferredPhase = get_phase(phaseStr);
        }

        const auto& refDepthItem = record.getItem("REF_DEPTH");

        double refDepth = refDepthItem.hasValue( 0 )
                        ? refDepthItem.getSIDouble( 0 )
                        : -1.0;

        double drainageRadius = record.getItem( "D_RADIUS" ).getSIDouble(0);

        bool allowCrossFlow = true;
        const std::string& allowCrossFlowStr = record.getItem<ParserKeywords::WELSPECS::CROSSFLOW>().getTrimmedString(0);
        if (allowCrossFlowStr == "NO")
            allowCrossFlow = false;

        bool automaticShutIn = true;
        const std::string& automaticShutInStr = record.getItem<ParserKeywords::WELSPECS::AUTO_SHUTIN>().getTrimmedString(0);
        if (automaticShutInStr == "STOP") {
            automaticShutIn = false;
        }
        {
            wells_static.insert( std::make_pair(wellName, DynamicState<std::shared_ptr<Well>>(m_timeMap, nullptr)));

            auto& dynamic_state = wells_static.at(wellName);
            const std::string& group = record.getItem<ParserKeywords::WELSPECS::GROUP>().getTrimmedString(0);
            std::size_t insert_index = this->wells_static.size() - 1;
            auto well_ptr = std::make_shared<Well>(wellName,
                                                    group,
                                                    timeStep,
                                                    insert_index,
                                                    headI, headJ,
                                                    refDepth,
                                                    preferredPhase,
                                                    this->global_whistctl_mode[timeStep],
                                                    wellConnectionOrder,
                                                    unit_system,
                                                    this->getUDQConfig(timeStep).params().undefinedValue());

            well_ptr->updateCrossFlow(allowCrossFlow);
            well_ptr->updateAutoShutin(automaticShutIn);
            well_ptr->updateDrainageRadius(drainageRadius);

            dynamic_state.update(timeStep, well_ptr);
        }
        m_events.addEvent( ScheduleEvents::NEW_WELL , timeStep );
        wellgroup_events.insert( std::make_pair(wellName, Events(this->m_timeMap)));
        this->addWellGroupEvent(wellName, ScheduleEvents::NEW_WELL, timeStep);
    }

    size_t Schedule::numWells() const {
        return wells_static.size();
    }

    size_t Schedule::numWells(size_t timestep) const {
        auto well_names = this->wellNames(timestep);
        return well_names.size();
    }

    bool Schedule::hasWell(const std::string& wellName) const {
        return wells_static.count( wellName ) > 0;
    }

    bool Schedule::hasWell(const std::string& wellName, std::size_t timeStep) const {
        if (this->wells_static.count(wellName) == 0)
            return false;

        const auto& well = this->getWellatEnd(wellName);
        return well.hasBeenDefined(timeStep);
    }

    std::vector< const Group* > Schedule::getChildGroups2(const std::string& group_name, size_t timeStep) const {
        if (!hasGroup(group_name))
            throw std::invalid_argument("No such group: '" + group_name + "'");
        {
            const auto& group = getGroup( group_name, timeStep );
            std::vector<const Group*> child_groups;

            if (group.defined( timeStep )) {
                for (const auto& child_name : group.groups())
                    child_groups.push_back( std::addressof(this->getGroup(child_name, timeStep)));
            }
            return child_groups;
        }
    }


    std::vector< Well > Schedule::getChildWells2(const std::string& group_name, size_t timeStep) const {
        if (!hasGroup(group_name))
            throw std::invalid_argument("No such group: '" + group_name + "'");
        {
            const auto& dynamic_state = this->groups.at(group_name);
            const auto& group_ptr = dynamic_state.get(timeStep);
            if (group_ptr) {
                std::vector<Well> wells;

                if (group_ptr->groups().size()) {
                    for (const auto& child_name : group_ptr->groups()) {
                        const auto& child_wells = getChildWells2( child_name, timeStep);
                        wells.insert( wells.end() , child_wells.begin() , child_wells.end());
                    }
                } else {
                    for (const auto& well_name : group_ptr->wells( ))
                        wells.push_back( this->getWell( well_name, timeStep ));
                }

                return wells;
            } else
                return {};
        }
    }



    std::vector<Well> Schedule::getWells(size_t timeStep) const {
        std::vector<Well> wells;
        if (timeStep >= this->m_timeMap.size())
            throw std::invalid_argument("timeStep argument beyond the length of the simulation");

        for (const auto& dynamic_pair : this->wells_static) {
            auto& well_ptr = dynamic_pair.second.get(timeStep);
            if (well_ptr)
                wells.push_back(*well_ptr.get());
        }
        return wells;
    }

    std::vector<Well> Schedule::getWellsatEnd() const {
        return this->getWells(this->m_timeMap.size() - 1);
    }


    const Well& Schedule::getWellatEnd(const std::string& well_name) const {
        return this->getWell(well_name, this->m_timeMap.size() - 1);
    }

    const Well& Schedule::getWell(const std::string& wellName, size_t timeStep) const {
        if (this->wells_static.count(wellName) == 0)
            throw std::invalid_argument("No such well: " + wellName);

        const auto& dynamic_state = this->wells_static.at(wellName);
        auto& well_ptr = dynamic_state.get(timeStep);
        if (!well_ptr)
            throw std::invalid_argument("Well: " + wellName + " not yet defined at step: " + std::to_string(timeStep));

        return *well_ptr;
    }

    const Group& Schedule::getGroup(const std::string& groupName, size_t timeStep) const {
        if (this->groups.count(groupName) == 0)
            throw std::invalid_argument("No such group: '" + groupName + "'");

        const auto& dynamic_state = this->groups.at(groupName);
        auto& group_ptr = dynamic_state.get(timeStep);
        if (!group_ptr)
            throw std::invalid_argument("Group: " + groupName + " not yet defined at step: " + std::to_string(timeStep));

        return *group_ptr;
    }

    void Schedule::updateGroup(std::shared_ptr<Group> group, size_t reportStep) {
        auto& dynamic_state = this->groups.at(group->name());
        dynamic_state.update(reportStep, std::move(group));
    }

    /*
      There are many SCHEDULE keyword which take a wellname as argument. In
      addition to giving a fully qualified name like 'W1' you can also specify
      shell wildcard patterns like like 'W*', you can get all the wells in the
      well-list '*WL'[1] and the wellname '?' is used to get all the wells which
      already have matched a condition in a ACTIONX keyword. This function
      should be one-stop function to get all well names according to a input
      pattern. The timestep argument is used to check that the wells have
      indeed been defined at the point in time we are considering.

      [1]: The leading '*' in a WLIST name should not be interpreted as a shell
           wildcard!
    */


    std::vector<std::string> Schedule::wellNames(const std::string& pattern, size_t timeStep, const std::vector<std::string>& matching_wells) const {
        if (pattern.size() == 0)
            return {};

        // WLIST
        if (pattern[0] == '*' && pattern.size() > 1) {
            const auto& wlm = this->getWListManager(timeStep);
            if (wlm.hasList(pattern)) {
                const auto& wlist = wlm.getList(pattern);
                return { wlist.begin(), wlist.end() };
            } else
                return {};
        }

        // Normal pattern matching
        auto star_pos = pattern.find('*');
        if (star_pos != std::string::npos) {
            std::vector<std::string> names;
            for (const auto& well_pair : this->wells_static) {
                if (name_match(pattern, well_pair.first)) {
                    const auto& dynamic_state = well_pair.second;
                    if (dynamic_state.get(timeStep))
                        names.push_back(well_pair.first);
                }
            }
            return names;
        }

        // ACTIONX handler
        if (pattern == "?")
            return { matching_wells.begin(), matching_wells.end() };

        // Normal well name without any special characters
        if (this->hasWell(pattern)) {
            const auto& dynamic_state = this->wells_static.at(pattern);
            if (dynamic_state.get(timeStep))
                return { pattern };
        }
        return {};
    }

    std::vector<std::string> Schedule::wellNames(const std::string& pattern) const {
        return this->wellNames(pattern, this->size() - 1);
    }

    std::vector<std::string> Schedule::wellNames(std::size_t timeStep) const {
        std::vector<std::string> names;
        for (const auto& well_pair : this->wells_static) {
            const auto& well_name = well_pair.first;
            const auto& dynamic_state = well_pair.second;
            auto open_step = static_cast<std::size_t>(dynamic_state.find_not(nullptr));
            if (open_step <= timeStep)
                names.push_back(well_name);
        }
        return names;
    }

    std::vector<std::string> Schedule::wellNames() const {
        std::vector<std::string> names;
        for (const auto& well_pair : this->wells_static)
            names.push_back(well_pair.first);

        return names;
    }

    std::vector<std::string> Schedule::groupNames(const std::string& pattern, size_t timeStep) const {
        if (pattern.size() == 0)
            return {};

        // Normal pattern matching
        auto star_pos = pattern.find('*');
        if (star_pos != std::string::npos) {
            std::vector<std::string> names;
            for (const auto& group_pair : this->groups) {
                if (name_match(pattern, group_pair.first)) {
                    const auto& dynamic_state = group_pair.second;
                    const auto& group_ptr = dynamic_state.get(timeStep);
                    if (group_ptr)
                        names.push_back(group_pair.first);
                }
            }
            return names;
        }

        // Normal group name without any special characters
        if (this->hasGroup(pattern)) {
            const auto& dynamic_state = this->groups.at(pattern);
            const auto& group_ptr = dynamic_state.get(timeStep);
            if (group_ptr)
                return { pattern };
        }
        return {};
    }

    std::vector<std::string> Schedule::groupNames(size_t timeStep) const {
        std::vector<std::string> names;
        for (const auto& group_pair : this->groups) {
            const auto& dynamic_state = group_pair.second;
            const auto& group_ptr = dynamic_state.get(timeStep);
            if (group_ptr)
                names.push_back(group_pair.first);
        }
        return names;
    }

    std::vector<std::string> Schedule::groupNames(const std::string& pattern) const {
        if (pattern.size() == 0)
            return {};

        // Normal pattern matching
        auto star_pos = pattern.find('*');
        if (star_pos != std::string::npos) {
            int flags = 0;
            std::vector<std::string> names;
            for (const auto& group_pair : this->groups) {
                if (fnmatch(pattern.c_str(), group_pair.first.c_str(), flags) == 0)
                    names.push_back(group_pair.first);
            }
            return names;
        }

        // Normal group name without any special characters
        if (this->hasGroup(pattern))
            return { pattern };

        return {};
    }

    std::vector<std::string> Schedule::groupNames() const {
        std::vector<std::string> names;
        for (const auto& group_pair : this->groups)
            names.push_back(group_pair.first);

        return names;
    }


    void Schedule::addGroup(const std::string& groupName, size_t timeStep, const UnitSystem& unit_system) {
        const size_t gseqIndex = this->groups.size();

        groups.insert( std::make_pair( groupName, DynamicState<std::shared_ptr<Group>>(this->m_timeMap, nullptr)));
        auto group_ptr = std::make_shared<Group>(groupName, gseqIndex, timeStep, this->getUDQConfig(timeStep).params().undefinedValue(), unit_system);
        auto& dynamic_state = this->groups.at(groupName);
        dynamic_state.update(timeStep, group_ptr);

        m_events.addEvent( ScheduleEvents::NEW_GROUP , timeStep );
        wellgroup_events.insert( std::make_pair(groupName, Events(this->m_timeMap)));
        this->addWellGroupEvent(groupName, ScheduleEvents::NEW_GROUP, timeStep);

        // All newly created groups are attached to the field group,
        // can then be relocated with the GRUPTREE keyword.
        if (groupName != "FIELD")
            this->addGroupToGroup("FIELD", *group_ptr, timeStep);
    }

    size_t Schedule::numGroups() const {
        return groups.size();
    }

    size_t Schedule::numGroups(size_t timeStep) const {
        const auto group_names = this->groupNames(timeStep);
        return group_names.size();
    }

    bool Schedule::hasGroup(const std::string& groupName) const {
        return groups.count(groupName) > 0;
    }

    bool Schedule::hasGroup(const std::string& groupName, std::size_t timeStep) const {
        if (timeStep >= this->size())
            return false;

        auto grpMap = this->groups.find(groupName);

        return (grpMap != this->groups.end())
            && grpMap->second.at(timeStep);
    }

    void Schedule::addGroupToGroup( const std::string& parent_group, const Group& child_group, size_t timeStep) {
        // Add to new parent
        auto& dynamic_state = this->groups.at(parent_group);
        auto parent_ptr = std::make_shared<Group>( *dynamic_state[timeStep] );
        if (parent_ptr->addGroup(child_group.name()))
            this->updateGroup(std::move(parent_ptr), timeStep);

        // Check and update backreference in child
        if (child_group.parent() != parent_group) {
            auto old_parent = std::make_shared<Group>( this->getGroup(child_group.parent(), timeStep) );
            old_parent->delGroup(child_group.name());
            this->updateGroup(std::move(old_parent), timeStep);

            auto child_ptr = std::make_shared<Group>( child_group );
            child_ptr->updateParent(parent_group);
            this->updateGroup(std::move(child_ptr), timeStep);

        }
    }

    void Schedule::addGroupToGroup( const std::string& parent_group, const std::string& child_group, size_t timeStep) {
        this->addGroupToGroup(parent_group, this->getGroup(child_group, timeStep), timeStep);
    }

    void Schedule::addWellToGroup( const std::string& group_name, const std::string& well_name , size_t timeStep) {
        const auto& well = this->getWell(well_name, timeStep);
        const auto old_gname = well.groupName();
        if (old_gname != group_name) {
            auto well_ptr = std::make_shared<Well>( well );
            well_ptr->updateGroup(group_name);
            this->updateWell(well_ptr, timeStep);
            this->addWellGroupEvent(well_ptr->name(), ScheduleEvents::WELL_WELSPECS_UPDATE, timeStep);

            // Remove well child reference from previous group
            auto group = std::make_shared<Group>(this->getGroup(old_gname, timeStep));
            group->delWell(well_name);
            this->updateGroup(std::move(group), timeStep);
        }

        // Add well child reference to new group
        auto group_ptr = std::make_shared<Group>(this->getGroup(group_name, timeStep));
        group_ptr->addWell(well_name);
        this->updateGroup(group_ptr, timeStep);
        this->m_events.addEvent( ScheduleEvents::GROUP_CHANGE , timeStep);
   }



    const Tuning& Schedule::getTuning() const {
        return this->m_tuning;
    }

    const Deck& Schedule::getModifierDeck(size_t timeStep) const {
        return m_modifierDeck.iget( timeStep );
    }

    const MessageLimits& Schedule::getMessageLimits() const {
        return m_messageLimits;
    }


    const Events& Schedule::getWellGroupEvents(const std::string& wellGroup) const {
        if (this->wellgroup_events.count(wellGroup) > 0)
            return this->wellgroup_events.at(wellGroup);
        else
            throw std::invalid_argument("No such well og group " + wellGroup);
    }

    void Schedule::addWellGroupEvent(const std::string& wellGroup, ScheduleEvents::Events event, size_t reportStep)  {
        auto& events = this->wellgroup_events.at(wellGroup);
        events.addEvent(event, reportStep);
    }

    bool Schedule::hasWellGroupEvent(const std::string& wellGroup, uint64_t event_mask, size_t reportStep) const {
        const auto& events = this->getWellGroupEvents(wellGroup);
        return events.hasEvent(event_mask, reportStep);
    }

    const Events& Schedule::getEvents() const {
        return this->m_events;
    }

    const OilVaporizationProperties& Schedule::getOilVaporizationProperties(size_t timestep) const {
        return m_oilvaporizationproperties.get(timestep);
    }

    const Well::ProducerCMode& Schedule::getGlobalWhistctlMmode(size_t timestep) const {
        return global_whistctl_mode.get(timestep);
    }


    bool Schedule::hasOilVaporizationProperties() const {
        for( size_t i = 0; i < this->m_timeMap.size(); ++i )
            if( m_oilvaporizationproperties.at( i ).defined() ) return true;

        return false;
    }

    void Schedule::checkIfAllConnectionsIsShut(size_t timeStep) {
        const auto& well_names = this->wellNames(timeStep);
        for (const auto& wname : well_names) {
            const auto& well = this->getWell(wname, timeStep);
            const auto& connections = well.getConnections();
            if (connections.allConnectionsShut())
                this->updateWellStatus( well.name(), timeStep, Well::Status::SHUT);
        }
    }


    void Schedule::filterConnections(const ActiveGridCells& grid) {
        for (auto& dynamic_pair : this->wells_static) {
            auto& dynamic_state = dynamic_pair.second;
            for (auto& well_pair : dynamic_state.unique()) {
                if (well_pair.second)
                    well_pair.second->filterConnections(grid);
            }
        }

        for (auto& dynamic_pair : this->wells_static) {
            auto& dynamic_state = dynamic_pair.second;
            for (auto& well_pair : dynamic_state.unique()) {
                if (well_pair.second)
                    well_pair.second->filterConnections(grid);
            }
        }

        for (auto& dynamic_pair : this->wells_static) {
            auto& dynamic_state = dynamic_pair.second;
            for (auto& well_pair : dynamic_state.unique()) {
                if (well_pair.second)
                    well_pair.second->filterConnections(grid);
            }
        }
    }

    const VFPProdTable& Schedule::getVFPProdTable(int table_id, size_t timeStep) const {
        const auto pair = vfpprod_tables.find(table_id);
        if (pair == vfpprod_tables.end())
            throw std::invalid_argument("No such table id: " + std::to_string(table_id));

        auto table_ptr = pair->second.get(timeStep);
        if (!table_ptr)
            throw std::invalid_argument("Table not yet defined at timeStep:" + std::to_string(timeStep));

        return *table_ptr;
    }

    const VFPInjTable& Schedule::getVFPInjTable(int table_id, size_t timeStep) const {
        const auto pair = vfpinj_tables.find(table_id);
        if (pair == vfpinj_tables.end())
            throw std::invalid_argument("No such table id: " + std::to_string(table_id));

        auto table_ptr = pair->second.get(timeStep);
        if (!table_ptr)
            throw std::invalid_argument("Table not yet defined at timeStep:" + std::to_string(timeStep));

        return *table_ptr;
    }

    std::map<int, std::shared_ptr<const VFPInjTable> > Schedule::getVFPInjTables(size_t timeStep) const {
        std::map<int, std::shared_ptr<const VFPInjTable> > tables;
        for (const auto& pair : this->vfpinj_tables) {
            if (pair.second.get(timeStep)) {
                tables.insert(std::make_pair(pair.first, pair.second.get(timeStep)) );
            }
        }
        return tables;
    }

    std::map<int, std::shared_ptr<const VFPProdTable> > Schedule::getVFPProdTables(size_t timeStep) const {
        std::map<int, std::shared_ptr<const VFPProdTable> > tables;
        for (const auto& pair : this->vfpprod_tables) {
            if (pair.second.get(timeStep)) {
                tables.insert(std::make_pair(pair.first, pair.second.get(timeStep)) );
            }
        }
        return tables;
    }

    const UDQActive& Schedule::udqActive(size_t timeStep) const {
        return *this->udq_active[timeStep];
    }

    void Schedule::updateUDQActive( size_t timeStep, std::shared_ptr<UDQActive> udq ) {
        this->udq_active.update(timeStep, udq);
    }

    const WellTestConfig& Schedule::wtestConfig(size_t timeStep) const {
        const auto& ptr = this->wtest_config.get(timeStep);
        return *ptr;
    }

    const GConSale& Schedule::gConSale(size_t timeStep) const {
        const auto& ptr = this->gconsale.get(timeStep);
        return *ptr;
    }

    const GConSump& Schedule::gConSump(size_t timeStep) const {
        const auto& ptr = this->gconsump.get(timeStep);
        return *ptr;
    }

    const WListManager& Schedule::getWListManager(size_t timeStep) const {
        const auto& ptr = this->wlist_manager.get(timeStep);
        return *ptr;
    }

    const UDQConfig& Schedule::getUDQConfig(size_t timeStep) const {
        const auto& ptr = this->udq_config.get(timeStep);
        return *ptr;
    }

    const GuideRateConfig& Schedule::guideRateConfig(size_t timeStep) const {
        const auto& ptr = this->guide_rate_config.get(timeStep);
        return *ptr;
    }


    size_t Schedule::size() const {
        return this->m_timeMap.size();
    }


    double  Schedule::seconds(size_t timeStep) const {
        return this->m_timeMap.seconds(timeStep);
    }

    time_t Schedule::simTime(size_t timeStep) const {
        return this->m_timeMap[timeStep];
    }

    double Schedule::stepLength(size_t timeStep) const {
        return this->m_timeMap.getTimeStepLength(timeStep);
    }


    const Action::Actions& Schedule::actions(std::size_t timeStep) const {
        const auto& ptr = this->m_actions.get(timeStep);
        return *ptr;
    }


    void Schedule::applyAction(size_t reportStep, const Action::ActionX& action, const Action::Result& result) {
        ParseContext parseContext;
        ErrorGuard errors;

        for (const auto& keyword : action) {
            if (!Action::ActionX::valid_keyword(keyword.name()))
                throw std::invalid_argument("The keyword: " + keyword.name() + " can not be handled in the ACTION body");

            if (keyword.name() == "WELOPEN")
                this->handleWELOPEN(keyword, reportStep, parseContext, errors, result.wells());
        }

    }

    int Schedule::getNupcol(size_t reportStep) const {
        return this->m_nupcol.get(reportStep);
    }

    const Schedule::WellMap& Schedule::getStaticWells() const {
        return wells_static;
    }

    const Schedule::GroupMap& Schedule::getGroups() const {
        return groups;
    }

    const DynamicState<OilVaporizationProperties>& Schedule::getOilVapProps() const {
        return m_oilvaporizationproperties;
    }

    const DynamicVector<Deck>& Schedule::getModifierDeck() const {
        return m_modifierDeck;
    }

    const Runspec& Schedule::getRunspec() const {
        return m_runspec;
    }

    const Schedule::VFPProdMap& Schedule::getVFPProdTables() const {
        return vfpprod_tables;
    }

    const Schedule::VFPInjMap& Schedule::getVFPInjTables() const {
        return vfpinj_tables;
    }

    const DynamicState<std::shared_ptr<WellTestConfig>>& Schedule::getWellTestConfig() const  {
        return wtest_config;
    }

    const DynamicState<std::shared_ptr<WListManager>>& Schedule::getWListManager() const {
        return wlist_manager;
    }

    const DynamicState<std::shared_ptr<UDQConfig>>& Schedule::getUDQConfig() const {
        return udq_config;
    }

    const DynamicState<std::shared_ptr<UDQActive>>& Schedule::getUDQActive() const {
        return udq_active;
    }

    const DynamicState<std::shared_ptr<GuideRateConfig>>& Schedule::getGuideRateConfig() const {
        return guide_rate_config;
    }

    const DynamicState<std::shared_ptr<GConSale>>& Schedule::getGConSale() const {
        return gconsale;
    }

    const DynamicState<std::shared_ptr<GConSump>>& Schedule::getGConSump() const {
        return gconsump;
    }

    const DynamicState<Well::ProducerCMode>& Schedule::getGlobalWhistCtlMode() const {
        return global_whistctl_mode;
    }

    const DynamicState<std::shared_ptr<Action::Actions>>& Schedule::getActions() const {
        return m_actions;
    }

    const DynamicState<int>& Schedule::getNupCol() const {
        return m_nupcol;
    }

     const std::map<std::string,Events>& Schedule::getWellGroupEvents() const {
        return wellgroup_events;
     }

     bool Schedule::operator==(const Schedule& data) const {
        auto&& comparePtr = [](const auto& t1, const auto& t2) {
                               if ((t1 && !t2) || (!t1 && t2))
                                   return false;
                               if (!t1)
                                   return true;

                               return *t1 == *t2;
        };

        auto&& compareDynState = [comparePtr](const auto& state1, const auto& state2) {
            if (state1.data().size() != state2.data().size())
                return false;
            return std::equal(state1.data().begin(), state1.data().end(),
                              state2.data().begin(), comparePtr);
        };

        auto&& compareMap = [comparePtr,
                             compareDynState](const auto& map1, const auto& map2) {
            if (map1.size() != map2.size())
                return false;
            auto it2 = map2.begin();
            for (const auto& it : map1) {
                if (it.first != it2->first)
                    return false;
                if (!compareDynState(it.second, it2->second))
                    return false;

                ++it2;
            }
            return true;
        };

        return this->getTimeMap() == data.getTimeMap() &&
               compareMap(this->getStaticWells(), data.getStaticWells()) &&
               compareMap(this->getGroups(), data.getGroups()) &&
               this->getOilVapProps() == data.getOilVapProps() &&
               this->getEvents() == data.getEvents() &&
               this->getModifierDeck() == data.getModifierDeck() &&
               this->getTuning() == data.getTuning() &&
               this->getMessageLimits() == data.getMessageLimits() &&
               this->getRunspec() == data.getRunspec() &&
               compareMap(this->getVFPProdTables(), data.getVFPProdTables()) &&
               compareMap(this->getVFPInjTables(), data.getVFPInjTables()) &&
               compareDynState(this->getWellTestConfig(), data.getWellTestConfig()) &&
               compareDynState(this->getWListManager(), data.getWListManager()) &&
               compareDynState(this->getUDQConfig(), data.getUDQConfig()) &&
               compareDynState(this->getUDQActive(), data.getUDQActive()) &&
               compareDynState(this->getGuideRateConfig(), data.getGuideRateConfig()) &&
               compareDynState(this->getGConSale(), data.getGConSale()) &&
               compareDynState(this->getGConSump(), data.getGConSump()) &&
               this->getGlobalWhistCtlMode() == data.getGlobalWhistCtlMode() &&
               compareDynState(this->getActions(), data.getActions()) &&
               this->rftConfig () == data.rftConfig() &&
               this->getNupCol() == data.getNupCol() &&
               this->getWellGroupEvents() == data.getWellGroupEvents();
     }

}
