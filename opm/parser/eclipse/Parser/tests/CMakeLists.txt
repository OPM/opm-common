add_executable(runParserTests ParserTests.cpp)
add_executable(runParserKWTests ParserKWTests.cpp)
add_executable(runParserRecordTests ParserRecordTests.cpp)
add_executable(runParserRecordSizeTests ParserRecordSizeTests.cpp)
add_executable(runParserItemTests ParserItemTests.cpp)
add_executable(runParserTestsInternalDataTests ParserTestsInternalData.cpp)

target_link_libraries(runParserTests Parser ${Boost_LIBRARIES})
target_link_libraries(runParserKWTests Parser ${Boost_LIBRARIES})
target_link_libraries(runParserRecordSizeTests Parser ${Boost_LIBRARIES})
target_link_libraries(runParserRecordTests Parser ${Boost_LIBRARIES})
target_link_libraries(runParserItemTests Parser ${Boost_LIBRARIES})
target_link_libraries(runParserTestsInternalDataTests Parser ${Boost_LIBRARIES})


add_test(NAME runParserTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserTests )
add_test(NAME runParserKWTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserKWTests )
add_test(NAME runParserRecordSizeTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserRecordSizeTests )
add_test(NAME runParserRecordTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserRecordTests )
add_test(NAME runParserItemTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserItemTests )
add_test(NAME runParserTestsInternalDataTests WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserTestsInternalDataTests )

set_tests_properties(runParserTestsInternalDataTests PROPERTIES LABEL Statoil)
set_property(SOURCE ParserRecordTests.cpp PROPERTY COMPILE_FLAGS "-Wno-error")
