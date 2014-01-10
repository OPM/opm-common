add_executable(opm-eclkwtest opm-eclkwtest.cpp)
target_link_libraries(opm-eclkwtest Parser)
install(TARGETS opm-eclkwtest DESTINATION "bin")

add_executable(opm-eclkw opm-eclkw.cpp)
target_link_libraries(opm-eclkw Parser)
install(TARGETS opm-eclkw DESTINATION "bin")

add_executable(schedule Schedule.cpp)
target_link_libraries(schedule Parser)

