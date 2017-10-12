# -*- mode: cmake; tab-width: 2; indent-tabs-mode: t; truncate-lines: t
# vim: set filetype=cmake autoindent tabstop=2 shiftwidth=2 noexpandtab softtabstop=2 nowrap:

# set up project and specify the minimum cmake version
project("opm-common" C CXX)

cmake_minimum_required(VERSION 3.0)

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
opm_add_test(test_messagelimiter LIBRARIES "${Boost_LIBRARIES}" INCLUDE_DIRS "${Boost_INCLUDE_DIRS}")
opm_add_test(test_OpmLog LIBRARIES "${Boost_LIBRARIES}" INCLUDE_DIRS "${Boost_INCLUDE_DIRS}")
opm_add_test(test_param LIBRARIES "${Boost_LIBRARIES}" INCLUDE_DIRS "${Boost_INCLUDE_DIRS}")
opm_add_test(test_SimulationDataContainer LIBRARIES "${Boost_LIBRARIES}" INCLUDE_DIRS "${Boost_INCLUDE_DIRS}")

opm_recusive_copy_testdata("tests/*.param")

# finalize the dune project, e.g. generating config.h etc.
finalize_dune_project(GENERATE_CONFIG_H_CMAKE)
