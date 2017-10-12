# .. cmake_module::
#
# This module's content is executed whenever a Dune module requires or
# suggests opm-common!
#


# Check if valgrind library is available or not.
find_package(Valgrind)

# export include flags for valgrind
set(HAVE_VALGRIND "${VALGRIND_FOUND}")
if(VALGRIND_FOUND)
  dune_register_package_flags(
    INCLUDE_DIRS "${VALGRIND_INCLUDE_DIR}")
endif()