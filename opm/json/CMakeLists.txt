add_subdirectory( tests )

enable_language( C )
set( json_source JsonObject.cpp cjson/cJSON.c )

add_library(opm-json ${json_source})


