/*
  Copyright (c) 2016 Statoil ASA
  Copyright (c) 2013-2015 Andreas Lauser
  Copyright (c) 2013 SINTEecl_kw_get_data_refF ICT, Applied Mathematics.
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
#include <opm/output/eclipse/libECLRestart.hpp>
#include <string>
#include <vector>
#include <cstddef>
#include <time.h>
#include <ctime>
#include <cstring>
#include <stdlib.h> 
#include <iostream>
#include <stdio.h>
#include <sys/stat.h>
#include <fstream> 
#include <fcntl.h>
#include <cstdint>
#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <stdarg.h>
#include <assert.h>
#include <signal.h>
#include <pthread.h>
//#include <ert/util/@TYPE@_vector.h>



//  This function is purely a helper function for util_abort().
// start define for util_abort
//#define __USE_GNU       // Must be defined to get access to the dladdr() function; Man page says the symbol should be: _GNU_SOURCE but that does not seem to work?
//#define _GNU_SOURCE     // Must be defined _before_ #include <errno.h> to get the symbol 'program_invocation_name'.

//#include <errno.h>
//#include <stdlib.h>
//#include <string.h>

//#include <ert/util/util.h>
//#include <ert/util/test_util.h>

//#include <stdbool.h>

//#include <dlfcn.h>
//#include <execinfo.h>
//#include <pthread.h>
//#include <pwd.h>
//#include <signal.h>
//#include <unistd.h>

//#if !defined(__GLIBC__)         /* note: not same as __GNUC__ */
//#  if defined (__APPLE__)
//#    include <stdlib.h>         /* alloca   */
//#    include <sys/syslimits.h>  /* PATH_MAX */
//#    include <mach-o/dyld.h>    /* _NSGetExecutablePath */
//#  elif defined (__LINUX__)
//#    include <stdlib.h>         /* alloca   */
//#    include <limits.h>         /* PATH_MAX */
//#    include <unistd.h>         /* readlink */
//#  else
//#    error No known program_invocation_name in runtime library
//#  endif
//#endif  

#include <opm/parser/eclipse/EclipseState/Grid/EclipseGrid.hpp>
#include <opm/parser/eclipse/EclipseState/EclipseState.hpp>
#include <opm/parser/eclipse/EclipseState/Schedule/Schedule.hpp>

#include <opm/output/eclipse/RestartIO.hpp>

//#include <ert/ecl/EclKW.hpp>
#include <ert/ecl/FortIO.hpp>
#include <ert/ecl/EclFilename.hpp>
//#include <ert/ecl/ecl_kw_magic.h>
//#include <ert/ecl/ecl_init_file.h>
//#include <ert/ecl/ecl_file.h>
//#include <ert/ecl/ecl_kw.h>
//#include <ert/ecl/ecl_type.h>
//#include <ert/ecl/ecl_grid.h>
//#include <ert/ecl/ecl_util.h>
//#include <ert/ecl/ecl_rft_file.h>
//#include <ert/ecl/ecl_file_view.h>
//#include <ert/ecl_well/well_const.h>
//#include <ert/ecl/ecl_rsthead.h>
//#include <ert/util/util.h>
#include <ert/ecl/fortio.h>
#define OPM_XWEL      "OPM_XWEL"
#define OPM_IWEL      "OPM_IWEL"
#define IWEL_KW      "IWEL"
#define ZWEL_KW      "ZWEL"
#define ICON_KW      "ICON"

namespace Opm {
namespace RestartIO  {

/*****************************************************************/
/* For some peculiar reason the keyword data is written in blocks, all
   numeric data is in blocks of 1000 elements, and character data is
   in blocks of 105 elements.
*/

#define BLOCKSIZE_NUMERIC  1000
#define BLOCKSIZE_CHAR     105
#define ECL_KW_TYPE_ID  6111098
#define PERM_VECTOR_TYPE_ID 661433
#define VECTOR_TYPE_ID      551087
#define ECL_FILE_ID 776107

#define HAVE_TIMEGM

/*****************************************************************/
/* Format string used when reading and writing formatted
   files. Observe the following about these format strings:

    1. The format string for reading double contains two '%'
       identifiers, that is because doubles are read by parsing a
       prefix and power separately.

    2. For both double and float the write format contains two '%'
       characters - that is because the values are split in a prefix
       and a power prior to writing - see the function
       __fprintf_scientific().

    3. The logical type involves converting back and forth between 'T'
       and 'F' and internal logical representation. The format strings
       are therefor for reading/writing a character.

*/

#define READ_FMT_CHAR     "%8c"
#define READ_FMT_FLOAT    "%gE"
#define READ_FMT_INT      "%d"
#define READ_FMT_MESS     "%8c"
#define READ_FMT_BOOL     "  %c"
#define READ_FMT_DOUBLE   "%lgD%d"


#define WRITE_FMT_CHAR    " '%-8s'"
#define WRITE_FMT_INT     " %11d"
#define WRITE_FMT_FLOAT   "  %11.8fE%+03d"
#define WRITE_FMT_DOUBLE  "  %17.14fD%+03d"
#define WRITE_FMT_MESS    "%s"
#define WRITE_FMT_BOOL    "  %c"

#define SEQNUM_KW    "SEQNUM"       /* Contains the report step as the only data; not
                                       present in non-unified files, where the report
                                       step must be inferred from the filename. */

// For formatted files:
#define BOOL_TRUE_CHAR       'T'
#define BOOL_FALSE_CHAR      'F'

/*****************************************************************/
/* Format string used when writing a formatted header. */
#define WRITE_HEADER_FMT  " '%-8s' %11d '%-4s'\n"


/*****************************************************************/
/* When writing formatted data, the data comes in columns, with a
   certain number of elements in each row, i.e. four columns for float
   data:

   0.000   0.000   0.000   0.000
   0.000   0.000   0.000   0.000
   0.000   0.000   0.000   0.000
   ....

   These #define symbols define the number of columns for the
   different datatypes.
*/
#define COLUMNS_CHAR     7
#define COLUMNS_FLOAT    4
#define COLUMNS_DOUBLE   3
#define COLUMNS_INT      6
#define COLUMNS_MESSAGE  1
#define COLUMNS_BOOL    25


static const char * string_type = "int";
#define TYPE_VECTOR_ID ((int *) string_type )[0]
#define VECTOR_DEFAULT_SIZE 10

#define INTEHEAD_UNIT_INDEX      2

#define HASH_DEFAULT_SIZE 16
#define HASH_TYPE_ID      771065
#define STRINGLIST_TYPE_ID 671855
#define ECL_FILE_KW_TYPE_ID 646107


/*****************************************************************/
/* The string names for the different ECLIPSE low-level
   types.
*/
#define ECL_TYPE_NAME_CHAR     "CHAR"
#define ECL_TYPE_NAME_FLOAT    "REAL"
#define ECL_TYPE_NAME_INT      "INTE"
#define ECL_TYPE_NAME_DOUBLE   "DOUB"
#define ECL_TYPE_NAME_BOOL     "LOGI"
#define ECL_TYPE_NAME_MESSAGE  "MESS"




struct int_vector_struct {
  UTIL_TYPE_ID_DECLARATION;
  int      alloc_size;    /* The allocated size of data. */
  int      size;          /* The index of the last valid - i.e. actively set - element in the vector. */
  int   default_value; /* The data vector is initialized with this value. */
  int * data;          /* The actual data. */
  bool     data_owner;    /* Is the vector owner of the the actual storage data?
                             If this is false the vector can not be resized. */
  bool     read_only;
};  


struct size_t_vector_struct {
  UTIL_TYPE_ID_DECLARATION;
  int      alloc_size;    /* The allocated size of data. */
  int      size;          /* The index of the last valid - i.e. actively set - element in the vector. */
  size_t   default_value; /* The data vector is initialized with this value. */
  size_t * data;          /* The actual data. */
  bool     data_owner;    /* Is the vector owner of the the actual storage data?
                             If this is false the vector can not be resized. */
  bool     read_only;
};



struct hash_node_struct {
  char             *key;
  uint32_t          global_index;
  uint32_t          table_index;
  ::Opm::RestartIO::node_data_type   *data;
  ::Opm::RestartIO::hash_node_type   *next_node;
};



struct inv_map_struct {
  ::Opm::RestartIO::size_t_vector_type * file_kw_ptr;
  ::Opm::RestartIO::size_t_vector_type * ecl_kw_ptr;
  bool                 sorted;
};

struct ecl_file_kw_struct {
  UTIL_TYPE_ID_DECLARATION;
  ::Opm::RestartIO::offset_type      file_offset;
  ::Opm::RestartIO::ecl_data_type    data_type;
  int              kw_size;
  int              ref_count;
  char           * header;
  ecl_kw_type    * kw;
};


struct hash_struct {
  UTIL_TYPE_ID_DECLARATION;
  uint32_t          size;            /* This is the size of the internal table **NOT**NOT** the number of elements in the table. */
  uint32_t          elements;        /* The number of elements in the hash table. */
  double            resize_fill;
  ::Opm::RestartIO::hash_sll_type   **table;
  ::Opm::RestartIO::hashf_type       *hashf;

  ::Opm::RestartIO::lock_type         rwlock;
};

struct node_data_struct {
  ::Opm::RestartIO::node_ctype        ctype;
  void             *data;
  int               buffer_size;  /* This is to facilitate deep copies of buffers. */
  
  /*
    For the copy constructor and delete
    operator the logic is very simple:
    
    if they are present they are used.
  */
  ::Opm::RestartIO::copyc_ftype    *copyc;  /* Copy constructor - can be NULL. */
  ::Opm::RestartIO::free_ftype     *del;    /* Destructor - can be NULL. */ 
};


struct ecl_file_struct {
  UTIL_TYPE_ID_DECLARATION;
  fortio_type       * fortio;       /* The source of all the keywords - must be retained
                                       open for reading for the entire lifetime of the
                                       ecl_file object. */
  ::Opm::RestartIO::ecl_file_view_type * global_view;       /* The index of all the ecl_kw instances in the file. */
  ::Opm::RestartIO::ecl_file_view_type * active_view;       /* The currently active index. */
  bool            read_only;
  int             flags;
  ::Opm::RestartIO::vector_type   * map_stack;
  ::Opm::RestartIO::inv_map_type  * inv_view;
};

/*
  These ifdefs are an attempt to support large files (> 2GB)
  transparently on both Windows and Linux. See source file
  libert_util/src/util_lfs.c for more details.

  During the configure step CMAKE should check the size of (void *)
  and set the ERT_WINDOWS_LFS variable to true if a 64 bit platform is
  detected.
*/


struct hash_sll_struct {
  int              length;
  ::Opm::RestartIO::hash_node_type * head;
};


struct vector_struct {
  UTIL_TYPE_ID_DECLARATION;
  int              alloc_size;   /* The number of elements allocated in the data vector - in general > size. */
  int              size;         /* THe number of elements the user has added to the vector. */
  ::Opm::RestartIO::node_data_type **data;         /* node_data instances - which again contain user data. */
};


struct perm_vector_struct {
  UTIL_TYPE_ID_DECLARATION;
  int   size;
  int * perm;
};


struct stringlist_struct {
  UTIL_TYPE_ID_DECLARATION;
  ::Opm::RestartIO::vector_type * strings;
};



struct ecl_file_view_struct {
  vector_type       * kw_list;      /* This is a vector of ecl_file_kw instances corresponding to the content of the file. */
  hash_type         * kw_index;     /* A hash table with integer vectors of indices - see comment below. */
  stringlist_type   * distinct_kw;  /* A stringlist of the keywords occuring in the file - each string occurs ONLY ONCE. */
  fortio_type       * fortio;       /* The same fortio instance pointer as in the ecl_file styructure. */
  bool                owner;        /* Is this map the owner of the ecl_file_kw instances; only true for the global_map. */
  inv_map_type      * inv_map;      /* Shared reference owned by the ecl_file structure. */
  vector_type       * child_list;
  int               * flags;
};


char * __abort_program_message;
char * __current_executable;

int ecl_type_get_sizeof_ctype(const ::Opm::RestartIO::ecl_data_type ecl_type) {
  return ecl_type.element_size;
}

size_t ecl_kw_get_sizeof_ctype(const ::Opm::RestartIO::ecl_kw_type * ecl_kw) {
  return ::Opm::RestartIO::ecl_type_get_sizeof_ctype(ecl_kw->data_type);
}
void ecl_kw_iset_static(::Opm::RestartIO::ecl_kw_type *ecl_kw , int i , const void *iptr) {
  ::Opm::RestartIO::ecl_kw_assert_index(ecl_kw , i , __func__);
  memcpy(&ecl_kw->data[i * ::Opm::RestartIO::ecl_kw_get_sizeof_ctype(ecl_kw)] , iptr, ::Opm::RestartIO::ecl_kw_get_sizeof_ctype(ecl_kw));
}

void ecl_kw_assert_index(const ::Opm::RestartIO::ecl_kw_type *ecl_kw , int index, const char *caller) {
  if (index < 0 || index >= ecl_kw->size)
    util_abort("%s: Invalid index lookup. kw:%s input_index:%d   size:%d \n",caller , ecl_kw->header , index , ecl_kw->size);
}

void * ecl_kw_iget_ptr_static(const ::Opm::RestartIO::ecl_kw_type *ecl_kw , int i) {
  ::Opm::RestartIO::ecl_kw_assert_index(ecl_kw , i , __func__);
  return &ecl_kw->data[i * ::Opm::RestartIO::ecl_kw_get_sizeof_ctype(ecl_kw)];
}

void ecl_kw_iget_static(const ::Opm::RestartIO::ecl_kw_type *ecl_kw , int i , void *iptr) {
  memcpy(iptr , ::Opm::RestartIO::ecl_kw_iget_ptr_static(ecl_kw , i) , ::Opm::RestartIO::ecl_kw_get_sizeof_ctype(ecl_kw));
}

void util_abort__(const char * file , const char * function , int line , const char * fmt , ...) {
  fprintf(stderr,"\n-----------------------------------------------------------------\n");
  fprintf(stderr,"A fatal error has been detected and the program will abort.\n\n");
  fprintf(stderr, "Current executable : %s\n" , (__current_executable == NULL) ? "<Not set>" : __current_executable);
  fprintf(stderr, "Version info       : %s\n" , (__abort_program_message == NULL) ? "<Not set>" : __abort_program_message);
  fprintf(stderr, "\nError message: ");
  fprintf(stderr , "Abort called from: %s (%s:%d) \n",function , file , line);
  {  
    va_list ap;
    va_start(ap , fmt);
    vfprintf(stderr , fmt , ap);
    va_end(ap);
  } 
  fprintf(stderr,"-----------------------------------------------------------------\n");

  signal(SIGABRT , SIG_DFL);
  fprintf(stderr , "Aborting ... \n");
  assert(0);
  abort();
}


static void int_vector_assert_index(const ::Opm::RestartIO::int_vector_type * vector , int index) {
  if ((index < 0) || (index >= vector->size))
    util_abort("%s: index:%d invalid. Valid interval: [0,%d>.\n",__func__ , index , vector->size);
};

int int_vector_iget(const ::Opm::RestartIO::int_vector_type * vector , int index) {
  ::Opm::RestartIO::int_vector_assert_index(vector , index);
  return vector->data[index];
};

static void * util_malloc__(size_t size) {
  void * data;
  if (size == 0)
    /*
       Not entirely clear from documentation what you get when you
       call malloc( 0 ); this code will return NULL in that case.
    */
    data = NULL;
  else {
    data = std::malloc( size );
    if (data == NULL)
      util_abort("%s: failed to allocate %zu bytes - aborting \n",__func__ , size);

    /*
       Initializing with something different from zero - hopefully
       errors will pop up more easily this way?
    */
    memset(data , 255 , size);
  }
  return data;
}

void * util_malloc(size_t size) {
  return ::Opm::RestartIO::util_malloc__( size );
}


bool ecl_util_unified_file(const char *filename) {
  int report_nr;
  ::Opm::RestartIO::ecl_file_enum file_type;
  bool fmt_file;
  file_type = ::Opm::RestartIO::ecl_util_get_file_type(filename , &fmt_file , &report_nr);

  if ((file_type == ECL_UNIFIED_RESTART_FILE) || (file_type == ECL_UNIFIED_SUMMARY_FILE))
    return true;
  else
    return false;
}


bool ecl_util_fmt_file(const char *filename , bool * __fmt_file) {
  /*const int min_size = 32768;*/
  const int min_size = 256; /* Veeeery small */

  int report_nr;
  ::Opm::RestartIO::ecl_file_enum file_type;
  bool status = true;
  bool fmt_file = 0;

  if (::Opm::RestartIO::util_file_exists(filename)) {
    file_type = ::Opm::RestartIO::ecl_util_get_file_type(filename , &fmt_file , &report_nr);
    if (file_type == ECL_OTHER_FILE) {
      if (::Opm::RestartIO::util_file_size(filename) > min_size)
        fmt_file = ::Opm::RestartIO::util_fmt_bit8(filename);
      else
        status = false; // Do not know ??
    }
  } else {
    file_type = ::Opm::RestartIO::ecl_util_get_file_type(filename , &fmt_file , &report_nr);
    if (file_type == ECL_OTHER_FILE)
      status = false; // Do not know ??
  }

  *__fmt_file = fmt_file;
  return status;
}


size_t util_file_size(const char *file) {
  size_t file_size;

  {
    int fildes = open(file , O_RDONLY);
    if (fildes == -1)
      util_abort("%s: failed to open:%s - %s \n",__func__ , file , strerror(errno));

    file_size = ::Opm::RestartIO::util_fd_size( fildes );
    close(fildes);
  }

  return file_size;
}

int util_fstat(int fileno, ::Opm::RestartIO::stat_type * stat_info) {
#ifdef ERT_WINDOWS_LFS
  return _fstat64(fileno , stat_info);
#else
  return fstat(fileno , stat_info);
#endif
}

size_t util_fd_size(int fd) {
  ::Opm::RestartIO::stat_type buffer;

  ::Opm::RestartIO::util_fstat(fd, &buffer);

  return buffer.st_size;
}


void * util_calloc( size_t elements , size_t element_size ) {
  return util_malloc( elements * element_size );
}


bool util_fmt_bit8_stream(FILE * stream ) {
  const int    min_read      = 256; /* Critically small */
  const double bit8set_limit = 0.00001;
  const int    buffer_size   = 131072;
  long int start_pos         = ::Opm::RestartIO::util_ftell(stream);
  bool fmt_file;
  {
    double bit8set_fraction;
    int N_bit8set = 0;
    int elm_read,i;
    char *buffer = (char*)util_calloc(buffer_size , sizeof * buffer );

    elm_read = fread(buffer , 1 , buffer_size , stream);
    if (elm_read < min_read)
      util_abort("%s: file is too small to automatically determine formatted/unformatted status \n",__func__);

    for (i=0; i < elm_read; i++)
      N_bit8set += (buffer[i] & (1 << 7)) >> 7;


    free(buffer);

    bit8set_fraction = 1.0 * N_bit8set / elm_read;
    if (bit8set_fraction < bit8set_limit)
      fmt_file =  true;
    else
      fmt_file = false;
  }
  util_fseek(stream , start_pos , SEEK_SET);
  return fmt_file;
}

int util_fseek(FILE * stream, offset_type offset, int whence) {
#ifdef ERT_WINDOWS_LFS
  return _fseeki64(stream , offset , whence);
#else
  #ifdef HAVE_FSEEKO
  return fseeko( stream , offset , whence );
  #else
  return fseek( stream , offset , whence );
  #endif
#endif
}

offset_type util_ftell(FILE * stream) {
#ifdef ERT_WINDOWS_LFS
  return _ftelli64(stream);
#else
  #ifdef HAVE_FSEEKO
  return ftello(stream);
  #else
  return ftell(stream);
  #endif
#endif
}

bool util_fmt_bit8(const char *filename ) {
  FILE *stream;
  bool fmt_file = true;

  if (util_file_exists(filename)) {
    stream   = fopen(filename , "r");
    fmt_file = ::Opm::RestartIO::util_fmt_bit8_stream(stream);
    fclose(stream);
  } else
    util_abort("%s: could not find file: %s - aborting \n",__func__ , filename);

  return fmt_file;
}


/**
   Currently only checks if the entry exists - this will return true
   if path points to directory.
*/
bool util_file_exists(const char *filename) {
  return util_entry_exists( filename );
}

bool util_entry_exists( const char * entry ) {
  ::Opm::RestartIO::stat_type stat_buffer;
  int stat_return = ::Opm::RestartIO::util_stat(entry, &stat_buffer);
  if (stat_return == 0)
    return true;
  else {
    if (errno == ENOENT)
      return false;
    else {
      util_abort("%s: error checking for entry:%s  %d/%s \n",__func__ , entry , errno , strerror(errno));
      return false;
    }
  }
}

int util_stat(const char * filename , ::Opm::RestartIO::stat_type * stat_info) {
#ifdef ERT_WINDOWS_LFS
  return _stat64(filename , stat_info);
#else
  return stat(filename , stat_info);
#endif
}

/**
   Observe that all the open() functions expect that filename conforms
   to the standard ECLIPSE conventions, i.e. with extension .FUNRST /
   .UNRST / .Xnnnn / .Fnnnn.
*/


static ::Opm::RestartIO::ecl_rst_file_type * ecl_rst_file_alloc( const char * filename ) {
  bool unified  = ::Opm::RestartIO::ecl_util_unified_file( filename );
  bool fmt_file;
  ::Opm::RestartIO::ecl_rst_file_type * rst_file = static_cast<::Opm::RestartIO::ecl_rst_file_type*>(util_malloc( sizeof * rst_file ));

  if (::Opm::RestartIO::ecl_util_fmt_file( filename , &fmt_file)) {
    rst_file->unified = unified;
    rst_file->fmt_file = fmt_file;
    return rst_file;
  } else {
    util_abort("%s: invalid restart filename:%s - could not determine formatted/unformatted status\n",__func__ , filename);
    return NULL;
  }
}

::Opm::RestartIO::ecl_rst_file_type * ecl_rst_file_open_read( const char * filename ) {
  ::Opm::RestartIO::ecl_rst_file_type * rst_file = ::Opm::RestartIO::ecl_rst_file_alloc( filename );
  rst_file->fortio = fortio_open_reader( filename , rst_file->fmt_file , ECL_ENDIAN_FLIP );
  return rst_file;
}

::Opm::RestartIO::ecl_rst_file_type * ecl_rst_file_open_write( const char * filename ) {
  ::Opm::RestartIO::ecl_rst_file_type * rst_file = ::Opm::RestartIO::ecl_rst_file_alloc( filename  );
  rst_file->fortio = fortio_open_writer( filename , rst_file->fmt_file , ECL_ENDIAN_FLIP );
  return rst_file;
}


::Opm::RestartIO::ecl_rst_file_type * ecl_rst_file_open_append( const char * filename ) {
  ::Opm::RestartIO::ecl_rst_file_type * rst_file = ::Opm::RestartIO::ecl_rst_file_alloc( filename );
  rst_file->fortio = fortio_open_append( filename , rst_file->fmt_file , ECL_ENDIAN_FLIP );
  return rst_file;
}

void ecl_rst_file_close( ::Opm::RestartIO::ecl_rst_file_type * rst_file ) {
  fortio_fclose( rst_file->fortio );
  free( rst_file );
}


void ecl_kw_set_data_type(::Opm::RestartIO::ecl_kw_type * ecl_kw, ::Opm::RestartIO::ecl_data_type data_type) {
    memcpy(&ecl_kw->data_type, &data_type, sizeof data_type);
}


/**
   The function will allocate a new copy of src where leading and
   trailing whitespace has been stripped off. If the source string is
   all blanks a string of length one - only containing \0 is returned,
   i.e. not NULL.

   If src is NULL the function will return NULL. The incoming source
   string is not modified, see the function util_realloc_strip_copy()
   for a similar function implementing realloc() semantics.
*/


char * util_alloc_strip_copy(const char *src) {
  char * target;
  int strip_length = 0;
  int end_index   = strlen(src) - 1;
  while (end_index >= 0 && src[end_index] == ' ')
    end_index--;

  if (end_index >= 0) {

    int start_index = 0;
    while (src[start_index] == ' ')
      start_index++;
    strip_length = end_index - start_index + 1;
    target = (char*)::Opm::RestartIO::util_calloc(strip_length + 1 , sizeof * target );
    memcpy(target , &src[start_index] , strip_length);
  } else
    /* A blank string */
    target = (char*)::Opm::RestartIO::util_calloc(strip_length + 1 , sizeof * target );

  target[strip_length] = '\0';
  return target;
}

/**
   Allocates byte_size bytes of storage, and initializes content with
   the value found in src.
*/

void * util_alloc_copy(const void * src , size_t byte_size ) {
  if (byte_size == 0 && src == NULL)
    return NULL;
  {
    void * next = ::Opm::RestartIO::util_malloc(byte_size );
    memcpy(next , src , byte_size);
    return next;
  }
}


/**
   This function check that a pointer is different from NULL, and
   frees the memory if that is the case.
*/
void util_safe_free(void *ptr) {
   if (ptr != NULL) free(ptr);
}



void ecl_kw_set_header_name(::Opm::RestartIO::ecl_kw_type * ecl_kw , const char * header) {
  ecl_kw->header8 = static_cast<char *>(realloc(ecl_kw->header8 , ECL_STRING8_LENGTH + 1));
  if (strlen(header) <= 8) {
     sprintf(ecl_kw->header8 , "%-8s" , header);

     /* Internalizing a header without the trailing spaces as well. */
     ::Opm::RestartIO::util_safe_free( ecl_kw->header );
     ecl_kw->header = ::Opm::RestartIO::util_alloc_strip_copy( ecl_kw->header8 );
  }
  else {
     ecl_kw->header = static_cast<char *>(::Opm::RestartIO::util_alloc_copy(header, strlen( header ) + 1));
  }

}


static void ecl_kw_initialize(::Opm::RestartIO::ecl_kw_type * ecl_kw , const char *header ,  int size , ::Opm::RestartIO::ecl_data_type data_type) {
  ::Opm::RestartIO::ecl_kw_set_data_type(ecl_kw, data_type);
  ::Opm::RestartIO::ecl_kw_set_header_name(ecl_kw , header);
  ecl_kw->size = size;
}


void * util_realloc(void * old_ptr , size_t new_size ) {
  /* The realloc documentation as ambigous regarding realloc() with size 0 - WE return NULL. */
  if (new_size == 0) {
    if (old_ptr != NULL)
      free(old_ptr);
    return NULL;
  } else {
    void * tmp = realloc(old_ptr , new_size);
    if (tmp == NULL)
      util_abort("%s: failed to realloc %zu bytes - aborting \n",__func__ , new_size);
    return tmp;
  }
}


/**
   This is where the storage buffer of the ecl_kw is allocated.
*/
void ecl_kw_alloc_data(::Opm::RestartIO::ecl_kw_type *ecl_kw) {
  if (ecl_kw->shared_data)
    util_abort("%s: trying to allocate data for ecl_kw object which has been declared with shared storage - aborting \n",__func__);

  {
    size_t byte_size = ecl_kw->size * ::Opm::RestartIO::ecl_kw_get_sizeof_ctype(ecl_kw);
    ecl_kw->data = static_cast<char *>(::Opm::RestartIO::util_realloc(ecl_kw->data , byte_size ));
    memset(ecl_kw->data , 0 , byte_size);
  }
}



::Opm::RestartIO::ecl_kw_type * ecl_kw_alloc( const char * header , int size , ::Opm::RestartIO::ecl_data_type data_type ) {
  ::Opm::RestartIO::ecl_kw_type *ecl_kw;

  ecl_kw = ::Opm::RestartIO::ecl_kw_alloc_empty();
  ::Opm::RestartIO::ecl_kw_initialize(ecl_kw , header , size , data_type);
  ::Opm::RestartIO::ecl_kw_alloc_data(ecl_kw);

  return ecl_kw;
}

/** Allocates a untyped buffer with exactly the same content as the ecl_kw instances data. */
void * ecl_kw_alloc_data_copy(const ::Opm::RestartIO::ecl_kw_type * ecl_kw) {
  void * buffer = ::Opm::RestartIO::util_alloc_copy( ecl_kw->data , ecl_kw->size * ::Opm::RestartIO::ecl_kw_get_sizeof_ctype(ecl_kw) );
  return buffer;
}

::Opm::RestartIO::ecl_kw_type * ecl_kw_alloc_empty() {
  ::Opm::RestartIO::ecl_kw_type *ecl_kw;

  ecl_kw                 = static_cast<::Opm::RestartIO::ecl_kw_type*>(::Opm::RestartIO::util_malloc(sizeof *ecl_kw ));
  ecl_kw->header         = NULL;
  ecl_kw->header8        = NULL;
  ecl_kw->data           = NULL;
  ecl_kw->shared_data    = false;
  ecl_kw->size           = 0;

  UTIL_TYPE_ID_INIT(ecl_kw , ECL_KW_TYPE_ID);

  return ecl_kw;
}


static void ecl_kw_set_shared_ref(::Opm::RestartIO::ecl_kw_type * ecl_kw , void *data_ptr) {
  if (!ecl_kw->shared_data) {
    if (ecl_kw->data != NULL)
      util_abort("%s: can not change to shared for keyword with allocated storage - aborting \n",__func__);
  }
  ecl_kw->shared_data = true;
  ecl_kw->data = static_cast<char *>(data_ptr);
}


::Opm::RestartIO::ecl_kw_type * ecl_kw_alloc_new_shared(const char * header ,  int size, ::Opm::RestartIO::ecl_data_type data_type , void * data) {
  ::Opm::RestartIO::ecl_kw_type *ecl_kw;
  ecl_kw = ::Opm::RestartIO::ecl_kw_alloc_empty();
  ::Opm::RestartIO::ecl_kw_initialize(ecl_kw , header , size , data_type);
  ::Opm::RestartIO::ecl_kw_set_shared_ref(ecl_kw , data);
  return ecl_kw;
}


bool ecl_kw_fread_realloc(::Opm::RestartIO::ecl_kw_type *ecl_kw , fortio_type *fortio) {
  if (ecl_kw_fread_header(ecl_kw , fortio) == ECL_KW_READ_OK)
    return ecl_kw_fread_realloc_data( ecl_kw , fortio );
  else
    return false;
}


::Opm::RestartIO::ecl_kw_type *ecl_kw_fread_alloc(fortio_type *fortio) {
  bool OK;
  ::Opm::RestartIO::ecl_kw_type *ecl_kw = ::Opm::RestartIO::ecl_kw_alloc_empty();
  OK = ::Opm::RestartIO::ecl_kw_fread_realloc(ecl_kw , fortio);
  if (!OK) {
    free(ecl_kw);
    ecl_kw = NULL;
  }

  return ecl_kw;
}


void ecl_kw_set_memcpy_data(::Opm::RestartIO::ecl_kw_type *ecl_kw , const void *src) {
  if (src != NULL)
    memcpy(ecl_kw->data , src , ecl_kw->size * ::Opm::RestartIO::ecl_kw_get_sizeof_ctype(ecl_kw));
}


/**
   The data is copied from the input argument to the ecl_kw; data can be NULL.
*/
::Opm::RestartIO::ecl_kw_type * ecl_kw_alloc_new(const char * header ,  int size, ::Opm::RestartIO::ecl_data_type data_type , const void * data) {
  ::Opm::RestartIO::ecl_kw_type *ecl_kw;
  ecl_kw = ::Opm::RestartIO::ecl_kw_alloc_empty();
  ::Opm::RestartIO::ecl_kw_initialize(ecl_kw , header , size , data_type);
  if (data != NULL) {
    ::Opm::RestartIO::ecl_kw_alloc_data(ecl_kw);
    ::Opm::RestartIO::ecl_kw_set_memcpy_data(ecl_kw , data);
  }
  return ecl_kw;
}

void ecl_kw_free_data(::Opm::RestartIO::ecl_kw_type *ecl_kw) {
  if (!ecl_kw->shared_data)
    ::Opm::RestartIO::util_safe_free(ecl_kw->data);

  ecl_kw->data = NULL;
}

void ecl_kw_free(::Opm::RestartIO::ecl_kw_type *ecl_kw) {
  ::Opm::RestartIO::util_safe_free( ecl_kw->header );
  ::Opm::RestartIO::util_safe_free(ecl_kw->header8);
  ::Opm::RestartIO::ecl_kw_free_data(ecl_kw);
  free(ecl_kw);
}

/*
  This function will scan through the file and look for seqnum
  headers, and position the file pointer in the right location to
  start writing data for the report step given by @report_step. The
  file is truncated, so that the filepointer will be at the (new) EOF
  when returning.
*/


::Opm::RestartIO::ecl_rst_file_type * ecl_rst_file_open_write_seek( const char * filename , int report_step) {
  ::Opm::RestartIO::ecl_rst_file_type * rst_file = ::Opm::RestartIO::ecl_rst_file_alloc( filename  );
  ::Opm::RestartIO::offset_type target_pos = 0;
  bool seqnum_found = false;
  rst_file->fortio = fortio_open_readwrite( filename , rst_file->fmt_file , ECL_ENDIAN_FLIP );
  /*
     If the file does not exist at all the fortio_open_readwrite()
     will fail, we just try again - opening a new file in normal write
     mode, and then immediately returning.
  */
  if (!rst_file->fortio) {
    rst_file->fortio = fortio_open_writer( filename , rst_file->fmt_file , ECL_ENDIAN_FLIP );
    return rst_file;
  }

  fortio_fseek( rst_file->fortio , 0 , SEEK_SET );
  {

    ::Opm::RestartIO::ecl_kw_type * work_kw = ::Opm::RestartIO::ecl_kw_alloc_new("WORK-KW" , 0 , ECL_INT_2, NULL);

    while (true) {
      ::Opm::RestartIO::offset_type current_offset = fortio_ftell( rst_file->fortio );

      if (fortio_read_at_eof(rst_file->fortio)) {
        if (seqnum_found)
          target_pos = current_offset;
        break;
      }

      if (::Opm::RestartIO::ecl_kw_fread_header( work_kw , rst_file->fortio) == ECL_KW_READ_FAIL)
        break;

      if (::Opm::RestartIO::ecl_kw_name_equal( work_kw , SEQNUM_KW)) {
        ::Opm::RestartIO::ecl_kw_fread_realloc_data( work_kw , rst_file->fortio);
	int file_step = ::Opm::RestartIO::ecl_kw_iget_type<int>( work_kw , ECL_INT_TYPE, 0 );
        if (file_step >= report_step) {
          target_pos = current_offset;
          break;
        }
        seqnum_found = true;
      } else
          ::Opm::RestartIO::ecl_kw_fskip_data( work_kw , rst_file->fortio );

    }

    ::Opm::RestartIO::ecl_kw_free( work_kw );
  }

  fortio_fseek( rst_file->fortio , target_pos , SEEK_SET);
  fortio_ftruncate_current( rst_file->fortio );
  return rst_file;
}


static int get_blocksize( ::Opm::RestartIO::ecl_data_type data_type ) {
  if (::Opm::RestartIO::ecl_type_is_alpha(data_type))
    return BLOCKSIZE_CHAR;

  return BLOCKSIZE_NUMERIC;
}

/**
   Static method without a class instance.
*/

bool ecl_kw_fskip_data__(::Opm::RestartIO::ecl_data_type data_type , const int element_count , fortio_type * fortio) {
  if (element_count <= 0)
    return true;

  bool fmt_file = fortio_fmt_file(fortio);
  if (fmt_file) {
    /* Formatted skipping actually involves reading the data - nice ??? */
    ::Opm::RestartIO::ecl_kw_type * tmp_kw = ecl_kw_alloc_empty( );
    ::Opm::RestartIO::ecl_kw_initialize( tmp_kw , "WORK" , element_count , data_type );
    ::Opm::RestartIO::ecl_kw_alloc_data(tmp_kw);
    ::Opm::RestartIO::ecl_kw_fread_data(tmp_kw , fortio);
    ::Opm::RestartIO::ecl_kw_free( tmp_kw );
  } else {
    const int blocksize = ::Opm::RestartIO::get_blocksize( data_type );
    const int block_count = element_count / blocksize + (element_count % blocksize != 0);
    int element_size = ::Opm::RestartIO::ecl_type_get_sizeof_ctype_fortio(data_type);

    if(!fortio_data_fskip(fortio, element_size, element_count, block_count))
      return false;
  }

  return true;
}


::Opm::RestartIO::ecl_data_type ecl_kw_get_data_type(const ::Opm::RestartIO::ecl_kw_type * ecl_kw) {
  return ecl_kw->data_type;
}

bool ecl_kw_fskip_data(::Opm::RestartIO::ecl_kw_type *ecl_kw, fortio_type *fortio) {
  return ::Opm::RestartIO::ecl_kw_fskip_data__( ::Opm::RestartIO::ecl_kw_get_data_type(ecl_kw) , ecl_kw->size , fortio );
}



bool ecl_kw_name_equal( const ::Opm::RestartIO::ecl_kw_type * ecl_kw , const char * name) {
  return (strcmp( ecl_kw->header , name) == 0);
}

/**
   Allocates storage and reads data.
*/
bool ecl_kw_fread_realloc_data(::Opm::RestartIO::ecl_kw_type *ecl_kw, fortio_type *fortio) {
  ::Opm::RestartIO::ecl_kw_alloc_data(ecl_kw);
  return ::Opm::RestartIO::ecl_kw_fread_data(ecl_kw , fortio);
}


static void ecl_kw_endian_convert_data(::Opm::RestartIO::ecl_kw_type *ecl_kw) {
  if (::Opm::RestartIO::ecl_type_is_numeric(ecl_kw->data_type) || ::Opm::RestartIO::ecl_type_is_bool(ecl_kw->data_type))
    ::Opm::RestartIO::util_endian_flip_vector(ecl_kw->data , ::Opm::RestartIO::ecl_kw_get_sizeof_ctype(ecl_kw) , ecl_kw->size);
}


char * util_alloc_sprintf(const char * fmt , ...) {
  char * s;
  va_list ap;
  va_start(ap , fmt);
  s = ::Opm::RestartIO::util_alloc_sprintf_va(fmt , ap);
  va_end(ap);
  return s;
}


static char * alloc_string_name(const ::Opm::RestartIO::ecl_data_type ecl_type) {
  return ::Opm::RestartIO::util_alloc_sprintf(
          "C%03d",
          ::Opm::RestartIO::ecl_type_get_sizeof_ctype_fortio(ecl_type)
          );
}


static char * alloc_read_fmt_string(const ::Opm::RestartIO::ecl_data_type ecl_type) {
  return ::Opm::RestartIO::util_alloc_sprintf(
          "%%%dc",
          ::Opm::RestartIO::ecl_type_get_sizeof_ctype_fortio(ecl_type)
          );
}


bool ecl_type_is_char(const ::Opm::RestartIO::ecl_data_type ecl_type) {
  return (ecl_type.type == ECL_CHAR_TYPE);
}


bool ecl_type_is_mess(const ::Opm::RestartIO::ecl_data_type ecl_type) {
  return (ecl_type.type == ECL_MESS_TYPE);
}


bool ecl_type_is_string(const ::Opm::RestartIO::ecl_data_type ecl_type) {
  return (ecl_type.type == ECL_STRING_TYPE);
}


char * util_alloc_sprintf_va(const char * fmt , va_list ap) {
  char *s = NULL;
  int length;
  va_list tmp_va;

  UTIL_VA_COPY(tmp_va , ap);

  length = vsnprintf(NULL , 0 , fmt , tmp_va);
  s = (char*) ::Opm::RestartIO::util_calloc(length + 1 , sizeof * s );
  vsprintf(s , fmt , ap);
  return s;
}

int ecl_type_get_sizeof_ctype_fortio(const ::Opm::RestartIO::ecl_data_type ecl_type) {
  if(::Opm::RestartIO::ecl_type_is_char(ecl_type) || ::Opm::RestartIO::ecl_type_is_string(ecl_type))
      return ecl_type.element_size - 1;
  else
      return ::Opm::RestartIO::ecl_type_get_sizeof_ctype(ecl_type);
}





char * ecl_type_alloc_name(const ::Opm::RestartIO::ecl_data_type ecl_type) {
  switch (ecl_type.type) {
  case(ECL_CHAR_TYPE):
    return ::Opm::RestartIO::util_alloc_string_copy(ECL_TYPE_NAME_CHAR);
  case(ECL_STRING_TYPE):
    return ::Opm::RestartIO::alloc_string_name(ecl_type);
  case(ECL_FLOAT_TYPE):
    return ::Opm::RestartIO::util_alloc_string_copy(ECL_TYPE_NAME_FLOAT);
  case(ECL_DOUBLE_TYPE):
    return ::Opm::RestartIO::util_alloc_string_copy(ECL_TYPE_NAME_DOUBLE);
  case(ECL_INT_TYPE):
    return ::Opm::RestartIO::util_alloc_string_copy(ECL_TYPE_NAME_INT);
  case(ECL_BOOL_TYPE):
    return ::Opm::RestartIO::util_alloc_string_copy(ECL_TYPE_NAME_BOOL);
  case(ECL_MESS_TYPE):
    return ::Opm::RestartIO::util_alloc_string_copy(ECL_TYPE_NAME_MESSAGE);
  default:
    util_abort("Internal error in %s - internal eclipse_type: %d not recognized - aborting \n",__func__ , ecl_type.type);
    return NULL; /* Dummy */
  }
}

static char * alloc_read_fmt(const ::Opm::RestartIO::ecl_data_type data_type ) {
  switch(::Opm::RestartIO::ecl_type_get_type(data_type)) {
  case(ECL_CHAR_TYPE):
    return ::Opm::RestartIO::util_alloc_string_copy(READ_FMT_CHAR);
  case(ECL_INT_TYPE):
    return ::Opm::RestartIO::util_alloc_string_copy(READ_FMT_INT);
  case(ECL_FLOAT_TYPE):
    return ::Opm::RestartIO::util_alloc_string_copy(READ_FMT_FLOAT);
  case(ECL_DOUBLE_TYPE):
    return ::Opm::RestartIO::util_alloc_string_copy(READ_FMT_DOUBLE);
  case(ECL_BOOL_TYPE):
    return ::Opm::RestartIO::util_alloc_string_copy(READ_FMT_BOOL);
  case(ECL_MESS_TYPE):
    return ::Opm::RestartIO::util_alloc_string_copy(READ_FMT_MESS);
  case(ECL_STRING_TYPE):
    return ::Opm::RestartIO::alloc_read_fmt_string(data_type);
  default:
    util_abort("%s: invalid ecl_type:%s \n",__func__ , ::Opm::RestartIO::ecl_type_alloc_name(data_type));
    return NULL;
  }
}

::Opm::RestartIO::ecl_type_enum ecl_type_get_type(const ::Opm::RestartIO::ecl_data_type ecl_type) {
    return ecl_type.type;
}

::Opm::RestartIO::ecl_type_enum ecl_kw_get_type(const ::Opm::RestartIO::ecl_kw_type * ecl_kw) {
  return ::Opm::RestartIO::ecl_type_get_type(ecl_kw->data_type);
}



static bool ecl_kw_qskip(FILE *stream) {
  const char sep       = '\'';
  const char space     = ' ';
  const char newline   = '\n';
  const char tab       = '\t';
  bool OK = true;
  char c;
  bool cont = true;
  while (cont) {
    c = fgetc(stream);
    if (c == EOF) {
      cont = false;
      OK   = false;
    } else {
      if (c == space || c == newline || c == tab)
        cont = true;
      else if (c == sep)
        cont = false;
    }
  }
  return OK;
}



static bool ecl_kw_fscanf_qstring(char *s , const char *fmt , int len, FILE *stream) {
  const char null_char = '\0';
  char last_sep;
  bool OK;
  OK = ::Opm::RestartIO::ecl_kw_qskip(stream);
  if (OK) {
    int read_count = 0;
    read_count += fscanf(stream , fmt , s);
    s[len] = null_char;
    read_count += fscanf(stream , "%c" , &last_sep);

    if (read_count != 2)
      util_abort("%s: reading \'xxxxxxxx\' formatted string failed \n",__func__);
  }
  return OK;
}


/** Should be: NESTED */

static double __fscanf_ECL_double( FILE * stream , const char * fmt) {
  int    read_count , power;
  double value , arg;
  read_count = fscanf( stream , fmt , &arg , &power);
  if (read_count == 2)
    value = arg * pow(10 , power );
  else {
    util_abort("%s: read failed \n",__func__);
    value = -1;
  }
  return value;
}

int util_int_min(int a , int b) {
  return (a < b) ? a : b;
}


void util_fread(void *ptr , size_t element_size , size_t items, FILE * stream , const char * caller) {
  int items_read = fread(ptr , element_size , items , stream);
  if (items_read != static_cast<int>(items))
    util_abort("%s/%s: only read %d/%d items from disk - aborting.\n %s(%d) \n",caller , __func__ , items_read , items , strerror(errno) , errno);
}


bool ecl_kw_fread_data(::Opm::RestartIO::ecl_kw_type *ecl_kw, fortio_type *fortio) {
  const char null_char         = '\0';
  bool fmt_file                = fortio_fmt_file( fortio );
  if (ecl_kw->size > 0) {
    const int blocksize = ::Opm::RestartIO::get_blocksize( ecl_kw->data_type );
    if (fmt_file) {
      const int blocks      = ecl_kw->size / blocksize + (ecl_kw->size % blocksize == 0 ? 0 : 1);
      char * read_fmt       = ::Opm::RestartIO::alloc_read_fmt( ecl_kw->data_type );
      FILE * stream         = fortio_get_FILE(fortio);
      int    offset         = 0;
      int    index          = 0;
      int    ib,ir;
      for (ib = 0; ib < blocks; ib++) {
        int read_elm = ::Opm::RestartIO::util_int_min((ib + 1) * blocksize , ecl_kw->size) - ib * blocksize;
        for (ir = 0; ir < read_elm; ir++) {
          switch(::Opm::RestartIO::ecl_kw_get_type(ecl_kw)) {
          case(ECL_CHAR_TYPE):
            ::Opm::RestartIO::ecl_kw_fscanf_qstring(&ecl_kw->data[offset] , read_fmt , 8, stream);
            break;
          case(ECL_STRING_TYPE):
            ::Opm::RestartIO::ecl_kw_fscanf_qstring(
                    &ecl_kw->data[offset],
                    read_fmt,
                    ::Opm::RestartIO::ecl_type_get_sizeof_ctype_fortio(::Opm::RestartIO::ecl_kw_get_data_type(ecl_kw)),
                    stream
                    );
            break;
          case(ECL_INT_TYPE):
            {
              int iread = fscanf(stream , read_fmt , (int *) &ecl_kw->data[offset]);
              if (iread != 1)
                util_abort("%s: after reading %d values reading of keyword:%s from:%s failed - aborting \n",__func__ , offset / ::Opm::RestartIO::ecl_kw_get_sizeof_ctype(ecl_kw) , ecl_kw->header8 , fortio_filename_ref(fortio));
            }
            break;
          case(ECL_FLOAT_TYPE):
            {
              int iread = fscanf(stream , read_fmt , (float *) &ecl_kw->data[offset]);
              if (iread != 1) {
                util_abort("%s: after reading %d values reading of keyword:%s from:%s failed - aborting \n",__func__ ,
                           offset / ::Opm::RestartIO::ecl_kw_get_sizeof_ctype(ecl_kw) ,
                           ecl_kw->header8 ,
                           fortio_filename_ref(fortio));
              }
            }
            break;
          case(ECL_DOUBLE_TYPE):
            {
              double value = ::Opm::RestartIO::__fscanf_ECL_double( stream , read_fmt );
              ecl_kw_iset(ecl_kw , index , &value);
            }
            break;
          case(ECL_BOOL_TYPE):
            {
              char bool_char;
              if (fscanf(stream , read_fmt , &bool_char) == 1) {
                if (bool_char == BOOL_TRUE_CHAR)
                  ::Opm::RestartIO::ecl_kw_iset_bool(ecl_kw , index , true);
                else if (bool_char == BOOL_FALSE_CHAR)
                  ::Opm::RestartIO::ecl_kw_iset_bool(ecl_kw , index , false);
                else
                  util_abort("%s: Logical value: [%c] not recogniced - aborting \n", __func__ , bool_char);
              } else
                util_abort("%s: read failed - premature file end? \n",__func__ );
            }
            break;
          case(ECL_MESS_TYPE):
            ::Opm::RestartIO::ecl_kw_fscanf_qstring(&ecl_kw->data[offset] , read_fmt , 8 , stream);
            break;
          default:
            util_abort("%s: Internal error: internal eclipse_type: %d not recognized - aborting \n",__func__ , ::Opm::RestartIO::ecl_kw_get_type(ecl_kw));
          }
          offset += ::Opm::RestartIO::ecl_kw_get_sizeof_ctype(ecl_kw);
          index++;
        }
      }

      /* Skip the trailing newline */
      fortio_fseek( fortio , 1 , SEEK_CUR);
      free(read_fmt);
      return true;
    } else {
      bool read_ok = true;
      if (::Opm::RestartIO::ecl_type_is_char(ecl_kw->data_type) || ::Opm::RestartIO::ecl_type_is_mess(ecl_kw->data_type) || ::Opm::RestartIO::ecl_type_is_string(ecl_kw->data_type)) {
        const int blocks = ecl_kw->size / blocksize + (ecl_kw->size % blocksize == 0 ? 0 : 1);
        int ib = 0;
        while (true) {
          /*
            Due to the necessary terminating \0 characters there is
            not a continous file/memory mapping.
          */
          int  read_elm = ::Opm::RestartIO::util_int_min((ib + 1) * blocksize , ecl_kw->size) - ib * blocksize;
          FILE * stream = fortio_get_FILE(fortio);
          int record_size = fortio_init_read(fortio);
          if (record_size >= 0) {
            int ir;
            const int sizeof_ctype        = ::Opm::RestartIO::ecl_type_get_sizeof_ctype(ecl_kw->data_type);
            const int sizeof_ctype_fortio = ::Opm::RestartIO::ecl_type_get_sizeof_ctype_fortio(ecl_kw->data_type);
            for (ir = 0; ir < read_elm; ir++) {
              ::Opm::RestartIO::util_fread( &ecl_kw->data[(ib * blocksize + ir) * sizeof_ctype] , 1 , sizeof_ctype_fortio , stream , __func__);
              ecl_kw->data[(ib * blocksize + ir) * sizeof_ctype + sizeof_ctype_fortio] = null_char;
            }
            read_ok = fortio_complete_read(fortio , record_size);
          } else
            read_ok = false;

          if (!read_ok)
            break;

          ib++;
          if (ib == blocks)
            break;
        }
      } else {
        /**
           This function handles the fuc***g blocks transparently at a
           low level.
        */
        read_ok = fortio_fread_buffer(fortio , ecl_kw->data , ecl_kw->size * ::Opm::RestartIO::ecl_kw_get_sizeof_ctype(ecl_kw));
        if (read_ok && ECL_ENDIAN_FLIP)
          ::Opm::RestartIO::ecl_kw_endian_convert_data(ecl_kw);
      }
      return read_ok;
    }
  } else
    /* The keyword has zero size - and reading data is trivially OK. */
    return true;
}


bool ecl_type_is_int(const ::Opm::RestartIO::ecl_data_type ecl_type) {
  return (ecl_type.type == ECL_INT_TYPE);
}

bool ecl_type_is_float(const ::Opm::RestartIO::ecl_data_type ecl_type) {
  return (ecl_type.type == ECL_FLOAT_TYPE);
}

bool ecl_type_is_double(const ::Opm::RestartIO::ecl_data_type ecl_type) {
  return (ecl_type.type == ECL_DOUBLE_TYPE);
}

 
bool ecl_type_is_bool(const ::Opm::RestartIO::ecl_data_type ecl_type) {
  return (ecl_type.type == ECL_BOOL_TYPE);
}

bool ecl_type_is_numeric(const ::Opm::RestartIO::ecl_data_type ecl_type) {
  return (::Opm::RestartIO::ecl_type_is_int(ecl_type) ||
          ::Opm::RestartIO::ecl_type_is_float(ecl_type) ||
          ::Opm::RestartIO::ecl_type_is_double(ecl_type));
}

static uint16_t util_endian_convert16( uint16_t u ) {
  return (( u >> 8U ) & 0xFFU) | (( u & 0xFFU) >> 8U);
}


static uint32_t util_endian_convert32( uint32_t u ) {
  const uint32_t m8  = (uint32_t) 0x00FF00FFUL;
  const uint32_t m16 = (uint32_t) 0x0000FFFFUL;

  u = (( u >> 8U ) & m8)   | ((u & m8) << 8U);
  u = (( u >> 16U ) & m16) | ((u & m16) << 16U);
  return u;
}


static uint64_t util_endian_convert64( uint64_t u ) {
  const uint64_t m8  = (uint64_t) 0x00FF00FF00FF00FFULL;
  const uint64_t m16 = (uint64_t) 0x0000FFFF0000FFFFULL;
  const uint64_t m32 = (uint64_t) 0x00000000FFFFFFFFULL;


  u = (( u >> 8U ) & m8)   | ((u & m8) << 8U);
  u = (( u >> 16U ) & m16) | ((u & m16) << 16U);
  u = (( u >> 32U ) & m32) | ((u & m32) << 32U);
  return u;
}

void util_endian_flip_vector(void *data, int element_size , int elements) {
  int i;
  switch (element_size) {
  case(1):
    break;
  case(2):
    {
      uint16_t *tmp16 = (uint16_t *) data;

      for (i = 0; i <elements; i++)
        tmp16[i] = ::Opm::RestartIO::util_endian_convert16(tmp16[i]);
      break;
    }
  case(4):
    {
#ifdef ARCH64
      /*
        In the case of a 64 bit CPU the fastest way to swap 32 bit
        variables will be by swapping two elements in one operation;
        this is provided by the util_endian_convert32_64() function. In the case
        of binary ECLIPSE files this case is quite common, and
        therefor worth supporting as a special case.
      */
      uint64_t *tmp64 = (uint64_t *) data;

      for (i = 0; i <elements/2; i++)
        tmp64[i] = ::Opm::RestartIO::util_endian_convert32_64(tmp64[i]);

      if ( elements & 1 ) {
        // Odd number of elements - flip the last element as an ordinary 32 bit swap.
        uint32_t *tmp32 = (uint32_t *) data;
        tmp32[ elements - 1] = ::Opm::RestartIO::util_endian_convert32( tmp32[elements - 1] );
      }
      break;
#else
      uint32_t *tmp32 = (uint32_t *) data;

      for (i = 0; i <elements; i++)
        tmp32[i] = util_endian_convert32(tmp32[i]);

      break;
#endif
    }
  case(8):
    {
      uint64_t *tmp64 = (uint64_t *) data;

      for (i = 0; i <elements; i++)
        tmp64[i] = ::Opm::RestartIO::util_endian_convert64(tmp64[i]);
      break;
    }
  default:
    fprintf(stderr,"%s: current element size: %d \n",__func__ , element_size);
    util_abort("%s: can only endian flip 1/2/4/8 byte variables - aborting \n",__func__);
  }
}

void ecl_kw_iset(::Opm::RestartIO::ecl_kw_type *ecl_kw , int i , const void *iptr) {
  ::Opm::RestartIO::ecl_kw_iset_static(ecl_kw , i , iptr);
}

const char * ecl_kw_get_header8(const ::Opm::RestartIO::ecl_kw_type *ecl_kw) {
  return ecl_kw->header8;
}

void ecl_kw_iset_bool( ecl_kw_type * ecl_kw , int i , bool bool_value) {
  int  int_value;
  if (::Opm::RestartIO::ecl_kw_get_type(ecl_kw) != ECL_BOOL_TYPE) {
    util_abort("%s: Keyword: %s is wrong type - aborting \n",__func__ , ::Opm::RestartIO::ecl_kw_get_header8(ecl_kw));
  }
  if (bool_value)
    int_value = ECL_BOOL_TRUE_INT;
  else
    int_value = ECL_BOOL_FALSE_INT;

  ::Opm::RestartIO::ecl_kw_iset_static(ecl_kw , i , &int_value);
}

bool ecl_type_is_alpha(const ::Opm::RestartIO::ecl_data_type ecl_type) {
  return (::Opm::RestartIO::ecl_type_is_char(ecl_type) ||
          ::Opm::RestartIO::ecl_type_is_mess(ecl_type) ||
          ::Opm::RestartIO::ecl_type_is_string(ecl_type));
}


char * util_alloc_string_copy(const char *src ) {
  if (src != NULL) {
    int byte_size = (strlen(src) + 1) * sizeof * src;
    char * copy   = (char*)::Opm::RestartIO::util_calloc( byte_size , sizeof * copy );
    memcpy( copy , src , byte_size );
    return copy;
  } else
    return NULL;
}



//namespace {

    static const int NIWELZ = 11; //Number of data elements per well in IWEL array in restart file
    static const int NZWELZ = 3;  //Number of 8-character words per well in ZWEL array restart file
    static const int NICONZ = 15; //Number of data elements per completion in ICON array restart file

    /**
     * The constants NIWELZ and NZWELZ referes to the number of
     * elements per well that we write to the IWEL and ZWEL eclipse
     * restart file data arrays. The constant NICONZ refers to the
     * number of elements per completion in the eclipse restart file
     * ICON data array.These numbers are written to the INTEHEAD
     * header.
     *
     * Observe that all of these values are our "current-best-guess"
     * for how many numbers are needed; there might very well be third
     * party applications out there which have a hard expectation for
     * these values.
     */


/*
  Calling scope will handle the NULL return value, and (optionally)
  reopen the fortio stream and then call the ecl_file_kw_get_kw()
  function.
*/

ecl_kw_type * ecl_file_kw_get_kw_ptr( ::Opm::RestartIO::ecl_file_kw_type * file_kw) {
  if (file_kw->ref_count == 0)
    return NULL;

  file_kw->ref_count++;
  return file_kw->kw;
}


static ::Opm::RestartIO::ecl_kw_type * ecl_file_view_get_kw(const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view, ::Opm::RestartIO::ecl_file_kw_type * file_kw) {
  ::Opm::RestartIO::ecl_kw_type * ecl_kw = ::Opm::RestartIO::ecl_file_kw_get_kw_ptr( file_kw );
  if (!ecl_kw) {
    if (fortio_assert_stream_open( ecl_file_view->fortio )) {

      ecl_kw = ::Opm::RestartIO::ecl_file_kw_get_kw( file_kw , ecl_file_view->fortio , ecl_file_view->inv_map);

      if (::Opm::RestartIO::ecl_file_view_flags_set( ecl_file_view , ECL_FILE_CLOSE_STREAM))
        fortio_fclose_stream( ecl_file_view->fortio );
    }
  }
  return ecl_kw;
}


::Opm::RestartIO::ecl_kw_type * ecl_file_view_iget_named_kw( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , const char * kw, int ith) {
  ::Opm::RestartIO::ecl_file_kw_type * file_kw = ::Opm::RestartIO::ecl_file_view_iget_named_file_kw( ecl_file_view , kw , ith);
  return ::Opm::RestartIO::ecl_file_view_get_kw(ecl_file_view, file_kw);
}


::Opm::RestartIO::ecl_file_kw_type * ecl_file_view_iget_named_file_kw( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , const char * kw, int ith) {
  int global_index = ::Opm::RestartIO::ecl_file_view_get_global_index( ecl_file_view , kw , ith);
  ::Opm::RestartIO::ecl_file_kw_type * file_kw = ::Opm::RestartIO::ecl_file_view_iget_file_kw( ecl_file_view , global_index );
  return file_kw;
}



bool ecl_file_view_flags_set( const ::Opm::RestartIO::ecl_file_view_type * file_view , int query_flags) {
  return ::Opm::RestartIO::ecl_file_view_check_flags( *file_view->flags , query_flags );
}


bool ecl_file_view_check_flags( int state_flags , int query_flags) {
  if ((state_flags & query_flags) == query_flags)
    return true;
  else
    return false;
}


void ecl_kw_resize( ::Opm::RestartIO::ecl_kw_type * ecl_kw, int new_size) {
  if (ecl_kw->shared_data)
    util_abort("%s: trying to allocate data for ecl_kw object which has been declared with shared storage - aborting \n",__func__);

  if (new_size != ecl_kw->size) {
    size_t old_byte_size = ecl_kw->size * ecl_kw_get_sizeof_ctype(ecl_kw);
    size_t new_byte_size = new_size * ::Opm::RestartIO::ecl_kw_get_sizeof_ctype(ecl_kw);
    ecl_kw->data = static_cast<char *>(::Opm::RestartIO::util_realloc(ecl_kw->data , new_byte_size ));
    if (new_byte_size > old_byte_size) {
      size_t offset = old_byte_size;
      memset(&ecl_kw->data[offset] , 0 , new_byte_size - old_byte_size);
    }
    ecl_kw->size = new_size;
  }
}


static void size_t_vector_assert_writable( const ::Opm::RestartIO::size_t_vector_type * vector ) {
  if (vector->read_only)
    util_abort("%s: Sorry - tried to modify a read_only vector instance.\n",__func__);
}

void size_t_vector_permute(::Opm::RestartIO::size_t_vector_type * vector , const ::Opm::RestartIO::perm_vector_type * perm) {
  ::Opm::RestartIO::size_t_vector_assert_writable( vector );
  {
    int i;
    size_t * tmp = (size_t*)::Opm::RestartIO::util_alloc_copy( vector->data , sizeof * tmp * vector->size );
    for (i=0; i < vector->size; i++)
      vector->data[i] = tmp[::Opm::RestartIO::perm_vector_iget( perm , i )];
    free( tmp );
  }
}


static void inv_map_assert_sort( ::Opm::RestartIO::inv_map_type * map ) {
  if (!map->sorted) {
    ::Opm::RestartIO::perm_vector_type * perm = ::Opm::RestartIO::size_t_vector_alloc_sort_perm( map->ecl_kw_ptr );

    ::Opm::RestartIO::size_t_vector_permute( map->ecl_kw_ptr , perm );
    ::Opm::RestartIO::size_t_vector_permute( map->file_kw_ptr , perm );
    map->sorted = true;

    free( perm );
  }
}

static void inv_map_drop_kw( ::Opm::RestartIO::inv_map_type * map , const ::Opm::RestartIO::ecl_kw_type * ecl_kw) {
  ::Opm::RestartIO::inv_map_assert_sort( map );
  {
    int index = ::Opm::RestartIO::size_t_vector_index_sorted( map->ecl_kw_ptr , (size_t) ecl_kw );
    if (index == -1)
      util_abort("%s: trying to drop non-existent kw \n",__func__);

    ::Opm::RestartIO::size_t_vector_idel( map->ecl_kw_ptr  , index );
    ::Opm::RestartIO::size_t_vector_idel( map->file_kw_ptr , index );
    map->sorted = false;
  }
}


static void ecl_file_kw_drop_kw( ::Opm::RestartIO::ecl_file_kw_type * file_kw , ::Opm::RestartIO::inv_map_type * inv_map ) {
  if (file_kw->kw != NULL) {
    ::Opm::RestartIO::inv_map_drop_kw( inv_map , file_kw->kw );
    ::Opm::RestartIO::ecl_kw_free( file_kw->kw );
    file_kw->kw = NULL;
  }
}


void size_t_vector_append(::Opm::RestartIO::size_t_vector_type * vector , size_t value) {
  ::Opm::RestartIO::size_t_vector_iset(vector , vector->size , value);
}

static void inv_map_add_kw( ::Opm::RestartIO::inv_map_type * map , const ::Opm::RestartIO::ecl_file_kw_type * file_kw , const ::Opm::RestartIO::ecl_kw_type * ecl_kw) {
  ::Opm::RestartIO::size_t_vector_append( map->file_kw_ptr , (size_t) file_kw );
  ::Opm::RestartIO::size_t_vector_append( map->ecl_kw_ptr  , (size_t) ecl_kw );
  map->sorted = false;
}

::Opm::RestartIO::ecl_data_type ecl_file_kw_get_data_type(const ::Opm::RestartIO::ecl_file_kw_type * file_kw) {
  return file_kw->data_type;
}

static void ecl_file_kw_assert_kw( const ::Opm::RestartIO::ecl_file_kw_type * file_kw ) {
  if(!::Opm::RestartIO::ecl_type_is_equal(
              ::Opm::RestartIO::ecl_file_kw_get_data_type(file_kw),
              ::Opm::RestartIO::ecl_kw_get_data_type(file_kw->kw)
              ))
    util_abort("%s: type mismatch between header and file.\n",__func__);

  if (file_kw->kw_size != ::Opm::RestartIO::ecl_kw_get_size( file_kw->kw ))
    util_abort("%s: size mismatch between header and file.\n",__func__);

  if (strcmp( file_kw->header , ::Opm::RestartIO::ecl_kw_get_header( file_kw->kw )) != 0 )
    util_abort("%s: name mismatch between header and file.\n",__func__);
}

static void ecl_file_kw_load_kw( ::Opm::RestartIO::ecl_file_kw_type * file_kw , fortio_type * fortio , ::Opm::RestartIO::inv_map_type * inv_map) {
  if (fortio == NULL)
    util_abort("%s: trying to load a keyword after the backing file has been detached.\n",__func__);

  if (file_kw->kw != NULL)
    ::Opm::RestartIO::ecl_file_kw_drop_kw( file_kw , inv_map );

  {
    fortio_fseek( fortio , file_kw->file_offset , SEEK_SET );
    file_kw->kw = Opm::RestartIO::ecl_kw_fread_alloc( fortio );
    ::Opm::RestartIO::ecl_file_kw_assert_kw( file_kw );
    ::Opm::RestartIO::inv_map_add_kw( inv_map , file_kw , file_kw->kw );
  }
}


/*
  Will return the ecl_kw instance of this file_kw; if it is not
  currently loaded the method will instantiate the ecl_kw instance
  from the @fortio input handle.

  After loading the keyword it will be kept in memory, so a possible
  subsequent lookup will be served from memory.

  The ecl_file layer maintains a pointer mapping between the
  ecl_kw_type pointers and their ecl_file_kw_type containers; this
  mapping needs the new_load return value from the
  ecl_file_kw_get_kw() function.
*/


::Opm::RestartIO::ecl_kw_type * ecl_file_kw_get_kw( ::Opm::RestartIO::ecl_file_kw_type * file_kw , fortio_type * fortio , ::Opm::RestartIO::inv_map_type * inv_map ) {
  if (file_kw->ref_count == 0)
    ::Opm::RestartIO::ecl_file_kw_load_kw( file_kw , fortio , inv_map);

  if(file_kw->kw)
    file_kw->ref_count++;

  return file_kw->kw;
}


int perm_vector_iget( const ::Opm::RestartIO::perm_vector_type * perm, int index) {
  if (index < perm->size)
    return perm->perm[index];
  else {
    util_abort("%s: invalid index:%d \n",__func__ , index );
    return -1;
  }
}




::Opm::RestartIO::ecl_file_kw_type * ecl_file_view_iget_file_kw( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , int global_index) {
  ::Opm::RestartIO::ecl_file_kw_type * file_kw = static_cast<::Opm::RestartIO::ecl_file_kw_type *>(::Opm::RestartIO::vector_iget( ecl_file_view->kw_list , global_index));
  return file_kw;
}


int ecl_file_view_get_global_index( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , const char * kw , int ith) {
  const ::Opm::RestartIO::int_vector_type * index_vector = static_cast<::Opm::RestartIO::int_vector_type *>(::Opm::RestartIO::hash_get(ecl_file_view->kw_index , kw));
  int global_index = ::Opm::RestartIO::int_vector_iget( index_vector , ith);
  return global_index;
}

/*****************************************************************/
/*                    Low level access functions                 */
/*****************************************************************/


bool hash_node_key_eq(const ::Opm::RestartIO::hash_node_type * node , uint32_t global_index , const char *key) {
  bool eq;
  eq = true;
  if (global_index != node->global_index)
    eq = false;
  else if (strcmp(node->key , key) != 0)
    eq = false;
  return eq;
}

::Opm::RestartIO::hash_node_type * hash_node_get_next(const ::Opm::RestartIO::hash_node_type * node) { 
  return node->next_node; 
}

::Opm::RestartIO::hash_node_type * hash_sll_get(const ::Opm::RestartIO::hash_sll_type *hash_sll, uint32_t global_index , const char *key) {
  ::Opm::RestartIO::hash_node_type * node = hash_sll->head;
  
  while ((node != NULL) && (!::Opm::RestartIO::hash_node_key_eq(node , global_index , key))) 
    node =  ::Opm::RestartIO::hash_node_get_next(node);
  
  return node;
}


/*
  This function looks up a hash_node from the hash. This is the common
  low-level function to get content from the hash. The function takes
  read-lock which is held during execution.

  Would strongly preferred that the hash_type * was const - but that is
  difficult due to locking requirements.
*/


static void * __hash_get_node_unlocked(const ::Opm::RestartIO::hash_type *__hash , const char *key, bool abort_on_error) {
  ::Opm::RestartIO::hash_type * hash = (::Opm::RestartIO::hash_type *) __hash;  /* The net effect is no change - but .... ?? */
  ::Opm::RestartIO::hash_node_type * node = NULL;
  {
    const uint32_t global_index = hash->hashf(key , strlen(key));
    const uint32_t table_index  = (global_index % hash->size);

    node = ::Opm::RestartIO::hash_sll_get(hash->table[table_index] , global_index , key);
    if (node == NULL && abort_on_error)
      util_abort("%s: tried to get from key:%s which does not exist - aborting \n",__func__ , key);

  }
  return node;
}


/*****************************************************************/
/*                          locking                              */
/*****************************************************************/
#ifdef HAVE_PTHREAD

static void __hash_rdlock(::Opm::RestartIO::hash_type * hash) {
  int lock_error = pthread_rwlock_tryrdlock( &hash->rwlock );
  if (lock_error != 0)
    util_abort("%s: did not get hash->read_lock - fix locking in calling scope\n",__func__);
}


static void __hash_wrlock(::Opm::RestartIO::hash_type * hash) {
  int lock_error = pthread_rwlock_trywrlock( &hash->rwlock );
  if (lock_error != 0)
    util_abort("%s: did not get hash->write_lock - fix locking in calling scope\n",__func__);
}


static void __hash_unlock( ::Opm::RestartIO::hash_type * hash) {
  pthread_rwlock_unlock( &hash->rwlock );
}


static void LOCK_DESTROY( ::Opm::RestartIO::lock_type * rwlock ) {
  pthread_rwlock_destroy( rwlock );
}

static void LOCK_INIT( ::Opm::RestartIO::lock_type * rwlock ) {
  pthread_rwlock_init( rwlock , NULL);
}

#else

static void __hash_rdlock(::Opm::RestartIO::hash_type * hash) {}
static void __hash_wrlock(::Opm::RestartIO::hash_type * hash) {}
static void __hash_unlock(::Opm::RestartIO::hash_type * hash) {}
static void LOCK_DESTROY(::Opm::RestartIO::lock_type * rwlock) {}
static void LOCK_INIT(::Opm::RestartIO::lock_type * rwlock) {}

#endif


/*
  This function looks up a hash_node from the hash. This is the common
  low-level function to get content from the hash. The function takes
  read-lock which is held during execution.

  Would strongly preferred that the hash_type * was const - but that is
  difficult due to locking requirements.
*/
static void * __hash_get_node(const ::Opm::RestartIO::hash_type *hash_in , const char *key, bool abort_on_error) {
  ::Opm::RestartIO::hash_node_type * node;
  ::Opm::RestartIO::hash_type * hash = (::Opm::RestartIO::hash_type *)hash_in;
  ::Opm::RestartIO::__hash_rdlock( hash );
  node = (::Opm::RestartIO::hash_node_type*)::Opm::RestartIO::__hash_get_node_unlocked(hash , key , abort_on_error);
  __hash_unlock( hash );
  return node;
}

/*****************************************************************/
/* The three functions below here are the only functions accessing the
   data field of the hash_node.  
*/

::Opm::RestartIO::node_data_type * hash_node_get_data(const ::Opm::RestartIO::hash_node_type * node) {
  return node->data;
}

void * node_data_get_ptr(const ::Opm::RestartIO::node_data_type * node_data) {
  return node_data->data;
}

void * hash_get(const ::Opm::RestartIO::hash_type *hash , const char *key) {
  ::Opm::RestartIO::hash_node_type * hash_node = (::Opm::RestartIO::hash_node_type*)__hash_get_node(hash , key , true);
  ::Opm::RestartIO::node_data_type * data_node = (::Opm::RestartIO::node_data_type*)hash_node_get_data( hash_node );
  return ::Opm::RestartIO::node_data_get_ptr( data_node );
}

bool hash_has_key(const ::Opm::RestartIO::hash_type *hash , const char *key) {
  if (::Opm::RestartIO::__hash_get_node(hash , key , false) == NULL)
    return false;
  else
    return true;
}


bool ecl_file_view_has_kw( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view, const char * kw) {
  return ::Opm::RestartIO::hash_has_key( ecl_file_view->kw_index , kw );
}

static void size_t_vector_realloc_data__(::Opm::RestartIO::size_t_vector_type * vector , int new_alloc_size) {
  if (new_alloc_size != vector->alloc_size) {
    if (vector->data_owner) {
      if (new_alloc_size > 0) {
        int i;
        vector->data = (size_t*)::Opm::RestartIO::util_realloc(vector->data , new_alloc_size * sizeof * vector->data );
        for (i=vector->alloc_size;  i < new_alloc_size; i++)
          vector->data[i] = vector->default_value;
      } else {
        if (vector->alloc_size > 0) {
          free(vector->data);
          vector->data = NULL;
        }
      }
      vector->alloc_size = new_alloc_size;
    } else
      util_abort("%s: tried to change the storage are for a shared data segment \n",__func__);
  }
}

/**
   Observe that this function will grow the vector if necessary. If
   index > size - i.e. leaving holes in the vector, these are
   explicitly set to the default value. If a reallocation is needed it
   is done in the realloc routine, otherwise it is done here.
*/

void size_t_vector_iset(::Opm::RestartIO::size_t_vector_type * vector , int index , size_t value) {
  ::Opm::RestartIO::size_t_vector_assert_writable( vector );
  {
    if (index < 0)
      util_abort("%s: Sorry - can NOT set negative indices. called with index:%d \n",__func__ , index);
    {
      if (vector->alloc_size <= index)
        ::Opm::RestartIO::size_t_vector_realloc_data__(vector , 2 * (index + 1));  /* Must have ( + 1) here to ensure we are not doing 2*0 */
      vector->data[index] = value;
      if (index >= vector->size) {
        int i;
        for (i=vector->size; i < index; i++)
          vector->data[i] = vector->default_value;
        vector->size = index + 1;
      }
    }
  }
}


bool ecl_type_is_equal(const ::Opm::RestartIO::ecl_data_type ecl_type1,
                       const ::Opm::RestartIO::ecl_data_type ecl_type2) {
  return (ecl_type1.type         == ecl_type2.type &&
          ecl_type1.element_size == ecl_type2.element_size);
}



static int size_t_vector_cmp_node(const void *_a, const void *_b) {
  ::Opm::RestartIO::sort_node_type a = *((::Opm::RestartIO::sort_node_type *) _a);
  ::Opm::RestartIO::sort_node_type b = *((::Opm::RestartIO::sort_node_type *) _b);

  if (a.value < b.value)
    return -1;

  if (a.value > b.value)
    return 1;

  return 0;
}

static int size_t_vector_rcmp_node(const void *a, const void *b) {
  return size_t_vector_cmp_node( b , a );
}


/*
  This constructor will *take ownership* of the input int* array; and
  call free( ) on it when the perm_vector is destroyed.
*/

::Opm::RestartIO::perm_vector_type * perm_vector_alloc( int * perm_input, int size ) {
  ::Opm::RestartIO::perm_vector_type * perm = (::Opm::RestartIO::perm_vector_type*)::Opm::RestartIO::util_malloc( sizeof * perm );
  UTIL_TYPE_ID_INIT( perm , PERM_VECTOR_TYPE_ID );
  perm->size = size;
  perm->perm = perm_input;
  return perm;
}


static ::Opm::RestartIO::perm_vector_type * size_t_vector_alloc_sort_perm__(const ::Opm::RestartIO::size_t_vector_type * vector, bool reverse) {
  int * perm = (int*)util_calloc( vector->size , sizeof * perm ); // The perm_vector return value will take ownership of this array.
  ::Opm::RestartIO::sort_node_type * sort_nodes = (::Opm::RestartIO::sort_node_type*)util_calloc( vector->size , sizeof * sort_nodes );
  int i;
  for (i=0; i < vector->size; i++) {
    sort_nodes[i].index = i;
    sort_nodes[i].value = vector->data[i];
  }
  if (reverse)
    qsort(sort_nodes , vector->size , sizeof * sort_nodes ,  size_t_vector_rcmp_node);
  else
    qsort(sort_nodes , vector->size , sizeof * sort_nodes ,  size_t_vector_cmp_node);

  for (i=0; i < vector->size; i++)
    perm[i] = sort_nodes[i].index;

  free( sort_nodes );
  return perm_vector_alloc( perm , vector->size );
}



::Opm::RestartIO::perm_vector_type * size_t_vector_alloc_sort_perm(const ::Opm::RestartIO::size_t_vector_type * vector) {
  return size_t_vector_alloc_sort_perm__( vector , false );
}

/*
  Should be reimplemented with util_sorted_contains_xxx().
*/

int size_t_vector_index_sorted(const ::Opm::RestartIO::size_t_vector_type * vector , size_t value) {
  if (vector->size) {

    if (value < vector->data[0])
      return -1;
    if (value == vector->data[ 0 ])
      return 0;

    {
      int last_index = vector->size - 1;
      if (value > vector->data[ last_index ])
        return -1;
      if (value == vector->data[ last_index])
        return last_index;
    }


    {
      int lower_index = 0;
      int upper_index = vector->size - 1;

      while (true) {
        if ((upper_index - lower_index) <= 1)
          /* Not found */
          return -1;

        {
          int center_index = (lower_index + upper_index) / 2;
          size_t center_value = vector->data[ center_index ];

          if (center_value == value)
            /* Found it */
            return center_index;
          else {
            if (center_value > value)
              upper_index = center_index;
            else
              lower_index = center_index;
          }
        }
      }
    }
  } else
    return -1;
}


static void size_t_vector_assert_index(const ::Opm::RestartIO::size_t_vector_type * vector , int index) {
  if ((index < 0) || (index >= vector->size))
    util_abort("%s: index:%d invalid. Valid interval: [0,%d>.\n",__func__ , index , vector->size);
}

static void size_t_vector_memmove(::Opm::RestartIO::size_t_vector_type * vector , int offset , int shift) {
  if ((shift + offset) < 0)
    util_abort("%s: offset:%d  left_shift:%d - invalid \n",__func__ , offset , -shift);

  if ((shift + vector->size > vector->alloc_size))
    size_t_vector_realloc_data__( vector , util_int_min( 2*vector->alloc_size , shift + vector->size ));

  {
    size_t move_size   = (vector->size - offset) * sizeof(size_t);
    size_t * target    = &vector->data[offset + shift];
    const size_t * src = &vector->data[offset];
    memmove( target , src , move_size );

    vector->size += shift;
  }
}



size_t size_t_vector_iget(const ::Opm::RestartIO::size_t_vector_type * vector , int index) {
  size_t_vector_assert_index(vector , index);
  return vector->data[index];
}


void size_t_vector_idel_block( ::Opm::RestartIO::size_t_vector_type * vector , int index , int block_size) {
  size_t_vector_assert_writable( vector );
  {
    if ((index >= 0) && (index < vector->size) && (block_size >= 0)) {
      if (index + block_size > vector->size)
        block_size = vector->size - index;

      index += block_size;
      size_t_vector_memmove( vector , index , -block_size );
    } else
      util_abort("%s: invalid input \n",__func__);
  }
}


/**
   Removes element @index from the vector, shifting all elements to
   the right of @index one element to the left and shrinking the total
   vector. The return value is the value which is removed.
*/


size_t size_t_vector_idel( ::Opm::RestartIO::size_t_vector_type * vector , int index) {
  size_t del_value = size_t_vector_iget( vector , index );
  ::Opm::RestartIO::size_t_vector_idel_block( vector , index , 1 );
  return del_value;
}




void * vector_iget(const vector_type * vector, int index) {
  if ((index >= 0) && (index < vector->size)) {
    const ::Opm::RestartIO::node_data_type * node = vector->data[index];
    return node_data_get_ptr( node );
  } else {
    util_abort("%s: Invalid index:%d  Valid range: [0,%d> \n",__func__ , index , vector->size);
    return NULL;
  }
}


/**
   This function does NOT call the destructor on the data. That means
   that calling scope is responsible for freeing the data; used by the
   vector_pop function.
*/
void node_data_free_container(::Opm::RestartIO::node_data_type * node_data) {
  free(node_data);
}


void node_data_free(::Opm::RestartIO::node_data_type * node_data) {
  if (node_data->del != NULL)
    node_data->del( (void *) node_data->data );
  
  ::Opm::RestartIO::node_data_free_container( node_data );
}


/**
   This vector frees all the storage of the vector, including all the
   nodes which have been installed with a destructor.
*/

void vector_clear(::Opm::RestartIO::vector_type * vector) {
  int i;
  for (i = 0; i < vector->size; i++) {
    ::Opm::RestartIO::node_data_free(vector->data[i]);  /* User specific destructors are called here. */
    vector->data[i] = NULL;           /* This is essential to protect against unwaranted calls to destructors when data is reused. */
  }
  vector->size = 0;
}


void vector_free(::Opm::RestartIO::vector_type * vector) {
  ::Opm::RestartIO::vector_clear( vector );
  free( vector->data );
  free( vector );
}

void hash_node_free(::Opm::RestartIO::hash_node_type * node) {
  free(node->key);
  ::Opm::RestartIO::node_data_free( node->data );
  free(node);
}

void hash_sll_free(::Opm::RestartIO::hash_sll_type *hash_sll) {
  if (hash_sll->head != NULL) {
    ::Opm::RestartIO::hash_node_type *node , *next_node;
    node = hash_sll->head;
    while (node != NULL) {
      next_node = ::Opm::RestartIO::hash_node_get_next(node);
      ::Opm::RestartIO::hash_node_free(node);
      node = next_node;
    }
  }
  free( hash_sll );
}

void hash_free(::Opm::RestartIO::hash_type *hash) {
  uint32_t i;
  for (i=0; i < hash->size; i++)
    ::Opm::RestartIO::hash_sll_free(hash->table[i]);
  free(hash->table);
  LOCK_DESTROY( &hash->rwlock );
  free(hash);
}

/**
    Frees all the memory contained by the stringlist.
*/
void stringlist_clear(::Opm::RestartIO::stringlist_type * stringlist) {
  ::Opm::RestartIO::vector_clear( stringlist->strings );
}

void stringlist_free(::Opm::RestartIO::stringlist_type * stringlist) {
  ::Opm::RestartIO::stringlist_clear(stringlist);
  ::Opm::RestartIO::vector_free(stringlist->strings);
  free(stringlist);
}


void ecl_file_view_free( ::Opm::RestartIO::ecl_file_view_type * ecl_file_view ) {
  ::Opm::RestartIO::vector_free( ecl_file_view->child_list );
  ::Opm::RestartIO::hash_free( ecl_file_view->kw_index );
  ::Opm::RestartIO::stringlist_free( ecl_file_view->distinct_kw );
  ::Opm::RestartIO::vector_free( ecl_file_view->kw_list );
  free( ecl_file_view );
}

void size_t_vector_free_container(::Opm::RestartIO::size_t_vector_type * vector) {
  free( vector );
}

void size_t_vector_free(::Opm::RestartIO::size_t_vector_type * vector) {
  if (vector->data_owner)
    ::Opm::RestartIO::util_safe_free( vector->data );
    ::Opm::RestartIO::size_t_vector_free_container( vector );
}

void inv_map_free( ::Opm::RestartIO::inv_map_type * map ) {
  ::Opm::RestartIO::size_t_vector_free( map->file_kw_ptr );
  ::Opm::RestartIO::size_t_vector_free( map->ecl_kw_ptr );
  free( map );
}

/**
   The ecl_file_close() function will close the fortio instance and
   free all the data created by the ecl_file instance; this includes
   the ecl_kw instances which have been loaded on demand.
*/

void ecl_file_close(::Opm::RestartIO::ecl_file_type * ecl_file) {
  if (ecl_file->fortio != NULL)
    fortio_fclose( ecl_file->fortio  );

  if (ecl_file->global_view)
    ::Opm::RestartIO::ecl_file_view_free( ecl_file->global_view );

  ::Opm::RestartIO::inv_map_free( ecl_file->inv_view );
  ::Opm::RestartIO::vector_free( ecl_file->map_stack );
  free( ecl_file );
}

int int_vector_size(const ::Opm::RestartIO::int_vector_type * vector) {
  return vector->size;
}

::Opm::RestartIO::ecl_kw_type * ecl_file_view_iget_kw( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , int index) {
  ::Opm::RestartIO::ecl_file_kw_type * file_kw = ::Opm::RestartIO::ecl_file_view_iget_file_kw( ecl_file_view , index );
  return ::Opm::RestartIO::ecl_file_view_get_kw(ecl_file_view, file_kw);
}


static bool ecl_kw_data_equal__( const ::Opm::RestartIO::ecl_kw_type * ecl_kw , const void * data , int cmp_elements) {
  int cmp = memcmp( ecl_kw->data , data , cmp_elements * ::Opm::RestartIO::ecl_kw_get_sizeof_ctype(ecl_kw));
  if (cmp == 0)
    return true;
  else
    return false;
}

/**
   Observe that the comparison is done with memcmp() -
   i.e. "reasonably good" numerical agreement is *not* enough.
*/

bool ecl_kw_data_equal( const ::Opm::RestartIO::ecl_kw_type * ecl_kw , const void * data) {
  return ::Opm::RestartIO::ecl_kw_data_equal__( ecl_kw , data , ecl_kw->size);
}


int ecl_file_view_find_kw_value( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , const char * kw , const void * value) {
  int global_index = -1;
  if ( ::Opm::RestartIO::ecl_file_view_has_kw( ecl_file_view , kw)) {
    const ::Opm::RestartIO::int_vector_type * index_list = static_cast<::Opm::RestartIO::int_vector_type *> (hash_get( ecl_file_view->kw_index , kw ));
    int index = 0;
    while (index < ::Opm::RestartIO::int_vector_size( index_list )) {
      const ::Opm::RestartIO::ecl_kw_type * ecl_kw = ::Opm::RestartIO::ecl_file_view_iget_kw( ecl_file_view , ::Opm::RestartIO::int_vector_iget( index_list , index ));
      if (::Opm::RestartIO::ecl_kw_data_equal( ecl_kw , value )) {
        global_index = ::Opm::RestartIO::int_vector_iget( index_list , index );
        break;
      }
      index++;
    }
  }
  return global_index;
}

const void * vector_iget_const(const ::Opm::RestartIO::vector_type * vector, int index) {
  if ((index >= 0) && (index < vector->size)) {
    const ::Opm::RestartIO::node_data_type * node = vector->data[index];
    return ::Opm::RestartIO::node_data_get_ptr( node );
  } else {
    util_abort("%s: Invalid index:%d  Valid range: [0,%d> \n",__func__ , index , vector->size);
    return NULL;
  }
}

const char * ecl_file_kw_get_header( const ::Opm::RestartIO::ecl_file_kw_type * file_kw ) {
  return file_kw->header;
}

const int * int_vector_get_const_ptr(const ::Opm::RestartIO::int_vector_type * vector) {
  return vector->data;
}

int ecl_file_view_iget_occurence( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , int global_index) {
  const ::Opm::RestartIO::ecl_file_kw_type * file_kw = static_cast<const ::Opm::RestartIO::ecl_file_kw_type *>(::Opm::RestartIO::vector_iget_const( ecl_file_view->kw_list , global_index));
  const char * header              = ::Opm::RestartIO::ecl_file_kw_get_header( file_kw );
  const int_vector_type * index_vector = static_cast<const int_vector_type *> (::Opm::RestartIO::hash_get( ecl_file_view->kw_index , header ));
  const int * index_data = ::Opm::RestartIO::int_vector_get_const_ptr( index_vector );

  int occurence = -1;
  {
    /* Manual reverse lookup. */
    int i;
    for (i=0; i < int_vector_size( index_vector ); i++)
      if (index_data[i] == global_index)
        occurence = i;
  }
  if (occurence < 0)
    util_abort("%s: internal error ... \n" , __func__);

  return occurence;
}

int ecl_file_view_get_num_named_kw(const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , const char * kw) {
  if (::Opm::RestartIO::hash_has_key(ecl_file_view->kw_index , kw)) {
    const ::Opm::RestartIO::int_vector_type * index_vector = static_cast<const ::Opm::RestartIO::int_vector_type *> (::Opm::RestartIO::hash_get(ecl_file_view->kw_index , kw));
    return ::Opm::RestartIO::int_vector_size( index_vector );
  } else
    return 0;
}

static ::Opm::RestartIO::hash_sll_type * hash_sll_alloc( void ) {
  ::Opm::RestartIO::hash_sll_type * hash_sll = (::Opm::RestartIO::hash_sll_type*) ::Opm::RestartIO::util_malloc(sizeof * hash_sll );
  hash_sll->length = 0;
  hash_sll->head   = NULL;
  return hash_sll;
}


::Opm::RestartIO::hash_sll_type ** hash_sll_alloc_table(int size) {
  ::Opm::RestartIO::hash_sll_type ** table = (::Opm::RestartIO::hash_sll_type**) ::Opm::RestartIO::util_malloc(size * sizeof * table );
  int i;
  for (i=0; i<size; i++)
    table[i] = ::Opm::RestartIO::hash_sll_alloc();
  return table;
}


static ::Opm::RestartIO::hash_type * __hash_alloc(int size, double resize_fill , ::Opm::RestartIO::hashf_type *hashf) {
  ::Opm::RestartIO::hash_type* hash;
  hash = (::Opm::RestartIO::hash_type*)::Opm::RestartIO::util_malloc(sizeof *hash );
  UTIL_TYPE_ID_INIT(hash , HASH_TYPE_ID);
  hash->size      = size;
  hash->hashf     = hashf;
  hash->table     = ::Opm::RestartIO::hash_sll_alloc_table(hash->size);
  hash->elements  = 0;
  hash->resize_fill  = resize_fill;
  ::Opm::RestartIO::LOCK_INIT( &hash->rwlock );

  return hash;
}

/**
   This is **THE** hash function - which actually does the hashing.
*/

static uint32_t hash_index(const char *key, size_t len) {
  uint32_t hash = 0;
  size_t i;

  for (i=0; i < len; i++) {
    hash += key[i];
    hash += (hash << 10);
    hash ^= (hash >>  6);
  }
  hash += (hash << 3);
  hash ^= (hash >> 11);
  hash += (hash << 15);
  return hash;
}


::Opm::RestartIO::hash_type * hash_alloc() {
  return ::Opm::RestartIO::__hash_alloc(HASH_DEFAULT_SIZE , 0.50 , ::Opm::RestartIO::hash_index);
}


static void vector_resize__(::Opm::RestartIO::vector_type * vector, int new_alloc_size) {
  int i;
  if (new_alloc_size < vector->alloc_size) {
    /* The vector is shrinking. */
    for (i=new_alloc_size; i < vector->alloc_size; i++)
      ::Opm::RestartIO::node_data_free( vector->data[i] );
  }

  vector->data = (::Opm::RestartIO::node_data_type**)::Opm::RestartIO::util_realloc( vector->data , new_alloc_size * sizeof * vector->data );
  for (i = vector->alloc_size; i < new_alloc_size; i++)
    vector->data[i] = NULL; /* Initialising new nodes to NULL */

  vector->alloc_size = new_alloc_size;
}



::Opm::RestartIO::vector_type * vector_alloc_new() {
  ::Opm::RestartIO::vector_type * vector = (::Opm::RestartIO::vector_type*)::Opm::RestartIO::util_malloc( sizeof * vector );
  UTIL_TYPE_ID_INIT(vector , VECTOR_TYPE_ID);
  vector->size       = 0;
  vector->alloc_size = 0;
  vector->data       = NULL;
  ::Opm::RestartIO::vector_resize__(vector , VECTOR_DEFAULT_SIZE);
  return vector;
}

static ::Opm::RestartIO::stringlist_type * stringlist_alloc_empty( bool alloc_vector ) {
  ::Opm::RestartIO::stringlist_type * stringlist = (::Opm::RestartIO::stringlist_type*)::Opm::RestartIO::util_malloc(sizeof * stringlist );
  UTIL_TYPE_ID_INIT( stringlist , STRINGLIST_TYPE_ID);

  if (alloc_vector)
    stringlist->strings = ::Opm::RestartIO::vector_alloc_new();
  else
    stringlist->strings = NULL;

  return stringlist;
}

stringlist_type * stringlist_alloc_new() {
  return ::Opm::RestartIO::stringlist_alloc_empty( true );
}

::Opm::RestartIO::ecl_file_view_type * ecl_file_view_alloc( fortio_type * fortio , int * flags , inv_map_type * inv_map , bool owner ) {
  ::Opm::RestartIO::ecl_file_view_type * ecl_file_view  = static_cast<::Opm::RestartIO::ecl_file_view_type *>(::Opm::RestartIO::util_malloc( sizeof * ecl_file_view ));
  ecl_file_view->kw_list              = ::Opm::RestartIO::vector_alloc_new();
  ecl_file_view->kw_index             = ::Opm::RestartIO::hash_alloc();
  ecl_file_view->distinct_kw          = ::Opm::RestartIO::stringlist_alloc_new();
  ecl_file_view->child_list           = ::Opm::RestartIO::vector_alloc_new();
  ecl_file_view->owner                = owner;
  ecl_file_view->fortio               = fortio;
  ecl_file_view->inv_map              = inv_map;
  ecl_file_view->flags                = flags;
  return ecl_file_view;
}

/** 
   If the node has a copy constructor, the data is copied immediately
   - so the node contains a copy from object creation time.
*/

static ::Opm::RestartIO::node_data_type * node_data_alloc__(const void * data , node_ctype ctype , int buffer_size , ::Opm::RestartIO::copyc_ftype * copyc, ::Opm::RestartIO::free_ftype * del) {
  ::Opm::RestartIO::node_data_type * node = (::Opm::RestartIO::node_data_type*)::Opm::RestartIO::util_malloc(sizeof * node );
  node->ctype           = ctype;
  node->copyc           = copyc;
  node->del             = del;
  node->buffer_size     = buffer_size;  /* If buffer_size >0 copyc MUST be NULL */

  if (node->copyc != NULL)
    node->data = node->copyc( data );
  else
    node->data = (void *) data;
  
  return node;
}

::Opm::RestartIO::node_data_type * node_data_alloc_ptr(const void * data  , ::Opm::RestartIO::copyc_ftype * copyc, ::Opm::RestartIO::free_ftype * del) {
  return node_data_alloc__( data , ::Opm::RestartIO::CTYPE_VOID_POINTER , 0 , copyc , del);
}


//jal - declare function before definition due to mutual calls for two functions
static int vector_append_node(::Opm::RestartIO::vector_type * vector , ::Opm::RestartIO::node_data_type * node);
  
/**
    If the index is beyond the length of the vector the hole in the
    vector will be filled with NULL nodes.
*/

static void vector_iset__(::Opm::RestartIO::vector_type * vector , int index , ::Opm::RestartIO::node_data_type * node) {
  if (index > vector->size)
    ::Opm::RestartIO::vector_grow_NULL( vector , index );

  if (index == vector->size)
    ::Opm::RestartIO::vector_append_node( vector , node );
  else {
    if (vector->data[index] != NULL)
      ::Opm::RestartIO::node_data_free( vector->data[index] );

    vector->data[index] = node;
  }
}

/**
   This is the low-level append node function which actually "does
   it", the node has been allocated in one of the front-end
   functions. The return value is the index of the node (which can
   subsequently be used in a vector_iget() call)).
*/
static int vector_append_node(::Opm::RestartIO::vector_type * vector , ::Opm::RestartIO::node_data_type * node) {
  if (vector->size == vector->alloc_size)
    ::Opm::RestartIO::vector_resize__(vector , 2*(vector->alloc_size + 1));

  vector->size++;
  ::Opm::RestartIO::vector_iset__(vector , vector->size - 1 , node);
  return vector->size - 1;
}


/**
   Append a user-pointer which comes without either copy constructor
   or destructor, this implies that the calling scope has FULL
   responsabilty for the storage of the data added to the vector.
*/


int vector_append_ref(::Opm::RestartIO::vector_type * vector , const void * data) {
  ::Opm::RestartIO::node_data_type * node = node_data_alloc_ptr( data, NULL , NULL);
  return ::Opm::RestartIO::vector_append_node(vector , node);
}

void vector_grow_NULL( ::Opm::RestartIO::vector_type * vector , int new_size ) {
  int i;
  for (i = vector->size; i < new_size; i++)
    ::Opm::RestartIO::vector_append_ref( vector , NULL );
}


int vector_append_owned_ref(::Opm::RestartIO::vector_type * vector , const void * data , ::Opm::RestartIO::free_ftype * del) {
  ::Opm::RestartIO::node_data_type * node = node_data_alloc_ptr( data, NULL , del);
  return ::Opm::RestartIO::vector_append_node(vector , node);
}



void ecl_file_view_add_kw( ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , ::Opm::RestartIO::ecl_file_kw_type * file_kw) {
  if (ecl_file_view->owner)
    ::Opm::RestartIO::vector_append_owned_ref( ecl_file_view->kw_list , file_kw , ecl_file_kw_free__ );
  else
    ::Opm::RestartIO::vector_append_ref( ecl_file_view->kw_list , file_kw);
}

int vector_get_size( const ::Opm::RestartIO::vector_type * vector) {
  return vector->size;
}

::Opm::RestartIO::ecl_file_view_type * ecl_file_view_alloc_blockview2(const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , const char * start_kw, const char * end_kw, int occurence) {
  if ((start_kw != NULL) && ::Opm::RestartIO::ecl_file_view_get_num_named_kw( ecl_file_view , start_kw ) <= occurence)
    return NULL;


  ::Opm::RestartIO::ecl_file_view_type * block_map = ::Opm::RestartIO::ecl_file_view_alloc( ecl_file_view->fortio , ecl_file_view->flags , ecl_file_view->inv_map , false);
  int kw_index = 0;
  if (start_kw)
    kw_index = ::Opm::RestartIO::ecl_file_view_get_global_index( ecl_file_view , start_kw , occurence );

  {
    ::Opm::RestartIO::ecl_file_kw_type * file_kw = static_cast<::Opm::RestartIO::ecl_file_kw_type *> (::Opm::RestartIO::vector_iget( ecl_file_view->kw_list , kw_index ));
    while (true) {
      ::Opm::RestartIO::ecl_file_view_add_kw( block_map , file_kw );

      kw_index++;
      if (kw_index == ::Opm::RestartIO::vector_get_size( ecl_file_view->kw_list ))
        break;
      else {
        if (end_kw) {
          file_kw = static_cast<::Opm::RestartIO::ecl_file_kw_type *> (::Opm::RestartIO::vector_iget(ecl_file_view->kw_list , kw_index));
          if (strcmp( end_kw , ::Opm::RestartIO::ecl_file_kw_get_header( file_kw )) == 0)
            break;
        }
      }
    }
  }
  ::Opm::RestartIO::ecl_file_view_make_index( block_map );
  return block_map;
}

/**
   Will return NULL if the block which is asked for is not present.
*/
::Opm::RestartIO::ecl_file_view_type * ecl_file_view_alloc_blockview(const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , const char * header, int occurence) {
  return ::Opm::RestartIO::ecl_file_view_alloc_blockview2( ecl_file_view , header , header , occurence );
}

//jal - jeg har laget en foreløpig fiks på mangel på rutinene: timegm og _mkgmtime

static time_t timegm(struct tm * a_tm)
{
    time_t ltime = std::mktime(a_tm);
    struct tm * tm_val;
    tm_val = std::gmtime(&ltime);
   //orig std::gmtime_s(&tm_val, &ltime);
    int offset = (tm_val->tm_hour - a_tm->tm_hour);
    if (offset > 12)
    {
        offset = 24 - offset;
    }
    time_t utc = std::mktime(a_tm) - offset * 3600;
    return utc;
}	

static time_t _mkgmtime(const struct tm *tm) 
{
    // Month-to-day offset for non-leap-years.
    static const int month_day[12] =
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

    // Most of the calculation is easy; leap years are the main difficulty.
    int month = tm->tm_mon % 12;
    int year = tm->tm_year + tm->tm_mon / 12;
    if (month < 0) {   // Negative values % 12 are still negative.
        month += 12;
        --year;
    }

    // This is the number of Februaries since 1900.
    const int year_for_leap = (month > 1) ? year + 1 : year;

    time_t rt = tm->tm_sec                             // Seconds
        + 60 * (tm->tm_min                          // Minute = 60 seconds
        + 60 * (tm->tm_hour                         // Hour = 60 minutes
        + 24 * (month_day[month] + tm->tm_mday - 1  // Day = 24 hours
        + 365 * (year - 70)                         // Year = 365 days
        + (year_for_leap - 69) / 4                  // Every 4 years is     leap...
        - (year_for_leap - 1) / 100                 // Except centuries...
        + (year_for_leap + 299) / 400)));           // Except 400s.
    return rt < 0 ? -1 : rt;
}


static bool util_make_datetime_utc__(int sec, int min, int hour , int mday , int month , int year, bool force_set, time_t * t) {
  bool valid = false;
  struct tm ts;
  ts.tm_sec    = sec;
  ts.tm_min    = min;
  ts.tm_hour   = hour;
  ts.tm_mday   = mday;
  ts.tm_mon    = month - 1;
  ts.tm_year   = year - 1900;
  ts.tm_isdst  = -1;    /* Negative value means mktime tries to determine automagically whether Daylight Saving Time is in effect. */
  {

#ifdef HAVE_TIMEGM
    time_t work_t = ::Opm::RestartIO::timegm( &ts );
#else
    time_t work_t = ::Opm::RestartIO::_mkgmtime( &ts );
#endif

    if ((ts.tm_sec  == sec) &&
        (ts.tm_min  == min) &&
        (ts.tm_hour == hour) &&
        (ts.tm_mday == mday) &&
        (ts.tm_mon == (month - 1)) &&
        (ts.tm_year == (year - 1900)))
      valid = true;

    if (t) {
      if (valid || force_set)
        *t = work_t;
    }
  }
  return valid;
}

time_t util_make_datetime_utc(int sec, int min, int hour , int mday , int month , int year) {
  time_t t;
  ::Opm::RestartIO::util_make_datetime_utc__( sec,min,hour,mday,month,year,true, &t);
  return t;
}

time_t util_make_date_utc(int mday , int month , int year) {
  return ::Opm::RestartIO::util_make_datetime_utc(0 , 0 , 0 , mday , month , year);
}

time_t ecl_util_make_date__(int mday , int month , int year, int * __year_offset) {
time_t date;

#ifdef ERT_TIME_T_64BIT_ACCEPT_PRE1970
  *__year_offset = 0;
  date = ::Opm::RestartIO::util_make_date_utc(mday , month , year);
#else
  static bool offset_initialized = false;
  static int  year_offset = 0;

  if (!offset_initialized) {
    if (year < 1970) {
      year_offset = 2000 - year;
      fprintf(stderr,"Warning: all year values will be shifted %d years forward. \n", year_offset);
    }
    offset_initialized = true;
  }
  *__year_offset = year_offset;
  date = ::Opm::RestartIO::util_make_date_utc(mday , month , year + year_offset);
#endif

  return date;
}


time_t ecl_util_make_date(int mday , int month , int year) {
  int year_offset;
  return ::Opm::RestartIO::ecl_util_make_date__( mday , month , year , &year_offset);
}

static time_t rsthead_date( int day , int month , int year) {
  return ::Opm::RestartIO::ecl_util_make_date( day , month, year );
}

time_t ecl_rsthead_date( const ::Opm::RestartIO::ecl_kw_type * intehead_kw ) {
  /*return ::Opm::RestartIO::rsthead_date(  ::Opm::RestartIO::ecl_kw_iget_int( intehead_kw , INTEHEAD_DAY_INDEX)   ,
                        ::Opm::RestartIO::ecl_kw_iget_int( intehead_kw , INTEHEAD_MONTH_INDEX) ,
                        ::Opm::RestartIO::ecl_kw_iget_int( intehead_kw , INTEHEAD_YEAR_INDEX)  ); 
                        */
  return ::Opm::RestartIO::rsthead_date(  ::Opm::RestartIO::ecl_kw_iget_type<int>( intehead_kw , ECL_INT_TYPE, INTEHEAD_DAY_INDEX)   ,
                        ::Opm::RestartIO::ecl_kw_iget_type<int>( intehead_kw , ECL_INT_TYPE, INTEHEAD_MONTH_INDEX) ,
                        ::Opm::RestartIO::ecl_kw_iget_type<int>( intehead_kw , ECL_INT_TYPE, INTEHEAD_YEAR_INDEX)  ); 
}


time_t ecl_file_view_iget_restart_sim_date(const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , int seqnum_index) {
  time_t sim_time = -1;
  ::Opm::RestartIO::ecl_file_view_type * seqnum_map = ::Opm::RestartIO::ecl_file_view_alloc_blockview( ecl_file_view , SEQNUM_KW , seqnum_index);

  if (seqnum_map != NULL) {
    ::Opm::RestartIO::ecl_kw_type * intehead_kw = ::Opm::RestartIO::ecl_file_view_iget_named_kw( seqnum_map , INTEHEAD_KW , 0);
    sim_time = ::Opm::RestartIO::ecl_rsthead_date( intehead_kw );
    ::Opm::RestartIO::ecl_file_view_free( seqnum_map );
  }

  return sim_time;
}


/**
   This function will scan through the ecl_file looking for INTEHEAD
   headers corresponding to sim_time. If sim_time is found the
   function will return the INTEHEAD occurence number, i.e. for a
   unified restart file like:

   INTEHEAD  /  01.01.2000
   ...
   PRESSURE
   SWAT
   ...
   INTEHEAD  /  01.03.2000
   ...
   PRESSURE
   SWAT
   ...
   INTEHEAD  /  01.05.2000
   ...
   PRESSURE
   SWAT
   ....

   The function call:

   ecl_file_get_restart_index( restart_file , (time_t) "01.03.2000")

   will return 1. Observe that this will in general NOT agree with the
   DATES step number.

   If the sim_time can not be found the function will return -1, that
   includes the case when the file in question is not a restart file
   at all, and no INTEHEAD headers can be found.

   Observe that the function requires on-the-second-equality; which is
   of course quite strict.

   Each report step only has one occurence of SEQNUM, but one INTEHEAD
   for each LGR; i.e. one should call iselect_rstblock() prior to
   calling this function.
*/


bool ecl_file_view_has_sim_time( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , time_t sim_time) {
  int num_INTEHEAD = ::Opm::RestartIO::ecl_file_view_get_num_named_kw( ecl_file_view , INTEHEAD_KW );
  if (num_INTEHEAD == 0)
    return false;       /* We have no INTEHEAD headers - probably not a restart file at all. */
  else {
    int intehead_index = 0;
    while (true) {
      time_t itime = ::Opm::RestartIO::ecl_file_view_iget_restart_sim_date( ecl_file_view , intehead_index );

      if (itime == sim_time) /* Perfect hit. */
        return true;

      if (itime > sim_time)  /* We have gone past the target_time - i.e. we do not have it. */
        return false;

      intehead_index++;
      if (intehead_index == num_INTEHEAD)  /* We have iterated through the whole thing without finding sim_time. */
        return false;
    }
  }
}


int ecl_file_view_seqnum_index_from_sim_time( ::Opm::RestartIO::ecl_file_view_type * parent_map , time_t sim_time) {
    int num_seqnum = ::Opm::RestartIO::ecl_file_view_get_num_named_kw( parent_map , SEQNUM_KW );
  ::Opm::RestartIO::ecl_file_view_type * seqnum_map;

  for (int s_idx = 0; s_idx < num_seqnum; s_idx++) {
    seqnum_map = ::Opm::RestartIO::ecl_file_view_alloc_blockview( parent_map , SEQNUM_KW , s_idx );

    if (!seqnum_map)
      continue;

    bool sim = ::Opm::RestartIO::ecl_file_view_has_sim_time( seqnum_map , sim_time);
    ::Opm::RestartIO::ecl_file_view_free( seqnum_map );
    if (sim)
      return s_idx;
  }
  return -1;
}

double ecl_file_view_iget_restart_sim_days(const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , int seqnum_index) {
  double sim_days = 0;
  ::Opm::RestartIO::ecl_file_view_type * seqnum_map = ::Opm::RestartIO::ecl_file_view_alloc_blockview( ecl_file_view , SEQNUM_KW , seqnum_index);

  if (seqnum_map != NULL) {
    ::Opm::RestartIO::ecl_kw_type * doubhead_kw = ::Opm::RestartIO::ecl_file_view_iget_named_kw( seqnum_map , DOUBHEAD_KW , 0);
    sim_days = ::Opm::RestartIO::ecl_kw_iget_type<double>( doubhead_kw , ECL_DOUBLE_TYPE, DOUBHEAD_DAYS_INDEX);
    ::Opm::RestartIO::ecl_file_view_free( seqnum_map );
  }

  return sim_days;
}


/*
  If an epsilon value is identically equal to zero that comparison
  will be ignored.
*/

bool util_double_approx_equal__( double d1 , double d2, double rel_eps, double abs_eps) {
  if ((fabs(d1) + fabs(d2)) == 0)
    return true;
  else {
    double diff = fabs(d1 - d2);
    if ((abs_eps > 0) && (diff > abs_eps))
      return false;
    {
      double sum  = fabs(d1) + fabs(d2);
      double rel_diff = diff / sum;

      if ((rel_eps > 0) && (rel_diff > rel_eps))
        return false;
    }
    return true;
  }
}


bool util_double_approx_equal( double d1 , double d2) {
  double epsilon = 1e-6;
  return ::Opm::RestartIO::util_double_approx_equal__( d1 , d2 , epsilon , 0.0 );
}


bool ecl_file_view_has_sim_days( const ::Opm::RestartIO::ecl_file_view_type * ecl_file_view , double sim_days) {
  int num_DOUBHEAD = ::Opm::RestartIO::ecl_file_view_get_num_named_kw( ecl_file_view , DOUBHEAD_KW );
  if (num_DOUBHEAD == 0)
    return false;       /* We have no DOUBHEAD headers - probably not a restart file at all. */
  else {
    int doubhead_index = 0;
    while (true) {
      double file_sim_days  = ::Opm::RestartIO::ecl_file_view_iget_restart_sim_days( ecl_file_view , doubhead_index );

      if (::Opm::RestartIO::util_double_approx_equal(sim_days, file_sim_days)) /* Perfect hit. */
        return true;

      if (file_sim_days > sim_days)  /* We have gone past the target_time - i.e. we do not have it. */
        return false;

      doubhead_index++;
      if (doubhead_index == num_DOUBHEAD)  /* We have iterated through the whole thing without finding sim_time. */
        return false;
    }
  }
}


int ecl_file_view_seqnum_index_from_sim_days( ::Opm::RestartIO::ecl_file_view_type * file_view , double sim_days) {
  int num_seqnum = ::Opm::RestartIO::ecl_file_view_get_num_named_kw( file_view , SEQNUM_KW );
  int seqnum_index = 0;
  ecl_file_view_type * seqnum_map;

  while (true) {
    seqnum_map = ::Opm::RestartIO::ecl_file_view_alloc_blockview( file_view , SEQNUM_KW , seqnum_index);

    if (seqnum_map != NULL) {
      if (::Opm::RestartIO::ecl_file_view_has_sim_days( seqnum_map , sim_days)) {
        ::Opm::RestartIO::ecl_file_view_free( seqnum_map );
        return seqnum_index;
      } else {
        ::Opm::RestartIO::ecl_file_view_free( seqnum_map );
        seqnum_index++;
      }
    }

    if (num_seqnum == seqnum_index)
      return -1;
  }
}


void ecl_file_view_free__( void * arg ) {
  ::Opm::RestartIO::ecl_file_view_type * ecl_file_view = ( ::Opm::RestartIO::ecl_file_view_type * ) arg;
  ::Opm::RestartIO::ecl_file_view_free( ecl_file_view );
}

::Opm::RestartIO::ecl_file_view_type * ecl_file_view_add_blockview(const ::Opm::RestartIO::ecl_file_view_type * file_view , const char * header, int occurence) {
  ::Opm::RestartIO::ecl_file_view_type * child  = ::Opm::RestartIO::ecl_file_view_alloc_blockview2(file_view, header, header, occurence);

  if (child)
    ::Opm::RestartIO::vector_append_owned_ref( file_view->child_list , child , ::Opm::RestartIO::ecl_file_view_free__ );

  return child;
}

/*
  Will mulitplex on the four input arguments.
*/
::Opm::RestartIO::ecl_file_view_type * ecl_file_view_add_restart_view( ::Opm::RestartIO::ecl_file_view_type * file_view , int input_index, int report_step , time_t sim_time, double sim_days) {
  ::Opm::RestartIO::ecl_file_view_type * child = NULL;
  int seqnum_index = -1;

  if (input_index >= 0)
    seqnum_index = input_index;
  else if (report_step >= 0) {
    int global_index = ::Opm::RestartIO::ecl_file_view_find_kw_value( file_view , SEQNUM_KW , &report_step);
    if ( global_index >= 0)
      seqnum_index = ::Opm::RestartIO::ecl_file_view_iget_occurence( file_view , global_index );
  } else if (sim_time != -1)
    seqnum_index = ::Opm::RestartIO::ecl_file_view_seqnum_index_from_sim_time( file_view , sim_time );
  else if (sim_days >= 0)
    seqnum_index = ::Opm::RestartIO::ecl_file_view_seqnum_index_from_sim_days( file_view , sim_days );


  if (seqnum_index >= 0)
    child = ::Opm::RestartIO::ecl_file_view_add_blockview( file_view , SEQNUM_KW , seqnum_index );

  return child;
}



::Opm::RestartIO::ecl_file_view_type * ecl_file_get_restart_view( ::Opm::RestartIO::ecl_file_type * ecl_file , int input_index, int report_step , time_t sim_time, double sim_days) {
  ::Opm::RestartIO::ecl_file_view_type * view = ::Opm::RestartIO::ecl_file_view_add_restart_view( ecl_file->global_view , input_index , report_step , sim_time , sim_days);
  return view;
}

::Opm::RestartIO::ecl_file_view_type * ecl_file_get_global_view( ::Opm::RestartIO::ecl_file_type * ecl_file ) {
  return ecl_file->global_view;
}

int ecl_kw_get_size(const ::Opm::RestartIO::ecl_kw_type * ecl_kw) {
  return ecl_kw->size;
}


void size_t_vector_set_read_only( ::Opm::RestartIO::size_t_vector_type * vector , bool read_only) {
  vector->read_only = read_only;
}

static ::Opm::RestartIO::size_t_vector_type * size_t_vector_alloc__(int init_size , size_t default_value, size_t * data, int alloc_size , bool data_owner ) {
  ::Opm::RestartIO::size_t_vector_type * vector = (::Opm::RestartIO::size_t_vector_type*)::Opm::RestartIO::util_malloc( sizeof * vector );
  UTIL_TYPE_ID_INIT( vector , TYPE_VECTOR_ID);
  vector->default_value       = default_value;

  /**
     Not all combinations of (data, alloc_size, data_owner) are valid:

     1. Creating a new vector instance with fresh storage allocation
        from size_t_vector_alloc():

          data       == NULL
          alloc_size == 0
          data_owner == true


     2. Creating a shared wrapper from the size_t_vector_alloc_shared_wrapper():

          data       != NULL
          data_size   > 0
          data_owner == false


     3. Creating a private wrapper which steals the input data from
        size_t_vector_alloc_private_wrapper():

          data       != NULL
          data_size   > 0
          data_owner == true

  */

  if (data == NULL) {  /* Case 1: */
    vector->data              = NULL;
    vector->data_owner        = true;     /* The input values alloc_size and */
    vector->alloc_size        = 0;        /* data_owner are not even consulted. */
  } else {             /* Case 2 & 3 */
    vector->data              = data;
    vector->data_owner        = data_owner;
    vector->alloc_size        = alloc_size;
  }
  vector->size                = 0;

  ::Opm::RestartIO::size_t_vector_set_read_only( vector , false );
  if (init_size > 0)
    ::Opm::RestartIO::size_t_vector_iset( vector , init_size - 1 , default_value );  /* Filling up the init size elements with the default value */

  return vector;
}

::Opm::RestartIO::size_t_vector_type * size_t_vector_alloc(int init_size , size_t default_value) {
  ::Opm::RestartIO::size_t_vector_type * vector = size_t_vector_alloc__( init_size , default_value , NULL , 0 , true );
  return vector;
}

::Opm::RestartIO::inv_map_type * inv_map_alloc() {
  ::Opm::RestartIO::inv_map_type * map = static_cast<::Opm::RestartIO::inv_map_type *> (::Opm::RestartIO::util_malloc( sizeof * map ));
  map->file_kw_ptr = ::Opm::RestartIO::size_t_vector_alloc( 0 , 0 );
  map->ecl_kw_ptr  = ::Opm::RestartIO::size_t_vector_alloc( 0 , 0 );
  map->sorted = false;
  return map;
}


::Opm::RestartIO::ecl_file_type * ecl_file_alloc_empty( int flags ) {
  ::Opm::RestartIO::ecl_file_type * ecl_file = static_cast<::Opm::RestartIO::ecl_file_type *>(::Opm::RestartIO::util_malloc( sizeof * ecl_file ));
  UTIL_TYPE_ID_INIT(ecl_file , ECL_FILE_ID);
  ecl_file->map_stack = ::Opm::RestartIO::vector_alloc_new();
  ecl_file->inv_view  = ::Opm::RestartIO::inv_map_alloc( );
  ecl_file->flags     = flags;
  return ecl_file;
}

void util_strupr(char *s) {
  int i;
  for (i=0; i < static_cast<int>(strlen(s)); i++)
    s[i] = toupper(s[i]);
}

char * util_alloc_strupr_copy(const char * s) {
  char * c = ::Opm::RestartIO::util_alloc_string_copy(s);
  ::Opm::RestartIO::util_strupr(c);
  return c;
}

/**
   Takes a char buffer as input, and parses it as an integer. Returns
   true if the parsing succeeded, and false otherwise. If parsing
   succeeded, the integer value is returned by reference.
*/

bool util_sscanf_int(const char * buffer , int * value) {
  if(!buffer)
      return false;

  bool value_OK = false;
  char * error_ptr;

  int tmp_value = strtol(buffer , &error_ptr , 10);

  /*
    Skip trailing white-space
  */

  while (error_ptr[0] != '\0' && isspace(error_ptr[0]))
    error_ptr++;

  if (error_ptr[0] == '\0') {
    value_OK = true;
    if(value != NULL)
      *value = tmp_value;
  }
  return value_OK;
}


/*
 We accept mixed lowercase/uppercase Eclipse file extensions even if Eclipse itself does not accept them.
*/
::Opm::RestartIO::ecl_file_enum ecl_util_inspect_extension(const char * ext , bool *_fmt_file, int * _report_nr) {
  ::Opm::RestartIO::ecl_file_enum file_type = ECL_OTHER_FILE;
  bool fmt_file = true;
  int report_nr = -1;
  char* upper_ext = ::Opm::RestartIO::util_alloc_strupr_copy(ext);
  if (strcmp(upper_ext , "UNRST") == 0) {
    file_type = ECL_UNIFIED_RESTART_FILE;
    fmt_file = false;
  } else if (strcmp(upper_ext , "FUNRST") == 0) {
    file_type = ECL_UNIFIED_RESTART_FILE;
    fmt_file = true;
  } else if (strcmp(upper_ext , "UNSMRY") == 0) {
    file_type = ECL_UNIFIED_SUMMARY_FILE;
    fmt_file  = false;
  } else if (strcmp(upper_ext , "FUNSMRY") == 0) {
    file_type = ECL_UNIFIED_SUMMARY_FILE;
    fmt_file  = true;
  } else if (strcmp(upper_ext , "SMSPEC") == 0) {
    file_type = ECL_SUMMARY_HEADER_FILE;
    fmt_file  = false;
  } else if (strcmp(upper_ext , "FSMSPEC") == 0) {
    file_type = ECL_SUMMARY_HEADER_FILE;
    fmt_file  = true;
  } else if (strcmp(upper_ext , "GRID") == 0) {
    file_type = ECL_GRID_FILE;
    fmt_file  = false;
  } else if (strcmp(upper_ext , "FGRID") == 0) {
    file_type = ECL_GRID_FILE;
    fmt_file  = true;
  } else if (strcmp(upper_ext , "EGRID") == 0) {
    file_type = ECL_EGRID_FILE;
    fmt_file  = false;
  } else if (strcmp(upper_ext , "FEGRID") == 0) {
    file_type = ECL_EGRID_FILE;
    fmt_file  = true;
  } else if (strcmp(upper_ext , "INIT") == 0) {
    file_type = ECL_INIT_FILE;
    fmt_file  = false;
  } else if (strcmp(upper_ext , "FINIT") == 0) {
    file_type = ECL_INIT_FILE;
    fmt_file  = true;
  } else if (strcmp(upper_ext , "FRFT") == 0) {
    file_type = ECL_RFT_FILE;
    fmt_file  = true;
  } else if (strcmp(upper_ext , "RFT") == 0) {
    file_type = ECL_RFT_FILE;
    fmt_file  = false;
  } else if (strcmp(upper_ext , "DATA") == 0) {
    file_type = ECL_DATA_FILE;
    fmt_file  = true;  /* Not really relevant ... */
  } else {
    switch (upper_ext[0]) {
    case('X'):
      file_type = ECL_RESTART_FILE;
      fmt_file  = false;
      break;
    case('F'):
      file_type = ECL_RESTART_FILE;
      fmt_file  = true;
      break;
    case('S'):
      file_type = ECL_SUMMARY_FILE;
      fmt_file  = false;
      break;
    case('A'):
      file_type = ECL_SUMMARY_FILE;
      fmt_file  = true;
      break;
    default:
      file_type = ECL_OTHER_FILE;
    }
    if (file_type != ECL_OTHER_FILE)
      if (!::Opm::RestartIO::util_sscanf_int(&upper_ext[1] , &report_nr))
        file_type = ECL_OTHER_FILE;
  }

  if (_fmt_file != NULL)
    *_fmt_file  = fmt_file;

  if (_report_nr != NULL)
    *_report_nr = report_nr;

  free( upper_ext );
  return file_type;
}

::Opm::RestartIO::ecl_file_enum ecl_util_get_file_type(const char * filename, bool *_fmt_file, int * _report_nr) {

  const char *ext = strrchr(filename , '.');
  if (ext != NULL) {
    ext++;
    return ::Opm::RestartIO::ecl_util_inspect_extension( ext , _fmt_file , _report_nr);
  } else
    return ECL_OTHER_FILE;

}

static fortio_type * ecl_file_alloc_fortio(const char * filename, int flags) {
  fortio_type * fortio = NULL;
  bool          fmt_file;

  ::Opm::RestartIO::ecl_util_fmt_file( filename , &fmt_file);

  if (::Opm::RestartIO::ecl_file_view_check_flags(flags , ::Opm::RestartIO::ECL_FILE_WRITABLE))
    fortio = fortio_open_readwrite( filename , fmt_file , ECL_ENDIAN_FLIP);
  else
    fortio = fortio_open_reader( filename , fmt_file , ECL_ENDIAN_FLIP);

  return fortio;
}




::Opm::RestartIO::ecl_file_kw_type * ecl_file_kw_alloc0( const char * header , ::Opm::RestartIO::ecl_data_type data_type , int size , ::Opm::RestartIO::offset_type offset) {
  ::Opm::RestartIO::ecl_file_kw_type * file_kw = static_cast<::Opm::RestartIO::ecl_file_kw_type *> (::Opm::RestartIO::util_malloc( sizeof * file_kw ));
  UTIL_TYPE_ID_INIT( file_kw , ECL_FILE_KW_TYPE_ID );

  file_kw->header = ::Opm::RestartIO::util_alloc_string_copy( header );
  memcpy(&file_kw->data_type, &data_type, sizeof data_type);
  file_kw->kw_size = size;
  file_kw->file_offset = offset;
  file_kw->ref_count = 0;
  file_kw->kw = NULL;

  return file_kw;
}

/*
   Return the header without the trailing spaces
*/
const char * ecl_kw_get_header(const ::Opm::RestartIO::ecl_kw_type * ecl_kw ) {
  return ecl_kw->header;
}


/**
   Create a new ecl_file_kw instance based on header information from
   the input keyword. Typically only the header has been loaded from
   the keyword.

   Observe that it is the users responsability that the @offset
   argument in ecl_file_kw_alloc() comes from the same fortio instance
   as used when calling ecl_file_kw_get_kw() to actually instatiate
   the ecl_kw. This is automatically assured when using ecl_file to
   access the ecl_file_kw instances.
*/

::Opm::RestartIO::ecl_file_kw_type * ecl_file_kw_alloc( const ::Opm::RestartIO::ecl_kw_type * ecl_kw , ::Opm::RestartIO::offset_type offset ) {
  return ::Opm::RestartIO::ecl_file_kw_alloc0( ::Opm::RestartIO::ecl_kw_get_header( ecl_kw ) , ::Opm::RestartIO::ecl_kw_get_data_type( ecl_kw ) , ::Opm::RestartIO::ecl_kw_get_size( ecl_kw ) , offset );
}

bool ecl_file_kw_fskip_data( const ::Opm::RestartIO::ecl_file_kw_type * file_kw , fortio_type * fortio) {
  return ecl_kw_fskip_data__( ecl_file_kw_get_data_type(file_kw) , file_kw->kw_size , fortio );
}



void ecl_file_kw_free( ::Opm::RestartIO::ecl_file_kw_type * file_kw ) {
  if (file_kw->kw != NULL) {
    ecl_kw_free( file_kw->kw );
    file_kw->kw = NULL;
  }
  free( file_kw->header );
  free( file_kw );
}


static UTIL_SAFE_CAST_FUNCTION( ecl_file_kw , ECL_FILE_KW_TYPE_ID )
UTIL_IS_INSTANCE_FUNCTION( ecl_file_kw , ECL_FILE_KW_TYPE_ID )

void ecl_file_kw_free__( void * arg ) {
  ::Opm::RestartIO::ecl_file_kw_type * file_kw = ::Opm::RestartIO::ecl_file_kw_safe_cast( arg );
  ::Opm::RestartIO::ecl_file_kw_free( file_kw );
}

int hash_get_size(const ::Opm::RestartIO::hash_type *hash) {
  return hash->elements;
}

bool hash_sll_empty(const ::Opm::RestartIO::hash_sll_type * hash_sll) {
  if (hash_sll->length == 0)
    return true;
  else
    return false;
}


const char * hash_node_get_key(const ::Opm::RestartIO::hash_node_type * node) { 
  return node->key; 
}

::Opm::RestartIO::hash_node_type * hash_sll_get_head(const ::Opm::RestartIO::hash_sll_type *hash_sll) { return hash_sll->head; }

uint32_t hash_node_get_table_index(const ::Opm::RestartIO::hash_node_type * node)  { 
  return node->table_index; 
}

/**
   This functions takes a hash_node and finds the "next" hash node by
   traversing the internal hash structure. Should NOT be confused with
   the other functions providing iterations to user-space.
*/

static ::Opm::RestartIO::hash_node_type * hash_internal_iter_next(const ::Opm::RestartIO::hash_type *hash , const ::Opm::RestartIO::hash_node_type * node) {
  ::Opm::RestartIO::hash_node_type *next_node = ::Opm::RestartIO::hash_node_get_next(node);
  if (next_node == NULL) {
    const uint32_t table_index = ::Opm::RestartIO::hash_node_get_table_index(node);
    if (table_index < hash->size) {
      uint32_t i = table_index + 1;
      while (i < hash->size && ::Opm::RestartIO::hash_sll_empty(hash->table[i]))
        i++;

      if (i < hash->size)
        next_node = ::Opm::RestartIO::hash_sll_get_head(hash->table[i]);
    }
  }
  return next_node;
}

/**
   This is the low level function which traverses a hash table and
   allocates a char ** list of keys.

   It takes a read-lock which is held during the execution of the
   function. The locking guarantees that the list of keys is valid
   when this function is exited, but the the hash table can be
   subsequently updated.

   If the hash table is empty NULL is returned.
*/

static char ** hash_alloc_keylist__(::Opm::RestartIO::hash_type *hash , bool lock) {
  char **keylist;
  if (lock) ::Opm::RestartIO::__hash_rdlock( hash );
  {
    if (hash->elements > 0) {
      int i = 0;
      ::Opm::RestartIO::hash_node_type *node = NULL;
      keylist = (char**)std::calloc(hash->elements , sizeof *keylist);
      {
        uint32_t i = 0;
        while (i < hash->size && ::Opm::RestartIO::hash_sll_empty(hash->table[i]))
          i++;

        if (i < hash->size)
          node = ::Opm::RestartIO::hash_sll_get_head(hash->table[i]);
      }

      while (node != NULL) {
        const char *key = ::Opm::RestartIO::hash_node_get_key(node);
        keylist[i] = ::Opm::RestartIO::util_alloc_string_copy(key);
        node = hash_internal_iter_next(hash , node);
        i++;
      }
    } else keylist = NULL;
  }
  if (lock) ::Opm::RestartIO::__hash_unlock( hash );
  return keylist;
}


void hash_node_set_next(::Opm::RestartIO::hash_node_type *node , const ::Opm::RestartIO::hash_node_type *next_node) {
  node->next_node = (::Opm::RestartIO::hash_node_type *) next_node;
}



void hash_sll_del_node(::Opm::RestartIO::hash_sll_type *hash_sll , ::Opm::RestartIO::hash_node_type *del_node) {
  if (del_node == NULL) 
    util_abort("%s: tried to delete NULL node - aborting \n",__func__);

  {
    ::Opm::RestartIO::hash_node_type *node, *p_node;
    p_node = NULL;
    node   = hash_sll->head;
    while (node != NULL && node != del_node) {
      p_node = node;
      node   = ::Opm::RestartIO::hash_node_get_next(node);
    }

    if (node == del_node) {
      if (p_node == NULL) 
        /* 
           We are attempting to delete the first element in the list.
        */
        hash_sll->head = ::Opm::RestartIO::hash_node_get_next(del_node);
      else
        ::Opm::RestartIO::hash_node_set_next(p_node , ::Opm::RestartIO::hash_node_get_next(del_node));
      ::Opm::RestartIO::hash_node_free(del_node);
      hash_sll->length--;
    } else 
      util_abort("%s: tried to delete node not in list - aborting \n",__func__);

  }
}

/**
   This function deletes a node from the hash_table. Observe that this
   function does *NOT* do any locking - it is the repsonsibility of
   the calling functions: hash_del() and hash_clear() to take the
   necessary write lock.
*/


static void hash_del_unlocked__(::Opm::RestartIO::hash_type *hash , const char *key) {
  const uint32_t global_index = hash->hashf(key , strlen(key));
  const uint32_t table_index  = (global_index % hash->size);
  ::Opm::RestartIO::hash_node_type *node        = ::Opm::RestartIO::hash_sll_get(hash->table[table_index] , global_index , key);

  if (node == NULL)
    util_abort("%s: hash does not contain key:%s - aborting \n",__func__ , key);
  else
    ::Opm::RestartIO::hash_sll_del_node(hash->table[table_index] , node);

  hash->elements--;
}

/**
   This function iterates over the kw_list vector and builds the
   internal index fields 'kw_index' and 'distinct_kw'. This function
   must be called every time the content of the kw_list vector is
   modified (otherwise the ecl_file instance will be in an
   inconsistent state).
*/


void hash_clear(::Opm::RestartIO::hash_type *hash) {
  ::Opm::RestartIO::__hash_wrlock( hash );
  {
    int old_size = ::Opm::RestartIO::hash_get_size(hash);
    if (old_size > 0) {
      char **keyList = ::Opm::RestartIO::hash_alloc_keylist__( hash , false);
      int i;
      for (i=0; i < old_size; i++) {
        ::Opm::RestartIO::hash_del_unlocked__(hash , keyList[i]);
        free(keyList[i]);
      }
      free(keyList);
    }
  }
  ::Opm::RestartIO::__hash_unlock( hash );
}

void int_vector_set_read_only( ::Opm::RestartIO::int_vector_type * vector , bool read_only) {
  vector->read_only = read_only;
}

static void int_vector_assert_writable( const ::Opm::RestartIO::int_vector_type * vector ) {
  if (vector->read_only)
    util_abort("%s: Sorry - tried to modify a read_only vector instance.\n",__func__);
}


static void int_vector_realloc_data__(::Opm::RestartIO::int_vector_type * vector , int new_alloc_size) {
  if (new_alloc_size != vector->alloc_size) {
    if (vector->data_owner) {
      if (new_alloc_size > 0) {
        int i;
        vector->data = (int*)::Opm::RestartIO::util_realloc(vector->data , new_alloc_size * sizeof * vector->data );
        for (i=vector->alloc_size;  i < new_alloc_size; i++)
          vector->data[i] = vector->default_value;
      } else {
        if (vector->alloc_size > 0) {
          free(vector->data);
          vector->data = NULL;
        }
      }
      vector->alloc_size = new_alloc_size;
    } else
      util_abort("%s: tried to change the storage are for a shared data segment \n",__func__);
  }
}

/**
   Observe that this function will grow the vector if necessary. If
   index > size - i.e. leaving holes in the vector, these are
   explicitly set to the default value. If a reallocation is needed it
   is done in the realloc routine, otherwise it is done here.
*/

void int_vector_iset(::Opm::RestartIO::int_vector_type * vector , int index , int value) {
  ::Opm::RestartIO::int_vector_assert_writable( vector );
  {
    if (index < 0)
      util_abort("%s: Sorry - can NOT set negative indices. called with index:%d \n",__func__ , index);
    {
      if (vector->alloc_size <= index)
        ::Opm::RestartIO::int_vector_realloc_data__(vector , 2 * (index + 1));  /* Must have ( + 1) here to ensure we are not doing 2*0 */
      vector->data[index] = value;
      if (index >= vector->size) {
        int i;
        for (i=vector->size; i < index; i++)
          vector->data[i] = vector->default_value;
        vector->size = index + 1;
      }
    }
  }
}

static ::Opm::RestartIO::int_vector_type * int_vector_alloc__(int init_size , int default_value, int * data, int alloc_size , bool data_owner ) {
  ::Opm::RestartIO::int_vector_type * vector = (::Opm::RestartIO::int_vector_type*)::Opm::RestartIO::util_malloc( sizeof * vector );
  UTIL_TYPE_ID_INIT( vector , TYPE_VECTOR_ID);
  vector->default_value       = default_value;

  /**
     Not all combinations of (data, alloc_size, data_owner) are valid:

     1. Creating a new vector instance with fresh storage allocation
        from int_vector_alloc():

          data       == NULL
          alloc_size == 0
          data_owner == true


     2. Creating a shared wrapper from the int_vector_alloc_shared_wrapper():

          data       != NULL
          data_size   > 0
          data_owner == false


     3. Creating a private wrapper which steals the input data from
        int_vector_alloc_private_wrapper():

          data       != NULL
          data_size   > 0
          data_owner == true

  */

  if (data == NULL) {  /* Case 1: */
    vector->data              = NULL;
    vector->data_owner        = true;     /* The input values alloc_size and */
    vector->alloc_size        = 0;        /* data_owner are not even consulted. */
  } else {             /* Case 2 & 3 */
    vector->data              = data;
    vector->data_owner        = data_owner;
    vector->alloc_size        = alloc_size;
  }
  vector->size                = 0;

  int_vector_set_read_only( vector , false );
  if (init_size > 0)
    ::Opm::RestartIO::int_vector_iset( vector , init_size - 1 , default_value );  /* Filling up the init size elements with the default value */

  return vector;
}


/**
   The alloc_size argument is just a hint - the vector will grow as
   needed.
*/

::Opm::RestartIO::int_vector_type * int_vector_alloc(int init_size , int default_value) {
  ::Opm::RestartIO::int_vector_type * vector = int_vector_alloc__( init_size , default_value , NULL , 0 , true );
  return vector;
}


uint32_t hash_node_set_table_index(::Opm::RestartIO::hash_node_type *node , uint32_t table_size) {
  node->table_index = (node->global_index % table_size);
  return node->table_index;
}


hash_node_type * hash_node_alloc_new(const char *key, ::Opm::RestartIO::node_data_type * data, ::Opm::RestartIO::hashf_type *hashf , uint32_t table_size) {
  ::Opm::RestartIO::hash_node_type *node;
  node              = (::Opm::RestartIO::hash_node_type*) ::Opm::RestartIO::util_malloc(sizeof *node );
  node->key         = util_alloc_string_copy( key );
  node->data        = data;
  node->next_node   = NULL;
        
  node->global_index = hashf(node->key , strlen(node->key));
  ::Opm::RestartIO::hash_node_set_table_index(node , table_size);
  return node;
}


void hash_sll_add_node(::Opm::RestartIO::hash_sll_type *hash_sll , ::Opm::RestartIO::hash_node_type *new_node) {
  ::Opm::RestartIO::hash_node_set_next(new_node, hash_sll->head);
  hash_sll->head = new_node;
  hash_sll->length++;
}


/**
   This function resizes the hash table when it has become to full.
   The table only grows - this function is called from
   __hash_insert_node().

   If you know in advance (roughly) how large the hash table will be
   it can be advantageous to call hash_resize() manually, to avoid
   repeated internal calls to hash_resize().
*/

void hash_resize(::Opm::RestartIO::hash_type *hash, int new_size) {
  ::Opm::RestartIO::hash_sll_type ** new_table = ::Opm::RestartIO::hash_sll_alloc_table( new_size );
  ::Opm::RestartIO::hash_node_type * node;
  uint32_t i;

  for (i=0; i < hash->size; i++) {
    node = ::Opm::RestartIO::hash_sll_get_head(hash->table[i]);
    while (node != NULL) {
      uint32_t new_table_index  = ::Opm::RestartIO::hash_node_set_table_index(node , new_size);
      ::Opm::RestartIO::hash_node_type *next_node = ::Opm::RestartIO::hash_node_get_next(node);
      ::Opm::RestartIO::hash_sll_add_node(new_table[new_table_index] , node);
      node = next_node;
    }
  }

  /*
     Only freeing the table structure, *NOT* calling the node_free()
     functions, which happens when hash_sll_free() is called.
  */

  {
    for (i=0; i < hash->size; i++)
      free( hash->table[i] );
    free( hash->table );
  }

  hash->size     = new_size;
  hash->table    = new_table;
}




/**
   This is the low-level function for inserting a hash node. This
   function takes a write-lock which is held during the execution of
   the function.
*/

static void __hash_insert_node(::Opm::RestartIO::hash_type *hash , ::Opm::RestartIO::hash_node_type *node) {
  ::Opm::RestartIO::__hash_wrlock( hash );
  {
    uint32_t table_index = ::Opm::RestartIO::hash_node_get_table_index(node);
    {
      /*
        If a node with the same key already exists in the table
        it is removed.
      */
      ::Opm::RestartIO::hash_node_type *existing_node = (::Opm::RestartIO::hash_node_type*)__hash_get_node_unlocked(hash , hash_node_get_key(node) , false);
      if (existing_node != NULL) {
        ::Opm::RestartIO::hash_sll_del_node(hash->table[table_index] , existing_node);
        hash->elements--;
      }
    }

    ::Opm::RestartIO::hash_sll_add_node(hash->table[table_index] , node);
    hash->elements++;
    if ((1.0 * hash->elements / hash->size) > hash->resize_fill)
      ::Opm::RestartIO::hash_resize(hash , hash->size * 2);
  }
  ::Opm::RestartIO::__hash_unlock( hash );
}


/**
  This function will insert a reference "value" with key "key"; when
  the key is deleted - either by an explicit call to hash_del(), or
  when the complete hash table is free'd with hash_free(), the
  destructur 'del' is called with 'value' as argument.

  It is importand to realize that when elements are inserted into a
  hash table with this function the calling scope gives up
  responsibility of freeing the memory pointed to by value.
*/

void hash_insert_hash_owned_ref(::Opm::RestartIO::hash_type *hash , const char *key , const void *value , free_ftype *del) {
  ::Opm::RestartIO::hash_node_type *hash_node;
  if (del == NULL)
    util_abort("%s: must provide delete operator for insert hash_owned_ref - aborting \n",__func__);
  {
    ::Opm::RestartIO::node_data_type * data_node = ::Opm::RestartIO::node_data_alloc_ptr( value , NULL , del );
    hash_node                  = ::Opm::RestartIO::hash_node_alloc_new(key , data_node , hash->hashf , hash->size);
    ::Opm::RestartIO::__hash_insert_node(hash , hash_node);
  }
}


::Opm::RestartIO::node_data_type * node_data_alloc_buffer(const void * data, int buffer_size) {  /* The buffer is copied on insert. */
  void * data_copy = ::Opm::RestartIO::util_alloc_copy( data , buffer_size );
  return ::Opm::RestartIO::node_data_alloc__( data_copy , ::Opm::RestartIO::CTYPE_VOID_POINTER , buffer_size , NULL , free);
}


/**
   A buffer is unstructured storage (i.e. a void *) which is destroyed
   with free, and copied with malloc + memcpy. The vector takes a copy
   of the buffer which is inserted (and freed on vector destruction).
*/


void vector_append_buffer(::Opm::RestartIO::vector_type * vector , const void * buffer, int buffer_size) {
  ::Opm::RestartIO::node_data_type * node = ::Opm::RestartIO::node_data_alloc_buffer( buffer , buffer_size );
  ::Opm::RestartIO::vector_append_node(vector , node);
}

/**
   This function appends a copy of s into the stringlist.
*/
void stringlist_append_copy(::Opm::RestartIO::stringlist_type * stringlist , const char * s) {
  ::Opm::RestartIO::vector_append_buffer(stringlist->strings , s , strlen(s) + 1);
}

void int_vector_free_container(::Opm::RestartIO::int_vector_type * vector) {
  free( vector );
}

void int_vector_free(::Opm::RestartIO::int_vector_type * vector) {
  if (vector->data_owner)
    ::Opm::RestartIO::util_safe_free( vector->data );
  ::Opm::RestartIO::int_vector_free_container( vector );
}

void int_vector_free__(void * __vector) {
  ::Opm::RestartIO::int_vector_type * vector = (::Opm::RestartIO::int_vector_type *) __vector;
  ::Opm::RestartIO::int_vector_free( vector );
}

void int_vector_append(::Opm::RestartIO::int_vector_type * vector , int value) {
  ::Opm::RestartIO::int_vector_iset(vector , vector->size , value);
}


void ecl_file_view_make_index( ::Opm::RestartIO::ecl_file_view_type * ecl_file_view ) {
  ::Opm::RestartIO::stringlist_clear( ecl_file_view->distinct_kw );
  ::Opm::RestartIO::hash_clear( ecl_file_view->kw_index );
  {
    int i;
    for (i=0; i < ::Opm::RestartIO::vector_get_size( ecl_file_view->kw_list ); i++) {
      const ::Opm::RestartIO::ecl_file_kw_type * file_kw = static_cast<const ::Opm::RestartIO::ecl_file_kw_type *> (::Opm::RestartIO::vector_iget_const( ecl_file_view->kw_list , i));
      const char             * header  = ::Opm::RestartIO::ecl_file_kw_get_header( file_kw );
      if ( !::Opm::RestartIO::hash_has_key( ecl_file_view->kw_index , header )) {
        ::Opm::RestartIO::int_vector_type * index_vector = ::Opm::RestartIO::int_vector_alloc( 0 , -1 );
        ::Opm::RestartIO::hash_insert_hash_owned_ref( ecl_file_view->kw_index , header , index_vector , int_vector_free__);
        ::Opm::RestartIO::stringlist_append_copy( ecl_file_view->distinct_kw , header);
      }

      {
        ::Opm::RestartIO::int_vector_type * index_vector = static_cast<::Opm::RestartIO::int_vector_type *>(::Opm::RestartIO::hash_get( ecl_file_view->kw_index , header));
        ::Opm::RestartIO::int_vector_append( index_vector , i);
      }
    }
  }
}


/**
   The ecl_file_scan() function will scan through the whole file and
   build up an index of all the kewyords. The map created from this
   scan will be stored under the 'global_view' field; and all
   subsequent lookup operations will ultimately be based on the global
   map.
*/


static bool ecl_file_scan( ::Opm::RestartIO::ecl_file_type * ecl_file ) {
  bool scan_ok = false;
  fortio_fseek( ecl_file->fortio , 0 , SEEK_SET );
  {
    ::Opm::RestartIO::ecl_kw_type * work_kw = ::Opm::RestartIO::ecl_kw_alloc_new("WORK-KW" , 0 , ECL_INT_2 , NULL);

    while (true) {
      if (fortio_read_at_eof(ecl_file->fortio)) {
        scan_ok = true;
        break;
      }

      {
        ::Opm::RestartIO::offset_type current_offset = fortio_ftell( ecl_file->fortio );
        ::Opm::RestartIO::ecl_read_status_enum read_status = ::Opm::RestartIO::ecl_kw_fread_header( work_kw , ecl_file->fortio);
        if (read_status == ::Opm::RestartIO::ECL_KW_READ_FAIL)
          break;

        if (read_status == ::Opm::RestartIO::ECL_KW_READ_OK) {
          ::Opm::RestartIO::ecl_file_kw_type * file_kw = ::Opm::RestartIO::ecl_file_kw_alloc( work_kw , current_offset);
          if (::Opm::RestartIO::ecl_file_kw_fskip_data( file_kw , ecl_file->fortio ))
            ::Opm::RestartIO::ecl_file_view_add_kw( ecl_file->global_view , file_kw );
          else
            break;
        }
      }
    }

    ::Opm::RestartIO::ecl_kw_free( work_kw );
  }
  if (scan_ok)
    ::Opm::RestartIO::ecl_file_view_make_index( ecl_file->global_view );

  return scan_ok;
}

void ecl_file_select_global( ::Opm::RestartIO::ecl_file_type * ecl_file ) {
  ecl_file->active_view = ecl_file->global_view;
}

::Opm::RestartIO::ecl_file_type * ecl_file_open( const char * filename , int flags) {
  fortio_type * fortio = ::Opm::RestartIO::ecl_file_alloc_fortio(filename, flags);

  if (fortio) {
    ::Opm::RestartIO::ecl_file_type * ecl_file = ::Opm::RestartIO::ecl_file_alloc_empty( flags );
    ecl_file->fortio = fortio;
    ecl_file->global_view = ::Opm::RestartIO::ecl_file_view_alloc( ecl_file->fortio , &ecl_file->flags , ecl_file->inv_view , true );

    if (::Opm::RestartIO::ecl_file_scan( ecl_file )) {
      ::Opm::RestartIO::ecl_file_select_global( ecl_file );

      if (ecl_file_view_check_flags( ecl_file->flags , ::Opm::RestartIO::ECL_FILE_CLOSE_STREAM))
        fortio_fclose_stream( ecl_file->fortio );

      return ecl_file;
    } else {
      ::Opm::RestartIO::ecl_file_close( ecl_file );
      return NULL;
    }
  } else
    return NULL;
}


void ecl_kw_fwrite_header(const ::Opm::RestartIO::ecl_kw_type *ecl_kw , fortio_type *fortio) {
  FILE *stream  = fortio_get_FILE(fortio);
  bool fmt_file = fortio_fmt_file(fortio);
  char * type_name = ::Opm::RestartIO::ecl_type_alloc_name(ecl_kw->data_type);

  if (fmt_file)
    fprintf(stream , WRITE_HEADER_FMT , ecl_kw->header8 , ecl_kw->size , type_name);
  else {
    int size = ecl_kw->size;
    if (ECL_ENDIAN_FLIP)
      ::Opm::RestartIO::util_endian_flip_vector(&size , sizeof size , 1);

    fortio_init_write(fortio , ECL_KW_HEADER_DATA_SIZE );

    fwrite(ecl_kw->header8                            , sizeof(char)    , ECL_STRING8_LENGTH  , stream);
    fwrite(&size                                      , sizeof(int)     , 1                  , stream);
    fwrite(type_name , sizeof(char)    , ECL_TYPE_LENGTH    , stream);

    fortio_complete_write(fortio , ECL_KW_HEADER_DATA_SIZE);

  }

  free(type_name);
}


static void ecl_kw_fwrite_data_unformatted( ::Opm::RestartIO::ecl_kw_type * ecl_kw , fortio_type * fortio ) {
  if (ECL_ENDIAN_FLIP)
    ::Opm::RestartIO::ecl_kw_endian_convert_data(ecl_kw);

  {
    const int blocksize  = ::Opm::RestartIO::get_blocksize( ecl_kw->data_type );
    const int num_blocks = ecl_kw->size / blocksize + (ecl_kw->size % blocksize == 0 ? 0 : 1);
    int block_nr;

    for (block_nr = 0; block_nr < num_blocks; block_nr++) {
      int this_blocksize = util_int_min((block_nr + 1)*blocksize , ecl_kw->size) - block_nr*blocksize;
      if (::Opm::RestartIO::ecl_type_is_char(ecl_kw->data_type) || ::Opm::RestartIO::ecl_type_is_mess(ecl_kw->data_type) || ::Opm::RestartIO::ecl_type_is_string(ecl_kw->data_type)) {
        /*
           Due to the terminating \0 characters there is not a
           continous file/memory mapping - the \0 characters arel
           skipped.
        */
        FILE *stream    = fortio_get_FILE(fortio);
        int word_size   = ::Opm::RestartIO::ecl_type_get_sizeof_ctype_fortio(ecl_kw->data_type);
        int record_size = this_blocksize * word_size;     /* The total size in bytes of the record written by the fortio layer. */
        int i;
        fortio_init_write(fortio , record_size );
        for (i = 0; i < this_blocksize; i++)
          fwrite(&ecl_kw->data[(block_nr * blocksize + i) * ::Opm::RestartIO::ecl_kw_get_sizeof_ctype(ecl_kw)] , 1 , word_size , stream);
        fortio_complete_write(fortio , record_size);
      } else {
        int   record_size = this_blocksize * ::Opm::RestartIO::ecl_kw_get_sizeof_ctype(ecl_kw);  /* The total size in bytes of the record written by the fortio layer. */
        fortio_fwrite_record(fortio , &ecl_kw->data[block_nr * blocksize * ::Opm::RestartIO::ecl_kw_get_sizeof_ctype(ecl_kw)] , record_size);
      }
    }
  }

  if (ECL_ENDIAN_FLIP)
    ::Opm::RestartIO::ecl_kw_endian_convert_data(ecl_kw);
}


static int get_columns(const ::Opm::RestartIO::ecl_data_type data_type) {
  switch(::Opm::RestartIO::ecl_type_get_type(data_type)) {
  case(ECL_CHAR_TYPE):
    return COLUMNS_CHAR;
  case(ECL_INT_TYPE):
    return COLUMNS_INT;
  case(ECL_FLOAT_TYPE):
    return COLUMNS_FLOAT;
  case(ECL_DOUBLE_TYPE):
    return COLUMNS_DOUBLE;
  case(ECL_BOOL_TYPE):
    return COLUMNS_BOOL;
  case(ECL_MESS_TYPE):
    return COLUMNS_MESSAGE;
  case(ECL_STRING_TYPE):
    return COLUMNS_CHAR; // TODO: Is this correct?
  default:
    util_abort("%s: invalid ecl_type: %s\n",__func__ , ::Opm::RestartIO::ecl_type_alloc_name(data_type));
    return -1;
  }
}


static char * alloc_write_fmt_string(const ::Opm::RestartIO::ecl_data_type ecl_type) {
  return ::Opm::RestartIO::util_alloc_sprintf(
          " '%%-%ds'",
          ::Opm::RestartIO::ecl_type_get_sizeof_ctype_fortio(ecl_type)
          );
}

static char * alloc_write_fmt(const ::Opm::RestartIO::ecl_data_type data_type) {
  switch(::Opm::RestartIO::ecl_type_get_type(data_type)) {
  case(ECL_CHAR_TYPE):
    return ::Opm::RestartIO::util_alloc_string_copy(WRITE_FMT_CHAR);
  case(ECL_INT_TYPE):
    return ::Opm::RestartIO::util_alloc_string_copy(WRITE_FMT_INT);
  case(ECL_FLOAT_TYPE):
    return ::Opm::RestartIO::util_alloc_string_copy(WRITE_FMT_FLOAT);
  case(ECL_DOUBLE_TYPE):
    return ::Opm::RestartIO::util_alloc_string_copy(WRITE_FMT_DOUBLE);
  case(ECL_BOOL_TYPE):
    return ::Opm::RestartIO::util_alloc_string_copy(WRITE_FMT_BOOL);
  case(ECL_MESS_TYPE):
    return ::Opm::RestartIO::util_alloc_string_copy(WRITE_FMT_MESS);
  case(ECL_STRING_TYPE):
    return ::Opm::RestartIO::alloc_write_fmt_string(data_type);
  default:
    util_abort("%s: invalid ecl_type: %s\n",__func__ , ::Opm::RestartIO::ecl_type_alloc_name(data_type));
    return NULL;
  }
}

/**
     The point of this awkward function is that I have not managed to
     use C fprintf() syntax to reproduce the ECLIPSE
     formatting. ECLIPSE expects the following formatting for float
     and double values:

        0.ddddddddE+03       (float)
        0.ddddddddddddddD+03 (double)

     The problem with printf have been:

        1. To force the radix part to start with 0.
        2. To use 'D' as the exponent start for double values.

     If you are more proficient with C fprintf() format strings than I
     am, the __fprintf_scientific() function should be removed, and
     the WRITE_FMT_DOUBLE and WRITE_FMT_FLOAT format specifiers
     updated accordingly.
  */

static void __fprintf_scientific(FILE * stream, const char * fmt , double x) {
 double pow_x = ceil(log10(fabs(x)));
 double arg_x   = x / pow(10.0 , pow_x);
 if (x != 0.0) {
   if (fabs(arg_x) == 1.0) {
     arg_x *= 0.10;
     pow_x += 1;
   }
 } else {
   arg_x = 0.0;
   pow_x = 0.0;
 }
 fprintf(stream , fmt , arg_x , (int) pow_x);
}


static void ecl_kw_fwrite_data_formatted( ::Opm::RestartIO::ecl_kw_type * ecl_kw , fortio_type * fortio ) {

  {

    FILE * stream           = fortio_get_FILE( fortio );
    const int blocksize     = ::Opm::RestartIO::get_blocksize( ecl_kw->data_type );
    const  int columns      = ::Opm::RestartIO::get_columns( ecl_kw->data_type );
    char * write_fmt        = ::Opm::RestartIO::alloc_write_fmt( ecl_kw->data_type );
    const int num_blocks    = ecl_kw->size / blocksize + (ecl_kw->size % blocksize == 0 ? 0 : 1);
    int block_nr;

    for (block_nr = 0; block_nr < num_blocks; block_nr++) {
      int this_blocksize = util_int_min((block_nr + 1)*blocksize , ecl_kw->size) - block_nr*blocksize;
      int num_lines      = this_blocksize / columns + ( this_blocksize % columns == 0 ? 0 : 1);
      int line_nr;
      for (line_nr = 0; line_nr < num_lines; line_nr++) {
        int num_columns = util_int_min( (line_nr + 1)*columns , this_blocksize) - columns * line_nr;
        int col_nr;
        for (col_nr =0; col_nr < num_columns; col_nr++) {
          int data_index  = block_nr * blocksize + line_nr * columns + col_nr;
          void * data_ptr = ::Opm::RestartIO::ecl_kw_iget_ptr_static( ecl_kw , data_index );
          switch (::Opm::RestartIO::ecl_kw_get_type(ecl_kw)) {
          case(ECL_CHAR_TYPE):
            fprintf(stream , write_fmt , data_ptr);
            break;
          case(ECL_STRING_TYPE):
            fprintf(stream , write_fmt , data_ptr);
            break;
          case(ECL_INT_TYPE):
            {
              int int_value = ((int *) data_ptr)[0];
              fprintf(stream , write_fmt , int_value);
            }
            break;
          case(ECL_BOOL_TYPE):
            {
              bool bool_value = ((bool *) data_ptr)[0];
              if (bool_value)
                fprintf(stream , write_fmt , BOOL_TRUE_CHAR);
              else
                fprintf(stream , write_fmt , BOOL_FALSE_CHAR);
            }
            break;
          case(ECL_FLOAT_TYPE):
            {
              float float_value = ((float *) data_ptr)[0];
              __fprintf_scientific( stream , write_fmt , float_value );
            }
            break;
          case(ECL_DOUBLE_TYPE):
            {
              double double_value = ((double *) data_ptr)[0];
              __fprintf_scientific( stream , write_fmt , double_value );
            }
            break;
          case(ECL_MESS_TYPE):
            util_abort("%s: internal fuckup : message type keywords should NOT have data ??\n",__func__);
            break;
          }
        }
        fprintf(stream , "\n");
      }
    }

    free(write_fmt);
  }
}

void ecl_kw_fwrite_data(const ::Opm::RestartIO::ecl_kw_type *_ecl_kw , fortio_type *fortio) {
  ::Opm::RestartIO::ecl_kw_type *ecl_kw = (::Opm::RestartIO::ecl_kw_type *) _ecl_kw;
  bool  fmt_file      = fortio_fmt_file( fortio );

  if (fmt_file)
    ::Opm::RestartIO::ecl_kw_fwrite_data_formatted( ecl_kw , fortio );
  else
    ::Opm::RestartIO::ecl_kw_fwrite_data_unformatted( ecl_kw ,fortio );
}



bool ecl_kw_fwrite(const ::Opm::RestartIO::ecl_kw_type *ecl_kw , fortio_type *fortio) {
  if (strlen(::Opm::RestartIO::ecl_kw_get_header( ecl_kw)) > ECL_STRING8_LENGTH) {
     fortio_fwrite_error(fortio);
     return false;
  }
  ::Opm::RestartIO::ecl_kw_fwrite_header(ecl_kw ,  fortio);
  ::Opm::RestartIO::ecl_kw_fwrite_data(ecl_kw   ,  fortio);
  return true;
}



//namespace {

void ecl_rst_file_add_kw(::Opm::RestartIO::ecl_rst_file_type * rst_file , const ::Opm::RestartIO::ecl_kw_type * ecl_kw ) {
  ::Opm::RestartIO::ecl_kw_fwrite( ecl_kw , rst_file->fortio );
}


void util_time_utc( time_t * t , struct tm * ts ) {
#ifdef HAVE_GMTIME_R
  gmtime_r( t , ts );
#else
  struct tm * ts_shared = std::localtime( t );
  std::memcpy( ts , ts_shared , sizeof * ts );
#endif
}


static void __util_set_timevalues_utc(time_t t , int * sec , int * min , int * hour , int * mday , int * month , int * year) {
  struct tm ts;

  ::Opm::RestartIO::util_time_utc(&t , &ts);
  if (sec   != NULL) *sec   = ts.tm_sec;
  if (min   != NULL) *min   = ts.tm_min;
  if (hour  != NULL) *hour  = ts.tm_hour;
  if (mday  != NULL) *mday  = ts.tm_mday;
  if (month != NULL) *month = ts.tm_mon  + 1;
  if (year  != NULL) *year  = ts.tm_year + 1900;
}


void util_set_date_values_utc(time_t t , int * mday , int * month , int * year) {
  ::Opm::RestartIO::__util_set_timevalues_utc(t , NULL , NULL , NULL , mday , month , year);
}

void ecl_util_set_date_values(time_t t , int * mday , int * month , int * year) {
  return ::Opm::RestartIO::util_set_date_values_utc(t,mday,month,year);
}

void ecl_rst_file_fwrite_SEQNUM( Opm::RestartIO::ecl_rst_file_type * rst_file , int seqnum ) {
  ::Opm::RestartIO::ecl_kw_type * seqnum_kw = ::Opm::RestartIO::ecl_kw_alloc( SEQNUM_KW , 1 , ECL_INT_2 );
  Opm::RestartIO::ecl_kw_iset_type( seqnum_kw , ECL_INT_TYPE, 0 , seqnum );
  ::Opm::RestartIO::ecl_kw_fwrite( seqnum_kw , rst_file->fortio );
  ::Opm::RestartIO::ecl_kw_free( seqnum_kw );
}

static ::Opm::RestartIO::ecl_kw_type * ecl_rst_file_alloc_INTEHEAD( ::Opm::RestartIO::ecl_rst_file_type * rst_file,
                                                  ::Opm::RestartIO::ecl_rsthead_type * rsthead,
                                                  int simulator ) {
  ::Opm::RestartIO::ecl_kw_type * intehead_kw = ::Opm::RestartIO::ecl_kw_alloc( INTEHEAD_KW , INTEHEAD_RESTART_SIZE , ECL_INT_2 );
  ::Opm::RestartIO::ecl_kw_scalar_set_type( intehead_kw , ECL_INT_TYPE, 0 );
  ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_UNIT_INDEX    , rsthead->unit_system );
  ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_NX_INDEX      , rsthead->nx);
  ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_NY_INDEX      , rsthead->ny);
  ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_NZ_INDEX      , rsthead->nz);
  ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_NACTIVE_INDEX , rsthead->nactive);
  ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_PHASE_INDEX   , rsthead->phase_sum );
  {
    int NGMAXZ = 0;
    int NWGMAX = 0;
    int NIGRPZ = 0;
    int NSWLMX = 0;
    int NSEGMX = 0;
    int NISEGZ = 0;

    ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_NWELLS_INDEX  , rsthead->nwells );
    ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_NCWMAX_INDEX  , rsthead->ncwmax );
    ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_NWGMAX_INDEX  , NWGMAX );
    ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_NGMAXZ_INDEX  , NGMAXZ );
    ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_NIWELZ_INDEX  , rsthead->niwelz );
    ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_NZWELZ_INDEX  , rsthead->nzwelz );
    ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_NICONZ_INDEX  , rsthead->niconz );
    ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_NIGRPZ_INDEX  , NIGRPZ );

    {
      ::Opm::RestartIO::ecl_util_set_date_values( rsthead->sim_time , &rsthead->day , &rsthead->month , &rsthead->year );
      ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_DAY_INDEX    , rsthead->day );
      ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_MONTH_INDEX  , rsthead->month );
      ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_YEAR_INDEX   , rsthead->year );
    }

    ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_IPROG_INDEX , simulator);
    ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_NSWLMX_INDEX  , NSWLMX );
    ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_NSEGMX_INDEX  , NSEGMX );
    ::Opm::RestartIO::ecl_kw_iset_type( intehead_kw , ECL_INT_TYPE, INTEHEAD_NISEGZ_INDEX  , NISEGZ );
  }
  return intehead_kw;
}

void ecl_kw_scalar_set_bool( ::Opm::RestartIO::ecl_kw_type * ecl_kw , bool bool_value) {
  if (::Opm::RestartIO::ecl_kw_get_type(ecl_kw) != ECL_BOOL_TYPE)
    util_abort("%s: Keyword: %s is wrong type - aborting \n",__func__ , ::Opm::RestartIO::ecl_kw_get_header8(ecl_kw));
  {
    int * data = static_cast<int *>(::Opm::RestartIO::ecl_kw_get_data_ref(ecl_kw));
    int int_value;
    if (bool_value)
      int_value = ECL_BOOL_TRUE_INT;
    else
      int_value = ECL_BOOL_FALSE_INT;
    {
      int i;
      for ( i=0; i < ecl_kw->size; i++)
        data[i] = int_value;
    }
  }
}


static ::Opm::RestartIO::ecl_kw_type * ecl_rst_file_alloc_LOGIHEAD( int simulator ) {
  bool dual_porosity          = false;
  bool radial_grid_ECLIPSE100 = false;
  bool radial_grid_ECLIPSE300 = false;

  ::Opm::RestartIO::ecl_kw_type * logihead_kw = ::Opm::RestartIO::ecl_kw_alloc( LOGIHEAD_KW , LOGIHEAD_RESTART_SIZE , ECL_BOOL );

  ::Opm::RestartIO::ecl_kw_scalar_set_bool( logihead_kw , false );

  if (simulator == INTEHEAD_ECLIPSE100_VALUE)
    ::Opm::RestartIO::ecl_kw_iset_bool( logihead_kw , LOGIHEAD_RADIAL100_INDEX , radial_grid_ECLIPSE100);
  else
    ::Opm::RestartIO::ecl_kw_iset_bool( logihead_kw , LOGIHEAD_RADIAL300_INDEX , radial_grid_ECLIPSE300);

  ::Opm::RestartIO::ecl_kw_iset_bool( logihead_kw , LOGIHEAD_DUALP_INDEX , dual_porosity);
  return logihead_kw;
}

static ::Opm::RestartIO::ecl_kw_type * ecl_rst_file_alloc_DOUBHEAD( ::Opm::RestartIO::ecl_rst_file_type * rst_file , double days) {
  ::Opm::RestartIO::ecl_kw_type * doubhead_kw = ::Opm::RestartIO::ecl_kw_alloc( DOUBHEAD_KW , DOUBHEAD_RESTART_SIZE , ECL_DOUBLE );
  ::Opm::RestartIO::ecl_kw_scalar_set_type( doubhead_kw , ECL_DOUBLE_TYPE, 0);
  ::Opm::RestartIO::ecl_kw_iset_type( doubhead_kw , ECL_DOUBLE_TYPE, DOUBHEAD_DAYS_INDEX , days );


  return doubhead_kw;
}


void ecl_rst_file_fwrite_header( ::Opm::RestartIO::ecl_rst_file_type * rst_file ,
                                 int seqnum ,
                                 ::Opm::RestartIO::ecl_rsthead_type * rsthead_data ) {

  if (rst_file->unified)
    ::Opm::RestartIO::ecl_rst_file_fwrite_SEQNUM( rst_file , seqnum );

  {
    ::Opm::RestartIO::ecl_kw_type * intehead_kw = ::Opm::RestartIO::ecl_rst_file_alloc_INTEHEAD( rst_file , rsthead_data , INTEHEAD_ECLIPSE100_VALUE);
    ::Opm::RestartIO::ecl_kw_fwrite( intehead_kw , rst_file->fortio );
    ::Opm::RestartIO::ecl_kw_free( intehead_kw );
  }

  {
    ::Opm::RestartIO::ecl_kw_type * logihead_kw = ::Opm::RestartIO::ecl_rst_file_alloc_LOGIHEAD( INTEHEAD_ECLIPSE100_VALUE);
    ::Opm::RestartIO::ecl_kw_fwrite( logihead_kw , rst_file->fortio );
    ::Opm::RestartIO::ecl_kw_free( logihead_kw );
  }


  {
    ::Opm::RestartIO::ecl_kw_type * doubhead_kw = ::Opm::RestartIO::ecl_rst_file_alloc_DOUBHEAD( rst_file , rsthead_data->sim_days );
    ::Opm::RestartIO::ecl_kw_fwrite( doubhead_kw , rst_file->fortio );
    ::Opm::RestartIO::ecl_kw_free( doubhead_kw );
  }
}

void ecl_rst_file_start_solution( Opm::RestartIO::ecl_rst_file_type * rst_file ) {
  ::Opm::RestartIO::ecl_kw_type * startsol_kw = ::Opm::RestartIO::ecl_kw_alloc( STARTSOL_KW , 0 , ECL_MESS_2 );
  ::Opm::RestartIO::ecl_kw_fwrite( startsol_kw , rst_file->fortio );
  ::Opm::RestartIO::ecl_kw_free( startsol_kw );
}

void ecl_rst_file_end_solution( ::Opm::RestartIO::ecl_rst_file_type * rst_file ) {
  ::Opm::RestartIO::ecl_kw_type * endsol_kw = ::Opm::RestartIO::ecl_kw_alloc( ENDSOL_KW , 0 , ECL_MESS_2 );
  ::Opm::RestartIO::ecl_kw_fwrite( endsol_kw , rst_file->fortio );
  ::Opm::RestartIO::ecl_kw_free( endsol_kw );
}




void * ecl_kw_iget_ptr(const ::Opm::RestartIO::ecl_kw_type *ecl_kw , int i) {
  return ::Opm::RestartIO::ecl_kw_iget_ptr_static(ecl_kw , i);
}


/**
   This will set the elemnts of the ecl_kw data storage in index to
   the value of s8; if s8 is shorter than 8 characters the result will
   be padded, if s8 is longer than 8 characters the characters from 9
   and out will be ignored.
*/
void ecl_kw_iset_string8(::Opm::RestartIO::ecl_kw_type * ecl_kw , int index , const char *s8) {
  char * ecl_string = (char *) ::Opm::RestartIO::ecl_kw_iget_ptr( ecl_kw , index );
  if (strlen( s8 ) >= ECL_STRING8_LENGTH) {
    /* The whole string goes in - possibly loosing content at the end. */
    int i;
    for (i=0; i < ECL_STRING8_LENGTH; i++)
      ecl_string[i] = s8[i];
  } else {
    /* The string is padded with trailing spaces. */
    int string_length = strlen( s8 );
    int i;

    for (i=0; i < string_length; i++)
      ecl_string[i] = s8[i];

    for (i=string_length; i < ECL_STRING8_LENGTH; i++)
      ecl_string[i] = ' ';

  }

  ecl_string[ ECL_STRING8_LENGTH ] = '\0';
}


const char * ecl_kw_iget_char_ptr( const ::Opm::RestartIO::ecl_kw_type * ecl_kw , int i) {
  if (::Opm::RestartIO::ecl_kw_get_type(ecl_kw) != ECL_CHAR_TYPE)
    util_abort("%s: Keyword: %s is wrong type - aborting \n",__func__ , ::Opm::RestartIO::ecl_kw_get_header8(ecl_kw));
  return static_cast<char *>(::Opm::RestartIO::ecl_kw_iget_ptr( ecl_kw , i ));
}


/**
   This file allocates a filename consisting of a leading path, a
   basename and an extension. Both the path and the extension can be
   NULL, but not the basename.

   Observe that this function does pure string manipulation; there is
   no input check on whether path exists, if baseneme contains "."
   (or even a '/') and so on.
*/

char * util_alloc_filename(const char * path , const char * basename , const char * extension) {
  char * file;
  int    length = strlen(basename) + 1;

  if (path != NULL)
    length += strlen(path) + 1;

  if (extension != NULL)
    length += strlen(extension) + 1;

  file = (char*)util_calloc(length , sizeof * file );

  if (path == NULL) {
    if (extension == NULL)
      memcpy(file , basename , strlen(basename) + 1);
    else
      sprintf(file , "%s.%s" , basename , extension);
  } else {
    if (extension == NULL)
      sprintf(file , "%s%c%s" , path , UTIL_PATH_SEP_CHAR , basename);
    else
      sprintf(file , "%s%c%s.%s" , path , UTIL_PATH_SEP_CHAR , basename , extension);
  }

  return file;
}


/**
   This function takes a path, along with a filetype as input and
   allocates a new string with the filename. If path == NULL, the
   filename is allocated without a leading path component.

   If the flag 'must_exist' is set to true the function will check
   with the filesystem if the file actually exists; if the file does
   not exist NULL is returned.
*/

static char * ecl_util_alloc_filename_static(const char * path, const char * base , ::Opm::RestartIO::ecl_file_enum file_type , bool fmt_file, int report_nr, bool must_exist) {
  char * filename;
  char * ext;
  switch (file_type) {
  case(ECL_RESTART_FILE):
    if (fmt_file)
      ext = ::Opm::RestartIO::util_alloc_sprintf("F%04d" , report_nr);
    else
      ext = ::Opm::RestartIO::util_alloc_sprintf("X%04d" , report_nr);
    break;

  case(ECL_UNIFIED_RESTART_FILE):
    if (fmt_file)
      ext = ::Opm::RestartIO::util_alloc_string_copy("FUNRST");
    else
      ext = ::Opm::RestartIO::util_alloc_string_copy("UNRST");
    break;

  case(ECL_SUMMARY_FILE):
    if (fmt_file)
      ext = ::Opm::RestartIO::util_alloc_sprintf("A%04d" , report_nr);
    else
      ext = ::Opm::RestartIO::util_alloc_sprintf("S%04d" , report_nr);
    break;

  case(ECL_UNIFIED_SUMMARY_FILE):
    if (fmt_file)
      ext = ::Opm::RestartIO::util_alloc_string_copy("FUNSMRY");
    else
      ext = ::Opm::RestartIO::util_alloc_string_copy("UNSMRY");
    break;

  case(ECL_SUMMARY_HEADER_FILE):
    if (fmt_file)
      ext = ::Opm::RestartIO::util_alloc_string_copy("FSMSPEC");
    else
      ext = ::Opm::RestartIO::util_alloc_string_copy("SMSPEC");
    break;

  case(ECL_GRID_FILE):
    if (fmt_file)
      ext = ::Opm::RestartIO::util_alloc_string_copy("FGRID");
    else
      ext = ::Opm::RestartIO::util_alloc_string_copy("GRID");
    break;

  case(ECL_EGRID_FILE):
    if (fmt_file)
      ext = ::Opm::RestartIO::util_alloc_string_copy("FEGRID");
    else
      ext = ::Opm::RestartIO::util_alloc_string_copy("EGRID");
    break;

  case(ECL_INIT_FILE):
    if (fmt_file)
      ext = ::Opm::RestartIO::util_alloc_string_copy("FINIT");
    else
      ext = ::Opm::RestartIO::util_alloc_string_copy("INIT");
    break;

  case(ECL_RFT_FILE):
    if (fmt_file)
      ext = ::Opm::RestartIO::util_alloc_string_copy("FRFT");
    else
      ext = ::Opm::RestartIO::util_alloc_string_copy("RFT");
    break;

  case(ECL_DATA_FILE):
    ext = ::Opm::RestartIO::util_alloc_string_copy("DATA");
    break;

  default:
    util_abort("%s: Invalid input file_type to ecl_util_alloc_filename - aborting \n",__func__);
    /* Dummy to shut up compiler */
    ext        = NULL;
  }

  filename = util_alloc_filename(path , base , ext);
  free(ext);

  if (must_exist) {
    if (!util_file_exists( filename )) {
      free(filename);
      filename = NULL;
    }
  }

  return filename;
}


char * ecl_util_alloc_filename(const char * path, const char * base , ::Opm::RestartIO::ecl_file_enum file_type , bool fmt_file, int report_nr) {
  return ::Opm::RestartIO::ecl_util_alloc_filename_static(path , base , file_type ,fmt_file , report_nr , false);
}


std::string EclFilename( const std::string& path, const std::string& base, ::Opm::RestartIO::ecl_file_enum file_type , int report_step, bool fmt_file) {
    char* tmp = ::Opm::RestartIO::ecl_util_alloc_filename( path.c_str(), base.c_str(), file_type, fmt_file , report_step );
    std::string retval = tmp;
    free(tmp);
    return retval;
}

std::string EclFilename( const std::string& base, ::Opm::RestartIO::ecl_file_enum file_type , int report_step, bool fmt_file) {
    char* tmp = ::Opm::RestartIO::ecl_util_alloc_filename( nullptr, base.c_str(), file_type, fmt_file , report_step );
    std::string retval = tmp;
    free(tmp);
    return retval;
}

//namespace {
    bool require_report_step( ::Opm::RestartIO::ecl_file_enum file_type ) {
        if ((file_type == ECL_RESTART_FILE) || (file_type == ECL_SUMMARY_FILE))
            return true;
        else
            return false;
    }
//}

std::string EclFilename( const std::string& path, const std::string& base, ::Opm::RestartIO::ecl_file_enum file_type , bool fmt_file) {
    if (::Opm::RestartIO::require_report_step( file_type ))
        throw std::runtime_error("Must use overload with report step for this file type");
    else {
        char* tmp = ::Opm::RestartIO::ecl_util_alloc_filename( path.c_str(), base.c_str(), file_type, fmt_file , -1);
        std::string retval = tmp;
        free(tmp);
        return retval;
    }
}


std::string EclFilename( const std::string& base, ::Opm::RestartIO::ecl_file_enum file_type , bool fmt_file) {
    if (::Opm::RestartIO::require_report_step( file_type ))
        throw std::runtime_error("Must use overload with report step for this file type");
    else {
        char* tmp = ::Opm::RestartIO::ecl_util_alloc_filename( nullptr , base.c_str(), file_type, fmt_file , -1);
        std::string retval = tmp;
        free(tmp);
        return retval;
    }
}


::Opm::RestartIO::ecl_file_enum EclFiletype(const std::string& filename) {
    return ::Opm::RestartIO::ecl_util_get_file_type( filename.c_str(), nullptr, nullptr );
}

static bool is_ecl_string_name(const char * type_name) {
  return (type_name[0] == 'C'   &&
          isdigit(type_name[1]) &&
          isdigit(type_name[2]) &&
          isdigit(type_name[3])
          );
}

static size_t get_ecl_string_length(const char * type_name) {
  if(!::Opm::RestartIO::is_ecl_string_name(type_name))
    util_abort("%s: Expected eclipse string (CXXX), received %s\n",
               __func__, type_name);

  return atoi(type_name+1);
}


::Opm::RestartIO::ecl_data_type ecl_type_create_from_name( const char * type_name ) {
  if (strncmp( type_name , ECL_TYPE_NAME_FLOAT , ECL_TYPE_LENGTH) == 0)
    return ECL_FLOAT;
  else if (strncmp( type_name , ECL_TYPE_NAME_INT , ECL_TYPE_LENGTH) == 0)
    return ECL_INT_2;
  else if (strncmp( type_name , ECL_TYPE_NAME_DOUBLE , ECL_TYPE_LENGTH) == 0)
    return ECL_DOUBLE;
  else if (strncmp( type_name , ECL_TYPE_NAME_CHAR , ECL_TYPE_LENGTH) == 0)
    return ECL_CHAR;
  else if (::Opm::RestartIO::is_ecl_string_name(type_name))
    return ECL_STRING(::Opm::RestartIO::get_ecl_string_length(type_name));
  else if (strncmp( type_name , ECL_TYPE_NAME_MESSAGE , ECL_TYPE_LENGTH) == 0)
    return ECL_MESS_2;
  else if (strncmp( type_name , ECL_TYPE_NAME_BOOL , ECL_TYPE_LENGTH) == 0)
    return ECL_BOOL;
  else {
    util_abort("%s: unrecognized type name:%s \n",__func__ , type_name);
    return ECL_INT_2; /* Dummy */
  }
}


::Opm::RestartIO::ecl_read_status_enum ecl_kw_fread_header(::Opm::RestartIO::ecl_kw_type *ecl_kw , fortio_type * fortio) {
  const char null_char = '\0';
  FILE *stream  = fortio_get_FILE( fortio );
  bool fmt_file = fortio_fmt_file( fortio );
  char header[ECL_STRING8_LENGTH + 1];
  char ecl_type_str[ECL_TYPE_LENGTH + 1];
  int record_size;
  int size;

  if (fmt_file) {
    if(!::Opm::RestartIO::ecl_kw_fscanf_qstring(header , "%8c" , 8 , stream))
      return ECL_KW_READ_FAIL;

    int read_count = fscanf(stream , "%d" , &size);
    if (read_count != 1)
      return ECL_KW_READ_FAIL;

    if (!::Opm::RestartIO::ecl_kw_fscanf_qstring(ecl_type_str , "%4c" , 4 , stream))
      return ECL_KW_READ_FAIL;

    fgetc(stream);             /* Reading the trailing newline ... */
  }
  else {
    header[ECL_STRING8_LENGTH]    = null_char;
    ecl_type_str[ECL_TYPE_LENGTH] = null_char;
    record_size = fortio_init_read(fortio);

    if (record_size <= 0)
      return ECL_KW_READ_FAIL;

    char buffer[ECL_KW_HEADER_DATA_SIZE];
    size_t read_bytes = fread(buffer , 1 , ECL_KW_HEADER_DATA_SIZE , stream);

    if (read_bytes != ECL_KW_HEADER_DATA_SIZE)
      return ECL_KW_READ_FAIL;

    memcpy( header , &buffer[0] , ECL_STRING8_LENGTH);
    size = * ( (int *) &buffer[ECL_STRING8_LENGTH] );
    memcpy( ecl_type_str , &buffer[ECL_STRING8_LENGTH + sizeof(size)] , ECL_TYPE_LENGTH);

    if(!fortio_complete_read(fortio , record_size))
      return ECL_KW_READ_FAIL;

    if (ECL_ENDIAN_FLIP)
      ::Opm::RestartIO::util_endian_flip_vector(&size , sizeof size , 1);
  }

  ::Opm::RestartIO::ecl_data_type data_type = ::Opm::RestartIO::ecl_type_create_from_name( ecl_type_str );
  ecl_kw_initialize( ecl_kw , header , size , data_type);

  return ECL_KW_READ_OK;
}


void ecl_kw_free__(void *void_ecl_kw) {
  ::Opm::RestartIO::ecl_kw_free((::Opm::RestartIO::ecl_kw_type *) void_ecl_kw);
}

bool ecl_kw_size_and_type_equal( const ::Opm::RestartIO::ecl_kw_type *ecl_kw1 , const ::Opm::RestartIO::ecl_kw_type * ecl_kw2 ) {
  return (ecl_kw1->size == ecl_kw2->size &&
          ::Opm::RestartIO::ecl_type_is_equal(ecl_kw1->data_type, ecl_kw2->data_type));
}


void ecl_kw_memcpy_data( ::Opm::RestartIO::ecl_kw_type * target , const ::Opm::RestartIO::ecl_kw_type * src) {
  if (!::Opm::RestartIO::ecl_kw_size_and_type_equal( target , src ))
    util_abort("%s: type/size mismatch \n",__func__);

  memcpy(target->data , src->data , target->size * ::Opm::RestartIO::ecl_kw_get_sizeof_ctype(target));
}



void ecl_kw_memcpy(::Opm::RestartIO::ecl_kw_type *target, const ::Opm::RestartIO::ecl_kw_type *src) {
  target->size                = src->size;
  ::Opm::RestartIO::ecl_kw_set_data_type(target, src->data_type);
  ::Opm::RestartIO::ecl_kw_set_header_name( target , src->header );
  ::Opm::RestartIO::ecl_kw_alloc_data(target);
  ::Opm::RestartIO::ecl_kw_memcpy_data( target , src );
}


::Opm::RestartIO::ecl_kw_type *ecl_kw_alloc_copy(const ::Opm::RestartIO::ecl_kw_type *src) {
  ::Opm::RestartIO::ecl_kw_type *new_1;
  new_1 = ::Opm::RestartIO::ecl_kw_alloc_empty();
  ::Opm::RestartIO::ecl_kw_memcpy(new_1 , src);
  return new_1;
}

void * ecl_kw_get_data_ref(const ::Opm::RestartIO::ecl_kw_type *ecl_kw) {
  return ecl_kw->data;
}

void * ecl_kw_get_ptr(const ::Opm::RestartIO::ecl_kw_type *ecl_kw) {
  return ::Opm::RestartIO::ecl_kw_get_data_ref( ecl_kw );
}

::Opm::RestartIO::ecl_data_type ecl_type_create_from_type(const ::Opm::RestartIO::ecl_type_enum type) {
  switch(type) {
  case(ECL_CHAR_TYPE):
    return ECL_CHAR;
  case(ECL_INT_TYPE):
    return ECL_INT_2;
  case(ECL_FLOAT_TYPE):
    return ECL_FLOAT;
  case(ECL_DOUBLE_TYPE):
    return ECL_DOUBLE;
  case(ECL_BOOL_TYPE):
    return ECL_BOOL;
  case(ECL_MESS_TYPE):
    return ECL_MESS_2;
  case(ECL_STRING_TYPE):
    util_abort("%s: Variable length string type cannot be created"
            " from type alone!\n" , __func__);
    return ECL_STRING(0); /* Dummy */
  default:
    util_abort("%s: invalid ecl_type: %d\n", __func__, type);
    return ECL_INT_2; /* Dummy */
  }
}

offset_type ecl_rst_file_ftell(const ::Opm::RestartIO::ecl_rst_file_type * rst_file ) {
  return fortio_ftell( rst_file->fortio );
}


}
}

