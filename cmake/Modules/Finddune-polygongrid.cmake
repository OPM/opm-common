# - Find DUNE Fem library
#
# Defines the following variables:
#   dune-polygongrid_INCLUDE_DIRS    Directory of header files
#   dune-polygongrid_LIBRARIES       Directory of shared object files
#   dune-polygongrid_DEFINITIONS     Defines that must be set to compile
#   dune-alugrid_CONFIG_VARS     List of defines that should be in config.h
#   HAVE_DUNE_POLYGONGRID            Binary value to use in config.h

# Copyright (C) 2015 IRIS AS
# This code is licensed under The GNU General Public License v3.0

include (OpmPackage)
find_opm_package (
  # module name
  "dune-polygongrid"

  # dependencies
  # TODO: we should probe for all the HAVE_* values listed below;
  # however, we don't actually use them in our implementation, so
  # we just include them to forward here in case anyone else does
  "dune-common REQUIRED;
  "
  # header to search for
  "dune/polygongrid/mesh.hh"

  # library to search for
  "dunepolygongrid"

  # defines to be added to compilations
  ""

  # test program
"#include <dune/polygongrid/mesh.hh>
int main (void) {
   return 0;
}
"
# config variables
# config variables
  "HAVE_DUNE_POLYGONGRID
  ")

#debug_find_vars ("dune-polygongrid")

# make version number available in config.h
include (UseDuneVer)
find_dune_version ("dune" "polygongrid")
