add_executable(runTimeMapTests TimeMapTest.cpp)
target_link_libraries(runTimeMapTests Parser ${Boost_LIBRARIES})
add_test(NAME runTimeMapTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runTimeMapTests )


add_executable(runScheduleTests ScheduleTests.cpp)
target_link_libraries(runScheduleTests Parser ${Boost_LIBRARIES})
add_test(NAME runScheduleTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runScheduleTests )


add_executable(runWellTests WellTests.cpp)
target_link_libraries(runWellTests Parser ${Boost_LIBRARIES})
add_test(NAME runWellTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runWellTests )


add_executable(runDynamicStateTests DynamicStateTests.cpp)
target_link_libraries(runDynamicStateTests Parser ${Boost_LIBRARIES})
add_test(NAME runDynamicStateTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runDynamicStateTests )
