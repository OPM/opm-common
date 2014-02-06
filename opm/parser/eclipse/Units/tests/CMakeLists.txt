add_executable(runUnitTests UnitTests.cpp)
target_link_libraries(runUnitTests Parser ${Boost_LIBRARIES})
add_test(NAME runUnitTests COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runUnitTests )
