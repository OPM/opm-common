add_subdirectory( tests )
enable_language( C )


find_package(cjson)
if (HAVE_CJSON)
    include_directories( ${CJSON_INCLUDE_DIR} )
else()
    add_library( cjson cjson/cJSON.c )
    install( TARGETS cjson DESTINATION ${CMAKE_INSTALL_LIBDIR} )
    install( FILES cjson/cJSON.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/cjson )

    set( CJSON_LIBRARY cjson )
endif()


set( json_source JsonObject.cpp )
set( json_headers JsonObject.hpp )
add_library(opm-json ${json_source})
target_link_libraries( opm-json ${CJSON_LIBRARY} ${Boost_LIBRARIES} )


install( TARGETS opm-json DESTINATION ${CMAKE_INSTALL_LIBDIR} )
foreach ( header ${json_headers} ) 
     install( FILES ${header} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/opm/json RENAME ${header})
endforeach()

