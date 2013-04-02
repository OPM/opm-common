# Add test executables
add_executable(runParserTests ParserTests.cpp)
add_executable(runRawDataTests RawDataStructuresTests.cpp)

target_link_libraries(runParserTests Parser Logger ${Boost_LIBRARIES})
target_link_libraries(runRawDataTests Parser Logger ${Boost_LIBRARIES})


add_test(NAME runParserTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserTests )
add_test(NAME runRawDataTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runRawDataTests )

