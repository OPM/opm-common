set( json_source JsonObject.cpp )
set( json_headers JsonObject.hpp )
if (NOT HAVE_CJSON)
    list(APPEND json_source cjson/cJSON.c)
    set( CJSON_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} )
    set(CJSON_LIBRARY "")
endif()
add_library(opm-json ${json_source})
target_link_libraries( opm-json ${CJSON_LIBRARY} ${Boost_LIBRARIES} )


install( TARGETS opm-json DESTINATION ${CMAKE_INSTALL_LIBDIR} )
foreach ( header ${json_headers} ) 
     install( FILES ${header} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/opm/json RENAME ${header})
endforeach()

if (BUILD_TESTING)
   add_subdirectory( tests )
endif()
