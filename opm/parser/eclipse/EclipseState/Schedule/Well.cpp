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

#include <boost/date_time.hpp>

#include <opm/parser/eclipse/EclipseState/Util/Value.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Completion.hpp>



namespace Opm {

    Well::Well(const std::string& name_, std::shared_ptr<const EclipseGrid> grid , int headI, int headJ, Value<double> refDepth , Phase::PhaseEnum preferredPhase,
               TimeMapConstPtr timeMap, size_t creationTimeStep, WellCompletion::CompletionOrderEnum completionOrdering, bool allowCrossFlow)
        : m_status(new DynamicState<WellCommon::StatusEnum>(timeMap, WellCommon::SHUT)),
          m_isAvailableForGroupControl(new DynamicState<bool>(timeMap, true)),
          m_guideRate(new DynamicState<double>(timeMap, -1.0)),
          m_guideRatePhase(new DynamicState<GuideRate::GuideRatePhaseEnum>(timeMap, GuideRate::UNDEFINED)),
          m_guideRateScalingFactor(new DynamicState<double>(timeMap, 1.0)),
          m_isProducer(new DynamicState<bool>(timeMap, true)) ,
          m_completions( new DynamicState<CompletionSetConstPtr>( timeMap , CompletionSetConstPtr( new CompletionSet()) )),
          m_productionProperties( new DynamicState<WellProductionProperties>(timeMap, WellProductionProperties() )),
          m_injectionProperties( new DynamicState<WellInjectionProperties>(timeMap, WellInjectionProperties() )),
          m_polymerProperties( new DynamicState<WellPolymerProperties>(timeMap, WellPolymerProperties() )),
          m_solventFraction( new DynamicState<double>(timeMap, 0.0 )),
          m_groupName( new DynamicState<std::string>( timeMap , "" )),
          m_rft( new DynamicState<bool>(timeMap,false)),
          m_plt( new DynamicState<bool>(timeMap,false)),
          m_timeMap( timeMap ),
          m_headI(headI),
          m_headJ(headJ),
          m_refDepth(refDepth),
          m_preferredPhase(preferredPhase),
          m_grid( grid ),
          m_comporder(completionOrdering),
          m_allowCrossFlow(allowCrossFlow),
          m_is_multi_segment(false),
          m_segmentset(new DynamicState<SegmentSetPtr>(timeMap, SegmentSetPtr(new SegmentSet())))
    {
        m_name = name_;
        m_creationTimeStep = creationTimeStep;


    }

    const std::string& Well::name() const {
        return m_name;
    }


    void Well::switchToProducer( size_t timeStep) {
        WellInjectionProperties p = getInjectionPropertiesCopy(timeStep);

        p.BHPLimit = 0;
        p.dropInjectionControl( Opm::WellInjector::BHP );
        setInjectionProperties( timeStep , p );
    }


    void Well::switchToInjector( size_t timeStep) {
        WellProductionProperties p = getProductionPropertiesCopy(timeStep);

        p.BHPLimit = 0;
        p.dropProductionControl( Opm::WellProducer::BHP );
        setProductionProperties( timeStep , p );
    }



    bool Well::setProductionProperties(size_t timeStep , const WellProductionProperties newProperties) {
        if (isInjector(timeStep))
            switchToProducer( timeStep );

        m_isProducer->update(timeStep , true);
        return m_productionProperties->update(timeStep, newProperties);
    }

    WellProductionProperties Well::getProductionPropertiesCopy(size_t timeStep) const {
        return m_productionProperties->get(timeStep);
    }

    const WellProductionProperties& Well::getProductionProperties(size_t timeStep) const {
        return m_productionProperties->at(timeStep);
    }

    bool Well::setInjectionProperties(size_t timeStep , const WellInjectionProperties newProperties) {
        if (isProducer(timeStep))
            switchToInjector( timeStep );

        m_isProducer->update(timeStep , false);
        return m_injectionProperties->update(timeStep, newProperties);
    }

    WellInjectionProperties Well::getInjectionPropertiesCopy(size_t timeStep) const {
        return m_injectionProperties->get(timeStep);
    }

    const WellInjectionProperties& Well::getInjectionProperties(size_t timeStep) const {
        return m_injectionProperties->at(timeStep);
    }

    bool Well::setPolymerProperties(size_t timeStep , const WellPolymerProperties newProperties) {
        m_isProducer->update(timeStep , false);
        return m_polymerProperties->update(timeStep, newProperties);
    }

    WellPolymerProperties Well::getPolymerPropertiesCopy(size_t timeStep) const {
        return m_polymerProperties->get(timeStep);
    }

    const WellPolymerProperties& Well::getPolymerProperties(size_t timeStep) const {
        return m_polymerProperties->at(timeStep);
    }

    bool Well::setSolventFraction(size_t timeStep , const double fraction) {
        m_isProducer->update(timeStep , false);
        return m_solventFraction->update(timeStep, fraction);
    }

    const double& Well::getSolventFraction(size_t timeStep) const {
        return m_solventFraction->at(timeStep);
    }

    bool Well::hasBeenDefined(size_t timeStep) const {
        if (timeStep < m_creationTimeStep)
            return false;
        else
            return true;
    }

    WellCommon::StatusEnum Well::getStatus(size_t timeStep) const {
        return m_status->get( timeStep );
    }

    bool Well::setStatus(size_t timeStep, WellCommon::StatusEnum status) {
        if ((WellCommon::StatusEnum::OPEN == status) && getCompletions(timeStep)->allCompletionsShut()) {
            std::cerr << "ERROR when handling keyword for well "<< name()  << ": Cannot open a well where all completions are shut" << std::endl;
            return false;
        } else
            return m_status->update( timeStep , status );
    }

    bool Well::isProducer(size_t timeStep) const {
        return m_isProducer->get(timeStep);
    }

    bool Well::isInjector(size_t timeStep) const {
        return !isProducer(timeStep);
    }

    bool Well::isAvailableForGroupControl(size_t timeStep) const {
        return m_isAvailableForGroupControl->get(timeStep);
    }

    void Well::setAvailableForGroupControl(size_t timeStep, bool isAvailableForGroupControl_) {
        m_isAvailableForGroupControl->update(timeStep, isAvailableForGroupControl_);
    }

    double Well::getGuideRate(size_t timeStep) const {
        return m_guideRate->get(timeStep);
    }

    void Well::setGuideRate(size_t timeStep, double guideRate) {
        m_guideRate->update(timeStep, guideRate);
    }

    GuideRate::GuideRatePhaseEnum Well::getGuideRatePhase(size_t timeStep) const {
        return m_guideRatePhase->get(timeStep);
    }

    void Well::setGuideRatePhase(size_t timeStep, GuideRate::GuideRatePhaseEnum phase) {
        m_guideRatePhase->update(timeStep, phase);
    }

    double Well::getGuideRateScalingFactor(size_t timeStep) const {
        return m_guideRateScalingFactor->get(timeStep);
    }

    void Well::setGuideRateScalingFactor(size_t timeStep, double scalingFactor) {
        m_guideRateScalingFactor->update(timeStep, scalingFactor);
    }

    /*****************************************************************/

    // WELSPECS

    int Well::getHeadI() const {
        return m_headI;
    }

    int Well::getHeadJ() const {
        return m_headJ;
    }


    double Well::getRefDepth() const{
        if (!m_refDepth.hasValue())
            setRefDepthFromCompletions();

        return m_refDepth.getValue();
    }


    void Well::setRefDepthFromCompletions() const {
        size_t timeStep = m_creationTimeStep;
        while (true) {
            auto completions = getCompletions( timeStep );
            if (completions->size() > 0) {
                auto firstCompletion = completions->get(0);
                double depth = m_grid->getCellDepth( firstCompletion->getI() , firstCompletion->getJ() , firstCompletion->getK());
                m_refDepth.setValue( depth );
                break;
            } else {
                timeStep++;
                if (timeStep >= m_timeMap->size())
                    throw std::invalid_argument("No completions defined for well: " + name() + " can not infer reference depth");
            }
        }
    }


    Phase::PhaseEnum Well::getPreferredPhase() const {
        return m_preferredPhase;
    }

    CompletionSetConstPtr Well::getCompletions(size_t timeStep) const {
        return m_completions->get( timeStep );
    }

    void Well::addCompletions(size_t time_step , const std::vector<CompletionPtr>& newCompletions) {
        CompletionSetConstPtr currentCompletionSet = m_completions->get(time_step);
        CompletionSetPtr newCompletionSet = CompletionSetPtr( currentCompletionSet->shallowCopy() );

        for (size_t ic = 0; ic < newCompletions.size(); ic++) {
            newCompletions[ic]->fixDefaultIJ( m_headI , m_headJ );
            newCompletionSet->add( newCompletions[ic] );
        }

        addCompletionSet( time_step , newCompletionSet);
    }

    void Well::addCompletionSet(size_t time_step, const CompletionSetConstPtr newCompletionSet){
        CompletionSetPtr mutable_copy(newCompletionSet->shallowCopy());
        if (getWellCompletionOrdering() == WellCompletion::TRACK) {
            mutable_copy->orderCompletions(m_headI, m_headJ, m_grid);
        }
        m_completions->update(time_step, mutable_copy);
    }

    const std::string Well::getGroupName(size_t time_step) const {
        return m_groupName->get(time_step);
    }


    void Well::setGroupName(size_t time_step, const std::string& groupName ) {
        m_groupName->update(time_step , groupName);
    }



    void Well::setRFTActive(size_t time_step, bool value){
        m_rft->update(time_step, value);
    }

    bool Well::getRFTActive(size_t time_step) const{
        return m_rft->get(time_step);
    }

    bool Well::getPLTActive(size_t time_step) const{
     return m_plt->get(time_step);
    }
    void Well::setPLTActive(size_t time_step, bool value){
        m_plt->update(time_step, value);
    }

    int Well::findWellFirstOpen(int startTimeStep) const{
        int numberOfTimeSteps = m_timeMap->numTimesteps();
        for(int i = startTimeStep; i < numberOfTimeSteps;i++){
            if(getStatus(i)==WellCommon::StatusEnum::OPEN){
                return i;
            }
        }
        return -1;
    }

    void Well::setRFTForWellWhenFirstOpen(int numSteps,size_t currentStep){
        int time;
        if(getStatus(currentStep)==WellCommon::StatusEnum::OPEN ){
            time = currentStep;
        }else {
            time = findWellFirstOpen(currentStep);
        }
        if(time>-1){
            setRFTActive(time, true);
            if(time < numSteps){
                setRFTActive(time+1, false);
            }
        }
    }

    WellCompletion::CompletionOrderEnum Well::getWellCompletionOrdering() const {
        return m_comporder;
    }


    bool Well::wellNameInWellNamePattern(const std::string& wellName, const std::string& wellNamePattern) {
        bool wellNameInPattern = false;
        if (util_fnmatch( wellNamePattern.c_str() , wellName.c_str()) == 0) {
            wellNameInPattern = true;
        }
        return wellNameInPattern;
    }

    bool Well::getAllowCrossFlow() const {
        return m_allowCrossFlow;
    }

    bool Well::canOpen(size_t currentStep) const {
        bool canOpen = true;
        if (!getAllowCrossFlow()) {
            if ( isInjector(currentStep) ) {
                if (getInjectionProperties(currentStep).surfaceInjectionRate == 0) {;
                    canOpen = false;
                }
            } else {
                if ( (getProductionProperties(currentStep).WaterRate + getProductionProperties(currentStep).OilRate +
                      getProductionProperties(currentStep).GasRate) == 0) {
                    canOpen = false;
                }
            }
        }
        return canOpen;
    }

    bool Well::isMultiSegment() const {
        return m_is_multi_segment;
    }

    void Well::setMultiSegment(const bool is_multi_segment) {
        m_is_multi_segment = is_multi_segment;
    }

    SegmentSetConstPtr Well::getSegmentSet(size_t time_step) const {
        return m_segmentset->get(time_step);
    }


}



