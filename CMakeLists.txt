# -*- mode: cmake; tab-width: 2; indent-tabs-mode: t; truncate-lines: t
# vim: set filetype=cmake autoindent tabstop=2 shiftwidth=2 noexpandtab softtabstop=2 nowrap:

# specify the minimum cmake version
cmake_minimum_required(VERSION 3.0)

# specify the option that allows to select the build system. if
# dunecontrol is used, we default to the dune build system. if not, we
# use the OPM home-brewn one
set(USE_DUNE_BUILDSYSTEM_DEFAULT OFF)
if (DEFINED ENV{DUNE_CONTROL_PATH})
  set(USE_DUNE_BUILDSYSTEM_DEFAULT ON)
endif()
option(USE_DUNE_BUILDSYSTEM "Use the Dune build system?" ${USE_DUNE_BUILDSYSTEM_DEFAULT})

if (USE_DUNE_BUILDSYSTEM)

######
# use DUNE's build system
######

# set up project
project("opm-common" C CXX)

# Set CMP0053 (how to handle escape sequences in strings) to the new
# behavior to avoid a pretty annoying cmake warning if a library is
# defined in the toplevel CMakeLists.txt. This should probably be
# considered to be a bug in the dune build system. Note that the old
# behaviour will most likely also work fine, but the result of setting
# this policy to NEW is most likely what's intended.
if (POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW)
endif()

# find the build system (i.e., dune-common) and set cmake's module path
find_package(dune-common REQUIRED)
list(APPEND CMAKE_MODULE_PATH ${dune-common_MODULE_PATH}
  "${PROJECT_SOURCE_DIR}/cmake/modules")

# include the dune macros
include(DuneMacros)

# include the OPM cmake macros
include(OpmMacros)

# find the packages needed to compile the module
find_package(Boost COMPONENTS unit_test_framework  REQUIRED)

# start a dune project with information from dune.module
dune_project()

# recursively mark all header files beneath the "opm" directory for
# installation.
opm_recusive_export_all_headers("opm")

# the cmake modules get a special treatment
opm_export_cmake_modules()

# we want all features detected by the build system to be enabled,
# thank you!
dune_enable_all_packages()

# add libopmcommon.a and specify that it encompasses all source files
# which are located somewhere beneath the "opm" directory. Note that
# if the tests and the source files of the library were intermingled,
# this would be not so easy, i.e. it would require a separate
# CMakeLists.txt file in each subdirectory and a call to
# add_subdirectory() for each of them.
opm_recursive_add_library("opmcommon" "opm")

# add all unit tests
opm_add_test(test_cmp LIBRARIES "${Boost_LIBRARIES}" INCLUDE_DIRS "${Boost_INCLUDE_DIRS}")
opm_add_test(test_ConditionalStorage LIBRARIES "${Boost_LIBRARIES}" INCLUDE_DIRS "${Boost_INCLUDE_DIRS}")
opm_add_test(test_messagelimiter LIBRARIES "${Boost_LIBRARIES}" INCLUDE_DIRS "${Boost_INCLUDE_DIRS}")
opm_add_test(test_OpmLog LIBRARIES "${Boost_LIBRARIES}" INCLUDE_DIRS "${Boost_INCLUDE_DIRS}")
opm_add_test(test_param LIBRARIES "${Boost_LIBRARIES}" INCLUDE_DIRS "${Boost_INCLUDE_DIRS}")
opm_add_test(test_SimulationDataContainer LIBRARIES "${Boost_LIBRARIES}" INCLUDE_DIRS "${Boost_INCLUDE_DIRS}")

opm_recusive_copy_testdata("tests/*.param")

# finalize the dune project, e.g. generating config.h etc.
finalize_dune_project(GENERATE_CONFIG_H_CMAKE)

else()

######
# use the traditional OPM build system
######

# set up project
project("opm-common" CXX)

option(SIBLING_SEARCH "Search for other modules in sibling directories?" ON)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)
set(OPM_MACROS_ROOT ${PROJECT_SOURCE_DIR})

# not the same location as most of the other projects; this hook overrides
macro (dir_hook)
endmacro (dir_hook)

# We need to define this variable in the installed cmake config file.
set(OPM_PROJECT_EXTRA_CODE_INSTALLED  "set(OPM_MACROS_ROOT ${CMAKE_INSTALL_PREFIX}/share/opm)
                                       list(APPEND CMAKE_MODULE_PATH \${OPM_MACROS_ROOT}/cmake/Modules)")

set(OPM_PROJECT_EXTRA_CODE_INTREE  "set(OPM_MACROS_ROOT ${OPM_MACROS_ROOT})
                                    list(APPEND CMAKE_MODULE_PATH \${OPM_MACROS_ROOT}/cmake/Modules)")

# project information is in dune.module. Read this file and set variables.
# we cannot generate dune.module since it is read by dunecontrol before
# the build starts, so it makes sense to keep the data there then.
include (OpmInit)

# list of prerequisites for this particular project; this is in a
# separate file (in cmake/Modules sub-directory) because it is shared
# with the find module
include (${project}-prereqs)

# read the list of components from this file (in the project directory);
# it should set various lists with the names of the files to include
include (CMakeLists_files.cmake)

macro (config_hook)
endmacro (config_hook)

macro (prereqs_hook)
endmacro (prereqs_hook)

macro (sources_hook)
endmacro (sources_hook)

macro (fortran_hook)
endmacro (fortran_hook)

macro (files_hook)
endmacro (files_hook)

macro (tests_hook)
endmacro (tests_hook)

# all setup common to the OPM library modules is done here
include (OpmLibMain)

# Install build system files
install(DIRECTORY cmake DESTINATION share/opm)

endif()
