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

namespace Opm {

namespace {

/*
 * A series of helper functions to read & compute values from the simulator,
 * intended to clean up the keyword -> action mapping in the *_keyword
 * functions.
 */

using rt = data::Rates::opt;

/* The supported Eclipse keywords */
enum class E : out::Summary::kwtype {
    WBHP,
    WBHPH,
    WGIR,
    WGIRH,
    WGIT,
    WGITH,
    WGOR,
    WGORH,
    WGLR,
    WGLRH,
    WGPR,
    WGPRH,
    WGPT,
    WGPTH,
    WLPR,
    WLPRH,
    WLPT,
    WLPTH,
    WOIR,
    WOIRH,
    WOIT,
    WOITH,
    WOPR,
    WOPRH,
    WOPT,
    WOPTH,
    WTHP,
    WTHPH,
    WWCT,
    WWCTH,
    WWIR,
    WWIRH,
    WWIT,
    WWITH,
    WWPR,
    WWPRH,
    WWPT,
    WWPTH,
    GWPT,
    GOPT,
    GGPT,
    GWPR,
    GOPR,
    GLPR,
    GGPR,
    GWIR,
    GWIT,
    GGIR,
    GGIT,
    GGIRH,
    GWIRH,
    GOIRH,
    GGITH,
    GWITH,
    GWPRH,
    GOPRH,
    GGPRH,
    GLPRH,
    GWPTH,
    GOPTH,
    GGPTH,
    GLPTH,
    GWCT,
    GGOR,
    UNSUPPORTED,
};

const std::map< std::string, E > keyhash = {
    { "WBHP",  E::WBHP },
    { "WBHPH", E::WBHPH },
    { "WGIR",  E::WGIR },
    { "WGIRH", E::WGIRH },
    { "WGIT",  E::WGIT },
    { "WGITH", E::WGITH },
    { "WGOR",  E::WGOR },
    { "WGORH", E::WGORH },
    { "WGLR",  E::WGLR },
    { "WGLRH", E::WGLRH },
    { "WGPR",  E::WGPR },
    { "WGPRH", E::WGPRH },
    { "WGPT",  E::WGPT },
    { "WGPTH", E::WGPTH },
    { "WLPR",  E::WLPR },
    { "WLPRH", E::WLPRH },
    { "WLPT",  E::WLPT },
    { "WLPTH", E::WLPTH },
    { "WOIR",  E::WOIR },
    { "WOIRH", E::WOIRH },
    { "WOIT",  E::WOIT },
    { "WOITH", E::WOITH },
    { "WOPR",  E::WOPR },
    { "WOPRH", E::WOPRH },
    { "WOPT",  E::WOPT },
    { "WOPTH", E::WOPTH },
    { "WTHP",  E::WTHP },
    { "WTHPH", E::WTHPH },
    { "WWCT",  E::WWCT },
    { "WWCTH", E::WWCTH },
    { "WWIR",  E::WWIR },
    { "WWIRH", E::WWIRH },
    { "WWIT",  E::WWIT },
    { "WWITH", E::WWITH },
    { "WWPR",  E::WWPR },
    { "WWPRH", E::WWPRH },
    { "WWPT",  E::WWPT },
    { "WWPTH", E::WWPTH },
    { "GWPT",  E::GWPT },
    { "GOPT",  E::GOPT },
    { "GGPT",  E::GGPT },
    { "GWPR",  E::GWPR },
    { "GOPR",  E::GOPR },
    { "GLPR",  E::GLPR },
    { "GGPR",  E::GGPR },
    { "GWIR",  E::GWIR },
    { "GWIT",  E::GWIT },
    { "GGIR",  E::GGIR },
    { "GWPRH", E::GWPRH },
    { "GOPRH", E::GOPRH },
    { "GGPRH", E::GGPRH },
    { "GLPRH", E::GLPRH },
    { "GWPTH", E::GWPTH },
    { "GOPTH", E::GOPTH },
    { "GGPTH", E::GGPTH },
    { "GLPTH", E::GLPTH },
    { "GGIRH", E::GGIRH },
    { "GWIRH", E::GWIRH },
    { "GOIRH", E::GOIRH },
    { "GGIT",  E::GGIT },
    { "GWITH", E::GWITH },
    { "GGITH", E::GGITH },
    { "GWCT",  E::GWCT },
    { "GGOR",  E::GGOR },
};

inline E khash( const char* key ) {
    /* Since a switch is used to determine the proper computation from the
     * input node, but keywords are stored as strings, we need a string -> enum
     * mapping for keywords.
     */
    auto itr = keyhash.find( key );
    if( itr == keyhash.end() ) return E::UNSUPPORTED;
    return itr->second;
}

inline double wct( double wat, double oil ) {
    /* handle div-by-zero - if this well is shut, all production rates will be
     * zero and there is no cut (i.e. zero). */
    if( oil == 0 ) return 0;

    return wat / ( wat + oil );
}

inline double wwcth( const Well& w, size_t ts ) {
    /* From our point of view, injectors don't have meaningful water cuts. */
    if( w.isInjector( ts ) ) return 0;

    const auto& p = w.getProductionProperties( ts );
    return wct( p.WaterRate, p.OilRate );
}

inline double glr( double gas, double liq ) {
    /* handle div-by-zero - if this well is shut, all production rates will be
     * zero and there is no gas/oil ratio, (i.e. zero). 
     *
     * Also, if this is a gas well that produces no liquid, gas/liquid ratio
     * would be undefined and is explicitly set to 0. This is the author's best
     * guess.  If other semantics when just gas is produced, this must be
     * changed.
     */
    if( liq == 0 ) return 0;

    return gas / liq;
}

inline double wgorh( const Well& w, size_t ts ) {
    /* We do not support mixed injections, and gas/oil is undefined when oil is
     * zero (i.e. pure gas injector), so always output 0 if this is an injector
     */
    if( w.isInjector( ts ) ) return 0;

    const auto& p = w.getProductionProperties( ts );

    return glr( p.GasRate, p.OilRate );
}

inline double wglrh( const Well& w, size_t ts ) {
    /* We do not support mixed injections, and gas/oil is undefined when oil is
     * zero (i.e. pure gas injector), so always output 0 if this is an injector
     */
    if( w.isInjector( ts ) ) return 0;

    const auto& p = w.getProductionProperties( ts );

    return glr( p.GasRate, p.WaterRate + p.OilRate );
}

enum class WT { wat, oil, gas };
inline double prodrate( const Well& w,
                        size_t timestep,
                        WT wt,
                        const UnitSystem& units ) {

    if( !w.isProducer( timestep ) ) return 0;

    const auto& p = w.getProductionProperties( timestep );
    switch( wt ) {
        case WT::wat: return units.from_si( UnitSystem::measure::liquid_surface_rate, p.WaterRate );
        case WT::oil: return units.from_si( UnitSystem::measure::liquid_surface_rate, p.OilRate );
        case WT::gas: return units.from_si( UnitSystem::measure::gas_surface_rate, p.GasRate );
    }

    throw std::runtime_error( "Reached impossible state in prodrate" );
}

inline double prodvol( const Well& w,
                       size_t timestep,
                       WT wt,
                       const UnitSystem& units ) {
    const auto rate = prodrate( w, timestep, wt, units );
    return rate * units.from_si( UnitSystem::measure::time, 1 );
}

inline double injerate( const Well& w,
                        size_t timestep,
                        WellInjector::TypeEnum wt,
                       const UnitSystem& units ) {

    if( !w.isInjector( timestep ) ) return 0;
    const auto& i = w.getInjectionProperties( timestep );

    /* we don't support mixed injectors, so querying a water injector for
     * gas rate gives 0.0
     */
    if( wt != i.injectorType ) return 0;

    if( wt == WellInjector::GAS )
        return units.from_si( UnitSystem::measure::gas_surface_rate,
                              i.surfaceInjectionRate );

    return units.from_si( UnitSystem::measure::liquid_surface_rate,
                          i.surfaceInjectionRate );
}

inline double injevol( const Well& w,
                       size_t timestep,
                       WellInjector::TypeEnum wt,
                       const UnitSystem& units ) {

    const auto rate = injerate( w, timestep, wt, units );
    return rate * units.from_si( UnitSystem::measure::time, 1 );
}

inline double get_rate( const data::Well& w,
                        rt phase,
                        const UnitSystem& units ) {
    const auto x = w.rates.get( phase, 0.0 );

    switch( phase ) {
        case rt::gas: return units.from_si( UnitSystem::measure::gas_surface_rate, x );
        default: return units.from_si( UnitSystem::measure::liquid_surface_rate, x );
    }
}

inline double get_vol( const data::Well& w,
                       rt phase,
                       const UnitSystem& units ) {

    const auto x = w.rates.get( phase, 0.0 );
    switch( phase ) {
        case rt::gas: return units.from_si( UnitSystem::measure::gas_surface_volume, x );
        default: return units.from_si( UnitSystem::measure::liquid_surface_volume, x );
    }
}

inline double well_keywords( E keyword,
                             const smspec_node_type* node,
                             const ecl_sum_tstep_type* prev,
                             const UnitSystem& units,
                             const data::Well& sim_well,
                             const Well& state_well,
                             size_t tstep ) {

    const auto* genkey = smspec_node_get_gen_key1( node );
    const auto accu = prev ? ecl_sum_tstep_get_from_key( prev, genkey ) : 0;

    /* Keyword families tend to share parameters. Since C++'s support for
     * partial application or currying is somewhat clunky (std::bind), we grow
     * our own with a handful of lambdas. The optimizer might be able to
     * reorder this function so that only the needed lambda is created (or even
     * better - inline it). This is not really a very performance sensitive
     * function, so regardless of optimisation conciseness triumphs.
     *
     * The binding of lambdas is also done for groups, fields etc.
     */
    const auto rate = [&]( rt phase )
        { return get_rate( sim_well, phase, units ); };

    const auto vol = [&]( rt phase )
        { return get_vol( sim_well, phase, units ); };

    const auto histprate = [&]( WT phase )
        { return prodrate( state_well, tstep, phase, units ); };

    const auto histpvol = [&]( WT phase )
        { return prodvol( state_well, tstep, phase, units ); };

    const auto histirate = [&]( WellInjector::TypeEnum phase )
        { return injerate( state_well, tstep, phase, units ); };

    const auto histivol = [&]( WellInjector::TypeEnum phase )
        { return injevol( state_well, tstep, phase, units ); };

    switch( keyword ) {

        /* Production rates */
        case E::WWPR: return - rate( rt::wat );
        case E::WOPR: return - rate( rt::oil );
        case E::WGPR: return - rate( rt::gas );
        case E::WLPR: return - ( rate( rt::wat ) + rate( rt::oil ) );

        /* Production totals */
        case E::WWPT: return accu - vol( rt::wat );
        case E::WOPT: return accu - vol( rt::oil );
        case E::WGPT: return accu - vol( rt::gas );
        case E::WLPT: return accu - ( vol( rt::wat ) + vol( rt::oil ) );

        /* Production history rates */
        case E::WWPRH: return histprate( WT::wat );
        case E::WOPRH: return histprate( WT::oil );
        case E::WGPRH: return histprate( WT::gas );
        case E::WLPRH: return histprate( WT::wat ) + histprate( WT::oil );

        /* Production history totals */
        case E::WWPTH: return accu + histpvol( WT::wat );
        case E::WOPTH: return accu + histpvol( WT::oil );
        case E::WGPTH: return accu + histpvol( WT::gas );
        case E::WLPTH: return accu + histpvol( WT::wat ) + histpvol( WT::oil );

        /* Production ratios */
        case E::WWCT: return wct( rate( rt::wat ), rate( rt::oil ) );

        case E::WWCTH: return wwcth( state_well, tstep );

        case E::WGOR: return glr( rate( rt::gas ), rate( rt::oil ) );
        case E::WGORH: return wgorh( state_well, tstep );

        case E::WGLR: return glr( rate( rt::gas ),
                                  rate( rt::wat ) + rate( rt::oil ) );
        case E::WGLRH: return wglrh( state_well, tstep );

        /* Pressures */
        case E::WBHP: return units.from_si( UnitSystem::measure::pressure, sim_well.bhp );
        case E::WBHPH: return 0; /* not supported */

        case E::WTHP: return units.from_si( UnitSystem::measure::pressure, sim_well.thp );
        case E::WTHPH: return 0; /* not supported */

        /* Injection rates */
        case E::WWIR: return rate( rt::wat );
        case E::WOIR: return rate( rt::oil );
        case E::WGIR: return rate( rt::gas );
        case E::WWIT: return accu + vol( rt::wat );
        case E::WOIT: return accu + vol( rt::oil );
        case E::WGIT: return accu + vol( rt::gas );

        case E::WWIRH: return histirate( WellInjector::WATER );
        case E::WOIRH: return histirate( WellInjector::OIL );
        case E::WGIRH: return histirate( WellInjector::GAS );

        case E::WWITH: return accu + histivol( WellInjector::WATER );
        case E::WOITH: return accu + histivol( WellInjector::OIL );
        case E::WGITH: return accu + histivol( WellInjector::GAS );

        case E::UNSUPPORTED:
        default:
            return -1;
    }
}

inline double sum( const std::vector< const data::Well* >& wells, rt phase ) {
    double res = 0;

    for( const auto* well : wells )
        res += well->rates.get( phase, 0 );

    return res;
}

inline double sum_rate( const std::vector< const data::Well* >& wells,
                        rt phase,
                        const UnitSystem& units ) {

    switch( phase ) {
        case rt::wat: /* intentional fall-through */
        case rt::oil: return units.from_si( UnitSystem::measure::liquid_surface_rate,
                                            sum( wells, phase ) );

        case rt::gas: return units.from_si( UnitSystem::measure::gas_surface_rate,
                                            sum( wells, phase ) );
        default: break;
    }

    throw std::runtime_error( "Reached impossible state in sum_rate" );
}

inline double sum_vol( const std::vector< const data::Well* >& wells,
                       rt phase,
                       const UnitSystem& units ) {

    switch( phase ) {
        case rt::wat: /* intentional fall-through */
        case rt::oil: return units.from_si( UnitSystem::measure::liquid_surface_volume,
                                            sum( wells, phase ) );

        case rt::gas: return units.from_si( UnitSystem::measure::gas_surface_volume,
                                            sum( wells, phase ) );
        default: break;
    }

    throw std::runtime_error( "Reached impossible state in sum_vol" );
}

template< typename F, typename Phase >
inline double sum_hist( F f,
                        const WellSet& wells,
                        size_t tstep,
                        Phase phase,
                        const UnitSystem& units ) {
    double res = 0;
    for( const auto& well : wells )
        res += f( *well.second, tstep, phase, units );

    return res;
}

inline double group_keywords( E keyword,
                              const smspec_node_type* node,
                              const ecl_sum_tstep_type* prev,
                              const UnitSystem& units,
                              size_t tstep,
                              const std::vector< const data::Well* >& sim_wells,
                              const WellSet& state_wells ) {

    const auto* genkey = smspec_node_get_gen_key1( node );
    const auto accu = prev ? ecl_sum_tstep_get_from_key( prev, genkey ) : 0;

    const auto rate = [&]( rt phase ) {
        return sum_rate( sim_wells, phase, units );
    };

    const auto vol = [&]( rt phase ) {
        return sum_vol( sim_wells, phase, units );
    };

    const auto histprate = [&]( WT phase ) {
        return sum_hist( prodrate, state_wells, tstep, phase, units );
    };

    const auto histpvol = [&]( WT phase ) {
        return sum_hist( prodvol, state_wells, tstep, phase, units );
    };

    const auto histirate = [&]( WellInjector::TypeEnum phase ) {
        return sum_hist( injerate, state_wells, tstep, phase, units );
    };

    const auto histivol = [&]( WellInjector::TypeEnum phase ) {
        return sum_hist( injevol, state_wells, tstep, phase, units );
    };

    switch( keyword ) {
        /* Production rates */
        case E::GWPR: return - rate( rt::wat );
        case E::GOPR: return - rate( rt::oil );
        case E::GGPR: return - rate( rt::gas );
        case E::GLPR: return (- rate( rt::wat )) + (- rate( rt::oil ));

        /* Production totals */
        case E::GWPT: return accu - vol( rt::wat );
        case E::GOPT: return accu - vol( rt::oil );
        case E::GGPT: return accu - vol( rt::gas );

        /* Injection rates */
        case E::GWIR: return rate( rt::wat );
        case E::GGIR: return rate( rt::gas );
        case E::GWIT: return accu + vol( rt::wat );
        case E::GGIT: return accu + vol( rt::gas );

        /* Production ratios */
        case E::GWCT: return wct( rate( rt::wat ), rate( rt::oil ) );
        case E::GGOR: return glr( rate( rt::gas ), rate( rt::oil ) );

        /* Historical rates */
        case E::GWPRH: return histprate( WT::wat );
        case E::GOPRH: return histprate( WT::oil );
        case E::GGPRH: return histprate( WT::gas );
        case E::GLPRH: return histprate( WT::wat ) + histprate( WT::oil );
        case E::GWIRH: return histirate( WellInjector::WATER );
        case E::GOIRH: return histirate( WellInjector::OIL );
        case E::GGIRH: return histirate( WellInjector::GAS );

        /* Historical totals */
        case E::GWPTH: return accu + histpvol( WT::wat );
        case E::GOPTH: return accu + histpvol( WT::oil );
        case E::GGPTH: return accu + histpvol( WT::gas );
        case E::GLPTH: return accu + histpvol( WT::wat ) + histpvol( WT::oil );
        case E::GGITH: return accu + histivol( WellInjector::GAS );
        case E::GWITH: return accu + histivol( WellInjector::WATER );

        default:
            return -1;
    }
}

}

namespace out {

Summary::Summary( const EclipseState& st, const SummaryConfig& sum ) :
    Summary( st, sum, st.getTitle().c_str() )
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
            )
{
    for( const auto& node : sum ) {

        const auto keyword = khash( node.keyword() );

        auto* nodeptr = ecl_sum_add_var( this->ecl_sum.get(), node.keyword(),
                                            node.wgname(), node.num(), "", 0 );

        const auto kw = static_cast< Summary::kwtype >( keyword );

        switch( node.type() ) {
            case ECL_SMSPEC_WELL_VAR:
                this->wvar[ node.wgname() ].emplace_back( kw, nodeptr );
                break;

            case ECL_SMSPEC_GROUP_VAR:
                this->gvar[ node.wgname() ].emplace_back( kw, nodeptr );
                break;

            default:
                break;
        }
    }
}

void Summary::add_timestep( int report_step,
                            double step_duration,
                            const EclipseState& es,
                            const data::Wells& wells ) {
    this->duration += step_duration;
    auto* tstep = ecl_sum_add_tstep( this->ecl_sum.get(), report_step, this->duration );

    /* calculate the values for the Well-family of keywords. */
    for( const auto& pair : this->wvar ) {
        const auto* wname = pair.first;

        const auto& state_well = es.getSchedule()->getWell( wname );
        const auto& sim_well = wells.at( wname );

        for( const auto& node : pair.second ) {
            auto val = well_keywords( static_cast< E >( node.kw ),
                                      node.node, this->prev_tstep,
                                      es.getUnits(), sim_well,
                                      state_well, report_step );
            ecl_sum_tstep_set_from_node( tstep, node.node, val );
        }
    }

    /* calculate the values for the Group-family of keywords. */
    for( const auto& pair : this->gvar ) {
        const auto* gname = pair.first;
        const auto& state_group = *es.getSchedule()->getGroup( gname );
        const auto& state_wells = state_group.getWells( report_step );

        std::vector< const data::Well* > sim_wells;
        for( const auto& well : state_wells )
            sim_wells.push_back( &wells.at( well.first ) );

        for( const auto& node : pair.second ) {
            auto val = group_keywords( static_cast< E >( node.kw ),
                                       node.node, this->prev_tstep,
                                       es.getUnits(), report_step,
                                       sim_wells, state_wells );
            ecl_sum_tstep_set_from_node( tstep, node.node, val );
        }
    }

    this->prev_tstep = tstep;
}

void Summary::write() {
    ecl_sum_fwrite( this->ecl_sum.get() );
}

}

}
