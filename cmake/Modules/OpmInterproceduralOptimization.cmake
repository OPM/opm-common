include(CheckIPOSupported)
include(TestCXXAcceptsFlag)

# ------------------------------------------------------------------------------
# opm_interprocedural_optimization
#
# Configure interprocedural optimization (IPO) for a target.
#
# This function enables IPO (Link Time Optimization) on the specified target.
# It supports the standard CMake IPO interface as well as extended interfaces for
# GCC and Clang settings.
#
# The type of optimization applied is controlled via the cache variable:
#   OPM_INTERPROCEDURAL_OPTIMIZATION_TYPE
# which can be one of:
#   - CMake: use standard CMake IPO via INTERPROCEDURAL_OPTIMIZATION_<config>
#   - Clang: use Clang's ThinLTO with parallelism and cache configuration
#   - GNU:   use GNU's incremental LTO with parallelism and cache configuration
#   - NONE:    disable IPO
#
# Incremental cache directories are automatically managed under:
#   ${CMAKE_BINARY_DIR}/LTOCache/<config>
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
#   OPM_INTERPROCEDURAL_OPTIMIZATION_TYPE                 (CMake, GNU, Clang, NONE)
#   OPM_INTERPROCEDURAL_OPTIMIZATION_JOBS                 (e.g. 1, 8)
#   OPM_INTERPROCEDURAL_OPTIMIZATION_CACHE_POLICY         (e.g. "prune_after=604800")
#   OPM_INTERPROCEDURAL_OPTIMIZATION_CONFIGURATION_TYPES  (e.g. "Release;RelWithDebInfo")
#
# ------------------------------------------------------------------------------

set(OPM_INTERPROCEDURAL_OPTIMIZATION_CONFIGURATION_TYPES "Release;RelWithDebInfo" CACHE STRING "Default configuration types to apply OPM interprocedural optimization.")
set(OPM_INTERPROCEDURAL_OPTIMIZATION_JOBS 1 CACHE STRING "Default OPM interprocedural optimization jobs.")
set(OPM_INTERPROCEDURAL_OPTIMIZATION_CACHE_POLICY "" CACHE STRING "Default OPM interprocedural optimization cache policy.")
mark_as_advanced(OPM_INTERPROCEDURAL_OPTIMIZATION_JOBS)
mark_as_advanced(OPM_INTERPROCEDURAL_OPTIMIZATION_CACHE_POLICY)

# define possible values for LTO (order matters: first one in the list is used as default type)
set(opm_ipo_types)
check_ipo_supported(LANGUAGES C CXX RESULT ipo_supported)
if(ipo_supported)
  list(PREPEND opm_ipo_types CMake)
endif()

# Add custom for known compiler versions
if(ipo_supported)
  if((CMAKE_CXX_COMPILER_ID MATCHES "Clang") AND (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0.0))
    list(PREPEND opm_ipo_types Clang)
  elseif((CMAKE_CXX_COMPILER_ID MATCHES "GNU") AND (NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.5.4))
    list(PREPEND opm_ipo_types GNU)
  endif()
endif()

# Some linkers decide to discard "unused" functions prematurely and takes long, so disable for now.
list(INSERT opm_ipo_types 0 NONE)

set(OPM_INTERPROCEDURAL_OPTIMIZATION_TYPES ${opm_ipo_types} CACHE STRING "OPM interprocedural optimization types." FORCE)
mark_as_advanced(OPM_INTERPROCEDURAL_OPTIMIZATION_TYPES)
list(GET OPM_INTERPROCEDURAL_OPTIMIZATION_TYPES 0 opm_ipo_default_type)

list(JOIN OPM_INTERPROCEDURAL_OPTIMIZATION_TYPES ", " opm_ipo_types_str)
set(OPM_INTERPROCEDURAL_OPTIMIZATION_TYPE ${opm_ipo_default_type} CACHE STRING "Default OPM interprocedural optimization type. Possible values: ${opm_ipo_types_str}")
set_property(CACHE OPM_INTERPROCEDURAL_OPTIMIZATION_TYPE PROPERTY STRINGS ${OPM_INTERPROCEDURAL_OPTIMIZATION_TYPES})

message(STATUS "OPM interprocedural optimization type: ${OPM_INTERPROCEDURAL_OPTIMIZATION_TYPE}")

function(opm_interprocedural_optimization)
  cmake_parse_arguments(OPM_IPO "" "TARGET" "CONFIGURATION_TYPES" ${ARGN})

  # if not set, use the default configuration types
  if(NOT OPM_IPO_CONFIGURATION_TYPES)
    set(OPM_IPO_CONFIGURATION_TYPES ${OPM_INTERPROCEDURAL_OPTIMIZATION_CONFIGURATION_TYPES})
  endif()

  foreach(config ${OPM_IPO_CONFIGURATION_TYPES})
    if((CMAKE_CONFIGURATION_TYPES MATCHES ${config}) OR (CMAKE_BUILD_TYPE MATCHES ${config}))

      set(LTO_CACHE_PATH ${CMAKE_BINARY_DIR}/LTOCache/${config})

      if(OPM_INTERPROCEDURAL_OPTIMIZATION_TYPE STREQUAL CMake)
        # Use default CMake IPO
        string(TOUPPER ${config} uconfig)
        set_target_properties(${OPM_IPO_TARGET} PROPERTIES INTERPROCEDURAL_OPTIMIZATION_${uconfig} ON)
      elseif(OPM_INTERPROCEDURAL_OPTIMIZATION_TYPE STREQUAL GNU)
        if((NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.6.4) AND (NOT OPM_INTERPROCEDURAL_OPTIMIZATION_JOBS EQUAL 1))
          # https://gcc.gnu.org/onlinedocs/gcc-4.6.4/gcc/Optimize-Options.html
          target_compile_options(${OPM_IPO_TARGET} PRIVATE $<$<CONFIG:${config}>:-flto=${OPM_INTERPROCEDURAL_OPTIMIZATION_JOBS}>)
          target_link_options(${OPM_IPO_TARGET} INTERFACE $<$<CONFIG:${config}>:-flto=${OPM_INTERPROCEDURAL_OPTIMIZATION_JOBS}>)
        elseif(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.5.4)
          # https://gcc.gnu.org/onlinedocs/gcc-4.5.4/gcc/Optimize-Options.html
          target_compile_options(${OPM_IPO_TARGET} PRIVATE $<$<CONFIG:${config}>:-flto>)
          target_link_options(${OPM_IPO_TARGET} INTERFACE $<$<CONFIG:${config}>:-flto>)
        endif()

        if(NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 15.0.0)
          # use incremental LTO https://gcc.gnu.org/onlinedocs/gcc-15.2.0/gcc/Optimize-Options.html
          target_compile_options(${OPM_IPO_TARGET} PRIVATE $<$<CONFIG:${config}>:-flto-incremental=${LTO_CACHE_PATH}>)
          target_link_options(${OPM_IPO_TARGET} INTERFACE $<$<CONFIG:${config}>:-flto-incremental=${LTO_CACHE_PATH}>)

          # Configure cache directory
          file(MAKE_DIRECTORY ${LTO_CACHE_PATH})
          set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${LTO_CACHE_PATH})
        endif()

      elseif(OPM_INTERPROCEDURAL_OPTIMIZATION_TYPE STREQUAL Clang)
        # Use ThinLTO and reuse cache (see https://releases.llvm.org/20.1.0/tools/clang/docs/ThinLTO.html)

        target_compile_options(${OPM_IPO_TARGET} PRIVATE $<$<CONFIG:${config}>:-flto=thin>)
        target_link_options(${OPM_IPO_TARGET} INTERFACE $<$<CONFIG:${config}>:-flto=thin>)

        # Configure cache directory
        file(MAKE_DIRECTORY ${LTO_CACHE_PATH})
        set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_CLEAN_FILES ${LTO_CACHE_PATH})

        # Parallel and cache options are linker dependent
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
        elseif(${CMAKE_CXX_COMPILER_LINKER_ID} MATCHES "GNU|GNUgold|MOLD")
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
          message(DEBUG "IPO for Clang is supported, but linker type '${CMAKE_CXX_COMPILER_LINKER_ID}' is unrecognized. Skip adding parallelism or cache flags.")
        endif()
      endif()
    endif()
  endforeach()
endfunction()
