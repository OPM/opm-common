# Look for the cjson library; will probably newer be found.
# If found, it sets these variables:
#
#       CJSON_INCLUDE_DIRS      Header file directories
#       CJSON_LIBRARRY          Archive/shared objects

if (CJSON_ROOT)
  set (_no_default_path "NO_DEFAULT_PATH")
else (CJSON_ROOT)
  set (_no_default_path "")
endif (CJSON_ROOT)


find_path (CJSON_INCLUDE_DIR
  NAMES "cjson/cJSON.h"
  HINTS "${CJSON_ROOT}"
  PATH_SUFFIXES "include"
  DOC "Path to cjson library header files"
  ${_no_default_path} )


find_library (CJSON_LIBRARY
  NAMES "cjson"
  HINTS "${CJSON_ROOT}"
  PATH_SUFFIXES "lib" "lib${_BITS}" "lib/${CMAKE_LIBRARY_ARCHITECTURE}"
  DOC "Path to cjson library archive/shared object files"
  ${_no_default_path} )



# see if we can compile a minimum example
# CMake logical test doesn't handle lists (sic)
if (NOT (CJSON_INCLUDE_DIR MATCHES "-NOTFOUND" OR CJSON_LIBRARY MATCHES "-NOTFOUND"))
  include (CMakePushCheckState)
  include (CheckCSourceCompiles)
  cmake_push_check_state ()
  set (CMAKE_REQUIRED_INCLUDES ${CJSON_INCLUDE_DIR})
  set (CMAKE_REQUIRED_LIBRARIES ${CJSON_LIBRARY})

  check_c_source_compiles (
"#include <stdlib.h>
#include <cjson/cJSON.h>
int main (void) {
    cJSON root;
  return 0;
}"  HAVE_CJSON)
  cmake_pop_check_state ()
else ()
  # clear the cache so the find probe is attempted again if files becomes
  # available (only upon a unsuccessful *compile* should we disable further
  # probing)
  set (HAVE_CJSON)
  unset (HAVE_CJSON CACHE)
endif ()

