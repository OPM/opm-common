add_executable(runParserTests ParserTests.cpp)
add_executable(runRawDataTests RawDataStructuresTests.cpp)
add_executable(runParserKWTests ParserKWTests.cpp)
add_executable(runParserRecordSizeTests ParserRecordSizeTests.cpp)

target_link_libraries(runParserTests Parser Logger ${Boost_LIBRARIES})
target_link_libraries(runRawDataTests Parser Logger ${Boost_LIBRARIES})
target_link_libraries(runParserKWTests Parser Logger ${Boost_LIBRARIES})
target_link_libraries(runParserRecordSizeTests Parser Logger ${Boost_LIBRARIES})


add_test(NAME runParserTests WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserTests )
add_test(NAME runRawDataTests WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runRawDataTests )
add_test(NAME runParserKWTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserKWTests )
add_test(NAME runParserRecordSizeTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserRecordSizeTests )
