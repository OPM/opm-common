# - Initialize project-specific variables
#
# This will read the dune.module file for project information and
# set the following variables:
#
#	project                     From the Module: field
#	${project}_NAME             Same as above
#	${project}_DESCRIPTION      From the Description: field
#	${project}_VERSION          From the Version: field
#	${project}_VERSION_MAJOR    From the Version: field
#	${project}_VERSION_MINOR    From the Version: field also
#
# This module should be the first to be included in the project,
# because most of the others (OpmXxx.cmake) use these variables.
#
# In addition it will try to set ${module}_DIR for each module
# that is in Depends or Suggests if sibling builds are enabled.
# Note that for sibling build to succeed all dependencies need
# to be listed. Note also that both dune-common and opm-common
# have to be handled in the modules CMakeLists.txt directly.

include(OpmSiblingSearch)

# helper macro to retrieve a single field of a dune.module file
macro(OpmGetDuneModuleDirective field variable contents)
  string (REGEX MATCH ".*${field}:[ ]*([^\n]+).*" ${variable} "${contents}")
  string (REGEX REPLACE ".*${field}:[ ]*([^\n]+).*" "\\1" "${variable}" "${${variable}}")
  string (STRIP "${${variable}}" ${variable})
endmacro()

function (OpmInitProjVars)
  # locate the "dune.module" file
  set (DUNE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/dune.module")

  # read this file into a variable
  file (READ "${DUNE_MODULE_PATH}" DUNE_MODULE)

  # read fields from the file
  OpmGetDuneModuleDirective ("Module" project "${DUNE_MODULE}")
  OpmGetDuneModuleDirective ("Description" description "${DUNE_MODULE}")
  OpmGetDuneModuleDirective ("Version" version "${DUNE_MODULE}")
  OpmGetDuneModuleDirective ("Label" label "${DUNE_MODULE}")
  OpmGetDuneModuleDirective ("Depends" depends "${DUNE_MODULE}")
  OpmGetDuneModuleDirective ("Suggests" suggests "${DUNE_MODULE}")

  # Create a list of modules that we depend on.
  set(depends "${depends} ${suggests}")
  set(stripver_regex "([a-zA-Z-]+[ \t]+)\\\([^)]*\\\)")
  string (REGEX MATCH "${stripver_regex}" _match "${depends}")
  string (REGEX REPLACE "${stripver_regex}" "\\1" depends "${depends}")
  string (REGEX REPLACE "[\t ]+" ";" depends "${depends}")
  string (REGEX REPLACE "^(.*);$" "\\1" depends "${depends}")

  # Set the *_DIR variables needed for sibling builds to work
  foreach(_dune_module ${depends})
    create_module_dir_var(${_dune_module})
    set(${_dune_module}_DIR "${${_dune_module}_DIR}" PARENT_SCOPE)
  endforeach()

  # parse the version number
  set (verno_regex "^([0-9]*)\\.([0-9]*).*\$")
  string (REGEX REPLACE "${verno_regex}" "\\1" major "${version}")
  string (REGEX REPLACE "${verno_regex}" "\\2" minor "${version}")

  # return these variables
  set (project "${project}" PARENT_SCOPE)
  set (${project}_NAME "${project}" PARENT_SCOPE)
  set (${project}_DESCRIPTION "${description}" PARENT_SCOPE)
  set (${project}_VERSION_MAJOR "${major}" PARENT_SCOPE)
  set (${project}_VERSION_MINOR "${minor}" PARENT_SCOPE)
  set (${project}_VERSION "${major}.${minor}" PARENT_SCOPE)
  set (${project}_LABEL "${label}" PARENT_SCOPE)
endfunction ()

macro (OpmInitDirVars)
  # these are the most common (and desired locations)
  set (${project}_DIR "opm")
  set (doxy_dir "doc/doxygen")

  # but for backward compatibility we can override it
  if (COMMAND dir_hook)
	dir_hook ()
  endif (COMMAND dir_hook)
endmacro ()

if("${CMAKE_SIZEOF_VOID_P}" LESS 8)
  message(FATAL_ERROR "OPM will only work correctly on 64bit (or higher) systems!")
endif()

OpmInitProjVars ()
OpmInitDirVars ()

# For prereqs-file
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR})

set (${project}_SUITE "opm")

# We do not want language extensions enabled
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_EXTENSIONS OFF)
