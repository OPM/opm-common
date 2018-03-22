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

#include <memory>
#include <string>
#include <vector>

#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Events.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellEconProductionLimits.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellInjectionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellPolymerProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellProductionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/SegmentSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>

namespace Opm {

    template< typename > class DynamicState;

    class Completion;
    class CompletionSet;
    class Segment;
    class SegmentSet;
    class TimeMap;
    class EclipseGrid;

    class Well {
    public:
        Well(const std::string& name, int headI,
             int headJ, double refDepth, Phase preferredPhase,
             const TimeMap& timeMap, size_t creationTimeStep,
             WellCompletion::CompletionOrderEnum completionOrdering = WellCompletion::TRACK,
             bool allowCrossFlow = true, bool automaticShutIn = true);
        const std::string& name() const;

        bool hasBeenDefined(size_t timeStep) const;
        const std::string getGroupName(size_t timeStep) const;
        void setGroupName(size_t timeStep , const std::string& groupName);

        WellCommon::StatusEnum getStatus(size_t timeStep) const;
        bool                   setStatus(size_t timeStep, WellCommon::StatusEnum Status);

        int getHeadI() const;
        int getHeadJ() const;
        int getHeadI( size_t timestep ) const;
        int getHeadJ( size_t timestep ) const;
        void setHeadI( size_t timestep, int I );
        void setHeadJ( size_t timestep, int J );

        double getRefDepth() const;
        double getRefDepth( size_t timestep ) const;
        void setRefDepth( size_t timestep, double );
        Phase getPreferredPhase() const;

        bool isAvailableForGroupControl(size_t timeStep) const;
        void setAvailableForGroupControl(size_t timeStep, bool isAvailableForGroupControl);
        double getGuideRate(size_t timeStep) const;
        void setGuideRate(size_t timeStep, double guideRate);
        GuideRate::GuideRatePhaseEnum getGuideRatePhase(size_t timeStep) const;
        void setGuideRatePhase(size_t timeStep, GuideRate::GuideRatePhaseEnum phase);
        double getGuideRateScalingFactor(size_t timeStep) const;
        void setGuideRateScalingFactor(size_t timeStep, double scalingFactor);

        void setEfficiencyFactor (size_t timestep, double efficiencyFactor);
        double getEfficiencyFactor (size_t timestep) const;

        void switchToInjector( size_t timeStep);
        void switchToProducer( size_t timeStep);

        bool isProducer(size_t timeStep) const;
        bool isInjector(size_t timeStep) const;
        void addWELSPECS(const DeckRecord& deckRecord);

        void addCompletions(size_t time_step, const std::vector< Completion >& );
        void addCompletionSet(size_t time_step, CompletionSet );
        const CompletionSet& getCompletions(size_t timeStep) const;
        const CompletionSet& getCompletions() const;

        /* The rate of a given phase under the following assumptions:
         * * Returns zero if production is requested for an injector (and vice
         *   versa)
         * * If this is an injector and something else than the
         *   requested phase is injected, returns 0, i.e.
         *   water_injector.injection_rate( gas ) == 0
         * * Mixed injection is not supported and always returns 0.
         */
        double production_rate( Phase phase, size_t timestep ) const;
        double injection_rate( Phase phase, size_t timestep ) const;

        bool operator==(const Well&) const;
        bool operator!=(const Well&) const;

        bool                            setProductionProperties(size_t timeStep , const WellProductionProperties& properties);
        WellProductionProperties        getProductionPropertiesCopy(size_t timeStep) const;
        const WellProductionProperties& getProductionProperties(size_t timeStep)  const;

        bool                           setInjectionProperties(size_t timeStep , const WellInjectionProperties& properties);
        WellInjectionProperties        getInjectionPropertiesCopy(size_t timeStep) const;
        const WellInjectionProperties& getInjectionProperties(size_t timeStep) const;

        bool                           setPolymerProperties(size_t timeStep , const WellPolymerProperties& properties);
        WellPolymerProperties          getPolymerPropertiesCopy(size_t timeStep) const;
        const WellPolymerProperties&   getPolymerProperties(size_t timeStep) const;

        bool                           setSolventFraction(size_t timeStep , const double fraction);
        const double&                  getSolventFraction(size_t timeStep) const;

        bool                            setEconProductionLimits(const size_t timeStep, const WellEconProductionLimits& productionlimits);
        const WellEconProductionLimits& getEconProductionLimits(const size_t timeStep) const;

        int  firstRFTOutput( ) const;
        bool getRFTActive(size_t time_step) const;
        void updateRFTActive(size_t time_step, RFTConnections::RFTEnum mode);
        bool getPLTActive(size_t time_step) const;
        void updatePLTActive(size_t time_step, PLTConnections::PLTEnum mode);
        int  findWellFirstOpen(int startTimeStep) const;

        /*
          Will return the report step when the well is created with
          WELSPECS, actually opening the well might be later.
        */
        size_t firstTimeStep( ) const;
        void setRFTForWellWhenFirstOpen(size_t currentStep);

        static bool wellNameInWellNamePattern(const std::string& wellName, const std::string& wellNamePattern);

        WellCompletion::CompletionOrderEnum getWellCompletionOrdering() const;

        bool getAllowCrossFlow() const;
        bool getAutomaticShutIn() const;
        bool canOpen(size_t time_step) const;


        // for multi-segment wells
        bool isMultiSegment(size_t time_step) const;
        const SegmentSet& getSegmentSet(size_t time_step) const;

        void addSegmentSet(size_t time_step, SegmentSet new_segmentset);

        const Events& getEvents() const;
        void addEvent(ScheduleEvents::Events event, size_t reportStep);
        bool hasEvent(uint64_t eventMask, size_t reportStep) const;

        /*
          Will remove all completions which are attached to inactive cells. Will
          scan through all timesteps.
        */
        void filterCompletions(const EclipseGrid& grid);
    private:
        size_t m_creationTimeStep;
        std::string m_name;

        DynamicState< WellCommon::StatusEnum > m_status;

        DynamicState< int > m_isAvailableForGroupControl;
        DynamicState< double > m_guideRate;
        DynamicState< GuideRate::GuideRatePhaseEnum > m_guideRatePhase;
        DynamicState< double > m_guideRateScalingFactor;
        DynamicState< double > m_efficiencyFactors;

        DynamicState< int > m_isProducer;
        DynamicState< CompletionSet > m_completions;
        DynamicState< WellProductionProperties > m_productionProperties;
        DynamicState< WellInjectionProperties > m_injectionProperties;
        DynamicState< WellPolymerProperties > m_polymerProperties;
        DynamicState< WellEconProductionLimits > m_econproductionlimits;
        DynamicState< double > m_solventFraction;
        DynamicState< std::string > m_groupName;
        DynamicState< int > m_rft;
        DynamicState< int > m_plt;

        DynamicState< int > m_headI;
        DynamicState< int > m_headJ;
        DynamicState< double > m_refDepth;
        Phase m_preferredPhase;

        WellCompletion::CompletionOrderEnum m_comporder;
        bool m_allowCrossFlow;
        bool m_automaticShutIn;
        // WELSEGS DATA - for mutli-segment wells
        // flag indicating if the well is a multi-segment well
        DynamicState< SegmentSet > m_segmentset;
        size_t timesteps;
        Events events;
    };
}



#endif /* WELL_HPP_ */
