set(genkw_SOURCES src/opm/json/JsonObject.cpp
                  src/opm/parser/eclipse/Parser/createDefaultKeywordList.cpp
                  src/opm/parser/eclipse/Deck/UDAValue.cpp
                  src/opm/parser/eclipse/Deck/DeckValue.cpp
                  src/opm/parser/eclipse/Deck/Deck.cpp
                  src/opm/parser/eclipse/Deck/DeckItem.cpp
                  src/opm/parser/eclipse/Deck/DeckKeyword.cpp
                  src/opm/parser/eclipse/Deck/DeckRecord.cpp
                  src/opm/parser/eclipse/Deck/DeckOutput.cpp
                  src/opm/parser/eclipse/Generator/KeywordGenerator.cpp
                  src/opm/parser/eclipse/Generator/KeywordLoader.cpp
                  src/opm/parser/eclipse/Parser/ErrorGuard.cpp
                  src/opm/parser/eclipse/Parser/ParseContext.cpp
                  src/opm/parser/eclipse/Parser/ParserEnums.cpp
                  src/opm/parser/eclipse/Parser/ParserItem.cpp
                  src/opm/parser/eclipse/Parser/ParserKeyword.cpp
                  src/opm/parser/eclipse/Parser/ParserRecord.cpp
                  src/opm/parser/eclipse/Parser/raw/RawKeyword.cpp
                  src/opm/parser/eclipse/Parser/raw/RawRecord.cpp
                  src/opm/parser/eclipse/Parser/raw/StarToken.cpp
                  src/opm/parser/eclipse/Units/Dimension.cpp
                  src/opm/parser/eclipse/Units/UnitSystem.cpp
                  src/opm/parser/eclipse/Utility/Stringview.cpp
                  src/opm/common/OpmLog/OpmLog.cpp
                  src/opm/common/OpmLog/Logger.cpp
                  src/opm/common/OpmLog/StreamLog.cpp
                  src/opm/common/OpmLog/LogBackend.cpp
                  src/opm/common/OpmLog/LogUtil.cpp
)
if(NOT cjson_FOUND)
  list(APPEND genkw_SOURCES external/cjson/cJSON.c)
endif()
add_executable(genkw ${genkw_SOURCES})

target_link_libraries(genkw ecl Boost::regex Boost::filesystem Boost::system)

# Generate keyword list
include(src/opm/parser/eclipse/share/keywords/keyword_list.cmake)
string(REGEX REPLACE "([^;]+)" "${PROJECT_SOURCE_DIR}/src/opm/parser/eclipse/share/keywords/\\1" keyword_files "${keywords}")
configure_file(src/opm/parser/eclipse/keyword_list.argv.in keyword_list.argv)

# Generate keyword source
add_custom_command(
    OUTPUT  ${PROJECT_BINARY_DIR}/tmp_gen/ParserKeywords.cpp ${PROJECT_BINARY_DIR}/tmp_gen/inlinekw.cpp
    COMMAND genkw keyword_list.argv
                  ${PROJECT_BINARY_DIR}/tmp_gen/ParserKeywords.cpp
                  ${PROJECT_BINARY_DIR}/tmp_gen/include/
                  opm/parser/eclipse/Parser/ParserKeywords
                  ${PROJECT_BINARY_DIR}/tmp_gen/inlinekw.cpp
    DEPENDS genkw ${keyword_files} src/opm/parser/eclipse/share/keywords/keyword_list.cmake
)

# To avoid some rebuilds
add_custom_command(OUTPUT ParserKeywords.cpp inlinekw.cpp
                   DEPENDS ${PROJECT_BINARY_DIR}/tmp_gen/ParserKeywords.cpp
                   COMMAND ${CMAKE_COMMAND} -DBASE_DIR=${PROJECT_BINARY_DIR}
                                            -P ${PROJECT_SOURCE_DIR}/CopyHeaders.cmake)
