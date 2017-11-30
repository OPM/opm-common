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
      opm/common/OpmLog/CounterLog.cpp
      opm/common/OpmLog/EclipsePRTLog.cpp
      opm/common/OpmLog/LogBackend.cpp
      opm/common/OpmLog/Logger.cpp
      opm/common/OpmLog/LogUtil.cpp
      opm/common/OpmLog/OpmLog.cpp
      opm/common/OpmLog/StreamLog.cpp
      opm/common/OpmLog/TimerLog.cpp
      opm/core/utility/parameters/Parameter.cpp
      opm/core/utility/parameters/ParameterGroup.cpp
      opm/core/utility/parameters/ParameterTools.cpp
)

list (APPEND TEST_SOURCE_FILES
      tests/test_SimulationDataContainer.cpp
      tests/test_cmp.cpp
      tests/test_ConditionalStorage.cpp
      tests/test_OpmLog.cpp
      tests/test_messagelimiter.cpp
      tests/test_param.cpp
)

list (APPEND TEST_DATA_FILES
      tests/testdata.param
)

list (APPEND EXAMPLE_SOURCE_FILES
)

# programs listed here will not only be compiled, but also marked for
# installation
list (APPEND PROGRAM_SOURCE_FILES
)


list( APPEND PUBLIC_HEADER_FILES
      opm/common/ConditionalStorage.hpp
      opm/common/ErrorMacros.hpp
      opm/common/Exceptions.hpp
      opm/common/ResetLocale.hpp
      opm/common/Unused.hpp
      opm/common/Valgrind.hpp
      opm/common/data/SimulationDataContainer.hpp
      opm/common/OpmLog/CounterLog.hpp
      opm/common/OpmLog/EclipsePRTLog.hpp
      opm/common/OpmLog/LogBackend.hpp
      opm/common/OpmLog/Logger.hpp
      opm/common/OpmLog/LogUtil.hpp
      opm/common/OpmLog/MessageFormatter.hpp
      opm/common/OpmLog/MessageLimiter.hpp
      opm/common/OpmLog/OpmLog.hpp
      opm/common/OpmLog/StreamLog.hpp
      opm/common/OpmLog/TimerLog.hpp
      opm/common/util/numeric/cmp.hpp
      opm/common/utility/platform_dependent/disable_warnings.h
      opm/common/utility/platform_dependent/reenable_warnings.h
      opm/core/utility/parameters/ParameterGroup.hpp
      opm/core/utility/parameters/ParameterGroup_impl.hpp
      opm/core/utility/parameters/Parameter.hpp
      opm/core/utility/parameters/ParameterMapItem.hpp
      opm/core/utility/parameters/ParameterRequirement.hpp
      opm/core/utility/parameters/ParameterStrings.hpp
      opm/core/utility/parameters/ParameterTools.hpp
)
