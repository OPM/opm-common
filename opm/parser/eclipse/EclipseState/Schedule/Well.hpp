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


#ifndef WELL_HPP_
#define WELL_HPP_

#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Completion.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>

#include <boost/optional.hpp>

#include <memory>
#include <string>
#include <limits>

namespace Opm {

    typedef struct WellProductionProperties {
        double  OilRate;
        double  GasRate;
        double  WaterRate;
        double  LiquidRate;
        double  ResVRate;
        int     ProductionControls;
        WellProductionProperties()
            {OilRate=0.0; GasRate=0.0; WaterRate=0.0; LiquidRate=0.0; ResVRate=0.0; ProductionControls=0;}
        WellProductionProperties(const WellProductionProperties& props)
            {OilRate=props.OilRate; GasRate=props.GasRate; WaterRate=props.WaterRate;
             LiquidRate=props.LiquidRate; ResVRate=props.ResVRate; ProductionControls=props.ProductionControls;}
    } WellProductionProperties;

    class Well {
    public:
        Well(const std::string& name, int headI, int headJ, double refDepth, TimeMapConstPtr timeMap , size_t creationTimeStep);
        const std::string& name() const;

        bool hasBeenDefined(size_t timeStep) const;
        const std::string getGroupName(size_t timeStep) const;
        void setGroupName(size_t timeStep , const std::string& groupName);

        double getSurfaceInjectionRate(size_t timeStep) const;
        void   setSurfaceInjectionRate(size_t timeStep, double injectionRate);
        double getReservoirInjectionRate(size_t timeStep) const;
        void   setReservoirInjectionRate(size_t timeStep, double injectionRate);
        double getBHPLimit(size_t timeStep) const;
        void   setBHPLimit(size_t timeStep, double BHPLimit , bool producer);
        double getTHPLimit(size_t timeStep) const;
        void   setTHPLimit(size_t timeStep, double THPLimit , bool producer);
        WellInjector::TypeEnum getInjectorType(size_t timeStep) const;
        void                   setInjectorType(size_t timeStep, WellInjector::TypeEnum injectorType);
        WellInjector::ControlModeEnum getInjectorControlMode(size_t timeStep) const;
        void                          setInjectorControlMode(size_t timeStep, WellInjector::ControlModeEnum injectorControlMode);
        WellProducer::ControlModeEnum getProducerControlMode(size_t timeStep) const;
        void                          setProducerControlMode(size_t timeStep, WellProducer::ControlModeEnum controlMode);
        WellCommon::StatusEnum getStatus(size_t timeStep) const;
        void                   setStatus(size_t timeStep, WellCommon::StatusEnum Status);
        
        bool hasProductionControl(size_t timeStep , WellProducer::ControlModeEnum controlMode) const;
        void dropProductionControl(size_t timeStep , WellProducer::ControlModeEnum controlMode);
        
        bool hasInjectionControl(size_t timeStep   , WellInjector::ControlModeEnum controlMode) const;
        void dropInjectionControl(size_t timeStep   , WellInjector::ControlModeEnum controlMode);


        int    getHeadI() const;
        int    getHeadJ() const;
        double getRefDepth() const;
        
        bool isInPredictionMode(size_t timeStep) const;
        void setInPredictionMode(size_t timeStep, bool isInPredictionMode);
        bool isProducer(size_t timeStep) const;
        bool isInjector(size_t timeStep) const;

        bool isAvailableForGroupControl(size_t timeStep) const;
        void setAvailableForGroupControl(size_t timeStep, bool isAvailableForGroupControl);
        double getGuideRate(size_t timeStep) const;
        void setGuideRate(size_t timeStep, double guideRate);
        GuideRate::GuideRatePhaseEnum getGuideRatePhase(size_t timeStep) const;
        void setGuideRatePhase(size_t timeStep, GuideRate::GuideRatePhaseEnum phase);
        double getGuideRateScalingFactor(size_t timeStep) const;
        void setGuideRateScalingFactor(size_t timeStep, double scalingFactor);

        void addWELSPECS(DeckRecordConstPtr deckRecord);
        void addCompletions(size_t time_step , const std::vector<CompletionConstPtr>& newCompletions);
               CompletionSetConstPtr getCompletions(size_t timeStep) const;
        void setProductionProperties(size_t timeStep , const WellProductionProperties properties);
        WellProductionProperties getProductionProperties(size_t timeStep) const;

    private:
        void switch2Producer(size_t timeStep );
        void switch2Injector(size_t timeStep );

        void addInjectionControl(size_t timeStep   , WellInjector::ControlModeEnum controlMode);
        void addProductionControl(size_t timeStep , WellProducer::ControlModeEnum controlMode);
        
        
        size_t m_creationTimeStep;
        std::string m_name;

        std::shared_ptr<DynamicState<double> > m_surfaceInjectionRate;
        std::shared_ptr<DynamicState<double> > m_reservoirInjectionRate;
        std::shared_ptr<DynamicState<double> > m_BHPLimit;
        std::shared_ptr<DynamicState<double> > m_THPLimit;
        std::shared_ptr<DynamicState<WellInjector::TypeEnum> > m_injectorType;
        std::shared_ptr<DynamicState<WellInjector::ControlModeEnum> > m_injectorControlMode;
        std::shared_ptr<DynamicState<WellProducer::ControlModeEnum> > m_producerControlMode;
        std::shared_ptr<DynamicState<WellCommon::StatusEnum> > m_status;
        std::shared_ptr<DynamicState<int> > m_injectionControls;
        
        std::shared_ptr<DynamicState<bool> > m_inPredictionMode;
        std::shared_ptr<DynamicState<bool> > m_isProducer;
        std::shared_ptr<DynamicState<bool> > m_isAvailableForGroupControl;
        std::shared_ptr<DynamicState<double> > m_guideRate;
        std::shared_ptr<DynamicState<GuideRate::GuideRatePhaseEnum> > m_guideRatePhase;
        std::shared_ptr<DynamicState<double> > m_guideRateScalingFactor;

        std::shared_ptr<DynamicState<CompletionSetConstPtr> > m_completions;
        std::shared_ptr<DynamicState<WellProductionProperties> > m_productionProperties;
        std::shared_ptr<DynamicState<std::string> > m_groupName;

        // WELSPECS data - assumes this is not dynamic
        int m_headI;
        int m_headJ;
        double m_refDepth;
    };
    typedef std::shared_ptr<Well> WellPtr;
    typedef std::shared_ptr<const Well> WellConstPtr;
}



#endif /* WELL_HPP_ */
