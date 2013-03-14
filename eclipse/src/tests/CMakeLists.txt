# Add test executable
add_executable(runUnitTests ParserTests.cpp)

target_link_libraries(runUnitTests Parser ${Boost_LIBRARIES} gtest gtest_main)

add_test(runUnitTests ${EXECUTABLE_OUTPUT_PATH}/runUnitTests)
