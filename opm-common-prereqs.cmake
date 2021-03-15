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
      "cjson"
      # Still it produces compile errors complaining that it
      # cannot format UDQVarType. Hence we use the same version
      # as the embedded one.
      "fmt 7.0.3"
)
if(${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.12.0")
  list(APPEND opm-common_DEPS
    # Needed for the imported target Python3::Python
    "Python3 COMPONENTS Interpreter Development"
    )
endif()
find_package_deps(opm-common)
