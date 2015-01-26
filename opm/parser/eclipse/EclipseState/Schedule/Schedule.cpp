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

#include <string>
#include <vector>

#include <boost/algorithm/string.hpp>

#include <opm/parser/eclipse/Deck/Section.hpp>

#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellProductionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellInjectionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellPolymerProperties.hpp>




namespace Opm {

    Schedule::Schedule(DeckConstPtr deck, LoggerPtr logger) {
        initFromDeck(deck, logger);
    }

    void Schedule::initFromDeck(DeckConstPtr deck, LoggerPtr logger) {
        createTimeMap(deck, logger);
        addGroup( "FIELD", 0 );
        initRootGroupTreeNode(getTimeMap());
        iterateScheduleSection(deck, logger);
    }

    void Schedule::initRootGroupTreeNode(TimeMapConstPtr timeMap) {
        m_rootGroupTree.reset(new DynamicState<GroupTreePtr>(timeMap, GroupTreePtr(new GroupTree())));
    }

    void Schedule::createTimeMap(DeckConstPtr deck, LoggerPtr /*logger*/) {
        boost::posix_time::ptime startTime(defaultStartDate);
        if (deck->hasKeyword("START")) {
            DeckKeywordConstPtr startKeyword = deck->getKeyword("START");
            startTime = TimeMap::timeFromEclipse(startKeyword->getRecord(0));
        }

        m_timeMap.reset(new TimeMap(startTime));
    }

    void Schedule::iterateScheduleSection(DeckConstPtr deck, LoggerPtr logger) {
        size_t currentStep = 0;

        for (size_t keywordIdx = 0; keywordIdx < deck->size(); ++keywordIdx) {
            DeckKeywordConstPtr keyword = deck->getKeyword(keywordIdx);

            if (keyword->name() == "DATES") {
                handleDATES(keyword, logger);
                currentStep += keyword->size();
            }

            if (keyword->name() == "TSTEP") {
                handleTSTEP(keyword, logger);
                currentStep += keyword->getRecord(0)->getItem(0)->size(); // This is a bit weird API.
            }

            if (keyword->name() == "WELSPECS") {
                handleWELSPECS(keyword, logger, currentStep);
            }

            if (keyword->name() == "WCONHIST")
                handleWCONHIST(keyword, logger, currentStep);

            if (keyword->name() == "WCONPROD")
                handleWCONPROD(keyword, logger, currentStep);

            if (keyword->name() == "WCONINJE")
                handleWCONINJE(deck, keyword, logger, currentStep);

            if (keyword->name() == "WPOLYMER")
                handleWPOLYMER(keyword, logger, currentStep);

            if (keyword->name() == "WCONINJH")
                handleWCONINJH(deck, keyword, logger, currentStep);

            if (keyword->name() == "WGRUPCON")
                handleWGRUPCON(keyword, logger, currentStep);

            if (keyword->name() == "COMPDAT")
                handleCOMPDAT(keyword, logger, currentStep);

            if (keyword->name() == "WELOPEN")
                handleWELOPEN(keyword, logger, currentStep, deck->hasKeyword("COMPLUMP"));

            if (keyword->name() == "GRUPTREE")
                handleGRUPTREE(keyword, logger, currentStep);

            if (keyword->name() == "GCONINJE")
                handleGCONINJE(deck, keyword, logger, currentStep);

            if (keyword->name() == "GCONPROD")
                handleGCONPROD(keyword, logger, currentStep);
        }
    }

    void Schedule::handleDATES(DeckKeywordConstPtr keyword, LoggerPtr /*logger*/) {
        m_timeMap->addFromDATESKeyword(keyword);
    }

    void Schedule::handleTSTEP(DeckKeywordConstPtr keyword, LoggerPtr /*logger*/) {
        m_timeMap->addFromTSTEPKeyword(keyword);
    }

    bool Schedule::handleGroupFromWELSPECS(const std::string& groupName, GroupTreePtr newTree) const {
        bool treeUpdated = false;
        if (!newTree->getNode(groupName)) {
            treeUpdated = true;
            newTree->updateTree(groupName);
        }
        return treeUpdated;
    }

    void Schedule::handleWELSPECS(DeckKeywordConstPtr keyword, LoggerPtr logger, size_t currentStep) {
        bool needNewTree = false;
        GroupTreePtr newTree = m_rootGroupTree->get(currentStep)->deepCopy();

        for (size_t recordNr = 0; recordNr < keyword->size(); recordNr++) {
            DeckRecordConstPtr record = keyword->getRecord(recordNr);
            const std::string& wellName = record->getItem("WELL")->getTrimmedString(0);
            const std::string& groupName = record->getItem("GROUP")->getTrimmedString(0);

            if (!hasGroup(groupName)) {
                addGroup(groupName , currentStep);
            }

            if (!hasWell(wellName)) {
                addWell(wellName, record, currentStep);
            }

            WellConstPtr currentWell = getWell(wellName);
            checkWELSPECSConsistency(currentWell, keyword, recordNr, logger);

            addWellToGroup( getGroup(groupName) , getWell(wellName) , currentStep);
            bool treeChanged = handleGroupFromWELSPECS(groupName, newTree);
            needNewTree = needNewTree || treeChanged;
        }
        if (needNewTree) {
            m_rootGroupTree->add(currentStep, newTree);
        }
    }

    void Schedule::checkWELSPECSConsistency(WellConstPtr well, DeckKeywordConstPtr keyword, size_t recordIdx, LoggerPtr logger) const {
        DeckRecordConstPtr record = keyword->getRecord(recordIdx);
        if (well->getHeadI() != record->getItem("HEAD_I")->getInt(0) - 1) {
            std::string msg =
                "Unable process WELSPECS for well " + well->name() + ", HEAD_I deviates from existing value";
            logger->addError(keyword->getFileName(),
                                keyword->getLineNumber(),
                                msg);
            throw std::invalid_argument(msg);
        }
        if (well->getHeadJ() != record->getItem("HEAD_J")->getInt(0) - 1) {
            std::string msg =
                "Unable process WELSPECS for well " + well->name() + ", HEAD_J deviates from existing value";
            logger->addError(keyword->getFileName(),
                                keyword->getLineNumber(),
                                msg);
            throw std::invalid_argument(msg);
        }
        if (well->getRefDepthDefaulted() != record->getItem("REF_DEPTH")->defaultApplied(0)) {
            std::string msg =
                "Unable process WELSPECS for well " + well->name() + ", REF_DEPTH defaulted state deviates from existing value";
            logger->addError(keyword->getFileName(),
                                keyword->getLineNumber(),
                                msg);
            throw std::invalid_argument(msg);
        }
        if (!well->getRefDepthDefaulted()) {
            if (well->getRefDepth() != record->getItem("REF_DEPTH")->getSIDouble(0)) {
                std::string msg =
                    "Unable process WELSPECS for well " + well->name() + ", REF_DEPTH deviates from existing value";
                logger->addError(keyword->getFileName(),
                                    keyword->getLineNumber(),
                                    msg);
                throw std::invalid_argument(msg);
            }
        }
    }

    void Schedule::handleWCONProducer(DeckKeywordConstPtr keyword, LoggerPtr logger, size_t currentStep, bool isPredictionMode) {
        for (size_t recordNr = 0; recordNr < keyword->size(); recordNr++) {
            DeckRecordConstPtr record = keyword->getRecord(recordNr);

            const std::string& wellNamePattern =
                record->getItem("WELL")->getTrimmedString(0);

            const WellCommon::StatusEnum status =
                WellCommon::StatusFromString(record->getItem("STATUS")->getTrimmedString(0));

            WellProductionProperties properties =
                ((isPredictionMode)
                 ? WellProductionProperties::prediction(record)
                 : WellProductionProperties::history   (record));

            const std::vector<WellPtr>& wells = getWells(wellNamePattern);

            for (auto wellIter=wells.begin(); wellIter != wells.end(); ++wellIter) {
                WellPtr well = *wellIter;

                if (well->isAvailableForGroupControl(currentStep)) {
                    properties.addProductionControl(WellProducer::GRUP);
                }
                else {
                    properties.dropProductionControl(WellProducer::GRUP);
                }

                if (status != WellCommon::SHUT) {
                    const std::string& cmodeString =
                        record->getItem("CMODE")->getTrimmedString(0);

                    WellProducer::ControlModeEnum control =
                        WellProducer::ControlModeFromString(cmodeString);

                    if (properties.hasProductionControl(control)) {
                        properties.controlMode = control;
                    }
                    else {
                        std::string msg =
                            "Tried to set invalid control: " +
                            cmodeString + " for well: " + well->name();
                        logger->addError(keyword->getFileName(),
                                            keyword->getLineNumber(),
                                            msg);
                        throw std::invalid_argument(msg);
                    }
                }

                well->setStatus( currentStep , status );
                well->setProductionProperties(currentStep, properties);
            }
        }
    }

    void Schedule::handleWCONHIST(DeckKeywordConstPtr keyword, LoggerPtr logger, size_t currentStep) {
        handleWCONProducer(keyword, logger, currentStep, false);
    }

    void Schedule::handleWCONPROD(DeckKeywordConstPtr keyword, LoggerPtr logger, size_t currentStep) {
        handleWCONProducer(keyword, logger, currentStep, true);
    }

    void Schedule::handleWCONINJE(DeckConstPtr deck, DeckKeywordConstPtr keyword, LoggerPtr /*logger*/, size_t currentStep) {
        for (size_t recordNr = 0; recordNr < keyword->size(); recordNr++) {
            DeckRecordConstPtr record = keyword->getRecord(recordNr);
            const std::string& wellNamePattern = record->getItem("WELL")->getTrimmedString(0);
            std::vector<WellPtr> wells = getWells(wellNamePattern);

            for (auto wellIter=wells.begin(); wellIter != wells.end(); ++wellIter) {
                WellPtr well = *wellIter;
                WellInjector::TypeEnum injectorType = WellInjector::TypeFromString( record->getItem("TYPE")->getTrimmedString(0) );
                WellCommon::StatusEnum status = WellCommon::StatusFromString( record->getItem("STATUS")->getTrimmedString(0));

                well->setStatus( currentStep , status );
                WellInjectionProperties properties(well->getInjectionPropertiesCopy(currentStep));

                properties.injectorType = injectorType;
                properties.predictionMode = true;

                if (!record->getItem("RATE")->defaultApplied(0)) {
                    properties.surfaceInjectionRate = convertInjectionRateToSI(record->getItem("RATE")->getRawDouble(0) , injectorType, *deck->getActiveUnitSystem());
                    properties.addInjectionControl(WellInjector::RATE);
                } else
                    properties.dropInjectionControl(WellInjector::RATE);


                if (!record->getItem("RESV")->defaultApplied(0)) {
                    properties.reservoirInjectionRate = convertInjectionRateToSI(record->getItem("RESV")->getRawDouble(0) , injectorType, *deck->getActiveUnitSystem());
                    properties.addInjectionControl(WellInjector::RESV);
                } else
                    properties.dropInjectionControl(WellInjector::RESV);


                if (!record->getItem("THP")->defaultApplied(0)) {
                    properties.THPLimit = record->getItem("THP")->getSIDouble(0);
                    properties.addInjectionControl(WellInjector::THP);
                } else
                    properties.dropInjectionControl(WellInjector::THP);

                /*
                  What a mess; there is a sensible default BHP limit
                  defined, so the BHPLimit can be safely set
                  unconditionally - but should we make BHP control
                  available based on that default value - currently we
                  do not do that.
                */
                properties.BHPLimit = record->getItem("BHP")->getSIDouble(0);
                if (!record->getItem("BHP")->defaultApplied(0)) {
                    properties.addInjectionControl(WellInjector::BHP);
                } else
                    properties.dropInjectionControl(WellInjector::BHP);

                if (well->isAvailableForGroupControl(currentStep))
                    properties.addInjectionControl(WellInjector::GRUP);
                else
                    properties.dropInjectionControl(WellInjector::GRUP);
                {
                    const std::string& cmodeString = record->getItem("CMODE")->getTrimmedString(0);
                    WellInjector::ControlModeEnum controlMode = WellInjector::ControlModeFromString( cmodeString );
                    if (properties.hasInjectionControl( controlMode))
                        properties.controlMode = controlMode;
                    else {
                        throw std::invalid_argument("Tried to set invalid control: " + cmodeString + " for well: " + wellNamePattern);
                    }
                }
                well->setInjectionProperties(currentStep, properties);
            }
        }
    }


    void Schedule::handleWPOLYMER(DeckKeywordConstPtr keyword, LoggerPtr /*logger*/, size_t currentStep) {
        for (size_t recordNr = 0; recordNr < keyword->size(); recordNr++) {
            DeckRecordConstPtr record = keyword->getRecord(recordNr);
            const std::string& wellNamePattern = record->getItem("WELL")->getTrimmedString(0);
            std::vector<WellPtr> wells = getWells(wellNamePattern);

            for (auto wellIter=wells.begin(); wellIter != wells.end(); ++wellIter) {
                WellPtr well = *wellIter;

                WellPolymerProperties properties(well->getPolymerPropertiesCopy(currentStep));

                properties.m_polymerConcentration = record->getItem("POLYMER_CONCENTRATION")->getSIDouble(0);
                properties.m_saltConcentration = record->getItem("SALT_CONCENTRATION")->getSIDouble(0);

                auto group_polymer_item = record->getItem("GROUP_POLYMER_CONCENTRATION");
                auto group_salt_item = record->getItem("GROUP_SALT_CONCENTRATION");

                if (!group_polymer_item->defaultApplied(0)) {
                    throw std::logic_error("Sorry explicit setting of \'GROUP_POLYMER_CONCENTRATION\' is not supported!");
                }

                if (!group_salt_item->defaultApplied(0)) {
                    throw std::logic_error("Sorry explicit setting of \'GROUP_SALT_CONCENTRATION\' is not supported!");
                }
                well->setPolymerProperties(currentStep, properties);
            }
        }
    }

    void Schedule::handleWCONINJH(DeckConstPtr deck, DeckKeywordConstPtr keyword, LoggerPtr /*logger*/, size_t currentStep) {
        for (size_t recordNr = 0; recordNr < keyword->size(); recordNr++) {
            DeckRecordConstPtr record = keyword->getRecord(recordNr);
            const std::string& wellName = record->getItem("WELL")->getTrimmedString(0);
            WellPtr well = getWell(wellName);

            // convert injection rates to SI
            WellInjector::TypeEnum injectorType = WellInjector::TypeFromString( record->getItem("TYPE")->getTrimmedString(0));
            double injectionRate = record->getItem("RATE")->getRawDouble(0);
            injectionRate = convertInjectionRateToSI(injectionRate, injectorType, *deck->getActiveUnitSystem());

            WellCommon::StatusEnum status = WellCommon::StatusFromString( record->getItem("STATUS")->getTrimmedString(0));

            well->setStatus( currentStep , status );
            WellInjectionProperties properties(well->getInjectionPropertiesCopy(currentStep));

            properties.injectorType = injectorType;

            const std::string& cmodeString = record->getItem("CMODE")->getTrimmedString(0);
            WellInjector::ControlModeEnum controlMode = WellInjector::ControlModeFromString( cmodeString );
            if (!record->getItem("RATE")->defaultApplied(0)) {
                properties.surfaceInjectionRate = injectionRate;
                properties.addInjectionControl(controlMode);
                properties.controlMode = controlMode;
            }
            properties.predictionMode = false;

            well->setInjectionProperties(currentStep, properties);
        }
    }

    static Opm::Value<int> getValueItem(DeckItemPtr item){
        Opm::Value<int> data(item->name());
        if(item->hasValue(0)) {
            int tempValue = item->getInt(0);
            if( tempValue >0){
                data.setValue(tempValue-1);
            }
        }
        return data;
    }

    void Schedule::handleWELOPEN(DeckKeywordConstPtr keyword, LoggerPtr /*logger*/, size_t currentStep, bool hascomplump) {
        for (size_t recordNr = 0; recordNr < keyword->size(); recordNr++) {
            DeckRecordConstPtr record = keyword->getRecord(recordNr);

            bool haveCompletionData = false;
            for (size_t i=2; i<7; i++) {
                if (record->getItem(i)->hasValue(0)) {
                    haveCompletionData = true;
                    break;
                }
            }


            const std::string& wellNamePattern = record->getItem("WELL")->getTrimmedString(0);
            const std::vector<WellPtr>& wells = getWells(wellNamePattern);


            for (auto wellIter=wells.begin(); wellIter != wells.end(); ++wellIter) {
                WellPtr well = *wellIter;

                if(haveCompletionData){
                    CompletionSetConstPtr currentCompletionSet = well->getCompletions(currentStep);

                    CompletionSetPtr newCompletionSet(new CompletionSet( ));

                    Opm::Value<int> I  = getValueItem(record->getItem("I"));
                    Opm::Value<int> J  = getValueItem(record->getItem("J"));
                    Opm::Value<int> K  = getValueItem(record->getItem("K"));
                    Opm::Value<int> C1 = getValueItem(record->getItem("C1"));
                    Opm::Value<int> C2 = getValueItem(record->getItem("C2"));

                    if(hascomplump && (C2.hasValue() || C1.hasValue())){
                        std::cerr << "ERROR the keyword COMPLUMP is not supported used when C1 or C2 in WELOPEN have values" << std::endl;
                        throw std::exception();
                    }

                    size_t completionSize = currentCompletionSet->size();

                    for(size_t i = 0; i < completionSize;i++) {

                        CompletionConstPtr currentCompletion = currentCompletionSet->get(i);

                        if (C1.hasValue()) {
                            if (i < (size_t) C1.getValue()) {
                                newCompletionSet->add(currentCompletion);
                                continue;
                            }
                        }
                        if (C2.hasValue()) {
                            if (i > (size_t) C2.getValue()) {
                                newCompletionSet->add(currentCompletion);
                                continue;
                            }
                        }

                        int ci = currentCompletion->getI();
                        int cj = currentCompletion->getJ();
                        int ck = currentCompletion->getK();

                        if (I.hasValue() && (!(I.getValue() == ci) )) {
                            newCompletionSet->add(currentCompletion);
                            continue;
                        }

                        if (J.hasValue() && (!(J.getValue() == cj) )) {
                            newCompletionSet->add(currentCompletion);
                            continue;
                        }

                        if (K.hasValue() && (!(K.getValue() == ck) )) {
                            newCompletionSet->add(currentCompletion);
                            continue;
                        }
                        WellCompletion::StateEnum completionStatus = WellCompletion::StateEnumFromString(record->getItem("STATUS")->getTrimmedString(0));
                        CompletionPtr newCompletion = std::make_shared<Completion>(currentCompletion, completionStatus);
                        newCompletionSet->add(newCompletion);
                    }
                    well->addCompletionSet(currentStep, newCompletionSet);

                    if (newCompletionSet->allCompletionsShut()) {
                      well->setStatus(currentStep, WellCommon::StatusEnum::SHUT);
                    }
                }
                else if(!haveCompletionData) {
                    WellCommon::StatusEnum status = WellCommon::StatusFromString( record->getItem("STATUS")->getTrimmedString(0));
                    well->setStatus(currentStep, status);

                }


            }
        }
    }


    void Schedule::handleGCONINJE(DeckConstPtr deck, DeckKeywordConstPtr keyword, LoggerPtr /*logger*/, size_t currentStep) {
        for (size_t recordNr = 0; recordNr < keyword->size(); recordNr++) {
            DeckRecordConstPtr record = keyword->getRecord(recordNr);
            const std::string& groupName = record->getItem("GROUP")->getTrimmedString(0);
            GroupPtr group = getGroup(groupName);

            {
                Phase::PhaseEnum phase = Phase::PhaseEnumFromString( record->getItem("PHASE")->getTrimmedString(0) );
                group->setInjectionPhase( currentStep , phase );
            }
            {
                GroupInjection::ControlEnum controlMode = GroupInjection::ControlEnumFromString( record->getItem("CONTROL_MODE")->getTrimmedString(0) );
                group->setInjectionControlMode( currentStep , controlMode );
            }

            Phase::PhaseEnum wellPhase = Phase::PhaseEnumFromString( record->getItem("PHASE")->getTrimmedString(0));

            // calculate SI injection rates for the group
            double surfaceInjectionRate = record->getItem("SURFACE_TARGET")->getRawDouble(0);
            surfaceInjectionRate = convertInjectionRateToSI(surfaceInjectionRate, wellPhase, *deck->getActiveUnitSystem());
            double reservoirInjectionRate = record->getItem("RESV_TARGET")->getRawDouble(0);
            reservoirInjectionRate = convertInjectionRateToSI(reservoirInjectionRate, wellPhase, *deck->getActiveUnitSystem());

            group->setSurfaceMaxRate( currentStep , surfaceInjectionRate);
            group->setReservoirMaxRate( currentStep , reservoirInjectionRate);
            group->setTargetReinjectFraction( currentStep , record->getItem("REINJ_TARGET")->getSIDouble(0));
            group->setTargetVoidReplacementFraction( currentStep , record->getItem("VOIDAGE_TARGET")->getSIDouble(0));

            group->setProductionGroup(currentStep, false);
        }
    }


    void Schedule::handleGCONPROD(DeckKeywordConstPtr keyword, LoggerPtr /*logger*/, size_t currentStep) {
        for (size_t recordNr = 0; recordNr < keyword->size(); recordNr++) {
            DeckRecordConstPtr record = keyword->getRecord(recordNr);
            const std::string& groupName = record->getItem("GROUP")->getTrimmedString(0);
            GroupPtr group = getGroup(groupName);
            {
                GroupProduction::ControlEnum controlMode = GroupProduction::ControlEnumFromString( record->getItem("CONTROL_MODE")->getTrimmedString(0) );
                group->setProductionControlMode( currentStep , controlMode );
            }
            group->setOilTargetRate( currentStep , record->getItem("OIL_TARGET")->getSIDouble(0));
            group->setGasTargetRate( currentStep , record->getItem("GAS_TARGET")->getSIDouble(0));
            group->setWaterTargetRate( currentStep , record->getItem("WATER_TARGET")->getSIDouble(0));
            group->setLiquidTargetRate( currentStep , record->getItem("LIQUID_TARGET")->getSIDouble(0));
            {
                GroupProductionExceedLimit::ActionEnum exceedAction = GroupProductionExceedLimit::ActionEnumFromString(record->getItem("EXCEED_PROC")->getTrimmedString(0) );
                group->setProductionExceedLimitAction( currentStep , exceedAction );
            }

            group->setProductionGroup(currentStep, true);
        }
    }

    void Schedule::handleCOMPDAT(DeckKeywordConstPtr keyword, LoggerPtr /*logger*/, size_t currentStep) {
        std::map<std::string , std::vector< CompletionPtr> > completionMapList = Completion::completionsFromCOMPDATKeyword( keyword );
        std::map<std::string , std::vector< CompletionPtr> >::iterator iter;

        for( iter= completionMapList.begin(); iter != completionMapList.end(); iter++) {
            const std::string wellName = iter->first;
            WellPtr well = getWell(wellName);
            well->addCompletions(currentStep, iter->second);
        }
    }

    void Schedule::handleWGRUPCON(DeckKeywordConstPtr keyword, LoggerPtr /*logger*/, size_t currentStep) {
        for (size_t recordNr = 0; recordNr < keyword->size(); recordNr++) {
            DeckRecordConstPtr record = keyword->getRecord(recordNr);
            const std::string& wellName = record->getItem("WELL")->getTrimmedString(0);
            WellPtr well = getWell(wellName);

            bool availableForGroupControl = convertEclipseStringToBool(record->getItem("GROUP_CONTROLLED")->getTrimmedString(0));
            well->setAvailableForGroupControl(currentStep, availableForGroupControl);

            well->setGuideRate(currentStep, record->getItem("GUIDE_RATE")->getRawDouble(0));

            if (!record->getItem("PHASE")->defaultApplied(0)) {
                std::string guideRatePhase = record->getItem("PHASE")->getTrimmedString(0);
                well->setGuideRatePhase(currentStep, GuideRate::GuideRatePhaseEnumFromString(guideRatePhase));
            } else
                well->setGuideRatePhase(currentStep, GuideRate::UNDEFINED);

            well->setGuideRateScalingFactor(currentStep, record->getItem("SCALING_FACTOR")->getRawDouble(0));
        }
    }

    void Schedule::handleGRUPTREE(DeckKeywordConstPtr keyword, LoggerPtr /*logger*/, size_t currentStep) {
        GroupTreePtr currentTree = m_rootGroupTree->get(currentStep);
        GroupTreePtr newTree = currentTree->deepCopy();
        for (size_t recordNr = 0; recordNr < keyword->size(); recordNr++) {
            DeckRecordConstPtr record = keyword->getRecord(recordNr);
            const std::string& childName = record->getItem("CHILD_GROUP")->getTrimmedString(0);
            const std::string& parentName = record->getItem("PARENT_GROUP")->getTrimmedString(0);
            newTree->updateTree(childName, parentName);

            if (!hasGroup(parentName))
                addGroup( parentName , currentStep );

            if (!hasGroup(childName))
                addGroup( childName , currentStep );
        }
        m_rootGroupTree->add(currentStep, newTree);
    }

    TimeMapConstPtr Schedule::getTimeMap() const {
        return m_timeMap;
    }

    GroupTreePtr Schedule::getGroupTree(size_t timeStep) const {
        return m_rootGroupTree->get(timeStep);
    }

    void Schedule::addWell(const std::string& wellName, DeckRecordConstPtr record, size_t timeStep) {

        // We change from eclipse's 1 - n, to a 0 - n-1 solution
        int headI = record->getItem("HEAD_I")->getInt(0) - 1;
        int headJ = record->getItem("HEAD_J")->getInt(0) - 1;
        Phase::PhaseEnum preferredPhase = Phase::PhaseEnumFromString(record->getItem("PHASE")->getTrimmedString(0));
        WellPtr well;

        if (!record->getItem("REF_DEPTH")->defaultApplied(0)) {
            double refDepth = record->getItem("REF_DEPTH")->getSIDouble(0);
            well = std::make_shared<Well>(wellName, headI, headJ, refDepth, preferredPhase, m_timeMap , timeStep);
        } else {
            well = std::make_shared<Well>(wellName, headI, headJ, preferredPhase, m_timeMap , timeStep);
        }

        m_wells.insert( wellName  , well);
    }

    size_t Schedule::numWells() const {
        return m_wells.size();
    }

    size_t Schedule::numWells(size_t timestep) const {
      std::vector<WellConstPtr> wells = getWells(timestep);
      return wells.size();
    }


    bool Schedule::hasWell(const std::string& wellName) const {
        return m_wells.hasKey( wellName );
    }

    WellPtr Schedule::getWell(const std::string& wellName) const {
        return m_wells.get( wellName );
    }

    std::vector<WellConstPtr> Schedule::getWells() const {
        return getWells(m_timeMap->size()-1);
    }

    std::vector<WellConstPtr> Schedule::getWells(size_t timeStep) const {
        if (timeStep >= m_timeMap->size()) {
            throw std::invalid_argument("Timestep to large");
        }

        std::vector<WellConstPtr> wells;
        for (auto iter = m_wells.begin(); iter != m_wells.end(); ++iter) {
            WellConstPtr well = *iter;
            if (well->hasBeenDefined(timeStep)) {
                wells.push_back(well);
            }
        }
        return wells;
    }

    std::vector<WellPtr> Schedule::getWells(const std::string& wellNamePattern) const {
        std::vector<WellPtr> wells;
        size_t wildcard_pos = wellNamePattern.find("*");
        if (wildcard_pos == wellNamePattern.length()-1) {
            for (auto wellIter = m_wells.begin(); wellIter != m_wells.end(); ++wellIter) {
                WellPtr well = *wellIter;
                if (wellNamePattern.compare (0, wildcard_pos, well->name(), 0, wildcard_pos) == 0) {
                    wells.push_back (well);
                }
            }
        }
        else {
            wells.push_back(getWell(wellNamePattern));
        }
        return wells;
    }

    void Schedule::addGroup(const std::string& groupName, size_t timeStep) {
        if (!m_timeMap) {
            throw std::invalid_argument("TimeMap is null, can't add group named: " + groupName);
        }
        GroupPtr group(new Group(groupName, m_timeMap , timeStep));
        m_groups[ groupName ] = group;
    }

    size_t Schedule::numGroups() const {
        return m_groups.size();
    }

    bool Schedule::hasGroup(const std::string& groupName) const {
        return m_groups.find(groupName) != m_groups.end();
    }

    GroupPtr Schedule::getGroup(const std::string& groupName) const {
        if (hasGroup(groupName)) {
            return m_groups.at(groupName);
        } else
            throw std::invalid_argument("Group: " + groupName + " does not exist");
    }

    void Schedule::addWellToGroup( GroupPtr newGroup , WellPtr well , size_t timeStep) {
        const std::string currentGroupName = well->getGroupName(timeStep);
        if (currentGroupName != "") {
            GroupPtr currentGroup = getGroup( currentGroupName );
            currentGroup->delWell( timeStep , well->name());
        }
        well->setGroupName(timeStep , newGroup->name());
        newGroup->addWell(timeStep , well);
    }


    double Schedule::convertInjectionRateToSI(double rawRate, WellInjector::TypeEnum wellType, const Opm::UnitSystem &unitSystem) const {
        switch (wellType) {
        case WellInjector::MULTI:
            // multi-phase controlled injectors are a really funny
            // construct in Eclipse: the quantity controlled for is
            // not physically meaningful, i.e. Eclipse adds up
            // MCFT/day and STB/day.
            throw std::logic_error("There is no generic way to handle multi-phase injectors at this level!");

        case WellInjector::OIL:
        case WellInjector::WATER:
            return rawRate * unitSystem.parse("LiquidVolume/Time")->getSIScaling();

        case WellInjector::GAS:
            return rawRate * unitSystem.parse("GasVolume/Time")->getSIScaling();

        default:
            throw std::logic_error("Unknown injector type");
        }
    }

    double Schedule::convertInjectionRateToSI(double rawRate, Phase::PhaseEnum wellPhase, const Opm::UnitSystem &unitSystem) const {
        switch (wellPhase) {
        case Phase::OIL:
        case Phase::WATER:
            return rawRate * unitSystem.parse("LiquidVolume/Time")->getSIScaling();

        case Phase::GAS:
            return rawRate * unitSystem.parse("GasVolume/Time")->getSIScaling();

        default:
            throw std::logic_error("Unknown injection phase");
        }
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


    size_t Schedule::getMaxNumCompletionsForWells(size_t timestep) const {
      size_t ncwmax = 0;
      const std::vector<WellConstPtr>& wells = getWells();
      for (auto wellIter=wells.begin(); wellIter != wells.end(); ++wellIter) {
        WellConstPtr wellPtr = *wellIter;
        CompletionSetConstPtr completionsSetPtr = wellPtr->getCompletions(timestep);

        if (completionsSetPtr->size() > ncwmax )
          ncwmax = completionsSetPtr->size();

      }
      return ncwmax;
    }
}
