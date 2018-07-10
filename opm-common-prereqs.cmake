# -*- mode: cmake; tab-width: 2; indent-tabs-mode: t; truncate-lines: t; compile-command: "cmake -Wdev" -*-
# vim: set filetype=cmake autoindent tabstop=2 shiftwidth=2 noexpandtab softtabstop=2 nowrap:

# defines that must be present in config.h for our headers
set (opm-common_CONFIG_VAR
	"HAS_ATTRIBUTE_UNUSED")

# dependencies
set (opm-common_DEPS
	# compile with C99 support if available
	"C99"
	# compile with C++0x/11 support if available
	"CXX11Features REQUIRED"
	)

if(ENABLE_ECL_INPUT)
  list(APPEND opm-common_DEPS
        "ecl REQUIRED"
        # various runtime library enhancements
        "Boost 1.44.0
          COMPONENTS system filesystem unit_test_framework regex REQUIRED")
else()
  list(APPEND opm-common_DEPS
        # various runtime library enhancements
        "Boost 1.44.0
          COMPONENTS system unit_test_framework REQUIRED")
endif()
# We need a defined target for libecl when linking.
# The definition of the target is done implicitly below when
# libecl is searched for.
find_package_deps(opm-common)
