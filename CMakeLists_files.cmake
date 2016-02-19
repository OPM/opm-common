# This file sets up five lists:
#	MAIN_SOURCE_FILES     List of compilation units which will be included in
#	                      the library. If it isn't on this list, it won't be
#	                      part of the library. Please try to keep it sorted to
#	                      maintain sanity.
#
#	TEST_SOURCE_FILES     List of programs that will be run as unit tests.
#
#	TEST_DATA_FILES       Files from the source three that should be made
#	                      available in the corresponding location in the build
#	                      tree in order to run tests there.
#
#	EXAMPLE_SOURCE_FILES  Other programs that will be compiled as part of the
#	                      build, but which is not part of the library nor is
#	                      run as tests.
#
#	PUBLIC_HEADER_FILES   List of public header files that should be
#	                      distributed together with the library. The source
#	                      files can of course include other files than these;
#	                      you should only add to this list if the *user* of
#	                      the library needs it.

list (APPEND MAIN_SOURCE_FILES
      opm/common/data/SimulationDataContainer.cpp
)

list (APPEND TEST_SOURCE_FILES
      tests/test_SimulationDataContainer.cpp
      tests/test_cmp.cpp
      )

list (APPEND TEST_DATA_FILES
	)

list (APPEND EXAMPLE_SOURCE_FILES
	)

# programs listed here will not only be compiled, but also marked for
# installation
list (APPEND PROGRAM_SOURCE_FILES
	)


list( APPEND PUBLIC_HEADER_FILES
      opm/common/data/SimulationDataContainer.hpp
      opm/common/util/numeric/cmp.hpp
      opm/common/ErrorMacros.hpp
      opm/common/Exceptions.hpp
      opm/common/utility/platform_dependent/disable_warnings.h
      opm/common/utility/platform_dependent/reenable_warnings.h )
