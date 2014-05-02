add_executable(runTableTests TableTests.cpp)
target_link_libraries(runTableTests Parser ${Boost_LIBRARIES})
add_test(NAME runTableTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runTableTests )
