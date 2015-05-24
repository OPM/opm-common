add_executable(runIOConfigTests IOConfigTest.cpp)
target_link_libraries(runIOConfigTests Parser ${Boost_LIBRARIES})
add_test(NAME runIOConfigTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runIOConfigTests )
