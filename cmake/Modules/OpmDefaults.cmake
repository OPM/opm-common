# - Default settings for the build

# build release by default
if (NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set (CMAKE_BUILD_TYPE "Release")
endif()

# default to building a static library, but let user override
option(BUILD_SHARED_LIBS "Build shared libraries?" OFF)
if (BUILD_SHARED_LIBS)
  set(${project}_LIBRARY_TYPE SHARED)
else()
  set(${project}_LIBRARY_TYPE STATIC)
endif()
