cmake_minimum_required (VERSION 3.23)
project(opm-common C CXX)

option(SIBLING_SEARCH "Search for other modules in sibling directories?" ON)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)
set(OPM_MACROS_ROOT ${PROJECT_SOURCE_DIR})

option(ENABLE_ECL_INPUT "Enable eclipse input support?" ON)
option(ENABLE_ECL_OUTPUT "Enable eclipse output support?" ON)
option(ENABLE_MOCKSIM "Build the mock simulator for io testing" ON)
option(OPM_ENABLE_PYTHON "Enable python bindings?" OFF)
option(OPM_INSTALL_PYTHON "Install python bindings?" ON)
option(OPM_ENABLE_EMBEDDED_PYTHON "Enable embedded python?" OFF)
option(OPM_ENABLE_DUNE "Enable code requiring dune-common?" ON)

# Output implies input
if(ENABLE_ECL_OUTPUT)
  set(ENABLE_ECL_INPUT ON)
endif()

# And likewise, no input means no output
if(NOT ENABLE_ECL_INPUT)
  set(ENABLE_ECL_OUTPUT OFF)
endif()

# We need to define this variable in the installed cmake config file.
set(OPM_PROJECT_EXTRA_CODE_INSTALLED  "#ENABLE_ECL_INPUT is needed by opm-common-prereq.cmake
                                       set(ENABLE_ECL_INPUT ${ENABLE_ECL_INPUT})
                                       set(OPM_MACROS_ROOT ${CMAKE_INSTALL_PREFIX}/share/opm)
                                       list(APPEND CMAKE_MODULE_PATH \${OPM_MACROS_ROOT}/cmake/Modules)
                                       include(OpmPackage) #Make macros available after find_package(opm-common)")

set(OPM_PROJECT_EXTRA_CODE_INTREE "#ENABLE_ECL_INPUT is needed by opm-common-prereq.cmake
                                   set(ENABLE_ECL_INPUT ${ENABLE_ECL_INPUT})
                                   set(OPM_MACROS_ROOT ${OPM_MACROS_ROOT})
                                   list(APPEND CMAKE_MODULE_PATH \${OPM_MACROS_ROOT}/cmake/Modules)
                                   include(OpmPackage) #Make macros available after find_package(opm-common)")

# project information is in dune.module. Read this file and set variables.
# we cannot generate dune.module since it is read by dunecontrol before
# the build starts, so it makes sense to keep the data there then.
include (OpmInit)
OpmSetPolicies()

# Look for the opm-tests repository; if found the variable
# HAVE_OPM_TESTS will be set to true.
include(Findopm-tests)

# list of prerequisites for this particular project; this is in a
# separate file (in cmake/Modules sub-directory) because it is shared
# with the find module
include (${project}-prereqs)

if(OPM_ENABLE_EMBEDDED_PYTHON AND NOT OPM_ENABLE_PYTHON)
  # This needs to be here to run before source_hook
  message(WARNING "Inconsistent settings: OPM_ENABLE_PYTHON=OFF and "
    "OPM_ENABLE_EMBEDDED_PYTHON=ON. OPM_ENABLE_EMBEDDED_PYTHON=ON now implies OPM_ENABLE_PYTHON=ON.")
  set(OPM_ENABLE_PYTHON ON CACHE BOOL "Enable python bindings?" FORCE)
endif()
# read the list of components from this file (in the project directory);
# it should set various lists with the names of the files to include
include (CMakeLists_files.cmake)

macro(${project}_prereqs_hook)
  if(NOT TARGET cjson)
    include(DownloadCjson)
  endif()

  if(TARGET fmt::fmt)
    # OpmSatellites will not add the library, do it here.
    list(APPEND opm-common_LIBRARIES fmt::fmt)
  else()
    include(DownloadFmt)
  endif()

  # If opm-common is configured to embed the python interpreter we must make sure
  # that all downstream modules link libpython transitively. Due to the required
  # integration with Python+cmake machinery provided by pybind11 this is done by
  # manually adding to the opm-common_LIBRARIES variable here, and not in the
  # OpmLibMain function. Here only the library dependency is implemented, the
  # bulk of the python configuration is further down in the file.
  if (OPM_ENABLE_PYTHON)
    # Be backwards compatible.
    if(PYTHON_EXECUTABLE AND NOT Python3_EXECUTABLE)
      set(Python3_EXECUTABLE ${PYTHON_EXECUTABLE})
    endif()
    # We always need to search for Development as we use
    # pybind11_add_module even if don't embed Python
    if (OPM_ENABLE_EMBEDDED_PYTHON)
      find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Embed Development.Module)
      get_target_property(_lib_path Python3::Python IMPORTED_LOCATION)
      set(PYTHON_LIBRARY ${_lib_path})
      list(APPEND opm-common_LIBRARIES ${_lib_path})
    else()
      find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
    endif()
    if(Python3_VERSION_MINOR LESS 3)
      # Python native namespace packages requires python >= 3.3
      message(SEND_ERROR "OPM requires python >= 3.3 but only version ${Python3_VERSION} was found")
    endif()

    # Directory to install common (for opm modules) python scripts
    include (GNUInstallDirs)
    set(OPM_PYTHON_COMMON_DIR "${CMAKE_INSTALL_DATAROOTDIR}/opm/python")
    set(OPM_PROJECT_EXTRA_CODE_INTREE
      "${OPM_PROJECT_EXTRA_CODE_INTREE}
       set(opm-common_PYTHON_COMMON_DIR ${PROJECT_SOURCE_DIR}/python)")
    set(OPM_PROJECT_EXTRA_CODE_INSTALLED
        "${OPM_PROJECT_EXTRA_CODE_INSTALLED}
         set(opm-common_PYTHON_COMMON_DIR ${CMAKE_INSTALL_PREFIX}/${OPM_PYTHON_COMMON_DIR})")

    find_package(pybind11 CONFIG)
    if (NOT pybind11_FOUND)
      include(DownloadPyBind11)
    endif()
  endif()
endmacro()

macro(${project}_config_hook)
  if(ENABLE_ECL_INPUT)
    include_directories(${EXTRA_INCLUDES} ${PROJECT_BINARY_DIR})
    set(OPM_PROJECT_EXTRA_CODE_INTREE "${OPM_PROJECT_EXTRA_CODE_INTREE}
                                        list(APPEND opm-common_INCLUDE_DIRS ${EXTRA_INCLUDES} ${PROJECT_BINARY_DIR})")
    if(ENABLE_ECL_INPUT)
      set(OPM_PROJECT_EXTRA_CODE_INTREE "${OPM_PROJECT_EXTRA_CODE_INTREE}
                                         set(HAVE_ECL_INPUT 1)")
      set(OPM_PROJECT_EXTRA_CODE_INSTALLED "${OPM_PROJECT_EXTRA_CODE_INSTALLED}
                                            set(HAVE_ECL_INPUT 1)")
      set(HAVE_ECL_INPUT 1)
    endif()
    if(ENABLE_ECL_OUTPUT)
      set(OPM_PROJECT_EXTRA_CODE_INTREE "${OPM_PROJECT_EXTRA_CODE_INTREE}
                                         set(HAVE_ECL_OUTPUT 1)")
      set(OPM_PROJECT_EXTRA_CODE_INSTALLED "${OPM_PROJECT_EXTRA_CODE_INSTALLED}
                                            set(HAVE_ECL_OUTPUT 1)")
    endif()
  endif()

  include(CheckIncludeFile)
  check_include_file(fnmatch.h FNMATCH_H_FOUND)
  if (FNMATCH_H_FOUND)
    set(HAVE_FNMATCH_H 1)
  endif()

  if(OPM_ENABLE_DUNE)
    find_package(dune-common REQUIRED)
    opm_need_version_of ("dune-common")
  endif()
endmacro()

macro(${project}_sources_hook)
  if(ENABLE_ECL_INPUT)
    # Keyword generation
    include(GenerateKeywords.cmake)

    # Append generated sources
    list(INSERT opm-common_SOURCES 0 ${PROJECT_BINARY_DIR}/ParserInit.cpp)
    foreach (name A B C D E F G H I J K L M N O P Q R S T U V W X Y Z)
        list(INSERT opm-common_SOURCES 0 ${PROJECT_BINARY_DIR}/ParserKeywords/${name}.cpp)
        list(INSERT opm-common_SOURCES 0 ${PROJECT_BINARY_DIR}/ParserKeywords/ParserInit${name}.cpp)
        list(INSERT opm-common_SOURCES 0 ${PROJECT_BINARY_DIR}/ParserKeywords/Builtin${name}.cpp)
    endforeach()
    if (OPM_ENABLE_EMBEDDED_PYTHON)
      list(INSERT opm-common_SOURCES 0 ${PROJECT_BINARY_DIR}/python/cxx/builtin_pybind11.cpp)
    endif()
  endif()
  set_source_files_properties(src/opm/input/eclipse/Python/Python.cpp
                              PROPERTIES COMPILE_FLAGS -Wno-shadow)
  if (OPM_ENABLE_PYTHON)
    # Set the path to the input docstrings.json file and the output .hpp file
    set(PYTHON_DOCSTRINGS_FILE "${PROJECT_SOURCE_DIR}/python/docstrings_common.json")
    set(PYTHON_DOCSTRINGS_GENERATED_HPP "${PROJECT_BINARY_DIR}/python/cxx/OpmCommonPythonDoc.hpp")
    # Command to run the Python script
    add_custom_command(
        OUTPUT
          ${PYTHON_DOCSTRINGS_GENERATED_HPP}
        COMMAND
          $<TARGET_FILE:Python3::Interpreter>
          ${PROJECT_SOURCE_DIR}/python/generate_docstring_hpp.py
          ${PYTHON_DOCSTRINGS_FILE}
          ${PYTHON_DOCSTRINGS_GENERATED_HPP}
          OPMCOMMONPYTHONDOC_HPP
          "Opm::Common::DocStrings"
        DEPENDS
          ${PYTHON_DOCSTRINGS_FILE}
        COMMENT
          "Generating OpmCommonPythonDoc.hpp from JSON file"
    )
    list(INSERT opm-common_SOURCES 0 ${PYTHON_DOCSTRINGS_GENERATED_HPP})
  endif()

  if(QuadMath_FOUND)
    get_target_property(qm_defs QuadMath::QuadMath INTERFACE_COMPILE_DEFINITIONS)
    if(qm_defs)
      list(APPEND qm_defs HAVE_QUAD=1)
    else()
      set(qm_defs HAVE_QUAD=1)
    endif()
    get_target_property(qm_options QuadMath::QuadMath INTERFACE_COMPILE_OPTIONS)
    set_source_files_properties(opm/material/components/CO2.cpp
                                opm/material/densead/Evaluation.cpp
                                PROPERTIES COMPILE_DEFINITIONS "${qm_defs}"
                                COMPILE_OPTIONS "${qm_options}")
  endif()
endmacro()

macro(${project}_files_hook)
  if (OPM_ENABLE_PYTHON)
    make_directory(${PROJECT_BINARY_DIR}/python)

    file(
      COPY
        ${PROJECT_SOURCE_DIR}/python/README.md
        ${PROJECT_SOURCE_DIR}/python/MANIFEST.in
      DESTINATION
        ${PROJECT_BINARY_DIR}/python
    )

    # Generate versioned setup.py
    configure_file(${PROJECT_SOURCE_DIR}/python/setup.py.in
                   ${PROJECT_BINARY_DIR}/python/setup.py.tmp)
    file(
      GENERATE
        OUTPUT
          ${PROJECT_BINARY_DIR}/python/setup.py
        INPUT
          ${PROJECT_BINARY_DIR}/python/setup.py.tmp
    )

    # -------------------------------------------------------------------------
    # Let cmake configure some small shell scripts which can be used to simplify
    # building, testing and installation of the Python extensions.
    configure_file(python/setup-build.sh.in tmp/setup-build.sh)
    file(
      COPY
        ${PROJECT_BINARY_DIR}/tmp/setup-build.sh
      DESTINATION
        ${PROJECT_BINARY_DIR}
       FILE_PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
    )

    configure_file(python/setup-test.sh.in tmp/setup-test.sh)
    file(
      COPY
        ${PROJECT_BINARY_DIR}/tmp/setup-test.sh
      DESTINATION
        ${PROJECT_BINARY_DIR}
      FILE_PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
    )

    configure_file(python/setup-install.sh.in tmp/setup-install.sh)
    file(
      COPY
        ${PROJECT_BINARY_DIR}/tmp/setup-install.sh
      DESTINATION
        ${PROJECT_BINARY_DIR}
      FILE_PERMISSIONS
        OWNER_READ OWNER_WRITE OWNER_EXECUTE
    )

    configure_file(python/enable-python.sh.in enable-python.sh)
  endif()
endmacro()

macro(${project}_tests_hook)
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

    add_executable(summary
      test_util/summary.cpp
      )

    add_executable(arraylist
      test_util/arraylist.cpp
      )

    add_executable(rewriteEclFile
      test_util/rewriteEclFile.cpp
      )

    foreach(target compareECL convertECL summary rewriteEclFile arraylist)
      opm_add_target_options(TARGET ${target})
      target_link_libraries(${target} PRIVATE opmcommon)
      install(TARGETS ${target} DESTINATION bin)
    endforeach()
    list(APPEND opm-common_EXTRA_TARGETS compareECL rst_deck)

    # Add the tests
    set(_libs opmcommon Boost::unit_test_framework)

    opm_add_test(test_EclFilesComparator
      CONDITION
        ENABLE_ECL_INPUT AND TARGET Boost::unit_test_framework
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
        ENABLE_ECL_INPUT AND TARGET Boost::unit_test_framework
      SOURCES
        tests/test_EclRegressionTest.cpp
        test_util/EclFilesComparator.cpp
        test_util/EclRegressionTest.cpp
      LIBRARIES
        ${_libs}
      WORKING_DIRECTORY
        ${PROJECT_BINARY_DIR}/tests
      )

    include(ExtraTests.cmake)
  endif()

  if (dune-common_FOUND)
    list(APPEND tests_SOURCES ${DUNE_TEST_SOURCE_FILES})
  endif()

  if(OPM_ENABLE_PYTHON)
    add_test(
      NAME
        python_tests
      WORKING_DIRECTORY
        ${PROJECT_BINARY_DIR}/python
      COMMAND
        ${Python3_EXECUTABLE} -m unittest discover
    )
 endif()
endmacro()

macro(${project}_install_hook)
  # Install build system files and documentation
  install(
    DIRECTORY
      cmake
    DESTINATION
      share/opm
    USE_SOURCE_PERMISSIONS
    PATTERN
      "OPM-CMake.md" EXCLUDE
  )

  install(
    FILES
      cmake/OPM-CMake.md
    DESTINATION
      ${CMAKE_INSTALL_DOCDIR}
  )

  # Install tab completion skeleton
  install(
    FILES
      etc/opm_bash_completion.sh.in
    DESTINATION
      share/opm/etc
  )

  install(
    DIRECTORY
      docs/man1
    DESTINATION
      ${CMAKE_INSTALL_MANDIR}
    FILES_MATCHING PATTERN
      "*.1"
  )
endmacro()

# all setup common to the OPM library modules is done here
include (OpmLibMain)

if(NOT cJSON_FOUND) # TODO: Remove once json is split out
  target_sources(opmcommon PRIVATE $<TARGET_OBJECTS:cjson>)
  target_include_directories(opmcommon PRIVATE ${CMAKE_BINARY_DIR}/_deps)
endif()

if (ENABLE_MOCKSIM AND ENABLE_ECL_INPUT)
  add_library(mocksim
              msim/src/msim.cpp)
  opm_add_target_options(TARGET mocksim)
  target_link_libraries(mocksim PUBLIC opmcommon)
  target_include_directories(mocksim PUBLIC msim/include)
  add_executable(msim examples/msim.cpp)
  opm_add_target_options(TARGET msim)
  target_link_libraries(msim PRIVATE mocksim)

  if (TARGET Boost::unit_test_framework)
    set(_libs mocksim opmcommon Boost::unit_test_framework)

    foreach( test test_msim test_msim_ACTIONX test_msim_EXIT)
      opm_add_test(${test} SOURCES tests/msim/${test}.cpp
        LIBRARIES ${_libs}
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/tests
        CONDITION ${HAVE_ECL_INPUT})
    endforeach()
    set_tests_properties(msim_ACTIONX PROPERTIES
                        ENVIRONMENT "PYTHONPATH=${PROJECT_BINARY_DIR}/python:$ENV{PYTHONPATH}")
  endif()
endif()

if(dune-common_FOUND)
  # Explicitly link tests needing dune-common.
  # To avoid pulling dune-common into the opm-common interface
  target_include_directories(dunecommon INTERFACE ${dune-common_INCLUDE_DIRS})
  target_include_directories(opmcommon PRIVATE ${dune-common_INCLUDE_DIRS})
  string(REPLACE " " ";" dflags "${dune-common_CXX_FLAGS}")
  target_compile_options(dunecommon INTERFACE ${dflags})
  target_compile_definitions(dunecommon INTERFACE DUNE_COMMON_VERSION_MAJOR=${DUNE_COMMON_VERSION_MAJOR})
  target_compile_definitions(dunecommon INTERFACE DUNE_COMMON_VERSION_MINOR=${DUNE_COMMON_VERSION_MINOR})
  target_compile_definitions(dunecommon INTERFACE DUNE_COMMON_VERSION_REVISION=${DUNE_COMMON_VERSION_REVISION})
  if(TARGET Boost::unit_test_framework)
    foreach(src ${DUNE_TEST_SOURCE_FILES})
      get_filename_component(tgt ${src} NAME_WE)
      target_link_libraries(${tgt} PRIVATE dunecommon)
    endforeach()

    foreach(tst test_SymmTensor test_densead test_EvaluationFormat
        test_binarycoefficients test_components test_blackoilfluidstate
        test_blackoilfluidsystem_nonstatic test_fluidmatrixinteractions
        test_fluidsystems test_tabulation test_h2brinepvt test_co2brinepvt
        test_eclblackoilfluidsystem test_eclblackoilfluidsystemnonstatic
        test_eclblackoilpvt)
      opm_add_target_options(TARGET ${tst})
      target_link_libraries(${tst} PRIVATE dunecommon)
    endforeach()
  endif()
  if(BUILD_EXAMPLES)
    target_link_libraries(co2brinepvt PRIVATE dunecommon)
    opm_add_target_options(TARGET co2brinepvt)
  endif()
endif()

if(TARGET Boost::unit_test_framework)
  foreach(test ACTIONX EmbeddedPython PYACTION)
    set_tests_properties(${test} PROPERTIES
                         ENVIRONMENT "PYTHONPATH=${PROJECT_BINARY_DIR}/python:$ENV{PYTHONPATH}")
  endforeach()
endif()

if (OPM_ENABLE_PYTHON)
  set(opm-common_PYTHON_PACKAGE_VERSION ${OPM_PYTHON_PACKAGE_VERSION_TAG})

  add_custom_target(copy_python ALL
    COMMAND
      $<TARGET_FILE:Python3::Interpreter>
      ${PROJECT_SOURCE_DIR}/python/install.py
      ${PROJECT_SOURCE_DIR}/python
      ${PROJECT_BINARY_DIR}
      0
  )

  pybind11_add_module(opmcommon_python
                      ${PYTHON_CXX_SOURCE_FILES}
                      ${PROJECT_BINARY_DIR}/python/cxx/builtin_pybind11.cpp)

  target_link_libraries(opmcommon_python PRIVATE
                        opmcommon)
  if(TARGET pybind11::pybind11)
    target_link_libraries(opmcommon_python PRIVATE
                          pybind11::pybind11)
    opm_add_target_options(TARGET opmcommon_python)
  else()
    target_include_directories(opmcommon_python SYSTEM PRIVATE ${pybind11_INCLUDE_DIRS})
  endif()
  set_target_properties(opmcommon_python PROPERTIES
                        LIBRARY_OUTPUT_DIRECTORY python/opm)
  add_dependencies(opmcommon_python copy_python)

  # Since the installation of Python code is nonstandard it is protected by an
  # extra cmake switch, OPM_INSTALL_PYTHON. If you prefer you can still invoke
  # setup.py install manually - optionally with the generated script
  # setup-install.sh - and completely bypass cmake in the installation phase.
  # If OPM_ENABLE_EMBEDDED_PYTHON is enabled, we also install opmcommon_python,
  # to make it available in a Python console and for e.g. opm-simulators.
  if (OPM_INSTALL_PYTHON OR OPM_ENABLE_EMBEDDED_PYTHON)
      include(PyInstallPrefix)
      install(
        TARGETS
          opmcommon_python
        DESTINATION
          ${DEST_PREFIX}${CMAKE_INSTALL_PREFIX}/${PYTHON_INSTALL_PREFIX}/opm
      )
      if (OPM_INSTALL_PYTHON)
        install(
          CODE
            "execute_process(
               COMMAND
                 $<TARGET_FILE:Python3::Interpreter>
                 python/install.py
                 ${PROJECT_BINARY_DIR}/python/opm
                 ${DEST_PREFIX}${CMAKE_INSTALL_PREFIX}/${PYTHON_INSTALL_PREFIX}
                 1)"
        )
      endif()
      if (OPM_ENABLE_EMBEDDED_PYTHON)
        install(
          CODE
            "execute_process(
               COMMAND
                 $<TARGET_FILE:Python3::Interpreter>
                 python/install.py
                 ${PROJECT_BINARY_DIR}/python/opm_embedded
                 ${DEST_PREFIX}${CMAKE_INSTALL_PREFIX}/${PYTHON_INSTALL_PREFIX}
                 1)"
        )
      endif()
      # Need to install this Python script such that it can be used by
      # opm-simulators when building against an installed opm-common
      install(
        PROGRAMS
          python/install.py
        DESTINATION
          ${OPM_PYTHON_COMMON_DIR}
      )
  endif()

  # if OPM_ENABLE_EMBEDDED_PYTHON is true, then OPM_ENABLE_PYTHON is also automatically true
  if (OPM_ENABLE_PYTHON OR OPM_INSTALL_PYTHON)
    ## Need to install this Python script such that it can be used
    # by opm-simulators when building against an installed opm-common
    install(
      PROGRAMS
        python/generate_docstring_hpp.py
      DESTINATION
        ${OPM_PYTHON_COMMON_DIR}
    )
    install(
      FILES
        python/docstrings_common.json
      DESTINATION
        ${OPM_PYTHON_COMMON_DIR}
    )
  endif()

  set_target_properties(opmcommon PROPERTIES POSITION_INDEPENDENT_CODE ON)
  set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${PROJECT_BINARY_DIR}/python)

  # -------------------------------------------------------------------------
  # 2: Embed the Python interpreter for keywords like PYACTION and PYINPUT
  target_include_directories(opmcommon SYSTEM PRIVATE "${pybind11_INCLUDE_DIRS}")
  if (OPM_ENABLE_EMBEDDED_PYTHON)
    target_link_libraries(opmcommon PUBLIC ${PYTHON_LIBRARY})
    add_definitions(-DEMBEDDED_PYTHON)
  endif()
endif()
