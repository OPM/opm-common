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



#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>

#define INVALID_GROUP_RATE -999e100
#define INVALID_EFFICIENCY_FACTOR 0.0


namespace Opm {
    GroupProduction::ProductionData::ProductionData(const TimeMap& timeMap) :
        controlMode( timeMap , GroupProduction::NONE ),
        exceedAction( timeMap , GroupProductionExceedLimit::NONE ),
        oilTarget( timeMap , INVALID_GROUP_RATE),
        waterTarget( timeMap , INVALID_GROUP_RATE ),
        gasTarget( timeMap , INVALID_GROUP_RATE ),
        liquidTarget( timeMap , INVALID_GROUP_RATE ),
        reservoirVolumeTarget( timeMap , INVALID_GROUP_RATE )
    {}

    GroupInjection::InjectionData::InjectionData(const TimeMap& timeMap) :
        phase( timeMap, Phase::WATER  ),
        controlMode( timeMap, NONE  ),
        rate( timeMap, 0  ),
        surfaceFlowMaxRate( timeMap, 0 ),
        reservoirFlowMaxRate( timeMap, 0 ),
        targetReinjectFraction( timeMap, 0 ),
        targetVoidReplacementFraction( timeMap, 0 )
    {}

    /*****************************************************************/

    Group::Group(const std::string& name_, const size_t& seqIndex_, const TimeMap& timeMap , size_t creationTimeStep) :
        m_creationTimeStep( creationTimeStep ),
        m_name( name_ ),
        m_seqIndex( seqIndex_),
        m_injection( timeMap ),
        m_production( timeMap ),
        m_wells( timeMap, {} ),
        m_isProductionGroup( timeMap, false),
        m_isInjectionGroup( timeMap, false),
        m_efficiencyFactor( timeMap, 1.0),
        m_transferEfficiencyFactor( timeMap, 1),
        m_groupNetVFPTable( timeMap, 0 )
    {}


    const std::string& Group::name() const {
        return m_name;
    }

    const size_t& Group::seqIndex() const {
        return m_seqIndex;
    }

    bool Group::hasBeenDefined(size_t timeStep) const {
        if (timeStep < m_creationTimeStep)
            return false;
        else
            return true;
    }

    bool Group::isProductionGroup(size_t timeStep) const {
        return bool( m_isProductionGroup.get(timeStep) );
    }

    bool Group::isInjectionGroup(size_t timeStep) const {
        return bool( m_isInjectionGroup.get(timeStep) );
    }

    void Group::setProductionGroup(size_t timeStep, bool isProductionGroup_) {
        m_isProductionGroup.update(timeStep, isProductionGroup_);
    }

    void Group::setInjectionGroup(size_t timeStep, bool isInjectionGroup_) {
        m_isInjectionGroup.update(timeStep, isInjectionGroup_);
    }


    /**********************************************************************/


    void Group::setInjectionPhase(size_t time_step, Phase phase){
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
           weird, and we do currently not support it - we only set the latest
           specified phase. Changing the injected phase from one time step to
           the next is supported.
           */
        m_injection.phase.update( time_step , phase );
    }

    Phase Group::getInjectionPhase( size_t time_step ) const {
        return m_injection.phase.get( time_step );
    }

    void Group::setInjectionRate( size_t time_step , double rate) {
        m_injection.rate.update( time_step , rate);
    }

    double Group::getInjectionRate( size_t time_step ) const {
        return m_injection.rate.get( time_step );
    }

    void Group::setInjectionControlMode(size_t time_step , GroupInjection::ControlEnum controlMode) {
        m_injection.controlMode.update( time_step , controlMode );
    }

    GroupInjection::ControlEnum Group::getInjectionControlMode( size_t time_step) const {
        return m_injection.controlMode.get( time_step );
    }

    void Group::setSurfaceMaxRate( size_t time_step , double rate) {
        m_injection.surfaceFlowMaxRate.update( time_step , rate);
    }

    double Group::getSurfaceMaxRate( size_t time_step ) const {
        return m_injection.surfaceFlowMaxRate.get( time_step );
    }

    void Group::setReservoirMaxRate( size_t time_step , double rate) {
        m_injection.reservoirFlowMaxRate.update( time_step , rate);
    }

    double Group::getReservoirMaxRate( size_t time_step ) const {
        return m_injection.reservoirFlowMaxRate.get( time_step );
    }

    void Group::setTargetReinjectFraction( size_t time_step , double rate) {
        m_injection.targetReinjectFraction.update( time_step , rate);
    }

    double Group::getTargetReinjectFraction( size_t time_step ) const {
        return m_injection.targetReinjectFraction.get( time_step );
    }

    void Group::setTargetVoidReplacementFraction( size_t time_step , double rate) {
        m_injection.targetVoidReplacementFraction.update( time_step , rate);
    }

    double Group::getTargetVoidReplacementFraction( size_t time_step ) const {
        return m_injection.targetVoidReplacementFraction.get( time_step );
    }


    /*****************************************************************/

    void Group::setProductionControlMode( size_t time_step , GroupProduction::ControlEnum controlMode) {
        m_production.controlMode.update(time_step , controlMode );
    }

    GroupProduction::ControlEnum Group::getProductionControlMode( size_t time_step ) const {
        return m_production.controlMode.get(time_step);
    }


    GroupProductionExceedLimit::ActionEnum Group::getProductionExceedLimitAction( size_t time_step ) const  {
        return m_production.exceedAction.get(time_step);
    }


    void Group::setProductionExceedLimitAction( size_t time_step , GroupProductionExceedLimit::ActionEnum action) {
        m_production.exceedAction.update(time_step , action);
    }


    void Group::setOilTargetRate(size_t time_step , double oilTargetRate) {
        m_production.oilTarget.update(time_step , oilTargetRate);
    }


    double Group::getOilTargetRate(size_t time_step) const {
        return m_production.oilTarget.get(time_step);
    }


    void Group::setGasTargetRate(size_t time_step , double gasTargetRate) {
        m_production.gasTarget.update(time_step , gasTargetRate);
    }


    double Group::getGasTargetRate(size_t time_step) const {
        return m_production.gasTarget.get(time_step);
    }


    void Group::setWaterTargetRate(size_t time_step , double waterTargetRate) {
        m_production.waterTarget.update(time_step , waterTargetRate);
    }


    double Group::getWaterTargetRate(size_t time_step) const {
        return m_production.waterTarget.get(time_step);
    }


    void Group::setLiquidTargetRate(size_t time_step , double liquidTargetRate) {
        m_production.liquidTarget.update(time_step , liquidTargetRate);
    }


    double Group::getLiquidTargetRate(size_t time_step) const {
        return m_production.liquidTarget.get(time_step);
    }


    void Group::setReservoirVolumeTargetRate(size_t time_step , double reservoirVolumeTargetRate) {
        m_production.reservoirVolumeTarget.update(time_step , reservoirVolumeTargetRate);
    }


    double Group::getReservoirVolumeTargetRate(size_t time_step) const {
        return m_production.reservoirVolumeTarget.get(time_step);
    }


    void Group::setGroupEfficiencyFactor(size_t time_step, double factor) {
        m_efficiencyFactor.update(time_step , factor);
    }

    double Group::getGroupEfficiencyFactor(size_t time_step) const {
        return m_efficiencyFactor.get(time_step);
    }

    void  Group::setTransferGroupEfficiencyFactor(size_t time_step, bool transfer) {
        m_transferEfficiencyFactor.update(time_step , transfer);
    }


    bool  Group::getTransferGroupEfficiencyFactor(size_t time_step) const {
        return m_transferEfficiencyFactor.get(time_step);
    }

    void   Group::setGroupNetVFPTable(size_t time_step, int table) {
        m_groupNetVFPTable.update(time_step, table);
    }

    int  Group::getGroupNetVFPTable(size_t time_step) const {
        return m_groupNetVFPTable.get(time_step);
    }

    bool Group::groupNameInGroupNamePattern(const std::string& groupName, const std::string& groupNamePattern) {
        if (util_fnmatch( groupNamePattern.c_str() , groupName.c_str()) == 0)
            return true;
        return false;
    }
    /*****************************************************************/

    bool Group::hasWell(const std::string& wellName , size_t time_step) const {
        return this->m_wells.at( time_step ).find( wellName ) !=
               this->m_wells.at( time_step ).end();
    }

    const std::set< std::string >& Group::getWells( size_t time_step ) const {
        return this->m_wells.at( time_step );
    }

    size_t Group::numWells(size_t time_step) const {
        return this->m_wells.at( time_step ).size();
    }

    void Group::addWell(size_t time_step, const Well* well ) {
        auto new_set = this->m_wells.at( time_step );
        new_set.insert( well->name() );
        this->m_wells.update( time_step, new_set );
    }

    void Group::delWell(size_t time_step, const std::string& wellName) {
        auto itr = this->m_wells.at( time_step ).find( wellName );

        if( itr == this->m_wells.at( time_step ).end() ) return;

        auto new_set = this->m_wells.at( time_step );
        new_set.erase( wellName );
        this->m_wells.update( time_step, new_set );
    }

}
