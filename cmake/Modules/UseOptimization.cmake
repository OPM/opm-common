# - Turn on optimizations based on build type

include(TestCXXAcceptsFlag)

# native instruction set tuning
option (WITH_NATIVE "Use native instruction set" ON)
if (WITH_NATIVE)
  check_cxx_accepts_flag ("-mtune=native" HAVE_MTUNE)
endif()

option(WITH_NDEBUG "Disable asserts in release mode" OFF)

function(use_additional_optimization)
  cmake_parse_arguments(PARAM "" "TARGET" "" ${ARGN})
  if(NOT PARAM_TARGET)
    message(FATAL_ERROR "Function needs a TARGET parameter")
  endif()

  if (WITH_NATIVE AND HAVE_MTUNE)
    target_compile_options(${PARAM_TARGET} PRIVATE $<$<CONFIG:Release,RelWithDebInfo>:-mtune=native>)
  endif()

  target_compile_definitions(${PARAM_TARGET} PRIVATE $<$<CONFIG:Debug>:DEBUG>)

  if(NOT WITH_NDEBUG)
    target_compile_options(${PARAM_TARGET} PRIVATE $<$<CONFIG:Release,RelWithDebInfo>:-UNDEBUG>)
  endif()
endfunction()
