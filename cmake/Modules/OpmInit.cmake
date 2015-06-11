# - Initialize project-specific variables
#
# This will read the dune.module file for project information and
# set the following variables:
#
#	project                     From the Module: field
#	${project}_NAME             Same as above
#	${project}_DESCRIPTION      From the Description: field
#	${project}_VERSION_MAJOR    From the Version: field
#	${project}_VERSION_MINOR    From the Version: field also
#
# This module should be the first to be included in the project,
# because most of the others (OpmXxx.cmake) use these variables.


# add ${basePath}/cmake/Modules to the cmake search path for modules
# if it exists
function(AddToCMakeSearchPath basePath)
  set(CMakeModuleDir "${basePath}/cmake/Modules")
  if(EXISTS "${CMakeModuleDir}" AND IS_DIRECTORY "${CMakeModuleDir}")
    list(INSERT CMAKE_MODULE_PATH 0 "${CMakeModuleDir}")
  endif()
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)

  # TODO (?): warn if a file from the new path is already contained in
  # the existing search path. (this could lead to hard to find build
  # system issues)
endfunction()

# add all relevant directories to the search path for cmake
function(UpdateCMakeModulePath)
  # add all paths which are specified in variables named "*_ROOT" to
  # the CMake search path, but only if they contain CMake modules
  get_cmake_property(allVariableNames VARIABLES)
  foreach(variableName ${allVariableNames})
    STRING(REGEX MATCH "_ROOT$" variableNameIsModuleRoot "${variableName}")
    if(variableNameIsModuleRoot)
      AddToCMakeSearchPath("${${variableName}}")
    endif()
  endforeach()

  # finally add the current source directory to the module search path
  AddToCMakeSearchPath("${CMAKE_CURRENT_SOURCE_DIR}")

  # we want to set the global CMAKE_MODULE_PATH variable which we have
  # to do this way. note that this is a bit hacky because the parent
  # scope is not necessarily the global scope. (we need to do it this
  # way anyway because CMake does not seem to expose a GLOBAL_SCOPE
  # for this...)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} PARENT_SCOPE)
endfunction()


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

# add paths which are specified by the user to the search path for
# CMake modules. (This allows to distribute the required build system
# files with the modules that need them instead of centrally.)
UpdateCMakeModulePath()

OpmInitProjVars ()
OpmInitDirVars ()

# if we are backporting this release to a system which already have an
# earlier version, set this flag to have everything scoped into a directory
# which incorporates the label of the release. this is done by interjecting
# the ${project}_VER_DIR into the installation path.
option (USE_VERSIONED_DIR "Put files in release-specific directories" OFF)
set (${project}_SUITE "opm")
if (USE_VERSIONED_DIR)
  set (${project}_VER_DIR "/${${project}_SUITE}-${${project}_LABEL}")
else ()
  set (${project}_VER_DIR "")
endif ()
