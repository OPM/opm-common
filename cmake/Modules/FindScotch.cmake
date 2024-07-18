# Module that checks whether Scotch is available.
#
# Accepts the following variables:
#
# Sets the following variables:
# SCOTCH_INCLUDE_DIRS: All include directories needed to compile Scotch programs.
# SCOTCH_LIBRARIES:    Alle libraries needed to link Scotch programs.
# SCOTCH_FOUND:        True if Scotch was found.
#
# Provides the following macros:
#
# find_package(Scotch)

# Search for Scotch includes
find_path(SCOTCH_INCLUDE_DIR scotch.h
    PATHS /usr/include /usr/local/include
    PATH_SUFFIXES scotch
)

if (SCOTCH_INCLUDE_DIR)
    set(SCOTCH_INCLUDE_DIRS ${SCOTCH_INCLUDE_DIR})
endif()

# Search for Scotch libraries
find_library(SCOTCH_LIBRARY NAMES scotch scotch-int32 scotch-int64 scotch-long
    PATHS
        /usr/lib
        /usr/local/lib
)

if (SCOTCH_LIBRARY)
    set(SCOTCH_LIBRARIES ${SCOTCH_LIBRARY})
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(
  "Scotch"
  DEFAULT_MSG
  SCOTCH_INCLUDE_DIRS
  SCOTCH_LIBRARIES
)

mark_as_advanced(SCOTCH_INCLUDE_DIRS SCOTCH_LIBRARIES)
