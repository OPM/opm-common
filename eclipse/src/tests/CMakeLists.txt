# Add test executable
add_executable(runUnitTests ParserTests.cpp)

target_link_libraries(runUnitTests Parser gtest gtest_main)

add_test(runUnitTests runUnitTests )
