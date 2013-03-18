# Add test executable
add_executable(runUnitTests ParserTests.cpp)

target_link_libraries(runUnitTests Parser Logger ${Boost_LIBRARIES} gtest gtest_main)

add_test(NAME runUnitTests WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runUnitTests )
