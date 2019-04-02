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
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WList.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WListManager.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellInjectionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellPolymerProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellProductionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellConnections.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>
#include <opm/parser/eclipse/Units/Dimension.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>

#include "injection.hpp"

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
        udq_config(this->m_timeMap, std::make_shared<UDQInput>(deck))
    {
        m_controlModeWHISTCTL = WellProducer::CMODE_UNDEFINED;
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

#ifdef CHECK_WELLS
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
            handleWELSPECS( section, keywordIdx, currentStep );

        else if (keyword.name() == "WHISTCTL")
            handleWHISTCTL(parseContext, errors, keyword);

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
            handleCOMPSEGS(keyword, currentStep, grid);

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
            if (keyword.name() == "WRFT") {
                handleWRFT(keyword,  timeStep);
            }

            if (keyword.name() == "WRFTPLT"){
                handleWRFTPLT(keyword, timeStep);
            }
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

    void Schedule::handleWHISTCTL(const ParseContext& parseContext, ErrorGuard& errors, const DeckKeyword& keyword) {
        for( const auto& record : keyword ) {
            const std::string& cmodeString = record.getItem("CMODE").getTrimmedString(0);
            const WellProducer::ControlModeEnum controlMode = WellProducer::ControlModeFromString( cmodeString );

            if (controlMode != WellProducer::NONE) {
                if (!WellProductionProperties::effectiveHistoryProductionControl(controlMode) ) {
                    std::string msg = "The WHISTCTL keyword specifies an un-supported control mode " + cmodeString
                                    + ", which makes WHISTCTL keyword not affect the simulation at all";
                    OpmLog::warning(msg);
                }
            }

            m_controlModeWHISTCTL = controlMode;
            const std::string bhp_terminate = record.getItem("BPH_TERMINATE").getTrimmedString(0);
            if (bhp_terminate == "YES") {
                std::string msg = "The WHISTCTL keyword does not handle 'YES'. i.e. to terminate the run";
                OpmLog::error(msg);
                parseContext.handleError( ParseContext::UNSUPPORTED_TERMINATE_IF_BHP , msg, errors );
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
                                   size_t currentStep ) {
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
            bool new_well = false;

            if (!hasGroup(groupName))
                addGroup(groupName , currentStep);

            if (!hasWell(wellName)) {
                WellCompletion::CompletionOrderEnum wellConnectionOrder = WellCompletion::TRACK;

                if( const auto* compordp = COMPORD_in_timestep() ) {
                     const auto& compord = *compordp;

                    for (size_t compordRecordNr = 0; compordRecordNr < compord.size(); compordRecordNr++) {
                        const auto& compordRecord = compord.getRecord(compordRecordNr);

                        const std::string& wellNamePattern = compordRecord.getItem(0).getTrimmedString(0);
                        if (Well::wellNameInWellNamePattern(wellName, wellNamePattern)) {
                            const std::string& compordString = compordRecord.getItem(1).getTrimmedString(0);
                            wellConnectionOrder = WellCompletion::CompletionOrderEnumFromString(compordString);
                        }
                    }
                }
                addWell(wellName, record, currentStep, wellConnectionOrder);
                new_well = true;
            }

            auto& currentWell = this->m_wells.get( wellName );

            const auto headI = record.getItem( "HEAD_I" ).get< int >( 0 ) - 1;
            const auto headJ = record.getItem( "HEAD_J" ).get< int >( 0 ) - 1;
            if (!new_well)
                this->addWellEvent(currentWell.name(), ScheduleEvents::WELL_WELSPECS_UPDATE, currentStep);

            if( currentWell.getHeadI() != headI ) {
                std::string msg = "HEAD_I changed for well " + currentWell.name();
                OpmLog::info(Log::fileMessage(keyword.getFileName(), keyword.getLineNumber(), msg));
                currentWell.setHeadI( currentStep, headI );
            }

            if( currentWell.getHeadJ() != headJ ) {
                std::string msg = "HEAD_J changed for well " + currentWell.name();
                OpmLog::info(Log::fileMessage(keyword.getFileName(), keyword.getLineNumber(), msg));
                currentWell.setHeadJ( currentStep, headJ );
            }

            const auto& refDepthItem = record.getItem( "REF_DEPTH" );
            double refDepth = refDepthItem.hasValue( 0 )
                            ? refDepthItem.getSIDouble( 0 )
                            : -1.0;
            currentWell.setRefDepth( currentStep, refDepth );

            double drainageRadius = record.getItem( "D_RADIUS" ).getSIDouble(0);
            currentWell.setDrainageRadius( currentStep, drainageRadius );

            addWellToGroup( this->m_groups.at( groupName ), this->m_wells.get( wellName ), currentStep);
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

    void Schedule::handleWCONProducer( const DeckKeyword& keyword, size_t currentStep, bool isPredictionMode, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& wellNamePattern =
                record.getItem("WELL").getTrimmedString(0);

            const WellCommon::StatusEnum status =
                WellCommon::StatusFromString(record.getItem("STATUS").getTrimmedString(0));

            auto well_names = this->wellNames(wellNamePattern, currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for( const auto& well_name : well_names) {
                auto& well = this->m_wells.at(well_name);
                bool addGrupProductionControl = well.isAvailableForGroupControl(currentStep);
                bool switching_from_injector = !well.isProducer(currentStep);
                const WellProductionProperties& prev_properties = well.getProductionProperties(currentStep);
                WellProductionProperties properties(record, isPredictionMode, prev_properties, m_controlModeWHISTCTL, switching_from_injector, addGrupProductionControl);

                updateWellStatus( well , currentStep , status );
                if (well.setProductionProperties(currentStep, properties)) {
                    m_events.addEvent( ScheduleEvents::PRODUCTION_UPDATE , currentStep);
                    this->addWellEvent( well.name(), ScheduleEvents::PRODUCTION_UPDATE, currentStep);
                }
                if ( !well.getAllowCrossFlow() && !isPredictionMode && (properties.OilRate + properties.WaterRate + properties.GasRate) == 0 ) {

                    std::string msg =
                            "Well " + well.name() + " is a history matched well with zero rate where crossflow is banned. " +
                            "This well will be closed at " + std::to_string ( m_timeMap.getTimePassedUntil(currentStep) / (60*60*24) ) + " days";
                    OpmLog::note(msg);
                    updateWellStatus( well, currentStep, WellCommon::StatusEnum::SHUT );
                }
            }
        }
    }

    void Schedule::updateWellStatus( Well& well, size_t reportStep , WellCommon::StatusEnum status) {
        if( well.setStatus( reportStep, status ) ) {
            m_events.addEvent( ScheduleEvents::WELL_STATUS_CHANGE, reportStep );
            this->addWellEvent( well.name(), ScheduleEvents::WELL_STATUS_CHANGE, reportStep);
        }
    }


    void Schedule::handleWCONHIST( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        handleWCONProducer(keyword, currentStep, false, parseContext, errors);
    }

    void Schedule::handleWCONPROD( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        handleWCONProducer( keyword, currentStep, true, parseContext, errors);
    }

    void Schedule::handleWPIMULT( const DeckKeyword& keyword, size_t currentStep) {
        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = this->wellNames(wellNamePattern, currentStep);

            for( const auto& well_name : well_names) {
                auto& well = this->m_wells.at(well_name);
                well.handleWPIMULT(record, currentStep);
            }
        }
    }




    void Schedule::handleWCONINJE( const SCHEDULESection& section, const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);

            const auto well_names = this->wellNames(wellNamePattern, currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for(const auto& well_name : well_names) {
                auto& well = this->m_wells.at(well_name);
                WellInjectionProperties properties(well.getInjectionPropertiesCopy(currentStep));
                properties.handleWCONINJE(record, well.isAvailableForGroupControl(currentStep), well.name(), section.unitSystem());

                updateWellStatus( well, currentStep, WellCommon::StatusFromString( record.getItem("STATUS").getTrimmedString(0)));
                if (well.setInjectionProperties(currentStep, properties)) {
                    m_events.addEvent( ScheduleEvents::INJECTION_UPDATE , currentStep );
                    this->addWellEvent( well.name(), ScheduleEvents::INJECTION_UPDATE, currentStep);
                }

                // if the well has zero surface rate limit or reservior rate limit, while does not allow crossflow,
                // it should be turned off.
                if ( ! well.getAllowCrossFlow()
                     && ( (properties.hasInjectionControl(WellInjector::RATE) && properties.surfaceInjectionRate == 0)
                       || (properties.hasInjectionControl(WellInjector::RESV) && properties.reservoirInjectionRate == 0) ) ) {
                    std::string msg =
                            "Well " + well.name() + " is an injector with zero rate where crossflow is banned. " +
                            "This well will be closed at " + std::to_string ( m_timeMap.getTimePassedUntil(currentStep) / (60*60*24) ) + " days";
                    OpmLog::note(msg);
                    updateWellStatus( well, currentStep, WellCommon::StatusEnum::SHUT );
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
                auto& well = this->m_wells.at(well_name);
                WellPolymerProperties properties(well.getPolymerPropertiesCopy(currentStep));

                properties.m_polymerConcentration = record.getItem("POLYMER_CONCENTRATION").getSIDouble(0);
                properties.m_saltConcentration = record.getItem("SALT_CONCENTRATION").getSIDouble(0);

                const auto& group_polymer_item = record.getItem("GROUP_POLYMER_CONCENTRATION");
                const auto& group_salt_item = record.getItem("GROUP_SALT_CONCENTRATION");

                if (!group_polymer_item.defaultApplied(0)) {
                    throw std::logic_error("Sorry explicit setting of \'GROUP_POLYMER_CONCENTRATION\' is not supported!");
                }

                if (!group_salt_item.defaultApplied(0)) {
                    throw std::logic_error("Sorry explicit setting of \'GROUP_SALT_CONCENTRATION\' is not supported!");
                }
                well.setPolymerProperties(currentStep, properties);
            }
        }
    }


    void Schedule::handleWPMITAB( const DeckKeyword& keyword,  const size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {

        for (const auto& record : keyword) {

            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames(wellNamePattern, currentStep);

            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for (const auto& well_name : well_names) {
                auto& well = this->m_wells.at(well_name);
                if (well.isProducer(currentStep) ) {
                    throw std::logic_error("WPMITAB keyword can not be applied to production well " + well.name() );
                }
                WellPolymerProperties properties(well.getPolymerProperties(currentStep));
                properties.m_plymwinjtable = record.getItem("TABLE_NUMBER").get<int>(0);
                well.setPolymerProperties(currentStep, properties);
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
                auto& well = this->m_wells.at(well_name);
                if (well.isProducer(currentStep) )
                    throw std::logic_error("WSKPTAB can not be applied to production well " + well.name() );

                WellPolymerProperties properties(well.getPolymerProperties(currentStep));
                properties.m_skprwattable = record.getItem("TABLE_NUMBER_WATER").get<int>(0);
                properties.m_skprpolytable = record.getItem("TABLE_NUMBER_POLYMER").get<int>(0);
                well.setPolymerProperties(currentStep, properties);
            }
        }
    }


    void Schedule::handleWECON( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            WellEconProductionLimits econ_production_limits(record);
            const auto well_names = wellNames( wellNamePattern , currentStep);

            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for(const auto& well_name : well_names) {
                auto& well = this->m_wells.at(well_name);
                well.setEconProductionLimits(currentStep, econ_production_limits);
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
                auto& well = this->m_wells.at(well_name);
                well.setEfficiencyFactor(currentStep, efficiencyFactor);
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
                auto& well = this->m_wells.at(well_name);
                if (reason.size() == 0)
                    new_config->drop_well(well.name());
                else
                    new_config->add_well(well.name(), reason, test_interval, num_test, startup_time);
            }
        }
        this->wtest_config.update(currentStep, new_config);
    }

    void Schedule::handleWSOLVENT( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {

        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames( wellNamePattern , currentStep);

            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for(const auto& well_name : well_names) {
                auto& well = this->m_wells.at(well_name);
                WellInjectionProperties injectionProperties = well.getInjectionProperties( currentStep );
                if (well.isInjector( currentStep ) && injectionProperties.injectorType == WellInjector::GAS) {
                    double fraction = record.getItem("SOLVENT_FRACTION").get< double >(0);
                    well.setSolventFraction(currentStep, fraction);
                } else {
                    throw std::invalid_argument("WSOLVENT keyword can only be applied to Gas injectors");
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
                auto& well = this->m_wells.at(well_name);
                WellTracerProperties wellTracerProperties = well.getTracerProperties( currentStep );
                double tracerConcentration = record.getItem("CONCENTRATION").get< double >(0);
                const std::string& tracerName = record.getItem("TRACER").getTrimmedString(0);
                wellTracerProperties.setConcentration(tracerName, tracerConcentration);
                well.setTracerProperties(currentStep, wellTracerProperties);

            }
        }
    }

    void Schedule::handleWTEMP( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors) {
        for( const auto& record : keyword ) {
            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);
            const auto well_names = wellNames( wellNamePattern, currentStep );

            if (well_names.empty())
                invalidNamePattern( wellNamePattern, parseContext, errors, keyword);

            for (const auto& well_name : well_names) {
                // TODO: Is this the right approach? Setting the well temperature only
                // has an effect on injectors, but specifying it for producers won't hurt
                // and wells can also switch their injector/producer status. Note that
                // modifying the injector properties for producer wells currently leads
                // to a very weird segmentation fault downstream. For now, let's take the
                // water route.
                auto& well = this->m_wells.at(well_name);
                if (well.isInjector(currentStep)) {
                    WellInjectionProperties injectionProperties = well.getInjectionProperties(currentStep);
                    injectionProperties.temperature = record.getItem("TEMP").getSIDouble(0);
                    well.setInjectionProperties(currentStep, injectionProperties);
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

            if (well_names.empty())
                invalidNamePattern( wellNamePattern, parseContext, errors, keyword);

            for (const auto& well_name : well_names) {
                // TODO: Is this the right approach? Setting the well temperature only
                // has an effect on injectors, but specifying it for producers won't hurt
                // and wells can also switch their injector/producer status. Note that
                // modifying the injector properties for producer wells currently leads
                // to a very weird segmentation fault downstream. For now, let's take the
                // water route.
                auto& well = this->m_wells.at(well_name);
                if (well.isInjector(currentStep)) {
                    WellInjectionProperties injectionProperties = well.getInjectionProperties(currentStep);
                    injectionProperties.temperature = record.getItem("TEMPERATURE").getSIDouble(0);
                    well.setInjectionProperties(currentStep, injectionProperties);
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
                auto& well = this->m_wells.at(well_name);
                updateWellStatus( well, currentStep, status );

                WellInjectionProperties properties(well.getInjectionPropertiesCopy(currentStep));
                properties.handleWCONINJH(record, well.isProducer(currentStep), well.name(), section.unitSystem());
                if (well.setInjectionProperties(currentStep, properties)) {
                    m_events.addEvent( ScheduleEvents::INJECTION_UPDATE , currentStep );
                    this->addWellEvent( well.name(), ScheduleEvents::INJECTION_UPDATE, currentStep);
                }

                if ( ! well.getAllowCrossFlow() && (properties.surfaceInjectionRate == 0)) {
                    std::string msg =
                            "Well " + well.name() + " is an injector with zero rate where crossflow is banned. " +
                            "This well will be closed at " + std::to_string ( m_timeMap.getTimePassedUntil(currentStep) / (60*60*24) ) + " days";
                    OpmLog::note(msg);
                    updateWellStatus( well, currentStep, WellCommon::StatusEnum::SHUT );
                }
            }
        }
    }

    void Schedule::handleCOMPLUMP( const DeckKeyword& keyword,
                                   size_t timestep ) {

        for( const auto& record : keyword ) {
            const std::string& well_name = record.getItem("WELL").getTrimmedString(0);
            auto& well = this->m_wells.get(well_name);
            well.handleCOMPLUMP(record, timestep);
        }
    }

    void Schedule::handleWELOPEN( const DeckKeyword& keyword, size_t currentStep, const ParseContext& parseContext, ErrorGuard& errors, const std::vector<std::string>& matching_wells) {

        auto all_defaulted = []( const DeckRecord& rec ) {
            auto defaulted = []( const DeckItem& item ) {
                return item.defaultApplied( 0 );
            };

            return std::all_of( rec.begin() + 2, rec.end(), defaulted );
        };

        constexpr auto open = WellCommon::StatusEnum::OPEN;

        for( const auto& record : keyword ) {
            const auto& wellNamePattern = record.getItem( "WELL" ).getTrimmedString(0);
            const auto& status_str = record.getItem( "STATUS" ).getTrimmedString( 0 );

            const auto well_names = wellNames( wellNamePattern, currentStep, matching_wells );

            if (well_names.empty())
                invalidNamePattern( wellNamePattern, parseContext, errors, keyword);

            /* if all records are defaulted or just the status is set, only
             * well status is updated
             */
            if( all_defaulted( record ) ) {
                const auto well_status = WellCommon::StatusFromString( status_str );
                for(const auto& well_name : well_names) {
                    auto& well = this->m_wells.at(well_name);
                    if( well_status == open && !well.canOpen(currentStep) ) {
                        auto days = m_timeMap.getTimePassedUntil( currentStep ) / (60 * 60 * 24);
                        std::string msg = "Well " + well.name()
                            + " where crossflow is banned has zero total rate."
                            + " This well is prevented from opening at "
                            + std::to_string( days ) + " days";
                        OpmLog::note(msg);
                    } else {
                        this->updateWellStatus( well, currentStep, well_status );
                    }
                }

                continue;
            }

            for( const auto& well_name : well_names ) {
                auto& well = this->m_wells.at(well_name);
                const auto comp_status = WellCompletion::StateEnumFromString( status_str );
                well.handleWELOPEN(record, currentStep, comp_status);
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
            const std::string& cMode = record.getItem("CMODE").getTrimmedString(0);
            double newValue = record.getItem("NEW_VALUE").get< double >(0);

            const auto well_names = wellNames( wellNamePattern, currentStep );

            if( well_names.empty() )
                invalidNamePattern( wellNamePattern, parseContext, errors, keyword);

            for(const auto& well_name : well_names) {
                auto& well = this->m_wells.at(well_name);
                if(well.isProducer(currentStep)){
                    WellProductionProperties prop = well.getProductionPropertiesCopy(currentStep);

                    if (cMode == "ORAT"){
                        prop.OilRate = newValue * siFactorL;
                    }
                    else if (cMode == "WRAT"){
                        prop.WaterRate = newValue * siFactorL;
                    }
                    else if (cMode == "GRAT"){
                        prop.GasRate = newValue * siFactorG;
                    }
                    else if (cMode == "LRAT"){
                        prop.LiquidRate = newValue * siFactorL;
                    }
                    else if (cMode == "RESV"){
                        prop.ResVRate = newValue * siFactorL;
                    }
                    else if (cMode == "BHP"){
                        prop.BHPLimit = newValue * siFactorP;
                    }
                    else if (cMode == "THP"){
                        prop.THPLimit = newValue * siFactorP;
                    }
                    else if (cMode == "VFP"){
                        prop.VFPTableNumber = static_cast<int> (newValue);
                    }
                    else if (cMode == "GUID"){
                        well.setGuideRate(currentStep, newValue);
                    }
                    else{
                        throw std::invalid_argument("Invalid keyword (MODE) supplied");
                    }

                    well.setProductionProperties(currentStep, prop);
                }else{
                    WellInjectionProperties prop = well.getInjectionPropertiesCopy(currentStep);
                    if (cMode == "BHP"){
                        prop.BHPLimit = newValue * siFactorP;
                    }
                    else if (cMode == "ORAT"){
                        if(prop.injectorType == WellInjector::TypeEnum::OIL){
                            prop.surfaceInjectionRate = newValue * siFactorL;
                        }else{
                             std::invalid_argument("Well type must be OIL to set the oil rate");
                        }
                    }
                    else if (cMode == "WRAT"){
                        if(prop.injectorType == WellInjector::TypeEnum::WATER){
                            prop.surfaceInjectionRate = newValue * siFactorL;
                        }else{
                             std::invalid_argument("Well type must be WATER to set the water rate");
                        }
                    }
                    else if (cMode == "GRAT"){
                        if(prop.injectorType == WellInjector::TypeEnum::GAS){
                            prop.surfaceInjectionRate = newValue * siFactorG;
                        }else{
                            std::invalid_argument("Well type must be GAS to set the gas rate");
                        }
                    }
                    else if (cMode == "THP"){
                        prop.THPLimit = newValue * siFactorP;
                    }
                    else if (cMode == "VFP"){
                        prop.VFPTableNumber = static_cast<int> (newValue);
                    }
                    else if (cMode == "GUID"){
                        well.setGuideRate(currentStep, newValue);
                    }
                    else if (cMode == "RESV"){
                        prop.reservoirInjectionRate = newValue * siFactorL;
                    }
                    else{
                        throw std::invalid_argument("Invalid keyword (MODE) supplied");
                    }

                    well.setInjectionProperties(currentStep, prop);
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

                // calculate SI injection rates for the group
                double surfaceInjectionRate = record.getItem("SURFACE_TARGET").get< double >(0);
                surfaceInjectionRate = injection::rateToSI(surfaceInjectionRate, wellPhase, section.unitSystem());
                double reservoirInjectionRate = record.getItem("RESV_TARGET").getSIDouble(0);

                group->setSurfaceMaxRate( currentStep , surfaceInjectionRate);
                group->setReservoirMaxRate( currentStep , reservoirInjectionRate);
                group->setTargetReinjectFraction( currentStep , record.getItem("REINJ_TARGET").getSIDouble(0));
                group->setTargetVoidReplacementFraction( currentStep , record.getItem("VOIDAGE_TARGET").getSIDouble(0));

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
                group->setOilTargetRate( currentStep , record.getItem("OIL_TARGET").getSIDouble(0));
                group->setGasTargetRate( currentStep , record.getItem("GAS_TARGET").getSIDouble(0));
                group->setWaterTargetRate( currentStep , record.getItem("WATER_TARGET").getSIDouble(0));
                group->setLiquidTargetRate( currentStep , record.getItem("LIQUID_TARGET").getSIDouble(0));
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
            auto well_names = wellNames(wellNamePattern, currentStep);
            if (well_names.empty())
                invalidNamePattern(wellNamePattern, parseContext, errors, keyword);

            for (const auto& well_name : well_names) {
                auto& well = this->m_wells.at(well_name);
                well.handleCOMPDAT(currentStep, record, grid, eclipseProperties);

                if (well.getConnections( currentStep ).allConnectionsShut()) {
                    std::string msg =
                        "All completions in well " + well.name() + " is shut at " + std::to_string ( m_timeMap.getTimePassedUntil(currentStep) / (60*60*24) ) + " days. \n" +
                        "The well is therefore also shut.";
                    OpmLog::note(msg);
                    updateWellStatus( well, currentStep, WellCommon::StatusEnum::SHUT);
                }
                this->addWellEvent(well.name(), ScheduleEvents::COMPLETION_CHANGE, currentStep);
            }
        }
        m_events.addEvent(ScheduleEvents::COMPLETION_CHANGE, currentStep);
    }




    void Schedule::handleWELSEGS( const DeckKeyword& keyword, size_t currentStep) {
        const auto& record1 = keyword.getRecord(0);
        auto& well = this->m_wells.get(record1.getItem("WELL").getTrimmedString(0));
        well.handleWELSEGS(keyword, currentStep);WellSegments newSegmentset;
    }

    void Schedule::handleCOMPSEGS( const DeckKeyword& keyword, size_t currentStep, const EclipseGrid& grid) {
        const auto& record1 = keyword.getRecord(0);
        const std::string& well_name = record1.getItem("WELL").getTrimmedString(0);
        auto& well = this->m_wells.get( well_name );
        well.handleCOMPSEGS(keyword, grid, currentStep);
    }

    void Schedule::handleWGRUPCON( const DeckKeyword& keyword, size_t currentStep) {
        for( const auto& record : keyword ) {
            const std::string& wellName = record.getItem("WELL").getTrimmedString(0);
            auto& well = this->m_wells.get( wellName );

            bool availableForGroupControl = convertEclipseStringToBool(record.getItem("GROUP_CONTROLLED").getTrimmedString(0));
            well.setAvailableForGroupControl(currentStep, availableForGroupControl);

            well.setGuideRate(currentStep, record.getItem("GUIDE_RATE").get< double >(0));

            if (!record.getItem("PHASE").defaultApplied(0)) {
                std::string guideRatePhase = record.getItem("PHASE").getTrimmedString(0);
                well.setGuideRatePhase(currentStep, GuideRate::GuideRatePhaseEnumFromString(guideRatePhase));
            } else
                well.setGuideRatePhase(currentStep, GuideRate::UNDEFINED);

            well.setGuideRateScalingFactor(currentStep, record.getItem("SCALING_FACTOR").get< double >(0));
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
            for(const auto& well_name : well_names) {
                auto& well = this->m_wells.at(well_name);
                well.updateRFTActive( currentStep, RFTConnections::RFTEnum::YES);
            }
        }

        for( auto& well_pair : this->m_wells ) {
            auto& well = well_pair.second;
            well.setRFTForWellWhenFirstOpen( currentStep );
        }
    }

    void Schedule::handleWRFTPLT( const DeckKeyword& keyword,  size_t currentStep) {
        for( const auto& record : keyword ) {

            const std::string& wellNamePattern = record.getItem("WELL").getTrimmedString(0);

            RFTConnections::RFTEnum RFTKey = RFTConnections::RFTEnumFromString(record.getItem("OUTPUT_RFT").getTrimmedString(0));
            PLTConnections::PLTEnum PLTKey = PLTConnections::PLTEnumFromString(record.getItem("OUTPUT_PLT").getTrimmedString(0));
            const auto well_names = wellNames(wellNamePattern, currentStep);
            for(const auto& well_name : well_names) {
                auto& well = this->m_wells.at(well_name);
                well.updateRFTActive( currentStep, RFTKey );
                well.updatePLTActive( currentStep, PLTKey );
            }
        }
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

    void Schedule::addWell(const std::string& wellName, const DeckRecord& record, size_t timeStep, WellCompletion::CompletionOrderEnum wellConnectionOrder) {
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

        const size_t wseqIndex = m_wells.size();

        Well well(wellName, wseqIndex,
                  headI, headJ, refDepth, drainageRadius,
                  preferredPhase, m_timeMap,
                  timeStep,
                  wellConnectionOrder, allowCrossFlow, automaticShutIn);

        m_wells.insert( std::make_pair(wellName, well ));
        m_events.addEvent( ScheduleEvents::NEW_WELL , timeStep );
        well_events.insert( std::make_pair(wellName, Events(this->m_timeMap)));
        this->addWellEvent(wellName, ScheduleEvents::NEW_WELL, timeStep);
    }

    size_t Schedule::numWells() const {
        return m_wells.size();
    }

    size_t Schedule::numWells(size_t timestep) const {
        return this->getWells( timestep ).size();
    }

    bool Schedule::hasWell(const std::string& wellName) const {
        return m_wells.count( wellName ) > 0;
    }

    std::vector< const Well* > Schedule::getWells() const {
        return getWells(m_timeMap.size()-1);
    }

    /*
      This will recursively go all the way down through the group tree
      until the well leaf-nodes are encountered.
    */
    std::vector< const Well* > Schedule::getWells(const std::string& group_name, size_t timeStep) const {
        if (!hasGroup(group_name))
            throw std::invalid_argument("No such group: " + group_name);
        {
            const auto& group = getGroup( group_name );
            std::vector<const Well*> wells;

            if (group.hasBeenDefined( timeStep )) {
                const GroupTree& group_tree = getGroupTree( timeStep );
                const auto& child_groups = group_tree.children( group_name );

                if (child_groups.size()) {
                    for (const auto& child : child_groups) {
                        const auto& child_wells = getWells( child, timeStep );
                        wells.insert( wells.end() , child_wells.begin() , child_wells.end());
                    }
                } else {
                    for (const auto& well_name : group.getWells( timeStep )) {
                        wells.push_back( getWell( well_name ));
                    }
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

        std::vector< const Well* > Schedule::getChildWells(const std::string& group_name, size_t timeStep) const {
        if (!hasGroup(group_name))
            throw std::invalid_argument("No such group: " + group_name);
        {
            const auto& group = getGroup( group_name );
            std::vector<const Well*> wells;

            if (group.hasBeenDefined( timeStep )) {
                const GroupTree& group_tree = getGroupTree( timeStep );
                const auto& child_groups = group_tree.children( group_name );

                if (!child_groups.size()) {
                    //for (const auto& well_name : group.getWells( timeStep )) {
                    const auto& ch_wells = group.getWells( timeStep );
                    for (auto it= ch_wells.begin(); it != ch_wells.end(); it++) {
                        wells.push_back( getWell( *it ));
                    }
                }
            }
            return wells;
        }
    }




    std::vector< const Well* > Schedule::getWells(size_t timeStep) const {
        if (timeStep >= m_timeMap.size()) {
            throw std::invalid_argument("Timestep to large");
        }

        auto defined = [=]( const Well& w ) {
            return w.hasBeenDefined( timeStep );
        };

        std::vector< const Well* > wells;
        for( const auto& well_pair : m_wells ) {
            const auto& well = well_pair.second;
            if( !defined( well ) ) continue;
            wells.push_back( std::addressof( well ) );
        }

        return wells;
    }

    const Well* Schedule::getWell(const std::string& wellName) const {
        return std::addressof( m_wells.get( wellName ) );
    }


    /*
      Observe that this method only returns wells which have state ==
      OPEN; it does not include wells in state AUTO which might have
      been opened by the simulator.
    */

    std::vector< const Well* > Schedule::getOpenWells(size_t timeStep) const {

        auto open = [=]( const Well& w ) {
            return w.getStatus( timeStep ) == WellCommon::OPEN;
        };

        std::vector< const Well* > wells;
        for( const auto& well_pair : m_wells ) {
            const auto& well = well_pair.second;
            if( !open( well ) ) continue;
            wells.push_back( std::addressof( well ) );
        }

        return wells;
    }




    /*
      There are many SCHEDULE keyword which take a wellname as argument. In
      addition to giving a fully qualified name like 'W1' you can also specify
      shell wildcard patterns like like 'W*', you can get all the wells in the
      well-list '*WL'[1] and the wellname '?' is used to get all the wells which
      already have matched a condition in a ACTIONX keyword. This function
      should be one-stop function to get all well names according to a input
      pattern. The timestep argument is used to check that the wells have
      indeewd been defined at the point in time we are considering.

      [1]: The leading '*' in a WLIST name should not be interpreted as a shell
           wildcard!
    */


    std::vector<std::string> Schedule::wellNames(const std::string& pattern, size_t timeStep, const std::vector<std::string>& matching_wells) const {
        std::vector<std::string> names;
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
            for (const auto& well_pair : this->m_wells) {
                if (fnmatch(pattern.c_str(), well_pair.first.c_str(), flags) == 0) {
                    if (well_pair.second.firstTimeStep() <= timeStep)
                        names.push_back(well_pair.first);
                }
            }
            return names;
        }

        // ACTIONX handler
        if (pattern == "?") {
            return { matching_wells.begin(), matching_wells.end() };
        }

        // Normal well name without any special characters
        if (this->hasWell(pattern)) {
            auto well = this->getWell(pattern);
            if (well->firstTimeStep() <= timeStep)
                return { pattern };
        }
        return {};
    }

    std::vector<std::string> Schedule::wellNames(const std::string& pattern) const {
        return this->wellNames(pattern, this->size() - 1);
    }

    std::vector<std::string> Schedule::wellNames(std::size_t timeStep) const {
        std::vector<std::string> names;
        for (const auto& well_pair : this->m_wells) {
            if (well_pair.second.firstTimeStep() <= timeStep)
                names.push_back(well_pair.first);
        }
        return names;
    }

    std::vector<std::string> Schedule::wellNames() const {
        std::vector<std::string> names;
        for (const auto& well_pair : this->m_wells)
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

    void Schedule::addWellToGroup( Group& newGroup, Well& well , size_t timeStep) {
        const std::string currentGroupName = well.getGroupName(timeStep);
        if (currentGroupName != "") {
            m_groups.at( currentGroupName ).delWell( timeStep, well.name() );
        }
        well.setGroupName(timeStep , newGroup.name());
        newGroup.addWell(timeStep , &well);
    }


    bool Schedule::convertEclipseStringToBool(const std::string& eclipseString) {
        std::string lowerTrimmed = boost::algorithm::to_lower_copy(eclipseString);
        boost::algorithm::trim(lowerTrimmed);

        if (lowerTrimmed == "y" || lowerTrimmed == "yes") {
            return true;
        }
        else if (lowerTrimmed == "n" || lowerTrimmed == "no") {
            return false;
        }
        else throw std::invalid_argument("String " + eclipseString + " not recognized as a boolean-convertible string.");
    }

    size_t Schedule::getMaxNumConnectionsForWells(size_t timestep) const {
      size_t ncwmax = 0;
      for( const auto& well_pair : this->m_wells) {
          const auto& completions = well_pair.second.getConnections(timestep);

          if( completions.size() > ncwmax )
              ncwmax = completions.size();
      }
      return ncwmax;
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

    void Schedule::checkIfAllConnectionsIsShut(size_t timestep) {
        for( auto& well_pair : this->m_wells ) {
            auto& well = well_pair.second;
            const auto& completions = well.getConnections(timestep);
            if( completions.allConnectionsShut() )
                this->updateWellStatus( well, timestep, WellCommon::StatusEnum::SHUT);
        }
    }


    void Schedule::filterConnections(const EclipseGrid& grid) {
        for (auto& well_pair : this->m_wells) {
            auto well = well_pair.second;
            well.filterConnections(grid);
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


#ifdef CHECK_WELLS
    bool checkWells(const ParseContext& parseContext, ErrorGuard& errors) const {
        return true;
    }
#endif
}
