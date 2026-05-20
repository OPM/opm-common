# - Create config.h based on a list of variables
#
# Synopsis:
#   configure_vars (FILE filename verb varlist)
# where
#   filename      Full path (including name) of config.h
#   verb          WRITE or APPEND if truncating or not
#   varlist       List of variable names that has been defined
#
# In addition, this function will define HAVE_CONFIG_H for the
# following compilations, (only) if the filename is "config.h".
#
# Example:
#   list (APPEND FOO_CONFIG_VARS
#     "HAVE_BAR"
#     "HAVE_BAR_VERSION_2"
#     )
#   configure_vars (
#     FILE  ${PROJECT_BINARY_DIR}/config.h
#     WRITE ${FOO_CONFIG_VARS}
#     )

# Copyright (C) 2012 Uni Research AS
# This file is licensed under the GNU General Public License v3.0

function(configure_vars)
  cmake_parse_arguments(PARAM "" "TARGET;FILE" "VARIABLES" ${ARGN})
  if(NOT PARAM_TARGET)
    message(FATAL_ERROR "Function needs a TARGET parameter")
  endif()
  if(NOT PARAM_FILE)
    message(FATAL_ERROR "Function needs a FILE parameter")
  endif()

  file(WRITE "${PARAM_FILE}" "")

  target_compile_definitions(${PARAM_TARGET} PRIVATE HAVE_CONFIG_H=1)
  target_include_directories(${PARAM_TARGET}
    BEFORE
    PUBLIC
      $<BUILD_INTERFACE:${PROJECT_BINARY_DIR}>
  )

  # only write the current value of each variable once
  list(REMOVE_DUPLICATES PARAM_VARIABLES)

  if(NOT PARAM_VARIABLES)
    file(WRITE "${PARAM_FILE}" "// No config variables defined")
  endif()

  # process each variable
  foreach(_var IN LISTS PARAM_VARIABLES)
    # check for empty variable; variables that are explicitly set to false
    # is not included in this clause
    if((NOT DEFINED ${_var}) OR ("${${_var}}" STREQUAL "") OR NOT _var)
      file (APPEND "${PARAM_FILE}" "/* #undef ${_var} */\n")
    else()
      # write to file using the correct syntax
      if("${_var}" MATCHES "^HAVE_.*")
        file (APPEND "${PARAM_FILE}" "#define ${_var} 1\n")
      else()
        file (APPEND "${PARAM_FILE}" "#define ${_var} ${${_var}}\n")
      endif()
    endif()
  endforeach()
endfunction()
