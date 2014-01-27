add_executable(runStarTokenTests StarTokenTests.cpp)
add_executable(runRawRecordTests RawRecordTests.cpp)
add_executable(runRawKeywordTests RawKeywordTests.cpp)

target_link_libraries(runStarTokenTests Parser ${Boost_LIBRARIES})
target_link_libraries(runRawRecordTests Parser ${Boost_LIBRARIES})
target_link_libraries(runRawKeywordTests Parser ${Boost_LIBRARIES})

add_test(NAME runRawRecordTests COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runRawRecordTests )
add_test(NAME runRawKeywordTests COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runRawKeywordTests )
add_test(NAME runStarTokenTests COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runStarTokenTests )
