/*
  Copyright (c) 2013-2015 Andreas Lauser
  Copyright (c) 2013 SINTEF ICT, Applied Mathematics.
  Copyright (c) 2013 Uni Research AS
  Copyright (c) 2015 IRIS AS

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
#include "config.h"

#include "EclipseWriter.hpp"

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Units/Dimension.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperty.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/CompletionSet.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/Utility/Functional.hpp>
#include <opm/output/eclipse/Summary.hpp>

#include <cstdlib>
#include <memory>     // unique_ptr
#include <utility>    // move

#include <ert/ecl/EclKW.hpp>
#include <ert/ecl/FortIO.hpp>
#include <ert/ecl/ecl_kw_magic.h>
#include <ert/ecl/ecl_init_file.h>
#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_rft_file.h>
#include <ert/ecl/ecl_rst_file.h>
#include <ert/ecl_well/well_const.h>
#include <ert/ecl/ecl_rsthead.h>
#include <ert/util/util.h>
#define OPM_XWEL      "OPM_XWEL"

// namespace start here since we don't want the ERT headers in it
namespace Opm {
namespace {

// throw away the data for all non-active cells and reorder to the Cartesian logic of
// eclipse
void restrictAndReorderToActiveCells(std::vector<double> &data,
                                     int numCells,
                                     const int* compressedToCartesianCellIdx)
{
    if (!compressedToCartesianCellIdx)
        // if there is no active -> global mapping, all cells
        // are considered active
        return;

    std::vector<double> eclData;
    eclData.reserve( numCells );

    // activate those cells that are actually there
    for (int i = 0; i < numCells; ++i) {
        eclData.push_back( data[ compressedToCartesianCellIdx[i] ] );
    }
    data.swap( eclData );
}

inline void convertFromSiTo( std::vector< double >& siValues,
                                    const UnitSystem& units,
                                    UnitSystem::measure m ) {
    for (size_t curIdx = 0; curIdx < siValues.size(); ++curIdx) {
        siValues[curIdx] = units.from_si( m, siValues[ curIdx ] );
    }
}

inline int to_ert_welltype( const Well& well, size_t timestep ) {

    if( well.isProducer( timestep ) ) return IWEL_PRODUCER;

    switch( well.getInjectionProperties( timestep ).injectorType ) {
        case WellInjector::WATER: return IWEL_WATER_INJECTOR;
        case WellInjector::GAS: return IWEL_GAS_INJECTOR;
        case WellInjector::OIL: return IWEL_OIL_INJECTOR;
        default: return IWEL_UNDOCUMENTED_ZERO;
    }
}

/**
 * Pointer to memory that holds the name to an Eclipse output file.
 */
class FileName {
public:
    FileName(const std::string& outputDir,
             const std::string& baseName,
             ecl_file_enum type,
             int writeStepIdx,
             bool formatted ) :
        filename( ecl_util_alloc_filename(
                                outputDir.c_str(),
                                baseName.c_str(),
                                type,
                                formatted,
                                writeStepIdx ),
                std::free )
    {}

    const char* ertHandle() const { return this->filename.get(); }

private:
    using fd = std::unique_ptr< char, decltype( std::free )* >;
    fd filename;
};

class Restart {
public:
    static const int NIWELZ = 11; //Number of data elements per well in IWEL array in restart file
    static const int NZWELZ = 3;  //Number of 8-character words per well in ZWEL array restart file
    static const int NICONZ = 15; //Number of data elements per completion in ICON array restart file

    /**
     * The constants NIWELZ and NZWELZ referes to the number of elements per
     * well that we write to the IWEL and ZWEL eclipse restart file data
     * arrays. The constant NICONZ refers to the number of elements per
     * completion in the eclipse restart file ICON data array.These numbers are
     * written to the INTEHEAD header.

     * The elements are added in the method addRestartFileIwelData(...) and and
     * addRestartFileIconData(...), respectively.  We write as many elements
     * that we need to be able to view the restart file in Resinsight.  The
     * restart file will not be possible to restart from with Eclipse, we write
     * to little information to be able to do this.
     *
     * Observe that all of these values are our "current-best-guess" for how
     * many numbers are needed; there might very well be third party
     * applications out there which have a hard expectation for these values.
    */


    Restart(const std::string& outputDir,
            const std::string& baseName,
            int writeStepIdx,
            const IOConfig& ioConfig ) :
        filename( outputDir,
                baseName,
                ioConfig.getUNIFOUT() ? ECL_UNIFIED_RESTART_FILE : ECL_RESTART_FILE,
                writeStepIdx,
                ioConfig.getFMTOUT() ),
        rst_file(
                ( writeStepIdx > 0 && ioConfig.getUNIFOUT() )
                ? ecl_rst_file_open_append( filename.ertHandle() )
                : ecl_rst_file_open_write( filename.ertHandle() ) )
    {}

    template< typename T >
    void add_kw( ERT::EclKW< T >&& kw ) {
        ecl_rst_file_add_kw( this->rst_file.get(), kw.get() );
    }

    void addRestartFileIwelData( std::vector<int>& data,
                                 size_t step,
                                 const Well& well,
                                 size_t offset ) const {

        CompletionSetConstPtr completions = well.getCompletions( step );

        data[ offset + IWEL_HEADI_ITEM ] = well.getHeadI() + 1;
        data[ offset + IWEL_HEADJ_ITEM ] = well.getHeadJ() + 1;
        data[ offset + IWEL_CONNECTIONS_ITEM ] = completions->size();
        data[ offset + IWEL_GROUP_ITEM ] = 1;

        data[ offset + IWEL_TYPE_ITEM ] = to_ert_welltype( well, step );
        data[ offset + IWEL_STATUS_ITEM ] =
            well.getStatus( step ) == WellCommon::OPEN ? 1 : 0;
    }

    void addRestartFileIconData( std::vector< int >& data,
                                 CompletionSetConstPtr completions,
                                 size_t wellICONOffset ) const {

        for( size_t i = 0; i < completions->size(); ++i ) {
            const auto& completion = *completions->get( i );
            size_t offset = wellICONOffset + i * Restart::NICONZ;
            data[ offset + ICON_IC_ITEM ] = 1;

            data[ offset + ICON_I_ITEM ] = completion.getI() + 1;
            data[ offset + ICON_J_ITEM ] = completion.getJ() + 1;
            data[ offset + ICON_K_ITEM ] = completion.getK() + 1;

            const auto open = WellCompletion::StateEnum::OPEN;
            data[ offset + ICON_STATUS_ITEM ] = completion.getState() == open
                                              ? 1
                                              : 0;

            data[ offset + ICON_DIRECTION_ITEM ] = completion.getDirection();
        }
    }

    void writeHeader( int stepIdx, ecl_rsthead_type* rsthead_data ) {
      ecl_util_set_date_values( rsthead_data->sim_time,
                                &rsthead_data->day,
                                &rsthead_data->month,
                                &rsthead_data->year );
      ecl_rst_file_fwrite_header( this->rst_file.get(), stepIdx, rsthead_data );

    }

    ecl_rst_file_type* ertHandle() { return this->rst_file.get(); }
    const ecl_rst_file_type* ertHandle() const { return this->rst_file.get(); }

private:
    FileName filename;
    ERT::ert_unique_ptr< ecl_rst_file_type, ecl_rst_file_close > rst_file;
};

/**
 * The Solution class wraps the actions that must be done to the restart file while
 * writing solution variables; it is not a handle on its own.
 */
class Solution {
public:
    Solution( Restart& res ) : restart( res ) {
        ecl_rst_file_start_solution( res.ertHandle() );
    }

    template< typename T >
    void add( ERT::EclKW< T >&& kw ) {
        ecl_rst_file_add_kw( this->restart.ertHandle(), kw.get() );
    }

    ecl_rst_file_type* ertHandle() { return this->restart.ertHandle(); }

    ~Solution() { ecl_rst_file_end_solution( this->restart.ertHandle() ); }

private:
    Restart& restart;
};

/**
 * Initialization file which contains static properties (such as
 * porosity and permeability) for the simulation field.
 */
class Init {
public:
    Init( const std::string& outputDir,
          const std::string& baseName,
          int writeStepIdx,
          const IOConfig& ioConfig ) :
        egrid( outputDir,
               baseName,
               ECL_EGRID_FILE,
               writeStepIdx,
               ioConfig.getFMTOUT() ),
        init( FileName( outputDir,
                        baseName,
                        ECL_INIT_FILE,
                        writeStepIdx,
                        ioConfig.getFMTOUT() ).ertHandle(),
              std::ios_base::out,
              ioConfig.getFMTOUT(),
              ECL_ENDIAN_FLIP )
    {}

    void writeHeader( int numCells,
                      const int* compressedToCartesianCellIdx,
                      time_t current_posix_time,
                      const EclipseState& es,
                      const EclipseGrid& eclGrid,
                      int ert_phase_mask )
    {
        auto dataField = es.get3DProperties().getDoubleGridProperty("PORO").getData();
        restrictAndReorderToActiveCells( dataField, numCells, compressedToCartesianCellIdx );

        // finally, write the grid to disk
        const auto& ioConfig = *es.getIOConfig();
        if( ioConfig.getWriteEGRIDFile() ) {
            const bool is_metric =
                es.getDeckUnitSystem().getType() == UnitSystem::UNIT_TYPE_METRIC;
            eclGrid.fwriteEGRID( egrid.ertHandle(), is_metric );
        }


        if( ioConfig.getWriteINITFile() ) {
            ERT::EclKW< float > poro_kw( "PORO", dataField );
            ecl_init_file_fwrite_header( this->ertHandle(),
                                         eclGrid.c_ptr(),
                                         poro_kw.get(),
                                         ert_phase_mask,
                                         current_posix_time );
        }
    }

    void writeKeyword( const std::string& keywordName,
                       const std::vector<double> &data ) {
        ERT::EclKW< float > kw( keywordName, data );
        ecl_kw_fwrite( kw.get(), this->ertHandle() );
    }

    fortio_type* ertHandle() { return this->init.get(); }

private:
    FileName egrid;
    ERT::FortIO init;
};

const int inactive_index = -1;

/// Convert OPM phase usage to ERT bitmask
inline int ertPhaseMask( const TableManager& tm ) {
    return ( tm.hasPhase( Phase::PhaseEnum::WATER ) ? ECL_WATER_PHASE : 0 )
         | ( tm.hasPhase( Phase::PhaseEnum::OIL ) ? ECL_OIL_PHASE : 0 )
         | ( tm.hasPhase( Phase::PhaseEnum::GAS ) ? ECL_GAS_PHASE : 0 );
}

class RFT {
    public:
        RFT( const char* output_dir,
             const char* basename,
             bool format,
             const int* compressed_to_cartesian,
             size_t num_cells,
             size_t cartesian_size );

        void writeTimeStep( std::vector< const Well* >,
                            const EclipseGrid& grid,
                            int report_step,
                            time_t current_time,
                            double days,
                            ert_ecl_unit_enum,
                            const std::vector< double >& pressure,
                            const std::vector< double >& swat,
                            const std::vector< double >& sgas );
    private:
        std::vector< int > global_to_active;
        ERT::FortIO fortio;
};

RFT::RFT( const char* output_dir,
          const char* basename,
          bool format,
          const int* compressed_to_cartesian,
          size_t num_cells,
          size_t cart_size ) :
    global_to_active( cart_size, inactive_index ),
    fortio(
        FileName( output_dir, basename, ECL_RFT_FILE, format, 0 ).ertHandle(),
        std::ios_base::out
        )
{
    if( !compressed_to_cartesian ) {
        /* without a global to active mapping we assume identity mapping, i.e.
         * 0 -> 0, 1 -> 1 etc.
         */
        fun::iota range( num_cells );
        std::copy( range.begin(), range.end(), this->global_to_active.begin() );
        return;
    }

    for( size_t active_index = 0; active_index < num_cells; ++active_index )
        global_to_active[ compressed_to_cartesian[ active_index ] ] = active_index;
}

inline ert_ecl_unit_enum to_ert_unit( UnitSystem::UnitType t ) {
    switch ( t ) {
        case UnitSystem::UNIT_TYPE_METRIC: return ERT_ECL_METRIC_UNITS;
        case UnitSystem::UNIT_TYPE_FIELD: return ERT_ECL_FIELD_UNITS;
        case UnitSystem::UNIT_TYPE_LAB: return ERT_ECL_LAB_UNITS;
    }

    throw std::invalid_argument("unhandled enum value");
}

void RFT::writeTimeStep( std::vector< const Well* > wells,
                         const EclipseGrid& grid,
                         int report_step,
                         time_t current_time,
                         double days,
                         ert_ecl_unit_enum unitsystem,
                         const std::vector< double >& pressure,
                         const std::vector< double >& swat,
                         const std::vector< double >& sgas ) {

    using rft = ERT::ert_unique_ptr< ecl_rft_node_type, ecl_rft_node_free >;

    for( const auto& well : wells ) {
        if( !( well->getRFTActive( report_step )
            || well->getPLTActive( report_step ) ) )
            continue;

        auto* rft_node = ecl_rft_node_alloc_new( well->name().c_str(), "RFT",
                current_time, days );

        for( const auto& completion : *well->getCompletions( report_step ) ) {
            const size_t i = size_t( completion->getI() );
            const size_t j = size_t( completion->getJ() );
            const size_t k = size_t( completion->getK() );

            if( !grid.cellActive( i, j, k ) ) continue;

            const auto index = grid.activeIndex( i, j, k );
            const double depth = grid.getCellDepth( i, j, k );
            const double press = !pressure.empty() ? pressure[ index ] : 0.0;
            const double satwat = !swat.empty() ? swat[ index ] : 0.0;
            const double satgas = !sgas.empty() ? sgas[ index ] : 0.0;

            auto* cell = ecl_rft_cell_alloc_RFT(
                            i, j, k, depth, press, satwat, satgas );

            ecl_rft_node_append_cell( rft_node, cell );
        }

        rft ecl_node( rft_node );
        ecl_rft_node_fwrite( ecl_node.get(), this->fortio.get(), unitsystem );
    }
}

inline std::string uppercase( std::string x ) {
    std::transform( x.begin(), x.end(), x.begin(),
        []( char c ) { return std::toupper( c ); } );

    return x;
}

}

class EclipseWriter::Impl {
    public:
        Impl( std::shared_ptr< const EclipseState > es,
              int numCells,
              const int* comp_to_cart,
              const NNC& );

        std::shared_ptr< const EclipseState > es;
        EclipseGrid grid;
        const NNC& nnc;
        std::string outputDir;
        std::string baseName;
        out::Summary summary;
        RFT rft;
        time_t sim_start_time;
        int numCells;
        std::array< int, 3 > cartesianSize;
        const int* compressed_to_cartesian;
        std::vector< int > gridToEclipseIdx;
        bool output_enabled;
        int ert_phase_mask;
};

EclipseWriter::Impl::Impl( std::shared_ptr< const EclipseState > eclipseState,
                           int numCellsArg,
                           const int* compressed_to_cart,
                           const NNC& rnnc )
    : es( eclipseState )
    , grid( *eclipseState->getInputGrid() )
    , nnc( rnnc )
    , outputDir( eclipseState->getIOConfig()->getOutputDir() )
    , baseName( uppercase( eclipseState->getIOConfig()->getBaseName() ) )
    , summary( *eclipseState, eclipseState->getSummaryConfig() )
    , rft( outputDir.c_str(), baseName.c_str(),
           es->getIOConfig()->getFMTOUT(),
           compressed_to_cart,
           numCellsArg, es->getInputGrid()->getCartesianSize() )
    , sim_start_time( es->getSchedule()->posixStartTime() )
    , numCells( numCellsArg )
    , compressed_to_cartesian( compressed_to_cart )
    , gridToEclipseIdx( numCells, int(-1) )
    , output_enabled( eclipseState->getIOConfig()->getOutputEnabled() )
    , ert_phase_mask( ertPhaseMask( eclipseState->getTableManager() ) )
{}

void EclipseWriter::writeInit() {
    if( !this->impl->output_enabled )
        return;

    const auto& es = *this->impl->es;
    Init fortio( this->impl->outputDir,
                 this->impl->baseName,
                 /*stepIdx=*/0,
                 *es.getIOConfigConst());

    fortio.writeHeader( this->impl->numCells,
                        this->impl->compressed_to_cartesian,
                        this->impl->sim_start_time,
                        es,
                        this->impl->grid,
                        this->impl->ert_phase_mask );

    IOConfigConstPtr ioConfig = es.getIOConfigConst();
    const auto& props = es.get3DProperties();

    if( !ioConfig->getWriteINITFile() ) return;

    const auto& gridToEclipseIdx = this->impl->gridToEclipseIdx;
    const auto& units = es.getUnits();

    if (props.hasDeckDoubleGridProperty("PERMX")) {
        auto data = props.getDoubleGridProperty("PERMX").getData();
        convertFromSiTo( data,
                         units,
                         UnitSystem::measure::permeability );
        restrictAndReorderToActiveCells(data, gridToEclipseIdx.size(), gridToEclipseIdx.data());
        fortio.writeKeyword("PERMX", data);
    }
    if (props.hasDeckDoubleGridProperty("PERMY")) {
        auto data = props.getDoubleGridProperty("PERMY").getData();
        convertFromSiTo( data,
                         units,
                         UnitSystem::measure::permeability );
        restrictAndReorderToActiveCells(data, gridToEclipseIdx.size(), gridToEclipseIdx.data());
        fortio.writeKeyword("PERMY", data);
    }
    if (props.hasDeckDoubleGridProperty("PERMZ")) {
        auto data = props.getDoubleGridProperty("PERMZ").getData();
        convertFromSiTo( data,
                         units,
                         UnitSystem::measure::permeability );
        restrictAndReorderToActiveCells(data, gridToEclipseIdx.size(), gridToEclipseIdx.data());
        fortio.writeKeyword("PERMZ", data);
    }

    if( !this->impl->nnc.hasNNC() ) return;

    std::vector<double> tran;
    for( NNCdata nd : this->impl->nnc.nncdata() ) {
        tran.push_back( nd.trans );
    }

    convertFromSiTo( tran, units, UnitSystem::measure::transmissibility );
    fortio.writeKeyword("TRANNNC", tran);

}

// implementation of the writeTimeStep method
void EclipseWriter::writeTimeStep(int report_step,
                                  double secs_elapsed,
                                  data::Solution cells,
                                  data::Wells wells,
                                  bool  isSubstep)
{

    if( !this->impl->output_enabled )
        return;

    using dc = data::Solution::key;

    time_t current_posix_time = this->impl->sim_start_time + secs_elapsed;
    const auto& gridToEclipseIdx = this->impl->gridToEclipseIdx;
    const auto& es = *this->impl->es;
    const auto& grid = this->impl->grid;
    const auto& units = es.getUnits();

    auto& pressure = cells[ dc::PRESSURE ];
    convertFromSiTo( pressure,
                     units,
                     UnitSystem::measure::pressure );
    restrictAndReorderToActiveCells(pressure, gridToEclipseIdx.size(), gridToEclipseIdx.data());

    if( cells.has( dc::SWAT ) ) {
        auto& saturation_water = cells[ dc::SWAT ];
        restrictAndReorderToActiveCells(saturation_water, gridToEclipseIdx.size(), gridToEclipseIdx.data());
    }


    if( cells.has( dc::SGAS ) ) {
        auto& saturation_gas = cells[ dc::SGAS ];
        restrictAndReorderToActiveCells(saturation_gas, gridToEclipseIdx.size(), gridToEclipseIdx.data());
    }

    IOConfigConstPtr ioConfig = this->impl->es->getIOConfigConst();


    const auto days = units.from_si( UnitSystem::measure::time, secs_elapsed );
    const auto& schedule = *es.getSchedule();

    // Write restart file
    if(!isSubstep && ioConfig->getWriteRestartFile(report_step))
    {
        const size_t ncwmax     = schedule.getMaxNumCompletionsForWells(report_step);
        const size_t numWells   = schedule.numWells(report_step);
        auto wells_ptr          = schedule.getWells(report_step);

        std::vector<const char*> zwell_data( numWells * Restart::NZWELZ , "");
        std::vector<int>         iwell_data( numWells * Restart::NIWELZ , 0 );
        std::vector<int>         icon_data( numWells * ncwmax * Restart::NICONZ , 0 );

        Restart restartHandle( this->impl->outputDir,
                               this->impl->baseName,
                               report_step,
                               *ioConfig);

        for (size_t iwell = 0; iwell < wells_ptr.size(); ++iwell) {
            const auto& well = *wells_ptr[iwell];
            {
                size_t wellIwelOffset = Restart::NIWELZ * iwell;
                restartHandle.addRestartFileIwelData(iwell_data, report_step, well , wellIwelOffset);
            }
            {
                size_t wellIconOffset = ncwmax * Restart::NICONZ * iwell;
                restartHandle.addRestartFileIconData(icon_data,  well.getCompletions( report_step ), wellIconOffset);
            }
            zwell_data[ iwell * Restart::NZWELZ ] = well.name().c_str();
        }


        {
            ecl_rsthead_type rsthead_data = {};
            rsthead_data.sim_time   = current_posix_time;
            rsthead_data.nactive    = this->impl->numCells;
            rsthead_data.nx         = grid.getNX();
            rsthead_data.ny         = grid.getNY();
            rsthead_data.nz         = grid.getNZ();
            rsthead_data.nwells     = numWells;
            rsthead_data.niwelz     = Restart::NIWELZ;
            rsthead_data.nzwelz     = Restart::NZWELZ;
            rsthead_data.niconz     = Restart::NICONZ;
            rsthead_data.ncwmax     = ncwmax;
            rsthead_data.phase_sum  = this->impl->ert_phase_mask;
            rsthead_data.sim_days   = days;

            restartHandle.writeHeader( report_step, &rsthead_data);
        }

        const auto sz = wells.bhp.size() + wells.perf_pressure.size()
                      + wells.perf_rate.size() + wells.temperature.size()
                      + wells.well_rate.size();
        std::vector< double > xwel;
        xwel.reserve( sz );

        for( const auto& vec : { wells.bhp, wells.temperature, wells.well_rate,
                                 wells.perf_pressure, wells.perf_rate } )
            xwel.insert( xwel.end(), vec.begin(), vec.end() );

        restartHandle.add_kw( ERT::EclKW< int >(IWEL_KW, iwell_data) );
        restartHandle.add_kw( ERT::EclKW< const char* >(ZWEL_KW, zwell_data ) );
        restartHandle.add_kw( ERT::EclKW< double >(OPM_XWEL, xwel ) );
        restartHandle.add_kw( ERT::EclKW< int >( ICON_KW, icon_data ) );


        Solution sol(restartHandle);
        sol.add( ERT::EclKW<float>("PRESSURE", pressure) );


        // write the cell temperature
        auto& temperature = cells[ dc::TEMP ];
        convertFromSiTo( temperature,
                         units,
                         UnitSystem::measure::temperature );
        sol.add( ERT::EclKW<float>("TEMP", temperature) );


        if( cells.has( dc::SWAT ) ) {
            sol.add( ERT::EclKW<float>( "SWAT", cells[ dc::SWAT ] ) );
        }


        if( cells.has( dc::SGAS ) ) {
            sol.add( ERT::EclKW<float>( "SGAS", cells[ dc::SGAS ] ) );
        }

        // Write RS - Dissolved GOR
        if( cells.has( dc::RS ) )
            sol.add( ERT::EclKW<float>("RS", cells[ dc::RS ] ) );

        // Write RV - Volatilized oil/gas ratio
        if( cells.has( dc::RV ) )
            sol.add( ERT::EclKW<float>("RV", cells[ dc::RV ] ) );
    }

    const auto unit_type = es.getDeckUnitSystem().getType();
    this->impl->rft.writeTimeStep( schedule.getWells( report_step ),
                                   grid,
                                   report_step,
                                   current_posix_time,
                                   days,
                                   to_ert_unit( unit_type ),
                                   pressure,
                                   cells[ dc::SWAT ],
                                   cells[ dc::SGAS ] );

    if( isSubstep ) return;

    this->impl->summary.add_timestep( report_step,
                                      secs_elapsed,
                                      es,
                                      wells );
    this->impl->summary.write();
}

EclipseWriter::EclipseWriter( std::shared_ptr< const EclipseState > es,
                              int numCells,
                              const int* compressedToCartesianCellIdx,
                              const NNC& nnc ) :
    impl( new Impl( es, numCells, compressedToCartesianCellIdx, nnc ) )
{
    if( compressedToCartesianCellIdx ) {
        // if compressedToCartesianCellIdx available then
        // compute mapping to eclipse order
        std::map< int , int > indexMap;
        for (int cellIdx = 0; cellIdx < numCells; ++cellIdx) {
            int cartesianCellIdx = compressedToCartesianCellIdx[cellIdx];
            indexMap[ cartesianCellIdx ] = cellIdx;
        }

        int idx = 0;
        for( auto it = indexMap.begin(), end = indexMap.end(); it != end; ++it ) {
            this->impl->gridToEclipseIdx[ idx++ ] = (*it).second;
        }
    }
    else {
        // if not compressedToCartesianCellIdx was given use identity
        for (int cellIdx = 0; cellIdx < numCells; ++cellIdx) {
            this->impl->gridToEclipseIdx[ cellIdx ] = cellIdx;
        }
    }

    auto& grid = this->impl->grid;

    // update the ACTNUM array using the processed cornerpoint grid
    std::vector< int > actnumData( grid.getCartesianSize(), 1 );
    if ( compressedToCartesianCellIdx ) {
        actnumData.assign( actnumData.size(), 0 );

        for( int cellIdx = 0; cellIdx < numCells; ++cellIdx )
            actnumData[ compressedToCartesianCellIdx[ cellIdx ] ] = 1;
    }

    grid.resetACTNUM( &actnumData[0] );

    if( nnc.hasNNC() ) {
        int idx = 0;
        // const_cast is safe, since this is a copy of the input grid
        auto* ecl_grid = const_cast< ecl_grid_type* >( grid.c_ptr() );
        for( NNCdata n : nnc.nncdata() ) {
            ecl_grid_add_self_nnc( ecl_grid, n.cell1, n.cell2, idx++);
        }
    }

    if( !this->impl->output_enabled ) return;

    const auto& outputDir = this->impl->outputDir;

    // make sure that the output directory exists, if not try to create it
    if ( !util_entry_exists( outputDir.c_str() ) ) {
        util_make_path( outputDir.c_str() );
    }

    if( !util_is_directory( outputDir.c_str() ) ) {
        throw std::runtime_error( "The path specified as output directory '"
                                  + outputDir + "' is not a directory");
    }
}

EclipseWriter::~EclipseWriter() {}

} // namespace Opm
