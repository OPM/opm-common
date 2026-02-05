# - Identify source code

macro (opm_sources opm)
  # this is necessary to set so that we know where we are going to
  # execute the test programs (and make datafiles available)
  set (tests_DIR "tests")

  # how to retrieve the "fancy" name from the filename
  set (tests_REGEXP
    "^test_([^/]*)$"
    "^([^/]*)_test$"
  )

  # these are the lists that must be defined in CMakeLists_files
  # - MAIN_SOURCE_FILES
  # - EXAMPLE_SOURCE_FILES
  # - TEST_SOURCE_FILES
  # - TEST_DATA_FILES
  # - PUBLIC_HEADER_FILES
  # - PROGRAM_SOURCE_FILES
  # - ADDITIONAL_SOURCE_FILES

  # rename from "friendly" names to ones that fit the "almost-structural"
  # scheme used in the .cmake modules, converting them to absolute file
  # names in the process
  foreach (_file IN LISTS MAIN_SOURCE_FILES)
    list (APPEND ${opm}_SOURCES ${PROJECT_SOURCE_DIR}/${_file})
    # further classify into language if some other modules need to add props
    if (_file MATCHES ".*\\.[cC][a-zA-Z]*$")
      if (_file MATCHES ".*\\.c$")
        list (APPEND ${opm}_C_SOURCES ${PROJECT_SOURCE_DIR}/${_file})
      else (_file MATCHES ".*\\.c$")
        list (APPEND ${opm}_CXX_SOURCES ${PROJECT_SOURCE_DIR}/${_file})
      endif (_file MATCHES ".*\\.c$")
    elseif (_file MATCHES ".*\\.[fF][a-zA-Z]*$")
      list (APPEND ${opm}_Fortran_SOURCES ${PROJECT_SOURCE_DIR}/${_file})
    endif (_file MATCHES ".*\\.[cC][a-zA-Z]*$")
  endforeach (_file)
  foreach (_file IN LISTS PUBLIC_HEADER_FILES)
    list (APPEND ${opm}_HEADERS ${PROJECT_SOURCE_DIR}/${_file})
  endforeach (_file)
  foreach (_file IN LISTS PRIVATE_HEADER_FILES)
    list (APPEND ${opm}_PRIVATE_HEADERS ${PROJECT_SOURCE_DIR}/${_file})
  endforeach (_file)
  foreach (_file IN LISTS GENERATED_HEADER_FILES)
    list (APPEND ${opm}_GENERATED_HEADERS ${PROJECT_BINARY_DIR}/${_file})
  endforeach (_file)
  foreach (_file IN LISTS TEST_SOURCE_FILES)
    list (APPEND tests_SOURCES ${PROJECT_SOURCE_DIR}/${_file})
  endforeach (_file)
  foreach (_file IN LISTS TEST_DATA_FILES)
    list (APPEND tests_DATA ${PROJECT_SOURCE_DIR}/${_file})
  endforeach (_file)
  foreach (_file IN LISTS EXAMPLE_SOURCE_FILES)
    list (APPEND examples_SOURCES ${PROJECT_SOURCE_DIR}/${_file})
  endforeach (_file)
  foreach (_file IN LISTS ADDITIONAL_SOURCE_FILES)
    list (APPEND additionals_SOURCES ${PROJECT_SOURCE_DIR}/${_file})
    list (APPEND additionals_SOURCES_DIST ${PROJECT_SOURCE_DIR}/${_file})
  endforeach (_file)
  foreach (_file IN LISTS PROGRAM_SOURCE_FILES)
    list (APPEND examples_SOURCES_DIST ${PROJECT_SOURCE_DIR}/${_file})
  endforeach (_file)
  foreach (_file IN LISTS ATTIC_FILES)
    list (APPEND attic_SOURCES ${PROJECT_SOURCE_DIR}/${_file})
  endforeach (_file)

  # identify pre-compile header; if the project is called opm-foobar,
  # then it should be in opm/foobar/opm-foobar-pch.hpp
  string (REPLACE "-" "/" opm_NAME_AS_DIR ${${opm}_NAME})
  set (${opm}_PRECOMP_CXX_HEADER "${opm_NAME_AS_DIR}/${${opm}_NAME}-pch.hpp")
  if (NOT EXISTS ${PROJECT_SOURCE_DIR}/${${opm}_PRECOMP_CXX_HEADER})
    set (${opm}_PRECOMP_CXX_HEADER "")
  endif (NOT EXISTS ${PROJECT_SOURCE_DIR}/${${opm}_PRECOMP_CXX_HEADER})
endmacro (opm_sources opm)
