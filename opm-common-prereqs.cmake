# -*- mode: cmake; tab-width: 2; indent-tabs-mode: nil; truncate-lines: t; compile-command: "cmake -Wdev" -*-
# vim: set filetype=cmake autoindent tabstop=2 shiftwidth=2 expandtab softtabstop=2 nowrap:

# CMake 3.30.0 requires to find Boost in CONFIG mode
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.30.0)
  set(_Boost_CONFIG_MODE CONFIG)
endif()

find_package(Boost REQUIRED ${_Boost_CONFIG_MODE})
find_package(cJSON)
find_package(fmt)
find_package(QuadMath)

if(TARGET opmcommon)
  get_property(opm-common_EMBEDDED_PYTHON TARGET opmcommon PROPERTY EMBEDDED_PYTHON)
  get_property(opm-common_COMPILE_DEFINITIONS TARGET opmcommon PROPERTY INTERFACE_COMPILE_DEFINITIONS)

  if(opm-common_EMBEDDED_PYTHON)
    find_package(Python3 COMPONENTS Development.Embed REQUIRED)
  endif()

  if(opm-common_COMPILE_DEFINITIONS MATCHES HAVE_DUNE_COMMON)
    find_package(dune-common REQUIRED)
    if(dune-common_VERSION VERSION_LESS 2.11)
      target_include_directories(dunecommon INTERFACE ${dune-common_INCLUDE_DIRS})
    endif()
  endif()
endif()
