# Add test executables
add_executable(runParserTests ParserTests.cpp)
add_executable(runKeywordRecordSetTests KeywordRecordSetTests.cpp)
add_executable(runParserKWTests ParserKWTests.cpp)


target_link_libraries(runParserTests Parser Logger ${Boost_LIBRARIES})
target_link_libraries(runKeywordRecordSetTests Parser Logger ${Boost_LIBRARIES})
target_link_libraries(runParserKWTests Parser Logger ${Boost_LIBRARIES})


add_test(NAME runParserTests WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserTests )
add_test(NAME runKeywordRecordSetTests WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runKeywordRecordSetTests )
add_test(NAME runParserKWTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserKWTests )
