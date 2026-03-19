if(TARGET Boost::unit_test_framework)
  return()
endif()

find_package (Boost COMPONENTS unit_test_framework QUIET)

if(TARGET Boost::unit_test_framework)
  # setup to do a test compile
  include(CMakePushCheckState)
  cmake_push_check_state()
  include(CheckCXXSourceCompiles)
  list(APPEND CMAKE_REQUIRED_INCLUDES ${Boost_INCLUDE_DIRS})
  list(APPEND CMAKE_REQUIRED_LIBRARIES ${Boost_LIBRARIES})

  unset(HAVE_DYNAMIC_BOOST_TEST CACHE)
  check_cxx_source_compiles("
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE DYNLINK_TEST
#include <boost/test/unit_test.hpp>

int f(int x) { return 2 * x; }

BOOST_AUTO_TEST_CASE(DynlinkConfigureTest) {
  BOOST_CHECK_MESSAGE(f(2) == 4,
                      \"Apparently, multiplication doesn't \"
                      \"work: f(2) = \" << f(2));
}" HAVE_DYNAMIC_BOOST_TEST)
  cmake_pop_check_state ()
else()
  # no Boost, no compile
  set (HAVE_DYNAMIC_BOOST_TEST 0)
endif()

if(HAVE_DYNAMIC_BOOST_TEST)
  target_compile_definitions(Boost::unit_test_framework INTERFACE BOOST_TEST_DYN_LINK=1)
endif()
