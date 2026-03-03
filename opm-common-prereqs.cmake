# -*- mode: cmake; tab-width: 2; indent-tabs-mode: nil; truncate-lines: t; compile-command: "cmake -Wdev" -*-
# vim: set filetype=cmake autoindent tabstop=2 shiftwidth=2 expandtab softtabstop=2 nowrap:

# defines that must be present in config.h for our headers
set (opm-common_CONFIG_VAR
  HAVE_ECL_INPUT
  HAVE_CXA_DEMANGLE
  HAVE_DUNE_COMMON
)

# CMake 3.30.0 requires to find Boost in CONFIG mode
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.30.0)
  set(_Boost_CONFIG_MODE CONFIG)
endif()

list(APPEND opm-common_DEPS
      # various runtime library enhancements
      "Boost 1.44.0 REQUIRED ${_Boost_CONFIG_MODE}"
      "cJSON"
      # Still it produces compile errors complaining that it
      # cannot format UDQVarType. Hence we use the same version
      # as the embedded one.
      "fmt 8.0"
      "QuadMath"
)

if(TARGET opmcommon)
  get_property(opm-common_EMBEDDED_PYTHON TARGET opmcommon PROPERTY EMBEDDED_PYTHON)
endif()

if(opm-common_EMBEDDED_PYTHON)
  list(APPEND opm-common_DEPS
    "Python3 COMPONENTS Development.Embed REQUIRED"
  )
endif()

find_package_deps(opm-common)
