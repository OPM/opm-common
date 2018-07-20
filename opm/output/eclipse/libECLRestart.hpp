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
//#ifndef RESTART_IO_HPP
//#define RESTART_IO_HPP
#ifndef libECLRestart_hpp
#define libECLRestart_hpp

#include <cassert>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <map>
#include <type_traits>
#include <vector>

#include <stdbool.h>
#include <time.h>

#include <sys/types.h> 
#include <sys/stat.h>

//#ifndef ERT_VECTOR_H
//#define ERT_VECTOR_H

//#include <ert/util/node_data.h>
//#include <ert/util/type_macros.h>
//#include <ert/util/int_vector.h>

//jal - comment on the following include to avoid duplication
#include <opm/parser/eclipse/Units/UnitSystem.hpp>
#include <opm/parser/eclipse/EclipseState/Runspec.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Well.hpp>

#include <opm/output/data/Cells.hpp>
#include <opm/output/data/Solution.hpp>
#include <opm/output/data/Wells.hpp>
#include <opm/output/eclipse/RestartValue.hpp>

//#include <ert/ecl/EclKW.hpp>
#include <ert/ecl/FortIO.hpp>
//#include <ert/ecl/ecl_rsthead.h>
//#include <ert/ecl/ecl_rst_file.h>
//#include <ert/util/util.h>
#include <ert/ecl/fortio.h>

namespace Opm {

class EclipseGrid;
class EclipseState;
class Phases;
class Schedule;

namespace RestartIO {

  typedef struct {
    // The report step is from the SEQNUM keyword for unified files,
    // and inferred from the filename for non unified files.
    int    report_step;
    int    day;
    int    year;
    int    month;
    time_t sim_time;
    int    version;         // 100, 300, 500 (Eclipse300-Thermal)
    int    phase_sum;       // Oil = 1   Gas = 2    Water = 4

    ert_ecl_unit_enum unit_system;

    int    nx;
    int    ny;
    int    nz;
    int    nactive;
    /*-----------------------------------------------------------------*/
    /* All fields below the line are taken literally (apart from
       lowercasing) from the section about restart files in the
       ECLIPSE File Formats Reference Manual. The elements typically
       serve as dimensions in the ?WEL, ?SEG and ?CON arrays.
    */

    // Pure well properties
    int    nwells;          // Number of wells
    int    niwelz;          // Number of elements pr well in IWEL array
    int    nzwelz;          // Number of 8 character words pr well in ZWEL array
    int    nxwelz;          // Number of elements pr well in XWEL array.

    // Connection properties
    int    niconz;          // Number of elements per completion in ICON array
    int    ncwmax;          // Maximum number of completions per well
    int    nsconz;          // Number of elements per completion in SCON array
    int    nxconz;          // Number of elements per completion in XCON array

    // Segment properties
    int    nisegz;          // Number of entries pr segment in the ISEG array
    int    nsegmx;          // The maximum number of segments pr well
    int    nswlmx;          // The maximum number of segmented wells
    int    nlbrmx;          // The maximum number of lateral branches pr well
    int    nilbrz;          // The number of entries pr segment in ILBR array
    int    nrsegz;          // The number of entries pr segment in RSEG array

    // Properteies from the LOGIHEAD keyword:
    bool   dualp;


    // Properties from the DOUBHEAD keyword:
    double sim_days;
  } ecl_rsthead_type;


typedef enum {
  ECL_METRIC_UNITS = 1,
  ECL_FIELD_UNITS  = 2,
  ECL_LAB_UNITS    = 3,
  ECL_PVT_M_UNITS  = 4
} ert_ecl_unit_enum;

  
  
typedef enum { ECL_OTHER_FILE           = 0   ,
               ECL_RESTART_FILE         = 1   ,
               ECL_UNIFIED_RESTART_FILE = 2   ,
               ECL_SUMMARY_FILE         = 4   ,
               ECL_UNIFIED_SUMMARY_FILE = 8   ,
               ECL_SUMMARY_HEADER_FILE  = 16  ,
               ECL_GRID_FILE            = 32  ,
               ECL_EGRID_FILE           = 64  ,
               ECL_INIT_FILE            = 128 ,
               ECL_RFT_FILE             = 256 ,
               ECL_DATA_FILE            = 512 } ecl_file_enum;


/*
  The type of an eclipse keyword is carried by a struct
  ecl_type_struct which contains a type enum, and the size in bytes of
  one such element. These structs are for the most part handled with
  value semantics, and created with macros ECL_INT, ECL_FLOAT and so
  on.

  The macros in C use designated initializers, whereas the C++ macros
  use a constructor, for this reason this file has two slightly
  different code paths for C and C++.
*/

#define ECL_STRING8_LENGTH   8
#define ECL_TYPE_LENGTH      4
#define ECL_KW_HEADER_DATA_SIZE   ECL_STRING8_LENGTH + ECL_TYPE_LENGTH + 4

typedef enum {
  ECL_CHAR_TYPE   = 0,
  ECL_FLOAT_TYPE  = 1,
  ECL_DOUBLE_TYPE = 2,
  ECL_INT_TYPE    = 3,
  ECL_BOOL_TYPE   = 4,
  ECL_MESS_TYPE   = 5,
  ECL_STRING_TYPE = 7
} ecl_type_enum;

#define ECL_TYPE_ENUM_DEFS {.value = 0 , .name = "ECL_CHAR_TYPE"}, \
{.value = 1 , .name = "ECL_FLOAT_TYPE"} ,                          \
{.value = 2 , .name = "ECL_DOUBLE_TYPE"},                          \
{.value = 3 , .name = "ECL_INT_TYPE"},                             \
{.value = 4 , .name = "ECL_BOOL_TYPE"},                            \
{.value = 5 , .name = "ECL_MESS_TYPE"},                            \
{.value = 7 , .name = "ECL_STRING_TYPE"}

typedef enum {
  ECL_KW_READ_OK = 0,
  ECL_KW_READ_FAIL = 1
} ecl_read_status_enum;


typedef enum  {CTYPE_VOID_POINTER = 1,
	       CTYPE_INT_VALUE    = 2, 
	       CTYPE_DOUBLE_VALUE = 3, 
	       CTYPE_FLOAT_VALUE  = 4 , 
	       CTYPE_CHAR_VALUE   = 5 , 
	       CTYPE_BOOL_VALUE   = 6 , 
	       CTYPE_SIZE_T_VALUE = 7 ,
	       CTYPE_INVALID      = 100} node_ctype;


typedef enum {
  ECL_FILE_CLOSE_STREAM  =  1 ,  /*
                                    This flag will close the underlying FILE object between each access; this is
                                    mainly to save filedescriptors in cases where many ecl_file instances are open at
                                    the same time. */
  //
  ECL_FILE_WRITABLE      =  2    /*
                                    This flag opens the file in a mode where it can be updated and modified, but it
                                    must still exist and be readable. I.e. this should not compared with the normal:
                                    fopen(filename , "w") where an existing file is truncated to zero upon successfull
                                    open.
                                 */
} ecl_file_flag_type;

/*
   This header file contains names and indices of ECLIPSE keywords
   which have special significance in various files. Observe that many
   of the keywords like e.g. INTEHEAD occur in many different file
   types, with partly overlapping layout and values.
 */

/*****************************************************************/
/*                INIT files:                                    */
/*****************************************************************/

#define INTEHEAD_KW  "INTEHEAD"
#define LOGIHEAD_KW  "LOGIHEAD"
#define DOUBHEAD_KW  "DOUBHEAD"
/*
   Observe that many of the elements in the INTEHEAD keyword is shared
   between the restart and init files. The ones listed below here are
   in both the INIT and the restart files. In addition the restart
   files have many well related items which are only in the restart
   files.
*/
#define INTEHEAD_DAY_INDEX      64
#define INTEHEAD_MONTH_INDEX    65
#define INTEHEAD_YEAR_INDEX     66
#define DOUBHEAD_DAYS_INDEX      0
// - unit-system modification
#define INTEHEAD_UNIT_INDEX      2
#define INTEHEAD_NX_INDEX        8
#define INTEHEAD_NY_INDEX        9
#define INTEHEAD_NZ_INDEX       10
#define INTEHEAD_NACTIVE_INDEX  11
#define INTEHEAD_PHASE_INDEX    14
#define INTEHEAD_ECLIPSE100_VALUE        100

#define INTEHEAD_NWELLS_INDEX  16     // Number of wells
#define INTEHEAD_NIWELZ_INDEX  24     // Number of elements pr. well in the IWEL array.
#define INTEHEAD_NZWELZ_INDEX  27     // Number of 8 character words pr. well
#define INTEHEAD_NCWMAX_INDEX  17     // Maximum number of completions per well
#define INTEHEAD_NWGMAX_INDEX  19     // Maximum number of wells in any group
#define INTEHEAD_NGMAXZ_INDEX  20     // Maximum number of groups in field
#define INTEHEAD_NICONZ_INDEX  32     // Number of elements pr completion in the ICON array.
#define INTEHEAD_NIGRPZ_INDEX  36     // Number of elements pr group in the IGRP array.
#define INTEHEAD_NSWLMX_INDEX  175
#define INTEHEAD_NSEGMX_INDEX  176
#define INTEHEAD_NISEGZ_INDEX  178
#define INTEHEAD_IPROG_INDEX    94

#define INTEHEAD_RESTART_SIZE             180

#define LOGIHEAD_RESTART_SIZE             15
#define DOUBHEAD_RESTART_SIZE              1

#define LOGIHEAD_RADIAL100_INDEX                    4   /* use of radial grids is interchanged between */
#define LOGIHEAD_RADIAL300_INDEX                    3

#define LOGIHEAD_DUALP_INDEX                       14

#define STARTSOL_KW  "STARTSOL"
#define ENDSOL_KW    "ENDSOL"


/*
  Observe that the values given as _ITEM are not indices which can
  be directly used in the IWEL or ICON keywords; an offset must be
  added.
*/

#define IWEL_HEADI_INDEX               0
#define IWEL_HEADJ_INDEX               1
//#define IWEL_HEADK_INDEX               2
#define IWEL_CONNECTIONS_INDEX         4
#define IWEL_GROUP_INDEX               5
#define IWEL_TYPE_INDEX                6
#define IWEL_STATUS_INDEX             10

#define ICON_IC_INDEX         0
#define ICON_I_INDEX          1
#define ICON_J_INDEX          2
#define ICON_K_INDEX          3
#define ICON_STATUS_INDEX     5
#define ICON_DIRECTION_INDEX 13

#ifdef ERT_WINDOWS
//#define UTIL_PATH_SEP_STRING           "\\"   /* A \0 terminated separator used when we want a (char *) instance.                   */
#define UTIL_PATH_SEP_CHAR             '\\'   /* A simple character used when we want an actual char instance (i.e. not a pointer). */
#else
//#define UTIL_PATH_SEP_STRING           "/"   /* A \0 terminated separator used when we want a (char *) instance.                   */
#define UTIL_PATH_SEP_CHAR             '/'   /* A simple character used when we want an actual char instance (i.e. not a pointer). */
#endif // ERT_WINDOWS


typedef struct {
  int    index;
  int value;
} sort_node_type;


#ifdef ERT_WINDOWS_LFS
//struct _stat64;
typedef struct _stat64 stat_type;
typedef __int64 offset_type;
#else
//struct stat;
typedef struct stat stat_type;
#ifdef HAVE_FSEEKO
  typedef off_t offset_type;
#else
  typedef long offset_type;
#endif // HAVE_FSEEKO
#endif // ERT_WINDOWS_LFS

typedef uint32_t      (hashf_type) (const char *key, size_t len);
struct ecl_file_struct;
typedef struct ecl_file_struct ecl_file_type;
struct hash_node_struct;
typedef struct hash_node_struct hash_node_type;
struct hash_struct;
typedef struct hash_struct      hash_type;
struct node_data_struct;
typedef struct node_data_struct node_data_type;
struct hash_sll_struct;
typedef struct hash_sll_struct hash_sll_type;
typedef int lock_type;
struct vector_struct;
typedef struct vector_struct vector_type;
struct int_vector_struct;
typedef struct int_vector_struct int_vector_type;
//typedef int (int_ftype) (int);
typedef void       * (  copyc_ftype ) (const void *);
typedef void         (  free_ftype )  (void *);
struct ecl_rst_file_struct; 
typedef struct ecl_rst_file_struct    ecl_rst_file_type;

struct ecl_file_view_struct; 
typedef struct ecl_file_view_struct  ecl_file_view_type;

struct ecl_type_struct {
   //::Opm::RestartIO::ecl_type_enum type;
   //size_t element_size;
//orig
  const ::Opm::RestartIO::ecl_type_enum type;
  const size_t element_size;
 };  
typedef struct ecl_type_struct       ecl_data_type;


struct ecl_rst_file_struct {
  fortio_type * fortio;
  bool          unified;
  bool          fmt_file;
}; 


//struct ecl_kw_struct;
// 
struct ecl_kw_struct {
  UTIL_TYPE_ID_DECLARATION;
  int               size;
  ::Opm::RestartIO::ecl_data_type     data_type;
  char            * header8;              /* Header which is right padded with ' ' to become exactly 8 characters long. Should only be used internally.*/
  char            * header;               /* Header which is trimmed to no-space. */
  char            * data;                 /* The actual data vector. */
  bool              shared_data;          /* Whether this keyword has shared data or not. */
};
typedef struct ecl_kw_struct          ecl_kw_type;

struct ecl_file_kw_struct;
typedef struct ecl_file_kw_struct ecl_file_kw_type;
struct inv_map_struct;
typedef struct inv_map_struct inv_map_type;
struct size_t_vector_struct;
typedef struct size_t_vector_struct size_t_vector_type;
typedef size_t (size_t_ftype) (size_t);  
struct perm_vector_struct;
typedef struct perm_vector_struct perm_vector_type;
struct stringlist_struct;
typedef struct stringlist_struct stringlist_type;

::Opm::RestartIO::ecl_data_type * ECL_INT_F( ::Opm::RestartIO::ecl_type_enum * ecl_type_int, size_t * ecl_t_enum);
::Opm::RestartIO::ecl_data_type      ecl_type_create_from_type(const ::Opm::RestartIO::ecl_type_enum);
::Opm::RestartIO::ecl_data_type      ecl_type_create_from_name(const char *);

::Opm::RestartIO::ecl_type_enum      ecl_type_get_type(const ::Opm::RestartIO::ecl_data_type);
char *             ecl_type_alloc_name(const ::Opm::RestartIO::ecl_data_type);

void                ecl_rsthead_free( ::Opm::RestartIO::ecl_rsthead_type * rsthead );
::Opm::RestartIO::ecl_rsthead_type  * ecl_rsthead_alloc_from_kw( int report_step , const ::Opm::RestartIO::ecl_kw_type * intehead_kw , 
				const ::Opm::RestartIO::ecl_kw_type * doubhead_kw , const ::Opm::RestartIO::ecl_kw_type * logihead_kw );
Opm::RestartIO::ecl_rsthead_type  * ecl_rsthead_alloc( const Opm::RestartIO::ecl_file_view_type * rst_file , int report_step);
Opm::RestartIO::ecl_rsthead_type  * ecl_rsthead_alloc_empty(void);
::Opm::RestartIO::ecl_type_enum ecl_type_get_type(const ::Opm::RestartIO::ecl_data_type ecl_type);
time_t  ecl_rsthead_date( const ::Opm::RestartIO::ecl_kw_type * intehead_kw );
time_t ecl_util_make_date(int mday , int month , int year);
time_t ecl_util_make_date__(int mday , int month , int year, int * __year_offset);
time_t util_make_date_utc(int mday , int month , int year);
time_t util_make_datetime_utc(int sec, int min, int hour , int mday , int month , int year);
void ecl_util_set_date_values(time_t t , int * mday , int * month , int * year);
void util_set_date_values_utc(time_t t , int * mday , int * month , int * year);
void util_time_utc( time_t * t , struct tm * ts );
::Opm::RestartIO::ecl_data_type  ecl_kw_get_data_type(const ::Opm::RestartIO::ecl_kw_type *);
int 		    ecl_type_get_sizeof_iotype(const ::Opm::RestartIO::ecl_data_type ecl_type); 
void                ecl_rsthead_fprintf( const ::Opm::RestartIO::ecl_rsthead_type * header , FILE * stream);
void                ecl_rsthead_fprintf_struct( const ::Opm::RestartIO::ecl_rsthead_type * header , FILE * stream);
bool                ecl_rsthead_equal( const ::Opm::RestartIO::ecl_rsthead_type * header1 , const Opm::RestartIO::ecl_rsthead_type * header2);
bool                ecl_type_is_numeric(const ::Opm::RestartIO::ecl_data_type);
bool                ecl_type_is_equal(const ::Opm::RestartIO::ecl_data_type, const ::Opm::RestartIO::ecl_data_type);
bool                ecl_type_is_bool(const ::Opm::RestartIO::ecl_data_type);
bool                ecl_type_is_int(const ::Opm::RestartIO::ecl_data_type);
bool                ecl_type_is_float(const ::Opm::RestartIO::ecl_data_type);
bool                ecl_type_is_double(const ::Opm::RestartIO::ecl_data_type);
bool                ecl_file_view_has_kw( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view, const char * kw);
bool                hash_has_key(const hash_type *, const char *);
::Opm::RestartIO::hash_type       * hash_alloc(void);
::Opm::RestartIO::hash_sll_type  **hash_sll_alloc_table(int );
bool                hash_node_key_eq(const ::Opm::RestartIO::hash_node_type * , uint32_t  , const char *);
double              ecl_rsthead_get_sim_days( const ::Opm::RestartIO::ecl_rsthead_type * header );
int                 ecl_rsthead_get_report_step( const ::Opm::RestartIO::ecl_rsthead_type * header );
time_t              ecl_rsthead_get_sim_time( const ::Opm::RestartIO::ecl_rsthead_type * header );
bool                util_file_exists(const char *);
bool         util_entry_exists( const char * entry );
size_t       util_file_size(const char *);
size_t       util_fd_size(int fd);
int          util_fstat(int fileno, ::Opm::RestartIO::stat_type * stat_info);
bool         util_fmt_bit8   (const char *);
bool         util_fmt_bit8_stream(FILE * );
::Opm::RestartIO::offset_type  util_ftell(FILE * stream);
void      *  util_calloc( size_t elements , size_t element_size );
void      *  util_alloc_copy(const void * , size_t );
void      *  util_realloc(void *  , size_t  );
void         util_fread (void *, size_t , size_t , FILE * , const char * );
int          util_int_min   (int     , int);
void     util_endian_flip_vector(void *data, int element_size , int elements);
void     size_t_vector_append(::Opm::RestartIO::size_t_vector_type *  , size_t);
void     size_t_vector_iset(::Opm::RestartIO::size_t_vector_type *    , int , size_t);
void     size_t_vector_permute(::Opm::RestartIO::size_t_vector_type * vector , const ::Opm::RestartIO::perm_vector_type * perm);
void     size_t_vector_set_read_only( ::Opm::RestartIO::size_t_vector_type * vector , bool read_only);
int      vector_append_owned_ref( ::Opm::RestartIO::vector_type * , const void * , ::Opm::RestartIO::free_ftype * del);
::Opm::RestartIO::size_t_vector_type * size_t_vector_alloc( int init_size , size_t );  
int          util_stat(const char * filename , ::Opm::RestartIO::stat_type * stat_info);
/*
  The two loose functions RestartIO::save() and RestartIO::load() can
  be used to save and load reservoir and well state from restart
  files. Observe that these functions 'just do it', i.e. the checking
  of which report step to load from, if output is enabled at all and
  so on is handled by an outer scope.

  If the filename corresponds to unified eclipse restart file,
  i.e. UNRST the functions will seek correctly to the correct report
  step, and truncate in the case of save. For any other filename the
  functions will start reading and writing from file offset zero. If
  the input filename does not correspond to a unified restart file
  there is no consistency checking between filename and report step;
  i.e. these calls:

     load("CASE.X0010" , 99 , ...)
     save("CASE.X0010" , 99 , ...)

   will read and write to the file "CASE.X0010" - completely ignoring
   the report step argument '99'.
*/
const char   * ecl_kw_get_header8(const ::Opm::RestartIO::ecl_kw_type *);
const char   * ecl_kw_get_header(const ::Opm::RestartIO::ecl_kw_type * ecl_kw );
 
::Opm::RestartIO::ecl_rst_file_type * ecl_rst_file_open_read( const char * filename );
::Opm::RestartIO::ecl_rst_file_type * ecl_rst_file_open_write( const char * filename );
::Opm::RestartIO::ecl_rst_file_type * ecl_rst_file_open_append( const char * filename );
::Opm::RestartIO::ecl_rst_file_type * ecl_rst_file_open_write_seek( const char * filename , int report_step);
void ecl_rst_file_close( ::Opm::RestartIO::ecl_rst_file_type * rst_file );
void ecl_rst_file_start_solution( ::Opm::RestartIO::ecl_rst_file_type * rst_file );
void ecl_rst_file_end_solution( ::Opm::RestartIO::ecl_rst_file_type * rst_file );
void ecl_rst_file_fwrite_SEQNUM( Opm::RestartIO::ecl_rst_file_type * rst_file , int seqnum );
void ecl_rst_file_fwrite_header( ::Opm::RestartIO::ecl_rst_file_type * rst_file , int seqnum ,
                                  ::Opm::RestartIO::ecl_rsthead_type * rsthead_data );
void ecl_rst_file_add_kw(::Opm::RestartIO::ecl_rst_file_type * rst_file , const ::Opm::RestartIO::ecl_kw_type * ecl_kw );
::Opm::RestartIO::offset_type   ecl_rst_file_ftell(const ::Opm::RestartIO::ecl_rst_file_type * rst_file );
void *  util_malloc(size_t size);

bool            ecl_util_unified_file(const char *filename);
::Opm::RestartIO::ecl_file_enum   ecl_util_get_file_type(const char * , bool * , int * );
bool            ecl_util_fmt_file(const char * filename , bool * __fmt_file);

std::string EclFilename( const std::string& base, ::Opm::RestartIO::ecl_file_enum file_type , int report_step, bool fmt_file = false);
std::string EclFilename( const std::string& base, ::Opm::RestartIO::ecl_file_enum file_type , bool fmt_file = false);

std::string EclFilename( const std::string& path, const std::string& base, ::Opm::RestartIO::ecl_file_enum file_type , int report_step, bool fmt_file = false);
std::string EclFilename( const std::string& path, const std::string& base, ::Opm::RestartIO::ecl_file_enum file_type , bool fmt_file = false);

::Opm::RestartIO::ecl_file_enum EclFiletype( const std::string& filename );
int     ecl_kw_get_size(const ::Opm::RestartIO::ecl_kw_type *);
bool ecl_kw_fwrite(const ::Opm::RestartIO::ecl_kw_type *ecl_kw , fortio_type *fortio);
void ecl_kw_resize(::Opm::RestartIO::ecl_kw_type * ecl_kw, int new_size);
const char  *  ecl_kw_iget_char_ptr( const ::Opm::RestartIO::ecl_kw_type * ecl_kw , int i);
void * ecl_kw_get_data_ref(const ::Opm::RestartIO::ecl_kw_type *ecl_kw);

void ecl_kw_scalar_set_bool( ::Opm::RestartIO::ecl_kw_type * ecl_kw , bool bool_value);
bool ecl_kw_size_and_type_equal( const ::Opm::RestartIO::ecl_kw_type *ecl_kw1 , const ::Opm::RestartIO::ecl_kw_type * ecl_kw2 );



::Opm::RestartIO::ecl_kw_type * ecl_kw_alloc( const char * header , int size , ::Opm::RestartIO::ecl_data_type data_type );
void ecl_kw_set_memcpy_data(::Opm::RestartIO::ecl_kw_type * , const void *);
::Opm::RestartIO::ecl_kw_type *  ecl_kw_fread_alloc(fortio_type *);
void           ecl_kw_set_header_name(::Opm::RestartIO::ecl_kw_type * , const char * );
void           ecl_kw_iset_string8(::Opm::RestartIO::ecl_kw_type * ecl_kw , int index , const char *s8);
::Opm::RestartIO::ecl_read_status_enum ecl_kw_fread_header(::Opm::RestartIO::ecl_kw_type *, fortio_type *);
::Opm::RestartIO::ecl_kw_type * ecl_kw_alloc_new(const char * header ,  int size, ::Opm::RestartIO::ecl_data_type data_type , const void * data);

::Opm::RestartIO::ecl_kw_type * ecl_kw_alloc_empty();

::Opm::RestartIO::ecl_kw_type * ecl_file_kw_get_kw_ptr( ::Opm::RestartIO::ecl_file_kw_type * file_kw );
::Opm::RestartIO::ecl_kw_type      * ecl_file_kw_get_kw( ::Opm::RestartIO::ecl_file_kw_type * file_kw , fortio_type * fortio, ::Opm::RestartIO::inv_map_type * inv_map);
::Opm::RestartIO::inv_map_type     * inv_map_alloc(void);
bool ecl_kw_fread_realloc(::Opm::RestartIO::ecl_kw_type *, fortio_type *);

bool           ecl_kw_name_equal( const ::Opm::RestartIO::ecl_kw_type * ecl_kw , const char * name);
bool           ecl_kw_fread_realloc_data(::Opm::RestartIO::ecl_kw_type *ecl_kw, fortio_type *fortio);
bool           ecl_kw_fread_data(::Opm::RestartIO::ecl_kw_type *ecl_kw, fortio_type *fortio);
bool           ecl_type_is_alpha(const ::Opm::RestartIO::ecl_data_type);
bool           ecl_kw_fskip_data(::Opm::RestartIO::ecl_kw_type *ecl_kw, fortio_type *fortio);
bool           ecl_kw_fskip_data__(::Opm::RestartIO::ecl_data_type, int, fortio_type *);
char          * ecl_util_alloc_filename(const char * /* path */, const char * /* base */, ::Opm::RestartIO::ecl_file_enum , bool /* fmt_file */ , int /*report_nr*/);

char         * util_alloc_string_copy(const char *);
char         * util_alloc_sprintf(const char *  , ...);
char       * util_alloc_sprintf_va(const char * fmt , va_list ap);
char       * util_alloc_filename(const char * , const char *  , const char * );

void         * ecl_kw_alloc_data_copy(const ::Opm::RestartIO::ecl_kw_type * ecl_kw);
::Opm::RestartIO::ecl_kw_type *  ecl_kw_alloc_new_shared(const char * ,  int , ::Opm::RestartIO::ecl_data_type , void * );
void *  ecl_kw_iget_ptr(const ::Opm::RestartIO::ecl_kw_type *, int);

// next line check if needed

//const char *   ecl_kw_iget_string_ptr(const ::Opm::RestartIO::ecl_kw_type *, int);
void *  ecl_kw_get_ptr(const ::Opm::RestartIO::ecl_kw_type *ecl_kw);

int     ecl_type_get_sizeof_ctype_fortio(const ::Opm::RestartIO::ecl_data_type);
int     ecl_file_view_get_global_index( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , const char * kw , int ith);
int     util_int_min(int, int);
void ecl_kw_free(::Opm::RestartIO::ecl_kw_type *ecl_kw);

void ecl_kw_memcpy_data( ::Opm::RestartIO::ecl_kw_type * target , const ::Opm::RestartIO::ecl_kw_type * src);

void ecl_kw_memcpy(::Opm::RestartIO::ecl_kw_type *target, const ::Opm::RestartIO::ecl_kw_type *src);
::Opm::RestartIO::ecl_kw_type *ecl_kw_alloc_copy(const ::Opm::RestartIO::ecl_kw_type *src);
::Opm::RestartIO::ecl_kw_type  * ecl_file_view_iget_named_kw( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , const char * kw, int ith);
::Opm::RestartIO::ecl_file_kw_type        * ecl_file_view_iget_named_file_kw( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , const char * kw, int ith);
::Opm::RestartIO::ecl_file_kw_type * ecl_file_kw_alloc( const ::Opm::RestartIO::ecl_kw_type * ecl_kw , ::Opm::RestartIO::offset_type offset);
::Opm::RestartIO::ecl_file_kw_type * ecl_file_kw_alloc0( const char * header , ::Opm::RestartIO::ecl_data_type data_type , int size , ::Opm::RestartIO::offset_type offset);
bool   ecl_file_kw_fskip_data( const ::Opm::RestartIO::ecl_file_kw_type * file_kw , fortio_type * fortio);
const char * ecl_file_kw_get_header( const ::Opm::RestartIO::ecl_file_kw_type * file_kw );

bool   ecl_kw_fskip_data__( ::Opm::RestartIO::ecl_data_type, int, fortio_type *);
::Opm::RestartIO::ecl_data_type   ecl_file_kw_get_data_type(const ::Opm::RestartIO::ecl_file_kw_type * file_kw);
::Opm::RestartIO::ecl_file_kw_type * ecl_file_view_iget_file_kw( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , int global_index);
::Opm::RestartIO::ecl_file_view_type * ecl_file_get_restart_view( ::Opm::RestartIO::ecl_file_type * ecl_file , 
								  int input_index, int report_step , time_t sim_time, double sim_days);
::Opm::RestartIO::ecl_file_view_type * ecl_file_view_add_restart_view(::Opm::RestartIO::ecl_file_view_type * file_view , 
								      int seqnum_index, int report_step , time_t sim_time, double sim_days);
::Opm::RestartIO::ecl_file_view_type * ecl_file_get_global_view( ::Opm::RestartIO::ecl_file_type * ecl_file );
::Opm::RestartIO::ecl_file_view_type      * ecl_file_view_alloc( fortio_type * fortio , int * flags , ::Opm::RestartIO::inv_map_type * inv_map , bool owner );
void      ecl_file_view_add_kw( ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , ::Opm::RestartIO::ecl_file_kw_type * file_kw);
void      ecl_file_view_free( ::Opm::RestartIO::ecl_file_view_type * ecl_file_view );
void      ecl_file_select_global( ::Opm::RestartIO::ecl_file_type * ecl_file );
void      vector_free(::Opm::RestartIO::vector_type * );
void      vector_clear(::Opm::RestartIO::vector_type * vector); 
void      hash_free(::Opm::RestartIO::hash_type *);
void      hash_sll_free(::Opm::RestartIO::hash_sll_type *);
void      stringlist_free(::Opm::RestartIO::stringlist_type *);
void      inv_map_free( ::Opm::RestartIO::inv_map_type * map );
void      size_t_vector_free(::Opm::RestartIO::size_t_vector_type *);

::Opm::RestartIO::ecl_file_type  * ecl_file_open( const char * filename , int flags);
void   ecl_file_close( ::Opm::RestartIO::ecl_file_type * ecl_file );
::Opm::RestartIO::perm_vector_type *   size_t_vector_alloc_sort_perm(const ::Opm::RestartIO::size_t_vector_type * vector);
::Opm::RestartIO::perm_vector_type * perm_vector_alloc( int * perm_input , int size );
int                                  perm_vector_iget( const ::Opm::RestartIO::perm_vector_type * perm, int index);

::Opm::RestartIO::stringlist_type * stringlist_alloc_new(void);
void              stringlist_clear(::Opm::RestartIO::stringlist_type * );
void              stringlist_append_copy(::Opm::RestartIO::stringlist_type * , const char *);

bool ecl_file_view_flags_set( const ::Opm::RestartIO::ecl_file_view_type * file_view, int query_flags);
void ecl_file_view_make_index( ::Opm::RestartIO::ecl_file_view_type * ecl_file_view );
bool ecl_file_view_check_flags( int state_flags , int query_flags);
int  ecl_file_view_iget_occurence( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , int global_index);
::Opm::RestartIO::ecl_kw_type  * ecl_file_view_iget_kw( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , int index);
::Opm::RestartIO::ecl_file_view_type * ecl_file_view_add_blockview(const ::Opm::RestartIO::ecl_file_view_type * file_view , const char * header, int occurence);
void ecl_file_view_free__( void * arg );

void ecl_kw_alloc_data(::Opm::RestartIO::ecl_kw_type *ecl_kw);
void ecl_file_kw_free__( void * arg );
int  ecl_file_view_find_kw_value( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , const char * kw , const void * value);
int  ecl_file_view_seqnum_index_from_sim_time( ::Opm::RestartIO::ecl_file_view_type * parent_map , time_t sim_time);
int  ecl_file_view_seqnum_index_from_sim_days( ::Opm::RestartIO::ecl_file_view_type * file_view , double sim_days);
int  ecl_file_view_get_num_named_kw(const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , const char * kw);
::Opm::RestartIO::ecl_file_view_type * ecl_file_view_alloc_blockview(const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , const char * header, int occurence);
::Opm::RestartIO::ecl_file_view_type * ecl_file_view_alloc_blockview2(const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , const char * start_kw, const char * end_kw, int occurence);
bool  ecl_file_view_has_sim_time( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , time_t sim_time);
time_t ecl_file_view_iget_restart_sim_date(const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , int seqnum_index);
bool ecl_file_view_has_sim_days( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , double sim_days);
double ecl_file_view_iget_restart_sim_days(const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , int seqnum_index);

void ecl_kw_free_data(::Opm::RestartIO::ecl_kw_type *ecl_kw);
bool ecl_kw_data_equal( const ::Opm::RestartIO::ecl_kw_type * ecl_kw , const void * data);

::Opm::RestartIO::ecl_type_enum  ecl_kw_get_type(const ::Opm::RestartIO::ecl_kw_type *);

void ecl_kw_fwrite_header(const ::Opm::RestartIO::ecl_kw_type *ecl_kw , fortio_type *fortio);
void ecl_kw_fwrite_data(const ::Opm::RestartIO::ecl_kw_type *_ecl_kw , fortio_type *fortio);
void    util_abort__(const char * file , const char * function , int line , const char * fmt , ...);
size_t  ecl_kw_get_sizeof_ctype(const ::Opm::RestartIO::ecl_kw_type *);
size_t  size_t_vector_idel( ::Opm::RestartIO::size_t_vector_type * vector , int index);
size_t  size_t_vector_iget(const ::Opm::RestartIO::size_t_vector_type * , int);
void    ecl_kw_iset(::Opm::RestartIO::ecl_kw_type *ecl_kw , int i , const void *iptr);
void    ecl_kw_free__(void *void_ecl_kw);
void ecl_kw_iset_bool(::Opm::RestartIO::ecl_kw_type * ecl_kw , int i , bool bool_value);
void * vector_iget(const ::Opm::RestartIO::vector_type * , int );
int    vector_get_size( const ::Opm::RestartIO::vector_type * );
const void  * vector_iget_const(const ::Opm::RestartIO::vector_type * , int );
::Opm::RestartIO::int_vector_type * int_vector_alloc( int init_size , int );
void   int_vector_set_read_only( ::Opm::RestartIO::int_vector_type * vector , bool read_only);
void   int_vector_iset(::Opm::RestartIO::int_vector_type *       , int , int);
void   int_vector_free__(void *);
int    int_vector_size(const ::Opm::RestartIO::int_vector_type * );
const int * int_vector_get_const_ptr(const ::Opm::RestartIO::int_vector_type * );

void   int_vector_free(::Opm::RestartIO::int_vector_type *);
void   vector_append_buffer(::Opm::RestartIO::vector_type * , const void * , int);
void   int_vector_free_container(::Opm::RestartIO::int_vector_type * vector);
void   int_vector_append(::Opm::RestartIO::int_vector_type *     , int);

void   size_t_vector_idel_block( ::Opm::RestartIO::size_t_vector_type * vector , int index , int block_size);
int    size_t_vector_index_sorted(const ::Opm::RestartIO::size_t_vector_type * vector , size_t value);
::Opm::RestartIO::vector_type * vector_alloc_new(void);
void   vector_clear(::Opm::RestartIO::vector_type * vector);
void * node_data_get_ptr(const ::Opm::RestartIO::node_data_type *);
void   node_data_free(::Opm::RestartIO::node_data_type *);
void   node_data_free_container(::Opm::RestartIO::node_data_type * );
::Opm::RestartIO::node_data_type  * node_data_alloc_ptr(const void * , ::Opm::RestartIO::copyc_ftype * , ::Opm::RestartIO::free_ftype *);
void * hash_get(const ::Opm::RestartIO::hash_type *, const char *);

bool   hash_has_key(const ::Opm::RestartIO::hash_type *, const char *);
void   hash_clear(::Opm::RestartIO::hash_type *);
void   hash_resize(::Opm::RestartIO::hash_type *hash, int new_size);
int    hash_get_size(const ::Opm::RestartIO::hash_type *);
bool   hash_sll_empty(const ::Opm::RestartIO::hash_sll_type * hash_sll);
void   hash_sll_del_node(::Opm::RestartIO::hash_sll_type * , ::Opm::RestartIO::hash_node_type *);
void   hash_sll_add_node(::Opm::RestartIO::hash_sll_type *, ::Opm::RestartIO::hash_node_type *);

::Opm::RestartIO::hash_node_type * hash_sll_get_head(const ::Opm::RestartIO::hash_sll_type *);
const char * hash_node_get_key(const ::Opm::RestartIO::hash_node_type * );
uint32_t     hash_node_get_table_index(const ::Opm::RestartIO::hash_node_type * );
void         hash_node_set_next(::Opm::RestartIO::hash_node_type * , const ::Opm::RestartIO::hash_node_type * );
void         hash_node_free(::Opm::RestartIO::hash_node_type *);
void         hash_insert_hash_owned_ref(::Opm::RestartIO::hash_type *, const char * , const void *, ::Opm::RestartIO::free_ftype *);
::Opm::RestartIO::hash_node_type * hash_node_alloc_new(const char *, ::Opm::RestartIO::node_data_type * , ::Opm::RestartIO::hashf_type *, uint32_t);
uint32_t         hash_node_set_table_index(::Opm::RestartIO::hash_node_type *, uint32_t );
::Opm::RestartIO::node_data_type * node_data_alloc_buffer(const void *, int );
void util_safe_free(void *ptr);
void *  util_realloc(void *  , size_t  );
bool    util_file_exists(const char *);
char *  util_alloc_string_copy(const char *);
char * util_alloc_strip_copy(const char *);
bool     util_double_approx_equal( double d1 , double d2);
bool     util_double_approx_equal__( double d1 , double d2, double rel_eps, double abs_eps);
void     util_strupr(char *s);
char *   util_alloc_strupr_copy(const char * s);
bool     util_sscanf_int(const char * buffer , int * value);

char * ecl_type_alloc_name(const ::Opm::RestartIO::ecl_data_type ecl_type);
bool   ecl_util_fmt_file(const char * filename , bool * __fmt_file);

bool   util_entry_exists( const char * entry );

void vector_grow_NULL( ::Opm::RestartIO::vector_type * vector , int new_size );
int vector_append_ref(::Opm::RestartIO::vector_type * vector , const void * data);
void ecl_kw_assert_index(const ::Opm::RestartIO::ecl_kw_type *ecl_kw , int index, const char *caller);
void * ecl_kw_iget_ptr_static(const ::Opm::RestartIO::ecl_kw_type *ecl_kw , int i);
void ecl_kw_iget_static(const ::Opm::RestartIO::ecl_kw_type *ecl_kw , int i , void *iptr);
void ecl_kw_iset_static(::Opm::RestartIO::ecl_kw_type *ecl_kw , int i , const void *iptr);

template< typename > struct ecl_type {};

template<> struct ecl_type< float >
 { static const ::Opm::RestartIO::ecl_type_enum type { ::Opm::RestartIO::ECL_FLOAT_TYPE }; };

template<> struct ecl_type< double >
 { static const ::Opm::RestartIO::ecl_type_enum type { ::Opm::RestartIO::ECL_DOUBLE_TYPE }; };

template<> struct ecl_type< int >
 { static const ::Opm::RestartIO::ecl_type_enum type { ::Opm::RestartIO::ECL_INT_TYPE }; };

template<> struct ecl_type< char* >
 { static const ::Opm::RestartIO::ecl_type_enum type { ::Opm::RestartIO::ECL_CHAR_TYPE }; };

template<> struct ecl_type< const char* >
 { static const ::Opm::RestartIO::ecl_type_enum type { ::Opm::RestartIO::ECL_CHAR_TYPE }; };
 
 
template <typename T>
 
class EclKW_ref {
public:
     explicit EclKW_ref( ::Opm::RestartIO::ecl_kw_type * kw ) : m_kw( kw ) {
         if( ::Opm::RestartIO::ecl_type_get_type(::Opm::RestartIO::ecl_kw_get_data_type( kw )) != ::Opm::RestartIO::ecl_type< T >::type )
             throw std::invalid_argument("Type error");
     }

     EclKW_ref() noexcept = default;

     const char* name() const {
         return ::Opm::RestartIO::ecl_kw_get_header( this->m_kw );
     }

     size_t size() const {
           return ::Opm::RestartIO::ecl_kw_get_size( this->m_kw ) ;
     }

   void fwrite(ERT::FortIO& fortio) const {
         ::Opm::RestartIO::ecl_kw_fwrite( this->m_kw , fortio.get() );
     }

     T at( size_t i ) const {
         return *static_cast< T* >( ecl_kw_iget_ptr( this->m_kw, i ) );
     }

     T& operator[](size_t i) {
         return *static_cast< T* >( ecl_kw_iget_ptr( this->m_kw, i ) );
     }

     const typename std::remove_pointer< T >::type* data() const {
         using Tp = const typename std::remove_pointer< T >::type*;
         return static_cast< Tp >( ecl_kw_get_ptr( this->m_kw ) );
     }

     ::Opm::RestartIO::ecl_kw_type* get() const {
         return this->m_kw;
     }

     void resize(size_t new_size) {
        ::Opm::RestartIO::ecl_kw_resize( this->m_kw , new_size );
     }

protected:
     ::Opm::RestartIO::ecl_kw_type* m_kw = nullptr;
};

template<>
inline const char* EclKW_ref< const char* >::at( size_t i ) const {
    return ::Opm::RestartIO::ecl_kw_iget_char_ptr( this->m_kw, i );
}


/*
  The current implementation of "string" storage in the underlying C
  ecl_kw structure does not lend itself to easily implement
  operator[]. We have therefor explicitly deleted it here.
*/

template<>
const char*& EclKW_ref< const char* >::operator[]( size_t i )  = delete;

template <typename T> class EclKW;       // Generic
template <>           class EclKW<bool>; // Specialisation for bool

template< typename T >
class EclKW : public ::Opm::RestartIO::EclKW_ref< T > {
    private:
        using base = ::Opm::RestartIO::EclKW_ref< T >;

    public:
        using ::Opm::RestartIO::EclKW_ref < T > ::EclKW_ref;

        EclKW( const EclKW& ) = delete;
        EclKW( EclKW&& rhs ) : base( rhs.m_kw ) {
            rhs.m_kw = nullptr;
        }

        ~EclKW() {
            if( this->m_kw ) ::Opm::RestartIO::ecl_kw_free( this->m_kw );
        }

     EclKW( const std::string& kw, int size_ ) :
       base( ::Opm::RestartIO::ecl_kw_alloc( kw.c_str(), size_, ::Opm::RestartIO::ecl_type_create_from_type(ecl_type< T >::type) ) )
        {}

        EclKW( const std::string& kw, const std::vector< T >& data ) :
            EclKW( kw, data.size() )
        {
            ::Opm::RestartIO::ecl_kw_set_memcpy_data( this->m_kw, data.data() );
        }

        template< typename U >
        EclKW( const std::string& kw, const std::vector< U >& data ) :
            EclKW( kw, data.size() )
        {
            T* target = static_cast< T* >( ecl_kw_get_ptr( this->m_kw ) );

            for( size_t i = 0; i < data.size(); ++i )
                target[ i ] = T( data[ i ] );
        }

        static EclKW load( ERT::FortIO& fortio ) {
            ::Opm::RestartIO::ecl_kw_type* c_ptr = ::Opm::RestartIO::ecl_kw_fread_alloc( fortio.get() );

            if( !c_ptr )
                throw std::invalid_argument("fread kw failed - EOF?");

            return EclKW( c_ptr );
        }
};

template<> inline
EclKW< const char* >::EclKW( const std::string& kw,
                             const std::vector< const char* >& data ) :
    EclKW( kw, data.size() )
{
    auto* ptr = this->get();
    for( size_t i = 0; i < data.size(); ++i )
        ::Opm::RestartIO::ecl_kw_iset_string8( ptr, i, data[ i ] );
}

/// Specialisation of class EclKW<T> for T==bool.
///
/// Mainly for outputting a \code std::vector<bool> \endcode as a keyword to
/// a restart or summary file (e.g., keyword LOGIHEAD).
template <>
class EclKW<bool>
{
public:
    explicit EclKW(const std::string& kw, const std::vector<bool>& data)
        : m_kw(::Opm::RestartIO::ecl_kw_alloc(kw.c_str(), data.size(), ECL_BOOL))
    {
        if (m_kw != nullptr) {
            const auto n = static_cast<int>(data.size());

            for (auto i = 0*n; i < n; ++i) {
                ::Opm::RestartIO::ecl_kw_iset_bool(this->m_kw, i, data[i]);
            }
        }
    }

    EclKW(const EclKW& rhs)
        : m_kw(::Opm::RestartIO::ecl_kw_alloc_copy(rhs.m_kw))
    {
    }

    EclKW(EclKW&& rhs)
        : m_kw(rhs.m_kw)
    {
        rhs.m_kw = nullptr;
    }

    EclKW& operator=(const EclKW& rhs)
    {
        if (rhs.m_kw != this->m_kw) {
            return *this;       // Self assignment (nothing to do)
        }

        this->clear();

        this->m_kw = ::Opm::RestartIO::ecl_kw_alloc_copy(rhs.m_kw);

        return *this;
    }

    EclKW& operator=(EclKW&& rhs)
    {
        assert (this != &rhs);  // kw = move(kw) is bug in the caller...

        this->m_kw = rhs.m_kw;
        rhs.m_kw = nullptr;

        return *this;
    }

    ~EclKW()
    {
        this->clear();
    }

    const ::Opm::RestartIO::ecl_kw_type* get() const {
        return this->m_kw;
    }

private:
    ::Opm::RestartIO::ecl_kw_type* m_kw = nullptr;

    void clear()
    {
        if (this->m_kw != nullptr) { // kw_free(nullptr) is undefined
            ::Opm::RestartIO::ecl_kw_free(this->m_kw);
        }

        this->m_kw = nullptr;
    }
};

template <typename T , void (*F)(T*)>
struct deleter
{
  void operator() (T * arg) const {
       F( arg );
    }
};
 
template <typename T , void (*F)(T*)>
using ert_unique_ptr = std::unique_ptr<T, deleter<T,F> >;


template <typename T> 
T * ecl_kw_get_type_ptr (const ::Opm::RestartIO::ecl_kw_type * ecl_kw, ::Opm::RestartIO::ecl_type_enum ECL_TYPE)
{ 
  /* implementation */
    if (::Opm::RestartIO::ecl_kw_get_type(ecl_kw) != ECL_TYPE)                                                                          \
    util_abort("%s: Keyword: %s is wrong type - aborting \n",__func__ , ::Opm::RestartIO::ecl_kw_get_header8(ecl_kw));                \
  return (T *) ecl_kw->data;   
}

  template <typename T> 
T ecl_kw_iget_type (const ::Opm::RestartIO::ecl_kw_type * ecl_kw, ::Opm::RestartIO::ecl_type_enum ECL_TYPE, int i)
{ 
  /* implementation */
  T value;                                                                                              \
  if (::Opm::RestartIO::ecl_kw_get_type(ecl_kw) != ECL_TYPE)                                                                  \
    util_abort("%s: Keyword: %s is wrong type - aborting \n",__func__ , ::Opm::RestartIO::ecl_kw_get_header8(ecl_kw));        \
    ::Opm::RestartIO::ecl_kw_iget_static(ecl_kw , i , &value);                                                                  \
  return value;
}


template <typename T> 
void ecl_kw_scalar_set_type( ::Opm::RestartIO::ecl_kw_type * ecl_kw , ::Opm::RestartIO::ecl_type_enum ECL_TYPE , T value){  
  /* implementation */
 if (::Opm::RestartIO::ecl_kw_get_type(ecl_kw) == ECL_TYPE) {                                    \
    T * data = static_cast<T *>(::Opm::RestartIO::ecl_kw_get_data_ref(ecl_kw));                         \
    int i;                                                              \
    for (i=0;i < ecl_kw->size; i++)                                     \
      data[i] = value;                                                  \
 } else util_abort("%s: wrong type\n",__func__);                        \
}

template <typename T> 
void ecl_kw_iset_type(::Opm::RestartIO::ecl_kw_type * ecl_kw, ::Opm::RestartIO::ecl_type_enum ECL_TYPE, int i, T value) {                                      \
  if (::Opm::RestartIO::ecl_kw_get_type(ecl_kw) != ECL_TYPE)                                                                  \
    util_abort("%s: Keyword: %s is wrong type - aborting \n",__func__ , ::Opm::RestartIO::ecl_kw_get_header8(ecl_kw));        \
  ::Opm::RestartIO::ecl_kw_iset_static(ecl_kw , i , &value);                                                                  \
}                         \


//#undef  ECL_INT 
#undef  ECL_FLOAT 
#undef  ECL_DOUBLE
#undef  ECL_BOOL 
#undef  ECL_CHAR
//#undef  ECL_MESS 
#undef  ECL_STRING
  
#ifdef __cplusplus
//#undef ECL_INT
#define ECL_INT_2 ::Opm::RestartIO::ecl_data_type{::Opm::RestartIO::ECL_INT_TYPE, sizeof(int)}
#define ECL_FLOAT ::Opm::RestartIO::ecl_data_type{ ECL_FLOAT_TYPE, sizeof(float)}
#define ECL_DOUBLE ::Opm::RestartIO::ecl_data_type{ ECL_DOUBLE_TYPE, sizeof(double)}
#define ECL_BOOL ::Opm::RestartIO::ecl_data_type{ ECL_BOOL_TYPE, sizeof(int)}
#define ECL_CHAR ::Opm::RestartIO::ecl_data_type{ ECL_CHAR_TYPE, ECL_STRING8_LENGTH + 1}
#define ECL_MESS_2 ::Opm::RestartIO::ecl_data_type{ ECL_MESS_TYPE, 0}
#define ECL_STRING(size) ::Opm::RestartIO::ecl_data_type{ECL_STRING_TYPE, (size) + 1}

}

#else

#define ECL_CHAR (::Opm::RestartIO::ecl_data_type) {.type = ECL_CHAR_TYPE, .element_size = ECL_STRING8_LENGTH + 1}
#define ECL_INT_2 (::Opm::RestartIO::ecl_data_type) {.type = ::Opm::RestartIO::ECL_INT_TYPE, .element_size = sizeof(int)}
#define ECL_FLOAT (::Opm::RestartIO::ecl_data_type) {.type = ECL_FLOAT_TYPE, .element_size = sizeof(float)}
#define ECL_DOUBLE (::Opm::RestartIO::ecl_data_type) {.type = ECL_DOUBLE_TYPE, .element_size = sizeof(double)}
#define ECL_BOOL (::Opm::RestartIO::ecl_data_type) {.type = ECL_BOOL_TYPE, .element_size = sizeof(int)}
#define ECL_MESS_2 (::Opm::RestartIO::ecl_data_type) {.type = ECL_MESS_TYPE, .element_size = 0}
#define ECL_STRING(size) (::Opm::RestartIO::ecl_data_type) {.type = ECL_STRING_TYPE, .element_size = (size) + 1}

#endif // __cplusplus


#undef util_abort
#ifdef _MSC_VER
#define util_abort(fmt , ...) ::Opm::RestartIO::util_abort__(__FILE__ , __func__ , __LINE__ , fmt , __VA_ARGS__)
#elif __GNUC__
/* Also clang defines the __GNUC__ symbol */
#define util_abort(fmt , ...) ::Opm::RestartIO::util_abort__(__FILE__ , __func__ , __LINE__ , fmt , ##__VA_ARGS__)
#endif // _MSC_VER

  
}

// #ifndef ERT_ECL_ENDIAN_FLIP_H
// #define ERT_ECL_ENDIAN_FLIP_H
#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


/**
   This header file checks if the ECLIPSE endianness and the hardware
   endianness are equal, and defines the macro ECL_ENDIAN_FLIP
   accordingly.
   
   All the ecl_xxx functions will use the ECL_ENDIAN_FLIP macro to
   determine whether the endian flip should be performed. When opening
   a fortio instance explicitly you can use the ECL_ENDIAN_FLIP macro
   to get the endianness correct (for ECLIPSE usage that is).  
*/

#define ECLIPSE_BYTE_ORDER  __BIG_ENDIAN   // Alternatively: __LITTLE_ENDIAN

#ifdef BYTE_ORDER
  #if  BYTE_ORDER == ECLIPSE_BYTE_ORDER
    #define ECL_ENDIAN_FLIP false
  #else
    #define ECL_ENDIAN_FLIP true
  #endif  // BYTE_ORDER == ECLIPSE_BYTE_ORDER
#else
  #ifdef WIN32
    #define ECL_ENDIAN_FLIP true    // Unconditional byte flip on Windows.
  #else
    #error: The macro BYTE_ORDER is not defined?
  #endif // WIN32
#endif   // BYTE_ORDER

#undef ECLIPSE_BYTE_ORDER


#ifdef __cplusplus
}
#endif // __cplusplus
//#endif

// For unformatted files:
#define ECL_BOOL_TRUE_INT         -1   // Binary representation: 11111111  11111111  11111111  1111111
#define ECL_BOOL_FALSE_INT         0   // Binary representation: 00000000  00000000  00000000  0000000


/*
  The ECLIPSE documentation says that a certain item in the IWEL array
  should indicate the type of the well, the available types are the
  ones given in the enum below. Unfortunately it turns out that when
  the well is closed the integer value in the IWEL array can be 0, if
  the well is indeed closed we accept this zero - otherwise we fail
  hard. Theese hoops are in the well_state_alloc() routine.
*/

#define IWEL_UNDOCUMENTED_ZERO 0
#define IWEL_PRODUCER          1
#define IWEL_OIL_INJECTOR      2
#define IWEL_WATER_INJECTOR    3
#define IWEL_GAS_INJECTOR      4

#define UTIL_TYPE_ID_DECLARATION           int   __type_id
#define UTIL_TYPE_ID_INIT(var , TYPE_ID)   var->__type_id = TYPE_ID;

#define TYPE_ID(int) ;
#define HAVE_VA_COPY

#ifdef HAVE_VA_COPY
#define UTIL_VA_COPY(target,src) va_copy(target,src)
#else
#define UTIL_VA_COPY(target,src) target = src
#endif  // HAVE_VA_COPY

#define UTIL_SAFE_CAST_FUNCTION(type , TYPE_ID)                                          \
type ## _type * type ## _safe_cast( void * __arg ) {                                     \
   if (__arg == NULL) {                                                                  \
      util_abort("%s: runtime cast failed - tried to dereference NULL\n",__func__);      \
      return NULL;                                                                       \
   }                                                                                     \
   {                                                                                     \
      type ## _type * arg = (type ## _type *) __arg;                                     \
      if ( arg->__type_id == TYPE_ID)                                                       \
         return arg;                                                                        \
      else {                                                                                \
         util_abort("%s: runtime cast failed: Got ID:%d  Expected ID:%d \n", __func__ , arg->__type_id , TYPE_ID); \
         return NULL;                                                                       \
      }                                                                                   \
   }                                                                                     \
}

#endif // libECLRestart_hpp
