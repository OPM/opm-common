# - Find DUNE Fem library
#
# Defines the following variables:
#   dune-alugrid_INCLUDE_DIRS    Directory of header files
#   dune-alugrid_LIBRARIES       Directory of shared object files
#   dune-alugrid_DEFINITIONS     Defines that must be set to compile
#   dune-alugrid_CONFIG_VARS     List of defines that should be in config.h
#   HAVE_DUNE_FEM            Binary value to use in config.h

# Copyright (C) 2015 IRIS AS
# This code is licensed under The GNU General Public License v3.0
if(dune-fem_FOUND)
  return()
endif()

if(dune-fem_FIND_REQUIRED)
  find_package(dune-fem CONFIG REQUIRED)
else()
  find_package(dune-fem CONFIG)
endif()

if(dune-fem_FOUND)
  find_package(GMP)
  find_package(SuperLU)
  # make version number available in config.h
  include (UseDuneVer)
  find_dune_version ("dune" "fem")
  target_compile_definitions(Dune::Fem
    INTERFACE
      HAVE_DUNE_FEM=1
      DUNE_FEM_VERSION_MAJOR=${DUNE_FEM_VERSION_MAJOR}
      DUNE_FEM_VERSION_MINOR=${DUNE_FEM_VERSION_MINOR}
      DUNE_FEM_VERSION_REVISION=${DUNE_FEM_VERSION_REVISION}
  )
endif()
