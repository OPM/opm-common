cmake_minimum_required (VERSION 2.6)

project (EclipseParser CXX)
enable_testing()
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

add_subdirectory (externals)
add_subdirectory (src)

