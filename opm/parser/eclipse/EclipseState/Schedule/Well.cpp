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
        : m_oilRate( new DynamicState<double>( timeMap , 0.0)) ,
          m_gasRate(new DynamicState<double>(timeMap, 0.0)),
          m_waterRate(new DynamicState<double>(timeMap, 0.0)),
          m_surfaceInjectionRate(new DynamicState<double>(timeMap, 0.0)),
          m_reservoirInjectionRate(new DynamicState<double>(timeMap, 0.0)),
          m_BHPLimit(new DynamicState<double>(timeMap , 0.0)),
          m_inPredictionMode(new DynamicState<bool>(timeMap, true)),
          m_isProducer(new DynamicState<bool>(timeMap, true)) ,
          m_completions( new DynamicState<CompletionSetConstPtr>( timeMap , CompletionSetConstPtr( new CompletionSet()) )),
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


    bool Well::hasBeenDefined(size_t timeStep) const {
        if (timeStep < m_creationTimeStep)
            return false;
        else
            return true;
    }


    double Well::getBHPLimit(size_t timeStep) const {
        return m_BHPLimit->get(timeStep);
    }

    void Well::setBHPLimit(size_t timeStep, double BHPLimit) {
        m_BHPLimit->add(timeStep, BHPLimit);
    }


    double Well::getOilRate(size_t timeStep) const {
        return m_oilRate->get(timeStep);
    }

    void Well::setOilRate(size_t timeStep, double oilRate) {
        m_oilRate->add(timeStep, oilRate);
        switch2Producer( timeStep );
    }


    double Well::getGasRate(size_t timeStep) const {
        return m_gasRate->get(timeStep);
    }

    void Well::setGasRate(size_t timeStep, double gasRate) {
        m_gasRate->add(timeStep, gasRate);
        switch2Producer( timeStep );
    }

    double Well::getWaterRate(size_t timeStep) const {
        return m_waterRate->get(timeStep);
    }

    void Well::setWaterRate(size_t timeStep, double waterRate) {
        m_waterRate->add(timeStep, waterRate);
        switch2Producer( timeStep );
    }

    double Well::getSurfaceInjectionRate(size_t timeStep) const {
        return m_surfaceInjectionRate->get(timeStep);
    }

    void Well::setSurfaceInjectionRate(size_t timeStep, double injectionRate) {
        m_surfaceInjectionRate->add(timeStep, injectionRate);
        switch2Injector( timeStep );
    }

    double Well::getReservoirInjectionRate(size_t timeStep) const {
        return m_reservoirInjectionRate->get(timeStep);
    }

    void Well::setReservoirInjectionRate(size_t timeStep, double injectionRate) {
        m_reservoirInjectionRate->add(timeStep, injectionRate);
        switch2Injector( timeStep );
    }


    bool Well::isProducer(size_t timeStep) const {
        return m_isProducer->get(timeStep);
    }

    bool Well::isInjector(size_t timeStep) const {
        return !isProducer(timeStep);
    }
    
    void Well::switch2Producer(size_t timeStep ) {
        m_isProducer->add(timeStep , true);
        m_surfaceInjectionRate->add(timeStep, 0);
        m_reservoirInjectionRate->add(timeStep , 0);
    }

    void Well::switch2Injector(size_t timeStep ) {
        m_isProducer->add(timeStep , false);
        m_oilRate->add(timeStep, 0);
        m_gasRate->add(timeStep, 0);
        m_waterRate->add(timeStep, 0);
    }

    bool Well::isInPredictionMode(size_t timeStep) const {
        return m_inPredictionMode->get(timeStep);
    }
    
    void Well::setInPredictionMode(size_t timeStep, bool inPredictionMode) {
        m_inPredictionMode->add(timeStep, inPredictionMode);
    }

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



