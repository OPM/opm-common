# -*- mode: cmake; tab-width: 2; indent-tabs-mode: t; truncate-lines: t; compile-command: "cmake -Wdev" -*-
# vim: set filetype=cmake autoindent tabstop=2 shiftwidth=2 noexpandtab softtabstop=2 nowrap:

# defines that must be present in config.h for our headers
set (opm-common_CONFIG_VAR
	HAVE_OPENMP
	HAVE_TYPE_TRAITS
	HAVE_VALGRIND
	HAVE_FINAL
	HAVE_ECL_INPUT
	HAVE_CXA_DEMANGLE
	HAVE_FNMATCH_H
	)

# dependencies
set (opm-common_DEPS
	# compile with C99 support if available
	"C99"
	# valgrind client requests
	"Valgrind"
)

list(APPEND opm-common_DEPS
      # various runtime library enhancements
      "Boost 1.44.0 COMPONENTS system unit_test_framework REQUIRED"
      "cjson"
      # Still it produces compile errors complaining that it
      # cannot format UDQVarType. Hence we use the same version
      # as the embedded one.
      "fmt 8.0"
      "QuadMath"
)
find_package_deps(opm-common)
