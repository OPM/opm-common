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

#include <opm/parser/eclipse/EclipseState/Util/Value.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Completion.hpp>



namespace Opm {

    Well::Well(const std::string& name_, std::shared_ptr<const EclipseGrid> grid , int headI, int headJ, Value<double> refDepth , Phase::PhaseEnum preferredPhase,
               TimeMapConstPtr timeMap, size_t creationTimeStep, WellCompletion::CompletionOrderEnum completionOrdering, bool allowCrossFlow)
        : m_status(new DynamicState<WellCommon::StatusEnum>(timeMap, WellCommon::SHUT)),
          m_isAvailableForGroupControl(new DynamicState<bool>(timeMap, true)),
          m_guideRate(new DynamicState<double>(timeMap, -1.0)),
          m_guideRatePhase(new DynamicState<GuideRate::GuideRatePhaseEnum>(timeMap, GuideRate::UNDEFINED)),
          m_guideRateScalingFactor(new DynamicState<double>(timeMap, 1.0)),
          m_isProducer(new DynamicState<bool>(timeMap, true)) ,
          m_completions( new DynamicState<CompletionSetConstPtr>( timeMap , CompletionSetConstPtr( new CompletionSet()) )),
          m_productionProperties( new DynamicState<WellProductionProperties>(timeMap, WellProductionProperties() )),
          m_injectionProperties( new DynamicState<WellInjectionProperties>(timeMap, WellInjectionProperties() )),
          m_polymerProperties( new DynamicState<WellPolymerProperties>(timeMap, WellPolymerProperties() )),
          m_solventFraction( new DynamicState<double>(timeMap, 0.0 )),
          m_groupName( new DynamicState<std::string>( timeMap , "" )),
          m_rft( new DynamicState<bool>(timeMap,false)),
          m_plt( new DynamicState<bool>(timeMap,false)),
          m_timeMap( timeMap ),
          m_headI(headI),
          m_headJ(headJ),
          m_refDepth(refDepth),
          m_preferredPhase(preferredPhase),
          m_grid( grid ),
          m_comporder(completionOrdering),
          m_allowCrossFlow(allowCrossFlow),
          m_is_multi_segment(false),
          m_segmentset(new DynamicState<SegmentSetPtr>(timeMap, SegmentSetPtr(new SegmentSet())))
    {
        m_name = name_;
        m_creationTimeStep = creationTimeStep;


    }

    const std::string& Well::name() const {
        return m_name;
    }


    void Well::switchToProducer( size_t timeStep) {
        WellInjectionProperties p = getInjectionPropertiesCopy(timeStep);

        p.BHPLimit = 0;
        p.dropInjectionControl( Opm::WellInjector::BHP );
        setInjectionProperties( timeStep , p );
    }


    void Well::switchToInjector( size_t timeStep) {
        WellProductionProperties p = getProductionPropertiesCopy(timeStep);

        p.BHPLimit = 0;
        p.dropProductionControl( Opm::WellProducer::BHP );
        setProductionProperties( timeStep , p );
    }



    bool Well::setProductionProperties(size_t timeStep , const WellProductionProperties newProperties) {
        if (isInjector(timeStep))
            switchToProducer( timeStep );

        m_isProducer->update(timeStep , true);
        return m_productionProperties->update(timeStep, newProperties);
    }

    WellProductionProperties Well::getProductionPropertiesCopy(size_t timeStep) const {
        return m_productionProperties->get(timeStep);
    }

    const WellProductionProperties& Well::getProductionProperties(size_t timeStep) const {
        return m_productionProperties->at(timeStep);
    }

    bool Well::setInjectionProperties(size_t timeStep , const WellInjectionProperties newProperties) {
        if (isProducer(timeStep))
            switchToInjector( timeStep );

        m_isProducer->update(timeStep , false);
        return m_injectionProperties->update(timeStep, newProperties);
    }

    WellInjectionProperties Well::getInjectionPropertiesCopy(size_t timeStep) const {
        return m_injectionProperties->get(timeStep);
    }

    const WellInjectionProperties& Well::getInjectionProperties(size_t timeStep) const {
        return m_injectionProperties->at(timeStep);
    }

    bool Well::setPolymerProperties(size_t timeStep , const WellPolymerProperties newProperties) {
        m_isProducer->update(timeStep , false);
        return m_polymerProperties->update(timeStep, newProperties);
    }

    WellPolymerProperties Well::getPolymerPropertiesCopy(size_t timeStep) const {
        return m_polymerProperties->get(timeStep);
    }

    const WellPolymerProperties& Well::getPolymerProperties(size_t timeStep) const {
        return m_polymerProperties->at(timeStep);
    }

    bool Well::setSolventFraction(size_t timeStep , const double fraction) {
        m_isProducer->update(timeStep , false);
        return m_solventFraction->update(timeStep, fraction);
    }

    const double& Well::getSolventFraction(size_t timeStep) const {
        return m_solventFraction->at(timeStep);
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

    bool Well::setStatus(size_t timeStep, WellCommon::StatusEnum status) {
        if ((WellCommon::StatusEnum::OPEN == status) && getCompletions(timeStep)->allCompletionsShut()) {
            std::cerr << "ERROR when handling keyword for well "<< name()  << ": Cannot open a well where all completions are shut" << std::endl;
            return false;
        } else
            return m_status->update( timeStep , status );
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

    void Well::setAvailableForGroupControl(size_t timeStep, bool isAvailableForGroupControl_) {
        m_isAvailableForGroupControl->update(timeStep, isAvailableForGroupControl_);
    }

    double Well::getGuideRate(size_t timeStep) const {
        return m_guideRate->get(timeStep);
    }

    void Well::setGuideRate(size_t timeStep, double guideRate) {
        m_guideRate->update(timeStep, guideRate);
    }

    GuideRate::GuideRatePhaseEnum Well::getGuideRatePhase(size_t timeStep) const {
        return m_guideRatePhase->get(timeStep);
    }

    void Well::setGuideRatePhase(size_t timeStep, GuideRate::GuideRatePhaseEnum phase) {
        m_guideRatePhase->update(timeStep, phase);
    }

    double Well::getGuideRateScalingFactor(size_t timeStep) const {
        return m_guideRateScalingFactor->get(timeStep);
    }

    void Well::setGuideRateScalingFactor(size_t timeStep, double scalingFactor) {
        m_guideRateScalingFactor->update(timeStep, scalingFactor);
    }

    /*****************************************************************/

    // WELSPECS

    int Well::getHeadI() const {
        return m_headI;
    }

    int Well::getHeadJ() const {
        return m_headJ;
    }


    double Well::getRefDepth() const{
        if (!m_refDepth.hasValue())
            setRefDepthFromCompletions();

        return m_refDepth.getValue();
    }


    void Well::setRefDepthFromCompletions() const {
        size_t timeStep = m_creationTimeStep;
        while (true) {
            auto completions = getCompletions( timeStep );
            if (completions->size() > 0) {
                auto firstCompletion = completions->get(0);
                double depth = m_grid->getCellDepth( firstCompletion->getI() , firstCompletion->getJ() , firstCompletion->getK());
                m_refDepth.setValue( depth );
                break;
            } else {
                timeStep++;
                if (timeStep >= m_timeMap->size())
                    throw std::invalid_argument("No completions defined for well: " + name() + " can not infer reference depth");
            }
        }
    }


    Phase::PhaseEnum Well::getPreferredPhase() const {
        return m_preferredPhase;
    }

    CompletionSetConstPtr Well::getCompletions(size_t timeStep) const {
        return m_completions->get( timeStep );
    }

    void Well::addCompletions(size_t time_step , const std::vector<CompletionPtr>& newCompletions) {
        CompletionSetConstPtr currentCompletionSet = m_completions->get(time_step);
        CompletionSetPtr newCompletionSet = CompletionSetPtr( currentCompletionSet->shallowCopy() );

        for (size_t ic = 0; ic < newCompletions.size(); ic++) {
            newCompletions[ic]->fixDefaultIJ( m_headI , m_headJ );
            newCompletionSet->add( newCompletions[ic] );
        }

        addCompletionSet( time_step , newCompletionSet);
    }

    void Well::addCompletionSet(size_t time_step, const CompletionSetConstPtr newCompletionSet){
        CompletionSetPtr mutable_copy(newCompletionSet->shallowCopy());
        if (getWellCompletionOrdering() == WellCompletion::TRACK) {
            mutable_copy->orderCompletions(m_headI, m_headJ, m_grid);
        }
        m_completions->update(time_step, mutable_copy);
    }

    const std::string Well::getGroupName(size_t time_step) const {
        return m_groupName->get(time_step);
    }


    void Well::setGroupName(size_t time_step, const std::string& groupName ) {
        m_groupName->update(time_step , groupName);
    }



    void Well::setRFTActive(size_t time_step, bool value){
        m_rft->update(time_step, value);
    }

    bool Well::getRFTActive(size_t time_step) const{
        return m_rft->get(time_step);
    }

    bool Well::getPLTActive(size_t time_step) const{
     return m_plt->get(time_step);
    }
    void Well::setPLTActive(size_t time_step, bool value){
        m_plt->update(time_step, value);
    }

    int Well::findWellFirstOpen(int startTimeStep) const{
        int numberOfTimeSteps = m_timeMap->numTimesteps();
        for(int i = startTimeStep; i < numberOfTimeSteps;i++){
            if(getStatus(i)==WellCommon::StatusEnum::OPEN){
                return i;
            }
        }
        return -1;
    }

    void Well::setRFTForWellWhenFirstOpen(int numSteps,size_t currentStep){
        int time;
        if(getStatus(currentStep)==WellCommon::StatusEnum::OPEN ){
            time = currentStep;
        }else {
            time = findWellFirstOpen(currentStep);
        }
        if(time>-1){
            setRFTActive(time, true);
            if(time < numSteps){
                setRFTActive(time+1, false);
            }
        }
    }

    WellCompletion::CompletionOrderEnum Well::getWellCompletionOrdering() const {
        return m_comporder;
    }


    bool Well::wellNameInWellNamePattern(const std::string& wellName, const std::string& wellNamePattern) {
        bool wellNameInPattern = false;
        if (util_fnmatch( wellNamePattern.c_str() , wellName.c_str()) == 0) {
            wellNameInPattern = true;
        }
        return wellNameInPattern;
    }

    bool Well::getAllowCrossFlow() const {
        return m_allowCrossFlow;
    }

    bool Well::canOpen(size_t currentStep) const {
        bool canOpen = true;
        if (!getAllowCrossFlow()) {
            if ( isInjector(currentStep) ) {
                if (getInjectionProperties(currentStep).surfaceInjectionRate == 0) {;
                    canOpen = false;
                }
            } else {
                if ( (getProductionProperties(currentStep).WaterRate + getProductionProperties(currentStep).OilRate +
                      getProductionProperties(currentStep).GasRate) == 0) {
                    canOpen = false;
                }
            }
        }
        return canOpen;
    }

    bool Well::isMultiSegment() const {
        return m_is_multi_segment;
    }

    void Well::setMultiSegment(const bool is_multi_segment) {
        m_is_multi_segment = is_multi_segment;
    }

    SegmentSetConstPtr Well::getSegmentSet(size_t time_step) const {
        return m_segmentset->get(time_step);
    }

    void Well::addSegmentSet(size_t time_step, const SegmentSetPtr new_segmentset) {
        // to see if it is the first time entering WELSEGS input to this well.
        // if the well is not multi-segment well, it will be the first time
        // not sure if a well can switch between mutli-segment well and other
        // type of well
        // Here, we assume not
        const bool first_time = !isMultiSegment();

        if (first_time) {
            // overwrite the BHP reference depth with the one from WELSEGS keyword
            const double ref_depth = new_segmentset->depthTopSegment();
            m_refDepth.setValue(ref_depth);
            // set the well to be a multi-segment well
            m_is_multi_segment = true;
        } else {
            // checking the consistency of the input WELSEGS information
            throw std::logic_error("re-entering WELSEGS for a well is not supported yet!!.");
        }

        if (new_segmentset->lengthDepthType() == WellSegment::ABS) {
            addSegmentSetABS(time_step, new_segmentset, first_time);
        } else if (new_segmentset->lengthDepthType() == WellSegment::INC) {
            addSegmentSetINC(time_step, new_segmentset, first_time);
        } else {
            throw std::logic_error(" unknown length_depth_type in the new_segmentset");
        }
    }

    void Well::addSegmentSetABS(size_t time_step, const SegmentSetPtr new_segmentset, const bool first_time) {
        const double meaningless_value = -1.e100; // meaningless value to indicate unspecified values
        if (first_time) {
            // only need to update the volume and the length and depth values specified in the middle of the range
            // top segment is always ready
            // then looking for range that the information of the first segment whose outlet segment information are ready.
            bool all_ready;
            do {
                all_ready = true;
                for (int i = 1; i < new_segmentset->numberSegment(); ++i) {
                    if ((*new_segmentset)[i]->dataReady() == false) {
                        all_ready = false;
                        // then looking for unready segment with a ready outlet segment
                        // and looking for the continous unready segments
                        // int index_begin = i;
                        int location_begin = i;

                        int outlet_segment = (*new_segmentset)[i]->outletSegment();
                        int outlet_location = new_segmentset->numberToLocation(outlet_segment);

                        // assuming no loop
                        while ((*new_segmentset)[outlet_location]->dataReady() == false) {
                            location_begin = outlet_location;
                            assert(location_begin > 0);
                            outlet_segment = (*new_segmentset)[location_begin]->outletSegment();
                            outlet_location = new_segmentset->numberToLocation(outlet_segment);
                        }

                        // begin from location_begin to look for the unready segments continous
                        int location_end = -1;

                        for (int j = location_begin + 1; j < new_segmentset->numberSegment(); ++j) {
                            if ((*new_segmentset)[j]->dataReady() == true) {
                                location_end = j;
                                break;
                            }
                        }
                        if (location_end == -1) {
                            throw std::logic_error("One of the range records in WELSEGS is wrong.");
                        }

                        // set the value for the segments in the range
                        int number_segments = location_end - location_begin + 1;
                        assert(number_segments > 1);

                        double length_outlet = (*new_segmentset)[outlet_location]->length();
                        double depth_outlet = (*new_segmentset)[outlet_location]->depth();
                        double length_x_outlet = (*new_segmentset)[outlet_location]->lengthX();
                        double length_y_outlet = (*new_segmentset)[outlet_location]->lengthY();

                        double length_last = (*new_segmentset)[location_end]->length();
                        double depth_last = (*new_segmentset)[location_end]->depth();
                        double length_x_last = (*new_segmentset)[location_end]->lengthX();
                        double length_y_last = (*new_segmentset)[location_end]->lengthY();

                        double length_segment = (length_last - length_outlet) / number_segments;
                        double depth_segment = (depth_last - depth_outlet) / number_segments;
                        double length_x_segment = (length_x_last - length_x_outlet) / number_segments;
                        double length_y_segment = (length_y_last - length_y_outlet) / number_segments;

                        // the segments in the same range should share the same properties
                        double volume_segment = (*new_segmentset)[location_end]->crossArea() * length_segment;

                        if ((*new_segmentset)[location_end]->volume() < 0.5 * meaningless_value) {
                            (*new_segmentset)[location_end]->setVolume(volume_segment);
                        }

                        for (int k = location_begin; k < location_end; ++k) {
                            const double temp_length = length_outlet + (k - location_begin + 1) * length_segment;
                            (*new_segmentset)[k]->setLength(temp_length);
                            const double temp_depth = depth_outlet + (k - location_begin + 1) * depth_segment;
                            (*new_segmentset)[k]->setDepth(temp_depth);
                            const double temp_length_x = length_x_outlet + (k - location_begin + 1) * length_x_segment;
                            (*new_segmentset)[k]->setLengthX(temp_length_x);
                            const double temp_length_y = length_y_outlet + (k - location_begin + 1) * length_y_segment;
                            (*new_segmentset)[k]->setLengthY(temp_length_y);
                            (*new_segmentset)[k]->setDataReady(true);

                            if ((*new_segmentset)[k]->volume() < 0.5 * meaningless_value) {
                                (*new_segmentset)[k]->setVolume(volume_segment);
                            }
                        }
                        break;
                    }
                }
            } while (!all_ready);

            // then update the volume for all the segments except the top segment
            for (int i = 1; i < new_segmentset->numberSegment(); ++i) {
               int outlet_segment = (*new_segmentset)[i]->outletSegment();
               int outlet_location = new_segmentset->numberToLocation(outlet_segment);
               double segment_length = (*new_segmentset)[i]->length() - (*new_segmentset)[outlet_location]->length();
               if ((*new_segmentset)[i]->volume() < 0.5 * meaningless_value) {
                   (*new_segmentset)[i]->setVolume((*new_segmentset)[i]->crossArea() * segment_length);
               }
            }

            m_segmentset->update(time_step, new_segmentset);
        } else {
            throw std::logic_error("re-entering WELSEGS for a well is not supported yet!!.");
        }
    }

    void Well::addSegmentSetINC(size_t time_step, const SegmentSetPtr new_segmentset, const bool first_time) {
        // The following code only applies when no loop exist.
        if (first_time) {
            // update the information inside new_segmentset to be in ABS way
            (*new_segmentset)[0]->setLength(new_segmentset->lengthTopSegment());
            (*new_segmentset)[0]->setDepth(new_segmentset->depthTopSegment());
            (*new_segmentset)[0]->setLengthX(new_segmentset->xTop());
            (*new_segmentset)[0]->setLengthY(new_segmentset->yTop());
            (*new_segmentset)[0]->setDataReady(true);

            bool all_ready;

            do {
                all_ready = true;
                for (int i = 1; i < new_segmentset->numberSegment(); ++i) {
                    if ((*new_segmentset)[i]->dataReady() == false) {
                        all_ready = false;
                        // check the information about the outlet segment
                        // find the outlet segment with true dataReady()
                        int outlet_segment = (*new_segmentset)[i]->outletSegment();
                        int outlet_location = new_segmentset->numberToLocation(outlet_segment);

                        if ((*new_segmentset)[outlet_location]->dataReady() == true) {
                            const double temp_length = (*new_segmentset)[i]->length() + (*new_segmentset)[outlet_location]->length();
                            (*new_segmentset)[i]->setLength(temp_length);
                            const double temp_depth = (*new_segmentset)[i]->depth() + (*new_segmentset)[outlet_location]->depth();
                            (*new_segmentset)[i]->setDepth(temp_depth);
                            const double temp_length_x = (*new_segmentset)[i]->lengthX() + (*new_segmentset)[outlet_location]->lengthX();
                            (*new_segmentset)[i]->setLengthX(temp_length_x);
                            const double temp_length_y = (*new_segmentset)[i]->lengthY() + (*new_segmentset)[outlet_location]->lengthY();
                            (*new_segmentset)[i]->setLengthY(temp_length_y);
                            (*new_segmentset)[i]->setDataReady(true);
                            break;
                        }

                        int current_location;
                        int i_depth = 0;
                        while ((*new_segmentset)[outlet_location]->dataReady() == false) {
                            current_location = outlet_location;
                            outlet_segment = (*new_segmentset)[outlet_location]->outletSegment();
                            outlet_location = new_segmentset->numberToLocation(outlet_segment);

                            assert((outlet_location >= 0) && (outlet_location < new_segmentset->numberSegment()));

                            ++i_depth;
                            // when it enters dead loop, should throw an exception
                            // 10000 is a temporary manual here.
                            if (i_depth > 10000) {
                                throw std::runtime_error("loop exist or something wrong with the segment structure ");
                            }
                        }

                        if ((*new_segmentset)[outlet_location]->dataReady() == true) {
                            const double temp_length = (*new_segmentset)[current_location]->length() + (*new_segmentset)[outlet_location]->length();
                            (*new_segmentset)[current_location]->setLength(temp_length);
                            const double temp_depth = (*new_segmentset)[current_location]->depth() + (*new_segmentset)[outlet_location]->depth();
                            (*new_segmentset)[current_location]->setDepth(temp_depth);
                            const double temp_length_x = (*new_segmentset)[current_location]->lengthX() + (*new_segmentset)[outlet_location]->lengthX();
                            (*new_segmentset)[current_location]->setLengthX(temp_length_x);
                            const double temp_length_y = (*new_segmentset)[current_location]->lengthY() + (*new_segmentset)[outlet_location]->lengthY();
                            (*new_segmentset)[current_location]->setLengthY(temp_length_y);
                            (*new_segmentset)[current_location]->setDataReady(true);
                            break;
                        }
                    }
                }

            } while (!all_ready);

            m_segmentset->update(time_step, new_segmentset);
        } else {
            throw std::logic_error("re-entering WELSEGS for a well is not supported yet!!.");
        }
    }


}



