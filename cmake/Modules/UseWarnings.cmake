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

macro(use_warnings target)
  if (CXX_COMPAT_GCC AND OPM_ENABLE_WARNINGS)
    # default warnings flags, if not set by user
    target_compile_options(${target}
      PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Wshadow>
        $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra -Wshadow>
    )
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 13)
      target_compile_options(${target}
        PRIVATE
          $<$<COMPILE_LANGUAGE:CXX>:-Wno-dangling-reference>
      )
    endif()
  endif ()

  if(SILENCE_EXTERNAL_WARNINGS AND CXX_COMPAT_GCC)
    target_compile_definitions(${${opm_}_TARGET} PRIVATE SILENCE_EXTERNAL_WARNINGS)
  endif()
endmacro()
