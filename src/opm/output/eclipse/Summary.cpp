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

#include <opm/output/eclipse/Summary.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Group.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/WellProductionProperties.hpp>
#include <opm/parser/eclipse/EclipseState/SummaryConfig/SummaryConfig.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

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
 * unit conversion. This removes a ton of boilerplate.
 *
 * Some arithmetic operations are supported, which also codifies the following
 * assumptions:
 * * Division by zero, such as in gas/oil ratios without oil, shall default to 0
 * * Division typically happen to calculate ratios, in which case unit
 *   conversion is a no-op
 * * When performing an operation on two operands, the left operand's unit is
 *   applied, meaning arithmetic is **not** associative with respect to units.
 */
struct quantity {
    double value;
    UnitSystem::measure unit;

    quantity operator+( const quantity& rhs ) const {
        return { this->value + rhs.value, this->unit };
    }

    quantity operator*( const quantity& rhs ) const {
        return { this->value * rhs.value, this->unit };
    }

    quantity operator/( const quantity& rhs ) const {
        if( rhs.value == 0 ) return { 0.0, measure::identity };
        return { this->value / rhs.value, measure::identity };
    }
};

/*
 * All functions must have the same parameters, so they're gathered in a struct
 * and functions use whatever information they care about.
 *
 * ecl_wells are wells from the deck, provided by opm-parser. wells is
 * simulation data
 */
struct fn_args {
    const std::vector< const Well* >& ecl_wells;
    double duration;
    size_t timestep;
    const data::Wells& wells;
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

template< rt phase >
inline quantity rate( const fn_args& args ) {
    double sum = 0.0;

    for( const auto* ecl_well : args.ecl_wells ) {
        if( args.wells.wells.count( ecl_well->name() ) == 0 ) continue;
        sum += args.wells.at( ecl_well->name() ).rates.get( phase, 0.0 );
    }

    return { sum, rate_unit< phase >() };
}

inline quantity bhp( const fn_args& args ) {
    assert( args.ecl_wells.size() == 1 );

    const auto p = args.wells.wells.find( args.ecl_wells.front()->name() );
    if( p == args.wells.wells.end() )
        return { 0, measure::pressure };

    return { p->second.bhp, measure::pressure };
}

inline quantity thp( const fn_args& args ) {
    assert( args.ecl_wells.size() == 1 );

    const auto p = args.wells.wells.find( args.ecl_wells.front()->name() );
    if( p == args.wells.wells.end() )
        return { 0, measure::pressure };

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
    for( const Well* ecl_well : args.ecl_wells )
        sum += ecl_well->production_rate( phase, timestep );

    return { sum, rate_unit< phase >() };
}

template< Phase::PhaseEnum phase >
inline quantity injection_history( const fn_args& args ) {
    if( args.timestep == 0 ) return { 0.0, rate_unit< phase >() };

    const auto timestep = args.timestep - 1;

    double sum = 0.0;
    for( const Well* ecl_well : args.ecl_wells )
        sum += ecl_well->injection_rate( phase, timestep );

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

struct duration {
    quantity operator()( const fn_args& args ) const {
        return { args.duration, measure::identity };
    }
};

/* constant also uses the arithmetic-gets-left-hand-as-unit assumption, meaning
 * it can also be used for unit conversion.
 */
template< int N, measure M = measure::identity >
struct constant {
    quantity operator()( const fn_args& args ) const { return { N, M }; }
};

using const_liq = constant< 1, measure::liquid_surface_volume >;
using const_gas = constant< 1, measure::gas_surface_volume >;

template< typename F, typename G >
using mul = bin_op< F, G, std::multiplies< quantity > >;

template< typename F, typename G >
auto sum( F f, G g ) -> bin_op< F, G, std::plus< quantity > > { return { f, g }; }

template< typename F, typename G >
auto div( F f, G g ) -> bin_op< F, G, std::divides< quantity > > { return { f, g }; }

template< typename F >
auto negate( F f ) -> mul< F, constant< -1 > > { return { f }; }

template< typename F >
auto liq_vol( F f ) -> mul< const_liq, mul< F, duration > >
{ return { const_liq(), f }; }

template< typename F >
auto gas_vol( F f ) -> mul< const_gas, mul< F, duration > >
{ return { const_gas(), f }; }

using ofun = std::function< quantity( const fn_args& ) >;

static const std::unordered_map< std::string, ofun > funs = {
    { "WWIR", rate< rt::wat > },
    { "WOIR", rate< rt::oil > },
    { "WGIR", rate< rt::gas > },
    { "WWIT", liq_vol( rate< rt::wat > ) },
    { "WOIT", liq_vol( rate< rt::oil > ) },
    { "WGIT", gas_vol( rate< rt::gas > ) },

    { "WWPR", negate( rate< rt::wat > ) },
    { "WOPR", negate( rate< rt::oil > ) },
    { "WGPR", negate( rate< rt::gas > ) },
    { "WLPR", negate( sum( rate< rt::wat >, rate< rt::oil > ) ) },
    { "WWPT", negate( liq_vol( rate< rt::wat > ) ) },
    { "WOPT", negate( liq_vol( rate< rt::oil > ) ) },
    { "WGPT", negate( gas_vol( rate< rt::gas > ) ) },
    { "WLPT", negate( liq_vol( sum( rate< rt::wat >, rate< rt::oil > ) ) ) },

    { "WWCT", div( rate< rt::wat >,
                   sum( rate< rt::wat >, rate< rt::oil > ) ) },
    { "GWCT", div( rate< rt::wat >,
                   sum( rate< rt::wat >, rate< rt::oil > ) ) },
    { "WGOR", div( rate< rt::gas >, rate< rt::oil > ) },
    { "GGOR", div( rate< rt::gas >, rate< rt::oil > ) },
    { "WGLR", div( rate< rt::gas >,
                    sum( rate< rt::wat >, rate< rt::oil > ) ) },

    { "WBHP", bhp },
    { "WTHP", thp },

    { "GWIR", rate< rt::wat > },
    { "GOIR", rate< rt::oil > },
    { "GGIR", rate< rt::gas > },
    { "GWIT", liq_vol( rate< rt::wat > ) },
    { "GOIT", liq_vol( rate< rt::oil > ) },
    { "GGIT", gas_vol( rate< rt::gas > ) },

    { "GWPR", negate( rate< rt::wat > ) },
    { "GOPR", negate( rate< rt::oil > ) },
    { "GGPR", negate( rate< rt::gas > ) },
    { "GLPR", negate( sum( rate< rt::wat >, rate< rt::oil > ) ) },

    { "GWPT", negate( liq_vol( rate< rt::wat > ) ) },
    { "GOPT", negate( liq_vol( rate< rt::oil > ) ) },
    { "GGPT", negate( gas_vol( rate< rt::gas > ) ) },
    { "GLPT", negate( liq_vol( sum( rate< rt::wat >, rate< rt::oil > ) ) ) },

    { "WWPRH", production_history< Phase::WATER > },
    { "WOPRH", production_history< Phase::OIL > },
    { "WGPRH", production_history< Phase::GAS > },
    { "WLPRH", sum( production_history< Phase::WATER >,
                    production_history< Phase::OIL > ) },

    { "WWPTH", liq_vol( production_history< Phase::WATER > ) },
    { "WOPTH", liq_vol( production_history< Phase::OIL > ) },
    { "WGPTH", gas_vol( production_history< Phase::GAS > ) },
    { "WLPTH", liq_vol( sum( production_history< Phase::WATER >,
                            production_history< Phase::OIL > ) ) },

    { "WWIRH", injection_history< Phase::WATER > },
    { "WOIRH", injection_history< Phase::OIL > },
    { "WGIRH", injection_history< Phase::GAS > },
    { "WWITH", liq_vol( injection_history< Phase::WATER > ) },
    { "WOITH", liq_vol( injection_history< Phase::OIL > ) },
    { "WGITH", gas_vol( injection_history< Phase::GAS > ) },

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

    { "GWPTH", liq_vol( production_history< Phase::WATER > ) },
    { "GOPTH", liq_vol( production_history< Phase::OIL > ) },
    { "GGPTH", gas_vol( production_history< Phase::GAS > ) },
    { "GLPTH", liq_vol( sum( production_history< Phase::WATER >,
                             production_history< Phase::OIL > ) ) },
    { "GWITH", liq_vol( injection_history< Phase::WATER > ) },
    { "GGITH", gas_vol( injection_history< Phase::GAS > ) },
};

inline std::vector< const Well* > find_wells( const Schedule& schedule,
                                              const smspec_node_type* node,
                                              size_t timestep ) {

    const auto* name = smspec_node_get_wgname( node );
    const auto type = smspec_node_get_var_type( node );

    if( type == ECL_SMSPEC_WELL_VAR ) {
        const auto* well = schedule.getWell( name );
        if( !well ) return {};
        return { well };
    }

    if( type == ECL_SMSPEC_GROUP_VAR ) {
        const auto* group = schedule.getGroup( name );
        if( !group ) return {};

        std::vector< const Well* > wells;
        for( const auto& pair : group->getWells( timestep ) )
            wells.push_back( pair.second );

        return wells;
    }

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
    Summary( st, sum, st.getIOConfig()->fullBasePath().c_str() )
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
                st.getIOConfig()->getFMTOUT(),
                st.getIOConfig()->getUNIFOUT(),
                ":",
                st.getSchedule()->posixStartTime(),
                true,
                st.getInputGrid()->getNX(),
                st.getInputGrid()->getNY(),
                st.getInputGrid()->getNZ()
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

        auto* nodeptr = ecl_sum_add_var( this->ecl_sum.get(), keyword,
                                         node.wgname(), node.num(), "", 0 );

        this->handlers->handlers.emplace_back( nodeptr,
                                               funs.find( keyword )->second );
    }
}

void Summary::add_timestep( int report_step,
                            double secs_elapsed,
                            const EclipseState& es,
                            const data::Wells& wells ) {

    auto* tstep = ecl_sum_add_tstep( this->ecl_sum.get(), report_step, secs_elapsed );
    const double duration = secs_elapsed - this->prev_time_elapsed;

    const size_t timestep = report_step;
    const auto& schedule = *es.getSchedule();

    for( auto& f : this->handlers->handlers ) {
        const auto* genkey = smspec_node_get_gen_key1( f.first );

        const auto ecl_wells = find_wells( schedule, f.first, timestep );
        const auto val = f.second( { ecl_wells, duration, timestep, wells } );

        const auto num_val = val.value > 0 ? val.value : 0.0;
        const auto unit_applied_val = es.getUnits().from_si( val.unit, num_val );
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
