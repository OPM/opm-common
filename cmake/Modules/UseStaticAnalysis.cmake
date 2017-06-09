# Add static analysis tests for a given source file

macro(setup_static_analysis_tools)
  find_package(CppCheck)
  if(CMAKE_EXPORT_COMPILE_COMMANDS)
    find_package(ClangCheck)
  else()
    message(STATUS "Disabling clang-check as CMAKE_EXPORT_COMPILE_COMMANDS is not enabled")
  endif()
  if(OPM_COMMON_ROOT)
    set(DIR ${OPM_COMMON_ROOT})
  elseif(OPM_MACROS_ROOT)
    set(DIR ${OPM_MACROS_ROOT})
  else()
    set(DIR ${CMAKE_CURRENT_SOURCE_DIR})
  endif()
  if(CPPCHECK_FOUND)
    file(COPY        ${DIR}/cmake/Scripts/cppcheck-test.sh
         DESTINATION bin
         FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
    endif()
  if(CLANGCHECK_FOUND AND CMAKE_EXPORT_COMPILE_COMMANDS)
    configure_file(${DIR}/cmake/Scripts/clang-check-test.sh.in
                   ${CMAKE_BINARY_DIR}/CMakeFiles/clang-check-test.sh)
    file(COPY ${CMAKE_BINARY_DIR}/CMakeFiles/clang-check-test.sh
         DESTINATION bin
         FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
   endif()
   set(CPPCHECK_SCRIPT ${PROJECT_BINARY_DIR}/bin/cppcheck-test.sh)
   set(CLANGCHECK_SCRIPT ${PROJECT_BINARY_DIR}/bin/clang-check-test.sh)
endmacro()

function(add_static_analysis_tests sources includes)
  if(CPPCHECK_FOUND OR (CLANGCHECK_FOUND AND CMAKE_EXPORT_COMPILE_COMMANDS))
    foreach(dep ${${includes}})
      list(APPEND IPATHS -I ${dep})
    endforeach()
    foreach(src ${${sources}})
      file(RELATIVE_PATH name ${PROJECT_SOURCE_DIR} ${src})
      if(CPPCHECK_FOUND AND UNIX)
        add_test(NAME cppcheck+${name}
                 COMMAND ${CPPCHECK_SCRIPT} ${CPPCHECK_PROGRAM} ${src} ${IPATHS}
                 CONFIGURATIONS analyze cppcheck)
      endif()
      if(CLANGCHECK_FOUND AND CMAKE_EXPORT_COMPILE_COMMANDS AND UNIX)
        add_test(NAME clang-check+${name}
                 COMMAND ${CLANGCHECK_SCRIPT} ${CLANGCHECK_PROGRAM} ${src}
                 CONFIGURATIONS analyze clang-check)
      endif()
    endforeach()
  endif()
endfunction()
