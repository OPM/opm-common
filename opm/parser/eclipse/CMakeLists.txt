add_subdirectory( Logger )
add_subdirectory(Parser/tests)
add_subdirectory(RawDeck/tests)

set( rawdeck_source 
RawDeck/RawDeck.cpp 
RawDeck/RawKeyword.cpp 
RawDeck/RawRecord.cpp 
RawDeck/RawParserKWs.cpp )

set( parser_source 
Parser/ParserRecordSize.cpp 
Parser/ParserKW.cpp 
Parser/Parser.cpp 
Parser/ParserItemSize.cpp )

add_library(Parser ${rawdeck_source} ${parser_source})



