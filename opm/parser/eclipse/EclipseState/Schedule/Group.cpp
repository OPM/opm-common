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
#include <opm/parser/eclipse/EclipseState/Schedule/Group.hpp>

namespace Opm {

    Group::Group(const std::string& name , TimeMapConstPtr timeMap) 
        : m_injectionPhase( new DynamicState<PhaseEnum>( timeMap , WATER )),
          m_injectionControlMode( new DynamicState<GroupInjectionControlEnum>( timeMap , NONE )),
          m_injectionRate( new DynamicState<double>( timeMap , 0 )),
          m_surfaceFlowMaxRate( new DynamicState<double>( timeMap , 0)),
          m_reservoirFlowMaxRate( new DynamicState<double>( timeMap , 0)),
          m_targetReinjectFraction( new DynamicState<double>( timeMap , 0)),
          m_targetVoidReplacementFraction( new DynamicState<double>( timeMap , 0))
    {
        m_name = name;
    }


    const std::string& Group::name() const {
        return m_name;
    }

    
    void Group::setInjectionPhase(size_t time_step , PhaseEnum phase){
        if (m_injectionPhase->size() == time_step + 1) {
            PhaseEnum currentPhase = m_injectionPhase->get(time_step);
            /*
              The ECLIPSE documentation of the GCONINJE keyword seems
              to indicate that a group can inject more than one phase
              simultaneously. This should be implemented in the input
              file as:

              GCONINJE
                 'GROUP'   'PHASE1'    'RATE'   ... /
                 'GROUP'   'PHASE2'    'RATE'   ... /
                 ...
              /

              I.e. the same group occurs more than once at the same
              time step, with different phases. This seems quite
              weird, and we do currently not support it. Changing the
              injected phase from one time step to the next is
              supported.
            */
            if (phase != currentPhase)
                throw std::invalid_argument("Sorry - we currently do not support injecting multiple phases at the same time.");
        }
        m_injectionPhase->add( time_step , phase );
    }

    PhaseEnum Group::getInjectionPhase( size_t time_step ) const {
        return m_injectionPhase->get( time_step );
    }

    void Group::setInjectionRate( size_t time_step , double rate) {
        return m_injectionRate->add( time_step , rate);
    }

    double Group::getInjectionRate( size_t time_step ) const {
        return m_injectionRate->get( time_step );
    }

    void Group::setInjectionControlMode(size_t time_step , GroupInjectionControlEnum controlMode) {
        m_injectionControlMode->add( time_step , controlMode );
    }

    GroupInjectionControlEnum Group::getInjectionControlMode( size_t time_step) const {
        return m_injectionControlMode->get( time_step );
    }

    void Group::setSurfaceMaxRate( size_t time_step , double rate) {
        return m_surfaceFlowMaxRate->add( time_step , rate);
    }

    double Group::getSurfaceMaxRate( size_t time_step ) const {
        return m_surfaceFlowMaxRate->get( time_step );
    }

    void Group::setReservoirMaxRate( size_t time_step , double rate) {
        return m_reservoirFlowMaxRate->add( time_step , rate);
    }

    double Group::getReservoirMaxRate( size_t time_step ) const {
        return m_reservoirFlowMaxRate->get( time_step );
    }

    void Group::setTargetReinjectFraction( size_t time_step , double rate) {
        return m_targetReinjectFraction->add( time_step , rate);
    }

    double Group::getTargetReinjectFraction( size_t time_step ) const {
        return m_targetReinjectFraction->get( time_step );
    }

    void Group::setTargetVoidReplacementFraction( size_t time_step , double rate) {
        return m_targetVoidReplacementFraction->add( time_step , rate);
    }

    double Group::getTargetVoidReplacementFraction( size_t time_step ) const {
        return m_targetVoidReplacementFraction->get( time_step );
    }

    

}



