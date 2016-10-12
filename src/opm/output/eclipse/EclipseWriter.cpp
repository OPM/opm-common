/*
  Copyright (c) 2016 Statoil ASA
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
#include <unordered_map>

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
#include <opm/output/eclipse/RegionCache.hpp>

#include <cstdlib>
#include <memory>     // unique_ptr
#include <utility>    // move

#include <ert/ecl/EclKW.hpp>
#include <ert/ecl/FortIO.hpp>
#include <ert/ecl/ecl_kw_magic.h>
#include <ert/ecl/ecl_init_file.h>
#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_grid.h>
#include <ert/ecl/ecl_rft_file.h>
#include <ert/ecl/ecl_rst_file.h>
#include <ert/ecl_well/well_const.h>
#include <ert/ecl/ecl_rsthead.h>
#include <ert/util/util.h>
#define OPM_XWEL      "OPM_XWEL"
#define OPM_IWEL      "OPM_IWEL"

// namespace start here since we don't want the ERT headers in it
namespace Opm {
namespace {


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


void writeKeyword( ERT::FortIO& fortio ,
                   const std::string& keywordName,
                   const std::vector<int> &data ) {
    ERT::EclKW< int > kw( keywordName, data );
    kw.fwrite( fortio );
}

/*
  This overload hardcodes the common assumption that properties which
  are stored internally as double values in OPM should be stored as
  float values in the ECLIPSE formatted binary files.
*/

void writeKeyword( ERT::FortIO& fortio ,
                   const std::string& keywordName,
                   const std::vector<double> &data) {

    ERT::EclKW< float > kw( keywordName, data );
    kw.fwrite( fortio );

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

    FileName(const std::string& outputDir,
             const std::string& baseName,
             ecl_file_enum type,
             bool formatted ) :

        filename( ecl_util_alloc_filename(
                                outputDir.c_str(),
                                baseName.c_str(),
                                type,
                                formatted,
                                0 ),
                  std::free )
    {}

    const char* get() const { return this->filename.get(); }

private:
    using fd = std::unique_ptr< char, decltype( std::free )* >;
    fd filename;
};

using restart_file = ERT::ert_unique_ptr< ecl_rst_file_type, ecl_rst_file_close >;

restart_file open_rst( const char* filename,
          bool first_restart,
          bool unifout,
          int report_step ) {

    if( !unifout )
        return restart_file{ ecl_rst_file_open_write( filename ) };

    if( first_restart )
        return restart_file{ ecl_rst_file_open_write_seek( filename, report_step ) };

    return restart_file{ ecl_rst_file_open_append( filename ) };
}

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
            const IOConfig& ioConfig,
            bool first_restart ) :
        filename( outputDir,
                baseName,
                ioConfig.getUNIFOUT() ? ECL_UNIFIED_RESTART_FILE : ECL_RESTART_FILE,
                writeStepIdx,
                ioConfig.getFMTOUT() ),
        rst_file( open_rst( filename.get(),
                            first_restart,
                            ioConfig.getUNIFOUT(),
                            writeStepIdx ) )
    {}

    template< typename T >
    void add_kw( ERT::EclKW< T >&& kw ) {
        ecl_rst_file_add_kw( this->rst_file.get(), kw.get() );
    }

    void addRestartFileIwelData( std::vector<int>& data,
                                 size_t step,
                                 const Well& well,
                                 size_t offset ) const {

        const auto& completions = well.getCompletions( step );

        data[ offset + IWEL_HEADI_INDEX ] = well.getHeadI() + 1;
        data[ offset + IWEL_HEADJ_INDEX ] = well.getHeadJ() + 1;
        data[ offset + IWEL_CONNECTIONS_INDEX ] = completions.size();
        data[ offset + IWEL_GROUP_INDEX ] = 1;

        data[ offset + IWEL_TYPE_INDEX ] = to_ert_welltype( well, step );
        data[ offset + IWEL_STATUS_INDEX ] =
            well.getStatus( step ) == WellCommon::OPEN ? 1 : 0;
    }

    void addRestartFileIconData( std::vector< int >& data,
                                 const CompletionSet& completions,
                                 size_t wellICONOffset ) const {

        for( size_t i = 0; i < completions.size(); ++i ) {
            const auto& completion = completions.get( i );
            size_t offset = wellICONOffset + i * Restart::NICONZ;
            data[ offset + ICON_IC_INDEX ] = 1;

            data[ offset + ICON_I_INDEX ] = completion.getI() + 1;
            data[ offset + ICON_J_INDEX ] = completion.getJ() + 1;
            data[ offset + ICON_K_INDEX ] = completion.getK() + 1;

            const auto open = WellCompletion::StateEnum::OPEN;
            data[ offset + ICON_STATUS_INDEX ] = completion.getState() == open
                                              ? 1
                                              : 0;

            data[ offset + ICON_DIRECTION_INDEX ] = completion.getDirection();
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
    restart_file rst_file;
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

    template<typename T>
    void addFromCells(const data::Solution& solution) {
        for (const auto& elm: solution) {
            if (elm.second.target == data::TargetType::RESTART_SOLUTION)
                this->add( ERT::EclKW<T>(elm.first, elm.second.data ));
        }
    }

    ecl_rst_file_type* ertHandle() { return this->restart.ertHandle(); }

    ~Solution() { ecl_rst_file_end_solution( this->restart.ertHandle() ); }

private:
    Restart& restart;
};

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
             bool format );

        void writeTimeStep( std::vector< const Well* >,
                            const EclipseGrid& grid,
                            int report_step,
                            time_t current_time,
                            double days,
                            ert_ecl_unit_enum,
                            const data::Solution& cells);
    private:
        ERT::FortIO fortio;
};


RFT::RFT( const char* output_dir,
          const char* basename,
          bool format ) :
    fortio(FileName( output_dir, basename, ECL_RFT_FILE, format ).get(),std::ios_base::out)
{}

inline ert_ecl_unit_enum to_ert_unit( UnitSystem::UnitType t ) {
    using ut = UnitSystem::UnitType;
    switch ( t ) {
        case ut::UNIT_TYPE_METRIC: return ERT_ECL_METRIC_UNITS;
        case ut::UNIT_TYPE_FIELD:  return ERT_ECL_FIELD_UNITS;
        case ut::UNIT_TYPE_LAB:    return ERT_ECL_LAB_UNITS;
    }

    throw std::invalid_argument("unhandled enum value");
}

void RFT::writeTimeStep( std::vector< const Well* > wells,
                         const EclipseGrid& grid,
                         int report_step,
                         time_t current_time,
                         double days,
                         ert_ecl_unit_enum unitsystem,
                         const data::Solution& cells) {

    using rft = ERT::ert_unique_ptr< ecl_rft_node_type, ecl_rft_node_free >;
    const std::vector<double>& pressure = cells.data("PRESSURE");
    const std::vector<double>& swat = cells.data("SWAT");
    const std::vector<double>& sgas = cells.has("SGAS") ? cells.data("SGAS") : std::vector<double>( pressure.size() , 0 );

    for( const auto& well : wells ) {
        if( !( well->getRFTActive( report_step )
            || well->getPLTActive( report_step ) ) )
            continue;

        auto* rft_node = ecl_rft_node_alloc_new( well->name().c_str(), "RFT",
                current_time, days );

        for( const auto& completion : well->getCompletions( report_step ) ) {
            const size_t i = size_t( completion.getI() );
            const size_t j = size_t( completion.getJ() );
            const size_t k = size_t( completion.getK() );

            if( !grid.cellActive( i, j, k ) ) continue;

            const auto index = grid.activeIndex( i, j, k );
            const double depth = grid.getCellDepth( i, j, k );
            const double press = pressure[ index ];
            const double satwat = swat[ index ];
            const double satgas = sgas[ index ];

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
        Impl( const EclipseState& es, EclipseGrid grid );
        void writeINITFile( const data::Solution& simProps, const NNC& nnc) const;
        void writeEGRIDFile( const NNC& nnc ) const;

        const EclipseState& es;
        EclipseGrid grid;
        out::RegionCache regionCache;
        std::string outputDir;
        std::string baseName;
        out::Summary summary;
        RFT rft;
        time_t sim_start_time;
        std::array< int, 3 > cartesianSize;
        bool output_enabled;
        int ert_phase_mask;
        bool first_restart = true;
};

EclipseWriter::Impl::Impl( const EclipseState& eclipseState,
                           EclipseGrid grid_)
    : es( eclipseState )
    , grid( std::move( grid_ ) )
    , regionCache( es , grid )
    , outputDir( eclipseState.getIOConfig().getOutputDir() )
    , baseName( uppercase( eclipseState.getIOConfig().getBaseName() ) )
    , summary( eclipseState, eclipseState.getSummaryConfig() )
    , rft( outputDir.c_str(), baseName.c_str(), es.getIOConfig().getFMTOUT() )
    , sim_start_time( es.getSchedule().posixStartTime() )
    , output_enabled( eclipseState.getIOConfig().getOutputEnabled() )
    , ert_phase_mask( ertPhaseMask( eclipseState.getTableManager() ) )
{}



void EclipseWriter::Impl::writeINITFile( const data::Solution& simProps, const NNC& nnc) const {
    const auto& units = this->es.getUnits();
    const IOConfig& ioConfig = this->es.cfg().io();

    FileName  initFile( this->outputDir,
                        this->baseName,
                        ECL_INIT_FILE,
                        ioConfig.getFMTOUT() );

    ERT::FortIO fortio( initFile.get() ,
                        std::ios_base::out,
                        ioConfig.getFMTOUT(),
                        ECL_ENDIAN_FLIP );


    // Write INIT header. Observe that the PORV vector is treated
    // specially; that is because for this particulat vector we write
    // a total of nx*ny*nz values, where the PORV vector has been
    // explicitly set to zero for inactive cells. The convention is
    // that the active/inactive cell mapping can be inferred by
    // reading the PORV vector.
    {

        const auto& opm_data = this->es.get3DProperties().getDoubleGridProperty("PORV").getData();
        auto ecl_data = opm_data;

        for (size_t global_index = 0; global_index < opm_data.size(); global_index++)
            if (!this->grid.cellActive( global_index ))
                ecl_data[global_index] = 0;


        ecl_init_file_fwrite_header( fortio.get(),
                                     this->grid.c_ptr(),
                                     NULL,
                                     this->ert_phase_mask,
                                     this->sim_start_time);

        convertFromSiTo( ecl_data,
                         units,
                         UnitSystem::measure::volume);

        writeKeyword( fortio, "PORV" , ecl_data );
    }

    // Writing quantities which are calculated by the grid to the INIT file.
    ecl_grid_fwrite_depth( this->grid.c_ptr() , fortio.get() , to_ert_unit( units.getType( )) );
    ecl_grid_fwrite_dims( this->grid.c_ptr() , fortio.get() , to_ert_unit( units.getType( )) );

    // Write properties from the input deck.
    {
        const auto& properties = this->es.get3DProperties();
        using double_kw = std::pair<std::string, UnitSystem::measure>;
        std::vector<double_kw> doubleKeywords = {{"PORO"  , UnitSystem::measure::identity },
                                                 {"PERMX" , UnitSystem::measure::permeability },
                                                 {"PERMY" , UnitSystem::measure::permeability },
                                                 {"PERMZ" , UnitSystem::measure::permeability }};

        for (const auto& kw_pair : doubleKeywords) {
            const auto& opm_property = properties.getDoubleGridProperty(kw_pair.first);
            auto ecl_data = opm_property.compressedCopy( this->grid );

            convertFromSiTo( ecl_data,
                             units,
                             kw_pair.second );

            writeKeyword( fortio, kw_pair.first, ecl_data );
        }
    }


    // Write properties which have been initialized by the simulator.
    {
        for (const auto& prop : simProps) {
            auto ecl_data = this->grid.compressedVector( prop.second.data );
            writeKeyword( fortio, prop.first, ecl_data );
        }
    }


    // Write all integer field properties from the input deck.
    {
        const auto& properties = this->es.get3DProperties().getIntProperties();

        // It seems that the INIT file should always contain these
        // keywords, we therefor call getKeyword() here to invoke the
        // autocreation property, and ensure that the keywords exist
        // in the properties container.
        properties.getKeyword("PVTNUM");
        properties.getKeyword("SATNUM");
        properties.getKeyword("EQLNUM");
        properties.getKeyword("FIPNUM");

        for (const auto& property : properties) {
            auto ecl_data = property.compressedCopy( this->grid );
            writeKeyword( fortio , property.getKeywordName() , ecl_data );
        }
    }


    // Write NNC transmissibilities
    {
        std::vector<double> tran;
        for( const NNCdata& nd : nnc.nncdata() )
            tran.push_back( nd.trans );

        convertFromSiTo( tran, units, UnitSystem::measure::transmissibility );
        writeKeyword( fortio, "TRANNNC" , tran );
    }
}


void EclipseWriter::Impl::writeEGRIDFile( const NNC& nnc ) const {
    const auto& ioConfig = this->es.getIOConfig();

    FileName  egridFile( this->outputDir,
                         this->baseName,
                         ECL_EGRID_FILE,
                         ioConfig.getFMTOUT() );

    {
        int idx = 0;
        auto* ecl_grid = const_cast< ecl_grid_type* >( this->grid.c_ptr() );
        for (const NNCdata& n : nnc.nncdata())
            ecl_grid_add_self_nnc( ecl_grid, n.cell1, n.cell2, idx++);

        ecl_grid_fwrite_EGRID2(ecl_grid, egridFile.get(), to_ert_unit( this->es.getDeckUnitSystem().getType()));
    }
}


void EclipseWriter::writeInitAndEgrid(data::Solution simProps, const NNC& nnc) {
    if( !this->impl->output_enabled )
        return;

    {
        const auto& es = this->impl->es;
        const IOConfig& ioConfig = es.cfg().io();

        simProps.convertFromSI( es.getUnits() );
        if( ioConfig.getWriteINITFile() )
            this->impl->writeINITFile( simProps , nnc );

        if( ioConfig.getWriteEGRIDFile( ) )
            this->impl->writeEGRIDFile( nnc );
    }
}

std::vector< double > serialize_XWEL( const data::Wells& wells,
                                      int report_step,
                                      const std::vector< const Well* > sched_wells,
                                      const TableManager& tm,
                                      const EclipseGrid& grid ) {

    using rt = data::Rates::opt;

    std::vector< rt > phases;
    if( tm.hasPhase( Phase::PhaseEnum::WATER ) ) phases.push_back( rt::wat );
    if( tm.hasPhase( Phase::PhaseEnum::OIL ) )   phases.push_back( rt::oil );
    if( tm.hasPhase( Phase::PhaseEnum::GAS ) )   phases.push_back( rt::gas );

    std::vector< double > xwel;
    for( const auto* sched_well : sched_wells ) {

        if( wells.count( sched_well->name() ) == 0 ) {
            const auto elems = (sched_well->getCompletions( report_step ).size()
                               * (phases.size() + data::Completion::restart_size))
                + 2 /* bhp, temperature */
                + phases.size();

            // write zeros if no well data is provided
            xwel.insert( xwel.end(), elems, 0.0 );
            continue;
        }

        const auto& well = wells.at( sched_well->name() );

        xwel.push_back( well.bhp );
        xwel.push_back( well.temperature );
        for( auto phase : phases )
            xwel.push_back( well.rates.get( phase ) );

        for( const auto& sc : sched_well->getCompletions( report_step ) ) {
            const auto i = sc.getI(), j = sc.getJ(), k = sc.getK();

            const auto rs_size = phases.size() + data::Completion::restart_size;
            if( !grid.cellActive( i, j, k ) || sc.getState() == WellCompletion::SHUT ) {
                xwel.insert( xwel.end(), rs_size, 0.0 );
                continue;
            }

            const auto active_index = grid.activeIndex( i, j, k );
            const auto at_index = [=]( const data::Completion& c ) {
                return c.index == active_index;
            };
            const auto& completion = std::find_if( well.completions.begin(),
                                                   well.completions.end(),
                                                   at_index );

            if( completion == well.completions.end() ) {
                xwel.insert( xwel.end(), rs_size, 0.0 );
                continue;
            }

            xwel.push_back( completion->pressure );
            xwel.push_back( completion->reservoir_rate );
            for( auto phase : phases )
                xwel.push_back( completion->rates.get( phase ) );
        }
    }

    return xwel;
};

std::vector< int > serialize_IWEL( const data::Wells& wells,
                                   const std::vector< const Well* > sched_wells ) {

    const auto getctrl = [&]( const Well* w ) {
        const auto itr = wells.find( w->name() );
        return itr == wells.end() ? 0 : itr->second.control;
    };

    std::vector< int > iwel( sched_wells.size(), 0.0 );
    std::transform( sched_wells.begin(), sched_wells.end(), iwel.begin(), getctrl );

    return iwel;
}


// implementation of the writeTimeStep method
void EclipseWriter::writeTimeStep(int report_step,
                                  bool  isSubstep,
                                  double secs_elapsed,
                                  data::Solution cells,
                                  data::Wells wells,
                                  bool  write_float)
{

    if( !this->impl->output_enabled )
        return;


    time_t current_posix_time = this->impl->sim_start_time + secs_elapsed;
    const auto& es = this->impl->es;
    const auto& grid = this->impl->grid;
    const auto& units = es.getUnits();

    const auto& restart = es.cfg().restart();


    const auto days = units.from_si( UnitSystem::measure::time, secs_elapsed );
    const auto& schedule = es.getSchedule();

    /*
       This routine can optionally write RFT and/or restart file; to
       be certain that the data correctly converted to output units
       the conversion is done once here - and not closer to the actual
       use-site.
    */
    cells.convertFromSI( units );

    // Write restart file
    if(!isSubstep && restart.getWriteRestartFile(report_step))
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
                               es.cfg().io(),
                               this->impl->first_restart );

        this->impl->first_restart = false;

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
            rsthead_data.nactive    = grid.getNumActive(),
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

        const auto& tm = es.getTableManager();
        const auto& sched_wells = schedule.getWells( report_step );
        const auto xwel = serialize_XWEL( wells, report_step, sched_wells, tm, grid );
        const auto iwel = serialize_IWEL( wells, sched_wells );

        restartHandle.add_kw( ERT::EclKW< int >(IWEL_KW, iwell_data) );
        restartHandle.add_kw( ERT::EclKW< const char* >(ZWEL_KW, zwell_data ) );
        restartHandle.add_kw( ERT::EclKW< double >( OPM_XWEL, xwel ) );
        restartHandle.add_kw( ERT::EclKW< int >( OPM_IWEL, iwel ) );
        restartHandle.add_kw( ERT::EclKW< int >( ICON_KW, icon_data ) );


        /*
          The code in the block below is in a state of flux, and not
          very well tested. In particular there have been issues with
          the mapping between active an inactive cells in the past -
          there might be more those.

          Currently the code hard-codes the following assumptions:

            1. All the cells[ ] vectors have size equal to the number
               of active cells, and they are already in eclipse
               ordering.

            2. No unit transformatio applied to the saturation and
               RS/RV vectors.

        */
        {
            Solution sol(restartHandle);  //Type solution: confusing

            if (write_float)
                sol.addFromCells<float>( cells );
            else
                sol.addFromCells<double>( cells );
            // MIssing a ENDSOL output.
            {
                for (const auto& prop : cells) {
                    if (prop.second.target == data::TargetType::RESTART_AUXILLARY) {
                        auto ecl_data = grid.compressedVector( prop.second.data );

                        if (write_float)
                            sol.add( ERT::EclKW<float>(prop.first, ecl_data));
                        else
                            sol.add( ERT::EclKW<double>(prop.first, ecl_data));
                    }
                }
            }
        }
    }

    const auto unit_type = es.getDeckUnitSystem().getType();
    this->impl->rft.writeTimeStep( schedule.getWells( report_step ),
                                   grid,
                                   report_step,
                                   current_posix_time,
                                   days,
                                   to_ert_unit( unit_type ),
                                   cells );

    if( isSubstep ) return;

    this->impl->summary.add_timestep( report_step,
                                      secs_elapsed,
                                      this->impl->grid,
                                      this->impl->es,
                                      this->impl->regionCache,
                                      wells ,
                                      cells );
    this->impl->summary.write();
 }



EclipseWriter::EclipseWriter( const EclipseState& es, EclipseGrid grid)
    : impl( new Impl( es, std::move( grid ) ) )
{
    if( !this->impl->output_enabled )
        return;
    {
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
}

EclipseWriter::~EclipseWriter() {}

} // namespace Opm
