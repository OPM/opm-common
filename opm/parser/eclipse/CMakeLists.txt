add_library(Parser data/RawDeck.cpp data/RawKeyword.cpp data/RawRecord.cpp Parser/Parser.cpp Parser/ParserRecordSize.cpp Parser/ParserKW.cpp)
add_library(Logger Logger.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Parser)

