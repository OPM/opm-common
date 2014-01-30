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

#include <memory>
#include <string>

namespace Opm {

    class Well {
    public:
        Well(const std::string& name, int headI, int headJ, double refDepth, TimeMapConstPtr timeMap , size_t creationTimeStep);
        const std::string& name() const;

        bool hasBeenDefined(size_t timeStep) const;
        const std::string getGroupName(size_t timeStep) const;
        void setGroupName(size_t timeStep , const std::string& groupName);

        double getOilRate(size_t timeStep) const;
        void   setOilRate(size_t timeStep, double oilRate);
        double getLiquidRate(size_t timeStep) const;
        void   setLiquidRate(size_t timeStep, double LiquidRate);
        double getResVRate(size_t timeStep) const;
        void   setResVRate(size_t timeStep, double resVRate);
        double getGasRate(size_t timeStep) const;
        void   setGasRate(size_t timeStep, double gasRate);
        double getWaterRate(size_t timeStep) const;
        void   setWaterRate(size_t timeStep, double waterRate);
        double getSurfaceInjectionRate(size_t timeStep) const;
        void   setSurfaceInjectionRate(size_t timeStep, double injectionRate);
        double getReservoirInjectionRate(size_t timeStep) const;
        void   setReservoirInjectionRate(size_t timeStep, double injectionRate);
        double getBHPLimit(size_t timeStep) const;
        void   setBHPLimit(size_t timeStep, double BHPLimit);
        double getTHPLimit(size_t timeStep) const;
        void   setTHPLimit(size_t timeStep, double THPLimit);
        WellInjector::TypeEnum getInjectorType(size_t timeStep) const;
        void   setInjectorType(size_t timeStep, WellInjector::TypeEnum injectorType);
        WellInjector::ControlModeEnum getInjectorControlMode(size_t timeStep) const;
        void   setInjectorControlMode(size_t timeStep, WellInjector::ControlModeEnum injectorControlMode);
        WellCommon::StatusEnum getStatus(size_t timeStep) const;
        void   setStatus(size_t timeStep, WellCommon::StatusEnum Status);
        

        int    getHeadI() const;
        int    getHeadJ() const;
        double getRefDepth() const;
        
        bool isInPredictionMode(size_t timeStep) const;
        void setInPredictionMode(size_t timeStep, bool isInPredictionMode);
        bool isProducer(size_t timeStep) const;
        bool isInjector(size_t timeStep) const;
        void addWELSPECS(DeckRecordConstPtr deckRecord);
        void addCompletions(size_t time_step , const std::vector<CompletionConstPtr>& newCompletions);
        CompletionSetConstPtr getCompletions(size_t timeStep) const;

    private:
        void switch2Producer(size_t timeStep );
        void switch2Injector(size_t timeStep );
        
        size_t m_creationTimeStep;
        std::string m_name;
        std::shared_ptr<DynamicState<double> > m_oilRate;
        std::shared_ptr<DynamicState<double> > m_gasRate;
        std::shared_ptr<DynamicState<double> > m_waterRate;
        std::shared_ptr<DynamicState<double> > m_liquidRate;
        std::shared_ptr<DynamicState<double> > m_resVRate;
        std::shared_ptr<DynamicState<double> > m_surfaceInjectionRate;
        std::shared_ptr<DynamicState<double> > m_reservoirInjectionRate;
        std::shared_ptr<DynamicState<double> > m_BHPLimit;
        std::shared_ptr<DynamicState<double> > m_THPLimit;
        std::shared_ptr<DynamicState<WellInjector::TypeEnum> > m_injectorType;
        std::shared_ptr<DynamicState<WellInjector::ControlModeEnum> > m_injectorControlMode;
        std::shared_ptr<DynamicState<WellCommon::StatusEnum> > m_status;
        
        std::shared_ptr<DynamicState<bool> > m_inPredictionMode;
        std::shared_ptr<DynamicState<bool> > m_isProducer;
        std::shared_ptr<DynamicState<CompletionSetConstPtr> > m_completions;
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
