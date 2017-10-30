# -*- mode: cmake; tab-width: 2; indent-tabs-mode: t; truncate-lines: t; compile-command: "cmake -Wdev" -*-
# vim: set filetype=cmake autoindent tabstop=2 shiftwidth=2 noexpandtab softtabstop=2 nowrap:

# defines that must be present in config.h for our headers
set (opm-simulators_CONFIG_VAR
	HAVE_OPM_GRID
	HAVE_PTHREAD
	HAVE_EWOMS
        DUNE_ISTL_VERSION_MAJOR
        DUNE_ISTL_VERSION_MINOR
        DUNE_ISTL_VERSION_REVISION
	HAVE_SUITESPARSE_UMFPACK
	)

# dependencies
set (opm-simulators_DEPS
	# Compile with C99 support if available
	"C99"
	# Compile with C++0x/11 support if available
	"CXX11Features"
	# Various runtime library enhancements
	"Boost 1.44.0
		COMPONENTS date_time filesystem system unit_test_framework REQUIRED"
	# DUNE prerequisites
	"dune-common REQUIRED;
	dune-istl REQUIRED"
	"ERTPython"
	# Tim Davis' SuiteSparse archive
	"SuiteSparse COMPONENTS umfpack"
  # OPM dependency
	"opm-common REQUIRED;
   opm-parser REQUIRED;
	 opm-grid REQUIRED;
	 opm-core REQUIRED;
	 opm-output REQUIRED;
	 ewoms REQUIRED"
	# Eigen
	"Eigen3 3.2.0"
	)
