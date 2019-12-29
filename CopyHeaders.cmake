execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${BASE_DIR}/tmp_gen/ParserKeywords.cpp
                        ${BASE_DIR}/ParserKeywords.cpp)

execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${BASE_DIR}/tmp_gen/ParserInit.cpp
                        ${BASE_DIR}/ParserInit.cpp)

execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        ${BASE_DIR}/tmp_gen/inlinekw.cpp
                        ${BASE_DIR}/inlinekw.cpp)


file(GLOB HDRS ${BASE_DIR}/tmp_gen/include/opm/parser/eclipse/Parser/ParserKeywords/*.hpp)

foreach(HDR ${HDRS})
  file(RELATIVE_PATH hdr ${BASE_DIR}/tmp_gen/include/opm/parser/eclipse/Parser/ParserKeywords ${HDR})
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                          ${HDR}
                          ${BASE_DIR}/include/opm/parser/eclipse/Parser/ParserKeywords/${hdr})
endforeach()
