# - Default settings for the build

macro(opm_defaults opm)
  # build release by default
  if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    set (CMAKE_BUILD_TYPE "Release")
  endif()

  # default to building a static library, but let user override
  if (DEFINED BUILD_SHARED_LIBS)
    if (BUILD_SHARED_LIBS)
      set (${opm}_LIBRARY_TYPE SHARED)
    else()
      set (${opm}_LIBRARY_TYPE STATIC)
    endif()
  else()
    set(${opm}_LIBRARY_TYPE STATIC)
  endif()
endmacro (opm_defaults opm)

# overwrite a cache entry's value, but keep docstring and type
# if not already in cache, then does nothing
function (update_cache name)
  get_property (_help CACHE "${name}" PROPERTY HELPSTRING)
  get_property (_type CACHE "${name}" PROPERTY TYPE)
  if (NOT "${_type}" STREQUAL "")
	#message ("Setting ${name} to \"${${name}}\" in cache.")
	set ("${name}" "${${name}}" CACHE ${_type} "${_help}" FORCE)
  endif ()
endfunction (update_cache name)

# put all compiler options currently set back into the cache, so that
# they can be queried from there (using ccmake for instance)
function (write_back_options)
  # build type
  update_cache (CMAKE_BUILD_TYPE)

  # compilers
  set (languages C CXX Fortran)
  foreach (language IN LISTS _languages)
	if (CMAKE_${language}_COMPILER)
	  update_cache (CMAKE_${language}_COMPILER)
	endif ()
  endforeach (language)

  # flags (notice use of IN LISTS to get the empty variant)
  set (buildtypes "" "_DEBUG" "_RELEASE" "_MINSIZEREL" "_RELWITHDEBINFO")
  set (processors "C" "CXX" "Fortran" "EXE_LINKER" "MODULE_LINKER" "SHARED_LINKER")
  foreach (processor IN LISTS processors)
	foreach (buildtype IN LISTS buildtypes)
	  if (CMAKE_${processor}_FLAGS${buildtype})
		update_cache (CMAKE_${processor}_FLAGS${buildtype})
	  endif ()
	endforeach (buildtype)
  endforeach (processor)
endfunction (write_back_options)
