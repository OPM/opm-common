project(opm-cjson C)

add_library(cjson OBJECT cJSON.c)
set_target_properties(cjson PROPERTIES POSITION_INDEPENDENT_CODE TRUE)
target_include_directories(cjson
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
           $<INSTALL_INTERFACE:include>
)
