execute_process( COMMAND git ls-tree --name-only -r HEAD WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE KEYWORD_LIST_STRING)
string(REPLACE "\n" ";" KEYWORD_LIST ${KEYWORD_LIST_STRING})

foreach( keyword ${KEYWORD_LIST} )
    install( FILES ${keyword} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/opm/parser/eclipse RENAME ${keyword})
endforeach()
