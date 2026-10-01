# - Find DUNE common library
#
# Defines the following variables:
#   dune-common_INCLUDE_DIRS    Directory of header files
#   dune-common_LIBRARIES       Directory of shared object files
#   dune-common_DEFINITIONS     Defines that must be set to compile
#   dune-common_CONFIG_VARS     List of defines that should be in config.h
#   HAVE_DUNE_COMMON            Binary value to use in config.h

# Copyright (C) 2012 Uni Research AS
# This code is licensed under The GNU General Public License v3.0
if(dune-common_FOUND)
  return()
endif()

if(dune-common_FIND_REQUIRED)
  find_package(dune-common CONFIG REQUIRED)
else()
  find_package(dune-common CONFIG)
endif()

if(dune-common_FOUND)
  # make version number available in config.h
  include (UseDuneVer)
  find_dune_version ("dune" "common")
  target_compile_definitions(Dune::Common INTERFACE HAVE_DUNE_COMMON=1)
endif()
