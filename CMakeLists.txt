project(opm-common C CXX)
cmake_minimum_required (VERSION 2.8)

option(SIBLING_SEARCH "Search for other modules in sibling directories?" ON)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)
set(OPM_MACROS_ROOT ${PROJECT_SOURCE_DIR})

option(ENABLE_MOCKSIM "Build the mock simulator for io testing" ON)
option(OPM_ENABLE_PYTHON "Enable python bindings?" OFF)
option(OPM_INSTALL_PYTHON "Enable python bindings?" OFF)
option(OPM_ENABLE_EMBEDDED_PYTHON "Enable python bindings?" OFF)


# not the same location as most of the other projects; this hook overrides
macro (dir_hook)
endmacro (dir_hook)

# We need to define this variable in the installed cmake config file.
set(OPM_PROJECT_EXTRA_CODE_INSTALLED  "set(OPM_MACROS_ROOT ${CMAKE_INSTALL_PREFIX}/share/opm)
                                       list(APPEND CMAKE_MODULE_PATH \${OPM_MACROS_ROOT}/cmake/Modules)
                                       include(OpmPackage) #Make macros availabe after find_package(opm-common)")

set(OPM_PROJECT_EXTRA_CODE_INTREE "set(OPM_MACROS_ROOT ${OPM_MACROS_ROOT})
                                   list(APPEND CMAKE_MODULE_PATH \${OPM_MACROS_ROOT}/cmake/Modules)
                                   include(OpmPackage) #Make macros availabe after find_package(opm-common)")
set(OPM_PROJECT_EXTRA_CODE_INSTALLED "${OPM_PROJECT_EXTRA_CODE_INSTALLED}
                                      set(COMPARE_ECL_COMMAND ${CMAKE_INSTALL_PREFIX}/bin${${name}_VER_DIR}/compareECL)
                                      set(OPM_PACK_COMMAND ${CMAKE_INSTALL_PREFIX}/bin${${name}_VER_DIR}/opmpack)")

set(OPM_PROJECT_EXTRA_CODE_INTREE "${OPM_PROJECT_EXTRA_CODE_INTREE}
                                   set(COMPARE_ECL_COMMAND ${PROJECT_BINARY_DIR}/bin/compareECL)
                                   set(OPM_PACK_COMMAND ${PROJECT_BINARY_DIR}/bin/opmpack)")

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
  if(NOT cjson_FOUND)
    list(APPEND EXTRA_INCLUDES ${PROJECT_SOURCE_DIR}/external/cjson)
  endif()
  # For this project
  include_directories(${EXTRA_INCLUDES} ${PROJECT_BINARY_DIR}/include)

  # For downstreams
  list(APPEND EXTRA_INCLUDES ${PROJECT_BINARY_DIR}/include)
  set(OPM_PROJECT_EXTRA_CODE_INTREE "${OPM_PROJECT_EXTRA_CODE_INTREE}
                                     list(APPEND opm-common_INCLUDE_DIRS ${EXTRA_INCLUDES})")

  # Configure boost targets for old cmake
  include(cmake/Modules/BoostTargets.cmake)

  if (HAVE_DYNAMIC_BOOST_TEST)
    set_target_properties(Boost::unit_test_framework PROPERTIES INTERFACE_COMPILE_DEFINITIONS BOOST_TEST_DYN_LINK=1)
  endif()
endmacro (config_hook)

macro (prereqs_hook)
endmacro (prereqs_hook)

macro (sources_hook)
  # Keyword generation
  include(GenerateKeywords.cmake)

  # Append generated sources
  list(INSERT opm-common_SOURCES 0 ${PROJECT_BINARY_DIR}/ParserKeywords.cpp)
  set_source_files_properties(src/opm/parser/eclipse/Python/Python.cpp
                              PROPERTIES COMPILE_FLAGS -Wno-shadow)
endmacro (sources_hook)

macro (fortran_hook)
endmacro (fortran_hook)

macro (files_hook)
endmacro (files_hook)

macro (tests_hook)
  include(ExtraTests.cmake)
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
    opm_add_test(${test} SOURCES tests/msim/${test}.cpp
                         LIBRARIES ${_libs}
                         WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
  endforeach()
endif()

# Build the compare utilities
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
    SOURCES
      tests/test_EclFilesComparator.cpp
      test_util/EclFilesComparator.cpp
    LIBRARIES
      ${_libs}
    WORKING_DIRECTORY
      ${PROJECT_BINARY_DIR}/tests
    )

  opm_add_test(test_EclRegressionTest
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
  opm_add_test(${test} LIBRARIES ${_libs}
                       WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/tests)
endforeach()

# Install build system files
install(DIRECTORY cmake DESTINATION share/opm)

# Install tab completion skeleton
install(FILES etc/opm_bash_completion.sh.in DESTINATION share/opm/etc)

if (OPM_ENABLE_PYTHON)
  # -------------------------------------------------------------------------
  # 1: Wrap C++ functionality in Python
  find_package(PythonInterp REQUIRED)
  include(FindPythonInterp)

  set(PYTHON_PACKAGE_PATH "site-packages")
  set(PYTHON_INSTALL_PREFIX "lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/${PYTHON_PACKAGE_PATH}" CACHE STRING "Subdirectory to install Python modules in")

  make_directory(${CMAKE_BINARY_DIR}/python)
  get_target_property(_opmcommon_include_dirs opmcommon INCLUDE_DIRECTORIES)
  list(APPEND _opmcommon_include_dirs ${_ecl_include_dirs})
  string(REPLACE ";" ":" _setup_include_dirs "${_opmcommon_include_dirs}")

  set(_opmcommon_lib_dirs ${CMAKE_BINARY_DIR}/lib ${CMAKE_PREFIX_PATH}/lib)
  string(REPLACE ";" ":" _setup_lib_dirs "${_opmcommon_lib_dirs}")

  add_custom_command(OUTPUT python/python/opm/libopmcommon_python.so
                     DEPENDS
                     python/cxx/unit_system.cpp
                     python/cxx/connection.cpp
                     python/cxx/converters.hpp
                     python/cxx/deck.cpp
                     python/cxx/deck_keyword.cpp
                     python/cxx/eclipse_3d_properties.cpp
                     python/cxx/eclipse_config.cpp
                     python/cxx/eclipse_grid.cpp
                     python/cxx/eclipse_state.cpp
                     python/cxx/group.cpp
                     python/cxx/log.cpp
                     python/cxx/parsecontext.cpp
                     python/cxx/parser.cpp
                     python/cxx/schedule.cpp
                     python/cxx/export.cpp
                     python/cxx/export.hpp
                     python/cxx/table_manager.cpp
                     python/cxx/well.cpp
                     COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/python/ ${CMAKE_BINARY_DIR}/python
                     COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_BINARY_DIR}/python/setup.py
                                                      build
                                                      build_ext
                                                      --build-lib=${CMAKE_BINARY_DIR}/python/python/opm
                                                      --library-dirs=${_setup_lib_dirs}
                                                      --include-dirs=${_setup_include_dirs}
                     WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/python
                     COMMENT "Building python bindings")
  add_custom_target(opmcommon_python ALL DEPENDS python/python/opm/libopmcommon_python.so)
  add_dependencies(opmcommon_python opmcommon)

  # The install target is based on manually copying the python file tree to the
  # installation area with a small installation script 'install.py'. Would have
  # preferred to use standard setup.py install, but the setup.py based solution
  # refuses to install to a location which the current python executable can not
  # load from, and the use of eggs in the setup.py based installation makes
  # debugging quite difficult.
  #
  # Since the installation of Python code is nonstandard it is protected by an
  # extra cmake switch, OPM_INSTALL_PYTHON. If you prefer you can still invoke
  # setup.py install manually - optionally with the generated script
  # setup-install.sh - and completely bypass cmake in the installation phase.
  if (OPM_INSTALL_PYTHON)
    install( CODE "execute_process(COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_BINARY_DIR}/python/install.py ${CMAKE_BINARY_DIR}/python/python/opm ${CMAKE_INSTALL_PREFIX}/${PYTHON_INSTALL_PREFIX})")
  endif()

  add_test(NAME python_tests
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/python
            COMMAND ${PYTHON_EXECUTABLE} setup.py build_ext --dry-run --build-lib ${CMAKE_BINARY_DIR}/python/python/opm test
           )

  set_target_properties(opmcommon PROPERTIES POSITION_INDEPENDENT_CODE ON)
  set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_BINARY_DIR}/python/python)

  # -------------------------------------------------------------------------
  # Let cmake configure some small shell scripts which can be used to simplify
  # building, testing and installation of the Python extensions.
  configure_file(python/setup-build.sh.in tmp/setup-build.sh)
  file( COPY ${PROJECT_BINARY_DIR}/tmp/setup-build.sh
        DESTINATION ${PROJECT_BINARY_DIR}
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE )

  configure_file(python/setup-test.sh.in tmp/setup-test.sh)
  file( COPY ${PROJECT_BINARY_DIR}/tmp/setup-test.sh
        DESTINATION ${PROJECT_BINARY_DIR}
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE )

  configure_file(python/setup-install.sh.in tmp/setup-install.sh)
  file( COPY ${PROJECT_BINARY_DIR}/tmp/setup-install.sh
        DESTINATION ${PROJECT_BINARY_DIR}
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE )

  configure_file(python/enable-python.sh.in enable-python.sh)

  # -------------------------------------------------------------------------
  # 2: Embed the Python interpreter for keywords like PYACTION and PYINPUT
  if (OPM_ENABLE_EMBEDDED_PYTHON)
    add_subdirectory(python/pybind11)
    target_include_directories(opmcommon PRIVATE "python/pybind11/include;${PYTHON_INCLUDE_DIRS}")
    target_link_libraries(opmcommon PUBLIC ${PYTHON_LIBRARY})

    add_definitions(-DEMBEDDED_PYTHON)
  endif()
endif()
