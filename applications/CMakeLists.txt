project(opm-parser-applications CXX)

add_executable(opmi opmi.cpp)
target_link_libraries(opmi opmparser)
install(TARGETS opmi DESTINATION ${CMAKE_INSTALL_BINDIR})

set(app_src ${PROJECT_SOURCE_DIR}/opmi.cpp)
get_property(app_includes_all TARGET opmparser PROPERTY INTERFACE_INCLUDE_DIRECTORIES)
foreach(prop ${app_includes_all})
  string(REGEX REPLACE "^.*BUILD_INTERFACE:([^>]*)>.*" "\\1" incl "${prop}")
  if(NOT incl MATCHES "<")
    list(APPEND app_includes ${incl})
  endif()
endforeach()

add_static_analysis_tests(app_src app_includes)
