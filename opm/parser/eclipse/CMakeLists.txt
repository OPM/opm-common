
add_library(Parser Parser.cpp data/RawDeck.cpp data/RawKeyword.cpp data/RawRecord.cpp ParserKW.cpp)
add_library(Logger Logger.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

