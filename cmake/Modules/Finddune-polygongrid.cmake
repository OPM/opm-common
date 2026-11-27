# - Find DUNE polygongrid library
#
# Defines the following variables:
#   dune-polygongrid_INCLUDE_DIRS    Directory of header files
#   dune-polygongrid_LIBRARIES       Directory of shared object files
#   dune-polygongrid_DEFINITIONS     Defines that must be set to compile
#   dune-alugrid_CONFIG_VARS     List of defines that should be in config.h
#   HAVE_DUNE_POLYGONGRID            Binary value to use in config.h

# Copyright (C) 2015 IRIS AS
# This code is licensed under The GNU General Public License v3.0
if(dune-polygongrid_FOUND)
  return()
endif()

if(dune-polygongrid_FIND_REQUIRED)
  find_package(dune-polygongrid CONFIG REQUIRED)
else()
  find_package(dune-polygongrid CONFIG)
endif()

if(dune-polygongrid_FOUND)
  target_compile_definitions(Dune::PolygonGrid INTERFACE HAVE_DUNE_POLYGONGRID=1)
  # make version number available in config.h
  include (UseDuneVer)
  find_dune_version ("dune" "polygongrid")
endif()
