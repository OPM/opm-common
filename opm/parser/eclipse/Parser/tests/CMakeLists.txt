add_definitions( -DJSON_CONFIG_FILE="${PROJECT_SOURCE_DIR}/opm/parser/share/parser_config.json")

add_executable(runParserTests ParserTests.cpp)
add_executable(runParserKeywordTests ParserKeywordTests.cpp)
add_executable(runParserRecordTests ParserRecordTests.cpp)
add_executable(runParserItemTests ParserItemTests.cpp)
add_executable(runParserEnumTests ParserEnumTests.cpp)

target_link_libraries(runParserTests Parser ${Boost_LIBRARIES})
target_link_libraries(runParserKeywordTests Parser ${Boost_LIBRARIES})
target_link_libraries(runParserRecordTests Parser ${Boost_LIBRARIES})
target_link_libraries(runParserItemTests Parser ${Boost_LIBRARIES})
target_link_libraries(runParserEnumTests Parser ${Boost_LIBRARIES})

add_test(NAME runParserTests WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserTests )
add_test(NAME runParserKeywordTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserKeywordTests )
add_test(NAME runParserRecordTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserRecordTests )
add_test(NAME runParserItemTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserItemTests )
add_test(NAME runParserEnumTests COMMAND ${EXECUTABLE_OUTPUT_PATH}/runParserEnumTests )

set_property(SOURCE ParserRecordTests.cpp PROPERTY COMPILE_FLAGS "-Wno-error")

#-----------------------------------------------------------------

add_executable( createInlineItemTest createInlineItemTest.cpp ) 
target_link_libraries( createInlineItemTest buildParser ${Boost_LIBRARIES} )

add_custom_command( OUTPUT ${PROJECT_BINARY_DIR}/generated-source/inlineItemTest.cpp 
                    COMMAND createInlineItemTest ${PROJECT_BINARY_DIR}/generated-source/inlineItemTest.cpp inlineItemTest
                    DEPENDS createInlineItemTest )


add_executable( inlineItemTest ${PROJECT_BINARY_DIR}/generated-source/inlineItemTest.cpp )
target_link_libraries( inlineItemTest buildParser ${Boost_LIBRARIES} )
add_test(inlineItemTest ${EXECUTABLE_OUTPUT_PATH}/inlineItemTest)

#-----------------------------------------------------------------

add_executable( createInlineKeywordTest createInlineKeywordTest.cpp ) 
target_link_libraries( createInlineKeywordTest buildParser ${Boost_LIBRARIES} )

add_custom_command( OUTPUT ${PROJECT_BINARY_DIR}/generated-source/inlineKeywordTest.cpp 
                    COMMAND createInlineKeywordTest ${PROJECT_BINARY_DIR}/generated-source/inlineKeywordTest.cpp inlineKeywordTest ${PROJECT_SOURCE_DIR}/opm/parser/share/keywords 
                    DEPENDS createInlineKeywordTest )

add_executable( inlineKeywordTest ${PROJECT_BINARY_DIR}/generated-source/inlineKeywordTest.cpp )
target_link_libraries( inlineKeywordTest buildParser ${Boost_LIBRARIES} )
add_test(inlineKeywordTest ${EXECUTABLE_OUTPUT_PATH}/inlineKeywordTest)

