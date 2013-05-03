add_library(Parser RawDeck/RawDeck.cpp RawDeck/RawKeyword.cpp RawDeck/RawRecord.cpp Parser/ParserRecordSize.cpp Parser/ParserKW.cpp Parser/Parser.cpp Parser/ItemSize.cpp)


include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Parser)

add_subdirectory( Logger )

