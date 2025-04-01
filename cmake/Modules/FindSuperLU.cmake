#
# Module that checks whether SuperLU is available and usable.
# SuperLU must be a version released after the year 2005.
#
# Variables used by this module which you may want to set:
# SUPERLU_ROOT                Path list to search for SuperLU
#
# Sets the follwing variable:
#
# SUPERLU_FOUND               True if SuperLU available and usable.
# SUPERLU_INCLUDE_DIRS        Path to the SuperLU include dirs.
# SUPERLU_LIBRARIES           Name to the SuperLU library.
#

if(NOT USE_SUPERLU)
  return()
endif()

include(CheckIncludeFiles)
include(CMakePushCheckState)
include(CheckCSourceCompiles)

cmake_push_check_state()

set(SUPERLU_FOUND "FALSE")

# find out the size of a pointer. this is required to only search for
# libraries in the directories relevant for the architecture
if (CMAKE_SIZEOF_VOID_P)
  math (EXPR _BITS "8 * ${CMAKE_SIZEOF_VOID_P}")
endif (CMAKE_SIZEOF_VOID_P)

# look for files only at the positions given by the user if
# an explicit path is specified
if(SUPERLU_ROOT)
  set (_no_default_path "NO_DEFAULT_PATH")
else()
  set (_no_default_path "")
endif()

# look for a system-wide BLAS library
find_package(BLAS QUIET)

# look for the internal SuperLU blas library (but only if no
# system-wide library was found and a path to the superLU library was
# specified)
set(SUPERLU_BLAS_LIBRARY "")
if(NOT BLAS_FOUND AND SUPERLU_ROOT)
  find_library(SUPERLU_BLAS_LIBRARY
    NAMES "blas"
    PATHS ${SUPERLU_ROOT}
    PATH_SUFFIXES "lib" "lib${_BITS}" "lib/${CMAKE_LIBRARY_ARCHITECTURE}"
    NO_DEFAULT_PATH)
endif()

# print message if there was still no blas found!
if(NOT BLAS_FOUND AND NOT SUPERLU_BLAS_LIBRARY)
  message(SEND_ERROR "BLAS not found but required for SuperLU")
  return()
endif()
list(APPEND CMAKE_REQUIRED_LIBRARIES "${SUPERLU_BLAS_LIBRARY}")

# find the directory containing the SuperLU include files
if (NOT SUPERLU_INCLUDE_DIR)
  find_path(SUPERLU_INCLUDE_DIR
    NAMES "supermatrix.h"
    PATHS ${SUPERLU_ROOT}
    PATH_SUFFIXES "superlu" "include/superlu" "include" "SRC"
    ${_no_default_path}
    )
endif()
if(NOT SUPERLU_INCLUDE_DIR)
  message(STATUS "Directory with the SuperLU include files not found")
  return()
endif()
#list(APPEND CMAKE_REQUIRED_INCLUDES "${SUPERLU_INCLUDE_DIR}")

# look for actual SuperLU library
if (NOT SUPERLU_LIBRARY)
  find_library(SUPERLU_LIBRARY
    NAMES "superlu" # skipping "superlu_5.2.1" "superlu_5.2" "superlu_5.1.1" "superlu_5.1" "superlu_5.0"
    PATHS ${SUPERLU_ROOT}
    PATH_SUFFIXES "lib" "lib${_BITS}" "lib/${CMAKE_LIBRARY_ARCHITECTURE}"
    ${_no_default_path}
    )
endif()
if(NOT SUPERLU_LIBRARY)
  message(STATUS "Directory with the SuperLU library not found")
  return()
endif()
list(APPEND CMAKE_REQUIRED_LIBRARIES "${SUPERLU_LIBRARY}")

cmake_pop_check_state()

# behave like a CMake module is supposed to behave
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  "SuperLU"
  DEFAULT_MSG
  SUPERLU_INCLUDE_DIR
  SUPERLU_LIBRARY)

mark_as_advanced(SUPERLU_INCLUDE_DIR SUPERLU_LIBRARY)

# if both headers and library are found, store results
if(SUPERLU_FOUND)
  set(SUPERLU_INT_TYPE int)
  if (SUPERLU_BLAS_LIBRARY)
    list(APPEND SUPERLU_LIBRARIES ${SUPERLU_BLAS_LIBRARY})
  endif()
  if(NOT TARGET SuperLU::SuperLU)
    add_library(SuperLU::SuperLU UNKNOWN IMPORTED GLOBAL)
    set_target_properties(SuperLU::SuperLU PROPERTIES
      IMPORTED_LOCATION ${SUPERLU_LIBRARY}
      INCLUDE_DIRECTORIES ${SUPERLU_INCLUDE_DIR})
    if(TARGET BLAS::BLAS)
      set_property(TARGET SuperLU::SuperLU PROPERTY
        INTERFACE_LINK_LIBRARIES BLAS::BLAS)
    else()
      set_property(TARGET SuperLU::SuperLU PROPERTY
	INTERFACE_LINK_LIBRARIES ${SUPERLU_BLAS_LIBRARY})
    endif()
  endif()
endif()

cmake_pop_check_state()
