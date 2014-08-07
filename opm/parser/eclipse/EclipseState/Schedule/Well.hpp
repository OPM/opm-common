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
#include <opm/parser/eclipse/EclipseState/Schedule/WellProductionProperties.hpp>

#include <boost/optional.hpp>

#include <memory>
#include <string>
#include <limits>

namespace Opm {

    typedef struct WellInjectionProperties {
        double  surfaceInjectionRate;
        double  reservoirInjectionRate;
        double  BHPLimit;
        double  THPLimit;
        bool    predictionMode;
        int     injectionControls;
        WellInjector::TypeEnum injectorType;
        WellInjector::ControlModeEnum controlMode;

        WellInjectionProperties() {
            surfaceInjectionRate=0.0; 
            reservoirInjectionRate=0.0; 
            BHPLimit=0.0; 
            THPLimit=0.0; 
            predictionMode=true;
            injectionControls=0;
            injectorType = WellInjector::WATER;
            controlMode = WellInjector::RATE;
        }

        bool hasInjectionControl(WellInjector::ControlModeEnum controlModeArg) const {
            if (injectionControls & controlModeArg)
                return true;
            else
                return false;
        }

        void dropInjectionControl(WellInjector::ControlModeEnum controlModeArg) {
            if ((injectionControls & controlModeArg) != 0) {
                injectionControls -= controlModeArg;
            }
        }

        void addInjectionControl(WellInjector::ControlModeEnum controlModeArg) {
            if ((injectionControls & controlModeArg) == 0) {
                injectionControls += controlModeArg;
            }
        }
    } WellInjectionProperties;


    class Well {
    public:
        Well(const std::string& name, int headI, int headJ,
             bool refDepthDefaulted, double refDepth,
             Phase::PhaseEnum preferredPhase,
             TimeMapConstPtr timeMap, size_t creationTimeStep);
        const std::string& name() const;

        bool hasBeenDefined(size_t timeStep) const;
        const std::string getGroupName(size_t timeStep) const;
        void setGroupName(size_t timeStep , const std::string& groupName);

        WellCommon::StatusEnum getStatus(size_t timeStep) const;
        void                   setStatus(size_t timeStep, WellCommon::StatusEnum Status);

        int    getHeadI() const;
        int    getHeadJ() const;
        bool   getRefDepthDefaulted() const;
        double getRefDepth() const;
        Phase::PhaseEnum getPreferredPhase() const;
        
        bool isAvailableForGroupControl(size_t timeStep) const;
        void setAvailableForGroupControl(size_t timeStep, bool isAvailableForGroupControl);
        double getGuideRate(size_t timeStep) const;
        void setGuideRate(size_t timeStep, double guideRate);
        GuideRate::GuideRatePhaseEnum getGuideRatePhase(size_t timeStep) const;
        void setGuideRatePhase(size_t timeStep, GuideRate::GuideRatePhaseEnum phase);
        double getGuideRateScalingFactor(size_t timeStep) const;
        void setGuideRateScalingFactor(size_t timeStep, double scalingFactor);

        bool isProducer(size_t timeStep) const;
        bool isInjector(size_t timeStep) const;
        void addWELSPECS(DeckRecordConstPtr deckRecord);
        void addCompletions(size_t time_step , const std::vector<CompletionPtr>& newCompletions);
        CompletionSetConstPtr getCompletions(size_t timeStep) const;
    
        void                            setProductionProperties(size_t timeStep , const WellProductionProperties properties);
        WellProductionProperties        getProductionPropertiesCopy(size_t timeStep) const;
        const WellProductionProperties& getProductionProperties(size_t timeStep)  const;
        
        void                           setInjectionProperties(size_t timeStep , const WellInjectionProperties properties);
        WellInjectionProperties        getInjectionPropertiesCopy(size_t timeStep) const;
        const WellInjectionProperties& getInjectionProperties(size_t timeStep) const;

    private:
        size_t m_creationTimeStep;
        std::string m_name;
        
        std::shared_ptr<DynamicState<WellCommon::StatusEnum> > m_status;

        std::shared_ptr<DynamicState<bool> > m_isAvailableForGroupControl;
        std::shared_ptr<DynamicState<double> > m_guideRate;
        std::shared_ptr<DynamicState<GuideRate::GuideRatePhaseEnum> > m_guideRatePhase;
        std::shared_ptr<DynamicState<double> > m_guideRateScalingFactor;

        std::shared_ptr<DynamicState<bool> > m_isProducer;
        std::shared_ptr<DynamicState<CompletionSetConstPtr> > m_completions;
        std::shared_ptr<DynamicState<WellProductionProperties> > m_productionProperties;
        std::shared_ptr<DynamicState<WellInjectionProperties> > m_injectionProperties;
        std::shared_ptr<DynamicState<std::string> > m_groupName;

        // WELSPECS data - assumes this is not dynamic
        int m_headI;
        int m_headJ;
        bool m_refDepthDefaulted;
        double m_refDepth;
        Phase::PhaseEnum m_preferredPhase;
    };
    typedef std::shared_ptr<Well> WellPtr;
    typedef std::shared_ptr<const Well> WellConstPtr;
}



#endif /* WELL_HPP_ */
