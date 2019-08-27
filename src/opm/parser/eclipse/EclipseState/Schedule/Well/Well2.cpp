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
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well2.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQActive.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellInjectionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellProductionProperties.hpp>

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


Well2::Well2(const std::string& wname_arg,
             const std::string& gname,
             std::size_t init_step_arg,
             std::size_t insert_index_arg,
             int headI_arg,
             int headJ_arg,
             double ref_depth_arg,
             Phase phase_arg,
             WellProducer::ControlModeEnum whistctl_cmode,
             WellCompletion::CompletionOrderEnum ordering_arg,
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
    guide_rate({true, -1, GuideRate::UNDEFINED,ParserKeywords::WGRUPCON::SCALING_FACTOR::defaultValue}),
    efficiency_factor(1.0),
    solvent_fraction(0.0),
    econ_limits(std::make_shared<WellEconProductionLimits>()),
    foam_properties(std::make_shared<WellFoamProperties>()),
    polymer_properties(std::make_shared<WellPolymerProperties>()),
    tracer_properties(std::make_shared<WellTracerProperties>()),
    connections(std::make_shared<WellConnections>(headI, headJ)),
    production(std::make_shared<WellProductionProperties>(wname)),
    injection(std::make_shared<WellInjectionProperties>(wname))
{
    auto p = std::make_shared<WellProductionProperties>(wname);
    p->whistctl_cmode = whistctl_cmode;
    this->updateProduction(p);
}

bool Well2::updateEfficiencyFactor(double efficiency_factor_arg) {
    if (this->efficiency_factor != efficiency_factor_arg) {
        this->efficiency_factor = efficiency_factor_arg;
        return true;
    }

    return false;
}

bool Well2::updateWellGuideRate(double guide_rate_arg) {
    if (this->guide_rate.guide_rate != guide_rate_arg) {
        this->guide_rate.guide_rate = guide_rate_arg;
        return true;
    }

    return false;
}


bool Well2::updateFoamProperties(std::shared_ptr<WellFoamProperties> foam_properties_arg) {
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


bool Well2::updatePolymerProperties(std::shared_ptr<WellPolymerProperties> polymer_properties_arg) {
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


bool Well2::updateEconLimits(std::shared_ptr<WellEconProductionLimits> econ_limits_arg) {
    if (*this->econ_limits != *econ_limits_arg) {
        this->econ_limits = econ_limits_arg;
        return true;
    }

    return false;
}

void Well2::switchToProducer() {
    auto p = std::make_shared<WellInjectionProperties>(this->getInjectionProperties());

    p->BHPLimit.reset( 0 );
    p->dropInjectionControl( Opm::Well2::InjectorCMode::BHP );
    this->updateInjection( p );
    this->updateProducer(true);
}


void Well2::switchToInjector() {
    auto p = std::make_shared<WellProductionProperties>(getProductionProperties());

    p->BHPLimit.assert_numeric();
    p->BHPLimit.reset(0);
    p->dropProductionControl( Opm::WellProducer::BHP );
    this->updateProduction( p );
    this->updateProducer( false );
}

bool Well2::updateInjection(std::shared_ptr<WellInjectionProperties> injection_arg) {
    if (this->producer)
        this->switchToInjector( );

    if (*this->injection != *injection_arg) {
        this->injection = injection_arg;
        return true;
    }

    return false;
}

bool Well2::updateProduction(std::shared_ptr<WellProductionProperties> production_arg) {
    if (!this->producer)
        this->switchToProducer( );

    if (*this->production != *production_arg) {
        this->production = production_arg;
        return true;
    }

    return false;
}

bool Well2::updateTracer(std::shared_ptr<WellTracerProperties> tracer_properties_arg) {
    if (*this->tracer_properties != *tracer_properties_arg) {
        this->tracer_properties = tracer_properties_arg;
        return true;
    }

    return false;
}

bool Well2::updateWellGuideRate(bool available, double guide_rate_arg, GuideRate::GuideRatePhaseEnum guide_phase, double scale_factor) {
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


bool Well2::updateProducer(bool producer_arg) {
    if (this->producer != producer_arg) {
        this->producer = producer_arg;
        return true;
    }
    return false;
}


bool Well2::updateGroup(const std::string& group_arg) {
    if (this->group_name != group_arg) {
        this->group_name = group_arg;
        return true;
    }
    return false;
}


bool Well2::updateHead(int I, int J) {
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


bool Well2::updateStatus(Status status_arg) {
    if (this->status != status_arg) {
        this->status = status_arg;
        return true;
    }

    return false;
}


bool Well2::updateRefDepth(double ref_depth_arg) {
    if (this->ref_depth != ref_depth_arg) {
        this->ref_depth = ref_depth_arg;
        return true;
    }

    return false;
}

bool Well2::updateDrainageRadius(double drainage_radius_arg) {
    if (this->drainage_radius != drainage_radius_arg) {
        this->drainage_radius = drainage_radius_arg;
        return true;
    }

    return false;
}


bool Well2::updateCrossFlow(bool allow_cross_flow_arg) {
    if (this->allow_cross_flow != allow_cross_flow_arg) {
        this->allow_cross_flow = allow_cross_flow_arg;
        return true;
    }

    return false;
}

bool Well2::updateAutoShutin(bool auto_shutin) {
    if (this->automatic_shutin != auto_shutin) {
        this->automatic_shutin = auto_shutin;
        return true;
    }

    return false;
}


bool Well2::updateConnections(const std::shared_ptr<WellConnections> connections_arg) {
    if( this->ordering  == WellCompletion::TRACK)
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


bool Well2::updateSolventFraction(double solvent_fraction_arg) {
    if (this->solvent_fraction != solvent_fraction_arg) {
        this->solvent_fraction = solvent_fraction_arg;
        return true;
    }

    return false;
}


bool Well2::handleCOMPSEGS(const DeckKeyword& keyword, const EclipseGrid& grid,
                           const ParseContext& parseContext, ErrorGuard& errors) {
    std::shared_ptr<WellConnections> new_connection_set( newConnectionsWithSegments(keyword, *this->connections, *this->segments , grid,
                                                                                    parseContext, errors) );
    return this->updateConnections(new_connection_set);
}

const std::string& Well2::groupName() const {
    return this->group_name;
}


bool Well2::isMultiSegment() const {
    if (this->segments)
        return true;
    return false;
}

bool Well2::isProducer() const {
    return this->producer;
}

bool Well2::isInjector() const {
    return !this->producer;
}


Well2::InjectorType Well2::injectorType() const {
    if (this->producer)
        throw std::runtime_error("Can not access injectorType attribute of a producer");

    return this->injection->injectorType;
}



bool Well2::isAvailableForGroupControl() const {
    return this->guide_rate.available;
}

double Well2::getGuideRate() const {
    return this->guide_rate.guide_rate;
}

GuideRate::GuideRatePhaseEnum Well2::getGuideRatePhase() const {
    return this->guide_rate.guide_phase;
}

double Well2::getGuideRateScalingFactor() const {
    return this->guide_rate.scale_factor;
}


double Well2::getEfficiencyFactor() const {
    return this->efficiency_factor;
}

double Well2::getSolventFraction() const {
    return this->solvent_fraction;
}



std::size_t Well2::seqIndex() const {
    return this->insert_index;
}

int Well2::getHeadI() const {
    return this->headI;
}

int Well2::getHeadJ() const {
    return this->headJ;
}

bool Well2::getAutomaticShutIn() const {
    return this->automatic_shutin;
}

bool Well2::getAllowCrossFlow() const {
    return this->allow_cross_flow;
}

double Well2::getRefDepth() const {
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


double Well2::getDrainageRadius() const {
    return this->drainage_radius;
}


const std::string& Well2::name() const {
    return this->wname;
}


const WellConnections& Well2::getConnections() const {
    return *this->connections;
}

const WellFoamProperties& Well2::getFoamProperties() const {
    return *this->foam_properties;
}

const WellPolymerProperties& Well2::getPolymerProperties() const {
    return *this->polymer_properties;
}


const WellTracerProperties& Well2::getTracerProperties() const {
    return *this->tracer_properties;
}

const WellEconProductionLimits& Well2::getEconLimits() const {
    return *this->econ_limits;
}

const WellProductionProperties& Well2::getProductionProperties() const {
    return *this->production;
}

const WellSegments& Well2::getSegments() const {
    if (this->segments)
        return *this->segments;
    else
        throw std::logic_error("Asked for segment information in not MSW well: " + this->name());
}

const Well2::WellInjectionProperties& Well2::getInjectionProperties() const {
    return *this->injection;
}

Well2::Status Well2::getStatus() const {
    return this->status;
}


std::map<int, std::vector<Connection>> Well2::getCompletions() const {
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

Phase Well2::getPreferredPhase() const {
    return this->phase;
}

bool Well2::handleWELOPEN(const DeckRecord& record, Connection::State state_arg) {

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
    return this->updateConnections(new_connections);
}

bool Well2::handleCOMPLUMP(const DeckRecord& record) {

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



bool Well2::handleWPIMULT(const DeckRecord& record) {

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


bool Well2::handleWELSEGS(const DeckKeyword& keyword) {
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

void Well2::filterConnections(const EclipseGrid& grid) {
    this->connections->filter(grid);
}


std::size_t Well2::firstTimeStep() const {
    return this->init_step;
}

bool Well2::hasBeenDefined(size_t timeStep) const {
    if (timeStep < this->init_step)
        return false;
    else
        return true;
}



bool Well2::canOpen() const {
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

        return ((prod.OilRate.get<double>() + prod.GasRate.get<double>() + prod.WaterRate.get<double>()) != 0);
    } else {
        const auto& inj = *this->injection;
        if (inj.surfaceInjectionRate.is<std::string>())
            return true;
        return inj.surfaceInjectionRate.get<double>() != 0;
    }
}


bool Well2::predictionMode() const {
    return this->prediction_mode;
}


bool Well2::updatePrediction(bool prediction_mode_arg) {
    if (this->prediction_mode != prediction_mode_arg) {
        this->prediction_mode = prediction_mode_arg;
        return true;
    }

    return false;
}


WellCompletion::CompletionOrderEnum Well2::getWellConnectionOrdering() const {
    return this->ordering;
}

double Well2::production_rate(const SummaryState& st, Phase prod_phase) const {
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
    }

    throw std::logic_error( "Unreachable state. Invalid Phase value. "
                            "This is likely a programming error." );
}

double Well2::injection_rate(const SummaryState& st, Phase phase_arg) const {
    if( !this->isInjector() ) return 0.0;
    const auto controls = this->injectionControls(st);

    const auto type = controls.injector_type;

    if( phase_arg == Phase::WATER && type != Well2::InjectorType::WATER ) return 0.0;
    if( phase_arg == Phase::OIL   && type != Well2::InjectorType::OIL   ) return 0.0;
    if( phase_arg == Phase::GAS   && type != Well2::InjectorType::GAS   ) return 0.0;

    return controls.surface_rate;
}


bool Well2::wellNameInWellNamePattern(const std::string& wellName, const std::string& wellNamePattern) {
    bool wellNameInPattern = false;
    if (util_fnmatch( wellNamePattern.c_str() , wellName.c_str()) == 0) {
        wellNameInPattern = true;
    }
    return wellNameInPattern;
}


ProductionControls Well2::productionControls(const SummaryState& st) const {
    if (this->isProducer()) {
        auto controls = this->production->controls(st, this->udq_undefined);
        controls.prediction_mode = this->predictionMode();
        return controls;
    } else
        throw std::logic_error("Trying to get production data from an injector");
}

Well2::InjectionControls Well2::injectionControls(const SummaryState& st) const {
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


int Well2::vfp_table_number() const {
    if (this->producer)
        return this->production->VFPTableNumber;
    else
        return this->injection->VFPTableNumber;
}

double Well2::alq_value() const {
    if (this->producer)
        return this->production->ALQValue;

    throw std::runtime_error("Can not ask for ALQ value in an injector");
}

double Well2::temperature() const {
    if (!this->producer)
        return this->injection->temperature;

    throw std::runtime_error("Can not ask for temperature in a producer");
}


std::string Well2::Status2String(Well2::Status enumValue) {
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


Well2::Status Well2::StatusFromString(const std::string& stringValue) {
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


const std::string Well2::InjectorType2String( Well2::InjectorType enumValue ) {
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

Well2::InjectorType Well2::InjectorTypeFromString( const std::string& stringValue ) {
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

const std::string Well2::InjectorCMode2String( InjectorCMode enumValue ) {
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


Well2::InjectorCMode Well2::InjectorCModeFromString(const std::string &stringValue) {
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

}
