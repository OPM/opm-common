/*
  Copyright 2015 Statoil ASA.

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

#include <stdio.h>
#include <iostream>
#include <iterator>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>

#include <opm/parser/eclipse/Deck/DeckItem.hpp>
#include <opm/parser/eclipse/Deck/DeckKeyword.hpp>
#include <opm/parser/eclipse/Deck/DeckRecord.hpp>
#include <opm/parser/eclipse/Deck/Section.hpp>
#include <opm/parser/eclipse/EclipseState/IOConfig/IOConfig.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/DynamicState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>

#include <ert/ecl/EclFilename.hpp>


namespace Opm {

    namespace {
        const char* default_dir = ".";

        inline std::string basename( const std::string& path ) {
            return boost::filesystem::path( path ).stem().string();
        }

        inline std::string outputdir( const std::string& path ) {
            auto dir = boost::filesystem::path( path ).parent_path().string();

            if( dir.empty() ) return default_dir;

            return dir;
        }
    }


    IOConfig::IOConfig( const Deck& deck ) :
        IOConfig( GRIDSection( deck ),
                  RUNSPECSection( deck ),
                  deck.hasKeyword("NOSIM"),
                  deck.getDataFile() )
    {}

    IOConfig::IOConfig( const std::string& input_path ) :
        m_deck_filename( input_path ),
        m_output_dir( outputdir( input_path ) ),
        m_base_name( basename( input_path ) )
    {}

    static inline bool write_egrid_file( const GRIDSection& grid ) {
        if( grid.hasKeyword( "NOGGF" ) ) return false;
        if( !grid.hasKeyword( "GRIDFILE" ) ) return true;

        const auto& keyword = grid.getKeyword( "GRIDFILE" );
        const auto& rec = keyword.getRecord( 0 );

        {
            const auto& grid_item = rec.getItem( 0 );
            if (grid_item.get<int>(0) != 0) {
                std::cerr << "IOConfig: Reading GRIDFILE keyword from GRID section: "
                          << "Output of GRID file is not supported. "
                          << "Supported format: EGRID"
                          << std::endl;

                // It was asked for GRID file - that output is not
                // supported, but we will output EGRID file;
                // irrespective of whether that was actually
                // requested.
                return true;
            }
        }

        {
            const auto& egrid_item = rec.getItem( 1 );
            return (egrid_item.get<int>(0) == 1);
        }
    }

    IOConfig::IOConfig( const GRIDSection& grid,
                        const RUNSPECSection& runspec,
                        bool nosim,
                        const std::string& input_path ) :
        m_write_INIT_file( grid.hasKeyword( "INIT" ) ),
        m_write_EGRID_file( write_egrid_file( grid ) ),
        m_UNIFIN( runspec.hasKeyword( "UNIFIN" ) ),
        m_UNIFOUT( runspec.hasKeyword( "UNIFOUT" ) ),
        m_FMTIN( runspec.hasKeyword( "FMTIN" ) ),
        m_FMTOUT( runspec.hasKeyword( "FMTOUT" ) ),
        m_deck_filename( input_path ),
        m_output_dir( outputdir( input_path ) ),
        m_base_name( basename( input_path ) ),
        m_nosim( nosim  )
    {}


    bool IOConfig::getWriteEGRIDFile() const {
        return m_write_EGRID_file;
    }

    bool IOConfig::getWriteINITFile() const {
        return m_write_INIT_file;
    }




    void IOConfig::overrideNOSIM(bool nosim) {
        m_nosim = nosim;
    }


    bool IOConfig::getUNIFIN() const {
        return m_UNIFIN;
    }

    bool IOConfig::getUNIFOUT() const {
        return m_UNIFOUT;
    }

    bool IOConfig::getFMTIN() const {
        return m_FMTIN;
    }

    bool IOConfig::getFMTOUT() const {
        return m_FMTOUT;
    }



    std::string IOConfig::getRestartFileName(const std::string& restart_base, int report_step, bool output) const {
        bool unified  = output ? getUNIFOUT() : getUNIFIN();
        bool fmt_file = output ? getFMTOUT()  : getFMTIN();
        ecl_file_enum file_type = (unified) ? ECL_UNIFIED_RESTART_FILE : ECL_RESTART_FILE;

        return ERT::EclFilename( restart_base , file_type , report_step , fmt_file );
    }


    bool IOConfig::getOutputEnabled() const {
        return m_output_enabled;
    }

    void IOConfig::setOutputEnabled(bool enabled){
        m_output_enabled = enabled;
    }

    std::string IOConfig::getOutputDir() const {
        return m_output_dir;
    }

    void IOConfig::setOutputDir(const std::string& outputDir) {
        m_output_dir = outputDir;
    }

    const std::string& IOConfig::getBaseName() const {
        return m_base_name;
    }

    void IOConfig::setBaseName(std::string baseName) {
        m_base_name = baseName;
    }

    std::string IOConfig::fullBasePath( ) const {
        namespace fs = boost::filesystem;

        fs::path dir( m_output_dir );
        fs::path base( m_base_name );
        fs::path full_path = dir.make_preferred() / base.make_preferred();

        return full_path.string();
    }


    bool IOConfig::initOnly( ) const {
        return m_nosim;
    }


    /*****************************************************************/
    /* Here at the bottom are some forwarding proxy methods which just
       forward to the appropriate RestartConfig method. They are
       retained here as a temporary convenience method to prevent
       downstream breakage.

       Currently the EclipseState object can return a mutable IOConfig
       object, which application code can alter to override settings
       from the deck - this is quite ugly. When the API is reworked to
       remove the ability modify IOConfig objects we should also
       remove these forwarding methods.
    */

} //namespace Opm
