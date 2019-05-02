project(opm-common C CXX)
cmake_minimum_required (VERSION 2.8)

option(SIBLING_SEARCH "Search for other modules in sibling directories?" ON)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)
set(OPM_MACROS_ROOT ${PROJECT_SOURCE_DIR})

option(ENABLE_ECL_INPUT "Enable eclipse input support?" ON)
option(ENABLE_ECL_OUTPUT "Enable eclipse output support?" ON)
option(ENABLE_MOCKSIM "Build the mock simulator for io testing" ON)
option(ENABLE_WELL_TEST "Enable testing of well code when building Schedule object" OFF)

if (ENABLE_WELL_TEST)
  add_definitions(-DWELL_TEST)
endif()

# Output implies input
if(ENABLE_ECL_OUTPUT)
  set(ENABLE_ECL_INPUT ON)
endif()

# And likewise, no input means no output
if(NOT ENABLE_ECL_INPUT)
  set(ENABLE_ECL_OUTPUT OFF)
endif()


# not the same location as most of the other projects; this hook overrides
macro (dir_hook)
endmacro (dir_hook)

# We need to define this variable in the installed cmake config file.
set(OPM_PROJECT_EXTRA_CODE_INSTALLED  "#ENABLE_ECL_INPUT is needed by opm-common-prereq.cmake
                                       set(ENABLE_ECL_INPUT ${ENABLE_ECL_INPUT})
                                       set(OPM_MACROS_ROOT ${CMAKE_INSTALL_PREFIX}/share/opm)
                                       list(APPEND CMAKE_MODULE_PATH \${OPM_MACROS_ROOT}/cmake/Modules)
                                       include(OpmPackage) #Make macros availabe after find_package(opm-common)")

set(OPM_PROJECT_EXTRA_CODE_INTREE "#ENABLE_ECL_INPUT is needed by opm-common-prereq.cmake
                                   set(ENABLE_ECL_INPUT ${ENABLE_ECL_INPUT})
                                   set(OPM_MACROS_ROOT ${OPM_MACROS_ROOT})
                                   list(APPEND CMAKE_MODULE_PATH \${OPM_MACROS_ROOT}/cmake/Modules)
                                   include(OpmPackage) #Make macros availabe after find_package(opm-common)")
if(ENABLE_ECL_OUTPUT)
  set(OPM_PROJECT_EXTRA_CODE_INSTALLED "${OPM_PROJECT_EXTRA_CODE_INSTALLED}
                                        set(COMPARE_ECL_COMMAND ${CMAKE_INSTALL_PREFIX}/bin${${name}_VER_DIR}/compareECL)
                                        set(OPM_PACK_COMMAND ${CMAKE_INSTALL_PREFIX}/bin${${name}_VER_DIR}/opmpack)")

  set(OPM_PROJECT_EXTRA_CODE_INTREE "${OPM_PROJECT_EXTRA_CODE_INTREE}
                                     set(COMPARE_ECL_COMMAND ${PROJECT_BINARY_DIR}/bin/compareECL)
                                     set(OPM_PACK_COMMAND ${PROJECT_BINARY_DIR}/bin/opmpack)")
endif()

# project information is in dune.module. Read this file and set variables.
# we cannot generate dune.module since it is read by dunecontrol before
# the build starts, so it makes sense to keep the data there then.
include (OpmInit)

# Look for the opm-tests repository; if found the variable
# HAVE_OPM_TESTS will be set to true.
include(Findopm-tests)

# list of prerequisites for this particular project; this is in a
# separate file (in cmake/Modules sub-directory) because it is shared
# with the find module
include (${project}-prereqs)

# read the list of components from this file (in the project directory);
# it should set various lists with the names of the files to include
include (CMakeLists_files.cmake)

macro (config_hook)
  if(ENABLE_ECL_INPUT)
    if(NOT cjson_FOUND)
      list(APPEND EXTRA_INCLUDES ${PROJECT_SOURCE_DIR}/external/cjson)
    endif()
    # For this project
    include_directories(${EXTRA_INCLUDES} ${PROJECT_BINARY_DIR}/include)

    # For downstreams
    list(APPEND EXTRA_INCLUDES ${PROJECT_BINARY_DIR}/include)
    set(OPM_PROJECT_EXTRA_CODE_INTREE "${OPM_PROJECT_EXTRA_CODE_INTREE}
                                       list(APPEND opm-common_INCLUDE_DIRS ${EXTRA_INCLUDES})")
    if(ENABLE_ECL_INPUT)
      set(OPM_PROJECT_EXTRA_CODE_INTREE "${OPM_PROJECT_EXTRA_CODE_INTREE}
                                         set(HAVE_ECL_INPUT 1)")
      set(OPM_PROJECT_EXTRA_CODE_INSTALLED "${OPM_PROJECT_EXTRA_CODE_INSTALLED}
                                            set(HAVE_ECL_INPUT 1)")
    endif()
    if(ENABLE_ECL_OUTPUT)
      set(OPM_PROJECT_EXTRA_CODE_INTREE "${OPM_PROJECT_EXTRA_CODE_INTREE}
                                         set(HAVE_ECL_OUTPUT 1)")
      set(OPM_PROJECT_EXTRA_CODE_INSTALLED "${OPM_PROJECT_EXTRA_CODE_INSTALLED}
                                            set(HAVE_ECL_OUTPUT 1)")
    endif()

    # Configure boost targets for old cmake
    include(cmake/Modules/BoostTargets.cmake)

    if (HAVE_DYNAMIC_BOOST_TEST)
      set_target_properties(Boost::unit_test_framework PROPERTIES INTERFACE_COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK=1)
    endif()
  endif()
endmacro (config_hook)

macro (prereqs_hook)
endmacro (prereqs_hook)

macro (sources_hook)
  if(ENABLE_ECL_INPUT)
    # Keyword generation
    include(GenerateKeywords.cmake)

    # Append generated sources
    list(APPEND opm-common_SOURCES ${PROJECT_BINARY_DIR}/ParserKeywords.cpp)
  endif()
endmacro (sources_hook)

macro (fortran_hook)
endmacro (fortran_hook)

macro (files_hook)
endmacro (files_hook)

macro (tests_hook)
  if(ENABLE_ECL_INPUT)
    include(ExtraTests.cmake)
  endif()
endmacro (tests_hook)

macro (install_hook)
  install(DIRECTORY ${PROJECT_BINARY_DIR}/include/
          DESTINATION include
          PATTERN *.hpp)
endmacro (install_hook)

# all setup common to the OPM library modules is done here
include (OpmLibMain)

if (ENABLE_MOCKSIM)
  add_library(mocksim
              msim/src/msim.cpp)
  target_link_libraries(mocksim opmcommon)
  target_include_directories(mocksim PUBLIC msim/include)
  add_executable(msim examples/msim.cpp)
  target_link_libraries(msim mocksim)

  set(_libs mocksim opmcommon
    ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

  foreach( test test_msim test_msim_ACTIONX )
    opm_add_test(${test} SOURCES tests/msim/${test}
                         LIBRARIES ${_libs}
                         WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
  endforeach()
endif()

# Build the compare utilities
if(ENABLE_ECL_INPUT)
  add_executable(compareECL
    test_util/EclFilesComparator.cpp
    test_util/EclRegressionTest.cpp
    test_util/compareECL.cpp
    )

  add_executable(convertECL
    test_util/convertECL.cpp
    )

  foreach(target compareECL convertECL)
    target_link_libraries(${target} opmcommon)
    install(TARGETS ${target} DESTINATION bin)
  endforeach()

  # Add the tests
  set(_libs opmcommon
            ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})

  opm_add_test(test_EclFilesComparator
    CONDITION
      ENABLE_ECL_INPUT
    SOURCES
      tests/test_EclFilesComparator.cpp
      test_util/EclFilesComparator.cpp
    LIBRARIES
      ${_libs}
    WORKING_DIRECTORY
      ${PROJECT_BINARY_DIR}/tests
    )

  opm_add_test(test_EclRegressionTest
    CONDITION
      ENABLE_ECL_INPUT
    SOURCES
      tests/test_EclRegressionTest.cpp
      test_util/EclFilesComparator.cpp
      test_util/EclRegressionTest.cpp
    LIBRARIES
      ${_libs}
    WORKING_DIRECTORY
      ${PROJECT_BINARY_DIR}/tests
    )

  foreach(test test_EclIO test_EGrid test_ERft test_ERst test_ESmry)
    opm_add_test(${test} CONDITION ENABLE_ECL_INPUT
                         LIBRARIES ${_libs}
                         WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
  endforeach()
endif()

# Install build system files
install(DIRECTORY cmake DESTINATION share/opm)

# Install tab completion skeleton
install(FILES etc/opm_bash_completion.sh.in DESTINATION share/opm/etc)
