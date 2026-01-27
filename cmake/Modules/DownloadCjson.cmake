include(FetchContent)
FetchContent_Declare(cjson
                     DOWNLOAD_EXTRACT_TIMESTAMP ON
                     URL https://github.com/DaveGamble/cJSON/archive/refs/tags/v1.7.16.tar.gz
                     URL_HASH SHA512=3a894de03c33d89f1e7ee572418d5483c844d38e1e64aa4f6297ddaa01f4111f07601f8d26617b424b5af15d469e3955dae075d9f30b5c25e16ec348fdb06e6f)
FetchContent_Populate(cjson)

add_library(cjson OBJECT)
target_sources(cjson PRIVATE ${cjson_SOURCE_DIR}/cJSON.c)
execute_process(
  COMMAND
    ${CMAKE_COMMAND} -E create_symlink ${cjson_SOURCE_DIR} ${CMAKE_BINARY_DIR}/_deps/cjson
)
target_include_directories(cjson INTERFACE ${CMAKE_BINARY_DIR}/_deps)
