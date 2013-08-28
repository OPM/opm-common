# The execute_process() code below asks git for a list of all the repo
# files which should be installed; an alternative and simpler approach
# would just be:
#  
#     install( DIRECTORY keywords DESTINATION ${CMAKE_INSTALL_PREFIX}/share/opm/parser/eclipse)
# 
# The disadvantage with that is that very easily end up installing
# editor backup files and other pieces of garbage. The disadvantage with
# the current setup is the list of files uis configured during the
# install cmake configure process, and will typically be out of sync if
# you have added more keywords.

execute_process( COMMAND git ls-tree --name-only -r HEAD WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} OUTPUT_VARIABLE KEYWORD_LIST_STRING)
string(REPLACE "\n" ";" KEYWORD_LIST ${KEYWORD_LIST_STRING})

foreach( keyword ${KEYWORD_LIST} )
    install( FILES ${keyword} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/opm/parser/eclipse RENAME ${keyword})
endforeach()
