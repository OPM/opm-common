project(opm-common C CXX)
cmake_minimum_required (VERSION 2.8)

option(SIBLING_SEARCH "Search for other modules in sibling directories?" ON)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)
set(OPM_MACROS_ROOT ${PROJECT_SOURCE_DIR})

option(ENABLE_ECL_INPUT "Enable eclipse input support?" ON)
option(ENABLE_ECL_OUTPUT "Enable eclipse output support?" ON)

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

# Build the compare utilities
if(ENABLE_ECL_INPUT)
  add_library(testutil STATIC
              examples/test_util/EclFilesComparator.cpp
              examples/test_util/EclIntegrationTest.cpp
              examples/test_util/EclRegressionTest.cpp
              examples/test_util/summaryComparator.cpp
              examples/test_util/summaryIntegrationTest.cpp
              examples/test_util/summaryRegressionTest.cpp)
  target_link_libraries(testutil ecl)
  add_executable(compareECL examples/test_util/compareECL.cpp)
  target_link_libraries(compareECL testutil opmcommon)

  # Add the tests
  set(_libs testutil opmcommon
            ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
  opm_add_test(test_compareSummary CONDITION ENABLE_ECL_INPUT
                                   LIBRARIES ${_libs})
  opm_add_test(test_EclFilesComparator CONDITION ENABLE_ECL_INPUT
                                       LIBRARIES ${_libs})
  if(HAVE_DYNAMIC_BOOST_TEST AND NOT BUILD_SHARED_LIBS)
    set_target_properties(test_compareSummary PROPERTIES
                          COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK)
    set_target_properties(test_EclFilesComparator PROPERTIES
                          COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK)
  endif()
  install(TARGETS compareECL DESTINATION bin)
endif()

# Install build system files
install(DIRECTORY cmake DESTINATION share/opm)
