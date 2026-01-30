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
