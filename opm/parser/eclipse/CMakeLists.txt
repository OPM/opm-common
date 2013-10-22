add_subdirectory(Parser/tests)
add_subdirectory(RawDeck/tests)
add_subdirectory(Deck/tests)
add_subdirectory(IntegrationTests)
add_subdirectory(EclipseState/Schedule/tests)

add_subdirectory( Applications )

set( rawdeck_source 
RawDeck/StarToken.cpp
RawDeck/RawKeyword.cpp 
RawDeck/RawRecord.cpp )

set( deck_source
Deck/Deck.cpp
Deck/DeckKeyword.cpp
Deck/DeckRecord.cpp
Deck/DeckItem.cpp
Deck/DeckIntItem.cpp
Deck/DeckDoubleItem.cpp
Deck/DeckStringItem.cpp
Deck/KeywordContainer.cpp
)

set( parser_source 
Parser/ParserEnums.cpp
Parser/ParserKeyword.cpp 
Parser/Parser.cpp 
Parser/ParserRecord.cpp
Parser/ParserItem.cpp
Parser/ParserIntItem.cpp  
Parser/ParserBoolItem.cpp  
Parser/ParserDoubleItem.cpp  
Parser/ParserStringItem.cpp  
${PROJECT_BINARY_DIR}/generated-source/DefaultKeywordList.cpp
)

set( build_parser_source 
Parser/ParserEnums.cpp
Parser/ParserKeyword.cpp 
Parser/ParserRecord.cpp
Parser/ParserItem.cpp
Parser/ParserIntItem.cpp  
Parser/ParserBoolItem.cpp  
Parser/ParserDoubleItem.cpp  
Parser/ParserStringItem.cpp  
)

set (state_source
EclipseState/Schedule/TimeMap.cpp )

set( HEADER_FILES
RawDeck/RawConsts.hpp 
RawDeck/RawKeyword.hpp 
RawDeck/RawRecord.hpp 
RawDeck/StarToken.hpp
#
Deck/Deck.hpp
Deck/DeckKeyword.hpp
Deck/DeckRecord.hpp
Deck/DeckItem.hpp
Deck/DeckIntItem.hpp
Deck/DeckDoubleItem.hpp
Deck/DeckStringItem.hpp
Deck/KeywordContainer.hpp
#
Parser/ParserEnums.hpp
Parser/ParserKeyword.hpp 
Parser/Parser.hpp 
Parser/ParserRecord.hpp
Parser/ParserItem.hpp
Parser/ParserIntItem.hpp  
Parser/ParserBoolItem.hpp  
Parser/ParserDoubleItem.hpp  
Parser/ParserStringItem.hpp  
Parser/ParserItemTemplate.hpp
#
EclipseState/Schedule/TimeMap.hpp
)

add_library(buildParser ${rawdeck_source} ${build_parser_source} ${deck_source})
target_link_libraries(buildParser opm-json ${Boost_LIBRARIES})

#-----------------------------------------------------------------

add_executable( createDefaultKeywordList Parser/createDefaultKeywordList.cpp )
target_link_libraries( createDefaultKeywordList buildParser opm-json ${Boost_LIBRARIES} )

add_custom_command( OUTPUT ${PROJECT_BINARY_DIR}/generated-source/DefaultKeywordList.cpp
                    COMMAND createDefaultKeywordList ${PROJECT_SOURCE_DIR}/opm/parser/share/keywords ${PROJECT_BINARY_DIR}/generated-source/DefaultKeywordList.cpp
                    DEPENDS createDefaultKeywordList )


#-----------------------------------------------------------------

add_library(Parser ${rawdeck_source} ${parser_source} ${deck_source} ${state_source})
target_link_libraries(Parser opm-json ${Boost_LIBRARIES})



install( TARGETS Parser DESTINATION ${CMAKE_INSTALL_LIBDIR} )
install( FILES ${HEADER_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/opm/parser/eclipse)
