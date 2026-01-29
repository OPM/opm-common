# - Use only needed imports from libraries
#
# Add the -Wl,--as-needed flag to the linker flags on Linux
# in order to get only the minimal set of dependencies.

option(ONLY_NEEDED_LIBRARIES "Instruct the linker to not use libraries which are unused?" OFF)

function(use_only_needed)
  cmake_parse_arguments(PARAM "" "TARGET" "" ${ARGN})
  if(NOT PARAM_TARGET)
    message(FATAL_ERROR "Function needs a TARGET parameter")
  endif()

  # only ELF shared objects can be underlinked, and only GNU will accept
  # these parameters; otherwise just leave it to the defaults
  if ((CMAKE_CXX_PLATFORM_ID STREQUAL "Linux") AND CMAKE_COMPILER_IS_GNUCC AND ONLY_NEEDED_LIBRARIES)
    target_link_options(${PARAM_TARGET} BEFORE PRIVATE LINKER:--as-needed)
  endif()
endfunction()
