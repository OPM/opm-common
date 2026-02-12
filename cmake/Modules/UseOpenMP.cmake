# - Use OpenMP features
#
# Synopsis:
#
# use_openmp (TARGET target)
#
# where:
#
# target            Name of the target to which OpenMP libraries
#                   etc. should be added, e.g. "opmcommon".
#
# Example:
# use_openmp (TARGET opmcommon)

include(UseCompVer)
is_compiler_gcc_compatible ()

# default is that OpenMP is not considered to be there; if we set this
# to a blank definition, it may be added but it cannot be removed if
# it propagates to other projects (someone declares it to be part of
# _CONFIG_VARS)
set(HAVE_OPENMP)

# user code can be conservative by setting USE_OPENMP_DEFAULT
if(NOT DEFINED USE_OPENMP_DEFAULT)
  set(USE_OPENMP_DEFAULT ON)
endif()
option(USE_OPENMP "Enable OpenMP for parallelization" ${USE_OPENMP_DEFAULT})
if(USE_OPENMP)
  # enabling OpenMP is supposedly enough to make the compiler link with
  # the appropriate libraries
  find_package(OpenMP)
  if(OpenMP_FOUND)
    set(HAVE_OPENMP 1)
  endif()
endif()

function(use_openmp)
  cmake_parse_arguments(PARAM "" "TARGET" "" ${ARGN})
  if(NOT PARAM_TARGET)
    message(FATAL_ERROR "Function needs a TARGET parameter")
  endif()

  if(OpenMP_FOUND AND USE_OPENMP)
    if(TARGET OpenMP::OpenMP_CXX)
      target_link_libraries(${PARAM_TARGET} PUBLIC OpenMP::OpenMP_CXX)
    endif()
    if(TARGET OpenMP::OpenMP_C)
      target_link_libraries(${PARAM_TARGET} PUBLIC OpenMP::OpenMP_C)
    endif()
  else()
    include(FindPackageMessage)
    find_package_message(OpenMP "OpenMP: disabled" omp_status)
    # if we don't have OpenMP support, then don't show warnings that these
    # pragmas are unknown
    if(CXX_COMPAT_GCC)
      target_compile_options(${PARAM_TARGET} PUBLIC -Wno-unknown-pragmas)
    elseif(MSVC)
      target_compile_options(${PARAM_TARGET} PUBLIC -wd4068)
    endif()
  endif()
endfunction()
