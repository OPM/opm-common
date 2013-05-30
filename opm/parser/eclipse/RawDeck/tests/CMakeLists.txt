add_executable(runRawRecordTests RawRecordTests.cpp)
add_executable(runRawKeywordTests RawKeywordTests.cpp)
add_executable(runRawDeckTests RawDeckTests.cpp)
add_executable(runRawDeckInternalDataTests RawDeckInternalDataTests.cpp)
add_executable(runRawParserKWsTests RawParserKWsTests.cpp)

target_link_libraries(runRawRecordTests Parser ${Boost_LIBRARIES})
target_link_libraries(runRawKeywordTests Parser ${Boost_LIBRARIES})
target_link_libraries(runRawDeckTests Parser ${Boost_LIBRARIES})
target_link_libraries(runRawDeckInternalDataTests Parser ${Boost_LIBRARIES})
target_link_libraries(runRawParserKWsTests Parser ${Boost_LIBRARIES})

add_test(NAME runRawRecordTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runRawRecordTests )

add_test(NAME runRawKeywordTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runRawKeywordTests )

add_test(NAME runRawDeckTests WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND runRawDeckTests )

add_test(NAME runRawDeckInternalDataTests WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND runRawDeckInternalDataTests )
set_tests_properties(runRawDeckInternalDataTests PROPERTIES LABEL Statoil)

add_test(NAME runRawParserKWsTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runRawParserKWsTests )
