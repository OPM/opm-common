foreach(tapp ParserTests ParserKeywordTests ParserRecordTests
             ParserItemTests ParserEnumTests ParserIncludeTests)
  opm_add_test(run${tapp} SOURCES ${tapp}.cpp
                          LIBRARIES Parser ${Boost_LIBRARIES})
endforeach()
set_property(SOURCE ParserRecordTests.cpp PROPERTY COMPILE_FLAGS "-Wno-error")

#-----------------------------------------------------------------

add_executable( createInlineItemTest EXCLUDE_FROM_ALL createInlineItemTest.cpp)
target_link_libraries( createInlineItemTest Parser ${Boost_LIBRARIES})

add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/generated-source/inlineItemTest.cpp
                   COMMAND createInlineItemTest ${PROJECT_BINARY_DIR}/generated-source/inlineItemTest.cpp inlineItemTest
                   DEPENDS createInlineItemTest )

foreach(tapp inlineItemTest inlineKeywordTest)
  opm_add_test(run${tapp}
               SOURCES ${PROJECT_BINARY_DIR}/generated-source/${tapp}.cpp
               LIBRARIES buildParser ${Boost_LIBRARIES}
               DEPENDS keywordlist)
endforeach()

#-----------------------------------------------------------------

set_source_files_properties(${PROJECT_BINARY_DIR}/generated-source/inlineKeywordTest.cpp PROPERTIES GENERATED TRUE)
set_source_files_properties(${PROJECT_BINARY_DIR}/generated-source/inlineItemTest.cpp PROPERTIES GENERATED TRUE)
