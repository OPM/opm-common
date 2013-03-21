# Add test executables
add_executable(runParserTests ParserTests.cpp)
add_executable(runKeywordRecordSetTests KeywordRecordSetTests.cpp)

target_link_libraries(runParserTests Parser Logger ${Boost_LIBRARIES})
target_link_libraries(runKeywordRecordSetTests Parser Logger ${Boost_LIBRARIES})


add_test(NAME runParserTests WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserTests )
add_test(NAME runKeywordRecordSetTests WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runKeywordRecordSetTests )

