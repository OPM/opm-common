<<<<<<< HEAD
add_subdirectory( Logger )


set( rawdeck_source 
RawDeck/RawDeck.cpp 
RawDeck/RawKeyword.cpp 
RawDeck/RawRecord.cpp )


set( parser_source 
Parser/ParserRecordSize.cpp 
Parser/ParserKW.cpp 
Parser/Parser.cpp 
Parser/ItemSize.cpp )


add_library(Parser ${rawdeck_source} ${parser_source})
add_library(Logger Logger.cpp)



