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

#include <numeric>

#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellProductionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperty.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

#include <opm/output/eclipse/Summary.hpp>
#include <opm/output/eclipse/RegionCache.hpp>

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
    size_t timestep;
    int  num;
    const data::Wells& wells;
    const data::Solution& state;
    const out::RegionCache& regionCache;
    const EclipseGrid& grid;
};

/* Since there are several enums in opm scattered about more-or-less
 * representing the same thing. Since functions use template parameters to
 * expand into the actual implementations we need a static way to determine
 * what unit to tag the return value with.
 */
template< rt > constexpr
measure rate_unit() { return measure::liquid_surface_rate; }
template< Phase::PhaseEnum > constexpr
measure rate_unit() { return measure::liquid_surface_rate; }

template<> constexpr
measure rate_unit< rt::gas >() { return measure::gas_surface_rate; }
template<> constexpr
measure rate_unit< Phase::GAS >() { return measure::gas_surface_rate; }

template<> constexpr
measure rate_unit< rt::solvent >() { return measure::gas_surface_rate; }

template< rt phase, bool injection = true >
inline quantity rate( const fn_args& args ) {
    double sum = 0.0;

    for( const auto* sched_well : args.schedule_wells ) {
        const auto& name = sched_well->name();
        if( args.wells.count( name ) == 0 ) continue;
        const auto v = args.wells.at( name ).rates.get( phase, 0.0 );
        if( ( v > 0 ) == injection )
            sum += v;
    }

    if( !injection ) sum *= -1;
    return { sum, rate_unit< phase >() };
}

template< rt phase, bool injection = true >
inline quantity crate( const fn_args& args ) {
    const quantity zero = { 0, rate_unit< phase >() };
    // The args.num value is the literal value which will go to the
    // NUMS array in the eclispe SMSPEC file; the values in this array
    // are offset 1 - whereas we need to use this index here to look
    // up a completion with offset 0.
    const auto global_index = args.num - 1;
    const auto active_index = args.grid.activeIndex( global_index );
    if( args.schedule_wells.empty() ) return zero;

    const auto& name = args.schedule_wells.front()->name();
    if( args.wells.count( name ) == 0 ) return zero;

    const auto& well = args.wells.at( name );
    const auto& completion = std::find_if( well.completions.begin(),
                                           well.completions.end(),
                                           [=]( const data::Completion& c ) {
                                                return c.index == active_index;
                                           } );

    if( completion == well.completions.end() ) return zero;
    const auto v = completion->rates.get( phase, 0.0 );
    if( ( v > 0 ) != injection ) return zero;

    if( !injection ) return { -v, rate_unit< phase >() };
    return { v, rate_unit< phase >() };
}


template< rt phase > inline quantity prodrate( const fn_args& args )
{ return rate< phase, false >( args ); }
template< rt phase > inline quantity injerate( const fn_args& args )
{ return rate< phase, true >( args ); }
template< rt phase > inline quantity prodcrate( const fn_args& args )
{ return crate< phase, false >( args ); }
template< rt phase > inline quantity injecrate( const fn_args& args )
{ return crate< phase, true >( args ); }


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

template< Phase::PhaseEnum phase >
inline quantity production_history( const fn_args& args ) {
    /*
     * For well data, looking up historical rates (both for production and
     * injection) before simulation actually starts is impossible and
     * nonsensical. We therefore default to writing zero (which is what eclipse
     * seems to do as well). Additionally, when an input deck is parsed,
     * timesteps and rates are structured as such:
     *
     * The rates observed in timestep N is denoted at timestep N-1, i.e. at the
     * **end** of the previous timestep. Which means that what observed at
     * timestep 1 is denoted at timestep 0, and what happens "on" timestep 0
     * doesn't exist and would in code give an arithmetic error. We therefore
     * special-case timestep N == 0, and for all other timesteps look up the
     * value *reported* at N-1 which applies to timestep N.
     */
    if( args.timestep == 0 ) return { 0.0, rate_unit< phase >() };

    const auto timestep = args.timestep - 1;

    double sum = 0.0;
    for( const Well* sched_well : args.schedule_wells )
        sum += sched_well->production_rate( phase, timestep );

    return { sum, rate_unit< phase >() };
}

template< Phase::PhaseEnum phase >
inline quantity injection_history( const fn_args& args ) {
    if( args.timestep == 0 ) return { 0.0, rate_unit< phase >() };

    const auto timestep = args.timestep - 1;

    double sum = 0.0;
    for( const Well* sched_well : args.schedule_wells )
        sum += sched_well->injection_rate( phase, timestep );

    return { sum, rate_unit< phase >() };
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
    const auto& well_completions = args.regionCache.completions( args.num );
    for (const auto& pair : well_completions) {
        double rate = args.wells.get( pair.first , pair.second , phase );

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

template<rt phase>
quantity region_production(const fn_args& args) {
    return region_rate<phase , false>(args);
}

template<rt phase>
quantity region_injection(const fn_args& args) {
    return region_rate<phase , true>(args);
}


quantity region_sum( const fn_args& args , const std::string& keyword , UnitSystem::measure unit) {
    const auto& cells = args.regionCache.cells( args.num );
    if (cells.empty())
        return { 0.0 , unit };

    double sum = 0;

    if( args.state.count( keyword ) == 0 )
        return { 0.0, unit };

    const std::vector<double>& sim_value = args.state.data( keyword );

    for (auto cell_index : cells)
        sum += sim_value[cell_index];

    return { sum , unit };
}


quantity rpr(const fn_args& args) {
    quantity p = region_sum( args , "PRESSURE" ,measure::pressure );
    const auto& cells = args.regionCache.cells( args.num );
    if (cells.size() > 0)
        p /= cells.size();
    return p;
}

quantity roip(const fn_args& args) {
    return region_sum( args , "OIP", measure::volume );
}

quantity rgip(const fn_args& args) {
    return region_sum( args , "GIP", measure::volume );
}

quantity roipl(const fn_args& args) {
    return region_sum( args , "OIPL", measure::volume );
}

quantity roipg(const fn_args& args) {
    return region_sum( args , "OIPG", measure::volume );
}

quantity fgip( const fn_args& args ) {
    quantity zero { 0.0, measure::volume };
    if( !args.state.has( "GIP" ) )
        return zero;

    const auto& cells = args.state.at( "GIP" ).data;
    return { std::accumulate( cells.begin(), cells.end(), 0.0 ),
             measure::volume };
}

quantity foip( const fn_args& args ) {
    if( !args.state.has( "OIP" ) )
        return { 0.0, measure::volume };

    const auto& cells = args.state.at( "OIP" ).data;
    return { std::accumulate( cells.begin(), cells.end(), 0.0 ),
             measure::volume };
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
    { "WWIR", injerate< rt::wat > },
    { "WOIR", injerate< rt::oil > },
    { "WGIR", injerate< rt::gas > },
    { "WNIR", injerate< rt::solvent > },

    { "WWIT", mul( injerate< rt::wat >, duration ) },
    { "WOIT", mul( injerate< rt::oil >, duration ) },
    { "WGIT", mul( injerate< rt::gas >, duration ) },
    { "WNIT", mul( injerate< rt::solvent >, duration ) },

    { "WWPR", prodrate< rt::wat > },
    { "WOPR", prodrate< rt::oil > },
    { "WGPR", prodrate< rt::gas > },
    { "WNPR", prodrate< rt::solvent > },

    { "WLPR", sum( prodrate< rt::wat >, prodrate< rt::oil > ) },
    { "WWPT", mul( prodrate< rt::wat >, duration ) },
    { "WOPT", mul( prodrate< rt::oil >, duration ) },
    { "WGPT", mul( prodrate< rt::gas >, duration ) },
    { "WNPT", mul( prodrate< rt::solvent >, duration ) },
    { "WLPT", mul( sum( prodrate< rt::wat >, prodrate< rt::oil > ),
                   duration ) },

    { "WWCT", div( prodrate< rt::wat >,
                   sum( prodrate< rt::wat >, prodrate< rt::oil > ) ) },
    { "GWCT", div( prodrate< rt::wat >,
                   sum( prodrate< rt::wat >, prodrate< rt::oil > ) ) },
    { "WGOR", div( prodrate< rt::gas >, prodrate< rt::oil > ) },
    { "GGOR", div( prodrate< rt::gas >, prodrate< rt::oil > ) },
    { "WGLR", div( prodrate< rt::gas >,
                   sum( prodrate< rt::wat >, prodrate< rt::oil > ) ) },

    { "WBHP", bhp },
    { "WTHP", thp },

    { "GWIR", injerate< rt::wat > },
    { "GOIR", injerate< rt::oil > },
    { "GGIR", injerate< rt::gas > },
    { "GNIR", injerate< rt::solvent > },
    { "GWIT", mul( injerate< rt::wat >, duration ) },
    { "GOIT", mul( injerate< rt::oil >, duration ) },
    { "GGIT", mul( injerate< rt::gas >, duration ) },
    { "GNIT", mul( injerate< rt::solvent >, duration ) },

    { "GWPR", prodrate< rt::wat > },
    { "GOPR", prodrate< rt::oil > },
    { "GGPR", prodrate< rt::gas > },
    { "GNPR", prodrate< rt::solvent > },
    { "GLPR", sum( prodrate< rt::wat >, prodrate< rt::oil > ) },

    { "GWPT", mul( prodrate< rt::wat >, duration ) },
    { "GOPT", mul( prodrate< rt::oil >, duration ) },
    { "GGPT", mul( prodrate< rt::gas >, duration ) },
    { "GNPT", mul( prodrate< rt::solvent >, duration ) },
    { "GLPT", mul( sum( prodrate< rt::wat >, prodrate< rt::oil > ),
                   duration ) },

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
    { "GLPTH", mul( sum( production_history< Phase::WATER >,
                         production_history< Phase::OIL > ),
                    duration ) },
    { "GWITH", mul( injection_history< Phase::WATER >, duration ) },
    { "GGITH", mul( injection_history< Phase::GAS >, duration ) },

    { "CWIR", injecrate< rt::wat > },
    { "CGIR", injecrate< rt::gas > },
    { "CWIT", mul( injecrate< rt::wat >, duration ) },
    { "CGIT", mul( injecrate< rt::gas >, duration ) },
    { "CNIT", mul( injecrate< rt::solvent >, duration ) },

    { "CWPR", prodcrate< rt::wat > },
    { "COPR", prodcrate< rt::oil > },
    { "CGPR", prodcrate< rt::gas > },
    // Minus for injection rates and pluss for production rate
    { "CNFR", sub( prodcrate< rt::solvent>, injecrate<rt::solvent>) },
    { "CWPT", mul( prodcrate< rt::wat >, duration ) },
    { "COPT", mul( prodcrate< rt::oil >, duration ) },
    { "CGPT", mul( prodcrate< rt::gas >, duration ) },
    { "CNPT", mul( prodcrate< rt::solvent >, duration ) },

    { "FWPR", prodrate< rt::wat > },
    { "FOPR", prodrate< rt::oil > },
    { "FGPR", prodrate< rt::gas > },
    { "FNPR", prodrate< rt::solvent > },

    { "FLPR", sum( prodrate< rt::wat >, prodrate< rt::oil > ) },
    { "FWPT", mul( prodrate< rt::wat >, duration ) },
    { "FOPT", mul( prodrate< rt::oil >, duration ) },
    { "FGPT", mul( prodrate< rt::gas >, duration ) },
    { "FNPT", mul( prodrate< rt::solvent >, duration ) },
    { "FLPT", mul( sum( prodrate< rt::wat >, prodrate< rt::oil > ),
                   duration ) },

    { "FWIR", injerate< rt::wat > },
    { "FOIR", injerate< rt::oil > },
    { "FGIR", injerate< rt::gas > },
    { "FNIR", injerate< rt::solvent > },

    { "FLIR", sum( injerate< rt::wat >, injerate< rt::oil > ) },
    { "FWIT", mul( injerate< rt::wat >, duration ) },
    { "FOIT", mul( injerate< rt::oil >, duration ) },
    { "FGIT", mul( injerate< rt::gas >, duration ) },
    { "FNIT", mul( injerate< rt::solvent >, duration ) },
    { "FLIT", mul( sum( injerate< rt::wat >, injerate< rt::oil > ),
                   duration ) },

    { "FOIP", foip },
    { "FGIP", fgip },

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

    { "FWCT", div( prodrate< rt::wat >,
                   sum( prodrate< rt::wat >, prodrate< rt::oil > ) ) },
    { "FGOR", div( prodrate< rt::gas >, prodrate< rt::oil > ) },
    { "FGLR", div( prodrate< rt::gas >,
                   sum( prodrate< rt::wat >, prodrate< rt::oil > ) ) },
    { "FWCTH", div( production_history< Phase::WATER >,
                    sum( production_history< Phase::WATER >,
                         production_history< Phase::OIL > ) ) },
    { "FGORH", div( production_history< Phase::GAS >,
                    production_history< Phase::OIL > ) },
    { "FGLRH", div( production_history< Phase::GAS >,
                    sum( production_history< Phase::WATER >,
                         production_history< Phase::OIL > ) ) },

    /* Region properties */
    { "RPR" , rpr},
    { "ROIP" , roip},
    { "ROIPL" , roipl},
    { "ROIPG" , roipg},
    { "RGIP"  , rgip},
    { "ROPR"  , region_production<rt::oil>},
    { "RGPR"  , region_production<rt::gas>},
    { "RWPR"  , region_production<rt::wat>},
    { "ROPT"  , mul( region_production<rt::oil>, duration )},
    { "RGPT"  , mul( region_production<rt::gas>, duration )},
    { "RWPT"  , mul( region_production<rt::wat>, duration )},
};


inline std::vector< const Well* > find_wells( const Schedule& schedule,
                                              const smspec_node_type* node,
                                              size_t timestep ) {

    const auto* name = smspec_node_get_wgname( node );
    const auto type = smspec_node_get_var_type( node );

    if( type == ECL_SMSPEC_WELL_VAR || type == ECL_SMSPEC_COMPLETION_VAR ) {
        const auto* well = schedule.getWell( name );
        if( !well ) return {};
        return { well };
    }

    if( type == ECL_SMSPEC_GROUP_VAR ) {
        if( !schedule.hasGroup( name ) ) return {};

        const auto& group = schedule.getGroup( name );

        std::vector< const Well* > wells;
        for( const auto& pair : group.getWells( timestep ) )
            wells.push_back( pair.second );

        return wells;
    }

    if( type == ECL_SMSPEC_FIELD_VAR )
        return schedule.getWells();

    return {};
}

}

namespace out {

class Summary::keyword_handlers {
    public:
        using fn = ofun;
        std::vector< std::pair< smspec_node_type*, fn > > handlers;
};

Summary::Summary( const EclipseState& st, const SummaryConfig& sum ) :
    Summary( st, sum, st.getIOConfig().fullBasePath().c_str() )
{}

Summary::Summary( const EclipseState& st,
                  const SummaryConfig& sum,
                  const std::string& basename ) :
    Summary( st, sum, basename.c_str() )
{}

Summary::Summary( const EclipseState& st,
                  const SummaryConfig& sum,
                  const char* basename ) :
    ecl_sum(
            ecl_sum_alloc_writer(
                basename,
                st.getIOConfig().getFMTOUT(),
                st.getIOConfig().getUNIFOUT(),
                ":",
                st.getSchedule().posixStartTime(),
                true,
                st.getInputGrid().getNX(),
                st.getInputGrid().getNY(),
                st.getInputGrid().getNZ()
                )
            ),
    handlers( new keyword_handlers() )
{
    /* register all keywords handlers and pair with the newly-registered ert
     * entry.
     */
    for( const auto& node : sum ) {
        const auto* keyword = node.keyword();
        if( funs.find( keyword ) == funs.end() ) continue;

        /* get unit strings by calling each function with dummy input */
        const auto handle = funs.find( keyword )->second;
        const std::vector< const Well* > dummy_wells;
        EclipseGrid dummy_grid(1,1,1);
        const fn_args no_args{ dummy_wells, 0, 0, 0, {} , {}, {} , dummy_grid };
        const auto val = handle( no_args );
        const auto* unit = st.getUnits().name( val.unit );

        auto* nodeptr = ecl_sum_add_var( this->ecl_sum.get(), keyword,
                                         node.wgname(), node.num(), unit, 0 );
        this->handlers->handlers.emplace_back( nodeptr, handle );
    }
}

void Summary::add_timestep( int report_step,
                            double secs_elapsed,
                            const EclipseGrid& grid,
                            const EclipseState& es,
                            const RegionCache& regionCache,
                            const data::Wells& wells ,
                            const data::Solution& state) {

    auto* tstep = ecl_sum_add_tstep( this->ecl_sum.get(), report_step, secs_elapsed );
    const double duration = secs_elapsed - this->prev_time_elapsed;

    const size_t timestep = report_step;
    const auto& schedule = es.getSchedule();

    for( auto& f : this->handlers->handlers ) {
        const int num = smspec_node_get_num( f.first );
        const auto* genkey = smspec_node_get_gen_key1( f.first );

        const auto schedule_wells = find_wells( schedule, f.first, timestep );
        const auto val = f.second( { schedule_wells, duration, timestep, num, wells , state , regionCache , grid} );

        const auto unit_applied_val = es.getUnits().from_si( val.unit, val.value );
        const auto res = smspec_node_is_total( f.first ) && prev_tstep
            ? ecl_sum_tstep_get_from_key( prev_tstep, genkey ) + unit_applied_val
            : unit_applied_val;

        ecl_sum_tstep_set_from_node( tstep, f.first, res );
    }

    this->prev_tstep = tstep;
    this->prev_time_elapsed = secs_elapsed;
}

void Summary::write() {
    ecl_sum_fwrite( this->ecl_sum.get() );
}

Summary::~Summary() {}

}
}
