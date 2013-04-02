
add_library(Parser Parser.cpp RawDeck/RawDeck.cpp RawDeck/RawKeyword.cpp RawDeck/RawRecord.cpp)
add_library(Logger Logger.cpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

