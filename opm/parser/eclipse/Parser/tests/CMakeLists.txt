add_executable(runParserTests ParserTests.cpp)
add_executable(runParserKeywordTests ParserKeywordTests.cpp)
add_executable(runParserRecordTests ParserRecordTests.cpp)
add_executable(runParserItemTests ParserItemTests.cpp)
add_executable(runParserEnumTests ParserEnumTests.cpp)
add_executable(runParserIncludeTests ParserIncludeTests.cpp)

target_link_libraries(runParserTests Parser ${Boost_LIBRARIES})
target_link_libraries(runParserKeywordTests Parser ${Boost_LIBRARIES})
target_link_libraries(runParserRecordTests Parser ${Boost_LIBRARIES})
target_link_libraries(runParserItemTests Parser ${Boost_LIBRARIES})
target_link_libraries(runParserIncludeTests Parser ${Boost_LIBRARIES})
target_link_libraries(runParserEnumTests Parser ${Boost_LIBRARIES})

add_test(NAME runParserTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runParserTests )
add_test(NAME runParserKeywordTests COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runParserKeywordTests )
add_test(NAME runParserRecordTests COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runParserRecordTests )
add_test(NAME runParserItemTests COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runParserItemTests )
add_test(NAME runParserIncludeTests WORKING_DIRECTORY ${EXECUTABLE_OUTPUT_PATH} COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runParserIncludeTests )
add_test(NAME runParserEnumTests COMMAND ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/runParserEnumTests )

set_property(SOURCE ParserRecordTests.cpp PROPERTY COMPILE_FLAGS "-Wno-error")

#-----------------------------------------------------------------

add_executable( createInlineItemTest createInlineItemTest.cpp ) 
target_link_libraries( createInlineItemTest buildParser ${Boost_LIBRARIES} )

add_custom_command( OUTPUT ${PROJECT_BINARY_DIR}/generated-source/inlineItemTest.cpp 
                    COMMAND createInlineItemTest ${PROJECT_BINARY_DIR}/generated-source/inlineItemTest.cpp inlineItemTest
                    DEPENDS createInlineItemTest )

add_executable( inlineItemTest ${PROJECT_BINARY_DIR}/generated-source/inlineItemTest.cpp )
add_dependencies( inlineItemTest keywordlist )
target_link_libraries( inlineItemTest buildParser ${Boost_LIBRARIES} )
add_test(inlineItemTest ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/inlineItemTest)

#-----------------------------------------------------------------

SET_SOURCE_FILES_PROPERTIES(${PROJECT_BINARY_DIR}/generated-source/inlineKeywordTest.cpp PROPERTIES GENERATED TRUE)

add_executable( inlineKeywordTest ${PROJECT_BINARY_DIR}/generated-source/inlineKeywordTest.cpp )
add_dependencies( inlineKeywordTest keywordlist )
target_link_libraries( inlineKeywordTest buildParser ${Boost_LIBRARIES} )
add_test(inlineKeywordTest ${TEST_MEMCHECK_TOOL} ${EXECUTABLE_OUTPUT_PATH}/inlineKeywordTest)
