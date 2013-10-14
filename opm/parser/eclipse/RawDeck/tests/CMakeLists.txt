add_executable(runStarTokenTests StarTokenTests.cpp)
add_executable(runRawRecordTests RawRecordTests.cpp)
add_executable(runRawKeywordTests RawKeywordTests.cpp)

target_link_libraries(runStarTokenTests Parser ${Boost_LIBRARIES})
target_link_libraries(runRawRecordTests Parser ${Boost_LIBRARIES})
target_link_libraries(runRawKeywordTests Parser ${Boost_LIBRARIES})

add_test(NAME runRawRecordTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runRawRecordTests )
add_test(NAME runRawKeywordTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runRawKeywordTests )
add_test(NAME runStarTokenTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runStarTokenTests )
