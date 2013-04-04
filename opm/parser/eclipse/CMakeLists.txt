add_library(Parser RawDeck/RawDeck.cpp RawDeck/RawKeyword.cpp RawDeck/RawRecord.cpp RawDeck/RawParserKWs.cpp Parser/Parser.cpp Parser/ParserRecordSize.cpp Parser/ParserKW.cpp)
add_library(Logger Logger.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Parser)

