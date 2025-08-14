include(CheckIPOSupported)
include(TestCXXAcceptsFlag)

# ------------------------------------------------------------------------------
# opm_interprocedural_optimization
#
# Configure interprocedural optimization (IPO) for a target.
#
# This function enables IPO (Link Time Optimization) on the specified target.
# It supports both the standard CMake IPO interface and ThinLTO settings for Clang.
#
# The type of optimization applied is controlled via the cache variable:
#   OPM_INTERPROCEDURAL_OPTIMIZATION_TYPE
# which can be one of:
#   - DEFAULT: use standard CMake IPO via INTERPROCEDURAL_OPTIMIZATION_<config>
#   - ThinLTO: use Clang's ThinLTO with parallelism and cache configuration
#   - NONE:    disable IPO
#
# ThinLTO cache directories are automatically created under:
#   ${CMAKE_BINARY_DIR}/LTOCache/<config>
# and registered for clean-up.
#
# Supported linkers for ThinLTO customization:
#   - LLD:       --thinlto-jobs, --thinlto-cache-dir, --thinlto-cache-policy
#   - GNUgold:   -plugin-opt,jobs=..., cache-dir=..., cache-policy=...
#   - AppleClang: -mllvm,-threads=..., -cache_path_lto,...
#
# Usage:
#
#   opm_interprocedural_optimization(
#     TARGET <target>
#     [CONFIGURATION_TYPES <config1;config2;...>]
#   )
#
# Arguments:
#   TARGET               (required) Target to which optimization will be applied.
#   CONFIGURATION_TYPES  (optional) List of configuration names (e.g. Release).
#                        If omitted, the default list from:
#                          OPM_INTERPROCEDURAL_OPTIMIZATION_CONFIGURATION_TYPES
#                        is used.
#
# Cache Variables:
#   OPM_INTERPROCEDURAL_OPTIMIZATION_TYPE                 (DEFAULT, ThinLTO, NONE)
#   OPM_INTERPROCEDURAL_OPTIMIZATION_JOBS                 (e.g. 1, 8)
#   OPM_INTERPROCEDURAL_OPTIMIZATION_CACHE_POLICY         (e.g. "prune_after=604800")
#   OPM_INTERPROCEDURAL_OPTIMIZATION_CONFIGURATION_TYPES  (e.g. "Release;RelWithDebInfo")
#
# ------------------------------------------------------------------------------


check_ipo_supported (RESULT ipo_supported)

# Only test ThinLTO if using Clang
if(ipo_supported AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
  check_cxx_accepts_flag ("-flto=thin" has_thin_lto_support)
endif()

# define default values for LTO
if(ipo_supported AND has_thin_lto_support)
  set(opm_ipo_type ThinLTO)
elseif (ipo_supported)
  set(opm_ipo_type DEFAULT)
else()
  set(opm_ipo_type NONE)
endif()

set(OPM_INTERPROCEDURAL_OPTIMIZATION_TYPE ${opm_ipo_type} CACHE STRING "Default OPM interprocedural optimization type. Possible values: DEFAULT, ThinLTO, NONE")
set(OPM_INTERPROCEDURAL_OPTIMIZATION_JOBS 1 CACHE STRING "Default OPM interprocedural optimization jobs.")
set(OPM_INTERPROCEDURAL_OPTIMIZATION_CACHE_POLICY "" CACHE STRING "Default OPM interprocedural optimization cache policy.")
set(OPM_INTERPROCEDURAL_OPTIMIZATION_CONFIGURATION_TYPES "Release;RelWithDebInfo" CACHE STRING "Default configuration types to apply OPM interprocedural optimization.")

message(STATUS "OPM interprocedural optimization type: ${OPM_INTERPROCEDURAL_OPTIMIZATION_TYPE}")

function(opm_interprocedural_optimization)
  cmake_parse_arguments(OPM_IPO "" "TARGET" "CONFIGURATION_TYPES" ${ARGN})

  # if not set, use the default configuration types
  if(NOT OPM_IPO_CONFIGURATION_TYPES)
    set(OPM_IPO_CONFIGURATION_TYPES ${OPM_INTERPROCEDURAL_OPTIMIZATION_CONFIGURATION_TYPES})
  endif()

  foreach(config ${OPM_IPO_CONFIGURATION_TYPES})
    if((CMAKE_CONFIGURATION_TYPES MATCHES ${config}) OR (CMAKE_BUILD_TYPE MATCHES ${config}))
      if(OPM_INTERPROCEDURAL_OPTIMIZATION_TYPE STREQUAL DEFAULT)
        # Use default CMake IPO
        set_target_properties(${OPM_IPO_TARGET} PROPERTIES INTERPROCEDURAL_OPTIMIZATION_${config} TRUE)
      elseif(OPM_INTERPROCEDURAL_OPTIMIZATION_TYPE STREQUAL ThinLTO)
        # Use ThinLTO and reuse cache (see https://releases.llvm.org/20.1.0/tools/clang/docs/ThinLTO.html)

        target_compile_options(${OPM_IPO_TARGET} PRIVATE $<$<CONFIG:${config}>:-flto=thin>)
        target_link_options(${OPM_IPO_TARGET} INTERFACE $<$<CONFIG:${config}>:-flto=thin>)

        # Configure cache directory
        set(LTO_CACHE_PATH ${CMAKE_BINARY_DIR}/LTOCache/${config})
        file(MAKE_DIRECTORY ${LTO_CACHE_PATH})
        set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${LTO_CACHE_PATH})

        if(${CMAKE_CXX_COMPILER_LINKER_ID} MATCHES "LLD")
          target_link_options(${OPM_IPO_TARGET} INTERFACE
            $<$<CONFIG:${config}>:LINKER:--thinlto-jobs=${OPM_INTERPROCEDURAL_OPTIMIZATION_JOBS}>
            $<$<CONFIG:${config}>:LINKER:--thinlto-cache-dir=${LTO_CACHE_PATH}>
          )
          if(OPM_INTERPROCEDURAL_OPTIMIZATION_CACHE_POLICY)
            target_link_options(${OPM_IPO_TARGET} INTERFACE
              $<$<CONFIG:${config}>:LINKER:--thinlto-cache-policy=${OPM_INTERPROCEDURAL_OPTIMIZATION_CACHE_POLICY}>
            )
          endif()
        elseif(${CMAKE_CXX_COMPILER_LINKER_ID} MATCHES "GNUgold")
          target_link_options(${OPM_IPO_TARGET} INTERFACE
            $<$<CONFIG:${config}>:LINKER:-plugin-opt,jobs=${OPM_INTERPROCEDURAL_OPTIMIZATION_JOBS}>
            $<$<CONFIG:${config}>:LINKER:-plugin-opt,cache-dir=${LTO_CACHE_PATH}>
          )
          if(OPM_INTERPROCEDURAL_OPTIMIZATION_CACHE_POLICY)
            target_link_options(${OPM_IPO_TARGET} INTERFACE
              $<$<CONFIG:${config}>:LINKER:-plugin-opt,cache-policy=${OPM_INTERPROCEDURAL_OPTIMIZATION_CACHE_POLICY}>
            )
          endif()
        elseif(${CMAKE_CXX_COMPILER_LINKER_ID} MATCHES "AppleClang")
          target_link_options(${OPM_IPO_TARGET} INTERFACE
            $<$<CONFIG:${config}>:LINKER:-mllvm,-threads=${OPM_INTERPROCEDURAL_OPTIMIZATION_JOBS}>
            $<$<CONFIG:${config}>:LINKER:-cache_path_lto,${LTO_CACHE_PATH}>
          )
        else()
          message(WARNING "ThinLTO supported, but linker type '${CMAKE_CXX_COMPILER_LINKER_ID}' is unrecognized. Not adding parallelism or cache flags.")
        endif()
      endif()
    endif()
  endforeach()

endfunction()
