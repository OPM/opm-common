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
  cmake_parse_arguments(PARAM "PIC" "TARGET;TYPE;VERSION;SOVERSION" "SOURCES;HEADERS;LIBRARIES" ${ARGN})
  if(NOT PARAM_TARGET)
    message(FATAL_ERROR "Function needs a TARGET parameter")
  endif()
  if(NOT PARAM_SOVERSION)
    set(PARAM_SOVERSION ${PARAM_VERSION})
  endif()

  add_library(${PARAM_TARGET} ${PARAM_TYPE})
  if(PARAM_PIC)
    set_target_properties(${PARAM_TARGET}
      PROPERTIES
      POSITION_INDEPENDENT_CODE
        ON
    )
  endif()
  if(PARAM_VERSION)
    set_target_properties(${PARAM_TARGET}
      PROPERTIES
      SOVERSION
        ${PARAM_SOVERSION}
      VERSION
        ${PARAM_VERSION}
    )
  endif()
  if(PARAM_SOURCES)
    target_sources(${PARAM_TARGET} PRIVATE ${PARAM_SOURCES})
  endif()
  if(PARAM_HEADERS)
    target_sources(${PARAM_TARGET}
      PRIVATE
      FILE_SET
        HEADERS
      FILES
        ${PARAM_HEADERS}
    )
  endif()

  if(PARAM_LIBRARIES)
    target_link_libraries(${PARAM_TARGET} PUBLIC ${PARAM_LIBRARIES})
  endif()

  get_property(target_props TARGET ${${project}_TARGET} PROPERTY COMPILE_DEFINITIONS)
  if(target_props MATCHES HAVE_CONFIG_H)
    target_compile_definitions(${PARAM_TARGET} PRIVATE HAVE_CONFIG_H=1)
    target_include_directories(${PARAM_TARGET}
      BEFORE
      PUBLIC
        $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
    )
  endif()

  opm_add_target_options(TARGET ${PARAM_TARGET})
  opm_interprocedural_optimization(TARGET ${PARAM_TARGET})
endfunction()
