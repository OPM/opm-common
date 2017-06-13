# Find the OPM Eclipse input parser.
#
# Set the cache variable OPM_PARSER_ROOT to the install location of the
# library, or OPM_ROOT to the parent directory of the build tree.
#
# If found, it sets these variables:
#
#       HAVE_OPM_PARSER              Defined if a test program compiled
#       OPM_PARSER_INCLUDE_DIRS      Header file directories
#       OPM_PARSER_LIBRARIES         Archives and shared objects

include (FindPackageHandleStandardArgs)

# variables to pass on to other packages
if (FIND_QUIETLY)
  set (OPM_PARSER_QUIET "QUIET")
else ()
  set (OPM_PARSER_QUIET "")
endif ()


# inherit "suite" root if not specifically set for this library
if ((NOT OPM_PARSER_ROOT) AND OPM_ROOT)
  set (OPM_PARSER_ROOT "${OPM_ROOT}/opm-parser")
endif ()

# Detect the build dir suffix or subdirectory
string(REGEX REPLACE "${PROJECT_SOURCE_DIR}/?(.*)" "\\1"  BUILD_DIR_SUFFIX "${PROJECT_BINARY_DIR}")

# if a root is specified, then don't search in system directories
# or in relative directories to this one
if (OPM_PARSER_ROOT)
  set (_no_default_path "NO_DEFAULT_PATH")
  set (_opm_parser_source "")
  set (_opm_parser_build "")
else ()
  set (_no_default_path "")
  set (_opm_parser_source
    "${PROJECT_SOURCE_DIR}/../opm-parser")
  set (_opm_parser_build
    "${PROJECT_BINARY_DIR}/../opm-parser"
    "${PROJECT_BINARY_DIR}/../opm-parser${BUILD_DIR_SUFFIX}"
    "${PROJECT_BINARY_DIR}/../../opm-parser/${BUILD_DIR_SUFFIX}")
endif ()


find_package(opm-parser CONFIG HINTS ${_opm_parser_build})
if (opm-parser_FOUND)
    find_package(ecl REQUIRED)
    set(HAVE_OPM_PARSER 1)
    # setting HAVE_ERT is a mega hack here, but some downstreams require it.
    # Eventually projets should move on to properly handle dependencies and
    # configurations, and Findopm-parser be deprecated
    set(HAVE_ERT 1)
    set(opm-parser_CONFIG_VARS HAVE_OPM_PARSER HAVE_REGEX HAVE_ERT)
    set(opm-parser_LIBRARIES opmparser)

    find_package_handle_standard_args(opm-parser
        DEFAULT_MSG
        opm-parser_LIBRARIES HAVE_OPM_PARSER HAVE_ERT
    )
endif ()

mark_as_advanced(opm-parser_LIBRARIES)
