# - Compile main library target

include(OpmInterproceduralOptimization)

macro(opm_compile opm)
  # some CMake properties do not do list expansion
  string(REPLACE ";" " " ${opm}_LINKER_FLAGS_STR "${${opm}_LINKER_FLAGS}")

  # name of the library should not contain dashes, as CMake will
  # define a symbol with that name, and those cannot contain dashes
  string(REPLACE "-" "" ${opm}_TARGET "${${opm}_NAME}")

  # all public header files are together with the source. prepend our own
  # source path to the one of the dependencies so that our version of any
  # ambigious paths are used.

  option(SILENCE_CROSSMODULE_WARNINGS "Disable warnings from cross-module includes" OFF)
  if(SILENCE_CROSSMODULE_WARNINGS)
     include_directories("${PROJECT_SOURCE_DIR}")
     include_directories(SYSTEM ${${opm}_INCLUDE_DIRS})
     set(${opm}_INCLUDE_DIR "${PROJECT_SOURCE_DIR}")
     set(${opm}_INCLUDE_DIRS ${${opm}_INCLUDE_DIR} ${${opm}_INCLUDE_DIRS})
  else()
     set(${opm}_INCLUDE_DIR "${PROJECT_SOURCE_DIR}")
     set(${opm}_INCLUDE_DIRS ${${opm}_INCLUDE_DIR} ${${opm}_INCLUDE_DIRS})
     include_directories (${${opm}_INCLUDE_DIRS})
  endif()

  # create this library, if there are any compilation units
  link_directories(${${opm}_LIBRARY_DIRS})
  if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.12")
    # Some modules may still export definitions using -D, strip it
    string(REGEX REPLACE "-D" "" _clean_defs "${${opm}_DEFINITIONS}")
    add_compile_definitions(${_clean_defs})
  else()
    add_definitions(${${opm}_DEFINITIONS})
  endif()

  if(NOT ${opm}_SOURCES)
    set(${opm}_LIBRARY_TYPE INTERFACE)
  endif()
  add_library(${${opm}_TARGET} ${${opm}_LIBRARY_TYPE})

  if(${opm}_SOURCES)
    target_sources(${${opm}_TARGET}
      PRIVATE
        ${${opm}_SOURCES}
    )
    set(${opm}_LIBRARY $<TARGET_FILE:${${opm}_TARGET}>)
  endif()
  target_sources(${${opm}_TARGET}
    PRIVATE
    FILE_SET
      private_headers
    TYPE
      HEADERS
    FILES
      ${${opm}_PRIVATE_HEADERS}
  )
  target_sources(${${opm}_TARGET}
    PUBLIC
    FILE_SET
      HEADERS
    BASE_DIRS
      ${PROJECT_SOURCE_DIR}
    FILES
      ${${opm}_HEADERS}
  )
  target_sources(${${opm}_TARGET}
    PUBLIC
    FILE_SET
      generated_headers
    TYPE
      HEADERS
    BASE_DIRS
      ${PROJECT_BINARY_DIR}
    FILES
      ${${opm}_GENERATED_HEADERS}
  )

  if(${opm}_LIBRARY)
    set_target_properties(${${opm}_TARGET}
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
    opm_interprocedural_optimization(TARGET ${${opm}_TARGET})
  endif()

  target_link_libraries(${${opm}_TARGET}
    PUBLIC
      ${${opm}_LIBRARIES}
  )
endmacro (opm_compile opm)
