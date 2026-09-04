# - Find DUNE localfunctions library
#
# Defines the following variables:
#   dune-localfunctions_INCLUDE_DIRS      Directory of header files
#   dune-localfunctions_LIBRARIES         Directory of shared object files
#   dune-localfunctions_DEFINITIONS       Defines that must be set to compile
#   dune-localfunctions_CONFIG_VARS       List of defines that should be in config.h
#   HAVE_DUNE_LOCALFUNCTIONS              Binary value to use in config.h

# Copyright (C) 2012 Uni Research AS
# This code is licensed under The GNU General Public License v3.0
if(dune-localfunctions_FOUND)
  return()
endif()

if(dune-localfunctions_FIND_REQUIRED)
  find_package(dune-localfunctions CONFIG REQUIRED)
else()
  find_package(dune-localfunctions CONFIG)
endif()

if(dune-localfunctions_FOUND)
  target_compile_definitions(Dune::LocalFunctions INTERFACE HAVE_DUNE_LOCALFUNCTIONS=1)
  # make version number available in config.h
  include (UseDuneVer)
  find_dune_version ("dune" "localfunctions")
endif()
