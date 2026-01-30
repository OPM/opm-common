# - Use runpath features
#
# Synopsis:
#
# use_runpath(target)
#
# where:
#
# target            Name of the target to which rpath options
#                   should be added
#
# Example:
# use_runpath(mytarget)

# This option is only needed if you use dynamic libraries and install
# to a non-standard prefix.
option(USE_RUNPATH "Embed dependency paths in installed binaries?" OFF)

function(use_runpath)
  cmake_parse_arguments(PARAM "" "TARGET" "" ${ARGN})
  if(NOT PARAM_TARGET)
    message(FATAL_ERROR "Function needs a TARGET parameter")
  endif()
  if(USE_RUNPATH)
    set_property(TARGET ${PARAM_TARGET} PROPERTY INSTALL_RPATH_USE_LINK_PATH ON)
    if(APPLE)
      set(base_path "@loader_path")
     else()
      set(base_path "\${ORIGIN}")
    endif()
    set_property(TARGET ${PARAM_TARGET} PROPERTY INSTALL_RPATH ${base_path}/../${CMAKE_INSTALL_LIBDIR})
  endif()
endfunction()
