add_subdirectory( Logger )
add_subdirectory(Parser/tests)
add_subdirectory(RawDeck/tests)
add_subdirectory(Deck/tests)

set( rawdeck_source 
RawDeck/RawDeck.cpp 
RawDeck/RawKeyword.cpp 
RawDeck/RawRecord.cpp 
RawDeck/RawParserKWs.cpp )

set( deck_source
Deck/DeckIntItem.cpp
)

set( parser_source 
Parser/ParserRecordSize.cpp 
Parser/ParserKW.cpp 
Parser/Parser.cpp 
Parser/ParserItemSize.cpp
Parser/ParserRecord.cpp
Parser/ParserItem.cpp
Parser/ParserIntItem.cpp  
Parser/ParserBoolItem.cpp  
Parser/ParserDoubleItem.cpp  
Parser/ParserStringItem.cpp  
)


add_library(Parser ${rawdeck_source} ${parser_source} ${deck_source})



