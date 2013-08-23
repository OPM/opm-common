add_definitions( -DJSON_CONFIG_FILE="${PROJECT_SOURCE_DIR}/opm/parser/share/parser_config.json")
add_executable(eclipsedatadoctor EclipseDataDoctor.cpp)
target_link_libraries(eclipsedatadoctor Parser)

