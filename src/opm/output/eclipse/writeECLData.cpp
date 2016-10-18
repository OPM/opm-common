/*
  Copyright 2012 SINTEF ICT, Applied Mathematics.
  Copyright 2012 Statoil ASA.

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

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <opm/output/eclipse/writeECLData.hpp>
#include <opm/parser/eclipse/Units/Units.hpp>

#include <vector>

#ifdef HAVE_ERT // This one goes almost to the bottom of the file

#include <ert/ecl/ecl_rst_file.h>
#include <ert/util/ert_unique_ptr.hpp>


namespace Opm {

  /*
    This function will write the data solution data in the DataMap
    @data as an ECLIPSE restart file, in addition to the solution
    fields the ECLIPSE restart file will have a minimum (hopefully
    sufficient) amount of header information.

    The ECLIPSE restart files come in two varietes; unified restart
    files which have all the report steps lumped together in one large
    chunk and non-unified restart files which are one file for each
    report step. In addition the files can be either formatted
    (i.e. ASCII) or unformatted (i.e. binary).

    The writeECLData() function has two hardcoded settings:
    'file_type' and 'fmt_file' which regulate which type of files the
    should be created. The extension of the files follow a convention:

      Unified, formatted    : .FUNRST
      Unified, unformatted  : .UNRST
      Multiple, formatted   : .Fnnnn
      Multiple, unformatted : .Xnnnn

    For the multiple files the 'nnnn' part is the report number,
    formatted with '%04d' format specifier. The writeECLData()
    function will use the ecl_util_alloc_filename() function to create
    an ECLIPSE filename according to this conventions.
  */

  void writeECLData(int nx, int ny, int nz, int nactive,
                    data::Solution data,
                    const int current_step,
                    const double current_time,
                    time_t current_posix_time,
                    const std::string& output_dir,
                    const std::string& base_name) {

    ecl_file_enum file_type = ECL_UNIFIED_RESTART_FILE;  // Alternatively ECL_RESTART_FILE for multiple restart files.
    bool fmt_file           = false;

    char * filename         = ecl_util_alloc_filename(output_dir.c_str() , base_name.c_str() , file_type , fmt_file , current_step );
    int phases              = ECL_OIL_PHASE + ECL_WATER_PHASE;
    ecl_rst_file_type * rst_file;

    if (current_step > 0 && file_type == ECL_UNIFIED_RESTART_FILE)
      rst_file = ecl_rst_file_open_append( filename );
    else
      rst_file = ecl_rst_file_open_write( filename );

    {
      ecl_rsthead_type rsthead_data = {};

      const int num_wells    = 0;
      const int niwelz       = 0;
      const int nzwelz       = 0;
      const int niconz       = 0;
      const int ncwmax       = 0;

      rsthead_data.nx        = nx;
      rsthead_data.ny        = ny;
      rsthead_data.nz        = nz;
      rsthead_data.nwells    = num_wells;
      rsthead_data.niwelz    = niwelz;
      rsthead_data.nzwelz    = nzwelz;
      rsthead_data.niconz    = niconz;
      rsthead_data.ncwmax    = ncwmax;
      rsthead_data.nactive   = nactive;
      rsthead_data.phase_sum = phases;
      rsthead_data.sim_time  = current_posix_time;

      rsthead_data.sim_days = current_time * Opm::Metric::Time; //Data for doubhead

      ecl_rst_file_fwrite_header( rst_file , current_step , &rsthead_data);
    }

    ecl_rst_file_start_solution( rst_file );

    using eclkw = ERT::ert_unique_ptr< ecl_kw_type, ecl_kw_free >;
    for (const auto&  elm : data) {
        if (elm.second.target == data::TargetType::RESTART_SOLUTION) {
            eclkw kw( ecl_kw_alloc( elm.first.c_str() , nactive, ECL_FLOAT_TYPE ) );

            for( int i = 0; i < nactive; i++ )
                ecl_kw_iset_float( kw.get(), i, elm.second.data[ i ] );

            ecl_rst_file_add_kw( rst_file, kw.get() );
        }
    }

    ecl_rst_file_end_solution( rst_file );
    ecl_rst_file_close( rst_file );
    free(filename);
  }
}

#else // that is, we have not defined HAVE_ERT

namespace Opm
{

    void writeECLData(int, int, int, int,
                      data::Solution,
                      const int,
                      const double,
                      time_t,
                      const std::string&,
                      const std::string&)
    {
        throw std::runtime_error(
            "Cannot call writeECLData() without ERT library support. "
            "Reconfigure opm-output with ERT support and recompile."
            );
    }
}

#endif
