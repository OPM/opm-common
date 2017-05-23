project(opm-parser-applications CXX)

add_executable(opmi opmi.cpp)
target_link_libraries(opmi opmparser)
install(TARGETS opmi DESTINATION ${CMAKE_INSTALL_BINDIR})
