# - Build satellites that are dependent of main library
#
# Enumerate all source code in a "satellite" directory such as tests/,
# compile each of them and optionally set them as a test for CTest to
# run. They will be linked to the main library created by the project.
#
# The following suffices must be defined for the opm prefix passed as
# parameter:
#
#	_LINKER_FLAGS   Necessary flags to link with this library
#	_TARGET         CMake target which creates the library
#	_LIBRARIES      Other dependencies that must also be linked

# Synopsis:
#	opm_compile_satellites (opm satellite excl_all test_regexp)
#
# Parameters:
#	opm             Prefix of the variable which contain information
#	                about the library these satellites depends on, e.g.
#	                pass "opm-core" if opm-core_TARGET is the name of
#	                the target the builds this library. Variables with
#	                suffixes _TARGET and _LIBRARIES must exist.
#
#	satellite       Prefix of variable which contain the names of the
#	                files, e.g. pass "tests" if the files are in the
#	                variable tests_SOURCES. Variables with suffixes
#	                _DATAFILES, _SOURCES and _DIR should exist. This
#	                name is also used as name of the target that builds
#	                all these files.
#
#	excl_all        EXCLUDE_FROM_ALL if these targets should not be built by
#	                default, otherwise empty string.
#
#	test_regexp     Regular expression which picks the name of a test
#	                out of the filename, or blank if no test should be
#	                setup.
#
# Example:
#	opm_compile_satellites (opm-core test "" "^test_([^/]*)$")
#
macro (opm_compile_satellites opm satellite excl_all test_regexp)
  # if we are going to build the tests always, then make sure that
  # the datafiles are present too
  if (NOT (${excl_all} MATCHES "EXCLUDE_FROM_ALL"))
	set (_incl_all "ALL")
  else (NOT (${excl_all} MATCHES "EXCLUDE_FROM_ALL"))
	set (_incl_all "")
  endif (NOT (${excl_all} MATCHES "EXCLUDE_FROM_ALL"))

  # if a set of datafiles has been setup, pull those in
  add_custom_target (${satellite} ${_incl_all})
  if (${satellite}_DATAFILES)
	add_dependencies (${satellite} ${${satellite}_DATAFILES})
  endif (${satellite}_DATAFILES)

  # compile each of these separately
  foreach (_sat_FILE IN LISTS ${satellite}_SOURCES)
	get_filename_component (_sat_NAME "${_sat_FILE}" NAME_WE)
	add_executable (${_sat_NAME} ${excl_all} ${_sat_FILE})
	add_dependencies (${satellite} ${_sat_NAME})
	set_target_properties (${_sat_NAME} PROPERTIES
	  LINK_FLAGS "${${opm}_LINKER_FLAGS_STR}"
	  )
	# are we building a test? luckily, the testing framework doesn't
	# require anything else, so we don't have to figure out where it
	# should go in the library list
	if (NOT "${test_regexp}" STREQUAL "")
	  set (_test_lib "${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}")
	else (NOT "${test_regexp}" STREQUAL "")
	  set (_test_lib "")
	endif (NOT "${test_regexp}" STREQUAL "")
	target_link_libraries (${_sat_NAME} ${${opm}_TARGET} ${${opm}_LIBRARIES} ${_test_lib})
        if (STRIP_DEBUGGING_SYMBOLS)
	  strip_debug_symbols (${_sat_NAME} _sat_DEBUG)
	  list (APPEND ${satellite}_DEBUG ${_sat_DEBUG})
        endif()

	# variable with regular expression doubles as a flag for
	# whether tests should be setup or not
	if (NOT "${test_regexp}" STREQUAL "")
	  foreach (_regexp IN ITEMS ${test_regexp})
		if ("${_sat_NAME}" MATCHES "${_regexp}")
		  string (REGEX REPLACE "${_regexp}" "\\1" _sat_FANCY "${_sat_NAME}")
		endif ("${_sat_NAME}" MATCHES "${_regexp}")
	  endforeach (_regexp)
	  get_target_property (_sat_LOC ${_sat_NAME} LOCATION)
	  if (CMAKE_VERSION VERSION_LESS "2.8.4")
		add_test (
		  NAME ${_sat_FANCY}
		  COMMAND ${CMAKE_COMMAND} -E chdir "${PROJECT_BINARY_DIR}/${${satellite}_DIR}" ${_sat_LOC}
		  )
	  else (CMAKE_VERSION VERSION_LESS "2.8.4")
		add_test (${_sat_FANCY} ${_sat_LOC})
		# run the test in the directory where the data files are
		set_tests_properties (${_sat_FANCY} PROPERTIES
		  WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/${${satellite}_DIR}
		  )
	  endif (CMAKE_VERSION VERSION_LESS "2.8.4")
          if(NOT TARGET test-suite)
            add_custom_target(test-suite)
          endif()
          add_dependencies(test-suite "${_sat_NAME}")
	endif(NOT "${test_regexp}" STREQUAL "")

	# if this program on the list of files that should be distributed?
	# we check by the name of the source file
	list (FIND ${satellite}_SOURCES_DIST "${_sat_FILE}" _is_util)
	if (NOT (_is_util EQUAL -1))
	  install (TARGETS ${_sat_NAME} RUNTIME
		DESTINATION bin${${opm}_VER_DIR}/
		)
	endif (NOT (_is_util EQUAL -1))
  endforeach (_sat_FILE)
endmacro (opm_compile_satellites opm prefix)

# Synopsis:
#	opm_data (satellite target dirname files)
#
# provides these output variables:
#
#	${satellite}_INPUT_FILES   List of all files that are copied
#	${satellite}_DATAFILES     Name of target which copies these files
#
# Example:
#
#	opm_data (tests datafiles "tests/")
#
macro (opm_data satellite target dirname)
  # even if there are no datafiles, create the directory so the
  # satellite programs have a homedir to run in
  execute_process (
	COMMAND ${CMAKE_COMMAND} -E make_directory ${dirname}
	)

  # if ever huge test datafiles are necessary, then change this
  # into "create_symlink" (on UNIX only, apparently)
  set (make_avail "copy")

  # provide datafiles as inputs for the tests, by copying them
  # to a tests/ directory in the output tree (if different)
  set (${satellite}_INPUT_FILES)
  if (NOT PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
	foreach (input_datafile IN LISTS ${satellite}_DATA)
	  file (RELATIVE_PATH rel_datafile "${PROJECT_SOURCE_DIR}" ${input_datafile})
	  set (output_datafile "${PROJECT_BINARY_DIR}/${rel_datafile}")
	  add_custom_command (
		OUTPUT ${output_datafile}
		COMMAND ${CMAKE_COMMAND}
		ARGS -E ${make_avail} ${input_datafile} ${output_datafile}
		DEPENDS ${input_datafile}
		VERBATIM
		)
	  list (APPEND ${satellite}_INPUT_FILES "${output_datafile}")
	endforeach (input_datafile)
  endif(NOT PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)

  # setup a target which does all the copying
  set (${satellite}_DATAFILES "${target}")
  add_custom_target (${${satellite}_DATAFILES}
	DEPENDS ${${satellite}_INPUT_FILES}
	COMMENT "Making \"${satellite}\" data available in output tree"
	)
endmacro (opm_data satellite target dirname files)

# Add a test
# Synopsis:
#       opm_add_test (TestName)
# Parameters:
#       TestName           Name of test
#       ONLY_COMPILE       Only build test (optional)
#       ALWAYS_ENABLE      Force enabling test (optional)
#       EXE_NAME           Name of test executable (optional)
#       CONDITION          Condition to enable test (optional, cmake code)
#       DRIVER_ARGS        Arguments to pass to test (optional)
#       DEPENDS            Targets test depends on (optional)
#       SOURCES            Sources for test (optional)
#       PROCESSORS         Number of processors to run test on (optional)
#       TEST_DEPENDS       Additional test dependencies for test (optional)
#       LIBRARIES          Libraries to link test against (optional)
#       WORKING_DIRECTORY  Working directory for test (optional)
# Example:
# opm_add_test(TestName
#              [NO_COMPILE]
#              [ONLY_COMPILE]
#              [ALWAYS_ENABLE]
#              [EXE_NAME TestExecutableName]
#              [CONDITION ConditionalExpression]
#              [DRIVER_ARGS TestDriverScriptArguments]
#              [WORKING_DIRECTORY dir]
#              [SOURCES SourceFile1 SourceFile2 ...]
#              [PROCESSORS NumberOfRequiredCores]
#              [DEPENDS Target1 Target2 ...]
#              [TEST_DEPENDS TestName1 TestName2 ...]
#              [LIBRARIES Lib1 Lib2 ...])
include(CMakeParseArguments)

macro(opm_add_test TestName)
  cmake_parse_arguments(CURTEST
                        "NO_COMPILE;ONLY_COMPILE;ALWAYS_ENABLE" # flags
                        "EXE_NAME;PROCESSORS;WORKING_DIRECTORY" # one value args
                        "CONDITION;TEST_DEPENDS;DRIVER_ARGS;DEPENDS;SOURCES;LIBRARIES" # multi-value args
                        ${ARGN})

  set(BUILD_TESTING "${BUILD_TESTING}")

  if (NOT CURTEST_EXE_NAME)
    set(CURTEST_EXE_NAME ${TestName})
  endif()
  if (NOT CURTEST_SOURCES)
    set(CURTEST_SOURCES ${CURTEST_EXE_NAME}.cpp)
  endif()
  if (NOT CURTEST_WORKING_DIRECTORY)
    set(CURTEST_WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
  endif()

  set(CURTEST_EXCLUDE_FROM_ALL "")
  if (NOT BUILD_TESTING AND NOT CURTEST_ALWAYS_ENABLE)
    # don't build the tests by _default_ (i.e., when typing
    # 'make'). Though they can still be build using 'make ctests' and
    # they can be build and run using 'make check'
    set(CURTEST_EXCLUDE_FROM_ALL "EXCLUDE_FROM_ALL")
  endif()

  set(SKIP_CUR_TEST "1")
  # the "AND OR " is a hack which is required to prevent CMake from
  # evaluating the condition in the string. (which might
  # evaluate to an empty string even though "${CURTEST_CONDITION}"
  # is not empty.)
  if ("AND OR ${CURTEST_CONDITION}" STREQUAL "AND OR ")
    set(SKIP_CUR_TEST "0")
  elseif(${CURTEST_CONDITION})
    set(SKIP_CUR_TEST "0")
  endif()

  if (NOT SKIP_CUR_TEST)
    if (CURTEST_ONLY_COMPILE)
      add_executable("${CURTEST_EXE_NAME}" ${CURTEST_EXCLUDE_FROM_ALL} ${CURTEST_SOURCES})
      target_link_libraries (${CURTEST_EXE_NAME} ${CURTEST_LIBRARIES})
    else()
      if (NOT CURTEST_NO_COMPILE)
        add_executable("${CURTEST_EXE_NAME}" ${CURTEST_EXCLUDE_FROM_ALL} ${CURTEST_SOURCES})
        target_link_libraries (${CURTEST_EXE_NAME} ${CURTEST_LIBRARIES})
        if(CURTEST_DEPENDS)
          add_dependencies("${CURTEST_EXE_NAME}" ${CURTEST_DEPENDS})
        endif()

        if(NOT TARGET test-suite)
          add_custom_target(test-suite)
        endif()
        add_dependencies(test-suite "${CURTEST_EXE_NAME}")
      endif()

      add_test(NAME ${TestName}
               WORKING_DIRECTORY "${CURTEST_WORKING_DIRECTORY}"
               ${DEPENDS_ON}
               COMMAND ${CURTEST_EXE_NAME} ${CURTEST_DRIVER_ARGS})
      if (CURTEST_TEST_DEPENDS)
        set_tests_properties(${TestName} PROPERTIES DEPENDS "${CURTEST_TEST_DEPENDS}")
      endif()
      if (CURTEST_PROCESSORS)
        set_tests_properties(${TestName} PROPERTIES PROCESSORS "${CURTEST_PROCESSORS}")
      endif()
    endif()
  endif()
endmacro()
