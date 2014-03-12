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

    Well::Well(const std::string& name, int headI, int headJ, double refDepth, TimeMapConstPtr timeMap , size_t creationTimeStep)
        : //m_injectorType(new DynamicState<WellInjector::TypeEnum>(timeMap, WellInjector::WATER)),
          m_injectorControlMode(new DynamicState<WellInjector::ControlModeEnum>(timeMap, WellInjector::RATE)),
          m_producerControlMode(new DynamicState<WellProducer::ControlModeEnum>(timeMap, WellProducer::ORAT)),
          m_status(new DynamicState<WellCommon::StatusEnum>(timeMap, WellCommon::OPEN)),
          //m_injectionControls(new DynamicState<int>(timeMap, 0)),
          m_inPredictionMode(new DynamicState<bool>(timeMap, true)),
          m_isProducer(new DynamicState<bool>(timeMap, true)),
          m_isAvailableForGroupControl(new DynamicState<bool>(timeMap, true)),
          m_guideRate(new DynamicState<double>(timeMap, -1.0)),
          m_guideRatePhase(new DynamicState<GuideRate::GuideRatePhaseEnum>(timeMap, GuideRate::UNDEFINED)),
          m_guideRateScalingFactor(new DynamicState<double>(timeMap, 1.0)),
          m_completions( new DynamicState<CompletionSetConstPtr>( timeMap , CompletionSetConstPtr( new CompletionSet()) )),
          m_productionProperties( new DynamicState<WellProductionProperties>(timeMap, WellProductionProperties() )),
          m_injectionProperties( new DynamicState<WellInjectionProperties>(timeMap, WellInjectionProperties() )),
          m_groupName( new DynamicState<std::string>( timeMap , "" )),
          m_headI(headI),
          m_headJ(headJ),
          m_refDepth(refDepth)      
    {
        m_name = name;
        m_creationTimeStep = creationTimeStep;
    }

    const std::string& Well::name() const {
        return m_name;
    }


    void Well::setProductionProperties(size_t timeStep , const WellProductionProperties newProperties) {
        m_isProducer->add(timeStep , true);
        m_productionProperties->add(timeStep, newProperties);
    }

    WellProductionProperties Well::getProductionProperties(size_t timeStep) const {
        return m_productionProperties->get(timeStep);
    }

    void Well::setInjectionProperties(size_t timeStep , const WellInjectionProperties newProperties) {
        m_isProducer->add(timeStep , false);
        m_injectionProperties->add(timeStep, newProperties);
    }

    WellInjectionProperties Well::getInjectionProperties(size_t timeStep) const {
        return m_injectionProperties->get(timeStep);
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
    

//    WellInjector::TypeEnum Well::getInjectorType(size_t timeStep) const {
//        return m_injectorType->get(timeStep);
//    }

//    void Well::setInjectorType(size_t timeStep, WellInjector::TypeEnum injectorType) {
//        m_injectorType->add(timeStep , injectorType);
//    }

    WellInjector::ControlModeEnum Well::getInjectorControlMode(size_t timeStep) const {
        return m_injectorControlMode->get(timeStep);
    }

    void Well::setInjectorControlMode(size_t timeStep, WellInjector::ControlModeEnum injectorControlMode) {
        m_injectorControlMode->add(timeStep , injectorControlMode);
    }

    WellProducer::ControlModeEnum Well::getProducerControlMode(size_t timeStep) const {
        return m_producerControlMode->get(timeStep);
    }

    void Well::setProducerControlMode(size_t timeStep, WellProducer::ControlModeEnum controlMode) {
        m_producerControlMode->add(timeStep , controlMode);
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
    
    void Well::setAvailableForGroupControl(size_t timeStep, bool isAvailableForGroupControl) {
        m_isAvailableForGroupControl->add(timeStep, isAvailableForGroupControl);
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


    void Well::switch2Producer(size_t timeStep ) {
        m_isProducer->add(timeStep , true);
//        m_surfaceInjectionRate->add(timeStep, 0);
//        m_reservoirInjectionRate->add(timeStep , 0);
    }

    void Well::switch2Injector(size_t timeStep ) {
        m_isProducer->add(timeStep , false);
//        m_oilRate->add(timeStep, 0);
//        m_gasRate->add(timeStep, 0);
//        m_waterRate->add(timeStep, 0);
    }

    bool Well::isInPredictionMode(size_t timeStep) const {
        return m_inPredictionMode->get(timeStep);
    }
    
    void Well::setInPredictionMode(size_t timeStep, bool inPredictionMode) {
        m_inPredictionMode->add(timeStep, inPredictionMode);
    }

    /*****************************************************************/

    bool Well::hasProductionControl(size_t timeStep , WellProducer::ControlModeEnum controlMode) const {
        WellProductionProperties properties = getProductionProperties(timeStep);
        if (properties.ProductionControls & controlMode)
            return true;
        else
            return false;
    }

    
    void Well::addProductionControl(size_t timeStep , WellProducer::ControlModeEnum controlMode) {
        WellProductionProperties properties = getProductionProperties(timeStep);
        if ((properties.ProductionControls & controlMode) == 0) {
            properties.ProductionControls += controlMode;
            setProductionProperties(timeStep, properties);
        }
    }

    
    void Well::dropProductionControl(size_t timeStep , WellProducer::ControlModeEnum controlMode) {
        WellProductionProperties properties = getProductionProperties(timeStep);
        if ((properties.ProductionControls & controlMode) != 0) {
            properties.ProductionControls -= controlMode;
            setProductionProperties(timeStep, properties);
        }
    }

    
    bool Well::hasInjectionControl(size_t timeStep , WellInjector::ControlModeEnum controlMode) const {
        WellInjectionProperties properties = getInjectionProperties(timeStep);
        if (properties.InjectionControls & controlMode)
            return true;
        else
            return false;
    }

    
    void Well::addInjectionControl(size_t timeStep , WellInjector::ControlModeEnum controlMode) {
        WellInjectionProperties properties = getInjectionProperties(timeStep);
        if ((properties.InjectionControls & controlMode) == 0) {
            properties.InjectionControls += controlMode;
            setInjectionProperties(timeStep, properties);
        }
    }

    
    void Well::dropInjectionControl(size_t timeStep , WellInjector::ControlModeEnum controlMode) {
        WellInjectionProperties properties = getInjectionProperties(timeStep);
        if ((properties.InjectionControls & controlMode) != 0) {
            properties.InjectionControls -= controlMode;
            setInjectionProperties(timeStep, properties);
        }
    }

    /*****************************************************************/


    // WELSPECS
    
    int Well::getHeadI() const {
        return m_headI;
    }

    int Well::getHeadJ() const {
        return m_headJ;
    }

    double Well::getRefDepth() const {
        return m_refDepth;
    }

    CompletionSetConstPtr Well::getCompletions(size_t timeStep) const {
        return m_completions->get( timeStep );
    }
    
    void Well::addCompletions(size_t time_step , const std::vector<CompletionConstPtr>& newCompletions) {
        CompletionSetConstPtr currentCompletionSet = m_completions->get(time_step);
        CompletionSetPtr newCompletionSet = CompletionSetPtr( currentCompletionSet->shallowCopy() );

        for (size_t ic = 0; ic < newCompletions.size(); ic++) 
            newCompletionSet->add( newCompletions[ic] );

        m_completions->add( time_step , newCompletionSet);
    }
    
    const std::string Well::getGroupName(size_t time_step) const {
        return m_groupName->get(time_step);
    }


    void Well::setGroupName(size_t time_step, const std::string& groupName ) {
        m_groupName->add(time_step , groupName);
    }


}



