# - Try to build faster depending on the compiler

# faster builds by using a pipe instead of temp files
include (UseCompVer)
is_compiler_gcc_compatible ()

option(USE_FAST_BUILD "Enable flags for faster builds?" ON)

function(use_fast_build)
  cmake_parse_arguments(PARAM "" "TARGET" "" ${ARGN})
  if(NOT PARAM_TARGET)
    message(FATAL_ERROR "Function needs a TARGET parameter")
  endif()
  if (CXX_COMPAT_GCC AND USE_FAST_BUILD)
    target_compile_options(${PARAM_TARGET} PRIVATE -pipe)
  endif ()
endfunction()
