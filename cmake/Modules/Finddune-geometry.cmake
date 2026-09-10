# - Find DUNE geometry library
#
# Defines the following variables:
#   dune-geometry_INCLUDE_DIRS    Directory of header files
#   dune-geometry_LIBRARIES       Directory of shared object files
#   dune-geometry_DEFINITIONS     Defines that must be set to compile
#   dune-geometry_CONFIG_VARS     List of defines that should be in config.h
#   HAVE_DUNE_GEOMETRY            Binary value to use in config.h

# Copyright (C) 2013 Uni Research AS
# This code is licensed under The GNU General Public License v3.0
if(dune-geometry_FOUND)
  return()
endif()

if(dune-geometry_FIND_REQUIRED)
  find_package(dune-geometry CONFIG REQUIRED)
else()
  find_package(dune-geometry CONFIG)
endif()

if(dune-geometry_FOUND)
  target_compile_definitions(Dune::Geometry INTERFACE HAVE_DUNE_GEOMETRY=1)
  # make version number available in config.h
  include (UseDuneVer)
  find_dune_version ("dune" "geometry")
endif()
