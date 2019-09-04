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
#include "config.h"

#include <opm/output/eclipse/EclipseIO.hpp>

#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Units/Dimension.hpp>
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/EclipseState/Eclipse3DProperties.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Grid/GridProperty.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well/WellConnections.hpp>
#include <opm/parser/eclipse/Utility/Functional.hpp>

#include <opm/output/eclipse/RestartIO.hpp>
#include <opm/output/eclipse/Summary.hpp>
#include <opm/output/eclipse/WriteInit.hpp>

#include <opm/io/eclipse/OutputStream.hpp>

#include <cstdlib>
#include <memory>     // unique_ptr
#include <unordered_map>
#include <utility>    // move

#include <ert/ecl/EclKW.hpp>
#include <ert/ecl/EclFilename.hpp>

#include <ert/ecl/ecl_kw_magic.h>
#include <ert/ecl/ecl_kw.h>
#include <ert/ecl/ecl_init_file.h>
#include <ert/ecl/ecl_file.h>
#include <ert/ecl/ecl_grid.h>
#include <ert/ecl/ecl_rft_file.h>
#include <ert/ecl/ecl_rst_file.h>
#include <ert/ecl_well/well_const.h>
#include <ert/ecl/ecl_rsthead.h>
#include <ert/util/util.h>
#include <ert/ecl/fortio.h>

#define OPM_XWEL      "OPM_XWEL"
#define OPM_IWEL      "OPM_IWEL"

// namespace start here since we don't want the ERT headers in it
namespace Opm {
namespace {

class RFT {
    public:
    RFT( const std::string&  output_dir,
         const std::string&  basename,
         bool format );

        void writeTimeStep( const Schedule& schedule,
                            const EclipseGrid& grid,
                            std::size_t report_step,
                            time_t current_time,
                            double days,
                            const UnitSystem& units,
                            data::Wells wellData);
    private:
        std::string filename;
        bool fmt_file;
};


    RFT::RFT( const std::string& output_dir,
              const std::string& basename,
              bool format ) :
        filename( ERT::EclFilename( output_dir, basename, ECL_RFT_FILE, format ) ),
        fmt_file( format )
{}


void RFT::writeTimeStep( const Schedule& schedule,
                         const EclipseGrid& grid,
                         std::size_t report_step,
                         time_t current_time,
                         double days,
                         const UnitSystem& units,
                         data::Wells wellDatas) {
    using rft = ERT::ert_unique_ptr< ecl_rft_node_type, ecl_rft_node_free >;
    const auto& rft_config = schedule.rftConfig();
    auto well_names = schedule.wellNames(report_step);
    if (!rft_config.active(report_step))
        return;

    fortio_type * fortio;

    if (report_step > rft_config.firstRFTOutput())
        fortio = fortio_open_append( filename.c_str() , fmt_file , ECL_ENDIAN_FLIP );
    else
        fortio = fortio_open_writer( filename.c_str() , fmt_file , ECL_ENDIAN_FLIP );

    for ( const auto& well_name : well_names ) {

        if (!(rft_config.rft(well_name, report_step) || rft_config.plt(well_name, report_step)))
            continue;

        rft rft_node(ecl_rft_node_alloc_new( well_name.c_str(), "RFT", current_time, days ));
        const auto& wellData = wellDatas.at(well_name);

        if (wellData.connections.empty())
            continue;

        const auto& well = schedule.getWell2(well_name, report_step);
        for( const auto& connection : well.getConnections() ) {

            const size_t i = size_t( connection.getI() );
            const size_t j = size_t( connection.getJ() );
            const size_t k = size_t( connection.getK() );

            if( !grid.cellActive( i, j, k ) ) continue;

            const auto index = grid.getGlobalIndex( i, j, k );
            const double depth = grid.getCellDepth( i, j, k );

            const auto& connectionData = std::find_if( wellData.connections.begin(),
                                                   wellData.connections.end(),
                                                   [=]( const data::Connection& c ) {
                                                        return c.index == index;
                                                   } );


            const double press = units.from_si(UnitSystem::measure::pressure,connectionData->cell_pressure);
            const double satwat = units.from_si(UnitSystem::measure::identity, connectionData->cell_saturation_water);
            const double satgas = units.from_si(UnitSystem::measure::identity, connectionData->cell_saturation_gas);

            auto* cell = ecl_rft_cell_alloc_RFT(
                            i, j, k, depth, press, satwat, satgas );

            ecl_rft_node_append_cell( rft_node.get(), cell );
        }

        ecl_rft_node_fwrite( rft_node.get(), fortio, units.getEclType() );
    }

    fortio_fclose( fortio );
}

inline std::string uppercase( std::string x ) {
    std::transform( x.begin(), x.end(), x.begin(),
        []( char c ) { return std::toupper( c ); } );

    return x;
}

}

class EclipseIO::Impl {
    public:
    Impl( const EclipseState&, EclipseGrid, const Schedule&, const SummaryConfig& );
        void writeINITFile( const data::Solution& simProps, std::map<std::string, std::vector<int> > int_data, const NNC& nnc) const;
        void writeEGRIDFile( const NNC& nnc );

        const EclipseState& es;
        EclipseGrid grid;
        const Schedule& schedule;
        std::string outputDir;
        std::string baseName;
        out::Summary summary;
        RFT rft;
        bool output_enabled;
};

EclipseIO::Impl::Impl( const EclipseState& eclipseState,
                       EclipseGrid grid_,
                       const Schedule& schedule_,
                       const SummaryConfig& summary_config)
    : es( eclipseState )
    , grid( std::move( grid_ ) )
    , schedule( schedule_ )
    , outputDir( eclipseState.getIOConfig().getOutputDir() )
    , baseName( uppercase( eclipseState.getIOConfig().getBaseName() ) )
    , summary( eclipseState, summary_config, grid , schedule )
    , rft( outputDir.c_str(), baseName.c_str(), es.getIOConfig().getFMTOUT() )
    , output_enabled( eclipseState.getIOConfig().getOutputEnabled() )
{}


void EclipseIO::Impl::writeINITFile(const data::Solution&                   simProps,
                                    std::map<std::string, std::vector<int>> int_data,
                                    const NNC&                              nnc) const
{
    EclIO::OutputStream::Init initFile {
        EclIO::OutputStream::ResultSet { this->outputDir, this->baseName },
        EclIO::OutputStream::Formatted { this->es.cfg().io().getFMTOUT() }
    };

    InitIO::write(this->es, this->grid, this->schedule,
                  simProps, std::move(int_data), nnc, initFile);
}


void EclipseIO::Impl::writeEGRIDFile( const NNC& nnc ) {
    const auto& ioConfig = this->es.getIOConfig();

    std::string  egridFile( ERT::EclFilename( this->outputDir,
                                              this->baseName,
                                              ECL_EGRID_FILE,
                                              ioConfig.getFMTOUT() ));

    this->grid.addNNC( nnc );
    this->grid.save( egridFile, this->es.getDeckUnitSystem().getType());
}

/*
int_data: Writes key(string) and integers vector to INIT file as eclipse keywords
- Key: Max 8 chars.
- Wrong input: invalid_argument exception.
*/
void EclipseIO::writeInitial( data::Solution simProps, std::map<std::string, std::vector<int> > int_data, const NNC& nnc) {
    if( !this->impl->output_enabled )
        return;

    {
        const auto& es = this->impl->es;
        const IOConfig& ioConfig = es.cfg().io();

        simProps.convertFromSI( es.getUnits() );
        if( ioConfig.getWriteINITFile() )
            this->impl->writeINITFile( simProps , std::move(int_data), nnc );

        if( ioConfig.getWriteEGRIDFile( ) )
            this->impl->writeEGRIDFile( nnc );
    }

}

// implementation of the writeTimeStep method
void EclipseIO::writeTimeStep(const SummaryState& st,
                              int report_step,
                              bool  isSubstep,
                              double secs_elapsed,
                              RestartValue value,
                              const bool write_double)
 {

    if( !this->impl->output_enabled )
        return;


    const auto& es = this->impl->es;
    const auto& grid = this->impl->grid;
    const auto& schedule = this->impl->schedule;
    const auto& units = es.getUnits();
    const auto& ioConfig = es.getIOConfig();
    const auto& restart = es.cfg().restart();



    /*
      Summary data is written unconditionally for every timestep except for the
      very intial report_step==0 call, which is only garbage.
    */
    if (report_step > 0) {
        this->impl->summary.add_timestep( st,
                                          report_step);
        this->impl->summary.write();
    }

    /*
      Current implementation will not write restart files for substep,
      but there is an unsupported option to the RPTSCHED keyword which
      will request restart output from every timestep.
    */
    if(!isSubstep && restart.getWriteRestartFile(report_step))
    {
        EclIO::OutputStream::Restart rstFile {
            EclIO::OutputStream::ResultSet { this->impl->outputDir,
                                             this->impl->baseName },
            report_step,
            EclIO::OutputStream::Formatted { ioConfig.getFMTOUT() },
            EclIO::OutputStream::Unified   { ioConfig.getUNIFOUT() }
        };

        RestartIO::save(rstFile, report_step, secs_elapsed, value, es, grid, schedule,
                        st, write_double);
    }


    /*
      RFT files are not written for substep.
    */
    if( isSubstep )
        return;

    this->impl->rft.writeTimeStep( schedule,
                                   grid,
                                   report_step,
                                   secs_elapsed + this->impl->schedule.posixStartTime(),
                                   units.from_si( UnitSystem::measure::time, secs_elapsed ),
                                   units,
                                   value.wells );

 }


RestartValue EclipseIO::loadRestart(SummaryState& summary_state, const std::vector<RestartKey>& solution_keys, const std::vector<RestartKey>& extra_keys) const {
    const auto& es                       = this->impl->es;
    const auto& grid                     = this->impl->grid;
    const auto& schedule                 = this->impl->schedule;
    const InitConfig& initConfig         = es.getInitConfig();
    const auto& ioConfig                 = es.getIOConfig();
    const int report_step                = initConfig.getRestartStep();
    const std::string filename           = ioConfig.getRestartFileName( initConfig.getRestartRootName(),
                                                                        report_step,
                                                                        false );

    return RestartIO::load(filename, report_step, summary_state, solution_keys,
                           es, grid, schedule, extra_keys);
}

EclipseIO::EclipseIO( const EclipseState& es,
                      EclipseGrid grid,
                      const Schedule& schedule,
                      const SummaryConfig& summary_config)
    : impl( new Impl( es, std::move( grid ), schedule , summary_config) )
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

const out::Summary& EclipseIO::summary() {
    return this->impl->summary;
}


EclipseIO::~EclipseIO() {}

} // namespace Opm
