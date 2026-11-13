file(MAKE_DIRECTORY
  "${BASE_DIR}/generated/ParserKeywords"
  "${BASE_DIR}/generated/include/opm/input/eclipse/Parser/ParserKeywords"
)

execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${BASE_DIR}/tmp_gen/ParserInit.cpp"
                        "${BASE_DIR}/generated/ParserInit.cpp"
                COMMAND_ERROR_IS_FATAL ANY)

execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${BASE_DIR}/tmp_gen/TestKeywords.cpp"
                        "${BASE_DIR}/generated/TestKeywords.cpp"
                COMMAND_ERROR_IS_FATAL ANY)

if (OPM_ENABLE_PYTHON)
  file(MAKE_DIRECTORY "${BASE_DIR}/generated/python/cxx")
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                          "${BASE_DIR}/tmp_gen/builtin_pybind11.cpp"
                          "${BASE_DIR}/generated/python/cxx/builtin_pybind11.cpp"
                  COMMAND_ERROR_IS_FATAL ANY)
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "${BASE_DIR}/tmp_gen/include/opm/input/eclipse/Parser/ParserKeywords/Builtin.hpp"
                        "${BASE_DIR}/generated/include/opm/input/eclipse/Parser/ParserKeywords/Builtin.hpp"
                COMMAND_ERROR_IS_FATAL ANY)

file(GLOB HDRS "${BASE_DIR}/tmp_gen/include/opm/input/eclipse/Parser/ParserKeywords/*.hpp")

foreach(HDR ${HDRS})
  get_filename_component(hdr "${HDR}" NAME)
  execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                          "${HDR}"
                          "${BASE_DIR}/generated/include/opm/input/eclipse/Parser/ParserKeywords/${hdr}"
                  COMMAND_ERROR_IS_FATAL ANY)
endforeach()

foreach (name A B C D E F G H I J K L M N O P Q R S T U V W X Y Z)
  foreach (source ${name}.cpp ParserInit${name}.cpp Builtin${name}.cpp)
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                            "${BASE_DIR}/tmp_gen/ParserKeywords/${source}"
                            "${BASE_DIR}/generated/ParserKeywords/${source}"
                    COMMAND_ERROR_IS_FATAL ANY)
  endforeach()
endforeach()
