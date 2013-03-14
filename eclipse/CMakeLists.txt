
add_subdirectory (externals)
add_subdirectory (src)
file(COPY testdata DESTINATION ${CMAKE_BINARY_DIR})
