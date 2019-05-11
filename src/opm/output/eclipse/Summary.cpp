/*
  Copyright 2016 Statoil ASA.

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

#include <algorithm>
#include <exception>
#include <initializer_list>
#include <iterator>
#include <numeric>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <ert/ecl/smspec_node.hpp>
#include <ert/ecl/ecl_smspec.hpp>
#include <ert/ecl/ecl_kw_magic.h>

#include <opm/common/OpmLog/OpmLog.hpp>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperty.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQInput.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/UDQ/UDQContext.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellProductionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/SummaryState.hpp>

#include <opm/output/eclipse/Summary.hpp>
#include <opm/output/eclipse/RegionCache.hpp>

namespace {
    struct SegmentResultDescriptor
    {
        std::string vector;
        std::string well;
        std::size_t segNumber;
    };

    std::vector<std::string> requiredRestartVectors()
    {
        return {
            "OPR", "WPR", "GPR", "VPR",
            "OPT", "WPT", "GPT", "VPT",
            "WIR", "GIR",
            "WIT", "GIT",
            "WCT", "GOR",
            "OPTH", "WPTH", "GPTH",
            "WITH", "GITH",
        };
    }

    std::vector<std::pair<std::string, std::string>>
    requiredRestartVectors(const ::Opm::Schedule& sched)
    {
        auto entities = std::vector<std::pair<std::string, std::string>>{};

        const auto& vectors = requiredRestartVectors();

        auto makeEntities = [&vectors, &entities]
            (const char cat, const std::string& name)
        {
            for (const auto& vector : vectors) {
                entities.emplace_back(cat + vector, name);
            }
        };

        for (const auto& well_name : sched.wellNames()) {
            makeEntities('W', well_name);

            entities.emplace_back("WBHP" , well_name);
            entities.emplace_back("WGVIR", well_name);
            entities.emplace_back("WWVIR", well_name);
        }

        for (const auto* grp : sched.getGroups()) {
            const auto& grp_name = grp->name();

            if (grp_name != "FIELD") {
                makeEntities('G', grp_name);
            }
        }

        makeEntities('F', "FIELD");

        return entities;
    }

    std::vector<SegmentResultDescriptor>
    requiredSegmentVectors(const ::Opm::Schedule& sched)
    {
        using SRD = SegmentResultDescriptor;
        auto ret  = std::vector<SRD>{};

        auto makeVectors =
            [&ret](const std::string& well,
                   const std::size_t  segNumber) -> void
        {
            ret.push_back(SRD{"SOFR", well, segNumber});
            ret.push_back(SRD{"SGFR", well, segNumber});
            ret.push_back(SRD{"SWFR", well, segNumber});
            ret.push_back(SRD{"SPR" , well, segNumber});
        };

        const auto last_timestep = sched.getTimeMap().last();

        for (const auto* well : sched.getWells()) {
            if (! well->isMultiSegment(last_timestep)) {
                // Don't allocate MS summary vectors for non-MS wells.
                continue;
            }

            const auto& wname = well->name();
            const auto  nSeg  =
                well->getWellSegments(last_timestep).size();

            for (auto segID = 0*nSeg; segID < nSeg; ++segID) {
                makeVectors(wname, segID + 1); // One-based
            }
        }

        return ret;
    }

    std::string genKey(const std::string& vector,
                       const std::string& entity)
    {
        return (entity == "FIELD")
             ? vector
             : vector + ':' + entity;
    }

    std::string genKey(const SegmentResultDescriptor& segRes)
    {
        return segRes.vector + ':' + segRes.well +
            ':' + std::to_string(segRes.segNumber);
    }

    std::unique_ptr<ecl::smspec_node>
    makeRestartVectorSMSPEC(const std::string& vector,
                            const std::string& entity)
    {
        return std::unique_ptr<ecl::smspec_node>( new ecl::smspec_node(0, vector.c_str(), entity.c_str(), "UNIT", 0.0f, ":"));
    }

    std::unique_ptr<ecl::smspec_node>
    makeRestartVectorSMSPEC(const SegmentResultDescriptor& segRes)
    {
        return std::unique_ptr<ecl::smspec_node>(new ecl::smspec_node(0, segRes.vector.c_str(), segRes.well.c_str(), static_cast<int>(segRes.segNumber), "UNIT", 0.0f, ":"));
    }
} // namespace Anonymous

/*
 * This class takes simulator state and parser-provided information and
 * orchestrates ert to write simulation results as requested by the SUMMARY
 * section in eclipse. The implementation is somewhat compact as a lot of the
 * requested output may be similar-but-not-quite. Through various techniques
 * the compiler writes a lot of this code for us.
 */

namespace Opm {

namespace {

using rt = data::Rates::opt;
using measure = UnitSystem::measure;
constexpr const bool injector = true;
constexpr const bool producer = false;
constexpr const bool polymer = true;

/* Some numerical value with its unit tag embedded to enable caller to apply
 * unit conversion. This removes a lot of boilerplate. ad-hoc solution to poor
 * unit support in general.
 */
measure div_unit( measure denom, measure div ) {
    if( denom == measure::gas_surface_rate &&
        div   == measure::liquid_surface_rate )
        return measure::gas_oil_ratio;

    if( denom == measure::liquid_surface_rate &&
        div   == measure::gas_surface_rate )
        return measure::oil_gas_ratio;

    if( denom == measure::liquid_surface_rate &&
        div   == measure::liquid_surface_rate )
        return measure::water_cut;

    if( denom == measure::liquid_surface_rate &&
        div   == measure::time )
        return measure::liquid_surface_volume;

    if( denom == measure::gas_surface_rate &&
        div   == measure::time )
        return measure::gas_surface_volume;

    if( denom == measure::mass_rate &&
        div   == measure::time )
        return measure::mass;

    return measure::identity;
}

measure mul_unit( measure lhs, measure rhs ) {
    if( lhs == rhs ) return lhs;

    if( ( lhs == measure::liquid_surface_rate && rhs == measure::time ) ||
        ( rhs == measure::liquid_surface_rate && lhs == measure::time ) )
        return measure::liquid_surface_volume;

    if( ( lhs == measure::gas_surface_rate && rhs == measure::time ) ||
        ( rhs == measure::gas_surface_rate && lhs == measure::time ) )
        return measure::gas_surface_volume;

    if( ( lhs == measure::rate && rhs == measure::time ) ||
        ( rhs == measure::rate && lhs == measure::time ) )
        return measure::volume;

    if(  lhs == measure::mass_rate && rhs == measure::time)
        return measure::mass;

    return lhs;
}

struct quantity {
    double value;
    UnitSystem::measure unit;

    quantity operator+( const quantity& rhs ) const {
        assert( this->unit == rhs.unit );
        return { this->value + rhs.value, this->unit };
    }

    quantity operator*( const quantity& rhs ) const {
        return { this->value * rhs.value, mul_unit( this->unit, rhs.unit ) };
    }

    quantity operator/( const quantity& rhs ) const {
        const auto res_unit = div_unit( this->unit, rhs.unit );

        if( rhs.value == 0 ) return { 0.0, res_unit };
        return { this->value / rhs.value, res_unit };
    }

    quantity operator/( double divisor ) const {
        if( divisor == 0 ) return { 0.0, this->unit };
        return { this->value / divisor , this->unit };
    }

    quantity& operator/=( double divisor ) {
        if( divisor == 0 )
            this->value = 0;
        else
            this->value /= divisor;

        return *this;
    }


    quantity operator-( const quantity& rhs) const {
        return { this->value - rhs.value, this->unit };
    }
};

/*
 * All functions must have the same parameters, so they're gathered in a struct
 * and functions use whatever information they care about.
 *
 * schedule_wells are wells from the deck, provided by opm-parser. active_index
 * is the index of the block in question. wells is simulation data.
 */
struct fn_args {
    const std::vector< const Well* >& schedule_wells;
    double duration;
    const int sim_step;
    int  num;
    const data::Wells& wells;
    const out::RegionCache& regionCache;
    const EclipseGrid& grid;
    const std::vector< std::pair< std::string, double > > eff_factors;
};

/* Since there are several enums in opm scattered about more-or-less
 * representing the same thing. Since functions use template parameters to
 * expand into the actual implementations we need a static way to determine
 * what unit to tag the return value with.
 */
template< rt > constexpr
measure rate_unit() { return measure::liquid_surface_rate; }
template< Phase > constexpr
measure rate_unit() { return measure::liquid_surface_rate; }

template<> constexpr
measure rate_unit< rt::gas >() { return measure::gas_surface_rate; }
template<> constexpr
measure rate_unit< Phase::GAS >() { return measure::gas_surface_rate; }

template<> constexpr
measure rate_unit< rt::solvent >() { return measure::gas_surface_rate; }

template<> constexpr
measure rate_unit< rt::reservoir_water >() { return measure::rate; }

template<> constexpr
measure rate_unit< rt::reservoir_oil >() { return measure::rate; }

template<> constexpr
measure rate_unit< rt::reservoir_gas >() { return measure::rate; }

template<> constexpr
measure rate_unit < rt::productivity_index_water > () { return measure::liquid_productivity_index; }

template<> constexpr
measure rate_unit < rt::productivity_index_oil > () { return measure::liquid_productivity_index; }

template<> constexpr
measure rate_unit < rt::productivity_index_gas > () { return measure::gas_productivity_index; }

template<> constexpr
measure rate_unit< rt::well_potential_water >() { return measure::liquid_surface_rate; }

template<> constexpr
measure rate_unit< rt::well_potential_oil >() { return measure::liquid_surface_rate; }

template<> constexpr
measure rate_unit< rt::well_potential_gas >() { return measure::gas_surface_rate; }

double efac( const std::vector<std::pair<std::string,double>>& eff_factors, const std::string& name ) {
    auto it = std::find_if( eff_factors.begin(), eff_factors.end(),
                            [&] ( const std::pair< std::string, double > elem )
                            { return elem.first == name; }
                          );

    return (it != eff_factors.end()) ? it->second : 1;
}

template< rt phase, bool injection = true, bool polymer = false >
inline quantity rate( const fn_args& args ) {
    double sum = 0.0;

    for( const auto* sched_well : args.schedule_wells ) {
        const auto& name = sched_well->name();
        if( args.wells.count( name ) == 0 ) continue;

        double eff_fac = efac( args.eff_factors, name );

        double concentration = polymer
                             ? sched_well->getPolymerProperties( args.sim_step ).m_polymerConcentration
                             : 1;

        const auto v = args.wells.at(name).rates.get(phase, 0.0) * eff_fac * concentration;

        if( ( v > 0 ) == injection )
            sum += v;
    }

    if( !injection ) sum *= -1;

    if( polymer ) return { sum, measure::mass_rate };
    return { sum, rate_unit< phase >() };
}

template< bool injection >
inline quantity flowing( const fn_args& args ) {
    const auto& wells = args.wells;
    const auto ts = args.sim_step;
    auto pred = [&wells,ts]( const Well* w ) {
        const auto& name = w->name();
        return w->isInjector( ts ) == injection
            && wells.count( name ) > 0
            && wells.at( name ).flowing();
    };

    return { double( std::count_if( args.schedule_wells.begin(),
                                    args.schedule_wells.end(),
                                    pred ) ),
             measure::identity };
}

template< rt phase, bool injection = true, bool polymer = false >
inline quantity crate( const fn_args& args ) {
    const quantity zero = { 0, rate_unit< phase >() };
    // The args.num value is the literal value which will go to the
    // NUMS array in the eclispe SMSPEC file; the values in this array
    // are offset 1 - whereas we need to use this index here to look
    // up a completion with offset 0.
    const size_t global_index = args.num - 1;
    if( args.schedule_wells.empty() ) return zero;

    const auto& well = args.schedule_wells.front();
    const auto& name = well->name();
    if( args.wells.count( name ) == 0 ) return zero;

    const auto& well_data = args.wells.at( name );
    const auto& completion = std::find_if( well_data.connections.begin(),
                                           well_data.connections.end(),
                                           [=]( const data::Connection& c ) {
                                                return c.index == global_index;
                                           } );

    if( completion == well_data.connections.end() ) return zero;

    double eff_fac = efac( args.eff_factors, name );
    double concentration = polymer
                           ? well->getPolymerProperties( args.sim_step ).m_polymerConcentration
                           : 1;

    auto v = completion->rates.get( phase, 0.0 ) * eff_fac * concentration;
    if( ( v > 0 ) != injection ) return zero;
    if( !injection ) v *= -1;

    if( polymer ) return { v, measure::mass_rate };
    return { v, rate_unit< phase >() };
}

template< rt phase, bool polymer = false >
inline quantity srate( const fn_args& args ) {
    const quantity zero = { 0, rate_unit< phase >() };
    // The args.num value is the literal value which will go to the
    // NUMS array in the eclispe SMSPEC file; the values in this array
    // are offset 1 - whereas we need to use this index here to look
    // up a completion with offset 0.
    const size_t segNumber = args.num;
    if( args.schedule_wells.empty() ) return zero;

    const auto& well = args.schedule_wells.front();
    const auto& name = well->name();
    if( args.wells.count( name ) == 0 ) return zero;

    const auto& well_data = args.wells.at( name );

    const auto& segment = well_data.segments.find(segNumber);

    if( segment == well_data.segments.end() ) return zero;

    double eff_fac = efac( args.eff_factors, name );
    double concentration = polymer
                           ? well->getPolymerProperties( args.sim_step ).m_polymerConcentration
                           : 1;

    auto v = segment->second.rates.get( phase, 0.0 ) * eff_fac * concentration;
    //switch sign of rate - opposite convention in flow vs eclipse
    v *= -1;

    if( polymer ) return { v, measure::mass_rate };
    return { v, rate_unit< phase >() };
}

inline quantity trans_factors ( const fn_args& args ) {
    const quantity zero = { 0, measure::transmissibility };

    if( args.schedule_wells.empty() ) return zero;
    // Like completion rate we need to look
    // up a connection with offset 0.
    const size_t global_index = args.num - 1;

    const auto& well = args.schedule_wells.front();
    const auto& name = well->name();
    if( args.wells.count( name ) == 0 ) return zero;

    const auto& grid = args.grid;
    const auto& connections = well->getConnections(args.sim_step);

    const auto& connection = std::find_if(
        connections.begin(),
        connections.end(),
        [=]( const Opm::Connection& c ) {
            return grid.getGlobalIndex(c.getI(), c.getJ(), c.getK()) == global_index;
        } );

    if( connection == connections.end() ) return zero;

    const auto& v = connection->CF() * connection->wellPi();
    return { v, measure::transmissibility };
}

inline quantity spr ( const fn_args& args ) {
    const quantity zero = { 0, measure::pressure };

    if( args.schedule_wells.empty() ) return zero;
    // Like completion rate we need to look
    // up a connection with offset 0.
    const size_t segNumber = args.num;
    if( args.schedule_wells.empty() ) return zero;

    const auto& well = args.schedule_wells.front();
    const auto& name = well->name();
    if( args.wells.count( name ) == 0 ) return zero;

    const auto& well_data = args.wells.at( name );

    const auto& segment = well_data.segments.find(segNumber);

    if( segment == well_data.segments.end() ) return zero;


    const auto& v = segment->second.pressure;
    return { v, measure::pressure };
}


inline quantity bhp( const fn_args& args ) {
    const quantity zero = { 0, measure::pressure };
    if( args.schedule_wells.empty() ) return zero;

    const auto p = args.wells.find( args.schedule_wells.front()->name() );
    if( p == args.wells.end() ) return zero;

    return { p->second.bhp, measure::pressure };
}

inline quantity thp( const fn_args& args ) {
    const quantity zero = { 0, measure::pressure };
    if( args.schedule_wells.empty() ) return zero;

    const auto p = args.wells.find( args.schedule_wells.front()->name() );
    if( p == args.wells.end() ) return zero;

    return { p->second.thp, measure::pressure };
}

inline quantity bhp_history( const fn_args& args ) {
    if( args.schedule_wells.empty() ) return { 0.0, measure::pressure };

    const Well* sched_well = args.schedule_wells.front();

    double bhp_hist;
    if ( sched_well->isProducer( args.sim_step ) )
        bhp_hist = sched_well->getProductionProperties( args.sim_step ).BHPH;
    else
        bhp_hist = sched_well->getInjectionProperties( args.sim_step ).BHPH;

    return { bhp_hist, measure::pressure };
}

inline quantity thp_history( const fn_args& args ) {
    if( args.schedule_wells.empty() ) return { 0.0, measure::pressure };

    const Well* sched_well = args.schedule_wells.front();

    double thp_hist;
    if ( sched_well->isProducer( args.sim_step ) )
       thp_hist = sched_well->getProductionProperties( args.sim_step ).THPH;
    else
       thp_hist = sched_well->getInjectionProperties( args.sim_step ).THPH;

    return { thp_hist, measure::pressure };
}

template< Phase phase >
inline quantity production_history( const fn_args& args ) {
    /*
     * For well data, looking up historical rates (both for production and
     * injection) before simulation actually starts is impossible and
     * nonsensical. We therefore default to writing zero (which is what eclipse
     * seems to do as well).
     */

    double sum = 0.0;
    for( const Well* sched_well : args.schedule_wells ){

        double eff_fac = efac( args.eff_factors, sched_well->name() );
        sum += sched_well->production_rate( phase, args.sim_step ) * eff_fac;
    }


    return { sum, rate_unit< phase >() };
}

template< Phase phase >
inline quantity injection_history( const fn_args& args ) {

    double sum = 0.0;
    for( const Well* sched_well : args.schedule_wells ){

        double eff_fac = efac( args.eff_factors, sched_well->name() );
        sum += sched_well->injection_rate( phase, args.sim_step ) * eff_fac;
    }


    return { sum, rate_unit< phase >() };
}

inline quantity res_vol_production_target( const fn_args& args ) {

    double sum = 0.0;
    for( const Well* sched_well : args.schedule_wells )
        if (sched_well->getProductionProperties(args.sim_step).predictionMode)
            sum += sched_well->getProductionProperties( args.sim_step ).ResVRate;

    return { sum, measure::rate };
}

/*
 * A small DSL, really poor man's function composition, to avoid massive
 * repetition when declaring the handlers for each individual keyword. bin_op
 * and its aliases will apply the pair of functions F and G that all take const
 * fn_args& and return quantity, making them composable.
 */
template< typename F, typename G, typename Op >
struct bin_op {
    bin_op( F fn, G gn = {} ) : f( fn ), g( gn ) {}
    quantity operator()( const fn_args& args ) const {
        return Op()( f( args ), g( args ) );
    }

    private:
        F f;
        G g;
};

inline quantity duration( const fn_args& args ) {
    return { args.duration, measure::time };
}

template<rt phase , bool injection>
quantity region_rate( const fn_args& args ) {
    double sum = 0;
    const auto& well_connections = args.regionCache.connections( args.num );

    for (const auto& pair : well_connections) {

        double eff_fac = efac( args.eff_factors, pair.first );

        double rate = args.wells.get( pair.first , pair.second , phase ) * eff_fac;

        // We are asking for the production rate in an injector - or
        // opposite. We just clamp to zero.
        if ((rate > 0) != injection)
            rate = 0;

        sum += rate;
    }

    if( injection )
        return { sum, rate_unit< phase >() };
    else
        return { -sum, rate_unit< phase >() };
}

template < rt phase, bool outputProducer = true, bool outputInjector = true>
inline quantity potential_rate( const fn_args& args ) {
    double sum = 0.0;

    for( const auto* sched_well : args.schedule_wells ) {
        const auto& name = sched_well->name();
        if( args.wells.count( name ) == 0 ) continue;

        if (sched_well->isInjector(args.sim_step) && outputInjector) {
	    const auto v = args.wells.at(name).rates.get(phase, 0.0);
	    sum += v;
	}
	else if (sched_well->isProducer(args.sim_step) && outputProducer) {
	    const auto v = args.wells.at(name).rates.get(phase, 0.0);
	    sum += v;
	}
    }

    return { sum, rate_unit< phase >() };
}

template< typename F, typename G >
auto mul( F f, G g ) -> bin_op< F, G, std::multiplies< quantity > >
{ return { f, g }; }

template< typename F, typename G >
auto sum( F f, G g ) -> bin_op< F, G, std::plus< quantity > >
{ return { f, g }; }

template< typename F, typename G >
auto div( F f, G g ) -> bin_op< F, G, std::divides< quantity > >
{ return { f, g }; }

template< typename F, typename G >
auto sub( F f, G g ) -> bin_op< F, G, std::minus< quantity > >
{ return { f, g }; }

using ofun = std::function< quantity( const fn_args& ) >;

static const std::unordered_map< std::string, ofun > funs = {
    { "WWIR", rate< rt::wat, injector > },
    { "WOIR", rate< rt::oil, injector > },
    { "WGIR", rate< rt::gas, injector > },
    { "WNIR", rate< rt::solvent, injector > },
    { "WCIR", rate< rt::wat, injector, polymer > },

    { "WWIT", mul( rate< rt::wat, injector >, duration ) },
    { "WOIT", mul( rate< rt::oil, injector >, duration ) },
    { "WGIT", mul( rate< rt::gas, injector >, duration ) },
    { "WNIT", mul( rate< rt::solvent, injector >, duration ) },
    { "WCIT", mul( rate< rt::wat, injector, polymer >, duration ) },
    { "WVIT", mul( sum( sum( rate< rt::reservoir_water, injector >, rate< rt::reservoir_oil, injector > ),
                        rate< rt::reservoir_gas, injector > ), duration ) },

    { "WWPR", rate< rt::wat, producer > },
    { "WOPR", rate< rt::oil, producer > },
    { "WGPR", rate< rt::gas, producer > },
    { "WNPR", rate< rt::solvent, producer > },

    { "WGPRS", rate< rt::dissolved_gas, producer > },
    { "WGPRF", sub( rate< rt::gas, producer >, rate< rt::dissolved_gas, producer > ) },
    { "WOPRS", rate< rt::vaporized_oil, producer > },
    { "WOPRF", sub (rate < rt::oil, producer >, rate< rt::vaporized_oil, producer > )  },
    { "WVPR", sum( sum( rate< rt::reservoir_water, producer >, rate< rt::reservoir_oil, producer > ),
                   rate< rt::reservoir_gas, producer > ) },
    { "WGVPR", rate< rt::reservoir_gas, producer > },

    { "WLPR", sum( rate< rt::wat, producer >, rate< rt::oil, producer > ) },
    { "WWPT", mul( rate< rt::wat, producer >, duration ) },
    { "WOPT", mul( rate< rt::oil, producer >, duration ) },
    { "WGPT", mul( rate< rt::gas, producer >, duration ) },
    { "WNPT", mul( rate< rt::solvent, producer >, duration ) },
    { "WLPT", mul( sum( rate< rt::wat, producer >, rate< rt::oil, producer > ),
                   duration ) },

    { "WGPTS", mul( rate< rt::dissolved_gas, producer >, duration )},
    { "WGPTF", sub( mul( rate< rt::gas, producer >, duration ),
                        mul( rate< rt::dissolved_gas, producer >, duration ))},
    { "WOPTS", mul( rate< rt::vaporized_oil, producer >, duration )},
    { "WOPTF", sub( mul( rate< rt::oil, producer >, duration ),
                        mul( rate< rt::vaporized_oil, producer >, duration ))},
    { "WVPT", mul( sum( sum( rate< rt::reservoir_water, producer >, rate< rt::reservoir_oil, producer > ),
                        rate< rt::reservoir_gas, producer > ), duration ) },

    { "WWCT", div( rate< rt::wat, producer >,
                   sum( rate< rt::wat, producer >, rate< rt::oil, producer > ) ) },
    { "GWCT", div( rate< rt::wat, producer >,
                   sum( rate< rt::wat, producer >, rate< rt::oil, producer > ) ) },
    { "WGOR", div( rate< rt::gas, producer >, rate< rt::oil, producer > ) },
    { "GGOR", div( rate< rt::gas, producer >, rate< rt::oil, producer > ) },
    { "WGLR", div( rate< rt::gas, producer >,
                   sum( rate< rt::wat, producer >, rate< rt::oil, producer > ) ) },

    { "WBHP", bhp },
    { "WTHP", thp },
    { "WVPRT", res_vol_production_target },

    { "GWIR", rate< rt::wat, injector > },
    { "WGVIR", rate< rt::reservoir_gas, injector >},
    { "WWVIR", rate< rt::reservoir_water, injector >},
    { "GOIR", rate< rt::oil, injector > },
    { "GGIR", rate< rt::gas, injector > },
    { "GNIR", rate< rt::solvent, injector > },
    { "GCIR", rate< rt::wat, injector, polymer > },
    { "GVIR", sum( sum( rate< rt::reservoir_water, injector >, rate< rt::reservoir_oil, injector > ),
                        rate< rt::reservoir_gas, injector > ) },
    { "GWIT", mul( rate< rt::wat, injector >, duration ) },
    { "GOIT", mul( rate< rt::oil, injector >, duration ) },
    { "GGIT", mul( rate< rt::gas, injector >, duration ) },
    { "GNIT", mul( rate< rt::solvent, injector >, duration ) },
    { "GCIT", mul( rate< rt::wat, injector, polymer >, duration ) },
    { "GVIT", mul( sum( sum( rate< rt::reservoir_water, injector >, rate< rt::reservoir_oil, injector > ),
                        rate< rt::reservoir_gas, injector > ), duration ) },

    { "GWPR", rate< rt::wat, producer > },
    { "GOPR", rate< rt::oil, producer > },
    { "GGPR", rate< rt::gas, producer > },
    { "GNPR", rate< rt::solvent, producer > },
    { "GOPRS", rate< rt::vaporized_oil, producer > },
    { "GOPRF", sub (rate < rt::oil, producer >, rate< rt::vaporized_oil, producer > ) },
    { "GLPR", sum( rate< rt::wat, producer >, rate< rt::oil, producer > ) },
    { "GVPR", sum( sum( rate< rt::reservoir_water, producer >, rate< rt::reservoir_oil, producer > ),
                        rate< rt::reservoir_gas, producer > ) },

    { "GWPT", mul( rate< rt::wat, producer >, duration ) },
    { "GOPT", mul( rate< rt::oil, producer >, duration ) },
    { "GGPT", mul( rate< rt::gas, producer >, duration ) },
    { "GNPT", mul( rate< rt::solvent, producer >, duration ) },
    { "GOPTS", mul( rate< rt::vaporized_oil, producer >, duration ) },
    { "GOPTF", mul( sub (rate < rt::oil, producer >,
                         rate< rt::vaporized_oil, producer > ),
                    duration ) },
    { "GLPT", mul( sum( rate< rt::wat, producer >, rate< rt::oil, producer > ),
                   duration ) },
    { "GVPT", mul( sum( sum( rate< rt::reservoir_water, producer >, rate< rt::reservoir_oil, producer > ),
                        rate< rt::reservoir_gas, producer > ), duration ) },
    // Group potential
    { "GWPP", potential_rate< rt::well_potential_water , true, false>},
    { "GOPP", potential_rate< rt::well_potential_oil , true, false>},
    { "GGPP", potential_rate< rt::well_potential_gas , true, false>},
    { "GWPI", potential_rate< rt::well_potential_water , false, true>},
    { "GOPI", potential_rate< rt::well_potential_oil , false, true>},
    { "GGPI", potential_rate< rt::well_potential_gas , false, true>},

    { "WWPRH", production_history< Phase::WATER > },
    { "WOPRH", production_history< Phase::OIL > },
    { "WGPRH", production_history< Phase::GAS > },
    { "WLPRH", sum( production_history< Phase::WATER >,
                    production_history< Phase::OIL > ) },

    { "WWPTH", mul( production_history< Phase::WATER >, duration ) },
    { "WOPTH", mul( production_history< Phase::OIL >, duration ) },
    { "WGPTH", mul( production_history< Phase::GAS >, duration ) },
    { "WLPTH", mul( sum( production_history< Phase::WATER >,
                         production_history< Phase::OIL > ),
                    duration ) },

    { "WWIRH", injection_history< Phase::WATER > },
    { "WOIRH", injection_history< Phase::OIL > },
    { "WGIRH", injection_history< Phase::GAS > },
    { "WWITH", mul( injection_history< Phase::WATER >, duration ) },
    { "WOITH", mul( injection_history< Phase::OIL >, duration ) },
    { "WGITH", mul( injection_history< Phase::GAS >, duration ) },

    /* From our point of view, injectors don't have water cuts and div/sum will return 0.0 */
    { "WWCTH", div( production_history< Phase::WATER >,
                    sum( production_history< Phase::WATER >,
                         production_history< Phase::OIL > ) ) },

    /* We do not support mixed injections, and gas/oil is undefined when oil is
     * zero (i.e. pure gas injector), so always output 0 if this is an injector
     */
    { "WGORH", div( production_history< Phase::GAS >,
                    production_history< Phase::OIL > ) },
    { "WGLRH", div( production_history< Phase::GAS >,
                    sum( production_history< Phase::WATER >,
                         production_history< Phase::OIL > ) ) },

    { "WTHPH", thp_history },
    { "WBHPH", bhp_history },

    { "GWPRH", production_history< Phase::WATER > },
    { "GOPRH", production_history< Phase::OIL > },
    { "GGPRH", production_history< Phase::GAS > },
    { "GLPRH", sum( production_history< Phase::WATER >,
                    production_history< Phase::OIL > ) },
    { "GWIRH", injection_history< Phase::WATER > },
    { "GOIRH", injection_history< Phase::OIL > },
    { "GGIRH", injection_history< Phase::GAS > },
    { "GGORH", div( production_history< Phase::GAS >,
                    production_history< Phase::OIL > ) },
    { "GWCTH", div( production_history< Phase::WATER >,
                    sum( production_history< Phase::WATER >,
                         production_history< Phase::OIL > ) ) },

    { "GWPTH", mul( production_history< Phase::WATER >, duration ) },
    { "GOPTH", mul( production_history< Phase::OIL >, duration ) },
    { "GGPTH", mul( production_history< Phase::GAS >, duration ) },
    { "GGPRF", sub( rate < rt::gas, producer >, rate< rt::dissolved_gas, producer > )},
    { "GGPRS", rate< rt::dissolved_gas, producer> },
    { "GGPTF", mul( sub( rate < rt::gas, producer >, rate< rt::dissolved_gas, producer > ),
                         duration ) },
    { "GGPTS", mul( rate< rt::dissolved_gas, producer>, duration ) },
    { "GGLR",  div( rate< rt::gas, producer >,
                    sum( rate< rt::wat, producer >,
                         rate< rt::oil, producer > ) ) },
    { "GGLRH", div( production_history< Phase::GAS >,
                    sum( production_history< Phase::WATER >,
                         production_history< Phase::OIL > ) ) },
    { "GLPTH", mul( sum( production_history< Phase::WATER >,
                         production_history< Phase::OIL > ),
                    duration ) },
    { "GWITH", mul( injection_history< Phase::WATER >, duration ) },
    { "GGITH", mul( injection_history< Phase::GAS >, duration ) },
    { "GMWIN", flowing< injector > },
    { "GMWPR", flowing< producer > },

    { "GVPRT", res_vol_production_target },

    { "CWIR", crate< rt::wat, injector > },
    { "CGIR", crate< rt::gas, injector > },
    { "CCIR", crate< rt::wat, injector, polymer > },
    { "CWIT", mul( crate< rt::wat, injector >, duration ) },
    { "CGIT", mul( crate< rt::gas, injector >, duration ) },
    { "CNIT", mul( crate< rt::solvent, injector >, duration ) },

    { "CWPR", crate< rt::wat, producer > },
    { "COPR", crate< rt::oil, producer > },
    { "CGPR", crate< rt::gas, producer > },
    // Minus for injection rates and pluss for production rate
    { "CNFR", sub( crate< rt::solvent, producer >, crate<rt::solvent, injector >) },
    { "CWPT", mul( crate< rt::wat, producer >, duration ) },
    { "COPT", mul( crate< rt::oil, producer >, duration ) },
    { "CGPT", mul( crate< rt::gas, producer >, duration ) },
    { "CNPT", mul( crate< rt::solvent, producer >, duration ) },
    { "CCIT", mul( crate< rt::wat, injector, polymer >, duration ) },
    { "CTFAC", trans_factors },

    { "FWPR", rate< rt::wat, producer > },
    { "FOPR", rate< rt::oil, producer > },
    { "FGPR", rate< rt::gas, producer > },
    { "FNPR", rate< rt::solvent, producer > },
    { "FVPR", sum( sum( rate< rt::reservoir_water, producer>, rate< rt::reservoir_oil, producer >),
                   rate< rt::reservoir_gas, producer>)},
    { "FGPRS", rate< rt::dissolved_gas, producer > },
    { "FGPRF", sub( rate< rt::gas, producer >, rate< rt::dissolved_gas, producer > ) },
    { "FOPRS", rate< rt::vaporized_oil, producer > },
    { "FOPRF", sub (rate < rt::oil, producer >, rate< rt::vaporized_oil, producer > ) },

    { "FLPR", sum( rate< rt::wat, producer >, rate< rt::oil, producer > ) },
    { "FWPT", mul( rate< rt::wat, producer >, duration ) },
    { "FOPT", mul( rate< rt::oil, producer >, duration ) },
    { "FGPT", mul( rate< rt::gas, producer >, duration ) },
    { "FNPT", mul( rate< rt::solvent, producer >, duration ) },
    { "FLPT", mul( sum( rate< rt::wat, producer >, rate< rt::oil, producer > ),
                   duration ) },
    { "FVPT", mul(sum (sum( rate< rt::reservoir_water, producer>, rate< rt::reservoir_oil, producer >),
                       rate< rt::reservoir_gas, producer>), duration)},
    { "FGPTS", mul( rate< rt::dissolved_gas, producer > , duration )},
    { "FGPTF", mul( sub( rate< rt::gas, producer >, rate< rt::dissolved_gas, producer > ), duration )},
    { "FOPTS", mul( rate< rt::vaporized_oil, producer >, duration ) },
    { "FOPTF", mul( sub (rate < rt::oil, producer >,
                         rate< rt::vaporized_oil, producer > ),
                    duration ) },

    { "FWIR", rate< rt::wat, injector > },
    { "FOIR", rate< rt::oil, injector > },
    { "FGIR", rate< rt::gas, injector > },
    { "FNIR", rate< rt::solvent, injector > },
    { "FCIR", rate< rt::wat, injector, polymer > },
    { "FVIR", sum( sum( rate< rt::reservoir_water, injector>, rate< rt::reservoir_oil, injector >),
                   rate< rt::reservoir_gas, injector>)},

    { "FLIR", sum( rate< rt::wat, injector >, rate< rt::oil, injector > ) },
    { "FWIT", mul( rate< rt::wat, injector >, duration ) },
    { "FOIT", mul( rate< rt::oil, injector >, duration ) },
    { "FGIT", mul( rate< rt::gas, injector >, duration ) },
    { "FNIT", mul( rate< rt::solvent, injector >, duration ) },
    { "FCIT", mul( rate< rt::wat, injector, polymer >, duration ) },
    { "FLIT", mul( sum( rate< rt::wat, injector >, rate< rt::oil, injector > ),
                   duration ) },
    { "FVIT", mul( sum( sum( rate< rt::reservoir_water, injector>, rate< rt::reservoir_oil, injector >),
                   rate< rt::reservoir_gas, injector>), duration)},
    // Field potential
    { "FWPP", potential_rate< rt::well_potential_water , true, false>},
    { "FOPP", potential_rate< rt::well_potential_oil , true, false>},
    { "FGPP", potential_rate< rt::well_potential_gas , true, false>},
    { "FWPI", potential_rate< rt::well_potential_water , false, true>},
    { "FOPI", potential_rate< rt::well_potential_oil , false, true>},
    { "FGPI", potential_rate< rt::well_potential_gas , false, true>},


    { "FWPRH", production_history< Phase::WATER > },
    { "FOPRH", production_history< Phase::OIL > },
    { "FGPRH", production_history< Phase::GAS > },
    { "FLPRH", sum( production_history< Phase::WATER >,
                    production_history< Phase::OIL > ) },
    { "FWPTH", mul( production_history< Phase::WATER >, duration ) },
    { "FOPTH", mul( production_history< Phase::OIL >, duration ) },
    { "FGPTH", mul( production_history< Phase::GAS >, duration ) },
    { "FLPTH", mul( sum( production_history< Phase::WATER >,
                         production_history< Phase::OIL > ),
                    duration ) },

    { "FWIRH", injection_history< Phase::WATER > },
    { "FOIRH", injection_history< Phase::OIL > },
    { "FGIRH", injection_history< Phase::GAS > },
    { "FWITH", mul( injection_history< Phase::WATER >, duration ) },
    { "FOITH", mul( injection_history< Phase::OIL >, duration ) },
    { "FGITH", mul( injection_history< Phase::GAS >, duration ) },

    { "FWCT", div( rate< rt::wat, producer >,
                   sum( rate< rt::wat, producer >, rate< rt::oil, producer > ) ) },
    { "FGOR", div( rate< rt::gas, producer >, rate< rt::oil, producer > ) },
    { "FGLR", div( rate< rt::gas, producer >,
                   sum( rate< rt::wat, producer >, rate< rt::oil, producer > ) ) },
    { "FWCTH", div( production_history< Phase::WATER >,
                    sum( production_history< Phase::WATER >,
                         production_history< Phase::OIL > ) ) },
    { "FGORH", div( production_history< Phase::GAS >,
                    production_history< Phase::OIL > ) },
    { "FGLRH", div( production_history< Phase::GAS >,
                    sum( production_history< Phase::WATER >,
                         production_history< Phase::OIL > ) ) },
    { "FMWIN", flowing< injector > },
    { "FMWPR", flowing< producer > },
    { "FVPRT", res_vol_production_target },

    /* Region properties */
    { "ROIR"  , region_rate< rt::oil, injector > },
    { "RGIR"  , region_rate< rt::gas, injector > },
    { "RWIR"  , region_rate< rt::wat, injector > },
    { "ROPR"  , region_rate< rt::oil, producer > },
    { "RGPR"  , region_rate< rt::gas, producer > },
    { "RWPR"  , region_rate< rt::wat, producer > },
    { "ROIT"  , mul( region_rate< rt::oil, injector >, duration ) },
    { "RGIT"  , mul( region_rate< rt::gas, injector >, duration ) },
    { "RWIT"  , mul( region_rate< rt::wat, injector >, duration ) },
    { "ROPT"  , mul( region_rate< rt::oil, producer >, duration ) },
    { "RGPT"  , mul( region_rate< rt::gas, producer >, duration ) },
    { "RWPT"  , mul( region_rate< rt::wat, producer >, duration ) },
    //Multisegment well segment data
    { "SOFR", srate< rt::oil > },
    { "SWFR", srate< rt::wat > },
    { "SGFR", srate< rt::gas > },
    { "SPR",  spr }, 
    // Well productivity index
    { "WPIW", potential_rate< rt::productivity_index_water >},
    { "WPIO", potential_rate< rt::productivity_index_oil >},
    { "WPIG", potential_rate< rt::productivity_index_gas >},
    { "WPIL", sum( potential_rate< rt::productivity_index_water >, potential_rate< rt::productivity_index_oil>)},
    // Well potential
    { "WWPP", potential_rate< rt::well_potential_water , true, false>},
    { "WOPP", potential_rate< rt::well_potential_oil , true, false>},
    { "WGPP", potential_rate< rt::well_potential_gas , true, false>},
    { "WWPI", potential_rate< rt::well_potential_water , false, true>},
    { "WOPI", potential_rate< rt::well_potential_oil , false, true>},
    { "WGPI", potential_rate< rt::well_potential_gas , false, true>},
};


static const std::unordered_map< std::string, UnitSystem::measure> single_values_units = {
  {"TCPU"     , UnitSystem::measure::identity },
  {"ELAPSED"  , UnitSystem::measure::identity },
  {"NEWTON"   , UnitSystem::measure::identity },
  {"NLINERS"  , UnitSystem::measure::identity },
  {"NLINSMIN" , UnitSystem::measure::identity },
  {"NLINSMAX" , UnitSystem::measure::identity },
  {"MLINEARS" , UnitSystem::measure::identity },
  {"MSUMLINS" , UnitSystem::measure::identity },
  {"MSUMNEWT" , UnitSystem::measure::identity },
  {"TCPUTS"   , UnitSystem::measure::identity },
  {"TIMESTEP" , UnitSystem::measure::time },
  {"TCPUDAY"  , UnitSystem::measure::time },
  {"STEPTYPE" , UnitSystem::measure::identity },
  {"TELAPLIN" , UnitSystem::measure::time },
  {"FWIP"     , UnitSystem::measure::liquid_surface_volume },
  {"FOIP"     , UnitSystem::measure::liquid_surface_volume },
  {"FGIP"     , UnitSystem::measure::gas_surface_volume },
  {"FOIPL"    , UnitSystem::measure::liquid_surface_volume },
  {"FOIPG"    , UnitSystem::measure::liquid_surface_volume },
  {"FGIPL"    , UnitSystem::measure::gas_surface_volume },
  {"FGIPG"    , UnitSystem::measure::gas_surface_volume },
  {"FPR"      , UnitSystem::measure::pressure },

};

static const std::unordered_map< std::string, UnitSystem::measure> region_units = {
  {"RPR"      , UnitSystem::measure::pressure},
  {"ROIP"     , UnitSystem::measure::liquid_surface_volume },
  {"ROIPL"    , UnitSystem::measure::liquid_surface_volume },
  {"ROIPG"    , UnitSystem::measure::liquid_surface_volume },
  {"RGIP"     , UnitSystem::measure::gas_surface_volume },
  {"RGIPL"    , UnitSystem::measure::gas_surface_volume },
  {"RGIPG"    , UnitSystem::measure::gas_surface_volume },
  {"RWIP"     , UnitSystem::measure::liquid_surface_volume }
};

static const std::unordered_map< std::string, UnitSystem::measure> block_units = {
  {"BPR"        , UnitSystem::measure::pressure},
  {"BPRESSUR"   , UnitSystem::measure::pressure},
  {"BSWAT"      , UnitSystem::measure::identity},
  {"BWSAT"      , UnitSystem::measure::identity},
  {"BSGAS"      , UnitSystem::measure::identity},
  {"BGSAT"      , UnitSystem::measure::identity},
};

inline std::vector< const Well* > find_wells( const Schedule& schedule,
                                              const ecl::smspec_node* node,
                                              const int sim_step,
                                              const out::RegionCache& regionCache ) {

    const auto* name = smspec_node_get_wgname( node );
    const auto type = smspec_node_get_var_type( node );

    if ((type == ECL_SMSPEC_WELL_VAR) ||
        (type == ECL_SMSPEC_COMPLETION_VAR) ||
        (type == ECL_SMSPEC_SEGMENT_VAR))
    {
        const auto* well = schedule.getWell( name );
        if( !well ) return {};
        return { well };
    }

    if( type == ECL_SMSPEC_GROUP_VAR ) {
        if( !schedule.hasGroup( name ) ) return {};

        return schedule.getChildWells( name, sim_step, GroupWellQueryMode::Recursive);
    }

    if( type == ECL_SMSPEC_FIELD_VAR )
        return schedule.getWells();

    if( type == ECL_SMSPEC_REGION_VAR ) {
        std::vector< const Well* > wells;

        const auto region = smspec_node_get_num( node );

        for ( const auto& connection : regionCache.connections( region ) ){
            const auto& w_name = connection.first;
            const auto& well = schedule.getWell( w_name );

            const auto& it = std::find_if( wells.begin(), wells.end(),
                                           [&] ( const Well* elem )
                                           { return *elem == *well; });
            if ( it == wells.end() )
                wells.push_back( schedule.getWell( w_name ) );
        }

        return wells;
    }

    return {};
}


bool need_wells(ecl_smspec_var_type var_type, const std::string& keyword) {
    static const std::set<std::string> region_keywords{"ROIR", "RGIR", "RWIR", "ROPR", "RGPR", "RWPR", "ROIT", "RWIT", "RGIT", "ROPT", "RGPT", "RWPT"};
    if (var_type == ECL_SMSPEC_WELL_VAR)
        return true;

    if (var_type == ECL_SMSPEC_GROUP_VAR)
        return true;

    if (var_type == ECL_SMSPEC_FIELD_VAR)
        return true;

    if (var_type == ECL_SMSPEC_COMPLETION_VAR)
        return true;

    if (var_type == ECL_SMSPEC_SEGMENT_VAR)
        return true;

    /*
      Some of the region keywords are based on summing over all the connections
      which fall in the region; i.e. RGIR is the total gas injection rate in the
      region and consequently the list of defined wells is required, other
      region keywords like 'ROIP' do not require well information.
    */
    if (var_type == ECL_SMSPEC_REGION_VAR) {
        if (region_keywords.count(keyword) > 0)
            return true;
    }

    return false;
}



bool is_udq(const std::string& keyword) {
    return (keyword.size() > 1 && keyword[1] == 'U');
}

void eval_udq(const Schedule& schedule, std::size_t sim_step, SummaryState& st)
{
    const UDQInput& udq = schedule.getUDQConfig(sim_step);
    const auto& func_table = udq.function_table();
    UDQContext context(func_table, st);
    std::vector<std::string> wells;
    for (const auto& well_name : schedule.wellNames())
        wells.push_back(well_name);

    for (const auto& assign : udq.assignments(UDQVarType::WELL_VAR)) {
        auto ws = assign.eval_wells(wells);
        for (const auto& well : wells) {
            const auto& udq_value = ws[well];
            if (udq_value)
                st.update_well_var(well, ws.name(), udq_value.value());
        }
    }

    for (const auto& def : udq.definitions(UDQVarType::WELL_VAR)) {
        auto ws = def.eval_wells(context);
        for (const auto& well : wells) {
            const auto& udq_value = ws[well];
            if (udq_value)
                st.update_well_var(well, def.keyword(), udq_value.value());
        }
    }
}
}

namespace out {

class Summary::keyword_handlers {
    public:
        using fn = ofun;
        std::vector< std::pair< const ecl::smspec_node*, fn > > handlers;
        std::map< std::string, const ecl::smspec_node* > single_value_nodes;
        std::map< std::pair <std::string, int>, const ecl::smspec_node* > region_nodes;
        std::map< std::pair <std::string, int>, const ecl::smspec_node* > block_nodes;

        // Memory management for restart-related summary vectors
        // that are not requested in SUMMARY section.
        std::vector<std::unique_ptr<ecl::smspec_node>> rstvec_backing_store;
};

Summary::Summary( const EclipseState& st,
                  const SummaryConfig& sum ,
                  const EclipseGrid& grid_arg,
                  const Schedule& schedule) :
    Summary( st, sum, grid_arg, schedule, st.getIOConfig().fullBasePath().c_str() )
{}

Summary::Summary( const EclipseState& st,
                  const SummaryConfig& sum,
                  const EclipseGrid& grid_arg,
                  const Schedule& schedule,
                  const std::string& basename ) :
    Summary( st, sum, grid_arg, schedule, basename.c_str() )
{}

Summary::Summary( const EclipseState& st,
                  const SummaryConfig& sum,
                  const EclipseGrid& grid_arg,
                  const Schedule& schedule,
                  const char* basename ) :
    grid( grid_arg ),
    regionCache( st.get3DProperties( ) , grid_arg, schedule ),
    handlers( new keyword_handlers() )
{

    const auto& udq = schedule.getUDQConfig(schedule.size() - 1);
    const auto& init_config = st.getInitConfig();
    const char * restart_case = nullptr;
    int restart_step = -1;

    if (init_config.restartRequested( )) {
        if (init_config.getRestartRootName().size() <= ECL_STRING8_LENGTH * SUMMARY_RESTART_SIZE) {
            restart_case = init_config.getRestartRootName().c_str();
            restart_step = init_config.getRestartStep();
        } else
            OpmLog::warning("Restart case too long - not embedded in SMSPEC file");

        this->prev_time_elapsed =
            schedule.getTimeMap().getTimePassedUntil(restart_step);
    }
    ecl_sum.reset( ecl_sum_alloc_restart_writer2(basename,
                                                 restart_case,
                                                 restart_step,
                                                 st.getIOConfig().getFMTOUT(),
                                                 st.getIOConfig().getUNIFOUT(),
                                                 ":",
                                                 schedule.posixStartTime(),
                                                 true,
                                                 st.getInputGrid().getNX(),
                                                 st.getInputGrid().getNY(),
                                                 st.getInputGrid().getNZ()));

    /* register all keywords handlers and pair with the newly-registered ert
     * entry.
     */
    std::set< std::string > unsupported_keywords;

    for( const auto& node : sum ) {
        ecl_smspec_type * smspec = ecl_sum_get_smspec(this->ecl_sum.get());
        std::string keyword = node.keyword();

        const auto single_value_pair = single_values_units.find( keyword );
        const auto funs_pair = funs.find( keyword );
        const auto region_pair = region_units.find( keyword );
        const auto block_pair = block_units.find( keyword );

        /*
          All summary values of the type ECL_SMSPEC_MISC_VAR
          and ECL_SMSPEC_FIELD_VAR must be passed explicitly
          in the misc_values map when calling
          add_timestep.
        */
        if (single_value_pair != single_values_units.end()) {
            auto node_type = node.type();
            if ((node_type != ECL_SMSPEC_FIELD_VAR) && (node_type != ECL_SMSPEC_MISC_VAR)) {
                continue;
            }
            auto* nodeptr = ecl_smspec_add_node( smspec, keyword.c_str(), st.getUnits().name( single_value_pair->second ), 0);
            this->handlers->single_value_nodes.emplace( keyword, nodeptr );
        } else if (region_pair != region_units.end()) {
            auto* nodeptr = ecl_smspec_add_node( smspec, keyword.c_str(), node.num(), st.getUnits().name( region_pair->second ), 0);
            this->handlers->region_nodes.emplace( std::make_pair(keyword, node.num()), nodeptr );
        } else if (block_pair != block_units.end()) {
            if (node.type() != ECL_SMSPEC_BLOCK_VAR)
                continue;

            int global_index = node.num() - 1;
            if (!this->grid.cellActive(global_index))
                continue;

            auto* nodeptr = ecl_smspec_add_node( smspec, keyword.c_str(), node.num(), st.getUnits().name( block_pair->second ), 0 );
            this->handlers->block_nodes.emplace( std::make_pair(keyword, node.num()), nodeptr );
        } else if (funs_pair != funs.end()) {
            auto node_type = node.type();

            if ((node_type == ECL_SMSPEC_COMPLETION_VAR) || (node_type == ECL_SMSPEC_BLOCK_VAR)) {
                int global_index = node.num() - 1;
                if (!this->grid.cellActive(global_index))
                    continue;
            }

            /* get unit strings by calling each function with dummy input */
            const auto handle = funs_pair->second;
            const std::vector< const Well* > dummy_wells;

            const fn_args no_args { dummy_wells, // Wells from Schedule object
                                    0,           // Duration of time step
                                    0,           // Simulation step
                                    node.num(),
                                    {},          // Well results - data::Wells
                                    {},          // Region <-> cell mappings.
                                    this->grid,
                                    {}};

            const auto val = handle( no_args );

            auto * nodeptr = ecl_smspec_add_node( smspec, keyword.c_str(), node.wgname().c_str(), node.num(), st.getUnits().name( val.unit ), 0 );
            this->handlers->handlers.emplace_back( nodeptr, handle );
        } else if (is_udq(keyword)) {
            std::string udq_unit = "?????";
            const auto& udq_params = st.runspec().udqParams();

            if (udq.has_unit(keyword))
                udq_unit = udq.unit(keyword);

            ecl_smspec_add_node(smspec, keyword.c_str(), node.wgname().c_str(), node.num(), udq_unit.c_str(), udq_params.undefinedValue());
        } else
            unsupported_keywords.insert(keyword);
    }
    for ( const auto& keyword : unsupported_keywords ) {
        Opm::OpmLog::info("Keyword " + std::string(keyword) + " is unhandled");
    }

    // Guarantee existence of certain summary vectors (mostly rates and
    // cumulative totals for wells, groups, and field) that are required
    // for simulation restart.
    {
        auto& rvec     = this->handlers->rstvec_backing_store;
        auto& hndlrs   = this->handlers->handlers;

        // Required restart vectors for wells, groups, and field.
        for (const auto& vector : requiredRestartVectors(schedule)) {
            const auto& kw     = vector.first;
            const auto& entity = vector.second;

            const auto key = genKey(kw, entity);
            if (ecl_sum_has_key(this->ecl_sum.get(), key.c_str())) {
                // Vector already requested in SUMMARY section.
                // Don't add a second evaluation of this.
                continue;
            }

            auto func = funs.find(kw);
            if (func == std::end(funs)) {
                throw std::logic_error {
                    "Unable to find handler for '" + kw + "'"
                };
            }

            rvec.push_back(makeRestartVectorSMSPEC(kw, entity));
            hndlrs.emplace_back(rvec.back().get(), func->second);
        }

        // Required restart vectors for segments (if applicable).
        for (const auto& segRes : requiredSegmentVectors(schedule)) {
            const auto key = genKey(segRes);
            if (ecl_sum_has_key(this->ecl_sum.get(), key.c_str())) {
                // Segment result already requested in SUMMARY section.
                // Don't add a second evaluation of this.
                continue;
            }

            auto func = funs.find(segRes.vector);
            if (func == std::end(funs)) {
                throw std::logic_error {
                    "Unable to find handler for '" + segRes.vector + "'"
                };
            }

            rvec.push_back(makeRestartVectorSMSPEC(segRes));
            hndlrs.emplace_back(rvec.back().get(), func->second);
        }
    }

    for (const auto& pair : this->handlers->handlers) {
        const auto * nodeptr = pair.first;
        if (nodeptr->is_total())
            this->prev_state.update(*nodeptr, 0);
    }
}

/*
 * The well efficiency factor will not impact the well rate itself, but is
 * rather applied for accumulated values.The WEFAC can be considered to shut
 * and open the well for short intervals within the same timestep, and the well
 * is therefore solved at full speed.
 *
 * Groups are treated similarly as wells. The group's GEFAC is not applied for
 * rates, only for accumulated volumes. When GEFAC is set for a group, it is
 * considered that all wells are taken down simultaneously, and GEFAC is
 * therefore not applied to the group's rate. However, any efficiency factors
 * applied to the group's wells or sub-groups must be included.
 *
 * Regions and fields will have the well and group efficiency applied for both
 * rates and accumulated values.
 *
 */
std::vector< std::pair< std::string, double > >
well_efficiency_factors( const ecl::smspec_node* node,
                         const Schedule& schedule,
                         const std::vector< const Well* >& schedule_wells,
                         const int sim_step ) {
    std::vector< std::pair< std::string, double > > efac;

    auto var_type = node->get_var_type();
    if(    var_type != ECL_SMSPEC_GROUP_VAR
        && var_type != ECL_SMSPEC_FIELD_VAR
        && var_type != ECL_SMSPEC_REGION_VAR
           && !node->is_total()) {
        return efac;
    }

    const bool is_group = (var_type == ECL_SMSPEC_GROUP_VAR);
    const bool is_rate = !node->is_total();
    const auto &groupTree = schedule.getGroupTree(sim_step);

    for( const auto* well : schedule_wells ) {
        double eff_factor = well->getEfficiencyFactor(sim_step);

        if ( !well->hasBeenDefined( sim_step ) )
            continue;

        const auto* group_node = &schedule.getGroup(well->getGroupName(sim_step));

        while(true){
            if((   is_group
                && is_rate
                && group_node->name() == node->get_wgname() ))
                break;
            eff_factor *= group_node->getGroupEfficiencyFactor( sim_step );

            const auto& parent = groupTree.parent( group_node->name() );
            if( !schedule.hasGroup( parent ) )
                break;
            group_node = &schedule.getGroup( parent );
        }
        efac.emplace_back( well->name(), eff_factor );
    }

    return efac;
}

void Summary::eval( SummaryState& st,
                    int report_step,
                    double secs_elapsed,
                    const EclipseState& es,
                    const Schedule& schedule,
                    const data::Wells& wells ,
                    const std::map<std::string, double>& single_values,
                    const std::map<std::string, std::vector<double>>& region_values,
                    const std::map<std::pair<std::string, int>, double>& block_values) const {

    if (secs_elapsed < this->prev_time_elapsed) {
        const auto& usys    = es.getUnits();
        const auto  elapsed = usys.from_si(measure::time, secs_elapsed);
        const auto  prev_el = usys.from_si(measure::time, this->prev_time_elapsed);
        const auto  unt     = '[' + std::string{ usys.name(measure::time) } + ']';

        throw std::invalid_argument {
            "Elapsed time ("
            + std::to_string(elapsed) + ' ' + unt
            + ") must not precede previous elapsed time ("
            + std::to_string(prev_el) + ' ' + unt
            + "). Incorrect restart time?"
        };
    }

    const double duration = secs_elapsed - this->prev_time_elapsed;

    /* report_step is the number of the file we are about to write - i.e. for instance CASE.S$report_step
     * for the data in a non-unified summary file.
     * sim_step is the timestep which has been effective in the simulator, and as such is the value
     * necessary to use when consulting the Schedule object. */
    const auto sim_step = std::max( 0, report_step - 1 );

    for( auto& f : this->handlers->handlers ) {
        const int num = smspec_node_get_num( f.first );
        double unit_applied_val = smspec_node_get_default( f.first );

        if (need_wells(smspec_node_get_var_type(f.first ),
                       smspec_node_get_keyword(f.first))) {

            const auto schedule_wells = find_wells( schedule, f.first, sim_step, this->regionCache );
            /*
              It is not a bug as such if the schedule_wells list comes back
              empty; it just means that at the current timestep no relevant
              wells have been defined and we do not calculate a value.
            */
            if (schedule_wells.size() > 0) {
                auto eff_factors = well_efficiency_factors( f.first, schedule, schedule_wells, sim_step );
                const auto val = f.second( { schedule_wells,
                                             duration,
                                             sim_step,
                                             num,
                                             wells,
                                             this->regionCache,
                                             this->grid,
                                             eff_factors});
                unit_applied_val = es.getUnits().from_si( val.unit, val.value );
            }
        } else {
            const auto val = f.second({ {},
                                        duration,
                                        sim_step,
                                        num,
                                        {},
                                        this->regionCache,
                                        this->grid,
                                        {} });
            unit_applied_val = es.getUnits().from_si( val.unit, val.value );
        }

        if (smspec_node_is_total(f.first)) {
            const auto* genkey = smspec_node_get_gen_key1( f.first );
            unit_applied_val += this->prev_state.get(genkey);
        }

        st.update(*f.first, unit_applied_val);
    }

    for( const auto& value_pair : single_values ) {
        const std::string key = value_pair.first;
        const auto node_pair = this->handlers->single_value_nodes.find( key );
        if (node_pair != this->handlers->single_value_nodes.end()) {
            const auto unit = single_values_units.at( key );
            double si_value = value_pair.second;
            double output_value = es.getUnits().from_si(unit , si_value );
            st.update(*node_pair->second, output_value);
        }
    }

    for( const auto& value_pair : region_values ) {
        const std::string key = value_pair.first;
        for (size_t reg = 0; reg < value_pair.second.size(); ++reg) {
            const auto node_pair = this->handlers->region_nodes.find( std::make_pair(key, reg+1) );
            if (node_pair != this->handlers->region_nodes.end()) {
                const auto * nodeptr = node_pair->second;
                const auto unit = region_units.at( key );

                assert (smspec_node_get_num( nodeptr ) - 1 == static_cast<int>(reg));
                double si_value = value_pair.second[reg];
                double output_value = es.getUnits().from_si(unit , si_value );
                st.update(*nodeptr, output_value);
            }
        }
    }

    for( const auto& value_pair : block_values ) {
        const std::pair<std::string, int> key = value_pair.first;
        const auto node_pair = this->handlers->block_nodes.find( key );
        if (node_pair != this->handlers->block_nodes.end()) {
            const auto * nodeptr = node_pair->second;
            const auto unit = block_units.at( key.first );
            double si_value = value_pair.second;
            double output_value = es.getUnits().from_si(unit , si_value );
            st.update(*nodeptr, output_value);
        }
    }
    eval_udq(schedule, sim_step, st);
}


void Summary::internal_store(const SummaryState& st, int report_step, double secs_elapsed) {
    auto* tstep = ecl_sum_add_tstep( this->ecl_sum.get(), report_step, secs_elapsed );
    const ecl_sum_type * ecl_sum = this->ecl_sum.get();
    const ecl_smspec_type * smspec = ecl_sum_get_smspec(ecl_sum);
    auto num_nodes = ecl_smspec_num_nodes(smspec);
    for (int node_index = 0; node_index < num_nodes; node_index++) {
        const auto& smspec_node = ecl_smspec_iget_node(smspec, node_index);
        // The TIME node is treated specially, it is created internally in
        // the ecl_sum instance when the timestep is created - and
        // furthermore it is not in st SummaryState instance.
        if (smspec_node.get_params_index() == ecl_smspec_get_time_index(smspec))
            continue;

        const std::string key = smspec_node.get_gen_key1();
        if (st.has(key))
            ecl_sum_tstep_iset(tstep, smspec_node.get_params_index(), st.get(key));

        /*
          else
          OpmLog::warning("Have configured summary variable " + key + " for summary output - but it has not been calculated");
        */
    }
}


void Summary::add_timestep( int report_step,
                            double secs_elapsed,
                            const EclipseState& es,
                            const Schedule& schedule,
                            const data::Wells& wells ,
                            const std::map<std::string, double>& single_values,
                            const std::map<std::string, std::vector<double>>& region_values,
                            const std::map<std::pair<std::string, int>, double>& block_values) {
    SummaryState st;
    this->eval(st, report_step, secs_elapsed, es, schedule, wells, single_values, region_values, block_values);
    this->internal_store(st, report_step, secs_elapsed);

    this->prev_state = st;
    this->prev_time_elapsed = secs_elapsed;
}


void Summary::write() const {
    ecl_sum_fwrite( this->ecl_sum.get() );
}


Summary::~Summary() {}

const SummaryState& Summary::get_restart_vectors() const
{
    return this->prev_state;
}

void Summary::reset_cumulative_quantities(const SummaryState& rstrt)
{
    for (const auto& f : this->handlers->handlers) {
        if (! smspec_node_is_total(f.first)) {
            // Ignore quantities that are not cumulative ("total").
            continue;
        }

        const auto* genkey = smspec_node_get_gen_key1(f.first);
        if (rstrt.has(genkey)) {
            // Assume 'rstrt' uses output units.  This is satisfied if rstrt
            // is constructed from information in a restart file--i.e., from
            // the double precision restart vectors 'XGRP' and 'XWEL' during
            // RestartIO::load().
            this->prev_state.set(genkey, rstrt.get(genkey));
        }
    }
}


}} // namespace Opm::out
