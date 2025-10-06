# Module that checks whether the compiler supports the
# quadruple precision floating point math
#
# Adds target QuadMath::QuadMath if found
#
# perform tests
include(CheckCXXSourceCompiles)
include(CMakePushCheckState)
cmake_push_check_state()
set(CMAKE_REQUIRED_LIBRARIES quadmath)
if(${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
  set(CMAKE_REQUIRED_FLAGS "-fext-numeric-literals")
endif()
check_cxx_source_compiles("
#include <quadmath.h>

int main ()
{
  __float128 r = 1.0q;
  r = strtoflt128(\"1.2345678\", NULL);
  return 0;
}" QuadMath_COMPILES)
cmake_pop_check_state()  # Reset CMAKE_REQUIRED_XXX variables

if(QuadMath_COMPILES)
  # Use additional variable for better report message
  set(QuadMath_VAR "(Supported by compiler)")
endif()

# Report that package was found
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(QuadMath
  DEFAULT_MSG
  QuadMath_VAR QuadMath_COMPILES
)

# add imported target for quadmath
if(QuadMath_FOUND AND NOT TARGET QuadMath::QuadMath)
  # Compiler supports QuadMath: Add appropriate linker flag
  add_library(QuadMath::QuadMath INTERFACE IMPORTED)
  target_link_libraries(QuadMath::QuadMath INTERFACE quadmath)

  # Somehow needed for DUNE 2.9/g++-12
  target_compile_options(QuadMath::QuadMath INTERFACE
    $<$<CXX_COMPILER_ID:GNU>:-fext-numeric-literals>
  )

  # Check for numeric_limits specialization
  cmake_push_check_state()
  set(CMAKE_REQUIRED_LIBRARIES quadmath)
  set(CMAKE_REQUIRED_FLAGS)
  if(${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
    set(CMAKE_REQUIRED_FLAGS "-fext-numeric-literals")
  endif()

  check_cxx_source_compiles("
  #include <limits>
  int main()
  {
     static_assert(std::numeric_limits<__float128>::is_specialized);
     return 0;
  }" QuadMath_HAS_LIMITS)

  if(QuadMath_HAS_LIMITS)
    target_compile_definitions(QuadMath::QuadMath INTERFACE LIMITS_HAS_QUAD=1)
  endif()
  check_cxx_source_compiles("
  #include <limits>
  #include <iostream>
  int main()
  {
     __float128 b=0;
     std::cout<<b;
     return 0;
  }" QuadMath_HAS_IO_OPERATOR)

  if(QuadMath_HAS_IO_OPERATOR)
    target_compile_definitions(QuadMath::QuadMath INTERFACE QUADMATH_HAS_IO_OPERATOR=1)
  endif()
  check_cxx_source_compiles("
  #include <cmath>
  int main()
  {
     __float128 b=10;
     __float128 c=std::atan(b);
  }" QuadMath_HAS_MATH_OPS)
  cmake_pop_check_state()  # Reset CMAKE_REQUIRED_XXX variables
  if(QuadMath_HAS_MATH_OPS)
    target_compile_definitions(QuadMath::QuadMath INTERFACE QUADMATH_HAS_MATH_OPERATORS=1)
  endif()
  cmake_pop_check_state()  # Reset CMAKE_REQUIRED_XXX variables
endif()
