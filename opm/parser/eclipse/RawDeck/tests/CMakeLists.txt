add_executable(runRawRecordTests RawRecordTests.cpp)
add_executable(runRawKeywordTests RawKeywordTests.cpp)
add_executable(runRawDeckTests RawDeckTests.cpp)

target_link_libraries(runRawRecordTests Parser ${Boost_LIBRARIES})
target_link_libraries(runRawKeywordTests Parser ${Boost_LIBRARIES})
target_link_libraries(runRawDeckTests Parser ${Boost_LIBRARIES})

add_test(NAME runRawRecordTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runRawRecordTests )
add_test(NAME runRawKeywordTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runRawKeywordTests )
add_test(NAME runRawDeckTests WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND runRawDeckTests )
