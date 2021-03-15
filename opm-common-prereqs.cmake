# -*- mode: cmake; tab-width: 2; indent-tabs-mode: t; truncate-lines: t; compile-command: "cmake -Wdev" -*-
# vim: set filetype=cmake autoindent tabstop=2 shiftwidth=2 noexpandtab softtabstop=2 nowrap:

# defines that must be present in config.h for our headers
set (opm-common_CONFIG_VAR
	HAVE_OPENMP
	)

# dependencies
set (opm-common_DEPS
	# compile with C99 support if available
	"C99"
)

list(APPEND opm-common_DEPS
      # various runtime library enhancements
      "Boost 1.44.0 COMPONENTS system unit_test_framework REQUIRED"
      "OpenMP QUIET"
)
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12.0" AND OPM_ENABLE_PYTHON)
  # Imported target only used for newer CMake and python support
  if(OPM_ENABLE_EMBEDDED_PYTHON)
    list(APPEND opm-common_DEPS
      # Needed for the imported target Python3::Python
      "Python3 COMPONENTS Interpreter Development REQUIRED"
      )
  else()
    # Just to be safe. I actually think we only need the libs/Development stuff
    list(APPEND opm-common_DEPS
      # Needed for the imported target Python3::Python
      "Python3 COMPONENTS Interpreter REQUIRED"
      )
  endif()
endif()
find_package_deps(opm-common)
