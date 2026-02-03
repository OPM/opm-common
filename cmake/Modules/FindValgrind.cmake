# Find Valgrind.
#
# This module defines:
#  VALGRIND_INCLUDE_DIR, where to find valgrind/memcheck.h, etc.
#  VALGRIND_PROGRAM, the valgrind executable.
#  Valgrind_FOUND, If false, do not try to use valgrind.
#
# If you have valgrind installed in a non-standard place, you can define
# VALGRIND_ROOT to tell cmake where it is.

find_path(VALGRIND_INCLUDE_DIR
  NAME
    valgrind/memcheck.h
   HINTS
      ${VALGRIND_ROOT}/include
)

# if VALGRIND_ROOT is empty, we explicitly add /bin to the search
# path, but this does not hurt...
find_program(VALGRIND_PROGRAM
  NAMES
    valgrind
  HINTS
    ${VALGRIND_ROOT}/bin
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Valgrind DEFAULT_MSG
  VALGRIND_INCLUDE_DIR
  VALGRIND_PROGRAM)

mark_as_advanced(VALGRIND_ROOT VALGRIND_INCLUDE_DIR VALGRIND_PROGRAM)

if(VALGRIND_FOUND AND NOT TARGET Valgrind::Valgrind)
  add_library(Valgrind::Valgrind INTERFACE IMPORTED)
  target_include_directories(Valgrind::Valgrind INTERFACE ${VALGRIND_INCLUDE_DIR})
endif()
if(VALGRIND_PROGRAM AND NOT TARGET Valgrind::Command)
  add_executable(Valgrind::Command IMPORTED GLOBAL)
  set_target_properties(Valgrind::Command PROPERTIES IMPORTED_LOCATION ${VALGRIND_PROGRAM})
endif()
