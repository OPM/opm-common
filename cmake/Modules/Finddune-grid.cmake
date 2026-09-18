# - Find DUNE grid library
#
# Defines the following variables:
#   dune-grid_INCLUDE_DIRS    Directory of header files
#   dune-grid_LIBRARIES       Directory of shared object files
#   dune-grid_DEFINITIONS     Defines that must be set to compile
#   dune-grid_CONFIG_VARS     List of defines that should be in config.h
#   HAVE_DUNE_GRID            Binary value to use in config.h

# Copyright (C) 2013 Uni Research AS
# This code is licensed under The GNU General Public License v3.0
if(dune-grid_FOUND)
  return()
endif()

if(dune-grid_FIND_REQUIRED)
  find_package(dune-grid CONFIG REQUIRED)
else()
  find_package(dune-grid CONFIG)
endif()

if(dune-grid_FOUND)
  target_compile_definitions(Dune::Grid INTERFACE HAVE_DUNE_GRID=1)
  # make version number available in config.h
  include (UseDuneVer)
  find_dune_version ("dune" "grid")
endif()
