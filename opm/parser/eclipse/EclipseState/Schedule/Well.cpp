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
        : //m_oilRate( new DynamicState<double>( timeMap , 0.0)) ,
          //m_gasRate(new DynamicState<double>(timeMap, 0.0)),
          //m_waterRate(new DynamicState<double>(timeMap, 0.0)),
          //m_liquidRate(new DynamicState<double>(timeMap, 0.0)),
          //m_resVRate(new DynamicState<double>(timeMap, 0.0)),
          m_surfaceInjectionRate(new DynamicState<double>(timeMap, 0.0)),
          m_reservoirInjectionRate(new DynamicState<double>(timeMap, 0.0)),
          m_BHPLimit(new DynamicState<double>(timeMap , 0.0)),
          m_THPLimit(new DynamicState<double>(timeMap , 0.0)),
          m_injectorType(new DynamicState<WellInjector::TypeEnum>(timeMap, WellInjector::WATER)),
          m_injectorControlMode(new DynamicState<WellInjector::ControlModeEnum>(timeMap, WellInjector::RATE)),
          m_producerControlMode(new DynamicState<WellProducer::ControlModeEnum>(timeMap, WellProducer::ORAT)),
          m_status(new DynamicState<WellCommon::StatusEnum>(timeMap, WellCommon::OPEN)),
          //m_productionControls(new DynamicState<int>(timeMap, 0)),
          m_injectionControls(new DynamicState<int>(timeMap, 0)),
          m_inPredictionMode(new DynamicState<bool>(timeMap, true)),
          m_isProducer(new DynamicState<bool>(timeMap, true)),
          m_isAvailableForGroupControl(new DynamicState<bool>(timeMap, true)),
          m_guideRate(new DynamicState<double>(timeMap, -1.0)),
          m_guideRatePhase(new DynamicState<GuideRate::GuideRatePhaseEnum>(timeMap, GuideRate::UNDEFINED)),
          m_guideRateScalingFactor(new DynamicState<double>(timeMap, 1.0)),
          m_completions( new DynamicState<CompletionSetConstPtr>( timeMap , CompletionSetConstPtr( new CompletionSet()) )),
          m_productionProperties( new DynamicState<WellProductionProperties>(timeMap, WellProductionProperties() )),
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
        WellProductionProperties copyToBeSaved = getProductionProperties(timeStep);
        if ((copyToBeSaved.ProductionControls != newProperties.ProductionControls)) {
            copyToBeSaved.ProductionControls = newProperties.ProductionControls;
        }
        if (copyToBeSaved.OilRate != newProperties.OilRate) {
            copyToBeSaved.OilRate = newProperties.OilRate;
            copyToBeSaved.ProductionControls += WellProducer::ORAT;
        }
        if (copyToBeSaved.GasRate != newProperties.GasRate) {
            copyToBeSaved.GasRate = newProperties.GasRate;
            copyToBeSaved.ProductionControls += WellProducer::GRAT;
        }
        if (copyToBeSaved.WaterRate != newProperties.WaterRate) {
            copyToBeSaved.WaterRate = newProperties.WaterRate;
            copyToBeSaved.ProductionControls += WellProducer::WRAT;
        }
        if (copyToBeSaved.LiquidRate != newProperties.LiquidRate) {
            copyToBeSaved.LiquidRate = newProperties.LiquidRate;
            copyToBeSaved.ProductionControls += WellProducer::LRAT;
        }
        if (copyToBeSaved.ResVRate != newProperties.ResVRate) {
            copyToBeSaved.ResVRate = newProperties.ResVRate;
            copyToBeSaved.ProductionControls += WellProducer::RESV;
        }
        m_productionProperties->add(timeStep, copyToBeSaved);
    }

    WellProductionProperties Well::getProductionProperties(size_t timeStep) const {
        return m_productionProperties->get(timeStep);
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
    

    double Well::getBHPLimit(size_t timeStep) const {
        return m_BHPLimit->get(timeStep);
    }

    void Well::setBHPLimit(size_t timeStep, double BHPLimit , bool producer) {
        m_BHPLimit->add(timeStep, BHPLimit);
        if (producer)
            addProductionControl( timeStep , WellProducer::BHP);
        else
            addInjectionControl( timeStep , WellInjector::BHP );
    }


    double Well::getTHPLimit(size_t timeStep) const {
        return m_THPLimit->get(timeStep);
    }

    void Well::setTHPLimit(size_t timeStep, double THPLimit , bool producer) {
        m_THPLimit->add(timeStep, THPLimit);
        if (producer)
            addProductionControl( timeStep , WellProducer::THP);
        else
            addInjectionControl( timeStep , WellInjector::THP );
    }

    WellInjector::TypeEnum Well::getInjectorType(size_t timeStep) const {
        return m_injectorType->get(timeStep);
    }

    void Well::setInjectorType(size_t timeStep, WellInjector::TypeEnum injectorType) {
        m_injectorType->add(timeStep , injectorType);
    }

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


    double Well::getOilRate(size_t timeStep) const {
        return getProductionProperties(timeStep).OilRate;
    }

    void Well::setOilRate(size_t timeStep, double oilRate) {
        WellProductionProperties properties = getProductionProperties(timeStep);
        properties.OilRate = oilRate;
        setProductionProperties(timeStep, properties);
    }


    double Well::getGasRate(size_t timeStep) const {
        return getProductionProperties(timeStep).GasRate;
    }

    void Well::setGasRate(size_t timeStep, double gasRate) {
        WellProductionProperties properties = getProductionProperties(timeStep);
        properties.GasRate = gasRate;
        setProductionProperties(timeStep, properties);
    }

    double Well::getWaterRate(size_t timeStep) const {
        return getProductionProperties(timeStep).WaterRate;
    }

    void Well::setWaterRate(size_t timeStep, double waterRate) {
        WellProductionProperties properties = getProductionProperties(timeStep);
        properties.WaterRate = waterRate;
        setProductionProperties(timeStep, properties);
    }

    double Well::getLiquidRate(size_t timeStep) const {
        return getProductionProperties(timeStep).LiquidRate;
    }

    void Well::setLiquidRate(size_t timeStep, double liquidRate) {
        WellProductionProperties properties = getProductionProperties(timeStep);
        properties.LiquidRate = liquidRate;
        setProductionProperties(timeStep, properties);
    }

    double Well::getResVRate(size_t timeStep) const {
        return getProductionProperties(timeStep).ResVRate;
    }

    void Well::setResVRate(size_t timeStep, double resvRate) {
        WellProductionProperties properties = getProductionProperties(timeStep);
        properties.ResVRate = resvRate;
        setProductionProperties(timeStep, properties);
    }

    double Well::getSurfaceInjectionRate(size_t timeStep) const {
        return m_surfaceInjectionRate->get(timeStep);
    }

    void Well::setSurfaceInjectionRate(size_t timeStep, double injectionRate) {
        m_surfaceInjectionRate->add(timeStep, injectionRate);
        switch2Injector( timeStep );
        addInjectionControl( timeStep , WellInjector::RATE );
    }

    double Well::getReservoirInjectionRate(size_t timeStep) const {
        return m_reservoirInjectionRate->get(timeStep);
    }

    void Well::setReservoirInjectionRate(size_t timeStep, double injectionRate) {
        m_reservoirInjectionRate->add(timeStep, injectionRate);
        switch2Injector( timeStep );
        addInjectionControl( timeStep , WellInjector::RESV );
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
        if ((properties.ProductionControls & controlMode) != 0)
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
        int controls = m_injectionControls->get( timeStep );
        if (controls & controlMode)
            return true;
        else
            return false;
    }

    
    void Well::addInjectionControl(size_t timeStep , WellInjector::ControlModeEnum controlMode) {
        int controls = m_injectionControls->get( timeStep );
        if ((controls & controlMode) == 0) {
            controls += controlMode;
            m_injectionControls->add(timeStep , controls );
        }
    }

    
    void Well::dropInjectionControl(size_t timeStep , WellInjector::ControlModeEnum controlMode) {
        int controls = m_injectionControls->get( timeStep );
        if ((controls & controlMode) != 0) {
            controls -= controlMode;
            m_injectionControls->add(timeStep , controls );
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



