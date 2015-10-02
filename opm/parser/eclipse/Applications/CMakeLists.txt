add_executable(opm-eclkwtest opm-eclkwtest.cpp)
target_link_libraries(opm-eclkwtest opmparser)
install(TARGETS opm-eclkwtest DESTINATION "bin")

add_executable(opm-eclkw opm-eclkw.cpp)
target_link_libraries(opm-eclkw opmparser)
install(TARGETS opm-eclkw DESTINATION "bin")

add_executable(schedule Schedule.cpp)
target_link_libraries(schedule opmparser)

add_executable(OpmLoadDeck OpmLoadDeck)
target_link_libraries(OpmLoadDeck opmparser)
install(TARGETS OpmLoadDeck DESTINATION "bin")

