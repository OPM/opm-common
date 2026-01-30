# - Use thread features
#
# Synopsis:
#
#	use_threads (target)
#
# where:
#
#	target            Name of the target to which Threads support
#	                  etc. should be added
#
# Example:
#	use_threads(mytarget)

option(USE_THREADS "Use threading?" ON)
if(USE_THREADS)
  set (CMAKE_THREAD_PREFER_PTHREAD TRUE)
  find_package(Threads)

  if(CMAKE_USE_PTHREADS_INIT)
    set(HAVE_PTHREAD 1)
  endif()
endif()

function(use_threads)
  cmake_parse_arguments(PARAM "" "TARGET" "" ${ARGN})
  if(NOT PARAM_TARGET)
    message(FATAL_ERROR "Function needs a TARGET parameter")
  endif()

  # if USE_PTHREAD is enabled then check and set HAVE_PTHREAD
  if(USE_THREADS AND TARGET Threads::Threads)
    target_link_libraries(${PARAM_TARGET} PRIVATE Threads::Threads)
  endif()
endfunction()
