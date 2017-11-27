# - Turn on warnings when compiling

# sets CXX_COMPAT_GCC if we have either GCC or Clang
macro (is_compiler_gcc_compatible)
  # is the C++ compiler clang++?
  string (TOUPPER "${CMAKE_CXX_COMPILER_ID}" _comp_id)
  if (_comp_id MATCHES "CLANG")
    set (CMAKE_COMPILER_IS_CLANGXX TRUE)
  else ()
    set (CMAKE_COMPILER_IS_CLANGXX FALSE)
  endif ()
  # is the C++ compiler g++ or clang++?
  if (CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
    set (CXX_COMPAT_GCC TRUE)
  else ()
    set (CXX_COMPAT_GCC FALSE)
  endif ()
  # is the C compiler clang?
  string (TOUPPER "${CMAKE_C_COMPILER_ID}" _comp_id)
  if (_comp_id MATCHES "CLANG")
    set (CMAKE_COMPILER_IS_CLANG TRUE)
  else ()
    set (CMAKE_COMPILER_IS_CLANG FALSE)
  endif ()
  # is the C compiler gcc or clang?
  if (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_CLANG)
    set (C_COMPAT_GCC TRUE)
  else ()
    set (C_COMPAT_GCC FALSE)
  endif ()
endmacro (is_compiler_gcc_compatible)

is_compiler_gcc_compatible ()

option(SILENCE_EXTERNAL_WARNINGS "Disable some warnings from external packages (requires GCC 4.6 or newer)" OFF)
if(SILENCE_EXTERNAL_WARNINGS AND CXX_COMPAT_GCC)
  dune_register_package_flags(COMPILE_DEFINITIONS "SILENCE_EXTERNAL_WARNINGS=1")
endif()
