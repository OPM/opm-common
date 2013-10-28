add_executable(runTimeMapTests TimeMapTest.cpp)
target_link_libraries(runTimeMapTests Parser ${Boost_LIBRARIES})
add_test(NAME runTimeMapTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runTimeMapTests )


add_executable(runScheduleTests ScheduleTests.cpp)
target_link_libraries(runScheduleTests Parser ${Boost_LIBRARIES})
add_test(NAME runScheduleTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runScheduleTests )
