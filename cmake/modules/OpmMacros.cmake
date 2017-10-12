# .. cmake_module::
#
# This module contains a few convenience macros which are relevant for
# all OPM modules
#

# Specify the BUILD_TESTING option and set it to off by default. The
# reason is that builing the unit tests often takes considerable more
# time than the actual module and that these tests are not interesting
# for people who just want to use the module but not develop on
# it. Before merging changes into master, the continuous integration
# system makes sure that the unit tests pass, though.
option(BUILD_TESTING "Build the unit tests" OFF)

option(ADD_DISABLED_CTESTS "Add the tests which are disabled due to failed preconditions to the ctest output (this makes ctest return an error if such a test is present)" ON)
mark_as_advanced(ADD_DISABLED_CTESTS)

# add a "test-suite" target if it does not already exist
if(NOT TARGET test-suite)
  add_custom_target(test-suite)
endif()


# Add a single unit test (can be orchestrated by the 'ctest' command)
#
# Synopsis:
#       opm_add_test(TestName)
#
# Parameters:
#       TestName           Name of test
#       ONLY_COMPILE       Only build test but do not run it (optional)
#       ALWAYS_ENABLE      Force enabling test even if -DBUILD_TESTING=OFF was set (optional)
#       EXE_NAME           Name of test executable (optional, default: ./bin/${TestName})
#       CONDITION          Condition to enable test (optional, cmake code)
#       DEPENDS            Targets which the test depends on (optional)
#       DRIVER             The script which supervises the test (optional, default: ${OPM_TEST_DRIVER})
#       DRIVER_ARGS        The script which supervises the test (optional, default: ${OPM_TEST_DRIVER_ARGS})
#       TEST_ARGS          Arguments to pass to test's binary (optional, default: empty)
#       SOURCES            Source files for the test (optional, default: ${EXE_NAME}.cpp)
#       PROCESSORS         Number of processors to run test on (optional, default: 1)
#       TEST_DEPENDS       Other tests which must be run before running this test (optional, default: None)
#       INCLUDE_DIRS       Additional directories to look for include files (optional)
#       LIBRARIES          Libraries to link test against (optional)
#       WORKING_DIRECTORY  Working directory for test (optional, default: ${PROJECT_BINARY_DIR})
#
# Example:
#
# opm_add_test(funky_test
#              ALWAYS_ENABLE
#              CONDITION FUNKY_GRID_FOUND
#              SOURCES tests/MyFunkyTest.cpp
#              LIBRARIES -lgmp -lm)
include(CMakeParseArguments)

set(DUNE_REENABLE_ADD_TEST "YES")

macro(opm_add_test TestName)
  cmake_parse_arguments(CURTEST
                        "NO_COMPILE;ONLY_COMPILE;ALWAYS_ENABLE" # flags
                        "EXE_NAME;PROCESSORS;WORKING_DIRECTORY" # one value args
                        "CONDITION;TEST_DEPENDS;DRIVER;DRIVER_ARGS;DEPENDS;TEST_ARGS;SOURCES;LIBRARIES;INCLUDE_DIRS" # multi-value args
                        ${ARGN})

  # set the default values for optional parameters
  if (NOT CURTEST_EXE_NAME)
    set(CURTEST_EXE_NAME ${TestName})
  endif()

  # try to auto-detect the name of the source file if SOURCES are not
  # explicitly specified.
  if (NOT CURTEST_SOURCES)
    set(CURTEST_SOURCES "")
    set(_SDir "${PROJECT_SOURCE_DIR}")
    foreach(CURTEST_CANDIDATE "${CURTEST_EXE_NAME}.cpp"
                              "${CURTEST_EXE_NAME}.cc"
                              "tests/${CURTEST_EXE_NAME}.cpp"
                              "tests/${CURTEST_EXE_NAME}.cc")
      if (EXISTS "${_SDir}/${CURTEST_CANDIDATE}")
        set(CURTEST_SOURCES "${_SDir}/${CURTEST_CANDIDATE}")
      endif()
    endforeach()
  endif()

  # the default working directory is the content of
  # OPM_TEST_DEFAULT_WORKING_DIRECTORY or the source directory if this
  # is unspecified
  if (NOT CURTEST_WORKING_DIRECTORY)
    if (OPM_TEST_DEFAULT_WORKING_DIRECTORY)
      set(CURTEST_WORKING_DIRECTORY ${OPM_TEST_DEFAULT_WORKING_DIRECTORY})
    else()
      set(CURTEST_WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
    endif()
  endif()

  # don't build the tests by _default_ if BUILD_TESTING is false,
  # i.e., when typing 'make' the tests are not build in that
  # case. They can still be build using 'make test-suite' and they can
  # be build and run using 'make check'
  set(CURTEST_EXCLUDE_FROM_ALL "")
  if (NOT BUILD_TESTING AND NOT CURTEST_ALWAYS_ENABLE)
    set(CURTEST_EXCLUDE_FROM_ALL "EXCLUDE_FROM_ALL")
  endif()

  # figure out the test driver script and its arguments. (the variable
  # for the driver script may be empty. In this case the binary is run
  # "bare metal".)
  if (NOT CURTEST_DRIVER)
    set(CURTEST_DRIVER "${OPM_TEST_DRIVER}")
  endif()
  if (NOT CURTEST_DRIVER_ARGS)
    set(CURTEST_DRIVER_ARGS "${OPM_TEST_DRIVER_ARGS}")
  endif()

  # the libraries to link against. the libraries produced by the
  # current module are always linked against
  get_property(DUNE_MODULE_LIBRARIES GLOBAL PROPERTY DUNE_MODULE_LIBRARIES)
  if (NOT CURTEST_LIBRARIES)
    SET(CURTEST_LIBRARIES "${DUNE_MODULE_LIBRARIES}")
  else()
    SET(CURTEST_LIBRARIES "${CURTEST_LIBRARIES};${DUNE_MODULE_LIBRARIES}")
  endif()

  # the additional include directories
  get_property(DUNE_MODULE_INCLUDE_DIRS GLOBAL PROPERTY DUNE_INCLUDE_DIRS)
  if (NOT CURTEST_INCLUDE_DIRS)
    SET(CURTEST_INCLUDE_DIRS "${DUNE_INCLUDE_DIRS}")
  else()
    SET(CURTEST_INCLUDE_DIRS "${CURTEST_INCLUDE_DIRS};${DUNE_MODULE_INCLUDE_DIRS}")
  endif()

  # determine if the test should be completely ignored, i.e., the
  # CONDITION argument evaluates to false. the "AND OR " is a hack
  # which is required to prevent CMake from evaluating the condition
  # in the string. (which might evaluate to an empty string even
  # though "${CURTEST_CONDITION}" is not empty.)
  if ("AND OR ${CURTEST_CONDITION}" STREQUAL "AND OR ")
    set(SKIP_CUR_TEST "0")
  elseif(${CURTEST_CONDITION})
    set(SKIP_CUR_TEST "0")
  else()
    set(SKIP_CUR_TEST "1")
  endif()

  if (NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
  endif()

  if (NOT SKIP_CUR_TEST)
    if (CURTEST_ONLY_COMPILE)
      # only compile the binary but do not run it as a test
      add_executable("${CURTEST_EXE_NAME}" ${CURTEST_EXCLUDE_FROM_ALL} ${CURTEST_SOURCES})
      target_link_libraries (${CURTEST_EXE_NAME} ${CURTEST_LIBRARIES})
      target_include_directories(${CURTEST_EXE_NAME} PRIVATE ${CURTEST_INCLUDE_DIRS})
      
      if(CURTEST_DEPENDS)
        add_dependencies("${CURTEST_EXE_NAME}" ${CURTEST_DEPENDS})
      endif()
    else()
      if (NOT CURTEST_NO_COMPILE)
        # in addition to being run, the test must be compiled. (the
        # run-only case occurs if the binary is already compiled by an
        # earlier test.)
        add_executable("${CURTEST_EXE_NAME}" ${CURTEST_EXCLUDE_FROM_ALL} ${CURTEST_SOURCES})
        target_link_libraries (${CURTEST_EXE_NAME} ${CURTEST_LIBRARIES})
        target_include_directories(${CURTEST_EXE_NAME} PRIVATE ${CURTEST_INCLUDE_DIRS})

        if(CURTEST_DEPENDS)
          add_dependencies("${CURTEST_EXE_NAME}" ${CURTEST_DEPENDS})
        endif()
      endif()

      # figure out how the test should be run. if a test driver script
      # has been specified to supervise the test binary, use it else
      # run the test binary "naked".
      if (CURTEST_DRIVER)
        set(CURTEST_COMMAND ${CURTEST_DRIVER} ${CURTEST_DRIVER_ARGS} ${CURTEST_EXE_NAME} ${CURTEST_TEST_ARGS})
      else()
        set(CURTEST_COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CURTEST_EXE_NAME})
        if (CURTEST_TEST_ARGS)
          list(APPEND CURTEST_COMMAND ${CURTEST_TEST_ARGS})
        endif()
      endif()

      add_test(NAME ${TestName}
               WORKING_DIRECTORY "${CURTEST_WORKING_DIRECTORY}"
               COMMAND ${CURTEST_COMMAND})

      # return code 77 should be interpreted as skipped test
      set_tests_properties(${TestName} PROPERTIES SKIP_RETURN_CODE 77)

      # specify the dependencies between the tests
      if (CURTEST_TEST_DEPENDS)
        set_tests_properties(${TestName} PROPERTIES DEPENDS "${CURTEST_TEST_DEPENDS}")
      endif()

      # tell ctest how many cores it should reserve to run the test
      if (CURTEST_PROCESSORS)
        set_tests_properties(${TestName} PROPERTIES PROCESSORS "${CURTEST_PROCESSORS}")
      endif()
    endif()

    if (NOT CURTEST_NO_COMPILE)
      if(NOT TARGET test-suite)
        add_custom_target(test-suite)
      endif()
      add_dependencies(test-suite "${CURTEST_EXE_NAME}")#
    endif()

  else() # test is skipped

    # the following causes the test to appear as 'skipped' in the
    # CDash dashboard. it this is removed, the test is just silently
    # ignored.
    if (NOT CURTEST_ONLY_COMPILE AND ADD_DISABLED_CTESTS)
      add_test(${TestName} skip_test_dummy)

      # return code 77 should be interpreted as skipped test
      set_tests_properties(${TestName} PROPERTIES SKIP_RETURN_CODE 77)
    endif()

  endif()
endmacro()

# Add an application. This macro takes the same arguments as
# opm_add_test() but applications are always compiled if CONDITION
# evaluates to true and that applications are not added to the test
# suite.
#
# opm_add_application(AppName
#                     [EXE_NAME AppExecutableName]
#                     [CONDITION ConditionalExpression]
#                     [SOURCES SourceFile1 SourceFile2 ...])
macro(opm_add_application AppName)
  opm_add_test(${AppName} ${ARGN} ALWAYS_ENABLE ONLY_COMPILE)
endmacro()

# macro to set the default test driver script and the its default
# arguments
macro(opm_set_test_driver DriverBinary DriverDefaultArgs)
  set(OPM_TEST_DRIVER "${DriverBinary}")
  set(OPM_TEST_DRIVER_ARGS "${DriverDefaultArgs}")
endmacro()

# macro to set the default test driver script and the its default
# arguments
macro(opm_set_test_default_working_directory Dir)
  set(OPM_TEST_DEFAULT_WORKING_DIRECTORY "${Dir}")
endmacro()

macro(opm_recursive_add_library LIBNAME)
  set(TMP2 "")
  foreach(DIR ${ARGN})
    file(GLOB_RECURSE TMP RELATIVE "${CMAKE_SOURCE_DIR}" "${DIR}/*.cpp" "${DIR}/*.cc" "${DIR}/*.c")
    list(APPEND TMP2 ${TMP})
  endforeach()

  dune_add_library("${LIBNAME}"
    SOURCES "${TMP2}"
    )
endmacro()

macro(opm_recusive_copy_testdata)
  foreach(PAT ${ARGN})
    file(GLOB_RECURSE TMP RELATIVE "${CMAKE_SOURCE_DIR}" "${PAT}")

    foreach(SOURCE_FILE ${TMP})
      get_filename_component(DIRNAME "${SOURCE_FILE}" DIRECTORY)
      file(COPY "${SOURCE_FILE}" DESTINATION "${CMAKE_BINARY_DIR}/${DIRNAME}")
    endforeach()
  endforeach()
endmacro()

macro(opm_recusive_export_all_headers)
  foreach(DIR ${ARGN})
    file(GLOB_RECURSE TMP RELATIVE "${CMAKE_SOURCE_DIR}" "${DIR}/*.hpp" "${DIR}/*.hh" "${DIR}/*.h")

    foreach(HEADER ${TMP})
      get_filename_component(DIRNAME "${HEADER}" DIRECTORY)
      install(FILES "${HEADER}" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/${DIRNAME}")
    endforeach()
  endforeach()
endmacro()

macro(opm_export_cmake_modules)
  file(GLOB_RECURSE TMP RELATIVE "${CMAKE_SOURCE_DIR}" "cmake/*.cmake")

  foreach(CM_MOD ${TMP})
    get_filename_component(DIRNAME "${CM_MOD}" DIRECTORY)
    install(FILES "${CM_MOD}" DESTINATION "${DUNE_INSTALL_MODULEDIR}")
  endforeach()
endmacro()
