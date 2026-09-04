# - Find DUNE ISTL library
#
# Defines the following variables:
#   dune-istl_INCLUDE_DIRS      Directory of header files
#   dune-istl_LIBRARIES         Directory of shared object files
#   dune-istl_DEFINITIONS       Defines that must be set to compile
#   dune-istl_CONFIG_VARS       List of defines that should be in config.h
#   HAVE_DUNE_ISTL              Binary value to use in config.h

# Copyright (C) 2012 Uni Research AS
# This code is licensed under The GNU General Public License v3.0
if(dune-istl_FOUND)
  return()
endif()

if(dune-istl_FIND_REQUIRED)
  find_package(dune-istl CONFIG REQUIRED)
else()
  find_package(dune-istl CONFIG)
endif()

if(dune-istl_FOUND)
  # make version number available in config.h
  include (UseDuneVer)
  find_dune_version ("dune" "istl")
  target_compile_definitions(Dune::ISTL INTERFACE HAVE_DUNE_ISTL=1)
endif()
