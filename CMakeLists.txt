cmake_minimum_required (VERSION 3.23)
project(opm-common C CXX)

option(SIBLING_SEARCH "Search for other modules in sibling directories?" ON)
option(ENABLE_MOCKSIM "Build the mock simulator for io testing" ON)
option(OPM_ENABLE_PYTHON "Enable python bindings?" OFF)
option(OPM_INSTALL_PYTHON "Install python bindings?" ON)
option(OPM_ENABLE_EMBEDDED_PYTHON "Enable embedded python?" OFF)
option(OPM_ENABLE_DUNE "Enable code requiring dune-common?" ON)

macro(opm-common_dir_hook)
  set(doxy_dir docs/doxygen)
endmacro()

macro(opm-common_prereqs_hook)
  if(TARGET cjson)
    target_link_libraries(opmcommon PRIVATE cjson)
  else()
    include(DownloadCjson)
  endif()

  if(TARGET fmt::fmt)
    target_link_libraries(opmcommon PUBLIC fmt::fmt)
  else()
    include(DownloadFmt)
    DownloadFmt(opmcommon)
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
      target_link_libraries(opmcommon PRIVATE Python3::Python)
    else()
      find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
    endif()
    if(Python3_VERSION_MINOR LESS 3)
      # Python native namespace packages requires python >= 3.3
      message(SEND_ERROR "OPM requires python >= 3.3 but only version ${Python3_VERSION} was found")
    endif()

    # Set the build directory for the python modules.
    get_property(_opm_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    # The build directory for the python modules is set to a configuration specific.
    set(OPM_COMMON_PYTHON_BUILD_DIR ${PROJECT_BINARY_DIR}/python/$<$<BOOL:${_opm_multi_config}>:$<CONFIG>>)
    # Name suffix of per-configuration files kept in the build root, such as
    # the setup-*.sh helper scripts: empty, or -<config> if multi-config.
    set(OPM_PYTHON_SCRIPT_SUFFIX $<$<BOOL:${_opm_multi_config}>:-$<CONFIG>>)

    # Directory to install common (for opm modules) python scripts
    set(OPM_PYTHON_COMMON_DIR "${CMAKE_INSTALL_DATAROOTDIR}/opm/python")
    string(APPEND OPM_PROJECT_EXTRA_CODE_INTREE
      "\n  set(opm-common_PYTHON_COMMON_DIR ${OPM_COMMON_PYTHON_BUILD_DIR})")
    string(APPEND OPM_PROJECT_EXTRA_CODE_INSTALLED
        "\n  set(opm-common_PYTHON_COMMON_DIR ${CMAKE_INSTALL_PREFIX}/${OPM_PYTHON_COMMON_DIR})")

    find_package(pybind11 CONFIG)
    if (NOT pybind11_FOUND)
      include(DownloadPyBind11)
    endif()
  endif()

  if(OPM_ENABLE_DUNE)
    find_package(dune-common REQUIRED)
    target_link_libraries(opmcommon PUBLIC dunecommon)
  endif()
endmacro()

macro(opm-common_config_hook)
  include(CheckIncludeFile)
  check_include_file(fnmatch.h FNMATCH_H_FOUND)
  if (FNMATCH_H_FOUND)
    target_compile_definitions(opmcommon PRIVATE HAVE_FNMATCH_H=1)
  endif()

  target_compile_definitions(opmcommon INTERFACE HAVE_OPM_COMMON=1)
endmacro()

macro(opm-common_sources_hook)
  # Keyword generation
  include(GenerateKeywords.cmake)

  # Append generated sources
  list(INSERT opm-common_SOURCES 0 ${PROJECT_BINARY_DIR}/generated/ParserInit.cpp)
  foreach (name A B C D E F G H I J K L M N O P Q R S T U V W X Y Z)
    list(INSERT opm-common_SOURCES 0 ${PROJECT_BINARY_DIR}/generated/ParserKeywords/${name}.cpp)
    list(INSERT opm-common_SOURCES 0 ${PROJECT_BINARY_DIR}/generated/ParserKeywords/ParserInit${name}.cpp)
    list(INSERT opm-common_SOURCES 0 ${PROJECT_BINARY_DIR}/generated/ParserKeywords/Builtin${name}.cpp)
  endforeach()

  if (OPM_ENABLE_EMBEDDED_PYTHON)
    list(INSERT opm-common_SOURCES 0 ${PROJECT_BINARY_DIR}/generated/python/cxx/builtin_pybind11.cpp)
    set_source_files_properties(${PYTHON_CXX_SOURCE_FILES} PROPERTIES COMPILE_FLAGS -Wno-shadow)
    set_source_files_properties(opm/input/eclipse/Python/PythonInterp.cpp PROPERTIES COMPILE_FLAGS -Wno-shadow)
    set_source_files_properties(opm/input/eclipse/Schedule/Action/PyAction.cpp PROPERTIES COMPILE_FLAGS -Wno-shadow)
  endif()

  set_source_files_properties(
    opm/input/eclipse/Python/Python.cpp
    PROPERTIES
    COMPILE_FLAGS
      -Wno-shadow
  )
  if (OPM_ENABLE_PYTHON)
    # Set the path to the input docstrings.json file and the output .hpp file
    set(PYTHON_DOCSTRINGS_FILE "${PROJECT_SOURCE_DIR}/python/docstrings_common.json")
    set(PYTHON_DOCSTRINGS_GENERATED_HPP "${PROJECT_BINARY_DIR}/generated/include/python/cxx/OpmCommonPythonDoc.hpp")
    file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/generated/include/python/cxx")
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

macro(opm-common_files_hook)
  if (OPM_ENABLE_PYTHON)
    make_directory(${OPM_COMMON_PYTHON_BUILD_DIR})
    set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${OPM_COMMON_PYTHON_BUILD_DIR})
    set(opm-common_PYTHON_PACKAGE_VERSION ${OPM_PYTHON_PACKAGE_VERSION_TAG})

    # Generate a versioned setup.py per configuration, next to the package it
    # describes. The other packaging files are copied by copy_python.
    configure_file(${PROJECT_SOURCE_DIR}/python/setup.py.in
                   ${PROJECT_BINARY_DIR}/tmp_gen/setup.py)
    file(
      GENERATE
        OUTPUT
          ${OPM_COMMON_PYTHON_BUILD_DIR}/setup.py
        INPUT
          ${PROJECT_BINARY_DIR}/tmp_gen/setup.py
    )

    # Environment script, one per configuration (library dir differs).
    configure_file(python/enable-python.sh.in tmp_gen/enable-python.sh @ONLY)
    file(
      GENERATE
        OUTPUT
          ${OPM_COMMON_PYTHON_BUILD_DIR}/enable-python.sh
        INPUT
          ${PROJECT_BINARY_DIR}/tmp_gen/enable-python.sh
    )

    # -------------------------------------------------------------------------
    # Let cmake configure some small shell scripts which can be used to simplify
    # building, testing and installation of the Python extensions. One set per
    # configuration: setup-<step>.sh, or setup-<step>-<config>.sh with a
    # multi-config generator. configure_file fills the @VAR@ placeholders,
    # file(GENERATE) then evaluates $<CONFIG> per configuration.
    foreach(_script setup-build setup-test setup-install)
      configure_file(python/${_script}.sh.in tmp_gen/${_script}.sh @ONLY)
      file(
        GENERATE
          OUTPUT
            ${PROJECT_BINARY_DIR}/${_script}${OPM_PYTHON_SCRIPT_SUFFIX}.sh
          INPUT
            ${PROJECT_BINARY_DIR}/tmp_gen/${_script}.sh
          FILE_PERMISSIONS
            OWNER_READ OWNER_WRITE OWNER_EXECUTE
      )
    endforeach()
  endif()
endmacro()

macro(opm-common_tests_hook)
  # Add the tests
  opm_add_test(test_EclFilesComparator
    CONDITION
      TARGET Boost::unit_test_framework
    SOURCES
      tests/test_EclFilesComparator.cpp
      test_util/EclFilesComparator.cpp
    LIBRARIES
      opmcommon
      Boost::unit_test_framework
    WORKING_DIRECTORY
      ${PROJECT_BINARY_DIR}/tests
  )

  opm_add_test(test_EclRegressionTest
    CONDITION
      TARGET Boost::unit_test_framework
    SOURCES
      tests/test_EclRegressionTest.cpp
      test_util/EclFilesComparator.cpp
      test_util/EclRegressionTest.cpp
    LIBRARIES
      opmcommon
      Boost::unit_test_framework
    WORKING_DIRECTORY
      ${PROJECT_BINARY_DIR}/tests
  )

  include(ExtraTests.cmake)

  if(ENABLE_MOCKSIM AND TARGET Boost::unit_test_framework)
    foreach(test test_msim test_msim_ACTIONX test_msim_EXIT)
      opm_add_test(${test}
        SOURCES
          tests/msim/${test}.cpp
        LIBRARIES
          mocksim
          opmcommon
          Boost::unit_test_framework
        WORKING_DIRECTORY
          ${PROJECT_BINARY_DIR}/tests
      )
    endforeach()
  endif()

  if(OPM_ENABLE_PYTHON)
    add_test(
      NAME
        python_tests
      WORKING_DIRECTORY
        ${OPM_COMMON_PYTHON_BUILD_DIR}
      COMMAND
        ${Python3_EXECUTABLE} -m unittest discover
    )
    set_tests_properties(python_tests
      PROPERTIES
      ENVIRONMENT_MODIFICATION
        PYTHONPATH=path_list_prepend:${OPM_COMMON_PYTHON_BUILD_DIR}
    )

    # The helper scripts generated for the configuration ctest runs with must
    # exist and parse. bash -n only checks syntax, so nothing is executed.
    find_program(_opm_bash bash)
    if(_opm_bash)
      foreach(_script setup-build setup-test setup-install)
        add_test(
          NAME
            python_${_script}_script
          COMMAND
            ${_opm_bash} -n ${PROJECT_BINARY_DIR}/${_script}${OPM_PYTHON_SCRIPT_SUFFIX}.sh
        )
      endforeach()
    endif()
 endif()
endmacro()

macro(opm-common_targets_hook)
  if(NOT cJSON_FOUND) # TODO: Remove once json is split out
    target_sources(opmcommon PRIVATE $<TARGET_OBJECTS:cjson>)
    target_include_directories(opmcommon PRIVATE ${CMAKE_BINARY_DIR}/_deps)
  endif()
  target_include_directories(opmcommon PRIVATE ${PROJECT_BINARY_DIR}/generated/include)

  target_sources(compareECL
    PRIVATE
      test_util/EclFilesComparator.cpp
      test_util/EclRegressionTest.cpp
  )

  if(ENABLE_MOCKSIM)
    opm_add_library(
      TARGET
        mocksim
      SOURCES
        msim/src/msim.cpp
      LIBRARIES
        opmcommon
      TYPE
        STATIC
    )
    target_include_directories(mocksim PUBLIC msim/include)
    opm_add_executable(
      TARGET
        msim
      SOURCES
        examples/msim.cpp
      LIBRARIES
        mocksim
    )
  endif()

  list(APPEND opm-common_EXTRA_TARGETS compareECL rst_deck)

  if(TARGET Boost::unit_test_framework)
    foreach(test ACTIONX EmbeddedPython msim_ACTIONX PYACTION)
      if(TEST ${test})
        set_tests_properties(${test}
          PROPERTIES
          ENVIRONMENT_MODIFICATION
            PYTHONPATH=path_list_prepend:${OPM_COMMON_PYTHON_BUILD_DIR}
        )
      endif()
    endforeach()
  endif()

  if(OPM_ENABLE_PYTHON)
    # Copy the python source files to the build directory, so that they can be installed from there
    # together with the generated files.
    set(_copy_python_commands)
    foreach(_dir opm opm_embedded tests examples)
      list(APPEND _copy_python_commands
        COMMAND
          $<TARGET_FILE:Python3::Interpreter>
          ${PROJECT_SOURCE_DIR}/python/install.py
          ${PROJECT_SOURCE_DIR}/python/${_dir}
          ${OPM_COMMON_PYTHON_BUILD_DIR}
          0
      )
    endforeach()
    # Packaging files read by the generated setup.py (setup-*.sh, wheel builds).
    list(APPEND _copy_python_commands
      COMMAND
        ${CMAKE_COMMAND} -E copy_if_different
        ${PROJECT_SOURCE_DIR}/python/README.md
        ${PROJECT_SOURCE_DIR}/python/MANIFEST.in
        ${PROJECT_SOURCE_DIR}/python/requirements.txt
        ${PROJECT_SOURCE_DIR}/python/pyproject.toml
        ${OPM_COMMON_PYTHON_BUILD_DIR}
    )
    add_custom_target(copy_python ALL ${_copy_python_commands})

    pybind11_add_module(opmcommon_python
                        ${PYTHON_CXX_SOURCE_FILES}
                        ${PROJECT_BINARY_DIR}/generated/python/cxx/builtin_pybind11.cpp)

    target_include_directories(opmcommon_python PRIVATE ${PROJECT_BINARY_DIR}/generated/include)
    target_link_libraries(opmcommon_python PRIVATE opmcommon pybind11::module)
    opm_add_target_options(TARGET opmcommon_python)
    set_target_properties(opmcommon_python
      PROPERTIES
      LIBRARY_OUTPUT_DIRECTORY
        ${OPM_COMMON_PYTHON_BUILD_DIR}/opm
    )
    add_dependencies(opmcommon_python copy_python)

    set_target_properties(opmcommon PROPERTIES POSITION_INDEPENDENT_CODE ON)
  endif()

  if(OPM_ENABLE_EMBEDDED_PYTHON)
    target_include_directories(opmcommon SYSTEM PRIVATE ${pybind11_INCLUDE_DIRS})
    foreach(target opmcommon test_msim_ACTIONX EmbeddedPython PYACTION)
      if(TARGET ${target})
        target_compile_definitions(${target} PRIVATE EMBEDDED_PYTHON=1)
      endif()
    endforeach()
    set_target_properties(opmcommon
      PROPERTIES
        EMBEDDED_PYTHON ON
      EXPORT_PROPERTIES
        EMBEDDED_PYTHON
    )
  endif()
endmacro()

macro(opm-common_install_hook)
  # Install build system files and documentation
  install(
    DIRECTORY
      cmake
    DESTINATION
      ${CMAKE_INSTALL_DATADIR}/opm
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
      ${CMAKE_INSTALL_DATADIR}/opm/etc
  )

  install(
    DIRECTORY
      docs/man1
    DESTINATION
      ${CMAKE_INSTALL_MANDIR}
    FILES_MATCHING PATTERN
      "*.1"
  )

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
        ${PYTHON_INSTALL_PREFIX}/opm
    )
    if (OPM_INSTALL_PYTHON)
      install(
        CODE
          "execute_process(
             COMMAND
               $<TARGET_FILE:Python3::Interpreter>
               python/install.py
               ${OPM_COMMON_PYTHON_BUILD_DIR}/opm
               ${CMAKE_INSTALL_PREFIX}/${PYTHON_INSTALL_PREFIX}
               1
           )"
      )
    endif()
    if (OPM_ENABLE_EMBEDDED_PYTHON)
      install(
        CODE
          "execute_process(
             COMMAND
               $<TARGET_FILE:Python3::Interpreter>
               python/install.py
               ${OPM_COMMON_PYTHON_BUILD_DIR}/opm_embedded
               ${CMAKE_INSTALL_PREFIX}/${PYTHON_INSTALL_PREFIX}
               1
           )"
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
endmacro()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)
set(OPM_MACROS_ROOT ${PROJECT_SOURCE_DIR})

if(NOT OPM_ENABLE_PYTHON)
  set(OPM_INSTALL_PYTHON OFF)
endif()

include(GNUInstallDirs)
# We need to define this variable in the installed cmake config file.
set(OPM_PROJECT_EXTRA_CODE_INSTALLED
  "set(OPM_MACROS_ROOT ${CMAKE_INSTALL_FULL_DATADIR}/opm)
  list(APPEND CMAKE_MODULE_PATH \${OPM_MACROS_ROOT}/cmake/Modules)
  include(OpmPolicies)
  OpmSetPolicies()"
)

set(OPM_PROJECT_EXTRA_CODE_INTREE
  "set(OPM_MACROS_ROOT ${OPM_MACROS_ROOT})
  list(APPEND CMAKE_MODULE_PATH \${OPM_MACROS_ROOT}/cmake/Modules)
  include(OpmPolicies)
  OpmSetPolicies()"
)

# project information is in dune.module. Read this file and set variables.
# we cannot generate dune.module since it is read by dunecontrol before
# the build starts, so it makes sense to keep the data there then.
include (OpmInit)

# Look for the opm-tests repository; if found the variable
# HAVE_OPM_TESTS will be set to true.
include(Findopm-tests)

if(OPM_ENABLE_EMBEDDED_PYTHON AND NOT OPM_ENABLE_PYTHON)
  # This needs to be here to run before source_hook
  message(WARNING "Inconsistent settings: OPM_ENABLE_PYTHON=OFF and "
    "OPM_ENABLE_EMBEDDED_PYTHON=ON. OPM_ENABLE_EMBEDDED_PYTHON=ON now implies OPM_ENABLE_PYTHON=ON.")
  set(OPM_ENABLE_PYTHON ON CACHE BOOL "Enable python bindings?" FORCE)
endif()

# all setup common to the OPM library modules is done here
include (OpmLibMain)
