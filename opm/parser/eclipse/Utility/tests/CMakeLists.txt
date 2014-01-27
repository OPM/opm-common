add_executable(runSimpleTableTests SimpleTableTests.cpp)
target_link_libraries(runSimpleTableTests Parser ${Boost_LIBRARIES})
add_test(NAME runSimpleTableTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runTimeMapTests )
