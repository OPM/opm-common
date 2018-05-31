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
#include <opm/parser/eclipse/EclipseState/Schedule/WellConnections.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/ScheduleEnums.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>
#include <opm/parser/eclipse/Utility/Functional.hpp>

#include <opm/output/eclipse/RestartIO.hpp>
#include <opm/output/eclipse/Summary.hpp>
#include <opm/output/eclipse/SummaryState.hpp>
#include <opm/output/eclipse/Tables.hpp>

#include <cstdlib>
#include <memory>     // unique_ptr
#include <unordered_map>
#include <utility>    // move

#include <ert/ecl/EclKW.hpp>
#include <ert/ecl/EclFilename.hpp>

#include <ert/ecl/ecl_kw_magic.h>
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





class RFT {
    public:
    RFT( const std::string&  output_dir,
         const std::string&  basename,
         bool format );

        void writeTimeStep( std::vector< const Well* >,
                            const EclipseGrid& grid,
                            int report_step,
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


void RFT::writeTimeStep( std::vector< const Well* > wells,
                         const EclipseGrid& grid,
                         int report_step,
                         time_t current_time,
                         double days,
                         const UnitSystem& units,
                         data::Wells wellDatas) {
    using rft = ERT::ert_unique_ptr< ecl_rft_node_type, ecl_rft_node_free >;

    fortio_type * fortio;
    int first_report_step = report_step;

    for (const auto* well : wells)
        first_report_step = std::min( first_report_step, well->firstRFTOutput());

    if (report_step > first_report_step)
        fortio = fortio_open_append( filename.c_str() , fmt_file , ECL_ENDIAN_FLIP );
    else
        fortio = fortio_open_writer( filename.c_str() , fmt_file , ECL_ENDIAN_FLIP );

    for ( const auto& well : wells ) {
        if( !( well->getRFTActive( report_step )
            || well->getPLTActive( report_step ) ) )
            continue;

        auto* rft_node = ecl_rft_node_alloc_new( well->name().c_str(), "RFT",
                current_time, days );

        const auto& wellData = wellDatas.at(well->name());

        if (wellData.connections.empty())
            continue;

        for( const auto& connection : well->getConnections( report_step ) ) {

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

            ecl_rft_node_append_cell( rft_node, cell );
        }

        rft ecl_node( rft_node );
        ecl_rft_node_fwrite( ecl_node.get(), fortio, units.getEclType() );
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
        void writeEGRIDFile( const NNC& nnc ) const;

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


void EclipseIO::Impl::writeINITFile( const data::Solution& simProps, std::map<std::string, std::vector<int> > int_data, const NNC& nnc) const {
    const auto& units = this->es.getUnits();
    const IOConfig& ioConfig = this->es.cfg().io();

    std::string  initFile( ERT::EclFilename( this->outputDir,
                                             this->baseName,
                                             ECL_INIT_FILE,
                                             ioConfig.getFMTOUT() ));

    ERT::FortIO fortio( initFile,
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
                                     units.getEclType(),
                                     this->es.runspec( ).eclPhaseMask( ),
                                     this->schedule.posixStartTime( ));

        units.from_si( UnitSystem::measure::volume, ecl_data );
        writeKeyword( fortio, "PORV" , ecl_data );
    }

    // Writing quantities which are calculated by the grid to the INIT file.
    ecl_grid_fwrite_depth( this->grid.c_ptr() , fortio.get() , units.getEclType( ) );
    ecl_grid_fwrite_dims( this->grid.c_ptr() , fortio.get() , units.getEclType( ) );

    // Write properties from the input deck.
    {
        const auto& properties = this->es.get3DProperties().getDoubleProperties();
        using double_kw = std::pair<std::string, UnitSystem::measure>;
        /*
          This is a rather arbitrary hardcoded list of 3D keywords
          which are written to the INIT file, if they are in the
          current EclipseState.
        */
        std::vector<double_kw> doubleKeywords = {{"PORO"  , UnitSystem::measure::identity },
                                                 {"PERMX" , UnitSystem::measure::permeability },
                                                 {"PERMY" , UnitSystem::measure::permeability },
                                                 {"PERMZ" , UnitSystem::measure::permeability },
                                                 {"NTG"   , UnitSystem::measure::identity }};

        // The INIT file should always contain the NTG property, we
        // therefor invoke the auto create functionality to ensure
        // that "NTG" is included in the properties container.
        properties.assertKeyword("NTG");

        for (const auto& kw_pair : doubleKeywords) {
            if (properties.hasKeyword( kw_pair.first)) {
                const auto& opm_property = properties.getKeyword(kw_pair.first);
                auto ecl_data = opm_property.compressedCopy( this->grid );

                units.from_si( kw_pair.second, ecl_data );
                writeKeyword( fortio, kw_pair.first, ecl_data );
            }
        }
    }


    // Write properties which have been initialized by the simulator.
    {
        for (const auto& prop : simProps) {
            auto ecl_data = this->grid.compressedVector( prop.second.data );
            writeKeyword( fortio, prop.first, ecl_data );
        }
    }

    // Write tables
    {
        Tables tables( this->es.getUnits() );
        tables.addPVTO( this->es.getTableManager().getPvtoTables() );
        tables.addPVTG( this->es.getTableManager().getPvtgTables() );
        tables.addPVTW( this->es.getTableManager().getPvtwTable() );
        tables.addDensity( this->es.getTableManager().getDensityTable( ) );
        tables.addSatFunc(this->es);
        fwrite(tables, fortio);
    }

    // Write all integer field properties from the input deck.
    {
        const auto& properties = this->es.get3DProperties().getIntProperties();

        // It seems that the INIT file should always contain these
        // keywords, we therefor call getKeyword() here to invoke the
        // autocreation property, and ensure that the keywords exist
        // in the properties container.
        properties.assertKeyword("PVTNUM");
        properties.assertKeyword("SATNUM");
        properties.assertKeyword("EQLNUM");
        properties.assertKeyword("FIPNUM");

        for (const auto& property : properties) {
            auto ecl_data = property.compressedCopy( this->grid );
            writeKeyword( fortio , property.getKeywordName() , ecl_data );
        }
    }


    //Write Integer Vector Map
    {
        for( const auto& pair : int_data)  {
            const std::string& key = pair.first;
            const std::vector<int>& int_vector = pair.second;
            if (key.size() > ECL_STRING8_LENGTH)
              throw std::invalid_argument("Keyword is too long.");

            writeKeyword( fortio , key , int_vector );
        }
    }


    // Write NNC transmissibilities
    {
        std::vector<double> tran;
        for( const NNCdata& nd : nnc.nncdata() )
            tran.push_back( nd.trans );

        units.from_si( UnitSystem::measure::transmissibility , tran );
        writeKeyword( fortio, "TRANNNC" , tran );
    }
}


void EclipseIO::Impl::writeEGRIDFile( const NNC& nnc ) const {
    const auto& ioConfig = this->es.getIOConfig();

    std::string  egridFile( ERT::EclFilename( this->outputDir,
                                              this->baseName,
                                              ECL_EGRID_FILE,
                                              ioConfig.getFMTOUT() ));

    {
        int idx = 0;
        auto* ecl_grid = const_cast< ecl_grid_type* >( this->grid.c_ptr() );
        for (const NNCdata& n : nnc.nncdata())
            ecl_grid_add_self_nnc( ecl_grid, n.cell1, n.cell2, idx++);

        ecl_grid_fwrite_EGRID2(ecl_grid, egridFile.c_str(), this->es.getDeckUnitSystem().getEclType() );
    }
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
            this->impl->writeINITFile( simProps , int_data, nnc );

        if( ioConfig.getWriteEGRIDFile( ) )
            this->impl->writeEGRIDFile( nnc );
    }

}

// implementation of the writeTimeStep method
void EclipseIO::writeTimeStep(int report_step,
                              bool  isSubstep,
                              double secs_elapsed,
                              RestartValue value,
                              const std::map<std::string, double>& single_summary_values,
                              const std::map<std::string, std::vector<double> >& region_summary_values,
                              const std::map<std::pair<std::string, int>, double>& block_summary_values,
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
        this->impl->summary.add_timestep( report_step,
                                          secs_elapsed,
                                          es,
                                          schedule,
                                          value.wells ,
                                          single_summary_values ,
                                          region_summary_values,
                                          block_summary_values);
        this->impl->summary.write();
    }

    /*
      Current implementation will not write restart files for substep,
      but there is an unsupported option to the RPTSCHED keyword which
      will request restart output from every timestep.
    */
    if(!isSubstep && restart.getWriteRestartFile(report_step))
    {
        std::string filename = ERT::EclFilename( this->impl->outputDir,
                                                 this->impl->baseName,
                                                 ioConfig.getUNIFOUT() ? ECL_UNIFIED_RESTART_FILE : ECL_RESTART_FILE,
                                                 report_step,
                                                 ioConfig.getFMTOUT() );

        RestartIO::save(filename, report_step, secs_elapsed, value, es, grid, schedule,
                        this->impl->summary.get_restart_vectors(), write_double);
    }


    /*
      RFT files are not written for substep.
    */
    if( isSubstep )
        return;

    {
        std::vector<const Well*> sched_wells = this->impl->schedule.getWells( report_step );
        const auto rft_active = [report_step] (const Well* w) { return w->getRFTActive( report_step ) || w->getPLTActive( report_step ); };
        if (std::any_of(sched_wells.begin(), sched_wells.end(), rft_active)) {
            this->impl->rft.writeTimeStep( sched_wells,
                                           grid,
                                           report_step,
                                           secs_elapsed + this->impl->schedule.posixStartTime(),
                                           units.from_si( UnitSystem::measure::time, secs_elapsed ),
                                           units,
                                           value.wells );
        }
    }

 }



RestartValue EclipseIO::loadRestart(const std::vector<RestartKey>& solution_keys, const std::vector<RestartKey>& extra_keys) const {
    const auto& es                       = this->impl->es;
    const auto& grid                     = this->impl->grid;
    const auto& schedule                 = this->impl->schedule;
    const InitConfig& initConfig         = es.getInitConfig();
    const auto& ioConfig                 = es.getIOConfig();
    const int report_step                = initConfig.getRestartStep();
    const std::string filename           = ioConfig.getRestartFileName( initConfig.getRestartRootName(),
                                                                        report_step,
                                                                        false );

    return RestartIO::load( filename , report_step , solution_keys , es, grid , schedule, extra_keys);
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


EclipseIO::~EclipseIO() {}

} // namespace Opm



