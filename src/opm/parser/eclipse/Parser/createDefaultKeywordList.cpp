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

#if !defined(WIN32)
    #define _POSIX_C_SOURCE 200112L
    #include <stdlib.h>
#endif

#include <iostream>
#include <locale>

#include <boost/filesystem.hpp>

#include <opm/parser/eclipse/Generator/KeywordGenerator.hpp>
#include <opm/parser/eclipse/Generator/KeywordLoader.hpp>


int main(int argc , char ** argv) {
    if (argc != 5 && argc != 3) {
        std::cerr << "Error calling keyword generator: Expected arguments: <config_root> <source_file_name> <header_build_path> <header_file_name>" << std::endl;
        std::cerr << "Error calling keyword generator: Expected arguments: <config_root> <test_file_name>" << std::endl;
        return 1;
    }

    try {
        /* sometimes the local env's locales are broken on POSIX. Boost <=
         * 1.56 uses the std::locale("") constructor which respects user
         * preferred locales, and might crash. If this is the case, and
         * we're on a non-windows system (assuming POSIX), in case of an
         * exception, set the environment to "C" and keep going.
         *
         * Can be removed once boost < 1.57 is no longer supported
         */
        std::locale( "" );
    } catch( const std::runtime_error& ) {
        #if !defined(WIN32)
        setenv( "LC_ALL", "C", 1 );
        #endif
        auto loc = boost::filesystem::path::imbue( std::locale::classic() );
        boost::filesystem::path::imbue( loc );
        std::cout << "User preferred locale setting is invalid "
            << "which breaks Boost <= 1.56 "
            << "- forcing to 'C' as workaround for Boost <= 1.56. "
            << "This workaround only applies to compile opm-parser, "
            << "but your locale settings seem BROKEN, "
            << "and opm-parser is likely NOT GOING TO WORK. "
            << "If you're on linux you can try setting the LANG "
            << "or LC_ALL environment variables to C or POSIX."
            << std::endl;
    }

    const char * config_root = argv[1];
    Opm::KeywordGenerator generator( true );
    Opm::KeywordLoader loader( false );
    loader.loadMultipleKeywordDirectories( config_root );

    if( argc == 3 ) {
        const char * test_file_name = argv[2];
        generator.updateTest(loader , test_file_name );
    } else {
        const char * source_file_name = argv[2];
        const char * header_file_base_path = argv[3];
        const char * header_file_name = argv[4];

        generator.updateSource(loader , source_file_name );
        generator.updateHeader(loader, header_file_base_path, header_file_name );
    }
}
