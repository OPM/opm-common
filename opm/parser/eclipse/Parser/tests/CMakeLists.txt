foreach(tapp ParserTests ParserKeywordTests ParserRecordTests
             ParserItemTests ParserEnumTests ParserIncludeTests)
  opm_add_test(run${tapp} SOURCES ${tapp}.cpp
                          LIBRARIES opmparser ${Boost_LIBRARIES})
endforeach()
set_property(SOURCE ParserRecordTests.cpp PROPERTY COMPILE_FLAGS "-Wno-error")

set_source_files_properties( ${PROJECT_BINARY_DIR}/generated-source/inlineKeywordTest.cpp PROPERTIES GENERATED TRUE)

opm_add_test( runInlineKeywordTest SOURCES ${PROJECT_BINARY_DIR}/generated-source/inlineKeywordTest.cpp
              LIBRARIES opmparser ${Boost_LIBRARIES}
              DEPENDS generatedCode )



