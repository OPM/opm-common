# - Turn on warnings when compiling

include (UseCompVer)
is_compiler_gcc_compatible ()

option(OPM_ENABLE_WARNINGS "Enable warning flags?" ON)
option(SILENCE_EXTERNAL_WARNINGS "Disable some warnings from external packages" OFF)

if(CXX_COMPAT_GCC AND OPM_ENABLE_WARNINGS)
  set(_warn_flag "-Wall -Wextra -Wshadow")
  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13)
    string(APPEND _warn_flag " -Wno-dangling-reference")
  endif()
  message(STATUS "All warnings enabled: ${_warn_flag}")
endif()

function(use_warnings)
  cmake_parse_arguments(PARAM "" "TARGET" "" ${ARGN})
  if(NOT PARAM_TARGET)
    message(FATAL_ERROR "Function needs a TARGET parameter")
  endif()

  if (CXX_COMPAT_GCC AND OPM_ENABLE_WARNINGS)
    # default warnings flags, if not set by user
    target_compile_options(${PARAM_TARGET}
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wshadow>
        $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra -Wshadow>
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13)
      target_compile_options(${PARAM_TARGET}
        PRIVATE
          $<$<COMPILE_LANGUAGE:CXX>:-Wno-dangling-reference>
      )
    endif()
  endif ()

  if(SILENCE_EXTERNAL_WARNINGS AND CXX_COMPAT_GCC)
    target_compile_definitions(${PARAM_TARGET} PRIVATE SILENCE_EXTERNAL_WARNINGS)
  endif()
endfunction()
