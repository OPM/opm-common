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
#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/Parser/ParseContext.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/C.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/V.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/W.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Action/ActionX.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicVector.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Events.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/GroupTree.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/WellSegments.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/updatingConnectionsWithSegments.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/OilVaporizationProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQInput.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Tuning.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WList.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WListManager.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellPolymerProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellConnections.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/Units/Dimension.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>

#include "Well/injection.hpp"
#include "Well/WellProductionProperties.hpp"
#include "Well/WellInjectionProperties.hpp"

namespace Opm {


    Schedule::Schedule( const Deck& deck,
                        const EclipseGrid& grid,
                        const Eclipse3DProperties& eclipseProperties,
                        const Runspec &runspec,
                        const ParseContext& parseContext,
                        ErrorGuard& errors) :
        m_timeMap( deck ),
        m_rootGroupTree( this->m_timeMap, GroupTree{} ),
        m_oilvaporizationproperties( this->m_timeMap, OilVaporizationProperties(runspec.tabdims().getNumPVTTables()) ),
        m_events( this->m_timeMap ),
        m_modifierDeck( this->m_timeMap, Deck{} ),
        m_tuning( this->m_timeMap ),
        m_messageLimits( this->m_timeMap ),
        m_runspec( runspec ),
        wtest_config(this->m_timeMap, std::make_shared<WellTestConfig>() ),
        wlist_manager( this->m_timeMap, std::make_shared<WListManager>()),
        udq_config(this->m_timeMap, std::make_shared<UDQInput>(deck)),
        global_whistctl_mode(this->m_timeMap, WellProducer::CMODE_UNDEFINED),
        rft_config(this->m_timeMap)
    {
        addGroup( "FIELD", 0 );

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

        if (Section::hasSCHEDULE(deck))
            iterateScheduleSection( parseContext, errors, SCHEDULESection( deck ), grid, eclipseProperties );
#ifdef WELL_TEST
        checkWells(parseContext, errors);
#endif
    }


    template <typename T>
    Schedule::Schedule( const Deck& deck,
                        const EclipseGrid& grid,
                        const Eclipse3DProperties& eclipseProperties,
                        const Runspec &runspec,
                        const ParseContext& parseContext,
                        T&& errors) :
        Schedule(deck, grid, eclipseProperties, runspec, parseContext, errors)
    {}


    Schedule::Schedule( const Deck& deck,
                        const EclipseGrid& grid,
                        const Eclipse3DProperties& eclipseProperties,
                        const Runspec &runspec) :
        Schedule(deck, grid, eclipseProperties, runspec, ParseContext(), ErrorGuard())
    {}


    Schedule::Schedule(const Deck& deck, const EclipseState& es, const ParseContext& parse_context, ErrorGuard& errors) :
        Schedule(deck,
                 es.getInputGrid(),
                 es.get3DProperties(),
                 es.runspec(),
                 parse_context,
                 errors)
    {}



    template <typename T>
    Schedule::Schedule(const Deck& deck, const EclipseState& es, const ParseContext& parse_context, T&& errors) :
        Schedule(deck,
                 es.getInputGrid(),
                 es.get3DProperties(),
                 es.runspec(),
                 parse_context,
                 errors)
    {}


    Schedule::Schedule(const Deck& deck, const EclipseState& es) :
        Schedule(deck, es, ParseContext(), ErrorGuard())
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
            currentStep += keyword.getRecord(0).getItem(0).size(); // This is a bit weird API.
        }

        else if (keyword.name() == "UDQ")
            handleUDQ(keyword, currentStep);

        else if (keyword.name() == "WLIST")
            handleWLIST( keyword, currentStep );

        else if (keyword.name() == "WELSPECS")
            handleWELSPECS( section, keywordIdx, currentStep, unit_system);

        else if (keyword.name() == "WHISTCTL")
            handleWHISTCTL(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WCONHIST")
            handleWCONHIST(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WCONPROD")
            handleWCONPROD(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WCONINJE")
            handleWCONINJE(section, keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WPOLYMER")
            handleWPOLYMER(keyword, currentStep, parseContext, errors);

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
            handleWCONINJH(section, keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WGRUPCON")
            handleWGRUPCON(keyword, currentStep);

        else if (keyword.name() == "COMPDAT")
            handleCOMPDAT(keyword, currentStep, grid, eclipseProperties, parseContext, errors);

        else if (keyword.name() == "WELSEGS")
            handleWELSEGS(keyword, currentStep);

        else if (keyword.name() == "COMPSEGS")
            handleCOMPSEGS(keyword, currentStep, grid, parseContext, errors);

        else if (keyword.name() == "WELOPEN")
            handleWELOPEN(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "WELTARG")
            handleWELTARG(section, keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "GRUPTREE")
            handleGRUPTREE(keyword, currentStep);

        else if (keyword.name() == "GRUPNET")
            handleGRUPNET(keyword, currentStep);

        else if (keyword.name() == "GCONINJE")
            handleGCONINJE(section, keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "GCONPROD")
            handleGCONPROD(keyword, currentStep, parseContext, errors);

        else if (keyword.name() == "GEFAC")
            handleGEFAC(keyword, currentStep, parseContext, errors);

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
                                          const Eclipse3DProperties& eclipseProperties) {
        size_t currentStep = 0;
        const auto& unit_system = section.unitSystem();
        std::vector<std::pair< const DeckKeyword* , size_t> > rftProperties;
        size_t keywordIdx = 0;

        while (true) {
            const auto& keyword = section.getKeyword(keywordIdx);
            if (keyword.name() == "ACTIONX") {
                ActionX action(keyword, this->m_timeMap.getStartTime(currentStep + 1));
                while (true) {
                    keywordIdx++;
                    if (keywordIdx == section.size())
                        throw std::invalid_argument("Invalid ACTIONX section - missing ENDACTIO");

                    const auto& action_keyword = section.getKeyword(keywordIdx);
                    if (action_keyword.name() == "ENDACTIO")
                        break;

                    if (ActionX::valid_keyword(action_keyword.name()))
                        action.addKeyword(action_keyword);
                    else {
                        std::string msg = "The keyword " + action_keyword.name() + " is not supported in a ACTIONX block.";
                        parseContext.handleError( ParseContext::ACTIONX_ILLEGAL_KEYWORD, msg, errors);
                    }
                }
                this->m_actions.add(action);
            } else
                this->handleKeyword(currentStep, section, keywordIdx, keyword, parseContext, errors, grid, eclipseProperties, unit_system, rftProperties);

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


    void Schedule::checkUnhandledKeywords(const SCHEDULESection& /*section*/) const
    {
    }


    bool Schedule::handleGroupFromWELSPECS(const std::string& groupName, GroupTree& newTree) const {
        if( newTree.exists( groupName ) ) return false;

        newTree.update( groupName );
        return true;
    }


    void Schedule::handleWHISTCTL(const DeckKeyword& keyword, std::size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        const auto& record = keyword.getRecord(0);
        const std::string& cmodeString = record.getItem("CMODE").getTrimmedString(0);
        const WellProducer::ControlModeEnum controlMode = WellProducer::ControlModeFromString( cmodeString );

        if (controlMode != WellProducer::NONE) {
            if (!WellProductionProperties::effectiveHistoryProductionControl(controlMode) ) {
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
            auto well2 = std::make_shared<Well2>(*dynamic_state[currentStep]);
            auto prop = std::make_shared<WellProductionProperties>(well2->getProductionProperties());

            if (prop->whistctl_cmode != controlMode) {
                prop->whistctl_cmode = controlMode;
                well2->updateProduction(prop);
                this->updateWell(well2, currentStep);
            }
        }

        for (auto& well_pair : this->wells_static) {
            auto& dynamic_state = well_pair.second;
            auto well2 = std::make_shared<Well2>(*dynamic_state[currentStep]);
            auto prop = std::make_shared<WellProductionProperties>(well2->getProductionProperties());

            if (prop->whistctl_cmode != controlMode) {
                prop->whistctl_cmode = controlMode;
                well2->updateProduction(prop);
                this->updateWell(well2, currentStep);
            }
        }

        for (auto& well_pair : this->wells_static) {
            auto& dynamic_state = well_pair.second;
            auto well2 = std::make_shared<Well2>(*dynamic_state[currentStep]);
            auto prop = std::make_shared<WellProductionProperties>(well2->getProductionProperties());

            if (prop->whistctl_cmode != controlMode) {
                prop->whistctl_cmode = controlMode;
                well2->updateProduction(prop);
                this->updateWell(well2, currentStep);
            }
        }
    }


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
                                   const UnitSystem& unit_system) {
        bool needNewTree = false;
        auto newTree = m_rootGroupTree.get(currentStep);

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
            const std::string& wellName = record.getItem("WELL").getTrimmedString(0);
            const std::string& groupName = record.getItem("GROUP").getTrimmedString(0);

            if (!hasGroup(groupName))
                addGroup(groupName , currentStep);

            if (!hasWell(wellName)) {
                WellCompletion::CompletionOrderEnum wellConnectionOrder = WellCompletion::TRACK;

                if( const auto* compordp = COMPORD_in_timestep() ) {
                     const auto& compord = *compordp;

                    for (size_t compordRecordNr = 0; compordRecordNr < compord.size(); compordRecordNr++) {
                        const auto& compordRecord = compord.getRecord(compordRecordNr);

                        const std::string& wellNamePattern = compordRecord.getItem(0).getTrimmedString(0);
                        if (Well2::wellNameInWellNamePattern(wellName, wellNamePattern)) {
                            const std::string& compordString = compordRecord.getItem(1).getTrimmedString(0);
                            wellConnectionOrder = WellCompletion::CompletionOrderEnumFromString(compordString);
                        }
                    }
                }
                this->addWell(wellName, record, currentStep, wellConnectionOrder, unit_system);
                this->addWellToGroup(this->m_groups.at( groupName ), wellName, currentStep);
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
                    auto well2 = std::shared_ptr<Well2>(new Well2( this->getWell2(wellName, currentStep)));
                    update = well2->updateHead(headI, headJ);
                    update |= well2->updateRefDepth(refDepth);
                    update |= well2->updateDrainageRadius(drainageRadius);

                    if (well2->groupName() != groupName) {
                        auto& old_group = this->m_groups.at(well2->groupName());
                        auto& new_group = this->m_groups.at(groupName);

                        old_group.delWell(currentStep, well2->name());
                        new_group.addWell(currentStep, well2->name());
                        update = true;
                    }

                    if (update) {
                        this->updateWell(well2, currentStep);
                        this->addWellEvent(well2->name(), ScheduleEvents::WELL_WELSPECS_UPDATE, currentStep);
                    }
                }
            }

            if (handleGroupFromWELSPECS(groupName, newTree))
                needNewTree = true;
        }

        if (needNewTree) {
            m_rootGroupTree.update(currentStep, newTree);
            m_events.addEvent( ScheduleEvents::GROUP_CHANGE , currentStep);
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

            const WellCommon::StatusEnum status =
                WellCommon::StatusFromString(record.getItem("STATUS").getTrimmedString(0));

            auto well_names = this->wellNames(wellNamePattern, currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for( const auto& well_name : well_names) {
                updateWellStatus( well_name , currentStep , status );
                {
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well2 = std::make_shared<Well2>(*dynamic_state[currentStep]);
                    bool switching_from_injector = !well2->isProducer();
                    auto properties = std::make_shared<WellProductionProperties>(well2->getProductionProperties());
                    bool update_well = false;
                    properties->handleWCONHIST(record);

                    if (switching_from_injector) {
                        properties->resetDefaultBHPLimit();
                        well2->updateProducer(true);

                        auto inj_props = std::make_shared<WellInjectionProperties>(well2->getInjectionProperties());
                        inj_props->setBHPLimit(0);
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
                        this->addWellEvent( well2->name(), ScheduleEvents::PRODUCTION_UPDATE, currentStep);
                        this->updateWell(well2, currentStep);
                    }
                    if ( !well2->getAllowCrossFlow()) {
                        // The numerical content of the rate UDAValues is accessed unconditionally;
                        // since this is in history mode use of UDA values is not allowed anyway.
                        const auto& oil_rate = properties->OilRate;
                        const auto& water_rate = properties->WaterRate;
                        const auto& gas_rate = properties->GasRate;
                        if ((oil_rate.get<double>() + water_rate.get<double>() + gas_rate.get<double>()) == 0) {
                            std::string msg =
                                "Well " + well2->name() + " is a history matched well with zero rate where crossflow is banned. " +
                                "This well will be closed at " + std::to_string ( m_timeMap.getTimePassedUntil(currentStep) / (60*60*24) ) + " days";
                            OpmLog::note(msg);
                            updateWellStatus( well_name, currentStep, WellCommon::StatusEnum::SHUT );
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

            const WellCommon::StatusEnum status =
                WellCommon::StatusFromString(record.getItem("STATUS").getTrimmedString(0));

            auto well_names = this->wellNames(wellNamePattern, currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for( const auto& well_name : well_names) {

                updateWellStatus( well_name , currentStep , status );
                {
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well2 = std::make_shared<Well2>(*dynamic_state[currentStep]);
                    bool switching_from_injector = !well2->isProducer();
                    auto properties = std::make_shared<WellProductionProperties>(well2->getProductionProperties());
                    bool update_well = switching_from_injector;

                    properties->clearControls();
                    if (well2->isAvailableForGroupControl())
                        properties->addProductionControl(WellProducer::GRUP);

                    properties->handleWCONPROD(record);

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
                        this->addWellEvent( well2->name(), ScheduleEvents::PRODUCTION_UPDATE, currentStep);
                        this->updateWell(well2, currentStep);
                    }
                }
            }
        }
    }


    void Schedule::updateWell(std::shared_ptr<Well2> well, size_t reportStep) {
        auto& dynamic_state = this->wells_static.at(well->name());
        dynamic_state.update(reportStep, well);
    }


    /*
      Function is quite dangerous - because if this is called while holding a
      Well2 pointer that will go stale and needs to be refreshed.
    */
    bool Schedule::updateWellStatus( const std::string& well_name, size_t reportStep , WellCommon::StatusEnum status) {
        bool update = false;
        {
            auto& dynamic_state = this->wells_static.at(well_name);
            auto well2 = std::make_shared<Well2>(*dynamic_state[reportStep]);
            if (well2->updateStatus(status)) {
                m_events.addEvent( ScheduleEvents::WELL_STATUS_CHANGE, reportStep );
                this->addWellEvent( well2->name(), ScheduleEvents::WELL_STATUS_CHANGE, reportStep);
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
                    auto well_ptr = std::make_shared<Well2>( *dynamic_state[currentStep] );
                    if (well_ptr->handleWPIMULT(record))
                        this->updateWell(well_ptr, currentStep);
                }
            }
        }
    }




    void Schedule::handleWCONINJE( const SCHEDULESection& section, const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);

            auto well_names = wellNames(wellNamePattern, currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for( const auto& well_name : well_names ) {
                WellCommon::StatusEnum status = WellCommon::StatusFromString( record.getItem("STATUS").getTrimmedString(0));
                updateWellStatus( well_name , currentStep , status );
                {
                    bool update_well = false;
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well2 = std::make_shared<Well2>(*dynamic_state[currentStep]);
                    auto injection = std::make_shared<WellInjectionProperties>(well2->getInjectionProperties());
                    injection->handleWCONINJE(record, well2->isAvailableForGroupControl(), well_name, section.unitSystem());

                    if (well2->updateProducer(false))
                        update_well = true;

                    if (well2->updateInjection(injection))
                        update_well = true;

                    if (well2->updatePrediction(true))
                        update_well = true;

                    if (update_well) {
                        this->updateWell(well2, currentStep);
                        m_events.addEvent( ScheduleEvents::INJECTION_UPDATE , currentStep );
                        this->addWellEvent( well_name, ScheduleEvents::INJECTION_UPDATE, currentStep);
                    }

                    // if the well has zero surface rate limit or reservior rate limit, while does not allow crossflow,
                    // it should be turned off.
                    if ( ! well2->getAllowCrossFlow()
                         && ( (injection->hasInjectionControl(WellInjector::RATE) && injection->surfaceInjectionRate == 0)
                              || (injection->hasInjectionControl(WellInjector::RESV) && injection->reservoirInjectionRate == 0) ) ) {
                        std::string msg =
                            "Well " + well_name + " is an injector with zero rate where crossflow is banned. " +
                            "This well will be closed at " + std::to_string ( m_timeMap.getTimePassedUntil(currentStep) / (60*60*24) ) + " days";
                        OpmLog::note(msg);
                        updateWellStatus( well_name, currentStep, WellCommon::StatusEnum::SHUT );
                    }
                }
            }
        }
    }

    void Schedule::handleWCONINJH( const SCHEDULESection& section,  const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            WellCommon::StatusEnum status = WellCommon::StatusFromString( record.getItem("STATUS").getTrimmedString(0));
            const auto well_names = wellNames( wellNamePattern, currentStep );

            if (well_names.empty())
                invalidNamePattern( wellNamePattern, parseContext, errors, keyword);

            for (const auto& well_name : well_names) {
                updateWellStatus( well_name, currentStep, status );
                {
                    bool update_well = false;
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well2 = std::make_shared<Well2>(*dynamic_state[currentStep]);
                    auto injection = std::make_shared<WellInjectionProperties>(well2->getInjectionProperties());
                    injection->handleWCONINJH(record, well2->isProducer(), well_name, section.unitSystem());

                    if (well2->updateProducer(false))
                        update_well = true;

                    if (well2->updateInjection(injection))
                        update_well = true;

                    if (well2->updatePrediction(false))
                        update_well = true;

                    if (update_well) {
                        this->updateWell(well2, currentStep);
                        m_events.addEvent( ScheduleEvents::INJECTION_UPDATE , currentStep );
                        this->addWellEvent( well_name, ScheduleEvents::INJECTION_UPDATE, currentStep);
                    }

                    if ( ! well2->getAllowCrossFlow() && (injection->surfaceInjectionRate == 0)) {
                        std::string msg =
                            "Well " + well_name + " is an injector with zero rate where crossflow is banned. " +
                            "This well will be closed at " + std::to_string ( m_timeMap.getTimePassedUntil(currentStep) / (60*60*24) ) + " days";
                        OpmLog::note(msg);
                        updateWellStatus( well_name, currentStep, WellCommon::StatusEnum::SHUT );
                    }
                }
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
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well2 = std::make_shared<Well2>(*dynamic_state[currentStep]);
                    auto polymer_properties = std::make_shared<WellPolymerProperties>( well2->getPolymerProperties() );
                    polymer_properties->handleWPOLYMER(record);
                    if (well2->updatePolymerProperties(polymer_properties))
                        this->updateWell(well2, currentStep);
                }
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
                    auto well2 = std::make_shared<Well2>(*dynamic_state[currentStep]);
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
                    auto well2 = std::make_shared<Well2>(*dynamic_state[currentStep]);
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
                    auto well2 = std::make_shared<Well2>(*dynamic_state[currentStep]);
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
                    auto well2 = std::make_shared<Well2>(*dynamic_state[currentStep]);
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
        std::shared_ptr<UDQInput> new_udq = std::make_shared<UDQInput>(current);
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
            const std::string& reason = record.getItem("REASON").get<std::string>(0);
            int num_test = record.getItem("TEST_NUM").get<int>(0);
            double startup_time = record.getItem("START_TIME").getSIDouble(0);

            for(const auto& well_name : well_names) {
                if (reason.size() == 0)
                    new_config->drop_well(well_name);
                else
                    new_config->add_well(well_name, reason, test_interval, num_test, startup_time);
            }
        }
        this->wtest_config.update(currentStep, new_config);
    }

    void Schedule::handleWSOLVENT( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {

        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames( wellNamePattern , currentStep);
            double fraction = record.getItem("SOLVENT_FRACTION").get< UDAValue >(0).get<double>();

            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for(const auto& well_name : well_names) {
                {
                    const auto& well = this->getWell2(well_name, currentStep);
                    const auto& inj = well.getInjectionProperties();
                    if (!well.isProducer() && inj.injectorType == WellInjector::GAS) {
                        if (well.getSolventFraction() != fraction) {
                            auto new_well = std::make_shared<Well2>(well);
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
                double tracerConcentration = record.getItem("CONCENTRATION").get< UDAValue >(0).get<double>();
                const std::string& tracerName = record.getItem("TRACER").getTrimmedString(0);
                {
                    auto well = std::make_shared<Well2>( this->getWell2(well_name, currentStep));
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
                    const auto& well = this->getWell2(well_name, currentStep);
                    double current_temp = well.getInjectionProperties().temperature;
                    if (current_temp != temp && !well.isProducer()) {
                        auto& dynamic_state = this->wells_static.at(well_name);
                        auto well_ptr = std::make_shared<Well2>( *dynamic_state[currentStep] );
                        auto inj = std::make_shared<WellInjectionProperties>(well_ptr->getInjectionProperties());
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
                    const auto& well = this->getWell2(well_name, currentStep);
                    double current_temp = well.getInjectionProperties().temperature;
                    if (current_temp != temp && !well.isProducer()) {
                        auto& dynamic_state = this->wells_static.at(well_name);
                        auto well_ptr = std::make_shared<Well2>( *dynamic_state[currentStep] );
                        auto inj = std::make_shared<WellInjectionProperties>(well_ptr->getInjectionProperties());
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
                    auto well_ptr = std::make_shared<Well2>( *dynamic_state[timestep] );
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

        constexpr auto open = WellCommon::StatusEnum::OPEN;

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
                const auto well_status = WellCommon::StatusFromString( status_str );
                for (const auto& wname : well_names) {
                    {
                        const auto& well = this->getWell2(wname, currentStep);
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
                        }
                    }
                }

                continue;
            }

            for (const auto& wname : well_names) {
                const auto comp_status = WellCompletion::StateEnumFromString( status_str );
                {
                    auto& dynamic_state = this->wells_static.at(wname);
                    auto well_ptr = std::make_shared<Well2>( *dynamic_state[currentStep] );
                    if (well_ptr->handleWELOPEN(record, comp_status))
                        // The updateWell call breaks test at line 825 and 831 in ScheduleTests
                        this->updateWell(well_ptr, currentStep);
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
    */

    void Schedule::handleWELTARG( const SCHEDULESection& section ,
                                  const DeckKeyword& keyword,
                                  size_t currentStep,
                                  const ParseContext& parseContext, ErrorGuard& errors) {
        Opm::UnitSystem unitSystem = section.unitSystem();
        double siFactorL = unitSystem.parse("LiquidSurfaceVolume/Time").getSIScaling();
        double siFactorG = unitSystem.parse("GasSurfaceVolume/Time").getSIScaling();
        double siFactorP = unitSystem.parse("Pressure").getSIScaling();

        for( const auto& record : keyword ) {

            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto cmode = WellTarget::ControlModeFromString(record.getItem("CMODE").getTrimmedString(0));
            double newValue = record.getItem("NEW_VALUE").get< double >(0);

            const auto well_names = wellNames( wellNamePattern, currentStep );

            if( well_names.empty() )
                invalidNamePattern( wellNamePattern, parseContext, errors, keyword);

            for(const auto& well_name : well_names) {
                {
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well2 = std::make_shared<Well2>(*dynamic_state[currentStep]);
                    bool update = false;
                    if (well2->isProducer()) {
                        auto prop = std::make_shared<WellProductionProperties>(well2->getProductionProperties());
                        prop->handleWELTARG(cmode, newValue, siFactorG, siFactorL, siFactorP);
                        update = well2->updateProduction(prop);
                        if (cmode == WellTarget::GUID)
                            update |= well2->updateWellGuideRate(newValue);
                    } else {
                        auto inj = std::make_shared<WellInjectionProperties>(well2->getInjectionProperties());
                        inj->handleWELTARG(cmode, newValue, siFactorG, siFactorL, siFactorP);
                        update = well2->updateInjection(inj);
                        if (cmode == WellTarget::GUID)
                            update |= well2->updateWellGuideRate(newValue);
                    }
                    if (update)
                        this->updateWell(well2, currentStep);
                }
            }
        }
    }

    void Schedule::handleGCONINJE( const SCHEDULESection& section,  const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& groupNamePattern = record.getItem("GROUP").getTrimmedString(0);
            auto groups = getGroups ( groupNamePattern );

            if (groups.empty())
                invalidNamePattern(groupNamePattern, parseContext, errors, keyword);

            for (auto* group : groups){
                {
                    Phase phase = get_phase( record.getItem("PHASE").getTrimmedString(0) );
                    group->setInjectionPhase( currentStep , phase );
                }
                {
                    GroupInjection::ControlEnum controlMode = GroupInjection::ControlEnumFromString( record.getItem("CONTROL_MODE").getTrimmedString(0) );
                    group->setInjectionControlMode( currentStep , controlMode );
                }

                Phase wellPhase = get_phase( record.getItem("PHASE").getTrimmedString(0));

                double surfaceInjectionRate = record.getItem("SURFACE_TARGET").get< UDAValue >(0).get<double>();
                surfaceInjectionRate = injection::rateToSI(surfaceInjectionRate, wellPhase, section.unitSystem());
                double reservoirInjectionRate = record.getItem("RESV_TARGET").get<UDAValue>(0).get<double>();

                group->setSurfaceMaxRate( currentStep , surfaceInjectionRate);
                group->setReservoirMaxRate( currentStep , reservoirInjectionRate);
                group->setTargetReinjectFraction( currentStep , record.getItem("REINJ_TARGET").get<UDAValue>(0).get<double>());
                group->setTargetVoidReplacementFraction( currentStep , record.getItem("VOIDAGE_TARGET").get<UDAValue>(0).get<double>());

                group->setInjectionGroup(currentStep, true);
            }
        }
    }

    void Schedule::handleGCONPROD( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& groupNamePattern = record.getItem("GROUP").getTrimmedString(0);
            auto groups = getGroups ( groupNamePattern );

            if (groups.empty())
                invalidNamePattern(groupNamePattern, parseContext, errors, keyword);

            for (auto* group : groups){
                {
                    GroupProduction::ControlEnum controlMode = GroupProduction::ControlEnumFromString( record.getItem("CONTROL_MODE").getTrimmedString(0) );
                    group->setProductionControlMode( currentStep , controlMode );
                }
                group->setOilTargetRate( currentStep , record.getItem("OIL_TARGET").get<UDAValue>(0).get<double>());
                group->setGasTargetRate( currentStep , record.getItem("GAS_TARGET").get<UDAValue>(0).get<double>());
                group->setWaterTargetRate( currentStep , record.getItem("WATER_TARGET").get<UDAValue>(0).get<double>());
                group->setLiquidTargetRate( currentStep , record.getItem("LIQUID_TARGET").get<UDAValue>(0).get<double>());
                group->setReservoirVolumeTargetRate( currentStep , record.getItem("RESERVOIR_FLUID_TARGET").getSIDouble(0));
                {
                    GroupProductionExceedLimit::ActionEnum exceedAction = GroupProductionExceedLimit::ActionEnumFromString(record.getItem("EXCEED_PROC").getTrimmedString(0) );
                    group->setProductionExceedLimitAction( currentStep , exceedAction );
                }

                group->setProductionGroup(currentStep, true);
            }
        }
    }


    void Schedule::handleGEFAC( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& groupNamePattern = record.getItem("GROUP").getTrimmedString(0);
            auto groups = getGroups ( groupNamePattern );

            if (groups.empty())
                invalidNamePattern(groupNamePattern, parseContext, errors, keyword);

            for (auto* group : groups){
                group->setGroupEfficiencyFactor(currentStep, record.getItem("EFFICIENCY_FACTOR").get< double >(0));

                const std::string& transfer_str = record.getItem("TRANSFER_EXT_NET").getTrimmedString(0);
                bool transfer = (transfer_str == "YES") ? true : false;
                group->setTransferGroupEfficiencyFactor(currentStep, transfer);
            }
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

    void Schedule::handleCOMPDAT( const DeckKeyword& keyword, size_t currentStep, const EclipseGrid& grid, const Eclipse3DProperties& eclipseProperties, const ParseContext& parseContext, ErrorGuard& errors) {
        for (const auto& record : keyword) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            auto wellnames = this->wellNames(wellNamePattern, currentStep);
            if (wellnames.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for (const auto& name : wellnames) {
                {
                    auto well2 = std::shared_ptr<Well2>(new Well2( this->getWell2(name, currentStep)));
                    auto connections = std::shared_ptr<WellConnections>( new WellConnections( well2->getConnections()));
                    connections->loadCOMPDAT(record, grid, eclipseProperties);

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
                                    well2 = std::shared_ptr<Well2>(new Well2(this->getWell2(name, currentStep)));
                            }
                        }
                    }
                    */
                    if (well2->updateConnections(connections))
                        this->updateWell(well2, currentStep);

                    if (well2->getStatus() == WellCommon::StatusEnum::SHUT) {
                        std::string msg =
                            "All completions in well " + well2->name() + " is shut at " + std::to_string ( m_timeMap.getTimePassedUntil(currentStep) / (60*60*24) ) + " days. \n" +
                            "The well is therefore also shut.";
                        OpmLog::note(msg);
                    }

                }
                this->addWellEvent(name, ScheduleEvents::COMPLETION_CHANGE, currentStep);
            }
        }
        m_events.addEvent(ScheduleEvents::COMPLETION_CHANGE, currentStep);
    }




    void Schedule::handleWELSEGS( const DeckKeyword& keyword, size_t currentStep) {
        const auto& record1 = keyword.getRecord(0);
        const auto& wname = record1.getItem("WELL").getTrimmedString(0);
        {
            auto& dynamic_state = this->wells_static.at(wname);
            auto well_ptr = std::make_shared<Well2>( *dynamic_state[currentStep] );
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
            auto well_ptr = std::make_shared<Well2>( *dynamic_state[currentStep] );
            if (well_ptr->handleCOMPSEGS(keyword, grid, parseContext, errors))
                this->updateWell(well_ptr, currentStep);
        }
    }

    void Schedule::handleWGRUPCON( const DeckKeyword& keyword, size_t currentStep) {
        for( const auto& record : keyword ) {
            const auto well_names = this->wellNames(record.getItem("WELL").getTrimmedString(0), currentStep);
            for (const auto& well_name : well_names) {
              bool availableForGroupControl = DeckItem::to_bool(record.getItem("GROUP_CONTROLLED").getTrimmedString(0));
                double guide_rate = record.getItem("GUIDE_RATE").get< double >(0);
                double scaling_factor = record.getItem("SCALING_FACTOR").get< double >(0);
                GuideRate::GuideRatePhaseEnum phase = GuideRate::UNDEFINED;
                if (!record.getItem("PHASE").defaultApplied(0)) {
                    std::string guideRatePhase = record.getItem("PHASE").getTrimmedString(0);
                    phase = GuideRate::GuideRatePhaseEnumFromString(guideRatePhase);
                }

                {
                    auto& dynamic_state = this->wells_static.at(well_name);
                    auto well_ptr = std::make_shared<Well2>( *dynamic_state[currentStep] );
                    if (well_ptr->updateWellGuideRate(availableForGroupControl, guide_rate, phase, scaling_factor))
                        this->updateWell(well_ptr, currentStep);
                }
            }
        }
    }

    void Schedule::handleGRUPTREE( const DeckKeyword& keyword, size_t currentStep) {
        const auto& currentTree = m_rootGroupTree.get(currentStep);
        auto newTree = currentTree;
        for( const auto& record : keyword ) {
            const std::string& childName = record.getItem("CHILD_GROUP").getTrimmedString(0);
            const std::string& parentName = record.getItem("PARENT_GROUP").getTrimmedString(0);
            newTree.update(childName, parentName);
            newTree.updateSeqIndex(childName, parentName);

            if (!hasGroup(parentName))
                addGroup( parentName , currentStep );

            if (!hasGroup(childName))
                addGroup( childName , currentStep );
        }
        m_rootGroupTree.update(currentStep, newTree);
    }

    void Schedule::handleGRUPNET( const DeckKeyword& keyword, size_t currentStep) {
        for( const auto& record : keyword ) {
            const auto& groupName = record.getItem("NAME").getTrimmedString(0);

            if (!hasGroup(groupName))
                addGroup(groupName , currentStep);

            auto& group = this->m_groups.at( groupName );
            int table = record.getItem("VFP_TABLE").get< int >(0);
            group.setGroupNetVFPTable(currentStep, table);
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
                this->rft_config.updateRFT(well_name, currentStep, RFTConnections::RFTEnum::YES);

        }

        this->rft_config.setWellOpenRFT(currentStep);
    }

    void Schedule::handleWRFTPLT( const DeckKeyword& keyword,  size_t currentStep) {
        for( const auto& record : keyword ) {

            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);

            RFTConnections::RFTEnum RFTKey = RFTConnections::RFTEnumFromString(record.getItem("OUTPUT_RFT").getTrimmedString(0));
            PLTConnections::PLTEnum PLTKey = PLTConnections::PLTEnumFromString(record.getItem("OUTPUT_PLT").getTrimmedString(0));
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

    const GroupTree& Schedule::getGroupTree(size_t timeStep) const {
        return m_rootGroupTree.get(timeStep);
    }

    void Schedule::addWell(const std::string& wellName,
                           const DeckRecord& record,
                           size_t timeStep,
                           WellCompletion::CompletionOrderEnum wellConnectionOrder,
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
            wells_static.insert( std::make_pair(wellName, DynamicState<std::shared_ptr<Well2>>(m_timeMap, nullptr)));

            auto& dynamic_state = wells_static.at(wellName);
            const std::string& group = record.getItem<ParserKeywords::WELSPECS::GROUP>().getTrimmedString(0);
            std::size_t insert_index = this->wells_static.size() - 1;
            auto well_ptr = std::make_shared<Well2>(wellName,
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
        well_events.insert( std::make_pair(wellName, Events(this->m_timeMap)));
        this->addWellEvent(wellName, ScheduleEvents::NEW_WELL, timeStep);
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

        const auto& well = this->getWell2atEnd(wellName);
        return well.hasBeenDefined(timeStep);
    }

    std::vector< Well2 > Schedule::getChildWells2(const std::string& group_name, size_t timeStep, GroupWellQueryMode query_mode) const {
        if (!hasGroup(group_name))
            throw std::invalid_argument("No such group: " + group_name);
        {
            const auto& group = getGroup( group_name );
            std::vector<Well2> wells;

            if (group.hasBeenDefined( timeStep )) {
                const GroupTree& group_tree = getGroupTree( timeStep );
                const auto& child_groups = group_tree.children( group_name );

                if (child_groups.size() && query_mode == GroupWellQueryMode::Recursive) {
                    for (const auto& child : child_groups) {
                        const auto& child_wells = getChildWells2( child, timeStep, query_mode );
                        wells.insert( wells.end() , child_wells.begin() , child_wells.end());
                    }
                } else {
                    for (const auto& well_name : group.getWells( timeStep ))
                        wells.push_back( this->getWell2( well_name, timeStep ));
                }
            }
            return wells;
        }
    }


    std::vector< const Group* > Schedule::getChildGroups(const std::string& group_name, size_t timeStep) const {
        if (!hasGroup(group_name))
            throw std::invalid_argument("No such group: " + group_name);
        {
            const auto& group = getGroup( group_name );
            std::vector<const Group*> child_groups;

            if (group.hasBeenDefined( timeStep )) {
                const GroupTree& group_tree = getGroupTree( timeStep );
                const auto& ch_grps = group_tree.children( group_name );
                //for (const std::string& group_name : ch_grps) {
                for ( auto it = ch_grps.begin() ; it != ch_grps.end(); it++) {
                    child_groups.push_back( &getGroup(*it));
                }
            }
            return child_groups;
        }
    }


    std::vector<Well2> Schedule::getWells2(size_t timeStep) const {
        std::vector<Well2> wells;
        if (timeStep >= this->m_timeMap.size())
            throw std::invalid_argument("timeStep argument beyond the length of the simulation");

        for (const auto& dynamic_pair : this->wells_static) {
            auto& well_ptr = dynamic_pair.second.get(timeStep);
            if (well_ptr)
                wells.push_back(*well_ptr.get());
        }
        return wells;
    }

    std::vector<Well2> Schedule::getWells2atEnd() const {
        return this->getWells2(this->m_timeMap.size() - 1);
    }


    const Well2& Schedule::getWell2atEnd(const std::string& well_name) const {
        return this->getWell2(well_name, this->m_timeMap.size() - 1);
    }

    const Well2& Schedule::getWell2(const std::string& wellName, size_t timeStep) const {
        if (this->wells_static.count(wellName) == 0)
            throw std::invalid_argument("No such well: " + wellName);

        const auto& dynamic_state = this->wells_static.at(wellName);
        auto& well_ptr = dynamic_state.get(timeStep);
        if (!well_ptr)
            throw std::invalid_argument("Well: " + wellName + " not yet defined at step: " + std::to_string(timeStep));

        return *well_ptr;
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
            int flags = 0;
            std::vector<std::string> names;
            for (const auto& well_pair : this->wells_static) {
                if (fnmatch(pattern.c_str(), well_pair.first.c_str(), flags) == 0) {
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


    void Schedule::addGroup(const std::string& groupName, size_t timeStep) {
        const size_t gseqIndex = m_groups.size();
        m_groups.insert( std::make_pair( groupName, Group { groupName, gseqIndex, m_timeMap, timeStep } ));
        m_events.addEvent( ScheduleEvents::NEW_GROUP , timeStep );
    }

    size_t Schedule::numGroups() const {
        return m_groups.size();
    }
    size_t Schedule::numGroups(size_t timeStep) const {
        return this->getGroups( timeStep ).size();
    }

    bool Schedule::hasGroup(const std::string& groupName) const {
        return m_groups.count(groupName) > 0;
    }


    const Group& Schedule::getGroup(const std::string& groupName) const {
        if (hasGroup(groupName)) {
            return m_groups.at(groupName);
        } else
            throw std::invalid_argument("Group: " + groupName + " does not exist");
    }

    std::vector< const Group* > Schedule::getGroups() const {
        std::vector< const Group* > groups;

        for( const auto& group : m_groups )
            groups.push_back( std::addressof(group.second) );

        return groups;
    }

    std::vector< Group* > Schedule::getGroups(const std::string& groupNamePattern) {
        size_t wildcard_pos = groupNamePattern.find("*");

        if( wildcard_pos != groupNamePattern.length()-1 ) {
            if( m_groups.count( groupNamePattern ) == 0) return {};
            return { std::addressof( m_groups.get( groupNamePattern ) ) };
        }

        std::vector< Group* > groups;
        for( auto& group_pair : this->m_groups ) {
            auto& group = group_pair.second;
            if( Group::groupNameInGroupNamePattern( group.name(), groupNamePattern ) ) {
                groups.push_back( std::addressof( group ) );
            }
        }

        return groups;
    }

    std::vector< const Group* > Schedule::getGroups(size_t timeStep) const {

        if (timeStep >= m_timeMap.size()) {
            throw std::invalid_argument("Timestep to large");
        }

        auto defined = [=]( const Group& g ) {
            return g.hasBeenDefined( timeStep );
        };

        std::vector< const Group* > groups;

        for( const auto& group_pair : m_groups ) {
            const auto& group = group_pair.second;
            if( !defined( group) ) continue;
            groups.push_back( &group );
        }
        return groups;
    }

    void Schedule::addWellToGroup( Group& newGroup, const std::string& wellName , size_t timeStep) {
        auto& dynamic_state = this->wells_static.at(wellName);
        auto well_ptr = std::make_shared<Well2>( *dynamic_state[timeStep] );
        if (well_ptr->groupName() != "")
            this->m_groups.at(well_ptr->groupName()).delWell(timeStep, wellName);

        well_ptr->updateGroup(newGroup.name());
        newGroup.addWell(timeStep, well_ptr->name());
        this->updateWell(well_ptr, timeStep);
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


    const Events& Schedule::getWellEvents(const std::string& well) const {
        if (this->well_events.count(well) > 0)
            return this->well_events.at(well);
        else
            throw std::invalid_argument("No such well " + well);
    }

    void Schedule::addWellEvent(const std::string& well, ScheduleEvents::Events event, size_t reportStep)  {
        auto& events = this->well_events.at(well);
        events.addEvent(event, reportStep);
    }

    bool Schedule::hasWellEvent(const std::string& well, uint64_t event_mask, size_t reportStep) const {
        const auto& events = this->getWellEvents(well);
        return events.hasEvent(event_mask, reportStep);
    }

    const Events& Schedule::getEvents() const {
        return this->m_events;
    }

    const OilVaporizationProperties& Schedule::getOilVaporizationProperties(size_t timestep) const {
        return m_oilvaporizationproperties.get(timestep);
    }

    bool Schedule::hasOilVaporizationProperties() const {
        for( size_t i = 0; i < this->m_timeMap.size(); ++i )
            if( m_oilvaporizationproperties.at( i ).defined() ) return true;

        return false;
    }

    void Schedule::checkIfAllConnectionsIsShut(size_t timeStep) {
        const auto& well_names = this->wellNames(timeStep);
        for (const auto& wname : well_names) {
            const auto& well = this->getWell2(wname, timeStep);
            const auto& connections = well.getConnections();
            if (connections.allConnectionsShut())
                this->updateWellStatus( well.name(), timeStep, WellCommon::StatusEnum::SHUT);
        }
    }


    void Schedule::filterConnections(const EclipseGrid& grid) {
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


    const WellTestConfig& Schedule::wtestConfig(size_t timeStep) const {
        const auto& ptr = this->wtest_config.get(timeStep);
        return *ptr;
    }

    const WListManager& Schedule::getWListManager(size_t timeStep) const {
        const auto& ptr = this->wlist_manager.get(timeStep);
        return *ptr;
    }

    const UDQInput& Schedule::getUDQConfig(size_t timeStep) const {
        const auto& ptr = this->udq_config.get(timeStep);
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


    const Actions& Schedule::actions() const {
        return this->m_actions;
    }


    void Schedule::applyAction(size_t reportStep, const ActionX& action, const std::vector<std::string>& matching_wells) {
        ParseContext parseContext;
        ErrorGuard errors;

        for (const auto& keyword : action) {
            if (!ActionX::valid_keyword(keyword.name()))
                throw std::invalid_argument("The keyword: " + keyword.name() + " can not be handled in the ACTION body");

            if (keyword.name() == "WELOPEN")
                this->handleWELOPEN(keyword, reportStep, parseContext, errors, matching_wells);
        }

    }

}

