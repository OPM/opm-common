add_executable(runParserTests ParserTests.cpp)
add_executable(runParserTestsInternalData ParserTestsInternalData.cpp)

add_executable(runRawDataTests RawDataStructuresTests.cpp)
add_executable(runRawDeckTests RawDeckTests.cpp)
add_executable(runRawParserKWsTests RawParserKWsTests.cpp)

add_executable(runParserKWTests ParserKWTests.cpp)
add_executable(runParserRecordSizeTests ParserRecordSizeTests.cpp)

target_link_libraries(runParserTests Parser Logger ${Boost_LIBRARIES})
target_link_libraries(runParserTestsInternalData Parser Logger ${Boost_LIBRARIES})

target_link_libraries(runRawDataTests Parser Logger ${Boost_LIBRARIES})
target_link_libraries(runRawDeckTests Parser Logger ${Boost_LIBRARIES})
target_link_libraries(runRawParserKWsTests Parser Logger ${Boost_LIBRARIES})
target_link_libraries(runParserKWTests Parser Logger ${Boost_LIBRARIES})
target_link_libraries(runParserRecordSizeTests Parser Logger ${Boost_LIBRARIES})


add_test(NAME runParserTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserTests )
add_test(NAME runParserTestsInternalData WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserTestsInternalData)
set_tests_properties(runParserTestsInternalData PROPERTIES LABELS Statoil)

add_test(NAME runRawDataTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runRawDataTests )
add_test(NAME runRawDeckTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runRawDeckTests )
add_test(NAME runRawParserKWsTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runRawDeckTests )
add_test(NAME runParserKWTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserKWTests )
add_test(NAME runParserRecordSizeTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserRecordSizeTests )
