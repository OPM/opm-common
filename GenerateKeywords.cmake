set(genkw_SOURCES src/opm/json/JsonObject.cpp
                  src/opm/input/eclipse/Parser/createDefaultKeywordList.cpp
                  src/opm/input/eclipse/Deck/UDAValue.cpp
                  src/opm/input/eclipse/Deck/DeckTree.cpp
                  src/opm/input/eclipse/Deck/DeckValue.cpp
                  src/opm/input/eclipse/Deck/Deck.cpp
                  src/opm/input/eclipse/Deck/DeckView.cpp
                  src/opm/input/eclipse/Deck/DeckItem.cpp
                  src/opm/input/eclipse/Deck/DeckKeyword.cpp
                  src/opm/input/eclipse/Deck/DeckRecord.cpp
                  src/opm/input/eclipse/Deck/DeckOutput.cpp
                  src/opm/input/eclipse/Generator/KeywordGenerator.cpp
                  src/opm/input/eclipse/Generator/KeywordLoader.cpp
                  src/opm/input/eclipse/Schedule/UDQ/UDQEnums.cpp
                  src/opm/input/eclipse/Parser/ErrorGuard.cpp
                  src/opm/input/eclipse/Parser/ParseContext.cpp
                  src/opm/input/eclipse/Parser/ParserEnums.cpp
                  src/opm/input/eclipse/Parser/ParserItem.cpp
                  src/opm/input/eclipse/Parser/ParserKeyword.cpp
                  src/opm/input/eclipse/Parser/ParserRecord.cpp
                  src/opm/input/eclipse/Parser/raw/RawKeyword.cpp
                  src/opm/input/eclipse/Parser/raw/RawRecord.cpp
                  src/opm/input/eclipse/Parser/raw/StarToken.cpp
                  src/opm/input/eclipse/Units/Dimension.cpp
                  src/opm/input/eclipse/Units/UnitSystem.cpp
                  src/opm/common/utility/OpmInputError.cpp
                  src/opm/common/utility/shmatch.cpp
                  src/opm/common/OpmLog/OpmLog.cpp
                  src/opm/common/OpmLog/Logger.cpp
                  src/opm/common/OpmLog/StreamLog.cpp
                  src/opm/common/OpmLog/LogBackend.cpp
                  src/opm/common/OpmLog/LogUtil.cpp
)
if(NOT cjson_FOUND)
  list(APPEND genkw_SOURCES external/cjson/cJSON.c)
endif()
add_executable(genkw ${genkw_SOURCES})

target_link_libraries(genkw ${opm-common_LIBRARIES})

# Generate keyword list
include(src/opm/input/eclipse/share/keywords/keyword_list.cmake)
string(REGEX REPLACE "([^;]+)" "${PROJECT_SOURCE_DIR}/src/opm/input/eclipse/share/keywords/\\1" keyword_files "${keywords}")
configure_file(src/opm/input/eclipse/keyword_list.argv.in keyword_list.argv)

# Generate keyword source

set( genkw_argv keyword_list.argv
  ${PROJECT_BINARY_DIR}/tmp_gen/ParserKeywords
  ${PROJECT_BINARY_DIR}/tmp_gen/ParserInit.cpp
  ${PROJECT_BINARY_DIR}/tmp_gen/include/
  opm/input/eclipse/Parser/ParserKeywords
  ${PROJECT_BINARY_DIR}/tmp_gen/TestKeywords.cpp)

foreach (name A B C D E F G H I J K L M N O P Q R S T U V W X Y Z)
  list(APPEND _tmp_output ${PROJECT_BINARY_DIR}/tmp_gen/ParserKeywords/${name}.cpp
                          ${PROJECT_BINARY_DIR}/tmp_gen/include/opm/input/eclipse/Parser/ParserKeywords/${name}.hpp)
  list(APPEND _target_output ${PROJECT_BINARY_DIR}/ParserKeywords/${name}.cpp
                             ${PROJECT_BINARY_DIR}/include/opm/input/eclipse/Parser/ParserKeywords/${name}.hpp)
endforeach()

foreach(name TestKeywords.cpp ParserInit.cpp)
  list(APPEND _target_output ${PROJECT_BINARY_DIR}/${name})
  list(APPEND _tmp_output ${PROJECT_BINARY_DIR}/tmp_gen/${name})
endforeach()

list(APPEND _target_output ${PROJECT_BINARY_DIR}/include/opm/input/eclipse/Parser/ParserKeywords/Builtin.hpp)
list(APPEND _tmp_output ${PROJECT_BINARY_DIR}/tmp_gen/include/opm/input/eclipse/Parser/ParserKeywords/Builtin.hpp)

if (OPM_ENABLE_PYTHON)
  list(APPEND genkw_argv ${PROJECT_BINARY_DIR}/tmp_gen/builtin_pybind11.cpp)
  list(APPEND _tmp_output ${PROJECT_BINARY_DIR}/tmp_gen/builtin_pybind11.cpp)
  list(APPEND _target_output ${PROJECT_BINARY_DIR}/python/cxx/builtin_pybind11.cpp)
endif()

add_custom_command( OUTPUT
  ${_tmp_output}
  COMMAND genkw ${genkw_argv}
  DEPENDS genkw ${keyword_files} src/opm/input/eclipse/share/keywords/keyword_list.cmake)

# To avoid some rebuilds
add_custom_command(OUTPUT
  ${_target_output}
  DEPENDS ${_tmp_output}
  COMMAND ${CMAKE_COMMAND} -DBASE_DIR=${PROJECT_BINARY_DIR} -P ${PROJECT_SOURCE_DIR}/CopyHeaders.cmake)
