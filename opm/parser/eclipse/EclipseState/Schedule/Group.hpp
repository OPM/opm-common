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


#ifndef GROUP_HPP_
#define GROUP_HPP_

#include <opm/parser/eclipse/EclipseState/Schedule/TimeMap.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>

#include <boost/shared_ptr.hpp>
#include <string>

namespace Opm {

    class Group {
    public:
        Group(const std::string& name, TimeMapConstPtr timeMap);
        
        const std::string& name() const;
        void      setInjectionPhase(size_t time_step , PhaseEnum phase);
        PhaseEnum getInjectionPhase( size_t time_step) const;

        void                      setInjectionControlMode(size_t time_step , GroupInjectionControlEnum ControlMode);
        GroupInjectionControlEnum getInjectionControlMode( size_t time_step) const;

        void   setInjectionRate(size_t time_step , double rate);
        double getInjectionRate( size_t time_step) const;

        void   setSurfaceMaxRate( size_t time_step , double rate);
        double getSurfaceMaxRate( size_t time_step ) const;

        void   setReservoirMaxRate( size_t time_step , double rate);
        double getReservoirMaxRate( size_t time_step ) const;

        void   setTargetReinjectFraction( size_t time_step , double rate);
        double getTargetReinjectFraction( size_t time_step ) const;

        void   setTargetVoidReplacementFraction( size_t time_step , double rate);
        double getTargetVoidReplacementFraction( size_t time_step ) const;

    private:
        std::string m_name;
        boost::shared_ptr<DynamicState<PhaseEnum> > m_injectionPhase;
        boost::shared_ptr<DynamicState<GroupInjectionControlEnum> > m_injectionControlMode;
        boost::shared_ptr<DynamicState<double> > m_injectionRate;
        boost::shared_ptr<DynamicState<double> > m_surfaceFlowMaxRate;
        boost::shared_ptr<DynamicState<double> > m_reservoirFlowMaxRate;
        boost::shared_ptr<DynamicState<double> > m_targetReinjectFraction;
        boost::shared_ptr<DynamicState<double> > m_targetVoidReplacementFraction;
    };
    typedef boost::shared_ptr<Group> GroupPtr;
    typedef boost::shared_ptr<const Group> GroupConstPtr;
}



#endif /* WELL_HPP_ */
