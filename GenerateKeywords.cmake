set(genkw_SOURCES lib/eclipse/Parser/createDefaultKeywordList.cpp
                  lib/eclipse/Deck/Deck.cpp
                  lib/eclipse/Deck/DeckItem.cpp
                  lib/eclipse/Deck/DeckKeyword.cpp
                  lib/eclipse/Deck/DeckRecord.cpp
                  lib/eclipse/Deck/DeckOutput.cpp
                  lib/eclipse/Generator/KeywordGenerator.cpp
                  lib/eclipse/Generator/KeywordLoader.cpp
                  lib/eclipse/Parser/MessageContainer.cpp
                  lib/eclipse/Parser/ParseContext.cpp
                  lib/eclipse/Parser/ParserEnums.cpp
                  lib/eclipse/Parser/ParserItem.cpp
                  lib/eclipse/Parser/ParserKeyword.cpp
                  lib/eclipse/Parser/ParserRecord.cpp
                  lib/eclipse/RawDeck/RawKeyword.cpp
                  lib/eclipse/RawDeck/RawRecord.cpp
                  lib/eclipse/RawDeck/StarToken.cpp
                  lib/eclipse/Units/Dimension.cpp
                  lib/eclipse/Units/UnitSystem.cpp
                  lib/eclipse/Utility/Stringview.cpp
)
add_executable(genkw ${genkw_SOURCES})

target_link_libraries(genkw opmjson ecl Boost::regex Boost::filesystem Boost::system)
target_include_directories(genkw PRIVATE lib/eclipse/include
                                         lib/json/include)

# Generate keyword list
include(lib/eclipse/share/keywords/keyword_list.cmake)
string(REGEX REPLACE "([^;]+)" "${PROJECT_SOURCE_DIR}/lib/eclipse/share/keywords/\\1" keyword_files "${keywords}")
configure_file(lib/eclipse/keyword_list.argv.in keyword_list.argv)

# Generate keyword source
add_custom_command(
    OUTPUT  ${PROJECT_BINARY_DIR}/ParserKeywords.cpp ${PROJECT_BINARY_DIR}/inlinekw.cpp
    COMMAND genkw keyword_list.argv
                  ${PROJECT_BINARY_DIR}/ParserKeywords.cpp
                  ${PROJECT_BINARY_DIR}/include/
                  opm/parser/eclipse/Parser/ParserKeywords
                  ${PROJECT_BINARY_DIR}/inlinekw.cpp
    DEPENDS genkw ${keyword_files} lib/eclipse/share/keywords/keyword_list.cmake
)
