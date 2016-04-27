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
#include <opm/parser/eclipse/Units/ConversionFactors.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>

namespace Opm {

namespace {

/*
 * It is VERY important that the dim enum has the same order as the
 * metric and field arrays. C++ does not support designated initializers, so
 * this cannot be done in a declaration-order independent matter.
 */

enum class dim : int {
    length,
    time,
    density,
    pressure,
    temperature_absolute,
    temperature,
    viscosity,
    permeability,
    liquid_surface_volume,
    gas_surface_volume,
    volume,
    liquid_surface_rate,
    gas_surface_rate,
    rate,
    transmissibility,
    mass,
};

namespace conversions {
    /* lookup tables for SI-to-unit system
     *
     * We assume that all values in the report structures are plain SI units,
     * but output can be configured to use other (inconsistent) unit systems.
     * These lookup tables are passed to the convert function that translates
     * between SI and the target unit.
     */

const double metric[] = {
    1 / Metric::Length,
    1 / Metric::Time,
    1 / Metric::Density,
    1 / Metric::Pressure,
    1 / Metric::AbsoluteTemperature,
    1 / Metric::Temperature,
    1 / Metric::Viscosity,
    1 / Metric::Permeability,
    1 / Metric::LiquidSurfaceVolume,
    1 / Metric::GasSurfaceVolume,
    1 / Metric::ReservoirVolume,
    1 / ( Metric::LiquidSurfaceVolume / Metric::Time ),
    1 / ( Metric::GasSurfaceVolume / Metric::Time ),
    1 / ( Metric::ReservoirVolume / Metric::Time ),
    1 / Metric::Transmissibility,
    1 / Metric::Mass,
};

const double field[] = {
    1 / Field::Length,
    1 / Field::Time,
    1 / Field::Density,
    1 / Field::Pressure,
    1 / Field::AbsoluteTemperature,
    1 / Field::Temperature,
    1 / Field::Viscosity,
    1 / Field::Permeability,
    1 / Field::LiquidSurfaceVolume,
    1 / Field::GasSurfaceVolume,
    1 / Field::ReservoirVolume,
    1 / ( Field::LiquidSurfaceVolume / Field::Time ),
    1 / ( Field::GasSurfaceVolume / Field::Time ),
    1 / ( Field::ReservoirVolume / Field::Time ),
    1 / Field::Transmissibility,
    1 / Field::Mass,
};

}

/*
 * A series of helper functions to read & compute values from the simulator,
 * intended to clean up the keyword -> action mapping in the *_keyword
 * functions.
 */

inline double convert( double si_val, dim d, const double* table ) {
    return si_val * table[ static_cast< int >( d ) ];
}

using rt = data::Rates::opt;

/* posix_time -> time_t isn't in older versions of boost (at least not in
 * 1.53), so it's implemented here
 */
inline std::time_t to_time_t( boost::posix_time::ptime pt ) {
    auto dur = pt - boost::posix_time::ptime( boost::gregorian::date( 1970, 1, 1 ) );
    return std::time_t( dur.total_seconds() );
}

/* The supported Eclipse keywords */
enum class E {
    WBHP,
    WBHPH,
    WGIR,
    WGIRH,
    WGIT,
    WGITH,
    WGOR,
    WGORH,
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
    GGIR,
    GGIT,
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
    { "GGIR",  E::GGIR },
    { "GGIT",  E::GGIT },
    { "GWCT",  E::GWCT },
    { "GGOR",  E::GGOR },
};

inline const E khash( const char* key ) {
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

inline double gor( double gas, double oil ) {
    /* handle div-by-zero - if this well is shut, all production rates will be
     * zero and there is no gas/oil ratio, (i.e. zero). 
     *
     * Also, if this is a gas well that produces no oil, gas/oil ratio would be
     * undefined and is explicitly set to 0. This is the author's best guess.
     * If other semantics when just gas is produced, this must be changed.
     */
    if( oil == 0 ) return 0;

    return gas / oil;
}

inline double wgorh( const Well& w, size_t ts ) {
    /* We do not support mixed injections, and gas/oil is undefined when oil is
     * zero (i.e. pure gas injector), so always output 0 if this is an injector
     */
    if( w.isInjector( ts ) ) return 0;

    const auto& p = w.getProductionProperties( ts );

    return gor( p.GasRate, p.OilRate );
}

enum class WT { wat, oil, gas };
inline double prodrate( const Well& w,
                        size_t timestep,
                        WT wt,
                        const double* conversion_table ) {
    if( !w.isProducer( timestep ) ) return 0;

    const auto& p = w.getProductionProperties( timestep );
    switch( wt ) {
        case WT::wat: return convert( p.WaterRate, dim::liquid_surface_rate, conversion_table );
        case WT::oil: return convert( p.OilRate, dim::liquid_surface_rate, conversion_table );
        case WT::gas: return convert( p.GasRate, dim::gas_surface_rate, conversion_table );
    }

    throw std::runtime_error( "Reached impossible state in prodrate" );
}

inline double prodvol( const Well& w,
                       size_t timestep,
                       WT wt,
                       const double* conversion_table ) {
    const auto rate = prodrate( w, timestep, wt, conversion_table );
    return rate * convert( 1, dim::time, conversion_table );
}

inline double injerate( const Well& w,
                        size_t timestep,
                        WellInjector::TypeEnum wt,
                        const double* conversion_table ) {

    if( !w.isInjector( timestep ) ) return 0;
    const auto& i = w.getInjectionProperties( timestep );

    /* we don't support mixed injectors, so querying a water injector for
     * gas rate gives 0.0
     */
    if( wt != i.injectorType ) return 0;

    if( wt == WellInjector::GAS )
        return convert( i.surfaceInjectionRate, dim::gas_surface_rate, conversion_table );

    return convert( i.surfaceInjectionRate, dim::liquid_surface_rate, conversion_table );
}

inline double injevol( const Well& w,
                       size_t timestep,
                       WellInjector::TypeEnum wt,
                       const double* conversion_table ) {

    const auto rate = injerate( w, timestep, wt, conversion_table );
    return rate * convert( 1, dim::time, conversion_table );
}

inline double get_rate( const data::Well& w,
                        rt phase,
                        const double* conversion_table ) {
    const auto x = w.rates.get( phase, 0.0 );

    switch( phase ) {
        case rt::gas: return convert( x, dim::gas_surface_rate, conversion_table );
        default: return convert( x, dim::liquid_surface_rate, conversion_table );
    }
}

inline double get_vol( const data::Well& w,
                       rt phase,
                       const double* conversion_table ) {
    const auto x = w.rates.get( phase, 0.0 );
    switch( phase ) {
        case rt::gas: return convert( x, dim::gas_surface_volume, conversion_table );
        default: return convert( x, dim::liquid_surface_volume, conversion_table );
    }
}

inline double well_keywords( const smspec_node_type* node,
                             const ecl_sum_tstep_type* prev,
                             const double* conversion_table,
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
        { return get_rate( sim_well, phase, conversion_table ); };

    const auto vol = [&]( rt phase )
        { return get_vol( sim_well, phase, conversion_table ); };

    const auto histprate = [&]( WT phase )
        { return prodrate( state_well, tstep, phase, conversion_table ); };

    const auto histpvol = [&]( WT phase )
        { return prodvol( state_well, tstep, phase, conversion_table ); };

    const auto histirate = [&]( WellInjector::TypeEnum phase )
        { return injerate( state_well, tstep, phase, conversion_table ); };

    const auto histivol = [&]( WellInjector::TypeEnum phase )
        { return injevol( state_well, tstep, phase, conversion_table ); };

    switch( khash( smspec_node_get_keyword( node ) ) ) {

        /* Production rates */
        case E::WWPR: return rate( rt::wat );
        case E::WOPR: return rate( rt::oil );
        case E::WGPR: return rate( rt::gas );
        case E::WLPR: return rate( rt::wat ) + rate( rt::oil );

        /* Production totals */
        case E::WWPT: return accu + vol( rt::wat );
        case E::WOPT: return accu + vol( rt::oil );
        case E::WGPT: return accu + vol( rt::gas );
        case E::WLPT: return accu + vol( rt::wat ) + vol( rt::oil );

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

        case E::WGOR: return gor( rate( rt::gas ), rate( rt::oil ) );
        case E::WGORH: return wgorh( state_well, tstep );

        /* Pressures */
        case E::WBHP: return convert( sim_well.bhp, dim::pressure, conversion_table );
        case E::WBHPH: return 0; /* not supported */

        case E::WTHP: return convert( sim_well.thp, dim::pressure, conversion_table );
        case E::WTHPH: return 0; /* not supported */

        /* Injection rates */
        /* TODO: read from sim or compute (how?) */
        /* TODO: Tests */
        case E::WWIR: return - rate( rt::wat );
        case E::WOIR: return - rate( rt::oil );
        case E::WGIR: return - rate( rt::gas );
        case E::WWIT: return accu - vol( rt::wat );
        case E::WOIT: return accu - vol( rt::oil );
        case E::WGIT: return accu - vol( rt::gas );

        case E::WWIRH: return histirate( WellInjector::WATER );
        case E::WOIRH: return histirate( WellInjector::GAS );
        case E::WGIRH: return histirate( WellInjector::OIL );

        case E::WWITH: return accu + histivol( WellInjector::WATER );
        case E::WOITH: return accu + histivol( WellInjector::GAS );
        case E::WGITH: return accu + histivol( WellInjector::OIL );

        case E::UNSUPPORTED:
        default:
            return 0;
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
                   const double* conversion_table ) {

    switch( phase ) {
        case rt::wat: /* intentional fall-through */
        case rt::oil: return convert( sum( wells, phase ),
                                      dim::liquid_surface_rate,
                                      conversion_table );

        case rt::gas: return convert( sum( wells, phase ),
                                      dim::gas_surface_rate,
                                      conversion_table );
        default: break;
    }

    throw std::runtime_error( "Reached impossible state in prodrate" );
}

inline double sum_vol( const std::vector< const data::Well* >& wells,
                   rt phase,
                   const double* conversion_table ) {

    switch( phase ) {
        case rt::wat: /* intentional fall-through */
        case rt::oil: return convert( sum( wells, phase ),
                                      dim::liquid_surface_volume,
                                      conversion_table );

        case rt::gas: return convert( sum( wells, phase ),
                                      dim::gas_surface_volume,
                                      conversion_table );
        default: break;
    }

    throw std::runtime_error( "Reached impossible state in prodrate" );
}

inline double group_keywords( const smspec_node_type* node,
                              const ecl_sum_tstep_type* prev,
                              const double* conversion_table,
                              const std::vector< const data::Well* > sim_wells ) {

    const auto* genkey = smspec_node_get_gen_key1( node );
    const auto accu = prev ? ecl_sum_tstep_get_from_key( prev, genkey ) : 0;

    const auto rate = [&]( rt phase ) {
        return sum_rate( sim_wells, phase, conversion_table );
    };

    const auto vol = [&]( rt phase ) {
        return sum_vol( sim_wells, phase, conversion_table );
    };

    switch( khash( smspec_node_get_keyword( node ) ) ) {
        /* Production rates */
        case E::GWPR: return rate( rt::wat );
        case E::GOPR: return rate( rt::oil );
        case E::GGPR: return rate( rt::gas );
        case E::GLPR: return rate( rt::wat ) + rate( rt::oil );

        /* Production totals */
        case E::GWPT: return accu + vol( rt::wat );
        case E::GOPT: return accu + vol( rt::oil );
        case E::GGPT: return accu + vol( rt::gas );

        /* Injection rates */
        case E::GWIR: return - rate( rt::wat );
        case E::GGIR: return - rate( rt::gas );
        case E::GGIT: return accu - vol( rt::gas );

        /* Production ratios */
        case E::GWCT: return wct( rate( rt::wat ), rate( rt::oil ) );
        case E::GGOR: return gor( rate( rt::gas ), rate( rt::oil ) );

        default:
            return 0;
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

static inline const double* get_conversions( const EclipseState& es ) {
    using namespace conversions;

    switch( es.getDeckUnitSystem().getType() ) {
        case UnitSystem::UNIT_TYPE_METRIC: return metric;
        case UnitSystem::UNIT_TYPE_FIELD: return field;
        default: return metric;
    }
}

Summary::Summary( const EclipseState& st,
                  const SummaryConfig& sum,
                  const char* basename ) :
    ecl_sum( 
            ecl_sum_alloc_writer( 
                basename,
                st.getIOConfig()->getFMTOUT(),
                st.getIOConfig()->getUNIFOUT(),
                ":",
                to_time_t( st.getSchedule()->getStartTime() ),
                true,
                st.getInputGrid()->getNX(),
                st.getInputGrid()->getNY(),
                st.getInputGrid()->getNZ()
                )
            ),
    conversions( get_conversions( st ) )
{
    for( const auto& node : sum ) {
        auto* nodeptr = ecl_sum_add_var( this->ecl_sum.get(), node.keyword(),
                                            node.wgname(), node.num(), "", 0 );

        switch( smspec_node_get_var_type( nodeptr ) ) {
            case ECL_SMSPEC_WELL_VAR:
                this->wvar[ node.wgname() ].push_back( nodeptr );
                break;

            case ECL_SMSPEC_GROUP_VAR:
                this->gvar[ node.wgname() ].push_back( nodeptr );
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

        for( const auto* node : pair.second ) {
            auto val = well_keywords( node, this->prev_tstep,
                                      this->conversions, sim_well,
                                      state_well, report_step );
            ecl_sum_tstep_set_from_node( tstep, node, val );
        }
    }

    /* calculate the values for the Group-family of keywords. */
    for( const auto& pair : this->gvar ) {
        const auto* gname = pair.first;
        const auto& state_group = *es.getSchedule()->getGroup( gname );

        std::vector< const data::Well* > sim_wells;
        for( const auto& well : state_group.getWells( report_step ) )
            sim_wells.push_back( &wells.at( well.first ) );

        for( const auto* node : pair.second ) {
            auto val = group_keywords( node, this->prev_tstep,
                                      this->conversions, sim_wells );
            ecl_sum_tstep_set_from_node( tstep, node, val );
        }
    }

    this->prev_tstep = tstep;
}

void Summary::write() {
    ecl_sum_fwrite( this->ecl_sum.get() );
}

}

}
