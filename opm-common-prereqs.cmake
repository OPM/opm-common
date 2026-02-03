# -*- mode: cmake; tab-width: 2; indent-tabs-mode: nil; truncate-lines: t; compile-command: "cmake -Wdev" -*-
# vim: set filetype=cmake autoindent tabstop=2 shiftwidth=2 expandtab softtabstop=2 nowrap:

# defines that must be present in config.h for our headers
set (opm-common_CONFIG_VAR
  HAVE_OPENMP
  HAVE_VALGRIND
  HAVE_ECL_INPUT
  HAVE_CXA_DEMANGLE
  HAVE_FNMATCH_H
  HAVE_DUNE_COMMON
)

# CMake 3.30.0 requires to find Boost in CONFIG mode
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.30.0)
  set(_Boost_CONFIG_MODE CONFIG)
endif()

find_package(Valgrind)
find_package(Boost 1.44.0 COMPONENTS unit_test_framework REQUIRED ${_Boost_CONFIG_MODE})
find_package(cJSON)
find_package(fmt)
find_package(QuadMath)

opm_add_dependencies(
  PREFIX
    opm-common
  OPTIONAL
    fmt::fmt
    Valgrind::Valgrind
    cjson
)
