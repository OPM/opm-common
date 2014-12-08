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

#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Completion.hpp>

namespace Opm {

    Well::Well(const std::string& name_, int headI, int headJ, double refDepth, Phase::PhaseEnum preferredPhase,
               TimeMapConstPtr timeMap, size_t creationTimeStep)
        : m_status(new DynamicState<WellCommon::StatusEnum>(timeMap, WellCommon::OPEN)),
          m_isAvailableForGroupControl(new DynamicState<bool>(timeMap, true)),
          m_guideRate(new DynamicState<double>(timeMap, -1.0)),
          m_guideRatePhase(new DynamicState<GuideRate::GuideRatePhaseEnum>(timeMap, GuideRate::UNDEFINED)),
          m_guideRateScalingFactor(new DynamicState<double>(timeMap, 1.0)),
          m_isProducer(new DynamicState<bool>(timeMap, true)) ,
          m_completions( new DynamicState<CompletionSetConstPtr>( timeMap , CompletionSetConstPtr( new CompletionSet()) )),
          m_productionProperties( new DynamicState<WellProductionProperties>(timeMap, WellProductionProperties() )),
          m_injectionProperties( new DynamicState<WellInjectionProperties>(timeMap, WellInjectionProperties() )),
          m_polymerProperties( new DynamicState<WellPolymerProperties>(timeMap, WellPolymerProperties() )),
          m_groupName( new DynamicState<std::string>( timeMap , "" )),
          m_headI(headI),
          m_headJ(headJ),
          m_refDepthDefaulted(false),
          m_refDepth(refDepth),
          m_preferredPhase(preferredPhase)
    {
        m_name = name_;
        m_creationTimeStep = creationTimeStep;
    }

    Well::Well(const std::string& name_, int headI, int headJ, Phase::PhaseEnum preferredPhase,
               TimeMapConstPtr timeMap, size_t creationTimeStep)
        : m_status(new DynamicState<WellCommon::StatusEnum>(timeMap, WellCommon::OPEN)),
          m_isAvailableForGroupControl(new DynamicState<bool>(timeMap, true)),
          m_guideRate(new DynamicState<double>(timeMap, -1.0)),
          m_guideRatePhase(new DynamicState<GuideRate::GuideRatePhaseEnum>(timeMap, GuideRate::UNDEFINED)),
          m_guideRateScalingFactor(new DynamicState<double>(timeMap, 1.0)),
          m_isProducer(new DynamicState<bool>(timeMap, true)) ,
          m_completions( new DynamicState<CompletionSetConstPtr>( timeMap , CompletionSetConstPtr( new CompletionSet()) )),
          m_productionProperties( new DynamicState<WellProductionProperties>(timeMap, WellProductionProperties() )),
          m_injectionProperties( new DynamicState<WellInjectionProperties>(timeMap, WellInjectionProperties() )),
          m_polymerProperties( new DynamicState<WellPolymerProperties>(timeMap, WellPolymerProperties() )),
          m_groupName( new DynamicState<std::string>( timeMap , "" )),
          m_headI(headI),
          m_headJ(headJ),
          m_refDepthDefaulted(true),
          m_refDepth(-1e100),
          m_preferredPhase(preferredPhase)
    {
        m_name = name_;
        m_creationTimeStep = creationTimeStep;
    }

    const std::string& Well::name() const {
        return m_name;
    }


    void Well::setProductionProperties(size_t timeStep , const WellProductionProperties newProperties) {
        m_isProducer->add(timeStep , true);
        m_productionProperties->add(timeStep, newProperties);
    }

    WellProductionProperties Well::getProductionPropertiesCopy(size_t timeStep) const {
        return m_productionProperties->get(timeStep);
    }

    const WellProductionProperties& Well::getProductionProperties(size_t timeStep) const {
        return m_productionProperties->at(timeStep);
    }

    void Well::setInjectionProperties(size_t timeStep , const WellInjectionProperties newProperties) {
        m_isProducer->add(timeStep , false);
        m_injectionProperties->add(timeStep, newProperties);
    }

    WellInjectionProperties Well::getInjectionPropertiesCopy(size_t timeStep) const {
        return m_injectionProperties->get(timeStep);
    }

    const WellInjectionProperties& Well::getInjectionProperties(size_t timeStep) const {
        return m_injectionProperties->at(timeStep);
    }

    void Well::setPolymerProperties(size_t timeStep , const WellPolymerProperties newProperties) {
        m_isProducer->add(timeStep , false);
        m_polymerProperties->add(timeStep, newProperties);
    }

    WellPolymerProperties Well::getPolymerPropertiesCopy(size_t timeStep) const {
        return m_polymerProperties->get(timeStep);
    }

    const WellPolymerProperties& Well::getPolymerProperties(size_t timeStep) const {
        return m_polymerProperties->at(timeStep);
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

    void Well::setStatus(size_t timeStep, WellCommon::StatusEnum status) {
        m_status->add( timeStep , status );
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
        m_isAvailableForGroupControl->add(timeStep, isAvailableForGroupControl_);
    }

    double Well::getGuideRate(size_t timeStep) const {
        return m_guideRate->get(timeStep);
    }

    void Well::setGuideRate(size_t timeStep, double guideRate) {
        m_guideRate->add(timeStep, guideRate);
    }

    GuideRate::GuideRatePhaseEnum Well::getGuideRatePhase(size_t timeStep) const {
        return m_guideRatePhase->get(timeStep);
    }

    void Well::setGuideRatePhase(size_t timeStep, GuideRate::GuideRatePhaseEnum phase) {
        m_guideRatePhase->add(timeStep, phase);
    }

    double Well::getGuideRateScalingFactor(size_t timeStep) const {
        return m_guideRateScalingFactor->get(timeStep);
    }

    void Well::setGuideRateScalingFactor(size_t timeStep, double scalingFactor) {
        m_guideRateScalingFactor->add(timeStep, scalingFactor);
    }

    /*****************************************************************/

    // WELSPECS

    int Well::getHeadI() const {
        return m_headI;
    }

    int Well::getHeadJ() const {
        return m_headJ;
    }

    bool Well::getRefDepthDefaulted() const {
        return m_refDepthDefaulted;
    }

    double Well::getRefDepth() const {
        return m_refDepth;
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

        m_completions->add( time_step , newCompletionSet);
    }

    const std::string Well::getGroupName(size_t time_step) const {
        return m_groupName->get(time_step);
    }


    void Well::setGroupName(size_t time_step, const std::string& groupName ) {
        m_groupName->add(time_step , groupName);
    }


}



