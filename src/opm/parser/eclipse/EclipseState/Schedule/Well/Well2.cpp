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


Well2::Well2(const std::string& wname,
             const std::string& gname,
             std::size_t init_step,
             std::size_t insert_index,
             int headI,
             int headJ,
             double ref_depth,
             Phase phase,
             WellProducer::ControlModeEnum whistctl_cmode,
             WellCompletion::CompletionOrderEnum ordering):
    wname(wname),
    group_name(gname),
    init_step(init_step),
    insert_index(insert_index),
    headI(headI),
    headJ(headJ),
    ref_depth(ref_depth),
    phase(phase),
    ordering(ordering),
    status(WellCommon::SHUT),
    drainage_radius(ParserKeywords::WELSPECS::D_RADIUS::defaultValue),
    allow_cross_flow(DeckItem::to_bool(ParserKeywords::WELSPECS::CROSSFLOW::defaultValue)),
    automatic_shutin( ParserKeywords::WELSPECS::CROSSFLOW::defaultValue == "SHUT"),
    producer(true),
    guide_rate({true, -1, GuideRate::UNDEFINED,ParserKeywords::WGRUPCON::SCALING_FACTOR::defaultValue}),
    efficiency_factor(1.0),
    solvent_fraction(0.0),
    econ_limits(std::make_shared<WellEconProductionLimits>()),
    polymer_properties(std::make_shared<WellPolymerProperties>()),
    tracer_properties(std::make_shared<WellTracerProperties>()),
    connections(std::make_shared<WellConnections>(headI, headJ)),
    production(std::make_shared<WellProductionProperties>()),
    injection(std::make_shared<WellInjectionProperties>())
{
    WellProductionProperties * p = new WellProductionProperties(*this->production);
    p->whistctl_cmode = whistctl_cmode;
    this->production.reset( p );
}

bool Well2::updateEfficiencyFactor(double efficiency_factor) {
    if (this->efficiency_factor != efficiency_factor) {
        this->efficiency_factor = efficiency_factor;
        return true;
    }

    return false;
}

bool Well2::updateWellGuideRate(double guide_rate) {
    if (this->guide_rate.guide_rate != guide_rate) {
        this->guide_rate.guide_rate = guide_rate;
        return true;
    }

    return false;
}


bool Well2::updatePolymerProperties(std::shared_ptr<WellPolymerProperties> polymer_properties) {
    if (*this->polymer_properties != *polymer_properties) {
        this->polymer_properties = polymer_properties;
        this->producer = false;
        return true;
    }

    return false;
}


bool Well2::updateEconLimits(std::shared_ptr<WellEconProductionLimits> econ_limits) {
    if (*this->econ_limits != *econ_limits) {
        this->econ_limits = econ_limits;
        return true;
    }

    return false;
}


void Well2::switchToProducer() {
    auto p = std::make_shared<WellInjectionProperties>(this->getInjectionProperties());

    p->BHPLimit = 0;
    p->dropInjectionControl( Opm::WellInjector::BHP );
    this->updateInjection( p );
    this->updateProducer(true);
}


void Well2::switchToInjector() {
    auto p = std::make_shared<WellProductionProperties>(getProductionProperties());

    p->BHPLimit = 0;
    p->dropProductionControl( Opm::WellProducer::BHP );
    this->updateProduction( p );
    this->updateProducer( false );
}

bool Well2::updateInjection(std::shared_ptr<WellInjectionProperties> injection) {
    if (this->producer)
        this->switchToInjector( );

    if (*this->injection != *injection) {
        this->injection = injection;
        return true;
    }

    return false;
}

bool Well2::updateProduction(std::shared_ptr<WellProductionProperties> production) {
    if (!this->producer)
        this->switchToProducer( );

    if (*this->production != *production) {
        this->production = production;
        return true;
    }

    return false;
}

bool Well2::updateTracer(std::shared_ptr<WellTracerProperties> tracer_properties) {
    if (*this->tracer_properties != *tracer_properties) {
        this->tracer_properties = tracer_properties;
        return true;
    }

    return false;
}

bool Well2::updateWellGuideRate(bool available, double guide_rate, GuideRate::GuideRatePhaseEnum guide_phase, double scale_factor) {
    bool update = false;
    if (this->guide_rate.available != available) {
        this->guide_rate.available = available;
        update = true;
    }

    if(this->guide_rate.guide_rate != guide_rate) {
        this->guide_rate.guide_rate = guide_rate;
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


bool Well2::updateProducer(bool producer) {
    if (this->producer != producer) {
        this->producer = producer;
        return true;
    }
    return false;
}


bool Well2::updateGroup(const std::string& group) {
    if (this->group_name != group) {
        this->group_name = group;
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


bool Well2::updateStatus(WellCommon::StatusEnum status) {
    if (this->status != status) {
        this->status = status;
        return true;
    }

    return false;
}


bool Well2::updateRefDepth(double ref_depth) {
    if (this->ref_depth != ref_depth) {
        this->ref_depth = ref_depth;
        return true;
    }

    return false;
}

bool Well2::updateDrainageRadius(double drainage_radius) {
    if (this->drainage_radius != drainage_radius) {
        this->drainage_radius = drainage_radius;
        return true;
    }

    return false;
}


bool Well2::updateCrossFlow(bool allow_cross_flow) {
    if (this->allow_cross_flow != allow_cross_flow) {
        this->allow_cross_flow = allow_cross_flow;
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


bool Well2::updateConnections(const std::shared_ptr<WellConnections> connections) {
    if( this->ordering  == WellCompletion::TRACK)
        connections->orderConnections( this->headI, this->headJ );

    if (*this->connections != *connections) {
        this->connections = connections;
        //if (this->connections->allConnectionsShut()) {}
        // This status update breaks line 825 in ScheduleTests
        //this->status = WellCommon::StatusEnum::SHUT;
        return true;
    }

    return false;
}


bool Well2::updateSolventFraction(double solvent_fraction) {
    if (this->solvent_fraction != solvent_fraction) {
        this->solvent_fraction = solvent_fraction;
        return true;
    }

    return false;
}


bool Well2::handleCOMPSEGS(const DeckKeyword& keyword, const EclipseGrid& grid) {
    std::shared_ptr<WellConnections> new_connection_set( newConnectionsWithSegments(keyword, *this->connections, *this->segments , grid) );
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

bool Well2::isAvailableForGroupControl() const {
    return this->guide_rate.available;
}

double Well2::getGuideRate() const {
    return this->guide_rate.guide_rate;
};

GuideRate::GuideRatePhaseEnum Well2::getGuideRatePhase() const {
    return this->guide_rate.guide_phase;
};

double Well2::getGuideRateScalingFactor() const {
    return this->guide_rate.scale_factor;
};


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

const WellInjectionProperties& Well2::getInjectionProperties() const {
    return *this->injection;
}

WellCommon::StatusEnum Well2::getStatus() const {
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

bool Well2::handleWELOPEN(const DeckRecord& record, WellCompletion::StateEnum status) {

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
            c.setState( status );

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



bool Well2::canOpen() const {
    if (this->allow_cross_flow)
        return true;

    if (this->producer) {
        const auto& prod = *this->production;
        return (prod.WaterRate + prod.OilRate + prod.GasRate) != 0;
    } else {
        const auto& inj = *this->injection;
        return inj.surfaceInjectionRate != 0;
    }
}

WellCompletion::CompletionOrderEnum Well2::getWellConnectionOrdering() const {
    return this->ordering;
}


}

