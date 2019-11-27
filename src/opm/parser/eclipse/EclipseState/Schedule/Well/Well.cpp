/*
  Copyright 2019 Equinor ASA.

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

#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Parser/ParserKeywords/W.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/MSW/updatingConnectionsWithSegments.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQActive.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellInjectionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellProductionProperties.hpp>
#include <fnmatch.h>

namespace Opm {

namespace {

    bool defaulted(const DeckRecord& rec, const std::string& s) {
        const auto& item = rec.getItem( s );
        if (item.defaultApplied(0))
            return true;

        if (item.get<int>(0) == 0)
            return true;

        return false;
    }


    int limit(const DeckRecord& rec, const std::string&s , int shift) {
        const auto& item = rec.getItem( s );
        return shift + item.get<int>(0);
    }

    bool match_le(int value, const DeckRecord& rec, const std::string& s, int shift = 0) {
        if (defaulted(rec,s))
            return true;

        return (value <= limit(rec,s,shift));
    }

    bool match_ge(int value, const DeckRecord& rec, const std::string& s, int shift = 0) {
        if (defaulted(rec,s))
            return true;

        return (value >= limit(rec,s,shift));
    }


    bool match_eq(int value, const DeckRecord& rec, const std::string& s, int shift = 0) {
        if (defaulted(rec,s))
            return true;

        return (limit(rec,s,shift) == value);
    }

}

Well::Well() :
    init_step(0),
    insert_index(0),
    headI(0),
    headJ(0),
    ref_depth(0.0),
    phase(Phase::OIL),
    ordering(Connection::Order::DEPTH),
    udq_undefined(0.0),
    status(Status::STOP),
    drainage_radius(0.0),
    allow_cross_flow(false),
    automatic_shutin(false),
    producer(false),
    efficiency_factor(0.0),
    solvent_fraction(0.0)
{
}


Well::Well(const std::string& wname_arg,
             const std::string& gname,
             std::size_t init_step_arg,
             std::size_t insert_index_arg,
             int headI_arg,
             int headJ_arg,
             double ref_depth_arg,
             Phase phase_arg,
             ProducerCMode whistctl_cmode,
             Connection::Order ordering_arg,
             const UnitSystem& unit_system_arg,
             double udq_undefined_arg) :
    wname(wname_arg),
    group_name(gname),
    init_step(init_step_arg),
    insert_index(insert_index_arg),
    headI(headI_arg),
    headJ(headJ_arg),
    ref_depth(ref_depth_arg),
    phase(phase_arg),
    ordering(ordering_arg),
    unit_system(unit_system_arg),
    udq_undefined(udq_undefined_arg),
    status(Status::SHUT),
    drainage_radius(ParserKeywords::WELSPECS::D_RADIUS::defaultValue),
    allow_cross_flow(DeckItem::to_bool(ParserKeywords::WELSPECS::CROSSFLOW::defaultValue)),
    automatic_shutin( ParserKeywords::WELSPECS::CROSSFLOW::defaultValue == "SHUT"),
    producer(true),
    guide_rate({true, -1, Well::GuideRateTarget::UNDEFINED,ParserKeywords::WGRUPCON::SCALING_FACTOR::defaultValue}),
    efficiency_factor(1.0),
    solvent_fraction(0.0),
    econ_limits(std::make_shared<WellEconProductionLimits>()),
    foam_properties(std::make_shared<WellFoamProperties>()),
    polymer_properties(std::make_shared<WellPolymerProperties>()),
    brine_properties(std::make_shared<WellBrineProperties>()),
    tracer_properties(std::make_shared<WellTracerProperties>()),
    connections(std::make_shared<WellConnections>(headI, headJ)),
    production(std::make_shared<WellProductionProperties>(wname)),
    injection(std::make_shared<WellInjectionProperties>(wname))
{
    auto p = std::make_shared<WellProductionProperties>(wname);
    p->whistctl_cmode = whistctl_cmode;
    this->updateProduction(p);
}

Well::Well(const std::string& wname_arg,
          const std::string& gname,
          std::size_t init_step_arg,
          std::size_t insert_index_arg,
          int headI_arg,
          int headJ_arg,
          double ref_depth_arg,
          const Phase& phase_arg,
          Connection::Order ordering_arg,
          const UnitSystem& units,
          double udq_undefined_arg,
          Status status_arg,
          double drainageRadius,
          bool allowCrossFlow,
          bool automaticShutIn,
          bool isProducer,
          const WellGuideRate& guideRate,
          double efficiencyFactor,
          double solventFraction,
          bool predictionMode,
          std::shared_ptr<const WellEconProductionLimits> econLimits,
          std::shared_ptr<const WellFoamProperties> foamProperties,
          std::shared_ptr<const WellPolymerProperties> polymerProperties,
          std::shared_ptr<const WellTracerProperties> tracerProperties,
          std::shared_ptr<WellConnections> connections_arg,
          std::shared_ptr<const WellProductionProperties> production_arg,
          std::shared_ptr<const WellInjectionProperties> injection_arg,
          std::shared_ptr<const WellSegments> segments_arg) :
    wname(wname_arg),
    group_name(gname),
    init_step(init_step_arg),
    insert_index(insert_index_arg),
    headI(headI_arg),
    headJ(headJ_arg),
    ref_depth(ref_depth_arg),
    phase(phase_arg),
    ordering(ordering_arg),
    unit_system(units),
    udq_undefined(udq_undefined_arg),
    status(status_arg),
    drainage_radius(drainageRadius),
    allow_cross_flow(allowCrossFlow),
    automatic_shutin(automaticShutIn),
    producer(isProducer),
    guide_rate(guideRate),
    efficiency_factor(efficiencyFactor),
    solvent_fraction(solventFraction),
    prediction_mode(predictionMode),
    econ_limits(econLimits),
    foam_properties(foamProperties),
    polymer_properties(polymerProperties),
    tracer_properties(tracerProperties),
    connections(connections_arg),
    production(production_arg),
    injection(injection_arg),
    segments(segments_arg)
{
}

bool Well::updateEfficiencyFactor(double efficiency_factor_arg) {
    if (this->efficiency_factor != efficiency_factor_arg) {
        this->efficiency_factor = efficiency_factor_arg;
        return true;
    }

    return false;
}

bool Well::updateWellGuideRate(double guide_rate_arg) {
    if (this->guide_rate.guide_rate != guide_rate_arg) {
        this->guide_rate.guide_rate = guide_rate_arg;
        return true;
    }

    return false;
}


bool Well::updateFoamProperties(std::shared_ptr<WellFoamProperties> foam_properties_arg) {
    if (this->producer) {
        throw std::runtime_error("Not allowed to set foam injection properties for well " + name()
                                 + " since it is a production well");
    }
    if (*this->foam_properties != *foam_properties_arg) {
        this->foam_properties = foam_properties_arg;
        return true;
    }

    return false;
}


bool Well::updatePolymerProperties(std::shared_ptr<WellPolymerProperties> polymer_properties_arg) {
    if (this->producer) {
        throw std::runtime_error("Not allowed to set polymer injection properties for well " + name() +
                                 " since it is a production well");
    }
    if (*this->polymer_properties != *polymer_properties_arg) {
        this->polymer_properties = polymer_properties_arg;
        return true;
    }

    return false;
}

bool Well::updateBrineProperties(std::shared_ptr<WellBrineProperties> brine_properties_arg) {
    if (this->producer) {
        throw std::runtime_error("Not allowed to set brine injection properties for well " + name() +
                                 " since it is a production well");
    }
    if (*this->brine_properties != *brine_properties_arg) {
        this->brine_properties = brine_properties_arg;
        return true;
    }

    return false;
}


bool Well::updateEconLimits(std::shared_ptr<WellEconProductionLimits> econ_limits_arg) {
    if (*this->econ_limits != *econ_limits_arg) {
        this->econ_limits = econ_limits_arg;
        return true;
    }

    return false;
}

void Well::switchToProducer() {
    auto p = std::make_shared<WellInjectionProperties>(this->getInjectionProperties());

    p->BHPTarget.reset(0);
    p->dropInjectionControl( Opm::Well::InjectorCMode::BHP );
    this->updateInjection( p );
    this->updateProducer(true);
}


void Well::switchToInjector() {
    auto p = std::make_shared<WellProductionProperties>(getProductionProperties());

    p->setBHPLimit(0);
    p->dropProductionControl( ProducerCMode::BHP );
    this->updateProduction( p );
    this->updateProducer( false );
}

bool Well::updateInjection(std::shared_ptr<WellInjectionProperties> injection_arg) {
    if (this->producer)
        this->switchToInjector( );

    if (*this->injection != *injection_arg) {
        this->injection = injection_arg;
        return true;
    }

    return false;
}

bool Well::updateProduction(std::shared_ptr<WellProductionProperties> production_arg) {
    if (!this->producer)
        this->switchToProducer( );

    if (*this->production != *production_arg) {
        this->production = production_arg;
        return true;
    }

    return false;
}

bool Well::updateTracer(std::shared_ptr<WellTracerProperties> tracer_properties_arg) {
    if (*this->tracer_properties != *tracer_properties_arg) {
        this->tracer_properties = tracer_properties_arg;
        return true;
    }

    return false;
}

bool Well::updateWellGuideRate(bool available, double guide_rate_arg, GuideRateTarget guide_phase, double scale_factor) {
    bool update = false;
    if (this->guide_rate.available != available) {
        this->guide_rate.available = available;
        update = true;
    }

    if(this->guide_rate.guide_rate != guide_rate_arg) {
        this->guide_rate.guide_rate = guide_rate_arg;
        update = true;
    }

    if(this->guide_rate.guide_phase != guide_phase) {
        this->guide_rate.guide_phase = guide_phase;
        update = true;
    }

    if(this->guide_rate.scale_factor != scale_factor) {
        this->guide_rate.scale_factor = scale_factor;
        update = true;
    }

    return update;
}


bool Well::updateProducer(bool producer_arg) {
    if (this->producer != producer_arg) {
        this->producer = producer_arg;
        return true;
    }
    return false;
}


bool Well::updateGroup(const std::string& group_arg) {
    if (this->group_name != group_arg) {
        this->group_name = group_arg;
        return true;
    }
    return false;
}


bool Well::updateHead(int I, int J) {
    bool update = false;
    if (this->headI != I) {
        this->headI = I;
        update = true;
    }

    if (this->headJ != J) {
        this->headJ = J;
        update = true;
    }

    return update;
}


bool Well::updateStatus(Status status_arg) {
    if (this->status != status_arg) {
        this->status = status_arg;
        return true;
    }

    return false;
}


bool Well::updateRefDepth(double ref_depth_arg) {
    if (this->ref_depth != ref_depth_arg) {
        this->ref_depth = ref_depth_arg;
        return true;
    }

    return false;
}

bool Well::updateDrainageRadius(double drainage_radius_arg) {
    if (this->drainage_radius != drainage_radius_arg) {
        this->drainage_radius = drainage_radius_arg;
        return true;
    }

    return false;
}


bool Well::updateCrossFlow(bool allow_cross_flow_arg) {
    if (this->allow_cross_flow != allow_cross_flow_arg) {
        this->allow_cross_flow = allow_cross_flow_arg;
        return true;
    }

    return false;
}

bool Well::updateAutoShutin(bool auto_shutin) {
    if (this->automatic_shutin != auto_shutin) {
        this->automatic_shutin = auto_shutin;
        return true;
    }

    return false;
}


bool Well::updateConnections(const std::shared_ptr<WellConnections> connections_arg) {
    if( this->ordering  == Connection::Order::TRACK)
        connections_arg->orderConnections( this->headI, this->headJ );

    if (*this->connections != *connections_arg) {
        this->connections = connections_arg;
        //if (this->connections->allConnectionsShut()) {}
        // This status update breaks line 825 in ScheduleTests
        //this->status = WellCommon::StatusEnum::SHUT;
        return true;
    }

    return false;
}


bool Well::updateSolventFraction(double solvent_fraction_arg) {
    if (this->solvent_fraction != solvent_fraction_arg) {
        this->solvent_fraction = solvent_fraction_arg;
        return true;
    }

    return false;
}


bool Well::handleCOMPSEGS(const DeckKeyword& keyword, const EclipseGrid& grid,
                           const ParseContext& parseContext, ErrorGuard& errors) {
    std::shared_ptr<WellConnections> new_connection_set( newConnectionsWithSegments(keyword, *this->connections, *this->segments , grid,
                                                                                    parseContext, errors) );
    return this->updateConnections(new_connection_set);
}

const std::string& Well::groupName() const {
    return this->group_name;
}


bool Well::isMultiSegment() const {
    if (this->segments)
        return true;
    return false;
}

bool Well::isProducer() const {
    return this->producer;
}

bool Well::isInjector() const {
    return !this->producer;
}


Well::InjectorType Well::injectorType() const {
    if (this->producer)
        throw std::runtime_error("Can not access injectorType attribute of a producer");

    return this->injection->injectorType;
}



bool Well::isAvailableForGroupControl() const {
    return this->guide_rate.available;
}

double Well::getGuideRate() const {
    return this->guide_rate.guide_rate;
}

Well::GuideRateTarget Well::getGuideRatePhase() const {
    return this->guide_rate.guide_phase;
}

double Well::getGuideRateScalingFactor() const {
    return this->guide_rate.scale_factor;
}


double Well::getEfficiencyFactor() const {
    return this->efficiency_factor;
}

double Well::getSolventFraction() const {
    return this->solvent_fraction;
}



std::size_t Well::seqIndex() const {
    return this->insert_index;
}

int Well::getHeadI() const {
    return this->headI;
}

int Well::getHeadJ() const {
    return this->headJ;
}

bool Well::getAutomaticShutIn() const {
    return this->automatic_shutin;
}

bool Well::getAllowCrossFlow() const {
    return this->allow_cross_flow;
}

double Well::getRefDepth() const {
    if( this->ref_depth >= 0.0 )
        return this->ref_depth;

    // ref depth was defaulted and we get the depth of the first completion
    if( this->connections->size() == 0 ) {
        throw std::invalid_argument( "No completions defined for well: "
                                     + name()
                                     + ". Can not infer reference depth" );
    }
    return this->connections->get(0).depth();
}


double Well::getDrainageRadius() const {
    return this->drainage_radius;
}


const std::string& Well::name() const {
    return this->wname;
}


const WellConnections& Well::getConnections() const {
    return *this->connections;
}

const WellFoamProperties& Well::getFoamProperties() const {
    return *this->foam_properties;
}

const WellPolymerProperties& Well::getPolymerProperties() const {
    return *this->polymer_properties;
}

const WellBrineProperties& Well::getBrineProperties() const {
    return *this->brine_properties;
}

const WellTracerProperties& Well::getTracerProperties() const {
    return *this->tracer_properties;
}

const WellEconProductionLimits& Well::getEconLimits() const {
    return *this->econ_limits;
}

const Well::WellProductionProperties& Well::getProductionProperties() const {
    return *this->production;
}

const WellSegments& Well::getSegments() const {
    if (this->segments)
        return *this->segments;
    else
        throw std::logic_error("Asked for segment information in not MSW well: " + this->name());
}

const Well::WellInjectionProperties& Well::getInjectionProperties() const {
    return *this->injection;
}

Well::Status Well::getStatus() const {
    return this->status;
}


std::map<int, std::vector<Connection>> Well::getCompletions() const {
    std::map<int, std::vector<Connection>> completions;

    for (const auto& conn : *this->connections) {
        auto pair = completions.find( conn.complnum() );
        if (pair == completions.end())
            completions[conn.complnum()] = {};

        pair = completions.find(conn.complnum());
        pair->second.push_back(conn);
    }

    return completions;
}

Phase Well::getPreferredPhase() const {
    return this->phase;
}

/*
  When all connections of a well are closed with the WELOPEN keywords, the well
  itself should also be SHUT. In the main parsing code this is handled by the
  function checkIfAllConnectionsIsShut() which is called at the end of every
  report step in Schedule::iterateScheduleSection(). This is donn in this way
  because there is some twisted logic aggregating connection changes over a
  complete report step.

  However - when the WELOPEN is called as a ACTIONX action the full
  Schedule::iterateScheduleSection() is not run and the check if all connections
  is closed is not done. Therefor we have a action_mode flag here which makes
  sure to close the well in this case.
*/


bool Well::handleWELOPEN(const DeckRecord& record, Connection::State state_arg, bool action_mode) {

    auto match = [=]( const Connection &c) -> bool {
        if (!match_eq(c.getI(), record, "I" , -1)) return false;
        if (!match_eq(c.getJ(), record, "J" , -1)) return false;
        if (!match_eq(c.getK(), record, "K", -1))  return false;
        if (!match_ge(c.complnum(), record, "C1")) return false;
        if (!match_le(c.complnum(), record, "C2")) return false;

        return true;
    };

    auto new_connections = std::make_shared<WellConnections>(this->headI, this->headJ);

    for (auto c : *this->connections) {
        if (match(c))
            c.setState( state_arg );

        new_connections->add(c);
    }
    if (action_mode) {
        if (new_connections->allConnectionsShut())
            this->status = Status::SHUT;
    }

    return this->updateConnections(new_connections);
}

bool Well::handleCOMPLUMP(const DeckRecord& record) {

    auto match = [=]( const Connection &c) -> bool {
        if (!match_eq(c.getI(), record, "I" , -1))  return false;
        if (!match_eq(c.getJ(), record, "J" , -1))  return false;
        if (!match_ge(c.getK(), record, "K1", -1)) return false;
        if (!match_le(c.getK(), record, "K2", -1)) return false;

        return true;
    };

    auto new_connections = std::make_shared<WellConnections>(this->headI, this->headJ);
    const int complnum = record.getItem("N").get<int>(0);
    if (complnum <= 0)
        throw std::invalid_argument("Completion number must be >= 1. COMPLNUM=" + std::to_string(complnum) + "is invalid");

    for (auto c : *this->connections) {
        if (match(c))
            c.setComplnum( complnum );

        new_connections->add(c);
    }

    return this->updateConnections(new_connections);
}



bool Well::handleWPIMULT(const DeckRecord& record) {

    auto match = [=]( const Connection &c) -> bool {
        if (!match_ge(c.complnum(), record, "FIRST")) return false;
        if (!match_le(c.complnum(), record, "LAST"))  return false;
        if (!match_eq(c.getI()  , record, "I", -1)) return false;
        if (!match_eq(c.getJ()  , record, "J", -1)) return false;
        if (!match_eq(c.getK()  , record, "K", -1)) return false;

        return true;
    };

    auto new_connections = std::make_shared<WellConnections>(this->headI, this->headJ);
    double wellPi = record.getItem("WELLPI").get< double >(0);

    for (auto c : *this->connections) {
        if (match(c))
            c.scaleWellPi( wellPi );

        new_connections->add(c);
    }

    return this->updateConnections(new_connections);
}


bool Well::handleWELSEGS(const DeckKeyword& keyword) {
    if( this->segments )
        throw std::logic_error("re-entering WELSEGS for a well is not supported yet!!.");

    auto new_segmentset = std::make_shared<WellSegments>();
    new_segmentset->loadWELSEGS(keyword);

    new_segmentset->process(true);
    if (new_segmentset != this->segments) {
        this->segments = new_segmentset;
        this->ref_depth = new_segmentset->depthTopSegment();
        return true;
    } else
        return false;
}



bool Well::updateWSEGSICD(const std::vector<std::pair<int, SpiralICD> >& sicd_pairs) {
    auto new_segments = std::make_shared<WellSegments>(*this->segments);
    if (new_segments->updateWSEGSICD(sicd_pairs)) {
        this->segments = new_segments;
        return true;
    } else
        return false;
}


bool Well::updateWSEGVALV(const std::vector<std::pair<int, Valve> >& valve_pairs) {
    auto new_segments = std::make_shared<WellSegments>(*this->segments);
    if (new_segments->updateWSEGVALV(valve_pairs)) {
        this->segments = new_segments;
        return true;
    } else
        return false;
}

void Well::filterConnections(const ActiveGridCells& grid) {
    this->connections->filter(grid);
}


std::size_t Well::firstTimeStep() const {
    return this->init_step;
}

bool Well::hasBeenDefined(size_t timeStep) const {
    if (timeStep < this->init_step)
        return false;
    else
        return true;
}



bool Well::canOpen() const {
    if (this->allow_cross_flow)
        return true;

    /*
      If the UDAValue is in string mode we return true unconditionally, without
      evaluating the internal UDA value.
    */
    if (this->producer) {
        const auto& prod = *this->production;
        if (prod.OilRate.is<std::string>())
            return true;

        if (prod.GasRate.is<std::string>())
          return true;

        if (prod.WaterRate.is<std::string>())
          return true;

        if (!prod.OilRate.zero())
            return true;

        if (!prod.GasRate.zero())
            return true;

        if (!prod.WaterRate.zero())
            return true;

        return false;
    } else {
        const auto& inj = *this->injection;
        if (inj.surfaceInjectionRate.is<std::string>())
            return true;

        return !inj.surfaceInjectionRate.zero();
    }
}


bool Well::predictionMode() const {
    return this->prediction_mode;
}


bool Well::updatePrediction(bool prediction_mode_arg) {
    if (this->prediction_mode != prediction_mode_arg) {
        this->prediction_mode = prediction_mode_arg;
        return true;
    }

    return false;
}


Connection::Order Well::getWellConnectionOrdering() const {
    return this->ordering;
}

double Well::production_rate(const SummaryState& st, Phase prod_phase) const {
    if( !this->isProducer() ) return 0.0;

    const auto controls = this->productionControls(st);

    switch( prod_phase ) {
        case Phase::WATER: return controls.water_rate;
        case Phase::OIL:   return controls.oil_rate;
        case Phase::GAS:   return controls.gas_rate;
        case Phase::SOLVENT:
            throw std::invalid_argument( "Production of 'SOLVENT' requested." );
        case Phase::POLYMER:
            throw std::invalid_argument( "Production of 'POLYMER' requested." );
        case Phase::ENERGY:
            throw std::invalid_argument( "Production of 'ENERGY' requested." );
        case Phase::POLYMW:
            throw std::invalid_argument( "Production of 'POLYMW' requested.");
        case Phase::FOAM:
            throw std::invalid_argument( "Production of 'FOAM' requested.");
        case Phase::BRINE:
        throw std::invalid_argument( "Production of 'BRINE' requested.");
    }

    throw std::logic_error( "Unreachable state. Invalid Phase value. "
                            "This is likely a programming error." );
}

double Well::injection_rate(const SummaryState& st, Phase phase_arg) const {
    if( !this->isInjector() ) return 0.0;
    const auto controls = this->injectionControls(st);

    const auto type = controls.injector_type;

    if( phase_arg == Phase::WATER && type != Well::InjectorType::WATER ) return 0.0;
    if( phase_arg == Phase::OIL   && type != Well::InjectorType::OIL   ) return 0.0;
    if( phase_arg == Phase::GAS   && type != Well::InjectorType::GAS   ) return 0.0;

    return controls.surface_rate;
}


bool Well::wellNameInWellNamePattern(const std::string& wellName, const std::string& wellNamePattern) {
    bool wellNameInPattern = false;
    if (fnmatch( wellNamePattern.c_str() , wellName.c_str() , 0 ) == 0) {
        wellNameInPattern = true;
    }
    return wellNameInPattern;
}


Well::ProductionControls Well::productionControls(const SummaryState& st) const {
    if (this->isProducer()) {
        auto controls = this->production->controls(st, this->udq_undefined);
        controls.prediction_mode = this->predictionMode();
        return controls;
    } else
        throw std::logic_error("Trying to get production data from an injector");
}

Well::InjectionControls Well::injectionControls(const SummaryState& st) const {
    if (!this->isProducer()) {
        auto controls = this->injection->controls(this->unit_system, st, this->udq_undefined);
        controls.prediction_mode = this->predictionMode();
        return controls;
    } else
        throw std::logic_error("Trying to get injection data from a producer");
}


/*
  These three accessor functions are at the "wrong" level of abstraction;
  the same properties are part of the InjectionControls and
  ProductionControls structs. They are made available here to avoid passing
  a SummaryState instance in situations where it is not really needed.
*/


int Well::vfp_table_number() const {
    if (this->producer)
        return this->production->VFPTableNumber;
    else
        return this->injection->VFPTableNumber;
}

double Well::alq_value() const {
    if (this->producer)
        return this->production->ALQValue;

    throw std::runtime_error("Can not ask for ALQ value in an injector");
}

double Well::temperature() const {
    if (!this->producer)
        return this->injection->temperature;

    throw std::runtime_error("Can not ask for temperature in a producer");
}


std::string Well::Status2String(Well::Status enumValue) {
    switch( enumValue ) {
    case Status::OPEN:
        return "OPEN";
    case Status::SHUT:
        return "SHUT";
    case Status::AUTO:
        return "AUTO";
    case Status::STOP:
        return "STOP";
    default:
        throw std::invalid_argument("unhandled enum value");
    }
}


Well::Status Well::StatusFromString(const std::string& stringValue) {
    if (stringValue == "OPEN")
        return Status::OPEN;
    else if (stringValue == "SHUT")
        return Status::SHUT;
    else if (stringValue == "STOP")
        return Status::STOP;
    else if (stringValue == "AUTO")
        return Status::AUTO;
    else
        throw std::invalid_argument("Unknown enum state string: " + stringValue );
}


const std::string Well::InjectorType2String( Well::InjectorType enumValue ) {
    switch( enumValue ) {
    case InjectorType::OIL:
        return "OIL";
    case InjectorType::GAS:
        return "GAS";
    case InjectorType::WATER:
        return "WATER";
    case InjectorType::MULTI:
        return "MULTI";
    default:
        throw std::invalid_argument("unhandled enum value");
    }
}

Well::InjectorType Well::InjectorTypeFromString( const std::string& stringValue ) {
    if (stringValue == "OIL")
        return InjectorType::OIL;
    else if (stringValue == "WATER")
        return InjectorType::WATER;
    else if (stringValue == "WAT")
        return InjectorType::WATER;
    else if (stringValue == "GAS")
        return InjectorType::GAS;
    else if (stringValue == "MULTI")
        return InjectorType::MULTI;
    else
        throw std::invalid_argument("Unknown enum state string: " + stringValue );
}

const std::string Well::InjectorCMode2String( InjectorCMode enumValue ) {
    switch( enumValue ) {
    case InjectorCMode::RESV:
        return "RESV";
    case InjectorCMode::RATE:
        return "RATE";
    case InjectorCMode::BHP:
        return "BHP";
    case InjectorCMode::THP:
        return "THP";
    case InjectorCMode::GRUP:
        return "GRUP";
    default:
        throw std::invalid_argument("unhandled enum value");
    }
}


Well::InjectorCMode Well::InjectorCModeFromString(const std::string &stringValue) {
    if (stringValue == "RATE")
        return InjectorCMode::RATE;
    else if (stringValue == "RESV")
        return InjectorCMode::RESV;
    else if (stringValue == "BHP")
        return InjectorCMode::BHP;
    else if (stringValue == "THP")
        return InjectorCMode::THP;
    else if (stringValue == "GRUP")
        return InjectorCMode::GRUP;
    else
        throw std::invalid_argument("Unknown enum state string: " + stringValue);
}

Well::WELTARGCMode Well::WELTARGCModeFromString(const std::string& string_value) {
    if (string_value == "ORAT")
        return WELTARGCMode::ORAT;

    if (string_value == "WRAT")
        return WELTARGCMode::WRAT;

    if (string_value == "GRAT")
        return WELTARGCMode::GRAT;

    if (string_value == "LRAT")
        return WELTARGCMode::LRAT;

    if (string_value == "CRAT")
        return WELTARGCMode::CRAT;

    if (string_value == "RESV")
        return WELTARGCMode::RESV;

    if (string_value == "BHP")
        return WELTARGCMode::BHP;

    if (string_value == "THP")
        return WELTARGCMode::THP;

    if (string_value == "VFP")
        return WELTARGCMode::VFP;

    if (string_value == "LIFT")
        return WELTARGCMode::LIFT;

    if (string_value == "GUID")
        return WELTARGCMode::GUID;

    throw std::invalid_argument("WELTARG control mode: " + string_value + " not recognized.");
}


const std::string Well::ProducerCMode2String( ProducerCMode enumValue ) {
    switch( enumValue ) {
    case ProducerCMode::ORAT:
        return "ORAT";
    case ProducerCMode::WRAT:
        return "WRAT";
    case ProducerCMode::GRAT:
        return "GRAT";
    case ProducerCMode::LRAT:
        return "LRAT";
    case ProducerCMode::CRAT:
        return "CRAT";
    case ProducerCMode::RESV:
        return "RESV";
    case ProducerCMode::BHP:
        return "BHP";
    case ProducerCMode::THP:
        return "THP";
    case ProducerCMode::GRUP:
        return "GRUP";
    default:
        throw std::invalid_argument("unhandled enum value");
    }
}

Well::ProducerCMode Well::ProducerCModeFromString( const std::string& stringValue ) {
    if (stringValue == "ORAT")
        return ProducerCMode::ORAT;
    else if (stringValue == "WRAT")
        return ProducerCMode::WRAT;
    else if (stringValue == "GRAT")
        return ProducerCMode::GRAT;
    else if (stringValue == "LRAT")
        return ProducerCMode::LRAT;
    else if (stringValue == "CRAT")
        return ProducerCMode::CRAT;
    else if (stringValue == "RESV")
        return ProducerCMode::RESV;
    else if (stringValue == "BHP")
        return ProducerCMode::BHP;
    else if (stringValue == "THP")
        return ProducerCMode::THP;
    else if (stringValue == "GRUP")
        return ProducerCMode::GRUP;
    else if (stringValue == "NONE")
        return ProducerCMode::NONE;
    else
        throw std::invalid_argument("Unknown enum state string: " + stringValue );
}


const std::string Well::GuideRateTarget2String( GuideRateTarget enumValue ) {
    switch( enumValue ) {
    case GuideRateTarget::OIL:
        return "OIL";
    case GuideRateTarget::WAT:
        return "WAT";
    case GuideRateTarget::GAS:
        return "GAS";
    case GuideRateTarget::LIQ:
        return "LIQ";
    case GuideRateTarget::COMB:
        return "COMB";
    case GuideRateTarget::WGA:
        return "WGA";
    case GuideRateTarget::CVAL:
        return "CVAL";
    case GuideRateTarget::RAT:
        return "RAT";
    case GuideRateTarget::RES:
        return "RES";
    case GuideRateTarget::UNDEFINED:
        return "UNDEFINED";
    default:
        throw std::invalid_argument("unhandled enum value");
    }
}

Well::GuideRateTarget Well::GuideRateTargetFromString( const std::string& stringValue ) {
    if (stringValue == "OIL")
        return GuideRateTarget::OIL;
    else if (stringValue == "WAT")
        return GuideRateTarget::WAT;
    else if (stringValue == "GAS")
        return GuideRateTarget::GAS;
    else if (stringValue == "LIQ")
        return GuideRateTarget::LIQ;
    else if (stringValue == "COMB")
        return GuideRateTarget::COMB;
    else if (stringValue == "WGA")
        return GuideRateTarget::WGA;
    else if (stringValue == "CVAL")
        return GuideRateTarget::CVAL;
    else if (stringValue == "RAT")
        return GuideRateTarget::RAT;
    else if (stringValue == "RES")
        return GuideRateTarget::RES;
    else if (stringValue == "UNDEFINED")
        return GuideRateTarget::UNDEFINED;
    else
        throw std::invalid_argument("Unknown enum state string: " + stringValue );
}

const Well::WellGuideRate& Well::wellGuideRate() const {
    return guide_rate;
}

const UnitSystem& Well::units() const {
    return unit_system;
}

double Well::udqUndefined() const {
    return udq_undefined;
}

bool Well::hasSegments() const {
    return segments != nullptr;
}


bool Well::operator==(const Well& data) const {
    if (this->hasSegments() != data.hasSegments()) {
        return false;
    }

    if (this->hasSegments() && (this->getSegments() != data.getSegments()))  {
        return false;
    }

    return this->name() == data.name() &&
           this->groupName() == data.groupName() &&
           this->firstTimeStep() == data.firstTimeStep() &&
           this->seqIndex() == data.seqIndex() &&
           this->getHeadI() == data.getHeadI() &&
           this->getHeadJ() == data.getHeadJ() &&
           this->getRefDepth() == data.getRefDepth() &&
           this->getPreferredPhase() == data.getPreferredPhase() &&
           this->getWellConnectionOrdering() == data.getWellConnectionOrdering() &&
           this->units() == data.units() &&
           this->udqUndefined() == data.udqUndefined() &&
           this->getStatus() == data.getStatus() &&
           this->getDrainageRadius() == data.getDrainageRadius() &&
           this->getAllowCrossFlow() == data.getAllowCrossFlow() &&
           this->getAutomaticShutIn() == data.getAutomaticShutIn() &&
           this->isProducer() == data.isProducer() &&
           this->wellGuideRate() == data.wellGuideRate() &&
           this->getEfficiencyFactor() == data.getEfficiencyFactor() &&
           this->getSolventFraction() == data.getSolventFraction() &&
           this->getEconLimits() == data.getEconLimits() &&
           this->getFoamProperties() == data.getFoamProperties() &&
           this->getTracerProperties() == data.getTracerProperties() &&
           this->getProductionProperties() == data.getProductionProperties() &&
           this->getInjectionProperties() == data.getInjectionProperties();
}

}
