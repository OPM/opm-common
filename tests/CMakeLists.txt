add_executable(runParserTests ParserTests.cpp)
add_executable(runParserTestsInternalData ParserTestsInternalData.cpp)

add_executable(runRawRecordTests RawRecordTests.cpp)
add_executable(runRawKeywordTests RawKeywordTests.cpp)
add_executable(runRawDeckTests RawDeckTests.cpp)
add_executable(runRawParserKWsTests RawParserKWsTests.cpp)

add_executable(runParserKWTests ParserKWTests.cpp)
add_executable(runParserRecordSizeTests ParserRecordSizeTests.cpp)

target_link_libraries(runParserTests Parser Logger ${Boost_LIBRARIES})
target_link_libraries(runParserTestsInternalData Parser Logger ${Boost_LIBRARIES})

target_link_libraries(runRawRecordTests Parser Logger ${Boost_LIBRARIES})
target_link_libraries(runRawKeywordTests Parser Logger ${Boost_LIBRARIES})
target_link_libraries(runRawDeckTests Parser Logger ${Boost_LIBRARIES})
target_link_libraries(runRawParserKWsTests Parser Logger ${Boost_LIBRARIES})

target_link_libraries(runParserKWTests Parser Logger ${Boost_LIBRARIES})
target_link_libraries(runParserRecordSizeTests Parser Logger ${Boost_LIBRARIES})

add_test(NAME runParserTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserTests )
add_test(NAME runParserTestsInternalData WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserTestsInternalData)
set_tests_properties(runParserTestsInternalData PROPERTIES LABELS Statoil)

add_test(NAME runRawRecordTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runRawRecordTests )
set_tests_properties(runRawRecordTests PROPERTIES LABELS Raw)

add_test(NAME runRawKeywordTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runRawKeywordTests )
set_tests_properties(runRawKeywordTests PROPERTIES LABELS Raw)

add_test(NAME runRawDeckTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runRawDeckTests )
set_tests_properties(runRawDeckTests PROPERTIES LABELS Raw)

add_test(NAME runRawParserKWsTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runRawParserKWsTests )
set_tests_properties(runRawParserKWsTests PROPERTIES LABELS Raw)

add_test(NAME runParserKWTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserKWTests )
add_test(NAME runParserRecordSizeTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserRecordSizeTests )
