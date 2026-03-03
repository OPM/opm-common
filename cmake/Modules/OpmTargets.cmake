include(OpmInterproceduralOptimization)
include(UseFastBuilds)
include(UseOnlyNeeded)
include(UseOpenMP)
include(UseOptimization)
include(UseRunPath)
include(UseValgrind)
include(UseTracy)
include(UseWarnings)

# This adds all optional parameters to targets
function(opm_add_target_options)
  cmake_parse_arguments(PARAM "" "TARGET" "" ${ARGN})
  if(NOT PARAM_TARGET)
    message(FATAL_ERROR "Function needs a TARGET parameter")
  endif()

  # don't import more libraries than we need to
  use_only_needed(TARGET ${PARAM_TARGET})

  # setup rpath options
  use_runpath(TARGET ${PARAM_TARGET})

  # use tricks to do faster builds
  use_fast_build(TARGET ${PARAM_TARGET})

  # optionally turn on all warnings
  use_warnings(TARGET ${PARAM_TARGET})

  # additional optimization flags
  use_additional_optimization(TARGET ${PARAM_TARGET})

  # Parallel programming.
  use_openmp(TARGET ${PARAM_TARGET})

  # Tracy profiler
  use_tracy(TARGET ${PARAM_TARGET})

  # Valgrind memory error checker
  use_valgrind(TARGET ${PARAM_TARGET})

  # output binaries in 'bin' folder
  set_target_properties(${PARAM_TARGET}
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY
      bin
    ARCHIVE_OUTPUT_DIRECTORY
      lib
    LIBRARY_OUTPUT_DIRECTORY
      lib
  )

  # Set C11 and C++-20
  target_compile_features(${PARAM_TARGET}
    PUBLIC
    cxx_std_20
    c_std_11
  )
endfunction()

# Add a library target
function(opm_add_library)
  cmake_parse_arguments(PARAM "INTERFACE" "TARGET;TYPE;VERSION;" "" ${ARGN})
  if(NOT PARAM_TARGET)
    message(FATAL_ERROR "Function needs a TARGET parameter")
  endif()
  if(NOT PARAM_TARGET)
    message(FATAL_ERROR "Function needs a VERSION parameter")
  endif()

  add_library(${PARAM_TARGET} ${PARAM_TYPE})
  set_target_properties(${PARAM_TARGET}
    PROPERTIES
    SOVERSION
      ${${opm}_VERSION}
    VERSION
      ${${opm}_VERSION}
    LINK_FLAGS
      "${${opm}_LINKER_FLAGS_STR}"
    POSITION_INDEPENDENT_CODE
      TRUE
  )

  opm_add_target_options(TARGET ${PARAM_TARGET})
  opm_interprocedural_optimization(TARGET ${PARAM_TARGET})
endfunction()
