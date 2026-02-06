# - Initialize project-specific variables
#
# This will read the dune.module file for project information and
# set the following variables:
#
#	project                     From the Module: field
#	${project}_NAME             Same as above
#	${project}_DESCRIPTION      From the Description: field
#	${project}_VERSION          From the Version: field
#	${project}_VERSION_MAJOR    From the Version: field
#	${project}_VERSION_MINOR    From the Version: field also
#
# This module should be the first to be included in the project,
# because most of the others (OpmXxx.cmake) use these variables.

# for CMake >= 3.0, we need to change a few policies:
#
#   - CMP0026 to allow access to the LOCATION target property
#   - CMP0048 to indicate that we want to deal with the *VERSION*
#     variables ourselves
#   - CMP0064 to indicate that we want TEST if conditions to be evaluated
#   - CMP0074 to indicate that <PackageName>_ROOT can be used to find package
#             config files
macro(OpmSetPolicies)
  if (POLICY CMP0048)
    # We do not set version. Hence NEW should work and this can be removed later
    cmake_policy(SET CMP0048 NEW)
  endif()

  if(POLICY CMP0064)
    cmake_policy(SET CMP0064 NEW)
  endif()

  # set the behavior of the policy 0054 to NEW. (i.e. do not implicitly
  # expand variables in if statements)
  if (POLICY CMP0054)
    cmake_policy(SET CMP0054 NEW)
  endif()

  # set the behavior of policy 0074 to new as we always used <PackageName>_ROOT as the
  # root of the installation
  if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
  endif()

  # de-duplicates libraries for capable linkers (Apple ld64, lld, MSVC)
  if(POLICY CMP0156)
    cmake_policy(SET CMP0156 NEW)
  endif()
endmacro()


# helper macro to retrieve a single field of a dune.module file
macro(OpmGetDuneModuleDirective field variable contents)
  string (REGEX MATCH ".*${field}:[ ]*([^\n]+).*" ${variable} "${contents}")
  string (REGEX REPLACE ".*${field}:[ ]*([^\n]+).*" "\\1" "${variable}" "${${variable}}")
  string (STRIP "${${variable}}" ${variable})
endmacro()

function (OpmInitProjVars)
  # locate the "dune.module" file
  set (DUNE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/dune.module")

  # read this file into a variable
  file (READ "${DUNE_MODULE_PATH}" DUNE_MODULE)

  # read fields from the file
  OpmGetDuneModuleDirective ("Module" project "${DUNE_MODULE}")
  OpmGetDuneModuleDirective ("Description" description "${DUNE_MODULE}")
  OpmGetDuneModuleDirective ("Version" version "${DUNE_MODULE}")
  OpmGetDuneModuleDirective ("Label" label "${DUNE_MODULE}")

  # parse the version number
  set (verno_regex "^([0-9]*)\\.([0-9]*).*\$")
  string (REGEX REPLACE "${verno_regex}" "\\1" major "${version}")
  string (REGEX REPLACE "${verno_regex}" "\\2" minor "${version}")

  # return these variables
  set (project "${project}" PARENT_SCOPE)
  set (${project}_NAME "${project}" PARENT_SCOPE)
  set (${project}_DESCRIPTION "${description}" PARENT_SCOPE)
  set (${project}_VERSION_MAJOR "${major}" PARENT_SCOPE)
  set (${project}_VERSION_MINOR "${minor}" PARENT_SCOPE)
  set (${project}_VERSION "${major}.${minor}" PARENT_SCOPE)
  set (${project}_LABEL "${label}" PARENT_SCOPE)
endfunction ()

macro (OpmInitDirVars)
  # these are the most common (and desired locations)
  set (${project}_DIR "opm")
  set (doxy_dir "doc/doxygen")

  # but for backward compatibility we can override it
  if (COMMAND dir_hook)
	dir_hook ()
  endif (COMMAND dir_hook)
endmacro ()

if("${CMAKE_SIZEOF_VOID_P}" LESS 8)
  message(FATAL_ERROR "OPM will only work correctly on 64bit (or higher) systems!")
endif()

OpmInitProjVars ()
OpmInitDirVars ()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})
include(OpmPackage)

set (${project}_SUITE "opm")

# parallel computing must be explicitly enabled
# This needs to be in OpmInit as prereqs is called before OpmLibMain is included.
option (USE_MPI "Use Message Passing Interface for parallel computing" ON)
if (NOT USE_MPI)
  set (CMAKE_DISABLE_FIND_PACKAGE_MPI TRUE)
endif ()

# Compiler standard version needs to be requested here as prereqs is included
# before OpmLibMain and some tests need/use CXX_STANDARD_VERSION (e.g. pybind11)
# Languages and global compiler settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option (USE_SUPERLU "Use SuperLU direct solvers for AMG (if umfpack is not found)" ON)
if (NOT USE_SUPERLU)
  set (CMAKE_DISABLE_FIND_PACKAGE_SuperLU TRUE)
endif ()

include(UseHDF5)
