option(USE_VALGRIND "Enable valgrind support?" OFF)

if(USE_VALGRIND)
  find_package(Valgrind)
endif()

function(use_valgrind)
  cmake_parse_arguments(PARAM "" "TARGET" "" ${ARGN})
  if(NOT PARAM_TARGET)
    message(FATAL_ERROR "Function needs a TARGET parameter")
  endif()

  if(TARGET Valgrind::Valgrind)
    target_link_libraries(${PARAM_TARGET} PRIVATE Valgrind::Valgrind)
    target_compile_definitions(${PARAM_TARGET} PRIVATE HAVE_VALGRIND=1)
  endif()
endfunction()
